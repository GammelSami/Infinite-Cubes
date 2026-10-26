#pragma once
#include "cube/widgets/BaseWidget.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class SystemWidget : public BaseWidget {
public:
    SystemWidget(Game *game, plasma::Node *node);

    // SystemWidget displays the system menu at y positions 30, 60, 90 and 120, highlights the hovered entry and
    // connects its release handler.
    virtual void draw();

    // The pointer-release handler Render connects: hides the menu, then toggles the options or controls widget,
    // returns to the start menu or shuts the game down, by m_hoveredButton.
    void buttonReleased();

protected:
    Game *m_game;
    int m_hoveredButton;
};

}
