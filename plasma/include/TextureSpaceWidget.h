// The view created by TextureSheet, named "texturespace".
#pragma once
#include "plasma/Widget.h"
class TextureSheet;

class TextureSpaceWidget : public plasma::Widget {
public:
    TextureSpaceWidget(plasma::Engine *engine, TextureSheet *sheet, plasma::Node *node);
    virtual ~TextureSpaceWidget();
    virtual void draw();
    virtual plasma::Vector<2, float> getMinimum();
    virtual plasma::Vector<2, float> getMaximum();
    virtual bool hasBounds();

    TextureSheet *m_sheet;
};

void drawAnnulus(const plasma::Vector<2, float> &center, float innerRadius, float outerRadius,
                 const plasma::Vector<4, float> &outline, const plasma::Vector<4, float> &fill);

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
