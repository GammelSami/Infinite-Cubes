// StampModule: the editor's stamp panel ("module:stamp", captioned "Stamp"): the scale, shear and rotation the
// stamp tool gives its copies, their random jitter, the stamp frequency, and whether copies or instances are
// stamped and which bounds they use ("stamp:boundary:control", "stamp:boundary:shape").
#pragma once
#include "Module.h"
#include "FormatModule.h"

class StampModule : public Module {
public:
    StampModule();

    virtual void build();

    // Shows the values in the widgets after clamping them; zeroes the shear while "stamp:link:shear" is checked.
    void update();

    // "stamp:reset": no scale, shear or rotation.
    void reset();

    // Each value has an edit box (event 19: typed), a drag button ("...:drag", event 12) and steps (events 2 and 4).
    void editScaleX(); // "stamp:scale:x".
    void dragScaleX(plasma::Widget *sender);
    void stepScaleX(plasma::Widget *sender);
    void editScaleY(); // "stamp:scale:y".
    void dragScaleY(plasma::Widget *sender);
    void stepScaleY(plasma::Widget *sender);
    void editShearX(); // "stamp:shear:x".
    void dragShearX(plasma::Widget *sender);
    void stepShearX(plasma::Widget *sender);
    void editShearY(); // "stamp:shear:y".
    void dragShearY(plasma::Widget *sender);
    void stepShearY(plasma::Widget *sender);
    void editRotation(); // "stamp:rotation".
    void dragRotation(plasma::Widget *sender);
    void stepRotation(plasma::Widget *sender);
    void editFrequency(); // "stamp:frequency".
    void dragFrequency(plasma::Widget *sender);
    void stepFrequency(plasma::Widget *sender);
    void editRotationJitter(); // "stamp:rotation:jitter".
    void dragRotationJitter(plasma::Widget *sender);
    void stepRotationJitter(plasma::Widget *sender);
    void editScaleJitter(); // "stamp:scale:jitter".
    void dragScaleJitter(plasma::Widget *sender);
    void stepScaleJitter(plasma::Widget *sender);
    void editPositionJitter(); // "stamp:position:jitter".
    void dragPositionJitter(plasma::Widget *sender);
    void stepPositionJitter(plasma::Widget *sender);

    // The stamp tool sets the values it changed by dragging.
    void setScale(const plasma::Vector<2, float> &scale);
    void setShear(const plasma::Vector<2, float> &shear);
    void setRotation(float rotation);

    // "stamp:copy" (otherwise "stamp:instance"); asked by the stamp tool.
    bool isCopying();
    void useShapeBoundary();   // "stamp:boundary:shape".
    void useControlBoundary(); // "stamp:boundary:control".

    plasma::Vector<2, float> m_scale; // 1 = 100 %.
    plasma::Vector<2, float> m_shear; // 1 = 100 %.
    float m_rotation;                 // Degrees.
    float m_scaleJitter;              // 1 = 100 %.
    float m_rotationJitter;           // Degrees.
    float m_positionJitter;           // Pixels.
    int m_frequency;                  // Percent (0..100).
    bool m_shapeBoundary;
};
