#include "GridModule.h"
#include "DesignSheet.h"
#include "plasma/GenericWidget.h"
#include <sstream>
#include <cstdlib>
#include "FormatModule.h" // The edit box filters.

std::string narrowString(const std::wstring &text);

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
GridModule::GridModule()
    : m_enabled(false),
      m_moveX(true),
      m_moveY(true)
{
    {
        std::wstring name(L"module:grid");
        std::wstring kind(L"toolbar");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 125.0f, 35.0f, g_workspace.m_scalableNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_orientation = ORIENTATION_HORIZONTAL;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void GridModule::build()
{
    m_cursor = plasma::Vector<2, float>(2.0f, 2.0f);
    m_margin = m_cursor;
    std::list<ModuleWidgetItem> items;
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"grid:enabled", L"checkbutton2", 0, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"grid:size", L"editbox", 25, 0, 50, 25));
    items.push_back(ModuleWidgetItem(L" ", L"grid:size:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 75, 0,
                                     20, 25));
    items.push_back(ModuleWidgetItem(L"\u00bd", L"grid:size:decrease", L"button2", 100, 0, 25, 25)); // One half.
    items.push_back(ModuleWidgetItem(L"X2", L"grid:size:increase", L"button2", 125, 0, 25, 25));
    addPanel(items, L"", 0);
    m_cursor.m_elements[1] += 3;
    addWidget(L"px", L"text:unit", L"static", 20, 20, 0);
    m_cursor.m_elements[1] -= 3;
    placeIcon(L"grid:enabled", L"icon:grid");
    getPanel()->getNode()->setEditFilter(L"grid:size", &plasma::g_unsignedFloatFilter, true);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"grid:move:xy", L"radiobutton2", 0, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"grid:move:x", L"radiobutton2", 25, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"grid:move:y", L"radiobutton2", 50, 0, 25, 25));
    addPanel(items, L"", 3);
    m_panel->setChecked(L"grid:move:xy", true, false, true);
    placeIcon(L"grid:move:xy", L"icon:constrain:xy");
    placeIcon(L"grid:move:x", L"icon:constrain:x");
    placeIcon(L"grid:move:y", L"icon:constrain:y");
    fitWidth(3);
    getPanel()->getNode()->connectWidget(L"grid:size", plasma::WIDGET_EVENT_FOCUS_LOST, this, &GridModule::editSize,
                                         true);
    getPanel()->getNode()->connectWidget(L"grid:size:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &GridModule::dragSize, true);
    getPanel()->getNode()->connectWidget(L"grid:size:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &GridModule::stepSize, true);
    getPanel()->getNode()->connectWidget(L"grid:size:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &GridModule::stepSize, true);
    getPanel()->getNode()->connectWidget(L"grid:size:decrease", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &GridModule::halveSize, true);
    getPanel()->getNode()->connectWidget(L"grid:size:decrease", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &GridModule::halveSize, true);
    getPanel()->getNode()->connectWidget(L"grid:size:decrease", plasma::WIDGET_EVENT_REPEAT, this,
                                         &GridModule::halveSize, true);
    getPanel()->getNode()->connectWidget(L"grid:size:increase", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &GridModule::doubleSize, true);
    getPanel()->getNode()->connectWidget(L"grid:size:increase", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &GridModule::doubleSize, true);
    getPanel()->getNode()->connectWidget(L"grid:size:increase", plasma::WIDGET_EVENT_REPEAT, this,
                                         &GridModule::doubleSize, true);
    getPanel()->getNode()->connectWidget(L"grid:enabled", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &GridModule::chooseEnabled, true);
    getPanel()->getNode()->connectWidget(L"grid:enabled", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &GridModule::chooseEnabled, true);
    getPanel()->getNode()->connectWidget(L"grid:move:xy", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &GridModule::chooseMoveXY, true);
    getPanel()->getNode()->connectWidget(L"grid:move:x", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &GridModule::chooseMoveX, true);
    getPanel()->getNode()->connectWidget(L"grid:move:y", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &GridModule::chooseMoveY, true);
    getPanel()->getNode()->setText(L"grid:size", L"", true);
    createZoomSequences();
    chooseEnabled();
}

void GridModule::chooseEnabled()
{
    m_enabled = m_panel->isChecked(L"grid:enabled", true);
}

void GridModule::editSize()
{
    if (m_workspace->getOpenSheet()) {
        m_workspace->getOpenSheet()->m_gridSize = m_workspace->toDesignSheetInternalUnits(
            (float)atof(narrowString(m_workspace->getGuiNode()->getText(L"grid:size", true)).c_str()));
        if (m_workspace->getOpenSheet()->m_gridSize < 0.0f)
            m_workspace->getOpenSheet()->m_gridSize = 0.0f;
        updateSize();
    }
}

void GridModule::dragSize(plasma::Widget *sender)
{
    if (m_workspace->getOpenSheet() && isDragging(sender)) {
        updateDrag();
        m_workspace->getOpenSheet()->m_gridSize = m_workspace->toDesignSheetInternalUnits(m_dragPosition) + m_dragStart;
        updateSize();
    }
}

void GridModule::stepSize(plasma::Widget *sender)
{
    if (m_workspace->getOpenSheet()) {
        m_workspace->getOpenSheet()->m_gridSize +=
            m_workspace->toDesignSheetInternalUnits((float)pressDragButton(sender));
        updateSize();
        beginDrag(m_workspace->getOpenSheet()->m_gridSize);
    }
}

void GridModule::halveSize()
{
    if (m_workspace->getOpenSheet()) {
        // The code makes this unused conversion before converting again.
        m_workspace->fromDesignSheetInternalUnits(m_workspace->getOpenSheet()->m_gridSize);
        m_workspace->getOpenSheet()->m_gridSize = m_workspace->toDesignSheetInternalUnits(
            m_workspace->fromDesignSheetInternalUnits(m_workspace->getOpenSheet()->m_gridSize) * 0.5f);
        updateSize();
    }
}

void GridModule::doubleSize()
{
    if (m_workspace->getOpenSheet()) {
        m_workspace->getOpenSheet()->m_gridSize = m_workspace->toDesignSheetInternalUnits(
            m_workspace->fromDesignSheetInternalUnits(m_workspace->getOpenSheet()->m_gridSize) * 2.0f);
        updateSize();
    }
}

void GridModule::chooseMoveXY()
{
    m_moveX = true;
    m_moveY = true;
}

void GridModule::chooseMoveX()
{
    m_moveX = true;
    m_moveY = false;
}

void GridModule::chooseMoveY()
{
    m_moveX = false;
    m_moveY = true;
}

void GridModule::updateSize()
{
    if (!m_workspace->getOpenSheet()) {
        m_panel->setText(L"grid:size", L"", true);
    } else {
        if (m_workspace->getOpenSheet()->m_gridSize < 0.0f)
            m_workspace->getOpenSheet()->m_gridSize = 0.0f;
        std::wstringstream text;
        text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_workspace->getOpenSheet()->m_gridSize));
        m_panel->setText(L"grid:size", text.str(), true);
    }
}

void GridModule::unitChanged()
{
    updateSize();
}

void GridModule::sheetChanged()
{
    updateSize();
}
