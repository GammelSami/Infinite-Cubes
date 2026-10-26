// ID catalog companion to Creature.h.
#pragma once

namespace cube {

// Creature class identifiers; unnamed values keep numeric names because their meaning is unknown.
enum ClassType {
    ClassNone = 0, // no class: equip draws 1..4 for it
    ClassWarrior = 1,
    ClassRanger = 2,
    ClassMage = 3,
    ClassRogue = 4,
    ClassItemVendor = 128,            // "[Item Vendor]"; village plot 3
    ClassWeaponVendor = 129,          // "[Weapon Vendor]"; plot 5
    ClassArmorVendor = 130,           // " [Armor Vendor]"; plot 4
    ClassAnalyzer = 131,              // "[Analyzer]"; plot 6 (identifier)
    ClassInnkeeper = 132,             // "[Innkeeper]"; plot 2
    ClassSmith = 133,                 // "[Smith]"; plot 7
    ClassCarpenter = 134,             // "[Carpenter]"; plot 8
    ClassTailor = 135,                // "[Tailor]"; plot 9
    ClassArchaeologist = 136,         // "[Archaeologist]"
    ClassAlchemist = 137,             // "[Alchemist]"
    ClassVillager = 138,              // Zone::generate's resident default and wanderers; a place's namesake
    Class_139 = 139,                  // clothed, and signed with sprite 3503 like 155
    ClassOrderOfTheLightMember = 141, // Zone::update, realm count 1 (FactionOrderOfTheLight)
    ClassObsidianKnight = 142,        // realm count 2 (FactionObsidianKnights); equip obsidian armor
    ClassDruidOfMana = 143,           // realm count 8 (FactionDruidsOfMana); equip druid chest, wooden staff
    Class_144 = 144,                  // equip gives a light-material chest
    Class_145 = 145,    // Monster class using abilities 7 and 8 for plant creatures; its further meaning is unknown.
    ClassMonster = 146, // equip's melee class of most monster races; abilities 7/8
    Class_147 =
        147, // Monster class using abilities 2 and 3 for radish creatures and biters; its further meaning is unknown.
    Class_148 = 148, // Monster class using abilities 66 and 67 for ogres; its further meaning is unknown.
    ClassMonsterGunner =
        149, // Ranged monster class, including snout beetles; fires ProjectileBullet rather than ProjectileArrow.
    ClassMonsterArcher =
        150, // Ranged class firing ProjectileArrow, with AbilityChargedShotRelease as its special attack. equip assigns it to no race.
    ClassMonsterBeamCaster = 151, // Caster using AbilityFireBeam and AbilityFireRay.
    ClassMonsterCaster = 152,     // isCaster; Firebolt, Waterbolt for specialization 1 (spitter)
    ClassSpawner = 153,           // equip's summoning races (demon portal, slime well, hornets' nest, ...)
    ClassFlightMaster = 154,      // "[Flight Master]"; plot 10
    ClassGuildReceptionist = 155, // "[Guild Receptionist]"; plot 11
    ClassGemTrader = 156,         // "[Gem Trader]"
    ClassShopSupplier = 157,      // "[Shop Supplier]"
    ClassMasterSailor = 159,      // "[Master Sailor]"
    ClassMasterGlider = 160,      // "[Master Glider]"
    ClassArenaChampion = 161      // the arena podium's resident; areEnemies and canDoDamage special-case it
};

// EntityData::specialization, per class. Values repeat across classes: a use names the specialization of the class its
// code tests.
enum Specialization {
    SpecializationBerserker = 0, // ClassWarrior
    SpecializationGuardian = 1,
    SpecializationSniper = 0, // ClassRanger
    SpecializationScout = 1,
    SpecializationFire = 0, // ClassMage
    SpecializationWater = 1,
    SpecializationAssassin = 0, // ClassRogue
    SpecializationNinja = 1,
    Specialization2 = 2 // equip gives it to classless wizards, witches, necromancers, seekers
};

// Creature race identifiers.
enum RaceId {
    RaceFamilyGhost = -100000,             // Dictionary key: Ghost
    RaceFamilyKnight = -99999,             // Dictionary key: Knight
    RaceFamilyDruid = -99998,              // Dictionary key: Druid
    RaceFamilyDemon = -99997,              // Dictionary key: Demon
    RaceFamilySteelEmpireForce = -99996,   // Dictionary key: SteelEmpireForce
    RaceFamilyCultOfDoomAdherer = -99995,  // Dictionary key: CultOfDoomAdherer
    RaceFamilyUnholyPactFollower = -99994, // Dictionary key: UnholyPactFollower
    RaceFamilyBandit = -99993,             // Dictionary key: Bandit
    RaceFamilyAssassin = -99992,           // Dictionary key: Assassin
    RaceFamilyGoods = -99991,              // Dictionary key: Goods
    RaceNone =
        -1, // no race: randomName takes the default syllables for it; metroNameVariant returns it for the other place types; WorldMap's death marker (the tombstone) carries it
    RaceElfMale = 0,                // Dictionary key: ElfMale
    RaceElfFemale = 1,              // Dictionary key: ElfFemale
    RaceHumanMale = 2,              // Dictionary key: HumanMale
    RaceHumanFemale = 3,            // Dictionary key: HumanFemale
    RaceGoblinMale = 4,             // Dictionary key: GoblinMale
    RaceGoblinFemale = 5,           // Dictionary key: GoblinFemale
    RaceBullterrier = 6,            // Dictionary key: Bullterrier
    RaceLizardmanMale = 7,          // Dictionary key: LizardmanMale
    RaceLizardmanFemale = 8,        // Dictionary key: LizardmanFemale
    RaceDwarfMale = 9,              // Dictionary key: DwarfMale
    RaceDwarfFemale = 10,           // Dictionary key: DwarfFemale
    RaceOrcMale = 11,               // Dictionary key: OrcMale
    RaceOrcFemale = 12,             // Dictionary key: OrcFemale
    RaceFrogmanMale = 13,           // Dictionary key: FrogmanMale
    RaceFrogmanFemale = 14,         // Dictionary key: FrogmanFemale
    RaceUndeadMale = 15,            // Dictionary key: UndeadMale
    RaceUndeadFemale = 16,          // Dictionary key: UndeadFemale
    RaceSkeleton = 17,              // Dictionary key: Skeleton
    RaceOldMan = 18,                // Dictionary key: OldMan
    RaceCollie = 19,                // Dictionary key: Collie
    RaceShepherdDog = 20,           // Dictionary key: ShepherdDog
    RaceSkullBull = 21,             // Dictionary key: SkullBull
    RaceAlpaca = 22,                // Dictionary key: Alpaca
    RaceBrownAlpaca = 23,           // Dictionary key: BrownAlpaca
    RaceEgg = 24,                   // Dictionary key: Egg
    RaceTurtle = 25,                // Dictionary key: Turtle
    RaceTerrier = 26,               // Dictionary key: Terrier
    RaceScottishTerrier = 27,       // Dictionary key: ScottishTerrier
    RaceWolf = 28,                  // Dictionary key: Wolf
    RacePanther = 29,               // Dictionary key: Panther
    RaceCat = 30,                   // Dictionary key: Cat
    RaceBrownCat = 31,              // Dictionary key: BrownCat
    RaceWhiteCat = 32,              // Dictionary key: WhiteCat
    RacePig = 33,                   // Dictionary key: Pig
    RaceSheep = 34,                 // Dictionary key: Sheep
    RaceBunny = 35,                 // Dictionary key: Bunny
    RacePorcupine = 36,             // Dictionary key: Porcupine
    RaceGreenSlime = 37,            // Dictionary key: GreenSlime
    RacePinkSlime = 38,             // Dictionary key: PinkSlime
    RaceYellowSlime = 39,           // Dictionary key: YellowSlime
    RaceBlueSlime = 40,             // Dictionary key: BlueSlime
    RaceFrightener = 41,            // Dictionary key: Frightener
    RaceSandHorror = 42,            // Dictionary key: SandHorror
    RaceWizard = 43,                // Dictionary key: Wizard
    RaceBandit = 44,                // Dictionary key: Bandit
    RaceWitch = 45,                 // Dictionary key: Witch
    RaceOgre = 46,                  // Dictionary key: Ogre
    RaceRockling = 47,              // Dictionary key: Rockling
    RaceGnoll = 48,                 // Dictionary key: Gnoll
    RacePolarGnoll = 49,            // Dictionary key: PolarGnoll
    RaceMonkey = 50,                // Dictionary key: Monkey
    RaceGnobold = 51,               // Dictionary key: Gnobold
    RaceInsectoid = 52,             // Dictionary key: Insectoid
    RaceHornet = 53,                // Dictionary key: Hornet
    RaceInsectGuard = 54,           // Dictionary key: InsectGuard
    RaceCrow = 55,                  // Dictionary key: Crow
    RaceChicken = 56,               // Dictionary key: Chicken
    RaceSeagull = 57,               // Dictionary key: Seagull
    RaceParrot = 58,                // Dictionary key: Parrot
    RaceBat = 59,                   // Dictionary key: Bat
    RaceFly = 60,                   // Dictionary key: Fly
    RaceMidge = 61,                 // Dictionary key: Midge
    RaceMosquito = 62,              // Dictionary key: Mosquito
    RacePlainRunner = 63,           // Dictionary key: PlainRunner
    RaceLeafRunner = 64,            // Dictionary key: LeafRunner
    RaceSnowRunner = 65,            // Dictionary key: SnowRunner
    RaceDesertRunner = 66,          // Dictionary key: DesertRunner
    RacePeacock = 67,               // Dictionary key: Peacock
    RaceFrog = 68,                  // Dictionary key: Frog
    RacePlantCreature = 69,         // Dictionary key: PlantCreature
    RaceRadishCreature = 70,        // Dictionary key: RadishCreature
    RaceOnionling = 71,             // Dictionary key: Onionling
    RaceDesertOnionling = 72,       // Dictionary key: DesertOnionling
    RaceDevourer = 73,              // Dictionary key: Devourer
    RaceDuckbill = 74,              // Dictionary key: Duckbill
    RaceCrocodile = 75,             // Dictionary key: Crocodile
    RaceSpikeCreature = 76,         // Dictionary key: SpikeCreature
    RaceAnubis = 77,                // Dictionary key: Anubis
    RaceHorus = 78,                 // Dictionary key: Horus
    RaceJester = 79,                // Dictionary key: Jester
    RaceSpectrino = 80,             // Dictionary key: Spectrino
    RaceDjinn = 81,                 // Dictionary key: Djinn
    RaceMinotaur = 82,              // Dictionary key: Minotaur
    RaceNomadMale = 83,             // Dictionary key: NomadMale
    RaceNomadFemale = 84,           // Dictionary key: NomadFemale
    RaceImp = 85,                   // Dictionary key: Imp
    RaceSpitter = 86,               // Dictionary key: Spitter
    RaceMole = 87,                  // Dictionary key: Mole
    RaceBiter = 88,                 // Dictionary key: Biter
    RaceKoala = 89,                 // Dictionary key: Koala
    RaceSquirrel = 90,              // Dictionary key: Squirrel
    RaceRaccoon = 91,               // Dictionary key: Raccoon
    RaceOwl = 92,                   // Dictionary key: Owl
    RacePenguin = 93,               // Dictionary key: Penguin
    RaceWerewolf = 94,              // Dictionary key: Werewolf
    RaceSanta = 95,                 // fillAppearance uses sprite 342 (santa-head.cub)
    RaceZombie = 96,                // Dictionary key: Zombie
    RaceVampire = 97,               // Dictionary key: Vampire
    RaceHorse = 98,                 // Dictionary key: Horse
    RaceCamel = 99,                 // Dictionary key: Camel
    RaceCow = 100,                  // Dictionary key: Cow
    RaceDragon = 101,               // Dictionary key: Dragon
    RaceBarkBeetle = 102,           // Dictionary key: BarkBeetle
    RaceFireBeetle = 103,           // Dictionary key: FireBeetle
    RaceSnoutBeetle = 104,          // Dictionary key: SnoutBeetle
    RaceLemonBeetle = 105,          // Dictionary key: LemonBeetle
    RaceCrab = 106,                 // Dictionary key: Crab
    RaceSeaCrab = 107,              // Dictionary key: SeaCrab
    RaceTroll = 108,                // Dictionary key: Troll
    RaceDarkTroll = 109,            // Dictionary key: DarkTroll
    RaceHellDemon = 110,            // Dictionary key: HellDemon
    RaceGolem = 111,                // Dictionary key: Golem
    RaceEmberGolem = 112,           // Dictionary key: EmberGolem
    RaceSnowGolem = 113,            // Dictionary key: SnowGolem
    RaceYeti = 114,                 // Dictionary key: Yeti
    RaceCyclops = 115,              // Dictionary key: Cyclops
    RaceMammoth = 116,              // Dictionary key: Mammoth
    RaceLich = 117,                 // Dictionary key: Lich
    RaceRuneGiant = 118,            // Dictionary key: RuneGiant
    RaceSaurian = 119,              // Dictionary key: Saurian
    RaceBush = 120,                 // Dictionary key: Bush
    RaceSnowBush = 121,             // Dictionary key: SnowBush
    RaceSnowBerryBush = 122,        // Dictionary key: SnowBerryBush
    RaceCottonPlant = 123,          // Dictionary key: CottonPlant
    RaceScrub = 124,                // Dictionary key: Scrub
    RaceCobwebScrub = 125,          // Dictionary key: CobwebScrub
    RaceFireScrub = 126,            // Dictionary key: FireScrub
    RaceGinseng = 127,              // Dictionary key: Ginseng
    RaceCactus = 128,               // Dictionary key: Cactus
    RaceFirTree = 129,              // fillAppearance uses sprite 2817 (fir-tree.cub)
    RaceThornTree = 130,            // Dictionary key: ThornTree
    RaceGoldDeposit = 131,          // Dictionary key: GoldDeposit
    RaceIronDeposit = 132,          // Dictionary key: IronDeposit
    RaceSilverDeposit = 133,        // Dictionary key: SilverDeposit
    RaceSandstoneDeposit = 134,     // Dictionary key: SandstoneDeposit
    RaceEmeraldDeposit = 135,       // Dictionary key: EmeraldDeposit
    RaceSapphireDeposit = 136,      // Dictionary key: SapphireDeposit
    RaceRubyDeposit = 137,          // Dictionary key: RubyDeposit
    RaceDiamondDeposit = 138,       // Dictionary key: DiamondDeposit
    RaceIceCrystalDeposit = 139,    // Dictionary key: IceCrystalDeposit
    RaceScarecrow = 140,            // Dictionary key: Scarecrow
    RaceAim = 141,                  // Dictionary key: Aim
    RaceDummy = 142,                // Dictionary key: Dummy
    RaceVase = 143,                 // Dictionary key: Vase
    RaceBomb = 144,                 // Dictionary key: Bomb
    RaceSapphireFish = 145,         // Dictionary key: SapphireFish
    RaceLemonFish = 146,            // Dictionary key: LemonFish
    RaceSeahorse = 147,             // Dictionary key: Seahorse
    RaceMermaid = 148,              // Dictionary key: Mermaid
    RaceMerman = 149,               // Dictionary key: Merman
    RaceShark = 150,                // Dictionary key: Shark
    RaceBumblebee = 151,            // Dictionary key: Bumblebee
    RaceLanternFish = 152,          // Dictionary key: LanternFish
    RaceMawFish = 153,              // Dictionary key: MawFish
    RacePiranha = 154,              // Dictionary key: Piranha
    RaceBlowfish = 155,             // Dictionary key: Blowfish
    RaceBeaver = 156,               // Dictionary key: Beaver
    RaceMoose = 157,                // Dictionary key: Moose
    RaceZebra = 158,                // Dictionary key: Zebra
    RaceElephant = 159,             // Dictionary key: Elephant
    RaceIceling = 160,              // Dictionary key: Iceling
    RaceRhino = 161,                // Dictionary key: Rhino
    RaceOstrich = 162,              // Dictionary key: Ostrich
    RaceLionMale = 163,             // Dictionary key: LionMale
    RaceLionFemale = 164,           // Dictionary key: LionFemale
    RaceSkeletonKnight = 165,       // Dictionary key: SkeletonKnight
    RaceSkeletonHorse = 166,        // Dictionary key: SkeletonHorse
    RaceSorcerer = 167,             // Dictionary key: Sorcerer
    RaceHyena = 168,                // Dictionary key: Hyena
    RaceGiraffe = 169,              // Dictionary key: Giraffe
    RaceWarthog = 170,              // Dictionary key: Warthog
    RaceAntelope = 171,             // Dictionary key: Antelope
    RaceMakake = 172,               // Dictionary key: Makake
    RaceSkeletonDog = 173,          // Dictionary key: SkeletonDog
    RaceLavaTurtle = 174,           // Dictionary key: LavaTurtle
    RaceDemonPortal = 175,          // Dictionary key: DemonPortal
    RaceSlimeWell = 176,            // Dictionary key: SlimeWell
    RaceHornetsNest = 177,          // Dictionary key: HornetsNest
    RaceUnholyTomb = 178,           // Dictionary key: UnholyTomb
    RaceBonePile = 179,             // Dictionary key: BonePile
    RaceMolehill = 180,             // Dictionary key: Molehill
    RaceWoodChest = 181,            // Dictionary key: WoodChest
    RaceSkullChest = 182,           // Dictionary key: SkullChest
    RaceObsidianChest = 183,        // Dictionary key: ObsidianChest
    RaceBoneChest = 184,            // Dictionary key: BoneChest
    RaceCampfire = 185,             // Dictionary key: Campfire
    RaceTent = 186,                 // Dictionary key: Tent
    RaceSleepingMat = 187,          // Dictionary key: SleepingMat
    RaceBeachTowel = 188,           // Dictionary key: BeachTowel
    RaceSunshade = 189,             // Dictionary key: Sunshade
    RaceSign = 190,                 // Dictionary key: Sign
    RaceMantis = 191,               // Dictionary key: Mantis
    RaceRadishCreatureSprout = 192, // Dictionary key: RadishCreatureSprout
    RacePlantCreatureSprout = 193,  // Dictionary key: PlantCreatureSprout
    RaceCage1 = 194,                // Dictionary key: Cage1
    RaceCage2 = 195,                // Dictionary key: Cage2
    RaceCage3 = 196,                // Dictionary key: Cage3
    RaceCage4 = 197,                // Dictionary key: Cage4
    RaceCauldron = 198,             // Dictionary key: Cauldron
    RaceSeeker = 199,               // Dictionary key: Seeker
    RaceBabyElephant = 200,         // Dictionary key: BabyElephant
    RaceBabyMammoth = 201,          // Dictionary key: BabyMammoth
    RacePirateMale = 202,           // Dictionary key: PirateMale
    RacePirateFemale = 203,         // Dictionary key: PirateFemale
    RaceTreasure = 204,             // Dictionary key: Treasure
    RaceTreasureStatue = 205,       // Dictionary key: TreasureStatue
    RaceTreasureJewel = 206,        // Dictionary key: TreasureJewel
    RaceTreasureVase = 207,         // Dictionary key: TreasureVase
    RaceEnhancedPirateMale = 208,   // fillAppearance: PirateMale models plus bit 17, enlarged 1.5x base size
    RaceGoldKnightHorse = 209,      // Dictionary key: GoldKnightHorse
    RaceFlamingo = 210,             // Dictionary key: Flamingo
    RaceToucan = 211,               // Dictionary key: Toucan
    RaceWagon = 212,                // Dictionary key: Wagon
    RaceSilverLion = 213,           // Dictionary key: SilverLion
    RaceRuneCube = 214,             // Dictionary key: RuneCube
    RaceBlueTurtle = 215,           // Dictionary key: BlueTurtle
    RaceSnowGnobold = 216,          // Dictionary key: SnowGnobold
    RaceSavageDog = 217,            // Dictionary key: SavageDog
    RaceSnowLionMale = 218,         // Dictionary key: SnowLionMale
    RaceSnowLionFemale = 219,       // Dictionary key: SnowLionFemale
    RaceSnowLeopard = 220,          // Dictionary key: SnowLeopard
    RaceBrazier = 221,              // Dictionary key: Brazier
    RaceGhostBrazier = 222,         // Dictionary key: GhostBrazier
    RaceUnholyBonfire = 223,        // Dictionary key: UnholyBonfire
    RaceAlphaDog = 224,             // Dictionary key: AlphaDog
    RaceFlyingEye = 225,            // Dictionary key: FlyingEye
    RaceDemonMarauder = 226,        // Dictionary key: DemonMarauder
    RaceDarkCultistMale = 227,      // Dictionary key: DarkCultistMale
    RaceDarkCultistFemale = 228,    // Dictionary key: DarkCultistFemale
    RaceCrawler = 229,              // Dictionary key: Crawler
    RaceNecromancer = 230,          // Dictionary key: Necromancer
    RaceDemonIdol = 231,            // Dictionary key: DemonIdol
    RaceBloodaxeClanBanner = 232,   // Dictionary key: BloodaxeClanBanner
    RaceStoneTablet = 233,          // Dictionary key: StoneTablet
    RaceLever = 234,                // Dictionary key: Lever
    RaceLockedLever = 235,          // Dictionary key: LockedLever
    RaceEmpireSoldierMale = 236,    // Dictionary key: EmpireSoldierMale
    RaceEmpireSoldierFemale = 237,  // Dictionary key: EmpireSoldierFemale
    RaceTombstone = 238,            // Dictionary key: Tombstone
    RaceManaPump = 239,             // Dictionary key: ManaPump
    RaceManaInductor = 240,         // Dictionary key: ManaInductor
    RaceManaGenerator = 241,        // Dictionary key: ManaGenerator
    RaceSack = 242,                 // Dictionary key: Sack
    RaceCrate = 243,                // Dictionary key: Crate
    RaceGate = 244,                 // Dictionary key: Gate
    RaceLockedGate = 245,           // fillAppearance uses sprite 3551 (locked-gate.cub)
    RaceRuneGate = 246,             // fillAppearance uses sprite 3553 (rune-gate.cub)
    RaceStoneGate = 247,            // fillAppearance uses sprite 3555 (stone-gate.cub)
    RaceWall = 248,                 // Dictionary key: Wall
    RaceSpiderweb = 249,            // Dictionary key: Spiderweb
    RaceMagicBarrier = 250,         // Dictionary key: MagicBarrier
    RaceGrid = 251,                 // fillAppearance uses sprite 3634 (grid.cub)
    RaceEmpress = 252,              // Dictionary key: Empress
    RaceArchdruid = 253,            // Dictionary key: Archdruid
    RaceHalfDemon = 254,            // Dictionary key: HalfDemon
    RaceGhostPreacher = 255,        // Dictionary key: GhostPreacher
    RaceLegendaryPaladin = 256,     // Dictionary key: LegendaryPaladin
    RaceCreatorMale = 257,          // Dictionary key: CreatorMale
    RaceCreatorFemale = 258,        // Dictionary key: CreatorFemale
    RaceAngel = 259,                // Dictionary key: Angel
    RaceEnhancedOrcMale =
        260, // fillAppearance: normal Orc models plus appearance bit 17, which enlarges size and boosts stats
    RaceEnhancedOrcFemale =
        261, // fillAppearance: normal Orc models plus appearance bit 17, which enlarges size and boosts stats
    RaceManaMech = 262,          // Dictionary key: ManaMech
    RaceTreant = 263,            // Dictionary key: Treant
    RaceFireRunestone = 264,     // Dictionary key: FireRunestone
    RaceWaterRunestone = 265,    // Dictionary key: WaterRunestone
    RaceIceRunestone = 266,      // Dictionary key: IceRunestone
    RaceWindRunestone = 267,     // Dictionary key: WindRunestone
    RaceRunePortal = 268,        // Dictionary key: RunePortal
    RaceMonstrosity = 269,       // Dictionary key: Monstrosity
    RaceSkullSlater = 270,       // Dictionary key: SkullSlater
    RaceTortureRack = 271,       // Dictionary key: TortureRack
    RaceWitchWagon = 272,        // Dictionary key: WitchWagon
    RaceLegIrons = 273,          // Dictionary key: LegIrons
    RacePole = 274,              // Dictionary key: Pole
    RaceCeilingChain = 275,      // Dictionary key: CeilingChain
    RaceWraith = 276,            // Dictionary key: Wraith
    RaceManaDeer = 277,          // Dictionary key: ManaDeer
    RaceShrineOfTheLight = 278,  // Dictionary key: ShrineOfTheLight
    RacePirateBoat = 279,        // fillAppearance uses sprite 3607 (pirate-boat.cub)
    RaceBarrel = 280,            // Dictionary key: Barrel
    RaceBarrelTnt = 281,         // fillAppearance uses sprite 3608 (barrel-tnt.cub)
    RaceFootprints = 282,        // Dictionary key: Footprints
    RaceHorseFootprints = 283,   // Dictionary key: HorseFootprints
    RaceMonsterFootprints = 284, // Dictionary key: MonsterFootprints
    RaceWaterBubble = 285,       // fillAppearance uses sprite 3616 (water-bubble.cub)
    RaceSarcophagus = 286,       // Dictionary key: Sarcophagus
    RaceChiling = 287,           // Dictionary key: Chiling
    RaceChilingSprout = 288,     // Dictionary key: ChilingSprout
    RaceMagicCrystal = 289,      // Dictionary key: MagicCrystal
    RaceHabanero = 290,          // Dictionary key: Habanero
    RaceBloomling = 291,         // Dictionary key: Bloomling
    RaceEarthPile = 292,         // Dictionary key: EarthPile
    RaceCaterpillar = 293,       // Dictionary key: Caterpillar
    RaceEarthCaterpillar = 294,  // Dictionary key: EarthCaterpillar
    RaceSnail = 295,             // Dictionary key: Snail
    RaceDarkJester = 296,        // Dictionary key: DarkJester
    RaceSwimmingSign = 297,      // fillAppearance uses sprite 3524 (swimming-sign.cub)
    RacePedestal = 298,          // Dictionary key: Pedestal
    RaceSocket = 299,            // Dictionary key: Socket
    RaceBirdStatue = 300,        // Dictionary key: BirdStatue
    RaceMourningStatue = 301,    // Dictionary key: MourningStatue
    RaceClimbingPlant =
        302, // fillAppearance uses sprite 3636 (climbing-plant-soil.cub); AnimationState::addSprites also stacks climbing-plant-stalk.cub (3635)
    RaceEagle = 303,         // Dictionary key: Eagle
    RaceGnome = 304,         // Dictionary key: Gnome
    RaceSupplierGnome = 305, // fillAppearance uses sprite 3647 (supplier-gnome-body.cub)
};

// Creature ability identifiers for attacks, movement, interaction and other actions.
enum Ability {
    AbilityUseCurrent = -1, // Meaning unknown.
    AbilityNone = 0,        // zero clears current ability
    AbilityWatchPlayer =
        1, // World::AIManager::watchOrGreetPlayer: a creature that sees a player within 20 blocks stops and faces it (friendly NPCs sometimes wave instead); Game::interactionPrompt offers "Pet" for an animal in this state
    AbilityRightSlash = 2,    // dictionary key SkillAttack
    AbilityLeftSlash = 3,     // dictionary key SkillAttack
    Ability_4 = 4,            // Meaning unknown.
    Ability_5 = 5,            // Meaning unknown.
    AbilityPerforate = 6,     // dictionary key SkillPerforate
    AbilityRightStab = 7,     // dictionary key SkillAttack
    AbilityLeftStab = 8,      // dictionary key SkillAttack
    AbilityBlock = 9,         // dictionary key SkillBlock
    AbilityShieldAttack = 10, // dictionary key SkillAttack
    AbilityBlockJab = 11,     // dictionary key SkillAttack
    AbilitySwirlStrike =
        12, // release of the charged AbilitySwirl: Game::controlCharacter and AIManager::combat switch Swirl to it when the charge is let go; performAbility strikes around the creature three times (range x3, period strikeTime/3)
    AbilityCrossedWeaponSweep = 13,
    AbilityLeftSweep = 14,  // dictionary key SkillAttack
    AbilityRightSweep = 15, // dictionary key SkillAttack
    AbilityOverheadChop = 16,
    AbilityMonsterChargeStrike =
        17,             // release of the charged AbilityMonsterCharge (65): Game::controlCharacter, AIManager::combat
    AbilityAmbush = 18, // dictionary key SkillAmbush
    AbilityLeftDaggerPierce = 19,  // dictionary key SkillAttack
    AbilityRightDaggerPierce = 20, // dictionary key SkillAttack
    AbilityKick = 21,              // dictionary key SkillKick
    AbilityRangerKick = 22,        // dictionary key SkillRangerKick
    AbilityShoot = 23,             // dictionary key SkillAttack
    AbilityChargedShotRelease =
        24,                  // Release of a charged shot; class 150's special attack. Fires a powered arrow or bullet.
    AbilityChargedShot = 25, // dictionary key SkillChargedShot
    AbilitySalvo = 26,       // dictionary key SkillSalvo
    AbilityThrowBoomerang = 27,   // dictionary key SkillAttack
    AbilityChargedBoomerang = 28, // dictionary key SkillChargedBoomerang
    AbilitySupportRay =
        29, // AI::checkSupport aims it at an ally; World::performAbility heals the focused creature by maxHP/10 every 200 ms; Renderer draws a ray
    Ability_30 = 30,         // Meaning unknown.
    AbilityFireSwirl = 31,   // dictionary key SkillFireSwirl
    AbilityFireVortex = 32,  // dictionary key SkillFireVortex
    AbilityWaterSwirl = 33,  // dictionary key SkillWaterSwirl
    AbilityWaterVortex = 34, // dictionary key SkillWaterVortex
    AbilityHeal =
        35, // World::performAbility: eight heals of twice the spell power on the focused creature or the caster, paid in mana
    Ability_36 = 36, // Meaning unknown.
    AbilityFireballCharge =
        37, // Held fireball charge, increasing by tempo * 0.0005 per millisecond; releases into AbilityFireball and glows like beam spells. No code starts it.
    AbilityFireball = 38, // dictionary key SkillFireball
    AbilityMonsterFirebolt =
        39, // ClassMonsterCaster's firebolt, charged like a fireball; dictionary key SkillFirebolt. Player wands use beams instead.
    AbilityFirebolt = 40,          // Second caster attack, using AnimationLeftCast and dictionary key SkillFirebolt.
    AbilityBraceletFirebolt = 41,  // First caster attack, using AnimationRightCast and dictionary key SkillFirebolt.
    AbilityWaterbolt = 42,         // Second water caster attack; dictionary key SkillWaterbolt.
    AbilityBraceletWaterbolt = 43, // First water caster attack; dictionary key SkillWaterbolt.
    AbilityWaterball = 44,         // dictionary key SkillWaterball
    AbilityMonsterWaterbolt =
        45, // ClassMonsterCaster's water bolt, paired with AbilityMonsterFirebolt; dictionary key SkillWaterbolt.
    AbilityWaterSalvo = 46,
    AbilityFireMissiles = 47, // dictionary key SkillFireMissiles
    Ability_48 = 48,          // Meaning unknown.
    AbilitySmashLeap =
        49, // World::performAbility treats it with AbilityIntercept and AbilityDash (stamina at the start, SoundSmashJump); World::moveCreature turns it into AbilitySmash when the leap reaches a creature (the branch is only reached through the charging set, which leaves 49 out); Renderer::collectSprites draws it with the smash animation. No key and no starter
    AbilityIntercept = 50, // dictionary key SkillIntercept
    AbilityTeleport = 51,  // dictionary key SkillTeleport
    AbilityRetreat = 52,   // dictionary key SkillRetreat
    AbilityDiveBite =
        53,          // AIManager::combat: the dragon and flying biters within 8 blocks lunge at the target at speed 10
    Ability_54 = 54, // Meaning unknown.
    AbilitySmash = 55, // dictionary key SkillSmash
    AbilitySalvoRelease =
        56, // release of the charged AbilitySalvo: Game::controlCharacter, AIManager::combat; performAbility fires the volley
    Ability_57 = 57,             // Meaning unknown.
    AbilityTwoHandedSweep = 58,  // dictionary key SkillAttack
    AbilityTwoHandedThrust = 59, // dictionary key SkillAttack
    AbilityCrush = 60,           // dictionary key SkillCrush
    AbilityCrushStrike =
        61, // release of the charged AbilityCrush for every specialization but Guardian: Game::controlCharacter, AIManager::combat
    AbilityGuardianCrushStrike =
        62, // release of the charged AbilityCrush for SpecializationGuardian: Game::controlCharacter, AIManager::combat; strikes around the creature (range x3)
    Ability_63 = 63,   // Meaning unknown.
    AbilitySwirl = 64, // dictionary key SkillSwirl
    AbilityMonsterCharge =
        65, // ClassMonster's special attack, charged like Crush and released as AbilityMonsterChargeStrike.
    AbilityTwoHandedSpin = 66,
    AbilityTwoHandedSlam = 67,
    AbilityTwoHandedSwing = 68, // dictionary key SkillAttack
    AbilityHeadbutt =
        69, // AIManager::combat: saurians without a horned head; moveCreature rushes forward after the wind-up; World::attackCreature sounds it as weapon 17 (WeaponGreatmace) and always stuns
    AbilityHornCharge = 70, // Horned saurians, mammoths and skull bulls rush forward and always stun the target.
    AbilityTailStrike =
        71, // AIManager::combat: saurians with a spiked tail; performAbility strikes around the creature (no forward offset)
    AbilityShellBlock =
        72, // AIManager::combat: monsters with the reptile shell body; moveCreature stops it; attackCreature blocks hits during the strike time at no mana cost
    AbilityShellSpin =
        73, // Turtles and spiked-shell reptiles spin at 720 degrees per second, strike nearby creatures and block hits as with AbilityShellBlock.
    AbilityScream =
        74, // AIManager::combat: saurians and mammoths; performAbility only plays SoundLichScream at the wind-up
    AbilityStomp =
        75, // Baby elephants, mammoths and clawless saurians strike grounded creatures within 20 blocks and always stun.
    AbilityClawStrike = 76,  // Reptile-clawed saurians and weapon 23 use this attack and special attack.
    AbilityDashForward = 77, // Crested saurians rush forward at speed 60 during the strike.
    AbilityDashLeft = 78,    // Spiked-body saurians rush left at speed 60 during the strike.
    AbilityDashRight = 79,   // Spiked-body saurians rush right at speed 60 during the strike.
    AbilitySneak = 80,       // dictionary key SkillSneak
    AbilitySprint = 81,      // dictionary key SkillSprint
    AbilityDash =
        82, // World::moveCreature runs at speed 40 for 500 ms, then ends it; performAbility takes its stamina (staminaCost 0.25, as Intercept) and plays SoundSmashJump; AnimationState::addSprites leans the creature forward as in a sprint, turned 30 degrees, a foot swung out. No key and no starter
    AbilityFloat = 83,        // dictionary key SkillFloat
    AbilityDrink = 84,        // Game::useItem selects for potion ids 1..3/12 and elixirs
    AbilityEat = 85,          // Game::useItem selects for other consumable foods
    AbilityOfferPetFood = 86, // AI::combat requires target actionItem category PetFood and id equal race
    AbilitySit = 87,          // updateStaticEntities assigns for stool/bench/chair
    AbilitySleep = 88,        // updateStaticEntities assigns for bed/towel/mat
    AbilityLieDown =
        89, // Renderer::collectSprites selects AnimationLieDown (the creature turned onto its back or side and lowered to the ground, the pose of a stunned creature); with AbilitySit, AbilitySleep and 90 it is one of the resting abilities that World::updateSurvival ends when the creature moves and moveCreature leaves out of its attack facing. No starter
    Ability_90 = 90,             // Meaning unknown.
    AbilityTortureRackPose = 91, // updateStaticEntities assigns to torture-rack occupant
    AbilityPolePose = 92,        // updateStaticEntities assigns to pole occupant
    AbilityCeilingChainPose =
        93, // World::performAbility / attachCreature; Game::controlCharacter / chat command handler / useItem; World::attachCreature places CeilingChain occupant and assigns 93
    Ability_94 = 94,     // Meaning unknown.
    AbilityCyclone = 95, // dictionary key SkillCyclone
    AbilityDjinnExplosion =
        96, // Djinn special attack: after a 5000 ms cast, strikes within eight blocks like AbilityFireExplosion.
    AbilityFireExplosion = 97, // dictionary key SkillFireExplosion
    AbilityLavaPuddle = 98,    // dictionary key SkillLavaPuddle
    AbilityWaterPuddle =
        99, // Water ground spot ticking every 200 ms, cast like AbilityLavaPuddle with ElementWater. Its projectile heals creatures its owner cannot damage and attacks none. No code starts it.
    AbilityQuake =
        100, // Trolls and yetis strike grounded creatures within 20 blocks every 1000 ms, always stunning them.
    AbilitySummon =
        101, // Liches, necromancers, djinns and ogres summon up to three followers with BehaviourFollowOwner.
    AbilitySpawnerSummon =
        102, // AIManager::updateCreature starts it for ClassSpawner creatures; performAbility creates a roaming creature (BehaviourRoam)
    AbilityLunge =
        103, // Cyclopes, trolls, yetis, skeletons, werewolves and vampires rush forward after wind-up and always stun.
    AbilityFireBeam = 104,   // dictionary key SkillFireBeam
    AbilityFireRay = 105,    // dictionary key SkillFireRay
    AbilityWaterRay = 106,   // dictionary key SkillWaterRay
    AbilityWaterBeam = 107,  // dictionary key SkillWaterBeam
    AbilityShuriken = 108,   // dictionary key SkillShuriken
    AbilityCamouflage = 109, // dictionary key SkillCamouflage
    AbilityParry =
        110, // Creature::isBlocking counts it as blocking (with AbilityBlock, Cyclone and the guardian's Crush/Swirl); Renderer::collectSprites selects AnimationParry, a weapon-hand swing across the body at 100 and 500 ms. No key and no starter
    AbilityAim = 111,        // dictionary key SkillAim
    AbilitySwiftness = 112,  // dictionary key SkillSwiftness
    AbilityBulwark = 113,    // dictionary key SkillBulwark
    AbilityWarFrenzy = 114,  // dictionary key SkillWarFrenzy
    AbilityManaShield = 115, // dictionary key SkillManaShield
    AbilityBlockStrike =
        116, // release of the charged AbilityBlock: Game::controlCharacter, AIManager::combat; performAbility strikes around the creature (range x2)
    AbilityPortCity = 117,     // dictionary key SkillPortCity
    AbilityRidePet = 118,      // World::moveCreature controls pet when player ability is 118; riderOffset applied
    AbilityRideNpcMount = 119, // World::updateCreature attaches NPC to focusId mount
    AbilityHangGlide = 120,    // Game::useItem toggles using HangGlider; Creature::isGliding tests 120
    AbilitySailBoat = 121,     // Game::useItem toggles using Boat
    AbilityClimb = 122,        // Game::interact Climb prompt sets 122
    AbilityThrowRock =
        123, // World::performAbility / attachCreature; Game::controlCharacter / chat command handler / useItem; World::performAbility launches projectile 7, whose model is iron-deposit.cub
    AbilityPresentItem = 124, // Game::updateSpeech sets upon receiving one special item
    Ability_125 = 125,        // Suppresses input and hits for a timed interval; no code starts this state.
    AbilityEatPetFood =
        126, // World::performAbility / attachCreature; Game::controlCharacter / chat command handler / useItem; World::performAbility requires offered PetFood ID equal race, increases BuffTameness and converts to companion
    Ability_127 = 127,          // Meaning unknown.
    AbilityCarryCreature = 128, // Game::interact Pick up prompt sets ownerId and 128
    AbilityThrowCreature = 129, // Game::interact Throw prompt sets 129
    AbilityRestHeal = 130,      // World::performAbility heals over attack duration; Creature::isPassive accepts 130
    AbilityCarryRider =
        131, // World::attachCreature: a carrier with this ability holds its occupant (focusId) on top of itself even while the occupant moves, where a sleeping mat or beach towel lets it go; World::moveCreature leaves such an occupant out of collisions. No starter
    Ability_132 = 132, // Meaning unknown.
    AbilitySelfKill =
        133, // A 60000 ms cast that reduces the player's HP to zero when complete. Movement cancels it; Interface displays its cast bar.
    AbilityHealingStream = 134, // dictionary key SkillHealingStream
    AbilityCenteredHealingStream =
        135, // AbilityHealingStream centred on the caster instead of the aimed hand (World::performAbility, Renderer::collectLights); the water mage's alternative attack (nextAttackSkill); dictionary key SkillHealingStream
    AbilityFireBreath = 136, // dictionary key SkillFireBreath
    AbilityUseSpecialItem =
        137, // World::performAbility / attachCreature; Game::controlCharacter / chat command handler / useItem; Game::useItem selects for special items other than glider/boat; World applies instrument/key effects
    AbilitySlowFall = 138, // World::moveCreature applies 3.5% gravity; update clears at ground
    AbilityWraithAttack =
        139, // World::AIManager::combat starts it after every attack duration for a creature with CreatureToggleWraithTargeting (set only for RaceWraith with a target, in combat itself), at contact range and always visible; moveCreature gives such a creature speed 10 and no block collision; performAbility strikes like AbilityShellSpin/Quake
    AbilityAssassinate = 140,   // dictionary key SkillAssassinate
    AbilityNinjutsu = 141,      // dictionary key SkillNinjutsu
    AbilityQuicksandTrap = 142, // dictionary key SkillQuicksandTrap
    AbilityShadowClone = 143,   // dictionary key SkillShadowClone
    AbilityWaterBubbles = 144,  // dictionary key SkillWaterBubbles
    AbilityAttachToTarget =
        145,                // AI::attach fixes attackRotation offset; World::updateCreature applies it and slows target
    AbilityFlameRush = 146, // dictionary key SkillFlameRush
    AbilityRockFistCharge = 147, // Held charge converting stamina into power; releasing it starts AbilityRockFist.
    AbilityRockFist = 148,       // dictionary key SkillRockFist
    AbilityHeroicShout = 149,    // dictionary key SkillHeroicShout
    AbilityMarkCreature =
        150, // Game::interactionPrompt: when the cast ends the focused creature is marked on the world map (WorldMap::markCreature, SoundPickup)
    AbilityUnmarkCreature =
        151, // Game::interactionPrompt: when the cast ends the focused creature's map mark is removed (WorldMap::unmarkCreature, SoundDrop)
    AbilityFlightToPoint = 152, // World::performAbility follows flight curve to m_abilityTargetPosition
    AbilityGrowClimbingPlant =
        153, // World interaction sets on climbing plant; update increases attackRotation[2] height
    AbilityFeedClimbingPlant =
        154, // World::performAbility / attachCreature; Game::controlCharacter / chat command handler / useItem; World::performAbility starts GrowClimbingPlant on ownerId after 1.8s and emits food crumbs
    AbilityCallPet =
        155, // World::performAbility / attachCreature; Game::controlCharacter / chat command handler / useItem; Game::controlCharacter pet-control fallback sets 155, category 33, plays whistle, sends interaction type 10
    AbilityEagleFlight = 156, // Game/World flight travel state; menu destination path and animation eagle sprite
    AbilityMenuView = 157,    // Game::update sets while menu visible, circles camera
    AbilityBloomlingAttack =
        158, // Bloomling attack and special attack: strikes around the creature three times like AbilitySwirlStrike.
    AbilityWarriorJumpAttack =
        159, // Game class skill while jumping: sets z velocity 20, costs stamina 0.25, performs melee hit
    AbilityThrowPoisonVial = 160, // dictionary key SkillThrowPoisonVial
    AbilityWave =
        161, // World::performAbility / attachCreature; Game::controlCharacter / chat command handler / useItem; Game chat command /wave sets 161
    AbilityPetAnimal = 162, // Game::interact literal Pet prompt sets 162
    AbilityDance = 163,     // Game chat /dance command sets 163
};

// Animation identifiers used by ability selection. m_animation stores the selected value as an integer.
enum AnimationId {
    AnimationNone = 0,            // every ability without a case of its own (default), and no ability
    AnimationSmash = 1,           // AbilitySmashLeap, AbilitySmash
    AnimationTwoHandedSweep = 2,  // AbilityTwoHandedSweep, AbilityCrushStrike
    AnimationTwoHandedThrust = 3, // AbilityTwoHandedThrust
    AnimationCrush = 4,           // AbilityCrush
    AnimationMonsterCharge = 5,   // AbilityMonsterCharge
    AnimationSwirl = 6,           // AbilitySwirl
    AnimationTwoHandedRest = 7,   // No ability selects this two-handed rest pose.
    AnimationParry =
        8, // AbilityParry alone selects it; AnimationState::addSprites swings the weapon hand across the body (body turned 80 then 90 degrees at 100 and 500 ms)
    AnimationBlock = 9,          // AbilityBlock; AbilityIntercept with a shield (weapon 13) in the left hand
    AnimationBlockJab = 10,      // AbilityBlockJab
    AnimationSupportRay = 11,    // AbilitySupportRay
    Animation_12 = 12,           // none: no ability selects it
    Animation_13 = 13,           // none: no ability selects it
    Animation_14 = 14,           // none: no ability selects it
    AnimationLeftSlash = 15,     // AbilityLeftSlash
    AnimationRightSlash = 16,    // AbilityRightSlash
    AnimationRightBackhand = 17, // Ability_5, AbilityLeftDaggerPierce
    AnimationLeftBackhand = 18,  // Ability_4, AbilityShieldAttack, AbilityRightDaggerPierce, Ability_63
    AnimationLeftStab = 19,      // AbilityLeftStab
    AnimationRightStab = 20,     // AbilityRightStab
    AnimationDrawBow = 21,       // AbilityShoot, AbilityChargedShotRelease
    AnimationShoot =
        22, // AbilityChargedShot, AbilitySalvo, AbilitySalvoRelease; AbilityIntercept with a bow or crossbow (weapons 6, 7) in the right hand
    AnimationFireball = 23,  // AbilityFireball, AbilityWaterball, AbilityFireMissiles
    AnimationLeftCast = 24,  // AbilityFirebolt, AbilityWaterbolt
    AnimationRightCast = 25, // AbilityBraceletFirebolt, AbilityBraceletWaterbolt
    AnimationFirebolt =
        26, // AbilityMonsterFirebolt, AbilityMonsterWaterbolt, AbilityWaterSalvo, Ability_48, AbilityFireBeam, AbilityFireRay, AbilityWaterRay, AbilityWaterBeam
    AnimationFloat = 27,              // AbilityFloat
    AnimationFireballCharge = 28,     // AbilityFireballCharge
    AnimationSwirlStrike = 29,        // AbilitySwirlStrike, AbilityBloomlingAttack
    AnimationCrossedWeaponSweep = 30, // AbilityCrossedWeaponSweep
    AnimationLeftSweep = 31,          // AbilityLeftSweep
    AnimationRightSweep = 32,         // AbilityRightSweep
    AnimationOverheadChop = 33,       // AbilityOverheadChop
    AnimationPerforate = 34,          // AbilityPerforate
    AnimationIntercept =
        35, // AbilityIntercept with neither a bow or crossbow in the right hand nor a weapon in the left
    AnimationAmbush =
        36, // AbilityMonsterChargeStrike, AbilityAmbush; AbilityIntercept and AbilityLunge with a weapon in the left hand
    Animation_37 = 37,            // none: no ability selects it
    AnimationTwoHandedSwing = 38, // AbilityTwoHandedSwing
    AnimationHeadbutt = 39,       // AbilityHeadbutt
    AnimationHornCharge = 40,     // AbilityHornCharge
    AnimationTailStrike = 41,     // AbilityTailStrike
    AnimationShell = 42,          // AbilityShellBlock, AbilityShellSpin
    AnimationScream = 43,         // AbilityScream
    AnimationStomp = 44,          // AbilityStomp
    AnimationClawStrike = 45,     // AbilityClawStrike
    AnimationDashForward = 46,    // AbilityDashForward
    AnimationDashLeft = 47,       // AbilityDashLeft
    AnimationDashRight = 48,      // AbilityDashRight
    AnimationTwoHandedSpin = 49,  // AbilityTwoHandedSpin
    AnimationGuardianCrushStrike =
        50,              // AbilityGuardianCrushStrike, AbilityTwoHandedSlam; AbilityLunge with nothing in the left hand
    AnimationKick = 51,  // AbilityKick, AbilityRangerKick
    AnimationDrink = 52, // AbilityDrink
    AnimationEat = 53,   // AbilityEat
    AnimationOfferPetFood = 54, // AbilityOfferPetFood
    AnimationSneak = 55,        // AbilitySneak
    AnimationSleep = 56,        // AbilitySleep, Ability_90
    AnimationLieDown =
        57, // AbilityLieDown alone selects it; AnimationState::addSprites turns the creature 90 degrees about x (a quadruped about y) and lowers it to the ground, the pose a stunned creature takes, without the hand and head pose of AnimationSleep (turned the other way)
    AnimationTortureRackPose = 58,  // AbilityTortureRackPose
    AnimationPolePose = 59,         // AbilityPolePose
    AnimationCeilingChainPose = 60, // AbilityCeilingChainPose
    AnimationActionLocked = 61,     // any ability while EntityData::actionLockTime is nonzero (overrides the switch)
    AnimationSit = 62,              // AbilitySit
    AnimationRide = 63,             // AbilityRidePet, AbilityRideNpcMount
    AnimationSailBoat = 64,         // AbilitySailBoat
    AnimationCyclone = 65,          // AbilityCyclone, AbilityWarriorJumpAttack
    AnimationCastSpell =
        66, // AbilityFireSwirl, AbilityFireVortex, AbilityWaterSwirl, AbilityWaterVortex, AbilityHeal, AbilityTeleport, AbilityDjinnExplosion, AbilityFireExplosion, AbilityLavaPuddle, AbilityWaterPuddle, AbilitySummon, AbilityPortCity, Ability_132, AbilitySelfKill, AbilityMarkCreature, AbilityUnmarkCreature
    AnimationHealingStream = 67,    // AbilityHealingStream, AbilityCenteredHealingStream, AbilityWaterBubbles
    Animation_68 = 68,              // Ability_36
    AnimationQuake = 69,            // AbilityQuake
    Animation_70 = 70,              // none: no ability selects it
    AnimationThrow = 71,            // AbilityShuriken, AbilityThrowPoisonVial
    AnimationThrowBoomerang = 72,   // AbilityThrowBoomerang
    AnimationChargedBoomerang = 73, // AbilityChargedBoomerang
    AnimationBulwark = 74,          // AbilityBulwark, AbilityManaShield
    AnimationBlockStrike = 75,      // AbilityBlockStrike
    AnimationDiveBite = 76,         // AbilityDiveBite
    AnimationThrowRock = 77,        // AbilityThrowRock
    AnimationPresentItem =
        78,                   // AbilityPresentItem; AbilityUseSpecialItem with an item that has no animation of its own
    Animation_79 = 79,        // Ability_125
    AnimationEatPetFood = 80, // AbilityEatPetFood
    AnimationCarryCreature = 81,     // AbilityCarryCreature
    AnimationThrowCreature = 82,     // AbilityThrowCreature
    Animation_83 = 83,               // none: no ability selects it
    AnimationRestHeal = 84,          // AbilityRestHeal
    AnimationFireBreath = 85,        // AbilityFireBreath
    AnimationAssassinate = 86,       // AbilityAssassinate
    AnimationRockFist = 87,          // AbilityRockFist
    AnimationRockFistCharge = 88,    // AbilityRockFistCharge
    AnimationHeroicShout = 89,       // AbilityHeroicShout
    AnimationPlayFlute = 90,         // AbilityUseSpecialItem with SpecialItemFlute
    AnimationPlayHarp = 91,          // AbilityUseSpecialItem with SpecialItemHarp
    AnimationPlayFiddle = 92,        // AbilityUseSpecialItem with SpecialItemFiddle
    AnimationPlayWhistle = 93,       // AbilityUseSpecialItem with SpecialItemWhistle
    AnimationPlayBell = 94,          // AbilityUseSpecialItem with SpecialItemBell
    AnimationPlayLute = 95,          // AbilityUseSpecialItem with SpecialItemLute
    AnimationPlayNightingale = 96,   // AbilityUseSpecialItem with SpecialItemNightingale
    AnimationFlightToPoint = 97,     // AbilityFlightToPoint
    AnimationCallPet = 98,           // AbilityCallPet
    AnimationFeedClimbingPlant = 99, // AbilityFeedClimbingPlant
    AnimationWave = 100,             // AbilityWave
    AnimationPetAnimal = 101,        // AbilityPetAnimal
    AnimationDance = 102,            // AbilityDance
};

// buffs identifiers.
enum BuffType {
    BuffNone =
        0, // World::performAbility; Creature::armor / maxAggroTarget / maxHP; sentinel: only nonzero buff is applied
    BuffBulwark = 1,    // aura:bulwark; aura icon lookup uses buff.type
    BuffWarfrenzy = 2,  // aura:warfrenzy; aura icon lookup uses buff.type
    BuffCamouflage = 3, // aura:camouflage; aura icon lookup uses buff.type
    BuffPoisoned = 4,   // aura:poisoned; aura icon lookup uses buff.type
    BuffManashield = 6, // aura:manashield; aura icon lookup uses buff.type
    BuffTameness = 7,   // World::performAbility case 126..13489: accumulated tameness; hitType 6
    BuffTamingAnger =
        8, // After rejected feeding, AI::combat applies taming anger for 20000 ms with the target as its source.
    BuffInstantfire = 9,   // aura:instantfire; aura icon lookup uses buff.type
    BuffInstantshoot = 10, // aura:instantshoot; aura icon lookup uses buff.type
    BuffDodge = 11,        // aura:dodge; aura icon lookup uses buff.type
    BuffSwiftness = 12,    // aura:swiftness; aura icon lookup uses buff.type
    BuffNinjutsu = 13,     // aura:ninjutsu; aura icon lookup uses buff.type
    BuffDefenseWeakened =
        14, // World::performAbility; Creature::armor / maxAggroTarget / maxHP; armor/resistance/reduction multiplied down, model sinks by quarter height
    BuffShadowclone = 15,  // aura:shadowclone; aura icon lookup uses buff.type
    BuffFlamerush = 16,    // aura:flamerush; aura icon lookup uses buff.type
    BuffRegeneration = 17, // aura:regeneration; aura icon lookup uses buff.type
    BuffForcedAggroTarget =
        18, // World::performAbility; Creature::armor / maxAggroTarget / maxHP; maxAggroTarget returns buff.sourceId; World interaction applies for 5s
    BuffSpiritworld = 19,      // aura:spiritworld; aura icon lookup uses buff.type
    BuffLifeflask = 22,        // aura:lifeflask; aura icon lookup uses buff.type
    BuffAttackflask = 23,      // aura:attackflask; aura icon lookup uses buff.type
    BuffArmorflask = 24,       // aura:armorflask; aura icon lookup uses buff.type
    BuffResistanceflask = 25,  // aura:resistanceflask; aura icon lookup uses buff.type
    BuffHeatresistance = 26,   // aura:heatresistance; aura icon lookup uses buff.type
    BuffColdresistance = 27,   // aura:coldresistance; aura icon lookup uses buff.type
    BuffPlagueresistance = 28, // aura:plagueresistance; aura icon lookup uses buff.type
    BuffFrozen = 29,           // aura:frozen; aura icon lookup uses buff.type
    Buff_30 = 30,              // Meaning unknown.
    Buff_31 = 31,              // Meaning unknown.
    BuffMaxHpMultiplier =
        32, // World::performAbility; Creature::armor / maxAggroTarget / maxHP; Creature::maxHP multiplies by multiplier; AI and World create scaling buff
    BuffAncientpower = 33,   // aura:ancientpower; aura icon lookup uses buff.type
    BuffManaabsorption = 34, // aura:manaabsorption; aura icon lookup uses buff.type
};

// Independent creature flag bits.
enum CreatureFlag {
    CreatureFlagNone = 0,               // zero clears all flags
    CreatureFlagOnGround = 1,           // barrier below box sets bit 0
    CreatureFlagInWater = 2,            // overlapping BlockWater sets bit 1
    CreatureFlagAtWall = 4,             // touchWall sets bit 2 on a side barrier
    CreatureFlagPartlyAboveWater = 16,  // water overlap with any non-water voxel sets bit 4
    CreatureFlagMovementBlocked = 32,   // acceleration and velocity with less than 1% intended displacement sets bit 5
    CreatureFlagInsideBarrier = 64,     // overlap with BlockFlag_64 sets bit 6
    CreatureFlagInLava = 128,           // BlockLava below box sets bit 7
    CreatureFlagAtSpecialWall = 256,    // touchWall sets bit 8 for block type 16; the surface meaning is unknown.
    CreatureFlagInPoison = 512,         // BlockPoison below box sets bit 9
    CreatureFlagOnSnow = 1024,          // BlockSnow/SNOW_LEAVES below box sets bit 10
    CreatureFlagOnFoliage = 2048,       // BlockLeaves/type 12 below box sets bit 11
    CreatureFlagWallAboveFeet = 4096,   // touchWall aboveFeet sets bit 12
    CreatureFlagOnPath = 8192,          // BlockPath below box sets bit 13
    CreatureFlagAtPassableWall = 16384, // touchWall sees BlockFlag_4 and sets bit 14; AI routes round edge
};

// hostility-types identifiers.
enum HostilityType {
    HostilityPlayer =
        0, // AI::update / follow; Game::interact; Creature::canDoDamage / canBeDamaged; AI::update gathers playerPositions for hostility 0; only local player runs client physics
    HostilityEnemy =
        1, // AI::update / follow; Game::interact; Creature::canDoDamage / canBeDamaged; Creature::canDoDamage compares side against type 1
    HostilityFriendlyNpc =
        3, // AI::update / follow; Game::interact; Creature::canDoDamage / canBeDamaged; Game::interact prompts Talk for type 3; Creature::isCage uses 8 instead
    HostilityNeutral =
        4, // AI::update / follow; Game::interact; Creature::canDoDamage / canBeDamaged; Game::interact offers Pet/Interact; wild tameable creatures can use type 4
    Hostility_5 = 5, // Overrides movement speed; no creator or distinct faction behavior is known.
    HostilityPet =
        6, // AI::update / follow; Game::interact; Creature::canDoDamage / canBeDamaged; AI follow routines regenerate 10% maxHP/s for type 6; Creature pet ownership
    HostilityAttackableObject =
        7, // AI::update / follow; Game::interact; Creature::canDoDamage / canBeDamaged; canBeDamaged accepts 7, all types canDamage it; includes scarecrow/dummy exception
    HostilityInteractiveObject =
        8, // AI::update / follow; Game::interact; Creature::canDoDamage / canBeDamaged; isCage/isGate require type 8; canDoDamage rejects it
};

// appearance-flags: Single bits only (Appearance::appearanceFlags); masks of several are ORs of these.
enum AppearanceFlag {
    AppearanceFlagQuadruped =
        1, // fillAppearance sets it for the four-legged races (horses, dogs, cats, cattle, turtles, beetles, lions; 71 races); AnimationState::addSprites keeps the hands at the appearance's positions, swings them with the opposite hind foot and lays a stunned creature on its side; World::moveCreature does not pitch it
    AppearanceFlagFlying =
        2, // fillAppearance sets it for bats, insects, birds, the dragon, angels and wraiths (22 races); AnimationState::addSprites and World::moveCreature treat it like standing on ground when airborne
    AppearanceFlagHeadAimOrigin = 4, // Creature::sightPosition and renderer spell origin use head instead of hands
    AppearanceFlagHandAttacks =
        8, // Arms both hands with WeaponHandAttacks and skips class weapons. Used by monkeys, golems, zombies, crabs, plant creatures and skeletons. Enemies with this flag drop leftovers.
    AppearanceFlagBiteAttack =
        16, // Arms the right hand with WeaponBiteAttack and empties the left, using attack duration 15 instead of 30. Used by insects, onionlings and quadrupeds; creatures with it drop leftovers.
    AppearanceFlagUnarmored =
        32, // equip gives no class chest armour, weapon materials or druid staff and puts the null chest item ChestBackpack on; fillAppearance sets it for animals, monsters and costumed races (195 races)
    AppearanceFlagClassMaster = 64, // Game::updateCreatureInfo displays Warrior/Ranger/Mage/Rogue Master
    AppearanceFlagFish =
        256, // fillAppearance sets it for the eight fish races only; World::moveCreature gives objects with it (or NoTerrainCollision) the ordinary buoyancy
    AppearanceFlagTinted =
        512, // Tinted creatures have their own light, 1.5 times the size, ten times the HP, more armor and power, and a random special skill.
    AppearanceFlagSuppressHairDebris = 1024, // Game::update excludes hair from disintegration particles
    AppearanceFlagSummoned = 2048,           // World summon code sets; excludes loot and permits feeding owned summon
    AppearanceFlagCaptive = 4096, // World::updateCreature handles release and mission discovery; used with mission flag
    AppearanceFlagMissionResident =
        8192,                      // Missions generates residents with this flag; restricts damage to player/pet aggro
    AppearanceFlagPassive = 16384, // Creature::isPassive uses flag unless active; allows Pick up
    AppearanceFlagMarked = 32768,  // Creature::isMarked returns mask test
    AppearanceFlagNoTerrainCollision = 65536,     // suppresses collision correction and buoyancy
    AppearanceFlagEnhanced = 131072,              // size x1.5; stats +20% and hp x1.5
    AppearanceFlagLarge = 262144,                 // Creature::isLarge returns mask test; size x7
    AppearanceFlagSpiritForm = 1048576,           // same transparency/light/grid bypass as BuffSpiritworld
    AppearanceFlagUsable = 2097152,               // Game literal Use prompt; interaction sets state
    AppearanceFlagPetrified = 4194304,            // Game literal he_is_petrified text; cannot damage
    AppearanceFlagObstacle = 8388608,             // Creature::isObstacle mask test with gate/dead exceptions
    AppearanceFlagFullShininess = 16777216,       // Animation draws equipment using full shine
    AppearanceFlagNoCreatureCollision = 33554432, // World skips mutual creature collision
    AppearanceFlagPromotedResident = 67108864,    // World resident promotion consumed per region; stats +10%, red tint
};

// inventory-tabs identifiers.
enum InventoryTab {
    InventoryTabEquipment = 0,   // Inventory::itemTab; ..5557; category dispatch
    InventoryTabSpecial = 1,     // Inventory::itemTab; ..5557; category dispatch
    InventoryTabConsumables = 2, // Inventory::itemTab; ..5557; category dispatch
    InventoryTabCrafting = 3,    // Inventory::itemTab; ..5557; category dispatch
    InventoryTabPets = 4,        // Inventory::itemTab; ..5557; category dispatch
    InventoryTabArtifacts = 5,   // Inventory::itemTab; ..5557; category dispatch
};

// creature-toggles identifiers.
enum CreatureToggle {
    CreatureToggleLampOn = 0,             // Creature::lampDiameter/Game update reads lamp toggle
    CreatureToggleActive = 1,             // isPassive requires clear; captivity/activation sets it
    CreatureToggleFaceAim = 2,            // World::moveCreature sets while attacking; AI faces attackRotation
    CreatureToggle_3 = 3,                 // Stored toggle whose effect is unknown.
    CreatureToggleNoTerrainCollision = 4, // World::moveCreature excludes terrain correction
    CreatureToggleCombatSideOverride = 5, // Creature::canDoDamage permits damage when either set
    CreatureToggleFastMovement = 6,       // World chooses speed bonus; AI sets when far from owner
    CreatureToggleDodge = 7,              // World attackCreature converts hit to MISS
    CreatureToggleMissionComplete =
        8, // World::updateCreature: set when all tracked mission creatures are dead; Missions::update consumes missionComplete with interactionState
    CreatureTogglePlayerAi = 9,         // AI updates type 0 if set
    CreatureToggleLeanForwardPose = 10, // Animation rotates hands 60 degrees
    CreatureToggleSuppressResidentPromotion =
        11, // World::spawn/update residents: region set from suppressResidentPromotion clears AppearanceFlagPromotedResident and prevents promoted hostile spawning
    CreatureToggleWraithTargeting =
        12, // AI::combat sets this for a wraith with a target, suppressing ordinary physics.
    CreatureToggleAttachedToFurniture = 13, // updateStaticEntities sets on stool/bed occupants
    CreatureToggleMagicBarriersDisabled =
        14, // Game update uses to show description_magic_barriers; mission completion sets
    CreatureToggleRemovePetrification =
        15, // World::spawn/update residents: player region set from removePetrification clears AppearanceFlagPetrified on residents
};

// Creature behavior flags in Creature::m_behaviours and Zone::Resident::behaviours, selecting AI behavior.
enum Behaviour {
    BehaviourNone = 0,      // Set by the Creature constructor and by mission objects.
    BehaviourSleep = 2,     // idle returns home beyond 4 blocks (not 32) and otherwise sleeps (AbilitySleep)
    BehaviourStand = 4,     // idle stands still for 5..13 s
    BehaviourSit = 8,       // idle sits (AbilitySit) for 5..13 s
    BehaviourWander = 0x10, // idle walks in a random direction for 3..8 s
    BehaviourUseFurniture =
        0x20, // idle goes to a free stool, bench, beach towel, wood mat or druid chair within a zone and uses it
    BehaviourFollowOwner = 0x80,    // AIManager::escort follows ownerId (companions, mounts)
    BehaviourRestHeal = 0x100,      // idle rests (AbilityRestHeal)
    BehaviourAttachToEnemy = 0x200, // AIManager::targetSkill attaches the object to the nearest enemy
    BehaviourMirrorOwner =
        0x400, // AIManager::mirrorOwnerActions copies the owner's actions for shadow clones; World::passInteractions sets this mode.
    BehaviourRoam = 0x1000,         // idle walks to a random spawn position up to five cells away
    Behaviour_8192 = 0x2000,        // set for captives and captive mission residents; nothing tests it
    BehaviourSupportOwner = 0x4000, // AIManager::checkSupport stays by the owner and supports it
};

enum HiddenPart {
    HiddenPartHead = 1,          // m_headSprite cleared
    HiddenPartHair = 2,          // m_hairSprite cleared
    HiddenPartShoulderArmor = 4, // m_shoulderArmorSprite cleared
    HiddenPartChest = 8,         // m_chestSprite cleared
    HiddenPartLeftHand = 0x10,   // the left hand is not placed
    HiddenPartRightHand = 0x20,  // the right hand is not placed
    HiddenPartWeapons = 0x40,    // both weapon sprites cleared
    HiddenPartLeftFoot = 0x80,   // the left foot is not placed
    HiddenPartRightFoot = 0x100, // the right foot is not placed
};

enum FactionType {
    FactionNone = 0,             // Game::updateCreatureLabels: no faction label
    FactionOrderOfTheLight = 1,  // key OrderOfTheLight
    FactionObsidianKnights = 2,  // key ObsidianKnights
    FactionBloodaxeClan = 3,     // key BloodaxeClan
    FactionCultOfDoom = 4,       // key CultOfDoom
    FactionUnholyPact = 5,       // key UnholyPact
    FactionSteelEmpire = 6,      // key SteelEmpire
    FactionExplorersGuild = 7,   // key ExplorersGuild
    FactionDruidsOfMana = 8,     // key DruidsOfMana
    FactionMonsterHunters = 9,   // key MonsterHunters
    FactionGang = 10,            // key Gang
    FactionRestlessWarrior = 11, // key RestlessWarrior
    FactionRealmKingdom = 12,    // key RealmFactionKingdom
    FactionRealmKingdomOf = 13,  // key RealmFactionKingdomOf
    FactionRealmKingdoms = 14,   // key RealmFactionKingdoms
    FactionRealmKingdomsOf = 15, // key RealmFactionKingdomsOf
    FactionRealmRealm = 16,      // key RealmFactionRealm
    FactionRealmRealmOf = 17,    // key RealmFactionRealmOf
    FactionRealmRealms = 18,     // key RealmFactionRealms
    FactionRealmRealmsOf = 19,   // key RealmFactionRealmsOf
    FactionRealmTribe = 20,      // key RealmFactionTribe
    FactionRealmCult = 21,       // key RealmFactionCult
    FactionRealmCultOf = 22,     // key RealmFactionCultOf
};

enum RulerType {
    RulerKing = 0,             // key RulerKing
    RulerQueen = 1,            // key RulerQueen
    RulerCultistMale = 2,      // key RulerCultistMale
    RulerCultistFemale = 3,    // key RulerCultistFemale
    RulerEmperor = 4,          // key RulerEmperor
    RulerEmpress = 5,          // key RulerEmpress
    RulerLord = 6,             // key RulerLord
    RulerLady = 7,             // key RulerLady
    RulerWizard = 8,           // key RulerWizard
    RulerWitch = 9,            // key RulerWitch
    RulerSorcerer = 10,        // key RulerSorcerer
    RulerSorceress = 11,       // key RulerSorceress
    RulerPrince = 12,          // key RulerPrince
    RulerPrincess = 13,        // key RulerPrincess
    RulerChieftainMale = 14,   // key RulerChieftainMale
    RulerChieftainFemale = 15, // key RulerChieftainFemale
    RulerSeer = 16,            // key RulerSeer
    RulerSeeress = 17,         // key RulerSeeress
    RulerElderMale = 18,       // key RulerElderMale
    RulerElderFemale = 19,     // key RulerElderFemale
    RulerShamanMale = 20,      // key RulerShamanMale
    RulerShamanFemale = 21,    // key RulerShamanFemale
    RulerKnightMale = 22,      // key RulerKnightMale
    RulerKnightFemale = 23,    // key RulerKnightFemale
    RulerPreacherMale = 24,    // key RulerPreacherMale
    RulerPreacherFemale = 25,  // key RulerPreacherFemale
};

enum SpeciesType {
    SpeciesNone = 0,         // the default: races without a case
    SpeciesPlant = 1,        // bushes, scrubs, ginseng, cactus, thorn tree
    SpeciesObject = 2,       // ore deposits, scarecrow, aim, dummy, vase, bomb
    SpeciesHumanoid = 3,     // the peoples (elves .. undead people), wizards, bandits, gnolls
    SpeciesAnimal = 4,       // dogs, cats, birds, insects, beetles, mounts, fish
    SpeciesUndead = 5,       // skeletons, zombies, vampires, lich, frightener, sand horror
    SpeciesDemon = 6,        // devourer, minotaur, imp, hell demon; demonDamageBonus applies
    SpeciesPlantCreature = 7 // plant creature, radish creature, onionlings
};

// Names from what each case does and who queues it.
enum InteractionType {
    InteractionUseItem = 1, // Game::useItem; the bomb (consumable 9) spawns a bomb creature
    InteractionUseSpecialItem =
        2, // the action item after ability 137 (5 s) or a speech bubble's item; special item 5 opens gates
    InteractionActivate = 3,      // Game::interact; opens gates, starts the arena champion's fight
    InteractionSpeech = 4,        // Game::updateSpeech, a speech bubble without an item; no case in passInteractions
    InteractionTalk = 5,          // Game::ask/interactionPrompt and AI; the innkeeper rests the player at night
    InteractionPickUp = 7,        // Game's pickup of a zone item (position = zone, item index)
    InteractionDrop = 8,          // World::dropItem of the item near the creature
    InteractionWhistle = 10,      // the tamer's whistle (sound 124): the pet comes back
    InteractionShadowClone = 11,  // AbilityShadowClone: spawns up to three owned copies
    InteractionWaterBubbles = 12, // AbilityWaterBubbles: spawns an owned race 285 creature
    InteractionHeroicShout = 13   // AbilityHeroicShout: draws the enemies' aggro
};

// These indices are not race IDs.
enum CharacterRaceChoice {
    CharacterRaceChoiceHuman = 0,
    CharacterRaceChoiceElf = 1,
    CharacterRaceChoiceDwarf = 2,
    CharacterRaceChoiceOrc = 3,
    CharacterRaceChoiceGoblin = 4,
    CharacterRaceChoiceLizard = 5,
    CharacterRaceChoiceUndead = 6,
    CharacterRaceChoiceFrogman = 7,
};

// Character style conversion adds one.
enum CharacterClassChoice {
    CharacterClassChoiceWarrior = 0,
    CharacterClassChoiceRanger = 1,
    CharacterClassChoiceMage = 2,
    CharacterClassChoiceRogue = 3,
};

// World::performAbility switch at base copies summoner (1), opponent with summoner fallback (2), or calls
// fillAppearance/equip on a selected summon race (0). Creature::equip stores 0 for spawners such as demon portal and
// slime well.
enum SummonAppearanceMode {
    SummonSelectedRace = 0,
    SummonCopySummoner = 1,
    SummonCopyOpponent = 2,
};

}
