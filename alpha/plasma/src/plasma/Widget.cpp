// Widget implements input handling, layout deformation and surface rendering.
#include "plasma/Widget.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Shape.h"
#include "plasma/RenderSurface.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/ScrollSlider.h"
#include "plasma/TextShape.h"

namespace plasma {
// A wheel step scrolls a widget WheelScrollStep pixels (anonymous: this file's own).
namespace {
enum {
    WheelScrollStep = 10
};
}

Vector<2, float> Widget::getVisibleExtent()
{
    Vector<2, float> extent = getInnerBindSize() + m_frameSize;
    return extent - m_bindSize;
}

Vector<2, float> Widget::getPointerMovement()
{
    Matrix<float> matrix = getContentMatrix();
    matrix.invert();
    Vector<2, float> movement = m_engine->m_pointer - m_engine->m_previousPointer;
    return Vector<2, float>(movement[0] * matrix.m_elements[0] + movement[1] * matrix.m_elements[4],
                            movement[0] * matrix.m_elements[1] + movement[1] * matrix.m_elements[5]);
}

void Widget::updateScrollSliders()
{
    if (m_horizontalSlider)
        m_horizontalSlider->updateSlider();
    if (m_verticalSlider)
        m_verticalSlider->updateSlider();
}

Matrix<float> Widget::getPlacementMatrix()
{
    Matrix<float> result;
    result.identity();
    Widget *parent = getParentWidget();
    if (parent)
        result.translate(parent->getViewContentPosition() * -1.0f);
    result *= getViewContentMatrix();
    return result;
}

Vector<2, float> Widget::getContentPosition()
{
    return transformPoint(getContentMatrix(), Vector<2, float>(0.0f, 0.0f));
}

Vector<2, float> Widget::getViewContentPosition()
{
    return transformPoint(getViewContentMatrix(), Vector<2, float>(0.0f, 0.0f));
}

Vector<2, float> Widget::screenSize()
{
    return getContentMatrix().transformDirection(m_bindSize + getClampedVisibleExtent() - m_innerBindSize);
}

Vector<2, float> Widget::getContentSize()
{
    Vector<2, float> size = getClampedVisibleExtent() + m_bindSize;
    return size - m_innerBindSize;
}

float Widget::viewX()
{
    return getViewPosition()[0];
}

float Widget::viewY()
{
    return getViewPosition()[1];
}

float Widget::viewWidth()
{
    return getViewSize()[0];
}

Widget::Widget(Engine *engine, Node *node, const std::wstring &name)
    : NamedObject(engine, name),
      m_surfaceScale(1.0f),
      m_surfaceOffset(0.0f, 0.0f),
      m_surfaceSamples(1),
      m_kind(WIDGET_PLAIN),
      m_surface(0),
      m_surfacePosition(0.0f, 0.0f),
      m_innerBindPos(0.0f, 0.0f),
      m_innerBindSize(1.0f, 1.0f),
      m_bindPos(0.0f, 0.0f),
      m_bindSize(1.0f, 1.0f),
      m_framePos(0.0f, 0.0f),
      m_frameSize(1.0f, 1.0f),
      m_bindSizeMinusInnerSize(0.0f, 0.0f),
      m_flags(WIDGET_FLAGS_NONE),
      m_verticalAlignment(ALIGN_TOP),
      m_horizontalAlignment(ALIGN_LEFT),
      m_changed(false),
      m_horizontalSlider(0),
      m_verticalSlider(0),
      m_node(node),
      m_enabled(true)
{
    m_bindMatrix.identity();
    m_inverseBindMatrix.identity();
    m_engineEntry = engine->addWidget(this);
    if (node)
        node->m_widget = this;
}

// Leaves m_kind uninitialised, (Button's copy sets it).
Widget::Widget(const Widget &other, Node *node)
    : NamedObject(other.m_engine, other.m_name),
      Deformer(other),
      m_surfaceScale(other.m_surfaceScale),
      m_surfaceOffset(other.m_surfaceOffset),
      m_surfaceSamples(other.m_surfaceSamples),
      m_surface(0),
      m_innerBindPos(other.m_innerBindPos),
      m_innerBindSize(other.m_innerBindSize),
      m_bindPos(other.m_bindPos),
      m_bindSize(other.m_bindSize),
      m_framePos(other.m_framePos),
      m_frameSize(other.m_frameSize),
      m_bindSizeMinusInnerSize(other.m_bindSizeMinusInnerSize),
      m_caption(other.m_caption),
      m_bindMatrix(other.m_bindMatrix),
      m_inverseBindMatrix(other.m_inverseBindMatrix),
      m_flags(other.m_flags),
      m_verticalAlignment(other.m_verticalAlignment),
      m_horizontalAlignment(other.m_horizontalAlignment),
      m_changed(false),
      m_viewportSize(other.m_viewportSize),
      m_horizontalSlider(0),
      m_verticalSlider(0),
      m_node(node)
{
    m_engineEntry = m_engine->addWidget(this);
    m_enabled = other.m_enabled;
    if (node)
        node->m_widget = this;
}

Widget::~Widget()
{
    for (std::map<int, Connection *>::iterator it = m_connections.begin(); it != m_connections.end(); ++it) {
        if (it->second)
            delete it->second;
    }
    if (m_surface)
        m_engine->deleteRenderSurface(m_surface);
}

void Widget::clampToParent()
{
    if (!getParentWidget())
        return;
    Vector<2, float> position = getViewPosition();
    if (position.m_elements[0] > 0.0f)
        position.m_elements[0] = 0.0f;
    if (viewWidth() > getParentWidget()->viewWidth()) {
        if (viewWidth() + position.m_elements[0] < getParentWidget()->viewWidth())
            position.m_elements[0] = getParentWidget()->viewWidth() - viewWidth();
    }
    if (position.m_elements[1] > 0.0f)
        position.m_elements[1] = 0.0f;
    if (getViewHeight() > getParentWidget()->getViewHeight()) {
        if (getViewHeight() + position.m_elements[1] < getParentWidget()->getViewHeight())
            position.m_elements[1] = getParentWidget()->getViewHeight() - getViewHeight();
    }
    if (viewWidth() < getParentWidget()->viewWidth())
        position.m_elements[0] = 0.0f;
    if (getViewHeight() < getParentWidget()->getViewHeight())
        position.m_elements[1] = 0.0f;
    setViewPosition(position, true);
    updateScrollSliders();
    scrollChanged();
}

void Widget::setGeometry(const Vector<2, float> &origin, const Vector<2, float> &extent,
                         const Vector<2, float> &position, const Vector<2, float> &size, const Matrix<float> &matrix)
{
    m_innerBindPos = position;
    m_innerBindSize = size;
    m_bindPos = origin;
    m_bindSize = extent;
    m_framePos = origin;
    m_frameSize = extent;
    m_bindSizeMinusInnerSize = m_bindSize - m_innerBindSize;
    m_bindMatrix = matrix;
    m_inverseBindMatrix = matrix;
    m_inverseBindMatrix.invert();
}

void Widget::sendEvent(WidgetEvent event)
{
    Connection *connection = m_connections[event];
    if (connection)
        connection->invoke();
}

float Widget::getContentHeight()
{
    return getContentSize().m_elements[1];
}

float Widget::getContentWidth()
{
    return getContentSize().m_elements[0];
}

Widget *Widget::clone(Node *node)
{
    return new Widget(*this, node);
}

// Maps a point of node (a node of this widget's subtree) from the widget's original layout to the
// scrolled and resized one: the content between the position and position + size stretches to the
// visible extent.
Vector<2, float> Widget::deform(const Vector<2, float> &point, Node *node)
{
    Vector<2, float> scrollPosition = getScrollPosition();
    Vector<2, float> extent = getClampedVisibleExtent();
    if (!(extent != getInnerBindSize()) && !(scrollPosition != getInnerBindPos()))
        return point;
    Matrix<float> toWidget = m_bindMatrix;
    toWidget *= m_node->m_inverseMatrix;
    Matrix<float> fromNode = toWidget;
    fromNode *= node->m_matrix;
    Matrix<float> toNode = node->m_inverseMatrix;
    toNode *= m_node->m_matrix;
    toWidget = toNode;
    toWidget *= m_inverseBindMatrix;
    Vector<2, float> local = fromNode * point;
    float x;
    if (local.m_elements[0] < m_innerBindPos.m_elements[0]) {
        x = 0.0f;
    } else {
        float end = m_innerBindSize.m_elements[0] + m_innerBindPos.m_elements[0];
        if (end > local.m_elements[0])
            x = (local.m_elements[0] - m_innerBindPos.m_elements[0]) / m_innerBindSize.m_elements[0];
        else
            x = 1.0f;
    }
    float y;
    if (local.m_elements[1] < m_innerBindPos.m_elements[1]) {
        y = 0.0f;
    } else {
        float end = m_innerBindSize.m_elements[1] + m_innerBindPos.m_elements[1];
        if (end > local.m_elements[1])
            y = (local.m_elements[1] - m_innerBindPos.m_elements[1]) / m_innerBindSize.m_elements[1];
        else
            y = 1.0f;
    }
    Vector<2, float> growth = extent - m_innerBindSize;
    Vector<2, float> stretch(growth.m_elements[0] * x, growth.m_elements[1] * y);
    Vector<2, float> moved = local + scrollPosition;
    moved = moved - m_innerBindPos;
    moved = moved + stretch;
    return toWidget * moved;
}

void Widget::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (enabled) {
        getNode()->play(L"widget:enable", 0);
        getNode()->setFrameToSequenceEnd(L"widget:enable");
    } else {
        getNode()->play(L"widget:disable", 0);
        getNode()->setFrameToSequenceEnd(L"widget:disable");
    }
    getNode()->stop();
}

void Widget::collectChildWidgets(Node *node, std::list<Widget *> &widgets)
{
    if (!node->isShown())
        return;
    if (node != m_node && node->m_widget) {
        Widget *widget = node->m_widget;
        widgets.push_back(widget);
        return;
    }
    for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        collectChildWidgets(*it, widgets);
}

std::list<Widget *> Widget::getChildWidgets()
{
    std::list<Widget *> widgets;
    collectChildWidgets(getNode(), widgets);
    return widgets;
}

float Widget::getViewHeight()
{
    return getViewSize()[1];
}

Vector<2, float> Widget::getInnerBindPos() const
{
    return m_innerBindPos;
}

Vector<2, float> Widget::getInnerBindSize() const
{
    return m_innerBindSize;
}

Vector<2, float> Widget::getScrollPosition()
{
    Vector<2, float> position = m_framePos + m_innerBindPos;
    return position - m_bindPos;
}

Vector<2, float> Widget::getClampedVisibleExtent()
{
    Vector<2, float> extent = getVisibleExtent();
    Vector<2, float> minimum = m_bindSizeMinusInnerSize + m_innerBindSize - m_bindSize;
    if (minimum.m_elements[0] < 0.0f)
        minimum.m_elements[0] = 0.0f;
    if (extent.m_elements[0] < minimum.m_elements[0])
        extent.m_elements[0] = minimum.m_elements[0];
    if (minimum.m_elements[1] < 0.0f)
        minimum.m_elements[1] = 0.0f;
    if (extent.m_elements[1] < minimum.m_elements[1])
        extent.m_elements[1] = minimum.m_elements[1];
    return extent;
}

bool Widget::isUnderPointer(Node *node)
{
    if (!node->isShown() || !node->m_display->m_visibility.value() || (node->m_widget && node->m_widget != this))
        return false;
    const Vector<2, float> pointer = m_engine->getPointer();
    Vector<2, float> point = transformPoint(node->m_inverseMatrix, pointer);
    Vector<2, float> local = undeform(point, node);
    Shape *shape = node->m_shape;
    if ((shape && shape->contains(local)) || (node->m_widget && node->m_widget->contains(point)))
        return true;
    if (node->m_display->m_clipping.value())
        return false;
    for (std::list<Node *>::reverse_iterator it = node->m_children.rbegin(); it != node->m_children.rend(); ++it) {
        if (isUnderPointer(*it))
            return true;
    }
    return false;
}

bool Widget::isPointerOver()
{
    return isUnderPointer(m_node);
}

Vector<2, float> Widget::getPointerOffset()
{
    return m_engine->getPointer() - getContentPosition();
}

void Widget::setViewPosition(const Vector<2, float> &position, bool deferred)
{
    if (!(position != getViewPosition()))
        return;
    bool previousAttributeChangeFlag = getNode()->m_transformation->m_changed;
    // The code makes these calls and discards their results.
    getContentPosition();
    screenSize();
    Transformation *transformation = getNode()->m_transformation;
    Matrix<float> matrix;
    matrix = getPlacementMatrix();
    matrix *= m_bindMatrix;
    Matrix<float> toNode = matrix;
    toNode *= transformation->m_inverseMatrix;
    toNode.invert();
    transformation = getNode()->m_transformation;
    matrix = transformation->m_matrix * m_inverseBindMatrix;
    Vector<2, float> origin = matrix.transform(Vector<2, float>(0.0f, 0.0f));
    Vector<2, float> target = toNode.transform(position);
    Vector<2, float> movement = target - origin;
    Vector<2, float> &translation = transformation->m_translation.value();
    translation.m_elements[0] += movement.m_elements[0];
    translation.m_elements[1] += movement.m_elements[1];
    getNode()->m_transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    // The code makes these calls and discards their results.
    getContentPosition();
    screenSize();
    if (!deferred) {
        getNode()->m_transformation->m_changed = previousAttributeChangeFlag;
        getNode()->updateMatricesRecursively();
    }
}

void Widget::moveTo(float x, float y, bool deferred)
{
    setViewPosition(Vector<2, float>(x, y), deferred);
}

// Base widget callback does nothing; PopUpButton invokes it from its override.
void Widget::pointerReleasedAnywhere() {}

void Widget::middleButtonPressedAnywhere()
{
    if (isMovableX() || isMovableY()) {
        if (getParentWidget() && getParentWidget()->isPointerOver()) {
            m_engine->setPointerCapture(m_node);
            m_moveStart = getPointerInParent();
            m_moveStartTranslation = m_node->m_transformation->m_translation.value();
        }
    }
}

void Widget::middleButtonReleasedAnywhere()
{
    if (isMovableX() || isMovableY()) {
        if (m_engine->m_pointerCapture == m_node)
            m_engine->releasePointerCapture();
    }
}

void Widget::pointerWheelAnywhere()
{
    if (isMovableY()) {
        if (getParentWidget() && getParentWidget()->isPointerOver()) {
            float y = viewY();
            int wheel = m_engine->m_wheelDelta;
            moveTo(viewX(), y + WheelScrollStep * wheel, true);
            clampToParent();
            m_engine->sendEvent(m_node, WIDGET_EVENT_SCROLLED);
        }
    }
}

void Widget::initialize()
{
    Engine *engine = m_engine;
    m_viewportSize.m_elements[0] = engine->viewportWidth();
    m_viewportSize.m_elements[1] = engine->viewportHeight();
    m_bindSizeMinusInnerSize = m_bindSize - m_innerBindSize;
    if (!m_enabled) {
        getNode()->play(L"widget:disable", 0);
        getNode()->setFrameToSequenceEnd(L"widget:disable");
    }
    updateCaption();
}

void Widget::pointerPressed()
{
    if (isResizable()) {
        m_resizeEdges = Vector<2, signed char>(WIDGET_RESIZE_NONE, WIDGET_RESIZE_NONE);
        Vector<2, float> pointer = getPointerOffset();
        if (pointer.m_elements[0] < getScrollPosition()[0]) {
            m_engine->setPointerCapture(m_node);
            m_resizeEdges.m_elements[0] = WIDGET_RESIZE_MINIMUM;
        }
        if (pointer.m_elements[0] > getScrollPosition()[0] + getClampedVisibleExtent()[0]) {
            m_engine->setPointerCapture(m_node);
            m_resizeEdges.m_elements[0] = WIDGET_RESIZE_MAXIMUM;
        }
        if (pointer.m_elements[1] < getScrollPosition()[1]) {
            m_engine->setPointerCapture(m_node);
            m_resizeEdges.m_elements[1] = WIDGET_RESIZE_MINIMUM;
        }
        if (pointer.m_elements[1] > getScrollPosition()[1] + getClampedVisibleExtent()[1]) {
            m_engine->setPointerCapture(m_node);
            m_resizeEdges.m_elements[1] = WIDGET_RESIZE_MAXIMUM;
        }
    }
}

void Widget::pointerReleased()
{
    Node *capture = m_engine->m_pointerCapture;
    if (capture && capture->m_enclosingWidget == this)
        m_engine->releasePointerCapture();
    m_resizeEdges = Vector<2, signed char>(WIDGET_RESIZE_NONE, WIDGET_RESIZE_NONE);
}

void Widget::pointerMoved()
{
    if (isResizable() && m_engine->isButtonDown(MOUSE_BUTTON_LEFT) &&
        (m_resizeEdges.m_elements[0] || m_resizeEdges.m_elements[1])) {
        Vector<2, float> position = getViewPosition();
        Vector<2, float> size = getViewSize();
        for (int i = 0; i < 2; i++) {
            switch (m_resizeEdges.m_elements[i]) {
            case WIDGET_RESIZE_MINIMUM:
                position.m_elements[i] = getPointerMovement()[i] + position.m_elements[i];
                size.m_elements[i] -= getPointerMovement()[i];
                break;
            case WIDGET_RESIZE_MAXIMUM:
                size.m_elements[i] = getPointerMovement()[i] + size.m_elements[i];
                break;
            }
        }
        setViewRect(position, size, true);
    }
    if (isMovableX() || isMovableY()) {
        if (m_engine->isButtonDown(MOUSE_BUTTON_MIDDLE)) {
            Vector<2, float> movement = getPointerInParent() - m_moveStart;
            if (!isMovableX())
                movement.m_elements[0] = 0.0f;
            if (!isMovableY())
                movement.m_elements[1] = 0.0f;
            m_node->m_transformation->m_translation.value() = m_moveStartTranslation + movement;
            m_node->m_transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
            clampToParent();
            m_engine->sendEvent(m_node, WIDGET_EVENT_SCROLLED);
        }
    }
}

void Widget::parentResized(const Vector<2, float> &offset, const Vector<2, float> &delta)
{
    if (!m_node->m_parent)
        return;
    if (m_horizontalAlignment == ALIGN_RIGHT && !stretchesX())
        moveTo(viewX() + offset.m_elements[0] + delta.m_elements[0], viewY(), true);
    else if (m_horizontalAlignment == ALIGN_CENTER)
        moveTo(viewX() + delta.m_elements[0] * 0.5f, viewY(), true);
    if (m_verticalAlignment == ALIGN_BOTTOM && !stretchesY())
        moveTo(viewX(), viewY() + offset.m_elements[1] + delta.m_elements[1], true);
    else if (m_verticalAlignment == ALIGN_MIDDLE)
        moveTo(viewX(), viewY() + delta.m_elements[1] * 0.5f, true);
    Vector<2, float> extent = getPlacementMatrix().transformDirection(m_frameSize);
    if (stretchesX() || stretchesY()) {
        Vector<2, float> size = extent;
        if (stretchesX())
            size.m_elements[0] = extent.m_elements[0] + delta.m_elements[0];
        if (stretchesY())
            size.m_elements[1] = extent.m_elements[1] + delta.m_elements[1];
        resize(size, true);
    }
    updateScrollSliders();
    if (m_verticalSlider || m_horizontalSlider)
        clampToParent();
}

void Widget::viewportChanged()
{
    if (!getParentWidget()) {
        Vector<2, int> change(m_engine->m_viewportWidth - m_viewportSize.m_elements[0],
                              m_engine->m_viewportHeight - m_viewportSize.m_elements[1]);
        parentResized(Vector<2, float>(0.0f, 0.0f), Vector<2, float>((float)change[0], (float)change[1]));
    }
    Engine *engine = m_engine;
    m_viewportSize.m_elements[0] = engine->m_viewportWidth;
    m_viewportSize.m_elements[1] = engine->m_viewportHeight;
}

Widget *Widget::getParentWidget() const
{
    for (Node *node = m_node->m_parent; node; node = node->m_parent) {
        if (node->m_widget)
            return node->m_widget;
    }
    return 0;
}

Vector<2, float> Widget::getPointerInParent()
{
    Node *parent = m_node->m_parent;
    if (parent) {
        return transformPoint(parent->m_inverseMatrix, m_engine->getPointer());
    }
    return m_engine->getPointer();
}

Vector<2, float> Widget::getViewPosition()
{
    return transformPoint(getPlacementMatrix(), Vector<2, float>(0.0f, 0.0f));
}

void Widget::updateCaption()
{
    if (m_caption.size() && m_node) {
        std::list<Node *> nodes;
        getNode()->findNodes(nodes, L"caption");
        for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            Shape *shape = (*it)->m_shape;
            if (shape && shape->getType() == SHAPE_TEXT)
                static_cast<TextShape *>(shape)->setText(m_caption, *it);
        }
    }
}

void Widget::propagateResize(Node *node, const Vector<2, float> &offset, const Vector<2, float> &delta, bool notify)
{
    if (!node->isShown())
        return;
    Widget *widget = node->m_widget;
    if (widget == this || !widget) {
        node->updateMatrices();
        if (delta.m_elements[0] != 0.0f || delta.m_elements[1] != 0.0f) {
            m_changed = true;
            for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
                propagateResize(*it, offset, delta, notify);
        }
    } else if (notify) {
        widget->parentResized(offset, delta);
    }
}

void Widget::setViewRect(const Vector<2, float> &position, const Vector<2, float> &size, bool notify)
{
    setViewPosition(position, false);
    resize(size, notify);
}

void Widget::setViewRect(float x, float y, float width, float height, bool notify)
{
    setViewRect(Vector<2, float>(x, y), Vector<2, float>(width, height), notify);
}

void Widget::resize(const Vector<2, float> &size, bool notify)
{
    Vector<2, float> before = getClampedVisibleExtent();
    Matrix<float> matrix = getPlacementMatrix();
    matrix.invert();
    m_frameSize = matrix.transformDirection(Vector<2, float>(size));
    Vector<2, float> change = getClampedVisibleExtent() - before;
    Vector<2, float> delta = getPlacementMatrix().transformDirection(change);
    propagateResize(m_node, Vector<2, float>(0.0f, 0.0f), delta, notify);
    resized();
}

void Widget::resizeTo(float width, float height, bool notify)
{
    resize(Vector<2, float>(width, height), notify);
}

Matrix<float> Widget::getViewContentMatrix()
{
    Matrix<float> result;
    result.identity();
    result *= getViewMatrix();
    result.translate(m_framePos);
    return result;
}

Matrix<float> Widget::getNodeMatrix()
{
    Matrix<float> matrix = m_node->m_transformation->m_matrix;
    matrix *= m_inverseBindMatrix;
    Matrix<float> result = matrix;
    for (Node *node = m_node->m_parent; node; node = node->m_parent) {
        result = node->m_transformation->m_matrix * result;
    }
    return result;
}

Matrix<float> Widget::getViewMatrix()
{
    Transformation *view = m_engine->m_root->m_transformation;
    return view->m_inverseMatrix * getNodeMatrix();
}

Matrix<float> Widget::getContentMatrix()
{
    Matrix<float> result;
    result.identity();
    result *= getNodeMatrix();
    result.translate(m_framePos);
    return result;
}

void Widget::setCaption(const std::wstring &caption, bool notify)
{
    m_caption = caption;
    if (notify)
        updateCaption();
}

void Widget::setHorizontalSlider(ScrollSlider *slider)
{
    m_horizontalSlider = slider;
    slider->setTarget(this);
}

void Widget::setNode(Node *node)
{
    if (node)
        node->m_widget = this;
    m_node = node;
}

void Widget::setVerticalSlider(ScrollSlider *slider)
{
    m_verticalSlider = slider;
    slider->setTarget(this);
}

Vector<2, float> Widget::getViewSize()
{
    return getPlacementMatrix().transformDirection(getClampedVisibleExtent() + m_bindSize - m_innerBindSize);
}

Vector<2, float> Widget::undeform(const Vector<2, float> &point, Node *node)
{
    Vector<2, float> scrollPosition = getScrollPosition();
    Vector<2, float> extent = getClampedVisibleExtent();
    if (!(extent != getInnerBindSize()) && !(scrollPosition != getInnerBindPos()))
        return point;
    Matrix<float> toWidget = m_bindMatrix;
    toWidget *= getNode()->m_inverseMatrix;
    Matrix<float> fromNode = toWidget;
    fromNode *= node->m_matrix;
    Matrix<float> toNode = node->m_inverseMatrix;
    toNode *= getNode()->m_matrix;
    toWidget = toNode;
    toWidget *= m_inverseBindMatrix;
    Vector<2, float> local = fromNode.transform(point);
    float x;
    if (local.m_elements[0] < scrollPosition.m_elements[0])
        x = 0.0f;
    else if (extent.m_elements[0] + scrollPosition.m_elements[0] > local.m_elements[0])
        x = (local.m_elements[0] - scrollPosition.m_elements[0]) / extent.m_elements[0];
    else
        x = 1.0f;
    float y;
    if (local.m_elements[1] < scrollPosition.m_elements[1])
        y = 0.0f;
    else if (extent.m_elements[1] + scrollPosition.m_elements[1] > local.m_elements[1])
        y = (local.m_elements[1] - scrollPosition.m_elements[1]) / extent.m_elements[1];
    else
        y = 1.0f;
    Vector<2, float> growth = m_innerBindSize - extent;
    Vector<2, float> stretch(growth.m_elements[0] * x, growth.m_elements[1] * y);
    Vector<2, float> moved = local + m_innerBindPos;
    moved = moved - scrollPosition;
    moved = moved + stretch;
    return toWidget.transform(moved);
}

void Widget::updateSurface()
{
    if (rendersToSurface()) {
        if (!m_surface) {
            m_surface = m_engine->createRenderSurface(false);
            // The code dereferences the surface before checking for null.
            m_surface->m_linearFiltering = false;
            m_changed = true;
            if (!m_surface)
                return;
        }
        Vector<2, float> position = getContentPosition();
        if (position != m_surfacePosition) {
            screenSize();
            m_surfacePosition = position;
        }
        if (m_surface->resize((int)(screenSize()[0] + 0.5f), (int)(screenSize()[1] + 0.5f)))
            m_changed = true;
    } else if (m_surface) {
        m_engine->deleteRenderSurface(m_surface);
        m_surface = 0;
    }
}

}
