cmake_minimum_required(VERSION 3.21)
project(LegendDragonBones LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml)
include(GNUInstallDirs)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

add_subdirectory(src/quick)
