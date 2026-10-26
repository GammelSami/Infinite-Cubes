#include "WidthTool.h"
#include <windows.h>
#include "DesignSheet.h"
#include "StrokeWidthModule.h"
#include "plasma/ListWidget.h"

using plasma::Vector;

WidthTool::WidthTool()
    : Tool(TOOL_WIDTH),
      m_picking(false)
{}

void WidthTool::applyStrokeWidth()
{
    if (!m_workspace->getDesignSheet())
        return;
    if (m_workspace->m_engine.isKeyDown(VK_SHIFT)) {
        bool found = false;
        float minimum, maximum;
        DesignSheet *sheet = m_workspace->getDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                float low, high;
                if (it->second->m_shapeInterface->getStrokeWidthRange(low, high)) {
                    if (!found) {
                        minimum = low;
                        maximum = high;
                        found = true;
                    }
                    if (minimum > low)
                        minimum = low;
                    if (maximum < high)
                        maximum = high;
                }
            }
        }
        if (found) {
            sheet = m_workspace->getDesignSheet();
            if (sheet->isBusy())
                it = sheet->m_sheetNodes.end();
            else
                it = sheet->m_sheetNodes.begin();
            for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
                if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface)
                    it->second->m_shapeInterface->scaleStrokeWidths(minimum, maximum);
            }
        }
    } else {
        DesignSheet *sheet = m_workspace->getDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface)
                it->second->m_shapeInterface->paintStrokeWidth();
        }
    }
}

void WidthTool::pickStrokeWidth()
{
    if (!m_workspace->getDesignSheet())
        return;
    float nearestDistance = 0.0f;
    bool found = false;
    float pickedWidth;
    m_pickPosition = m_workspace->m_sceneMousePosition;
    if (m_workspace->getDesignSheet()) {
        DesignSheet *sheet = m_workspace->getDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                float width;
                Vector<2, float> position;
                if (it->second->m_shapeInterface->pickStrokeWidth(width, position)) {
                    Vector<2, float> offset = position - m_workspace->m_sceneMousePosition;
                    float distance =
                        offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1];
                    if (!found || nearestDistance > distance) {
                        nearestDistance = distance;
                        found = true;
                        m_pickPosition = position;
                        pickedWidth = width;
                    }
                }
            }
        }
        if (found)
            m_workspace->m_strokeWidthModule->setWidth(pickedWidth);
    }
}

void WidthTool::pointerPressed()
{
    if (m_workspace->getOpenDesignSheet()) {
        if (m_workspace->m_engine.isKeyDown(VK_MENU)) {
            pickStrokeWidth();
            m_savedWidth = m_workspace->m_strokeWidthModule->m_width;
        } else {
            m_workspace->getOpenDesignSheet()->beginModifySelected();
            applyStrokeWidth();
        }
    }
}

void WidthTool::pointerReleased()
{
    refreshChangedShapes();
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->getOpenDesignSheet()->endModifyChanged();
        m_workspace->getOpenDesignSheet()->commit();
    }
}

void WidthTool::pointerMoved(int x, int y)
{
    Workspace *workspace = m_workspace;
    if (workspace->m_engine.isKeyDown(VK_MENU)) {
        if (!workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT))
            pickStrokeWidth();
    } else if (workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT)) {
        applyStrokeWidth();
    }
}

void WidthTool::fillContextMenu()
{
    std::wstring caption(L"Resize Tool Radius...");
    Workspace *workspace = m_workspace;
    plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
    item->setPopUpItem(true, true);
    item->setCaption(caption, true);
    item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::resizeToolRadius, true, true);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Draws the tool's
// icons: the pipette at the picked vertex while Alt picks, and beside the pointer the colorize icon while Shift
// applies the width to whole shapes.
void WidthTool::draw()
{
    if (m_workspace->m_engine.isKeyDown(VK_MENU))
        drawIcon(L"icon:pipette", m_pickPosition);
    if (m_workspace->m_engine.isKeyDown(VK_SHIFT)) {
        std::wstring name(L"icon:color:colorize");
        Vector<2, float> position = m_workspace->m_sceneMousePosition;
        position.m_elements[0] += 25.0;
        position.m_elements[1] += 15.0;
        drawIcon(name, position);
    }
}

void WidthTool::keyPressed(unsigned short key)
{
    if (key == VK_MENU) {
        m_picking = true;
        m_savedWidth = m_workspace->m_strokeWidthModule->m_width;
        pickStrokeWidth();
        m_workspace->m_redrawRequested = true;
    }
}

void WidthTool::keyReleased(unsigned short key)
{
    if (key == VK_MENU && m_picking) {
        m_picking = false;
        m_workspace->m_strokeWidthModule->setWidth(m_savedWidth);
        m_workspace->m_redrawRequested = true;
    }
}
