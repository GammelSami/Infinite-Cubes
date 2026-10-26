// Freehand-tool support implementations.
#include "SheetTool.h"
#include "DrawFreehandTool.h"
#include "DesignSheet.h"
#include "MeshInterface.h"
#include "ColorModule.h"
#include "StrokeWidthModule.h"
#include "DrawFreehandModule.h"
#include "plasma/ListWidget.h"
#include "plasma/Shape.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/RenderSurface.h"
#include <GL/glew.h>
#include <windows.h>

using plasma::Vector;

void DrawFreehandTool::pointerPressed()
{
    Workspace *workspace = m_workspace;
    if (workspace->m_engine.isKeyDown(VK_MENU)) {
        pickColor(false);
        m_savedColor = m_workspace->m_colorModule->getColor();
    } else if (workspace->getDesignSheet()) {
        if (m_workspace->getDesignSheet()->m_currentNode && m_workspace->m_engine.isKeyDown(VK_SHIFT)) {
            plasma::Node *current = m_workspace->getDesignSheet()->m_currentNode;
            NodeInterface *node = m_workspace->getDesignSheet()->findSheetNode(current);
            ShapeInterface *shapeInterface = node->m_shapeInterface;
            m_meshInterface = shapeInterface && shapeInterface->m_type == INTERFACE_MESH
                                  ? static_cast<MeshInterface *>(shapeInterface)
                                  : 0;
        } else {
            m_meshInterface = 0;
        }
        const Vector<2, float> &pointer =
            *reinterpret_cast<Vector<2, float> *>(reinterpret_cast<char *>(m_workspace) + 0x108);
        pointerMoved((int)pointer.m_elements[0], (int)pointer.m_elements[1]);
    }
}

void DrawFreehandTool::pickColor(bool preview)
{
    if (m_workspace->getOpenSheet()) {
        glPushAttrib(GL_COLOR_BUFFER_BIT);
        m_workspace->getOpenSheet()->m_viewWidget->m_surface->begin();
        glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
        int x;
        {
            float pointerX = *reinterpret_cast<float *>(reinterpret_cast<char *>(m_workspace) + 0x108);
            const float &originX = m_workspace->getOpenSheet()->m_viewWidget->getContentPosition().m_elements[0];
            x = (int)((pointerX - originX) * m_workspace->getOpenSheet()->m_viewWidget->m_surfaceScale);
        }
        int y;
        {
            float pointerY = *reinterpret_cast<float *>(reinterpret_cast<char *>(m_workspace) + 0x10c);
            const float &originY = m_workspace->getOpenSheet()->m_viewWidget->getContentPosition().m_elements[1];
            y = (int)((pointerY - originY) * m_workspace->getOpenSheet()->m_viewWidget->m_surfaceScale);
        }
        unsigned char pixels[4];
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        Vector<4, float> color;
        color.m_elements[0] = (double)pixels[0] / 255.0;
        color.m_elements[1] = (double)pixels[1] / 255.0;
        color.m_elements[2] = (double)pixels[2] / 255.0;
        color.m_elements[3] = (double)pixels[3] / 255.0;
        if (color.m_elements[3] > 0.0001) {
            color.m_elements[0] /= color.m_elements[3];
            color.m_elements[1] /= color.m_elements[3];
            color.m_elements[2] /= color.m_elements[3];
        } else {
            color = m_workspace->getOpenSheet()->m_page.color;
        }
        m_workspace->getOpenSheet()->m_viewWidget->m_surface->end();
        if (m_workspace->m_engine.topSurface())
            m_workspace->m_engine.topSurface()->begin();
        glPopAttrib();
        m_workspace->m_colorModule->setColor(color, true, !preview, false);
    }
}
