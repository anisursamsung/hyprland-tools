#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Checkbox.hpp>
#include <hyprtoolkit/element/Combobox.hpp>
#include <hyprtoolkit/element/Button.hpp>  // ADD THIS
#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/signal/Signal.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <iomanip>  // ADD THIS
#include <xkbcommon/xkbcommon-keysyms.h>
#include "AppDatabase.hpp"
#include "AppItem.hpp"

using namespace Hyprutils::Memory;
using namespace Hyprtoolkit;


class AppLauncher {
public:
    AppLauncher() {
        // Check if we're in a Wayland session
        if (!std::getenv("WAYLAND_DISPLAY") && !std::getenv("DISPLAY")) {
            std::cerr << "ERROR: Not running in a graphical session!\n";
            m_valid = false;
            return;
        }
        
        // Initialize backend
        m_backend = IBackend::create();
        if (!m_backend) {
            std::cerr << "ERROR: Failed to create Hyprtoolkit backend!\n";
            m_valid = false;
            return;
        }
        
        // Set up logging
        m_backend->setLogFn([](eLogLevel level, const std::string& msg) {
            const char* levelStr = "UNKNOWN";
            switch (level) {
                case HT_LOG_TRACE: levelStr = "TRACE"; break;
                case HT_LOG_DEBUG: levelStr = "DEBUG"; break;
                case HT_LOG_WARNING: levelStr = "WARN"; break;
                case HT_LOG_ERROR: levelStr = "ERROR"; break;
                case HT_LOG_CRITICAL: levelStr = "CRITICAL"; break;
            }
            std::cout << "[Hyprtoolkit " << levelStr << "] " << msg << std::endl;
        });
        
        // Initialize database
        m_appDatabase = std::make_unique<AppDatabase>();
        IconCache::instance().preloadCommonIcons();
        
        m_valid = true;
    }
    
    ~AppLauncher() {
        if (m_backend) {
            m_backend->destroy();
        }
    }
    
    bool isValid() const { return m_valid; }
    
    void run() {
        if (!m_valid) {
            std::cerr << "ERROR: Launcher not properly initialized!\n";
            return;
        }
        
        createWindow();
        if (!m_window) {
            std::cerr << "ERROR: Failed to create window!\n";
            return;
        }
        
        createUI();
        setupEventHandlers();
        
        printWelcomeMessage();
        
        m_window->open();
        m_backend->enterLoop();
    }
    
    void printWelcomeMessage() const {
        std::cout << "\n╔══════════════════════════════════════════════════════╗\n"
                  << "║                App Launcher v2.0                     ║\n"
                  << "╠══════════════════════════════════════════════════════╣\n"
                  << "║ Apps loaded: " << std::setw(36) 
                  << std::left << std::to_string(m_appDatabase->count()) + " " << "║\n"
                  << "║                                                        ║\n"
                  << "║ Controls:                                             ║\n"
                  << "║   ↑/↓/j/k      Navigate apps                         ║\n"
                  << "║   Page Up/Dn   Jump 10 items                         ║\n"
                  << "║   Home/End     First/Last app                        ║\n"
                  << "║   ↵/Enter      Launch selected app                   ║\n"
                  << "║   ⎋/Escape     Close launcher                        ║\n"
                  << "║   Ctrl+R       Reload app database                   ║\n"
                  << "║   Ctrl+Q       Quit                                  ║\n"
                  << "║   /            Focus search box                      ║\n"
                  << "║                                                        ║\n"
                  << "║ Type in search box to filter applications             ║\n"
                  << "║ Use category dropdown to filter by type               ║\n"
                  << "╚══════════════════════════════════════════════════════╝\n\n";
    }

private:
    void createWindow() {
        // Wait for app database to load
        m_appDatabase->waitForLoad();
        
        m_window = CWindowBuilder::begin()
            ->type(HT_WINDOW_LAYER)
            ->appTitle("App Launcher")
            ->appClass("app-launcher")
            ->preferredSize({600, 700})
            ->anchor(1 | 2 | 4 | 8) // All edges
            ->layer(3) // Top layer
            ->marginTopLeft({50, 50})
            ->marginBottomRight({50, 50})
            ->kbInteractive(1)
            ->exclusiveZone(-1) // Take full space
            ->commence();
            
        if (!m_window) {
            std::cerr << "ERROR: Window creation failed!\n";
            m_valid = false;
        }
    }

    void createUI() {
        if (!m_backend || !m_window) return;
        
        auto palette = m_backend->getPalette();
        if (!palette) {
            palette = CPalette::emptyPalette();
        }
        
        // Create background
        m_background = CRectangleBuilder::begin()
            ->color([palette] { return palette->m_colors.background; })
            ->rounding(palette->m_vars.bigRounding)
            ->borderColor([palette] { 
                return palette->m_colors.accent.darken(0.3); 
            })
            ->borderThickness(2)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT,
                                {1.0F, 1.0F}))
            ->commence();

        // Main layout
        m_mainLayout = CColumnLayoutBuilder::begin()
            ->gap(8)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT,
                                {1.0F, 1.0F}))
            ->commence();
        m_mainLayout->setMargin(16);

        // Header with title
        createHeader();
        
        // Search box
        createSearchBox();
        
        // Filter row (category dropdown + reload button)
        createFilterRow();
        
        // Apps list
        createAppList();
        
        // Footer with shortcuts help
        createFooter();
        
        // Assemble everything
        m_background->addChild(m_mainLayout);
        
        // Create a transparent root if needed
        if (!m_window->m_rootElement) {
            auto root = CRectangleBuilder::begin()
                ->color([] { return CHyprColor(0, 0, 0, 0); })
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_PERCENT,
                                    {1.0F, 1.0F}))
                ->commence();
            root->addChild(m_background);
            m_window->m_rootElement = root;
        } else {
            m_window->m_rootElement->addChild(m_background);
        }
        
        // Focus search box
        m_backend->addIdle([this] {
            if (m_searchBox) {
                m_searchBox->focus(true);
            }
        });
    }
    
    void createHeader() {
        auto palette = m_backend->getPalette();
        
        auto headerRow = CRowLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 40.F}))
            ->commence();
        
        // Title
        auto title = CTextBuilder::begin()
            ->text("App Launcher")
            ->fontSize(CFontSize(CFontSize::HT_FONT_H1, 1.0))
            ->color([palette] { return palette->m_colors.accent; })
            ->commence();
        
        // App count
        m_appCountText = CTextBuilder::begin()
            ->text("(" + std::to_string(m_appDatabase->count()) + " apps)")
            ->fontSize(CFontSize(CFontSize::HT_FONT_SMALL, 0.9))
            ->color([palette] { 
                return palette->m_colors.text.mix(
                    palette->m_colors.background, 0.6); 
            })
            ->commence();
        
        headerRow->addChild(title);
        headerRow->addChild(m_appCountText);
        
        m_mainLayout->addChild(headerRow);
    }
    
    void createSearchBox() {
        m_searchBox = CTextboxBuilder::begin()
            ->placeholder("Search applications by name or description...")
            ->defaultText("")
            ->multiline(false)
            ->onTextEdited([this](CSharedPointer<CTextboxElement>, 
                                 const std::string& text) {
                filterApps(text);
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 40.F}))
            ->commence();
        
        m_mainLayout->addChild(m_searchBox);
    }
    
  private:
    void createFilterRow() {
        auto palette = m_backend->getPalette();
        
        auto filterRow = CRowLayoutBuilder::begin()
            ->gap(10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 40.F}))
            ->commence();
        
        // Category dropdown
        auto categories = m_appDatabase->getAllCategories();
        std::vector<std::string> categoryItems = {"All Categories"};
        categoryItems.insert(categoryItems.end(), 
                           categories.begin(), categories.end());
        
        m_categoryDropdown = CComboboxBuilder::begin()
            ->items(std::move(categoryItems))
            ->currentItem(0)
            ->onChanged([this](CSharedPointer<CComboboxElement>, size_t idx) {
                m_selectedCategory = (idx == 0) ? "" : 
                    m_appDatabase->getAllCategories()[idx - 1];
                filterApps(m_currentSearch);
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {200.F, 40.F}))
            ->commence();
        
        // Reload button - FIXED TYPE
        m_reloadButton = CButtonBuilder::begin()
            ->label("Reload")
            ->onMainClick([this](CSharedPointer<CButtonElement>) {
                reloadApps();
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {100.F, 40.F}))
            ->commence();
        
        filterRow->addChild(m_categoryDropdown);
        filterRow->addChild(m_reloadButton);
        
        m_mainLayout->addChild(filterRow);
    }
    void createAppList() {
        m_scrollArea = CScrollAreaBuilder::begin()
            ->scrollY(true)
            ->scrollX(false)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 450.F}))
            ->commence();
        
        m_appList = CColumnLayoutBuilder::begin()
            ->gap(2)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_AUTO,
                                {1.0F, 1.0F}))
            ->commence();
        
        m_scrollArea->addChild(m_appList);
        m_mainLayout->addChild(m_scrollArea);
        
        // Initial app loading
        filterApps("");
    }
    
    void createFooter() {
        auto palette = m_backend->getPalette();
        
        auto footer = CTextBuilder::begin()
            ->text("↑/↓ Navigate | ↵ Launch | ⎋ Close | / Search | Ctrl+R Reload")
            ->fontSize(CFontSize(CFontSize::HT_FONT_SMALL, 0.8))
            ->color([palette] { 
                return palette->m_colors.text.mix(
                    palette->m_colors.background, 0.5); 
            })
            ->align(eFontAlignment::HT_FONT_ALIGN_CENTER)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, 20.F}))
            ->commence();
        
        m_mainLayout->addChild(footer);
    }
    
    void filterApps(const std::string& query) {
        m_currentSearch = query;
        
        m_filteredApps = m_appDatabase->filterApps(query, m_selectedCategory);
        createAppItems();
        
        // Update app count
        if (m_appCountText) {
            std::string countText = "(" + 
                std::to_string(m_filteredApps.size()) + 
                " of " + std::to_string(m_appDatabase->count()) + 
                " apps)";
            
            if (auto builder = m_appCountText->rebuild()) {
                builder->text(std::move(countText))->commence();
            }
        }
        
        std::cout << "Filter: '" << query << "' Category: '" 
                  << (m_selectedCategory.empty() ? "All" : m_selectedCategory)
                  << "' - Showing " << m_filteredApps.size() 
                  << " apps" << std::endl;
    }
    
    void createAppItems() {
        if (!m_appList) return;
        
        // Clear existing
        m_appItems.clear();
        m_appList->clearChildren();
        
        if (m_filteredApps.empty()) {
            // Show "no results" message
            auto palette = m_backend->getPalette();
            auto noResults = CTextBuilder::begin()
                ->text("No applications found")
                ->fontSize(CFontSize(CFontSize::HT_FONT_H3, 1.0))
                ->color([palette] { 
                    return palette->m_colors.text.mix(
                        palette->m_colors.background, 0.7); 
                })
                ->align(eFontAlignment::HT_FONT_ALIGN_CENTER)
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {1.0F, 100.F}))
                ->commence();
            m_appList->addChild(noResults);
            return;
        }
        
        // Create items
        for (const auto& app : m_filteredApps) {
            auto appItem = std::make_shared<AppItem>(app, m_backend);
            if (appItem->getElement()) {
                m_appList->addChild(appItem->getElement());
                m_appItems.push_back(appItem);
            }
        }

        // Select first item
        if (!m_appItems.empty()) {
            m_selectedIndex = 0;
            m_appItems[0]->setActive(true);
        } else {
            m_selectedIndex = 0;
        }
    }
    
    void reloadApps() {
        std::cout << "Reloading app database..." << std::endl;
        
        IconCache::instance().clear();
        m_appDatabase->reload();
        m_appDatabase->waitForLoad();
        
        filterApps(m_currentSearch);
        
        std::cout << "Reload complete. Found " 
                  << m_appDatabase->count() << " applications." << std::endl;
    }
    
    void updateSelection(int delta) {
        if (m_appItems.empty()) return;
        
        m_appItems[m_selectedIndex]->setActive(false);
        
        int newIndex = static_cast<int>(m_selectedIndex) + delta;
        if (newIndex < 0) newIndex = static_cast<int>(m_appItems.size()) - 1;
        if (newIndex >= static_cast<int>(m_appItems.size())) newIndex = 0;
        
        m_selectedIndex = static_cast<size_t>(newIndex);
        m_appItems[m_selectedIndex]->setActive(true);
        
        ensureSelectionVisible();
        
        std::cout << "Selected: " << m_appItems[m_selectedIndex]->getApp().name 
                  << " (" << m_selectedIndex + 1 << "/" << m_appItems.size() << ")" 
                  << std::endl;
    }
    
    void ensureSelectionVisible() {
        if (m_appItems.empty() || !m_scrollArea) return;
        
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
    }
    
    void launchSelectedApp() {
        if (m_selectedIndex >= m_appItems.size()) return;
        
        const auto& app = m_appItems[m_selectedIndex]->getApp();
        std::cout << "\n═══════════════════════════════════════════════════\n"
                  << "Launching: " << app.name << "\n"
                  << "Command: " << app.cleanExecCommand() << "\n"
                  << "═══════════════════════════════════════════════════\n";
        
        m_appItems[m_selectedIndex]->launch();
        closeLauncher();
    }
    
    void closeLauncher() {
        std::cout << "Closing launcher..." << std::endl;
        if (m_window) {
            m_window->close();
        }
    }
    
    void focusSearchBox() {
        if (m_searchBox) {
            m_searchBox->focus(true);
        }
    }
    
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   
   

    void setupEventHandlers() {
        if (!m_window) return;
        
        // Window close events - FIXED: Store the listeners
        m_layerClosedListener = m_window->m_events.layerClosed.listen([this] { 
            if (m_backend) m_backend->destroy(); 
        });
        
        m_closeRequestListener = m_window->m_events.closeRequest.listen([this] { 
            if (m_backend) m_backend->destroy(); 
        });

        // Keyboard shortcuts - FIXED: Remove shift variable and fix slash key
        m_keyboardListener = m_window->m_events.keyboardKey.listen(
            [this](const Input::SKeyboardKeyEvent& event) {
            if (!event.down) return;

            uint32_t modMask = event.modMask;
            bool ctrl = modMask & Input::HT_MODIFIER_CTRL;

            switch (event.xkbKeysym) {
                case XKB_KEY_Escape:
                    closeLauncher();
                    break;
                    
                case XKB_KEY_Down:
                case XKB_KEY_j:
                case XKB_KEY_J:
                    updateSelection(+1);
                    break;
                    
                case XKB_KEY_Up:
                case XKB_KEY_k:
                case XKB_KEY_K:
                    updateSelection(-1);
                    break;
                    
                case XKB_KEY_Page_Down:
                    if (!m_appItems.empty()) {
                        size_t jumpSize = std::min(static_cast<size_t>(10), 
                                                 m_appItems.size() - 1);
                        size_t newIndex = std::min(m_selectedIndex + jumpSize, 
                                                 m_appItems.size() - 1);
                        if (newIndex != m_selectedIndex) {
                            m_appItems[m_selectedIndex]->setActive(false);
                            m_selectedIndex = newIndex;
                            m_appItems[m_selectedIndex]->setActive(true);
                            ensureSelectionVisible();
                        }
                    }
                    break;
                    
                case XKB_KEY_Page_Up:
                    if (!m_appItems.empty()) {
                        size_t jumpSize = std::min(static_cast<size_t>(10), 
                                                 m_selectedIndex);
                        size_t newIndex = m_selectedIndex - jumpSize;
                        if (newIndex != m_selectedIndex) {
                            m_appItems[m_selectedIndex]->setActive(false);
                            m_selectedIndex = newIndex;
                            m_appItems[m_selectedIndex]->setActive(true);
                            ensureSelectionVisible();
                        }
                    }
                    break;
                    
                case XKB_KEY_Home:
                    if (!m_appItems.empty() && m_selectedIndex != 0) {
                        m_appItems[m_selectedIndex]->setActive(false);
                        m_selectedIndex = 0;
                        m_appItems[m_selectedIndex]->setActive(true);
                        ensureSelectionVisible();
                    }
                    break;
                    
                case XKB_KEY_End:
                    if (!m_appItems.empty() && m_selectedIndex != m_appItems.size() - 1) {
                        m_appItems[m_selectedIndex]->setActive(false);
                        m_selectedIndex = m_appItems.size() - 1;
                        m_appItems[m_selectedIndex]->setActive(true);
                        ensureSelectionVisible();
                    }
                    break;
                    
                case XKB_KEY_Return:
                case XKB_KEY_KP_Enter:
                    launchSelectedApp();
                    break;
                    
                case XKB_KEY_slash:
                    if (!ctrl) {
                        focusSearchBox();
                    }
                    break;
                    
                case XKB_KEY_r:
                case XKB_KEY_R:
                    if (ctrl) {
                        reloadApps();
                    }
                    break;
                    
                case XKB_KEY_q:
                case XKB_KEY_Q:
                    if (ctrl) {
                        closeLauncher();
                    }
                    break;
                    
                default:
                    break;
            }
        });
    }

    // Member variables - FIXED TYPES
    bool m_valid = false;
    
    CSharedPointer<IBackend> m_backend;
    CSharedPointer<IWindow> m_window;
    
    CSharedPointer<CRectangleElement> m_background;
    CSharedPointer<CColumnLayoutElement> m_mainLayout;
    CSharedPointer<CTextboxElement> m_searchBox;
    CSharedPointer<CComboboxElement> m_categoryDropdown;
    CSharedPointer<CButtonElement> m_reloadButton;  // FIXED TYPE
    CSharedPointer<CScrollAreaElement> m_scrollArea;
    CSharedPointer<CColumnLayoutElement> m_appList;
    CSharedPointer<CTextElement> m_appCountText;
    
    std::unique_ptr<AppDatabase> m_appDatabase;
    std::vector<DesktopApp> m_filteredApps;
    std::vector<std::shared_ptr<AppItem>> m_appItems;
    
    std::string m_currentSearch;
    std::string m_selectedCategory;
    size_t m_selectedIndex = 0;
    
    Hyprutils::Signal::CHyprSignalListener m_keyboardListener;
    Hyprutils::Signal::CHyprSignalListener m_layerClosedListener;    // ADD THESE
    Hyprutils::Signal::CHyprSignalListener m_closeRequestListener;   // ADD THESE
};