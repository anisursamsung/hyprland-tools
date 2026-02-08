cmake_minimum_required(VERSION 3.19)

project(button_dropdown VERSION 0.1 DESCRIPTION "Button Dropdown Example")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET hyprtoolkit pixman-1 libdrm)

# Also find hyprutils explicitly
pkg_check_modules(HYPRUTILS REQUIRED IMPORTED_TARGET hyprutils)

add_executable(button_dropdown src/main.cpp)

# Link both libraries
target_link_libraries(button_dropdown
    PkgConfig::DEPS
    PkgConfig::HYPRUTILS
)