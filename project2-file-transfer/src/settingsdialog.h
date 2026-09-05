#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui
{
class SettingsDialog;
}

// 独立收集角色、地址、端口和保存路径，并通过自定义信号传递配置。
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;

    void setSettings(bool serverMode, const QString &host, quint16 port,
                     const QString &savePath);

signals:
    void settingsApplied(bool serverMode, const QString &host, quint16 port,
                         const QString &savePath);

private slots:
    void chooseSavePath();
    void applyAndAccept();
    void updateRoleUi();

private:
    Ui::SettingsDialog *ui;
};

#endif
