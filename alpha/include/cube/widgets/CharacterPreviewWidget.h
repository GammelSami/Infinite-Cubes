// CharacterPreviewWidget displays a saved-character list entry.
#pragma once
#include "plasma/Widget.h"
namespace cube {
class GameController;
class Creature;
class CharacterPreviewWidget : public plasma::Widget {
public:
    CharacterPreviewWidget(plasma::Engine *engine, plasma::Node *node, GameController *game);
    virtual void draw();
    virtual bool contains(const plasma::Vector<2, float> &point);
    Creature *m_creature; // null until a saved character is assigned
    GameController *m_game;
};
}
