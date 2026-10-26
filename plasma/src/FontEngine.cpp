#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
#include "plasma/Engine.h"
#include <algorithm>

extern "C" int FT_Init_FreeType(FT_Library *library);
extern "C" int FT_Done_FreeType(FT_Library library);

namespace plasma {

FontEngine::FontEngine(Engine *engine)
    : m_engine(engine)
{
    FT_Init_FreeType(&m_freeType);
    InitializeCriticalSectionAndSpinCount(&m_lock, 0x80000400);
}

FontEngine::~FontEngine()
{
    for (std::map<std::wstring, ScalableFont *>::iterator face = m_fonts.begin(); face != m_fonts.end(); ++face) {
        delete face->second;
    }
    FT_Done_FreeType(m_freeType);
    DeleteCriticalSection(&m_lock);
}

void FontEngine::collectUnusedGlyphs()
{
    lock();
    for (std::map<std::wstring, ScalableFont *>::iterator face = m_fonts.begin(); face != m_fonts.end(); ++face) {
        if (face->second)
            face->second->collectUnusedGlyphs();
    }
    unlock();
}

void FontEngine::reloadFont(const std::wstring &path)
{
    lock();
    int slash = path.find_last_of(L'/');
    int backslash = path.find_last_of(L'\\');
    int separator = (std::max)(slash, backslash);
    std::wstring name = path.substr(separator + 1, path.size() - separator - 1);
    std::map<std::wstring, ScalableFont *>::iterator face = m_fonts.find(name);
    if (face != m_fonts.end()) {
        ScalableFont *font = face->second;
        font->load(m_freeType, font->m_path);
    }
    unlock();
}

void FontEngine::addFontPath(const std::wstring &path)
{
    m_fontPaths.push_back(path);
}

ScalableFont *FontEngine::getFont(const std::wstring &path)
{
    lock();
    int slash = path.find_last_of(L'/');
    int backslash = path.find_last_of(L'\\');
    int separator = (std::max)(slash, backslash);
    std::wstring name = path.substr(separator + 1, path.size() - separator - 1);
    std::map<std::wstring, ScalableFont *>::iterator face = m_fonts.find(name);
    if (face != m_fonts.end()) {
        unlock();
        return face->second;
    }
    ScalableFont *font = new ScalableFont(m_engine);
    m_fonts[name] = font;
    if (!font->load(m_freeType, path)) {
        std::list<std::wstring>::iterator search = m_fontPaths.begin();
        for (; search != m_fontPaths.end(); ++search) {
            if (font->load(m_freeType, *search + L"/" + name))
                break;
        }
        if (search == m_fontPaths.end()) {
            delete font;
            m_fonts.erase(name);
            unlock();
            return 0;
        }
    }
    unlock();
    return font;
}

void FontEngine::renderText(const std::wstring &path, const std::wstring &text, float spacing, float leading, float x,
                            float y, float size, float stroke, const float *fill, const float *outline,
                            const float *extrusion, int flags, float width, bool hinted)
{
    ScalableFont *font = getFont(path);
    if (font) {
        lock();
        m_engine->beginTextDrawing(0);
        font->renderText(text, x, y, size, stroke, spacing, leading, width, fill, outline, extrusion, flags, hinted);
        m_engine->endTextDrawing(0);
        unlock();
    }
}

void FontEngine::lock()
{
    EnterCriticalSection(&m_lock);
}

void FontEngine::unlock()
{
    LeaveCriticalSection(&m_lock);
}

}
