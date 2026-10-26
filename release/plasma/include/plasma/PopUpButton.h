// PopUpButton opens and closes a pop-up widget found among its children or siblings.
#pragma once
#include <string>
#include "plasma/Button.h"

namespace plasma {

class Engine;
class Node;

class PopUpButton : public Button {
public:
    PopUpButton(Engine *engine, Node *node, const std::wstring &name);
    PopUpButton(const PopUpButton &other, Node *node); // Copied by clone.
    virtual ~PopUpButton();

    // Button::initialize, then finds the pop-up and hides its display.
    virtual void initialize();
    // Button::pointerPressed, then shows the pop-up when checked.
    virtual void pointerPressed();
    // Calls Button::pointerReleased, then hides the pop-up when checked.
    virtual void pointerReleased();
    virtual void pointerReleasedAnywhere();
    virtual Widget *clone(Node *node);

    // The first child widget with WIDGET_POPUP, else the first such sibling (the parent widget's children), else null.
    Widget *findPopUp();

protected:
    Widget *m_popUp;      // Pop-up widget found by initialize.
    bool m_updateCaption; // true from the constructor; copied by Clone
};

}
