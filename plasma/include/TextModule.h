// TextModule.cpp: the editor panel identified by "module:text" and "text:*" widgets.
#pragma once
#include <map>
#include <string>
#include <vector>
#include "Module.h"
#include "DesignSheet.h"
#include "plasma/TextShape.h"
#include "plasma/FontEngine.h"
#include "plasma/ListWidget.h"
#include "FontWidget.h"
#include "main.h" // NarrowString.

// A snapshot of a text shape's editable state.
struct TextState {
    TextState()
    {
        // Default colour vectors contain indeterminate components.
    }
    ~TextState();
    plasma::DiscreteAttribute<std::wstring> m_string;
    plasma::ContinuousAttribute<plasma::Vector<4, float>> m_color;
    plasma::ContinuousAttribute<plasma::Vector<4, float>> m_strokeColor;
    plasma::ContinuousAttribute<plasma::Vector<4, float>> m_extrusionColor;
    std::wstring m_fontName;
    float m_pixelSize;
    float m_strokeRadius;
    float m_spacing;
    float m_lineSpacing;
    float m_wrapWidth;
    int m_flags;
};

class TextModule : public Module {
public:
    TextModule();
    // The smaller axis scale of the sheet node's matrix relative to the sheet's root. It does not depend on the
    // module.
    float getTextScale(NodeInterface *node);
    virtual void build();
    virtual void updatePanel();
    virtual void showSelectedValues();
    virtual void currentNodeChanged();
    void chooseFontType();
    void reloadFont();
    void updateSize();
    void applySize();
    void editSize();
    void dragSize(plasma::Widget *sender);
    void stepSize(plasma::Widget *sender);
    void updateStroke();
    void applyStroke();
    void editStroke();
    void dragStroke(plasma::Widget *sender);
    void stepStroke(plasma::Widget *sender);
    void updateSpacing();
    void applySpacing();
    void editSpacing();
    void dragSpacing(plasma::Widget *sender);
    void stepSpacing(plasma::Widget *sender);
    void updateLineSpacing();
    void applyLineSpacing();
    void editLineSpacing();
    void dragLineSpacing(plasma::Widget *sender);
    void stepLineSpacing(plasma::Widget *sender);
    void updateWrapWidth();
    void applyWrapWidth();
    void editWrapWidth();
    void dragWrapWidth(plasma::Widget *sender);
    void stepWrapWidth(plasma::Widget *sender);
    void applyVerticalAlignment();
    void applyHorizontalAlignment();
    void chooseLeft();
    void applyLeft();
    void chooseCenter();
    void applyCenter();
    void chooseRight();
    void applyRight();
    void chooseTop();
    void applyTop();
    void chooseMiddle();
    void applyMiddle();
    void chooseBottom();
    void applyBottom();
    void applyWrap();
    void pick();
    void apply();
    void pickNode(plasma::Node *node);
    void clearFontSearch();
    void filterFonts();
    void findFonts();
    void addFontDirectory(const std::wstring &path);
    void selectFont(const std::wstring &path, bool apply);
    const std::wstring &getFontName();
    // MouseMove reads these values before applying the sheet's zoom.
    float getSize() const { return m_pixelSize; }
    void setSize(float size);
    float getStrokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(float width);
    void chooseWrap();
    // TextTool uses these settings for new text.
    float getWrapWidth() const { return m_wrapWidth; }
    bool getWrap() const { return m_wrap; }

    std::vector<std::wstring> m_filteredFonts;
    std::map<std::wstring, std::wstring> m_fontCaptions;
    std::vector<std::wstring> m_fonts;
    std::wstring m_fontName;
    FontWidget *m_fontWidget;  // The font popup's list.
    float m_pixelSize;         // "text:size".
    float m_strokeWidth;       // "text:stroke": the stroke width, twice the shapes' m_strokeRadius.
    float m_spacing;           // "text:spacing".
    float m_lineSpacing;       // "text:line".
    float m_wrapWidth;         // "text:wrap:width".
    int m_verticalAlignment;   // plasma::TextAlignment.
    int m_horizontalAlignment; // plasma::TextAlignment.
    bool m_trueType;           // "text:type:truetype".
    bool m_wrap;               // "text:wrap".
};

// Converts a byte string to std::wstring, character by character.
std::wstring widenString(const std::string &text);

namespace plasma {
template<>
void Widget::setConnection<TextModule>(Connection *&connection, TextModule *object, void (TextModule::*method)(),
                                       bool enabledOnly);
}

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
