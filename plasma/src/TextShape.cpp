#include "plasma/TextShape.h"
#include "plasma/Widget.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
#include "HalfEdgeMesh.h"

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
      m_font(0),
      m_flags(0),
      m_hintingFlags(0)
{
    m_type = SHAPE_TEXT;
    addAttribute(&m_string);
    addAttribute(&m_color);
    addAttribute(&m_strokeColor);
    addAttribute(&m_extrusionColor);
    InitializeCriticalSectionAndSpinCount(&m_lock, 0x80000400);
}

TextShape::~TextShape()
{
    DeleteCriticalSection(&m_lock);
}

void TextShape::draw(Node *node)
{
    lock();
    if (m_font) {
        m_engine->getFontEngine()->lock();
        m_engine->beginTextDrawing(node);
        m_font->renderText(m_string.value(), 0.0f, 0.0f, m_pixelSize, m_strokeRadius, m_spacing, m_lineSpacing,
                           m_wrapWidth, m_color.value().m_elements, m_strokeColor.value().m_elements,
                           m_extrusionColor.value().m_elements, m_flags, m_hintingFlags & (1 << TEXT_HINTED));
        m_engine->endTextDrawing(node);
        m_engine->getFontEngine()->unlock();
    }
    unlock();
}

void TextShape::setText(const std::wstring &text)
{
    m_string.value().assign(text);
}

void TextShape::updateBounds(const Matrix<float> &matrix)
{
    lock();
    if (m_font && (m_hintingFlags & (1 << TEXT_HINTED))) {
        m_engine->getFontEngine()->lock();
        m_font->measure(m_string.value(), matrix, m_pixelSize, m_strokeRadius, m_spacing, m_lineSpacing, m_wrapWidth,
                        m_minimum, m_maximum, m_flags, m_hintingFlags & (1 << TEXT_HINTED));
        m_engine->getFontEngine()->unlock();
    }
    unlock();
}

void TextShape::attributesChanged(int reason)
{
    lock();
    m_changed = true;
    m_font = m_engine->getFontEngine()->getFont(m_fontName);
    if (!m_font)
        m_font = m_engine->getFontEngine()->getFont(L"arial.ttf");
    if (!m_font) {
        m_minimum = Vector<2, float>(0.0f, 0.0f);
        m_maximum = Vector<2, float>(0.0f, 0.0f);

        // The code returns with the critical section held when font lookup fails.
    } else {
        if (!(m_hintingFlags & (1 << TEXT_HINTED))) {
            Matrix<float> matrix;
            matrix.identity();
            m_engine->getFontEngine()->lock();
            m_font->measure(m_string.value(), matrix, m_pixelSize, m_strokeRadius, m_spacing, m_lineSpacing,
                            m_wrapWidth, m_minimum, m_maximum, m_flags, m_hintingFlags & (1 << TEXT_HINTED));
            m_engine->getFontEngine()->unlock();
        }
        unlock();
    }
}

Vector<2, float> TextShape::getMinimum()
{
    return m_minimum;
}

Vector<2, float> TextShape::getMaximum()
{
    return m_maximum;
}

Shape *TextShape::clone()
{
    TextShape *shape = m_engine->createTextShape(m_string.value(), m_name);
    shape->m_string = m_string;
    shape->m_color = m_color;
    shape->m_strokeColor = m_strokeColor;
    shape->m_extrusionColor = m_extrusionColor;
    shape->m_fontName.assign(m_fontName);
    shape->m_pixelSize = m_pixelSize;
    shape->m_strokeRadius = m_strokeRadius;
    shape->m_spacing = m_spacing;
    shape->m_lineSpacing = m_lineSpacing;
    shape->m_wrapWidth = m_wrapWidth;
    shape->m_flags = m_flags;
    shape->m_hintingFlags = m_hintingFlags;
    shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return shape;
}

bool TextShape::contains(const Vector<2, float> &point)
{
    return point.m_elements[0] >= m_minimum.m_elements[0] && point.m_elements[1] >= m_minimum.m_elements[1] &&
           point.m_elements[0] <= m_maximum.m_elements[0] && point.m_elements[1] <= m_maximum.m_elements[1];
}

bool TextShape::intersects(const Vector<2, float> &point, float radius, const Matrix<float> &matrix,
                           const Matrix<float> &inverse)
{
    Vector<2, float> local = inverse.transformPoint(point);
    if (!contains(local)) {
        Vector<2, float> corners[4];
        corners[0] = matrix.transformPoint(Vector<2, float>(m_minimum.m_elements[0], m_minimum.m_elements[1]));
        corners[1] = matrix.transformPoint(Vector<2, float>(m_maximum.m_elements[0], m_minimum.m_elements[1]));
        corners[2] = matrix.transformPoint(Vector<2, float>(m_maximum.m_elements[0], m_maximum.m_elements[1]));
        corners[3] = matrix.transformPoint(Vector<2, float>(m_minimum.m_elements[0], m_maximum.m_elements[1]));
        float squared = radius * radius;
        for (int i = 0; i < 4; ++i) {
            if (squaredDistanceToSegment(point, corners[i], corners[(i + 1) % 4]) <= squared)
                return true;
        }
        return false;
    }
    return true;
}

void TextShape::lock()
{
    EnterCriticalSection(&m_lock);
}

void TextShape::unlock()
{
    LeaveCriticalSection(&m_lock);
}

}
