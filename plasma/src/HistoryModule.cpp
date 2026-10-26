#include "HistoryModule.h"
#include "DesignSheet.h"
#include "plasma/GenericWidget.h"

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
HistoryModule::HistoryModule()
{
    // Release both factory strings before changing the panel flags.
    {
        std::wstring name(L"module:history");
        std::wstring kind(L"toolbar");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 55.0f, 35.0f, g_workspace.m_scalableNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void HistoryModule::build()
{
    std::list<ModuleWidgetItem> items;
    items.push_back(ModuleWidgetItem(L" ", L"history:undo", L"button2", 0, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"history:redo", L"button2", 25, 0, 25, 25));
    addPanelAt(items, L"", 3, 3);
    getPanel()->getNode()->connectWidget(L"history:undo", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &HistoryModule::undo, true);
    getPanel()->getNode()->connectWidget(L"history:undo", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &HistoryModule::undo, true);
    getPanel()->getNode()->connectWidget(L"history:redo", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &HistoryModule::redo, true);
    getPanel()->getNode()->connectWidget(L"history:redo", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &HistoryModule::redo, true);
    placeIcon(L"history:undo", L"icon:undo");
    placeIcon(L"history:redo", L"icon:redo");
    createZoomSequences();
}

void HistoryModule::undo()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->getOpenDesignSheet()->undo();
        m_workspace->updateModules();
        m_workspace->refreshPointer();
    }
}

void HistoryModule::redo()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->getOpenDesignSheet()->redo();
        m_workspace->updateModules();
        m_workspace->refreshPointer();
    }
}

// Widget connection template instantiations used by Build.
