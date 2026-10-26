#include <string.h>
#include "plasma/GLTexture.h"
#include <GL/glu.h>

namespace plasma {

GLTexture::GLTexture(Engine *engine, int width, int height, const unsigned char *pixels, const TextureFormat &format,
                     const std::wstring &name, bool transient)
    : Texture(engine, name, transient),
      m_textureId(0)
{
    setPixels(width, height, pixels, format);
}

GLTexture::~GLTexture()
{
    if (m_textureId)
        glDeleteTextures(1, &m_textureId);
}

void GLTexture::bind()
{
    if (!m_textureId) {
        glGenTextures(1, &m_textureId);
        upload(&m_pixels[0]);
        if (m_transient)
            m_pixels.clear();
    }
    glBindTexture(GL_TEXTURE_2D, m_textureId);
}

void GLTexture::unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLTexture::upload(const unsigned char *pixels)
{
    glBindTexture(GL_TEXTURE_2D, m_textureId);

    GLint minFilter = 0;
    switch (m_format.minFilter) {
    case TEXTURE_FILTER_NEAREST:
        minFilter = GL_NEAREST;
        break;
    case TEXTURE_FILTER_LINEAR:
        minFilter = GL_LINEAR;
        break;
    case TEXTURE_FILTER_MIPMAP: {
        minFilter = GL_LINEAR_MIPMAP_LINEAR;
        GLfloat anisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
        break;
    }
    }
    GLint magFilter = 0;
    switch (m_format.maxFilter) {
    case TEXTURE_FILTER_NEAREST:
        magFilter = GL_NEAREST;
        break;
    case TEXTURE_FILTER_LINEAR:
    case TEXTURE_FILTER_MIPMAP:
        magFilter = GL_LINEAR;
        break;
    }
    GLint wrapS = 0;
    switch (m_format.horizontalWrap) {
    case TEXTURE_WRAP_NONE:
        wrapS = GL_CLAMP_TO_EDGE;
        break;
    case TEXTURE_WRAP_REPEAT:
        wrapS = GL_REPEAT;
        break;
    case TEXTURE_WRAP_MIRROR:
        wrapS = GL_MIRRORED_REPEAT;
        break;
    }
    GLint wrapT = 0;
    switch (m_format.verticalWrap) {
    case TEXTURE_WRAP_NONE:
        wrapT = GL_CLAMP_TO_EDGE;
        break;
    case TEXTURE_WRAP_REPEAT:
        wrapT = GL_REPEAT;
        break;
    case TEXTURE_WRAP_MIRROR:
        wrapT = GL_MIRRORED_REPEAT;
        break;
    }
    GLint components = 0;
    GLenum format = 0;
    switch (m_format.pixelFormat) {
    case TEXTURE_RGB:
        components = 3;
        format = GL_RGB;
        break;
    case TEXTURE_RGBA:
        components = 4;
        format = GL_RGBA;
        break;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

    std::vector<unsigned char> bled;
    if (format == GL_RGBA && magFilter != GL_NEAREST) {
        bled.resize(m_width * m_height * 4);
        memcpy(&bled[0], pixels, m_width * m_height * 4);
        pixels = &bled[0];
        for (int x = 0; x < m_width; x++) {
            for (int y = 0; y < m_height; y++) {
                int pixel = (x + y * m_width) * 4;
                if (bled[pixel + 3] != 0)
                    continue;
                int weight = 0;
                int red = 0;
                int green = 0;
                int blue = 0;
                for (int nx = x - 1; nx <= x + 1; nx++) {
                    for (int ny = y - 1; ny <= y + 1; ny++) {
                        if ((nx != x || ny != y) && nx >= 0 && ny >= 0 && nx < m_width && ny < m_height) {
                            int neighbour = (nx + ny * m_width) * 4;
                            unsigned char alpha = bled[neighbour + 3];
                            if (alpha) {
                                red += alpha * bled[neighbour];
                                green += alpha * bled[neighbour + 1];
                                blue += alpha * bled[neighbour + 2];
                                weight += alpha;
                            }
                        }
                    }
                }
                if (weight > 0) {
                    bled[pixel] = red / weight;
                    bled[pixel + 1] = green / weight;
                    bled[pixel + 2] = blue / weight;
                }
            }
        }
    }
    if (minFilter == GL_LINEAR_MIPMAP_LINEAR || magFilter == GL_LINEAR_MIPMAP_LINEAR)
        gluBuild2DMipmaps(GL_TEXTURE_2D, components, m_width, m_height, format, GL_UNSIGNED_BYTE, pixels);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, components, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, pixels);
}

}
