#include "financialsettingsdialog.h"
#include "ui_financialsettingsdialog.h"

#include <QDialogButtonBox>

FinancialSettingsDialog::FinancialSettingsDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::FinancialSettingsDialog)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
            &FinancialSettingsDialog::applyAndAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

FinancialSettingsDialog::~FinancialSettingsDialog()
{
    delete ui;
}

void FinancialSettingsDialog::setParameters(double annualRatePercent, int years)
{
    ui->rateSpinBox->setValue(annualRatePercent);
    ui->yearsSpinBox->setValue(years);
}

void FinancialSettingsDialog::applyAndAccept()
{
    // 对话框通过自定义信号把财务参数传回主窗口。
    emit parametersAccepted(ui->rateSpinBox->value(), ui->yearsSpinBox->value());
    accept();
}
