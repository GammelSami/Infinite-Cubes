// BoundingBoxTool is the base of NodeTool and the mesh tools (VertexTool, EdgeTool, FaceTool and ExtrusionTool),
// and derives from Tool. It draws the selection's bounding box with eight corner and edge-midpoint handles and a
// size label. It starts handle drags and keeps the selection rectangle or lasso dragged by the pointer.
#pragma once
#include <list>
#include <vector>
#include "Tool.h"
#include "plasma/Vector.h"

// Sets kept to one flag per path point: 1 keeps a point in the Douglas-Peucker simplification at the given
// tolerance. Defined in DrawFreehandTool.cpp.
void simplifyPath(float tolerance, const std::vector<plasma::Vector<2, float>> &path, std::vector<int> &kept);
// Its recursive step (WidgetBindOperation.cpp): marks in kept the points between first and last that the
// simplification keeps.
void simplifyPathSegment(float tolerance, const std::vector<plasma::Vector<2, float>> &points, int first, int last,
                         std::vector<int> &kept);

class BoundingBoxTool : public Tool {
    friend class ::Workspace; // MouseMove tests the box's selection mode.
public:
    // FindHandle, GetDragHandle and m_dragHandle without a handle.
    enum {
        NoHandle = -1
    };

    explicit BoundingBoxTool(int type);

    // Whether the pointer is over a handle.
    virtual bool isOverHandle();

    // Whether a handle is being dragged.
    virtual bool isDraggingHandle();

    // Appends the box's four corners relative to point (the minimum corner, (min x, max y), (max x, min y), the
    // maximum corner).
    virtual bool getSnapOffsets(const plasma::Vector<2, float> &point, std::list<plasma::Vector<2, float>> &offsets);
    // The scale box panel calls BeginBoxChange before a change and EndBoxChange afterwards. SetBox takes the new
    // corners; RotateBox takes a rotation in degrees ("scalebox:rotation"). The base implementations do nothing.
    virtual void beginBoxChange() {}
    virtual void endBoxChange() {}
    virtual void setBox(const plasma::Vector<2, float> &boxMin, const plasma::Vector<2, float> &boxMax) {}
    virtual void rotateBox(float rotation) {}

    // Places the eight handles on the box's corners and edge midpoints.
    void updateHandles();

    // The handle under the pointer, or NoHandle.
    int findHandle();

    // Whether the handles are shown: an open sheet, none of the workspace's m_setPivotRequested,
    // m_chooseParentRequested, m_sendBehindRequested, m_sendInFrontOfRequested set, and the box at least 20 pixels
    // wide or high.
    bool handlesShown();

    // The handle being dragged, or NoHandle.
    int getDragHandle() const { return m_dragHandle; }

    // Draws the box, its handles and, while one is hovered or dragged, the box's size in the sheet's unit beside the
    // pointer.
    void renderBox();

    // Draws the selection rectangle or lasso.
    void renderSelection();

    // Starts the selection rectangle or lasso at the pointer.
    void beginSelection();

    // Simplifies the finished lasso.
    void endSelection();
    void cancelSelection();

    // Whether the point (in window coordinates) lies inside the selection rectangle or lasso.
    bool contains(const plasma::Vector<2, float> &point);

    // Extends the selection rectangle or lasso to the pointer.
    void updateSelection();

    // Starts dragging the handle under the pointer, if the handles are shown.
    void beginHandleDrag();

    // The box corners of the selection (ScaleBoxModule shows them as "scalebox:x", "scalebox:y", "scalebox:width",
    // "scalebox:height").
    plasma::Vector<2, float> m_boxMin;
    plasma::Vector<2, float> m_boxMax;

protected:
    // The four corners from the minimum, counter-clockwise, then the midpoints of the bottom, left, top and right
    // edges.
    plasma::Vector<2, float> m_handles[8];
    int m_dragHandle; // NoHandle when none.
    plasma::Vector<2, float> m_dragStart;
    plasma::Vector<2, float> m_dragPosition;

    // The point the dragged handle scales about: the opposite handle, or the middle between it and the handle while
    // Shift is held.
    plasma::Vector<2, float> m_dragAnchor;
    int m_selectionMode; // SelectionMode.
    plasma::Vector<2, float> m_rectangleStart;
    plasma::Vector<2, float> m_rectangleEnd;
    std::vector<plasma::Vector<2, float>> m_lasso;
};
