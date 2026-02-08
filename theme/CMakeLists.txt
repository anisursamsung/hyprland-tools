cmake_minimum_required(VERSION 3.19)

project(theme VERSION 1.0 DESCRIPTION "Theme App - Wallpaper Selector")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(PkgConfig REQUIRED)

# Core Hyprland toolkit dependencies
pkg_check_modules(HYPRTOOLKIT REQUIRED IMPORTED_TARGET hyprtoolkit)
pkg_check_modules(HYPRUTILS REQUIRED IMPORTED_TARGET hyprutils)
pkg_check_modules(AQUAMARINE REQUIRED IMPORTED_TARGET aquamarine)

# Graphics dependencies for image rendering
pkg_check_modules(HYPRGRAPHICS REQUIRED IMPORTED_TARGET hyprgraphics)

# System dependencies
pkg_check_modules(PIXMAN REQUIRED IMPORTED_TARGET pixman-1)
pkg_check_modules(LIBDRM REQUIRED IMPORTED_TARGET libdrm)
pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(PANGO REQUIRED IMPORTED_TARGET pango)
pkg_check_modules(PANGOCAIRO REQUIRED IMPORTED_TARGET pangocairo)

# For notifications (optional)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNOTIFY QUIET IMPORTED_TARGET libnotify)

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
    message(STATUS "Configuring theme app in Debug mode")
    add_compile_definitions(THEME_DEBUG)
    add_compile_options(-g -O0)
else()
    add_compile_options(-O3)
    message(STATUS "Configuring theme app in Release mode")
endif()

# Create executable
add_executable(theme-app src/main.cpp)

# Include directories
target_include_directories(theme-app PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
)

# Link all required libraries
target_link_libraries(theme-app 
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
    xkbcommon
    stdc++fs
    pthread
    dl
    m
)

# Optional: link libnotify if available
if(TARGET PkgConfig::LIBNOTIFY)
    target_link_libraries(theme-app PkgConfig::LIBNOTIFY)
else()
    message(STATUS "libnotify not found - using terminal notifications")
endif()

# Installation
install(TARGETS theme-app RUNTIME DESTINATION bin)

# Optional: create a desktop entry
install(FILES ${CMAKE_SOURCE_DIR}/theme-app.desktop DESTINATION share/applications)