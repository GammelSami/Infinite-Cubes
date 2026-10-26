// Widget supports input callbacks, surface rendering and deformation.
#pragma once
#include "plasma/Alignment.h"
#include <map>
#include <string>
#include <list>
#include "plasma/NamedObject.h"
#include "plasma/Vector.h"
#include "plasma/Matrix.h"
#include "plasma/Transformation.h"
namespace plasma {
class Node;
class RenderSurface;
class ScrollSlider;
enum WidgetEvent {
    WIDGET_EVENT_INITIALIZED = 0,
    WIDGET_EVENT_VIEWPORT_CHANGED = 1,
    WIDGET_EVENT_POINTER_PRESSED = 2,
    WIDGET_EVENT_POINTER_RELEASED = 3,
    WIDGET_EVENT_RIGHT_BUTTON_PRESSED = 5,
    WIDGET_EVENT_RIGHT_BUTTON_RELEASED = 6,
    WIDGET_EVENT_POINTER_MOVED = 12,
    WIDGET_EVENT_POINTER_ENTERED = 13,
    WIDGET_EVENT_POINTER_LEFT = 14,
    WIDGET_EVENT_SCROLLED = 17,
    WIDGET_EVENT_FOCUS_GAINED = 18,
    WIDGET_EVENT_FOCUS_LOST = 19,
    WIDGET_EVENT_TEXT_CHANGED = 20,
    WIDGET_EVENT_REPEAT = 21
};
// Per-axis edge captured by pointerPressed and consumed by pointerMoved.
enum WidgetResizeEdge {
    WIDGET_RESIZE_NONE = 0,
    WIDGET_RESIZE_MINIMUM = -1,
    WIDGET_RESIZE_MAXIMUM = 1
};
// Individual flags are bit indices; use (1 << flag) to form a mask.
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

// Whole-field masks; the flags above are bit indices.
enum WidgetFlagMasks {
    WIDGET_FLAGS_NONE = 0
};
enum WidgetKind {
    WIDGET_PLAIN,
    WIDGET_BUTTON,
    WIDGET_EDIT,
    WIDGET_GENERIC,
    WIDGET_LIST
};
class Deformer {
public:
    virtual Vector<2, float> deform(const Vector<2, float> &point, Node *node) = 0;
    virtual Vector<2, float> undeform(const Vector<2, float> &point, Node *node) = 0;
};

class Widget : public NamedObject, public Deformer {
public:
    static Vector<2, float> &getTranslation(Transformation *transformation)
    {
        return transformation->m_translation.value();
    }

    class Connection {
    public:
        explicit Connection(bool enabledOnly)
            : m_enabledOnly(enabledOnly)
        {}
        virtual void invoke() = 0;

        bool m_enabledOnly; // invoked only while the widget is enabled (Engine::sendEvent)
    };
    template<class T> class MemberFunctionConnection : public Connection {
    public:
        typedef void (T::*Method)();
        MemberFunctionConnection(T *object, Method method, bool enabledOnly)
            : Connection(enabledOnly),
              m_object(object),
              m_method(method)
        {}
        virtual void invoke() { (m_object->*m_method)(); }
        T *m_object; // The callback receiver and member-function pointer.
        Method m_method;
    };

    template<class T> void connect(int event, T *object, void (T::*method)(), bool recursive, bool enabledOnly)
    {
        Connection *&connection = m_connections[event];
        // UB: Connection has no virtual destructor.
        if (connection)
            delete connection;
        connection = new MemberFunctionConnection<T>(object, method, enabledOnly);
        if (recursive) {
            std::list<Widget *> children = getChildWidgets();
            for (std::list<Widget *>::iterator it = children.begin(); it != children.end(); ++it)
                (*it)->connect(event, object, method, recursive, enabledOnly);
        }
    }
    Widget *getParentWidget() const;
    std::list<Widget *> getChildWidgets();
    Widget(Engine *engine, Node *node, const std::wstring &name = L"");
    Widget(const Widget &other, Node *node);
    virtual ~Widget();
    virtual void draw() {}
    virtual Vector<2, float> getMinimum() { return Vector<2, float>(0.0f, 0.0f); }
    virtual Vector<2, float> getMaximum() { return Vector<2, float>(0.0f, 0.0f); }
    virtual bool hasBounds() { return false; }
    virtual bool contains(const Vector<2, float> &point) { return false; }
    virtual void surfaceRendered() {}
    virtual void initialize();
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
    virtual void characterEntered(int) {}
    virtual void update() {}
    virtual void popUpItemPressedAnywhere() {}
    virtual void pointerPressedAnywhere() {}
    virtual void pointerReleasedAnywhere();
    virtual void pointerDoubleClickedAnywhere() {}
    virtual void rightButtonPressedAnywhere() {}
    virtual void rightButtonReleasedAnywhere() {}
    virtual void rightButtonDoubleClickedAnywhere() {}
    virtual void middleButtonPressedAnywhere();
    virtual void middleButtonReleasedAnywhere();
    virtual void middleButtonDoubleClickedAnywhere() {}
    virtual void pointerMovedAnywhere() {}
    virtual void pointerWheelAnywhere();
    virtual Widget *clone(Node *node);
    virtual void setEnabled(bool enabled);
    virtual void scrollChanged() {}
    virtual Vector<2, float> deform(const Vector<2, float> &point, Node *node);
    virtual Vector<2, float> undeform(const Vector<2, float> &point, Node *node);

    Node *getNode() const { return m_node; }
    void setNode(Node *node);
    void collectChildWidgets(Node *, std::list<Widget *> &);
    void setGeometry(const Vector<2, float> &, const Vector<2, float> &, const Vector<2, float> &,
                     const Vector<2, float> &, const Matrix<float> &);
    Vector<2, float> getInnerBindPos() const;
    Vector<2, float> getInnerBindSize() const;
    Vector<2, float> getScrollPosition();
    Vector<2, float> getVisibleExtent();
    Vector<2, float> getClampedVisibleExtent();
    bool isPointerOver();
    bool isUnderPointer(Node *);

    Vector<2, float> getPointerOffset();
    Vector<2, float> getPointerInParent();
    Vector<2, float> getPointerMovement();
    void clampToParent();
    void setViewRect(float x, float y, float width, float height, bool notify);
    void setViewRect(const Vector<2, float> &position, const Vector<2, float> &size, bool notify);
    void setViewPosition(const Vector<2, float> &position, bool deferred);
    void propagateResize(Node *node, const Vector<2, float> &offset, const Vector<2, float> &delta, bool notify);
    void updateScrollSliders();
    void setVerticalSlider(ScrollSlider *);
    void setHorizontalSlider(ScrollSlider *);
    Matrix<float> getPlacementMatrix();
    Vector<2, float> getViewPosition();
    Vector<2, float> getContentPosition();
    Vector<2, float> getContentSize();
    float viewX();
    float viewY();
    float viewWidth();
    float getViewHeight();
    float getContentWidth();
    float getContentHeight();
    void moveTo(float x, float y, bool deferred);
    void resizeTo(float width, float height, bool notify);
    Matrix<float> getNodeMatrix();
    Matrix<float> getViewMatrix();
    Matrix<float> getContentMatrix();
    Matrix<float> getViewContentMatrix();
    Vector<2, float> getViewContentPosition();
    Vector<2, float> screenSize();
    Vector<2, float> getViewSize();
    void resize(const Vector<2, float> &, bool notify);

    void setBindMatrix(const Matrix<float> &matrix)
    {
        m_bindMatrix = matrix;
        m_inverseBindMatrix = matrix;
        m_inverseBindMatrix.invert();
    }
    void setCaption(const std::wstring &caption, bool notify);
    void updateCaption();
    void sendEvent(WidgetEvent event);
    bool isMovableX() const { return (m_flags >> WIDGET_SCROLL_HORIZONTALLY) & 1; }
    bool isMovableY() const { return (m_flags >> WIDGET_SCROLL_VERTICALLY) & 1; }
    bool stretchesX() const { return (m_flags >> WIDGET_ADAPT_WIDTH) & 1; }
    bool stretchesY() const { return (m_flags >> WIDGET_ADAPT_HEIGHT) & 1; }
    void setStretchesX(bool stretches)
    {
        if (stretches)
            m_flags |= 1 << WIDGET_ADAPT_WIDTH;
        else
            m_flags &= ~(1 << WIDGET_ADAPT_WIDTH);
    }
    void setStretchesY(bool stretches)
    {
        if (stretches)
            m_flags |= 1 << WIDGET_ADAPT_HEIGHT;
        else
            m_flags &= ~(1 << WIDGET_ADAPT_HEIGHT);
    }
    // Flag bit 6: the widget draws its subtree into m_surface (Node::draw).
    bool rendersToSurface() const { return (m_flags >> WIDGET_RENDER_SURFACE) & 1; }
    // Creates and sizes m_surface for a rendersToSurface widget.
    void updateSurface();
    bool isResizable() const { return (m_flags >> WIDGET_RESIZABLE) & 1; }
    bool isPopUpItem() const { return (m_flags >> WIDGET_POPUP_ITEM) & 1; }
    bool isAlwaysHit() const { return (m_flags >> WIDGET_ALWAYS_HIT) & 1; }
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

    float m_surfaceScale;
    Vector<2, float> m_surfaceOffset;
    int m_surfaceSamples;
    int m_kind;
    RenderSurface *m_surface;
    Vector<2, float> m_surfacePosition;
    Vector<2, float> m_innerBindPos;
    Vector<2, float> m_innerBindSize;
    Vector<2, float> m_bindPos;
    Vector<2, float> m_bindSize;
    Vector<2, float> m_framePos;
    Vector<2, float> m_frameSize;
    Vector<2, float> m_bindSizeMinusInnerSize;
    std::wstring m_caption;
    Vector<2, float> m_moveStart;
    Vector<2, float> m_moveStartTranslation;
    Matrix<float> m_bindMatrix;
    Matrix<float> m_inverseBindMatrix;
    unsigned m_flags;
    int m_verticalAlignment;
    int m_horizontalAlignment;
    bool m_changed;
    Vector<2, int> m_viewportSize;
    ScrollSlider *m_horizontalSlider;
    ScrollSlider *m_verticalSlider;
    Node *m_node;
    Vector<2, signed char> m_resizeEdges;
    std::map<int, Connection *> m_connections;
    std::list<Widget *>::iterator m_engineEntry;
    bool m_enabled;
};
}

#include "plasma/Node.h"
namespace plasma {
template<class T>
void Node::connectWidget(const std::wstring &name, int event, T *object, void (T::*method)(), bool recursive)
{
    if (m_widget && m_widget->getName() == name)
        m_widget->connect(event, object, method, recursive, true);
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->connectWidget(name, event, object, method, recursive);
}
}
