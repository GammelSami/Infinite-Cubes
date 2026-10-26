// Zone owns terrain in 64 x 64 columns (m_fields, indexed by 64 * local x + local y), a voxel mesh and generated
// items, plants and creatures. World keeps and deletes loaded zones. Some auxiliary record types remain opaque.
#pragma once
#include "cube/ZoneIds.h"
#include "cube/WorldDimensions.h"
#include <string.h>
#include <array>
#include <list>
#include <map>
#include <set>
#include <vector>
#include "plasma/Vector.h"
#include "cube/Block.h"
#include "cube/Creature.h"
#include "cube/Field.h"
#include "cube/Item.h"
#include "gfx/Chunk.h"

namespace cube {

class Missions;
class World;

struct StaticEntity {
    StaticEntity()
        : scale(1.0f),
          active(1),
          transitionTime(0.0f),
          occupantId(0),
          itemCount(1)
    {}

    int type;
    plasma::Vector<3, Fixed> position; // world (dot) position
    unsigned char orientation;
    float scale;          // Initially 1.0.
    unsigned char active; // Initially 1.
    float transitionTime; // Floating-point value, initially zero.
    long long occupantId; // Initially 0.
    Item item;
    unsigned char itemCount; // Initially 1.

    bool operator==(const StaticEntity &other) const;
    // Zone::randomPosition adds its height to the position of a type 117 entity.
    plasma::Vector<3, float> scaledSize() const;
};

struct ZoneItem {
    ZoneItem()
        : destination(-1, -1),
          flags(0),
          dropTime(0),
          hiddenTimeRemaining(0),
          dropDay(-1),
          quantity(1),
          ownerSeed(0)
    {}

    Item item;
    plasma::Vector<3, Fixed> position; // Three stored values.
    plasma::Vector<2, int> destination;
    float
        rotation; // not set by the constructor; an angle in degrees (World::dropItem stores its float argument, then rand() * 360 / 32767)
    unsigned char flags; // Initially 0.
    float dropTime;      // Initially zero; World::dropItem sets 500..799 for a dropped item.
    // This timer's further uses are unknown.
    int hiddenTimeRemaining; // Initially 0.
    int dropDay;             // Initially -1.
    int quantity;            // Initially 1.
    long long ownerSeed;     // Initially 0.
};

struct CubeChange {
    CubeChange()
        : day(0)
    {}

    plasma::Vector<3, int> position;
    Block originalBlock;
    Block newBlock;
    int day;
};

class Zone {
public:
    // The key of the position maps: a 16-block cell (x, y) and a position kind.
    struct PositionKey {
        bool operator<(const PositionKey &other) const { return elements < other.elements; }

        std::array<int, 3> elements;
    };

    struct Plant {
        Plant()
            : rotation(0.0f),
              color(1.0f, 1.0f, 1.0f),
              flags(PlantFlagNeedsSupport)
        {}

        int type;
        plasma::Vector<3, Fixed> position; // world (dot) position (addPlant tests x and y)
        float scale;                       // generate stores a height in blocks
        float rotation;                    // 1.0f for the type 89 plants
        float
            daylight; // Not written by generate. Renderer::collectLights scales it by daylight / 255 to dim lamp types 23..25.
        plasma::Vector<3, float> color;
        int flags; // PlantFlag
    };
    struct UnusedRecord {
        UnusedRecord()
            : unused00(0),
              unused08(0),
              unused10(0),
              unused18(0),
              unused20(0)
        {}

        long long unused00;
        long long unused08;
        long long unused10;
        long long unused18;
        int unused20;
    };
    // Description of a creature to spawn.
    struct Resident {
        Resident(); // (the mission file)
        ~Resident();
        // False, and nothing changed, for a type without a name variant. Defined in the mission file.
        bool setNamesake(int x, int y, int type);

        plasma::Vector<3, Fixed> position; // World position in fixed-point units.
        float angle;                       // degrees; 270.0f, rand() * 360 / 32767
        int hostility;                     // 3 (villagers), 4 (pets), 7 (wild creatures); equip's hostilityType
        int race;                          // randomRace(260); fillAppearance's race
        int level;                         // World copies to creature level
        int factionKind;                   // 3; a villager's Realm::form() or 0; equip's factionKind
        int factionVariant;                // copied to EntityData
        unsigned int realm;
        unsigned char classType;                     // Class passed to equip.
        unsigned char specialization;                // equip's specialization
        long long id;                                // World constructs Creature with this id
        long long ownerId;                           // copied to EntityData
        int unused48;                                // Initially zero; never read.
        unsigned char interactionState;              // copied to EntityData
        Creature::EntityData::Appearance appearance; // fillAppearance's appearance; appearanceFlags is equip's flags
        char name[16];
        Equipment equipment; // equip's equipment
        // Resident's equipment and inventory containers.
        Inventory inventory; // equip's inventory
        int behaviours;
        int currentAbility;             // Ability copied to the creature and compared with AbilitySlowFall.
        std::vector<int> specialSkills; // equip's specialSkills
        int summonAppearanceMode;       // equip's summonAppearanceMode
        std::vector<int> summonRaces;   // equip's summonRaces
        plasma::Vector<2, int> destination;
        std::set<std::array<int, 2>> activatorCells;
        std::set<long long> groupMembers;
        int requiredDefeatedCount;
        std::set<std::array<int, 2>> prerequisiteEntityCells;
        std::list<long long> prerequisiteCreatureIds;
        int requiredInteractionState;
    };

public:
    Zone(World *world, const plasma::Vector<2, int> &position);
    virtual ~Zone();

    // Returns the block at world block coordinates in this zone, or the default block outside it. Below the stored
    // column returns solid, above it returns empty.
    Block getBlock(int x, int y, int z);
    // Appends the plant to m_plants when its position lies in the zone; false otherwise.
    bool addPlant(const Plant &plant)
    {
        int x = m_position[0] * BlocksPerZone;
        int y = m_position[1] * BlocksPerZone;
        if (plant.position[0] < x || plant.position[0] >= x + BlocksPerZone || plant.position[1] < y ||
            plant.position[1] >= y + BlocksPerZone) {
            return false;
        }
        m_plants.push_back(plant);
        return true;
    }
    // Zone::generate calls it 52 times.
    bool addStaticEntity(const StaticEntity &entity)
    {
        int x = m_position[0] * BlocksPerZone;
        int y = m_position[1] * BlocksPerZone;
        if (entity.position[0] < x || entity.position[0] >= x + BlocksPerZone || entity.position[1] < y ||
            entity.position[1] >= y + BlocksPerZone) {
            return false;
        }
        m_staticEntities.push_back(entity);
        return true;
    }
    // Defined in the mission file; generate calls it for every resident it creates.
    void adoptResident(Resident *resident);
    // Puts a static entity on the ground: false when its position lies outside the zone; otherwise its height goes
    // down while the layer of blocks under its box (scaledSize, turned by orientation's bit 0) is empty or water and
    // up while something stands in it, twenty blocks at most each way, and ends on a whole block. False as well when
    // needsSupport is set and a column under the box is empty or water below it, when the entity stands in water, or
    // on a wet, lava or poison block. Called by Zone::generate (32 times), addRoomEntities and the mission file.
    bool putOnGround(StaticEntity &entity, bool needsSupport);
    // Zone-generation helpers.
    void addCastleResidents(int cellX, int cellY, Missions &missions, int castlePart,
                            const plasma::Vector<2, int> &castleCell);
    void addResidents(int cellX, int cellY, Missions &missions);
    // Places the entities of a storey's room (putOnGround 14 times, addStaticEntity, cubeAt; buildingKind,
    // buildingStoreys, hasWalls and isOccupiedStorey of the neighbours), given the floor's height, the house type and
    // storey, the place's type (a cellar: the dungeon type) and its hamlet plot (a cellar: 0).
    void addRoomEntities(int cellX, int cellY, int floorZ, int type, int storey, int placeType, int plot);
    // A group of wild creatures in a 16-block cell: SpawnTable::chooseGroup for the cell (special sets its
    // SpawnLocationSpecial), randomFlag of the group's size flags for how many (1: one; 2, 4, 8: three to five; 16:
    // six to thirteen; never more than the cell has positions of kind 1, or 5 when special), each a resident of a
    // random type of the group at a random position of that kind, with a rider for race 270, and for size 8 a camp of
    // a fire and up to five more static entities.
    void addSpawnGroup(int cellX, int cellY, bool special);
    // The position of the list of (cellX, cellY, kind) in m_positionsByKind that is nearest to the centre of that
    // 16-block cell, or randomPosition's result when there is none.
    plasma::Vector<3, Fixed> choosePosition(int cellX, int cellY, int kind);
    // The block at world (dot) coordinates, 65536 per block, rounding down.
    Block cubeAt(Fixed x, Fixed y, Fixed z);
    // The column at world block coordinates (x, y), or 0 outside the zone.
    Field *field(int x, int y)
    {
        if (x < m_position[0] * BlocksPerZone || y < m_position[1] * BlocksPerZone ||
            x >= m_position[0] * BlocksPerZone + BlocksPerZone || y >= m_position[1] * BlocksPerZone + BlocksPerZone) {
            return nullptr;
        }
        return &m_fields[(long long)(x - m_position[0] * BlocksPerZone) * BlocksPerZone +
                         (y - m_position[1] * BlocksPerZone)];
    }
    // Whether the column at world block coordinates (x, y) lies in the zone.
    bool contains(int x, int y) const
    {
        int minX = m_position[0] * BlocksPerZone;
        int minY = m_position[1] * BlocksPerZone;
        return x >= minX && y >= minY && x < minX + BlocksPerZone && y < minY + BlocksPerZone;
    }
    // Generates the zone: the terrain of its BlocksPerZone x BlocksPerZone columns, the caves, the structures of the
    // 16-block cells around it, its plants and its creatures.
    void generate();
    void generatePlants();
    // The zone is not read.
    bool isTerrain(const Block &block);
    // Whether the 5 x 5 columns around (x, y) are free above z: the three blocks from z + d / step, and those up to z
    // + 7, are empty or water, where d is the larger axis distance from (x, y).
    bool isSpaceFree(int x, int y, int z, int step);
    // How many positions m_positionsByKind holds for (cellX, cellY, kind).
    int positionCount(int cellX, int cellY, int kind)
    {
        std::map<PositionKey, std::vector<plasma::Vector<3, Fixed>>>::iterator it =
            m_positionsByKind.find(PositionKey {{cellX, cellY, kind}});
        if (it == m_positionsByKind.end())
            return 0;
        return (int)it->second.size();
    }
    // Takes a random position out of the list of (cellX, cellY, kind) in m_remainingPositions, refilling an empty list
    // from m_positionsByKind first; for kind 3 the top of the zone's type 117 static entity, if there is one; without
    // any, the top of the zone's centre column.
    plasma::Vector<3, Fixed> randomPosition(int cellX, int cellY, int kind);
    // randomPosition for the first of the kinds 2, 7, 16 and 1 that has positions in the cell, else for kind 0.
    plasma::Vector<3, Fixed> randomSpawnPosition(int cellX, int cellY);
    // An empty block at or below height 0 without BlockFlagProtected becomes water.
    void setBlock(int x, int y, int z, const Block &block);
    // Rebuilds two lists from the zone's contents: m_manaCubes gets the static entities of type 0, 16, 63..66, 83,
    // 112..114 and 126, and m_treasurePositions the positions of the items of category 30 and of the type 22 static
    // entities whose block does not have BlockFlagDungeon. Called by Game::update, generate and update.
    void update();
    void updateLists();
    // Marks the blocks the static entities stand in: clears BlockFlagBarrier on every block position of
    // m_occupiedBlockPositions, then sets it (with BlockFlagSpiritPassable for type 15 and 4 for types 3..5) on every
    // block inside each solid entity's box and records the position.
    void updateOccupancy();

    World *m_world;
    plasma::Vector<2, int> m_position; // Zone coordinates set by the constructor.

    std::vector<Plant> m_plants;
    std::vector<ZoneItem> m_items;        // the items lying in the zone
    std::vector<ZoneItem> m_initialItems; // a copy of m_items taken at the end of generate
    std::vector<StaticEntity> m_staticEntities;
    std::vector<StaticEntity> m_initialStaticEntities; // a copy of m_staticEntities taken at the end of generate
    std::vector<Plant> m_lightPlants;
    std::vector<Plant> m_soundPlants;
    std::vector<StaticEntity> m_lightEntities;
    std::list<CubeChange> m_cubeChanges;
    std::set<IntTriple> m_occupiedBlockPositions;
    bool m_awaitingData;          // true after construction
    bool m_itemsChanged;          // false after construction
    bool m_staticEntitiesChanged; // false after construction
    std::map<PositionKey, std::vector<plasma::Vector<3, Fixed>>> m_positionsByKind;
    std::map<PositionKey, std::vector<plasma::Vector<3, Fixed>>> m_remainingPositions;
    // Four separate state members.
    UnusedRecord m_unusedRecord120;
    UnusedRecord m_unusedRecord148;
    UnusedRecord m_unusedRecord170;
    UnusedRecord m_unusedRecord198;
    gfx::Chunk m_chunk;
    bool m_unused448; // false after construction; no other access
    std::map<int, Item> m_objectRewards;
    std::map<long long, Resident *> m_residents;
    std::set<std::array<int, 2>> m_missionGroupZones; // Zone state container.
    std::list<long long> m_trackedMissionCreatureIds; // Zone state container.
    int m_defeatedMissionCreatureCount;               // 0 after construction
    std::list<long long> m_missionCreatures;          // Zone state container.
    std::list<int> m_prerequisiteEntityIndices;       // Zone state container.
    std::list<plasma::Vector<3, Fixed>> m_treasurePositions;
    // Zone::generate prints it with operator<<(int).
    std::map<std::array<int, 2>, unsigned int> m_requiredActivators;
    std::map<std::array<int, 2>, unsigned int> m_completedActivators;
    std::list<Resident *> m_dailyResidents;
    int m_lastUpdateDay;                                       // -1 after construction
    std::vector<plasma::Vector<3, Fixed>> m_residentPositions; // Position records used like m_positionsByKind.
    Field m_fields[BlocksPerZone * BlocksPerZone];
};

// A value in 1..100000 from two integers: libnoise's integer value noise (n = x + 57 y; n = (n << 13) ^ n; n (n n
// 60493 + 19990303) + 1376312589, 31 bits) scaled down. Called by Game, the zone building code and Zone::generate.
unsigned int fastRandom(int x, int y);
// The largest integer not above the value.
inline int floor(const float &value)
{
    if (value < 0.0f && value != (float)(int)value) {
        return (int)value - 1;
    }
    return (int)value;
}

inline int floor(const double &value)
{
    if (value < 0.0 && value != (double)(int)value) {
        return (int)value - 1;
    }
    return (int)value;
}
// True for half of the positions (fastRandom of the position, offset by 66945 or 1264824 in x, is even).
bool halfChanceA(int x, int y);
bool halfChanceB(int x, int y);
// isSolid: whether the block is neither empty nor water. isFree: whether a block lets a structure be placed through
// it.
bool isSolid(const Block &block);
bool isFree(const Block &block, bool allowSpiritPassage, bool checkBarriers, bool blockPassableBarriers);
bool isNatural(const unsigned char &category);
// Whether the square of the radius around (x, y) lies outside the zone at zone coordinates (zoneX, zoneY).
bool isOutsideZone(int x, int y, int radius, int zoneX, int zoneY);
// Tests whether (x, y) is the village remembered by the region's missions, pointed to by a kind 19 place.
// Zone::generate marks its residents AppearanceFlagPetrified, preventing damage in either direction.
bool isProtectedVillage(const Missions &missions, int x, int y);
// Whether a zone building type is a house: kind 1 (but not type 26), 2, 7 or 8, or type 25.
bool isHouse(int type);
// Whether a field (isField: the types 27..34) may fence the side towards a neighbouring cell of the given type: not a
// field and without residents (residentCount); type is not read.
bool hasFence(int type, int neighbour);
bool isField(int type);
class Realm;
void hasResidentRace(int dungeonType, Realm &realm, Zone::Resident &resident);
// Whether the building type has storeys (buildingStoreyHeight > 0); Zone::generate uses it when checking a house's
// neighbours.
bool hasStoreys(int type);
// Whether the type is 63..65, whose plots receive corner posts instead of a foundation.
bool hasPosts(int type);
// Whether the house may place its door on any open side: types 24, 36, 37, 46, 71, 76, 81, 89, 92 and 100. Other types
// choose a random side.
bool hasStreetDoor(int type);
// Shared zone queries.
struct StructureCell;
int dungeonCellarDepth(const StructureCell &cell);
plasma::Vector<3, float> dungeonTrimColor(StructureCell cell, int dungeonType);
plasma::Vector<3, float> dungeonWallColor(StructureCell cell, int dungeonType);
// For a point (x, y) in world (dot) coordinates and the segment from block position a to block position b:
// lineFraction is how far along the segment the point's projection lies (0 at a, 1 at b), and lineSquaredDistance the
// squared distance in blocks from the point to the segment. Called by Zone::generate.
float lineFraction(const plasma::Vector<2, int> &a, const plasma::Vector<2, int> &b, const Fixed &x, const Fixed &y);
inline float lineSquaredDistance(const plasma::Vector<2, int> &a, const plasma::Vector<2, int> &b, const Fixed &x,
                                 const Fixed &y)
{
    plasma::Vector<2, float> direction(b - a);
    plasma::Vector<2, float> offset((float)(x - Fixed(a[0])), (float)(y - Fixed(a[1])));
    float along = direction.dot(offset);
    if (along <= 0.0f) {
        return offset.squaredLength();
    } else {
        float squaredLength = direction.squaredLength();
        if (along >= squaredLength) {
            plasma::Vector<2, float> fromEnd((float)(x - Fixed(b[0])), (float)(y - Fixed(b[1])));
            return fromEnd.squaredLength();
        } else {
            return (offset - direction * (along / squaredLength)).squaredLength();
        }
    }
}
// value modulo divisor, never negative for a positive divisor.
inline int modulo(int value, int divisor)
{
    return (value % divisor + divisor) % divisor;
}
// The flower type (0..2) of the patch a column lies in: low-frequency noise at the block position, below -0.5 type 0,
// above 0.5 type 1, else type 2, so that neighbouring columns grow the same flower.
int randomBloom(int x, int y);
// One of the SpawnGroupSizeFlag values that are set in flags, each as likely as the others (rand), or 0 when none is.
int randomFlag(int flags);

}
