// plasma::RenderSurface: an off-screen render target (the abstract base of GLRenderSurface, which owns the GL
// textures and the framebuffer object).
#pragma once
#include "plasma/Vector.h"

class Sheet;
class Module;
class Workspace;
class ExportModule;

namespace plasma {

class RenderSurface {
public:
    RenderSurface();
    virtual ~RenderSurface();

    // Clamps the size to at least 1x1; false when it did not change.
    virtual bool resize(int width, int height) = 0;
    virtual void begin() = 0;       // Renders into the surface.
    virtual void end() = 0;         // Renders to the window again.
    virtual void bindTexture() = 0; // Binds the colour texture.

    // Draws the surface's texture from texturePosition into the rectangle, undoing the alpha premultiplication; the
    // texture is scaled by scale.
    virtual void draw(const Vector<2, float> &position, const Vector<2, float> &size,
                      const Vector<2, float> &texturePosition, float scale) = 0;

    // Draws the surface blurred along one axis (radius in pixels).
    virtual void drawBlurred(const Vector<2, float> &, const Vector<2, float> &, float, bool, float) {}

    // Draws the surface averaging factor x factor texels, in m_drawColor.
    virtual void drawDownsampled(const Vector<2, float> &, const Vector<2, float> &, const Vector<2, float> &, float,
                                 int)
    {}

    // Clears a rectangle (colour and depth) to transparent.
    virtual void clearRect(const Vector<2, float> &, const Vector<2, float> &, bool) {}

    // Clears the whole surface to m_clearColor.
    virtual void clear() {}

protected:
    friend class Widget;           // Creates its surface without linear filtering.
    friend class ::Sheet;          // Sets the clear colour to the page colour.
    friend class ::Module;         // Fades its panel's surface.
    friend class ::Workspace;      // UpdateLayout eases the bottom panels by their surface's fade.
    friend class ::ExportModule;   // Clears to the premultiplied page colour.
    bool m_linearFiltering;        // GL_LINEAR rather than GL_NEAREST.
    Vector<4, float> m_drawColor;  // The colour DrawDownsampled draws in; 1, 1, 1, 1 at construction.
    Vector<4, float> m_clearColor; // The colour Clear clears to; 0, 0, 0, 0 at construction.
    bool m_depthAttachment;        // Also attaches a depth texture.
};

}
