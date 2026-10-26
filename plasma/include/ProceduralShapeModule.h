// ProceduralShapeModule.cpp: the "module:procedural" panel ("Shape"). Its parameters drive ProceduralTool, which
// generates rectangles, polygons, stars and spirals from them.
#pragma once
#include <string>
#include "Module.h"
#include "TextModule.h"
#include "main.h" // NarrowString.

namespace plasma {
class SmoothMeshShape;
}

class ProceduralShapeModule : public Module {
public:
    // "procedural:corners:*" and "procedural:inner:corners:*".
    enum CornerStyle {
        CORNERS_SOFT = 0,
        CORNERS_SHARP = 1,
        CORNERS_ROUND = 2,
        CORNERS_PARAMETRIC = 3
    };

    // "procedural:rectangular", ":radial", ":star" and ":spiral".
    enum Kind {
        KIND_RECTANGULAR = 0,
        KIND_RADIAL = 1,
        KIND_STAR = 2,
        KIND_SPIRAL = 3
    };

    ProceduralShapeModule();

    virtual void build();
    virtual void unitChanged();

    // Shows the parameters in the panel's widgets.
    void updateWidgets();

    // Generates the tool's preview shape from the parameters (vertices, texture coordinates, the current colours and
    // mesh settings).
    void generateShape(plasma::SmoothMeshShape *shape);

    void chooseShapeBoundary();
    void chooseControlBoundary();
    void chooseCornersSoft();
    void chooseCornersSharp();
    void chooseCornersParametric();
    void chooseCornersRound();
    void chooseInnerCornersSoft();
    void chooseInnerCornersSharp();
    void chooseInnerCornersParametric();
    void chooseInnerCornersRound();
    void chooseStar();
    void chooseSpiral();
    void chooseRadial();
    void chooseRectangular();

    void editCorners();
    void dragCorners(plasma::Widget *sender);
    void stepCorners(plasma::Widget *sender);
    void editAngle();
    void dragAngle(plasma::Widget *sender);
    void stepAngle(plasma::Widget *sender);
    void editInnerAngle();
    void dragInnerAngle(plasma::Widget *sender);
    void stepInnerAngle(plasma::Widget *sender);
    void editInnerRadius();
    void dragInnerRadius(plasma::Widget *sender);
    void stepInnerRadius(plasma::Widget *sender);
    void editRoundness();
    void dragRoundness(plasma::Widget *sender);
    void stepRoundness(plasma::Widget *sender);
    void editInnerRoundness();
    void dragInnerRoundness(plasma::Widget *sender);
    void stepInnerRoundness(plasma::Widget *sender);
    void editWidth();
    void dragWidth(plasma::Widget *sender);
    void stepWidth(plasma::Widget *sender);
    void editHeight();
    void dragHeight(plasma::Widget *sender);
    void stepHeight(plasma::Widget *sender);

    // ProceduralTool sets the size and the angle as the user drags a shape out.
    void setSize(const plasma::Vector<2, float> &size);
    void linkWidthHeight();
    void setAngle(float angle);
    void reset();
    void presetStar();
    void presetSpiral();
    void presetCircle();
    void presetTriangle();
    void presetRectangle();

    float m_width;          // "procedural:width", pixels.
    float m_height;         // "procedural:height", pixels.
    int m_corners;          // "procedural:corners", at least 3.
    int m_cornerStyle;      // CornerStyle.
    int m_innerCornerStyle; // CornerStyle of a star's inner corners.
    int m_kind;             // Kind.
    float m_roundness;      // "procedural:roundness", pixels.
    float m_innerRoundness; // "procedural:inner:roundness", pixels.
    float m_angle;          // "procedural:angle", degrees.
    float m_innerAngle;     // "procedural:inner:angle", degrees.
    float m_innerRadius;    // "procedural:inner:radius", shown in percent.
    bool m_shapeBoundary;   // "procedural:boundary:shape" (else ":control").
};
