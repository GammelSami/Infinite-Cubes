// InventoryWidget tab indices depend on m_type; stored indices stay int.
#pragma once

namespace cube {

// These are the m_type == 2 crafting tabs, not Inventory::itemTab results.
enum CraftingTab {
    CraftingTabWeapons = 0,
    CraftingTabArmor = 1,
    CraftingTabAmulets = 2,
    CraftingTabCooking = 3,
    CraftingTabAlchemy = 4,
    CraftingTabIngredients = 5,
    CraftingTabRepair = 6,
};

enum VendorTab {
    VendorTabStock = 0,
    VendorTabBuyback = 1,
};

}
