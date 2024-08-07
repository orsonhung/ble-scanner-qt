cmake_minimum_required(VERSION 3.21.1)

project(ble_scanner VERSION 0.1 LANGUAGES CXX)
#aux_source_directory(ble_scanner PROJECT_SOURCES)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Bluetooth Core Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Bluetooth Core Gui)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        bluetoothdevice.h
        bluetoothdevice.cpp
        datautility.h
        datautility.cpp
)

add_definitions(-DQT_QOBJECT)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ble_scanner
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ble_scanner APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(ble_scanner SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(ble_scanner
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(ble_scanner PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(ble_scanner PUBLIC
    Qt${QT_VERSION_MAJOR}::Bluetooth
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
)

set_target_properties(ble_scanner PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if (APPLE)
    # Using absolute path for shared plist files is a Ninja bug workaround
    if (IOS)
        set_target_properties(ble_scanner PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.cmake.ios.plist"
        )
    else()
        set_target_properties(ble_scanner PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.cmake.macos.plist"
        )
    endif()
endif()
message("PROJECT_SOURCES: ${PROJECT_SOURCES}")
message("CMAKE_INSTALL_LIBDIR: ${CMAKE_INSTALL_LIBDIR}")
message("CMAKE_INSTALL_BINDIR: ${CMAKE_INSTALL_BINDIR}")
install(TARGETS ble_scanner
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ble_scanner)
endif()
