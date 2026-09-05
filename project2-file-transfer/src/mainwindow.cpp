#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "settingsdialog.h"

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), transferManager_(this)
{
    ui->setupUi(this);

    QString downloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (downloadPath.isEmpty())
    {
        downloadPath = QDir::homePath();
    }
    savePath_ = QDir(downloadPath).filePath(QStringLiteral("FileTransferReceived"));
    QDir().mkpath(savePath_);

    // 第一组连接处理用户操作：主窗口只收集参数和更新控件，不直接进行 Socket 读写。
    connect(ui->settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);
    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::startConnection);
    connect(ui->disconnectButton, &QPushButton::clicked, &transferManager_,
            &TransferManager::stop);
    connect(ui->selectFileButton, &QPushButton::clicked, this, &MainWindow::selectFile);
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::sendSelectedFile);
    connect(ui->dropZone, &DropZoneWidget::fileDropped, this, &MainWindow::handleDroppedFile);

    // 第二组连接处理网络层反馈，通过信号把状态、日志和进度安全地同步到界面。
    connect(&transferManager_, &TransferManager::stateChanged, this, &MainWindow::updateState);
    connect(&transferManager_, &TransferManager::logMessage, this, &MainWindow::appendLog);
    connect(&transferManager_, &TransferManager::sendProgress, this,
            &MainWindow::updateSendProgress);
    connect(&transferManager_, &TransferManager::receiveProgress, this,
            &MainWindow::updateReceiveProgress);
    connect(&transferManager_, &TransferManager::transferError, this,
            &MainWindow::showTransferError);
    connect(&transferManager_, &TransferManager::fileSent, this,
            [this](const QString &path)
            {
                statusBar()->showMessage(QStringLiteral("发送完成：%1").arg(path), 5000);
            });
    connect(&transferManager_, &TransferManager::fileReceived, this,
            [this](const QString &path)
            {
                statusBar()->showMessage(QStringLiteral("接收完成：%1").arg(path), 5000);
            });

    updateSettingsSummary();
    updateState(false, false, QStringLiteral("未连接"));
    appendLog(QStringLiteral("程序已启动，请先检查连接设置。"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(this);
    dialog.setSettings(serverMode_, host_, port_, savePath_);
    // 设置窗口通过自定义信号把参数交给主窗口，不直接操作网络对象。
    connect(&dialog, &SettingsDialog::settingsApplied, this, &MainWindow::applySettings);
    dialog.exec();
}

void MainWindow::applySettings(bool serverMode, const QString &host, quint16 port,
                               const QString &savePath)
{
    transferManager_.stop();
    serverMode_ = serverMode;
    host_ = host;
    port_ = port;
    savePath_ = savePath;
    updateSettingsSummary();
    appendLog(QStringLiteral("连接参数已更新。"));
}

void MainWindow::startConnection()
{
    transferManager_.configure(serverMode_ ? TransferManager::Mode::Server
                                           : TransferManager::Mode::Client,
                               host_, port_, savePath_);
    transferManager_.start();
}

void MainWindow::selectFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择要发送的文件"), QString(),
        QStringLiteral("支持的文件 (*.txt *.md *.log *.png *.jpg *.jpeg *.bmp);;所有文件 (*)"));
    if (!filePath.isEmpty())
    {
        setSelectedFile(filePath);
    }
}

void MainWindow::sendSelectedFile()
{
    if (selectedFilePath_.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("尚未选择文件"),
                                 QStringLiteral("请先选择文件，或把文件拖入发送区。"));
        return;
    }
    transferManager_.sendFile(selectedFilePath_);
}

void MainWindow::handleDroppedFile(const QString &filePath)
{
    // 拖拽与“选择文件”共用同一发送入口，避免两套路径产生不同的校验行为。
    setSelectedFile(filePath);
    appendLog(QStringLiteral("已拖入文件，准备自动发送：%1").arg(filePath));
    transferManager_.sendFile(filePath);
}

void MainWindow::updateState(bool active, bool connected, const QString &description)
{
    ui->connectionStatusLabel->setText(description);
    ui->connectionStatusLabel->setProperty("connected", connected);
    ui->connectionStatusLabel->style()->unpolish(ui->connectionStatusLabel);
    ui->connectionStatusLabel->style()->polish(ui->connectionStatusLabel);

    // 根据网络状态统一控制按钮，防止连接过程中重复启动或未连接时误发送。
    ui->connectButton->setEnabled(!active);
    ui->disconnectButton->setEnabled(active);
    ui->settingsButton->setEnabled(!active);
    ui->selectFileButton->setEnabled(connected);
    ui->sendButton->setEnabled(connected && !selectedFilePath_.isEmpty());
    ui->dropZone->setEnabled(connected);
    ui->dropZone->setText(connected
                              ? QStringLiteral("把文本或图片文件拖到这里\n释放后自动发送")
                              : QStringLiteral("连接成功后可拖拽文件发送"));
    statusBar()->showMessage(description, 4000);
}

void MainWindow::updateSendProgress(qint64 current, qint64 total, const QString &fileName)
{
    ui->sendProgressBar->setValue(progressPercent(current, total));
    ui->sendProgressLabel->setText(QStringLiteral("发送：%1  %2 / %3 字节")
                                       .arg(fileName)
                                       .arg(current)
                                       .arg(total));
}

void MainWindow::updateReceiveProgress(qint64 current, qint64 total,
                                       const QString &fileName)
{
    ui->receiveProgressBar->setValue(progressPercent(current, total));
    ui->receiveProgressLabel->setText(QStringLiteral("接收：%1  %2 / %3 字节")
                                          .arg(fileName)
                                          .arg(current)
                                          .arg(total));
}

void MainWindow::showTransferError(const QString &message)
{
    statusBar()->showMessage(message, 6000);
    QMessageBox::warning(this, QStringLiteral("传输错误"), message);
}

void MainWindow::appendLog(const QString &message)
{
    ui->logEdit->append(QStringLiteral("[%1] %2")
                            .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")),
                                 message.toHtmlEscaped()));
}

int MainWindow::progressPercent(qint64 current, qint64 total)
{
    // 空文件没有可传输字节，协议头处理完成后直接视为 100%。
    if (total == 0)
    {
        return 100;
    }
    return static_cast<int>((current * 100) / total);
}

void MainWindow::updateSettingsSummary()
{
    const QString role = serverMode_ ? QStringLiteral("服务端") : QStringLiteral("客户端");
    const QString endpoint = serverMode_ ? QStringLiteral("0.0.0.0:%1").arg(port_)
                                         : QStringLiteral("%1:%2").arg(host_).arg(port_);
    ui->roleValueLabel->setText(role);
    ui->endpointValueLabel->setText(endpoint);
    ui->savePathValueLabel->setText(savePath_);
    ui->connectButton->setText(serverMode_ ? QStringLiteral("开始监听")
                                           : QStringLiteral("连接服务器"));
}

void MainWindow::setSelectedFile(const QString &filePath)
{
    const QFileInfo info(filePath);
    selectedFilePath_ = filePath;
    ui->selectedFileLabel->setText(QStringLiteral("%1（%2 字节）")
                                       .arg(info.fileName())
                                       .arg(info.size()));
    ui->sendButton->setEnabled(transferManager_.isConnected());
}
