// Texture stores image data and texture settings used by the graphics backend.
#pragma once
#include <iosfwd>
#include <list>
#include <string>
#include <vector>
#include "plasma/NamedObject.h"

namespace plasma {
enum TexturePixelFormat {
    TEXTURE_RGB = 0,
    TEXTURE_RGBA = 1
};
enum TextureFilter {
    TEXTURE_FILTER_NEAREST = 0,
    TEXTURE_FILTER_LINEAR = 1,
    TEXTURE_FILTER_MIPMAP = 2
};
enum TextureWrap {
    TEXTURE_WRAP_NONE = 0,
    TEXTURE_WRAP_REPEAT = 1,
    TEXTURE_WRAP_MIRROR = 2
};

struct TextureFormat {
    TextureFormat()
        : pixelFormat(TEXTURE_RGBA),
          minFilter(TEXTURE_FILTER_LINEAR),
          maxFilter(TEXTURE_FILTER_LINEAR),
          horizontalWrap(TEXTURE_WRAP_REPEAT),
          verticalWrap(TEXTURE_WRAP_REPEAT)
    {}
    TextureFormat(int pixelFormat, int minFilter, int maxFilter, int horizontalWrap, int verticalWrap)
        : pixelFormat(pixelFormat),
          minFilter(minFilter),
          maxFilter(maxFilter),
          horizontalWrap(horizontalWrap),
          verticalWrap(verticalWrap)
    {}

    int pixelFormat;
    int minFilter;
    int maxFilter;
    int horizontalWrap;
    int verticalWrap;
};

class Texture : public NamedObject {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;
    friend class Engine;
    friend class MeshShape;
    friend class SmoothMeshShape;

public:
    Texture(Engine *engine, const std::wstring &name, bool transient);
    virtual ~Texture();

    virtual void bind(int stage) = 0; // the texture stage
    virtual void upload(const unsigned char *pixels) = 0;
    void setPixels(int width, int height, const unsigned char *pixels, const TextureFormat &format);
    void setFormat(const TextureFormat &format);
    const TextureFormat &getFormat() const { return m_format; }
    const std::vector<unsigned char> &getPixels() const { return m_pixels; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int id() const { return m_id; }

protected:
    std::vector<unsigned char> m_pixels;
    TextureFormat m_format;
    int m_width;
    int m_height;
    int m_id;
    bool m_transient;
    std::list<Texture *>::iterator m_engineEntry;
};

}
