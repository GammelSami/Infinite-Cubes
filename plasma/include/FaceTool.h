// FaceTool: the tool that selects and moves the faces of the selected mesh shapes (type 4 in the tool module,
// "tool:face"). Its context menu offers "Delete Faces" and "Detach Faces".
#pragma once
#include "BoundingBoxTool.h"

class FaceTool : public BoundingBoxTool {
public:
    FaceTool();

    virtual void pointerMoved(int, int);
    virtual void pointerPressed();
    virtual void pointerReleased();

    // Deletes the selected faces (the context menu's "Delete Faces").
    virtual void deleteSelected();
    virtual void draw();

    // Fills the context menu.
    virtual void fillContextMenu();

    // BoundingBoxTool::GetSnapOffsets, only while the selection box is enabled.
    virtual bool getSnapOffsets(const plasma::Vector<2, float> &point, std::list<plasma::Vector<2, float>> &offsets);

    // Whether VK_MENU is held with VertexModule::EDIT_MOVE, or not held with EDIT_ROTATE.
    virtual bool isRotating();
    virtual void beginBoxChange();
    virtual void endBoxChange();
    virtual void setBox(const plasma::Vector<2, float> &boxMin, const plasma::Vector<2, float> &boxMax);
    virtual void rotateBox(float rotation);

    // Resets the box, grows it over the selected faces of the selected shapes and places the handles.
    void updateBox();

    // "Detach Faces".
    void detachFaces();
};
