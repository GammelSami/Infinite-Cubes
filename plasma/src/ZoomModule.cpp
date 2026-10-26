#include "ZoomModule.h"
#include "DesignSheet.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/GenericWidget.h"
#include <sstream>
#include <cstdlib>
#include "FormatModule.h" // The edit box filters.

float roundToTenths(float value);
std::string narrowString(const std::wstring &text);

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
ZoomModule::ZoomModule()
{
    {
        std::wstring name(L"module:zoom");
        std::wstring kind(L"toolbar");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 125.0f, 35.0f, g_workspace.m_scalableNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_orientation = ORIENTATION_HORIZONTAL;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void ZoomModule::build()
{
    m_cursor = plasma::Vector<2, float>(2.0f, 5.0f);
    m_margin = m_cursor;
    std::list<ModuleWidgetItem> items;
    addWidget(L"Zoom:", L"", L"static", 40, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"zoom:zoom", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"zoom:zoom:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    addWidget(L"%", L"", L"static", 20, 20, 0);
    m_cursor.m_elements[1] = 2;
    items.clear();
    items.push_back(ModuleWidgetItem(L"100%", L"zoom:reset", L"button2", 0, 0, 35, 25));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"zoom:zoom", &plasma::g_unsignedFloatFilter, true);
    fitWidth(3);
    getPanel()->getNode()->connectWidget(L"zoom:zoom", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ZoomModule::editZoom,
                                         true);
    getPanel()->getNode()->connectWidget(L"zoom:zoom:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ZoomModule::dragZoom, true);
    getPanel()->getNode()->connectWidget(L"zoom:zoom:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ZoomModule::stepZoom, true);
    getPanel()->getNode()->connectWidget(L"zoom:zoom:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ZoomModule::stepZoom, true);
    getPanel()->getNode()->connectWidget(L"zoom:reset", plasma::WIDGET_EVENT_POINTER_PRESSED, this, &ZoomModule::reset,
                                         true);
    getPanel()->getNode()->setText(L"zoom:zoom", L"100", true);
    createZoomSequences();
}

void ZoomModule::reset()
{
    m_workspace->zoomView(1.0f, true);
}

void ZoomModule::editZoom()
{
    if (m_workspace->getOpenSheet()) {
        float zoom = atof(narrowString(m_workspace->getGuiNode()->getText(L"zoom:zoom", true)).c_str()) * 0.01f;
        if (zoom < 0.0f)
            zoom = 0.0f;
        m_workspace->zoomView(zoom, true);
    }
}

void ZoomModule::dragZoom(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        float zoom = atof(narrowString(m_workspace->getGuiNode()->getText(L"zoom:zoom", true)).c_str()) * 0.01f;
        zoom = getDragMovement() * 0.01f + zoom;
        m_workspace->zoomView(zoom, true);
    }
}

void ZoomModule::stepZoom(plasma::Widget *sender)
{
    float zoom = atof(narrowString(m_workspace->getGuiNode()->getText(L"zoom:zoom", true)).c_str()) * 0.01f;
    zoom = (double)pressDragButton(sender) * 0.1f + zoom;
    m_workspace->zoomView(zoom, true);
}

void ZoomModule::sheetChanged()
{
    update();
}

void ZoomModule::update()
{
    if (!m_workspace->getOpenSheet()) {
        m_panel->setText(L"zoom:zoom", L"100", true);
    } else {
        float zoom = m_workspace->getOpenSheet()->m_root->m_transformation->m_deformation.value().m_elements[0] *
                     100.0f / m_workspace->getOpenSheet()->getZoomScale();
        std::wstringstream text;
        text << roundToTenths(zoom);
        m_panel->setText(L"zoom:zoom", text.str(), true);
    }
}
