cmake_minimum_required(VERSION 3.21)
project(PrintStation VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.5)   # activa AUTOMOC, AUTORCC, etc.

# MACOSX_BUNDLE: genera PrintStation.app. Además evita un choque de nombres:
# qt_add_qml_module crea la carpeta build/<preset>/PrintStation/ (qmldir del módulo),
# y un ejecutable suelto con el mismo nombre no podría escribirse ahí.
qt_add_executable(PrintStation MACOSX_BUNDLE src/main.cpp)

# Los .qml viven en qml/: con QTP0004 en NEW, Qt genera un qmldir también para esa subcarpeta.
qt_policy(SET QTP0004 NEW)

qt_add_qml_module(PrintStation
    URI PrintStation
    VERSION 1.0
    SOURCES
        src/core/PrinterState.h
        src/core/PrintJob.h
        src/core/PrintEngine.h
        src/core/PrintEngine.cpp
        src/viewmodels/PrinterViewModel.h
        src/viewmodels/PrinterViewModel.cpp
        src/viewmodels/JobQueueModel.h
        src/viewmodels/JobQueueModel.cpp
    QML_FILES
        qml/Main.qml
        qml/HomePage.qml
        qml/QueuePage.qml
        qml/JobDelegate.qml
)

target_include_directories(PrintStation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/viewmodels
)

target_link_libraries(PrintStation PRIVATE Qt6::Quick Qt6::QuickControls2)
