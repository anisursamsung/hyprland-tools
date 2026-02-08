#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <filesystem>

namespace fs = std::filesystem;

class IconCache {
public:
    static IconCache& instance() {
        static IconCache cache;
        return cache;
    }

    std::string getIconPath(const std::string& iconName) {
        if (iconName.empty()) return "";
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Check cache first
        auto it = m_cache.find(iconName);
        if (it != m_cache.end()) {
            return it->second;
        }
        
        // Find icon
        std::string path = findIconSystem(iconName);
        
        // Cache result (even if empty)
        m_cache[iconName] = path;
        
        if (!path.empty()) {
            m_reverseCache[path] = iconName;
        }
        
        return path;
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache.clear();
        m_reverseCache.clear();
    }
    
    void preloadCommonIcons() {
        std::vector<std::string> commonIcons = {
            "application-x-executable",
            "system-run",
            "folder",
            "document",
            "image",
            "audio",
            "video",
            "text",
            "archive"
        };
        
        for (const auto& icon : commonIcons) {
            getIconPath(icon);
        }
    }

private:
    IconCache() = default;
    ~IconCache() = default;
    
    std::string findIconSystem(const std::string& iconName) {
        // Check if it's already a path
        if (fs::path(iconName).is_absolute() && fs::exists(iconName)) {
            return iconName;
        }
        
        // Check common icon directories
        std::vector<fs::path> iconDirs = {
            "/usr/share/pixmaps",
            "/usr/share/icons/hicolor/48x48/apps",
            "/usr/share/icons/hicolor/scalable/apps",
            "/usr/share/icons/Adwaita/48x48/apps",
            "/usr/share/icons/Adwaita/scalable/apps",
            fs::path(std::getenv("HOME")) / ".local/share/icons/hicolor/48x48/apps",
            fs::path(std::getenv("HOME")) / ".local/share/icons"
        };
        
        std::vector<std::string> extensions = {"", ".png", ".svg", ".jpg", ".jpeg", ".xpm"};
        
        for (const auto& dir : iconDirs) {
            if (!fs::exists(dir)) continue;
            
            for (const auto& ext : extensions) {
                fs::path iconPath = dir / (iconName + ext);
                if (fs::exists(iconPath)) {
                    return iconPath.string();
                }
            }
        }
        
        return "";
    }
    
    std::unordered_map<std::string, std::string> m_cache;
    std::unordered_map<std::string, std::string> m_reverseCache;
    std::mutex m_mutex;
};