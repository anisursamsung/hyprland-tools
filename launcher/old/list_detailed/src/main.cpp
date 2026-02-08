#include "AppLauncher.hpp"
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <atomic>

std::atomic<bool> g_running{true};

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

int main(int argc, char** argv) {
    // Set up signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    try {
        std::cout << "=== Starting App Launcher ===" << std::endl;
        
        AppLauncher launcher;
        
        if (!launcher.isValid()) {
            std::cerr << "ERROR: Failed to initialize App Launcher!" << std::endl;
            
            // Check common issues
            if (!std::getenv("WAYLAND_DISPLAY")) {
                std::cerr << "  - Not running in a Wayland session" << std::endl;
                std::cerr << "  - Try running from a Wayland compositor like Hyprland or Sway" << std::endl;
            }
            
            return 1;
        }
        
        std::cout << "Initialization successful. Starting main loop..." << std::endl;
        
        launcher.run();
        
        std::cout << "=== App Launcher Exited ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n═══════════════════════════════════════════════════\n";
        std::cerr << "FATAL ERROR: " << e.what() << "\n";
        std::cerr << "═══════════════════════════════════════════════════\n";
        
        // Print backtrace if available
        #ifdef LAUNCHER_DEBUG
            std::cerr << "\nStack trace:\n";
            // Add your debug backtrace code here
        #endif
        
        return 1;
    }
}