#pragma once

#include <QDialog>

namespace Ui
{
class FinancialSettingsDialog;
}

class FinancialSettingsDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit FinancialSettingsDialog(QWidget *parent = nullptr);
    ~FinancialSettingsDialog() override;

    void setParameters(double annualRatePercent, int years);

  signals:
    void parametersAccepted(double annualRatePercent, int years);

  private slots:
    void applyAndAccept();

  private:
    Ui::FinancialSettingsDialog *ui;
};
