#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprutils/signal/Signal.hpp>
#include "SearchBox.hpp"
#include "Box.hpp"
#include "GridLayout.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <filesystem>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;

// Function to check if a file is an image
bool isImageFile(const fs::path& path) {
    if (!fs::is_regular_file(path)) return false;
    
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || 
           ext == ".gif" || ext == ".bmp" || 
           ext == ".webp" || ext == ".ico" || ext == ".tiff" ||
           ext == ".tif";
}

// Function to get filename without extension
std::string getFileNameWithoutExtension(const fs::path& path) {
    std::string filename = path.filename().string();
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        return filename.substr(0, dotPos);
    }
    return filename;
}

// Function to scan directory for images
std::vector<fs::path> scanDirectoryForImages(const std::string& directoryPath) {
    std::vector<fs::path> imageFiles;
    
    try {
        fs::path dir(directoryPath);
        
        if (!fs::exists(dir)) {
            std::cerr << "ERROR: Directory does not exist: " << directoryPath << std::endl;
            return imageFiles;
        }
        
        if (!fs::is_directory(dir)) {
            std::cerr << "ERROR: Path is not a directory: " << directoryPath << std::endl;
            return imageFiles;
        }
        
        std::cout << "Scanning directory: " << directoryPath << std::endl;
        
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (isImageFile(entry.path())) {
                imageFiles.push_back(entry.path());
            }
        }
        
        std::sort(imageFiles.begin(), imageFiles.end());
        
        std::cout << "Found " << imageFiles.size() << " image files" << std::endl;
        
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }
    
    return imageFiles;
}

int main() {
    try {
        // 1. Scan Downloads directory
        std::string homeDir = std::getenv("HOME");
        std::string downloadsDir = homeDir + "/Downloads";
        auto imageFiles = scanDirectoryForImages(downloadsDir);
        
        if (imageFiles.empty()) {
            std::cerr << "ERROR: No image files found in " << downloadsDir << std::endl;
            return 1;
        }
        
        // 2. Create backend
        auto backend = Hyprtoolkit::IBackend::create();
        if (!backend) {
            throw std::runtime_error("Failed to create backend");
        }
        
        // 3. Create window
        auto window = Hyprtoolkit::CWindowBuilder::begin()
            ->type(Hyprtoolkit::HT_WINDOW_TOPLEVEL)
            ->appTitle("Image Gallery with Search")
            ->appClass("image-gallery-search")
            ->preferredSize({1280, 720})
            ->commence();
        
        // 4. Create root element
        auto root = Hyprtoolkit::CRectangleBuilder::begin()
            ->color([] { return Hyprtoolkit::CHyprColor(0, 0, 0, 0); })
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 1.0f}))
            ->commence();
        window->m_rootElement = root;
        
        // 5. Create main layout (10% search, 90% grid)
        auto mainLayout = Hyprtoolkit::CColumnLayoutBuilder::begin()
            ->gap(0)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 1.0f}))
            ->commence();
        
        // 6. Create SearchBox
        auto searchBox = std::make_shared<Hyprtoolkit::SearchBox>(
            backend, window, "Search images...");
        
        // Search container (10% height)
        auto searchContainer = Hyprtoolkit::CRectangleBuilder::begin()
            ->color([] { return Hyprtoolkit::CHyprColor(0, 0, 0, 0); })
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 0.1f}))
            ->commence();
        searchContainer->setMargin(20);
        searchContainer->addChild(searchBox->getView());
        
        // 7. Create GridLayout
        auto grid = std::make_unique<GridLayout>(backend, window);
        GridLayout::Config config;
        config.boxSize = 200.0f;
        config.horizontalSpacing = 10.0f;
        config.verticalSpacing = 10.0f;
        config.scrollable = true;
        config.centerHorizontal = true;
        grid->setConfig(config);
        
        // 8. Create boxes for images
        std::vector<std::unique_ptr<Box>> boxes;
        std::vector<Hyprtoolkit::CHyprColor> borderColors = {
            {0.2f, 0.5f, 0.8f, 1.0f},   // Blue
            {0.8f, 0.3f, 0.3f, 1.0f},   // Red
            {0.3f, 0.8f, 0.3f, 1.0f},   // Green
            {0.8f, 0.8f, 0.3f, 1.0f},   // Yellow
            {0.8f, 0.3f, 0.8f, 1.0f},   // Purple
        };
        
        for (size_t i = 0; i < imageFiles.size(); ++i) {
            const auto& path = imageFiles[i];
            std::string filename = getFileNameWithoutExtension(path);
            
            // Truncate long names
            std::string displayName = filename;
            if (displayName.length() > 15) {
                displayName = displayName.substr(0, 12) + "...";
            }
            
            auto box = std::make_unique<Box>(
                backend,
                displayName,
                path.string(),
                Hyprtoolkit::CHyprColor(0.15f, 0.15f, 0.15f, 1.0f),
                borderColors[i % borderColors.size()],
                Hyprtoolkit::CHyprColor(1.0f, 1.0f, 1.0f, 1.0f),
                config.boxSize, config.boxSize,
                10, 1
            );
            boxes.push_back(std::move(box));
        }
        
        // 9. Add boxes to grid
        grid->addBoxes(std::move(boxes));
        
        // 10. Content container (90% height)
        auto contentContainer = Hyprtoolkit::CRectangleBuilder::begin()
            ->color([] { return Hyprtoolkit::CHyprColor(0, 0, 0, 0); })
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 0.9f}))
            ->commence();
        contentContainer->setMargin(5);
        contentContainer->addChild(grid->getElement());
        
        // 11. Assemble layout
        mainLayout->addChild(searchContainer);
        mainLayout->addChild(contentContainer);
        root->addChild(mainLayout);
        
        // 12. Setup search box callbacks
        searchBox->setOnTextChanged([](const std::string& text) {
            std::cout << "Search: " << text << std::endl;
        });
        
        searchBox->setOnSearchSubmitted([](const std::string& query) {
            std::cout << "Search submitted: " << query << std::endl;
        });
        
        // In the cleanup section, replace with this:

// 12. Setup proper close handler
window->m_events.closeRequest.listenStatic([&backend, &grid]() {
    std::cout << "Close requested..." << std::endl;
    
    // Clear grid first (before destroying UI elements)
    if (grid) {
        grid->clear();
    }
    
    // Destroy backend
    if (backend) {
        backend->destroy();
    }
});

// 13. Run
std::cout << "\n=== Image Gallery with Search ===" << std::endl;
std::cout << "Images loaded: " << imageFiles.size() << std::endl;
std::cout << "Ready to use!" << std::endl;
 backend->addIdle([searchBox] {
            searchBox->focus();
        });
window->open();
backend->enterLoop();

// Clean up grid after loop exits
if (grid) {
    grid->clear();
    grid.reset();
}

std::cout << "Window closed." << std::endl;
return 0;   
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}