// plasma::Button: a widget that plays its node's sequences as the pointer enters, leaves, presses and releases it.
// A toggle or radio button ("Button.type" Toggle, Radio) also keeps a checked state with its own sequences; a
// radio button unchecks the others of its group (the radio buttons among siblings and their children; on a
// NODE_GENERIC node the grouping moves to the grandparent). While pressed it sends event 21 repeatedly, faster and
// faster.
#pragma once
#include <list>
#include <string>
#include "plasma/Widget.h"

namespace plasma {

class Button : public Widget {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    // m_type, PLX chunk "Button.type" ("button:normal", "button:check", "button:radio" in the widget panel).
    enum Type {
        push = 0,
        Toggle = 1,
        Radio = 2,
    };

    Button(Engine *engine, Node *node, int type, const std::wstring &name);
    Button(const Button &other, Node *node);
    virtual ~Button();

    virtual void initialize();
    virtual void pointerPressed();
    virtual void pointerReleased();
    virtual void pointerDoubleClicked(); // Presses again.
    virtual void pointerEntered();
    virtual void pointerLeft();
    virtual void update();
    virtual Widget *clone(Node *node);

    // Shows the checked or unchecked state at once (the sequences' last frames); a radio button being checked unchecks
    // its group unless fromGroup.
    void setChecked(bool checked, bool fromGroup);

    // The widget as a button (WIDGET_BUTTON), or 0.
    static Button *cast(Widget *widget)
    {
        if (!widget || widget->m_kind != WIDGET_BUTTON)
            return 0;
        return static_cast<Button *>(widget);
    }

    // NODE_GENERIC extends radio grouping to the grandparent.
    static bool groupsFromGrandparent(const Node *node) { return (node->m_flags >> NODE_GENERIC) & 1; }

    std::wstring m_pressSequence;
    std::wstring m_releaseSequence;
    std::wstring m_enterSequence;
    std::wstring m_leaveSequence;
    std::wstring m_pressCheckedSequence;
    std::wstring m_releaseCheckedSequence;
    std::wstring m_enterCheckedSequence;
    std::wstring m_leaveCheckedSequence;
    std::list<Button *> m_group; // The other radio buttons.
    bool m_pressed;
    bool m_checked;
    int m_type;       // A Type.
    int m_repeatTime; // Milliseconds towards the next repeat.
    int m_repeatInterval;
};

}

// Widget.cpp historically includes Button.h for the slider interface.
#include "plasma/ScrollSlider.h"
