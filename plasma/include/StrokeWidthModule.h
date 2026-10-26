// The "module:stroke:width" panel, synchronized with the Freehand width.
#pragma once
#include "Module.h"

class StrokeWidthModule : public Module {
public:
    StrokeWidthModule();
    virtual void build();
    virtual void unitChanged();
    // The stroke width read by MouseMove before applying the sheet's zoom.
    float getWidth() const { return m_width; }
    void setWidth(float width);
    void updateWidth();
    void editWidth();
    void dragWidth(plasma::Widget *sender);
    void stepWidth(plasma::Widget *sender);

    float m_width; // "stroke:width".
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
