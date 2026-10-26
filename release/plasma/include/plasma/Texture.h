// Texture stores a named image's pixels and sampling format. D3D11Texture creates the device resource on first use.
// Transient textures have no engine ID and discard pixels after upload.
#pragma once
#include <list>
#include <string>
#include <vector>
#include "plasma/NamedObject.h"

namespace plasma {

// Pixel storage and sampling settings.
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
    TEXTURE_WRAP_NONE = 0, // clamp to edge
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

    int pixelFormat; // a TexturePixelFormat
    int minFilter;   // a TextureFilter
    int maxFilter;
    int horizontalWrap; // a TextureWrap
    int verticalWrap;
};

class Texture : public NamedObject {
    friend class MeshShape;       // Compares the texture ID while updating bounds and textures.
    friend class PLXStreamReader; // Texture ID used by readTexture.
    friend class SmoothMeshShape; // Texture ID used by updateDrawingTextures.
public:
    // A transient texture gets no id and drops its pixels once uploaded.
    Texture(Engine *engine, const std::wstring &name, bool transient);
    // Initialises the texture and copies the supplied pixels. Takes the engine lock even for a transient texture.
    Texture(Engine *engine, int width, int height, const unsigned char *pixels, const TextureFormat &format,
            const std::wstring &name, bool transient);
    virtual ~Texture();

    // Binds the image to a pixel-shader resource slot; D3D11Drawing uses slot 1.
    virtual void bind(unsigned slot) {}
    // Gives the pixels (in the texture's format) to the device.
    virtual void upload(const unsigned char *pixels) {}

    // Replaces the pixels with a copy of width * height pixels in format.
    void setPixels(int width, int height, const unsigned char *pixels, const TextureFormat &format);
    // Re-sizes the current bytes under the new format (no conversion).
    void setFormat(const TextureFormat &format);

protected:
    friend class Engine; // createPixelFont scans m_pixels; loadPixelFont sets m_transient
    std::vector<unsigned char> m_pixels;
    TextureFormat m_format;
    int m_width;
    int m_height;
    int m_id; // -1 for a transient texture
    bool m_transient;
    std::list<Texture *>::iterator m_engineEntry; // returned by the engine's addTexture
};

}
