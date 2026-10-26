// RenderSurface is the engine's off-screen drawing target. It keeps filtering, colour, clear colour and the requested
// depth-attachment flag.
#pragma once
#include "plasma/Vector.h"

namespace plasma {

class RenderSurface {
public:
    // Requested depth attachment; stored but not read by the D3D11 implementation.
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
    friend class Widget;           // stores false in m_linearFiltering after creating its surface
    bool m_linearFiltering;        // true at construction
    Vector<4, float> m_drawColor;  // 1, 1, 1, 1 at construction; D3D11RenderSurface::draw passes it as the colour
    Vector<4, float> m_clearColor; // 0, 0, 0, 0 at construction; no D3D11 reader (Begin clears to 0)
    bool m_depthAttachment;        // the constructor's argument
};

}
