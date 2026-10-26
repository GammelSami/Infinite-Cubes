#include "plasma/Widget.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Shape.h"
#include "plasma/TextShape.h"
#include "plasma/RenderSurface.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/ScrollSlider.h"
#include "plasma/EditFilter.h"

namespace plasma {

static const int WheelScrollDistancePerDelta = 10;

Widget::Widget(Engine *engine, Node *node, const std::wstring &name)
    : NamedObject(engine, name),
      m_surfaceScale(1.0f),
      m_surfaceOffset(0.0f, 0.0f),
      m_surfaceSamples(1),
      m_kind(WIDGET_PLAIN),
      m_surface(nullptr),
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
      m_horizontalSlider(nullptr),
      m_verticalSlider(nullptr),
      m_node(node),
      m_enabled(true)
{
    m_bindMatrix.identity();
    m_inverseBindMatrix.identity();
    m_engineEntry = engine->addWidget(this);
    if (node)
        node->replaceWidget(this);
}

Widget::Widget(const Widget &other, Node *node)
    : NamedObject(other),
      Deformer(other),
      m_surfaceScale(other.m_surfaceScale),
      m_surfaceOffset(other.m_surfaceOffset),
      m_surfaceSamples(other.m_surfaceSamples),
      m_surface(nullptr),
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
      m_horizontalSlider(nullptr),
      m_verticalSlider(nullptr),
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

void Widget::invokeConnection(int event)
{
    Connection *connection = getConnection(event);
    if (connection)
        connection->invoke();
}

float Widget::getContentHeight()
{
    return getContentSize()[1];
}

Matrix<float> Widget::getMatrixToParentWidget()
{
    Matrix<float> result;
    result = m_node->m_transformation->m_matrix;
    result *= m_inverseBindMatrix;
    for (Node *node = m_node->m_parent; node; node = node->m_parent) {
        if (node->m_widget)
            break;
        result = node->m_transformation->m_matrix * result;
    }
    return result;
}

Vector<2, float> Widget::getOffsetInParent()
{
    Widget *parent = getParentWidget();
    if (!parent)
        return getContentPosition();
    Vector<2, float> position = getMatrixToParentWidget().transform(getFramePos());
    Vector<2, float> parentPosition = parent->m_inverseBindMatrix * parent->getFramePos();
    return position - parentPosition;
}

Vector<2, float> Widget::getContentSize()
{
    Vector<2, float> size = getClampedVisibleExtent() + m_bindSize;
    return size - m_innerBindSize;
}

float Widget::getContentWidth()
{
    return getContentSize()[0];
}

Widget::Connection *Widget::getConnection(int event)
{
    return m_connections[event];
}

Widget *Widget::clone(Node *node)
{
    return new Widget(*this, node);
}

Vector<2, float> Widget::deform(const Vector<2, float> &point, Node *node)
{
    Vector<2, float> scrollPosition = getScrollPosition();
    Vector<2, float> extent = getClampedVisibleExtent();
    if (!(extent != getInnerBindSize()) && !(scrollPosition != getInnerBindPos()))
        return point;
    Matrix<float> widgetInverse;
    getNode()->getInverseMatrix(widgetInverse);
    Matrix<float> widgetMatrix;
    m_node->getMatrix(widgetMatrix);
    Matrix<float> nodeInverse;
    node->getInverseMatrix(nodeInverse);
    Matrix<float> nodeMatrix;
    node->getMatrix(nodeMatrix);
    Matrix<float> toWidget = m_bindMatrix;
    toWidget *= widgetInverse;
    Matrix<float> fromNode = toWidget;
    fromNode *= nodeMatrix;
    Matrix<float> toNode = nodeInverse;
    toNode *= widgetMatrix;
    toWidget = toNode;
    toWidget *= m_inverseBindMatrix;
    Vector<2, float> local = fromNode.transform(point);
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
    Vector<2, float> stretch = growth * Vector<2, float>(x, y);
    Vector<2, float> moved = local + scrollPosition;
    moved = moved - m_innerBindPos;
    moved = moved + stretch;
    return toWidget.transform(moved);
}

void Widget::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (enabled) {
        getNode()->play(L"widget:enable", 0);
        getNode()->setFrame(getNode()->getSequenceEnd(L"widget:enable"));
    } else {
        getNode()->play(L"widget:disable", 0);
        getNode()->setFrame(getNode()->getSequenceEnd(L"widget:disable"));
    }
    getNode()->stop();
}

Vector<2, float> Widget::getFramePos() const
{
    return m_framePos;
}

Vector<2, float> Widget::getFrameSize() const
{
    return m_frameSize;
}

void Widget::collectChildWidgets(Node *node, std::list<Widget *> &widgets)
{
    if (!node->isShown())
        return;
    if (node != m_node && node->m_widget) {
        widgets.push_back(node->widget());
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

Vector<2, float> Widget::getPointer()
{
    return m_engine->m_pointer;
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

bool Widget::isUnderPointer(Node *node)
{
    if (!node->isShown() || !node->m_display->m_visibility.value() || (node->m_widget && node->m_widget != this))
        return false;
    Matrix<float> inverse;
    node->getInverseMatrix(inverse);
    Vector<2, float> point = inverse.transform(m_engine->getPointer());
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

Vector<2, float> Widget::getPointerMovement()
{
    Matrix<float> matrix = getContentMatrix();
    matrix.invert();
    Vector<2, float> movement = m_engine->m_pointer - m_engine->m_previousPointer;
    return matrix.transformDirection(movement);
}

Vector<2, float> Widget::getPointerOffset()
{
    Matrix<float> inverse;
    getNode()->getInverseMatrix(inverse);
    Vector<2, float> offset = m_engine->getPointer() - getContentPosition();
    Vector<2, float> result;
    result[0] = offset[0] * inverse(0, 0) + offset[1] * inverse(0, 1);
    result[1] = inverse(1, 0) * offset[0] + inverse(1, 1) * offset[1];
    return result;
}

void Widget::setViewPosition(const Vector<2, float> &position, bool deferred)
{
    if (!(position != getViewPosition()))
        return;
    bool previousAttributeChangeFlag = getNode()->m_transformation->m_changed;
    // Computes these values and discards the results.
    getContentPosition();
    screenSize();
    Transformation *transformation = getNode()->m_transformation;
    Matrix<float> matrix = getPlacementMatrix();
    matrix *= m_bindMatrix;
    Matrix<float> toNode = matrix;
    toNode *= transformation->m_inverseMatrix;
    toNode.invert();
    transformation = getNode()->m_transformation;
    matrix = transformation->m_matrix;
    matrix *= m_inverseBindMatrix;
    Vector<2, float> origin = matrix.transform(Vector<2, float>(0.0f, 0.0f));
    Vector<2, float> target = toNode.transform(position);
    Vector<2, float> &translation = transformation->m_translation.value();
    translation = target - origin + translation;
    getNode()->m_transformation->attributesChanged(true);
    // Computes these values and discards the results.
    getContentPosition();
    screenSize();
    if (!deferred)
        getNode()->m_transformation->m_changed = previousAttributeChangeFlag;
}

void Widget::moveTo(float x, float y, bool deferred)
{
    setViewPosition(Vector<2, float>(x, y), deferred);
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

void Widget::pointerWheelAnywhere()
{
    if (isMovableY()) {
        if (getParentWidget() && getParentWidget()->isPointerOver()) {
            float y = viewY();
            int wheel = m_engine->m_wheelDelta;
            moveTo(viewX(), y + WheelScrollDistancePerDelta * wheel, true);
            clampToParent();
            m_engine->sendEvent(m_node, WIDGET_EVENT_SCROLLED);
        }
    }
}

void Widget::pointerReleasedAnywhere() {}

void Widget::initialize()
{
    Engine *engine = m_engine;
    m_viewportSize.m_elements[0] = engine->m_viewportWidth;
    m_viewportSize.m_elements[1] = engine->m_viewportHeight;
    m_bindSizeMinusInnerSize = m_bindSize - m_innerBindSize;
    if (!m_enabled) {
        getNode()->play(L"widget:disable", 0);
        getNode()->setFrame(getNode()->getSequenceEnd(L"widget:disable"));
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
    if (isResizable() && (m_engine->m_buttons & (1 << MOUSE_BUTTON_LEFT)) &&
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
    if ((isMovableX() || isMovableY()) && m_engine->isButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector<2, float> movement = getPointerInParent() - m_moveStart;
        if (!isMovableX())
            movement.m_elements[0] = 0.0f;
        if (!isMovableY())
            movement.m_elements[1] = 0.0f;
        m_node->m_transformation->m_translation.value() = m_moveStartTranslation + movement;
        m_node->m_transformation->attributesChanged(true);
        clampToParent();
        m_engine->sendEvent(m_node, WIDGET_EVENT_SCROLLED);
    }
}

void Widget::parentResized(const Vector<2, float> &offset, const Vector<2, float> &delta)
{
    if (!m_node->getParent())
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
    return nullptr;
}

Matrix<float> Widget::getPlacementMatrix()
{
    Matrix<float> result;
    result.identity();
    Widget *parent = getParentWidget();
    if (parent)
        result.translate(parent->getViewContentPosition() * -1.0f);
    else
        m_node->m_parent->getMatrix(result);
    result *= getViewContentMatrix();
    return result;
}

Vector<2, float> Widget::getPointerInParent()
{
    if (m_node->getParent()) {
        Matrix<float> inverse;
        m_node->getParent()->getInverseMatrix(inverse);
        return inverse.transform(m_engine->getPointer());
    }
    return m_engine->m_pointer;
}

Vector<2, float> Widget::getViewPosition()
{
    return getPlacementMatrix().transform(Vector<2, float>(0.0f, 0.0f));
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

void Widget::propagateResize(Node *node, const Vector<2, float> &offset, const Vector<2, float> &delta, bool notify)
{
    if (!node->isShown())
        return;
    Widget *widget = node->m_widget;
    if (widget == this || !widget) {
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
    Vector<2, float> size(width, height);
    Vector<2, float> position(x, y);
    setViewRect(position, size, notify);
}

void Widget::resize(const Vector<2, float> &size, bool notify)
{
    Vector<2, float> before = getClampedVisibleExtent();
    Matrix<float> matrix = getPlacementMatrix();
    matrix.invert();
    Vector<2, float> frameSize = size;
    frameSize = matrix.transformDirection(frameSize);
    m_frameSize = frameSize;
    Vector<2, float> change = getClampedVisibleExtent() - before;
    Vector<2, float> delta = getPlacementMatrix().transformDirection(change);
    propagateResize(m_node, Vector<2, float>(0.0f, 0.0f), delta, notify);
    resized();
}

void Widget::resizeTo(float width, float height, bool notify)
{
    resize(Vector<2, float>(width, height), notify);
}

void Widget::resizeScaled(float width, float height, bool notify)
{
    Matrix<float> matrix;
    m_node->m_parent->getMatrix(matrix);
    float scale = matrix.m_elements[0];
    resize(Vector<2, float>(scale * width, scale * height), notify);
}

// Returns the node matrix relative to the root's inverse transformation.
inline Matrix<float> Widget::getViewMatrix()
{
    Transformation *view = m_engine->m_root->m_transformation;
    return view->m_inverseMatrix * getNodeMatrix();
}

Matrix<float> Widget::getViewContentMatrix()
{
    Matrix<float> result;
    result.identity();
    result *= getViewMatrix();
    result.translate(getFramePos());
    return result;
}

Vector<2, float> Widget::getViewContentPosition()
{
    return getViewContentMatrix().transform(Vector<2, float>(0.0f, 0.0f));
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

Matrix<float> Widget::getContentMatrix()
{
    Matrix<float> result;
    result.identity();
    result *= getNodeMatrix();
    result.translate(getFramePos());
    return result;
}

Vector<2, float> Widget::getContentPosition()
{
    return getContentMatrix().transform(Vector<2, float>(0.0f, 0.0f));
}

Vector<2, float> Widget::screenSize()
{
    return getContentMatrix().transformDirection(m_bindSize + getClampedVisibleExtent() - m_innerBindSize);
}

void Widget::setCaption(const std::wstring &caption, bool update)
{
    m_caption = caption;
    if (update)
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
    return getPlacementMatrix().transformDirection(m_bindSize + getClampedVisibleExtent() - m_innerBindSize);
}

Vector<2, float> Widget::undeform(const Vector<2, float> &point, Node *node)
{
    Vector<2, float> scrollPosition = getScrollPosition();
    Vector<2, float> extent = getClampedVisibleExtent();
    if (!(extent != getInnerBindSize()) && !(scrollPosition != getInnerBindPos()))
        return point;
    Matrix<float> widgetInverse;
    getNode()->getInverseMatrix(widgetInverse);
    Matrix<float> widgetMatrix;
    m_node->getMatrix(widgetMatrix);
    Matrix<float> nodeInverse;
    node->getInverseMatrix(nodeInverse);
    Matrix<float> nodeMatrix;
    node->getMatrix(nodeMatrix);
    Matrix<float> toWidget = m_bindMatrix;
    toWidget *= widgetInverse;
    Matrix<float> fromNode = toWidget;
    fromNode *= nodeMatrix;
    Matrix<float> toNode = nodeInverse;
    toNode *= widgetMatrix;
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
            m_surface->m_linearFiltering = false;
            m_changed = true;
        }
        if (m_surface) {
            Vector<2, float> position = getContentPosition();
            if (position != m_surfacePosition) {
                // Discards the returned value.
                screenSize();
                m_surfacePosition = position;
            }
            if (m_surface->resize((int)(screenSize()[0] + 0.5f), (int)(screenSize()[1] + 0.5f)))
                m_changed = true;
        }
    } else if (m_surface) {
        m_engine->deleteRenderSurface(m_surface);
        m_surface = nullptr;
    }
}

void Widget::updateScrollSliders()
{
    if (m_horizontalSlider)
        m_horizontalSlider->updateSlider();
    if (m_verticalSlider)
        m_verticalSlider->updateSlider();
}

float Widget::viewWidth()
{
    return getViewSize()[0];
}

float Widget::viewX()
{
    return getViewPosition()[0];
}

float Widget::viewY()
{
    return getViewPosition()[1];
}

}
