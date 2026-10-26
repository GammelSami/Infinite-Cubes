// ScalableFont opens FreeType TTF/DAT fonts or Plasma documents whose single-character nodes are glyphs. FontEngine
// owns it. Per-size and per-transform glyph caches support text layout, measuring, caret placement and drawing.
#pragma once
#include <windows.h>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

// Narrows each character of a wide string.
std::string NarrowString(const std::wstring &text);

namespace plasma {

class Engine;
class Node;
class Texture;

// One cached glyph.
struct ScalableGlyph {
    Vector<2, float> m_position;
    Vector<2, float> m_size;
    Vector<2, float> m_advance;
    Vector<2, float> m_minimum;
    Vector<2, float> m_maximum;
    Texture *m_texture; // set by getBitmapGlyph; null for Plasma glyphs
    Node *m_node;       // the Plasma glyph's node; null for bitmap glyphs
    bool m_used;        // Set by getGlyph when the glyph is fetched.
};

typedef std::map<int, ScalableGlyph *> ScalableGlyphMap;
typedef std::pair<std::pair<int, int>, int> ScalableSizeKey;
typedef std::pair<std::pair<int, int>, std::vector<long>> ScalableTransformKey;
typedef std::map<ScalableSizeKey, ScalableGlyphMap *> ScalableSizeCache;
typedef std::map<ScalableTransformKey, ScalableGlyphMap *> ScalableTransformCache;

class ScalableFont {
public:
    explicit ScalableFont(Engine *engine);
    virtual ~ScalableFont();

    void lock() { EnterCriticalSection(&m_lock); }
    void unlock() { LeaveCriticalSection(&m_lock); }

    // Deletes the face, stroker, every cached glyph and the Plasma root.
    void clear();
    // Stores path, then by extension: .TTF/.DAT -> loadTrueTypeFromMemory if fontData has a non-empty entry for path,
    // else loadTrueType; anything else -> loadPlasma. fontData may be null (getFont's search-path retries).
    bool load(FT_Library library, const std::wstring &path,
              std::map<std::wstring, std::vector<unsigned char>> *fontData);
    bool loadTrueType(FT_Library library, const std::wstring &path);
    bool loadTrueTypeFromMemory(FT_Library library, const std::wstring &path, const unsigned char *data, long size);
    bool loadPlasma(const std::wstring &path);
    void setSize(float size, float stroke); // (forwards)
    void setSize(float width, float height, float stroke);
    ScalableGlyph *getGlyph(unsigned short character);
    ScalableGlyph *getStrokeGlyph(unsigned short character)
    {
        if (m_hinted)
            return getBitmapGlyph(m_strokeGlyphs, character, true);
        return getTransformedGlyph(m_strokeGlyphs, character, true);
    }
    ScalableGlyph *getPlasmaGlyph(unsigned short character)
    {
        int key = character;
        ScalableGlyphMap::iterator glyph = m_plasmaGlyphs.find(key);
        if (glyph != m_plasmaGlyphs.end()) {
            glyph->second->m_used = true;
            return glyph->second;
        }
        return nullptr;
    }
    void setTransform(float size, float stroke, const Matrix<float> &matrix);
    ScalableGlyph *getBitmapGlyph(ScalableGlyphMap *glyphs, unsigned short character, bool stroke);
    ScalableGlyph *getTransformedGlyph(ScalableGlyphMap *glyphs, unsigned short character, bool stroke);
    void wrapText(std::wstring &text, float width);
    void calculateSize(const Matrix<float> &matrix, float requestedSize, float requestedStroke, float requestedSpacing,
                       float requestedLeading, Vector<2, float> &size, float &stroke, float &spacing, float &leading);
    void prepare(float size, float stroke, float spacing, float leading, const Matrix<float> &matrix, bool hinted);
    Vector<2, float> renderText(const std::wstring &text, float x, float y, float size, float stroke, float spacing,
                                float leading, float width, const float *fill, const float *outline,
                                const float *extrusion, int flags, bool hinted);
    // Renders text and returns the ending pen position.
    Vector<2, float> drawGlyphs(const std::wstring &text, float x, float y, const float *fill, const float *outline,
                                const float *extrusion, int flags);
    void getCaret(const std::wstring &text, int index, Vector<2, float> &position, Vector<2, float> &size, int flags);
    void getCaret(const std::wstring &text, int index, const Matrix<float> &matrix, float size, float stroke,
                  float spacing, float leading, float width, Vector<2, float> &position, Vector<2, float> &extent,
                  int flags, bool hinted);
    void measure(const std::wstring &text, const Matrix<float> &matrix, float size, float stroke, float spacing,
                 float leading, float width, Vector<2, float> &minimum, Vector<2, float> &maximum, int flags,
                 bool hinted, int lineStart, bool advancesOnly);
    void getBounds(const std::wstring &text, Vector<2, float> &minimum, Vector<2, float> &maximum, int flags,
                   int lineStart, bool advancesOnly);

    // 16.16 fixed point, as FreeType's matrices hold it.
    static long toFreeTypeFixed(float value) { return (long)(value * 65536.0f); }

protected:
    Engine *m_engine;
    std::wstring m_path; // assigned by load
    std::wstring m_style;
    std::wstring m_family;
    std::wstring m_postscriptName;
    ScalableSizeCache m_sizeCache;           // Glyph cache, destroyed with the font.
    ScalableTransformCache m_transformCache; // Stroke-glyph cache, destroyed with the font.
    FT_Matrix m_ftTransform;                 // identity, 0, 0, from the constructor; setTransform
    Matrix<float> m_transform;               // Initially identity; updated by setTransform.
    ScalableGlyphMap *m_glyphs;              // zeroed by the constructor; getGlyph
    ScalableGlyphMap *m_strokeGlyphs;        // zeroed by the constructor
    ScalableGlyphMap m_plasmaGlyphs;         // Glyph cache searched by getGlyph and destroyed with the font.
    FT_Face m_face;
    FT_Stroker m_stroker;
    float m_width;              // setSize
    float m_height;             // setSize
    float m_stroke;             // setSize (not initialised, as in plasma)
    float m_spacing;            // prepare
    float m_leading;            // prepare
    Node *m_plasmaRoot;         // loadPlasma; tested by getGlyph
    Vector<2, float> m_minimum; // loadPlasma, loadTrueType (not initialised)
    Vector<2, float> m_maximum; // loadPlasma, loadTrueType (not initialised)
    bool m_hinted;              // 1 in the constructor; set by setSize, cleared by prepare
    CRITICAL_SECTION m_lock;    // Critical section initialised by the constructor.
};

}
