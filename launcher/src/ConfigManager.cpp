#include "ConfigManager.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdlib>

ConfigManager::ConfigManager() {
    loadConfig();
}

void ConfigManager::loadConfig() {
    // Try to get user's home directory
    const char* homeDir = std::getenv("HOME");
    if (!homeDir) {
        std::cerr << "Warning: Could not determine home directory, using defaults\n";
        return;
    }
    
    // Config file path
    fs::path configPath = fs::path(homeDir) / ".config" / "launcher" / "launcher.conf";
    
    // Create directory if it doesn't exist
    fs::path configDir = configPath.parent_path();
    if (!fs::exists(configDir)) {
        try {
            fs::create_directories(configDir);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Could not create config directory: " << e.what() << "\n";
            return;
        }
    }
    
    // Check if config file exists
    if (!fs::exists(configPath)) {
        std::cout << "Config file not found at: " << configPath << "\n";
        std::cout << "Using default configuration\n";
        
        // Create default config file
        try {
            std::ofstream defaultConfig(configPath);
            if (defaultConfig.is_open()) {
                defaultConfig << "# Launcher Configuration\n";
                defaultConfig << "# Available options:\n";
                defaultConfig << "# default_view = list | grid\n";
                defaultConfig << "# column_count = <positive integer>\n";
                defaultConfig << "# grid_item_width = <positive integer>\n";
                defaultConfig << "# grid_item_height = <positive integer>\n";
                defaultConfig << "# grid_horizontal_gap = <positive integer>\n";
                defaultConfig << "# grid_vertical_gap = <positive integer>\n\n";
                defaultConfig << "default_view = list\n";
                defaultConfig << "column_count = 6\n";
                defaultConfig << "grid_item_width = 120\n";
                defaultConfig << "grid_item_height = 120\n";
                defaultConfig << "grid_horizontal_gap = 10\n";
                defaultConfig << "grid_vertical_gap = 10\n";
                defaultConfig.close();
                std::cout << "Created default config file at: " << configPath << "\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Could not create default config file: " << e.what() << "\n";
        }
        return;
    }
    
    // Parse existing config file
    try {
        parseConfigFile(configPath);
        configLoaded = true;
        std::cout << "Loaded configuration from: " << configPath << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config file: " << e.what() << "\n";
        std::cerr << "Using default configuration\n";
    }
}

void ConfigManager::parseConfigFile(const fs::path& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file");
    }
    
    std::string line;
    int lineNum = 0;
    
    while (std::getline(file, line)) {
        lineNum++;
        
        // Remove comments
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        // Trim whitespace
        line = trim(line);
        
        // Skip empty lines
        if (line.empty()) {
            continue;
        }
        
        // Parse key = value
        size_t equalsPos = line.find('=');
        if (equalsPos == std::string::npos) {
            std::cerr << "Warning: Invalid config line " << lineNum 
                      << " (missing '='): " << line << "\n";
            continue;
        }
        
        std::string key = trim(line.substr(0, equalsPos));
        std::string value = trim(line.substr(equalsPos + 1));
        
        // Convert key to lowercase for case-insensitive comparison
        key = toLower(key);
        
        // Set the value
        setValue(key, value);
    }
}

void ConfigManager::setValue(const std::string& key, const std::string& value) {
    if (key == "default_view") {
        if (isValidViewMode(value)) {
            defaultView = value;
        } else {
            std::cerr << "Warning: Invalid default_view: " << value 
                      << ". Must be 'list' or 'grid'. Using default: " << defaultView << "\n";
        }
    } 
    else if (key == "column_count") {
        int val;
        if (isValidPositiveInt(value, val)) {
            columnCount = val;
        } else {
            std::cerr << "Warning: Invalid column_count: " << value 
                      << ". Using default: " << columnCount << "\n";
        }
    }
    else if (key == "grid_item_width") {
        int val;
        if (isValidPositiveInt(value, val)) {
            gridItemWidth = val;
        } else {
            std::cerr << "Warning: Invalid grid_item_width: " << value 
                      << ". Using default: " << gridItemWidth << "\n";
        }
    }
    else if (key == "grid_item_height") {
        int val;
        if (isValidPositiveInt(value, val)) {
            gridItemHeight = val;
        } else {
            std::cerr << "Warning: Invalid grid_item_height: " << value 
                      << ". Using default: " << gridItemHeight << "\n";
        }
    }
    else if (key == "grid_horizontal_gap") {
        int val;
        if (isValidPositiveInt(value, val)) {
            gridHorizontalGap = val;
        } else {
            std::cerr << "Warning: Invalid grid_horizontal_gap: " << value 
                      << ". Using default: " << gridHorizontalGap << "\n";
        }
    }
    else if (key == "grid_vertical_gap") {
        int val;
        if (isValidPositiveInt(value, val)) {
            gridVerticalGap = val;
        } else {
            std::cerr << "Warning: Invalid grid_vertical_gap: " << value 
                      << ". Using default: " << gridVerticalGap << "\n";
        }
    }
    else {
        std::cerr << "Warning: Unknown configuration key: " << key << "\n";
    }
}

std::string ConfigManager::trim(const std::string& str) const {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

std::string ConfigManager::toLower(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool ConfigManager::isValidViewMode(const std::string& view) const {
    std::string lowerView = toLower(view);
    return (lowerView == "list" || lowerView == "grid");
}

bool ConfigManager::isValidPositiveInt(const std::string& str, int& result) const {
    try {
        result = std::stoi(str);
        return result > 0;
    } catch (const std::exception&) {
        return false;
    }
}