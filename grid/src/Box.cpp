#include "Box.hpp"
#include <iostream>

Box::Box(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend,
         const std::string& title,
         const std::string& imagePath,
         Hyprtoolkit::CHyprColor boxColor,
         Hyprtoolkit::CHyprColor borderColor,
         Hyprtoolkit::CHyprColor textColor,
         float width,
         float height,
         int borderRadius,
         int borderThickness)
    : m_title(title),              // m_title is declared first in Box.hpp
      m_imagePath(imagePath),      // then m_imagePath
      m_boxColor(boxColor),        // then m_boxColor
      m_borderColor(borderColor),  // then m_borderColor
      m_textColor(textColor),      // then m_textColor
      m_width(width),              // then m_width
      m_height(height),            // then m_height
      m_borderRadius(borderRadius), // then m_borderRadius
      m_borderThickness(borderThickness), // then m_borderThickness
      m_backend(backend) {         // m_backend is declared LAST in Box.hpp (line 61)
    createUI();
}

void Box::createUI() {
    // Create main column layout for entire box
    m_mainLayout = Hyprtoolkit::CColumnLayoutBuilder::begin()
        ->gap(0)
        ->size(Hyprtoolkit::CDynamicSize(
            Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
            Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
            {m_width - (m_borderThickness * 2), m_height - (m_borderThickness * 2)}))
        ->commence();
    
    // Create content layout for image and text
    m_contentLayout = Hyprtoolkit::CColumnLayoutBuilder::begin()
        ->gap(0)
        ->size(Hyprtoolkit::CDynamicSize(
            Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
            Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
            {1.0f, 1.0f}))
        ->commence();
    
    m_contentLayout->setMargin(10);
    
    // Create image element (top part)
    m_image = Hyprtoolkit::CImageBuilder::begin()
        ->path(std::string{m_imagePath})
        ->size(Hyprtoolkit::CDynamicSize(
            Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
            Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
            {1.0f, 0.9f}))  // Image takes 90% of content height
        ->fitMode(Hyprtoolkit::eImageFitMode::IMAGE_FIT_MODE_CONTAIN)
        ->rounding(m_borderRadius / 2)  // Half of box rounding
        ->sync(false)
        ->commence();
    
    std::string fontFamilyStr = "Sans Serif";
    if (m_backend) {
        auto palette = m_backend->getPalette();	
        if (palette && !palette->m_vars.fontFamily.empty()) {
            fontFamilyStr = palette->m_vars.fontFamily;
        }
    }
    
    // Create text label (bottom part) - takes remaining 10% space
    m_text = Hyprtoolkit::CTextBuilder::begin()
        ->text(std::string{m_title})
        ->color([this] { 
            // Check if we have backend and palette
            if (m_backend) {
                auto palette = m_backend->getPalette();
                if (palette) {
                    return palette->m_colors.text;  // Use palette text color
                }
            }
            // Fallback to original text color
            return m_textColor; 
        })
        ->fontFamily(std::move(fontFamilyStr))
        ->align(Hyprtoolkit::HT_FONT_ALIGN_CENTER)
        ->size(Hyprtoolkit::CDynamicSize(
            Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
            Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
            {1.0f, 0.1f}))  // Text takes 10% of content height
        ->commence();
    
    // Add image and text to content layout
    m_contentLayout->addChild(m_image);
    m_contentLayout->addChild(m_text);
    
    // Add content layout to main layout
    m_mainLayout->addChild(m_contentLayout);
    
    // Create background box
    m_background = Hyprtoolkit::CRectangleBuilder::begin()
        ->color([this] { 
            // Check if we have backend and palette
            if (m_backend) {
                auto palette = m_backend->getPalette();
                if (palette) {
                    return palette->m_colors.alternateBase;  // Use palette alternateBase
                }
            }
            // Fallback to original box color
            return m_boxColor; 
        })
        ->rounding(m_borderRadius)
        ->borderColor([this] { 
            // Check if we have backend and palette
            if (m_backend) {
                auto palette = m_backend->getPalette();
                if (palette) {
                    return palette->m_colors.accent;  // Use palette accent
                }
            }
            // Fallback to original border color
            return m_borderColor; 
        })
        ->borderThickness(m_borderThickness)
        ->size(Hyprtoolkit::CDynamicSize(
            Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
            Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
            {m_width, m_height}))
        ->commence();
    
    // Add main layout to background
    m_background->addChild(m_mainLayout);
}

void Box::updateUI() {
    // Rebuild background with updated properties
    if (auto builder = m_background->rebuild()) {
        builder
            ->color([this] { 
                if (m_backend) {
                    auto palette = m_backend->getPalette();
                    if (palette) {
                        return palette->m_colors.alternateBase;
                    }
                }
                return m_boxColor; 
            })
            ->rounding(m_borderRadius)
            ->borderColor([this] { 
                if (m_backend) {
                    auto palette = m_backend->getPalette();
                    if (palette) {
                        return palette->m_colors.accent;
                    }
                }
                return m_borderColor; 
            })
            ->borderThickness(m_borderThickness)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                {m_width, m_height}))
            ->commence();
    }
    
    // Rebuild text with updated properties
    if (auto builder = m_text->rebuild()) {
        std::string fontFamilyStr = "Sans Serif";
        if (m_backend) {
            auto palette = m_backend->getPalette();
            if (palette && !palette->m_vars.fontFamily.empty()) {
                fontFamilyStr = palette->m_vars.fontFamily;
            }
        }
        
        builder
            ->text(std::string{m_title})
            ->color([this] { 
                if (m_backend) {
                    auto palette = m_backend->getPalette();
                    if (palette) {
                        return palette->m_colors.text;
                    }
                }
                return m_textColor; 
            })
            ->fontFamily(std::move(fontFamilyStr))
            ->align(Hyprtoolkit::HT_FONT_ALIGN_CENTER)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 0.1f}))  // Text takes 10% of content height
            ->commence();
    }
    
    // Update main layout size
    if (auto builder = m_mainLayout->rebuild()) {
        builder
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                Hyprtoolkit::CDynamicSize::HT_SIZE_ABSOLUTE,
                {m_width - (m_borderThickness * 2), m_height - (m_borderThickness * 2)}))
            ->commence();
    }
    
    // Update image size and rounding
    if (auto builder = m_image->rebuild()) {
        builder
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 0.9f}))  // Image takes 90% of content height
            ->rounding(m_borderRadius / 2)
            ->commence();
    }
    
    updateImage();
    m_background->forceReposition();
}

void Box::updateImage() {
    if (auto builder = m_image->rebuild()) {
        builder
            ->path(std::string{m_imagePath})
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0f, 0.9f}))  // Image takes 90% of content height
            ->fitMode(Hyprtoolkit::eImageFitMode::IMAGE_FIT_MODE_CONTAIN)
            ->commence();
    }
}

// Setter implementations
void Box::setTitle(const std::string& newTitle) {
    m_title = newTitle;
    updateUI();
}

void Box::setImagePath(const std::string& newImagePath) {
    m_imagePath = newImagePath;
    updateUI();
}

void Box::setBoxColor(Hyprtoolkit::CHyprColor newColor) {
    m_boxColor = newColor;
    updateUI();
}

void Box::setBorderColor(Hyprtoolkit::CHyprColor newColor) {
    m_borderColor = newColor;
    updateUI();
}

void Box::setTextColor(Hyprtoolkit::CHyprColor newColor) {
    m_textColor = newColor;
    updateUI();
}

void Box::setSize(float newWidth, float newHeight) {
    m_width = newWidth;
    m_height = newHeight;
    updateUI();
}

void Box::setBorderRadius(int newRadius) {
    m_borderRadius = newRadius;
    updateUI();
}

void Box::setBorderThickness(int newThickness) {
    m_borderThickness = newThickness;
    updateUI();
}