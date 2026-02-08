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
#include <fstream>
#include <algorithm>
#include <cctype>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <cstdlib>
#include <cmath>

namespace fs = std::filesystem;

using namespace Hyprutils::Memory;
using namespace Hyprtoolkit;

struct DesktopApp {
    std::string name;
    std::string exec;
    std::string icon;
    std::string desktopFile;
    bool noDisplay = false;
    bool hidden = false;
    
    bool operator<(const DesktopApp& other) const {
        return name < other.name;
    }
};

class AppDatabase {
  public:
    AppDatabase() {
        loadApps();
    }
    
    const std::vector<DesktopApp>& getAllApps() const { return m_allApps; }
    
    std::vector<DesktopApp> filterApps(const std::string& query) const {
        if (query.empty()) {
            return m_allApps;
        }
        
        std::string lowerQuery = query;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
        
        std::vector<DesktopApp> filtered;
        for (const auto& app : m_allApps) {
            std::string lowerName = app.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            if (lowerName.find(lowerQuery) != std::string::npos) {
                filtered.push_back(app);
            }
        }
        return filtered;
    }
    
    void reload() {
        m_allApps.clear();
        loadApps();
    }
    
  private:
    void loadApps() {
        std::vector<fs::path> desktopDirs = {
            "/usr/share/applications",
            fs::path(std::getenv("HOME")) / ".local/share/applications"
        };
        
        for (const auto& dir : desktopDirs) {
            if (fs::exists(dir)) {
                loadAppsFromDirectory(dir);
            }
        }
        
        std::sort(m_allApps.begin(), m_allApps.end());
        std::cout << "Loaded " << m_allApps.size() << " applications" << std::endl;
    }
    
    void loadAppsFromDirectory(const fs::path& directory) {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.path().extension() == ".desktop") {
                parseDesktopFile(entry.path());
            }
        }
    }
    
    void parseDesktopFile(const fs::path& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return;
        
        DesktopApp app;
        app.desktopFile = filepath.string();
        
        std::string line;
        bool inDesktopEntry = false;
        
        while (std::getline(file, line)) {
            // Remove comments
            if (line.find('#') != std::string::npos) {
                line = line.substr(0, line.find('#'));
            }
            
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            if (line.empty()) continue;
            
            if (line == "[Desktop Entry]") {
                inDesktopEntry = true;
                continue;
            } else if (line[0] == '[') {
                inDesktopEntry = false;
                continue;
            }
            
            if (!inDesktopEntry) continue;
            
            size_t equalsPos = line.find('=');
            if (equalsPos == std::string::npos) continue;
            
            std::string key = line.substr(0, equalsPos);
            std::string value = line.substr(equalsPos + 1);
            
            if (key == "Name") {
                app.name = value;
            } else if (key == "Exec") {
                app.exec = value;
            } else if (key == "Icon") {
                app.icon = value;
            } else if (key == "NoDisplay") {
                app.noDisplay = (value == "true");
            } else if (key == "Hidden") {
                app.hidden = (value == "true");
            }
        }
        
        if (!app.name.empty() && !app.exec.empty() && !app.noDisplay && !app.hidden) {
            m_allApps.push_back(app);
        }
    }
    
    std::vector<DesktopApp> m_allApps;
};

class GridItem {
  public:
    GridItem(const DesktopApp& app, CSharedPointer<IBackend> backend)
        : m_app(app), m_backend(backend) {
        createUI();
    }

    CSharedPointer<IElement> getElement() const { return m_background; }
    const DesktopApp& getApp() const { return m_app; }
    
    void setActive(bool active) {
        if (m_active == active) return;
        m_active = active;
        updateColors();
        updateAppearance();
    }

    bool isActive() const { return m_active; }
    
    void launch() const {
        std::string cleanCmd = cleanExecCommand(m_app.exec);
        std::string fullCmd = cleanCmd + " &";
        std::system(fullCmd.c_str());
    }

  private:
    std::string cleanExecCommand(const std::string& exec) const {
        std::string result = exec;
        size_t pos = 0;
        while ((pos = result.find('%', pos)) != std::string::npos) {
            if (pos + 1 < result.length()) {
                result.erase(pos, 2);
            } else {
                result.erase(pos, 1);
            }
        }
        return result;
    }

    std::string findIconPath() {
        if (m_app.icon.empty()) return "";
        
        if (fs::path(m_app.icon).is_absolute()) {
            if (fs::exists(m_app.icon)) return m_app.icon;
        }
        
        fs::path desktopDir = fs::path(m_app.desktopFile).parent_path();
        fs::path localPath = desktopDir / m_app.icon;
        if (fs::exists(localPath)) return localPath.string();
        
        std::vector<std::string> extensions = {".png", ".svg", ".jpg", ".jpeg", ".xpm", ""};
        for (const auto& ext : extensions) {
            fs::path withExt = localPath.string() + ext;
            if (fs::exists(withExt)) return withExt.string();
        }
        
        std::vector<fs::path> iconDirs = {
            "/usr/share/pixmaps",
            "/usr/share/icons",
            "/usr/share/icons/hicolor/48x48/apps",
            "/usr/share/icons/hicolor/scalable/apps",
            fs::path(std::getenv("HOME")) / ".local/share/icons"
        };
        
        for (const auto& dir : iconDirs) {
            if (fs::exists(dir)) {
                for (const auto& ext : extensions) {
                    fs::path checkPath = dir / (m_app.icon + ext);
                    if (fs::exists(checkPath)) return checkPath.string();
                }
            }
        }
        
        return "";
    }

    CSharedPointer<IElement> createIconElement() {
        if (m_app.icon.empty()) return createPlaceholder();
        
        auto icons = m_backend->systemIcons();
        if (icons) {
            auto iconHandle = icons->lookupIcon(m_app.icon);
            if (iconHandle && iconHandle->exists()) {
                return CImageBuilder::begin()
                    ->icon(iconHandle)
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                        CDynamicSize::HT_SIZE_ABSOLUTE,
                                        {48.F, 48.F})) // Larger icon for grid
                    ->fitMode(eImageFitMode::IMAGE_FIT_MODE_CONTAIN)
                    ->sync(false)
                    ->commence();
            }
        }
        
        std::string iconPath = findIconPath();
        if (!iconPath.empty()) {
            return CImageBuilder::begin()
                ->path(std::string{iconPath})
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {48.F, 48.F}))
                ->fitMode(eImageFitMode::IMAGE_FIT_MODE_CONTAIN)
                ->sync(false)
                ->commence();
        }
        
        std::vector<std::string> fallbackIcons = {
            "application-x-executable", "executable", 
            "application-default-icon", "unknown"
        };
        
        if (icons) {
            for (const auto& fallback : fallbackIcons) {
                auto fallbackHandle = icons->lookupIcon(fallback);
                if (fallbackHandle && fallbackHandle->exists()) {
                    return CImageBuilder::begin()
                        ->icon(fallbackHandle)
                        ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                            CDynamicSize::HT_SIZE_ABSOLUTE,
                                            {48.F, 48.F}))
                        ->fitMode(eImageFitMode::IMAGE_FIT_MODE_CONTAIN)
                        ->sync(false)
                        ->commence();
                }
            }
        }
        
        return createPlaceholder();
    }

    CSharedPointer<IElement> createPlaceholder() {
        if (!m_backend || !m_backend->getPalette()) {
            return CRectangleBuilder::begin()
                ->color([] { return CHyprColor(0.5, 0.5, 0.5, 0.5); })
                ->rounding(8)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {48.F, 48.F}))
                ->commence();
        }
        
        auto placeholderColor = m_backend->getPalette()->m_colors.alternateBase.darken(0.2);
        return CRectangleBuilder::begin()
            ->color([placeholderColor] { return placeholderColor; })
            ->rounding(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {48.F, 48.F}))
            ->commence();
    }

    void createUI() {
        if (!m_backend) {
            std::cerr << "ERROR: Backend not initialized in GridItem!" << std::endl;
            return;
        }
        
        updateColors();
        
        // Create a vertical layout for icon + text
        m_mainLayout = CColumnLayoutBuilder::begin()
            ->gap(5)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_AUTO,
                                {100.F, 100.F})) // Fixed size for grid cell
            ->commence();
        m_mainLayout->setMargin(10);

        // Icon
        m_iconElement = createIconElement();
        m_mainLayout->addChild(m_iconElement);

        // App name - centered and possibly truncated
        m_text = CTextBuilder::begin()
            ->text(std::string{m_app.name})
            ->color([this] { return m_textColor; })
            ->align(eFontAlignment::HT_FONT_ALIGN_CENTER)
            ->clampSize({90.F, 30.F}) // Limit text width
            ->noEllipsize(true)
            ->commence();
        m_mainLayout->addChild(m_text);

        // Background for the entire grid cell
        m_background = CRectangleBuilder::begin()
            ->color([this] { return m_backgroundColor; })
            ->rounding(12)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {120.F, 120.F})) // Grid cell size
            ->commence();
        
        m_background->addChild(m_mainLayout);
    }

    void updateColors() {
        if (!m_backend || !m_backend->getPalette()) {
            m_textColor = CHyprColor(1, 1, 1, 1);
            m_backgroundColor = m_active ? CHyprColor(0.2, 0.4, 0.8, 1) : CHyprColor(0.3, 0.3, 0.3, 1);
            return;
        }
        
        auto& palette = m_backend->getPalette()->m_colors;
        m_textColor = m_active ? palette.brightText : palette.text;
        m_backgroundColor = m_active ? palette.accent : palette.base;
    }

    void updateAppearance() {
        if (!m_background || !m_text) return;
        
        if (auto builder = m_background->rebuild()) {
            builder->color([this] { return m_backgroundColor; })->commence();
        }
        if (auto builder = m_text->rebuild()) {
            builder->color([this] { return m_textColor; })->commence();
        }
        m_background->forceReposition();
    }

    DesktopApp m_app;
    bool m_active = false;
    CSharedPointer<IBackend> m_backend;
    CSharedPointer<CRectangleElement> m_background;
    CSharedPointer<CColumnLayoutElement> m_mainLayout;
    CSharedPointer<CTextElement> m_text;
    CSharedPointer<IElement> m_iconElement;
    CHyprColor m_textColor;
    CHyprColor m_backgroundColor;
};

class AppLauncher {
  public:
    AppLauncher() {
        // Initialize backend first
        m_backend = IBackend::create();
        if (!m_backend) {
            std::cerr << "ERROR: Failed to create backend!" << std::endl;
            return;
        }
        
        m_appDatabase = std::make_unique<AppDatabase>();
        if (m_appDatabase) {
            m_filteredApps = m_appDatabase->getAllApps();
        }
    }

    void run() {
        if (!m_backend) {
            std::cerr << "ERROR: Backend not initialized!" << std::endl;
            return;
        }
        
        createWindow();
        if (!m_window) {
            std::cerr << "ERROR: Failed to create window!" << std::endl;
            return;
        }
        
        createUI();
        setupEventHandlers();

        std::cout << "\n=== App Launcher Ready ===" << std::endl;
        std::cout << "Apps: " << (m_appDatabase ? m_appDatabase->getAllApps().size() : 0) << std::endl;
        std::cout << "Controls: ↑/↓/←/→ = Navigate, ↵ = Launch, ⎋ = Close" << std::endl;
        std::cout << "Type in search box to filter applications" << std::endl;
        std::cout << "Grid layout: " << m_gridCols << " columns" << std::endl;
        std::cout << "===========================\n" << std::endl;

        m_window->open();
        m_backend->enterLoop();
    }

  private:
    void createWindow() {
        if (!m_backend) return;
        
        m_window = CWindowBuilder::begin()
            ->type(HT_WINDOW_LAYER)
            ->appTitle("App Launcher")
            ->appClass("launcher")
            ->preferredSize({800, 800})
            ->anchor(1 | 2 | 4 | 8)
            ->layer(3)
            ->marginTopLeft({10, 10})
            ->marginBottomRight({10, 10})
            ->kbInteractive(1)
            ->exclusiveZone(-1)
            ->commence();
            
        if (!m_window) {
            std::cerr << "ERROR: Failed to create window!" << std::endl;
        }
    }

    void createUI() {
        if (!m_backend || !m_window) return;
        
        // Create a temporary root element if needed
        if (!m_window->m_rootElement) {
            std::cerr << "WARNING: Window has no root element, creating one" << std::endl;
            auto tempRoot = CRectangleBuilder::begin()
                ->color([] { return CHyprColor(0, 0, 0, 0); }) // Transparent
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_PERCENT,
                                    {1.0F, 1.0F}))
                ->commence();
            m_window->m_rootElement = tempRoot;
        }
        
        auto palette = m_backend->getPalette();
        if (!palette) {
            std::cerr << "WARNING: No palette available!" << std::endl;
            palette = CPalette::emptyPalette();
        }
        
        m_background = CRectangleBuilder::begin()
            ->color([palette] { return palette->m_colors.background; })
            ->rounding(12)
            ->borderColor([palette] { return palette->m_colors.accent.darken(0.2); })
            ->borderThickness(1)
            ->commence();

        m_mainLayout = CColumnLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT,
                                {1.0F, 1.0F}))
            ->commence();
        m_mainLayout->setMargin(12);

        m_textBox = CTextboxBuilder::begin()
            ->placeholder("Search applications...")
            ->defaultText("")
            ->multiline(false)
            ->onTextEdited([this](CSharedPointer<CTextboxElement>, const std::string& text) {
                filterApps(text);
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 40.F}))
            ->commence();

        m_scrollArea = CScrollAreaBuilder::begin()
            ->scrollY(true)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 500.F})) // More height for grid
            ->commence();
        m_scrollArea->setGrow(true);

        m_gridLayout = CColumnLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO,
                                {1.0F, 1.0F}))
            ->commence();

        createGridItems();
        
        m_mainLayout->addChild(m_textBox);
        m_mainLayout->addChild(m_scrollArea);
        m_scrollArea->addChild(m_gridLayout);
        m_background->addChild(m_mainLayout);
        m_window->m_rootElement->addChild(m_background);
        
        m_backend->addIdle([this] {
            if (m_textBox) {
                m_textBox->focus(true);
            }
        });
    }

    void createGridItems() {
        if (!m_gridLayout) return;
        
        m_gridItems.clear();
        m_gridLayout->clearChildren();
        
        if (m_filteredApps.empty()) {
            std::cout << "No apps to display" << std::endl;
            return;
        }
        
        std::cout << "Creating grid for " << m_filteredApps.size() << " applications..." << std::endl;
        
        // Calculate number of rows needed
        size_t numApps = m_filteredApps.size();
        size_t numRows = (numApps + m_gridCols - 1) / m_gridCols; // Ceiling division
        
        // Create grid rows
        for (size_t row = 0; row < numRows; row++) {
            auto rowLayout = CRowLayoutBuilder::begin()
                ->gap(10)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_AUTO,
                                    {1.0F, 1.0F}))
                ->commence();
            
            // Add items to this row
            for (size_t col = 0; col < m_gridCols; col++) {
                size_t index = row * m_gridCols + col;
                if (index >= m_filteredApps.size()) {
                    // Add empty space if no more apps
                    auto emptySpace = CRectangleBuilder::begin()
                        ->color([] { return CHyprColor(0, 0, 0, 0); })
                        ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                            CDynamicSize::HT_SIZE_ABSOLUTE,
                                            {120.F, 120.F}))
                        ->commence();
                    rowLayout->addChild(emptySpace);
                    continue;
                }
                
                const auto& app = m_filteredApps[index];
                auto gridItem = std::make_shared<GridItem>(app, m_backend);
                if (gridItem->getElement()) {
                    rowLayout->addChild(gridItem->getElement());
                    m_gridItems.push_back(gridItem);
                }
            }
            
            m_gridLayout->addChild(rowLayout);
        }

        if (!m_gridItems.empty()) {
            m_selectedIndex = 0;
            m_gridItems[0]->setActive(true);
            updateGridPosition(); // Calculate grid position
        }
    }

    void filterApps(const std::string& query) {
        if (!m_appDatabase) return;
        
        m_filteredApps = m_appDatabase->filterApps(query);
        createGridItems(); // Recreate grid with filtered apps
        
        if (!m_gridItems.empty()) {
            m_selectedIndex = 0;
            m_gridItems[0]->setActive(true);
            updateGridPosition();
        }
        
        std::cout << "Filter: '" << query << "' - Showing " << m_gridItems.size() 
                  << " apps" << std::endl;
    }

    void updateGridPosition() {
        m_gridRow = m_selectedIndex / m_gridCols;
        m_gridCol = m_selectedIndex % m_gridCols;
    }

    void moveSelection(int deltaRow, int deltaCol) {
        if (m_gridItems.empty()) return;
        
        m_gridItems[m_selectedIndex]->setActive(false);
        
        // Calculate new position
        int newRow = static_cast<int>(m_gridRow) + deltaRow;
        int newCol = static_cast<int>(m_gridCol) + deltaCol;
        
        // Wrap around rows if needed
        if (newRow < 0) newRow = (m_gridItems.size() + m_gridCols - 1) / m_gridCols - 1;
        else if (newRow >= static_cast<int>((m_gridItems.size() + m_gridCols - 1) / m_gridCols)) newRow = 0;
        
        // Calculate new index
        size_t newIndex = newRow * m_gridCols + newCol;
        
        // Clamp column within row bounds
        size_t itemsInRow = std::min(m_gridCols, m_gridItems.size() - newRow * m_gridCols);
        if (newCol < 0) {
            newCol = static_cast<int>(itemsInRow) - 1;
            newIndex = newRow * m_gridCols + newCol;
        } else if (newCol >= static_cast<int>(itemsInRow)) {
            newCol = 0;
            newIndex = newRow * m_gridCols;
        }
        
        // Ensure index is valid
        if (newIndex >= m_gridItems.size()) {
            newIndex = m_gridItems.size() - 1;
        }
        
        m_selectedIndex = newIndex;
        m_gridItems[m_selectedIndex]->setActive(true);
        
        updateGridPosition();
        ensureSelectionVisible();
        
        std::cout << "Selected: " << m_gridItems[m_selectedIndex]->getApp().name 
                  << " [" << m_gridRow << "," << m_gridCol << "] (" 
                  << m_selectedIndex + 1 << "/" << m_gridItems.size() << ")" << std::endl;
    }

    void ensureSelectionVisible() {
        if (m_gridItems.empty() || !m_scrollArea) return;
        
        const float ROW_HEIGHT = 140.F; // Approximate row height
        const float SCROLL_AREA_HEIGHT = m_scrollArea->size().y;
        const float CURRENT_SCROLL = m_scrollArea->getCurrentScroll().y;
        
        const float SELECTION_TOP = static_cast<float>(m_gridRow) * ROW_HEIGHT;
        const float SELECTION_BOTTOM = SELECTION_TOP + ROW_HEIGHT;
        
        if (SELECTION_TOP < CURRENT_SCROLL) {
            m_scrollArea->setScroll({0.F, SELECTION_TOP});
        } else if (SELECTION_BOTTOM > CURRENT_SCROLL + SCROLL_AREA_HEIGHT) {
            m_scrollArea->setScroll({0.F, SELECTION_BOTTOM - SCROLL_AREA_HEIGHT});
        }
    }

    void launchSelectedApp() {
        if (m_selectedIndex >= m_gridItems.size()) return;
        
        std::cout << "Launching: " << m_gridItems[m_selectedIndex]->getApp().name << std::endl;
        m_gridItems[m_selectedIndex]->launch();
        closeLauncher();
    }

    void closeLauncher() {
        std::cout << "Closing launcher" << std::endl;
        if (m_window) {
            m_window->close();
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

            switch (event.xkbKeysym) {
                case XKB_KEY_Escape:
                    closeLauncher();
                    break;
                    
                case XKB_KEY_Down:
                case XKB_KEY_j:
                case XKB_KEY_J:
                    moveSelection(+1, 0); // Move down
                    break;
                    
                case XKB_KEY_Up:
                case XKB_KEY_k:
                case XKB_KEY_K:
                    moveSelection(-1, 0); // Move up
                    break;
                    
                case XKB_KEY_Right:
                case XKB_KEY_l:
                case XKB_KEY_L:
                    moveSelection(0, +1); // Move right
                    break;
                    
                case XKB_KEY_Left:
                case XKB_KEY_h:
                case XKB_KEY_H:
                    moveSelection(0, -1); // Move left
                    break;
                    
                case XKB_KEY_Page_Down:
                    // Move down by 3 rows
                    moveSelection(+3, 0);
                    break;
                    
                case XKB_KEY_Page_Up:
                    // Move up by 3 rows
                    moveSelection(-3, 0);
                    break;
                    
                case XKB_KEY_Home:
                    if (!m_gridItems.empty()) {
                        m_gridItems[m_selectedIndex]->setActive(false);
                        m_selectedIndex = 0;
                        m_gridItems[m_selectedIndex]->setActive(true);
                        updateGridPosition();
                        ensureSelectionVisible();
                    }
                    break;
                    
                case XKB_KEY_End:
                    if (!m_gridItems.empty()) {
                        m_gridItems[m_selectedIndex]->setActive(false);
                        m_selectedIndex = m_gridItems.size() - 1;
                        m_gridItems[m_selectedIndex]->setActive(true);
                        updateGridPosition();
                        ensureSelectionVisible();
                    }
                    break;
                    
                case XKB_KEY_Return:
                case XKB_KEY_KP_Enter:
                    launchSelectedApp();
                    break;
                    
                default:
                    break;
            }
        });
    }

    CSharedPointer<IBackend> m_backend;
    CSharedPointer<IWindow> m_window;
    CSharedPointer<CRectangleElement> m_background;
    CSharedPointer<CColumnLayoutElement> m_mainLayout;
    CSharedPointer<CTextboxElement> m_textBox;
    CSharedPointer<CScrollAreaElement> m_scrollArea;
    CSharedPointer<CColumnLayoutElement> m_gridLayout;
    
    std::unique_ptr<AppDatabase> m_appDatabase;
    std::vector<DesktopApp> m_filteredApps;
    std::vector<std::shared_ptr<GridItem>> m_gridItems;
    
    // Grid properties
    const size_t m_gridCols = 4; // Number of columns in the grid
    size_t m_selectedIndex = 0;
    size_t m_gridRow = 0;
    size_t m_gridCol = 0;
    
    Hyprutils::Signal::CHyprSignalListener m_keyboardListener;
};

int main() {
    try {
        std::cout << "=== App Launcher Starting ===" << std::endl;
        AppLauncher launcher;
        launcher.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}