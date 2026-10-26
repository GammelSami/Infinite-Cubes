#include "Tool.h"
#include "HistoryState.h"
#include "Operations.h"
#include "DesignSheet.h"
#include "ShapeInterface.h"
#include "ToolModule.h"
#include "plasma/Node.h"
#include "plasma/Shape.h"
#include "plasma/Transformation.h"
#include <GL/glew.h>

using plasma::Node;

Tool::Tool(int type)
    : m_workspace(&g_workspace),
      m_type(type)
{}

void Tool::pointerDoubleClicked()
{
    if (!m_workspace->getDesignSheet() || !m_workspace->getHoveredSheetNode() ||
        m_workspace->getHoveredSheetNode()->m_selected)
        return;
    m_workspace->getHoveredSheetNode()->setSelected(true);
    m_workspace->getDesignSheet()->addOperation(new NodeSelectionOperation(m_workspace->getDesignSheet(), "select node",
                                                                           m_workspace->getHoveredSheetNode(), true));
    m_workspace->getDesignSheet()->m_currentNode = m_workspace->getHoveredNode();
    m_workspace->updateModules();
    m_workspace->selectionChanged();
    m_workspace->getOpenDesignSheet()->commit();
}

// Deselects the selected nodes; when none was selected, selects the edit root's shown, unlocked children.
void Tool::toggleSelectAll()
{
    if (!m_workspace->getOpenDesignSheet() || m_workspace->getOpenDesignSheet()->isBusy())
        return;
    deactivate();
    bool deselected = false;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it = sheet->m_sheetNodes.end();
    if (!sheet->isBusy())
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected && !it->first->isLocked()) {
            it->second->setSelected(false);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "deselect node", it->second, false));
            deselected = true;
        }
    }
    if (deselected) {
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->getOpenDesignSheet()->m_currentNode = 0;
    } else {
        for (std::list<Node *>::iterator child = m_workspace->getOpenDesignSheet()->m_editRoot->m_children.begin();
             child != m_workspace->getOpenDesignSheet()->m_editRoot->m_children.end(); ++child) {
            if ((*child)->isShown() && !(*child)->isLocked()) {
                m_workspace->getOpenDesignSheet()->findSheetNode(*child)->setSelected(true);
                m_workspace->getOpenDesignSheet()->addOperation(
                    new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "select node",
                                               m_workspace->getOpenDesignSheet()->findSheetNode(*child), true));
            }
        }
        m_workspace->getOpenDesignSheet()->updateCurrentNode(0);
    }
    m_workspace->updateModules();
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->refreshTimeline();
    m_workspace->redraw();
    m_workspace->markDesignSheetChanged();
}

void Tool::refreshChangedShapes()
{
    if (!m_workspace->getOpenDesignSheet() || m_workspace->getOpenDesignSheet()->isBusy())
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it = sheet->m_sheetNodes.end();
    if (!sheet->isBusy())
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_shapeInterface && it->second->m_shapeInterface->m_hasChanges) {
            it->first->m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            it->second->m_shapeInterface->updateChildren();
        }
    }
}

// Draws the interface icon of that name with its pivot at the position.
void Tool::drawIcon(const std::wstring &name, const plasma::Vector<2, float> &position)
{
    Node *icon = m_workspace->findIcon(name);
    if (!icon)
        return;
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    plasma::Vector<2, float> offset =
        position - icon->m_transformation->m_translation.value() - icon->m_transformation->m_pivot.value();
    glTranslatef(offset.m_elements[0], offset.m_elements[1], 0.0f);
    icon->draw(1 << plasma::NODE_RENDER_CULL_SCREEN);
    glPopMatrix();
}
