#include "ProgressModule.h"
#include "plasma/GenericWidget.h"

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void ProgressModule::build()
{
    addWidgetAt(ModuleWidgetItem(L"Progress", L"progress:text", L"title", 190, 10, 100, 20));
    {
        std::wstring name(L"progress:bar");
        std::wstring shape(L"progressbar");
        m_bar = m_workspace->m_engine.createGenericWidget(name, 10.0f, 70.0f, m_panel->viewWidth() - 20.0, 50.0f,
                                                          getPanel()->getNode(), shape);
    }
    m_workspace->m_engine.resolveSources(m_bar->m_node);
    m_bar->m_node->initializeWidgets();
    m_bar->m_node->animate(0);
}

void ProgressModule::advance(float amount)
{
    setProgress(m_progress + amount);
}

void ProgressModule::setText(const std::wstring &text)
{
    m_panel->setText(L"progress:text", text, true);
}
