// Bitmap font glyph bounds and texture handling.
#include "plasma/PixelFont.h"
#include <algorithm>
#include "plasma/EditFilter.h"

namespace plasma {

PixelFont::PixelFont(Engine *engine, const std::wstring &name)
    : Font(engine, name),
      m_texture(nullptr)
{
    m_minimum.m_elements[0] = 0.0f;
    m_minimum.m_elements[1] = 0.0f;
    m_maximum.m_elements[0] = 0.0f;
    m_maximum.m_elements[1] = 0.0f;
}

PixelFont::~PixelFont()
{
    for (std::map<unsigned short, PixelGlyph *>::iterator glyph = m_glyphs.begin(); glyph != m_glyphs.end(); ++glyph)
        if (glyph->second)
            delete glyph->second;
}

Vector<2, float> PixelFont::getMaximum()
{
    return m_maximum;
}

Vector<2, float> PixelFont::getMinimum()
{
    return m_minimum;
}

void PixelFont::getBounds(const std::wstring &text, Vector<2, float> &minimum, Vector<2, float> &maximum, int flags)
{
    minimum.m_elements[0] = 0.0f;
    minimum.m_elements[1] = 0.0f;
    maximum.m_elements[0] = 0.0f;
    maximum.m_elements[1] = 0.0f;
    int count = text.size();
    float x = 0.0f;
    float y = 0.0f;
    for (int character = 0; character < count; character++) {
        if (text[character] == L'\n') {
            x = 0.0f;
            y = m_maximum.m_elements[1] - m_minimum.m_elements[1] + y;
        } else {
            PixelGlyph *glyph = getGlyph(text[character]);
            if (glyph) {
                Vector<2, float> lower = glyph->m_position;
                lower.m_elements[0] = lower.m_elements[0] + x;
                lower.m_elements[1] = lower.m_elements[1] + y;
                Vector<2, float> upper = lower;
                upper.m_elements[0] = glyph->m_size.m_elements[0] + upper.m_elements[0];
                upper.m_elements[1] = glyph->m_size.m_elements[1] + upper.m_elements[1];
                if (character == 0) {
                    minimum = lower;
                    maximum = upper;
                } else {
                    minimum.m_elements[0] = (std::min)(minimum.m_elements[0], lower.m_elements[0]);
                    minimum.m_elements[1] = (std::min)(minimum.m_elements[1], lower.m_elements[1]);
                    maximum.m_elements[0] = (std::max)(maximum.m_elements[0], upper.m_elements[0]);
                    maximum.m_elements[1] = (std::max)(maximum.m_elements[1], upper.m_elements[1]);
                }
                x = x + glyph->m_advance;
                if (character < count - 1)
                    x = getKerning(text[character], text[character + 1]) + x;
            }
        }
    }
    if (flags & (1 << TEXT_CENTER)) {
        float center = 0.5f * (minimum.m_elements[0] + maximum.m_elements[0]);
        minimum.m_elements[0] -= center;
        maximum.m_elements[0] -= center;
    } else if (flags & (1 << TEXT_RIGHT)) {
        minimum.m_elements[0] -= maximum.m_elements[0];
        maximum.m_elements[0] = 0.0f;
    }
    if (flags & (1 << TEXT_MIDDLE)) {
        float center = 0.5f * (minimum.m_elements[1] + maximum.m_elements[1]);
        minimum.m_elements[1] -= center;
        maximum.m_elements[1] -= center;
    } else if (flags & (1 << TEXT_BOTTOM)) {
        minimum.m_elements[1] -= maximum.m_elements[1];
        maximum.m_elements[1] = 0.0f;
    }
}

void PixelFont::getCaret(const std::wstring &text, int index, Vector<2, float> &position, Vector<2, float> &size,
                         int flags)
{
    std::wstring characters = text + L" ";
    int count = characters.size();
    if (count == 0) {
        position = Vector<2, float>(0.0f, 0.0f);
        size = Vector<2, float>(0.0f, 0.0f);
        return;
    }
    if (index < 0)
        index = 0;
    int last = count - 1;
    if (index > last)
        index = last;
    float x = 0.0f;
    float y = 0.0f;
    for (int character = 0; character < index + 1; character++) {
        if (text[character] == L'\n') {
            x = 0.0f;
            y = m_maximum.m_elements[1] - m_minimum.m_elements[1] + y;
        } else {
            PixelGlyph *glyph = getGlyph(characters[character]);
            if (glyph) {
                position = Vector<2, float>(x, y) + glyph->m_position;
                size = glyph->m_size;
                x = x + glyph->m_advance;
                if (character < last)
                    x = getKerning(characters[character], characters[character + 1]) + x;
            }
        }
    }
    Vector<2, float> unaligned;
    Vector<2, float> aligned;
    Vector<2, float> maximum;
    getBounds(text, unaligned, maximum, 0);
    getBounds(text, aligned, maximum, flags);
    aligned.m_elements[0] -= unaligned.m_elements[0];
    aligned.m_elements[1] -= unaligned.m_elements[1];
    position.m_elements[0] += aligned.m_elements[0];
    position.m_elements[1] += aligned.m_elements[1];
}

void PixelFont::setGlyph(unsigned short character, const Vector<2, float> &position, const Vector<2, float> &size,
                         const Vector<2, float> &texPosition, const Vector<2, float> &texSize, float advance)
{
    PixelGlyph *glyph = m_glyphs[character];
    if (!glyph) {
        glyph = new PixelGlyph();
        m_glyphs[character] = glyph;
    }
    if (position.m_elements[0] < m_minimum.m_elements[0])
        m_minimum.m_elements[0] = position.m_elements[0];
    if (position.m_elements[1] < m_minimum.m_elements[1])
        m_minimum.m_elements[1] = position.m_elements[1];
    if (m_maximum.m_elements[0] < position.m_elements[0] + size.m_elements[0])
        m_maximum.m_elements[0] = position.m_elements[0] + size.m_elements[0];
    if (m_maximum.m_elements[1] < position.m_elements[1] + size.m_elements[1])
        m_maximum.m_elements[1] = position.m_elements[1] + size.m_elements[1];
    glyph->m_position = position;
    glyph->m_size = size;
    glyph->m_texPosition = texPosition;
    glyph->m_texSize = texSize;
    glyph->m_advance = advance;
}

void PixelFont::renderText(const std::wstring &, const Vector<2, float> &, const float *, const float *, int) {}

PixelGlyph *PixelFont::getGlyph(unsigned short character)
{
    std::map<unsigned short, PixelGlyph *>::iterator glyph = m_glyphs.find(character);
    if (glyph != m_glyphs.end())
        return glyph->second;
    return nullptr;
}

void PixelFont::setTexture(Texture *texture)
{
    m_texture = texture;
}

}
