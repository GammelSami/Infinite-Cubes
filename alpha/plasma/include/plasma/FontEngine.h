// Font ownership and search paths.
#pragma once
#include <windows.h>
#include <list>
#include <map>
#include <string>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace plasma {
class Engine;
class ScalableFont;

class FontEngine {
public:
    explicit FontEngine(Engine *engine);
    virtual ~FontEngine();
    void addFontPath(const std::wstring &path);
    void collectUnusedGlyphs();
    void lock();
    ScalableFont *getFont(const std::wstring &path);
    void unlock();
    void renderText(const std::wstring &path, const std::wstring &text, float spacing, float leading, float x, float y,
                    float size, float stroke, const float *fill, const float *outline, const float *extrusion,
                    int flags, float width, bool hinted);

protected:
    Engine *m_engine;
    std::map<std::wstring, std::vector<unsigned char>> m_fontData;
    std::map<std::wstring, ScalableFont *> m_fonts;
    FT_Library m_freeType;
    std::list<std::wstring> m_fontPaths;
    CRITICAL_SECTION m_lock;
};
}
