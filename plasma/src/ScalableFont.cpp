#include "plasma/FilePath.h"
#include "plasma/ScalableFont.h"
#include "plasma/Engine.h"
#include "plasma/Texture.h"
#include "plasma/Node.h"
#include "plasma/Shape.h"
#include "plasma/Transformation.h"
#include <fstream>
#include "main.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include "plasma/SmoothMeshShape.h"

#ifndef PLASMA_PORTABLE
extern "C" {
int FT_Done_Face(FT_Face face);
int FT_New_Face(FT_Library library, const char *path, long index, FT_Face *face);
const char *FT_Get_Postscript_Name(FT_Face face);
int FT_Set_Char_Size(FT_Face face, long width, long height, unsigned xResolution, unsigned yResolution);
void FT_Set_Transform(FT_Face face, FT_Matrix *matrix, FT_Vector *delta);
int FT_Load_Char(FT_Face face, unsigned long character, long flags);
unsigned FT_Get_Char_Index(FT_Face face, unsigned long character);
int FT_Get_Kerning(FT_Face face, unsigned left, unsigned right, unsigned mode, FT_Vector *kerning);
int FT_Get_Glyph(FT_GlyphSlot slot, FT_Glyph *glyph);
int FT_Glyph_To_Bitmap(FT_Glyph *glyph, int mode, const FT_Vector *origin, unsigned char destroy);
void FT_Done_Glyph(FT_Glyph glyph);
int FT_Stroker_New(FT_Library library, FT_Stroker *stroker);
void FT_Stroker_Done(FT_Stroker stroker);
void FT_Stroker_Set(FT_Stroker stroker, long radius, int cap, int join, long limit);
int FT_Glyph_Stroke(FT_Glyph *glyph, FT_Stroker stroker, unsigned char destroy);
}
#endif

std::wstring widenString(const std::string &text);

// The maximum stroke radius accepted by CalculateSize. The global is mutable but never changed.
float g_maxFontStroke = 500.0f;

namespace plasma {

ScalableFont::ScalableFont(Engine *engine)
    : m_engine(engine),
      m_glyphs(0),
      m_strokeGlyphs(0),
      m_face(0),
      m_stroker(0),
      m_width(0.0f),
      m_height(0.0f),
      m_spacing(0.0f),
      m_leading(0.0f),
      m_plasmaRoot(0),
      m_hinted(true)
{
    m_ftTransform.xx = toFreeTypeFixed(1.0f);
    m_ftTransform.xy = toFreeTypeFixed(0.0f);
    m_ftTransform.yx = m_ftTransform.xy;
    m_ftTransform.yy = toFreeTypeFixed(1.0f);
    m_transform.identity();
    InitializeCriticalSectionAndSpinCount(&m_lock, 0x80000400);
}

ScalableFont::~ScalableFont()
{
    clear();
    DeleteCriticalSection(&m_lock);
}

void ScalableFont::lock()
{
    EnterCriticalSection(&m_lock);
}

void ScalableFont::unlock()
{
    LeaveCriticalSection(&m_lock);
}

void ScalableFont::clear()
{
    lock();
    if (m_face) {
        FT_Done_Face(m_face);
        m_face = 0;
    }
    if (m_stroker) {
        FT_Stroker_Done(m_stroker);
        m_stroker = 0;
    }
    for (ScalableSizeCache::iterator size = m_sizeCache.begin(); size != m_sizeCache.end(); ++size) {
        if (size->second) {
            for (ScalableGlyphMap::iterator glyph = size->second->begin(); glyph != size->second->end(); ++glyph) {
                if (glyph->second)
                    delete glyph->second;
            }
            delete size->second;
        }
    }

    // The Clear leaves the active cache pointers unchanged after deleting their maps.
    m_sizeCache.clear();
    for (ScalableTransformCache::iterator transform = m_transformCache.begin(); transform != m_transformCache.end();
         ++transform) {
        if (transform->second) {
            for (ScalableGlyphMap::iterator glyph = transform->second->begin(); glyph != transform->second->end();
                 ++glyph) {
                if (glyph->second)
                    delete glyph->second;
            }
            delete transform->second;
        }
    }
    m_transformCache.clear();
    for (ScalableGlyphMap::iterator glyph = m_plasmaGlyphs.begin(); glyph != m_plasmaGlyphs.end(); ++glyph) {
        if (glyph->second)
            delete glyph->second;
    }
    m_plasmaGlyphs.clear();
    if (m_plasmaRoot) {
        m_engine->destroyNode(m_plasmaRoot);
        m_plasmaRoot = 0;
    }
    unlock();
}

bool ScalableFont::load(FT_Library library, const std::wstring &path)
{
    clear();
    m_path = path;
    int dot = path.find_last_of(L'.');
    std::wstring extension = path.substr(dot + 1, path.size() - dot - 1);

    // The code uses narrow toupper on wchar_t; values outside its domain have UB.
    std::transform(extension.begin(), extension.end(), extension.begin(), toupper);
    if (extension == L"TTF")
        return loadTrueType(library, path);
    return loadPlasma(path);
}

bool ScalableFont::loadTrueType(FT_Library library, const std::wstring &path)
{
    lock();
    int error = FT_New_Face(library, narrowString(path).c_str(), 0, &m_face);
    if (error) {
        unlock();
        return false;
    }
    m_family = widenString(std::string(m_face->family_name));
    m_style = widenString(std::string(m_face->style_name));
    const char *postscript = FT_Get_Postscript_Name(m_face);
    if (postscript)
        m_postscriptName = widenString(std::string(postscript));
    m_minimum = Vector<2, float>((float)m_face->bbox.xMin, (float)m_face->bbox.yMin) / (float)m_face->units_per_EM;
    m_maximum = Vector<2, float>((float)m_face->bbox.xMax, (float)m_face->bbox.yMax) / (float)m_face->units_per_EM;
    FT_Stroker_New(library, &m_stroker);
    unlock();
    return true;
}

bool ScalableFont::loadPlasma(const std::wstring &path)
{
    lock();
    std::ifstream file;
    file.open(PLASMA_FILE_PATH(path.c_str()), std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        unlock();
        return false;
    }
    file.close();
    int slash = path.find_last_of(L'/');
    int backslash = path.find_last_of(L'\\');
    int separator = (std::max)(slash, backslash);
    std::wstring name = path.substr(separator + 1, path.size() - separator - 1);
    m_plasmaRoot = m_engine->createNode(0, 0, 0, 0, name);
    m_family = name;
    m_style = L"";
    {
        std::vector<unsigned __int64> keys(1, 0);
        m_engine->readPlasmaFileIntoNode(path, m_plasmaRoot, (1 << LOAD_SHARED) | (1 << LOAD_NO_WIDGET_INIT),
                                         static_cast<Engine::Page *>(0), keys);
    }
    m_plasmaRoot->m_flags |= (1 << NODE_INTERNAL);
    m_minimum = Vector<2, float>(0.0f, 0.0f);
    m_maximum = Vector<2, float>(0.0f, 0.0f);
    for (std::list<Node *>::iterator child = m_plasmaRoot->m_children.begin(); child != m_plasmaRoot->m_children.end();
         ++child) {
        if ((*child)->getName().size() == 1 && (*child)->m_shape) {
            std::wstring glyphName = (*child)->getName();
            if (m_plasmaGlyphs.find(glyphName[0]) != m_plasmaGlyphs.end())
                continue;
            Shape *shape = (*child)->m_shape;
            Transformation *transformation = (*child)->m_transformation;
            Vector<2, float> minimum = transformation->m_matrix * shape->getMinimum();
            shape = (*child)->m_shape;
            transformation = (*child)->m_transformation;
            Vector<2, float> maximum = transformation->m_matrix * shape->getMaximum();
            Vector<2, float> nodeMinimum = (*child)->getMinimum(0);
            Vector<2, float> nodeMaximum = (*child)->getMaximum(0);
            if ((*child)->m_flags & (1 << NODE_INACTIVE))
                (*child)->setShape(0);
            ScalableGlyph *glyph = new ScalableGlyph;
            m_plasmaGlyphs[glyphName[0]] = glyph;
            glyph->m_advance = maximum - minimum;
            glyph->m_size = maximum - minimum;
            glyph->m_position = Vector<2, float>(minimum.m_elements[0], maximum.m_elements[1]) * -1.0;
            glyph->m_node = *child;
            glyph->m_minimum = nodeMinimum;
            glyph->m_maximum = nodeMaximum;
            glyph->m_texture = 0;
            glyph->m_used = true;

            // The code replaces these bounds for each glyph, rather than taking a union.
            m_minimum = glyph->m_position;
            m_maximum = glyph->m_position + glyph->m_size;
        }
    }
    unlock();
    return true;
}

void ScalableFont::setSize(float size, float stroke)
{
    setSize(size, size, stroke);
}

void ScalableFont::setSize(float width, float height, float stroke)
{
    if (height <= 0.0f)
        height = width;
    if (width <= 0.0f) {
        width = height;
        if (width <= 0.0f)
            return;
    }

    // A NaN stroke passes the negative-only rejection.
    if (stroke < 0.0f)
        return;
    m_width = width;
    m_height = height;
    m_stroke = stroke;
    FT_Set_Char_Size(m_face, (long)(width * 64.0 + 0.5), (long)(height * 64.0 + 0.5), 0, 0);
    FT_Set_Transform(m_face, 0, 0);
    m_transform.identity();
    m_hinted = true;
    ScalableSizeCache::iterator found =
        m_sizeCache.find(std::make_pair(std::make_pair((int)(m_width * 64.0 + 0.5), (int)(m_height * 64.0 + 0.5)), 0));
    if (found != m_sizeCache.end()) {
        m_glyphs = found->second;
    } else {
        m_glyphs = new ScalableGlyphMap;
        m_sizeCache[ScalableSizeKey(std::make_pair((int)(m_width * 64.0 + 0.5), (int)(m_height * 64.0 + 0.5)), 0)] =
            m_glyphs;
    }
    if (stroke > 0.0f) {
        found = m_sizeCache.find(std::make_pair(
            std::make_pair((int)(m_width * 64.0 + 0.5), (int)(m_height * 64.0 + 0.5)), (int)(stroke * 64.0 + 0.5)));
        if (found != m_sizeCache.end()) {
            m_strokeGlyphs = found->second;
        } else {
            m_strokeGlyphs = new ScalableGlyphMap;
            m_sizeCache[ScalableSizeKey(std::make_pair((int)(m_width * 64.0 + 0.5), (int)(m_height * 64.0 + 0.5)),
                                        (int)(stroke * 64.0 + 0.5))] = m_strokeGlyphs;
        }
        FT_Stroker_Set(m_stroker, (long)(m_stroke * 64.0 + 0.5), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND,
                       0);
    } else {
        m_strokeGlyphs = 0;
    }
}

ScalableGlyph *ScalableFont::getGlyph(unsigned short character)
{
    if (m_plasmaRoot)
        return getPlasmaGlyph(character);
    if (m_hinted)
        return m_glyphs->getBitmapGlyph(this, character, false);
    return m_glyphs->getTransformedGlyph(this, character, false);
}

ScalableGlyph *ScalableFont::getStrokeGlyph(unsigned short character)
{
    if (m_hinted)
        return m_strokeGlyphs->getBitmapGlyph(this, character, true);
    return m_strokeGlyphs->getTransformedGlyph(this, character, true);
}

ScalableGlyph *ScalableGlyphMap::getBitmapGlyph(ScalableFont *font, unsigned short character, bool stroke)
{
    // A null receiver leads to undefined behavior.
    if (!this)
        return 0;
    int key = character;
    ScalableGlyphMap::iterator found = find(character);
    if (found != end()) {
        found->second->m_used = true;
        return found->second;
    }
    if (FT_Set_Char_Size(font->m_face, (long)(64.0 * font->m_width + 0.5), (long)(font->m_height * 64.0 + 0.5), 0, 0) ||
        FT_Load_Char(font->m_face, character, FT_LOAD_NO_BITMAP)) {
        return 0;
    }
    FT_Glyph glyph;
    FT_Get_Glyph(font->m_face->glyph, &glyph);
    if (stroke)
        FT_Glyph_Stroke(&glyph, font->m_stroker, 1);
    FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
    FT_BitmapGlyphRec_ *bitmap = (FT_BitmapGlyphRec_ *)glyph;
    FT_Bitmap pixels = bitmap->bitmap;
    int width = pixels.width + 2;
    int height = pixels.rows + 2;
    Texture *texture = 0;
    if (width > 0 && height > 0) {
        std::vector<unsigned char> rgba(4 * width * height);
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                unsigned char *pixel = &rgba[4 * (y * width + x)];
                pixel[0] = 255;
                pixel[1] = 255;
                pixel[2] = 255;
                if (x <= 0 || y <= 0 || x > pixels.width || y > pixels.rows)
                    pixel[3] = 0;
                else
                    pixel[3] = pixels.buffer[(y - 1) * pixels.pitch + x - 1];
            }
        }
        TextureFormat format;
        format.horizontalWrap = TEXTURE_WRAP_NONE;
        format.verticalWrap = TEXTURE_WRAP_NONE;
        texture = font->m_engine->createTexture(width, height, &rgba[0], format, std::wstring(L"") + (wchar_t)character,
                                                true);
    }
    ScalableGlyph *result = new ScalableGlyph;
    (*this)[key] = result;

    // The metric arithmetic can overflow signed int before conversion to float.
    result->m_position = Vector<2, int>(bitmap->left - 1, -1 - bitmap->top);
    result->m_size = Vector<2, int>(width, height);
    result->m_node = 0;
    result->m_advance = Vector<2, float>((double)glyph->advance.x * (double)0.0000152587890625f,
                                         (double)0.0000152587890625f * (double)glyph->advance.y);
    result->m_texture = texture;
    result->m_used = true;
    FT_Done_Glyph(glyph);
    return result;
}

void ScalableFont::setTransform(const Matrix<float> &matrix, float size, float stroke)
{
    m_transform.m_elements[0] = matrix[0];
    m_transform.m_elements[1] = matrix[1];
    m_transform.m_elements[5] = matrix[5];
    m_transform.m_elements[4] = matrix[4];
    m_ftTransform.xx = toFreeTypeFixed(matrix[0]);

    // The code also negates LONG_MIN on exceptional inputs (signed-overflow UB).
    long yx = toFreeTypeFixed(matrix[1]);
    m_ftTransform.yx = -yx;
    m_ftTransform.yy = toFreeTypeFixed(matrix[5]);
    long xy = toFreeTypeFixed(matrix[4]);
    m_ftTransform.xy = -xy;

    // The code lets NaN enter this path; an unordered comparison is not <= zero.
    if (!(size <= 0.0f)) {
        m_width = size;
        m_hinted = false;
        m_height = size;
        Vector<2, float> x(matrix[0] * 1.0f + matrix[4] * 0.0f, matrix[1] * 1.0f + matrix[5] * 0.0f);
        Vector<2, float> y(matrix[0] * 0.0f + matrix[4] * 1.0f, matrix[1] * 0.0f + matrix[5] * 1.0f);
        float yLength = std::sqrt(y.m_elements[0] * y.m_elements[0] + y.m_elements[1] * y.m_elements[1]);
        float xLength = std::sqrt(x.m_elements[0] * x.m_elements[0] + x.m_elements[1] * x.m_elements[1]);
        m_stroke = (std::min)(xLength, yLength) * stroke;
        FT_Set_Char_Size(m_face, (long)(size * 64.0 + 0.5), (long)(size * 64.0 + 0.5), 0, 0);
        FT_Set_Transform(m_face, &m_ftTransform, 0);
        std::vector<long> transform;
        transform.resize(4);
        transform[0] = m_ftTransform.xx;
        transform[1] = m_ftTransform.xy;
        transform[2] = m_ftTransform.yx;
        transform[3] = m_ftTransform.yy;
        ScalableTransformCache::iterator found =
            m_transformCache.find(std::make_pair(std::make_pair((int)(size * 64.0 + 0.5), 0), transform));
        if (found == m_transformCache.end()) {
            m_glyphs = new ScalableGlyphMap;
            m_transformCache[std::make_pair(std::make_pair((int)(size * 64.0 + 0.5), 0), transform)] = m_glyphs;
        } else {
            m_glyphs = found->second;
        }
        if (stroke > 0.0f) {
            found = m_transformCache.find(
                std::make_pair(std::make_pair((int)(size * 64.0 + 0.5), (int)(stroke * 64.0 + 0.5)), transform));
            if (found == m_transformCache.end()) {
                m_strokeGlyphs = new ScalableGlyphMap;
                m_transformCache[std::make_pair(std::make_pair((int)(size * 64.0 + 0.5), (int)(stroke * 64.0 + 0.5)),
                                                transform)] = m_strokeGlyphs;
            } else {
                m_strokeGlyphs = found->second;
            }
            FT_Stroker_Set(m_stroker, (long)(m_stroke * 64.0 + 0.5), FT_STROKER_LINECAP_ROUND,
                           FT_STROKER_LINEJOIN_ROUND, 0);
        } else {
            m_strokeGlyphs = 0;
        }
    }
}

ScalableGlyph *ScalableGlyphMap::getTransformedGlyph(ScalableFont *font, unsigned short character, bool stroke)
{
    // A null receiver leads to undefined behavior.
    if (!this)
        return 0;
    int key = character;
    ScalableGlyphMap::iterator found = find(character);
    if (found != end()) {
        found->second->m_used = true;
        return found->second;
    }
    int error =
        FT_Set_Char_Size(font->m_face, (long)(64.0 * font->m_width + 0.5), (long)(font->m_height * 64.0 + 0.5), 0, 0);
    FT_Set_Transform(font->m_face, &font->m_ftTransform, 0);
    if (error || FT_Load_Char(font->m_face, character, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING))
        return 0;
    Vector<2, float> dimensions((double)font->m_face->glyph->metrics.width * 0.015625,
                                0.015625 * (double)font->m_face->glyph->metrics.height);
    Matrix<float> matrix;
    matrix.identity();
    matrix.m_elements[0] = (double)font->m_ftTransform.xx * (double)0.0000152587890625f;
    matrix.m_elements[1] = (double)-font->m_ftTransform.yx * (double)0.0000152587890625f;
    matrix.m_elements[5] = (double)font->m_ftTransform.yy * (double)0.0000152587890625f;
    matrix.m_elements[4] = (double)0.0000152587890625f * (double)-font->m_ftTransform.xy;
    Vector<2, float> x = matrix * Vector<2, float>(dimensions.m_elements[0], 0.0f);
    Vector<2, float> xy = matrix * dimensions;
    Vector<2, float> y = matrix * Vector<2, float>(0.0f, dimensions.m_elements[1]);
    Vector<2, float> minimum(0.0f, 0.0f);
    Vector<2, float> maximum(0.0f, 0.0f);
    for (int i = 0; i < 2; ++i) {
        if (x.m_elements[i] < minimum.m_elements[i])
            minimum.m_elements[i] = x.m_elements[i];
    }
    for (int i = 0; i < 2; ++i) {
        if (x.m_elements[i] > maximum.m_elements[i])
            maximum.m_elements[i] = x.m_elements[i];
    }
    for (int i = 0; i < 2; ++i) {
        if (xy.m_elements[i] < minimum.m_elements[i])
            minimum.m_elements[i] = xy.m_elements[i];
    }
    for (int i = 0; i < 2; ++i) {
        if (xy.m_elements[i] > maximum.m_elements[i])
            maximum.m_elements[i] = xy.m_elements[i];
    }
    for (int i = 0; i < 2; ++i) {
        if (y.m_elements[i] < minimum.m_elements[i])
            minimum.m_elements[i] = y.m_elements[i];
    }
    for (int i = 0; i < 2; ++i) {
        if (y.m_elements[i] > maximum.m_elements[i])
            maximum.m_elements[i] = y.m_elements[i];
    }
    dimensions = maximum - minimum;
    float area = dimensions.m_elements[1] * dimensions.m_elements[0];
    area = std::sqrt(area);
    float scale = 500.0f / area;
    if (scale >= 1.0f) {
        scale = 1.0f;
    } else {
        matrix.scale(scale, scale);
        FT_Matrix transform;
        transform.xx = (long)(matrix.m_elements[0] * 65536.0);
        transform.yx = -(long)(matrix.m_elements[1] * 65536.0);
        transform.yy = (long)(matrix.m_elements[5] * 65536.0);
        transform.xy = -(long)(65536.0 * matrix.m_elements[4]);
        FT_Set_Transform(font->m_face, &transform, 0);
        if (FT_Load_Char(font->m_face, character, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING))
            return 0;
    }
    FT_Glyph glyph;
    FT_Get_Glyph(font->m_face->glyph, &glyph);
    if (stroke) {
        FT_Stroker_Set(font->m_stroker, (long)(font->m_stroke * 64.0 * scale + 0.5), FT_STROKER_LINECAP_ROUND,
                       FT_STROKER_LINEJOIN_ROUND, 0);
        FT_Glyph_Stroke(&glyph, font->m_stroker, 1);
    }
    FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
    FT_BitmapGlyphRec_ *bitmap = (FT_BitmapGlyphRec_ *)glyph;
    FT_Bitmap pixels = bitmap->bitmap;
    int width = pixels.width + 2;
    int height = pixels.rows + 2;
    Texture *texture = 0;
    if (width > 0 && height > 0) {
        std::vector<unsigned char> rgba(4 * width * height, 0);
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                unsigned char *pixel = &rgba[4 * (y * width + x)];
                pixel[0] = 255;
                pixel[1] = 255;
                pixel[2] = 255;
                if (x <= 0 || y <= 0 || x > pixels.width || y > pixels.rows)
                    pixel[3] = 0;
                else
                    pixel[3] = pixels.buffer[(y - 1) * pixels.pitch + x - 1];
            }
        }
        TextureFormat format;
        format.horizontalWrap = TEXTURE_WRAP_NONE;
        format.verticalWrap = TEXTURE_WRAP_NONE;
        texture = font->m_engine->createTexture(width, height, &rgba[0], format, std::wstring(L"") + (wchar_t)character,
                                                true);
    }
    ScalableGlyph *result = new ScalableGlyph;
    (*this)[key] = result;
    Vector<2, float> position((float)(bitmap->left - 1), (float)(-1 - bitmap->top));
    position.m_elements[0] = position.m_elements[0] / scale;
    position.m_elements[1] = position.m_elements[1] / scale;
    result->m_position = position;
    Vector<2, float> size((float)width, (float)height);
    size.m_elements[0] = size.m_elements[0] / scale;
    size.m_elements[1] = size.m_elements[1] / scale;
    result->m_size = size;
    result->m_node = 0;
    result->m_advance = Vector<2, float>((double)font->m_face->glyph->metrics.horiAdvance * 0.015625, 0.0f);
    result->m_texture = texture;
    result->m_used = true;
    FT_Done_Glyph(glyph);
    return result;
}

ScalableGlyph *ScalableFont::getPlasmaGlyph(unsigned short character)
{
    int key = character;
    ScalableGlyphMap::iterator glyph = m_plasmaGlyphs.find(key);
    if (glyph != m_plasmaGlyphs.end()) {
        glyph->second->m_used = true;
        return glyph->second;
    }
    return 0;
}

void ScalableFont::drawGlyphs(const std::wstring &text, float x, float y, const float *fill, const float *outline,
                              const float *extrusion, int flags)
{
    float scale = 1.0f;
    if (m_plasmaRoot)
        scale = m_height / (m_maximum.m_elements[1] - m_minimum.m_elements[1]);
    Vector<2, float> minimum, maximum, alignedMinimum;
    getBounds(text, minimum, maximum, 0, -1, true);
    getBounds(text, alignedMinimum, maximum, flags, -1, true);
    alignedMinimum = alignedMinimum - minimum;
    int length = text.size();
    x = alignedMinimum.m_elements[0] + x;
    Vector<2, float> pen(x, alignedMinimum.m_elements[1] + y);
    m_engine->enableTexture2D();
    if (m_plasmaRoot) {
        m_engine->pushFillColor();
        m_engine->pushOutlineColor();
        m_engine->pushExtrusionColor();
        m_engine->setFillColor(fill);
        m_engine->setOutlineColor(outline);
        m_engine->setExtrusionColor(extrusion);
    }
    Vector<2, float> lineMinimum, lineMaximum;
    getBounds(text, lineMinimum, lineMaximum, flags, 0, true);
    if (flags & (1 << TEXT_CENTER))
        pen.m_elements[0] = x - (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] - maximum.m_elements[0] +
                                 alignedMinimum.m_elements[0]) *
                                    0.5;
    else if (flags & (1 << TEXT_RIGHT))
        pen.m_elements[0] = x - (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] - maximum.m_elements[0] +
                                 alignedMinimum.m_elements[0]);
    for (int i = 0; i < length; ++i) {
        if (text[i] == L'\n') {
            getBounds(text, lineMinimum, lineMaximum, 0, i + 1, true);
            pen.m_elements[0] = x;
            if (flags & (1 << TEXT_CENTER))
                pen.m_elements[0] = x - (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] - maximum.m_elements[0] +
                                         alignedMinimum.m_elements[0]) *
                                            0.5;
            else if (flags & (1 << TEXT_RIGHT))
                pen.m_elements[0] = x - (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] - maximum.m_elements[0] +
                                         alignedMinimum.m_elements[0]);
            pen.m_elements[1] = m_leading + m_height + pen.m_elements[1];
            continue;
        }
        ScalableGlyph *glyph = getGlyph(text[i]);
        float advance = 0.0f;
        if (glyph) {
            if (glyph->m_texture) {
                glyph->m_texture->bind();
                const Vector<4, float> &color = *reinterpret_cast<const Vector<4, float> *>(fill);
                if (m_hinted) {
                    Vector<2, int> glyphPosition(glyph->m_position);
                    Vector<2, int> transformedPosition(m_transform * pen);
                    Vector<2, float> position(transformedPosition + glyphPosition);
                    m_engine->drawTexturedRectangle(position, glyph->m_size, Vector<2, float>(0.0f, 0.0f),
                                                    Vector<2, float>(1.0f, 1.0f), color);
                } else {
                    Vector<2, int> pixelPosition(m_transform * pen + glyph->m_position);
                    Vector<2, float> position(pixelPosition);
                    m_engine->drawTexturedRectangle(position, glyph->m_size, Vector<2, float>(0.0f, 0.0f),
                                                    Vector<2, float>(1.0f, 1.0f), color);
                }
            } else if (glyph->m_node) {
                Matrix<float> matrix;
                m_engine->getModelViewMatrix(matrix);
                Matrix<float> previous = matrix;
                Vector<2, float> position = pen + glyph->m_position * scale;
                matrix.translate(position);
                matrix.scale(scale, scale);
                m_engine->loadMatrix(matrix);
                glyph->m_node->draw(0);
                m_engine->loadMatrix(previous);
            }
            advance = glyph->m_advance.m_elements[0];
        }
        if (m_stroke > 0.0f) {
            ScalableGlyph *stroke = getStrokeGlyph(text[i]);
            if (stroke && stroke->m_texture) {
                stroke->m_texture->bind();
                const Vector<4, float> &color = *reinterpret_cast<const Vector<4, float> *>(outline);
                if (m_hinted) {
                    Vector<2, int> glyphPosition(stroke->m_position);
                    Vector<2, int> transformedPosition(m_transform * pen);
                    Vector<2, float> position(transformedPosition + glyphPosition);
                    m_engine->drawTexturedRectangle(position, stroke->m_size, Vector<2, float>(0.0f, 0.0f),
                                                    Vector<2, float>(1.0f, 1.0f), color);
                } else {
                    Vector<2, int> pixelPosition(m_transform * pen + stroke->m_position);
                    Vector<2, float> position(pixelPosition);
                    m_engine->drawTexturedRectangle(position, stroke->m_size, Vector<2, float>(0.0f, 0.0f),
                                                    Vector<2, float>(1.0f, 1.0f), color);
                }
            }
        }
        if (i < length - 1) {
            if (!m_plasmaRoot) {
                FT_Vector kerning;
                if (m_hinted) {
                    FT_Get_Kerning(m_face, FT_Get_Char_Index(m_face, text[i]), FT_Get_Char_Index(m_face, text[i + 1]),
                                   FT_KERNING_DEFAULT, &kerning);
                    advance = (double)(kerning.x >> 6) + advance;
                } else {
                    FT_Get_Kerning(m_face, FT_Get_Char_Index(m_face, text[i]), FT_Get_Char_Index(m_face, text[i + 1]),
                                   FT_KERNING_UNSCALED, &kerning);
                    advance = m_width * (double)kerning.x / m_face->units_per_EM + advance;
                }
            }
            pen.m_elements[0] = m_spacing + pen.m_elements[0];
        }
        if (m_hinted) {
            pen.m_elements[0] += advance;
            pen.m_elements[0] = (float)(int)pen.m_elements[0];
        } else {
            pen.m_elements[0] = advance * scale + pen.m_elements[0];
        }
    }
    m_engine->disableTexture2D();
    if (m_plasmaRoot) {
        m_engine->popFillColor();
        m_engine->popOutlineColor();
        m_engine->popExtrusionColor();
    }
}

void ScalableFont::wrapText(std::wstring &text, float width)
{
    float advance = 0.0f;
    float lastBreakAdvance = 0.0f;
    float scale = 1.0f;
    int length = text.size();
    int lastBreak = -1;
    if (m_plasmaRoot)
        scale = m_height / (m_maximum.m_elements[1] - m_minimum.m_elements[1]);
    for (int i = 0; i < length; ++i) {
        if (text[i] == L' ' || text[i] == L'\n' || text[i] == L'\r' || i == length - 1) {
            if (advance >= width) {
                if (lastBreak >= 0)
                    text[lastBreak] = L'\n';
                advance = advance - lastBreakAdvance;
            }
            lastBreakAdvance = advance;
            lastBreak = i;
        }
        if (text[i] == L'\n' || text[i] == L'\r') {
            advance = 0.0f;
            lastBreakAdvance = 0.0f;
        } else {
            ScalableGlyph *glyph = getGlyph(text[i]);
            float glyphAdvance = 0.0f;
            if (glyph)
                glyphAdvance = glyph->m_advance.m_elements[0];
            if (i < length - 1) {
                if (!m_plasmaRoot) {
                    FT_Vector kerning;
                    if (m_hinted) {
                        FT_Get_Kerning(m_face, FT_Get_Char_Index(m_face, text[i]),
                                       FT_Get_Char_Index(m_face, text[i + 1]), FT_KERNING_DEFAULT, &kerning);
                        glyphAdvance = (double)(kerning.x >> 6) + glyphAdvance;
                    } else {
                        FT_Get_Kerning(m_face, FT_Get_Char_Index(m_face, text[i]),
                                       FT_Get_Char_Index(m_face, text[i + 1]), FT_KERNING_UNSCALED, &kerning);
                        glyphAdvance = m_width * (double)kerning.x / (double)m_face->units_per_EM + glyphAdvance;
                    }
                }
                advance = m_spacing + advance;
            }
            if (m_hinted)
                advance = (float)(int)(float)(glyphAdvance + advance);
            else
                advance = glyphAdvance * scale + advance;
        }
    }
}

void ScalableFont::calculateSize(Vector<2, float> &size, float &stroke, float &spacing, float &leading,
                                 const Matrix<float> &matrix, float requestedSize, float requestedStroke,
                                 float requestedSpacing, float requestedLeading)
{
    if (m_plasmaRoot) {
        size = Vector<2, float>(requestedSize, requestedSize);
        stroke = requestedStroke;
        spacing = requestedSpacing;
        leading = requestedLeading;
    } else {
        Vector<2, float> x = matrix.transformDirection(Vector<2, float>(1.0f, 0.0f));
        Vector<2, float> y = matrix.transformDirection(Vector<2, float>(0.0f, 1.0f));
        Vector<2, float> lengths;
        lengths[0] = std::sqrt(x[0] * x[0] + x[1] * x[1]);
        lengths[1] = std::sqrt(y[0] * y[0] + y[1] * y[1]);
        size = lengths * requestedSize;
        size[0] = (double)(int)(size[0] * 64.0 + 0.5) * 0.015625;
        size[1] = (double)0.015625f * (int)(size[1] * 64.0 + 0.5);
        if (size[0] > 1000.0f)
            size[0] = 1000.0f;
        if (size[1] > 1000.0f)
            size[1] = 1000.0f;
        Vector<2, float> scale = size / requestedSize;
        stroke = (double)0.5f * (scale[0] + scale[1]) * requestedStroke;
        spacing = scale[0] * requestedSpacing;
        leading = scale[1] * requestedLeading;
        if (size[0] < 1.0f)
            size[0] = 1.0f;
        if (size[1] < 1.0f)
            size[1] = 1.0f;
        if (g_maxFontStroke < stroke)
            stroke = g_maxFontStroke;
    }
}

void ScalableFont::prepare(const Matrix<float> &matrix, float size, float stroke, float spacing, float leading,
                           bool hinted)
{
    // CalculateSize returns the raster stroke into this copy of the size.
    float rasterSize = size;
    if (m_plasmaRoot) {
        m_hinted = false;
        m_width = rasterSize;
        m_height = rasterSize;
        m_stroke = stroke;
    } else if (hinted) {
        Vector<2, float> extent;
        float rasterSpacing, rasterLeading;
        calculateSize(extent, rasterSize, rasterSpacing, rasterLeading, matrix, rasterSize, stroke, spacing, leading);
        setSize(extent.m_elements[0], extent.m_elements[1], rasterSize);
        m_spacing = rasterSpacing;
        m_leading = rasterLeading;
        return;
    } else {
        setTransform(matrix, rasterSize, stroke);
    }
    m_spacing = spacing;
    m_leading = leading;
}

void ScalableFont::renderText(const std::wstring &text, float x, float y, float size, float stroke, float spacing,
                              float leading, float width, const float *fill, const float *outline,
                              const float *extrusion, int flags, bool hinted)
{
    lock();
    Matrix<float> previous;
    m_engine->getModelViewMatrix(previous);
    prepare(previous, size, stroke, spacing, leading, hinted);
    if (!m_plasmaRoot) {
        Matrix<float> matrix;
        if (hinted) {
            float xScale = size / m_width;
            float yScale = size / m_height;
            matrix = previous;
            width = width / xScale;
            matrix.scale(xScale, yScale);
            matrix.m_elements[12] = (float)(int)(matrix.m_elements[12] + 0.5);
            matrix.m_elements[13] = (float)(int)(matrix.m_elements[13] + 0.5);
            m_engine->loadMatrix(matrix);
        } else {
            matrix.identity();
            matrix.translate((float)(int)(previous.m_elements[12] + 0.5), (float)(int)(previous.m_elements[13] + 0.5));
            m_engine->loadMatrix(matrix);
        }
    }
    if (flags & (1 << TEXT_WRAP)) {
        std::wstring wrapped = text;
        wrapText(wrapped, width);
        drawGlyphs(wrapped, x, y, fill, outline, extrusion, flags);
    } else {
        drawGlyphs(text, x, y, fill, outline, extrusion, flags);
    }
    m_engine->loadMatrix(previous);
    unlock();
}

void ScalableFont::collectUnusedGlyphs()
{
    lock();
    std::list<ScalableSizeKey> emptySizes;
    for (ScalableSizeCache::iterator size = m_sizeCache.begin(); size != m_sizeCache.end(); ++size) {
        if (size->second) {
            std::list<unsigned short> unused;
            for (ScalableGlyphMap::iterator glyph = size->second->begin(); glyph != size->second->end(); ++glyph) {
                if (glyph->second) {
                    if (glyph->second->m_used) {
                        glyph->second->m_used = false;
                    } else {
                        if (glyph->second->m_texture)
                            m_engine->deleteTexture(glyph->second->m_texture);
                        delete glyph->second;
                        unused.push_back((unsigned short)glyph->first);
                    }
                }
            }
            for (std::list<unsigned short>::iterator character = unused.begin(); character != unused.end(); ++character)
                size->second->erase((int)*character);
            if (size->second->empty()) {
                if (m_glyphs == size->second)
                    m_glyphs = 0;
                if (m_strokeGlyphs == size->second)
                    m_strokeGlyphs = 0;
                delete size->second;
                emptySizes.push_back(size->first);
            }
        }
    }
    for (std::list<ScalableSizeKey>::iterator key = emptySizes.begin(); key != emptySizes.end(); ++key)
        m_sizeCache.erase(*key);
    std::list<ScalableTransformKey> emptyTransforms;
    for (ScalableTransformCache::iterator transform = m_transformCache.begin(); transform != m_transformCache.end();
         ++transform) {
        if (transform->second) {
            std::list<unsigned short> unused;
            for (ScalableGlyphMap::iterator glyph = transform->second->begin(); glyph != transform->second->end();
                 ++glyph) {
                if (glyph->second) {
                    if (glyph->second->m_used) {
                        glyph->second->m_used = false;
                    } else {
                        if (glyph->second->m_texture)
                            m_engine->deleteTexture(glyph->second->m_texture);
                        delete glyph->second;
                        unused.push_back((unsigned short)glyph->first);
                    }
                }
            }
            for (std::list<unsigned short>::iterator character = unused.begin(); character != unused.end(); ++character)
                transform->second->erase((int)*character);
            if (transform->second->empty()) {
                if (m_glyphs == transform->second)
                    m_glyphs = 0;
                if (m_strokeGlyphs == transform->second)
                    m_strokeGlyphs = 0;
                delete transform->second;
                emptyTransforms.push_back(transform->first);
            }
        }
    }
    for (std::list<ScalableTransformKey>::iterator key = emptyTransforms.begin(); key != emptyTransforms.end(); ++key)
        m_transformCache.erase(*key);
    unlock();
}
int ScalableFont::hitTest(const std::wstring &text, const Matrix<float> &matrix, const Vector<2, float> &point,
                          float size, float stroke, float spacing, float leading, float width, int flags)
{
    lock();
    prepare(matrix, size, stroke, spacing, leading, false);
    float xScale = size / m_width;
    float yScale = size / m_height;
    int index;
    if (flags & (1 << TEXT_WRAP)) {
        std::wstring wrapped = text;
        wrapText(wrapped, width);
        index = hitTestText(wrapped, Vector<2, float>(point) / Vector<2, float>(xScale, yScale), flags);
    } else {
        index = hitTestText(text, Vector<2, float>(point) / Vector<2, float>(xScale, yScale), flags);
    }
    unlock();
    return index;
}

int ScalableFont::hitTestText(const std::wstring &text, const Vector<2, float> &point, int flags)
{
    Vector<2, float> local = point;
    if (flags) {
        Vector<2, float> minimum, maximum;
        getBounds(text, minimum, maximum, 0, -1, false);
        if (flags & (1 << TEXT_CENTER)) {
            float center = (minimum.m_elements[0] + maximum.m_elements[0]) * (double)0.5f;
            local.m_elements[0] = center + local.m_elements[0];
        } else if (flags & (1 << TEXT_RIGHT))
            local.m_elements[0] = local.m_elements[0] + maximum.m_elements[0];
        if (flags & (1 << TEXT_MIDDLE)) {
            float center = (minimum.m_elements[1] + maximum.m_elements[1]) * (double)0.5f;
            local.m_elements[1] = center + local.m_elements[1];
        } else if (flags & (1 << TEXT_BOTTOM))
            local.m_elements[1] = local.m_elements[1] + maximum.m_elements[1];
    }
    float scale = 1.0f;
    if (m_plasmaRoot)
        scale = m_height / (m_maximum.m_elements[1] - m_minimum.m_elements[1]);
    int length = text.size();
    Vector<2, float> pen(0.0f, -m_height);
    float lastAdvance = 0.0f;
    Vector<2, float> minimum, maximum;
    getBounds(text, minimum, maximum, 0, -1, false);
    Vector<2, float> lineMinimum, lineMaximum;
    getBounds(text, lineMinimum, lineMaximum, flags, 0, false);
    if (flags & (1 << TEXT_CENTER))
        pen.m_elements[0] = 0.0 - (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] - maximum.m_elements[0] +
                                   minimum.m_elements[0]) *
                                      0.5;
    else if (flags & (1 << TEXT_RIGHT))
        pen.m_elements[0] = 0.0 - (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] - maximum.m_elements[0] +
                                   minimum.m_elements[0]);
    int result = 0;
    bool newLine = true;
    if (length < 0)
        return 0;
    for (int i = 0; i <= length; ++i) {
        if ((newLine || local.m_elements[0] > pen.m_elements[0] - lastAdvance * (double)0.3f) &&
            local.m_elements[1] >= pen.m_elements[1]) {
            result = i;
        }
        newLine = false;
        if (text[i] == L'\n' && i != length) {
            pen.m_elements[0] = 0.0f;
            newLine = true;
            Vector<2, float> lineMinimum, lineMaximum;
            getBounds(text, lineMinimum, lineMaximum, flags, i + 1, false);
            if (flags & (1 << TEXT_CENTER))
                pen.m_elements[0] = 0.0 - (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] -
                                           maximum.m_elements[0] + minimum.m_elements[0]) *
                                              0.5;
            else if (flags & (1 << TEXT_RIGHT))
                pen.m_elements[0] = 0.0 - (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] -
                                           maximum.m_elements[0] + minimum.m_elements[0]);
            pen.m_elements[1] = m_leading + m_height + pen.m_elements[1];
        } else {
            if (!text[i])
                return result;
            ScalableGlyph *glyph = getGlyph(text[i]);
            if (glyph) {
                float glyphAdvance = glyph->m_advance[0];
                if (i < length - 1) {
                    if (!m_plasmaRoot) {
                        FT_Vector kerning;
                        if (m_hinted) {
                            FT_Get_Kerning(m_face, FT_Get_Char_Index(m_face, text[i]),
                                           FT_Get_Char_Index(m_face, text[i + 1]), FT_KERNING_DEFAULT, &kerning);
                            pen.m_elements[0] = (double)(kerning.x >> 6) + pen.m_elements[0];
                        } else {
                            FT_Get_Kerning(m_face, FT_Get_Char_Index(m_face, text[i]),
                                           FT_Get_Char_Index(m_face, text[i + 1]), FT_KERNING_UNSCALED, &kerning);
                            pen.m_elements[0] =
                                m_width * (double)kerning.x / (double)m_face->units_per_EM + pen.m_elements[0];
                        }
                    }
                    pen.m_elements[0] = m_spacing + pen.m_elements[0];
                }
                lastAdvance = m_spacing + glyphAdvance * scale;
                pen[0] += glyphAdvance * scale;
                if (m_hinted)
                    pen.m_elements[0] = (float)(int)pen.m_elements[0];
            }
        }
    }
    return result;
}

void ScalableFont::getCaret(const std::wstring &text, int index, Vector<2, float> &position, Vector<2, float> &size,
                            int flags)
{
    float scale = 1.0f;
    if (m_plasmaRoot)
        scale = m_height / (m_maximum.m_elements[1] - m_minimum.m_elements[1]);
    if (index > (int)text.size())
        index = text.size();
    position = Vector<2, float>(0.0f, -m_height);
    size = Vector<2, float>(0.0f, m_height);
    Vector<2, float> minimum, maximum;
    getBounds(text, minimum, maximum, 0, -1, false);
    {
        Vector<2, float> lineMinimum, lineMaximum;
        getBounds(text, lineMinimum, lineMaximum, flags, 0, false);
        if (flags & (1 << TEXT_CENTER))
            position.m_elements[0] = position.m_elements[0] - (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] -
                                                               maximum.m_elements[0] + minimum.m_elements[0]) *
                                                                  0.5;
        else if (flags & (1 << TEXT_RIGHT))
            position.m_elements[0] = position.m_elements[0] - (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] -
                                                               maximum.m_elements[0] + minimum.m_elements[0]);
    }
    for (int i = 0; i <= index; ++i) {
        if (text[i] == L'\n' && i != index) {
            position.m_elements[0] = 0.0f;
            Vector<2, float> lineMinimum, lineMaximum;
            getBounds(text, lineMinimum, lineMaximum, flags, i + 1, false);
            if (flags & (1 << TEXT_CENTER))
                position.m_elements[0] =
                    position.m_elements[0] - (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] -
                                              maximum.m_elements[0] + minimum.m_elements[0]) *
                                                 0.5;
            else if (flags & (1 << TEXT_RIGHT))
                position.m_elements[0] =
                    position.m_elements[0] - (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] -
                                              maximum.m_elements[0] + minimum.m_elements[0]);
            position.m_elements[1] = m_leading + m_height + position.m_elements[1];
        } else {
            if (!text[i])
                break;
            ScalableGlyph *glyph = getGlyph(text[i]);
            if (glyph) {
                float advance = glyph->m_advance[0];
                if (i == index)
                    size.m_elements[0] = advance * scale;
                else
                    position.m_elements[0] = advance * scale + position.m_elements[0];
            }
            if (i < index - 1) {
                if (!m_plasmaRoot) {
                    FT_Vector kerning;
                    if (m_hinted) {
                        FT_Get_Kerning(m_face, FT_Get_Char_Index(m_face, text[i]),
                                       FT_Get_Char_Index(m_face, text[i + 1]), FT_KERNING_DEFAULT, &kerning);
                        position.m_elements[0] = (double)(kerning.x >> 6) + position.m_elements[0];
                    } else {
                        FT_Get_Kerning(m_face, FT_Get_Char_Index(m_face, text[i]),
                                       FT_Get_Char_Index(m_face, text[i + 1]), FT_KERNING_UNSCALED, &kerning);
                        position.m_elements[0] =
                            m_width * (double)kerning.x / (double)m_face->units_per_EM + position.m_elements[0];
                    }
                }
                position[0] += m_spacing;
                if (m_hinted)
                    position[0] = (float)(int)position[0];
            }
        }
    }
    if (flags) {
        Vector<2, float> minimum, maximum;
        getBounds(text, minimum, maximum, 0, -1, false);
        if (flags & (1 << TEXT_CENTER)) {
            float center = (minimum.m_elements[0] + maximum.m_elements[0]) * (double)0.5f;
            position.m_elements[0] = position.m_elements[0] - center;
        } else if (flags & (1 << TEXT_RIGHT))
            position.m_elements[0] = position.m_elements[0] - maximum.m_elements[0];
        if (flags & (1 << TEXT_MIDDLE)) {
            float center = (minimum.m_elements[1] + maximum.m_elements[1]) * (double)0.5f;
            position.m_elements[1] = position.m_elements[1] - center;
        } else if (flags & (1 << TEXT_BOTTOM))
            position.m_elements[1] = position.m_elements[1] - maximum.m_elements[1];
    }
}

void ScalableFont::getCaret(const Matrix<float> &matrix, const std::wstring &text, int index, float size, float stroke,
                            float spacing, float leading, float width, Vector<2, float> &position,
                            Vector<2, float> &extent, int flags, bool hinted)
{
    lock();
    prepare(matrix, size, stroke, spacing, leading, hinted);
    float xScale = size / m_width;
    float yScale = size / m_height;
    if (hinted)
        width = width / xScale;
    if (flags & (1 << TEXT_WRAP)) {
        std::wstring wrapped = text;
        wrapText(wrapped, width);
        getCaret(wrapped, index, position, extent, flags);
    } else {
        getCaret(text, index, position, extent, flags);
    }
    position.m_elements[0] = position.m_elements[0] * xScale;
    position.m_elements[1] = position.m_elements[1] * yScale;
    extent.m_elements[0] = xScale * extent.m_elements[0];
    extent.m_elements[1] = yScale * extent.m_elements[1];
    unlock();
}

void ScalableFont::measure(const std::wstring &text, const Matrix<float> &matrix, float size, float stroke,
                           float spacing, float leading, float width, Vector<2, float> &minimum,
                           Vector<2, float> &maximum, int flags, bool hinted)
{
    lock();
    prepare(matrix, size, stroke, spacing, leading, hinted);
    float xScale = size / m_width;
    float yScale = size / m_height;
    if (hinted)
        width = width / xScale;
    if (flags & (1 << TEXT_WRAP)) {
        std::wstring wrapped = text;
        wrapText(wrapped, width);
        getBounds(wrapped, minimum, maximum, flags, -1, false);
    } else {
        getBounds(text, minimum, maximum, flags, -1, false);
    }
    minimum.m_elements[0] = minimum.m_elements[0] * xScale;
    minimum.m_elements[1] = minimum.m_elements[1] * yScale;
    maximum.m_elements[0] = xScale * maximum.m_elements[0];
    maximum.m_elements[1] = yScale * maximum.m_elements[1];
    unlock();
}

void ScalableFont::getBounds(const std::wstring &text, Vector<2, float> &minimum, Vector<2, float> &maximum, int flags,
                             int lineStart, bool advancesOnly)
{
    int length = text.size();
    int start = 0;
    if (lineStart >= 0) {
        start = lineStart;
        for (int i = lineStart; i <= length; ++i) {
            if (text[i] == L'\n') {
                length = i;
                break;
            }
        }
    }
    minimum = Vector<2, float>(0.0f, -m_height);
    maximum = Vector<2, float>(0.0f, 0.0f);
    Vector<2, float> pen(0.0f, 0.0f);
    float scale = 1.0f;
    if (m_plasmaRoot)
        scale = m_height / (m_maximum.m_elements[1] - m_minimum.m_elements[1]);
    bool firstGlyph = true;
    if (advancesOnly)
        ++length;
    for (int i = start; i < length; ++i) {
        ScalableGlyph *glyph = getGlyph(text[i]);
        if (m_stroke > 0.0f)
            glyph = getStrokeGlyph(text[i]);
        if (glyph && !advancesOnly && text[i] != L'\n' && text[i] != L'\r') {
            if (m_plasmaRoot) {
                if (firstGlyph) {
                    minimum = pen + (glyph->m_minimum + glyph->m_position) * scale;
                    maximum = pen + (glyph->m_maximum + glyph->m_position) * scale;
                    firstGlyph = false;
                } else {
                    minimum.m_elements[0] = (std::min)(
                        minimum.m_elements[0],
                        (glyph->m_minimum.m_elements[0] + glyph->m_position.m_elements[0]) * scale + pen.m_elements[0]);
                    minimum.m_elements[1] = (std::min)(
                        minimum.m_elements[1],
                        (glyph->m_minimum.m_elements[1] + glyph->m_position.m_elements[1]) * scale + pen.m_elements[1]);
                    maximum.m_elements[0] = (std::max)(
                        maximum.m_elements[0],
                        pen.m_elements[0] + (glyph->m_maximum.m_elements[0] + glyph->m_position.m_elements[0]) * scale);
                    maximum.m_elements[1] = (std::max)(
                        maximum.m_elements[1],
                        pen.m_elements[1] + (glyph->m_maximum.m_elements[1] + glyph->m_position.m_elements[1]) * scale);
                }
            } else {
                if (firstGlyph) {
                    minimum = pen + glyph->m_position + Vector<2, float>(1.0f, 1.0f);
                    maximum = pen + glyph->m_position + glyph->m_size - Vector<2, float>(1.0f, 1.0f);
                    firstGlyph = false;
                } else {
                    minimum.m_elements[0] =
                        (std::min)(minimum.m_elements[0], pen.m_elements[0] + glyph->m_position.m_elements[0] + 1.0f);
                    minimum.m_elements[1] =
                        (std::min)(minimum.m_elements[1], pen.m_elements[1] + glyph->m_position.m_elements[1] + 1.0f);
                    maximum.m_elements[0] =
                        (std::max)(maximum.m_elements[0], pen.m_elements[0] + glyph->m_position.m_elements[0] +
                                                              glyph->m_size.m_elements[0] - 1.0f);
                    maximum.m_elements[1] =
                        (std::max)(maximum.m_elements[1], pen.m_elements[1] + glyph->m_position.m_elements[1] +
                                                              glyph->m_size.m_elements[1] - 1.0f);
                }
            }
        } else if (advancesOnly) {
            maximum.m_elements[0] = (std::max)(maximum.m_elements[0], pen.m_elements[0]);
            maximum.m_elements[1] = (std::max)(maximum.m_elements[1], pen.m_elements[1]);
        }
        if (text[i] == L'\n') {
            pen.m_elements[0] = 0.0f;
            pen.m_elements[1] = m_leading + m_height + pen.m_elements[1];
        } else {
            if (!text[i])
                break;
            if (glyph)
                pen.m_elements[0] = scale * glyph->m_advance.m_elements[0] + pen.m_elements[0];
            if (i < length - 1 && text[i + 1] != L'\n') {
                if (!m_plasmaRoot) {
                    FT_Vector kerning;
                    if (m_hinted) {
                        FT_Get_Kerning(m_face, FT_Get_Char_Index(m_face, text[i]),
                                       FT_Get_Char_Index(m_face, text[i + 1]), FT_KERNING_DEFAULT, &kerning);
                        pen.m_elements[0] = (double)(kerning.x >> 6) + pen.m_elements[0];
                    } else {
                        FT_Get_Kerning(m_face, FT_Get_Char_Index(m_face, text[i]),
                                       FT_Get_Char_Index(m_face, text[i + 1]), FT_KERNING_UNSCALED, &kerning);
                        pen.m_elements[0] =
                            m_width * (double)kerning.x / (double)m_face->units_per_EM + pen.m_elements[0];
                    }
                }
                pen.m_elements[0] = m_spacing + pen.m_elements[0];
                if (m_hinted)
                    pen.m_elements[0] = (float)(int)pen.m_elements[0];
            }
        }
    }
    if (flags) {
        Vector<2, float> totalMinimum, totalMaximum;
        getBounds(text, totalMinimum, totalMaximum, 0, -1, advancesOnly);
        float xOffset = 0.0f;
        if (flags & (1 << TEXT_CENTER))
            xOffset = (totalMaximum.m_elements[0] + totalMinimum.m_elements[0]) * 0.5f;
        else if (flags & (1 << TEXT_RIGHT))
            xOffset = totalMaximum.m_elements[0];
        if (flags & TEXT_HORIZONTAL_MASK) {
            minimum.m_elements[0] = minimum.m_elements[0] - xOffset;
            maximum.m_elements[0] = maximum.m_elements[0] - xOffset;
        }
        float yOffset = 0.0f;
        if (flags & (1 << TEXT_MIDDLE))
            yOffset = 0.5f * (totalMaximum.m_elements[1] + totalMinimum.m_elements[1]);
        else if (flags & (1 << TEXT_BOTTOM))
            yOffset = totalMaximum.m_elements[1];
        if (flags & TEXT_VERTICAL_MASK) {
            minimum.m_elements[1] = minimum.m_elements[1] - yOffset;
            maximum.m_elements[1] = maximum.m_elements[1] - yOffset;
        }
    }
}

// Multiplies the colours of every mesh shape below the node by the fill, outline and extrusion colours (the font's
// glyph nodes, ConvertGlyphToNode).
void multiplyFontNodeColors(Node *node, const float *fill, const float *outline, const float *extrusion)
{
    Shape *shape = node->m_shape;
    if (shape && shape->getType() == SHAPE_SMOOTH_MESH) {
        MeshShape *mesh = static_cast<MeshShape *>(shape);

        // Each loop runs over the first frame's length.
        std::vector<Vector<4, float>> &fills = *mesh->m_vertexColors.m_values[0];
        for (int i = 0; i < (int)fills.size(); ++i) {
            Vector<4, float> &color = mesh->m_vertexColors.value()[i];
            for (int c = 0; c < 4; ++c)
                color[c] *= fill[c];
        }
        std::vector<Vector<4, float>> &outlines = *mesh->m_strokeColors.m_values[0];
        for (int i = 0; i < (int)outlines.size(); ++i) {
            Vector<4, float> &color = mesh->m_strokeColors.value()[i];
            for (int c = 0; c < 4; ++c)
                color[c] *= outline[c];
        }
        std::vector<Vector<4, float>> &extrusions = *mesh->m_extrusionFrontColors.m_values[0];
        for (int i = 0; i < (int)extrusions.size(); ++i) {
            Vector<4, float> &front = mesh->m_extrusionFrontColors.value()[i];
            for (int c = 0; c < 4; ++c)
                front[c] *= extrusion[c];
            Vector<4, float> &back = mesh->m_extrusionBackColors.value()[i];
            for (int c = 0; c < 4; ++c)
                back[c] *= extrusion[c];
        }
        shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    }
    for (std::list<Node *>::iterator child = node->m_children.begin(); child != node->m_children.end(); ++child)
        multiplyFontNodeColors(*child, fill, outline, extrusion);
}

// A node for one character: a Plasma font's glyph node (cloned, or instanced from the cache), or a smooth mesh
// built from the TrueType outline.
Node *ScalableFont::convertGlyphToNode(unsigned short character, float size, float stroke, const float *fill,
                                       const float *outline, const float *extrusion,
                                       std::map<unsigned short, Node *> *cache)
{
    if (character == L' ' || character == L'\n' || character == L'\t' || character == L'\r')
        return 0;
    if (m_plasmaRoot) {
        ScalableGlyph *glyph = getGlyph(character);
        if (!glyph || !glyph->m_node)
            return 0;
        if (cache && cache->find(character) != cache->end()) {
            Node *node = (*cache)[character]->instance(0);
            node->setTransformation(glyph->m_node->m_transformation->clone());
            float scale = size / (m_maximum.m_elements[1] - m_minimum.m_elements[1]);
            node->m_transformation->m_translation.value() += glyph->m_position;
            node->m_transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
            node->m_transformation->scaleAbout(Vector<2, float>(0.0f, 0.0f), Vector<2, float>(scale, scale));
            node->m_transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
            return node;
        }
        Node *node = glyph->m_node->clone(0);
        float scale = size / (m_maximum.m_elements[1] - m_minimum.m_elements[1]);
        node->m_transformation->m_translation.value() += glyph->m_position;
        node->m_transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
        node->m_transformation->scaleAbout(Vector<2, float>(0.0f, 0.0f), Vector<2, float>(scale, scale));
        node->m_transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
        multiplyFontNodeColors(node, fill, outline, extrusion);
        if (cache)
            (*cache)[character] = node;
        return node;
    }
    if (cache && cache->find(character) != cache->end()) {
        Node *node = (*cache)[character]->instance(0);
        node->setTransformation(node->m_transformation->clone());
        node->m_transformation->m_translation.value() = Vector<2, float>(0.0f, 0.0f);
        node->m_transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
        return node;
    }
    FT_Set_Char_Size(m_face, (long)(size * 64.0 + 0.5), 0, 0, 0);
    FT_Load_Char(m_face, character, FT_LOAD_NO_BITMAP | FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING);
    wchar_t name[2] = {character, 0};
    SmoothMeshShape *shape = m_engine->createSmoothMeshShape(std::wstring(name));
    Node *node = m_engine->createNode(0, shape, 0, 0, std::wstring(name));

    // The fill, outline and extrusion arguments are RGBA colours.
    const Vector<4, float> &fillColor = *reinterpret_cast<const Vector<4, float> *>(fill);
    const Vector<4, float> &outlineColor = *reinterpret_cast<const Vector<4, float> *>(outline);
    const Vector<4, float> &extrusionColor = *reinterpret_cast<const Vector<4, float> *>(extrusion);
    std::vector<std::vector<int>> faces;
    int start = 0;
    for (int contour = 0; contour < m_face->glyph->outline.n_contours; ++contour) {
        Vector<2, float> previousDelta(0.0f, 0.0f);
        int last = m_face->glyph->outline.contours[contour];
        float x = (float)m_face->glyph->outline.points[last].x;
        float y = -(float)m_face->glyph->outline.points[last].y;
        Vector<2, float> previous(x * 0.015625, y * 0.015625);
        bool previousOffCurve = !(m_face->glyph->outline.tags[last] & 1);
        std::vector<int> vertices;
        for (int point = start; point <= m_face->glyph->outline.contours[contour]; ++point) {
            float x = (float)m_face->glyph->outline.points[point].x;
            float y = -(float)m_face->glyph->outline.points[point].y;
            Vector<2, float> position(x * 0.015625, y * 0.015625);
            Vector<2, float> delta = position - previous;
            float distanceSquared =
                delta.m_elements[0] * delta.m_elements[0] + delta.m_elements[1] * delta.m_elements[1];
            if (distanceSquared < 0.001f)
                continue;
            if (m_face->glyph->outline.tags[point] & 2)
                std::cout << "FT_CURVE_TAG_CUBIC" << std::endl;
            if (previousDelta.m_elements[1] * delta.m_elements[0] - delta.m_elements[1] * previousDelta.m_elements[0] ==
                0.0f) {
                float dot = delta.m_elements[0] * previousDelta.m_elements[0];
                dot += delta.m_elements[1] * previousDelta.m_elements[1];
                if (dot < 0.0f) {
                    // A reversal on a straight line drops the last vertex.
                    shape->m_vertexPositions.erase(shape->m_vertexPositions.m_values[0]->size() - 1);
                    shape->m_vertexTexCoords.erase(shape->m_vertexPositions.m_values[0]->size());
                    shape->m_vertexColors.erase(shape->m_vertexPositions.m_values[0]->size());
                    shape->m_strokeColors.erase(shape->m_vertexPositions.m_values[0]->size());
                    shape->m_extrusionFrontColors.erase(shape->m_vertexPositions.m_values[0]->size());
                    shape->m_extrusionBackColors.erase(shape->m_vertexPositions.m_values[0]->size());
                    shape->m_strokeWidths.erase(shape->m_vertexPositions.m_values[0]->size());
                    shape->m_vertexFlags.pop_back();
                    vertices.pop_back();
                }
            }
            previousDelta = delta;
            if (previousOffCurve && !(m_face->glyph->outline.tags[point] & 1)) {
                // Two off-curve points: the implied on-curve midpoint.
                vertices.push_back(shape->m_vertexPositions.m_values[0]->size());
                shape->m_vertexPositions.append((position + previous) * 0.5f);
                shape->m_vertexColors.append(fillColor);
                shape->m_vertexTexCoords.append(Vector<2, float>(0.0f, 0.0f));
                shape->m_strokeColors.append(outlineColor);
                shape->m_extrusionFrontColors.append(extrusionColor);
                shape->m_extrusionBackColors.append(extrusionColor);
                shape->m_strokeWidths.append(stroke + stroke);
                shape->m_vertexFlags.push_back(VERTEX_SHARP);
            }
            vertices.push_back(shape->m_vertexPositions.m_values[0]->size());
            shape->m_vertexPositions.append(position);
            shape->m_vertexColors.append(fillColor);
            shape->m_vertexTexCoords.append(Vector<2, float>(0.0f, 0.0f));
            shape->m_strokeColors.append(outlineColor);
            shape->m_extrusionFrontColors.append(extrusionColor);
            shape->m_extrusionBackColors.append(extrusionColor);
            shape->m_strokeWidths.append(stroke + stroke);
            shape->m_vertexFlags.push_back((m_face->glyph->outline.tags[point] & 1) ? VERTEX_SHARP : VERTEX_BEZIER);
            previous = position;
            previousOffCurve = !(m_face->glyph->outline.tags[point] & 1);
        }
        faces.push_back(vertices);
        start = m_face->glyph->outline.contours[contour] + 1;
    }
    shape->m_flags = (shape->m_flags & ~((1 << MESH_NO_FILL) | (1 << MESH_OPEN))) | (1 << MESH_CONTOUR);
    if (stroke > 0.0f)
        shape->m_flags |= (1 << MESH_STROKE);
    else
        shape->m_flags &= ~(1 << MESH_STROKE);
    shape->setFaces(faces, true);
    if (cache)
        (*cache)[character] = node;
    return node;
}

void ScalableFont::convertTextToNodes(Node *parent, const std::wstring &text, float size, float stroke, float spacing,
                                      float leading, const float *fill, const float *outline, const float *extrusion,
                                      int flags)
{
    lock();
    float scale = 1.0f;
    if (m_plasmaRoot)
        scale = size / (m_maximum.m_elements[1] - m_minimum.m_elements[1]);
    Matrix<float> identity;
    identity.identity();
    prepare(identity, size, stroke, spacing, leading, false);
    Vector<2, float> minimum, maximum, alignedMinimum;
    getBounds(text, minimum, maximum, 0, -1, false);
    getBounds(text, alignedMinimum, maximum, flags, -1, false);
    alignedMinimum = alignedMinimum - minimum;
    Vector<2, float> pen = alignedMinimum;
    int length = text.size();
    Vector<2, float> lineMinimum, lineMaximum;
    getBounds(text, lineMinimum, lineMaximum, flags, 0, false);
    if (flags & (1 << TEXT_CENTER))
        pen.m_elements[0] = alignedMinimum.m_elements[0] -
                            (alignedMinimum.m_elements[0] +
                             (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] - maximum.m_elements[0])) *
                                0.5;
    else if (flags & (1 << TEXT_RIGHT))
        pen.m_elements[0] = alignedMinimum.m_elements[0] -
                            (alignedMinimum.m_elements[0] +
                             (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] - maximum.m_elements[0]));
    std::map<unsigned short, Node *> cache;
    for (int i = 0; i < length; ++i) {
        if (text[i] == L'\n') {
            getBounds(text, lineMinimum, lineMaximum, 0, i + 1, false);
            pen.m_elements[0] = alignedMinimum.m_elements[0];
            if (flags & (1 << TEXT_CENTER))
                pen.m_elements[0] = alignedMinimum.m_elements[0] -
                                    (alignedMinimum.m_elements[0] +
                                     (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] - maximum.m_elements[0])) *
                                        0.5;
            else if (flags & (1 << TEXT_RIGHT))
                pen.m_elements[0] = alignedMinimum.m_elements[0] -
                                    (alignedMinimum.m_elements[0] +
                                     (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] - maximum.m_elements[0]));
            pen.m_elements[1] = m_leading + m_height + pen.m_elements[1];
        } else {
            Node *node = convertGlyphToNode(text[i], size, stroke, fill, outline, extrusion, &cache);
            if (node) {
                node->setParent(parent);
                Vector<2, float> &translation = node->m_transformation->m_translation.value();
                translation.m_elements[0] = pen.m_elements[0] + translation.m_elements[0];
                translation.m_elements[1] = translation.m_elements[1] + pen.m_elements[1];
                node->m_transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
            }
            ScalableGlyph *glyph = getGlyph(text[i]);
            if (glyph)
                pen.m_elements[0] = glyph->m_advance.m_elements[0] * scale + m_spacing + pen.m_elements[0];
        }
    }
    unlock();
}

}
