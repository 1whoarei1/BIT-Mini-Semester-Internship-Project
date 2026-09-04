#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QPixmap>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("Socket 文件传输工具"));
    application.setOrganizationName(QStringLiteral("BIT Mini Semester"));
    application.setWindowIcon(QIcon(QStringLiteral(":/icons/transfer.svg")));

    QFile styleFile(QStringLiteral(":/styles/app.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        application.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    MainWindow window;
    window.show();

    const QStringList arguments = application.arguments();
    if (arguments.contains(QStringLiteral("--smoke-test")))
    {
        QTimer::singleShot(800, &application, &QCoreApplication::quit);
    }

    const int screenshotIndex = arguments.indexOf(QStringLiteral("--screenshot"));
    if (screenshotIndex >= 0 && screenshotIndex + 1 < arguments.size())
    {
        const QString screenshotPath = arguments.at(screenshotIndex + 1);
        QTimer::singleShot(600, &window, [&window, screenshotPath, &application]()
                           {
                               window.grab().save(screenshotPath);
                               application.quit();
                           });
    }

    return application.exec();
}
