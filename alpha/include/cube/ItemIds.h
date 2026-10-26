#pragma once
#include "cube/CreatureIds.h"
namespace cube {
enum ItemRarity {
    ItemRarityWhite = 0,
    ItemRarityGreen = 1,
    ItemRarityBlue = 2,
    ItemRarityPurple = 3,
    ItemRarityYellow = 4
};
// World:createItemName: rarity selects a ten-name adjective table. Values are the modifier
// remainder, named by the alpha dictionary keys.
enum ItemNameModifier {
    ItemNameWhiteShabby = 0,
    ItemNameWhitePlain = 1,
    ItemNameWhiteBattered = 2,
    ItemNameWhiteArtless = 3,
    ItemNameWhiteUnwieldy = 4,
    ItemNameWhiteUsed = 5,
    ItemNameWhiteDusty = 6,
    ItemNameWhiteScratched = 7,
    ItemNameWhiteWorn = 8,
    ItemNameWhiteCommon = 9,
    ItemNameGreenAdjusted = 0,
    ItemNameGreenBalanced = 1,
    ItemNameGreenBattletested = 2,
    ItemNameGreenGood = 3,
    ItemNameGreenHandmade = 4,
    ItemNameGreenFair = 5,
    ItemNameGreenNeat = 6,
    ItemNameGreenClean = 7,
    ItemNameGreenUndamaged = 8,
    ItemNameGreenFlawless = 9,
    ItemNameBlueExceptional = 0,
    ItemNameBluePolished = 1,
    ItemNameBlueExtraordinary = 2,
    ItemNameBlueExquisite = 3,
    ItemNameBlueSuperb = 4,
    ItemNameBlueUnique = 5,
    ItemNameBlueHandsome = 6,
    ItemNameBlueGrand = 7,
    ItemNameBlueMagic = 8,
    ItemNameBlueDecorated = 9,
    ItemNamePurpleExceptional = 0,
    ItemNamePurplePolished = 1,
    ItemNamePurpleExtraordinary = 2,
    ItemNamePurpleExquisite = 3,
    ItemNamePurpleSuperb = 4,
    ItemNamePurpleUnique = 5,
    ItemNamePurpleHandsome = 6,
    ItemNamePurpleGrand = 7,
    ItemNamePurpleMagic = 8,
    ItemNamePurpleDecorated = 9,
    ItemNameYellowBrilliant = 0,
    ItemNameYellowShining = 1,
    ItemNameYellowMagnificent = 2,
    ItemNameYellowSublime = 3,
    ItemNameYellowPompous = 4,
    ItemNameYellowGlorious = 5,
    ItemNameYellowSplendid = 6,
    ItemNameYellowFamous = 7,
    ItemNameYellowLegendary = 8,
    ItemNameYellowFabulous = 9
};
enum {
    ItemNameModifierCount = 10
};

// Alpha World::World item dictionary keys.
enum MaterialItemId {
    MaterialItemFlask = 12,
    MaterialItemShimmerMushroom = 19,
    MaterialItemNugget = 0,
    MaterialItemLog = 1,
    MaterialItemFeather = 2,
    MaterialItemHorn = 3,
    MaterialItemClaw = 4,
    MaterialItemFiber = 5,
    MaterialItemCobweb = 6,
    MaterialItemHair = 7,
    MaterialItemCrystal = 8,
    MaterialItemYarn = 9,
    MaterialItemCube = 10,
    MaterialItemCapsule = 11,
    MaterialItemOrb = 13,
    MaterialItemSpirit = 14,
    MaterialItemMushroom = 15,
    MaterialItemPumpkin = 16,
    MaterialItemPineapple = 17,
    MaterialItemRadishSlice = 18,
    MaterialItemGinsengRoot = 20,
    MaterialItemOnionSlice = 21,
    MaterialItemHeartflower = 22,
    MaterialItemPricklyPear = 23,
    MaterialItemFrozenHeartflower = 24,
    MaterialItemSoulflower = 25,
    MaterialItemWaterFlask = 26,
    MaterialItemSnowBerry = 27,
};

enum SpecialItemId {
    SpecialItemHangGlider = 0,
    SpecialItemBoat = 1
};

// Alpha category-1 item IDs, confirmed by the consumable model switch.
enum ConsumableId {
    ConsumableCookie = 0,
    ConsumableLifePotion = 1,
    ConsumableCactusPotion = 2,
    ConsumableManaPotion = 3, // the alpha's dictionary key (id 3 is EssenceOfWisdom)
    ConsumableGinsengSoup = 4,
    ConsumableSnowBerryMash = 5,
    ConsumableMushroomSpit = 6,
    ConsumableBomb = 7,
    ConsumablePineappleSlice = 8,
    ConsumablePumpkinMuffin = 9
};

enum WeaponId {
    WeaponSword = 0,
    WeaponAxe = 1,
    WeaponMace = 2,
    WeaponDagger = 3,
    WeaponFist = 4,
    WeaponLongsword = 5,
    WeaponBow = 6,
    WeaponCrossbow = 7,
    WeaponBoomerang = 8,
    WeaponArrow = 9,
    WeaponStaff = 10,
    WeaponWand = 11,
    WeaponBracelet = 12,
    WeaponShield = 13,
    WeaponArrows = 14,
    WeaponGreatsword = 15,
    WeaponGreataxe = 16,
    WeaponGreatmace = 17,
    WeaponPitchfork = 18,
    WeaponIronPickaxe = 19,
    WeaponTorch = 20
};

// Material identifiers selected by model filenames and dictionary keys.
enum ItemMaterial {
    ItemMaterialIron = 1,
    ItemMaterialWood = 2,
    ItemMaterial3 = 3,
    ItemMaterialObsidian = 5,
    ItemMaterialClothing = 6,
    ItemMaterialBone = 7,
    ItemMaterialGold = 11,
    ItemMaterialSilver = 12,
    ItemMaterialSaurian = 18,
    ItemMaterialParrot = 19,
    ItemMaterialIce = 22,
    ItemMaterialLight = 23,
    ItemMaterialSilk = 25,
    ItemMaterialLinen = 26,
    ItemMaterialCotton = 27,
    ItemMaterialFireSpirit = 128,
    ItemMaterialUnholySpirit = 129,
    ItemMaterialIceSpirit = 130,
    ItemMaterialWindSpirit = 131,
    // Named after their dictionary keys.
    ItemMaterialCopper = 10,
    ItemMaterialEmerald = 13,
    ItemMaterialSapphire = 14,
    ItemMaterialRuby = 15,
    ItemMaterialDiamond = 16,
    ItemMaterialSandstone = 17,
    ItemMaterialMammoth = 20,
    ItemMaterialPlant = 21,
    ItemMaterialGlass = 24
};

// Subtype values consumed by SpriteManager::itemSprite. Unknown names stay numeric.
enum PetItemId {
    PetItemEgg = 24,
    PetItemTurtle = 25,
};
// Category 18 (candle) subtypes: both have the key Candle; 0 burns warm, 1 green with the undead
// candle models (Item::lightColor, SpriteManager::itemSprite).
enum CandleItemId {
    CandleItemCandle = 0,
    CandleItemUndead = 1
};
// Category 4 (chest) subtypes, ChestId: 0 has the dictionary key ChestArmor (with clothing
// material its model is innkeeper-chest), 1.. the clothing models SpriteManager::itemSprite
// selects.
enum ChestId {
    ChestChestArmor = 0,
    ChestBackpack = 1,
    ChestBody1 = 2,
    ChestBody2 = 3,
    ChestBody3 = 4,
    ChestBody4 = 5,
    ChestBody5 = 6,
};

enum QuestItemId {
    QuestItemAmulet1 = 0,
    QuestItemAmulet2 = 1,
    QuestItemJewelCase = 2,
    QuestItemKey = 3,
    QuestItemMedicine = 4,
    QuestItemAntivenom = 5,
    QuestItemBandAid = 6,
    QuestItemCrutch = 7,
    QuestItemBandage = 8,
    QuestItemSalve = 9,
};

// Single-subtype categories, named by their subtype-zero dictionary key.
enum FeetId {
    FeetBoots = 0
}; // category 6, key Boots
enum HandsId {
    HandsGloves = 0
}; // category 5, key Gloves
enum ShoulderId {
    ShoulderShoulderArmor = 0
}; // category 7, key ShoulderArmor
enum NeckId {
    NeckAmulet = 0
}; // category 8, key Amulet
enum RingId {
    RingRing = 0
}; // category 9, key Ring
enum FormulaId {
    FormulaFormula = 0
}; // category 2, key Formula
enum CoinId {
    CoinCoin = 0
}; // category 12, key Coin
enum PlatinumCoinId {
    PlatinumCoinPlatinumCoin = 0
}; // category 13, key PlatinumCoin
enum LampId {
    LampLamp = 0
}; // category 24, key Lamp
enum ManaCubeId {
    ManaCubeManaCube = 0
}; // category 25, key ManaCube
enum BeakId {
    BeakBeak = 0
}; // category 15, key Beak
enum LeftoversId {
    LeftoversLeftovers = 0
}; // category 14, key Leftovers
enum PaintingId {
    PaintingPainting = 0
}; // category 16, key Painting

// Category 20 (pet food) subtypes 0..155: World::World maps every one to the key Bait, then
// overrides 47 with their own keys. A bait is named after the race whose id it shares (AI's
// feeding test compares the item id with the creature's race); PetFoodId has the same scheme.
enum PetFoodId {
    PetFoodBaitForElfMale = 0,
    PetFoodBaitForElfFemale = 1,
    PetFoodBaitForHumanMale = 2,
    PetFoodBaitForHumanFemale = 3,
    PetFoodBaitForGoblinMale = 4,
    PetFoodBaitForGoblinFemale = 5,
    PetFoodBaitForBullterrier = 6,
    PetFoodBaitForLizardmanMale = 7,
    PetFoodBaitForLizardmanFemale = 8,
    PetFoodBaitForDwarfMale = 9,
    PetFoodBaitForDwarfFemale = 10,
    PetFoodBaitForOrcMale = 11,
    PetFoodBaitForOrcFemale = 12,
    PetFoodBaitForFrogmanMale = 13,
    PetFoodBaitForFrogmanFemale = 14,
    PetFoodBaitForUndeadMale = 15,
    PetFoodBaitForUndeadFemale = 16,
    PetFoodBaitForSkeleton = 17,
    PetFoodBaitForOldMan = 18,
    PetFoodBubbleGum = 19,
    PetFoodBaitForShepherdDog = 20,
    PetFoodBaitForSkullBull = 21,
    PetFoodVanillaCupcake = 22,
    PetFoodChocolateCupcake = 23,
    PetFoodBaitForEgg = 24,
    PetFoodCinnamonRole = 25,
    PetFoodWaffle = 26,
    PetFoodCroissant = 27,
    PetFoodBaitForWolf = 28,
    PetFoodBaitForPanther = 29,
    PetFoodCandy = 30,
    PetFoodBaitForBrownCat = 31,
    PetFoodBaitForWhiteCat = 32,
    PetFoodPumpkinMash = 33,
    PetFoodCottonCandy = 34,
    PetFoodCarrot = 35,
    PetFoodBlackberryMarmelade = 36,
    PetFoodGreenJelly = 37,
    PetFoodPinkJelly = 38,
    PetFoodYellowJelly = 39,
    PetFoodBlueJelly = 40,
    PetFoodBaitForFrightener = 41,
    PetFoodBaitForSandHorror = 42,
    PetFoodBaitForWizard = 43,
    PetFoodBaitForBandit = 44,
    PetFoodBaitForWitch = 45,
    PetFoodBaitForOgre = 46,
    PetFoodBaitForRockling = 47,
    PetFoodBaitForGnoll = 48,
    PetFoodBaitForPolarGnoll = 49,
    PetFoodBananaSplit = 50,
    PetFoodBaitForGnobold = 51,
    PetFoodBaitForInsectoid = 52,
    PetFoodPopcorn = 53,
    PetFoodBaitForInsectGuard = 54,
    PetFoodLicoriceCandy = 55,
    PetFoodCerealBar = 56,
    PetFoodSaltedCaramel = 57,
    PetFoodGingerTartlet = 58,
    PetFoodMangoJuice = 59,
    PetFoodFruitBasket = 60,
    PetFoodMelonIceCream = 61,
    PetFoodBloodOrangeJuice = 62,
    PetFoodMilkChocolateBar = 63,
    PetFoodMintChocolateBar = 64,
    PetFoodWhiteChocolateBar = 65,
    PetFoodCaramelChocolateBar = 66,
    PetFoodChocolateCookie = 67,
    PetFoodBaitForFrog = 68,
    PetFoodBaitForPlantCreature = 69,
    PetFoodBaitForRadishCreature = 70,
    PetFoodBaitForOnionling = 71,
    PetFoodBaitForDesertOnionling = 72,
    PetFoodBaitForDevourer = 73,
    PetFoodSugarCandy = 74,
    PetFoodAppleRing = 75,
    PetFoodBaitForSpikeCreature = 76,
    PetFoodBaitForAnubis = 77,
    PetFoodBaitForHorus = 78,
    PetFoodBaitForJester = 79,
    PetFoodBaitForSpectrino = 80,
    PetFoodBaitForDjinn = 81,
    PetFoodBaitForMinotaur = 82,
    PetFoodBaitForNomadMale = 83,
    PetFoodBaitForNomadFemale = 84,
    PetFoodBaitForImp = 85,
    PetFoodWaterIce = 86,
    PetFoodChocolateDonut = 87,
    PetFoodPancakes = 88,
    PetFoodBaitForKoala = 89,
    PetFoodStrawberryCake = 90,
    PetFoodChocolateCake = 91,
    PetFoodLollipop = 92,
    PetFoodSoftice = 93,
    PetFoodBaitForWerewolf = 94,
    PetFoodBaitForSanta = 95,
    PetFoodBaitForZombie = 96,
    PetFoodBaitForVampire = 97,
    PetFoodCandiedApple = 98,
    PetFoodDateCookie = 99,
    PetFoodBaitForCow = 100,
    PetFoodBaitForDragon = 101,
    PetFoodBread = 102,
    PetFoodCurry = 103,
    PetFoodLolly = 104,
    PetFoodLemonTart = 105,
    PetFoodStrawberryCocktail = 106,
    PetFoodBaitForSeaCrab = 107,
    PetFoodBaitForTroll = 108,
    PetFoodBaitForDarkTroll = 109,
    PetFoodBaitForHellDemon = 110,
    PetFoodBaitForGolem = 111,
    PetFoodBaitForEmberGolem = 112,
    PetFoodBaitForSnowGolem = 113,
    PetFoodBaitForYeti = 114,
    PetFoodBaitForCyclops = 115,
    PetFoodBaitForMammoth = 116,
    PetFoodBaitForLich = 117,
    PetFoodBaitForRuneGiant = 118,
    PetFoodBaitForSaurian = 119,
    PetFoodBaitForBush = 120,
    PetFoodBaitForSnowBush = 121,
    PetFoodBaitForSnowBerryBush = 122,
    PetFoodBaitForCottonPlant = 123,
    PetFoodBaitForScrub = 124,
    PetFoodBaitForCobwebScrub = 125,
    PetFoodBaitForFireScrub = 126,
    PetFoodBaitForGinseng = 127,
    PetFoodBaitForCactus = 128,
    PetFoodBaitForFirTree = 129,
    PetFoodBaitForThornTree = 130,
    PetFoodBaitForGoldDeposit = 131,
    PetFoodBaitForIronDeposit = 132,
    PetFoodBaitForSilverDeposit = 133,
    PetFoodBaitForSandstoneDeposit = 134,
    PetFoodBaitForEmeraldDeposit = 135,
    PetFoodBaitForSapphireDeposit = 136,
    PetFoodBaitForRubyDeposit = 137,
    PetFoodBaitForDiamondDeposit = 138,
    PetFoodBaitForIceCrystalDeposit = 139,
    PetFoodBaitForScarecrow = 140,
    PetFoodBaitForAim = 141,
    PetFoodBaitForDummy = 142,
    PetFoodBaitForVase = 143,
    PetFoodBaitForBomb = 144,
    PetFoodBaitForSapphireFish = 145,
    PetFoodBaitForLemonFish = 146,
    PetFoodBaitForSeahorse = 147,
    PetFoodBaitForMermaid = 148,
    PetFoodBaitForMerman = 149,
    PetFoodBaitForShark = 150,
    PetFoodBiscuitRole = 151,
    PetFoodBaitForLanternFish = 152,
    PetFoodBaitForMawFish = 153,
    PetFoodBaitForPiranha = 154,
    PetFoodBaitForBlowfish = 155,
};

}
