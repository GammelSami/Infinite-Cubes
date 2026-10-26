// See cube/SystemWidget.h.
#include "plasma/TextIds.h"
#include "cube/Game.h"
#include "cube/widgets/SystemWidget.h"
#include "cube/World.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Node.h"

#include "cube/InterfaceIds.h"
#include "plasma/EditFilter.h"

namespace cube {

SystemWidget::SystemWidget(Game *game, plasma::Node *node)
    : BaseWidget(game->m_engine, node, L""),
      m_game(game),
      m_hoveredButton(-1)
{
    setFont(L"resource1.dat");
}

void SystemWidget::draw()
{
    // The entries, centred, 30 units apart; the one whose band (20 units above to 10 below its line) holds the pointer
    // is highlighted and recorded.
    plasma::Vector<2, float> pointer = getPointerOffset();
    m_hoveredButton = -1;
    plasma::Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
    if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= 10.0f && pointer[1] < 40.0f) {
        m_hoveredButton = SystemMenuOptions;
        color = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
    }
    setTextSize(13.0f);
    setTextStrokeRadius(3.0f);
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    setTextFlags(1 << plasma::TEXT_CENTER);
    setTextColor(color);
    renderText(L"Options", getContentWidth() * 0.5f, 30.0f);
    m_game->m_interface.addButtonCenter(this, (int)(getContentWidth() * 0.5f), 25);

    color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= 40.0f && pointer[1] < 70.0f) {
        m_hoveredButton = SystemMenuControls;
        color = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
    }
    setTextColor(color);
    renderText(L"Controls", getContentWidth() * 0.5f, 60.0f);
    m_game->m_interface.addButtonCenter(this, (int)(getContentWidth() * 0.5f), 55);

    color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= 70.0f && pointer[1] < 100.0f) {
        m_hoveredButton = SystemMenuStartMenu;
        color = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
    }
    setTextColor(color);
    renderText(L"Start Menu", getContentWidth() * 0.5f, 90.0f);
    m_game->m_interface.addButtonCenter(this, (int)(getContentWidth() * 0.5f), 85);

    color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    if (pointer[0] >= 0.0f && getContentWidth() > pointer[0] && pointer[1] >= 100.0f && pointer[1] < 130.0f) {
        m_hoveredButton = SystemMenuExit;
        color = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
    }
    setTextColor(color);
    renderText(L"Exit Game", getContentWidth() * 0.5f, 120.0f);
    m_game->m_interface.addButtonCenter(this, (int)(getContentWidth() * 0.5f), 115);

    connect(plasma::WIDGET_EVENT_POINTER_RELEASED, this, &SystemWidget::buttonReleased, true, true);
}

void SystemWidget::buttonReleased()
{
    // Hide the menu, then act on the entry under the pointer.
    m_node->display()->m_visibility.value() = 0;
    switch (m_hoveredButton) {
    case SystemMenuOptions:
        m_game->m_interface.toggleOptionsWidget();
        break;
    case SystemMenuControls:
        m_game->m_interface.toggleControlsWidget();
        break;
    case SystemMenuStartMenu:
        m_game->onStartMenu();
        break;
    case SystemMenuExit:
        m_game->onExit();
        break;
    }
}

}
