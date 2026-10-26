// See cube/SpawnTable.h.
#include "cube/ZoneIds.h"
#include <stdlib.h>
#include "cube/SpawnTable.h"
#include "cube/World.h"
#include "cube/CreatureIds.h"
#include "cube/WorldIds.h"

namespace cube {

// Fills the two tables. The indices of m_powers and the members of Group::creatureTypes are creature type numbers
// (EntityData::race values, RaceId), one power per race.
SpawnTable::SpawnTable(World *world)
    : m_world(world)
{
    m_powers.resize(RaceSupplierGnome + 1, 0);
    m_powers[RaceSkeleton] = 2;
    m_powers[RaceBonePile] = 2;
    m_powers[RaceCollie] = 4;
    m_powers[RaceShepherdDog] = 2;
    m_powers[RaceSkullBull] = 2;
    m_powers[RaceAlpaca] = 0;
    m_powers[RaceBrownAlpaca] = 1;
    m_powers[RaceTurtle] = 1;
    m_powers[RaceTerrier] = 0;
    m_powers[RaceScottishTerrier] = 0;
    m_powers[RaceWolf] = 2;
    m_powers[RacePanther] = 3;
    m_powers[RaceCat] = 0;
    m_powers[RaceBrownCat] = 0;
    m_powers[RaceWhiteCat] = 0;
    m_powers[RacePig] = 0;
    m_powers[RaceSheep] = 0;
    m_powers[RaceBunny] = 0;
    m_powers[RacePorcupine] = 2;
    m_powers[RaceGreenSlime] = 1;
    m_powers[RaceBlueSlime] = 2;
    m_powers[RacePinkSlime] = 3;
    m_powers[RaceYellowSlime] = 4;
    m_powers[RaceFrightener] = 3;
    m_powers[RaceSandHorror] = 3;
    m_powers[RaceOgre] = 2;
    m_powers[RaceRockling] = 2;
    m_powers[RaceMonkey] = 1;
    m_powers[RaceHornet] = 1;
    m_powers[RaceHornetsNest] = 1;
    m_powers[RaceInsectGuard] = 2;
    m_powers[RaceCrow] = 0;
    m_powers[RaceChicken] = 0;
    m_powers[RaceSeagull] = 0;
    m_powers[RaceParrot] = 1;
    m_powers[RaceBat] = 1;
    m_powers[RaceFly] = 0;
    m_powers[RaceMidge] = 2;
    m_powers[RaceMosquito] = 4;
    m_powers[RacePlainRunner] = 1;
    m_powers[RaceLeafRunner] = 2;
    m_powers[RaceSnowRunner] = 3;
    m_powers[RaceDesertRunner] = 4;
    m_powers[RacePeacock] = 1;
    m_powers[RaceFrog] = 1;
    m_powers[RacePlantCreature] = 1;
    m_powers[RacePlantCreatureSprout] = 0;
    m_powers[RaceRadishCreature] = 2;
    m_powers[RaceRadishCreatureSprout] = 0;
    m_powers[RaceOnionling] = 1;
    m_powers[RaceDesertOnionling] = 2;
    m_powers[RaceDevourer] = 3;
    m_powers[RaceDuckbill] = 0;
    m_powers[RaceCrocodile] = 3;
    m_powers[RaceDjinn] = 4;
    m_powers[RaceMinotaur] = 4;
    m_powers[RaceNomadMale] = 3;
    m_powers[RaceNomadFemale] = 3;
    m_powers[RaceImp] = 2;
    m_powers[RaceSpitter] = 2;
    m_powers[RaceMole] = 0;
    m_powers[RaceMolehill] = 0;
    m_powers[RaceBiter] = 1;
    m_powers[RaceKoala] = 1;
    m_powers[RaceSquirrel] = 2;
    m_powers[RaceRaccoon] = 2;
    m_powers[RaceOwl] = 2;
    m_powers[RacePenguin] = 2;
    m_powers[RaceWerewolf] = 3;
    m_powers[RaceZombie] = 1;
    m_powers[RaceUnholyTomb] = 1;
    m_powers[RaceVampire] = 4;
    m_powers[RaceHorse] = 1;
    m_powers[RaceCamel] = 1;
    m_powers[RaceCow] = 0;
    m_powers[RaceBarkBeetle] = 2;
    m_powers[RaceFireBeetle] = 3;
    m_powers[RaceSnoutBeetle] = 1;
    m_powers[RaceLemonBeetle] = 1;
    m_powers[RaceCrab] = 1;
    m_powers[RaceSeaCrab] = 2;
    m_powers[RaceSapphireFish] = 2;
    m_powers[RaceLemonFish] = 1;
    m_powers[RaceSeahorse] = 1;
    m_powers[RaceMermaid] = 3;
    m_powers[RaceMerman] = 3;
    m_powers[RaceShark] = 4;
    m_powers[RaceBumblebee] = 1;
    m_powers[RaceLanternFish] = 2;
    m_powers[RaceMawFish] = 3;
    m_powers[RacePiranha] = 3;
    m_powers[RaceBlowfish] = 2;
    m_powers[RaceBeaver] = 2;
    m_powers[RaceMoose] = 2;
    m_powers[RaceZebra] = 2;
    m_powers[RaceElephant] = 2;
    m_powers[RaceIceling] = 4;
    m_powers[RaceRhino] = 3;
    m_powers[RaceOstrich] = 1;
    m_powers[RaceLionMale] = 3;
    m_powers[RaceLionFemale] = 3;
    m_powers[RaceSkeletonKnight] = 3;
    m_powers[RaceSkeletonHorse] = 2;
    m_powers[RaceHyena] = 2;
    m_powers[RaceGiraffe] = 2;
    m_powers[RaceWarthog] = 2;
    m_powers[RaceAntelope] = 1;
    m_powers[RaceMakake] = 2;
    m_powers[RaceSkeletonDog] = 3;
    m_powers[RaceLavaTurtle] = 3;
    m_powers[RaceMantis] = 3;
    m_powers[RaceFlamingo] = 2;
    m_powers[RaceSavageDog] = 3;
    m_powers[RaceSnowLionMale] = 3;
    m_powers[RaceSnowLionFemale] = 3;
    m_powers[RaceSnowLeopard] = 2;
    m_powers[RaceAlphaDog] = 4;
    m_powers[RaceFlyingEye] = 2;
    m_powers[RaceDemonMarauder] = 3;
    m_powers[RaceCrawler] = 2;
    m_powers[RaceMonstrosity] = 3;
    m_powers[RaceSkullSlater] = 3;
    m_powers[RaceManaDeer] = 2;
    m_powers[RaceWraith] = 3;
    m_powers[RaceChiling] = 3;
    m_powers[RaceChilingSprout] = 1;
    m_powers[RaceHabanero] = 4;
    m_powers[RaceBloomling] = 1;
    m_powers[RaceCaterpillar] = 1;
    m_powers[RaceEarthCaterpillar] = 2;
    m_powers[RaceSnail] = 1;
    m_powers[RaceDarkJester] = 3;
    m_powers[RaceTreant] = 3;
    m_powers[RaceTroll] = 2;
    m_powers[RaceSaurian] = 2;
    m_powers[RaceCyclops] = 3;
    m_powers[RaceYeti] = 3;
    m_powers[RaceMammoth] = 3;
    m_powers[RaceDarkTroll] = 4;
    // { creature types }, size flags, terrain flags, location flags
    m_groups = {
        {{RaceSkeleton, RaceBonePile},
         SpawnGroupSizeSmall,
         SpawnTerrainUndead,
         SpawnLocationSpecial | SpawnLocationPlain},
        {{RaceCollie}, SpawnGroupSizeSmall, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceShepherdDog}, SpawnGroupSizeSmall, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceSkullBull}, SpawnGroupSizeSmall, SpawnTerrainPlain | SpawnTerrainUndead, SpawnLocationPlain},
        {{RaceAlpaca, RaceBrownAlpaca},
         SpawnGroupSizeSmall,
         SpawnTerrainPlain | SpawnTerrainMountain,
         SpawnLocationPlain},
        {{RaceTurtle},
         SpawnGroupSizeSingle,
         SpawnTerrainPlain | SpawnTerrainJungle | SpawnTerrainWoodlands,
         SpawnLocationRiverEdge | SpawnLocationRiver | SpawnLocationCoast | SpawnLocationUnderwater},
        {{RaceTerrier}, SpawnGroupSizeSmall, SpawnTerrainPlain | SpawnTerrainWoodlands, SpawnLocationPlain},
        {{RaceScottishTerrier}, SpawnGroupSizeSmall, SpawnTerrainPlain | SpawnTerrainMountain, SpawnLocationPlain},
        {{RaceWolf}, SpawnGroupSizeSmall, SpawnTerrainWoodlands, SpawnLocationPlain},
        {{RacePanther}, SpawnGroupSizeSingle, SpawnTerrainJungle, SpawnLocationPlain},
        {{RaceCat, RaceBrownCat, RaceWhiteCat}, SpawnGroupSizeSingle, SpawnTerrainPlain, SpawnLocationPlain},
        {{RacePig}, SpawnGroupSizeSmall, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceSheep}, SpawnGroupSizeSmall, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceBunny}, SpawnGroupSizeSmall, SpawnTerrainPlain | SpawnTerrainWoodlands, SpawnLocationPlain},
        {{RacePorcupine}, SpawnGroupSizeSmall, SpawnTerrainJungle | SpawnTerrain_64, SpawnLocationPlain},
        {{RaceGreenSlime}, SpawnGroupSizeSmall, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceBlueSlime}, SpawnGroupSizeSmall, 0, SpawnLocationRiverEdge | SpawnLocationRiver},
        {{RacePinkSlime}, SpawnGroupSizeSmall, 0, SpawnLocationSpecial},
        {{RaceYellowSlime}, SpawnGroupSizeSmall, 0, SpawnLocationPoisonRiver | SpawnLocationPoisonRiverEdge},
        {{RaceFrightener}, SpawnGroupSizeSmall, SpawnTerrainUndead, SpawnLocationPlain},
        {{RaceSandHorror}, SpawnGroupSizeSmall, SpawnTerrainDesert, SpawnLocationPlain},
        {{RaceOgre},
         SpawnGroupSizeSmall,
         SpawnTerrainPlain | SpawnTerrainMountain | SpawnTerrain_64 | SpawnTerrainWoodlands,
         SpawnLocationPlain},
        {{RaceRockling}, SpawnGroupSizeSmall, 0, SpawnLocationCliffs},
        {{RacePolarGnoll}, SpawnGroupSizeSmallLeader, SpawnTerrainSnow, SpawnLocationSpecial | SpawnLocationPlain},
        {{RaceMonkey}, SpawnGroupSizeSmall, SpawnTerrainJungle | SpawnTerrain_64, SpawnLocationStreetBuilding},
        {{RaceHornet, RaceHornetsNest}, SpawnGroupSizeSmall, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceInsectGuard}, SpawnGroupSizeSmall, SpawnTerrainPlain | SpawnTerrainWoodlands, SpawnLocationPlain},
        {{RaceCrow},
         SpawnGroupSizeSmall,
         SpawnTerrainPlain | SpawnTerrainSnow | SpawnTerrainUndead | SpawnTerrainWoodlands,
         SpawnLocationPlain},
        {{RaceChicken}, SpawnGroupSizeSmall, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceSeagull}, SpawnGroupSizeSmall, 0, SpawnLocationCoast},
        {{RaceParrot}, SpawnGroupSizeSmall, SpawnTerrainJungle, SpawnLocationStreetBuilding},
        {{RaceBat}, SpawnGroupSizeSmall, 0, SpawnLocationSpecial},
        {{RaceFly}, SpawnGroupSizeLarge, SpawnTerrainPlain | SpawnTerrainWoodlands, SpawnLocationRiverEdge},
        {{RaceMidge}, SpawnGroupSizeLarge, SpawnTerrainPlain | SpawnTerrainWoodlands, SpawnLocationRiverEdge},
        {{RaceMosquito},
         SpawnGroupSizeLarge,
         SpawnTerrainDesert | SpawnTerrainJungle | SpawnTerrain_64,
         SpawnLocationRiverEdge},
        {{RacePlainRunner}, SpawnGroupSizeSmall, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceLeafRunner}, SpawnGroupSizeSmall, SpawnTerrainJungle, SpawnLocationPlain},
        {{RaceSnowRunner}, SpawnGroupSizeSmall, SpawnTerrainSnow, SpawnLocationPlain},
        {{RaceDesertRunner}, SpawnGroupSizeSmall, SpawnTerrainDesert, SpawnLocationPlain},
        {{RacePeacock}, SpawnGroupSizeSmall, SpawnTerrainJungle, SpawnLocationRiverEdge},
        {{RaceFrog},
         SpawnGroupSizeSmall,
         SpawnTerrainPlain | SpawnTerrainDesert | SpawnTerrainJungle | SpawnTerrain_64 | SpawnTerrainWoodlands,
         SpawnLocationRiverEdge | SpawnLocationRiver},
        {{RacePlantCreature, RacePlantCreatureSprout}, SpawnGroupSizeSmall, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceRadishCreature, RaceRadishCreatureSprout}, SpawnGroupSizeSmall, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceOnionling}, SpawnGroupSizeSmall, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceDesertOnionling}, SpawnGroupSizeSmall, SpawnTerrainDesert, SpawnLocationPlain},
        {{RaceDevourer}, SpawnGroupSizeSmall, SpawnTerrainLava, SpawnLocationPlain},
        {{RaceDuckbill}, SpawnGroupSizeSingle, SpawnTerrainPlain, SpawnLocationRiverEdge | SpawnLocationRiver},
        {{RaceCrocodile},
         SpawnGroupSizeSmall,
         SpawnTerrainDesert | SpawnTerrainJungle | SpawnTerrain_64,
         SpawnLocationRiverEdge | SpawnLocationRiver},
        {{RaceDjinn}, SpawnGroupSizeSingle, SpawnTerrainDesert | SpawnTerrainJungle, SpawnLocationPlain},
        {{RaceMinotaur}, SpawnGroupSizeSingle, SpawnTerrainLava, SpawnLocationPlain},
        {{RaceImp}, SpawnGroupSizeSmall, SpawnTerrainLava, SpawnLocationPlain},
        {{RaceSpitter},
         SpawnGroupSizeSmall,
         0,
         SpawnLocationRiverEdge | SpawnLocationRiver | SpawnLocationCoast | SpawnLocationUnderwater},
        {{RaceMole, RaceMolehill}, SpawnGroupSizeSmall, SpawnTerrainPlain | SpawnTerrainWoodlands, SpawnLocationPlain},
        {{RaceBiter}, SpawnGroupSizeSmall, SpawnTerrainPlain | SpawnTerrainWoodlands, SpawnLocationPlain},
        {{RaceTreant}, SpawnGroupSizeSingle, SpawnTerrainWoodlands, SpawnLocationPlain},
        {{RaceKoala}, SpawnGroupSizeSmall, SpawnTerrainJungle | SpawnTerrain_64, SpawnLocationStreetBuilding},
        {{RaceSquirrel}, SpawnGroupSizeSmall, SpawnTerrainPlain | SpawnTerrainWoodlands, SpawnLocationStreetBuilding},
        {{RaceRaccoon}, SpawnGroupSizeSmall, SpawnTerrainWoodlands, SpawnLocationRiverEdge},
        {{RaceOwl}, SpawnGroupSizeSingle, SpawnTerrainUndead | SpawnTerrainWoodlands, SpawnLocationStreetBuilding},
        {{RacePenguin}, SpawnGroupSizeSmall, SpawnTerrainSnow, SpawnLocationRiverEdge | SpawnLocationCoast},
        {{RaceWerewolf}, SpawnGroupSizeSingle, SpawnTerrainUndead, SpawnLocationPlain},
        {{RaceZombie}, SpawnGroupSizeLarge, SpawnTerrainUndead, SpawnLocationSpecial | SpawnLocationPlain},
        {{RaceUnholyTomb}, SpawnGroupSizeSmall, SpawnTerrainUndead, SpawnLocationPlain},
        {{RaceVampire}, SpawnGroupSizeSingle, SpawnTerrainUndead, SpawnLocationPlain},
        {{RaceHorse}, SpawnGroupSizeSmall, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceCamel}, SpawnGroupSizeSmall, SpawnTerrainDesert, SpawnLocationPlain},
        {{RaceCow}, SpawnGroupSizeSmall, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceBarkBeetle}, SpawnGroupSizeSmall, SpawnTerrainPlain | SpawnTerrainWoodlands, SpawnLocationStreetBuilding},
        {{RaceFireBeetle}, SpawnGroupSizeSmall, SpawnTerrainLava, SpawnLocationPlain},
        {{RaceSnoutBeetle}, SpawnGroupSizeSmall, SpawnTerrainPlain | SpawnTerrainWoodlands, SpawnLocationPlain},
        {{RaceLemonBeetle}, SpawnGroupSizeSmall, SpawnTerrainPlain | SpawnTerrainWoodlands, SpawnLocationPlain},
        {{RaceCrab}, SpawnGroupSizeSmall, 0, SpawnLocationCoast},
        {{RaceSeaCrab}, SpawnGroupSizeSmall, 0, SpawnLocationCoast},
        {{RaceSapphireFish}, SpawnGroupSizeSmall, 0, SpawnLocationRiver | SpawnLocationUnderwater},
        {{RaceLemonFish}, SpawnGroupSizeSmall, 0, SpawnLocationRiver | SpawnLocationUnderwater},
        {{RaceSeahorse}, SpawnGroupSizeSmall, 0, SpawnLocationUnderwater},
        {{RaceMermaid, RaceMerman}, SpawnGroupSizeSmall, 0, SpawnLocationCoast | SpawnLocationUnderwater},
        {{RaceShark}, SpawnGroupSizeSingle, 0, SpawnLocationUnderwater},
        {{RaceBumblebee}, SpawnGroupSizeSingle, SpawnTerrainPlain, SpawnLocationPlain},
        {{RaceLanternFish}, SpawnGroupSizeSingle, 0, SpawnLocationUnderwater},
        {{RaceMawFish}, SpawnGroupSizeSingle, 0, SpawnLocationUnderwater},
        {{RacePiranha}, SpawnGroupSizeSmall, SpawnTerrainJungle, SpawnLocationRiver},
        {{RaceBlowfish}, SpawnGroupSizeSingle, 0, SpawnLocationUnderwater},
        {{RaceBeaver}, SpawnGroupSizeSmall, SpawnTerrainPlain | SpawnTerrainWoodlands, SpawnLocationRiverEdge},
        {{RaceMoose}, SpawnGroupSizeSingle, SpawnTerrainWoodlands, SpawnLocationPlain},
        {{RaceZebra}, SpawnGroupSizeSmall, SpawnTerrain_64, SpawnLocationPlain},
        {{RaceElephant, RaceBabyElephant},
         SpawnGroupSizeSmall,
         SpawnTerrainJungle | SpawnTerrain_64,
         SpawnLocationPlain},
        {{RaceIceling}, SpawnGroupSizeSmall, SpawnTerrainSnow, SpawnLocationPlain},
        {{RaceBabyMammoth}, SpawnGroupSizeSmall, SpawnTerrainSnow, SpawnLocationPlain},
        {{RaceRhino}, SpawnGroupSizeSingle, SpawnTerrain_64, SpawnLocationPlain},
        {{RaceOstrich}, SpawnGroupSizeSmall, SpawnTerrain_64, SpawnLocationPlain},
        {{RaceLionMale, RaceLionFemale}, SpawnGroupSizeSmall, SpawnTerrainJungle | SpawnTerrain_64, SpawnLocationPlain},
        {{RaceSkeletonKnight}, SpawnGroupSizeSmall, SpawnTerrainUndead, SpawnLocationPlain},
        {{RaceSkeletonHorse}, SpawnGroupSizeSmall, SpawnTerrainUndead, SpawnLocationPlain},
        {{RaceHyena}, SpawnGroupSizeSmall, SpawnTerrain_64, SpawnLocationPlain},
        {{RaceGiraffe}, SpawnGroupSizeSmall, SpawnTerrain_64, SpawnLocationPlain},
        {{RaceWarthog}, SpawnGroupSizeSmall, SpawnTerrainJungle | SpawnTerrain_64, SpawnLocationPlain},
        {{RaceAntelope}, SpawnGroupSizeSmall, SpawnTerrain_64, SpawnLocationStreetBuilding},
        {{RaceMakake}, SpawnGroupSizeSmall, SpawnTerrainJungle | SpawnTerrain_64, SpawnLocationStreetBuilding},
        {{RaceSkeletonDog}, SpawnGroupSizeSmall, SpawnTerrainUndead, SpawnLocationPlain},
        {{RaceLavaTurtle}, SpawnGroupSizeSingle, 0, SpawnLocationLavaRiverEdge | SpawnLocationLavaRiver},
        {{RaceFlamingo}, SpawnGroupSizeSmall, SpawnTerrainJungle | SpawnTerrain_64, SpawnLocationRiverEdge},
        {{RaceSnowGnobold}, SpawnGroupSizeSmallLeader, SpawnTerrainSnow, SpawnLocationSpecial | SpawnLocationPlain},
        {{RaceSavageDog, RaceAlphaDog},
         SpawnGroupSizeSmall,
         SpawnTerrainPlain | SpawnTerrainWoodlands,
         SpawnLocationPlain},
        {{RaceSnowLionMale, RaceSnowLionFemale}, SpawnGroupSizeSmall, SpawnTerrainSnow, SpawnLocationPlain},
        {{RaceSnowLeopard}, SpawnGroupSizeSmall, SpawnTerrainSnow, SpawnLocationPlain},
        {{RaceFlyingEye}, SpawnGroupSizeSmall, SpawnTerrainLava, SpawnLocationPlain},
        {{RaceDemonMarauder}, SpawnGroupSizeSmall, SpawnTerrainLava, SpawnLocationPlain},
        {{RaceDarkCultistMale, RaceDarkCultistFemale},
         SpawnGroupSizeSmallLeader,
         SpawnTerrainLava,
         SpawnLocationSpecial | SpawnLocationPlain},
        {{RaceCrawler}, SpawnGroupSizeLarge, SpawnTerrainLava, SpawnLocationPlain},
        {{RaceMonstrosity}, SpawnGroupSizeSingle, SpawnTerrainUndead, SpawnLocationPlain},
        {{RaceSkullSlater}, SpawnGroupSizeSingle, SpawnTerrainUndead, SpawnLocationPlain},
        {{RaceManaDeer}, SpawnGroupSizeSingle, SpawnTerrainWoodlands, SpawnLocationPlain},
        {{RaceWraith}, SpawnGroupSizeSingle, SpawnTerrainUndead, SpawnLocationPlain},
        {{RaceChiling},
         SpawnGroupSizeSmall,
         SpawnTerrainPlain | SpawnTerrainDesert | SpawnTerrain_64,
         SpawnLocationPlain},
        {{RaceHabanero}, SpawnGroupSizeSmall, SpawnTerrainDesert | SpawnTerrain_64, SpawnLocationPlain},
        {{RaceBloomling},
         SpawnGroupSizeSmall,
         SpawnTerrainPlain | SpawnTerrainMountain | SpawnTerrainWoodlands,
         SpawnLocationPlain},
        {{RaceCaterpillar},
         SpawnGroupSizeSmall,
         SpawnTerrainPlain | SpawnTerrainMountain | SpawnTerrainWoodlands,
         SpawnLocationPlain},
        {{RaceEarthCaterpillar},
         SpawnGroupSizeSmall,
         SpawnTerrainPlain | SpawnTerrainMountain | SpawnTerrainWoodlands,
         SpawnLocationStreetBuilding},
        {{RaceSnail},
         SpawnGroupSizeSmall,
         SpawnTerrainPlain | SpawnTerrainMountain | SpawnTerrainWoodlands,
         SpawnLocationStreetBuilding},
    };
}

bool SpawnTable::chooseGroup(int x, int y, bool special, Group &group)
{
    // The centre of the square, in the world generator's units.
    int pointX = x * 16 + 8;
    int pointY = y * 16 + 8;
    plasma::Vector<2, int> region = m_world->findRegion(pointX, pointY);
    int biome = m_world->landBiome(region[0], region[1]);

    int terrain = 0;
    World *world = m_world;
    int locationFlags = 0;
    {
        plasma::Vector<2, int> spawnPoint = world->streetPoint(x, y);
        World::AreaInfo spawnArea;
        world->fillAreaInfo(spawnPoint[0], spawnPoint[1], spawnArea);
        int part = world->streetBuildingType(x, y, spawnPoint, spawnArea);
        if (part >= 1 && part <= 4) {
            locationFlags = SpawnLocationStreetBuilding;
        }
    }

    bool plain = true;
    World::BiomeSample sample;
    m_world->biomeSample(pointX, pointY, sample);
    World::AreaInfo area;
    m_world->fillAreaInfo(pointX, pointY, area);
    float undead = sample.undead(area.type, area.influence);
    float temperature = sample.temperature(area.type, area.influence);
    float humidity = sample.humidity();
    float mountains = sample.mountains();
    float firelands = sample.lavaWeight(area.type, area.influence);
    float enchantedForest = sample.enclave(area.type, area.influence);
    float road = m_world->streetAmount(pointX, pointY, area.type, area.influence);
    float river = m_world->valleyWater(pointX, pointY, road, firelands, undead, sample.wetness(), mountains, &area);
    float lavaRiver = m_world->lavaChannels(pointX, pointY, road, firelands, mountains, &area);
    float poisonRiver = m_world->poisonChannels(pointX, pointY, road, undead, mountains, &area);
    float elevation = sample.biomeElevation(area.type, area.influence);
    float cliffs =
        m_world->mountainCliffs(pointX, pointY, elevation, mountains, river, lavaRiver, poisonRiver, road, &area);
    float sand = m_world->sandAmount(pointX, pointY, humidity, cliffs, sample.desertness());
    float height = m_world->height(pointX, pointY, elevation, cliffs, river, lavaRiver, poisonRiver, sand, road,
                                   mountains, &area, 0);

    if (undead > 0.5f) {
        plain = false;
        terrain = SpawnTerrainUndead;
    }
    if (temperature < 0.2f) {
        terrain |= SpawnTerrainSnow;
        plain = false;
    }
    if (humidity < 0.1f && temperature > 0.9f) {
        terrain |= SpawnTerrainDesert;
        plain = false;
    }
    if (humidity > 0.8f && temperature > 0.9f) {
        terrain |= SpawnTerrainJungle;
        plain = false;
    }
    if (humidity < 0.4f && temperature > 0.8f) {
        terrain |= SpawnTerrainDesert;
    }
    if (mountains > 0.5f) {
        terrain |= SpawnTerrainMountain;
        plain = false;
    }
    if (firelands > 0.5f) {
        plain = false;
        terrain = SpawnTerrainLava;
    }
    if (biome == BiomeWoodlands) {
        terrain |= SpawnTerrainWoodlands;
        plain = false;
    }
    if (enchantedForest > 0.5f) {
        terrain |= SpawnTerrainEnclave;
        plain = false;
    }

    if (height < 10.0f && height > 0.0f) {
        locationFlags |= SpawnLocationCoast;
    }
    if (height <= 0.0f) {
        locationFlags |= SpawnLocationUnderwater;
    }
    if (river > 0.25f && river < 0.5f) {
        locationFlags |= SpawnLocationRiverEdge;
    }
    if (river > 0.5f) {
        locationFlags |= SpawnLocationRiver;
    }
    if (lavaRiver > 0.25f && lavaRiver < 0.5f) {
        locationFlags |= SpawnLocationLavaRiverEdge;
    }
    if (lavaRiver > 0.5f) {
        locationFlags |= SpawnLocationLavaRiver;
    }
    if (poisonRiver > 0.25f && poisonRiver < 0.5f) {
        locationFlags |= SpawnLocationPoisonRiverEdge;
    }
    if (poisonRiver > 0.5f) {
        locationFlags |= SpawnLocationPoisonRiver;
    }
    if (cliffs > 0.75f) {
        locationFlags |= SpawnLocationCliffs;
    }
    // Terrain without any special biome is terrain 1.
    int terrainFlags = terrain;
    if (plain) {
        terrainFlags |= SpawnTerrainPlain;
    }
    if (special) {
        locationFlags |= SpawnLocationSpecial;
    } else if (locationFlags == 0) {
        locationFlags = SpawnLocationPlain;
    }

    std::vector<Group *> candidates;
    for (Group &candidate : m_groups) {
        if ((candidate.terrainFlags == 0 || (candidate.terrainFlags & terrainFlags) != 0) &&
            ((!special && candidate.locationFlags == 0) || (candidate.locationFlags & locationFlags) != 0) &&
            !candidate.creatureTypes.empty()) {
            candidates.push_back(&candidate);
        }
    }
    if (candidates.empty()) {
        return false;
    }
    group = *candidates[rand() % candidates.size()];
    return true;
}

}
