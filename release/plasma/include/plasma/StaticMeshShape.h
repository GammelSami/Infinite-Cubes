// StaticMeshShape stores baked fill, stroke and extrusion drawings and their combined bounds.
#pragma once
#include <string>
#include "plasma/Shape.h"
#include "plasma/Vector.h"

namespace plasma {

class Drawing;

class StaticMeshShape : public Shape {
public:
    StaticMeshShape(Engine *engine, Drawing *fill, Drawing *stroke, Drawing *extrusion, const std::wstring &name);
    virtual ~StaticMeshShape();
    virtual void draw(Node *node);
    virtual bool contains(const Vector<2, float> &point);
    virtual Vector<2, float> getMinimum();
    virtual Vector<2, float> getMaximum();
    virtual void updateDrawing();

    Drawing *m_fillDrawing; // deleted first by the destructor
    Drawing *m_strokeDrawing;
    Drawing *m_extrusionDrawing; // drawn first by Render
    Vector<2, float> m_minimum;  // updateDrawing, getMinimum
    Vector<2, float> m_maximum;  // updateDrawing, getMaximum
};

}
