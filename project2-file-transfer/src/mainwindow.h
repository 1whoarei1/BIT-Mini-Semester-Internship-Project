#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "transfermanager.h"

#include <QMainWindow>

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void openSettings();
    void applySettings(bool serverMode, const QString &host, quint16 port,
                       const QString &savePath);
    void startConnection();
    void selectFile();
    void sendSelectedFile();
    void handleDroppedFile(const QString &filePath);
    void updateState(bool active, bool connected, const QString &description);
    void updateSendProgress(qint64 current, qint64 total, const QString &fileName);
    void updateReceiveProgress(qint64 current, qint64 total, const QString &fileName);
    void showTransferError(const QString &message);
    void appendLog(const QString &message);

private:
    static int progressPercent(qint64 current, qint64 total);
    void updateSettingsSummary();
    void setSelectedFile(const QString &filePath);

    Ui::MainWindow *ui;
    TransferManager transferManager_;
    bool serverMode_ = false;
    QString host_ = QStringLiteral("127.0.0.1");
    quint16 port_ = 45454;
    QString savePath_;
    QString selectedFilePath_;
};

#endif
