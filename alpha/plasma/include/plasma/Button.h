// Button manages pressed and checked states and invokes input callbacks.
#pragma once
#include <list>
#include <string>
#include "plasma/Widget.h"
#include "plasma/Node.h"

namespace plasma {

class Button : public Widget {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    // M_type, PLX chunk "Button.type" ("button:normal", "button:check", "button:radio" in the widget
    // panel).
    enum Type {
        Push = 0,
        Toggle = 1,
        Radio = 2,
    };

    Button(Engine *engine, Node *node, int type, const std::wstring &name);
    Button(const Button &other, Node *node);
    virtual ~Button();

    virtual void initialize();
    virtual void pointerPressed();
    virtual void pointerReleased();
    virtual void pointerDoubleClicked(); // presses again
    virtual void pointerEntered();
    virtual void pointerLeft();
    virtual void update();
    virtual Widget *clone(Node *node);

    // Shows the checked or unchecked state at once (the sequences' last frames); a radio button being
    // checked unchecks its group unless fromGroup.
    void setChecked(bool checked, bool fromGroup);

    // The widget as a button (WIDGET_BUTTON), or 0.
    static Button *cast(Widget *widget)
    {
        if (!widget || widget->m_kind != WIDGET_BUTTON)
            return 0;
        return static_cast<Button *>(widget);
    }

    // A generic parent container (Node bit 4) extends the radio group to its parent.
    static bool groupsFromGrandparent(const Node *node) { return (node->m_flags >> NODE_GENERIC) & 1; }

    std::wstring m_pressSequence;
    std::wstring m_releaseSequence;
    std::wstring m_enterSequence;
    std::wstring m_leaveSequence;
    std::wstring m_pressCheckedSequence;
    std::wstring m_releaseCheckedSequence;
    std::wstring m_enterCheckedSequence;
    std::wstring m_leaveCheckedSequence;
    std::list<Button *> m_group; // the other radio buttons
    bool m_pressed;
    bool m_checked;
    int m_type;       // a Type
    int m_repeatTime; // milliseconds towards the next repeat
    int m_repeatInterval;
};

}
