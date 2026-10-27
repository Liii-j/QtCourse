/**
 * @file    mainwindow.cpp
 * @brief   带键盘事件的计算器 —— 主窗口实现。
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QKeyEvent>
#include <QList>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QtGlobal>

// =====================================================================
//  构造 / 析构
// =====================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_entry(QStringLiteral("0"))
    , m_accumulator(0.0)
    , m_pendingOp(Operator::None)
    , m_freshEntry(true)
    , m_error(false)
{
    ui->setupUi(this);
    // 记住结果行的正常字体，显示错误提示时临时缩小
    m_resultFont = ui->labelResult->font();
    setupButtons();
    refreshDisplay();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// =====================================================================
//  初始化：按钮统一配置
// =====================================================================

void MainWindow::setupButtons()
{
    const QList<QPushButton *> buttons = findChildren<QPushButton *>();

    for (QPushButton *button : buttons) {
        // 键盘焦点始终留在窗口上：否则某个按钮获得焦点后，空格/回车会被它"抢走"
        button->setFocusPolicy(Qt::NoFocus);
        // 让按钮随窗口一起等比缩放
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // UI 设计器中为每个按钮设置了动态属性 key，作为该按钮的按键标识
        const QString keyId = button->property("key").toString();
        if (!keyId.isEmpty()) {
            m_keyToButton.insert(keyId, button);
        }

        // 24 个按钮共用一个槽函数，槽内通过 sender() 判断按下的具体按钮，
        // 避免写 24 个 connect 和 24 个槽（大段重复代码）
        connect(button, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
    }
}

// =====================================================================
//  按键处理
// =====================================================================

void MainWindow::onButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (button == nullptr) {
        return;
    }
    // 取出按钮的按键标识，转入统一处理入口
    handleKeyInput(button->property("key").toString());
}

void MainWindow::handleKeyInput(const QString &keyId)
{
    // 错误状态（如除数为 0）下只允许"清除"，其余按键一律忽略，
    // 避免用户在没有明确清除的情况下继续用错误结果参与运算
    if (m_error) {
        if (keyId == QLatin1String("clear") || keyId == QLatin1String("ce")) {
            clearAll();
        }
        return;
    }

    if (keyId.size() == 1
        && keyId.at(0) >= QLatin1Char('0') && keyId.at(0) <= QLatin1Char('9')) {
        inputDigit(keyId);                          // 数字 0 ~ 9
    } else if (keyId == QLatin1String(".")) {
        inputDot();                                 // 小数点
    } else if (keyId == QLatin1String("back")) {
        backspace();                                // 退格
    } else if (keyId == QLatin1String("ce")) {
        clearEntry();                               // CE：清除当前输入
    } else if (keyId == QLatin1String("clear")) {
        clearAll();                                 // C：全部清除
    } else if (keyId == QLatin1String("+")) {
        inputOperator(Operator::Add);
    } else if (keyId == QLatin1String("-")) {
        inputOperator(Operator::Subtract);
    } else if (keyId == QLatin1String("*")) {
        inputOperator(Operator::Multiply);
    } else if (keyId == QLatin1String("/")) {
        inputOperator(Operator::Divide);
    } else if (keyId == QLatin1String("=")) {
        inputEquals();
    }
}

void MainWindow::inputDigit(const QString &digit)
{
    if (m_freshEntry) {
        // 没有待执行的运算，说明上一次计算已经结束（按过 "="），
        // 此时输入数字代表开始一次全新的计算，要把上一次的算式行清掉
        if (m_pendingOp == Operator::None) {
            m_expression.clear();
        }
        // 开始输入新的操作数，直接替换掉屏幕上旧的内容
        m_entry = digit;
        m_freshEntry = false;
    } else if (m_entry == QLatin1String("0")) {
        m_entry = digit;                            // 避免出现 "0123" 这样的前导零
    } else if (m_entry.size() < kMaxEntryLength) {
        m_entry += digit;                           // 超出长度上限后忽略按键
    }

    refreshDisplay();
}

void MainWindow::inputDot()
{
    if (m_freshEntry) {
        if (m_pendingOp == Operator::None) {
            m_expression.clear();                   // 同理：开始一次全新的计算
        }
        // 新的操作数：直接以 "0." 起步
        m_entry = QStringLiteral("0.");
        m_freshEntry = false;
    } else if (!m_entry.contains(QLatin1Char('.'))) {
        // 关键点：同一个操作数内只要已经出现过小数点就忽略本次输入，
        // 这样即使用户连续点击 "." 也不会出现 "1.2.3" 这种非法数据
        m_entry += QLatin1Char('.');
    }

    refreshDisplay();
}

void MainWindow::backspace()
{
    if (m_freshEntry) {
        // 当前显示的是运算结果或尚未输入的新操作数，退格无意义，直接忽略
        return;
    }

    m_entry.chop(1);
    if (m_entry.isEmpty()) {
        // 已退格到空，回到初始状态
        m_entry = QStringLiteral("0");
        m_freshEntry = true;
    }

    refreshDisplay();
}

void MainWindow::clearEntry()
{
    // CE 只清除当前操作数，保留第一个操作数和待执行的运算符
    m_entry = QStringLiteral("0");
    m_freshEntry = true;

    refreshDisplay();
}

void MainWindow::clearAll()
{
    // C 清除全部状态，回到初始状态
    m_entry = QStringLiteral("0");
    m_accumulator = 0.0;
    m_pendingOp = Operator::None;
    m_freshEntry = true;
    m_error = false;
    m_expression.clear();

    refreshDisplay();
}

// =====================================================================
//  四则运算与双操作数状态管理
// =====================================================================

void MainWindow::inputOperator(Operator op)
{
    // 情况一：连续按下运算符（如 "5 + ×"），此时 m_freshEntry 为 true，
    //         说明用户还没输入第二个操作数，只替换运算符，不做运算
    // 情况二：已经输入了第二个操作数，先算出中间结果，支持 "2 + 3 × 4" 这类连续运算
    if (m_pendingOp != Operator::None && !m_freshEntry) {
        double intermediate = 0.0;
        if (!compute(m_pendingOp, m_accumulator, currentValue(), &intermediate)) {
            return;                                  // 出错（除数为 0）时已进入错误状态
        }
        m_accumulator = intermediate;
        m_entry = formatNumber(m_accumulator);
    } else {
        // 第一个操作数：把当前显示值记入累加器
        m_accumulator = currentValue();
        m_entry = formatNumber(m_accumulator);
    }

    m_pendingOp = op;
    m_freshEntry = true;                             // 下一步输入的数字是第二个操作数
    m_expression = formatNumber(m_accumulator) + QLatin1Char(' ') + operatorText(op);

    refreshDisplay();
}

void MainWindow::inputEquals()
{
    if (m_pendingOp == Operator::None) {
        // 没有待执行的运算（例如刚启动就按 "="），只把当前输入"定型"
        m_entry = formatNumber(currentValue());
        m_freshEntry = true;
        refreshDisplay();
        return;
    }

    // m_freshEntry 为 true 时说明用户按了 "5 + =" 这类输入，
    // 此时把显示值当作第二个操作数，与 Windows 计算器行为一致（5 + 5 = 10）
    const double lhs = m_accumulator;
    const double rhs = currentValue();

    double result = 0.0;
    if (!compute(m_pendingOp, lhs, rhs, &result)) {
        return;                                      // 除数为 0，已进入错误状态
    }

    m_expression = formatNumber(lhs) + QLatin1Char(' ') + operatorText(m_pendingOp)
                   + QLatin1Char(' ') + formatNumber(rhs) + QLatin1String(" =");

    m_accumulator = result;
    m_pendingOp = Operator::None;                    // 运算结束，等待下一次输入
    showResult(result);
}

bool MainWindow::compute(Operator op, double lhs, double rhs, double *result)
{
    switch (op) {
    case Operator::Add:
        *result = lhs + rhs;
        break;
    case Operator::Subtract:
        *result = lhs - rhs;
        break;
    case Operator::Multiply:
        *result = lhs * rhs;
        break;
    case Operator::Divide:
        if (qFuzzyIsNull(rhs)) {                     // 边界处理：除数为 0
            setError(QStringLiteral("除数不能为 0"));
            return false;
        }
        *result = lhs / rhs;
        break;
    case Operator::None:
        return false;
    }
    return true;
}

void MainWindow::showResult(double value)
{
    m_entry = formatNumber(value);
    m_freshEntry = true;      // 结果已经产出，下一个数字键应开始新的计算
    refreshDisplay();
}

void MainWindow::setError(const QString &message)
{
    m_error = true;
    m_pendingOp = Operator::None;   // 丢弃未完成的运算，避免错误结果继续参与计算
    m_entry = message;
    m_freshEntry = true;
    refreshDisplay();
}

double MainWindow::currentValue() const
{
    bool ok = false;
    const double value = m_entry.toDouble(&ok);
    return ok ? value : 0.0;
}

// =====================================================================
//  界面刷新与格式化
// =====================================================================

void MainWindow::refreshDisplay()
{
    // 输入过程中直接显示用户键入的原始字符串，
    // 这样 "0."、"-" 这一类中间状态能被如实呈现
    ui->labelResult->setText(m_entry);
    ui->labelExpression->setText(m_expression);

    // 结果行的颜色由样式表里的属性选择器 [state="error"] 控制。
    // 动态属性变化后 Qt 不会自动重新计算样式，需要手动 unpolish / polish 一次。
    ui->labelResult->setProperty("state",
                                 m_error ? QStringLiteral("error") : QStringLiteral("normal"));
    ui->labelResult->style()->unpolish(ui->labelResult);
    ui->labelResult->style()->polish(ui->labelResult);

    // 错误提示文字较长，临时换成小号字体，避免被窗口裁掉。
    // 注意要放在 polish 之后设置，否则会被样式重新计算时覆盖掉。
    QFont font = m_resultFont;
    if (m_error) {
        font.setPointSize(qMax(14, m_resultFont.pointSize() / 2));
    }
    ui->labelResult->setFont(font);
}

QString MainWindow::formatNumber(double value) const
{
    if (value == 0.0) {
        return QStringLiteral("0");            // 同时把 -0 归一化成 0
    }
    if (qIsNaN(value) || qIsInf(value)) {
        return QStringLiteral("无效结果");
    }
    // 'g' 会自动去掉多余的尾随 0，并用科学计数法表示过大/过小的数，
    // 15 位有效数字可以消除 0.1 + 0.2 = 0.30000000000000004 之类的浮点噪声
    QString text = QString::number(value, 'g', kMaxSignificantDigits);
    return text;
}

QString MainWindow::operatorText(Operator op)
{
    switch (op) {
    case Operator::Add:      return QStringLiteral("+");
    case Operator::Subtract: return QStringLiteral("\u2212");   // 减号 U+2212
    case Operator::Multiply: return QStringLiteral("\u00d7");   // 乘号 U+00D7
    case Operator::Divide:   return QStringLiteral("\u00f7");   // 除号 U+00F7
    case Operator::None:     break;
    }
    return QString();
}

// =====================================================================
//  键盘事件
// =====================================================================

/*!
 * \brief 把 Qt 按键映射成与按钮完全相同的按键标识。
 *
 * 主键盘和数字小键盘共用同一套映射：小键盘的 "+" "-" "*" "/" 以及数字键
 * 在 Qt 中本身就是 Qt::Key_Plus / Qt::Key_0，因此不需要额外分支。
 *
 * \return 找不到对应按键时返回空字符串，按键交回基类处理。
 */
QString MainWindow::keyIdFromEvent(QKeyEvent *event)
{
    const int key = event->key();

    // 数字键 0 ~ 9（含数字小键盘）
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return QString(QChar(static_cast<ushort>(key)));
    }

    switch (key) {
    case Qt::Key_Plus:      return QStringLiteral("+");
    case Qt::Key_Minus:     return QStringLiteral("-");
    case Qt::Key_Asterisk:  return QStringLiteral("*");
    case Qt::Key_Slash:     return QStringLiteral("/");
    case Qt::Key_Period:
    case Qt::Key_Comma:     return QStringLiteral(".");       // "." 和 "," 都当作小数点
    case Qt::Key_Equal:
    case Qt::Key_Return:
    case Qt::Key_Enter:     return QStringLiteral("=");       // "=" 和回车都当作等号
    case Qt::Key_Backspace: return QStringLiteral("back");    // 退格
    case Qt::Key_Escape:    return QStringLiteral("clear");   // Esc 相当于 C
    case Qt::Key_Delete:    return QStringLiteral("ce");      // Delete 相当于 CE
    case Qt::Key_Percent:   return QStringLiteral("%");
    default:                break;
    }

    return QString();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    const QString keyId = keyIdFromEvent(event);
    if (keyId.isEmpty()) {
        QMainWindow::keyPressEvent(event);      // 不认识的按键交回基类
        return;
    }

    // 核心思路：键盘不直接调用 handleKeyInput()，而是先找到这个按键对应的按钮、
    // 触发它的 clicked 信号，让键盘输入和鼠标点击汇入同一条链路：
    //     鼠标点击 / 键盘按键 -> onButtonClicked() -> handleKeyInput()
    // 这样两条输入路径的行为必然一致，也不会出现「改了鼠标忘了改键盘」的维护问题。
    QPushButton *button = m_keyToButton.value(keyId, nullptr);
    if (button != nullptr) {
        button->click();
    } else {
        handleKeyInput(keyId);                  // 兜底：没有对应按钮时直接处理
    }

    event->accept();
}
