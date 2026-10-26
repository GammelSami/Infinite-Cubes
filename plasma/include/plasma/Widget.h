// Widget is an interactive element shown by a scene node. It keeps layout rectangles for the view, scrolled
// content and frame, a bind matrix and inverse, alignment and movement flags, caption and event connections. The
// parent widget is found through the node tree; Engine::SendEvent invokes connections by event number.
#pragma once
#include "plasma/Alignment.h"
#include <iosfwd>
#include <list>
#include <map>
#include <string>
#include "plasma/NamedObject.h"
#include "plasma/Engine.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/Vector.h"
#include "plasma/Matrix.h"
#include "plasma/Shape.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/DiscreteAttribute.h"

namespace plasma {

class Node;
class RenderSurface;
class ScrollSlider;

// Widget::m_flags ("Widget.flags"), named after the widget panel's strings.
enum WidgetResizeEdge {
    WIDGET_RESIZE_NONE = 0,
    WIDGET_RESIZE_MINIMUM = -1,
    WIDGET_RESIZE_MAXIMUM = 1
};

enum WidgetFlags {
    WIDGET_SCROLL_HORIZONTALLY = 0,
    WIDGET_SCROLL_VERTICALLY = 1,
    WIDGET_ADAPT_WIDTH = 2,
    WIDGET_ADAPT_HEIGHT = 3,
    WIDGET_POPUP = 4,
    WIDGET_POPUP_ITEM = 5,
    WIDGET_RENDER_SURFACE = 6,
    WIDGET_RESIZABLE = 7,
    WIDGET_ALWAYS_HIT = 8
};

enum WidgetFlagMasks {
    WIDGET_FLAGS_NONE = 0
};

// Widget::m_kind, set by each class's constructor.
// Widget::m_horizontalAlignment / m_verticalAlignment ("Widget.horizontalAlignment", "Widget.verticalAlignment";
// "widget:horizontal:left"... "widget:vertical:center").
enum WidgetKind {
    WIDGET_PLAIN = 0,
    WIDGET_BUTTON = 1, // Button and its subclasses.
    WIDGET_EDIT = 2,
    WIDGET_GENERIC = 3,
    WIDGET_LIST = 4
};

// The event numbers of Widget::m_connections (Connect, Engine::SendEvent).
enum WidgetEvent {
    WIDGET_EVENT_INITIALIZED = 0,      // Engine::InitializeWidgets.
    WIDGET_EVENT_VIEWPORT_CHANGED = 1, // Engine::SetViewport.
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
    WIDGET_EVENT_SCROLLED = 17,     // The widget's content moved (wheel, scroll button, slider).
    WIDGET_EVENT_FOCUS_GAINED = 18, // Engine::SetFocusWidget.
    WIDGET_EVENT_FOCUS_LOST = 19,
    WIDGET_EVENT_TEXT_CHANGED = 20, // Edit, through NotifyTextChanged.
    WIDGET_EVENT_REPEAT = 21        // A held Button repeats.
};

// Maps a point through the widget for hit tests and rendering.
class Deformer {
public:
    virtual Vector<2, float> deform(const Vector<2, float> &point, Node *node) = 0;
    virtual Vector<2, float> undeform(const Vector<2, float> &point, Node *node) = 0;
};

class Widget : public NamedObject, public Deformer {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    // Accessors for derived widgets to use the transformation, display and shape owned by Widget.
    static Vector<2, float> &getTranslation(Transformation *transformation)
    {
        return transformation->m_translation.value();
    }

    static Matrix<float> &getPlacement(Transformation *transformation) { return transformation->m_matrix; }

    static bool isDisplayVisible(Display *display) { return display->m_visibility.value() != 0; }

    static int getShapeType(Shape *shape) { return shape->m_type; }

    static void setDisplayVisibility(Display *display, int shown) { display->m_visibility.value() = shown; }

    static void setDisplayClipping(Display *display, int enabled) { display->m_clipping.value() = enabled; }

    // Transforms a point with perspective division, scaling the coordinates by 1 / w.
    static Vector<2, float> transformPoint(const Matrix<float> &matrix, const Vector<2, float> &point)
    {
        float w = matrix.m_elements[3] * point.m_elements[0] + matrix.m_elements[7] * point.m_elements[1] +
                  matrix.m_elements[15];
        Vector<2, float> transformed(matrix.m_elements[4] * point.m_elements[1] +
                                         point.m_elements[0] * matrix.m_elements[0] + matrix.m_elements[12],
                                     matrix.m_elements[5] * point.m_elements[1] +
                                         matrix.m_elements[1] * point.m_elements[0] + matrix.m_elements[13]);
        return transformed * (1 / w);
    }

    // An event callback without a virtual destructor; connections are released with plain operator delete.
    class Connection {
    public:
        explicit Connection(bool enabledOnly)
            : m_enabledOnly(enabledOnly)
        {}
        virtual void invoke() = 0;

        bool m_enabledOnly; // Invoked only while the widget is enabled.
    };

    // Calls object->method().
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

    // Calls object->method(sender).
    template<class T> class MemberFunctionSenderConnection : public Connection {
    public:
        typedef void (T::*Method)(Widget *sender);
        MemberFunctionSenderConnection(T *object, Widget *sender, Method method, bool enabledOnly)
            : Connection(enabledOnly),
              m_object(object),
              m_sender(sender),
              m_method(method)
        {}
        virtual void invoke() { (m_object->*m_method)(m_sender); }

        T *m_object;
        Widget *m_sender;
        Method m_method;
    };

    Widget(Engine *engine, Node *node, const std::wstring &name);

    // A copy shown with node (Clone).
    Widget(const Widget &other, Node *node);
    virtual ~Widget();

    // Draws the widget after its node's shape, before the children (Node.cpp).
    virtual void draw() {}

    // The node's bounds when it has no shape, if HasBounds (Node::GetMinimum, GetMaximum, GetBounds).
    virtual Vector<2, float> getMinimum() { return Vector<2, float>(0.0f, 0.0f); }
    virtual Vector<2, float> getMaximum() { return Vector<2, float>(0.0f, 0.0f); }
    virtual bool hasBounds() { return false; }

    // The widget's own hit test (Widget::IsUnderPointer asks it after the shape).
    virtual bool contains(const Vector<2, float> &point) { return false; }

    // Called after the subtree is redrawn into m_surface, before the surface ends (Node::Render). No overrides
    // currently use it.
    virtual void surfaceRendered() {}

    // Records the viewport size and m_bindSize - m_innerBindSize, shows the disabled state ("widget:disable" at its
    // end) and updates the caption (Button collects its radio group here).
    virtual void initialize();
    virtual void viewportChanged();

    // Moves an aligned widget when its parent's size changes by delta.
    virtual void parentResized(const Vector<2, float> &offset, const Vector<2, float> &delta);

    // Called at the end of Resize.
    virtual void resized() {}
    // Engine dispatches pointer events to the enabled widget under the pointer and key events to the focused widget.
    // It then sends the corresponding WidgetEvent and calls the matching Anywhere method throughout the widget tree.
    // Right and middle double clicks are never dispatched.
    virtual void pointerPressed();
    virtual void pointerReleased();
    virtual void pointerDoubleClicked() {} // Button: presses again; Edit selects all.
    virtual void rightButtonPressed() {}
    virtual void rightButtonReleased() {}
    virtual void rightButtonDoubleClicked() {}
    virtual void middleButtonPressed() {}
    virtual void middleButtonReleased() {}
    virtual void middleButtonDoubleClicked() {}
    virtual void pointerWheel() {} // The delta is Engine::m_wheelDelta.
    virtual void pointerMoved();
    virtual void pointerEntered() {}
    virtual void pointerLeft() {}
    virtual void keyPressed(unsigned short key) {}  // WM_KEYDOWN.
    virtual void keyReleased(unsigned short key) {} // WM_KEYUP.
    virtual void characterEntered(int) {}           // WM_CHAR; Edit inserts it.

    // Each frame of playback (Node::Animate, SetFrame); Button repeats while pressed.
    virtual void update() {}

    // Sent to every widget before PointerPressedAnywhere when the pressed widget is a pop-up item (WIDGET_POPUP_ITEM).
    virtual void popUpItemPressedAnywhere() {}
    virtual void pointerPressedAnywhere() {}
    virtual void pointerReleasedAnywhere() {}
    virtual void pointerDoubleClickedAnywhere() {}
    virtual void rightButtonPressedAnywhere() {}
    virtual void rightButtonReleasedAnywhere() {}
    virtual void rightButtonDoubleClickedAnywhere() {}

    // Widget's: a middle-button drag over the parent moves a movable widget (captures the pointer here, releases it at
    // the release).
    virtual void middleButtonPressedAnywhere();
    virtual void middleButtonReleasedAnywhere();
    virtual void middleButtonDoubleClickedAnywhere() {}
    virtual void pointerMovedAnywhere() {}
    virtual void
    pointerWheelAnywhere(); // Widget's moves a vertically movable widget by 10 * the wheel delta over its parent (event 17).
    virtual Widget *clone(Node *node);
    virtual void setEnabled(bool enabled);
    virtual void scrollChanged() {}

    // Deformer.
    virtual Vector<2, float> deform(const Vector<2, float> &point, Node *node);
    virtual Vector<2, float> undeform(const Vector<2, float> &point, Node *node);

    bool isMovableX() const { return m_flags & (1 << WIDGET_SCROLL_HORIZONTALLY); }
    bool isMovableY() const { return (m_flags >> WIDGET_SCROLL_VERTICALLY) & 1; }
    bool stretchesX() const { return (m_flags >> WIDGET_ADAPT_WIDTH) & 1; } // Follows the parent's width changes.
    bool stretchesY() const { return (m_flags >> WIDGET_ADAPT_HEIGHT) & 1; }
    // Used by Engine.cpp's PLA reader.
    void setStretchesX(bool stretch);
    void setStretchesY(bool stretch);
    bool rendersToSurface() const { return (m_flags >> WIDGET_RENDER_SURFACE) & 1; }
    bool isResizable() const { return (m_flags >> WIDGET_RESIZABLE) & 1; }

    // WIDGET_POPUP_ITEM: a pop-up menu item, chosen on release.
    bool isPopUpWidget() const { return (m_flags >> WIDGET_POPUP) & 1; }
    bool isPopUpItem() const { return (m_flags >> WIDGET_POPUP_ITEM) & 1; }
    // Bit 8: the whole widget takes the pointer (Node::HitTest).
    bool isAlwaysHit() const { return (m_flags >> WIDGET_ALWAYS_HIT) & 1; }

    void setNode(Node *node);

    // The widget of the nearest ancestor node that has one.
    Widget *getParentWidget() const;
    std::list<Widget *> getChildWidgets();
    void collectChildWidgets(Node *node, std::list<Widget *> &widgets);
    // Removes the event connection from this widget and its descendants. Descendants are visited even when recursive
    // is false.
    void disconnect(int event, bool recursive)
    {
        if (m_connections[event]) {
            delete m_connections[event];
            m_connections.erase(event);
        }
        std::list<Widget *> children = getChildWidgets();
        for (std::list<Widget *>::iterator it = children.begin(); it != children.end(); ++it)
            (*it)->disconnect(event, true);
    }

    // The pointer relative to the content position (in the nodes' space).
    Vector<2, float> getPointerOffset();

    // The pointer in the parent node's space.
    Vector<2, float> getPointerInParent();

    // The pointer's movement since the last event, in the content's space.
    Vector<2, float> getPointerMovement();

    // Keeps the widget inside its parent widget (a scrolled view).
    void clampToParent();

    // Sets the position and the size in the parent's view.
    void setViewRect(const Vector<2, float> &position, const Vector<2, float> &size);
    void resize(const Vector<2, float> &size);

    // Moves the node so that the widget's view position becomes position; unless deferred, the node's matrices are
    // updated at once.
    void setViewPosition(const Vector<2, float> &position, bool deferred);
    void propagateResize(Node *node, const Vector<2, float> &offset, const Vector<2, float> &delta, bool notify);
    void moveTo(float x, float y, bool deferred);
    void setViewRect(float x, float y, float width, float height);
    void resizeTo(float width, float height);
    bool isPointerOver();
    bool isUnderPointer(Node *node);
    void updateSurface();

    // The node's transformations from the root down, with the inverse widget matrix applied first.
    Matrix<float> getNodeMatrix();

    // The same in the root's view.
    Matrix<float> getViewMatrix();

    // The node's transformations up to the nearest node with a widget.
    Matrix<float> getMatrixToParentWidget();

    // Places the widget in its parent's scrolled content.
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

    void setGeometry(const Vector<2, float> &position, const Vector<2, float> &size, const Vector<2, float> &origin,
                     const Vector<2, float> &extent, const Matrix<float> &matrix);
    Vector<2, float> getInnerBindPos() const;
    Vector<2, float> getInnerBindSize() const;
    Vector<2, float> getBindPos() const;
    Vector<2, float> getBindSize() const;
    Vector<2, float> getFramePos() const;
    Vector<2, float> getFrameSize() const;

    // The node showing this widget.
    Node *getNode() const { return m_node; }
    // Forwards named-widget accessors through the node, including checked state and text.
    inline bool isChecked(const std::wstring &name, bool recursive);
    inline void setChecked(const std::wstring &name, bool checked, bool fromGroup, bool recursive);
    inline void setText(const std::wstring &name, const std::wstring &text, bool recursive);
    void setCaption(const std::wstring &caption, bool update);
    void updateCaption();
    void setPopUpItem(bool set, bool recursive);
    void notifyTextChanged();

    void setHorizontalSlider(ScrollSlider *slider);
    void setVerticalSlider(ScrollSlider *slider);
    void updateScrollSliders();

    // Replaces the event connection.
    template<class T> void setConnection(Connection *&connection, T *object, void (T::*method)(), bool enabledOnly);
    // Connects event to object->method(), including child widgets when recursive.
    template<class T> void connect(int event, T *object, void (T::*method)(), bool recursive, bool enabledOnly)
    {
        setConnection(m_connections[event], object, method, enabledOnly);
        if (recursive) {
            std::list<Widget *> children = getChildWidgets();
            for (std::list<Widget *>::iterator it = children.begin(); it != children.end(); ++it)
                (*it)->connect(event, object, method, recursive, enabledOnly);
        }
    }
    // Connects event to object->method(this).
    template<class T>
    void setConnection(Connection *&connection, T *object, void (T::*method)(Widget *), bool enabledOnly);
    // Connects event to object->method(widget), passing the widget itself.
    template<class T> void connect(int event, T *object, void (T::*method)(Widget *), bool recursive, bool enabledOnly)
    {
        setConnection(m_connections[event], object, method, enabledOnly);
        if (recursive) {
            std::list<Widget *> children = getChildWidgets();
            for (std::list<Widget *>::iterator it = children.begin(); it != children.end(); ++it)
                (*it)->connect(event, object, method, recursive, enabledOnly);
        }
    }

    float m_surfaceScale; // 1; the surface's pixels per unit (below 1: a reduced preview; Workspace's export preview).
    Vector<2, float> m_surfaceOffset;          // Translation applied after m_surfaceScale (Node::Render).
    int m_surfaceSamples;                      // 1; samples per pixel when the surface is downsampled (Node::Render).
    int m_kind;                                // A WidgetKind, set by each class's constructor (not copied).
    RenderSurface *m_surface;                  // WIDGET_RENDER_SURFACE: the widget renders into it.
    Vector<2, float> m_surfacePosition;        // Where the surface was last placed (not copied).
    Vector<2, float> m_innerBindPos;           // "Widget.innerBindPos".
    Vector<2, float> m_innerBindSize;          // "Widget.innerBindSize".
    Vector<2, float> m_bindPos;                // "Widget.bindPos".
    Vector<2, float> m_bindSize;               // "Widget.bindSize".
    Vector<2, float> m_framePos;               // "Widget.framePos".
    Vector<2, float> m_frameSize;              // "Widget.frameSize".
    Vector<2, float> m_bindSizeMinusInnerSize; // m_bindSize - m_innerBindSize.
    std::wstring m_caption;                    // Shown by the "caption" text nodes.
    Vector<2, float> m_moveStart;              // The pointer where a move began (not copied).
    Vector<2, float> m_moveStartTranslation;   // The node's translation then.
    Matrix<float> m_bindMatrix;                // "Widget.bindMatrix".
    Matrix<float> m_inverseBindMatrix;
    unsigned m_flags;          // WidgetFlags ("Widget.flags").
    int m_verticalAlignment;   // WidgetAlignment ("Widget.verticalAlignment").
    int m_horizontalAlignment; // WidgetAlignment ("Widget.horizontalAlignment").
    bool m_changed;

    // At Initialize or the last ViewportChanged; not initialised by the normal constructor.
    Vector<2, int> m_viewportSize;
    ScrollSlider *m_horizontalSlider; // A ScrollSlider of direction SLIDER_HORIZONTAL targeting this widget.
    ScrollSlider *m_verticalSlider;
    Node *m_node;
    Vector<2, signed char> m_resizeEdges;        // -1/0/1 per axis while resizing (not initialised).
    std::map<int, Connection *> m_connections;   // By event number.
    std::list<Widget *>::iterator m_engineEntry; // This widget's entry in the engine's list.
    bool m_enabled;
};

}

namespace plasma {

template<class T> void Widget::setConnection(Connection *&connection, T *object, void (T::*method)(), bool enabledOnly)
{
    if (connection)
        delete connection;
    connection = new MemberFunctionConnection<T>(object, method, enabledOnly);
}

template<class T>
void Widget::setConnection(Connection *&connection, T *object, void (T::*method)(Widget *), bool enabledOnly)
{
    if (connection)
        delete connection;
    connection = new MemberFunctionSenderConnection<T>(object, this, method, enabledOnly);
}

template<class T>
void Node::connectWidget(const std::wstring &name, int event, T *object, void (T::*method)(), bool recursive)
{
    if (m_widget && m_widget->getName() == name)
        m_widget->connect(event, object, method, recursive, true);
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->connectWidget(name, event, object, method, recursive);
}

template<class T>
void Node::connectWidget(const std::wstring &name, int event, T *object, void (T::*method)(Widget *), bool recursive)
{
    if (m_widget && m_widget->getName() == name)
        m_widget->connect(event, object, method, recursive, true);
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->connectWidget(name, event, object, method, recursive);
}

inline bool Widget::isChecked(const std::wstring &name, bool recursive)
{
    return getNode()->isChecked(name, recursive);
}

inline void Widget::setChecked(const std::wstring &name, bool checked, bool fromGroup, bool recursive)
{
    getNode()->setChecked(name, checked, fromGroup, recursive);
}

inline void Widget::setText(const std::wstring &name, const std::wstring &text, bool recursive)
{
    getNode()->setText(name, text, recursive);
}

template<class T> void Node::collectWidgets(std::list<T *> &widgets)
{
    if (m_widget) {
        if (T *widget = dynamic_cast<T *>(m_widget))
            widgets.push_back(widget);
    }
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->collectWidgets(widgets);
}

}

// Caption users include Widget.h; TextShape has its own declaration.
#include "plasma/TextShape.h"
