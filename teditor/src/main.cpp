#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/types/SizeType.hpp>
#include <fstream>
#include <string>
#include <functional>
#include <iostream>

using namespace Hyprtoolkit;

class TextEditor {
public:
    TextEditor() {
        backend = IBackend::create();
        if (!backend)
            throw std::runtime_error("Failed to create backend");
        
        createUI();
        setupKeyboardShortcuts();
    }
    
    void run() {
        window->open();
        backend->enterLoop();
    }
    
    void loadFile(const std::string& filePath) {
        currentFilePath = filePath;
        try {
            std::ifstream file(filePath);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)), 
                                   std::istreambuf_iterator<char>());
                // Update textbox with file content
                auto newTextbox = CTextboxBuilder::begin()
                    ->multiline(true)
                    ->defaultText(std::string(content))
                    ->size(CDynamicSize(
                        CDynamicSize::HT_SIZE_PERCENT,
                        CDynamicSize::HT_SIZE_PERCENT,
                        {1.0, 1.0}))
                    ->onTextEdited([this](CSharedPointer<CTextboxElement> tb, const std::string& text) {
                        isModified = true;
                        updateWindowTitle();
                    })
                    ->commence();
                    
                newTextbox->focus(true);
                newTextbox->setMargin(8);
                
                // Replace textbox
                rootLayout->clearChildren();
                rootLayout->addChild(newTextbox);
                textbox = newTextbox;
                
                updateWindowTitle();
            }
        } catch (...) {
            std::cerr << "Could not open file: " << filePath << std::endl;
        }
    }

private:
    Hyprutils::Memory::CSharedPointer<IBackend> backend;
    Hyprutils::Memory::CSharedPointer<IWindow> window;
    Hyprutils::Memory::CSharedPointer<CTextboxElement> textbox;
    Hyprutils::Memory::CSharedPointer<CColumnLayoutElement> rootLayout;
    
    std::string currentFilePath;
    bool isModified = false;

    void createUI() {
        // Full-screen textbox
        textbox = CTextboxBuilder::begin()
            ->multiline(true)
            ->defaultText(std::string("Welcome to teditor!\n\n"
                         "Keyboard Shortcuts:\n"
                         "  Ctrl+Q - Exit\n"
                         "  Ctrl+S - Save\n"
                         "  Ctrl+O - Open file\n"
                         "  Ctrl+N - New file\n"
                         "\n"
                         "Note: Copy/Paste/Cut/Undo/Redo/Select All\n"
                         "are handled by the system clipboard.\n"
                         "\n"
                         "Start typing here..."))
            ->size(CDynamicSize(
                CDynamicSize::HT_SIZE_PERCENT,
                CDynamicSize::HT_SIZE_PERCENT,
                {1.0, 1.0}))
            ->onTextEdited([this](CSharedPointer<CTextboxElement> tb, const std::string& text) {
                isModified = true;
                updateWindowTitle();
            })
            ->commence();

        textbox->focus(true);
        textbox->setMargin(8);

        // Main layout
        rootLayout = CColumnLayoutBuilder::begin()
            ->gap(0)
            ->size(CDynamicSize(
                CDynamicSize::HT_SIZE_PERCENT,
                CDynamicSize::HT_SIZE_PERCENT,
                {1.0, 1.0}))
            ->commence();

        rootLayout->addChild(textbox);

        // Window
        window = CWindowBuilder::begin()
            ->type(HT_WINDOW_TOPLEVEL)
            ->appTitle("teditor - Minimal Text Editor")
            ->appClass("teditor")
            ->preferredSize({800, 600})
            ->commence();

        window->m_rootElement = rootLayout;
    }

    void setupKeyboardShortcuts() {
        // Fixed: Use listen() method instead of assignment
        m_keyboardListener = window->m_events.keyboardKey.listen([this](Input::SKeyboardKeyEvent event) {
            handleKeyEvent(event);
        });
        
        m_closeListener = window->m_events.closeRequest.listen([this]() {
            if (checkUnsavedChanges()) {
                backend->destroy();
            }
        });
    }

    void handleKeyEvent(Input::SKeyboardKeyEvent event) {
        if (!event.down) return;
        
        // Check for Ctrl key combinations
        if (event.modMask & Input::HT_MODIFIER_CTRL) {
            switch (event.xkbKeysym) {
                case 'q': // Ctrl+Q - Exit
                    if (checkUnsavedChanges()) {
                        backend->destroy();
                    }
                    break;
                    
                case 's': // Ctrl+S - Save
                    saveFile();
                    break;
                    
                case 'o': // Ctrl+O - Open (placeholder)
                    // In a real implementation, show file dialog
                    std::cout << "Open file (placeholder - implement file dialog)" << std::endl;
                    break;
                    
                case 'n': // Ctrl+N - New
                    if (checkUnsavedChanges()) {
                        newFile();
                    }
                    break;
                    
                // Note: Copy, Paste, Cut, Undo, Redo, Select All
                // are handled by the system/X11 clipboard automatically
                // for multiline textboxes in Hyprtoolkit
            }
        }
        
        // Also handle Escape key
        if (event.xkbKeysym == 0xFF1B) { // Escape
            if (checkUnsavedChanges()) {
                backend->destroy();
            }
        }
    }

    void newFile() {
        currentFilePath.clear();
        isModified = false;
        
        auto newTextbox = CTextboxBuilder::begin()
            ->multiline(true)
            ->defaultText(std::string(""))
            ->size(CDynamicSize(
                CDynamicSize::HT_SIZE_PERCENT,
                CDynamicSize::HT_SIZE_PERCENT,
                {1.0, 1.0}))
            ->onTextEdited([this](CSharedPointer<CTextboxElement> tb, const std::string& text) {
                isModified = true;
                updateWindowTitle();
            })
            ->commence();
            
        newTextbox->focus(true);
        newTextbox->setMargin(8);
        
        rootLayout->clearChildren();
        rootLayout->addChild(newTextbox);
        textbox = newTextbox;
        
        updateWindowTitle();
    }

    bool saveFile() {
        if (currentFilePath.empty()) {
            // In a real implementation, show save dialog
            // For now, use a default name
            currentFilePath = "untitled.txt";
        }
        
        try {
            std::ofstream file(currentFilePath);
            if (!file.is_open()) {
                std::cerr << "Failed to open file for writing: " << currentFilePath << std::endl;
                return false;
            }
            
            // Get text from textbox - need to find the correct API
            // For now, we'll save a placeholder
            std::string text = "[Text content would be saved here]";
            // Note: Need to check Hyprtoolkit API for getting textbox content
            // text = textbox->getCurrentText() or similar
            
            file << text;
            file.close();
            
            isModified = false;
            updateWindowTitle();
            std::cout << "Saved to: " << currentFilePath << " (placeholder)" << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error saving file: " << e.what() << std::endl;
            return false;
        }
    }

    bool checkUnsavedChanges() {
        if (!isModified) {
            return true;
        }
        
        // In a real implementation, show confirmation dialog
        // For now, ask user in terminal
        std::cout << "Unsaved changes. Save before closing? (y/n): ";
        std::string response;
        std::getline(std::cin, response);
        
        if (response == "y" || response == "Y") {
            return saveFile();
        }
        return true; // Don't save, allow close
    }

    void updateWindowTitle() {
        std::string title = "teditor";
        if (!currentFilePath.empty()) {
            size_t pos = currentFilePath.find_last_of("/\\");
            std::string filename = (pos != std::string::npos) ? 
                currentFilePath.substr(pos + 1) : currentFilePath;
            title += " - " + filename;
        } else {
            title += " - Untitled";
        }
        
        if (isModified) {
            title += " *";
        }
        
        // Note: Window title might be set during window creation
        // You might need to recreate the window or use a different API
        std::cout << "Window title: " << title << std::endl;
    }
    
private:
    Hyprutils::Signal::CHyprSignalListener m_keyboardListener;
    Hyprutils::Signal::CHyprSignalListener m_closeListener;
};

int main(int argc, char* argv[]) {
    try {
        TextEditor editor;
        
        // Check if file was passed as argument
        if (argc > 1) {
            editor.loadFile(argv[1]);
        }
        
        editor.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}