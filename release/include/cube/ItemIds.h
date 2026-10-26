// ID catalog companion to Item.h.
#pragma once

namespace cube {

// items-30 identifiers.
enum GeneratedItemId {
    GeneratedItemEquipment0 = 0, // Item::transformed; ..764 and final category 31 branch; generates rand()%3
    GeneratedItemGoldBag = 1,    // Item::transformed; ..764 and final category 31 branch; generates rand()%3
    GeneratedItemEquipment2 = 2, // Item::transformed; ..764 and final category 31 branch; generates rand()%3
};

// items-6 identifiers.
enum FeetId {
    FeetBoots = 0, // category 6; dictionary key Boots
};

// items-5 identifiers.
enum HandsId {
    HandsGloves = 0, // category 5; dictionary key Gloves
};

// items-4 identifiers.
enum ChestId {
    ChestChestArmor = 0, // category 4; dictionary key ChestArmor
    ChestBackpack =
        1, // SpriteManager::itemSprite clothing-material branch selects sprite 1437; SpriteManager::loadSprites; loads backpack.cub; id 1 returns null before material dispatch: this asset names the inner branch, not an obtainable item
    ChestBody1 =
        2, // SpriteManager::itemSprite clothing-material branch selects sprite 1438; SpriteManager::loadSprites; loads body1.cub
    ChestBody2 =
        3, // SpriteManager::itemSprite clothing-material branch selects sprite 1439; SpriteManager::loadSprites; loads body2.cub
    ChestBody3 =
        4, // SpriteManager::itemSprite clothing-material branch selects sprite 1440; SpriteManager::loadSprites; loads body3.cub
    ChestBody4 =
        5, // SpriteManager::itemSprite clothing-material branch selects sprite 1441; SpriteManager::loadSprites; loads body4.cub
    ChestBody5 =
        6, // SpriteManager::itemSprite clothing-material branch selects sprite 1442; SpriteManager::loadSprites; loads body5.cub
};

// items-7 identifiers.
enum ShoulderId {
    ShoulderShoulderArmor = 0, // category 7; dictionary key ShoulderArmor
};

// items-20 identifiers.
enum PetItemId {
    PetItemElfMale =
        0, // category 20 id 0; dictionary key Pet; generic pet entry; other pet ids carry a race; shared race ID 0 = RaceElfMale; Dictionary key: ElfMale
    PetItemElfFemale =
        1, // Item::petName / Game pet handling; id is race; Dictionary key: ElfFemale; shared race ID 1 = RaceElfFemale; Dictionary key: ElfFemale
    PetItemHumanMale =
        2, // Item::petName / Game pet handling; id is race; Dictionary key: HumanMale; shared race ID 2 = RaceHumanMale; Dictionary key: HumanMale
    PetItemHumanFemale =
        3, // Item::petName / Game pet handling; id is race; Dictionary key: HumanFemale; shared race ID 3 = RaceHumanFemale; Dictionary key: HumanFemale
    PetItemGoblinMale =
        4, // Item::petName / Game pet handling; id is race; Dictionary key: GoblinMale; shared race ID 4 = RaceGoblinMale; Dictionary key: GoblinMale
    PetItemGoblinFemale =
        5, // Item::petName / Game pet handling; id is race; Dictionary key: GoblinFemale; shared race ID 5 = RaceGoblinFemale; Dictionary key: GoblinFemale
    PetItemBullterrier =
        6, // Item::petName / Game pet handling; id is race; Dictionary key: Bullterrier; shared race ID 6 = RaceBullterrier; Dictionary key: Bullterrier
    PetItemLizardmanMale =
        7, // Item::petName / Game pet handling; id is race; Dictionary key: LizardmanMale; shared race ID 7 = RaceLizardmanMale; Dictionary key: LizardmanMale
    PetItemLizardmanFemale =
        8, // Item::petName / Game pet handling; id is race; Dictionary key: LizardmanFemale; shared race ID 8 = RaceLizardmanFemale; Dictionary key: LizardmanFemale
    PetItemDwarfMale =
        9, // Item::petName / Game pet handling; id is race; Dictionary key: DwarfMale; shared race ID 9 = RaceDwarfMale; Dictionary key: DwarfMale
    PetItemDwarfFemale =
        10, // Item::petName / Game pet handling; id is race; Dictionary key: DwarfFemale; shared race ID 10 = RaceDwarfFemale; Dictionary key: DwarfFemale
    PetItemOrcMale =
        11, // Item::petName / Game pet handling; id is race; Dictionary key: OrcMale; shared race ID 11 = RaceOrcMale; Dictionary key: OrcMale
    PetItemOrcFemale =
        12, // Item::petName / Game pet handling; id is race; Dictionary key: OrcFemale; shared race ID 12 = RaceOrcFemale; Dictionary key: OrcFemale
    PetItemFrogmanMale =
        13, // Item::petName / Game pet handling; id is race; Dictionary key: FrogmanMale; shared race ID 13 = RaceFrogmanMale; Dictionary key: FrogmanMale
    PetItemFrogmanFemale =
        14, // Item::petName / Game pet handling; id is race; Dictionary key: FrogmanFemale; shared race ID 14 = RaceFrogmanFemale; Dictionary key: FrogmanFemale
    PetItemUndeadMale =
        15, // Item::petName / Game pet handling; id is race; Dictionary key: UndeadMale; shared race ID 15 = RaceUndeadMale; Dictionary key: UndeadMale
    PetItemUndeadFemale =
        16, // Item::petName / Game pet handling; id is race; Dictionary key: UndeadFemale; shared race ID 16 = RaceUndeadFemale; Dictionary key: UndeadFemale
    PetItemSkeleton =
        17, // Item::petName / Game pet handling; id is race; Dictionary key: Skeleton; shared race ID 17 = RaceSkeleton; Dictionary key: Skeleton
    PetItemOldMan =
        18, // Item::petName / Game pet handling; id is race; Dictionary key: OldMan; shared race ID 18 = RaceOldMan; Dictionary key: OldMan
    PetItemCollie =
        19, // Item::petName / Game pet handling; id is race; Dictionary key: Collie; shared race ID 19 = RaceCollie; Dictionary key: Collie
    PetItemShepherdDog =
        20, // Item::petName / Game pet handling; id is race; Dictionary key: ShepherdDog; shared race ID 20 = RaceShepherdDog; Dictionary key: ShepherdDog
    PetItemSkullBull =
        21, // Item::petName / Game pet handling; id is race; Dictionary key: SkullBull; shared race ID 21 = RaceSkullBull; Dictionary key: SkullBull
    PetItemAlpaca =
        22, // Item::petName / Game pet handling; id is race; Dictionary key: Alpaca; shared race ID 22 = RaceAlpaca; Dictionary key: Alpaca
    PetItemBrownAlpaca =
        23, // Item::petName / Game pet handling; id is race; Dictionary key: BrownAlpaca; shared race ID 23 = RaceBrownAlpaca; Dictionary key: BrownAlpaca
    PetItemEgg =
        24, // Item::petName / Game pet handling; id is race; Dictionary key: Egg; shared race ID 24 = RaceEgg; Dictionary key: Egg
    PetItemTurtle =
        25, // Item::petName / Game pet handling; id is race; Dictionary key: Turtle; shared race ID 25 = RaceTurtle; Dictionary key: Turtle
    PetItemTerrier =
        26, // Item::petName / Game pet handling; id is race; Dictionary key: Terrier; shared race ID 26 = RaceTerrier; Dictionary key: Terrier
    PetItemScottishTerrier =
        27, // Item::petName / Game pet handling; id is race; Dictionary key: ScottishTerrier; shared race ID 27 = RaceScottishTerrier; Dictionary key: ScottishTerrier
    PetItemWolf =
        28, // Item::petName / Game pet handling; id is race; Dictionary key: Wolf; shared race ID 28 = RaceWolf; Dictionary key: Wolf
    PetItemPanther =
        29, // Item::petName / Game pet handling; id is race; Dictionary key: Panther; shared race ID 29 = RacePanther; Dictionary key: Panther
    PetItemCat =
        30, // Item::petName / Game pet handling; id is race; Dictionary key: Cat; shared race ID 30 = RaceCat; Dictionary key: Cat
    PetItemBrownCat =
        31, // Item::petName / Game pet handling; id is race; Dictionary key: BrownCat; shared race ID 31 = RaceBrownCat; Dictionary key: BrownCat
    PetItemWhiteCat =
        32, // Item::petName / Game pet handling; id is race; Dictionary key: WhiteCat; shared race ID 32 = RaceWhiteCat; Dictionary key: WhiteCat
    PetItemPig =
        33, // Item::petName / Game pet handling; id is race; Dictionary key: Pig; shared race ID 33 = RacePig; Dictionary key: Pig
    PetItemSheep =
        34, // Item::petName / Game pet handling; id is race; Dictionary key: Sheep; shared race ID 34 = RaceSheep; Dictionary key: Sheep
    PetItemBunny =
        35, // Item::petName / Game pet handling; id is race; Dictionary key: Bunny; shared race ID 35 = RaceBunny; Dictionary key: Bunny
    PetItemPorcupine =
        36, // Item::petName / Game pet handling; id is race; Dictionary key: Porcupine; shared race ID 36 = RacePorcupine; Dictionary key: Porcupine
    PetItemGreenSlime =
        37, // Item::petName / Game pet handling; id is race; Dictionary key: GreenSlime; shared race ID 37 = RaceGreenSlime; Dictionary key: GreenSlime
    PetItemPinkSlime =
        38, // Item::petName / Game pet handling; id is race; Dictionary key: PinkSlime; shared race ID 38 = RacePinkSlime; Dictionary key: PinkSlime
    PetItemYellowSlime =
        39, // Item::petName / Game pet handling; id is race; Dictionary key: YellowSlime; shared race ID 39 = RaceYellowSlime; Dictionary key: YellowSlime
    PetItemBlueSlime =
        40, // Item::petName / Game pet handling; id is race; Dictionary key: BlueSlime; shared race ID 40 = RaceBlueSlime; Dictionary key: BlueSlime
    PetItemFrightener =
        41, // Item::petName / Game pet handling; id is race; Dictionary key: Frightener; shared race ID 41 = RaceFrightener; Dictionary key: Frightener
    PetItemSandHorror =
        42, // Item::petName / Game pet handling; id is race; Dictionary key: SandHorror; shared race ID 42 = RaceSandHorror; Dictionary key: SandHorror
    PetItemWizard =
        43, // Item::petName / Game pet handling; id is race; Dictionary key: Wizard; shared race ID 43 = RaceWizard; Dictionary key: Wizard
    PetItemBandit =
        44, // Item::petName / Game pet handling; id is race; Dictionary key: Bandit; shared race ID 44 = RaceBandit; Dictionary key: Bandit
    PetItemWitch =
        45, // Item::petName / Game pet handling; id is race; Dictionary key: Witch; shared race ID 45 = RaceWitch; Dictionary key: Witch
    PetItemOgre =
        46, // Item::petName / Game pet handling; id is race; Dictionary key: Ogre; shared race ID 46 = RaceOgre; Dictionary key: Ogre
    PetItemRockling =
        47, // Item::petName / Game pet handling; id is race; Dictionary key: Rockling; shared race ID 47 = RaceRockling; Dictionary key: Rockling
    PetItemGnoll =
        48, // Item::petName / Game pet handling; id is race; Dictionary key: Gnoll; shared race ID 48 = RaceGnoll; Dictionary key: Gnoll
    PetItemPolarGnoll =
        49, // Item::petName / Game pet handling; id is race; Dictionary key: PolarGnoll; shared race ID 49 = RacePolarGnoll; Dictionary key: PolarGnoll
    PetItemMonkey =
        50, // Item::petName / Game pet handling; id is race; Dictionary key: Monkey; shared race ID 50 = RaceMonkey; Dictionary key: Monkey
    PetItemGnobold =
        51, // Item::petName / Game pet handling; id is race; Dictionary key: Gnobold; shared race ID 51 = RaceGnobold; Dictionary key: Gnobold
    PetItemInsectoid =
        52, // Item::petName / Game pet handling; id is race; Dictionary key: Insectoid; shared race ID 52 = RaceInsectoid; Dictionary key: Insectoid
    PetItemHornet =
        53, // Item::petName / Game pet handling; id is race; Dictionary key: Hornet; shared race ID 53 = RaceHornet; Dictionary key: Hornet
    PetItemInsectGuard =
        54, // Item::petName / Game pet handling; id is race; Dictionary key: InsectGuard; shared race ID 54 = RaceInsectGuard; Dictionary key: InsectGuard
    PetItemCrow =
        55, // Item::petName / Game pet handling; id is race; Dictionary key: Crow; shared race ID 55 = RaceCrow; Dictionary key: Crow
    PetItemChicken =
        56, // Item::petName / Game pet handling; id is race; Dictionary key: Chicken; shared race ID 56 = RaceChicken; Dictionary key: Chicken
    PetItemSeagull =
        57, // Item::petName / Game pet handling; id is race; Dictionary key: Seagull; shared race ID 57 = RaceSeagull; Dictionary key: Seagull
    PetItemParrot =
        58, // Item::petName / Game pet handling; id is race; Dictionary key: Parrot; shared race ID 58 = RaceParrot; Dictionary key: Parrot
    PetItemBat =
        59, // Item::petName / Game pet handling; id is race; Dictionary key: Bat; shared race ID 59 = RaceBat; Dictionary key: Bat
    PetItemFly =
        60, // Item::petName / Game pet handling; id is race; Dictionary key: Fly; shared race ID 60 = RaceFly; Dictionary key: Fly
    PetItemMidge =
        61, // Item::petName / Game pet handling; id is race; Dictionary key: Midge; shared race ID 61 = RaceMidge; Dictionary key: Midge
    PetItemMosquito =
        62, // Item::petName / Game pet handling; id is race; Dictionary key: Mosquito; shared race ID 62 = RaceMosquito; Dictionary key: Mosquito
    PetItemPlainRunner =
        63, // Item::petName / Game pet handling; id is race; Dictionary key: PlainRunner; shared race ID 63 = RacePlainRunner; Dictionary key: PlainRunner
    PetItemLeafRunner =
        64, // Item::petName / Game pet handling; id is race; Dictionary key: LeafRunner; shared race ID 64 = RaceLeafRunner; Dictionary key: LeafRunner
    PetItemSnowRunner =
        65, // Item::petName / Game pet handling; id is race; Dictionary key: SnowRunner; shared race ID 65 = RaceSnowRunner; Dictionary key: SnowRunner
    PetItemDesertRunner =
        66, // Item::petName / Game pet handling; id is race; Dictionary key: DesertRunner; shared race ID 66 = RaceDesertRunner; Dictionary key: DesertRunner
    PetItemPeacock =
        67, // Item::petName / Game pet handling; id is race; Dictionary key: Peacock; shared race ID 67 = RacePeacock; Dictionary key: Peacock
    PetItemFrog =
        68, // Item::petName / Game pet handling; id is race; Dictionary key: Frog; shared race ID 68 = RaceFrog; Dictionary key: Frog
    PetItemPlantCreature =
        69, // Item::petName / Game pet handling; id is race; Dictionary key: PlantCreature; shared race ID 69 = RacePlantCreature; Dictionary key: PlantCreature
    PetItemRadishCreature =
        70, // Item::petName / Game pet handling; id is race; Dictionary key: RadishCreature; shared race ID 70 = RaceRadishCreature; Dictionary key: RadishCreature
    PetItemOnionling =
        71, // Item::petName / Game pet handling; id is race; Dictionary key: Onionling; shared race ID 71 = RaceOnionling; Dictionary key: Onionling
    PetItemDesertOnionling =
        72, // Item::petName / Game pet handling; id is race; Dictionary key: DesertOnionling; shared race ID 72 = RaceDesertOnionling; Dictionary key: DesertOnionling
    PetItemDevourer =
        73, // Item::petName / Game pet handling; id is race; Dictionary key: Devourer; shared race ID 73 = RaceDevourer; Dictionary key: Devourer
    PetItemDuckbill =
        74, // Item::petName / Game pet handling; id is race; Dictionary key: Duckbill; shared race ID 74 = RaceDuckbill; Dictionary key: Duckbill
    PetItemCrocodile =
        75, // Item::petName / Game pet handling; id is race; Dictionary key: Crocodile; shared race ID 75 = RaceCrocodile; Dictionary key: Crocodile
    PetItemSpikeCreature =
        76, // Item::petName / Game pet handling; id is race; Dictionary key: SpikeCreature; shared race ID 76 = RaceSpikeCreature; Dictionary key: SpikeCreature
    PetItemAnubis =
        77, // Item::petName / Game pet handling; id is race; Dictionary key: Anubis; shared race ID 77 = RaceAnubis; Dictionary key: Anubis
    PetItemHorus =
        78, // Item::petName / Game pet handling; id is race; Dictionary key: Horus; shared race ID 78 = RaceHorus; Dictionary key: Horus
    PetItemJester =
        79, // Item::petName / Game pet handling; id is race; Dictionary key: Jester; shared race ID 79 = RaceJester; Dictionary key: Jester
    PetItemSpectrino =
        80, // Item::petName / Game pet handling; id is race; Dictionary key: Spectrino; shared race ID 80 = RaceSpectrino; Dictionary key: Spectrino
    PetItemDjinn =
        81, // Item::petName / Game pet handling; id is race; Dictionary key: Djinn; shared race ID 81 = RaceDjinn; Dictionary key: Djinn
    PetItemMinotaur =
        82, // Item::petName / Game pet handling; id is race; Dictionary key: Minotaur; shared race ID 82 = RaceMinotaur; Dictionary key: Minotaur
    PetItemNomadMale =
        83, // Item::petName / Game pet handling; id is race; Dictionary key: NomadMale; shared race ID 83 = RaceNomadMale; Dictionary key: NomadMale
    PetItemNomadFemale =
        84, // Item::petName / Game pet handling; id is race; Dictionary key: NomadFemale; shared race ID 84 = RaceNomadFemale; Dictionary key: NomadFemale
    PetItemImp =
        85, // Item::petName / Game pet handling; id is race; Dictionary key: Imp; shared race ID 85 = RaceImp; Dictionary key: Imp
    PetItemSpitter =
        86, // Item::petName / Game pet handling; id is race; Dictionary key: Spitter; shared race ID 86 = RaceSpitter; Dictionary key: Spitter
    PetItemMole =
        87, // Item::petName / Game pet handling; id is race; Dictionary key: Mole; shared race ID 87 = RaceMole; Dictionary key: Mole
    PetItemBiter =
        88, // Item::petName / Game pet handling; id is race; Dictionary key: Biter; shared race ID 88 = RaceBiter; Dictionary key: Biter
    PetItemKoala =
        89, // Item::petName / Game pet handling; id is race; Dictionary key: Koala; shared race ID 89 = RaceKoala; Dictionary key: Koala
    PetItemSquirrel =
        90, // Item::petName / Game pet handling; id is race; Dictionary key: Squirrel; shared race ID 90 = RaceSquirrel; Dictionary key: Squirrel
    PetItemRaccoon =
        91, // Item::petName / Game pet handling; id is race; Dictionary key: Raccoon; shared race ID 91 = RaceRaccoon; Dictionary key: Raccoon
    PetItemOwl =
        92, // Item::petName / Game pet handling; id is race; Dictionary key: Owl; shared race ID 92 = RaceOwl; Dictionary key: Owl
    PetItemPenguin =
        93, // Item::petName / Game pet handling; id is race; Dictionary key: Penguin; shared race ID 93 = RacePenguin; Dictionary key: Penguin
    PetItemWerewolf =
        94, // Item::petName / Game pet handling; id is race; Dictionary key: Werewolf; shared race ID 94 = RaceWerewolf; Dictionary key: Werewolf
    PetItemSanta = 95, // Meaning unknown.
    PetItemZombie =
        96, // Item::petName / Game pet handling; id is race; Dictionary key: Zombie; shared race ID 96 = RaceZombie; Dictionary key: Zombie
    PetItemVampire =
        97, // Item::petName / Game pet handling; id is race; Dictionary key: Vampire; shared race ID 97 = RaceVampire; Dictionary key: Vampire
    PetItemHorse =
        98, // Item::petName / Game pet handling; id is race; Dictionary key: Horse; shared race ID 98 = RaceHorse; Dictionary key: Horse
    PetItemCamel =
        99, // Item::petName / Game pet handling; id is race; Dictionary key: Camel; shared race ID 99 = RaceCamel; Dictionary key: Camel
    PetItemCow =
        100, // Item::petName / Game pet handling; id is race; Dictionary key: Cow; shared race ID 100 = RaceCow; Dictionary key: Cow
    PetItemDragon =
        101, // Item::petName / Game pet handling; id is race; Dictionary key: Dragon; shared race ID 101 = RaceDragon; Dictionary key: Dragon
    PetItemBarkBeetle =
        102, // Item::petName / Game pet handling; id is race; Dictionary key: BarkBeetle; shared race ID 102 = RaceBarkBeetle; Dictionary key: BarkBeetle
    PetItemFireBeetle =
        103, // Item::petName / Game pet handling; id is race; Dictionary key: FireBeetle; shared race ID 103 = RaceFireBeetle; Dictionary key: FireBeetle
    PetItemSnoutBeetle =
        104, // Item::petName / Game pet handling; id is race; Dictionary key: SnoutBeetle; shared race ID 104 = RaceSnoutBeetle; Dictionary key: SnoutBeetle
    PetItemLemonBeetle =
        105, // Item::petName / Game pet handling; id is race; Dictionary key: LemonBeetle; shared race ID 105 = RaceLemonBeetle; Dictionary key: LemonBeetle
    PetItemCrab =
        106, // Item::petName / Game pet handling; id is race; Dictionary key: Crab; shared race ID 106 = RaceCrab; Dictionary key: Crab
    PetItemSeaCrab =
        107, // Item::petName / Game pet handling; id is race; Dictionary key: SeaCrab; shared race ID 107 = RaceSeaCrab; Dictionary key: SeaCrab
    PetItemTroll =
        108, // Item::petName / Game pet handling; id is race; Dictionary key: Troll; shared race ID 108 = RaceTroll; Dictionary key: Troll
    PetItemDarkTroll =
        109, // Item::petName / Game pet handling; id is race; Dictionary key: DarkTroll; shared race ID 109 = RaceDarkTroll; Dictionary key: DarkTroll
    PetItemHellDemon =
        110, // Item::petName / Game pet handling; id is race; Dictionary key: HellDemon; shared race ID 110 = RaceHellDemon; Dictionary key: HellDemon
    PetItemGolem =
        111, // Item::petName / Game pet handling; id is race; Dictionary key: Golem; shared race ID 111 = RaceGolem; Dictionary key: Golem
    PetItemEmberGolem =
        112, // Item::petName / Game pet handling; id is race; Dictionary key: EmberGolem; shared race ID 112 = RaceEmberGolem; Dictionary key: EmberGolem
    PetItemSnowGolem =
        113, // Item::petName / Game pet handling; id is race; Dictionary key: SnowGolem; shared race ID 113 = RaceSnowGolem; Dictionary key: SnowGolem
    PetItemYeti =
        114, // Item::petName / Game pet handling; id is race; Dictionary key: Yeti; shared race ID 114 = RaceYeti; Dictionary key: Yeti
    PetItemCyclops =
        115, // Item::petName / Game pet handling; id is race; Dictionary key: Cyclops; shared race ID 115 = RaceCyclops; Dictionary key: Cyclops
    PetItemMammoth =
        116, // Item::petName / Game pet handling; id is race; Dictionary key: Mammoth; shared race ID 116 = RaceMammoth; Dictionary key: Mammoth
    PetItemLich =
        117, // Item::petName / Game pet handling; id is race; Dictionary key: Lich; shared race ID 117 = RaceLich; Dictionary key: Lich
    PetItemRuneGiant =
        118, // Item::petName / Game pet handling; id is race; Dictionary key: RuneGiant; shared race ID 118 = RaceRuneGiant; Dictionary key: RuneGiant
    PetItemSaurian =
        119, // Item::petName / Game pet handling; id is race; Dictionary key: Saurian; shared race ID 119 = RaceSaurian; Dictionary key: Saurian
    PetItemBush =
        120, // Item::petName / Game pet handling; id is race; Dictionary key: Bush; shared race ID 120 = RaceBush; Dictionary key: Bush
    PetItemSnowBush =
        121, // Item::petName / Game pet handling; id is race; Dictionary key: SnowBush; shared race ID 121 = RaceSnowBush; Dictionary key: SnowBush
    PetItemSnowBerryBush =
        122, // Item::petName / Game pet handling; id is race; Dictionary key: SnowBerryBush; shared race ID 122 = RaceSnowBerryBush; Dictionary key: SnowBerryBush
    PetItemCottonPlant =
        123, // Item::petName / Game pet handling; id is race; Dictionary key: CottonPlant; shared race ID 123 = RaceCottonPlant; Dictionary key: CottonPlant
    PetItemScrub =
        124, // Item::petName / Game pet handling; id is race; Dictionary key: Scrub; shared race ID 124 = RaceScrub; Dictionary key: Scrub
    PetItemCobwebScrub =
        125, // Item::petName / Game pet handling; id is race; Dictionary key: CobwebScrub; shared race ID 125 = RaceCobwebScrub; Dictionary key: CobwebScrub
    PetItemFireScrub =
        126, // Item::petName / Game pet handling; id is race; Dictionary key: FireScrub; shared race ID 126 = RaceFireScrub; Dictionary key: FireScrub
    PetItemGinseng =
        127, // Item::petName / Game pet handling; id is race; Dictionary key: Ginseng; shared race ID 127 = RaceGinseng; Dictionary key: Ginseng
    PetItemCactus =
        128, // Item::petName / Game pet handling; id is race; Dictionary key: Cactus; shared race ID 128 = RaceCactus; Dictionary key: Cactus
    PetItemFirTree = 129, // Meaning unknown.
    PetItemThornTree =
        130, // Item::petName / Game pet handling; id is race; Dictionary key: ThornTree; shared race ID 130 = RaceThornTree; Dictionary key: ThornTree
    PetItemGoldDeposit =
        131, // Item::petName / Game pet handling; id is race; Dictionary key: GoldDeposit; shared race ID 131 = RaceGoldDeposit; Dictionary key: GoldDeposit
    PetItemIronDeposit =
        132, // Item::petName / Game pet handling; id is race; Dictionary key: IronDeposit; shared race ID 132 = RaceIronDeposit; Dictionary key: IronDeposit
    PetItemSilverDeposit =
        133, // Item::petName / Game pet handling; id is race; Dictionary key: SilverDeposit; shared race ID 133 = RaceSilverDeposit; Dictionary key: SilverDeposit
    PetItemSandstoneDeposit =
        134, // Item::petName / Game pet handling; id is race; Dictionary key: SandstoneDeposit; shared race ID 134 = RaceSandstoneDeposit; Dictionary key: SandstoneDeposit
    PetItemEmeraldDeposit =
        135, // Item::petName / Game pet handling; id is race; Dictionary key: EmeraldDeposit; shared race ID 135 = RaceEmeraldDeposit; Dictionary key: EmeraldDeposit
    PetItemSapphireDeposit =
        136, // Item::petName / Game pet handling; id is race; Dictionary key: SapphireDeposit; shared race ID 136 = RaceSapphireDeposit; Dictionary key: SapphireDeposit
    PetItemRubyDeposit =
        137, // Item::petName / Game pet handling; id is race; Dictionary key: RubyDeposit; shared race ID 137 = RaceRubyDeposit; Dictionary key: RubyDeposit
    PetItemDiamondDeposit =
        138, // Item::petName / Game pet handling; id is race; Dictionary key: DiamondDeposit; shared race ID 138 = RaceDiamondDeposit; Dictionary key: DiamondDeposit
    PetItemIceCrystalDeposit =
        139, // Item::petName / Game pet handling; id is race; Dictionary key: IceCrystalDeposit; shared race ID 139 = RaceIceCrystalDeposit; Dictionary key: IceCrystalDeposit
    PetItemScarecrow =
        140, // Item::petName / Game pet handling; id is race; Dictionary key: Scarecrow; shared race ID 140 = RaceScarecrow; Dictionary key: Scarecrow
    PetItemAim =
        141, // Item::petName / Game pet handling; id is race; Dictionary key: Aim; shared race ID 141 = RaceAim; Dictionary key: Aim
    PetItemDummy =
        142, // Item::petName / Game pet handling; id is race; Dictionary key: Dummy; shared race ID 142 = RaceDummy; Dictionary key: Dummy
    PetItemVase =
        143, // Item::petName / Game pet handling; id is race; Dictionary key: Vase; shared race ID 143 = RaceVase; Dictionary key: Vase
    PetItemBomb =
        144, // Item::petName / Game pet handling; id is race; Dictionary key: Bomb; shared race ID 144 = RaceBomb; Dictionary key: Bomb
    PetItemSapphireFish =
        145, // Item::petName / Game pet handling; id is race; Dictionary key: SapphireFish; shared race ID 145 = RaceSapphireFish; Dictionary key: SapphireFish
    PetItemLemonFish =
        146, // Item::petName / Game pet handling; id is race; Dictionary key: LemonFish; shared race ID 146 = RaceLemonFish; Dictionary key: LemonFish
    PetItemSeahorse =
        147, // Item::petName / Game pet handling; id is race; Dictionary key: Seahorse; shared race ID 147 = RaceSeahorse; Dictionary key: Seahorse
    PetItemMermaid =
        148, // Item::petName / Game pet handling; id is race; Dictionary key: Mermaid; shared race ID 148 = RaceMermaid; Dictionary key: Mermaid
    PetItemMerman =
        149, // Item::petName / Game pet handling; id is race; Dictionary key: Merman; shared race ID 149 = RaceMerman; Dictionary key: Merman
    PetItemShark =
        150, // Item::petName / Game pet handling; id is race; Dictionary key: Shark; shared race ID 150 = RaceShark; Dictionary key: Shark
    PetItemBumblebee =
        151, // Item::petName / Game pet handling; id is race; Dictionary key: Bumblebee; shared race ID 151 = RaceBumblebee; Dictionary key: Bumblebee
    PetItemLanternFish =
        152, // Item::petName / Game pet handling; id is race; Dictionary key: LanternFish; shared race ID 152 = RaceLanternFish; Dictionary key: LanternFish
    PetItemMawFish =
        153, // Item::petName / Game pet handling; id is race; Dictionary key: MawFish; shared race ID 153 = RaceMawFish; Dictionary key: MawFish
    PetItemPiranha =
        154, // Item::petName / Game pet handling; id is race; Dictionary key: Piranha; shared race ID 154 = RacePiranha; Dictionary key: Piranha
    PetItemBlowfish =
        155, // Item::petName / Game pet handling; id is race; Dictionary key: Blowfish; shared race ID 155 = RaceBlowfish; Dictionary key: Blowfish
    PetItemBeaver =
        156, // Item::petName / Game pet handling; id is race; Dictionary key: Beaver; shared race ID 156 = RaceBeaver; Dictionary key: Beaver
    PetItemMoose =
        157, // Item::petName / Game pet handling; id is race; Dictionary key: Moose; shared race ID 157 = RaceMoose; Dictionary key: Moose
    PetItemZebra =
        158, // Item::petName / Game pet handling; id is race; Dictionary key: Zebra; shared race ID 158 = RaceZebra; Dictionary key: Zebra
    PetItemElephant =
        159, // Item::petName / Game pet handling; id is race; Dictionary key: Elephant; shared race ID 159 = RaceElephant; Dictionary key: Elephant
    PetItemIceling =
        160, // Item::petName / Game pet handling; id is race; Dictionary key: Iceling; shared race ID 160 = RaceIceling; Dictionary key: Iceling
    PetItemRhino =
        161, // Item::petName / Game pet handling; id is race; Dictionary key: Rhino; shared race ID 161 = RaceRhino; Dictionary key: Rhino
    PetItemOstrich =
        162, // Item::petName / Game pet handling; id is race; Dictionary key: Ostrich; shared race ID 162 = RaceOstrich; Dictionary key: Ostrich
    PetItemLionMale =
        163, // Item::petName / Game pet handling; id is race; Dictionary key: LionMale; shared race ID 163 = RaceLionMale; Dictionary key: LionMale
    PetItemLionFemale =
        164, // Item::petName / Game pet handling; id is race; Dictionary key: LionFemale; shared race ID 164 = RaceLionFemale; Dictionary key: LionFemale
    PetItemSkeletonKnight =
        165, // Item::petName / Game pet handling; id is race; Dictionary key: SkeletonKnight; shared race ID 165 = RaceSkeletonKnight; Dictionary key: SkeletonKnight
    PetItemSkeletonHorse =
        166, // Item::petName / Game pet handling; id is race; Dictionary key: SkeletonHorse; shared race ID 166 = RaceSkeletonHorse; Dictionary key: SkeletonHorse
    PetItemSorcerer =
        167, // Item::petName / Game pet handling; id is race; Dictionary key: Sorcerer; shared race ID 167 = RaceSorcerer; Dictionary key: Sorcerer
    PetItemHyena =
        168, // Item::petName / Game pet handling; id is race; Dictionary key: Hyena; shared race ID 168 = RaceHyena; Dictionary key: Hyena
    PetItemGiraffe =
        169, // Item::petName / Game pet handling; id is race; Dictionary key: Giraffe; shared race ID 169 = RaceGiraffe; Dictionary key: Giraffe
    PetItemWarthog =
        170, // Item::petName / Game pet handling; id is race; Dictionary key: Warthog; shared race ID 170 = RaceWarthog; Dictionary key: Warthog
    PetItemAntelope =
        171, // Item::petName / Game pet handling; id is race; Dictionary key: Antelope; shared race ID 171 = RaceAntelope; Dictionary key: Antelope
    PetItemMakake =
        172, // Item::petName / Game pet handling; id is race; Dictionary key: Makake; shared race ID 172 = RaceMakake; Dictionary key: Makake
    PetItemSkeletonDog =
        173, // Item::petName / Game pet handling; id is race; Dictionary key: SkeletonDog; shared race ID 173 = RaceSkeletonDog; Dictionary key: SkeletonDog
    PetItemLavaTurtle =
        174, // Item::petName / Game pet handling; id is race; Dictionary key: LavaTurtle; shared race ID 174 = RaceLavaTurtle; Dictionary key: LavaTurtle
    PetItemDemonPortal =
        175, // Item::petName / Game pet handling; id is race; Dictionary key: DemonPortal; shared race ID 175 = RaceDemonPortal; Dictionary key: DemonPortal
    PetItemSlimeWell =
        176, // Item::petName / Game pet handling; id is race; Dictionary key: SlimeWell; shared race ID 176 = RaceSlimeWell; Dictionary key: SlimeWell
    PetItemHornetsNest =
        177, // Item::petName / Game pet handling; id is race; Dictionary key: HornetsNest; shared race ID 177 = RaceHornetsNest; Dictionary key: HornetsNest
    PetItemUnholyTomb =
        178, // Item::petName / Game pet handling; id is race; Dictionary key: UnholyTomb; shared race ID 178 = RaceUnholyTomb; Dictionary key: UnholyTomb
    PetItemBonePile =
        179, // Item::petName / Game pet handling; id is race; Dictionary key: BonePile; shared race ID 179 = RaceBonePile; Dictionary key: BonePile
    PetItemMolehill =
        180, // Item::petName / Game pet handling; id is race; Dictionary key: Molehill; shared race ID 180 = RaceMolehill; Dictionary key: Molehill
    PetItemWoodChest =
        181, // Item::petName / Game pet handling; id is race; Dictionary key: WoodChest; shared race ID 181 = RaceWoodChest; Dictionary key: WoodChest
    PetItemSkullChest =
        182, // Item::petName / Game pet handling; id is race; Dictionary key: SkullChest; shared race ID 182 = RaceSkullChest; Dictionary key: SkullChest
    PetItemObsidianChest =
        183, // Item::petName / Game pet handling; id is race; Dictionary key: ObsidianChest; shared race ID 183 = RaceObsidianChest; Dictionary key: ObsidianChest
    PetItemBoneChest =
        184, // Item::petName / Game pet handling; id is race; Dictionary key: BoneChest; shared race ID 184 = RaceBoneChest; Dictionary key: BoneChest
    PetItemCampfire =
        185, // Item::petName / Game pet handling; id is race; Dictionary key: Campfire; shared race ID 185 = RaceCampfire; Dictionary key: Campfire
    PetItemTent =
        186, // Item::petName / Game pet handling; id is race; Dictionary key: Tent; shared race ID 186 = RaceTent; Dictionary key: Tent
    PetItemSleepingMat =
        187, // Item::petName / Game pet handling; id is race; Dictionary key: SleepingMat; shared race ID 187 = RaceSleepingMat; Dictionary key: SleepingMat
    PetItemBeachTowel =
        188, // Item::petName / Game pet handling; id is race; Dictionary key: BeachTowel; shared race ID 188 = RaceBeachTowel; Dictionary key: BeachTowel
    PetItemSunshade =
        189, // Item::petName / Game pet handling; id is race; Dictionary key: Sunshade; shared race ID 189 = RaceSunshade; Dictionary key: Sunshade
    PetItemSign =
        190, // Item::petName / Game pet handling; id is race; Dictionary key: Sign; shared race ID 190 = RaceSign; Dictionary key: Sign
    PetItemMantis =
        191, // Item::petName / Game pet handling; id is race; Dictionary key: Mantis; shared race ID 191 = RaceMantis; Dictionary key: Mantis
    PetItemRadishCreatureSprout =
        192, // Item::petName / Game pet handling; id is race; Dictionary key: RadishCreatureSprout; shared race ID 192 = RaceRadishCreatureSprout; Dictionary key: RadishCreatureSprout
    PetItemPlantCreatureSprout =
        193, // Item::petName / Game pet handling; id is race; Dictionary key: PlantCreatureSprout; shared race ID 193 = RacePlantCreatureSprout; Dictionary key: PlantCreatureSprout
    PetItemCage1 =
        194, // Item::petName / Game pet handling; id is race; Dictionary key: Cage1; shared race ID 194 = RaceCage1; Dictionary key: Cage1
    PetItemCage2 =
        195, // Item::petName / Game pet handling; id is race; Dictionary key: Cage2; shared race ID 195 = RaceCage2; Dictionary key: Cage2
    PetItemCage3 =
        196, // Item::petName / Game pet handling; id is race; Dictionary key: Cage3; shared race ID 196 = RaceCage3; Dictionary key: Cage3
    PetItemCage4 =
        197, // Item::petName / Game pet handling; id is race; Dictionary key: Cage4; shared race ID 197 = RaceCage4; Dictionary key: Cage4
    PetItemCauldron =
        198, // Item::petName / Game pet handling; id is race; Dictionary key: Cauldron; shared race ID 198 = RaceCauldron; Dictionary key: Cauldron
    PetItemSeeker =
        199, // Item::petName / Game pet handling; id is race; Dictionary key: Seeker; shared race ID 199 = RaceSeeker; Dictionary key: Seeker
    PetItemBabyElephant =
        200, // Item::petName / Game pet handling; id is race; Dictionary key: BabyElephant; shared race ID 200 = RaceBabyElephant; Dictionary key: BabyElephant
    PetItemBabyMammoth =
        201, // Item::petName / Game pet handling; id is race; Dictionary key: BabyMammoth; shared race ID 201 = RaceBabyMammoth; Dictionary key: BabyMammoth
    PetItemPirateMale =
        202, // Item::petName / Game pet handling; id is race; Dictionary key: PirateMale; shared race ID 202 = RacePirateMale; Dictionary key: PirateMale
    PetItemPirateFemale =
        203, // Item::petName / Game pet handling; id is race; Dictionary key: PirateFemale; shared race ID 203 = RacePirateFemale; Dictionary key: PirateFemale
    PetItemTreasure =
        204, // Item::petName / Game pet handling; id is race; Dictionary key: Treasure; shared race ID 204 = RaceTreasure; Dictionary key: Treasure
    PetItemTreasureStatue =
        205, // Item::petName / Game pet handling; id is race; Dictionary key: TreasureStatue; shared race ID 205 = RaceTreasureStatue; Dictionary key: TreasureStatue
    PetItemTreasureJewel =
        206, // Item::petName / Game pet handling; id is race; Dictionary key: TreasureJewel; shared race ID 206 = RaceTreasureJewel; Dictionary key: TreasureJewel
    PetItemTreasureVase =
        207, // Item::petName / Game pet handling; id is race; Dictionary key: TreasureVase; shared race ID 207 = RaceTreasureVase; Dictionary key: TreasureVase
    PetItemEnhancedPirateMale =
        208, // Item::petName / Game pet handling; id is race; Dictionary key: PirateMale; shared race ID 208 = RaceEnhancedPirateMale; Dictionary key: PirateMale; fillAppearance: PirateMale models plus bit 17, enlarged 1.5x base size
    PetItemGoldKnightHorse =
        209, // Item::petName / Game pet handling; id is race; Dictionary key: GoldKnightHorse; shared race ID 209 = RaceGoldKnightHorse; Dictionary key: GoldKnightHorse
    PetItemFlamingo =
        210, // Item::petName / Game pet handling; id is race; Dictionary key: Flamingo; shared race ID 210 = RaceFlamingo; Dictionary key: Flamingo
    PetItemToucan =
        211, // Item::petName / Game pet handling; id is race; Dictionary key: Toucan; shared race ID 211 = RaceToucan; Dictionary key: Toucan
    PetItemWagon =
        212, // Item::petName / Game pet handling; id is race; Dictionary key: Wagon; shared race ID 212 = RaceWagon; Dictionary key: Wagon
    PetItemSilverLion =
        213, // Item::petName / Game pet handling; id is race; Dictionary key: SilverLion; shared race ID 213 = RaceSilverLion; Dictionary key: SilverLion
    PetItemRuneCube =
        214, // Item::petName / Game pet handling; id is race; Dictionary key: RuneCube; shared race ID 214 = RaceRuneCube; Dictionary key: RuneCube
    PetItemBlueTurtle =
        215, // Item::petName / Game pet handling; id is race; Dictionary key: BlueTurtle; shared race ID 215 = RaceBlueTurtle; Dictionary key: BlueTurtle
    PetItemSnowGnobold =
        216, // Item::petName / Game pet handling; id is race; Dictionary key: SnowGnobold; shared race ID 216 = RaceSnowGnobold; Dictionary key: SnowGnobold
    PetItemSavageDog =
        217, // Item::petName / Game pet handling; id is race; Dictionary key: SavageDog; shared race ID 217 = RaceSavageDog; Dictionary key: SavageDog
    PetItemSnowLionMale =
        218, // Item::petName / Game pet handling; id is race; Dictionary key: SnowLionMale; shared race ID 218 = RaceSnowLionMale; Dictionary key: SnowLionMale
    PetItemSnowLionFemale =
        219, // Item::petName / Game pet handling; id is race; Dictionary key: SnowLionFemale; shared race ID 219 = RaceSnowLionFemale; Dictionary key: SnowLionFemale
    PetItemSnowLeopard =
        220, // Item::petName / Game pet handling; id is race; Dictionary key: SnowLeopard; shared race ID 220 = RaceSnowLeopard; Dictionary key: SnowLeopard
    PetItemBrazier =
        221, // Item::petName / Game pet handling; id is race; Dictionary key: Brazier; shared race ID 221 = RaceBrazier; Dictionary key: Brazier
    PetItemGhostBrazier =
        222, // Item::petName / Game pet handling; id is race; Dictionary key: GhostBrazier; shared race ID 222 = RaceGhostBrazier; Dictionary key: GhostBrazier
    PetItemUnholyBonfire =
        223, // Item::petName / Game pet handling; id is race; Dictionary key: UnholyBonfire; shared race ID 223 = RaceUnholyBonfire; Dictionary key: UnholyBonfire
    PetItemAlphaDog =
        224, // Item::petName / Game pet handling; id is race; Dictionary key: AlphaDog; shared race ID 224 = RaceAlphaDog; Dictionary key: AlphaDog
    PetItemFlyingEye =
        225, // Item::petName / Game pet handling; id is race; Dictionary key: FlyingEye; shared race ID 225 = RaceFlyingEye; Dictionary key: FlyingEye
    PetItemDemonMarauder =
        226, // Item::petName / Game pet handling; id is race; Dictionary key: DemonMarauder; shared race ID 226 = RaceDemonMarauder; Dictionary key: DemonMarauder
    PetItemDarkCultistMale =
        227, // Item::petName / Game pet handling; id is race; Dictionary key: DarkCultistMale; shared race ID 227 = RaceDarkCultistMale; Dictionary key: DarkCultistMale
    PetItemDarkCultistFemale =
        228, // Item::petName / Game pet handling; id is race; Dictionary key: DarkCultistFemale; shared race ID 228 = RaceDarkCultistFemale; Dictionary key: DarkCultistFemale
    PetItemCrawler =
        229, // Item::petName / Game pet handling; id is race; Dictionary key: Crawler; shared race ID 229 = RaceCrawler; Dictionary key: Crawler
    PetItemNecromancer =
        230, // Item::petName / Game pet handling; id is race; Dictionary key: Necromancer; shared race ID 230 = RaceNecromancer; Dictionary key: Necromancer
    PetItemDemonIdol =
        231, // Item::petName / Game pet handling; id is race; Dictionary key: DemonIdol; shared race ID 231 = RaceDemonIdol; Dictionary key: DemonIdol
    PetItemBloodaxeClanBanner =
        232, // Item::petName / Game pet handling; id is race; Dictionary key: BloodaxeClanBanner; shared race ID 232 = RaceBloodaxeClanBanner; Dictionary key: BloodaxeClanBanner
    PetItemStoneTablet =
        233, // Item::petName / Game pet handling; id is race; Dictionary key: StoneTablet; shared race ID 233 = RaceStoneTablet; Dictionary key: StoneTablet
    PetItemLever =
        234, // Item::petName / Game pet handling; id is race; Dictionary key: Lever; shared race ID 234 = RaceLever; Dictionary key: Lever
    PetItemLockedLever =
        235, // Item::petName / Game pet handling; id is race; Dictionary key: LockedLever; shared race ID 235 = RaceLockedLever; Dictionary key: LockedLever
    PetItemEmpireSoldierMale =
        236, // Item::petName / Game pet handling; id is race; Dictionary key: EmpireSoldierMale; shared race ID 236 = RaceEmpireSoldierMale; Dictionary key: EmpireSoldierMale
    PetItemEmpireSoldierFemale =
        237, // Item::petName / Game pet handling; id is race; Dictionary key: EmpireSoldierFemale; shared race ID 237 = RaceEmpireSoldierFemale; Dictionary key: EmpireSoldierFemale
    PetItemTombstone =
        238, // Item::petName / Game pet handling; id is race; Dictionary key: Tombstone; shared race ID 238 = RaceTombstone; Dictionary key: Tombstone
    PetItemManaPump =
        239, // Item::petName / Game pet handling; id is race; Dictionary key: ManaPump; shared race ID 239 = RaceManaPump; Dictionary key: ManaPump
    PetItemManaInductor =
        240, // Item::petName / Game pet handling; id is race; Dictionary key: ManaInductor; shared race ID 240 = RaceManaInductor; Dictionary key: ManaInductor
    PetItemManaGenerator =
        241, // Item::petName / Game pet handling; id is race; Dictionary key: ManaGenerator; shared race ID 241 = RaceManaGenerator; Dictionary key: ManaGenerator
    PetItemSack =
        242, // Item::petName / Game pet handling; id is race; Dictionary key: Sack; shared race ID 242 = RaceSack; Dictionary key: Sack
    PetItemCrate =
        243, // Item::petName / Game pet handling; id is race; Dictionary key: Crate; shared race ID 243 = RaceCrate; Dictionary key: Crate
    PetItemGate =
        244, // Item::petName / Game pet handling; id is race; Dictionary key: Gate; shared race ID 244 = RaceGate; Dictionary key: Gate
    PetItemLockedGate =
        245, // Item::petName / Game pet handling; id is race; Dictionary key: Gate; shared race ID 245 = RaceLockedGate; Dictionary key: Gate; fillAppearance uses sprite 3551 (locked-gate.cub)
    PetItemRuneGate =
        246, // Item::petName / Game pet handling; id is race; Dictionary key: Gate; shared race ID 246 = RaceRuneGate; Dictionary key: Gate; fillAppearance uses sprite 3553 (rune-gate.cub)
    PetItemStoneGate =
        247, // Item::petName / Game pet handling; id is race; Dictionary key: Gate; shared race ID 247 = RaceStoneGate; Dictionary key: Gate; fillAppearance uses sprite 3555 (stone-gate.cub)
    PetItemWall =
        248, // Item::petName / Game pet handling; id is race; Dictionary key: Wall; shared race ID 248 = RaceWall; Dictionary key: Wall
    PetItemSpiderweb =
        249, // Item::petName / Game pet handling; id is race; Dictionary key: Spiderweb; shared race ID 249 = RaceSpiderweb; Dictionary key: Spiderweb
    PetItemMagicBarrier =
        250, // Item::petName / Game pet handling; id is race; Dictionary key: MagicBarrier; shared race ID 250 = RaceMagicBarrier; Dictionary key: MagicBarrier
    PetItemGrid = 251, // Meaning unknown.
    PetItemEmpress =
        252, // Item::petName / Game pet handling; id is race; Dictionary key: Empress; shared race ID 252 = RaceEmpress; Dictionary key: Empress
    PetItemArchdruid =
        253, // Item::petName / Game pet handling; id is race; Dictionary key: Archdruid; shared race ID 253 = RaceArchdruid; Dictionary key: Archdruid
    PetItemHalfDemon =
        254, // Item::petName / Game pet handling; id is race; Dictionary key: HalfDemon; shared race ID 254 = RaceHalfDemon; Dictionary key: HalfDemon
    PetItemGhostPreacher =
        255, // Item::petName / Game pet handling; id is race; Dictionary key: GhostPreacher; shared race ID 255 = RaceGhostPreacher; Dictionary key: GhostPreacher
    PetItemLegendaryPaladin =
        256, // Item::petName / Game pet handling; id is race; Dictionary key: LegendaryPaladin; shared race ID 256 = RaceLegendaryPaladin; Dictionary key: LegendaryPaladin
    PetItemCreatorMale =
        257, // Item::petName / Game pet handling; id is race; Dictionary key: CreatorMale; shared race ID 257 = RaceCreatorMale; Dictionary key: CreatorMale
    PetItemCreatorFemale =
        258, // Item::petName / Game pet handling; id is race; Dictionary key: CreatorFemale; shared race ID 258 = RaceCreatorFemale; Dictionary key: CreatorFemale
    PetItemAngel =
        259, // Item::petName / Game pet handling; id is race; Dictionary key: Angel; shared race ID 259 = RaceAngel; Dictionary key: Angel
    PetItemEnhancedOrcMale =
        260, // Item::petName / Game pet handling; id is race; Dictionary key: OrcMale; shared race ID 260 = RaceEnhancedOrcMale; Dictionary key: OrcMale; fillAppearance: normal Orc models plus appearance bit 17, which enlarges size and boosts stats
    PetItemEnhancedOrcFemale =
        261, // Item::petName / Game pet handling; id is race; Dictionary key: OrcFemale; shared race ID 261 = RaceEnhancedOrcFemale; Dictionary key: OrcFemale; fillAppearance: normal Orc models plus appearance bit 17, which enlarges size and boosts stats
    PetItemManaMech =
        262, // Item::petName / Game pet handling; id is race; Dictionary key: ManaMech; shared race ID 262 = RaceManaMech; Dictionary key: ManaMech
    PetItemTreant =
        263, // Item::petName / Game pet handling; id is race; Dictionary key: Treant; shared race ID 263 = RaceTreant; Dictionary key: Treant
    PetItemFireRunestone =
        264, // Item::petName / Game pet handling; id is race; Dictionary key: FireRunestone; shared race ID 264 = RaceFireRunestone; Dictionary key: FireRunestone
    PetItemWaterRunestone =
        265, // Item::petName / Game pet handling; id is race; Dictionary key: WaterRunestone; shared race ID 265 = RaceWaterRunestone; Dictionary key: WaterRunestone
    PetItemIceRunestone =
        266, // Item::petName / Game pet handling; id is race; Dictionary key: IceRunestone; shared race ID 266 = RaceIceRunestone; Dictionary key: IceRunestone
    PetItemWindRunestone =
        267, // Item::petName / Game pet handling; id is race; Dictionary key: WindRunestone; shared race ID 267 = RaceWindRunestone; Dictionary key: WindRunestone
    PetItemRunePortal =
        268, // Item::petName / Game pet handling; id is race; Dictionary key: RunePortal; shared race ID 268 = RaceRunePortal; Dictionary key: RunePortal
    PetItemMonstrosity =
        269, // Item::petName / Game pet handling; id is race; Dictionary key: Monstrosity; shared race ID 269 = RaceMonstrosity; Dictionary key: Monstrosity
    PetItemSkullSlater =
        270, // Item::petName / Game pet handling; id is race; Dictionary key: SkullSlater; shared race ID 270 = RaceSkullSlater; Dictionary key: SkullSlater
    PetItemTortureRack =
        271, // Item::petName / Game pet handling; id is race; Dictionary key: TortureRack; shared race ID 271 = RaceTortureRack; Dictionary key: TortureRack
    PetItemWitchWagon =
        272, // Item::petName / Game pet handling; id is race; Dictionary key: WitchWagon; shared race ID 272 = RaceWitchWagon; Dictionary key: WitchWagon
    PetItemLegIrons =
        273, // Item::petName / Game pet handling; id is race; Dictionary key: LegIrons; shared race ID 273 = RaceLegIrons; Dictionary key: LegIrons
    PetItemPole =
        274, // Item::petName / Game pet handling; id is race; Dictionary key: Pole; shared race ID 274 = RacePole; Dictionary key: Pole
    PetItemCeilingChain =
        275, // Item::petName / Game pet handling; id is race; Dictionary key: CeilingChain; shared race ID 275 = RaceCeilingChain; Dictionary key: CeilingChain
    PetItemWraith =
        276, // Item::petName / Game pet handling; id is race; Dictionary key: Wraith; shared race ID 276 = RaceWraith; Dictionary key: Wraith
    PetItemManaDeer =
        277, // Item::petName / Game pet handling; id is race; Dictionary key: ManaDeer; shared race ID 277 = RaceManaDeer; Dictionary key: ManaDeer
    PetItemShrineOfTheLight =
        278, // Item::petName / Game pet handling; id is race; Dictionary key: ShrineOfTheLight; shared race ID 278 = RaceShrineOfTheLight; Dictionary key: ShrineOfTheLight
    PetItemPirateBoat = 279, // Meaning unknown.
    PetItemBarrel =
        280, // Item::petName / Game pet handling; id is race; Dictionary key: Barrel; shared race ID 280 = RaceBarrel; Dictionary key: Barrel
    PetItemBarrelTnt =
        281, // Item::petName / Game pet handling; id is race; Dictionary key: Barrel; shared race ID 281 = RaceBarrelTnt; Dictionary key: Barrel; fillAppearance uses sprite 3608 (barrel-tnt.cub)
    PetItemFootprints =
        282, // Item::petName / Game pet handling; id is race; Dictionary key: Footprints; shared race ID 282 = RaceFootprints; Dictionary key: Footprints
    PetItemHorseFootprints =
        283, // Item::petName / Game pet handling; id is race; Dictionary key: HorseFootprints; shared race ID 283 = RaceHorseFootprints; Dictionary key: HorseFootprints
    PetItemMonsterFootprints =
        284, // Item::petName / Game pet handling; id is race; Dictionary key: MonsterFootprints; shared race ID 284 = RaceMonsterFootprints; Dictionary key: MonsterFootprints
    PetItemWaterBubble = 285, // Meaning unknown.
    PetItemSarcophagus =
        286, // Item::petName / Game pet handling; id is race; Dictionary key: Sarcophagus; shared race ID 286 = RaceSarcophagus; Dictionary key: Sarcophagus
    PetItemChiling =
        287, // Item::petName / Game pet handling; id is race; Dictionary key: Chiling; shared race ID 287 = RaceChiling; Dictionary key: Chiling
    PetItemChilingSprout =
        288, // Item::petName / Game pet handling; id is race; Dictionary key: ChilingSprout; shared race ID 288 = RaceChilingSprout; Dictionary key: ChilingSprout
    PetItemMagicCrystal =
        289, // Item::petName / Game pet handling; id is race; Dictionary key: MagicCrystal; shared race ID 289 = RaceMagicCrystal; Dictionary key: MagicCrystal
    PetItemHabanero =
        290, // Item::petName / Game pet handling; id is race; Dictionary key: Habanero; shared race ID 290 = RaceHabanero; Dictionary key: Habanero
    PetItemBloomling =
        291, // Item::petName / Game pet handling; id is race; Dictionary key: Bloomling; shared race ID 291 = RaceBloomling; Dictionary key: Bloomling
    PetItemEarthPile =
        292, // Item::petName / Game pet handling; id is race; Dictionary key: EarthPile; shared race ID 292 = RaceEarthPile; Dictionary key: EarthPile
    PetItemCaterpillar =
        293, // Item::petName / Game pet handling; id is race; Dictionary key: Caterpillar; shared race ID 293 = RaceCaterpillar; Dictionary key: Caterpillar
    PetItemEarthCaterpillar =
        294, // Item::petName / Game pet handling; id is race; Dictionary key: EarthCaterpillar; shared race ID 294 = RaceEarthCaterpillar; Dictionary key: EarthCaterpillar
    PetItemSnail =
        295, // Item::petName / Game pet handling; id is race; Dictionary key: Snail; shared race ID 295 = RaceSnail; Dictionary key: Snail
    PetItemDarkJester =
        296, // Item::petName / Game pet handling; id is race; Dictionary key: DarkJester; shared race ID 296 = RaceDarkJester; Dictionary key: DarkJester
    PetItemSwimmingSign =
        297, // Item::petName / Game pet handling; id is race; Dictionary key: Sign; shared race ID 297 = RaceSwimmingSign; Dictionary key: Sign; fillAppearance uses sprite 3524 (swimming-sign.cub)
    PetItemPedestal =
        298, // Item::petName / Game pet handling; id is race; Dictionary key: Pedestal; shared race ID 298 = RacePedestal; Dictionary key: Pedestal
    PetItemSocket =
        299, // Item::petName / Game pet handling; id is race; Dictionary key: Socket; shared race ID 299 = RaceSocket; Dictionary key: Socket
    PetItemBirdStatue =
        300, // Item::petName / Game pet handling; id is race; Dictionary key: BirdStatue; shared race ID 300 = RaceBirdStatue; Dictionary key: BirdStatue
    PetItemMourningStatue =
        301, // Item::petName / Game pet handling; id is race; Dictionary key: MourningStatue; shared race ID 301 = RaceMourningStatue; Dictionary key: MourningStatue
    PetItemClimbingPlant = 302, // Meaning unknown.
    PetItemEagle =
        303, // Item::petName / Game pet handling; id is race; Dictionary key: Eagle; shared race ID 303 = RaceEagle; Dictionary key: Eagle
    PetItemGnome =
        304, // Item::petName / Game pet handling; id is race; Dictionary key: Gnome; shared race ID 304 = RaceGnome; Dictionary key: Gnome
    PetItemSupplierGnome =
        305, // Item::petName / Game pet handling; id is race; Dictionary key: Gnome; shared race ID 305 = RaceSupplierGnome; Dictionary key: Gnome; fillAppearance uses sprite 3647 (supplier-gnome-body.cub)
};

// items-8 identifiers.
enum NeckId {
    NeckAmulet = 0, // category 8; dictionary key Amulet
};

// items-9 identifiers.
enum RingId {
    RingRing = 0, // category 9; dictionary key Ring
};

// items-2 identifiers.
enum FormulaId {
    FormulaFormula = 0, // category 2; dictionary key Formula
};

// items-3 identifiers.
enum WeaponId {
    WeaponSword = 0,       // category 3; dictionary key Sword
    WeaponAxe = 1,         // category 3; dictionary key Axe
    WeaponMace = 2,        // category 3; dictionary key Mace
    WeaponDagger = 3,      // category 3; dictionary key Dagger
    WeaponFist = 4,        // category 3; dictionary key Fist
    WeaponLongsword = 5,   // category 3; dictionary key Longsword
    WeaponBow = 6,         // category 3; dictionary key Bow
    WeaponCrossbow = 7,    // category 3; dictionary key Crossbow
    WeaponBoomerang = 8,   // category 3; dictionary key Boomerang
    WeaponArrow = 9,       // category 3; dictionary key Arrow
    WeaponStaff = 10,      // category 3; dictionary key Staff
    WeaponWand = 11,       // category 3; dictionary key Wand
    WeaponBracelet = 12,   // category 3; dictionary key Bracelet
    WeaponShield = 13,     // category 3; dictionary key Shield
    WeaponArrows = 14,     // category 3; dictionary key Arrows
    WeaponGreatsword = 15, // category 3; dictionary key Greatsword
    WeaponGreataxe = 16,   // category 3; dictionary key Greataxe
    WeaponGreatmace = 17,  // category 3; dictionary key Greatmace
    WeaponPitchfork =
        18, // SpriteManager item/weapon selector / selects sprite 3195; SpriteManager::loadSprites; loads pitchfork.cub
    WeaponIronPickaxe =
        19, // SpriteManager item/weapon selector / selects sprite 2284; SpriteManager::loadSprites; loads iron-pickaxe.cub
    WeaponTorch = 20, // category 3; dictionary key Torch
    WeaponMagnifier =
        21, // SpriteManager item/weapon selector / selects sprite 3620; SpriteManager::loadSprites; loads magnifier.cub
    WeaponHandAttacks =
        22, // equip gives one to both hands for AppearanceFlagHandAttacks (); nextAttackSkill and nextSpecialAttackSkill then pick AbilityRightSlash/AbilityLeftSlash, unarmed strikes; itemSprite returns no model for it and World::dropLoot never drops it; no dictionary key (dict_en.xml); named after the flag
    WeaponBiteAttack =
        23, // equip assigns this bite weapon to the right hand and empties the left. Both attack selectors use AbilityClawStrike. It is two-handed, has no model or dictionary key and is never dropped as loot.
};

// items-12 identifiers.
enum CoinId {
    CoinCoin = 0, // category 12; dictionary key Coin
};

// items-13 identifiers.
enum PlatinumCoinId {
    PlatinumCoinPlatinumCoin = 0, // category 13; dictionary key PlatinumCoin
};

// items-19 identifiers.
enum CandleItemId {
    CandleItemCandle = 0, // category 19; dictionary key Candle
    CandleItemUndead = 1, // category 19; dictionary key Candle
};

// items-24 identifiers.
enum SpecialItemId {
    SpecialItemHangGlider = 1,             // category 24; dictionary key HangGlider
    SpecialItemBoat = 2,                   // category 24; dictionary key Boat
    SpecialItemReins = 3,                  // category 24; dictionary key Reins
    SpecialItemFlute = 4,                  // category 24; dictionary key Flute
    SpecialItemHarp = 5,                   // category 24; dictionary key Harp
    SpecialItemFiddle = 6,                 // category 24; dictionary key Fiddle
    SpecialItemWhistle = 7,                // category 24; dictionary key Whistle
    SpecialItemBell = 8,                   // category 24; dictionary key Bell
    SpecialItemTreasureSpirit = 9,         // category 24; dictionary key TreasureSpirit
    SpecialItemLute = 10,                  // category 24; dictionary key Lute
    SpecialItemNightingale = 11,           // category 24; dictionary key Nightingale
    SpecialItemVoidStone = 12,             // category 24; dictionary key VoidStone
    SpecialItemGateKey = 13,               // category 24; dictionary key GateKey
    SpecialItemMagicCraftingBook = 14,     // category 24; dictionary key MagicCraftingBook
    SpecialItemRareCraftingBook = 15,      // category 24; dictionary key RareCraftingBook
    SpecialItemEpicCraftingBook = 16,      // category 24; dictionary key EpicCraftingBook
    SpecialItemLegendaryCraftingBook = 17, // category 24; dictionary key LegendaryCraftingBook
    SpecialItemClimbingSpikes = 18,        // category 24; dictionary key ClimbingSpikes
};

// items-25 identifiers.
enum LampId {
    LampLamp = 0, // category 25; dictionary key Lamp
};

// items-14 identifiers.
enum ManaCubeId {
    ManaCubeManaCube = 0, // category 14; dictionary key ManaCube
};

// items-31 identifiers.
enum GoldBagId {
    GoldBagGoldBag = 0, // category 31; dictionary key GoldBag
};

// items-32 identifiers.
enum ManaSpiritId {
    ManaSpiritManaSpirit = 0, // category 32; dictionary key ManaSpirit
};

// items-21 identifiers.
enum PetFoodId {
    PetFoodBaitForElfMale =
        0, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 0 (RaceElfMale); Dictionary key: ElfMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForElfFemale =
        1, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 1 (RaceElfFemale); Dictionary key: ElfFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForHumanMale =
        2, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 2 (RaceHumanMale); Dictionary key: HumanMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForHumanFemale =
        3, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 3 (RaceHumanFemale); Dictionary key: HumanFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForGoblinMale =
        4, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 4 (RaceGoblinMale); Dictionary key: GoblinMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForGoblinFemale =
        5, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 5 (RaceGoblinFemale); Dictionary key: GoblinFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBullterrier =
        6, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 6 (RaceBullterrier); Dictionary key: Bullterrier; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForLizardmanMale =
        7, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 7 (RaceLizardmanMale); Dictionary key: LizardmanMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForLizardmanFemale =
        8, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 8 (RaceLizardmanFemale); Dictionary key: LizardmanFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDwarfMale =
        9, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 9 (RaceDwarfMale); Dictionary key: DwarfMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDwarfFemale =
        10, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 10 (RaceDwarfFemale); Dictionary key: DwarfFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForOrcMale =
        11, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 11 (RaceOrcMale); Dictionary key: OrcMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForOrcFemale =
        12, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 12 (RaceOrcFemale); Dictionary key: OrcFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForFrogmanMale =
        13, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 13 (RaceFrogmanMale); Dictionary key: FrogmanMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForFrogmanFemale =
        14, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 14 (RaceFrogmanFemale); Dictionary key: FrogmanFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForUndeadMale =
        15, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 15 (RaceUndeadMale); Dictionary key: UndeadMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForUndeadFemale =
        16, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 16 (RaceUndeadFemale); Dictionary key: UndeadFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSkeleton =
        17, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 17 (RaceSkeleton); Dictionary key: Skeleton; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForOldMan =
        18, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 18 (RaceOldMan); Dictionary key: OldMan; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBubbleGum = 19, // category 21; dictionary key BubbleGum
    PetFoodBaitForShepherdDog =
        20, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 20 (RaceShepherdDog); Dictionary key: ShepherdDog; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSkullBull =
        21, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 21 (RaceSkullBull); Dictionary key: SkullBull; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodVanillaCupcake = 22,   // category 21; dictionary key VanillaCupcake
    PetFoodChocolateCupcake = 23, // category 21; dictionary key ChocolateCupcake
    PetFoodBaitForEgg =
        24, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 24 (RaceEgg); Dictionary key: Egg; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodCinnamonRole = 25, // category 21; dictionary key CinnamonRole
    PetFoodWaffle = 26,       // category 21; dictionary key Waffle
    PetFoodCroissant = 27,    // category 21; dictionary key Croissant
    PetFoodBaitForWolf =
        28, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 28 (RaceWolf); Dictionary key: Wolf; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForPanther =
        29, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 29 (RacePanther); Dictionary key: Panther; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodCandy = 30, // category 21; dictionary key Candy
    PetFoodBaitForBrownCat =
        31, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 31 (RaceBrownCat); Dictionary key: BrownCat; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForWhiteCat =
        32, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 32 (RaceWhiteCat); Dictionary key: WhiteCat; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodPumpkinMash = 33,         // category 21; dictionary key PumpkinMash
    PetFoodCottonCandy = 34,         // category 21; dictionary key CottonCandy
    PetFoodCarrot = 35,              // category 21; dictionary key Carrot
    PetFoodBlackberryMarmelade = 36, // category 21; dictionary key BlackberryMarmelade
    PetFoodGreenJelly = 37,          // category 21; dictionary key GreenJelly
    PetFoodPinkJelly = 38,           // category 21; dictionary key PinkJelly
    PetFoodYellowJelly = 39,         // category 21; dictionary key YellowJelly
    PetFoodBlueJelly = 40,           // category 21; dictionary key BlueJelly
    PetFoodBaitForFrightener =
        41, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 41 (RaceFrightener); Dictionary key: Frightener; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSandHorror =
        42, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 42 (RaceSandHorror); Dictionary key: SandHorror; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForWizard =
        43, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 43 (RaceWizard); Dictionary key: Wizard; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBandit =
        44, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 44 (RaceBandit); Dictionary key: Bandit; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForWitch =
        45, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 45 (RaceWitch); Dictionary key: Witch; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForOgre =
        46, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 46 (RaceOgre); Dictionary key: Ogre; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForRockling =
        47, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 47 (RaceRockling); Dictionary key: Rockling; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForGnoll =
        48, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 48 (RaceGnoll); Dictionary key: Gnoll; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForPolarGnoll =
        49, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 49 (RacePolarGnoll); Dictionary key: PolarGnoll; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBananaSplit = 50, // category 21; dictionary key BananaSplit
    PetFoodBaitForGnobold =
        51, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 51 (RaceGnobold); Dictionary key: Gnobold; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForInsectoid =
        52, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 52 (RaceInsectoid); Dictionary key: Insectoid; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodPopcorn = 53, // category 21; dictionary key Popcorn
    PetFoodBaitForInsectGuard =
        54, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 54 (RaceInsectGuard); Dictionary key: InsectGuard; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodLicoriceCandy = 55,       // category 21; dictionary key LicoriceCandy
    PetFoodCerealBar = 56,           // category 21; dictionary key CerealBar
    PetFoodSaltedCaramel = 57,       // category 21; dictionary key SaltedCaramel
    PetFoodGingerTartlet = 58,       // category 21; dictionary key GingerTartlet
    PetFoodMangoJuice = 59,          // category 21; dictionary key MangoJuice
    PetFoodFruitBasket = 60,         // category 21; dictionary key FruitBasket
    PetFoodMelonIceCream = 61,       // category 21; dictionary key MelonIceCream
    PetFoodBloodOrangeJuice = 62,    // category 21; dictionary key BloodOrangeJuice
    PetFoodMilkChocolateBar = 63,    // category 21; dictionary key MilkChocolateBar
    PetFoodMintChocolateBar = 64,    // category 21; dictionary key MintChocolateBar
    PetFoodWhiteChocolateBar = 65,   // category 21; dictionary key WhiteChocolateBar
    PetFoodCaramelChocolateBar = 66, // category 21; dictionary key CaramelChocolateBar
    PetFoodChocolateCookie = 67,     // category 21; dictionary key ChocolateCookie
    PetFoodBaitForFrog =
        68, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 68 (RaceFrog); Dictionary key: Frog; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForPlantCreature =
        69, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 69 (RacePlantCreature); Dictionary key: PlantCreature; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForRadishCreature =
        70, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 70 (RaceRadishCreature); Dictionary key: RadishCreature; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForOnionling =
        71, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 71 (RaceOnionling); Dictionary key: Onionling; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDesertOnionling =
        72, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 72 (RaceDesertOnionling); Dictionary key: DesertOnionling; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDevourer =
        73, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 73 (RaceDevourer); Dictionary key: Devourer; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodSugarCandy = 74, // category 21; dictionary key SugarCandy
    PetFoodAppleRing = 75,  // category 21; dictionary key AppleRing
    PetFoodBaitForSpikeCreature =
        76, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 76 (RaceSpikeCreature); Dictionary key: SpikeCreature; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForAnubis =
        77, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 77 (RaceAnubis); Dictionary key: Anubis; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForHorus =
        78, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 78 (RaceHorus); Dictionary key: Horus; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForJester =
        79, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 79 (RaceJester); Dictionary key: Jester; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSpectrino =
        80, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 80 (RaceSpectrino); Dictionary key: Spectrino; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDjinn =
        81, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 81 (RaceDjinn); Dictionary key: Djinn; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForMinotaur =
        82, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 82 (RaceMinotaur); Dictionary key: Minotaur; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForNomadMale =
        83, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 83 (RaceNomadMale); Dictionary key: NomadMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForNomadFemale =
        84, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 84 (RaceNomadFemale); Dictionary key: NomadFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForImp =
        85, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 85 (RaceImp); Dictionary key: Imp; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodWaterIce = 86,        // category 21; dictionary key WaterIce
    PetFoodChocolateDonut = 87,  // category 21; dictionary key ChocolateDonut
    PetFoodPancakes = 88,        // category 21; dictionary key Pancakes
    PetFoodEucalyptusCandy = 89, // category 21; dictionary key EucalyptusCandy
    PetFoodStrawberryCake = 90,  // category 21; dictionary key StrawberryCake
    PetFoodChocolateCake = 91,   // category 21; dictionary key ChocolateCake
    PetFoodLollipop = 92,        // category 21; dictionary key Lollipop
    PetFoodSoftice = 93,         // category 21; dictionary key Softice
    PetFoodBaitForWerewolf =
        94, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 94 (RaceWerewolf); Dictionary key: Werewolf; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSanta =
        95, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 95 (RaceSanta); Race meaning unknown; fillAppearance uses sprite 342 (santa-head.cub); AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForZombie =
        96, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 96 (RaceZombie); Dictionary key: Zombie; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForVampire =
        97, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 97 (RaceVampire); Dictionary key: Vampire; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodCandiedApple = 98, // category 21; dictionary key CandiedApple
    PetFoodDateCookie = 99,   // category 21; dictionary key DateCookie
    PetFoodBaitForCow =
        100, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 100 (RaceCow); Dictionary key: Cow; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDragon =
        101, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 101 (RaceDragon); Dictionary key: Dragon; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBread = 102,              // category 21; dictionary key Bread
    PetFoodCurry = 103,              // category 21; dictionary key Curry
    PetFoodLolly = 104,              // category 21; dictionary key Lolly
    PetFoodLemonTart = 105,          // category 21; dictionary key LemonTart
    PetFoodStrawberryCocktail = 106, // category 21; dictionary key StrawberryCocktail
    PetFoodBaitForSeaCrab =
        107, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 107 (RaceSeaCrab); Dictionary key: SeaCrab; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForTroll =
        108, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 108 (RaceTroll); Dictionary key: Troll; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDarkTroll =
        109, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 109 (RaceDarkTroll); Dictionary key: DarkTroll; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForHellDemon =
        110, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 110 (RaceHellDemon); Dictionary key: HellDemon; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForGolem =
        111, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 111 (RaceGolem); Dictionary key: Golem; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForEmberGolem =
        112, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 112 (RaceEmberGolem); Dictionary key: EmberGolem; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSnowGolem =
        113, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 113 (RaceSnowGolem); Dictionary key: SnowGolem; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForYeti =
        114, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 114 (RaceYeti); Dictionary key: Yeti; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCyclops =
        115, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 115 (RaceCyclops); Dictionary key: Cyclops; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForMammoth =
        116, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 116 (RaceMammoth); Dictionary key: Mammoth; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForLich =
        117, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 117 (RaceLich); Dictionary key: Lich; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForRuneGiant =
        118, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 118 (RaceRuneGiant); Dictionary key: RuneGiant; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSaurian =
        119, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 119 (RaceSaurian); Dictionary key: Saurian; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBush =
        120, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 120 (RaceBush); Dictionary key: Bush; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSnowBush =
        121, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 121 (RaceSnowBush); Dictionary key: SnowBush; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSnowBerryBush =
        122, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 122 (RaceSnowBerryBush); Dictionary key: SnowBerryBush; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCottonPlant =
        123, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 123 (RaceCottonPlant); Dictionary key: CottonPlant; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForScrub =
        124, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 124 (RaceScrub); Dictionary key: Scrub; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCobwebScrub =
        125, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 125 (RaceCobwebScrub); Dictionary key: CobwebScrub; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForFireScrub =
        126, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 126 (RaceFireScrub); Dictionary key: FireScrub; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForGinseng =
        127, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 127 (RaceGinseng); Dictionary key: Ginseng; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCactus =
        128, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 128 (RaceCactus); Dictionary key: Cactus; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForFirTree =
        129, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 129 (RaceFirTree); Race meaning unknown; fillAppearance uses sprite 2817 (fir-tree.cub); AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForThornTree =
        130, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 130 (RaceThornTree); Dictionary key: ThornTree; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForGoldDeposit =
        131, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 131 (RaceGoldDeposit); Dictionary key: GoldDeposit; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForIronDeposit =
        132, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 132 (RaceIronDeposit); Dictionary key: IronDeposit; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSilverDeposit =
        133, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 133 (RaceSilverDeposit); Dictionary key: SilverDeposit; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSandstoneDeposit =
        134, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 134 (RaceSandstoneDeposit); Dictionary key: SandstoneDeposit; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForEmeraldDeposit =
        135, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 135 (RaceEmeraldDeposit); Dictionary key: EmeraldDeposit; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSapphireDeposit =
        136, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 136 (RaceSapphireDeposit); Dictionary key: SapphireDeposit; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForRubyDeposit =
        137, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 137 (RaceRubyDeposit); Dictionary key: RubyDeposit; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDiamondDeposit =
        138, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 138 (RaceDiamondDeposit); Dictionary key: DiamondDeposit; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForIceCrystalDeposit =
        139, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 139 (RaceIceCrystalDeposit); Dictionary key: IceCrystalDeposit; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForScarecrow =
        140, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 140 (RaceScarecrow); Dictionary key: Scarecrow; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForAim =
        141, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 141 (RaceAim); Dictionary key: Aim; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDummy =
        142, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 142 (RaceDummy); Dictionary key: Dummy; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForVase =
        143, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 143 (RaceVase); Dictionary key: Vase; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBomb =
        144, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 144 (RaceBomb); Dictionary key: Bomb; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSapphireFish =
        145, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 145 (RaceSapphireFish); Dictionary key: SapphireFish; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForLemonFish =
        146, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 146 (RaceLemonFish); Dictionary key: LemonFish; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSeahorse =
        147, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 147 (RaceSeahorse); Dictionary key: Seahorse; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForMermaid =
        148, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 148 (RaceMermaid); Dictionary key: Mermaid; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForMerman =
        149, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 149 (RaceMerman); Dictionary key: Merman; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForShark =
        150, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 150 (RaceShark); Dictionary key: Shark; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBiscuitRole = 151, // category 21; dictionary key BiscuitRole
    PetFoodBaitForLanternFish =
        152, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 152 (RaceLanternFish); Dictionary key: LanternFish; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForMawFish =
        153, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 153 (RaceMawFish); Dictionary key: MawFish; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForPiranha =
        154, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 154 (RacePiranha); Dictionary key: Piranha; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBlowfish =
        155, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 155 (RaceBlowfish); Dictionary key: Blowfish; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBuckhorn = 156, // category 21; dictionary key Buckhorn
    PetFoodBaitForMoose =
        157, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 157 (RaceMoose); Dictionary key: Moose; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForZebra =
        158, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 158 (RaceZebra); Dictionary key: Zebra; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForElephant =
        159, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 159 (RaceElephant); Dictionary key: Elephant; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForIceling =
        160, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 160 (RaceIceling); Dictionary key: Iceling; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForRhino =
        161, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 161 (RaceRhino); Dictionary key: Rhino; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForOstrich =
        162, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 162 (RaceOstrich); Dictionary key: Ostrich; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForLionMale =
        163, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 163 (RaceLionMale); Dictionary key: LionMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForLionFemale =
        164, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 164 (RaceLionFemale); Dictionary key: LionFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSkeletonKnight =
        165, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 165 (RaceSkeletonKnight); Dictionary key: SkeletonKnight; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSkeletonHorse =
        166, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 166 (RaceSkeletonHorse); Dictionary key: SkeletonHorse; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSorcerer =
        167, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 167 (RaceSorcerer); Dictionary key: Sorcerer; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForHyena =
        168, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 168 (RaceHyena); Dictionary key: Hyena; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForGiraffe =
        169, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 169 (RaceGiraffe); Dictionary key: Giraffe; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBananaMash = 170, // category 21; dictionary key BananaMash
    PetFoodBaitForAntelope =
        171, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 171 (RaceAntelope); Dictionary key: Antelope; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForMakake =
        172, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 172 (RaceMakake); Dictionary key: Makake; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSkeletonDog =
        173, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 173 (RaceSkeletonDog); Dictionary key: SkeletonDog; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForLavaTurtle =
        174, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 174 (RaceLavaTurtle); Dictionary key: LavaTurtle; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDemonPortal =
        175, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 175 (RaceDemonPortal); Dictionary key: DemonPortal; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSlimeWell =
        176, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 176 (RaceSlimeWell); Dictionary key: SlimeWell; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForHornetsNest =
        177, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 177 (RaceHornetsNest); Dictionary key: HornetsNest; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForUnholyTomb =
        178, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 178 (RaceUnholyTomb); Dictionary key: UnholyTomb; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBonePile =
        179, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 179 (RaceBonePile); Dictionary key: BonePile; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForMolehill =
        180, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 180 (RaceMolehill); Dictionary key: Molehill; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForWoodChest =
        181, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 181 (RaceWoodChest); Dictionary key: WoodChest; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSkullChest =
        182, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 182 (RaceSkullChest); Dictionary key: SkullChest; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForObsidianChest =
        183, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 183 (RaceObsidianChest); Dictionary key: ObsidianChest; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBoneChest =
        184, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 184 (RaceBoneChest); Dictionary key: BoneChest; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCampfire =
        185, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 185 (RaceCampfire); Dictionary key: Campfire; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForTent =
        186, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 186 (RaceTent); Dictionary key: Tent; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSleepingMat =
        187, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 187 (RaceSleepingMat); Dictionary key: SleepingMat; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBeachTowel =
        188, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 188 (RaceBeachTowel); Dictionary key: BeachTowel; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSunshade =
        189, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 189 (RaceSunshade); Dictionary key: Sunshade; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSign =
        190, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 190 (RaceSign); Dictionary key: Sign; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForMantis =
        191, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 191 (RaceMantis); Dictionary key: Mantis; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodMineralWater = 192, // category 21; dictionary key MineralWater
    PetFoodSpringWater = 193,  // category 21; dictionary key SpringWater
    PetFoodBaitForCage1 =
        194, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 194 (RaceCage1); Dictionary key: Cage1; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCage2 =
        195, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 195 (RaceCage2); Dictionary key: Cage2; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCage3 =
        196, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 196 (RaceCage3); Dictionary key: Cage3; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCage4 =
        197, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 197 (RaceCage4); Dictionary key: Cage4; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCauldron =
        198, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 198 (RaceCauldron); Dictionary key: Cauldron; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSeeker =
        199, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 199 (RaceSeeker); Dictionary key: Seeker; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodPeanut = 200,            // category 21; dictionary key Peanut
    PetFoodChocolateIceCream = 201, // category 21; dictionary key ChocolateIceCream
    PetFoodBaitForPirateMale =
        202, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 202 (RacePirateMale); Dictionary key: PirateMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForPirateFemale =
        203, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 203 (RacePirateFemale); Dictionary key: PirateFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForTreasure =
        204, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 204 (RaceTreasure); Dictionary key: Treasure; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForTreasureStatue =
        205, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 205 (RaceTreasureStatue); Dictionary key: TreasureStatue; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForTreasureJewel =
        206, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 206 (RaceTreasureJewel); Dictionary key: TreasureJewel; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForTreasureVase =
        207, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 207 (RaceTreasureVase); Dictionary key: TreasureVase; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForEnhancedPirateMale =
        208, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 208 (RaceEnhancedPirateMale); Dictionary key: PirateMale; fillAppearance: PirateMale models plus bit 17, enlarged 1.5x base size; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForGoldKnightHorse =
        209, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 209 (RaceGoldKnightHorse); Dictionary key: GoldKnightHorse; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodRaspberryJuice = 210, // category 21; dictionary key RaspberryJuice
    PetFoodBaitForToucan =
        211, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 211 (RaceToucan); Dictionary key: Toucan; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForWagon =
        212, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 212 (RaceWagon); Dictionary key: Wagon; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSilverLion =
        213, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 213 (RaceSilverLion); Dictionary key: SilverLion; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForRuneCube =
        214, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 214 (RaceRuneCube); Dictionary key: RuneCube; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBlueTurtle =
        215, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 215 (RaceBlueTurtle); Dictionary key: BlueTurtle; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSnowGnobold =
        216, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 216 (RaceSnowGnobold); Dictionary key: SnowGnobold; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSavageDog =
        217, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 217 (RaceSavageDog); Dictionary key: SavageDog; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSnowLionMale =
        218, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 218 (RaceSnowLionMale); Dictionary key: SnowLionMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSnowLionFemale =
        219, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 219 (RaceSnowLionFemale); Dictionary key: SnowLionFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSnowLeopard =
        220, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 220 (RaceSnowLeopard); Dictionary key: SnowLeopard; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBrazier =
        221, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 221 (RaceBrazier); Dictionary key: Brazier; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForGhostBrazier =
        222, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 222 (RaceGhostBrazier); Dictionary key: GhostBrazier; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForUnholyBonfire =
        223, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 223 (RaceUnholyBonfire); Dictionary key: UnholyBonfire; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForAlphaDog =
        224, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 224 (RaceAlphaDog); Dictionary key: AlphaDog; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForFlyingEye =
        225, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 225 (RaceFlyingEye); Dictionary key: FlyingEye; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDemonMarauder =
        226, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 226 (RaceDemonMarauder); Dictionary key: DemonMarauder; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDarkCultistMale =
        227, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 227 (RaceDarkCultistMale); Dictionary key: DarkCultistMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDarkCultistFemale =
        228, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 228 (RaceDarkCultistFemale); Dictionary key: DarkCultistFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCrawler =
        229, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 229 (RaceCrawler); Dictionary key: Crawler; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForNecromancer =
        230, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 230 (RaceNecromancer); Dictionary key: Necromancer; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForDemonIdol =
        231, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 231 (RaceDemonIdol); Dictionary key: DemonIdol; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBloodaxeClanBanner =
        232, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 232 (RaceBloodaxeClanBanner); Dictionary key: BloodaxeClanBanner; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForStoneTablet =
        233, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 233 (RaceStoneTablet); Dictionary key: StoneTablet; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForLever =
        234, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 234 (RaceLever); Dictionary key: Lever; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForLockedLever =
        235, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 235 (RaceLockedLever); Dictionary key: LockedLever; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForEmpireSoldierMale =
        236, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 236 (RaceEmpireSoldierMale); Dictionary key: EmpireSoldierMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForEmpireSoldierFemale =
        237, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 237 (RaceEmpireSoldierFemale); Dictionary key: EmpireSoldierFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForTombstone =
        238, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 238 (RaceTombstone); Dictionary key: Tombstone; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForManaPump =
        239, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 239 (RaceManaPump); Dictionary key: ManaPump; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForManaInductor =
        240, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 240 (RaceManaInductor); Dictionary key: ManaInductor; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForManaGenerator =
        241, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 241 (RaceManaGenerator); Dictionary key: ManaGenerator; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSack =
        242, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 242 (RaceSack); Dictionary key: Sack; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCrate =
        243, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 243 (RaceCrate); Dictionary key: Crate; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForGate =
        244, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 244 (RaceGate); Dictionary key: Gate; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForLockedGate =
        245, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 245 (RaceLockedGate); Dictionary key: Gate; fillAppearance uses sprite 3551 (locked-gate.cub); AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForRuneGate =
        246, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 246 (RaceRuneGate); Dictionary key: Gate; fillAppearance uses sprite 3553 (rune-gate.cub); AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForStoneGate =
        247, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 247 (RaceStoneGate); Dictionary key: Gate; fillAppearance uses sprite 3555 (stone-gate.cub); AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForWall =
        248, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 248 (RaceWall); Dictionary key: Wall; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSpiderweb =
        249, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 249 (RaceSpiderweb); Dictionary key: Spiderweb; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForMagicBarrier =
        250, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 250 (RaceMagicBarrier); Dictionary key: MagicBarrier; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForGrid =
        251, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 251 (RaceGrid); Race meaning unknown; fillAppearance uses sprite 3634 (grid.cub); AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForEmpress =
        252, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 252 (RaceEmpress); Dictionary key: Empress; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForArchdruid =
        253, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 253 (RaceArchdruid); Dictionary key: Archdruid; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForHalfDemon =
        254, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 254 (RaceHalfDemon); Dictionary key: HalfDemon; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForGhostPreacher =
        255, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 255 (RaceGhostPreacher); Dictionary key: GhostPreacher; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForLegendaryPaladin =
        256, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 256 (RaceLegendaryPaladin); Dictionary key: LegendaryPaladin; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCreatorMale =
        257, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 257 (RaceCreatorMale); Dictionary key: CreatorMale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCreatorFemale =
        258, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 258 (RaceCreatorFemale); Dictionary key: CreatorFemale; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForAngel =
        259, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 259 (RaceAngel); Dictionary key: Angel; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForEnhancedOrcMale =
        260, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 260 (RaceEnhancedOrcMale); Dictionary key: OrcMale; fillAppearance: normal Orc models plus appearance bit 17, which enlarges size and boosts stats; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForEnhancedOrcFemale =
        261, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 261 (RaceEnhancedOrcFemale); Dictionary key: OrcFemale; fillAppearance: normal Orc models plus appearance bit 17, which enlarges size and boosts stats; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForManaMech =
        262, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 262 (RaceManaMech); Dictionary key: ManaMech; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForTreant =
        263, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 263 (RaceTreant); Dictionary key: Treant; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForFireRunestone =
        264, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 264 (RaceFireRunestone); Dictionary key: FireRunestone; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForWaterRunestone =
        265, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 265 (RaceWaterRunestone); Dictionary key: WaterRunestone; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForIceRunestone =
        266, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 266 (RaceIceRunestone); Dictionary key: IceRunestone; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForWindRunestone =
        267, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 267 (RaceWindRunestone); Dictionary key: WindRunestone; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForRunePortal =
        268, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 268 (RaceRunePortal); Dictionary key: RunePortal; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForMonstrosity =
        269, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 269 (RaceMonstrosity); Dictionary key: Monstrosity; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSkullSlater =
        270, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 270 (RaceSkullSlater); Dictionary key: SkullSlater; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForTortureRack =
        271, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 271 (RaceTortureRack); Dictionary key: TortureRack; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForWitchWagon =
        272, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 272 (RaceWitchWagon); Dictionary key: WitchWagon; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForLegIrons =
        273, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 273 (RaceLegIrons); Dictionary key: LegIrons; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForPole =
        274, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 274 (RacePole); Dictionary key: Pole; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForCeilingChain =
        275, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 275 (RaceCeilingChain); Dictionary key: CeilingChain; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForWraith =
        276, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 276 (RaceWraith); Dictionary key: Wraith; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForManaDeer =
        277, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 277 (RaceManaDeer); Dictionary key: ManaDeer; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForShrineOfTheLight =
        278, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 278 (RaceShrineOfTheLight); Dictionary key: ShrineOfTheLight; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForPirateBoat =
        279, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 279 (RacePirateBoat); Race meaning unknown; fillAppearance uses sprite 3607 (pirate-boat.cub); AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBarrel =
        280, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 280 (RaceBarrel); Dictionary key: Barrel; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBarrelTnt =
        281, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 281 (RaceBarrelTnt); Dictionary key: Barrel; fillAppearance uses sprite 3608 (barrel-tnt.cub); AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForFootprints =
        282, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 282 (RaceFootprints); Dictionary key: Footprints; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForHorseFootprints =
        283, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 283 (RaceHorseFootprints); Dictionary key: HorseFootprints; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForMonsterFootprints =
        284, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 284 (RaceMonsterFootprints); Dictionary key: MonsterFootprints; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForWaterBubble =
        285, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 285 (RaceWaterBubble); Race meaning unknown; fillAppearance uses sprite 3616 (water-bubble.cub); AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSarcophagus =
        286, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 286 (RaceSarcophagus); Dictionary key: Sarcophagus; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForChiling =
        287, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 287 (RaceChiling); Dictionary key: Chiling; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForChilingSprout =
        288, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 288 (RaceChilingSprout); Dictionary key: ChilingSprout; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForMagicCrystal =
        289, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 289 (RaceMagicCrystal); Dictionary key: MagicCrystal; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForHabanero =
        290, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 290 (RaceHabanero); Dictionary key: Habanero; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBloomling =
        291, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 291 (RaceBloomling); Dictionary key: Bloomling; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForEarthPile =
        292, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 292 (RaceEarthPile); Dictionary key: EarthPile; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodMixedSalad = 293,     // category 21; dictionary key MixedSalad
    PetFoodRadicchioSalad = 294, // category 21; dictionary key RadicchioSalad
    PetFoodCabbageRolls = 295,   // category 21; dictionary key CabbageRolls
    PetFoodBaitForDarkJester =
        296, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 296 (RaceDarkJester); Dictionary key: DarkJester; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSwimmingSign =
        297, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 297 (RaceSwimmingSign); Dictionary key: Sign; fillAppearance uses sprite 3524 (swimming-sign.cub); AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForPedestal =
        298, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 298 (RacePedestal); Dictionary key: Pedestal; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSocket =
        299, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 299 (RaceSocket); Dictionary key: Socket; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForBirdStatue =
        300, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 300 (RaceBirdStatue); Dictionary key: BirdStatue; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForMourningStatue =
        301, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 301 (RaceMourningStatue); Dictionary key: MourningStatue; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForClimbingPlant =
        302, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 302 (RaceClimbingPlant); Race meaning unknown; fillAppearance uses sprite 3636 (climbing-plant-soil.cub); AnimationState::addSprites also stacks climbing-plant-stalk.cub (3635); AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForEagle =
        303, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 303 (RaceEagle); Dictionary key: Eagle; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForGnome =
        304, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 304 (RaceGnome); Dictionary key: Gnome; AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
    PetFoodBaitForSupplierGnome =
        305, // Bait IDs 0..305 use the Bait dictionary key unless overridden. This ID also identifies race 305 (RaceSupplierGnome); Dictionary key: Gnome; fillAppearance uses sprite 3647 (supplier-gnome-body.cub); AI::combat tests actionItem.id == entity.race for feeding; generic model is pet-box.cub (sprite 2635), no distinct food asset; BaitFor identifies the discriminator, not tamability
};

// items-1 identifiers.
enum ConsumableId {
    ConsumableCookie = 0,          // category 1; dictionary key Cookie
    ConsumableLifePotion = 1,      // category 1; dictionary key LifePotion
    ConsumableCactusPotion = 2,    // category 1; dictionary key CactusPotion
    ConsumableEssenceOfWisdom = 3, // category 1; dictionary key EssenceOfWisdom
    ConsumableGinsengSoup = 4,     // category 1; dictionary key GinsengSoup
    ConsumableUnnamedHealing5 =
        5, // observable role: effectValue gives it 200 like the life potions and soups, so writeItemInfo shows "Restores health over N seconds"; cooldown the default 10 s; no dictionary key (m_itemIdentifierMap skips 5 and 6) and the default model (SpriteMushroom); no producer in the program
    ConsumableUnnamedHealing6 = 6,    // observable role: as ConsumableUnnamedHealing5
    ConsumableSnowBerryMash = 7,      // category 1; dictionary key SnowBerryMash
    ConsumableMushroomSpit = 8,       // category 1; dictionary key MushroomSpit
    ConsumableBomb = 9,               // category 1; dictionary key Bomb
    ConsumablePineappleSlice = 10,    // category 1; dictionary key PineappleSlice
    ConsumablePumpkinMuffin = 11,     // category 1; dictionary key PumpkinMuffin
    ConsumableGreaterLifePotion = 12, // category 1; dictionary key GreaterLifePotion
    ConsumablePortalStone = 13,       // category 1; dictionary key PortalStone
    ConsumableApple = 14,             // category 1; dictionary key Apple
};

// items-28 identifiers.
enum ElixirId {
    ElixirElixirOfLife = 0,      // category 28; dictionary key ElixirOfLife
    ElixirElixirOfPower = 1,     // category 28; dictionary key ElixirOfPower
    ElixirElixirOfToughness = 2, // category 28; dictionary key ElixirOfToughness
    ElixirElixirOfSanity = 3,    // category 28; dictionary key ElixirOfSanity
    ElixirLemonade = 4,          // category 28; dictionary key Lemonade
    ElixirHotChocolate = 5,      // category 28; dictionary key HotChocolate
    ElixirGreenSmoothie = 6,     // category 28; dictionary key GreenSmoothie
};

// items-11 identifiers.
enum MaterialItemId {
    MaterialItemNugget = 0,             // category 11; dictionary key Nugget
    MaterialItemLog = 1,                // category 11; dictionary key Log
    MaterialItemFeather = 2,            // category 11; dictionary key Feather
    MaterialItemHorn = 3,               // category 11; dictionary key Horn
    MaterialItemClaw = 4,               // category 11; dictionary key Claw
    MaterialItemFiber = 5,              // category 11; dictionary key Fiber
    MaterialItemCobweb = 6,             // category 11; dictionary key Cobweb
    MaterialItemHair = 7,               // category 11; dictionary key Hair
    MaterialItemCrystal = 8,            // category 11; dictionary key Crystal
    MaterialItemYarn = 9,               // category 11; dictionary key Yarn
    MaterialItemCube = 10,              // category 11; dictionary key Cube
    MaterialItemCapsule = 11,           // category 11; dictionary key Capsule
    MaterialItemFlask = 12,             // category 11; dictionary key Flask
    MaterialItemOrb = 13,               // category 11; dictionary key Orb
    MaterialItemSpirit = 14,            // category 11; dictionary key Spirit
    MaterialItemMushroom = 15,          // category 11; dictionary key Mushroom
    MaterialItemPumpkin = 16,           // category 11; dictionary key Pumpkin
    MaterialItemPineapple = 17,         // category 11; dictionary key Pineapple
    MaterialItemRadishSlice = 18,       // category 11; dictionary key RadishSlice
    MaterialItemShimmerMushroom = 19,   // category 11; dictionary key ShimmerMushroom
    MaterialItemGinsengRoot = 20,       // category 11; dictionary key GinsengRoot
    MaterialItemOnionSlice = 21,        // category 11; dictionary key OnionSlice
    MaterialItemHeartflower = 22,       // category 11; dictionary key Heartflower
    MaterialItemPricklyPear = 23,       // category 11; dictionary key PricklyPear
    MaterialItemFrozenHeartflower = 24, // category 11; dictionary key FrozenHeartflower
    MaterialItemSoulFlower = 25,        // category 11; dictionary key SoulFlower
    MaterialItemWaterFlask = 26,        // category 11; dictionary key WaterFlask
    MaterialItemSnowBerry = 27,         // category 11; dictionary key SnowBerry
    MaterialItemMindflower = 28,        // category 11; dictionary key Mindflower
    MaterialItemManaOrchid = 29,        // category 11; dictionary key ManaOrchid
    MaterialItemSpiritflower = 30,      // category 11; dictionary key Spiritflower
    MaterialItemSalad =
        31, // SpriteManager item/weapon selector / selects sprite 3199; SpriteManager::loadSprites; loads Salad.cub
    MaterialItemEmerald = 32,    // category 11; dictionary key Emerald
    MaterialItemSapphire = 33,   // category 11; dictionary key Sapphire
    MaterialItemRuby = 34,       // category 11; dictionary key Ruby
    MaterialItemDiamond = 35,    // category 11; dictionary key Diamond
    MaterialItemDragonRoot = 36, // category 11; dictionary key DragonRoot
    MaterialItemDewdrop = 37,    // category 11; dictionary key Dewdrop
    MaterialItemSugarCube = 38,  // category 11; dictionary key SugarCube
    MaterialItemIceCube = 39,    // category 11; dictionary key IceCube
    MaterialItemLemon = 40,      // category 11; dictionary key Lemon
    MaterialItemBanana = 41,     // category 11; dictionary key Banana
    MaterialItemKale = 42,       // category 11; dictionary key Kale
    MaterialItemCocoaBean = 43,  // category 11; dictionary key CocoaBean
};

// items-16 identifiers.
enum BeakId {
    BeakBeak = 0, // category 16; dictionary key Beak
};

// items-22 identifiers.
enum QuestItemId {
    QuestItemGoldKey = 0,              // category 22; dictionary key GoldKey
    QuestItemBoneKey = 1,              // category 22; dictionary key BoneKey
    QuestItemBronzeKey = 2,            // category 22; dictionary key BronzeKey
    QuestItemSilverKey = 3,            // category 22; dictionary key SilverKey
    QuestItemSkullKey = 4,             // category 22; dictionary key SkullKey
    QuestItemIronKey = 5,              // category 22; dictionary key IronKey
    QuestItemRustyKey = 6,             // category 22; dictionary key RustyKey
    QuestItemScratchedKey = 7,         // category 22; dictionary key ScratchedKey
    QuestItemAdornedKey = 8,           // category 22; dictionary key AdornedKey
    QuestItemCopperKey = 9,            // category 22; dictionary key CopperKey
    QuestItemObsidianKey = 10,         // category 22; dictionary key ObsidianKey
    QuestItemSteelKey = 11,            // category 22; dictionary key SteelKey
    QuestItemTwistedKey = 12,          // category 22; dictionary key TwistedKey
    QuestItemSmallKey = 13,            // category 22; dictionary key SmallKey
    QuestItemHeavyKey = 14,            // category 22; dictionary key HeavyKey
    QuestItemMassiveKey = 15,          // category 22; dictionary key MassiveKey
    QuestItemWoodKey = 16,             // category 22; dictionary key WoodKey
    QuestItemStoneKey = 17,            // category 22; dictionary key StoneKey
    QuestItemTinyKey = 18,             // category 22; dictionary key TinyKey
    QuestItemDelicateKey = 19,         // category 22; dictionary key DelicateKey
    QuestItemAmulet1 = 20,             // category 22; dictionary key Amulet1
    QuestItemAmulet2 = 21,             // category 22; dictionary key Amulet2
    QuestItemJewelCase = 22,           // category 22; dictionary key JewelCase
    QuestItemItemTreasure = 23,        // category 22; dictionary key ItemTreasure
    QuestItemItemTreasureStatue = 24,  // category 22; dictionary key ItemTreasureStatue
    QuestItemItemTreasureJewel = 25,   // category 22; dictionary key ItemTreasureJewel
    QuestItemItemTreasureVase = 26,    // category 22; dictionary key ItemTreasureVase
    QuestItemHerb = 27,                // category 22; dictionary key Herb
    QuestItemFlower = 28,              // category 22; dictionary key Flower
    QuestItemLetter = 29,              // category 22; dictionary key Letter
    QuestItemDragonDrool = 30,         // category 22; dictionary key DragonDrool
    QuestItemFrogSlime = 31,           // category 22; dictionary key FrogSlime
    QuestItemGargoyleTooth = 32,       // category 22; dictionary key GargoyleTooth
    QuestItemSlimyEye = 33,            // category 22; dictionary key SlimyEye
    QuestItemBroom = 34,               // category 22; dictionary key Broom
    QuestItemAnchor = 35,              // category 22; dictionary key Anchor
    QuestItemWalkingStick = 36,        // category 22; dictionary key WalkingStick
    QuestItemTrollFingernail = 37,     // category 22; dictionary key TrollFingernail
    QuestItemSpoon = 38,               // category 22; dictionary key Spoon
    QuestItemSocks = 39,               // category 22; dictionary key Socks
    QuestItemPicnicBasket = 40,        // category 22; dictionary key PicnicBasket
    QuestItemPan = 41,                 // category 22; dictionary key Pan
    QuestItemLunchBox = 42,            // category 22; dictionary key LunchBox
    QuestItemLuckyCharm = 43,          // category 22; dictionary key LuckyCharm
    QuestItemBelt = 44,                // category 22; dictionary key Belt
    QuestItemRareCrystal = 45,         // category 22; dictionary key RareCrystal
    QuestItemFabricScrap = 46,         // category 22; dictionary key FabricScrap
    QuestItemButton = 47,              // category 22; dictionary key Button
    QuestItemFruit = 48,               // category 22; dictionary key Fruit
    QuestItemNut = 49,                 // category 22; dictionary key Nut
    QuestItemRareFiber = 50,           // category 22; dictionary key RareFiber
    QuestItemRareBlossom = 51,         // category 22; dictionary key RareBlossom
    QuestItemMagicBook = 52,           // category 22; dictionary key MagicBook
    QuestItemSpellBook = 53,           // category 22; dictionary key SpellBook
    QuestItemMagicallyLockedBook = 54, // category 22; dictionary key MagicallyLockedBook
    QuestItemBrokenWeapon = 55,        // category 22; dictionary key BrokenWeapon
    QuestItemEnchantedTimber = 56,     // category 22; dictionary key EnchantedTimber
    QuestItemGhostRemains = 57,        // category 22; dictionary key GhostRemains
    QuestItemSalve = 58,               // category 22; dictionary key Salve
    QuestItemAntivenom = 59,           // category 22; dictionary key Antivenom
    QuestItemPainkiller = 60,          // category 22; dictionary key Painkiller
    QuestItemStaminaPotion = 61,       // category 22; dictionary key StaminaPotion
    QuestItemMedicine = 62,            // category 22; dictionary key Medicine
    QuestItemWeapon =
        63, // Item::displayedItem shows it as a weapon of its formulaCategory; Renderer::collectSprites lays it like category 3; no dictionary key
    QuestItemChestArmor =
        64, // Item::displayedItem shows it as ChestChestArmor; Renderer::collectSprites lays it like category 4; no dictionary key
    QuestItemAmulet = 65,       // Item::displayedItem shows it as NeckAmulet; no dictionary key
    QuestItemRing = 66,         // Item::displayedItem shows it as RingRing; no dictionary key
    QuestItemNote = 67,         // category 22; dictionary key Note
    QuestItemScroll = 68,       // category 22; dictionary key Scroll
    QuestItemMap = 69,          // category 22; dictionary key Map
    QuestItemTablet = 70,       // category 22; dictionary key Tablet
    QuestItemBag = 71,          // category 22; dictionary key Bag
    QuestItemPouch = 72,        // category 22; dictionary key Pouch
    QuestItemEternalEmber = 73, // category 22; dictionary key EternalEmber
};

// items-23 identifiers.
enum ArtifactId : int {
    Artifact_ArtifactVase = 0,    // category 23; dictionary key ArtifactVase
    Artifact_ArtifactAmulet = 1,  // category 23; dictionary key ArtifactAmulet
    Artifact_ArtifactRing = 2,    // category 23; dictionary key ArtifactRing
    Artifact_ArtifactStone = 3,   // category 23; dictionary key ArtifactStone
    Artifact_ArtifactGem = 4,     // category 23; dictionary key ArtifactGem
    Artifact_ArtifactCrown = 5,   // category 23; dictionary key ArtifactCrown
    Artifact_ArtifactScepter = 6, // category 23; dictionary key ArtifactScepter
};

// items-15 identifiers.
enum LeftoversId {
    LeftoversLeftovers = 0, // category 15; dictionary key Leftovers
};

// items-17 identifiers.
enum PaintingId {
    PaintingPainting = 0, // category 17; dictionary key Painting
};

// Item category constants have one numeric owner in cube/Item.h.

// item-materials identifiers.
enum ItemMaterial {
    ItemMaterialNone =
        0, // Speech::createItemName and addItem suppress material words for 0; Item default and cleared equipment use material 0
    ItemMaterialIron = 1,         // getMaterialKey; dictionary key MaterialIron
    ItemMaterialWood = 2,         // getMaterialKey; dictionary key MaterialWood
    ItemMaterial3 = 3,            // Meaning unknown.
    ItemMaterialSteel = 4,        // getMaterialKey; dictionary key MaterialSteel
    ItemMaterialObsidian = 5,     // getMaterialKey; dictionary key MaterialObsidian
    ItemMaterialClothing = 6,     // getMaterialKey; dictionary key MaterialClothing
    ItemMaterialBone = 7,         // getMaterialKey; dictionary key MaterialBone
    ItemMaterialCopper = 10,      // getMaterialKey; dictionary key MaterialCopper
    ItemMaterialGold = 11,        // getMaterialKey; dictionary key MaterialGold
    ItemMaterialSilver = 12,      // getMaterialKey; dictionary key MaterialSilver
    ItemMaterialPlatinum = 13,    // getMaterialKey; dictionary key MaterialPlatinum
    ItemMaterialEmerald = 14,     // getMaterialKey; dictionary key MaterialEmerald
    ItemMaterialSapphire = 15,    // getMaterialKey; dictionary key MaterialSapphire
    ItemMaterialRuby = 16,        // getMaterialKey; dictionary key MaterialRuby
    ItemMaterialDiamond = 17,     // getMaterialKey; dictionary key MaterialDiamond
    ItemMaterialSandstone = 18,   // getMaterialKey; dictionary key MaterialSandstone
    ItemMaterialSaurian = 19,     // getMaterialKey; dictionary key MaterialSaurian
    ItemMaterialParrot = 20,      // getMaterialKey; dictionary key MaterialParrot
    ItemMaterialMammoth = 21,     // getMaterialKey; dictionary key MaterialMammoth
    ItemMaterialPlant = 22,       // getMaterialKey; dictionary key MaterialPlant
    ItemMaterialIce = 23,         // getMaterialKey; dictionary key MaterialIce
    ItemMaterialLight = 24,       // getMaterialKey; dictionary key MaterialLight
    ItemMaterialGlass = 25,       // getMaterialKey; dictionary key MaterialGlass
    ItemMaterialSilk = 26,        // getMaterialKey; dictionary key MaterialSilk
    ItemMaterialLinen = 27,       // getMaterialKey; dictionary key MaterialLinen
    ItemMaterialCotton = 28,      // getMaterialKey; dictionary key MaterialCotton
    ItemMaterialBronze = 29,      // getMaterialKey; dictionary key MaterialBronze
    ItemMaterialCalico = 30,      // getMaterialKey; dictionary key MaterialCalico
    ItemMaterialFelt = 31,        // getMaterialKey; dictionary key MaterialFelt
    ItemMaterialFlannell = 32,    // getMaterialKey; dictionary key MaterialFlannell
    ItemMaterialMail = 33,        // getMaterialKey; dictionary key MaterialMail
    ItemMaterialRivet = 34,       // getMaterialKey; dictionary key MaterialRivet
    ItemMaterialFilament = 35,    // getMaterialKey; dictionary key MaterialFilament
    ItemMaterialSpike = 36,       // getMaterialKey; dictionary key MaterialSpike
    ItemMaterialFiber = 37,       // getMaterialKey; dictionary key MaterialFiber
    ItemMaterialScale = 38,       // getMaterialKey; dictionary key MaterialScale
    ItemMaterialWeb = 39,         // getMaterialKey; dictionary key MaterialWeb
    ItemMaterialShadow = 40,      // getMaterialKey; dictionary key MaterialShadow
    ItemMaterialDark = 41,        // getMaterialKey; dictionary key MaterialDark
    ItemMaterialDemon = 42,       // getMaterialKey; dictionary key MaterialDemon
    ItemMaterialFire = 43,        // getMaterialKey; dictionary key MaterialFire
    ItemMaterialDruid = 44,       // getMaterialKey; dictionary key MaterialDruid
    ItemMaterialFireSpirit = 128, // getMaterialKey; dictionary key MaterialFire
    ItemMaterialUnholy = 129,     // getMaterialKey; dictionary key MaterialUnholy
    ItemMaterialIceSpirit = 130,  // getMaterialKey; dictionary key MaterialIce
    ItemMaterialWind = 131,       // getMaterialKey; dictionary key MaterialWind
};

// rarities identifiers.
enum ItemRarity {
    ItemRarityGrey = -1,  // rarityColor; ..6382; RGB return values
    ItemRarityWhite = 0,  // rarityColor; ..6382; RGB return values
    ItemRarityGreen = 1,  // rarityColor; ..6382; RGB return values
    ItemRarityBlue = 2,   // rarityColor; ..6382; RGB return values
    ItemRarityPurple = 3, // rarityColor; ..6382; RGB return values
    ItemRarityYellow = 4, // rarityColor; ..6382; RGB return values
};
enum {
    ItemRarityCount = ItemRarityYellow + 1
};

// items-0 identifiers.
enum NoneId {
    NoneCleared = 0, // Meaning unknown.
};

// items-26 identifiers.
enum EyePatchId {
    EyePatchHumanEyePatch =
        0, // SpriteManager::itemSprite; ..291; category 26 id 0 -> sprite 3577; loads human-eye-patch.cub
    EyePatchFrogmanEyePatch =
        1, // SpriteManager::itemSprite; ..291; category 26 id 1 -> sprite 3578; loads frogman-eye-patch.cub
    EyePatchDwarfFemaleEyePatch =
        2, // SpriteManager::itemSprite; ..291; category 26 id 2 -> sprite 3586; loads dwarf-female-eye-patch.cub
    EyePatchDwarfMaleEyePatch =
        3, // SpriteManager::itemSprite; ..291; category 26 id 3 -> sprite 3587; loads dwarf-male-eye-patch.cub
    EyePatchElfEyePatch =
        4, // SpriteManager::itemSprite; ..291; category 26 id 4 -> sprite 3588; loads elf-eye-patch.cub
    EyePatchLizardmanEyePatch =
        5, // SpriteManager::itemSprite; ..291; category 26 id 5 -> sprite 3591; loads lizardman-eye-patch.cub
    EyePatchGoblinFemaleEyePatch =
        6, // SpriteManager::itemSprite; ..291; category 26 id 6 -> sprite 3589; loads goblin-female-eye-patch.cub
    EyePatchGoblinMaleEyePatch =
        7, // SpriteManager::itemSprite; ..291; category 26 id 7 -> sprite 3590; loads goblin-male-eye-patch.cub
};

// item-adjectives-0 identifiers.
enum ItemAdjectiveRarity0 {
    ItemAdjective0Shabby = 0,    // modifier % 10; dictionary key item:shabby
    ItemAdjective0Plain = 1,     // modifier % 10; dictionary key item:plain
    ItemAdjective0Battered = 2,  // modifier % 10; dictionary key item:battered
    ItemAdjective0Artless = 3,   // modifier % 10; dictionary key item:artless
    ItemAdjective0Unwieldy = 4,  // modifier % 10; dictionary key item:unwieldy
    ItemAdjective0Used = 5,      // modifier % 10; dictionary key item:used
    ItemAdjective0Dusty = 6,     // modifier % 10; dictionary key item:dusty
    ItemAdjective0Scratched = 7, // modifier % 10; dictionary key item:scratched
    ItemAdjective0Worn = 8,      // modifier % 10; dictionary key item:worn
    ItemAdjective0Common = 9,    // modifier % 10; dictionary key item:common
};

// item-adjectives-1 identifiers.
enum ItemAdjectiveRarity1 {
    ItemAdjective1Adjusted = 0,     // modifier % 10; dictionary key item:adjusted
    ItemAdjective1Balanced = 1,     // modifier % 10; dictionary key item:balanced
    ItemAdjective1Battletested = 2, // modifier % 10; dictionary key item:battletested
    ItemAdjective1Good = 3,         // modifier % 10; dictionary key item:good
    ItemAdjective1Handmade = 4,     // modifier % 10; dictionary key item:handmade
    ItemAdjective1Fair = 5,         // modifier % 10; dictionary key item:fair
    ItemAdjective1Neat = 6,         // modifier % 10; dictionary key item:neat
    ItemAdjective1Clean = 7,        // modifier % 10; dictionary key item:clean
    ItemAdjective1Undamaged = 8,    // modifier % 10; dictionary key item:undamaged
    ItemAdjective1Flawless = 9,     // modifier % 10; dictionary key item:flawless
};

// item-adjectives-2 identifiers.
enum ItemAdjectiveRarity2 {
    ItemAdjective2Exceptional = 0,   // modifier % 10; dictionary key item:exceptional
    ItemAdjective2Polished = 1,      // modifier % 10; dictionary key item:polished
    ItemAdjective2Extraordinary = 2, // modifier % 10; dictionary key item:extraordinary
    ItemAdjective2Exquisite = 3,     // modifier % 10; dictionary key item:exquisite
    ItemAdjective2Superb = 4,        // modifier % 10; dictionary key item:superb
    ItemAdjective2Unique = 5,        // modifier % 10; dictionary key item:unique
    ItemAdjective2Handsome = 6,      // modifier % 10; dictionary key item:handsome
    ItemAdjective2Grand = 7,         // modifier % 10; dictionary key item:grand
    ItemAdjective2Magic = 8,         // modifier % 10; dictionary key item:magic
    ItemAdjective2Decorated = 9,     // modifier % 10; dictionary key item:decorated
};

// item-adjectives-3 identifiers.
enum ItemAdjectiveRarity3 {
    ItemAdjective3Exceptional2 = 0,   // modifier % 10; dictionary key item:exceptional2
    ItemAdjective3Polished2 = 1,      // modifier % 10; dictionary key item:polished2
    ItemAdjective3Extraordinary2 = 2, // modifier % 10; dictionary key item:extraordinary2
    ItemAdjective3Exquisite2 = 3,     // modifier % 10; dictionary key item:exquisite2
    ItemAdjective3Superb2 = 4,        // modifier % 10; dictionary key item:superb2
    ItemAdjective3Unique2 = 5,        // modifier % 10; dictionary key item:unique2
    ItemAdjective3Handsome2 = 6,      // modifier % 10; dictionary key item:handsome2
    ItemAdjective3Grand2 = 7,         // modifier % 10; dictionary key item:grand2
    ItemAdjective3Magic2 = 8,         // modifier % 10; dictionary key item:magic2
    ItemAdjective3Decorated2 = 9,     // modifier % 10; dictionary key item:decorated2
};

// item-adjectives-4 identifiers.
enum ItemAdjectiveRarity4 {
    ItemAdjective4Brilliant = 0,   // modifier % 10; dictionary key item:brilliant
    ItemAdjective4Shining = 1,     // modifier % 10; dictionary key item:shining
    ItemAdjective4Magnificent = 2, // modifier % 10; dictionary key item:magnificent
    ItemAdjective4Sublime = 3,     // modifier % 10; dictionary key item:sublime
    ItemAdjective4Pompous = 4,     // modifier % 10; dictionary key item:pompous
    ItemAdjective4Glorious = 5,    // modifier % 10; dictionary key item:glorious
    ItemAdjective4Splendid = 6,    // modifier % 10; dictionary key item:splendid
    ItemAdjective4Famous = 7,      // modifier % 10; dictionary key item:famous
    ItemAdjective4Legendary = 8,   // modifier % 10; dictionary key item:legendary
    ItemAdjective4Fabulous = 9,    // modifier % 10; dictionary key item:fabulous
};

// Required crafting workstations displayed by FormulaDetailsWidget: water, Spinning Wheel, Loom, Furnace, Anvil, Saw,
// Workbench and Campfire. World::generateFormula uses these values; value 2 has no known use.
enum Workstation {
    WorkstationNone = 0,
    WorkstationWater = 1,
    WorkstationUnobserved = 2,
    WorkstationFurnace = 3,
    WorkstationAnvil = 4,
    WorkstationSpinningWheel = 5,
    WorkstationLoom = 6,
    WorkstationSaw = 7,
    WorkstationWorkbench = 8,
    WorkstationCampfire = 9,
};

// This generated effect domain is distinct from the artifact model ID.
enum ArtifactEffect {
    ArtifactEffect_ClimbingSpeed = 0,
    ArtifactEffect_SwimmingSpeed = 1,
    ArtifactEffect_DivingStamina = 2,
    ArtifactEffect_RidingSpeed = 3,
    ArtifactEffect_HangGlidingSpeed = 4,
    ArtifactEffect_SailingSpeed = 5,
    ArtifactEffect_LampRadius = 6,
};

}
