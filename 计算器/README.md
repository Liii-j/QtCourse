# 带键盘事件的计算器（Qt Widgets）

实验一：带键盘事件的计算器。用 Qt 6 + Qt Designer 实现一个界面参照 Windows 计算器的四则运算计算器，  
鼠标与键盘输入复用同一套处理逻辑，并配有 Qt Test 自动化测试。

## 1. 运行环境

| 项目   | 版本                                   |
| ---- | ------------------------------------ |
| Qt   | 6.9.2 及以上（开发环境为 6.11.2 MinGW 64-bit） |
| 编译工具 | MinGW 13.1.0 / qmake                 |
| 系统   | Windows 10 / 11 64 位                 |

## 2. 构建与运行

**方式一：Qt Creator（推荐）**

1. 打开 `calculator.pro`；
2. 选择 `Desktop Qt 6.x MinGW 64-bit` 套件；
3. 按 `Ctrl + R` 运行。

**方式二：命令行**

```bash
qmake calculator.pro
mingw32-make
./release/Calculator.exe
```

## 3. 功能说明

- 四则运算：加、减、乘、除，支持连续运算与负数结果；
- 小数输入：同一操作数内最多一个小数点，直接按 `.` 会自动补前导 0；
- 退格 `⌫`：逐位删除当前操作数，删空后回到 0；
- `CE`：只清除当前操作数，保留第一个操作数与待执行的运算符；
- `C`：清除全部状态，回到初始界面；
- 一元运算：`1/x`、`x²`、`²√x`、`%`、`+/-`（取反）；
- 异常处理：除数为 0 时提示「除数不能为 0」，进入错误状态后只响应清除键；
- 显示区：上方表达式行显示「第一个操作数 运算符 第二个操作数 =」，下方结果行显示当前数值。

## 4. 键盘对照表

| 键盘              | 等价按钮 | 说明          |
| --------------- | ---- | ----------- |
| `0` ~ `9`       | 数字键  | 主键盘与数字小键盘均可 |
| `+` `-` `*` `/` | 加减乘除 | 小键盘的运算键同样有效 |
| `.` / `,`       | `.`  | 小数点         |
| `=` / `Enter`   | `=`  | 求值          |
| `Backspace`     | `⌫`  | 退格          |
| `Delete`        | `CE` | 清除当前输入      |
| `Esc`           | `C`  | 全部清除        |
| `Shift` + `5`   | `%`  | 百分号         |

## 5. 项目结构

```
计算器/
├── calculator.pro          主工程文件
├── main.cpp                程序入口：加载字体与样式表
├── mainwindow.h            主窗口声明：状态机、统一按键入口
├── mainwindow.cpp          主窗口实现：输入、运算、键盘事件
├── mainwindow.ui           Qt Designer 界面文件（网格布局 + 动态属性 key / role）
├── style.qss               界面样式表
├── stylesheet.h            样式表加载工具（主程序与测试共用）
├── res.qrc                 资源文件（把 style.qss 编译进可执行文件）
├── tests/
│   ├── tests.pro           单元测试工程
│   └── tst_calculator.cpp  35 个测试用例 + 截图生成
└── docs/
    ├── 实验报告.md
    └── screenshots/        实验报告用界面截图
```

## 6. 自动化测试

```bash
cd tests
qmake tests.pro
mingw32-make
./release/CalculatorTest.exe
```

- 35 个用例覆盖数字输入、小数点、退格、CE/C、四则运算与各类异常输入；
- 同一组按键序列会分别走鼠标路径与键盘路径执行，断言两者结果完全一致；
- 用例 `captureScreenshots()` 会直接渲染窗口生成 `docs/screenshots/` 下的界面截图，  
  测试与出图一次完成，避免手工截图与代码脱节。

> 从工程根目录运行测试，截图才会输出到 `docs/screenshots/`；  
> 也可以用环境变量 `CALC_SHOT_DIR` 指定其它输出目录。

## 7. 关键设计

### 7.1 按键标识与统一入口

每个按钮在 Qt Designer 里通过自定义动态属性 `key` 携带一个按键标识（`"7"`、`"+"`、`"back"` …），  
24 个按钮共用一个槽函数：

```
鼠标点击 ─┐
          ├─→ onButtonClicked() ─→ handleKeyInput(keyId)
键盘按键 ─┘
```

键盘事件不直接调用处理函数，而是先按键标识找到对应按钮、触发它的 `clicked` 信号，  
因此两条输入路径走的是同一条代码链路，行为必然一致，也不会出现「改了鼠标忘了改键盘」。

### 7.2 计算状态

| 成员              | 作用                            |
| --------------- | ----------------------------- |
| `m_entry`       | 当前正在输入的操作数（保存为字符串，便于处理小数点与退格） |
| `m_accumulator` | 第一个操作数 / 上一次的运算结果             |
| `m_pendingOp`   | 待执行的运算符，`None` 表示没有未完成的运算     |
| `m_freshEntry`  | 为 `true` 时，下一个数字键将开始一个新的操作数   |
| `m_error`       | 错误状态标记（除数为 0）                 |

## 8. 上传到 GitHub

本实验代码放在课程仓库 <https://github.com/Liii-j/QtCourse> 的 `计算器/` 目录下。  
课程仓库里已经有一个实验工程，所以要做的是**把本工程放进子目录，同时保留这 10 次提交历史**  
（直接把文件拷过去再提交一次，提交历史就没了，满足不了"体现迭代过程"的要求）。

### 做法一：补丁重放（推荐）

```bash
cd D:/Code/Qt
git clone https://github.com/Liii-j/QtCourse.git
cd QtCourse
git am --directory=计算器 D:/Code/Qt/qtcourse-patches/*.patch
git push origin main
```

补丁由 `git format-patch --root -o <目录> main` 生成，已备在 `D:/Code/Qt/qtcourse-patches/`  
（`0001` ~ `0010` 共 10 个）。  
**注意**：`*` 通配符要在 **Git Bash** 里执行（CMD 不展开通配符）。

这么做的结果是：10 次提交里的每一次都真实改动了 `计算器/` 下的文件。  
GitHub 上既能在仓库首页 **Commits** 看到完整迭代过程，  
点进 `计算器/` 目录再点 History 也能看到同样的 10 条记录。

### 做法二：git subtree（一条命令）

```bash
cd D:/Code/Qt
git clone https://github.com/Liii-j/QtCourse.git
cd QtCourse
git subtree add --prefix=计算器 "D:/Code/Qt/计算器" main
git push origin main
```

一条命令搬完整个工程，提交历史也都在。差别是它会额外产生一个  
`Add '计算器/' from commit ...` 的合并提交，在 GitHub 上看起来像"一次性导入"；  
而且点进 `计算器/` 目录看 History 只会追到这一条合并提交  
（提交历史本身没丢，仓库首页的 Commits 里依然完整）。

两种做法都已在本地仓库实测通过。

### 推送之后

打开仓库首页点 **Commits**，就是实验报告「五、GitHub 日志」要交的截图。

> `build/`、`moc_*`、`ui_*`、`*.pro.user`、`.workbuddy/` 等已经在 `.gitignore` 里排除，不会污染仓库。
