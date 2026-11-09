#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QLabel;
class QTimer;
class QToolBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // 文件
    void on_actFileNew_triggered();
    void on_actFileOpen_triggered();
    void on_actFileSave_triggered();
    void on_actFileSaveAs_triggered();
    void on_actFileExit_triggered();
    void on_actAutoSave_toggled(bool checked);
    void doAutoSave();          // 定时器到点：有改动且已有文件路径才写盘

    // 编辑
    void on_actEditUndo_triggered();
    void on_actEditRedo_triggered();
    void on_actEditCut_triggered();
    void on_actEditCopy_triggered();
    void on_actEditPaste_triggered();
    void on_actEditSelectAll_triggered();
    void on_actEditFind_triggered();
    void on_actEditReplace_triggered();

    // 格式
    void on_actLineWrapAuto_toggled(bool checked);
    void on_actLineNumber_toggled(bool checked);
    void on_actFont_triggered();
    void on_actFontColor_triggered();
    void on_actFontBackgroundColor_triggered();
    void on_actEditBackgroundColor_triggered();

    // 查看
    void on_actToolBar_toggled(bool checked);
    void on_actStatusBar_toggled(bool checked);

    // 帮助
    void on_actHelpAbout_triggered();

    // 内部同步
    void updateWindowTitle();
    void updateCursorStatus();
    void updateEditActions();   // 剪切/复制/粘贴/撤销/重做的使能状态

private:
    bool maybeSave();
    bool saveFile();
    bool saveFileAs();
    bool saveToFile(const QString &fileName);
    bool loadFromFile(const QString &fileName);

    Ui::MainWindow *ui;
    QString m_currentFile;      // 当前文件路径，为空表示还没保存过
    QToolBar *m_toolBar = nullptr;
    QLabel *m_statusLabel = nullptr;   // 状态栏右侧常驻的行列信息
    QTimer *m_autoSaveTimer = nullptr; // 定时自动保存
    QString m_lastFindText;            // 上次查找的内容，方便连续查找
};

#endif // MAINWINDOW_H
