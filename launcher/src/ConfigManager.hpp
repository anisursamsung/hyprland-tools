#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <string>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

class ConfigManager {
public:
    // Constructor that loads config from file
    ConfigManager();
    
    // Getters for configuration values
    std::string getDefaultView() const { return defaultView; }
    int getColumnCount() const { return columnCount; }
    int getGridItemWidth() const { return gridItemWidth; }
    int getGridItemHeight() const { return gridItemHeight; }
    int getGridHorizontalGap() const { return gridHorizontalGap; }
    int getGridVerticalGap() const { return gridVerticalGap; }
    
    // Check if config was loaded successfully
    bool isLoaded() const { return configLoaded; }
    
private:
    // Default configuration values
    std::string defaultView = "list";
    int columnCount = 6;
    int gridItemWidth = 120;
    int gridItemHeight = 120;
    int gridHorizontalGap = 10;
    int gridVerticalGap = 10;
    
    bool configLoaded = false;
    
    // Helper methods
    void loadConfig();
    void parseConfigFile(const fs::path& configPath);
    void setValue(const std::string& key, const std::string& value);
    
    // Mark these as const since they don't modify the object
    std::string trim(const std::string& str) const;
    std::string toLower(const std::string& str) const;
    bool isValidViewMode(const std::string& view) const;
    bool isValidPositiveInt(const std::string& str, int& result) const;
};

#endif // CONFIG_MANAGER_HPP