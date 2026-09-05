#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    // 对话框内部只处理表单交互；确认后通过 settingsApplied 信号把结果交给主窗口。
    connect(ui->browseButton, &QPushButton::clicked, this, &SettingsDialog::chooseSavePath);
    connect(ui->roleComboBox, &QComboBox::currentIndexChanged, this,
            &SettingsDialog::updateRoleUi);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
            &SettingsDialog::applyAndAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updateRoleUi();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::setSettings(bool serverMode, const QString &host, quint16 port,
                                 const QString &savePath)
{
    ui->roleComboBox->setCurrentIndex(serverMode ? 1 : 0);
    ui->hostEdit->setText(host);
    ui->portSpinBox->setValue(port);
    ui->savePathEdit->setText(savePath);
    updateRoleUi();
}

void SettingsDialog::chooseSavePath()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择接收文件保存目录"), ui->savePathEdit->text());
    if (!directory.isEmpty())
    {
        ui->savePathEdit->setText(directory);
    }
}

void SettingsDialog::applyAndAccept()
{
    const bool serverMode = ui->roleComboBox->currentIndex() == 1;
    const QString host = ui->hostEdit->text().trimmed();
    const QString savePath = QDir::cleanPath(ui->savePathEdit->text().trimmed());

    if (!serverMode && host.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("参数不完整"),
                             QStringLiteral("客户端模式必须填写服务器 IP 或主机名。"));
        return;
    }
    if (savePath.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("参数不完整"),
                             QStringLiteral("请选择接收文件保存目录。"));
        return;
    }

    // 提前创建目录，在开始监听或连接前就向用户反馈无效路径和权限问题。
    QDir directory;
    if (!directory.mkpath(savePath))
    {
        QMessageBox::warning(this, QStringLiteral("目录不可用"),
                             QStringLiteral("无法创建或访问保存目录：%1").arg(savePath));
        return;
    }

    // 跨窗口通信：设置对话框只发出参数，主窗口负责更新网络管理器。
    emit settingsApplied(serverMode, host, static_cast<quint16>(ui->portSpinBox->value()),
                         savePath);
    accept();
}

void SettingsDialog::updateRoleUi()
{
    const bool serverMode = ui->roleComboBox->currentIndex() == 1;
    // 服务端固定监听全部 IPv4 地址，无需用户填写远端主机；客户端则必须填写。
    ui->hostEdit->setEnabled(!serverMode);
    ui->hostLabel->setText(serverMode ? QStringLiteral("监听地址：")
                                      : QStringLiteral("服务器地址："));
    ui->hostEdit->setPlaceholderText(serverMode
                                         ? QStringLiteral("服务端监听全部 IPv4 地址")
                                         : QStringLiteral("例如 127.0.0.1"));
}
