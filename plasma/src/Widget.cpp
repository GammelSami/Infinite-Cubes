#include "plasma/Widget.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Shape.h"
#include "plasma/RenderSurface.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/Button.h"
#include <algorithm>

namespace plasma {

static const int WheelScrollDistancePerDelta = 10;

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
      m_flags(0),
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
        Engine::deleteRenderSurface(m_surface);
}

void Widget::setNode(Node *node)
{
    if (node)
        node->m_widget = this;
    m_node = node;
}

Widget *Widget::getParentWidget() const
{
    for (Node *node = m_node->m_parent; node; node = node->m_parent) {
        if (node->m_widget)
            return node->m_widget;
    }
    return 0;
}

Vector<2, float> Widget::getPointerOffset()
{
    Vector<2, float> pointer = m_engine->m_pointer;
    return pointer - getContentPosition();
}

Vector<2, float> Widget::getPointerInParent()
{
    Node *parent = m_node->m_parent;
    if (parent) {
        Vector<2, float> position = m_engine->getPointer();
        return parent->m_inverseMatrix * position;
    }
    return m_engine->getPointer();
}

Vector<2, float> Widget::getPointerMovement()
{
    Matrix<float> matrix = getContentMatrix();
    matrix.invert();
    Vector<2, float> movement = m_engine->m_pointer - m_engine->m_previousPointer;
    return Vector<2, float>(movement[0] * matrix.m_elements[0] + movement[1] * matrix.m_elements[4],
                            movement[0] * matrix.m_elements[1] + movement[1] * matrix.m_elements[5]);
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

Widget *Widget::clone(Node *node)
{
    return new Widget(*this, node);
}

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
        setViewRect(position, size);
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
            Engine::sendEvent(m_node, WIDGET_EVENT_SCROLLED);
        }
    }
}

void Widget::pointerWheelAnywhere()
{
    if (isMovableY()) {
        if (getParentWidget() && getParentWidget()->isPointerOver()) {
            int wheel = m_engine->m_wheelDelta;
            moveTo(viewX(), viewY() + WheelScrollDistancePerDelta * wheel, true);
            clampToParent();
            Engine::sendEvent(m_node, WIDGET_EVENT_SCROLLED);
        }
    }
}

void Widget::initialize()
{
    m_viewportSize.m_elements[0] = m_engine->viewportWidth();
    m_viewportSize.m_elements[1] = m_engine->viewportHeight();
    m_bindSizeMinusInnerSize = m_bindSize - m_innerBindSize;
    if (!m_enabled) {
        getNode()->play(L"widget:disable", 0);
        getNode()->setFrameToSequenceEnd(L"widget:disable");
    }
    updateCaption();
}

void Widget::viewportChanged()
{
    if (!getParentWidget()) {
        Vector<2, int> change(m_engine->m_viewportWidth - m_viewportSize.m_elements[0],
                              m_engine->m_viewportHeight - m_viewportSize.m_elements[1]);
        parentResized(Vector<2, float>(0.0f, 0.0f), Vector<2, float>((float)change[0], (float)change[1]));
    }
    m_viewportSize.m_elements[0] = m_engine->m_viewportWidth;
    m_viewportSize.m_elements[1] = m_engine->m_viewportHeight;
}

void Widget::parentResized(const Vector<2, float> &offset, const Vector<2, float> &delta)
{
    if (!m_node->m_parent)
        return;
    if (m_horizontalAlignment == ALIGN_RIGHT && !stretchesX())
        moveTo(viewX() + offset.m_elements[0] + delta.m_elements[0], viewY(), true);
    else if (m_horizontalAlignment == ALIGN_CENTER)
        moveTo(viewX() + delta.m_elements[0] * (double)0.5f, viewY(), true);
    if (m_verticalAlignment == ALIGN_BOTTOM && !stretchesY())
        moveTo(viewX(), viewY() + offset.m_elements[1] + delta.m_elements[1], true);
    else if (m_verticalAlignment == ALIGN_MIDDLE)
        moveTo(viewX(), viewY() + delta.m_elements[1] * (double)0.5f, true);
    Vector<2, float> extent = getPlacementMatrix().transformDirection(m_frameSize);
    if (stretchesX() || stretchesY()) {
        Vector<2, float> size = extent;
        if (stretchesX())
            size.m_elements[0] = extent.m_elements[0] + delta.m_elements[0];
        if (stretchesY())
            size.m_elements[1] = extent.m_elements[1] + delta.m_elements[1];
        resize(size);
    }
    updateScrollSliders();
    if (m_verticalSlider || m_horizontalSlider)
        clampToParent();
}

void Widget::setGeometry(const Vector<2, float> &position, const Vector<2, float> &size, const Vector<2, float> &origin,
                         const Vector<2, float> &extent, const Matrix<float> &matrix)
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

void Widget::setViewRect(const Vector<2, float> &position, const Vector<2, float> &size)
{
    setViewPosition(position, false);
    resize(size);
}

void Widget::resize(const Vector<2, float> &size)
{
    Vector<2, float> before = getClampedVisibleExtent();
    Matrix<float> matrix = getPlacementMatrix();
    matrix.invert();
    // Transforms a copy of size.
    m_frameSize = matrix.transformDirection(Vector<2, float>(size));
    Vector<2, float> change = getClampedVisibleExtent() - before;
    Vector<2, float> delta = getPlacementMatrix().transformDirection(change);
    propagateResize(m_node, Vector<2, float>(0.0f, 0.0f), delta, true);
    resized();
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
    matrix = transformation->m_matrix;
    matrix *= m_inverseBindMatrix;
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

void Widget::setVerticalSlider(ScrollSlider *slider)
{
    m_verticalSlider = slider;
    slider->setTarget(this);
}

void Widget::setHorizontalSlider(ScrollSlider *slider)
{
    m_horizontalSlider = slider;
    slider->setTarget(this);
}

void Widget::updateScrollSliders()
{
    if (m_horizontalSlider)
        m_horizontalSlider->updateSlider();
    if (m_verticalSlider)
        m_verticalSlider->updateSlider();
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

Vector<2, float> Widget::getFrameSize() const
{
    return m_frameSize;
}

Vector<2, float> Widget::getVisibleExtent()
{
    Vector<2, float> extent = m_frameSize + m_innerBindSize;
    return extent - m_bindSize;
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

Vector<2, float> Widget::getBindPos() const
{
    return m_bindPos;
}

Vector<2, float> Widget::getBindSize() const
{
    return m_bindSize;
}

Vector<2, float> Widget::getFramePos() const
{
    return m_framePos;
}

Matrix<float> Widget::getNodeMatrix()
{
    Matrix<float> matrix = m_node->m_transformation->m_matrix;
    matrix *= m_inverseBindMatrix;
    Matrix<float> result = matrix;
    for (Node *node = m_node->m_parent; node; node = node->m_parent) {
        matrix = node->m_transformation->m_matrix;
        matrix *= result;
        result = matrix;
    }
    return result;
}

Matrix<float> Widget::getViewMatrix()
{
    Transformation *view = m_engine->m_root->m_transformation;
    const Matrix<float> &matrix = getNodeMatrix();
    return view->m_inverseMatrix * matrix;
}

Matrix<float> Widget::getMatrixToParentWidget()
{
    Matrix<float> result = m_node->m_transformation->m_matrix;
    result *= m_inverseBindMatrix;
    for (Node *node = m_node->m_parent; node; node = node->m_parent) {
        if (node->m_widget)
            break;
        Matrix<float> matrix = node->m_transformation->m_matrix;
        matrix *= result;
        result = matrix;
    }
    return result;
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

Matrix<float> Widget::getContentMatrix()
{
    Matrix<float> result;
    result.identity();
    result *= getNodeMatrix();
    result.translate(getFramePos());
    return result;
}

Matrix<float> Widget::getViewContentMatrix()
{
    Matrix<float> result;
    result.identity();
    result *= getViewMatrix();
    result.translate(getFramePos());
    return result;
}

Vector<2, float> Widget::getViewPosition()
{
    return transformPoint(getPlacementMatrix(), Vector<2, float>(0.0f, 0.0f));
}

Vector<2, float> Widget::getContentPosition()
{
    return transformPoint(getContentMatrix(), Vector<2, float>(0.0f, 0.0f));
}

Vector<2, float> Widget::getViewContentPosition()
{
    return transformPoint(getViewContentMatrix(), Vector<2, float>(0.0f, 0.0f));
}

Vector<2, float> Widget::getContentSize()
{
    Vector<2, float> size = getClampedVisibleExtent() + m_bindSize;
    return size - m_innerBindSize;
}

Vector<2, float> Widget::getOffsetInParent()
{
    Widget *parent = getParentWidget();
    if (!parent)
        return getContentPosition();
    Vector<2, float> position = getMatrixToParentWidget().transform(getFramePos());
    Vector<2, float> parentPosition = parent->m_inverseBindMatrix.transform(parent->getFramePos());
    return position - parentPosition;
}

Vector<2, float> Widget::getViewSize()
{
    return getPlacementMatrix().transformDirection(getClampedVisibleExtent() + m_bindSize - m_innerBindSize);
}

Vector<2, float> Widget::screenSize()
{
    return getContentMatrix().transformDirection(getClampedVisibleExtent() + m_bindSize - m_innerBindSize);
}

std::list<Widget *> Widget::getChildWidgets()
{
    std::list<Widget *> widgets;
    collectChildWidgets(getNode(), widgets);
    return widgets;
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

// Returns view position and dimensions by indexing the corresponding vectors.
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

float Widget::getViewHeight()
{
    return getViewSize()[1];
}

float Widget::getContentWidth()
{
    return getContentSize().m_elements[0];
}

float Widget::getContentHeight()
{
    return getContentSize().m_elements[1];
}

void Widget::setViewRect(float x, float y, float width, float height)
{
    setViewRect(Vector<2, float>(x, y), Vector<2, float>(width, height));
}

void Widget::moveTo(float x, float y, bool deferred)
{
    setViewPosition(Vector<2, float>(x, y), deferred);
}

void Widget::resizeTo(float width, float height)
{
    resize(Vector<2, float>(width, height));
}

void Widget::setCaption(const std::wstring &caption, bool update)
{
    m_caption = caption;
    if (update)
        updateCaption();
}

void Widget::updateCaption()
{
    if (m_caption.size() && m_node) {
        std::list<Node *> nodes;
        getNode()->findNodes(nodes, L"caption");
        for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            Shape *shape = (*it)->m_shape;
            if (shape && shape->getType() == SHAPE_TEXT)
                static_cast<TextShape *>(shape)->setText(m_caption);
        }
    }
}

bool Widget::isPointerOver()
{
    return isUnderPointer(m_node);
}

bool Widget::isUnderPointer(Node *node)
{
    if (!node->isShown())
        return false;
    if (!node->m_display->m_visibility.value())
        return false;
    if (node->m_widget && node->m_widget != this)
        return false;
    const Vector<2, float> pointer = m_engine->getPointer();
    Vector<2, float> point = transformPoint(node->m_inverseMatrix, pointer);
    Vector<2, float> local = undeform(point, node);
    Shape *shape = node->m_shape;
    if ((shape && shape->contains(local)) || (node->m_widget && node->m_widget->contains(point)))
        return true;
    if (!node->m_display->m_clipping.value()) {
        for (std::list<Node *>::reverse_iterator it = node->m_children.rbegin(); it != node->m_children.rend(); ++it) {
            if (isUnderPointer(*it))
                return true;
        }
    }
    return false;
}

void Widget::notifyTextChanged()
{
    Connection *connection = m_connections[WIDGET_EVENT_TEXT_CHANGED];
    if (connection)
        connection->invoke();
}

// Maps a point of node (a node of this widget's subtree) from the widget's original layout to the scrolled and
// resized one: the content between the position and position + size stretches to the visible extent.
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
    Vector<2, float> fraction;
    if (local.m_elements[0] < m_innerBindPos.m_elements[0]) {
        fraction.m_elements[0] = 0.0f;
    } else {
        float end = m_innerBindSize.m_elements[0] + m_innerBindPos.m_elements[0];
        if (end > local.m_elements[0])
            fraction.m_elements[0] =
                (local.m_elements[0] - m_innerBindPos.m_elements[0]) / m_innerBindSize.m_elements[0];
        else
            fraction.m_elements[0] = 1.0f;
    }
    if (local.m_elements[1] < m_innerBindPos.m_elements[1]) {
        fraction.m_elements[1] = 0.0f;
    } else {
        float end = m_innerBindSize.m_elements[1] + m_innerBindPos.m_elements[1];
        if (end > local.m_elements[1])
            fraction.m_elements[1] =
                (local.m_elements[1] - m_innerBindPos.m_elements[1]) / m_innerBindSize.m_elements[1];
        else
            fraction.m_elements[1] = 1.0f;
    }
    return toWidget * (local + scrollPosition - m_innerBindPos + (extent - m_innerBindSize) * fraction);
}

// The inverse of Deform.
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
    Vector<2, float> fraction;
    if (local[0] < scrollPosition[0])
        fraction[0] = 0.0f;
    else if (local[0] < scrollPosition[0] + extent[0])
        fraction[0] = (local[0] - scrollPosition[0]) / extent[0];
    else
        fraction[0] = 1.0f;
    if (local[1] < scrollPosition[1])
        fraction[1] = 0.0f;
    else if (local[1] < scrollPosition[1] + extent[1])
        fraction[1] = (local[1] - scrollPosition[1]) / extent[1];
    else
        fraction[1] = 1.0f;
    return toWidget.transform(local + m_innerBindPos - scrollPosition + (m_innerBindSize - extent) * fraction);
}

void Widget::setPopUpItem(bool set, bool recursive)
{
    if (set)
        m_flags |= (1 << WIDGET_POPUP_ITEM);
    else
        m_flags &= ~(1 << WIDGET_POPUP_ITEM);
    if (recursive) {
        std::list<Widget *> children = getChildWidgets();
        for (std::list<Widget *>::iterator it = children.begin(); it != children.end(); ++it)
            (*it)->setPopUpItem(set, recursive);
    }
}

void Widget::updateSurface()
{
    if (rendersToSurface()) {
        if (!m_surface) {
            m_surface = m_engine->createRenderSurface(0);
            m_surface->m_linearFiltering = false;
            m_changed = true;

            // The code tests for null only after using the surface.
            if (!m_surface)
                return;
        }
        Vector<2, float> position = getContentPosition();
        if (position != m_surfacePosition) {
            // The code discards this result too.
            screenSize();
            m_surfacePosition = position;
        }
        if (m_surface->resize((int)(screenSize()[0] + (double)0.5f), (int)(screenSize()[1] + (double)0.5f)))
            m_changed = true;
    } else if (m_surface) {
        Engine::deleteRenderSurface(m_surface);
        m_surface = 0;
    }
}
}
