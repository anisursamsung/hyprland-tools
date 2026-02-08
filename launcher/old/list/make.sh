#!/bin/bash
set -e

echo "=== Building Enhanced App Launcher ==="
echo "Features: Icons + Auto-scroll + Extended Keyboard Navigation"

# Clean previous build
rm -rf build
mkdir -p build
cd build

# Configure with Release settings
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build with all cores
make -j$(nproc)

echo ""
echo "=== Build Successful! ==="
echo ""
echo "Run with: ./launcher"
echo ""
echo "🎯 Enhanced Features:"
echo "  ✓ Real .desktop file parsing"
echo "  ✓ Application icons"
echo "  ✓ Auto-scrolling for long lists"
echo "  ✓ Extended keyboard navigation:"
echo "    - ↑/↓/j/k = Move by 1"
echo "    - Page Up/Down = Jump 10 items"
echo "    - Home/End = First/Last item"
echo "  ✓ Launch actual applications"
echo ""
echo "Note: Some icons may show as placeholders if not found in current icon theme"