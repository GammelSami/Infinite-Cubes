// Inventory stores item stacks, known formulas and coin totals.
#pragma once
#include <vector>
#include "cube/Item.h"

namespace cube {
// Inventory balances and prices are measured in copper.
enum {
    CopperPerSilver = 100,
    CopperPerGold = 10000
};

enum InventoryTab {
    InventoryTabEquipment = 0,
    InventoryTabGeneral = 1,
    InventoryTabMaterials = 2,
    InventoryTabPets = 3
};
// Number of entries, not an ID.
enum {
    InventoryTabCount = 4
};

struct ItemStack {
    ItemStack()
        : quantity(0)
    {}

    // Consuming the last item clears its category and subtype only.
    void removeOne()
    {
        --quantity;
        if (quantity <= 0) {
            quantity = 0;
            item.category = ItemCategoryNone;
            item.id = 0;
        }
    }

    int quantity;
    Item item;
};

struct Inventory {
    Inventory()
        : m_copperBalance(0),
          m_platinumCoins(0)
    {}

    // The inventory tab for an item.
    int itemTab(const Item &item) const
    {
        switch (item.category) {
        case ItemCategoryWeapon:
        case ItemCategoryChest:
        case ItemCategoryHands:
        case ItemCategoryFeet:
        case ItemCategoryShoulder:
        case ItemCategoryNeck:
        case ItemCategoryRing:
        case ItemCategorySpecial:
        case ItemCategoryLamp:
            return InventoryTabEquipment;
        case ItemCategoryPet:
        case ItemCategoryPetFood:
            return InventoryTabPets;
        case ItemCategoryMaterial:
            return InventoryTabMaterials;
        default:
            return InventoryTabGeneral;
        }
    }

    void add(const Item &item, int tab = -1)
    {
        if (tab == -1)
            tab = itemTab(item);
        if (item.category == ItemCategoryNone)
            return;
        if (int(m_inventoryTabs.size()) <= tab)
            m_inventoryTabs.resize(tab + 1);
        Item normalized(item);
        int quantity = 1;
        if (normalized.category == ItemCategoryCoin || normalized.category == ItemCategoryPlatinumCoin ||
            normalized.category == ItemCategoryQuestItem ||
            (normalized.category == ItemCategoryMaterial && normalized.id != MaterialItemSpirit) ||
            normalized.category == ItemCategoryNone || normalized.category == ItemCategoryManaCube ||
            normalized.category == ItemCategoryPetFood || normalized.category == ItemCategoryLamp ||
            normalized.category == ItemCategorySpecial) {
            quantity = normalized.level;
            normalized.level = 1;
        }
        if (item.category == ItemCategoryCoin) {
            switch (item.material) {
            case ItemMaterialCopper:
                m_copperBalance += quantity;
                return;
            case ItemMaterialGold:
                m_copperBalance += quantity * CopperPerGold;
                return;
            case ItemMaterialSilver:
                m_copperBalance += quantity * CopperPerSilver;
                return;
            }
        }
        if (item.category == ItemCategoryPlatinumCoin) {
            m_platinumCoins += quantity;
            return;
        }
        int emptySlot = -1;
        // Callers must supply -1 or a valid tab; other negatives index outside the array.
        int i = 0;
        while (i < int(m_inventoryTabs[tab].size())) {
            if (m_inventoryTabs[tab][i].quantity == 0 && emptySlot < 0)
                emptySlot = i;
            if (item.isStackable() && m_inventoryTabs[tab][i].item == normalized) {
                m_inventoryTabs[tab][i].quantity += quantity;
                return;
            }
            ++i;
        }
        if (emptySlot >= 0) {
            m_inventoryTabs[tab][emptySlot].item = normalized;
            m_inventoryTabs[tab][emptySlot].quantity = quantity;
            return;
        }
        ItemStack stack;
        stack.item = normalized;
        stack.quantity = quantity;
        m_inventoryTabs[tab].push_back(stack);
    }

    std::vector<std::vector<ItemStack>> m_inventoryTabs;
    // Stack carried by the pointer and exchanged with inventory stacks.
    ItemStack m_cursor;
    int m_copperBalance;
    int m_platinumCoins;
};

}
