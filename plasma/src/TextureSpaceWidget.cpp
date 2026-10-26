#include "TextureSpaceWidget.h"
#include <cmath>
#include <GL/glew.h>

using plasma::Vector;

static const int AnnulusSegments = 50;

TextureSpaceWidget::TextureSpaceWidget(plasma::Engine *engine, TextureSheet *sheet, plasma::Node *node)
    : plasma::Widget(engine, node, L"texturespace"),
      m_sheet(sheet)
{}

TextureSpaceWidget::~TextureSpaceWidget() {}

Vector<2, float> TextureSpaceWidget::getMinimum()
{
    return transformPoint(m_node->m_inverseMatrix, Vector<2, float>(0.0f, 0.0f));
}

bool TextureSpaceWidget::hasBounds()
{
    return true;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void drawAnnulus(const Vector<2, float> &center, float innerRadius, float outerRadius, const Vector<4, float> &outline,
                 const Vector<4, float> &fill)
{
    glColor4fv(fill.m_elements);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= AnnulusSegments; ++i) {
        float angle = (double)i * 6.283185307179586 / (double)AnnulusSegments;
        float x = cos(angle);
        float y = sin(angle);
        Vector<2, float> direction(x, y);
        Vector<2, float> inner = center + direction * innerRadius;
        glVertex2fv(inner.m_elements);
        Vector<2, float> outer = center + Vector<2, float>(x, y) * outerRadius;
        glVertex2fv(outer.m_elements);
    }
    glEnd();
    glColor4fv(outline.m_elements);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < AnnulusSegments; ++i) {
        float angle = (double)i * 6.283185307179586 / (double)AnnulusSegments;
        Vector<2, float> direction(cos(angle), sin(angle));
        Vector<2, float> point = center + direction * innerRadius;
        glVertex2fv(point.m_elements);
    }
    glEnd();
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < AnnulusSegments; ++i) {
        float angle = (double)i * 6.283185307179586 / (double)AnnulusSegments;
        Vector<2, float> direction(cos(angle), sin(angle));
        Vector<2, float> point = center + direction * outerRadius;
        glVertex2fv(point.m_elements);
    }
    glEnd();
}
