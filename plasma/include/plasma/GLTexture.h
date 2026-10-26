// plasma::GLTexture: a texture kept as a GL_TEXTURE_2D object, created and uploaded on first use.
#pragma once
#include "plasma/Texture.h"
#include <GL/glew.h>

namespace plasma {

class GLTexture : public Texture {
public:
    GLTexture(Engine *engine, int width, int height, const unsigned char *pixels, const TextureFormat &format,
              const std::wstring &name, bool transient);

    ~GLTexture();

    virtual void bind();
    virtual void unbind();

    // Sets the filters and wrapping and uploads the pixels (mipmapped for a mipmapped filter). Fully transparent RGBA
    // pixels of a linearly magnified texture first take the alpha-weighted average colour of their neighbours, so that
    // filtering does not darken the edges.
    virtual void upload(const unsigned char *pixels);

private:
    GLuint m_textureId; // 0 until the first Bind.
};

}
