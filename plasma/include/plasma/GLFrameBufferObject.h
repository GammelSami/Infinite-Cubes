// An EXT_framebuffer_object render target. Attachments use guarded binding to restore the previous framebuffer
// afterwards.
#pragma once
#include <GL/glew.h>

class GLFrameBufferObject {
public:
    GLFrameBufferObject();
    virtual ~GLFrameBufferObject();

    // Makes this framebuffer the current render target.
    void bind();

    // Attach to this framebuffer, restoring the previous binding afterwards.
    virtual void attachTexture(GLenum attachment, GLenum texTarget, GLuint texId, int mipLevel = 0, int zSlice = 0);
    virtual void attachRenderBuffer(GLenum attachment, GLuint buffId);

    // Renders to the window again.
    static void disable();

    // Attaches while this framebuffer is bound (GLRenderSurface calls it so).
    void _FramebufferTextureND(GLenum attachment, GLenum texTarget, GLuint texId, int mipLevel, int zSlice);

protected:
    void _GuardedBind();
    void _GuardedUnbind();
    static GLuint _GenerateFboId();

private:
    GLuint m_fboId;
    GLint m_savedFboId; // The binding _GuardedBind replaced.
};
