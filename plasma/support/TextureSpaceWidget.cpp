// Texture-space view support for selected meshes and overlays.
#include "plasma/GenericShape.h"
#include "plasma/Drawing.h"
#include "plasma/Transformation.h"
#include "TexturePreviewWidget.h"
#include "TextureSpaceWidget.h"
#include "TextureModule.h"
#include "DesignSheet.h"
#include "FormatModule.h"
#include "ShapeInterface.h"
#include <GL/glew.h>

using namespace plasma;

static Texture *g_previousTexture;

Vector<2, float> TextureSpaceWidget::getMaximum()
{
    Vector<2, float> viewport((float)m_engine->viewportWidth(), (float)m_engine->viewportHeight());
    return transformPoint(m_node->m_inverseMatrix, viewport);
}

void TextureSpaceWidget::draw()
{
    ShapeInterface *shape = 0;
    if (m_sheet->m_workspace->getOpenDesignSheet() && m_sheet->m_workspace->getOpenDesignSheet()->m_currentNode) {
        shape = m_sheet->m_workspace->getOpenDesignSheet()
                    ->findSheetNode(m_sheet->m_workspace->getOpenDesignSheet()->m_currentNode)
                    ->m_shapeInterface;
        if (!shape || shape->m_type != INTERFACE_MESH)
            shape = 0;
    }
    glEnable(GL_LINE_SMOOTH);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    static_cast<GLEngine *>(m_engine)->useFixedFunction();
    float width = m_sheet->m_panel->viewWidth();
    float height = m_sheet->m_panel->getViewHeight();
    glPushAttrib(GL_SCISSOR_BIT);
    glEnable(GL_SCISSOR_TEST);
    int windowHeight = m_engine->viewportHeight();
    // Reads both coordinates from the sheet's panel: y first, then x.
    Vector<2, float> position = m_sheet->m_panel->getContentPosition();
    Vector<2, float> positionX = m_sheet->m_panel->getContentPosition();
    glScissor((int)positionX.m_elements[0], windowHeight - (int)(height + position.m_elements[1]), (int)width,
              (int)height);
    glDisable(GL_TEXTURE_2D);
    if (shape) {
        if (g_previousTexture != shape->getTexture()) {
            Texture *texture = shape->getTexture();
            g_previousTexture = texture;
            if (texture) {
                m_sheet->m_page.size.m_elements[0] = (float)texture->getWidth();
                m_sheet->m_page.size.m_elements[1] = (float)texture->getHeight();
            } else {
                m_sheet->m_page.size.m_elements[0] = 100.0f;
                m_sheet->m_page.size.m_elements[1] = 100.0f;
            }
            m_sheet->m_workspace->m_formatModule->update();
        }
        glMatrixMode(GL_TEXTURE);
        glPushMatrix();
        if (shape->getTexture()) {
            glEnable(GL_TEXTURE_2D);
            shape->getTexture()->bind();
        } else {
            glDisable(GL_TEXTURE_2D);
        }
        glLoadIdentity();
        glScalef(1.0 / m_sheet->m_page.size.m_elements[0], 1.0 / m_sheet->m_page.size.m_elements[1], 1.0f);
        glMultMatrixf(m_sheet->m_root->m_transformation->getInverseMatrix().m_elements);
        glEnable(GL_BLEND);
        glBegin(GL_QUADS);
        glColor3f(1.0f, 1.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glTexCoord2f(width, 0.0f);
        glVertex2f(width, 0.0f);
        glTexCoord2f(width, height);
        glVertex2f(width, height);
        glTexCoord2f(0.0f, height);
        glVertex2f(0.0f, height);
        glEnd();
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_BLEND);
        m_sheet->drawGrid();
        m_sheet->drawPageBorders();
        glPushMatrix();
        glMultMatrixf(m_sheet->m_root->m_transformation->getMatrix().m_elements);
        glScalef(m_sheet->m_page.size.m_elements[0], m_sheet->m_page.size.m_elements[1], 1.0f);
        glDisable(GL_TEXTURE_2D);

        // The selected meshes in texture space.
        plasma::Matrix<float> matrix = m_sheet->getMatrix();
        if (m_sheet->m_workspace->getOpenDesignSheet()) {
            for (std::map<Node *, NodeInterface *>::iterator it =
                     m_sheet->m_workspace->getOpenDesignSheet()->sheetNodesBegin();
                 it != m_sheet->m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
                if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                    it->second->m_shapeInterface->setMatrix(matrix);
                    it->second->m_shapeInterface->draw();
                    it->second->m_shapeInterface->resetMatrix();
                }
            }
        }
        glPopMatrix();
        if (!m_sheet->m_workspace->isPanning()) {
            glEnable(GL_LINE_SMOOTH);
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glMultMatrixf(m_node->m_inverseMatrix.m_elements);
            if (!m_sheet->m_workspace->m_guiPointerNode) {
                glLineWidth(1.0f);
                drawAnnulus(m_sheet->m_workspace->m_sceneMousePosition,
                            m_sheet->m_workspace->m_toolRadius - (double)1.0f,
                            m_sheet->m_workspace->m_toolRadius + (double)2.0f, Vector<4, float>(0.0f, 0.0f, 0.0f, 0.6f),
                            Vector<4, float>(1.0f, 1.0f, 1.0f, 0.6f));
            }
            TextureSheet *sheet = m_sheet;
            if (sheet == sheet->m_workspace->getTextureSheet())
                sheet->m_workspace->drawConstraintIcons();
            glPopMatrix();
        }
    }
    m_sheet->renderOverlays();
    glPopAttrib();
}
