// plasma::Texture: a named image the engine draws with. It keeps its pixels and their format; GLTexture (the only
// derived class) uploads them on first use. Unless it is transient, a texture gets an id one above the largest
// existing id in the engine's id map (the PLX files refer to textures by id).
#pragma once
#include <iosfwd>
#include <list>
#include <string>
#include <vector>
#include "plasma/NamedObject.h"

class ApplyTextureOperation;
class TextureModule;
class DrawFaceTool;
class DrawFreehandTool;
class MeshOperation;
class ProceduralTool;
class TextureThumbnailWidget;
class Workspace;

namespace plasma {

// How the pixels are stored and sampled (GLTexture::Upload maps each value to its GL constant). TextureFormat's
// values, named after the texture panel's strings ("texture:filter:nearest"... "texture:wrap:mirror").
enum TexturePixelFormat {
    TEXTURE_RGB = 0, // 3 bytes per pixel (SetPixels; GL_RGB in GLTexture::Upload).
    TEXTURE_RGBA = 1
};
enum TextureFilter {
    TEXTURE_FILTER_NEAREST = 0,
    TEXTURE_FILTER_LINEAR = 1,
    TEXTURE_FILTER_MIPMAP = 2
};
enum TextureWrap {
    TEXTURE_WRAP_NONE = 0, // Clamp to edge.
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

    int pixelFormat;    // "Texture.format.pixelFormat": a TexturePixelFormat.
    int minFilter;      // "Texture.format.minFilter": a TextureFilter (mipmapped: anisotropic).
    int maxFilter;      // "Texture.format.maxFilter" (GL's magnification filter; mipmapped magnifies linearly).
    int horizontalWrap; // "Texture.format.horizontalWrap": a TextureWrap.
    int verticalWrap;   // "Texture.format.verticalWrap".
};

class Texture : public NamedObject {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;
    friend class Engine;          // DeleteTexture, BuildPixelFont, WritePLA.
    friend class MeshShape;       // Compares its textures' ids (UpdateBoundsAndTextures).
    friend class SmoothMeshShape; // Likewise (UpdateDrawingTextures).
    friend void writePLATexture(std::ostream &, Texture *);
    friend class ::Workspace;              // A textured rectangle takes the size and id (CreateTexturedRectangle).
    friend class ::TextureModule;          // Sets the filter/wrap format and re-uploads the pixels.
    friend class ::DrawFaceTool;           // The new mesh takes the texture module's texture id.
    friend class ::DrawFreehandTool;       // Likewise for the drawn mesh (PointerReleased).
    friend class ::MeshOperation;          // Forgets a deleted texture's id in its states (ForgetTexture).
    friend class ::ProceduralTool;         // The preview takes the texture module's texture id.
    friend class ::ApplyTextureOperation;  // Stores the id in the shape (Do, Undo).
    friend class ::TextureThumbnailWidget; // Scales the thumbnail to the texture's aspect.
public:
    // A transient texture gets no id and drops its pixels once uploaded.
    Texture(Engine *engine, const std::wstring &name, bool transient);
    virtual ~Texture();

    virtual void bind() = 0;
    virtual void unbind() = 0;

    // Gives the pixels (in the texture's format) to the device.
    virtual void upload(const unsigned char *pixels) = 0;

    // Replaces the pixels with a copy of width * height pixels in format.
    void setPixels(int width, int height, const unsigned char *pixels, const TextureFormat &format);

    // Reinterprets/copies the current bytes via SetPixels under the new format; these methods do no channel conversion
    // or immediate GL re-upload.
    void setFormat(const TextureFormat &format);

    // Texture dimensions used by Workspace::CreateTexturedRectangle.
    const TextureFormat &getFormat() const { return m_format; }
    const std::vector<unsigned char> &getPixels() const { return m_pixels; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

protected:
    std::vector<unsigned char> m_pixels;
    TextureFormat m_format;
    int m_width;
    int m_height;
    int m_id; // -1 for a transient texture.
    bool m_transient;
    std::list<Texture *>::iterator m_engineEntry; // This texture's node in the engine's list.
};

}
