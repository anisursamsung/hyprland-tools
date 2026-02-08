cmake_minimum_required(VERSION 3.19)

project(toplayer VERSION 1.0 DESCRIPTION "Top Layer Example")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET hyprtoolkit pixman-1 libdrm)
pkg_check_modules(HYPRUTILS REQUIRED IMPORTED_TARGET hyprutils)

add_executable(toplayer src/main.cpp)

target_link_libraries(toplayer 
    PkgConfig::DEPS
    PkgConfig::HYPRUTILS
)