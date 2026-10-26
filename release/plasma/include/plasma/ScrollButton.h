// ScrollButton moves a scrollable target while held down, using the configured direction.
#pragma once
#include <string>
#include "plasma/Button.h"

namespace plasma {

class Engine;
class Node;

// ScrollButton::m_direction: which way a press moves the target.
enum ScrollButtonDirection {
    SCROLL_UP = 0,
    SCROLL_DOWN = 1,
    SCROLL_LEFT = 2,
    SCROLL_RIGHT = 3
};

class ScrollButton : public Button {
    friend class PLXStreamReader; // Reads ScrollButton.direction into m_direction.
    friend class Engine;          // The PLA reader sets direction, step and target ID, then resolves the target.
public:
    ScrollButton(Engine *engine, Node *node, const std::wstring &name);
    ScrollButton(const ScrollButton &other, Node *node); // Copied by clone.
    virtual ~ScrollButton();

    // Calls Button::initialize, then finds the target, searching the parent widget's subtree before its ancestors.
    virtual void initialize();
    virtual void pointerPressed();
    // While pressed, moves the target by 20 * frame time * 0.01.
    virtual void update();
    virtual Widget *clone(Node *node);

    // The nearest of widget and its ancestors whose subtree has a target.
    Widget *findTarget(Widget *widget);
    // widget or the first widget of its subtree that is movable along m_direction (WIDGET_SCROLL_VERTICALLY for
    // up/down, WIDGET_SCROLL_HORIZONTALLY for left/right).
    Widget *findTargetInSubtree(Widget *widget);
    void setTarget(Widget *target);

protected:
    int m_direction; // a ScrollButtonDirection
    float m_step;    // 10 from the constructor, the reader's chunk 18; nothing else reads it
    Widget *m_target;
    // Unused scroll state.
    int m_unused2d8;
    // Not initialised by the constructors.
    int m_targetId;
};

}
