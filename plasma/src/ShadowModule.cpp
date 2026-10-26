#include "ShadowModule.h"
#include "ToolModule.h"
#include "OnlineModule.h"
#include "TextModule.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "FormatModule.h"
std::string narrowString(const std::wstring &text);
#include <sstream>
#include <cstdlib>

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
ShadowModule::ShadowModule()
    : m_offsetX(10.0f),
      m_offsetY(10.0f),
      m_blur(10.0f),
      m_color(0.0f, 0.0f, 0.0f, 0.5f)
{
    {
        std::wstring name(L"module:procedural");
        std::wstring kind(L"sidewidget");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 300.0f, 140.0f, 120.0f, 100.0f, g_workspace.m_guiNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Drop Shadow", true);
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, plasma::DISPLAY_INVISIBLE);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void ShadowModule::build()
{
    std::list<ModuleWidgetItem> items;
    newRow(3);
    addWidget(L"Offset X:", L"", L"static", 50, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"shadow:offset:x", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"shadow:offset:x:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"shadow:offset:x", &plasma::g_floatFilter, true);
    addWidget(L"px", L"text:unit", L"static", 10, 20, 0);
    newRow(3);
    addWidget(L"Offset Y:", L"", L"static", 50, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"shadow:offset:y", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"shadow:offset:y:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"shadow:offset:y", &plasma::g_floatFilter, true);
    addWidget(L"px", L"text:unit", L"static", 10, 20, 0);
    newRow(3);
    addWidget(L"Blur:", L"", L"static", 50, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"shadow:blur", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"shadow:blur:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"shadow:blur", &plasma::g_unsignedFloatFilter, true);
    addWidget(L"px", L"text:unit", L"static", 10, 20, 0);
    newRow(3);
    addWidget(L"Color:", L"", L"static", 50, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"shadow:color", L"colorbutton2", 0, 0, 70, 70));
    addPanel(items, L"", 0);
    placeIcon(L"shadow:color", L"icon:pick");
    newRow(3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"shadow:ok", L"button2", 0, 0, 70, 30));
    items.push_back(ModuleWidgetItem(L" ", L"shadow:cancel", L"button2", 70, 0, 70, 30));
    addPanel(items, L"", 0);
    placeIcon(L"shadow:ok", L"icon:apply:big");
    placeIcon(L"shadow:cancel", L"icon:cancel");
    fitWidth(10);
    fitHeight(10);
    m_colorPanel = m_workspace->m_engine.findNode(L"shadow:color:colorpanel");
    if (m_colorPanel)
        m_colorPanel->setDisplay(m_colorPanel->m_display->clone());
    getPanel()->getNode()->connectWidget(L"shadow:offset:x", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ShadowModule::editOffsetX, true);
    getPanel()->getNode()->connectWidget(L"shadow:offset:x:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ShadowModule::dragOffsetX, true);
    getPanel()->getNode()->connectWidget(L"shadow:offset:x:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ShadowModule::stepOffsetX, true);
    getPanel()->getNode()->connectWidget(L"shadow:offset:x:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ShadowModule::stepOffsetX, true);
    getPanel()->getNode()->connectWidget(L"shadow:offset:y", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ShadowModule::editOffsetY, true);
    getPanel()->getNode()->connectWidget(L"shadow:offset:y:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ShadowModule::dragOffsetY, true);
    getPanel()->getNode()->connectWidget(L"shadow:offset:y:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ShadowModule::stepOffsetY, true);
    getPanel()->getNode()->connectWidget(L"shadow:offset:y:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ShadowModule::stepOffsetY, true);
    getPanel()->getNode()->connectWidget(L"shadow:blur", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ShadowModule::editBlur,
                                         true);
    getPanel()->getNode()->connectWidget(L"shadow:blur:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ShadowModule::dragBlur, true);
    getPanel()->getNode()->connectWidget(L"shadow:blur:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ShadowModule::stepBlur, true);
    getPanel()->getNode()->connectWidget(L"shadow:blur:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ShadowModule::stepBlur, true);
    getPanel()->getNode()->connectWidget(L"shadow:color", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ShadowModule::chooseShadowColor, true);
    getPanel()->getNode()->connectWidget(L"shadow:ok", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ShadowModule::apply, true);
    getPanel()->getNode()->connectWidget(L"shadow:cancel", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ShadowModule::close, true);
    std::wstringstream text;
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_offsetX));
    m_panel->setText(L"shadow:offset:x", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_offsetY));
    m_panel->setText(L"shadow:offset:y", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_blur));
    m_panel->setText(L"shadow:blur", text.str(), true);
    if (m_colorPanel)
        m_colorPanel->display()->m_fillColor = m_color;
}

void ShadowModule::unitChanged()
{
    std::wstringstream text;
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_offsetX));
    m_panel->setText(L"shadow:offset:x", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_offsetY));
    m_panel->setText(L"shadow:offset:y", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_blur));
    m_panel->setText(L"shadow:blur", text.str(), true);
}

void ShadowModule::editOffsetX()
{
    float value; // The extraction leaves this uninitialized when parsing fails.
    std::wstringstream(m_workspace->getGuiNode()->getText(L"shadow:offset:x", true)) >> value;
    m_offsetX = m_workspace->toDesignSheetInternalUnits(value);
}

void ShadowModule::dragOffsetX(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_offsetX = m_workspace->toDesignSheetInternalUnits(m_dragPosition) + m_dragStart;
        std::wstringstream text;
        text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_offsetX));
        m_panel->setText(L"shadow:offset:x", text.str(), true);
    }
}

void ShadowModule::stepOffsetX(plasma::Widget *sender)
{
    float value = m_workspace->toDesignSheetInternalUnits((float)pressDragButton(sender)) + m_offsetX;
    m_offsetX = value;
    beginDrag(value);
    std::wstringstream text;
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_offsetX));
    m_panel->setText(L"shadow:offset:x", text.str(), true);
}

void ShadowModule::editOffsetY()
{
    float value; // The extraction leaves this uninitialized when parsing fails.
    std::wstringstream(m_workspace->getGuiNode()->getText(L"shadow:offset:y", true)) >> value;
    m_offsetY = m_workspace->toDesignSheetInternalUnits(value);
}

void ShadowModule::dragOffsetY(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_offsetY = m_workspace->toDesignSheetInternalUnits(m_dragPosition) + m_dragStart;
        std::wstringstream text;
        text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_offsetY));
        m_panel->setText(L"shadow:offset:y", text.str(), true);
    }
}

void ShadowModule::stepOffsetY(plasma::Widget *sender)
{
    float value = m_workspace->toDesignSheetInternalUnits((float)pressDragButton(sender)) + m_offsetY;
    m_offsetY = value;
    beginDrag(value);
    std::wstringstream text;
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_offsetY));
    m_panel->setText(L"shadow:offset:y", text.str(), true);
}

void ShadowModule::editBlur()
{
    float value; // The extraction leaves this uninitialized when parsing fails.
    std::wstringstream(m_workspace->getGuiNode()->getText(L"shadow:blur", true)) >> value;
    m_blur = m_workspace->toDesignSheetInternalUnits(value);
}

void ShadowModule::dragBlur(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_blur = m_workspace->toDesignSheetInternalUnits(m_dragPosition) + m_dragStart;
        if (m_blur < 0.0f) {
            m_blur = 0.0f;
            beginDrag(0.0f);
        }
        std::wstringstream text;
        text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_blur));
        m_panel->setText(L"shadow:blur", text.str(), true);
    }
}

void ShadowModule::stepBlur(plasma::Widget *sender)
{
    m_blur = m_workspace->toDesignSheetInternalUnits((float)pressDragButton(sender)) + m_blur;
    if (m_blur < 0.0f)
        m_blur = 0.0f;
    beginDrag(m_blur);
    std::wstringstream text;
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_blur));
    m_panel->setText(L"shadow:blur", text.str(), true);
}

void ShadowModule::chooseShadowColor()
{
    m_color = m_workspace->m_colorModule->getColor();
    if (m_colorPanel) {
        m_colorPanel->m_display->m_fillColor.value() = m_color;
        m_colorPanel->m_display->m_changed = true;
    }
}

void ShadowModule::apply()
{
    m_workspace->m_toolModule->m_nodeTool.dropToShadow();
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, plasma::DISPLAY_INVISIBLE);
}

void ShadowModule::close()
{
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, plasma::DISPLAY_INVISIBLE);
}
