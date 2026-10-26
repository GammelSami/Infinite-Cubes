// VoxelWidget displays spirit-cube editing controls.
#pragma once
#include "plasma/Widget.h"
#include "cube/Inventory.h"
#include "cube/Sprite.h"
namespace cube {
class GameController;
class VoxelWidget : public plasma::Widget {
public:
    VoxelWidget(plasma::Engine *engine, plasma::Node *node, GameController *game);
    virtual void draw();
    void cancelUpgrade();
    void selectUpgrade();
    // A click on the item's model: places a spirit cube of the selected upgrade, moves the held one
    // there, or picks up the one there. The arguments (the item's material and level,
    // GameController::handleButtonDown) are not read.
    void placeUpgrade(int material, int level);
    // The upgrade item under the pointer, if any.
    Item *GetHoveredUpgradeItem() { return m_hoveredUpgradeIndex >= 0 ? &m_previewUpgradeItem : 0; }

    Item *m_item; // item receiving spirit cubes
    plasma::Vector<3, float> m_rotation;
    std::vector<ItemStack> m_availableCustomizations;
    int m_selectedUpgradeIndex;
    Item m_selectedUpgradeItem;
    GameController *m_game;
    plasma::Vector<3, int> m_voxelPlacementPosition;
    bool m_pointerOnVoxel;
    int m_hoveredUpgradeIndex;
    Item m_previewUpgradeItem;
    bool m_holdingPlacedUpgrade;
    plasma::Vector<3, int> m_upgradePickupLocation;
    int m_upgradePickupMaterial;
    int m_upgradePickupLevel; // Spirit::level
    Sprite m_voxelSprite;     // one white voxel at construction
};
}
