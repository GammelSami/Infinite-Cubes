#include "StatusModule.h"
#include "plasma/Node.h"
#include "plasma/Engine.h"
#include "plasma/Transformation.h"
#include "ExportModule.h" // WidenString.
#include "main.h"         // NarrowString.

StatusInfo::StatusInfo(const std::wstring &title, const std::wstring &text, const std::string &icon,
                       const std::wstring &annotation, const std::wstring &extraText)
    : m_title(title),
      m_text(text),
      m_icon(icon),
      m_annotation(annotation),
      m_extraText(extraText)
{}

StatusInfo::~StatusInfo() {}

StatusModule::~StatusModule()
{
    for (std::map<std::string, StatusInfo *>::iterator it = m_tooltips.begin(); it != m_tooltips.end(); ++it) {
        delete it->second;
    }
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void StatusModule::build()
{
    plasma::Node *background = m_workspace->m_engine.createNode(0, 0, 0, getPanel()->getNode(), L"icon:background");
    background->transformation()->m_translation.value() = plasma::Vector<2, float>(60.0f, 60.0f);
    background->transformation()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    placeIcon(L"icon:background", L"icon:tooltip:background");
    m_icon = m_workspace->m_engine.createNode(0, 0, 0, background, L"icon");
    m_icon->transformation()->m_deformation.value().identity();
    m_icon->transformation()->m_deformation.value().scale(2.5f, 2.5f);
    m_icon->transformation()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    addWidgetAt(ModuleWidgetItem(L"", L"status:name", L"title", 55, 90, 0, 0));
    std::list<plasma::TextShape *> shapes;
    getPanel()->getNode()->collectShapes<plasma::TextShape>(shapes);
    for (std::list<plasma::TextShape *>::iterator it = shapes.begin(); it != shapes.end(); ++it) {
        (*it)->setFlags((*it)->getFlags() | (1 << plasma::TEXT_WRAP));
        (*it)->m_wrapWidth = 100.0f;
    }
    plasma::Node *node = m_workspace->m_engine.createNode(0, 0, 0, getPanel()->getNode());
    m_tooltip = new StatusWidget(&m_workspace->m_engine, m_workspace, node);
}

// Shows the tooltip of the interface node under the pointer (named up to its last ':'), unless a button is held:
// title, texts and its icon.
void StatusModule::updatePanel()
{
    plasma::Engine &engine = m_workspace->m_engine;
    if (engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) || engine.isButtonDown(plasma::MOUSE_BUTTON_MIDDLE) ||
        !m_workspace->m_guiPointerNode) {
        setFadingIn(false);
        return;
    }
    std::string name = narrowString(m_workspace->m_guiPointerNode->getName());
    std::string key = name.substr(0, name.rfind(':'));
    StatusInfo *info = m_tooltips[key];
    if (info) {
        getPanel()->getNode()->setText(L"status:name", info->m_title, true);
        m_tooltip->m_text = info->m_text;
        m_tooltip->m_annotation = info->m_annotation;
        m_tooltip->m_extraText = info->m_extraText;
        m_tooltip->m_changed = true;

        // Replace the previous icon and scale the new one so that its larger extent is 80 units.
        m_icon->destroyChildren();
        getIcon()->transformation()->m_deformation.value().identity();
        m_icon->transformation()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        placeIcon(L"icon", widenString(info->m_icon));
        plasma::Vector<2, float> minimum = m_icon->getMinimum(0);
        plasma::Vector<2, float> maximum = m_icon->getMaximum(0);
        float size =
            maximum[0] - minimum[0] > maximum[1] - minimum[1] ? maximum[0] - minimum[0] : maximum[1] - minimum[1];
        if (size > 0.0f) {
            float scale = 80.0 / size;
            m_icon->transformation()->m_deformation.value().scale(scale, scale);
            m_icon->transformation()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        }
        setFadingIn(true);
    } else {
        setFadingIn(false);
    }
}
