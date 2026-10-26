#include "DrawFreehandModule.h"
#include "OnlineModule.h"
#include "StrokeWidthModule.h"
#include "TextModule.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "FormatModule.h"
std::string narrowString(const std::wstring &text);
#include <sstream>
#include <cstdlib>

DrawFreehandModule::DrawFreehandModule()
    : m_width(10.0f),
      m_simplification(50.0f),
      m_pressureStroke(false),
      m_pressureOpacity(false),
      m_corners(true)
{
    {
        std::wstring name(L"module:freehand:width");
        std::wstring kind(L"bar");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 90.0f, 70.0f, 120.0f, 100.0f,
                                                           g_workspace.m_guiLeftNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Freehand", true);
    m_orientation = ORIENTATION_HORIZONTAL;
}

void DrawFreehandModule::build()
{
    m_cursor.m_elements[0] = 50;
    m_cursor.m_elements[1] = 15;
    std::list<ModuleWidgetItem> items;
    addWidget(L"Stroke width:", L"", L"static", 70, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"freehand:width", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"freehand:width:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    addWidget(L"px", L"text:unit", L"static", 20, 20, 0);
    getPanel()->getNode()->setEditFilter(L"freehand:width", &plasma::g_unsignedFloatFilter, true);
    addWidget(L"Simplification:", L"", L"static", 70, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"freehand:simplification", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"freehand:simplification:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    addWidget(L"%", L"", L"static", 20, 20, 0);
    getPanel()->getNode()->setEditFilter(L"freehand:simplification", &plasma::g_unsignedFloatFilter, true);
    m_cursor.m_elements[1] = 10;
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"freehand:corners", L"checkbutton2", 0, 0, 30, 30));
    addPanel(items, L"", 3);
    m_panel->setChecked(L"freehand:corners", true, false, true);
    placeIcon(L"freehand:corners", L"icon:freehand:corners");
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"freehand:pressure:stroke", L"checkbutton2", 0, 0, 50, 30));
    items.push_back(ModuleWidgetItem(L" ", L"freehand:pressure:opacity", L"checkbutton2", 50, 0, 50, 30));
    addPanel(items, L"", 3);
    placeIcon(L"freehand:pressure:stroke", L"icon:pressure:stroke");
    placeIcon(L"freehand:pressure:opacity", L"icon:pressure:opacity");
    getPanel()->getNode()->connectWidget(L"freehand:width", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &DrawFreehandModule::editWidth, true);
    getPanel()->getNode()->connectWidget(L"freehand:width:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &DrawFreehandModule::dragWidth, true);
    getPanel()->getNode()->connectWidget(L"freehand:width:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &DrawFreehandModule::stepWidth, true);
    getPanel()->getNode()->connectWidget(L"freehand:width:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &DrawFreehandModule::stepWidth, true);
    getPanel()->getNode()->connectWidget(L"freehand:simplification", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &DrawFreehandModule::editSimplification, true);
    getPanel()->getNode()->connectWidget(L"freehand:simplification:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &DrawFreehandModule::dragSimplification, true);
    getPanel()->getNode()->connectWidget(L"freehand:simplification:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &DrawFreehandModule::stepSimplification, true);
    getPanel()->getNode()->connectWidget(L"freehand:simplification:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &DrawFreehandModule::stepSimplification, true);
    getPanel()->getNode()->connectWidget(L"freehand:pressure:stroke", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &DrawFreehandModule::choosePressure, true);
    getPanel()->getNode()->connectWidget(L"freehand:pressure:stroke", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &DrawFreehandModule::choosePressure, true);
    getPanel()->getNode()->connectWidget(L"freehand:pressure:opacity", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &DrawFreehandModule::choosePressure, true);
    getPanel()->getNode()->connectWidget(L"freehand:pressure:opacity", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &DrawFreehandModule::choosePressure, true);
    getPanel()->getNode()->connectWidget(L"freehand:corners", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &DrawFreehandModule::chooseCorners, true);
    getPanel()->getNode()->connectWidget(L"freehand:corners", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &DrawFreehandModule::chooseCorners, true);
    updateWidth();
    updateSimplification();
    fitWidth(10);
    fitHeight(10);
    createZoomSequences();
}

void DrawFreehandModule::choosePressure()
{
    m_pressureStroke = m_panel->isChecked(L"freehand:pressure:stroke", true);
    m_pressureOpacity = m_panel->isChecked(L"freehand:pressure:opacity", true);
}

void DrawFreehandModule::chooseCorners()
{
    m_corners = m_panel->isChecked(L"freehand:corners", true);
}

void DrawFreehandModule::setWidth(float width)
{
    m_width = width;
    updateWidth();
}

void DrawFreehandModule::updateWidth()
{
    if (m_width < 0.0f)
        m_width = 0.0f;
    std::wstringstream text;
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_width));
    m_panel->setText(L"freehand:width", text.str(), true);
    float stroke = m_workspace->m_strokeWidthModule->m_width;
    if (stroke != m_width)
        m_workspace->m_strokeWidthModule->setWidth(m_width);
}

void DrawFreehandModule::editWidth()
{
    m_width = m_workspace->toDesignSheetInternalUnits(
        (float)atof(narrowString(m_workspace->getGuiNode()->getText(L"freehand:width", true)).c_str()));
    updateWidth();
}

void DrawFreehandModule::dragWidth(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        m_width = m_workspace->toDesignSheetInternalUnits(getDragMovement()) + m_width;
        updateWidth();
    }
}

void DrawFreehandModule::stepWidth(plasma::Widget *sender)
{
    m_width = m_workspace->toDesignSheetInternalUnits((float)pressDragButton(sender)) + m_width;
    updateWidth();
}

void DrawFreehandModule::updateSimplification()
{
    if (m_simplification < 0.0f)
        m_simplification = 0.0f;
    if (m_simplification > 100.0f)
        m_simplification = 100.0f;
    std::wstringstream text;
    text << roundToHundredths(m_simplification);
    m_panel->setText(L"freehand:simplification", text.str(), true);
}

void DrawFreehandModule::editSimplification()
{
    // The code also converts the simplification percentage through the page unit.
    m_simplification = m_workspace->toDesignSheetInternalUnits(
        (float)atof(narrowString(m_workspace->getGuiNode()->getText(L"freehand:simplification", true)).c_str()));
    updateSimplification();
}

void DrawFreehandModule::dragSimplification(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        m_simplification = getDragMovement() + m_simplification;
        updateSimplification();
    }
}

void DrawFreehandModule::stepSimplification(plasma::Widget *sender)
{
    m_simplification = (double)pressDragButton(sender) + m_simplification;
    updateSimplification();
}

void DrawFreehandModule::unitChanged()
{
    updateWidth();
}
