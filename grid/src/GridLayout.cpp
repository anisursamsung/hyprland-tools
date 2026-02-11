#include "GridLayout.hpp"
#include <iostream>

GridLayout::GridLayout(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend,
                       Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IWindow> window)
    : m_backend(backend), m_window(window) {
    
    if (!m_backend || !m_window) {
        throw std::runtime_error("GridLayout requires valid backend and window");
    }
    
    createUI();
    setupResizeHandler();
}

GridLayout::~GridLayout() {
    // Listener will be automatically cleaned up when destroyed
}

void GridLayout::setupResizeHandler() {
    // Setup window resize handler - use idle callback to avoid recursion
    m_resizeListener = m_window->m_events.resized.listen([this](Hyprutils::Math::Vector2D newSize) {
        std::cout << "[GridLayout] Window resized to: " << newSize.x << "x" << newSize.y << std::endl;
        
        // Schedule update on next idle to avoid multiple updates during resize
        if (m_backend) {
            m_backend->addIdle([this]() {
                std::cout << "[GridLayout] Processing resize update..." << std::endl;
                update();
            });
        }
    });
}

void GridLayout::createUI() {
    if (m_config.scrollable) {
        // Create scrollable area
        m_scrollArea = Hyprtoolkit::CScrollAreaBuilder::begin()
            ->scrollY(true)
            ->scrollX(false)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0F, 1.0F}))
            ->commence();
        
        // Create grid container inside scroll area
        m_gridContainer = Hyprtoolkit::CColumnLayoutBuilder::begin()
            ->gap(0)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {1.0F, 1.0F}))
            ->commence();
        
        m_scrollArea->addChild(m_gridContainer);
    } else {
        // Non-scrollable container
        m_gridContainer = Hyprtoolkit::CColumnLayoutBuilder::begin()
            ->gap(0)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0F, 1.0F}))
            ->commence();
    }
}

void GridLayout::addBox(std::unique_ptr<Box> box) {
    if (!box) return;
    
    m_boxes.push_back(std::move(box));
    update();
}

void GridLayout::addBoxes(std::vector<std::unique_ptr<Box>> boxes) {
    for (auto& box : boxes) {
        if (box) {
            m_boxes.push_back(std::move(box));
        }
    }
    update();
}

void GridLayout::clear() {
    m_boxes.clear();
    update();
}

Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> GridLayout::getElement() {
    return m_config.scrollable ? 
           static_cast<Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement>>(m_scrollArea) :
           static_cast<Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement>>(m_gridContainer);
}

void GridLayout::update() {
    std::cout << "[GridLayout] Updating layout..." << std::endl;
    calculateLayout();
    createGridStructure();
    
    // Force reposition of all elements
    if (m_gridContainer) {
        m_gridContainer->forceReposition();
    }
    if (m_scrollArea) {
        m_scrollArea->forceReposition();
    }
}

void GridLayout::setConfig(const Config& newConfig) {
    m_config = newConfig;
    
    // Recreate UI if scrollable setting changed
    bool needsRecreate = false;
    if (m_config.scrollable && !m_scrollArea) {
        needsRecreate = true;
    } else if (!m_config.scrollable && m_scrollArea) {
        needsRecreate = true;
    }
    
    if (needsRecreate) {
        createUI();
    }
    
    update();
}

void GridLayout::calculateLayout() {
    if (m_boxes.empty() || !m_window) {
        m_columnCount = 0;
        m_rowCount = 0;
        m_gridWidth = 0.0f;
        m_gridHeight = 0.0f;
        std::cout << "[GridLayout] No boxes to layout" << std::endl;
        return;
    }
    
    // Get window size
    auto windowSize = m_window->pixelSize();
    float windowWidth = windowSize.x;
    
    // Calculate how many columns can fit
    float totalWidthPerColumn = m_config.boxSize + m_config.horizontalSpacing;
    int maxColumns = static_cast<int>(windowWidth / totalWidthPerColumn);
    
    // Ensure reasonable bounds
    int totalBoxes = static_cast<int>(m_boxes.size());
    m_columnCount = std::max(1, std::min(maxColumns, totalBoxes));
    
    // Calculate rows needed (ceiling division)
    m_rowCount = (totalBoxes + m_columnCount - 1) / m_columnCount;
    
    // Calculate grid dimensions
    m_gridWidth = (m_columnCount * m_config.boxSize) + 
                  ((m_columnCount - 1) * m_config.horizontalSpacing);
    m_gridHeight = (m_rowCount * m_config.boxSize) + 
                   ((m_rowCount - 1) * m_config.verticalSpacing);
    
    // Debug output
    std::cout << "[GridLayout] Layout calculated:" << std::endl;
    std::cout << "  Window: " << windowWidth << "x" << windowSize.y << std::endl;
    std::cout << "  Columns: " << m_columnCount << std::endl;
    std::cout << "  Rows: " << m_rowCount << std::endl;
    std::cout << "  Grid size: " << m_gridWidth << "x" << m_gridHeight << std::endl;
    std::cout << "  Total boxes: " << totalBoxes << std::endl;
}

void GridLayout::createGridStructure() {
    // Clear existing content
    m_gridContainer->clearChildren();
    
    if (m_boxes.empty()) {
        std::cout << "[GridLayout] No boxes to create grid" << std::endl;
        return;
    }
    
    // Create wrapper for centering (if enabled)
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> gridWrapper;
    
    if (m_config.centerHorizontal) {
        gridWrapper = Hyprtoolkit::CColumnLayoutBuilder::begin()
            ->gap(0)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {1.0F, 1.0F}))
            ->commence();
        
        gridWrapper->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_AUTO);
        gridWrapper->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_HCENTER, true);
        
        std::cout << "[GridLayout] Grid will be centered horizontally" << std::endl;
    } else {
        gridWrapper = m_gridContainer;
        std::cout << "[GridLayout] Grid will be left-aligned" << std::endl;
    }
    
    // Update vertical spacing
    if (auto builder = gridWrapper->rebuild()) {
        builder->gap(m_config.verticalSpacing)->commence();
    }
    
    // Create rows
    int totalBoxes = static_cast<int>(m_boxes.size());
    
    std::cout << "[GridLayout] Creating " << m_rowCount << " rows..." << std::endl;
    
    for (int row = 0; row < m_rowCount; ++row) {
        // Create row layout with exact width
        auto rowLayout = Hyprtoolkit::CRowLayoutBuilder::begin()
            ->gap(m_config.horizontalSpacing)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO,
                {m_gridWidth, m_config.boxSize}))
            ->commence();
        
        // Add boxes to row
        for (int col = 0; col < m_columnCount; ++col) {
            int index = (row * m_columnCount) + col;
            
            if (index < totalBoxes && m_boxes[index] && m_boxes[index]->getElement()) {
                rowLayout->addChild(m_boxes[index]->getElement());
            } else if (index >= totalBoxes) {
                // Empty placeholder for alignment
                auto emptySpace = Hyprtoolkit::CRectangleBuilder::begin()
                    ->color([] { return Hyprtoolkit::CHyprColor(0, 0, 0, 0); })
                    ->size(Hyprtoolkit::CDynamicSize(
                        Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                        Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                        {m_config.boxSize, m_config.boxSize}))
                    ->commence();
                rowLayout->addChild(emptySpace);
            }
        }
        
        // Add row to wrapper
        gridWrapper->addChild(rowLayout);
        
        // Force reposition of row immediately
        rowLayout->forceReposition();
    }
    
    // Add wrapper to container if centering is enabled
    if (m_config.centerHorizontal && gridWrapper != m_gridContainer) {
        m_gridContainer->addChild(gridWrapper);
    }
    
    // Force reposition of entire container
    m_gridContainer->forceReposition();
    if (m_scrollArea) {
        m_scrollArea->forceReposition();
    }
    
    std::cout << "[GridLayout] Grid structure created successfully" << std::endl;
}