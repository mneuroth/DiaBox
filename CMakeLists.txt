cmake_minimum_required(VERSION 3.21)
project(dia_box VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.9 REQUIRED COMPONENTS Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.9)

qt_add_executable(dia_box
    main.cpp
)

qt_add_qml_module(dia_box
    URI dia_box
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES
        DirectoryModel.h
        DirectoryModel.cpp
)

set_target_properties(dia_box PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE    TRUE
)

target_link_libraries(dia_box PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
)
