// Button plays press, release, enter and leave sequences. Toggle and radio buttons also maintain checked-state
// sequences; radio buttons uncheck the other buttons collected in their group by initialize. Holding a button sends
// repeated event 21 with an increasing repeat rate.
#pragma once
#include <list>
#include <string>
#include "plasma/Widget.h"

namespace plasma {

class Engine;
class Node;

class Button : public Widget {
    friend class PLXStreamReader; // Reads Button.type into m_type.
    friend class Engine;          // The PLA reader stores m_type.
public:
    // Button type, supplied as the third constructor argument and read from Button.type in PLX files. Game widgets use
    // Push.
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
    // Inherited unchanged by ScrollButton and ScrollSlider.
    virtual void pointerReleased();
    virtual void pointerDoubleClicked();
    virtual void pointerEntered();
    virtual void pointerLeft();
    virtual void update();
    virtual Widget *clone(Node *node);

    // Shows the checked or unchecked state at once (the sequences' last frames); a radio button being checked unchecks
    // its group unless fromGroup. Called by PopUpButton::pointerReleasedAnywhere.
    void setChecked(bool checked, bool fromGroup);

    // The widget as a button (WIDGET_BUTTON), or 0.
    static Button *cast(Widget *widget)
    {
        if (!widget || widget->m_kind != WIDGET_BUTTON)
            return nullptr;
        return static_cast<Button *>(widget);
    }

protected:
    std::wstring m_pressSequence;          // "button:press"
    std::wstring m_releaseSequence;        // "button:release"
    std::wstring m_enterSequence;          // "button:enter"
    std::wstring m_leaveSequence;          // "button:leave"
    std::wstring m_pressCheckedSequence;   // "button:press:checked"
    std::wstring m_releaseCheckedSequence; // "button:release:checked"
    std::wstring m_enterCheckedSequence;   // "button:enter:checked"
    std::wstring m_leaveCheckedSequence;   // "button:leave:checked"
    std::list<Button *> m_group;           // Other radio buttons, collected by initialize.
    bool m_pressed;                        // set by pointerPressed, cleared by pointerReleased
    bool m_checked;                        // Checked state managed by setChecked.
    int m_type;                            // a Type
    int m_repeatTime;                      // Milliseconds until the next repeat event, maintained by update.
    int m_repeatInterval;
};

}
