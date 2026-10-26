// Texture construction, pixel storage and format changes. The pixel-copying constructor takes the engine lock even for
// transient textures.
#include <string.h>
#include "plasma/Texture.h"
#include "plasma/Engine.h"
#include "plasma/EditFilter.h"

namespace plasma {

Texture::Texture(Engine *engine, const std::wstring &name, bool transient)
    : NamedObject(engine, name),
      m_width(0),
      m_height(0),
      m_id(-1),
      m_transient(transient)
{
    m_engineEntry = engine->addTexture(this);
    if (!transient) {
        engine->lock();
        int largest = -1;
        for (std::map<int, Texture *>::const_iterator entry = engine->m_texturesById.begin();
             entry != engine->m_texturesById.end(); ++entry) {
            if (entry->first > largest)
                largest = entry->first;
        }
        m_id = largest + 1;
        engine->m_texturesById[m_id] = this;
        engine->unlock();
    }
}

Texture::Texture(Engine *engine, int width, int height, const unsigned char *pixels, const TextureFormat &format,
                 const std::wstring &name, bool transient)
    : NamedObject(engine, name),
      m_id(-1),
      m_transient(transient)
{
    m_engineEntry = engine->addTexture(this);
    engine->lock();
    if (!transient) {
        int largest = -1;
        for (std::map<int, Texture *>::const_iterator entry = engine->m_texturesById.begin();
             entry != engine->m_texturesById.end(); ++entry) {
            if (entry->first > largest)
                largest = entry->first;
        }
        m_id = largest + 1;
        engine->m_texturesById[m_id] = this;
    }
    setPixels(width, height, pixels, format);
    engine->unlock();
}

Texture::~Texture() {}

// Returns the byte size of a pixel in the selected format.
static int bytesPerPixel(const TextureFormat &format)
{
    switch (format.pixelFormat) {
    case TEXTURE_RGB:
        return 3;
    case TEXTURE_RGBA:
        return 4;
    }
    return 4;
}

void Texture::setPixels(int width, int height, const unsigned char *pixels, const TextureFormat &format)
{
    m_width = width;
    m_height = height;
    m_format = format;
    m_pixels.resize(bytesPerPixel(format) * width * height);
    memcpy(&m_pixels[0], pixels, m_pixels.size());
}

void Texture::setFormat(const TextureFormat &format)
{
    setPixels(m_width, m_height, &m_pixels[0], format);
}

}
