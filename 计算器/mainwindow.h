/**
 * @file    mainwindow.h
 * @brief   带键盘事件的计算器 —— 主窗口声明。
 *
 * 设计要点：
 *  1. 所有按钮共用一个槽 onButtonClicked()，通过按钮的 "key" 动态属性识别按键，
 *     避免 24 个 connect / 24 个槽函数造成的大段重复代码。
 *  2. 鼠标与键盘最终都汇聚到同一个入口 handleKeyInput()，保证两条输入路径行为一致。
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFont>
#include <QHash>
#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QKeyEvent;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    /// 所有数字键 / 运算符键 / 功能键 共用的槽函数
    void onButtonClicked();

private:
    /// 四则运算符
    enum class Operator {
        None,       ///< 没有待执行的运算
        Add,        ///< 加
        Subtract,   ///< 减
        Multiply,   ///< 乘
        Divide      ///< 除
    };

    /// 单个操作数允许输入的最大字符数，防止无限输入
    static constexpr int kMaxEntryLength = 16;
    /// 结果保留的有效数字位数
    static constexpr int kMaxSignificantDigits = 15;

    // ---------- 初始化 ----------
    /// 统一设置按钮属性、建立按键标识到按钮的映射、连接信号槽
    void setupButtons();

    // ---------- 键盘 ----------
    /// 把 Qt 按键映射成与按钮一致的按键标识，找不到对应按键时返回空字符串
    static QString keyIdFromEvent(QKeyEvent *event);

    // ---------- 统一的按键处理入口 ----------
    /// @param keyId 按键标识：数字 "0"~"9"、运算符 "+" "-" "*" "/" "="、
    ///              以及功能键 "." "back" "ce" "clear" "neg" 等
    void handleKeyInput(const QString &keyId);

    void inputDigit(const QString &digit);   ///< 输入数字
    void inputDot();                         ///< 输入小数点
    void inputOperator(Operator op);         ///< 输入运算符（含连续运算）
    void inputEquals();                      ///< 输入等号
    void backspace();                        ///< 退格
    void clearEntry();                       ///< CE：清除当前输入
    void clearAll();                         ///< C：清除全部状态

    // ---------- 运算 ----------
    /// 取出当前显示值对应的数值
    double currentValue() const;
    /// 执行二元运算，除数为 0 时返回 false 并进入错误状态
    bool compute(Operator op, double lhs, double rhs, double *result);
    /// 把运算结果写回显示与状态
    void showResult(double value);
    /// 进入错误状态（如除以 0）
    void setError(const QString &message);

    // ---------- 界面刷新与格式化 ----------
    /// 把内部状态同步到界面上（结果行 + 表达式行）
    void refreshDisplay();
    /// 把 double 格式化成适合显示的字符串
    QString formatNumber(double value) const;
    /// 运算符对应的显示符号
    static QString operatorText(Operator op);

    // ---------- 成员变量 ----------
    Ui::MainWindow *ui;

    /// 按键标识 -> 按钮，键盘事件据此模拟按钮点击
    QHash<QString, QPushButton *> m_keyToButton;

    // ---- 计算器状态 ----
    QString  m_entry;        ///< 当前正在输入的操作数（用字符串保存，便于小数点和退格处理）
    double   m_accumulator;  ///< 第一个操作数 / 上一次的运算结果
    Operator m_pendingOp;    ///< 待执行的运算符
    bool     m_freshEntry;   ///< true 表示下一次输入数字应开始一个新的操作数
    bool     m_error;        ///< true 表示处于错误状态（如除数为 0）

    QString  m_expression;   ///< 表达式行文本，便于观察"第一个操作数 运算符 第二个操作数"

    QFont    m_resultFont;   ///< 结果行正常字号，显示错误提示时临时换成小号字体
};

#endif // MAINWINDOW_H
