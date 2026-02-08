cmake_minimum_required(VERSION 3.19)

project(launcher VERSION 2.0 DESCRIPTION "Improved App Launcher")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find all required packages
find_package(PkgConfig REQUIRED)

# Core Hyprland toolkit dependencies
pkg_check_modules(HYPRTOOLKIT REQUIRED IMPORTED_TARGET hyprtoolkit)
pkg_check_modules(HYPRUTILS REQUIRED IMPORTED_TARGET hyprutils)
pkg_check_modules(AQUAMARINE REQUIRED IMPORTED_TARGET aquamarine)

# Graphics dependencies for icon rendering
pkg_check_modules(HYPRGRAPHICS REQUIRED IMPORTED_TARGET hyprgraphics)

# System dependencies
pkg_check_modules(PIXMAN REQUIRED IMPORTED_TARGET pixman-1)
pkg_check_modules(LIBDRM REQUIRED IMPORTED_TARGET libdrm)
pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(PANGO REQUIRED IMPORTED_TARGET pango)
pkg_check_modules(PANGOCAIRO REQUIRED IMPORTED_TARGET pangocairo)

# For icon theme support
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)

# Compiler flags
add_compile_options(
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-value
    -Wno-missing-field-initializers
    -Wpedantic
)

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES DEBUG)
    message(STATUS "Configuring launcher in Debug")
    add_compile_definitions(LAUNCHER_DEBUG)
    add_compile_options(-g -O0)
else()
    add_compile_options(-O3)
    message(STATUS "Configuring launcher in Release")
endif()

# Explicitly list source files instead of using GLOB
set(SRCFILES
    src/ConfigManager.cpp
    src/main.cpp
)

# Create executable
add_executable(launcher ${SRCFILES})

# Include directories
target_include_directories(launcher PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
)

# Link all required libraries
target_link_libraries(launcher 
    PkgConfig::HYPRTOOLKIT
    PkgConfig::HYPRUTILS
    PkgConfig::AQUAMARINE
    PkgConfig::HYPRGRAPHICS
    PkgConfig::PIXMAN
    PkgConfig::LIBDRM
    PkgConfig::FONTCONFIG
    PkgConfig::CAIRO
    PkgConfig::PANGO
    PkgConfig::PANGOCAIRO
    PkgConfig::GTK3
    xkbcommon
    stdc++fs
    pthread
    dl
    m
)

# Installation
install(TARGETS launcher RUNTIME DESTINATION bin)