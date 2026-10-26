// StartMenuWidget manages the title and selection screens.
#include "cube/widgets/StartMenuWidget.h"

#include "cube/DefaultBlocks.h"
#include "cube/GameController.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
namespace cube {

StartMenuWidget::StartMenuWidget(plasma::Engine *engine, plasma::Node *node, GameController *game)
    : Widget(engine, node),
      m_game(game)
{
    m_hoveredButton = -1;
}

// The three entries, centred 50 units apart: each is drawn twice, a white outlined pass and then
// the fill in its colour (cyan while the pointer is in its band, 20 units above to 10 below the
// line, which also records it). The entries are 18 pixels high with a stroke of 4, from y 30.
void StartMenuWidget::draw()
{
    plasma::ScalableFont *font = m_engine->getFontEngine()->getFont(L"resource1.dat");
    if (font) {
        m_engine->beginTextDrawing(m_node);
        plasma::Vector<2, float> pointer = getPointerOffset();
        m_hoveredButton = -1;

        plasma::Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
        if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= 10.0f && 40.0f > pointer[1]) {
            m_hoveredButton = StartMenuPlay;
            color = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
        }
        font->renderText(L"Start Game", getContentWidth() * 0.5f, 30.0f, 18.0f, 4.0f, 0.0f, 0.0f, 0.0f,
                         plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, true);
        font->renderText(L"Start Game", getContentWidth() * 0.5f, 30.0f, 18.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                         color.m_elements, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, true);

        color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= 60.0f && 90.0f > pointer[1]) {
            m_hoveredButton = StartMenuOptions;
            color = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
        }
        font->renderText(L"Options", getContentWidth() * 0.5f, 80.0f, 18.0f, 4.0f, 0.0f, 0.0f, 0.0f,
                         plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, true);
        font->renderText(L"Options", getContentWidth() * 0.5f, 80.0f, 18.0f, 0.0f, 0.0f, 0.0f, 0.0f, color.m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, true);

        color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= 110.0f && 140.0f > pointer[1]) {
            m_hoveredButton = StartMenuExit;
            color = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
        }
        font->renderText(L"Exit", getContentWidth() * 0.5f, 130.0f, 18.0f, 4.0f, 0.0f, 0.0f, 0.0f,
                         plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, true);
        font->renderText(L"Exit", getContentWidth() * 0.5f, 130.0f, 18.0f, 0.0f, 0.0f, 0.0f, 0.0f, color.m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                         plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, true);
        m_engine->endTextDrawing(m_node);
    }
}
}
