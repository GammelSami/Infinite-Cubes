// A mesh baked into fill, stroke and extrusion drawings.
#pragma once
#include "plasma/Shape.h"

namespace plasma {

class Drawing;

class StaticMeshShape : public Shape {
public:
    StaticMeshShape(Engine *engine, const std::wstring &name, Drawing *fill, Drawing *stroke, Drawing *extrusion);

    ~StaticMeshShape();
    virtual void draw(Node *node);
    virtual bool contains(const Vector<2, float> &point);
    virtual Vector<2, float> getMinimum();
    virtual Vector<2, float> getMaximum();
    virtual void updateDrawing(); // Updates the drawing for this shape.

    Drawing *m_fillDrawing;
    Drawing *m_strokeDrawing;
    Drawing *m_extrusionDrawing;
    Vector<2, float> m_minimum;
    Vector<2, float> m_maximum;
};
}
