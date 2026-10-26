#include "plasma/PopUpButton.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Display.h"

namespace plasma {

PopUpButton::PopUpButton(Engine *engine, const std::wstring &name)
    : Button(engine, 0, 0, name)
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

void PopUpButton::initialize()
{
    Button::initialize();
    m_popUp = findPopUp();
    if (m_popUp)
        setDisplayVisibility(m_popUp->m_node->m_display, 0);
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

void PopUpButton::pointerReleasedAnywhere()
{
    if (m_checked && m_popUp) {
        Display *display = m_popUp->m_node->m_display;
        if (isDisplayVisible(display) != m_checked)
            setChecked(isDisplayVisible(display), false);
    }
    if (m_popUp && m_updateCaption) {
        Widget *pressedItem = m_engine->m_pressedPopUpItem;
        if (pressedItem && m_popUp->m_node->isAncestorOf(pressedItem->m_node))
            setCaption(pressedItem->m_caption, true);
    }
}

Widget *PopUpButton::clone(Node *node)
{
    return new PopUpButton(*this, node);
}

}
