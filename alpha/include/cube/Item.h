// Item describes an inventory object, including equipment attributes and spirit cubes.
#pragma once
#include <string.h>
#include "plasma/Vector.h"
#include "cube/ItemIds.h"

namespace cube {

// Alpha's spirits carry an additional dword, copied with each voxel.
struct Spirit {
    plasma::Vector<3, signed char> position;
    unsigned char material;
    int level; // scales spirit damage through levelBonus
};

enum ItemCategory {
    ItemCategoryNone = 0,
    ItemCategoryConsumable = 1,
    ItemCategoryFormula = 2,
    ItemCategoryWeapon = 3,
    ItemCategoryChest = 4,
    ItemCategoryHands = 5,
    ItemCategoryFeet = 6,
    ItemCategoryShoulder = 7,
    ItemCategoryNeck = 8,
    ItemCategoryRing = 9,
    ItemCategoryCube = 10,
    ItemCategoryMaterial = 11,
    ItemCategoryCoin = 12,
    ItemCategoryPlatinumCoin = 13,
    ItemCategoryLeftovers = 14,
    ItemCategoryBeak = 15,
    ItemCategoryPainting = 16,
    ItemCategoryVase = 17,
    ItemCategoryCandle = 18,
    ItemCategoryPet = 19,
    ItemCategoryPetFood = 20,
    ItemCategoryQuestItem = 21,
    ItemCategorySpecial = 23,
    ItemCategoryLamp = 24,
    ItemCategoryManaCube = 25
};

// Item::flags. Adapted: crafted or adapted gear (GameController::craft, the adaption panel) and
// generated vendor stock; the item text shows " (adapted)" and the adaption panel refuses it.
// Individual flags are bit indices; use (1 << flag) to form a mask.
enum ItemFlag {
    ItemFlagAdapted = 0
};

struct Item {
    enum {
        SpiritCapacity = 32
    };
    // Initializes an item, leaving padding unset.
    Item()
        : category(ItemCategoryNone),
          id(0),
          modifier(0),
          formulaCategory(0),
          rarity(ItemRarityWhite),
          material(0),
          flags(0),
          level(1),
          spiritCount(0)
    {
        memset(spirits, 0, sizeof(spirits));
    }

    void addSpirit(int x, int y, int z, int material, int level);
    float armor() const;
    int cooldown() const;
    float critical() const;
    plasma::Vector<4, float> effectColor() const;
    float effectValue() const;
    bool fitsClass(int classType) const;
    float hp() const;
    plasma::Vector<3, float> lightColor() const;
    int maxSpiritCount() const;
    int power() const;
    int price() const;
    float regeneration() const;
    void removeSpirit(int x, int y, int z);
    float regenerationValue() const;
    float resistance() const;
    float shininess() const;
    float tempo() const;
    plasma::Vector<4, float> textColor() const;
    void varyLevel(float variation, bool increase);
    // Whether the item has a level that follows its owner's: not coins, category 13 or 21 items,
    // materials other than spirits, pet food, lamps, specials or category 25.
    bool scalesWithLevel() const
    {
        if (category == ItemCategoryCoin || category == ItemCategoryPlatinumCoin || category == ItemCategoryQuestItem)
            return false;
        if (category == ItemCategoryMaterial && id != MaterialItemSpirit)
            return false;
        if (category == ItemCategoryNone || category == ItemCategoryManaCube || category == ItemCategoryPetFood ||
            category == ItemCategoryLamp || category == ItemCategorySpecial)
            return false;
        return true;
    }
    float weaponDamage() const;

    bool isStackable() const
    {
        return category == ItemCategoryConsumable || category == ItemCategoryCube || category == ItemCategoryCoin ||
               category == ItemCategoryPlatinumCoin || category == ItemCategoryMaterial ||
               category == ItemCategoryQuestItem;
    }

    bool operator!=(const Item &other) const { return !(*this == other); }

    bool operator==(const Item &other) const
    {
        if (category != other.category || id != other.id || modifier != other.modifier || flags != other.flags ||
            formulaCategory != other.formulaCategory || rarity != other.rarity || level != other.level ||
            material != other.material || spiritCount != other.spiritCount)
            return false;
        // Trusts spiritCount; a count above capacity reads beyond spirits.
        for (int i = 0; i < spiritCount; ++i) {
            if (spirits[i].material != other.spirits[i].material || spirits[i].position != other.spirits[i].position)
                return false;
        }
        return true;
    }

    bool isTwoHanded() const
    {
        return category == ItemCategoryWeapon &&
               (id == WeaponGreatsword || id == WeaponGreataxe || id == WeaponGreatmace || id == WeaponLongsword ||
                id == WeaponStaff || id == WeaponWand || id == WeaponPitchfork || id == WeaponBoomerang ||
                id == WeaponBow || id == WeaponCrossbow);
    }

    unsigned char category;
    unsigned char id;
    unsigned int modifier;
    int formulaCategory;
    unsigned char rarity;
    unsigned char material;
    unsigned char flags; // ItemFlag bits; other bits unexamined
    short level;         // signed loads in all stat curves
    Spirit spirits[SpiritCapacity];
    int spiritCount;
};

plasma::Vector<4, float> materialColor(int material, const plasma::Vector<4, float> &tint, float glow);

}
