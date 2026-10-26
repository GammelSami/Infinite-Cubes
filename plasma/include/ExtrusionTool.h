// ExtrusionTool: the tool that deforms the extrusion of the selected mesh shapes (type 13 in the tool module,
// "tool:extrusion"). Its context menu offers "Reset Extrusion Deformation".
#pragma once
#include "BoundingBoxTool.h"

class ExtrusionTool : public BoundingBoxTool {
public:
    ExtrusionTool();

    virtual void pointerMoved(int, int);
    virtual void pointerPressed();
    virtual void pointerReleased();
    virtual void rightButtonPressed();  // As PointerPressed.
    virtual void rightButtonReleased(); // As PointerReleased.
    virtual void draw();

    // Fills the context menu.
    virtual void fillContextMenu();

    void updateBox();

    // "Reset Extrusion Deformation".
    void resetDeformation();
};
