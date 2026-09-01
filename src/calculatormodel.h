#pragma once

#include <QString>

struct CalculationResult
{
    bool ok = false;
    double value = 0.0;
    QString error;
};

class CalculatorModel
{
  public:
    static CalculationResult evaluate(const QString &expression);
    static double simpleInterestTotal(double principal, double annualRatePercent, int years);
    static double compoundInterestTotal(double principal, double annualRatePercent, int years);
};
