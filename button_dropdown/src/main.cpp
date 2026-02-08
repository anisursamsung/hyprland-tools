#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Combobox.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <functional>
#include <iostream>

using namespace Hyprutils::Memory;

int main() {
    // Create backend
    auto backend = Hyprtoolkit::IBackend::create();

    // Create window
    auto window = Hyprtoolkit::CWindowBuilder::begin()
                      ->appTitle("Theme Configurator")
                      ->appClass("themeconfig")
                      ->commence();

    // Create background
    auto background = Hyprtoolkit::CRectangleBuilder::begin()
                          ->color([&backend] { 
                              return backend->getPalette()->m_colors.background; 
                          })
                          ->commence();

    // Main layout
    auto mainLayout = Hyprtoolkit::CColumnLayoutBuilder::begin()
                          ->gap(15)
                          ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, 
                                  Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, 
                                  {0.9F, 0.9F}})
                          ->commence();

    // Header with H1 font
    auto header = Hyprtoolkit::CTextBuilder::begin()
                      ->text("Theme Configuration")
                      ->color([&backend] { 
                          return backend->getPalette()->m_colors.text; 
                      })
                      ->fontSize(Hyprtoolkit::CFontSize{Hyprtoolkit::CFontSize::HT_FONT_H1})
                      ->commence();

    // Theme mode section
    auto themeSection = Hyprtoolkit::CColumnLayoutBuilder::begin()
                            ->gap(8)
                            ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, 
                                    Hyprtoolkit::CDynamicSize::HT_SIZE_AUTO, 
                                    {1.0F, 1.0F}})
                            ->commence();

    auto themeLabel = Hyprtoolkit::CTextBuilder::begin()
                          ->text("Change Theme Mode:")
                          ->color([&backend] { 
                              return backend->getPalette()->m_colors.text; 
                          })
                          ->commence();

    auto themeCombo = Hyprtoolkit::CComboboxBuilder::begin()
                          ->items({"Dark", "Light", "Toggle"})
                          ->currentItem(0)
                          ->onChanged([](CSharedPointer<Hyprtoolkit::CComboboxElement> elem, size_t idx) {
                              std::cout << "Theme selected: " << idx << std::endl;
                          })
                          ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, 
                                  Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, 
                                  {0.8F, 35.F}})
                          ->commence();

    themeSection->addChild(themeLabel);
    themeSection->addChild(themeCombo);

    // Wallpaper section
    auto wallpaperButton = Hyprtoolkit::CButtonBuilder::begin()
                               ->label("Change Wallpaper")
                               ->onMainClick([](CSharedPointer<Hyprtoolkit::CButtonElement>) {
                                   std::cout << "Wallpaper change requested" << std::endl;
                               })
                               ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT, 
                                       Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE, 
                                       {0.8F, 40.F}})
                               ->commence();

    // Add everything
    mainLayout->addChild(header);
    mainLayout->addChild(themeSection);
    mainLayout->addChild(wallpaperButton);

    background->addChild(mainLayout);
    window->m_rootElement->addChild(background);

    // Handle close
    window->m_events.closeRequest.listenStatic([&backend] {
        backend->destroy();
    });

    // Run
    window->open();
    backend->enterLoop();

    return 0;
}