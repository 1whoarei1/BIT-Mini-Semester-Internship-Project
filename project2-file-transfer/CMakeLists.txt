cmake_minimum_required(VERSION 3.16)

project(FileTransferTool VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Network Test)

qt_add_executable(file_transfer_tool
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/settingsdialog.cpp
    src/settingsdialog.h
    src/settingsdialog.ui
    src/transfermanager.cpp
    src/transfermanager.h
    src/dropzonewidget.cpp
    src/dropzonewidget.h
    resources/resources.qrc
)

target_include_directories(file_transfer_tool PRIVATE src)
target_link_libraries(file_transfer_tool PRIVATE Qt6::Widgets Qt6::Network)

include(CTest)
if(BUILD_TESTING)
    qt_add_executable(file_transfer_tests
        tests/tst_transfermanager.cpp
        src/transfermanager.cpp
        src/transfermanager.h
    )
    target_include_directories(file_transfer_tests PRIVATE src)
    target_link_libraries(file_transfer_tests PRIVATE Qt6::Test Qt6::Network Qt6::Gui)
    add_test(NAME file_transfer_tests COMMAND file_transfer_tests)
    set_tests_properties(file_transfer_tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()
