#pragma once

#include <string>
#include <vector>
#include <optional>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <iostream>  // ADD THIS

namespace fs = std::filesystem;

struct DesktopApp {
    std::string name;
    std::string exec;
    std::string icon;
    std::string comment;
    std::string categories;
    std::string desktopFile;
    bool terminal = false;
    bool noDisplay = false;
    bool hidden = false;
    
    bool operator<(const DesktopApp& other) const {
        std::string aName = name;
        std::string bName = other.name;
        std::transform(aName.begin(), aName.end(), aName.begin(), ::tolower);
        std::transform(bName.begin(), bName.end(), bName.begin(), ::tolower);
        return aName < bName;
    }
    
    std::string cleanExecCommand() const {
        std::string result = exec;
        
        // Remove desktop file field codes
        size_t pos = 0;
        while ((pos = result.find('%', pos)) != std::string::npos) {
            if (pos + 1 < result.length()) {
                char code = result[pos + 1];
                if (code == 'f' || code == 'F' || code == 'u' || code == 'U' ||
                    code == 'd' || code == 'D' || code == 'n' || code == 'N' ||
                    code == 'i' || code == 'c' || code == 'k' || code == 'v' ||
                    code == 'm') {
                    result.erase(pos, 2);
                } else {
                    pos += 2;
                }
            } else {
                result.erase(pos, 1);
                break;
            }
        }
        
        // Trim whitespace
        result.erase(0, result.find_first_not_of(" \t"));
        result.erase(result.find_last_not_of(" \t") + 1);
        
        return result;
    }
};

class DesktopAppParser {
public:
    static std::optional<DesktopApp> parseDesktopFile(const fs::path& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return std::nullopt;
        
        DesktopApp app;
        app.desktopFile = filepath.string();
        
        std::string line;
        bool inDesktopEntry = false;
        bool hasMainSection = false;
        
        while (std::getline(file, line)) {
            // Remove comments and trim
            if (line.find('#') != std::string::npos) {
                line = line.substr(0, line.find('#'));
            }
            
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            if (line.empty()) continue;
            
            if (line == "[Desktop Entry]") {
                inDesktopEntry = true;
                hasMainSection = true;
                continue;
            } else if (line[0] == '[') {
                inDesktopEntry = false;
                continue;
            }
            
            if (!inDesktopEntry) continue;
            
            size_t equalsPos = line.find('=');
            if (equalsPos == std::string::npos) continue;
            
            std::string key = line.substr(0, equalsPos);
            std::string value = line.substr(equalsPos + 1);
            
            if (key == "Name") {
                app.name = value;
            } else if (key == "Exec") {
                app.exec = value;
            } else if (key == "Icon") {
                app.icon = value;
            } else if (key == "Comment") {
                app.comment = value;
            } else if (key == "Categories") {
                app.categories = value;
            } else if (key == "Terminal") {
                app.terminal = (value == "true");
            } else if (key == "NoDisplay") {
                app.noDisplay = (value == "true");
            } else if (key == "Hidden") {
                app.hidden = (value == "true");
            }
        }
        
        if (!hasMainSection || app.name.empty() || app.exec.empty() || 
            app.noDisplay || app.hidden) {
            return std::nullopt;
        }
        
        return app;
    }
};