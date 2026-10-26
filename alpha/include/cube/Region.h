// Region owns a 64-by-64 grid of zones and their map cells, region records and bookkeeping. World saves its zones before deleting the region.
#pragma once
#include <set>
#include "plasma/Vector.h"
#include "cube/Fixed.h"
#include "cube/MissionState.h"
#include "cube/MonsterState.h"

namespace cube {

class Zone;
struct Fixed;

struct Region {
    // One map cell per zone, indexed by (x % 64) * 64 + y % 64. Its color bytes begin at zero.
    struct Cell {
        // World::World's structure dictionary; kind 3 invokes generateDungeon.
        enum StructureKind {
            StructureNone = 0,
            StructureQuarter = 1,
            StructureDungeon = 3,
            StructurePortal = 4,
            StructurePalace = 5
        };
        enum SettlementDistrict {
            DistrictResidential = 0,
            DistrictTrade = 1,
            DistrictCrafting = 2,
            DistrictClass = 3,
            DistrictPet = 4
        };
        Cell()
            : kind(StructureNone),
              style(0),
              unused02(0),
              unused03(0),
              seed(0),
              level(1),
              rarity(false)
        {}

        // Tested before the zone is used.
        unsigned char kind;
        unsigned char style;
        // Unused bytes, initially zero, copied and saved with the region map.
        unsigned char unused02;
        unsigned char unused03;
        int seed;
        // Initially one; compared to the creature level with a tolerance of two.
        int level;
        unsigned char rarity; // generated quality (0..4)
    };
    // One record per eight-by-eight zone square. Construction initializes terrain, mission and monster state, with a default level of one.
    struct Record {
        // World::World's settlement dictionary; generateZone and Speech consume these same keys.
        enum SceneryKind {
            SceneryNone = 0,
            SceneryCity = 1,
            SceneryMountain = 2,
            SceneryForest = 3,
            SceneryLake = 4,
            SceneryRuins = 5,
            SceneryCanyon = 6,
            SceneryValley = 7,
            SceneryCrater = 8,
            SceneryCave = 9,
            SceneryPortal = 10,
            SceneryRock = 11,
            SceneryTree = 12,
            SceneryPeak = 13,
            SceneryCastle = 14,
            SceneryIsland = 15
        };
        // Subtype keys of SceneryCastle; also generateDungeon's variant.
        enum DungeonStyle {
            DungeonCastle = 0,
            DungeonRuins = 1,
            DungeonCatacombs = 2,
            DungeonPalace = 3,
            DungeonTemple = 4,
            DungeonPyramid = 5
        };
        // The squared falloff at a block column, 0 outside the shape.
        float influence(const Fixed &x, const Fixed &y);
        // Squared distance from the centre in radii (noisy outline for most kinds).
        float shapeDistance(const Fixed &x, const Fixed &y);
        Record()
            : center(plasma::Vector<2, int>(0, 0)),
              radius(0),
              height(0),
              kind(SceneryNone),
              style(0),
              seed(0),
              level(1),
              rarity(0)
        {}

        // The record's centre, in Fixed block coordinates. 65536. The map saves the whole sixteen-byte
        // Item.
        plasma::Vector<2, Fixed> center;
        float radius; // radius, in blocks
        float height; // terrain height at the centre
        int kind;
        int style;
        int seed;
        int level; // 1
        int rarity;
        MissionState mission;
        MonsterState monster;
    };

    Region();
    virtual ~Region();

    // Implicit alignment gap.
    bool m_needsServerUpdate; // true
    int m_level;              // 1
    int m_rarity;             // 0
    int m_style;              // 0
    Cell m_cells[0x1000];
    Zone *m_zones[64][64]; // owned; null where not loaded
    Record m_records[64];
    bool m_populated; // Initially false.
    // An unused set of block positions; the intended key meaning is unknown.
    std::set<plasma::Vector<3, int>> unused15a1c; // only the constructor and destructor touch it
};

}
