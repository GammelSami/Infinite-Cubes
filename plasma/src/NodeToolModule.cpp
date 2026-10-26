#include "NodeToolModule.h"
#include "Tool.h" // SelectionMode.
#include "ToolInfoModule.h"
#include "plasma/GenericWidget.h"

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
NodeToolModule::NodeToolModule()
    : m_scaleBox(true),
      m_shapeBoundary(false),
      m_editMode(EDIT_MOVE),
      m_selectionMode(SELECTION_BOX)
{
    {
        std::wstring name(L"module:node:tool");
        std::wstring kind(L"bar");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 90.0f, 70.0f, 120.0f, 100.0f,
                                                           g_workspace.m_guiLeftNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Node", true);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void NodeToolModule::build()
{
    m_cursor.m_elements[0] = 50;
    m_cursor.m_elements[1] = 10;
    std::list<ModuleWidgetItem> items;
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:tool:select", L"radiobutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:tool:move", L"radiobutton2", 30, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:tool:rotate", L"radiobutton2", 60, 0, 30, 30));
    addPanel(items, L"", 3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:tool:selection:box", L"radiobutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:tool:selection:lasso", L"radiobutton2", 30, 0, 30, 30));
    addPanel(items, L"", 3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:tool:boundary:control", L"radiobutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:tool:boundary:shape", L"radiobutton2", 30, 0, 30, 30));
    addPanel(items, L"", 3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:tool:scalebox", L"checkbutton2", 0, 0, 30, 30));
    addPanel(items, L"", 3);
    getPanel()->getNode()->connectWidget(L"node:tool:scalebox", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeToolModule::chooseScaleBox, true);
    getPanel()->getNode()->connectWidget(L"node:tool:scalebox", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeToolModule::chooseScaleBox, true);
    getPanel()->getNode()->connectWidget(L"node:tool:select", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeToolModule::chooseSelect, true);
    getPanel()->getNode()->connectWidget(L"node:tool:move", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeToolModule::chooseMove, true);
    getPanel()->getNode()->connectWidget(L"node:tool:rotate", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeToolModule::chooseRotate, true);
    getPanel()->getNode()->connectWidget(L"node:tool:selection:box", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeToolModule::chooseBoxSelection, true);
    getPanel()->getNode()->connectWidget(L"node:tool:selection:lasso", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeToolModule::chooseLassoSelection, true);
    getPanel()->getNode()->connectWidget(L"node:tool:boundary:shape", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeToolModule::chooseShapeBoundary, true);
    getPanel()->getNode()->connectWidget(L"node:tool:boundary:control", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeToolModule::chooseControlBoundary, true);
    m_panel->setChecked(L"node:tool:move", true, false, true);
    m_panel->setChecked(L"node:tool:selection:box", true, false, true);
    m_panel->setChecked(L"node:tool:scalebox", true, false, true);
    m_panel->setChecked(L"node:tool:boundary:control", true, false, true);
    placeIcon(L"node:tool:select", L"icon:edit:select");
    placeIcon(L"node:tool:move", L"icon:edit:move");
    placeIcon(L"node:tool:rotate", L"icon:edit:rotate");
    placeIcon(L"node:tool:selection:box", L"icon:selection:box");
    placeIcon(L"node:tool:selection:lasso", L"icon:selection:lasso");
    placeIcon(L"node:tool:scalebox", L"icon:scalebox");
    placeIcon(L"node:tool:boundary:control", L"icon:bounds:control");
    placeIcon(L"node:tool:boundary:shape", L"icon:bounds:shape");
    fitWidth(10);
    fitHeight(10);
    createZoomSequences();
}

void NodeToolModule::chooseShapeBoundary()
{
    m_shapeBoundary = true;
}

void NodeToolModule::chooseControlBoundary()
{
    m_shapeBoundary = false;
}

void NodeToolModule::chooseScaleBox()
{
    m_scaleBox = m_panel->isChecked(L"node:tool:scalebox", true);
}

void NodeToolModule::chooseSelect()
{
    m_editMode = EDIT_SELECT;
    m_workspace->m_toolInfoModule->updatePanel();
}

void NodeToolModule::chooseMove()
{
    m_editMode = EDIT_MOVE;
    m_workspace->m_toolInfoModule->updatePanel();
}

void NodeToolModule::chooseRotate()
{
    m_editMode = EDIT_ROTATE;
    m_workspace->m_toolInfoModule->updatePanel();
}

void NodeToolModule::chooseBoxSelection()
{
    m_selectionMode = SELECTION_BOX;
}

void NodeToolModule::chooseLassoSelection()
{
    m_selectionMode = SELECTION_LASSO;
}
