// PreviewWidget displays item and skill descriptions.
#pragma once
#include <list>
#include <vector>
#include "plasma/Widget.h"
namespace cube {
class GameController;
struct Item;
class PreviewWidget : public plasma::Widget {
public:
    PreviewWidget(plasma::Engine *engine, plasma::Node *node, GameController *game);
    virtual void draw();

    Item *m_item;
    bool m_itemInfoFlag; // forwarded when drawing equipped items
    int m_skill;         // -1 when unset
    int m_skillLevel;
    int m_specialization;              // -1 when unset
    std::list<Item *> m_equippedItems; // Render's "Currently equipped" list
    GameController *m_game;
};
}
