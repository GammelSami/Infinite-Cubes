#pragma once
#include "cube/widgets/BaseWidget.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class CharacterWidget : public BaseWidget {
public:
    CharacterWidget(Game *game, plasma::Node *node);

    virtual void draw();
    // CharacterWidget displays the local character's statistics and equipment details.

    Game *m_game;
};

}
