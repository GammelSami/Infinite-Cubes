#include "plasma/ScrollSlider.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"

namespace plasma {

ScrollSlider::ScrollSlider(Engine *engine, Node *node, const std::wstring &name)
    : Button(engine, node, 0, name)
{
    m_direction = SLIDER_VERTICAL;
    m_target = 0;
}

ScrollSlider::ScrollSlider(const ScrollSlider &other, Node *node)
    : Button(other, node),
      m_direction(other.m_direction),
      m_target(0)
{}

ScrollSlider::~ScrollSlider() {}

void ScrollSlider::initialize()
{
    Button::initialize();
    m_target = findTarget(getParentWidget());
    if (m_target) {
        if (m_direction == SLIDER_VERTICAL)
            m_target->setVerticalSlider(this);
        else
            m_target->setHorizontalSlider(this);
    }
    updateSlider();
}

Widget *ScrollSlider::findTarget(Widget *widget)
{
    while (widget) {
        Widget *target = findTargetInSubtree(widget);
        if (target)
            return target;
        widget = widget->getParentWidget();
    }
    return 0;
}

Widget *ScrollSlider::findTargetInSubtree(Widget *widget)
{
    if (!widget)
        return 0;
    if ((m_direction == SLIDER_VERTICAL && widget->isMovableY()) ||
        (m_direction == SLIDER_HORIZONTAL && widget->isMovableX()))
        return widget;
    std::list<Widget *> children = widget->getChildWidgets();
    for (std::list<Widget *>::iterator it = children.begin(); it != children.end(); ++it) {
        Widget *target = findTargetInSubtree(*it);
        if (target)
            return target;
    }
    return 0;
}

void ScrollSlider::pointerPressed()
{
    Button::pointerPressed();
    m_pressPosition = m_node->getMinimum(1 << BOUNDS_SKIP_WIDGETS);
    m_previousPosition = m_pressPosition;
    updateSlider();
}

void ScrollSlider::pointerMoved()
{
    Widget::pointerMoved();
    if (getParentWidget() && (m_engine->m_buttons & (1 << MOUSE_BUTTON_LEFT))) {
        int axis = m_direction != SLIDER_HORIZONTAL;
        float travel = getParentWidget()->getViewSize()[axis] - getViewSize()[axis];
        Vector<2, float> position = getViewPosition();
        Vector<2, float> movement = m_engine->m_pointer - m_engine->m_previousPointer;
        position[axis] = movement[axis] + position[axis];
        if (position[axis] < 0.0f)
            position[axis] = 0.0f;
        if (position[axis] > travel)
            position[axis] = travel;
        setViewPosition(position, true);
        if (m_target && m_target->getParentWidget()) {
            if (getParentWidget()->getViewSize()[axis] > getViewSize()[axis]) {
                Vector<2, float> targetPosition = m_target->getViewPosition();
                targetPosition[axis] =
                    getViewPosition()[axis] / (getParentWidget()->getViewSize()[axis] - getViewSize()[axis]) *
                    (m_target->getParentWidget()->getViewSize()[axis] - m_target->getViewSize()[axis]);
                m_target->setViewPosition(targetPosition, true);
            }
            m_target->clampToParent();
            m_target->scrollChanged();
            Engine::sendEvent(m_target->m_node, WIDGET_EVENT_SCROLLED);
        }
    }
}

Widget *ScrollSlider::clone(Node *node)
{
    return new ScrollSlider(*this, node);
}

void ScrollSlider::setTarget(Widget *target)
{
    m_target = target;
}

void ScrollSlider::parentResized(const Vector<2, float> &offset, const Vector<2, float> &delta)
{
    Widget::parentResized(offset, delta);
    updateSlider();
}

void ScrollSlider::updateSlider()
{
    Widget *target = m_target;
    if (target && getParentWidget() && target->getParentWidget()) {
        int axis = m_direction != SLIDER_HORIZONTAL;
        float ratio = target->getViewSize()[axis] / m_target->getParentWidget()->getViewSize()[axis];
        if (ratio < 1.0f)
            ratio = 1.0f;
        Vector<2, float> size = getViewSize();
        size[axis] = getParentWidget()->getViewSize()[axis] / ratio;
        resize(size);
        float fraction;
        if (m_target->getViewSize()[axis] <= m_target->getParentWidget()->getViewSize()[axis])
            fraction = 0.0f;
        else
            fraction = -m_target->getViewPosition()[axis] /
                       (m_target->getViewSize()[axis] - m_target->getParentWidget()->getViewSize()[axis]);
        setScrollFraction(fraction);
    }
}

void ScrollSlider::setScrollFraction(float fraction)
{
    if (getParentWidget()) {
        if (fraction < 0.0f)
            fraction = 0.0f;
        else if (fraction > 1.0f)
            fraction = 1.0f;
        int axis = m_direction != SLIDER_HORIZONTAL;
        Vector<2, float> position = getViewPosition();
        position[axis] = (getParentWidget()->getViewSize()[axis] - getViewSize()[axis]) * fraction;
        setViewPosition(position, true);
    }
}

}
