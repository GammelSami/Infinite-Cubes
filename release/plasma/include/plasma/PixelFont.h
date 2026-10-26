// plasma::PixelFont: a legacy bitmap font; each glyph is a rectangle of the font's texture with its placement and
// advance.
#pragma once
#include <map>
#include <string>
#include "plasma/Font.h"
#include "plasma/Vector.h"

namespace plasma {

class Engine;
class Texture;

struct PixelGlyph {
    Vector<2, float> m_position;
    Vector<2, float> m_size;
    Vector<2, float> m_texPosition;
    Vector<2, float> m_texSize;
    float m_advance; // added to the pen position by getBounds
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

    // Returns the glyph for character, or null if it is absent.
    PixelGlyph *getGlyph(unsigned short character);
    void setTexture(Texture *texture);
    void setGlyph(unsigned short character, const Vector<2, float> &position, const Vector<2, float> &size,
                  const Vector<2, float> &texPosition, const Vector<2, float> &texSize, float advance);

    Texture *m_texture;                              // setTexture
    std::map<unsigned short, PixelGlyph *> m_glyphs; // constructor, destructor
    Vector<2, float> m_minimum;                      // getMinimum
    Vector<2, float> m_maximum;                      // getMaximum
};

}
