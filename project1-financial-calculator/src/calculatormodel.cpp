#include "calculatormodel.h"

#include <cmath>

namespace
{
class ExpressionParser
{
  public:
    explicit ExpressionParser(QString expression) : m_expression(std::move(expression))
    {
        // 先统一处理界面输入的空格、乘号和除号，便于后续解析。
        m_expression.remove(' ');
        m_expression.replace(QChar(0x00D7), '*');
        m_expression.replace(QChar(0x00F7), '/');
    }

    CalculationResult parse()
    {
        if (m_expression.isEmpty())
        {
            return failure(QStringLiteral("表达式不能为空"));
        }

        // 使用递归下降方法按优先级解析整个表达式。
        const double value = parseExpression();
        if (!m_error.isEmpty())
        {
            return failure(m_error);
        }
        if (m_position != m_expression.size())
        {
            return failure(QStringLiteral("表达式格式错误"));
        }
        if (!std::isfinite(value))
        {
            return failure(QStringLiteral("计算结果超出范围"));
        }
        return {true, value, {}};
    }

  private:
    // 处理加减运算，parseTerm() 保证乘除优先计算。
    double parseExpression()
    {
        double value = parseTerm();
        while (m_error.isEmpty() && m_position < m_expression.size())
        {
            const QChar operation = m_expression.at(m_position);
            if (operation != '+' && operation != '-')
            {
                break;
            }
            ++m_position;
            const double right = parseTerm();
            value = operation == '+' ? value + right : value - right;
        }
        return value;
    }

    // 处理乘除运算，并在这里检查除数是否为零。
    double parseTerm()
    {
        double value = parseFactor();
        while (m_error.isEmpty() && m_position < m_expression.size())
        {
            const QChar operation = m_expression.at(m_position);
            if (operation != '*' && operation != '/')
            {
                break;
            }
            ++m_position;
            const double right = parseFactor();
            if (operation == '/')
            {
                if (std::abs(right) < 1e-12)
                {
                    m_error = QStringLiteral("除数不能为零");
                    return 0.0;
                }
                value /= right;
            }
            else
            {
                value *= right;
            }
        }
        return value;
    }

    double parseFactor()
    {
        if (m_position >= m_expression.size())
        {
            m_error = QStringLiteral("表达式不完整");
            return 0.0;
        }

        const QChar current = m_expression.at(m_position);
        if (current == '+' || current == '-')
        {
            ++m_position;
            const double value = parseFactor();
            return current == '-' ? -value : value;
        }

        if (current == '(')
        {
            ++m_position;
            const double value = parseExpression();
            if (m_position >= m_expression.size() || m_expression.at(m_position) != ')')
            {
                m_error = QStringLiteral("括号不匹配");
                return 0.0;
            }
            ++m_position;
            return value;
        }

        return parseNumber();
    }

    // 从当前位置读取一个整数或小数，并交给 Qt 做格式转换。
    double parseNumber()
    {
        const qsizetype start = m_position;
        bool hasDecimalPoint = false;
        while (m_position < m_expression.size())
        {
            const QChar current = m_expression.at(m_position);
            if (current.isDigit())
            {
                ++m_position;
                continue;
            }
            if (current == '.' && !hasDecimalPoint)
            {
                hasDecimalPoint = true;
                ++m_position;
                continue;
            }
            break;
        }

        if (start == m_position)
        {
            m_error = QStringLiteral("此处应输入数字");
            return 0.0;
        }

        bool ok = false;
        const double value = m_expression.mid(start, m_position - start).toDouble(&ok);
        if (!ok)
        {
            m_error = QStringLiteral("数字格式错误");
            return 0.0;
        }
        return value;
    }

    static CalculationResult failure(const QString &message)
    {
        return {false, 0.0, message};
    }

    QString m_expression;
    qsizetype m_position = 0;
    QString m_error;
};
} // namespace

CalculationResult CalculatorModel::evaluate(const QString &expression)
{
    return ExpressionParser(expression).parse();
}

double CalculatorModel::simpleInterestTotal(double principal, double annualRatePercent, int years)
{
    // 年利率以百分数传入，因此计算前需要除以 100。
    return principal * (1.0 + annualRatePercent / 100.0 * years);
}

double CalculatorModel::compoundInterestTotal(double principal, double annualRatePercent, int years)
{
    // 复利按照“本金 × (1 + 年利率)^年限”计算到期本息。
    return principal * std::pow(1.0 + annualRatePercent / 100.0, years);
}
