// The split tool ("tool:split", type 10).
#pragma once
#include "Tool.h"
#include "EditableMesh.h"

class MeshInterface;

class DrawEdgeTool : public Tool {
public:
    DrawEdgeTool();
    virtual void pointerPressed();
    virtual void rightButtonPressed();
    virtual void deactivate();
    virtual void draw();
    virtual void fillContextMenu();

    void stopDrawing();

protected:
    friend class DesignSheet; // RebuildSharedInterfaces compares m_mesh with the sheet nodes' interfaces.
    MeshInterface *m_mesh;    // Current drawing interface.
    MeshVertex *m_vertex;     // Current drawing vertex.
};
