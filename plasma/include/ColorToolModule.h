// The paint panel, identified by "module:color:tool" and "color:tool:*".
#pragma once
#include "Module.h"

class ColorToolModule : public Module {
public:
    ColorToolModule();
    virtual void build();
    void chooseColorAlpha();
    void chooseColorOnly();
    void chooseAlpha();
    void chooseColorize();
    void editTolerance();
    void dragTolerance(plasma::Widget *sender);
    void stepTolerance(plasma::Widget *sender);
    void updateTolerance();

    bool m_colorize;   // "color:tool:colorize".
    bool m_alpha;      // "color:tool:alpha".
    bool m_color;      // "color:tool:color".
    float m_tolerance; // "color:tool:tolerance".
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
