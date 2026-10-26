#pragma once
#include "plasma/Font.h"
namespace plasma {
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
    void setTexture(Texture *texture);
    void setGlyph(unsigned short character, const Vector<2, float> &position, const Vector<2, float> &size,
                  const Vector<2, float> &texPosition, const Vector<2, float> &texSize, float advance);
    PixelGlyph *getGlyph(unsigned short character);

    Texture *m_texture;
    std::map<unsigned short, PixelGlyph *> m_glyphs;
    Vector<2, float> m_minimum;
    Vector<2, float> m_maximum;
};

}
