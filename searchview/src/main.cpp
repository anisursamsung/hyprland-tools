#include "SearchBox.hpp"
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <iostream>
#include <memory>

using namespace Hyprutils::Memory;
using namespace Hyprtoolkit;

int main() {
    try {
        // Create backend
        auto backend = IBackend::create();
        if (!backend) {
            throw std::runtime_error("Failed to create backend");
        }
        
        // Create window
        auto window = CWindowBuilder::begin()
            ->type(HT_WINDOW_TOPLEVEL)
            ->appTitle("Search Box - Press Enter to Search")
            ->appClass("search-example")
            ->preferredSize({400, 300})
            ->commence();
        
        // Setup UI
        auto root = CRectangleBuilder::begin()
            ->color([] { return CHyprColor(0, 0, 0, 0); })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                               CDynamicSize::HT_SIZE_PERCENT,
                               {1.0f, 1.0f}))
            ->commence();
        window->m_rootElement = root;
        
        // Get palette for colors
        auto palette = CPalette::palette();
        
        // Background
        auto background = CRectangleBuilder::begin()
            ->color([palette] {
                if (palette) {
                    auto bg = palette->m_colors.background;
                    return CHyprColor(bg.r, bg.g, bg.b, 0.95);
                }
                return CHyprColor(0.1, 0.1, 0.15, 0.95);
            })
            ->rounding(palette ? palette->m_vars.bigRounding : 10)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                               CDynamicSize::HT_SIZE_PERCENT,
                               {1.0f, 1.0f}))
            ->commence();
        
        // Main layout
        auto layout = CColumnLayoutBuilder::begin()
            ->gap(20)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                               CDynamicSize::HT_SIZE_PERCENT,
                               {1.0f, 1.0f}))
            ->commence();
        layout->setMargin(30);
        
        // Title
        std::string titleText = "Search Box Demo";
        auto title = CTextBuilder::begin()
            ->text(std::move(titleText))
            ->color([palette] {
                if (palette) return palette->m_colors.text;
                return CHyprColor(0.9, 0.9, 0.9, 1.0);
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                               CDynamicSize::HT_SIZE_ABSOLUTE,
                               {1.0f, 40.0f}))
            ->commence();
        layout->addChild(title);
        
        // Create search box component - Use constructor directly
        auto searchBox = std::make_shared<SearchBox>(backend, window, std::string("Type and press Enter to search"));
        
        // Store last search text to display
        std::string lastSearch = "No search yet";
        
        // Create display text
        auto displayText = CTextBuilder::begin()
            ->text("Last search: " + lastSearch)
            ->color([palette] {
                if (palette) {
                    auto color = palette->m_colors.text;
                    color.a = 0.8;
                    return color;
                }
                return CHyprColor(0.8, 0.8, 0.8, 1.0);
            })
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                               CDynamicSize::HT_SIZE_ABSOLUTE,
                               {1.0f, 40.0f}))
            ->commence();
        
        // Update display function
        auto updateDisplay = [displayText, &lastSearch, palette](const std::string& query) {
            lastSearch = query.empty() ? "Empty search" : query;
            
            // Rebuild text element with new content
            if (auto builder = displayText->rebuild()) {
                std::string newText = "Last search: \"" + lastSearch + "\"";
                builder->text(std::move(newText))
                      ->color([palette] {
                          if (palette) {
                              auto color = palette->m_colors.text;
                              color.a = 0.8;
                              return color;
                          }
                          return CHyprColor(0.8, 0.8, 0.8, 1.0);
                      })
                      ->commence();
            }
        };
        
        // Set up search box callbacks
        searchBox->setOnTextChanged([](const std::string& text) {
            std::cout << "Text changed: \"" << text << "\"" << std::endl;
        });
        
        searchBox->setOnSearchSubmitted([updateDisplay](const std::string& query) {
            std::cout << "Search submitted: \"" << query << "\"" << std::endl;
            updateDisplay(query);
        });
        
        // Add components to layout
        layout->addChild(searchBox->getView());
        layout->addChild(displayText);
        
        // Assemble UI
        background->addChild(layout);
        root->addChild(background);
        
        // Setup window close event (Escape handled by SearchBox)
        window->m_events.closeRequest.listenStatic([&backend] {
            backend->destroy();
        });
        
        // Focus search box after window opens
        backend->addIdle([searchBox] {
            searchBox->focus();
        });
        
        // Run
        std::cout << "=== Search Box Demo ===" << std::endl;
        std::cout << "1. Type in the search box" << std::endl;
        std::cout << "2. Press Enter to submit and clear (SearchBox handles this)" << std::endl;
        std::cout << "3. SearchBox handles all keyboard events internally" << std::endl;
        std::cout << "======================" << std::endl;
        
        window->open();
        backend->enterLoop();
        
        std::cout << "Window closed." << std::endl;
        return 0;
        
    } catch (const std::string& e) {
        std::cerr << "Error: " << e << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}