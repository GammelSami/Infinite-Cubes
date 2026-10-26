// plasma::Shape: the abstract base of drawable shapes (GenericShape, MeshShape, CurveShape, TextShape,
// StaticMeshShape). The engine keeps every shape in a list; the nodes that show the shape are kept in m_nodes.
#pragma once
#include <list>
#include <set>
#include "plasma/Keyable.h"
#include "plasma/Vector.h"
#include "plasma/Node.h"

class NodeModule;
class PDFWriter;
class Workspace;

namespace plasma {

// Shape::m_type values, set by the derived constructors.
enum ShapeType {
    SHAPE_UNTYPED = 0,     // Shape's default, kept by CurveShape and StaticMeshShape.
    SHAPE_MESH = 1,        // MeshShape.
    SHAPE_SMOOTH_MESH = 2, // SmoothMeshShape.
    SHAPE_TEXT = 3,        // TextShape (Node::MakeAnimatedUnique treats it as animated).
    SHAPE_GENERIC = 4,     // GenericShape.
};

// Virtual operations shared by drawable shapes. MeshShape updates bounds and textures; curve, generic and static
// mesh shapes update their drawings.
class Shape : public Keyable {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    Shape(Engine *engine, const std::wstring &name);
    virtual ~Shape();

    // Returns the shape type for MeshModule's smooth mesh tests.
    int getType() const { return m_type; }

    // Draws the shape for node (Node::RenderShape).
    virtual void draw(Node *node) = 0;

    // Whether the shape comes within radius of the point (on the sheet), drawn with matrix; inverse is its inverse
    // (Node::HitTest with a radius passes m_matrix and m_inverseMatrix); as Drawing::Intersects.
    virtual bool intersects(const Vector<2, float> &point, float radius, const Matrix<float> &matrix,
                            const Matrix<float> &inverse)
    {
        return false;
    }

    // The point in the shape's space (TextShape tests its bounds).
    virtual bool contains(const Vector<2, float> &point) { return false; }

    // Whether an enclosing widget's Deform/Undeform applies (every caller tests it before the widget's deformation);
    // TextShape says no.
    virtual bool isDeformable() { return true; }
    virtual Vector<2, float> getMinimum() { return Vector<2, float>(0.0f, 0.0f); } // GenericShape: position.
    virtual Vector<2, float> getMaximum() { return Vector<2, float>(0.0f, 0.0f); } // GenericShape: position + size.

    // Widen the bounds by the shape's transformed drawings or its transformed vertex positions; 'first' says the
    // bounds are still empty (MeshInterface).
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

    // Mesh and curve shapes: the vertex positions' current element to element 1 and back (as
    // MeshInterface::StorePositions/RestorePositions).
    virtual void storePositions() {}
    virtual void restorePositions() {}

    // Mesh and curve shapes map vertices through from, stretch points inside the box as it changes to
    // target/targetSize, move outside points with the nearer edge, then map back through to. Currently unused.
    virtual void stretchPositions(const Vector<2, float> &origin, const Vector<2, float> &size,
                                  const Vector<2, float> &target, const Vector<2, float> &targetSize,
                                  const Matrix<float> &from, const Matrix<float> &to)
    {}
    virtual Shape *clone() { return 0; } // A new copy; 0 for StaticMeshShape.

    // Called before the bounds are read or the shape drawn through matrix: a hinted TextShape measures its bounds
    // under it.
    virtual void updateBounds(const Matrix<float> &matrix) {}

    // Node::SetShown passes the new state (SmoothMeshShape creates its drawings when first shown).
    virtual void shownChanged(bool shown) {}

    // Frees subdivision levels, refined arrays and drawing data, or reports whether they are empty. Currently unused.
    virtual void releaseCache() {}
    virtual bool isCacheEmpty() { return false; }

    // Rescales the shape into the bounds (Node::Animate, NODE_ADAPT).
    virtual void fitBounds(const Vector<2, float> &minimum, const Vector<2, float> &maximum) {}

    int countShownNodes();
    int countNodes() const { return m_nodes.size(); }

protected:
    friend class Widget;                        // Finds the text shapes of its caption nodes.
    friend class Node;                          // A node registers itself in m_nodes.
    friend class Engine;                        // DeleteShape/DeleteTransformation/DeleteDisplay erase m_engineEntry.
    friend class ::NodeModule;                  // "adapt to children" marks the shape changed.
    friend class ::Workspace;                   // Refreshes the interfaces of the shape's other nodes.
    friend class ::PDFWriter;                   // The writer dispatches on m_type.
    int m_type;                                 // A ShapeType, set by the derived constructors.
    std::list<Shape *>::iterator m_engineEntry; // This shape's node in the engine's list.
    std::set<Node *> m_nodes;
    bool m_unused6C; // Set by the constructor and otherwise unused.
    bool m_unused6D; // Cleared by the constructor; likewise unused.
};

}
