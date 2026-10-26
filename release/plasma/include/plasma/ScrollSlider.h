// ScrollSlider reflects and controls a scrollable target's position and visible fraction.
#pragma once
#include <string>
#include "plasma/Button.h"
#include "plasma/Vector.h"

namespace plasma {

class Engine;
class Node;

enum ScrollSliderDirection {
    SLIDER_VERTICAL = 0,  // Tracks the target's vertical position.
    SLIDER_HORIZONTAL = 1 // Tracks the target's horizontal position.
};

class ScrollSlider : public Button {
    friend class PLXStreamReader; // Reads ScrollSlider.direction into m_direction.
    friend class Engine;          // The PLA reader sets direction and target ID, then resolves the target.
public:
    ScrollSlider(Engine *engine, Node *node, const std::wstring &name);
    ScrollSlider(const ScrollSlider &other, Node *node); // Copied by clone.
    virtual ~ScrollSlider();

    // Button::initialize, then finds the target (findTargetInSubtree on the parent widget, then findTarget on its
    // parent), registers this slider with it and calls updateSlider.
    virtual void initialize();
    // Widget::parentResized, then updateSlider.
    virtual void parentResized(const Vector<2, float> &offset, const Vector<2, float> &delta);
    // Button::pointerPressed, then records the node's position in m_pressPosition and m_previousPosition and calls
    // updateSlider.
    virtual void pointerPressed();
    virtual void pointerMoved();
    virtual Widget *clone(Node *node);

    // The nearest of widget and its ancestors whose subtree has a target.
    Widget *findTarget(Widget *widget);
    // widget or the first widget of its subtree that is movable along m_direction (WIDGET_SCROLL_VERTICALLY when
    // vertical, WIDGET_SCROLL_HORIZONTALLY when horizontal).
    Widget *findTargetInSubtree(Widget *widget);
    void setTarget(Widget *target);
    // Resizes the slider to the target's visible fraction and places it at the target's scroll position.
    void updateSlider();
    // Places the slider at fraction (clamped to 0..1) of its travel.
    void setScrollFraction(float fraction);

protected:
    int m_direction;                     // a ScrollSliderDirection
    Vector<2, float> m_pressPosition;    // Pointer position when pressed.
    Vector<2, float> m_previousPosition; // set equal to m_pressPosition by pointerPressed; no reader found
    Widget *m_target;
    // Not initialised by the constructors.
    int m_targetId;
};

}
