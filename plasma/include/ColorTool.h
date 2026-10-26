// ColorTool: the tool that paints the selected shapes' vertices with the colour module's colour (type 5 in the
// tool module, "tool:color"). While Alt is held it picks the colour under the pointer instead and restores the
// colour on release.
#pragma once
#include "Tool.h"
#include "plasma/Vector.h"

class NodeInterface;

class ColorTool : public Tool {
public:
    ColorTool();

    virtual void keyPressed(unsigned short key);
    virtual void keyReleased(unsigned short key);
    virtual void pointerMoved(int, int); // Paints while dragging.
    virtual void pointerPressed();       // Begins a colour stroke.

    // Ends the stroke: refreshes the changed interfaces and commits the design sheet.
    virtual void pointerReleased();
    virtual void draw();            // The tool's icon.
    virtual void fillContextMenu(); // Colour tool menu.

    // Picks the colour of the shape under the pointer into the colour module (as DrawFreehandTool::PickColor).
    void pickColor(bool preview);

    void applyColor();
    void colorize(int colorTarget);

    void flattenTransparency();
    static void flattenSheetNode(NodeInterface *node);

protected:
    bool m_picking;                          // Alt is held.
    plasma::Vector<4, float> m_savedColor;   // The colour before picking.
    plasma::Vector<2, float> m_pickPosition; // Nearest picked vertex.
};
