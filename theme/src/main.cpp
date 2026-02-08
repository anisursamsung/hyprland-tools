#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/system/Icons.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <functional>
#include <iostream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <cstdlib>

namespace fs = std::filesystem;

using namespace Hyprutils::Memory;
using namespace Hyprtoolkit;

// ============================================
// Simplified Data Structures
// ============================================

struct WallpaperItem {
    std::string filename;
    std::string path;
    fs::path filepath;
    std::string lowercaseFilename; // For case-insensitive search
    
    bool operator<(const WallpaperItem& other) const {
        return filename < other.filename;
    }
};

// ============================================
// Wallpaper Database
// ============================================

class WallpaperDatabase {
  public:
    WallpaperDatabase() {
        loadWallpapers();
    }
    
    const std::vector<WallpaperItem>& getAllWallpapers() const { return m_allWallpapers; }
    
  private:
    void loadWallpapers() {
        // Get downloads directory
        const char* home = std::getenv("HOME");
        if (!home) {
            std::cerr << "Warning: Could not find HOME directory" << std::endl;
            return;
        }
        
        fs::path downloadsPath = fs::path(home) / "Downloads";
        
        if (!fs::exists(downloadsPath)) {
            std::cerr << "Warning: Downloads directory not found at: " << downloadsPath << std::endl;
            return;
        }
        
        std::cout << "Scanning directory: " << downloadsPath << std::endl;
        
        // Supported image extensions
        static const std::vector<std::string> imageExtensions = {
            ".jpg", ".jpeg", ".png", ".bmp", ".gif", ".tiff", ".webp"
        };
        
        // Iterate through files in downloads
        for (const auto& entry : fs::directory_iterator(downloadsPath)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                
                // Convert to lowercase for comparison
                std::string lowerExt = extension;
                std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);
                
                // Check if it's an image file
                bool isImage = false;
                for (const auto& imgExt : imageExtensions) {
                    if (lowerExt == imgExt) {
                        isImage = true;
                        break;
                    }
                }
                
                if (isImage) {
                    WallpaperItem item;
                    item.filename = entry.path().filename().string();
                    item.path = entry.path().string();
                    item.filepath = entry.path();
                    item.lowercaseFilename = item.filename;
                    std::transform(item.lowercaseFilename.begin(), item.lowercaseFilename.end(), 
                                  item.lowercaseFilename.begin(), ::tolower);
                    
                    m_allWallpapers.push_back(item);
                }
            }
        }
        
        std::sort(m_allWallpapers.begin(), m_allWallpapers.end());
        std::cout << "Found " << m_allWallpapers.size() << " wallpaper images" << std::endl;
    }
    
    std::vector<WallpaperItem> m_allWallpapers;
};

// ============================================
// GridWallpaperItem - For Grid View
// ============================================

class GridWallpaperItem {
  public:
    GridWallpaperItem(const WallpaperItem& wallpaper, CSharedPointer<IBackend> backend,
                     std::function<void()> onHover = nullptr,
                     std::function<void()> onClick = nullptr)
        : m_wallpaper(wallpaper), m_backend(backend), m_onHover(onHover), m_onClick(onClick) {
        createUI();
    }
    
    ~GridWallpaperItem() = default;
    
    CSharedPointer<IElement> getElement() const { return m_background; }
    
    void setActive(bool active) {
        if (m_active == active) return;
        m_active = active;
        updateAppearance();
    }
    
    bool isActive() const { return m_active; }
    
    void setVisible(bool visible) {
        if (m_visible == visible) return;
        m_visible = visible;
        
        // Set opacity to show/hide
        if (auto builder = m_background->rebuild()) {
            builder->color([this, visible] { 
                auto palette = CPalette::palette();
                if (!palette) return CHyprColor(0.2, 0.2, 0.2, visible ? 0.3 : 0.0);
                
                auto color = visible ? palette->m_colors.alternateBase 
                                     : CHyprColor(0, 0, 0, 0);
                return color;
            })->commence();
        }
        
        // Also hide the text
        if (m_text) {
            if (auto builder = m_text->rebuild()) {
                builder->color([this, visible] { 
                    auto palette = CPalette::palette();
                    if (!palette) return CHyprColor(0.8, 0.8, 0.8, visible ? 1.0 : 0.0);
                    
                    return CHyprColor(palette->m_colors.text.r, palette->m_colors.text.g, 
                                     palette->m_colors.text.b, visible ? 1.0 : 0.0);
                })->commence();
            }
        }
    }
    
    bool isVisible() const { return m_visible; }
    
    void updateAppearance() {
        if (!m_background || !m_text) return;
        
        // Use system palette
        auto palette = CPalette::palette();
        
        if (auto builder = m_background->rebuild()) {
            if (palette) {
                auto& colors = palette->m_colors;
                builder->color([this, colors] { 
                    return m_active ? colors.accent.mix(colors.base, 0.3) 
                                   : colors.alternateBase;
                })->commence();
            } else {
                builder->color([this] { 
                    return m_active ? CHyprColor(0.2, 0.4, 0.8, 0.8) 
                                   : CHyprColor(0.2, 0.2, 0.2, 0.3);
                })->commence();
            }
        }
        if (auto builder = m_text->rebuild()) {
            if (palette) {
                auto& colors = palette->m_colors;
                builder->color([this, colors] { 
                    return m_active ? colors.brightText : colors.text;
                })->commence();
            } else {
                builder->color([this] { 
                    return m_active ? CHyprColor(1, 1, 1, 1) : CHyprColor(0.8, 0.8, 0.8, 1);
                })->commence();
            }
        }
        
        m_background->forceReposition();
    }
    
    const WallpaperItem& getWallpaper() const { return m_wallpaper; }
    
    bool matchesFilter(const std::string& filter) const {
        if (filter.empty()) return true;
        
        // Case-insensitive search
        return m_wallpaper.lowercaseFilename.find(filter) != std::string::npos;
    }
    
    void select() {
        // Send notification
        std::string command = "notify-send \"Theme App\" \"Selected: " + m_wallpaper.filename + "\"";
        std::system(command.c_str());
        std::cout << "Selected wallpaper: " << m_wallpaper.filename << std::endl;
    }
    
  private:
    void createUI() {
        auto palette = CPalette::palette();
        
        // Grid item dimensions
        const float ITEM_WIDTH = 180.0F;
        const float ITEM_HEIGHT = 180.0F;
        
        // Create background rectangle
        m_background = CRectangleBuilder::begin()
            ->color([palette] { 
                if (!palette) return CHyprColor(0.2, 0.2, 0.2, 0.3);
                return palette->m_colors.alternateBase;
            })
            ->rounding(palette ? palette->m_vars.smallRounding : 12)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {ITEM_WIDTH, ITEM_HEIGHT}))
            ->commence();
        
        // Main column layout
        m_columnLayout = CColumnLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT,
                                {1.0F, 1.0F}))
            ->commence();
        
        // Image container
        auto imageContainer = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, ITEM_WIDTH - 40.F}))
            ->commence();
        
        // Load and display the image
        auto imageElement = CImageBuilder::begin()
            ->path(std::string{m_wallpaper.path})
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT,
                                {1.0F, 1.0F}))
            ->fitMode(eImageFitMode::IMAGE_FIT_MODE_COVER)
            ->rounding(palette ? palette->m_vars.smallRounding : 8)
            ->sync(false)
            ->commence();
        imageContainer->addChild(imageElement);
        
        m_columnLayout->addChild(imageContainer);
        
        // Small gap
        auto gapElement = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 5.F}))
            ->commence();
        m_columnLayout->addChild(gapElement);
        
        // Text label (filename) with ellipsis
        m_text = CTextBuilder::begin()
            ->text(std::string{m_wallpaper.filename})
            ->color([palette] { 
                if (!palette) return CHyprColor(0.8, 0.8, 0.8, 1);
                return palette->m_colors.text;
            })
            ->align(eFontAlignment::HT_FONT_ALIGN_CENTER)
            ->fontSize(CFontSize(CFontSize::HT_FONT_SMALL, 0.9F))
            ->fontFamily(palette ? palette->m_vars.fontFamily : "Sans Serif")
            ->clampSize({ITEM_WIDTH - 20.F, 20.F})
            ->noEllipsize(false)
            ->commence();
        
        // Text container
        auto textContainer = CColumnLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 30.F}))
            ->commence();
        textContainer->addChild(m_text);
        
        m_columnLayout->addChild(textContainer);
        
        m_background->addChild(m_columnLayout);
        
        // Add mouse interaction
        setupMouseHandlers();
    }
    
    void setupMouseHandlers() {
        m_background->setReceivesMouse(true);
        
        // Hover enter
        m_background->setMouseEnter([this](const Hyprutils::Math::Vector2D&) {
            if (m_onHover) {
                m_onHover();
            }
        });
        
        // Hover leave
        m_background->setMouseLeave([this]() {
            // Handled by parent
        });
        
        // Click
        m_background->setMouseButton([this](Input::eMouseButton button, bool down) {
            if (button == Input::MOUSE_BUTTON_LEFT && down) {
                if (m_onClick) {
                    m_onClick();
                }
            }
        });
    }
    
    WallpaperItem m_wallpaper;
    CSharedPointer<IBackend> m_backend;
    bool m_active = false;
    bool m_visible = true;
    
    CSharedPointer<CRectangleElement> m_background;
    CSharedPointer<CColumnLayoutElement> m_columnLayout;
    CSharedPointer<CTextElement> m_text;
    
    std::function<void()> m_onHover;
    std::function<void()> m_onClick;
};

// ============================================
// Main ThemeApp Class
// ============================================

class ThemeApp {
  public:
    ThemeApp() {
        m_backend = IBackend::create();
        if (!m_backend) {
            throw std::runtime_error("Failed to create backend");
        }
        
        m_wallpaperDatabase = std::make_unique<WallpaperDatabase>();
        
        std::cout << "Theme App: Found " << m_wallpaperDatabase->getAllWallpapers().size() 
                  << " wallpaper images in Downloads" << std::endl;
    }
    
    void run() {
        createWindow();
        if (!m_window) {
            throw std::runtime_error("Failed to create window");
        }
        
        createUI();
        setupEventHandlers();

        std::cout << "\n=== Theme App Ready ===" << std::endl;
        std::cout << "Wallpapers: " << m_wallpaperDatabase->getAllWallpapers().size() << std::endl;
        std::cout << "Controls: ↑/↓/←/→ = Navigate, ↵ = Select, ⎋ = Close" << std::endl;
        std::cout << "Type to search, Ctrl+F to focus search box" << std::endl;
        std::cout << "Mouse: Hover to select, Click to select" << std::endl;
        std::cout << "===========================\n" << std::endl;

        m_window->open();
        m_backend->enterLoop();
    }
    
  private:
    void createWindow() {
        // Hardcoded window size for 4-column grid
        Hyprutils::Math::Vector2D preferredSize(750.0, 650.0);
        
        m_window = CWindowBuilder::begin()
            ->type(HT_WINDOW_LAYER)
            ->appTitle("Theme App - Wallpaper Selector")
            ->appClass("theme-app")
            ->preferredSize(preferredSize)
            ->anchor(1 | 2 | 4 | 8)  // All anchors (center the window)
            ->layer(3)               // Top layer
            ->marginTopLeft({100, 100})
            ->marginBottomRight({100, 100})
            ->kbInteractive(1)       // Allow keyboard interaction
            ->exclusiveZone(-1)      // Don't reserve space
            ->commence();
            
        if (!m_window) {
            throw std::runtime_error("Failed to create window");
        }
    }
    
    void createUI() {
        // Create root element
        auto root = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT,
                                {1.0F, 1.0F}))
            ->commence();
        m_window->m_rootElement = root;
        
        // Use system palette
        auto palette = CPalette::palette();
        if (!palette) {
            palette = CPalette::emptyPalette();
        }
        
        // Background with transparency
        m_background = CRectangleBuilder::begin()
            ->color([palette] { 
                auto color = palette->m_colors.background;
                return CHyprColor(color.r, color.g, color.b, 0.95);
            })
            ->rounding(palette->m_vars.bigRounding)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT,
                                {1.0F, 1.0F}))
            ->commence();

        // Main column layout
        m_mainLayout = CColumnLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT,
                                {1.0F, 1.0F}))
            ->commence();
        m_mainLayout->setMargin(12);

        // Title
        auto title = CTextBuilder::begin()
            ->text("Wallpaper Selector")
            ->color([palette] { return palette->m_colors.text; })
            ->fontSize(CFontSize(CFontSize::HT_FONT_H2, 1.0F))
            ->align(eFontAlignment::HT_FONT_ALIGN_CENTER)
            ->commence();
        m_mainLayout->addChild(title);

        // Search box
        m_searchBox = CTextboxBuilder::begin()
            ->placeholder("Search wallpapers...")
            ->defaultText("")
            ->onTextEdited([this](CSharedPointer<CTextboxElement>, const std::string& text) {
                // Filter items based on search text
                filterItems(text);
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 40.F}))
            ->commence();
        m_mainLayout->addChild(m_searchBox);

        // Status text
        m_statusText = CTextBuilder::begin()
            ->text("Loading wallpapers...")
            ->color([palette] { return palette->m_colors.text; })
            ->fontSize(CFontSize(CFontSize::HT_FONT_SMALL, 1.0F))
            ->commence();
        m_mainLayout->addChild(m_statusText);

        // Scroll area for grid
        m_scrollArea = CScrollAreaBuilder::begin()
            ->scrollY(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 500.F}))
            ->commence();
        m_scrollArea->setGrow(true);
        m_mainLayout->addChild(m_scrollArea);

        // Create the grid container
        createGridContainer();
        
        // Add all elements to the window
        m_background->addChild(m_mainLayout);
        root->addChild(m_background);
    }
    
    void createGridContainer() {
        // Clear previous state
        m_gridItems.clear();
        m_visibleItems.clear();
        m_selectedIndex = 0;
        
        auto& wallpapers = m_wallpaperDatabase->getAllWallpapers();
        
        // Update status text
        if (auto builder = m_statusText->rebuild()) {
            builder->text(std::string{"Found " + std::to_string(wallpapers.size()) + 
                                      " wallpaper" + (wallpapers.size() != 1 ? "s" : "") + 
                                      " in Downloads"})
                   ->commence();
        }
        
        if (wallpapers.empty()) {
            // Show "no wallpapers" message
            auto message = CTextBuilder::begin()
                ->text("No wallpaper images found in ~/Downloads")
                ->color([] { return CHyprColor(0.7, 0.7, 0.7, 1); })
                ->align(eFontAlignment::HT_FONT_ALIGN_CENTER)
                ->commence();
            m_scrollArea->addChild(message);
            return;
        }
        
        // Hardcoded grid configuration
        const int COLUMN_COUNT = 4;
        const float ITEM_WIDTH = 180.0F;
        const float ITEM_HEIGHT = 180.0F;
        const float ROW_GAP = 10.0F;
        const float COLUMN_GAP = 10.0F;
        
        size_t numWallpapers = wallpapers.size();
        size_t numRows = (numWallpapers + COLUMN_COUNT - 1) / COLUMN_COUNT;
        
        // Calculate total grid dimensions
        float totalGridWidth = (ITEM_WIDTH * COLUMN_COUNT) + 
                              (COLUMN_GAP * (COLUMN_COUNT - 1));
        float totalGridHeight = (ITEM_HEIGHT * numRows) + 
                               (ROW_GAP * (numRows - 1));
        
        // Create a container for the grid
        m_gridContainer = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {totalGridWidth, totalGridHeight}))
            ->commence();
        
        // Create the main grid layout (rows)
        m_gridLayout = CColumnLayoutBuilder::begin()
            ->gap(static_cast<size_t>(ROW_GAP))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT,
                                {1.0F, 1.0F}))
            ->commence();
        
        // Create rows
        for (size_t row = 0; row < numRows; row++) {
            auto rowLayout = CRowLayoutBuilder::begin()
                ->gap(static_cast<size_t>(COLUMN_GAP))
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {1.0F, ITEM_HEIGHT}))
                ->commence();
            
            // Add items to this row
            for (int col = 0; col < COLUMN_COUNT; col++) {
                size_t index = row * COLUMN_COUNT + col;
                
                if (index >= numWallpapers) {
                    // Add empty placeholder to maintain grid alignment
                    auto empty = CRectangleBuilder::begin()
                        ->color([] { return CHyprColor(0, 0, 0, 0); })
                        ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                            CDynamicSize::HT_SIZE_ABSOLUTE,
                                            {ITEM_WIDTH, ITEM_HEIGHT}))
                        ->commence();
                    rowLayout->addChild(empty);
                    continue;
                }
                
                const auto& wallpaper = wallpapers[index];
                size_t itemIndex = m_gridItems.size(); // Current size before adding
                
                // Create callbacks for this specific item
                auto onHover = [this, itemIndex]() {
                    selectItem(itemIndex);
                };
                
                auto onClick = [this, itemIndex]() {
                    selectItem(itemIndex);
                    selectCurrentWallpaper();
                };
                
                auto gridItem = std::make_shared<GridWallpaperItem>(wallpaper, m_backend, onHover, onClick);
                if (gridItem->getElement()) {
                    rowLayout->addChild(gridItem->getElement());
                    m_gridItems.push_back(gridItem);
                    
                    // Initially all items are visible
                    m_visibleItems.push_back(itemIndex);
                }
            }
            
            m_gridLayout->addChild(rowLayout);
        }
        
        m_gridContainer->addChild(m_gridLayout);
        m_scrollArea->addChild(m_gridContainer);
        
        // Select first item
        if (!m_gridItems.empty()) {
            selectFirstVisibleItem();
        }
    }
    
    void filterItems(const std::string& filter) {
        if (!m_searchBox) return;
        
        std::string lowercaseFilter = filter;
        std::transform(lowercaseFilter.begin(), lowercaseFilter.end(), 
                      lowercaseFilter.begin(), ::tolower);
        
        // Clear visible items
        m_visibleItems.clear();
        
        // Filter items
        for (size_t i = 0; i < m_gridItems.size(); ++i) {
            bool visible = m_gridItems[i]->matchesFilter(lowercaseFilter);
            m_gridItems[i]->setVisible(visible);
            
            if (visible) {
                m_visibleItems.push_back(i);
            }
        }
        
        // Update status text
        if (auto builder = m_statusText->rebuild()) {
            builder->text(std::string{"Showing " + std::to_string(m_visibleItems.size()) + 
                                      " of " + std::to_string(m_gridItems.size()) + 
                                      " wallpapers"})
                   ->commence();
        }
        
        // Select first visible item
        if (!m_visibleItems.empty()) {
            selectFirstVisibleItem();
        } else {
            m_selectedIndex = 0;
            for (auto& item : m_gridItems) {
                item->setActive(false);
            }
        }
    }
    
    void selectFirstVisibleItem() {
        if (m_visibleItems.empty()) return;
        
        // Deselect current item
        if (m_selectedIndex < m_gridItems.size() && m_gridItems[m_selectedIndex]->isActive()) {
            m_gridItems[m_selectedIndex]->setActive(false);
        }
        
        // Select first visible
        m_selectedIndex = m_visibleItems[0];
        m_gridItems[m_selectedIndex]->setActive(true);
        updateGridPosition();
        ensureSelectionVisible();
    }
    
    void selectItem(size_t index) {
        if (index >= m_gridItems.size()) return;
        
        // Deselect current item
        if (m_selectedIndex < m_gridItems.size()) {
            m_gridItems[m_selectedIndex]->setActive(false);
        }
        
        // Select new item
        m_selectedIndex = index;
        m_gridItems[m_selectedIndex]->setActive(true);
        
        // Update grid position
        updateGridPosition();
        
        // Ensure item is visible
        ensureSelectionVisible();
    }
    
    void updateGridPosition() {
        if (m_gridItems.empty()) return;
        
        const int COLUMN_COUNT = 4;
        m_gridRow = m_selectedIndex / COLUMN_COUNT;
        m_gridCol = m_selectedIndex % COLUMN_COUNT;
    }
    
    
    
    
    void moveGridSelection(int deltaRow, int deltaCol) {
    if (m_visibleItems.empty()) return;
    
    // Find current item in visible items
    auto it = std::find(m_visibleItems.begin(), m_visibleItems.end(), m_selectedIndex);
    if (it == m_visibleItems.end()) {
        selectFirstVisibleItem();
        return;
    }
    
    size_t visibleIndex = std::distance(m_visibleItems.begin(), it);
    
    // Move within visible items
    if (deltaRow != 0) {
        // For row navigation in grid, we need to consider columns
        const int COLUMN_COUNT = 4;
        int currentRow = static_cast<int>(m_selectedIndex) / COLUMN_COUNT;
        int currentCol = static_cast<int>(m_selectedIndex) % COLUMN_COUNT;
        
        int newRow = currentRow + deltaRow;
        
        // Find first visible item in the target row
        int newIndex = -1;
        for (size_t i = 0; i < m_gridItems.size(); ++i) {
            int row = static_cast<int>(i) / COLUMN_COUNT;
            int col = static_cast<int>(i) % COLUMN_COUNT;
            
            if (row == newRow && m_gridItems[i]->isVisible()) {
                // Prefer same column if available
                if (col == currentCol) {
                    newIndex = static_cast<int>(i);
                    break;
                } else if (newIndex == -1) {
                    newIndex = static_cast<int>(i);
                }
            }
        }
        
        if (newIndex != -1) {
            selectItem(static_cast<size_t>(newIndex));
            return;
        }
    } else if (deltaCol != 0) {
        // Move left/right within same row, considering visibility
        const int COLUMN_COUNT = 4;
        int currentRow = static_cast<int>(m_selectedIndex) / COLUMN_COUNT;
        int currentCol = static_cast<int>(m_selectedIndex) % COLUMN_COUNT;
        
        int newCol = currentCol + deltaCol;
        
        // Try to stay in same row first
        while (newCol >= 0 && newCol < COLUMN_COUNT) {
            size_t potentialIndex = currentRow * COLUMN_COUNT + newCol;
            if (potentialIndex < m_gridItems.size() && m_gridItems[potentialIndex]->isVisible()) {
                selectItem(potentialIndex);
                return;
            }
            newCol += deltaCol;
        }
        
        // If not found in current row, move to next/prev visible item
        int direction = deltaCol > 0 ? 1 : -1;
        if (direction > 0) {
            // Move forward
            if (visibleIndex + 1 < m_visibleItems.size()) {
                selectItem(m_visibleItems[visibleIndex + 1]);
                return;
            }
        } else {
            // Move backward
            if (visibleIndex > 0) {
                selectItem(m_visibleItems[visibleIndex - 1]);
                return;
            }
        }
    }
    
    // Fallback: move to next/prev visible item
    if (deltaRow > 0 || deltaCol > 0) {
        // Move forward
        if (visibleIndex + 1 < m_visibleItems.size()) {
            selectItem(m_visibleItems[visibleIndex + 1]);
        }
    } else {
        // Move backward
        if (visibleIndex > 0) {
            selectItem(m_visibleItems[visibleIndex - 1]);
        }
    }
}
    
    
    
    void ensureSelectionVisible() {
        if (m_gridItems.empty() || !m_scrollArea) return;
        
        // Grid view scrolling
        const float ITEM_HEIGHT = 180.0F;
        const float ROW_GAP = 10.0F;
        const float ROW_HEIGHT = ITEM_HEIGHT + ROW_GAP;
        const float SCROLL_AREA_HEIGHT = m_scrollArea->size().y;
        const float CURRENT_SCROLL = m_scrollArea->getCurrentScroll().y;
        
        const float SELECTION_TOP = static_cast<float>(m_gridRow) * ROW_HEIGHT;
        const float SELECTION_BOTTOM = SELECTION_TOP + ITEM_HEIGHT;
        
        if (SELECTION_TOP < CURRENT_SCROLL) {
            m_scrollArea->setScroll({0.F, SELECTION_TOP});
        } else if (SELECTION_BOTTOM > CURRENT_SCROLL + SCROLL_AREA_HEIGHT) {
            m_scrollArea->setScroll({0.F, SELECTION_BOTTOM - SCROLL_AREA_HEIGHT});
        }
    }
    
    void selectCurrentWallpaper() {
        if (m_selectedIndex >= m_gridItems.size()) return;
        
        std::cout << "Selecting wallpaper: " << m_gridItems[m_selectedIndex]->getWallpaper().filename << std::endl;
        m_gridItems[m_selectedIndex]->select();
    }
    
    void closeApp() {
        std::cout << "Closing Theme App" << std::endl;
        if (m_window) {
            m_window->close();
        }
    }
    
    void focusSearchBox() {
        if (m_searchBox) {
            m_searchBox->focus();
        }
    }
    
    void setupEventHandlers() {
        if (!m_window) return;
        
        m_window->m_events.layerClosed.listenStatic([this] { 
            if (m_backend) m_backend->destroy(); 
        });
        
        m_window->m_events.closeRequest.listenStatic([this] { 
            if (m_backend) m_backend->destroy(); 
        });

        m_keyboardListener = m_window->m_events.keyboardKey.listen([this](const Input::SKeyboardKeyEvent& event) {
            if (!event.down) return;

            // Check for Ctrl+F to focus search
            if (event.xkbKeysym == XKB_KEY_f && (event.modMask & Input::HT_MODIFIER_CTRL)) {
                focusSearchBox();
                return;
            }
            
            // If search box has focus, don't handle navigation keys
            if (m_searchBox && m_searchBox->currentText().size() > 0) {
                // Let the textbox handle its own keys
                return;
            }

            switch (event.xkbKeysym) {
                case XKB_KEY_Escape:
                    closeApp();
                    break;
                    
                case XKB_KEY_Down:
                    moveGridSelection(+1, 0);
                    break;
                    
                case XKB_KEY_Up:
                    moveGridSelection(-1, 0);
                    break;
                    
                case XKB_KEY_Right:
                    moveGridSelection(0, +1);
                    break;
                    
                case XKB_KEY_Left:
                    moveGridSelection(0, -1);
                    break;
                    
                case XKB_KEY_Return:
                case XKB_KEY_KP_Enter:
                    selectCurrentWallpaper();
                    break;
                    
                default:
                    // If it's a regular character, focus search box and add the character
                    if (!event.utf8.empty() && event.utf8[0] >= 32 && event.utf8[0] <= 126) {
                        focusSearchBox();
                    }
                    break;
            }
        });
    }
    
    // Member variables
    CSharedPointer<IBackend> m_backend;
    CSharedPointer<IWindow> m_window;
    
    CSharedPointer<CRectangleElement> m_background;
    CSharedPointer<CColumnLayoutElement> m_mainLayout;
    CSharedPointer<CTextboxElement> m_searchBox;
    CSharedPointer<CTextElement> m_statusText;
    CSharedPointer<CScrollAreaElement> m_scrollArea;
    CSharedPointer<CRectangleElement> m_gridContainer;
    CSharedPointer<CColumnLayoutElement> m_gridLayout;
    
    std::unique_ptr<WallpaperDatabase> m_wallpaperDatabase;
    std::vector<std::shared_ptr<GridWallpaperItem>> m_gridItems;
    
    std::vector<size_t> m_visibleItems; // Indices of visible items
    
    size_t m_selectedIndex = 0;
    size_t m_gridRow = 0;
    size_t m_gridCol = 0;
    
    Hyprutils::Signal::CHyprSignalListener m_keyboardListener;
};

int main() {
    try {
        std::cout << "=== Theme App Starting ===" << std::endl;
        std::cout << "Scanning ~/Downloads for wallpaper images..." << std::endl;
        
        ThemeApp app;
        app.run();
        
        std::cout << "=== Theme App Exited ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}