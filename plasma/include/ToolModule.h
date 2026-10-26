// ToolModule: the editor's tool panel (captioned "Tool"): it owns one object of each tool and an icon per tool
// type ("icon:sheet", "icon:node"...), and switches the current tool ("tool:sheet", "tool:node"...).
#pragma once
#include <map>
#include "Module.h"
#include "FormatModule.h"
#include "GenericTool.h"
#include "NodeTool.h"
#include "VertexTool.h"
#include "EdgeTool.h"
#include "FaceTool.h"
#include "DrawFaceTool.h"
#include "ExtrusionTool.h"
#include "ColorTool.h"
#include "WidthTool.h"
#include "WidgetTool.h"
#include "TextTool.h"
#include "StampTool.h"
#include "SheetTool.h"
#include "DrawFreehandTool.h"

#include "DrawEdgeTool.h"
#include "ProceduralTool.h"

class ToolModule : public Module {
    friend class ::Workspace;

public:
    ToolModule();

    virtual void build();

    // After a tool change: shows the current tool's icon on the "tool:display:background" node, activates the tool,
    // fades the per-tool modules in or out and refreshes the tool info, cursor and sheet.
    void update();
    // Checks the tool's button and makes it current (Workspace's key shortcuts,; the argument is a tool type).
    void setTool(int type);
    Tool *getCurrentTool() const { return m_currentTool; }

    // Each makes its tool current ("tool:sheet"... buttons).
    void selectSheetTool();
    void selectNodeTool();
    void selectVertexTool();
    void selectEdgeTool();
    void selectFaceTool();
    void selectColorTool();
    void selectWidthTool();
    void selectDrawTool();
    void selectDrawFreehandTool();
    void selectSplitTool();
    void selectTextTool();
    void selectWidgetTool();
    void selectGenericTool();
    void selectExtrusionTool();
    void selectProceduralTool();
    void selectStampTool();

    SheetTool m_sheetTool;
    NodeTool m_nodeTool;
    VertexTool m_vertexTool;
    EdgeTool m_edgeTool;
    FaceTool m_faceTool;
    DrawFaceTool m_drawFaceTool;
    DrawFreehandTool m_drawFreehandTool;
    DrawEdgeTool m_drawEdgeTool;
    ColorTool m_colorTool;
    WidthTool m_widthTool;
    GenericTool m_genericTool;
    WidgetTool m_widgetTool;
    TextTool m_textTool;
    ExtrusionTool m_extrusionTool;
    ProceduralTool m_proceduralTool;
    StampTool m_stampTool;
    std::map<int, plasma::Node *> m_icons; // By tool type.
    Tool *m_currentTool;
    int m_unused5A0; // Not initialized by the constructor and otherwise unused.
};
