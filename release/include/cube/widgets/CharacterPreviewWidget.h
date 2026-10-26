#pragma once
#include "cube/widgets/BaseWidget.h"

namespace plasma {
class Node;
}

namespace cube {

class Creature;
class Game;

class CharacterPreviewWidget : public BaseWidget {
public:
    CharacterPreviewWidget(Game *game, plasma::Node *node);

    virtual void draw();
    // CharacterPreviewWidget draws the current character preview using the game's renderer.

    Creature *m_creature; // Game used throughout draw; initially null.
    Game *m_game;         // Preview creature used by draw.
};

}
