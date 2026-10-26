// A slider follows a movable widget along one axis (m_direction).
#pragma once
#include "plasma/Button.h"

namespace plasma {

// ScrollSlider::m_direction ("ScrollSlider.direction").
enum ScrollSliderDirection {
    SLIDER_VERTICAL = 0, // The target's y (Widget::SetVerticalSlider).
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

    int m_direction; // A ScrollSliderDirection.
    Vector<2, float> m_pressPosition;
    Vector<2, float> m_previousPosition;
    Widget *m_target;
    int m_targetId; // The PLA file's id of the target widget (chunk 21); never initialized here.
};

}
