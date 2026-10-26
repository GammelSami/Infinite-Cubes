// ObjectiveWidget owns an unused objective list.
#pragma once
#include "plasma/Widget.h"
#include <vector>

namespace cube {
class GameController;

class ObjectiveWidget : public plasma::Widget {
public:
    ObjectiveWidget(plasma::Engine *engine, plasma::Node *node, GameController *game);

    // The frame clears this vector; no code fills or reads it. Its element type is unknown.
    std::vector<void *> unused160;
    GameController *m_game;
};
}
