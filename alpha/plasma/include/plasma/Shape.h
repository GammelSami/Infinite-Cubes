// Shape defines drawing, bounds and hit testing for scene geometry.
#pragma once
#include <list>
#include <set>
#include "plasma/Keyable.h"
#include "plasma/Vector.h"
#include "plasma/Matrix.h"

namespace plasma {
enum ShapeType {
    SHAPE_UNTYPED = 0,
    SHAPE_MESH = 1,
    SHAPE_SMOOTH_MESH = 2,
    SHAPE_TEXT = 3,
    SHAPE_GENERIC = 4,
};
class Node;

class Shape : public Keyable {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    Shape(Engine *engine, const std::wstring &name);
    virtual ~Shape();
    int getType() const { return m_type; }
    virtual void draw(Node *node) = 0;
    virtual bool intersects(const Vector<2, float> &point, float radius, const Matrix<float> &matrix,
                            const Matrix<float> &inverse)
    {
        return false;
    }
    virtual bool contains(const Vector<2, float> &point) { return false; }
    virtual bool isDeformable() { return true; }
    virtual Vector<2, float> getMinimum() { return Vector<2, float>(0.0f, 0.0f); }
    virtual Vector<2, float> getMaximum() { return Vector<2, float>(0.0f, 0.0f); }
    virtual bool getTransformedBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix,
                                      bool &first)
    {
        return false;
    }
    virtual bool getVertexBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix,
                                 bool &first)
    {
        return false;
    }
    virtual void storePositions() {}
    virtual void restorePositions() {}
    virtual void stretchPositions(const Vector<2, float> &origin, const Vector<2, float> &size,
                                  const Vector<2, float> &target, const Vector<2, float> &targetSize,
                                  const Matrix<float> &from, const Matrix<float> &to)
    {}
    virtual Shape *clone() { return 0; }
    virtual void updateBounds(const Matrix<float> &matrix) {}
    virtual void shownChanged(bool shown) {}
    virtual void releaseCache() {}
    virtual bool isCacheEmpty() { return false; }
    virtual void fitBounds(const Vector<2, float> &minimum, const Vector<2, float> &maximum) {}

    int countNodes() const { return (int)m_nodes.size(); }

protected:
    friend class Widget;
    friend class Node;
    friend class Engine;
    int m_type;
    std::list<Shape *>::iterator m_engineEntry;
    std::set<Node *> m_nodes;
    bool m_unused58; // set by the constructor; nothing reads it (the 2012 engine's m_unused6C)
    bool m_unused59; // cleared by the constructor; likewise (m_unused6D)
};

}
