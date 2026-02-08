#!/bin/bash

# Simple make script for Teditor

echo "Building Teditor..."

# Clean previous build
rm -rf build
mkdir -p build

cd build

# Configure with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
make -j$(nproc)

echo "Build complete!"
echo "Run with: ./build/teditor"