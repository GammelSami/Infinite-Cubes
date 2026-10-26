// StampTool: the tool that stamps copies of the clipboard's shapes (type 15 in the tool module, "tool:stamp"),
// scaled, sheared and rotated by the stamp module.
#pragma once
#include "Tool.h"
#include "plasma/Matrix.h"

class StampTool : public Tool {
public:
    StampTool();

    // Button pressed: stamps once (with copies on the clipboard) and restarts the distance count.
    virtual void pointerPressed();

    // Ends the stamping: refreshes the pointer and commits the design sheet.
    virtual void pointerReleased();

    virtual void rightButtonPressed();
    virtual void pointerMoved(int x, int y);
    virtual void draw();

    // Alt swaps moving for rotating.
    virtual bool isRotating();

    virtual bool getSnapOffsets(const plasma::Vector<2, float> &point, std::list<plasma::Vector<2, float>> &offsets);

    // Bounds of the transformed clipboard trees.
    void getBounds(plasma::Vector<2, float> &minimum, plasma::Vector<2, float> &maximum);

    // Clears the stamp's offset and transformation (ClipboardModule::Copy).
    void reset();

    // Stamps the clipboard's instance copies at the pointer.
    void stamp();

protected:
    // Zeroed with the matrix by the constructor and Reset; otherwise unused.
    float m_unusedFloat;
    plasma::Matrix<float> m_matrix; // Identity after construction.

    // Accumulated by PointerMoved as the pointer moves; a stamp is placed once it reaches the spacing.
    float m_distance;
    plasma::Vector<2, float> m_dragOrigin; // Transformed drag origin.
};
