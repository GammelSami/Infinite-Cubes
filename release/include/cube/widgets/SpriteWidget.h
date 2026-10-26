// SpriteWidget draws one rotating item, or a sprite when the item is empty, centred within its widget.
#pragma once
#include "cube/widgets/BaseWidget.h"
#include "cube/Item.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;
class Sprite;

class SpriteWidget : public BaseWidget {
public:
    SpriteWidget(Game *game, plasma::Node *node);

    virtual void draw();

protected:
    Sprite *m_sprite;
    long long m_unused1f0;

public:
    Item m_item; // Item shown when its category is nonzero.

protected:
    Game *m_game;
};

}
