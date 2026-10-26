// VoxelWidget customizes equipped items. It draws the selected weapon's voxels, lists available upgrades and lets the
// player place, move or pick up spirit cubes under the pointer.
#pragma once
#include <vector>
#include "plasma/Vector.h"
#include "cube/widgets/BaseWidget.h"
#include "cube/Item.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class VoxelWidget : public BaseWidget {
public:
    VoxelWidget(Game *game, plasma::Node *node);

    // Rebuilds m_availableCustomizations, renders the item and the upgrade list, and tracks the hovered item, upgrade
    // and voxel.
    virtual void draw();

    // Called by Game::handleButtonDown.
    void selectItem();
    // Selects or deselects the hovered upgrade and plays the menu sound. With a voxel under the pointer, places the
    // selected upgrade's spirit, moves a held spirit or picks up the spirit there. Placing an upgrade consumes one
    // inventory item. Otherwise, unless the pointer is over an engine node, returns the held spirit and clears the
    // selection.
    void placeUpgrade(unsigned char material);
    void selectUpgrade();
    void cancelUpgrade();
    // The equipped weapon the widget shows: m_selectedItemIndex 0 is the local creature's right weapon, 1 its left
    // one; null when that slot is empty (category 0) or the index is neither.
    Item *selectedItem();
    // Returns the hovered upgrade's item, or null when none is hovered; used by Game::getHoveredIngredient and
    // Game::handleButtonDown.
    Item *getHoveredUpgradeItem()
    {
        Item *item = &m_previewUpgradeItem;
        return m_hoveredUpgradeIndex < 0 ? nullptr : item;
    }

public:
    plasma::Vector<3, float> m_rotation;
    std::vector<ItemStack> m_availableCustomizations;
    int m_selectedUpgradeIndex; // Initially -1.
    Item m_selectedUpgradeItem; // copied by draw
    Game *m_game;
    int m_selectedItemIndex;
    int m_hoveredItemIndex;                          // Initially -1.
    plasma::Vector<3, int> m_voxelPlacementPosition; // the voxel under the pointer
    // Meaning not known.
    bool m_pointerOnVoxel;
    int m_hoveredUpgradeIndex; // Initially -1.
    Item m_previewUpgradeItem; // Hovered upgrade item recorded by draw.
    bool m_holdingPlacedUpgrade;
    plasma::Vector<3, int> m_upgradePickupLocation; // where it was picked up
    int m_upgradePickupMaterial;                    // Material of the held spirit.
};

}
