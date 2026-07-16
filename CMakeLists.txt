cmake_minimum_required(VERSION 3.16)
project(zerotier-status-daemon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

# Find Qt6 or Qt5
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Gui Core Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Gui Core Network REQUIRED)

add_executable(zerotier-status-daemon main.cpp)

target_link_libraries(zerotier-status-daemon PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)

install(TARGETS zerotier-status-daemon DESTINATION bin)
