#pragma once
#include "cube/widgets/BaseWidget.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;
struct Item;

class AdaptionWidget : public BaseWidget {
public:
    AdaptionWidget(Game *game, plasma::Node *node, plasma::Node *itemFrame, plasma::Node *rightArrow);

    virtual void draw();
    virtual void resized();
    // AdaptionWidget displays an item's upgrade cost and the player's gold, and offers Upgrade and Goodbye buttons.

    // The item's price in gold (through a float), 0 without an item.
    int getCost();
    // Whether the pointer is over the "Goodbye!" button (the right half of the bottom 30 units) or, with an affordable
    // item, over the "Upgrade" button (the left half). Called by Render and Game::HandleMouseUp.
    bool isPointerOverGoodbye();
    bool isPointerOverUpgrade();

    Item *m_item;               // Item being upgraded; initially null. Its price determines the upgrade cost.
    Game *m_game;               // Game used to read the player's gold.
    plasma::Node *m_itemFrame;  // Instanced button node, laid out on resize.
    plasma::Node *m_rightArrow; // Instanced button node, laid out on resize.
};

}
