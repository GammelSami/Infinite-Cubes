// EdgeTool: the tool that selects and moves the edges of the selected mesh shapes (type 3 in the tool module,
// "tool:edge"). Its context menu offers "Delete Edges" and "Open Edge".
#pragma once
#include "BoundingBoxTool.h"

class EdgeTool : public BoundingBoxTool {
public:
    EdgeTool();

    virtual void pointerMoved(int, int);
    virtual void pointerPressed();
    virtual void pointerReleased();

    // Deletes the selected edges (the context menu's "Delete Edges").
    virtual void deleteSelected();
    virtual void draw();

    // Fills the context menu.
    virtual void fillContextMenu();
    virtual bool getSnapOffsets(const plasma::Vector<2, float> &point, std::list<plasma::Vector<2, float>> &offsets);
    virtual bool isRotating();
    virtual void beginBoxChange();
    virtual void endBoxChange();
    virtual void setBox(const plasma::Vector<2, float> &boxMin, const plasma::Vector<2, float> &boxMax);
    virtual void rotateBox(float rotation);

    // Resets the box, grows it over the selected edges of the selected shapes and places the handles.
    void updateBox();

    // "Open Edge".
    void openEdge();
};
