#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprutils/memory/SharedPtr.hpp>

using namespace Hyprutils::Memory;

int main() {
    // Create backend
    auto backend = Hyprtoolkit::IBackend::create();

    // Create TOP LAYER window
    auto layer = Hyprtoolkit::CWindowBuilder::begin()
                     ->type(Hyprtoolkit::HT_WINDOW_LAYER)  // Layer window
                     ->appTitle("Top Layer Demo")
                     ->appClass("toplayer")
                     ->preferredSize({300, 150})
                     ->anchor(0)        // Anchor to all edges
                     ->layer(2)         // Top layer (2 = top, 3 = overlay)
                     ->marginTopLeft({20, 20})      // 20px from top-left
                     ->marginBottomRight({20, 20})  // 20px from bottom-right
                     ->kbInteractive(1)  // Keyboard interactive
                     ->exclusiveZone(0)  // Non-exclusive (allows interaction)
                     ->commence();

    // Create background
    auto background = Hyprtoolkit::CRectangleBuilder::begin()
                          ->color([&backend] {
                              return backend->getPalette()->m_colors.background;
                          })
                          ->commence();

    // Create layout to center content
    auto layout = Hyprtoolkit::CColumnLayoutBuilder::begin()
                      ->size({Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                              Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                              {1.0F, 1.0F}})
                      ->commence();

    // Create text element
    auto text = Hyprtoolkit::CTextBuilder::begin()
                    ->text("TOP LAYER DEMO")
                    ->color([&backend] {
                        return backend->getPalette()->m_colors.text;
                    })
                    ->fontSize(Hyprtoolkit::CFontSize{Hyprtoolkit::CFontSize::HT_FONT_H1})
                    ->commence();

    // Add text to layout
    layout->addChild(text);

    // Assemble UI
    background->addChild(layout);
    layer->m_rootElement->addChild(background);

    // Handle layer close
    layer->m_events.layerClosed.listenStatic([&backend] {
        backend->destroy();
    });

    // Handle close request
    layer->m_events.closeRequest.listenStatic([&backend] {
        backend->destroy();
    });

    // Open layer
    layer->open();
    backend->enterLoop();

    return 0;
}