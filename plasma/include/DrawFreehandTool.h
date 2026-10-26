// The freehand stroke tool (type 9), sampled in the design root's coordinates.
#pragma once
#include <vector>
#include "Tool.h"

class MeshInterface;

class DrawFreehandTool : public Tool {
public:
    DrawFreehandTool();
    virtual void keyPressed(unsigned short key);
    virtual void keyReleased(unsigned short key);
    virtual void pointerMoved(int x, int y);
    virtual void pointerPressed();
    virtual void pointerReleased();
    virtual void draw();

    void pickColor(bool preview);

    MeshInterface *m_meshInterface;
    std::vector<plasma::Vector<2, float>> m_points;
    std::vector<float> m_widths;
    std::vector<plasma::Vector<4, float>> m_fillColors;
    std::vector<plasma::Vector<4, float>> m_strokeColors;

    // The release handler binds these to the PLX extrusionFrontColors/backColors arrays.
    std::vector<plasma::Vector<4, float>> m_extrusionFrontColors;
    std::vector<plasma::Vector<4, float>> m_extrusionBackColors;
    bool m_picking;
    plasma::Vector<4, float> m_savedColor;
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif

// The recursive Douglas-Peucker step of SimplifyPath, defined in WidgetBindOperation.cpp.
void simplifyPathSegment(float tolerance, const std::vector<plasma::Vector<2, float>> &points, int first, int last,
                         std::vector<int> &kept);
