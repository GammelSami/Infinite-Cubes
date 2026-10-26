// GenericShape displays a source node instance or a shaded placeholder rectangle, using its position, size and source
// name.
#pragma once
#include <string>
#include "plasma/Shape.h"
#include "plasma/Vector.h"

namespace plasma {

class Drawing;

class GenericShape : public Shape {
public:
    GenericShape(Engine *engine, const std::wstring &source, const std::wstring &name);
    virtual ~GenericShape();
    virtual void attributesChanged(bool all);
    virtual void draw(Node *node) {}
    virtual Vector<2, float> getMinimum();
    virtual Vector<2, float> getMaximum();
    virtual Shape *clone();
    virtual void updateDrawing();

    void setSource(const std::wstring &source) { m_source = source; }

    // Public as in plasma (the PLX reader and the engine's source resolution use them).
    Vector<2, float> m_position;
    Vector<2, float> m_size; // (100, 100) from the constructor
    std::wstring m_source;   // the PLX source name
    Node *m_instanceNode;    // tested by updateDrawing; set by the engine's source resolution
    Drawing *m_drawing;
};

}
