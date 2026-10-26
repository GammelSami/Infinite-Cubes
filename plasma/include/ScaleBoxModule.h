// ScaleBoxModule: the editor's scale box ("module:scalebox"): the position and size of the bounding box tool's box
// ("scalebox:x", "scalebox:y", "scalebox:width", "scalebox:height"), its rotation ("scalebox:rotation", applied by
// "scalebox:rotate"), and the panel's expansion ("scalebox:expand").
#pragma once
#include "Module.h"
#include "FormatModule.h"

class BoundingBoxTool;

class ScaleBoxModule : public Module {
public:
    ScaleBoxModule();

    virtual void build();
    virtual void currentNodeChanged() { update(); }
    virtual void unitChanged() { update(); }

    // Shows the box of the bounding box tool, or blanks and disables the edit boxes when the tool has no box.
    void update();
    // Moves the panel towards expanded or collapsed (Workspace::Update); true while it moves.
    bool updateExpansion();

    // Each coordinate has an edit box (event 19), a drag button ("...:drag", event 12; event 3 ends the drag) and
    // steps (events 2 and 4).
    void editX(); // "scalebox:x".
    void dragX(plasma::Widget *sender);
    void stepX(plasma::Widget *sender);
    void editY(); // "scalebox:y".
    void dragY(plasma::Widget *sender);
    void stepY(plasma::Widget *sender);
    void editWidth(); // "scalebox:width".
    void dragWidth(plasma::Widget *sender);
    void stepWidth(plasma::Widget *sender);
    void editHeight(); // "scalebox:height".
    void dragHeight(plasma::Widget *sender);
    void stepHeight(plasma::Widget *sender);
    void editRotation(); // "scalebox:rotation".
    void dragRotation(plasma::Widget *sender);
    void stepRotation(plasma::Widget *sender);
    void rotate();       // "scalebox:rotate".
    void endDrag();      // Event 3 of the drag buttons.
    void chooseExpand(); // "scalebox:expand".

    float getExpansion() const { return m_expansion; }

    plasma::Vector<2, float> m_boxPosition; // The box when a drag or step began.
    plasma::Vector<2, float> m_boxSize;
    int m_expandDirection; // -1 expanding, 1 collapsing, 0 at rest.
    float m_expansion;     // 1 expanded.
    float m_rotation;      // Degrees.
};
