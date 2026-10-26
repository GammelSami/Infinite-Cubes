// ScrollButton repeatedly scrolls an ancestor’s movable child while pressed.
#include "plasma/ScrollButton.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"

namespace plasma {
// A press scrolls the target ScrollStep pixels; held, it scrolls ScrollStep per 100 milliseconds
// (timeScale) (anonymous: this file's own).
namespace {
enum {
    ScrollStep = 20
};
}

ScrollButton::ScrollButton(Engine *engine, Node *node, const std::wstring &name)
    : Button(engine, node, Push, name)
{
    m_direction = SCROLL_UP;
    m_step = 10.0f;
    m_target = 0;
}

ScrollButton::ScrollButton(const ScrollButton &other, Node *node)
    : Button(other, node),
      m_direction(other.m_direction),
      m_step(other.m_step),
      m_target(other.m_target)
{}

ScrollButton::~ScrollButton() {}

Widget *ScrollButton::clone(Node *node)
{
    return new ScrollButton(*this, node);
}

void ScrollButton::initialize()
{
    Button::initialize();
    m_target = findTarget(getParentWidget());
}

void ScrollButton::pointerPressed()
{
    Button::pointerPressed();
    if (m_target) {
        switch (m_direction) {
        case SCROLL_UP:
            m_target->moveTo(m_target->viewX(), m_target->viewY() + ScrollStep, true);
            break;
        case SCROLL_DOWN:
            m_target->moveTo(m_target->viewX(), m_target->viewY() - ScrollStep, true);
            break;
        case SCROLL_LEFT:
            m_target->moveTo(m_target->viewX() + ScrollStep, m_target->viewY(), true);
            break;
        case SCROLL_RIGHT:
            m_target->moveTo(m_target->viewX() - ScrollStep, m_target->viewY(), true);
            break;
        }
        m_target->clampToParent();
        m_target->updateScrollSliders();
        m_unused244 = 0;
        m_target->scrollChanged();
        m_engine->sendEvent(m_target->m_node, WIDGET_EVENT_SCROLLED);
    }
}

void ScrollButton::update()
{
    const float timeScale = 0.01f;
    // Integer multiplication can overflow before scaling.
    if (m_target && m_pressed) {
        switch (m_direction) {
        case SCROLL_UP: {
            int frameTime = m_engine->m_frameTime;
            m_target->moveTo(m_target->viewX(), m_target->viewY() + (ScrollStep * frameTime) * timeScale, true);
            break;
        }
        case SCROLL_DOWN: {
            int frameTime = m_engine->m_frameTime;
            m_target->moveTo(m_target->viewX(), m_target->viewY() - (ScrollStep * frameTime) * timeScale, true);
            break;
        }
        case SCROLL_LEFT: {
            int frameTime = m_engine->m_frameTime;
            m_target->moveTo(m_target->viewX() + (ScrollStep * frameTime) * timeScale, m_target->viewY(), true);
            break;
        }
        case SCROLL_RIGHT: {
            int frameTime = m_engine->m_frameTime;
            m_target->moveTo(m_target->viewX() - (ScrollStep * frameTime) * timeScale, m_target->viewY(), true);
            break;
        }
        }
        m_target->clampToParent();
        m_target->updateScrollSliders();
        m_target->scrollChanged();
        m_engine->sendEvent(m_target->m_node, WIDGET_EVENT_SCROLLED);
    }
}

Widget *ScrollButton::findTarget(Widget *widget)
{
    if (!widget)
        return 0;
    Widget *target = findTargetInSubtree(widget);
    if (target)
        return target;
    return findTarget(widget->getParentWidget());
}

Widget *ScrollButton::findTargetInSubtree(Widget *widget)
{
    if (!widget)
        return 0;
    if ((m_direction == SCROLL_UP || m_direction == SCROLL_DOWN) && widget->isMovableY())
        return widget;
    if ((m_direction == SCROLL_LEFT || m_direction == SCROLL_RIGHT) && widget->isMovableX())
        return widget;
    std::list<Widget *> children = widget->getChildWidgets();
    for (std::list<Widget *>::iterator it = children.begin(); it != children.end(); ++it) {
        Widget *target = findTargetInSubtree(*it);
        if (target)
            return target;
    }
    return 0;
}

void ScrollButton::setTarget(Widget *target)
{
    m_target = target;
}

}
