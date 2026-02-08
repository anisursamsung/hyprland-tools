// src/AppItem.hpp - Fixed version
#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/system/Icons.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <memory>
#include <mutex>
#include <functional>
#include <iostream>
#include "DesktopApp.hpp"
#include "IconCache.hpp"

using namespace Hyprutils::Memory;
using namespace Hyprtoolkit;

class AppItem {
public:
    AppItem(const DesktopApp& app, CSharedPointer<IBackend> backend)
        : m_app(app), m_backendWeak(backend) {
        createPlaceholderUI();
        loadIconAsync();
    }

    ~AppItem() {
        cancelIconLoad();
    }

    CSharedPointer<IElement> getElement() const { 
        return m_background; 
    }
    
    float getHeight() const { 
        return ITEM_HEIGHT; 
    }

    void setActive(bool active) {
        if (m_active == active) return;
        m_active = active;
        updateColors();
        scheduleUIUpdate();
    }

    bool isActive() const { 
        return m_active; 
    }
    
    const DesktopApp& getApp() const { 
        return m_app; 
    }
    
    void launch() const {
        std::string command = m_app.cleanExecCommand();
        
        if (m_app.terminal) {
            // Find a terminal emulator
            std::vector<std::string> terminals = {
                "foot", "kitty", "alacritty", "wezterm", "konsole",
                "gnome-terminal", "xfce4-terminal", "terminator"
            };
            
            std::string terminal;
            for (const auto& term : terminals) {
                if (std::system(("which " + term + " > /dev/null 2>&1").c_str()) == 0) {
                    terminal = term;
                    break;
                }
            }
            
            if (!terminal.empty()) {
                command = terminal + " -e " + command;
            }
        }
        
        command += " &";
        std::system(command.c_str());
    }
    
    void scheduleUIUpdate() {
        auto backend = m_backendWeak.lock();
        if (!backend) return;
        
        backend->addIdle([this] {
            updateAppearance();
        });
    }

private:
    static constexpr float ITEM_HEIGHT = 52.F;
    
    void createPlaceholderUI() {
        auto backend = m_backendWeak.lock();
        if (!backend) return;
        
        auto palette = backend->getPalette();
        if (!palette) return;
        
        updateColors();
        
        // Create background
        m_background = CRectangleBuilder::begin()
            ->color([this] { return m_backgroundColor; })
            ->rounding(palette->m_vars.smallRounding)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {1.0F, ITEM_HEIGHT}))
            ->commence();

        // Create layout
        m_rowLayout = CRowLayoutBuilder::begin()
            ->gap(12)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                CDynamicSize::HT_SIZE_PERCENT,
                                {1.0F, 1.0F}))
            ->commence();
        m_rowLayout->setMargin(10);

        // Placeholder icon
        m_placeholderIcon = CRectangleBuilder::begin()
            ->color([palette] { return palette->m_colors.alternateBase.darken(0.1); })
            ->rounding(4)
            ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                CDynamicSize::HT_SIZE_ABSOLUTE,
                                {32.F, 32.F}))
            ->commence();

        // Placeholder text
        m_placeholderText = CTextBuilder::begin()
            ->text("Loading...")
            ->color([palette] { return palette->m_colors.text; })
            ->fontSize(CFontSize(CFontSize::HT_FONT_SMALL, 0.9))
            ->commence();

        m_rowLayout->addChild(m_placeholderIcon);
        m_rowLayout->addChild(m_placeholderText);
        m_background->addChild(m_rowLayout);
    }

    void loadIconAsync() {
        auto backend = m_backendWeak.lock();
        if (!backend) return;
        
        // Check cache first
        std::string iconPath = IconCache::instance().getIconPath(m_app.icon);
        
        backend->addIdle([this, iconPath] {
            updateIconElement(iconPath);
            updateTextElement();
        });
    }
    
    void cancelIconLoad() {
        std::lock_guard<std::mutex> lock(m_iconMutex);
        m_iconLoading = false;
    }

    void updateIconElement(const std::string& iconPath) {
        std::lock_guard<std::mutex> lock(m_iconMutex);
        
        auto backend = m_backendWeak.lock();
        if (!backend) return;
        
        CSharedPointer<IElement> newIcon;
        
        if (!iconPath.empty() && fs::exists(iconPath)) {
            // Load from file
            newIcon = CImageBuilder::begin()
                ->path(std::string{iconPath})
                ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                    CDynamicSize::HT_SIZE_ABSOLUTE,
                                    {32.F, 32.F}))
                ->fitMode(eImageFitMode::IMAGE_FIT_MODE_CONTAIN)
                ->sync(false)
                ->rounding(4)
                ->commence();
        } else {
            // Try system icon
            auto icons = backend->systemIcons();
            if (icons) {
                auto iconHandle = icons->lookupIcon(m_app.icon);
                if (iconHandle && iconHandle->exists()) {
                    newIcon = CImageBuilder::begin()
                        ->icon(iconHandle)
                        ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                            CDynamicSize::HT_SIZE_ABSOLUTE,
                                            {32.F, 32.F}))
                        ->fitMode(eImageFitMode::IMAGE_FIT_MODE_CONTAIN)
                        ->sync(false)
                        ->rounding(4)
                        ->commence();
                }
            }
        }
        
        // Fallback to placeholder
        if (!newIcon) {
            auto palette = backend->getPalette();
            if (palette) {
                auto placeholderColor = palette->m_colors.alternateBase.darken(0.1);
                newIcon = CRectangleBuilder::begin()
                    ->color([placeholderColor] { return placeholderColor; })
                    ->rounding(4)
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                        CDynamicSize::HT_SIZE_ABSOLUTE,
                                        {32.F, 32.F}))
                    ->commence();
            }
        }
        
        // Replace placeholder with actual icon
        if (newIcon && m_rowLayout) {
            m_rowLayout->removeChild(m_placeholderIcon);
            m_iconElement = newIcon;
            m_rowLayout->addChild(m_iconElement);
        }
    }
    
    void updateTextElement() {
        auto backend = m_backendWeak.lock();
        if (!backend || !m_rowLayout) return;
        
        // Remove placeholder text
        m_rowLayout->removeChild(m_placeholderText);
        
        // Create actual text
        m_text = CTextBuilder::begin()
            ->text(std::string{m_app.name})
            ->color([this] { return m_textColor; })
            ->commence();
        
        m_rowLayout->addChild(m_text);
        
        // Add comment if available
        if (!m_app.comment.empty()) {
            auto palette = backend->getPalette();
            if (palette) {
                auto commentColor = palette->m_colors.text.mix(
                    palette->m_colors.background, 0.5);
                
                m_comment = CTextBuilder::begin()
                    ->text(" - " + m_app.comment)
                    ->color([commentColor] { return commentColor; })
                    ->fontSize(CFontSize(CFontSize::HT_FONT_SMALL, 0.8))
                    ->commence();
                
                m_rowLayout->addChild(m_comment);
            }
        }
    }

    void updateColors() {
        auto backend = m_backendWeak.lock();
        if (!backend) {
            // Fallback colors
            m_textColor = CHyprColor(1, 1, 1, 1);
            m_backgroundColor = m_active ? 
                CHyprColor(0.2, 0.4, 0.8, 1) : 
                CHyprColor(0.3, 0.3, 0.3, 1);
            return;
        }
        
        auto palette = backend->getPalette();
        if (!palette) return;
        
        auto& colors = palette->m_colors;
        
        if (m_active) {
            m_textColor = colors.brightText;
            m_backgroundColor = colors.accent;
        } else {
            m_textColor = colors.text;
            m_backgroundColor = colors.base;
        }
    }

    void updateAppearance() {
        if (!m_background || !m_text) return;
        
        // Update background color
        auto bgBuilder = m_background->rebuild();
        if (bgBuilder) {
            bgBuilder->color([this] { return m_backgroundColor; })->commence();
        }
        
        // Update text color
        auto textBuilder = m_text->rebuild();
        if (textBuilder) {
            textBuilder->color([this] { return m_textColor; })->commence();
        }
        
        // Force reposition
        m_background->forceReposition();
    }

    DesktopApp m_app;
    bool m_active = false;
    bool m_iconLoading = true;
    
    CWeakPointer<IBackend> m_backendWeak;
    
    CSharedPointer<CRectangleElement> m_background;
    CSharedPointer<CRowLayoutElement> m_rowLayout;
    CSharedPointer<CTextElement> m_text;
    CSharedPointer<CTextElement> m_comment;
    CSharedPointer<IElement> m_iconElement;
    CSharedPointer<IElement> m_placeholderIcon;
    CSharedPointer<CTextElement> m_placeholderText;
    
    CHyprColor m_textColor;
    CHyprColor m_backgroundColor;
    
    mutable std::mutex m_iconMutex;
};