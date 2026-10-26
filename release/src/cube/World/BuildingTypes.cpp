// Building dimensions, styles and colours shared by world, zone and map generation; declarations are in cube/World.h.
#include "cube/Noise.h"
#include "cube/World.h"
#include "cube/WorldIds.h"
#include "plasma/EditFilter.h"

namespace cube {

// A realm's buildings vary by hashes of the realm, hash(realm + salt), with a salt of its own for each choice
// (battlements 12003, the roof 9063, the house style 8983, the trim 80981, the band 3413, the tower radius 98133, and
// one per colour channel); the salts only decorrelate the choices. The colours (buildingTrimColor, roofColor,
// buildingColor, styleColor) are hand-authored palettes: a base and a hashed range per channel, by building type, kind
// and house style.

int hasBattlements(int type, int realm, int form, int biome)
{
    if (type == BuildingTower)
        return (hash(realm + 12003) & 1) == 0;
    if (!(type >= BuildingDungeonDomedHall && type <= BuildingCastleTower && type != BuildingTerrace) &&
        buildingKind(type) != BuildingKindGuildHall)
        return 0;
    return 1;
}

bool hasRoof(int type, int realm, int form, int biome)
{
    if (type == BuildingTower)
        return (hash(realm + 9063) & 1) != 0;
    return type == BuildingDungeonTower || type == BuildingCastleTower || type == BuildingRuinedPavilion ||
           type == BuildingShrineOfLife || type == BuildingFloatingIslandTreasurePavilion ||
           type == BuildingShrineEntrance || type == BuildingShrineDescent10 || type == BuildingShrineDescent20 ||
           type == BuildingShrineDescent30 || type == BuildingShrineDescent40 || type == BuildingMineEntrance ||
           type == BuildingMineDescent10 || type == BuildingMineDescent20 || type == BuildingMineDescent30 ||
           type == BuildingMineDescent40 || type == BuildingVaultEntrance || type == BuildingVaultDescent10 ||
           type == BuildingVaultDescent20 || type == BuildingVaultDescent30 || type == BuildingVaultDescent40 ||
           type == BuildingSewerOrGrottoEntrance || type == BuildingOldHutRoom || type == BuildingOldHutChimney ||
           type == BuildingOldHutEntrance || type == BuildingGuildHall || type == BuildingGuildHallCentre ||
           type == BuildingTempleEntrance || type == BuildingArenaTower || type == BuildingSandstoneTreasureChamber ||
           type == BuildingRockLorePavilion || type == BuildingHouseRoom || type == BuildingHouseUpperRoom ||
           type == BuildingHouseEntrance || type == BuildingHousePavilion || type == BuildingCryptRoom ||
           type == BuildingCryptEntrance || type == BuildingLoreCrypt || type == BuildingCryptUpperRoom ||
           type == BuildingCatacombsEntrance || type == BuildingDungeonDomedHall;
}

bool hasWalls(int type)
{
    if (type != BuildingRockLorePavilion && type != BuildingTreasurePavilion && type != BuildingHousePavilion &&
        type != BuildingHouseCourtyard && type != BuildingRuinedPavilion && type != BuildingRuinedFlatRoofPavilion &&
        type != BuildingBirdStatuePavilion && type != BuildingShrineOfLife &&
        type != BuildingFloatingIslandTreasurePavilion && type != BuildingSewerOrGrottoEntrance &&
        type != BuildingPyramidEntrance && type != BuildingTempleEntrance &&
        buildingKind(type) != BuildingKindGuildHall)
        return true;
    return false;
}

int houseStyle(int realm, int form, int biome)
{
    switch (biome) {
    case BiomeDesert:
        return HouseStyle_DesertFlatRoof;
    case BiomeJungle:
        if (hash(realm + 8983) & 1)
            return HouseStyle_SavannahLowRoof;
        return HouseStyle_DesertFlatRoof;
    case BiomeSavannah:
        return HouseStyle_SavannahLowRoof;
    case BiomeDarkWoods:
    case BiomeDeadlands:
        return HouseStyle_DarkSteepRoof;
    case BiomeMountains:
        return HouseStyle_MountainSlabRoof;
    case BiomeWoodlands:
    case BiomeSnowForest:
        if (hash(realm + 8983) & 1)
            return HouseStyle_PaintedTimber;
        return HouseStyle_Timber;
    default:
        switch (hash(realm + 8983) & 3) {
        case 1:
            return HouseStyle_PaintedTimber;
        case 2:
            return HouseStyle_MountainSlabRoof;
        case 3:
            return HouseStyle_Timber;
        default:
            return HouseStyle_PlainFrame;
        }
    }
}

// Whether a zone building type is one of the three castle parts (45, 46 and 47: castleZoneType).
bool isCastleBuilding(int type)
{
    return type >= BuildingCastleCell && type <= BuildingCastleTower;
}

// Whether a zone building type is one of 40..43 (dungeonType gives 40, 41 and 43): structureLevel takes the highest
// terrain of the 32-block square for them.
bool isDungeonBuilding(int type)
{
    return type >= BuildingDungeonDomedHall && type <= BuildingDungeonTower;
}

// Whether a zone building type is at most 12 (the street buildings and no building): they stand on the terrain.
bool isPlainBuilding(int type)
{
    return type <= BuildingTallRock;
}

bool isJoinedCastle(int type, int neighbourType)
{
    if (type < BuildingDungeonDomedHall || type > BuildingCastleTower || type == BuildingTerrace ||
        neighbourType < BuildingDungeonDomedHall || neighbourType > BuildingCastleTower ||
        neighbourType == BuildingTerrace)
        return false;
    return buildingHeight(type) <= buildingHeight(neighbourType);
}

bool hasTowerTrim(int type, int realm, int form, int biome)
{
    return buildingKind(type) == BuildingKindStonePavilion && (hash(realm + 80981) & 1) != 0;
}

bool skipsCellDecorations(int type)
{
    return type == BuildingCatacombsEntrance || type == BuildingShrineDescent40 || type == BuildingMineDescent40 ||
           type == BuildingVaultDescent40 || type == BuildingSewerOrGrottoEntrance || type == BuildingPyramidEntrance ||
           type == BuildingTempleEntrance;
}

bool hasTowerBand(int type, int realm, int form, int biome)
{
    if (buildingKind(type) == BuildingKindStonePavilion && type != BuildingRockLorePavilion &&
        type != BuildingBirdStatuePavilion && type != BuildingShrineOfLife &&
        type != BuildingFloatingIslandTreasurePavilion && (hash(realm + 3413) & 1))
        return true;
    return false;
}

int realmTowerRadius(int realm)
{
    // 20 to 32 blocks.
    return hash(realm + 98133) % 13 + 20;
}

plasma::Vector<3, float> buildingTrimColor(int type, int x, int y, int realm, int form, int biome)
{
    if (type >= BuildingCastleCell && type <= BuildingCastleTower) {
        switch (hash(realm + 233003) % 3) {
        case 1: {
            return buildingColor(type, x, y, realm, form, biome) * 0.25f + plasma::Vector<3, float>(22.5f, 60, 22.5f);
        }
        case 2: {
            unsigned int shade = hash(realm + 9093) % 150 + 100;
            return plasma::Vector<3, float>((float)(shade + hash(realm + 2202) % 50),
                                            (float)(shade + hash(realm + 344) % 50),
                                            (float)(shade + hash(realm + 2355) % 50));
        }
        }
        return buildingColor(type, x, y, realm, form, biome);
    }
    switch (buildingKind(type)) {
    case BuildingKindHouse:
    case BuildingKindGuildHall:
        switch (houseStyle(realm, form, biome)) {
        case HouseStyle_PaintedTimber:
            return buildingColor(type, x, y, realm, form, biome) * 0.8f;
        case HouseStyle_Timber:
            return buildingColor(type, x, y, realm, form, biome) * 0.8f;
        case HouseStyle_DarkSteepRoof: {
            int shade = hash(realm + 70208) % 40 + 40;
            return plasma::Vector<3, float>((float)(shade + hash(realm + 6352) % 30),
                                            (float)(shade + hash(realm + 123) % 30),
                                            (float)(shade + hash(realm + 330) % 30));
        }
        case HouseStyle_DesertFlatRoof:
            return plasma::Vector<3, float>((float)(hash(realm + 88732) % 30 + 250),
                                            (float)(hash(realm + 96325) % 30 + 240),
                                            (float)(hash(realm + 63664) % 30 + 230));
        case HouseStyle_SavannahLowRoof: {
            return buildingColor(type, x, y, realm, form, biome) * 0.75f + plasma::Vector<3, float>(15, 30, 15);
        }
        default: {
            int shade = hash(realm + 108837) % 100 + 100;
            return plasma::Vector<3, float>((float)(shade + hash(realm + 6352) % 30),
                                            (float)(shade + hash(realm + 88734) % 30),
                                            (float)(shade + hash(realm + 295) % 30));
        }
        }
    default:
        if (type != BuildingTower)
            return buildingColor(type, x, y, realm, form, biome);
        switch (hash(realm + 37223) % 3) {
        case 1: {
            return buildingColor(type, x, y, realm, form, biome) * 0.25f + plasma::Vector<3, float>(22.5f, 60, 22.5f);
        }
        case 2: {
            unsigned int shade = hash(realm + 6667) % 150 + 100;
            return plasma::Vector<3, float>((float)(shade + hash(realm + 101011) % 50),
                                            (float)(shade + hash(realm + 2044) % 50),
                                            (float)(shade + hash(realm + 5432) % 50));
        }
        }
        return buildingColor(type, x, y, realm, form, biome);
    case BuildingKindStonePavilion:
        switch (hash(realm + 37223) % 3) {
        case 1: {
            return buildingColor(type, x, y, realm, form, biome) * 0.75f + plasma::Vector<3, float>(15, 30, 15);
        }
        case 2: {
            unsigned int shade = hash(realm + 6667) % 150 + 100;
            return plasma::Vector<3, float>((float)(shade + hash(realm + 101011) % 50),
                                            (float)(shade + hash(realm + 2044) % 50),
                                            (float)(shade + hash(realm + 5432) % 50));
        }
        }
        return buildingColor(type, x, y, realm, form, biome);
    }
}

// The height of one storey of a zone building type, in blocks; 0 for a type without storeys.
int buildingStoreyHeight(int type)
{
    switch (type) {
    case BuildingHouseRoom:
    case BuildingHouseUpperRoom:
    case BuildingHouseEntrance:
    case BuildingHousePavilion:
    case BuildingHouseCourtyard:
    case BuildingOldHutRoom:
    case BuildingOldHutChimney:
    case BuildingOldHutEntrance:
        return 8;
    case BuildingTreasurePavilion:
    case BuildingCryptRoom:
    case BuildingCryptEntrance:
    case BuildingLoreCrypt:
    case BuildingCryptUpperRoom:
    case BuildingCatacombsEntrance:
    case BuildingRuinedPavilion:
    case BuildingRuinedFlatRoofPavilion:
    case BuildingBirdStatuePavilion:
    case BuildingShrineOfLife:
    case BuildingAltar:
    case BuildingMourningStatuePavilion:
    case BuildingLakePlatform:
    case BuildingRaisedPostPlatform:
    case BuildingMountainPlatform:
    case BuildingShrineEntrance:
    case BuildingShrineDescent10:
    case BuildingShrineDescent20:
    case BuildingShrineDescent30:
    case BuildingShrineDescent40:
    case BuildingMineEntrance:
    case BuildingMineDescent10:
    case BuildingMineDescent20:
    case BuildingMineDescent30:
    case BuildingMineDescent40:
    case BuildingVaultEntrance:
    case BuildingVaultDescent10:
    case BuildingVaultDescent20:
    case BuildingVaultDescent30:
    case BuildingVaultDescent40:
    case BuildingGuildHall:
    case BuildingGuildHallCentre:
    case BuildingArenaTower:
        return 10;
    case BuildingRockLorePavilion:
    case BuildingDungeonDomedHall:
    case BuildingDungeonTower:
    case BuildingFloatingIslandTreasurePavilion:
    case BuildingSewerOrGrottoEntrance:
    case BuildingPyramidEntrance:
    case BuildingTempleEntrance:
    case BuildingSandstoneTreasureChamber:
        return 15;
    case BuildingCastleCell:
    case BuildingCastleGate:
    case BuildingCastleTower:
        return 25;
    default:
        return 0;
    }
}

// The number of storeys (1..3) of the building types that have them.
int buildingStoreys(int type)
{
    switch (type) {
    case BuildingRockLorePavilion:
    case BuildingHouseRoom:
    case BuildingHouseEntrance:
    case BuildingHousePavilion:
    case BuildingHouseCourtyard:
    case BuildingCryptRoom:
    case BuildingCryptEntrance:
    case BuildingLoreCrypt:
    case BuildingCatacombsEntrance:
    case BuildingCastleCell:
    case BuildingCastleGate:
    case BuildingRuinedPavilion:
    case BuildingRuinedFlatRoofPavilion:
    case BuildingBirdStatuePavilion:
    case BuildingShrineOfLife:
    case BuildingInscribedMenhir:
    case BuildingManaWell:
    case BuildingShrineOfFinesse:
    case BuildingFloatingIslandTreasurePavilion:
    case BuildingShrineEntrance:
    case BuildingShrineDescent10:
    case BuildingShrineDescent20:
    case BuildingShrineDescent30:
    case BuildingShrineDescent40:
    case BuildingMineEntrance:
    case BuildingMineDescent10:
    case BuildingMineDescent20:
    case BuildingMineDescent30:
    case BuildingMineDescent40:
    case BuildingVaultEntrance:
    case BuildingVaultDescent10:
    case BuildingVaultDescent20:
    case BuildingVaultDescent30:
    case BuildingVaultDescent40:
    case BuildingSewerOrGrottoEntrance:
    case BuildingOldHutRoom:
    case BuildingOldHutChimney:
    case BuildingOldHutEntrance:
    case BuildingGuildHall:
    case BuildingGuildHallCentre:
    case BuildingPyramidEntrance:
    case BuildingTempleEntrance:
    case BuildingSandstoneTreasureChamber:
        return 1;
    case BuildingHouseUpperRoom:
    case BuildingCryptUpperRoom:
    case BuildingDungeonDomedHall:
    case BuildingCastleTower:
        return 2;
    case BuildingDungeonTower:
    case BuildingArenaTower:
        return 3;
    default:
        return 0;
    }
}

// Structure-level adjustment used by isJoined; defaults to 8 for unlisted types.
int buildingHeight(int type)
{
    switch (type) {
    case BuildingHouseUpperRoom:
        return 16;
    case BuildingTreasurePavilion:
    case BuildingCryptRoom:
    case BuildingCryptEntrance:
    case BuildingLoreCrypt:
    case BuildingCatacombsEntrance:
    case BuildingRuinedPavilion:
    case BuildingRuinedFlatRoofPavilion:
    case BuildingBirdStatuePavilion:
    case BuildingShrineOfLife:
    case BuildingAltar:
    case BuildingMourningStatuePavilion:
    case BuildingLakePlatform:
    case BuildingRaisedPostPlatform:
    case BuildingMountainPlatform:
    case BuildingShrineEntrance:
    case BuildingShrineDescent10:
    case BuildingShrineDescent20:
    case BuildingShrineDescent30:
    case BuildingShrineDescent40:
    case BuildingMineEntrance:
    case BuildingMineDescent10:
    case BuildingMineDescent20:
    case BuildingMineDescent30:
    case BuildingMineDescent40:
    case BuildingVaultDescent10:
    case BuildingVaultDescent20:
    case BuildingVaultDescent30:
    case BuildingVaultDescent40:
    case BuildingGuildHall:
        return 10;
    case BuildingRockLorePavilion:
    case BuildingFloatingIslandTreasurePavilion:
    case BuildingSewerOrGrottoEntrance:
    case BuildingPyramidEntrance:
    case BuildingTempleEntrance:
    case BuildingSandstoneTreasureChamber:
        return 15;
    case BuildingSanctuary:
        return 18;
    case BuildingDungeonDomedHall:
    case BuildingDungeonHall:
    case BuildingDungeonHall42:
    case BuildingCastleCell:
    case BuildingCastleGate:
    case BuildingRaisedSanctuary:
        return 25;
    case BuildingVaultEntrance:
        return 35;
    case BuildingCryptUpperRoom:
        return 20;
    case BuildingDungeonTower:
    case BuildingCastleTower:
    case BuildingArenaTower:
        return 60;
    case BuildingGuildHallCentre:
        return 30;
    case BuildingHouseRoom:
    case BuildingHouseEntrance:
    case BuildingHousePavilion:
    case BuildingHouseCourtyard:
    case BuildingOldHutRoom:
    case BuildingOldHutChimney:
    case BuildingOldHutEntrance:
        return 8;
    default:
        return 8;
    }
}

// How many 16-block cells around its own a building type reaches.
int buildingReach(int type)
{
    switch (type) {
    case BuildingOak:
    case BuildingEvergreen:
    case BuildingBirch:
    case BuildingJungleTree:
    case BuildingPalm:
    case BuildingOakPlaza:
    case BuildingPalmPlaza:
    case BuildingGreatOak:
    case BuildingBoulder:
    case BuildingHill:
    case BuildingRock:
    case BuildingTallRock:
    case BuildingRockLorePavilion:
    case BuildingRockTreasure:
    case BuildingCave:
    case BuildingTower:
    case BuildingShrineOfLife:
    case BuildingFloatingIslandTreasurePavilion:
    case BuildingManaPump:
    case BuildingSanctuary:
    case BuildingRaisedSanctuary:
        return 2;
    case BuildingTreasurePavilion:
    case BuildingHouseRoom:
    case BuildingHouseUpperRoom:
    case BuildingHouseEntrance:
    case BuildingHousePavilion:
    case BuildingCryptRoom:
    case BuildingCryptEntrance:
    case BuildingLoreCrypt:
    case BuildingCryptUpperRoom:
    case BuildingCatacombsEntrance:
    case BuildingDungeonDomedHall:
    case BuildingDungeonHall:
    case BuildingDungeonHall42:
    case BuildingDungeonTower:
    case BuildingCastleCell:
    case BuildingCastleGate:
    case BuildingCastleTower:
    case BuildingRuinedPavilion:
    case BuildingRuinedFlatRoofPavilion:
    case BuildingBirdStatuePavilion:
    case BuildingMenhir:
    case BuildingInscribedMenhir:
    case BuildingAltar:
    case BuildingMourningStatuePavilion:
    case BuildingBuriedTreasureChamber:
    case BuildingShrineEntrance:
    case BuildingShrineDescent10:
    case BuildingShrineDescent20:
    case BuildingShrineDescent30:
    case BuildingShrineDescent40:
    case BuildingMineEntrance:
    case BuildingMineDescent10:
    case BuildingMineDescent20:
    case BuildingMineDescent30:
    case BuildingMineDescent40:
    case BuildingVaultEntrance:
    case BuildingVaultDescent10:
    case BuildingVaultDescent20:
    case BuildingVaultDescent30:
    case BuildingVaultDescent40:
    case BuildingSewerOrGrottoEntrance:
    case BuildingOldHutRoom:
    case BuildingOldHutChimney:
    case BuildingOldHutEntrance:
    case BuildingGuildHall:
    case BuildingGuildHallCentre:
    case BuildingTempleEntrance:
    case BuildingArenaTower:
    case BuildingSandstoneTreasureChamber:
        return 1;
    default:
        return 0;
    }
}

plasma::Vector<3, float> roofColor(int type, int x, int y, int realm, int form, int biome)
{
    switch (type) {
    case BuildingTower:
        return plasma::Vector<3, float>((float)(hash(realm + 3981) % 200 + 50), (float)(hash(realm + 3341) % 200 + 50),
                                        (float)(hash(realm + 5833) % 200 + 50));
    case BuildingCastleTower:
        return plasma::Vector<3, float>(50, 50, 220);
    case BuildingHousePavilion:
        return styleColor(type, realm, form, biome);
    case BuildingInscribedMenhir:
        return plasma::Vector<3, float>(200, 200, 200);
    case BuildingLakePlatform:
    case BuildingRaisedPostPlatform:
    case BuildingMountainPlatform:
        return plasma::Vector<3, float>(160, 80, 10);
    case BuildingPyramidEntrance:
        return plasma::Vector<3, float>(260, 250, 130);
    case BuildingTempleEntrance:
        return plasma::Vector<3, float>(180, 170, 90);
    case BuildingArenaTower:
    case BuildingArenaPodium:
        return plasma::Vector<3, float>(120, 110, 90);
    default:
        switch (buildingKind(type)) {
        case BuildingKindCrypt:
            return plasma::Vector<3, float>(135, 130, 140);
        case BuildingKindStonePavilion:
        case BuildingKindDungeonShaftEntrance:
            return buildingColor(type, x, y, realm, form, biome);
        case BuildingKindOldHut:
            return plasma::Vector<3, float>(60, 40, 10);
        case BuildingKindSandstone:
            return plasma::Vector<3, float>(180, 170, 90);
        case BuildingKindHouse:
        case BuildingKindGuildHall:
            switch (houseStyle(realm, form, biome)) {
            case HouseStyle_Timber:
                return styleColor(type, realm, form, biome) * 0.7f;
            case HouseStyle_DarkSteepRoof:
                return plasma::Vector<3, float>((float)(hash(realm + 8371) % 20 + 50),
                                                (float)(hash(realm + 657) % 20 + 50),
                                                (float)(hash(realm + 4366) % 20 + 50));
            case HouseStyle_DesertFlatRoof:
                return buildingColor(type, x, y, realm, form, biome);
            case HouseStyle_SavannahLowRoof:
                return plasma::Vector<3, float>((float)(hash(realm + 8371) % 50 + 200),
                                                (float)(hash(realm + 657) % 100 + 100),
                                                (float)(hash(realm + 4366) % 30 + 50));
            default:
                switch (hash(realm + 7163) % 3) {
                case 1:
                    return plasma::Vector<3, float>((float)(hash(realm + 8371) % 20 + 100),
                                                    (float)(hash(realm + 657) % 20 + 100),
                                                    (float)(hash(realm + 4366) % 100 + 150));
                case 2:
                    return plasma::Vector<3, float>((float)(hash(realm + 8371) % 50 + 130),
                                                    (float)(hash(realm + 657) % 30 + 100),
                                                    (float)(hash(realm + 4366) % 30 + 50));
                }
                return plasma::Vector<3, float>((float)(hash(realm + 8371) % 20 + 50),
                                                (float)(hash(realm + 657) % 20 + 50),
                                                (float)(hash(realm + 4366) % 20 + 50));
            }
        default:
            return plasma::Vector<3, float>(50, 50, 50);
        }
    }
}

// The kind 1..11 of a building type, 0 for none.
int buildingKind(int type)
{
    switch (type) {
    case BuildingHouseRoom:
    case BuildingHouseUpperRoom:
    case BuildingHouseEntrance:
    case BuildingHouseCourtyard:
        return BuildingKindHouse;
    case BuildingDungeonDomedHall:
    case BuildingDungeonHall:
    case BuildingDungeonHall42:
    case BuildingDungeonTower:
        return BuildingKindDungeonFortification;
    case BuildingCryptRoom:
    case BuildingCryptEntrance:
    case BuildingLoreCrypt:
    case BuildingCryptUpperRoom:
    case BuildingCatacombsEntrance:
        return BuildingKindCrypt;
    case BuildingRockLorePavilion:
    case BuildingRuinedPavilion:
    case BuildingRuinedFlatRoofPavilion:
    case BuildingBirdStatuePavilion:
    case BuildingShrineOfLife:
    case BuildingFloatingIslandTreasurePavilion:
    case BuildingBuriedTreasureChamber:
    case BuildingSanctuary:
    case BuildingRaisedSanctuary:
    case BuildingShrineEntrance:
    case BuildingShrineDescent10:
    case BuildingShrineDescent20:
    case BuildingShrineDescent30:
    case BuildingShrineDescent40:
        return BuildingKindStonePavilion;
    case BuildingMineEntrance:
    case BuildingMineDescent10:
    case BuildingMineDescent20:
    case BuildingMineDescent30:
    case BuildingMineDescent40:
        return BuildingKindTimberMineEntrance;
    case BuildingVaultEntrance:
    case BuildingVaultDescent10:
    case BuildingVaultDescent20:
    case BuildingVaultDescent30:
    case BuildingVaultDescent40:
    case BuildingSandstoneTreasureChamber:
        return BuildingKindSandstone;
    case BuildingCastleCell:
    case BuildingCastleGate:
    case BuildingCastleTower:
    case Building_48:
        return BuildingKindCastle;
    case BuildingSewerOrGrottoEntrance:
    case BuildingPyramidEntrance:
        return BuildingKindDungeonShaftEntrance;
    case BuildingOldHutRoom:
    case BuildingOldHutChimney:
    case BuildingOldHutEntrance:
        return BuildingKindOldHut;
    case BuildingGuildHall:
    case BuildingGuildHallCentre:
        return BuildingKindGuildHall;
    case BuildingTempleEntrance:
        return BuildingKindTempleEntrance;
    default:
        return BuildingKindUnclassified;
    }
}

plasma::Vector<3, float> buildingColor(int type, int x, int y, int realm, int form, int biome)
{
    if (type >= BuildingCastleCell && type <= BuildingCastleTower) {
        unsigned int shade = hash(realm + 3434) % 150 + 100;
        return plasma::Vector<3, float>((float)(shade + hash(realm + 98632) % 50),
                                        (float)(shade + hash(realm + 4234) % 50),
                                        (float)(shade + hash(realm + 49049) % 50));
    }
    switch (type) {
    case BuildingShrineOfLife:
        return plasma::Vector<3, float>(240, 250, 260);
    case BuildingArenaTower:
    case BuildingArenaPodium:
        return plasma::Vector<3, float>(180, 170, 120);
    case BuildingHousePavilion:
        return styleColor(type, realm, form, biome);
    case BuildingTower: {
        unsigned int shade = hash(realm + 708) % 150 + 100;
        return plasma::Vector<3, float>((float)(shade + hash(realm + 5323) % 50),
                                        (float)(shade + hash(realm + 123) % 50),
                                        (float)(shade + hash(realm + 223) % 50));
    }
    case BuildingManaPump:
    case BuildingManaPipe:
    case BuildingManaPipeEndpoint:
        return plasma::Vector<3, float>(128, 128, 140);
    case BuildingInscribedMenhir:
        return plasma::Vector<3, float>(200, 200, 200);
    case BuildingTreasurePavilion:
        return plasma::Vector<3, float>(140, 140, 140);
    case BuildingLakePlatform:
    case BuildingRaisedPostPlatform:
    case BuildingMountainPlatform:
        return plasma::Vector<3, float>(160, 80, 10);
    case BuildingPyramidEntrance:
        return plasma::Vector<3, float>(260, 250, 130);
    case BuildingTempleEntrance:
        return plasma::Vector<3, float>(180, 170, 90);
    case BuildingAltar:
    case BuildingMourningStatuePavilion:
        return plasma::Vector<3, float>(135, 130, 140);
    default:
        switch (buildingKind(type)) {
        case BuildingKindStonePavilion: {
            unsigned int shade = hash(realm + 708) % 150 + 100;
            return plasma::Vector<3, float>((float)(shade + hash(realm + 5323) % 50),
                                            (float)(shade + hash(realm + 123) % 50),
                                            (float)(shade + hash(realm + 223) % 50));
        }
        case BuildingKindDungeonShaftEntrance:
            return plasma::Vector<3, float>(200, 200, 200);
        case BuildingKindSandstone:
            return plasma::Vector<3, float>(180, 170, 90);
        case BuildingKindCrypt:
            return plasma::Vector<3, float>(135, 130, 140);
        case BuildingKindTimberMineEntrance:
            return plasma::Vector<3, float>(200, 100, 20);
        case BuildingKindOldHut:
            return plasma::Vector<3, float>(80, 80, 80);
        case BuildingKindGuildHall: {
            int shade = hash(realm + 708) % 100 + 100;
            return plasma::Vector<3, float>((float)(shade + hash(realm + 5323) % 30),
                                            (float)(shade + hash(realm + 123) % 30),
                                            (float)(shade + hash(realm + 223) % 30));
        }
        case BuildingKindHouse:
            if (type != BuildingHouseCourtyard) {
                switch (houseStyle(realm, form, biome)) {
                case HouseStyle_PaintedTimber: {
                    // Painted in patches of 3 x 3 blocks, one patch in three with a colour of its own.
                    unsigned int cell = random(floorDivide(x, 3), floorDivide(y, 3));
                    if (cell % 3 == 0)
                        realm = cell / 3;
                    int red = hash(realm + 708) & 255;
                    return plasma::Vector<3, float>((float)red, (float)(hash(realm + 123) % 200 + 50),
                                                    (float)(255 - red)) *
                               0.5f +
                           plasma::Vector<3, float>(120, 120, 120);
                }
                case HouseStyle_Timber:
                case HouseStyle_SavannahLowRoof:
                    return styleColor(type, realm, form, biome);
                case HouseStyle_DarkSteepRoof: {
                    int shade = hash(realm + 708) % 40 + 40;
                    return plasma::Vector<3, float>((float)(shade + hash(realm + 5323) % 30),
                                                    (float)(shade + hash(realm + 123) % 30),
                                                    (float)(shade + hash(realm + 223) % 30));
                }
                case HouseStyle_DesertFlatRoof:
                    return plasma::Vector<3, float>((float)(hash(realm + 9083) % 30 + 250),
                                                    (float)(hash(realm + 5059) % 30 + 240),
                                                    (float)(hash(realm + 8773) % 30 + 230));
                }
            }
            {
                int shade = hash(realm + 708) % 100 + 100;
                return plasma::Vector<3, float>((float)(shade + hash(realm + 5323) % 30),
                                                (float)(shade + hash(realm + 123) % 30),
                                                (float)(shade + hash(realm + 223) % 30));
            }
        default:
            return plasma::Vector<3, float>(240, 240, 240);
        }
    }
}

plasma::Vector<3, float> styleColor(int type, int realm, int form, int biome)
{
    switch (houseStyle(realm, form, biome)) {
    case HouseStyle_DarkSteepRoof:
        return plasma::Vector<3, float>((float)(hash(realm + 8371) % 10 + 130), (float)(hash(realm + 657) % 10 + 120),
                                        (float)(hash(realm + 4366) % 10 + 110));
    case HouseStyle_SavannahLowRoof:
        return plasma::Vector<3, float>((float)(hash(realm + 8371) % 50 + 150), (float)(hash(realm + 657) % 30 + 120),
                                        (float)(hash(realm + 4366) % 30 + 90));
    default:
        return plasma::Vector<3, float>((float)(hash(realm + 8371) % 50 + 130), (float)(hash(realm + 657) % 30 + 100),
                                        (float)(hash(realm + 4366) % 30 + 50));
    }
}

}
