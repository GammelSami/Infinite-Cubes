// ScalableFont.cpp: FreeType faces, raster glyph caches and text layout.
#pragma once
#include <windows.h>
#include <map>
#include <string>
#include <vector>
#include "plasma/Matrix.h"
#include "plasma/PixelFont.h"

#ifdef PLASMA_PORTABLE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H
#else

// FreeType constants used for font loading and stroking.
enum {
    FT_LOAD_DEFAULT = 0x0,
    FT_LOAD_NO_SCALE = 0x1,
    FT_LOAD_NO_HINTING = 0x2,
    FT_LOAD_NO_BITMAP = 0x8,
    FT_LOAD_NO_AUTOHINT = 0x8000
};
enum {
    FT_KERNING_DEFAULT = 0,
    FT_KERNING_UNFITTED = 1,
    FT_KERNING_UNSCALED = 2
};
enum {
    FT_RENDER_MODE_NORMAL = 0
};
enum {
    FT_CURVE_TAG_ON = 1
};
enum {
    FT_STROKER_LINEJOIN_ROUND = 0
};
enum {
    FT_STROKER_LINECAP_ROUND = 1
};
typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_FaceRec_ *FT_Face;
typedef struct FT_GlyphSlotRec_ *FT_GlyphSlot;
typedef struct FT_StrokerRec_ *FT_Stroker;
struct FT_Vector {
    long x;
    long y;
};
struct FT_Matrix {
    long xx;
    long xy;
    long yx;
    long yy;
};
struct FT_BBox {
    long xMin;
    long yMin;
    long xMax;
    long yMax;
};
struct FT_Generic {
    void *data;
    void (*finalizer)(void *);
};
struct FT_Bitmap {
    int rows;
    int width;
    int pitch;
    unsigned char *buffer;
    short num_grays;
    char pixel_mode;
    char palette_mode;
    void *palette;
};
struct FT_Outline {
    short n_contours;
    short n_points;
    FT_Vector *points;
    char *tags;
    short *contours;
    int flags;
};
struct FT_Glyph_Metrics {
    long width;
    long height;
    long horiBearingX;
    long horiBearingY;
    long horiAdvance;
    long vertBearingX;
    long vertBearingY;
    long vertAdvance;
};
struct FT_GlyphSlotRec_ {
    FT_Library library;
    FT_Face face;
    FT_GlyphSlot next;
    unsigned reserved;
    FT_Generic generic;
    FT_Glyph_Metrics metrics;
    long linearHoriAdvance;
    long linearVertAdvance;
    FT_Vector advance;
    unsigned long format;
    FT_Bitmap bitmap;
    int bitmap_left;
    int bitmap_top;
    FT_Outline outline;
};
struct FT_FaceRec_ {
    long num_faces;
    long face_index;
    long face_flags;
    long style_flags;
    long num_glyphs;
    char *family_name;
    char *style_name;
    int num_fixed_sizes;
    void *available_sizes;
    int num_charmaps;
    void *charmaps;
    FT_Generic generic;
    FT_BBox bbox;
    unsigned short units_per_EM;
    short ascender;
    short descender;
    short height;
    short max_advance_width;
    short max_advance_height;
    short underline_position;
    short underline_thickness;
    FT_GlyphSlot glyph;
};
struct FT_GlyphRec_ {
    FT_Library library;
    const void *clazz;
    unsigned long format;
    FT_Vector advance;
};
typedef FT_GlyphRec_ *FT_Glyph;
struct FT_BitmapGlyphRec_ {
    FT_GlyphRec_ root;
    int left;
    int top;
    FT_Bitmap bitmap;
};

#endif

namespace plasma {

class Engine;
class Node;
class Texture;
class ScalableFont;

struct ScalableGlyph {
    Vector<2, float> m_position;
    Vector<2, float> m_size;
    Vector<2, float> m_advance;
    Vector<2, float> m_minimum;
    Vector<2, float> m_maximum;
    Texture *m_texture;
    Node *m_node; // The outline glyph's mesh node; null for bitmap glyphs.
    bool m_used;
};

// A glyph cache with std::map storage and no additional fields; raster helpers receive the font separately.
class ScalableGlyphMap : public std::map<int, ScalableGlyph *> {
public:
    ScalableGlyph *getBitmapGlyph(ScalableFont *font, unsigned short character, bool stroke);
    ScalableGlyph *getTransformedGlyph(ScalableFont *font, unsigned short character, bool stroke);
};
typedef std::pair<std::pair<int, int>, int> ScalableSizeKey;
typedef std::pair<std::pair<int, int>, std::vector<long>> ScalableTransformKey;
typedef std::map<ScalableSizeKey, ScalableGlyphMap *> ScalableSizeCache;
typedef std::map<ScalableTransformKey, ScalableGlyphMap *> ScalableTransformCache;

class ScalableFont {
public:
    explicit ScalableFont(Engine *engine);
    virtual ~ScalableFont();

    void lock();
    void unlock();
    void clear();
    bool load(FT_Library library, const std::wstring &path);
    bool loadTrueType(FT_Library library, const std::wstring &path);
    bool loadPlasma(const std::wstring &path);
    void setSize(float size, float stroke);
    void setSize(float width, float height, float stroke);
    ScalableGlyph *getGlyph(unsigned short character);
    ScalableGlyph *getStrokeGlyph(unsigned short character);
    void setTransform(const Matrix<float> &matrix, float size, float stroke);
    ScalableGlyph *getPlasmaGlyph(unsigned short character);
    void wrapText(std::wstring &text, float width);
    Node *convertGlyphToNode(unsigned short character, float size, float stroke, const float *fill,
                             const float *outline, const float *extrusion, std::map<unsigned short, Node *> *cache);
    void convertTextToNodes(Node *parent, const std::wstring &text, float size, float stroke, float spacing,
                            float leading, const float *fill, const float *outline, const float *extrusion, int flags);
    void calculateSize(Vector<2, float> &size, float &stroke, float &spacing, float &leading,
                       const Matrix<float> &matrix, float requestedSize, float requestedStroke, float requestedSpacing,
                       float requestedLeading);
    void prepare(const Matrix<float> &matrix, float size, float stroke, float spacing, float leading, bool hinted);
    void renderText(const std::wstring &text, float x, float y, float size, float stroke, float spacing, float leading,
                    float width, const float *fill, const float *outline, const float *extrusion, int flags,
                    bool hinted);
    void drawGlyphs(const std::wstring &text, float x, float y, const float *fill, const float *outline,
                    const float *extrusion, int flags);
    int hitTestText(const std::wstring &text, const Vector<2, float> &point, int flags);
    void getCaret(const std::wstring &text, int index, Vector<2, float> &position, Vector<2, float> &size, int flags);
    int hitTest(const std::wstring &text, const Matrix<float> &matrix, const Vector<2, float> &point, float size,
                float stroke, float spacing, float leading, float width, int flags);
    void getCaret(const Matrix<float> &matrix, const std::wstring &text, int index, float size, float stroke,
                  float spacing, float leading, float width, Vector<2, float> &position, Vector<2, float> &extent,
                  int flags, bool hinted);
    void measure(const std::wstring &text, const Matrix<float> &matrix, float size, float stroke, float spacing,
                 float leading, float width, Vector<2, float> &minimum, Vector<2, float> &maximum, int flags,
                 bool hinted);
    void collectUnusedGlyphs();
    void getBounds(const std::wstring &text, Vector<2, float> &minimum, Vector<2, float> &maximum, int flags,
                   int lineStart, bool advancesOnly);

    static long toFreeTypeFixed(float value)
    {
        // Float-to-long conversion is undefined for NaN or out-of-range values.
        return (long)(value * 65536.0);
    }

    Engine *m_engine;
    std::wstring m_path;
    std::wstring m_style;
    std::wstring m_family;
    std::wstring m_postscriptName;
    ScalableSizeCache m_sizeCache;
    ScalableTransformCache m_transformCache;
    FT_Matrix m_ftTransform;
    Matrix<float> m_transform;
    ScalableGlyphMap *m_glyphs;
    ScalableGlyphMap *m_strokeGlyphs;
    ScalableGlyphMap m_plasmaGlyphs;
    FT_Face m_face;
    FT_Stroker m_stroker;
    float m_width;
    float m_height;
    float m_stroke; // (not initialised).
    float m_spacing;
    float m_leading;
    Node *m_plasmaRoot;
    Vector<2, float> m_minimum; // (not initialised).
    Vector<2, float> m_maximum; // (not initialised).
    bool m_hinted;
    CRITICAL_SECTION m_lock;
};

}
