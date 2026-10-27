#-------------------------------------------------------------------
# 实验一：带键盘事件的计算器
# 构建方式：Qt Creator 打开本文件，或用 qmake + mingw32-make
#-------------------------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG   += c++17

TEMPLATE  = app
TARGET    = Calculator

# 去掉已废弃 API，避免旧写法混入
DEFINES  += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h \
    stylesheet.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    res.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
