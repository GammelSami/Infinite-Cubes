#include "TexturePreviewWidget.h"
#include "TextureModule.h"
#include "ColorModule.h"
#include <GL/glew.h>

TexturePreviewWidget::TexturePreviewWidget(plasma::Engine *engine, Workspace *workspace, plasma::Node *node)
    : plasma::Button(engine, node, plasma::Button::Toggle, L"TextureWidget"),
      m_workspace(workspace)
{}

void TexturePreviewWidget::pointerPressed()
{
    plasma::Button::pointerPressed();
    if (m_checked)
        m_workspace->m_textureModule->showSelection();
}

void TexturePreviewWidget::pointerReleased()
{
    plasma::Button::pointerReleased();
}

void TexturePreviewWidget::pointerDoubleClicked()
{
    plasma::Button::pointerDoubleClicked();
    if (m_checked)
        m_workspace->m_textureModule->showSelection();
}

bool TexturePreviewWidget::contains(const plasma::Vector<2, float> &point)
{
    double x;
    double y;

    // Snapshot each coordinate before its dimension call can update layout.
    return point[0] >= getFramePos()[0] && point[1] >= getFramePos()[1] &&
           ((x = point.m_elements[0]), getContentWidth() > x) && ((y = point.m_elements[1]), getContentHeight() > y);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void TexturePreviewWidget::draw()
{
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glDisable(GL_DEPTH_TEST);
    plasma::Texture *texture = m_workspace->m_textureModule->m_texture;
    if (texture) {
        texture->bind();
        glEnable(GL_TEXTURE_2D);
        m_workspace->m_engine.setShapeShader(m_node, true, 1.0f, 1.0f, 1.0f, 1.0f);
        plasma::Vector<4, float> material(1.0f, 1.0f, 1.0f, m_workspace->m_textureModule->m_opacity);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, material.m_elements);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(getContentWidth() * (double)0.5f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(getContentWidth() * (double)0.5f, getContentHeight());
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(0.0f, getContentHeight());
    glColor4fv(m_workspace->m_colorModule->getColor().m_elements);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(getContentWidth() * (double)0.5f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(getContentWidth(), 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(getContentWidth(), getContentHeight());
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(getContentWidth() * (double)0.5f, getContentHeight());
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glColor4f(0.0f, 0.0f, 0.0f, 0.1f);
    float lineWidth;
    glGetFloatv(GL_LINE_WIDTH, &lineWidth);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, getContentHeight());
    glVertex2f(getContentWidth(), getContentHeight());
    glEnd();
}
