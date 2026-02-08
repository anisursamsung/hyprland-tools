#include "ConfigManager.hpp"
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
#include <memory>

namespace fs = std::filesystem;

using namespace Hyprutils::Memory;
using namespace Hyprtoolkit;

// ============================================
// Simplified Data Structures
// ============================================

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

enum class ViewMode {
    LIST,
    GRID
};

// ============================================
// Simplified AppDatabase
// ============================================

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

// ============================================
// Base AppItem Class
// ============================================

class BaseAppItem {
  public:
    BaseAppItem(const DesktopApp& app, CSharedPointer<IBackend> backend)
        : m_app(app), m_backend(backend) {
    }
    
    virtual ~BaseAppItem() = default;
    
    virtual CSharedPointer<IElement> getElement() const = 0;
    virtual void setActive(bool active) = 0;
    virtual bool isActive() const = 0;
    virtual void updateAppearance() = 0;
    
    const DesktopApp& getApp() const { return m_app; }
    
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

  protected:
    DesktopApp m_app;
    CSharedPointer<IBackend> m_backend;
    bool m_active = false;
    
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

    CSharedPointer<IElement> createIconElement(float size) {
        if (m_app.icon.empty()) return createPlaceholder(size);
        
        auto icons = m_backend->systemIcons();
        if (icons) {
            auto iconHandle = icons->lookupIcon(m_app.icon);
            if (iconHandle && iconHandle->exists()) {
                return CImageBuilder::begin()
                    ->icon(iconHandle)
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                        CDynamicSize::HT_SIZE_ABSOLUTE,
                                        {size, size}))
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
                                    {size, size}))
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
                                            {size, size}))
                        ->fitMode(eImageFitMode::IMAGE_FIT_MODE_CONTAIN)
                        ->sync(false)
                        ->commence();
                }
            }
        }
        
        return createPlaceholder(size);
    }

    CSharedPointer<IElement> createPlaceholder(float size) {
        // Use system palette for placeholder
        auto palette = CPalette::palette();
        if (!m_backend || !palette) {
            return CRectangleBuilder::begin()
                ->color([] { return CHyprColor(0.5, 0.5, 0.5, 0.5); })
                ->rounding(static_cast<int>(size * 0.25))
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {size, size}))
                ->commence();
        }
        
        auto placeholderColor = palette->m_colors.alternateBase.darken(0.2);
        return CRectangleBuilder::begin()
            ->color([placeholderColor] { return placeholderColor; })
            ->rounding(static_cast<int>(size * 0.25))
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {size, size}))
            ->commence();
    }
};

// ============================================
// ListAppItem - For List View
// ============================================

class ListAppItem : public BaseAppItem {
  public:
    ListAppItem(const DesktopApp& app, CSharedPointer<IBackend> backend, 
                std::function<void()> onHover = nullptr, 
                std::function<void()> onClick = nullptr)
        : BaseAppItem(app, backend), m_onHover(onHover), m_onClick(onClick) {
        createUI();
    }
    
    CSharedPointer<IElement> getElement() const override { return m_background; }
    
    void setActive(bool active) override {
        if (m_active == active) return;
        m_active = active;
        updateAppearance();
    }
    
    bool isActive() const override { return m_active; }
    
    void updateAppearance() override {
        if (!m_background || !m_text) return;
        
        // Use system palette for updates
        auto palette = CPalette::palette();
        
        if (auto builder = m_background->rebuild()) {
            if (palette) {
                auto& colors = palette->m_colors;
                builder->color([this, colors] { 
                    return m_active ? colors.accent.mix(colors.base, 0.3) 
                                   : CHyprColor(0, 0, 0, 0);
                })->commence();
            } else {
                builder->color([this] { 
                    return m_active ? CHyprColor(0.2, 0.4, 0.8, 0.8) 
                                   : CHyprColor(0, 0, 0, 0);
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
    
  private:
    void createUI() {
        auto palette = CPalette::palette();
        
        m_background = CRectangleBuilder::begin()
            ->color([palette] { 
                if (!palette) return CHyprColor(0, 0, 0, 0);
                return CHyprColor(0, 0, 0, 0); // Transparent by default
            })
            ->rounding(palette ? palette->m_vars.smallRounding : 6)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 50.F}))
            ->commence();

        m_rowLayout = CRowLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT,
                                {1.0F, 1.0F}))
            ->commence();
        m_rowLayout->setMargin(8);

        m_iconElement = createIconElement(32.0F);
        m_rowLayout->addChild(m_iconElement);

        m_text = CTextBuilder::begin()
            ->text(std::string{m_app.name})
            ->color([palette] { 
                if (!palette) return CHyprColor(0.8, 0.8, 0.8, 1);
                return palette->m_colors.text;
            })
            ->fontFamily(palette ? palette->m_vars.fontFamily : "Sans Serif")
            ->commence();
        m_rowLayout->addChild(m_text);

        m_background->addChild(m_rowLayout);
        
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
            // We'll handle leaving in the parent
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
    
    CSharedPointer<CRectangleElement> m_background;
    CSharedPointer<CRowLayoutElement> m_rowLayout;
    CSharedPointer<CTextElement> m_text;
    CSharedPointer<IElement> m_iconElement;
    
    std::function<void()> m_onHover;
    std::function<void()> m_onClick;
};

// ============================================
// GridAppItem - For Grid View
// ============================================

class GridAppItem : public BaseAppItem {
  public:
    GridAppItem(const DesktopApp& app, CSharedPointer<IBackend> backend,
                std::function<void()> onHover = nullptr,
                std::function<void()> onClick = nullptr)
        : BaseAppItem(app, backend), m_onHover(onHover), m_onClick(onClick) {
        createUI();
    }
    
    CSharedPointer<IElement> getElement() const override { return m_background; }
    
    void setActive(bool active) override {
        if (m_active == active) return;
        m_active = active;
        updateAppearance();
    }
    
    bool isActive() const override { return m_active; }
    
    void updateAppearance() override {
        if (!m_background || !m_text) return;
        
        // Use system palette for updates
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
    
  private:
    void createUI() {
        auto palette = CPalette::palette();
        
        m_background = CRectangleBuilder::begin()
            ->color([palette] { 
                if (!palette) return CHyprColor(0.2, 0.2, 0.2, 0.3);
                return palette->m_colors.alternateBase;
            })
            ->rounding(palette ? palette->m_vars.smallRounding : 12)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {120.F, 120.F}))
            ->commence();
        
        // Main column layout
        m_columnLayout = CColumnLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT,
                                {1.0F, 1.0F}))
            ->commence();
        
        // Top spacer (fixed height)
        auto topSpacer = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 20.F}))
            ->commence();
        m_columnLayout->addChild(topSpacer);
        
        // Icon container - simpler approach
        auto iconContainer = CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 48.F}))
            ->commence();
        
        // Fixed-width left spacer (36px = (120-48)/2)
        auto leftSpacer = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {36.F, 48.F}))
            ->commence();
        iconContainer->addChild(leftSpacer);
        
        // Icon
        m_iconElement = createIconElement(48.0F);
        iconContainer->addChild(m_iconElement);
        
        // Fixed-width right spacer
        auto rightSpacer = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {36.F, 48.F}))
            ->commence();
        iconContainer->addChild(rightSpacer);
        
        m_columnLayout->addChild(iconContainer);
        
        // Small gap between icon and text
        auto iconTextGap = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 5.F}))
            ->commence();
        m_columnLayout->addChild(iconTextGap);
        
        // Text with ellipsis
        m_text = CTextBuilder::begin()
            ->text(std::string{m_app.name})
            ->color([palette] { 
                if (!palette) return CHyprColor(0.8, 0.8, 0.8, 1);
                return palette->m_colors.text;
            })
            ->align(eFontAlignment::HT_FONT_ALIGN_CENTER)
            
            ->fontFamily(palette ? palette->m_vars.fontFamily : "Sans Serif")
            ->clampSize({110.F, 30.F})
            ->noEllipsize(false)
            ->commence();
        
        // Text container
        auto textContainer = CColumnLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 40.F}))
            ->commence();
        
        // Top spacer in text area
        auto textTopSpacer = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 5.F}))
            ->commence();
        textContainer->addChild(textTopSpacer);
        
        textContainer->addChild(m_text);
        
        // Bottom spacer in text area
        auto textBottomSpacer = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 5.F}))
            ->commence();
        textContainer->addChild(textBottomSpacer);
        
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
            // We'll handle leaving in the parent
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
    
    CSharedPointer<CRectangleElement> m_background;
    CSharedPointer<CColumnLayoutElement> m_columnLayout;
    CSharedPointer<CTextElement> m_text;
    CSharedPointer<IElement> m_iconElement;
    
    std::function<void()> m_onHover;
    std::function<void()> m_onClick;
};

// ============================================
// Main AppLauncher Class - Simplified
// ============================================

class AppLauncher {
  public:
    AppLauncher() {
        // Load configuration first
        m_config = std::make_unique<ConfigManager>();
        
        m_backend = IBackend::create();
        if (!m_backend) {
            throw std::runtime_error("Failed to create backend");
        }
        
        m_appDatabase = std::make_unique<AppDatabase>();
        m_filteredApps = m_appDatabase->getAllApps();
        
        // Set initial view mode based on config
        std::string defaultView = m_config->getDefaultView();
        m_viewMode = (defaultView == "grid") ? ViewMode::GRID : ViewMode::LIST;
        
        std::cout << "Configuration loaded:\n";
        std::cout << "  Default view: " << defaultView << "\n";
        std::cout << "  Grid columns: " << m_config->getColumnCount() << "\n";
        std::cout << "  Grid item size: " << m_config->getGridItemWidth() 
                  << "x" << m_config->getGridItemHeight() << "\n";
    }
    
    void run() {
        createWindow();
        if (!m_window) {
            throw std::runtime_error("Failed to create window");
        }
        
        createUI();
        setupEventHandlers();

        std::cout << "\n=== App Launcher Ready ===" << std::endl;
        std::cout << "Apps: " << m_filteredApps.size() << std::endl;
        std::cout << "Controls: ↑/↓/←/→ = Navigate, ↵ = Launch, ⎋ = Close" << std::endl;
        std::cout << "Ctrl+Esc: Switch between list/grid view" << std::endl;
        std::cout << "Mouse: Hover to select, Click to launch" << std::endl;
        std::cout << "Type to search applications" << std::endl;
        std::cout << "===========================\n" << std::endl;

        m_window->open();
        m_backend->enterLoop();
    }
    
  private:
       void createWindow() {
        Hyprutils::Math::Vector2D preferredSize;
        
        if (m_viewMode == ViewMode::GRID) {
            // Calculate window width based on grid configuration
            float totalGridWidth = (m_config->getGridItemWidth() * m_config->getColumnCount()) + 
                                  (m_config->getGridHorizontalGap() * (m_config->getColumnCount() - 1));
            
            // Add margins (12px on each side from mainLayout margin)
            float windowWidth = totalGridWidth + 24;
            
            // Set reasonable height - use explicit double values to avoid ambiguity
            preferredSize = Hyprutils::Math::Vector2D(static_cast<double>(windowWidth), 600.0);
            
            std::cout << "Grid view: calculated window width = " << windowWidth << "px\n";
        } else {
            // List view: fixed width - use explicit double values
            preferredSize = Hyprutils::Math::Vector2D(800.0, 600.0);
        }
        
        m_window = CWindowBuilder::begin()
            ->type(HT_WINDOW_LAYER)
            ->appTitle("App Launcher")
            ->appClass("launcher")
            ->preferredSize(preferredSize)
            ->anchor(1 | 2 | 4 | 8)  // All anchors
            ->layer(3)
            ->marginTopLeft({100, 100})
            ->marginBottomRight({100, 100})
            ->kbInteractive(1)
            ->exclusiveZone(-1)
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
        
        // Use the system palette from Hyprtoolkit
        auto palette = CPalette::palette();
        if (!palette) {
            palette = CPalette::emptyPalette();
        }
        
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

        m_mainLayout = CColumnLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT,
                                {1.0F, 1.0F}))
            ->commence();
        m_mainLayout->setMargin(12);

        // Create search box with custom text handling
        m_searchBox = CTextboxBuilder::begin()
            ->placeholder("Search applications...")
            ->defaultText("")
            ->multiline(false)
            ->onTextEdited([this](CSharedPointer<CTextboxElement> textbox, const std::string& text) {
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
                                {1.0F, 500.F}))
            ->commence();
        m_scrollArea->setGrow(true);

        updateView();
        
        m_mainLayout->addChild(m_searchBox);
        m_mainLayout->addChild(m_scrollArea);
        m_background->addChild(m_mainLayout);
        root->addChild(m_background);
        
        // Focus search box after UI is created
        m_backend->addIdle([this] {
            if (m_searchBox) {
                m_searchBox->focus(true);
            }
        });
    }
    
    void updateView() {
        if (!m_scrollArea) return;
        
        m_scrollArea->clearChildren();
        m_appItems.clear();
        
        if (m_filteredApps.empty()) {
            auto message = CTextBuilder::begin()
                ->text("No applications found" + (m_currentQuery.empty() ? "" : " matching \"" + m_currentQuery + "\""))
                ->color([] { return CHyprColor(0.7, 0.7, 0.7, 1); })
                ->align(eFontAlignment::HT_FONT_ALIGN_CENTER)
                ->commence();
            m_scrollArea->addChild(message);
            return;
        }
        
        if (m_viewMode == ViewMode::LIST) {
            createListView();
        } else {
            createGridView();
        }
        
        if (!m_appItems.empty()) {
            m_selectedIndex = 0;
            m_appItems[0]->setActive(true);
            
            if (m_viewMode == ViewMode::GRID) {
                updateGridPosition();
            }
        }
    }
    
    void createListView() {
        auto listLayout = CColumnLayoutBuilder::begin()
            ->gap(2)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO,
                                {1.0F, 1.0F}))
            ->commence();
        
        for (size_t i = 0; i < m_filteredApps.size(); ++i) {
            const auto& app = m_filteredApps[i];
            
            // Create callbacks for this specific item
            auto onHover = [this, i]() {
                selectItem(i);
            };
            
            auto onClick = [this, i]() {
                selectItem(i);
                launchSelectedApp();
            };
            
            auto appItem = std::make_shared<ListAppItem>(app, m_backend, onHover, onClick);
            if (appItem->getElement()) {
                listLayout->addChild(appItem->getElement());
                m_appItems.push_back(appItem);
            }
        }
        
        m_scrollArea->addChild(listLayout);
    }
    
    void createGridView() {
        // Use config values
        const int COLUMN_COUNT = m_config->getColumnCount();
        const float ITEM_WIDTH = static_cast<float>(m_config->getGridItemWidth());
        const float ITEM_HEIGHT = static_cast<float>(m_config->getGridItemHeight());
        const float ROW_GAP = static_cast<float>(m_config->getGridVerticalGap());
        const float COLUMN_GAP = static_cast<float>(m_config->getGridHorizontalGap());
        
        size_t numApps = m_filteredApps.size();
        size_t numRows = (numApps + COLUMN_COUNT - 1) / COLUMN_COUNT;
        
        // Calculate total grid dimensions
        float totalGridWidth = (ITEM_WIDTH * COLUMN_COUNT) + 
                              (COLUMN_GAP * (COLUMN_COUNT - 1));
        float totalGridHeight = (ITEM_HEIGHT * numRows) + 
                               (ROW_GAP * (numRows - 1));
        
        // Create a container for the grid
        auto gridContainer = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {totalGridWidth, totalGridHeight}))
            ->commence();
        
        // Create the main grid layout
        auto gridLayout = CColumnLayoutBuilder::begin()
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
                
                if (index >= numApps) {
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
                
                const auto& app = m_filteredApps[index];
                size_t itemIndex = m_appItems.size(); // Current size before adding
                
                // Create callbacks for this specific item
                auto onHover = [this, itemIndex]() {
                    selectItem(itemIndex);
                };
                
                auto onClick = [this, itemIndex]() {
                    selectItem(itemIndex);
                    launchSelectedApp();
                };
                
                auto appItem = std::make_shared<GridAppItem>(app, m_backend, onHover, onClick);
                if (appItem->getElement()) {
                    rowLayout->addChild(appItem->getElement());
                    m_appItems.push_back(appItem);
                }
            }
            
            gridLayout->addChild(rowLayout);
        }
        
        gridContainer->addChild(gridLayout);
        m_scrollArea->addChild(gridContainer);
    }
    
    void selectItem(size_t index) {
        if (index >= m_appItems.size()) return;
        
        // Deselect current item
        if (m_selectedIndex < m_appItems.size()) {
            m_appItems[m_selectedIndex]->setActive(false);
        }
        
        // Select new item
        m_selectedIndex = index;
        m_appItems[m_selectedIndex]->setActive(true);
        
        // Update grid position if in grid view
        if (m_viewMode == ViewMode::GRID) {
            updateGridPosition();
        }
        
        ensureSelectionVisible();
    }
    
    void filterApps(const std::string& query) {
        m_currentQuery = query;
        m_filteredApps = m_appDatabase->filterApps(query);
        updateView();
    }
    
    void updateGridPosition() {
        if (m_viewMode != ViewMode::GRID || m_appItems.empty()) return;
        
        const int COLUMN_COUNT = m_config->getColumnCount();
        m_gridRow = m_selectedIndex / COLUMN_COUNT;
        m_gridCol = m_selectedIndex % COLUMN_COUNT;
    }
    
    void moveGridSelection(int deltaRow, int deltaCol) {
        if (m_viewMode != ViewMode::GRID || m_appItems.empty()) return;
        
        m_appItems[m_selectedIndex]->setActive(false);
        
        const int COLUMN_COUNT = m_config->getColumnCount();
        
        int newRow = static_cast<int>(m_gridRow) + deltaRow;
        int newCol = static_cast<int>(m_gridCol) + deltaCol;
        
        // Wrap around rows
        int totalRows = (m_appItems.size() + COLUMN_COUNT - 1) / COLUMN_COUNT;
        if (newRow < 0) newRow = totalRows - 1;
        else if (newRow >= totalRows) newRow = 0;
        
        // Wrap columns within row
        size_t itemsInRow = std::min(static_cast<size_t>(COLUMN_COUNT), 
                                   m_appItems.size() - newRow * COLUMN_COUNT);
        if (newCol < 0) {
            newCol = itemsInRow - 1;
            newRow--;
            if (newRow < 0) newRow = totalRows - 1;
        } else if (newCol >= static_cast<int>(itemsInRow)) {
            newCol = 0;
            newRow++;
            if (newRow >= totalRows) newRow = 0;
        }
        
        size_t newIndex = newRow * COLUMN_COUNT + newCol;
        if (newIndex >= m_appItems.size()) {
            newIndex = m_appItems.size() - 1;
        }
        
        m_selectedIndex = newIndex;
        m_gridRow = newRow;
        m_gridCol = newCol;
        
        m_appItems[m_selectedIndex]->setActive(true);
        ensureSelectionVisible();
    }
    
    void moveSelection(int delta) {
        if (m_appItems.empty()) return;
        
        m_appItems[m_selectedIndex]->setActive(false);
        
        if (m_viewMode == ViewMode::LIST) {
            // List navigation: simple up/down
            int newIndex = static_cast<int>(m_selectedIndex) + delta;
            if (newIndex < 0) newIndex = m_appItems.size() - 1;
            else if (newIndex >= static_cast<int>(m_appItems.size())) newIndex = 0;
            
            m_selectedIndex = newIndex;
        } else {
            // Grid navigation with configurable columns
            const int COLUMN_COUNT = m_config->getColumnCount();
            
            int currentRow = static_cast<int>(m_gridRow);
            int currentCol = static_cast<int>(m_gridCol);
            
            int newRow = currentRow + delta;
            int totalRows = (m_appItems.size() + COLUMN_COUNT - 1) / COLUMN_COUNT;
            
            // Wrap rows
            if (newRow < 0) newRow = totalRows - 1;
            else if (newRow >= totalRows) newRow = 0;
            
            // Calculate new index
            size_t newIndex = newRow * COLUMN_COUNT + currentCol;
            
            // Check if this position exists in the new row
            size_t itemsInNewRow = std::min(static_cast<size_t>(COLUMN_COUNT), 
                                          m_appItems.size() - newRow * COLUMN_COUNT);
            
            // If column position doesn't exist in new row, adjust to last column in that row
            if (currentCol >= static_cast<int>(itemsInNewRow)) {
                currentCol = itemsInNewRow - 1;
                newIndex = newRow * COLUMN_COUNT + currentCol;
            }
            
            // Ensure index is valid
            if (newIndex >= m_appItems.size()) {
                newIndex = m_appItems.size() - 1;
            }
            
            m_selectedIndex = newIndex;
            m_gridRow = newRow;
            m_gridCol = currentCol;
        }
        
        m_appItems[m_selectedIndex]->setActive(true);
        ensureSelectionVisible();
    }
    
    void ensureSelectionVisible() {
        if (m_appItems.empty() || !m_scrollArea) return;
        
        if (m_viewMode == ViewMode::LIST) {
            const float ITEM_HEIGHT = 52.F;
            const float SCROLL_AREA_HEIGHT = m_scrollArea->size().y;
            const float CURRENT_SCROLL = m_scrollArea->getCurrentScroll().y;
            
            const float SELECTION_TOP = static_cast<float>(m_selectedIndex) * ITEM_HEIGHT;
            const float SELECTION_BOTTOM = SELECTION_TOP + ITEM_HEIGHT;
            
            if (SELECTION_TOP < CURRENT_SCROLL) {
                m_scrollArea->setScroll({0.F, SELECTION_TOP});
            } else if (SELECTION_BOTTOM > CURRENT_SCROLL + SCROLL_AREA_HEIGHT) {
                m_scrollArea->setScroll({0.F, SELECTION_BOTTOM - SCROLL_AREA_HEIGHT});
            }
        } else {
            // Grid view
            const float ITEM_HEIGHT = static_cast<float>(m_config->getGridItemHeight());
            const float ROW_GAP = static_cast<float>(m_config->getGridVerticalGap());
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
    }
    
    void launchSelectedApp() {
        if (m_selectedIndex >= m_appItems.size()) return;
        
        std::cout << "Launching: " << m_appItems[m_selectedIndex]->getApp().name << std::endl;
        m_appItems[m_selectedIndex]->launch();
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
                    if (event.modMask & Input::HT_MODIFIER_CTRL) {
                        // Ctrl+Esc: Switch view mode
                        toggleViewMode();
                        // Refocus search box
                        m_backend->addIdle([this] {
                            if (m_searchBox) {
                                m_searchBox->focus(true);
                            }
                        });
                    } else {
                        // Esc alone: Close launcher
                        closeLauncher();
                    }
                    break;
                    
                case XKB_KEY_Down:
                    moveSelection(+1);
                    break;
                    
                case XKB_KEY_Up:
                    moveSelection(-1);
                    break;
                    
                case XKB_KEY_Right:
                    if (m_viewMode == ViewMode::GRID) {
                        moveGridSelection(0, +1);
                    }
                    break;
                    
                case XKB_KEY_Left:
                    if (m_viewMode == ViewMode::GRID) {
                        moveGridSelection(0, -1);
                    }
                    break;
                    
                case XKB_KEY_Return:
                case XKB_KEY_KP_Enter:
                    launchSelectedApp();
                    break;
                    
                default:
                    // Let the textbox handle text input
                    break;
            }
        });
    }
    
    void toggleViewMode() {
        m_viewMode = (m_viewMode == ViewMode::LIST) ? ViewMode::GRID : ViewMode::LIST;
        updateView();
        std::cout << "Switched to " << (m_viewMode == ViewMode::LIST ? "list" : "grid") << " view" << std::endl;
        
        // Refocus the search box after view change
        m_backend->addIdle([this] {
            if (m_searchBox) {
                m_searchBox->focus(true);
            }
        });
    }
    
    // Member variables
    std::unique_ptr<ConfigManager> m_config;
    CSharedPointer<IBackend> m_backend;
    CSharedPointer<IWindow> m_window;
    
    CSharedPointer<CRectangleElement> m_background;
    CSharedPointer<CColumnLayoutElement> m_mainLayout;
    CSharedPointer<CTextboxElement> m_searchBox;
    CSharedPointer<CScrollAreaElement> m_scrollArea;
    
    std::unique_ptr<AppDatabase> m_appDatabase;
    std::vector<DesktopApp> m_filteredApps;
    std::vector<std::shared_ptr<BaseAppItem>> m_appItems;
    
    ViewMode m_viewMode = ViewMode::LIST;
    size_t m_selectedIndex = 0;
    size_t m_gridRow = 0;
    size_t m_gridCol = 0;
    std::string m_currentQuery;
    
    Hyprutils::Signal::CHyprSignalListener m_keyboardListener;
};

int main() {
    try {
        std::cout << "=== App Launcher Starting ===" << std::endl;
        std::setlocale(LC_ALL, "");
        
        AppLauncher launcher;
        launcher.run();
        
        std::cout << "=== App Launcher Exited ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}