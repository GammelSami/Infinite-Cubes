// Node shows a shape through a transformation and display, which may be shared with other nodes. It owns a child
// hierarchy, widget association, flags and named string variables. When the last node releases a shape, transformation
// or display, the engine deletes that object. clone copies the subtree's objects; instance shares them.
#pragma once
#include "plasma/Display.h"
#include <list>
#include <map>
#include <string>
#include "plasma/NamedObject.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"
#include "plasma/Transformation.h"

namespace plasma {

class Engine;
class Shape;
class Transformation;
class Display;
class Widget;
class RenderSurface;

// Node flags controlling visibility, hit testing, animation and caching.
enum NodeFlags {
    NODE_LOCKED = 0,                 // hit tests with HIT_SKIP_LOCKED skip it
    NODE_INACTIVE = 1,               // not hit-testable unless HIT_INCLUDE_INACTIVE
    NODE_HIDDEN = 2,                 // deleted or detached: the walks skip it
    NODE_INTERNAL = 3,               // not part of the document (makeUnique skips it)
    NODE_GENERIC = 4,                // Reserved document flag.
    NODE_SNAPABLE = 5,               // Reserved document flag.
    NODE_DEFORMER = 6,               // Reserved document flag.
    NODE_ADAPT = 7,                  // Animate fits the children's bounds when changed
    NODE_ALIGN_OUTLINE = 8,          // Reserved document flag.
    NODE_ALIGN_OUTLINE_ROTATION = 9, // Reserved document flag.
    NODE_CACHED = 10,                // the subtree is drawn into m_cacheSurface (Render)
    NODE_FIXED = 11,                 // Reserved document flag.
    NODE_IGNORES_POINTER = 12,       // hit tests skip the node and its subtree
    NODE_ALWAYS_ANIMATE = 13         // Animate/setFrame play it even while invisible
};

// Engine::drawWindow passes its argument through.
enum NodeRenderMode {
    NODE_RENDER_CULL_SCREEN = 0,    // only nodes on screen (isOnScreen)
    NODE_RENDER_STOP_AT_EFFECT = 2, // stop before the first clipped or blurred child; no clipping here
    NODE_RENDER_SKIP_BLUR = 3,      // no blur (cleared for the children)
    NODE_RENDER_NO_CHILDREN = 4     // the node only; no clipping here
};

// The flags argument of Node::hitTest.
enum HitTestFlags {
    HIT_SKIP_LOCKED = 0,
    HIT_INCLUDE_INACTIVE = 1,
    HIT_SKIP_INTERNAL = 2
};

// The flags argument of Node::getMinimum and getMaximum.
enum BoundsFlags {
    BOUNDS_SKIP_WIDGETS = 0,   // not the children with widgets
    BOUNDS_INCLUDE_HIDDEN = 1, // also the children whose display is not visible
    BOUNDS_SKIP_INACTIVE = 2,  // not the NODE_INACTIVE children
    BOUNDS_NO_CHILDREN = 3     // the node's own bounds only
};

class Node : public NamedObject {
public:
    Node(Engine *engine, Transformation *transformation, Shape *shape, Display *display, const std::wstring &name);
    virtual ~Node();

    // Each releases the current object (the engine deletes it when this node was its only user) and registers this
    // node with the new one.
    void setShape(Shape *shape);
    void setTransformation(Transformation *transformation);
    void setDisplay(Display *display);
    void setWidget(Widget *widget);
    void replaceWidget(Widget *widget);

    void setParent(Node *parent);
    // Detaches the child from its old parent and appends it (no matrices).
    void addChild(Node *child);
    void removeChild(Node *child);
    // The engine destroys a copy of the children list.
    void destroyChildren();
    bool isAncestorOf(const Node *node) const;

    // Whether NODE_HIDDEN is clear; all tree walks test this.
    bool isShown() const { return !((m_flags >> NODE_HIDDEN) & 1); }
    bool isInternal() const { return (m_flags >> NODE_INTERNAL) & 1; }
    bool isHitTestable() const { return !((m_flags >> NODE_INACTIVE) & 1); }
    // NODE_INACTIVE for the node and its subtree: set, the pointer passes through them.
    void setHitTestable(bool hitTestable)
    {
        if (hitTestable)
            m_flags &= ~(1 << NODE_INACTIVE);
        else
            m_flags |= (1 << NODE_INACTIVE);
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->setHitTestable(hitTestable);
    }
    bool isSource() const { return (m_flags >> NODE_GENERIC) & 1; }
    bool ignoresPointer() const { return (m_flags >> NODE_IGNORES_POINTER) & 1; }
    // Controls NODE_IGNORES_POINTER for this node only.
    void setIgnoresPointer(bool ignoresPointer)
    {
        if (ignoresPointer)
            m_flags |= (1 << NODE_IGNORES_POINTER);
        else
            m_flags &= ~(1 << NODE_IGNORES_POINTER);
    }
    bool fitsShapeToChildren() const { return (m_flags >> NODE_ADAPT) & 1; }

    // Writes the accumulated transformation matrix or its inverse to matrix. Without a transformation the node
    // contributes an identity matrix.
    void getMatrix(Matrix<float> &matrix) const
    {
        if (m_parent)
            m_parent->getMatrix(matrix);
        else
            matrix.identity();
        if (m_transformation)
            matrix *= m_transformation->m_matrix;
    }
    void getInverseMatrix(Matrix<float> &matrix) const
    {
        if (m_transformation)
            matrix = m_transformation->m_inverseMatrix;
        else
            matrix.identity();
        if (m_parent) {
            Matrix<float> parent;
            m_parent->getInverseMatrix(parent);
            matrix *= parent;
        }
    }

    // Node state accessors.
    Transformation *transformation() const { return m_transformation; }
    Display *display() const { return m_display; }
    Widget *widget() const { return m_widget; }
    Node *getParent() const { return m_parent; }
    // Queries or changes the node's display visibility.
    bool isVisible() const { return m_display->m_visibility.value() != 0; }
    void setVisible(bool visible)
    {
        int value = visible ? 1 : 0;
        m_display->m_visibility.value() = value;
    }

    // Connects event to object->method() on the widgets named name in the subtree, as enabled-only connections
    // (recursive: on their child widgets too).
    template<class T>
    void connectWidget(const std::wstring &name, int event, T *object, void (T::*method)(), bool recursive);

    // The keyables' playback, recursively.
    void play(const std::wstring &sequenceName, int loopLength);
    void stop();
    int getSequenceEnd(const std::wstring &sequenceName);
    // Advances the subtree's playback by frames; whether anything changed.
    bool animate(int frames);
    void setFrame(int frame);
    void setFrameToSequenceEnd(const std::wstring &sequenceName) { setFrame(getSequenceEnd(sequenceName)); }
    bool isPlaying();

    // Give this node (and its subtree) copies of shared shapes, transformations and displays.
    void makeUnique();
    // Makes a single shared object unique to this node. The named variant makes displays unique for shown,
    // non-internal nodes of that name in the subtree.
    void makeShapeUnique();
    void makeTransformationUnique();
    void makeDisplayUnique();
    void makeDisplayUnique(const std::wstring &name);
    // A copy of the subtree under parent, with copies of the shapes, transformations, displays and widgets.
    Node *clone(Node *parent);
    // The same sharing this subtree's shapes, transformations and displays.
    Node *instance(Node *parent);

    // Renders the subtree and returns the number of shapes drawn.
    int draw(int mode);

    Vector<2, float> getMinimum(int flags);
    Vector<2, float> getMaximum(int flags);
    // Extends minimum/maximum by the subtree's shape bounds through matrix (the shown, visible children's unless the
    // display clips; withBlur: grown by the blur radius); empty is cleared once anything was added.
    bool extendBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix, bool &empty,
                      bool withBlur);
    // Extends bounds using the parent's accumulated matrix and this node's transformation, including blur.
    bool getShapeBounds(Vector<2, float> &minimum, Vector<2, float> &maximum);
    // The node's bounds in its own space through matrix.
    bool getBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix);
    bool getScreenBounds(Vector<2, float> &minimum, Vector<2, float> &maximum);
    bool isOnScreen();
    void initializeWidgets();

    // Returns the topmost shown node under point, visiting children in reverse order and honoring HitTestFlags.
    // Rejects points outside a widget's render surface.
    Node *hitTest(const Vector<2, float> &position, int flags);
    // Whether hitTest finds a node under the engine pointer.
    bool isUnderPointer();
    void setPosition(const Vector<2, float> &position, const Vector<2, float> &offset);
    void setPosition(float x, float y, float offsetX, float offsetY);

    Node *findNode(const std::wstring &name);
    // Appends the shapes of the subtree that are a T.
    template<class T> void collectShapes(std::list<T *> &shapes)
    {
        if (m_shape) {
            T *shape = dynamic_cast<T *>(m_shape);
            if (shape)
                shapes.push_back(shape);
        }
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->collectShapes(shapes);
    }
    void findNodes(std::list<Node *> &nodes, const std::wstring &name);
    // Moves widget's node under the nodes named name in the subtree (fit: the widget takes the view size of that
    // node's own widget).
    int moveIntoWidget(const std::wstring &name, Widget *widget, bool fit);

    // The text shapes (shape type 3) of the subtree. The named variants affect the nodes named name (recursive: their
    // whole subtree).
    void setText(const std::wstring &name, const std::wstring &text, bool recursive);
    // The same for a narrow text, widened character by character.
    void setText(const std::wstring &name, const std::string &text, bool recursive)
    {
        setText(name, std::wstring(text.begin(), text.end()), recursive);
    }
    void setText(const std::wstring &text);
    std::wstring getText(const std::wstring &name, bool recursive);
    std::wstring getText();
    void setTextColor(const std::wstring &name, const Vector<4, float> &color, bool recursive);
    void setTextColor(const Vector<4, float> &color);

    // The "Node.variable" key/value strings.
    void setVariable(const std::string &key, const std::string &value);

private:
    // Animate's steps: clear the keyables' stepped flags, advance, then clear the changed flags of what changed.
    void resetStepped();
    void resetChanged();
    bool animate(int frames, bool force);
    void setFrame(int frame, bool force);

public:
    // Initially zero; never read by Node.cpp.
    void *m_userData;
    Node *m_parent;
    std::list<Node *> m_children;
    Shape *m_shape;
    Transformation *m_transformation;
    Display *m_display;
    Widget *m_widget;
    // This node's widget or the nearest ancestor's (set by Animate, setFrame, initializeWidgets, Clone).
    Widget *m_enclosingWidget;
    unsigned m_flags; // NodeFlags
    bool m_changed;   // set by Animate when something in the subtree changed
    RenderSurface *m_cacheSurface;
    std::list<Node *>::iterator m_engineEntry; // Engine::addNode
    // Child IDs read from a PLA document.
    std::list<int> m_pendingChildIds;
    std::list<std::string> m_variableKeys;          // in the order they were first set
    std::map<std::string, std::string> m_variables; // "Node.variable"
};

}
