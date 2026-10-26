// Text shape geometry and font lookup. attributesChanged looks up the font; shape locking and font-engine locking are
// inactive.
#include "plasma/TextShape.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
#include "plasma/Font.h"
#include "plasma/EditFilter.h"
#include "plasma/SegmentLength.h"

namespace plasma {

TextShape::TextShape(Engine *engine, const std::wstring &text, const std::wstring &name)
    : Shape(engine, name),
      m_string("string", text),
      m_color("colors", Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f)),
      m_strokeColor("strokeColors", Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f)),
      m_extrusionColor("extrusionColors", Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f)),
      m_pixelSize(0.0f),
      m_strokeRadius(0.0f),
      m_spacing(0.0f),
      m_lineSpacing(0.0f),
      m_wrapWidth(0.0f),
      m_font(nullptr),
      m_flags(0),
      m_hintingFlags(0)
{
    m_type = SHAPE_TEXT;
    addAttribute(&m_string);
    addAttribute(&m_color);
    addAttribute(&m_strokeColor);
    addAttribute(&m_extrusionColor);
}

TextShape::~TextShape() {}

Vector<2, float> TextShape::getMaximum()
{
    return m_maximum;
}

Vector<2, float> TextShape::getMinimum()
{
    return m_minimum;
}

Shape *TextShape::clone()
{
    TextShape *shape = m_engine->createTextShape(m_string.value(), m_name);
    shape->m_string = m_string;
    shape->m_color = m_color;
    shape->m_strokeColor = m_strokeColor;
    shape->m_extrusionColor = m_extrusionColor;
    shape->m_fontName = m_fontName;
    shape->m_pixelSize = m_pixelSize;
    shape->m_strokeRadius = m_strokeRadius;
    shape->m_spacing = m_spacing;
    shape->m_lineSpacing = m_lineSpacing;
    shape->m_wrapWidth = m_wrapWidth;
    shape->m_flags = m_flags;
    shape->m_hintingFlags = m_hintingFlags;
    shape->attributesChanged(true);
    return shape;
}

void TextShape::draw(Node *node)
{
    if (m_font) {
        m_engine->getFontEngine()->lock();
        m_engine->beginTextDrawing(node);
        m_font->renderText(m_string.value(), 0.0f, 0.0f, m_pixelSize, m_strokeRadius, m_spacing, m_lineSpacing,
                           m_wrapWidth, m_color.value().m_elements, m_strokeColor.value().m_elements,
                           m_extrusionColor.value().m_elements, m_flags, m_hintingFlags & (1 << TEXT_HINTED));
        m_engine->endTextDrawing(node);
        m_engine->getFontEngine()->unlock();
    }
}

void TextShape::setText(const std::wstring &text)
{
    m_string.value() = text;
}

bool TextShape::contains(const Vector<2, float> &point)
{
    return point.m_elements[0] >= m_minimum.m_elements[0] && point.m_elements[1] >= m_minimum.m_elements[1] &&
           point.m_elements[0] <= m_maximum.m_elements[0] && point.m_elements[1] <= m_maximum.m_elements[1];
}

// The squared distance from point to the segment first..second.
static float squaredDistanceToSegment(const Vector<2, float> &point, const Vector<2, float> &first,
                                      const Vector<2, float> &second);

bool TextShape::intersects(const Vector<2, float> &point, float radius, const Matrix<float> &matrix,
                           const Matrix<float> &inverse)
{
    // Inside the bounds in the shape's own space, or within radius of the transformed bounds' outline.
    if (contains(inverse * point))
        return true;
    // Transforms each bounds corner with operator*.
    Vector<2, float> corners[4] = {matrix * Vector<2, float>(m_minimum.m_elements[0], m_minimum.m_elements[1]),
                                   matrix * Vector<2, float>(m_maximum.m_elements[0], m_minimum.m_elements[1]),
                                   matrix * Vector<2, float>(m_maximum.m_elements[0], m_maximum.m_elements[1]),
                                   matrix * Vector<2, float>(m_minimum.m_elements[0], m_maximum.m_elements[1])};
    float squared = radius * radius;
    for (int i = 0; i < 4; ++i) {
        if (squaredDistanceToSegment(point, corners[i], corners[(i + 1) % 4]) <= squared)
            return true;
    }
    return false;
}

static float squaredDistanceToSegment(const Vector<2, float> &point, const Vector<2, float> &first,
                                      const Vector<2, float> &second)
{
    Vector<2, float> relative = point - first;
    Vector<2, float> direction = second - first;
    float length =
        direction.m_elements[0] * direction.m_elements[0] + direction.m_elements[1] * direction.m_elements[1];
    if (!(length < MinimumSegmentLengthSquared)) {
        float projection =
            (relative.m_elements[0] * direction.m_elements[0] + relative.m_elements[1] * direction.m_elements[1]) /
            length;
        if (!(projection <= 0.0f)) {
            if (projection >= 1.0f) {
                Vector<2, float> end = point - second;
                return end.m_elements[0] * end.m_elements[0] + end.m_elements[1] * end.m_elements[1];
            }
            relative = relative - direction * projection;
        }
    }
    return relative.m_elements[0] * relative.m_elements[0] + relative.m_elements[1] * relative.m_elements[1];
}

void TextShape::attributesChanged(bool all)
{
    m_changed = true;
    if (!m_font || all) {
        m_font = m_engine->getFontEngine()->getFont(m_fontName);
        if (!m_font)
            m_font = m_engine->getFontEngine()->getFont(L"arial.ttf");
    }
}

void TextShape::updateBounds(const Matrix<float> &matrix)
{
    if (m_font && (m_hintingFlags & (1 << TEXT_HINTED))) {
        m_engine->getFontEngine()->lock();
        m_font->measure(m_string.value(), matrix, m_pixelSize, m_strokeRadius, m_spacing, m_lineSpacing, m_wrapWidth,
                        m_minimum, m_maximum, m_flags, m_hintingFlags & (1 << TEXT_HINTED), -1, false);
        m_engine->getFontEngine()->unlock();
    }
}

}
