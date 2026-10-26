// Builds the progress panel and updates its status text.
#include "SpecialColorModule.h"
#include "AboutModule.h"
#include "StatusModule.h"
#include "ProgressModule.h"
#include "PaletteWidget.h"
#include "ColorPickerWidget.h"
#include "FontWidget.h"
#include "PaletteModule.h"
#include "ColorModule.h"
#include "TextModule.h"
#include "plasma/ScalableFont.h"
#include <GL/glew.h>

#include "plasma/GenericWidget.h"
#include "plasma/ListWidget.h"
#include "plasma/Transformation.h"
#include "main.h"
#include "../vendor/pugixml-0.9/src/pugixml.hpp"
#include <sstream>

ProgressModule::ProgressModule()
    : m_progress(0.0f)
{
    {
        std::wstring name(L"module:progress");
        std::wstring kind(L"statuspanel");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 400.0f, 150.0f, g_workspace.m_guiNode, kind);
    }
    m_panel->setCaption(L"Progress", true);
    plasma::Vector<2, float> window((float)*reinterpret_cast<int *>(reinterpret_cast<char *>(&g_workspace) + 0x148),
                                    (float)*reinterpret_cast<int *>(reinterpret_cast<char *>(&g_workspace) + 0x14c));
    plasma::Vector<2, float> position = window - m_panel->getViewSize();
    position.m_elements[0] *= (double)0.5f;
    position.m_elements[1] *= (double)0.5f;
    m_panel->setViewPosition(position, true);
    m_panel->m_flags |= (1 << plasma::WIDGET_ALWAYS_HIT);
    m_panel->m_horizontalAlignment = plasma::ALIGN_CENTER;
    m_panel->m_verticalAlignment = plasma::ALIGN_MIDDLE;
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, 0);
}

void ProgressModule::setProgress(float progress)
{
    m_progress = progress;
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, progress < 1.0f);
    if (m_progress < 1.0f) {
        plasma::Widget *status = m_workspace->m_statusModule->getPanel();
        plasma::Widget::setDisplayVisibility(status->getNode()->m_display, 0);
    }
    if (m_progress > 1.0f)
        m_progress = 1.0f;
    m_bar->resizeTo((getPanel()->viewWidth() - (double)20.0f) * m_progress, m_bar->getViewHeight());
    m_panel->m_changed = true;
}
