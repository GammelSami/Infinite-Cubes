// The five-column "palettewidget" control of module:palette.
#pragma once
#include "plasma/Widget.h"
class Workspace;

class PaletteWidget : public plasma::Widget {
public:
    PaletteWidget(plasma::Engine *engine, Workspace *workspace, plasma::Node *node);
    virtual ~PaletteWidget();
    virtual void draw();
    virtual plasma::Vector<2, float> getMaximum();
    virtual bool hasBounds();
    virtual bool contains(const plasma::Vector<2, float> &point);
    virtual void pointerPressed();
    virtual void pointerReleased();
    virtual void pointerDoubleClicked();
    // The palette does not move or resize by dragging.
    virtual void pointerMoved() {}
    virtual void colorChanged();

    // The palette is laid out in rows of this many colours.
    enum {
        COLUMNS = 5
    };

    void refresh();

    Workspace *m_workspace;
    int m_cellWidth;  // Initially 15.
    int m_cellHeight; // Initially 15.
};
