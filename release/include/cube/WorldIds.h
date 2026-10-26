// ID catalog companion to World.h.
#pragma once

namespace cube {

// environment-types identifiers.
enum EnvironmentType {
    EnvironmentTypeNone = 0, // World::environmentType / height computation / masonryFloor; no height alteration
    EnvironmentTypeRaisedPlateau =
        1, // World::environmentType / height computation / masonryFloor; smooth radial rise between 0.5 and 0.7 radius
    EnvironmentTypeSpiralHill = 2, // World::environmentType / height computation / masonryFloor; spiral ramp adds rise
    EnvironmentTypeSpiralDepression =
        3, // World::environmentType / height computation / masonryFloor; same spiral ramp subtracts rise; classifier does not currently emit this value
    EnvironmentTypeElementalRing =
        4, // World::environmentType / height computation / masonryFloor; annulus modifies lava/plague weighting depending on Firelands biome
    EnvironmentTypeMoat =
        5, // World::environmentType / height computation / masonryFloor; annular height depression by up to 100, also suppresses hamlet plots
};

enum BuildingKind {
    BuildingKindUnclassified = 0,
    BuildingKindHouse = 1,
    BuildingKindDungeonFortification = 2,
    BuildingKindStonePavilion = 3,
    BuildingKindCrypt = 4,
    BuildingKindCastle = 5,
    BuildingKindDungeonShaftEntrance = 6,
    BuildingKindTimberMineEntrance = 7,
    BuildingKindOldHut = 8,
    BuildingKindGuildHall = 9,
    BuildingKindTempleEntrance = 10,
    BuildingKindSandstone = 11,
    BuildingKind_12 = 12,
};

enum BuildingType : int {
    BuildingNone = 0,
    BuildingOak = 1,
    BuildingEvergreen = 2,
    BuildingBirch = 3,
    BuildingJungleTree = 4,
    BuildingPalm = 5,
    BuildingOakPlaza = 6,
    BuildingPalmPlaza = 7,
    BuildingGreatOak = 8,
    BuildingBoulder = 9,
    BuildingHill = 10,
    BuildingRock = 11,
    BuildingTallRock = 12,
    BuildingRockLorePavilion = 13,
    BuildingRockTreasure = 14,
    BuildingCave = 15,
    BuildingTreasurePavilion = 16,
    BuildingGroundTreasureSite = 17,
    BuildingUnderwaterTreasureSite = 18,
    BuildingStump = 19,
    BuildingFallenTrunk = 20,
    BuildingTreasureTrunk = 21,
    BuildingHouseRoom = 22,
    BuildingHouseUpperRoom = 23,
    BuildingHouseEntrance = 24,
    BuildingHousePavilion = 25,  // Visual identifiers.
    BuildingHouseCourtyard = 26, // Visual identifiers.
    BuildingPumpkinField = 27,
    BuildingSunflowerField = 28,
    BuildingBeanField = 29,
    BuildingWheatField = 30,
    BuildingCornField = 31,
    BuildingPineappleField = 32,
    BuildingKaleField = 33,
    BuildingGraveyard = 34,
    BuildingCryptRoom = 35,
    BuildingCryptEntrance = 36,
    BuildingLoreCrypt = 37,
    BuildingCryptUpperRoom = 38,
    BuildingCatacombsEntrance = 39,
    BuildingDungeonDomedHall = 40,
    BuildingDungeonHall = 41,
    BuildingDungeonHall42 = 42,
    BuildingDungeonTower = 43,
    BuildingTerrace = 44,
    BuildingCastleCell = 45,
    BuildingCastleGate = 46,
    BuildingCastleTower = 47,
    Building_48 = 48, // Meaning unknown.
    BuildingRuinedPavilion = 49,
    BuildingRuinedFlatRoofPavilion = 50,
    BuildingBirdStatuePavilion = 51,
    BuildingTower = 52,
    BuildingShrineOfLife = 53,
    BuildingMenhir = 54,
    BuildingInscribedMenhir = 55,
    BuildingCircleOfPowerSite = 56,
    BuildingAltar = 57,
    BuildingManaWell = 58,
    BuildingShrineOfFinesse = 59,
    BuildingFloatingIslandTreasurePavilion = 60,
    BuildingMourningStatuePavilion = 61,
    BuildingBuriedTreasureChamber = 62,
    BuildingLakePlatform = 63,
    BuildingRaisedPostPlatform = 64,
    BuildingMountainPlatform = 65,
    BuildingManaPump = 66,
    BuildingManaPipe = 67,
    BuildingManaPipeEndpoint = 68,
    BuildingSanctuary = 69,
    BuildingRaisedSanctuary = 70,
    BuildingShrineEntrance = 71,
    BuildingShrineDescent10 = 72,
    BuildingShrineDescent20 = 73,
    BuildingShrineDescent30 = 74,
    BuildingShrineDescent40 = 75,
    BuildingMineEntrance = 76,
    BuildingMineDescent10 = 77,
    BuildingMineDescent20 = 78,
    BuildingMineDescent30 = 79,
    BuildingMineDescent40 = 80,
    BuildingVaultEntrance = 81,
    BuildingVaultDescent10 = 82,
    BuildingVaultDescent20 = 83,
    BuildingVaultDescent30 = 84,
    BuildingVaultDescent40 = 85,
    BuildingSewerOrGrottoEntrance = 86,
    BuildingOldHutRoom = 87,
    BuildingOldHutChimney = 88,
    BuildingOldHutEntrance = 89,
    BuildingGuildHall = 90,
    BuildingGuildHallCentre = 91,
    BuildingFlightMasterSite = 92,
    BuildingClearedMonumentGround = 93,
    BuildingPyramidEntrance = 94,
    BuildingTempleEntrance = 95,
    BuildingArenaTower = 96,
    BuildingArenaPodium = 97,
    BuildingCampfireSite = 98,
    BuildingCampEquipmentSite = 99,
    BuildingSandstoneTreasureChamber = 100,
};

// places identifiers.
enum PlaceType {
    PlaceNone =
        0, // Game::update clears place type outside influence; Missions::Missions skips zero; World::findRegion uses nearest region for zero
    PlaceHouse = 1,            // getLocationKey; dictionary key House
    PlaceFarm = 2,             // getLocationKey; dictionary key Farm
    PlaceInn = 3,              // getLocationKey; dictionary key Inn
    PlaceCrypt = 4,            // getLocationKey; dictionary key Crypt
    PlaceVillage = 5,          // getLocationKey; dictionary key Village
    Place_6 = 6,               // Meaning unknown.
    PlaceTower = 7,            // getLocationKey; dictionary key Tower
    PlaceCastle = 8,           // getLocationKey; dictionary key Castle
    PlaceGraveyard = 9,        // getLocationKey; dictionary key Graveyard
    PlaceMountain = 10,        // getLocationKey; dictionary key Mountain
    Place_11 = 11,             // Meaning unknown.
    PlaceCave = 12,            // getLocationKey; dictionary key Cave
    PlaceLake = 13,            // getLocationKey; dictionary key Lake
    PlaceLavaLake = 14,        // getLocationKey; dictionary key LavaLake
    PlaceAncientRuins = 15,    // getLocationKey; dictionary key AncientRuins
    PlaceCircleOfPower = 16,   // getLocationKey; dictionary key CircleOfPower
    PlaceManaPump = 17,        // getLocationKey; dictionary key ManaPump
    PlaceCrater = 18,          // getLocationKey; dictionary key Crater
    PlaceVolcano = 19,         // getLocationKey; dictionary key Volcano
    PlaceTree = 20,            // getLocationKey; dictionary key Tree
    PlaceRock = 21,            // getLocationKey; dictionary key Rock
    PlaceOldHut = 22,          // getLocationKey; dictionary key OldHut
    PlaceIsland = 23,          // getLocationKey; dictionary key Island
    PlaceArena = 24,           // getLocationKey; dictionary key Arena
    PlaceForest = 25,          // getLocationKey; dictionary key Forest
    PlaceDarkForest = 26,      // getLocationKey; dictionary key DarkForest
    PlaceEnchantedForest = 27, // getLocationKey; dictionary key EnchantedForest
    PlaceCanyon = 28,          // getLocationKey; dictionary key Canyon
    PlaceRocks = 29,           // getLocationKey; dictionary key Rocks
    PlacePlain = 30,           // getLocationKey; dictionary key Plain
    PlaceHill = 31,            // getLocationKey; dictionary key Hill
    PlaceGrove = 32,           // getLocationKey; dictionary key Grove
    PlaceGlade = 33,           // getLocationKey; dictionary key Glade
    PlaceCamp = 34,            // getLocationKey; dictionary key Camp
    PlaceBanditCamp = 35,      // getLocationKey; dictionary key BanditCamp
    PlaceBeach = 36,           // getLocationKey; dictionary key Beach
    PlaceAltar = 37,           // getLocationKey; dictionary key Altar
    PlaceSanctuary = 38,       // getLocationKey; dictionary key Sanctuary
    PlaceMenhir = 39,          // getLocationKey; dictionary key Menhir
    PlaceCatacombs = 40,       // getLocationKey; dictionary key Catacombs
    PlaceSewers = 41,          // getLocationKey; dictionary key Sewers
    PlaceMines = 42,           // getLocationKey; dictionary key Mines
    PlaceShrine = 43,          // getLocationKey; dictionary key Shrine
    PlaceGrotto = 44,          // getLocationKey; dictionary key Grotto
    PlacePyramid = 45,         // getLocationKey; dictionary key Pyramid
    PlaceTemple = 46,          // getLocationKey; dictionary key Temple
    PlaceVault = 47,           // getLocationKey; dictionary key Vault
};

// biomes identifiers.
enum BiomeIdentifier : int {
    BiomeNone = -1,      // no biome: randomFolkRace then adds none of the biome-specific families
    BiomePlains = 0,     // getLandName; dictionary key Plains
    BiomeOcean = 1,      // getLandName; dictionary key Ocean
    BiomeHills = 2,      // getLandName; dictionary key Hills
    BiomeWoodlands = 3,  // getLandName; dictionary key Woodlands
    BiomeJungle = 4,     // getLandName; dictionary key Jungle
    BiomeDarkWoods = 5,  // getLandName; dictionary key DarkWoods
    BiomeSnowForest = 6, // getLandName; dictionary key SnowForest
    BiomeMountains = 7,  // getLandName; dictionary key Mountains
    BiomeWetlands = 8,   // getLandName; dictionary key Wetlands
    BiomeDesert = 9,     // getLandName; dictionary key Desert
    BiomeSavannah = 10,  // getLandName; dictionary key Savannah
    BiomeSnowlands = 11, // getLandName; dictionary key Snowlands
    BiomeDeadlands = 12, // getLandName; dictionary key Deadlands
    BiomeFirelands = 13, // getLandName; dictionary key Firelands
};

// Names: the resident role Zone::generate gives the plot's house and the dictionary place keys (Inn, ItemShop, ...)
// MapOverlayWidget's icons stand for.
enum PlotType {
    PlotNone = 0,             // no plot: World::streetBuildingType leaves the cell free
    PlotHouse = 1,            // an ordinary house plot near the centre (houses 22..24, no role)
    PlotInn = 2,              // ClassInnkeeper; inn icon
    PlotItemShop = 3,         // ClassItemVendor; item-shop icon
    PlotArmorShop = 4,        // ClassArmorVendor; armor-shop icon
    PlotWeaponShop = 5,       // ClassWeaponVendor; weapon-shop icon
    PlotIdentifiersShop = 6,  // ClassAnalyzer; identifier icon
    PlotSmithy = 7,           // ClassSmith; smithy icon; furnace and anvil
    PlotCarpentersShop = 8,   // ClassCarpenter; carpenters-shop icon; sawbench
    PlotTailorsShop = 9,      // ClassTailor; tailors-shop icon; loom
    PlotFlightMaster = 10,    // ClassFlightMaster; flight-master icon; BuildingFlightMasterSite
    PlotGuildHall = 11,       // ClassGuildReceptionist; guild-hall icon; BuildingGuildHall
    PlotMarkerShrine = 12,    // a map marker only: the shrine icon (WorldMap::update)
    PlotField = 13,           // a field plot (BuildingPumpkinField..BuildingKaleField, a graveyard in undead lands)
    PlotMarkerCharacter = 14, // a map marker only: the npc icon
    PlotMarkerTreasure = 15   // a map marker only: the treasure icon
};

enum HouseStyle {
    HouseStyle_PlainFrame = 0,
    HouseStyle_MountainSlabRoof = 1,
    HouseStyle_PaintedTimber = 2,
    HouseStyle_Timber = 3,
    HouseStyle_DarkSteepRoof = 4,
    HouseStyle_DesertFlatRoof = 5,
    HouseStyle_SavannahLowRoof = 6,
};

// Zone::generate exterior-post switch at base ,5629,5658 calls angularPillar/angularColumn/roundPillar/underpinnedBox
// respectively. Same style is repeated at all four corners.
enum PillarStyle {
    PillarStyle_AngularPillar = 0,
    PillarStyle_AngularColumn = 1,
    PillarStyle_RoundPillar = 2,
    PillarStyle_SquarePost = 3,
};

// Zone::generate domed-hall switches at base call angularPillar/roundPillar/angularColumn. Distinct from exterior
// PillarStyle: values 1 and 2 are reversed.
enum HallPillarStyle {
    HallPillarStyle_AngularPillar = 0,
    HallPillarStyle_RoundPillar = 1,
    HallPillarStyle_AngularColumn = 2,
};

// Known building styles; values 7 and above 12 have no known use.
enum StructureCellKind {
    StructureCell_None = 0,
    StructureCell_Room = 1,
    StructureCell_Guardian = 2,
    StructureCell_LootChest = 3,
    StructureCell_EndBoss = 4,
    StructureCell_RewardChest = 5,
    StructureCell_Enemy = 6,
    StructureCell_Doorway = 8,
    StructureCell_DeepPit = 9,
    StructureCell_WaterPool = 10,
    StructureCell_LavaPool = 11,
    StructureCell_MuckPool = 12,
};

// These are positional room types; names are inferred from all produced cells.
enum CorridorKind {
    Corridor_None = 0,
    Corridor_Room = 1,
    Corridor_FirstRoom = 2,
    Corridor_SecondRoom = 3,
    Corridor_GuardianRoom = 4,
    Corridor_LootRoom = 5,
    Corridor_EndBossRoom = 6,
    Corridor_RewardRoom = 7,
    Corridor_EnemyRoom = 8,
    Corridor_CrossPit = 9,
    Corridor_CrossWater = 10,
    Corridor_CrossLava = 11,
    Corridor_CrossMuck = 12,
    Corridor_RimPit = 13,
    Corridor_RimLava = 14,
    Corridor_RimMuck = 15,
};

// Dungeon room styles.
enum DungeonCellStyle {
    DungeonCellStyle_Standard = 0,
    DungeonCellStyle_Wide = 1,
    DungeonCellStyle_Overgrown = 2,
};

// Room parts generated by Zone: gates, locked or rune gates, mission rooms, central pillars (1), raised walks (7),
// daises (8), crossbeams with optional lamps (11) and stalactites (12). Part 13 creates creatures whose defeat
// activates a gate. Part 14 marks the first room's corner; its other meaning is unknown. Values 9 and 10 have no known
// use.
enum RoomPart {
    RoomPart_None = 0,
    RoomPart_CentralPillar = 1,
    RoomPart_Gate = 2,
    RoomPart_LockedGate = 3,
    RoomPart_RuneGate = 4,
    RoomPart_MissionRoom = 5,
    RoomPart_EmptyGate = 6,
    RoomPart_RaisedWalk = 7,
    RoomPart_Dais = 8,
    RoomPart_CrossBeam = 11,
    RoomPart_Stalactite = 12,
    RoomPart_Activator = 13,
    RoomPart_EntranceMarker = 14,
};

// StructureBuilder::toZoneRotation and PlantBuilder::wallPlant transform local XY coordinates by the number of
// quarter-turns. Enumerators state a count, not an assumed compass origin. World::castlePart rotation is excluded:
// case 2 reflects both coordinates with a different formula.
enum QuarterTurn {
    QuarterTurn_Zero = 0,
    QuarterTurn_One = 1,
    QuarterTurn_Two = 2,
    QuarterTurn_Three = 3,
};

// Distinct from StructureCell.side, which carries packed length/level.
enum CorridorSide {
    CorridorSide_Any = 0,
    CorridorSide_West = 1,
    CorridorSide_East = 2,
    CorridorSide_South = 3,
    CorridorSide_North = 4,
};

// These zero-based neighbour-loop indices differ from CorridorSide by one.
enum NeighbourSide {
    NeighbourSide_West = 0,
    NeighbourSide_East = 1,
    NeighbourSide_South = 2,
    NeighbourSide_North = 3,
};

// Value 2 reflects the placement rather than rotating it by half a turn.
enum CastleTransform {
    CastleTransform_Identity = 0,
    CastleTransform_Clockwise = 1,
    CastleTransform_AntiDiagonalReflection = 2,
    CastleTransform_Counterclockwise = 3,
};

enum JoinedSide {
    JoinedSideMinusX = 1, // isJoined(x, y, x - 1, y)
    JoinedSidePlusX = 2,  // isJoined(x, y, x + 1, y)
    JoinedSideMinusY = 4, // isJoined(x, y, x, y - 1)
    JoinedSidePlusY = 8   // isJoined(x, y, x, y + 1)
};

enum CorridorFlag {
    CorridorFlagPillars = 0x1,     // permits central pillars
    CorridorFlagRaisedWalks = 0x2, // permits raised walks
    CorridorFlagUnevenFloor = 0x4, // adds deterministic height variation
};

enum HeightFlag {
    HeightFlagSkipPlaceShape = 0x1,    // skip local place shaping
    HeightFlagSkipPlaceAndHills = 0x2, // skip place shaping, hills and type-10 rise
    HeightFlagCaveMouths = 0x4,        // lower height to cave-density boundary
    HeightFlagSkipRiverBeds = 0x8,     // skip river bed and beach cuts
};

enum ZoneStateFlag {
    ZoneStateFlagItems = 0x1,          // saved item vector replaces generated items
    ZoneStateFlagStaticEntities = 0x2, // saved entity vector replaces generated entities
};

}
