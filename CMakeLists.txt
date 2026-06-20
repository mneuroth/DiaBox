cmake_minimum_required(VERSION 3.21)
project(DiaBox VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.9 REQUIRED COMPONENTS Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.9)

qt_add_executable(dia_box
    main.cpp
)

qt_add_qml_module(dia_box
    URI DiaBox
    VERSION 0.2
    QML_FILES
        Main.qml
    SOURCES
        DirectoryModel.h
        DirectoryModel.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(dia_box PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.dia_box
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE    TRUE
)

target_link_libraries(dia_box PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
)

install(TARGETS dia_box
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
