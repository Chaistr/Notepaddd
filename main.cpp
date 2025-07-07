#include "mainwindow.h"
#include <QCoreApplication>
#include <QSslSocket>

#include <QApplication>
#include <QProcessEnvironment>
int main(int argc, char *argv[])
{
    qputenv("QT_SSL_BACKEND", "schannel");

    // 输出当前设置，用于调试
    qDebug() << "当前QT_SSL_BACKEND:" << qgetenv("QT_SSL_BACKEND");
    qDebug() << "当前SSL后端:" << QSslSocket::sslLibraryVersionString();
    QApplication a(argc, argv);
    MainWindow w;
    //w.setStyleSheet("background-color: white;");
    a.setStyle("Fusion");
    a.setStyleSheet(R"(
        /* 菜单栏背景和文本颜色 */
        QMenuBar {
            background-color: #f0f0f0;
            color: black;
        }

        /* 菜单栏项目悬停效果 */
        QMenuBar::item:selected {
            background-color: #d0d0d0;
        }

        /* 菜单背景和文本颜色 */
        QMenu {
            background-color: white;
            color: black;
            border: 1px solid #c0c0c0;
        }
QPalette {
        colorGroup: light; /* 强制使用浅色配色方案 */
    }

        /* 菜单项悬停效果 */
        QMenu::item:selected {
            background-color: #e0e0e0;
        }
        QToolBar {
            background-color: #f0f0f0;    /* 背景色 */
            color: black;                /* 文本颜色 */
            border: 1px solid #d0d0d0;   /* 边框 */
            padding: 4px;                /* 内边距 */
        }

        QToolBar::separator {
            background-color: #c0c0c0;   /* 分隔符颜色 */
            width: 1px;                  /* 分隔符宽度 */
            margin: 2px 4px;             /* 分隔符边距 */
        }
/* TabWidget主体背景 */
        QTabWidget {
            background-color: white;
        }

        /* 标签页内容区域 */
        QTabWidget::pane {
            border: 1px solid #d0d0d0;
            background-color: white;
            top: -1px; /* 与标签页对齐 */
        }

        /* 标签栏 */
        QTabBar {
            background-color: #f0f0f0;
        }

        /* 标签 */
        QTabBar::tab {
            background-color: #e0e0e0;
            color: black;
            border: 1px solid #d0d0d0;
            border-bottom: none;
            padding: 6px 12px;
            margin-right: -1px; /* 标签之间无缝连接 */
        }

        /* 选中的标签 */
        QTabBar::tab:selected {
            background-color: white;
            border-bottom-color: white;
            z-index: 10; /* 确保选中的标签在最上面 */
        }

        /* 悬停的标签 */
        QTabBar::tab:hover:!selected {
            background-color: #f0f0f0;
        }
* {
            background-color: white;
            color: black;
        }
    )");

    // 全局样式表设置（确保菜单文本可见）

    w.show();
    qputenv("QT_DEBUG_PLUGINS", "1");  // 显示插件加载信息
    qputenv("QT_LOGGING_RULES", "qt.network.ssl=true");  // 启用 SSL 调试日志
    qDebug() << "OpenSSL supported:" << QSslSocket::supportsSsl();
    qDebug() << "SSL library version:" << QSslSocket::sslLibraryVersionString();
    qDebug() << "SSL library build version:" << QSslSocket::sslLibraryBuildVersionString();

    return a.exec();
}
