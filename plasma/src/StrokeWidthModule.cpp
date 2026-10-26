#include "StrokeWidthModule.h"
#include "DesignSheet.h"
#include "DrawFreehandModule.h"
#include "plasma/TextShape.h"
#include "plasma/GenericWidget.h"
#include <sstream>
#include <cstdlib>
#include "FormatModule.h" // The edit box filters.

std::string narrowString(const std::wstring &text);

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
StrokeWidthModule::StrokeWidthModule()
    : m_width(10.0f)
{
    {
        std::wstring name(L"module:stroke:width");
        std::wstring kind(L"bar");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 90.0f, 70.0f, 120.0f, 100.0f,
                                                           g_workspace.m_guiLeftNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Stroke width", true);
    m_orientation = ORIENTATION_HORIZONTAL;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void StrokeWidthModule::build()
{
    m_cursor.m_elements[0] = 50;
    m_cursor.m_elements[1] = 15;
    std::list<ModuleWidgetItem> items;
    addWidget(L"Stroke width:", L"", L"static", 70, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"stroke:width", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"stroke:width:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    addWidget(L"px", L"text:unit", L"static", 20, 25, 0);
    getPanel()->getNode()->setEditFilter(L"stroke:width", &plasma::g_floatFilter, true);
    m_cursor.m_elements[1] = 10;
    getPanel()->getNode()->connectWidget(L"stroke:width", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &StrokeWidthModule::editWidth, true);
    getPanel()->getNode()->connectWidget(L"stroke:width:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &StrokeWidthModule::dragWidth, true);
    getPanel()->getNode()->connectWidget(L"stroke:width:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &StrokeWidthModule::stepWidth, true);
    getPanel()->getNode()->connectWidget(L"stroke:width:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &StrokeWidthModule::stepWidth, true);
    updateWidth();
    fitWidth(10);
    fitHeight(10);
    createZoomSequences();
}

void StrokeWidthModule::setWidth(float width)
{
    m_width = width;
    updateWidth();
}

void StrokeWidthModule::updateWidth()
{
    if (m_width < 0.0f)
        m_width = 0.0f;
    std::wstringstream text;
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_width));
    m_panel->setText(L"stroke:width", text.str(), true);
    float width = m_workspace->m_drawFreehandModule->m_width;
    if (width != m_width)
        m_workspace->m_drawFreehandModule->setWidth(m_width);
}

void StrokeWidthModule::editWidth()
{
    m_width = m_workspace->toDesignSheetInternalUnits(
        (float)atof(narrowString(m_workspace->getGuiNode()->getText(L"stroke:width", true)).c_str()));
    updateWidth();
}

void StrokeWidthModule::dragWidth(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        m_width = m_workspace->toDesignSheetInternalUnits(getDragMovement()) + m_width;
        updateWidth();
    }
}

void StrokeWidthModule::stepWidth(plasma::Widget *sender)
{
    m_width = m_workspace->toDesignSheetInternalUnits((float)pressDragButton(sender)) + m_width;
    updateWidth();
}

void StrokeWidthModule::unitChanged()
{
    updateWidth();
}

void plasma::TextShape::setHorizontalAlignment(int alignment)
{
    // Each case leaves the other alignment bit as it was.
    switch (alignment) {
    case ALIGN_LEFT:
        m_flags &= ~TEXT_HORIZONTAL_MASK;
        break;
    case ALIGN_RIGHT:
        m_flags = (m_flags & ~(1 << TEXT_CENTER)) | (1 << TEXT_RIGHT);
        break;
    case ALIGN_CENTER:
        m_flags = (m_flags & ~(1 << TEXT_RIGHT)) | (1 << TEXT_CENTER);
        break;
    }
}
