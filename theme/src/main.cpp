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
#include <xkbcommon/xkbcommon-keysyms.h>
#include <cstdlib>

namespace fs = std::filesystem;

using namespace Hyprutils::Memory;
using namespace Hyprtoolkit;

// ============================================
// WallpaperItem
// ============================================

struct WallpaperItem {
    std::string filename;
    std::string path;
    fs::path filepath;

    bool operator<(const WallpaperItem& other) const {
        return filename < other.filename;
    }
};

// ============================================
// WallpaperDatabase
// ============================================

class WallpaperDatabase {
public:
    WallpaperDatabase() { loadWallpapers(); }

    const std::vector<WallpaperItem>& getAllWallpapers() const { return m_allWallpapers; }

private:
    void loadWallpapers() {
        const char* home = std::getenv("HOME");
        if (!home) return;

        fs::path downloadsPath = fs::path(home) / "Downloads";
        if (!fs::exists(downloadsPath)) return;

        static const std::vector<std::string> imageExtensions = {
            ".jpg", ".jpeg", ".png", ".bmp", ".gif", ".tiff", ".webp"
        };

        for (const auto& entry : fs::directory_iterator(downloadsPath)) {
            if (!entry.is_regular_file()) continue;

            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (std::find(imageExtensions.begin(), imageExtensions.end(), ext) != imageExtensions.end()) {
                WallpaperItem item;
                item.filename = entry.path().filename().string();
                item.path = entry.path().string();
                item.filepath = entry.path();
                m_allWallpapers.push_back(item);
            }
        }

        std::sort(m_allWallpapers.begin(), m_allWallpapers.end());
    }

    std::vector<WallpaperItem> m_allWallpapers;
};

// ============================================
// GridWallpaperItem
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
    bool isActive() const { return m_active; }

    void setActive(bool active) {
        if (m_active == active) return;
        m_active = active;
        updateAppearance();
    }

    void updateAppearance() {
        if (!m_background || !m_text) return;
        auto palette = CPalette::palette();
        if (auto builder = m_background->rebuild()) {
            builder->color([this, palette] {
                if (palette)
                    return m_active ? palette->m_colors.accent.mix(palette->m_colors.base, 0.3)
                    : palette->m_colors.alternateBase;
                return m_active ? CHyprColor(0.2, 0.4, 0.8, 0.8)
                : CHyprColor(0.2, 0.2, 0.2, 0.3);
            })->commence();
        }
        if (auto builder = m_text->rebuild()) {
            builder->color([this, palette] {
                if (palette) return m_active ? palette->m_colors.brightText : palette->m_colors.text;
                return m_active ? CHyprColor(1, 1, 1, 1) : CHyprColor(0.8, 0.8, 0.8, 1);
            })->commence();
        }
        m_background->forceReposition();
    }

    const WallpaperItem& getWallpaper() const { return m_wallpaper; }

    void select() {
        std::string command = "notify-send \"Theme App\" \"Selected: " + m_wallpaper.filename + "\"";
        std::system(command.c_str());
        std::cout << "Selected wallpaper: " << m_wallpaper.filename << std::endl;
    }

private:
    void createUI() {
        auto palette = CPalette::palette();
        const float ITEM_WIDTH = 180.0F;
        const float ITEM_HEIGHT = 180.0F;

        m_background = CRectangleBuilder::begin()
        ->color([palette] { return palette ? palette->m_colors.alternateBase : CHyprColor(0.2, 0.2, 0.2, 0.3); })
        ->rounding(palette ? palette->m_vars.smallRounding : 12)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                            CDynamicSize::HT_SIZE_ABSOLUTE,
                            {ITEM_WIDTH, ITEM_HEIGHT}))
        ->commence();

        m_columnLayout = CColumnLayoutBuilder::begin()
        ->gap(0)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                            CDynamicSize::HT_SIZE_PERCENT,
                            {1.0F, 1.0F}))
        ->commence();

        auto imageContainer = CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0, 0, 0, 0); })
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                            CDynamicSize::HT_SIZE_ABSOLUTE,
                            {1.0F, ITEM_WIDTH - 40.F}))
        ->commence();

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

        auto gapElement = CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0, 0, 0, 0); })
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                            CDynamicSize::HT_SIZE_ABSOLUTE,
                            {1.0F, 5.F}))
        ->commence();
        m_columnLayout->addChild(gapElement);

        m_text = CTextBuilder::begin()
        ->text(std::string{m_wallpaper.filename})
        ->color([palette] { return palette ? palette->m_colors.text : CHyprColor(0.8, 0.8, 0.8, 1); })
        ->align(eFontAlignment::HT_FONT_ALIGN_CENTER)
        ->fontSize(CFontSize(CFontSize::HT_FONT_SMALL, 0.9F))
        ->fontFamily(palette ? palette->m_vars.fontFamily : "Sans Serif")
        ->clampSize({ITEM_WIDTH - 20.F, 20.F})
        ->noEllipsize(false)
        ->commence();

        auto textContainer = CColumnLayoutBuilder::begin()
        ->gap(0)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                            CDynamicSize::HT_SIZE_ABSOLUTE,
                            {1.0F, 30.F}))
        ->commence();
        textContainer->addChild(m_text);
        m_columnLayout->addChild(textContainer);

        m_background->addChild(m_columnLayout);
        setupMouseHandlers();
    }

    void setupMouseHandlers() {
        m_background->setReceivesMouse(true);
        m_background->setMouseEnter([this](const Hyprutils::Math::Vector2D&) { if (m_onHover) m_onHover(); });
        m_background->setMouseButton([this](Input::eMouseButton button, bool down) {
            if (button == Input::MOUSE_BUTTON_LEFT && down && m_onClick) m_onClick();
        });
    }

    WallpaperItem m_wallpaper;
    CSharedPointer<IBackend> m_backend;
    bool m_active = false;

    CSharedPointer<CRectangleElement> m_background;
    CSharedPointer<CColumnLayoutElement> m_columnLayout;
    CSharedPointer<CTextElement> m_text;

    std::function<void()> m_onHover;
    std::function<void()> m_onClick;
};

// ============================================
// ThemeApp
// ============================================

class ThemeApp {
public:
    ThemeApp() {
        m_backend = IBackend::create();
        if (!m_backend) throw std::runtime_error("Failed to create backend");

        m_wallpaperDatabase = std::make_unique<WallpaperDatabase>();
    }

    void run() {
        createWindow();
        createUI();
        setupEventHandlers();
        m_window->open();
        m_backend->enterLoop();
    }

private:
    // -------------------------------
    void createWindow() {
        Hyprutils::Math::Vector2D preferredSize(750.0, 600.0);

        m_window = CWindowBuilder::begin()
        ->type(HT_WINDOW_LAYER)
        ->appTitle("Theme App - Wallpaper Selector")
        ->appClass("theme-app")
        ->preferredSize(preferredSize)
        ->anchor(1 | 2 | 4 | 8)
        ->layer(3)
        ->marginTopLeft({100, 100})
        ->marginBottomRight({100, 100})
        ->kbInteractive(1)
        ->exclusiveZone(-1)
        ->commence();
    }

    void createUI() {
        auto root = CRectangleBuilder::begin()
        ->color([] { return CHyprColor(0, 0, 0, 0); })
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                            CDynamicSize::HT_SIZE_PERCENT,
                            {1.0F, 1.0F}))
        ->commence();
        m_window->m_rootElement = root;

        auto palette = CPalette::palette();
        if (!palette) palette = CPalette::emptyPalette();

        m_background = CRectangleBuilder::begin()
        ->color([palette] { return CHyprColor(0,0,0,0.95); })
        ->rounding(palette->m_vars.bigRounding)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F,1.0F}))
        ->commence();

        m_mainLayout = CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0F,1.0F}))->commence();
        m_mainLayout->setMargin(12);

        // Title
        auto title = CTextBuilder::begin()
        ->text("Wallpaper Selector")
        ->color([palette] { return palette->m_colors.text; })
        ->fontSize(CFontSize(CFontSize::HT_FONT_H2, 1.0F))
        ->align(eFontAlignment::HT_FONT_ALIGN_CENTER)
        ->commence();
        m_mainLayout->addChild(title);

        // Status
        m_statusText = CTextBuilder::begin()
        ->text("Loading wallpapers...")
        ->color([palette] { return palette->m_colors.text; })
        ->fontSize(CFontSize(CFontSize::HT_FONT_SMALL, 1.0F))
        ->commence();
        m_mainLayout->addChild(m_statusText);

        // Search box
        m_searchBox = CTextboxBuilder::begin()
        ->placeholder("Type to search...")
        ->color([palette]{ return palette->m_colors.text; })
        ->fontSize(CFontSize(CFontSize::HT_FONT_SMALL, 1.0F))
        ->commence();

        m_searchBox->setTextChanged([this](const std::string& newText) { filterWallpapers(newText); });
        m_mainLayout->addChild(m_searchBox);

        // Scroll area
        m_scrollArea = CScrollAreaBuilder::begin()->scrollY(true)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0F, 500.F}))->commence();
        m_scrollArea->setGrow(true);
        m_mainLayout->addChild(m_scrollArea);

        // Create grid once
        createGridView();

        // Select first visible item
        for (size_t i=0;i<m_gridItems.size();i++)
            if(m_gridItems[i]->getElement()->isVisible()){ selectItem(i); break; }

            m_background->addChild(m_mainLayout);
        root->addChild(m_background);
    }

    // -------------------------------
    void createGridView() {
        const int COLUMN_COUNT = 4;
        const float ITEM_WIDTH = 180.0F;
        const float ITEM_HEIGHT = 180.0F;
        const float ROW_GAP = 10.0F;
        const float COLUMN_GAP = 10.0F;

        auto& wallpapers = m_wallpaperDatabase->getAllWallpapers();
        size_t numWallpapers = wallpapers.size();
        size_t numRows = (numWallpapers + COLUMN_COUNT -1)/COLUMN_COUNT;

        auto gridContainer = CRectangleBuilder::begin()
        ->color([]{ return CHyprColor(0,0,0,0); })
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE,
                            {COLUMN_COUNT*ITEM_WIDTH + COLUMN_GAP*(COLUMN_COUNT-1),
                                numRows*ITEM_HEIGHT + ROW_GAP*(numRows-1)}))
        ->commence();

        auto gridLayout = CColumnLayoutBuilder::begin()->gap(static_cast<size_t>(ROW_GAP))->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT,{1.0F,1.0F}))->commence();

        for(size_t row=0;row<numRows;row++){
            auto rowLayout = CRowLayoutBuilder::begin()->gap(static_cast<size_t>(COLUMN_GAP))->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE,{1.0F, ITEM_HEIGHT}))->commence();
            for(int col=0;col<COLUMN_COUNT;col++){
                size_t index = row*COLUMN_COUNT + col;
                if(index>=numWallpapers){
                    auto empty = CRectangleBuilder::begin()->color([]{return CHyprColor(0,0,0,0);})->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE,{ITEM_WIDTH,ITEM_HEIGHT}))->commence();
                    rowLayout->addChild(empty);
                    continue;
                }
                const auto &wallpaper = wallpapers[index];
                size_t itemIndex = m_gridItems.size();
                auto onHover = [this,itemIndex](){ selectItem(itemIndex); };
                auto onClick = [this,itemIndex](){ selectItem(itemIndex); selectCurrentWallpaper(); };
                auto gridItem = std::make_shared<GridWallpaperItem>(wallpaper,m_backend,onHover,onClick);
                if(gridItem->getElement()){
                    rowLayout->addChild(gridItem->getElement());
                    m_gridItems.push_back(gridItem);
                }
            }
            gridLayout->addChild(rowLayout);
        }
        gridContainer->addChild(gridLayout);
        m_scrollArea->addChild(gridContainer);
    }

    // -------------------------------
    void filterWallpapers(const std::string& filter){
        if(m_gridItems.empty()) return;
        std::string lowerFilter = filter;
        std::transform(lowerFilter.begin(),lowerFilter.end(),lowerFilter.begin(),::tolower);

        size_t visibleCount=0;
        for(auto &item : m_gridItems){
            std::string name = item->getWallpaper().filename;
            std::transform(name.begin(),name.end(),name.begin(),::tolower);
            bool match = name.find(lowerFilter)!=std::string::npos;
            item->getElement()->setVisible(match);
            if(match) visibleCount++;
        }

        if(auto builder = m_statusText->rebuild()){
            builder->text("Found "+std::to_string(visibleCount)+" wallpapers")->commence();
        }

        if(!m_gridItems[m_selectedIndex]->getElement()->isVisible()){
            for(size_t i=0;i<m_gridItems.size();i++){
                if(m_gridItems[i]->getElement()->isVisible()){ selectItem(i); break; }
            }
        }
    }

    // -------------------------------
    void selectItem(size_t index){
        if(index>=m_gridItems.size()) return;
        if(m_selectedIndex<m_gridItems.size()) m_gridItems[m_selectedIndex]->setActive(false);
        m_selectedIndex=index;
        m_gridItems[m_selectedIndex]->setActive(true);
        updateGridPosition();
        ensureSelectionVisible();
    }

    void updateGridPosition(){ const int COLUMN_COUNT=4; m_gridRow=m_selectedIndex/COLUMN_COUNT; m_gridCol=m_selectedIndex%COLUMN_COUNT; }

    void moveGridSelection(int deltaRow,int deltaCol){
        if(m_gridItems.empty()) return;
        m_gridItems[m_selectedIndex]->setActive(false);

        const int COLUMN_COUNT=4;
        int newRow=int(m_gridRow)+deltaRow;
        int newCol=int(m_gridCol)+deltaCol;
        int totalRows=(m_gridItems.size()+COLUMN_COUNT-1)/COLUMN_COUNT;

        if(newRow<0) newRow=totalRows-1;
        else if(newRow>=totalRows) newRow=0;

        size_t itemsInRow=std::min(static_cast<size_t>(COLUMN_COUNT),m_gridItems.size()-newRow*COLUMN_COUNT);
        if(newCol<0){ newCol=itemsInRow-1; newRow--; if(newRow<0) newRow=totalRows-1; }
        else if(newCol>=int(itemsInRow)){ newCol=0; newRow++; if(newRow>=totalRows) newRow=0; }

        size_t newIndex=newRow*COLUMN_COUNT+newCol;
        if(newIndex>=m_gridItems.size()) newIndex=m_gridItems.size()-1;

        m_selectedIndex=newIndex;
        m_gridRow=newRow;
        m_gridCol=newCol;
        m_gridItems[m_selectedIndex]->setActive(true);
        ensureSelectionVisible();
    }

    void ensureSelectionVisible(){
        if(m_gridItems.empty()||!m_scrollArea) return;
        const float ITEM_HEIGHT=180.0F;
        const float ROW_GAP=10.0F;
        const float ROW_HEIGHT=ITEM_HEIGHT+ROW_GAP;
        const float SCROLL_HEIGHT=m_scrollArea->size().y;
        const float CURRENT_SCROLL=m_scrollArea->getCurrentScroll().y;
        const float TOP=m_gridRow*ROW_HEIGHT;
        const float BOTTOM=TOP+ITEM_HEIGHT;
        if(TOP<CURRENT_SCROLL) m_scrollArea->setScroll({0.F,TOP});
        else if(BOTTOM>CURRENT_SCROLL+SCROLL_HEIGHT) m_scrollArea->setScroll({0.F,BOTTOM-SCROLL_HEIGHT});
    }

    void selectCurrentWallpaper(){ if(m_selectedIndex<m_gridItems.size()) m_gridItems[m_selectedIndex]->select(); }
    void closeApp(){ if(m_window) m_window->close(); }

    void setupEventHandlers(){
        if(!m_window) return;
        m_window->m_events.layerClosed.listenStatic([this]{if(m_backend)m_backend->destroy();});
        m_window->m_events.closeRequest.listenStatic([this]{if(m_backend)m_backend->destroy();});
        m_keyboardListener=m_window->m_events.keyboardKey.listen([this](const Input::SKeyboardKeyEvent& e){
            if(!e.down) return;
            switch(e.xkbKeysym){
                case XKB_KEY_Escape: closeApp(); break;
                case XKB_KEY_Down: moveGridSelection(+1,0); break;
                case XKB_KEY_Up: moveGridSelection(-1,0); break;
                case XKB_KEY_Right: moveGridSelection(0,+1); break;
                case XKB_KEY_Left: moveGridSelection(0,-1); break;
                case XKB_KEY_Return:
                case XKB_KEY_KP_Enter: selectCurrentWallpaper(); break;
                default: break;
            }
        });
    }

    // -------------------------------
    CSharedPointer<IBackend> m_backend;
    CSharedPointer<IWindow> m_window;
    CSharedPointer<CRectangleElement> m_background;
    CSharedPointer<CColumnLayoutElement> m_mainLayout;
    CSharedPointer<CTextElement> m_statusText;
    CSharedPointer<CScrollAreaElement> m_scrollArea;
    CSharedPointer<CTextboxElement> m_searchBox;
    std::unique_ptr<WallpaperDatabase> m_wallpaperDatabase;
    std::vector<std::shared_ptr<GridWallpaperItem>> m_gridItems;
    size_t m_selectedIndex=0;
    size_t m_gridRow=0,m_gridCol=0;
    Hyprutils::Signal::CHyprSignalListener m_keyboardListener;
};

// ============================================
// main
// ============================================

int main(){
    try{
        ThemeApp app;
        app.run();
        return 0;
    }catch(const std::exception &e){
        std::cerr<<"Error: "<<e.what()<<std::endl;
        return 1;
    }
}
