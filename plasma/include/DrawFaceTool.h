// DrawFaceTool: the tool that draws a new face vertex by vertex ("draw face", "create mesh"; type 8 in the tool
// module, "tool:draw"). Its context menu offers "Stop Drawing" and "Resize Tool Radius...".
#pragma once
#include <list>
#include "Tool.h"

class MeshInterface;
struct MeshFace;
struct MeshVertex;

class DrawFaceTool : public Tool {
public:
    DrawFaceTool();

    virtual void pointerMoved(int, int);

    // A press: adds the snapped or a new vertex to the face being drawn.
    virtual void pointerPressed();
    virtual void pointerReleased();
    virtual void deactivate(); // Stops drawing.

    // Draws the face so far, the rubber band and the pointer.
    virtual void draw();

    // Fills the context menu.
    virtual void fillContextMenu();

    // Sets the drawing state (MeshInterface::RestoreState restores it).
    void restoreDrawingState(MeshInterface *meshInterface, const std::list<MeshVertex *> &vertices, MeshFace *face);
    // Ends the face being drawn ("Stop Drawing"; Workspace calls it).
    void stopDrawing();

    MeshInterface *m_meshInterface;     // The mesh being drawn into.
    std::list<MeshVertex *> m_vertices; // The face's vertices so far.
    MeshFace *m_face;                   // The face being drawn.
};
