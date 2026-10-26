#include <algorithm>
#include "cube/SpriteManager.h"
#include <algorithm>
#include "cube/ArtifactGenerator.h"
#include "cube/Database.h"
#include "cube/Item.h"
#include "cube/Noise.h"
#include "cube/Sprite.h"
#include "cube/ItemIds.h"
#include "cube/ZoneIds.h"
#include "cube/SpriteIds.h"

namespace cube {

// Sprite lookup and model loading.

plasma::Vector<3, unsigned char> Sprite::s_emptyVoxel;
int Sprite::s_frame;

int consumableSpriteIndex(int id, int rarity);
int weaponSpriteIndex(int id, unsigned int modifier, int rarity, int material);

SpriteManager::SpriteManager(gfx::Graphics *graphics)
    : m_graphics(graphics)
{
    loadSprites("sprites");
}

SpriteManager::~SpriteManager()
{
    for (int i = 0; i < (int)m_sprites.size(); i++) {
        if (m_sprites[i])
            delete m_sprites[i];
    }
    std::map<std::array<int, 2>, Sprite *>::iterator end = m_artifactSprites.end();
    for (std::map<std::array<int, 2>, Sprite *>::iterator it = m_artifactSprites.begin(); it != end; ++it) {
        if (it->second)
            delete it->second;
    }
}

Sprite *SpriteManager::artifactSprite(int type, int seed)
{
    Sprite *&sprite = m_artifactSprites[std::array<int, 2> {{type, seed}}];
    if (!sprite) {
        sprite = new Sprite(*m_graphics, true);
        ArtifactGenerator generator(sprite, type, seed);
    }
    return sprite;
}

// The index in m_sprites of the model of a consumable (item category 1).
int consumableSpriteIndex(int id, int rarity)
{
    switch (id) {
    case ConsumableCookie:
        return SpriteCookie;
    case ConsumableLifePotion:
        return SpriteLifePotion1 + rarity;
    case ConsumableEssenceOfWisdom:
        return SpriteManaPotion;
    case ConsumableGinsengSoup:
        return SpriteSoupGinseng;
    case ConsumableMushroomSpit:
        return SpriteMushroomSpit;
    case ConsumableBomb:
        return SpriteBomb1;
    case ConsumableCactusPotion:
        return SpriteCactusPotion;
    case ConsumableSnowBerryMash:
        return SpriteSnowBerryMash;
    case ConsumablePumpkinMuffin:
        return SpritePumpkinMuffin;
    case ConsumablePineappleSlice:
        return SpritePineappleSlice;
    case ConsumablePortalStone:
        return SpritePortalStone;
    case ConsumableApple:
        return SpriteApple;
    default:
        return SpriteMushroom;
    }
}

Sprite *SpriteManager::createSprite(bool flag)
{
    return new Sprite(*m_graphics, flag);
}

Sprite *SpriteManager::decorationSprite(int index)
{
    if (index < 0 || index >= m_environmentSprites.size())
        return nullptr;
    return m_environmentSprites[index];
}

Sprite *SpriteManager::itemSprite(const Item *item)
{
    // The rarity tier (0..4) and, for armour, modifier % 5 (five styles of five tiers each) select within a set of
    // consecutive sprites.
    Item appearance = item->displayedItem();
    // Item sprite selection.
    switch (appearance.category) {
    case ItemCategoryConsumable:
        return sprite(consumableSpriteIndex(appearance.id, appearance.rarity));
    case ItemCategoryFormula:
        return sprite(SpriteFormula);
    case ItemCategoryWeapon:
        if (appearance.id == WeaponHandAttacks || appearance.id == WeaponBiteAttack)
            return nullptr;
        return sprite(weaponSpriteIndex(appearance.id, appearance.modifier, appearance.rarity, appearance.material));
    case ItemCategoryChest:
        if (appearance.id == ChestBackpack)
            return nullptr;
        switch (appearance.material) {
        case ItemMaterialObsidian:
            return sprite(SpriteObsidianChest);
        case ItemMaterialGold:
            return sprite(SpriteGoldChest);
        case ItemMaterialIce:
            return sprite(SpriteIceCrystalChest);
        case ItemMaterialLinen:
            return sprite(SpriteLinenChest1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialCotton:
            return sprite(SpriteCottonChest1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialLight:
            return sprite(SpriteLightChest);
        case ItemMaterialDruid:
            return sprite(SpriteDruidChest);
        case ItemMaterialSilk:
            return sprite(SpriteSilkChest1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialBone:
            return sprite(SpriteBoneChest);
        case ItemMaterialClothing:
            switch (appearance.id) {
            case ChestChestArmor:
                return sprite(SpriteInnkeeperChest);
            case ChestBackpack:
                return sprite(SpriteBackpack);
            case ChestBody1:
                return sprite(SpriteBody1);
            case ChestBody2:
                return sprite(SpriteBody2Slot1439);
            case ChestBody3:
                return sprite(SpriteBody3Slot1440);
            case ChestBody4:
                return sprite(SpriteBody4Slot1441);
            case ChestBody5:
                return sprite(SpriteBody5);
            }
            break;
        case ItemMaterialSaurian:
            return sprite(SpriteSaurianChest);
        case ItemMaterialParrot:
            return sprite(SpriteParrotChest);
        case ItemMaterialMammoth:
            return sprite(SpriteMammothChest);
        case ItemMaterialBronze:
            return sprite(SpriteBronzeChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialCalico:
            return sprite(SpriteWoolChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFelt:
            return sprite(SpriteFeltChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFlannell:
            return sprite(SpriteFlannellChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialSteel:
            return sprite(SpriteSteelChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialMail:
            return sprite(SpriteMailChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialRivet:
            return sprite(SpriteRivetChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFilament:
            return sprite(SpriteFilamentChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialSpike:
            return sprite(SpriteSpikeChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFiber:
            return sprite(SpriteFiberChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialScale:
            return sprite(SpriteScaleChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialWeb:
            return sprite(SpriteWebChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialShadow:
            return sprite(SpriteShadowChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialDark:
            return sprite(SpriteDarkChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialDemon:
            return sprite(SpriteDemonChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFire:
            return sprite(SpriteFireChest1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        default:
            return sprite(SpriteIronChest1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        }
        // An unlisted id of material 6 continues with the next category's sprites.
    case ItemCategoryShoulder:
        switch (appearance.material) {
        case ItemMaterialObsidian:
            return sprite(SpriteObsidianShoulders);
        case ItemMaterialGold:
            return sprite(SpriteGoldShoulder);
        case ItemMaterialIce:
            return sprite(SpriteIceCrystalShoulder);
        case ItemMaterialLinen:
            return sprite(SpriteLinenShoulder1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialCotton:
            return sprite(SpriteCottonShoulder1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialLight:
            return sprite(SpriteLightShoulder);
        case ItemMaterialSilk:
            return sprite(SpriteSilkShoulder1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialBone:
            return sprite(SpriteBoneShoulders);
        case ItemMaterialSaurian:
            return sprite(SpriteSaurianShoulder);
        case ItemMaterialParrot:
            return sprite(SpriteParrotShoulder);
        case ItemMaterialMammoth:
            return sprite(SpriteMammothShoulder);
        case ItemMaterialBronze:
            return sprite(SpriteBronzeShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialCalico:
            return sprite(SpriteWoolShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFelt:
            return sprite(SpriteFeltShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFlannell:
            return sprite(SpriteFlannellShoulder1 +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialSteel:
            return sprite(SpriteSteelShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialMail:
            return sprite(SpriteMailShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialRivet:
            return sprite(SpriteRivetShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFilament:
            return sprite(SpriteFilamentShoulder1 +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialSpike:
            return sprite(SpriteSpikeShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFiber:
            return sprite(SpriteFiberShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialScale:
            return sprite(SpriteScaleShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialWeb:
            return sprite(SpriteWebShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialShadow:
            return sprite(SpriteShadowShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialDark:
            return sprite(SpriteDarkShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialDemon:
            return sprite(SpriteDemonShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFire:
            return sprite(SpriteFireShoulder1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        default:
            return sprite(SpriteIronShoulder1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        }
    case ItemCategoryNeck:
        switch (appearance.material) {
        case ItemMaterialSilver:
            return sprite(SpriteSilverAmulet1Random1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        default:
            return sprite(SpriteGoldAmulet1Random1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        }
    case ItemCategoryRing:
        switch (appearance.material) {
        case ItemMaterialSilver:
            return sprite(SpriteSilverRing1Random1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        default:
            return sprite(SpriteGoldRing1Random1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        }
    case ItemCategoryEyePatch:
        switch (appearance.id) {
        case EyePatchFrogmanEyePatch:
            return sprite(SpriteFrogmanEyePatch);
        case EyePatchDwarfFemaleEyePatch:
            return sprite(SpriteDwarfFemaleEyePatch);
        case EyePatchDwarfMaleEyePatch:
            return sprite(SpriteDwarfMaleEyePatch);
        case EyePatchElfEyePatch:
            return sprite(SpriteElfEyePatch);
        case EyePatchGoblinFemaleEyePatch:
            return sprite(SpriteGoblinFemaleEyePatch);
        case EyePatchGoblinMaleEyePatch:
            return sprite(SpriteGoblinMaleEyePatch);
        case EyePatchLizardmanEyePatch:
            return sprite(SpriteLizardmanEyePatch);
        default:
            return sprite(SpriteHumanEyePatch);
        }
    case ItemCategoryHands:
        switch (appearance.material) {
        case ItemMaterialObsidian:
            return sprite(SpriteObsidianGlove);
        case ItemMaterialGold:
            return sprite(SpriteGoldGlove);
        case ItemMaterialIce:
            return sprite(SpriteIceCrystalGlove);
        case ItemMaterialLinen:
            return sprite(SpriteLinenHand1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialCotton:
            return sprite(SpriteCottonHand1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialSilk:
            return sprite(SpriteSilkHand1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialLight:
            return sprite(SpriteLightGlove);
        case ItemMaterialBone:
            return sprite(SpriteBoneGlove);
        case ItemMaterialSaurian:
            return sprite(SpriteSaurianGlove);
        case ItemMaterialParrot:
            return sprite(SpriteParrotGlove);
        case ItemMaterialMammoth:
            return sprite(SpriteMammothGlove);
        case ItemMaterialIron:
            return sprite(SpriteIronHand1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialBronze:
            return sprite(SpriteBronzeHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialCalico:
            return sprite(SpriteWoolHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFelt:
            return sprite(SpriteFeltHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFlannell:
            return sprite(SpriteFlannellHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialSteel:
            return sprite(SpriteSteelHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialMail:
            return sprite(SpriteMailHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialRivet:
            return sprite(SpriteRivetHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFilament:
            return sprite(SpriteFilamentHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialSpike:
            return sprite(SpriteSpikeHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFiber:
            return sprite(SpriteFiberHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialScale:
            return sprite(SpriteScaleHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialWeb:
            return sprite(SpriteWebHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialShadow:
            return sprite(SpriteShadowHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialDark:
            return sprite(SpriteDarkHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialDemon:
            return sprite(SpriteDemonHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFire:
            return sprite(SpriteFireHand1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialWood:
            return sprite(SpriteGlove);
        default:
            return sprite(SpriteGlove);
        }
    case ItemCategoryFeet:
        switch (appearance.material) {
        case ItemMaterialObsidian:
            return sprite(SpriteObsidianBoot);
        case ItemMaterialGold:
            return sprite(SpriteGoldBoot);
        case ItemMaterialIce:
            return sprite(SpriteIceCrystalBoot);
        case ItemMaterialLinen:
            return sprite(SpriteLinenFoot1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialCotton:
            return sprite(SpriteCottonFoot1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialLight:
            return sprite(SpriteLightBoot);
        case ItemMaterialSilk:
            return sprite(SpriteSilkFoot1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialBone:
            return sprite(SpriteBoneBoot);
        case ItemMaterialSaurian:
            return sprite(SpriteSaurianBoot);
        case ItemMaterialParrot:
            return sprite(SpriteParrotBoot);
        case ItemMaterialMammoth:
            return sprite(SpriteMammothBoot);
        case ItemMaterialIron:
            return sprite(SpriteIronFoot1 + appearance.modifier % 5 * ItemRarityCount +
                          std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialBronze:
            return sprite(SpriteBronzeFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialCalico:
            return sprite(SpriteWoolFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFelt:
            return sprite(SpriteFeltFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFlannell:
            return sprite(SpriteFlannellFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialSteel:
            return sprite(SpriteSteelFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialMail:
            return sprite(SpriteMailFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialRivet:
            return sprite(SpriteRivetFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFilament:
            return sprite(SpriteFilamentFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialSpike:
            return sprite(SpriteSpikeFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFiber:
            return sprite(SpriteFiberFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialScale:
            return sprite(SpriteScaleFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialWeb:
            return sprite(SpriteWebFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialShadow:
            return sprite(SpriteShadowFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialDark:
            return sprite(SpriteDarkFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialDemon:
            return sprite(SpriteDemonFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        case ItemMaterialFire:
            return sprite(SpriteFireFoot1 + std::max(0, std::min((int)appearance.rarity, (int)ItemRarityYellow)));
        default:
            return sprite(SpriteBoot);
        }
    case ItemCategoryPet:
        switch (appearance.id) {
        case PetItemEgg:
            return sprite(SpriteEgg1Slot2889);
        case PetItemTurtle:
            return sprite(SpriteTurtleBody);
        default:
            return sprite(SpritePetBox);
        }
    case ItemCategoryPetFood:
        switch (appearance.id) {
        case PetFoodBlueJelly:
            return sprite(SpriteJellyBlue);
        case PetFoodGreenJelly:
            return sprite(SpriteJellyGreen);
        case PetFoodPinkJelly:
            return sprite(SpriteJellyPink);
        case PetFoodYellowJelly:
            return sprite(SpriteJellyYellow);
        case PetFoodCarrot:
            return sprite(SpriteCarrot);
        case PetFoodPumpkinMash:
            return sprite(SpritePumpkinMash);
        case PetFoodCandy:
            return sprite(SpriteCandy);
        case PetFoodLollipop:
            return sprite(SpriteLollipop);
        case PetFoodSoftice:
            return sprite(SpriteSoftice);
        case PetFoodChocolateDonut:
            return sprite(SpriteDonutChocolate);
        case PetFoodCottonCandy:
            return sprite(SpriteCottonCandy);
        case PetFoodPopcorn:
            return sprite(SpritePopcorn);
        case PetFoodCerealBar:
            return sprite(SpriteCerealBar);
        case PetFoodStrawberryCake:
            return sprite(SpriteStrawberryCake);
        case PetFoodChocolateCake:
            return sprite(SpriteChocolateCake);
        case PetFoodVanillaCupcake:
            return sprite(SpriteVanillaCupcake);
        case PetFoodChocolateCupcake:
            return sprite(SpriteChocolateCupcake);
        case PetFoodBananaSplit:
            return sprite(SpriteBananaSplit);
        case PetFoodCroissant:
            return sprite(SpriteCroissant);
        case PetFoodBread:
            return sprite(SpriteBread);
        case PetFoodLolly:
            return sprite(SpriteLolly);
        case PetFoodLemonTart:
            return sprite(SpriteLemonTart);
        case PetFoodChocolateCookie:
            return sprite(SpriteChocolateCookie);
        case PetFoodBubbleGum:
            return sprite(SpriteBubbleGum);
        case PetFoodLicoriceCandy:
            return sprite(SpriteLicoriceCandy);
        case PetFoodCinnamonRole:
            return sprite(SpriteCinnamonRole);
        case PetFoodAppleRing:
            return sprite(SpriteAppleRing);
        case PetFoodWaterIce:
            return sprite(SpriteWaterIce);
        case PetFoodWaffle:
            return sprite(SpriteWaffle);
        case PetFoodDateCookie:
            return sprite(SpriteDateCookie);
        case PetFoodCandiedApple:
            return sprite(SpriteCandiedApple);
        case PetFoodStrawberryCocktail:
            return sprite(SpriteStrawberryCocktail);
        case PetFoodMilkChocolateBar:
            return sprite(SpriteMilkChocolateBar);
        case PetFoodCaramelChocolateBar:
            return sprite(SpriteCaramelChocolateBar);
        case PetFoodMintChocolateBar:
            return sprite(SpriteMintChocolateBar);
        case PetFoodWhiteChocolateBar:
            return sprite(SpriteWhiteChocolateBar);
        case PetFoodSugarCandy:
            return sprite(SpriteSugarCandy);
        case PetFoodBlackberryMarmelade:
            return sprite(SpriteBlackberryMarmelade);
        case PetFoodSaltedCaramel:
            return sprite(SpriteSaltedCaramel);
        case PetFoodGingerTartlet:
            return sprite(SpriteGingerTartlet);
        case PetFoodMangoJuice:
            return sprite(SpriteMangoJuice);
        case PetFoodFruitBasket:
            return sprite(SpriteFruitBasket);
        case PetFoodMelonIceCream:
            return sprite(SpriteMelonIcecream);
        case PetFoodBloodOrangeJuice:
            return sprite(SpriteBloodorangeJuice);
        case PetFoodPancakes:
            return sprite(SpritePancakes);
        case PetFoodCurry:
            return sprite(SpriteCurry);
        case PetFoodBiscuitRole:
            return sprite(SpriteBiscuitRole);
        case PetFoodEucalyptusCandy:
            return sprite(SpriteEucalyptusCandy);
        case PetFoodBuckhorn:
            return sprite(SpriteBuckhorn);
        case PetFoodMineralWater:
            return sprite(SpriteMineralWater);
        case PetFoodSpringWater:
            return sprite(SpriteSpringWater);
        case PetFoodPeanut:
            return sprite(SpritePeanut);
        case PetFoodChocolateIceCream:
            return sprite(SpriteChocolateIceCream);
        case PetFoodRaspberryJuice:
            return sprite(SpriteRaspberryJuice);
        case PetFoodBananaMash:
            return sprite(SpriteBananaMash);
        case PetFoodMixedSalad:
            return sprite(SpriteMixedSalad);
        case PetFoodRadicchioSalad:
            return sprite(SpriteRadicchioSalad);
        case PetFoodCabbageRolls:
            return sprite(SpriteCabbageRolls);
        default:
            return sprite(SpritePineapple);
        }
    case ItemCategoryManaCube:
        return sprite(SpriteManacube);
    case ItemCategoryManaSpirit:
        return sprite(SpriteSpiribit);
    case ItemCategoryGlyph:
        return sprite(SpritePortalStone);
    case ItemCategoryGoldBag:
        return sprite(SpritePouch1);
    case ItemCategoryCoin:
        switch (appearance.material) {
        case ItemMaterialGold:
            return sprite(SpriteGoldCoin);
        case ItemMaterialSilver:
            return sprite(SpriteSilverCoin);
        case ItemMaterialPlatinum:
            return sprite(SpriteSilverCoin);
        default:
            return sprite(SpriteCopperCoin);
        }
    case ItemCategoryPlatinumCoin:
        return sprite(SpriteIceCrystal);
    case ItemCategoryBeak:
        return sprite(SpriteParrotBeak);
    case ItemCategoryMaterial:
        switch (appearance.id) {
        case MaterialItemLog:
            return sprite(SpriteWoodLog);
        case MaterialItemFeather:
            return sprite(SpriteParrotFeather);
        case MaterialItemHair:
            return sprite(SpriteMammothHair);
        case MaterialItemCapsule:
            return sprite(SpriteCotton);
        case MaterialItemFiber:
            return sprite(SpritePlantFiber);
        case MaterialItemYarn:
            return sprite(SpriteIceCoatedYarn);
        case MaterialItemCobweb:
            return sprite(SpriteCobweb);
        case MaterialItemFlask:
            return sprite(SpriteFlask);
        case MaterialItemSpirit:
            switch (appearance.material) {
            case ItemMaterialUnholy:
                return sprite(SpriteUnholyCube);
            case ItemMaterialIceSpirit:
                return sprite(SpriteIceCube);
            case ItemMaterialWind:
                return sprite(SpriteWindCube);
            default:
                return sprite(SpriteFireCube);
            }
        case MaterialItemNugget:
            switch (appearance.material) {
            case ItemMaterialIron:
                return sprite(SpriteIronNugget);
            case ItemMaterialWood:
                return sprite(SpriteWoodLog);
            case ItemMaterialSilver:
                return sprite(SpriteSilverNugget);
            case ItemMaterialGold:
                return sprite(SpriteGoldNugget);
            case ItemMaterialEmerald:
                return sprite(SpriteEmeraldNugget);
            case ItemMaterialSapphire:
                return sprite(SpriteSapphireNugget);
            case ItemMaterialRuby:
                return sprite(SpriteRubyNugget);
            case ItemMaterialDiamond:
                return sprite(SpriteDiamondNugget);
            case ItemMaterialSandstone:
                return sprite(SpriteSandstone);
            default:
                return nullptr;
            }
        case MaterialItemCube:
            switch (appearance.material) {
            case ItemMaterialIron:
                return sprite(SpriteIronCube);
            case ItemMaterialWood:
                return sprite(SpriteWoodCube);
            case ItemMaterialGold:
                return sprite(SpriteGoldCube);
            case ItemMaterialSilver:
                return sprite(SpriteSilverCube);
            default:
                return nullptr;
            }
        case MaterialItemCrystal:
            return sprite(SpriteIceCrystal);
        case MaterialItemOrb:
            return sprite(SpriteSpiribit);
        case MaterialItemMushroom:
            return sprite(SpriteMushroom);
        case MaterialItemPumpkin:
            return sprite(SpritePumpkin);
        case MaterialItemPineapple:
            return sprite(SpritePineapple);
        case MaterialItemSalad:
            return sprite(SpriteSalad);
        case MaterialItemHeartflower:
            return sprite(SpriteHeartflower);
        case MaterialItemFrozenHeartflower:
            return sprite(SpriteHeartflowerFrozen);
        case MaterialItemPricklyPear:
            return sprite(SpritePricklyPear);
        case MaterialItemMindflower:
            return sprite(SpriteMindflower);
        case MaterialItemManaOrchid:
            return sprite(SpriteManaorchid);
        case MaterialItemSpiritflower:
            return sprite(SpriteSpiritflower);
        case MaterialItemSoulFlower:
            return sprite(SpriteSoulFlower);
        case MaterialItemGinsengRoot:
            return sprite(SpriteGinsengRoot);
        case MaterialItemRadishSlice:
            return sprite(SpriteRadishSlice);
        case MaterialItemOnionSlice:
            return sprite(SpriteOnionSlice);
        case MaterialItemShimmerMushroom:
            return sprite(SpriteShimmerMushroom);
        case MaterialItemWaterFlask:
            return sprite(SpriteWaterFlask);
        case MaterialItemSnowBerry:
            return sprite(SpriteSnowBerry);
        case MaterialItemEmerald:
            return sprite(SpriteEmeraldNugget);
        case MaterialItemSapphire:
            return sprite(SpriteSapphireNugget);
        case MaterialItemRuby:
            return sprite(SpriteRubyNugget);
        case MaterialItemDiamond:
            return sprite(SpriteDiamondNugget);
        case MaterialItemDragonRoot:
            return sprite(SpriteDragonRoot);
        case MaterialItemDewdrop:
            return sprite(SpriteDewdrop);
        case MaterialItemSugarCube:
            return sprite(SpriteSugarCube);
        case MaterialItemIceCube:
            return sprite(SpriteIceCube);
        case MaterialItemLemon:
            return sprite(SpriteLemon);
        case MaterialItemBanana:
            return sprite(SpriteBanana);
        case MaterialItemKale:
            return sprite(SpriteKale);
        case MaterialItemCocoaBean:
            return sprite(SpriteCocoaBean);
        default:
            return nullptr;
        }
    case ItemCategoryCube:
        return sprite(SpriteCube);
    case ItemCategoryHouseIcon:
        return sprite(SpriteIconHouse);
    case ItemCategoryQuest:
        switch (appearance.id) {
        case QuestItemGoldKey:
            return sprite(SpriteGoldKey);
        case QuestItemBoneKey:
            return sprite(SpriteBoneKey);
        case QuestItemBronzeKey:
            return sprite(SpriteBronzeKey);
        case QuestItemSilverKey:
            return sprite(SpriteSilverKey);
        case QuestItemSkullKey:
            return sprite(SpriteSkullKey);
        case QuestItemIronKey:
            return sprite(SpriteIronKey);
        case QuestItemRustyKey:
            return sprite(SpriteRustyKey);
        case QuestItemScratchedKey:
            return sprite(SpriteScratchedKey);
        case QuestItemAdornedKey:
            return sprite(SpriteAdornedKey);
        case QuestItemCopperKey:
            return sprite(SpriteCopperKey);
        case QuestItemObsidianKey:
            return sprite(SpriteObsidianKey);
        case QuestItemSteelKey:
            return sprite(SpriteSteelKey);
        case QuestItemTwistedKey:
            return sprite(SpriteTwistedKey);
        case QuestItemSmallKey:
            return sprite(SpriteSmallKey);
        case QuestItemHeavyKey:
            return sprite(SpriteHeavyKey);
        case QuestItemMassiveKey:
            return sprite(SpriteMassiveKey);
        case QuestItemWoodKey:
            return sprite(SpriteWoodKey);
        case QuestItemStoneKey:
            return sprite(SpriteStoneKey);
        case QuestItemTinyKey:
            return sprite(SpriteTinyKey);
        case QuestItemDelicateKey:
            return sprite(SpriteDelicateKey);
        case QuestItemAmulet1:
            return sprite(SpriteQuestItemAmulet01);
        case QuestItemAmulet2:
            return sprite(SpriteQuestItemAmulet02);
        case QuestItemJewelCase:
            return sprite(SpriteQuestItemJewelCase);
        case QuestItemLetter:
            return sprite(SpriteLetter1 + appearance.modifier % 3);
        case QuestItemNote:
            return sprite(SpriteNote1 + appearance.modifier % 3);
        case QuestItemScroll:
            return sprite(SpriteScroll1Slot2674 + appearance.modifier % 3);
        case QuestItemMap:
            return sprite(SpriteScroll1Slot2677 + appearance.modifier % 3);
        case QuestItemTablet:
            return sprite(SpriteTablet1 + appearance.modifier % 3);
        case QuestItemItemTreasure:
            return sprite(SpriteTreasure1 + appearance.modifier % 5);
        case QuestItemItemTreasureStatue:
            return sprite(SpriteTreasureStatue1 + appearance.modifier % 5);
        case QuestItemItemTreasureJewel:
            return sprite(SpriteTreasureJewel1);
        case QuestItemItemTreasureVase:
            return sprite(SpriteTreasureVase1 + appearance.modifier % 2);
        case QuestItemHerb:
            return sprite(SpriteHerb1 + appearance.modifier % 10);
        case QuestItemFlower:
            return sprite(SpriteFlower1 + appearance.modifier % 6);
        case QuestItemDragonDrool:
            return sprite(SpriteDragonDrool);
        case QuestItemFrogSlime:
            return sprite(SpriteFrogSlime);
        case QuestItemGargoyleTooth:
            return sprite(SpriteGargoyleTooth);
        case QuestItemSlimyEye:
            return sprite(SpriteSlimyEye);
        case QuestItemBroom:
            return sprite(SpriteBroom);
        case QuestItemAnchor:
            return sprite(SpriteAnchor);
        case QuestItemWalkingStick:
            return sprite(SpriteWalkingStick);
        case QuestItemTrollFingernail:
            return sprite(SpriteTrollFingernail);
        case QuestItemSpoon:
            return sprite(SpriteSpoon);
        case QuestItemSocks:
            return sprite(SpriteSocks1 + appearance.modifier % 4);
        case QuestItemPicnicBasket:
            return sprite(SpritePicnicBasket);
        case QuestItemPan:
            return sprite(SpritePan);
        case QuestItemLunchBox:
            return sprite(SpriteLunchBox);
        case QuestItemLuckyCharm:
            return sprite(SpriteLuckyCharm);
        case QuestItemBelt:
            return sprite(SpriteBelt);
        case QuestItemRareCrystal:
            return sprite(SpriteRareCrystal1 + appearance.modifier % 4);
        case QuestItemFabricScrap:
            return sprite(SpriteFabricScrap1 + appearance.modifier % 4);
        case QuestItemButton:
            return sprite(SpriteButton1 + appearance.modifier % 4);
        case QuestItemFruit:
            return sprite(SpriteFruit1 + appearance.modifier % 4);
        case QuestItemNut:
            return sprite(SpriteNut1 + appearance.modifier % 4);
        case QuestItemRareFiber:
            return sprite(SpriteRareFiber1 + appearance.modifier % 4);
        case QuestItemRareBlossom:
            return sprite(SpriteRareBlossom1 + appearance.modifier % 4);
        case QuestItemMagicBook:
            return sprite(SpriteMagicBook1 + appearance.modifier % 4);
        case QuestItemSpellBook:
            return sprite(SpriteSpellBook);
        case QuestItemMagicallyLockedBook:
            return sprite(SpriteMagicallyLockedBook);
        case QuestItemBrokenWeapon:
            return sprite(SpriteBrokenWeapon1 + appearance.modifier % 4);
        case QuestItemEnchantedTimber:
            return sprite(SpriteEnchantedTimber1 + appearance.modifier % 5);
        case QuestItemGhostRemains:
            return sprite(SpriteGhostRemains);
        case QuestItemSalve:
            return sprite(SpriteSalve);
        case QuestItemAntivenom:
            return sprite(SpriteAntivenom);
        case QuestItemPainkiller:
            return sprite(SpritePainkiller);
        case QuestItemStaminaPotion:
            return sprite(SpriteStaminaPotion);
        case QuestItemMedicine:
            return sprite(SpriteMedicine);
        case QuestItemBag:
            return sprite(SpriteBag1 + appearance.modifier % 2);
        case QuestItemPouch:
            return sprite(SpritePouch1);
        case QuestItemEternalEmber:
            return sprite(SpriteEternalEmber);
        default:
            return sprite(SpriteQuestItemAmulet01);
        }
    case ItemCategoryLeftovers:
        return sprite(SpriteDigestedLeftovers01 + std::min(3, std::max(0, appearance.rarity - ItemRarityGreen)));
    case ItemCategoryPainting:
        return sprite(SpritePainting01 + appearance.modifier % 4);
    case ItemCategoryVase:
        return sprite(SpriteVase + appearance.modifier % 4);
    case ItemCategoryCandle:
        switch (appearance.id) {
        case CandleItemUndead:
            return sprite(SpriteUndeadCandle1 + appearance.modifier % 3);
        default:
            return sprite(SpriteCandle01 + appearance.modifier % 3);
        }
    case ItemCategorySpecial:
        switch (appearance.id) {
        case SpecialItemHangGlider:
            return sprite(SpriteGlider);
        case SpecialItemBoat:
            return sprite(SpriteBoat2);
        case SpecialItemClimbingSpikes:
            return sprite(SpriteClimbingSpikes);
        case SpecialItemReins:
            return sprite(SpriteReins);
        case SpecialItemFlute:
            return sprite(SpriteFlute);
        case SpecialItemHarp:
            return sprite(SpriteHarp);
        case SpecialItemTreasureSpirit:
            return sprite(SpriteSpiribit);
        case SpecialItemFiddle:
            return sprite(SpriteFiddle);
        case SpecialItemWhistle:
            return sprite(SpriteWhistle);
        case SpecialItemBell:
            return sprite(SpriteBell);
        case SpecialItemLute:
            return sprite(SpriteLute);
        case SpecialItemNightingale:
            return sprite(SpriteNightingale);
        case SpecialItemVoidStone:
            return sprite(SpriteVoidStone);
        case SpecialItemGateKey:
            return sprite(SpriteGoldKey);
        case SpecialItemMagicCraftingBook:
            return sprite(SpriteMagicBook1);
        case SpecialItemRareCraftingBook:
            return sprite(SpriteMagicBook2);
        case SpecialItemEpicCraftingBook:
            return sprite(SpriteMagicBook3);
        case SpecialItemLegendaryCraftingBook:
            return sprite(SpriteMagicBook4);
        default:
            return nullptr;
        }
    case ItemCategoryArtifact:
        return artifactSprite(appearance.id, appearance.modifier);
    case ItemCategoryLamp:
        return sprite(SpriteLantern02);
    case ItemCategoryElixir:
        switch (appearance.id) {
        case ElixirElixirOfLife:
            return sprite(SpriteLifeFlask);
        case ElixirElixirOfPower:
            return sprite(SpriteAttackFlask);
        case ElixirElixirOfToughness:
            return sprite(SpriteArmorFlask);
        case ElixirElixirOfSanity:
            return sprite(SpriteResistanceFlask);
        case ElixirLemonade:
            return sprite(SpriteLemonade);
        case ElixirHotChocolate:
            return sprite(SpriteHotChocolate);
        case ElixirGreenSmoothie:
            return sprite(SpriteGreenSmoothie);
        default:
            return sprite(SpritePetWhistle);
        }
    case ItemCategoryPetCall:
        return sprite(SpritePetWhistle);
    default:
        return nullptr;
    }
}

// Loads the game's sprite models and prepares their meshes.
void SpriteManager::loadSprites(const std::string &directory)
{
    m_sprites.resize(3652);
    for (int i = 0; i < (int)m_sprites.size(); i++)
        m_sprites[i] = new Sprite(*m_graphics, false);

    std::string path;
    if (!directory.empty())
        path = directory + "/";

    // Never set: every model is read from its file.
    Database *database = nullptr;

    m_sprites[SpriteCubeworld]->load(path + "cubeworld.cub", database, true, false);
    m_sprites[SpriteSpiribit]->load(path + "spiribit.cub", database, true, false);
    m_sprites[SpritePetBox]->load(path + "pet-box.cub", database, true, false);
    m_sprites[SpriteEgg1Slot2889]->load(path + "egg1.cub", database, true, false);
    m_sprites[SpriteEgg1Slot2890]->load(path + "egg1.cub", database, true, false);
    m_sprites[SpriteEgg1Slot2891]->load(path + "egg1.cub", database, true, false);
    m_sprites[SpriteFormula]->load(path + "formula.cub", database, true, false);
    m_sprites[SpriteBody2Slot0]->load(path + "body2.cub", database, true, false);
    m_sprites[SpriteBody4Slot1]->load(path + "body4.cub", database, true, false);
    m_sprites[SpriteBody3Slot2]->load(path + "body3.cub", database, true, false);
    m_sprites[SpriteGlove]->load(path + "glove.cub", database, true, false);
    m_sprites[SpriteHead2]->load(path + "head2.cub", database, true, false);
    m_sprites[SpriteOldHead]->load(path + "old-head.cub", database, true, false);
    m_sprites[SpriteGirlHead3]->load(path + "girl-head3.cub", database, true, false);
    m_sprites[SpriteElfHeadFemale]->load(path + "elf-head-female.cub", database, true, false);
    m_sprites[SpriteHand2]->load(path + "hand2.cub", database, true, false);
    m_sprites[SpriteBrownHand]->load(path + "brown-hand.cub", database, true, false);
    m_sprites[SpriteFootSlot615]->load(path + "foot.cub", database, true, false);
    m_sprites[SpriteBoot]->load(path + "boot.cub", database, true, false);
    m_sprites[SpriteTrollFoot]->load(path + "troll-foot.cub", database, true, false);
    m_sprites[SpriteTrollHead]->load(path + "troll-head.cub", database, true, false);
    m_sprites[SpriteTrollBody]->load(path + "troll-body.cub", database, true, false);
    m_sprites[SpriteTrollHand]->load(path + "troll-hand.cub", database, true, false);
    m_sprites[SpriteTrollArm]->load(path + "troll-arm.cub", database, true, false);
    m_sprites[SpriteDarkTrollFoot]->load(path + "dark-troll-foot.cub", database, true, false);
    m_sprites[SpriteDarkTrollHead]->load(path + "dark-troll-head.cub", database, true, false);
    m_sprites[SpriteDarkTrollBody]->load(path + "dark-troll-body.cub", database, true, false);
    m_sprites[SpriteDarkTrollHand]->load(path + "dark-troll-hand.cub", database, true, false);
    m_sprites[SpriteDarkTrollArm]->load(path + "dark-troll-arm.cub", database, true, false);
    m_sprites[SpriteDarkTrollEyes]->load(path + "dark-troll-eyes.cub", database, false, false);
    m_sprites[SpriteHellDemonFoot]->load(path + "hell-demon-foot.cub", database, true, false);
    m_sprites[SpriteHellDemonHead]->load(path + "hell-demon-head.cub", database, true, false);
    m_sprites[SpriteHellDemonBody]->load(path + "hell-demon-body.cub", database, true, false);
    m_sprites[SpriteHellDemonArm]->load(path + "hell-demon-arm.cub", database, true, false);
    m_sprites[SpriteHellDemonHand]->load(path + "hell-demon-hand.cub", database, true, false);
    m_sprites[SpriteGolemFoot]->load(path + "golem-foot.cub", database, true, false);
    m_sprites[SpriteGolemHead]->load(path + "golem-head.cub", database, true, false);
    m_sprites[SpriteGolemBody]->load(path + "golem-body.cub", database, true, false);
    m_sprites[SpriteGolemHand]->load(path + "golem-hand.cub", database, true, false);
    m_sprites[SpriteGolemArm]->load(path + "golem-arm.cub", database, true, false);
    m_sprites[SpriteGolemEmberFoot]->load(path + "golem-ember-foot.cub", database, true, false);
    m_sprites[SpriteGolemEmberHead]->load(path + "golem-ember-head.cub", database, true, false);
    m_sprites[SpriteGolemEmberBody]->load(path + "golem-ember-body.cub", database, true, false);
    m_sprites[SpriteGolemEmberHand]->load(path + "golem-ember-hand.cub", database, true, false);
    m_sprites[SpriteGolemEmberArm]->load(path + "golem-ember-arm.cub", database, true, false);
    m_sprites[SpriteGolemSnowFoot]->load(path + "golem-snow-foot.cub", database, true, false);
    m_sprites[SpriteGolemSnowHead]->load(path + "golem-snow-head.cub", database, true, false);
    m_sprites[SpriteGolemSnowBody]->load(path + "golem-snow-body.cub", database, true, false);
    m_sprites[SpriteGolemSnowHand]->load(path + "golem-snow-hand.cub", database, true, false);
    m_sprites[SpriteGolemSnowArm]->load(path + "golem-snow-arm.cub", database, true, false);
    m_sprites[SpriteYetiFoot]->load(path + "yeti-foot.cub", database, true, false);
    m_sprites[SpriteYetiHead]->load(path + "yeti-head.cub", database, true, false);
    m_sprites[SpriteYetiBody]->load(path + "yeti-body.cub", database, true, false);
    m_sprites[SpriteYetiHand]->load(path + "yeti-hand.cub", database, true, false);
    m_sprites[SpriteYetiArm]->load(path + "yeti-arm.cub", database, true, false);
    m_sprites[SpriteCyclopsFoot]->load(path + "cyclops-foot.cub", database, true, false);
    m_sprites[SpriteCyclopsHead]->load(path + "cyclops-head.cub", database, true, false);
    m_sprites[SpriteCyclopsBody]->load(path + "cyclops-body.cub", database, true, false);
    m_sprites[SpriteCyclopsHand]->load(path + "cyclops-hand.cub", database, true, false);
    m_sprites[SpriteCyclopsArm]->load(path + "cyclops-arm.cub", database, true, false);
    m_sprites[SpriteOgreFoot]->load(path + "ogre-foot.cub", database, true, false);
    m_sprites[SpriteOgreHead]->load(path + "ogre-head.cub", database, true, false);
    m_sprites[SpriteOgreBody]->load(path + "ogre-body.cub", database, true, false);
    m_sprites[SpriteOgreHand]->load(path + "ogre-hand.cub", database, true, false);
    m_sprites[SpriteRocklingFoot]->load(path + "rockling-foot.cub", database, true, false);
    m_sprites[SpriteRocklingHead]->load(path + "rockling-head.cub", database, true, false);
    m_sprites[SpriteRocklingHand]->load(path + "rockling-hand.cub", database, true, false);
    m_sprites[SpriteSpikeFoot]->load(path + "spike-foot.cub", database, true, false);
    m_sprites[SpriteSpikeHead]->load(path + "spike-head.cub", database, true, false);
    m_sprites[SpriteSpikeBody]->load(path + "spike-body.cub", database, true, false);
    m_sprites[SpriteSpikeHand]->load(path + "spike-hand.cub", database, true, false);
    m_sprites[SpriteAnubisFoot]->load(path + "anubis-foot.cub", database, true, false);
    m_sprites[SpriteAnubisHead]->load(path + "anubis-head.cub", database, true, false);
    m_sprites[SpriteAnubisBody]->load(path + "anubis-body.cub", database, true, false);
    m_sprites[SpriteAnubisHand]->load(path + "anubis-hand.cub", database, true, false);
    m_sprites[SpriteHorusFoot]->load(path + "horus-foot.cub", database, true, false);
    m_sprites[SpriteHorusHead]->load(path + "horus-head.cub", database, true, false);
    m_sprites[SpriteHorusBody]->load(path + "horus-body.cub", database, true, false);
    m_sprites[SpriteHorusHand]->load(path + "horus-hand.cub", database, true, false);
    m_sprites[SpriteJesterFoot]->load(path + "jester-foot.cub", database, true, false);
    m_sprites[SpriteJesterHead]->load(path + "jester-head.cub", database, true, false);
    m_sprites[SpriteJesterBody]->load(path + "jester-body.cub", database, true, false);
    m_sprites[SpriteJesterHand]->load(path + "jester-hand.cub", database, true, false);
    m_sprites[SpriteDarkJesterFoot]->load(path + "dark-jester-foot.cub", database, true, false);
    m_sprites[SpriteDarkJesterHead]->load(path + "dark-jester-head.cub", database, true, false);
    m_sprites[SpriteDarkJesterBody]->load(path + "dark-jester-body.cub", database, true, false);
    m_sprites[SpriteDarkJesterHand]->load(path + "dark-jester-hand.cub", database, true, false);
    m_sprites[SpriteSpectrinoFoot]->load(path + "spectrino-foot.cub", database, true, false);
    m_sprites[SpriteSpectrinoHead]->load(path + "spectrino-head.cub", database, true, false);
    m_sprites[SpriteSpectrinoBody]->load(path + "spectrino-body.cub", database, true, false);
    m_sprites[SpriteSpectrinoHand]->load(path + "spectrino-hand.cub", database, true, false);
    m_sprites[SpriteDjinnFoot]->load(path + "djinn-foot.cub", database, true, false);
    m_sprites[SpriteDjinnHead]->load(path + "djinn-head.cub", database, true, false);
    m_sprites[SpriteDjinnBody]->load(path + "djinn-body.cub", database, true, false);
    m_sprites[SpriteDjinnHand]->load(path + "djinn-hand.cub", database, true, false);
    m_sprites[SpriteMinotaurFoot]->load(path + "minotaur-foot.cub", database, true, false);
    m_sprites[SpriteMinotaurHead]->load(path + "minotaur-head.cub", database, true, false);
    m_sprites[SpriteMinotaurBody]->load(path + "minotaur-body.cub", database, true, false);
    m_sprites[SpriteMinotaurHand]->load(path + "minotaur-hand.cub", database, true, false);
    m_sprites[SpriteMinotaurArm]->load(path + "minotaur-arm.cub", database, true, false);
    m_sprites[SpriteDesertNomadFoot]->load(path + "desert-nomad-foot.cub", database, true, false);
    m_sprites[SpriteDesertNomadHead1]->load(path + "desert-nomad-head1.cub", database, true, false);
    m_sprites[SpriteDesertNomadHead2]->load(path + "desert-nomad-head2.cub", database, true, false);
    m_sprites[SpriteDesertNomadHead3]->load(path + "desert-nomad-head3.cub", database, true, false);
    m_sprites[SpriteDesertNomadFemaleHead1]->load(path + "desert-nomad-female-head1.cub", database, true, false);
    m_sprites[SpriteDesertNomadFemaleHead2]->load(path + "desert-nomad-female-head2.cub", database, true, false);
    m_sprites[SpriteDesertNomadFemaleHead3]->load(path + "desert-nomad-female-head3.cub", database, true, false);
    m_sprites[SpriteDesertNomadBody]->load(path + "desert-nomad-body.cub", database, true, false);
    m_sprites[SpriteDesertNomadHand]->load(path + "desert-nomad-hand.cub", database, true, false);
    m_sprites[SpriteDesertNomadHand2]->load(path + "desert-nomad-hand2.cub", database, true, false);
    m_sprites[SpriteDarkCultistFoot]->load(path + "dark-cultist-foot.cub", database, true, false);
    m_sprites[SpriteDarkCultistHead1]->load(path + "dark-cultist-head1.cub", database, true, false);
    m_sprites[SpriteDarkCultistHead2]->load(path + "dark-cultist-head2.cub", database, true, false);
    m_sprites[SpriteDarkCultistHead3]->load(path + "dark-cultist-head3.cub", database, true, false);
    m_sprites[SpriteDarkCultistHead4]->load(path + "dark-cultist-head4.cub", database, true, false);
    m_sprites[SpriteDarkCultistHead5]->load(path + "dark-cultist-head5.cub", database, true, false);
    m_sprites[SpriteDarkCultistFemaleHead1]->load(path + "dark-cultist-female-head1.cub", database, true, false);
    m_sprites[SpriteDarkCultistFemaleHead2]->load(path + "dark-cultist-female-head2.cub", database, true, false);
    m_sprites[SpriteDarkCultistFemaleHead3]->load(path + "dark-cultist-female-head3.cub", database, true, false);
    m_sprites[SpriteDarkCultistFemaleHead4]->load(path + "dark-cultist-female-head4.cub", database, true, false);
    m_sprites[SpriteDarkCultistFemaleHead5]->load(path + "dark-cultist-female-head5.cub", database, true, false);
    m_sprites[SpriteDarkCultistBody]->load(path + "dark-cultist-body.cub", database, true, false);
    m_sprites[SpriteDarkCultistHand]->load(path + "dark-cultist-hand.cub", database, true, false);
    m_sprites[SpriteImpFoot]->load(path + "imp-foot.cub", database, true, false);
    m_sprites[SpriteImpHead]->load(path + "imp-head.cub", database, true, false);
    m_sprites[SpriteImpBody]->load(path + "imp-body.cub", database, true, false);
    m_sprites[SpriteImpHand]->load(path + "imp-hand.cub", database, true, false);
    m_sprites[SpriteImpEyes]->load(path + "imp-eyes.cub", database, false, false);
    m_sprites[SpriteSquidFoot]->load(path + "squid-foot.cub", database, true, false);
    m_sprites[SpriteSquidBody]->load(path + "squid-body.cub", database, true, false);
    m_sprites[SpriteMammothBody]->load(path + "mammoth-body.cub", database, true, false);
    m_sprites[SpriteMammothHead]->load(path + "mammoth-head.cub", database, true, false);
    m_sprites[SpriteMammothFoot]->load(path + "mammoth-foot.cub", database, true, false);
    m_sprites[SpriteElephantBody]->load(path + "elephant-body.cub", database, true, false);
    m_sprites[SpriteElephantHead]->load(path + "elephant-head.cub", database, true, false);
    m_sprites[SpriteElephantFoot]->load(path + "elephant-foot.cub", database, true, false);
    m_sprites[SpriteElephantBabyBody]->load(path + "elephant-baby-body.cub", database, true, false);
    m_sprites[SpriteElephantBabyHead]->load(path + "elephant-baby-head.cub", database, true, false);
    m_sprites[SpriteElephantBabyFoot]->load(path + "elephant-baby-foot.cub", database, true, false);
    m_sprites[SpriteMammothBabyBody]->load(path + "mammoth-baby-body.cub", database, true, false);
    m_sprites[SpriteMammothBabyHead]->load(path + "mammoth-baby-head.cub", database, true, false);
    m_sprites[SpriteMammothBabyFoot]->load(path + "mammoth-baby-foot.cub", database, true, false);
    m_sprites[SpritePirateFoot]->load(path + "pirate-foot.cub", database, true, false);
    m_sprites[SpritePirateHead1]->load(path + "pirate-head1.cub", database, true, false);
    m_sprites[SpritePirateHead2]->load(path + "pirate-head2.cub", database, true, false);
    m_sprites[SpritePirateHead3]->load(path + "pirate-head3.cub", database, true, false);
    m_sprites[SpritePirateHead4]->load(path + "pirate-head4.cub", database, true, false);
    m_sprites[SpritePirateHead5]->load(path + "pirate-head5.cub", database, true, false);
    m_sprites[SpritePirateHead6]->load(path + "pirate-head6.cub", database, true, false);
    m_sprites[SpritePirateHead7]->load(path + "pirate-head7.cub", database, true, false);
    m_sprites[SpritePirateHead8]->load(path + "pirate-head8.cub", database, true, false);
    m_sprites[SpritePirateHead9]->load(path + "pirate-head9.cub", database, true, false);
    m_sprites[SpritePirateFemaleHead1]->load(path + "pirate-female-head1.cub", database, true, false);
    m_sprites[SpritePirateFemaleHead2]->load(path + "pirate-female-head2.cub", database, true, false);
    m_sprites[SpritePirateFemaleHead3]->load(path + "pirate-female-head3.cub", database, true, false);
    m_sprites[SpritePirateFemaleHead4]->load(path + "pirate-female-head4.cub", database, true, false);
    m_sprites[SpritePirateFemaleHead5]->load(path + "pirate-female-head5.cub", database, true, false);
    m_sprites[SpritePirateFemaleHead6]->load(path + "pirate-female-head6.cub", database, true, false);
    m_sprites[SpritePirateFemaleHead7]->load(path + "pirate-female-head7.cub", database, true, false);
    m_sprites[SpritePirateFemaleHead8]->load(path + "pirate-female-head8.cub", database, true, false);
    m_sprites[SpritePirateFemaleHead9]->load(path + "pirate-female-head9.cub", database, true, false);
    m_sprites[SpritePirateBody1]->load(path + "pirate-body1.cub", database, true, false);
    m_sprites[SpritePirateBody2]->load(path + "pirate-body2.cub", database, true, false);
    m_sprites[SpritePirateBody3]->load(path + "pirate-body3.cub", database, true, false);
    m_sprites[SpritePirateBody4]->load(path + "pirate-body4.cub", database, true, false);
    m_sprites[SpritePirateBody5]->load(path + "pirate-body5.cub", database, true, false);
    m_sprites[SpritePirateBody6]->load(path + "pirate-body6.cub", database, true, false);
    m_sprites[SpritePirateBody7]->load(path + "pirate-body7.cub", database, true, false);
    m_sprites[SpritePirateBody8]->load(path + "pirate-body8.cub", database, true, false);
    m_sprites[SpritePirateBody9]->load(path + "pirate-body9.cub", database, true, false);
    m_sprites[SpritePirateHand]->load(path + "pirate-hand.cub", database, true, false);
    m_sprites[SpriteRhinoBody]->load(path + "rhino-body.cub", database, true, false);
    m_sprites[SpriteRhinoHead]->load(path + "rhino-head.cub", database, true, false);
    m_sprites[SpriteRhinoFoot]->load(path + "rhino-foot.cub", database, true, false);
    m_sprites[SpriteLionHeadMale]->load(path + "lion-head-male.cub", database, true, false);
    m_sprites[SpriteLionHeadFemale]->load(path + "lion-head-female.cub", database, true, false);
    m_sprites[SpriteLionBody]->load(path + "lion-body.cub", database, true, false);
    m_sprites[SpriteLionFoot]->load(path + "lion-foot.cub", database, true, false);
    m_sprites[SpriteLionTail]->load(path + "lion-tail.cub", database, true, false);
    m_sprites[SpriteSnowLionHeadMale]->load(path + "snow-lion-head-male.cub", database, true, false);
    m_sprites[SpriteSnowLionHeadFemale]->load(path + "snow-lion-head-female.cub", database, true, false);
    m_sprites[SpriteSnowLionBody]->load(path + "snow-lion-body.cub", database, true, false);
    m_sprites[SpriteSnowLionFoot]->load(path + "snow-lion-foot.cub", database, true, false);
    m_sprites[SpriteSnowLionTail]->load(path + "snow-lion-tail.cub", database, true, false);
    m_sprites[SpriteSilverLionHead]->load(path + "silver-lion-head.cub", database, true, false);
    m_sprites[SpriteSilverLionBody]->load(path + "silver-lion-body.cub", database, true, false);
    m_sprites[SpriteSilverLionFoot]->load(path + "silver-lion-foot.cub", database, true, false);
    m_sprites[SpriteSilverLionTail]->load(path + "silver-lion-tail.cub", database, true, false);
    m_sprites[SpriteBullterrierHead1]->load(path + "bullterrier-head1.cub", database, true, false);
    m_sprites[SpriteBullterrierHead2]->load(path + "bullterrier-head2.cub", database, true, false);
    m_sprites[SpriteBullterrierHead3]->load(path + "bullterrier-head3.cub", database, true, false);
    m_sprites[SpriteGoblinHeadM01]->load(path + "goblin-head-m01.cub", database, true, false);
    m_sprites[SpriteGoblinHeadM02]->load(path + "goblin-head-m02.cub", database, true, false);
    m_sprites[SpriteGoblinHeadM03]->load(path + "goblin-head-m03.cub", database, true, false);
    m_sprites[SpriteGoblinHeadM04]->load(path + "goblin-head-m04.cub", database, true, false);
    m_sprites[SpriteGoblinHeadM05]->load(path + "goblin-head-m05.cub", database, true, false);
    m_sprites[SpriteGoblinHairM01]->load(path + "goblin-hair-m01.cub", database, false, false);
    m_sprites[SpriteGoblinHairM02]->load(path + "goblin-hair-m02.cub", database, false, false);
    m_sprites[SpriteGoblinHairM03]->load(path + "goblin-hair-m03.cub", database, false, false);
    m_sprites[SpriteGoblinHairM04]->load(path + "goblin-hair-m04.cub", database, false, false);
    m_sprites[SpriteGoblinHairM05]->load(path + "goblin-hair-m05.cub", database, false, false);
    m_sprites[SpriteGoblinHairM06]->load(path + "goblin-hair-m06.cub", database, false, false);
    m_sprites[SpriteGoblinHeadF01]->load(path + "goblin-head-f01.cub", database, true, false);
    m_sprites[SpriteGoblinHeadF02]->load(path + "goblin-head-f02.cub", database, true, false);
    m_sprites[SpriteGoblinHeadF03]->load(path + "goblin-head-f03.cub", database, true, false);
    m_sprites[SpriteGoblinHeadF04]->load(path + "goblin-head-f04.cub", database, true, false);
    m_sprites[SpriteGoblinHeadF05]->load(path + "goblin-head-f05.cub", database, true, false);
    m_sprites[SpriteGoblinHairF01]->load(path + "goblin-hair-f01.cub", database, false, false);
    m_sprites[SpriteGoblinHairF02]->load(path + "goblin-hair-f02.cub", database, false, false);
    m_sprites[SpriteGoblinHairF03]->load(path + "goblin-hair-f03.cub", database, false, false);
    m_sprites[SpriteGoblinHairF04]->load(path + "goblin-hair-f04.cub", database, false, false);
    m_sprites[SpriteGoblinHairF05]->load(path + "goblin-hair-f05.cub", database, false, false);
    m_sprites[SpriteGoblinHairF06]->load(path + "goblin-hair-f06.cub", database, false, false);
    m_sprites[SpriteGoblinHand]->load(path + "goblin-hand.cub", database, true, false);
    m_sprites[SpriteElfHeadM01]->load(path + "elf-head-m01.cub", database, true, false);
    m_sprites[SpriteElfHeadM02]->load(path + "elf-head-m02.cub", database, true, false);
    m_sprites[SpriteElfHeadM03]->load(path + "elf-head-m03.cub", database, true, false);
    m_sprites[SpriteElfHeadM04]->load(path + "elf-head-m04.cub", database, true, false);
    m_sprites[SpriteElfHeadF01]->load(path + "elf-head-f01.cub", database, true, false);
    m_sprites[SpriteElfHeadF02]->load(path + "elf-head-f02.cub", database, true, false);
    m_sprites[SpriteElfHeadF03]->load(path + "elf-head-f03.cub", database, true, false);
    m_sprites[SpriteElfHeadF04]->load(path + "elf-head-f04.cub", database, true, false);
    m_sprites[SpriteElfHeadF05]->load(path + "elf-head-f05.cub", database, true, false);
    m_sprites[SpriteElfHeadF06]->load(path + "elf-head-f06.cub", database, true, false);
    m_sprites[SpriteHumanHeadM01]->load(path + "human-head-m01.cub", database, true, false);
    m_sprites[SpriteHumanHeadM02]->load(path + "human-head-m02.cub", database, true, false);
    m_sprites[SpriteHumanHeadM03]->load(path + "human-head-m03.cub", database, true, false);
    m_sprites[SpriteHumanHeadM04]->load(path + "human-head-m04.cub", database, true, false);
    m_sprites[SpriteHumanHeadM05]->load(path + "human-head-m05.cub", database, true, false);
    m_sprites[SpriteHumanHeadM06]->load(path + "human-head-m06.cub", database, true, false);
    m_sprites[SpriteHumanHairM01]->load(path + "human-hair-m01.cub", database, false, false);
    m_sprites[SpriteHumanHairM02]->load(path + "human-hair-m02.cub", database, false, false);
    m_sprites[SpriteHumanHairM03]->load(path + "human-hair-m03.cub", database, false, false);
    m_sprites[SpriteHumanHairM04]->load(path + "human-hair-m04.cub", database, false, false);
    m_sprites[SpriteHumanHairM05]->load(path + "human-hair-m05.cub", database, false, false);
    m_sprites[SpriteHumanHairM06]->load(path + "human-hair-m06.cub", database, false, false);
    m_sprites[SpriteHumanHairM07]->load(path + "human-hair-m07.cub", database, false, false);
    m_sprites[SpriteHumanHairM08]->load(path + "human-hair-m08.cub", database, false, false);
    m_sprites[SpriteHumanHairM09]->load(path + "human-hair-m09.cub", database, false, false);
    m_sprites[SpriteHumanHairM10]->load(path + "human-hair-m10.cub", database, false, false);
    m_sprites[SpriteHumanHairM11]->load(path + "human-hair-m11.cub", database, false, false);
    m_sprites[SpriteHumanHairM12]->load(path + "human-hair-m12.cub", database, false, false);
    m_sprites[SpriteHumanHairM13]->load(path + "human-hair-m13.cub", database, false, false);
    m_sprites[SpriteHumanHairM14]->load(path + "human-hair-m14.cub", database, false, false);
    m_sprites[SpriteHumanHairM15]->load(path + "human-hair-m15.cub", database, false, false);
    m_sprites[SpriteHumanHeadF01]->load(path + "human-head-f01.cub", database, true, false);
    m_sprites[SpriteHumanHeadF02]->load(path + "human-head-f02.cub", database, true, false);
    m_sprites[SpriteHumanHeadF03]->load(path + "human-head-f03.cub", database, true, false);
    m_sprites[SpriteHumanHeadF04]->load(path + "human-head-f04.cub", database, true, false);
    m_sprites[SpriteHumanHeadF05]->load(path + "human-head-f05.cub", database, true, false);
    m_sprites[SpriteHumanHeadF06]->load(path + "human-head-f06.cub", database, true, false);
    m_sprites[SpriteHumanHairF01]->load(path + "human-hair-f01.cub", database, false, false);
    m_sprites[SpriteHumanHairF02]->load(path + "human-hair-f02.cub", database, false, false);
    m_sprites[SpriteHumanHairF03]->load(path + "human-hair-f03.cub", database, false, false);
    m_sprites[SpriteHumanHairF04]->load(path + "human-hair-f04.cub", database, false, false);
    m_sprites[SpriteHumanHairF05]->load(path + "human-hair-f05.cub", database, false, false);
    m_sprites[SpriteHumanHairF06]->load(path + "human-hair-f06.cub", database, false, false);
    m_sprites[SpriteHumanHairF07]->load(path + "human-hair-f07.cub", database, false, false);
    m_sprites[SpriteHumanHairF08]->load(path + "human-hair-f08.cub", database, false, false);
    m_sprites[SpriteHumanHairF09]->load(path + "human-hair-f09.cub", database, false, false);
    m_sprites[SpriteElfHairM01]->load(path + "elf-hair-m01.cub", database, false, false);
    m_sprites[SpriteElfHairM02]->load(path + "elf-hair-m02.cub", database, false, false);
    m_sprites[SpriteElfHairM03]->load(path + "elf-hair-m03.cub", database, false, false);
    m_sprites[SpriteElfHairM04]->load(path + "elf-hair-m04.cub", database, false, false);
    m_sprites[SpriteElfHairM05]->load(path + "elf-hair-m05.cub", database, false, false);
    m_sprites[SpriteElfHairM06]->load(path + "elf-hair-m06.cub", database, false, false);
    m_sprites[SpriteElfHairM07]->load(path + "elf-hair-m07.cub", database, false, false);
    m_sprites[SpriteElfHairM08]->load(path + "elf-hair-m08.cub", database, false, false);
    m_sprites[SpriteElfHairM09]->load(path + "elf-hair-m09.cub", database, false, false);
    m_sprites[SpriteElfHairM10]->load(path + "elf-hair-m10.cub", database, false, false);
    m_sprites[SpriteElfHairF01]->load(path + "elf-hair-f01.cub", database, false, false);
    m_sprites[SpriteElfHairF02]->load(path + "elf-hair-f02.cub", database, false, false);
    m_sprites[SpriteElfHairF03]->load(path + "elf-hair-f03.cub", database, false, false);
    m_sprites[SpriteElfHairF04]->load(path + "elf-hair-f04.cub", database, false, false);
    m_sprites[SpriteElfHairF05]->load(path + "elf-hair-f05.cub", database, false, false);
    m_sprites[SpriteElfHairF06]->load(path + "elf-hair-f06.cub", database, false, false);
    m_sprites[SpriteElfHairF07]->load(path + "elf-hair-f07.cub", database, false, false);
    m_sprites[SpriteElfHairF08]->load(path + "elf-hair-f08.cub", database, false, false);
    m_sprites[SpriteElfHairF09]->load(path + "elf-hair-f09.cub", database, false, false);
    m_sprites[SpriteElfHairF10]->load(path + "elf-hair-f10.cub", database, false, false);
    m_sprites[SpriteElfHairF11]->load(path + "elf-hair-f11.cub", database, false, false);
    m_sprites[SpriteGnoboldFoot]->load(path + "gnobold-foot.cub", database, true, false);
    m_sprites[SpriteGnoboldHand]->load(path + "gnobold-hand.cub", database, true, false);
    m_sprites[SpriteGnoboldHead]->load(path + "gnobold-head.cub", database, true, false);
    m_sprites[SpriteSnowGnoboldFoot]->load(path + "snow-gnobold-foot.cub", database, true, false);
    m_sprites[SpriteSnowGnoboldHand]->load(path + "snow-gnobold-hand.cub", database, true, false);
    m_sprites[SpriteSnowGnoboldHead]->load(path + "snow-gnobold-head.cub", database, true, false);
    m_sprites[SpriteSnowGnoboldBody]->load(path + "snow-gnobold-Body.cub", database, true, false);
    m_sprites[SpriteInsectoidFoot]->load(path + "insectoid-foot.cub", database, true, false);
    m_sprites[SpriteInsectoidHand]->load(path + "insectoid-hand.cub", database, true, false);
    m_sprites[SpriteInsectoidHead]->load(path + "insectoid-head.cub", database, true, false);
    m_sprites[SpriteInsectoidBody]->load(path + "insectoid-body.cub", database, true, false);
    m_sprites[SpriteMantisFoot]->load(path + "mantis-foot.cub", database, true, false);
    m_sprites[SpriteMantisHand]->load(path + "mantis-hand.cub", database, true, false);
    m_sprites[SpriteMantisHead]->load(path + "mantis-head.cub", database, true, false);
    m_sprites[SpriteMantisBody]->load(path + "mantis-body.cub", database, true, false);
    m_sprites[SpritePirateBossFoot]->load(path + "pirate-boss-foot.cub", database, true, false);
    m_sprites[SpritePirateBossHand]->load(path + "pirate-boss-hand.cub", database, true, false);
    m_sprites[SpritePirateBossArm]->load(path + "pirate-boss-arm.cub", database, true, false);
    m_sprites[SpritePirateBossHead]->load(path + "pirate-boss-head.cub", database, true, false);
    m_sprites[SpritePirateBossHead2]->load(path + "pirate-boss-head2.cub", database, true, false);
    m_sprites[SpritePirateBossBody]->load(path + "pirate-boss-body.cub", database, true, false);
    m_sprites[SpriteGnollFoot]->load(path + "gnoll-foot.cub", database, true, false);
    m_sprites[SpriteGnollHand]->load(path + "gnoll-hand.cub", database, true, false);
    m_sprites[SpriteGnollHead]->load(path + "gnoll-head.cub", database, true, false);
    m_sprites[SpriteGnollBody]->load(path + "gnoll-body.cub", database, true, false);
    m_sprites[SpritePolarGnollFoot]->load(path + "polar-gnoll-foot.cub", database, true, false);
    m_sprites[SpritePolarGnollHand]->load(path + "polar-gnoll-hand.cub", database, true, false);
    m_sprites[SpritePolarGnollHead]->load(path + "polar-gnoll-head.cub", database, true, false);
    m_sprites[SpritePolarGnollBody]->load(path + "polar-gnoll-body.cub", database, true, false);
    m_sprites[SpriteMonkeyFoot]->load(path + "monkey-foot.cub", database, true, false);
    m_sprites[SpriteMonkeyHand]->load(path + "monkey-hand.cub", database, true, false);
    m_sprites[SpriteMonkeyHead]->load(path + "monkey-head.cub", database, true, false);
    m_sprites[SpriteMonkeyBody]->load(path + "monkey-body.cub", database, true, false);
    m_sprites[SpriteMakakeFoot]->load(path + "makake-foot.cub", database, true, false);
    m_sprites[SpriteMakakeHand]->load(path + "makake-hand.cub", database, true, false);
    m_sprites[SpriteMakakeHead]->load(path + "makake-head.cub", database, true, false);
    m_sprites[SpriteMakakeBody]->load(path + "makake-body.cub", database, true, false);
    m_sprites[SpriteOrcHeadM01]->load(path + "orc-head-m01.cub", database, true, false);
    m_sprites[SpriteOrcHeadM02]->load(path + "orc-head-m02.cub", database, true, false);
    m_sprites[SpriteOrcHeadM03]->load(path + "orc-head-m03.cub", database, true, false);
    m_sprites[SpriteOrcHeadM04]->load(path + "orc-head-m04.cub", database, true, false);
    m_sprites[SpriteOrcHeadM05]->load(path + "orc-head-m05.cub", database, true, false);
    m_sprites[SpriteOrcHairM01]->load(path + "orc-hair-m01.cub", database, false, false);
    m_sprites[SpriteOrcHairM02]->load(path + "orc-hair-m02.cub", database, false, false);
    m_sprites[SpriteOrcHairM03]->load(path + "orc-hair-m03.cub", database, false, false);
    m_sprites[SpriteOrcHairM04]->load(path + "orc-hair-m04.cub", database, false, false);
    m_sprites[SpriteOrcHairM05]->load(path + "orc-hair-m05.cub", database, false, false);
    m_sprites[SpriteOrcHairM06]->load(path + "orc-hair-m06.cub", database, false, false);
    m_sprites[SpriteOrcHairM07]->load(path + "orc-hair-m07.cub", database, false, false);
    m_sprites[SpriteOrcHairM08]->load(path + "orc-hair-m08.cub", database, false, false);
    m_sprites[SpriteOrcHairF01]->load(path + "orc-hair-f01.cub", database, false, false);
    m_sprites[SpriteOrcHairF02]->load(path + "orc-hair-f02.cub", database, false, false);
    m_sprites[SpriteOrcHairF03]->load(path + "orc-hair-f03.cub", database, false, false);
    m_sprites[SpriteOrcHairF04]->load(path + "orc-hair-f04.cub", database, false, false);
    m_sprites[SpriteFrogmanHeadM01]->load(path + "frogman-head-m01.cub", database, true, false);
    m_sprites[SpriteFrogmanHeadM02]->load(path + "frogman-head-m02.cub", database, true, false);
    m_sprites[SpriteFrogmanHeadM03]->load(path + "frogman-head-m03.cub", database, true, false);
    m_sprites[SpriteFrogmanHeadM04]->load(path + "frogman-head-m04.cub", database, true, false);
    m_sprites[SpriteFrogmanHeadM05]->load(path + "frogman-head-m05.cub", database, true, false);
    m_sprites[SpriteFrogmanHairM01]->load(path + "frogman-hair-m01.cub", database, false, false);
    m_sprites[SpriteFrogmanHairM02]->load(path + "frogman-hair-m02.cub", database, false, false);
    m_sprites[SpriteFrogmanHairM03]->load(path + "frogman-hair-m03.cub", database, false, false);
    m_sprites[SpriteFrogmanHairM04]->load(path + "frogman-hair-m04.cub", database, false, false);
    m_sprites[SpriteFrogmanHairM05]->load(path + "frogman-hair-m05.cub", database, false, false);
    m_sprites[SpriteFrogmanHeadF01]->load(path + "frogman-head-f01.cub", database, true, false);
    m_sprites[SpriteFrogmanHeadF02]->load(path + "frogman-head-f02.cub", database, true, false);
    m_sprites[SpriteFrogmanHeadF03]->load(path + "frogman-head-f03.cub", database, true, false);
    m_sprites[SpriteFrogmanHeadF04]->load(path + "frogman-head-f04.cub", database, true, false);
    m_sprites[SpriteFrogmanHairF01]->load(path + "frogman-hair-f01.cub", database, false, false);
    m_sprites[SpriteFrogmanHairF02]->load(path + "frogman-hair-f02.cub", database, false, false);
    m_sprites[SpriteFrogmanHairF03]->load(path + "frogman-hair-f03.cub", database, false, false);
    m_sprites[SpriteFrogmanHairF04]->load(path + "frogman-hair-f04.cub", database, false, false);
    m_sprites[SpriteFrogmanHand]->load(path + "frogman-hand.cub", database, true, false);
    m_sprites[SpriteMermaidHeadF01]->load(path + "mermaid-head-f01.cub", database, true, false);
    m_sprites[SpriteMermaidHeadF02]->load(path + "mermaid-head-f02.cub", database, true, false);
    m_sprites[SpriteMermaidHeadF03]->load(path + "mermaid-head-f03.cub", database, true, false);
    m_sprites[SpriteMermaidHairF01]->load(path + "mermaid-hair-f01.cub", database, false, false);
    m_sprites[SpriteMermaidHairF02]->load(path + "mermaid-hair-f02.cub", database, false, false);
    m_sprites[SpriteMermaidHairF03]->load(path + "mermaid-hair-f03.cub", database, false, false);
    m_sprites[SpriteMermaidBody]->load(path + "mermaid-body.cub", database, true, false);
    m_sprites[SpriteMermaidHandSlot1563]->load(path + "mermaid-hand.cub", database, true, false);
    m_sprites[SpriteMermanHeadM01]->load(path + "merman-head-m01.cub", database, true, false);
    m_sprites[SpriteMermanHeadM02]->load(path + "merman-head-m02.cub", database, true, false);
    m_sprites[SpriteMermanHeadM03]->load(path + "merman-head-m03.cub", database, true, false);
    m_sprites[SpriteMermanHairM01]->load(path + "merman-hair-m01.cub", database, false, false);
    m_sprites[SpriteMermanHairM02]->load(path + "merman-hair-m02.cub", database, false, false);
    m_sprites[SpriteMermanHairM03]->load(path + "merman-hair-m03.cub", database, false, false);
    m_sprites[SpriteMermanBody]->load(path + "merman-body.cub", database, true, false);
    m_sprites[SpriteMermaidHandSlot1571]->load(path + "mermaid-hand.cub", database, true, false);
    m_sprites[SpriteEmpireSoldierHeadM]->load(path + "empire-soldier-head-m.cub", database, true, false);
    m_sprites[SpriteEmpireSoldierHeadF]->load(path + "empire-soldier-head-f.cub", database, true, false);
    m_sprites[SpriteEmpireSoldierHair01]->load(path + "empire-soldier-hair01.cub", database, false, false);
    m_sprites[SpriteEmpireSoldierHair02]->load(path + "empire-soldier-hair02.cub", database, false, false);
    m_sprites[SpriteEmpireSoldierHair03]->load(path + "empire-soldier-hair03.cub", database, false, false);
    m_sprites[SpriteEmpireSoldierHair04]->load(path + "empire-soldier-hair04.cub", database, false, false);
    m_sprites[SpriteEmpressHead]->load(path + "empress-head.cub", database, true, false);
    m_sprites[SpriteEmpressBody]->load(path + "empress-body.cub", database, true, false);
    m_sprites[SpriteArchdruidHead]->load(path + "archdruid-head.cub", database, true, false);
    m_sprites[SpriteArchdruidBody]->load(path + "archdruid-body.cub", database, true, false);
    m_sprites[SpriteHalfdemonHead]->load(path + "halfdemon-head.cub", database, true, false);
    m_sprites[SpriteHalfdemonHand]->load(path + "halfdemon-hand.cub", database, true, false);
    m_sprites[SpriteGhostPreacherHead]->load(path + "ghost-preacher-head.cub", database, true, false);
    m_sprites[SpriteGhostPreacherBody]->load(path + "ghost-preacher-body.cub", database, true, false);
    m_sprites[SpriteLegendaryPaladinHead]->load(path + "legendary-paladin-head.cub", database, true, false);
    m_sprites[SpriteLegendaryPaladinBody]->load(path + "legendary-paladin-body.cub", database, true, false);
    m_sprites[SpriteAngelHead]->load(path + "angel-head.cub", database, true, false);
    m_sprites[SpriteAngelBody]->load(path + "angel-body.cub", database, true, false);
    m_sprites[SpriteAngelWings]->load(path + "angel-wings.cub", database, true, false);
    m_sprites[SpriteLizardHeadM01]->load(path + "lizard-head-m01.cub", database, true, false);
    m_sprites[SpriteLizardHeadM02]->load(path + "lizard-head-m02.cub", database, true, false);
    m_sprites[SpriteLizardHairM01]->load(path + "lizard-hair-m01.cub", database, false, false);
    m_sprites[SpriteLizardHairM02]->load(path + "lizard-hair-m02.cub", database, false, false);
    m_sprites[SpriteLizardHairM03]->load(path + "lizard-hair-m03.cub", database, false, false);
    m_sprites[SpriteLizardHairM04]->load(path + "lizard-hair-m04.cub", database, false, false);
    m_sprites[SpriteLizardHairM05]->load(path + "lizard-hair-m05.cub", database, false, false);
    m_sprites[SpriteLizardHairM06]->load(path + "lizard-hair-m06.cub", database, false, false);
    m_sprites[SpriteLizardHeadF01]->load(path + "lizard-head-f01.cub", database, true, false);
    m_sprites[SpriteLizardHeadF02]->load(path + "lizard-head-f02.cub", database, true, false);
    m_sprites[SpriteLizardHeadF03]->load(path + "lizard-head-f03.cub", database, true, false);
    m_sprites[SpriteLizardHeadF04]->load(path + "lizard-head-f04.cub", database, true, false);
    m_sprites[SpriteLizardHeadF05]->load(path + "lizard-head-f05.cub", database, true, false);
    m_sprites[SpriteLizardHand]->load(path + "lizard-hand.cub", database, true, false);
    m_sprites[SpriteLizardBody]->load(path + "lizard-body.cub", database, true, false);
    m_sprites[SpriteLizardFoot]->load(path + "lizard-foot.cub", database, true, false);
    m_sprites[SpriteDwarfHeadM01]->load(path + "dwarf-head-m01.cub", database, true, false);
    m_sprites[SpriteDwarfHeadM02]->load(path + "dwarf-head-m02.cub", database, true, false);
    m_sprites[SpriteDwarfHeadM03]->load(path + "dwarf-head-m03.cub", database, true, false);
    m_sprites[SpriteDwarfHeadM04]->load(path + "dwarf-head-m04.cub", database, true, false);
    m_sprites[SpriteDwarfHeadM05]->load(path + "dwarf-head-m05.cub", database, true, false);
    m_sprites[SpriteDwarfHairM01]->load(path + "dwarf-hair-m01.cub", database, false, false);
    m_sprites[SpriteDwarfHairM02]->load(path + "dwarf-hair-m02.cub", database, false, false);
    m_sprites[SpriteDwarfHairM03]->load(path + "dwarf-hair-m03.cub", database, false, false);
    m_sprites[SpriteDwarfHeadF01]->load(path + "dwarf-head-f01.cub", database, true, false);
    m_sprites[SpriteDwarfHeadF02]->load(path + "dwarf-head-f02.cub", database, true, false);
    m_sprites[SpriteDwarfHeadF03]->load(path + "dwarf-head-f03.cub", database, true, false);
    m_sprites[SpriteDwarfHeadF04]->load(path + "dwarf-head-f04.cub", database, true, false);
    m_sprites[SpriteDwarfHeadF05]->load(path + "dwarf-head-f05.cub", database, true, false);
    m_sprites[SpriteDwarfHairF01]->load(path + "dwarf-hair-f01.cub", database, false, false);
    m_sprites[SpriteDwarfHairF02]->load(path + "dwarf-hair-f02.cub", database, false, false);
    m_sprites[SpriteDwarfHairF03]->load(path + "dwarf-hair-f03.cub", database, false, false);
    m_sprites[SpriteDwarfHairF04]->load(path + "dwarf-hair-f04.cub", database, false, false);
    m_sprites[SpriteDwarfHairF05]->load(path + "dwarf-hair-f05.cub", database, false, false);
    m_sprites[SpriteDwarfHairF06]->load(path + "dwarf-hair-f06.cub", database, false, false);
    m_sprites[SpriteDwarfBody]->load(path + "dwarf-body.cub", database, true, false);
    m_sprites[SpriteDwarfBodyFemale]->load(path + "dwarf-body-female.cub", database, true, false);
    m_sprites[SpriteOrcHand]->load(path + "orc-hand.cub", database, true, false);
    m_sprites[SpriteOrcHeadF01]->load(path + "orc-head-f01.cub", database, true, false);
    m_sprites[SpriteOrcHeadF02]->load(path + "orc-head-f02.cub", database, true, false);
    m_sprites[SpriteOrcHeadF03]->load(path + "orc-head-f03.cub", database, true, false);
    m_sprites[SpriteOrcHeadF04]->load(path + "orc-head-f04.cub", database, true, false);
    m_sprites[SpriteOrcHeadF05]->load(path + "orc-head-f05.cub", database, true, false);
    m_sprites[SpriteBloodaxeOrcHand]->load(path + "bloodaxe-orc-hand.cub", database, true, false);
    m_sprites[SpriteBloodaxeOrcHeadM]->load(path + "bloodaxe-orc-head-m.cub", database, true, false);
    m_sprites[SpriteBloodaxeOrcHeadF]->load(path + "bloodaxe-orc-head-f.cub", database, true, false);
    m_sprites[SpriteUndeadHeadM01]->load(path + "undead-head-m01.cub", database, true, false);
    m_sprites[SpriteUndeadHeadM02]->load(path + "undead-head-m02.cub", database, true, false);
    m_sprites[SpriteUndeadHeadM03]->load(path + "undead-head-m03.cub", database, true, false);
    m_sprites[SpriteUndeadHeadM04]->load(path + "undead-head-m04.cub", database, true, false);
    m_sprites[SpriteUndeadHeadM05]->load(path + "undead-head-m05.cub", database, true, false);
    m_sprites[SpriteUndeadHeadM06]->load(path + "undead-head-m06.cub", database, true, false);
    m_sprites[SpriteUndeadHairM01]->load(path + "undead-hair-m01.cub", database, false, false);
    m_sprites[SpriteUndeadHairM02]->load(path + "undead-hair-m02.cub", database, false, false);
    m_sprites[SpriteUndeadHairM03]->load(path + "undead-hair-m03.cub", database, false, false);
    m_sprites[SpriteUndeadHairM04]->load(path + "undead-hair-m04.cub", database, false, false);
    m_sprites[SpriteUndeadHairM05]->load(path + "undead-hair-m05.cub", database, false, false);
    m_sprites[SpriteUndeadHairM06]->load(path + "undead-hair-m06.cub", database, false, false);
    m_sprites[SpriteUndeadHeadF01]->load(path + "undead-head-f01.cub", database, true, false);
    m_sprites[SpriteUndeadHeadF02]->load(path + "undead-head-f02.cub", database, true, false);
    m_sprites[SpriteUndeadHeadF03]->load(path + "undead-head-f03.cub", database, true, false);
    m_sprites[SpriteUndeadHeadF04]->load(path + "undead-head-f04.cub", database, true, false);
    m_sprites[SpriteUndeadHeadF05]->load(path + "undead-head-f05.cub", database, true, false);
    m_sprites[SpriteUndeadHeadF06]->load(path + "undead-head-f06.cub", database, true, false);
    m_sprites[SpriteUndeadHairF01]->load(path + "undead-hair-f01.cub", database, false, false);
    m_sprites[SpriteUndeadHairF02]->load(path + "undead-hair-f02.cub", database, false, false);
    m_sprites[SpriteUndeadHairF03]->load(path + "undead-hair-f03.cub", database, false, false);
    m_sprites[SpriteUndeadHairF04]->load(path + "undead-hair-f04.cub", database, false, false);
    m_sprites[SpriteUndeadHairF05]->load(path + "undead-hair-f05.cub", database, false, false);
    m_sprites[SpriteUndeadHairF06]->load(path + "undead-hair-f06.cub", database, false, false);
    m_sprites[SpriteUndeadHand]->load(path + "undead-hand.cub", database, true, false);
    m_sprites[SpriteSkeletonEyesSlot447]->load(path + "skeleton-eyes.cub", database, false, false);
    m_sprites[SpriteSkeletonHead]->load(path + "skeleton-head.cub", database, true, false);
    m_sprites[SpriteSkeletonHand]->load(path + "skeleton-hand.cub", database, true, false);
    m_sprites[SpriteSkeletonBody]->load(path + "skeleton-body.cub", database, true, false);
    m_sprites[SpriteSkeletonFootSlot450]->load(path + "skeleton-foot.cub", database, true, false);
    m_sprites[SpriteSkeletonEyesSlot455]->load(path + "skeleton-eyes.cub", database, false, false);
    m_sprites[SpriteSkeletonKnightHead1]->load(path + "skeleton-knight-head1.cub", database, true, false);
    m_sprites[SpriteSkeletonKnightHead2]->load(path + "skeleton-knight-head2.cub", database, true, false);
    m_sprites[SpriteSkeletonKnightHead3]->load(path + "skeleton-knight-head3.cub", database, true, false);
    m_sprites[SpriteSkeletonKnightHead4]->load(path + "skeleton-knight-head4.cub", database, true, false);
    m_sprites[SpriteSkeletonKnightHand]->load(path + "skeleton-knight-hand.cub", database, true, false);
    m_sprites[SpriteSkeletonKnightBody]->load(path + "skeleton-knight-body.cub", database, true, false);
    m_sprites[SpriteSkeletonFootSlot458]->load(path + "skeleton-foot.cub", database, true, false);
    m_sprites[SpriteWizardHead]->load(path + "wizard-head.cub", database, true, false);
    m_sprites[SpriteWizardBody]->load(path + "wizard-body.cub", database, true, false);
    m_sprites[SpriteNecromancerHead]->load(path + "necromancer-head.cub", database, true, false);
    m_sprites[SpriteNecromancerBody]->load(path + "necromancer-body.cub", database, true, false);
    m_sprites[SpriteSorcererEyes]->load(path + "sorcerer-eyes.cub", database, false, false);
    m_sprites[SpriteSorcererHead]->load(path + "sorcerer-head.cub", database, true, false);
    m_sprites[SpriteSorcererBody]->load(path + "sorcerer-body.cub", database, true, false);
    m_sprites[SpriteSorcererHand]->load(path + "sorcerer-hand.cub", database, true, false);
    m_sprites[SpriteSeekerEyes]->load(path + "seeker-eyes.cub", database, false, false);
    m_sprites[SpriteSeekerFoot]->load(path + "seeker-foot.cub", database, true, false);
    m_sprites[SpriteSeekerHead]->load(path + "seeker-head.cub", database, true, false);
    m_sprites[SpriteSeekerBody]->load(path + "seeker-body.cub", database, true, false);
    m_sprites[SpriteSeekerHand]->load(path + "seeker-hand.cub", database, true, false);
    m_sprites[SpriteWitchHead]->load(path + "witch-head.cub", database, true, false);
    m_sprites[SpriteWitchBody]->load(path + "witch-body.cub", database, true, false);
    m_sprites[SpriteSantaHead]->load(path + "santa-head.cub", database, true, false);
    m_sprites[SpriteSantaBody]->load(path + "santa-body.cub", database, true, false);
    m_sprites[SpriteWerewolfBody]->load(path + "werewolf-body.cub", database, true, false);
    m_sprites[SpriteWerewolfHand]->load(path + "werewolf-hand.cub", database, true, false);
    m_sprites[SpriteWerewolfHead]->load(path + "werewolf-head.cub", database, true, false);
    m_sprites[SpriteWerewolfFoot]->load(path + "werewolf-foot.cub", database, true, false);
    m_sprites[SpriteDemonMarauderBody]->load(path + "demon-marauder-body.cub", database, true, false);
    m_sprites[SpriteDemonMarauderHand]->load(path + "demon-marauder-hand.cub", database, true, false);
    m_sprites[SpriteDemonMarauderArm]->load(path + "demon-marauder-arm.cub", database, true, false);
    m_sprites[SpriteDemonMarauderHead2]->load(path + "demon-marauder-head2.cub", database, true, false);
    m_sprites[SpriteDemonMarauderFoot]->load(path + "demon-marauder-foot.cub", database, true, false);
    m_sprites[SpriteFlyingEyeHead]->load(path + "flying-eye-head.cub", database, true, false);
    m_sprites[SpriteFlyingEyeHand]->load(path + "flying-eye-hand.cub", database, true, false);
    m_sprites[SpriteHyenaBody]->load(path + "hyena-body.cub", database, true, false);
    m_sprites[SpriteHyenaHand]->load(path + "hyena-hand.cub", database, true, false);
    m_sprites[SpriteHyenaHead]->load(path + "hyena-head.cub", database, true, false);
    m_sprites[SpriteHyenaFoot]->load(path + "hyena-foot.cub", database, true, false);
    m_sprites[SpriteHyenaTail]->load(path + "hyena-tail.cub", database, true, false);
    m_sprites[SpriteZombieBody]->load(path + "zombie-body.cub", database, true, false);
    m_sprites[SpriteZombieHand]->load(path + "zombie-hand.cub", database, true, false);
    m_sprites[SpriteZombieHead]->load(path + "zombie-head.cub", database, true, false);
    m_sprites[SpriteFootSlot345]->load(path + "foot.cub", database, true, false);
    m_sprites[SpriteLichBody]->load(path + "lich-body.cub", database, true, false);
    m_sprites[SpriteLichHand]->load(path + "lich-hand.cub", database, true, false);
    m_sprites[SpriteLichHead]->load(path + "lich-head.cub", database, true, false);
    m_sprites[SpriteLichArm]->load(path + "lich-arm.cub", database, true, false);
    m_sprites[SpriteFrogBody]->load(path + "frog-body.cub", database, true, false);
    m_sprites[SpriteFrogHand]->load(path + "frog-hand.cub", database, true, false);
    m_sprites[SpriteFrogHead]->load(path + "frog-head.cub", database, true, false);
    m_sprites[SpriteFrogFoot]->load(path + "frog-foot.cub", database, true, false);
    m_sprites[SpritePlantCreatureBody]->load(path + "plant-creature-body.cub", database, true, false);
    m_sprites[SpritePlantCreatureHand]->load(path + "plant-creature-hand.cub", database, true, false);
    m_sprites[SpritePlantCreatureHead]->load(path + "plant-creature-head.cub", database, true, false);
    m_sprites[SpritePlantCreatureFoot]->load(path + "plant-creature-foot.cub", database, true, false);
    m_sprites[SpriteIcelingBody]->load(path + "iceling-body.cub", database, true, false);
    m_sprites[SpriteIcelingHand]->load(path + "iceling-hand.cub", database, true, false);
    m_sprites[SpriteIcelingHead]->load(path + "iceling-head.cub", database, true, false);
    m_sprites[SpriteIcelingFoot]->load(path + "iceling-foot.cub", database, true, false);
    m_sprites[SpriteCrawlerBody]->load(path + "crawler-body.cub", database, true, false);
    m_sprites[SpriteCrawlerHead]->load(path + "crawler-head.cub", database, true, false);
    m_sprites[SpriteCrawlerFoot]->load(path + "crawler-foot.cub", database, true, false);
    m_sprites[SpriteRadishCreatureHead]->load(path + "radish-creature-head.cub", database, true, false);
    m_sprites[SpriteRadishCreatureHand]->load(path + "radish-creature-hand.cub", database, true, false);
    m_sprites[SpriteRadishCreatureFoot]->load(path + "radish-creature-foot.cub", database, true, false);
    m_sprites[SpriteRadishCreatureSproutHead]->load(path + "radish-creature-sprout-head.cub", database, true, false);
    m_sprites[SpriteRadishCreatureSproutHand]->load(path + "radish-creature-sprout-hand.cub", database, true, false);
    m_sprites[SpriteRadishCreatureSproutFoot]->load(path + "radish-creature-sprout-foot.cub", database, true, false);
    m_sprites[SpritePlantCreatureSproutHead]->load(path + "plant-creature-sprout-head.cub", database, true, false);
    m_sprites[SpritePlantCreatureSproutHand]->load(path + "plant-creature-sprout-hand.cub", database, true, false);
    m_sprites[SpritePlantCreatureSproutFoot]->load(path + "plant-creature-sprout-foot.cub", database, true, false);
    m_sprites[SpriteOnionlingHead]->load(path + "onionling-head.cub", database, true, false);
    m_sprites[SpriteOnionlingFoot]->load(path + "onionling-foot.cub", database, true, false);
    m_sprites[SpriteChilingHead]->load(path + "chiling-head.cub", database, true, false);
    m_sprites[SpriteChilingHand]->load(path + "chiling-hand.cub", database, true, false);
    m_sprites[SpriteChilingSprout]->load(path + "chiling-sprout.cub", database, true, false);
    m_sprites[SpriteChilingSproutHand]->load(path + "chiling-sprout-hand.cub", database, true, false);
    m_sprites[SpriteHabaneroHead]->load(path + "habanero-head.cub", database, true, false);
    m_sprites[SpriteHabaneroHand]->load(path + "habanero-hand.cub", database, true, false);
    m_sprites[SpriteBloomlingHead]->load(path + "bloomling-head.cub", database, true, false);
    m_sprites[SpriteDesertOnionling]->load(path + "desert-onionling.cub", database, true, false);
    m_sprites[SpriteDesertOnionlingFoot]->load(path + "desert-onionling-foot.cub", database, true, false);
    m_sprites[SpriteDevourerHead]->load(path + "devourer-head.cub", database, true, false);
    m_sprites[SpriteDevourerFoot]->load(path + "devourer-foot.cub", database, true, false);
    m_sprites[SpriteMoleBody]->load(path + "mole-body.cub", database, true, false);
    m_sprites[SpriteMoleHand]->load(path + "mole-hand.cub", database, true, false);
    m_sprites[SpriteMoleHead]->load(path + "mole-head.cub", database, true, false);
    m_sprites[SpriteMoleFoot]->load(path + "mole-foot.cub", database, true, false);
    m_sprites[SpriteBeaverBody]->load(path + "beaver-body.cub", database, true, false);
    m_sprites[SpriteBeaverHand]->load(path + "beaver-hand.cub", database, true, false);
    m_sprites[SpriteBeaverHead]->load(path + "beaver-head.cub", database, true, false);
    m_sprites[SpriteBeaverFoot]->load(path + "beaver-foot.cub", database, true, false);
    m_sprites[SpriteBiterBody]->load(path + "biter-body.cub", database, true, false);
    m_sprites[SpriteBiterHand]->load(path + "biter-hand.cub", database, true, false);
    m_sprites[SpriteBiterHead]->load(path + "biter-head.cub", database, true, false);
    m_sprites[SpriteBiterFoot]->load(path + "biter-foot.cub", database, true, false);
    m_sprites[SpriteKoalaBody]->load(path + "koala-body.cub", database, true, false);
    m_sprites[SpriteKoalaHand]->load(path + "koala-hand.cub", database, true, false);
    m_sprites[SpriteKoalaHead]->load(path + "koala-head.cub", database, true, false);
    m_sprites[SpriteKoalaFoot]->load(path + "koala-foot.cub", database, true, false);
    m_sprites[SpriteSquirrelBody]->load(path + "squirrel-body.cub", database, true, false);
    m_sprites[SpriteSquirrelHand]->load(path + "squirrel-hand.cub", database, true, false);
    m_sprites[SpriteSquirrelHead]->load(path + "squirrel-head.cub", database, true, false);
    m_sprites[SpriteSquirrelFoot]->load(path + "squirrel-foot.cub", database, true, false);
    m_sprites[SpriteRaccoonBody]->load(path + "raccoon-body.cub", database, true, false);
    m_sprites[SpriteRaccoonHand]->load(path + "raccoon-hand.cub", database, true, false);
    m_sprites[SpriteRaccoonHead]->load(path + "raccoon-head.cub", database, true, false);
    m_sprites[SpriteRaccoonFoot]->load(path + "raccoon-foot.cub", database, true, false);
    m_sprites[SpriteOwlHand]->load(path + "owl-hand.cub", database, true, false);
    m_sprites[SpriteOwlHead]->load(path + "owl-head.cub", database, true, false);
    m_sprites[SpriteOwlFoot]->load(path + "owl-foot.cub", database, true, false);
    m_sprites[SpritePenguinBody]->load(path + "penguin-body.cub", database, true, false);
    m_sprites[SpritePenguinHand]->load(path + "penguin-hand.cub", database, true, false);
    m_sprites[SpritePenguinHead]->load(path + "penguin-head.cub", database, true, false);
    m_sprites[SpritePenguinFoot]->load(path + "penguin-foot.cub", database, true, false);
    m_sprites[SpriteCrabBody]->load(path + "crab-body.cub", database, true, false);
    m_sprites[SpriteCrabHand]->load(path + "crab-hand.cub", database, true, false);
    m_sprites[SpriteCrabHead]->load(path + "crab-head.cub", database, true, false);
    m_sprites[SpriteCrabFoot]->load(path + "crab-foot.cub", database, true, false);
    m_sprites[SpriteHornetBody]->load(path + "hornet-body.cub", database, true, false);
    m_sprites[SpriteHornetHand]->load(path + "hornet-hand.cub", database, true, false);
    m_sprites[SpriteHornetHead]->load(path + "hornet-head.cub", database, true, false);
    m_sprites[SpriteHornetFoot]->load(path + "hornet-foot.cub", database, true, false);
    m_sprites[SpriteInsectGuardBody]->load(path + "insect-guard-body.cub", database, true, false);
    m_sprites[SpriteInsectGuardWing]->load(path + "insect-guard-wing.cub", database, true, false);
    m_sprites[SpriteInsectGuardHead]->load(path + "insect-guard-head.cub", database, true, false);
    m_sprites[SpriteInsectGuardHand]->load(path + "insect-guard-hand.cub", database, true, false);
    m_sprites[SpriteInsectGuardFoot]->load(path + "insect-guard-foot.cub", database, true, false);
    m_sprites[SpriteFlyBody]->load(path + "fly-body.cub", database, true, false);
    m_sprites[SpriteFlyHand]->load(path + "fly-hand.cub", database, true, false);
    m_sprites[SpriteFlyHead]->load(path + "fly-head.cub", database, true, false);
    m_sprites[SpriteFlyFoot]->load(path + "fly-foot.cub", database, true, false);
    m_sprites[SpriteBumblebee]->load(path + "bumblebee.cub", database, true, false);
    m_sprites[SpriteBumblebeeHand]->load(path + "bumblebee-hand.cub", database, true, false);
    m_sprites[SpriteBumblebeeFoot]->load(path + "bumblebee-foot.cub", database, true, false);
    m_sprites[SpriteMidgeBody]->load(path + "midge-body.cub", database, true, false);
    m_sprites[SpriteMidgeHand]->load(path + "midge-hand.cub", database, true, false);
    m_sprites[SpriteMidgeHead]->load(path + "midge-head.cub", database, true, false);
    m_sprites[SpriteMidgeFoot]->load(path + "midge-foot.cub", database, true, false);
    m_sprites[SpriteMosquitoBody]->load(path + "mosquito-body.cub", database, true, false);
    m_sprites[SpriteMosquitoHand]->load(path + "mosquito-hand.cub", database, true, false);
    m_sprites[SpriteMosquitoHead]->load(path + "mosquito-head.cub", database, true, false);
    m_sprites[SpriteMosquitoFoot]->load(path + "mosquito-foot.cub", database, true, false);
    m_sprites[SpriteGinseng]->load(path + "ginseng.cub", database, true, false);
    m_sprites[SpriteGinsengRoot]->load(path + "ginseng-root.cub", database, true, false);
    m_sprites[SpriteIdol1]->load(path + "idol1.cub", database, true, false);
    m_sprites[SpriteIdol2]->load(path + "idol2.cub", database, true, false);
    m_sprites[SpriteIdol3]->load(path + "idol3.cub", database, true, false);
    m_sprites[SpriteIdol4]->load(path + "idol4.cub", database, true, false);
    m_sprites[SpriteIdol5]->load(path + "idol5.cub", database, true, false);
    m_sprites[SpriteMagicCrystal1]->load(path + "magic-crystal1.cub", database, true, false);
    m_sprites[SpriteMagicCrystal2]->load(path + "magic-crystal2.cub", database, true, false);
    m_sprites[SpriteMagicCrystal3]->load(path + "magic-crystal3.cub", database, true, false);
    m_sprites[SpriteMagicCrystal4]->load(path + "magic-crystal4.cub", database, true, false);
    m_sprites[SpriteStoneTablet1]->load(path + "stone-tablet1.cub", database, true, false);
    m_sprites[SpriteStoneTablet2]->load(path + "stone-tablet2.cub", database, true, false);
    m_sprites[SpriteStoneTablet3]->load(path + "stone-tablet3.cub", database, true, false);
    m_sprites[SpriteStoneTablet4]->load(path + "stone-tablet4.cub", database, true, false);
    m_sprites[SpriteStoneTablet5]->load(path + "stone-tablet5.cub", database, true, false);
    m_sprites[SpriteStoneTablet6]->load(path + "stone-tablet6.cub", database, true, false);
    m_sprites[SpritePedestal]->load(path + "pedestal.cub", database, true, false);
    m_sprites[SpriteInscribedTablet]->load(path + "inscribed-tablet.cub", database, true, false);
    m_sprites[SpriteSocket]->load(path + "socket.cub", database, true, false);
    m_sprites[SpriteLeverSlot3550]->load(path + "lever.cub", database, true, false);
    m_sprites[SpriteLeverBase]->load(path + "lever-base.cub", database, true, false);
    m_sprites[SpriteLockedLeverBase]->load(path + "locked-lever-base.cub", database, true, false);
    m_sprites[SpriteLockedGate]->load(path + "locked-gate.cub", database, true, false);
    m_sprites[SpriteGoldenGate]->load(path + "golden-gate.cub", database, true, false);
    m_sprites[SpriteRuneGate]->load(path + "rune-gate.cub", database, true, false);
    m_sprites[SpriteRuneGateGlow]->load(path + "rune-gate-glow.cub", database, true, false);
    m_sprites[SpriteStoneGate]->load(path + "stone-gate.cub", database, true, false);
    m_sprites[SpriteObstacleWall]->load(path + "obstacle-wall.cub", database, true, false);
    m_sprites[SpriteSpiderwebWall]->load(path + "spiderweb-wall.cub", database, true, false);
    m_sprites[SpriteMagicBarrier]->load(path + "magic-barrier.cub", database, true, false);
    m_sprites[SpriteGrid]->load(path + "grid.cub", database, true, false);
    m_sprites[SpriteCrowBody]->load(path + "crow-body.cub", database, true, false);
    m_sprites[SpriteCrowHand]->load(path + "crow-hand.cub", database, true, false);
    m_sprites[SpriteCrowHead]->load(path + "crow-head.cub", database, true, false);
    m_sprites[SpriteCrowFoot]->load(path + "crow-foot.cub", database, true, false);
    m_sprites[SpriteChickenBody]->load(path + "chicken-body.cub", database, true, false);
    m_sprites[SpriteChickenHand]->load(path + "chicken-hand.cub", database, true, false);
    m_sprites[SpriteChickenHead]->load(path + "chicken-head.cub", database, true, false);
    m_sprites[SpriteChickenFoot]->load(path + "chicken-foot.cub", database, true, false);
    m_sprites[SpriteParrotBody]->load(path + "parrot-body.cub", database, true, false);
    m_sprites[SpriteParrotHand]->load(path + "parrot-hand.cub", database, true, false);
    m_sprites[SpriteParrotHead]->load(path + "parrot-head.cub", database, true, false);
    m_sprites[SpriteParrotFoot]->load(path + "parrot-foot.cub", database, true, false);
    m_sprites[SpriteParrotBlueBody]->load(path + "parrot-blue-body.cub", database, true, false);
    m_sprites[SpriteParrotBlueHand]->load(path + "parrot-blue-hand.cub", database, true, false);
    m_sprites[SpriteParrotBlueHead]->load(path + "parrot-blue-head.cub", database, true, false);
    m_sprites[SpriteParrotBlueFoot]->load(path + "parrot-blue-foot.cub", database, true, false);
    m_sprites[SpriteBarkbeetleHead]->load(path + "barkbeetle-head.cub", database, true, false);
    m_sprites[SpriteBarkbeetleFoot]->load(path + "barkbeetle-foot.cub", database, true, false);
    m_sprites[SpriteCaterpillarHead]->load(path + "caterpillar-head.cub", database, true, false);
    m_sprites[SpriteEarthCaterpillarHead]->load(path + "earth-caterpillar-head.cub", database, true, false);
    m_sprites[SpriteSnailHead]->load(path + "snail-head.cub", database, true, false);
    m_sprites[SpriteSnailHouse]->load(path + "snail-house.cub", database, true, false);
    m_sprites[SpriteFirebeetleHead]->load(path + "firebeetle-head.cub", database, true, false);
    m_sprites[SpriteFirebeetleFoot]->load(path + "firebeetle-foot.cub", database, true, false);
    m_sprites[SpriteSnoutbeetleHead]->load(path + "snoutbeetle-head.cub", database, true, false);
    m_sprites[SpriteSnoutbeetleFoot]->load(path + "snoutbeetle-foot.cub", database, true, false);
    m_sprites[SpriteLemonbeetleHead]->load(path + "lemonbeetle-head.cub", database, true, false);
    m_sprites[SpriteLemonbeetleFoot]->load(path + "lemonbeetle-foot.cub", database, true, false);
    m_sprites[SpriteSeagullBody]->load(path + "seagull-body.cub", database, true, false);
    m_sprites[SpriteSeagullHand]->load(path + "seagull-hand.cub", database, true, false);
    m_sprites[SpriteSeagullHead]->load(path + "seagull-head.cub", database, true, false);
    m_sprites[SpriteSeagullFoot]->load(path + "seagull-foot.cub", database, true, false);
    m_sprites[SpriteBatBody]->load(path + "bat-body.cub", database, true, false);
    m_sprites[SpriteBatHand]->load(path + "bat-hand.cub", database, true, false);
    m_sprites[SpriteBatHead]->load(path + "bat-head.cub", database, true, false);
    m_sprites[SpriteBatFoot]->load(path + "bat-foot.cub", database, true, false);
    m_sprites[SpritePonyBody]->load(path + "pony-body.cub", database, true, false);
    m_sprites[SpritePonyHead]->load(path + "pony-head.cub", database, true, false);
    m_sprites[SpritePonyFoot]->load(path + "pony-foot.cub", database, true, false);
    m_sprites[SpritePonyTail]->load(path + "pony-tail.cub", database, true, false);
    m_sprites[SpriteGoldKnightHorseBody]->load(path + "gold-knight-horse-body.cub", database, true, false);
    m_sprites[SpriteGoldKnightHorseHead]->load(path + "gold-knight-horse-head.cub", database, true, false);
    m_sprites[SpriteGoldKnightHorseFoot]->load(path + "gold-knight-horse-foot.cub", database, true, false);
    m_sprites[SpriteGoldKnightHorseTail]->load(path + "gold-knight-horse-tail.cub", database, true, false);
    m_sprites[SpriteSkullHorseBody]->load(path + "skull-horse-body.cub", database, true, false);
    m_sprites[SpriteSkullHorseHead]->load(path + "skull-horse-head.cub", database, true, false);
    m_sprites[SpriteSkullHorseFoot]->load(path + "skull-horse-foot.cub", database, true, false);
    m_sprites[SpriteSkullHorseTail]->load(path + "skull-horse-tail.cub", database, true, false);
    m_sprites[SpriteSkullHorseEyes]->load(path + "skull-horse-eyes.cub", database, true, false);
    m_sprites[SpriteZebraBody]->load(path + "zebra-body.cub", database, true, false);
    m_sprites[SpriteZebraHead]->load(path + "zebra-head.cub", database, true, false);
    m_sprites[SpriteZebraFoot]->load(path + "zebra-foot.cub", database, true, false);
    m_sprites[SpriteZebraTail]->load(path + "zebra-tail.cub", database, true, false);
    m_sprites[SpriteCamelBody]->load(path + "camel-body.cub", database, true, false);
    m_sprites[SpriteCamelHead]->load(path + "camel-head.cub", database, true, false);
    m_sprites[SpriteCamelFoot]->load(path + "camel-foot.cub", database, true, false);
    m_sprites[SpriteMooseBody]->load(path + "moose-body.cub", database, true, false);
    m_sprites[SpriteMooseHead]->load(path + "moose-head.cub", database, true, false);
    m_sprites[SpriteMooseFoot]->load(path + "moose-foot.cub", database, true, false);
    m_sprites[SpriteManaDeerBody]->load(path + "mana-deer-body.cub", database, true, false);
    m_sprites[SpriteManaDeerHead]->load(path + "mana-deer-head.cub", database, true, false);
    m_sprites[SpriteManaDeerFoot]->load(path + "mana-deer-foot.cub", database, true, false);
    m_sprites[SpriteManaDeerHand]->load(path + "mana-deer-hand.cub", database, true, false);
    m_sprites[SpriteCowBody]->load(path + "cow-body.cub", database, true, false);
    m_sprites[SpriteCowHead]->load(path + "cow-head.cub", database, true, false);
    m_sprites[SpriteCowFoot]->load(path + "cow-foot.cub", database, true, false);
    m_sprites[SpriteCollieBody]->load(path + "collie-body.cub", database, true, false);
    m_sprites[SpriteCollieHead]->load(path + "collie-head.cub", database, true, false);
    m_sprites[SpriteCollieFoot]->load(path + "collie-foot.cub", database, true, false);
    m_sprites[SpriteCollieHand]->load(path + "collie-hand.cub", database, true, false);
    m_sprites[SpriteCollieTail]->load(path + "collie-tail.cub", database, true, false);
    m_sprites[SpriteSavageDogBody]->load(path + "savage-dog-body.cub", database, true, false);
    m_sprites[SpriteSavageDogHead]->load(path + "savage-dog-head.cub", database, true, false);
    m_sprites[SpriteSavageDogFoot]->load(path + "savage-dog-foot.cub", database, true, false);
    m_sprites[SpriteSavageDogHand]->load(path + "savage-dog-hand.cub", database, true, false);
    m_sprites[SpriteSavageDogTail]->load(path + "savage-dog-tail.cub", database, true, false);
    m_sprites[SpriteAlphaDogBody]->load(path + "alpha-dog-body.cub", database, true, false);
    m_sprites[SpriteAlphaDogHead]->load(path + "alpha-dog-head.cub", database, true, false);
    m_sprites[SpriteAlphaDogFoot]->load(path + "alpha-dog-foot.cub", database, true, false);
    m_sprites[SpriteAlphaDogHand]->load(path + "alpha-dog-hand.cub", database, true, false);
    m_sprites[SpriteAlphaDogTail]->load(path + "alpha-dog-tail.cub", database, true, false);
    m_sprites[SpriteShepherdDogBody]->load(path + "shepherd-dog-body.cub", database, true, false);
    m_sprites[SpriteShepherdDogHead]->load(path + "shepherd-dog-head.cub", database, true, false);
    m_sprites[SpriteShepherdDogFoot]->load(path + "shepherd-dog-foot.cub", database, true, false);
    m_sprites[SpriteShepherdDogHand]->load(path + "shepherd-dog-hand.cub", database, true, false);
    m_sprites[SpriteShepherdDogTail]->load(path + "shepherd-dog-tail.cub", database, true, false);
    m_sprites[SpriteSkeletonDogBody]->load(path + "skeleton-dog-body.cub", database, true, false);
    m_sprites[SpriteSkeletonDogHead]->load(path + "skeleton-dog-head.cub", database, true, false);
    m_sprites[SpriteSkeletonDogFoot]->load(path + "skeleton-dog-foot.cub", database, true, false);
    m_sprites[SpriteSkeletonDogHand]->load(path + "skeleton-dog-hand.cub", database, true, false);
    m_sprites[SpriteSkullBullBody]->load(path + "skull-bull-body.cub", database, true, false);
    m_sprites[SpriteSkullBullHead]->load(path + "skull-bull-head.cub", database, true, false);
    m_sprites[SpriteSkullBullFoot]->load(path + "skull-bull-foot.cub", database, true, false);
    m_sprites[SpriteSkullBullHand]->load(path + "skull-bull-hand.cub", database, true, false);
    m_sprites[SpriteSkullBullTail]->load(path + "skull-bull-tail.cub", database, true, false);
    m_sprites[SpriteAlpacaBody]->load(path + "alpaca-body.cub", database, true, false);
    m_sprites[SpriteAlpacaHead]->load(path + "alpaca-head.cub", database, true, false);
    m_sprites[SpriteAlpacaFoot]->load(path + "alpaca-foot.cub", database, true, false);
    m_sprites[SpriteAlpacaHand]->load(path + "alpaca-hand.cub", database, true, false);
    m_sprites[SpriteAlpacaBrownBody]->load(path + "alpaca-brown-body.cub", database, true, false);
    m_sprites[SpriteAlpacaBrownHead]->load(path + "alpaca-brown-head.cub", database, true, false);
    m_sprites[SpriteAlpacaBrownFoot]->load(path + "alpaca-brown-foot.cub", database, true, false);
    m_sprites[SpriteAlpacaBrownHand]->load(path + "alpaca-brown-hand.cub", database, true, false);
    m_sprites[SpriteTurtleBody]->load(path + "turtle-body.cub", database, true, false);
    m_sprites[SpriteTurtleHead]->load(path + "turtle-head.cub", database, true, false);
    m_sprites[SpriteTurtleFoot]->load(path + "turtle-foot.cub", database, true, false);
    m_sprites[SpriteBlueTurtleBody]->load(path + "blue-turtle-body.cub", database, true, false);
    m_sprites[SpriteBlueTurtleHead]->load(path + "blue-turtle-head.cub", database, true, false);
    m_sprites[SpriteBlueTurtleFoot]->load(path + "blue-turtle-foot.cub", database, true, false);
    m_sprites[SpriteLavaTurtleBody]->load(path + "lava-turtle-body.cub", database, true, false);
    m_sprites[SpriteLavaTurtleHead]->load(path + "lava-turtle-head.cub", database, true, false);
    m_sprites[SpriteLavaTurtleFoot]->load(path + "lava-turtle-foot.cub", database, true, false);
    m_sprites[SpriteDogBody2]->load(path + "dog-body2.cub", database, true, false);
    m_sprites[SpriteDogHead2]->load(path + "dog-head2.cub", database, true, false);
    m_sprites[SpriteDogFoot2]->load(path + "dog-foot2.cub", database, true, false);
    m_sprites[SpriteDogHand2]->load(path + "dog-hand2.cub", database, true, false);
    m_sprites[SpriteDogTail2]->load(path + "dog-tail2.cub", database, true, false);
    m_sprites[SpriteScottishTerrierBody]->load(path + "scottish-terrier-body.cub", database, true, false);
    m_sprites[SpriteScottishTerrierHead]->load(path + "scottish-terrier-head.cub", database, true, false);
    m_sprites[SpriteScottishTerrierFoot]->load(path + "scottish-terrier-foot.cub", database, true, false);
    m_sprites[SpriteScottishTerrierHand]->load(path + "scottish-terrier-hand.cub", database, true, false);
    m_sprites[SpriteScottishTerrierTail]->load(path + "scottish-terrier-tail.cub", database, true, false);
    m_sprites[SpriteWolfBody]->load(path + "wolf-body.cub", database, true, false);
    m_sprites[SpriteWolfHead]->load(path + "wolf-head.cub", database, true, false);
    m_sprites[SpriteWolfFoot]->load(path + "wolf-foot.cub", database, true, false);
    m_sprites[SpriteWolfHand]->load(path + "wolf-hand.cub", database, true, false);
    m_sprites[SpriteWolfTail]->load(path + "wolf-tail.cub", database, true, false);
    m_sprites[SpriteGiraffeBody]->load(path + "giraffe-body.cub", database, true, false);
    m_sprites[SpriteGiraffeHead]->load(path + "giraffe-head.cub", database, true, false);
    m_sprites[SpriteGiraffeFootSlot557]->load(path + "giraffe-foot.cub", database, true, false);
    m_sprites[SpriteGiraffeFootSlot556]->load(path + "giraffe-foot.cub", database, true, false);
    m_sprites[SpriteAntelopeBody]->load(path + "antelope-body.cub", database, true, false);
    m_sprites[SpriteAntelopeHead]->load(path + "antelope-head.cub", database, true, false);
    m_sprites[SpriteAntelopeFootSlot561]->load(path + "antelope-foot.cub", database, true, false);
    m_sprites[SpriteAntelopeFootSlot560]->load(path + "antelope-foot.cub", database, true, false);
    m_sprites[SpritePantherBody]->load(path + "panther-body.cub", database, true, false);
    m_sprites[SpritePantherHead]->load(path + "panther-head.cub", database, true, false);
    m_sprites[SpritePantherFootSlot570]->load(path + "panther-foot.cub", database, true, false);
    m_sprites[SpritePantherFootSlot569]->load(path + "panther-foot.cub", database, true, false);
    m_sprites[SpritePantherTail]->load(path + "panther-tail.cub", database, true, false);
    m_sprites[SpriteSnowLeopardBody]->load(path + "snow-leopard-body.cub", database, true, false);
    m_sprites[SpriteSnowLeopardHead]->load(path + "snow-leopard-head.cub", database, true, false);
    m_sprites[SpriteSnowLeopardFoot]->load(path + "snow-leopard-foot.cub", database, true, false);
    m_sprites[SpriteSnowLeopardHand]->load(path + "snow-leopard-hand.cub", database, true, false);
    m_sprites[SpriteSnowLeopardTail]->load(path + "snow-leopard-tail.cub", database, true, false);
    m_sprites[SpriteCatBody]->load(path + "cat-body.cub", database, true, false);
    m_sprites[SpriteCatHead]->load(path + "cat-head.cub", database, true, false);
    m_sprites[SpriteCatFoot]->load(path + "cat-foot.cub", database, true, false);
    m_sprites[SpriteCatHand]->load(path + "cat-hand.cub", database, true, false);
    m_sprites[SpriteCatTail]->load(path + "cat-tail.cub", database, true, false);
    m_sprites[SpriteCatBody2]->load(path + "cat-body2.cub", database, true, false);
    m_sprites[SpriteCatHead2]->load(path + "cat-head2.cub", database, true, false);
    m_sprites[SpriteCatFoot2]->load(path + "cat-foot2.cub", database, true, false);
    m_sprites[SpriteCatHand2]->load(path + "cat-hand2.cub", database, true, false);
    m_sprites[SpriteCatTail2]->load(path + "cat-tail2.cub", database, true, false);
    m_sprites[SpriteCatBody3]->load(path + "cat-body3.cub", database, true, false);
    m_sprites[SpriteCatHead3]->load(path + "cat-head3.cub", database, true, false);
    m_sprites[SpriteCatFoot3]->load(path + "cat-foot3.cub", database, true, false);
    m_sprites[SpriteCatHand3]->load(path + "cat-hand3.cub", database, true, false);
    m_sprites[SpriteCatTail3]->load(path + "cat-tail3.cub", database, true, false);
    m_sprites[SpriteDragonBody]->load(path + "dragon-body.cub", database, true, false);
    m_sprites[SpriteDragonHead]->load(path + "dragon-head.cub", database, true, false);
    m_sprites[SpriteDragonFoot]->load(path + "dragon-foot.cub", database, true, false);
    m_sprites[SpriteDragonWing]->load(path + "dragon-wing.cub", database, true, false);
    m_sprites[SpriteDragonTail]->load(path + "dragon-tail.cub", database, true, false);
    m_sprites[SpriteCrabBodyBlue]->load(path + "crab-body-blue.cub", database, true, false);
    m_sprites[SpriteCrabHandBlue]->load(path + "crab-hand-blue.cub", database, true, false);
    m_sprites[SpriteCrabHeadBlue]->load(path + "crab-head-blue.cub", database, true, false);
    m_sprites[SpriteCrabFootBlue]->load(path + "crab-foot-blue.cub", database, true, false);
    m_sprites[SpritePlainrunnerBody]->load(path + "plainrunner-body.cub", database, true, false);
    m_sprites[SpritePlainrunnerFoot]->load(path + "plainrunner-foot.cub", database, true, false);
    m_sprites[SpriteLeafrunnerBody]->load(path + "leafrunner-body.cub", database, true, false);
    m_sprites[SpriteLeafrunnerFoot]->load(path + "leafrunner-foot.cub", database, true, false);
    m_sprites[SpriteSnowrunnerBody]->load(path + "snowrunner-body.cub", database, true, false);
    m_sprites[SpriteSnowrunnerFoot]->load(path + "snowrunner-foot.cub", database, true, false);
    m_sprites[SpriteDesertrunnerBody]->load(path + "desertrunner-body.cub", database, true, false);
    m_sprites[SpriteDesertrunnerFoot]->load(path + "desertrunner-foot.cub", database, true, false);
    m_sprites[SpritePeacockBody]->load(path + "peacock-body.cub", database, true, false);
    m_sprites[SpritePeacockFoot]->load(path + "peacock-foot.cub", database, true, false);
    m_sprites[SpritePeacockHead]->load(path + "peacock-head.cub", database, true, false);
    m_sprites[SpriteFlamingoBody]->load(path + "flamingo-body.cub", database, true, false);
    m_sprites[SpriteFlamingoFoot]->load(path + "flamingo-foot.cub", database, true, false);
    m_sprites[SpriteFlamingoHead]->load(path + "flamingo-head.cub", database, true, false);
    m_sprites[SpriteToucanBody]->load(path + "toucan-body.cub", database, true, false);
    m_sprites[SpriteToucanFoot]->load(path + "toucan-foot.cub", database, true, false);
    m_sprites[SpriteToucanHead]->load(path + "toucan-head.cub", database, true, false);
    m_sprites[SpriteOstrichBody]->load(path + "ostrich-body.cub", database, true, false);
    m_sprites[SpriteOstrichFoot]->load(path + "ostrich-foot.cub", database, true, false);
    m_sprites[SpriteOstrichHead]->load(path + "ostrich-head.cub", database, true, false);
    m_sprites[SpriteWoodStaff1]->load(path + "wood-staff1.cub", database, true, false);
    m_sprites[SpriteWoodStaff2]->load(path + "wood-staff2.cub", database, true, false);
    m_sprites[SpriteWoodStaff3]->load(path + "wood-staff3.cub", database, true, false);
    m_sprites[SpriteWoodStaff4]->load(path + "wood-staff4.cub", database, true, false);
    m_sprites[SpriteWoodStaff6]->load(path + "wood-staff6.cub", database, true, false);
    m_sprites[SpriteWoodStaff1Random1]->load(path + "wood-staff1-random1.cub", database, true, false);
    m_sprites[SpriteWoodStaff2Random1]->load(path + "wood-staff2-random1.cub", database, true, false);
    m_sprites[SpriteWoodStaff3Random1]->load(path + "wood-staff3-random1.cub", database, true, false);
    m_sprites[SpriteWoodStaff4Random1]->load(path + "wood-staff4-random1.cub", database, true, false);
    m_sprites[SpriteWoodStaff5Random1]->load(path + "wood-staff5-random1.cub", database, true, false);
    m_sprites[SpriteWoodStaff1Random2]->load(path + "wood-staff1-random2.cub", database, true, false);
    m_sprites[SpriteWoodStaff2Random2]->load(path + "wood-staff2-random2.cub", database, true, false);
    m_sprites[SpriteWoodStaff3Random2]->load(path + "wood-staff3-random2.cub", database, true, false);
    m_sprites[SpriteWoodStaff4Random2]->load(path + "wood-staff4-random2.cub", database, true, false);
    m_sprites[SpriteWoodStaff5Random2]->load(path + "wood-staff5-random2.cub", database, true, false);
    m_sprites[SpriteWoodStaff1Random3]->load(path + "wood-staff1-random3.cub", database, true, false);
    m_sprites[SpriteWoodStaff2Random3]->load(path + "wood-staff2-random3.cub", database, true, false);
    m_sprites[SpriteWoodStaff3Random3]->load(path + "wood-staff3-random3.cub", database, true, false);
    m_sprites[SpriteWoodStaff4Random3]->load(path + "wood-staff4-random3.cub", database, true, false);
    m_sprites[SpriteWoodStaff5Random3]->load(path + "wood-staff5-random3.cub", database, true, false);
    m_sprites[SpriteWoodStaff1Random4]->load(path + "wood-staff1-random4.cub", database, true, false);
    m_sprites[SpriteWoodStaff2Random4]->load(path + "wood-staff2-random4.cub", database, true, false);
    m_sprites[SpriteWoodStaff3Random4]->load(path + "wood-staff3-random4.cub", database, true, false);
    m_sprites[SpriteWoodStaff4Random4]->load(path + "wood-staff4-random4.cub", database, true, false);
    m_sprites[SpriteWoodStaff5Random4]->load(path + "wood-staff5-random4.cub", database, true, false);
    m_sprites[SpriteWoodStaff1Random5]->load(path + "wood-staff1-random5.cub", database, true, false);
    m_sprites[SpriteWoodStaff2Random5]->load(path + "wood-staff2-random5.cub", database, true, false);
    m_sprites[SpriteWoodStaff3Random5]->load(path + "wood-staff3-random5.cub", database, true, false);
    m_sprites[SpriteWoodStaff4Random5]->load(path + "wood-staff4-random5.cub", database, true, false);
    m_sprites[SpriteWoodStaff5Random5]->load(path + "wood-staff5-random5.cub", database, true, false);
    m_sprites[SpriteWoodStaff1Random6]->load(path + "wood-staff1-random6.cub", database, true, false);
    m_sprites[SpriteWoodStaff2Random6]->load(path + "wood-staff2-random6.cub", database, true, false);
    m_sprites[SpriteWoodStaff3Random6]->load(path + "wood-staff3-random6.cub", database, true, false);
    m_sprites[SpriteWoodStaff4Random6]->load(path + "wood-staff4-random6.cub", database, true, false);
    m_sprites[SpriteWoodStaff5Random6]->load(path + "wood-staff5-random6.cub", database, true, false);
    m_sprites[SpriteWoodStaff1Random7]->load(path + "wood-staff1-random7.cub", database, true, false);
    m_sprites[SpriteWoodStaff2Random7]->load(path + "wood-staff2-random7.cub", database, true, false);
    m_sprites[SpriteWoodStaff3Random7]->load(path + "wood-staff3-random7.cub", database, true, false);
    m_sprites[SpriteWoodStaff4Random7]->load(path + "wood-staff4-random7.cub", database, true, false);
    m_sprites[SpriteWoodStaff5Random7]->load(path + "wood-staff5-random7.cub", database, true, false);
    m_sprites[SpriteWoodStaff1Random8]->load(path + "wood-staff1-random8.cub", database, true, false);
    m_sprites[SpriteWoodStaff2Random8]->load(path + "wood-staff2-random8.cub", database, true, false);
    m_sprites[SpriteWoodStaff3Random8]->load(path + "wood-staff3-random8.cub", database, true, false);
    m_sprites[SpriteWoodStaff4Random8]->load(path + "wood-staff4-random8.cub", database, true, false);
    m_sprites[SpriteWoodStaff5Random8]->load(path + "wood-staff5-random8.cub", database, true, false);
    m_sprites[SpriteWoodStaff1Random9]->load(path + "wood-staff1-random9.cub", database, true, false);
    m_sprites[SpriteWoodStaff2Random9]->load(path + "wood-staff2-random9.cub", database, true, false);
    m_sprites[SpriteWoodStaff3Random9]->load(path + "wood-staff3-random9.cub", database, true, false);
    m_sprites[SpriteWoodStaff4Random9]->load(path + "wood-staff4-random9.cub", database, true, false);
    m_sprites[SpriteWoodStaff5Random9]->load(path + "wood-staff5-random9.cub", database, true, false);
    m_sprites[SpriteWoodStaff1Random10]->load(path + "wood-staff1-random10.cub", database, true, false);
    m_sprites[SpriteWoodStaff2Random10]->load(path + "wood-staff2-random10.cub", database, true, false);
    m_sprites[SpriteWoodStaff3Random10]->load(path + "wood-staff3-random10.cub", database, true, false);
    m_sprites[SpriteWoodStaff4Random10]->load(path + "wood-staff4-random10.cub", database, true, false);
    m_sprites[SpriteWoodStaff5Random10]->load(path + "wood-staff5-random10.cub", database, true, false);
    m_sprites[SpriteObsidianStaff1]->load(path + "obsidian-staff1.cub", database, true, false);
    m_sprites[SpriteObsidianStaff2]->load(path + "obsidian-staff2.cub", database, true, false);
    m_sprites[SpriteObsidianStaff3]->load(path + "obsidian-staff3.cub", database, true, false);
    m_sprites[SpriteObsidianStaff4]->load(path + "obsidian-staff4.cub", database, true, false);
    m_sprites[SpriteObsidianStaff5]->load(path + "obsidian-staff5.cub", database, true, false);
    m_sprites[SpriteWoodWand1]->load(path + "wood-wand1.cub", database, true, false);
    m_sprites[SpriteWoodWand2]->load(path + "wood-wand2.cub", database, true, false);
    m_sprites[SpriteWoodWand3]->load(path + "wood-wand3.cub", database, true, false);
    m_sprites[SpriteWoodWand4]->load(path + "wood-wand4.cub", database, true, false);
    m_sprites[SpriteWoodWand5]->load(path + "wood-wand5.cub", database, true, false);
    m_sprites[SpriteWoodWand1Random1]->load(path + "wood-wand1-random1.cub", database, true, false);
    m_sprites[SpriteWoodWand2Random1]->load(path + "wood-wand2-random1.cub", database, true, false);
    m_sprites[SpriteWoodWand3Random1]->load(path + "wood-wand3-random1.cub", database, true, false);
    m_sprites[SpriteWoodWand4Random1]->load(path + "wood-wand4-random1.cub", database, true, false);
    m_sprites[SpriteWoodWand5Random1]->load(path + "wood-wand5-random1.cub", database, true, false);
    m_sprites[SpriteWoodWand1Random2]->load(path + "wood-wand1-random2.cub", database, true, false);
    m_sprites[SpriteWoodWand2Random2]->load(path + "wood-wand2-random2.cub", database, true, false);
    m_sprites[SpriteWoodWand3Random2]->load(path + "wood-wand3-random2.cub", database, true, false);
    m_sprites[SpriteWoodWand4Random2]->load(path + "wood-wand4-random2.cub", database, true, false);
    m_sprites[SpriteWoodWand5Random2]->load(path + "wood-wand5-random2.cub", database, true, false);
    m_sprites[SpriteWoodWand1Random3]->load(path + "wood-wand1-random3.cub", database, true, false);
    m_sprites[SpriteWoodWand2Random3]->load(path + "wood-wand2-random3.cub", database, true, false);
    m_sprites[SpriteWoodWand3Random3]->load(path + "wood-wand3-random3.cub", database, true, false);
    m_sprites[SpriteWoodWand4Random3]->load(path + "wood-wand4-random3.cub", database, true, false);
    m_sprites[SpriteWoodWand5Random3]->load(path + "wood-wand5-random3.cub", database, true, false);
    m_sprites[SpriteWoodWand1Random4]->load(path + "wood-wand1-random4.cub", database, true, false);
    m_sprites[SpriteWoodWand2Random4]->load(path + "wood-wand2-random4.cub", database, true, false);
    m_sprites[SpriteWoodWand3Random4]->load(path + "wood-wand3-random4.cub", database, true, false);
    m_sprites[SpriteWoodWand4Random4]->load(path + "wood-wand4-random4.cub", database, true, false);
    m_sprites[SpriteWoodWand5Random4]->load(path + "wood-wand5-random4.cub", database, true, false);
    m_sprites[SpriteWoodWand1Random5]->load(path + "wood-wand1-random5.cub", database, true, false);
    m_sprites[SpriteWoodWand2Random5]->load(path + "wood-wand2-random5.cub", database, true, false);
    m_sprites[SpriteWoodWand3Random5]->load(path + "wood-wand3-random5.cub", database, true, false);
    m_sprites[SpriteWoodWand4Random5]->load(path + "wood-wand4-random5.cub", database, true, false);
    m_sprites[SpriteWoodWand5Random5]->load(path + "wood-wand5-random5.cub", database, true, false);
    m_sprites[SpriteWoodWand1Random6]->load(path + "wood-wand1-random6.cub", database, true, false);
    m_sprites[SpriteWoodWand2Random6]->load(path + "wood-wand2-random6.cub", database, true, false);
    m_sprites[SpriteWoodWand3Random6]->load(path + "wood-wand3-random6.cub", database, true, false);
    m_sprites[SpriteWoodWand4Random6]->load(path + "wood-wand4-random6.cub", database, true, false);
    m_sprites[SpriteWoodWand5Random6]->load(path + "wood-wand5-random6.cub", database, true, false);
    m_sprites[SpriteWoodWand1Random7]->load(path + "wood-wand1-random7.cub", database, true, false);
    m_sprites[SpriteWoodWand2Random7]->load(path + "wood-wand2-random7.cub", database, true, false);
    m_sprites[SpriteWoodWand3Random7]->load(path + "wood-wand3-random7.cub", database, true, false);
    m_sprites[SpriteWoodWand4Random7]->load(path + "wood-wand4-random7.cub", database, true, false);
    m_sprites[SpriteWoodWand5Random7]->load(path + "wood-wand5-random7.cub", database, true, false);
    m_sprites[SpriteWoodWand1Random8]->load(path + "wood-wand1-random8.cub", database, true, false);
    m_sprites[SpriteWoodWand2Random8]->load(path + "wood-wand2-random8.cub", database, true, false);
    m_sprites[SpriteWoodWand3Random8]->load(path + "wood-wand3-random8.cub", database, true, false);
    m_sprites[SpriteWoodWand4Random8]->load(path + "wood-wand4-random8.cub", database, true, false);
    m_sprites[SpriteWoodWand5Random8]->load(path + "wood-wand5-random8.cub", database, true, false);
    m_sprites[SpriteWoodWand1Random9]->load(path + "wood-wand1-random9.cub", database, true, false);
    m_sprites[SpriteWoodWand2Random9]->load(path + "wood-wand2-random9.cub", database, true, false);
    m_sprites[SpriteWoodWand3Random9]->load(path + "wood-wand3-random9.cub", database, true, false);
    m_sprites[SpriteWoodWand4Random9]->load(path + "wood-wand4-random9.cub", database, true, false);
    m_sprites[SpriteWoodWand5Random9]->load(path + "wood-wand5-random9.cub", database, true, false);
    m_sprites[SpriteWoodWand1Random10]->load(path + "wood-wand1-random10.cub", database, true, false);
    m_sprites[SpriteWoodWand2Random10]->load(path + "wood-wand2-random10.cub", database, true, false);
    m_sprites[SpriteWoodWand3Random10]->load(path + "wood-wand3-random10.cub", database, true, false);
    m_sprites[SpriteWoodWand4Random10]->load(path + "wood-wand4-random10.cub", database, true, false);
    m_sprites[SpriteWoodWand5Random10]->load(path + "wood-wand5-random10.cub", database, true, false);
    m_sprites[SpriteGoldBracelet1]->load(path + "gold-bracelet1.cub", database, true, false);
    m_sprites[SpriteGoldBracelet2]->load(path + "gold-bracelet2.cub", database, true, false);
    m_sprites[SpriteGoldBracelet3]->load(path + "gold-bracelet3.cub", database, true, false);
    m_sprites[SpriteGoldBracelet4]->load(path + "gold-bracelet4.cub", database, true, false);
    m_sprites[SpriteGoldBracelet5]->load(path + "gold-bracelet5.cub", database, true, false);
    m_sprites[SpriteGoldBracelet1Random1]->load(path + "gold-bracelet1-random1.cub", database, true, false);
    m_sprites[SpriteGoldBracelet2Random1]->load(path + "gold-bracelet2-random1.cub", database, true, false);
    m_sprites[SpriteGoldBracelet3Random1]->load(path + "gold-bracelet3-random1.cub", database, true, false);
    m_sprites[SpriteGoldBracelet4Random1]->load(path + "gold-bracelet4-random1.cub", database, true, false);
    m_sprites[SpriteGoldBracelet5Random1]->load(path + "gold-bracelet5-random1.cub", database, true, false);
    m_sprites[SpriteGoldBracelet1Random2]->load(path + "gold-bracelet1-random2.cub", database, true, false);
    m_sprites[SpriteGoldBracelet2Random2]->load(path + "gold-bracelet2-random2.cub", database, true, false);
    m_sprites[SpriteGoldBracelet3Random2]->load(path + "gold-bracelet3-random2.cub", database, true, false);
    m_sprites[SpriteGoldBracelet4Random2]->load(path + "gold-bracelet4-random2.cub", database, true, false);
    m_sprites[SpriteGoldBracelet5Random2]->load(path + "gold-bracelet5-random2.cub", database, true, false);
    m_sprites[SpriteGoldBracelet1Random3]->load(path + "gold-bracelet1-random3.cub", database, true, false);
    m_sprites[SpriteGoldBracelet2Random3]->load(path + "gold-bracelet2-random3.cub", database, true, false);
    m_sprites[SpriteGoldBracelet3Random3]->load(path + "gold-bracelet3-random3.cub", database, true, false);
    m_sprites[SpriteGoldBracelet4Random3]->load(path + "gold-bracelet4-random3.cub", database, true, false);
    m_sprites[SpriteGoldBracelet5Random3]->load(path + "gold-bracelet5-random3.cub", database, true, false);
    m_sprites[SpriteGoldBracelet1Random4]->load(path + "gold-bracelet1-random4.cub", database, true, false);
    m_sprites[SpriteGoldBracelet2Random4]->load(path + "gold-bracelet2-random4.cub", database, true, false);
    m_sprites[SpriteGoldBracelet3Random4]->load(path + "gold-bracelet3-random4.cub", database, true, false);
    m_sprites[SpriteGoldBracelet4Random4]->load(path + "gold-bracelet4-random4.cub", database, true, false);
    m_sprites[SpriteGoldBracelet5Random4]->load(path + "gold-bracelet5-random4.cub", database, true, false);
    m_sprites[SpriteGoldBracelet1Random5]->load(path + "gold-bracelet1-random5.cub", database, true, false);
    m_sprites[SpriteGoldBracelet2Random5]->load(path + "gold-bracelet2-random5.cub", database, true, false);
    m_sprites[SpriteGoldBracelet3Random5]->load(path + "gold-bracelet3-random5.cub", database, true, false);
    m_sprites[SpriteGoldBracelet4Random5]->load(path + "gold-bracelet4-random5.cub", database, true, false);
    m_sprites[SpriteGoldBracelet5Random5]->load(path + "gold-bracelet5-random5.cub", database, true, false);
    m_sprites[SpriteSilverBracelet1]->load(path + "silver-bracelet1.cub", database, true, false);
    m_sprites[SpriteSilverBracelet2]->load(path + "silver-bracelet2.cub", database, true, false);
    m_sprites[SpriteSilverBracelet3]->load(path + "silver-bracelet3.cub", database, true, false);
    m_sprites[SpriteSilverBracelet4]->load(path + "silver-bracelet4.cub", database, true, false);
    m_sprites[SpriteSilverBracelet5]->load(path + "silver-bracelet5.cub", database, true, false);
    m_sprites[SpriteSilverBracelet1Random1]->load(path + "silver-bracelet1-random1.cub", database, true, false);
    m_sprites[SpriteSilverBracelet2Random1]->load(path + "silver-bracelet2-random1.cub", database, true, false);
    m_sprites[SpriteSilverBracelet3Random1]->load(path + "silver-bracelet3-random1.cub", database, true, false);
    m_sprites[SpriteSilverBracelet4Random1]->load(path + "silver-bracelet4-random1.cub", database, true, false);
    m_sprites[SpriteSilverBracelet5Random1]->load(path + "silver-bracelet5-random1.cub", database, true, false);
    m_sprites[SpriteSilverBracelet1Random2]->load(path + "silver-bracelet1-random2.cub", database, true, false);
    m_sprites[SpriteSilverBracelet2Random2]->load(path + "silver-bracelet2-random2.cub", database, true, false);
    m_sprites[SpriteSilverBracelet3Random2]->load(path + "silver-bracelet3-random2.cub", database, true, false);
    m_sprites[SpriteSilverBracelet4Random2]->load(path + "silver-bracelet4-random2.cub", database, true, false);
    m_sprites[SpriteSilverBracelet5Random2]->load(path + "silver-bracelet5-random2.cub", database, true, false);
    m_sprites[SpriteSilverBracelet1Random3]->load(path + "silver-bracelet1-random3.cub", database, true, false);
    m_sprites[SpriteSilverBracelet2Random3]->load(path + "silver-bracelet2-random3.cub", database, true, false);
    m_sprites[SpriteSilverBracelet3Random3]->load(path + "silver-bracelet3-random3.cub", database, true, false);
    m_sprites[SpriteSilverBracelet4Random3]->load(path + "silver-bracelet4-random3.cub", database, true, false);
    m_sprites[SpriteSilverBracelet5Random3]->load(path + "silver-bracelet5-random3.cub", database, true, false);
    m_sprites[SpriteSilverBracelet1Random4]->load(path + "silver-bracelet1-random4.cub", database, true, false);
    m_sprites[SpriteSilverBracelet2Random4]->load(path + "silver-bracelet2-random4.cub", database, true, false);
    m_sprites[SpriteSilverBracelet3Random4]->load(path + "silver-bracelet3-random4.cub", database, true, false);
    m_sprites[SpriteSilverBracelet4Random4]->load(path + "silver-bracelet4-random4.cub", database, true, false);
    m_sprites[SpriteSilverBracelet5Random4]->load(path + "silver-bracelet5-random4.cub", database, true, false);
    m_sprites[SpriteSilverBracelet1Random5]->load(path + "silver-bracelet1-random5.cub", database, true, false);
    m_sprites[SpriteSilverBracelet2Random5]->load(path + "silver-bracelet2-random5.cub", database, true, false);
    m_sprites[SpriteSilverBracelet3Random5]->load(path + "silver-bracelet3-random5.cub", database, true, false);
    m_sprites[SpriteSilverBracelet4Random5]->load(path + "silver-bracelet4-random5.cub", database, true, false);
    m_sprites[SpriteSilverBracelet5Random5]->load(path + "silver-bracelet5-random5.cub", database, true, false);
    m_sprites[SpriteIronMace1]->load(path + "iron-mace1.cub", database, true, false);
    m_sprites[SpriteIronMace2]->load(path + "iron-mace2.cub", database, true, false);
    m_sprites[SpriteIronMace3]->load(path + "iron-mace3.cub", database, true, false);
    m_sprites[SpriteIronMace4]->load(path + "iron-mace4.cub", database, true, false);
    m_sprites[SpriteIronMace5]->load(path + "iron-mace5.cub", database, true, false);
    m_sprites[SpriteIronMace1Random1]->load(path + "iron-mace1-random1.cub", database, true, false);
    m_sprites[SpriteIronMace2Random1]->load(path + "iron-mace2-random1.cub", database, true, false);
    m_sprites[SpriteIronMace3Random1]->load(path + "iron-mace3-random1.cub", database, true, false);
    m_sprites[SpriteIronMace4Random1]->load(path + "iron-mace4-random1.cub", database, true, false);
    m_sprites[SpriteIronMace5Random1]->load(path + "iron-mace5-random1.cub", database, true, false);
    m_sprites[SpriteIronMace1Random2]->load(path + "iron-mace1-random2.cub", database, true, false);
    m_sprites[SpriteIronMace2Random2]->load(path + "iron-mace2-random2.cub", database, true, false);
    m_sprites[SpriteIronMace3Random2]->load(path + "iron-mace3-random2.cub", database, true, false);
    m_sprites[SpriteIronMace4Random2]->load(path + "iron-mace4-random2.cub", database, true, false);
    m_sprites[SpriteIronMace5Random2]->load(path + "iron-mace5-random2.cub", database, true, false);
    m_sprites[SpriteIronMace1Random3]->load(path + "iron-mace1-random3.cub", database, true, false);
    m_sprites[SpriteIronMace2Random3]->load(path + "iron-mace2-random3.cub", database, true, false);
    m_sprites[SpriteIronMace3Random3]->load(path + "iron-mace3-random3.cub", database, true, false);
    m_sprites[SpriteIronMace4Random3]->load(path + "iron-mace4-random3.cub", database, true, false);
    m_sprites[SpriteIronMace5Random3]->load(path + "iron-mace5-random3.cub", database, true, false);
    m_sprites[SpriteIronMace1Random4]->load(path + "iron-mace1-random4.cub", database, true, false);
    m_sprites[SpriteIronMace2Random4]->load(path + "iron-mace2-random4.cub", database, true, false);
    m_sprites[SpriteIronMace3Random4]->load(path + "iron-mace3-random4.cub", database, true, false);
    m_sprites[SpriteIronMace4Random4]->load(path + "iron-mace4-random4.cub", database, true, false);
    m_sprites[SpriteIronMace5Random4]->load(path + "iron-mace5-random4.cub", database, true, false);
    m_sprites[SpriteIronMace1Random5]->load(path + "iron-mace1-random5.cub", database, true, false);
    m_sprites[SpriteIronMace2Random5]->load(path + "iron-mace2-random5.cub", database, true, false);
    m_sprites[SpriteIronMace3Random5]->load(path + "iron-mace3-random5.cub", database, true, false);
    m_sprites[SpriteIronMace4Random5]->load(path + "iron-mace4-random5.cub", database, true, false);
    m_sprites[SpriteIronMace5Random5]->load(path + "iron-mace5-random5.cub", database, true, false);
    m_sprites[SpriteIronMace1Random6]->load(path + "iron-mace1-random6.cub", database, true, false);
    m_sprites[SpriteIronMace2Random6]->load(path + "iron-mace2-random6.cub", database, true, false);
    m_sprites[SpriteIronMace3Random6]->load(path + "iron-mace3-random6.cub", database, true, false);
    m_sprites[SpriteIronMace4Random6]->load(path + "iron-mace4-random6.cub", database, true, false);
    m_sprites[SpriteIronMace5Random6]->load(path + "iron-mace5-random6.cub", database, true, false);
    m_sprites[SpriteIronMace1Random7]->load(path + "iron-mace1-random7.cub", database, true, false);
    m_sprites[SpriteIronMace2Random7]->load(path + "iron-mace2-random7.cub", database, true, false);
    m_sprites[SpriteIronMace3Random7]->load(path + "iron-mace3-random7.cub", database, true, false);
    m_sprites[SpriteIronMace4Random7]->load(path + "iron-mace4-random7.cub", database, true, false);
    m_sprites[SpriteIronMace5Random7]->load(path + "iron-mace5-random7.cub", database, true, false);
    m_sprites[SpriteIronMace1Random8]->load(path + "iron-mace1-random8.cub", database, true, false);
    m_sprites[SpriteIronMace2Random8]->load(path + "iron-mace2-random8.cub", database, true, false);
    m_sprites[SpriteIronMace3Random8]->load(path + "iron-mace3-random8.cub", database, true, false);
    m_sprites[SpriteIronMace4Random8]->load(path + "iron-mace4-random8.cub", database, true, false);
    m_sprites[SpriteIronMace5Random8]->load(path + "iron-mace5-random8.cub", database, true, false);
    m_sprites[SpriteIronMace1Random9]->load(path + "iron-mace1-random9.cub", database, true, false);
    m_sprites[SpriteIronMace2Random9]->load(path + "iron-mace2-random9.cub", database, true, false);
    m_sprites[SpriteIronMace3Random9]->load(path + "iron-mace3-random9.cub", database, true, false);
    m_sprites[SpriteIronMace4Random9]->load(path + "iron-mace4-random9.cub", database, true, false);
    m_sprites[SpriteIronMace5Random9]->load(path + "iron-mace5-random9.cub", database, true, false);
    m_sprites[SpriteIronMace1Random10]->load(path + "iron-mace1-random10.cub", database, true, false);
    m_sprites[SpriteIronMace2Random10]->load(path + "iron-mace2-random10.cub", database, true, false);
    m_sprites[SpriteIronMace3Random10]->load(path + "iron-mace3-random10.cub", database, true, false);
    m_sprites[SpriteIronMace4Random10]->load(path + "iron-mace4-random10.cub", database, true, false);
    m_sprites[SpriteIronMace5Random10]->load(path + "iron-mace5-random10.cub", database, true, false);
    m_sprites[SpriteWoodMace01]->load(path + "wood-mace01.cub", database, true, false);
    m_sprites[SpriteWoodMace02]->load(path + "wood-mace02.cub", database, true, false);
    m_sprites[SpriteWoodMace03]->load(path + "wood-mace03.cub", database, true, false);
    m_sprites[SpriteWoodMace04]->load(path + "wood-mace04.cub", database, true, false);
    m_sprites[SpriteWoodMace05]->load(path + "wood-mace05.cub", database, true, false);
    m_sprites[SpriteBow]->load(path + "bow.cub", database, true, false);
    m_sprites[SpriteWoodBow2]->load(path + "wood-bow2.cub", database, true, false);
    m_sprites[SpriteWoodBow3]->load(path + "wood-bow3.cub", database, true, false);
    m_sprites[SpriteWoodBow4]->load(path + "wood-bow4.cub", database, true, false);
    m_sprites[SpriteWoodBow5]->load(path + "wood-bow5.cub", database, true, false);
    m_sprites[SpriteWoodBow1Random1]->load(path + "wood-bow1-random1.cub", database, true, false);
    m_sprites[SpriteWoodBow2Random1]->load(path + "wood-bow2-random1.cub", database, true, false);
    m_sprites[SpriteWoodBow3Random1]->load(path + "wood-bow3-random1.cub", database, true, false);
    m_sprites[SpriteWoodBow4Random1]->load(path + "wood-bow4-random1.cub", database, true, false);
    m_sprites[SpriteWoodBow5Random1]->load(path + "wood-bow5-random1.cub", database, true, false);
    m_sprites[SpriteWoodBow1Random2]->load(path + "wood-bow1-random2.cub", database, true, false);
    m_sprites[SpriteWoodBow2Random2]->load(path + "wood-bow2-random2.cub", database, true, false);
    m_sprites[SpriteWoodBow3Random2]->load(path + "wood-bow3-random2.cub", database, true, false);
    m_sprites[SpriteWoodBow4Random2]->load(path + "wood-bow4-random2.cub", database, true, false);
    m_sprites[SpriteWoodBow5Random2]->load(path + "wood-bow5-random2.cub", database, true, false);
    m_sprites[SpriteWoodBow1Random3]->load(path + "wood-bow1-random3.cub", database, true, false);
    m_sprites[SpriteWoodBow2Random3]->load(path + "wood-bow2-random3.cub", database, true, false);
    m_sprites[SpriteWoodBow3Random3]->load(path + "wood-bow3-random3.cub", database, true, false);
    m_sprites[SpriteWoodBow4Random3]->load(path + "wood-bow4-random3.cub", database, true, false);
    m_sprites[SpriteWoodBow5Random3]->load(path + "wood-bow5-random3.cub", database, true, false);
    m_sprites[SpriteWoodBow1Random4]->load(path + "wood-bow1-random4.cub", database, true, false);
    m_sprites[SpriteWoodBow2Random4]->load(path + "wood-bow2-random4.cub", database, true, false);
    m_sprites[SpriteWoodBow3Random4]->load(path + "wood-bow3-random4.cub", database, true, false);
    m_sprites[SpriteWoodBow4Random4]->load(path + "wood-bow4-random4.cub", database, true, false);
    m_sprites[SpriteWoodBow5Random4]->load(path + "wood-bow5-random4.cub", database, true, false);
    m_sprites[SpriteWoodBow1Random5]->load(path + "wood-bow1-random5.cub", database, true, false);
    m_sprites[SpriteWoodBow2Random5]->load(path + "wood-bow2-random5.cub", database, true, false);
    m_sprites[SpriteWoodBow3Random5]->load(path + "wood-bow3-random5.cub", database, true, false);
    m_sprites[SpriteWoodBow4Random5]->load(path + "wood-bow4-random5.cub", database, true, false);
    m_sprites[SpriteWoodBow5Random5]->load(path + "wood-bow5-random5.cub", database, true, false);
    m_sprites[SpriteWoodBow1Random6]->load(path + "wood-bow1-random6.cub", database, true, false);
    m_sprites[SpriteWoodBow2Random6]->load(path + "wood-bow2-random6.cub", database, true, false);
    m_sprites[SpriteWoodBow3Random6]->load(path + "wood-bow3-random6.cub", database, true, false);
    m_sprites[SpriteWoodBow4Random6]->load(path + "wood-bow4-random6.cub", database, true, false);
    m_sprites[SpriteWoodBow5Random6]->load(path + "wood-bow5-random6.cub", database, true, false);
    m_sprites[SpriteWoodBow1Random7]->load(path + "wood-bow1-random7.cub", database, true, false);
    m_sprites[SpriteWoodBow2Random7]->load(path + "wood-bow2-random7.cub", database, true, false);
    m_sprites[SpriteWoodBow3Random7]->load(path + "wood-bow3-random7.cub", database, true, false);
    m_sprites[SpriteWoodBow4Random7]->load(path + "wood-bow4-random7.cub", database, true, false);
    m_sprites[SpriteWoodBow5Random7]->load(path + "wood-bow5-random7.cub", database, true, false);
    m_sprites[SpriteWoodBow1Random8]->load(path + "wood-bow1-random8.cub", database, true, false);
    m_sprites[SpriteWoodBow2Random8]->load(path + "wood-bow2-random8.cub", database, true, false);
    m_sprites[SpriteWoodBow3Random8]->load(path + "wood-bow3-random8.cub", database, true, false);
    m_sprites[SpriteWoodBow4Random8]->load(path + "wood-bow4-random8.cub", database, true, false);
    m_sprites[SpriteWoodBow5Random8]->load(path + "wood-bow5-random8.cub", database, true, false);
    m_sprites[SpriteWoodBow1Random9]->load(path + "wood-bow1-random9.cub", database, true, false);
    m_sprites[SpriteWoodBow2Random9]->load(path + "wood-bow2-random9.cub", database, true, false);
    m_sprites[SpriteWoodBow3Random9]->load(path + "wood-bow3-random9.cub", database, true, false);
    m_sprites[SpriteWoodBow4Random9]->load(path + "wood-bow4-random9.cub", database, true, false);
    m_sprites[SpriteWoodBow5Random9]->load(path + "wood-bow5-random9.cub", database, true, false);
    m_sprites[SpriteWoodBow1Random10]->load(path + "wood-bow1-random10.cub", database, true, false);
    m_sprites[SpriteWoodBow2Random10]->load(path + "wood-bow2-random10.cub", database, true, false);
    m_sprites[SpriteWoodBow3Random10]->load(path + "wood-bow3-random10.cub", database, true, false);
    m_sprites[SpriteWoodBow4Random10]->load(path + "wood-bow4-random10.cub", database, true, false);
    m_sprites[SpriteWoodBow5Random10]->load(path + "wood-bow5-random10.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow1]->load(path + "wood-crossbow1.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow2]->load(path + "wood-crossbow2.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow3]->load(path + "wood-crossbow3.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow4]->load(path + "wood-crossbow4.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow5]->load(path + "wood-crossbow5.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow1Random1]->load(path + "wood-crossbow1-random1.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow2Random1]->load(path + "wood-crossbow2-random1.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow3Random1]->load(path + "wood-crossbow3-random1.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow4Random1]->load(path + "wood-crossbow4-random1.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow5Random1]->load(path + "wood-crossbow5-random1.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow1Random2]->load(path + "wood-crossbow1-random2.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow2Random2]->load(path + "wood-crossbow2-random2.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow3Random2]->load(path + "wood-crossbow3-random2.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow4Random2]->load(path + "wood-crossbow4-random2.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow5Random2]->load(path + "wood-crossbow5-random2.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow1Random3]->load(path + "wood-crossbow1-random3.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow2Random3]->load(path + "wood-crossbow2-random3.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow3Random3]->load(path + "wood-crossbow3-random3.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow4Random3]->load(path + "wood-crossbow4-random3.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow5Random3]->load(path + "wood-crossbow5-random3.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow1Random4]->load(path + "wood-crossbow1-random4.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow2Random4]->load(path + "wood-crossbow2-random4.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow3Random4]->load(path + "wood-crossbow3-random4.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow4Random4]->load(path + "wood-crossbow4-random4.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow5Random4]->load(path + "wood-crossbow5-random4.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow1Random5]->load(path + "wood-crossbow1-random5.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow2Random5]->load(path + "wood-crossbow2-random5.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow3Random5]->load(path + "wood-crossbow3-random5.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow4Random5]->load(path + "wood-crossbow4-random5.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow5Random5]->load(path + "wood-crossbow5-random5.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow1Random6]->load(path + "wood-crossbow1-random6.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow2Random6]->load(path + "wood-crossbow2-random6.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow3Random6]->load(path + "wood-crossbow3-random6.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow4Random6]->load(path + "wood-crossbow4-random6.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow5Random6]->load(path + "wood-crossbow5-random6.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow1Random7]->load(path + "wood-crossbow1-random7.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow2Random7]->load(path + "wood-crossbow2-random7.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow3Random7]->load(path + "wood-crossbow3-random7.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow4Random7]->load(path + "wood-crossbow4-random7.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow5Random7]->load(path + "wood-crossbow5-random7.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow1Random8]->load(path + "wood-crossbow1-random8.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow2Random8]->load(path + "wood-crossbow2-random8.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow3Random8]->load(path + "wood-crossbow3-random8.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow4Random8]->load(path + "wood-crossbow4-random8.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow5Random8]->load(path + "wood-crossbow5-random8.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow1Random9]->load(path + "wood-crossbow1-random9.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow2Random9]->load(path + "wood-crossbow2-random9.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow3Random9]->load(path + "wood-crossbow3-random9.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow4Random9]->load(path + "wood-crossbow4-random9.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow5Random9]->load(path + "wood-crossbow5-random9.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow1Random10]->load(path + "wood-crossbow1-random10.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow2Random10]->load(path + "wood-crossbow2-random10.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow3Random10]->load(path + "wood-crossbow3-random10.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow4Random10]->load(path + "wood-crossbow4-random10.cub", database, true, false);
    m_sprites[SpriteWoodCrossbow5Random10]->load(path + "wood-crossbow5-random10.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang1]->load(path + "wood-boomerang1.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang2]->load(path + "wood-boomerang2.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang3]->load(path + "wood-boomerang3.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang4]->load(path + "wood-boomerang4.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang5]->load(path + "wood-boomerang5.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang1Random1]->load(path + "wood-boomerang1-random1.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang2Random1]->load(path + "wood-boomerang2-random1.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang3Random1]->load(path + "wood-boomerang3-random1.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang4Random1]->load(path + "wood-boomerang4-random1.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang5Random1]->load(path + "wood-boomerang5-random1.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang1Random2]->load(path + "wood-boomerang1-random2.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang2Random2]->load(path + "wood-boomerang2-random2.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang3Random2]->load(path + "wood-boomerang3-random2.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang4Random2]->load(path + "wood-boomerang4-random2.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang5Random2]->load(path + "wood-boomerang5-random2.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang1Random3]->load(path + "wood-boomerang1-random3.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang2Random3]->load(path + "wood-boomerang2-random3.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang3Random3]->load(path + "wood-boomerang3-random3.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang4Random3]->load(path + "wood-boomerang4-random3.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang5Random3]->load(path + "wood-boomerang5-random3.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang1Random4]->load(path + "wood-boomerang1-random4.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang2Random4]->load(path + "wood-boomerang2-random4.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang3Random4]->load(path + "wood-boomerang3-random4.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang4Random4]->load(path + "wood-boomerang4-random4.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang5Random4]->load(path + "wood-boomerang5-random4.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang1Random5]->load(path + "wood-boomerang1-random5.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang2Random5]->load(path + "wood-boomerang2-random5.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang3Random5]->load(path + "wood-boomerang3-random5.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang4Random5]->load(path + "wood-boomerang4-random5.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang5Random5]->load(path + "wood-boomerang5-random5.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang1Random6]->load(path + "wood-boomerang1-random6.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang2Random6]->load(path + "wood-boomerang2-random6.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang3Random6]->load(path + "wood-boomerang3-random6.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang4Random6]->load(path + "wood-boomerang4-random6.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang5Random6]->load(path + "wood-boomerang5-random6.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang1Random7]->load(path + "wood-boomerang1-random7.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang2Random7]->load(path + "wood-boomerang2-random7.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang3Random7]->load(path + "wood-boomerang3-random7.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang4Random7]->load(path + "wood-boomerang4-random7.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang5Random7]->load(path + "wood-boomerang5-random7.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang1Random8]->load(path + "wood-boomerang1-random8.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang2Random8]->load(path + "wood-boomerang2-random8.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang3Random8]->load(path + "wood-boomerang3-random8.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang4Random8]->load(path + "wood-boomerang4-random8.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang5Random8]->load(path + "wood-boomerang5-random8.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang1Random9]->load(path + "wood-boomerang1-random9.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang2Random9]->load(path + "wood-boomerang2-random9.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang3Random9]->load(path + "wood-boomerang3-random9.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang4Random9]->load(path + "wood-boomerang4-random9.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang5Random9]->load(path + "wood-boomerang5-random9.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang1Random10]->load(path + "wood-boomerang1-random10.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang2Random10]->load(path + "wood-boomerang2-random10.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang3Random10]->load(path + "wood-boomerang3-random10.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang4Random10]->load(path + "wood-boomerang4-random10.cub", database, true, false);
    m_sprites[SpriteWoodBoomerang5Random10]->load(path + "wood-boomerang5-random10.cub", database, true, false);
    m_sprites[SpriteArrow]->load(path + "arrow.cub", database, true, false);
    m_sprites[SpriteBullet]->load(path + "bullet.cub", database, true, false);
    m_sprites[SpriteShuriken]->load(path + "shuriken.cub", database, true, false);
    m_sprites[SpriteFireball]->load(path + "fireball.cub", database, true, false);
    m_sprites[SpriteQuiver]->load(path + "quiver.cub", database, true, false);
    m_sprites[SpriteTorchSlot1029]->load(path + "torch.cub", database, true, false);
    m_sprites[SpriteCube]->load(path + "cube.cub", database, true, false);
    m_sprites[SpriteStars]->load(path + "stars.cub", database, true, false);
    m_sprites[SpritePlaster]->load(path + "plaster.cub", database, true, false);
    m_sprites[SpriteZzz]->load(path + "zzz.cub", database, true, false);
    m_sprites[SpriteHeartSlot3496]->load(path + "heart.cub", database, true, false);
    m_sprites[SpriteDoorSlot2631]->load(path + "door.cub", database, true, false);
    m_sprites[SpriteWindowSlot2632]->load(path + "window.cub", database, true, false);
    m_sprites[SpriteGoddess2Slot2633]->load(path + "goddess2.cub", database, true, false);
    m_sprites[SpriteArtifact]->load(path + "artifact.cub", database, true, false);
    m_sprites[SpriteGoldKey]->load(path + "gold-key.cub", database, true, false);
    m_sprites[SpriteBoneKey]->load(path + "bone-key.cub", database, true, false);
    m_sprites[SpriteBronzeKey]->load(path + "bronze-key.cub", database, true, false);
    m_sprites[SpriteSilverKey]->load(path + "silver-key.cub", database, true, false);
    m_sprites[SpriteSkullKey]->load(path + "skull-key.cub", database, true, false);
    m_sprites[SpriteIronKey]->load(path + "iron-key.cub", database, true, false);
    m_sprites[SpriteRustyKey]->load(path + "rusty-key.cub", database, true, false);
    m_sprites[SpriteScratchedKey]->load(path + "scratched-key.cub", database, true, false);
    m_sprites[SpriteAdornedKey]->load(path + "adorned-key.cub", database, true, false);
    m_sprites[SpriteCopperKey]->load(path + "copper-key.cub", database, true, false);
    m_sprites[SpriteObsidianKey]->load(path + "obsidian-key.cub", database, true, false);
    m_sprites[SpriteSteelKey]->load(path + "steel-key.cub", database, true, false);
    m_sprites[SpriteTwistedKey]->load(path + "twisted-key.cub", database, true, false);
    m_sprites[SpriteSmallKey]->load(path + "small-key.cub", database, true, false);
    m_sprites[SpriteHeavyKey]->load(path + "heavy-key.cub", database, true, false);
    m_sprites[SpriteMassiveKey]->load(path + "massive-key.cub", database, true, false);
    m_sprites[SpriteWoodKey]->load(path + "wood-key.cub", database, true, false);
    m_sprites[SpriteStoneKey]->load(path + "stone-key.cub", database, true, false);
    m_sprites[SpriteTinyKey]->load(path + "tiny-key.cub", database, true, false);
    m_sprites[SpriteDelicateKey]->load(path + "delicate-key.cub", database, true, false);
    m_sprites[SpriteQuestItemAmulet01]->load(path + "quest-item-amulet01.cub", database, true, false);
    m_sprites[SpriteQuestItemAmulet02]->load(path + "quest-item-amulet02.cub", database, true, false);
    m_sprites[SpriteQuestItemJewelCase]->load(path + "quest-item-jewel-case.cub", database, true, false);
    m_sprites[SpriteLetter1]->load(path + "letter1.cub", database, true, false);
    m_sprites[SpriteLetter2]->load(path + "letter2.cub", database, true, false);
    m_sprites[SpriteLetter3]->load(path + "letter3.cub", database, true, false);
    m_sprites[SpriteNote1]->load(path + "note1.cub", database, true, false);
    m_sprites[SpriteNote2]->load(path + "note2.cub", database, true, false);
    m_sprites[SpriteNote3]->load(path + "note3.cub", database, true, false);
    m_sprites[SpriteTablet1]->load(path + "tablet1.cub", database, true, false);
    m_sprites[SpriteTablet2]->load(path + "tablet2.cub", database, true, false);
    m_sprites[SpriteTablet3]->load(path + "tablet3.cub", database, true, false);
    m_sprites[SpriteScroll1Slot2674]->load(path + "scroll1.cub", database, true, false);
    m_sprites[SpriteScroll2Slot2675]->load(path + "scroll2.cub", database, true, false);
    m_sprites[SpriteScroll3Slot2676]->load(path + "scroll3.cub", database, true, false);
    m_sprites[SpriteScroll1Slot2677]->load(path + "scroll1.cub", database, true, false);
    m_sprites[SpriteScroll2Slot2678]->load(path + "scroll2.cub", database, true, false);
    m_sprites[SpriteScroll3Slot2679]->load(path + "scroll3.cub", database, true, false);
    m_sprites[SpriteTreasure1]->load(path + "treasure1.cub", database, true, false);
    m_sprites[SpriteTreasure2]->load(path + "treasure2.cub", database, true, false);
    m_sprites[SpriteTreasure3]->load(path + "treasure3.cub", database, true, false);
    m_sprites[SpriteTreasure4]->load(path + "treasure4.cub", database, true, false);
    m_sprites[SpriteTreasure5]->load(path + "treasure5.cub", database, true, false);
    m_sprites[SpriteTreasureStatue1]->load(path + "treasure-statue1.cub", database, true, false);
    m_sprites[SpriteTreasureStatue2]->load(path + "treasure-statue2.cub", database, true, false);
    m_sprites[SpriteTreasureStatue3]->load(path + "treasure-statue3.cub", database, true, false);
    m_sprites[SpriteTreasureStatue4]->load(path + "treasure-statue4.cub", database, true, false);
    m_sprites[SpriteTreasureStatue5]->load(path + "treasure-statue5.cub", database, true, false);
    m_sprites[SpriteTreasureJewel1]->load(path + "treasure-jewel1.cub", database, true, false);
    m_sprites[SpriteTreasureVase1]->load(path + "treasure-vase1.cub", database, true, false);
    m_sprites[SpriteTreasureVase2]->load(path + "treasure-vase2.cub", database, true, false);
    m_sprites[SpriteDragonDrool]->load(path + "dragon-drool.cub", database, true, false);
    m_sprites[SpriteFrogSlime]->load(path + "frog-slime.cub", database, true, false);
    m_sprites[SpriteGargoyleTooth]->load(path + "gargoyle-tooth.cub", database, true, false);
    m_sprites[SpriteSlimyEye]->load(path + "slimy-eye.cub", database, true, false);
    m_sprites[SpriteBroom]->load(path + "broom.cub", database, true, false);
    m_sprites[SpriteAnchor]->load(path + "anchor.cub", database, true, false);
    m_sprites[SpriteWalkingStick]->load(path + "walking-stick.cub", database, true, false);
    m_sprites[SpriteTrollFingernail]->load(path + "troll-fingernail.cub", database, true, false);
    m_sprites[SpriteSpoon]->load(path + "spoon.cub", database, true, false);
    m_sprites[SpriteSocks1]->load(path + "socks1.cub", database, true, false);
    m_sprites[SpriteSocks2]->load(path + "socks2.cub", database, true, false);
    m_sprites[SpriteSocks3]->load(path + "socks3.cub", database, true, false);
    m_sprites[SpriteSocks4]->load(path + "socks4.cub", database, true, false);
    m_sprites[SpritePicnicBasket]->load(path + "picnic-basket.cub", database, true, false);
    m_sprites[SpritePan]->load(path + "pan.cub", database, true, false);
    m_sprites[SpriteLunchBox]->load(path + "lunch-box.cub", database, true, false);
    m_sprites[SpriteLuckyCharm]->load(path + "lucky-charm.cub", database, true, false);
    m_sprites[SpriteBelt]->load(path + "belt.cub", database, true, false);
    m_sprites[SpriteRareCrystal1]->load(path + "rare-crystal1.cub", database, true, false);
    m_sprites[SpriteRareCrystal2]->load(path + "rare-crystal2.cub", database, true, false);
    m_sprites[SpriteRareCrystal3]->load(path + "rare-crystal3.cub", database, true, false);
    m_sprites[SpriteRareCrystal4]->load(path + "rare-crystal4.cub", database, true, false);
    m_sprites[SpriteFabricScrap1]->load(path + "fabric-scrap1.cub", database, true, false);
    m_sprites[SpriteFabricScrap2]->load(path + "fabric-scrap2.cub", database, true, false);
    m_sprites[SpriteFabricScrap3]->load(path + "fabric-scrap3.cub", database, true, false);
    m_sprites[SpriteFabricScrap4]->load(path + "fabric-scrap4.cub", database, true, false);
    m_sprites[SpriteButton1]->load(path + "button1.cub", database, true, false);
    m_sprites[SpriteButton2]->load(path + "button2.cub", database, true, false);
    m_sprites[SpriteButton3]->load(path + "button3.cub", database, true, false);
    m_sprites[SpriteButton4]->load(path + "button4.cub", database, true, false);
    m_sprites[SpriteFruit1]->load(path + "fruit1.cub", database, true, false);
    m_sprites[SpriteFruit2]->load(path + "fruit2.cub", database, true, false);
    m_sprites[SpriteFruit3]->load(path + "fruit3.cub", database, true, false);
    m_sprites[SpriteFruit4]->load(path + "fruit4.cub", database, true, false);
    m_sprites[SpriteNut1]->load(path + "nut1.cub", database, true, false);
    m_sprites[SpriteNut2]->load(path + "nut2.cub", database, true, false);
    m_sprites[SpriteNut3]->load(path + "nut3.cub", database, true, false);
    m_sprites[SpriteNut4]->load(path + "nut4.cub", database, true, false);
    m_sprites[SpriteRareFiber1]->load(path + "rare-fiber1.cub", database, true, false);
    m_sprites[SpriteRareFiber2]->load(path + "rare-fiber2.cub", database, true, false);
    m_sprites[SpriteRareFiber3]->load(path + "rare-fiber3.cub", database, true, false);
    m_sprites[SpriteRareFiber4]->load(path + "rare-fiber4.cub", database, true, false);
    m_sprites[SpriteRareBlossom1]->load(path + "rare-blossom1.cub", database, true, false);
    m_sprites[SpriteRareBlossom2]->load(path + "rare-blossom2.cub", database, true, false);
    m_sprites[SpriteRareBlossom3]->load(path + "rare-blossom3.cub", database, true, false);
    m_sprites[SpriteRareBlossom4]->load(path + "rare-blossom4.cub", database, true, false);
    m_sprites[SpriteMagicBook1]->load(path + "magic-book1.cub", database, true, false);
    m_sprites[SpriteMagicBook2]->load(path + "magic-book2.cub", database, true, false);
    m_sprites[SpriteMagicBook3]->load(path + "magic-book3.cub", database, true, false);
    m_sprites[SpriteMagicBook4]->load(path + "magic-book4.cub", database, true, false);
    m_sprites[SpriteSpellBook]->load(path + "spell-book.cub", database, true, false);
    m_sprites[SpriteMagicallyLockedBook]->load(path + "magically-locked-book.cub", database, true, false);
    m_sprites[SpriteHerb1]->load(path + "herb1.cub", database, true, false);
    m_sprites[SpriteHerb2]->load(path + "herb2.cub", database, true, false);
    m_sprites[SpriteHerb3]->load(path + "herb3.cub", database, true, false);
    m_sprites[SpriteHerb4]->load(path + "herb4.cub", database, true, false);
    m_sprites[SpriteHerb5]->load(path + "herb5.cub", database, true, false);
    m_sprites[SpriteHerb6]->load(path + "herb6.cub", database, true, false);
    m_sprites[SpriteHerb7]->load(path + "herb7.cub", database, true, false);
    m_sprites[SpriteHerb8]->load(path + "herb8.cub", database, true, false);
    m_sprites[SpriteHerb9]->load(path + "herb9.cub", database, true, false);
    m_sprites[SpriteHerb10]->load(path + "herb10.cub", database, true, false);
    m_sprites[SpriteFlower1]->load(path + "flower1.cub", database, true, false);
    m_sprites[SpriteFlower2]->load(path + "flower2.cub", database, true, false);
    m_sprites[SpriteFlower3]->load(path + "flower3.cub", database, true, false);
    m_sprites[SpriteFlower4]->load(path + "flower4.cub", database, true, false);
    m_sprites[SpriteFlower5]->load(path + "flower5.cub", database, true, false);
    m_sprites[SpriteFlower6]->load(path + "flower6.cub", database, true, false);
    m_sprites[SpriteBrokenWeapon1]->load(path + "broken-weapon1.cub", database, true, false);
    m_sprites[SpriteBrokenWeapon2]->load(path + "broken-weapon2.cub", database, true, false);
    m_sprites[SpriteBrokenWeapon3]->load(path + "broken-weapon3.cub", database, true, false);
    m_sprites[SpriteBrokenWeapon4]->load(path + "broken-weapon4.cub", database, true, false);
    m_sprites[SpriteEnchantedTimber1]->load(path + "enchanted-timber1.cub", database, true, false);
    m_sprites[SpriteEnchantedTimber2]->load(path + "enchanted-timber2.cub", database, true, false);
    m_sprites[SpriteEnchantedTimber3]->load(path + "enchanted-timber3.cub", database, true, false);
    m_sprites[SpriteEnchantedTimber4]->load(path + "enchanted-timber4.cub", database, true, false);
    m_sprites[SpriteEnchantedTimber5]->load(path + "enchanted-timber5.cub", database, true, false);
    m_sprites[SpriteGhostRemains]->load(path + "ghost-remains.cub", database, true, false);
    m_sprites[SpriteSalve]->load(path + "salve.cub", database, true, false);
    m_sprites[SpriteAntivenom]->load(path + "antivenom.cub", database, true, false);
    m_sprites[SpritePainkiller]->load(path + "painkiller.cub", database, true, false);
    m_sprites[SpriteStaminaPotion]->load(path + "stamina-potion.cub", database, true, false);
    m_sprites[SpriteMedicine]->load(path + "medicine.cub", database, true, false);
    m_sprites[SpriteBag1]->load(path + "bag1.cub", database, true, false);
    m_sprites[SpriteBag2]->load(path + "bag2.cub", database, true, false);
    m_sprites[SpritePouch1]->load(path + "pouch1.cub", database, true, false);
    m_sprites[SpriteEternalEmber]->load(path + "eternal-ember.cub", database, true, false);
    m_sprites[SpriteFlute]->load(path + "flute.cub", database, true, false);
    m_sprites[SpriteHarp]->load(path + "harp.cub", database, true, false);
    m_sprites[SpriteFiddle]->load(path + "fiddle.cub", database, true, false);
    m_sprites[SpriteMagicBean]->load(path + "magic-bean.cub", database, true, false);
    m_sprites[SpriteWhistle]->load(path + "whistle.cub", database, true, false);
    m_sprites[SpriteBell]->load(path + "bell.cub", database, true, false);
    m_sprites[SpriteLute]->load(path + "lute.cub", database, true, false);
    m_sprites[SpriteNightingale]->load(path + "nightingale.cub", database, true, false);
    m_sprites[SpriteMeteorite]->load(path + "meteorite.cub", database, true, false);
    m_sprites[SpriteVoidStone]->load(path + "void-stone.cub", database, true, false);
    m_sprites[SpritePetWhistle]->load(path + "pet-whistle.cub", database, true, false);
    m_sprites[SpriteMushroom]->load(path + "mushroom.cub", database, true, false);
    m_sprites[SpriteShimmerMushroom]->load(path + "shimmer-mushroom.cub", database, true, false);
    m_sprites[SpriteHeartflower]->load(path + "heartflower.cub", database, true, false);
    m_sprites[SpriteHeartflowerFrozen]->load(path + "heartflower-frozen.cub", database, true, false);
    m_sprites[SpritePricklyPear]->load(path + "prickly-pear.cub", database, true, false);
    m_sprites[SpriteSoulFlower]->load(path + "SoulFlower.cub", database, true, false);
    m_sprites[SpriteMindflower]->load(path + "mindflower.cub", database, true, false);
    m_sprites[SpriteManaorchid]->load(path + "manaorchid.cub", database, true, false);
    m_sprites[SpriteSpiritflower]->load(path + "spiritflower.cub", database, true, false);
    m_sprites[SpriteLifePotion1]->load(path + "life-potion1.cub", database, true, false);
    m_sprites[SpriteLifePotion2]->load(path + "life-potion2.cub", database, true, false);
    m_sprites[SpriteLifePotion3]->load(path + "life-potion3.cub", database, true, false);
    m_sprites[SpriteLifePotion4]->load(path + "life-potion4.cub", database, true, false);
    m_sprites[SpriteLifePotion5]->load(path + "life-potion5.cub", database, true, false);
    m_sprites[SpriteCactusPotion]->load(path + "cactus-potion.cub", database, true, false);
    m_sprites[SpriteManaPotion]->load(path + "mana-potion.cub", database, true, false);
    m_sprites[SpriteCookie]->load(path + "cookie.cub", database, true, false);
    m_sprites[SpriteJellyGreen]->load(path + "jelly-green.cub", database, true, false);
    m_sprites[SpriteJellyBlue]->load(path + "jelly-blue.cub", database, true, false);
    m_sprites[SpriteJellyPink]->load(path + "jelly-pink.cub", database, true, false);
    m_sprites[SpriteJellyYellow]->load(path + "jelly-yellow.cub", database, true, false);
    m_sprites[SpriteCarrot]->load(path + "carrot.cub", database, true, false);
    m_sprites[SpritePumpkinMash]->load(path + "pumpkin-mash.cub", database, true, false);
    m_sprites[SpriteBananaMash]->load(path + "banana-mash.cub", database, true, false);
    m_sprites[SpriteNutCookie]->load(path + "nut-cookie.cub", database, true, false);
    m_sprites[SpriteWhiteNutCookie]->load(path + "white-nut-cookie.cub", database, true, false);
    m_sprites[SpriteCandy]->load(path + "candy.cub", database, true, false);
    m_sprites[SpriteLollipop]->load(path + "lollipop.cub", database, true, false);
    m_sprites[SpriteSoftice]->load(path + "softice.cub", database, true, false);
    m_sprites[SpriteDonutChocolate]->load(path + "donut-chocolate.cub", database, true, false);
    m_sprites[SpriteCottonCandy]->load(path + "cotton-candy.cub", database, true, false);
    m_sprites[SpritePopcorn]->load(path + "popcorn.cub", database, true, false);
    m_sprites[SpriteCerealBar]->load(path + "cereal-bar.cub", database, true, false);
    m_sprites[SpriteStrawberryCake]->load(path + "strawberry-cake.cub", database, true, false);
    m_sprites[SpriteChocolateCake]->load(path + "chocolate-cake.cub", database, true, false);
    m_sprites[SpriteVanillaCupcake]->load(path + "vanilla-cupcake.cub", database, true, false);
    m_sprites[SpriteChocolateCupcake]->load(path + "chocolate-cupcake.cub", database, true, false);
    m_sprites[SpriteBananaSplit]->load(path + "banana-split.cub", database, true, false);
    m_sprites[SpriteMixedSalad]->load(path + "mixed-salad.cub", database, true, false);
    m_sprites[SpriteRadicchioSalad]->load(path + "radicchio-salad.cub", database, true, false);
    m_sprites[SpriteCabbageRolls]->load(path + "cabbage-rolls.cub", database, true, false);
    m_sprites[SpriteCroissant]->load(path + "croissant.cub", database, true, false);
    m_sprites[SpriteLolly]->load(path + "lolly.cub", database, true, false);
    m_sprites[SpriteLemonTart]->load(path + "lemon-tart.cub", database, true, false);
    m_sprites[SpriteChocolateCookie]->load(path + "chocolate-cookie.cub", database, true, false);
    m_sprites[SpriteBubbleGum]->load(path + "bubble-gum.cub", database, true, false);
    m_sprites[SpriteLicoriceCandy]->load(path + "licorice-candy.cub", database, true, false);
    m_sprites[SpriteCinnamonRole]->load(path + "cinnamon-role.cub", database, true, false);
    m_sprites[SpriteAppleRing]->load(path + "apple-ring.cub", database, true, false);
    m_sprites[SpriteWaffle]->load(path + "waffle.cub", database, true, false);
    m_sprites[SpriteWaterIce]->load(path + "water-ice.cub", database, true, false);
    m_sprites[SpriteDateCookie]->load(path + "date-cookie.cub", database, true, false);
    m_sprites[SpriteCandiedApple]->load(path + "candied-apple.cub", database, true, false);
    m_sprites[SpriteStrawberryCocktail]->load(path + "strawberry-cocktail.cub", database, true, false);
    m_sprites[SpriteMilkChocolateBar]->load(path + "milk-chocolate-bar.cub", database, true, false);
    m_sprites[SpriteCaramelChocolateBar]->load(path + "caramel-chocolate-bar.cub", database, true, false);
    m_sprites[SpriteMintChocolateBar]->load(path + "mint-chocolate-bar.cub", database, true, false);
    m_sprites[SpriteWhiteChocolateBar]->load(path + "white-chocolate-bar.cub", database, true, false);
    m_sprites[SpriteSugarCandy]->load(path + "sugar-candy.cub", database, true, false);
    m_sprites[SpriteBlackberryMarmelade]->load(path + "blackberry-marmelade.cub", database, true, false);
    m_sprites[SpriteSaltedCaramel]->load(path + "salted-caramel.cub", database, true, false);
    m_sprites[SpriteGingerTartlet]->load(path + "ginger-tartlet.cub", database, true, false);
    m_sprites[SpriteMangoJuice]->load(path + "mango-juice.cub", database, true, false);
    m_sprites[SpriteRaspberryJuice]->load(path + "raspberry-juice.cub", database, true, false);
    m_sprites[SpriteFruitBasket]->load(path + "fruit-basket.cub", database, true, false);
    m_sprites[SpriteMelonIcecream]->load(path + "melon-icecream.cub", database, true, false);
    m_sprites[SpriteBloodorangeJuice]->load(path + "bloodorange-juice.cub", database, true, false);
    m_sprites[SpritePancakes]->load(path + "pancakes.cub", database, true, false);
    m_sprites[SpriteCurry]->load(path + "curry.cub", database, true, false);
    m_sprites[SpriteBiscuitRole]->load(path + "biscuit-role.cub", database, true, false);
    m_sprites[SpriteBuckhorn]->load(path + "buckhorn.cub", database, true, false);
    m_sprites[SpriteEucalyptusCandy]->load(path + "eucalyptus-candy.cub", database, true, false);
    m_sprites[SpriteMineralWater]->load(path + "mineral-water.cub", database, true, false);
    m_sprites[SpriteSpringWater]->load(path + "spring-water.cub", database, true, false);
    m_sprites[SpriteChocolateIceCream]->load(path + "chocolate-ice-cream.cub", database, true, false);
    m_sprites[SpritePeanut]->load(path + "peanut.cub", database, true, false);
    m_sprites[SpriteIronSword1]->load(path + "iron-sword1.cub", database, true, false);
    m_sprites[SpriteIronSword2]->load(path + "iron-sword2.cub", database, true, false);
    m_sprites[SpriteIronSword3]->load(path + "iron-sword3.cub", database, true, false);
    m_sprites[SpriteIronSword4]->load(path + "iron-sword4.cub", database, true, false);
    m_sprites[SpriteIronSword5]->load(path + "iron-sword5.cub", database, true, false);
    m_sprites[SpriteIronSword1Random1]->load(path + "iron-sword1-random1.cub", database, true, false);
    m_sprites[SpriteIronSword2Random1]->load(path + "iron-sword2-random1.cub", database, true, false);
    m_sprites[SpriteIronSword3Random1]->load(path + "iron-sword3-random1.cub", database, true, false);
    m_sprites[SpriteIronSword4Random1]->load(path + "iron-sword4-random1.cub", database, true, false);
    m_sprites[SpriteIronSword5Random1]->load(path + "iron-sword5-random1.cub", database, true, false);
    m_sprites[SpriteIronSword1Random2]->load(path + "iron-sword1-random2.cub", database, true, false);
    m_sprites[SpriteIronSword2Random2]->load(path + "iron-sword2-random2.cub", database, true, false);
    m_sprites[SpriteIronSword3Random2]->load(path + "iron-sword3-random2.cub", database, true, false);
    m_sprites[SpriteIronSword4Random2]->load(path + "iron-sword4-random2.cub", database, true, false);
    m_sprites[SpriteIronSword5Random2]->load(path + "iron-sword5-random2.cub", database, true, false);
    m_sprites[SpriteIronSword1Random3]->load(path + "iron-sword1-random3.cub", database, true, false);
    m_sprites[SpriteIronSword2Random3]->load(path + "iron-sword2-random3.cub", database, true, false);
    m_sprites[SpriteIronSword3Random3]->load(path + "iron-sword3-random3.cub", database, true, false);
    m_sprites[SpriteIronSword4Random3]->load(path + "iron-sword4-random3.cub", database, true, false);
    m_sprites[SpriteIronSword5Random3]->load(path + "iron-sword5-random3.cub", database, true, false);
    m_sprites[SpriteIronSword1Random4]->load(path + "iron-sword1-random4.cub", database, true, false);
    m_sprites[SpriteIronSword2Random4]->load(path + "iron-sword2-random4.cub", database, true, false);
    m_sprites[SpriteIronSword3Random4]->load(path + "iron-sword3-random4.cub", database, true, false);
    m_sprites[SpriteIronSword4Random4]->load(path + "iron-sword4-random4.cub", database, true, false);
    m_sprites[SpriteIronSword5Random4]->load(path + "iron-sword5-random4.cub", database, true, false);
    m_sprites[SpriteIronSword1Random5]->load(path + "iron-sword1-random5.cub", database, true, false);
    m_sprites[SpriteIronSword2Random5]->load(path + "iron-sword2-random5.cub", database, true, false);
    m_sprites[SpriteIronSword3Random5]->load(path + "iron-sword3-random5.cub", database, true, false);
    m_sprites[SpriteIronSword4Random5]->load(path + "iron-sword4-random5.cub", database, true, false);
    m_sprites[SpriteIronSword5Random5]->load(path + "iron-sword5-random5.cub", database, true, false);
    m_sprites[SpriteIronSword1Random6]->load(path + "iron-sword1-random6.cub", database, true, false);
    m_sprites[SpriteIronSword2Random6]->load(path + "iron-sword2-random6.cub", database, true, false);
    m_sprites[SpriteIronSword3Random6]->load(path + "iron-sword3-random6.cub", database, true, false);
    m_sprites[SpriteIronSword4Random6]->load(path + "iron-sword4-random6.cub", database, true, false);
    m_sprites[SpriteIronSword5Random6]->load(path + "iron-sword5-random6.cub", database, true, false);
    m_sprites[SpriteIronSword1Random7]->load(path + "iron-sword1-random7.cub", database, true, false);
    m_sprites[SpriteIronSword2Random7]->load(path + "iron-sword2-random7.cub", database, true, false);
    m_sprites[SpriteIronSword3Random7]->load(path + "iron-sword3-random7.cub", database, true, false);
    m_sprites[SpriteIronSword4Random7]->load(path + "iron-sword4-random7.cub", database, true, false);
    m_sprites[SpriteIronSword5Random7]->load(path + "iron-sword5-random7.cub", database, true, false);
    m_sprites[SpriteIronSword1Random8]->load(path + "iron-sword1-random8.cub", database, true, false);
    m_sprites[SpriteIronSword2Random8]->load(path + "iron-sword2-random8.cub", database, true, false);
    m_sprites[SpriteIronSword3Random8]->load(path + "iron-sword3-random8.cub", database, true, false);
    m_sprites[SpriteIronSword4Random8]->load(path + "iron-sword4-random8.cub", database, true, false);
    m_sprites[SpriteIronSword5Random8]->load(path + "iron-sword5-random8.cub", database, true, false);
    m_sprites[SpriteIronSword1Random9]->load(path + "iron-sword1-random9.cub", database, true, false);
    m_sprites[SpriteIronSword2Random9]->load(path + "iron-sword2-random9.cub", database, true, false);
    m_sprites[SpriteIronSword3Random9]->load(path + "iron-sword3-random9.cub", database, true, false);
    m_sprites[SpriteIronSword4Random9]->load(path + "iron-sword4-random9.cub", database, true, false);
    m_sprites[SpriteIronSword5Random9]->load(path + "iron-sword5-random9.cub", database, true, false);
    m_sprites[SpriteIronSword1Random10]->load(path + "iron-sword1-random10.cub", database, true, false);
    m_sprites[SpriteIronSword2Random10]->load(path + "iron-sword2-random10.cub", database, true, false);
    m_sprites[SpriteIronSword3Random10]->load(path + "iron-sword3-random10.cub", database, true, false);
    m_sprites[SpriteIronSword4Random10]->load(path + "iron-sword4-random10.cub", database, true, false);
    m_sprites[SpriteIronSword5Random10]->load(path + "iron-sword5-random10.cub", database, true, false);
    m_sprites[SpriteIronDagger1]->load(path + "iron-dagger1.cub", database, true, false);
    m_sprites[SpriteIronDagger2]->load(path + "iron-dagger2.cub", database, true, false);
    m_sprites[SpriteIronDagger3]->load(path + "iron-dagger3.cub", database, true, false);
    m_sprites[SpriteIronDagger4]->load(path + "iron-dagger4.cub", database, true, false);
    m_sprites[SpriteIronDagger5]->load(path + "iron-dagger5.cub", database, true, false);
    m_sprites[SpriteIronDagger1Random1]->load(path + "iron-dagger1-random1.cub", database, true, false);
    m_sprites[SpriteIronDagger2Random1]->load(path + "iron-dagger2-random1.cub", database, true, false);
    m_sprites[SpriteIronDagger3Random1]->load(path + "iron-dagger3-random1.cub", database, true, false);
    m_sprites[SpriteIronDagger4Random1]->load(path + "iron-dagger4-random1.cub", database, true, false);
    m_sprites[SpriteIronDagger5Random1]->load(path + "iron-dagger5-random1.cub", database, true, false);
    m_sprites[SpriteIronDagger1Random2]->load(path + "iron-dagger1-random2.cub", database, true, false);
    m_sprites[SpriteIronDagger2Random2]->load(path + "iron-dagger2-random2.cub", database, true, false);
    m_sprites[SpriteIronDagger3Random2]->load(path + "iron-dagger3-random2.cub", database, true, false);
    m_sprites[SpriteIronDagger4Random2]->load(path + "iron-dagger4-random2.cub", database, true, false);
    m_sprites[SpriteIronDagger5Random2]->load(path + "iron-dagger5-random2.cub", database, true, false);
    m_sprites[SpriteIronDagger1Random3]->load(path + "iron-dagger1-random3.cub", database, true, false);
    m_sprites[SpriteIronDagger2Random3]->load(path + "iron-dagger2-random3.cub", database, true, false);
    m_sprites[SpriteIronDagger3Random3]->load(path + "iron-dagger3-random3.cub", database, true, false);
    m_sprites[SpriteIronDagger4Random3]->load(path + "iron-dagger4-random3.cub", database, true, false);
    m_sprites[SpriteIronDagger5Random3]->load(path + "iron-dagger5-random3.cub", database, true, false);
    m_sprites[SpriteIronDagger1Random4]->load(path + "iron-dagger1-random4.cub", database, true, false);
    m_sprites[SpriteIronDagger2Random4]->load(path + "iron-dagger2-random4.cub", database, true, false);
    m_sprites[SpriteIronDagger3Random4]->load(path + "iron-dagger3-random4.cub", database, true, false);
    m_sprites[SpriteIronDagger4Random4]->load(path + "iron-dagger4-random4.cub", database, true, false);
    m_sprites[SpriteIronDagger5Random4]->load(path + "iron-dagger5-random4.cub", database, true, false);
    m_sprites[SpriteIronDagger1Random5]->load(path + "iron-dagger1-random5.cub", database, true, false);
    m_sprites[SpriteIronDagger2Random5]->load(path + "iron-dagger2-random5.cub", database, true, false);
    m_sprites[SpriteIronDagger3Random5]->load(path + "iron-dagger3-random5.cub", database, true, false);
    m_sprites[SpriteIronDagger4Random5]->load(path + "iron-dagger4-random5.cub", database, true, false);
    m_sprites[SpriteIronDagger5Random5]->load(path + "iron-dagger5-random5.cub", database, true, false);
    m_sprites[SpriteIronDagger1Random6]->load(path + "iron-dagger1-random6.cub", database, true, false);
    m_sprites[SpriteIronDagger2Random6]->load(path + "iron-dagger2-random6.cub", database, true, false);
    m_sprites[SpriteIronDagger3Random6]->load(path + "iron-dagger3-random6.cub", database, true, false);
    m_sprites[SpriteIronDagger4Random6]->load(path + "iron-dagger4-random6.cub", database, true, false);
    m_sprites[SpriteIronDagger5Random6]->load(path + "iron-dagger5-random6.cub", database, true, false);
    m_sprites[SpriteIronDagger1Random7]->load(path + "iron-dagger1-random7.cub", database, true, false);
    m_sprites[SpriteIronDagger2Random7]->load(path + "iron-dagger2-random7.cub", database, true, false);
    m_sprites[SpriteIronDagger3Random7]->load(path + "iron-dagger3-random7.cub", database, true, false);
    m_sprites[SpriteIronDagger4Random7]->load(path + "iron-dagger4-random7.cub", database, true, false);
    m_sprites[SpriteIronDagger5Random7]->load(path + "iron-dagger5-random7.cub", database, true, false);
    m_sprites[SpriteIronDagger1Random8]->load(path + "iron-dagger1-random8.cub", database, true, false);
    m_sprites[SpriteIronDagger2Random8]->load(path + "iron-dagger2-random8.cub", database, true, false);
    m_sprites[SpriteIronDagger3Random8]->load(path + "iron-dagger3-random8.cub", database, true, false);
    m_sprites[SpriteIronDagger4Random8]->load(path + "iron-dagger4-random8.cub", database, true, false);
    m_sprites[SpriteIronDagger5Random8]->load(path + "iron-dagger5-random8.cub", database, true, false);
    m_sprites[SpriteIronDagger1Random9]->load(path + "iron-dagger1-random9.cub", database, true, false);
    m_sprites[SpriteIronDagger2Random9]->load(path + "iron-dagger2-random9.cub", database, true, false);
    m_sprites[SpriteIronDagger3Random9]->load(path + "iron-dagger3-random9.cub", database, true, false);
    m_sprites[SpriteIronDagger4Random9]->load(path + "iron-dagger4-random9.cub", database, true, false);
    m_sprites[SpriteIronDagger5Random9]->load(path + "iron-dagger5-random9.cub", database, true, false);
    m_sprites[SpriteIronDagger1Random10]->load(path + "iron-dagger1-random10.cub", database, true, false);
    m_sprites[SpriteIronDagger2Random10]->load(path + "iron-dagger2-random10.cub", database, true, false);
    m_sprites[SpriteIronDagger3Random10]->load(path + "iron-dagger3-random10.cub", database, true, false);
    m_sprites[SpriteIronDagger4Random10]->load(path + "iron-dagger4-random10.cub", database, true, false);
    m_sprites[SpriteIronDagger5Random10]->load(path + "iron-dagger5-random10.cub", database, true, false);
    m_sprites[SpriteIronFist1]->load(path + "iron-fist1.cub", database, true, false);
    m_sprites[SpriteIronFist2]->load(path + "iron-fist2.cub", database, true, false);
    m_sprites[SpriteIronFist3]->load(path + "iron-fist3.cub", database, true, false);
    m_sprites[SpriteIronFist4]->load(path + "iron-fist4.cub", database, true, false);
    m_sprites[SpriteIronFist5]->load(path + "iron-fist5.cub", database, true, false);
    m_sprites[SpriteIronFist1Random1]->load(path + "iron-fist1-random1.cub", database, true, false);
    m_sprites[SpriteIronFist2Random1]->load(path + "iron-fist2-random1.cub", database, true, false);
    m_sprites[SpriteIronFist3Random1]->load(path + "iron-fist3-random1.cub", database, true, false);
    m_sprites[SpriteIronFist4Random1]->load(path + "iron-fist4-random1.cub", database, true, false);
    m_sprites[SpriteIronFist5Random1]->load(path + "iron-fist5-random1.cub", database, true, false);
    m_sprites[SpriteIronFist1Random2]->load(path + "iron-fist1-random2.cub", database, true, false);
    m_sprites[SpriteIronFist2Random2]->load(path + "iron-fist2-random2.cub", database, true, false);
    m_sprites[SpriteIronFist3Random2]->load(path + "iron-fist3-random2.cub", database, true, false);
    m_sprites[SpriteIronFist4Random2]->load(path + "iron-fist4-random2.cub", database, true, false);
    m_sprites[SpriteIronFist5Random2]->load(path + "iron-fist5-random2.cub", database, true, false);
    m_sprites[SpriteIronFist1Random3]->load(path + "iron-fist1-random3.cub", database, true, false);
    m_sprites[SpriteIronFist2Random3]->load(path + "iron-fist2-random3.cub", database, true, false);
    m_sprites[SpriteIronFist3Random3]->load(path + "iron-fist3-random3.cub", database, true, false);
    m_sprites[SpriteIronFist4Random3]->load(path + "iron-fist4-random3.cub", database, true, false);
    m_sprites[SpriteIronFist5Random3]->load(path + "iron-fist5-random3.cub", database, true, false);
    m_sprites[SpriteIronFist1Random4]->load(path + "iron-fist1-random4.cub", database, true, false);
    m_sprites[SpriteIronFist2Random4]->load(path + "iron-fist2-random4.cub", database, true, false);
    m_sprites[SpriteIronFist3Random4]->load(path + "iron-fist3-random4.cub", database, true, false);
    m_sprites[SpriteIronFist4Random4]->load(path + "iron-fist4-random4.cub", database, true, false);
    m_sprites[SpriteIronFist5Random4]->load(path + "iron-fist5-random4.cub", database, true, false);
    m_sprites[SpriteIronFist1Random5]->load(path + "iron-fist1-random5.cub", database, true, false);
    m_sprites[SpriteIronFist2Random5]->load(path + "iron-fist2-random5.cub", database, true, false);
    m_sprites[SpriteIronFist3Random5]->load(path + "iron-fist3-random5.cub", database, true, false);
    m_sprites[SpriteIronFist4Random5]->load(path + "iron-fist4-random5.cub", database, true, false);
    m_sprites[SpriteIronFist5Random5]->load(path + "iron-fist5-random5.cub", database, true, false);
    m_sprites[SpriteIronFist1Random6]->load(path + "iron-fist1-random6.cub", database, true, false);
    m_sprites[SpriteIronFist2Random6]->load(path + "iron-fist2-random6.cub", database, true, false);
    m_sprites[SpriteIronFist3Random6]->load(path + "iron-fist3-random6.cub", database, true, false);
    m_sprites[SpriteIronFist4Random6]->load(path + "iron-fist4-random6.cub", database, true, false);
    m_sprites[SpriteIronFist5Random6]->load(path + "iron-fist5-random6.cub", database, true, false);
    m_sprites[SpriteIronFist1Random7]->load(path + "iron-fist1-random7.cub", database, true, false);
    m_sprites[SpriteIronFist2Random7]->load(path + "iron-fist2-random7.cub", database, true, false);
    m_sprites[SpriteIronFist3Random7]->load(path + "iron-fist3-random7.cub", database, true, false);
    m_sprites[SpriteIronFist4Random7]->load(path + "iron-fist4-random7.cub", database, true, false);
    m_sprites[SpriteIronFist5Random7]->load(path + "iron-fist5-random7.cub", database, true, false);
    m_sprites[SpriteIronFist1Random8]->load(path + "iron-fist1-random8.cub", database, true, false);
    m_sprites[SpriteIronFist2Random8]->load(path + "iron-fist2-random8.cub", database, true, false);
    m_sprites[SpriteIronFist3Random8]->load(path + "iron-fist3-random8.cub", database, true, false);
    m_sprites[SpriteIronFist4Random8]->load(path + "iron-fist4-random8.cub", database, true, false);
    m_sprites[SpriteIronFist5Random8]->load(path + "iron-fist5-random8.cub", database, true, false);
    m_sprites[SpriteIronFist1Random9]->load(path + "iron-fist1-random9.cub", database, true, false);
    m_sprites[SpriteIronFist2Random9]->load(path + "iron-fist2-random9.cub", database, true, false);
    m_sprites[SpriteIronFist3Random9]->load(path + "iron-fist3-random9.cub", database, true, false);
    m_sprites[SpriteIronFist4Random9]->load(path + "iron-fist4-random9.cub", database, true, false);
    m_sprites[SpriteIronFist5Random9]->load(path + "iron-fist5-random9.cub", database, true, false);
    m_sprites[SpriteIronFist1Random10]->load(path + "iron-fist1-random10.cub", database, true, false);
    m_sprites[SpriteIronFist2Random10]->load(path + "iron-fist2-random10.cub", database, true, false);
    m_sprites[SpriteIronFist3Random10]->load(path + "iron-fist3-random10.cub", database, true, false);
    m_sprites[SpriteIronFist4Random10]->load(path + "iron-fist4-random10.cub", database, true, false);
    m_sprites[SpriteIronFist5Random10]->load(path + "iron-fist5-random10.cub", database, true, false);
    m_sprites[SpriteIronHelmet]->load(path + "iron-helmet.cub", database, true, false);
    m_sprites[SpriteInnkeeperChest]->load(path + "innkeeper-chest.cub", database, true, false);
    m_sprites[SpriteBackpack]->load(path + "backpack.cub", database, true, false);
    m_sprites[SpriteBody1]->load(path + "body1.cub", database, true, false);
    m_sprites[SpriteBody2Slot1439]->load(path + "body2.cub", database, true, false);
    m_sprites[SpriteBody3Slot1440]->load(path + "body3.cub", database, true, false);
    m_sprites[SpriteBody4Slot1441]->load(path + "body4.cub", database, true, false);
    m_sprites[SpriteBody5]->load(path + "body5.cub", database, true, false);
    m_sprites[SpriteBackpack]->load(path + "backpack.cub", database, true, false);
    m_sprites[SpriteObsidianChest]->load(path + "obsidian-chest.cub", database, true, false);
    m_sprites[SpriteObsidianShoulders]->load(path + "obsidian-shoulders.cub", database, true, false);
    m_sprites[SpriteObsidianBoot]->load(path + "obsidian-boot.cub", database, true, false);
    m_sprites[SpriteObsidianGlove]->load(path + "obsidian-glove.cub", database, true, false);
    m_sprites[SpriteObsidianHelmet]->load(path + "obsidian-helmet.cub", database, true, false);
    m_sprites[SpriteGoldChest]->load(path + "gold-chest.cub", database, true, false);
    m_sprites[SpriteGoldShoulder]->load(path + "gold-shoulder.cub", database, true, false);
    m_sprites[SpriteGoldBoot]->load(path + "gold-boot.cub", database, true, false);
    m_sprites[SpriteGoldGlove]->load(path + "gold-glove.cub", database, true, false);
    m_sprites[SpriteGoldHelmet]->load(path + "gold-helmet.cub", database, true, false);
    m_sprites[SpriteIceCrystalChest]->load(path + "ice-crystal-chest.cub", database, true, false);
    m_sprites[SpriteIceCrystalShoulder]->load(path + "ice-crystal-shoulder.cub", database, true, false);
    m_sprites[SpriteIceCrystalBoot]->load(path + "ice-crystal-boot.cub", database, true, false);
    m_sprites[SpriteIceCrystalGlove]->load(path + "ice-crystal-glove.cub", database, true, false);
    m_sprites[SpriteIceCrystalHelmet]->load(path + "ice-crystal-helmet.cub", database, true, false);
    m_sprites[SpriteSaurianChest]->load(path + "saurian-chest.cub", database, true, false);
    m_sprites[SpriteSaurianShoulder]->load(path + "saurian-shoulder.cub", database, true, false);
    m_sprites[SpriteSaurianBoot]->load(path + "saurian-boot.cub", database, true, false);
    m_sprites[SpriteSaurianGlove]->load(path + "saurian-glove.cub", database, true, false);
    m_sprites[SpriteSaurianHelmet]->load(path + "saurian-helmet.cub", database, true, false);
    m_sprites[SpriteSaurianChest]->load(path + "saurian-chest.cub", database, true, false);
    m_sprites[SpriteSaurianShoulder]->load(path + "saurian-shoulder.cub", database, true, false);
    m_sprites[SpriteSaurianBoot]->load(path + "saurian-boot.cub", database, true, false);
    m_sprites[SpriteSaurianGlove]->load(path + "saurian-glove.cub", database, true, false);
    m_sprites[SpriteSaurianHelmet]->load(path + "saurian-helmet.cub", database, true, false);
    m_sprites[SpriteParrotChest]->load(path + "parrot-chest.cub", database, true, false);
    m_sprites[SpriteParrotShoulder]->load(path + "parrot-shoulder.cub", database, true, false);
    m_sprites[SpriteParrotBoot]->load(path + "parrot-boot.cub", database, true, false);
    m_sprites[SpriteParrotGlove]->load(path + "parrot-glove.cub", database, true, false);
    m_sprites[SpriteParrotHelmet]->load(path + "parrot-helmet.cub", database, true, false);
    m_sprites[SpriteLightChest]->load(path + "light-chest.cub", database, true, false);
    m_sprites[SpriteLightShoulder]->load(path + "light-shoulder.cub", database, true, false);
    m_sprites[SpriteLightBoot]->load(path + "light-boot.cub", database, true, false);
    m_sprites[SpriteLightGlove]->load(path + "light-glove.cub", database, true, false);
    m_sprites[SpriteLightHelmet]->load(path + "light-helmet.cub", database, true, false);
    m_sprites[SpriteDruidChest]->load(path + "druid-chest.cub", database, true, false);
    m_sprites[SpriteDruidStaff]->load(path + "druid-staff.cub", database, true, false);
    m_sprites[SpriteBronzeChest1]->load(path + "bronze-chest1.cub", database, true, false);
    m_sprites[SpriteBronzeChest2]->load(path + "bronze-chest2.cub", database, true, false);
    m_sprites[SpriteBronzeChest3]->load(path + "bronze-chest3.cub", database, true, false);
    m_sprites[SpriteBronzeChest4]->load(path + "bronze-chest4.cub", database, true, false);
    m_sprites[SpriteBronzeChest5]->load(path + "bronze-chest5.cub", database, true, false);
    m_sprites[SpriteBronzeShoulder1]->load(path + "bronze-shoulder1.cub", database, true, false);
    m_sprites[SpriteBronzeShoulder2]->load(path + "bronze-shoulder2.cub", database, true, false);
    m_sprites[SpriteBronzeShoulder3]->load(path + "bronze-shoulder3.cub", database, true, false);
    m_sprites[SpriteBronzeShoulder4]->load(path + "bronze-shoulder4.cub", database, true, false);
    m_sprites[SpriteBronzeShoulder5]->load(path + "bronze-shoulder5.cub", database, true, false);
    m_sprites[SpriteBronzeFoot1]->load(path + "bronze-foot1.cub", database, true, false);
    m_sprites[SpriteBronzeFoot2]->load(path + "bronze-foot2.cub", database, true, false);
    m_sprites[SpriteBronzeFoot3]->load(path + "bronze-foot3.cub", database, true, false);
    m_sprites[SpriteBronzeFoot4]->load(path + "bronze-foot4.cub", database, true, false);
    m_sprites[SpriteBronzeFoot5]->load(path + "bronze-foot5.cub", database, true, false);
    m_sprites[SpriteBronzeHand1]->load(path + "bronze-hand1.cub", database, true, false);
    m_sprites[SpriteBronzeHand2]->load(path + "bronze-hand2.cub", database, true, false);
    m_sprites[SpriteBronzeHand3]->load(path + "bronze-hand3.cub", database, true, false);
    m_sprites[SpriteBronzeHand4]->load(path + "bronze-hand4.cub", database, true, false);
    m_sprites[SpriteBronzeHand5]->load(path + "bronze-hand5.cub", database, true, false);
    m_sprites[SpriteWoolChest1]->load(path + "wool-chest1.cub", database, true, false);
    m_sprites[SpriteWoolChest2]->load(path + "wool-chest2.cub", database, true, false);
    m_sprites[SpriteWoolChest3]->load(path + "wool-chest3.cub", database, true, false);
    m_sprites[SpriteWoolChest4]->load(path + "wool-chest4.cub", database, true, false);
    m_sprites[SpriteWoolChest5]->load(path + "wool-chest5.cub", database, true, false);
    m_sprites[SpriteWoolShoulder1]->load(path + "wool-shoulder1.cub", database, true, false);
    m_sprites[SpriteWoolShoulder2]->load(path + "wool-shoulder2.cub", database, true, false);
    m_sprites[SpriteWoolShoulder3]->load(path + "wool-shoulder3.cub", database, true, false);
    m_sprites[SpriteWoolShoulder4]->load(path + "wool-shoulder4.cub", database, true, false);
    m_sprites[SpriteWoolShoulder5]->load(path + "wool-shoulder5.cub", database, true, false);
    m_sprites[SpriteWoolFoot1]->load(path + "wool-foot1.cub", database, true, false);
    m_sprites[SpriteWoolFoot2]->load(path + "wool-foot2.cub", database, true, false);
    m_sprites[SpriteWoolFoot3]->load(path + "wool-foot3.cub", database, true, false);
    m_sprites[SpriteWoolFoot4]->load(path + "wool-foot4.cub", database, true, false);
    m_sprites[SpriteWoolFoot5]->load(path + "wool-foot5.cub", database, true, false);
    m_sprites[SpriteWoolHand1]->load(path + "wool-hand1.cub", database, true, false);
    m_sprites[SpriteWoolHand2]->load(path + "wool-hand2.cub", database, true, false);
    m_sprites[SpriteWoolHand3]->load(path + "wool-hand3.cub", database, true, false);
    m_sprites[SpriteWoolHand4]->load(path + "wool-hand4.cub", database, true, false);
    m_sprites[SpriteWoolHand5]->load(path + "wool-hand5.cub", database, true, false);
    m_sprites[SpriteFeltChest1]->load(path + "felt-chest1.cub", database, true, false);
    m_sprites[SpriteFeltChest2]->load(path + "felt-chest2.cub", database, true, false);
    m_sprites[SpriteFeltChest3]->load(path + "felt-chest3.cub", database, true, false);
    m_sprites[SpriteFeltChest4]->load(path + "felt-chest4.cub", database, true, false);
    m_sprites[SpriteFeltChest5]->load(path + "felt-chest5.cub", database, true, false);
    m_sprites[SpriteFeltShoulder1]->load(path + "felt-shoulder1.cub", database, true, false);
    m_sprites[SpriteFeltShoulder2]->load(path + "felt-shoulder2.cub", database, true, false);
    m_sprites[SpriteFeltShoulder3]->load(path + "felt-shoulder3.cub", database, true, false);
    m_sprites[SpriteFeltShoulder4]->load(path + "felt-shoulder4.cub", database, true, false);
    m_sprites[SpriteFeltShoulder5]->load(path + "felt-shoulder5.cub", database, true, false);
    m_sprites[SpriteFeltFoot1]->load(path + "felt-foot1.cub", database, true, false);
    m_sprites[SpriteFeltFoot2]->load(path + "felt-foot2.cub", database, true, false);
    m_sprites[SpriteFeltFoot3]->load(path + "felt-foot3.cub", database, true, false);
    m_sprites[SpriteFeltFoot4]->load(path + "felt-foot4.cub", database, true, false);
    m_sprites[SpriteFeltFoot5]->load(path + "felt-foot5.cub", database, true, false);
    m_sprites[SpriteFeltHand1]->load(path + "felt-hand1.cub", database, true, false);
    m_sprites[SpriteFeltHand2]->load(path + "felt-hand2.cub", database, true, false);
    m_sprites[SpriteFeltHand3]->load(path + "felt-hand3.cub", database, true, false);
    m_sprites[SpriteFeltHand4]->load(path + "felt-hand4.cub", database, true, false);
    m_sprites[SpriteFeltHand5]->load(path + "felt-hand5.cub", database, true, false);
    m_sprites[SpriteFlannellChest1]->load(path + "flannell-chest1.cub", database, true, false);
    m_sprites[SpriteFlannellChest2]->load(path + "flannell-chest2.cub", database, true, false);
    m_sprites[SpriteFlannellChest3]->load(path + "flannell-chest3.cub", database, true, false);
    m_sprites[SpriteFlannellChest4]->load(path + "flannell-chest4.cub", database, true, false);
    m_sprites[SpriteFlannellChest5]->load(path + "flannell-chest5.cub", database, true, false);
    m_sprites[SpriteFlannellShoulder1]->load(path + "flannell-shoulder1.cub", database, true, false);
    m_sprites[SpriteFlannellShoulder2]->load(path + "flannell-shoulder2.cub", database, true, false);
    m_sprites[SpriteFlannellShoulder3]->load(path + "flannell-shoulder3.cub", database, true, false);
    m_sprites[SpriteFlannellShoulder4]->load(path + "flannell-shoulder4.cub", database, true, false);
    m_sprites[SpriteFlannellShoulder5]->load(path + "flannell-shoulder5.cub", database, true, false);
    m_sprites[SpriteFlannellFoot1]->load(path + "flannell-foot1.cub", database, true, false);
    m_sprites[SpriteFlannellFoot2]->load(path + "flannell-foot2.cub", database, true, false);
    m_sprites[SpriteFlannellFoot3]->load(path + "flannell-foot3.cub", database, true, false);
    m_sprites[SpriteFlannellFoot4]->load(path + "flannell-foot4.cub", database, true, false);
    m_sprites[SpriteFlannellFoot5]->load(path + "flannell-foot5.cub", database, true, false);
    m_sprites[SpriteFlannellHand1]->load(path + "flannell-hand1.cub", database, true, false);
    m_sprites[SpriteFlannellHand2]->load(path + "flannell-hand2.cub", database, true, false);
    m_sprites[SpriteFlannellHand3]->load(path + "flannell-hand3.cub", database, true, false);
    m_sprites[SpriteFlannellHand4]->load(path + "flannell-hand4.cub", database, true, false);
    m_sprites[SpriteFlannellHand5]->load(path + "flannell-hand5.cub", database, true, false);
    m_sprites[SpriteSteelChest1]->load(path + "steel-chest1.cub", database, true, false);
    m_sprites[SpriteSteelChest2]->load(path + "steel-chest2.cub", database, true, false);
    m_sprites[SpriteSteelChest3]->load(path + "steel-chest3.cub", database, true, false);
    m_sprites[SpriteSteelChest4]->load(path + "steel-chest4.cub", database, true, false);
    m_sprites[SpriteSteelChest5]->load(path + "steel-chest5.cub", database, true, false);
    m_sprites[SpriteSteelShoulder1]->load(path + "steel-shoulder1.cub", database, true, false);
    m_sprites[SpriteSteelShoulder2]->load(path + "steel-shoulder2.cub", database, true, false);
    m_sprites[SpriteSteelShoulder3]->load(path + "steel-shoulder3.cub", database, true, false);
    m_sprites[SpriteSteelShoulder4]->load(path + "steel-shoulder4.cub", database, true, false);
    m_sprites[SpriteSteelShoulder5]->load(path + "steel-shoulder5.cub", database, true, false);
    m_sprites[SpriteSteelFoot1]->load(path + "steel-foot1.cub", database, true, false);
    m_sprites[SpriteSteelFoot2]->load(path + "steel-foot2.cub", database, true, false);
    m_sprites[SpriteSteelFoot3]->load(path + "steel-foot3.cub", database, true, false);
    m_sprites[SpriteSteelFoot4]->load(path + "steel-foot4.cub", database, true, false);
    m_sprites[SpriteSteelFoot5]->load(path + "steel-foot5.cub", database, true, false);
    m_sprites[SpriteSteelHand1]->load(path + "steel-hand1.cub", database, true, false);
    m_sprites[SpriteSteelHand2]->load(path + "steel-hand2.cub", database, true, false);
    m_sprites[SpriteSteelHand3]->load(path + "steel-hand3.cub", database, true, false);
    m_sprites[SpriteSteelHand4]->load(path + "steel-hand4.cub", database, true, false);
    m_sprites[SpriteSteelHand5]->load(path + "steel-hand5.cub", database, true, false);
    m_sprites[SpriteSteelStaff]->load(path + "steel-staff.cub", database, true, false);
    m_sprites[SpriteMailChest1]->load(path + "mail-chest1.cub", database, true, false);
    m_sprites[SpriteMailChest2]->load(path + "mail-chest2.cub", database, true, false);
    m_sprites[SpriteMailChest3]->load(path + "mail-chest3.cub", database, true, false);
    m_sprites[SpriteMailChest4]->load(path + "mail-chest4.cub", database, true, false);
    m_sprites[SpriteMailChest5]->load(path + "mail-chest5.cub", database, true, false);
    m_sprites[SpriteMailShoulder1]->load(path + "mail-shoulder1.cub", database, true, false);
    m_sprites[SpriteMailShoulder2]->load(path + "mail-shoulder2.cub", database, true, false);
    m_sprites[SpriteMailShoulder3]->load(path + "mail-shoulder3.cub", database, true, false);
    m_sprites[SpriteMailShoulder4]->load(path + "mail-shoulder4.cub", database, true, false);
    m_sprites[SpriteMailShoulder5]->load(path + "mail-shoulder5.cub", database, true, false);
    m_sprites[SpriteMailFoot1]->load(path + "mail-foot1.cub", database, true, false);
    m_sprites[SpriteMailFoot2]->load(path + "mail-foot2.cub", database, true, false);
    m_sprites[SpriteMailFoot3]->load(path + "mail-foot3.cub", database, true, false);
    m_sprites[SpriteMailFoot4]->load(path + "mail-foot4.cub", database, true, false);
    m_sprites[SpriteMailFoot5]->load(path + "mail-foot5.cub", database, true, false);
    m_sprites[SpriteMailHand1]->load(path + "mail-hand1.cub", database, true, false);
    m_sprites[SpriteMailHand2]->load(path + "mail-hand2.cub", database, true, false);
    m_sprites[SpriteMailHand3]->load(path + "mail-hand3.cub", database, true, false);
    m_sprites[SpriteMailHand4]->load(path + "mail-hand4.cub", database, true, false);
    m_sprites[SpriteMailHand5]->load(path + "mail-hand5.cub", database, true, false);
    m_sprites[SpriteRivetChest1]->load(path + "rivet-chest1.cub", database, true, false);
    m_sprites[SpriteRivetChest2]->load(path + "rivet-chest2.cub", database, true, false);
    m_sprites[SpriteRivetChest3]->load(path + "rivet-chest3.cub", database, true, false);
    m_sprites[SpriteRivetChest4]->load(path + "rivet-chest4.cub", database, true, false);
    m_sprites[SpriteRivetChest5]->load(path + "rivet-chest5.cub", database, true, false);
    m_sprites[SpriteRivetShoulder1]->load(path + "rivet-shoulder1.cub", database, true, false);
    m_sprites[SpriteRivetShoulder2]->load(path + "rivet-shoulder2.cub", database, true, false);
    m_sprites[SpriteRivetShoulder3]->load(path + "rivet-shoulder3.cub", database, true, false);
    m_sprites[SpriteRivetShoulder4]->load(path + "rivet-shoulder4.cub", database, true, false);
    m_sprites[SpriteRivetShoulder5]->load(path + "rivet-shoulder5.cub", database, true, false);
    m_sprites[SpriteRivetFoot1]->load(path + "rivet-foot1.cub", database, true, false);
    m_sprites[SpriteRivetFoot2]->load(path + "rivet-foot2.cub", database, true, false);
    m_sprites[SpriteRivetFoot3]->load(path + "rivet-foot3.cub", database, true, false);
    m_sprites[SpriteRivetFoot4]->load(path + "rivet-foot4.cub", database, true, false);
    m_sprites[SpriteRivetFoot5]->load(path + "rivet-foot5.cub", database, true, false);
    m_sprites[SpriteRivetHand1]->load(path + "rivet-hand1.cub", database, true, false);
    m_sprites[SpriteRivetHand2]->load(path + "rivet-hand2.cub", database, true, false);
    m_sprites[SpriteRivetHand3]->load(path + "rivet-hand3.cub", database, true, false);
    m_sprites[SpriteRivetHand4]->load(path + "rivet-hand4.cub", database, true, false);
    m_sprites[SpriteRivetHand5]->load(path + "rivet-hand5.cub", database, true, false);
    m_sprites[SpriteFilamentChest1]->load(path + "filament-chest1.cub", database, true, false);
    m_sprites[SpriteFilamentChest2]->load(path + "filament-chest2.cub", database, true, false);
    m_sprites[SpriteFilamentChest3]->load(path + "filament-chest3.cub", database, true, false);
    m_sprites[SpriteFilamentChest4]->load(path + "filament-chest4.cub", database, true, false);
    m_sprites[SpriteFilamentChest5]->load(path + "filament-chest5.cub", database, true, false);
    m_sprites[SpriteFilamentShoulder1]->load(path + "filament-shoulder1.cub", database, true, false);
    m_sprites[SpriteFilamentShoulder2]->load(path + "filament-shoulder2.cub", database, true, false);
    m_sprites[SpriteFilamentShoulder3]->load(path + "filament-shoulder3.cub", database, true, false);
    m_sprites[SpriteFilamentShoulder4]->load(path + "filament-shoulder4.cub", database, true, false);
    m_sprites[SpriteFilamentShoulder5]->load(path + "filament-shoulder5.cub", database, true, false);
    m_sprites[SpriteFilamentFoot1]->load(path + "filament-foot1.cub", database, true, false);
    m_sprites[SpriteFilamentFoot2]->load(path + "filament-foot2.cub", database, true, false);
    m_sprites[SpriteFilamentFoot3]->load(path + "filament-foot3.cub", database, true, false);
    m_sprites[SpriteFilamentFoot4]->load(path + "filament-foot4.cub", database, true, false);
    m_sprites[SpriteFilamentFoot5]->load(path + "filament-foot5.cub", database, true, false);
    m_sprites[SpriteFilamentHand1]->load(path + "filament-hand1.cub", database, true, false);
    m_sprites[SpriteFilamentHand2]->load(path + "filament-hand2.cub", database, true, false);
    m_sprites[SpriteFilamentHand3]->load(path + "filament-hand3.cub", database, true, false);
    m_sprites[SpriteFilamentHand4]->load(path + "filament-hand4.cub", database, true, false);
    m_sprites[SpriteFilamentHand5]->load(path + "filament-hand5.cub", database, true, false);
    m_sprites[SpriteSpikeChest1]->load(path + "spike-chest1.cub", database, true, false);
    m_sprites[SpriteSpikeChest2]->load(path + "spike-chest2.cub", database, true, false);
    m_sprites[SpriteSpikeChest3]->load(path + "spike-chest3.cub", database, true, false);
    m_sprites[SpriteSpikeChest4]->load(path + "spike-chest4.cub", database, true, false);
    m_sprites[SpriteSpikeChest5]->load(path + "spike-chest5.cub", database, true, false);
    m_sprites[SpriteSpikeShoulder1]->load(path + "spike-shoulder1.cub", database, true, false);
    m_sprites[SpriteSpikeShoulder2]->load(path + "spike-shoulder2.cub", database, true, false);
    m_sprites[SpriteSpikeShoulder3]->load(path + "spike-shoulder3.cub", database, true, false);
    m_sprites[SpriteSpikeShoulder4]->load(path + "spike-shoulder4.cub", database, true, false);
    m_sprites[SpriteSpikeShoulder5]->load(path + "spike-shoulder5.cub", database, true, false);
    m_sprites[SpriteSpikeFoot1]->load(path + "spike-foot1.cub", database, true, false);
    m_sprites[SpriteSpikeFoot2]->load(path + "spike-foot2.cub", database, true, false);
    m_sprites[SpriteSpikeFoot3]->load(path + "spike-foot3.cub", database, true, false);
    m_sprites[SpriteSpikeFoot4]->load(path + "spike-foot4.cub", database, true, false);
    m_sprites[SpriteSpikeFoot5]->load(path + "spike-foot5.cub", database, true, false);
    m_sprites[SpriteSpikeHand1]->load(path + "spike-hand1.cub", database, true, false);
    m_sprites[SpriteSpikeHand2]->load(path + "spike-hand2.cub", database, true, false);
    m_sprites[SpriteSpikeHand3]->load(path + "spike-hand3.cub", database, true, false);
    m_sprites[SpriteSpikeHand4]->load(path + "spike-hand4.cub", database, true, false);
    m_sprites[SpriteSpikeHand5]->load(path + "spike-hand5.cub", database, true, false);
    m_sprites[SpriteFiberChest1]->load(path + "fiber-chest1.cub", database, true, false);
    m_sprites[SpriteFiberChest2]->load(path + "fiber-chest2.cub", database, true, false);
    m_sprites[SpriteFiberChest3]->load(path + "fiber-chest3.cub", database, true, false);
    m_sprites[SpriteFiberChest4]->load(path + "fiber-chest4.cub", database, true, false);
    m_sprites[SpriteFiberChest5]->load(path + "fiber-chest5.cub", database, true, false);
    m_sprites[SpriteFiberShoulder1]->load(path + "fiber-shoulder1.cub", database, true, false);
    m_sprites[SpriteFiberShoulder2]->load(path + "fiber-shoulder2.cub", database, true, false);
    m_sprites[SpriteFiberShoulder3]->load(path + "fiber-shoulder3.cub", database, true, false);
    m_sprites[SpriteFiberShoulder4]->load(path + "fiber-shoulder4.cub", database, true, false);
    m_sprites[SpriteFiberShoulder5]->load(path + "fiber-shoulder5.cub", database, true, false);
    m_sprites[SpriteFiberFoot1]->load(path + "fiber-foot1.cub", database, true, false);
    m_sprites[SpriteFiberFoot2]->load(path + "fiber-foot2.cub", database, true, false);
    m_sprites[SpriteFiberFoot3]->load(path + "fiber-foot3.cub", database, true, false);
    m_sprites[SpriteFiberFoot4]->load(path + "fiber-foot4.cub", database, true, false);
    m_sprites[SpriteFiberFoot5]->load(path + "fiber-foot5.cub", database, true, false);
    m_sprites[SpriteFiberHand1]->load(path + "fiber-hand1.cub", database, true, false);
    m_sprites[SpriteFiberHand2]->load(path + "fiber-hand2.cub", database, true, false);
    m_sprites[SpriteFiberHand3]->load(path + "fiber-hand3.cub", database, true, false);
    m_sprites[SpriteFiberHand4]->load(path + "fiber-hand4.cub", database, true, false);
    m_sprites[SpriteFiberHand5]->load(path + "fiber-hand5.cub", database, true, false);
    m_sprites[SpriteScaleChest1]->load(path + "scale-chest1.cub", database, true, false);
    m_sprites[SpriteScaleChest2]->load(path + "scale-chest2.cub", database, true, false);
    m_sprites[SpriteScaleChest3]->load(path + "scale-chest3.cub", database, true, false);
    m_sprites[SpriteScaleChest4]->load(path + "scale-chest4.cub", database, true, false);
    m_sprites[SpriteScaleChest5]->load(path + "scale-chest5.cub", database, true, false);
    m_sprites[SpriteScaleShoulder1]->load(path + "scale-shoulder1.cub", database, true, false);
    m_sprites[SpriteScaleShoulder2]->load(path + "scale-shoulder2.cub", database, true, false);
    m_sprites[SpriteScaleShoulder3]->load(path + "scale-shoulder3.cub", database, true, false);
    m_sprites[SpriteScaleShoulder4]->load(path + "scale-shoulder4.cub", database, true, false);
    m_sprites[SpriteScaleShoulder5]->load(path + "scale-shoulder5.cub", database, true, false);
    m_sprites[SpriteScaleFoot1]->load(path + "scale-foot1.cub", database, true, false);
    m_sprites[SpriteScaleFoot2]->load(path + "scale-foot2.cub", database, true, false);
    m_sprites[SpriteScaleFoot3]->load(path + "scale-foot3.cub", database, true, false);
    m_sprites[SpriteScaleFoot4]->load(path + "scale-foot4.cub", database, true, false);
    m_sprites[SpriteScaleFoot5]->load(path + "scale-foot5.cub", database, true, false);
    m_sprites[SpriteScaleHand1]->load(path + "scale-hand1.cub", database, true, false);
    m_sprites[SpriteScaleHand2]->load(path + "scale-hand2.cub", database, true, false);
    m_sprites[SpriteScaleHand3]->load(path + "scale-hand3.cub", database, true, false);
    m_sprites[SpriteScaleHand4]->load(path + "scale-hand4.cub", database, true, false);
    m_sprites[SpriteScaleHand5]->load(path + "scale-hand5.cub", database, true, false);
    m_sprites[SpriteWebChest1]->load(path + "web-chest1.cub", database, true, false);
    m_sprites[SpriteWebChest2]->load(path + "web-chest2.cub", database, true, false);
    m_sprites[SpriteWebChest3]->load(path + "web-chest3.cub", database, true, false);
    m_sprites[SpriteWebChest4]->load(path + "web-chest4.cub", database, true, false);
    m_sprites[SpriteWebChest5]->load(path + "web-chest5.cub", database, true, false);
    m_sprites[SpriteWebShoulder1]->load(path + "web-shoulder1.cub", database, true, false);
    m_sprites[SpriteWebShoulder2]->load(path + "web-shoulder2.cub", database, true, false);
    m_sprites[SpriteWebShoulder3]->load(path + "web-shoulder3.cub", database, true, false);
    m_sprites[SpriteWebShoulder4]->load(path + "web-shoulder4.cub", database, true, false);
    m_sprites[SpriteWebShoulder5]->load(path + "web-shoulder5.cub", database, true, false);
    m_sprites[SpriteWebFoot1]->load(path + "web-foot1.cub", database, true, false);
    m_sprites[SpriteWebFoot2]->load(path + "web-foot2.cub", database, true, false);
    m_sprites[SpriteWebFoot3]->load(path + "web-foot3.cub", database, true, false);
    m_sprites[SpriteWebFoot4]->load(path + "web-foot4.cub", database, true, false);
    m_sprites[SpriteWebFoot5]->load(path + "web-foot5.cub", database, true, false);
    m_sprites[SpriteWebHand1]->load(path + "web-hand1.cub", database, true, false);
    m_sprites[SpriteWebHand2]->load(path + "web-hand2.cub", database, true, false);
    m_sprites[SpriteWebHand3]->load(path + "web-hand3.cub", database, true, false);
    m_sprites[SpriteWebHand4]->load(path + "web-hand4.cub", database, true, false);
    m_sprites[SpriteWebHand5]->load(path + "web-hand5.cub", database, true, false);
    m_sprites[SpriteShadowChest1]->load(path + "shadow-chest1.cub", database, true, false);
    m_sprites[SpriteShadowChest2]->load(path + "shadow-chest2.cub", database, true, false);
    m_sprites[SpriteShadowChest3]->load(path + "shadow-chest3.cub", database, true, false);
    m_sprites[SpriteShadowChest4]->load(path + "shadow-chest4.cub", database, true, false);
    m_sprites[SpriteShadowChest5]->load(path + "shadow-chest5.cub", database, true, false);
    m_sprites[SpriteShadowShoulder1]->load(path + "shadow-shoulder1.cub", database, true, false);
    m_sprites[SpriteShadowShoulder2]->load(path + "shadow-shoulder2.cub", database, true, false);
    m_sprites[SpriteShadowShoulder3]->load(path + "shadow-shoulder3.cub", database, true, false);
    m_sprites[SpriteShadowShoulder4]->load(path + "shadow-shoulder4.cub", database, true, false);
    m_sprites[SpriteShadowShoulder5]->load(path + "shadow-shoulder5.cub", database, true, false);
    m_sprites[SpriteShadowFoot1]->load(path + "shadow-foot1.cub", database, true, false);
    m_sprites[SpriteShadowFoot2]->load(path + "shadow-foot2.cub", database, true, false);
    m_sprites[SpriteShadowFoot3]->load(path + "shadow-foot3.cub", database, true, false);
    m_sprites[SpriteShadowFoot4]->load(path + "shadow-foot4.cub", database, true, false);
    m_sprites[SpriteShadowFoot5]->load(path + "shadow-foot5.cub", database, true, false);
    m_sprites[SpriteShadowHand1]->load(path + "shadow-hand1.cub", database, true, false);
    m_sprites[SpriteShadowHand2]->load(path + "shadow-hand2.cub", database, true, false);
    m_sprites[SpriteShadowHand3]->load(path + "shadow-hand3.cub", database, true, false);
    m_sprites[SpriteShadowHand4]->load(path + "shadow-hand4.cub", database, true, false);
    m_sprites[SpriteShadowHand5]->load(path + "shadow-hand5.cub", database, true, false);
    m_sprites[SpriteDarkChest1]->load(path + "dark-chest1.cub", database, true, false);
    m_sprites[SpriteDarkChest2]->load(path + "dark-chest2.cub", database, true, false);
    m_sprites[SpriteDarkChest3]->load(path + "dark-chest3.cub", database, true, false);
    m_sprites[SpriteDarkChest4]->load(path + "dark-chest4.cub", database, true, false);
    m_sprites[SpriteDarkChest5]->load(path + "dark-chest5.cub", database, true, false);
    m_sprites[SpriteDarkShoulder1]->load(path + "dark-shoulder1.cub", database, true, false);
    m_sprites[SpriteDarkShoulder2]->load(path + "dark-shoulder2.cub", database, true, false);
    m_sprites[SpriteDarkShoulder3]->load(path + "dark-shoulder3.cub", database, true, false);
    m_sprites[SpriteDarkShoulder4]->load(path + "dark-shoulder4.cub", database, true, false);
    m_sprites[SpriteDarkShoulder5]->load(path + "dark-shoulder5.cub", database, true, false);
    m_sprites[SpriteDarkFoot1]->load(path + "dark-foot1.cub", database, true, false);
    m_sprites[SpriteDarkFoot2]->load(path + "dark-foot2.cub", database, true, false);
    m_sprites[SpriteDarkFoot3]->load(path + "dark-foot3.cub", database, true, false);
    m_sprites[SpriteDarkFoot4]->load(path + "dark-foot4.cub", database, true, false);
    m_sprites[SpriteDarkFoot5]->load(path + "dark-foot5.cub", database, true, false);
    m_sprites[SpriteDarkHand1]->load(path + "dark-hand1.cub", database, true, false);
    m_sprites[SpriteDarkHand2]->load(path + "dark-hand2.cub", database, true, false);
    m_sprites[SpriteDarkHand3]->load(path + "dark-hand3.cub", database, true, false);
    m_sprites[SpriteDarkHand4]->load(path + "dark-hand4.cub", database, true, false);
    m_sprites[SpriteDarkHand5]->load(path + "dark-hand5.cub", database, true, false);
    m_sprites[SpriteFireChest1]->load(path + "fire-chest1.cub", database, true, false);
    m_sprites[SpriteFireChest2]->load(path + "fire-chest2.cub", database, true, false);
    m_sprites[SpriteFireChest3]->load(path + "fire-chest3.cub", database, true, false);
    m_sprites[SpriteFireChest4]->load(path + "fire-chest4.cub", database, true, false);
    m_sprites[SpriteFireChest5]->load(path + "fire-chest5.cub", database, true, false);
    m_sprites[SpriteFireShoulder1]->load(path + "fire-shoulder1.cub", database, true, false);
    m_sprites[SpriteFireShoulder2]->load(path + "fire-shoulder2.cub", database, true, false);
    m_sprites[SpriteFireShoulder3]->load(path + "fire-shoulder3.cub", database, true, false);
    m_sprites[SpriteFireShoulder4]->load(path + "fire-shoulder4.cub", database, true, false);
    m_sprites[SpriteFireShoulder5]->load(path + "fire-shoulder5.cub", database, true, false);
    m_sprites[SpriteFireFoot1]->load(path + "fire-foot1.cub", database, true, false);
    m_sprites[SpriteFireFoot2]->load(path + "fire-foot2.cub", database, true, false);
    m_sprites[SpriteFireFoot3]->load(path + "fire-foot3.cub", database, true, false);
    m_sprites[SpriteFireFoot4]->load(path + "fire-foot4.cub", database, true, false);
    m_sprites[SpriteFireFoot5]->load(path + "fire-foot5.cub", database, true, false);
    m_sprites[SpriteFireHand1]->load(path + "fire-hand1.cub", database, true, false);
    m_sprites[SpriteFireHand2]->load(path + "fire-hand2.cub", database, true, false);
    m_sprites[SpriteFireHand3]->load(path + "fire-hand3.cub", database, true, false);
    m_sprites[SpriteFireHand4]->load(path + "fire-hand4.cub", database, true, false);
    m_sprites[SpriteFireHand5]->load(path + "fire-hand5.cub", database, true, false);
    m_sprites[SpriteDemonChest1]->load(path + "demon-chest1.cub", database, true, false);
    m_sprites[SpriteDemonChest2]->load(path + "demon-chest2.cub", database, true, false);
    m_sprites[SpriteDemonChest3]->load(path + "demon-chest3.cub", database, true, false);
    m_sprites[SpriteDemonChest4]->load(path + "demon-chest4.cub", database, true, false);
    m_sprites[SpriteDemonChest5]->load(path + "demon-chest5.cub", database, true, false);
    m_sprites[SpriteDemonShoulder1]->load(path + "demon-shoulder1.cub", database, true, false);
    m_sprites[SpriteDemonShoulder2]->load(path + "demon-shoulder2.cub", database, true, false);
    m_sprites[SpriteDemonShoulder3]->load(path + "demon-shoulder3.cub", database, true, false);
    m_sprites[SpriteDemonShoulder4]->load(path + "demon-shoulder4.cub", database, true, false);
    m_sprites[SpriteDemonShoulder5]->load(path + "demon-shoulder5.cub", database, true, false);
    m_sprites[SpriteDemonFoot1]->load(path + "demon-foot1.cub", database, true, false);
    m_sprites[SpriteDemonFoot2]->load(path + "demon-foot2.cub", database, true, false);
    m_sprites[SpriteDemonFoot3]->load(path + "demon-foot3.cub", database, true, false);
    m_sprites[SpriteDemonFoot4]->load(path + "demon-foot4.cub", database, true, false);
    m_sprites[SpriteDemonFoot5]->load(path + "demon-foot5.cub", database, true, false);
    m_sprites[SpriteDemonHand1]->load(path + "demon-hand1.cub", database, true, false);
    m_sprites[SpriteDemonHand2]->load(path + "demon-hand2.cub", database, true, false);
    m_sprites[SpriteDemonHand3]->load(path + "demon-hand3.cub", database, true, false);
    m_sprites[SpriteDemonHand4]->load(path + "demon-hand4.cub", database, true, false);
    m_sprites[SpriteDemonHand5]->load(path + "demon-hand5.cub", database, true, false);
    m_sprites[SpriteSilkChest1]->load(path + "silk-chest1.cub", database, true, false);
    m_sprites[SpriteSilkChest2]->load(path + "silk-chest2.cub", database, true, false);
    m_sprites[SpriteSilkChest3]->load(path + "silk-chest3.cub", database, true, false);
    m_sprites[SpriteSilkChest4]->load(path + "silk-chest4.cub", database, true, false);
    m_sprites[SpriteSilkChest5]->load(path + "silk-chest5.cub", database, true, false);
    m_sprites[SpriteSilkChest1Random1]->load(path + "silk-chest1-random1.cub", database, true, false);
    m_sprites[SpriteSilkChest2Random1]->load(path + "silk-chest2-random1.cub", database, true, false);
    m_sprites[SpriteSilkChest3Random1]->load(path + "silk-chest3-random1.cub", database, true, false);
    m_sprites[SpriteSilkChest4Random1]->load(path + "silk-chest4-random1.cub", database, true, false);
    m_sprites[SpriteSilkChest5Random1]->load(path + "silk-chest5-random1.cub", database, true, false);
    m_sprites[SpriteSilkChest1Random2]->load(path + "silk-chest1-random2.cub", database, true, false);
    m_sprites[SpriteSilkChest2Random2]->load(path + "silk-chest2-random2.cub", database, true, false);
    m_sprites[SpriteSilkChest3Random2]->load(path + "silk-chest3-random2.cub", database, true, false);
    m_sprites[SpriteSilkChest4Random2]->load(path + "silk-chest4-random2.cub", database, true, false);
    m_sprites[SpriteSilkChest5Random2]->load(path + "silk-chest5-random2.cub", database, true, false);
    m_sprites[SpriteSilkChest1Random3]->load(path + "silk-chest1-random3.cub", database, true, false);
    m_sprites[SpriteSilkChest2Random3]->load(path + "silk-chest2-random3.cub", database, true, false);
    m_sprites[SpriteSilkChest3Random3]->load(path + "silk-chest3-random3.cub", database, true, false);
    m_sprites[SpriteSilkChest4Random3]->load(path + "silk-chest4-random3.cub", database, true, false);
    m_sprites[SpriteSilkChest5Random3]->load(path + "silk-chest5-random3.cub", database, true, false);
    m_sprites[SpriteSilkChest1Random4]->load(path + "silk-chest1-random4.cub", database, true, false);
    m_sprites[SpriteSilkChest2Random4]->load(path + "silk-chest2-random4.cub", database, true, false);
    m_sprites[SpriteSilkChest3Random4]->load(path + "silk-chest3-random4.cub", database, true, false);
    m_sprites[SpriteSilkChest4Random4]->load(path + "silk-chest4-random4.cub", database, true, false);
    m_sprites[SpriteSilkChest5Random4]->load(path + "silk-chest5-random4.cub", database, true, false);
    m_sprites[SpriteSilkShoulder1]->load(path + "silk-shoulder1.cub", database, true, false);
    m_sprites[SpriteSilkShoulder2]->load(path + "silk-shoulder2.cub", database, true, false);
    m_sprites[SpriteSilkShoulder3]->load(path + "silk-shoulder3.cub", database, true, false);
    m_sprites[SpriteSilkShoulder4]->load(path + "silk-shoulder4.cub", database, true, false);
    m_sprites[SpriteSilkShoulder5]->load(path + "silk-shoulder5.cub", database, true, false);
    m_sprites[SpriteSilkShoulder1Random1]->load(path + "silk-shoulder1-random1.cub", database, true, false);
    m_sprites[SpriteSilkShoulder2Random1]->load(path + "silk-shoulder2-random1.cub", database, true, false);
    m_sprites[SpriteSilkShoulder3Random1]->load(path + "silk-shoulder3-random1.cub", database, true, false);
    m_sprites[SpriteSilkShoulder4Random1]->load(path + "silk-shoulder4-random1.cub", database, true, false);
    m_sprites[SpriteSilkShoulder5Random1]->load(path + "silk-shoulder5-random1.cub", database, true, false);
    m_sprites[SpriteSilkShoulder1Random2]->load(path + "silk-shoulder1-random2.cub", database, true, false);
    m_sprites[SpriteSilkShoulder2Random2]->load(path + "silk-shoulder2-random2.cub", database, true, false);
    m_sprites[SpriteSilkShoulder3Random2]->load(path + "silk-shoulder3-random2.cub", database, true, false);
    m_sprites[SpriteSilkShoulder4Random2]->load(path + "silk-shoulder4-random2.cub", database, true, false);
    m_sprites[SpriteSilkShoulder5Random2]->load(path + "silk-shoulder5-random2.cub", database, true, false);
    m_sprites[SpriteSilkShoulder1Random3]->load(path + "silk-shoulder1-random3.cub", database, true, false);
    m_sprites[SpriteSilkShoulder2Random3]->load(path + "silk-shoulder2-random3.cub", database, true, false);
    m_sprites[SpriteSilkShoulder3Random3]->load(path + "silk-shoulder3-random3.cub", database, true, false);
    m_sprites[SpriteSilkShoulder4Random3]->load(path + "silk-shoulder4-random3.cub", database, true, false);
    m_sprites[SpriteSilkShoulder5Random3]->load(path + "silk-shoulder5-random3.cub", database, true, false);
    m_sprites[SpriteSilkShoulder1Random4]->load(path + "silk-shoulder1-random4.cub", database, true, false);
    m_sprites[SpriteSilkShoulder2Random4]->load(path + "silk-shoulder2-random4.cub", database, true, false);
    m_sprites[SpriteSilkShoulder3Random4]->load(path + "silk-shoulder3-random4.cub", database, true, false);
    m_sprites[SpriteSilkShoulder4Random4]->load(path + "silk-shoulder4-random4.cub", database, true, false);
    m_sprites[SpriteSilkShoulder5Random4]->load(path + "silk-shoulder5-random4.cub", database, true, false);
    m_sprites[SpriteSilkHand1]->load(path + "silk-hand1.cub", database, true, false);
    m_sprites[SpriteSilkHand2]->load(path + "silk-hand2.cub", database, true, false);
    m_sprites[SpriteSilkHand3]->load(path + "silk-hand3.cub", database, true, false);
    m_sprites[SpriteSilkHand4]->load(path + "silk-hand4.cub", database, true, false);
    m_sprites[SpriteSilkHand5]->load(path + "silk-hand5.cub", database, true, false);
    m_sprites[SpriteSilkHand1Random1]->load(path + "silk-hand1-random1.cub", database, true, false);
    m_sprites[SpriteSilkHand2Random1]->load(path + "silk-hand2-random1.cub", database, true, false);
    m_sprites[SpriteSilkHand3Random1]->load(path + "silk-hand3-random1.cub", database, true, false);
    m_sprites[SpriteSilkHand4Random1]->load(path + "silk-hand4-random1.cub", database, true, false);
    m_sprites[SpriteSilkHand5Random1]->load(path + "silk-hand5-random1.cub", database, true, false);
    m_sprites[SpriteSilkHand1Random2]->load(path + "silk-hand1-random2.cub", database, true, false);
    m_sprites[SpriteSilkHand2Random2]->load(path + "silk-hand2-random2.cub", database, true, false);
    m_sprites[SpriteSilkHand3Random2]->load(path + "silk-hand3-random2.cub", database, true, false);
    m_sprites[SpriteSilkHand4Random2]->load(path + "silk-hand4-random2.cub", database, true, false);
    m_sprites[SpriteSilkHand5Random2]->load(path + "silk-hand5-random2.cub", database, true, false);
    m_sprites[SpriteSilkHand1Random3]->load(path + "silk-hand1-random3.cub", database, true, false);
    m_sprites[SpriteSilkHand2Random3]->load(path + "silk-hand2-random3.cub", database, true, false);
    m_sprites[SpriteSilkHand3Random3]->load(path + "silk-hand3-random3.cub", database, true, false);
    m_sprites[SpriteSilkHand4Random3]->load(path + "silk-hand4-random3.cub", database, true, false);
    m_sprites[SpriteSilkHand5Random3]->load(path + "silk-hand5-random3.cub", database, true, false);
    m_sprites[SpriteSilkHand1Random4]->load(path + "silk-hand1-random4.cub", database, true, false);
    m_sprites[SpriteSilkHand2Random4]->load(path + "silk-hand2-random4.cub", database, true, false);
    m_sprites[SpriteSilkHand3Random4]->load(path + "silk-hand3-random4.cub", database, true, false);
    m_sprites[SpriteSilkHand4Random4]->load(path + "silk-hand4-random4.cub", database, true, false);
    m_sprites[SpriteSilkHand5Random4]->load(path + "silk-hand5-random4.cub", database, true, false);
    m_sprites[SpriteSilkFoot1]->load(path + "silk-foot1.cub", database, true, false);
    m_sprites[SpriteSilkFoot2]->load(path + "silk-foot2.cub", database, true, false);
    m_sprites[SpriteSilkFoot3]->load(path + "silk-foot3.cub", database, true, false);
    m_sprites[SpriteSilkFoot4]->load(path + "silk-foot4.cub", database, true, false);
    m_sprites[SpriteSilkFoot5]->load(path + "silk-foot5.cub", database, true, false);
    m_sprites[SpriteSilkFoot1Random1]->load(path + "silk-foot1-random1.cub", database, true, false);
    m_sprites[SpriteSilkFoot2Random1]->load(path + "silk-foot2-random1.cub", database, true, false);
    m_sprites[SpriteSilkFoot3Random1]->load(path + "silk-foot3-random1.cub", database, true, false);
    m_sprites[SpriteSilkFoot4Random1]->load(path + "silk-foot4-random1.cub", database, true, false);
    m_sprites[SpriteSilkFoot5Random1]->load(path + "silk-foot5-random1.cub", database, true, false);
    m_sprites[SpriteSilkFoot1Random2]->load(path + "silk-foot1-random2.cub", database, true, false);
    m_sprites[SpriteSilkFoot2Random2]->load(path + "silk-foot2-random2.cub", database, true, false);
    m_sprites[SpriteSilkFoot3Random2]->load(path + "silk-foot3-random2.cub", database, true, false);
    m_sprites[SpriteSilkFoot4Random2]->load(path + "silk-foot4-random2.cub", database, true, false);
    m_sprites[SpriteSilkFoot5Random2]->load(path + "silk-foot5-random2.cub", database, true, false);
    m_sprites[SpriteSilkFoot1Random3]->load(path + "silk-foot1-random3.cub", database, true, false);
    m_sprites[SpriteSilkFoot2Random3]->load(path + "silk-foot2-random3.cub", database, true, false);
    m_sprites[SpriteSilkFoot3Random3]->load(path + "silk-foot3-random3.cub", database, true, false);
    m_sprites[SpriteSilkFoot4Random3]->load(path + "silk-foot4-random3.cub", database, true, false);
    m_sprites[SpriteSilkFoot5Random3]->load(path + "silk-foot5-random3.cub", database, true, false);
    m_sprites[SpriteSilkFoot1Random4]->load(path + "silk-foot1-random4.cub", database, true, false);
    m_sprites[SpriteSilkFoot2Random4]->load(path + "silk-foot2-random4.cub", database, true, false);
    m_sprites[SpriteSilkFoot3Random4]->load(path + "silk-foot3-random4.cub", database, true, false);
    m_sprites[SpriteSilkFoot4Random4]->load(path + "silk-foot4-random4.cub", database, true, false);
    m_sprites[SpriteSilkFoot5Random4]->load(path + "silk-foot5-random4.cub", database, true, false);
    m_sprites[SpriteGoldAmulet1Random1]->load(path + "gold-amulet1-random1.cub", database, true, false);
    m_sprites[SpriteGoldAmulet2Random1]->load(path + "gold-amulet2-random1.cub", database, true, false);
    m_sprites[SpriteGoldAmulet3Random1]->load(path + "gold-amulet3-random1.cub", database, true, false);
    m_sprites[SpriteGoldAmulet4Random1]->load(path + "gold-amulet4-random1.cub", database, true, false);
    m_sprites[SpriteGoldAmulet5Random1]->load(path + "gold-amulet5-random1.cub", database, true, false);
    m_sprites[SpriteGoldAmulet1Random2]->load(path + "gold-amulet1-random2.cub", database, true, false);
    m_sprites[SpriteGoldAmulet2Random2]->load(path + "gold-amulet2-random2.cub", database, true, false);
    m_sprites[SpriteGoldAmulet3Random2]->load(path + "gold-amulet3-random2.cub", database, true, false);
    m_sprites[SpriteGoldAmulet4Random2]->load(path + "gold-amulet4-random2.cub", database, true, false);
    m_sprites[SpriteGoldAmulet5Random2]->load(path + "gold-amulet5-random2.cub", database, true, false);
    m_sprites[SpriteGoldAmulet1Random3]->load(path + "gold-amulet1-random3.cub", database, true, false);
    m_sprites[SpriteGoldAmulet2Random3]->load(path + "gold-amulet2-random3.cub", database, true, false);
    m_sprites[SpriteGoldAmulet3Random3]->load(path + "gold-amulet3-random3.cub", database, true, false);
    m_sprites[SpriteGoldAmulet4Random3]->load(path + "gold-amulet4-random3.cub", database, true, false);
    m_sprites[SpriteGoldAmulet5Random3]->load(path + "gold-amulet5-random3.cub", database, true, false);
    m_sprites[SpriteGoldAmulet1Random4]->load(path + "gold-amulet1-random4.cub", database, true, false);
    m_sprites[SpriteGoldAmulet2Random4]->load(path + "gold-amulet2-random4.cub", database, true, false);
    m_sprites[SpriteGoldAmulet3Random4]->load(path + "gold-amulet3-random4.cub", database, true, false);
    m_sprites[SpriteGoldAmulet4Random4]->load(path + "gold-amulet4-random4.cub", database, true, false);
    m_sprites[SpriteGoldAmulet5Random4]->load(path + "gold-amulet5-random4.cub", database, true, false);
    m_sprites[SpriteGoldAmulet1Random5]->load(path + "gold-amulet1-random5.cub", database, true, false);
    m_sprites[SpriteGoldAmulet2Random5]->load(path + "gold-amulet2-random5.cub", database, true, false);
    m_sprites[SpriteGoldAmulet3Random5]->load(path + "gold-amulet3-random5.cub", database, true, false);
    m_sprites[SpriteGoldAmulet4Random5]->load(path + "gold-amulet4-random5.cub", database, true, false);
    m_sprites[SpriteGoldAmulet5Random5]->load(path + "gold-amulet5-random5.cub", database, true, false);
    m_sprites[SpriteSilverAmulet1Random1]->load(path + "silver-amulet1-random1.cub", database, true, false);
    m_sprites[SpriteSilverAmulet2Random1]->load(path + "silver-amulet2-random1.cub", database, true, false);
    m_sprites[SpriteSilverAmulet3Random1]->load(path + "silver-amulet3-random1.cub", database, true, false);
    m_sprites[SpriteSilverAmulet4Random1]->load(path + "silver-amulet4-random1.cub", database, true, false);
    m_sprites[SpriteSilverAmulet5Random1]->load(path + "silver-amulet5-random1.cub", database, true, false);
    m_sprites[SpriteSilverAmulet1Random2]->load(path + "silver-amulet1-random2.cub", database, true, false);
    m_sprites[SpriteSilverAmulet2Random2]->load(path + "silver-amulet2-random2.cub", database, true, false);
    m_sprites[SpriteSilverAmulet3Random2]->load(path + "silver-amulet3-random2.cub", database, true, false);
    m_sprites[SpriteSilverAmulet4Random2]->load(path + "silver-amulet4-random2.cub", database, true, false);
    m_sprites[SpriteSilverAmulet5Random2]->load(path + "silver-amulet5-random2.cub", database, true, false);
    m_sprites[SpriteSilverAmulet1Random3]->load(path + "silver-amulet1-random3.cub", database, true, false);
    m_sprites[SpriteSilverAmulet2Random3]->load(path + "silver-amulet2-random3.cub", database, true, false);
    m_sprites[SpriteSilverAmulet3Random3]->load(path + "silver-amulet3-random3.cub", database, true, false);
    m_sprites[SpriteSilverAmulet4Random3]->load(path + "silver-amulet4-random3.cub", database, true, false);
    m_sprites[SpriteSilverAmulet5Random3]->load(path + "silver-amulet5-random3.cub", database, true, false);
    m_sprites[SpriteSilverAmulet1Random4]->load(path + "silver-amulet1-random4.cub", database, true, false);
    m_sprites[SpriteSilverAmulet2Random4]->load(path + "silver-amulet2-random4.cub", database, true, false);
    m_sprites[SpriteSilverAmulet3Random4]->load(path + "silver-amulet3-random4.cub", database, true, false);
    m_sprites[SpriteSilverAmulet4Random4]->load(path + "silver-amulet4-random4.cub", database, true, false);
    m_sprites[SpriteSilverAmulet5Random4]->load(path + "silver-amulet5-random4.cub", database, true, false);
    m_sprites[SpriteSilverAmulet1Random5]->load(path + "silver-amulet1-random5.cub", database, true, false);
    m_sprites[SpriteSilverAmulet2Random5]->load(path + "silver-amulet2-random5.cub", database, true, false);
    m_sprites[SpriteSilverAmulet3Random5]->load(path + "silver-amulet3-random5.cub", database, true, false);
    m_sprites[SpriteSilverAmulet4Random5]->load(path + "silver-amulet4-random5.cub", database, true, false);
    m_sprites[SpriteSilverAmulet5Random5]->load(path + "silver-amulet5-random5.cub", database, true, false);
    m_sprites[SpriteGoldRing1Random1]->load(path + "gold-ring1-random1.cub", database, true, false);
    m_sprites[SpriteGoldRing2Random1]->load(path + "gold-ring2-random1.cub", database, true, false);
    m_sprites[SpriteGoldRing3Random1]->load(path + "gold-ring3-random1.cub", database, true, false);
    m_sprites[SpriteGoldRing4Random1]->load(path + "gold-ring4-random1.cub", database, true, false);
    m_sprites[SpriteGoldRing5Random1]->load(path + "gold-ring5-random1.cub", database, true, false);
    m_sprites[SpriteGoldRing1Random2]->load(path + "gold-ring1-random2.cub", database, true, false);
    m_sprites[SpriteGoldRing2Random2]->load(path + "gold-ring2-random2.cub", database, true, false);
    m_sprites[SpriteGoldRing3Random2]->load(path + "gold-ring3-random2.cub", database, true, false);
    m_sprites[SpriteGoldRing4Random2]->load(path + "gold-ring4-random2.cub", database, true, false);
    m_sprites[SpriteGoldRing5Random2]->load(path + "gold-ring5-random2.cub", database, true, false);
    m_sprites[SpriteGoldRing1Random3]->load(path + "gold-ring1-random3.cub", database, true, false);
    m_sprites[SpriteGoldRing2Random3]->load(path + "gold-ring2-random3.cub", database, true, false);
    m_sprites[SpriteGoldRing3Random3]->load(path + "gold-ring3-random3.cub", database, true, false);
    m_sprites[SpriteGoldRing4Random3]->load(path + "gold-ring4-random3.cub", database, true, false);
    m_sprites[SpriteGoldRing5Random3]->load(path + "gold-ring5-random3.cub", database, true, false);
    m_sprites[SpriteGoldRing1Random4]->load(path + "gold-ring1-random4.cub", database, true, false);
    m_sprites[SpriteGoldRing2Random4]->load(path + "gold-ring2-random4.cub", database, true, false);
    m_sprites[SpriteGoldRing3Random4]->load(path + "gold-ring3-random4.cub", database, true, false);
    m_sprites[SpriteGoldRing4Random4]->load(path + "gold-ring4-random4.cub", database, true, false);
    m_sprites[SpriteGoldRing5Random4]->load(path + "gold-ring5-random4.cub", database, true, false);
    m_sprites[SpriteGoldRing1Random5]->load(path + "gold-ring1-random5.cub", database, true, false);
    m_sprites[SpriteGoldRing2Random5]->load(path + "gold-ring2-random5.cub", database, true, false);
    m_sprites[SpriteGoldRing3Random5]->load(path + "gold-ring3-random5.cub", database, true, false);
    m_sprites[SpriteGoldRing4Random5]->load(path + "gold-ring4-random5.cub", database, true, false);
    m_sprites[SpriteGoldRing5Random5]->load(path + "gold-ring5-random5.cub", database, true, false);
    m_sprites[SpriteSilverRing1Random1]->load(path + "silver-ring1-random1.cub", database, true, false);
    m_sprites[SpriteSilverRing2Random1]->load(path + "silver-ring2-random1.cub", database, true, false);
    m_sprites[SpriteSilverRing3Random1]->load(path + "silver-ring3-random1.cub", database, true, false);
    m_sprites[SpriteSilverRing4Random1]->load(path + "silver-ring4-random1.cub", database, true, false);
    m_sprites[SpriteSilverRing5Random1]->load(path + "silver-ring5-random1.cub", database, true, false);
    m_sprites[SpriteSilverRing1Random2]->load(path + "silver-ring1-random2.cub", database, true, false);
    m_sprites[SpriteSilverRing2Random2]->load(path + "silver-ring2-random2.cub", database, true, false);
    m_sprites[SpriteSilverRing3Random2]->load(path + "silver-ring3-random2.cub", database, true, false);
    m_sprites[SpriteSilverRing4Random2]->load(path + "silver-ring4-random2.cub", database, true, false);
    m_sprites[SpriteSilverRing5Random2]->load(path + "silver-ring5-random2.cub", database, true, false);
    m_sprites[SpriteSilverRing1Random3]->load(path + "silver-ring1-random3.cub", database, true, false);
    m_sprites[SpriteSilverRing2Random3]->load(path + "silver-ring2-random3.cub", database, true, false);
    m_sprites[SpriteSilverRing3Random3]->load(path + "silver-ring3-random3.cub", database, true, false);
    m_sprites[SpriteSilverRing4Random3]->load(path + "silver-ring4-random3.cub", database, true, false);
    m_sprites[SpriteSilverRing5Random3]->load(path + "silver-ring5-random3.cub", database, true, false);
    m_sprites[SpriteSilverRing1Random4]->load(path + "silver-ring1-random4.cub", database, true, false);
    m_sprites[SpriteSilverRing2Random4]->load(path + "silver-ring2-random4.cub", database, true, false);
    m_sprites[SpriteSilverRing3Random4]->load(path + "silver-ring3-random4.cub", database, true, false);
    m_sprites[SpriteSilverRing4Random4]->load(path + "silver-ring4-random4.cub", database, true, false);
    m_sprites[SpriteSilverRing5Random4]->load(path + "silver-ring5-random4.cub", database, true, false);
    m_sprites[SpriteSilverRing1Random5]->load(path + "silver-ring1-random5.cub", database, true, false);
    m_sprites[SpriteSilverRing2Random5]->load(path + "silver-ring2-random5.cub", database, true, false);
    m_sprites[SpriteSilverRing3Random5]->load(path + "silver-ring3-random5.cub", database, true, false);
    m_sprites[SpriteSilverRing4Random5]->load(path + "silver-ring4-random5.cub", database, true, false);
    m_sprites[SpriteSilverRing5Random5]->load(path + "silver-ring5-random5.cub", database, true, false);
    m_sprites[SpriteParrotFeather]->load(path + "parrot-feather.cub", database, true, false);
    m_sprites[SpriteParrotBeak]->load(path + "parrot-beak.cub", database, true, false);
    m_sprites[SpriteMammothChest]->load(path + "mammoth-chest.cub", database, true, false);
    m_sprites[SpriteMammothShoulder]->load(path + "mammoth-shoulder.cub", database, true, false);
    m_sprites[SpriteMammothBoot]->load(path + "mammoth-boot.cub", database, true, false);
    m_sprites[SpriteMammothGlove]->load(path + "mammoth-glove.cub", database, true, false);
    m_sprites[SpriteMammothHelmet]->load(path + "mammoth-helmet.cub", database, true, false);
    m_sprites[SpriteMammothHair]->load(path + "mammoth-hair.cub", database, true, false);
    m_sprites[SpriteFlask]->load(path + "flask.cub", database, true, false);
    m_sprites[SpriteWaterFlask]->load(path + "water-flask.cub", database, true, false);
    m_sprites[SpriteRadishSlice]->load(path + "radish-slice.cub", database, true, false);
    m_sprites[SpriteOnionSlice]->load(path + "onion-slice.cub", database, true, false);
    m_sprites[SpritePlantFiber]->load(path + "plant-fiber.cub", database, true, false);
    m_sprites[SpriteSoupGinseng]->load(path + "soup-ginseng.cub", database, true, false);
    m_sprites[SpriteSoupChili]->load(path + "soup-chili.cub", database, true, false);
    m_sprites[SpriteSoupGinger]->load(path + "soup-ginger.cub", database, true, false);
    m_sprites[SpriteMushroomSpit]->load(path + "mushroom-spit.cub", database, true, false);
    m_sprites[SpriteObsidianSword]->load(path + "obsidian-sword.cub", database, true, false);
    m_sprites[SpriteBoneChest]->load(path + "bone-chest.cub", database, true, false);
    m_sprites[SpriteBoneShoulders]->load(path + "bone-shoulders.cub", database, true, false);
    m_sprites[SpriteBoneBoot]->load(path + "bone-boot.cub", database, true, false);
    m_sprites[SpriteBoneGlove]->load(path + "bone-glove.cub", database, true, false);
    m_sprites[SpriteBoneMace]->load(path + "bone-mace.cub", database, true, false);
    m_sprites[SpriteBoneSword]->load(path + "bone-sword.cub", database, true, false);
    m_sprites[SpriteBoneAxe]->load(path + "bone-axe.cub", database, true, false);
    m_sprites[SpriteIronChest1]->load(path + "iron-chest1.cub", database, true, false);
    m_sprites[SpriteIronChest2]->load(path + "iron-chest2.cub", database, true, false);
    m_sprites[SpriteIronChest3]->load(path + "iron-chest3.cub", database, true, false);
    m_sprites[SpriteIronChest4]->load(path + "iron-chest4.cub", database, true, false);
    m_sprites[SpriteIronChest5]->load(path + "iron-chest5.cub", database, true, false);
    m_sprites[SpriteIronChest1Random1]->load(path + "iron-chest1-random1.cub", database, true, false);
    m_sprites[SpriteIronChest2Random1]->load(path + "iron-chest2-random1.cub", database, true, false);
    m_sprites[SpriteIronChest3Random1]->load(path + "iron-chest3-random1.cub", database, true, false);
    m_sprites[SpriteIronChest4Random1]->load(path + "iron-chest4-random1.cub", database, true, false);
    m_sprites[SpriteIronChest5Random1]->load(path + "iron-chest5-random1.cub", database, true, false);
    m_sprites[SpriteIronChest1Random2]->load(path + "iron-chest1-random2.cub", database, true, false);
    m_sprites[SpriteIronChest2Random2]->load(path + "iron-chest2-random2.cub", database, true, false);
    m_sprites[SpriteIronChest3Random2]->load(path + "iron-chest3-random2.cub", database, true, false);
    m_sprites[SpriteIronChest4Random2]->load(path + "iron-chest4-random2.cub", database, true, false);
    m_sprites[SpriteIronChest5Random2]->load(path + "iron-chest5-random2.cub", database, true, false);
    m_sprites[SpriteIronChest1Random3]->load(path + "iron-chest1-random3.cub", database, true, false);
    m_sprites[SpriteIronChest2Random3]->load(path + "iron-chest2-random3.cub", database, true, false);
    m_sprites[SpriteIronChest3Random3]->load(path + "iron-chest3-random3.cub", database, true, false);
    m_sprites[SpriteIronChest4Random3]->load(path + "iron-chest4-random3.cub", database, true, false);
    m_sprites[SpriteIronChest5Random3]->load(path + "iron-chest5-random3.cub", database, true, false);
    m_sprites[SpriteIronChest1Random4]->load(path + "iron-chest1-random4.cub", database, true, false);
    m_sprites[SpriteIronChest2Random4]->load(path + "iron-chest2-random4.cub", database, true, false);
    m_sprites[SpriteIronChest3Random4]->load(path + "iron-chest3-random4.cub", database, true, false);
    m_sprites[SpriteIronChest4Random4]->load(path + "iron-chest4-random4.cub", database, true, false);
    m_sprites[SpriteIronChest5Random4]->load(path + "iron-chest5-random4.cub", database, true, false);
    m_sprites[SpriteIronShoulder1]->load(path + "iron-shoulder1.cub", database, true, false);
    m_sprites[SpriteIronShoulder2]->load(path + "iron-shoulder2.cub", database, true, false);
    m_sprites[SpriteIronShoulder3]->load(path + "iron-shoulder3.cub", database, true, false);
    m_sprites[SpriteIronShoulder4]->load(path + "iron-shoulder4.cub", database, true, false);
    m_sprites[SpriteIronShoulder5]->load(path + "iron-shoulder5.cub", database, true, false);
    m_sprites[SpriteIronShoulder1Random1]->load(path + "iron-shoulder1-random1.cub", database, true, false);
    m_sprites[SpriteIronShoulder2Random1]->load(path + "iron-shoulder2-random1.cub", database, true, false);
    m_sprites[SpriteIronShoulder3Random1]->load(path + "iron-shoulder3-random1.cub", database, true, false);
    m_sprites[SpriteIronShoulder4Random1]->load(path + "iron-shoulder4-random1.cub", database, true, false);
    m_sprites[SpriteIronShoulder5Random1]->load(path + "iron-shoulder5-random1.cub", database, true, false);
    m_sprites[SpriteIronShoulder1Random2]->load(path + "iron-shoulder1-random2.cub", database, true, false);
    m_sprites[SpriteIronShoulder2Random2]->load(path + "iron-shoulder2-random2.cub", database, true, false);
    m_sprites[SpriteIronShoulder3Random2]->load(path + "iron-shoulder3-random2.cub", database, true, false);
    m_sprites[SpriteIronShoulder4Random2]->load(path + "iron-shoulder4-random2.cub", database, true, false);
    m_sprites[SpriteIronShoulder5Random2]->load(path + "iron-shoulder5-random2.cub", database, true, false);
    m_sprites[SpriteIronShoulder1Random3]->load(path + "iron-shoulder1-random3.cub", database, true, false);
    m_sprites[SpriteIronShoulder2Random3]->load(path + "iron-shoulder2-random3.cub", database, true, false);
    m_sprites[SpriteIronShoulder3Random3]->load(path + "iron-shoulder3-random3.cub", database, true, false);
    m_sprites[SpriteIronShoulder4Random3]->load(path + "iron-shoulder4-random3.cub", database, true, false);
    m_sprites[SpriteIronShoulder5Random3]->load(path + "iron-shoulder5-random3.cub", database, true, false);
    m_sprites[SpriteIronShoulder1Random4]->load(path + "iron-shoulder1-random4.cub", database, true, false);
    m_sprites[SpriteIronShoulder2Random4]->load(path + "iron-shoulder2-random4.cub", database, true, false);
    m_sprites[SpriteIronShoulder3Random4]->load(path + "iron-shoulder3-random4.cub", database, true, false);
    m_sprites[SpriteIronShoulder4Random4]->load(path + "iron-shoulder4-random4.cub", database, true, false);
    m_sprites[SpriteIronShoulder5Random4]->load(path + "iron-shoulder5-random4.cub", database, true, false);
    m_sprites[SpriteIronHand1]->load(path + "iron-hand1.cub", database, true, false);
    m_sprites[SpriteIronHand2]->load(path + "iron-hand2.cub", database, true, false);
    m_sprites[SpriteIronHand3]->load(path + "iron-hand3.cub", database, true, false);
    m_sprites[SpriteIronHand4]->load(path + "iron-hand4.cub", database, true, false);
    m_sprites[SpriteIronHand5]->load(path + "iron-hand5.cub", database, true, false);
    m_sprites[SpriteIronHand1Random1]->load(path + "iron-hand1-random1.cub", database, true, false);
    m_sprites[SpriteIronHand2Random1]->load(path + "iron-hand2-random1.cub", database, true, false);
    m_sprites[SpriteIronHand3Random1]->load(path + "iron-hand3-random1.cub", database, true, false);
    m_sprites[SpriteIronHand4Random1]->load(path + "iron-hand4-random1.cub", database, true, false);
    m_sprites[SpriteIronHand5Random1]->load(path + "iron-hand5-random1.cub", database, true, false);
    m_sprites[SpriteIronHand1Random2]->load(path + "iron-hand1-random2.cub", database, true, false);
    m_sprites[SpriteIronHand2Random2]->load(path + "iron-hand2-random2.cub", database, true, false);
    m_sprites[SpriteIronHand3Random2]->load(path + "iron-hand3-random2.cub", database, true, false);
    m_sprites[SpriteIronHand4Random2]->load(path + "iron-hand4-random2.cub", database, true, false);
    m_sprites[SpriteIronHand5Random2]->load(path + "iron-hand5-random2.cub", database, true, false);
    m_sprites[SpriteIronHand1Random3]->load(path + "iron-hand1-random3.cub", database, true, false);
    m_sprites[SpriteIronHand2Random3]->load(path + "iron-hand2-random3.cub", database, true, false);
    m_sprites[SpriteIronHand3Random3]->load(path + "iron-hand3-random3.cub", database, true, false);
    m_sprites[SpriteIronHand4Random3]->load(path + "iron-hand4-random3.cub", database, true, false);
    m_sprites[SpriteIronHand5Random3]->load(path + "iron-hand5-random3.cub", database, true, false);
    m_sprites[SpriteIronHand1Random4]->load(path + "iron-hand1-random4.cub", database, true, false);
    m_sprites[SpriteIronHand2Random4]->load(path + "iron-hand2-random4.cub", database, true, false);
    m_sprites[SpriteIronHand3Random4]->load(path + "iron-hand3-random4.cub", database, true, false);
    m_sprites[SpriteIronHand4Random4]->load(path + "iron-hand4-random4.cub", database, true, false);
    m_sprites[SpriteIronHand5Random4]->load(path + "iron-hand5-random4.cub", database, true, false);
    m_sprites[SpriteIronFoot1]->load(path + "iron-foot1.cub", database, true, false);
    m_sprites[SpriteIronFoot2]->load(path + "iron-foot2.cub", database, true, false);
    m_sprites[SpriteIronFoot3]->load(path + "iron-foot3.cub", database, true, false);
    m_sprites[SpriteIronFoot4]->load(path + "iron-foot4.cub", database, true, false);
    m_sprites[SpriteIronFoot5]->load(path + "iron-foot5.cub", database, true, false);
    m_sprites[SpriteIronFoot1Random1]->load(path + "iron-foot1-random1.cub", database, true, false);
    m_sprites[SpriteIronFoot2Random1]->load(path + "iron-foot2-random1.cub", database, true, false);
    m_sprites[SpriteIronFoot3Random1]->load(path + "iron-foot3-random1.cub", database, true, false);
    m_sprites[SpriteIronFoot4Random1]->load(path + "iron-foot4-random1.cub", database, true, false);
    m_sprites[SpriteIronFoot5Random1]->load(path + "iron-foot5-random1.cub", database, true, false);
    m_sprites[SpriteIronFoot1Random2]->load(path + "iron-foot1-random2.cub", database, true, false);
    m_sprites[SpriteIronFoot2Random2]->load(path + "iron-foot2-random2.cub", database, true, false);
    m_sprites[SpriteIronFoot3Random2]->load(path + "iron-foot3-random2.cub", database, true, false);
    m_sprites[SpriteIronFoot4Random2]->load(path + "iron-foot4-random2.cub", database, true, false);
    m_sprites[SpriteIronFoot5Random2]->load(path + "iron-foot5-random2.cub", database, true, false);
    m_sprites[SpriteIronFoot1Random3]->load(path + "iron-foot1-random3.cub", database, true, false);
    m_sprites[SpriteIronFoot2Random3]->load(path + "iron-foot2-random3.cub", database, true, false);
    m_sprites[SpriteIronFoot3Random3]->load(path + "iron-foot3-random3.cub", database, true, false);
    m_sprites[SpriteIronFoot4Random3]->load(path + "iron-foot4-random3.cub", database, true, false);
    m_sprites[SpriteIronFoot5Random3]->load(path + "iron-foot5-random3.cub", database, true, false);
    m_sprites[SpriteIronFoot1Random4]->load(path + "iron-foot1-random4.cub", database, true, false);
    m_sprites[SpriteIronFoot2Random4]->load(path + "iron-foot2-random4.cub", database, true, false);
    m_sprites[SpriteIronFoot3Random4]->load(path + "iron-foot3-random4.cub", database, true, false);
    m_sprites[SpriteIronFoot4Random4]->load(path + "iron-foot4-random4.cub", database, true, false);
    m_sprites[SpriteIronFoot5Random4]->load(path + "iron-foot5-random4.cub", database, true, false);
    m_sprites[SpriteLinenChest1]->load(path + "linen-chest1.cub", database, true, false);
    m_sprites[SpriteLinenChest2]->load(path + "linen-chest2.cub", database, true, false);
    m_sprites[SpriteLinenChest3]->load(path + "linen-chest3.cub", database, true, false);
    m_sprites[SpriteLinenChest4]->load(path + "linen-chest4.cub", database, true, false);
    m_sprites[SpriteLinenChest5]->load(path + "linen-chest5.cub", database, true, false);
    m_sprites[SpriteLinenChest1Random1]->load(path + "linen-chest1-random1.cub", database, true, false);
    m_sprites[SpriteLinenChest2Random1]->load(path + "linen-chest2-random1.cub", database, true, false);
    m_sprites[SpriteLinenChest3Random1]->load(path + "linen-chest3-random1.cub", database, true, false);
    m_sprites[SpriteLinenChest4Random1]->load(path + "linen-chest4-random1.cub", database, true, false);
    m_sprites[SpriteLinenChest5Random1]->load(path + "linen-chest5-random1.cub", database, true, false);
    m_sprites[SpriteLinenChest1Random2]->load(path + "linen-chest1-random2.cub", database, true, false);
    m_sprites[SpriteLinenChest2Random2]->load(path + "linen-chest2-random2.cub", database, true, false);
    m_sprites[SpriteLinenChest3Random2]->load(path + "linen-chest3-random2.cub", database, true, false);
    m_sprites[SpriteLinenChest4Random2]->load(path + "linen-chest4-random2.cub", database, true, false);
    m_sprites[SpriteLinenChest5Random2]->load(path + "linen-chest5-random2.cub", database, true, false);
    m_sprites[SpriteLinenChest1Random3]->load(path + "linen-chest1-random3.cub", database, true, false);
    m_sprites[SpriteLinenChest2Random3]->load(path + "linen-chest2-random3.cub", database, true, false);
    m_sprites[SpriteLinenChest3Random3]->load(path + "linen-chest3-random3.cub", database, true, false);
    m_sprites[SpriteLinenChest4Random3]->load(path + "linen-chest4-random3.cub", database, true, false);
    m_sprites[SpriteLinenChest5Random3]->load(path + "linen-chest5-random3.cub", database, true, false);
    m_sprites[SpriteLinenChest1Random4]->load(path + "linen-chest1-random4.cub", database, true, false);
    m_sprites[SpriteLinenChest2Random4]->load(path + "linen-chest2-random4.cub", database, true, false);
    m_sprites[SpriteLinenChest3Random4]->load(path + "linen-chest3-random4.cub", database, true, false);
    m_sprites[SpriteLinenChest4Random4]->load(path + "linen-chest4-random4.cub", database, true, false);
    m_sprites[SpriteLinenChest5Random4]->load(path + "linen-chest5-random4.cub", database, true, false);
    m_sprites[SpriteLinenShoulder1]->load(path + "linen-shoulder1.cub", database, true, false);
    m_sprites[SpriteLinenShoulder2]->load(path + "linen-shoulder2.cub", database, true, false);
    m_sprites[SpriteLinenShoulder3]->load(path + "linen-shoulder3.cub", database, true, false);
    m_sprites[SpriteLinenShoulder4]->load(path + "linen-shoulder4.cub", database, true, false);
    m_sprites[SpriteLinenShoulder5]->load(path + "linen-shoulder5.cub", database, true, false);
    m_sprites[SpriteLinenShoulder1Random1]->load(path + "linen-shoulder1-random1.cub", database, true, false);
    m_sprites[SpriteLinenShoulder2Random1]->load(path + "linen-shoulder2-random1.cub", database, true, false);
    m_sprites[SpriteLinenShoulder3Random1]->load(path + "linen-shoulder3-random1.cub", database, true, false);
    m_sprites[SpriteLinenShoulder4Random1]->load(path + "linen-shoulder4-random1.cub", database, true, false);
    m_sprites[SpriteLinenShoulder5Random1]->load(path + "linen-shoulder5-random1.cub", database, true, false);
    m_sprites[SpriteLinenShoulder1Random2]->load(path + "linen-shoulder1-random2.cub", database, true, false);
    m_sprites[SpriteLinenShoulder2Random2]->load(path + "linen-shoulder2-random2.cub", database, true, false);
    m_sprites[SpriteLinenShoulder3Random2]->load(path + "linen-shoulder3-random2.cub", database, true, false);
    m_sprites[SpriteLinenShoulder4Random2]->load(path + "linen-shoulder4-random2.cub", database, true, false);
    m_sprites[SpriteLinenShoulder5Random2]->load(path + "linen-shoulder5-random2.cub", database, true, false);
    m_sprites[SpriteLinenShoulder1Random3]->load(path + "linen-shoulder1-random3.cub", database, true, false);
    m_sprites[SpriteLinenShoulder2Random3]->load(path + "linen-shoulder2-random3.cub", database, true, false);
    m_sprites[SpriteLinenShoulder3Random3]->load(path + "linen-shoulder3-random3.cub", database, true, false);
    m_sprites[SpriteLinenShoulder4Random3]->load(path + "linen-shoulder4-random3.cub", database, true, false);
    m_sprites[SpriteLinenShoulder5Random3]->load(path + "linen-shoulder5-random3.cub", database, true, false);
    m_sprites[SpriteLinenShoulder1Random4]->load(path + "linen-shoulder1-random4.cub", database, true, false);
    m_sprites[SpriteLinenShoulder2Random4]->load(path + "linen-shoulder2-random4.cub", database, true, false);
    m_sprites[SpriteLinenShoulder3Random4]->load(path + "linen-shoulder3-random4.cub", database, true, false);
    m_sprites[SpriteLinenShoulder4Random4]->load(path + "linen-shoulder4-random4.cub", database, true, false);
    m_sprites[SpriteLinenShoulder5Random4]->load(path + "linen-shoulder5-random4.cub", database, true, false);
    m_sprites[SpriteLinenHand1]->load(path + "linen-hand1.cub", database, true, false);
    m_sprites[SpriteLinenHand2]->load(path + "linen-hand2.cub", database, true, false);
    m_sprites[SpriteLinenHand3]->load(path + "linen-hand3.cub", database, true, false);
    m_sprites[SpriteLinenHand4]->load(path + "linen-hand4.cub", database, true, false);
    m_sprites[SpriteLinenHand5]->load(path + "linen-hand5.cub", database, true, false);
    m_sprites[SpriteLinenHand1Random1]->load(path + "linen-hand1-random1.cub", database, true, false);
    m_sprites[SpriteLinenHand2Random1]->load(path + "linen-hand2-random1.cub", database, true, false);
    m_sprites[SpriteLinenHand3Random1]->load(path + "linen-hand3-random1.cub", database, true, false);
    m_sprites[SpriteLinenHand4Random1]->load(path + "linen-hand4-random1.cub", database, true, false);
    m_sprites[SpriteLinenHand5Random1]->load(path + "linen-hand5-random1.cub", database, true, false);
    m_sprites[SpriteLinenHand1Random2]->load(path + "linen-hand1-random2.cub", database, true, false);
    m_sprites[SpriteLinenHand2Random2]->load(path + "linen-hand2-random2.cub", database, true, false);
    m_sprites[SpriteLinenHand3Random2]->load(path + "linen-hand3-random2.cub", database, true, false);
    m_sprites[SpriteLinenHand4Random2]->load(path + "linen-hand4-random2.cub", database, true, false);
    m_sprites[SpriteLinenHand5Random2]->load(path + "linen-hand5-random2.cub", database, true, false);
    m_sprites[SpriteLinenHand1Random3]->load(path + "linen-hand1-random3.cub", database, true, false);
    m_sprites[SpriteLinenHand2Random3]->load(path + "linen-hand2-random3.cub", database, true, false);
    m_sprites[SpriteLinenHand3Random3]->load(path + "linen-hand3-random3.cub", database, true, false);
    m_sprites[SpriteLinenHand4Random3]->load(path + "linen-hand4-random3.cub", database, true, false);
    m_sprites[SpriteLinenHand5Random3]->load(path + "linen-hand5-random3.cub", database, true, false);
    m_sprites[SpriteLinenHand1Random4]->load(path + "linen-hand1-random4.cub", database, true, false);
    m_sprites[SpriteLinenHand2Random4]->load(path + "linen-hand2-random4.cub", database, true, false);
    m_sprites[SpriteLinenHand3Random4]->load(path + "linen-hand3-random4.cub", database, true, false);
    m_sprites[SpriteLinenHand4Random4]->load(path + "linen-hand4-random4.cub", database, true, false);
    m_sprites[SpriteLinenHand5Random4]->load(path + "linen-hand5-random4.cub", database, true, false);
    m_sprites[SpriteLinenFoot1]->load(path + "linen-foot1.cub", database, true, false);
    m_sprites[SpriteLinenFoot2]->load(path + "linen-foot2.cub", database, true, false);
    m_sprites[SpriteLinenFoot3]->load(path + "linen-foot3.cub", database, true, false);
    m_sprites[SpriteLinenFoot4]->load(path + "linen-foot4.cub", database, true, false);
    m_sprites[SpriteLinenFoot5]->load(path + "linen-foot5.cub", database, true, false);
    m_sprites[SpriteLinenFoot1Random1]->load(path + "linen-foot1-random1.cub", database, true, false);
    m_sprites[SpriteLinenFoot2Random1]->load(path + "linen-foot2-random1.cub", database, true, false);
    m_sprites[SpriteLinenFoot3Random1]->load(path + "linen-foot3-random1.cub", database, true, false);
    m_sprites[SpriteLinenFoot4Random1]->load(path + "linen-foot4-random1.cub", database, true, false);
    m_sprites[SpriteLinenFoot5Random1]->load(path + "linen-foot5-random1.cub", database, true, false);
    m_sprites[SpriteLinenFoot1Random2]->load(path + "linen-foot1-random2.cub", database, true, false);
    m_sprites[SpriteLinenFoot2Random2]->load(path + "linen-foot2-random2.cub", database, true, false);
    m_sprites[SpriteLinenFoot3Random2]->load(path + "linen-foot3-random2.cub", database, true, false);
    m_sprites[SpriteLinenFoot4Random2]->load(path + "linen-foot4-random2.cub", database, true, false);
    m_sprites[SpriteLinenFoot5Random2]->load(path + "linen-foot5-random2.cub", database, true, false);
    m_sprites[SpriteLinenFoot1Random3]->load(path + "linen-foot1-random3.cub", database, true, false);
    m_sprites[SpriteLinenFoot2Random3]->load(path + "linen-foot2-random3.cub", database, true, false);
    m_sprites[SpriteLinenFoot3Random3]->load(path + "linen-foot3-random3.cub", database, true, false);
    m_sprites[SpriteLinenFoot4Random3]->load(path + "linen-foot4-random3.cub", database, true, false);
    m_sprites[SpriteLinenFoot5Random3]->load(path + "linen-foot5-random3.cub", database, true, false);
    m_sprites[SpriteLinenFoot1Random4]->load(path + "linen-foot1-random4.cub", database, true, false);
    m_sprites[SpriteLinenFoot2Random4]->load(path + "linen-foot2-random4.cub", database, true, false);
    m_sprites[SpriteLinenFoot3Random4]->load(path + "linen-foot3-random4.cub", database, true, false);
    m_sprites[SpriteLinenFoot4Random4]->load(path + "linen-foot4-random4.cub", database, true, false);
    m_sprites[SpriteLinenFoot5Random4]->load(path + "linen-foot5-random4.cub", database, true, false);
    m_sprites[SpriteCottonChest1]->load(path + "cotton-chest1.cub", database, true, false);
    m_sprites[SpriteCottonChest2]->load(path + "cotton-chest2.cub", database, true, false);
    m_sprites[SpriteCottonChest3]->load(path + "cotton-chest3.cub", database, true, false);
    m_sprites[SpriteCottonChest4]->load(path + "cotton-chest4.cub", database, true, false);
    m_sprites[SpriteCottonChest5]->load(path + "cotton-chest5.cub", database, true, false);
    m_sprites[SpriteCottonChest1Random1]->load(path + "cotton-chest1-random1.cub", database, true, false);
    m_sprites[SpriteCottonChest2Random1]->load(path + "cotton-chest2-random1.cub", database, true, false);
    m_sprites[SpriteCottonChest3Random1]->load(path + "cotton-chest3-random1.cub", database, true, false);
    m_sprites[SpriteCottonChest4Random1]->load(path + "cotton-chest4-random1.cub", database, true, false);
    m_sprites[SpriteCottonChest5Random1]->load(path + "cotton-chest5-random1.cub", database, true, false);
    m_sprites[SpriteCottonChest1Random2]->load(path + "cotton-chest1-random2.cub", database, true, false);
    m_sprites[SpriteCottonChest2Random2]->load(path + "cotton-chest2-random2.cub", database, true, false);
    m_sprites[SpriteCottonChest3Random2]->load(path + "cotton-chest3-random2.cub", database, true, false);
    m_sprites[SpriteCottonChest4Random2]->load(path + "cotton-chest4-random2.cub", database, true, false);
    m_sprites[SpriteCottonChest5Random2]->load(path + "cotton-chest5-random2.cub", database, true, false);
    m_sprites[SpriteCottonChest1Random3]->load(path + "cotton-chest1-random3.cub", database, true, false);
    m_sprites[SpriteCottonChest2Random3]->load(path + "cotton-chest2-random3.cub", database, true, false);
    m_sprites[SpriteCottonChest3Random3]->load(path + "cotton-chest3-random3.cub", database, true, false);
    m_sprites[SpriteCottonChest4Random3]->load(path + "cotton-chest4-random3.cub", database, true, false);
    m_sprites[SpriteCottonChest5Random3]->load(path + "cotton-chest5-random3.cub", database, true, false);
    m_sprites[SpriteCottonChest1Random4]->load(path + "cotton-chest1-random4.cub", database, true, false);
    m_sprites[SpriteCottonChest2Random4]->load(path + "cotton-chest2-random4.cub", database, true, false);
    m_sprites[SpriteCottonChest3Random4]->load(path + "cotton-chest3-random4.cub", database, true, false);
    m_sprites[SpriteCottonChest4Random4]->load(path + "cotton-chest4-random4.cub", database, true, false);
    m_sprites[SpriteCottonChest5Random4]->load(path + "cotton-chest5-random4.cub", database, true, false);
    m_sprites[SpriteCottonShoulder1]->load(path + "cotton-shoulder1.cub", database, true, false);
    m_sprites[SpriteCottonShoulder2]->load(path + "cotton-shoulder2.cub", database, true, false);
    m_sprites[SpriteCottonShoulder3]->load(path + "cotton-shoulder3.cub", database, true, false);
    m_sprites[SpriteCottonShoulder4]->load(path + "cotton-shoulder4.cub", database, true, false);
    m_sprites[SpriteCottonShoulder5]->load(path + "cotton-shoulder5.cub", database, true, false);
    m_sprites[SpriteCottonShoulder1Random1]->load(path + "cotton-shoulder1-random1.cub", database, true, false);
    m_sprites[SpriteCottonShoulder2Random1]->load(path + "cotton-shoulder2-random1.cub", database, true, false);
    m_sprites[SpriteCottonShoulder3Random1]->load(path + "cotton-shoulder3-random1.cub", database, true, false);
    m_sprites[SpriteCottonShoulder4Random1]->load(path + "cotton-shoulder4-random1.cub", database, true, false);
    m_sprites[SpriteCottonShoulder5Random1]->load(path + "cotton-shoulder5-random1.cub", database, true, false);
    m_sprites[SpriteCottonShoulder1Random2]->load(path + "cotton-shoulder1-random2.cub", database, true, false);
    m_sprites[SpriteCottonShoulder2Random2]->load(path + "cotton-shoulder2-random2.cub", database, true, false);
    m_sprites[SpriteCottonShoulder3Random2]->load(path + "cotton-shoulder3-random2.cub", database, true, false);
    m_sprites[SpriteCottonShoulder4Random2]->load(path + "cotton-shoulder4-random2.cub", database, true, false);
    m_sprites[SpriteCottonShoulder5Random2]->load(path + "cotton-shoulder5-random2.cub", database, true, false);
    m_sprites[SpriteCottonShoulder1Random3]->load(path + "cotton-shoulder1-random3.cub", database, true, false);
    m_sprites[SpriteCottonShoulder2Random3]->load(path + "cotton-shoulder2-random3.cub", database, true, false);
    m_sprites[SpriteCottonShoulder3Random3]->load(path + "cotton-shoulder3-random3.cub", database, true, false);
    m_sprites[SpriteCottonShoulder4Random3]->load(path + "cotton-shoulder4-random3.cub", database, true, false);
    m_sprites[SpriteCottonShoulder5Random3]->load(path + "cotton-shoulder5-random3.cub", database, true, false);
    m_sprites[SpriteCottonShoulder1Random4]->load(path + "cotton-shoulder1-random4.cub", database, true, false);
    m_sprites[SpriteCottonShoulder2Random4]->load(path + "cotton-shoulder2-random4.cub", database, true, false);
    m_sprites[SpriteCottonShoulder3Random4]->load(path + "cotton-shoulder3-random4.cub", database, true, false);
    m_sprites[SpriteCottonShoulder4Random4]->load(path + "cotton-shoulder4-random4.cub", database, true, false);
    m_sprites[SpriteCottonShoulder5Random4]->load(path + "cotton-shoulder5-random4.cub", database, true, false);
    m_sprites[SpriteCottonHand1]->load(path + "cotton-hand1.cub", database, true, false);
    m_sprites[SpriteCottonHand2]->load(path + "cotton-hand2.cub", database, true, false);
    m_sprites[SpriteCottonHand3]->load(path + "cotton-hand3.cub", database, true, false);
    m_sprites[SpriteCottonHand4]->load(path + "cotton-hand4.cub", database, true, false);
    m_sprites[SpriteCottonHand5]->load(path + "cotton-hand5.cub", database, true, false);
    m_sprites[SpriteCottonHand1Random1]->load(path + "cotton-hand1-random1.cub", database, true, false);
    m_sprites[SpriteCottonHand2Random1]->load(path + "cotton-hand2-random1.cub", database, true, false);
    m_sprites[SpriteCottonHand3Random1]->load(path + "cotton-hand3-random1.cub", database, true, false);
    m_sprites[SpriteCottonHand4Random1]->load(path + "cotton-hand4-random1.cub", database, true, false);
    m_sprites[SpriteCottonHand5Random1]->load(path + "cotton-hand5-random1.cub", database, true, false);
    m_sprites[SpriteCottonHand1Random2]->load(path + "cotton-hand1-random2.cub", database, true, false);
    m_sprites[SpriteCottonHand2Random2]->load(path + "cotton-hand2-random2.cub", database, true, false);
    m_sprites[SpriteCottonHand3Random2]->load(path + "cotton-hand3-random2.cub", database, true, false);
    m_sprites[SpriteCottonHand4Random2]->load(path + "cotton-hand4-random2.cub", database, true, false);
    m_sprites[SpriteCottonHand5Random2]->load(path + "cotton-hand5-random2.cub", database, true, false);
    m_sprites[SpriteCottonHand1Random3]->load(path + "cotton-hand1-random3.cub", database, true, false);
    m_sprites[SpriteCottonHand2Random3]->load(path + "cotton-hand2-random3.cub", database, true, false);
    m_sprites[SpriteCottonHand3Random3]->load(path + "cotton-hand3-random3.cub", database, true, false);
    m_sprites[SpriteCottonHand4Random3]->load(path + "cotton-hand4-random3.cub", database, true, false);
    m_sprites[SpriteCottonHand5Random3]->load(path + "cotton-hand5-random3.cub", database, true, false);
    m_sprites[SpriteCottonHand1Random4]->load(path + "cotton-hand1-random4.cub", database, true, false);
    m_sprites[SpriteCottonHand2Random4]->load(path + "cotton-hand2-random4.cub", database, true, false);
    m_sprites[SpriteCottonHand3Random4]->load(path + "cotton-hand3-random4.cub", database, true, false);
    m_sprites[SpriteCottonHand4Random4]->load(path + "cotton-hand4-random4.cub", database, true, false);
    m_sprites[SpriteCottonHand5Random4]->load(path + "cotton-hand5-random4.cub", database, true, false);
    m_sprites[SpriteCottonFoot1]->load(path + "cotton-foot1.cub", database, true, false);
    m_sprites[SpriteCottonFoot2]->load(path + "cotton-foot2.cub", database, true, false);
    m_sprites[SpriteCottonFoot3]->load(path + "cotton-foot3.cub", database, true, false);
    m_sprites[SpriteCottonFoot4]->load(path + "cotton-foot4.cub", database, true, false);
    m_sprites[SpriteCottonFoot5]->load(path + "cotton-foot5.cub", database, true, false);
    m_sprites[SpriteCottonFoot1Random1]->load(path + "cotton-foot1-random1.cub", database, true, false);
    m_sprites[SpriteCottonFoot2Random1]->load(path + "cotton-foot2-random1.cub", database, true, false);
    m_sprites[SpriteCottonFoot3Random1]->load(path + "cotton-foot3-random1.cub", database, true, false);
    m_sprites[SpriteCottonFoot4Random1]->load(path + "cotton-foot4-random1.cub", database, true, false);
    m_sprites[SpriteCottonFoot5Random1]->load(path + "cotton-foot5-random1.cub", database, true, false);
    m_sprites[SpriteCottonFoot1Random2]->load(path + "cotton-foot1-random2.cub", database, true, false);
    m_sprites[SpriteCottonFoot2Random2]->load(path + "cotton-foot2-random2.cub", database, true, false);
    m_sprites[SpriteCottonFoot3Random2]->load(path + "cotton-foot3-random2.cub", database, true, false);
    m_sprites[SpriteCottonFoot4Random2]->load(path + "cotton-foot4-random2.cub", database, true, false);
    m_sprites[SpriteCottonFoot5Random2]->load(path + "cotton-foot5-random2.cub", database, true, false);
    m_sprites[SpriteCottonFoot1Random3]->load(path + "cotton-foot1-random3.cub", database, true, false);
    m_sprites[SpriteCottonFoot2Random3]->load(path + "cotton-foot2-random3.cub", database, true, false);
    m_sprites[SpriteCottonFoot3Random3]->load(path + "cotton-foot3-random3.cub", database, true, false);
    m_sprites[SpriteCottonFoot4Random3]->load(path + "cotton-foot4-random3.cub", database, true, false);
    m_sprites[SpriteCottonFoot5Random3]->load(path + "cotton-foot5-random3.cub", database, true, false);
    m_sprites[SpriteCottonFoot1Random4]->load(path + "cotton-foot1-random4.cub", database, true, false);
    m_sprites[SpriteCottonFoot2Random4]->load(path + "cotton-foot2-random4.cub", database, true, false);
    m_sprites[SpriteCottonFoot3Random4]->load(path + "cotton-foot3-random4.cub", database, true, false);
    m_sprites[SpriteCottonFoot4Random4]->load(path + "cotton-foot4-random4.cub", database, true, false);
    m_sprites[SpriteCottonFoot5Random4]->load(path + "cotton-foot5-random4.cub", database, true, false);
    m_sprites[SpriteIronShield01]->load(path + "iron-shield01.cub", database, true, false);
    m_sprites[SpriteIronShield02]->load(path + "iron-shield02.cub", database, true, false);
    m_sprites[SpriteIronShield03]->load(path + "iron-shield03.cub", database, true, false);
    m_sprites[SpriteIronShield04]->load(path + "iron-shield04.cub", database, true, false);
    m_sprites[SpriteIronShield05]->load(path + "iron-shield05.cub", database, true, false);
    m_sprites[SpriteIronShield1Random1]->load(path + "iron-shield1-random1.cub", database, true, false);
    m_sprites[SpriteIronShield2Random1]->load(path + "iron-shield2-random1.cub", database, true, false);
    m_sprites[SpriteIronShield3Random1]->load(path + "iron-shield3-random1.cub", database, true, false);
    m_sprites[SpriteIronShield4Random1]->load(path + "iron-shield4-random1.cub", database, true, false);
    m_sprites[SpriteIronShield5Random1]->load(path + "iron-shield5-random1.cub", database, true, false);
    m_sprites[SpriteIronShield1Random2]->load(path + "iron-shield1-random2.cub", database, true, false);
    m_sprites[SpriteIronShield2Random2]->load(path + "iron-shield2-random2.cub", database, true, false);
    m_sprites[SpriteIronShield3Random2]->load(path + "iron-shield3-random2.cub", database, true, false);
    m_sprites[SpriteIronShield4Random2]->load(path + "iron-shield4-random2.cub", database, true, false);
    m_sprites[SpriteIronShield5Random2]->load(path + "iron-shield5-random2.cub", database, true, false);
    m_sprites[SpriteIronShield1Random3]->load(path + "iron-shield1-random3.cub", database, true, false);
    m_sprites[SpriteIronShield2Random3]->load(path + "iron-shield2-random3.cub", database, true, false);
    m_sprites[SpriteIronShield3Random3]->load(path + "iron-shield3-random3.cub", database, true, false);
    m_sprites[SpriteIronShield4Random3]->load(path + "iron-shield4-random3.cub", database, true, false);
    m_sprites[SpriteIronShield5Random3]->load(path + "iron-shield5-random3.cub", database, true, false);
    m_sprites[SpriteIronShield1Random4]->load(path + "iron-shield1-random4.cub", database, true, false);
    m_sprites[SpriteIronShield2Random4]->load(path + "iron-shield2-random4.cub", database, true, false);
    m_sprites[SpriteIronShield3Random4]->load(path + "iron-shield3-random4.cub", database, true, false);
    m_sprites[SpriteIronShield4Random4]->load(path + "iron-shield4-random4.cub", database, true, false);
    m_sprites[SpriteIronShield5Random4]->load(path + "iron-shield5-random4.cub", database, true, false);
    m_sprites[SpriteIronShield1Random5]->load(path + "iron-shield1-random5.cub", database, true, false);
    m_sprites[SpriteIronShield2Random5]->load(path + "iron-shield2-random5.cub", database, true, false);
    m_sprites[SpriteIronShield3Random5]->load(path + "iron-shield3-random5.cub", database, true, false);
    m_sprites[SpriteIronShield4Random5]->load(path + "iron-shield4-random5.cub", database, true, false);
    m_sprites[SpriteIronShield5Random5]->load(path + "iron-shield5-random5.cub", database, true, false);
    m_sprites[SpriteIronShield1Random6]->load(path + "iron-shield1-random6.cub", database, true, false);
    m_sprites[SpriteIronShield2Random6]->load(path + "iron-shield2-random6.cub", database, true, false);
    m_sprites[SpriteIronShield3Random6]->load(path + "iron-shield3-random6.cub", database, true, false);
    m_sprites[SpriteIronShield4Random6]->load(path + "iron-shield4-random6.cub", database, true, false);
    m_sprites[SpriteIronShield5Random6]->load(path + "iron-shield5-random6.cub", database, true, false);
    m_sprites[SpriteIronShield1Random7]->load(path + "iron-shield1-random7.cub", database, true, false);
    m_sprites[SpriteIronShield2Random7]->load(path + "iron-shield2-random7.cub", database, true, false);
    m_sprites[SpriteIronShield3Random7]->load(path + "iron-shield3-random7.cub", database, true, false);
    m_sprites[SpriteIronShield4Random7]->load(path + "iron-shield4-random7.cub", database, true, false);
    m_sprites[SpriteIronShield5Random7]->load(path + "iron-shield5-random7.cub", database, true, false);
    m_sprites[SpriteIronShield1Random8]->load(path + "iron-shield1-random8.cub", database, true, false);
    m_sprites[SpriteIronShield2Random8]->load(path + "iron-shield2-random8.cub", database, true, false);
    m_sprites[SpriteIronShield3Random8]->load(path + "iron-shield3-random8.cub", database, true, false);
    m_sprites[SpriteIronShield4Random8]->load(path + "iron-shield4-random8.cub", database, true, false);
    m_sprites[SpriteIronShield5Random8]->load(path + "iron-shield5-random8.cub", database, true, false);
    m_sprites[SpriteIronShield1Random9]->load(path + "iron-shield1-random9.cub", database, true, false);
    m_sprites[SpriteIronShield2Random9]->load(path + "iron-shield2-random9.cub", database, true, false);
    m_sprites[SpriteIronShield3Random9]->load(path + "iron-shield3-random9.cub", database, true, false);
    m_sprites[SpriteIronShield4Random9]->load(path + "iron-shield4-random9.cub", database, true, false);
    m_sprites[SpriteIronShield5Random9]->load(path + "iron-shield5-random9.cub", database, true, false);
    m_sprites[SpriteIronShield1Random10]->load(path + "iron-shield1-random10.cub", database, true, false);
    m_sprites[SpriteIronShield2Random10]->load(path + "iron-shield2-random10.cub", database, true, false);
    m_sprites[SpriteIronShield3Random10]->load(path + "iron-shield3-random10.cub", database, true, false);
    m_sprites[SpriteIronShield4Random10]->load(path + "iron-shield4-random10.cub", database, true, false);
    m_sprites[SpriteIronShield5Random10]->load(path + "iron-shield5-random10.cub", database, true, false);
    m_sprites[SpriteWoodShield01]->load(path + "wood-shield01.cub", database, true, false);
    m_sprites[SpriteWoodShield02]->load(path + "wood-shield02.cub", database, true, false);
    m_sprites[SpriteWoodShield03]->load(path + "wood-shield03.cub", database, true, false);
    m_sprites[SpriteWoodShield04]->load(path + "wood-shield04.cub", database, true, false);
    m_sprites[SpriteWoodShield05]->load(path + "wood-shield05.cub", database, true, false);
    m_sprites[SpriteIronAxe1]->load(path + "iron-axe1.cub", database, true, false);
    m_sprites[SpriteIronAxe2]->load(path + "iron-axe2.cub", database, true, false);
    m_sprites[SpriteIronAxe3]->load(path + "iron-axe3.cub", database, true, false);
    m_sprites[SpriteIronAxe4]->load(path + "iron-axe4.cub", database, true, false);
    m_sprites[SpriteIronAxe5]->load(path + "iron-axe5.cub", database, true, false);
    m_sprites[SpriteIronAxe1Random1]->load(path + "iron-axe1-random1.cub", database, true, false);
    m_sprites[SpriteIronAxe2Random1]->load(path + "iron-axe2-random1.cub", database, true, false);
    m_sprites[SpriteIronAxe3Random1]->load(path + "iron-axe3-random1.cub", database, true, false);
    m_sprites[SpriteIronAxe4Random1]->load(path + "iron-axe4-random1.cub", database, true, false);
    m_sprites[SpriteIronAxe5Random1]->load(path + "iron-axe5-random1.cub", database, true, false);
    m_sprites[SpriteIronAxe1Random2]->load(path + "iron-axe1-random2.cub", database, true, false);
    m_sprites[SpriteIronAxe2Random2]->load(path + "iron-axe2-random2.cub", database, true, false);
    m_sprites[SpriteIronAxe3Random2]->load(path + "iron-axe3-random2.cub", database, true, false);
    m_sprites[SpriteIronAxe4Random2]->load(path + "iron-axe4-random2.cub", database, true, false);
    m_sprites[SpriteIronAxe5Random2]->load(path + "iron-axe5-random2.cub", database, true, false);
    m_sprites[SpriteIronAxe1Random3]->load(path + "iron-axe1-random3.cub", database, true, false);
    m_sprites[SpriteIronAxe2Random3]->load(path + "iron-axe2-random3.cub", database, true, false);
    m_sprites[SpriteIronAxe3Random3]->load(path + "iron-axe3-random3.cub", database, true, false);
    m_sprites[SpriteIronAxe4Random3]->load(path + "iron-axe4-random3.cub", database, true, false);
    m_sprites[SpriteIronAxe5Random3]->load(path + "iron-axe5-random3.cub", database, true, false);
    m_sprites[SpriteIronAxe1Random4]->load(path + "iron-axe1-random4.cub", database, true, false);
    m_sprites[SpriteIronAxe2Random4]->load(path + "iron-axe2-random4.cub", database, true, false);
    m_sprites[SpriteIronAxe3Random4]->load(path + "iron-axe3-random4.cub", database, true, false);
    m_sprites[SpriteIronAxe4Random4]->load(path + "iron-axe4-random4.cub", database, true, false);
    m_sprites[SpriteIronAxe5Random4]->load(path + "iron-axe5-random4.cub", database, true, false);
    m_sprites[SpriteIronAxe1Random5]->load(path + "iron-axe1-random5.cub", database, true, false);
    m_sprites[SpriteIronAxe2Random5]->load(path + "iron-axe2-random5.cub", database, true, false);
    m_sprites[SpriteIronAxe3Random5]->load(path + "iron-axe3-random5.cub", database, true, false);
    m_sprites[SpriteIronAxe4Random5]->load(path + "iron-axe4-random5.cub", database, true, false);
    m_sprites[SpriteIronAxe5Random5]->load(path + "iron-axe5-random5.cub", database, true, false);
    m_sprites[SpriteIronAxe1Random6]->load(path + "iron-axe1-random6.cub", database, true, false);
    m_sprites[SpriteIronAxe2Random6]->load(path + "iron-axe2-random6.cub", database, true, false);
    m_sprites[SpriteIronAxe3Random6]->load(path + "iron-axe3-random6.cub", database, true, false);
    m_sprites[SpriteIronAxe4Random6]->load(path + "iron-axe4-random6.cub", database, true, false);
    m_sprites[SpriteIronAxe5Random6]->load(path + "iron-axe5-random6.cub", database, true, false);
    m_sprites[SpriteIronAxe1Random7]->load(path + "iron-axe1-random7.cub", database, true, false);
    m_sprites[SpriteIronAxe2Random7]->load(path + "iron-axe2-random7.cub", database, true, false);
    m_sprites[SpriteIronAxe3Random7]->load(path + "iron-axe3-random7.cub", database, true, false);
    m_sprites[SpriteIronAxe4Random7]->load(path + "iron-axe4-random7.cub", database, true, false);
    m_sprites[SpriteIronAxe5Random7]->load(path + "iron-axe5-random7.cub", database, true, false);
    m_sprites[SpriteIronAxe1Random8]->load(path + "iron-axe1-random8.cub", database, true, false);
    m_sprites[SpriteIronAxe2Random8]->load(path + "iron-axe2-random8.cub", database, true, false);
    m_sprites[SpriteIronAxe3Random8]->load(path + "iron-axe3-random8.cub", database, true, false);
    m_sprites[SpriteIronAxe4Random8]->load(path + "iron-axe4-random8.cub", database, true, false);
    m_sprites[SpriteIronAxe5Random8]->load(path + "iron-axe5-random8.cub", database, true, false);
    m_sprites[SpriteIronAxe1Random9]->load(path + "iron-axe1-random9.cub", database, true, false);
    m_sprites[SpriteIronAxe2Random9]->load(path + "iron-axe2-random9.cub", database, true, false);
    m_sprites[SpriteIronAxe3Random9]->load(path + "iron-axe3-random9.cub", database, true, false);
    m_sprites[SpriteIronAxe4Random9]->load(path + "iron-axe4-random9.cub", database, true, false);
    m_sprites[SpriteIronAxe5Random9]->load(path + "iron-axe5-random9.cub", database, true, false);
    m_sprites[SpriteIronAxe1Random10]->load(path + "iron-axe1-random10.cub", database, true, false);
    m_sprites[SpriteIronAxe2Random10]->load(path + "iron-axe2-random10.cub", database, true, false);
    m_sprites[SpriteIronAxe3Random10]->load(path + "iron-axe3-random10.cub", database, true, false);
    m_sprites[SpriteIronAxe4Random10]->load(path + "iron-axe4-random10.cub", database, true, false);
    m_sprites[SpriteIronAxe5Random10]->load(path + "iron-axe5-random10.cub", database, true, false);
    m_sprites[SpriteGoldCoin]->load(path + "gold-coin.cub", database, true, false);
    m_sprites[SpriteCopperCoin]->load(path + "copper-coin.cub", database, true, false);
    m_sprites[SpriteSilverCoin]->load(path + "silver-coin.cub", database, true, false);
    m_sprites[SpriteIronGreatsword1]->load(path + "iron-greatsword1.cub", database, true, false);
    m_sprites[SpriteIronGreatsword2]->load(path + "iron-greatsword2.cub", database, true, false);
    m_sprites[SpriteIronGreatsword3]->load(path + "iron-greatsword3.cub", database, true, false);
    m_sprites[SpriteIronGreatsword4]->load(path + "iron-greatsword4.cub", database, true, false);
    m_sprites[SpriteIronGreatsword5]->load(path + "iron-greatsword5.cub", database, true, false);
    m_sprites[SpriteIronGreatsword1Random1]->load(path + "iron-greatsword1-random1.cub", database, true, false);
    m_sprites[SpriteIronGreatsword2Random1]->load(path + "iron-greatsword2-random1.cub", database, true, false);
    m_sprites[SpriteIronGreatsword3Random1]->load(path + "iron-greatsword3-random1.cub", database, true, false);
    m_sprites[SpriteIronGreatsword4Random1]->load(path + "iron-greatsword4-random1.cub", database, true, false);
    m_sprites[SpriteIronGreatsword5Random1]->load(path + "iron-greatsword5-random1.cub", database, true, false);
    m_sprites[SpriteIronGreatsword1Random2]->load(path + "iron-greatsword1-random2.cub", database, true, false);
    m_sprites[SpriteIronGreatsword2Random2]->load(path + "iron-greatsword2-random2.cub", database, true, false);
    m_sprites[SpriteIronGreatsword3Random2]->load(path + "iron-greatsword3-random2.cub", database, true, false);
    m_sprites[SpriteIronGreatsword4Random2]->load(path + "iron-greatsword4-random2.cub", database, true, false);
    m_sprites[SpriteIronGreatsword5Random2]->load(path + "iron-greatsword5-random2.cub", database, true, false);
    m_sprites[SpriteIronGreatsword1Random3]->load(path + "iron-greatsword1-random3.cub", database, true, false);
    m_sprites[SpriteIronGreatsword2Random3]->load(path + "iron-greatsword2-random3.cub", database, true, false);
    m_sprites[SpriteIronGreatsword3Random3]->load(path + "iron-greatsword3-random3.cub", database, true, false);
    m_sprites[SpriteIronGreatsword4Random3]->load(path + "iron-greatsword4-random3.cub", database, true, false);
    m_sprites[SpriteIronGreatsword5Random3]->load(path + "iron-greatsword5-random3.cub", database, true, false);
    m_sprites[SpriteIronGreatsword1Random4]->load(path + "iron-greatsword1-random4.cub", database, true, false);
    m_sprites[SpriteIronGreatsword2Random4]->load(path + "iron-greatsword2-random4.cub", database, true, false);
    m_sprites[SpriteIronGreatsword3Random4]->load(path + "iron-greatsword3-random4.cub", database, true, false);
    m_sprites[SpriteIronGreatsword4Random4]->load(path + "iron-greatsword4-random4.cub", database, true, false);
    m_sprites[SpriteIronGreatsword5Random4]->load(path + "iron-greatsword5-random4.cub", database, true, false);
    m_sprites[SpriteIronGreatsword1Random5]->load(path + "iron-greatsword1-random5.cub", database, true, false);
    m_sprites[SpriteIronGreatsword2Random5]->load(path + "iron-greatsword2-random5.cub", database, true, false);
    m_sprites[SpriteIronGreatsword3Random5]->load(path + "iron-greatsword3-random5.cub", database, true, false);
    m_sprites[SpriteIronGreatsword4Random5]->load(path + "iron-greatsword4-random5.cub", database, true, false);
    m_sprites[SpriteIronGreatsword5Random5]->load(path + "iron-greatsword5-random5.cub", database, true, false);
    m_sprites[SpriteIronGreatsword1Random6]->load(path + "iron-greatsword1-random6.cub", database, true, false);
    m_sprites[SpriteIronGreatsword2Random6]->load(path + "iron-greatsword2-random6.cub", database, true, false);
    m_sprites[SpriteIronGreatsword3Random6]->load(path + "iron-greatsword3-random6.cub", database, true, false);
    m_sprites[SpriteIronGreatsword4Random6]->load(path + "iron-greatsword4-random6.cub", database, true, false);
    m_sprites[SpriteIronGreatsword5Random6]->load(path + "iron-greatsword5-random6.cub", database, true, false);
    m_sprites[SpriteIronGreatsword1Random7]->load(path + "iron-greatsword1-random7.cub", database, true, false);
    m_sprites[SpriteIronGreatsword2Random7]->load(path + "iron-greatsword2-random7.cub", database, true, false);
    m_sprites[SpriteIronGreatsword3Random7]->load(path + "iron-greatsword3-random7.cub", database, true, false);
    m_sprites[SpriteIronGreatsword4Random7]->load(path + "iron-greatsword4-random7.cub", database, true, false);
    m_sprites[SpriteIronGreatsword5Random7]->load(path + "iron-greatsword5-random7.cub", database, true, false);
    m_sprites[SpriteIronGreatsword1Random8]->load(path + "iron-greatsword1-random8.cub", database, true, false);
    m_sprites[SpriteIronGreatsword2Random8]->load(path + "iron-greatsword2-random8.cub", database, true, false);
    m_sprites[SpriteIronGreatsword3Random8]->load(path + "iron-greatsword3-random8.cub", database, true, false);
    m_sprites[SpriteIronGreatsword4Random8]->load(path + "iron-greatsword4-random8.cub", database, true, false);
    m_sprites[SpriteIronGreatsword5Random8]->load(path + "iron-greatsword5-random8.cub", database, true, false);
    m_sprites[SpriteIronGreatsword1Random9]->load(path + "iron-greatsword1-random9.cub", database, true, false);
    m_sprites[SpriteIronGreatsword2Random9]->load(path + "iron-greatsword2-random9.cub", database, true, false);
    m_sprites[SpriteIronGreatsword3Random9]->load(path + "iron-greatsword3-random9.cub", database, true, false);
    m_sprites[SpriteIronGreatsword4Random9]->load(path + "iron-greatsword4-random9.cub", database, true, false);
    m_sprites[SpriteIronGreatsword5Random9]->load(path + "iron-greatsword5-random9.cub", database, true, false);
    m_sprites[SpriteIronGreatsword1Random10]->load(path + "iron-greatsword1-random10.cub", database, true, false);
    m_sprites[SpriteIronGreatsword2Random10]->load(path + "iron-greatsword2-random10.cub", database, true, false);
    m_sprites[SpriteIronGreatsword3Random10]->load(path + "iron-greatsword3-random10.cub", database, true, false);
    m_sprites[SpriteIronGreatsword4Random10]->load(path + "iron-greatsword4-random10.cub", database, true, false);
    m_sprites[SpriteIronGreatsword5Random10]->load(path + "iron-greatsword5-random10.cub", database, true, false);
    m_sprites[SpriteIronLongsword1]->load(path + "iron-longsword1.cub", database, true, false);
    m_sprites[SpriteIronLongsword2]->load(path + "iron-longsword2.cub", database, true, false);
    m_sprites[SpriteIronLongsword3]->load(path + "iron-longsword3.cub", database, true, false);
    m_sprites[SpriteIronLongsword4]->load(path + "iron-longsword4.cub", database, true, false);
    m_sprites[SpriteIronLongsword5]->load(path + "iron-longsword5.cub", database, true, false);
    m_sprites[SpriteIronLongsword1Random1]->load(path + "iron-longsword1-random1.cub", database, true, false);
    m_sprites[SpriteIronLongsword2Random1]->load(path + "iron-longsword2-random1.cub", database, true, false);
    m_sprites[SpriteIronLongsword3Random1]->load(path + "iron-longsword3-random1.cub", database, true, false);
    m_sprites[SpriteIronLongsword4Random1]->load(path + "iron-longsword4-random1.cub", database, true, false);
    m_sprites[SpriteIronLongsword5Random1]->load(path + "iron-longsword5-random1.cub", database, true, false);
    m_sprites[SpriteIronLongsword1Random2]->load(path + "iron-longsword1-random2.cub", database, true, false);
    m_sprites[SpriteIronLongsword2Random2]->load(path + "iron-longsword2-random2.cub", database, true, false);
    m_sprites[SpriteIronLongsword3Random2]->load(path + "iron-longsword3-random2.cub", database, true, false);
    m_sprites[SpriteIronLongsword4Random2]->load(path + "iron-longsword4-random2.cub", database, true, false);
    m_sprites[SpriteIronLongsword5Random2]->load(path + "iron-longsword5-random2.cub", database, true, false);
    m_sprites[SpriteIronLongsword1Random3]->load(path + "iron-longsword1-random3.cub", database, true, false);
    m_sprites[SpriteIronLongsword2Random3]->load(path + "iron-longsword2-random3.cub", database, true, false);
    m_sprites[SpriteIronLongsword3Random3]->load(path + "iron-longsword3-random3.cub", database, true, false);
    m_sprites[SpriteIronLongsword4Random3]->load(path + "iron-longsword4-random3.cub", database, true, false);
    m_sprites[SpriteIronLongsword5Random3]->load(path + "iron-longsword5-random3.cub", database, true, false);
    m_sprites[SpriteIronLongsword1Random4]->load(path + "iron-longsword1-random4.cub", database, true, false);
    m_sprites[SpriteIronLongsword2Random4]->load(path + "iron-longsword2-random4.cub", database, true, false);
    m_sprites[SpriteIronLongsword3Random4]->load(path + "iron-longsword3-random4.cub", database, true, false);
    m_sprites[SpriteIronLongsword4Random4]->load(path + "iron-longsword4-random4.cub", database, true, false);
    m_sprites[SpriteIronLongsword5Random4]->load(path + "iron-longsword5-random4.cub", database, true, false);
    m_sprites[SpriteIronLongsword1Random5]->load(path + "iron-longsword1-random5.cub", database, true, false);
    m_sprites[SpriteIronLongsword2Random5]->load(path + "iron-longsword2-random5.cub", database, true, false);
    m_sprites[SpriteIronLongsword3Random5]->load(path + "iron-longsword3-random5.cub", database, true, false);
    m_sprites[SpriteIronLongsword4Random5]->load(path + "iron-longsword4-random5.cub", database, true, false);
    m_sprites[SpriteIronLongsword5Random5]->load(path + "iron-longsword5-random5.cub", database, true, false);
    m_sprites[SpriteIronLongsword1Random6]->load(path + "iron-longsword1-random6.cub", database, true, false);
    m_sprites[SpriteIronLongsword2Random6]->load(path + "iron-longsword2-random6.cub", database, true, false);
    m_sprites[SpriteIronLongsword3Random6]->load(path + "iron-longsword3-random6.cub", database, true, false);
    m_sprites[SpriteIronLongsword4Random6]->load(path + "iron-longsword4-random6.cub", database, true, false);
    m_sprites[SpriteIronLongsword5Random6]->load(path + "iron-longsword5-random6.cub", database, true, false);
    m_sprites[SpriteIronLongsword1Random7]->load(path + "iron-longsword1-random7.cub", database, true, false);
    m_sprites[SpriteIronLongsword2Random7]->load(path + "iron-longsword2-random7.cub", database, true, false);
    m_sprites[SpriteIronLongsword3Random7]->load(path + "iron-longsword3-random7.cub", database, true, false);
    m_sprites[SpriteIronLongsword4Random7]->load(path + "iron-longsword4-random7.cub", database, true, false);
    m_sprites[SpriteIronLongsword5Random7]->load(path + "iron-longsword5-random7.cub", database, true, false);
    m_sprites[SpriteIronLongsword1Random8]->load(path + "iron-longsword1-random8.cub", database, true, false);
    m_sprites[SpriteIronLongsword2Random8]->load(path + "iron-longsword2-random8.cub", database, true, false);
    m_sprites[SpriteIronLongsword3Random8]->load(path + "iron-longsword3-random8.cub", database, true, false);
    m_sprites[SpriteIronLongsword4Random8]->load(path + "iron-longsword4-random8.cub", database, true, false);
    m_sprites[SpriteIronLongsword5Random8]->load(path + "iron-longsword5-random8.cub", database, true, false);
    m_sprites[SpriteIronLongsword1Random9]->load(path + "iron-longsword1-random9.cub", database, true, false);
    m_sprites[SpriteIronLongsword2Random9]->load(path + "iron-longsword2-random9.cub", database, true, false);
    m_sprites[SpriteIronLongsword3Random9]->load(path + "iron-longsword3-random9.cub", database, true, false);
    m_sprites[SpriteIronLongsword4Random9]->load(path + "iron-longsword4-random9.cub", database, true, false);
    m_sprites[SpriteIronLongsword5Random9]->load(path + "iron-longsword5-random9.cub", database, true, false);
    m_sprites[SpriteIronLongsword1Random10]->load(path + "iron-longsword1-random10.cub", database, true, false);
    m_sprites[SpriteIronLongsword2Random10]->load(path + "iron-longsword2-random10.cub", database, true, false);
    m_sprites[SpriteIronLongsword3Random10]->load(path + "iron-longsword3-random10.cub", database, true, false);
    m_sprites[SpriteIronLongsword4Random10]->load(path + "iron-longsword4-random10.cub", database, true, false);
    m_sprites[SpriteIronLongsword5Random10]->load(path + "iron-longsword5-random10.cub", database, true, false);
    m_sprites[SpriteBoneGreatsword]->load(path + "bone-greatsword.cub", database, true, false);
    m_sprites[SpriteObsidianGreatsword]->load(path + "obsidian-greatsword.cub", database, true, false);
    m_sprites[SpriteObsidianLongsword]->load(path + "obsidian-longsword.cub", database, true, false);
    m_sprites[SpriteIronGreataxe1]->load(path + "iron-greataxe1.cub", database, true, false);
    m_sprites[SpriteIronGreataxe2]->load(path + "iron-greataxe2.cub", database, true, false);
    m_sprites[SpriteIronGreataxe3]->load(path + "iron-greataxe3.cub", database, true, false);
    m_sprites[SpriteIronGreataxe4]->load(path + "iron-greataxe4.cub", database, true, false);
    m_sprites[SpriteIronGreataxe5]->load(path + "iron-greataxe5.cub", database, true, false);
    m_sprites[SpriteIronGreataxe1Random1]->load(path + "iron-greataxe1-random1.cub", database, true, false);
    m_sprites[SpriteIronGreataxe2Random1]->load(path + "iron-greataxe2-random1.cub", database, true, false);
    m_sprites[SpriteIronGreataxe3Random1]->load(path + "iron-greataxe3-random1.cub", database, true, false);
    m_sprites[SpriteIronGreataxe4Random1]->load(path + "iron-greataxe4-random1.cub", database, true, false);
    m_sprites[SpriteIronGreataxe5Random1]->load(path + "iron-greataxe5-random1.cub", database, true, false);
    m_sprites[SpriteIronGreataxe1Random2]->load(path + "iron-greataxe1-random2.cub", database, true, false);
    m_sprites[SpriteIronGreataxe2Random2]->load(path + "iron-greataxe2-random2.cub", database, true, false);
    m_sprites[SpriteIronGreataxe3Random2]->load(path + "iron-greataxe3-random2.cub", database, true, false);
    m_sprites[SpriteIronGreataxe4Random2]->load(path + "iron-greataxe4-random2.cub", database, true, false);
    m_sprites[SpriteIronGreataxe5Random2]->load(path + "iron-greataxe5-random2.cub", database, true, false);
    m_sprites[SpriteIronGreataxe1Random3]->load(path + "iron-greataxe1-random3.cub", database, true, false);
    m_sprites[SpriteIronGreataxe2Random3]->load(path + "iron-greataxe2-random3.cub", database, true, false);
    m_sprites[SpriteIronGreataxe3Random3]->load(path + "iron-greataxe3-random3.cub", database, true, false);
    m_sprites[SpriteIronGreataxe4Random3]->load(path + "iron-greataxe4-random3.cub", database, true, false);
    m_sprites[SpriteIronGreataxe5Random3]->load(path + "iron-greataxe5-random3.cub", database, true, false);
    m_sprites[SpriteIronGreataxe1Random4]->load(path + "iron-greataxe1-random4.cub", database, true, false);
    m_sprites[SpriteIronGreataxe2Random4]->load(path + "iron-greataxe2-random4.cub", database, true, false);
    m_sprites[SpriteIronGreataxe3Random4]->load(path + "iron-greataxe3-random4.cub", database, true, false);
    m_sprites[SpriteIronGreataxe4Random4]->load(path + "iron-greataxe4-random4.cub", database, true, false);
    m_sprites[SpriteIronGreataxe5Random4]->load(path + "iron-greataxe5-random4.cub", database, true, false);
    m_sprites[SpriteIronGreataxe1Random5]->load(path + "iron-greataxe1-random5.cub", database, true, false);
    m_sprites[SpriteIronGreataxe2Random5]->load(path + "iron-greataxe2-random5.cub", database, true, false);
    m_sprites[SpriteIronGreataxe3Random5]->load(path + "iron-greataxe3-random5.cub", database, true, false);
    m_sprites[SpriteIronGreataxe4Random5]->load(path + "iron-greataxe4-random5.cub", database, true, false);
    m_sprites[SpriteIronGreataxe5Random5]->load(path + "iron-greataxe5-random5.cub", database, true, false);
    m_sprites[SpriteIronGreataxe1Random6]->load(path + "iron-greataxe1-random6.cub", database, true, false);
    m_sprites[SpriteIronGreataxe2Random6]->load(path + "iron-greataxe2-random6.cub", database, true, false);
    m_sprites[SpriteIronGreataxe3Random6]->load(path + "iron-greataxe3-random6.cub", database, true, false);
    m_sprites[SpriteIronGreataxe4Random6]->load(path + "iron-greataxe4-random6.cub", database, true, false);
    m_sprites[SpriteIronGreataxe5Random6]->load(path + "iron-greataxe5-random6.cub", database, true, false);
    m_sprites[SpriteIronGreataxe1Random7]->load(path + "iron-greataxe1-random7.cub", database, true, false);
    m_sprites[SpriteIronGreataxe2Random7]->load(path + "iron-greataxe2-random7.cub", database, true, false);
    m_sprites[SpriteIronGreataxe3Random7]->load(path + "iron-greataxe3-random7.cub", database, true, false);
    m_sprites[SpriteIronGreataxe4Random7]->load(path + "iron-greataxe4-random7.cub", database, true, false);
    m_sprites[SpriteIronGreataxe5Random7]->load(path + "iron-greataxe5-random7.cub", database, true, false);
    m_sprites[SpriteIronGreataxe1Random8]->load(path + "iron-greataxe1-random8.cub", database, true, false);
    m_sprites[SpriteIronGreataxe2Random8]->load(path + "iron-greataxe2-random8.cub", database, true, false);
    m_sprites[SpriteIronGreataxe3Random8]->load(path + "iron-greataxe3-random8.cub", database, true, false);
    m_sprites[SpriteIronGreataxe4Random8]->load(path + "iron-greataxe4-random8.cub", database, true, false);
    m_sprites[SpriteIronGreataxe5Random8]->load(path + "iron-greataxe5-random8.cub", database, true, false);
    m_sprites[SpriteIronGreataxe1Random9]->load(path + "iron-greataxe1-random9.cub", database, true, false);
    m_sprites[SpriteIronGreataxe2Random9]->load(path + "iron-greataxe2-random9.cub", database, true, false);
    m_sprites[SpriteIronGreataxe3Random9]->load(path + "iron-greataxe3-random9.cub", database, true, false);
    m_sprites[SpriteIronGreataxe4Random9]->load(path + "iron-greataxe4-random9.cub", database, true, false);
    m_sprites[SpriteIronGreataxe5Random9]->load(path + "iron-greataxe5-random9.cub", database, true, false);
    m_sprites[SpriteIronGreataxe1Random10]->load(path + "iron-greataxe1-random10.cub", database, true, false);
    m_sprites[SpriteIronGreataxe2Random10]->load(path + "iron-greataxe2-random10.cub", database, true, false);
    m_sprites[SpriteIronGreataxe3Random10]->load(path + "iron-greataxe3-random10.cub", database, true, false);
    m_sprites[SpriteIronGreataxe4Random10]->load(path + "iron-greataxe4-random10.cub", database, true, false);
    m_sprites[SpriteIronGreataxe5Random10]->load(path + "iron-greataxe5-random10.cub", database, true, false);
    m_sprites[SpriteBoneGreataxe]->load(path + "bone-greataxe.cub", database, true, false);
    m_sprites[SpriteObsidianGreataxe]->load(path + "obsidian-greataxe.cub", database, true, false);
    m_sprites[SpriteSaurianGreataxe]->load(path + "saurian-greataxe.cub", database, true, false);
    m_sprites[SpriteIronGreatmace1]->load(path + "iron-greatmace1.cub", database, true, false);
    m_sprites[SpriteIronGreatmace2]->load(path + "iron-greatmace2.cub", database, true, false);
    m_sprites[SpriteIronGreatmace3]->load(path + "iron-greatmace3.cub", database, true, false);
    m_sprites[SpriteIronGreatmace4]->load(path + "iron-greatmace4.cub", database, true, false);
    m_sprites[SpriteIronGreatmace5]->load(path + "iron-greatmace5.cub", database, true, false);
    m_sprites[SpriteIronGreatmace1Random1]->load(path + "iron-greatmace1-random1.cub", database, true, false);
    m_sprites[SpriteIronGreatmace2Random1]->load(path + "iron-greatmace2-random1.cub", database, true, false);
    m_sprites[SpriteIronGreatmace3Random1]->load(path + "iron-greatmace3-random1.cub", database, true, false);
    m_sprites[SpriteIronGreatmace4Random1]->load(path + "iron-greatmace4-random1.cub", database, true, false);
    m_sprites[SpriteIronGreatmace5Random1]->load(path + "iron-greatmace5-random1.cub", database, true, false);
    m_sprites[SpriteIronGreatmace1Random2]->load(path + "iron-greatmace1-random2.cub", database, true, false);
    m_sprites[SpriteIronGreatmace2Random2]->load(path + "iron-greatmace2-random2.cub", database, true, false);
    m_sprites[SpriteIronGreatmace3Random2]->load(path + "iron-greatmace3-random2.cub", database, true, false);
    m_sprites[SpriteIronGreatmace4Random2]->load(path + "iron-greatmace4-random2.cub", database, true, false);
    m_sprites[SpriteIronGreatmace5Random2]->load(path + "iron-greatmace5-random2.cub", database, true, false);
    m_sprites[SpriteIronGreatmace1Random3]->load(path + "iron-greatmace1-random3.cub", database, true, false);
    m_sprites[SpriteIronGreatmace2Random3]->load(path + "iron-greatmace2-random3.cub", database, true, false);
    m_sprites[SpriteIronGreatmace3Random3]->load(path + "iron-greatmace3-random3.cub", database, true, false);
    m_sprites[SpriteIronGreatmace4Random3]->load(path + "iron-greatmace4-random3.cub", database, true, false);
    m_sprites[SpriteIronGreatmace5Random3]->load(path + "iron-greatmace5-random3.cub", database, true, false);
    m_sprites[SpriteIronGreatmace1Random4]->load(path + "iron-greatmace1-random4.cub", database, true, false);
    m_sprites[SpriteIronGreatmace2Random4]->load(path + "iron-greatmace2-random4.cub", database, true, false);
    m_sprites[SpriteIronGreatmace3Random4]->load(path + "iron-greatmace3-random4.cub", database, true, false);
    m_sprites[SpriteIronGreatmace4Random4]->load(path + "iron-greatmace4-random4.cub", database, true, false);
    m_sprites[SpriteIronGreatmace5Random4]->load(path + "iron-greatmace5-random4.cub", database, true, false);
    m_sprites[SpriteIronGreatmace1Random5]->load(path + "iron-greatmace1-random5.cub", database, true, false);
    m_sprites[SpriteIronGreatmace2Random5]->load(path + "iron-greatmace2-random5.cub", database, true, false);
    m_sprites[SpriteIronGreatmace3Random5]->load(path + "iron-greatmace3-random5.cub", database, true, false);
    m_sprites[SpriteIronGreatmace4Random5]->load(path + "iron-greatmace4-random5.cub", database, true, false);
    m_sprites[SpriteIronGreatmace5Random5]->load(path + "iron-greatmace5-random5.cub", database, true, false);
    m_sprites[SpriteIronGreatmace1Random6]->load(path + "iron-greatmace1-random6.cub", database, true, false);
    m_sprites[SpriteIronGreatmace2Random6]->load(path + "iron-greatmace2-random6.cub", database, true, false);
    m_sprites[SpriteIronGreatmace3Random6]->load(path + "iron-greatmace3-random6.cub", database, true, false);
    m_sprites[SpriteIronGreatmace4Random6]->load(path + "iron-greatmace4-random6.cub", database, true, false);
    m_sprites[SpriteIronGreatmace5Random6]->load(path + "iron-greatmace5-random6.cub", database, true, false);
    m_sprites[SpriteIronGreatmace1Random7]->load(path + "iron-greatmace1-random7.cub", database, true, false);
    m_sprites[SpriteIronGreatmace2Random7]->load(path + "iron-greatmace2-random7.cub", database, true, false);
    m_sprites[SpriteIronGreatmace3Random7]->load(path + "iron-greatmace3-random7.cub", database, true, false);
    m_sprites[SpriteIronGreatmace4Random7]->load(path + "iron-greatmace4-random7.cub", database, true, false);
    m_sprites[SpriteIronGreatmace5Random7]->load(path + "iron-greatmace5-random7.cub", database, true, false);
    m_sprites[SpriteIronGreatmace1Random8]->load(path + "iron-greatmace1-random8.cub", database, true, false);
    m_sprites[SpriteIronGreatmace2Random8]->load(path + "iron-greatmace2-random8.cub", database, true, false);
    m_sprites[SpriteIronGreatmace3Random8]->load(path + "iron-greatmace3-random8.cub", database, true, false);
    m_sprites[SpriteIronGreatmace4Random8]->load(path + "iron-greatmace4-random8.cub", database, true, false);
    m_sprites[SpriteIronGreatmace5Random8]->load(path + "iron-greatmace5-random8.cub", database, true, false);
    m_sprites[SpriteIronGreatmace1Random9]->load(path + "iron-greatmace1-random9.cub", database, true, false);
    m_sprites[SpriteIronGreatmace2Random9]->load(path + "iron-greatmace2-random9.cub", database, true, false);
    m_sprites[SpriteIronGreatmace3Random9]->load(path + "iron-greatmace3-random9.cub", database, true, false);
    m_sprites[SpriteIronGreatmace4Random9]->load(path + "iron-greatmace4-random9.cub", database, true, false);
    m_sprites[SpriteIronGreatmace5Random9]->load(path + "iron-greatmace5-random9.cub", database, true, false);
    m_sprites[SpriteIronGreatmace1Random10]->load(path + "iron-greatmace1-random10.cub", database, true, false);
    m_sprites[SpriteIronGreatmace2Random10]->load(path + "iron-greatmace2-random10.cub", database, true, false);
    m_sprites[SpriteIronGreatmace3Random10]->load(path + "iron-greatmace3-random10.cub", database, true, false);
    m_sprites[SpriteIronGreatmace4Random10]->load(path + "iron-greatmace4-random10.cub", database, true, false);
    m_sprites[SpriteIronGreatmace5Random10]->load(path + "iron-greatmace5-random10.cub", database, true, false);
    m_sprites[SpriteBoneGreatmace]->load(path + "bone-greatmace.cub", database, true, false);
    m_sprites[SpriteObsidianGreatmace]->load(path + "obsidian-greatmace.cub", database, true, false);
    m_sprites[SpriteWoodGreatmace02]->load(path + "wood-greatmace02.cub", database, true, false);
    m_sprites[SpriteBush]->load(path + "bush.cub", database, true, false);
    m_sprites[SpriteSnowBush]->load(path + "snow-bush.cub", database, true, false);
    m_sprites[SpriteCobwebscrub]->load(path + "cobwebscrub.cub", database, true, false);
    m_sprites[SpriteBerryBush]->load(path + "berry-bush.cub", database, true, false);
    m_sprites[SpriteSnowBerry]->load(path + "snow-berry.cub", database, true, false);
    m_sprites[SpriteSnowBerryMash]->load(path + "snow-berry-mash.cub", database, true, false);
    m_sprites[SpriteCottonPlant]->load(path + "cotton-plant.cub", database, true, false);
    m_sprites[SpriteCotton]->load(path + "cotton.cub", database, true, false);
    m_sprites[SpriteScrub]->load(path + "scrub.cub", database, true, false);
    m_sprites[SpriteScrubGreen]->load(path + "scrub-green.cub", database, true, false);
    m_sprites[SpriteFireScrub]->load(path + "fire-scrub.cub", database, true, false);
    m_sprites[SpriteScrubDark]->load(path + "scrub-dark.cub", database, true, false);
    m_sprites[SpriteScrubDarkEyes]->load(path + "scrub-dark-eyes.cub", database, false, false);
    m_sprites[SpriteFirTree]->load(path + "fir-tree.cub", database, true, false);
    m_sprites[SpriteThornTree]->load(path + "thorn-tree.cub", database, true, false);
    m_sprites[SpriteTreeLeaves]->load(path + "tree-leaves.cub", database, true, false);
    m_sprites[SpriteGoldDeposit]->load(path + "gold-deposit.cub", database, true, false);
    m_sprites[SpriteIronDeposit]->load(path + "iron-deposit.cub", database, true, false);
    m_sprites[SpriteSilverDeposit]->load(path + "silver-deposit.cub", database, true, false);
    m_sprites[SpriteSandstoneDeposit]->load(path + "sandstone-deposit.cub", database, true, false);
    m_sprites[SpriteEmeraldDeposit]->load(path + "emerald-deposit.cub", database, true, false);
    m_sprites[SpriteRubyDeposit]->load(path + "ruby-deposit.cub", database, true, false);
    m_sprites[SpriteDiamondDeposit]->load(path + "diamond-deposit.cub", database, true, false);
    m_sprites[SpriteSapphireDeposit]->load(path + "sapphire-deposit.cub", database, true, false);
    m_sprites[SpriteIceCrystalDeposit]->load(path + "ice-crystal-deposit.cub", database, true, false);
    m_sprites[SpriteScarecrow]->load(path + "scarecrow.cub", database, true, false);
    m_sprites[SpriteDummy]->load(path + "dummy.cub", database, true, false);
    m_sprites[SpriteAim]->load(path + "aim.cub", database, true, false);
    m_sprites[SpriteVase]->load(path + "vase.cub", database, true, false);
    m_sprites[SpriteVase2]->load(path + "vase2.cub", database, true, false);
    m_sprites[SpriteVase3]->load(path + "vase3.cub", database, true, false);
    m_sprites[SpriteVase4]->load(path + "vase4.cub", database, true, false);
    m_sprites[SpriteCampfire]->load(path + "campfire.cub", database, true, false);
    m_sprites[SpriteTent]->load(path + "tent.cub", database, true, false);
    m_sprites[SpriteBeachUmbrella]->load(path + "beach-umbrella.cub", database, true, false);
    m_sprites[SpriteBeachTowel]->load(path + "beach-towel.cub", database, true, false);
    m_sprites[SpriteWoodMat]->load(path + "wood-mat.cub", database, true, false);
    m_sprites[SpriteCandle01]->load(path + "candle01.cub", database, true, false);
    m_sprites[SpriteCandle02]->load(path + "candle02.cub", database, true, false);
    m_sprites[SpriteCandle03]->load(path + "candle03.cub", database, true, false);
    m_sprites[SpriteCompass2]->load(path + "compass2.cub", database, true, false);
    m_sprites[SpriteMission]->load(path + "mission.cub", database, true, false);
    m_sprites[SpriteCity]->load(path + "city.cub", database, true, false);
    m_sprites[SpriteSkull]->load(path + "skull.cub", database, true, false);
    m_sprites[SpriteUndeadCandle1]->load(path + "undead-candle1.cub", database, true, false);
    m_sprites[SpriteUndeadCandle2]->load(path + "undead-candle2.cub", database, true, false);
    m_sprites[SpriteUndeadCandle3]->load(path + "undead-candle3.cub", database, true, false);
    m_sprites[SpriteCharacterPlatform]->load(path + "character-platform.cub", database, true, false);
    m_sprites[SpriteAntiqueBuilding1]->load(path + "antique-building1.cub", database, true, true);
    m_sprites[SpriteAntiqueBuilding2]->load(path + "antique-building2.cub", database, true, true);
    m_sprites[SpriteAntiqueBuilding3]->load(path + "antique-building3.cub", database, true, true);
    m_sprites[SpriteAntiqueBuilding4]->load(path + "antique-building4.cub", database, true, true);
    m_sprites[SpriteEntranceCrypt]->load(path + "entrance-crypt.cub", database, true, true);
    m_sprites[SpriteEntranceBarrow]->load(path + "entrance-barrow.cub", database, true, true);
    m_sprites[SpriteEntranceMine]->load(path + "entrance-mine.cub", database, true, true);
    m_sprites[SpriteEntranceAntique]->load(path + "entrance-antique.cub", database, true, true);
    m_sprites[SpriteEntranceTomb]->load(path + "entrance-tomb.cub", database, true, true);
    m_sprites[SpriteVampireHead]->load(path + "vampire-head.cub", database, true, false);
    m_sprites[SpriteVampireHand]->load(path + "vampire-hand.cub", database, true, false);
    m_sprites[SpriteVampireEyes]->load(path + "vampire-eyes.cub", database, false, false);
    m_sprites[SpriteMonsterBodyReptileShell]->load(path + "monster-body-reptile-shell.cub", database, true, false);
    m_sprites[SpriteMonsterBodyReptileShellSpiked]->load(path + "monster-body-reptile-shell-spiked.cub", database, true,
                                                         false);
    m_sprites[SpriteMonsterBodyReptileCrest1]->load(path + "monster-body-reptile-crest1.cub", database, true, false);
    m_sprites[SpriteMonsterBodyReptileCrest2]->load(path + "monster-body-reptile-crest2.cub", database, true, false);
    m_sprites[SpriteMonsterBodyReptileCrest3]->load(path + "monster-body-reptile-crest3.cub", database, true, false);
    m_sprites[SpriteMonsterBodyReptileCrest4]->load(path + "monster-body-reptile-crest4.cub", database, true, false);
    m_sprites[SpriteMonsterBodyReptileCrest5]->load(path + "monster-body-reptile-crest5.cub", database, true, false);
    m_sprites[SpriteMonsterBodyReptileSpiked1]->load(path + "monster-body-reptile-spiked1.cub", database, true, false);
    m_sprites[SpriteMonsterBodyReptileSpiked2]->load(path + "monster-body-reptile-spiked2.cub", database, true, false);
    m_sprites[SpriteMonsterFootReptileNormal]->load(path + "monster-foot-reptile-normal.cub", database, true, false);
    m_sprites[SpriteMonsterFootReptileClaw1]->load(path + "monster-foot-reptile-claw1.cub", database, true, false);
    m_sprites[SpriteMonsterFootReptileClaw2]->load(path + "monster-foot-reptile-claw2.cub", database, true, false);
    m_sprites[SpriteMonsterHeadReptileLizard]->load(path + "monster-head-reptile-lizard.cub", database, true, false);
    m_sprites[SpriteMonsterHeadReptileTurtle]->load(path + "monster-head-reptile-turtle.cub", database, true, false);
    m_sprites[SpriteMonsterHeadReptileHorn1]->load(path + "monster-head-reptile-horn1.cub", database, true, false);
    m_sprites[SpriteMonsterHeadReptileHorn2]->load(path + "monster-head-reptile-horn2.cub", database, true, false);
    m_sprites[SpriteMonsterHeadReptileHorn3]->load(path + "monster-head-reptile-horn3.cub", database, true, false);
    m_sprites[SpriteMonsterTailReptileNormal]->load(path + "monster-tail-reptile-normal.cub", database, true, false);
    m_sprites[SpriteMonsterTailReptileSpike1]->load(path + "monster-tail-reptile-spike1.cub", database, true, false);
    m_sprites[SpriteMonsterTailReptileSpike2]->load(path + "monster-tail-reptile-spike2.cub", database, true, false);
    m_sprites[SpriteRuneGiantFoot01]->load(path + "rune-giant-foot01.cub", database, true, false);
    m_sprites[SpriteRuneGiantHeadNormal01]->load(path + "rune-giant-head-normal01.cub", database, true, false);
    m_sprites[SpriteRuneGiantHeadLaser]->load(path + "rune-giant-head-laser.cub", database, true, false);
    m_sprites[SpriteRuneGiantBody01]->load(path + "rune-giant-body01.cub", database, true, false);
    m_sprites[SpriteRuneGiantBody02]->load(path + "rune-giant-body02.cub", database, true, false);
    m_sprites[SpriteRuneGiantHand01]->load(path + "rune-giant-hand01.cub", database, true, false);
    m_sprites[SpriteBuildingStiltInn1]->load(path + "building-stilt-inn1.cub", database, true, true);
    m_sprites[SpriteBuildingStiltHut2]->load(path + "building-stilt-hut2.cub", database, true, true);
    m_sprites[SpriteBuildingStiltHut3]->load(path + "building-stilt-hut3.cub", database, true, true);
    m_sprites[SpriteBuildingStiltPath]->load(path + "building-stilt-path.cub", database, true, true);
    m_sprites[SpriteFrameHouse01]->load(path + "frame-house01.cub", database, true, true);
    m_sprites[SpriteFrameInn]->load(path + "frame-inn.cub", database, true, true);
    m_sprites[SpriteFrameShop]->load(path + "frame-shop.cub", database, true, true);
    m_sprites[SpriteFrameTower]->load(path + "frame-tower.cub", database, true, true);
    m_sprites[SpriteBuildingStoneWell]->load(path + "building-stone-well.cub", database, true, true);
    m_sprites[SpriteFountain]->load(path + "fountain.cub", database, true, true);
    m_sprites[SpriteLandingplace]->load(path + "landingplace.cub", database, true, true);
    m_sprites[SpriteBuildingWarrior]->load(path + "building-warrior.cub", database, true, true);
    m_sprites[SpriteBuildingRanger]->load(path + "building-ranger.cub", database, true, true);
    m_sprites[SpriteBuildingRogue]->load(path + "building-rogue.cub", database, true, true);
    m_sprites[SpriteBuildingMage]->load(path + "building-mage.cub", database, true, true);
    m_sprites[SpriteBuildingMuseum]->load(path + "building-museum.cub", database, true, true);
    m_sprites[SpriteBuildingAlchemy]->load(path + "building-alchemy.cub", database, true, true);
    m_sprites[SpriteBuildingSmithy]->load(path + "building-smithy.cub", database, true, true);
    m_sprites[SpriteBuildingCarpentersshop]->load(path + "building-carpentersshop.cub", database, true, true);
    m_sprites[SpriteBuildingWeavingmill]->load(path + "building-weavingmill.cub", database, true, true);
    m_sprites[SpritePalmLeaf]->load(path + "palm-leaf.cub", database, true, true);
    m_sprites[SpritePalmLeafDiagonal]->load(path + "palm-leaf-diagonal.cub", database, true, true);
    m_sprites[SpriteWhiteCastleRoundTower]->load(path + "white-castle-round-tower.cub", database, true, true);
    m_sprites[SpritePainting01]->load(path + "painting01.cub", database, true, false);
    m_sprites[SpritePainting02]->load(path + "painting02.cub", database, true, false);
    m_sprites[SpritePainting03]->load(path + "painting03.cub", database, true, false);
    m_sprites[SpritePainting04]->load(path + "painting04.cub", database, true, false);
    m_sprites[SpriteDigestedLeftovers01]->load(path + "digested-leftovers01.cub", database, true, false);
    m_sprites[SpriteDigestedLeftovers02]->load(path + "digested-leftovers02.cub", database, true, false);
    m_sprites[SpriteDigestedLeftovers03]->load(path + "digested-leftovers03.cub", database, true, false);
    m_sprites[SpriteDigestedLeftovers04]->load(path + "digested-leftovers04.cub", database, true, false);
    m_sprites[SpriteIconQuest1]->load(path + "icon-quest1.cub", database, true, false);
    m_sprites[SpriteIconQuest2]->load(path + "icon-quest2.cub", database, true, false);
    m_sprites[SpriteIconQuest3]->load(path + "icon-quest3.cub", database, true, false);
    m_sprites[SpriteIconQuest4]->load(path + "icon-quest4.cub", database, true, false);
    m_sprites[SpriteIconQuest5]->load(path + "icon-quest5.cub", database, true, false);
    m_sprites[SpriteIconTalk]->load(path + "icon-talk.cub", database, true, false);
    m_sprites[SpriteIconAnalyze]->load(path + "icon-analyze.cub", database, true, false);
    m_sprites[SpriteIconVendor]->load(path + "icon-vendor.cub", database, true, false);
    m_sprites[SpriteIconDungeon]->load(path + "icon-dungeon.cub", database, true, false);
    m_sprites[SpriteIconRock]->load(path + "icon-rock.cub", database, true, false);
    m_sprites[SpriteIconRocks]->load(path + "icon-rocks.cub", database, true, false);
    m_sprites[SpriteIconCamp]->load(path + "icon-camp.cub", database, true, false);
    m_sprites[SpriteIconTree]->load(path + "icon-tree.cub", database, true, false);
    m_sprites[SpriteIconHouse]->load(path + "icon-house.cub", database, true, false);
    m_sprites[SpriteIconTower]->load(path + "icon-tower.cub", database, true, false);
    m_sprites[SpriteIconArena]->load(path + "icon-arena.cub", database, true, false);
    m_sprites[SpriteIconLocation]->load(path + "icon-location.cub", database, true, false);
    m_sprites[SpriteIconMapcursor]->load(path + "icon-mapcursor.cub", database, true, false);
    m_sprites[SpriteIconZoneborder]->load(path + "icon-zoneborder.cub", database, true, false);
    m_sprites[SpriteIconRuins]->load(path + "icon-ruins.cub", database, true, false);
    m_sprites[SpriteIconPalace]->load(path + "icon-palace.cub", database, true, false);
    m_sprites[SpriteIconFortress]->load(path + "icon-fortress.cub", database, true, false);
    m_sprites[SpriteIconStronghold]->load(path + "icon-stronghold.cub", database, true, false);
    m_sprites[SpriteIconCitadel]->load(path + "icon-citadel.cub", database, true, false);
    m_sprites[SpriteIconCastle]->load(path + "icon-castle.cub", database, true, false);
    m_sprites[SpriteIconCatacombs]->load(path + "icon-catacombs.cub", database, true, false);
    m_sprites[SpriteIconPyramid]->load(path + "icon-pyramid.cub", database, true, false);
    m_sprites[SpriteIconTemple]->load(path + "icon-temple.cub", database, true, false);
    m_sprites[SpriteIconFort]->load(path + "icon-fort.cub", database, true, false);
    m_sprites[SpriteIconChapel]->load(path + "icon-chapel.cub", database, true, false);
    m_sprites[SpriteIconVault]->load(path + "icon-vault.cub", database, true, false);
    m_sprites[SpriteIconShip]->load(path + "icon-ship.cub", database, true, false);
    m_sprites[SpriteIconAltar]->load(path + "icon-altar.cub", database, true, false);
    m_sprites[SpriteIconCrypt]->load(path + "icon-crypt.cub", database, true, false);
    m_sprites[SpriteIconTomb]->load(path + "icon-tomb.cub", database, true, false);
    m_sprites[SpriteIconBloodaxeBase]->load(path + "icon-bloodaxe-base.cub", database, true, false);
    m_sprites[SpriteIconAntiqueRuins]->load(path + "icon-antique-ruins.cub", database, true, false);
    m_sprites[SpriteIconDesertRuins]->load(path + "icon-desert-ruins.cub", database, true, false);
    m_sprites[SpriteIconJungleRuins]->load(path + "icon-jungle-ruins.cub", database, true, false);
    m_sprites[SpriteIconNordicRuins]->load(path + "icon-nordic-ruins.cub", database, true, false);
    m_sprites[SpriteIconGravesite]->load(path + "icon-gravesite.cub", database, true, false);
    m_sprites[SpriteIconDemonRuins]->load(path + "icon-demon-ruins.cub", database, true, false);
    m_sprites[SpriteIconBarrows]->load(path + "icon-barrows.cub", database, true, false);
    m_sprites[SpriteIconGrove]->load(path + "icon-grove.cub", database, true, false);
    m_sprites[SpriteIconDestination]->load(path + "icon-destination.cub", database, true, false);
    m_sprites[SpriteIconDestinationSmall]->load(path + "icon-destination-small.cub", database, true, false);
    m_sprites[SpriteIconWeapons]->load(path + "icon-weapons.cub", database, true, false);
    m_sprites[SpriteIconArmor]->load(path + "icon-armor.cub", database, true, false);
    m_sprites[SpriteIconInn]->load(path + "icon-inn.cub", database, true, false);
    m_sprites[SpriteIconItemShop]->load(path + "icon-item-shop.cub", database, true, false);
    m_sprites[SpriteIconArmorShop]->load(path + "icon-armor-shop.cub", database, true, false);
    m_sprites[SpriteIconWeaponShop]->load(path + "icon-weapon-shop.cub", database, true, false);
    m_sprites[SpriteIconSmithy]->load(path + "icon-smithy.cub", database, true, false);
    m_sprites[SpriteIconCarpentersShop]->load(path + "icon-carpenters-shop.cub", database, true, false);
    m_sprites[SpriteIconTailorsShop]->load(path + "icon-tailors-shop.cub", database, true, false);
    m_sprites[SpriteIconIdentifier]->load(path + "icon-identifier.cub", database, true, false);
    m_sprites[SpriteFlowerVase1]->load(path + "flower-vase1.cub", database, true, false);
    m_sprites[SpriteFlowerVase2]->load(path + "flower-vase2.cub", database, true, false);
    m_sprites[SpriteFlowerVase3]->load(path + "flower-vase3.cub", database, true, false);
    m_sprites[SpriteFlowerVase4]->load(path + "flower-vase4.cub", database, true, false);
    m_sprites[SpriteFlowerVase5]->load(path + "flower-vase5.cub", database, true, false);
    m_sprites[SpriteFlowerVase6]->load(path + "flower-vase6.cub", database, true, false);
    m_sprites[SpriteFlowerVase7]->load(path + "flower-vase7.cub", database, true, false);
    m_sprites[SpriteFlowerVase8]->load(path + "flower-vase8.cub", database, true, false);
    m_sprites[SpriteFlowerVase9]->load(path + "flower-vase9.cub", database, true, false);
    m_sprites[SpritePitchfork]->load(path + "pitchfork.cub", database, true, false);
    m_sprites[SpriteCactus1]->load(path + "cactus1.cub", database, true, false);
    m_sprites[SpriteCactus2]->load(path + "cactus2.cub", database, true, false);
    m_sprites[SpritePumpkin]->load(path + "pumpkin.cub", database, true, false);
    m_sprites[SpriteBigPumpkin]->load(path + "big-pumpkin.cub", database, true, false);
    m_sprites[SpritePumpkinMuffin]->load(path + "pumpkin-muffin.cub", database, true, false);
    m_sprites[SpriteSalad]->load(path + "Salad.cub", database, true, false);
    m_sprites[SpritePineapple]->load(path + "pineapple.cub", database, true, false);
    m_sprites[SpritePineappleSlice]->load(path + "pineapple-slice.cub", database, true, false);
    m_sprites[SpriteIronNugget]->load(path + "iron-nugget.cub", database, true, false);
    m_sprites[SpriteGoldNugget]->load(path + "gold-nugget.cub", database, true, false);
    m_sprites[SpriteSilverNugget]->load(path + "silver-nugget.cub", database, true, false);
    m_sprites[SpriteEmeraldNugget]->load(path + "emerald-nugget.cub", database, true, false);
    m_sprites[SpriteSapphireNugget]->load(path + "sapphire-nugget.cub", database, true, false);
    m_sprites[SpriteRubyNugget]->load(path + "ruby-nugget.cub", database, true, false);
    m_sprites[SpriteDiamondNugget]->load(path + "diamond-nugget.cub", database, true, false);
    m_sprites[SpriteIceCrystal]->load(path + "ice-crystal.cub", database, true, false);
    m_sprites[SpriteIceCoatedYarn]->load(path + "ice-coated-yarn.cub", database, true, false);
    m_sprites[SpriteIronCube]->load(path + "iron-cube.cub", database, true, false);
    m_sprites[SpriteGoldCube]->load(path + "gold-cube.cub", database, true, false);
    m_sprites[SpriteSilverCube]->load(path + "silver-cube.cub", database, true, false);
    m_sprites[SpriteWoodCube]->load(path + "wood-cube.cub", database, true, false);
    m_sprites[SpriteFireCube]->load(path + "fire-cube.cub", database, true, false);
    m_sprites[SpriteIceCube]->load(path + "ice-cube.cub", database, true, false);
    m_sprites[SpriteUnholyCube]->load(path + "unholy-cube.cub", database, true, false);
    m_sprites[SpriteWindCube]->load(path + "wind-cube.cub", database, true, false);
    m_sprites[SpriteCobweb]->load(path + "cobweb.cub", database, true, false);
    m_sprites[SpriteSandstone]->load(path + "sandstone.cub", database, true, false);
    m_sprites[SpriteWoodLog]->load(path + "wood-log.cub", database, true, false);
    m_sprites[SpriteDesertRib]->load(path + "desert-rib.cub", database, true, false);
    m_sprites[SpriteDesertSkull]->load(path + "desert-skull.cub", database, true, false);
    m_sprites[SpriteIronPickaxe]->load(path + "iron-pickaxe.cub", database, true, false);
    m_sprites[SpriteSlimeGreen]->load(path + "slime-green.cub", database, true, false);
    m_sprites[SpriteSlimeYellow]->load(path + "slime-yellow.cub", database, true, false);
    m_sprites[SpriteSlimePink]->load(path + "slime-pink.cub", database, true, false);
    m_sprites[SpriteSlimeBlue]->load(path + "slime-blue.cub", database, true, false);
    m_sprites[SpriteFrightenerHead]->load(path + "frightener-head.cub", database, true, false);
    m_sprites[SpriteFrightenerEyes]->load(path + "frightener-eyes.cub", database, false, false);
    m_sprites[SpriteSandHorrorHead]->load(path + "sand-horror-head.cub", database, true, false);
    m_sprites[SpriteSandHorrorHand]->load(path + "sand-horror-hand.cub", database, true, false);
    m_sprites[SpriteBunnyBody]->load(path + "bunny-body.cub", database, true, false);
    m_sprites[SpriteBunnyFoot]->load(path + "bunny-foot.cub", database, true, false);
    m_sprites[SpritePorcupineBody]->load(path + "porcupine-body.cub", database, true, false);
    m_sprites[SpritePorcupineFoot]->load(path + "porcupine-foot.cub", database, true, false);
    m_sprites[SpritePigBody]->load(path + "pig-body.cub", database, true, false);
    m_sprites[SpritePigFoot]->load(path + "pig-foot.cub", database, true, false);
    m_sprites[SpritePigHead]->load(path + "pig-head.cub", database, true, false);
    m_sprites[SpriteWarthogBody]->load(path + "warthog-body.cub", database, true, false);
    m_sprites[SpriteWarthogFoot]->load(path + "warthog-foot.cub", database, true, false);
    m_sprites[SpriteWarthogHead]->load(path + "warthog-head.cub", database, true, false);
    m_sprites[SpriteSheepBody]->load(path + "sheep-body.cub", database, true, false);
    m_sprites[SpriteSheepFoot]->load(path + "sheep-foot.cub", database, true, false);
    m_sprites[SpriteSheepHead]->load(path + "sheep-head.cub", database, true, false);
    m_sprites[SpriteDuckbillBody]->load(path + "duckbill-body.cub", database, true, false);
    m_sprites[SpriteDuckbillFoot]->load(path + "duckbill-foot.cub", database, true, false);
    m_sprites[SpriteDuckbillHead]->load(path + "duckbill-head.cub", database, true, false);
    m_sprites[SpriteDuckbillTail]->load(path + "duckbill-tail.cub", database, true, false);
    m_sprites[SpriteCrocodileBody]->load(path + "crocodile-body.cub", database, true, false);
    m_sprites[SpriteCrocodileFoot]->load(path + "crocodile-foot.cub", database, true, false);
    m_sprites[SpriteCrocodileHead]->load(path + "crocodile-head.cub", database, true, false);
    m_sprites[SpriteFish1]->load(path + "fish1.cub", database, true, false);
    m_sprites[SpriteFish2]->load(path + "fish2.cub", database, true, false);
    m_sprites[SpriteShark]->load(path + "shark.cub", database, true, false);
    m_sprites[SpriteSeahorse]->load(path + "seahorse.cub", database, true, false);
    m_sprites[SpriteLanternFish]->load(path + "lantern-fish.cub", database, true, false);
    m_sprites[SpriteLanternFishEyes]->load(path + "lantern-fish-eyes.cub", database, true, false);
    m_sprites[SpriteMawfish]->load(path + "mawfish.cub", database, true, false);
    m_sprites[SpritePiranha]->load(path + "piranha.cub", database, true, false);
    m_sprites[SpriteBlowfish]->load(path + "blowfish.cub", database, true, false);
    m_sprites[SpriteSeastar]->load(path + "seastar.cub", database, true, false);
    m_sprites[SpriteBread]->load(path + "bread.cub", database, true, false);
    m_sprites[SpriteSandwich]->load(path + "sandwich.cub", database, true, false);
    m_sprites[SpriteUnknown]->load(path + "unknown.cub", database, true, false);
    m_sprites[SpriteFlowers]->load(path + "flowers.cub", database, true, false);
    m_sprites[SpriteFlowers2]->load(path + "flowers2.cub", database, true, false);
    m_sprites[SpriteFlowers3]->load(path + "flowers3.cub", database, true, false);
    m_sprites[SpriteFlowers4]->load(path + "flowers4.cub", database, true, false);
    m_sprites[SpriteGrass]->load(path + "grass.cub", database, true, false);
    m_sprites[SpriteGrass2]->load(path + "grass2.cub", database, true, false);
    m_sprites[SpriteGrass3]->load(path + "grass3.cub", database, true, false);
    m_sprites[SpriteGrassDry1]->load(path + "grass-dry1.cub", database, true, false);
    m_sprites[SpriteGrassDry2]->load(path + "grass-dry2.cub", database, true, false);
    m_sprites[SpriteGrassDark1]->load(path + "grass-dark1.cub", database, true, false);
    m_sprites[SpriteGrassDark2]->load(path + "grass-dark2.cub", database, true, false);
    m_sprites[SpriteSavannahPlant1]->load(path + "savannah-plant1.cub", database, true, false);
    m_sprites[SpriteSavannahPlant2]->load(path + "savannah-plant2.cub", database, true, false);
    m_sprites[SpriteDarkPlant1]->load(path + "dark-plant1.cub", database, true, false);
    m_sprites[SpriteDarkPlant2]->load(path + "dark-plant2.cub", database, true, false);
    m_sprites[SpriteLavaFlower]->load(path + "lava-flower.cub", database, true, false);
    m_sprites[SpriteLavaGrass]->load(path + "lava-grass.cub", database, true, false);
    m_sprites[SpriteThornPlant]->load(path + "thorn-plant.cub", database, true, false);
    m_sprites[SpriteEchinacea2]->load(path + "echinacea2.cub", database, true, false);
    m_sprites[SpriteLeaf]->load(path + "leaf.cub", database, true, false);
    m_sprites[SpriteLantern02]->load(path + "lantern02.cub", database, true, false);
    m_sprites[SpriteWallLantern]->load(path + "wall-lantern.cub", database, true, false);
    m_sprites[SpriteGreenWallLantern]->load(path + "green-wall-lantern.cub", database, true, false);
    m_sprites[SpriteLanternGreen]->load(path + "lantern-green.cub", database, true, false);
    m_sprites[SpriteTorchSlot3342]->load(path + "torch.cub", database, true, false);
    m_sprites[SpriteStone]->load(path + "stone.cub", database, true, false);
    m_sprites[SpriteStone2]->load(path + "stone2.cub", database, true, false);
    m_sprites[SpriteTendril]->load(path + "tendril.cub", database, true, false);
    m_sprites[SpriteTulipsColorful]->load(path + "tulips-colorful.cub", database, true, false);
    m_sprites[SpriteCornflower]->load(path + "cornflower.cub", database, true, false);
    m_sprites[SpriteReed]->load(path + "reed.cub", database, true, false);
    m_sprites[SpriteGraveStone1]->load(path + "grave-stone1.cub", database, true, false);
    m_sprites[SpriteGraveStone2]->load(path + "grave-stone2.cub", database, true, false);
    m_sprites[SpriteGraveStone3]->load(path + "grave-stone3.cub", database, true, false);
    m_sprites[SpriteSkullRotten]->load(path + "skull-rotten.cub", database, true, false);
    m_sprites[SpriteMoss1]->load(path + "moss1.cub", database, true, false);
    m_sprites[SpriteMoss2]->load(path + "moss2.cub", database, true, false);
    m_sprites[SpritePumpkinLeaves]->load(path + "pumpkin-leaves.cub", database, true, false);
    m_sprites[SpritePineappleLeaves]->load(path + "pineapple-leaves.cub", database, true, false);
    m_sprites[SpriteSunflower]->load(path + "sunflower.cub", database, true, false);
    m_sprites[SpriteBeanTendril]->load(path + "bean-tendril.cub", database, true, false);
    m_sprites[SpriteDesertFlower01]->load(path + "desert-flower01.cub", database, true, false);
    m_sprites[SpriteDesertFlower02]->load(path + "desert-flower02.cub", database, true, false);
    m_sprites[SpriteWheat]->load(path + "wheat.cub", database, true, false);
    m_sprites[SpriteCorn]->load(path + "corn.cub", database, true, false);
    m_sprites[SpriteWaterLily01]->load(path + "water-lily01.cub", database, true, false);
    m_sprites[SpriteWaterLily02]->load(path + "water-lily02.cub", database, true, false);
    m_sprites[SpriteInnSign]->load(path + "inn-sign.cub", database, true, false);
    m_sprites[SpriteIdentifierSign]->load(path + "identifier-sign.cub", database, true, false);
    m_sprites[SpriteShopSign]->load(path + "shop-sign.cub", database, true, false);
    m_sprites[SpriteWeaponShopSign]->load(path + "weapon-shop-sign.cub", database, true, false);
    m_sprites[SpriteArmorShopSign]->load(path + "armor-shop-sign.cub", database, true, false);
    m_sprites[SpriteSmithySign]->load(path + "smithy-sign.cub", database, true, false);
    m_sprites[SpriteCarpentersshopSign]->load(path + "carpentersshop-sign.cub", database, true, false);
    m_sprites[SpriteWeavingmillSign]->load(path + "weavingmill-sign.cub", database, true, false);
    m_sprites[SpriteIvy]->load(path + "ivy.cub", database, true, false);
    m_sprites[SpriteWallRosesWhite]->load(path + "wall-roses-white.cub", database, true, false);
    m_sprites[SpriteWallRosesRed]->load(path + "wall-roses-red.cub", database, true, false);
    m_sprites[SpriteChristmasTree]->load(path + "christmas-tree.cub", database, true, false);
    m_sprites[SpriteCoral]->load(path + "coral.cub", database, true, false);
    m_sprites[SpriteUnderwaterPlant]->load(path + "underwater-plant.cub", database, true, false);
    m_sprites[SpriteAlga]->load(path + "alga.cub", database, true, false);
    m_sprites[SpriteIncaArt1]->load(path + "inca-art1.cub", database, true, false);
    m_sprites[SpriteIncaArt2]->load(path + "inca-art2.cub", database, true, false);
    m_sprites[SpriteIncaArt3]->load(path + "inca-art3.cub", database, true, false);
    m_sprites[SpriteIncaArt4]->load(path + "inca-art4.cub", database, true, false);
    m_sprites[SpriteLiana]->load(path + "liana.cub", database, true, false);
    m_sprites[SpriteCrest1]->load(path + "crest1.cub", database, true, false);
    m_sprites[SpriteCastleChain]->load(path + "castle-chain.cub", database, true, false);
    m_sprites[SpriteWallSkull]->load(path + "wall-skull.cub", database, true, false);
    m_sprites[SpriteTorchRead]->load(path + "torch-read.cub", database, true, false);
    m_sprites[SpriteTorchGreen]->load(path + "torch-green.cub", database, true, false);
    m_sprites[SpriteTorchBlue]->load(path + "torch-blue.cub", database, true, false);
    m_sprites[SpriteTorchYellow]->load(path + "torch-yellow.cub", database, true, false);
    m_sprites[SpriteChandelier]->load(path + "chandelier.cub", database, true, false);
    m_sprites[SpriteCobwebs]->load(path + "cobwebs.cub", database, true, false);
    m_sprites[SpriteCobwebs2]->load(path + "cobwebs2.cub", database, true, false);
    m_sprites[SpriteStreetPennon1]->load(path + "street-pennon1.cub", database, true, false);
    m_sprites[SpriteStreetPennon2]->load(path + "street-pennon2.cub", database, true, false);
    m_sprites[SpriteStreetPennon3]->load(path + "street-pennon3.cub", database, true, false);
    m_sprites[SpriteStreetPennon4]->load(path + "street-pennon4.cub", database, true, false);
    m_sprites[SpriteStreetPennon5]->load(path + "street-pennon5.cub", database, true, false);
    m_sprites[SpriteGateBanner1]->load(path + "gate-banner1.cub", database, true, false);
    m_sprites[SpriteGateBanner2]->load(path + "gate-banner2.cub", database, true, false);
    m_sprites[SpriteGateBanner3]->load(path + "gate-banner3.cub", database, true, false);
    m_sprites[SpriteGateBanner4]->load(path + "gate-banner4.cub", database, true, false);
    m_sprites[SpriteGateBanner5]->load(path + "gate-banner5.cub", database, true, false);
    m_sprites[SpriteGoddess2Slot3405]->load(path + "goddess2.cub", database, true, false);
    m_sprites[SpriteDoorSlot3406]->load(path + "door.cub", database, true, false);
    m_sprites[SpriteBigDoor]->load(path + "big-door.cub", database, true, false);
    m_sprites[SpriteBigIronDoor]->load(path + "big-iron-door.cub", database, true, false);
    m_sprites[SpriteWindowSlot3409]->load(path + "window.cub", database, true, false);
    m_sprites[SpriteCastleWindow]->load(path + "castle-window.cub", database, true, false);
    m_sprites[SpriteGate]->load(path + "gate.cub", database, true, false);
    m_sprites[SpriteSpikeTrap]->load(path + "spike-trap.cub", database, true, false);
    m_sprites[SpriteStompTrap]->load(path + "stomp-trap.cub", database, true, false);
    m_sprites[SpriteLeverSlot3414]->load(path + "lever.cub", database, true, false);
    m_sprites[SpriteChestBase02]->load(path + "chest-base02.cub", database, true, false);
    m_sprites[SpriteChestTop02]->load(path + "chest-top02.cub", database, true, false);
    m_sprites[SpriteChestBaseUndead1]->load(path + "chest-base-undead1.cub", database, true, false);
    m_sprites[SpriteChestTopUndead1]->load(path + "chest-top-undead1.cub", database, true, false);
    m_sprites[SpriteChestBaseObsidian]->load(path + "chest-base-obsidian.cub", database, true, false);
    m_sprites[SpriteChestTopObsidian]->load(path + "chest-top-obsidian.cub", database, true, false);
    m_sprites[SpriteChestBaseBone]->load(path + "chest-base-bone.cub", database, true, false);
    m_sprites[SpriteChestTopBone]->load(path + "chest-top-bone.cub", database, true, false);
    m_sprites[SpriteTable]->load(path + "table.cub", database, true, false);
    m_sprites[SpriteStoneTable]->load(path + "stone-table.cub", database, true, false);
    m_sprites[SpriteSandstoneTable]->load(path + "sandstone-table.cub", database, true, false);
    m_sprites[SpriteStoneStool]->load(path + "stone-stool.cub", database, true, false);
    m_sprites[SpriteSandstoneStool]->load(path + "sandstone-stool.cub", database, true, false);
    m_sprites[SpriteStool]->load(path + "stool.cub", database, true, false);
    m_sprites[SpriteBench]->load(path + "bench.cub", database, true, false);
    m_sprites[SpriteBenchBlack]->load(path + "bench-black.cub", database, true, false);
    m_sprites[SpriteBed]->load(path + "bed.cub", database, true, false);
    m_sprites[SpriteBedtable]->load(path + "bedtable.cub", database, true, false);
    m_sprites[SpriteCrate]->load(path + "crate.cub", database, true, false);
    m_sprites[SpriteOpenCrate]->load(path + "open-crate.cub", database, true, false);
    m_sprites[SpriteSack]->load(path + "sack.cub", database, true, false);
    m_sprites[SpriteMarketStand1]->load(path + "market-stand1.cub", database, true, false);
    m_sprites[SpriteMarketStand2]->load(path + "market-stand2.cub", database, true, false);
    m_sprites[SpriteMarketStand3]->load(path + "market-stand3.cub", database, true, false);
    m_sprites[SpriteBarrel]->load(path + "barrel.cub", database, true, false);
    m_sprites[SpriteShelter]->load(path + "shelter.cub", database, true, false);
    m_sprites[SpriteCupboard]->load(path + "cupboard.cub", database, true, false);
    m_sprites[SpriteDesktop]->load(path + "desktop.cub", database, true, false);
    m_sprites[SpriteCounter]->load(path + "counter.cub", database, true, false);
    m_sprites[SpriteShelf1]->load(path + "shelf1.cub", database, true, false);
    m_sprites[SpriteShelf2]->load(path + "shelf2.cub", database, true, false);
    m_sprites[SpriteShelf3]->load(path + "shelf3.cub", database, true, false);
    m_sprites[SpriteCastleShelf1]->load(path + "castle-shelf1.cub", database, true, false);
    m_sprites[SpriteCastleShelf2]->load(path + "castle-shelf2.cub", database, true, false);
    m_sprites[SpriteCastleShelf3]->load(path + "castle-shelf3.cub", database, true, false);
    m_sprites[SpriteStoneShelf1]->load(path + "stone-shelf1.cub", database, true, false);
    m_sprites[SpriteStoneShelf2]->load(path + "stone-shelf2.cub", database, true, false);
    m_sprites[SpriteStoneShelf3]->load(path + "stone-shelf3.cub", database, true, false);
    m_sprites[SpriteSandstoneShelf1]->load(path + "sandstone-shelf1.cub", database, true, false);
    m_sprites[SpriteSandstoneShelf2]->load(path + "sandstone-shelf2.cub", database, true, false);
    m_sprites[SpriteSandstoneShelf3]->load(path + "sandstone-shelf3.cub", database, true, false);
    m_sprites[SpriteCorpse]->load(path + "corpse.cub", database, true, false);
    m_sprites[SpriteRunestone]->load(path + "runestone.cub", database, true, false);
    m_sprites[SpriteFlowerBox01]->load(path + "flower-box01.cub", database, true, false);
    m_sprites[SpriteFlowerBox02]->load(path + "flower-box02.cub", database, true, false);
    m_sprites[SpriteFlowerBox03]->load(path + "flower-box03.cub", database, true, false);
    m_sprites[SpriteStreetLight03]->load(path + "street-light03.cub", database, true, false);
    m_sprites[SpriteStreetLight02]->load(path + "street-light02.cub", database, true, false);
    m_sprites[SpriteStreetLight]->load(path + "street-light.cub", database, true, false);
    m_sprites[SpriteStreetLight01]->load(path + "street-light01.cub", database, true, false);
    m_sprites[SpriteStreetLightUndead]->load(path + "street-light-undead.cub", database, true, false);
    m_sprites[SpriteFence01]->load(path + "fence01.cub", database, true, false);
    m_sprites[SpriteFence02]->load(path + "fence02.cub", database, true, false);
    m_sprites[SpriteFence03]->load(path + "fence03.cub", database, true, false);
    m_sprites[SpriteFence04]->load(path + "fence04.cub", database, true, false);
    m_sprites[SpriteFenceIron1]->load(path + "fence-iron1.cub", database, true, false);
    m_sprites[SpriteFenceIron2]->load(path + "fence-iron2.cub", database, true, false);
    m_sprites[SpriteFenceIron3]->load(path + "fence-iron3.cub", database, true, false);
    m_sprites[SpriteFurnace]->load(path + "furnace.cub", database, true, false);
    m_sprites[SpriteAnvil]->load(path + "anvil.cub", database, true, false);
    m_sprites[SpriteSpinningwheel]->load(path + "spinningwheel.cub", database, true, false);
    m_sprites[SpriteLoom]->load(path + "loom.cub", database, true, false);
    m_sprites[SpriteSawbench]->load(path + "sawbench.cub", database, true, false);
    m_sprites[SpriteWorkbench]->load(path + "workbench.cub", database, true, false);
    m_sprites[SpriteCustomizationBench]->load(path + "customization-bench.cub", database, true, false);
    m_sprites[SpriteEagleSeat]->load(path + "eagle-seat.cub", database, true, false);
    m_sprites[SpriteOvergroundDungeon01]->load(path + "overground-dungeon01.cub", database, true, false);
    m_sprites[SpriteUndergroundDungeon01]->load(path + "underground-dungeon01.cub", database, true, false);
    m_sprites[SpriteCastleArc]->load(path + "castle-arc.cub", database, true, false);
    m_sprites[SpriteTempleArc]->load(path + "temple-arc.cub", database, true, false);
    m_sprites[SpritePalaceArc]->load(path + "palace-arc.cub", database, true, false);
    m_sprites[SpriteFortArc]->load(path + "fort-arc.cub", database, true, false);
    m_sprites[SpriteCitadelArc]->load(path + "citadel-arc.cub", database, true, false);
    m_sprites[SpriteBomb1]->load(path + "bomb1.cub", database, true, false);
    m_sprites[SpriteGlider]->load(path + "glider.cub", database, true, false);
    m_sprites[SpriteBoat2]->load(path + "boat2.cub", database, true, false);
    m_sprites[SpriteClimbingSpikes]->load(path + "climbing-spikes.cub", database, true, false);
    m_sprites[SpriteReins]->load(path + "reins.cub", database, true, false);
    m_sprites[SpritePortalStone]->load(path + "portal-stone.cub", database, true, false);
    m_sprites[SpriteNote]->load(path + "note.cub", database, true, false);
    m_sprites[SpriteStoneBridge]->load(path + "stone-bridge.cub", database, true, false);
    m_sprites[SpriteEntranceStairs]->load(path + "entrance-stairs.cub", database, true, false);
    m_sprites[SpriteCarpet1]->load(path + "carpet1.cub", database, true, false);
    m_sprites[SpriteCarpet2]->load(path + "carpet2.cub", database, true, false);
    m_sprites[SpriteCarpet3]->load(path + "carpet3.cub", database, true, false);
    m_sprites[SpriteFrameworkBase]->load(path + "framework-base.cub", database, true, true);
    m_sprites[SpriteFrameworkFloor]->load(path + "framework-floor.cub", database, true, true);
    m_sprites[SpriteFrameworkFloorStairs]->load(path + "framework-floor-stairs.cub", database, true, true);
    m_sprites[SpriteFrameworkWall]->load(path + "framework-wall.cub", database, true, true);
    m_sprites[SpriteFrameworkWallWindow]->load(path + "framework-wall-window.cub", database, true, true);
    m_sprites[SpriteFrameworkWallDoor]->load(path + "framework-wall-door.cub", database, true, true);
    m_sprites[SpriteFrameworkWallIndoor]->load(path + "framework-wall-indoor.cub", database, true, true);
    m_sprites[SpriteFrameworkWallBalcony]->load(path + "framework-wall-balcony.cub", database, true, true);
    m_sprites[SpriteFrameworkWallLamp]->load(path + "framework-wall-lamp.cub", database, true, true);
    m_sprites[SpriteFrameworkRoof1]->load(path + "framework-roof1.cub", database, true, true);
    m_sprites[SpriteFrameworkRoof2]->load(path + "framework-roof2.cub", database, true, true);
    m_sprites[SpriteFrameworkRoof3]->load(path + "framework-roof3.cub", database, true, true);
    m_sprites[SpriteFrameworkArc]->load(path + "framework-arc.cub", database, true, true);
    m_sprites[SpriteStoneBase]->load(path + "stone-base.cub", database, true, true);
    m_sprites[SpriteStoneFloor]->load(path + "stone-floor.cub", database, true, true);
    m_sprites[SpriteStoneFloorStairs]->load(path + "stone-floor-stairs.cub", database, true, true);
    m_sprites[SpriteStoneWall]->load(path + "stone-wall.cub", database, true, true);
    m_sprites[SpriteStoneWallWindow]->load(path + "stone-wall-window.cub", database, true, true);
    m_sprites[SpriteStoneWallDoor]->load(path + "stone-wall-door.cub", database, true, true);
    m_sprites[SpriteStoneWallIndoor]->load(path + "stone-wall-indoor.cub", database, true, true);
    m_sprites[SpriteStoneWallBalcony]->load(path + "stone-wall-balcony.cub", database, true, true);
    m_sprites[SpriteStoneWallLamp]->load(path + "stone-wall-lamp.cub", database, true, true);
    m_sprites[SpriteStoneRoof1]->load(path + "stone-roof1.cub", database, true, true);
    m_sprites[SpriteStoneRoof2]->load(path + "stone-roof2.cub", database, true, true);
    m_sprites[SpriteStoneRoof3]->load(path + "stone-roof3.cub", database, true, true);
    m_sprites[SpriteStoneArc]->load(path + "stone-arc.cub", database, true, true);
    m_sprites[SpriteWhitewoodBase]->load(path + "whitewood-base.cub", database, true, true);
    m_sprites[SpriteWhitewoodFloor]->load(path + "whitewood-floor.cub", database, true, true);
    m_sprites[SpriteWhitewoodFloorStairs]->load(path + "whitewood-floor-stairs.cub", database, true, true);
    m_sprites[SpriteWhitewoodWall]->load(path + "whitewood-wall.cub", database, true, true);
    m_sprites[SpriteWhitewoodWallWindow]->load(path + "whitewood-wall-window.cub", database, true, true);
    m_sprites[SpriteWhitewoodWallDoor]->load(path + "whitewood-wall-door.cub", database, true, true);
    m_sprites[SpriteWhitewoodWallIndoor]->load(path + "whitewood-wall-indoor.cub", database, true, true);
    m_sprites[SpriteWhitewoodWallBalcony]->load(path + "whitewood-wall-balcony.cub", database, true, true);
    m_sprites[SpriteWhitewoodWallLamp]->load(path + "whitewood-wall-lamp.cub", database, true, true);
    m_sprites[SpriteWhitewoodRoof1]->load(path + "whitewood-roof1.cub", database, true, true);
    m_sprites[SpriteWhitewoodRoof2]->load(path + "whitewood-roof2.cub", database, true, true);
    m_sprites[SpriteWhitewoodRoof3]->load(path + "whitewood-roof3.cub", database, true, true);
    m_sprites[SpriteWhitewoodArc]->load(path + "whitewood-arc.cub", database, true, true);
    m_sprites[SpriteWoodBase]->load(path + "wood-base.cub", database, true, true);
    m_sprites[SpriteWoodFloor]->load(path + "wood-floor.cub", database, true, true);
    m_sprites[SpriteWoodFloorStairs]->load(path + "wood-floor-stairs.cub", database, true, true);
    m_sprites[SpriteWoodWall]->load(path + "wood-wall.cub", database, true, true);
    m_sprites[SpriteWoodWallWindow]->load(path + "wood-wall-window.cub", database, true, true);
    m_sprites[SpriteWoodWallDoor]->load(path + "wood-wall-door.cub", database, true, true);
    m_sprites[SpriteWoodWallIndoor]->load(path + "wood-wall-indoor.cub", database, true, true);
    m_sprites[SpriteWoodWallBalcony]->load(path + "wood-wall-balcony.cub", database, true, true);
    m_sprites[SpriteWoodWallLamp]->load(path + "wood-wall-lamp.cub", database, true, true);
    m_sprites[SpriteWoodRoof1]->load(path + "wood-roof1.cub", database, true, true);
    m_sprites[SpriteWoodRoof2]->load(path + "wood-roof2.cub", database, true, true);
    m_sprites[SpriteWoodRoof3]->load(path + "wood-roof3.cub", database, true, true);
    m_sprites[SpriteWoodArc]->load(path + "wood-arc.cub", database, true, true);
    m_sprites[SpriteClayEntranceStairs]->load(path + "clay-entrance-stairs.cub", database, true, true);
    m_sprites[SpriteClayBase]->load(path + "clay-base.cub", database, true, true);
    m_sprites[SpriteClayFloor]->load(path + "clay-floor.cub", database, true, true);
    m_sprites[SpriteClayFloorStairs]->load(path + "clay-floor-stairs.cub", database, true, true);
    m_sprites[SpriteClayWall]->load(path + "clay-wall.cub", database, true, true);
    m_sprites[SpriteClayWallWindow]->load(path + "clay-wall-window.cub", database, true, true);
    m_sprites[SpriteClayWallDoor]->load(path + "clay-wall-door.cub", database, true, true);
    m_sprites[SpriteClayWallIndoor]->load(path + "clay-wall-indoor.cub", database, true, true);
    m_sprites[SpriteClayWallBalcony]->load(path + "clay-wall-balcony.cub", database, true, true);
    m_sprites[SpriteClayWallLamp]->load(path + "clay-wall-lamp.cub", database, true, true);
    m_sprites[SpriteClayRoof1]->load(path + "clay-roof1.cub", database, true, true);
    m_sprites[SpriteClayRoof2]->load(path + "clay-roof2.cub", database, true, true);
    m_sprites[SpriteClayRoof3]->load(path + "clay-roof3.cub", database, true, true);
    m_sprites[SpriteClayArc]->load(path + "clay-arc.cub", database, true, true);
    m_sprites[SpriteWhiteclayEntranceStairs]->load(path + "whiteclay-entrance-stairs.cub", database, true, true);
    m_sprites[SpriteWhiteclayBase]->load(path + "whiteclay-base.cub", database, true, true);
    m_sprites[SpriteWhiteclayFloor]->load(path + "whiteclay-floor.cub", database, true, true);
    m_sprites[SpriteWhiteclayFloorStairs]->load(path + "whiteclay-floor-stairs.cub", database, true, true);
    m_sprites[SpriteWhiteclayWall]->load(path + "whiteclay-wall.cub", database, true, true);
    m_sprites[SpriteWhiteclayWallWindow]->load(path + "whiteclay-wall-window.cub", database, true, true);
    m_sprites[SpriteWhiteclayWallDoor]->load(path + "whiteclay-wall-door.cub", database, true, true);
    m_sprites[SpriteWhiteclayWallIndoor]->load(path + "whiteclay-wall-indoor.cub", database, true, true);
    m_sprites[SpriteWhiteclayWallBalcony]->load(path + "whiteclay-wall-balcony.cub", database, true, true);
    m_sprites[SpriteWhiteclayWallLamp]->load(path + "whiteclay-wall-lamp.cub", database, true, true);
    m_sprites[SpriteWhiteclayRoof1]->load(path + "whiteclay-roof1.cub", database, true, true);
    m_sprites[SpriteWhiteclayRoof2]->load(path + "whiteclay-roof2.cub", database, true, true);
    m_sprites[SpriteWhiteclayRoof3]->load(path + "whiteclay-roof3.cub", database, true, true);
    m_sprites[SpriteWhiteclayArc]->load(path + "whiteclay-arc.cub", database, true, true);
    m_sprites[SpriteOrchouseEntranceStairs]->load(path + "orchouse-entrance-stairs.cub", database, true, true);
    m_sprites[SpriteOrchouseBase]->load(path + "orchouse-base.cub", database, true, true);
    m_sprites[SpriteOrchouseFloor]->load(path + "orchouse-floor.cub", database, true, true);
    m_sprites[SpriteOrchouseFloorStairs]->load(path + "orchouse-floor-stairs.cub", database, true, true);
    m_sprites[SpriteOrchouseWall]->load(path + "orchouse-wall.cub", database, true, true);
    m_sprites[SpriteOrchouseWallWindow]->load(path + "orchouse-wall-window.cub", database, true, true);
    m_sprites[SpriteOrchouseWallDoor]->load(path + "orchouse-wall-door.cub", database, true, true);
    m_sprites[SpriteOrchouseWallIndoor]->load(path + "orchouse-wall-indoor.cub", database, true, true);
    m_sprites[SpriteOrchouseWallBalcony]->load(path + "orchouse-wall-balcony.cub", database, true, true);
    m_sprites[SpriteOrchouseWallLamp]->load(path + "orchouse-wall-lamp.cub", database, true, true);
    m_sprites[SpriteOrchouseRoof1]->load(path + "orchouse-roof1.cub", database, true, true);
    m_sprites[SpriteOrchouseRoof2]->load(path + "orchouse-roof2.cub", database, true, true);
    m_sprites[SpriteOrchouseRoof3]->load(path + "orchouse-roof3.cub", database, true, true);
    m_sprites[SpriteOrchouseArc]->load(path + "orchouse-arc.cub", database, true, true);
    m_sprites[SpriteLavaEntranceStairs]->load(path + "lava-entrance-stairs.cub", database, true, true);
    m_sprites[SpriteLavaBase]->load(path + "lava-base.cub", database, true, true);
    m_sprites[SpriteLavaFloor]->load(path + "lava-floor.cub", database, true, true);
    m_sprites[SpriteLavaFloorStairs]->load(path + "lava-floor-stairs.cub", database, true, true);
    m_sprites[SpriteLavaWall]->load(path + "lava-wall.cub", database, true, true);
    m_sprites[SpriteLavaWallWindow]->load(path + "lava-wall-window.cub", database, true, true);
    m_sprites[SpriteLavaWallDoor]->load(path + "lava-wall-door.cub", database, true, true);
    m_sprites[SpriteLavaWallIndoor]->load(path + "lava-wall-indoor.cub", database, true, true);
    m_sprites[SpriteLavaWallBalcony]->load(path + "lava-wall-balcony.cub", database, true, true);
    m_sprites[SpriteLavaWallLamp]->load(path + "lava-wall-lamp.cub", database, true, true);
    m_sprites[SpriteLavaRoof1]->load(path + "lava-roof1.cub", database, true, true);
    m_sprites[SpriteLavaRoof2]->load(path + "lava-roof2.cub", database, true, true);
    m_sprites[SpriteLavaRoof3]->load(path + "lava-roof3.cub", database, true, true);
    m_sprites[SpriteLavaArc]->load(path + "lava-arc.cub", database, true, true);
    m_sprites[SpriteUndeadhouseEntranceStairs]->load(path + "undeadhouse-entrance-stairs.cub", database, true, true);
    m_sprites[SpriteUndeadhouseBase]->load(path + "undeadhouse-base.cub", database, true, true);
    m_sprites[SpriteUndeadhouseFloor]->load(path + "undeadhouse-floor.cub", database, true, true);
    m_sprites[SpriteUndeadhouseFloorStairs]->load(path + "undeadhouse-floor-stairs.cub", database, true, true);
    m_sprites[SpriteUndeadhouseWall]->load(path + "undeadhouse-wall.cub", database, true, true);
    m_sprites[SpriteUndeadhouseWallWindow]->load(path + "undeadhouse-wall-window.cub", database, true, true);
    m_sprites[SpriteUndeadhouseWallDoor]->load(path + "undeadhouse-wall-door.cub", database, true, true);
    m_sprites[SpriteUndeadhouseWallIndoor]->load(path + "undeadhouse-wall-indoor.cub", database, true, true);
    m_sprites[SpriteUndeadhouseWallBalcony]->load(path + "undeadhouse-wall-balcony.cub", database, true, true);
    m_sprites[SpriteUndeadhouseWallLamp]->load(path + "undeadhouse-wall-lamp.cub", database, true, true);
    m_sprites[SpriteUndeadhouseRoof]->load(path + "undeadhouse-roof.cub", database, true, true);
    m_sprites[SpriteUndeadhouseArc]->load(path + "undeadhouse-arc.cub", database, true, true);
    m_sprites[SpriteCemeteryEntranceStairs]->load(path + "cemetery-entrance-stairs.cub", database, true, true);
    m_sprites[SpriteCemeteryBase]->load(path + "cemetery-base.cub", database, true, true);
    m_sprites[SpriteCemeteryFloor]->load(path + "cemetery-floor.cub", database, true, true);
    m_sprites[SpriteCemeteryFloorStairs]->load(path + "cemetery-floor-stairs.cub", database, true, true);
    m_sprites[SpriteCemeteryWall]->load(path + "cemetery-wall.cub", database, true, true);
    m_sprites[SpriteCemeteryWallWindow]->load(path + "cemetery-wall-window.cub", database, true, true);
    m_sprites[SpriteCemeteryWallDoor]->load(path + "cemetery-wall-door.cub", database, true, true);
    m_sprites[SpriteCemeteryWallIndoor]->load(path + "cemetery-wall-indoor.cub", database, true, true);
    m_sprites[SpriteCemeteryWallBalcony]->load(path + "cemetery-wall-balcony.cub", database, true, true);
    m_sprites[SpriteCemeteryWallLamp]->load(path + "cemetery-wall-lamp.cub", database, true, true);
    m_sprites[SpriteCemeteryRoof]->load(path + "cemetery-roof.cub", database, true, true);
    m_sprites[SpriteCemeteryArc]->load(path + "cemetery-arc.cub", database, true, true);
    m_sprites[SpriteAntiqueruinEntranceStairs]->load(path + "antiqueruin-entrance-stairs.cub", database, true, true);
    m_sprites[SpriteAntiqueruinBase]->load(path + "antiqueruin-base.cub", database, true, true);
    m_sprites[SpriteAntiqueruinFloor]->load(path + "antiqueruin-floor.cub", database, true, true);
    m_sprites[SpriteAntiqueruinFloorStairs]->load(path + "antiqueruin-floor-stairs.cub", database, true, true);
    m_sprites[SpriteAntiqueruinWall]->load(path + "antiqueruin-wall.cub", database, true, true);
    m_sprites[SpriteAntiqueruinRoof1]->load(path + "antiqueruin-roof1.cub", database, true, true);
    m_sprites[SpriteAntiqueruinRoof2]->load(path + "antiqueruin-roof2.cub", database, true, true);
    m_sprites[SpriteAntiqueruinRoof3]->load(path + "antiqueruin-roof3.cub", database, true, true);
    m_sprites[SpriteAntiqueruinArc]->load(path + "antiqueruin-arc.cub", database, true, true);
    m_sprites[SpriteJungleruinEntranceStairs]->load(path + "jungleruin-entrance-stairs.cub", database, true, true);
    m_sprites[SpriteJungleruinBase]->load(path + "jungleruin-base.cub", database, true, true);
    m_sprites[SpriteJungleruinFloor]->load(path + "jungleruin-floor.cub", database, true, true);
    m_sprites[SpriteJungleruinFloorStairs]->load(path + "jungleruin-floor-stairs.cub", database, true, true);
    m_sprites[SpriteJungleruinWall]->load(path + "jungleruin-wall.cub", database, true, true);
    m_sprites[SpriteJungleruinWallDoor]->load(path + "jungleruin-wall-door.cub", database, true, true);
    m_sprites[SpriteJungleruinWallIndoor]->load(path + "jungleruin-wall-indoor.cub", database, true, true);
    m_sprites[SpriteJungleruinRoof1]->load(path + "jungleruin-roof1.cub", database, true, true);
    m_sprites[SpriteJungleruinRoof2]->load(path + "jungleruin-roof2.cub", database, true, true);
    m_sprites[SpriteJungleruinRoof3]->load(path + "jungleruin-roof3.cub", database, true, true);
    m_sprites[SpriteJungleruinArc]->load(path + "jungleruin-arc.cub", database, true, true);
    m_sprites[SpriteDesertruinEntranceStairs]->load(path + "desertruin-entrance-stairs.cub", database, true, true);
    m_sprites[SpriteDesertruinBase]->load(path + "desertruin-base.cub", database, true, true);
    m_sprites[SpriteDesertruinFloor]->load(path + "desertruin-floor.cub", database, true, true);
    m_sprites[SpriteDesertruinFloorStairs]->load(path + "desertruin-floor-stairs.cub", database, true, true);
    m_sprites[SpriteDesertruinWall]->load(path + "desertruin-wall.cub", database, true, true);
    m_sprites[SpriteDesertruinWallDoor]->load(path + "desertruin-wall-door.cub", database, true, true);
    m_sprites[SpriteDesertruinWallIndoor]->load(path + "desertruin-wall-indoor.cub", database, true, true);
    m_sprites[SpriteDesertruinRoof1]->load(path + "desertruin-roof1.cub", database, true, true);
    m_sprites[SpriteDesertruinRoof2]->load(path + "desertruin-roof2.cub", database, true, true);
    m_sprites[SpriteDesertruinRoof3]->load(path + "desertruin-roof3.cub", database, true, true);
    m_sprites[SpriteDesertruinArc]->load(path + "desertruin-arc.cub", database, true, true);
    m_sprites[SpriteDemonruinEntranceStairs]->load(path + "demonruin-entrance-stairs.cub", database, true, true);
    m_sprites[SpriteDemonruinBase]->load(path + "demonruin-base.cub", database, true, true);
    m_sprites[SpriteDemonruinFloor]->load(path + "demonruin-floor.cub", database, true, true);
    m_sprites[SpriteDemonruinFloorStairs]->load(path + "demonruin-floor-stairs.cub", database, true, true);
    m_sprites[SpriteDemonruinWall]->load(path + "demonruin-wall.cub", database, true, true);
    m_sprites[SpriteDemonruinWallDoor]->load(path + "demonruin-wall-door.cub", database, true, true);
    m_sprites[SpriteDemonruinWallIndoor]->load(path + "demonruin-wall-indoor.cub", database, true, true);
    m_sprites[SpriteDemonruinRoof1]->load(path + "demonruin-roof1.cub", database, true, true);
    m_sprites[SpriteDemonruinRoof2]->load(path + "demonruin-roof2.cub", database, true, true);
    m_sprites[SpriteDemonruinRoof3]->load(path + "demonruin-roof3.cub", database, true, true);
    m_sprites[SpriteDemonruinArc]->load(path + "demonruin-arc.cub", database, true, true);
    m_sprites[SpriteNordicruinEntranceStairs]->load(path + "nordicruin-entrance-stairs.cub", database, true, true);
    m_sprites[SpriteNordicruinBase]->load(path + "nordicruin-base.cub", database, true, true);
    m_sprites[SpriteNordicruinFloor]->load(path + "nordicruin-floor.cub", database, true, true);
    m_sprites[SpriteNordicruinFloorStairs]->load(path + "nordicruin-floor-stairs.cub", database, true, true);
    m_sprites[SpriteNordicruinWall]->load(path + "nordicruin-wall.cub", database, true, true);
    m_sprites[SpriteNordicruinWallDoor]->load(path + "nordicruin-wall-door.cub", database, true, true);
    m_sprites[SpriteNordicruinWallIndoor]->load(path + "nordicruin-wall-indoor.cub", database, true, true);
    m_sprites[SpriteNordicruinRoof1Slot3090]->load(path + "nordicruin-roof1.cub", database, true, true);
    m_sprites[SpriteNordicruinRoof1Slot3091]->load(path + "nordicruin-roof1.cub", database, true, true);
    m_sprites[SpriteNordicruinRoof1Slot3092]->load(path + "nordicruin-roof1.cub", database, true, true);
    m_sprites[SpriteNordicruinArc]->load(path + "nordicruin-arc.cub", database, true, true);
    m_sprites[SpriteAirship]->load(path + "airship.cub", database, true, false);

    // The red voxels of sprite 3506 are its marker positions: each is cleared and recorded in the sprite's position
    // map under key 0.
    for (int x = 0; x < m_sprites[SpriteAirship]->sizeX(); x++) {
        for (int y = 0; y < m_sprites[SpriteAirship]->sizeY(); y++) {
            for (int z = 0; z < m_sprites[SpriteAirship]->sizeZ(); z++) {
                if (m_sprites[SpriteAirship]->voxel(x, y, z) == plasma::Vector<3, unsigned char>(255, 0, 0)) {
                    m_sprites[SpriteAirship]->voxel(x, y, z) = plasma::Vector<3, unsigned char>(0, 0, 0);
                    m_sprites[SpriteAirship]->m_positionLists[0].push_back(plasma::Vector<3, int>(x, y, z));
                }
            }
        }
    }

    m_sprites[SpriteSteelEmpireAirship]->load(path + "steel-empire-airship.cub", database, true, false);
    m_sprites[SpriteSphinx01]->load(path + "sphinx01.cub", database, true, true);
    m_sprites[SpriteObelisk]->load(path + "obelisk.cub", database, true, true);
    m_sprites[SpriteManacube]->load(path + "manacube.cub", database, true, false);
    m_sprites[SpriteMaptile]->load(path + "maptile.cub", database, true, false);
    m_sprites[SpriteDewdrop]->load(path + "dewdrop.cub", database, true, false);
    m_sprites[SpriteSugarCube]->load(path + "sugar-cube.cub", database, true, false);
    m_sprites[SpriteLemon]->load(path + "lemon.cub", database, true, false);
    m_sprites[SpriteBanana]->load(path + "banana.cub", database, true, false);
    m_sprites[SpriteKale]->load(path + "kale.cub", database, true, false);
    m_sprites[SpriteCocoaBean]->load(path + "cocoa-bean.cub", database, true, false);
    m_sprites[SpriteApple]->load(path + "apple.cub", database, true, false);
    m_sprites[SpriteCloud02]->load(path + "cloud02.cub", database, true, false);

    // Sprite 3512 is recoloured: every voxel that is not black becomes (100, 130, 180), then its mesh is rebuilt.
    m_sprites[SpriteCloud02]->setFullIntensity(true);
    for (int x = 0; x < m_sprites[SpriteCloud02]->sizeX(); x++) {
        for (int y = 0; y < m_sprites[SpriteCloud02]->sizeY(); y++) {
            for (int z = 0; z < m_sprites[SpriteCloud02]->sizeZ(); z++) {
                if (m_sprites[SpriteCloud02]->voxel(x, y, z) != plasma::Vector<3, unsigned char>(0, 0, 0))
                    m_sprites[SpriteCloud02]->setVoxel(x, y, z, plasma::Vector<3, unsigned char>(100, 130, 180));
            }
        }
    }
    m_sprites[SpriteCloud02]->updateMesh(0);
    m_sprites[SpriteCloud02]->finalize();

    m_sprites[SpriteCloudDark]->setTint(plasma::Vector<3, unsigned char>(10, 0, 20));
    m_sprites[SpriteCloudDark]->load(path + "cloud-dark.cub", database, true, false);
    m_sprites[SpriteHatch]->load(path + "hatch.cub", database, true, false);
    m_sprites[SpriteDemonPortal]->load(path + "demon-portal.cub", database, true, false);
    m_sprites[SpriteSlimewell]->load(path + "slimewell.cub", database, true, false);
    m_sprites[SpriteHornetsNest]->load(path + "hornets-nest.cub", database, true, false);
    m_sprites[SpriteUnholyTomb]->load(path + "unholy-tomb.cub", database, true, false);
    m_sprites[SpriteBonePile]->load(path + "bone-pile.cub", database, true, false);
    m_sprites[SpriteMolehill]->load(path + "molehill.cub", database, true, false);
    m_sprites[SpriteEarthPile]->load(path + "earth-pile.cub", database, true, false);
    m_sprites[SpriteQuestTarget]->load(path + "quest-target.cub", database, true, false);
    m_sprites[SpriteSign]->load(path + "sign.cub", database, true, false);
    m_sprites[SpriteSwimmingSign]->load(path + "swimming-sign.cub", database, true, false);
    m_sprites[SpriteCageBase]->load(path + "cage-base.cub", database, true, false);
    m_sprites[SpriteCageDoor]->load(path + "cage-door.cub", database, true, false);
    m_sprites[SpriteCage2Base]->load(path + "cage2-base.cub", database, true, false);
    m_sprites[SpriteCage2Door]->load(path + "cage2-door.cub", database, true, false);
    m_sprites[SpriteCage3Base]->load(path + "cage3-base.cub", database, true, false);
    m_sprites[SpriteCage3Door]->load(path + "cage3-door.cub", database, true, false);
    m_sprites[SpriteCage4Base]->load(path + "cage4-base.cub", database, true, false);
    m_sprites[SpriteCage4Door]->load(path + "cage4-door.cub", database, true, false);
    m_sprites[SpriteCauldron]->load(path + "cauldron.cub", database, true, false);
    m_sprites[SpriteBrazier]->load(path + "brazier.cub", database, true, false);
    m_sprites[SpriteWagon1]->load(path + "wagon1.cub", database, true, false);
    m_sprites[SpriteWagon2]->load(path + "wagon2.cub", database, true, false);
    m_sprites[SpriteWitchWagon]->load(path + "witch-wagon.cub", database, true, false);
    m_sprites[SpriteRuneCube]->load(path + "rune-cube.cub", database, true, false);
    m_sprites[SpriteRuneCubeGlow]->load(path + "rune-cube-glow.cub", database, false, false);
    m_sprites[SpriteBloodaxeClanBanner]->load(path + "bloodaxe-clan-banner.cub", database, true, false);
    m_sprites[SpriteGoldKnightBanner]->load(path + "gold-knight-banner.cub", database, true, false);
    m_sprites[SpriteGoldKnightWallBanner]->load(path + "gold-knight-wall-banner.cub", database, true, false);
    m_sprites[SpriteGoldKnightBox]->load(path + "gold-knight-box.cub", database, true, false);
    m_sprites[SpriteSoldierTent]->load(path + "soldier-tent.cub", database, true, false);
    m_sprites[SpriteHorseTrough1]->load(path + "horse-trough1.cub", database, true, false);
    m_sprites[SpriteHorseTrough2]->load(path + "horse-trough2.cub", database, true, false);
    m_sprites[SpriteHorseTrough3]->load(path + "horse-trough3.cub", database, true, false);
    m_sprites[SpriteTombstone]->load(path + "tombstone.cub", database, true, false);
    m_sprites[SpriteManaPump]->load(path + "mana-pump.cub", database, true, false);
    m_sprites[SpriteManaInductor]->load(path + "mana-inductor.cub", database, true, false);
    m_sprites[SpriteManaGenerator]->load(path + "mana-generator.cub", database, true, false);
    m_sprites[SpriteParachute]->load(path + "parachute.cub", database, true, false);
    m_sprites[SpriteEmoteQuestion]->load(path + "emote-question.cub", database, true, false);
    m_sprites[SpriteEmoteAlert]->load(path + "emote-alert.cub", database, true, false);
    m_sprites[SpriteAngry]->load(path + "angry.cub", database, true, false);
    m_sprites[SpriteAngry2]->load(path + "angry2.cub", database, true, false);
    m_sprites[SpriteHeartSlot3568]->load(path + "heart.cub", database, true, false);
    m_sprites[SpriteEmoteSweat]->load(path + "emote-sweat.cub", database, true, false);
    m_sprites[SpriteEmoteIdea]->load(path + "emote-idea.cub", database, true, false);
    m_sprites[SpriteManaMechBody]->load(path + "mana-mech-body.cub", database, true, false);
    m_sprites[SpriteManaMechHand]->load(path + "mana-mech-hand.cub", database, true, false);
    m_sprites[SpriteManaMechFoot]->load(path + "mana-mech-foot.cub", database, true, false);
    m_sprites[SpriteTreantBody]->load(path + "treant-body.cub", database, true, false);
    m_sprites[SpriteTreantHand]->load(path + "treant-hand.cub", database, true, false);
    m_sprites[SpriteTreantFoot]->load(path + "treant-foot.cub", database, true, false);
    m_sprites[SpriteHumanEyePatch]->load(path + "human-eye-patch.cub", database, false, false);
    m_sprites[SpriteFrogmanEyePatch]->load(path + "frogman-eye-patch.cub", database, false, false);
    m_sprites[SpriteLizardmanEyePatch]->load(path + "lizardman-eye-patch.cub", database, false, false);
    m_sprites[SpriteGoblinFemaleEyePatch]->load(path + "goblin-female-eye-patch.cub", database, false, false);
    m_sprites[SpriteGoblinMaleEyePatch]->load(path + "goblin-male-eye-patch.cub", database, false, false);
    m_sprites[SpriteElfEyePatch]->load(path + "elf-eye-patch.cub", database, false, false);
    m_sprites[SpriteDwarfFemaleEyePatch]->load(path + "dwarf-female-eye-patch.cub", database, false, false);
    m_sprites[SpriteDwarfMaleEyePatch]->load(path + "dwarf-male-eye-patch.cub", database, false, false);
    m_sprites[SpriteRunestoneGlow]->load(path + "runestone-glow.cub", database, true, false);
    m_sprites[SpriteFireRunestone]->load(path + "fire-runestone.cub", database, true, false);
    m_sprites[SpriteWaterRunestone]->load(path + "water-runestone.cub", database, true, false);
    m_sprites[SpriteIceRunestone]->load(path + "ice-runestone.cub", database, true, false);
    m_sprites[SpriteWindRunestone]->load(path + "wind-runestone.cub", database, true, false);
    m_sprites[SpriteRunePortal]->load(path + "rune-portal.cub", database, true, false);
    m_sprites[SpriteRunePortalFeature]->load(path + "rune-portal-feature.cub", database, true, false);
    m_sprites[SpriteMonstrosityHead]->load(path + "monstrosity-head.cub", database, true, false);
    m_sprites[SpriteMonstrosityHeadFeature]->load(path + "monstrosity-head-feature.cub", database, true, false);
    m_sprites[SpriteMonstrosityBody]->load(path + "monstrosity-body.cub", database, true, false);
    m_sprites[SpriteMonstrosityHand]->load(path + "monstrosity-hand.cub", database, true, false);
    m_sprites[SpriteMonstrosityFoot]->load(path + "monstrosity-foot.cub", database, true, false);
    m_sprites[SpriteSlaterBody2]->load(path + "slater-body2.cub", database, true, false);
    m_sprites[SpriteSlaterHead2]->load(path + "slater-head2.cub", database, true, false);
    m_sprites[SpriteTortureRack]->load(path + "torture-rack.cub", database, true, false);
    m_sprites[SpriteLegIrons]->load(path + "leg-irons.cub", database, true, false);
    m_sprites[SpritePole]->load(path + "pole.cub", database, true, false);
    m_sprites[SpriteCeilingChain]->load(path + "ceiling-chain.cub", database, true, false);
    m_sprites[SpriteWraithHead]->load(path + "wraith-head.cub", database, true, false);
    m_sprites[SpriteWraithEyes]->load(path + "wraith-eyes.cub", database, true, false);
    m_sprites[SpriteWraithHand]->load(path + "wraith-hand.cub", database, true, false);
    m_sprites[SpriteShrineOfTheLight]->load(path + "shrine-of-the-light.cub", nullptr, true, false);
    m_sprites[SpritePirateBoat]->load(path + "pirate-boat.cub", nullptr, true, false);
    m_sprites[SpriteBarrelTnt]->load(path + "barrel-tnt.cub", nullptr, true, false);
    m_sprites[SpriteDruidLight]->load(path + "druid-light.cub", nullptr, true, false);
    m_sprites[SpriteDruidLight2]->load(path + "druid-light2.cub", nullptr, true, false);
    m_sprites[SpriteDruidLight3]->load(path + "druid-light3.cub", nullptr, true, false);
    m_sprites[SpriteDruidChair]->load(path + "druid-chair.cub", nullptr, true, false);
    m_sprites[SpriteFootprints]->load(path + "footprints.cub", nullptr, true, false);
    m_sprites[SpriteHorseFootprints]->load(path + "horse-footprints.cub", nullptr, true, false);
    m_sprites[SpriteMonsterFootprints]->load(path + "monster-footprints.cub", nullptr, true, false);
    m_sprites[SpriteWaterBubble]->setTint(plasma::Vector<3, unsigned char>(0, 100, 200));
    m_sprites[SpriteWaterBubble]->load(path + "water-bubble.cub", nullptr, true, false);
    m_sprites[SpriteRockFist]->load(path + "rock-fist.cub", nullptr, true, false);
    m_sprites[SpriteSarcophagus]->load(path + "sarcophagus.cub", nullptr, true, false);
    m_sprites[SpriteSarcophagusTop]->load(path + "sarcophagus-top.cub", nullptr, true, false);
    m_sprites[SpriteMagnifier]->load(path + "magnifier.cub", database, true, false);
    m_sprites[SpriteTileEntrance1]->load(path + "tile-entrance1.cub", database, true, true);
    m_sprites[SpriteTileRoom1]->load(path + "tile-room1.cub", database, true, true);
    m_sprites[SpriteTileEnd1]->load(path + "tile-end1.cub", database, true, true);
    m_sprites[SpriteWallIvy1]->load(path + "wall-ivy1.cub", database, true, false);
    m_sprites[SpriteWallIvy2]->load(path + "wall-ivy2.cub", database, true, false);
    m_sprites[SpriteWallIvy3]->load(path + "wall-ivy3.cub", database, true, false);
    m_sprites[SpriteWallIvy4]->load(path + "wall-ivy4.cub", database, true, false);
    m_sprites[SpriteBirdStatue]->load(path + "bird-statue.cub", nullptr, true, false);
    m_sprites[SpriteTinyBird1]->load(path + "tiny-bird1.cub", nullptr, true, false);
    m_sprites[SpriteTinyBird2]->load(path + "tiny-bird2.cub", nullptr, true, false);
    m_sprites[SpriteMourningStatue]->load(path + "mourning-statue.cub", nullptr, true, false);
    m_sprites[SpriteClimbingPlantStalk]->load(path + "climbing-plant-stalk.cub", nullptr, true, false);
    m_sprites[SpriteClimbingPlantSoil]->load(path + "climbing-plant-soil.cub", nullptr, true, false);
    m_sprites[SpriteEagle]->load(path + "eagle.cub", nullptr, true, false);
    m_sprites[SpriteEagleSitting]->load(path + "eagle-sitting.cub", nullptr, true, false);
    m_sprites[SpriteLifeFlask]->load(path + "life-flask.cub", nullptr, true, false);
    m_sprites[SpriteAttackFlask]->load(path + "attack-flask.cub", nullptr, true, false);
    m_sprites[SpriteArmorFlask]->load(path + "armor-flask.cub", nullptr, true, false);
    m_sprites[SpriteResistanceFlask]->load(path + "resistance-flask.cub", nullptr, true, false);
    m_sprites[SpriteLemonade]->load(path + "lemonade.cub", nullptr, true, false);
    m_sprites[SpriteHotChocolate]->load(path + "hot-chocolate.cub", nullptr, true, false);
    m_sprites[SpriteGreenSmoothie]->load(path + "green-smoothie.cub", nullptr, true, false);
    m_sprites[SpriteGnomeHead]->load(path + "gnome-head.cub", nullptr, true, false);
    m_sprites[SpriteGnomeBody]->load(path + "gnome-body.cub", nullptr, true, false);
    m_sprites[SpriteGnomeFoot]->load(path + "gnome-foot.cub", nullptr, true, false);
    m_sprites[SpriteSupplierGnomeHead]->load(path + "supplier-gnome-head.cub", nullptr, true, false);
    m_sprites[SpriteSupplierGnomeBody]->load(path + "supplier-gnome-body.cub", nullptr, true, false);
    m_sprites[SpriteDragonRoot]->load(path + "dragon-root.cub", nullptr, true, false);

    m_environmentSprites.resize(100, nullptr);
    m_environmentSprites[PlantFlowers] = sprite(SpriteFlowers);
    m_environmentSprites[PlantFlowers2] = sprite(SpriteFlowers2);
    m_environmentSprites[PlantFlowers3] = sprite(SpriteFlowers3);
    m_environmentSprites[PlantFlowers4] = sprite(SpriteFlowers4);
    m_environmentSprites[PlantGrass] = sprite(SpriteGrass);
    m_environmentSprites[PlantGrass2] = sprite(SpriteGrass2);
    m_environmentSprites[PlantGrass3] = sprite(SpriteGrass3);
    m_environmentSprites[PlantGrassDry1] = sprite(SpriteGrassDry1);
    m_environmentSprites[PlantGrassDry2] = sprite(SpriteGrassDry2);
    m_environmentSprites[PlantGrassDark1] = sprite(SpriteGrassDark1);
    m_environmentSprites[PlantGrassDark2] = sprite(SpriteGrassDark2);
    m_environmentSprites[PlantSavannahPlant1] = sprite(SpriteSavannahPlant1);
    m_environmentSprites[PlantSavannahPlant2] = sprite(SpriteSavannahPlant2);
    m_environmentSprites[PlantDarkPlant1] = sprite(SpriteDarkPlant1);
    m_environmentSprites[PlantDarkPlant2] = sprite(SpriteDarkPlant2);
    m_environmentSprites[PlantCoral] = sprite(SpriteCoral);
    m_environmentSprites[PlantUnderwaterPlant] = sprite(SpriteUnderwaterPlant);
    m_environmentSprites[PlantAlga] = sprite(SpriteAlga);
    m_environmentSprites[PlantLavaFlower] = sprite(SpriteLavaFlower);
    m_environmentSprites[PlantLavaGrass] = sprite(SpriteLavaGrass);
    m_environmentSprites[PlantThornPlant] = sprite(SpriteThornPlant);
    m_environmentSprites[PlantEchinacea2] = sprite(SpriteEchinacea2);
    m_environmentSprites[PlantLeaf] = sprite(SpriteLeaf);
    m_environmentSprites[PlantLantern02] = sprite(SpriteLantern02);
    m_environmentSprites[PlantWallLantern] = sprite(SpriteWallLantern);
    m_environmentSprites[PlantGreenWallLantern] = sprite(SpriteGreenWallLantern);
    m_environmentSprites[PlantLanternGreen] = sprite(SpriteLanternGreen);
    m_environmentSprites[PlantTorch] = sprite(SpriteTorchSlot3342);
    m_environmentSprites[PlantStone] = sprite(SpriteStone);
    m_environmentSprites[PlantStone2] = sprite(SpriteStone2);
    m_environmentSprites[PlantTendril] = sprite(SpriteTendril);
    m_environmentSprites[PlantTulipsColorful] = sprite(SpriteTulipsColorful);
    m_environmentSprites[PlantCornflower] = sprite(SpriteCornflower);
    m_environmentSprites[PlantReed] = sprite(SpriteReed);
    m_environmentSprites[PlantPumpkinLeaves] = sprite(SpritePumpkinLeaves);
    m_environmentSprites[PlantPineappleLeaves] = sprite(SpritePineappleLeaves);
    m_environmentSprites[PlantSunflower] = sprite(SpriteSunflower);
    m_environmentSprites[PlantBeanTendril] = sprite(SpriteBeanTendril);
    m_environmentSprites[PlantDesertFlower01] = sprite(SpriteDesertFlower01);
    m_environmentSprites[PlantDesertFlower02] = sprite(SpriteDesertFlower02);
    m_environmentSprites[PlantWheat] = sprite(SpriteWheat);
    m_environmentSprites[PlantCorn] = sprite(SpriteCorn);
    m_environmentSprites[PlantWaterLily01] = sprite(SpriteWaterLily01);
    m_environmentSprites[PlantWaterLily02] = sprite(SpriteWaterLily02);
    m_environmentSprites[PlantInnSign] = sprite(SpriteInnSign);
    m_environmentSprites[PlantIdentifierSign] = sprite(SpriteIdentifierSign);
    m_environmentSprites[PlantShopSign] = sprite(SpriteShopSign);
    m_environmentSprites[PlantWeaponShopSign] = sprite(SpriteWeaponShopSign);
    m_environmentSprites[PlantArmorShopSign] = sprite(SpriteArmorShopSign);
    m_environmentSprites[PlantSmithySign] = sprite(SpriteSmithySign);
    m_environmentSprites[PlantCarpentersshopSign] = sprite(SpriteCarpentersshopSign);
    m_environmentSprites[PlantWeavingmillSign] = sprite(SpriteWeavingmillSign);
    m_environmentSprites[PlantChristmasTree] = sprite(SpriteChristmasTree);
    m_environmentSprites[PlantIvy] = sprite(SpriteIvy);
    m_environmentSprites[PlantWallRosesRed] = sprite(SpriteWallRosesRed);
    m_environmentSprites[PlantWallRosesWhite] = sprite(SpriteWallRosesWhite);
    m_environmentSprites[PlantIncaArt1] = sprite(SpriteIncaArt1);
    m_environmentSprites[PlantIncaArt2] = sprite(SpriteIncaArt2);
    m_environmentSprites[PlantIncaArt3] = sprite(SpriteIncaArt3);
    m_environmentSprites[PlantIncaArt4] = sprite(SpriteIncaArt4);
    m_environmentSprites[PlantCrest1] = sprite(SpriteCrest1);
    m_environmentSprites[PlantCastleChain] = sprite(SpriteCastleChain);
    m_environmentSprites[PlantWallSkull] = sprite(SpriteWallSkull);
    m_environmentSprites[PlantTorchYellow] = sprite(SpriteTorchYellow);
    m_environmentSprites[PlantTorchGreenCyanFlame] = sprite(SpriteTorchYellow);
    m_environmentSprites[PlantTorchBlueCyanFlame] = sprite(SpriteTorchYellow);
    m_environmentSprites[PlantTorchRedYellowFlame] = sprite(SpriteTorchYellow);
    m_environmentSprites[PlantLiana] = sprite(SpriteLiana);
    m_environmentSprites[PlantChandelier] = sprite(SpriteChandelier);
    m_environmentSprites[PlantCobwebs] = sprite(SpriteCobwebs);
    m_environmentSprites[PlantCobwebs2] = sprite(SpriteCobwebs2);
    m_environmentSprites[PlantSkullRotten] = sprite(SpriteSkullRotten);
    m_environmentSprites[PlantMoss1] = sprite(SpriteMoss1);
    m_environmentSprites[PlantMoss2] = sprite(SpriteMoss2);
    m_environmentSprites[PlantGoldKnightWallBanner] = sprite(SpriteGoldKnightWallBanner);
    m_environmentSprites[PlantWallIvy1] = sprite(SpriteWallIvy1);
    m_environmentSprites[PlantWallIvy2] = sprite(SpriteWallIvy2);
    m_environmentSprites[PlantWallIvy3] = sprite(SpriteWallIvy3);
    m_environmentSprites[PlantWallIvy4] = sprite(SpriteWallIvy4);
    m_environmentSprites[PlantPainting01] = sprite(SpritePainting01);
    m_environmentSprites[PlantPainting02] = sprite(SpritePainting02);
    m_environmentSprites[PlantPainting03] = sprite(SpritePainting03);
    m_environmentSprites[PlantPainting04] = sprite(SpritePainting04);
    m_environmentSprites[PlantStreetPennon1] = sprite(SpriteStreetPennon1);
    m_environmentSprites[PlantStreetPennon2] = sprite(SpriteStreetPennon2);
    m_environmentSprites[PlantStreetPennon3] = sprite(SpriteStreetPennon3);
    m_environmentSprites[PlantStreetPennon4] = sprite(SpriteStreetPennon4);
    m_environmentSprites[PlantStreetPennon5] = sprite(SpriteStreetPennon5);
    m_environmentSprites[PlantGateBanner1] = sprite(SpriteGateBanner1);
    m_environmentSprites[PlantGateBanner2] = sprite(SpriteGateBanner2);
    m_environmentSprites[PlantGateBanner3] = sprite(SpriteGateBanner3);
    m_environmentSprites[PlantGateBanner4] = sprite(SpriteGateBanner4);
    m_environmentSprites[PlantGateBanner5] = sprite(SpriteGateBanner5);

    m_staticEntitySprites.resize(134, nullptr);
    m_staticEntitySprites[StaticEntityGoddess2] = sprite(SpriteGoddess2Slot3405);
    m_staticEntitySprites[StaticEntityBirdStatue] = sprite(SpriteBirdStatue);
    m_staticEntitySprites[StaticEntityMourningStatue] = sprite(SpriteMourningStatue);
    m_staticEntitySprites[StaticEntityDoor] = sprite(SpriteDoorSlot3406);
    m_staticEntitySprites[StaticEntityBigDoor] = sprite(SpriteBigDoor);
    m_staticEntitySprites[StaticEntityBigIronDoor] = sprite(SpriteBigIronDoor);
    m_staticEntitySprites[StaticEntityWindow] = sprite(SpriteWindowSlot3409);
    m_staticEntitySprites[StaticEntityCastleWindow] = sprite(SpriteCastleWindow);
    m_staticEntitySprites[StaticEntityGate] = sprite(SpriteGate);
    m_staticEntitySprites[StaticEntityLockedGate] = sprite(SpriteLockedGate);
    m_staticEntitySprites[StaticEntityRuneGate] = sprite(SpriteRuneGate);
    m_staticEntitySprites[StaticEntityStoneGate] = sprite(SpriteStoneGate);
    m_staticEntitySprites[StaticEntityStoneWall] = sprite(SpriteStoneWall);
    m_staticEntitySprites[StaticEntitySpiderwebWall] = sprite(SpriteSpiderwebWall);
    m_staticEntitySprites[StaticEntityMagicBarrier] = sprite(SpriteMagicBarrier);
    m_staticEntitySprites[StaticEntityGrid] = sprite(SpriteGrid);
    m_staticEntitySprites[StaticEntityGoldenGate] = sprite(SpriteGoldenGate);
    m_staticEntitySprites[StaticEntitySpikeTrap] = sprite(SpriteSpikeTrap);
    m_staticEntitySprites[StaticEntityStompTrap] = sprite(SpriteStompTrap);
    m_staticEntitySprites[StaticEntityLever] = sprite(SpriteLeverSlot3414);
    m_staticEntitySprites[StaticEntityChestBase02] = sprite(SpriteChestBase02);
    m_staticEntitySprites[StaticEntityRewardChest] = sprite(SpriteChestBase02);
    m_staticEntitySprites[StaticEntityChestTop02] = sprite(SpriteChestTop02);
    m_staticEntitySprites[StaticEntityTable] = sprite(SpriteTable);
    m_staticEntitySprites[StaticEntityStoneTable] = sprite(SpriteStoneTable);
    m_staticEntitySprites[StaticEntityStoneStool] = sprite(SpriteStoneStool);
    m_staticEntitySprites[StaticEntityStool] = sprite(SpriteStool);
    m_staticEntitySprites[StaticEntitySandstoneStool] = sprite(SpriteSandstoneStool);
    m_staticEntitySprites[StaticEntityBench] = sprite(SpriteBench);
    m_staticEntitySprites[StaticEntityBenchBlack] = sprite(SpriteBenchBlack);
    m_staticEntitySprites[StaticEntityBed] = sprite(SpriteBed);
    m_staticEntitySprites[StaticEntityBedtable] = sprite(SpriteBedtable);
    m_staticEntitySprites[StaticEntityMarketStand1] = sprite(SpriteMarketStand1);
    m_staticEntitySprites[StaticEntityMarketStand2] = sprite(SpriteMarketStand2);
    m_staticEntitySprites[StaticEntityMarketStand3] = sprite(SpriteMarketStand3);
    m_staticEntitySprites[StaticEntityBarrel] = sprite(SpriteBarrel);
    m_staticEntitySprites[StaticEntityCrate] = sprite(SpriteCrate);
    m_staticEntitySprites[StaticEntityOpenCrate] = sprite(SpriteOpenCrate);
    m_staticEntitySprites[StaticEntitySack] = sprite(SpriteSack);
    m_staticEntitySprites[StaticEntityShelter] = sprite(SpriteShelter);
    m_staticEntitySprites[StaticEntityCupboard] = sprite(SpriteCupboard);
    m_staticEntitySprites[StaticEntityDesktop] = sprite(SpriteDesktop);
    m_staticEntitySprites[StaticEntityCounter] = sprite(SpriteCounter);
    m_staticEntitySprites[StaticEntityShelf1] = sprite(SpriteShelf1);
    m_staticEntitySprites[StaticEntityShelf2] = sprite(SpriteShelf2);
    m_staticEntitySprites[StaticEntityShelf3] = sprite(SpriteShelf3);
    m_staticEntitySprites[StaticEntityCastleShelf1] = sprite(SpriteCastleShelf1);
    m_staticEntitySprites[StaticEntityCastleShelf2] = sprite(SpriteCastleShelf2);
    m_staticEntitySprites[StaticEntityCastleShelf3] = sprite(SpriteCastleShelf3);
    m_staticEntitySprites[StaticEntityStoneShelf1] = sprite(SpriteStoneShelf1);
    m_staticEntitySprites[StaticEntityStoneShelf2] = sprite(SpriteStoneShelf2);
    m_staticEntitySprites[StaticEntityStoneShelf3] = sprite(SpriteStoneShelf3);
    m_staticEntitySprites[StaticEntitySandstoneShelf1] = sprite(SpriteSandstoneShelf1);
    m_staticEntitySprites[StaticEntitySandstoneShelf2] = sprite(SpriteSandstoneShelf2);
    m_staticEntitySprites[StaticEntitySandstoneShelf3] = sprite(SpriteSandstoneShelf3);
    m_staticEntitySprites[StaticEntityCorpse] = sprite(SpriteCorpse);
    m_staticEntitySprites[StaticEntityRunestone] = sprite(SpriteRunestone);
    m_staticEntitySprites[StaticEntityArtifact] = sprite(SpriteArtifact);
    m_staticEntitySprites[StaticEntityFlowerBox01] = sprite(SpriteFlowerBox01);
    m_staticEntitySprites[StaticEntityFlowerBox02] = sprite(SpriteFlowerBox02);
    m_staticEntitySprites[StaticEntityFlowerBox03] = sprite(SpriteFlowerBox03);
    m_staticEntitySprites[StaticEntityStreetLight02] = sprite(SpriteStreetLight02);
    m_staticEntitySprites[StaticEntityStreetLight] = sprite(SpriteStreetLight);
    m_staticEntitySprites[StaticEntityStreetLight01] = sprite(SpriteStreetLight01);
    m_staticEntitySprites[StaticEntityStreetLightUndead] = sprite(SpriteStreetLightUndead);
    m_staticEntitySprites[StaticEntityFlowerVase1] = sprite(SpriteFlowerVase1);
    m_staticEntitySprites[StaticEntityFlowerVase2] = sprite(SpriteFlowerVase2);
    m_staticEntitySprites[StaticEntityFlowerVase3] = sprite(SpriteFlowerVase3);
    m_staticEntitySprites[StaticEntityFlowerVase4] = sprite(SpriteFlowerVase4);
    m_staticEntitySprites[StaticEntityFlowerVase5] = sprite(SpriteFlowerVase5);
    m_staticEntitySprites[StaticEntityFlowerVase6] = sprite(SpriteFlowerVase6);
    m_staticEntitySprites[StaticEntityFlowerVase7] = sprite(SpriteFlowerVase7);
    m_staticEntitySprites[StaticEntityFlowerVase8] = sprite(SpriteFlowerVase8);
    m_staticEntitySprites[StaticEntityFlowerVase9] = sprite(SpriteFlowerVase9);
    m_staticEntitySprites[StaticEntityFence01] = sprite(SpriteFence01);
    m_staticEntitySprites[StaticEntityFence02] = sprite(SpriteFence02);
    m_staticEntitySprites[StaticEntityFence03] = sprite(SpriteFence03);
    m_staticEntitySprites[StaticEntityFence04] = sprite(SpriteFence04);
    m_staticEntitySprites[StaticEntityFenceIron1] = sprite(SpriteFenceIron1);
    m_staticEntitySprites[StaticEntityFenceIron2] = sprite(SpriteFenceIron2);
    m_staticEntitySprites[StaticEntityFenceIron3] = sprite(SpriteFenceIron3);
    m_staticEntitySprites[StaticEntityFurnace] = sprite(SpriteFurnace);
    m_staticEntitySprites[StaticEntityAnvil] = sprite(SpriteAnvil);
    m_staticEntitySprites[StaticEntitySawbench] = sprite(SpriteSawbench);
    m_staticEntitySprites[StaticEntityWorkbench] = sprite(SpriteWorkbench);
    m_staticEntitySprites[StaticEntityCustomizationBench] = sprite(SpriteCustomizationBench);
    m_staticEntitySprites[StaticEntitySpinningwheel] = sprite(SpriteSpinningwheel);
    m_staticEntitySprites[StaticEntityLoom] = sprite(SpriteLoom);
    m_staticEntitySprites[StaticEntityEagleSeat] = sprite(SpriteEagleSeat);
    m_staticEntitySprites[StaticEntityPedestal] = sprite(SpritePedestal);
    m_staticEntitySprites[StaticEntityStoneTablet1] = sprite(SpriteStoneTablet1);
    m_staticEntitySprites[StaticEntityStoneTablet2] = sprite(SpriteStoneTablet2);
    m_staticEntitySprites[StaticEntityStoneTablet3] = sprite(SpriteStoneTablet3);
    m_staticEntitySprites[StaticEntityStoneTablet4] = sprite(SpriteStoneTablet4);
    m_staticEntitySprites[StaticEntityStoneTablet5] = sprite(SpriteStoneTablet5);
    m_staticEntitySprites[StaticEntityStoneTablet6] = sprite(SpriteStoneTablet6);
    m_staticEntitySprites[StaticEntityInscribedTablet] = sprite(SpriteInscribedTablet);
    m_staticEntitySprites[StaticEntityBrazier] = sprite(SpriteBrazier);
    m_staticEntitySprites[StaticEntityCampfire] = sprite(SpriteCampfire);
    m_staticEntitySprites[StaticEntityCageBase] = sprite(SpriteCageBase);
    m_staticEntitySprites[StaticEntityCage2Base] = sprite(SpriteCage2Base);
    m_staticEntitySprites[StaticEntityCage3Base] = sprite(SpriteCage3Base);
    m_staticEntitySprites[StaticEntityCage4Base] = sprite(SpriteCage4Base);
    m_staticEntitySprites[StaticEntityCauldron] = sprite(SpriteCauldron);
    m_staticEntitySprites[StaticEntityTortureRack] = sprite(SpriteTortureRack);
    m_staticEntitySprites[StaticEntityLegIrons] = sprite(SpriteLegIrons);
    m_staticEntitySprites[StaticEntityPole] = sprite(SpritePole);
    m_staticEntitySprites[StaticEntityCeilingChain] = sprite(SpriteCeilingChain);
    m_staticEntitySprites[StaticEntityTent] = sprite(SpriteTent);
    m_staticEntitySprites[StaticEntityBeachUmbrella] = sprite(SpriteBeachUmbrella);
    m_staticEntitySprites[StaticEntityBeachTowel] = sprite(SpriteBeachTowel);
    m_staticEntitySprites[StaticEntityWoodMat] = sprite(SpriteWoodMat);
    m_staticEntitySprites[StaticEntityHatch] = sprite(SpriteHatch);
    m_staticEntitySprites[StaticEntityGoldKnightBanner] = sprite(SpriteGoldKnightBanner);
    m_staticEntitySprites[StaticEntityGoldKnightBox] = sprite(SpriteGoldKnightBox);
    m_staticEntitySprites[StaticEntitySoldierTent] = sprite(SpriteSoldierTent);
    m_staticEntitySprites[StaticEntityHorseTrough1] = sprite(SpriteHorseTrough1);
    m_staticEntitySprites[StaticEntityHorseTrough2] = sprite(SpriteHorseTrough2);
    m_staticEntitySprites[StaticEntityHorseTrough3] = sprite(SpriteHorseTrough3);
    m_staticEntitySprites[StaticEntityDruidLight] = sprite(SpriteDruidLight);
    m_staticEntitySprites[StaticEntityDruidLight2] = sprite(SpriteDruidLight2);
    m_staticEntitySprites[StaticEntityDruidLight3] = sprite(SpriteDruidLight3);
    m_staticEntitySprites[StaticEntityDruidChair] = sprite(SpriteDruidChair);
    m_staticEntitySprites[StaticEntityBigPumpkin] = sprite(SpriteBigPumpkin);
    m_staticEntitySprites[StaticEntityGraveStone1] = sprite(SpriteGraveStone1);
    m_staticEntitySprites[StaticEntityGraveStone2] = sprite(SpriteGraveStone2);
    m_staticEntitySprites[StaticEntityGraveStone3] = sprite(SpriteGraveStone3);
    m_staticEntitySprites[StaticEntitySarcophagus] = sprite(SpriteSarcophagus);
    m_staticEntitySprites[StaticEntityWagon1] = sprite(SpriteWagon1);
    m_staticEntitySprites[StaticEntityWagon2] = sprite(SpriteWagon2);
    m_staticEntitySprites[StaticEntityWitchWagon] = sprite(SpriteWitchWagon);

    m_staticEntityDoorAndLidSprites.resize(134, nullptr);
    m_staticEntityDoorAndLidSprites[StaticEntityCageBase] = sprite(SpriteCageDoor);
    m_staticEntityDoorAndLidSprites[StaticEntityCage2Base] = sprite(SpriteCage2Door);
    m_staticEntityDoorAndLidSprites[StaticEntityCage3Base] = sprite(SpriteCage3Door);
    m_staticEntityDoorAndLidSprites[StaticEntityCage4Base] = sprite(SpriteCage4Door);
    m_staticEntityDoorAndLidSprites[StaticEntitySarcophagus] = sprite(SpriteSarcophagusTop);

    if (database)
        delete database;
}

Sprite *SpriteManager::objectSprite(int index)
{
    if (index < 0 || index >= m_staticEntityDoorAndLidSprites.size())
        return nullptr;
    return m_staticEntityDoorAndLidSprites[index];
}

Sprite *SpriteManager::partSprite(int index)
{
    if (index < 0 || index >= m_staticEntitySprites.size())
        return nullptr;
    return m_staticEntitySprites[index];
}

// The index in m_sprites of the model of a weapon (item category 3): most kinds have 11 styles (chosen by the
// modifier) of 5 rarity tiers, and single models for some materials.
int weaponSpriteIndex(int id, unsigned int modifier, int rarity, int material)
{
    switch (id) {
    case WeaponIronPickaxe:
        return SpriteIronPickaxe;
    case WeaponStaff:
        switch (material) {
        case ItemMaterialSteel:
            return SpriteSteelStaff;
        case ItemMaterialObsidian:
            return SpriteObsidianStaff1 + std::max(0, std::min(rarity, (int)ItemRarityYellow));
        case ItemMaterialDruid:
            return SpriteDruidStaff;
        default:
            return SpriteWoodStaff1 + modifier % 11 * ItemRarityCount +
                   std::max(0, std::min(rarity, (int)ItemRarityYellow));
        }
    case WeaponWand:
        return SpriteWoodWand1 + modifier % 11 * ItemRarityCount + std::max(0, std::min(rarity, (int)ItemRarityYellow));
    case WeaponBracelet:
        if (material == ItemMaterialGold)
            return SpriteGoldBracelet1 + modifier % 6 * ItemRarityCount +
                   std::max(0, std::min(rarity, (int)ItemRarityYellow));
        return SpriteSilverBracelet1 + modifier % 6 * ItemRarityCount +
               std::max(0, std::min(rarity, (int)ItemRarityYellow));
    case WeaponSword:
        switch (material) {
        case ItemMaterialObsidian:
            return SpriteObsidianSword;
        case ItemMaterialBone:
            return SpriteBoneSword;
        default:
            return SpriteIronSword1 + modifier % 11 * ItemRarityCount +
                   std::max(0, std::min(rarity, (int)ItemRarityYellow));
        }
    case WeaponDagger:
        return SpriteIronDagger1 + modifier % 11 * ItemRarityCount +
               std::max(0, std::min(rarity, (int)ItemRarityYellow));
    case WeaponFist:
        return SpriteIronFist1 + modifier % 11 * ItemRarityCount + std::max(0, std::min(rarity, (int)ItemRarityYellow));
    case WeaponGreatsword:
        switch (material) {
        case ItemMaterialObsidian:
            return SpriteObsidianGreatsword;
        case ItemMaterialBone:
            return SpriteBoneGreatsword;
        default:
            return SpriteIronGreatsword1 + modifier % 11 * ItemRarityCount +
                   std::max(0, std::min(rarity, (int)ItemRarityYellow));
        }
    case WeaponLongsword:
        switch (material) {
        case ItemMaterialObsidian:
            return SpriteObsidianLongsword;
        default:
            return SpriteIronLongsword1 + modifier % 11 * ItemRarityCount +
                   std::max(0, std::min(rarity, (int)ItemRarityYellow));
        }
    case WeaponMace:
        switch (material) {
        case ItemMaterialWood:
            return SpriteWoodMace01 + std::max(0, std::min(rarity, (int)ItemRarityYellow));
        case ItemMaterialBone:
            return SpriteBoneMace;
        default:
            return SpriteIronMace1 + modifier % 11 * ItemRarityCount +
                   std::max(0, std::min(rarity, (int)ItemRarityYellow));
        }
    case WeaponGreatmace:
        switch (material) {
        case ItemMaterialWood:
            return SpriteWoodGreatmace02;
        case ItemMaterialObsidian:
            return SpriteObsidianGreatmace;
        case ItemMaterialBone:
            return SpriteBoneGreatmace;
        default:
            return SpriteIronGreatmace1 + modifier % 11 * ItemRarityCount +
                   std::max(0, std::min(rarity, (int)ItemRarityYellow));
        }
    case WeaponAxe:
        switch (material) {
        case ItemMaterialObsidian:
            return SpriteUnloaded2293;
        case ItemMaterialBone:
            return SpriteBoneAxe;
        default:
            return SpriteIronAxe1 + modifier % 11 * ItemRarityCount +
                   std::max(0, std::min(rarity, (int)ItemRarityYellow));
        }
    case WeaponGreataxe:
        switch (material) {
        case ItemMaterialObsidian:
            return SpriteObsidianGreataxe;
        case ItemMaterialBone:
            return SpriteBoneGreataxe;
        case ItemMaterialSaurian:
            return SpriteSaurianGreataxe;
        default:
            return SpriteIronGreataxe1 + modifier % 11 * ItemRarityCount +
                   std::max(0, std::min(rarity, (int)ItemRarityYellow));
        }
    case WeaponBow:
        return SpriteBow + modifier % 11 * ItemRarityCount + std::max(0, std::min(rarity, (int)ItemRarityYellow));
    case WeaponBoomerang:
        return SpriteWoodBoomerang1 + modifier % 11 * ItemRarityCount +
               std::max(0, std::min(rarity, (int)ItemRarityYellow));
    case WeaponCrossbow:
        return SpriteWoodCrossbow1 + modifier % 11 * ItemRarityCount +
               std::max(0, std::min(rarity, (int)ItemRarityYellow));
    case WeaponArrows:
        return SpriteQuiver;
    case WeaponShield:
        switch (material) {
        case ItemMaterialWood:
            return SpriteWoodShield01 + std::max(0, std::min(rarity, (int)ItemRarityYellow));
        default:
            return SpriteIronShield01 + modifier % 11 * ItemRarityCount +
                   std::max(0, std::min(rarity, (int)ItemRarityYellow));
        }
    case WeaponArrow:
        return SpriteArrow;
    case WeaponTorch:
        return SpriteTorchSlot1029;
    case WeaponPitchfork:
        return SpritePitchfork;
    case WeaponMagnifier:
        return SpriteMagnifier;
    default:
        return SpriteIronSword1;
    }
}

}
