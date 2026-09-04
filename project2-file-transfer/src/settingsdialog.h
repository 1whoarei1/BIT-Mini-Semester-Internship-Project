#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui
{
class SettingsDialog;
}

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
