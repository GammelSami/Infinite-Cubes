// Loads and displays tool descriptions from XML.
#include "SpecialColorModule.h"
#include "ColorToolModule.h"
#include "NodeToolModule.h"
#include "PerformanceModule.h"
#include "ToolInfoModule.h"
#include "ExportPDFModule.h"
#include "ToolModule.h"
#include "TextModule.h"
#include "OnlineModule.h"
#include "FormatModule.h"
#include "plasma/Node.h"
#include "plasma/GenericWidget.h"
#include "../vendor/pugixml-0.9/src/pugixml.hpp"

ToolInfoModule::ToolInfoModule()
{
    m_panel = g_workspace.m_guiNode->findWidget(L"toolinfo");
    m_panel->getNode()->m_flags |= (1 << plasma::NODE_INACTIVE);
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, 0);
    pugi::xml_document document;
    if (document.load_file("toolinfos_en.xml")) {
        pugi::xml_node root = document.first_child();
        for (pugi::xml_node_iterator it = root.begin(); it != root.end(); ++it) {
            std::string key = (*it).attribute("key").value();
            if (!key.empty()) {
                ToolInfo *info = new ToolInfo;
                info->m_left = getXmlText((*it).child("left"));
                info->m_leftShift = getXmlText((*it).child("left_shift"));
                info->m_leftControl = getXmlText((*it).child("left_control"));
                info->m_leftAlt = getXmlText((*it).child("left_alt"));
                info->m_right = getXmlText((*it).child("right"));
                info->m_rightShift = getXmlText((*it).child("right_shift"));
                info->m_rightControl = getXmlText((*it).child("right_control"));
                info->m_rightAlt = getXmlText((*it).child("right_alt"));
                if (m_toolInfos[key])
                    delete m_toolInfos[key];
                m_toolInfos[key] = info;
            }
        }
    }
}

void ToolInfoModule::updatePanel()
{
    ToolInfo *info = 0;
    switch (m_workspace->getToolModule()->getCurrentTool()->getType()) {
    case Tool::TOOL_SHEET:
        info = m_toolInfos["tool:sheet"];
        break;
    case Tool::TOOL_NODE:
        switch (m_workspace->m_nodeToolModule->m_editMode) {
        case EDIT_SELECT:
            info = m_toolInfos["tool:node:select"];
            break;
        case EDIT_MOVE:
            info = m_toolInfos["tool:node:move"];
            break;
        case EDIT_ROTATE:
            info = m_toolInfos["tool:node:rotate"];
            break;
        }
        break;
    case Tool::TOOL_VERTEX:
        switch (m_workspace->getVertexModule()->m_editMode) {
        case EDIT_SELECT:
            info = m_toolInfos["tool:vertex:select"];
            break;
        case EDIT_MOVE:
            info = m_toolInfos["tool:vertex:move"];
            break;
        case EDIT_ROTATE:
            info = m_toolInfos["tool:vertex:rotate"];
            break;
        }
        break;
    case Tool::TOOL_EDGE:
        switch (m_workspace->getVertexModule()->m_editMode) {
        case EDIT_SELECT:
            info = m_toolInfos["tool:edge:select"];
            break;
        case EDIT_MOVE:
            info = m_toolInfos["tool:edge:move"];
            break;
        case EDIT_ROTATE:
            info = m_toolInfos["tool:edge:rotate"];
            break;
        }
        break;
    case Tool::TOOL_FACE:
        switch (m_workspace->getVertexModule()->m_editMode) {
        case EDIT_SELECT:
            info = m_toolInfos["tool:face:select"];
            break;
        case EDIT_MOVE:
            info = m_toolInfos["tool:face:move"];
            break;
        case EDIT_ROTATE:
            info = m_toolInfos["tool:face:rotate"];
            break;
        }
        break;
    case Tool::TOOL_COLOR:
        info = m_toolInfos["tool:color"];
        break;
    case Tool::TOOL_TEXT:
        info = m_toolInfos["tool:text"];
        break;
    case Tool::TOOL_GENERIC:
        info = m_toolInfos["tool:generic"];
        break;
    case Tool::TOOL_DRAW_FACE:
        info = m_toolInfos["tool:draw"];
        break;
    case Tool::TOOL_DRAW_FREEHAND:
        info = m_toolInfos["tool:draw:freehand"];
        break;
    case Tool::TOOL_DRAW_EDGE:
        info = m_toolInfos["tool:split"];
        break;
    case Tool::TOOL_WIDTH:
        info = m_toolInfos["tool:width"];
        break;
    case Tool::TOOL_WIDGET:
        info = m_toolInfos["tool:widget"];
        break;
    case Tool::TOOL_EXTRUSION:
        info = m_toolInfos["tool:extrusion"];
        break;
    case Tool::TOOL_PROCEDURAL:
        info = m_toolInfos["tool:procedural"];
        break;
    case Tool::TOOL_STAMP:
        info = m_toolInfos["tool:stamp"];
        break;
    }
    if (!info) {
        static ToolInfo empty;
        info = &empty;
    }
    getPanel()->getNode()->setText(L"toolinfo:left", info->m_left, true);
    getPanel()->getNode()->setText(L"toolinfo:left:shift", info->m_leftShift, true);
    getPanel()->getNode()->setText(L"toolinfo:left:control", info->m_leftControl, true);
    getPanel()->getNode()->setText(L"toolinfo:left:alt", info->m_leftAlt, true);
    getPanel()->getNode()->setText(L"toolinfo:right", info->m_right, true);
    getPanel()->getNode()->setText(L"toolinfo:right:shift", info->m_rightShift, true);
    getPanel()->getNode()->setText(L"toolinfo:right:control", info->m_rightControl, true);
    getPanel()->getNode()->setText(L"toolinfo:right:alt", info->m_rightAlt, true);
}
