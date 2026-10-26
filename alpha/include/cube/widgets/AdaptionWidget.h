// AdaptionWidget displays equipment adaptation controls.
#pragma once
#include "plasma/Widget.h"
namespace cube {
class GameController;
struct Item;
class AdaptionWidget : public plasma::Widget {
public:
    AdaptionWidget(plasma::Engine *engine, plasma::Node *node, GameController *game, plasma::Node *itemFrame,
                   plasma::Node *rightArrow);
    virtual void draw();
    virtual void resized();
    int getCost();
    Item *getItem();
    bool isPointerOverGoodbye();
    bool isPointerOverUpgrade();
    // Selects an inventory item by tab and stack index.
    void SetInventoryItem(int tabIndex, int itemIndex)
    {
        m_inventoryTabIndex = tabIndex;
        m_inventoryItemIndex = itemIndex;
    }
    Item *m_item; // direct item, or null to use the inventory indices
    int m_inventoryTabIndex;
    int m_inventoryItemIndex;
    GameController *m_game;
    plasma::Node *m_itemFrame;
    plasma::Node *m_rightArrow;
};
}
