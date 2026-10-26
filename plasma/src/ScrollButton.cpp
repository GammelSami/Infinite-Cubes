#include "plasma/ScrollButton.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"

namespace plasma {

static const int ScrollHoldDistanceFactor = 20;

ScrollButton::ScrollButton(Engine *engine, Node *node, const std::wstring &name)
    : Button(engine, node, 0, name),
      m_step(10.0f)
{
    m_direction = SCROLL_UP;
    m_target = 0;
}

ScrollButton::ScrollButton(const ScrollButton &other, Node *node)
    : Button(other, node),
      m_direction(other.m_direction),
      m_step(other.m_step),
      m_target(other.m_target)
{}

ScrollButton::~ScrollButton() {}

void ScrollButton::initialize()
{
    Button::initialize();
    m_target = findTarget(getParentWidget());
}

Widget *ScrollButton::findTarget(Widget *widget)
{
    while (widget) {
        Widget *target = findTargetInSubtree(widget);
        if (target)
            return target;
        widget = widget->getParentWidget();
    }
    return 0;
}

Widget *ScrollButton::findTargetInSubtree(Widget *widget)
{
    if (!widget)
        return 0;
    if (((m_direction == SCROLL_UP || m_direction == SCROLL_DOWN) && widget->isMovableY()) ||
        ((m_direction == SCROLL_LEFT || m_direction == SCROLL_RIGHT) && widget->isMovableX()))
        return widget;
    std::list<Widget *> children = widget->getChildWidgets();
    for (std::list<Widget *>::iterator it = children.begin(); it != children.end(); ++it) {
        Widget *target = findTargetInSubtree(*it);
        if (target)
            return target;
    }
    return 0;
}

void ScrollButton::pointerPressed()
{
    Button::pointerPressed();
    if (m_target) {
        switch (m_direction) {
        case SCROLL_UP:
            m_target->moveTo(m_target->viewX(), m_target->viewY() + 20.0, true);
            break;
        case SCROLL_DOWN:
            m_target->moveTo(m_target->viewX(), m_target->viewY() - 20.0, true);
            break;
        case SCROLL_LEFT:
            m_target->moveTo(m_target->viewX() + 20.0, m_target->viewY(), true);
            break;
        case SCROLL_RIGHT:
            m_target->moveTo(m_target->viewX() - 20.0, m_target->viewY(), true);
            break;
        }
        m_target->clampToParent();
        m_target->updateScrollSliders();
        m_unused278 = 0;
        m_target->scrollChanged();
        Engine::sendEvent(m_target->m_node, WIDGET_EVENT_SCROLLED);
    }
}

void ScrollButton::update()
{
    // The code multiplies by the double representation of this float constant.
    const double timeScale = 0.01f;

    // The integer multiplication can overflow before scaling.
    if (m_target && m_pressed) {
        int frameTime = m_engine->m_frameTime;
        switch (m_direction) {
        case SCROLL_UP:
            m_target->moveTo(m_target->viewX(), m_target->viewY() + (ScrollHoldDistanceFactor * frameTime) * timeScale,
                             true);
            break;
        case SCROLL_DOWN:
            m_target->moveTo(m_target->viewX(), m_target->viewY() - (ScrollHoldDistanceFactor * frameTime) * timeScale,
                             true);
            break;
        case SCROLL_LEFT:
            m_target->moveTo(m_target->viewX() + (ScrollHoldDistanceFactor * frameTime) * timeScale, m_target->viewY(),
                             true);
            break;
        case SCROLL_RIGHT:
            m_target->moveTo(m_target->viewX() - (ScrollHoldDistanceFactor * frameTime) * timeScale, m_target->viewY(),
                             true);
            break;
        }
        m_target->clampToParent();
        m_target->updateScrollSliders();
        m_target->scrollChanged();
        Engine::sendEvent(m_target->m_node, WIDGET_EVENT_SCROLLED);
    }
}

Widget *ScrollButton::clone(Node *node)
{
    return new ScrollButton(*this, node);
}

void ScrollButton::setTarget(Widget *target)
{
    m_target = target;
}

}
