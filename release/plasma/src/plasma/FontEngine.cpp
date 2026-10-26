// Font lookup uses file-name cache keys, in-memory data and search directories. renderText returns the final pen
// position. The lock is only initialised and deleted; lock and unlock do nothing.
#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
#include "plasma/Engine.h"
#include <algorithm>
#include "plasma/EditFilter.h"

namespace plasma {

static const int FontCriticalSectionSpinCount = 16;

FontEngine::FontEngine(Engine *engine)
    : m_engine(engine)
{
    FT_Init_FreeType(&m_freeType);
    InitializeCriticalSectionAndSpinCount(&m_lock, FontCriticalSectionSpinCount);
}

FontEngine::~FontEngine()
{
    for (std::map<std::wstring, ScalableFont *>::iterator font = m_fonts.begin(); font != m_fonts.end(); ++font) {
        delete font->second;
    }
    FT_Done_FreeType(m_freeType);
    DeleteCriticalSection(&m_lock);
}

void FontEngine::addFontPath(const std::wstring &path)
{
    m_fontPaths.push_back(path);
}

ScalableFont *FontEngine::getFont(const std::wstring &path)
{
    // The key is the file name without its directory.
    int slash = path.find_last_of(L'/');
    int backslash = path.find_last_of(L'\\');
    int separator = (std::max)(slash, backslash);
    std::wstring name = path.substr(separator + 1, path.size() - separator - 1);
    std::map<std::wstring, ScalableFont *>::iterator known = m_fonts.find(name);
    if (known != m_fonts.end())
        return known->second;
    // The font is registered before it is loaded and removed again if every load fails.
    ScalableFont *font = new ScalableFont(m_engine);
    m_fonts[name] = font;
    if (!font->load(m_freeType, path, &m_fontData)) {
        // Not at the given path: try every font directory (files only, no in-memory table).
        bool loaded = false;
        for (std::list<std::wstring>::iterator directory = m_fontPaths.begin(); directory != m_fontPaths.end();
             ++directory) {
            if (font->load(m_freeType, *directory + L"/" + name, nullptr)) {
                loaded = true;
                break;
            }
        }
        if (!loaded) {
            delete font;
            m_fonts.erase(name);
            return nullptr;
        }
    }
    return font;
}

Vector<2, float> FontEngine::renderText(const std::wstring &path, const std::wstring &text, float spacing,
                                        float leading, float x, float y, float size, float stroke, const float *fill,
                                        const float *outline, const float *extrusion, int flags, float width,
                                        bool hinted)
{
    ScalableFont *font = getFont(path);
    // Without the font nothing is drawn and the start position comes back.
    Vector<2, float> end(x, y);
    if (font) {
        m_engine->beginTextDrawing(nullptr);
        end = font->renderText(text, x, y, size, stroke, spacing, leading, width, fill, outline, extrusion, flags,
                               hinted);
        m_engine->endTextDrawing(nullptr);
    }
    return end;
}

void FontEngine::lock() {}

void FontEngine::unlock() {}

}
