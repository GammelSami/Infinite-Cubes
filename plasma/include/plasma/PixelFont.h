// PixelFont.cpp: bitmap font glyph metrics and text bounds. Also declares their base Font (Font.cpp) and the
// TextFlags.
#pragma once
#include <map>
#include <list>
#include "plasma/NamedObject.h"
#include "plasma/Vector.h"

namespace plasma {

class Node;
class Texture;

// The text layout flags: TextShape::m_flags ("TextShape.flags"), passed to the fonts' (Font's and ScalableFont's)
// RenderText, Measure, GetBounds and GetCaret.
enum TextFlags {
    TEXT_CENTER = 0, // Horizontally centred.
    TEXT_RIGHT = 1,  // Right-aligned.
    TEXT_MIDDLE = 2, // Vertically centred.
    TEXT_BOTTOM = 3,
    TEXT_WRAP = 4 // "text:wrap": wrapped at m_wrapWidth.
};

enum TextMasks {
    TEXT_FLAGS_NONE = 0,
    TEXT_HORIZONTAL_MASK = (1 << TEXT_CENTER) | (1 << TEXT_RIGHT),
    TEXT_VERTICAL_MASK = (1 << TEXT_MIDDLE) | (1 << TEXT_BOTTOM)
};

// Base of the legacy fonts PixelFont and PlasmaFont.
class Font : public NamedObject {
public:
    Font(Engine *engine, const std::wstring &name);
    virtual ~Font();
    virtual void getCaret(const std::wstring &text, int index, Vector<2, float> &position, Vector<2, float> &size,
                          int flags) = 0;
    virtual void getBounds(const std::wstring &text, Vector<2, float> &minimum, Vector<2, float> &maximum,
                           int flags) = 0;
    virtual Vector<2, float> getMinimum() = 0;
    virtual Vector<2, float> getMaximum() = 0;
    virtual void renderText(const std::wstring &text, const Vector<2, float> &position, const float *fill,
                            const float *outline, int flags);
    float getKerning(unsigned short left, unsigned short right);

    // Font cache lookup properties.
    float size() const { return m_size; }
    float getSpacing() const { return m_spacing; }

    std::map<std::pair<unsigned short, unsigned short>, float> m_kerning;
    std::list<Font *>::iterator m_engineEntry;
    int m_id;
    std::wstring m_path;
    float m_size;
    float m_spacing;
    bool
        m_transient; // The loaders' transient argument, part of the legacy font cache key (Engine::LoadLegacyFont/LoadPixelFont).
};

struct PixelGlyph {
    Vector<2, float> m_position;
    Vector<2, float> m_size;
    Vector<2, float> m_texPosition;
    Vector<2, float> m_texSize;
    float m_advance;
};

class PixelFont : public Font {
public:
    PixelFont(Engine *engine, const std::wstring &name);
    virtual ~PixelFont();
    virtual void getCaret(const std::wstring &text, int index, Vector<2, float> &position, Vector<2, float> &size,
                          int flags);
    virtual void getBounds(const std::wstring &text, Vector<2, float> &minimum, Vector<2, float> &maximum, int flags);
    virtual Vector<2, float> getMinimum();
    virtual Vector<2, float> getMaximum();
    virtual void renderText(const std::wstring &text, const Vector<2, float> &position, const float *fill,
                            const float *outline, int flags);
    PixelFont *setTexture(Texture *texture);
    void setGlyph(unsigned short character, const Vector<2, float> &position, const Vector<2, float> &size,
                  const Vector<2, float> &texPosition, const Vector<2, float> &texSize, float advance);
    PixelGlyph *getGlyph(unsigned short character);

    Texture *m_texture;
    std::map<unsigned short, PixelGlyph *> m_glyphs;
    Vector<2, float> m_minimum;
    Vector<2, float> m_maximum;
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif

}
