// "colorpickercontrol": hue, saturation, brightness and alpha.
#pragma once
#include "plasma/Widget.h"
class Workspace;

class ColorPickerWidget : public plasma::Widget {
public:
    ColorPickerWidget(plasma::Engine *engine, Workspace *workspace, plasma::Node *node);
    virtual ~ColorPickerWidget();
    virtual void draw();
    virtual bool contains(const plasma::Vector<2, float> &point);
    virtual void pointerPressed();
    virtual void pointerReleased();
    virtual void pointerMoved();
    virtual void colorChanged();
    plasma::Vector<4, float> getColor();

    Workspace *m_workspace;
    bool m_dragSaturationBrightness;
    bool m_dragHue;
    bool m_dragAlpha;
    plasma::Vector<4, float> m_hsv; // Initially 240 degrees hue, 100% saturation and brightness, and alpha 1.
    int m_barWidth;                 // Initially 15 display units.
};
