// CharacterWidget displays character equipment and statistics.
#pragma once
#include "plasma/Widget.h"
namespace cube {
class GameController;
class CharacterWidget : public plasma::Widget {
public:
    CharacterWidget(plasma::Engine *engine, plasma::Node *node, GameController *game);
    virtual void draw();
    GameController *m_game;
};

// Rounds to one decimal by multiplying by ten, adding 0.5 and truncating; handles negative inputs
// recursively.
float roundTenth(float value);
}
