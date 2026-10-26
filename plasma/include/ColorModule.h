// The "module:color" panel; "color:fill", "color:stroke" and "color:extrusion".
#pragma once
#include <functional>
#include "TextModule.h"
#include "plasma/Node.h"
#include "plasma/Display.h"
#include "plasma/ListWidget.h"
#include "plasma/Edit.h"
#include "ColorPickerWidget.h"
#include "ShapeInterface.h" // ColorTarget.

namespace plasma {
template<> bool Vector<4, float>::operator!=(const Vector<4, float> &other) const;
}

typedef plasma::Vector<4, float> EditorColor;

enum RgbaComponent {
    RGBA_RED = 0,
    RGBA_GREEN = 1,
    RGBA_BLUE = 2,
    RGBA_ALPHA = 3
};
enum {
    RGB_COMPONENT_COUNT = 3,
    RGBA_COMPONENT_COUNT = 4
};

enum HsvComponent {
    HSV_HUE = 0,
    HSV_SATURATION = 1,
    HSV_BRIGHTNESS = 2
};

enum CmykComponent {
    CMYK_CYAN = 0,
    CMYK_MAGENTA = 1,
    CMYK_YELLOW = 2,
    CMYK_KEY = 3
};

class ColorModule : public Module {
public:
    ColorModule();
    virtual ~ColorModule();
    virtual void build();
    void chooseFill();
    void chooseStroke();
    void chooseExtrusion();
    void editHtml();
    void editRed();
    void dragRed(plasma::Widget *sender);
    void stepRed(plasma::Widget *sender);
    void editGreen();
    void dragGreen(plasma::Widget *sender);
    void stepGreen(plasma::Widget *sender);
    void editBlue();
    void dragBlue(plasma::Widget *sender);
    void stepBlue(plasma::Widget *sender);
    void editAlpha();
    void dragAlpha(plasma::Widget *sender);
    void stepAlpha(plasma::Widget *sender);
    void chooseColorAlpha();
    void chooseColorOnly();
    void chooseAlpha();
    int getMode();
    const EditorColor &getColor();
    void setColor(const EditorColor &color, bool updatePicker, bool preview, bool fromCmyk);
    void copyFill();
    void copyStroke();
    void copyExtrusion();
    void showSpecial();
    void showManagement();
    void linkColors();
    void editCmyk();
    void editHue();
    void editSaturation();
    void editBrightness();

    EditorColor m_fillColor;
    EditorColor m_strokeColor;
    EditorColor m_extrusionColor;
    ColorPickerWidget *m_colorWidget;
    plasma::Node *m_fillPanel;
    plasma::Node *m_strokePanel;
    plasma::Node *m_extrusionPanel;
    int m_mode;  // The ColorTarget being edited.
    bool m_link; // "color:link".
    bool m_pickAlpha;
    bool m_pickColor;
};

// Validates HTML colour input.
class HtmlColorFilter : public plasma::EditFilter {
public:
    virtual bool validate(const std::wstring &text);
};
extern HtmlColorFilter g_htmlColorFilter;

// The HSV (degrees, percent) of an RGB colour (MeshInterface.cpp).
EditorColor rgbToHsv(const EditorColor &color);

namespace plasma {
template<>
void Widget::setConnection<ColorModule>(Connection *&connection, ColorModule *object, void (ColorModule::*method)(),
                                        bool enabledOnly);
}
#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
