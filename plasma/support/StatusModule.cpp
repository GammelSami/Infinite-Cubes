// Status panel support. Layout positions, extents and spacing use display units; colour tuples style the overlays.
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

std::wstring widenString(const std::string &text);

StatusModule::StatusModule()
{
    {
        std::wstring name(L"module:status");
        std::wstring kind(L"statuspanel");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 400.0f, 120.0f, g_workspace.m_guiNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Status", true);
    getPanel()->getNode()->m_flags |= (1 << plasma::NODE_IGNORES_POINTER);
    pugi::xml_document document;
    if (document.load_file("tooltips_en.xml")) {
        pugi::xml_node root = document.first_child();
        for (pugi::xml_node_iterator it = root.begin(); it != root.end(); it++) {
            std::string key = (*it).attribute("key").value();
            std::string icon = (*it).attribute("icon").value();
            std::string annotation = (*it).attribute("annotation").value();
            if (key.size()) {
                std::wstring title = getXmlText((*it).child("title"));
                std::wstring text = getXmlText((*it).child("text"));
                if (m_tooltips[key])
                    delete m_tooltips[key];
                m_tooltips[key] = new StatusInfo(title, text, icon, widenString(annotation), L"");
            }
        }
    }
    m_tooltips["updatebutton"] = new StatusInfo(L"Update", L"A new version of Plasma is available.", "icon:reload",
                                                L"Click to update Plasma", L"");
}
