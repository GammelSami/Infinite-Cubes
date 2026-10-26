// The procedural shape tool (type 14).
#pragma once
#include "Tool.h"

namespace plasma {
class SmoothMeshShape;
class Node;
}

class ProceduralTool : public Tool {
public:
    ProceduralTool();
    virtual void pointerMoved(int x, int y);
    virtual void pointerPressed();
    virtual void pointerReleased();
    virtual void rightButtonPressed();
    virtual void draw();
    virtual bool getSnapOffsets(const plasma::Vector<2, float> &point, std::list<plasma::Vector<2, float>> &offsets);
    virtual bool isRotating();

    // Marks the preview for regeneration at the next Render.
    void invalidatePreview();

protected:
    plasma::SmoothMeshShape *m_previewShape; // "procedural:preview".
    plasma::Node *m_previewNode;
    float m_angle;
    plasma::Vector<2, float> m_dragOrigin;
    bool m_refreshPreview;
};
