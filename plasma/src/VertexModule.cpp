#include "VertexModule.h"
#include "Tool.h" // SelectionMode.
#include "ToolInfoModule.h"
#include "plasma/GenericWidget.h"

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
VertexModule::VertexModule()
    : m_editMode(EDIT_MOVE),
      m_selectionMode(SELECTION_BOX)
{
    m_softSelection = false;
    m_scaleBoxEnabled = false;

    // Release both factory strings before changing the panel flags.
    {
        std::wstring name(L"module:vertex");
        std::wstring kind(L"bar");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 90.0f, 70.0f, 120.0f, 100.0f,
                                                           g_workspace.m_guiLeftNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Vertex", true);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void VertexModule::build()
{
    m_cursor = plasma::Vector<2, int>(50, 10);
    std::list<ModuleWidgetItem> items;
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"vertex:select", L"radiobutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"vertex:move", L"radiobutton2", 30, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"vertex:rotate", L"radiobutton2", 60, 0, 30, 30));
    addPanel(items, L"", 3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"vertex:selection:box", L"radiobutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"vertex:selection:lasso", L"radiobutton2", 30, 0, 30, 30));
    addPanel(items, L"", 3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"vertex:softselection", L"checkbutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"vertex:scalebox", L"checkbutton2", 30, 0, 30, 30));
    addPanel(items, L"", 3);
    getPanel()->getNode()->connectWidget(L"vertex:softselection", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &VertexModule::chooseSoftSelection, true);
    getPanel()->getNode()->connectWidget(L"vertex:softselection", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &VertexModule::chooseSoftSelection, true);
    getPanel()->getNode()->connectWidget(L"vertex:scalebox", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &VertexModule::chooseScaleBox, true);
    getPanel()->getNode()->connectWidget(L"vertex:scalebox", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &VertexModule::chooseScaleBox, true);
    getPanel()->getNode()->connectWidget(L"vertex:select", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &VertexModule::chooseSelect, true);
    getPanel()->getNode()->connectWidget(L"vertex:move", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &VertexModule::chooseMove, true);
    getPanel()->getNode()->connectWidget(L"vertex:rotate", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &VertexModule::chooseRotate, true);
    getPanel()->getNode()->connectWidget(L"vertex:selection:box", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &VertexModule::chooseBoxSelection, true);
    getPanel()->getNode()->connectWidget(L"vertex:selection:lasso", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &VertexModule::chooseLassoSelection, true);
    m_panel->setChecked(L"vertex:move", true, false, true);
    m_panel->setChecked(L"vertex:selection:box", true, false, true);
    placeIcon(L"vertex:select", L"icon:edit:select");
    placeIcon(L"vertex:move", L"icon:edit:move");
    placeIcon(L"vertex:rotate", L"icon:edit:rotate");
    placeIcon(L"vertex:selection:box", L"icon:selection:box");
    placeIcon(L"vertex:selection:lasso", L"icon:selection:lasso");
    placeIcon(L"vertex:scalebox", L"icon:scalebox");
    placeIcon(L"vertex:softselection", L"icon:softselection");
    fitWidth(10);
    fitHeight(10);
    createZoomSequences();
}

void VertexModule::chooseScaleBox()
{
    m_scaleBoxEnabled = m_panel->isChecked(L"vertex:scalebox", true);
}

void VertexModule::chooseSoftSelection()
{
    m_softSelection = m_panel->isChecked(L"vertex:softselection", true);
}

void VertexModule::chooseSelect()
{
    m_editMode = EDIT_SELECT;
    m_workspace->m_toolInfoModule->updatePanel();
}

void VertexModule::chooseMove()
{
    m_editMode = EDIT_MOVE;
    m_workspace->m_toolInfoModule->updatePanel();
}

void VertexModule::chooseRotate()
{
    m_editMode = EDIT_ROTATE;
    m_workspace->m_toolInfoModule->updatePanel();
}

void VertexModule::chooseBoxSelection()
{
    m_selectionMode = SELECTION_BOX;
}

void VertexModule::chooseLassoSelection()
{
    m_selectionMode = SELECTION_LASSO;
}

// Node::ConnectWidget<VertexModule>, instantiated by Build above. Widget::Connect<VertexModule>, instantiated by
// Node::ConnectWidget. Widget::SetConnection<VertexModule>, instantiated by Widget::Connect.
