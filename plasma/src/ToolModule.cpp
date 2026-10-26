#include "ToolModule.h"
#include "SheetTool.h"
#include "DrawFreehandTool.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "ColorToolModule.h"
#include "DrawFreehandModule.h"
#include "NodeToolModule.h"
#include "ProceduralShapeModule.h"
#include "ScaleBoxModule.h"
#include "StampModule.h"
#include "StrokeWidthModule.h"
#include "ToolInfoModule.h"
#include "VertexModule.h"
#include "WidgetModule.h"

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
ToolModule::ToolModule()
    : m_nodeTool(Tool::TOOL_NODE)
{
    m_currentTool = &m_nodeTool;
    {
        std::wstring name(L"");
        std::wstring kind(L"bar");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 50.0f, 100.0f, 55.0f, 500.0f,
                                                           g_workspace.m_scalableNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Tool", true);
    m_icons[Tool::TOOL_SHEET] = g_workspace.findIcon(L"icon:sheet");
    m_icons[Tool::TOOL_NODE] = g_workspace.findIcon(L"icon:node");
    m_icons[Tool::TOOL_VERTEX] = g_workspace.findIcon(L"icon:vertex");
    m_icons[Tool::TOOL_EDGE] = g_workspace.findIcon(L"icon:edge");
    m_icons[Tool::TOOL_FACE] = g_workspace.findIcon(L"icon:face");
    m_icons[Tool::TOOL_COLOR] = g_workspace.findIcon(L"icon:color");
    m_icons[Tool::TOOL_WIDTH] = g_workspace.findIcon(L"icon:width");
    m_icons[Tool::TOOL_EXTRUSION] = g_workspace.findIcon(L"icon:extrusion");
    m_icons[Tool::TOOL_DRAW_FACE] = g_workspace.findIcon(L"icon:draw");
    m_icons[Tool::TOOL_DRAW_FREEHAND] = g_workspace.findIcon(L"icon:draw:freehand");
    m_icons[Tool::TOOL_DRAW_EDGE] = g_workspace.findIcon(L"icon:split");
    m_icons[Tool::TOOL_TEXT] = g_workspace.findIcon(L"icon:text");
    m_icons[Tool::TOOL_WIDGET] = g_workspace.findIcon(L"icon:widget");
    m_icons[Tool::TOOL_GENERIC] = g_workspace.findIcon(L"icon:generic");
    m_icons[Tool::TOOL_PROCEDURAL] = g_workspace.findIcon(L"icon:procedural");
    m_icons[Tool::TOOL_STAMP] = g_workspace.findIcon(L"icon:stamp");
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void ToolModule::build()
{
    // The tool bar goes below the display bar, centred on it.
    plasma::Widget *display = m_workspace->m_engine.findWidget(L"tool:display");
    if (display) {
        display->m_node->setParent(getPanel()->getNode()->m_parent);
        display->moveTo(-10.0f, 20.0f, true);
        m_panel->moveTo(display->viewX() + display->viewWidth() * 0.5 - m_panel->viewWidth() * 0.5,
                        display->viewY() + display->getViewHeight() * 0.75, true);
    }
    std::list<ModuleWidgetItem> items;
    items.push_back(ModuleWidgetItem(L" ", L"tool:sheet", L"radiobutton3", 0, 0, 35, 35));
    items.push_back(ModuleWidgetItem(L" ", L"tool:node", L"radiobutton3", 0, 35, 35, 35));
    items.push_back(ModuleWidgetItem(L" ", L"tool:vertex", L"radiobutton3", 0, 70, 35, 35));
    items.push_back(ModuleWidgetItem(L" ", L"tool:edge", L"radiobutton3", 0, 105, 35, 35));
    items.push_back(ModuleWidgetItem(L" ", L"tool:face", L"radiobutton3", 0, 140, 35, 35));
    items.push_back(ModuleWidgetItem(L" ", L"tool:draw", L"radiobutton3", 0, 175, 35, 35));
    items.push_back(ModuleWidgetItem(L" ", L"tool:split", L"radiobutton3", 0, 210, 35, 35));
    items.push_back(ModuleWidgetItem(L" ", L"tool:color", L"radiobutton3", 0, 245, 35, 35));
    items.push_back(ModuleWidgetItem(L" ", L"tool:width", L"radiobutton3", 0, 280, 35, 35));
    items.push_back(ModuleWidgetItem(L" ", L"tool:extrusion", L"radiobutton3", 0, 315, 35, 35));
    items.push_back(ModuleWidgetItem(L" ", L"tool:draw:freehand", L"radiobutton3", 0, 350, 35, 35));
    items.push_back(ModuleWidgetItem(L" ", L"tool:text", L"radiobutton3", 0, 385, 35, 35));
    items.push_back(ModuleWidgetItem(L" ", L"tool:procedural", L"radiobutton3", 0, 420, 35, 35));
    items.push_back(ModuleWidgetItem(L" ", L"tool:stamp", L"radiobutton3", 0, 455, 35, 35));
    addPanelAt(items, L"", 10, 30);
    fitHeight(10);
    placeIcon(L"tool:sheet", L"icon:sheet");
    placeIcon(L"tool:node", L"icon:node");
    placeIcon(L"tool:vertex", L"icon:vertex");
    placeIcon(L"tool:edge", L"icon:edge");
    placeIcon(L"tool:face", L"icon:face");
    placeIcon(L"tool:color", L"icon:color");
    placeIcon(L"tool:width", L"icon:width");
    placeIcon(L"tool:extrusion", L"icon:extrusion");
    placeIcon(L"tool:draw", L"icon:draw");
    placeIcon(L"tool:draw:freehand", L"icon:draw:freehand");
    placeIcon(L"tool:split", L"icon:split");
    placeIcon(L"tool:text", L"icon:text");
    placeIcon(L"tool:widget", L"icon:widget");
    placeIcon(L"tool:generic", L"icon:generic");
    placeIcon(L"tool:procedural", L"icon:procedural");
    placeIcon(L"tool:stamp", L"icon:stamp");
    getPanel()->getNode()->connectWidget(L"tool:sheet", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectSheetTool, true);
    getPanel()->getNode()->connectWidget(L"tool:node", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectNodeTool, true);
    getPanel()->getNode()->connectWidget(L"tool:vertex", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectVertexTool, true);
    getPanel()->getNode()->connectWidget(L"tool:edge", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectEdgeTool, true);
    getPanel()->getNode()->connectWidget(L"tool:face", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectFaceTool, true);
    getPanel()->getNode()->connectWidget(L"tool:color", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectColorTool, true);
    getPanel()->getNode()->connectWidget(L"tool:width", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectWidthTool, true);
    getPanel()->getNode()->connectWidget(L"tool:extrusion", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectExtrusionTool, true);
    getPanel()->getNode()->connectWidget(L"tool:draw", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectDrawTool, true);
    getPanel()->getNode()->connectWidget(L"tool:draw:freehand", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectDrawFreehandTool, true);
    getPanel()->getNode()->connectWidget(L"tool:split", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectSplitTool, true);
    getPanel()->getNode()->connectWidget(L"tool:text", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectTextTool, true);
    getPanel()->getNode()->connectWidget(L"tool:widget", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectWidgetTool, true);
    getPanel()->getNode()->connectWidget(L"tool:generic", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectGenericTool, true);
    getPanel()->getNode()->connectWidget(L"tool:procedural", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectProceduralTool, true);
    getPanel()->getNode()->connectWidget(L"tool:stamp", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ToolModule::selectStampTool, true);
    (&m_sheetTool)->initialize();
    (&m_nodeTool)->initialize();
    (&m_vertexTool)->initialize();
    (&m_edgeTool)->initialize();
    (&m_faceTool)->initialize();
    (&m_colorTool)->initialize();
    (&m_widthTool)->initialize();
    (&m_drawFaceTool)->initialize();
    (&m_drawFreehandTool)->initialize();
    (&m_drawEdgeTool)->initialize();
    (&m_textTool)->initialize();
    (&m_widgetTool)->initialize();
    (&m_genericTool)->initialize();
    (&m_extrusionTool)->initialize();
    (&m_proceduralTool)->initialize();
    (&m_stampTool)->initialize();
    createZoomSequences();
    update();
}

void ToolModule::update()
{
    plasma::Node *background = m_workspace->getGuiNode()->findNode(L"tool:display:background");
    plasma::Node *source = m_icons[m_currentTool->getType()];
    if (background && source) {
        // Replace the icon shown before (the first child, unless it is the background's "shine").
        if (background->m_children.begin() != background->m_children.end() &&
            background->m_children.front()->getName() != L"shine")
            m_workspace->m_engine.destroyNode(background->m_children.front());
        plasma::Node *icon = source->instance(0);
        background->insertChild(background->m_children.begin(), icon);
        icon->m_flags |= (1 << plasma::NODE_IGNORES_POINTER);
        icon->setTransformation(icon->m_transformation->clone());
        icon->m_transformation->m_deformation.value().scale(5.0f, 5.0f);
        icon->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);

        // Centre the icon's bounds on its origin, then move it to the centre of the background's bounds.
        icon->m_transformation->m_translation.value() -= (icon->getMinimum(0) + icon->getMaximum(0)) * 0.5f;
        icon->m_transformation->m_translation.value() += background->m_transformation->m_inverseMatrix.transformPoint(
            (background->getMinimum(0) + background->getMaximum(0)) * 0.5f);
        icon->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
    m_currentTool->activate();

    // The modules that apply to the current tool fade in, the others out (tool types as in m_icons).
    if (m_workspace->m_proceduralShapeModule)
        m_workspace->m_proceduralShapeModule->setFadingIn(m_currentTool->getType() == Tool::TOOL_PROCEDURAL);
    if (m_workspace->m_nodeToolModule)
        m_workspace->m_nodeToolModule->setFadingIn(m_currentTool->getType() == Tool::TOOL_NODE);
    if (m_workspace->m_colorToolModule)
        m_workspace->m_colorToolModule->setFadingIn(m_currentTool->getType() == Tool::TOOL_COLOR);
    if (m_workspace->m_vertexModule) {
        int type = m_currentTool->getType();
        m_workspace->m_vertexModule->setFadingIn(type == Tool::TOOL_VERTEX || type == Tool::TOOL_EDGE ||
                                                 type == Tool::TOOL_FACE);
    }
    if (m_workspace->m_strokeWidthModule) {
        int type = m_currentTool->getType();
        m_workspace->m_strokeWidthModule->setFadingIn(type == Tool::TOOL_DRAW_FACE || type == Tool::TOOL_WIDTH ||
                                                      type == Tool::TOOL_PROCEDURAL);
    }
    if (m_workspace->m_drawFreehandModule)
        m_workspace->m_drawFreehandModule->setFadingIn(m_currentTool->getType() == Tool::TOOL_DRAW_FREEHAND);
    if (m_workspace->m_stampModule)
        m_workspace->m_stampModule->setFadingIn(m_currentTool->getType() == Tool::TOOL_STAMP);
    if (m_workspace->m_scaleBoxModule) {
        int type = m_currentTool->getType();
        m_workspace->m_scaleBoxModule->setFadingIn(type == Tool::TOOL_NODE || type == Tool::TOOL_VERTEX ||
                                                   type == Tool::TOOL_EDGE || type == Tool::TOOL_FACE);
    }
    if (m_workspace->m_widgetModule)
        m_workspace->m_widgetModule->setFadingIn(m_currentTool->getType() == Tool::TOOL_WIDGET);
    if (m_workspace->getToolInfoModule())
        m_workspace->getToolInfoModule()->updatePanel();
    m_workspace->updateCursor();
    m_workspace->markDesignSheetChanged();
}

void ToolModule::selectSheetTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_sheetTool;
    update();
}

void ToolModule::selectNodeTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_nodeTool;
    update();
}

void ToolModule::selectVertexTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_vertexTool;
    update();
}

void ToolModule::selectEdgeTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_edgeTool;
    update();
}

void ToolModule::selectFaceTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_faceTool;
    update();
}

void ToolModule::selectColorTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_colorTool;
    update();
}

void ToolModule::selectWidthTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_widthTool;
    update();
}

void ToolModule::selectDrawTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_drawFaceTool;
    update();
}

void ToolModule::selectDrawFreehandTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_drawFreehandTool;
    update();
}

void ToolModule::selectSplitTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_drawEdgeTool;
    update();
}

void ToolModule::selectTextTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_textTool;
    update();
}

void ToolModule::selectWidgetTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_widgetTool;
    update();
}

void ToolModule::selectGenericTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_genericTool;
    update();
}

void ToolModule::selectExtrusionTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_extrusionTool;
    update();
}

void ToolModule::selectProceduralTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_proceduralTool;
    update();
}

void ToolModule::selectStampTool()
{
    m_currentTool->deactivate();
    m_currentTool = &m_stampTool;
    update();
}

void ToolModule::setTool(int type)
{
    switch (type) {
    case Tool::TOOL_SHEET:
        m_panel->setChecked(L"tool:sheet", true, false, true);
        selectSheetTool();
        break;
    case Tool::TOOL_NODE:
        m_panel->setChecked(L"tool:node", true, false, true);
        selectNodeTool();
        break;
    case Tool::TOOL_VERTEX:
        m_panel->setChecked(L"tool:vertex", true, false, true);
        selectVertexTool();
        break;
    case Tool::TOOL_EDGE:
        m_panel->setChecked(L"tool:edge", true, false, true);
        selectEdgeTool();
        break;
    case Tool::TOOL_FACE:
        m_panel->setChecked(L"tool:face", true, false, true);
        selectFaceTool();
        break;
    case Tool::TOOL_COLOR:
        m_panel->setChecked(L"tool:color", true, false, true);
        selectColorTool();
        break;
    case Tool::TOOL_DRAW_FACE:
        m_panel->setChecked(L"tool:draw", true, false, true);
        selectDrawTool();
        break;
    case Tool::TOOL_DRAW_FREEHAND:
        m_panel->setChecked(L"tool:draw:freehand", true, false, true);
        selectDrawFreehandTool();
        break;
    case Tool::TOOL_DRAW_EDGE:
        m_panel->setChecked(L"tool:split", true, false, true);
        selectSplitTool();
        break;
    case Tool::TOOL_WIDTH:
        m_panel->setChecked(L"tool:width", true, false, true);
        selectWidthTool();
        break;
    case Tool::TOOL_EXTRUSION:
        m_panel->setChecked(L"tool:extrusion", true, false, true);
        selectExtrusionTool();
        break;
    case Tool::TOOL_TEXT:
        m_panel->setChecked(L"tool:text", true, false, true);
        selectTextTool();
        break;
    case Tool::TOOL_PROCEDURAL:
        m_panel->setChecked(L"tool:procedural", true, false, true);
        selectProceduralTool();
        break;
    case Tool::TOOL_STAMP:
        m_panel->setChecked(L"tool:stamp", true, false, true);
        selectStampTool();
        break;
    }
}
