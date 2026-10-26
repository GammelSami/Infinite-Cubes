// PopUpButton controls the visibility of a popup widget through its checked state.
#include "plasma/PopUpButton.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Display.h"

namespace plasma {

PopUpButton::PopUpButton(Engine *engine, Node *node, const std::wstring &name)
    : Button(engine, node, Push, name)
{
    m_popUp = 0;
    m_updateCaption = true;
}

PopUpButton::PopUpButton(const PopUpButton &other, Node *node)
    : Button(other, node),
      m_popUp(0),
      m_updateCaption(other.m_updateCaption)
{}

PopUpButton::~PopUpButton() {}

Widget *PopUpButton::clone(Node *node)
{
    return new PopUpButton(*this, node);
}

Widget *PopUpButton::findPopUp()
{
    std::list<Widget *> children = getChildWidgets();
    for (std::list<Widget *>::iterator it = children.begin(); it != children.end(); ++it) {
        if (isPopUpWidget(*it))
            return *it;
    }
    if (getParentWidget()) {
        std::list<Widget *> siblings = getParentWidget()->getChildWidgets();
        for (std::list<Widget *>::iterator it = siblings.begin(); it != siblings.end(); ++it) {
            if (isPopUpWidget(*it))
                return *it;
        }
    }
    return 0;
}

void PopUpButton::pointerReleasedAnywhere()
{
    Widget::pointerReleasedAnywhere();
    if (m_checked && m_popUp) {
        Display *display = m_popUp->m_node->m_display;
        bool visible = display->m_visibility.value() != DISPLAY_INVISIBLE;
        if (visible != m_checked)
            setChecked(display->m_visibility.value() != DISPLAY_INVISIBLE, false);
    }
    if (m_popUp && m_updateCaption) {
        if (m_engine->m_pressedPopUpItem && m_popUp->m_node->isAncestorOf(m_engine->m_pressedPopUpItem->m_node))
            setCaption(m_engine->m_pressedPopUpItem->m_caption, true);
    }
}

void PopUpButton::initialize()
{
    Button::initialize();
    m_popUp = findPopUp();
    if (m_popUp)
        m_popUp->m_node->m_display->m_visibility.value() = DISPLAY_INVISIBLE;
}

void PopUpButton::pointerPressed()
{
    Button::pointerPressed();
    if (m_checked && m_popUp)
        m_engine->setPopUpWidget(m_popUp, this);
}

void PopUpButton::pointerReleased()
{
    Button::pointerReleased();
    if (m_checked)
        m_engine->setPopUpWidget(m_popUp, 0);
}

}
