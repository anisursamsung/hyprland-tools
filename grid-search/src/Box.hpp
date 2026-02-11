#pragma once
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <string>
#include <functional>

class Box {
  public:
    // Constructor with all customizable properties
    Box(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend,
        const std::string& title,
        const std::string& imagePath,
        Hyprtoolkit::CHyprColor boxColor = {0.15f, 0.15f, 0.15f, 1.0f},
        Hyprtoolkit::CHyprColor borderColor = {0.2f, 0.5f, 0.8f, 1.0f},
        Hyprtoolkit::CHyprColor textColor = {1.0f, 1.0f, 1.0f, 1.0f},
        float width = 220.0f,
        float height = 220.0f,
        int borderRadius = 10,
        int borderThickness = 1);
        
        
    // Getters for the elements
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> getElement() const { return m_background; }
    float getWidth() const { return m_width; }
    float getHeight() const { return m_height; }
    std::string getTitle() const { return m_title; }
    std::string getImagePath() const { return m_imagePath; }
    
    // Setters to modify properties
    void setTitle(const std::string& newTitle);
    void setImagePath(const std::string& newImagePath);
    void setBoxColor(Hyprtoolkit::CHyprColor newColor);
    void setBorderColor(Hyprtoolkit::CHyprColor newColor);
    void setTextColor(Hyprtoolkit::CHyprColor newColor);
    void setSize(float newWidth, float newHeight);
    void setBorderRadius(int newRadius);
    void setBorderThickness(int newThickness);
    
  private:
    // Properties
    std::string m_title;
    std::string m_imagePath;
    Hyprtoolkit::CHyprColor m_boxColor;
    Hyprtoolkit::CHyprColor m_borderColor;
    Hyprtoolkit::CHyprColor m_textColor;
    float m_width;
    float m_height;
    int m_borderRadius;
    int m_borderThickness;
    
    // UI Elements
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRectangleElement> m_background;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> m_mainLayout;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> m_contentLayout;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CImageElement> m_image;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement> m_text;
     Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    
    // Helper methods
    void createUI();
    void updateUI();
    void updateImage();
};