// VertexTool: the tool that selects and moves the vertices of the selected mesh shapes (type 2 in the tool module,
// "tool:vertex"). It drives the shape interfaces of the selected sheet nodes; its context menu offers "Delete",
// "Soft", "Sharp", "Bezier", "Arc", "Free", "Fixed on edge" and "Rounded corner".
#pragma once
#include "BoundingBoxTool.h"

class VertexTool : public BoundingBoxTool {
public:
    VertexTool();

    virtual void pointerMoved(int, int);
    virtual void pointerPressed();
    virtual void pointerReleased();

    // Deletes the selected vertices (the context menu's "Delete").
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

    // Resets the box, grows it over the selected vertices of the selected shapes and places the handles.
    void updateBox();

    // The context menu's vertex types, applied to the selected vertices.
    void setSoft();                     // ("Soft").
    void setSharp();                    // ("Sharp").
    void setBezier();                   // ("Bezier").
    void setArc();                      // ("Arc").
    void setFree();                     // ("Free").
    void setFixedOnEdge();              // ("Fixed on edge").
    void setRoundedCorner();            // ("Rounded corner").
    void applyDeformation();            // ("Apply Deformation to Vertices").
    void applyRotationAndDeformation(); // ("Apply Rotation and Deformation to Vertices").
};
