#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <functional>
#include <string>
#include <memory>
#include <atomic>

// Forward declaration
namespace Hyprtoolkit {
    class IWindow;
}

using namespace Hyprutils::Memory;

namespace Hyprtoolkit {

class SearchBox {
public:
    // Callback types
    using OnTextChangedCallback = std::function<void(const std::string& text)>;
    using OnSearchSubmittedCallback = std::function<void(const std::string& text)>;
    
    // Constructor - takes window for keyboard events
    SearchBox(CSharedPointer<IBackend> backend,
              CSharedPointer<IWindow> window,
              const std::string& hint = "Search...");
    
    ~SearchBox();
    
    // Get the root element for adding to layouts
    CSharedPointer<IElement> getView() const { return m_rootElement; }
    
    // Programmatic controls
    std::string getText() const { return m_currentText; }
    void focus() { if (m_textbox) m_textbox->focus(true); }
    
    // Callback setters
    void setOnTextChanged(OnTextChangedCallback callback) {
        m_onTextChanged = callback;
    }
    
    void setOnSearchSubmitted(OnSearchSubmittedCallback callback) {
        m_onSearchSubmitted = callback;
    }
    
private:
    void setupUI(const std::string& hint);
    void setupKeyboardListener();
    void handleTextChanged(const std::string& text);
    void handleEnterPressed();
    void scheduleClear();
    void performClear();
    
    // Member variables
    CSharedPointer<IBackend> m_backend;
    CSharedPointer<IWindow> m_window;
    CSharedPointer<CRectangleElement> m_rootElement;
    CSharedPointer<CTextboxElement> m_textbox;
    std::shared_ptr<void> m_keyboardListener; // Store listener
    std::string m_currentText;
    std::string m_pendingSearchText;
    std::atomic<bool> m_shouldClear{false};
    OnTextChangedCallback m_onTextChanged;
    OnSearchSubmittedCallback m_onSearchSubmitted;
};

} // namespace Hyprtoolkit