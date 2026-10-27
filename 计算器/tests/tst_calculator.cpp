/**
 * @file    tst_calculator.cpp
 * @brief   计算器的自动化测试（Qt Test）。
 *
 * 覆盖范围：
 *   1. 基础输入：数字、前导零、小数点、退格、CE / C
 *   2. 四则运算与双操作数状态管理
 *   3. 边界情况：连续小数点、连续运算符、除数为 0、结果后继续输入、连续运算
 *   4. 鼠标输入与键盘输入结果一致性
 *   5. 生成实验报告用的界面截图
 *
 * 说明：所有断言都通过界面上的 labelResult / labelExpression 文本完成，
 *       不依赖类的私有成员，属于黑盒测试。
 */

#include <QtTest>

#include <QApplication>
#include <QDir>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>

#include "mainwindow.h"
#include "stylesheet.h"

// =====================================================================
//  测试辅助函数
// =====================================================================

namespace {

/// 创建并显示一个计算器窗口
MainWindow *createWindow()
{
    auto *window = new MainWindow;
    // 使用真实的 Windows 平台插件（这样能拿到系统字体，截图与真机一致），
    // 但设置 WA_DontShowOnScreen 让窗口不上屏：既能避免弹窗打扰，也能绕开
    // 系统窗口淡入动画导致的"半透明"截屏问题。
    window->setAttribute(Qt::WA_DontShowOnScreen, true);
    window->resize(400, 580);
    window->show();
    QTest::qWait(50);
    return window;
}

QLabel *labelOf(MainWindow *window, const char *objectName)
{
    return window->findChild<QLabel *>(QString::fromLatin1(objectName));
}

/// 读取结果行文本（大号数字）
QString displayText(MainWindow *window)
{
    return labelOf(window, "labelResult")->text();
}

/// 读取表达式行文本（小号灰色）
QString expressionText(MainWindow *window)
{
    return labelOf(window, "labelExpression")->text();
}

/// 按按键标识找到对应的按钮
QPushButton *buttonOf(MainWindow *window, const QString &keyId)
{
    const QList<QPushButton *> buttons = window->findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        if (button->property("key").toString() == keyId) {
            return button;
        }
    }
    return nullptr;
}

/// 走"鼠标"路径：模拟点击按钮
void clickKey(MainWindow *window, const QString &keyId)
{
    QPushButton *button = buttonOf(window, keyId);
    QVERIFY2(button != nullptr, qPrintable(QStringLiteral("找不到 key = ") + keyId + QStringLiteral(" 的按钮")));
    button->click();
}

/// 连续点击一串按键，例如 "12+34="
void clickKeys(MainWindow *window, const QString &keys)
{
    for (const QChar &ch : keys) {
        clickKey(window, QString(ch));
    }
}

/// 走"键盘"路径：模拟按键
void pressKey(MainWindow *window, Qt::Key key)
{
    QTest::keyClick(window, key);
}

/// 连续按下键盘上的数字/运算符
void pressKeys(MainWindow *window, const QString &keys)
{
    for (const QChar &ch : keys) {
        if (ch >= QLatin1Char('0') && ch <= QLatin1Char('9')) {
            pressKey(window, static_cast<Qt::Key>(Qt::Key_0 + ch.unicode() - '0'));
        } else if (ch == QLatin1Char('+')) {
            pressKey(window, Qt::Key_Plus);
        } else if (ch == QLatin1Char('-')) {
            pressKey(window, Qt::Key_Minus);
        } else if (ch == QLatin1Char('*')) {
            pressKey(window, Qt::Key_Asterisk);
        } else if (ch == QLatin1Char('/')) {
            pressKey(window, Qt::Key_Slash);
        } else if (ch == QLatin1Char('.')) {
            pressKey(window, Qt::Key_Period);
        } else if (ch == QLatin1Char('=')) {
            pressKey(window, Qt::Key_Equal);
        }
    }
}

/// 截图输出目录：优先读取环境变量，默认 docs/screenshots
QString screenshotDir()
{
    const QString dir = qEnvironmentVariable("CALC_SHOT_DIR", QStringLiteral("docs/screenshots"));
    QDir().mkpath(dir);
    return dir;
}

/// 把当前窗口内容保存为 PNG
void saveScreenshot(MainWindow *window, const QString &fileName)
{
    const QString path = screenshotDir() + QLatin1Char('/') + fileName;
    QPixmap pixmap = window->grab();
    pixmap.save(path);
}

} // namespace

// =====================================================================
//  测试用例
// =====================================================================

class CalculatorTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    // ---- 1. 基础输入 ----
    void digitInput();
    void leadingZeroIgnored();
    void decimalPointInput();
    void repeatedDecimalPointIgnored();
    void backspaceRemovesLastChar();
    void clearEntryKeepsFirstOperand();
    void clearAllResetsEverything();

    // ---- 2. 四则运算 ----
    void basicArithmetic_data();
    void basicArithmetic();
    void operandStateAfterOperator();

    // ---- 3. 边界情况 ----
    void divideByZero();
    void errorStateOnlyClearableByClear();
    void repeatedOperatorReplacesIt();
    void continuousCalculation();
    void continueAfterEquals();
    void freshOperandAfterEquals();

    // ---- 4. 键盘与鼠标一致 ----
    void keyboardMatchesMouse_data();
    void keyboardMatchesMouse();
    void mixedMouseAndKeyboard();
    void keyboardSpecialKeys();

    // ---- 5. 报告截图 ----
    void captureScreenshots();

private:
    MainWindow *m_window = nullptr;
};

void CalculatorTest::initTestCase()
{
    // 与主程序使用完全相同的字体和样式表，保证测试截图就是真实运行效果
    QFont appFont(QStringLiteral("Microsoft YaHei UI"), 10);
    appFont.setStyleStrategy(QFont::PreferAntialias);
    qApp->setFont(appFont);
    applyAppStyleSheet(*qApp);
}

void CalculatorTest::init()
{
    m_window = createWindow();
}

void CalculatorTest::cleanup()
{
    delete m_window;
    m_window = nullptr;
}

// ---------------------------------------------------------------------
//  1. 基础输入
// ---------------------------------------------------------------------

void CalculatorTest::digitInput()
{
    clickKeys(m_window, QStringLiteral("123"));
    QCOMPARE(displayText(m_window), QStringLiteral("123"));
}

void CalculatorTest::leadingZeroIgnored()
{
    clickKeys(m_window, QStringLiteral("0012"));
    QCOMPARE(displayText(m_window), QStringLiteral("12"));
}

void CalculatorTest::decimalPointInput()
{
    clickKeys(m_window, QStringLiteral("3.14"));
    QCOMPARE(displayText(m_window), QStringLiteral("3.14"));

    clickKey(m_window, QStringLiteral("clear"));
    // 直接按小数点，应自动补前导 0
    clickKey(m_window, QStringLiteral("."));
    QCOMPARE(displayText(m_window), QStringLiteral("0."));
    clickKey(m_window, QStringLiteral("5"));
    QCOMPARE(displayText(m_window), QStringLiteral("0.5"));
}

void CalculatorTest::repeatedDecimalPointIgnored()
{
    // 连续输入小数点，只有第一个生效
    clickKeys(m_window, QStringLiteral("1.2.3.4"));
    QCOMPARE(displayText(m_window), QStringLiteral("1.234"));

    clickKey(m_window, QStringLiteral("clear"));
    clickKeys(m_window, QStringLiteral("1..2"));
    QCOMPARE(displayText(m_window), QStringLiteral("1.2"));

    // 第二个操作数里同样只能有一个小数点
    clickKeys(m_window, QStringLiteral("+3.5.5"));
    QCOMPARE(displayText(m_window), QStringLiteral("3.55"));

    clickKey(m_window, QStringLiteral("="));
    QCOMPARE(displayText(m_window), QStringLiteral("4.75"));
}

void CalculatorTest::backspaceRemovesLastChar()
{
    clickKeys(m_window, QStringLiteral("1234"));
    clickKey(m_window, QStringLiteral("back"));
    QCOMPARE(displayText(m_window), QStringLiteral("123"));
    clickKey(m_window, QStringLiteral("back"));
    clickKey(m_window, QStringLiteral("back"));
    QCOMPARE(displayText(m_window), QStringLiteral("1"));
    // 退格到只剩一位再退格，应回到初始的 0
    clickKey(m_window, QStringLiteral("back"));
    QCOMPARE(displayText(m_window), QStringLiteral("0"));
    // 已经是 0 再退格不应出现空字符串
    clickKey(m_window, QStringLiteral("back"));
    QCOMPARE(displayText(m_window), QStringLiteral("0"));

    // 小数也能正常退格
    clickKeys(m_window, QStringLiteral("3.14"));
    clickKey(m_window, QStringLiteral("back"));
    QCOMPARE(displayText(m_window), QStringLiteral("3.1"));
}

void CalculatorTest::clearEntryKeepsFirstOperand()
{
    clickKeys(m_window, QStringLiteral("12+34"));
    QCOMPARE(displayText(m_window), QStringLiteral("34"));
    clickKey(m_window, QStringLiteral("ce"));
    QCOMPARE(displayText(m_window), QStringLiteral("0"));
    // CE 不影响已经输入的 12 和运算符
    QCOMPARE(expressionText(m_window), QStringLiteral("12 +"));

    clickKeys(m_window, QStringLiteral("34="));
    QCOMPARE(displayText(m_window), QStringLiteral("46"));
}

void CalculatorTest::clearAllResetsEverything()
{
    clickKeys(m_window, QStringLiteral("12+34"));
    clickKey(m_window, QStringLiteral("clear"));
    QCOMPARE(displayText(m_window), QStringLiteral("0"));
    QCOMPARE(expressionText(m_window), QString());

    // 清除后重新计算，不应残留上一次的状态
    clickKeys(m_window, QStringLiteral("7*6="));
    QCOMPARE(displayText(m_window), QStringLiteral("42"));
}

// ---------------------------------------------------------------------
//  2. 四则运算
// ---------------------------------------------------------------------

void CalculatorTest::basicArithmetic_data()
{
    QTest::addColumn<QString>("keys");
    QTest::addColumn<QString>("expected");

    QTest::newRow("整数加法")       << QStringLiteral("12+34=")   << QStringLiteral("46");
    QTest::newRow("整数减法")       << QStringLiteral("50-8=")    << QStringLiteral("42");
    QTest::newRow("结果为负数")     << QStringLiteral("9-15=")    << QStringLiteral("-6");
    QTest::newRow("整数乘法")       << QStringLiteral("7*6=")     << QStringLiteral("42");
    QTest::newRow("整除")           << QStringLiteral("8/2=")     << QStringLiteral("4");
    QTest::newRow("小数除法")       << QStringLiteral("10/4=")    << QStringLiteral("2.5");
    QTest::newRow("小数加法")       << QStringLiteral("2.5+2.5=") << QStringLiteral("5");
    QTest::newRow("浮点噪声消除")   << QStringLiteral("0.1+0.2=") << QStringLiteral("0.3");
    QTest::newRow("被除数为 0")     << QStringLiteral("0/7=")     << QStringLiteral("0");
    QTest::newRow("三位数乘法")     << QStringLiteral("125*8=")   << QStringLiteral("1000");
}

void CalculatorTest::basicArithmetic()
{
    QFETCH(QString, keys);
    QFETCH(QString, expected);
    clickKeys(m_window, keys);
    QCOMPARE(displayText(m_window), expected);
}

void CalculatorTest::operandStateAfterOperator()
{
    clickKeys(m_window, QStringLiteral("12+"));
    // 按下运算符后，显示保留第一个操作数，表达式行显示"第一个操作数 运算符"
    QCOMPARE(displayText(m_window), QStringLiteral("12"));
    QCOMPARE(expressionText(m_window), QStringLiteral("12 +"));

    clickKeys(m_window, QStringLiteral("34"));
    QCOMPARE(displayText(m_window), QStringLiteral("34"));
    // 第一个操作数不应被第二个操作数影响
    QCOMPARE(expressionText(m_window), QStringLiteral("12 +"));

    clickKey(m_window, QStringLiteral("="));
    QCOMPARE(displayText(m_window), QStringLiteral("46"));
    QCOMPARE(expressionText(m_window), QStringLiteral("12 + 34 ="));
}

// ---------------------------------------------------------------------
//  3. 边界情况
// ---------------------------------------------------------------------

void CalculatorTest::divideByZero()
{
    clickKeys(m_window, QStringLiteral("5/0="));
    QCOMPARE(displayText(m_window), QStringLiteral("除数不能为 0"));

    // 被除数为 0、除数为 0 也要拦下来
    clickKey(m_window, QStringLiteral("clear"));
    clickKeys(m_window, QStringLiteral("0/0="));
    QCOMPARE(displayText(m_window), QStringLiteral("除数不能为 0"));
}

void CalculatorTest::errorStateOnlyClearableByClear()
{
    clickKeys(m_window, QStringLiteral("5/0="));
    QCOMPARE(displayText(m_window), QStringLiteral("除数不能为 0"));

    // 错误状态下继续按数字键无效
    clickKeys(m_window, QStringLiteral("123"));
    QCOMPARE(displayText(m_window), QStringLiteral("除数不能为 0"));

    // 清除后恢复正常
    clickKey(m_window, QStringLiteral("clear"));
    QCOMPARE(displayText(m_window), QStringLiteral("0"));
    clickKeys(m_window, QStringLiteral("8/2="));
    QCOMPARE(displayText(m_window), QStringLiteral("4"));
}

void CalculatorTest::repeatedOperatorReplacesIt()
{
    // 连续按运算符，只有最后一个生效：5 + * 3 = 15
    clickKeys(m_window, QStringLiteral("5+*3="));
    QCOMPARE(displayText(m_window), QStringLiteral("15"));

    clickKey(m_window, QStringLiteral("clear"));
    clickKeys(m_window, QStringLiteral("20/-"));
    QCOMPARE(expressionText(m_window), QStringLiteral("20 \u2212"));
}

void CalculatorTest::continuousCalculation()
{
    // 即时计算：2 + 3 先算出 5，再 5 * 4 = 20
    clickKeys(m_window, QStringLiteral("2+3*4="));
    QCOMPARE(displayText(m_window), QStringLiteral("20"));

    clickKey(m_window, QStringLiteral("clear"));
    clickKeys(m_window, QStringLiteral("100-20-30="));
    QCOMPARE(displayText(m_window), QStringLiteral("50"));
}

void CalculatorTest::continueAfterEquals()
{
    clickKeys(m_window, QStringLiteral("2+3="));
    QCOMPARE(displayText(m_window), QStringLiteral("5"));

    // 直接把上一次的结果当作第一个操作数继续运算
    clickKeys(m_window, QStringLiteral("+8="));
    QCOMPARE(displayText(m_window), QStringLiteral("13"));
}

void CalculatorTest::freshOperandAfterEquals()
{
    clickKeys(m_window, QStringLiteral("2+3="));
    QCOMPARE(displayText(m_window), QStringLiteral("5"));

    // 计算完成后输入数字，应开始一次全新计算，而不是拼接在结果后面
    clickKey(m_window, QStringLiteral("7"));
    QCOMPARE(displayText(m_window), QStringLiteral("7"));
    QCOMPARE(expressionText(m_window), QString());

    clickKeys(m_window, QStringLiteral("*3="));
    QCOMPARE(displayText(m_window), QStringLiteral("21"));

    // 结果状态下按退格无效，结果应保持不变
    clickKey(m_window, QStringLiteral("back"));
    QCOMPARE(displayText(m_window), QStringLiteral("21"));
}

// ---------------------------------------------------------------------
//  4. 键盘与鼠标一致
// ---------------------------------------------------------------------

void CalculatorTest::keyboardMatchesMouse_data()
{
    QTest::addColumn<QString>("keys");
    QTest::addColumn<QString>("expected");

    QTest::newRow("键盘加法")     << QStringLiteral("12+34=")   << QStringLiteral("46");
    QTest::newRow("键盘减法")     << QStringLiteral("50-8=")    << QStringLiteral("42");
    QTest::newRow("键盘乘法")     << QStringLiteral("7*6=")     << QStringLiteral("42");
    QTest::newRow("键盘除法")     << QStringLiteral("10/4=")    << QStringLiteral("2.5");
    QTest::newRow("键盘小数")     << QStringLiteral("1.5+2.25=") << QStringLiteral("3.75");
    QTest::newRow("键盘连除零")   << QStringLiteral("9/0=")     << QStringLiteral("除数不能为 0");
}

void CalculatorTest::keyboardMatchesMouse()
{
    QFETCH(QString, keys);
    QFETCH(QString, expected);

    // 相同按键序列，键盘输入与鼠标点击必须得到完全一致的结果
    pressKeys(m_window, keys);
    const QString byKeyboard = displayText(m_window);

    clickKey(m_window, QStringLiteral("clear"));
    clickKeys(m_window, keys);
    const QString byMouse = displayText(m_window);

    QCOMPARE(byKeyboard, expected);
    QCOMPARE(byKeyboard, byMouse);
}

void CalculatorTest::mixedMouseAndKeyboard()
{
    // 鼠标点数字、键盘敲运算符、再鼠标按等号，混合输入不能出错
    clickKeys(m_window, QStringLiteral("12"));
    pressKey(m_window, Qt::Key_Plus);
    pressKeys(m_window, QStringLiteral("34"));
    clickKey(m_window, QStringLiteral("="));
    QCOMPARE(displayText(m_window), QStringLiteral("46"));

    clickKey(m_window, QStringLiteral("clear"));
    pressKeys(m_window, QStringLiteral("25"));
    clickKey(m_window, QStringLiteral("*"));
    clickKeys(m_window, QStringLiteral("4"));
    pressKey(m_window, Qt::Key_Equal);
    QCOMPARE(displayText(m_window), QStringLiteral("100"));
}

void CalculatorTest::keyboardSpecialKeys()
{
    // Backspace 退格
    pressKeys(m_window, QStringLiteral("1234"));
    pressKey(m_window, Qt::Key_Backspace);
    QCOMPARE(displayText(m_window), QStringLiteral("123"));

    // Esc 相当于 C
    pressKey(m_window, Qt::Key_Escape);
    QCOMPARE(displayText(m_window), QStringLiteral("0"));

    // Delete 相当于 CE
    pressKeys(m_window, QStringLiteral("12+34"));
    pressKey(m_window, Qt::Key_Delete);
    QCOMPARE(displayText(m_window), QStringLiteral("0"));
    QCOMPARE(expressionText(m_window), QStringLiteral("12 +"));

    // 回车相当于 =
    pressKeys(m_window, QStringLiteral("34"));
    pressKey(m_window, Qt::Key_Return);
    QCOMPARE(displayText(m_window), QStringLiteral("46"));
}

// ---------------------------------------------------------------------
//  5. 生成实验报告截图
// ---------------------------------------------------------------------

void CalculatorTest::captureScreenshots()
{
    QDir().mkpath(screenshotDir());

    // 对比用：先临时去掉样式表，截一张 Qt 默认外观的界面（实验报告第 5 节的"优化前"）
    qApp->setStyleSheet(QString());
    saveScreenshot(m_window, QStringLiteral("00-before-style.png"));
    applyAppStyleSheet(*qApp);

    // 美化后的初始界面
    saveScreenshot(m_window, QStringLiteral("01-main-window.png"));

    // 小数运算输入中
    clickKeys(m_window, QStringLiteral("12.5+3.75"));
    saveScreenshot(m_window, QStringLiteral("02-decimal-input.png"));

    // 小数运算结果
    clickKey(m_window, QStringLiteral("="));
    saveScreenshot(m_window, QStringLiteral("03-decimal-result.png"));

    // 连续小数点被忽略：1.2.3.4 只保留 1.234
    clickKey(m_window, QStringLiteral("clear"));
    clickKeys(m_window, QStringLiteral("1.2.3.4"));
    saveScreenshot(m_window, QStringLiteral("04-repeated-dot.png"));

    // 退格
    clickKey(m_window, QStringLiteral("clear"));
    clickKeys(m_window, QStringLiteral("1234"));
    clickKey(m_window, QStringLiteral("back"));
    saveScreenshot(m_window, QStringLiteral("05-backspace.png"));

    // 除数为 0
    clickKey(m_window, QStringLiteral("clear"));
    clickKeys(m_window, QStringLiteral("5/0="));
    saveScreenshot(m_window, QStringLiteral("06-divide-by-zero.png"));

    // 连续运算：2 + 3 * 4 = 20
    clickKey(m_window, QStringLiteral("clear"));
    clickKeys(m_window, QStringLiteral("2+3*4="));
    saveScreenshot(m_window, QStringLiteral("07-continuous.png"));

    // 纯键盘输入：256 / 8 = 32
    clickKey(m_window, QStringLiteral("clear"));
    pressKeys(m_window, QStringLiteral("256/8="));
    saveScreenshot(m_window, QStringLiteral("08-keyboard.png"));

    // 清除后重新计算
    clickKey(m_window, QStringLiteral("clear"));
    pressKeys(m_window, QStringLiteral("9"));
    clickKey(m_window, QStringLiteral("*"));
    pressKeys(m_window, QStringLiteral("9"));
    clickKey(m_window, QStringLiteral("="));
    saveScreenshot(m_window, QStringLiteral("09-after-clear.png"));

    QVERIFY(QFile::exists(screenshotDir() + QStringLiteral("/01-main-window.png")));
}

QTEST_MAIN(CalculatorTest)
#include "tst_calculator.moc"
