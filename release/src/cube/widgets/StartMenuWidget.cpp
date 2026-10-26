// See cube/StartMenuWidget.h.
#include "plasma/TextIds.h"
#include <sstream>
#include "cube/Game.h"
#include "cube/widgets/StartMenuWidget.h"
#include "cube/World.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Node.h"
#include "plasma/EditFilter.h"

namespace cube {

StartMenuWidget::StartMenuWidget(Game *game, plasma::Node *node)
    : BaseWidget(game->m_engine, node, L""),
      m_game(game),
      m_hoveredButton(-1)
{
    setFont(L"resource1.dat");
}

void StartMenuWidget::draw()
{
    // The entries, centred, 50 units apart; the one whose band (20 units above to 10 below its line) holds the pointer
    // is highlighted and recorded. "Continue" only when a saved character is selected.
    plasma::Vector<2, float> pointer = getPointerOffset();
    setTextSize(18.0f);
    setTextStrokeRadius(4.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    m_hoveredButton = -1;
    if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= 10.0f && 40.0f > pointer[1]) {
        m_hoveredButton = StartMenuPlay;
        setTextColor(plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f));
    } else {
        setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    }
    renderText(L"Start Game", getContentWidth() * 0.5f, 30.0f);
    m_game->m_interface.addButtonCenter(this, (int)(getContentWidth() * 0.5f), 25);

    int y = 80;
    if (m_game->m_currentCharacterSlot >= 0 && m_game->m_currentCharacterSlot < (int)m_game->m_savedCharacters.size()) {
        if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= 60.0f && 90.0f > pointer[1]) {
            m_hoveredButton = StartMenuContinue;
            setTextColor(plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f));
        } else {
            setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        }
        renderText(L"Continue", getContentWidth() * 0.5f, 80.0f);
        m_game->m_interface.addButtonCenter(this, (int)(getContentWidth() * 0.5f), 75);
        y = 130;
    }

    if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= y - 20 && y + 10 > pointer[1]) {
        m_hoveredButton = StartMenuOptions;
        setTextColor(plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f));
    } else {
        setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    }
    renderText(L"Options", getContentWidth() * 0.5f, (float)y);
    m_game->m_interface.addButtonCenter(this, (int)(getContentWidth() * 0.5f), y - 5);

    y += 50;
    if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= y - 20 && y + 10 > pointer[1]) {
        m_hoveredButton = StartMenuExit;
        setTextColor(plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f));
    } else {
        setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    }
    renderText(L"Exit", getContentWidth() * 0.5f, (float)y);
    m_game->m_interface.addButtonCenter(this, (int)(getContentWidth() * 0.5f), y - 5);

    // The copyright at the bottom left and the version at the bottom right of the window, taken into the widget's
    // units.
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextFlags(0);
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    plasma::Matrix<float> inverse;
    getNode()->getInverseMatrix(inverse);
    plasma::Vector<2, float> copyright = inverse * plasma::Vector<2, float>(10.0f, (float)(m_game->m_height - 15));
    renderText(L"Copyright (c) 2019 Picroma e.K.", copyright[0], copyright[1]);
    plasma::Vector<2, float> version =
        inverse * plasma::Vector<2, float>((float)(m_game->m_width - 10), (float)(m_game->m_height - 15));
    setTextFlags(1 << plasma::TEXT_RIGHT);
    std::wstringstream text;
    text << 1 << L"." << 0 << L"." << 0 << L"-" << 1;
    renderText(text.str(), version[0], version[1]);
}

}
