// Node is an element of the scene tree. Nodes share shapes, transformations and displays, each of which tracks its nodes. A node owns child links, widget references, accumulated and inverse matrices and string variables.
#pragma once
#include <list>
#include <map>
#include <string>
#include "plasma/NamedObject.h"
#include "plasma/Matrix.h"
namespace plasma {
class Shape;
class TextShape;
class Transformation;
class Display;
class Widget;
class RenderSurface;

// Node::m_flags. Individual flags are bit indices; use (1 << flag) to form a mask.
enum NodeFlags {
    NODE_LOCKED = 0,
    NODE_INACTIVE = 1, // not hit-testable
    NODE_HIDDEN = 2,
    NODE_INTERNAL = 3, // the engine's "sources" and "fonts" nodes
    NODE_GENERIC = 4,
    NODE_SNAPABLE = 5,
    NODE_DEFORMER = 6,
    NODE_ADAPT = 7,
    NODE_ALIGN_OUTLINE = 8,
    NODE_ALIGN_OUTLINE_ROTATION = 9,
    NODE_CACHED = 10,
    NODE_FIXED = 11,
    NODE_IGNORES_POINTER = 12,
    NODE_ALWAYS_ANIMATE = 13 // playback and setFrame run even while invisible
};

// Whole-field masks; the flags above are bit indices.
enum NodeFlagMasks {
    NODE_FLAGS_NONE = 0
};

// Node::getMinimum's and getMaximum's flags. Individual flags are bit indices; use (1 << flag) to
// form a mask.
enum BoundsFlags {
    BOUNDS_SKIP_WIDGETS = 0,   // not the children with widgets
    BOUNDS_INCLUDE_HIDDEN = 1, // also the children whose display is not visible
    BOUNDS_SKIP_INACTIVE = 2,  // not the NODE_INACTIVE children
    BOUNDS_NO_CHILDREN = 3     // the node's own bounds only
};

// Whole-field masks; the flags above are bit indices.
enum BoundsMasks {
    BOUNDS_FLAGS_NONE = 0
};

// Node::draw mode bits. Individual flags are bit indices; use (1 << flag) to form a mask.
enum NodeRenderMode {
    NODE_RENDER_CULL_SCREEN = 0,
    NODE_RENDER_STOP_AT_EFFECT = 2,
    NODE_RENDER_SKIP_BLUR = 3,
    NODE_RENDER_NO_CHILDREN = 4
};

// Whole-field masks; the flags above are bit indices.
enum NodeRenderMasks {
    NODE_RENDER_NONE = 0
};

// Node::hitTest's flags. Individual flags are bit indices; use (1 << flag) to form a mask.
enum HitTestFlags {
    HIT_SKIP_LOCKED = 0,      // NODE_LOCKED nodes are not hit
    HIT_INCLUDE_INACTIVE = 1, // NODE_INACTIVE nodes are hit too
    HIT_SKIP_INTERNAL = 2     // NODE_INTERNAL nodes are not hit
};

// Whole-field masks; the flags above are bit indices.
enum HitTestMasks {
    HIT_FLAGS_NONE = 0
};

class Node : public NamedObject {
public:
    Node(Engine *engine, Transformation *transformation, Shape *shape, Display *display, const std::wstring &name);
    virtual ~Node();
    template<class T>
    void connectWidget(const std::wstring &name, int event, T *object, void (T::*method)(), bool recursive);
    void setShape(Shape *shape);
    void setTransformation(Transformation *transformation);
    void setDisplay(Display *display);
    void setWidget(Widget *widget);
    void setParent(Node *parent);
    void addChild(Node *child);
    void insertChild(Node *child, std::list<Node *>::iterator where);
    bool insertChild(Node *child, Node *before);
    void removeChild(Node *child);
    void destroyChildren();
    void makeUnique();
    Node *clone(Node *parent);
    Node *instance(Node *parent);
    bool isPlaying();
    Node *findNode(const std::wstring &name);
    void findNodes(std::list<Node *> &nodes, const std::wstring &name);
    // Collects the text shapes of the node and its descendants (defined in TextShape.h, which
    // completes TextShape).
    void collectTextShapes(std::list<TextShape *> &shapes);
    void setVariable(const std::string &key, const std::string &value);
    void play(const std::wstring &sequenceName, int loopLength);
    void stop();
    void setFrame(int frame);
    void setFrameToSequenceEnd(const std::wstring &name) { setFrame(getSequenceEnd(name)); }
    bool animate(int frames);
    int draw(int mode);
    Node *hitTest(const Vector<2, float> &position, int flags);
    void initializeWidgets();
    bool isAncestorOf(const Node *node) const;
    bool getBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix);
    bool getScreenBounds(Vector<2, float> &minimum, Vector<2, float> &maximum);
    Vector<2, float> getMinimum(int flags);
    Vector<2, float> getMaximum(int flags);
    bool isOnScreen();
    void updateMatrices();
    void updateMatricesRecursively();
    Node *getParent() const { return m_parent; }
    Shape *getShape() const { return m_shape; }
    Transformation *transformation() const { return m_transformation; }
    Display *display() const { return m_display; }
    Widget *widget() const { return m_widget; }
    // Sets NODE_INACTIVE throughout the subtree so pointer input passes through.
    void setHitTestable(bool hitTestable)
    {
        if (hitTestable)
            m_flags &= ~(1 << NODE_INACTIVE);
        else
            m_flags |= (1 << NODE_INACTIVE);
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->setHitTestable(hitTestable);
    }
    bool isShown() const { return !(m_flags & (1 << NODE_HIDDEN)); }
    // Queries or changes whether the node's display is visible.
    bool isVisible() const;
    void setVisible(bool visible);
    bool isHitTestable() const { return !((m_flags >> NODE_INACTIVE) & 1); }
    bool isInternal() const { return (m_flags >> NODE_INTERNAL) & 1; }
    bool fitsShapeToChildren() const { return (m_flags >> NODE_ADAPT) & 1; }
    bool ignoresPointer() const { return (m_flags >> NODE_IGNORES_POINTER) & 1; }
    int moveIntoWidget(const std::wstring &name, Widget *widget, bool fit);
    std::wstring getText();
    std::wstring getText(const std::wstring &name, bool recursive);
    void setText(const std::wstring &text);
    void setText(const std::wstring &name, const std::wstring &text, bool recursive);
    // Narrow text (a creature's own name), widened character by character.
    void setText(const std::wstring &name, const std::string &text, bool recursive)
    {
        setText(name, std::wstring(text.begin(), text.end()), recursive);
    }
    int getSequenceEnd(const std::wstring &sequenceName);
    bool extendBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix, bool &empty,
                      bool withBlur);

private:
    void resetStepped();
    void resetChanged();
    bool animate(int frames, bool matricesChanged, bool force);
    void setFrame(int frame, bool matricesChanged, bool force);

public:
    void *m_userData;
    Node *m_parent;
    std::list<Node *> m_children;
    Shape *m_shape;
    Transformation *m_transformation;
    Display *m_display;
    Widget *m_widget;
    Widget *m_enclosingWidget; // this node's widget or the nearest ancestor's
    Matrix<float> m_matrix;
    Matrix<float> m_inverseMatrix;
    unsigned m_flags; // NodeFlags
    bool m_changed;
    RenderSurface *m_cacheSurface; // NODE_CACHED: the subtree is drawn into it
    std::list<Node *>::iterator m_engineEntry;
    std::list<int> m_pendingChildIds;      // child ids read by the PLA reader
    std::list<std::string> m_variableKeys; // in the order they were first set
    std::map<std::string, std::string> m_variables;
};
}
#include "plasma/Display.h"
namespace plasma {
inline bool Node::isVisible() const
{
    return m_display->m_visibility.value() != DISPLAY_INVISIBLE;
}
inline void Node::setVisible(bool visible)
{
    int value = visible ? DISPLAY_VISIBLE : DISPLAY_INVISIBLE;
    m_display->m_visibility.value() = value;
}
}
