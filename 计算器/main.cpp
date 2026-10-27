/**
 * @file    main.cpp
 * @brief   程序入口：创建主窗口并加载全局样式表。
 * @date    2026-10-08
 */

#include "mainwindow.h"
#include "stylesheet.h"

#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 统一界面字体，避免不同 Windows 环境下字体差异导致布局错位
    QFont appFont(QStringLiteral("Microsoft YaHei UI"), 10);
    appFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(appFont);

    applyAppStyleSheet(app);

    MainWindow window;
    window.show();

    return app.exec();
}
