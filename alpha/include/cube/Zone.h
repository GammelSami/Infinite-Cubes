// Zone owns 256-by-256 terrain columns, generated objects and creatures, plants, houses and cell object lists. Its critical section protects concurrent updates. Unidentified state remains opaque.
#pragma once
#include <windows.h>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "cube/Field.h"
#include "cube/Fixed.h"
#include "cube/Inventory.h"
#include "cube/Region.h"

namespace cube {

class House;
struct Spawn;

// Zone object types (Zone::ObjectState::type) named by World's static object keys
// (World:m_staticKeys: L"FireTrap" for 6,...). World::update seats creatures on the stools and
// benches, lays them on the beds and mats, and lets the traps hurt them. The building generators
// use this same ID domain; World's static keys name their furniture variants.
enum StaticObjectType {
    StaticObjectCastleShelf1 = 35,
    StaticObjectCastleShelf2 = 36,
    StaticObjectCastleShelf3 = 37,
    StaticObjectStoneShelf1 = 38,
    StaticObjectStoneShelf2 = 39,
    StaticObjectStoneShelf3 = 40,
    StaticObjectSandstoneShelf1 = 41,
    StaticObjectSandstoneShelf2 = 42,
    StaticObjectSandstoneShelf3 = 43,
    StaticObjectStatue = 0,
    StaticObjectDoor = 1,
    StaticObjectBigDoor = 2,
    StaticObjectWindow = 3,
    StaticObjectGate = 5,
    StaticObjectFireTrap = 6,
    StaticObjectSpikeTrap = 7,
    StaticObjectStompTrap = 8,
    StaticObjectLever = 9,
    StaticObjectChest = 10,
    StaticObjectTable = 12,
    StaticObjectStool = 16,
    StaticObjectBench = 18,
    StaticObjectBed = 19,
    StaticObjectBedTable = 20,
    StaticObjectMarketStand1 = 21,
    StaticObjectBarrel = 24,
    StaticObjectCrate = 25,
    StaticObjectOpenCrate = 26,
    StaticObjectSack = 27,
    StaticObjectShelter = 28,
    StaticObjectCupboard = 29,
    StaticObjectDesktop = 30,
    StaticObjectCounter = 31,
    StaticObjectShelf1 = 32,
    StaticObjectRuneStone = 45,
    StaticObjectStreetLight = 50,
    StaticObjectFence1 = 52,
    StaticObjectVase1 = 56,
    StaticObjectCampfire = 65,
    StaticObjectTent = 66,
    StaticObjectBeachTowel = 68,
    StaticObjectSleepingMat = 69,
    // The crafting stations (GameController::update's "[R] Use..." prompts).
    StaticObjectFurnace = 71,
    StaticObjectAnvil = 72,
    StaticObjectSpinningWheel = 73,
    StaticObjectLoom = 74,
    StaticObjectSaw = 75,
    StaticObjectWorkbench = 76,
    StaticObjectCustomizationBench = 77,
    StaticObjectCastleWindow = 4,
    StaticObjectTable14 = 14,
    StaticObjectStoneTable = 13,
    StaticObjectStoneStool = 15,
    StaticObjectSandstoneStool = 17,
    StaticObjectMarketStand2 = 22,
    StaticObjectMarketStand3 = 23,
    StaticObjectShelf2 = 33,
    StaticObjectShelf3 = 34,
    StaticObjectCorpse = 44,
    StaticObjectArtifact = 46,
    StaticObjectFlowerBox1 = 47,
    StaticObjectFlowerBox2 = 48,
    StaticObjectFlowerBox3 = 49,
    StaticObjectFireStreetLight = 51,
    StaticObjectFence2 = 53,
    StaticObjectFence3 = 54,
    StaticObjectFence4 = 55,
    StaticObjectVase2 = 57,
    StaticObjectVase3 = 58,
    StaticObjectVase4 = 59,
    StaticObjectVase5 = 60,
    StaticObjectVase6 = 61,
    StaticObjectVase7 = 62,
    StaticObjectVase8 = 63,
    StaticObjectVase9 = 64,
    StaticObjectBeachUmbrella = 67,
    // GameController constructor: the chest-top sprite has its own table slot.
    StaticObjectChestLid = 11
};
// Number of entries, not an ID.
enum {
    StaticObjectTypeCount = 78
};
// The nine vases, StaticObjectVase1.. StaticObjectVase9 (a house's and a dungeon's random vase).
enum {
    StaticObjectVaseCount = StaticObjectVase9 - StaticObjectVase1 + 1
};

// Object states: a generated chest in state 2 spills its inventory on first use.
enum StaticObjectState {
    StaticObjectInactive = 0,
    StaticObjectActive = 1,
    StaticObjectStockedChest = 2
};

// The update's four airship courses.
enum AirshipFlightState {
    AirshipFlyBehindHome = 0,
    AirshipReturnHome = 1,
    AirshipFlyAheadOfHome = 2,
    AirshipFlyToDestination = 3
};

// Plant type indexes GameController's plant-sprite table; these values differ from SpriteId.
enum PlantType {
    PlantFlowers2 = 0,
    PlantFlowers = 1,
    PlantGrass = 2,
    PlantGrass2 = 3,
    PlantGrass3 = 4,
    PlantUnderwaterPlant = 5,
    PlantCoral = 6,
    PlantAlga = 7,
    PlantLavaFlower = 8,
    PlantLavaGrass = 9,
    PlantThornPlant = 10,
    PlantEchinacea2 = 11,
    PlantLeaf = 12,
    PlantLantern02 = 13,
    PlantPlantTorch = 14,
    PlantStone = 15,
    PlantStone2 = 16,
    PlantChristmasTree = 17,
    PlantTendril = 19,
    PlantTulipsColorful = 20,
    PlantCornflower = 21,
    PlantReed = 22,
    PlantPumpkinLeaves = 23,
    PlantPineappleLeaves = 24,
    PlantSunflower = 25,
    PlantBeanTendril = 26,
    PlantDesertFlower01 = 27,
    PlantDesertFlower02 = 28,
    PlantWheat = 29,
    PlantCorn = 30,
    PlantWaterLily01 = 31,
    PlantWaterLily02 = 32,
    PlantInnSign = 33,
    PlantShopSign = 34,
    PlantWeaponShopSign = 35,
    PlantArmorShopSign = 36,
    PlantIdentifierSign = 37,
    PlantSmithySign = 38,
    PlantCarpentersshopSign = 39,
    PlantWeavingmillSign = 40,
    PlantIvy = 41,
    PlantWallRosesRed = 42,
    PlantWallRosesWhite = 43,
    PlantIncaArt1 = 44,
    PlantIncaArt2 = 45,
    PlantIncaArt3 = 46,
    PlantIncaArt4 = 47,
    PlantCrest1 = 48,
    PlantWallSkull = 49,
    PlantCastleChain = 50,
    PlantTorchRead = 51,
    PlantTorchGreen = 52,
    PlantTorchBlue = 53,
    PlantTorchYellow = 54,
    PlantLiana = 55,
    PlantChandelier = 56,
    PlantCobwebs = 57,
    PlantCobwebs2 = 58,
    // Effect-only plants: render emits leaves and ambient calls, not sprites.
    PlantLeafEmitter = 60,
    PlantBirdAndOwlEmitter = 61,
    PlantBird2Emitter = 62,
    PlantBird3Emitter = 63,
};

// The serializer removes unsupported plants; the light pass tests bit 0. Individual flags are bit
// indices; use (1 << flag) to form a mask.
enum PlantFlag {
    PlantFlagEmissive = 0,
    PlantFlagNeedsSupport = 1,
    PlantFlagSway = 2
};

// A fixed item stays where the generator places it instead of settling on terrain. Individual
// flags are bit indices; use (1 << flag) to form a mask.
enum ZoneItemFlag {
    ZoneItemFlagFixed = 0,
    ZoneItemFlagGeneratedMaterial = 1
};

// Generation records emitted for entrances, guards, resource and creature groups.
enum GenerationMarkerKind {
    GenerationMarkerNone = 0,
    GenerationMarkerEntrance = 5,
    GenerationMarkerGuard = 6,
    GenerationMarkerItemGroup = 9,
    GenerationMarkerCreatureGroup = 10
};

class Zone {
public:
    // A saved block change; loading verifies terrain support.
    struct Plant {
        Plant()
            : color(1.0f, 1.0f, 1.0f),
              flags((1 << PlantFlagNeedsSupport))
        {}

        // The zone generator's plants : 0..4 and 9..12 on land, 5..7 under water, 22 reeds, 27/28 rare
        // plants, 31/32 floating on water.
        int type; // index into GameController's plant sprites at
        plasma::Vector<3, Fixed> position;
        float scale;
        float rotation;                 // degrees about z
        float daylight;                 // cubeLight, 0..255
        plasma::Vector<3, float> color; // Initially white; light color of a lit plant.
        // Bit zero emits light, bit one requires terrain support and starts set, bit two identifies reeds
        // and some plants.
        unsigned int flags;
        // Padding excluded from copies.
    };
    // Shared object state, including the occupant creature identifier.
    struct ObjectState {
        ObjectState()
            : active(StaticObjectActive),
              transitionTime(0),
              unused38(0),
              occupantId(0)
        {}

        int type;
        plasma::Vector<3, Fixed> position;
        int rotation;
        plasma::Vector<3, float> size;
        // State one enables door collision; state two spills the inventory of a stocked chest.
        unsigned char active;
        // Milliseconds since active last changed (World::update counts it up; setObjectActive reverses a
        // transition in progress; render moves doors, gates, traps and lids by it).
        int transitionTime;
        int unused38; // zeroed by the constructor, copied with the state; nothing reads it
        long long occupantId;
    };
    // A placed object combines shared state, inventory and four integers.
    struct Object : ObjectState {
        Object()
            : linkedObjectIndex(-1),
              unused17c(-1),
              unused180(-1),
              unused184(0)
        {}

        Inventory inventory;
        int linkedObjectIndex; // Lever or rune-stone target index.
        // Unused integers initialized to (-1, -1, 0) and retained by copies.
        int unused17c;
        int unused180;
        int unused184;
    };

    // State describing an airship near a player. The send thread collects copies without transmitting them.
    struct AirshipState {
        // Everything zero, state 1.
        AirshipState()
            : id(0),
              unused08(0),
              position(Fixed(0), Fixed(0), Fixed(0)),
              velocity(0, 0, 0),
              rotation(0),
              home(Fixed(0), Fixed(0), Fixed(0)),
              heading(0),
              destination(Fixed(0), Fixed(0), Fixed(0)),
              state(AirshipReturnHome)
        {}

        long long id;                      // the World map's key
        unsigned char unused08;            // zeroed by the constructor, copied with the state; nothing reads it
        plasma::Vector<3, Fixed> position; // Compared with the player position.
        // Airship motion state: circle out, fly home, circle, then fly to the destination, with velocity,
        // heading and two target places.
        plasma::Vector<3, float> velocity; // Velocity in blocks per second.
        // Copied into currentRotation when the world adopts the airship state.
        float rotation;
        plasma::Vector<3, Fixed> home;        // states 0..2 circle it
        float heading;                        // degrees
        plasma::Vector<3, Fixed> destination; // state 3 flies there
        unsigned char state;
    };
    // An airship with current and target pose and its rider set. Collision checks use the transformed airship sprite.
    struct Airship : AirshipState {
        Airship()
            : currentPosition(Fixed(0), Fixed(0), Fixed(0)),
              currentRotation(0),
              unused9c(0)
        {}

        // The ids of the creatures riding it: World::update carries them along.
        std::set<long long> riders;
        // The adopted state initializes current position and rotation; update interpolates the pose and carries riders with it.
        plasma::Vector<3, Fixed> currentPosition;
        float currentRotation; // degrees about z
        int unused9c;          // zeroed by the constructor; nothing reads it
    };
    // An item lying on the ground in a zone.
    struct ZoneItem {
        ZoneItem()
            : scale(1.0f / 14.0f),
              flags(0),
              dropTime(0),
              hiddenTimeRemaining(0),
              dropDay(-1)
        {}

        bool operator==(const ZoneItem &other) const
        {
            return item == other.item && position == other.position && rotation == other.rotation &&
                   scale == other.scale && flags == other.flags && dropTime == other.dropTime &&
                   hiddenTimeRemaining == other.hiddenTimeRemaining;
        }

        Item item;
        plasma::Vector<3, Fixed> position;
        float rotation;
        float scale;         // initially 1/14
        unsigned char flags; // 1 << ZoneItemFlagGeneratedMaterial for generated materials
        int dropTime;
        int hiddenTimeRemaining;
        int dropDay;
    };
    // An item-bearing generation record with world position.
    struct GenerationMarker {
        GenerationMarker();
        // Initializes the record.
        GenerationMarker(const GenerationMarker &other)
            : kind(other.kind),
              item(other.item),
              spawnIndex(other.spawnIndex),
              race(other.race),
              position(other.position)
        {}

        int kind;
        Item item;
        int spawnIndex; // -1 initially
        int race;       // -1 initially
        plasma::Vector<3, Fixed> position;
    };
    // A changed block and its world coordinates.
    struct CubeChange {
        CubeChange()
            : day(0)
        {}

        plasma::Vector<3, int> position;
        Block block;
        int day;
    };
    // A zone-owned object containing two wide-string-keyed trees; the second tree also owns string values.
    struct Record94 {
        std::set<std::wstring> unused00;
        std::map<std::wstring, std::wstring> unused08;
    };
    // An unexamined cell-list record.
    struct CellRecord {
        CellRecord(Object *object, plasma::Vector<3, int> id)
            : object(object),
              id(id)
        {}
        Object *object;            // placed object in this cell
        plasma::Vector<3, int> id; // zone x/y and object index
    };

    Zone();
    virtual ~Zone();

    std::list<Plant> m_plants;
    std::vector<Object> m_objects;
    // Owned spawns created by the zone generator.
    std::vector<Spawn *> m_spawns;
    std::vector<Airship *> m_airships; // owned
    std::vector<ZoneItem> m_items;
    // Player-placed buildings, sent in BuildingListPacket updates.
    struct Building {
        int model;                       // World::lookup's index
        plasma::Vector<3, int> position; // the model's corner block
    };
    std::vector<Building> m_buildings;
    std::vector<GenerationMarker> m_generationMarkers;
    std::vector<int> unused54; // Only constructed and destroyed; intended element meaning is unknown.
    // Zone coordinates used by the save key zone<x>_<y>.
    plasma::Vector<2, int> m_position; // Initially (0, 0).
    // The zone is saved when this list is nonempty or m_modified is set.
    std::vector<CubeChange> m_blockChanges;
    bool m_needsServerUpdate; // client requests state until a reply arrives
    bool m_modified;          // false; World::saveZone saves a dirty zone when set
    bool m_objectsChanged;    // An object update is queued.
    // Region-cell level, quality and generation kind copied into this zone.
    Region::Cell cell;
    // Owned houses created by the house generator.
    std::vector<House *> m_houses;
    std::vector<Record94 *> unused94; // owned; nothing fills or reads it, the destructor deletes its elements
    int m_baseSpawnCount;             // 0
    int m_populationDays;             // Days accumulated by populateZone.
    // The 256-by-256 block columns.
    Field *m_fields;
    // One object list per eight-by-eight-block cell, indexed by (y % 32) * 32 + x % 32.
    std::list<CellRecord> *m_cells;
    CRITICAL_SECTION m_lock;
};

// A zone plant or decoration, also copied into chunk plant lists during meshing.
typedef Zone::Plant Plant;

}
