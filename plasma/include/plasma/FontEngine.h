// FontEngine.cpp: owns the scalable faces, search paths and FreeType library.
#pragma once
#include <windows.h>
#include <list>
#include <map>
#include <string>

typedef struct FT_LibraryRec_ *FT_Library;

namespace plasma {

class Engine;
class ScalableFont;

class FontEngine {
public:
    explicit FontEngine(Engine *engine);
    virtual ~FontEngine();

    void collectUnusedGlyphs();
    void reloadFont(const std::wstring &path);
    void addFontPath(const std::wstring &path);
    ScalableFont *getFont(const std::wstring &path);
    void renderText(const std::wstring &path, const std::wstring &text, float spacing, float leading, float x, float y,
                    float size, float stroke, const float *fill, const float *outline, const float *extrusion,
                    int flags, float width, bool hinted);
    void lock();
    void unlock();

    Engine *m_engine;
    std::map<std::wstring, ScalableFont *> m_fonts;
    FT_Library m_freeType;
    std::list<std::wstring> m_fontPaths;
    CRITICAL_SECTION m_lock;
};

}
