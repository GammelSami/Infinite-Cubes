// Item operations compute equipment attributes and select item behavior.
#include "cube/Item.h"
#include "cube/StatCurves.h"
#include "cube/DefaultBlocks.h"
#include <stdlib.h>

#include "cube/Fixed.h"

namespace cube {

namespace {
enum {
    LifePotionCooldownMs = 3000,
    DefaultConsumableCooldownMs = 10000,
    ModifierStepCount = 21,
    HpRegenerationModifierStride = 8,
};
}
using plasma::Vector;

void Item::addSpirit(int x, int y, int z, int material, int level)
{
    for (int i = 0; i < spiritCount; ++i) {
        if (spirits[i].position == Vector<3, signed char>(Vector<3, int>(x, y, z))) {
            spirits[i].material = material;
            spirits[i].level = level;
            return;
        }
    }
    if (spiritCount < maxSpiritCount()) {
        spirits[spiritCount].position = Vector<3, int>(x, y, z);
        spirits[spiritCount].material = material;
        spirits[spiritCount].level = level;
        ++spiritCount;
    }
}

float Item::armor() const
{
    if (category != ItemCategoryChest && category != ItemCategoryShoulder && category != ItemCategoryHands &&
        category != ItemCategoryFeet)
        return 0;
    float scale = 0.5f;
    if (category == ItemCategoryChest)
        scale = 1.0f;
    switch (material) {
    case ItemMaterialLight:
    case ItemMaterialSilk:
        scale *= 0.75f;
        break;
    case ItemMaterialParrot:
    case ItemMaterialLinen:
    case ItemMaterialCotton:
        scale *= 0.85f;
        break;
    case ItemMaterialSaurian:
        scale *= 0.8f;
        break;
    }
    return itemPowerScale(level + spiritCount * 0.1f, rarity) * scale;
}

int Item::cooldown() const
{
    if (category != ItemCategoryConsumable)
        return 0;
    switch (id) {
    case ConsumableLifePotion:
        return LifePotionCooldownMs;
    default:
        return DefaultConsumableCooldownMs;
    }
}

float Item::critical() const
{
    if (category != ItemCategoryNeck && category != ItemCategoryRing && category != ItemCategoryWeapon &&
        category != ItemCategoryChest && category != ItemCategoryShoulder && category != ItemCategoryHands &&
        category != ItemCategoryFeet)
        return 0;
    float scale = 0.05f;
    if (isTwoHanded() || category == ItemCategoryChest)
        scale = 0.1f;
    float factor = 1.0f - (float)(modifier % ModifierStepCount) / 20.0f;
    if (material == ItemMaterialGold)
        factor += 1.0f;
    float result = itemDamageScale(level, rarity) * scale * factor;
    if (result < 0.001f)
        return 0;
    return result;
}

Vector<4, float> Item::effectColor() const
{
    int count = 0;
    Vector<4, float> result(0, 0, 0, 0);
    for (int i = 0; i < spiritCount; ++i) {
        if (spirits[i].material >= ItemMaterialFireSpirit) {
            ++count;
            result += materialColor(spirits[i].material, Vector<4, float>(1, 1, 1, 1), 1);
        }
    }
    if (count > 0)
        result *= 1.0f / count;
    return result;
}

float Item::effectValue() const
{
    if (category == ItemCategoryConsumable) {
        switch (id) {
        case ConsumableLifePotion:
        case ConsumableCactusPotion:
            return itemPowerScale(level, rarity) * 200.0f;
        case ConsumableGinsengSoup:
        case ConsumableSnowBerryMash:
        case ConsumableMushroomSpit:
            return itemPowerScale(level, rarity) * 200.0f;
        case ConsumablePineappleSlice:
        case ConsumablePumpkinMuffin:
            return itemPowerScale(level, rarity) * 100.0f;
        }
    }
    return 0;
}

bool Item::fitsClass(int classType) const
{
    switch (category) {
    case ItemCategoryFormula: {
        Item result = *this;
        result.category = formulaCategory;
        result.formulaCategory = ItemCategoryNone;
        if (result.category == ItemCategoryFormula)
            return true;
        return result.fitsClass(classType);
    }
    case ItemCategoryWeapon:
        switch (id) {
        case WeaponDagger:
        case WeaponFist:
        case WeaponLongsword:
            return classType == ClassRogue;
        case WeaponBow:
        case WeaponCrossbow:
        case WeaponBoomerang:
        case WeaponArrows:
            return classType == ClassRanger;
        case WeaponStaff:
        case WeaponWand:
        case WeaponBracelet:
            return classType == ClassMage;
        case WeaponSword:
        case WeaponAxe:
        case WeaponMace:
        case WeaponShield:
        case WeaponGreatsword:
        case WeaponGreataxe:
        case WeaponGreatmace:
            return classType == ClassWarrior;
        }
        break;
    case ItemCategoryChest:
    case ItemCategoryHands:
    case ItemCategoryFeet:
    case ItemCategoryShoulder:
        switch (material) {
        case ItemMaterialIron:
        case ItemMaterialObsidian:
        case ItemMaterialSaurian:
        case ItemMaterialIce:
            return classType == ClassWarrior;
        case ItemMaterialParrot:
        case ItemMaterialLinen:
            return classType == ClassRanger;
        case ItemMaterialLight:
        case ItemMaterialSilk:
            return classType == ClassMage;
        case ItemMaterialCotton:
            return classType == ClassRogue;
        }
        break;
    }
    return true;
}

float Item::hp() const
{
    if (category != ItemCategoryWeapon && category != ItemCategoryChest && category != ItemCategoryShoulder &&
        category != ItemCategoryHands && category != ItemCategoryFeet)
        return 0;
    float scale = 0.5f;
    if (category == ItemCategoryChest)
        scale = 1.0f;
    float factor = 1.0f - (float)((modifier * HpRegenerationModifierStride) % ModifierStepCount) / 20.0f + 1.0f;
    switch (material) {
    case ItemMaterialIron:
        factor += 1.0f;
        break;
    case ItemMaterialLinen:
        factor += 0.5f;
        break;
    case ItemMaterialCotton:
        factor += 0.75f;
        break;
    }
    return itemPowerScale(level + spiritCount * 0.1f, rarity) * 5.0f * scale * factor;
}
// Item light palette: ordinary/undead candle and shimmer-mushroom RGB emission; other items emit
// no light.
Vector<3, float> Item::lightColor() const
{
    switch (category) {
    case ItemCategoryCandle:
        switch (id) {
        case CandleItemCandle:
            return Vector<3, float>(0.6f, 0.6f, 0.2f);
        case CandleItemUndead:
            return Vector<3, float>(0.0f, 0.6f, 0.2f);
        }
        break;
    case ItemCategoryMaterial:
        switch (id) {
        case MaterialItemShimmerMushroom:
            return Vector<3, float>(0.1f, 0.3f, 0.3f);
        }
        break;
    }
    return Vector<3, float>(0.0f, 0.0f, 0.0f);
}

// Cpp; cases confirmed by alpha's jump table. Material tint palette: RGB plus alpha; spirit
// entries also brighten with glow and inherit tint alpha.
plasma::Vector<4, float> materialColor(int material, const plasma::Vector<4, float> &tint, float glow)
{
    switch (material) {
    case ItemMaterialIron:
        return plasma::Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f) * tint;
    case ItemMaterialGold:
        return plasma::Vector<4, float>(1.0f, 0.7f, 0.2f, 1.0f) * tint;
    case ItemMaterialSilver:
        return plasma::Vector<4, float>(0.8f, 0.8f, 0.85f, 1.0f) * tint;
    case ItemMaterial3:
        return plasma::Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f) * tint;
    case ItemMaterialObsidian:
        return plasma::Vector<4, float>(0.1f, 0.1f, 0.1f, 1.0f) * tint;
    case ItemMaterialBone:
        return plasma::Vector<4, float>(0.9f, 0.9f, 0.9f, 1.0f) * tint;
    case ItemMaterialWood:
        return plasma::Vector<4, float>(0.4f, 0.3f, 0.2f, 1.0f) * tint;
    case ItemMaterialFireSpirit:
        return Vector<4, float>(1.0f, 0.5f, 0.1f, glow + tint[3]) * (glow * 0.5f + 1.0f);
    case ItemMaterialUnholySpirit:
        return Vector<4, float>(0.3f, 1.0f, 0.5f, glow + tint[3]) * (glow * 0.5f + 1.0f);
    case ItemMaterialIceSpirit:
        return Vector<4, float>(0.3f, 0.5f, 1.0f, glow + tint[3]) * (glow * 0.5f + 1.0f);
    case ItemMaterialWindSpirit:
        return Vector<4, float>(0.8f, 0.8f, 1.0f, glow + tint[3]) * (glow * 0.5f + 1.0f);
    default:
        return plasma::Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f) * tint;
    }
}

int Item::maxSpiritCount() const
{
    if (isTwoHanded())
        return SpiritCapacity;
    return SpiritCapacity / 2;
}

int Item::power() const
{
    return (int)(levelBonus(level) * 100.0f + 1.0f);
}

int Item::price() const
{
    float scale = 0.2f;
    switch (category) {
    case ItemCategoryWeapon:
    case ItemCategoryChest:
        scale = 2.0f;
        break;
    case ItemCategoryShoulder:
    case ItemCategoryNeck:
        scale = 1.5f;
        break;
    case ItemCategoryHands:
    case ItemCategoryFeet:
    case ItemCategoryRing:
        scale = 1.0f;
        break;
    case ItemCategorySpecial:
        scale = 100.0f;
        break;
    }
    if (isTwoHanded())
        scale *= 2.0f;
    if ((int)(itemPowerScale(level, rarity) * 10.0f * scale) < 1)
        return 1;
    return (int)(itemPowerScale(level, rarity) * 10.0f * scale);
}

float Item::regeneration() const
{
    if (category != ItemCategoryWeapon && category != ItemCategoryChest && category != ItemCategoryShoulder &&
        category != ItemCategoryHands && category != ItemCategoryFeet)
        return 0;
    float scale = 0.1f;
    if (category == ItemCategoryChest)
        scale = 0.2f;
    float factor = (float)((modifier * HpRegenerationModifierStride) % ModifierStepCount) / 20.0f;
    switch (material) {
    case ItemMaterialLinen:
        factor += 0.5f;
        break;
    case ItemMaterialCotton:
        factor += 1.0f;
        break;
    }
    return itemPowerScale(level + spiritCount * 0.1f, rarity) * scale * factor;
}

void Item::removeSpirit(int x, int y, int z)
{
    for (int i = 0; i < spiritCount; ++i) {
        if (Vector<3, int>(x, y, z) == Vector<3, int>(spirits[i].position)) {
            for (; i < spiritCount - 1; ++i)
                spirits[i] = spirits[i + 1];
            --spiritCount;
            return;
        }
    }
}

float Item::regenerationValue() const
{
    if (category != ItemCategoryConsumable)
        return 0;
    switch (id) {
    case ConsumableGinsengSoup:
    case ConsumableMushroomSpit:
        return itemDamageScale(level, rarity) * 1.5f;
    }
    return 0;
}

float Item::resistance() const
{
    if (category != ItemCategoryChest && category != ItemCategoryShoulder && category != ItemCategoryHands &&
        category != ItemCategoryFeet)
        return 0;
    float scale = 0.5f;
    if (category == ItemCategoryChest)
        scale = 1.0f;
    switch (material) {
    case ItemMaterialIron:
    case ItemMaterialParrot:
        scale *= 0.85f;
        break;
    case ItemMaterialLinen:
    case ItemMaterialCotton:
        scale *= 0.75f;
        break;
    }
    return itemPowerScale(level + spiritCount * 0.1f, rarity) * scale;
}

float Item::shininess() const
{
    if (category == ItemCategoryNone)
        return 0;
    if (material == ItemMaterialIron || material == ItemMaterialGold || material == ItemMaterialSilver ||
        material == ItemMaterialIce)
        return 1;
    return 0;
}

float Item::tempo() const
{
    if (category != ItemCategoryNeck && category != ItemCategoryRing && category != ItemCategoryWeapon &&
        category != ItemCategoryChest && category != ItemCategoryShoulder && category != ItemCategoryHands &&
        category != ItemCategoryFeet)
        return 0;
    float scale = 0.1f;
    if (isTwoHanded() || category == ItemCategoryChest)
        scale = 0.2f;
    float factor = (float)(modifier % ModifierStepCount) / 20.0f;
    if (material == ItemMaterialSilver)
        factor += 1.0f;
    float result = itemDamageScale(level, rarity) * scale * factor;
    if (result < 0.001f)
        return 0;
    return result;
}
// Item-label RGBA palette: category overrides followed by rarity colors; red marks an unrecognized
// rarity.
Vector<4, float> Item::textColor() const
{
    if (category == ItemCategoryQuestItem)
        return Vector<4, float>(0.5f, 0.5f, 0.5f, 1);
    if (category == ItemCategoryPlatinumCoin)
        return Vector<4, float>(0.5f, 0.1f, 1, 1);
    if (category == ItemCategoryManaCube)
        return Vector<4, float>(0.2f, 0.5f, 1, 1);
    switch (rarity) {
    case ItemRarityWhite:
        return Vector<4, float>(1, 1, 1, 1);
    case ItemRarityGreen:
        return Vector<4, float>(0, 1, 0, 1);
    case ItemRarityBlue:
        return Vector<4, float>(0.25f, 0.25f, 1, 1);
    case ItemRarityPurple:
        return Vector<4, float>(0.5f, 0, 1, 1);
    case ItemRarityYellow:
        return Vector<4, float>(1, 1, 0, 1);
    default:
        return Vector<4, float>(1, 0, 0, 1);
    }
}

void Item::varyLevel(float variation, bool increase)
{
    if (level < 2)
        return;
    float fraction = levelBonus(level) - variation;
    if (fraction <= 0) {
        level = 1;
        return;
    }
    int minimum = (int)levelFromBonus(fraction);
    int maximum = level;
    if (minimum < 1)
        minimum = 1;
    if (increase) {
        float upper = levelBonus(maximum) + variation;
        if (upper > 0.9999f)
            upper = 0.9999f;
        maximum = (int)levelFromBonus(upper);
    }
    level = minimum + rand() % (maximum - minimum + 1);
}

float Item::weaponDamage() const
{
    if (category != ItemCategoryWeapon)
        return 0;
    float bonus = spiritCount * 0.1f;
    switch (id) {
    case WeaponDagger:
    case WeaponFist:
        return itemPowerScale(level + bonus, rarity) * 2.0f;
    case WeaponLongsword:
        return itemPowerScale(level + bonus, rarity) * 4.0f;
    case WeaponShield:
        return itemPowerScale(level + bonus, rarity) * 2.0f;
    }
    if (isTwoHanded())
        return itemPowerScale(level + bonus, rarity) * 8.0f;
    return itemPowerScale(level + bonus, rarity) * 4.0f;
}
}
