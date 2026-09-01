#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("简易财务计算器"));
    application.setOrganizationName(QStringLiteral("BIT Mini Semester"));
    application.setWindowIcon(QIcon(QStringLiteral(":/icons/calculator.svg")));

    QFile styleFile(QStringLiteral(":/styles/app.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        application.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    MainWindow window;
    window.show();

    // SSH 自动化只验证程序能启动；正常运行时不会自动退出。
    if (application.arguments().contains(QStringLiteral("--smoke-test")))
    {
        QTimer::singleShot(800, &application, &QCoreApplication::quit);
    }

    return application.exec();
}
