#include <algorithm>
// Spirit, Item and ItemStack describe enchantment cubes, inventory and equipment items, and counted inventory stacks.
// Items are also used by widgets and the network protocol.
#pragma once
#include <vector>
#include <string.h>
#include <string>
#include "plasma/Vector.h"
#include "cube/ItemIds.h"

namespace cube {

class Creature;

// A spirit (an enchantment cube) placed on an item: a voxel position on the item's model and its material. A pet's
// name is stored one character per spirit material (Item::SetPetName).
struct Spirit {
    plasma::Vector<3, signed char> position; // x, y, z
    unsigned char material;
};

// Item::category values.
enum ItemCategory {
    ItemCategoryNone = 0,       // the empty item: Item() and every cleared stack
    ItemCategoryConsumable = 1, // L"LifePotion", L"Cookie", L"Bomb"...
    ItemCategoryFormula = 2,
    ItemCategoryWeapon = 3,
    ItemCategoryChest = 4,
    ItemCategoryHands = 5,
    ItemCategoryFeet = 6,
    ItemCategoryShoulder = 7,
    ItemCategoryNeck = 8,
    ItemCategoryRing = 9,
    ItemCategoryCube = 10,     // itemSprite draws cube.cub; these items are stackable.
    ItemCategoryMaterial = 11, // L"Nugget", L"Log", L"Feather"...
    ItemCategoryCoin = 12,
    ItemCategoryPlatinumCoin = 13,
    ItemCategoryManaCube = 14,
    ItemCategoryLeftovers = 15,
    ItemCategoryBeak = 16,
    ItemCategoryPainting = 17,
    ItemCategoryVase = 18, // itemSprite selects vase.cub through vase4.cub by modifier % 4.
    ItemCategoryCandle = 19,
    ItemCategoryPet = 20,
    ItemCategoryPetFood = 21, // L"Bait" for every id below 306, then the named foods
    ItemCategoryQuest = 22,   // L"GoldKey", L"Letter", L"DragonDrool"...
    ItemCategoryArtifact = 23,
    ItemCategorySpecial = 24, // L"HangGlider", L"Boat", L"Reins", the crafting books, the instruments
    ItemCategoryLamp = 25,
    ItemCategoryEyePatch = 26,  // its eight ids select the eye-patch models (EyePatchId); equipment (isEquipment)
    ItemCategoryHouseIcon = 27, // itemSprite draws icon-house.cub; used from the inventory like a consumable.
    ItemCategoryElixir = 28,
    ItemCategoryGlyph = 29,         // Speech::createItemName: L"Glyph of " and the skill of its id; portal-stone sprite
    ItemCategoryGeneratedItem = 30, // GeneratedItemId: Item::transformed turns it into equipment or a gold bag
    ItemCategoryGoldBag = 31,
    ItemCategoryManaSpirit = 32,
    ItemCategoryPetCall = 33, // the pet-call action's item (Game::controlCharacter, AbilityCallPet); pet-whistle sprite
};

struct Item {
    // Zero-initialises every item member.
    Item()
        : category(ItemCategoryNone),
          id(NoneCleared),
          modifier(0),
          region(0, 0),
          rarity(ItemRarityWhite),
          formulaCategory(ItemCategoryNone),
          material(ItemMaterialNone),
          spiritCount(0)
    {
        memset(spirits, 0, sizeof(spirits));
    }

    // Whether this is a two-handed weapon.
    bool isTwoHanded() const
    {
        return category == ItemCategoryWeapon &&
               (id == WeaponLongsword || id == WeaponBow || id == WeaponCrossbow || id == WeaponBoomerang ||
                id == WeaponStaff || id == WeaponWand || id == WeaponGreatsword || id == WeaponGreataxe ||
                id == WeaponGreatmace || id == WeaponPitchfork || id == WeaponBiteAttack);
    }
    // Whether this equipment is renowned: its modifier is a multiple of ten times its rarity plus one. Renowned
    // equipment costs twice as much and retains its rarity one region away.
    bool isRenowned() const
    {
        return ((category >= ItemCategoryWeapon && category <= ItemCategoryRing) || category == ItemCategoryEyePatch) &&
               modifier % ((rarity + 1) * 10) == 0;
    }

    // Whether the item belongs to a region: categories 2..9, 15, 22, 24, 26 and 30. Game assigns such items the region
    // of their source item or creature.
    bool hasRegion() const
    {
        return category == ItemCategoryFormula || category == ItemCategoryWeapon || category == ItemCategoryChest ||
               category == ItemCategoryHands || category == ItemCategoryFeet || category == ItemCategoryShoulder ||
               category == ItemCategoryNeck || category == ItemCategoryRing || category == ItemCategoryLeftovers ||
               category == ItemCategoryQuest || category == ItemCategorySpecial || category == ItemCategoryEyePatch ||
               category == ItemCategoryGeneratedItem;
    }

    void addSpirit(int x, int y, int z, int material);
    // The opacity the renderer draws the item with: 0.6 for the category 11 ids 37 and 39, else 1.
    float alpha() const;
    // For armour (categories 4..7): a modifier-seeded random base, doubled for chest armour (again for id 1) and
    // scaled by the material.
    float armor(Creature *creature) const;
    int artifactType() const;
    // weaponDamage without the item's spirits.
    float baseDamage(Creature *creature) const;
    // Milliseconds a consumable (category 1) or a category 28 item blocks further use: shorter for rarer items.
    int cooldown() const;
    // Coin amount for category 31, seeded by modifier and increasing with rarity.
    int count() const;
    // The item's critical chance (categories 3..9; the tooltip shows it times 100 as "CRITICAL %").
    float critical(Creature *creature) const;
    // 1.0 for category 24 id 12, else 0 (the tooltip: "Increases damage against demons by 100%").
    float demonDamageBonus() const;
    // The item shown for the item: a category 22 item with id 63..66 stands for a weapon (of the id in
    // formulaCategory), a chest armour, an amulet or a ring.
    Item displayedItem() const;
    // The average materialColor of the item's glowing spirits (materials from 128), zero without one.
    plasma::Vector<4, float> effectColor() const;
    // Consumable value: 200, 100 or 0 depending on its ID, shown in the tooltip.
    float effectValue() const;
    // Whether the creature's class can use the item, by weapon ID or armor material; formulas test the item they make.
    bool fitsClass(int classType) const;
    // Effective rarity in the given region: the item's rarity in its own region, and for renowned items one region
    // away; otherwise -1.
    int getRarity(const plasma::Vector<2, int> &region) const;
    // For weapons and armour (categories 3..7): a modifier-seeded random base, doubled like armor and scaled by the
    // material.
    float hp(Creature *creature) const;
    // Emitted light colour: categories 19 and 14, and category 11 id 19; black for other items.
    plasma::Vector<3, float> lightColor() const;
    // How many spirits the item can hold: 32 for two-handed weapons, else 16.
    int maxSpiritCount() const;
    // 2 to the power of the item's level for the creature: getRarity in the creature's region (the item's own region
    // for creatures other than players and their kind), a modifier-seeded fraction below one half, and half a level
    // for a full set of spirits. Every statistic below is a multiple of it.
    float power(Creature *creature, int spirits) const;
    // The item's price in gold: AdaptionWidget compares it with the local creature's gold and shows it as "COST:
    // <price> GOLD".
    int price() const;
    float regeneration(Creature *creature) const;
    // Removes the spirit at the voxel (x, y, z).
    void removeSpirit(int x, int y, int z);
    float resistance(Creature *creature) const;
    // The size the renderer draws the item's model with, by category and id (1/14 for most).
    float scale() const;
    // std::max(sqrt(price()) / 2, 1): what a vendor pays (InventoryWidget shows it with the coin sign).
    int sellPrice() const;
    // Stores up to 32 characters of name in the spirits' materials and the length in spiritCount (names of 33
    // characters or more are ignored).
    void setPetName(const std::wstring &name);
    // How strongly the renderer lets the item's model shine (1, 0.5, 0.25 or 0) by its material, and 1 for artifacts
    // and some category 24 ids.
    float shininess() const;
    float tempo(Creature *creature) const;
    // The colour of the item's name as seen from region: fixed colours for some categories, else rarityColor of
    // getRarity (equipment) or of the plain rarity.
    plasma::Vector<4, float> textColor(const plasma::Vector<2, int> &region) const;
    // A category 30 item resolved into the concrete item it stands for (seeded by its modifier, for the creature's
    // class); any other item is returned as a copy.
    Item transformed(Creature *creature) const;
    // Whether this is equipment: categories 3..9 or 26.
    bool isEquipment() const
    {
        return (unsigned char)(category - ItemCategoryWeapon) <= 6 || category == ItemCategoryEyePatch;
    }
    // Damage added by spirits: weaponDamage minus baseDamage.
    float upgradeDamage(Creature *creature) const;
    // Weapon damage for the creature, zero for other categories; summed by Creature::attackPower, spellPower and
    // blockPower.
    float weaponDamage(Creature *creature) const;

    // Equality: category, id, modifier, region, formula category, material, spirit count and rarity, then the spirits
    // in use (material first).
    bool operator==(const Item &other) const
    {
        if (category != other.category || id != other.id || modifier != other.modifier || region != other.region ||
            formulaCategory != other.formulaCategory || material != other.material ||
            spiritCount != other.spiritCount || rarity != other.rarity) {
            return false;
        }
        for (int i = 0; i < spiritCount; i++) {
            if (spirits[i].material != other.spirits[i].material || spirits[i].position != other.spirits[i].position) {
                return false;
            }
        }
        return true;
    }
    bool operator!=(const Item &other) const { return !(*this == other); }

    unsigned char category; // 3 weapon, 4 chest, 5 hands, 6 feet, 7 shoulder (Creature::armor, GetMaxHP), artifact
    unsigned int id;
    unsigned int modifier;         // seed of the item's random properties (GetArmor, GetItemHP)
    plasma::Vector<2, int> region; // the zone region the item comes from (GetEffectiveRarity)
    unsigned char rarity;
    int formulaCategory;
    unsigned char material; // scales GetArmor and GetItemHP
    Spirit spirits[32];
    unsigned char spiritCount;
};

// The interface colour of a rarity, clamped to -1..4 (-1: grey 0.7, 0: white, ...).
plasma::Vector<4, float> rarityColor(int rarity);

// Whether this item stacks: categories 1, 10..13, 22 and 28. Inventory::add combines equal items in these categories.
inline bool itemIsStackable(const Item &item)
{
    switch (item.category) {
    case ItemCategoryConsumable:
    case ItemCategoryCube:
    case ItemCategoryMaterial:
    case ItemCategoryCoin:
    case ItemCategoryPlatinumCoin:
    case ItemCategoryQuest:
    case ItemCategoryElixir:
        return true;
    }
    return false;
}

// A formula ingredient: an item and its required count.
struct Ingredient {
    Item item;
    int quantity;
};

// A crafting formula: the resulting item, required workstation and ingredients. Game keeps the active formula and
// FormulaDetailsWidget displays it.
struct Formula {
    Formula()
        : workstationRequired(WorkstationNone)
    {}

    Item result;
    int workstationRequired;
    std::vector<Ingredient> ingredients;
};

// Spirit material colour: ordinary materials multiplied by tint, glowing materials (128..131: fire, unholy, ice, wind)
// brightened by glow, with glow added to alpha.
plasma::Vector<4, float> materialColor(int material, const plasma::Vector<4, float> &tint, float glow);

// One inventory slot: how many of the item the slot holds.
struct ItemStack {
    ItemStack()
        : quantity(0)
    {}
    // Constructs an empty stack, copies the item and sets its count.
    ItemStack(const Item &item, int quantity)
        : quantity(0)
    {
        this->item = item;
        this->quantity = quantity;
    }

    int quantity; // the add path adds to it
    Item item;
};

}
