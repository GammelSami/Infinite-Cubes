#include "AboutModule.h"
#include "plasma/GenericWidget.h"
#include <sstream>

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
AboutModule::AboutModule()
{
    {
        std::wstring name(L"module:about");
        std::wstring kind(L"sidewidget");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 300.0f, 140.0f, 120.0f, 100.0f, g_workspace.m_guiNode, kind);
    }
    m_panel->setCaption(L"About Picroma Plasma", true);
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, 0);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void AboutModule::build()
{
    std::list<ModuleWidgetItem> items;
    addWidgetAt(ModuleWidgetItem(L" ", L"about:icon", L"static", 20, 40, 100, 100));
    placeIcon(L"about:icon", L"icon:logo");
    m_cursor.m_elements[1] = 40;
    addWidget(L"Picroma Plasma", L"", L"static", 160, 20, 150);
    newRow(3);
    std::wstringstream version;
    version << L"Version " << 1 << L"." << 0 << L"." << 1 << std::endl;
    addWidget(version.str(), L"", L"static", 160, 20, 150);
    newRow(3);
    std::wstring copyright(L" ");
    copyright[0] = L'\u00a9';
    addWidget(copyright + L" 2011 Picroma e.K.", L"", L"static", 160, 20, 150);
    newRow(40);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"about:ok", L"button2", 0, 0, 80, 30));
    addPanel(items, L"", 120);
    placeIcon(L"about:ok", L"icon:apply:big");
    m_panel->getNode()->connectWidget(L"about:ok", plasma::WIDGET_EVENT_POINTER_RELEASED, this, &AboutModule::close,
                                      true);
    fitWidth(10);
    fitHeight(10);
}

void AboutModule::close()
{
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, 0);
}
