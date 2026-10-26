// SpriteWidget displays a voxel-model preview.
#pragma once
#include "plasma/Widget.h"
namespace cube {
class GameController;
class Sprite;
struct Item;
class SpriteWidget : public plasma::Widget {
public:
    SpriteWidget(plasma::Engine *engine, plasma::Node *node, GameController *game);
    virtual void draw();
    virtual bool contains(const plasma::Vector<2, float> &point);
    Sprite *m_sprite;
    Item *m_item;
    GameController *m_game;
};
}
