QT += widgets network
CONFIG += c++17
TEMPLATE = app
TARGET = file_transfer_tool
INCLUDEPATH += src

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/settingsdialog.cpp \
    src/transfermanager.cpp \
    src/dropzonewidget.cpp

HEADERS += \
    src/mainwindow.h \
    src/settingsdialog.h \
    src/transfermanager.h \
    src/dropzonewidget.h

FORMS += \
    src/mainwindow.ui \
    src/settingsdialog.ui

RESOURCES += resources/resources.qrc
