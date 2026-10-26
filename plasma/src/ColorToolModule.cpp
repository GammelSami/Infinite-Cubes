#include "ColorToolModule.h"
#include "plasma/GenericWidget.h"
#include <cstdlib>
#include <sstream>
#include "FormatModule.h" // The edit box filters.

std::string narrowString(const std::wstring &text);

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
ColorToolModule::ColorToolModule()
    : m_colorize(false),
      m_color(true),
      m_alpha(true),
      m_tolerance(0.25f)
{
    {
        std::wstring name(L"module:color:tool");
        std::wstring kind(L"bar");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 90.0f, 70.0f, 120.0f, 100.0f,
                                                           g_workspace.m_guiLeftNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Paint", true);
    m_orientation = ORIENTATION_HORIZONTAL;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void ColorToolModule::build()
{
    std::list<ModuleWidgetItem> items;
    m_cursor.m_elements[0] = 50;
    m_cursor.m_elements[1] = 10;
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"color:tool:coloralpha", L"radiobutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"color:tool:color", L"radiobutton2", 30, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"color:tool:alpha", L"radiobutton2", 60, 0, 30, 30));
    addPanel(items, L"", 3);
    placeIcon(L"color:tool:coloralpha", L"icon:color:coloralpha");
    placeIcon(L"color:tool:color", L"icon:color:color");
    placeIcon(L"color:tool:alpha", L"icon:color:alpha");
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"color:tool:colorize", L"checkbutton2", 0, 0, 30, 30));
    addPanel(items, L"", 3);
    placeIcon(L"color:tool:colorize", L"icon:color:colorize");
    m_cursor.m_elements[1] += 5;
    addWidget(L"Tolerance:", L"", L"static", 55, 20, 3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"color:tool:tolerance", L"editbox", 0, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"color:tool:tolerance:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 40, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    addWidget(L"%", L"", L"static", 15, 20, 3);
    getPanel()->getNode()->setEditFilter(L"color:tool:tolerance", &plasma::g_unsignedFloatFilter, true);
    m_panel->setChecked(L"color:tool:coloralpha", true, false, true);
    getPanel()->getNode()->connectWidget(L"color:tool:coloralpha", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorToolModule::chooseColorAlpha, true);
    getPanel()->getNode()->connectWidget(L"color:tool:color", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorToolModule::chooseColorOnly, true);
    getPanel()->getNode()->connectWidget(L"color:tool:alpha", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorToolModule::chooseAlpha, true);
    getPanel()->getNode()->connectWidget(L"color:tool:colorize", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorToolModule::chooseColorize, true);
    getPanel()->getNode()->connectWidget(L"color:tool:colorize", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ColorToolModule::chooseColorize, true);
    getPanel()->getNode()->connectWidget(L"color:tool:tolerance", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ColorToolModule::editTolerance, true);
    getPanel()->getNode()->connectWidget(L"color:tool:tolerance:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ColorToolModule::dragTolerance, true);
    getPanel()->getNode()->connectWidget(L"color:tool:tolerance:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ColorToolModule::editTolerance, true);
    getPanel()->getNode()->connectWidget(L"color:tool:tolerance:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorToolModule::stepTolerance, true);
    getPanel()->getNode()->connectWidget(L"color:tool:tolerance:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &ColorToolModule::stepTolerance, true);
    fitWidth(10);
    fitHeight(10);
    updateTolerance();
    createZoomSequences();
}

void ColorToolModule::chooseColorAlpha()
{
    m_color = true;
    m_alpha = true;
}

void ColorToolModule::chooseColorOnly()
{
    m_color = true;
    m_alpha = false;
}

void ColorToolModule::chooseAlpha()
{
    m_color = false;
    m_alpha = true;
}

void ColorToolModule::chooseColorize()
{
    m_colorize = m_panel->isChecked(L"color:tool:colorize", true);
}

void ColorToolModule::editTolerance()
{
    m_tolerance = atof(narrowString(m_workspace->getGuiNode()->getText(L"color:tool:tolerance", true)).c_str()) * 0.01f;
    updateTolerance();
}

void ColorToolModule::dragTolerance(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        m_tolerance = getDragMovement() * (double)0.01f + m_tolerance;
        updateTolerance();
    }
}

void ColorToolModule::stepTolerance(plasma::Widget *sender)
{
    m_tolerance = (double)pressDragButton(sender) * 0.01f + m_tolerance;
    updateTolerance();
}

void ColorToolModule::updateTolerance()
{
    if (m_tolerance < 0.0f)
        m_tolerance = 0.0f;
    if (m_tolerance > 1.0f)
        m_tolerance = 1.0f;
    std::wstringstream text;
    text << m_tolerance * 100.0f;
    m_panel->setText(L"color:tool:tolerance", text.str(), true);
}
