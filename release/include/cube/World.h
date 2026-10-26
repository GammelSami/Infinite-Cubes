#include <algorithm>
// World owns loaded zones and creatures, the day and time, the save database, seeded world-generation tables and
// update queues. It provides procedural terrain, building and mission queries and advances the simulation.
#pragma once
#include <windows.h>
#include <math.h>
#include <algorithm>
#include <list>
#include <array>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "plasma/Vector.h"
#include "cube/Noise.h"
#include "cube/Block.h"
#include "cube/BlockIds.h"
#include "cube/Database.h"
#include "cube/Item.h"
#include "cube/ServerUpdates.h"
#include "cube/SpawnTable.h"
#include "cube/StructureCell.h"
#include "cube/Zone.h"
#include "cube/Creature.h"

namespace cube {

class Creature;
class Inventory;
class SpriteManager;
class Zone;
struct ZoneItem;

// Default blocks used for empty space, water and solid terrain.
static const Block s_airCube(plasma::Vector<3, unsigned char>(255, 255, 255), BlockAir);     // white, empty
static const Block s_waterCube(plasma::Vector<3, unsigned char>(255, 255, 255), BlockWater); // white, water
static const Block s_solidCube(plasma::Vector<3, unsigned char>(200, 200, 200), BlockSolid); // grey, solid

// BiomeIdentifier in WorldIds.h names the 8192-block region biomes.

// Hashes two integers into 0..100000 using libnoise's IntValueNoise constants. Generator choices combine the position
// with a salt identifying each decision; a salt is included in x when only x + 57 * y + constant is needed.
static inline unsigned int random(int x, int y)
{
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return 100000 - ((n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff) / 21474;
}

class World {
public:
    struct ZoneState {
        // 1: items holds the zone's items (Zone::m_itemsChanged was set), 2: staticEntities holds its static entities
        // (Zone::m_staticEntitiesChanged).
        unsigned char flags;
        std::vector<ZoneItem> items;                                    // Zone::m_items
        std::vector<StaticEntity> staticEntities;                       // Zone::m_staticEntities
        std::list<CubeChange> cubeChanges;                              // Zone::m_cubeChanges
        std::map<std::array<int, 2>, unsigned int> completedActivators; // Zone::m_completedActivators
        int defeatedMissionCreatureCount;                               // Zone::m_defeatedMissionCreatureCount
    };

    class WorldState {
    public:
        int m_day; // 1 on a new world
        float m_time;
        std::map<std::array<int, 2>, ZoneState> m_zoneStates;
        std::set<long long> m_deadCreatures;
        std::set<long long> m_droppedRewards;
    };

    // Information about the special area or structure site of the cell containing a point, filled by fillAreaInfo.
    struct AreaInfo {
        int type;   // Area type; zero outside the area.
        bool large; // Whether the place is large; selects radius 500 rather than 250.
        int environment;
        int radius; // 250 or 500
        plasma::Vector<2, int> center;
        float squaredDistance; // from the point to center
        float influence;       // (1 - squaredDistance / radius^2)^2 inside the radius, else 0
    };

    // Biome weights in 0..1: full weight near a region's centre, falling beyond the nearest region. areaType and
    // areaInfluence describe a place's area and raise the corresponding weights.
    struct BiomeSample {
        int biomes[3][3]; // landBiome of each region
        float squaredDistances[3][3];
        int x; // the sampled point
        int y;
        float nearestSquaredDistance; // -1 before the first region
        int nearestBiome;

        // 1 where the squared distance to a region's centre equals the nearest one's, falling to 0 as the difference
        // grows to 1 / slope^2 (0 for the -1 that stands for "no such region").
        float weight(float squaredDistance, float slope) const
        {
            if (squaredDistance >= 0.0f && squaredDistance >= nearestSquaredDistance) {
                return std::max(1.0f - (squaredDistance - nearestSquaredDistance) * slope * slope, 0.0f);
            }
            return 0.0f;
        }

        float undead(int areaType, float areaInfluence);
        float temperature(int areaType, float areaInfluence); // (types 4, 9, 10, 13 against 6, 11)
        float humidity();
        float mountains();
        float lavaWeight(int areaType, float areaInfluence);
        // areaInfluence when areaType is 27, else 0 (does not read the sample).
        float enclave(int areaType, float areaInfluence);
        float wetness();
        float biomeElevation(int areaType, float areaInfluence);
        float desertness();
        // The forest biomes 3..6 (Woodlands, Jungle, Darkwoods, SnowForest).
        float forest();
        // The snow biomes 11 and 6 (Snowlands, SnowForest).
        float snow();
    };

    class AIManager {
    public:
        struct PathNode {
            PathNode(); // Value-initialised terrain sample.
            int movementCost;
            int heuristic;
            int totalCost;
            plasma::Vector<3, int> parent;
        };
        typedef std::map<IntTriple, PathNode> PathMap;

        // Scalars whose roles are unknown retain their numeric representation.
        struct State {
            State();
            ~State();
            void clearPath();
            bool isOpen(int x, int y, int z);
            PathNode *pathNode(int x, int y, int z);
            void setPathNode(int x, int y, int z, const PathNode &node);

            int idleTime;
            int chaseTime; // Timeout value.
            int retargetDelay;
            bool playerInSight;            // Transition near the player.
            bool wasPassive;               // previous passive toggle
            unsigned int lastSightingTime; // Timestamp from timeGetTime.
            long long preferredTargetId;
            int specialSkillDelay; // constructor 10000
            plasma::Vector<3, float> acceleration;
            plasma::Vector<3, float> attackRotation;
            plasma::Vector<2, int> unusedCell; // constructor (-1,-1); never read
            bool probeAhead;                   // offset destination by facing direction
            bool moving;                       // movement squared > .001
            bool unsupportedStep;              // path step has no supporting block
            bool inSpiritWorld;                // controls breakable-bit tests
            int searchBudget;                  // constructor 50, update caps at 500
            float speed;                       // Floating-point state value.
            std::list<Interaction> interactions;
            unsigned int flags;         // EntityData::flags snapshot
            unsigned int binaryToggles; // Entity flag snapshot.
            plasma::Vector<3, Fixed> position;
            plasma::Vector<3, float> size;
            PathMap::iterator best; // map node with smallest heuristic
            PathMap nodes;          // Path map keyed by three coordinates, with six-integer state values.
            std::set<IntTriple> frontier;
            plasma::Vector<3, int> startBlock;
            plasma::Vector<3, Fixed> start;
            plasma::Vector<3, Fixed> destination;
            float range; // distance threshold, update and canWalk
            long long targetId;
            int unused108; // zeroed by startPath; never read
            std::list<plasma::Vector<3, int>> path;
            std::set<StaticEntity *> objectTargets;       // object pointer insertion
            std::list<plasma::Vector<3, int>> unusedPath; // Unused path container; constructed and destroyed only.
            plasma::Vector<3, int> furnitureObjectKey;    // World::object's argument, update output
        };

        // AI state and pathfinding operations.
        bool canWalk(const plasma::Vector<3, float> &size, const plasma::Vector<3, Fixed> &from,
                     const plasma::Vector<3, Fixed> &to, float range, bool passSpiritPassableBarriers,
                     unsigned int toggles);
        bool inRange(const plasma::Vector<3, Fixed> &from, const plasma::Vector<3, Fixed> &to, float range);
        float rayLength(const plasma::Vector<3, Fixed> &from, const plasma::Vector<3, float> &direction, float length,
                        bool passSpiritPassableBarriers);
        bool checkSupport(Creature *creature, int milliseconds);
        bool combat(Creature *creature, int milliseconds);
        bool escort(Creature *creature, int milliseconds);
        bool handleTarget(Creature *creature, int milliseconds);
        bool idle(Creature *creature, int milliseconds);
        bool mirrorOwnerActions(Creature *creature, int milliseconds);
        bool watchOrGreetPlayer(Creature *creature, int milliseconds);
        bool targetSkill(Creature *creature, int milliseconds);
        void finishPathSearch(State &state);
        void goTo(State &state, Creature *target, float range, bool probeAhead, bool matchSpeed);
        void startPath(State &state);
        void updateCreature(Creature *creature, int milliseconds);
        bool walkPath(State &state);

        explicit AIManager(World *world);
        void update(int milliseconds);

        World *m_world;
        std::map<long long, State> m_states; // Creature ID key.
    };

    explicit World(SpriteManager *spriteManager);
    virtual ~World();

    // Returns the block at world block coordinates from its loaded zone, or the default block if the zone is not
    // loaded.
    bool isBlocked(const plasma::Vector<3, Fixed> &position, const plasma::Vector<3, float> &size, bool unusedFlag);
    Block getBlock(int x, int y, int z);
    // The block at world (dot) coordinates (65536 per block).
    Block getBlockAtDot(Fixed x, Fixed y, Fixed z);
    // How bright the block at block coordinates (x, y, z) is, 0..255: the first block at or above it that is not of
    // type 10, 12 or 19 gives its fourth byte, and an empty or type 2 block at least 5 and at least its colour byte
    // `channel`; 255 where the zone is not loaded.
    int cubeLight(int x, int y, int z, int channel);
    int cubeLight(Fixed x, Fixed y, Fixed z, int channel);
    // The loaded zone at zone coordinates (x, y), or 0. The map key is y in the high and x in the low 32 bits.
    Zone *zone(int x, int y)
    {
        std::unordered_map<unsigned long long, Zone *>::iterator it =
            m_zones.find(((unsigned long long)(unsigned)y << 32) | (unsigned)x);
        if (it == m_zones.end()) {
            return nullptr;
        }
        return it->second;
    }
    // The column at block coordinates (x, y) of a loaded zone, or 0.
    Field *field(int x, int y)
    {
        Zone *zone = this->zone(floorDivide(x, BlocksPerZone), floorDivide(y, BlocksPerZone));
        if (!zone) {
            return nullptr;
        }
        if (x < zone->m_position[0] * BlocksPerZone || y < zone->m_position[1] * BlocksPerZone ||
            x >= zone->m_position[0] * BlocksPerZone + BlocksPerZone ||
            y >= zone->m_position[1] * BlocksPerZone + BlocksPerZone) {
            return nullptr;
        }
        return &zone->m_fields[(long long)(x - zone->m_position[0] * BlocksPerZone) * BlocksPerZone +
                               (y - zone->m_position[1] * BlocksPerZone)];
    }
    // Writes the block through Zone::setBlock when its zone is loaded.
    void setBlock(int x, int y, int z, const Block &block)
    {
        Zone *zone = this->zone(floorDivide(x, BlocksPerZone), floorDivide(y, BlocksPerZone));
        if (zone) {
            zone->setBlock(x, y, z, block);
        }
    }
    // The player's creature.
    Creature *getLocalCreature() { return m_localCreature; }
    void spawnLocalCreature();
    int flightPrice(const plasma::Vector<2, int> &flightPoint);
    int flightDuration(const plasma::Vector<2, int> &from, const plasma::Vector<2, int> &flightPoint);
    // The biome type of a zone region ().
    BiomeIdentifier landBiome(int x, int y);
    // The same for a region as a vector.
    BiomeIdentifier landBiome(const plasma::Vector<2, int> &region) { return landBiome(region[0], region[1]); }
    Creature *creatureByID(long long id)
    {
        std::map<long long, Creature *>::iterator it = m_creaturesById.find(id);
        if (it == m_creaturesById.end()) {
            return nullptr;
        }
        return it->second;
    }
    // Appends the creature to m_creatures and m_creaturesById ().
    void addCreature(Creature *creature);
    // For the building types whose kind (buildingKind) is 1, 3, 4, 7 or 8 and for type 25: completes the set of open
    // sides (JoinedSideMinusX and JoinedSidePlusX one axis, JoinedSideMinusY and JoinedSidePlusY the other) of the
    // building at zone (x, y) so that each axis is open at both ends or at neither, and at least one axis is open
    // (chosen by the position's hash).
    void adjustOpenSides(int x, int y, int type, unsigned int *sides);
    // Gives the creature the buff (Creature::setBuff) and queues the buff packet for the clients.
    void applyBuff(Creature *creature, const Creature::Buff &buff, bool replace);
    // The buff a skill gives its user (skills 109, 112..115, 141, 146), with its packet, and the skill's cooldown.
    void applyBuffSkill(Creature *creature, int skill);
    void applyEvents(const ServerUpdates &events);
    // Applies a hit to its target.
    void applyHit(const ServerUpdates::HitPacket &hit);
    // Shared with despawnDeadCreatures. projectile is the projectile that hits (updateStepProjectiles passes it, every
    // other caller 0); it is not read.
    bool attackCreature(Creature *target, Creature *attacker, float damage, bool critical, bool powered,
                        float stunChance, const plasma::Vector<3, Fixed> &position,
                        const plasma::Vector<3, float> &direction, int element, int ability,
                        ServerUpdates::BulletPacket *projectile, bool allowBlocking);
    // What a creature leaves when it dies (applyHit), and count rewards dropped at a position for a creature with
    // toggle 8. Puts an item into the zone at a position, on the ground below it, and marks the zone in
    // m_zonesWithChangedItems. Equipment dropped by a creature other than a player is given to one player
    // (ZoneItem::ownerSeed, the player's seed or ownerId) and may rise in rarity; an item with a region takes the
    // region of the dropping creature's place. The angle is replaced by a random one. unused is passed (1.0, 0.75, 0.8
    // or 3.0) and not read.
    void dropItem(const Item &source, const plasma::Vector<3, Fixed> &position, float angle, float unused, int quantity,
                  Creature *creature, long long ownerId);
    void dropLoot(Creature *creature);
    void dropReward(const plasma::Vector<3, Fixed> &position, int count);
    // Tells the clients how much the target holds against the attacker (an AggroPacket with the target's m_aggro
    // entry), and makes every creature within 8 blocks of the target that can damage the attacker and holds nothing
    // against it yet join in with 0.5.
    void onAttack(Creature *attacker, Creature *target);
    float borderNoise(int x, int y, float weight);
    int boughtCount(const Item &item, long long shopId);
    // throughSolid, stopAtBarriers and other are not read: they are rayDistance's parameters, which inClearLine passes
    // on in the same places.
    float cameraRay(const plasma::Vector<3, Fixed> &position, const plasma::Vector<3, float> &direction, float range,
                    bool throughSolid, bool stopAtBarriers, Creature *other);
    // Whether a nonempty, live zone item is visible to the local player.
    bool isGroundItemAvailable(const ZoneItem &item);
    float rayDistance(const plasma::Vector<3, Fixed> &position, const plasma::Vector<3, float> &direction, float range,
                      bool throughSolid, bool stopAtBarriers, Creature *other);
    // Whether nothing blocks the straight line from a to b, which are at most range blocks apart: cameraRay along it
    // reaches b (within 0.01).
    bool inClearLine(const plasma::Vector<3, Fixed> &a, const plasma::Vector<3, Fixed> &b, bool throughSolid,
                     bool stopAtBarriers, float range, Creature *other);
    int castleDirection(int x, int y);
    // What a block column is in a square structure of the given size around a centre, turned by rotation quarter
    // turns: 2 the outer wall (two columns; 0 for a pseudo-random fifth of them unless marked), 3 the inner wall ring,
    // 4 the inside, 1 and 2 two marked spots on a side chosen by the hash of the two-zone cell.
    int castlePart(int x, int y, int direction, int rotation, int size, int centerX, int centerY);
    int castleRotation(int x, int y);
    // The part a castle's zone (zoneBuildingType 47) plays, from the hash of its 1024-block cell and its side of the
    // cell's centre: 0, 13, 14 or 15 (Zone::generate keeps the values 2..4 it returns on other paths). Only generate
    // calls it.
    int castleBuildingPart(int x, int y, int floor);
    int castleSize(int x, int y);
    // 0..1 from three noise octaves at a block column, 0 where an area of type 10 or 19 has more than half its
    // influence.
    float caveDensity(int x, int y, int areaType, float areaInfluence);
    // Parameters describe the terrain inputs supplied by computeBlock.
    BlockProperties<float> columnBlock(int x, int y, int z, float cliffs, float temperature, float humidity,
                                       float firelands, float undead, float enchantedForest);
    // Computes a terrain block from its column's biome sample, area and generator values using columnBlock. The cover
    // query uses the same inputs with coverBlock.
    BlockProperties<float> computeCoverBlock(int x, int y, int z);
    // The colour of the ground below the terrain at a block: earthColor with the biome sample's enclave, undead, lava,
    // humidity and temperature weights at the place's influence.
    plasma::Vector<3, float> groundColor(int x, int y, int z);
    BlockProperties<float> computeBlock(int x, int y, int z)
    {
        BiomeSample sample;
        biomeSample(x, y, sample);
        AreaInfo info;
        fillAreaInfo(x, y, info);
        float firelands = sample.lavaWeight(info.type, info.influence);
        float road = streetAmount(x, y, info.type, info.influence);
        float undead = sample.undead(info.type, info.influence);
        float mountains = sample.mountains();
        float wetlands = sample.wetness();
        float river = valleyWater(x, y, road, firelands, undead, wetlands, mountains, &info);
        float enchantedForest = sample.enclave(info.type, info.influence);
        float humidity = sample.humidity();
        float temperature = sample.temperature(info.type, info.influence);
        float poisonRiver = poisonChannels(x, y, road, undead, mountains, &info);
        float lavaRiver = lavaChannels(x, y, road, firelands, mountains, &info);
        float elevation = sample.biomeElevation(info.type, info.influence);
        float cliffs = mountainCliffs(x, y, elevation, mountains, river, lavaRiver, poisonRiver, road, &info);
        return columnBlock(x, y, z, cliffs, temperature, humidity, firelands, undead, enchantedForest);
    }
    plasma::Vector<3, float> earthColor(int x, int y, int z, float temperature, float humidity, float firelands,
                                        float undead, float enchantedForest);
    float biomeElevationAt(int x, int y)
    {
        BiomeSample sample;
        biomeSample(x, y, sample);
        AreaInfo info;
        fillAreaInfo(x, y, info);
        return sample.biomeElevation(info.type, info.influence);
    }
    plasma::Vector<3, float> rockColor(int x, int y, int z, float temperature, float humidity, float firelands,
                                       float undead);
    // groundColor's twin for the rock below the ground at a block: rockColor with the biome sample's temperature,
    // humidity, lava and undead weights at the place's influence.
    plasma::Vector<3, float> rockColor(int x, int y, int z);
    void destroyBlock(int x, int y, int z)
    {
        Field *field = this->field(x, y);
        if (field && z >= field->m_baseZ && z < field->m_baseZ + (int)field->m_blocks.size()) {
            if (z <= 0) {
                field->setBlock(z - field->m_baseZ, s_waterCube);
            } else {
                field->setBlock(z - field->m_baseZ, s_airCube);
            }
        }
    }
    void destroyBlock(Fixed x, Fixed y, Fixed z);
    // The dungeon variant (40..47, 0 for none) of the two-zone cell that contains zone (x, y), from the cell's
    // building type: 9 and 15 give 40 and 43 for half of the cells, 5 gives 41 for an eighth, 42 and 44..47
    // themselves.
    int dungeonType(int x, int y, int type, bool large);
    // A property 0, 1, 2, 4 or 5 of a zone (or of its two-zone cell) by its building type and the position's hash
    // (AreaInfo::environment).
    int environmentType(int x, int y, int type, bool large);
    // Rebuilds m_obstacles from the creatures that are obstacles (Creature::isObstacle).
    void gatherObstacles();
    // Whether the zone's building type is one of 14, 15, 17, 18, 21, 60, 62 or 100.
    bool hasMission(int x, int y);
    // The building type of a zone, 0 for none.
    BuildingType zoneBuildingType(int x, int y);
    // Whether a zone building type is 24, 45 or 46, and whether it is 22..26 or 92 (Zone's generator).
    bool isCourtType(int type);
    bool isCoveredType(int type);
    plasma::Vector<2, int> landCenter(int x, int y);
    int missionTargetType(int type, int x, int y, bool large);
    plasma::Vector<2, int> houseCorner(int x, int y);
    int housePart(int x, int y);
    int notchWidth(int x, int y);                    // 1 for half the positions, else 3..6
    plasma::Vector<2, int> siteInZone(int x, int y); // a block 1..62 inside zone (x, y)
    // Ignores the block coordinates passed by callers.
    plasma::Vector<3, float> snowColor(int x, int y);
    int streetSides(int x, int y); // JoinedSideMinusX, JoinedSidePlusX, JoinedSideMinusY, JoinedSidePlusY for isJoined
    bool isJoined(int x, int y, int otherX, int otherY);
    ZoneItem *item(const plasma::Vector<3, int> &id);
    // 2 for the ground level of building types 39, 86, 94, 95 and above ground for 47, else 1 above ground and 0 at or
    // below it; type 0 means the zone's own type.
    int structureLayer(int x, int y, int level, int type);
    int towerStoreyCount(int x, int y);       // Returns four storeys.
    int townSizeA(int x, int y);              // 100..139
    int townSizeB(int x, int y);              // 20..29
    int townSizeC(int x, int y, float scale); // 12..21
    int townSizeD(int x, int y, float scale); // 20..29
    void createSpiritParticles(const plasma::Vector<3, Fixed> &position, const plasma::Vector<3, float> &acceleration,
                               float strength, bool burst, const Item &item);

    // Distances use the player.
    plasma::Vector<3, Fixed> nearestShrine(const plasma::Vector<3, Fixed> &position);

    // The object of a zone by (zone x, zone y, index), or 0 (the first game file's AI and Game).
    StaticEntity *object(const plasma::Vector<3, int> &id);
    void onKill(const ServerUpdates::KillPacket &kill);
    void initCompanion(long long ownerId, Creature *companion);
    // Removes and deletes every non-player creature that is more than 320 blocks (horizontally) from all player
    // creatures.
    void despawnFarCreatures();
    // The centre of the place of a 1024-block cell (the map and SpawnTable's AreaInfo use it): a random point at least
    // the place's reach (its radius, 500 or 250, plus the 32 blocks of its border noise: LargePlaceReach,
    // SmallPlaceReach in WorldLayout.h) inside its cell (a large place's is the 2 x 2 cells of even coordinates). The
    // hash salts 502572 and 2561335 draw the two coordinates independently. The numbers stay literal in this header
    // (see WorldLayout.h).
    plasma::Vector<2, int> placeCenter(int x, int y, bool large)
    {
        if (x == 0 && y == 0) {
            return plasma::Vector<2, int>(BlocksPerPlace / 2, BlocksPerPlace / 2);
        }
        if (large) {
            x = floorDivide(x, 2) * 2;
            y = floorDivide(y, 2) * 2;
            return plasma::Vector<2, int>(random(x + 502572, y) % 984 + 532 + x * BlocksPerPlace,
                                          random(x + 2561335, y) % 984 + 532 + y * BlocksPerPlace);
        }
        return plasma::Vector<2, int>(random(x + 502572, y) % 460 + 282 + x * BlocksPerPlace,
                                      random(x + 2561335, y) % 460 + 282 + y * BlocksPerPlace);
    }
    // The same for a cell whose size is looked up.
    plasma::Vector<2, int> placeCenter(int x, int y) { return placeCenter(x, y, isLargePlace(x, y)); }
    // Where the cell's dungeon lies, in 16-block units: the place centre, or for a cell with a dungeonType the centre
    // snapped to the middle of its 128-block square, moved by up to two squares (except for types 9, 42 and 44..47)
    // and kept inside the cell.
    plasma::Vector<2, int> dungeonPosition(int x, int y, int type, bool large);
    // The level of a dungeon floor for the 128-block square that contains (x, y) (16-block units): 40 below the lowest
    // of the terrain heights at the square's corners and centre, or below structureLevel in the square of the cell's
    // dungeon position.
    int dungeonZ(int x, int y);
    // Tests whether a column should be capped with snow, using temperature and neighbouring heights. Callers without
    // neighbour heights supply zeros.
    float hasLyingSnow(int x, int y, float temperature, float heightWest, float heightEast, float heightSouth,
                       float heightNorth);
    // Whether a column of the given height is shore: below 0, or below a noise threshold of 0..20.
    bool isBeach(int x, int y, float height);
    // BiomeSample::biomeElevation at a place's centre, with full influence.
    float locationWaterWeight(const AreaInfo &info);
    // Colours (0..255 and above, as floats) by noise at a block column: grass between two greens, sand, and the
    // pavement of a place (white unless the place's type is 24, 45 or 46; sand within 60 blocks of a type 24 place's
    // centre square).
    plasma::Vector<3, float> harvestGrassColor(int x, int y);
    // Grass colour shifted by rockB, blended towards two noise colours by weightA and weightB and towards rockColor by
    // rockWeight. Selects block type 14 instead of BlockGrass above rockWeight 0.5. Three parameters are unused.
    BlockProperties<float> coverBlock(int x, int y, int z, float height, float temperature, float humidity, float sand,
                                      float fade, float path, float firelands, float undead, float enchantedForest);
    plasma::Vector<3, float> sandColor(int x, int y);
    plasma::Vector<3, float> masonryPavement(int x, int y, int z, const AreaInfo &info);
    // Two colours of the region around a point for the map: defaults (0, 180, 100) and (255, 320, 0), replaced below
    // weightA 0.15 and blended towards hashed colours by weightB and weightC.
    void groundColors(int x, int y, plasma::Vector<3, float> &foliageBottom, plasma::Vector<3, float> &foliageTop,
                      float temperature, float undead, float enchantedForest);
    // The same with both colours' defaults hashed from the point.
    void groundHashedColors(int x, int y, plasma::Vector<3, float> &foliageBottom, plasma::Vector<3, float> &foliageTop,
                            float undead, float enchantedForest);
    // What stands on a 3 x 3 zone plot (x, y in zones) of a place with environment 5 (0 for others): the entry of one
    // of the 4 x 4 plot patterns of m_creatureViewDistance around the place's centre, else by the place's influence
    // and the terrain 1 (hashed, more likely near the centre), 13 or 0. Called by streetBuildingType, Zone::generate
    // and the map's region generator.
    int hamletPlot(int x, int y, int environment, const plasma::Vector<2, int> &dungeon);
    // The height offset of a zone by its building type: -10..-40 for the types 72..85 (in steps of five types), 0..10
    // by noise inside a castle (types 45, 46; 0 for castle parts 0, 1 and 4), else 0. Only Zone::generate calls it.
    int streetHeightOffset(int zoneX, int zoneY, int storey);
    // The roof height of a column (x, y in 0..15) of a zone building by its type, the sides (JoinedSideMinusX,
    // JoinedSidePlusX, JoinedSideMinusY, JoinedSidePlusY) on which the building continues, and the building's kind: 8
    // minus the distance from the ridge, doubled or halved for some types.
    int roofHeight(int x, int y, int type, int sides, int kind);
    // The floor and the top of a place's structure at a block column. base is raised first by the place's radius for
    // environments 1 and 2; zone is a zone position; height is what masonryFloor returns elsewhere, rounded down.
    int masonryFloor(int x, int y, const AreaInfo &info, const plasma::Vector<2, int> &zone, float height, float base);
    int monumentTop(int x, int y, const AreaInfo &info, const plasma::Vector<2, int> &zone, float height, float base);
    // Whether the building of a zone (streetBuildingType at the zone's spawn point) has the given storey: below
    // buildingStoreys of its type, and for type 45 inside a castle part.
    bool isOccupiedStorey(int zoneX, int zoneY, int storey);
    // Surface block at height z: lava or poison above weight 0.5; columnBlock below all five heights, in water, on a
    // path or beach; otherwise coverBlock blended with columnBlock where neighbouring heights differ by more than a
    // block. Path values above 0.5 select a path block.
    BlockProperties<float> surfaceBlock(int x, int y, int z, float height, float heightWest, float heightEast,
                                        float heightSouth, float heightNorth, float cliffs, float temperature,
                                        float humidity, float sand, float fade, float path, float firelands,
                                        float undead, float enchantedForest, float lava, float poison);
    void harvestEnvironmentColors(const plasma::Vector<3, Fixed> &position, plasma::Vector<4, float> &skyColor1,
                                  plasma::Vector<4, float> &skyColor2, plasma::Vector<4, float> &fogColor,
                                  plasma::Vector<4, float> &sunColor, plasma::Vector<4, float> &light,
                                  plasma::Vector<4, float> &backLightColor, plasma::Vector<4, float> &ambientColor);
    // Two hashed vegetation colours chosen from four palettes by slope, rockB and hash, then blended with
    // groundColors. Level ground uses groundColors' fixed pair.
    void harvestColors(int x, int y, plasma::Vector<3, float> &foliageBottom, plasma::Vector<3, float> &foliageTop,
                       float temperature, float humidity, float undead, float enchantedForest);
    // (1 - r)^2 for r = 1 - |noise + 0.05 * finer noise| (height).
    float hillRidge(int x, int y);
    bool isBlocked(const plasma::Vector<3, Fixed> &position, const plasma::Vector<3, float> &size, bool ignorePassable,
                   bool testDecorations);
    // Whether the local creature is on a flight more than 1000 blocks from its target and started more than 16384 from
    // it.
    bool isFlyingFar();
    // The colours of lava and of poison at a column (also expanded in surfaceBlock).
    plasma::Vector<3, float> lavaColor(int x, int y);
    plasma::Vector<3, float> muckColor(int x, int y);
    // BiomeSample::biomeElevation at a point, without a place (locationType).
    float marineWeight(int x, int y);
    // The noise displacement (up to 32 blocks) that placeSquaredDistance applies (height).
    plasma::Vector<2, float> morphOffset(int x, int y);
    // Whether a 3 x 3 zone plot other than the one of zone lies in the inner part of its cell's place (influence at
    // least 0.5); false for type 0 (streetBuildingType).
    bool isPlotInner(int plotX, int plotY, int type, const plasma::Vector<2, int> &zone);
    plasma::Vector<3, Fixed> startPosition();
    // base plus noise times amplitude.
    float roughness(int x, int y, int base, int amplitude);
    // height moved towards std::max(10, locationWaterWeight) by the place's influence (smoothed and doubled for types
    // 8, 13 and 14); unchanged for types 0, 10, 17, 19, 21, 28 and 36. x and y are not used.
    float villageHeight(int x, int y, float height, const AreaInfo *info);
    bool groundedFormula(const Item &item, Formula &formula, bool unused);
    void groundlessFormula(int id, Formula &formula);
    // Tests whether a column lies in a place's crater (environment 5), setting the terrain height and lowered floor
    // there.
    bool groundCrater(int x, int y, const AreaInfo &info, float weight, float *floor, float *terrainHeight);
    // 20 for a cell whose third byte is 1, else 16 (Zone::Generate adds it to dungeonZLevel).
    int dungeonStorey(StructureCell cell);
    // The flag's meaning is unknown.
    StructureCell structureCell(int x, int y, bool flag);
    plasma::Vector<3, float> dungeonShade(int x, int y);
    // The region of a world position: findRegion of its block, or of the centre of the block's cell's place when the
    // block there has BlockFlagDungeon.
    plasma::Vector<2, int> findRegion(const plasma::Vector<3, Fixed> &position);
    // The level in blocks on which the building of the zone square (x, y) (16-block units) stands; type is its zone
    // building type, or 0 to look it up. Castles stand a block above the terrain at the place's centre, dungeon
    // entrances on the highest terrain of their 32-block square, the others on or above the terrain around the square.
    int structureLevel(int x, int y, int type);
    // dungeonZ averaged with the higher neighbour square at a square's edge, raised by 0..5 where the cell's corridor
    // has CorridorFlagUnevenFloor.
    int dungeonZLevel(int x, int y);
    void fillArmorShop(Inventory &shop, Creature *shopkeeper);
    void fillFormulaShop(Inventory &shop, const long long &shopId);
    void fillGeneralShop(Inventory &shop, const long long &shopId);
    void fillWeaponShop(Inventory &shop, Creature *shopkeeper);
    // A random item of a quality tier for a class (the two shops above; which makes armour and which weapons is a
    // guess).
    Item randomArmor(int tier, int classType, bool plain);
    // Random pet-food item (category 21): half the time from m_petFoodByRace for key when available, otherwise from
    // m_petRaces. dropLoot supplies key 0.
    Item randomPetFood(int key);
    plasma::Vector<2, int> randomStartCell();
    std::vector<std::string> talkScript(Creature *creature);
    // Empty hook.
    void beforeServerUpdates();
    // The third argument is unused; Game passes false for both item generators.
    Item randomWeapon(int tier, int classType, bool unused);
    // A tier 0..4 of the local player (0 without one) that bounds the shops' item tiers: the number of missions solved
    // in the player's region (Missions::solvedCount).
    int mostLevelTier();
    void carryCreature(Creature *creature, float milliseconds);
    // inRange with the range reduced by the other creature's stealth (the first game file's AI).
    bool canSee(Creature *creature, Creature *other, float range);
    // Whether b is at most range blocks from a and rayDistance from a towards b reaches it (within 0.01).
    bool inRange(const plasma::Vector<3, Fixed> &a, const plasma::Vector<3, Fixed> &b, bool stopAtBarriers, float range,
                 Creature *other);
    // Removes the creature from both and deletes it.
    void removeCreature(Creature *creature);
    void retainLocalCreature();
    void changeLocalCreatureId(long long id);
    void onNewDay();
    // Called after retainLocalCreature by Client::connect.
    void updateChanges();
    bool isLargePlace(int x, int y);
    // The second reason for a large place: true for one cell pair in seven along a diagonal pattern.
    bool isLargePlaceAlways(int x, int y);
    // Squared block distance from a place's centre after noise displacement of up to 32 blocks, producing an irregular
    // border. Displacement uses anisotropic noise frequencies 0.005 and 0.01, swapped between axes. The function only
    // reads center.
    float placeSquaredDistance(const plasma::Vector<2, int> &center, int x, int y)
    {
        plasma::Vector<2, float> offset(noise(x * 0.005 + 18843.0, y * 0.01 + 9432.0) * 32.0f,
                                        noise(x * 0.01 + 299.0, y * 0.005 + 90650.0) * 32.0f);
        float dx = (float)(Fixed(((long long)x - center[0]) << FixedFractionBits) + offset[0]);
        float dy = (float)(Fixed(((long long)y - center[1]) << FixedFractionBits) + offset[1]);
        return dy * dy + dx * dx;
    }
    int locationType(int x, int y, bool large);
    // The type of the place of a cell, large or not. It passes the two coordinates on to the overload below: the copy
    // reads each element once and keeps it across the isLargePlace call.
    int locationType(const plasma::Vector<2, int> &cell) { return locationType(cell[0], cell[1]); }
    // Returns the location type from two cell coordinates.
    int locationType(int x, int y) { return locationType(x, y, isLargePlace(x, y)); }
    int randomPet(int seed) { return m_petRaces[hash(seed) % m_petRaces.size()]; }
    float height(int x, int y, int mode);
    // The region (8192-block cell with a hashed centre; Item::region, EntityData::region) that a block position
    // belongs to. Inside the inner part of a place (influence at least 0.25) the place's centre decides, so that a
    // place lies in one region.
    plasma::Vector<2, int> findRegion(int x, int y);
    // The same for a block position as a vector.
    plasma::Vector<2, int> findRegion(const plasma::Vector<2, int> &position)
    {
        return findRegion(position[0], position[1]);
    }
    // The region whose centre is nearest to the block position after the position is displaced by noise (up to 1024
    // blocks).
    plasma::Vector<2, int> findNearestRegion(int x, int y);
    float findRegionNoise(int x, int y, float weight);
    bool hasCraftingRank(int rank);
    // PreviewWidget::renderArtifact shows nothing otherwise.
    bool hasLore(int x, int y);
    // The id of the realm a region belongs to (the key of Speech's "realmname" and of PlayerData::loreByRealm).
    int mortalRealm(int regionX, int regionY, bool large);
    std::vector<std::string> storyScript(int x, int y);
    plasma::Vector<2, int> streetPoint(int zoneX,
                                       int zoneY); // a hashed point inside the zone's 16 x 16 square
    // Writes only info; does not retain a reference to it.
    void fillAreaInfo(int x, int y, AreaInfo &info)
    {
        std::array<int, 2> cell = {floorDivide(x, BlocksPerPlace), floorDivide(y, BlocksPerPlace)};
        info.large = isLargePlace(cell[0], cell[1]);
        info.center = placeCenter(cell[0], cell[1], info.large);
        int reach = info.large ? 532 : 282; // LargePlaceReach, SmallPlaceReach
        int environment;
        plasma::Vector<2, int> offset(x - info.center[0], y - info.center[1]);
        if (offset.squaredLength() > reach * reach) {
            info.squaredDistance = 0.0f;
            info.influence = 0.0f;
            info.type = PlaceNone;
            environment = EnvironmentTypeNone;
        } else {
            int type = locationType(cell[0], cell[1], info.large);
            info.type = static_cast<PlaceType>(type);
            if (type != PlaceNone) {
                int radius = 250;
                if (info.large) {
                    radius = 500;
                }
                info.radius = radius;
                float squaredDistance = placeSquaredDistance(info.center, x, y);
                info.squaredDistance = squaredDistance;
                float squaredRadius = (float)(radius * radius);
                info.influence = squaredDistance >= squaredRadius ? 0.0f
                                                                  : (1.0f - squaredDistance / squaredRadius) *
                                                                        (1.0f - squaredDistance / squaredRadius);
                environment = environmentType(cell[0], cell[1], type, info.large);
            } else {
                info.squaredDistance = 0.0f;
                info.influence = 0.0f;
                environment = 0;
            }
        }
        info.environment = static_cast<EnvironmentType>(environment);
    }
    // fillAreaInfo in two steps, for callers that walk the columns of one 1024-block cell (Zone::generate): getPlace
    // fills what depends on the cell only (large, type, radius, center, environment), and getPlaceInfluence adds
    // squaredDistance and influence for a block column. Unlike fillAreaInfo they do not test the distance to the
    // centre first.
    void getPlace(int cellX, int cellY, AreaInfo &info);
    void getPlaceInfluence(int x, int y, AreaInfo &info);
    // The influence of a place on a point at a squared distance from its centre: (1 - d^2 / r^2)^2 inside the radius
    // (500 blocks for a large place, else 250), 0 outside and for type 0. The place cell (1024 blocks) a block
    // coordinate lies in, and the mortal realm of a place by its size.
    static int placeIndex(int coordinate) { return floorDivide(coordinate, BlocksPerPlace); }
    // The zone (64 blocks) a block coordinate lies in.
    static int zoneIndex(int coordinate) { return floorDivide(coordinate, BlocksPerZone); }
    int placeRealm(int placeX, int placeY) { return mortalRealm(placeX, placeY, isLargePlace(placeX, placeY)); }
    float placeInfluence(int type, bool large, float squaredDistance)
    {
        if (type == PlaceNone) {
            return 0.0f;
        }
        int radius = 250;
        if (large)
            radius = 500;
        float squaredRadius = (float)(radius * radius);
        if (squaredDistance >= squaredRadius) {
            return 0.0f;
        }
        return (1.0f - squaredDistance / squaredRadius) * (1.0f - squaredDistance / squaredRadius);
    }
    BuildingType
    streetBuildingType(int zoneX, int zoneY, const plasma::Vector<2, int> &point,
                       const AreaInfo &info); // 1..4 make chooseGroup set location SpawnLocationStreetBuilding
    void biomeSample(int x, int y, BiomeSample &sample);
    // Terrain generator inputs for one block column, computed from its biome weights and passed to height. Each weight
    // lies in 0..1. The road weight is 1 on a road and 0 away from roads.
    float streetAmount(int x, int y, int areaType, float areaInfluence);
    // Water channels (rivers and lakes): ridged noise that is 1 along its zero lines, wider in wetlands, absent in the
    // firelands and the undead biomes and on roads. > 0.25 SpawnLocationRiverEdge, > 0.5 SpawnLocationRiver.
    float valleyWater(int x, int y, float road, float firelands, float undead, float wetlands, float mountains,
                      AreaInfo *info);
    // The same channels in the firelands (lava rivers, lava lakes, a volcano's crater): surfaceBlock's lava.
    float lavaChannels(int x, int y, float road, float firelands, float mountains, AreaInfo *info);
    // The same channels in the undead biomes (deadlands, dark woods): surfaceBlock's poison.
    float poisonChannels(int x, int y, float road, float undead, float mountains, AreaInfo *info);
    // Mountain ridges and cliffs: noise ridges strengthened by the mountains biome and mountain or volcano places,
    // kept off the channels and roads and below the sea; height raises the ground above 0.5 (columnBlock gives rock).
    // > 0.75 location SpawnLocationCliffs.
    float mountainCliffs(int x, int y, float elevation, float mountains, float river, float lavaRiver,
                         float poisonRiver, float road, AreaInfo *info);
    // Bare ground instead of grass (three noise octaves raised by low humidity and by desertness; the cliffs value is
    // not read): above 0.5 surfaceBlock gives the column's own block and height lowers it by one.
    float sandAmount(int x, int y, float humidity, float cliffs, float desertness);
    // The terrain height of a column from those values. <= 0 makes chooseGroup set location SpawnLocationUnderwater,
    // 0..10 SpawnLocationCoast.
    float height(int x, int y, float elevation, float cliffs, float river, float lavaRiver, float poisonRiver,
                 float sand, float road, float mountains, AreaInfo *info, int flags);
    void init(int seed, const std::string &name);
    void removeCreatures();
    bool requireZones(const std::set<std::array<int, 2>> &zones);
    // Generates the zone ().
    Zone *generateZone(int x, int y);
    // Writes a world state to the save database (the host's generation thread, once a minute and when it ends).
    void saveState(const WorldState &state);
    // Reads the world state from the save database (key "state"), when it has one ().
    void loadState();
    void updateStep(float milliseconds);
    // Per-tick simulation helpers, including the world clock.
    void updateTime(float milliseconds);
    // 2,653 instructions: the static entities of the zones (scaledSize, getBlockAtDot).
    void updateZonesState(float milliseconds);
    // Host only (m_isClient clear): the creatures of the loaded zones (it calls despawnFarCreatures' neighbours) and
    // the hit packets (Creature::canDoDamage).
    void updateTerritory(float milliseconds);
    void despawnDeadCreatures(float milliseconds);
    // The pets (Creature::reset, initCompanion, findBuff).
    void updateTamed();
    // Per creature: movement (anticipationTime, getBlock), the ability in progress (strikeTime, manaCost), buffs, and
    // what goes on for the dead too (maxHP, skillLevel).
    void moveCreature(Creature *creature, float milliseconds);
    void updateCollision(Creature *creature);
    plasma::Vector<3, float> wind(const plasma::Vector<3, Fixed> &position, const plasma::Vector<3, float> &velocity);
    void performAbility(Creature *creature, float milliseconds);
    void passInteractions(Creature *creature, float milliseconds);
    void updateSurvival(Creature *creature, float milliseconds);
    // Rebuilds terrain columns in a changed rectangular area.
    void updateSwath(int firstX, int firstY, int lastX, int lastY);
    void resetBuff(Creature *creature, int type);
    // Called with onNewDay when the host announces a new day (a 0 in packet list 13); updateTime calls both too.
    void rewindDay();
    // Updates projectile motion, contacts, terrain damage and expiration.
    void updateStepProjectiles(float milliseconds);
    void updateTasks();
    // Whether a static entity of the given type (and, with requireActive, an active one) lies within 8 blocks of the
    // local player. updateStep offers the whistle by a bird statue, the gate key by a locked gate, the flute by an
    // active goddess statue, the bell by a mourning statue or a grid and the harp by an active golden gate. The name
    // is only a word of the right place, between isHallEnd and isHallNearEnd.
    bool isHallNear(int type, bool requireActive);
    bool emptyUpdateStep() { return false; }
    void inspectCreatures(float milliseconds);
    void updateZones();
    bool removeFarZone(int radius);
    void removeZones();
    void enterZones();
    void leaveZones();
    void enterZoneMeshes();
    void leaveZoneMeshes();
    void setStaticActive(StaticEntity *entity, bool active);
    void storeZone(Zone *zone);

    WorldState m_state;
    std::list<Creature *> m_creatures; // ~World deletes each
    std::list<Creature *> m_obstacles;
    std::list<ServerUpdates::BulletPacket> m_projectiles;
    SpriteManager *m_spriteManager; // the constructor's argument
    AIManager m_aiManager;
    ServerUpdates m_serverUpdates;
    std::set<Zone *> m_zonesWithChangedItems;
    std::set<Zone *> m_zonesWithChangedObjects;
    SpawnTable m_spawnTable;
    std::string m_worldName; // Assigned when the world is set up.
    int m_seed;              // Also used as srand's seed.
    int m_seededRandBuf0[4];
    int m_seededRandBuf1[4];
    int m_generationSeed; // generateZone's srand((y << 10) + m_generationSeed + x)
    int m_seededRandBuf3[20];
    int m_seededRandBuf4[2];
    int m_seededRandBuf5[2];
    int m_seededRandBuf6[2];
    int m_seededRandBuf7[2];
    int m_seededRandBuf8[8];
    int m_seededRandBuf9[6];
    int m_seededRandBuf10[6];
    int m_seededRandBuf11[10];
    int m_sandShadeOffset[4];
    int m_seededRandBuf13[2];
    int m_seededRandBuf14[2];
    int m_regionNoiseOffset[2];
    bool m_isClient;
    std::vector<int> m_petRaces;                     // Thirty-five constant IDs supplied by the constructor.
    std::map<int, std::vector<int>> m_petFoodByRace; // Eight lists of four-integer entries set by the constructor.
    std::vector<long long> m_unusedVector388;        // Cleared by loadWorld; never filled or read.
    std::vector<long long> m_unusedVector3a0;        // Same type and use as the preceding container.
    Database m_database;
    CRITICAL_SECTION m_zoneMeshCriticalSection;
    CRITICAL_SECTION m_zonesLock;
    CRITICAL_SECTION m_unusedLock; // Initialised and deleted, but never entered.
    // Running time in milliseconds; initially zero.
    float m_clock;
    Creature *m_localCreature;
    // Creatures keyed by signed IDs.
    std::map<long long, Creature *> m_creaturesById;
    std::unordered_map<unsigned long long, Zone *> m_zones;
    std::vector<std::vector<int>> m_hamletPlotLayouts;
    // No code of the program reads any of the six (the only other accesses are ~World's), so they are named after
    // their contents (material items, category 11).
    std::vector<Item> m_ironCubeItems;     // an iron cube
    std::vector<Item> m_woodAndPlantItems; // a wood cube, a dewdrop, plant fiber, a mushroom, a shimmer mushroom
    std::vector<Item> m_unusedItems4e8;
    std::vector<Item> m_unusedItems500;
    std::vector<Item> m_unusedItems518;
    std::vector<Item> m_unusedItems530;
};

// Corridor describes a block column along a dungeon passage from its start towards a region border, wandering sideways
// according to dungeonWayX and dungeonWayY. Directions are 0: -x, 1: +x, 2: -y, 3: +y.
struct Corridor {
    Corridor()
        : kind(Corridor_None),
          flags(0),
          width(0),
          length(0)
    {}

    unsigned char
        kind; // Corridor cell kind: 0 absent, 1 plain, 2 and 3 initial columns, 6 and 7 final columns, 5 a side cell; other values are chosen by position hash.
    unsigned char flags;  // Corridor flags selected by position hash: pillars, raised walks and uneven floor.
    unsigned char width;  // Value 1 for types 42 and 44, or 2 for some side cells.
    unsigned char side;   // Side-cell neighbour: 1..4 for -x, +x, -y, +y. Not initialised by the default constructor.
    unsigned char length; // Low byte of the corridor length up to this column.
};
int dungeonWayX(int y, int startX, int startY, int regionX, int regionY);
int dungeonWayY(int x, int startX, int startY, int regionX, int regionY);
int dungeonWayLength(int x, int y, int direction, int startX, int startY, int regionX, int regionY);
bool isCorridor(int x, int y, int direction, int startX, int startY, int regionX, int regionY);
bool isHallEnd(int x, int y, int direction, int startX, int startY, int regionX, int regionY);
bool isHallNearEnd(int x, int y, int direction, int startX, int startY, int regionX, int regionY);
Corridor basicCorridor(int x, int y, int direction, int startX, int startY, int type, int regionX, int regionY);
Corridor dungeonWay(int x, int y, int direction, int startX, int startY, int type, int regionX, int regionY);

// Returns the block containing a world position, converting each coordinate with Fixed::toBlock.
inline plasma::Vector<3, int> fixedToBlock(const plasma::Vector<3, Fixed> &position)
{
    return plasma::Vector<3, int>(position[0].toBlock(), position[1].toBlock(), position[2].toBlock());
}

int castleZoneType(int x, int y, int cellX, int cellY, int direction, int rotation, int size, int centerX, int centerY);
// Colours of a building of a type at cell (x, y) in a realm of a form and biome: the walls, the trim and the roof
// (hashes of the cell and type).
plasma::Vector<3, float> buildingColor(int type, int x, int y, int realm, int form, int biome);
plasma::Vector<3, float> buildingTrimColor(int type, int x, int y, int realm, int form, int biome);
plasma::Vector<3, float> roofColor(int type, int x, int y, int realm, int form, int biome);
// The radius of a realm's towers (the type 69 and 70 structures).
int realmTowerRadius(int realm);
// Whether a building kind 3 tower of a realm has a trimmed cone (half of the realms by a hash) and a band.
bool hasTowerTrim(int type, int realm, int form, int biome);
bool hasTowerBand(int type, int realm, int form, int biome);
// The style of a type 52 tower's top (0 or 1 by a hash of the realm; 0 for other types) and whether it carries a dome.
int hasBattlements(int type, int realm, int form, int biome);
bool hasRoof(int type, int realm, int form, int biome);
// The storey height in blocks of a building type (8, 10, 15 or 25 by a table of the types 13..100, else 0):
// Zone::generate places the storeys of a house every so many blocks and tests it for 0 (a storeyed type).
int buildingStoreyHeight(int type);
// A house's style, 1..6 by its biome (a table of the biomes 3..12; 5 or 6 by a hash of the realm for one of them).
int houseStyle(int realm, int form, int biome);
plasma::Vector<3, float> styleColor(int type, int realm, int form, int biome);
// Whether a castle part (types 40..47 but 44) joins a neighbouring part of the same family that is at least as high
// (buildingHeight); Zone::generate leaves the shared wall of such a pair without battlements.
bool isJoinedCastle(int type, int neighbourType);
bool hasWalls(int type);

int initialFloors(int x, int y);

bool distanceExceeds2D(plasma::Vector<3, Fixed> a, plasma::Vector<3, Fixed> b, float distance);

void installNoise();
// Divides value by size, rounding down for positive size; used by structureLevel and Zone::generate.
int partIndex(int value, int size);

// Additional shared world-generation queries.
bool skipsCellDecorations(int type);
bool isCastleBuilding(int type);
bool isDungeonBuilding(int type);
bool isPlainBuilding(int type);
int buildingKind(int type);
int buildingStoreys(int type);
int buildingReach(int type);
int buildingHeight(int type);

inline bool distanceExceeds(const plasma::Vector<3, Fixed> &a, const plasma::Vector<3, Fixed> &b, float distance)
{
    if (fabs((double)(a - b)[0]) > distance || fabs((double)(a - b)[1]) > distance)
        return true;
    plasma::Vector<3, Fixed> difference = a - b;
    plasma::Vector<3, double> d((double)difference[0], (double)difference[1], (double)difference[2]);
    return d[0] * d[0] + d[1] * d[1] + d[2] * d[2] > distance * distance;
}

// Shared block getters.
inline Block Zone::getBlock(int x, int y, int z)
{
    Field *field = this->field(x, y);
    if (!field)
        return s_solidCube;
    if (z < field->m_baseZ)
        return s_solidCube;
    if (z >= field->m_baseZ + (int)field->m_blocks.size()) {
        if (z > 0)
            return s_airCube;
        return s_waterCube;
    }
    Block block = field->getBlock(z - field->m_baseZ);
    if (block.type == BlockAir && z <= 0 && (block.flags & BlockFlagProtected) == 0)
        return s_waterCube;
    return block;
}

inline Block World::getBlock(int x, int y, int z)
{
    Zone *zone = this->zone(x >= 0 ? x / BlocksPerZone : (x + 1) / BlocksPerZone - 1,
                            y >= 0 ? y / BlocksPerZone : (y + 1) / BlocksPerZone - 1);
    if (!zone)
        return s_solidCube;
    return zone->getBlock(x, y, z);
}

inline Block World::getBlockAtDot(Fixed x, Fixed y, Fixed z)
{
    return getBlock(x.toBlock(), y.toBlock(), z.toBlock());
}

inline int World::cubeLight(int x, int y, int z, int channel)
{
    Zone *zone = this->zone(floorDivide(x, BlocksPerZone), floorDivide(y, BlocksPerZone));
    if (!zone) {
        return 255;
    }
    Block block = zone->getBlock(x, y, z);
    while (block.type == BlockSnow || block.type == BlockType_12 || block.type == BlockPoison) {
        z++;
        block = getBlock(x, y, z);
    }
    if (block.isSolid()) {
        return block.lightLevel;
    }
    if (block.color[channel] < 5) {
        return (std::max)(5, (int)block.lightLevel);
    }
    return block.color[channel] < block.lightLevel ? block.lightLevel : block.color[channel];
}

inline int World::cubeLight(Fixed x, Fixed y, Fixed z, int channel)
{
    return cubeLight(x.toBlock(), y.toBlock(), z.toBlock(), channel);
}

plasma::Vector<3, float> entitySize(int type);
float entityScale(int type);
// The zone (64 blocks) a world (dot) coordinate lies in.
int zoneCoordinate(Fixed position);
// The 16-block cell a world (dot) coordinate lies in; the intermediate block index is truncated to 32 bits before the
// second floor division.
inline int tileCoordinate(Fixed position)
{
    int block = position.toBlock();
    return block >= 0 ? block / BlocksPerCell : (block + 1) / BlocksPerCell - 1;
}

int metroNameSeed(int x, int y);
// The name variant of the place of a cell for its type (-1 when the type has none).
int metroNameVariant(int x, int y, int type);

}
