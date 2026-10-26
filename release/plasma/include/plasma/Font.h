// plasma::Font: the abstract base of the engine's two legacy fonts, PlasmaFont (glyphs are scene nodes) and PixelFont
// (glyphs are rectangles of a texture). It holds the kerning table, the font's registration with the engine (the
// engine's font list and id map), the file path it was loaded from, and the size/spacing/transient values the engine's
// font loaders store (the legacy font cache key).
#pragma once
#include "plasma/TextIds.h"
#include <list>
#include <map>
#include <string>
#include <utility>
#include "plasma/NamedObject.h"
#include "plasma/Vector.h"

namespace plasma {

class Engine;

class Font : public NamedObject {
public:
    Font(Engine *engine, const std::wstring &name);
    virtual ~Font();
    // Font measurement and rendering interface.
    virtual void getCaret(const std::wstring &text, int index, Vector<2, float> &position, Vector<2, float> &size,
                          int flags) = 0;
    virtual void getBounds(const std::wstring &text, Vector<2, float> &minimum, Vector<2, float> &maximum,
                           int flags) = 0;
    virtual Vector<2, float> getMinimum() = 0;
    virtual Vector<2, float> getMaximum() = 0;
    virtual void renderText(const std::wstring &text, const Vector<2, float> &position, const float *fill,
                            const float *outline, int flags);

    float getKerning(unsigned short left, unsigned short right);

    std::map<std::pair<unsigned short, unsigned short>, float>
        m_kerning;                             // constructor, getKerning; copied by PlasmaFont::clone
    std::list<Font *>::iterator m_engineEntry; // Registration entry in the engine's font list.
    int m_id;                                  // key in the engine's font id map (engine), constructor
    std::wstring m_path;                       // Loaded font path used by the font cache.
    float m_size;                              // Font size set by loaders and used to scale glyph bounds.
    float m_spacing;                           // Loader-supplied spacing, copied by PlasmaFont::clone.
    bool m_transient; // Transient flag supplied to the PixelFont loader; part of the legacy font cache key.
};

}
