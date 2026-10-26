#include "plasma/GLRenderSurface.h"
#include "plasma/GLEngine.h"

namespace plasma {

// The colour texture the engine's framebuffer object last rendered into.
static GLuint s_targetTexture;

GLRenderSurface::GLRenderSurface(GLEngine *engine)
    : m_width(0),
      m_height(0),
      m_engine(engine)
{
    glGenTextures(1, &m_colorTexture);
#ifdef __EMSCRIPTEN__
    glGenRenderbuffers(1, &m_depthTexture);
#else
    glGenTextures(1, &m_depthTexture);
#endif
    m_frameBuffer = new GLFrameBufferObject;
    GLFrameBufferObject::disable();
}

GLRenderSurface::~GLRenderSurface()
{
    delete m_frameBuffer;
    glDeleteTextures(1, &m_colorTexture);
#ifdef __EMSCRIPTEN__
    glDeleteRenderbuffers(1, &m_depthTexture);
#else
    glDeleteTextures(1, &m_depthTexture);
#endif
}

bool GLRenderSurface::resize(int width, int height)
{
    if (width < 1)
        width = 1;
    if (height < 1)
        height = 1;
    if (m_width == width && m_height == height)
        return false;
    m_width = width;
    m_height = height;
#ifdef __EMSCRIPTEN__
    glDeleteRenderbuffers(1, &m_depthTexture);
#else
    glDeleteTextures(1, &m_depthTexture);
#endif
#ifdef __EMSCRIPTEN__
    glGenRenderbuffers(1, &m_depthTexture);
#else
    glGenTextures(1, &m_depthTexture);
#endif

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_colorTexture);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, m_linearFiltering ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, m_linearFiltering ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
    m_frameBuffer->attachTexture(GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, m_colorTexture);
    if (m_depthAttachment) {
#ifdef __EMSCRIPTEN__

        // WebGL has depth renderbuffers; avoid desktop depth-texture emulation.
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthTexture);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
        m_frameBuffer->attachRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, m_depthTexture);

// Gl4es rejects renderbuffer zero. Each allocation binds its own renderbuffer, so this scratch binding need not be
// cleared.
#else
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_depthTexture);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT,
                     GL_UNSIGNED_BYTE, 0);
        m_frameBuffer->attachTexture(GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_RECTANGLE_ARB, m_depthTexture);
#endif
    }
    glActiveTexture(GL_TEXTURE0);
    return true;
}

void GLRenderSurface::begin()
{
    if (m_depthAttachment) {
        m_engine->m_frameBuffer->bind();
        m_engine->m_frameBuffer->_FramebufferTextureND(GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB,
                                                       m_colorTexture, 0, 0);
#ifdef __EMSCRIPTEN__
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthTexture);
#else
        m_engine->m_frameBuffer->_FramebufferTextureND(GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_RECTANGLE_ARB,
                                                       m_depthTexture, 0, 0);
#endif
    } else {
        m_engine->m_frameBuffer->bind();
#ifdef __EMSCRIPTEN__
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
#endif
        m_engine->m_frameBuffer->_FramebufferTextureND(GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB,
                                                       m_colorTexture, 0, 0);
    }
    GLuint texture = m_colorTexture;
    if (s_targetTexture != texture) {
        m_engine->m_renderTargetSwitchCount++;
        s_targetTexture = texture;
    }
}

void GLRenderSurface::end()
{
    GLFrameBufferObject::disable();
}

void GLRenderSurface::bindTexture()
{
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_colorTexture);
}

void GLRenderSurface::drawBlurred(const Vector<2, float> &position, const Vector<2, float> &size, float radius,
                                  bool horizontal, float scale)
{
    glDisable(GL_DEPTH_TEST);
    GLfloat modelView[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
    Matrix<float> matrix(modelView);

    // The fraction of a pixel the translation leaves (none at scale 1).
    float offset[2] = {matrix.m_elements[12] * scale, matrix.m_elements[13] * scale};
    int whole[2];
    for (int i = 0; i < 2; i++)
        whole[i] = (int)offset[i];
    float wholeOffset[2] = {(float)whole[0], (float)whole[1]};
    offset[0] -= wholeOffset[0];
    offset[1] -= wholeOffset[1];
    if (1.0f == scale) {
        offset[0] = 0.0f;
        offset[1] = 0.0f;
    }
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_colorTexture);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_BLEND);
    m_engine->useBlurShader(horizontal);
    float x = position.m_elements[0];
    float y = position.m_elements[1];
    float width = size.m_elements[0];
    float height = size.m_elements[1];
    float surfaceSize[2] = {(float)m_width, (float)m_height};
    float margin = (1.0f - scale) * 0.5;
    float shift[2] = {margin * surfaceSize[0], margin * surfaceSize[1]};
    shift[0] -= offset[0];
    shift[1] -= offset[1];
    float s = position.m_elements[0] * scale + shift[0];
    float t = position.m_elements[1] * scale + shift[1];
    float textureWidth = size.m_elements[0] * scale;
    float textureHeight = scale * size.m_elements[1];

    // Texture-coordinate z supplies a quarter-radius step; the outer shader taps sample at four times that step,
    // reaching the requested radius.
    radius = radius * 0.25;
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glTexCoord3f(s, t, radius);
    glVertex2f(x, y);
    glTexCoord3f(textureWidth + s, t, radius);
    glVertex2f(width + x, y);
    glTexCoord3f(textureWidth + s, textureHeight + t, radius);
    glVertex2f(width + x, height + y);
    glTexCoord3f(s, textureHeight + t, radius);
    glVertex2f(x, height + y);
    glEnd();
    glPopMatrix();
    glEnable(GL_BLEND);
}

void GLRenderSurface::draw(const Vector<2, float> &position, const Vector<2, float> &size,
                           const Vector<2, float> &texturePosition, float scale)
{
    GLfloat modelView[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
    Matrix<float> matrix(modelView);
    float offset[2] = {matrix.m_elements[12] * scale, matrix.m_elements[13] * scale};
    int whole[2];
    for (int i = 0; i < 2; i++)
        whole[i] = (int)offset[i];
    float wholeOffset[2] = {(float)whole[0], (float)whole[1]};
    offset[0] -= wholeOffset[0];
    offset[1] -= wholeOffset[1];
    if (1.0f == scale) {
        offset[0] = 0.0f;
        offset[1] = 0.0f;
    }
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_colorTexture);
    glPushMatrix();
    glLoadIdentity();
    m_engine->useDownsampleShader(1);
    float x = position.m_elements[0];
    float y = position.m_elements[1];

    // A negative size draws the whole surface.
    float width = size.m_elements[0];
    float height = size.m_elements[1];
    if (width < 0.0f)
        width = (float)m_width;
    if (height < 0.0f)
        height = (float)m_height;
    float surfaceSize[2] = {(float)m_width, (float)m_height};
    float margin = (1.0f - scale) * 0.5;
    float shift[2] = {margin * surfaceSize[0], margin * surfaceSize[1]};

    // A negative scale uses surfaceWidth / requestedWidth for both texture axes.
    if (scale < 0.0f) {
        scale = surfaceSize[0] / size.m_elements[0];
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        shift[0] = 0.0f;
        shift[1] = 0.0f;
    }
    shift[0] -= offset[0];
    shift[1] -= offset[1];
    float s = texturePosition.m_elements[0] * scale + shift[0];
    float t = texturePosition.m_elements[1] * scale + shift[1];
    float textureWidth = width * scale;
    float textureHeight = scale * height;
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(s, t);
    glVertex2f(x, y);
    glTexCoord2f(textureWidth + s, t);
    glVertex2f(width + x, y);
    glTexCoord2f(textureWidth + s, textureHeight + t);
    glVertex2f(width + x, height + y);
    glTexCoord2f(s, textureHeight + t);
    glVertex2f(x, height + y);
    glEnd();
    glPopMatrix();
}

void GLRenderSurface::drawDownsampled(const Vector<2, float> &position, const Vector<2, float> &size,
                                      const Vector<2, float> &texturePosition, float scale, int factor)
{
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_colorTexture);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    m_engine->useDownsampleShader(factor);
    float x = position.m_elements[0];
    float y = position.m_elements[1];
    float width = size.m_elements[0];
    float height = size.m_elements[1];
    float s = texturePosition.m_elements[0];
    float t = texturePosition.m_elements[1];
    float textureWidth = size.m_elements[0] * scale;
    float textureHeight = scale * size.m_elements[1];
    glBegin(GL_QUADS);
    glColor4fv(m_drawColor.m_elements);
    glTexCoord2f(s, t);
    glVertex2f(x, y);
    glTexCoord2f(textureWidth + s, t);
    glVertex2f(width + x, y);
    glTexCoord2f(textureWidth + s, textureHeight + t);
    glVertex2f(width + x, height + y);
    glTexCoord2f(s, textureHeight + t);
    glVertex2f(x, height + y);
    glEnd();
    glPopMatrix();
}

void GLRenderSurface::clear()
{
    begin();
    glClearColor(m_clearColor.m_elements[0], m_clearColor.m_elements[1], m_clearColor.m_elements[2],
                 m_clearColor.m_elements[3]);
    glClearDepth(0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// Draws a transparent quad and a 2-pixel outline. Only the matrix stack is restored and blending re-enabled;
// callers must set subsequent GL state.
void GLRenderSurface::clearRect(const Vector<2, float> &position, const Vector<2, float> &size, bool loadIdentity)
{
    begin();
    m_engine->useFixedFunction();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    if (loadIdentity)
        glLoadIdentity();
    glScalef(1.0f, 1.0f, 0.0f);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_RECTANGLE_ARB);
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glBegin(GL_QUADS);
    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
    glVertex2f(position.m_elements[0], position.m_elements[1]);
    glVertex2f(position.m_elements[0] + size.m_elements[0], position.m_elements[1]);
    glVertex2f(position.m_elements[0] + size.m_elements[0], position.m_elements[1] + size.m_elements[1]);
    glVertex2f(position.m_elements[0], position.m_elements[1] + size.m_elements[1]);
    glEnd();
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(position.m_elements[0], position.m_elements[1]);
    glVertex2f(position.m_elements[0] + size.m_elements[0], position.m_elements[1]);
    glVertex2f(position.m_elements[0] + size.m_elements[0], position.m_elements[1] + size.m_elements[1]);
    glVertex2f(position.m_elements[0], position.m_elements[1] + size.m_elements[1]);
    glEnd();
    glEnable(GL_BLEND);
    glPopMatrix();
}

}
