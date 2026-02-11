cmake_minimum_required(VERSION 3.19)

project(grid-search VERSION 1.0 DESCRIPTION "Grid Search with Image Gallery")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(PkgConfig REQUIRED)

# Core dependencies
pkg_check_modules(HYPRTOOLKIT REQUIRED IMPORTED_TARGET hyprtoolkit)
pkg_check_modules(HYPRUTILS REQUIRED IMPORTED_TARGET hyprutils)
pkg_check_modules(AQUAMARINE REQUIRED IMPORTED_TARGET aquamarine)
pkg_check_modules(HYPRGRAPHICS REQUIRED IMPORTED_TARGET hyprgraphics)

# System dependencies
pkg_check_modules(PIXMAN REQUIRED IMPORTED_TARGET pixman-1)
pkg_check_modules(LIBDRM REQUIRED IMPORTED_TARGET libdrm)

# Compiler flags
add_compile_options(
    -Wall
    -Wextra
    -Wno-unused-parameter
)

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES DEBUG)
    add_compile_options(-g -O0)
    message(STATUS "Configuring in Debug mode")
else()
    add_compile_options(-O3)
    message(STATUS "Configuring in Release mode")
endif()

# Source files (including SearchBox)
set(SRC_FILES 
    src/main.cpp
    src/Box.cpp
    src/GridLayout.cpp
    src/SearchBox.cpp
)

# Header files
set(HEADER_FILES
    src/Box.hpp
    src/GridLayout.hpp
    src/SearchBox.hpp
)

# Create executable
add_executable(grid-search ${SRC_FILES} ${HEADER_FILES})

# Include directories
target_include_directories(grid-search PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(grid-search 
    PkgConfig::HYPRTOOLKIT
    PkgConfig::HYPRUTILS
    PkgConfig::AQUAMARINE
    PkgConfig::HYPRGRAPHICS
    PkgConfig::PIXMAN
    PkgConfig::LIBDRM
    pthread
    dl
    m
)

# Installation
install(TARGETS grid-search RUNTIME DESTINATION bin)