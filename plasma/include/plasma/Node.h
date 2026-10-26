// plasma::Node: an element of the engine's scene tree. A node shows a shape through a transformation and a display
// (several nodes may share each of them; they keep the set of their nodes), and keeps its children, a widget, its
// accumulated matrix and its inverse, and named string variables ("Node.variable").
#pragma once
#include <list>
#include <map>
#include <set>
#include <string>
#include "plasma/NamedObject.h"
#include "plasma/Matrix.h"

namespace plasma {

class Shape;
class Transformation;
class Display;
class Widget;
class EditFilter;
class RenderSurface;

// Node::m_flags ("Node.flags"), named after the node panel's strings where it sets them (NodeModule) and otherwise
// after what the engine does.
enum NodeFlags {
    NODE_LOCKED = 0,
    NODE_INACTIVE = 1, // Not hit-testable.
    NODE_HIDDEN = 2,
    NODE_INTERNAL = 3, // The engine's "sources" and "fonts" nodes.
    NODE_GENERIC = 4,
    NODE_SNAPABLE = 5,
    NODE_DEFORMER = 6,
    NODE_ADAPT = 7,
    NODE_ALIGN_OUTLINE = 8,
    NODE_ALIGN_OUTLINE_ROTATION = 9,
    NODE_CACHED = 10,
    NODE_FIXED = 11,
    NODE_IGNORES_POINTER = 12,
    NODE_ALWAYS_ANIMATE = 13 // Playback and setFrame run even while invisible.
};

// Bit indices in Node::draw(mode), distinct from Node::m_flags.
enum NodeRenderMode {
    NODE_RENDER_CULL_SCREEN = 0,
    NODE_RENDER_STOP_AT_EFFECT = 2,
    NODE_RENDER_SKIP_BLUR = 3,
    NODE_RENDER_NO_CHILDREN = 4
};

enum NodeFlagMasks {
    NODE_FLAGS_NONE = 0
};

// The flags argument of Node::HitTest (both overloads).
enum HitTestFlags {
    HIT_SKIP_LOCKED = 0,      // NODE_LOCKED nodes are not hit.
    HIT_INCLUDE_INACTIVE = 1, // NODE_INACTIVE nodes are hit too.
    HIT_SKIP_INTERNAL = 2     // NODE_INTERNAL nodes are not hit.
};

enum HitTestMasks {
    HIT_FLAGS_NONE = 0
};

// The flags argument of Node::GetMinimum and GetMaximum: which children extend the bounds.
enum BoundsFlags {
    BOUNDS_SKIP_WIDGETS = 0,   // Not the children with widgets.
    BOUNDS_INCLUDE_HIDDEN = 1, // Also the children whose display is not visible.
    BOUNDS_SKIP_INACTIVE = 2,  // Not the NODE_INACTIVE children.
    BOUNDS_NO_CHILDREN = 3     // The node's own bounds only.
};

enum BoundsMasks {
    BOUNDS_FLAGS_NONE = 0
};

class Node : public NamedObject {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    Node(Engine *engine, Transformation *transformation, Shape *shape, Display *display, const std::wstring &name);
    virtual ~Node();

    // Each releases the current object (the engine deletes it when this node was its only user) and registers this
    // node with the new one.
    void setShape(Shape *shape);
    void setTransformation(Transformation *transformation);
    void setDisplay(Display *display);
    void setWidget(Widget *widget);

    void setParent(Node *parent);
    void addChild(Node *child);
    void insertChild(std::list<Node *>::iterator where, Node *child);

    // Inserts the child before `before` (appends it without one); false when `before` is not a child.
    bool insertChild(Node *child, Node *before);
    void removeChild(Node *child);
    void destroyChildren();

    // NODE_HIDDEN clear: not deleted (kept for undo) and not a closing sheet's root (Shape::CountShownNodes counts
    // these nodes).
    bool isShown() const { return !((m_flags >> NODE_HIDDEN) & 1); }
    // NODE_LOCKED nodes cannot be selected.
    bool isLocked() const { return m_flags & (1 << NODE_LOCKED); }
    void setShown(bool shown);
    // Returns a copy of the children list.
    std::list<Node *> getChildren() { return m_children; }
    // Sets or clears NODE_HIDDEN for the node and its entire subtree.
    void setShownRecursively(bool shown)
    {
        setShown(shown);
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->setShownRecursively(shown);
    }

    // NODE_INTERNAL: not part of the document (skipped by MakeUnique, CountVisibleChildren and GetSequenceNames).
    bool isInternal() const { return (m_flags >> NODE_INTERNAL) & 1; }
    // NODE_ALIGN_OUTLINE, "node:align:outline": the children are spread along the mesh outline (refreshed by
    // Workspace).
    bool alignsRotationToOutline() const { return (m_flags >> NODE_ALIGN_OUTLINE_ROTATION) & 1; }
    bool alignsToOutline() const { return (m_flags >> NODE_ALIGN_OUTLINE) & 1; }

    // NODE_SNAPABLE, "node:snapable" (NodeModule::ToggleSnapable): the mesh's edges and points attract the pointer.
    bool isSnapable() const { return (m_flags >> NODE_SNAPABLE) & 1; }
    // NODE_DEFORMER, "node:deformer": the children's meshes follow this node's mesh (MeshInterface).
    bool isDeformer() const { return (m_flags >> NODE_DEFORMER) & 1; }

    // NODE_ADAPT, "node:adapt": playback fits the shape to the children's bounds (Shape::FitBounds).
    bool fitsShapeToChildren() const { return (m_flags >> NODE_ADAPT) & 1; }
    // Sets NODE_INACTIVE for the node and its subtree so the pointer passes through them. Workspace uses this for
    // software-cursor nodes.
    void setHitTestable(bool hitTestable)
    {
        if (hitTestable)
            m_flags &= ~(1 << NODE_INACTIVE);
        else
            m_flags |= (1 << NODE_INACTIVE);
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->setHitTestable(hitTestable);
    }

    // NODE_INACTIVE is clear; used by BOUNDS_SKIP_INACTIVE.
    bool isHitTestable() const { return !((m_flags >> NODE_INACTIVE) & 1); }
    // NODE_GENERIC, "node:generic": a named source collected for GenericShape instances (Engine::CollectSourceNodes,
    // ResolveSourceNodes).
    bool isSource() const { return (m_flags >> NODE_GENERIC) & 1; }

    // NODE_FIXED, "node:fixed": the editing modules leave the shape alone (the mesh, text and texture panels skip such
    // nodes).
    bool isFixed() const { return (m_flags >> NODE_FIXED) & 1; }

    // NODE_IGNORES_POINTER: hit tests skip the node and its subtree.
    bool ignoresPointer() const { return (m_flags >> NODE_IGNORES_POINTER) & 1; }
    bool isAncestorOf(const Node *node) const;
    Node *getParent() const { return m_parent; }
    // Object identifiers returned by value for Engine::WritePLA's id maps.
    Shape *getShape() const { return m_shape; }
    Display *display() const { return m_display; }
    const Matrix<float> &getMatrix() const { return m_matrix; }
    const Matrix<float> &getInverseMatrix() const { return m_inverseMatrix; }
    Transformation *transformation() const { return m_transformation; }
    Widget *widget() const { return m_widget; }
    template<class T> void collectShapes(std::list<T *> &shapes)
    {
        if (m_shape) {
            T *shape = dynamic_cast<T *>(m_shape);
            if (shape)
                shapes.push_back(shape);
        }
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->collectShapes<T>(shapes);
    }

    Node *getNextSibling() const;

    // The keyables' playback, recursively.
    void play(const std::wstring &sequenceName, int loopLength);
    void stop();

    // The last key frame of the sequence in the subtree.
    int getSequenceEnd(const std::wstring &sequenceName);

    // Advances the subtree's playback by frames; whether anything changed.
    bool animate(int frames);

    // Shows the frame of the subtree's sequences.
    void setFrame(int frame);

    // Shows the last frame of the named sequence.
    void setFrameToSequenceEnd(const std::wstring &sequenceName) { setFrame(getSequenceEnd(sequenceName)); }
    void getSequenceNames(std::set<std::wstring> &names);

    // Give this node (and its subtree) copies of shared shapes, transformations and displays.
    void makeUnique();
    void makeAnimatedUnique();

    // A copy of the subtree under parent, with copies of the shapes, transformations, displays and widgets.
    Node *clone(Node *parent);

    // The same sharing this subtree's shapes, transformations and displays.
    Node *instance(Node *parent);

    void updateMatrices();
    void updateMatricesRecursively();
    // Renders the subtree (recursive; the window passes 1, the export tiles 0); the number of shapes drawn. Mode
    // flags: see the body.
    int draw(int mode);

    // The corners of the node's bounds in its parent's space: the shape's bounds (deformed by the enclosing widget,
    // grown by the blur radius), else the widget's, else the origin, extended by the children's unless the display
    // clips; flags are BoundsFlags.
    Vector<2, float> getMinimum(int flags);
    Vector<2, float> getMaximum(int flags);

    // Extends minimum/maximum by the bounds of the subtree's shapes through matrix (the shown children's unless the
    // display clips; withBlur: grown by the blur radius); empty is cleared once anything was added. Whether the
    // subtree had bounds.
    bool extendBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix, bool &empty,
                      bool withBlur);

    // The same without the clipping test and the blur: the subtree's shape bounds through matrix (the shown, visible
    // children's).
    bool extendShapeBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix,
                           bool &empty);

    // The subtree's shape bounds through the node's matrix; whether it has any.
    bool getShapeBounds(Vector<2, float> &minimum, Vector<2, float> &maximum);

    // The node's bounds in its own space (its matrix not applied): the shape's (deformed by the enclosing widget),
    // else the widget's, else the children's, grown by the blur radius scaled by matrix; whether there are any.
    bool getBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix);

    // GetBounds through the engine's model-view matrix, in window space.
    bool getScreenBounds(Vector<2, float> &minimum, Vector<2, float> &maximum);

    // Whether the shape's bounds through the node's matrix overlap the viewport.
    bool isOnScreen();
    // Draws the node's own shape.
    void renderShape();

    // Initializes the subtree's widgets and finds each node's enclosing widget.
    void initializeWidgets();

    // The topmost shown node of the subtree under the point (in the root's space); flags are HitTestFlags.
    Node *hitTest(const Vector<2, float> &position, int flags);
    // Also hits strokes within radius; with HIT_SKIP_INTERNAL an internal child's hit promotes its parent (Workspace's
    // scene picking).
    Node *hitTest(const Vector<2, float> &position, float radius, int flags);

    Node *findNode(const std::wstring &name);

    // The widget of that name in the subtree.
    Widget *findWidget(const std::wstring &name);
    void findNodes(std::list<Node *> &nodes, const std::wstring &name);
    int countVisibleChildren() const;

    // Moves widget's node under the nodes named name in the subtree (fit: the widget takes the view size of that
    // node's own widget, e.g. a list box's list); returns how many matched.
    int moveIntoWidget(const std::wstring &name, Widget *widget, bool fit);

    // The widgets of the subtree by name (hidden subtrees are skipped). With recursive, the named node's whole subtree
    // is affected, otherwise only the named node's own widget or text shape.
    bool isChecked(const std::wstring &name, bool recursive);
    void setChecked(const std::wstring &name, bool checked, bool fromGroup, bool recursive);
    void setEnabled(const std::wstring &name, bool enabled, bool recursive);
    void setText(const std::wstring &name, const std::wstring &text, bool recursive);

    // Gives the edit boxes named name an input filter (plasma::EditFilter objects).
    void setEditFilter(const std::wstring &name, EditFilter *filter, bool recursive);
    std::wstring getText(const std::wstring &name, bool recursive);

    // The same for every widget or text shape of the subtree.
    bool isChecked();
    void setChecked(bool checked, bool fromGroup);
    void setEnabled(bool enabled);
    void setText(const std::wstring &text);
    void setEditFilter(EditFilter *filter);

    // The text of the first text shape in the subtree.
    std::wstring getText();

    // Connects an event to object->method or object->method(widget) on named widgets in the subtree. With recursive
    // set, connects child widgets too. Connections require an enabled widget.
    template<class T>
    void connectWidget(const std::wstring &name, int event, T *object, void (T::*method)(), bool recursive);
    template<class T>
    void connectWidget(const std::wstring &name, int event, T *object, void (T::*method)(Widget *), bool recursive);
    // Appends the subtree's widgets of type T, in depth-first order (Tab focus order for T = Edit: Workspace). Defined
    // in Widget.h.
    template<class T> void collectWidgets(std::list<T *> &widgets);

    // The "Node.variable" key/value strings.
    const std::string &getVariable(const std::string &key) const;
    void setVariable(const std::string &key, const std::string &value);

private:
    // Animate's steps: clear the keyables' stepped flags, advance, then clear the changed flags of what changed.
    void resetStepped();
    void resetChanged();
    bool animate(int frames, bool matricesChanged, bool force);
    void setFrame(int frame, bool matricesChanged, bool force);

public:
    // The owner's object for this node: the NodeInterface of a design sheet's node
    // (DesignSheet::CreateSheetNode/FindSheetNode), the Sheet of a sheet's frame node (Sheet::Sheet; Workspace's sheet
    // switching).
    void *m_userData;
    Node *m_parent;
    std::list<Node *> m_children;
    Shape *m_shape;
    Transformation *m_transformation;
    Display *m_display;
    Widget *m_widget;
    Widget *m_enclosingWidget;     // This node's widget or the nearest ancestor's.
    Matrix<float> m_matrix;        // The transformations' matrices from the root down.
    Matrix<float> m_inverseMatrix; // Their inverses in reverse order.
    unsigned m_flags;              // NodeFlags ("Node.flags").
    bool m_changed;                // Set by playback when something in the subtree changed.
    RenderSurface *m_cacheSurface; // NODE_CACHED: the subtree is drawn into it (Render); m_changed redraws it.
    std::list<Node *>::iterator m_engineEntry;      // This node's entry in the engine's list.
    std::list<int> m_pendingChildIds;               // Child ids read by Engine::ReadPLAFile; not used in Node.cpp.
    std::list<std::string> m_variableKeys;          // In the order they were first set.
    std::map<std::string, std::string> m_variables; // "Node.variable".
};

}
