// EnchantWidget displays item identification controls.
#pragma once
#include "plasma/Widget.h"
namespace cube {
class GameController;
struct Item;
class EnchantWidget : public plasma::Widget {
public:
    EnchantWidget(plasma::Engine *engine, plasma::Node *node, GameController *game, plasma::Node *itemFrame);
    virtual void draw();
    virtual void resized();
    int getCost();
    Item *getItem();
    bool isPointerOverGoodbye();
    bool isPointerOverIdentify();
    // Whether an unidentified leftovers item is selected for identification.
    bool isUnidentifiedItem();
    // Selects an inventory item by tab and stack index.
    void SetInventoryItem(int tabIndex, int itemIndex)
    {
        m_inventoryTabIndex = tabIndex;
        m_inventoryItemIndex = itemIndex;
    }
    int m_inventoryTabIndex;
    int m_inventoryItemIndex;
    int m_canIdentify; // constructor stores 1
    GameController *m_game;
    plasma::Node *m_itemFrame;
};
}
