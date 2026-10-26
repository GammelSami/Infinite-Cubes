#include "plasma/GLFrameBufferObject.h"

GLFrameBufferObject::GLFrameBufferObject()
    : m_fboId(_GenerateFboId()),
      m_savedFboId(0)
{
    // Binding the new name makes the driver create the framebuffer now.
    _GuardedBind();
    _GuardedUnbind();
}

GLFrameBufferObject::~GLFrameBufferObject()
{
    glDeleteFramebuffersEXT(1, &m_fboId);
}

void GLFrameBufferObject::bind()
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fboId);
}

void GLFrameBufferObject::disable()
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void GLFrameBufferObject::attachTexture(GLenum attachment, GLenum texTarget, GLuint texId, int mipLevel, int zSlice)
{
    _GuardedBind();
    _FramebufferTextureND(attachment, texTarget, texId, mipLevel, zSlice);
    _GuardedUnbind();
}

void GLFrameBufferObject::attachRenderBuffer(GLenum attachment, GLuint buffId)
{
    _GuardedBind();
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, attachment, GL_RENDERBUFFER_EXT, buffId);
    _GuardedUnbind();
}

GLuint GLFrameBufferObject::_GenerateFboId()
{
    GLuint id = 0;
    glGenFramebuffersEXT(1, &id);
    return id;
}

void GLFrameBufferObject::_GuardedBind()
{
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &m_savedFboId);
    if (m_fboId != (GLuint)m_savedFboId)
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fboId);
}

void GLFrameBufferObject::_GuardedUnbind()
{
    if ((GLuint)m_savedFboId != m_fboId)
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, (GLuint)m_savedFboId);
}

void GLFrameBufferObject::_FramebufferTextureND(GLenum attachment, GLenum texTarget, GLuint texId, int mipLevel,
                                                int zSlice)
{
    if (texTarget == GL_TEXTURE_1D) {
        glFramebufferTexture1DEXT(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_1D, texId, mipLevel);
    } else if (texTarget == GL_TEXTURE_3D) {
        glFramebufferTexture3DEXT(GL_FRAMEBUFFER_EXT, attachment, GL_TEXTURE_3D, texId, mipLevel, zSlice);
    } else {
        // GL_TEXTURE_2D, GL_TEXTURE_RECTANGLE_ARB or a cube map face.
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment, texTarget, texId, mipLevel);
    }
}
