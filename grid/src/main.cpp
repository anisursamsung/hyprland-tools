#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprutils/signal/Signal.hpp>
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
    
    // Common image extensions
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
        
        // Iterate through directory
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (isImageFile(entry.path())) {
                imageFiles.push_back(entry.path());
                std::cout << "  Found: " << entry.path().filename() << std::endl;
            }
        }
        
        // Sort alphabetically
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
        std::cout << "=== Starting Image Gallery from Downloads ===" << std::endl;
        
        // 1. Scan ~/Downloads for images
        std::string homeDir = std::getenv("HOME");
        std::string downloadsDir = homeDir + "/Downloads";
        
        auto imageFiles = scanDirectoryForImages(downloadsDir);
        
        if (imageFiles.empty()) {
            std::cerr << "ERROR: No image files found in " << downloadsDir << std::endl;
            std::cerr << "Supported formats: .png, .jpg, .jpeg, .gif, .bmp, .svg, .webp, .ico, .tiff" << std::endl;
            return 1;
        }
        
        // 2. Create backend
        auto backend = Hyprtoolkit::IBackend::create();
        if (!backend) {
            std::cerr << "ERROR: Failed to create backend" << std::endl;
            return 1;
        }
        std::cout << "Backend created successfully" << std::endl;
        
        // Get palette for colors
        auto palette = backend->getPalette();
        if (!palette) {
            palette = Hyprtoolkit::CPalette::emptyPalette();
            std::cout << "Using empty palette" << std::endl;
        } else {
            std::cout << "System palette loaded" << std::endl;
        }
        
        // 3. Create window
        auto window = Hyprtoolkit::CWindowBuilder::begin()
            ->type(Hyprtoolkit::HT_WINDOW_TOPLEVEL)
            ->appTitle("Downloads Image Gallery")
            ->appClass("image-gallery")
            ->preferredSize({0, 0})  // Fullscreen
            ->commence();
            
        if (!window) {
            std::cerr << "ERROR: Failed to create window" << std::endl;
            return 1;
        }
        std::cout << "Window created successfully" << std::endl;
        
       
        
        // 4. Create GridLayout with configuration
        auto grid = std::make_unique<GridLayout>(backend, window);
        
        GridLayout::Config config;
        config.boxSize = 220.0f;
        config.horizontalSpacing = 15.0f;
        config.verticalSpacing = 15.0f;
        config.scrollable = true;
        config.centerHorizontal = true;
        
        grid->setConfig(config);
        
        
         // 5. Setup close handler      
	Hyprutils::Signal::CHyprSignalListener closeListener;
	closeListener = window->m_events.closeRequest.listen([backend, &grid]() {
    		std::cout << "Close requested - cleaning up..." << std::endl;
    		if (grid) {
       			grid->clear();
        		grid.reset(); // Destroy the grid
    		}
 
    		if (backend) {
        		backend->addIdle([backend]() {
            			std::this_thread::sleep_for(std::chrono::milliseconds(50));
            	if (backend) {
                	backend->destroy();
            }
        	});
    	}
	});
        
        
        
        // 6. Different border colors for visual variety (fallback if no palette)
        std::vector<Hyprtoolkit::CHyprColor> borderColors = {
            {0.2f, 0.5f, 0.8f, 1.0f},   // Blue
            {0.8f, 0.3f, 0.3f, 1.0f},   // Red
            {0.3f, 0.8f, 0.3f, 1.0f},   // Green
            {0.8f, 0.8f, 0.3f, 1.0f},   // Yellow
            {0.8f, 0.3f, 0.8f, 1.0f},   // Purple
            {0.3f, 0.8f, 0.8f, 1.0f},   // Cyan
            {0.8f, 0.5f, 0.2f, 1.0f},   // Orange
            {0.5f, 0.3f, 0.8f, 1.0f},   // Violet
            {0.2f, 0.8f, 0.5f, 1.0f},   // Teal
            {0.8f, 0.2f, 0.5f, 1.0f}    // Pink
        };
        
        // 7. Create boxes for each image found
        std::vector<std::unique_ptr<Box>> boxes;
        std::cout << "\nCreating boxes for " << imageFiles.size() << " images..." << std::endl;
        
        for (size_t i = 0; i < imageFiles.size(); ++i) {
            const auto& imagePath = imageFiles[i];
            std::string filename = getFileNameWithoutExtension(imagePath);
            std::string fullPath = imagePath.string();
            
            // Truncate long filenames for display
            std::string displayName = filename;
            if (displayName.length() > 15) {
                displayName = displayName.substr(0, 12) + "...";
            }
            
            std::cout << "Creating box for: " << filename 
                      << " (" << (i + 1) << "/" << imageFiles.size() << ")" << std::endl;
            
            auto box = std::make_unique<Box>(
                backend,                         // Backend for palette access
                displayName,                     // Title
                fullPath,                        // Image path
                Hyprtoolkit::CHyprColor(0.15f, 0.15f, 0.15f, 1.0f),  // Fallback box color
                borderColors[i % borderColors.size()],               // Fallback border color
                Hyprtoolkit::CHyprColor(1.0f, 1.0f, 1.0f, 1.0f),     // Fallback text color
                config.boxSize, config.boxSize,  // Size
                10, 1                           // Border radius, thickness
            );
            boxes.push_back(std::move(box));
        }
        
        // 8. Add all boxes to grid
        grid->addBoxes(std::move(boxes));
        
        std::cout << "\nAdded " << grid->getTotalBoxes() << " boxes to grid" << std::endl;
        
        // 9. Create the complete UI hierarchy (like the working app launcher)
        // Create root element (transparent container)
        auto root = Hyprtoolkit::CRectangleBuilder::begin()
            ->color([] { 
                return Hyprtoolkit::CHyprColor(0, 0, 0, 0); // Transparent
            })
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 1.0f}))
            ->commence();
        
        // Create window background using palette
        auto background = Hyprtoolkit::CRectangleBuilder::begin()
            ->color([palette] { 
                auto color = palette->m_colors.background;
                return Hyprtoolkit::CHyprColor(color.r, color.g, color.b, 0.95);
            })
            ->rounding(palette->m_vars.bigRounding)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 1.0f}))
            ->commence();
        
        // Create main layout for content
        auto mainLayout = Hyprtoolkit::CColumnLayoutBuilder::begin()
            ->gap(0)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 1.0f}))
            ->commence();
        
        // Add grid to main layout
        mainLayout->addChild(grid->getElement());
        
        // Add main layout to background
        background->addChild(mainLayout);
        
        // Add background to root
        root->addChild(background);
        
        // 10. Set root as window element
        window->m_rootElement = root;
        
        // 11. Print gallery info
        std::cout << "\n=== Gallery Information ===" << std::endl;
        std::cout << "Directory: " << downloadsDir << std::endl;
        std::cout << "Total images: " << imageFiles.size() << std::endl;
        std::cout << "Grid columns: " << grid->getColumnCount() << std::endl;
        std::cout << "Grid rows: " << grid->getRowCount() << std::endl;
        std::cout << "Grid size: " << grid->getGridWidth() << "x" << grid->getGridHeight() << std::endl;
        std::cout << "Box size: " << config.boxSize << "x" << config.boxSize << std::endl;
        std::cout << "Scrollable: " << (config.scrollable ? "Yes" : "No") << std::endl;
        std::cout << "Palette available: " << (palette ? "Yes" : "No") << std::endl;
        if (palette) {
            std::cout << "Window background: " << palette->m_colors.background.r << "," 
                      << palette->m_colors.background.g << ","
                      << palette->m_colors.background.b << std::endl;
        }
        std::cout << "===========================\n" << std::endl;
        
        // 12. Force initial update after window is ready
        backend->addIdle([&grid]() {
            std::cout << "[Main] Performing initial layout update..." << std::endl;
            grid->update();
        });
        
        // 13. Open window
        window->open();
        
        // 14. Enter main loop
        backend->enterLoop();
        
        std::cout << "=== Gallery Closed ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}