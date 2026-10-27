#-------------------------------------------------------------------
# 计算器单元测试工程（Qt Test）
# 使用方法：Qt Creator 打开本文件运行；或命令行
#   qmake tests.pro && mingw32-make && ./CalculatorTest.exe
# 生成实验报告截图时，请在工程根目录下运行，截图输出到 ../docs/screenshots
#-------------------------------------------------------------------

QT       += core gui widgets testlib

CONFIG   += c++17 console

TEMPLATE  = app
TARGET    = CalculatorTest

INCLUDEPATH += ..

SOURCES += \
    tst_calculator.cpp \
    ../mainwindow.cpp

HEADERS += \
    ../mainwindow.h \
    ../stylesheet.h

FORMS += \
    ../mainwindow.ui

RESOURCES += \
    ../res.qrc
