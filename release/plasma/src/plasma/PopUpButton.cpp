// Pop-up button construction and interaction.
#include "plasma/PopUpButton.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Display.h"
#include "plasma/EditFilter.h"

namespace plasma {

PopUpButton::PopUpButton(Engine *engine, Node *node, const std::wstring &name)
    : Button(engine, node, 0, name)
{
    m_popUp = nullptr;
    m_updateCaption = true;
}

PopUpButton::PopUpButton(const PopUpButton &other, Node *node)
    : Button(other, node),
      m_popUp(nullptr),
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
        if ((*it)->isPopUp())
            return *it;
    }
    if (getParentWidget()) {
        std::list<Widget *> siblings = getParentWidget()->getChildWidgets();
        for (std::list<Widget *>::iterator it = siblings.begin(); it != siblings.end(); ++it) {
            if ((*it)->isPopUp())
                return *it;
        }
    }
    return nullptr;
}

void PopUpButton::pointerReleasedAnywhere()
{
    Widget::pointerReleasedAnywhere();
    if (m_checked && m_popUp) {
        Display *display = m_popUp->m_node->m_display;
        bool visible = display->m_visibility.value() != 0;
        if (visible != m_checked)
            setChecked(display->m_visibility.value() != 0, false);
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
        m_popUp->m_node->m_display->m_visibility.value() = 0;
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
        m_engine->setPopUpWidget(m_popUp, nullptr);
}

}
