// StartMenuWidget draws menu entries and records the hovered entry in m_hoveredButton for Game to handle.
#pragma once
#include "cube/widgets/BaseWidget.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class StartMenuWidget : public BaseWidget {
public:
    StartMenuWidget(Game *game, plasma::Node *node);

    // Draws the entries, highlights the one under the pointer, then the copyright and version at the bottom of the
    // screen.
    virtual void draw();

public:
    Game *m_game;
    int m_hoveredButton;
};

}
