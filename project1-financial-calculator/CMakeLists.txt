cmake_minimum_required(VERSION 3.16)

project(FinancialCalculator VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Test)

qt_add_executable(financial_calculator
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/calculatormodel.cpp
    src/calculatormodel.h
    src/dragtokenbutton.cpp
    src/dragtokenbutton.h
    src/droplineedit.cpp
    src/droplineedit.h
    src/financialsettingsdialog.cpp
    src/financialsettingsdialog.h
    src/financialsettingsdialog.ui
    resources/resources.qrc
)

target_include_directories(financial_calculator PRIVATE src)
target_link_libraries(financial_calculator PRIVATE Qt6::Widgets)

include(CTest)
if(BUILD_TESTING)
    qt_add_executable(financial_calculator_tests
        tests/tst_calculatormodel.cpp
        src/calculatormodel.cpp
        src/calculatormodel.h
        src/dragtokenbutton.cpp
        src/dragtokenbutton.h
        src/droplineedit.cpp
        src/droplineedit.h
    )
    target_include_directories(financial_calculator_tests PRIVATE src)
    target_link_libraries(financial_calculator_tests PRIVATE Qt6::Test Qt6::Widgets)
    add_test(NAME financial_calculator_tests COMMAND financial_calculator_tests)
    set_tests_properties(financial_calculator_tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()
