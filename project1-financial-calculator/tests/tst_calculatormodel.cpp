#include <QtTest>

#include "calculatormodel.h"
#include "droplineedit.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

class CalculatorModelTest : public QObject
{
    Q_OBJECT
  private slots:
    void evaluatesBasicOperations();
    void respectsOperatorPrecedence();
    void supportsParenthesesAndUnaryMinus();
    void reportsInvalidExpressions();
    void calculatesFinancialResults();
    void acceptsDraggedTokens();
};

void CalculatorModelTest::evaluatesBasicOperations()
{
    const auto addition = CalculatorModel::evaluate(QStringLiteral("12.5+7.5"));
    QVERIFY(addition.ok);
    QCOMPARE(addition.value, 20.0);
    const auto division = CalculatorModel::evaluate(QStringLiteral("18÷3"));
    QVERIFY(division.ok);
    QCOMPARE(division.value, 6.0);
}

void CalculatorModelTest::respectsOperatorPrecedence()
{
    const auto result = CalculatorModel::evaluate(QStringLiteral("10+2×3-4/2"));
    QVERIFY(result.ok);
    QCOMPARE(result.value, 14.0);
}

void CalculatorModelTest::supportsParenthesesAndUnaryMinus()
{
    const auto result = CalculatorModel::evaluate(QStringLiteral("-(2+3)*4"));
    QVERIFY(result.ok);
    QCOMPARE(result.value, -20.0);
}

void CalculatorModelTest::reportsInvalidExpressions()
{
    const auto divisionByZero = CalculatorModel::evaluate(QStringLiteral("8/0"));
    QVERIFY(!divisionByZero.ok);
    QVERIFY(divisionByZero.error.contains(QStringLiteral("零")));
    QVERIFY(!CalculatorModel::evaluate(QStringLiteral("2+")).ok);
    QVERIFY(!CalculatorModel::evaluate(QStringLiteral("(2+3")).ok);
}

void CalculatorModelTest::calculatesFinancialResults()
{
    QCOMPARE(CalculatorModel::simpleInterestTotal(10000.0, 3.0, 3), 10900.0);
    QVERIFY(qAbs(CalculatorModel::compoundInterestTotal(10000.0, 3.0, 3) - 10927.27) < 0.001);
}

void CalculatorModelTest::acceptsDraggedTokens()
{
    DropLineEdit edit;
    edit.show();
    edit.setText(QStringLiteral("12"));
    edit.setCursorPosition(edit.text().size());

    QMimeData mimeData;
    mimeData.setData(DropLineEdit::tokenMimeType(), QByteArray("+"));
    QDragEnterEvent enterEvent(QPoint(5, 5), Qt::CopyAction, &mimeData, Qt::LeftButton,
                               Qt::NoModifier);
    QApplication::sendEvent(&edit, &enterEvent);
    QVERIFY(enterEvent.isAccepted());

    QSignalSpy spy(&edit, &DropLineEdit::tokenDropped);
    QDropEvent dropEvent(QPointF(5, 5), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&edit, &dropEvent);
    QCOMPARE(edit.text(), QStringLiteral("12+"));
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(CalculatorModelTest)
#include "tst_calculatormodel.moc"
