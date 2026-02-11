#include "SearchBox.hpp"
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/window/Window.hpp>  // Add this!
#include <iostream>

using namespace Hyprtoolkit;

SearchBox::SearchBox(CSharedPointer<IBackend> backend,
                     CSharedPointer<IWindow> window,
                     const std::string& hint)
    : m_backend(backend), m_window(window) {
    setupUI(hint);
    setupKeyboardListener();
}

SearchBox::~SearchBox() {
    // Cleanup if needed
}

void SearchBox::setupUI(const std::string& hint) {
    // Get system palette
    auto palette = CPalette::palette();
    
    // Create root element (background rectangle)
    m_rootElement = CRectangleBuilder::begin()
        ->color([palette] {
            if (palette) {
                return palette->m_colors.alternateBase;
            }
            return CHyprColor(0.1, 0.1, 0.1, 0.95);
        })
        ->borderColor([palette] {
            if (palette) {
                auto borderColor = palette->m_colors.text;
                borderColor.a = 0.3;
                return borderColor;
            }
            return CHyprColor(0.3, 0.3, 0.3, 0.5);
        })
        ->borderThickness(1)
        ->rounding(8)
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                           CDynamicSize::HT_SIZE_ABSOLUTE,
                           {1.0f, 40.0f}))
        ->commence();
    
    // Create textbox
    std::string hintCopy = hint;
    m_textbox = CTextboxBuilder::begin()
        ->placeholder(std::move(hintCopy))
        ->defaultText("")
        ->multiline(false)
        ->onTextEdited([this](CSharedPointer<CTextboxElement> textbox, const std::string& text) {
            handleTextChanged(text);
        })
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                           CDynamicSize::HT_SIZE_PERCENT,
                           {1.0f, 1.0f}))
        ->commence();
    
    // Add some margin
    m_rootElement->setMargin(8);
    m_rootElement->addChild(m_textbox);
}

void SearchBox::setupKeyboardListener() {
    if (!m_window) return;
    
    // Listen for keyboard events on the window
    // Store the listener by assigning it to a member variable
    m_keyboardListener = std::shared_ptr<void>(
        new auto(m_window->m_events.keyboardKey.listen([this](const Input::SKeyboardKeyEvent& event) {
            if (!event.down || event.repeat) return;
            
            // Enter key (Return key = 0xFF0D)
            if (event.xkbKeysym == 0xFF0D) {
                handleEnterPressed();
                return;
            }
        }))
    );
}

void SearchBox::handleTextChanged(const std::string& text) {
    m_currentText = text;
    
    // Call text changed callback
    if (m_onTextChanged) {
        m_onTextChanged(text);
    }
}

void SearchBox::handleEnterPressed() {
    // Save current text before any potential modification by Enter key
    m_pendingSearchText = m_currentText;
    
    // Set flag to clear in next idle callback
    m_shouldClear = true;
    
    // Schedule clear for next event loop iteration
    scheduleClear();
    
    // Submit search immediately
    if (m_onSearchSubmitted && !m_pendingSearchText.empty()) {
        m_onSearchSubmitted(m_pendingSearchText);
    }
}

void SearchBox::scheduleClear() {
    // Schedule clearing for next event loop
    if (m_backend) {
        m_backend->addIdle([this] {
            performClear();
        });
    }
}

void SearchBox::performClear() {
    if (m_shouldClear && m_textbox) {
        // Clear the textbox
        if (auto builder = m_textbox->rebuild()) {
            builder->defaultText("")->commence();
        }
        
        // Update internal state
        m_currentText.clear();
        m_shouldClear = false;
        
        // Trigger text changed callback with empty string
        if (m_onTextChanged) {
            m_onTextChanged("");
        }
    }
}