/**
 * @file    stylesheet.h
 * @brief   全局样式表加载工具。
 *
 * 单独抽成一个头文件，让主程序（main.cpp）和单元测试共用同一份样式加载逻辑，
 * 从而保证测试时截下来的界面与真实运行界面完全一致。
 */

#ifndef STYLESHEET_H
#define STYLESHEET_H

#include <QApplication>
#include <QFile>
#include <QString>

/// 从编译进可执行文件的资源中读取 QSS 并应用到整个程序。
/// 使用资源（:/style.qss）而不是磁盘路径，程序拷到任何目录都能正常显示样式。
inline void applyAppStyleSheet(QApplication &app)
{
    QFile styleFile(QStringLiteral(":/style.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }
}

#endif // STYLESHEET_H
