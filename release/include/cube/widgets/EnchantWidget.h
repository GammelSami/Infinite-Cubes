// EnchantWidget identifies the inventory item selected by tab and slot, displaying its model, description and a cost
// of half its price, with Identify and Goodbye buttons.
#pragma once
#include "cube/widgets/BaseWidget.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;
struct Item;

class EnchantWidget : public BaseWidget {
public:
    EnchantWidget(Game *game, plasma::Node *node, plasma::Node *itemFrame);

    virtual void draw();
    virtual void resized();

    // Half the item's price, at least 1; 0 without an item.
    int getCost();
    // The selected stack's item if the indices are valid, the stack is not empty and the item's category is 15;
    // otherwise null.
    Item *getItem();
    // Whether the pointer is over the "Goodbye!" button (the right half of the bottom 30 units) or, with an item, the
    // "Identify" button (the left half).
    bool isPointerOverGoodbye();
    bool isPointerOverIdentify();
    // Whether there is an item to identify. Called by Game::HandleMouseUp.
    bool isUnidentifiedItem();

    int m_inventoryTabIndex;  // Selected tab index; initially -1.
    int m_inventoryItemIndex; // Selected item index; initially -1.
    int m_canIdentify;
    Game *m_game;              // Used by draw
    plasma::Node *m_itemFrame; // Used by draw; Resized
};

}
