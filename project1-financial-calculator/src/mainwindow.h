#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class DragTokenButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

  private slots:
    void appendToken();
    void clearExpression();
    void backspaceExpression();
    void calculateExpression();
    void calculateSimpleInterest();
    void calculateCompoundInterest();
    void openFinancialSettings();
    void updateFinancialParameters(double annualRatePercent, int years);

  private:
    void connectTokenButtons();
    void showCalculationError(const QString &message);
    void showFinanceResult(const QString &mode, double total);

    Ui::MainWindow *ui;
};
