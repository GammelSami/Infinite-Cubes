// plasma::GLRenderSurface: a render surface kept in GL_TEXTURE_RECTANGLE_ARB textures (colour, and depth when
// m_depthAttachment is set) that the engine's framebuffer object renders into.
#pragma once
#include "plasma/RenderSurface.h"
#include <GL/glew.h>

class GLFrameBufferObject;
class ExportModule;
class RenderWidget;

namespace plasma {

class GLEngine;

class GLRenderSurface : public RenderSurface {
public:
    explicit GLRenderSurface(GLEngine *engine);
    virtual ~GLRenderSurface();

    virtual bool resize(int width, int height);
    virtual void begin();
    virtual void end();
    virtual void bindTexture();
    virtual void draw(const Vector<2, float> &position, const Vector<2, float> &size,
                      const Vector<2, float> &texturePosition, float scale);
    virtual void drawBlurred(const Vector<2, float> &position, const Vector<2, float> &size, float radius,
                             bool horizontal, float scale);
    virtual void drawDownsampled(const Vector<2, float> &position, const Vector<2, float> &size,
                                 const Vector<2, float> &texturePosition, float scale, int factor);
    virtual void clearRect(const Vector<2, float> &position, const Vector<2, float> &size, bool loadIdentity);
    virtual void clear();

private:
    friend class ::ExportModule;        // Sizes the export tiles by the surface.
    friend class ::RenderWidget;        // Soft-proofs the surface's pixels.
    GLFrameBufferObject *m_frameBuffer; // Created, but the engine's is the one rendered into.
    GLuint m_colorTexture;
    GLuint m_depthTexture;
    int m_width;
    int m_height;
    GLEngine *m_engine;
};

}
