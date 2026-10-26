// A widget shows itself with a node (m_node; the node keeps it in Node::m_widget) and finds its parent widget through
// the node tree.
#pragma once
#include <list>
#include <map>
#include <string>
#include "plasma/Alignment.h"
#include "plasma/NamedObject.h"
#include "plasma/Vector.h"
#include "plasma/Matrix.h"
#include "plasma/Node.h"

namespace plasma {

class Engine;
class Node;
class RenderSurface;
class ScrollSlider;

// Widget flags stored in m_flags and serialized as Widget.flags.
enum WidgetFlags {
    WIDGET_SCROLL_HORIZONTALLY = 0, // "widget:scroll:horizontally" (isMovableX)
    WIDGET_SCROLL_VERTICALLY = 1,   // "widget:scroll:vertically" (isMovableY)
    WIDGET_ADAPT_WIDTH = 2,         // "widget:adapt:width" (stretchesX)
    WIDGET_ADAPT_HEIGHT = 3,        // "widget:adapt:height" (stretchesY)
    WIDGET_POPUP = 4,               // "widget:popup"
    WIDGET_POPUP_ITEM = 5,          // a pop-up menu item
    WIDGET_RENDER_SURFACE = 6,      // "widget:rendersurface" (rendersToSurface)
    WIDGET_RESIZABLE = 7,           // "widget:resizable"
    WIDGET_ALWAYS_HIT = 8,          // the whole widget takes the pointer (isAlwaysHit)
    WIDGET_RENDER_BEHIND_CHILDREN = 9
};

// Edges used when resizing a widget.
enum WidgetResizeEdge {
    WIDGET_RESIZE_NONE = 0,
    WIDGET_RESIZE_MINIMUM = -1, // the left or top edge
    WIDGET_RESIZE_MAXIMUM = 1   // the right or bottom edge
};

enum WidgetKind {
    WIDGET_PLAIN = 0,
    WIDGET_BUTTON = 1,
    WIDGET_EDIT = 2,
    WIDGET_GENERIC = 3,
    WIDGET_LIST = 4
};

// Event numbers used by Widget::m_connections.
enum WidgetEvent {
    WIDGET_EVENT_INITIALIZED = 0,
    WIDGET_EVENT_VIEWPORT_CHANGED = 1,
    WIDGET_EVENT_POINTER_PRESSED = 2,
    WIDGET_EVENT_POINTER_RELEASED = 3,
    WIDGET_EVENT_POINTER_DOUBLE_CLICKED = 4,
    WIDGET_EVENT_RIGHT_BUTTON_PRESSED = 5,
    WIDGET_EVENT_RIGHT_BUTTON_RELEASED = 6,
    WIDGET_EVENT_MIDDLE_BUTTON_PRESSED = 8,
    WIDGET_EVENT_MIDDLE_BUTTON_RELEASED = 9,
    WIDGET_EVENT_POINTER_WHEEL = 11,
    WIDGET_EVENT_POINTER_MOVED = 12,
    WIDGET_EVENT_POINTER_ENTERED = 13,
    WIDGET_EVENT_POINTER_LEFT = 14,
    WIDGET_EVENT_SCROLLED = 17,
    WIDGET_EVENT_FOCUS_GAINED = 18,
    WIDGET_EVENT_FOCUS_LOST = 19,
    WIDGET_EVENT_TEXT_CHANGED = 20,
    WIDGET_EVENT_REPEAT = 21
};

class Deformer {
public:
    virtual Vector<2, float> deform(const Vector<2, float> &point, Node *node) = 0;
    virtual Vector<2, float> undeform(const Vector<2, float> &point, Node *node) = 0;
};

class Widget : public NamedObject, public Deformer {
public:
    class Connection {
    public:
        explicit Connection(bool enabledOnly)
            : m_enabledOnly(enabledOnly)
        {}
        virtual void invoke() = 0;

        bool m_enabledOnly; // Invoked only while the widget is enabled.
    };

    // T must be complete when instantiating the event connection template so its member-function pointer
    // representation is known.
    template<class T> class MemberFunctionConnection : public Connection {
    public:
        typedef void (T::*Method)();
        MemberFunctionConnection(T *object, Method method, bool enabledOnly)
            : Connection(enabledOnly),
              m_object(object),
              m_method(method)
        {}
        virtual void invoke() { (m_object->*m_method)(); }

        T *m_object;
        Method m_method;
    };

    Widget(Engine *engine, Node *node, const std::wstring &name);
    // Copies the widget onto node, leaving m_kind and m_surfacePosition unset and m_changed false.
    Widget(const Widget &other, Node *node);
    virtual ~Widget();

    // Default widget event handlers.
    virtual void draw() {}
    virtual Vector<2, float> getMinimum() { return Vector<2, float>(0.0f, 0.0f); }
    virtual Vector<2, float> getMaximum() { return Vector<2, float>(0.0f, 0.0f); }
    virtual bool hasBounds() { return false; }
    virtual bool contains(const Vector<2, float> &point) { return false; }
    virtual void surfaceRendered() {}
    // Records the engine's viewport size, m_bindSizeMinusInnerSize, plays "widget:disable" when disabled, updates the
    // caption.
    virtual void initialize();
    // Calls parentResized with the viewport size change (top-level widgets).
    virtual void viewportChanged();
    virtual void parentResized(const Vector<2, float> &offset, const Vector<2, float> &delta);
    virtual void resized() {}
    virtual void pointerPressed();
    virtual void pointerReleased();
    virtual void pointerDoubleClicked() {}
    virtual void rightButtonPressed() {}
    virtual void rightButtonReleased() {}
    virtual void rightButtonDoubleClicked() {}
    virtual void middleButtonPressed() {}
    virtual void middleButtonReleased() {}
    virtual void middleButtonDoubleClicked() {}
    virtual void pointerWheel() {}
    virtual void pointerMoved();
    virtual void pointerEntered() {}
    virtual void pointerLeft() {}
    virtual void keyPressed(unsigned short key) {}
    virtual void keyReleased(unsigned short key) {}
    virtual void characterEntered(wchar_t character) {}
    virtual void update() {}
    virtual void popUpItemPressedAnywhere() {}
    virtual void pointerPressedAnywhere() {}
    virtual void pointerReleasedAnywhere();
    virtual void pointerDoubleClickedAnywhere() {}
    virtual void rightButtonPressedAnywhere() {}
    virtual void rightButtonReleasedAnywhere() {}
    virtual void rightButtonDoubleClickedAnywhere() {}
    // A middle-button drag over the parent moves a movable widget.
    virtual void middleButtonPressedAnywhere();
    virtual void middleButtonReleasedAnywhere();
    virtual void middleButtonDoubleClickedAnywhere() {}
    virtual void pointerMovedAnywhere() {}
    // Moves a vertically movable widget by 10 * the wheel delta, clamps it and sends WIDGET_EVENT_SCROLLED.
    virtual void pointerWheelAnywhere();
    virtual Widget *clone(Node *node);
    // Plays "widget:enable" / "widget:disable" to its end.
    virtual void setEnabled(bool enabled);
    virtual void scrollChanged() {}

    virtual Vector<2, float> deform(const Vector<2, float> &point, Node *node);
    virtual Vector<2, float> undeform(const Vector<2, float> &point, Node *node);

    bool isMovableX() const { return m_flags & (1 << WIDGET_SCROLL_HORIZONTALLY); }
    bool isMovableY() const { return (m_flags >> WIDGET_SCROLL_VERTICALLY) & 1; }
    bool stretchesX() const { return (m_flags >> WIDGET_ADAPT_WIDTH) & 1; }
    bool stretchesY() const { return (m_flags >> WIDGET_ADAPT_HEIGHT) & 1; }
    void setStretchesX(bool stretches)
    {
        if (stretches)
            m_flags |= (1 << WIDGET_ADAPT_WIDTH);
        else
            m_flags &= ~(1 << WIDGET_ADAPT_WIDTH);
    }
    void setStretchesY(bool stretches)
    {
        if (stretches)
            m_flags |= (1 << WIDGET_ADAPT_HEIGHT);
        else
            m_flags &= ~(1 << WIDGET_ADAPT_HEIGHT);
    }
    bool isPopUp() const { return (m_flags >> WIDGET_POPUP) & 1; }          // Used by PopUpButton::findPopUp.
    bool isPopUpItem() const { return (m_flags >> WIDGET_POPUP_ITEM) & 1; } // Engine::pointerPressed
    bool rendersToSurface() const { return (m_flags >> WIDGET_RENDER_SURFACE) & 1; }
    bool isResizable() const { return (m_flags >> WIDGET_RESIZABLE) & 1; }
    bool isAlwaysHit() const { return (m_flags >> WIDGET_ALWAYS_HIT) & 1; }

    Node *getNode() const { return m_node; }

    void setNode(Node *node);
    // The widget of the nearest ancestor node that has one.
    Widget *getParentWidget() const;
    std::list<Widget *> getChildWidgets();
    void collectChildWidgets(Node *node, std::list<Widget *> &widgets);

    Connection *getConnection(int event);
    // Invokes the event connection, if present, regardless of m_enabled. Edit uses it for WIDGET_EVENT_TEXT_CHANGED.
    void invokeConnection(int event);

    Vector<2, float> getPointer();
    // The pointer relative to the content position.
    Vector<2, float> getPointerOffset();
    // The pointer in the parent node's space.
    Vector<2, float> getPointerInParent();
    // The pointer's movement since the last event, in the content's space.
    Vector<2, float> getPointerMovement();
    // Keeps the widget inside its parent widget (a scrolled view).
    void clampToParent();
    void setViewRect(const Vector<2, float> &position, const Vector<2, float> &size, bool notify);
    // Sets m_frameSize so that the view size becomes size, propagates the change (notify: call the child widgets'
    // parentResized), then Resized.
    void resize(const Vector<2, float> &size, bool notify);
    void setViewPosition(const Vector<2, float> &position, bool deferred);
    void propagateResize(Node *node, const Vector<2, float> &offset, const Vector<2, float> &delta, bool notify);
    void moveTo(float x, float y, bool deferred);
    void setViewRect(float x, float y, float width, float height, bool notify);
    void resizeTo(float width, float height, bool notify);
    // Resizes to (width, height) multiplied by the x scale of the parent node's accumulated matrix; used by
    // PreviewWidget.
    void resizeScaled(float width, float height, bool notify);
    bool isPointerOver();
    bool isUnderPointer(Node *node);
    void updateSurface();

    Matrix<float> getNodeMatrix();
    Matrix<float> getViewMatrix();
    Matrix<float> getMatrixToParentWidget();
    Matrix<float> getPlacementMatrix();
    Matrix<float> getContentMatrix();
    Matrix<float> getViewContentMatrix();
    Vector<2, float> getViewPosition();
    Vector<2, float> getContentPosition();
    Vector<2, float> getViewContentPosition();
    Vector<2, float> getOffsetInParent();
    Vector<2, float> getScrollPosition();
    Vector<2, float> getVisibleExtent();
    Vector<2, float> getClampedVisibleExtent();
    Vector<2, float> getContentSize();
    Vector<2, float> getViewSize();
    Vector<2, float> screenSize();
    float viewX();
    float viewY();
    float viewWidth();
    float getViewHeight();
    float getContentWidth();
    float getContentHeight();

    // Sets bind, frame and inner-bind rectangles from origin, extent, position and size.
    void setGeometry(const Vector<2, float> &origin, const Vector<2, float> &extent, const Vector<2, float> &position,
                     const Vector<2, float> &size, const Matrix<float> &matrix);
    Vector<2, float> getInnerBindPos() const;
    Vector<2, float> getInnerBindSize() const;
    Vector<2, float> getFramePos() const;
    Vector<2, float> getFrameSize() const;

    void setCaption(const std::wstring &caption, bool update);
    // Sets the caption on the "caption" text nodes.
    void updateCaption();

    void setHorizontalSlider(ScrollSlider *slider);
    void setVerticalSlider(ScrollSlider *slider);
    void updateScrollSliders();

    // Replaces the connection.
    template<class T> void setConnection(Connection *&connection, T *object, void (T::*method)(), bool enabledOnly)
    {
        if (connection)
            delete connection;
        connection = new MemberFunctionConnection<T>(object, method, enabledOnly);
    }
    template<class T> void connect(int event, T *object, void (T::*method)(), bool recursive, bool enabledOnly)
    {
        setConnection(m_connections[event], object, method, enabledOnly);
        if (recursive) {
            std::list<Widget *> children = getChildWidgets();
            for (std::list<Widget *>::iterator it = children.begin(); it != children.end(); ++it)
                (*it)->connect(event, object, method, recursive, enabledOnly);
        }
    }

    float m_surfaceScale;               // 1; the surface's pixels per unit
    Vector<2, float> m_surfaceOffset;   // (0, 0); translation after m_surfaceScale
    int m_surfaceSamples;               // 1; samples per pixel when downsampled
    int m_kind;                         // a WidgetKind (0 here; not copied)
    RenderSurface *m_surface;           // WIDGET_RENDER_SURFACE target (updateSurface; deleted by the destructor)
    Vector<2, float> m_surfacePosition; // where the surface was last placed (updateSurface; not copied)
    Vector<2, float> m_innerBindPos;    // (0, 0)
    Vector<2, float> m_innerBindSize;   // (1, 1)
    Vector<2, float> m_bindPos;         // (0, 0)
    Vector<2, float> m_bindSize;        // (1, 1)
    Vector<2, float> m_framePos;        // (0, 0)
    Vector<2, float> m_frameSize;       // (1, 1)
    Vector<2, float> m_bindSizeMinusInnerSize; // m_bindSize - m_innerBindSize, set by initialize.
    std::wstring m_caption;                    // Caption displayed by updateCaption on named text nodes.
    Vector<2, float> m_moveStart;            // the pointer where a move began (middleButtonPressedAnywhere; not copied)
    Vector<2, float> m_moveStartTranslation; // the node's translation then (same)
    Matrix<float> m_bindMatrix;              // identity
    Matrix<float> m_inverseBindMatrix;       // Cached node matrix, initially identity.
    unsigned m_flags;                        // WidgetFlags
    int m_verticalAlignment;                 // VerticalAlignment used by parentResized.
    int m_horizontalAlignment;               // HorizontalAlignment used by parentResized.
    bool m_changed;                          // false (propagateResize sets it)
    Vector<2, int> m_viewportSize;
    ScrollSlider *m_horizontalSlider; // Horizontal slider set by setHorizontalSlider.
    ScrollSlider *m_verticalSlider;   // Vertical slider set by setVerticalSlider.
    Node *m_node;
    Vector<2, signed char> m_resizeEdges;        // WidgetResizeEdge per axis
    std::map<int, Connection *> m_connections;   // Event connections indexed by event number.
    std::list<Widget *>::iterator m_engineEntry; // Registration entry in the engine's widget list.
    bool m_enabled; // Enabled state, initially true; changed by setEnabled and read by Engine.
};

// Connects events to widgets by node name; declared in Node.h.
template<class T>
void Node::connectWidget(const std::wstring &name, int event, T *object, void (T::*method)(), bool recursive)
{
    if (m_widget && m_widget->getName() == name)
        m_widget->connect(event, object, method, recursive, true);
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->connectWidget(name, event, object, method, recursive);
}

}
