// Sheet-tool support for moving, creating, closing and arranging sheets.
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

void SheetTool::pointerDoubleClicked()
{
    if (m_workspace->getOpenSheet()) {
        if (m_workspace->m_currentSheetOnly) {
            m_workspace->showAllSheets();
        } else {
            Vector<2, float> pointer =
                *reinterpret_cast<Vector<2, float> *>(reinterpret_cast<char *>(m_workspace) + 0x108);
            plasma::Node *panel = m_workspace->getOpenSheet()->m_panel->getNode();
            Vector<2, float> point = panel->m_inverseMatrix.transform(pointer);
            Vector<2, float> adjusted =
                m_workspace->getOpenSheet()->m_panel->undeform(point, m_workspace->getOpenSheet()->m_panel->getNode());
            if (m_workspace->getOpenSheet()->m_panel->getNode()->m_shape) {
                if (m_workspace->getOpenSheet()->m_panel->getNode()->m_shape->contains(adjusted))
                    m_workspace->showCurrentSheetOnly();
            }
        }
    }
}

void SheetTool::bringToFront()
{
    if (m_workspace->getOpenSheet()) {
        m_workspace->m_sheetsNode->removeChild(m_workspace->getOpenSheet()->m_frame->getNode());
        m_workspace->m_sheetsNode->addChild(m_workspace->getOpenSheet()->m_frame->getNode());
        m_workspace->updateSheetOrder();
    }
}

void SheetTool::sendToBack()
{
    if (m_workspace->getOpenSheet()) {
        m_workspace->m_sheetsNode->removeChild(m_workspace->getOpenSheet()->m_frame->getNode());
        m_workspace->m_sheetsNode->insertChild(m_workspace->m_sheetsNode->m_children.begin(),
                                               m_workspace->getOpenSheet()->m_frame->getNode());
        m_workspace->updateSheetOrder();
    }
}

void SheetTool::bringForward()
{
    if (m_workspace->getOpenSheet()) {
        std::list<plasma::Node *>::iterator it = m_workspace->m_sheetsNode->m_children.begin();
        while (it != m_workspace->m_sheetsNode->m_children.end()) {
            bool found = *it == m_workspace->getOpenSheet()->m_frame->getNode();
            ++it;
            if (found) {
                if (it != m_workspace->m_sheetsNode->m_children.end()) {
                    ++it;
                    m_workspace->m_sheetsNode->insertChild(it, m_workspace->getOpenSheet()->m_frame->getNode());
                    m_workspace->updateSheetOrder();
                }
                break;
            }
        }
    }
}

void SheetTool::sendBackward()
{
    if (m_workspace->getOpenSheet()) {
        std::list<plasma::Node *>::iterator it = m_workspace->m_sheetsNode->m_children.begin();
        while (it != m_workspace->m_sheetsNode->m_children.end()) {
            if (*it == m_workspace->getOpenSheet()->m_frame->getNode()) {
                if (it != m_workspace->m_sheetsNode->m_children.begin()) {
                    --it;
                    m_workspace->m_sheetsNode->insertChild(it, m_workspace->getOpenSheet()->m_frame->getNode());
                    m_workspace->updateSheetOrder();
                }
                break;
            }
            ++it;
        }
    }
}
