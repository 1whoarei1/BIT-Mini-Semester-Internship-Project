QT += widgets
CONFIG += c++17
TEMPLATE = app
TARGET = financial_calculator
INCLUDEPATH += src

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/calculatormodel.cpp \
    src/dragtokenbutton.cpp \
    src/droplineedit.cpp \
    src/financialsettingsdialog.cpp

HEADERS += \
    src/mainwindow.h \
    src/calculatormodel.h \
    src/dragtokenbutton.h \
    src/droplineedit.h \
    src/financialsettingsdialog.h

FORMS += \
    src/mainwindow.ui \
    src/financialsettingsdialog.ui

RESOURCES += resources/resources.qrc
