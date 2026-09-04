#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "calculatormodel.h"
#include "dragtokenbutton.h"
#include "financialsettingsdialog.h"

#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 统一连接按钮、输入框和设置对话框的信号与槽。
    connectTokenButtons();

    connect(ui->clearButton, &QPushButton::clicked, this, &MainWindow::clearExpression);
    connect(ui->backspaceButton, &QPushButton::clicked, this, &MainWindow::backspaceExpression);
    connect(ui->equalsButton, &QPushButton::clicked, this, &MainWindow::calculateExpression);
    connect(ui->simpleInterestButton, &QPushButton::clicked, this,
            &MainWindow::calculateSimpleInterest);
    connect(ui->compoundInterestButton, &QPushButton::clicked, this,
            &MainWindow::calculateCompoundInterest);
    connect(ui->settingsButton, &QPushButton::clicked, this, &MainWindow::openFinancialSettings);
    connect(ui->expressionEdit, &QLineEdit::returnPressed, this, &MainWindow::calculateExpression);

    connect(ui->expressionEdit, &DropLineEdit::tokenDropped, this,
            [this](const QString &token)
            { statusBar()->showMessage(QStringLiteral("已拖入：%1").arg(token), 1500); });

    statusBar()->showMessage(QStringLiteral("就绪：可点击按钮，也可把数字或运算符拖入输入框"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectTokenButtons()
{
    const auto tokenButtons = findChildren<DragTokenButton *>();
    for (DragTokenButton *button : tokenButtons)
    {
        // 点击信号统一连接到一个槽，槽中通过 sender() 识别按钮。
        connect(button, &QPushButton::clicked, this, &MainWindow::appendToken);
    }
}

void MainWindow::appendToken()
{
    const auto *button = qobject_cast<DragTokenButton *>(sender());
    if (!button)
    {
        return;
    }
    ui->expressionEdit->insert(button->token());
    ui->expressionEdit->setFocus();
}

void MainWindow::clearExpression()
{
    ui->expressionEdit->clear();
    ui->resultLabel->setText(QStringLiteral("结果：--"));
    statusBar()->showMessage(QStringLiteral("已清除"), 1500);
}

void MainWindow::backspaceExpression()
{
    ui->expressionEdit->backspace();
    ui->expressionEdit->setFocus();
}

void MainWindow::calculateExpression()
{
    // 主窗口只负责展示结果，表达式解析交由 CalculatorModel 完成。
    const CalculationResult result = CalculatorModel::evaluate(ui->expressionEdit->text());
    if (!result.ok)
    {
        showCalculationError(result.error);
        return;
    }

    ui->resultLabel->setText(QStringLiteral("结果：%1").arg(result.value, 0, 'g', 14));
    statusBar()->showMessage(QStringLiteral("计算完成"), 2000);
}

void MainWindow::calculateSimpleInterest()
{
    const double total = CalculatorModel::simpleInterestTotal(
        ui->principalSpinBox->value(), ui->rateSpinBox->value(), ui->yearsSpinBox->value());
    showFinanceResult(QStringLiteral("单利"), total);
}

void MainWindow::calculateCompoundInterest()
{
    const double total = CalculatorModel::compoundInterestTotal(
        ui->principalSpinBox->value(), ui->rateSpinBox->value(), ui->yearsSpinBox->value());
    showFinanceResult(QStringLiteral("复利"), total);
}

void MainWindow::openFinancialSettings()
{
    FinancialSettingsDialog dialog(this);
    dialog.setParameters(ui->rateSpinBox->value(), ui->yearsSpinBox->value());

    // 跨窗口通信：对话框发出参数信号，主窗口槽函数负责更新界面。
    connect(&dialog, &FinancialSettingsDialog::parametersAccepted, this,
            &MainWindow::updateFinancialParameters);
    dialog.exec();
}

void MainWindow::updateFinancialParameters(double annualRatePercent, int years)
{
    ui->rateSpinBox->setValue(annualRatePercent);
    ui->yearsSpinBox->setValue(years);
    statusBar()->showMessage(QStringLiteral("财务参数已更新"), 2000);
}

void MainWindow::showCalculationError(const QString &message)
{
    ui->resultLabel->setText(QStringLiteral("结果：错误"));
    statusBar()->showMessage(message, 5000);
    QMessageBox::warning(this, QStringLiteral("计算错误"), message);
}

void MainWindow::showFinanceResult(const QString &mode, double total)
{
    const double principal = ui->principalSpinBox->value();
    const double interest = total - principal;
    ui->financeResultLabel->setText(QStringLiteral("%1到期本息：¥%2\n其中利息：¥%3")
                                        .arg(mode)
                                        .arg(total, 0, 'f', 2)
                                        .arg(interest, 0, 'f', 2));
    statusBar()->showMessage(QStringLiteral("%1计算完成").arg(mode), 2000);
}
