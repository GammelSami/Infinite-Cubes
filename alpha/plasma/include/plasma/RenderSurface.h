// RenderSurface defines offscreen drawing, compositing and blur operations.
#pragma once
#include "plasma/Vector.h"

namespace plasma {

class RenderSurface {
public:
    explicit RenderSurface(bool depthAttachment);
    virtual ~RenderSurface();
    virtual bool resize(int width, int height) = 0;
    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void bindTexture() = 0;
    virtual void draw(const Vector<2, float> &position, const Vector<2, float> &size,
                      const Vector<2, float> &texturePosition, float scale) = 0;
    virtual void drawBlurred(const Vector<2, float> &, const Vector<2, float> &, float, bool, float) {}
    virtual void drawDownsampled(const Vector<2, float> &, const Vector<2, float> &, const Vector<2, float> &, float,
                                 int)
    {}
    virtual void clearRect(const Vector<2, float> &, const Vector<2, float> &, bool) {}
    virtual void clear() {}

protected:
    friend class Widget;
    bool m_linearFiltering;
    Vector<4, float> m_drawColor;
    Vector<4, float> m_clearColor;
    bool m_depthAttachment;
};

}
