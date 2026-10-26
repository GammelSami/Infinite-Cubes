// ScrollSlider follows a movable widget along one axis, tracking drag positions and a target identifier.
#pragma once
#include "plasma/Button.h"

namespace plasma {

// ScrollSlider::m_direction ("ScrollSlider.direction").
enum ScrollSliderDirection {
    SLIDER_VERTICAL = 0, // the target's y (Widget::setVerticalSlider)
    SLIDER_HORIZONTAL = 1
};

class ScrollSlider : public Button {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    ScrollSlider(Engine *engine, Node *node, const std::wstring &name);
    ScrollSlider(const ScrollSlider &other, Node *node);
    virtual ~ScrollSlider();
    virtual void initialize();
    virtual void pointerPressed();
    virtual void pointerMoved();
    virtual void parentResized(const Vector<2, float> &offset, const Vector<2, float> &delta);
    virtual Widget *clone(Node *node);

    Widget *findTarget(Widget *widget);
    Widget *findTargetInSubtree(Widget *widget);
    void setTarget(Widget *target);
    void updateSlider();
    void setScrollFraction(float fraction);

    int m_direction; // a ScrollSliderDirection
    Vector<2, float> m_pressPosition;
    Vector<2, float> m_previousPosition;
    Widget *m_target;
    int m_targetId; // the PLA file's id of the target widget (chunk 21); never initialized here
};

}
