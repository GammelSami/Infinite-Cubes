// SystemWidget draws the system menu.
#include "cube/widgets/SystemWidget.h"

#include "cube/DefaultBlocks.h"
#include "cube/GameController.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
namespace cube {

SystemWidget::SystemWidget(plasma::Engine *engine, plasma::Node *node, GameController *game)
    : Widget(engine, node),
      m_game(game)
{
    m_hoveredButton = -1;
}

// The system menu's three entries, centred 30 units apart and drawn like the start menu's
// (StartMenuWidget::draw): Options, Start Menu, Exit Game, 14 pixels high with a stroke of 3.
void SystemWidget::draw()
{
    plasma::ScalableFont *font = m_engine->getFontEngine()->getFont(L"resource1.dat");
    if (font) {
        m_engine->beginTextDrawing(m_node);
        plasma::Vector<2, float> pointer = getPointerOffset();
        m_hoveredButton = -1;

        plasma::Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
        if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= 10.0f && 40.0f > pointer[1]) {
            m_hoveredButton = SystemMenuOptions;
            color = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
        }
        font->renderText(L"Options", getContentWidth() * 0.5f, 30.0f, 14.0f, 3.0f, 0.0f, 0.0f, 0.0f,
                         plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, true);
        font->renderText(L"Options", getContentWidth() * 0.5f, 30.0f, 14.0f, 0.0f, 0.0f, 0.0f, 0.0f, color.m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, true);

        color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= 40.0f && 70.0f > pointer[1]) {
            m_hoveredButton = SystemMenuStartMenu;
            color = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
        }
        font->renderText(L"Start Menu", getContentWidth() * 0.5f, 60.0f, 14.0f, 3.0f, 0.0f, 0.0f, 0.0f,
                         plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, true);
        font->renderText(L"Start Menu", getContentWidth() * 0.5f, 60.0f, 14.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                         color.m_elements, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, true);

        color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= 70.0f && 100.0f > pointer[1]) {
            m_hoveredButton = SystemMenuExit;
            color = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
        }
        font->renderText(L"Exit Game", getContentWidth() * 0.5f, 90.0f, 14.0f, 3.0f, 0.0f, 0.0f, 0.0f,
                         plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, true);
        font->renderText(L"Exit Game", getContentWidth() * 0.5f, 90.0f, 14.0f, 0.0f, 0.0f, 0.0f, 0.0f, color.m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, true);
        m_engine->endTextDrawing(m_node);
    }
}
}
