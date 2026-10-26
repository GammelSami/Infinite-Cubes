// SpriteManager loads voxel models and selects models for items and scenery.
#include "cube/SpriteManager.h"
#include <d3d9.h>
#include "cube/Sprite.h"
#include "cube/ItemIds.h"
#include "cube/SpriteIds.h"
#include "cube/Item.h"
#include "cube/Database.h"
#include "cube/RaceIds.h"

#include "cube/Fixed.h"

namespace cube {
enum {
    WeaponModelVariants = 11,
    BraceletModelVariants = 6,
    RaritySpriteCount = 5,
    ArmorModelVariants = 5,
    JewelryModelVariants = 5
};
// The decorations' variants by modifier (leftovers by rarity; the vases and candles are
// SpriteIds.h's), and the four slots from SpriteQuestItemAmulet01 (the two amulets, the jewel case
// and the copper coin's slot) that the other quest items take by their ID.
enum {
    LeftoversModelVariants = 4,
    PaintingModelVariants = 2,
    QuestItemFallbackSprites = 4
};
// The sprite slots loadSprites allocates (0.. SpriteCloud02).
enum {
    SpriteSlotCount = 2569
};

SpriteManager::SpriteManager(IDirect3DDevice9 *device)
    : m_device(device)
{}

SpriteManager::~SpriteManager()
{
    for (int i = 0; i < (int)m_sprites.size(); i++)
        delete m_sprites[i];
}

int consumableSpriteIndex(int id)
{
    switch (id) {
    case ConsumableCookie:
        return SpriteCookie;
    case ConsumableLifePotion:
        return SpriteLifePotion;
    case ConsumableManaPotion:
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
    default:
        return SpriteMushroom;
    }
}

Sprite *SpriteManager::itemSprite(const Item *item)
{
    switch (item->category) {
    case ItemCategoryConsumable:
        return sprite(consumableSpriteIndex(item->id));
    case ItemCategoryFormula:
        return sprite(SpriteFormula);
    case ItemCategoryWeapon:
        return sprite(weaponSpriteIndex(item->id, item->modifier, item->rarity, item->material));
    case ItemCategoryChest:
        switch (item->material) {
        case ItemMaterialObsidian:
            return sprite(SpriteObsidianChest);
        case ItemMaterialGold:
            return sprite(SpriteGoldChest);
        case ItemMaterialIce:
            return sprite(SpriteIceCrystalChest);
        case ItemMaterialLinen:
            return sprite(SpriteLinenChest1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        case ItemMaterialCotton:
            return sprite(SpriteWoolChest1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        case ItemMaterialLight:
            return sprite(SpriteLightChest);
        case ItemMaterialSilk:
            return sprite(SpriteSilkChest1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        case ItemMaterialBone:
            return sprite(SpriteBoneChest);
        case ItemMaterialClothing:
            switch (item->id) {
            case ChestChestArmor:
                return sprite(SpriteInnkeeperChest);
            case ChestBackpack:
                return sprite(SpriteBackpack);
            case ChestBody1:
                return sprite(SpriteBody1);
            case ChestBody2:
                return sprite(SpriteBody2);
            case ChestBody3:
                return sprite(SpriteBody3);
            case ChestBody4:
                return sprite(SpriteBody4);
            case ChestBody5:
                return sprite(SpriteBody5);
            default:
                break;
            }
            break;
        case ItemMaterialSaurian:
            return sprite(SpriteSaurianChest);
        case ItemMaterialParrot:
            return sprite(SpriteParrotChest);
        case ItemMaterialMammoth:
            return sprite(SpriteMammothChest);
        default:
            return sprite(SpriteIronChest1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        }
        // Unlisted clothing subtypes continue with shoulder armor.
    case ItemCategoryShoulder:
        switch (item->material) {
        case ItemMaterialObsidian:
            return sprite(SpriteObsidianShoulders);
        case ItemMaterialGold:
            return sprite(SpriteGoldShoulder);
        case ItemMaterialIce:
            return sprite(SpriteIceCrystalShoulder);
        case ItemMaterialLinen:
            return sprite(SpriteLinenShoulder1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        case ItemMaterialCotton:
            return sprite(SpriteWoolShoulder1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        case ItemMaterialLight:
            return sprite(SpriteLightShoulder);
        case ItemMaterialSilk:
            return sprite(SpriteSilkShoulder1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        case ItemMaterialBone:
            return sprite(SpriteBoneShoulders);
        case ItemMaterialSaurian:
            return sprite(SpriteSaurianShoulder);
        case ItemMaterialParrot:
            return sprite(SpriteParrotShoulder);
        case ItemMaterialMammoth:
            return sprite(SpriteMammothShoulder);
        default:
            return sprite(SpriteIronShoulder1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        }
    case ItemCategoryNeck:
        switch (item->material) {
        case ItemMaterialSilver:
            return sprite(SpriteSilverAmulet1Random1 + item->modifier % JewelryModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        default:
            return sprite(SpriteGoldAmulet1Random1 + item->modifier % JewelryModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        }
    case ItemCategoryRing:
        switch (item->material) {
        case ItemMaterialSilver:
            return sprite(SpriteSilverRing1Random1 + item->modifier % JewelryModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        default:
            return sprite(SpriteGoldRing1Random1 + item->modifier % JewelryModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        }
    case ItemCategoryHands:
        switch (item->material) {
        case ItemMaterialObsidian:
            return sprite(SpriteObsidianGlove);
        case ItemMaterialGold:
            return sprite(SpriteGoldGlove);
        case ItemMaterialIce:
            return sprite(SpriteIceCrystalGlove);
        case ItemMaterialLinen:
            return sprite(SpriteLinenHand1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        case ItemMaterialCotton:
            return sprite(SpriteWoolHand1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        case ItemMaterialSilk:
            return sprite(SpriteSilkHand1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
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
            return sprite(SpriteIronHand1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        default:
            return sprite(SpriteGlove);
        }
    case ItemCategoryFeet:
        switch (item->material) {
        case ItemMaterialObsidian:
            return sprite(SpriteObsidianBoot);
        case ItemMaterialGold:
            return sprite(SpriteGoldBoot);
        case ItemMaterialIce:
            return sprite(SpriteIceCrystalBoot);
        case ItemMaterialLinen:
            return sprite(SpriteLinenFoot1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        case ItemMaterialCotton:
            return sprite(SpriteWoolFoot1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        case ItemMaterialLight:
            return sprite(SpriteLightBoot);
        case ItemMaterialSilk:
            return sprite(SpriteSilkFoot1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        case ItemMaterialBone:
            return sprite(SpriteBoneBoot);
        case ItemMaterialSaurian:
            return sprite(SpriteSaurianBoot);
        case ItemMaterialParrot:
            return sprite(SpriteParrotBoot);
        case ItemMaterialMammoth:
            return sprite(SpriteMammothBoot);
        case ItemMaterialIron:
            return sprite(SpriteIronFoot1 + item->modifier % ArmorModelVariants * RaritySpriteCount +
                          std::min<int>((int)item->rarity, ItemRarityYellow));
        default:
            return sprite(SpriteBoot);
        }
    case ItemCategoryPet:
        switch (item->id) {
        case PetItemTurtle:
            return sprite(SpriteTurtleBody);
        case PetItemEgg:
            return sprite(SpriteEgg1);
        default:
            return sprite(SpritePetBox);
        }
    case ItemCategoryPetFood:
        switch (item->id) {
        case RaceBlueSlime:
            return sprite(SpriteJellyBlue);
        case RaceGreenSlime:
            return sprite(SpriteJellyGreen);
        case RacePinkSlime:
            return sprite(SpriteJellyPink);
        case RaceYellowSlime:
            return sprite(SpriteJellyYellow);
        case RaceBunny:
            return sprite(SpriteCarrot);
        case RacePig:
            return sprite(SpritePumpkinMash);
        case RaceCat:
            return sprite(SpriteCandy);
        case RaceOwl:
            return sprite(SpriteLollipop);
        case RacePenguin:
            return sprite(SpriteSoftice);
        case RaceMole:
            return sprite(SpriteDonutChocolate);
        case RaceSheep:
            return sprite(SpriteCottonCandy);
        case RaceHornet:
            return sprite(SpritePopcorn);
        case RaceChicken:
            return sprite(SpriteCerealBar);
        case RaceSquirrel:
            return sprite(SpriteStrawberryCake);
        case RaceRaccoon:
            return sprite(SpriteChocolateCake);
        case RaceAlpaca:
            return sprite(SpriteVanillaCupcake);
        case RaceBrownAlpaca:
            return sprite(SpriteChocolateCupcake);
        case RaceMonkey:
            return sprite(SpriteBananaSplit);
        case RaceScottishTerrier:
            return sprite(SpriteCroissant);
        case RaceBarkBeetle:
            return sprite(SpriteBread);
        case RaceSnoutBeetle:
            return sprite(SpriteLolly);
        case RaceLemonBeetle:
            return sprite(SpriteLemonTart);
        case RacePeacock:
            return sprite(SpriteChocolateCookie);
        case RaceCollie:
            return sprite(SpriteBubbleGum);
        case RaceCrow:
            return sprite(SpriteLicoriceCandy);
        case RaceTurtle:
            return sprite(SpriteCinnamonRole);
        case RaceCrocodile:
            return sprite(SpriteAppleRing);
        case RaceSpitter:
            return sprite(SpriteWaterIce);
        case RaceTerrier:
            return sprite(SpriteWaffle);
        case RaceCamel:
            return sprite(SpriteDateCookie);
        case RaceHorse:
            return sprite(SpriteCandiedApple);
        case RaceCrab:
            return sprite(SpriteStrawberryCocktail);
        case RacePlainRunner:
            return sprite(SpriteMilkChocolateBar);
        case RaceDesertRunner:
            return sprite(SpriteCaramelChocolateBar);
        case RaceLeafRunner:
            return sprite(SpriteMintChocolateBar);
        case RaceSnowRunner:
            return sprite(SpriteWhiteChocolateBar);
        case RaceDuckbill:
            return sprite(SpriteSugarCandy);
        case RacePorcupine:
            return sprite(SpriteBlackberryMarmelade);
        case RaceSeagull:
            return sprite(SpriteSaltedCaramel);
        case RaceParrot:
            return sprite(SpriteGingerTartlet);
        case RaceBat:
            return sprite(SpriteMangoJuice);
        case RaceFly:
            return sprite(SpriteFruitBasket);
        case RaceMidge:
            return sprite(SpriteMelonIcecream);
        case RaceMosquito:
            return sprite(SpriteBloodorangeJuice);
        case RaceBiter:
            return sprite(SpritePancakes);
        case RaceFireBeetle:
            return sprite(SpriteCurry);
        case RaceBumblebee:
            return sprite(SpriteBiscuitRole);
        default:
            return sprite(SpritePineapple);
        }
    case ItemCategoryCoin:
        switch (item->material) {
        case ItemMaterialSilver:
            return sprite(SpriteSilverCoin);
        case ItemMaterialGold:
            return sprite(SpriteGoldCoin);
        default:
            return sprite(SpriteCopperCoin);
        }
    case ItemCategoryPlatinumCoin:
        return sprite(SpriteIceCrystal);
    case ItemCategoryManaCube:
        return sprite(SpriteManacube);
    case ItemCategoryBeak:
        return sprite(SpriteParrotBeak);
    case ItemCategoryMaterial:
        switch (item->id) {
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
        case MaterialItemCrystal:
            return sprite(SpriteIceCrystal);
        case MaterialItemYarn:
            return sprite(SpriteIceCoatedYarn);
        case MaterialItemCobweb:
            return sprite(SpriteCobweb);
        case MaterialItemOrb:
            return sprite(SpriteSpiribit);
        case MaterialItemFlask:
            return sprite(SpriteFlask);
        case MaterialItemSpirit:
            switch (item->material) {
            case ItemMaterialWindSpirit:
                return sprite(SpriteWindCube);
            case ItemMaterialIceSpirit:
                return sprite(SpriteIceCube);
            case ItemMaterialUnholySpirit:
                return sprite(SpriteUnholyCube);
            default:
                return sprite(SpriteFireCube);
            }
        case MaterialItemNugget:
            switch (item->material) {
            case ItemMaterialIron:
                return sprite(SpriteIronNugget);
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
            case ItemMaterialWood:
                return sprite(SpriteWoodLog);
            case ItemMaterialSandstone:
                return sprite(SpriteSandstone);
            default:
                return 0;
            }
        case MaterialItemCube:
            switch (item->material) {
            case ItemMaterialIron:
                return sprite(SpriteIronCube);
            case ItemMaterialSilver:
                return sprite(SpriteSilverCube);
            case ItemMaterialGold:
                return sprite(SpriteGoldCube);
            case ItemMaterialWood:
                return sprite(SpriteWoodCube);
            default:
                return 0;
            }
        case MaterialItemMushroom:
            return sprite(SpriteMushroom);
        case MaterialItemPumpkin:
            return sprite(SpritePumpkin);
        case MaterialItemPineapple:
            return sprite(SpritePineapple);
        case MaterialItemHeartflower:
            return sprite(SpriteHeartflower);
        case MaterialItemFrozenHeartflower:
            return sprite(SpriteHeartflowerFrozen);
        case MaterialItemPricklyPear:
            return sprite(SpritePricklyPear);
        case MaterialItemSoulflower:
            return sprite(SpriteSoulflower);
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
        default:
            return 0;
        }
    case ItemCategoryCube:
        return sprite(SpriteCube);
    case ItemCategoryQuestItem:
        switch (item->id) {
        case QuestItemKey:
            return sprite(SpriteKey1);
        case QuestItemMedicine:
            return sprite(SpriteManaPotion);
        case QuestItemAntivenom:
            return sprite(SpriteManaPotion);
        case QuestItemBandAid:
            return sprite(SpritePlaster);
        case QuestItemCrutch:
            return sprite(SpriteIronMace1);
        case QuestItemBandage:
            return sprite(SpriteIceCoatedYarn);
        case QuestItemSalve:
            return sprite(SpriteSoupGinseng);
        default:
            return sprite(SpriteQuestItemAmulet01 + (int)item->id % QuestItemFallbackSprites);
        }
    case ItemCategoryLeftovers:
        return sprite(SpriteDigestedLeftovers01 +
                      std::min<int>(LeftoversModelVariants - 1, std::max(0, (int)item->rarity - ItemRarityGreen)));
    case ItemCategoryPainting:
        return sprite(SpritePainting01 + item->modifier % PaintingModelVariants);
    case ItemCategoryVase:
        return sprite(SpriteVase + item->modifier % VaseModelVariants);
    case ItemCategoryCandle:
        switch (item->id) {
        case CandleItemUndead:
            return sprite(SpriteUndeadCandle1 + item->modifier % CandleModelVariants);
        default:
            return sprite(SpriteCandle01 + item->modifier % CandleModelVariants);
        }
    case ItemCategorySpecial:
        switch (item->id) {
        case SpecialItemBoat:
            return sprite(SpriteBoat2);
        default:
            return sprite(SpriteGlider);
        }
    case ItemCategoryLamp:
        return sprite(SpriteLantern02);
    default:
        return 0;
    }
}

void SpriteManager::loadSprites(const std::string &directory)
{
    m_sprites.resize(SpriteSlotCount);
    for (int i = 0; i < (int)m_sprites.size(); i++)
        m_sprites[i] = new Sprite(m_device, false);

    std::string path;
    if (!directory.empty())
        path = directory + "/";

    Database *database = new Database();
    database->open(path + "data1.db");
    path = "";

    m_sprites[SpriteCubequest4]->load(path + "cubequest4.cub", database, true);
    m_sprites[SpriteSpiribit]->load(path + "spiribit.cub", database, true);
    m_sprites[SpritePetBox]->load(path + "pet-box.cub", database, true);
    m_sprites[SpriteEgg1]->load(path + "egg1.cub", database, true);
    m_sprites[SpriteEgg1Slot2165]->load(path + "egg1.cub", database, true);
    m_sprites[SpriteEgg1Slot2166]->load(path + "egg1.cub", database, true);
    m_sprites[SpriteFormula]->load(path + "formula.cub", database, true);
    m_sprites[SpriteBody2Slot0]->load(path + "body2.cub", database, true);
    m_sprites[SpriteBody4Slot1]->load(path + "body4.cub", database, true);
    m_sprites[SpriteBody3Slot2]->load(path + "body3.cub", database, true);
    m_sprites[SpriteGlove]->load(path + "glove.cub", database, true);
    m_sprites[SpriteHead2]->load(path + "head2.cub", database, true);
    m_sprites[SpriteOldHead]->load(path + "old-head.cub", database, true);
    m_sprites[SpriteGirlHead3]->load(path + "girl-head3.cub", database, true);
    m_sprites[SpriteElfHeadFemale]->load(path + "elf-head-female.cub", database, true);
    m_sprites[SpriteHand2]->load(path + "hand2.cub", database, true);
    m_sprites[SpriteBrownHand]->load(path + "brown-hand.cub", database, true);
    m_sprites[SpriteFoot]->load(path + "foot.cub", database, true);
    m_sprites[SpriteBoot]->load(path + "boot.cub", database, true);
    m_sprites[SpriteTrollFoot]->load(path + "troll-foot.cub", database, true);
    m_sprites[SpriteTrollHead]->load(path + "troll-head.cub", database, true);
    m_sprites[SpriteTrollBody]->load(path + "troll-body.cub", database, true);
    m_sprites[SpriteTrollHand]->load(path + "troll-hand.cub", database, true);
    m_sprites[SpriteTrollArm]->load(path + "troll-arm.cub", database, true);
    m_sprites[SpriteDarkTrollFoot]->load(path + "dark-troll-foot.cub", database, true);
    m_sprites[SpriteDarkTrollHead]->load(path + "dark-troll-head.cub", database, true);
    m_sprites[SpriteDarkTrollBody]->load(path + "dark-troll-body.cub", database, true);
    m_sprites[SpriteDarkTrollHand]->load(path + "dark-troll-hand.cub", database, true);
    m_sprites[SpriteDarkTrollArm]->load(path + "dark-troll-arm.cub", database, true);
    m_sprites[SpriteDarkTrollEyes]->load(path + "dark-troll-eyes.cub", database, false);
    m_sprites[SpriteHellDemonFoot]->load(path + "hell-demon-foot.cub", database, true);
    m_sprites[SpriteHellDemonHead]->load(path + "hell-demon-head.cub", database, true);
    m_sprites[SpriteHellDemonBody]->load(path + "hell-demon-body.cub", database, true);
    m_sprites[SpriteHellDemonArm]->load(path + "hell-demon-arm.cub", database, true);
    m_sprites[SpriteHellDemonHand]->load(path + "hell-demon-hand.cub", database, true);
    m_sprites[SpriteGolemFoot]->load(path + "golem-foot.cub", database, true);
    m_sprites[SpriteGolemHead]->load(path + "golem-head.cub", database, true);
    m_sprites[SpriteGolemBody]->load(path + "golem-body.cub", database, true);
    m_sprites[SpriteGolemHand]->load(path + "golem-hand.cub", database, true);
    m_sprites[SpriteGolemArm]->load(path + "golem-arm.cub", database, true);
    m_sprites[SpriteGolemEmberFoot]->load(path + "golem-ember-foot.cub", database, true);
    m_sprites[SpriteGolemEmberHead]->load(path + "golem-ember-head.cub", database, true);
    m_sprites[SpriteGolemEmberBody]->load(path + "golem-ember-body.cub", database, true);
    m_sprites[SpriteGolemEmberHand]->load(path + "golem-ember-hand.cub", database, true);
    m_sprites[SpriteGolemEmberArm]->load(path + "golem-ember-arm.cub", database, true);
    m_sprites[SpriteGolemSnowFoot]->load(path + "golem-snow-foot.cub", database, true);
    m_sprites[SpriteGolemSnowHead]->load(path + "golem-snow-head.cub", database, true);
    m_sprites[SpriteGolemSnowBody]->load(path + "golem-snow-body.cub", database, true);
    m_sprites[SpriteGolemSnowHand]->load(path + "golem-snow-hand.cub", database, true);
    m_sprites[SpriteGolemSnowArm]->load(path + "golem-snow-arm.cub", database, true);
    m_sprites[SpriteYetiFoot]->load(path + "yeti-foot.cub", database, true);
    m_sprites[SpriteYetiHead]->load(path + "yeti-head.cub", database, true);
    m_sprites[SpriteYetiBody]->load(path + "yeti-body.cub", database, true);
    m_sprites[SpriteYetiHand]->load(path + "yeti-hand.cub", database, true);
    m_sprites[SpriteYetiArm]->load(path + "yeti-arm.cub", database, true);
    m_sprites[SpriteCyclopsFoot]->load(path + "cyclops-foot.cub", database, true);
    m_sprites[SpriteCyclopsHead]->load(path + "cyclops-head.cub", database, true);
    m_sprites[SpriteCyclopsBody]->load(path + "cyclops-body.cub", database, true);
    m_sprites[SpriteCyclopsHand]->load(path + "cyclops-hand.cub", database, true);
    m_sprites[SpriteCyclopsArm]->load(path + "cyclops-arm.cub", database, true);
    m_sprites[SpriteOgreFoot]->load(path + "ogre-foot.cub", database, true);
    m_sprites[SpriteOgreHead]->load(path + "ogre-head.cub", database, true);
    m_sprites[SpriteOgreBody]->load(path + "ogre-body.cub", database, true);
    m_sprites[SpriteOgreHand]->load(path + "ogre-hand.cub", database, true);
    m_sprites[SpriteRocklingFoot]->load(path + "rockling-foot.cub", database, true);
    m_sprites[SpriteRocklingHead]->load(path + "rockling-head.cub", database, true);
    m_sprites[SpriteRocklingHand]->load(path + "rockling-hand.cub", database, true);
    m_sprites[SpriteSpikeFoot]->load(path + "spike-foot.cub", database, true);
    m_sprites[SpriteSpikeHead]->load(path + "spike-head.cub", database, true);
    m_sprites[SpriteSpikeBody]->load(path + "spike-body.cub", database, true);
    m_sprites[SpriteSpikeHand]->load(path + "spike-hand.cub", database, true);
    m_sprites[SpriteAnubisFoot]->load(path + "anubis-foot.cub", database, true);
    m_sprites[SpriteAnubisHead]->load(path + "anubis-head.cub", database, true);
    m_sprites[SpriteAnubisBody]->load(path + "anubis-body.cub", database, true);
    m_sprites[SpriteAnubisHand]->load(path + "anubis-hand.cub", database, true);
    m_sprites[SpriteHorusFoot]->load(path + "horus-foot.cub", database, true);
    m_sprites[SpriteHorusHead]->load(path + "horus-head.cub", database, true);
    m_sprites[SpriteHorusBody]->load(path + "horus-body.cub", database, true);
    m_sprites[SpriteHorusHand]->load(path + "horus-hand.cub", database, true);
    m_sprites[SpriteJesterFoot]->load(path + "jester-foot.cub", database, true);
    m_sprites[SpriteJesterHead]->load(path + "jester-head.cub", database, true);
    m_sprites[SpriteJesterBody]->load(path + "jester-body.cub", database, true);
    m_sprites[SpriteJesterHand]->load(path + "jester-hand.cub", database, true);
    m_sprites[SpriteSpectrinoFoot]->load(path + "spectrino-foot.cub", database, true);
    m_sprites[SpriteSpectrinoHead]->load(path + "spectrino-head.cub", database, true);
    m_sprites[SpriteSpectrinoBody]->load(path + "spectrino-body.cub", database, true);
    m_sprites[SpriteSpectrinoHand]->load(path + "spectrino-hand.cub", database, true);
    m_sprites[SpriteDjinnFoot]->load(path + "djinn-foot.cub", database, true);
    m_sprites[SpriteDjinnHead]->load(path + "djinn-head.cub", database, true);
    m_sprites[SpriteDjinnBody]->load(path + "djinn-body.cub", database, true);
    m_sprites[SpriteDjinnHand]->load(path + "djinn-hand.cub", database, true);
    m_sprites[SpriteMinotaurFoot]->load(path + "minotaur-foot.cub", database, true);
    m_sprites[SpriteMinotaurHead]->load(path + "minotaur-head.cub", database, true);
    m_sprites[SpriteMinotaurBody]->load(path + "minotaur-body.cub", database, true);
    m_sprites[SpriteMinotaurHand]->load(path + "minotaur-hand.cub", database, true);
    m_sprites[SpriteMinotaurArm]->load(path + "minotaur-arm.cub", database, true);
    m_sprites[SpriteDesertNomadFoot]->load(path + "desert-nomad-foot.cub", database, true);
    m_sprites[SpriteDesertNomadHead1]->load(path + "desert-nomad-head1.cub", database, true);
    m_sprites[SpriteDesertNomadHead2]->load(path + "desert-nomad-head2.cub", database, true);
    m_sprites[SpriteDesertNomadHead3]->load(path + "desert-nomad-head3.cub", database, true);
    m_sprites[SpriteDesertNomadFemaleHead1]->load(path + "desert-nomad-female-head1.cub", database, true);
    m_sprites[SpriteDesertNomadFemaleHead2]->load(path + "desert-nomad-female-head2.cub", database, true);
    m_sprites[SpriteDesertNomadFemaleHead3]->load(path + "desert-nomad-female-head3.cub", database, true);
    m_sprites[SpriteDesertNomadBody]->load(path + "desert-nomad-body.cub", database, true);
    m_sprites[SpriteDesertNomadHand]->load(path + "desert-nomad-hand.cub", database, true);
    m_sprites[SpriteDesertNomadHand2]->load(path + "desert-nomad-hand2.cub", database, true);
    m_sprites[SpriteImpFoot]->load(path + "imp-foot.cub", database, true);
    m_sprites[SpriteImpHead]->load(path + "imp-head.cub", database, true);
    m_sprites[SpriteImpBody]->load(path + "imp-body.cub", database, true);
    m_sprites[SpriteImpHand]->load(path + "imp-hand.cub", database, true);
    m_sprites[SpriteImpEyes]->load(path + "imp-eyes.cub", database, false);
    m_sprites[SpriteSquidFoot]->load(path + "squid-foot.cub", database, true);
    m_sprites[SpriteSquidBody]->load(path + "squid-body.cub", database, true);
    m_sprites[SpriteMammothBody]->load(path + "mammoth-body.cub", database, true);
    m_sprites[SpriteMammothHead]->load(path + "mammoth-head.cub", database, true);
    m_sprites[SpriteMammothFoot]->load(path + "mammoth-foot.cub", database, true);
    m_sprites[SpriteBullterrierHead1]->load(path + "bullterrier-head1.cub", database, true);
    m_sprites[SpriteBullterrierHead2]->load(path + "bullterrier-head2.cub", database, true);
    m_sprites[SpriteBullterrierHead3]->load(path + "bullterrier-head3.cub", database, true);
    m_sprites[SpriteGoblinHeadM01]->load(path + "goblin-head-m01.cub", database, true);
    m_sprites[SpriteGoblinHeadM02]->load(path + "goblin-head-m02.cub", database, true);
    m_sprites[SpriteGoblinHeadM03]->load(path + "goblin-head-m03.cub", database, true);
    m_sprites[SpriteGoblinHeadM04]->load(path + "goblin-head-m04.cub", database, true);
    m_sprites[SpriteGoblinHeadM05]->load(path + "goblin-head-m05.cub", database, true);
    m_sprites[SpriteGoblinHairM01]->load(path + "goblin-hair-m01.cub", database, false);
    m_sprites[SpriteGoblinHairM02]->load(path + "goblin-hair-m02.cub", database, false);
    m_sprites[SpriteGoblinHairM03]->load(path + "goblin-hair-m03.cub", database, false);
    m_sprites[SpriteGoblinHairM04]->load(path + "goblin-hair-m04.cub", database, false);
    m_sprites[SpriteGoblinHairM05]->load(path + "goblin-hair-m05.cub", database, false);
    m_sprites[SpriteGoblinHairM06]->load(path + "goblin-hair-m06.cub", database, false);
    m_sprites[SpriteGoblinHeadF01]->load(path + "goblin-head-f01.cub", database, true);
    m_sprites[SpriteGoblinHeadF02]->load(path + "goblin-head-f02.cub", database, true);
    m_sprites[SpriteGoblinHeadF03]->load(path + "goblin-head-f03.cub", database, true);
    m_sprites[SpriteGoblinHeadF04]->load(path + "goblin-head-f04.cub", database, true);
    m_sprites[SpriteGoblinHeadF05]->load(path + "goblin-head-f05.cub", database, true);
    m_sprites[SpriteGoblinHairF01]->load(path + "goblin-hair-f01.cub", database, false);
    m_sprites[SpriteGoblinHairF02]->load(path + "goblin-hair-f02.cub", database, false);
    m_sprites[SpriteGoblinHairF03]->load(path + "goblin-hair-f03.cub", database, false);
    m_sprites[SpriteGoblinHairF04]->load(path + "goblin-hair-f04.cub", database, false);
    m_sprites[SpriteGoblinHairF05]->load(path + "goblin-hair-f05.cub", database, false);
    m_sprites[SpriteGoblinHairF06]->load(path + "goblin-hair-f06.cub", database, false);
    m_sprites[SpriteGoblinHand]->load(path + "goblin-hand.cub", database, true);
    m_sprites[SpriteElfHeadM01]->load(path + "elf-head-m01.cub", database, true);
    m_sprites[SpriteElfHeadM02]->load(path + "elf-head-m02.cub", database, true);
    m_sprites[SpriteElfHeadM03]->load(path + "elf-head-m03.cub", database, true);
    m_sprites[SpriteElfHeadM04]->load(path + "elf-head-m04.cub", database, true);
    m_sprites[SpriteElfHeadF01]->load(path + "elf-head-f01.cub", database, true);
    m_sprites[SpriteElfHeadF02]->load(path + "elf-head-f02.cub", database, true);
    m_sprites[SpriteElfHeadF03]->load(path + "elf-head-f03.cub", database, true);
    m_sprites[SpriteElfHeadF04]->load(path + "elf-head-f04.cub", database, true);
    m_sprites[SpriteElfHeadF05]->load(path + "elf-head-f05.cub", database, true);
    m_sprites[SpriteElfHeadF06]->load(path + "elf-head-f06.cub", database, true);
    m_sprites[SpriteHumanHeadM01]->load(path + "human-head-m01.cub", database, true);
    m_sprites[SpriteHumanHeadM02]->load(path + "human-head-m02.cub", database, true);
    m_sprites[SpriteHumanHeadM03]->load(path + "human-head-m03.cub", database, true);
    m_sprites[SpriteHumanHeadM04]->load(path + "human-head-m04.cub", database, true);
    m_sprites[SpriteHumanHeadM05]->load(path + "human-head-m05.cub", database, true);
    m_sprites[SpriteHumanHeadM06]->load(path + "human-head-m06.cub", database, true);
    m_sprites[SpriteHumanHairM01]->load(path + "human-hair-m01.cub", database, false);
    m_sprites[SpriteHumanHairM02]->load(path + "human-hair-m02.cub", database, false);
    m_sprites[SpriteHumanHairM03]->load(path + "human-hair-m03.cub", database, false);
    m_sprites[SpriteHumanHairM04]->load(path + "human-hair-m04.cub", database, false);
    m_sprites[SpriteHumanHairM05]->load(path + "human-hair-m05.cub", database, false);
    m_sprites[SpriteHumanHairM06]->load(path + "human-hair-m06.cub", database, false);
    m_sprites[SpriteHumanHairM07]->load(path + "human-hair-m07.cub", database, false);
    m_sprites[SpriteHumanHairM08]->load(path + "human-hair-m08.cub", database, false);
    m_sprites[SpriteHumanHairM09]->load(path + "human-hair-m09.cub", database, false);
    m_sprites[SpriteHumanHairM10]->load(path + "human-hair-m10.cub", database, false);
    m_sprites[SpriteHumanHairM11]->load(path + "human-hair-m11.cub", database, false);
    m_sprites[SpriteHumanHairM12]->load(path + "human-hair-m12.cub", database, false);
    m_sprites[SpriteHumanHairM13]->load(path + "human-hair-m13.cub", database, false);
    m_sprites[SpriteHumanHairM14]->load(path + "human-hair-m14.cub", database, false);
    m_sprites[SpriteHumanHairM15]->load(path + "human-hair-m15.cub", database, false);
    m_sprites[SpriteHumanHeadF01]->load(path + "human-head-f01.cub", database, true);
    m_sprites[SpriteHumanHeadF02]->load(path + "human-head-f02.cub", database, true);
    m_sprites[SpriteHumanHeadF03]->load(path + "human-head-f03.cub", database, true);
    m_sprites[SpriteHumanHeadF04]->load(path + "human-head-f04.cub", database, true);
    m_sprites[SpriteHumanHeadF05]->load(path + "human-head-f05.cub", database, true);
    m_sprites[SpriteHumanHeadF06]->load(path + "human-head-f06.cub", database, true);
    m_sprites[SpriteHumanHairF01]->load(path + "human-hair-f01.cub", database, false);
    m_sprites[SpriteHumanHairF02]->load(path + "human-hair-f02.cub", database, false);
    m_sprites[SpriteHumanHairF03]->load(path + "human-hair-f03.cub", database, false);
    m_sprites[SpriteHumanHairF04]->load(path + "human-hair-f04.cub", database, false);
    m_sprites[SpriteHumanHairF05]->load(path + "human-hair-f05.cub", database, false);
    m_sprites[SpriteHumanHairF06]->load(path + "human-hair-f06.cub", database, false);
    m_sprites[SpriteHumanHairF07]->load(path + "human-hair-f07.cub", database, false);
    m_sprites[SpriteElfHairM01]->load(path + "elf-hair-m01.cub", database, false);
    m_sprites[SpriteElfHairM02]->load(path + "elf-hair-m02.cub", database, false);
    m_sprites[SpriteElfHairM03]->load(path + "elf-hair-m03.cub", database, false);
    m_sprites[SpriteElfHairM04]->load(path + "elf-hair-m04.cub", database, false);
    m_sprites[SpriteElfHairM05]->load(path + "elf-hair-m05.cub", database, false);
    m_sprites[SpriteElfHairM06]->load(path + "elf-hair-m06.cub", database, false);
    m_sprites[SpriteElfHairM07]->load(path + "elf-hair-m07.cub", database, false);
    m_sprites[SpriteElfHairM08]->load(path + "elf-hair-m08.cub", database, false);
    m_sprites[SpriteElfHairM09]->load(path + "elf-hair-m09.cub", database, false);
    m_sprites[SpriteElfHairM10]->load(path + "elf-hair-m10.cub", database, false);
    m_sprites[SpriteElfHairF01]->load(path + "elf-hair-f01.cub", database, false);
    m_sprites[SpriteElfHairF02]->load(path + "elf-hair-f02.cub", database, false);
    m_sprites[SpriteElfHairF03]->load(path + "elf-hair-f03.cub", database, false);
    m_sprites[SpriteElfHairF04]->load(path + "elf-hair-f04.cub", database, false);
    m_sprites[SpriteElfHairF05]->load(path + "elf-hair-f05.cub", database, false);
    m_sprites[SpriteElfHairF06]->load(path + "elf-hair-f06.cub", database, false);
    m_sprites[SpriteElfHairF07]->load(path + "elf-hair-f07.cub", database, false);
    m_sprites[SpriteElfHairF08]->load(path + "elf-hair-f08.cub", database, false);
    m_sprites[SpriteElfHairF09]->load(path + "elf-hair-f09.cub", database, false);
    m_sprites[SpriteElfHairF10]->load(path + "elf-hair-f10.cub", database, false);
    m_sprites[SpriteGnoboldFoot]->load(path + "gnobold-foot.cub", database, true);
    m_sprites[SpriteGnoboldHand]->load(path + "gnobold-hand.cub", database, true);
    m_sprites[SpriteGnoboldHead]->load(path + "gnobold-head.cub", database, true);
    m_sprites[SpriteInsectoidFoot]->load(path + "insectoid-foot.cub", database, true);
    m_sprites[SpriteInsectoidHand]->load(path + "insectoid-hand.cub", database, true);
    m_sprites[SpriteInsectoidHead]->load(path + "insectoid-head.cub", database, true);
    m_sprites[SpriteInsectoidBody]->load(path + "insectoid-body.cub", database, true);
    m_sprites[SpriteGnollFoot]->load(path + "gnoll-foot.cub", database, true);
    m_sprites[SpriteGnollHand]->load(path + "gnoll-hand.cub", database, true);
    m_sprites[SpriteGnollHead]->load(path + "gnoll-head.cub", database, true);
    m_sprites[SpriteGnollBody]->load(path + "gnoll-body.cub", database, true);
    m_sprites[SpritePolarGnollFoot]->load(path + "polar-gnoll-foot.cub", database, true);
    m_sprites[SpritePolarGnollHand]->load(path + "polar-gnoll-hand.cub", database, true);
    m_sprites[SpritePolarGnollHead]->load(path + "polar-gnoll-head.cub", database, true);
    m_sprites[SpritePolarGnollBody]->load(path + "polar-gnoll-body.cub", database, true);
    m_sprites[SpriteMonkeyFoot]->load(path + "monkey-foot.cub", database, true);
    m_sprites[SpriteMonkeyHand]->load(path + "monkey-hand.cub", database, true);
    m_sprites[SpriteMonkeyHead]->load(path + "monkey-head.cub", database, true);
    m_sprites[SpriteMonkeyBody]->load(path + "monkey-body.cub", database, true);
    m_sprites[SpriteOrcHeadM01]->load(path + "orc-head-m01.cub", database, true);
    m_sprites[SpriteOrcHeadM02]->load(path + "orc-head-m02.cub", database, true);
    m_sprites[SpriteOrcHeadM03]->load(path + "orc-head-m03.cub", database, true);
    m_sprites[SpriteOrcHeadM04]->load(path + "orc-head-m04.cub", database, true);
    m_sprites[SpriteOrcHeadM05]->load(path + "orc-head-m05.cub", database, true);
    m_sprites[SpriteOrcHairM01]->load(path + "orc-hair-m01.cub", database, false);
    m_sprites[SpriteOrcHairM02]->load(path + "orc-hair-m02.cub", database, false);
    m_sprites[SpriteOrcHairM03]->load(path + "orc-hair-m03.cub", database, false);
    m_sprites[SpriteOrcHairM04]->load(path + "orc-hair-m04.cub", database, false);
    m_sprites[SpriteOrcHairM05]->load(path + "orc-hair-m05.cub", database, false);
    m_sprites[SpriteOrcHairM06]->load(path + "orc-hair-m06.cub", database, false);
    m_sprites[SpriteOrcHairM07]->load(path + "orc-hair-m07.cub", database, false);
    m_sprites[SpriteOrcHairM08]->load(path + "orc-hair-m08.cub", database, false);
    m_sprites[SpriteOrcHairM09]->load(path + "orc-hair-m09.cub", database, false);
    m_sprites[SpriteOrcHairM10]->load(path + "orc-hair-m10.cub", database, false);
    m_sprites[SpriteOrcHairF01]->load(path + "orc-hair-f01.cub", database, false);
    m_sprites[SpriteOrcHairF02]->load(path + "orc-hair-f02.cub", database, false);
    m_sprites[SpriteOrcHairF03]->load(path + "orc-hair-f03.cub", database, false);
    m_sprites[SpriteOrcHairF04]->load(path + "orc-hair-f04.cub", database, false);
    m_sprites[SpriteFrogmanHeadM01]->load(path + "frogman-head-m01.cub", database, true);
    m_sprites[SpriteFrogmanHeadM02]->load(path + "frogman-head-m02.cub", database, true);
    m_sprites[SpriteFrogmanHeadM03]->load(path + "frogman-head-m03.cub", database, true);
    m_sprites[SpriteFrogmanHeadM04]->load(path + "frogman-head-m04.cub", database, true);
    m_sprites[SpriteFrogmanHeadM05]->load(path + "frogman-head-m05.cub", database, true);
    m_sprites[SpriteFrogmanHairM01]->load(path + "frogman-hair-m01.cub", database, false);
    m_sprites[SpriteFrogmanHairM02]->load(path + "frogman-hair-m02.cub", database, false);
    m_sprites[SpriteFrogmanHairM03]->load(path + "frogman-hair-m03.cub", database, false);
    m_sprites[SpriteFrogmanHairM04]->load(path + "frogman-hair-m04.cub", database, false);
    m_sprites[SpriteFrogmanHairM05]->load(path + "frogman-hair-m05.cub", database, false);
    m_sprites[SpriteFrogmanHeadF01]->load(path + "frogman-head-f01.cub", database, true);
    m_sprites[SpriteFrogmanHeadF02]->load(path + "frogman-head-f02.cub", database, true);
    m_sprites[SpriteFrogmanHeadF03]->load(path + "frogman-head-f03.cub", database, true);
    m_sprites[SpriteFrogmanHeadF04]->load(path + "frogman-head-f04.cub", database, true);
    m_sprites[SpriteFrogmanHairF01]->load(path + "frogman-hair-f01.cub", database, false);
    m_sprites[SpriteFrogmanHairF02]->load(path + "frogman-hair-f02.cub", database, false);
    m_sprites[SpriteFrogmanHairF03]->load(path + "frogman-hair-f03.cub", database, false);
    m_sprites[SpriteFrogmanHairF04]->load(path + "frogman-hair-f04.cub", database, false);
    m_sprites[SpriteFrogmanHand]->load(path + "frogman-hand.cub", database, true);
    m_sprites[SpriteMermaidHeadF01]->load(path + "mermaid-head-f01.cub", database, true);
    m_sprites[SpriteMermaidHeadF02]->load(path + "mermaid-head-f02.cub", database, true);
    m_sprites[SpriteMermaidHeadF03]->load(path + "mermaid-head-f03.cub", database, true);
    m_sprites[SpriteMermaidHairF01]->load(path + "mermaid-hair-f01.cub", database, false);
    m_sprites[SpriteMermaidHairF02]->load(path + "mermaid-hair-f02.cub", database, false);
    m_sprites[SpriteMermaidHairF03]->load(path + "mermaid-hair-f03.cub", database, false);
    m_sprites[SpriteMermaidBody]->load(path + "mermaid-body.cub", database, true);
    m_sprites[SpriteMermaidHand]->load(path + "mermaid-hand.cub", database, true);
    m_sprites[SpriteMermanHeadM01]->load(path + "merman-head-m01.cub", database, true);
    m_sprites[SpriteMermanHeadM02]->load(path + "merman-head-m02.cub", database, true);
    m_sprites[SpriteMermanHeadM03]->load(path + "merman-head-m03.cub", database, true);
    m_sprites[SpriteMermanHairM01]->load(path + "merman-hair-m01.cub", database, false);
    m_sprites[SpriteMermanHairM02]->load(path + "merman-hair-m02.cub", database, false);
    m_sprites[SpriteMermanHairM03]->load(path + "merman-hair-m03.cub", database, false);
    m_sprites[SpriteMermanBody]->load(path + "merman-body.cub", database, true);
    m_sprites[SpriteMermaidHandSlot1358]->load(path + "mermaid-hand.cub", database, true);
    m_sprites[SpriteLizardHeadM01]->load(path + "lizard-head-m01.cub", database, true);
    m_sprites[SpriteLizardHeadM02]->load(path + "lizard-head-m02.cub", database, true);
    m_sprites[SpriteLizardHairM01]->load(path + "lizard-hair-m01.cub", database, false);
    m_sprites[SpriteLizardHairM02]->load(path + "lizard-hair-m02.cub", database, false);
    m_sprites[SpriteLizardHairM03]->load(path + "lizard-hair-m03.cub", database, false);
    m_sprites[SpriteLizardHairM04]->load(path + "lizard-hair-m04.cub", database, false);
    m_sprites[SpriteLizardHairM05]->load(path + "lizard-hair-m05.cub", database, false);
    m_sprites[SpriteLizardHairM06]->load(path + "lizard-hair-m06.cub", database, false);
    m_sprites[SpriteLizardHeadF01]->load(path + "lizard-head-f01.cub", database, true);
    m_sprites[SpriteLizardHeadF02]->load(path + "lizard-head-f02.cub", database, true);
    m_sprites[SpriteLizardHeadF03]->load(path + "lizard-head-f03.cub", database, true);
    m_sprites[SpriteLizardHeadF04]->load(path + "lizard-head-f04.cub", database, true);
    m_sprites[SpriteLizardHeadF05]->load(path + "lizard-head-f05.cub", database, true);
    m_sprites[SpriteLizardHand]->load(path + "lizard-hand.cub", database, true);
    m_sprites[SpriteLizardBody]->load(path + "lizard-body.cub", database, true);
    m_sprites[SpriteLizardFoot]->load(path + "lizard-foot.cub", database, true);
    m_sprites[SpriteDwarfHeadM01]->load(path + "dwarf-head-m01.cub", database, true);
    m_sprites[SpriteDwarfHeadM02]->load(path + "dwarf-head-m02.cub", database, true);
    m_sprites[SpriteDwarfHeadM03]->load(path + "dwarf-head-m03.cub", database, true);
    m_sprites[SpriteDwarfHeadM04]->load(path + "dwarf-head-m04.cub", database, true);
    m_sprites[SpriteDwarfHeadM05]->load(path + "dwarf-head-m05.cub", database, true);
    m_sprites[SpriteDwarfHairM01]->load(path + "dwarf-hair-m01.cub", database, false);
    m_sprites[SpriteDwarfHairM02]->load(path + "dwarf-hair-m02.cub", database, false);
    m_sprites[SpriteDwarfHairM03]->load(path + "dwarf-hair-m03.cub", database, false);
    m_sprites[SpriteDwarfHeadF01]->load(path + "dwarf-head-f01.cub", database, true);
    m_sprites[SpriteDwarfHeadF02]->load(path + "dwarf-head-f02.cub", database, true);
    m_sprites[SpriteDwarfHeadF03]->load(path + "dwarf-head-f03.cub", database, true);
    m_sprites[SpriteDwarfHeadF04]->load(path + "dwarf-head-f04.cub", database, true);
    m_sprites[SpriteDwarfHeadF05]->load(path + "dwarf-head-f05.cub", database, true);
    m_sprites[SpriteDwarfHairF01]->load(path + "dwarf-hair-f01.cub", database, false);
    m_sprites[SpriteDwarfHairF02]->load(path + "dwarf-hair-f02.cub", database, false);
    m_sprites[SpriteDwarfHairF03]->load(path + "dwarf-hair-f03.cub", database, false);
    m_sprites[SpriteDwarfHairF04]->load(path + "dwarf-hair-f04.cub", database, false);
    m_sprites[SpriteDwarfHairF05]->load(path + "dwarf-hair-f05.cub", database, false);
    m_sprites[SpriteDwarfBody]->load(path + "dwarf-body.cub", database, true);
    m_sprites[SpriteDwarfBodyFemale]->load(path + "dwarf-body-female.cub", database, true);
    m_sprites[SpriteOrcHeadM01]->load(path + "orc-head.cub", database, true);
    m_sprites[SpriteOrcHand]->load(path + "orc-hand.cub", database, true);
    m_sprites[SpriteOrcHeadF01]->load(path + "orc-head-f01.cub", database, true);
    m_sprites[SpriteOrcHeadF02]->load(path + "orc-head-f02.cub", database, true);
    m_sprites[SpriteOrcHeadF03]->load(path + "orc-head-f03.cub", database, true);
    m_sprites[SpriteOrcHeadF04]->load(path + "orc-head-f04.cub", database, true);
    m_sprites[SpriteOrcHeadF05]->load(path + "orc-head-f05.cub", database, true);
    m_sprites[SpriteUndeadHeadM01]->load(path + "undead-head-m01.cub", database, true);
    m_sprites[SpriteUndeadHeadM02]->load(path + "undead-head-m02.cub", database, true);
    m_sprites[SpriteUndeadHeadM03]->load(path + "undead-head-m03.cub", database, true);
    m_sprites[SpriteUndeadHeadM04]->load(path + "undead-head-m04.cub", database, true);
    m_sprites[SpriteUndeadHeadM05]->load(path + "undead-head-m05.cub", database, true);
    m_sprites[SpriteUndeadHeadM06]->load(path + "undead-head-m06.cub", database, true);
    m_sprites[SpriteUndeadHairM01]->load(path + "undead-hair-m01.cub", database, false);
    m_sprites[SpriteUndeadHairM02]->load(path + "undead-hair-m02.cub", database, false);
    m_sprites[SpriteUndeadHairM03]->load(path + "undead-hair-m03.cub", database, false);
    m_sprites[SpriteUndeadHairM04]->load(path + "undead-hair-m04.cub", database, false);
    m_sprites[SpriteUndeadHairM05]->load(path + "undead-hair-m05.cub", database, false);
    m_sprites[SpriteUndeadHairM06]->load(path + "undead-hair-m06.cub", database, false);
    m_sprites[SpriteUndeadHeadF01]->load(path + "undead-head-f01.cub", database, true);
    m_sprites[SpriteUndeadHeadF02]->load(path + "undead-head-f02.cub", database, true);
    m_sprites[SpriteUndeadHeadF03]->load(path + "undead-head-f03.cub", database, true);
    m_sprites[SpriteUndeadHeadF04]->load(path + "undead-head-f04.cub", database, true);
    m_sprites[SpriteUndeadHeadF05]->load(path + "undead-head-f05.cub", database, true);
    m_sprites[SpriteUndeadHeadF06]->load(path + "undead-head-f06.cub", database, true);
    m_sprites[SpriteUndeadHairF01]->load(path + "undead-hair-f01.cub", database, false);
    m_sprites[SpriteUndeadHairF02]->load(path + "undead-hair-f02.cub", database, false);
    m_sprites[SpriteUndeadHairF03]->load(path + "undead-hair-f03.cub", database, false);
    m_sprites[SpriteUndeadHairF04]->load(path + "undead-hair-f04.cub", database, false);
    m_sprites[SpriteUndeadHairF05]->load(path + "undead-hair-f05.cub", database, false);
    m_sprites[SpriteUndeadHairF06]->load(path + "undead-hair-f06.cub", database, false);
    m_sprites[SpriteUndeadHand]->load(path + "undead-hand.cub", database, true);
    m_sprites[SpriteSkeletonEyes]->load(path + "skeleton-eyes.cub", database, false);
    m_sprites[SpriteSkeletonHead]->load(path + "skeleton-head.cub", database, true);
    m_sprites[SpriteSkeletonHand]->load(path + "skeleton-hand.cub", database, true);
    m_sprites[SpriteSkeletonBody]->load(path + "skeleton-body.cub", database, true);
    m_sprites[SpriteSkeletonFoot]->load(path + "skeleton-foot.cub", database, true);
    m_sprites[SpriteWizardHead]->load(path + "wizard-head.cub", database, true);
    m_sprites[SpriteWizardBody]->load(path + "wizard-body.cub", database, true);
    m_sprites[SpriteWitchHead]->load(path + "witch-head.cub", database, true);
    m_sprites[SpriteWitchBody]->load(path + "witch-body.cub", database, true);
    m_sprites[SpriteSantaHead]->load(path + "santa-head.cub", database, true);
    m_sprites[SpriteSantaBody]->load(path + "santa-body.cub", database, true);
    m_sprites[SpriteWerewolfBody]->load(path + "werewolf-body.cub", database, true);
    m_sprites[SpriteWerewolfHand]->load(path + "werewolf-hand.cub", database, true);
    m_sprites[SpriteWerewolfHead]->load(path + "werewolf-head.cub", database, true);
    m_sprites[SpriteWerewolfFoot]->load(path + "werewolf-foot.cub", database, true);
    m_sprites[SpriteZombieBody]->load(path + "zombie-body.cub", database, true);
    m_sprites[SpriteZombieHand]->load(path + "zombie-hand.cub", database, true);
    m_sprites[SpriteZombieHead]->load(path + "zombie-head.cub", database, true);
    m_sprites[SpriteZombieFoot]->load(path + "foot.cub", database, true);
    m_sprites[SpriteLichBody]->load(path + "lich-body.cub", database, true);
    m_sprites[SpriteLichHand]->load(path + "lich-hand.cub", database, true);
    m_sprites[SpriteLichHead]->load(path + "lich-head.cub", database, true);
    m_sprites[SpriteLichArm]->load(path + "lich-arm.cub", database, true);
    m_sprites[SpriteFrogBody]->load(path + "frog-body.cub", database, true);
    m_sprites[SpriteFrogHand]->load(path + "frog-hand.cub", database, true);
    m_sprites[SpriteFrogHead]->load(path + "frog-head.cub", database, true);
    m_sprites[SpriteFrogFoot]->load(path + "frog-foot.cub", database, true);
    m_sprites[SpritePlantCreatureBody]->load(path + "plant-creature-body.cub", database, true);
    m_sprites[SpritePlantCreatureHand]->load(path + "plant-creature-hand.cub", database, true);
    m_sprites[SpritePlantCreatureHead]->load(path + "plant-creature-head.cub", database, true);
    m_sprites[SpritePlantCreatureFoot]->load(path + "plant-creature-foot.cub", database, true);
    m_sprites[SpriteRadishCreatureHead]->load(path + "radish-creature-head.cub", database, true);
    m_sprites[SpriteRadishCreatureHand]->load(path + "radish-creature-hand.cub", database, true);
    m_sprites[SpriteRadishCreatureFoot]->load(path + "radish-creature-foot.cub", database, true);
    m_sprites[SpriteOnionlingHead]->load(path + "onionling-head.cub", database, true);
    m_sprites[SpriteOnionlingFoot]->load(path + "onionling-foot.cub", database, true);
    m_sprites[SpriteDesertOnionling]->load(path + "desert-onionling.cub", database, true);
    m_sprites[SpriteDesertOnionlingFoot]->load(path + "desert-onionling-foot.cub", database, true);
    m_sprites[SpriteDevourerHead]->load(path + "devourer-head.cub", database, true);
    m_sprites[SpriteDevourerFoot]->load(path + "devourer-foot.cub", database, true);
    m_sprites[SpriteMoleBody]->load(path + "mole-body.cub", database, true);
    m_sprites[SpriteMoleHand]->load(path + "mole-hand.cub", database, true);
    m_sprites[SpriteMoleHead]->load(path + "mole-head.cub", database, true);
    m_sprites[SpriteMoleFoot]->load(path + "mole-foot.cub", database, true);
    m_sprites[SpriteBiterBody]->load(path + "biter-body.cub", database, true);
    m_sprites[SpriteBiterHand]->load(path + "biter-hand.cub", database, true);
    m_sprites[SpriteBiterHead]->load(path + "biter-head.cub", database, true);
    m_sprites[SpriteBiterFoot]->load(path + "biter-foot.cub", database, true);
    m_sprites[SpriteKoalaBody]->load(path + "koala-body.cub", database, true);
    m_sprites[SpriteKoalaHand]->load(path + "koala-hand.cub", database, true);
    m_sprites[SpriteKoalaHead]->load(path + "koala-head.cub", database, true);
    m_sprites[SpriteKoalaFoot]->load(path + "koala-foot.cub", database, true);
    m_sprites[SpriteSquirrelBody]->load(path + "squirrel-body.cub", database, true);
    m_sprites[SpriteSquirrelHand]->load(path + "squirrel-hand.cub", database, true);
    m_sprites[SpriteSquirrelHead]->load(path + "squirrel-head.cub", database, true);
    m_sprites[SpriteSquirrelFoot]->load(path + "squirrel-foot.cub", database, true);
    m_sprites[SpriteRaccoonBody]->load(path + "raccoon-body.cub", database, true);
    m_sprites[SpriteRaccoonHand]->load(path + "raccoon-hand.cub", database, true);
    m_sprites[SpriteRaccoonHead]->load(path + "raccoon-head.cub", database, true);
    m_sprites[SpriteRaccoonFoot]->load(path + "raccoon-foot.cub", database, true);
    m_sprites[SpriteOwlHand]->load(path + "owl-hand.cub", database, true);
    m_sprites[SpriteOwlHead]->load(path + "owl-head.cub", database, true);
    m_sprites[SpriteOwlFoot]->load(path + "owl-foot.cub", database, true);
    m_sprites[SpritePenguinBody]->load(path + "penguin-body.cub", database, true);
    m_sprites[SpritePenguinHand]->load(path + "penguin-hand.cub", database, true);
    m_sprites[SpritePenguinHead]->load(path + "penguin-head.cub", database, true);
    m_sprites[SpritePenguinFoot]->load(path + "penguin-foot.cub", database, true);
    m_sprites[SpriteCrabBody]->load(path + "crab-body.cub", database, true);
    m_sprites[SpriteCrabHand]->load(path + "crab-hand.cub", database, true);
    m_sprites[SpriteCrabHead]->load(path + "crab-head.cub", database, true);
    m_sprites[SpriteCrabFoot]->load(path + "crab-foot.cub", database, true);
    m_sprites[SpriteHornetBody]->load(path + "hornet-body.cub", database, true);
    m_sprites[SpriteHornetHand]->load(path + "hornet-hand.cub", database, true);
    m_sprites[SpriteHornetHead]->load(path + "hornet-head.cub", database, true);
    m_sprites[SpriteHornetFoot]->load(path + "hornet-foot.cub", database, true);
    m_sprites[SpriteInsectGuardBody]->load(path + "insect-guard-body.cub", database, true);
    m_sprites[SpriteInsectGuardWing]->load(path + "insect-guard-wing.cub", database, true);
    m_sprites[SpriteInsectGuardHead]->load(path + "insect-guard-head.cub", database, true);
    m_sprites[SpriteInsectGuardHand]->load(path + "insect-guard-hand.cub", database, true);
    m_sprites[SpriteInsectGuardFoot]->load(path + "insect-guard-foot.cub", database, true);
    m_sprites[SpriteFlyBody]->load(path + "fly-body.cub", database, true);
    m_sprites[SpriteFlyHand]->load(path + "fly-hand.cub", database, true);
    m_sprites[SpriteFlyHead]->load(path + "fly-head.cub", database, true);
    m_sprites[SpriteFlyFoot]->load(path + "fly-foot.cub", database, true);
    m_sprites[SpriteBumblebee]->load(path + "bumblebee.cub", database, true);
    m_sprites[SpriteBumblebeeHand]->load(path + "bumblebee-hand.cub", database, true);
    m_sprites[SpriteBumblebeeFoot]->load(path + "bumblebee-foot.cub", database, true);
    m_sprites[SpriteMidgeBody]->load(path + "midge-body.cub", database, true);
    m_sprites[SpriteMidgeHand]->load(path + "midge-hand.cub", database, true);
    m_sprites[SpriteMidgeHead]->load(path + "midge-head.cub", database, true);
    m_sprites[SpriteMidgeFoot]->load(path + "midge-foot.cub", database, true);
    m_sprites[SpriteMosquitoBody]->load(path + "mosquito-body.cub", database, true);
    m_sprites[SpriteMosquitoHand]->load(path + "mosquito-hand.cub", database, true);
    m_sprites[SpriteMosquitoHead]->load(path + "mosquito-head.cub", database, true);
    m_sprites[SpriteMosquitoFoot]->load(path + "mosquito-foot.cub", database, true);
    m_sprites[SpriteGinseng]->load(path + "ginseng.cub", database, true);
    m_sprites[SpriteGinsengRoot]->load(path + "ginseng-root.cub", database, true);
    m_sprites[SpriteCrowBody]->load(path + "crow-body.cub", database, true);
    m_sprites[SpriteCrowHand]->load(path + "crow-hand.cub", database, true);
    m_sprites[SpriteCrowHead]->load(path + "crow-head.cub", database, true);
    m_sprites[SpriteCrowFoot]->load(path + "crow-foot.cub", database, true);
    m_sprites[SpriteChickenBody]->load(path + "chicken-body.cub", database, true);
    m_sprites[SpriteChickenHand]->load(path + "chicken-hand.cub", database, true);
    m_sprites[SpriteChickenHead]->load(path + "chicken-head.cub", database, true);
    m_sprites[SpriteChickenFoot]->load(path + "chicken-foot.cub", database, true);
    m_sprites[SpriteParrotBody]->load(path + "parrot-body.cub", database, true);
    m_sprites[SpriteParrotHand]->load(path + "parrot-hand.cub", database, true);
    m_sprites[SpriteParrotHead]->load(path + "parrot-head.cub", database, true);
    m_sprites[SpriteParrotFoot]->load(path + "parrot-foot.cub", database, true);
    m_sprites[SpriteParrotBlueBody]->load(path + "parrot-blue-body.cub", database, true);
    m_sprites[SpriteParrotBlueHand]->load(path + "parrot-blue-hand.cub", database, true);
    m_sprites[SpriteParrotBlueHead]->load(path + "parrot-blue-head.cub", database, true);
    m_sprites[SpriteParrotBlueFoot]->load(path + "parrot-blue-foot.cub", database, true);
    m_sprites[SpriteBarkbeetleHead]->load(path + "barkbeetle-head.cub", database, true);
    m_sprites[SpriteBarkbeetleFoot]->load(path + "barkbeetle-foot.cub", database, true);
    m_sprites[SpriteFirebeetleHead]->load(path + "firebeetle-head.cub", database, true);
    m_sprites[SpriteFirebeetleFoot]->load(path + "firebeetle-foot.cub", database, true);
    m_sprites[SpriteSnoutbeetleHead]->load(path + "snoutbeetle-head.cub", database, true);
    m_sprites[SpriteSnoutbeetleFoot]->load(path + "snoutbeetle-foot.cub", database, true);
    m_sprites[SpriteLemonbeetleHead]->load(path + "lemonbeetle-head.cub", database, true);
    m_sprites[SpriteLemonbeetleFoot]->load(path + "lemonbeetle-foot.cub", database, true);
    m_sprites[SpriteSeagullBody]->load(path + "seagull-body.cub", database, true);
    m_sprites[SpriteSeagullHand]->load(path + "seagull-hand.cub", database, true);
    m_sprites[SpriteSeagullHead]->load(path + "seagull-head.cub", database, true);
    m_sprites[SpriteSeagullFoot]->load(path + "seagull-foot.cub", database, true);
    m_sprites[SpriteBatBody]->load(path + "bat-body.cub", database, true);
    m_sprites[SpriteBatHand]->load(path + "bat-hand.cub", database, true);
    m_sprites[SpriteBatHead]->load(path + "bat-head.cub", database, true);
    m_sprites[SpriteBatFoot]->load(path + "bat-foot.cub", database, true);
    m_sprites[SpritePonyBody]->load(path + "pony-body.cub", database, true);
    m_sprites[SpritePonyHead]->load(path + "pony-head.cub", database, true);
    m_sprites[SpritePonyFoot]->load(path + "pony-foot.cub", database, true);
    m_sprites[SpritePonyTail]->load(path + "pony-tail.cub", database, true);
    m_sprites[SpriteCamelBody]->load(path + "camel-body.cub", database, true);
    m_sprites[SpriteCamelHead]->load(path + "camel-head.cub", database, true);
    m_sprites[SpriteCamelFoot]->load(path + "camel-foot.cub", database, true);
    m_sprites[SpriteCowBody]->load(path + "cow-body.cub", database, true);
    m_sprites[SpriteCowHead]->load(path + "cow-head.cub", database, true);
    m_sprites[SpriteCowFoot]->load(path + "cow-foot.cub", database, true);
    m_sprites[SpriteCollieBody]->load(path + "collie-body.cub", database, true);
    m_sprites[SpriteCollieHead]->load(path + "collie-head.cub", database, true);
    m_sprites[SpriteCollieFoot]->load(path + "collie-foot.cub", database, true);
    m_sprites[SpriteCollieHand]->load(path + "collie-hand.cub", database, true);
    m_sprites[SpriteCollieTail]->load(path + "collie-tail.cub", database, true);
    m_sprites[SpriteShepherdDogBody]->load(path + "shepherd-dog-body.cub", database, true);
    m_sprites[SpriteShepherdDogHead]->load(path + "shepherd-dog-head.cub", database, true);
    m_sprites[SpriteShepherdDogFoot]->load(path + "shepherd-dog-foot.cub", database, true);
    m_sprites[SpriteShepherdDogHand]->load(path + "shepherd-dog-hand.cub", database, true);
    m_sprites[SpriteShepherdDogTail]->load(path + "shepherd-dog-tail.cub", database, true);
    m_sprites[SpriteSkullBullBody]->load(path + "skull-bull-body.cub", database, true);
    m_sprites[SpriteSkullBullHead]->load(path + "skull-bull-head.cub", database, true);
    m_sprites[SpriteSkullBullFoot]->load(path + "skull-bull-foot.cub", database, true);
    m_sprites[SpriteSkullBullHand]->load(path + "skull-bull-hand.cub", database, true);
    m_sprites[SpriteSkullBullTail]->load(path + "skull-bull-tail.cub", database, true);
    m_sprites[SpriteAlpacaBody]->load(path + "alpaca-body.cub", database, true);
    m_sprites[SpriteAlpacaHead]->load(path + "alpaca-head.cub", database, true);
    m_sprites[SpriteAlpacaFoot]->load(path + "alpaca-foot.cub", database, true);
    m_sprites[SpriteAlpacaHand]->load(path + "alpaca-hand.cub", database, true);
    m_sprites[SpriteAlpacaBrownBody]->load(path + "alpaca-brown-body.cub", database, true);
    m_sprites[SpriteAlpacaBrownHead]->load(path + "alpaca-brown-head.cub", database, true);
    m_sprites[SpriteAlpacaBrownFoot]->load(path + "alpaca-brown-foot.cub", database, true);
    m_sprites[SpriteAlpacaBrownHand]->load(path + "alpaca-brown-hand.cub", database, true);
    m_sprites[SpriteTurtleBody]->load(path + "turtle-body.cub", database, true);
    m_sprites[SpriteTurtleHead]->load(path + "turtle-head.cub", database, true);
    m_sprites[SpriteTurtleFoot]->load(path + "turtle-foot.cub", database, true);
    m_sprites[SpriteDogBody2]->load(path + "dog-body2.cub", database, true);
    m_sprites[SpriteDogHead2]->load(path + "dog-head2.cub", database, true);
    m_sprites[SpriteDogFoot2]->load(path + "dog-foot2.cub", database, true);
    m_sprites[SpriteDogHand2]->load(path + "dog-hand2.cub", database, true);
    m_sprites[SpriteDogTail2]->load(path + "dog-tail2.cub", database, true);
    m_sprites[SpriteScottishTerrierBody]->load(path + "scottish-terrier-body.cub", database, true);
    m_sprites[SpriteScottishTerrierHead]->load(path + "scottish-terrier-head.cub", database, true);
    m_sprites[SpriteScottishTerrierFoot]->load(path + "scottish-terrier-foot.cub", database, true);
    m_sprites[SpriteScottishTerrierHand]->load(path + "scottish-terrier-hand.cub", database, true);
    m_sprites[SpriteScottishTerrierTail]->load(path + "scottish-terrier-tail.cub", database, true);
    m_sprites[SpriteWolfBody]->load(path + "wolf-body.cub", database, true);
    m_sprites[SpriteWolfHead]->load(path + "wolf-head.cub", database, true);
    m_sprites[SpriteWolfFoot]->load(path + "wolf-foot.cub", database, true);
    m_sprites[SpriteWolfHand]->load(path + "wolf-hand.cub", database, true);
    m_sprites[SpriteWolfTail]->load(path + "wolf-tail.cub", database, true);
    m_sprites[SpritePantherBody]->load(path + "panther-body.cub", database, true);
    m_sprites[SpritePantherHead]->load(path + "panther-head.cub", database, true);
    m_sprites[SpritePantherFoot]->load(path + "panther-foot.cub", database, true);
    m_sprites[SpritePantherHand]->load(path + "panther-hand.cub", database, true);
    m_sprites[SpritePantherTail]->load(path + "panther-tail.cub", database, true);
    m_sprites[SpriteCatBody]->load(path + "cat-body.cub", database, true);
    m_sprites[SpriteCatHead]->load(path + "cat-head.cub", database, true);
    m_sprites[SpriteCatFoot]->load(path + "cat-foot.cub", database, true);
    m_sprites[SpriteCatHand]->load(path + "cat-hand.cub", database, true);
    m_sprites[SpriteCatTail]->load(path + "cat-tail.cub", database, true);
    m_sprites[SpriteCatBody2]->load(path + "cat-body2.cub", database, true);
    m_sprites[SpriteCatHead2]->load(path + "cat-head2.cub", database, true);
    m_sprites[SpriteCatFoot2]->load(path + "cat-foot2.cub", database, true);
    m_sprites[SpriteCatHand2]->load(path + "cat-hand2.cub", database, true);
    m_sprites[SpriteCatTail2]->load(path + "cat-tail2.cub", database, true);
    m_sprites[SpriteCatBody3]->load(path + "cat-body3.cub", database, true);
    m_sprites[SpriteCatHead3]->load(path + "cat-head3.cub", database, true);
    m_sprites[SpriteCatFoot3]->load(path + "cat-foot3.cub", database, true);
    m_sprites[SpriteCatHand3]->load(path + "cat-hand3.cub", database, true);
    m_sprites[SpriteCatTail3]->load(path + "cat-tail3.cub", database, true);
    m_sprites[SpriteDragonBody]->load(path + "dragon-body.cub", database, true);
    m_sprites[SpriteDragonHead]->load(path + "dragon-head.cub", database, true);
    m_sprites[SpriteDragonFoot]->load(path + "dragon-foot.cub", database, true);
    m_sprites[SpriteDragonWing]->load(path + "dragon-wing.cub", database, true);
    m_sprites[SpriteDragonTail]->load(path + "dragon-tail.cub", database, true);
    m_sprites[SpriteCrabBodyBlue]->load(path + "crab-body-blue.cub", database, true);
    m_sprites[SpriteCrabHandBlue]->load(path + "crab-hand-blue.cub", database, true);
    m_sprites[SpriteCrabHeadBlue]->load(path + "crab-head-blue.cub", database, true);
    m_sprites[SpriteCrabFootBlue]->load(path + "crab-foot-blue.cub", database, true);
    m_sprites[SpritePlainrunnerBody]->load(path + "plainrunner-body.cub", database, true);
    m_sprites[SpritePlainrunnerFoot]->load(path + "plainrunner-foot.cub", database, true);
    m_sprites[SpriteLeafrunnerBody]->load(path + "leafrunner-body.cub", database, true);
    m_sprites[SpriteLeafrunnerFoot]->load(path + "leafrunner-foot.cub", database, true);
    m_sprites[SpriteSnowrunnerBody]->load(path + "snowrunner-body.cub", database, true);
    m_sprites[SpriteSnowrunnerFoot]->load(path + "snowrunner-foot.cub", database, true);
    m_sprites[SpriteDesertrunnerBody]->load(path + "desertrunner-body.cub", database, true);
    m_sprites[SpriteDesertrunnerFoot]->load(path + "desertrunner-foot.cub", database, true);
    m_sprites[SpritePeacockBody]->load(path + "peacock-body.cub", database, true);
    m_sprites[SpritePeacockFoot]->load(path + "peacock-foot.cub", database, true);
    m_sprites[SpritePeacockHead]->load(path + "peacock-head.cub", database, true);
    m_sprites[SpriteWoodStaff1]->load(path + "wood-staff1.cub", database, false);
    m_sprites[SpriteWoodStaff2]->load(path + "wood-staff2.cub", database, false);
    m_sprites[SpriteWoodStaff3]->load(path + "wood-staff3.cub", database, false);
    m_sprites[SpriteWoodStaff4]->load(path + "wood-staff4.cub", database, false);
    m_sprites[SpriteWoodStaff6]->load(path + "wood-staff6.cub", database, false);
    m_sprites[SpriteWoodStaff1Random1]->load(path + "wood-staff1-random1.cub", database, false);
    m_sprites[SpriteWoodStaff2Random1]->load(path + "wood-staff2-random1.cub", database, false);
    m_sprites[SpriteWoodStaff3Random1]->load(path + "wood-staff3-random1.cub", database, false);
    m_sprites[SpriteWoodStaff4Random1]->load(path + "wood-staff4-random1.cub", database, false);
    m_sprites[SpriteWoodStaff5Random1]->load(path + "wood-staff5-random1.cub", database, false);
    m_sprites[SpriteWoodStaff1Random2]->load(path + "wood-staff1-random2.cub", database, false);
    m_sprites[SpriteWoodStaff2Random2]->load(path + "wood-staff2-random2.cub", database, false);
    m_sprites[SpriteWoodStaff3Random2]->load(path + "wood-staff3-random2.cub", database, false);
    m_sprites[SpriteWoodStaff4Random2]->load(path + "wood-staff4-random2.cub", database, false);
    m_sprites[SpriteWoodStaff5Random2]->load(path + "wood-staff5-random2.cub", database, false);
    m_sprites[SpriteWoodStaff1Random3]->load(path + "wood-staff1-random3.cub", database, false);
    m_sprites[SpriteWoodStaff2Random3]->load(path + "wood-staff2-random3.cub", database, false);
    m_sprites[SpriteWoodStaff3Random3]->load(path + "wood-staff3-random3.cub", database, false);
    m_sprites[SpriteWoodStaff4Random3]->load(path + "wood-staff4-random3.cub", database, false);
    m_sprites[SpriteWoodStaff5Random3]->load(path + "wood-staff5-random3.cub", database, false);
    m_sprites[SpriteWoodStaff1Random4]->load(path + "wood-staff1-random4.cub", database, false);
    m_sprites[SpriteWoodStaff2Random4]->load(path + "wood-staff2-random4.cub", database, false);
    m_sprites[SpriteWoodStaff3Random4]->load(path + "wood-staff3-random4.cub", database, false);
    m_sprites[SpriteWoodStaff4Random4]->load(path + "wood-staff4-random4.cub", database, false);
    m_sprites[SpriteWoodStaff5Random4]->load(path + "wood-staff5-random4.cub", database, false);
    m_sprites[SpriteWoodStaff1Random5]->load(path + "wood-staff1-random5.cub", database, false);
    m_sprites[SpriteWoodStaff2Random5]->load(path + "wood-staff2-random5.cub", database, false);
    m_sprites[SpriteWoodStaff3Random5]->load(path + "wood-staff3-random5.cub", database, false);
    m_sprites[SpriteWoodStaff4Random5]->load(path + "wood-staff4-random5.cub", database, false);
    m_sprites[SpriteWoodStaff5Random5]->load(path + "wood-staff5-random5.cub", database, false);
    m_sprites[SpriteWoodStaff1Random6]->load(path + "wood-staff1-random6.cub", database, false);
    m_sprites[SpriteWoodStaff2Random6]->load(path + "wood-staff2-random6.cub", database, false);
    m_sprites[SpriteWoodStaff3Random6]->load(path + "wood-staff3-random6.cub", database, false);
    m_sprites[SpriteWoodStaff4Random6]->load(path + "wood-staff4-random6.cub", database, false);
    m_sprites[SpriteWoodStaff5Random6]->load(path + "wood-staff5-random6.cub", database, false);
    m_sprites[SpriteWoodStaff1Random7]->load(path + "wood-staff1-random7.cub", database, false);
    m_sprites[SpriteWoodStaff2Random7]->load(path + "wood-staff2-random7.cub", database, false);
    m_sprites[SpriteWoodStaff3Random7]->load(path + "wood-staff3-random7.cub", database, false);
    m_sprites[SpriteWoodStaff4Random7]->load(path + "wood-staff4-random7.cub", database, false);
    m_sprites[SpriteWoodStaff5Random7]->load(path + "wood-staff5-random7.cub", database, false);
    m_sprites[SpriteWoodStaff1Random8]->load(path + "wood-staff1-random8.cub", database, false);
    m_sprites[SpriteWoodStaff2Random8]->load(path + "wood-staff2-random8.cub", database, false);
    m_sprites[SpriteWoodStaff3Random8]->load(path + "wood-staff3-random8.cub", database, false);
    m_sprites[SpriteWoodStaff4Random8]->load(path + "wood-staff4-random8.cub", database, false);
    m_sprites[SpriteWoodStaff5Random8]->load(path + "wood-staff5-random8.cub", database, false);
    m_sprites[SpriteWoodStaff1Random9]->load(path + "wood-staff1-random9.cub", database, false);
    m_sprites[SpriteWoodStaff2Random9]->load(path + "wood-staff2-random9.cub", database, false);
    m_sprites[SpriteWoodStaff3Random9]->load(path + "wood-staff3-random9.cub", database, false);
    m_sprites[SpriteWoodStaff4Random9]->load(path + "wood-staff4-random9.cub", database, false);
    m_sprites[SpriteWoodStaff5Random9]->load(path + "wood-staff5-random9.cub", database, false);
    m_sprites[SpriteWoodStaff1Random10]->load(path + "wood-staff1-random10.cub", database, false);
    m_sprites[SpriteWoodStaff2Random10]->load(path + "wood-staff2-random10.cub", database, false);
    m_sprites[SpriteWoodStaff3Random10]->load(path + "wood-staff3-random10.cub", database, false);
    m_sprites[SpriteWoodStaff4Random10]->load(path + "wood-staff4-random10.cub", database, false);
    m_sprites[SpriteWoodStaff5Random10]->load(path + "wood-staff5-random10.cub", database, false);
    m_sprites[SpriteObsidianStaff1]->load(path + "obsidian-staff1.cub", database, false);
    m_sprites[SpriteObsidianStaff2]->load(path + "obsidian-staff2.cub", database, false);
    m_sprites[SpriteObsidianStaff3]->load(path + "obsidian-staff3.cub", database, false);
    m_sprites[SpriteObsidianStaff4]->load(path + "obsidian-staff4.cub", database, false);
    m_sprites[SpriteObsidianStaff5]->load(path + "obsidian-staff5.cub", database, false);
    m_sprites[SpriteWoodWand1]->load(path + "wood-wand1.cub", database, false);
    m_sprites[SpriteWoodWand2]->load(path + "wood-wand2.cub", database, false);
    m_sprites[SpriteWoodWand3]->load(path + "wood-wand3.cub", database, false);
    m_sprites[SpriteWoodWand4]->load(path + "wood-wand4.cub", database, false);
    m_sprites[SpriteWoodWand5]->load(path + "wood-wand5.cub", database, false);
    m_sprites[SpriteWoodWand1Random1]->load(path + "wood-wand1-random1.cub", database, false);
    m_sprites[SpriteWoodWand2Random1]->load(path + "wood-wand2-random1.cub", database, false);
    m_sprites[SpriteWoodWand3Random1]->load(path + "wood-wand3-random1.cub", database, false);
    m_sprites[SpriteWoodWand4Random1]->load(path + "wood-wand4-random1.cub", database, false);
    m_sprites[SpriteWoodWand5Random1]->load(path + "wood-wand5-random1.cub", database, false);
    m_sprites[SpriteWoodWand1Random2]->load(path + "wood-wand1-random2.cub", database, false);
    m_sprites[SpriteWoodWand2Random2]->load(path + "wood-wand2-random2.cub", database, false);
    m_sprites[SpriteWoodWand3Random2]->load(path + "wood-wand3-random2.cub", database, false);
    m_sprites[SpriteWoodWand4Random2]->load(path + "wood-wand4-random2.cub", database, false);
    m_sprites[SpriteWoodWand5Random2]->load(path + "wood-wand5-random2.cub", database, false);
    m_sprites[SpriteWoodWand1Random3]->load(path + "wood-wand1-random3.cub", database, false);
    m_sprites[SpriteWoodWand2Random3]->load(path + "wood-wand2-random3.cub", database, false);
    m_sprites[SpriteWoodWand3Random3]->load(path + "wood-wand3-random3.cub", database, false);
    m_sprites[SpriteWoodWand4Random3]->load(path + "wood-wand4-random3.cub", database, false);
    m_sprites[SpriteWoodWand5Random3]->load(path + "wood-wand5-random3.cub", database, false);
    m_sprites[SpriteWoodWand1Random4]->load(path + "wood-wand1-random4.cub", database, false);
    m_sprites[SpriteWoodWand2Random4]->load(path + "wood-wand2-random4.cub", database, false);
    m_sprites[SpriteWoodWand3Random4]->load(path + "wood-wand3-random4.cub", database, false);
    m_sprites[SpriteWoodWand4Random4]->load(path + "wood-wand4-random4.cub", database, false);
    m_sprites[SpriteWoodWand5Random4]->load(path + "wood-wand5-random4.cub", database, false);
    m_sprites[SpriteWoodWand1Random5]->load(path + "wood-wand1-random5.cub", database, false);
    m_sprites[SpriteWoodWand2Random5]->load(path + "wood-wand2-random5.cub", database, false);
    m_sprites[SpriteWoodWand3Random5]->load(path + "wood-wand3-random5.cub", database, false);
    m_sprites[SpriteWoodWand4Random5]->load(path + "wood-wand4-random5.cub", database, false);
    m_sprites[SpriteWoodWand5Random5]->load(path + "wood-wand5-random5.cub", database, false);
    m_sprites[SpriteWoodWand1Random6]->load(path + "wood-wand1-random6.cub", database, false);
    m_sprites[SpriteWoodWand2Random6]->load(path + "wood-wand2-random6.cub", database, false);
    m_sprites[SpriteWoodWand3Random6]->load(path + "wood-wand3-random6.cub", database, false);
    m_sprites[SpriteWoodWand4Random6]->load(path + "wood-wand4-random6.cub", database, false);
    m_sprites[SpriteWoodWand5Random6]->load(path + "wood-wand5-random6.cub", database, false);
    m_sprites[SpriteWoodWand1Random7]->load(path + "wood-wand1-random7.cub", database, false);
    m_sprites[SpriteWoodWand2Random7]->load(path + "wood-wand2-random7.cub", database, false);
    m_sprites[SpriteWoodWand3Random7]->load(path + "wood-wand3-random7.cub", database, false);
    m_sprites[SpriteWoodWand4Random7]->load(path + "wood-wand4-random7.cub", database, false);
    m_sprites[SpriteWoodWand5Random7]->load(path + "wood-wand5-random7.cub", database, false);
    m_sprites[SpriteWoodWand1Random8]->load(path + "wood-wand1-random8.cub", database, false);
    m_sprites[SpriteWoodWand2Random8]->load(path + "wood-wand2-random8.cub", database, false);
    m_sprites[SpriteWoodWand3Random8]->load(path + "wood-wand3-random8.cub", database, false);
    m_sprites[SpriteWoodWand4Random8]->load(path + "wood-wand4-random8.cub", database, false);
    m_sprites[SpriteWoodWand5Random8]->load(path + "wood-wand5-random8.cub", database, false);
    m_sprites[SpriteWoodWand1Random9]->load(path + "wood-wand1-random9.cub", database, false);
    m_sprites[SpriteWoodWand2Random9]->load(path + "wood-wand2-random9.cub", database, false);
    m_sprites[SpriteWoodWand3Random9]->load(path + "wood-wand3-random9.cub", database, false);
    m_sprites[SpriteWoodWand4Random9]->load(path + "wood-wand4-random9.cub", database, false);
    m_sprites[SpriteWoodWand5Random9]->load(path + "wood-wand5-random9.cub", database, false);
    m_sprites[SpriteWoodWand1Random10]->load(path + "wood-wand1-random10.cub", database, false);
    m_sprites[SpriteWoodWand2Random10]->load(path + "wood-wand2-random10.cub", database, false);
    m_sprites[SpriteWoodWand3Random10]->load(path + "wood-wand3-random10.cub", database, false);
    m_sprites[SpriteWoodWand4Random10]->load(path + "wood-wand4-random10.cub", database, false);
    m_sprites[SpriteWoodWand5Random10]->load(path + "wood-wand5-random10.cub", database, false);
    m_sprites[SpriteGoldBracelet1]->load(path + "gold-bracelet1.cub", database, false);
    m_sprites[SpriteGoldBracelet2]->load(path + "gold-bracelet2.cub", database, false);
    m_sprites[SpriteGoldBracelet3]->load(path + "gold-bracelet3.cub", database, false);
    m_sprites[SpriteGoldBracelet4]->load(path + "gold-bracelet4.cub", database, false);
    m_sprites[SpriteGoldBracelet5]->load(path + "gold-bracelet5.cub", database, false);
    m_sprites[SpriteGoldBracelet1Random1]->load(path + "gold-bracelet1-random1.cub", database, false);
    m_sprites[SpriteGoldBracelet2Random1]->load(path + "gold-bracelet2-random1.cub", database, false);
    m_sprites[SpriteGoldBracelet3Random1]->load(path + "gold-bracelet3-random1.cub", database, false);
    m_sprites[SpriteGoldBracelet4Random1]->load(path + "gold-bracelet4-random1.cub", database, false);
    m_sprites[SpriteGoldBracelet5Random1]->load(path + "gold-bracelet5-random1.cub", database, false);
    m_sprites[SpriteGoldBracelet1Random2]->load(path + "gold-bracelet1-random2.cub", database, false);
    m_sprites[SpriteGoldBracelet2Random2]->load(path + "gold-bracelet2-random2.cub", database, false);
    m_sprites[SpriteGoldBracelet3Random2]->load(path + "gold-bracelet3-random2.cub", database, false);
    m_sprites[SpriteGoldBracelet4Random2]->load(path + "gold-bracelet4-random2.cub", database, false);
    m_sprites[SpriteGoldBracelet5Random2]->load(path + "gold-bracelet5-random2.cub", database, false);
    m_sprites[SpriteGoldBracelet1Random3]->load(path + "gold-bracelet1-random3.cub", database, false);
    m_sprites[SpriteGoldBracelet2Random3]->load(path + "gold-bracelet2-random3.cub", database, false);
    m_sprites[SpriteGoldBracelet3Random3]->load(path + "gold-bracelet3-random3.cub", database, false);
    m_sprites[SpriteGoldBracelet4Random3]->load(path + "gold-bracelet4-random3.cub", database, false);
    m_sprites[SpriteGoldBracelet5Random3]->load(path + "gold-bracelet5-random3.cub", database, false);
    m_sprites[SpriteGoldBracelet1Random4]->load(path + "gold-bracelet1-random4.cub", database, false);
    m_sprites[SpriteGoldBracelet2Random4]->load(path + "gold-bracelet2-random4.cub", database, false);
    m_sprites[SpriteGoldBracelet3Random4]->load(path + "gold-bracelet3-random4.cub", database, false);
    m_sprites[SpriteGoldBracelet4Random4]->load(path + "gold-bracelet4-random4.cub", database, false);
    m_sprites[SpriteGoldBracelet5Random4]->load(path + "gold-bracelet5-random4.cub", database, false);
    m_sprites[SpriteGoldBracelet1Random5]->load(path + "gold-bracelet1-random5.cub", database, false);
    m_sprites[SpriteGoldBracelet2Random5]->load(path + "gold-bracelet2-random5.cub", database, false);
    m_sprites[SpriteGoldBracelet3Random5]->load(path + "gold-bracelet3-random5.cub", database, false);
    m_sprites[SpriteGoldBracelet4Random5]->load(path + "gold-bracelet4-random5.cub", database, false);
    m_sprites[SpriteGoldBracelet5Random5]->load(path + "gold-bracelet5-random5.cub", database, false);
    m_sprites[SpriteSilverBracelet1]->load(path + "silver-bracelet1.cub", database, false);
    m_sprites[SpriteSilverBracelet2]->load(path + "silver-bracelet2.cub", database, false);
    m_sprites[SpriteSilverBracelet3]->load(path + "silver-bracelet3.cub", database, false);
    m_sprites[SpriteSilverBracelet4]->load(path + "silver-bracelet4.cub", database, false);
    m_sprites[SpriteSilverBracelet5]->load(path + "silver-bracelet5.cub", database, false);
    m_sprites[SpriteSilverBracelet1Random1]->load(path + "silver-bracelet1-random1.cub", database, false);
    m_sprites[SpriteSilverBracelet2Random1]->load(path + "silver-bracelet2-random1.cub", database, false);
    m_sprites[SpriteSilverBracelet3Random1]->load(path + "silver-bracelet3-random1.cub", database, false);
    m_sprites[SpriteSilverBracelet4Random1]->load(path + "silver-bracelet4-random1.cub", database, false);
    m_sprites[SpriteSilverBracelet5Random1]->load(path + "silver-bracelet5-random1.cub", database, false);
    m_sprites[SpriteSilverBracelet1Random2]->load(path + "silver-bracelet1-random2.cub", database, false);
    m_sprites[SpriteSilverBracelet2Random2]->load(path + "silver-bracelet2-random2.cub", database, false);
    m_sprites[SpriteSilverBracelet3Random2]->load(path + "silver-bracelet3-random2.cub", database, false);
    m_sprites[SpriteSilverBracelet4Random2]->load(path + "silver-bracelet4-random2.cub", database, false);
    m_sprites[SpriteSilverBracelet5Random2]->load(path + "silver-bracelet5-random2.cub", database, false);
    m_sprites[SpriteSilverBracelet1Random3]->load(path + "silver-bracelet1-random3.cub", database, false);
    m_sprites[SpriteSilverBracelet2Random3]->load(path + "silver-bracelet2-random3.cub", database, false);
    m_sprites[SpriteSilverBracelet3Random3]->load(path + "silver-bracelet3-random3.cub", database, false);
    m_sprites[SpriteSilverBracelet4Random3]->load(path + "silver-bracelet4-random3.cub", database, false);
    m_sprites[SpriteSilverBracelet5Random3]->load(path + "silver-bracelet5-random3.cub", database, false);
    m_sprites[SpriteSilverBracelet1Random4]->load(path + "silver-bracelet1-random4.cub", database, false);
    m_sprites[SpriteSilverBracelet2Random4]->load(path + "silver-bracelet2-random4.cub", database, false);
    m_sprites[SpriteSilverBracelet3Random4]->load(path + "silver-bracelet3-random4.cub", database, false);
    m_sprites[SpriteSilverBracelet4Random4]->load(path + "silver-bracelet4-random4.cub", database, false);
    m_sprites[SpriteSilverBracelet5Random4]->load(path + "silver-bracelet5-random4.cub", database, false);
    m_sprites[SpriteSilverBracelet1Random5]->load(path + "silver-bracelet1-random5.cub", database, false);
    m_sprites[SpriteSilverBracelet2Random5]->load(path + "silver-bracelet2-random5.cub", database, false);
    m_sprites[SpriteSilverBracelet3Random5]->load(path + "silver-bracelet3-random5.cub", database, false);
    m_sprites[SpriteSilverBracelet4Random5]->load(path + "silver-bracelet4-random5.cub", database, false);
    m_sprites[SpriteSilverBracelet5Random5]->load(path + "silver-bracelet5-random5.cub", database, false);
    m_sprites[SpriteIronMace1]->load(path + "iron-mace1.cub", database, true);
    m_sprites[SpriteIronMace2]->load(path + "iron-mace2.cub", database, true);
    m_sprites[SpriteIronMace3]->load(path + "iron-mace3.cub", database, true);
    m_sprites[SpriteIronMace4]->load(path + "iron-mace4.cub", database, true);
    m_sprites[SpriteIronMace5]->load(path + "iron-mace5.cub", database, true);
    m_sprites[SpriteIronMace1Random1]->load(path + "iron-mace1-random1.cub", database, false);
    m_sprites[SpriteIronMace2Random1]->load(path + "iron-mace2-random1.cub", database, false);
    m_sprites[SpriteIronMace3Random1]->load(path + "iron-mace3-random1.cub", database, false);
    m_sprites[SpriteIronMace4Random1]->load(path + "iron-mace4-random1.cub", database, false);
    m_sprites[SpriteIronMace5Random1]->load(path + "iron-mace5-random1.cub", database, false);
    m_sprites[SpriteIronMace1Random2]->load(path + "iron-mace1-random2.cub", database, false);
    m_sprites[SpriteIronMace2Random2]->load(path + "iron-mace2-random2.cub", database, false);
    m_sprites[SpriteIronMace3Random2]->load(path + "iron-mace3-random2.cub", database, false);
    m_sprites[SpriteIronMace4Random2]->load(path + "iron-mace4-random2.cub", database, false);
    m_sprites[SpriteIronMace5Random2]->load(path + "iron-mace5-random2.cub", database, false);
    m_sprites[SpriteIronMace1Random3]->load(path + "iron-mace1-random3.cub", database, false);
    m_sprites[SpriteIronMace2Random3]->load(path + "iron-mace2-random3.cub", database, false);
    m_sprites[SpriteIronMace3Random3]->load(path + "iron-mace3-random3.cub", database, false);
    m_sprites[SpriteIronMace4Random3]->load(path + "iron-mace4-random3.cub", database, false);
    m_sprites[SpriteIronMace5Random3]->load(path + "iron-mace5-random3.cub", database, false);
    m_sprites[SpriteIronMace1Random4]->load(path + "iron-mace1-random4.cub", database, false);
    m_sprites[SpriteIronMace2Random4]->load(path + "iron-mace2-random4.cub", database, false);
    m_sprites[SpriteIronMace3Random4]->load(path + "iron-mace3-random4.cub", database, false);
    m_sprites[SpriteIronMace4Random4]->load(path + "iron-mace4-random4.cub", database, false);
    m_sprites[SpriteIronMace5Random4]->load(path + "iron-mace5-random4.cub", database, false);
    m_sprites[SpriteIronMace1Random5]->load(path + "iron-mace1-random5.cub", database, false);
    m_sprites[SpriteIronMace2Random5]->load(path + "iron-mace2-random5.cub", database, false);
    m_sprites[SpriteIronMace3Random5]->load(path + "iron-mace3-random5.cub", database, false);
    m_sprites[SpriteIronMace4Random5]->load(path + "iron-mace4-random5.cub", database, false);
    m_sprites[SpriteIronMace5Random5]->load(path + "iron-mace5-random5.cub", database, false);
    m_sprites[SpriteIronMace1Random6]->load(path + "iron-mace1-random6.cub", database, false);
    m_sprites[SpriteIronMace2Random6]->load(path + "iron-mace2-random6.cub", database, false);
    m_sprites[SpriteIronMace3Random6]->load(path + "iron-mace3-random6.cub", database, false);
    m_sprites[SpriteIronMace4Random6]->load(path + "iron-mace4-random6.cub", database, false);
    m_sprites[SpriteIronMace5Random6]->load(path + "iron-mace5-random6.cub", database, false);
    m_sprites[SpriteIronMace1Random7]->load(path + "iron-mace1-random7.cub", database, false);
    m_sprites[SpriteIronMace2Random7]->load(path + "iron-mace2-random7.cub", database, false);
    m_sprites[SpriteIronMace3Random7]->load(path + "iron-mace3-random7.cub", database, false);
    m_sprites[SpriteIronMace4Random7]->load(path + "iron-mace4-random7.cub", database, false);
    m_sprites[SpriteIronMace5Random7]->load(path + "iron-mace5-random7.cub", database, false);
    m_sprites[SpriteIronMace1Random8]->load(path + "iron-mace1-random8.cub", database, false);
    m_sprites[SpriteIronMace2Random8]->load(path + "iron-mace2-random8.cub", database, false);
    m_sprites[SpriteIronMace3Random8]->load(path + "iron-mace3-random8.cub", database, false);
    m_sprites[SpriteIronMace4Random8]->load(path + "iron-mace4-random8.cub", database, false);
    m_sprites[SpriteIronMace5Random8]->load(path + "iron-mace5-random8.cub", database, false);
    m_sprites[SpriteIronMace1Random9]->load(path + "iron-mace1-random9.cub", database, false);
    m_sprites[SpriteIronMace2Random9]->load(path + "iron-mace2-random9.cub", database, false);
    m_sprites[SpriteIronMace3Random9]->load(path + "iron-mace3-random9.cub", database, false);
    m_sprites[SpriteIronMace4Random9]->load(path + "iron-mace4-random9.cub", database, false);
    m_sprites[SpriteIronMace5Random9]->load(path + "iron-mace5-random9.cub", database, false);
    m_sprites[SpriteIronMace1Random10]->load(path + "iron-mace1-random10.cub", database, false);
    m_sprites[SpriteIronMace2Random10]->load(path + "iron-mace2-random10.cub", database, false);
    m_sprites[SpriteIronMace3Random10]->load(path + "iron-mace3-random10.cub", database, false);
    m_sprites[SpriteIronMace4Random10]->load(path + "iron-mace4-random10.cub", database, false);
    m_sprites[SpriteIronMace5Random10]->load(path + "iron-mace5-random10.cub", database, false);
    m_sprites[SpriteWoodMace01]->load(path + "wood-mace01.cub", database, true);
    m_sprites[SpriteWoodMace02]->load(path + "wood-mace02.cub", database, true);
    m_sprites[SpriteWoodMace03]->load(path + "wood-mace03.cub", database, true);
    m_sprites[SpriteWoodMace04]->load(path + "wood-mace04.cub", database, true);
    m_sprites[SpriteWoodMace05]->load(path + "wood-mace05.cub", database, true);
    m_sprites[SpriteBow]->load(path + "bow.cub", database, true);
    m_sprites[SpriteWoodBow2]->load(path + "wood-bow2.cub", database, true);
    m_sprites[SpriteWoodBow3]->load(path + "wood-bow3.cub", database, true);
    m_sprites[SpriteWoodBow4]->load(path + "wood-bow4.cub", database, true);
    m_sprites[SpriteWoodBow5]->load(path + "wood-bow5.cub", database, true);
    m_sprites[SpriteWoodBow1Random1]->load(path + "wood-bow1-random1.cub", database, false);
    m_sprites[SpriteWoodBow2Random1]->load(path + "wood-bow2-random1.cub", database, false);
    m_sprites[SpriteWoodBow3Random1]->load(path + "wood-bow3-random1.cub", database, false);
    m_sprites[SpriteWoodBow4Random1]->load(path + "wood-bow4-random1.cub", database, false);
    m_sprites[SpriteWoodBow5Random1]->load(path + "wood-bow5-random1.cub", database, false);
    m_sprites[SpriteWoodBow1Random2]->load(path + "wood-bow1-random2.cub", database, false);
    m_sprites[SpriteWoodBow2Random2]->load(path + "wood-bow2-random2.cub", database, false);
    m_sprites[SpriteWoodBow3Random2]->load(path + "wood-bow3-random2.cub", database, false);
    m_sprites[SpriteWoodBow4Random2]->load(path + "wood-bow4-random2.cub", database, false);
    m_sprites[SpriteWoodBow5Random2]->load(path + "wood-bow5-random2.cub", database, false);
    m_sprites[SpriteWoodBow1Random3]->load(path + "wood-bow1-random3.cub", database, false);
    m_sprites[SpriteWoodBow2Random3]->load(path + "wood-bow2-random3.cub", database, false);
    m_sprites[SpriteWoodBow3Random3]->load(path + "wood-bow3-random3.cub", database, false);
    m_sprites[SpriteWoodBow4Random3]->load(path + "wood-bow4-random3.cub", database, false);
    m_sprites[SpriteWoodBow5Random3]->load(path + "wood-bow5-random3.cub", database, false);
    m_sprites[SpriteWoodBow1Random4]->load(path + "wood-bow1-random4.cub", database, false);
    m_sprites[SpriteWoodBow2Random4]->load(path + "wood-bow2-random4.cub", database, false);
    m_sprites[SpriteWoodBow3Random4]->load(path + "wood-bow3-random4.cub", database, false);
    m_sprites[SpriteWoodBow4Random4]->load(path + "wood-bow4-random4.cub", database, false);
    m_sprites[SpriteWoodBow5Random4]->load(path + "wood-bow5-random4.cub", database, false);
    m_sprites[SpriteWoodBow1Random5]->load(path + "wood-bow1-random5.cub", database, false);
    m_sprites[SpriteWoodBow2Random5]->load(path + "wood-bow2-random5.cub", database, false);
    m_sprites[SpriteWoodBow3Random5]->load(path + "wood-bow3-random5.cub", database, false);
    m_sprites[SpriteWoodBow4Random5]->load(path + "wood-bow4-random5.cub", database, false);
    m_sprites[SpriteWoodBow5Random5]->load(path + "wood-bow5-random5.cub", database, false);
    m_sprites[SpriteWoodBow1Random6]->load(path + "wood-bow1-random6.cub", database, false);
    m_sprites[SpriteWoodBow2Random6]->load(path + "wood-bow2-random6.cub", database, false);
    m_sprites[SpriteWoodBow3Random6]->load(path + "wood-bow3-random6.cub", database, false);
    m_sprites[SpriteWoodBow4Random6]->load(path + "wood-bow4-random6.cub", database, false);
    m_sprites[SpriteWoodBow5Random6]->load(path + "wood-bow5-random6.cub", database, false);
    m_sprites[SpriteWoodBow1Random7]->load(path + "wood-bow1-random7.cub", database, false);
    m_sprites[SpriteWoodBow2Random7]->load(path + "wood-bow2-random7.cub", database, false);
    m_sprites[SpriteWoodBow3Random7]->load(path + "wood-bow3-random7.cub", database, false);
    m_sprites[SpriteWoodBow4Random7]->load(path + "wood-bow4-random7.cub", database, false);
    m_sprites[SpriteWoodBow5Random7]->load(path + "wood-bow5-random7.cub", database, false);
    m_sprites[SpriteWoodBow1Random8]->load(path + "wood-bow1-random8.cub", database, false);
    m_sprites[SpriteWoodBow2Random8]->load(path + "wood-bow2-random8.cub", database, false);
    m_sprites[SpriteWoodBow3Random8]->load(path + "wood-bow3-random8.cub", database, false);
    m_sprites[SpriteWoodBow4Random8]->load(path + "wood-bow4-random8.cub", database, false);
    m_sprites[SpriteWoodBow5Random8]->load(path + "wood-bow5-random8.cub", database, false);
    m_sprites[SpriteWoodBow1Random9]->load(path + "wood-bow1-random9.cub", database, false);
    m_sprites[SpriteWoodBow2Random9]->load(path + "wood-bow2-random9.cub", database, false);
    m_sprites[SpriteWoodBow3Random9]->load(path + "wood-bow3-random9.cub", database, false);
    m_sprites[SpriteWoodBow4Random9]->load(path + "wood-bow4-random9.cub", database, false);
    m_sprites[SpriteWoodBow5Random9]->load(path + "wood-bow5-random9.cub", database, false);
    m_sprites[SpriteWoodBow1Random10]->load(path + "wood-bow1-random10.cub", database, false);
    m_sprites[SpriteWoodBow2Random10]->load(path + "wood-bow2-random10.cub", database, false);
    m_sprites[SpriteWoodBow3Random10]->load(path + "wood-bow3-random10.cub", database, false);
    m_sprites[SpriteWoodBow4Random10]->load(path + "wood-bow4-random10.cub", database, false);
    m_sprites[SpriteWoodBow5Random10]->load(path + "wood-bow5-random10.cub", database, false);
    m_sprites[SpriteWoodCrossbow1]->load(path + "wood-crossbow1.cub", database, true);
    m_sprites[SpriteWoodCrossbow2]->load(path + "wood-crossbow2.cub", database, true);
    m_sprites[SpriteWoodCrossbow3]->load(path + "wood-crossbow3.cub", database, true);
    m_sprites[SpriteWoodCrossbow4]->load(path + "wood-crossbow4.cub", database, true);
    m_sprites[SpriteWoodCrossbow5]->load(path + "wood-crossbow5.cub", database, true);
    m_sprites[SpriteWoodCrossbow1Random1]->load(path + "wood-crossbow1-random1.cub", database, false);
    m_sprites[SpriteWoodCrossbow2Random1]->load(path + "wood-crossbow2-random1.cub", database, false);
    m_sprites[SpriteWoodCrossbow3Random1]->load(path + "wood-crossbow3-random1.cub", database, false);
    m_sprites[SpriteWoodCrossbow4Random1]->load(path + "wood-crossbow4-random1.cub", database, false);
    m_sprites[SpriteWoodCrossbow5Random1]->load(path + "wood-crossbow5-random1.cub", database, false);
    m_sprites[SpriteWoodCrossbow1Random2]->load(path + "wood-crossbow1-random2.cub", database, false);
    m_sprites[SpriteWoodCrossbow2Random2]->load(path + "wood-crossbow2-random2.cub", database, false);
    m_sprites[SpriteWoodCrossbow3Random2]->load(path + "wood-crossbow3-random2.cub", database, false);
    m_sprites[SpriteWoodCrossbow4Random2]->load(path + "wood-crossbow4-random2.cub", database, false);
    m_sprites[SpriteWoodCrossbow5Random2]->load(path + "wood-crossbow5-random2.cub", database, false);
    m_sprites[SpriteWoodCrossbow1Random3]->load(path + "wood-crossbow1-random3.cub", database, false);
    m_sprites[SpriteWoodCrossbow2Random3]->load(path + "wood-crossbow2-random3.cub", database, false);
    m_sprites[SpriteWoodCrossbow3Random3]->load(path + "wood-crossbow3-random3.cub", database, false);
    m_sprites[SpriteWoodCrossbow4Random3]->load(path + "wood-crossbow4-random3.cub", database, false);
    m_sprites[SpriteWoodCrossbow5Random3]->load(path + "wood-crossbow5-random3.cub", database, false);
    m_sprites[SpriteWoodCrossbow1Random4]->load(path + "wood-crossbow1-random4.cub", database, false);
    m_sprites[SpriteWoodCrossbow2Random4]->load(path + "wood-crossbow2-random4.cub", database, false);
    m_sprites[SpriteWoodCrossbow3Random4]->load(path + "wood-crossbow3-random4.cub", database, false);
    m_sprites[SpriteWoodCrossbow4Random4]->load(path + "wood-crossbow4-random4.cub", database, false);
    m_sprites[SpriteWoodCrossbow5Random4]->load(path + "wood-crossbow5-random4.cub", database, false);
    m_sprites[SpriteWoodCrossbow1Random5]->load(path + "wood-crossbow1-random5.cub", database, false);
    m_sprites[SpriteWoodCrossbow2Random5]->load(path + "wood-crossbow2-random5.cub", database, false);
    m_sprites[SpriteWoodCrossbow3Random5]->load(path + "wood-crossbow3-random5.cub", database, false);
    m_sprites[SpriteWoodCrossbow4Random5]->load(path + "wood-crossbow4-random5.cub", database, false);
    m_sprites[SpriteWoodCrossbow5Random5]->load(path + "wood-crossbow5-random5.cub", database, false);
    m_sprites[SpriteWoodCrossbow1Random6]->load(path + "wood-crossbow1-random6.cub", database, false);
    m_sprites[SpriteWoodCrossbow2Random6]->load(path + "wood-crossbow2-random6.cub", database, false);
    m_sprites[SpriteWoodCrossbow3Random6]->load(path + "wood-crossbow3-random6.cub", database, false);
    m_sprites[SpriteWoodCrossbow4Random6]->load(path + "wood-crossbow4-random6.cub", database, false);
    m_sprites[SpriteWoodCrossbow5Random6]->load(path + "wood-crossbow5-random6.cub", database, false);
    m_sprites[SpriteWoodCrossbow1Random7]->load(path + "wood-crossbow1-random7.cub", database, false);
    m_sprites[SpriteWoodCrossbow2Random7]->load(path + "wood-crossbow2-random7.cub", database, false);
    m_sprites[SpriteWoodCrossbow3Random7]->load(path + "wood-crossbow3-random7.cub", database, false);
    m_sprites[SpriteWoodCrossbow4Random7]->load(path + "wood-crossbow4-random7.cub", database, false);
    m_sprites[SpriteWoodCrossbow5Random7]->load(path + "wood-crossbow5-random7.cub", database, false);
    m_sprites[SpriteWoodCrossbow1Random8]->load(path + "wood-crossbow1-random8.cub", database, false);
    m_sprites[SpriteWoodCrossbow2Random8]->load(path + "wood-crossbow2-random8.cub", database, false);
    m_sprites[SpriteWoodCrossbow3Random8]->load(path + "wood-crossbow3-random8.cub", database, false);
    m_sprites[SpriteWoodCrossbow4Random8]->load(path + "wood-crossbow4-random8.cub", database, false);
    m_sprites[SpriteWoodCrossbow5Random8]->load(path + "wood-crossbow5-random8.cub", database, false);
    m_sprites[SpriteWoodCrossbow1Random9]->load(path + "wood-crossbow1-random9.cub", database, false);
    m_sprites[SpriteWoodCrossbow2Random9]->load(path + "wood-crossbow2-random9.cub", database, false);
    m_sprites[SpriteWoodCrossbow3Random9]->load(path + "wood-crossbow3-random9.cub", database, false);
    m_sprites[SpriteWoodCrossbow4Random9]->load(path + "wood-crossbow4-random9.cub", database, false);
    m_sprites[SpriteWoodCrossbow5Random9]->load(path + "wood-crossbow5-random9.cub", database, false);
    m_sprites[SpriteWoodCrossbow1Random10]->load(path + "wood-crossbow1-random10.cub", database, false);
    m_sprites[SpriteWoodCrossbow2Random10]->load(path + "wood-crossbow2-random10.cub", database, false);
    m_sprites[SpriteWoodCrossbow3Random10]->load(path + "wood-crossbow3-random10.cub", database, false);
    m_sprites[SpriteWoodCrossbow4Random10]->load(path + "wood-crossbow4-random10.cub", database, false);
    m_sprites[SpriteWoodCrossbow5Random10]->load(path + "wood-crossbow5-random10.cub", database, false);
    m_sprites[SpriteWoodBoomerang1]->load(path + "wood-boomerang1.cub", database, true);
    m_sprites[SpriteWoodBoomerang2]->load(path + "wood-boomerang2.cub", database, true);
    m_sprites[SpriteWoodBoomerang3]->load(path + "wood-boomerang3.cub", database, true);
    m_sprites[SpriteWoodBoomerang4]->load(path + "wood-boomerang4.cub", database, true);
    m_sprites[SpriteWoodBoomerang5]->load(path + "wood-boomerang5.cub", database, true);
    m_sprites[SpriteWoodBoomerang1Random1]->load(path + "wood-boomerang1-random1.cub", database, false);
    m_sprites[SpriteWoodBoomerang2Random1]->load(path + "wood-boomerang2-random1.cub", database, false);
    m_sprites[SpriteWoodBoomerang3Random1]->load(path + "wood-boomerang3-random1.cub", database, false);
    m_sprites[SpriteWoodBoomerang4Random1]->load(path + "wood-boomerang4-random1.cub", database, false);
    m_sprites[SpriteWoodBoomerang5Random1]->load(path + "wood-boomerang5-random1.cub", database, false);
    m_sprites[SpriteWoodBoomerang1Random2]->load(path + "wood-boomerang1-random2.cub", database, false);
    m_sprites[SpriteWoodBoomerang2Random2]->load(path + "wood-boomerang2-random2.cub", database, false);
    m_sprites[SpriteWoodBoomerang3Random2]->load(path + "wood-boomerang3-random2.cub", database, false);
    m_sprites[SpriteWoodBoomerang4Random2]->load(path + "wood-boomerang4-random2.cub", database, false);
    m_sprites[SpriteWoodBoomerang5Random2]->load(path + "wood-boomerang5-random2.cub", database, false);
    m_sprites[SpriteWoodBoomerang1Random3]->load(path + "wood-boomerang1-random3.cub", database, false);
    m_sprites[SpriteWoodBoomerang2Random3]->load(path + "wood-boomerang2-random3.cub", database, false);
    m_sprites[SpriteWoodBoomerang3Random3]->load(path + "wood-boomerang3-random3.cub", database, false);
    m_sprites[SpriteWoodBoomerang4Random3]->load(path + "wood-boomerang4-random3.cub", database, false);
    m_sprites[SpriteWoodBoomerang5Random3]->load(path + "wood-boomerang5-random3.cub", database, false);
    m_sprites[SpriteWoodBoomerang1Random4]->load(path + "wood-boomerang1-random4.cub", database, false);
    m_sprites[SpriteWoodBoomerang2Random4]->load(path + "wood-boomerang2-random4.cub", database, false);
    m_sprites[SpriteWoodBoomerang3Random4]->load(path + "wood-boomerang3-random4.cub", database, false);
    m_sprites[SpriteWoodBoomerang4Random4]->load(path + "wood-boomerang4-random4.cub", database, false);
    m_sprites[SpriteWoodBoomerang5Random4]->load(path + "wood-boomerang5-random4.cub", database, false);
    m_sprites[SpriteWoodBoomerang1Random5]->load(path + "wood-boomerang1-random5.cub", database, false);
    m_sprites[SpriteWoodBoomerang2Random5]->load(path + "wood-boomerang2-random5.cub", database, false);
    m_sprites[SpriteWoodBoomerang3Random5]->load(path + "wood-boomerang3-random5.cub", database, false);
    m_sprites[SpriteWoodBoomerang4Random5]->load(path + "wood-boomerang4-random5.cub", database, false);
    m_sprites[SpriteWoodBoomerang5Random5]->load(path + "wood-boomerang5-random5.cub", database, false);
    m_sprites[SpriteWoodBoomerang1Random6]->load(path + "wood-boomerang1-random6.cub", database, false);
    m_sprites[SpriteWoodBoomerang2Random6]->load(path + "wood-boomerang2-random6.cub", database, false);
    m_sprites[SpriteWoodBoomerang3Random6]->load(path + "wood-boomerang3-random6.cub", database, false);
    m_sprites[SpriteWoodBoomerang4Random6]->load(path + "wood-boomerang4-random6.cub", database, false);
    m_sprites[SpriteWoodBoomerang5Random6]->load(path + "wood-boomerang5-random6.cub", database, false);
    m_sprites[SpriteWoodBoomerang1Random7]->load(path + "wood-boomerang1-random7.cub", database, false);
    m_sprites[SpriteWoodBoomerang2Random7]->load(path + "wood-boomerang2-random7.cub", database, false);
    m_sprites[SpriteWoodBoomerang3Random7]->load(path + "wood-boomerang3-random7.cub", database, false);
    m_sprites[SpriteWoodBoomerang4Random7]->load(path + "wood-boomerang4-random7.cub", database, false);
    m_sprites[SpriteWoodBoomerang5Random7]->load(path + "wood-boomerang5-random7.cub", database, false);
    m_sprites[SpriteWoodBoomerang1Random8]->load(path + "wood-boomerang1-random8.cub", database, false);
    m_sprites[SpriteWoodBoomerang2Random8]->load(path + "wood-boomerang2-random8.cub", database, false);
    m_sprites[SpriteWoodBoomerang3Random8]->load(path + "wood-boomerang3-random8.cub", database, false);
    m_sprites[SpriteWoodBoomerang4Random8]->load(path + "wood-boomerang4-random8.cub", database, false);
    m_sprites[SpriteWoodBoomerang5Random8]->load(path + "wood-boomerang5-random8.cub", database, false);
    m_sprites[SpriteWoodBoomerang1Random9]->load(path + "wood-boomerang1-random9.cub", database, false);
    m_sprites[SpriteWoodBoomerang2Random9]->load(path + "wood-boomerang2-random9.cub", database, false);
    m_sprites[SpriteWoodBoomerang3Random9]->load(path + "wood-boomerang3-random9.cub", database, false);
    m_sprites[SpriteWoodBoomerang4Random9]->load(path + "wood-boomerang4-random9.cub", database, false);
    m_sprites[SpriteWoodBoomerang5Random9]->load(path + "wood-boomerang5-random9.cub", database, false);
    m_sprites[SpriteWoodBoomerang1Random10]->load(path + "wood-boomerang1-random10.cub", database, false);
    m_sprites[SpriteWoodBoomerang2Random10]->load(path + "wood-boomerang2-random10.cub", database, false);
    m_sprites[SpriteWoodBoomerang3Random10]->load(path + "wood-boomerang3-random10.cub", database, false);
    m_sprites[SpriteWoodBoomerang4Random10]->load(path + "wood-boomerang4-random10.cub", database, false);
    m_sprites[SpriteWoodBoomerang5Random10]->load(path + "wood-boomerang5-random10.cub", database, false);
    m_sprites[SpriteArrow]->load(path + "arrow.cub", database, true);
    m_sprites[SpriteFireball]->load(path + "fireball.cub", database, true);
    m_sprites[SpriteQuiver]->load(path + "quiver.cub", database, true);
    m_sprites[SpriteTorch]->load(path + "torch.cub", database, true);
    m_sprites[SpriteCube]->load(path + "cube.cub", database, true);
    m_sprites[SpriteStars]->load(path + "stars.cub", database, true);
    m_sprites[SpritePlaster]->load(path + "plaster.cub", database, true);
    m_sprites[SpriteZzz]->load(path + "zzz.cub", database, true);
    m_sprites[SpriteHeart]->load(path + "heart.cub", database, true);
    m_sprites[SpriteAngry]->load(path + "angry.cub", database, true);
    m_sprites[SpriteDoor]->load(path + "door.cub", database, true);
    m_sprites[SpriteWindow]->load(path + "window.cub", database, true);
    m_sprites[SpriteGoddess2]->load(path + "goddess2.cub", database, true);
    m_sprites[SpriteArtifact]->load(path + "artifact.cub", database, true);
    m_sprites[SpriteQuestItemAmulet01]->load(path + "quest-item-amulet01.cub", database, true);
    m_sprites[SpriteQuestItemAmulet02]->load(path + "quest-item-amulet02.cub", database, true);
    m_sprites[SpriteQuestItemJewelCase]->load(path + "quest-item-jewel-case.cub", database, true);
    m_sprites[SpriteMushroom]->load(path + "mushroom.cub", database, true);
    m_sprites[SpriteShimmerMushroom]->load(path + "shimmer-mushroom.cub", database, true);
    m_sprites[SpriteHeartflower]->load(path + "heartflower.cub", database, true);
    m_sprites[SpriteHeartflowerFrozen]->load(path + "heartflower-frozen.cub", database, true);
    m_sprites[SpritePricklyPear]->load(path + "prickly-pear.cub", database, true);
    m_sprites[SpriteSoulflower]->load(path + "soulflower.cub", database, true);
    m_sprites[SpriteLifePotion]->load(path + "life-potion.cub", database, true);
    m_sprites[SpriteCactusPotion]->load(path + "cactus-potion.cub", database, true);
    m_sprites[SpriteManaPotion]->load(path + "mana-potion.cub", database, true);
    m_sprites[SpriteCookie]->load(path + "cookie.cub", database, true);
    m_sprites[SpriteJellyGreen]->load(path + "jelly-green.cub", database, true);
    m_sprites[SpriteJellyBlue]->load(path + "jelly-blue.cub", database, true);
    m_sprites[SpriteJellyPink]->load(path + "jelly-pink.cub", database, true);
    m_sprites[SpriteJellyYellow]->load(path + "jelly-yellow.cub", database, true);
    m_sprites[SpriteCarrot]->load(path + "carrot.cub", database, true);
    m_sprites[SpritePumpkinMash]->load(path + "pumpkin-mash.cub", database, true);
    m_sprites[SpriteCandy]->load(path + "candy.cub", database, true);
    m_sprites[SpriteLollipop]->load(path + "lollipop.cub", database, true);
    m_sprites[SpriteSoftice]->load(path + "softice.cub", database, true);
    m_sprites[SpriteDonutChocolate]->load(path + "donut-chocolate.cub", database, true);
    m_sprites[SpriteCottonCandy]->load(path + "cotton-candy.cub", database, true);
    m_sprites[SpritePopcorn]->load(path + "popcorn.cub", database, true);
    m_sprites[SpriteCerealBar]->load(path + "cereal-bar.cub", database, true);
    m_sprites[SpriteStrawberryCake]->load(path + "strawberry-cake.cub", database, true);
    m_sprites[SpriteChocolateCake]->load(path + "chocolate-cake.cub", database, true);
    m_sprites[SpriteVanillaCupcake]->load(path + "vanilla-cupcake.cub", database, true);
    m_sprites[SpriteChocolateCupcake]->load(path + "chocolate-cupcake.cub", database, true);
    m_sprites[SpriteBananaSplit]->load(path + "banana-split.cub", database, true);
    m_sprites[SpriteCroissant]->load(path + "croissant.cub", database, true);
    m_sprites[SpriteLolly]->load(path + "lolly.cub", database, true);
    m_sprites[SpriteLemonTart]->load(path + "lemon-tart.cub", database, true);
    m_sprites[SpriteChocolateCookie]->load(path + "chocolate-cookie.cub", database, true);
    m_sprites[SpriteBubbleGum]->load(path + "bubble-gum.cub", database, true);
    m_sprites[SpriteLicoriceCandy]->load(path + "licorice-candy.cub", database, true);
    m_sprites[SpriteCinnamonRole]->load(path + "cinnamon-role.cub", database, true);
    m_sprites[SpriteAppleRing]->load(path + "apple-ring.cub", database, true);
    m_sprites[SpriteWaffle]->load(path + "waffle.cub", database, true);
    m_sprites[SpriteWaterIce]->load(path + "water-ice.cub", database, true);
    m_sprites[SpriteDateCookie]->load(path + "date-cookie.cub", database, true);
    m_sprites[SpriteCandiedApple]->load(path + "candied-apple.cub", database, true);
    m_sprites[SpriteStrawberryCocktail]->load(path + "strawberry-cocktail.cub", database, true);
    m_sprites[SpriteMilkChocolateBar]->load(path + "milk-chocolate-bar.cub", database, true);
    m_sprites[SpriteCaramelChocolateBar]->load(path + "caramel-chocolate-bar.cub", database, true);
    m_sprites[SpriteMintChocolateBar]->load(path + "mint-chocolate-bar.cub", database, true);
    m_sprites[SpriteWhiteChocolateBar]->load(path + "white-chocolate-bar.cub", database, true);
    m_sprites[SpriteSugarCandy]->load(path + "sugar-candy.cub", database, true);
    m_sprites[SpriteBlackberryMarmelade]->load(path + "blackberry-marmelade.cub", database, true);
    m_sprites[SpriteSaltedCaramel]->load(path + "salted-caramel.cub", database, true);
    m_sprites[SpriteGingerTartlet]->load(path + "ginger-tartlet.cub", database, true);
    m_sprites[SpriteMangoJuice]->load(path + "mango-juice.cub", database, true);
    m_sprites[SpriteFruitBasket]->load(path + "fruit-basket.cub", database, true);
    m_sprites[SpriteMelonIcecream]->load(path + "melon-icecream.cub", database, true);
    m_sprites[SpriteBloodorangeJuice]->load(path + "bloodorange-juice.cub", database, true);
    m_sprites[SpritePancakes]->load(path + "pancakes.cub", database, true);
    m_sprites[SpriteCurry]->load(path + "curry.cub", database, true);
    m_sprites[SpriteBiscuitRole]->load(path + "biscuit-role.cub", database, true);
    m_sprites[SpriteIronSword1]->load(path + "iron-sword1.cub", database, true);
    m_sprites[SpriteIronSword2]->load(path + "iron-sword2.cub", database, true);
    m_sprites[SpriteIronSword3]->load(path + "iron-sword3.cub", database, true);
    m_sprites[SpriteIronSword4]->load(path + "iron-sword4.cub", database, true);
    m_sprites[SpriteIronSword5]->load(path + "iron-sword5.cub", database, true);
    m_sprites[SpriteIronSword1Random1]->load(path + "iron-sword1-random1.cub", database, true);
    m_sprites[SpriteIronSword2Random1]->load(path + "iron-sword2-random1.cub", database, true);
    m_sprites[SpriteIronSword3Random1]->load(path + "iron-sword3-random1.cub", database, true);
    m_sprites[SpriteIronSword4Random1]->load(path + "iron-sword4-random1.cub", database, true);
    m_sprites[SpriteIronSword5Random1]->load(path + "iron-sword5-random1.cub", database, true);
    m_sprites[SpriteIronSword1Random2]->load(path + "iron-sword1-random2.cub", database, true);
    m_sprites[SpriteIronSword2Random2]->load(path + "iron-sword2-random2.cub", database, true);
    m_sprites[SpriteIronSword3Random2]->load(path + "iron-sword3-random2.cub", database, true);
    m_sprites[SpriteIronSword4Random2]->load(path + "iron-sword4-random2.cub", database, true);
    m_sprites[SpriteIronSword5Random2]->load(path + "iron-sword5-random2.cub", database, true);
    m_sprites[SpriteIronSword1Random3]->load(path + "iron-sword1-random3.cub", database, true);
    m_sprites[SpriteIronSword2Random3]->load(path + "iron-sword2-random3.cub", database, true);
    m_sprites[SpriteIronSword3Random3]->load(path + "iron-sword3-random3.cub", database, true);
    m_sprites[SpriteIronSword4Random3]->load(path + "iron-sword4-random3.cub", database, true);
    m_sprites[SpriteIronSword5Random3]->load(path + "iron-sword5-random3.cub", database, true);
    m_sprites[SpriteIronSword1Random4]->load(path + "iron-sword1-random4.cub", database, true);
    m_sprites[SpriteIronSword2Random4]->load(path + "iron-sword2-random4.cub", database, true);
    m_sprites[SpriteIronSword3Random4]->load(path + "iron-sword3-random4.cub", database, true);
    m_sprites[SpriteIronSword4Random4]->load(path + "iron-sword4-random4.cub", database, true);
    m_sprites[SpriteIronSword5Random4]->load(path + "iron-sword5-random4.cub", database, true);
    m_sprites[SpriteIronSword1Random5]->load(path + "iron-sword1-random5.cub", database, true);
    m_sprites[SpriteIronSword2Random5]->load(path + "iron-sword2-random5.cub", database, true);
    m_sprites[SpriteIronSword3Random5]->load(path + "iron-sword3-random5.cub", database, true);
    m_sprites[SpriteIronSword4Random5]->load(path + "iron-sword4-random5.cub", database, true);
    m_sprites[SpriteIronSword5Random5]->load(path + "iron-sword5-random5.cub", database, true);
    m_sprites[SpriteIronSword1Random6]->load(path + "iron-sword1-random6.cub", database, true);
    m_sprites[SpriteIronSword2Random6]->load(path + "iron-sword2-random6.cub", database, true);
    m_sprites[SpriteIronSword3Random6]->load(path + "iron-sword3-random6.cub", database, true);
    m_sprites[SpriteIronSword4Random6]->load(path + "iron-sword4-random6.cub", database, true);
    m_sprites[SpriteIronSword5Random6]->load(path + "iron-sword5-random6.cub", database, true);
    m_sprites[SpriteIronSword1Random7]->load(path + "iron-sword1-random7.cub", database, true);
    m_sprites[SpriteIronSword2Random7]->load(path + "iron-sword2-random7.cub", database, true);
    m_sprites[SpriteIronSword3Random7]->load(path + "iron-sword3-random7.cub", database, true);
    m_sprites[SpriteIronSword4Random7]->load(path + "iron-sword4-random7.cub", database, true);
    m_sprites[SpriteIronSword5Random7]->load(path + "iron-sword5-random7.cub", database, true);
    m_sprites[SpriteIronSword1Random8]->load(path + "iron-sword1-random8.cub", database, true);
    m_sprites[SpriteIronSword2Random8]->load(path + "iron-sword2-random8.cub", database, true);
    m_sprites[SpriteIronSword3Random8]->load(path + "iron-sword3-random8.cub", database, true);
    m_sprites[SpriteIronSword4Random8]->load(path + "iron-sword4-random8.cub", database, true);
    m_sprites[SpriteIronSword5Random8]->load(path + "iron-sword5-random8.cub", database, true);
    m_sprites[SpriteIronSword1Random9]->load(path + "iron-sword1-random9.cub", database, true);
    m_sprites[SpriteIronSword2Random9]->load(path + "iron-sword2-random9.cub", database, true);
    m_sprites[SpriteIronSword3Random9]->load(path + "iron-sword3-random9.cub", database, true);
    m_sprites[SpriteIronSword4Random9]->load(path + "iron-sword4-random9.cub", database, true);
    m_sprites[SpriteIronSword5Random9]->load(path + "iron-sword5-random9.cub", database, true);
    m_sprites[SpriteIronSword1Random10]->load(path + "iron-sword1-random10.cub", database, true);
    m_sprites[SpriteIronSword2Random10]->load(path + "iron-sword2-random10.cub", database, true);
    m_sprites[SpriteIronSword3Random10]->load(path + "iron-sword3-random10.cub", database, true);
    m_sprites[SpriteIronSword4Random10]->load(path + "iron-sword4-random10.cub", database, true);
    m_sprites[SpriteIronSword5Random10]->load(path + "iron-sword5-random10.cub", database, true);
    m_sprites[SpriteIronDagger1]->load(path + "iron-dagger1.cub", database, true);
    m_sprites[SpriteIronDagger2]->load(path + "iron-dagger2.cub", database, true);
    m_sprites[SpriteIronDagger3]->load(path + "iron-dagger3.cub", database, true);
    m_sprites[SpriteIronDagger4]->load(path + "iron-dagger4.cub", database, true);
    m_sprites[SpriteIronDagger5]->load(path + "iron-dagger5.cub", database, true);
    m_sprites[SpriteIronDagger1Random1]->load(path + "iron-dagger1-random1.cub", database, true);
    m_sprites[SpriteIronDagger2Random1]->load(path + "iron-dagger2-random1.cub", database, true);
    m_sprites[SpriteIronDagger3Random1]->load(path + "iron-dagger3-random1.cub", database, true);
    m_sprites[SpriteIronDagger4Random1]->load(path + "iron-dagger4-random1.cub", database, true);
    m_sprites[SpriteIronDagger5Random1]->load(path + "iron-dagger5-random1.cub", database, true);
    m_sprites[SpriteIronDagger1Random2]->load(path + "iron-dagger1-random2.cub", database, true);
    m_sprites[SpriteIronDagger2Random2]->load(path + "iron-dagger2-random2.cub", database, true);
    m_sprites[SpriteIronDagger3Random2]->load(path + "iron-dagger3-random2.cub", database, true);
    m_sprites[SpriteIronDagger4Random2]->load(path + "iron-dagger4-random2.cub", database, true);
    m_sprites[SpriteIronDagger5Random2]->load(path + "iron-dagger5-random2.cub", database, true);
    m_sprites[SpriteIronDagger1Random3]->load(path + "iron-dagger1-random3.cub", database, true);
    m_sprites[SpriteIronDagger2Random3]->load(path + "iron-dagger2-random3.cub", database, true);
    m_sprites[SpriteIronDagger3Random3]->load(path + "iron-dagger3-random3.cub", database, true);
    m_sprites[SpriteIronDagger4Random3]->load(path + "iron-dagger4-random3.cub", database, true);
    m_sprites[SpriteIronDagger5Random3]->load(path + "iron-dagger5-random3.cub", database, true);
    m_sprites[SpriteIronDagger1Random4]->load(path + "iron-dagger1-random4.cub", database, true);
    m_sprites[SpriteIronDagger2Random4]->load(path + "iron-dagger2-random4.cub", database, true);
    m_sprites[SpriteIronDagger3Random4]->load(path + "iron-dagger3-random4.cub", database, true);
    m_sprites[SpriteIronDagger4Random4]->load(path + "iron-dagger4-random4.cub", database, true);
    m_sprites[SpriteIronDagger5Random4]->load(path + "iron-dagger5-random4.cub", database, true);
    m_sprites[SpriteIronDagger1Random5]->load(path + "iron-dagger1-random5.cub", database, true);
    m_sprites[SpriteIronDagger2Random5]->load(path + "iron-dagger2-random5.cub", database, true);
    m_sprites[SpriteIronDagger3Random5]->load(path + "iron-dagger3-random5.cub", database, true);
    m_sprites[SpriteIronDagger4Random5]->load(path + "iron-dagger4-random5.cub", database, true);
    m_sprites[SpriteIronDagger5Random5]->load(path + "iron-dagger5-random5.cub", database, true);
    m_sprites[SpriteIronDagger1Random6]->load(path + "iron-dagger1-random6.cub", database, true);
    m_sprites[SpriteIronDagger2Random6]->load(path + "iron-dagger2-random6.cub", database, true);
    m_sprites[SpriteIronDagger3Random6]->load(path + "iron-dagger3-random6.cub", database, true);
    m_sprites[SpriteIronDagger4Random6]->load(path + "iron-dagger4-random6.cub", database, true);
    m_sprites[SpriteIronDagger5Random6]->load(path + "iron-dagger5-random6.cub", database, true);
    m_sprites[SpriteIronDagger1Random7]->load(path + "iron-dagger1-random7.cub", database, true);
    m_sprites[SpriteIronDagger2Random7]->load(path + "iron-dagger2-random7.cub", database, true);
    m_sprites[SpriteIronDagger3Random7]->load(path + "iron-dagger3-random7.cub", database, true);
    m_sprites[SpriteIronDagger4Random7]->load(path + "iron-dagger4-random7.cub", database, true);
    m_sprites[SpriteIronDagger5Random7]->load(path + "iron-dagger5-random7.cub", database, true);
    m_sprites[SpriteIronDagger1Random8]->load(path + "iron-dagger1-random8.cub", database, true);
    m_sprites[SpriteIronDagger2Random8]->load(path + "iron-dagger2-random8.cub", database, true);
    m_sprites[SpriteIronDagger3Random8]->load(path + "iron-dagger3-random8.cub", database, true);
    m_sprites[SpriteIronDagger4Random8]->load(path + "iron-dagger4-random8.cub", database, true);
    m_sprites[SpriteIronDagger5Random8]->load(path + "iron-dagger5-random8.cub", database, true);
    m_sprites[SpriteIronDagger1Random9]->load(path + "iron-dagger1-random9.cub", database, true);
    m_sprites[SpriteIronDagger2Random9]->load(path + "iron-dagger2-random9.cub", database, true);
    m_sprites[SpriteIronDagger3Random9]->load(path + "iron-dagger3-random9.cub", database, true);
    m_sprites[SpriteIronDagger4Random9]->load(path + "iron-dagger4-random9.cub", database, true);
    m_sprites[SpriteIronDagger5Random9]->load(path + "iron-dagger5-random9.cub", database, true);
    m_sprites[SpriteIronDagger1Random10]->load(path + "iron-dagger1-random10.cub", database, true);
    m_sprites[SpriteIronDagger2Random10]->load(path + "iron-dagger2-random10.cub", database, true);
    m_sprites[SpriteIronDagger3Random10]->load(path + "iron-dagger3-random10.cub", database, true);
    m_sprites[SpriteIronDagger4Random10]->load(path + "iron-dagger4-random10.cub", database, true);
    m_sprites[SpriteIronDagger5Random10]->load(path + "iron-dagger5-random10.cub", database, true);
    m_sprites[SpriteIronFist1]->load(path + "iron-fist1.cub", database, true);
    m_sprites[SpriteIronFist2]->load(path + "iron-fist2.cub", database, true);
    m_sprites[SpriteIronFist3]->load(path + "iron-fist3.cub", database, true);
    m_sprites[SpriteIronFist4]->load(path + "iron-fist4.cub", database, true);
    m_sprites[SpriteIronFist5]->load(path + "iron-fist5.cub", database, true);
    m_sprites[SpriteIronFist1Random1]->load(path + "iron-fist1-random1.cub", database, true);
    m_sprites[SpriteIronFist2Random1]->load(path + "iron-fist2-random1.cub", database, true);
    m_sprites[SpriteIronFist3Random1]->load(path + "iron-fist3-random1.cub", database, true);
    m_sprites[SpriteIronFist4Random1]->load(path + "iron-fist4-random1.cub", database, true);
    m_sprites[SpriteIronFist5Random1]->load(path + "iron-fist5-random1.cub", database, true);
    m_sprites[SpriteIronFist1Random2]->load(path + "iron-fist1-random2.cub", database, true);
    m_sprites[SpriteIronFist2Random2]->load(path + "iron-fist2-random2.cub", database, true);
    m_sprites[SpriteIronFist3Random2]->load(path + "iron-fist3-random2.cub", database, true);
    m_sprites[SpriteIronFist4Random2]->load(path + "iron-fist4-random2.cub", database, true);
    m_sprites[SpriteIronFist5Random2]->load(path + "iron-fist5-random2.cub", database, true);
    m_sprites[SpriteIronFist1Random3]->load(path + "iron-fist1-random3.cub", database, true);
    m_sprites[SpriteIronFist2Random3]->load(path + "iron-fist2-random3.cub", database, true);
    m_sprites[SpriteIronFist3Random3]->load(path + "iron-fist3-random3.cub", database, true);
    m_sprites[SpriteIronFist4Random3]->load(path + "iron-fist4-random3.cub", database, true);
    m_sprites[SpriteIronFist5Random3]->load(path + "iron-fist5-random3.cub", database, true);
    m_sprites[SpriteIronFist1Random4]->load(path + "iron-fist1-random4.cub", database, true);
    m_sprites[SpriteIronFist2Random4]->load(path + "iron-fist2-random4.cub", database, true);
    m_sprites[SpriteIronFist3Random4]->load(path + "iron-fist3-random4.cub", database, true);
    m_sprites[SpriteIronFist4Random4]->load(path + "iron-fist4-random4.cub", database, true);
    m_sprites[SpriteIronFist5Random4]->load(path + "iron-fist5-random4.cub", database, true);
    m_sprites[SpriteIronFist1Random5]->load(path + "iron-fist1-random5.cub", database, true);
    m_sprites[SpriteIronFist2Random5]->load(path + "iron-fist2-random5.cub", database, true);
    m_sprites[SpriteIronFist3Random5]->load(path + "iron-fist3-random5.cub", database, true);
    m_sprites[SpriteIronFist4Random5]->load(path + "iron-fist4-random5.cub", database, true);
    m_sprites[SpriteIronFist5Random5]->load(path + "iron-fist5-random5.cub", database, true);
    m_sprites[SpriteIronFist1Random6]->load(path + "iron-fist1-random6.cub", database, true);
    m_sprites[SpriteIronFist2Random6]->load(path + "iron-fist2-random6.cub", database, true);
    m_sprites[SpriteIronFist3Random6]->load(path + "iron-fist3-random6.cub", database, true);
    m_sprites[SpriteIronFist4Random6]->load(path + "iron-fist4-random6.cub", database, true);
    m_sprites[SpriteIronFist5Random6]->load(path + "iron-fist5-random6.cub", database, true);
    m_sprites[SpriteIronFist1Random7]->load(path + "iron-fist1-random7.cub", database, true);
    m_sprites[SpriteIronFist2Random7]->load(path + "iron-fist2-random7.cub", database, true);
    m_sprites[SpriteIronFist3Random7]->load(path + "iron-fist3-random7.cub", database, true);
    m_sprites[SpriteIronFist4Random7]->load(path + "iron-fist4-random7.cub", database, true);
    m_sprites[SpriteIronFist5Random7]->load(path + "iron-fist5-random7.cub", database, true);
    m_sprites[SpriteIronFist1Random8]->load(path + "iron-fist1-random8.cub", database, true);
    m_sprites[SpriteIronFist2Random8]->load(path + "iron-fist2-random8.cub", database, true);
    m_sprites[SpriteIronFist3Random8]->load(path + "iron-fist3-random8.cub", database, true);
    m_sprites[SpriteIronFist4Random8]->load(path + "iron-fist4-random8.cub", database, true);
    m_sprites[SpriteIronFist5Random8]->load(path + "iron-fist5-random8.cub", database, true);
    m_sprites[SpriteIronFist1Random9]->load(path + "iron-fist1-random9.cub", database, true);
    m_sprites[SpriteIronFist2Random9]->load(path + "iron-fist2-random9.cub", database, true);
    m_sprites[SpriteIronFist3Random9]->load(path + "iron-fist3-random9.cub", database, true);
    m_sprites[SpriteIronFist4Random9]->load(path + "iron-fist4-random9.cub", database, true);
    m_sprites[SpriteIronFist5Random9]->load(path + "iron-fist5-random9.cub", database, true);
    m_sprites[SpriteIronFist1Random10]->load(path + "iron-fist1-random10.cub", database, true);
    m_sprites[SpriteIronFist2Random10]->load(path + "iron-fist2-random10.cub", database, true);
    m_sprites[SpriteIronFist3Random10]->load(path + "iron-fist3-random10.cub", database, true);
    m_sprites[SpriteIronFist4Random10]->load(path + "iron-fist4-random10.cub", database, true);
    m_sprites[SpriteIronFist5Random10]->load(path + "iron-fist5-random10.cub", database, true);
    m_sprites[SpriteIronHelmet]->load(path + "iron-helmet.cub", database, true);
    m_sprites[SpriteInnkeeperChest]->load(path + "innkeeper-chest.cub", database, true);
    m_sprites[SpriteBackpack]->load(path + "backpack.cub", database, true);
    m_sprites[SpriteBody1]->load(path + "body1.cub", database, true);
    m_sprites[SpriteBody2]->load(path + "body2.cub", database, true);
    m_sprites[SpriteBody3]->load(path + "body3.cub", database, true);
    m_sprites[SpriteBody4]->load(path + "body4.cub", database, true);
    m_sprites[SpriteBody5]->load(path + "body5.cub", database, true);
    m_sprites[SpriteBackpack]->load(path + "backpack.cub", database, true);
    m_sprites[SpriteObsidianChest]->load(path + "obsidian-chest.cub", database, true);
    m_sprites[SpriteObsidianShoulders]->load(path + "obsidian-shoulders.cub", database, true);
    m_sprites[SpriteObsidianBoot]->load(path + "obsidian-boot.cub", database, true);
    m_sprites[SpriteObsidianGlove]->load(path + "obsidian-glove.cub", database, true);
    m_sprites[SpriteObsidianHelmet]->load(path + "obsidian-helmet.cub", database, true);
    m_sprites[SpriteGoldChest]->load(path + "gold-chest.cub", database, true);
    m_sprites[SpriteGoldShoulder]->load(path + "gold-shoulder.cub", database, true);
    m_sprites[SpriteGoldBoot]->load(path + "gold-boot.cub", database, true);
    m_sprites[SpriteGoldGlove]->load(path + "gold-glove.cub", database, true);
    m_sprites[SpriteGoldHelmet]->load(path + "gold-helmet.cub", database, true);
    m_sprites[SpriteIceCrystalChest]->load(path + "ice-crystal-chest.cub", database, true);
    m_sprites[SpriteIceCrystalShoulder]->load(path + "ice-crystal-shoulder.cub", database, true);
    m_sprites[SpriteIceCrystalBoot]->load(path + "ice-crystal-boot.cub", database, true);
    m_sprites[SpriteIceCrystalGlove]->load(path + "ice-crystal-glove.cub", database, true);
    m_sprites[SpriteIceCrystalHelmet]->load(path + "ice-crystal-helmet.cub", database, true);
    m_sprites[SpriteSaurianChest]->load(path + "saurian-chest.cub", database, true);
    m_sprites[SpriteSaurianShoulder]->load(path + "saurian-shoulder.cub", database, true);
    m_sprites[SpriteSaurianBoot]->load(path + "saurian-boot.cub", database, true);
    m_sprites[SpriteSaurianGlove]->load(path + "saurian-glove.cub", database, true);
    m_sprites[SpriteSaurianHelmet]->load(path + "saurian-helmet.cub", database, true);
    m_sprites[SpriteSaurianChest]->load(path + "saurian-chest.cub", database, true);
    m_sprites[SpriteSaurianShoulder]->load(path + "saurian-shoulder.cub", database, true);
    m_sprites[SpriteSaurianBoot]->load(path + "saurian-boot.cub", database, true);
    m_sprites[SpriteSaurianGlove]->load(path + "saurian-glove.cub", database, true);
    m_sprites[SpriteSaurianHelmet]->load(path + "saurian-helmet.cub", database, true);
    m_sprites[SpriteParrotChest]->load(path + "parrot-chest.cub", database, true);
    m_sprites[SpriteParrotShoulder]->load(path + "parrot-shoulder.cub", database, true);
    m_sprites[SpriteParrotBoot]->load(path + "parrot-boot.cub", database, true);
    m_sprites[SpriteParrotGlove]->load(path + "parrot-glove.cub", database, true);
    m_sprites[SpriteParrotHelmet]->load(path + "parrot-helmet.cub", database, true);
    m_sprites[SpriteLightChest]->load(path + "light-chest.cub", database, true);
    m_sprites[SpriteLightShoulder]->load(path + "light-shoulder.cub", database, true);
    m_sprites[SpriteLightBoot]->load(path + "light-boot.cub", database, true);
    m_sprites[SpriteLightGlove]->load(path + "light-glove.cub", database, true);
    m_sprites[SpriteLightHelmet]->load(path + "light-helmet.cub", database, true);
    m_sprites[SpriteSilkChest1]->load(path + "silk-chest1.cub", database, true);
    m_sprites[SpriteSilkChest2]->load(path + "silk-chest2.cub", database, true);
    m_sprites[SpriteSilkChest3]->load(path + "silk-chest3.cub", database, true);
    m_sprites[SpriteSilkChest4]->load(path + "silk-chest4.cub", database, true);
    m_sprites[SpriteSilkChest5]->load(path + "silk-chest5.cub", database, true);
    m_sprites[SpriteSilkChest1Random1]->load(path + "silk-chest1-random1.cub", database, true);
    m_sprites[SpriteSilkChest2Random1]->load(path + "silk-chest2-random1.cub", database, true);
    m_sprites[SpriteSilkChest3Random1]->load(path + "silk-chest3-random1.cub", database, true);
    m_sprites[SpriteSilkChest4Random1]->load(path + "silk-chest4-random1.cub", database, true);
    m_sprites[SpriteSilkChest5Random1]->load(path + "silk-chest5-random1.cub", database, true);
    m_sprites[SpriteSilkChest1Random2]->load(path + "silk-chest1-random2.cub", database, true);
    m_sprites[SpriteSilkChest2Random2]->load(path + "silk-chest2-random2.cub", database, true);
    m_sprites[SpriteSilkChest3Random2]->load(path + "silk-chest3-random2.cub", database, true);
    m_sprites[SpriteSilkChest4Random2]->load(path + "silk-chest4-random2.cub", database, true);
    m_sprites[SpriteSilkChest5Random2]->load(path + "silk-chest5-random2.cub", database, true);
    m_sprites[SpriteSilkChest1Random3]->load(path + "silk-chest1-random3.cub", database, true);
    m_sprites[SpriteSilkChest2Random3]->load(path + "silk-chest2-random3.cub", database, true);
    m_sprites[SpriteSilkChest3Random3]->load(path + "silk-chest3-random3.cub", database, true);
    m_sprites[SpriteSilkChest4Random3]->load(path + "silk-chest4-random3.cub", database, true);
    m_sprites[SpriteSilkChest5Random3]->load(path + "silk-chest5-random3.cub", database, true);
    m_sprites[SpriteSilkChest1Random4]->load(path + "silk-chest1-random4.cub", database, true);
    m_sprites[SpriteSilkChest2Random4]->load(path + "silk-chest2-random4.cub", database, true);
    m_sprites[SpriteSilkChest3Random4]->load(path + "silk-chest3-random4.cub", database, true);
    m_sprites[SpriteSilkChest4Random4]->load(path + "silk-chest4-random4.cub", database, true);
    m_sprites[SpriteSilkChest5Random4]->load(path + "silk-chest5-random4.cub", database, true);
    m_sprites[SpriteSilkShoulder1]->load(path + "silk-shoulder1.cub", database, true);
    m_sprites[SpriteSilkShoulder2]->load(path + "silk-shoulder2.cub", database, true);
    m_sprites[SpriteSilkShoulder3]->load(path + "silk-shoulder3.cub", database, true);
    m_sprites[SpriteSilkShoulder4]->load(path + "silk-shoulder4.cub", database, true);
    m_sprites[SpriteSilkShoulder5]->load(path + "silk-shoulder5.cub", database, true);
    m_sprites[SpriteSilkShoulder1Random1]->load(path + "silk-shoulder1-random1.cub", database, true);
    m_sprites[SpriteSilkShoulder2Random1]->load(path + "silk-shoulder2-random1.cub", database, true);
    m_sprites[SpriteSilkShoulder3Random1]->load(path + "silk-shoulder3-random1.cub", database, true);
    m_sprites[SpriteSilkShoulder4Random1]->load(path + "silk-shoulder4-random1.cub", database, true);
    m_sprites[SpriteSilkShoulder5Random1]->load(path + "silk-shoulder5-random1.cub", database, true);
    m_sprites[SpriteSilkShoulder1Random2]->load(path + "silk-shoulder1-random2.cub", database, true);
    m_sprites[SpriteSilkShoulder2Random2]->load(path + "silk-shoulder2-random2.cub", database, true);
    m_sprites[SpriteSilkShoulder3Random2]->load(path + "silk-shoulder3-random2.cub", database, true);
    m_sprites[SpriteSilkShoulder4Random2]->load(path + "silk-shoulder4-random2.cub", database, true);
    m_sprites[SpriteSilkShoulder5Random2]->load(path + "silk-shoulder5-random2.cub", database, true);
    m_sprites[SpriteSilkShoulder1Random3]->load(path + "silk-shoulder1-random3.cub", database, true);
    m_sprites[SpriteSilkShoulder2Random3]->load(path + "silk-shoulder2-random3.cub", database, true);
    m_sprites[SpriteSilkShoulder3Random3]->load(path + "silk-shoulder3-random3.cub", database, true);
    m_sprites[SpriteSilkShoulder4Random3]->load(path + "silk-shoulder4-random3.cub", database, true);
    m_sprites[SpriteSilkShoulder5Random3]->load(path + "silk-shoulder5-random3.cub", database, true);
    m_sprites[SpriteSilkShoulder1Random4]->load(path + "silk-shoulder1-random4.cub", database, true);
    m_sprites[SpriteSilkShoulder2Random4]->load(path + "silk-shoulder2-random4.cub", database, true);
    m_sprites[SpriteSilkShoulder3Random4]->load(path + "silk-shoulder3-random4.cub", database, true);
    m_sprites[SpriteSilkShoulder4Random4]->load(path + "silk-shoulder4-random4.cub", database, true);
    m_sprites[SpriteSilkShoulder5Random4]->load(path + "silk-shoulder5-random4.cub", database, true);
    m_sprites[SpriteSilkHand1]->load(path + "silk-hand1.cub", database, true);
    m_sprites[SpriteSilkHand2]->load(path + "silk-hand2.cub", database, true);
    m_sprites[SpriteSilkHand3]->load(path + "silk-hand3.cub", database, true);
    m_sprites[SpriteSilkHand4]->load(path + "silk-hand4.cub", database, true);
    m_sprites[SpriteSilkHand5]->load(path + "silk-hand5.cub", database, true);
    m_sprites[SpriteSilkHand1Random1]->load(path + "silk-hand1-random1.cub", database, true);
    m_sprites[SpriteSilkHand2Random1]->load(path + "silk-hand2-random1.cub", database, true);
    m_sprites[SpriteSilkHand3Random1]->load(path + "silk-hand3-random1.cub", database, true);
    m_sprites[SpriteSilkHand4Random1]->load(path + "silk-hand4-random1.cub", database, true);
    m_sprites[SpriteSilkHand5Random1]->load(path + "silk-hand5-random1.cub", database, true);
    m_sprites[SpriteSilkHand1Random2]->load(path + "silk-hand1-random2.cub", database, true);
    m_sprites[SpriteSilkHand2Random2]->load(path + "silk-hand2-random2.cub", database, true);
    m_sprites[SpriteSilkHand3Random2]->load(path + "silk-hand3-random2.cub", database, true);
    m_sprites[SpriteSilkHand4Random2]->load(path + "silk-hand4-random2.cub", database, true);
    m_sprites[SpriteSilkHand5Random2]->load(path + "silk-hand5-random2.cub", database, true);
    m_sprites[SpriteSilkHand1Random3]->load(path + "silk-hand1-random3.cub", database, true);
    m_sprites[SpriteSilkHand2Random3]->load(path + "silk-hand2-random3.cub", database, true);
    m_sprites[SpriteSilkHand3Random3]->load(path + "silk-hand3-random3.cub", database, true);
    m_sprites[SpriteSilkHand4Random3]->load(path + "silk-hand4-random3.cub", database, true);
    m_sprites[SpriteSilkHand5Random3]->load(path + "silk-hand5-random3.cub", database, true);
    m_sprites[SpriteSilkHand1Random4]->load(path + "silk-hand1-random4.cub", database, true);
    m_sprites[SpriteSilkHand2Random4]->load(path + "silk-hand2-random4.cub", database, true);
    m_sprites[SpriteSilkHand3Random4]->load(path + "silk-hand3-random4.cub", database, true);
    m_sprites[SpriteSilkHand4Random4]->load(path + "silk-hand4-random4.cub", database, true);
    m_sprites[SpriteSilkHand5Random4]->load(path + "silk-hand5-random4.cub", database, true);
    m_sprites[SpriteSilkFoot1]->load(path + "silk-foot1.cub", database, true);
    m_sprites[SpriteSilkFoot2]->load(path + "silk-foot2.cub", database, true);
    m_sprites[SpriteSilkFoot3]->load(path + "silk-foot3.cub", database, true);
    m_sprites[SpriteSilkFoot4]->load(path + "silk-foot4.cub", database, true);
    m_sprites[SpriteSilkFoot5]->load(path + "silk-foot5.cub", database, true);
    m_sprites[SpriteSilkFoot1Random1]->load(path + "silk-foot1-random1.cub", database, true);
    m_sprites[SpriteSilkFoot2Random1]->load(path + "silk-foot2-random1.cub", database, true);
    m_sprites[SpriteSilkFoot3Random1]->load(path + "silk-foot3-random1.cub", database, true);
    m_sprites[SpriteSilkFoot4Random1]->load(path + "silk-foot4-random1.cub", database, true);
    m_sprites[SpriteSilkFoot5Random1]->load(path + "silk-foot5-random1.cub", database, true);
    m_sprites[SpriteSilkFoot1Random2]->load(path + "silk-foot1-random2.cub", database, true);
    m_sprites[SpriteSilkFoot2Random2]->load(path + "silk-foot2-random2.cub", database, true);
    m_sprites[SpriteSilkFoot3Random2]->load(path + "silk-foot3-random2.cub", database, true);
    m_sprites[SpriteSilkFoot4Random2]->load(path + "silk-foot4-random2.cub", database, true);
    m_sprites[SpriteSilkFoot5Random2]->load(path + "silk-foot5-random2.cub", database, true);
    m_sprites[SpriteSilkFoot1Random3]->load(path + "silk-foot1-random3.cub", database, true);
    m_sprites[SpriteSilkFoot2Random3]->load(path + "silk-foot2-random3.cub", database, true);
    m_sprites[SpriteSilkFoot3Random3]->load(path + "silk-foot3-random3.cub", database, true);
    m_sprites[SpriteSilkFoot4Random3]->load(path + "silk-foot4-random3.cub", database, true);
    m_sprites[SpriteSilkFoot5Random3]->load(path + "silk-foot5-random3.cub", database, true);
    m_sprites[SpriteSilkFoot1Random4]->load(path + "silk-foot1-random4.cub", database, true);
    m_sprites[SpriteSilkFoot2Random4]->load(path + "silk-foot2-random4.cub", database, true);
    m_sprites[SpriteSilkFoot3Random4]->load(path + "silk-foot3-random4.cub", database, true);
    m_sprites[SpriteSilkFoot4Random4]->load(path + "silk-foot4-random4.cub", database, true);
    m_sprites[SpriteSilkFoot5Random4]->load(path + "silk-foot5-random4.cub", database, true);
    m_sprites[SpriteGoldAmulet1Random1]->load(path + "gold-amulet1-random1.cub", database, true);
    m_sprites[SpriteGoldAmulet2Random1]->load(path + "gold-amulet2-random1.cub", database, true);
    m_sprites[SpriteGoldAmulet3Random1]->load(path + "gold-amulet3-random1.cub", database, true);
    m_sprites[SpriteGoldAmulet4Random1]->load(path + "gold-amulet4-random1.cub", database, true);
    m_sprites[SpriteGoldAmulet5Random1]->load(path + "gold-amulet5-random1.cub", database, true);
    m_sprites[SpriteGoldAmulet1Random2]->load(path + "gold-amulet1-random2.cub", database, true);
    m_sprites[SpriteGoldAmulet2Random2]->load(path + "gold-amulet2-random2.cub", database, true);
    m_sprites[SpriteGoldAmulet3Random2]->load(path + "gold-amulet3-random2.cub", database, true);
    m_sprites[SpriteGoldAmulet4Random2]->load(path + "gold-amulet4-random2.cub", database, true);
    m_sprites[SpriteGoldAmulet5Random2]->load(path + "gold-amulet5-random2.cub", database, true);
    m_sprites[SpriteGoldAmulet1Random3]->load(path + "gold-amulet1-random3.cub", database, true);
    m_sprites[SpriteGoldAmulet2Random3]->load(path + "gold-amulet2-random3.cub", database, true);
    m_sprites[SpriteGoldAmulet3Random3]->load(path + "gold-amulet3-random3.cub", database, true);
    m_sprites[SpriteGoldAmulet4Random3]->load(path + "gold-amulet4-random3.cub", database, true);
    m_sprites[SpriteGoldAmulet5Random3]->load(path + "gold-amulet5-random3.cub", database, true);
    m_sprites[SpriteGoldAmulet1Random4]->load(path + "gold-amulet1-random4.cub", database, true);
    m_sprites[SpriteGoldAmulet2Random4]->load(path + "gold-amulet2-random4.cub", database, true);
    m_sprites[SpriteGoldAmulet3Random4]->load(path + "gold-amulet3-random4.cub", database, true);
    m_sprites[SpriteGoldAmulet4Random4]->load(path + "gold-amulet4-random4.cub", database, true);
    m_sprites[SpriteGoldAmulet5Random4]->load(path + "gold-amulet5-random4.cub", database, true);
    m_sprites[SpriteGoldAmulet1Random5]->load(path + "gold-amulet1-random5.cub", database, true);
    m_sprites[SpriteGoldAmulet2Random5]->load(path + "gold-amulet2-random5.cub", database, true);
    m_sprites[SpriteGoldAmulet3Random5]->load(path + "gold-amulet3-random5.cub", database, true);
    m_sprites[SpriteGoldAmulet4Random5]->load(path + "gold-amulet4-random5.cub", database, true);
    m_sprites[SpriteGoldAmulet5Random5]->load(path + "gold-amulet5-random5.cub", database, true);
    m_sprites[SpriteSilverAmulet1Random1]->load(path + "silver-amulet1-random1.cub", database, true);
    m_sprites[SpriteSilverAmulet2Random1]->load(path + "silver-amulet2-random1.cub", database, true);
    m_sprites[SpriteSilverAmulet3Random1]->load(path + "silver-amulet3-random1.cub", database, true);
    m_sprites[SpriteSilverAmulet4Random1]->load(path + "silver-amulet4-random1.cub", database, true);
    m_sprites[SpriteSilverAmulet5Random1]->load(path + "silver-amulet5-random1.cub", database, true);
    m_sprites[SpriteSilverAmulet1Random2]->load(path + "silver-amulet1-random2.cub", database, true);
    m_sprites[SpriteSilverAmulet2Random2]->load(path + "silver-amulet2-random2.cub", database, true);
    m_sprites[SpriteSilverAmulet3Random2]->load(path + "silver-amulet3-random2.cub", database, true);
    m_sprites[SpriteSilverAmulet4Random2]->load(path + "silver-amulet4-random2.cub", database, true);
    m_sprites[SpriteSilverAmulet5Random2]->load(path + "silver-amulet5-random2.cub", database, true);
    m_sprites[SpriteSilverAmulet1Random3]->load(path + "silver-amulet1-random3.cub", database, true);
    m_sprites[SpriteSilverAmulet2Random3]->load(path + "silver-amulet2-random3.cub", database, true);
    m_sprites[SpriteSilverAmulet3Random3]->load(path + "silver-amulet3-random3.cub", database, true);
    m_sprites[SpriteSilverAmulet4Random3]->load(path + "silver-amulet4-random3.cub", database, true);
    m_sprites[SpriteSilverAmulet5Random3]->load(path + "silver-amulet5-random3.cub", database, true);
    m_sprites[SpriteSilverAmulet1Random4]->load(path + "silver-amulet1-random4.cub", database, true);
    m_sprites[SpriteSilverAmulet2Random4]->load(path + "silver-amulet2-random4.cub", database, true);
    m_sprites[SpriteSilverAmulet3Random4]->load(path + "silver-amulet3-random4.cub", database, true);
    m_sprites[SpriteSilverAmulet4Random4]->load(path + "silver-amulet4-random4.cub", database, true);
    m_sprites[SpriteSilverAmulet5Random4]->load(path + "silver-amulet5-random4.cub", database, true);
    m_sprites[SpriteSilverAmulet1Random5]->load(path + "silver-amulet1-random5.cub", database, true);
    m_sprites[SpriteSilverAmulet2Random5]->load(path + "silver-amulet2-random5.cub", database, true);
    m_sprites[SpriteSilverAmulet3Random5]->load(path + "silver-amulet3-random5.cub", database, true);
    m_sprites[SpriteSilverAmulet4Random5]->load(path + "silver-amulet4-random5.cub", database, true);
    m_sprites[SpriteSilverAmulet5Random5]->load(path + "silver-amulet5-random5.cub", database, true);
    m_sprites[SpriteGoldRing1Random1]->load(path + "gold-ring1-random1.cub", database, true);
    m_sprites[SpriteGoldRing2Random1]->load(path + "gold-ring2-random1.cub", database, true);
    m_sprites[SpriteGoldRing3Random1]->load(path + "gold-ring3-random1.cub", database, true);
    m_sprites[SpriteGoldRing4Random1]->load(path + "gold-ring4-random1.cub", database, true);
    m_sprites[SpriteGoldRing5Random1]->load(path + "gold-ring5-random1.cub", database, true);
    m_sprites[SpriteGoldRing1Random2]->load(path + "gold-ring1-random2.cub", database, true);
    m_sprites[SpriteGoldRing2Random2]->load(path + "gold-ring2-random2.cub", database, true);
    m_sprites[SpriteGoldRing3Random2]->load(path + "gold-ring3-random2.cub", database, true);
    m_sprites[SpriteGoldRing4Random2]->load(path + "gold-ring4-random2.cub", database, true);
    m_sprites[SpriteGoldRing5Random2]->load(path + "gold-ring5-random2.cub", database, true);
    m_sprites[SpriteGoldRing1Random3]->load(path + "gold-ring1-random3.cub", database, true);
    m_sprites[SpriteGoldRing2Random3]->load(path + "gold-ring2-random3.cub", database, true);
    m_sprites[SpriteGoldRing3Random3]->load(path + "gold-ring3-random3.cub", database, true);
    m_sprites[SpriteGoldRing4Random3]->load(path + "gold-ring4-random3.cub", database, true);
    m_sprites[SpriteGoldRing5Random3]->load(path + "gold-ring5-random3.cub", database, true);
    m_sprites[SpriteGoldRing1Random4]->load(path + "gold-ring1-random4.cub", database, true);
    m_sprites[SpriteGoldRing2Random4]->load(path + "gold-ring2-random4.cub", database, true);
    m_sprites[SpriteGoldRing3Random4]->load(path + "gold-ring3-random4.cub", database, true);
    m_sprites[SpriteGoldRing4Random4]->load(path + "gold-ring4-random4.cub", database, true);
    m_sprites[SpriteGoldRing5Random4]->load(path + "gold-ring5-random4.cub", database, true);
    m_sprites[SpriteGoldRing1Random5]->load(path + "gold-ring1-random5.cub", database, true);
    m_sprites[SpriteGoldRing2Random5]->load(path + "gold-ring2-random5.cub", database, true);
    m_sprites[SpriteGoldRing3Random5]->load(path + "gold-ring3-random5.cub", database, true);
    m_sprites[SpriteGoldRing4Random5]->load(path + "gold-ring4-random5.cub", database, true);
    m_sprites[SpriteGoldRing5Random5]->load(path + "gold-ring5-random5.cub", database, true);
    m_sprites[SpriteSilverRing1Random1]->load(path + "silver-ring1-random1.cub", database, true);
    m_sprites[SpriteSilverRing2Random1]->load(path + "silver-ring2-random1.cub", database, true);
    m_sprites[SpriteSilverRing3Random1]->load(path + "silver-ring3-random1.cub", database, true);
    m_sprites[SpriteSilverRing4Random1]->load(path + "silver-ring4-random1.cub", database, true);
    m_sprites[SpriteSilverRing5Random1]->load(path + "silver-ring5-random1.cub", database, true);
    m_sprites[SpriteSilverRing1Random2]->load(path + "silver-ring1-random2.cub", database, true);
    m_sprites[SpriteSilverRing2Random2]->load(path + "silver-ring2-random2.cub", database, true);
    m_sprites[SpriteSilverRing3Random2]->load(path + "silver-ring3-random2.cub", database, true);
    m_sprites[SpriteSilverRing4Random2]->load(path + "silver-ring4-random2.cub", database, true);
    m_sprites[SpriteSilverRing5Random2]->load(path + "silver-ring5-random2.cub", database, true);
    m_sprites[SpriteSilverRing1Random3]->load(path + "silver-ring1-random3.cub", database, true);
    m_sprites[SpriteSilverRing2Random3]->load(path + "silver-ring2-random3.cub", database, true);
    m_sprites[SpriteSilverRing3Random3]->load(path + "silver-ring3-random3.cub", database, true);
    m_sprites[SpriteSilverRing4Random3]->load(path + "silver-ring4-random3.cub", database, true);
    m_sprites[SpriteSilverRing5Random3]->load(path + "silver-ring5-random3.cub", database, true);
    m_sprites[SpriteSilverRing1Random4]->load(path + "silver-ring1-random4.cub", database, true);
    m_sprites[SpriteSilverRing2Random4]->load(path + "silver-ring2-random4.cub", database, true);
    m_sprites[SpriteSilverRing3Random4]->load(path + "silver-ring3-random4.cub", database, true);
    m_sprites[SpriteSilverRing4Random4]->load(path + "silver-ring4-random4.cub", database, true);
    m_sprites[SpriteSilverRing5Random4]->load(path + "silver-ring5-random4.cub", database, true);
    m_sprites[SpriteSilverRing1Random5]->load(path + "silver-ring1-random5.cub", database, true);
    m_sprites[SpriteSilverRing2Random5]->load(path + "silver-ring2-random5.cub", database, true);
    m_sprites[SpriteSilverRing3Random5]->load(path + "silver-ring3-random5.cub", database, true);
    m_sprites[SpriteSilverRing4Random5]->load(path + "silver-ring4-random5.cub", database, true);
    m_sprites[SpriteSilverRing5Random5]->load(path + "silver-ring5-random5.cub", database, true);
    m_sprites[SpriteParrotFeather]->load(path + "parrot-feather.cub", database, true);
    m_sprites[SpriteParrotBeak]->load(path + "parrot-beak.cub", database, true);
    m_sprites[SpriteMammothChest]->load(path + "mammoth-chest.cub", database, true);
    m_sprites[SpriteMammothShoulder]->load(path + "mammoth-shoulder.cub", database, true);
    m_sprites[SpriteMammothBoot]->load(path + "mammoth-boot.cub", database, true);
    m_sprites[SpriteMammothGlove]->load(path + "mammoth-glove.cub", database, true);
    m_sprites[SpriteMammothHelmet]->load(path + "mammoth-helmet.cub", database, true);
    m_sprites[SpriteMammothHair]->load(path + "mammoth-hair.cub", database, true);
    m_sprites[SpriteFlask]->load(path + "flask.cub", database, true);
    m_sprites[SpriteWaterFlask]->load(path + "water-flask.cub", database, true);
    m_sprites[SpriteRadishSlice]->load(path + "radish-slice.cub", database, true);
    m_sprites[SpriteOnionSlice]->load(path + "onion-slice.cub", database, true);
    m_sprites[SpritePlantFiber]->load(path + "plant-fiber.cub", database, true);
    m_sprites[SpriteSoupGinseng]->load(path + "soup-ginseng.cub", database, true);
    m_sprites[SpriteMushroomSpit]->load(path + "mushroom-spit.cub", database, true);
    m_sprites[SpriteObsidianSword]->load(path + "obsidian-sword.cub", database, true);
    m_sprites[SpriteBoneChest]->load(path + "bone-chest.cub", database, true);
    m_sprites[SpriteBoneShoulders]->load(path + "bone-shoulders.cub", database, true);
    m_sprites[SpriteBoneBoot]->load(path + "bone-boot.cub", database, true);
    m_sprites[SpriteBoneGlove]->load(path + "bone-glove.cub", database, true);
    m_sprites[SpriteBoneMace]->load(path + "bone-mace.cub", database, true);
    m_sprites[SpriteBoneSword]->load(path + "bone-sword.cub", database, true);
    m_sprites[SpriteBoneAxe]->load(path + "bone-axe.cub", database, true);
    m_sprites[SpriteIronChest1]->load(path + "iron-chest1.cub", database, true);
    m_sprites[SpriteIronChest2]->load(path + "iron-chest2.cub", database, true);
    m_sprites[SpriteIronChest3]->load(path + "iron-chest3.cub", database, true);
    m_sprites[SpriteIronChest4]->load(path + "iron-chest4.cub", database, true);
    m_sprites[SpriteIronChest5]->load(path + "iron-chest5.cub", database, true);
    m_sprites[SpriteIronChest1Random1]->load(path + "iron-chest1-random1.cub", database, true);
    m_sprites[SpriteIronChest2Random1]->load(path + "iron-chest2-random1.cub", database, true);
    m_sprites[SpriteIronChest3Random1]->load(path + "iron-chest3-random1.cub", database, true);
    m_sprites[SpriteIronChest4Random1]->load(path + "iron-chest4-random1.cub", database, true);
    m_sprites[SpriteIronChest5Random1]->load(path + "iron-chest5-random1.cub", database, true);
    m_sprites[SpriteIronChest1Random2]->load(path + "iron-chest1-random2.cub", database, true);
    m_sprites[SpriteIronChest2Random2]->load(path + "iron-chest2-random2.cub", database, true);
    m_sprites[SpriteIronChest3Random2]->load(path + "iron-chest3-random2.cub", database, true);
    m_sprites[SpriteIronChest4Random2]->load(path + "iron-chest4-random2.cub", database, true);
    m_sprites[SpriteIronChest5Random2]->load(path + "iron-chest5-random2.cub", database, true);
    m_sprites[SpriteIronChest1Random3]->load(path + "iron-chest1-random3.cub", database, true);
    m_sprites[SpriteIronChest2Random3]->load(path + "iron-chest2-random3.cub", database, true);
    m_sprites[SpriteIronChest3Random3]->load(path + "iron-chest3-random3.cub", database, true);
    m_sprites[SpriteIronChest4Random3]->load(path + "iron-chest4-random3.cub", database, true);
    m_sprites[SpriteIronChest5Random3]->load(path + "iron-chest5-random3.cub", database, true);
    m_sprites[SpriteIronChest1Random4]->load(path + "iron-chest1-random4.cub", database, true);
    m_sprites[SpriteIronChest2Random4]->load(path + "iron-chest2-random4.cub", database, true);
    m_sprites[SpriteIronChest3Random4]->load(path + "iron-chest3-random4.cub", database, true);
    m_sprites[SpriteIronChest4Random4]->load(path + "iron-chest4-random4.cub", database, true);
    m_sprites[SpriteIronChest5Random4]->load(path + "iron-chest5-random4.cub", database, true);
    m_sprites[SpriteIronShoulder1]->load(path + "iron-shoulder1.cub", database, true);
    m_sprites[SpriteIronShoulder2]->load(path + "iron-shoulder2.cub", database, true);
    m_sprites[SpriteIronShoulder3]->load(path + "iron-shoulder3.cub", database, true);
    m_sprites[SpriteIronShoulder4]->load(path + "iron-shoulder4.cub", database, true);
    m_sprites[SpriteIronShoulder5]->load(path + "iron-shoulder5.cub", database, true);
    m_sprites[SpriteIronShoulder1Random1]->load(path + "iron-shoulder1-random1.cub", database, true);
    m_sprites[SpriteIronShoulder2Random1]->load(path + "iron-shoulder2-random1.cub", database, true);
    m_sprites[SpriteIronShoulder3Random1]->load(path + "iron-shoulder3-random1.cub", database, true);
    m_sprites[SpriteIronShoulder4Random1]->load(path + "iron-shoulder4-random1.cub", database, true);
    m_sprites[SpriteIronShoulder5Random1]->load(path + "iron-shoulder5-random1.cub", database, true);
    m_sprites[SpriteIronShoulder1Random2]->load(path + "iron-shoulder1-random2.cub", database, true);
    m_sprites[SpriteIronShoulder2Random2]->load(path + "iron-shoulder2-random2.cub", database, true);
    m_sprites[SpriteIronShoulder3Random2]->load(path + "iron-shoulder3-random2.cub", database, true);
    m_sprites[SpriteIronShoulder4Random2]->load(path + "iron-shoulder4-random2.cub", database, true);
    m_sprites[SpriteIronShoulder5Random2]->load(path + "iron-shoulder5-random2.cub", database, true);
    m_sprites[SpriteIronShoulder1Random3]->load(path + "iron-shoulder1-random3.cub", database, true);
    m_sprites[SpriteIronShoulder2Random3]->load(path + "iron-shoulder2-random3.cub", database, true);
    m_sprites[SpriteIronShoulder3Random3]->load(path + "iron-shoulder3-random3.cub", database, true);
    m_sprites[SpriteIronShoulder4Random3]->load(path + "iron-shoulder4-random3.cub", database, true);
    m_sprites[SpriteIronShoulder5Random3]->load(path + "iron-shoulder5-random3.cub", database, true);
    m_sprites[SpriteIronShoulder1Random4]->load(path + "iron-shoulder1-random4.cub", database, true);
    m_sprites[SpriteIronShoulder2Random4]->load(path + "iron-shoulder2-random4.cub", database, true);
    m_sprites[SpriteIronShoulder3Random4]->load(path + "iron-shoulder3-random4.cub", database, true);
    m_sprites[SpriteIronShoulder4Random4]->load(path + "iron-shoulder4-random4.cub", database, true);
    m_sprites[SpriteIronShoulder5Random4]->load(path + "iron-shoulder5-random4.cub", database, true);
    m_sprites[SpriteIronHand1]->load(path + "iron-hand1.cub", database, true);
    m_sprites[SpriteIronHand2]->load(path + "iron-hand2.cub", database, true);
    m_sprites[SpriteIronHand3]->load(path + "iron-hand3.cub", database, true);
    m_sprites[SpriteIronHand4]->load(path + "iron-hand4.cub", database, true);
    m_sprites[SpriteIronHand5]->load(path + "iron-hand5.cub", database, true);
    m_sprites[SpriteIronHand1Random1]->load(path + "iron-hand1-random1.cub", database, true);
    m_sprites[SpriteIronHand2Random1]->load(path + "iron-hand2-random1.cub", database, true);
    m_sprites[SpriteIronHand3Random1]->load(path + "iron-hand3-random1.cub", database, true);
    m_sprites[SpriteIronHand4Random1]->load(path + "iron-hand4-random1.cub", database, true);
    m_sprites[SpriteIronHand5Random1]->load(path + "iron-hand5-random1.cub", database, true);
    m_sprites[SpriteIronHand1Random2]->load(path + "iron-hand1-random2.cub", database, true);
    m_sprites[SpriteIronHand2Random2]->load(path + "iron-hand2-random2.cub", database, true);
    m_sprites[SpriteIronHand3Random2]->load(path + "iron-hand3-random2.cub", database, true);
    m_sprites[SpriteIronHand4Random2]->load(path + "iron-hand4-random2.cub", database, true);
    m_sprites[SpriteIronHand5Random2]->load(path + "iron-hand5-random2.cub", database, true);
    m_sprites[SpriteIronHand1Random3]->load(path + "iron-hand1-random3.cub", database, true);
    m_sprites[SpriteIronHand2Random3]->load(path + "iron-hand2-random3.cub", database, true);
    m_sprites[SpriteIronHand3Random3]->load(path + "iron-hand3-random3.cub", database, true);
    m_sprites[SpriteIronHand4Random3]->load(path + "iron-hand4-random3.cub", database, true);
    m_sprites[SpriteIronHand5Random3]->load(path + "iron-hand5-random3.cub", database, true);
    m_sprites[SpriteIronHand1Random4]->load(path + "iron-hand1-random4.cub", database, true);
    m_sprites[SpriteIronHand2Random4]->load(path + "iron-hand2-random4.cub", database, true);
    m_sprites[SpriteIronHand3Random4]->load(path + "iron-hand3-random4.cub", database, true);
    m_sprites[SpriteIronHand4Random4]->load(path + "iron-hand4-random4.cub", database, true);
    m_sprites[SpriteIronHand5Random4]->load(path + "iron-hand5-random4.cub", database, true);
    m_sprites[SpriteIronFoot1]->load(path + "iron-foot1.cub", database, true);
    m_sprites[SpriteIronFoot2]->load(path + "iron-foot2.cub", database, true);
    m_sprites[SpriteIronFoot3]->load(path + "iron-foot3.cub", database, true);
    m_sprites[SpriteIronFoot4]->load(path + "iron-foot4.cub", database, true);
    m_sprites[SpriteIronFoot5]->load(path + "iron-foot5.cub", database, true);
    m_sprites[SpriteIronFoot1Random1]->load(path + "iron-foot1-random1.cub", database, true);
    m_sprites[SpriteIronFoot2Random1]->load(path + "iron-foot2-random1.cub", database, true);
    m_sprites[SpriteIronFoot3Random1]->load(path + "iron-foot3-random1.cub", database, true);
    m_sprites[SpriteIronFoot4Random1]->load(path + "iron-foot4-random1.cub", database, true);
    m_sprites[SpriteIronFoot5Random1]->load(path + "iron-foot5-random1.cub", database, true);
    m_sprites[SpriteIronFoot1Random2]->load(path + "iron-foot1-random2.cub", database, true);
    m_sprites[SpriteIronFoot2Random2]->load(path + "iron-foot2-random2.cub", database, true);
    m_sprites[SpriteIronFoot3Random2]->load(path + "iron-foot3-random2.cub", database, true);
    m_sprites[SpriteIronFoot4Random2]->load(path + "iron-foot4-random2.cub", database, true);
    m_sprites[SpriteIronFoot5Random2]->load(path + "iron-foot5-random2.cub", database, true);
    m_sprites[SpriteIronFoot1Random3]->load(path + "iron-foot1-random3.cub", database, true);
    m_sprites[SpriteIronFoot2Random3]->load(path + "iron-foot2-random3.cub", database, true);
    m_sprites[SpriteIronFoot3Random3]->load(path + "iron-foot3-random3.cub", database, true);
    m_sprites[SpriteIronFoot4Random3]->load(path + "iron-foot4-random3.cub", database, true);
    m_sprites[SpriteIronFoot5Random3]->load(path + "iron-foot5-random3.cub", database, true);
    m_sprites[SpriteIronFoot1Random4]->load(path + "iron-foot1-random4.cub", database, true);
    m_sprites[SpriteIronFoot2Random4]->load(path + "iron-foot2-random4.cub", database, true);
    m_sprites[SpriteIronFoot3Random4]->load(path + "iron-foot3-random4.cub", database, true);
    m_sprites[SpriteIronFoot4Random4]->load(path + "iron-foot4-random4.cub", database, true);
    m_sprites[SpriteIronFoot5Random4]->load(path + "iron-foot5-random4.cub", database, true);
    m_sprites[SpriteLinenChest1]->load(path + "linen-chest1.cub", database, true);
    m_sprites[SpriteLinenChest2]->load(path + "linen-chest2.cub", database, true);
    m_sprites[SpriteLinenChest3]->load(path + "linen-chest3.cub", database, true);
    m_sprites[SpriteLinenChest4]->load(path + "linen-chest4.cub", database, true);
    m_sprites[SpriteLinenChest5]->load(path + "linen-chest5.cub", database, true);
    m_sprites[SpriteLinenChest1Random1]->load(path + "linen-chest1-random1.cub", database, true);
    m_sprites[SpriteLinenChest2Random1]->load(path + "linen-chest2-random1.cub", database, true);
    m_sprites[SpriteLinenChest3Random1]->load(path + "linen-chest3-random1.cub", database, true);
    m_sprites[SpriteLinenChest4Random1]->load(path + "linen-chest4-random1.cub", database, true);
    m_sprites[SpriteLinenChest5Random1]->load(path + "linen-chest5-random1.cub", database, true);
    m_sprites[SpriteLinenChest1Random2]->load(path + "linen-chest1-random2.cub", database, true);
    m_sprites[SpriteLinenChest2Random2]->load(path + "linen-chest2-random2.cub", database, true);
    m_sprites[SpriteLinenChest3Random2]->load(path + "linen-chest3-random2.cub", database, true);
    m_sprites[SpriteLinenChest4Random2]->load(path + "linen-chest4-random2.cub", database, true);
    m_sprites[SpriteLinenChest5Random2]->load(path + "linen-chest5-random2.cub", database, true);
    m_sprites[SpriteLinenChest1Random3]->load(path + "linen-chest1-random3.cub", database, true);
    m_sprites[SpriteLinenChest2Random3]->load(path + "linen-chest2-random3.cub", database, true);
    m_sprites[SpriteLinenChest3Random3]->load(path + "linen-chest3-random3.cub", database, true);
    m_sprites[SpriteLinenChest4Random3]->load(path + "linen-chest4-random3.cub", database, true);
    m_sprites[SpriteLinenChest5Random3]->load(path + "linen-chest5-random3.cub", database, true);
    m_sprites[SpriteLinenChest1Random4]->load(path + "linen-chest1-random4.cub", database, true);
    m_sprites[SpriteLinenChest2Random4]->load(path + "linen-chest2-random4.cub", database, true);
    m_sprites[SpriteLinenChest3Random4]->load(path + "linen-chest3-random4.cub", database, true);
    m_sprites[SpriteLinenChest4Random4]->load(path + "linen-chest4-random4.cub", database, true);
    m_sprites[SpriteLinenChest5Random4]->load(path + "linen-chest5-random4.cub", database, true);
    m_sprites[SpriteLinenShoulder1]->load(path + "linen-shoulder1.cub", database, true);
    m_sprites[SpriteLinenShoulder2]->load(path + "linen-shoulder2.cub", database, true);
    m_sprites[SpriteLinenShoulder3]->load(path + "linen-shoulder3.cub", database, true);
    m_sprites[SpriteLinenShoulder4]->load(path + "linen-shoulder4.cub", database, true);
    m_sprites[SpriteLinenShoulder5]->load(path + "linen-shoulder5.cub", database, true);
    m_sprites[SpriteLinenShoulder1Random1]->load(path + "linen-shoulder1-random1.cub", database, true);
    m_sprites[SpriteLinenShoulder2Random1]->load(path + "linen-shoulder2-random1.cub", database, true);
    m_sprites[SpriteLinenShoulder3Random1]->load(path + "linen-shoulder3-random1.cub", database, true);
    m_sprites[SpriteLinenShoulder4Random1]->load(path + "linen-shoulder4-random1.cub", database, true);
    m_sprites[SpriteLinenShoulder5Random1]->load(path + "linen-shoulder5-random1.cub", database, true);
    m_sprites[SpriteLinenShoulder1Random2]->load(path + "linen-shoulder1-random2.cub", database, true);
    m_sprites[SpriteLinenShoulder2Random2]->load(path + "linen-shoulder2-random2.cub", database, true);
    m_sprites[SpriteLinenShoulder3Random2]->load(path + "linen-shoulder3-random2.cub", database, true);
    m_sprites[SpriteLinenShoulder4Random2]->load(path + "linen-shoulder4-random2.cub", database, true);
    m_sprites[SpriteLinenShoulder5Random2]->load(path + "linen-shoulder5-random2.cub", database, true);
    m_sprites[SpriteLinenShoulder1Random3]->load(path + "linen-shoulder1-random3.cub", database, true);
    m_sprites[SpriteLinenShoulder2Random3]->load(path + "linen-shoulder2-random3.cub", database, true);
    m_sprites[SpriteLinenShoulder3Random3]->load(path + "linen-shoulder3-random3.cub", database, true);
    m_sprites[SpriteLinenShoulder4Random3]->load(path + "linen-shoulder4-random3.cub", database, true);
    m_sprites[SpriteLinenShoulder5Random3]->load(path + "linen-shoulder5-random3.cub", database, true);
    m_sprites[SpriteLinenShoulder1Random4]->load(path + "linen-shoulder1-random4.cub", database, true);
    m_sprites[SpriteLinenShoulder2Random4]->load(path + "linen-shoulder2-random4.cub", database, true);
    m_sprites[SpriteLinenShoulder3Random4]->load(path + "linen-shoulder3-random4.cub", database, true);
    m_sprites[SpriteLinenShoulder4Random4]->load(path + "linen-shoulder4-random4.cub", database, true);
    m_sprites[SpriteLinenShoulder5Random4]->load(path + "linen-shoulder5-random4.cub", database, true);
    m_sprites[SpriteLinenHand1]->load(path + "linen-hand1.cub", database, true);
    m_sprites[SpriteLinenHand2]->load(path + "linen-hand2.cub", database, true);
    m_sprites[SpriteLinenHand3]->load(path + "linen-hand3.cub", database, true);
    m_sprites[SpriteLinenHand4]->load(path + "linen-hand4.cub", database, true);
    m_sprites[SpriteLinenHand5]->load(path + "linen-hand5.cub", database, true);
    m_sprites[SpriteLinenHand1Random1]->load(path + "linen-hand1-random1.cub", database, true);
    m_sprites[SpriteLinenHand2Random1]->load(path + "linen-hand2-random1.cub", database, true);
    m_sprites[SpriteLinenHand3Random1]->load(path + "linen-hand3-random1.cub", database, true);
    m_sprites[SpriteLinenHand4Random1]->load(path + "linen-hand4-random1.cub", database, true);
    m_sprites[SpriteLinenHand5Random1]->load(path + "linen-hand5-random1.cub", database, true);
    m_sprites[SpriteLinenHand1Random2]->load(path + "linen-hand1-random2.cub", database, true);
    m_sprites[SpriteLinenHand2Random2]->load(path + "linen-hand2-random2.cub", database, true);
    m_sprites[SpriteLinenHand3Random2]->load(path + "linen-hand3-random2.cub", database, true);
    m_sprites[SpriteLinenHand4Random2]->load(path + "linen-hand4-random2.cub", database, true);
    m_sprites[SpriteLinenHand5Random2]->load(path + "linen-hand5-random2.cub", database, true);
    m_sprites[SpriteLinenHand1Random3]->load(path + "linen-hand1-random3.cub", database, true);
    m_sprites[SpriteLinenHand2Random3]->load(path + "linen-hand2-random3.cub", database, true);
    m_sprites[SpriteLinenHand3Random3]->load(path + "linen-hand3-random3.cub", database, true);
    m_sprites[SpriteLinenHand4Random3]->load(path + "linen-hand4-random3.cub", database, true);
    m_sprites[SpriteLinenHand5Random3]->load(path + "linen-hand5-random3.cub", database, true);
    m_sprites[SpriteLinenHand1Random4]->load(path + "linen-hand1-random4.cub", database, true);
    m_sprites[SpriteLinenHand2Random4]->load(path + "linen-hand2-random4.cub", database, true);
    m_sprites[SpriteLinenHand3Random4]->load(path + "linen-hand3-random4.cub", database, true);
    m_sprites[SpriteLinenHand4Random4]->load(path + "linen-hand4-random4.cub", database, true);
    m_sprites[SpriteLinenHand5Random4]->load(path + "linen-hand5-random4.cub", database, true);
    m_sprites[SpriteLinenFoot1]->load(path + "linen-foot1.cub", database, true);
    m_sprites[SpriteLinenFoot2]->load(path + "linen-foot2.cub", database, true);
    m_sprites[SpriteLinenFoot3]->load(path + "linen-foot3.cub", database, true);
    m_sprites[SpriteLinenFoot4]->load(path + "linen-foot4.cub", database, true);
    m_sprites[SpriteLinenFoot5]->load(path + "linen-foot5.cub", database, true);
    m_sprites[SpriteLinenFoot1Random1]->load(path + "linen-foot1-random1.cub", database, true);
    m_sprites[SpriteLinenFoot2Random1]->load(path + "linen-foot2-random1.cub", database, true);
    m_sprites[SpriteLinenFoot3Random1]->load(path + "linen-foot3-random1.cub", database, true);
    m_sprites[SpriteLinenFoot4Random1]->load(path + "linen-foot4-random1.cub", database, true);
    m_sprites[SpriteLinenFoot5Random1]->load(path + "linen-foot5-random1.cub", database, true);
    m_sprites[SpriteLinenFoot1Random2]->load(path + "linen-foot1-random2.cub", database, true);
    m_sprites[SpriteLinenFoot2Random2]->load(path + "linen-foot2-random2.cub", database, true);
    m_sprites[SpriteLinenFoot3Random2]->load(path + "linen-foot3-random2.cub", database, true);
    m_sprites[SpriteLinenFoot4Random2]->load(path + "linen-foot4-random2.cub", database, true);
    m_sprites[SpriteLinenFoot5Random2]->load(path + "linen-foot5-random2.cub", database, true);
    m_sprites[SpriteLinenFoot1Random3]->load(path + "linen-foot1-random3.cub", database, true);
    m_sprites[SpriteLinenFoot2Random3]->load(path + "linen-foot2-random3.cub", database, true);
    m_sprites[SpriteLinenFoot3Random3]->load(path + "linen-foot3-random3.cub", database, true);
    m_sprites[SpriteLinenFoot4Random3]->load(path + "linen-foot4-random3.cub", database, true);
    m_sprites[SpriteLinenFoot5Random3]->load(path + "linen-foot5-random3.cub", database, true);
    m_sprites[SpriteLinenFoot1Random4]->load(path + "linen-foot1-random4.cub", database, true);
    m_sprites[SpriteLinenFoot2Random4]->load(path + "linen-foot2-random4.cub", database, true);
    m_sprites[SpriteLinenFoot3Random4]->load(path + "linen-foot3-random4.cub", database, true);
    m_sprites[SpriteLinenFoot4Random4]->load(path + "linen-foot4-random4.cub", database, true);
    m_sprites[SpriteLinenFoot5Random4]->load(path + "linen-foot5-random4.cub", database, true);
    m_sprites[SpriteWoolChest1]->load(path + "wool-chest1.cub", database, true);
    m_sprites[SpriteWoolChest2]->load(path + "wool-chest2.cub", database, true);
    m_sprites[SpriteWoolChest3]->load(path + "wool-chest3.cub", database, true);
    m_sprites[SpriteWoolChest4]->load(path + "wool-chest4.cub", database, true);
    m_sprites[SpriteWoolChest5]->load(path + "wool-chest5.cub", database, true);
    m_sprites[SpriteWoolChest1Random1]->load(path + "wool-chest1-random1.cub", database, true);
    m_sprites[SpriteWoolChest2Random1]->load(path + "wool-chest2-random1.cub", database, true);
    m_sprites[SpriteWoolChest3Random1]->load(path + "wool-chest3-random1.cub", database, true);
    m_sprites[SpriteWoolChest4Random1]->load(path + "wool-chest4-random1.cub", database, true);
    m_sprites[SpriteWoolChest5Random1]->load(path + "wool-chest5-random1.cub", database, true);
    m_sprites[SpriteWoolChest1Random2]->load(path + "wool-chest1-random2.cub", database, true);
    m_sprites[SpriteWoolChest2Random2]->load(path + "wool-chest2-random2.cub", database, true);
    m_sprites[SpriteWoolChest3Random2]->load(path + "wool-chest3-random2.cub", database, true);
    m_sprites[SpriteWoolChest4Random2]->load(path + "wool-chest4-random2.cub", database, true);
    m_sprites[SpriteWoolChest5Random2]->load(path + "wool-chest5-random2.cub", database, true);
    m_sprites[SpriteWoolChest1Random3]->load(path + "wool-chest1-random3.cub", database, true);
    m_sprites[SpriteWoolChest2Random3]->load(path + "wool-chest2-random3.cub", database, true);
    m_sprites[SpriteWoolChest3Random3]->load(path + "wool-chest3-random3.cub", database, true);
    m_sprites[SpriteWoolChest4Random3]->load(path + "wool-chest4-random3.cub", database, true);
    m_sprites[SpriteWoolChest5Random3]->load(path + "wool-chest5-random3.cub", database, true);
    m_sprites[SpriteWoolChest1Random4]->load(path + "wool-chest1-random4.cub", database, true);
    m_sprites[SpriteWoolChest2Random4]->load(path + "wool-chest2-random4.cub", database, true);
    m_sprites[SpriteWoolChest3Random4]->load(path + "wool-chest3-random4.cub", database, true);
    m_sprites[SpriteWoolChest4Random4]->load(path + "wool-chest4-random4.cub", database, true);
    m_sprites[SpriteWoolChest5Random4]->load(path + "wool-chest5-random4.cub", database, true);
    m_sprites[SpriteWoolShoulder1]->load(path + "wool-shoulder1.cub", database, true);
    m_sprites[SpriteWoolShoulder2]->load(path + "wool-shoulder2.cub", database, true);
    m_sprites[SpriteWoolShoulder3]->load(path + "wool-shoulder3.cub", database, true);
    m_sprites[SpriteWoolShoulder4]->load(path + "wool-shoulder4.cub", database, true);
    m_sprites[SpriteWoolShoulder5]->load(path + "wool-shoulder5.cub", database, true);
    m_sprites[SpriteWoolShoulder1Random1]->load(path + "wool-shoulder1-random1.cub", database, true);
    m_sprites[SpriteWoolShoulder2Random1]->load(path + "wool-shoulder2-random1.cub", database, true);
    m_sprites[SpriteWoolShoulder3Random1]->load(path + "wool-shoulder3-random1.cub", database, true);
    m_sprites[SpriteWoolShoulder4Random1]->load(path + "wool-shoulder4-random1.cub", database, true);
    m_sprites[SpriteWoolShoulder5Random1]->load(path + "wool-shoulder5-random1.cub", database, true);
    m_sprites[SpriteWoolShoulder1Random2]->load(path + "wool-shoulder1-random2.cub", database, true);
    m_sprites[SpriteWoolShoulder2Random2]->load(path + "wool-shoulder2-random2.cub", database, true);
    m_sprites[SpriteWoolShoulder3Random2]->load(path + "wool-shoulder3-random2.cub", database, true);
    m_sprites[SpriteWoolShoulder4Random2]->load(path + "wool-shoulder4-random2.cub", database, true);
    m_sprites[SpriteWoolShoulder5Random2]->load(path + "wool-shoulder5-random2.cub", database, true);
    m_sprites[SpriteWoolShoulder1Random3]->load(path + "wool-shoulder1-random3.cub", database, true);
    m_sprites[SpriteWoolShoulder2Random3]->load(path + "wool-shoulder2-random3.cub", database, true);
    m_sprites[SpriteWoolShoulder3Random3]->load(path + "wool-shoulder3-random3.cub", database, true);
    m_sprites[SpriteWoolShoulder4Random3]->load(path + "wool-shoulder4-random3.cub", database, true);
    m_sprites[SpriteWoolShoulder5Random3]->load(path + "wool-shoulder5-random3.cub", database, true);
    m_sprites[SpriteWoolShoulder1Random4]->load(path + "wool-shoulder1-random4.cub", database, true);
    m_sprites[SpriteWoolShoulder2Random4]->load(path + "wool-shoulder2-random4.cub", database, true);
    m_sprites[SpriteWoolShoulder3Random4]->load(path + "wool-shoulder3-random4.cub", database, true);
    m_sprites[SpriteWoolShoulder4Random4]->load(path + "wool-shoulder4-random4.cub", database, true);
    m_sprites[SpriteWoolShoulder5Random4]->load(path + "wool-shoulder5-random4.cub", database, true);
    m_sprites[SpriteWoolHand1]->load(path + "wool-hand1.cub", database, true);
    m_sprites[SpriteWoolHand2]->load(path + "wool-hand2.cub", database, true);
    m_sprites[SpriteWoolHand3]->load(path + "wool-hand3.cub", database, true);
    m_sprites[SpriteWoolHand4]->load(path + "wool-hand4.cub", database, true);
    m_sprites[SpriteWoolHand5]->load(path + "wool-hand5.cub", database, true);
    m_sprites[SpriteWoolHand1Random1]->load(path + "wool-hand1-random1.cub", database, true);
    m_sprites[SpriteWoolHand2Random1]->load(path + "wool-hand2-random1.cub", database, true);
    m_sprites[SpriteWoolHand3Random1]->load(path + "wool-hand3-random1.cub", database, true);
    m_sprites[SpriteWoolHand4Random1]->load(path + "wool-hand4-random1.cub", database, true);
    m_sprites[SpriteWoolHand5Random1]->load(path + "wool-hand5-random1.cub", database, true);
    m_sprites[SpriteWoolHand1Random2]->load(path + "wool-hand1-random2.cub", database, true);
    m_sprites[SpriteWoolHand2Random2]->load(path + "wool-hand2-random2.cub", database, true);
    m_sprites[SpriteWoolHand3Random2]->load(path + "wool-hand3-random2.cub", database, true);
    m_sprites[SpriteWoolHand4Random2]->load(path + "wool-hand4-random2.cub", database, true);
    m_sprites[SpriteWoolHand5Random2]->load(path + "wool-hand5-random2.cub", database, true);
    m_sprites[SpriteWoolHand1Random3]->load(path + "wool-hand1-random3.cub", database, true);
    m_sprites[SpriteWoolHand2Random3]->load(path + "wool-hand2-random3.cub", database, true);
    m_sprites[SpriteWoolHand3Random3]->load(path + "wool-hand3-random3.cub", database, true);
    m_sprites[SpriteWoolHand4Random3]->load(path + "wool-hand4-random3.cub", database, true);
    m_sprites[SpriteWoolHand5Random3]->load(path + "wool-hand5-random3.cub", database, true);
    m_sprites[SpriteWoolHand1Random4]->load(path + "wool-hand1-random4.cub", database, true);
    m_sprites[SpriteWoolHand2Random4]->load(path + "wool-hand2-random4.cub", database, true);
    m_sprites[SpriteWoolHand3Random4]->load(path + "wool-hand3-random4.cub", database, true);
    m_sprites[SpriteWoolHand4Random4]->load(path + "wool-hand4-random4.cub", database, true);
    m_sprites[SpriteWoolHand5Random4]->load(path + "wool-hand5-random4.cub", database, true);
    m_sprites[SpriteWoolFoot1]->load(path + "wool-foot1.cub", database, true);
    m_sprites[SpriteWoolFoot2]->load(path + "wool-foot2.cub", database, true);
    m_sprites[SpriteWoolFoot3]->load(path + "wool-foot3.cub", database, true);
    m_sprites[SpriteWoolFoot4]->load(path + "wool-foot4.cub", database, true);
    m_sprites[SpriteWoolFoot5]->load(path + "wool-foot5.cub", database, true);
    m_sprites[SpriteWoolFoot1Random1]->load(path + "wool-foot1-random1.cub", database, true);
    m_sprites[SpriteWoolFoot2Random1]->load(path + "wool-foot2-random1.cub", database, true);
    m_sprites[SpriteWoolFoot3Random1]->load(path + "wool-foot3-random1.cub", database, true);
    m_sprites[SpriteWoolFoot4Random1]->load(path + "wool-foot4-random1.cub", database, true);
    m_sprites[SpriteWoolFoot5Random1]->load(path + "wool-foot5-random1.cub", database, true);
    m_sprites[SpriteWoolFoot1Random2]->load(path + "wool-foot1-random2.cub", database, true);
    m_sprites[SpriteWoolFoot2Random2]->load(path + "wool-foot2-random2.cub", database, true);
    m_sprites[SpriteWoolFoot3Random2]->load(path + "wool-foot3-random2.cub", database, true);
    m_sprites[SpriteWoolFoot4Random2]->load(path + "wool-foot4-random2.cub", database, true);
    m_sprites[SpriteWoolFoot5Random2]->load(path + "wool-foot5-random2.cub", database, true);
    m_sprites[SpriteWoolFoot1Random3]->load(path + "wool-foot1-random3.cub", database, true);
    m_sprites[SpriteWoolFoot2Random3]->load(path + "wool-foot2-random3.cub", database, true);
    m_sprites[SpriteWoolFoot3Random3]->load(path + "wool-foot3-random3.cub", database, true);
    m_sprites[SpriteWoolFoot4Random3]->load(path + "wool-foot4-random3.cub", database, true);
    m_sprites[SpriteWoolFoot5Random3]->load(path + "wool-foot5-random3.cub", database, true);
    m_sprites[SpriteWoolFoot1Random4]->load(path + "wool-foot1-random4.cub", database, true);
    m_sprites[SpriteWoolFoot2Random4]->load(path + "wool-foot2-random4.cub", database, true);
    m_sprites[SpriteWoolFoot3Random4]->load(path + "wool-foot3-random4.cub", database, true);
    m_sprites[SpriteWoolFoot4Random4]->load(path + "wool-foot4-random4.cub", database, true);
    m_sprites[SpriteWoolFoot5Random4]->load(path + "wool-foot5-random4.cub", database, true);
    m_sprites[SpriteIronShield01]->load(path + "iron-shield01.cub", database, true);
    m_sprites[SpriteIronShield02]->load(path + "iron-shield02.cub", database, true);
    m_sprites[SpriteIronShield03]->load(path + "iron-shield03.cub", database, true);
    m_sprites[SpriteIronShield04]->load(path + "iron-shield04.cub", database, true);
    m_sprites[SpriteIronShield05]->load(path + "iron-shield05.cub", database, true);
    m_sprites[SpriteIronShield1Random1]->load(path + "iron-shield1-random1.cub", database, true);
    m_sprites[SpriteIronShield2Random1]->load(path + "iron-shield2-random1.cub", database, true);
    m_sprites[SpriteIronShield3Random1]->load(path + "iron-shield3-random1.cub", database, true);
    m_sprites[SpriteIronShield4Random1]->load(path + "iron-shield4-random1.cub", database, true);
    m_sprites[SpriteIronShield5Random1]->load(path + "iron-shield5-random1.cub", database, true);
    m_sprites[SpriteIronShield1Random2]->load(path + "iron-shield1-random2.cub", database, true);
    m_sprites[SpriteIronShield2Random2]->load(path + "iron-shield2-random2.cub", database, true);
    m_sprites[SpriteIronShield3Random2]->load(path + "iron-shield3-random2.cub", database, true);
    m_sprites[SpriteIronShield4Random2]->load(path + "iron-shield4-random2.cub", database, true);
    m_sprites[SpriteIronShield5Random2]->load(path + "iron-shield5-random2.cub", database, true);
    m_sprites[SpriteIronShield1Random3]->load(path + "iron-shield1-random3.cub", database, true);
    m_sprites[SpriteIronShield2Random3]->load(path + "iron-shield2-random3.cub", database, true);
    m_sprites[SpriteIronShield3Random3]->load(path + "iron-shield3-random3.cub", database, true);
    m_sprites[SpriteIronShield4Random3]->load(path + "iron-shield4-random3.cub", database, true);
    m_sprites[SpriteIronShield5Random3]->load(path + "iron-shield5-random3.cub", database, true);
    m_sprites[SpriteIronShield1Random4]->load(path + "iron-shield1-random4.cub", database, true);
    m_sprites[SpriteIronShield2Random4]->load(path + "iron-shield2-random4.cub", database, true);
    m_sprites[SpriteIronShield3Random4]->load(path + "iron-shield3-random4.cub", database, true);
    m_sprites[SpriteIronShield4Random4]->load(path + "iron-shield4-random4.cub", database, true);
    m_sprites[SpriteIronShield5Random4]->load(path + "iron-shield5-random4.cub", database, true);
    m_sprites[SpriteIronShield1Random5]->load(path + "iron-shield1-random5.cub", database, true);
    m_sprites[SpriteIronShield2Random5]->load(path + "iron-shield2-random5.cub", database, true);
    m_sprites[SpriteIronShield3Random5]->load(path + "iron-shield3-random5.cub", database, true);
    m_sprites[SpriteIronShield4Random5]->load(path + "iron-shield4-random5.cub", database, true);
    m_sprites[SpriteIronShield5Random5]->load(path + "iron-shield5-random5.cub", database, true);
    m_sprites[SpriteIronShield1Random6]->load(path + "iron-shield1-random6.cub", database, true);
    m_sprites[SpriteIronShield2Random6]->load(path + "iron-shield2-random6.cub", database, true);
    m_sprites[SpriteIronShield3Random6]->load(path + "iron-shield3-random6.cub", database, true);
    m_sprites[SpriteIronShield4Random6]->load(path + "iron-shield4-random6.cub", database, true);
    m_sprites[SpriteIronShield5Random6]->load(path + "iron-shield5-random6.cub", database, true);
    m_sprites[SpriteIronShield1Random7]->load(path + "iron-shield1-random7.cub", database, true);
    m_sprites[SpriteIronShield2Random7]->load(path + "iron-shield2-random7.cub", database, true);
    m_sprites[SpriteIronShield3Random7]->load(path + "iron-shield3-random7.cub", database, true);
    m_sprites[SpriteIronShield4Random7]->load(path + "iron-shield4-random7.cub", database, true);
    m_sprites[SpriteIronShield5Random7]->load(path + "iron-shield5-random7.cub", database, true);
    m_sprites[SpriteIronShield1Random8]->load(path + "iron-shield1-random8.cub", database, true);
    m_sprites[SpriteIronShield2Random8]->load(path + "iron-shield2-random8.cub", database, true);
    m_sprites[SpriteIronShield3Random8]->load(path + "iron-shield3-random8.cub", database, true);
    m_sprites[SpriteIronShield4Random8]->load(path + "iron-shield4-random8.cub", database, true);
    m_sprites[SpriteIronShield5Random8]->load(path + "iron-shield5-random8.cub", database, true);
    m_sprites[SpriteIronShield1Random9]->load(path + "iron-shield1-random9.cub", database, true);
    m_sprites[SpriteIronShield2Random9]->load(path + "iron-shield2-random9.cub", database, true);
    m_sprites[SpriteIronShield3Random9]->load(path + "iron-shield3-random9.cub", database, true);
    m_sprites[SpriteIronShield4Random9]->load(path + "iron-shield4-random9.cub", database, true);
    m_sprites[SpriteIronShield5Random9]->load(path + "iron-shield5-random9.cub", database, true);
    m_sprites[SpriteIronShield1Random10]->load(path + "iron-shield1-random10.cub", database, true);
    m_sprites[SpriteIronShield2Random10]->load(path + "iron-shield2-random10.cub", database, true);
    m_sprites[SpriteIronShield3Random10]->load(path + "iron-shield3-random10.cub", database, true);
    m_sprites[SpriteIronShield4Random10]->load(path + "iron-shield4-random10.cub", database, true);
    m_sprites[SpriteIronShield5Random10]->load(path + "iron-shield5-random10.cub", database, true);
    m_sprites[SpriteWoodShield01]->load(path + "wood-shield01.cub", database, true);
    m_sprites[SpriteWoodShield02]->load(path + "wood-shield02.cub", database, true);
    m_sprites[SpriteWoodShield03]->load(path + "wood-shield03.cub", database, true);
    m_sprites[SpriteWoodShield04]->load(path + "wood-shield04.cub", database, true);
    m_sprites[SpriteWoodShield05]->load(path + "wood-shield05.cub", database, true);
    m_sprites[SpriteIronAxe1]->load(path + "iron-axe1.cub", database, true);
    m_sprites[SpriteIronAxe2]->load(path + "iron-axe2.cub", database, true);
    m_sprites[SpriteIronAxe3]->load(path + "iron-axe3.cub", database, true);
    m_sprites[SpriteIronAxe4]->load(path + "iron-axe4.cub", database, true);
    m_sprites[SpriteIronAxe5]->load(path + "iron-axe5.cub", database, true);
    m_sprites[SpriteIronAxe1Random1]->load(path + "iron-axe1-random1.cub", database, true);
    m_sprites[SpriteIronAxe2Random1]->load(path + "iron-axe2-random1.cub", database, true);
    m_sprites[SpriteIronAxe3Random1]->load(path + "iron-axe3-random1.cub", database, true);
    m_sprites[SpriteIronAxe4Random1]->load(path + "iron-axe4-random1.cub", database, true);
    m_sprites[SpriteIronAxe5Random1]->load(path + "iron-axe5-random1.cub", database, true);
    m_sprites[SpriteIronAxe1Random2]->load(path + "iron-axe1-random2.cub", database, true);
    m_sprites[SpriteIronAxe2Random2]->load(path + "iron-axe2-random2.cub", database, true);
    m_sprites[SpriteIronAxe3Random2]->load(path + "iron-axe3-random2.cub", database, true);
    m_sprites[SpriteIronAxe4Random2]->load(path + "iron-axe4-random2.cub", database, true);
    m_sprites[SpriteIronAxe5Random2]->load(path + "iron-axe5-random2.cub", database, true);
    m_sprites[SpriteIronAxe1Random3]->load(path + "iron-axe1-random3.cub", database, true);
    m_sprites[SpriteIronAxe2Random3]->load(path + "iron-axe2-random3.cub", database, true);
    m_sprites[SpriteIronAxe3Random3]->load(path + "iron-axe3-random3.cub", database, true);
    m_sprites[SpriteIronAxe4Random3]->load(path + "iron-axe4-random3.cub", database, true);
    m_sprites[SpriteIronAxe5Random3]->load(path + "iron-axe5-random3.cub", database, true);
    m_sprites[SpriteIronAxe1Random4]->load(path + "iron-axe1-random4.cub", database, true);
    m_sprites[SpriteIronAxe2Random4]->load(path + "iron-axe2-random4.cub", database, true);
    m_sprites[SpriteIronAxe3Random4]->load(path + "iron-axe3-random4.cub", database, true);
    m_sprites[SpriteIronAxe4Random4]->load(path + "iron-axe4-random4.cub", database, true);
    m_sprites[SpriteIronAxe5Random4]->load(path + "iron-axe5-random4.cub", database, true);
    m_sprites[SpriteIronAxe1Random5]->load(path + "iron-axe1-random5.cub", database, true);
    m_sprites[SpriteIronAxe2Random5]->load(path + "iron-axe2-random5.cub", database, true);
    m_sprites[SpriteIronAxe3Random5]->load(path + "iron-axe3-random5.cub", database, true);
    m_sprites[SpriteIronAxe4Random5]->load(path + "iron-axe4-random5.cub", database, true);
    m_sprites[SpriteIronAxe5Random5]->load(path + "iron-axe5-random5.cub", database, true);
    m_sprites[SpriteIronAxe1Random6]->load(path + "iron-axe1-random6.cub", database, true);
    m_sprites[SpriteIronAxe2Random6]->load(path + "iron-axe2-random6.cub", database, true);
    m_sprites[SpriteIronAxe3Random6]->load(path + "iron-axe3-random6.cub", database, true);
    m_sprites[SpriteIronAxe4Random6]->load(path + "iron-axe4-random6.cub", database, true);
    m_sprites[SpriteIronAxe5Random6]->load(path + "iron-axe5-random6.cub", database, true);
    m_sprites[SpriteIronAxe1Random7]->load(path + "iron-axe1-random7.cub", database, true);
    m_sprites[SpriteIronAxe2Random7]->load(path + "iron-axe2-random7.cub", database, true);
    m_sprites[SpriteIronAxe3Random7]->load(path + "iron-axe3-random7.cub", database, true);
    m_sprites[SpriteIronAxe4Random7]->load(path + "iron-axe4-random7.cub", database, true);
    m_sprites[SpriteIronAxe5Random7]->load(path + "iron-axe5-random7.cub", database, true);
    m_sprites[SpriteIronAxe1Random8]->load(path + "iron-axe1-random8.cub", database, true);
    m_sprites[SpriteIronAxe2Random8]->load(path + "iron-axe2-random8.cub", database, true);
    m_sprites[SpriteIronAxe3Random8]->load(path + "iron-axe3-random8.cub", database, true);
    m_sprites[SpriteIronAxe4Random8]->load(path + "iron-axe4-random8.cub", database, true);
    m_sprites[SpriteIronAxe5Random8]->load(path + "iron-axe5-random8.cub", database, true);
    m_sprites[SpriteIronAxe1Random9]->load(path + "iron-axe1-random9.cub", database, true);
    m_sprites[SpriteIronAxe2Random9]->load(path + "iron-axe2-random9.cub", database, true);
    m_sprites[SpriteIronAxe3Random9]->load(path + "iron-axe3-random9.cub", database, true);
    m_sprites[SpriteIronAxe4Random9]->load(path + "iron-axe4-random9.cub", database, true);
    m_sprites[SpriteIronAxe5Random9]->load(path + "iron-axe5-random9.cub", database, true);
    m_sprites[SpriteIronAxe1Random10]->load(path + "iron-axe1-random10.cub", database, true);
    m_sprites[SpriteIronAxe2Random10]->load(path + "iron-axe2-random10.cub", database, true);
    m_sprites[SpriteIronAxe3Random10]->load(path + "iron-axe3-random10.cub", database, true);
    m_sprites[SpriteIronAxe4Random10]->load(path + "iron-axe4-random10.cub", database, true);
    m_sprites[SpriteIronAxe5Random10]->load(path + "iron-axe5-random10.cub", database, true);
    m_sprites[SpriteGoldCoin]->load(path + "gold-coin.cub", database, true);
    m_sprites[SpriteCopperCoin]->load(path + "copper-coin.cub", database, true);
    m_sprites[SpriteSilverCoin]->load(path + "silver-coin.cub", database, true);
    m_sprites[SpriteIronGreatsword1]->load(path + "iron-greatsword1.cub", database, true);
    m_sprites[SpriteIronGreatsword2]->load(path + "iron-greatsword2.cub", database, true);
    m_sprites[SpriteIronGreatsword3]->load(path + "iron-greatsword3.cub", database, true);
    m_sprites[SpriteIronGreatsword4]->load(path + "iron-greatsword4.cub", database, true);
    m_sprites[SpriteIronGreatsword5]->load(path + "iron-greatsword5.cub", database, true);
    m_sprites[SpriteIronGreatsword1Random1]->load(path + "iron-greatsword1-random1.cub", database, true);
    m_sprites[SpriteIronGreatsword2Random1]->load(path + "iron-greatsword2-random1.cub", database, true);
    m_sprites[SpriteIronGreatsword3Random1]->load(path + "iron-greatsword3-random1.cub", database, true);
    m_sprites[SpriteIronGreatsword4Random1]->load(path + "iron-greatsword4-random1.cub", database, true);
    m_sprites[SpriteIronGreatsword5Random1]->load(path + "iron-greatsword5-random1.cub", database, true);
    m_sprites[SpriteIronGreatsword1Random2]->load(path + "iron-greatsword1-random2.cub", database, true);
    m_sprites[SpriteIronGreatsword2Random2]->load(path + "iron-greatsword2-random2.cub", database, true);
    m_sprites[SpriteIronGreatsword3Random2]->load(path + "iron-greatsword3-random2.cub", database, true);
    m_sprites[SpriteIronGreatsword4Random2]->load(path + "iron-greatsword4-random2.cub", database, true);
    m_sprites[SpriteIronGreatsword5Random2]->load(path + "iron-greatsword5-random2.cub", database, true);
    m_sprites[SpriteIronGreatsword1Random3]->load(path + "iron-greatsword1-random3.cub", database, true);
    m_sprites[SpriteIronGreatsword2Random3]->load(path + "iron-greatsword2-random3.cub", database, true);
    m_sprites[SpriteIronGreatsword3Random3]->load(path + "iron-greatsword3-random3.cub", database, true);
    m_sprites[SpriteIronGreatsword4Random3]->load(path + "iron-greatsword4-random3.cub", database, true);
    m_sprites[SpriteIronGreatsword5Random3]->load(path + "iron-greatsword5-random3.cub", database, true);
    m_sprites[SpriteIronGreatsword1Random4]->load(path + "iron-greatsword1-random4.cub", database, true);
    m_sprites[SpriteIronGreatsword2Random4]->load(path + "iron-greatsword2-random4.cub", database, true);
    m_sprites[SpriteIronGreatsword3Random4]->load(path + "iron-greatsword3-random4.cub", database, true);
    m_sprites[SpriteIronGreatsword4Random4]->load(path + "iron-greatsword4-random4.cub", database, true);
    m_sprites[SpriteIronGreatsword5Random4]->load(path + "iron-greatsword5-random4.cub", database, true);
    m_sprites[SpriteIronGreatsword1Random5]->load(path + "iron-greatsword1-random5.cub", database, true);
    m_sprites[SpriteIronGreatsword2Random5]->load(path + "iron-greatsword2-random5.cub", database, true);
    m_sprites[SpriteIronGreatsword3Random5]->load(path + "iron-greatsword3-random5.cub", database, true);
    m_sprites[SpriteIronGreatsword4Random5]->load(path + "iron-greatsword4-random5.cub", database, true);
    m_sprites[SpriteIronGreatsword5Random5]->load(path + "iron-greatsword5-random5.cub", database, true);
    m_sprites[SpriteIronGreatsword1Random6]->load(path + "iron-greatsword1-random6.cub", database, true);
    m_sprites[SpriteIronGreatsword2Random6]->load(path + "iron-greatsword2-random6.cub", database, true);
    m_sprites[SpriteIronGreatsword3Random6]->load(path + "iron-greatsword3-random6.cub", database, true);
    m_sprites[SpriteIronGreatsword4Random6]->load(path + "iron-greatsword4-random6.cub", database, true);
    m_sprites[SpriteIronGreatsword5Random6]->load(path + "iron-greatsword5-random6.cub", database, true);
    m_sprites[SpriteIronGreatsword1Random7]->load(path + "iron-greatsword1-random7.cub", database, true);
    m_sprites[SpriteIronGreatsword2Random7]->load(path + "iron-greatsword2-random7.cub", database, true);
    m_sprites[SpriteIronGreatsword3Random7]->load(path + "iron-greatsword3-random7.cub", database, true);
    m_sprites[SpriteIronGreatsword4Random7]->load(path + "iron-greatsword4-random7.cub", database, true);
    m_sprites[SpriteIronGreatsword5Random7]->load(path + "iron-greatsword5-random7.cub", database, true);
    m_sprites[SpriteIronGreatsword1Random8]->load(path + "iron-greatsword1-random8.cub", database, true);
    m_sprites[SpriteIronGreatsword2Random8]->load(path + "iron-greatsword2-random8.cub", database, true);
    m_sprites[SpriteIronGreatsword3Random8]->load(path + "iron-greatsword3-random8.cub", database, true);
    m_sprites[SpriteIronGreatsword4Random8]->load(path + "iron-greatsword4-random8.cub", database, true);
    m_sprites[SpriteIronGreatsword5Random8]->load(path + "iron-greatsword5-random8.cub", database, true);
    m_sprites[SpriteIronGreatsword1Random9]->load(path + "iron-greatsword1-random9.cub", database, true);
    m_sprites[SpriteIronGreatsword2Random9]->load(path + "iron-greatsword2-random9.cub", database, true);
    m_sprites[SpriteIronGreatsword3Random9]->load(path + "iron-greatsword3-random9.cub", database, true);
    m_sprites[SpriteIronGreatsword4Random9]->load(path + "iron-greatsword4-random9.cub", database, true);
    m_sprites[SpriteIronGreatsword5Random9]->load(path + "iron-greatsword5-random9.cub", database, true);
    m_sprites[SpriteIronGreatsword1Random10]->load(path + "iron-greatsword1-random10.cub", database, true);
    m_sprites[SpriteIronGreatsword2Random10]->load(path + "iron-greatsword2-random10.cub", database, true);
    m_sprites[SpriteIronGreatsword3Random10]->load(path + "iron-greatsword3-random10.cub", database, true);
    m_sprites[SpriteIronGreatsword4Random10]->load(path + "iron-greatsword4-random10.cub", database, true);
    m_sprites[SpriteIronGreatsword5Random10]->load(path + "iron-greatsword5-random10.cub", database, true);
    m_sprites[SpriteIronLongsword1]->load(path + "iron-longsword1.cub", database, true);
    m_sprites[SpriteIronLongsword2]->load(path + "iron-longsword2.cub", database, true);
    m_sprites[SpriteIronLongsword3]->load(path + "iron-longsword3.cub", database, true);
    m_sprites[SpriteIronLongsword4]->load(path + "iron-longsword4.cub", database, true);
    m_sprites[SpriteIronLongsword5]->load(path + "iron-longsword5.cub", database, true);
    m_sprites[SpriteIronLongsword1Random1]->load(path + "iron-longsword1-random1.cub", database, true);
    m_sprites[SpriteIronLongsword2Random1]->load(path + "iron-longsword2-random1.cub", database, true);
    m_sprites[SpriteIronLongsword3Random1]->load(path + "iron-longsword3-random1.cub", database, true);
    m_sprites[SpriteIronLongsword4Random1]->load(path + "iron-longsword4-random1.cub", database, true);
    m_sprites[SpriteIronLongsword5Random1]->load(path + "iron-longsword5-random1.cub", database, true);
    m_sprites[SpriteIronLongsword1Random2]->load(path + "iron-longsword1-random2.cub", database, true);
    m_sprites[SpriteIronLongsword2Random2]->load(path + "iron-longsword2-random2.cub", database, true);
    m_sprites[SpriteIronLongsword3Random2]->load(path + "iron-longsword3-random2.cub", database, true);
    m_sprites[SpriteIronLongsword4Random2]->load(path + "iron-longsword4-random2.cub", database, true);
    m_sprites[SpriteIronLongsword5Random2]->load(path + "iron-longsword5-random2.cub", database, true);
    m_sprites[SpriteIronLongsword1Random3]->load(path + "iron-longsword1-random3.cub", database, true);
    m_sprites[SpriteIronLongsword2Random3]->load(path + "iron-longsword2-random3.cub", database, true);
    m_sprites[SpriteIronLongsword3Random3]->load(path + "iron-longsword3-random3.cub", database, true);
    m_sprites[SpriteIronLongsword4Random3]->load(path + "iron-longsword4-random3.cub", database, true);
    m_sprites[SpriteIronLongsword5Random3]->load(path + "iron-longsword5-random3.cub", database, true);
    m_sprites[SpriteIronLongsword1Random4]->load(path + "iron-longsword1-random4.cub", database, true);
    m_sprites[SpriteIronLongsword2Random4]->load(path + "iron-longsword2-random4.cub", database, true);
    m_sprites[SpriteIronLongsword3Random4]->load(path + "iron-longsword3-random4.cub", database, true);
    m_sprites[SpriteIronLongsword4Random4]->load(path + "iron-longsword4-random4.cub", database, true);
    m_sprites[SpriteIronLongsword5Random4]->load(path + "iron-longsword5-random4.cub", database, true);
    m_sprites[SpriteIronLongsword1Random5]->load(path + "iron-longsword1-random5.cub", database, true);
    m_sprites[SpriteIronLongsword2Random5]->load(path + "iron-longsword2-random5.cub", database, true);
    m_sprites[SpriteIronLongsword3Random5]->load(path + "iron-longsword3-random5.cub", database, true);
    m_sprites[SpriteIronLongsword4Random5]->load(path + "iron-longsword4-random5.cub", database, true);
    m_sprites[SpriteIronLongsword5Random5]->load(path + "iron-longsword5-random5.cub", database, true);
    m_sprites[SpriteIronLongsword1Random6]->load(path + "iron-longsword1-random6.cub", database, true);
    m_sprites[SpriteIronLongsword2Random6]->load(path + "iron-longsword2-random6.cub", database, true);
    m_sprites[SpriteIronLongsword3Random6]->load(path + "iron-longsword3-random6.cub", database, true);
    m_sprites[SpriteIronLongsword4Random6]->load(path + "iron-longsword4-random6.cub", database, true);
    m_sprites[SpriteIronLongsword5Random6]->load(path + "iron-longsword5-random6.cub", database, true);
    m_sprites[SpriteIronLongsword1Random7]->load(path + "iron-longsword1-random7.cub", database, true);
    m_sprites[SpriteIronLongsword2Random7]->load(path + "iron-longsword2-random7.cub", database, true);
    m_sprites[SpriteIronLongsword3Random7]->load(path + "iron-longsword3-random7.cub", database, true);
    m_sprites[SpriteIronLongsword4Random7]->load(path + "iron-longsword4-random7.cub", database, true);
    m_sprites[SpriteIronLongsword5Random7]->load(path + "iron-longsword5-random7.cub", database, true);
    m_sprites[SpriteIronLongsword1Random8]->load(path + "iron-longsword1-random8.cub", database, true);
    m_sprites[SpriteIronLongsword2Random8]->load(path + "iron-longsword2-random8.cub", database, true);
    m_sprites[SpriteIronLongsword3Random8]->load(path + "iron-longsword3-random8.cub", database, true);
    m_sprites[SpriteIronLongsword4Random8]->load(path + "iron-longsword4-random8.cub", database, true);
    m_sprites[SpriteIronLongsword5Random8]->load(path + "iron-longsword5-random8.cub", database, true);
    m_sprites[SpriteIronLongsword1Random9]->load(path + "iron-longsword1-random9.cub", database, true);
    m_sprites[SpriteIronLongsword2Random9]->load(path + "iron-longsword2-random9.cub", database, true);
    m_sprites[SpriteIronLongsword3Random9]->load(path + "iron-longsword3-random9.cub", database, true);
    m_sprites[SpriteIronLongsword4Random9]->load(path + "iron-longsword4-random9.cub", database, true);
    m_sprites[SpriteIronLongsword5Random9]->load(path + "iron-longsword5-random9.cub", database, true);
    m_sprites[SpriteIronLongsword1Random10]->load(path + "iron-longsword1-random10.cub", database, true);
    m_sprites[SpriteIronLongsword2Random10]->load(path + "iron-longsword2-random10.cub", database, true);
    m_sprites[SpriteIronLongsword3Random10]->load(path + "iron-longsword3-random10.cub", database, true);
    m_sprites[SpriteIronLongsword4Random10]->load(path + "iron-longsword4-random10.cub", database, true);
    m_sprites[SpriteIronLongsword5Random10]->load(path + "iron-longsword5-random10.cub", database, true);
    m_sprites[SpriteBoneGreatsword]->load(path + "bone-greatsword.cub", database, true);
    m_sprites[SpriteObsidianGreatsword]->load(path + "obsidian-greatsword.cub", database, true);
    m_sprites[SpriteIronGreataxe1]->load(path + "iron-greataxe1.cub", database, true);
    m_sprites[SpriteIronGreataxe2]->load(path + "iron-greataxe2.cub", database, true);
    m_sprites[SpriteIronGreataxe3]->load(path + "iron-greataxe3.cub", database, true);
    m_sprites[SpriteIronGreataxe4]->load(path + "iron-greataxe4.cub", database, true);
    m_sprites[SpriteIronGreataxe5]->load(path + "iron-greataxe5.cub", database, true);
    m_sprites[SpriteIronGreataxe1Random1]->load(path + "iron-greataxe1-random1.cub", database, true);
    m_sprites[SpriteIronGreataxe2Random1]->load(path + "iron-greataxe2-random1.cub", database, true);
    m_sprites[SpriteIronGreataxe3Random1]->load(path + "iron-greataxe3-random1.cub", database, true);
    m_sprites[SpriteIronGreataxe4Random1]->load(path + "iron-greataxe4-random1.cub", database, true);
    m_sprites[SpriteIronGreataxe5Random1]->load(path + "iron-greataxe5-random1.cub", database, true);
    m_sprites[SpriteIronGreataxe1Random2]->load(path + "iron-greataxe1-random2.cub", database, true);
    m_sprites[SpriteIronGreataxe2Random2]->load(path + "iron-greataxe2-random2.cub", database, true);
    m_sprites[SpriteIronGreataxe3Random2]->load(path + "iron-greataxe3-random2.cub", database, true);
    m_sprites[SpriteIronGreataxe4Random2]->load(path + "iron-greataxe4-random2.cub", database, true);
    m_sprites[SpriteIronGreataxe5Random2]->load(path + "iron-greataxe5-random2.cub", database, true);
    m_sprites[SpriteIronGreataxe1Random3]->load(path + "iron-greataxe1-random3.cub", database, true);
    m_sprites[SpriteIronGreataxe2Random3]->load(path + "iron-greataxe2-random3.cub", database, true);
    m_sprites[SpriteIronGreataxe3Random3]->load(path + "iron-greataxe3-random3.cub", database, true);
    m_sprites[SpriteIronGreataxe4Random3]->load(path + "iron-greataxe4-random3.cub", database, true);
    m_sprites[SpriteIronGreataxe5Random3]->load(path + "iron-greataxe5-random3.cub", database, true);
    m_sprites[SpriteIronGreataxe1Random4]->load(path + "iron-greataxe1-random4.cub", database, true);
    m_sprites[SpriteIronGreataxe2Random4]->load(path + "iron-greataxe2-random4.cub", database, true);
    m_sprites[SpriteIronGreataxe3Random4]->load(path + "iron-greataxe3-random4.cub", database, true);
    m_sprites[SpriteIronGreataxe4Random4]->load(path + "iron-greataxe4-random4.cub", database, true);
    m_sprites[SpriteIronGreataxe5Random4]->load(path + "iron-greataxe5-random4.cub", database, true);
    m_sprites[SpriteIronGreataxe1Random5]->load(path + "iron-greataxe1-random5.cub", database, true);
    m_sprites[SpriteIronGreataxe2Random5]->load(path + "iron-greataxe2-random5.cub", database, true);
    m_sprites[SpriteIronGreataxe3Random5]->load(path + "iron-greataxe3-random5.cub", database, true);
    m_sprites[SpriteIronGreataxe4Random5]->load(path + "iron-greataxe4-random5.cub", database, true);
    m_sprites[SpriteIronGreataxe5Random5]->load(path + "iron-greataxe5-random5.cub", database, true);
    m_sprites[SpriteIronGreataxe1Random6]->load(path + "iron-greataxe1-random6.cub", database, true);
    m_sprites[SpriteIronGreataxe2Random6]->load(path + "iron-greataxe2-random6.cub", database, true);
    m_sprites[SpriteIronGreataxe3Random6]->load(path + "iron-greataxe3-random6.cub", database, true);
    m_sprites[SpriteIronGreataxe4Random6]->load(path + "iron-greataxe4-random6.cub", database, true);
    m_sprites[SpriteIronGreataxe5Random6]->load(path + "iron-greataxe5-random6.cub", database, true);
    m_sprites[SpriteIronGreataxe1Random7]->load(path + "iron-greataxe1-random7.cub", database, true);
    m_sprites[SpriteIronGreataxe2Random7]->load(path + "iron-greataxe2-random7.cub", database, true);
    m_sprites[SpriteIronGreataxe3Random7]->load(path + "iron-greataxe3-random7.cub", database, true);
    m_sprites[SpriteIronGreataxe4Random7]->load(path + "iron-greataxe4-random7.cub", database, true);
    m_sprites[SpriteIronGreataxe5Random7]->load(path + "iron-greataxe5-random7.cub", database, true);
    m_sprites[SpriteIronGreataxe1Random8]->load(path + "iron-greataxe1-random8.cub", database, true);
    m_sprites[SpriteIronGreataxe2Random8]->load(path + "iron-greataxe2-random8.cub", database, true);
    m_sprites[SpriteIronGreataxe3Random8]->load(path + "iron-greataxe3-random8.cub", database, true);
    m_sprites[SpriteIronGreataxe4Random8]->load(path + "iron-greataxe4-random8.cub", database, true);
    m_sprites[SpriteIronGreataxe5Random8]->load(path + "iron-greataxe5-random8.cub", database, true);
    m_sprites[SpriteIronGreataxe1Random9]->load(path + "iron-greataxe1-random9.cub", database, true);
    m_sprites[SpriteIronGreataxe2Random9]->load(path + "iron-greataxe2-random9.cub", database, true);
    m_sprites[SpriteIronGreataxe3Random9]->load(path + "iron-greataxe3-random9.cub", database, true);
    m_sprites[SpriteIronGreataxe4Random9]->load(path + "iron-greataxe4-random9.cub", database, true);
    m_sprites[SpriteIronGreataxe5Random9]->load(path + "iron-greataxe5-random9.cub", database, true);
    m_sprites[SpriteIronGreataxe1Random10]->load(path + "iron-greataxe1-random10.cub", database, true);
    m_sprites[SpriteIronGreataxe2Random10]->load(path + "iron-greataxe2-random10.cub", database, true);
    m_sprites[SpriteIronGreataxe3Random10]->load(path + "iron-greataxe3-random10.cub", database, true);
    m_sprites[SpriteIronGreataxe4Random10]->load(path + "iron-greataxe4-random10.cub", database, true);
    m_sprites[SpriteIronGreataxe5Random10]->load(path + "iron-greataxe5-random10.cub", database, true);
    m_sprites[SpriteBoneGreataxe]->load(path + "bone-greataxe.cub", database, true);
    m_sprites[SpriteObsidianGreataxe]->load(path + "obsidian-greataxe.cub", database, true);
    m_sprites[SpriteSaurianGreataxe]->load(path + "saurian-greataxe.cub", database, true);
    m_sprites[SpriteIronGreatmace1]->load(path + "iron-greatmace1.cub", database, true);
    m_sprites[SpriteIronGreatmace2]->load(path + "iron-greatmace2.cub", database, true);
    m_sprites[SpriteIronGreatmace3]->load(path + "iron-greatmace3.cub", database, true);
    m_sprites[SpriteIronGreatmace4]->load(path + "iron-greatmace4.cub", database, true);
    m_sprites[SpriteIronGreatmace5]->load(path + "iron-greatmace5.cub", database, true);
    m_sprites[SpriteIronGreatmace1Random1]->load(path + "iron-greatmace1-random1.cub", database, true);
    m_sprites[SpriteIronGreatmace2Random1]->load(path + "iron-greatmace2-random1.cub", database, true);
    m_sprites[SpriteIronGreatmace3Random1]->load(path + "iron-greatmace3-random1.cub", database, true);
    m_sprites[SpriteIronGreatmace4Random1]->load(path + "iron-greatmace4-random1.cub", database, true);
    m_sprites[SpriteIronGreatmace5Random1]->load(path + "iron-greatmace5-random1.cub", database, true);
    m_sprites[SpriteIronGreatmace1Random2]->load(path + "iron-greatmace1-random2.cub", database, true);
    m_sprites[SpriteIronGreatmace2Random2]->load(path + "iron-greatmace2-random2.cub", database, true);
    m_sprites[SpriteIronGreatmace3Random2]->load(path + "iron-greatmace3-random2.cub", database, true);
    m_sprites[SpriteIronGreatmace4Random2]->load(path + "iron-greatmace4-random2.cub", database, true);
    m_sprites[SpriteIronGreatmace5Random2]->load(path + "iron-greatmace5-random2.cub", database, true);
    m_sprites[SpriteIronGreatmace1Random3]->load(path + "iron-greatmace1-random3.cub", database, true);
    m_sprites[SpriteIronGreatmace2Random3]->load(path + "iron-greatmace2-random3.cub", database, true);
    m_sprites[SpriteIronGreatmace3Random3]->load(path + "iron-greatmace3-random3.cub", database, true);
    m_sprites[SpriteIronGreatmace4Random3]->load(path + "iron-greatmace4-random3.cub", database, true);
    m_sprites[SpriteIronGreatmace5Random3]->load(path + "iron-greatmace5-random3.cub", database, true);
    m_sprites[SpriteIronGreatmace1Random4]->load(path + "iron-greatmace1-random4.cub", database, true);
    m_sprites[SpriteIronGreatmace2Random4]->load(path + "iron-greatmace2-random4.cub", database, true);
    m_sprites[SpriteIronGreatmace3Random4]->load(path + "iron-greatmace3-random4.cub", database, true);
    m_sprites[SpriteIronGreatmace4Random4]->load(path + "iron-greatmace4-random4.cub", database, true);
    m_sprites[SpriteIronGreatmace5Random4]->load(path + "iron-greatmace5-random4.cub", database, true);
    m_sprites[SpriteIronGreatmace1Random5]->load(path + "iron-greatmace1-random5.cub", database, true);
    m_sprites[SpriteIronGreatmace2Random5]->load(path + "iron-greatmace2-random5.cub", database, true);
    m_sprites[SpriteIronGreatmace3Random5]->load(path + "iron-greatmace3-random5.cub", database, true);
    m_sprites[SpriteIronGreatmace4Random5]->load(path + "iron-greatmace4-random5.cub", database, true);
    m_sprites[SpriteIronGreatmace5Random5]->load(path + "iron-greatmace5-random5.cub", database, true);
    m_sprites[SpriteIronGreatmace1Random6]->load(path + "iron-greatmace1-random6.cub", database, true);
    m_sprites[SpriteIronGreatmace2Random6]->load(path + "iron-greatmace2-random6.cub", database, true);
    m_sprites[SpriteIronGreatmace3Random6]->load(path + "iron-greatmace3-random6.cub", database, true);
    m_sprites[SpriteIronGreatmace4Random6]->load(path + "iron-greatmace4-random6.cub", database, true);
    m_sprites[SpriteIronGreatmace5Random6]->load(path + "iron-greatmace5-random6.cub", database, true);
    m_sprites[SpriteIronGreatmace1Random7]->load(path + "iron-greatmace1-random7.cub", database, true);
    m_sprites[SpriteIronGreatmace2Random7]->load(path + "iron-greatmace2-random7.cub", database, true);
    m_sprites[SpriteIronGreatmace3Random7]->load(path + "iron-greatmace3-random7.cub", database, true);
    m_sprites[SpriteIronGreatmace4Random7]->load(path + "iron-greatmace4-random7.cub", database, true);
    m_sprites[SpriteIronGreatmace5Random7]->load(path + "iron-greatmace5-random7.cub", database, true);
    m_sprites[SpriteIronGreatmace1Random8]->load(path + "iron-greatmace1-random8.cub", database, true);
    m_sprites[SpriteIronGreatmace2Random8]->load(path + "iron-greatmace2-random8.cub", database, true);
    m_sprites[SpriteIronGreatmace3Random8]->load(path + "iron-greatmace3-random8.cub", database, true);
    m_sprites[SpriteIronGreatmace4Random8]->load(path + "iron-greatmace4-random8.cub", database, true);
    m_sprites[SpriteIronGreatmace5Random8]->load(path + "iron-greatmace5-random8.cub", database, true);
    m_sprites[SpriteIronGreatmace1Random9]->load(path + "iron-greatmace1-random9.cub", database, true);
    m_sprites[SpriteIronGreatmace2Random9]->load(path + "iron-greatmace2-random9.cub", database, true);
    m_sprites[SpriteIronGreatmace3Random9]->load(path + "iron-greatmace3-random9.cub", database, true);
    m_sprites[SpriteIronGreatmace4Random9]->load(path + "iron-greatmace4-random9.cub", database, true);
    m_sprites[SpriteIronGreatmace5Random9]->load(path + "iron-greatmace5-random9.cub", database, true);
    m_sprites[SpriteIronGreatmace1Random10]->load(path + "iron-greatmace1-random10.cub", database, true);
    m_sprites[SpriteIronGreatmace2Random10]->load(path + "iron-greatmace2-random10.cub", database, true);
    m_sprites[SpriteIronGreatmace3Random10]->load(path + "iron-greatmace3-random10.cub", database, true);
    m_sprites[SpriteIronGreatmace4Random10]->load(path + "iron-greatmace4-random10.cub", database, true);
    m_sprites[SpriteIronGreatmace5Random10]->load(path + "iron-greatmace5-random10.cub", database, true);
    m_sprites[SpriteBoneGreatmace]->load(path + "bone-greatmace.cub", database, true);
    m_sprites[SpriteObsidianGreatmace]->load(path + "obsidian-greatmace.cub", database, true);
    m_sprites[SpriteWoodGreatmace02]->load(path + "wood-greatmace02.cub", database, true);
    m_sprites[SpriteBush]->load(path + "bush.cub", database, true);
    m_sprites[SpriteSnowBush]->load(path + "snow-bush.cub", database, true);
    m_sprites[SpriteCobwebscrub]->load(path + "cobwebscrub.cub", database, true);
    m_sprites[SpriteBerryBush]->load(path + "berry-bush.cub", database, true);
    m_sprites[SpriteSnowBerry]->load(path + "snow-berry.cub", database, true);
    m_sprites[SpriteSnowBerryMash]->load(path + "snow-berry-mash.cub", database, true);
    m_sprites[SpriteCottonPlant]->load(path + "cotton-plant.cub", database, true);
    m_sprites[SpriteCotton]->load(path + "cotton.cub", database, true);
    m_sprites[SpriteScrub]->load(path + "scrub.cub", database, true);
    m_sprites[SpriteScrubGreen]->load(path + "scrub-green.cub", database, true);
    m_sprites[SpriteFireScrub]->load(path + "fire-scrub.cub", database, true);
    m_sprites[SpriteFirTree]->load(path + "fir-tree.cub", database, true);
    m_sprites[SpriteThornTree]->load(path + "thorn-tree.cub", database, true);
    m_sprites[SpriteTreeLeaves]->load(path + "tree-leaves.cub", database, true);
    m_sprites[SpriteGoldDeposit]->load(path + "gold-deposit.cub", database, true);
    m_sprites[SpriteIronDeposit]->load(path + "iron-deposit.cub", database, true);
    m_sprites[SpriteSilverDeposit]->load(path + "silver-deposit.cub", database, true);
    m_sprites[SpriteSandstoneDeposit]->load(path + "sandstone-deposit.cub", database, true);
    m_sprites[SpriteEmeraldDeposit]->load(path + "emerald-deposit.cub", database, true);
    m_sprites[SpriteRubyDeposit]->load(path + "ruby-deposit.cub", database, true);
    m_sprites[SpriteDiamondDeposit]->load(path + "diamond-deposit.cub", database, true);
    m_sprites[SpriteSapphireDeposit]->load(path + "sapphire-deposit.cub", database, true);
    m_sprites[SpriteIceCrystalDeposit]->load(path + "ice-crystal-deposit.cub", database, true);
    m_sprites[SpriteScarecrow]->load(path + "scarecrow.cub", database, true);
    m_sprites[SpriteDummy]->load(path + "dummy.cub", database, true);
    m_sprites[SpriteAim]->load(path + "aim.cub", database, true);
    m_sprites[SpriteVase]->load(path + "vase.cub", database, true);
    m_sprites[SpriteVase2]->load(path + "vase2.cub", database, true);
    m_sprites[SpriteVase3]->load(path + "vase3.cub", database, true);
    m_sprites[SpriteVase4]->load(path + "vase4.cub", database, true);
    m_sprites[SpriteCampfire]->load(path + "campfire.cub", database, true);
    m_sprites[SpriteTent]->load(path + "tent.cub", database, true);
    m_sprites[SpriteBeachUmbrella]->load(path + "beach-umbrella.cub", database, true);
    m_sprites[SpriteBeachTowel]->load(path + "beach-towel.cub", database, true);
    m_sprites[SpriteWoodMat]->load(path + "wood-mat.cub", database, true);
    m_sprites[SpriteCandle01]->load(path + "candle01.cub", database, true);
    m_sprites[SpriteCandle02]->load(path + "candle02.cub", database, true);
    m_sprites[SpriteCandle03]->load(path + "candle03.cub", database, true);
    m_sprites[SpriteCompass2]->load(path + "compass2.cub", database, true);
    m_sprites[SpriteMission]->load(path + "mission.cub", database, true);
    m_sprites[SpriteCity]->load(path + "city.cub", database, true);
    m_sprites[SpriteSkull]->load(path + "skull.cub", database, true);
    m_sprites[SpriteUndeadCandle1]->load(path + "undead-candle1.cub", database, true);
    m_sprites[SpriteUndeadCandle2]->load(path + "undead-candle2.cub", database, true);
    m_sprites[SpriteUndeadCandle3]->load(path + "undead-candle3.cub", database, true);
    m_sprites[SpriteCharacterPlatform]->load(path + "character-platform.cub", database, true);
    m_sprites[SpriteAntiqueBuilding1]->load(path + "antique-building1.cub", database, true);
    m_sprites[SpriteAntiqueBuilding2]->load(path + "antique-building2.cub", database, true);
    m_sprites[SpriteAntiqueBuilding3]->load(path + "antique-building3.cub", database, true);
    m_sprites[SpriteAntiqueBuilding4]->load(path + "antique-building4.cub", database, true);
    m_sprites[SpriteEntranceCrypt]->load(path + "entrance-crypt.cub", database, true);
    m_sprites[SpriteEntranceBarrow]->load(path + "entrance-barrow.cub", database, true);
    m_sprites[SpriteEntranceMine]->load(path + "entrance-mine.cub", database, true);
    m_sprites[SpriteEntranceAntique]->load(path + "entrance-antique.cub", database, true);
    m_sprites[SpriteEntranceTomb]->load(path + "entrance-tomb.cub", database, true);
    m_sprites[SpriteVampireHead]->load(path + "vampire-head.cub", database, true);
    m_sprites[SpriteVampireHand]->load(path + "vampire-hand.cub", database, true);
    m_sprites[SpriteVampireEyes]->load(path + "vampire-eyes.cub", database, false);
    m_sprites[SpriteMonsterBodyReptileShell]->load(path + "monster-body-reptile-shell.cub", database, true);
    m_sprites[SpriteMonsterBodyReptileShellSpiked]->load(path + "monster-body-reptile-shell-spiked.cub", database,
                                                         true);
    m_sprites[SpriteMonsterBodyReptileCrest1]->load(path + "monster-body-reptile-crest1.cub", database, true);
    m_sprites[SpriteMonsterBodyReptileCrest2]->load(path + "monster-body-reptile-crest2.cub", database, true);
    m_sprites[SpriteMonsterBodyReptileCrest3]->load(path + "monster-body-reptile-crest3.cub", database, true);
    m_sprites[SpriteMonsterBodyReptileCrest4]->load(path + "monster-body-reptile-crest4.cub", database, true);
    m_sprites[SpriteMonsterBodyReptileCrest5]->load(path + "monster-body-reptile-crest5.cub", database, true);
    m_sprites[SpriteMonsterBodyReptileSpiked1]->load(path + "monster-body-reptile-spiked1.cub", database, true);
    m_sprites[SpriteMonsterBodyReptileSpiked2]->load(path + "monster-body-reptile-spiked2.cub", database, true);
    m_sprites[SpriteMonsterFootReptileNormal]->load(path + "monster-foot-reptile-normal.cub", database, true);
    m_sprites[SpriteMonsterFootReptileClaw1]->load(path + "monster-foot-reptile-claw1.cub", database, true);
    m_sprites[SpriteMonsterFootReptileClaw2]->load(path + "monster-foot-reptile-claw2.cub", database, true);
    m_sprites[SpriteMonsterHeadReptileLizard]->load(path + "monster-head-reptile-lizard.cub", database, true);
    m_sprites[SpriteMonsterHeadReptileTurtle]->load(path + "monster-head-reptile-turtle.cub", database, true);
    m_sprites[SpriteMonsterHeadReptileHorn1]->load(path + "monster-head-reptile-horn1.cub", database, true);
    m_sprites[SpriteMonsterHeadReptileHorn2]->load(path + "monster-head-reptile-horn2.cub", database, true);
    m_sprites[SpriteMonsterHeadReptileHorn3]->load(path + "monster-head-reptile-horn3.cub", database, true);
    m_sprites[SpriteMonsterTailReptileNormal]->load(path + "monster-tail-reptile-normal.cub", database, true);
    m_sprites[SpriteMonsterTailReptileSpike1]->load(path + "monster-tail-reptile-spike1.cub", database, true);
    m_sprites[SpriteMonsterTailReptileSpike2]->load(path + "monster-tail-reptile-spike2.cub", database, true);
    m_sprites[SpriteRuneGiantFoot01]->load(path + "rune-giant-foot01.cub", database, true);
    m_sprites[SpriteRuneGiantHeadNormal01]->load(path + "rune-giant-head-normal01.cub", database, true);
    m_sprites[SpriteRuneGiantHeadLaser]->load(path + "rune-giant-head-laser.cub", database, true);
    m_sprites[SpriteRuneGiantBody01]->load(path + "rune-giant-body01.cub", database, true);
    m_sprites[SpriteRuneGiantBody02]->load(path + "rune-giant-body02.cub", database, true);
    m_sprites[SpriteRuneGiantHand01]->load(path + "rune-giant-hand01.cub", database, true);
    m_sprites[SpriteBuildingStiltInn1]->load(path + "building-stilt-inn1.cub", database, true);
    m_sprites[SpriteBuildingStiltHut2]->load(path + "building-stilt-hut2.cub", database, true);
    m_sprites[SpriteBuildingStiltHut3]->load(path + "building-stilt-hut3.cub", database, true);
    m_sprites[SpriteBuildingStiltPath]->load(path + "building-stilt-path.cub", database, true);
    m_sprites[SpriteFrameHouse01]->load(path + "frame-house01.cub", database, true);
    m_sprites[SpriteFrameInn]->load(path + "frame-inn.cub", database, true);
    m_sprites[SpriteFrameShop]->load(path + "frame-shop.cub", database, true);
    m_sprites[SpriteFrameTower]->load(path + "frame-tower.cub", database, true);
    m_sprites[SpriteBuildingStoneWell]->load(path + "building-stone-well.cub", database, true);
    m_sprites[SpriteLandingplace]->load(path + "landingplace.cub", database, true);
    m_sprites[SpriteBuildingWarrior]->load(path + "building-warrior.cub", database, true);
    m_sprites[SpriteBuildingRanger]->load(path + "building-ranger.cub", database, true);
    m_sprites[SpriteBuildingRogue]->load(path + "building-rogue.cub", database, true);
    m_sprites[SpriteBuildingMage]->load(path + "building-mage.cub", database, true);
    m_sprites[SpriteBuildingSmithy]->load(path + "building-smithy.cub", database, true);
    m_sprites[SpriteBuildingCarpentersshop]->load(path + "building-carpentersshop.cub", database, true);
    m_sprites[SpriteBuildingWeavingmill]->load(path + "building-weavingmill.cub", database, true);
    m_sprites[SpritePalmLeaf]->load(path + "palm-leaf.cub", database, true);
    m_sprites[SpritePalmLeafDiagonal]->load(path + "palm-leaf-diagonal.cub", database, true);
    m_sprites[SpriteWhiteCastleRoundTower]->load(path + "white-castle-round-tower.cub", database, true);
    m_sprites[SpritePainting01]->load(path + "painting01.cub", database, true);
    m_sprites[SpritePainting02]->load(path + "painting02.cub", database, true);
    m_sprites[SpriteDigestedLeftovers01]->load(path + "digested-leftovers01.cub", database, true);
    m_sprites[SpriteDigestedLeftovers02]->load(path + "digested-leftovers02.cub", database, true);
    m_sprites[SpriteDigestedLeftovers03]->load(path + "digested-leftovers03.cub", database, true);
    m_sprites[SpriteDigestedLeftovers04]->load(path + "digested-leftovers04.cub", database, true);
    m_sprites[SpriteIconTalk]->load(path + "icon-talk.cub", database, true);
    m_sprites[SpriteIconAnalyze]->load(path + "icon-analyze.cub", database, true);
    m_sprites[SpriteIconVendor]->load(path + "icon-vendor.cub", database, true);
    m_sprites[SpriteFlowerVase1]->load(path + "flower-vase1.cub", database, true);
    m_sprites[SpriteFlowerVase2]->load(path + "flower-vase2.cub", database, true);
    m_sprites[SpriteFlowerVase3]->load(path + "flower-vase3.cub", database, true);
    m_sprites[SpriteFlowerVase4]->load(path + "flower-vase4.cub", database, true);
    m_sprites[SpriteFlowerVase5]->load(path + "flower-vase5.cub", database, true);
    m_sprites[SpriteFlowerVase6]->load(path + "flower-vase6.cub", database, true);
    m_sprites[SpriteFlowerVase7]->load(path + "flower-vase7.cub", database, true);
    m_sprites[SpriteFlowerVase8]->load(path + "flower-vase8.cub", database, true);
    m_sprites[SpriteFlowerVase9]->load(path + "flower-vase9.cub", database, true);
    m_sprites[SpritePitchfork]->load(path + "pitchfork.cub", database, true);
    m_sprites[SpriteCactus1]->load(path + "cactus1.cub", database, true);
    m_sprites[SpriteCactus2]->load(path + "cactus2.cub", database, true);
    m_sprites[SpritePumpkin]->load(path + "pumpkin.cub", database, true);
    m_sprites[SpritePumpkinMuffin]->load(path + "pumpkin-muffin.cub", database, true);
    m_sprites[SpritePineapple]->load(path + "pineapple.cub", database, true);
    m_sprites[SpritePineappleSlice]->load(path + "pineapple-slice.cub", database, true);
    m_sprites[SpriteIronNugget]->load(path + "iron-nugget.cub", database, true);
    m_sprites[SpriteGoldNugget]->load(path + "gold-nugget.cub", database, true);
    m_sprites[SpriteSilverNugget]->load(path + "silver-nugget.cub", database, true);
    m_sprites[SpriteEmeraldNugget]->load(path + "emerald-nugget.cub", database, true);
    m_sprites[SpriteSapphireNugget]->load(path + "sapphire-nugget.cub", database, true);
    m_sprites[SpriteRubyNugget]->load(path + "ruby-nugget.cub", database, true);
    m_sprites[SpriteDiamondNugget]->load(path + "diamond-nugget.cub", database, true);
    m_sprites[SpriteIceCrystal]->load(path + "ice-crystal.cub", database, true);
    m_sprites[SpriteIceCoatedYarn]->load(path + "ice-coated-yarn.cub", database, true);
    m_sprites[SpriteIronCube]->load(path + "iron-cube.cub", database, true);
    m_sprites[SpriteGoldCube]->load(path + "gold-cube.cub", database, true);
    m_sprites[SpriteSilverCube]->load(path + "silver-cube.cub", database, true);
    m_sprites[SpriteWoodCube]->load(path + "wood-cube.cub", database, true);
    m_sprites[SpriteFireCube]->load(path + "fire-cube.cub", database, true);
    m_sprites[SpriteIceCube]->load(path + "ice-cube.cub", database, true);
    m_sprites[SpriteUnholyCube]->load(path + "unholy-cube.cub", database, true);
    m_sprites[SpriteWindCube]->load(path + "wind-cube.cub", database, true);
    m_sprites[SpriteCobweb]->load(path + "cobweb.cub", database, true);
    m_sprites[SpriteSandstone]->load(path + "sandstone.cub", database, true);
    m_sprites[SpriteWoodLog]->load(path + "wood-log.cub", database, true);
    m_sprites[SpriteDesertRib]->load(path + "desert-rib.cub", database, true);
    m_sprites[SpriteDesertSkull]->load(path + "desert-skull.cub", database, true);
    m_sprites[SpriteIronPickaxe]->load(path + "iron-pickaxe.cub", database, true);
    m_sprites[SpriteSlimeGreen]->load(path + "slime-green.cub", database, true);
    m_sprites[SpriteSlimeYellow]->load(path + "slime-yellow.cub", database, true);
    m_sprites[SpriteSlimePink]->load(path + "slime-pink.cub", database, true);
    m_sprites[SpriteSlimeBlue]->load(path + "slime-blue.cub", database, true);
    m_sprites[SpriteFrightenerHead]->load(path + "frightener-head.cub", database, true);
    m_sprites[SpriteFrightenerEyes]->load(path + "frightener-eyes.cub", database, false);
    m_sprites[SpriteSandHorrorHead]->load(path + "sand-horror-head.cub", database, true);
    m_sprites[SpriteSandHorrorHand]->load(path + "sand-horror-hand.cub", database, true);
    m_sprites[SpriteBunnyBody]->load(path + "bunny-body.cub", database, true);
    m_sprites[SpriteBunnyFoot]->load(path + "bunny-foot.cub", database, true);
    m_sprites[SpritePorcupineBody]->load(path + "porcupine-body.cub", database, true);
    m_sprites[SpritePorcupineFoot]->load(path + "porcupine-foot.cub", database, true);
    m_sprites[SpritePigBody]->load(path + "pig-body.cub", database, true);
    m_sprites[SpritePigFoot]->load(path + "pig-foot.cub", database, true);
    m_sprites[SpritePigHead]->load(path + "pig-head.cub", database, true);
    m_sprites[SpriteSheepBody]->load(path + "sheep-body.cub", database, true);
    m_sprites[SpriteSheepFoot]->load(path + "sheep-foot.cub", database, true);
    m_sprites[SpriteSheepHead]->load(path + "sheep-head.cub", database, true);
    m_sprites[SpriteDuckbillBody]->load(path + "duckbill-body.cub", database, true);
    m_sprites[SpriteDuckbillFoot]->load(path + "duckbill-foot.cub", database, true);
    m_sprites[SpriteDuckbillHead]->load(path + "duckbill-head.cub", database, true);
    m_sprites[SpriteDuckbillTail]->load(path + "duckbill-tail.cub", database, true);
    m_sprites[SpriteCrocodileBody]->load(path + "crocodile-body.cub", database, true);
    m_sprites[SpriteCrocodileFoot]->load(path + "crocodile-foot.cub", database, true);
    m_sprites[SpriteCrocodileHead]->load(path + "crocodile-head.cub", database, true);
    m_sprites[SpriteFish1]->load(path + "fish1.cub", database, true);
    m_sprites[SpriteFish2]->load(path + "fish2.cub", database, true);
    m_sprites[SpriteShark]->load(path + "shark.cub", database, true);
    m_sprites[SpriteSeahorse]->load(path + "seahorse.cub", database, true);
    m_sprites[SpriteLanternFish]->load(path + "lantern-fish.cub", database, true);
    m_sprites[SpriteLanternFishEyes]->load(path + "lantern-fish-eyes.cub", database, true);
    m_sprites[SpriteMawfish]->load(path + "mawfish.cub", database, true);
    m_sprites[SpritePiranha]->load(path + "piranha.cub", database, true);
    m_sprites[SpriteBlowfish]->load(path + "blowfish.cub", database, true);
    m_sprites[SpriteSeastar]->load(path + "seastar.cub", database, true);
    m_sprites[SpriteBread]->load(path + "bread.cub", database, true);
    m_sprites[SpriteSandwich]->load(path + "sandwich.cub", database, true);
    m_sprites[SpriteUnknown]->load(path + "unknown.cub", database, true);
    m_sprites[SpriteKey1]->load(path + "key1.cub", database, true);
    m_sprites[SpriteFlowers2]->load(path + "flowers2.cub", database, true);
    m_sprites[SpriteFlowers]->load(path + "flowers.cub", database, true);
    m_sprites[SpriteGrass]->load(path + "grass.cub", database, true);
    m_sprites[SpriteGrass2]->load(path + "grass2.cub", database, true);
    m_sprites[SpriteGrass3]->load(path + "grass3.cub", database, true);
    m_sprites[SpriteLavaFlower]->load(path + "lava-flower.cub", database, true);
    m_sprites[SpriteLavaGrass]->load(path + "lava-grass.cub", database, true);
    m_sprites[SpriteThornPlant]->load(path + "thorn-plant.cub", database, true);
    m_sprites[SpriteEchinacea2]->load(path + "echinacea2.cub", database, true);
    m_sprites[SpriteLeaf]->load(path + "leaf.cub", database, true);
    m_sprites[SpriteLantern02]->load(path + "lantern02.cub", database, true);
    m_sprites[SpritePlantTorch]->load(path + "torch.cub", database, true);
    m_sprites[SpriteStone]->load(path + "stone.cub", database, true);
    m_sprites[SpriteStone2]->load(path + "stone2.cub", database, true);
    m_sprites[SpriteTendril]->load(path + "tendril.cub", database, true);
    m_sprites[SpriteTulipsColorful]->load(path + "tulips-colorful.cub", database, true);
    m_sprites[SpriteCornflower]->load(path + "cornflower.cub", database, true);
    m_sprites[SpriteReed]->load(path + "reed.cub", database, true);
    m_sprites[SpritePumpkinLeaves]->load(path + "pumpkin-leaves.cub", database, true);
    m_sprites[SpritePineappleLeaves]->load(path + "pineapple-leaves.cub", database, true);
    m_sprites[SpriteSunflower]->load(path + "sunflower.cub", database, true);
    m_sprites[SpriteBeanTendril]->load(path + "bean-tendril.cub", database, true);
    m_sprites[SpriteDesertFlower01]->load(path + "desert-flower01.cub", database, true);
    m_sprites[SpriteDesertFlower02]->load(path + "desert-flower02.cub", database, true);
    m_sprites[SpriteWheat]->load(path + "wheat.cub", database, true);
    m_sprites[SpriteCorn]->load(path + "corn.cub", database, true);
    m_sprites[SpriteWaterLily01]->load(path + "water-lily01.cub", database, true);
    m_sprites[SpriteWaterLily02]->load(path + "water-lily02.cub", database, true);
    m_sprites[SpriteInnSign]->load(path + "inn-sign.cub", database, true);
    m_sprites[SpriteIdentifierSign]->load(path + "identifier-sign.cub", database, true);
    m_sprites[SpriteShopSign]->load(path + "shop-sign.cub", database, true);
    m_sprites[SpriteWeaponShopSign]->load(path + "weapon-shop-sign.cub", database, true);
    m_sprites[SpriteArmorShopSign]->load(path + "armor-shop-sign.cub", database, true);
    m_sprites[SpriteSmithySign]->load(path + "smithy-sign.cub", database, true);
    m_sprites[SpriteCarpentersshopSign]->load(path + "carpentersshop-sign.cub", database, true);
    m_sprites[SpriteWeavingmillSign]->load(path + "weavingmill-sign.cub", database, true);
    m_sprites[SpriteIvy]->load(path + "ivy.cub", database, true);
    m_sprites[SpriteWallRosesWhite]->load(path + "wall-roses-white.cub", database, true);
    m_sprites[SpriteWallRosesRed]->load(path + "wall-roses-red.cub", database, true);
    m_sprites[SpriteChristmasTree]->load(path + "christmas-tree.cub", database, true);
    m_sprites[SpriteCoral]->load(path + "coral.cub", database, true);
    m_sprites[SpriteUnderwaterPlant]->load(path + "underwater-plant.cub", database, true);
    m_sprites[SpriteAlga]->load(path + "alga.cub", database, true);
    m_sprites[SpriteIncaArt1]->load(path + "inca-art1.cub", database, true);
    m_sprites[SpriteIncaArt2]->load(path + "inca-art2.cub", database, true);
    m_sprites[SpriteIncaArt3]->load(path + "inca-art3.cub", database, true);
    m_sprites[SpriteIncaArt4]->load(path + "inca-art4.cub", database, true);
    m_sprites[SpriteLiana]->load(path + "liana.cub", database, true);
    m_sprites[SpriteCrest1]->load(path + "crest1.cub", database, true);
    m_sprites[SpriteCastleChain]->load(path + "castle-chain.cub", database, true);
    m_sprites[SpriteWallSkull]->load(path + "wall-skull.cub", database, true);
    m_sprites[SpriteTorchRead]->load(path + "torch-read.cub", database, true);
    m_sprites[SpriteTorchGreen]->load(path + "torch-green.cub", database, true);
    m_sprites[SpriteTorchBlue]->load(path + "torch-blue.cub", database, true);
    m_sprites[SpriteTorchYellow]->load(path + "torch-yellow.cub", database, true);
    m_sprites[SpriteChandelier]->load(path + "chandelier.cub", database, true);
    m_sprites[SpriteCobwebs]->load(path + "cobwebs.cub", database, true);
    m_sprites[SpriteCobwebs2]->load(path + "cobwebs2.cub", database, true);
    m_sprites[SpriteObjectGoddess]->load(path + "goddess2.cub", database, true);
    m_sprites[SpriteObjectDoor]->load(path + "door.cub", database, true);
    m_sprites[SpriteBigDoor]->load(path + "big-door.cub", database, true);
    m_sprites[SpriteObjectWindow]->load(path + "window.cub", database, true);
    m_sprites[SpriteCastleWindow]->load(path + "castle-window.cub", database, true);
    m_sprites[SpriteGate]->load(path + "gate.cub", database, true);
    m_sprites[SpriteSpikeTrap]->load(path + "spike-trap.cub", database, true);
    m_sprites[SpriteStompTrap]->load(path + "stomp-trap.cub", database, true);
    m_sprites[SpriteLever]->load(path + "lever.cub", database, true);
    m_sprites[SpriteChestBase02]->load(path + "chest-base02.cub", database, true);
    m_sprites[SpriteChestTop02]->load(path + "chest-top02.cub", database, true);
    m_sprites[SpriteTable]->load(path + "table.cub", database, true);
    m_sprites[SpriteStoneTable]->load(path + "stone-table.cub", database, true);
    m_sprites[SpriteSandstoneTable]->load(path + "sandstone-table.cub", database, true);
    m_sprites[SpriteStoneStool]->load(path + "stone-stool.cub", database, true);
    m_sprites[SpriteSandstoneStool]->load(path + "sandstone-stool.cub", database, true);
    m_sprites[SpriteStool]->load(path + "stool.cub", database, true);
    m_sprites[SpriteBench]->load(path + "bench.cub", database, true);
    m_sprites[SpriteBed]->load(path + "bed.cub", database, true);
    m_sprites[SpriteBedtable]->load(path + "bedtable.cub", database, true);
    m_sprites[SpriteCrate]->load(path + "crate.cub", database, true);
    m_sprites[SpriteOpenCrate]->load(path + "open-crate.cub", database, true);
    m_sprites[SpriteSack]->load(path + "sack.cub", database, true);
    m_sprites[SpriteMarketStand1]->load(path + "market-stand1.cub", database, true);
    m_sprites[SpriteMarketStand2]->load(path + "market-stand2.cub", database, true);
    m_sprites[SpriteMarketStand3]->load(path + "market-stand3.cub", database, true);
    m_sprites[SpriteBarrel]->load(path + "barrel.cub", database, true);
    m_sprites[SpriteShelter]->load(path + "shelter.cub", database, true);
    m_sprites[SpriteCupboard]->load(path + "cupboard.cub", database, true);
    m_sprites[SpriteDesktop]->load(path + "desktop.cub", database, true);
    m_sprites[SpriteCounter]->load(path + "counter.cub", database, true);
    m_sprites[SpriteShelf1]->load(path + "shelf1.cub", database, true);
    m_sprites[SpriteShelf2]->load(path + "shelf2.cub", database, true);
    m_sprites[SpriteShelf3]->load(path + "shelf3.cub", database, true);
    m_sprites[SpriteCastleShelf1]->load(path + "castle-shelf1.cub", database, true);
    m_sprites[SpriteCastleShelf2]->load(path + "castle-shelf2.cub", database, true);
    m_sprites[SpriteCastleShelf3]->load(path + "castle-shelf3.cub", database, true);
    m_sprites[SpriteStoneShelf1]->load(path + "stone-shelf1.cub", database, true);
    m_sprites[SpriteStoneShelf2]->load(path + "stone-shelf2.cub", database, true);
    m_sprites[SpriteStoneShelf3]->load(path + "stone-shelf3.cub", database, true);
    m_sprites[SpriteSandstoneShelf1]->load(path + "sandstone-shelf1.cub", database, true);
    m_sprites[SpriteSandstoneShelf2]->load(path + "sandstone-shelf2.cub", database, true);
    m_sprites[SpriteSandstoneShelf3]->load(path + "sandstone-shelf3.cub", database, true);
    m_sprites[SpriteCorpse]->load(path + "corpse.cub", database, true);
    m_sprites[SpriteRunestone]->load(path + "runestone.cub", database, true);
    m_sprites[SpriteFlowerBox01]->load(path + "flower-box01.cub", database, true);
    m_sprites[SpriteFlowerBox02]->load(path + "flower-box02.cub", database, true);
    m_sprites[SpriteFlowerBox03]->load(path + "flower-box03.cub", database, true);
    m_sprites[SpriteStreetLight02]->load(path + "street-light02.cub", database, true);
    m_sprites[SpriteStreetLight01]->load(path + "street-light01.cub", database, true);
    m_sprites[SpriteFence01]->load(path + "fence01.cub", database, true);
    m_sprites[SpriteFence02]->load(path + "fence02.cub", database, true);
    m_sprites[SpriteFence03]->load(path + "fence03.cub", database, true);
    m_sprites[SpriteFence04]->load(path + "fence04.cub", database, true);
    m_sprites[SpriteFurnace]->load(path + "furnace.cub", database, true);
    m_sprites[SpriteAnvil]->load(path + "anvil.cub", database, true);
    m_sprites[SpriteSpinningwheel]->load(path + "spinningwheel.cub", database, true);
    m_sprites[SpriteLoom]->load(path + "loom.cub", database, true);
    m_sprites[SpriteSawbench]->load(path + "sawbench.cub", database, true);
    m_sprites[SpriteWorkbench]->load(path + "workbench.cub", database, true);
    m_sprites[SpriteCustomizationBench]->load(path + "customization-bench.cub", database, true);
    m_sprites[SpriteOvergroundDungeon01]->load(path + "overground-dungeon01.cub", database, true);
    m_sprites[SpriteUndergroundDungeon01]->load(path + "underground-dungeon01.cub", database, true);
    m_sprites[SpriteCastleArc]->load(path + "castle-arc.cub", database, true);
    m_sprites[SpriteTempleArc]->load(path + "temple-arc.cub", database, true);
    m_sprites[SpriteBomb1]->load(path + "bomb1.cub", database, true);
    m_sprites[SpriteGlider]->load(path + "glider.cub", database, true);
    m_sprites[SpriteBoat2]->load(path + "boat2.cub", database, true);
    m_sprites[SpriteStoneBridge]->load(path + "stone-bridge.cub", database, true);
    m_sprites[SpriteEntranceStairs]->load(path + "entrance-stairs.cub", database, true);
    m_sprites[SpriteCarpet1]->load(path + "carpet1.cub", database, true);
    m_sprites[SpriteCarpet2]->load(path + "carpet2.cub", database, true);
    m_sprites[SpriteCarpet3]->load(path + "carpet3.cub", database, true);
    m_sprites[SpriteFrameworkBase]->load(path + "framework-base.cub", database, true);
    m_sprites[SpriteFrameworkFloor]->load(path + "framework-floor.cub", database, true);
    m_sprites[SpriteFrameworkFloorStairs]->load(path + "framework-floor-stairs.cub", database, true);
    m_sprites[SpriteFrameworkWall]->load(path + "framework-wall.cub", database, true);
    m_sprites[SpriteFrameworkWallWindow]->load(path + "framework-wall-window.cub", database, true);
    m_sprites[SpriteFrameworkWallDoor]->load(path + "framework-wall-door.cub", database, true);
    m_sprites[SpriteFrameworkWallIndoor]->load(path + "framework-wall-indoor.cub", database, true);
    m_sprites[SpriteFrameworkWallBalcony]->load(path + "framework-wall-balcony.cub", database, true);
    m_sprites[SpriteFrameworkWallLamp]->load(path + "framework-wall-lamp.cub", database, true);
    m_sprites[SpriteFrameworkRoof1]->load(path + "framework-roof1.cub", database, true);
    m_sprites[SpriteFrameworkRoof2]->load(path + "framework-roof2.cub", database, true);
    m_sprites[SpriteFrameworkRoof3]->load(path + "framework-roof3.cub", database, true);
    m_sprites[SpriteFrameworkArc]->load(path + "framework-arc.cub", database, true);
    m_sprites[SpriteStoneBase]->load(path + "stone-base.cub", database, true);
    m_sprites[SpriteStoneFloor]->load(path + "stone-floor.cub", database, true);
    m_sprites[SpriteStoneFloorStairs]->load(path + "stone-floor-stairs.cub", database, true);
    m_sprites[SpriteStoneWall]->load(path + "stone-wall.cub", database, true);
    m_sprites[SpriteStoneWallWindow]->load(path + "stone-wall-window.cub", database, true);
    m_sprites[SpriteStoneWallDoor]->load(path + "stone-wall-door.cub", database, true);
    m_sprites[SpriteStoneWallIndoor]->load(path + "stone-wall-indoor.cub", database, true);
    m_sprites[SpriteStoneWallBalcony]->load(path + "stone-wall-balcony.cub", database, true);
    m_sprites[SpriteStoneWallLamp]->load(path + "stone-wall-lamp.cub", database, true);
    m_sprites[SpriteStoneRoof1]->load(path + "stone-roof1.cub", database, true);
    m_sprites[SpriteStoneRoof2]->load(path + "stone-roof2.cub", database, true);
    m_sprites[SpriteStoneRoof3]->load(path + "stone-roof3.cub", database, true);
    m_sprites[SpriteStoneArc]->load(path + "stone-arc.cub", database, true);
    m_sprites[SpriteWhitewoodBase]->load(path + "whitewood-base.cub", database, true);
    m_sprites[SpriteWhitewoodFloor]->load(path + "whitewood-floor.cub", database, true);
    m_sprites[SpriteWhitewoodFloorStairs]->load(path + "whitewood-floor-stairs.cub", database, true);
    m_sprites[SpriteWhitewoodWall]->load(path + "whitewood-wall.cub", database, true);
    m_sprites[SpriteWhitewoodWallWindow]->load(path + "whitewood-wall-window.cub", database, true);
    m_sprites[SpriteWhitewoodWallDoor]->load(path + "whitewood-wall-door.cub", database, true);
    m_sprites[SpriteWhitewoodWallIndoor]->load(path + "whitewood-wall-indoor.cub", database, true);
    m_sprites[SpriteWhitewoodWallBalcony]->load(path + "whitewood-wall-balcony.cub", database, true);
    m_sprites[SpriteWhitewoodWallLamp]->load(path + "whitewood-wall-lamp.cub", database, true);
    m_sprites[SpriteWhitewoodRoof1]->load(path + "whitewood-roof1.cub", database, true);
    m_sprites[SpriteWhitewoodRoof2]->load(path + "whitewood-roof2.cub", database, true);
    m_sprites[SpriteWhitewoodRoof3]->load(path + "whitewood-roof3.cub", database, true);
    m_sprites[SpriteWhitewoodArc]->load(path + "whitewood-arc.cub", database, true);
    m_sprites[SpriteWoodBase]->load(path + "wood-base.cub", database, true);
    m_sprites[SpriteWoodFloor]->load(path + "wood-floor.cub", database, true);
    m_sprites[SpriteWoodFloorStairs]->load(path + "wood-floor-stairs.cub", database, true);
    m_sprites[SpriteWoodWall]->load(path + "wood-wall.cub", database, true);
    m_sprites[SpriteWoodWallWindow]->load(path + "wood-wall-window.cub", database, true);
    m_sprites[SpriteWoodWallDoor]->load(path + "wood-wall-door.cub", database, true);
    m_sprites[SpriteWoodWallIndoor]->load(path + "wood-wall-indoor.cub", database, true);
    m_sprites[SpriteWoodWallBalcony]->load(path + "wood-wall-balcony.cub", database, true);
    m_sprites[SpriteWoodWallLamp]->load(path + "wood-wall-lamp.cub", database, true);
    m_sprites[SpriteWoodRoof1]->load(path + "wood-roof1.cub", database, true);
    m_sprites[SpriteWoodRoof2]->load(path + "wood-roof2.cub", database, true);
    m_sprites[SpriteWoodRoof3]->load(path + "wood-roof3.cub", database, true);
    m_sprites[SpriteWoodArc]->load(path + "wood-arc.cub", database, true);
    m_sprites[SpriteClayEntranceStairs]->load(path + "clay-entrance-stairs.cub", database, true);
    m_sprites[SpriteClayBase]->load(path + "clay-base.cub", database, true);
    m_sprites[SpriteClayFloor]->load(path + "clay-floor.cub", database, true);
    m_sprites[SpriteClayFloorStairs]->load(path + "clay-floor-stairs.cub", database, true);
    m_sprites[SpriteClayWall]->load(path + "clay-wall.cub", database, true);
    m_sprites[SpriteClayWallWindow]->load(path + "clay-wall-window.cub", database, true);
    m_sprites[SpriteClayWallDoor]->load(path + "clay-wall-door.cub", database, true);
    m_sprites[SpriteClayWallIndoor]->load(path + "clay-wall-indoor.cub", database, true);
    m_sprites[SpriteClayWallBalcony]->load(path + "clay-wall-balcony.cub", database, true);
    m_sprites[SpriteClayWallLamp]->load(path + "clay-wall-lamp.cub", database, true);
    m_sprites[SpriteClayRoof1]->load(path + "clay-roof1.cub", database, true);
    m_sprites[SpriteClayRoof2]->load(path + "clay-roof2.cub", database, true);
    m_sprites[SpriteClayRoof3]->load(path + "clay-roof3.cub", database, true);
    m_sprites[SpriteClayArc]->load(path + "clay-arc.cub", database, true);
    m_sprites[SpriteWhiteclayEntranceStairs]->load(path + "whiteclay-entrance-stairs.cub", database, true);
    m_sprites[SpriteWhiteclayBase]->load(path + "whiteclay-base.cub", database, true);
    m_sprites[SpriteWhiteclayFloor]->load(path + "whiteclay-floor.cub", database, true);
    m_sprites[SpriteWhiteclayFloorStairs]->load(path + "whiteclay-floor-stairs.cub", database, true);
    m_sprites[SpriteWhiteclayWall]->load(path + "whiteclay-wall.cub", database, true);
    m_sprites[SpriteWhiteclayWallWindow]->load(path + "whiteclay-wall-window.cub", database, true);
    m_sprites[SpriteWhiteclayWallDoor]->load(path + "whiteclay-wall-door.cub", database, true);
    m_sprites[SpriteWhiteclayWallIndoor]->load(path + "whiteclay-wall-indoor.cub", database, true);
    m_sprites[SpriteWhiteclayWallBalcony]->load(path + "whiteclay-wall-balcony.cub", database, true);
    m_sprites[SpriteWhiteclayWallLamp]->load(path + "whiteclay-wall-lamp.cub", database, true);
    m_sprites[SpriteWhiteclayRoof1]->load(path + "whiteclay-roof1.cub", database, true);
    m_sprites[SpriteWhiteclayRoof2]->load(path + "whiteclay-roof2.cub", database, true);
    m_sprites[SpriteWhiteclayRoof3]->load(path + "whiteclay-roof3.cub", database, true);
    m_sprites[SpriteWhiteclayArc]->load(path + "whiteclay-arc.cub", database, true);
    m_sprites[SpriteAntiqueruinEntranceStairs]->load(path + "antiqueruin-entrance-stairs.cub", database, true);
    m_sprites[SpriteAntiqueruinBase]->load(path + "antiqueruin-base.cub", database, true);
    m_sprites[SpriteAntiqueruinFloor]->load(path + "antiqueruin-floor.cub", database, true);
    m_sprites[SpriteAntiqueruinFloorStairs]->load(path + "antiqueruin-floor-stairs.cub", database, true);
    m_sprites[SpriteAntiqueruinWall]->load(path + "antiqueruin-wall.cub", database, true);
    m_sprites[SpriteAntiqueruinRoof1]->load(path + "antiqueruin-roof1.cub", database, true);
    m_sprites[SpriteAntiqueruinRoof2]->load(path + "antiqueruin-roof2.cub", database, true);
    m_sprites[SpriteAntiqueruinRoof3]->load(path + "antiqueruin-roof3.cub", database, true);
    m_sprites[SpriteAntiqueruinArc]->load(path + "antiqueruin-arc.cub", database, true);
    m_sprites[SpriteJungleruinEntranceStairs]->load(path + "jungleruin-entrance-stairs.cub", database, true);
    m_sprites[SpriteJungleruinBase]->load(path + "jungleruin-base.cub", database, true);
    m_sprites[SpriteJungleruinFloor]->load(path + "jungleruin-floor.cub", database, true);
    m_sprites[SpriteJungleruinFloorStairs]->load(path + "jungleruin-floor-stairs.cub", database, true);
    m_sprites[SpriteJungleruinWall]->load(path + "jungleruin-wall.cub", database, true);
    m_sprites[SpriteJungleruinWallDoor]->load(path + "jungleruin-wall-door.cub", database, true);
    m_sprites[SpriteJungleruinWallIndoor]->load(path + "jungleruin-wall-indoor.cub", database, true);
    m_sprites[SpriteJungleruinRoof1]->load(path + "jungleruin-roof1.cub", database, true);
    m_sprites[SpriteJungleruinRoof2]->load(path + "jungleruin-roof2.cub", database, true);
    m_sprites[SpriteJungleruinRoof3]->load(path + "jungleruin-roof3.cub", database, true);
    m_sprites[SpriteJungleruinArc]->load(path + "jungleruin-arc.cub", database, true);
    m_sprites[SpriteDesertruinEntranceStairs]->load(path + "desertruin-entrance-stairs.cub", database, true);
    m_sprites[SpriteDesertruinBase]->load(path + "desertruin-base.cub", database, true);
    m_sprites[SpriteDesertruinFloor]->load(path + "desertruin-floor.cub", database, true);
    m_sprites[SpriteDesertruinFloorStairs]->load(path + "desertruin-floor-stairs.cub", database, true);
    m_sprites[SpriteDesertruinWall]->load(path + "desertruin-wall.cub", database, true);
    m_sprites[SpriteDesertruinWallDoor]->load(path + "desertruin-wall-door.cub", database, true);
    m_sprites[SpriteDesertruinWallIndoor]->load(path + "desertruin-wall-indoor.cub", database, true);
    m_sprites[SpriteDesertruinRoof1]->load(path + "desertruin-roof1.cub", database, true);
    m_sprites[SpriteDesertruinRoof2]->load(path + "desertruin-roof2.cub", database, true);
    m_sprites[SpriteDesertruinRoof3]->load(path + "desertruin-roof3.cub", database, true);
    m_sprites[SpriteDesertruinArc]->load(path + "desertruin-arc.cub", database, true);
    m_sprites[SpriteAirship]->load(path + "airship.cub", database, true);

    // Red voxels mark socket positions in this model; erase them after recording.
    for (int x = 0; x < m_sprites[SpriteAirship]->sizeX(); x++) {
        for (int y = 0; y < m_sprites[SpriteAirship]->sizeY(); y++) {
            for (int z = 0; z < m_sprites[SpriteAirship]->sizeZ(); z++) {
                if (m_sprites[SpriteAirship]->voxel(x, y, z) == plasma::Vector<3, unsigned char>(255, 0, 0)) {
                    m_sprites[SpriteAirship]->voxel(x, y, z) = plasma::Vector<3, unsigned char>(0, 0, 0);
                    m_sprites[SpriteAirship]->m_positionLists[SpritePositionListAirshipCrew].push_back(
                        plasma::Vector<3, int>(x, y, z));
                }
            }
        }
    }

    m_sprites[SpriteSphinx01]->load(path + "sphinx01.cub", database, true);
    m_sprites[SpriteObelisk]->load(path + "obelisk.cub", database, true);
    m_sprites[SpriteManacube]->load(path + "manacube.cub", database, true);
    m_sprites[SpriteMaptile]->load(path + "maptile.cub", database, true);
    m_sprites[SpriteCloud02]->load(path + "cloud02.cub", database, true);

    if (database)
        delete database;
}

int weaponSpriteIndex(int id, int modifier, int rarity, int material)
{
    switch (id) {
    case WeaponIronPickaxe:
        return SpriteIronPickaxe;
    case WeaponStaff:
        switch (material) {
        case ItemMaterialObsidian:
            return SpriteObsidianStaff1 + std::min<int>(rarity, ItemRarityYellow);
        default:
            return SpriteWoodStaff1 + modifier % WeaponModelVariants * RaritySpriteCount +
                   std::min<int>(rarity, ItemRarityYellow);
        }
    case WeaponWand:
        return SpriteWoodWand1 + modifier % WeaponModelVariants * RaritySpriteCount +
               std::min<int>(rarity, ItemRarityYellow);
    case WeaponBracelet:
        if (material == ItemMaterialGold)
            return SpriteGoldBracelet1 + modifier % BraceletModelVariants * RaritySpriteCount +
                   std::min<int>(rarity, ItemRarityYellow);
        return SpriteSilverBracelet1 + modifier % BraceletModelVariants * RaritySpriteCount +
               std::min<int>(rarity, ItemRarityYellow);
    case WeaponSword:
        switch (material) {
        case ItemMaterialObsidian:
            return SpriteObsidianSword;
        case ItemMaterialBone:
            return SpriteBoneSword;
        default:
            return SpriteIronSword1 + modifier % WeaponModelVariants * RaritySpriteCount +
                   std::min<int>(rarity, ItemRarityYellow);
        }
    case WeaponDagger:
        return SpriteIronDagger1 + modifier % WeaponModelVariants * RaritySpriteCount +
               std::min<int>(rarity, ItemRarityYellow);
    case WeaponFist:
        return SpriteIronFist1 + modifier % WeaponModelVariants * RaritySpriteCount +
               std::min<int>(rarity, ItemRarityYellow);
    case WeaponGreatsword:
        switch (material) {
        case ItemMaterialObsidian:
            return SpriteObsidianGreatsword;
        case ItemMaterialBone:
            return SpriteBoneGreatsword;
        default:
            return SpriteIronGreatsword1 + modifier % WeaponModelVariants * RaritySpriteCount +
                   std::min<int>(rarity, ItemRarityYellow);
        }
    case WeaponLongsword:
        return SpriteIronLongsword1 + modifier % WeaponModelVariants * RaritySpriteCount +
               std::min<int>(rarity, ItemRarityYellow);
    case WeaponMace:
        switch (material) {
        case ItemMaterialWood:
            return SpriteWoodMace01 + std::min<int>(rarity, ItemRarityYellow);
        case ItemMaterialBone:
            return SpriteBoneMace;
        default:
            return SpriteIronMace1 + modifier % WeaponModelVariants * RaritySpriteCount +
                   std::min<int>(rarity, ItemRarityYellow);
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
            return SpriteIronGreatmace1 + modifier % WeaponModelVariants * RaritySpriteCount +
                   std::min<int>(rarity, ItemRarityYellow);
        }
    case WeaponAxe:
        switch (material) {
        case ItemMaterialObsidian:
            return SpriteObsidianAxe;
        case ItemMaterialBone:
            return SpriteBoneAxe;
        default:
            return SpriteIronAxe1 + modifier % WeaponModelVariants * RaritySpriteCount +
                   std::min<int>(rarity, ItemRarityYellow);
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
            return SpriteIronGreataxe1 + modifier % WeaponModelVariants * RaritySpriteCount +
                   std::min<int>(rarity, ItemRarityYellow);
        }
    case WeaponBow:
        return SpriteBow + modifier % WeaponModelVariants * RaritySpriteCount + std::min<int>(rarity, ItemRarityYellow);
    case WeaponBoomerang:
        return SpriteWoodBoomerang1 + modifier % WeaponModelVariants * RaritySpriteCount +
               std::min<int>(rarity, ItemRarityYellow);
    case WeaponCrossbow:
        return SpriteWoodCrossbow1 + modifier % WeaponModelVariants * RaritySpriteCount +
               std::min<int>(rarity, ItemRarityYellow);
    case WeaponArrows:
        return SpriteQuiver;
    case WeaponShield:
        switch (material) {
        case ItemMaterialWood:
            return SpriteWoodShield01 + std::min<int>(rarity, ItemRarityYellow);
        default:
            return SpriteIronShield01 + modifier % WeaponModelVariants * RaritySpriteCount +
                   std::min<int>(rarity, ItemRarityYellow);
        }
    case WeaponArrow:
        return SpriteArrow;
    case WeaponTorch:
        return SpriteTorch;
    case WeaponPitchfork:
        return SpritePitchfork;
    default:
        return SpriteIronSword1;
    }
}

}
