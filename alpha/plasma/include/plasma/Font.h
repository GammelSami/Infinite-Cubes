// Font defines glyph measurement and text rendering for legacy fonts.
#pragma once
#include <map>
#include <list>
#include "plasma/NamedObject.h"
#include "plasma/Vector.h"

namespace plasma {

class Node;
class Texture;

// The text layout flags: TextShape::m_flags ("TextShape.flags"), passed to the fonts' (Font's and
// ScalableFont's) renderText, measure, getBounds and getCaret. Individual flags are bit indices;
// use (1 << flag) to form a mask.
enum TextFlags {
    TEXT_CENTER = 0, // horizontally centred
    TEXT_RIGHT = 1,  // right-aligned
    TEXT_MIDDLE = 2, // vertically centred
    TEXT_BOTTOM = 3,
    TEXT_WRAP = 4 // "text:wrap": wrapped at m_wrapWidth
};

// Whole-field masks; the flags above are bit indices.
enum TextMasks {
    TEXT_FLAGS_NONE = 0,
    TEXT_HORIZONTAL_MASK = (1 << TEXT_CENTER) | (1 << TEXT_RIGHT),
    TEXT_VERTICAL_MASK = (1 << TEXT_MIDDLE) | (1 << TEXT_BOTTOM)
};

// Base class for legacy fonts.
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
    // The id in Engine::m_fontsById.
    int id() const { return m_id; }
    // The font size and character spacing used by the font cache.
    float size() const { return m_size; }
    float getSpacing() const { return m_spacing; }

    std::map<std::pair<unsigned short, unsigned short>, float> m_kerning;
    std::list<Font *>::iterator m_engineEntry;
    int m_id;
    std::wstring m_path;
    float m_size;
    float m_spacing;
    bool m_transient; // the loaders' transient argument, part of the legacy font cache key
    // (Engine::loadLegacyFont/loadPixelFont)
};

}
