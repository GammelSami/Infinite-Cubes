// A repeating button that scrolls an ancestor's movable child.
#pragma once
#include "plasma/Button.h"

namespace plasma {

// ScrollButton::m_direction ("ScrollButton.direction"; "scrollbutton:up"...): a press or a held repeat moves the
// target by 20 units that way.
enum ScrollButtonDirection {
    SCROLL_UP = 0,
    SCROLL_DOWN = 1,
    SCROLL_LEFT = 2,
    SCROLL_RIGHT = 3
};

class ScrollButton : public Button {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    ScrollButton(Engine *engine, Node *node, const std::wstring &name);
    ScrollButton(const ScrollButton &other, Node *node);
    virtual ~ScrollButton();
    virtual void initialize();
    virtual void pointerPressed();
    virtual void update();
    virtual Widget *clone(Node *node);

    Widget *findTarget(Widget *widget);
    Widget *findTargetInSubtree(Widget *widget);
    void setTarget(Widget *target);

    int m_direction; // A ScrollButtonDirection.
    float m_step;    // Initialized to 10, unused here.
    Widget *m_target;
    int m_unused278; // Cleared at the first press; nothing reads it.
    int m_targetId;  // The PLA file's id of the target widget (chunk 19), resolved by Engine::ReadPLAFile.
};

}
