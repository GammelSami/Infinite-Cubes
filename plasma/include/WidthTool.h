// WidthTool: the tool that sets the stroke width of the selected shapes' vertices (type 11 in the tool module,
// "tool:width"). While Alt is held it picks the width under the pointer instead and restores the width on release.
#pragma once
#include "Tool.h"
#include "plasma/Vector.h"

class WidthTool : public Tool {
public:
    WidthTool();

    // Alt pressed: saves the stroke width module's width and picks.
    virtual void keyPressed(unsigned short key);

    // Alt released after picking: restores the saved width.
    virtual void keyReleased(unsigned short key);

    // The pointer moved: picks with Alt held and no button down, applies with a button down and no Alt.
    virtual void pointerMoved(int x, int y);
    virtual void pointerPressed(); // Begins a width stroke.

    // Ends the stroke by refreshing changed interfaces and committing the design sheet.
    virtual void pointerReleased();
    virtual void draw();            // The tool's icons.
    virtual void fillContextMenu(); // Radius menu.

    // Applies the stroke width module's width to the selected shapes near the pointer (with Shift, to all their
    // vertices).
    void applyStrokeWidth();

    // Picks the width of the vertex nearest the pointer into the stroke width module.
    void pickStrokeWidth();

protected:
    bool m_picking;                          // Alt is held.
    float m_savedWidth;                      // The width before picking.
    plasma::Vector<2, float> m_pickPosition; // The picked vertex.
};
