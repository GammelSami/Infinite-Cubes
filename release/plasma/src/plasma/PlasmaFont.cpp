// Scene-node font glyph rendering and measurement.
#include "plasma/PlasmaFont.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include <algorithm>
#include "plasma/EditFilter.h"

namespace plasma {

PlasmaFont::PlasmaFont(Engine *engine, const std::wstring &name)
    : Font(engine, name)
{
    m_minimum.m_elements[0] = 0.0f;
    m_minimum.m_elements[1] = 0.0f;
    m_maximum.m_elements[0] = 0.0f;
    m_maximum.m_elements[1] = 0.0f;
}

PlasmaFont::~PlasmaFont()
{
    for (std::map<wchar_t, PlasmaGlyph *>::iterator glyph = m_glyphs.begin(); glyph != m_glyphs.end(); ++glyph)
        if (glyph->second)
            delete glyph->second;
}

Vector<2, float> PlasmaFont::getMaximum()
{
    // The glyph bounds are stored in units of 1/200 of the font size.
    return m_maximum * 0.005f * m_size;
}

Vector<2, float> PlasmaFont::getMinimum()
{
    // The glyph bounds are stored in units of 1/200 of the font size.
    return m_minimum * 0.005f * m_size;
}

PlasmaFont *PlasmaFont::clone()
{
    PlasmaFont *copy = m_engine->createPlasmaFont(m_name);
    copy->m_maximum = m_maximum;
    copy->m_minimum = m_minimum;
    for (std::map<wchar_t, PlasmaGlyph *>::iterator glyph = m_glyphs.begin(); glyph != m_glyphs.end(); ++glyph) {
        const PlasmaGlyph &value = *glyph->second;
        copy->setGlyph(glyph->first, value.m_position, value.m_size, value.m_node, value.m_advance);
    }
    copy->m_kerning = m_kerning;
    copy->m_size = m_size;
    copy->m_spacing = m_spacing;
    return copy;
}

void PlasmaFont::renderText(const std::wstring &text, const Vector<2, float> &position, const float *fill,
                            const float *outline, int flags)
{
    m_engine->pushFillColor();
    m_engine->pushOutlineColor();
    m_engine->setFillColor(fill);
    m_engine->setOutlineColor(outline);
    Vector<2, float> unaligned;
    Vector<2, float> aligned;
    Vector<2, float> extent;
    getBounds(text, unaligned, extent, 0);
    getBounds(text, aligned, extent, flags);
    aligned[0] = (aligned[0] - unaligned[0]) + position[0];
    aligned[1] = (aligned[1] - unaligned[1]) + position[1];
    float x = 0.0f;
    float y = 0.0f;
    // Glyph height is twice the font size; doubles the sum before scaling.
    float scale = (m_size + m_size) / (m_maximum.m_elements[1] - m_minimum.m_elements[1]);
    int count = text.size();
    for (int character = 0; character < count; character++) {
        if (text[character] == L'\n') {
            x = 0.0f;
            y = m_maximum.m_elements[1] - m_minimum.m_elements[1] + y;
        } else {
            // operator[] inserts an empty entry when a character is missing.
            PlasmaGlyph *glyph = m_glyphs[text[character]];
            if (glyph) {
                Vector<2, float> origin(x, y - m_minimum.m_elements[1]);
                origin = origin + glyph->m_position;
                origin = origin * scale;
                glyph->m_node->m_transformation->m_translation.value() = aligned + origin;
                glyph->m_node->m_transformation->m_deformation.value().identity();
                glyph->m_node->m_transformation->m_deformation.value().scale(scale, scale);
                glyph->m_node->m_transformation->attributesChanged(true);
                glyph->m_node->draw(0);
                x = glyph->m_advance + x;
                if (character < count - 1)
                    x = getKerning(text[character], text[character + 1]) + x;
            }
        }
    }
    m_engine->popFillColor();
    m_engine->popOutlineColor();
}

void PlasmaFont::getBounds(const std::wstring &text, Vector<2, float> &minimum, Vector<2, float> &maximum, int flags)
{
    minimum.m_elements[0] = 0.0f;
    minimum.m_elements[1] = 0.0f;
    maximum.m_elements[0] = 0.0f;
    maximum.m_elements[1] = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
    float scale = (m_size + m_size) / (m_maximum.m_elements[1] - m_minimum.m_elements[1]);
    int count = text.size();
    for (int character = 0; character < count; character++) {
        if (text[character] == L'\n') {
            x = 0.0f;
            y = m_maximum.m_elements[1] - m_minimum.m_elements[1] + y;
        } else {
            PlasmaGlyph *glyph = getGlyph(text[character]);
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
    for (int axis = 0; axis < 2; axis++)
        minimum.m_elements[axis] *= scale;
    for (int axis = 0; axis < 2; axis++)
        maximum.m_elements[axis] *= scale;
    if (flags & (1 << TEXT_CENTER)) {
        float center = 0.5f * (maximum.m_elements[0] + minimum.m_elements[0]);
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

void PlasmaFont::getCaret(const std::wstring &text, int index, Vector<2, float> &position, Vector<2, float> &size,
                          int flags)
{
    std::wstring characters = text + L" ";
    Vector<2, float> lower(0.0f, 0.0f);
    Vector<2, float> upper(0.0f, 0.0f);
    float x = 0.0f;
    float y = 0.0f;
    float scale = (m_size + m_size) / (m_maximum.m_elements[1] - m_minimum.m_elements[1]);
    if (index < 0)
        index = 0;
    int last = characters.size() - 1;
    if (index > last)
        index = last;
    for (int character = 0; character <= index; character++) {
        if (characters[character] == L'\n') {
            x = 0.0f;
            y = (m_maximum.m_elements[1] - m_minimum.m_elements[1]) + y;
        } else {
            PlasmaGlyph *glyph = getGlyph(characters[character]);
            if (glyph) {
                Vector<2, float> origin = glyph->m_position;
                origin.m_elements[0] = origin.m_elements[0] + x;
                origin.m_elements[1] = origin.m_elements[1] + y;
                lower = origin;
                upper = lower + glyph->m_size;
                x = x + glyph->m_advance;
                if (character < last)
                    x = getKerning(characters[character], characters[character + 1]) + x;
            }
        }
    }
    lower.m_elements[0] = lower.m_elements[0] * scale;
    upper.m_elements[0] = upper.m_elements[0] * scale;
    lower.m_elements[1] = lower.m_elements[1] * scale;
    upper.m_elements[1] = upper.m_elements[1] * scale;
    if (flags & (1 << TEXT_CENTER)) {
        float center = (lower.m_elements[0] + upper.m_elements[0]) * 0.5f;
        lower.m_elements[0] -= center;
        upper.m_elements[0] -= center;
    } else if (flags & (1 << TEXT_RIGHT)) {
        lower.m_elements[0] -= upper.m_elements[0];
        upper.m_elements[0] = 0.0f;
    }
    if (flags & (1 << TEXT_MIDDLE)) {
        float center = 0.5f * (upper.m_elements[1] + lower.m_elements[1]);
        lower.m_elements[1] -= center;
        upper.m_elements[1] -= center;
    } else if (flags & (1 << TEXT_BOTTOM)) {
        lower.m_elements[1] -= upper.m_elements[1];
        upper.m_elements[1] = 0.0f;
    }
    position = lower;
    size = upper - lower;
}

void PlasmaFont::setGlyph(wchar_t character, const Vector<2, float> &position, const Vector<2, float> &size, Node *node,
                          float advance)
{
    if (m_glyphs.empty()) {
        m_minimum = position;
        m_maximum = size;
    }
    PlasmaGlyph *glyph = m_glyphs[character];
    if (!glyph) {
        glyph = new PlasmaGlyph();
        m_glyphs[character] = glyph;
    }
    if (position.m_elements[0] < m_minimum.m_elements[0])
        m_minimum.m_elements[0] = position.m_elements[0];
    if (position.m_elements[1] < m_minimum.m_elements[1])
        m_minimum.m_elements[1] = position.m_elements[1];
    if (m_maximum.m_elements[0] < position.m_elements[0] + size.m_elements[0])
        m_maximum.m_elements[0] = position.m_elements[0] + size.m_elements[0];
    float top = position.m_elements[1] + size.m_elements[1];
    if (m_maximum.m_elements[1] < top)
        m_maximum.m_elements[1] = top;
    glyph->m_position = position;
    glyph->m_size = size;
    glyph->m_node = node;
    glyph->m_advance = advance;
}

PlasmaGlyph *PlasmaFont::getGlyph(wchar_t character)
{
    std::map<wchar_t, PlasmaGlyph *>::iterator glyph = m_glyphs.find(character);
    if (glyph != m_glyphs.end())
        return glyph->second;
    return nullptr;
}

}
