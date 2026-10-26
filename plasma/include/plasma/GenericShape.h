// GenericShape.cpp: a source instance or a shaded placeholder rectangle.
#pragma once
#include <string>
#include "plasma/Shape.h"
#include "plasma/Vector.h"

namespace plasma {

class Drawing;

class GenericShape : public Shape {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    GenericShape(Engine *engine, const std::wstring &source, const std::wstring &name);
    virtual ~GenericShape();
    virtual void attributesChanged(int reason);
    virtual void draw(Node *node) {}
    virtual bool contains(const Vector<2, float> &point);
    virtual Vector<2, float> getMinimum();
    virtual Vector<2, float> getMaximum();
    virtual Shape *clone();
    virtual void updateDrawing(); // Updates the drawing for this shape.

    Vector<2, float> m_position;
    Vector<2, float> m_size;
    std::wstring m_source; // PLX source / wsource.
    Node *m_instanceNode;  // Engine::ResolveSourceNodes.
    Drawing *m_drawing;

    void setSource(const std::wstring &source) { m_source.assign(source); }
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif

}
