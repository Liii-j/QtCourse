#include "mainwindow.h"
#include "codeeditor.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QClipboard>
#include <QColorDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontDialog>
#include <QGuiApplication>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QStatusBar>
#include <QStringConverter>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QTime>
#include <QTimer>
#include <QToolBar>

// 定时自动保存的间隔（毫秒），想改频率只改这一处
static const int kAutoSaveIntervalMs = 30 * 1000;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. 编辑器默认字体（等宽，和行号对齐好看）
    QFont editorFont(QStringLiteral("Consolas"), 12);
    editorFont.setStyleHint(QFont::Monospace);
    ui->textEdit->setFont(editorFont);
    ui->textEdit->setTabStopDistance(
        4 * ui->textEdit->fontMetrics().horizontalAdvance(QLatin1Char(' ')));

    // 2. 文档被修改 / 已保存时，自动刷新窗口标题
    connect(ui->textEdit->document(), &QTextDocument::modificationChanged,
            this, &MainWindow::updateWindowTitle);
    connect(ui->textEdit, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateCursorStatus);

    // 状态栏右侧常驻显示行列数，不会被 showMessage 的提示冲掉
    m_statusLabel = new QLabel(this);
    ui->statusbar->addPermanentWidget(m_statusLabel);

    // 3. 工具栏：直接复用菜单里的 action（图标、快捷键、勾选状态都是同一份）
    m_toolBar = addToolBar(tr("工具栏"));
    m_toolBar->setObjectName(QStringLiteral("mainToolBar"));
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolBar->addAction(ui->actFileNew);
    m_toolBar->addAction(ui->actFileOpen);
    m_toolBar->addAction(ui->actFileSave);
    m_toolBar->addSeparator();
    m_toolBar->addAction(ui->actEditUndo);
    m_toolBar->addAction(ui->actEditRedo);
    m_toolBar->addSeparator();
    m_toolBar->addAction(ui->actEditCut);
    m_toolBar->addAction(ui->actEditCopy);
    m_toolBar->addAction(ui->actEditPaste);
    m_toolBar->addSeparator();
    m_toolBar->addAction(ui->actFont);
    m_toolBar->addAction(ui->actFontColor);
    m_toolBar->addSeparator();
    m_toolBar->addAction(ui->actLineNumber);
    m_toolBar->addAction(ui->actLineWrapAuto);

    // 4. 定时自动保存：每 kAutoSaveIntervalMs 检查一次，有改动才写盘
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(kAutoSaveIntervalMs);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::doAutoSave);

    // ui 文件里 actAutoSave 的 checked 默认是 true，但那时 setChecked 早就发过 toggled 了
    // （构造函数还没跑，定时器还不存在），所以这里按当前勾选状态补一次启动
    if (ui->actAutoSave->isChecked())
        m_autoSaveTimer->start();

    // 5. 菜单勾选状态与界面保持一致
    ui->actLineWrapAuto->setChecked(true);   // QPlainTextEdit 默认按窗口宽度换行
    ui->actLineNumber->setChecked(false);
    ui->actToolBar->setChecked(true);
    ui->actStatusBar->setChecked(true);

    // 6. Action 的使能状态跟随编辑区状态：没选中就灰掉剪切/复制，
    //    撤销栈空了就灰掉撤销，剪贴板里没有文字就灰掉粘贴
    connect(ui->textEdit, &QPlainTextEdit::copyAvailable,
            this, &MainWindow::updateEditActions);
    connect(ui->textEdit, &QPlainTextEdit::undoAvailable,
            this, &MainWindow::updateEditActions);
    connect(ui->textEdit, &QPlainTextEdit::redoAvailable,
            this, &MainWindow::updateEditActions);
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged,
            this, &MainWindow::updateEditActions);

    updateWindowTitle();
    updateCursorStatus();
    updateEditActions();   // 启动时先算一次，否则几个按钮默认全是亮的
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ------------------------------------------------------------------ 文件 --
void MainWindow::on_actFileNew_triggered()
{
    if (!maybeSave())
        return;

    ui->textEdit->clear();
    m_currentFile.clear();
    ui->textEdit->document()->setModified(false);
    updateWindowTitle();
    updateCursorStatus();
}

void MainWindow::on_actFileOpen_triggered()
{
    if (!maybeSave())
        return;

    const QString fileName = QFileDialog::getOpenFileName(
        this, tr("打开文件"), QString(),
        tr("文本文件 (*.txt);;所有文件 (*.*)"));
    if (fileName.isEmpty())
        return;

    loadFromFile(fileName);
}

void MainWindow::on_actFileSave_triggered()
{
    saveFile();
}

void MainWindow::on_actFileSaveAs_triggered()
{
    saveFileAs();
}

void MainWindow::on_actFileExit_triggered()
{
    close();
}

// 菜单里的「自动保存」勾选框：控制定时器的启停
void MainWindow::on_actAutoSave_toggled(bool checked)
{
    if (!m_autoSaveTimer)
        return;

    if (checked) {
        m_autoSaveTimer->start();
        ui->statusbar->showMessage(
            tr("已开启自动保存，每 %1 秒检查一次").arg(kAutoSaveIntervalMs / 1000), 3000);
    } else {
        m_autoSaveTimer->stop();
        ui->statusbar->showMessage(tr("已关闭自动保存"), 3000);
    }
}

// 定时器到点：没有改动就什么都不做；还没保存过的新文档则提示用户先手动保存
void MainWindow::doAutoSave()
{
    if (!ui->textEdit->document()->isModified())
        return;

    if (m_currentFile.isEmpty()) {
        ui->statusbar->showMessage(
            tr("自动保存跳过：文件尚未保存过，请先按 Ctrl+S 指定保存位置"), 4000);
        return;
    }

    if (saveToFile(m_currentFile)) {
        ui->statusbar->showMessage(
            tr("已自动保存：%1（%2）")
                .arg(QFileInfo(m_currentFile).fileName(),
                     QTime::currentTime().toString(QStringLiteral("HH:mm:ss"))),
            3000);
    }
}

bool MainWindow::maybeSave()
{
    if (!ui->textEdit->document()->isModified())
        return true;

    const QMessageBox::StandardButton ret = QMessageBox::warning(
        this, tr("文本编辑器"),
        tr("文档已被修改，是否保存？"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
        return saveFile();
    if (ret == QMessageBox::Cancel)
        return false;
    return true;
}

bool MainWindow::saveFile()
{
    if (m_currentFile.isEmpty())
        return saveFileAs();

    if (!saveToFile(m_currentFile))
        return false;

    ui->statusbar->showMessage(tr("已保存：%1").arg(QFileInfo(m_currentFile).fileName()), 3000);
    return true;
}

bool MainWindow::saveFileAs()
{
    const QString fileName = QFileDialog::getSaveFileName(
        this, tr("另存为"),
        m_currentFile.isEmpty() ? tr("无标题.txt") : m_currentFile,
        tr("文本文件 (*.txt);;所有文件 (*.*)"));
    if (fileName.isEmpty())
        return false;

    if (!saveToFile(fileName))
        return false;

    ui->statusbar->showMessage(tr("已保存：%1").arg(QFileInfo(fileName).fileName()), 3000);
    return true;
}

bool MainWindow::saveToFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(this, tr("保存失败"),
                             tr("无法写入文件：\n%1\n\n%2").arg(fileName, file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << ui->textEdit->toPlainText();
    file.close();

    m_currentFile = fileName;
    ui->textEdit->document()->setModified(false);   // 标题里的 * 会自动消失
    updateWindowTitle();
    return true;
}

bool MainWindow::loadFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("打开失败"),
                             tr("无法读取文件：\n%1\n\n%2").arg(fileName, file.errorString()));
        return false;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString text = in.readAll();
    file.close();

    if (text.startsWith(QChar(0xFEFF)))   // 去掉 UTF-8 BOM
        text.remove(0, 1);

    ui->textEdit->setPlainText(text);     // 文本里的 \n 本身就是换行，QPlainTextEdit 原生支持
    m_currentFile = fileName;
    ui->textEdit->document()->setModified(false);
    updateWindowTitle();
    updateCursorStatus();
    ui->statusbar->showMessage(tr("已打开：%1").arg(QFileInfo(fileName).fileName()), 3000);
    return true;
}

// ------------------------------------------------------------------ 编辑 --
void MainWindow::on_actEditUndo_triggered()
{
    ui->textEdit->undo();
}

void MainWindow::on_actEditRedo_triggered()
{
    ui->textEdit->redo();
}

void MainWindow::on_actEditCut_triggered()
{
    ui->textEdit->cut();
}

void MainWindow::on_actEditCopy_triggered()
{
    ui->textEdit->copy();
}

void MainWindow::on_actEditPaste_triggered()
{
    ui->textEdit->paste();
}

void MainWindow::on_actEditSelectAll_triggered()
{
    ui->textEdit->selectAll();
}

void MainWindow::on_actEditFind_triggered()
{
    bool ok = false;
    const QString text = QInputDialog::getText(this, tr("查找"), tr("查找内容："),
                                               QLineEdit::Normal, m_lastFindText, &ok);
    if (!ok || text.isEmpty())
        return;

    m_lastFindText = text;

    // 先往后找，找不到就从文档开头再找一次（循环查找）
    if (ui->textEdit->find(text)) {
        ui->statusbar->showMessage(tr("已找到「%1」，再次查找请按 Ctrl+F").arg(text), 3000);
        return;
    }

    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    ui->textEdit->setTextCursor(cursor);

    if (ui->textEdit->find(text))
        ui->statusbar->showMessage(tr("已从头找到「%1」").arg(text), 3000);
    else
        ui->statusbar->showMessage(tr("找不到「%1」").arg(text), 3000);
}

void MainWindow::on_actEditReplace_triggered()
{
    bool ok = false;
    const QString from = QInputDialog::getText(this, tr("替换"), tr("查找内容："),
                                               QLineEdit::Normal, m_lastFindText, &ok);
    if (!ok || from.isEmpty())
        return;

    m_lastFindText = from;

    const QString to = QInputDialog::getText(this, tr("替换"), tr("替换为："),
                                             QLineEdit::Normal, QString(), &ok);
    if (!ok)
        return;

    // 一次性全部替换，包在一个编辑块里，撤销一次就能整体回退
    QTextCursor cursor(ui->textEdit->document());
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Start);

    int count = 0;
    forever {
        QTextCursor found = ui->textEdit->document()->find(from, cursor);
        if (found.isNull())
            break;
        found.insertText(to);
        cursor = found;
        ++count;
    }
    cursor.endEditBlock();

    if (count > 0)
        ui->statusbar->showMessage(tr("共替换 %1 处「%2」").arg(count).arg(from), 3000);
    else
        ui->statusbar->showMessage(tr("找不到「%1」").arg(from), 3000);
}

// ------------------------------------------------------------------ 格式 --
void MainWindow::on_actLineWrapAuto_toggled(bool checked)
{
    ui->textEdit->setLineWrapMode(checked ? QPlainTextEdit::WidgetWidth
                                          : QPlainTextEdit::NoWrap);
}

void MainWindow::on_actLineNumber_toggled(bool checked)
{
    ui->textEdit->setLineNumberVisible(checked);
}

void MainWindow::on_actFont_triggered()
{
    bool ok = false;
    const QFont font = QFontDialog::getFont(&ok, ui->textEdit->font(), this, tr("选择字体"));
    if (!ok)
        return;

    ui->textEdit->setFont(font);          // 字体、字号对整个文档生效
    ui->textEdit->setTabStopDistance(
        4 * ui->textEdit->fontMetrics().horizontalAdvance(QLatin1Char(' ')));
    ui->textEdit->refreshLineNumberArea();
}

void MainWindow::on_actFontColor_triggered()
{
    const QColor color = QColorDialog::getColor(
        ui->textEdit->currentCharFormat().foreground().color(), this, tr("选择字颜色"));
    if (!color.isValid())
        return;

    QTextCharFormat fmt;
    fmt.setForeground(color);
    // 有选中内容就作用于选中部分，没选中则作用于随后输入的文本
    ui->textEdit->mergeCurrentCharFormat(fmt);
}

void MainWindow::on_actFontBackgroundColor_triggered()
{
    const QColor color = QColorDialog::getColor(
        ui->textEdit->currentCharFormat().background().color(), this, tr("选择字体背景色"));
    if (!color.isValid())
        return;

    QTextCharFormat fmt;
    fmt.setBackground(color);
    ui->textEdit->mergeCurrentCharFormat(fmt);
}

void MainWindow::on_actEditBackgroundColor_triggered()
{
    const QColor color = QColorDialog::getColor(
        ui->textEdit->editorBackgroundColor(), this, tr("选择编辑器背景色"));
    if (!color.isValid())
        return;

    ui->textEdit->setEditorBackgroundColor(color);
}

// ------------------------------------------------------------------ 查看 --
void MainWindow::on_actToolBar_toggled(bool checked)
{
    if (m_toolBar)
        m_toolBar->setVisible(checked);
}

void MainWindow::on_actStatusBar_toggled(bool checked)
{
    ui->statusbar->setVisible(checked);
}

// -------------------------------------------------------- 标题 / 状态栏 --
void MainWindow::updateWindowTitle()
{
    const QString name = m_currentFile.isEmpty()
                             ? tr("无标题")
                             : QFileInfo(m_currentFile).fileName();
    const QString mark = ui->textEdit->document()->isModified() ? QStringLiteral(" *")
                                                               : QString();

    setWindowTitle(name + mark + QStringLiteral(" - 文本编辑器"));
}

void MainWindow::updateCursorStatus()
{
    if (!m_statusLabel)
        return;

    const QTextCursor cursor = ui->textEdit->textCursor();
    m_statusLabel->setText(tr("行 %1，列 %2    共 %3 行")
                               .arg(cursor.blockNumber() + 1)
                               .arg(cursor.columnNumber() + 1)
                               .arg(ui->textEdit->blockCount()));
}

// ------------------------------------------------------------ Action 使能 --
// 剪切/复制/粘贴/撤销/重做 的可用状态都只在这里算，避免多处判断不一致
void MainWindow::updateEditActions()
{
    const bool hasSelection = ui->textEdit->textCursor().hasSelection();
    ui->actEditCut->setEnabled(hasSelection);
    ui->actEditCopy->setEnabled(hasSelection);

    // 撤销/重做栈由 QPlainTextEdit 内部维护，直接问它比自己去猜准
    const QTextDocument *doc = ui->textEdit->document();
    ui->actEditUndo->setEnabled(doc->isUndoAvailable());
    ui->actEditRedo->setEnabled(doc->isRedoAvailable());

    const QMimeData *mime = QGuiApplication::clipboard()->mimeData();
    ui->actEditPaste->setEnabled(mime && mime->hasText());
}

// ------------------------------------------------------------------ 帮助 --
void MainWindow::on_actHelpAbout_triggered()
{
    QMessageBox::about(this, tr("关于"),
                       tr("<h3>简单文本编辑器</h3>"
                          "<p>开发者：李嘉俊</p>"
                          "<p>学号：2024414290315</p>").arg(qVersion()));
}

// ------------------------------------------------------------------ 事件 --
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}
