#pragma once

#include "DesktopApp.hpp"
#include <vector>
#include <memory>
#include <mutex>
#include <algorithm>
#include <thread>
#include <future>

class AppDatabase {
public:
    AppDatabase() {
        // Start loading in background
        m_loadFuture = std::async(std::launch::async, [this] {
            loadApps();
        });
    }
    
    ~AppDatabase() {
        if (m_loadFuture.valid()) {
            m_loadFuture.wait();
        }
    }
    
    bool isLoaded() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_loaded;
    }
    
    void waitForLoad() {
        if (m_loadFuture.valid()) {
            m_loadFuture.wait();
        }
    }
    
    std::vector<DesktopApp> getAllApps() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_allApps;
    }
    
    std::vector<DesktopApp> filterApps(const std::string& query, 
                                       const std::string& category = "") const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (query.empty() && category.empty()) {
            return m_allApps;
        }
        
        std::string lowerQuery = query;
        std::transform(lowerQuery.begin(), lowerQuery.end(), 
                       lowerQuery.begin(), ::tolower);
        
        std::vector<DesktopApp> filtered;
        for (const auto& app : m_allApps) {
            bool matches = true;
            
            if (!query.empty()) {
                std::string lowerName = app.name;
                std::transform(lowerName.begin(), lowerName.end(), 
                             lowerName.begin(), ::tolower);
                
                std::string lowerComment = app.comment;
                std::transform(lowerComment.begin(), lowerComment.end(), 
                             lowerComment.begin(), ::tolower);
                
                matches = (lowerName.find(lowerQuery) != std::string::npos) ||
                         (lowerComment.find(lowerQuery) != std::string::npos);
            }
            
            if (!category.empty() && matches) {
                matches = (app.categories.find(category) != std::string::npos);
            }
            
            if (matches) {
                filtered.push_back(app);
            }
        }
        
        return filtered;
    }
    
    std::vector<std::string> getAllCategories() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::vector<std::string> categories;
        for (const auto& app : m_allApps) {
            size_t start = 0;
            while (start < app.categories.length()) {
                size_t end = app.categories.find(';', start);
                std::string category = app.categories.substr(
                    start, end - start);
                
                if (!category.empty() && 
                    std::find(categories.begin(), categories.end(), 
                             category) == categories.end()) {
                    categories.push_back(category);
                }
                
                if (end == std::string::npos) break;
                start = end + 1;
            }
        }
        
        std::sort(categories.begin(), categories.end());
        return categories;
    }
    
    void reload() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_allApps.clear();
        m_loaded = false;
        
        m_loadFuture = std::async(std::launch::async, [this] {
            loadApps();
        });
    }
    
    size_t count() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_allApps.size();
    }

private:
    void loadApps() {
        std::vector<DesktopApp> apps;
        
        std::vector<fs::path> desktopDirs = {
            "/usr/share/applications",
            "/usr/local/share/applications",
            fs::path(std::getenv("HOME")) / ".local/share/applications"
        };
        
        // Add flatpak directories if they exist
        auto flatpakDir = fs::path(std::getenv("HOME")) / ".local/share/flatpak/exports/share/applications";
        if (fs::exists(flatpakDir)) {
            desktopDirs.push_back(flatpakDir);
        }
        
        flatpakDir = fs::path("/var/lib/flatpak/exports/share/applications");
        if (fs::exists(flatpakDir)) {
            desktopDirs.push_back(flatpakDir);
        }
        
        for (const auto& dir : desktopDirs) {
            if (fs::exists(dir)) {
                loadAppsFromDirectory(dir, apps);
            }
        }
        
        std::sort(apps.begin(), apps.end());
        
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_allApps = std::move(apps);
            m_loaded = true;
        }
        
        std::cout << "AppDatabase: Loaded " << m_allApps.size() 
                  << " applications" << std::endl;
    }
    
    void loadAppsFromDirectory(const fs::path& directory, 
                               std::vector<DesktopApp>& apps) {
        try {
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (entry.path().extension() == ".desktop") {
                    auto app = DesktopAppParser::parseDesktopFile(entry.path());
                    if (app) {
                        apps.push_back(*app);
                    }
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Warning: Could not read directory " 
                      << directory << ": " << e.what() << std::endl;
        }
    }
    
    mutable std::mutex m_mutex;
    std::vector<DesktopApp> m_allApps;
    bool m_loaded = false;
    std::future<void> m_loadFuture;
};