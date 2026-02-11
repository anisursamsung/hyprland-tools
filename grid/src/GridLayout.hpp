#pragma once

#include "Box.hpp"
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <vector>
#include <memory>

class GridLayout {
  public:
    // Configuration structure for grid properties
    struct Config {
        float boxSize = 220.0f;           // Size of each box (square)
        float horizontalSpacing = 15.0f;  // Space between columns
        float verticalSpacing = 15.0f;    // Space between rows
        bool scrollable = true;           // Enable vertical scrolling
        bool centerHorizontal = true;     // Center grid horizontally
    };
    
    // Constructor
    GridLayout(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend,
               Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IWindow> window);
    
    // Destructor
    ~GridLayout();
    
    // Add a box to the grid
    void addBox(std::unique_ptr<Box> box);
    
    // Add multiple boxes
    void addBoxes(std::vector<std::unique_ptr<Box>> boxes);
    
    // Remove all boxes
    void clear();
    
    // Get the UI element to add to window/layout
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> getElement();
    
    // Update grid layout (call when window resizes or boxes change)
    void update();
    
    // Configuration
    void setConfig(const Config& newConfig);
    Config getConfig() const { return m_config; }
    
    // Getters for grid information
    int getColumnCount() const { return m_columnCount; }
    int getRowCount() const { return m_rowCount; }
    int getTotalBoxes() const { return static_cast<int>(m_boxes.size()); }
    float getGridWidth() const { return m_gridWidth; }
    float getGridHeight() const { return m_gridHeight; }
    
  private:
    // Dependencies
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IWindow> m_window;
    
    // Configuration
    Config m_config;
    
    // Box storage
    std::vector<std::unique_ptr<Box>> m_boxes;
    
    // UI Elements
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CScrollAreaElement> m_scrollArea;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> m_gridContainer;
    
    // Signal listener for window resize
    Hyprutils::Signal::CHyprSignalListener m_resizeListener;
    
    // Grid state
    int m_columnCount = 0;
    int m_rowCount = 0;
    float m_gridWidth = 0.0f;
    float m_gridHeight = 0.0f;
    
    // Private methods
    void createUI();
    void calculateLayout();
    void createGridStructure();
    void setupResizeHandler();
};