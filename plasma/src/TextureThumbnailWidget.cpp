#include "TextureModule.h"
#include "Workspace.h"
#include "plasma/Engine.h"
#include <GL/glew.h>

TextureThumbnailWidget::TextureThumbnailWidget(plasma::Engine *engine, Workspace *workspace, plasma::Node *node,
                                               plasma::Texture *texture)
    : plasma::Widget(engine, node, std::wstring(L"texture:thumbnail")),
      m_workspace(workspace),
      m_texture(texture)
{}

void TextureThumbnailWidget::pointerReleased()
{
    m_workspace->getTextureModule()->selectTexture(m_texture);
    m_workspace->getTextureModule()->applyAutomatic();
    m_workspace->getTextureModule()->hideSelection();
}

void TextureThumbnailWidget::draw()
{
    if (!m_texture)
        return;
    glDisable(GL_DEPTH_TEST);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    m_texture->bind();
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();
    int height = m_texture->m_height;
    int width = m_texture->m_width;
    glScalef(viewWidth() / getViewHeight() * height / width, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(getViewSize()[0], 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(getViewSize()[0], getViewSize()[1]);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(0.0f, getViewSize()[1]);
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
