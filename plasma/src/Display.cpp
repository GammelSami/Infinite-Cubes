#include "plasma/Display.h"

namespace plasma {

Display::Display(Engine *engine, const std::wstring &name)
    : Keyable(engine, name),
      m_visibility("visibility", DISPLAY_VISIBLE),
      m_clipping("clipping", DISPLAY_CLIPPING_DISABLED),
      m_fillColor("fill", Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)),
      m_strokeColor("stroke", Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)),
      m_blurRadius("blurRadius", 0.0f),
      m_flags(0)
{
    m_engineEntry = engine->addDisplay(this);
    addAttribute(&m_visibility);
    addAttribute(&m_clipping);
    addAttribute(&m_fillColor);
    addAttribute(&m_strokeColor);
    addAttribute(&m_blurRadius);
}

Display::~Display() {}

Display *Display::clone()
{
    Display *display = m_engine->createDisplay(m_name);
    display->m_visibility = m_visibility;
    display->m_clipping = m_clipping;
    display->m_strokeColor = m_strokeColor;
    display->m_fillColor = m_fillColor;
    display->m_blurRadius = m_blurRadius;
    display->m_flags = m_flags;
    return display;
}

}
