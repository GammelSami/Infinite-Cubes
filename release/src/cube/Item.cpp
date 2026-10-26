#include "cube/Item.h"

#include <algorithm>
#include <math.h>

#include "cube/Creature.h"
#include "cube/Noise.h"
#include "cube/CreatureIds.h"
#include "cube/ItemIds.h"

namespace cube {

static const int MaximumPetNameLength = 32;
static const int ArmorSalt = 98312;
static const int GoldAmountSalt = 837423;
static const int HpSalt = 9843;
static const int RegenerationSalt = 343;
static const int ResistanceSalt = 6765;

// The xorshift step that turns an item's modifier (plus a constant per statistic) into the seed of that statistic's
// random part.
static inline unsigned int scramble(unsigned int value)
{
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

void Item::addSpirit(int x, int y, int z, int material)
{
    for (int i = 0; i < spiritCount; i++) {
        if (spirits[i].position == plasma::Vector<3, signed char>(plasma::Vector<3, int>(x, y, z))) {
            spirits[i].material = material;
            return;
        }
    }
    if (spiritCount < (unsigned int)maxSpiritCount()) {
        spirits[spiritCount].position = plasma::Vector<3, int>(x, y, z);
        spirits[spiritCount].material = material;
        spiritCount++;
    }
}

float Item::alpha() const
{
    if (category == ItemCategoryMaterial) {
        if (id == MaterialItemDewdrop) {
            return 0.6f;
        }
        if (id == MaterialItemIceCube) {
            return 0.6f;
        }
    }
    return 1.0f;
}

float Item::armor(Creature *creature) const
{
    if (category != ItemCategoryChest && category != ItemCategoryHands && category != ItemCategoryFeet &&
        category != ItemCategoryShoulder) {
        return 0.0f;
    }
    float armor = (float)(scramble(modifier + ArmorSalt) % 100000) * 0.25f / 100000.0f + 1.0f;
    if (category == ItemCategoryChest) {
        armor = armor * 2.0f;
        if (id == ChestBackpack) {
            armor *= 2.0f;
        }
    }
    if (isTwoHanded()) {
        armor *= 2.0f;
    }
    switch (material) {
    case ItemMaterialGold:
        armor *= 1.125f;
        break;
    case ItemMaterialSteel:
        armor *= 1.25f;
        break;
    case ItemMaterialBronze:
        armor *= 1.15f;
        break;
    case ItemMaterialIron:
        armor *= 1.2f;
        break;
    case ItemMaterialLight:
    case ItemMaterialSilk:
    case ItemMaterialFlannell:
        armor *= 0.5f;
        break;
    case ItemMaterialClothing:
        armor *= 0.25f;
        break;
    case ItemMaterialFilament:
        armor *= 0.6f;
        break;
    case ItemMaterialMail:
        armor *= 0.9f;
        break;
    case ItemMaterialSaurian:
    case ItemMaterialParrot:
    case ItemMaterialLinen:
    case ItemMaterialCotton:
    case ItemMaterialCalico:
    case ItemMaterialFelt:
        armor *= 0.8f;
        break;
    }
    return power(creature, 0) * armor;
}

int Item::artifactType() const
{
    if (category != ItemCategoryArtifact) {
        return -1;
    }
    return modifier % (ArtifactEffect_LampRadius + 1);
}

float Item::baseDamage(Creature *creature) const
{
    if (category != ItemCategoryWeapon) {
        return 0.0f;
    }
    switch (id) {
    case WeaponLongsword:
        return power(creature, 0) * 4.0f;
    case WeaponShield:
        return power(creature, 0) * 4.0f;
    case WeaponDagger:
    case WeaponFist:
        return power(creature, 0) * 2.0f;
    }
    if (isTwoHanded()) {
        return power(creature, 0) * 8.0f;
    }
    return power(creature, 0) * 4.0f;
}

int Item::cooldown() const
{
    if (category == ItemCategoryElixir) {
        return 500;
    }
    if (category == ItemCategoryConsumable) {
        switch (id) {
        case ConsumableEssenceOfWisdom:
            return 500;
        default:
            return (int)((1.5f - (float)rarity * 0.5f * 0.25f) * 10000.0f);
        case ConsumableCactusPotion:
            return (int)((1.5f - (float)rarity * 0.5f * 0.25f) * 6000.0f);
        case ConsumableLifePotion:
        case ConsumableGreaterLifePotion:
            return (int)((1.5f - (float)rarity * 0.5f * 0.25f) * 5000.0f);
        }
    }
    return 0;
}

int Item::count() const
{
    if (category != ItemCategoryGoldBag) {
        return 0;
    }
    return (int)(((float)(scramble(modifier + GoldAmountSalt) % 100000) / 100000.0f + (float)(rarity + 1)) * 10.0f);
}

float Item::critical(Creature *creature) const
{
    if (category != ItemCategoryWeapon && category != ItemCategoryChest && category != ItemCategoryHands &&
        category != ItemCategoryFeet && category != ItemCategoryShoulder && category != ItemCategoryNeck &&
        category != ItemCategoryRing) {
        return 0.0f;
    }
    float critical = 0.01f;
    if (isTwoHanded() || category == ItemCategoryChest || category == ItemCategoryNeck ||
        category == ItemCategoryRing) {
        critical = 0.02f;
    }
    if (category == ItemCategoryChest && id == ChestBackpack) {
        critical *= 2.0f;
    }
    float factor = 1.0f - (float)(modifier % 21) / 20.0f;
    switch (material) {
    case ItemMaterialGold:
        factor += 1.0f;
        break;
    case ItemMaterialClothing:
        critical *= 0.5f;
        break;
    }
    float result = power(creature, 0) * 0.0625f * critical * factor * 3.0f;
    if (result < 0.001f) {
        return 0.0f;
    }
    return result;
}

float Item::demonDamageBonus() const
{
    if (category == ItemCategorySpecial && id == SpecialItemVoidStone) {
        return 1.0f;
    }
    return 0.0f;
}

Item Item::displayedItem() const
{
    Item item = *this;
    if (item.category == ItemCategoryQuest) {
        switch (item.id) {
        case QuestItemRing:
            item.category = ItemCategoryRing;
            item.id = RingRing;
            break;
        case QuestItemAmulet:
            item.category = ItemCategoryNeck;
            item.id = NeckAmulet;
            break;
        case QuestItemChestArmor:
            item.category = ItemCategoryChest;
            item.id = ChestChestArmor;
            break;
        case QuestItemWeapon:
            item.id = item.formulaCategory;
            item.category = ItemCategoryWeapon;
            break;
        }
    }
    return item;
}

plasma::Vector<4, float> Item::effectColor() const
{
    plasma::Vector<4, float> color(0.0f, 0.0f, 0.0f, 0.0f);
    int count = 0;
    for (int i = 0; i < spiritCount; i++) {
        if (spirits[i].material >= ItemMaterialFireSpirit) {
            count++;
            color += materialColor(spirits[i].material, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
        }
    }
    if (count > 0) {
        color *= 1.0f / count;
    }
    return color;
}

float Item::effectValue() const
{
    if (category == ItemCategoryConsumable) {
        switch (id) {
        case ConsumableLifePotion:
        case ConsumableCactusPotion:
        case ConsumableGinsengSoup:
        case ConsumableUnnamedHealing5:
        case ConsumableUnnamedHealing6:
        case ConsumableSnowBerryMash:
        case ConsumableMushroomSpit:
        case ConsumableGreaterLifePotion:
            return 200.0f;
        case ConsumablePineappleSlice:
        case ConsumablePumpkinMuffin:
            return 100.0f;
        }
    }
    return 0.0f;
}

bool Item::fitsClass(int classType) const
{
    switch (category) {
    case ItemCategoryChest:
    case ItemCategoryHands:
    case ItemCategoryFeet:
    case ItemCategoryShoulder:
        switch (material) {
        case ItemMaterialIron:
        case ItemMaterialSteel:
        case ItemMaterialObsidian:
        case ItemMaterialGold:
        case ItemMaterialSaurian:
        case ItemMaterialIce:
        case ItemMaterialBronze:
            return classType == ClassWarrior;
        case ItemMaterialCotton:
        case ItemMaterialCalico:
        case ItemMaterialRivet:
            return classType == ClassRogue;
        case ItemMaterialParrot:
        case ItemMaterialLinen:
        case ItemMaterialFelt:
        case ItemMaterialMail:
            return classType == ClassRanger;
        case ItemMaterialLight:
        case ItemMaterialSilk:
        case ItemMaterialFlannell:
        case ItemMaterialFilament:
            return classType == ClassMage;
        }
        break;
    case ItemCategoryWeapon:
        switch (id) {
        case WeaponSword:
        case WeaponAxe:
        case WeaponMace:
        case WeaponShield:
        case WeaponGreatsword:
        case WeaponGreataxe:
        case WeaponGreatmace:
            return classType == ClassWarrior;
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
        }
        break;
    case ItemCategoryFormula: {
        Item product = *this;
        product.category = formulaCategory;
        product.formulaCategory = ItemCategoryNone;
        if (product.category != ItemCategoryFormula) {
            return product.fitsClass(classType);
        }
        break;
    }
    }
    return true;
}

int Item::getRarity(const plasma::Vector<2, int> &region) const
{
    if (!isRenowned() || (std::max)(abs(region[0] - this->region[0]), abs(region[1] - this->region[1])) > 1) {
        if (!(region == this->region)) {
            return -1;
        }
    }
    return rarity;
}

float Item::hp(Creature *creature) const
{
    if (category != ItemCategoryWeapon && category != ItemCategoryChest && category != ItemCategoryHands &&
        category != ItemCategoryFeet && category != ItemCategoryShoulder) {
        return 0.0f;
    }
    float hp = ((float)(scramble(modifier + HpSalt) % 100000) * 0.25f / 100000.0f + 1.0f) * 10.0f;
    if (category == ItemCategoryChest) {
        hp = hp * 2.0f;
        if (id == ChestBackpack) {
            hp *= 2.0f;
        }
    }
    if (isTwoHanded()) {
        hp *= 2.0f;
    }
    float factor = 1.0f;
    switch (material) {
    case ItemMaterialCotton:
        factor = 1.75f;
        break;
    case ItemMaterialLinen:
        factor = 1.5f;
        break;
    case ItemMaterialIron:
        factor = 2.0f;
        break;
    }
    return power(creature, 0) * hp * factor;
}

plasma::Vector<3, float> Item::lightColor() const
{
    switch (category) {
    case ItemCategoryCandle:
        switch (id) {
        case CandleItemUndead:
            return plasma::Vector<3, float>(0.0f, 0.6f, 0.2f);
        case CandleItemCandle:
            return plasma::Vector<3, float>(0.6f, 0.6f, 0.2f);
        }
        break;
    case ItemCategoryManaCube:
        return plasma::Vector<3, float>(0.1f, 0.3f, 0.5f);
    case ItemCategoryMaterial:
        switch (id) {
        case MaterialItemShimmerMushroom:
            return plasma::Vector<3, float>(0.1f, 0.3f, 0.3f);
        }
        break;
    }
    return plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
}

plasma::Vector<4, float> materialColor(int material, const plasma::Vector<4, float> &tint, float glow)
{
    plasma::Vector<4, float> color;
    switch (material) {
    case ItemMaterialIron:
        color = plasma::Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f);
        break;
    case ItemMaterialGold:
        color = plasma::Vector<4, float>(1.0f, 0.7f, 0.2f, 1.0f);
        break;
    case ItemMaterialSilver:
        color = plasma::Vector<4, float>(0.8f, 0.8f, 0.85f, 1.0f);
        break;
    case ItemMaterial3:
        color = plasma::Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f);
        break;
    case ItemMaterialObsidian:
        color = plasma::Vector<4, float>(0.1f, 0.1f, 0.1f, 1.0f);
        break;
    case ItemMaterialBone:
        color = plasma::Vector<4, float>(0.9f, 0.9f, 0.9f, 1.0f);
        break;
    case ItemMaterialWood:
        color = plasma::Vector<4, float>(0.4f, 0.3f, 0.2f, 1.0f);
        break;
    case ItemMaterialFireSpirit:
        color = plasma::Vector<4, float>(1.0f, 0.5f, 0.1f, glow + tint[3]);
        color *= glow * 0.5f + 1.0f;
        return color;
    case ItemMaterialUnholy:
        color = plasma::Vector<4, float>(0.3f, 1.0f, 0.5f, glow + tint[3]);
        color *= glow * 0.5f + 1.0f;
        return color;
    case ItemMaterialIceSpirit:
        color = plasma::Vector<4, float>(0.3f, 0.5f, 1.0f, glow + tint[3]);
        color *= glow * 0.5f + 1.0f;
        return color;
    case ItemMaterialWind:
        color = plasma::Vector<4, float>(0.8f, 0.8f, 1.0f, glow + tint[3]);
        color *= glow * 0.5f + 1.0f;
        return color;
    default:
        color = plasma::Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f);
        break;
    }
    color *= tint;
    return color;
}

int Item::maxSpiritCount() const
{
    if (isTwoHanded()) {
        return 32;
    }
    return 16;
}

float Item::power(Creature *creature, int spirits) const
{
    const plasma::Vector<2, int> *where;
    if (creature->m_entityData.hostilityType == HostilityPlayer ||
        creature->m_entityData.hostilityType == HostilityNeutral ||
        creature->m_entityData.hostilityType == HostilityPet) {
        where = &creature->m_entityData.region;
    } else {
        where = &region;
    }
    unsigned int seed = modifier;
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    float level = (float)getRarity(*where) + (float)(seed % 1000) / 2000.0f;
    return powf(2.0f, level + (float)spirits * 0.5f / (float)maxSpiritCount());
}

int Item::price() const
{
    float price = 0.2f;
    switch (category) {
    case ItemCategoryWeapon:
        price = 5.0f;
        break;
    case ItemCategoryChest:
        price = 4.0f;
        break;
    case ItemCategoryShoulder:
    case ItemCategoryNeck:
        price = 3.0f;
        break;
    case ItemCategoryHands:
    case ItemCategoryFeet:
    case ItemCategoryRing:
        price = 2.0f;
        break;
    case ItemCategoryQuest:
        price = 50.0f;
        break;
    case ItemCategoryMaterial:
        if (id >= MaterialItemEmerald && id <= MaterialItemDiamond) {
            price = 10.0f;
        }
        break;
    }
    if (isTwoHanded()) {
        price *= 2.0f;
    }
    price *= 5.0f;
    if (isRenowned()) {
        price = price * 2.0f;
    }
    return (std::max)(1, (int)(powf(2.0f, (float)rarity) * price));
}

float Item::regeneration(Creature *creature) const
{
    if ((category < ItemCategoryChest || category > ItemCategoryRing) && category != ItemCategoryEyePatch) {
        return 0.0f;
    }
    float regeneration = ((float)(scramble(modifier + RegenerationSalt) % 100000) * 0.25f / 100000.0f + 1.0f) * 0.01f;
    if (category == ItemCategoryChest) {
        regeneration = regeneration * 2.0f;
    }
    if (isTwoHanded()) {
        regeneration *= 2.0f;
    }
    float factor = (float)((modifier * 8) % 21) / 20.0f;
    switch (material) {
    case ItemMaterialBronze:
        factor += 0.1f;
        break;
    case ItemMaterialRivet:
        factor += 0.75f;
        break;
    case ItemMaterialCotton:
    case ItemMaterialCalico:
        factor += 1.0f;
        break;
    case ItemMaterialMail:
        factor += 0.4f;
        break;
    case ItemMaterialLinen:
    case ItemMaterialFelt:
        factor += 0.5f;
        break;
    }
    float result = power(creature, 0) * regeneration * factor * 1.5;
    if (result < 0.001f) {
        return 0.0f;
    }
    return result;
}

void Item::removeSpirit(int x, int y, int z)
{
    for (int i = 0; i < spiritCount; i++) {
        if (plasma::Vector<3, int>(x, y, z) == plasma::Vector<3, int>(spirits[i].position)) {
            for (int j = i; j < spiritCount - 1; j++) {
                spirits[j] = spirits[j + 1];
            }
            spiritCount--;
            return;
        }
    }
}

float Item::resistance(Creature *creature) const
{
    if (category != ItemCategoryChest && category != ItemCategoryHands && category != ItemCategoryFeet &&
        category != ItemCategoryShoulder) {
        return 0.0f;
    }
    float resistance = (float)(scramble(modifier + ResistanceSalt) % 100000) * 0.25f / 100000.0f + 1.0f;
    if (category == ItemCategoryChest) {
        resistance = resistance * 2.0f;
        if (id == ChestBackpack) {
            resistance *= 2.0f;
        }
    }
    if (isTwoHanded()) {
        resistance *= 2.0f;
    }
    switch (material) {
    case ItemMaterialIron:
    case ItemMaterialSteel:
    case ItemMaterialParrot:
    case ItemMaterialBronze:
        resistance *= 0.85f;
        break;
    case ItemMaterialLinen:
    case ItemMaterialCotton:
    case ItemMaterialCalico:
    case ItemMaterialFelt:
    case ItemMaterialMail:
    case ItemMaterialRivet:
        resistance *= 0.75f;
        break;
    case ItemMaterialClothing:
        resistance *= 0.25f;
        break;
    }
    return power(creature, 0) * resistance;
}

float Item::scale() const
{
    if (category == ItemCategoryMaterial) {
        if (id == MaterialItemDragonRoot) {
            return 0.05f;
        }
        if (id == MaterialItemNugget || (id >= MaterialItemEmerald && id <= MaterialItemDiamond)) {
            return 0.15f;
        }
    }
    if (category == ItemCategoryCoin) {
        return 0.04f;
    }
    if (category == ItemCategorySpecial && (id == SpecialItemHangGlider || id == SpecialItemBoat)) {
        return 0.15f;
    }
    if (category == ItemCategoryMaterial && id == MaterialItemKale) {
        return 0.15f;
    }
    return 1.0f / 14.0f;
}

int Item::sellPrice() const
{
    return (int)(std::max)(1.0f, sqrtf((float)price()) * 0.5f);
}

void Item::setPetName(const std::wstring &name)
{
    if (name.size() > MaximumPetNameLength) {
        return;
    }
    spiritCount = (unsigned char)name.size();
    for (int i = 0; i < spiritCount; i++) {
        spirits[i].material = (unsigned char)name[i];
    }
}

float Item::shininess() const
{
    if (category == ItemCategoryNone) {
        return 0.0f;
    }
    if (material == ItemMaterialIron || material == ItemMaterialGold || material == ItemMaterialSilver ||
        material == ItemMaterialPlatinum || material == ItemMaterialIce) {
        return 1.0f;
    }
    if (material == ItemMaterialBronze || material == ItemMaterialSteel) {
        return 0.5f;
    }
    if (material == ItemMaterialMail || material == ItemMaterialFilament) {
        return 0.25f;
    }
    if (category == ItemCategorySpecial &&
        (id == SpecialItemFlute || id == SpecialItemHarp || id == SpecialItemWhistle || id == SpecialItemBell ||
         id == SpecialItemLute || id == SpecialItemClimbingSpikes)) {
        return 1.0f;
    }
    if (category == ItemCategoryArtifact) {
        return 1.0f;
    }
    return 0.0f;
}

float Item::tempo(Creature *creature) const
{
    if (category != ItemCategoryWeapon && category != ItemCategoryChest && category != ItemCategoryHands &&
        category != ItemCategoryFeet && category != ItemCategoryShoulder && category != ItemCategoryNeck &&
        category != ItemCategoryRing) {
        return 0.0f;
    }
    float tempo = 0.01f;
    if (isTwoHanded() || category == ItemCategoryChest || category == ItemCategoryNeck ||
        category == ItemCategoryRing) {
        tempo = 0.02f;
    }
    if (category == ItemCategoryChest && id == ChestBackpack) {
        tempo *= 2.0f;
    }
    float factor = (float)(modifier % 21) / 20.0f;
    switch (material) {
    case ItemMaterialBronze:
        factor += 0.5f;
        break;
    case ItemMaterialSilver:
        factor += 1.0f;
        break;
    case ItemMaterialSteel:
    case ItemMaterialMail:
    case ItemMaterialRivet:
    case ItemMaterialFilament:
        factor *= 0.9f;
        break;
    case ItemMaterialClothing:
        tempo *= 0.5f;
        break;
    }
    float result = power(creature, 0) * 0.0625f * tempo * factor * 3.0f;
    if (result < 0.001f) {
        return 0.0f;
    }
    return result;
}

plasma::Vector<4, float> Item::textColor(const plasma::Vector<2, int> &region) const
{
    if (category == ItemCategoryArtifact || category == ItemCategorySpecial) {
        return plasma::Vector<4, float>(1.0f, 0.6f, 0.25f, 1.0f);
    }
    if (category == ItemCategoryPetFood) {
        return plasma::Vector<4, float>(0.7f, 1.0f, 0.25f, 1.0f);
    }
    if (category == ItemCategoryNone) {
        return plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    }
    if (category == ItemCategoryQuest) {
        return plasma::Vector<4, float>(0.5f, 1.0f, 1.0f, 1.0f);
    }
    if (category == ItemCategoryPlatinumCoin) {
        return plasma::Vector<4, float>(0.5f, 0.1f, 1.0f, 1.0f);
    }
    if (category == ItemCategoryConsumable && id == ConsumablePortalStone) {
        return plasma::Vector<4, float>(0.5f, 1.0f, 1.0f, 1.0f);
    }
    if ((category >= ItemCategoryFormula && category <= ItemCategoryRing) || category == ItemCategoryLeftovers ||
        category == ItemCategoryEyePatch || category == ItemCategoryGeneratedItem) {
        return rarityColor(getRarity(region));
    }
    return rarityColor(rarity);
}

Item Item::transformed(Creature *creature) const
{
    if (category != ItemCategoryGeneratedItem) {
        return *this;
    }
    // Every choice below draws scramble of the next seed. The first one picks the kind: 0 a weapon and 1..4 armour of
    // the creature's class, 5 a ring, 6 an amulet. Id 1 stands for coins (category 31) instead.
    unsigned int seed = modifier;
    unsigned int random = scramble(seed++);
    Item item;
    if (id != GeneratedItemGoldBag) {
        unsigned int type = random % 7;
        switch (type) {
        case 5:
            item.category = ItemCategoryRing;
            item.material = scramble(seed++) % 2 == 0 ? ItemMaterialSilver : ItemMaterialGold;
            item.region = region;
            item.rarity = rarity;
            item.modifier = modifier;
            return item;
        case 6:
            item.category = ItemCategoryNeck;
            item.material = scramble(seed++) % 2 == 0 ? ItemMaterialSilver : ItemMaterialGold;
            item.region = region;
            item.rarity = rarity;
            item.modifier = modifier;
            return item;
        }
        switch (creature->m_entityData.classType) {
        case ClassWarrior:
            switch (type) {
            case 0:
                item.region = region;
                item.rarity = rarity;
                item.category = ItemCategoryWeapon;
                item.material = ItemMaterialIron;
                item.modifier = modifier;
                switch (scramble(seed++) % 3) {
                case 0:
                    item.id = WeaponSword + scramble(seed++) % 3;
                    break;
                case 1:
                    item.id = WeaponGreatsword + scramble(seed++) % 3;
                    break;
                case 2:
                    item.id = WeaponShield;
                    break;
                }
                return item;
            case 1:
            case 2:
            case 3:
            case 4:
                item.category = ItemCategoryChest + scramble(seed++) % 4;
                switch (scramble(seed++) % 3) {
                case 0:
                    item.material = ItemMaterialIron;
                    break;
                case 1:
                    item.material = ItemMaterialBronze;
                    break;
                case 2:
                    item.material = ItemMaterialSteel;
                    break;
                }
                item.region = region;
                item.rarity = rarity;
                item.modifier = modifier;
                return item;
            }
            break;
        case ClassRanger:
            switch (type) {
            case 0:
                item.region = region;
                item.rarity = rarity;
                item.category = ItemCategoryWeapon;
                item.material = ItemMaterialWood;
                item.modifier = modifier;
                switch (scramble(seed++) % 3) {
                case 0:
                    item.id = WeaponBow;
                    break;
                case 1:
                    item.id = WeaponCrossbow;
                    break;
                case 2:
                    item.id = WeaponBoomerang;
                    break;
                }
                return item;
            case 1:
            case 2:
            case 3:
            case 4:
                item.category = ItemCategoryChest + scramble(seed++) % 4;
                switch (scramble(seed++) % 3) {
                case 0:
                    item.material = ItemMaterialLinen;
                    break;
                case 1:
                    item.material = ItemMaterialFelt;
                    break;
                case 2:
                    item.material = ItemMaterialMail;
                    break;
                }
                item.region = region;
                item.rarity = rarity;
                item.modifier = modifier;
                return item;
            }
            break;
        case ClassMage:
            switch (type) {
            case 0:
                item.region = region;
                item.rarity = rarity;
                item.category = ItemCategoryWeapon;
                item.modifier = modifier;
                switch (scramble(seed++) % 3) {
                case 0:
                    item.id = WeaponStaff;
                    item.material = ItemMaterialWood;
                    break;
                case 1:
                    item.id = WeaponWand;
                    item.material = ItemMaterialWood;
                    break;
                case 2:
                    item.id = WeaponBracelet;
                    item.material = ItemMaterialGold + scramble(seed++) % 2;
                    break;
                }
                return item;
            case 1:
            case 2:
            case 3:
            case 4:
                item.category = ItemCategoryChest + scramble(seed++) % 4;
                switch (scramble(seed++) % 3) {
                case 0:
                    item.material = ItemMaterialSilk;
                    break;
                case 1:
                    item.material = ItemMaterialFlannell;
                    break;
                case 2:
                    item.material = ItemMaterialFilament;
                    break;
                }
                item.region = region;
                item.rarity = rarity;
                item.modifier = modifier;
                return item;
            }
            break;
        case ClassRogue:
            switch (type) {
            case 0:
                item.region = region;
                item.rarity = rarity;
                item.category = ItemCategoryWeapon;
                item.material = ItemMaterialIron;
                item.modifier = modifier;
                switch (scramble(seed++) % 3) {
                case 0:
                    item.id = WeaponDagger;
                    break;
                case 1:
                    item.id = WeaponFist;
                    break;
                case 2:
                    item.id = WeaponLongsword;
                    break;
                }
                return item;
            case 1:
            case 2:
            case 3:
            case 4:
                item.category = ItemCategoryChest + scramble(seed++) % 4;
                switch (scramble(seed++) % 3) {
                case 0:
                    item.material = ItemMaterialCotton;
                    break;
                case 1:
                    item.material = ItemMaterialCalico;
                    break;
                case 2:
                    item.material = ItemMaterialRivet;
                    break;
                }
                item.region = region;
                item.rarity = rarity;
                item.modifier = modifier;
                return item;
            }
            break;
        }
    } else {
        item.category = ItemCategoryGoldBag;
        item.rarity = rarity;
        item.modifier = modifier;
        return item;
    }
}

float Item::upgradeDamage(Creature *creature) const
{
    return weaponDamage(creature) - baseDamage(creature);
}

float Item::weaponDamage(Creature *creature) const
{
    if (category != ItemCategoryWeapon) {
        return 0.0f;
    }
    switch (id) {
    case WeaponLongsword:
        return power(creature, spiritCount) * 4.0f;
    case WeaponShield:
        return power(creature, spiritCount) * 4.0f;
    case WeaponDagger:
    case WeaponFist:
        return power(creature, spiritCount) * 2.0f;
    }
    if (isTwoHanded()) {
        if (id == WeaponWand && creature->m_entityData.hostilityType != HostilityPlayer) {
            return power(creature, spiritCount) * 6.0f;
        }
        return power(creature, spiritCount) * 8.0f;
    }
    return power(creature, spiritCount) * 4.0f;
}

}
