// The "Drop Shadow" panel (widget name "module:procedural").
#pragma once
#include "Module.h"
#include "plasma/Node.h"
#include "plasma/ContinuousAttribute.h"

class ShadowModule : public Module {
public:
    ShadowModule();
    virtual void build();
    virtual void unitChanged();
    void close();
    void editOffsetX();
    void dragOffsetX(plasma::Widget *sender);
    void stepOffsetX(plasma::Widget *sender);
    void editOffsetY();
    void dragOffsetY(plasma::Widget *sender);
    void stepOffsetY(plasma::Widget *sender);
    void editBlur();
    void dragBlur(plasma::Widget *sender);
    void stepBlur(plasma::Widget *sender);
    void chooseShadowColor();
    void apply();

    float m_offsetX;                  // "shadow:offset:x".
    float m_offsetY;                  // "shadow:offset:y".
    float m_blur;                     // "shadow:blur".
    plasma::Vector<4, float> m_color; // "shadow:color".
    plasma::Node *m_colorPanel;       // "shadow:color:colorpanel".
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
