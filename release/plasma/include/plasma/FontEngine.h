// FontEngine owns the FreeType library, cached ScalableFonts keyed by file name, font search paths and in-memory font
// data, and renders text through a named font.
#pragma once
#include <windows.h>
#include <list>
#include <map>
#include <string>
#include <vector>
#include "plasma/Vector.h"

// FreeType's opaque library handle (freetype.h declares the same typedef).
typedef struct FT_LibraryRec_ *FT_Library;

namespace plasma {

class Engine;
class ScalableFont;

class FontEngine {
public:
    explicit FontEngine(Engine *engine);
    virtual ~FontEngine();

    // Appends a directory to m_fontPaths.
    void addFontPath(const std::wstring &path);
    // Returns the font for the file name part of path, loading it on first use: from path itself (with m_fontData),
    // then from each font path + L"/" + name. Destroys the font and returns null if no load succeeds.
    ScalableFont *getFont(const std::wstring &path);
    Vector<2, float> renderText(const std::wstring &path, const std::wstring &text, float spacing, float leading,
                                float x, float y, float size, float stroke, const float *fill, const float *outline,
                                const float *extrusion, int flags, float width, bool hinted);

    // This lock is not entered here.
    void lock();
    void unlock();

protected:
    Engine *m_engine;
    // Optional font bytes supplied to ScalableFont::load and the memory font loader; no writer is present in
    // FontEngine.cpp.
    std::map<std::wstring, std::vector<unsigned char>> m_fontData;
    std::map<std::wstring, ScalableFont *> m_fonts;
    FT_Library m_freeType;
    std::list<std::wstring> m_fontPaths;
    CRITICAL_SECTION m_lock; // Critical section initialised and deleted, with no other use.
};

}
