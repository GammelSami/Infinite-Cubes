// Shape is the abstract base of drawable text, curves, meshes and source instances. The engine registers every shape
// in a list; m_nodes tracks the nodes displaying it.
#pragma once
#include <list>
#include <set>
#include <string>
#include "plasma/Keyable.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

namespace plasma {

class Engine;
class Node;

enum ShapeType {
    SHAPE_UNTYPED = 0,     // Shape's default, kept by CurveShape and StaticMeshShape
    SHAPE_MESH = 1,        // MeshShape
    SHAPE_SMOOTH_MESH = 2, // SmoothMeshShape
    SHAPE_TEXT = 3,        // TextShape
    SHAPE_GENERIC = 4,     // GenericShape
};

class Shape : public Keyable {
public:
    Shape(Engine *engine, const std::wstring &name);
    virtual ~Shape();

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
    virtual Shape *clone() { return nullptr; }
    virtual void updateBounds(const Matrix<float> &matrix) {}
    virtual void shownChanged(bool shown) {}
    virtual void releaseCache() {}
    virtual bool isCacheEmpty() { return false; }
    virtual void fitBounds(const Vector<2, float> &minimum, const Vector<2, float> &maximum) {}

    int getType() const { return m_type; }
    int countNodes() const { return m_nodes.size(); }

protected:
    friend class Node;                          // registers itself in m_nodes
    friend class Engine;                        // erases m_engineEntry when deleting the shape
    int m_type;                                 // a ShapeType, set by the derived constructors
    std::list<Shape *>::iterator m_engineEntry; // Registration entry in the engine's shape list.
    std::set<Node *> m_nodes;                   // Nodes displaying this shape.
    bool m_unused90;                            // Set by the constructor; no reader found.
    bool m_unused91;                            // Cleared by the constructor; no reader found.
};

}
