// Missions generates a region's missions, rewards, residents and mission objects, and tracks the local player's
// progress through them.
#pragma once
#include <array>
#include <map>
#include <vector>

#include "cube/Item.h"
#include "cube/WorldIds.h"
#include "plasma/Vector.h"

namespace cube {

class World;
class Zone;

// Uses the vector's squaredLength to measure squared distance.
inline int squaredLength(const plasma::Vector<2, int> &vector)
{
    return vector.squaredLength();
}

bool isRemoteType(int type);
// The building types 0..5, 10..12 and 99: no building, the street's houses and the lone buildings, where a mission may
// put its own.
inline bool isOpenBuilding(int type)
{
    return type == BuildingNone || type == BuildingOak || type == BuildingEvergreen || type == BuildingBirch ||
           type == BuildingJungleTree || type == BuildingPalm || type == BuildingHill || type == BuildingRock ||
           type == BuildingTallRock || type == BuildingCampEquipmentSite;
}
Item fetchItem(int locationType, int x, int y, int rarity, const plasma::Vector<2, int> &region);
int statueRace(int x, int y);
int monsterRace(int x, int y, int locationType);

class Missions {
public:
    Missions(World *world, const plasma::Vector<2, int> &region);
    // The kind the constructor gave the place of a cell, 0 for none.
    int kindAt(int x, int y)
    {
        std::array<int, 2> key = {x, y};
        std::map<std::array<int, 2>, unsigned int>::iterator it = m_kinds.find(key);
        if (it == m_kinds.end()) {
            return 0;
        }
        return it->second;
    }
    // Returns the artifact hidden at a kind 14 place, or a shared empty item.
    const Item &artifactAt(int x, int y)
    {
        std::array<int, 2> key = {x, y};
        std::map<std::array<int, 2>, Item>::iterator it = m_artifacts.find(key);
        static Item none;
        if (it == m_artifacts.end()) {
            return none;
        }
        return it->second;
    }
    // What a place's mission gives: an item of category 24 with id 14..17 of this region for the kinds 2..5, the
    // artifact for kind 14, else an empty item.
    Item taskRewardAt(int x, int y);
    // Builds resident dialog commands for World::talkScript in the creature's home region.
    std::vector<std::string> talk(Creature *creature);
    // What the resident of a place has to say (0..32; 26, 1 and 27 for the kinds 18, 16 and 17, 28 at a dungeon, else
    // one of a list by the location type, chosen by the cell's hash and, while the place has no reward, the day).
    int determineTopic(int x, int y);
    // Hides an artifact (category 23, a random id 0..6 and modifier, the realm of the cell) at a place; the first
    // argument is not read.
    void generateArtifact(int unused, int x, int y, int &seed);
    // Whether the local player may craft equipment of a rarity here: always for rank 0, otherwise once it has solved
    // one of the region's places of kind rank + 1 (the kinds 2..5, whose rewards are the crafting books 14..17). False
    // without a local player.
    bool hasCraftingRank(int rank);
    plasma::Vector<2, int> artifactPlace(int seed, int realm);
    // How many of the region's places of kind 1 the local player has solved, at most 4 (0 without a local player).
    // World::mostLevelTier returns it.
    int solvedCount();
    // The special room a house of a place holds, one of m_specialRooms by a hash of the cell: chooseRoomAt (8 for one
    // house in four and with an empty list) for the shops of kind 5 of a place, specialRoomAt (-1 with an empty list)
    // for the inns and the types 76, 81 and 100. Zone::generate stores the result as the room kind of the storeys.
    int chooseRoomAt(int x, int y);
    int specialRoomAt(int x, int y);
    plasma::Vector<2, int> randomPlace(int seed);
    // The number that goes with a place's kind (m_values), 0 for a place without one: a mission-kind-dependent count,
    // rank or tier.
    int requestCount(int x, int y);
    // Tests the terrain at the centre of a 16-block cell, returning false near a place's centre (influence below 0.8).
    // isLavaRiver tests lavaChannels above one half; isWaterway tests valleyWater; isSuitableGround requires dry
    // ground above the water and outside lava and poison channels.
    bool isLavaRiver(int cellX, int cellY);
    bool isSuitableGround(int cellX, int cellY);
    bool isWaterway(int cellX, int cellY);
    bool isMissionBuilding(int locationType, int cellX, int cellY, int unused);
    bool isMissionSite(int locationType, int cellX, int cellY);
    // The test generate uses: the given building type if there is one, open buildings in a valley for PlaceLake and on
    // a hill for PlaceLavaLake and PlaceMountain, else isMissionBuilding.
    bool isPlaceBuilding(int locationType, int cellX, int cellY, int buildingType);
    // The topics 28 (a dungeon) and 29 (a castle) put nothing into a zone: the place's location type is looked up and
    // dropped.
    void generateCastle(Zone *zone);
    void generateDungeon(Zone *zone);
    // A place overrun by creatures that come out of objects (the topics 2..8, "the_location_is_full_of_creatures" and
    // "you_must_destroy_the_objects"): the mission's resident in one of the place's mission buildings, 1 to 12 objects
    // to destroy (hostility 7, recorded in the zone's m_trackedMissionCreatureIds) on open plots, and creatures of the
    // given races (hostility 1, with the form, appearance flags and class given) beside each object and on one more
    // plot in twelve.
    void generateObjects(Zone *zone, int race, const std::vector<int> &races, int factionKind, unsigned int flags,
                         int classType);
    // generateObjects for each kind of object: the object's creature type and the creatures around it.
    void generateBonePiles(Zone *zone);    // topic 5
    void generateDemonPortals(Zone *zone); // topic 3
    void generateHornetsNests(Zone *zone); // topic 8
    void generateMolehills(Zone *zone);    // topic 7
    void generateSlimeWells(Zone *zone);   // topic 6
    void generateStatues(Zone *zone);      // topic 2
    void generateUnholyTombs(Zone *zone);  // topic 4
    void generatePlace(Zone *zone);
    void generateAborigines(Zone *zone);
    void generateBand(Zone *zone);
    // The creature gets factionKind (Resident::factionKind), the class and the appearance flags given, and
    // AppearanceFlagTinted.
    void generateBoss(Zone *zone, int race, int factionKind, int classType, unsigned int flags);
    // The captive is of race with the class and captiveFactionKind given; the keepers are of keeperRace with
    // keeperFactionKind.
    void generateCamp(Zone *zone, int race, int classType, int captiveFactionKind, int keeperRace,
                      int keeperFactionKind);
    // The target (a creature of race for a cult, the object of objectRace for a dark site) and three to six guards of
    // guardRace with the form, which belong to it.
    void generateCult(Zone *zone, int race, int guardRace, int factionKind);
    void generateDarkSite(Zone *zone, int objectRace, int guardRace, int guardFactionKind);
    void generateErrand(Zone *zone);
    void generateExchange(Zone *zone);
    // The chief is of race with flags and the form; the creatures on the plots are of the races with the form, the
    // class and creatureFlags.
    void generateInvasion(Zone *zone, int race, unsigned int flags, const std::vector<int> &races, int factionKind,
                          int classType, unsigned int creatureFlags);
    void generateMonster(Zone *zone);
    void generate(Zone *zone);

    // Updates the region's mission state for the world.
    void updateTasks(World *world);

    World *m_world;
    plasma::Vector<2, int> m_region;
    std::map<std::array<int, 2>, unsigned int> m_kinds;   // by cell: what the place is in the region's missions (1..23)
    std::map<std::array<int, 2>, Item> m_artifacts;       // by cell (generateArtifact)
    std::vector<plasma::Vector<2, int>> m_places;         // the cells whose kind is not one of 13..15, 17, 21..23
    std::vector<plasma::Vector<2, int>> m_artifactPlaces; // the cells of kind 14
    std::map<std::array<int, 2>, unsigned int>
        m_values; // by cell: a number that goes with the kind (a tier 0..4, a count)
    std::map<std::array<int, 2>, std::array<int, 2>>
        m_partners;                   // pairs a place whose topic is 24 with the nearest free place, both ways
    plasma::Vector<2, int> m_village; // One of the region's type 5 places, when there is no kind 19 place.
    bool m_hasDemonPortal;            // Whether the region has a kind 20 place.
    int m_flags; // MissionFlagBell, MissionFlagHarp, MissionFlagWhistle, MissionFlagMagicBarriers for PlaceCave, PlaceDoomTower, PlaceManaPump, PlaceRock
    std::vector<int> m_specialRooms; // 15, 16 and 14 for the kinds 12, 10 and 21
};

}
