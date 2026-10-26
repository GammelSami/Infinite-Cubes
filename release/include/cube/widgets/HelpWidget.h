#pragma once
#include "cube/widgets/BaseWidget.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class HelpWidget : public BaseWidget {
public:
    HelpWidget(Game *game, plasma::Node *node);

    virtual void draw();
    // HelpWidget displays the control hints and game help.

    Game *m_game;
};

}
