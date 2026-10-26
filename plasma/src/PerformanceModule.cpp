#include "PerformanceModule.h"
#include "plasma/GenericWidget.h"

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
PerformanceModule::PerformanceModule()
    : m_blurMode(BLUR_AFTERWARDS),
      m_realtimeUpdate(true)
{
    {
        std::wstring name(L"module:performance");
        std::wstring kind(L"toolbar");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 120.0f, 30.0f, g_workspace.m_scalableNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void PerformanceModule::build()
{
    m_cursor = plasma::Vector<2, float>(2.0f, 2.0f);
    m_margin = m_cursor;
    m_margin[1] = m_cursor[1] = 5;
    addWidget(L"FX:", L"", L"static", 20, 20, 0);
    std::list<ModuleWidgetItem> items;
    items.clear();
    items.push_back(ModuleWidgetItem(L"1", L"performance:blur:disabled", L"radiobutton2", 0, 0, 20, 20));
    items.push_back(ModuleWidgetItem(L"2", L"performance:blur:afterwards", L"radiobutton2", 20, 0, 20, 20));
    items.push_back(ModuleWidgetItem(L"3", L"performance:blur:realtime", L"radiobutton2", 40, 0, 20, 20));
    addPanel(items, L"", 3);
    items.clear();
    items.push_back(ModuleWidgetItem(L"RT", L"performance:realtime:update", L"checkbutton2", 0, 0, 30, 20));
    addPanel(items, L"", 3);
    fitWidth(3);
    fitHeight(10);
    createZoomSequences();
    switch (m_blurMode) {
    case BLUR_DISABLED:
        m_panel->setChecked(L"performance:blur:disabled", true, false, true);
        break;
    case BLUR_AFTERWARDS:
        m_panel->setChecked(L"performance:blur:afterwards", true, false, true);
        break;
    case BLUR_REALTIME:
        m_panel->setChecked(L"performance:blur:realtime", true, false, true);
        break;
    }
    getPanel()->getNode()->setChecked(L"performance:realtime:update", m_realtimeUpdate, false, true);
    getPanel()->getNode()->connectWidget(L"performance:blur:disabled", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &PerformanceModule::disableBlur, true);
    getPanel()->getNode()->connectWidget(L"performance:blur:afterwards", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &PerformanceModule::blurAfterwards, true);
    getPanel()->getNode()->connectWidget(L"performance:blur:realtime", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &PerformanceModule::blurRealtime, true);
    getPanel()->getNode()->connectWidget(L"performance:realtime:update", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &PerformanceModule::chooseRealtimeUpdate, true);
    getPanel()->getNode()->connectWidget(L"performance:realtime:update", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &PerformanceModule::chooseRealtimeUpdate, true);
}

void PerformanceModule::disableBlur()
{
    m_blurMode = BLUR_DISABLED;
    m_workspace->markSheetsChanged();
}

void PerformanceModule::blurAfterwards()
{
    m_blurMode = BLUR_AFTERWARDS;
    m_workspace->markSheetsChanged();
}

void PerformanceModule::blurRealtime()
{
    m_blurMode = BLUR_REALTIME;
    m_workspace->markSheetsChanged();
}

void PerformanceModule::chooseRealtimeUpdate()
{
    m_realtimeUpdate = m_panel->isChecked(L"performance:realtime:update", true);
}
