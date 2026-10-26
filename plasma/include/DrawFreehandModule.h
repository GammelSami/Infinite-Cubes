// The "module:freehand:width" toolbar and its "freehand:*" controls.
#pragma once
#include "Module.h"

class DrawFreehandModule : public Module {
public:
    DrawFreehandModule();
    virtual void build();
    virtual void unitChanged();
    void choosePressure();
    void chooseCorners();
    void setWidth(float width);
    void updateWidth();
    void editWidth();
    void dragWidth(plasma::Widget *sender);
    void stepWidth(plasma::Widget *sender);
    void updateSimplification();
    void editSimplification();
    void dragSimplification(plasma::Widget *sender);
    void stepSimplification(plasma::Widget *sender);

    float m_width;          // "freehand:width".
    float m_simplification; // "freehand:simplification".
    bool m_pressureStroke;  // "freehand:pressure:stroke".
    bool m_pressureOpacity; // "freehand:pressure:opacity".
    bool m_corners;         // "freehand:corners".
};

