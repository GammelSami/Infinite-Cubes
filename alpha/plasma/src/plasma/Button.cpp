// Button input and callback handling.
#include "plasma/Button.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"

namespace plasma {
// A held button's repeats (anonymous: this file's own; milliseconds).
namespace {
enum {
    RepeatDelay = 500
};
enum {
    InitialRepeatInterval = 150
};
enum {
    MinimumRepeatInterval = 50
};
enum {
    RepeatIntervalStep = 10
};
}

Button::Button(Engine *engine, Node *node, int type, const std::wstring &name)
    : Widget(engine, node, name),
      m_pressSequence(L"button:press"),
      m_releaseSequence(L"button:release"),
      m_enterSequence(L"button:enter"),
      m_leaveSequence(L"button:leave"),
      m_pressCheckedSequence(L"button:press:checked"),
      m_releaseCheckedSequence(L"button:release:checked"),
      m_enterCheckedSequence(L"button:enter:checked"),
      m_leaveCheckedSequence(L"button:leave:checked"),
      m_pressed(false),
      m_checked(false),
      m_type(type)
{
    m_kind = WIDGET_BUTTON;
}

Button::Button(const Button &other, Node *node)
    : Widget(other, node),
      m_pressSequence(other.m_pressSequence),
      m_releaseSequence(other.m_releaseSequence),
      m_enterSequence(other.m_enterSequence),
      m_leaveSequence(other.m_leaveSequence),
      m_pressCheckedSequence(other.m_pressCheckedSequence),
      m_releaseCheckedSequence(other.m_releaseCheckedSequence),
      m_enterCheckedSequence(other.m_enterCheckedSequence),
      m_leaveCheckedSequence(other.m_leaveCheckedSequence),
      m_pressed(false),
      m_checked(false),
      m_type(other.m_type)
{
    m_kind = WIDGET_BUTTON;
}

Button::~Button() {}

void Button::setChecked(bool checked, bool fromGroup)
{
    if (m_checked == checked || m_type == Push)
        return;
    if (!m_checked && m_type == Radio && !fromGroup) {
        for (std::list<Button *>::iterator it = m_group.begin(); it != m_group.end(); ++it)
            (*it)->setChecked(false, true);
    }
    m_checked = checked;
    int end;
    int frame;
    if (checked) {
        m_node->play(m_enterSequence, 0);
        m_node->play(m_pressSequence, 0);
        m_node->play(m_releaseCheckedSequence, 0);
        m_node->play(m_leaveCheckedSequence, 0);
        end = m_node->getSequenceEnd(m_enterSequence);
        frame = m_node->getSequenceEnd(m_pressSequence);
        if (frame > end)
            end = frame;
        frame = m_node->getSequenceEnd(m_releaseCheckedSequence);
        if (frame > end)
            end = frame;
        frame = m_node->getSequenceEnd(m_leaveCheckedSequence);
    } else {
        m_node->play(m_enterCheckedSequence, 0);
        m_node->play(m_pressCheckedSequence, 0);
        m_node->play(m_releaseSequence, 0);
        m_node->play(m_leaveSequence, 0);
        end = m_node->getSequenceEnd(m_enterCheckedSequence);
        frame = m_node->getSequenceEnd(m_pressCheckedSequence);
        if (frame > end)
            end = frame;
        frame = m_node->getSequenceEnd(m_releaseSequence);
        if (frame > end)
            end = frame;
        frame = m_node->getSequenceEnd(m_leaveSequence);
    }
    if (frame > end)
        end = frame;
    m_node->setFrame(end);
    m_node->stop();
}

Widget *Button::clone(Node *node)
{
    return new Button(*this, node);
}

void Button::initialize()
{
    Widget::initialize();
    m_group.clear();
    Widget *parent = getParentWidget();
    if (groupsFromGrandparent(m_node)) {
        if (!parent)
            goto show;
        parent = parent->getParentWidget();
    }
    if (parent) {
        std::list<Widget *> siblings = parent->getChildWidgets();
        for (std::list<Widget *>::iterator it = siblings.begin(); it != siblings.end(); ++it) {
            if (groupsFromGrandparent(m_node)) {
                std::list<Widget *> children = (*it)->getChildWidgets();
                for (std::list<Widget *>::iterator child = children.begin(); child != children.end(); ++child) {
                    Button *button = cast(*child);
                    if (button && button != this && button->m_type == Radio)
                        m_group.push_back(button);
                }
            }
            Button *button = cast(*it);
            if (button && button != this && button->m_type == Radio)
                m_group.push_back(button);
        }
    }
show:
    if (m_enabled) {
        // This pass selects sequences by mode and repeats the ordinary release; the later pass selects
        // sequences by checked state.
        if (m_type == Push) {
            m_node->play(m_releaseSequence, 0);
            m_node->setFrameToSequenceEnd(m_releaseSequence);
            m_node->stop();
        } else {
            m_node->play(m_pressCheckedSequence, 0);
            m_node->setFrameToSequenceEnd(m_pressCheckedSequence);
            m_node->stop();
            m_node->play(m_releaseCheckedSequence, 0);
            m_node->setFrameToSequenceEnd(m_releaseCheckedSequence);
            m_node->stop();
        }
        m_node->play(m_releaseSequence, 0);
        m_node->setFrameToSequenceEnd(m_releaseSequence);
        m_node->stop();
        m_node->play(m_leaveSequence, 0);
        m_node->setFrameToSequenceEnd(m_leaveSequence);
        m_node->stop();
        if (m_checked) {
            m_node->play(m_pressSequence, 0);
            m_node->setFrameToSequenceEnd(m_pressSequence);
            m_node->stop();
            m_node->play(m_releaseCheckedSequence, 0);
            m_node->setFrameToSequenceEnd(m_releaseCheckedSequence);
            m_node->stop();
            m_node->play(m_leaveCheckedSequence, 0);
            m_node->setFrameToSequenceEnd(m_leaveCheckedSequence);
            m_node->stop();
        }
    }
}

void Button::pointerDoubleClicked()
{
    pointerPressed();
}

void Button::pointerPressed()
{
    m_engine->setPointerCapture(m_node);
    // The accumulator starts at -500 ms; Update repeats only above the current interval, which shrinks
    // by 10 ms toward 50 ms after each repeat.
    m_repeatTime = -RepeatDelay;
    m_repeatInterval = InitialRepeatInterval;
    m_pressed = true;
    if (m_type == Push) {
        m_node->play(m_pressSequence, 0);
        return;
    }
    if (m_type == Radio) {
        for (std::list<Button *>::iterator it = m_group.begin(); it != m_group.end(); ++it) {
            if ((*it)->m_checked) {
                (*it)->m_checked = false;
                (*it)->m_node->play((*it)->m_leaveSequence, 0);
            }
        }
    }
    if (m_checked)
        m_node->play(m_pressCheckedSequence, 0);
    else
        m_node->play(m_pressSequence, 0);
    if (!m_checked || m_type != Radio)
        m_checked = !m_checked;
}

void Button::pointerReleased()
{
    m_engine->releasePointerCapture();
    m_pressed = false;
    if (m_type == Push)
        m_node->play(m_releaseSequence, 0);
    else if (m_checked)
        m_node->play(m_releaseCheckedSequence, 0);
    else
        m_node->play(m_releaseSequence, 0);
}

void Button::pointerEntered()
{
    if (m_type != Push && m_checked)
        m_node->play(m_enterCheckedSequence, 0);
    else
        m_node->play(m_enterSequence, 0);
}

void Button::pointerLeft()
{
    if (m_type != Push && m_checked)
        m_node->play(m_leaveCheckedSequence, 0);
    else
        m_node->play(m_leaveSequence, 0);
}

void Button::update()
{
    if (m_pressed) {
        m_repeatTime += m_engine->m_frameTime;
        while (m_repeatTime > m_repeatInterval) {
            m_engine->sendEvent(m_node, WIDGET_EVENT_REPEAT);
            m_repeatTime -= m_repeatInterval;
            if (m_repeatInterval > MinimumRepeatInterval)
                m_repeatInterval -= RepeatIntervalStep;
        }
    }
}

}
