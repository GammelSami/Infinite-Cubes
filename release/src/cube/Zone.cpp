#include <algorithm>
#include "cube/Zone.h"
#include "cube/MathConstants.h"

#include "cube/CaveBuilder.h"
#include "cube/Fixed.h"
#include "cube/Item.h"
#include "cube/LandmarkBuilder.h"
#include "cube/Missions.h"
#include "cube/Noise.h"
#include "cube/PlantBuilder.h"
#include "cube/Realm.h"
#include "cube/SpawnTable.h"
#include "cube/StructureBuilder.h"
#include "cube/World.h"
#include "cube/WorldLayout.h"

#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <algorithm>
#include <iostream>
#include "cube/CreatureIds.h"
#include "cube/BlockIds.h"
#include "cube/ItemIds.h"
#include "cube/MissionIds.h"
#include "cube/WorldIds.h"
#include "cube/ZoneIds.h"

namespace cube {

namespace {
// Camp mats are beach towels below this height (on a beach).
const int BeachTowelMaxHeight = 25;
// Dungeon residents are scaled down to this height at most.
const float MaxDungeonResidentHeight = 6.0f;
}

// fastRandom as the generator's own code has it expanded in place (signed arithmetic throughout).
static inline int fastRandomInline(int x, int y)
{
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return 100000 - ((n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff) / 21474;
}

Zone::Zone(World *world, const plasma::Vector<2, int> &position)
    : m_world(world),
      m_position(position),
      m_awaitingData(true),
      m_itemsChanged(false),
      m_staticEntitiesChanged(false),
      m_unused448(false),
      m_defeatedMissionCreatureCount(0),
      m_lastUpdateDay(-1)
{}

Zone::~Zone()
{
    for (std::pair<const long long, Resident *> &entry : m_residents) {
        delete entry.second;
    }
}

bool StaticEntity::operator==(const StaticEntity &other) const
{
    return type == other.type && position == other.position && orientation == other.orientation &&
           scale == other.scale && active == other.active && transitionTime == other.transitionTime &&
           occupantId == other.occupantId && item == other.item && itemCount == other.itemCount;
}

void Zone::addRoomEntities(int cellX, int cellY, int floorZ, int type, int storey, int placeType, int plot)
{
    // Uses signed left shifts for negative fixed-point coordinates.
    if (storey >= 0 && (buildingKind(type) == BuildingKindStonePavilion || type == BuildingHouseCourtyard ||
                        type == BuildingSewerOrGrottoEntrance || type == BuildingTempleEntrance))
        return;
    RoomFurnishing furnishing = RoomFurnishing_None;
    if (storey >= 0) {
        if ((type == BuildingCryptEntrance || type == BuildingLoreCrypt || type == BuildingCastleGate ||
             type == BuildingShrineEntrance || type == BuildingMineEntrance || type == BuildingVaultEntrance ||
             type == BuildingOldHutEntrance || type == BuildingGuildHallCentre || type == BuildingFlightMasterSite ||
             type == BuildingHouseEntrance || type == BuildingSandstoneTreasureChamber) &&
            ((plot >= PlotInn && plot <= PlotIdentifiersShop) || plot == PlotGuildHall)) {
            furnishing = RoomFurnishing_Shop;
        } else if (plot == PlotSmithy || plot == PlotCarpentersShop || plot == PlotTailorsShop || plot == PlotNone) {
        } else if (plot == PlotInn) {
            furnishing = storey != 0 ? RoomFurnishing_Bed : RoomFurnishing_Table;
        } else if (plot == PlotGuildHall) {
            furnishing = rand() % 2 != 0 ? RoomFurnishing_Table : RoomFurnishing_None;
        } else {
            std::vector<plasma::Vector<3, int>> groundStoreys;
            std::vector<plasma::Vector<3, int>> upperStoreys;
            int firstX = floorDivide(cellX, CellsPerPlot) * CellsPerPlot;
            int firstY = floorDivide(cellY, CellsPerPlot) * CellsPerPlot;
            for (int x = 0; x < CellsPerPlot; ++x) {
                for (int y = 0; y < CellsPerPlot; ++y) {
                    int count = buildingStoreys(m_world->zoneBuildingType(firstX + x, firstY + y));
                    for (int level = 0; level < count; ++level) {
                        if (m_world->isOccupiedStorey(firstX + x, firstY + y, level)) {
                            plasma::Vector<3, int> occupied(firstX + x, firstY + y, level);
                            (level == 0 ? groundStoreys : upperStoreys).push_back(occupied);
                        }
                    }
                }
            }
            plasma::Vector<3, int> selected(NoCoordinate, NoCoordinate, NoCoordinate);
            if (upperStoreys.empty()) {
                if (!groundStoreys.empty())
                    selected = groundStoreys[rand() % groundStoreys.size()];
            } else {
                selected = upperStoreys[rand() % upperStoreys.size()];
            }
            if (plasma::Vector<3, int>(cellX, cellY, storey) == selected)
                furnishing = RoomFurnishing_Bed;
            else if (rand() % 4 == 0)
                furnishing = RoomFurnishing_Table;
        }
    }

    StaticEntity entity;
    // In the other rooms a bed takes one occupied storey of the 3 x 3-cell plot, a table one room in four.
    switch (furnishing) {
    case RoomFurnishing_Bed:
        entity.type = StaticEntityBed;
        entity.position = plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + BlocksPerCell / 2),
                                                   Fixed(cellY * BlocksPerCell + BlocksPerCell / 2), Fixed(floorZ));
        entity.position[0] = entity.position[0] + (rand() % 5 - 2);
        entity.position[1] = entity.position[1] + (rand() % 5 - 2);
        entity.orientation = rand() % 4;
        if (putOnGround(entity, true))
            addStaticEntity(entity);
        entity.type = StaticEntityBedtable;
        switch (entity.orientation) {
        case QuarterTurn_Three:
            entity.position[0] = entity.position[0] + 1;
            entity.position[1] += Fixed(rand() % 2 != 0 ? -1.75 : 1.75);
            break;
        case QuarterTurn_Two:
            entity.position[0] += Fixed(rand() % 2 != 0 ? -1.75 : 1.75);
            entity.position[1] = entity.position[1] - 1;
            break;
        case QuarterTurn_One:
            entity.position[0] = entity.position[0] - 1;
            entity.position[1] += Fixed(rand() % 2 != 0 ? -1.75 : 1.75);
            break;
        case QuarterTurn_Zero:
            entity.position[0] += Fixed(rand() % 2 != 0 ? -1.75 : 1.75);
            entity.position[1] = entity.position[1] + 1;
            break;
        }
        if (putOnGround(entity, true))
            addStaticEntity(entity);
        break;
    case RoomFurnishing_Table:
        entity.type = StaticEntityTable;
        entity.position =
            plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 8), Fixed(cellY * BlocksPerCell + 8), Fixed(floorZ));
        entity.position[0] = entity.position[0] + (rand() % 3 - 1);
        entity.position[1] = entity.position[1] + (rand() % 3 - 1);
        entity.orientation = rand() % 4;
        if (putOnGround(entity, true))
            addStaticEntity(entity);
        if (rand() % 4 != 0) {
            StaticEntity chair;
            chair.type = StaticEntityStool;
            chair.position = entity.position;
            chair.position[1] = chair.position[1] - 2.5;
            chair.orientation = QuarterTurn_Two;
            if (putOnGround(chair, true))
                addStaticEntity(chair);
        }
        if (rand() % 4 != 0) {
            StaticEntity chair;
            chair.type = StaticEntityStool;
            chair.position = entity.position;
            chair.position[1] += Fixed(2.5);
            chair.orientation = QuarterTurn_Zero;
            if (putOnGround(chair, true))
                addStaticEntity(chair);
        }
        if (rand() % 4 != 0) {
            StaticEntity chair;
            chair.type = StaticEntityStool;
            chair.position = entity.position;
            chair.position[0] = chair.position[0] - 2.5;
            chair.orientation = QuarterTurn_One;
            if (putOnGround(chair, true))
                addStaticEntity(chair);
        }
        if (rand() % 4 != 0) {
            StaticEntity chair;
            chair.type = StaticEntityStool;
            chair.position = entity.position;
            chair.position[0] += Fixed(2.5);
            chair.orientation = QuarterTurn_Three;
            if (putOnGround(chair, true))
                addStaticEntity(chair);
        }
        break;
    case RoomFurnishing_Shop:
        entity.type = StaticEntityCounter;
        entity.position =
            plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 8), Fixed(cellY * BlocksPerCell + 5), Fixed(floorZ));
        entity.orientation = QuarterTurn_Two;
        if (putOnGround(entity, true))
            addStaticEntity(entity);
        entity.position = plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 8),
                                                   Fixed((cellY + 1) * BlocksPerCell - 5), Fixed(floorZ));
        entity.orientation = QuarterTurn_Zero;
        if (putOnGround(entity, true))
            addStaticEntity(entity);
        entity.position =
            plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 5), Fixed(cellY * BlocksPerCell + 8), Fixed(floorZ));
        entity.orientation = QuarterTurn_One;
        if (putOnGround(entity, true))
            addStaticEntity(entity);
        entity.position = plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 11), Fixed(cellY * BlocksPerCell + 8),
                                                   Fixed(floorZ));
        entity.orientation = QuarterTurn_Three;
        if (putOnGround(entity, true))
            addStaticEntity(entity);
        break;
    }

    // Wall furniture: up to three pieces per wall, spaced three blocks (two when not walled), each for one site in
    // three (a shelf one of three kinds), 1.5 blocks from the wall, moved inwards up to four blocks to an open block
    // and kept only with five blocks of wall behind it.
    std::vector<int> furnitureTypes = {StaticEntityStool,       StaticEntityShelf1, StaticEntityCupboard,
                                       StaticEntityFlowerVase1, StaticEntitySack,   StaticEntityCrate,
                                       StaticEntityOpenCrate,   StaticEntityBarrel, StaticEntityBench,
                                       StaticEntityBenchBlack};
    bool walls = hasWalls(type);
    int spacing = 3;
    if (storey < 0) {
        walls = true;
        switch (placeType) {
        case PlaceCatacombs:
            furnitureTypes = {StaticEntityStoneShelf1, StaticEntityStool, StaticEntityStoneStool, StaticEntityLegIrons};
            break;
        case PlaceShrine:
        case PlacePyramid:
        case PlaceTemple:
            furnitureTypes = {StaticEntitySandstoneShelf1, StaticEntityStoneStool, StaticEntityFlowerVase1};
            break;
        case PlaceVault:
            furnitureTypes = {StaticEntityCastleShelf1, StaticEntityStoneStool, StaticEntityBenchBlack,
                              StaticEntityLegIrons, StaticEntityTortureRack};
            break;
        case PlaceSewers:
            furnitureTypes = {StaticEntityStool, StaticEntityLegIrons,  StaticEntityBenchBlack,
                              StaticEntityCrate, StaticEntityOpenCrate, StaticEntityBarrel};
            break;
        default:
            furnitureTypes = {StaticEntityCastleShelf1, StaticEntityStool,     StaticEntityBarrel,  StaticEntityCrate,
                              StaticEntitySack,         StaticEntityOpenCrate, StaticEntityLegIrons};
            break;
        }
    } else {
        if (placeType == PlaceCastle)
            furnitureTypes = {StaticEntityCastleShelf1, StaticEntityStool, StaticEntityBenchBlack, StaticEntityLegIrons,
                              StaticEntityFlowerVase1};
        else if (buildingKind(type) == BuildingKindCrypt)
            furnitureTypes = {StaticEntityStoneShelf1, StaticEntityStool, StaticEntityStoneStool,
                              StaticEntityBenchBlack};
        if (!walls) {
            furnitureTypes = {StaticEntityFlowerVase1};
            spacing = 5;
        }
    }
    if (furnitureTypes.empty())
        return;

    // Negative y wall.
    for (int site = -1, offset = -spacing; site <= 1; ++site, offset += spacing) {
        if (rand() % 3 != 0 || (!walls && site == 0))
            continue;
        Fixed x = Fixed(cellX * BlocksPerCell + 8 + offset);
        Fixed y = (Fixed(cellY * BlocksPerCell) + 1.5);
        if (!walls)
            y = (Fixed(y) - 1);
        Fixed z = Fixed(floorZ);
        for (int depth = 0; depth < 4; ++depth) {
            if (cubeAt(x, y, z).isOpen())
                break;
            y = (Fixed(y) + 1);
        }
        StaticEntity decoration;
        decoration.type = furnitureTypes[rand() % furnitureTypes.size()];
        y += Fixed((double)decoration.scaledSize()[1] * 0.5);
        if (!cubeAt(x, y, z).isOpen())
            continue;
        bool backed = true;
        Fixed behind = (Fixed(y) - 1.5);
        for (int height = 0; height < 5; ++height) {
            for (int across = -1; across <= 1; ++across) {
                if (cubeAt((Fixed(x) + across), behind, (Fixed(z) + height)).isOpen()) {
                    backed = false;
                    break;
                }
            }
            if (!backed)
                break;
        }
        if (!backed && walls)
            continue;
        decoration.orientation = QuarterTurn_Two;
        if (decoration.type == StaticEntityShelf1 || decoration.type == StaticEntityCastleShelf1 ||
            decoration.type == StaticEntityStoneShelf1 || decoration.type == StaticEntitySandstoneShelf1)
            decoration.type += rand() % 3;
        else if (decoration.type == StaticEntityFlowerVase1)
            decoration.type += rand() % (StaticEntityFlowerVase9 - StaticEntityFlowerVase1 + 1);
        decoration.position = plasma::Vector<3, Fixed>(x, y, z);
        if (putOnGround(decoration, true))
            addStaticEntity(decoration);
    }

    // Positive y wall.
    for (int site = -1, offset = -spacing; site <= 1; ++site, offset += spacing) {
        if (rand() % 3 != 0 || (!walls && site == 0))
            continue;
        Fixed x = Fixed(cellX * BlocksPerCell + 8 + offset);
        Fixed y = (Fixed(Fixed((cellY + 1) * 16)) - 1.5);
        if (!walls)
            y = (Fixed(y) + 1);
        Fixed z = Fixed(floorZ);
        for (int depth = 0; depth < 4; ++depth) {
            if (cubeAt(x, y, z).isOpen())
                break;
            y = (Fixed(y) - 1);
        }
        StaticEntity decoration;
        decoration.type = furnitureTypes[rand() % furnitureTypes.size()];
        y -= Fixed((double)decoration.scaledSize()[1] * 0.5);
        if (!cubeAt(x, y, z).isOpen())
            continue;
        bool backed = true;
        Fixed behind = (Fixed(y) - -1.5);
        for (int height = 0; height < 5; ++height) {
            for (int across = -1; across <= 1; ++across) {
                if (cubeAt((Fixed(x) + across), behind, (Fixed(z) + height)).isOpen()) {
                    backed = false;
                    break;
                }
            }
            if (!backed)
                break;
        }
        if (!backed && walls)
            continue;
        decoration.orientation = QuarterTurn_Zero;
        if (decoration.type == StaticEntityShelf1 || decoration.type == StaticEntityCastleShelf1 ||
            decoration.type == StaticEntityStoneShelf1 || decoration.type == StaticEntitySandstoneShelf1)
            decoration.type += rand() % 3;
        else if (decoration.type == StaticEntityFlowerVase1)
            decoration.type += rand() % 9;
        decoration.position = plasma::Vector<3, Fixed>(x, y, z);
        if (putOnGround(decoration, true))
            addStaticEntity(decoration);
    }

    // Negative x wall.
    for (int site = -1, offset = -spacing; site <= 1; ++site, offset += spacing) {
        if (rand() % 3 != 0 || (!walls && site == 0))
            continue;
        Fixed y = Fixed(cellY * BlocksPerCell + 8 + offset);
        Fixed x = (Fixed(cellX * BlocksPerCell) + 1.5);
        if (!walls)
            x = (Fixed(x) - 1);
        Fixed z = Fixed(floorZ);
        for (int depth = 0; depth < 4; ++depth) {
            if (cubeAt(x, y, z).isOpen())
                break;
            x = (Fixed(x) + 1);
        }
        StaticEntity decoration;
        decoration.type = furnitureTypes[rand() % furnitureTypes.size()];
        x += Fixed((double)decoration.scaledSize()[1] * 0.5);
        if (!cubeAt(x, y, z).isOpen())
            continue;
        bool backed = true;
        Fixed behind = (Fixed(x) - 1.5);
        for (int height = 0; height < 5; ++height) {
            for (int across = -1; across <= 1; ++across) {
                if (cubeAt(behind, (Fixed(y) + across), (Fixed(z) + height)).isOpen()) {
                    backed = false;
                    break;
                }
            }
            if (!backed)
                break;
        }
        if (!backed && walls)
            continue;
        decoration.orientation = QuarterTurn_One;
        if (decoration.type == StaticEntityShelf1 || decoration.type == StaticEntityCastleShelf1 ||
            decoration.type == StaticEntityStoneShelf1 || decoration.type == StaticEntitySandstoneShelf1)
            decoration.type += rand() % 3;
        else if (decoration.type == StaticEntityFlowerVase1)
            decoration.type += rand() % 9;
        decoration.position = plasma::Vector<3, Fixed>(x, y, z);
        if (putOnGround(decoration, true))
            addStaticEntity(decoration);
    }

    // Positive x wall.
    for (int site = -1, offset = -spacing; site <= 1; ++site, offset += spacing) {
        if (rand() % 3 != 0 || (!walls && site == 0))
            continue;
        Fixed y = Fixed(cellY * BlocksPerCell + 8 + offset);
        Fixed x = (Fixed((Fixed(cellX * BlocksPerCell)) + (Fixed(16))) - 1.5);
        if (!walls)
            x = (Fixed(x) + 1);
        Fixed z = Fixed(floorZ);
        for (int depth = 0; depth < 4; ++depth) {
            if (cubeAt(x, y, z).isOpen())
                break;
            x = (Fixed(x) - 1);
        }
        StaticEntity decoration;
        decoration.type = furnitureTypes[rand() % furnitureTypes.size()];
        x -= Fixed((double)decoration.scaledSize()[1] * 0.5);
        if (!cubeAt(x, y, z).isOpen())
            continue;
        bool backed = true;
        Fixed behind = (Fixed(x) - -1.5);
        for (int height = 0; height < 5; ++height) {
            for (int across = -1; across <= 1; ++across) {
                if (cubeAt(behind, (Fixed(y) + across), (Fixed(z) + height)).isOpen()) {
                    backed = false;
                    break;
                }
            }
            if (!backed)
                break;
        }
        if (!backed && walls)
            continue;
        decoration.orientation = QuarterTurn_Three;
        if (decoration.type == StaticEntityShelf1 || decoration.type == StaticEntityCastleShelf1 ||
            decoration.type == StaticEntityStoneShelf1 || decoration.type == StaticEntitySandstoneShelf1)
            decoration.type += rand() % 3;
        else if (decoration.type == StaticEntityFlowerVase1)
            decoration.type += rand() % 9;
        decoration.position = plasma::Vector<3, Fixed>(x, y, z);
        if (putOnGround(decoration, true))
            addStaticEntity(decoration);
    }
}

plasma::Vector<3, Fixed> Zone::choosePosition(int cellX, int cellY, int kind)
{
    std::map<PositionKey, std::vector<plasma::Vector<3, Fixed>>>::iterator it =
        m_positionsByKind.find(PositionKey {{cellX, cellY, kind}});
    if (it != m_positionsByKind.end()) {
        float nearest = -1.0f;
        plasma::Vector<3, Fixed> result;
        for (plasma::Vector<3, Fixed> &position : it->second) {
            float dx = (float)((Fixed(cellX * BlocksPerCell + BlocksPerCell / 2)) - position[0]);
            float dy = (float)((Fixed(cellY * BlocksPerCell + BlocksPerCell / 2)) - position[1]);
            float squaredDistance = dy * dy + dx * dx;
            if (nearest < 0.0f || squaredDistance < nearest) {
                nearest = squaredDistance;
                result = position;
            }
        }
        if (nearest >= 0.0f) {
            return result;
        }
    }
    return randomPosition(cellX, cellY, kind);
}

Block Zone::cubeAt(Fixed x, Fixed y, Fixed z)
{
    return getBlock(x.toBlock(), y.toBlock(), z.toBlock());
}

// The depth of a dungeon cell's cellar by its kind: a deep pit 40 blocks, a water pool 20, a lava or muck pool 8.
int dungeonCellarDepth(const StructureCell &cell)
{
    if (cell.kind == StructureCell_DeepPit) {
        return 40;
    }
    if (cell.kind == StructureCell_WaterPool) {
        return 20;
    }
    if (cell.kind == StructureCell_LavaPool) {
        return 8;
    }
    if (cell.kind == StructureCell_MuckPool) {
        return 8;
    }
    return 0;
}

// The dungeons' trim colours by style and place (a hand-authored palette; components above 255 brighten).
plasma::Vector<3, float> dungeonTrimColor(StructureCell cell, int dungeonType)
{
    if (cell.style == DungeonCellStyle_Overgrown) {
        return plasma::Vector<3, float>(255.0f, 150.0f, 100.0f);
    }
    switch (dungeonType) {
    case PlaceCatacombs:
        return plasma::Vector<3, float>(60.0f, 180.0f, 160.0f);
    case PlaceSewers:
        return plasma::Vector<3, float>(100.0f, 180.0f, 160.0f);
    case PlaceShrine:
        return plasma::Vector<3, float>(180.0f, 200.0f, 260.0f);
    case PlaceGrotto:
        return plasma::Vector<3, float>(50.0f, 160.0f, 40.0f);
    case PlacePyramid:
        return plasma::Vector<3, float>(270.0f, 200.0f, 100.0f);
    case PlaceTemple:
        return plasma::Vector<3, float>(200.0f, 230.0f, 100.0f);
    case PlaceVault:
        return plasma::Vector<3, float>(130.0f, 160.0f, 160.0f);
    default:
        return plasma::Vector<3, float>(129.0f, 255.0f, 255.0f);
    }
}

// The dungeons' wall colours by style and place (a hand-authored palette; components above 255 brighten).
plasma::Vector<3, float> dungeonWallColor(StructureCell cell, int dungeonType)
{
    if (cell.style == DungeonCellStyle_Overgrown) {
        return plasma::Vector<3, float>(255.0f, 255.0f, 30.0f);
    }
    switch (dungeonType) {
    case PlaceCatacombs:
        return plasma::Vector<3, float>(120.0f, 100.0f, 180.0f);
    case PlaceSewers:
        return plasma::Vector<3, float>(100.0f, 160.0f, 90.0f);
    case PlaceShrine:
        return plasma::Vector<3, float>(250.0f, 280.0f, 300.0f);
    case PlaceMines:
        return plasma::Vector<3, float>(250.0f, 200.0f, 80.0f);
    case PlaceGrotto:
        return plasma::Vector<3, float>(200.0f, 250.0f, 250.0f);
    case PlacePyramid:
        return plasma::Vector<3, float>(270.0f, 250.0f, 100.0f);
    case PlaceTemple:
        return plasma::Vector<3, float>(250.0f, 230.0f, 100.0f);
    case PlaceVault:
        return plasma::Vector<3, float>(130.0f, 100.0f, 120.0f);
    default:
        return plasma::Vector<3, float>(100.0f, 160.0f, 200.0f);
    }
}

unsigned int fastRandom(int x, int y)
{
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return 100000 - (unsigned int)((n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff) / 21474;
}

// A column the terrain loop marks for a vertical shaft: its block position (the first block above the raised ground)
// and the depth of the drop.
class Shaft {
public:
    plasma::Vector<3, int> position;
    int depth;
};

struct Landform {
    int type;
    plasma::Vector<2, int> position;
    plasma::Vector<2, int> size;
    int level;
};

void World::getPlace(int cellX, int cellY, AreaInfo &info)
{
    info.large = isLargePlace(cellX, cellY);
    info.type = locationType(cellX, cellY, info.large);
    if (info.type != PlaceNone) {
        info.radius = !info.large ? SmallPlaceRadius : LargePlaceRadius;
        info.center = placeCenter(cellX, cellY, info.large);
        info.environment = environmentType(cellX, cellY, info.type, info.large);
    } else {
        info.environment = EnvironmentTypeNone;
    }
}

void World::getPlaceInfluence(int x, int y, AreaInfo &info)
{
    if (info.type != PlaceNone) {
        info.squaredDistance = placeSquaredDistance(info.center, x, y);
        int radius = !info.large ? SmallPlaceRadius : LargePlaceRadius;
        float squaredRadius = (float)(radius * radius);
        float influence;
        if (info.squaredDistance >= squaredRadius) {
            influence = 0.0f;
        } else {
            float fraction = 1.0f - info.squaredDistance / squaredRadius;
            influence = fraction * fraction;
        }
        info.influence = influence;
    } else {
        info.squaredDistance = 0.0f;
        info.influence = 0.0f;
    }
}

// A hash of the cell with a salt of its own (66945), one in two.
bool halfChanceA(int x, int y)
{
    return (fastRandomInline(x + 66945, y) & 1) == 0;
}

// The same with another salt (1264824).
bool halfChanceB(int x, int y)
{
    return (fastRandomInline(x + 1264824, y) & 1) == 0;
}

bool hasFence(int type, int neighbour)
{
    if ((neighbour < BuildingPumpkinField || neighbour > BuildingGraveyard) && buildingStoreyHeight(neighbour) <= 0) {
        return true;
    }
    return false;
}

void Zone::generatePlants()
{
    // Uses signed left shifts for negative fixed-point coordinates.
    int placeX = floorDivide(m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(m_position[1] * BlocksPerZone, BlocksPerPlace);
    m_world->locationType(placeX, placeY, m_world->isLargePlace(placeX, placeY));
    plasma::Vector<2, int> center = m_world->placeCenter(placeX, placeY, m_world->isLargePlace(placeX, placeY));
    World::BiomeSample sample;
    m_world->biomeSample(center[0], center[1], sample);
    for (int x = m_position[0] * BlocksPerZone; x < (m_position[0] + 1) * BlocksPerZone; ++x) {
        Sleep(0);
        for (int y = m_position[1] * BlocksPerZone; y < m_position[1] * BlocksPerZone + BlocksPerZone; ++y) {
            Field *column = field(x, y);
            for (int index = 0; index < (int)column->m_blocks.size(); ++index) {
                Block ground = column->getBlock(index);
                if (ground.flags & BlockFlagNotTerrain)
                    continue;
                if (ground.isPassable())
                    continue;
                // The vegetation recipe (hand-tuned per plant type): two noises pick where plants grow (frequency
                // 0.05, offsets 9843, 8437) and which (0.01, offsets 9843, 8437 and 34234, 234234); the climate tests
                // (temperature 0.2, 0.8; humidity 0.1, 0.3, 0.5; undead 0.3; the valley 0.43), the odds and each
                // plant's scale range choose the species.
                Block above = column->getBlock(index + 1);
                if ((above.flags & BlockFlagDungeon) && ground.type != BlockGrass)
                    continue;
                int type = ground.type;
                if (type == BlockPoison || type == BlockWater)
                    continue;
                int z = index + 1 + column->m_baseZ;
                if (type == BlockAir && above.type == BlockCave && rand() % 20 == 0) {
                    Plant plant;
                    plant.position[0] = Fixed(x) + 0.5;
                    plant.position[1] = Fixed(y) + 0.5;
                    plant.position[2] = Fixed(z);
                    plant.rotation = (float)((rand() % 4) * 90);
                    plant.type = PlantLiana;
                    plant.scale = (float)rand() * 0.04f / (float)RAND_MAX + 0.06f;
                    plant.position[2] = plant.position[2] - plant.scale * 20.0f;
                    m_plants.push_back(plant);
                }
                if (index < (int)column->m_blocks.size() - 1 && column->getBlock(index + 1).isSolid())
                    continue;
                if (above.type == BlockAir && type == BlockWet) {
                    if (rand() % 2 != 0 && column->m_temperature > 0.2f) {
                        if ((float)fabs((double)noise(x * 0.05 + 9843.0, y * 0.05 + 8437.0)) > 0.5f) {
                            Plant plant;
                            plant.position[0] = Fixed(x) + 0.5;
                            plant.position[1] = Fixed(y) + 0.5;
                            plant.position[2] = Fixed(z);
                            plant.rotation = (float)rand() * 360.0f / (float)RAND_MAX;
                            plant.scale = 0.09f;
                            plant.type = PlantReed;
                            plant.flags = PlantFlagSway;
                            m_plants.push_back(plant);
                        }
                        continue;
                    }
                } else if (type == BlockLava && x % 16 == 0 && y % 16 == 0 && rand() % 3 != 0) {
                    Plant plant;
                    plant.position[0] = Fixed(x) + 0.5;
                    plant.position[1] = Fixed(y) + 0.5;
                    plant.position[2] = Fixed(z);
                    plant.type = PlantWarmLight;
                    float color = (float)rand() * 0.5f / (float)RAND_MAX + 0.5f;
                    plant.color = plasma::Vector<3, float>(color, color * 0.5f, color * 0.2f);
                    // Copies scale without initializing it for type 76.
                    m_plants.push_back(plant);
                    continue;
                }
                if (type == BlockType_12) {
                    Plant plant;
                    plant.position[0] = Fixed(x) + 0.5;
                    plant.position[1] = Fixed(y) + 0.5;
                    plant.position[2] = Fixed(z);
                    plant.rotation = (float)rand() * 360.0f / (float)RAND_MAX;
                    plant.scale = 0.075f;
                    plant.flags = (PlantFlagNeedsSupport | PlantFlagSway);
                    if (noise(x * 0.01 + 9843.0, y * 0.01 + 8437.0) > 0.0f)
                        plant.type = noise((float)x * 0.01f + 34234.0f, (float)y * 0.01f + 234234.0f) > 0.0f
                                         ? PlantGrassDark1
                                         : PlantGrassDark2;
                    else
                        plant.type =
                            noise(x * 0.01 + 34234.0, y * 0.01 + 234234.0) > 0.5f ? PlantDarkPlant1 : PlantDarkPlant2;
                    m_plants.push_back(plant);
                    continue;
                }

                if (type != BlockGrass && type != BlockSand && type != BlockSnow && type != BlockRock &&
                    type != BlockCave && z > 0)
                    continue;
                if (!((float)fabs((double)noise(x * 0.05 + 9843.0, y * 0.05 + 8437.0)) > 0.6f) || rand() % 6 != 0)
                    continue;
                if (column->getBlock(index).isPassable())
                    continue;
                if (z <= 0 && !(column->getBlock(index).flags & BlockFlagProtected)) {
                    if (z >= -5)
                        continue;
                    Plant plant;
                    plant.position[0] = Fixed(x) + 0.5;
                    plant.position[1] = Fixed(y) + 0.5;
                    plant.position[2] = Fixed(z);
                    plant.rotation = (float)((rand() % 4) * 90);
                    float scale = 0.1f;
                    if (noise(x * 0.01 + 9843.0, y * 0.01 + 8437.0) > 0.0f)
                        plant.type = rand() % (PlantCoral - PlantUnderwaterPlant + 1) + PlantUnderwaterPlant;
                    else
                        plant.type = PlantAlga;
                    if (plant.type == PlantUnderwaterPlant) {
                        scale = 0.075f;
                        plant.flags = (PlantFlagNeedsSupport | PlantFlagSway);
                    } else if (plant.type == PlantAlga) {
                        scale = 0.1f;
                        plant.flags = (PlantFlagNeedsSupport | PlantFlagSway);
                    }
                    plant.scale = scale * ((float)rand() / (float)RAND_MAX + 1.0f);
                    m_plants.push_back(plant);
                    continue;
                }
                if (type == BlockRock) {
                    Plant plant;
                    plant.position[0] = Fixed(x) + 0.5;
                    plant.position[1] = Fixed(y) + 0.5;
                    plant.position[2] = Fixed(z);
                    plant.rotation = (float)((rand() % 4) * 90);
                    plant.scale = 0.075f;
                    plant.type = noise(x * 0.01 + 9843.0, y * 0.01 + 8437.0) > 0.0f ? PlantLavaGrass : PlantThornPlant;
                    if (plant.type == PlantLavaGrass)
                        plant.flags = (PlantFlagNeedsSupport | PlantFlagSway);
                    else if (plant.type == PlantThornPlant)
                        plant.scale = (float)((double)rand() * 0.02 / (double)RAND_MAX + 0.03);
                    m_plants.push_back(plant);
                    continue;
                }
                if (type == BlockSnow)
                    continue;
                if (column->m_undead > 0.3f) {
                    Plant plant;
                    plant.position[0] = Fixed(x) + 0.5;
                    plant.position[1] = Fixed(y) + 0.5;
                    plant.position[2] = Fixed(z);
                    plant.rotation = (float)rand() * 360.0f / (float)RAND_MAX;
                    plant.scale = 0.075f;
                    if (noise(x * 0.01 + 9843.0, y * 0.01 + 8437.0) > 0.0f)
                        plant.type =
                            noise(x * 0.01 + 34234.0, y * 0.01 + 234234.0) > 0.0f ? PlantGrassDark1 : PlantGrassDark2;
                    else
                        plant.type =
                            noise(x * 0.01 + 34234.0, y * 0.01 + 234234.0) > 0.5f ? PlantDarkPlant1 : PlantDarkPlant2;
                    if (plant.type != PlantDarkPlant1 && plant.type != PlantDarkPlant2)
                        plant.flags = (PlantFlagNeedsSupport | PlantFlagSway);
                    else
                        plant.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.05f;
                    if (type == BlockGrass || (plant.type == PlantGrassDark1 || plant.type == PlantGrassDark2))
                        m_plants.push_back(plant);
                    continue;
                }

                if (column->m_temperature >= 0.8f) {
                    if (column->m_humidity < 0.3f) {
                        Plant plant;
                        plant.position[0] = Fixed(x) + 0.5;
                        plant.position[1] = Fixed(y) + 0.5;
                        plant.position[2] = Fixed(z);
                        plant.rotation = (float)rand() * 360.0f / (float)RAND_MAX;
                        plant.scale = 0.075f;
                        if (noise(x * 0.01 + 9843.0, y * 0.01 + 8437.0) > 0.0f)
                            plant.type =
                                noise(x * 0.01 + 34234.0, y * 0.01 + 234234.0) > 0.0f ? PlantGrassDry1 : PlantGrassDry2;
                        else
                            plant.type = noise(x * 0.01 + 34234.0, y * 0.01 + 234234.0) > 0.5f ? PlantSavannahPlant1
                                                                                               : PlantSavannahPlant2;
                        plant.flags = (PlantFlagNeedsSupport | PlantFlagSway);
                        if (plant.type == PlantSavannahPlant1 || plant.type == PlantSavannahPlant2)
                            plant.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.05f;
                        if (plant.type == PlantLeaf) {
                            plant.flags = (PlantFlagNeedsSupport | PlantFlagSway);
                            plant.scale = (float)rand() * 0.02f / (float)RAND_MAX + 0.1f;
                        }
                        if (type == BlockGrass || (plant.type == PlantGrassDry1 || plant.type == PlantGrassDry2))
                            m_plants.push_back(plant);
                    } else if (column->m_humidity >= 0.1f) {
                        Plant plant;
                        plant.position[0] = Fixed(x) + 0.5;
                        plant.position[1] = Fixed(y) + 0.5;
                        plant.position[2] = Fixed(z);
                        plant.rotation = (float)rand() * 360.0f / (float)RAND_MAX;
                        plant.scale = 0.075f;
                        if (noise(x * 0.01 + 9843.0, y * 0.01 + 8437.0) > 0.0f)
                            plant.type =
                                noise(x * 0.01 + 34234.0, y * 0.01 + 234234.0) > 0.0f ? PlantGrass3 : PlantGrass2;
                        else
                            plant.type =
                                noise(x * 0.01 + 34234.0, y * 0.01 + 234234.0) > 0.5f ? PlantEchinacea2 : PlantLeaf;
                        plant.flags = (plant.type >= PlantGrass && plant.type <= PlantGrass3)
                                          ? (PlantFlagNeedsSupport | PlantFlagSway)
                                          : PlantFlagNeedsSupport;
                        if (plant.type == PlantEchinacea2) {
                            plant.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.05f;
                            plant.flags |= PlantFlagSway;
                        } else if (plant.type == PlantLeaf) {
                            plant.scale = (float)rand() * 0.02f / (float)RAND_MAX + 0.1f;
                            plant.flags |= PlantFlagSway;
                        }
                        if (type == BlockGrass || (plant.type == PlantGrass || plant.type == PlantGrass2))
                            m_plants.push_back(plant);
                    } else if (rand() % 100 == 0) {
                        Plant plant;
                        plant.position[0] = Fixed(x) + 0.5;
                        plant.position[1] = Fixed(y) + 0.5;
                        plant.position[2] = Fixed(z);
                        plant.rotation = (float)((rand() % 4) * 90);
                        plant.scale = 0.075f;
                        plant.type = rand() % 2 != 0 ? PlantDesertFlower01 : PlantDesertFlower02;
                        m_plants.push_back(plant);
                    }
                    continue;
                }

                Plant plant;
                plant.position[0] = Fixed(x) + 0.5;
                plant.position[1] = Fixed(y) + 0.5;
                plant.position[2] = Fixed(z);
                plant.rotation = (float)((rand() % 4) * 90);
                plant.scale = 0.075f;
                m_world->biomeSample(x, y, sample);
                World::AreaInfo info;
                m_world->fillAreaInfo(x, y, info);
                float mountains = sample.mountains();
                float wetness = sample.wetness();
                float undead = sample.undead(info.type, info.influence);
                float lava = sample.lavaWeight(info.type, info.influence);
                float streetAmount = m_world->streetAmount(x, y, info.type, info.influence);
                float valley = m_world->valleyWater(x, y, streetAmount, lava, undead, wetness, mountains, &info);
                if (valley > 0.43f) {
                    plant.rotation = (float)rand() * 360.0f / (float)RAND_MAX;
                    plant.type = PlantReed;
                    plant.scale = 0.09f;
                    plant.flags = PlantFlagSway;
                } else {
                    if (column->m_humidity > 0.5f) {
                        if (noise(x * 0.01 + 9843.0, y * 0.01 + 8437.0) > -0.1f)
                            plant.type =
                                noise(x * 0.01 + 34234.0, y * 0.01 + 234234.0) > 0.0f ? PlantLeaf : PlantGrass3;
                        else
                            plant.type = randomBloom(x, y);
                    } else {
                        if (noise(x * 0.01 + 9843.0, y * 0.01 + 8437.0) > -0.1f)
                            plant.type =
                                noise(x * 0.01 + 34234.0, y * 0.01 + 234234.0) > 0.0f ? PlantGrass : PlantGrass2;
                        else
                            plant.type = randomBloom(x, y);
                    }
                    if ((plant.type >= PlantFlowers4 && plant.type <= PlantGrass3) || plant.type == PlantLeaf) {
                        plant.flags = (PlantFlagNeedsSupport | PlantFlagSway);
                        if (plant.type == PlantLeaf)
                            plant.scale = (float)rand() * 0.02f / (float)RAND_MAX + 0.1f;
                    }
                }
                if (type == BlockGrass || (plant.type >= PlantGrass && plant.type <= PlantGrass3))
                    m_plants.push_back(plant);
            }
        }
    }
    m_lightPlants.clear();
    m_soundPlants.clear();
    for (const Plant &plant : m_plants) {
        if (plant.type == PlantWarmLight || (plant.flags & PlantFlagEmissive))
            m_lightPlants.push_back(plant);
        if ((plant.type >= PlantLeafCanopy && plant.type <= PlantBird3Sound) || plant.type == PlantRiverSound)
            m_soundPlants.push_back(plant);
    }
}

bool hasPosts(int type)
{
    return type >= BuildingLakePlatform && type <= BuildingMountainPlatform;
}

bool hasStoreys(int type)
{
    return buildingStoreyHeight(type) > 0;
}

bool hasStreetDoor(int type)
{
    return type == BuildingHouseEntrance || type == BuildingCryptEntrance || type == BuildingLoreCrypt ||
           type == BuildingCastleGate || type == BuildingShrineEntrance || type == BuildingMineEntrance ||
           type == BuildingVaultEntrance || type == BuildingOldHutEntrance || type == BuildingFlightMasterSite ||
           type == BuildingSandstoneTreasureChamber;
}

bool isField(int type)
{
    return type >= BuildingPumpkinField && type <= BuildingGraveyard;
}

bool isFree(const Block &block, bool allowSpiritPassage, bool checkBarriers, bool blockPassableBarriers)
{
    return !isSolid(block) && (!checkBarriers || !(block.flags & BlockFlagBarrier) ||
                               (!blockPassableBarriers && (block.flags & BlockFlagPassableBarrier)) ||
                               (allowSpiritPassage && (block.flags & BlockFlagSpiritPassable)));
}

bool isHouse(int type)
{
    return (buildingKind(type) == BuildingKindHouse && type != BuildingHouseCourtyard) ||
           buildingKind(type) == BuildingKindOldHut || buildingKind(type) == BuildingKindDungeonFortification ||
           buildingKind(type) == BuildingKindTimberMineEntrance || type == BuildingHousePavilion;
}

bool isNatural(const unsigned char &category)
{
    return category == ItemCategoryFormula || category == ItemCategoryWeapon || category == ItemCategoryChest ||
           category == ItemCategoryHands || category == ItemCategoryFeet || category == ItemCategoryShoulder ||
           category == ItemCategoryNeck || category == ItemCategoryRing || category == ItemCategoryLeftovers ||
           category == ItemCategoryQuest || category == ItemCategorySpecial || category == ItemCategoryEyePatch ||
           category == ItemCategoryGeneratedItem;
}

bool isOutsideZone(int x, int y, int radius, int zoneX, int zoneY)
{
    if (x + radius < zoneX * BlocksPerZone || y + radius < zoneY * BlocksPerZone ||
        x - radius >= zoneX * BlocksPerZone + BlocksPerZone || y - radius >= zoneY * BlocksPerZone + BlocksPerZone) {
        return true;
    }
    return false;
}

bool isProtectedVillage(const Missions &missions, int x, int y)
{
    return missions.m_village == plasma::Vector<2, int>(x, y);
}

bool isSolid(const Block &block)
{
    return block.isSolid();
}

bool Zone::isSpaceFree(int x, int y, int z, int step)
{
    for (int dx = -2; dx <= 2; dx++) {
        for (int dy = -2; dy <= 2; dy++) {
            int rise = (std::max)(abs(dx), abs(dy)) / step;
            if (isSolid(getBlock(x + dx, y + dy, z + rise))) {
                return false;
            }
            if (isSolid(getBlock(x + dx, y + dy, z + rise + 1))) {
                return false;
            }
            if (isSolid(getBlock(x + dx, y + dy, z + rise + 2))) {
                return false;
            }
            // Eight blocks of headroom above the step.
            for (int i = rise; i < 8; i++) {
                if (isSolid(getBlock(x + dx, y + dy, z + i))) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool Zone::isTerrain(const Block &block)
{
    if (block.type == BlockAir || (block.flags & BlockFlagNotTerrain)) {
        return false;
    }
    if (block.type == BlockWater || block.type == BlockWet || block.type == BlockTree || block.type == BlockLeaves ||
        block.type == BlockSnowLeaves || block.type == BlockLava || block.type == BlockPoison) {
        return false;
    }
    return true;
}

float lineFraction(const plasma::Vector<2, int> &a, const plasma::Vector<2, int> &b, const Fixed &x, const Fixed &y)
{
    plasma::Vector<2, float> direction(b - a);
    plasma::Vector<2, float> offset((float)(x - Fixed(a[0])), (float)(y - Fixed(a[1])));
    return direction.dot(offset) / direction.squaredLength();
}

bool Zone::putOnGround(StaticEntity &entity, bool needsSupport)
{
    int zoneX = m_position[0] * BlocksPerZone;
    int zoneY = m_position[1] * BlocksPerZone;
    if (entity.position[0] < zoneX || entity.position[0] >= Fixed(zoneX + BlocksPerZone) ||
        entity.position[1] < zoneY || entity.position[1] >= Fixed(zoneY + BlocksPerZone)) {
        return false;
    }
    plasma::Vector<3, float> size = entity.scaledSize();
    if (entity.orientation & 1) {
        std::swap(size[0], size[1]);
    }
    // The columns under the entity, a hundredth of a block inside its box.
    int x0 = (entity.position[0] - size[0] * 0.5f + 0.01f).toBlock();
    int x1 = (entity.position[0] + size[0] * 0.5f - 0.01f).toBlock();
    int y0 = (entity.position[1] - size[1] * 0.5f + 0.01f).toBlock();
    int y1 = (entity.position[1] + size[1] * 0.5f - 0.01f).toBlock();
    // Down while its layer is free, then up while something is in it, twenty blocks at most each way.
    for (int i = 0; i < 20; i++) {
        int z = (int)entity.position[2];
        bool grounded = false;
        for (int x = x0; x <= x1; x++) {
            for (int y = y0; y <= y1; y++) {
                if (getBlock(x, y, z).isSolid()) {
                    grounded = true;
                    break;
                }
            }
            if (grounded) {
                break;
            }
        }
        if (grounded) {
            break;
        }
        entity.position[2] -= Fixed(1);
    }
    for (int i = 0; i < 20; i++) {
        int z = (int)entity.position[2];
        bool blocked = false;
        for (int x = x0; x <= x1; x++) {
            for (int y = y0; y <= y1; y++) {
                if (getBlock(x, y, z).isSolid()) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) {
                break;
            }
        }
        if (!blocked) {
            break;
        }
        entity.position[2] += Fixed(1);
    }
    if (needsSupport) {
        int below = (int)(entity.position[2] - Fixed(1));
        for (int x = x0; x <= x1; x++) {
            for (int y = y0; y <= y1; y++) {
                if (!getBlock(x, y, below).isSolid()) {
                    return false;
                }
            }
        }
    }
    if (cubeAt(entity.position[0], entity.position[1], entity.position[2]).type == BlockWater) {
        return false;
    }
    switch (cubeAt(entity.position[0], entity.position[1], entity.position[2] - Fixed(1)).type) {
    case BlockWet:
    case BlockLava:
    case BlockPoison:
        return false;
    }
    entity.position[2] = Fixed(entity.position[2].toBlock());
    return true;
}

// Three blooms by bands of a noise (frequency 0.01; offsets 34234, 234234): below -0.5, above 0.5 and between.
int randomBloom(int x, int y)
{
    float value = noise(x * 0.01 + 34234.0, y * 0.01 + 234234.0);
    if (value < -0.5f) {
        return PlantFlowers;
    }
    if (value > 0.5f) {
        return PlantFlowers2;
    }
    return PlantFlowers3;
}

int randomFlag(int flags)
{
    std::vector<int> bits;
    if (flags & SpawnGroupSizeSingle) {
        bits.push_back(SpawnGroupSizeSingle);
    }
    if (flags & SpawnGroupSizeSmall) {
        bits.push_back(SpawnGroupSizeSmall);
    }
    if (flags & SpawnGroupSizeSmallLeader) {
        bits.push_back(SpawnGroupSizeSmallLeader);
    }
    if (flags & SpawnGroupSizeCamp) {
        bits.push_back(SpawnGroupSizeCamp);
    }
    if (flags & SpawnGroupSizeLarge) {
        bits.push_back(SpawnGroupSizeLarge);
    }
    if (bits.empty()) {
        return 0;
    }
    return bits[rand() % bits.size()];
}

plasma::Vector<3, Fixed> Zone::randomPosition(int cellX, int cellY, int kind)
{
    if (kind == PositionEagleSeat) {
        for (StaticEntity &entity : m_staticEntities) {
            if (entity.type == StaticEntityEagleSeat) {
                return entity.position +
                       plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, entity.scaledSize()[2]));
            }
        }
    }
    PositionKey key = {{cellX, cellY, kind}};
    std::map<PositionKey, std::vector<plasma::Vector<3, Fixed>>>::iterator it = m_remainingPositions.find(key);
    if (it != m_remainingPositions.end()) {
        std::vector<plasma::Vector<3, Fixed>> &positions = it->second;
        if (positions.empty()) {
            positions = m_positionsByKind[PositionKey {{cellX, cellY, kind}}];
        }
        if (!positions.empty()) {
            int index = rand() % positions.size();
            plasma::Vector<3, Fixed> result = positions[index];
            positions.erase(positions.begin() + index);
            return result;
        }
    }
    int x = m_position[0] * BlocksPerZone + BlocksPerZone / 2;
    int y = m_position[1] * BlocksPerZone + BlocksPerZone / 2;
    // UB reproduced: the column is used without a null check (it always exists: the point is the zone's centre).
    Field *column = field(x, y);
    int z = column->m_baseZ + (int)column->m_blocks.size();
    return plasma::Vector<3, Fixed>((Fixed(x))-Fixed(-0.5), (Fixed(y))-Fixed(-0.5), Fixed(z));
}

plasma::Vector<3, Fixed> Zone::randomSpawnPosition(int cellX, int cellY)
{
    if (positionCount(cellX, cellY, PositionSheltered)) {
        return randomPosition(cellX, cellY, PositionSheltered);
    }
    if (positionCount(cellX, cellY, PositionStorey0Floor)) {
        return randomPosition(cellX, cellY, PositionStorey0Floor);
    }
    if (positionCount(cellX, cellY, PositionNaturalGround)) {
        return randomPosition(cellX, cellY, PositionNaturalGround);
    }
    if (positionCount(cellX, cellY, PositionGround)) {
        return randomPosition(cellX, cellY, PositionGround);
    }
    return randomPosition(cellX, cellY, PositionAny);
}

void Zone::setBlock(int x, int y, int z, const Block &block)
{
    Field *column = field(x, y);
    if (!column) {
        return;
    }
    if (block.type == BlockAir && block.flags == BlockFlagNone && z >= (int)column->m_blocks.size() + column->m_baseZ) {
        return;
    }
    if ((int)column->m_blocks.size() == 0) {
        column->m_baseZ = z;
    }
    int base = column->m_baseZ;
    if (z <= 0 && block.type == BlockAir && !(block.flags & BlockFlagProtected)) {
        column->setBlock(z - base, s_waterCube);
    } else {
        column->setBlock(z - base, block);
    }
    for (int below = z + 1; below < base; below++) {
        Block generated =
            m_world->columnBlock(x, y, below, column->m_mountainCliffs, column->m_temperature, column->m_humidity,
                                 column->m_firelands, column->m_undead, column->m_enchantedForest);
        column->setBlock(below - column->m_baseZ, generated);
    }
}

void Zone::updateLists()
{
    m_lightEntities.clear();
    m_treasurePositions.clear();
    for (ZoneItem &item : m_items) {
        if (item.item.category == ItemCategoryGeneratedItem) {
            m_treasurePositions.push_back(item.position);
        }
    }
    for (StaticEntity &entity : m_staticEntities) {
        if (entity.type == StaticEntityStreetLight02 || entity.type == StaticEntityStreetLight ||
            entity.type == StaticEntityStreetLight01 || entity.type == StaticEntityStreetLightUndead ||
            entity.type == StaticEntityCampfire || entity.type == StaticEntityDruidLight ||
            entity.type == StaticEntityDruidLight2 || entity.type == StaticEntityDruidLight3 ||
            entity.type == StaticEntityBrazier || entity.type == StaticEntityGoldenGate ||
            entity.type == StaticEntityGoddess2) {
            m_lightEntities.push_back(entity);
        }
        if (entity.type == StaticEntityRewardChest) {
            if (!(cubeAt(entity.position[0], entity.position[1], entity.position[2]).flags & BlockFlagDungeon)) {
                m_treasurePositions.push_back(entity.position);
            }
        }
    }
}

void Zone::updateOccupancy()
{
    for (const IntTriple &position : m_occupiedBlockPositions) {
        Block block = getBlock(position[0], position[1], position[2]);
        if (block.flags & BlockFlagBarrier) {
            block.flags &= ~BlockFlagBarrier;
            setBlock(position[0], position[1], position[2], block);
        }
    }
    for (StaticEntity &entity : m_staticEntities) {
        if (entity.type == StaticEntityBeachTowel || entity.type == StaticEntityWoodMat ||
            entity.type == StaticEntityFireTrap || entity.type == StaticEntitySpikeTrap ||
            entity.type == StaticEntityLever || entity.type == StaticEntityInscribedTablet) {
            continue;
        }
        if (!entity.active && (entity.type == StaticEntityDoor || entity.type == StaticEntityBigDoor ||
                               entity.type == StaticEntityBigIronDoor || entity.type == StaticEntityWindow ||
                               entity.type == StaticEntityGate || entity.type == StaticEntityLockedGate ||
                               entity.type == StaticEntityRuneGate || entity.type == StaticEntityStoneGate ||
                               entity.type == StaticEntityStoneWall || entity.type == StaticEntitySpiderwebWall ||
                               entity.type == StaticEntityMagicBarrier || entity.type == StaticEntityGoldenGate)) {
            continue;
        }
        plasma::Vector<3, float> size = entity.scaledSize();
        if (entity.orientation & 1) {
            std::swap(size[0], size[1]);
        }
        // The occupied blocks: the box shrunk to just under half its width and its full height, so that an exact fit
        // does not reach the next block.
        Fixed halfX = Fixed(size[0] * 0.499f);
        int minX = (entity.position[0] - halfX).toBlock();
        int maxX = (entity.position[0] + halfX).toBlock();
        Fixed halfY = Fixed(size[1] * 0.499f);
        int minY = (entity.position[1] - halfY).toBlock();
        int maxY = (entity.position[1] + halfY).toBlock();
        int minZ = entity.position[2].toBlock();
        int maxZ = (entity.position[2] + size[2] * 0.999f).toBlock();
        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                for (int z = minZ; z <= maxZ; z++) {
                    Block block = getBlock(x, y, z);
                    if (!(block.flags & BlockFlagBarrier)) {
                        block.flags |= BlockFlagBarrier;
                        if (entity.type == StaticEntityGrid) {
                            block.flags |= BlockFlagSpiritPassable;
                        }
                        if (entity.type == StaticEntityDoor || entity.type == StaticEntityBigDoor ||
                            entity.type == StaticEntityBigIronDoor) {
                            block.flags |= BlockFlagPassableBarrier;
                        }
                        setBlock(x, y, z, block);
                        m_occupiedBlockPositions.insert(IntTriple {{x, y, z}});
                    }
                }
            }
        }
    }
}

void hasResidentRace(int dungeonType, Realm &realm, Zone::Resident &resident)
{
    std::vector<int> races;
    switch (dungeonType) {
    case PlacePyramid:
        races = {RaceZombie,          RaceSkeleton,   RaceAnubis,    RaceHorus,      RaceDjinn,        RaceNomadMale,
                 RaceDesertOnionling, RaceFireBeetle, RaceCrocodile, RaceSandHorror, RaceLizardmanMale};
        break;
    case PlaceTemple:
        races = {RaceZombie, RaceSkeleton, RaceAnubis,    RaceHorus,  RaceDjinn, RaceNomadMale,
                 RaceFrog,   RaceMosquito, RaceCrocodile, RaceMakake, RaceFly,   RaceFrogmanMale};
        break;
    case PlaceVault:
        races = {RaceImp,     RaceFlyingEye, RaceDemonMarauder, RaceDarkCultistMale, RaceDevourer,
                 RaceCrawler, RaceMinotaur,  RaceLavaTurtle,    RaceSeeker};
        break;
    case PlaceShrine:
        races = {RaceLemonBeetle, RaceBarkBeetle, RaceSnoutBeetle,    RaceCaterpillar,      RaceFly,
                 RaceHornet,      RaceBumblebee,  RaceSnail,          RaceEarthCaterpillar, RaceBloomling,
                 RaceHabanero,    RaceChiling,    RaceRadishCreature, RacePlantCreature,    RaceInsectGuard};
        break;
    case PlaceGrotto:
        races = {RaceGreenSlime, RaceBat, RaceMerman, RacePirateMale, RaceCrab, RaceSeaCrab, RaceRockling};
        break;
    case PlaceMines:
        races = {RaceAlphaDog, RaceSavageDog, RaceDwarfMale, RaceBat,  RaceZombie,
                 RaceWerewolf, RaceSkeleton,  RaceRockling,  RaceOgre, RaceFlyingEye};
        break;
    case PlaceSewers:
        races = {RaceCrocodile,   RaceGreenSlime, RaceZombie,  RaceSkeleton,
                 RaceSkeletonDog, RaceGnobold,    RaceCrawler, RaceDevourer};
        break;
    case PlaceCastle:
        races = {RaceElfMale,     RaceHumanMale,       RaceDwarfMale,     RaceGoblinMale,  RaceOrcMale, RaceUndeadMale,
                 RaceSkeleton,    RaceSkeletonKnight,  RaceImp,           RaceWizard,      RaceWitch,   RaceSorcerer,
                 RaceNecromancer, RaceFlyingEye,       RaceAlphaDog,      RaceSavageDog,   RaceBat,     RaceBlueSlime,
                 RaceCrawler,     RaceDarkCultistMale, RaceDemonMarauder, RaceMonstrosity, RaceWerewolf};
        break;
    default:
        races = {RaceZombie,      RaceSkeleton,    RaceSkeletonKnight, RaceVampire,    RaceNecromancer,
                 RaceSkeletonDog, RaceMonstrosity, RaceDarkJester,     RaceFrightener, RaceUndeadMale};
        break;
    }
    // In a tyrant's realm one resident in eight keeps a race of its own.
    if (!races.empty() && (!realm.hasTyrant() || rand() % 8 != 0)) {
        resident.race = races[rand() % races.size()];
    } else {
        resident.race = realm.native(rand());
        resident.factionKind = realm.form();
        resident.realm = realm.id;
        if (!realm.hasPeople()) {
            resident.appearance.appearanceFlags |= AppearanceFlagSpiritForm;
        }
    }
    resident.race = randomRace(resident.race);
}

void Zone::update()
{
    if (m_world->m_state.m_day == m_lastUpdateDay)
        return;
    m_lastUpdateDay = m_world->m_state.m_day;
    for (Resident *resident : m_dailyResidents) {
        m_residents.erase(resident->id);
        delete resident;
    }
    m_dailyResidents.clear();
    std::vector<StaticEntity> previous(m_staticEntities);
    m_staticEntities = m_initialStaticEntities;
    if (previous != m_staticEntities) {
        m_world->m_zonesWithChangedObjects.insert(this);
        m_staticEntitiesChanged = true;
    }
    // Generates level 0..4 daily residents from the day and a zone hash with salt 4420577. Every fifth day, staggered
    // by x + 3y, one zone in five gets visitors, sometimes a camp or gem trader. The signed seed hash can overflow.
    srand(m_world->m_state.m_day + fastRandomInline(m_position[0] + 4420577, m_position[1]));
    if (!m_residentPositions.empty()) {
        int count = (int)(rand() % (m_residentPositions.size() + 1));
        if (count < 2) {
            count = 2;
        }
        for (int i = 0; i < count; ++i) {
            Resident *resident = new Resident;
            resident->hostility = HostilityFriendlyNpc;
            resident->level = rand() % 5;
            resident->race = randomRace(randomFolkRace(BiomeNone));
            resident->behaviours = BehaviourUseFurniture;
            resident->classType = rand() % (ClassRogue - ClassWarrior + 1) + ClassWarrior;
            fillAppearance(resident->race, resident->appearance, nullptr, 0);
            equip(resident->hostility, resident->race, resident->level, resident->appearance.appearanceFlags,
                  resident->factionKind, resident->classType, resident->specialization, resident->equipment,
                  resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces, resident->inventory);
            resident->position = m_residentPositions[i % count];
            adoptResident(resident);
            m_dailyResidents.push_back(resident);
        }
    }
    if (modulo(m_world->m_state.m_day + m_position[0] + m_position[1] * 3, 5) != 0)
        return;
    if (rand() % 5 == 0) {
        int count = rand() % 4 + 1;
        int level = rand() % 5;
        rand();
        int form = 0;
        int realmId = 0;
        int classType = ClassNone;
        unsigned int flags = 0;
        bool camp = rand() % 4 == 0;
        int cellX = floorDivide(m_position[0] * BlocksPerZone, BlocksPerCell) + rand() % 4;
        int cellY = floorDivide(m_position[1] * BlocksPerZone, BlocksPerCell) + rand() % 4;
        plasma::Vector<2, int> region =
            m_world->findRegion(cellX * BlocksPerCell + BlocksPerCell / 2, cellY * BlocksPerCell + BlocksPerCell / 2);
        int raceTable = randomFolkRace(m_world->landBiome(region[0], region[1]));
        int positionKind = positionCount(cellX, cellY, PositionCave) ? PositionCave : PositionGround;
        if (!positionCount(cellX, cellY, positionKind))
            return;
        int hostility = HostilityFriendlyNpc;
        if (rand() % 4 == 0) {
            raceTable = RaceGnome;
            classType = ClassGemTrader;
            count = 1;
        } else {
            int placeY = floorDivide(m_position[1] * BlocksPerZone, BlocksPerPlace);
            int placeX = floorDivide(m_position[0] * BlocksPerZone, BlocksPerPlace);
            World *world = m_world;
            if (world->locationType(placeX, placeY, world->isLargePlace(placeX, placeY)) == PlaceVillage) {
                camp = false;
            } else {
                placeX = floorDivide(cellX * BlocksPerCell, BlocksPerPlace);
                placeY = floorDivide(cellY * BlocksPerCell, BlocksPerPlace);
                Realm realm(m_world->mortalRealm(placeX, placeY, m_world->isLargePlace(placeX, placeY)));
                if (rand() % 4 == 0) {
                    form = realm.count();
                    hostility = HostilityEnemy;
                    if (form < FactionObsidianKnights || form > FactionSteelEmpire)
                        hostility = HostilityFriendlyNpc;
                    switch (form) {
                    case FactionBloodaxeClan:
                        raceTable = randomCreature();
                        break;
                    case FactionUnholyPact:
                        raceTable = randomUndeadBoss();
                        break;
                    case FactionCultOfDoom:
                        raceTable = randomDemonBoss();
                        break;
                    case FactionSteelEmpire:
                        raceTable = RaceEmpireSoldierMale;
                        break;
                    case FactionOrderOfTheLight:
                        classType = ClassOrderOfTheLightMember;
                        raceTable = randomPeacefulRace();
                        break;
                    case FactionObsidianKnights:
                        classType = ClassObsidianKnight;
                        raceTable = randomPeacefulRace();
                        break;
                    case FactionDruidsOfMana:
                        classType = ClassDruidOfMana;
                        raceTable = randomFighterRace();
                        break;
                    }
                } else {
                    raceTable = realm.native(rand());
                    form = realm.form();
                    realmId = realm.id;
                    hostility = realm.hasTyrant() ? 1 : 3;
                    if (!realm.hasPeople())
                        flags = AppearanceFlagSpiritForm;
                }
            }
        }
        std::set<long long> group;
        for (int i = 0; i < count; ++i) {
            Resident *resident = new Resident;
            resident->hostility = hostility;
            resident->level = level;
            resident->race = randomRace(raceTable);
            resident->behaviours = BehaviourUseFurniture | BehaviourWander;
            if (!camp)
                resident->behaviours = BehaviourRoam | BehaviourUseFurniture | BehaviourWander;
            resident->classType = classType;
            resident->factionKind = form;
            resident->realm = realmId;
            resident->appearance.appearanceFlags |= flags;
            fillAppearance(resident->race, resident->appearance, nullptr, 0);
            equip(resident->hostility, resident->race, resident->level, resident->appearance.appearanceFlags,
                  resident->factionKind, resident->classType, resident->specialization, resident->equipment,
                  resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces, resident->inventory);
            resident->position = randomPosition(cellX, cellY, positionKind);
            adoptResident(resident);
            m_dailyResidents.push_back(resident);
            group.insert(resident->id);
        }
        for (Resident *resident : m_dailyResidents)
            resident->groupMembers = group;
        if (camp) {
            StaticEntity fire;
            fire.type = StaticEntityCampfire;
            fire.position = randomPosition(cellX, cellY, positionKind);
            fire.orientation = rand() % 4;
            addStaticEntity(fire);
            // Up to three furnishings around a camp fire.
            int furnishings = rand() % 4;
            for (int i = 0; i < furnishings; ++i) {
                StaticEntity entity;
                switch (rand() % 8) {
                case 1:
                    entity.type = StaticEntityStool;
                    break;
                case 2:
                    entity.type = StaticEntityWoodMat;
                    break;
                case 3:
                    entity.type = StaticEntitySack;
                    break;
                case 4:
                    entity.type = StaticEntityCrate;
                    break;
                case 5:
                    entity.type = StaticEntityOpenCrate;
                    break;
                case 6:
                    entity.type = StaticEntityBarrel;
                    break;
                case 7:
                    entity.type = rand() % (StaticEntityWagon2 - StaticEntityWagon1 + 1) + StaticEntityWagon1;
                    if (raceTable == RaceWitch || raceTable == RaceWizard || raceTable == RaceSorcerer ||
                        raceTable == RaceNecromancer)
                        entity.type = StaticEntityWitchWagon;
                    break;
                default:
                    entity.type = StaticEntityTent;
                    break;
                }
                entity.position = randomPosition(cellX, cellY, positionKind);
                entity.orientation = rand() % 4;
                addStaticEntity(entity);
            }
            updateLists();
        }
    }
    if (previous != m_staticEntities) {
        m_world->m_zonesWithChangedObjects.insert(this);
        m_staticEntitiesChanged = true;
    }
}

void Zone::addSpawnGroup(int cellX, int cellY, bool special)
{
    SpawnTable::Group group;
    if (!m_world->m_spawnTable.chooseGroup(cellX, cellY, special, group)) {
        return;
    }
    if (group.creatureTypes.empty()) {
        return;
    }
    int kind = special ? PositionCave : PositionGround;
    if (positionCount(cellX, cellY, kind) == 0) {
        return;
    }
    int count = 1;
    int size = randomFlag(group.sizeFlags);
    switch (size) {
    case SpawnGroupSizeSingle:
        count = 1;
        break;
    // Group sizes: 3..5 for the small groups, 6..13 for the large ones, levels 0..4.
    case SpawnGroupSizeSmall:
    case SpawnGroupSizeSmallLeader:
    case SpawnGroupSizeCamp:
        count = (std::min)(rand() % 3 + 3, positionCount(cellX, cellY, kind));
        break;
    case SpawnGroupSizeLarge:
        count = (std::min)(rand() % 8 + 6, positionCount(cellX, cellY, kind));
        break;
    }
    int leaderChance = 20;
    if (size == SpawnGroupSizeCamp) {
        leaderChance = 2;
    } else if (size == SpawnGroupSizeSmallLeader) {
        leaderChance = 8;
    }
    int level = rand() % 5;
    for (int i = 0; i < count; i++) {
        Resident *resident = new Resident;
        resident->hostility = HostilityEnemy;
        int race = group.creatureTypes[rand() % group.creatureTypes.size()];
        resident->race = race;
        if (size == SpawnGroupSizeSmallLeader || size == SpawnGroupSizeCamp) {
            resident->level = level;
        } else {
            resident->level = m_world->m_spawnTable.m_powers[race];
        }
        if ((race >= RaceHornetsNest && race <= RaceMolehill) || race == RaceDemonPortal) {
            resident->hostility = HostilityAttackableObject;
        } else if (i == 0 && rand() % leaderChance == 0) {
            resident->appearance.appearanceFlags |= AppearanceFlagTinted | AppearanceFlagMissionResident;
        }
        fillAppearance(resident->race, resident->appearance, nullptr, true);
        equip(resident->hostility, resident->race, resident->level, resident->appearance.appearanceFlags,
              resident->factionKind, resident->classType, resident->specialization, resident->equipment,
              resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces, resident->inventory);
        resident->position = randomPosition(cellX, cellY, kind);
        resident->behaviours = BehaviourWander;
        if (size == SpawnGroupSizeCamp) {
            resident->behaviours |= BehaviourUseFurniture;
        }
        adoptResident(resident);
        if (resident->race == RaceSkullSlater) {
            // Its rider (race 165), of the same level and appearance flags, owned by it.
            Resident *rider = new Resident;
            rider->race = RaceSkeletonKnight;
            rider->level = resident->level;
            fillAppearance(rider->race, rider->appearance, nullptr, true);
            equip(rider->hostility, rider->race, rider->level, rider->appearance.appearanceFlags, rider->factionKind,
                  rider->classType, rider->specialization, rider->equipment, rider->specialSkills,
                  rider->summonAppearanceMode, rider->summonRaces, rider->inventory);
            rider->position = resident->position;
            rider->appearance.appearanceFlags = resident->appearance.appearanceFlags;
            rider->ownerId = resident->id;
            adoptResident(rider);
        }
    }
    if (size == 8) {
        // A camp: a fire and up to five more things at the cell's positions.
        int campEntityCount = (std::min)(6, positionCount(cellX, cellY, kind));
        for (int i = 0; i < campEntityCount; i++) {
            StaticEntity entity;
            entity.type = StaticEntityCampfire;
            entity.position = randomPosition(cellX, cellY, kind);
            entity.orientation = rand() % 4;
            if (i > 0) {
                // Around a camp: a beach towel below height 25 (else a wooden mat) for three in four, an umbrella for
                // one in three of the rest on a beach.
                if (rand() % 4 != 0) {
                    entity.type =
                        entity.position[2] < Fixed(BeachTowelMaxHeight) ? StaticEntityBeachTowel : StaticEntityWoodMat;
                } else if (entity.position[2] < Fixed(BeachTowelMaxHeight) && rand() % 3 == 0) {
                    entity.type = StaticEntityBeachUmbrella;
                } else {
                    switch (rand() % 6) {
                    case 0:
                        entity.type = StaticEntityBarrel;
                        break;
                    case 1:
                        entity.type = StaticEntityCrate;
                        break;
                    case 2:
                        entity.type = StaticEntityOpenCrate;
                        break;
                    case 3:
                        entity.type = StaticEntitySack;
                        break;
                    case 4:
                        entity.type = StaticEntityTent;
                        break;
                    case 5:
                        entity.type = StaticEntityStool;
                        break;
                    }
                }
            }
            addStaticEntity(entity);
        }
    }
}

// Creature resident generation.
void Zone::addResidents(int cellX, int cellY, Missions &missions)
{
    int placeX = floorDivide(cellX * BlocksPerCell, BlocksPerPlace);
    int placeY = floorDivide(cellY * BlocksPerCell, BlocksPerPlace);
    int placeType = m_world->locationType(placeX, placeY, m_world->isLargePlace(placeX, placeY));
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int dungeon = world->dungeonType(placeX, placeY, placeType, large);
    StructureCell cell = m_world->structureCell(cellX, cellY, true);
    if (cell.kind == StructureCell_EndBoss)
        std::cout << "END BOSS?" << std::endl;
    if (!positionCount(cellX, cellY, PositionDungeonFloor))
        return;
    std::set<std::array<int, 2>> cells;
    // The dungeon room (8 x 8 cells) holds the activator cells.
    int firstX = floorDivide(cellX, CellsPerDungeonRoom) * CellsPerDungeonRoom;
    int firstY = floorDivide(cellY, CellsPerDungeonRoom) * CellsPerDungeonRoom;
    int lastX = firstX + 8;
    int lastY = firstY + 8;
    for (int x = firstX; x < lastX; ++x) {
        for (int y = firstY; y < lastY; ++y) {
            StructureCell neighbour = m_world->structureCell(x, y, true);
            if (neighbour.kind && neighbour.roomPart == RoomPart_Gate)
                cells.insert(std::array<int, 2> {{x, y}});
        }
    }
    int level = (std::min)(4, (cell.side >> 2) + 1);
    World *generator = m_world;
    Realm realm(generator->mortalRealm(placeX, placeY, generator->isLargePlace(placeX, placeY)));
    Resident *resident;
    // Seed hash uses overflowing signed arithmetic.
    if (cell.kind == StructureCell_Guardian) {
        resident = new Resident;
        resident->hostility = HostilityEnemy;
        resident->race = RaceDemonMarauder;
        resident->appearance.appearanceFlags |= AppearanceFlagTinted | AppearanceFlagMissionResident;
        resident->level = level;
        // The draws are seeded by hashes of the cell with salts of their own; one resident in eight is promoted a
        // level (half of them wandering).
        srand(fastRandomInline(cellX + 0x53735, cellY));
        hasResidentRace(dungeon, realm, *resident);
        srand(fastRandomInline(cellX + 0x8043e, cellY));
        resident->race = randomRace(resident->race);
        fillAppearance(resident->race, resident->appearance, nullptr, false);
        equip(resident->hostility, resident->race, resident->level, resident->appearance.appearanceFlags,
              resident->factionKind, resident->classType, resident->specialization, resident->equipment,
              resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces, resident->inventory);
        resident->position = randomPosition(cellX, cellY, PositionDungeonFloor);
        resident->behaviours = BehaviourStand;
    } else if (cell.kind == StructureCell_EndBoss) {
        resident = new Resident;
        resident->hostility = HostilityEnemy;
        resident->race = RaceDarkCultistMale;
        resident->appearance.appearanceFlags |=
            AppearanceFlagTinted | AppearanceFlagMissionResident | AppearanceFlagEnhanced;
        resident->level = level;
        srand(fastRandomInline(placeX + 0x20d2d, placeY));
        hasResidentRace(dungeon, realm, *resident);
        srand(fastRandomInline(cellX + 0x1191ac, placeY));
        resident->race = randomRace(resident->race);
        fillAppearance(resident->race, resident->appearance, nullptr, false);
        equip(resident->hostility, resident->race, resident->level, resident->appearance.appearanceFlags,
              resident->factionKind, resident->classType, resident->specialization, resident->equipment,
              resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces, resident->inventory);
        resident->position = randomPosition(cellX, cellY, PositionDungeonFloor);
        resident->behaviours = BehaviourStand;
    } else if (cell.kind == StructureCell_Enemy) {
        resident = new Resident;
        resident->hostility = HostilityEnemy;
        resident->level = level;
        srand(fastRandomInline(cellX, cellY) + 0xa26ed);
        bool wanderer = false;
        if (rand() % 8 == 0) {
            resident->level = (std::min)(4, resident->level + 1);
            wanderer = rand() % 2 != 0;
        }
        int seed = hash(resident->level) + fastRandomInline(placeX, placeY) +
                   fastRandomInline(floorDivide(cellX, CellsPerDungeonRoom), floorDivide(cellY, CellsPerDungeonRoom));
        if (wanderer)
            seed++;
        srand(seed);
        hasResidentRace(dungeon, realm, *resident);
        srand(fastRandomInline(cellX + 0x5a48, cellY));
        resident->race = randomRace(resident->race);
        resident->appearance.appearanceFlags |= AppearanceFlagMissionResident;
        fillAppearance(resident->race, resident->appearance, nullptr, false);
        equip(resident->hostility, resident->race, resident->level, resident->appearance.appearanceFlags,
              resident->factionKind, resident->classType, resident->specialization, resident->equipment,
              resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces, resident->inventory);
        resident->position = randomPosition(cellX, cellY, PositionDungeonFloor);
        resident->behaviours = wanderer ? BehaviourWander : BehaviourStand;
    } else {
        return;
    }
    resident->angle = (float)rand() * 360.0f / (float)RAND_MAX;
    resident->activatorCells = cells;
    // No dungeon resident is taller than six blocks.
    if (resident->appearance.size[2] > MaxDungeonResidentHeight)
        resident->appearance.size *= MaxDungeonResidentHeight / resident->appearance.size[2];
    adoptResident(resident);
}

// Mission resident generation.
void Zone::addCastleResidents(int cellX, int cellY, Missions &missions, int castlePart,
                              const plasma::Vector<2, int> &castleCell)
{
    int placeX = floorDivide(cellX * BlocksPerCell, BlocksPerPlace);
    int placeY = floorDivide(cellY * BlocksPerCell, BlocksPerPlace);
    int placeType = m_world->locationType(placeX, placeY, m_world->isLargePlace(placeX, placeY));
    World *world = m_world;
    if (world->isLargePlace(placeX, placeY)) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    plasma::Vector<2, int> center = world->placeCenter(placeX, placeY, world->isLargePlace(placeX, placeY));
    Realm realm(world->mortalRealm(placeX, placeY, world->isLargePlace(placeX, placeY)));
    // Seed hash uses overflowing signed arithmetic.
    if ((!halfChanceA(placeX, placeY) || missions.chooseRoomAt(placeX, placeY) == StaticEntityGate) &&
        !isCastleBuilding(m_world->zoneBuildingType(cellX, cellY))) {
        int neighbourX = cellX;
        int neighbourY = cellY;
        int side;
        bool found = false;
        for (side = 0; side < 4; ++side) {
            neighbourX = cellX;
            neighbourY = cellY;
            switch (side) {
            case NeighbourSide_West:
                --neighbourX;
                break;
            case NeighbourSide_East:
                ++neighbourX;
                break;
            case NeighbourSide_South:
                --neighbourY;
                break;
            case NeighbourSide_North:
                ++neighbourY;
                break;
            }
            if (m_world->zoneBuildingType(neighbourX, neighbourY) == BuildingCastleGate) {
                found = true;
                break;
            }
        }
        if (!found)
            return;
        int count = m_world->notchWidth(placeX, placeY);
        // The castle's residents: seeded by hashes of the place (salt 9003) and cell; one cell in six has residents,
        // one in eight is promoted.
        srand(fastRandomInline(placeX, placeY) + 9003);
        for (int i = 0; i < count; ++i) {
            Resident *resident = new Resident;
            resident->hostility = HostilityEnemy;
            resident->race = randomRace(RaceHumanMale);
            resident->activatorCells.insert(std::array<int, 2> {{neighbourX, neighbourY}});
            int level = missions.requestCount(placeX, placeY);
            resident->level = (std::max)(0, level - 1 + rand() % 2);
            srand(fastRandomInline(placeX + 0x2fdf5d, placeY));
            hasResidentRace(placeType, realm, *resident);
            resident->appearance.appearanceFlags |= AppearanceFlagMissionResident;
            if (count == 1)
                resident->appearance.appearanceFlags |= AppearanceFlagTinted;
            srand(fastRandomInline(cellX + 0x93ac8, cellY) + i);
            resident->race = randomRace(resident->race);
            fillAppearance(resident->race, resident->appearance, nullptr, false);
            equip(resident->hostility, resident->race, resident->level, resident->appearance.appearanceFlags,
                  resident->factionKind, resident->classType, resident->specialization, resident->equipment,
                  resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces, resident->inventory);
            resident->position = randomPosition(cellX, cellY, PositionAny);
            resident->behaviours = BehaviourStand | BehaviourWander;
            resident->angle = (float)rand() * 360.0f / (float)RAND_MAX;
            adoptResident(resident);
        }
        return;
    }
    if (!isCastleBuilding(m_world->zoneBuildingType(cellX, cellY)))
        return;
    int level = missions.requestCount(placeX, placeY);
    int centerCellX = floorDivide(center[0], BlocksPerCell);
    int centerCellY = floorDivide(center[1], BlocksPerCell);
    int direction = m_world->castleDirection(placeX, placeY);
    int rotation = m_world->castleRotation(placeX, placeY);
    int size = m_world->castleSize(placeX, placeY);
    if (m_world->castlePart(cellX, cellY, direction, rotation, size, centerCellX, centerCellY) == 2)
        level = (std::max)(0, level - 1);
    if (cellX == centerCellX && cellY == centerCellY) {
        Resident *resident = new Resident;
        resident->appearance.appearanceFlags |=
            AppearanceFlagTinted | AppearanceFlagMissionResident | AppearanceFlagEnhanced;
        resident->hostility = HostilityEnemy;
        srand(fastRandomInline(placeX + 0x1d5dde1, placeY));
        hasResidentRace(placeType, realm, *resident);
        resident->level = level;
        resident->behaviours = BehaviourWander;
        Item reward = missions.taskRewardAt(placeX, placeY);
        if (reward.category)
            resident->inventory.add(reward, 1, -1);
        srand(fastRandomInline(cellX + 0x1260b4, cellY));
        resident->race = randomRace(resident->race);
        fillAppearance(resident->race, resident->appearance, nullptr, false);
        equip(resident->hostility, resident->race, resident->level, resident->appearance.appearanceFlags,
              resident->factionKind, resident->classType, resident->specialization, resident->equipment,
              resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces, resident->inventory);
        resident->position = randomPosition(cellX, cellY, PositionSheltered);
        adoptResident(resident);
        return;
    }
    for (int storey = 0; storey < 2; ++storey) {
        int positionKind = PositionSheltered;
        if (storey > 0)
            positionKind = PositionStorey0Floor + 1;
        if (m_world->castleBuildingPart(cellX, cellY, storey) == RoomPart_Activator &&
            positionCount(cellX, cellY, positionKind)) {
            Resident *resident = new Resident;
            resident->appearance.appearanceFlags |= AppearanceFlagMissionResident;
            srand(fastRandomInline(placeX + 0x2ffdd, placeY));
            switch (castlePart) {
            default:
                resident->hostility = HostilityEnemy;
                hasResidentRace(placeType, realm, *resident);
                resident->appearance.appearanceFlags |= AppearanceFlagEnhanced;
                resident->level = level;
                resident->behaviours = BehaviourWander;
                resident->activatorCells.insert(std::array<int, 2> {{castleCell[0], castleCell[1]}});
                break;
            case RoomPart_RuneGate:
                resident->hostility = HostilityAttackableObject;
                resident->race = RaceRuneCube;
                resident->level = level;
                resident->activatorCells.insert(std::array<int, 2> {{castleCell[0], castleCell[1]}});
                break;
            case RoomPart_LockedGate:
                resident->hostility = HostilityEnemy;
                hasResidentRace(placeType, realm, *resident);
                resident->appearance.appearanceFlags |= AppearanceFlagTinted;
                resident->behaviours = BehaviourWander;
                Item key;
                key.category = ItemCategoryQuest;
                // One of the twenty quest keys by the place cell (its coordinates modulo 64, as one index).
                key.id = (modulo(placeX, 64) + modulo(placeY, 64) * 64) % 20;
                key.region = m_world->findRegion(center);
                resident->inventory.add(key, 1, -1);
                resident->level = level;
                break;
            }
            srand(fastRandomInline(cellX + 0x1260b4, cellY) + storey);
            resident->race = randomRace(resident->race);
            fillAppearance(resident->race, resident->appearance, nullptr, false);
            equip(resident->hostility, resident->race, resident->level, resident->appearance.appearanceFlags,
                  resident->factionKind, resident->classType, resident->specialization, resident->equipment,
                  resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces, resident->inventory);
            resident->position = randomPosition(cellX, cellY, positionKind);
            resident->angle = (float)(rand() % 4) * 90.0f;
            adoptResident(resident);
        }
    }
    if (!positionCount(cellX, cellY, PositionSheltered) || !m_world->isOccupiedStorey(cellX, cellY, 0))
        return;
    srand(fastRandomInline(cellX, cellY) + 0xa26ed);
    if (rand() % 6 != 0)
        return;
    int count = rand() % 4 + 1;
    for (int i = 0; i < count; ++i) {
        Resident *resident = new Resident;
        resident->hostility = HostilityEnemy;
        srand(fastRandomInline(cellX + 0x305d5, cellY) + i);
        resident->level = (std::max)(0, level - 1 + rand() % 2);
        bool wanderer = false;
        if (rand() % 8 == 0) {
            resident->level = (std::min)(4, resident->level + 1);
            wanderer = true;
        }
        int seed = hash(resident->level) + fastRandomInline(placeX, placeY);
        if (wanderer)
            seed++;
        srand(seed);
        hasResidentRace(placeType, realm, *resident);
        srand(fastRandomInline(cellX + 0x127fd, cellY) + i);
        resident->race = randomRace(resident->race);
        resident->appearance.appearanceFlags |= AppearanceFlagMissionResident;
        fillAppearance(resident->race, resident->appearance, nullptr, false);
        equip(resident->hostility, resident->race, resident->level, resident->appearance.appearanceFlags,
              resident->factionKind, resident->classType, resident->specialization, resident->equipment,
              resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces, resident->inventory);
        resident->position = randomPosition(cellX, cellY, PositionSheltered);
        resident->behaviours = wanderer ? BehaviourWander : BehaviourStand;
        resident->angle = (float)rand() * 360.0f / (float)RAND_MAX;
        adoptResident(resident);
    }
}

void Zone::generate()
{
    std::list<plasma::Vector<3, int>> holes;
    std::list<Shaft> shafts;
    std::vector<Landform> landforms;
    std::vector<Landform> ridgesA;
    std::vector<Landform> ridgesB;
    std::vector<Landform> ridgesC;
    std::vector<Landform> ridgesD;

    // The place (1024-block cell) the zone lies in, and what it holds.
    int placeX = floorDivide(m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(m_position[1] * BlocksPerZone, BlocksPerPlace);
    World::AreaInfo info;
    m_world->getPlace(placeX, placeY, info);
    if (info.large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int castlePart = 0;
    plasma::Vector<2, int> castleCell;
    if (info.type == PlaceCastle) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        m_world->castleDirection(placeX, placeY);
        m_world->castleRotation(placeX, placeY);
        int half = m_world->castleSize(placeX, placeY) / 2;
        for (int x = centerX - half; x < centerX + half; x++) {
            for (int y = centerY - half; y < centerY + half; y++) {
                if (isCastleBuilding(m_world->zoneBuildingType(x, y))) {
                    int part = m_world->castleBuildingPart(x, y, 0);
                    if (part >= RoomPart_Gate && part <= RoomPart_RuneGate) {
                        castlePart = part;
                        castleCell = plasma::Vector<2, int>(x, y);
                    }
                }
            }
        }
    }
    float placeBase = 0.0f;
    if (m_world->isCourtType(info.type)) {
        placeBase = (std::max)(m_world->locationWaterWeight(info), 10.0f);
    }
    int dungeonType = m_world->dungeonType(placeX, placeY, info.type, info.large);
    plasma::Vector<2, int> dungeonPosition = m_world->dungeonPosition(placeX, placeY, info.type, info.large);
    if (dungeonType == PlaceNone) {
        dungeonPosition = plasma::Vector<2, int>(NoCoordinate, NoCoordinate);
    }
    int realmId = m_world->mortalRealm(placeX, placeY, info.large);
    int realmForm = Realm(realmId).form();
    plasma::Vector<2, int> region = m_world->findRegion(info.center[0], info.center[1]);
    int biome = m_world->landBiome(region[0], region[1]);
    Missions missions(m_world, m_world->findRegion(info.center[0], info.center[1]));

    // The terrain, column by column.
    for (int x = m_position[0] * BlocksPerZone; x < m_position[0] * BlocksPerZone + BlocksPerZone; x++) {
        Sleep(0);
        for (int y = m_position[1] * BlocksPerZone; y < m_position[1] * BlocksPerZone + BlocksPerZone; y++) {
            Field &column = m_fields[(long long)(x - m_position[0] * BlocksPerZone) * BlocksPerZone +
                                     (y - m_position[1] * BlocksPerZone)];
            World::BiomeSample sample;
            m_world->biomeSample(x, y, sample);
            m_world->getPlaceInfluence(x, y, info);
            float elevation = sample.biomeElevation(info.type, info.influence);
            float desertness = sample.desertness();
            column.m_temperature = sample.temperature(info.type, info.influence);
            column.m_humidity = sample.humidity();
            column.m_mountains = sample.mountains();
            column.m_firelands = sample.lavaWeight(info.type, info.influence);
            column.m_undead = sample.undead(info.type, info.influence);
            column.m_enchantedForest = sample.enclave(info.type, info.influence);
            column.m_streetAmount = m_world->streetAmount(x, y, info.type, info.influence);
            float wetness = sample.wetness();
            column.m_valleyWater = m_world->valleyWater(x, y, column.m_streetAmount, column.m_firelands,
                                                        column.m_undead, wetness, column.m_mountains, &info);
            column.m_lavaChannels =
                m_world->lavaChannels(x, y, column.m_streetAmount, column.m_firelands, column.m_mountains, &info);
            column.m_poisonChannels =
                m_world->poisonChannels(x, y, column.m_streetAmount, column.m_undead, column.m_mountains, &info);
            column.m_mountainCliffs =
                m_world->mountainCliffs(x, y, elevation, column.m_mountains, column.m_valleyWater,
                                        column.m_lavaChannels, column.m_poisonChannels, column.m_streetAmount, &info);
            column.m_sandAmount = m_world->sandAmount(x, y, column.m_humidity, column.m_mountainCliffs, desertness);
            float terrainHeight = m_world->height(x, y, elevation, column.m_mountainCliffs, column.m_valleyWater,
                                                  column.m_lavaChannels, column.m_poisonChannels, column.m_sandAmount,
                                                  column.m_streetAmount, column.m_mountains, &info, 0);
            column.m_height = terrainHeight;

            bool hasSnowCap = false;
            if (column.m_valleyWater > ChannelThreshold && terrainHeight > -5.0f) {
                // A flooded column: wet ground under water up to the place's level.
                int ground = floor(terrainHeight);
                Block block;
                block = BlockProperties<float>(m_world->groundColor(x, y, ground - 1), BlockWet);
                setBlock(x, y, ground - 1, block);
                int top = floor(m_world->villageHeight(x, y, elevation, &info)) - 1;
                for (int z = ground; z < top; z++) {
                    Block water;
                    water = BlockProperties<float>(plasma::Vector<3, float>(255.0f, 255.0f, 255.0f), BlockWater);
                    setBlock(x, y, z, water);
                }
            } else {
                float heightWest = m_world->height(x - 1, y, 0);
                float heightEast = m_world->height(x + 1, y, 0);
                float heightSouth = m_world->height(x, y - 1, 0);
                float heightNorth = m_world->height(x, y + 1, 0);
                int ground = floor(terrainHeight);
                Block surface;
                surface = m_world->surfaceBlock(x, y, ground, terrainHeight, heightWest, heightEast, heightSouth,
                                                heightNorth, column.m_mountainCliffs, column.m_temperature,
                                                column.m_humidity, column.m_sandAmount, 0.0f, column.m_streetAmount,
                                                column.m_firelands, column.m_undead, column.m_enchantedForest,
                                                column.m_lavaChannels, column.m_poisonChannels);
                setBlock(x, y, floor(terrainHeight), surface);
                if (column.m_sandAmount <= 0.5f && column.m_streetAmount <= 0.5f && terrainHeight > 0.0f) {
                    if (m_world->hasLyingSnow(x, y, column.m_temperature, heightWest, heightEast, heightSouth,
                                              heightNorth) != 0.0f) {
                        BlockProperties<float> cap(m_world->snowColor(x, y), BlockSnow);
                        cap.flags = BlockFlagDestructible;
                        Block snow;
                        snow = cap;
                        setBlock(x, y, (int)terrainHeight + 1, snow);
                        hasSnowCap = true;
                    }
                }
                if (m_world->isCourtType(info.type)) {
                    int floorZ = m_world->masonryFloor(x, y, info, dungeonPosition, terrainHeight, placeBase);
                    int topZ = m_world->monumentTop(x, y, info, dungeonPosition, terrainHeight, placeBase);
                    for (int z = floorZ; z < topZ; z++) {
                        Block masonry;
                        masonry = BlockProperties<float>(m_world->masonryPavement(x, y, z, info), BlockBuilding);
                        setBlock(x, y, z, masonry);
                    }
                }
            }
            float craterFloor;
            float craterHeight;
            if (m_world->groundCrater(x, y, info, column.m_streetAmount, &craterFloor, &craterHeight)) {
                for (int z = floor(craterFloor); z <= floor(craterHeight); z++) {
                    Block block;
                    block =
                        m_world->columnBlock(x, y, z, column.m_mountainCliffs, column.m_temperature, column.m_humidity,
                                             column.m_firelands, column.m_undead, column.m_enchantedForest);
                    setBlock(x, y, z, block);
                }
            }

            // The landforms (never any: the vectors stay empty).
            if (!landforms.empty()) {
                // The landforms' noise profile (dormant: the vectors stay empty): the position warped ten blocks
                // (frequency 0.03), terraces of up to 20 blocks gated by suppression noise (0.02), a lift of up to 70,
                // ridges 5..20 high joined by a smoothstep from a quarter of their length, the base five blocks down.
                Fixed px = Fixed(x) + noise(x * 0.03 + 9431.0, y * 0.03 + 3844.0) * 10.0f;
                Fixed py = Fixed(y) + noise(x * 0.03 + 677.0, y * 0.03 + 766.0) * 10.0f;
                for (Landform &form : landforms) {
                    float terraceSuppressionNoise = noise(x * 0.02 + 123244.0, y * 0.02 + 131234.0);
                    float ground = elevation - (float)form.level;
                    float t = (std::min)(1.0f, (std::max)(0.0f, 1.0f - 2.0f * terraceSuppressionNoise));
                    float terraceHeightNoise = noise(x * 0.02 + 894844.0, y * 0.02 + 5565.0) * t;
                    float heightModulation = noise(x * 0.01 + 70075.0, y * 0.01 + 890458.0) * 0.4f + 0.6f;
                    bool partial = false;
                    float rise = 0.0f;
                    if (terraceHeightNoise > 0.0f && heightModulation > 0.0f) {
                        float step = (std::min)(1.0f, terraceHeightNoise * 20.0f);
                        rise += step * 20.0f * heightModulation;
                        if (step > 0.0f && step < 1.0f) {
                            partial = true;
                        }
                        if (terraceHeightNoise > 0.5f) {
                            float step2 = (std::min)(1.0f, (terraceHeightNoise - 0.5f) * 20.0f);
                            rise += step2 * 20.0f * heightModulation;
                            if (step2 > 0.0f && step2 < 1.0f) {
                                partial = true;
                            }
                        }
                        if (terraceHeightNoise > 0.75f) {
                            float step3 = (std::min)(1.0f, (terraceHeightNoise - 0.75f) * 20.0f);
                            rise += step3 * 20.0f * heightModulation;
                            if (step3 > 0.0f && step3 < 1.0f) {
                                partial = true;
                            }
                        }
                    }
                    ground += rise;
                    float lift = (std::min)(70.0f, (std::max)(0.0f, (terraceHeightNoise + 1.0f) * 0.5f) * 70.0f *
                                                       heightModulation);
                    float best = -1e8f;
                    float ridgeHeight = noise(x * 0.01 + 99444.0, y * 0.01 + 2332233.0) * 10.0f + 10.0f;
                    ridgeHeight = 5.0f > ridgeHeight ? 5.0f : ridgeHeight;
                    float fy = (float)((py - Fixed(form.position[1])) / ((float)form.size[1] * 0.5f));
                    float fx = (float)((px - Fixed(form.position[0])) / ((float)form.size[0] * 0.5f));
                    float distance2 = fx * fx + fy * fy;
                    if (distance2 < 1.0f) {
                        best = (1.0f - distance2 * distance2) * (ridgeHeight + lift) + (elevation - (float)form.level);
                    }
                    for (Landform &ridge : ridgesA) {
                        float d2 = lineSquaredDistance(form.position, ridge.position, px, py);
                        if (d2 < ridgeHeight * ridgeHeight) {
                            float f = (std::max)(
                                0.0f,
                                (std::min)(1.0f, (lineFraction(form.position, ridge.position, px, py) - 0.25f) * 2.0f));
                            float smooth = 3.0f * f * f - 2.0f * f * f * f;
                            float level = (1.0f - smooth) * (float)form.level + smooth * (float)ridge.level;
                            float ground2 = elevation - level;
                            ground = rise + ground2;
                            float lift2 = (std::min)(70.0f, (std::max)(0.0f, (terraceHeightNoise + 1.0f) * 0.5f) *
                                                                70.0f * heightModulation);
                            float q = d2 / (ridgeHeight * ridgeHeight);
                            float h = (lift2 + ridgeHeight) * (1.0f - q * q) + ground2;
                            best = (std::max)(best, h);
                        }
                    }
                    for (Landform &ridge : ridgesB) {
                        float d2 = lineSquaredDistance(form.position, ridge.position, px, py);
                        if (d2 < ridgeHeight * ridgeHeight) {
                            float f = (std::max)(
                                0.0f,
                                (std::min)(1.0f, (lineFraction(form.position, ridge.position, px, py) - 0.25f) * 2.0f));
                            float smooth = 3.0f * f * f - 2.0f * f * f * f;
                            float level = (1.0f - smooth) * (float)form.level + smooth * (float)ridge.level;
                            float ground2 = elevation - level;
                            ground = rise + ground2;
                            float lift2 = (std::min)(70.0f, (std::max)(0.0f, (terraceHeightNoise + 1.0f) * 0.5f) *
                                                                70.0f * heightModulation);
                            float q = d2 / (ridgeHeight * ridgeHeight);
                            float h = (lift2 + ridgeHeight) * (1.0f - q * q) + ground2;
                            best = (std::max)(best, h);
                        }
                    }
                    for (Landform &ridge : ridgesC) {
                        float d2 = lineSquaredDistance(form.position, ridge.position, px, py);
                        if (d2 < ridgeHeight * ridgeHeight) {
                            float f = (std::max)(
                                0.0f,
                                (std::min)(1.0f, (lineFraction(form.position, ridge.position, px, py) - 0.25f) * 2.0f));
                            float smooth = 3.0f * f * f - 2.0f * f * f * f;
                            float level = (1.0f - smooth) * (float)form.level + smooth * (float)ridge.level;
                            float ground2 = elevation - level;
                            ground = rise + ground2;
                            float lift2 = (std::min)(70.0f, (std::max)(0.0f, (terraceHeightNoise + 1.0f) * 0.5f) *
                                                                70.0f * heightModulation);
                            float q = d2 / (ridgeHeight * ridgeHeight);
                            float h = (lift2 + ridgeHeight) * (1.0f - q * q) + ground2;
                            best = (std::max)(best, h);
                        }
                    }
                    for (Landform &ridge : ridgesD) {
                        float d2 = lineSquaredDistance(form.position, ridge.position, px, py);
                        if (d2 < ridgeHeight * ridgeHeight) {
                            float f = (std::max)(
                                0.0f,
                                (std::min)(1.0f, (lineFraction(form.position, ridge.position, px, py) - 0.25f) * 2.0f));
                            float smooth = 3.0f * f * f - 2.0f * f * f * f;
                            float level = (1.0f - smooth) * (float)form.level + smooth * (float)ridge.level;
                            float ground2 = elevation - level;
                            ground = rise + ground2;
                            float lift2 = (std::min)(70.0f, (std::max)(0.0f, (terraceHeightNoise + 1.0f) * 0.5f) *
                                                                70.0f * heightModulation);
                            float q = d2 / (ridgeHeight * ridgeHeight);
                            float h = (lift2 + ridgeHeight) * (1.0f - q * q) + ground2;
                            best = (std::max)(best, h);
                        }
                    }
                    float base = ground - rise - 5.0f;
                    if (terraceSuppressionNoise > 0.5f) {
                        ground -= 25.0f;
                    }
                    int groundZ = floor(ground);
                    int bestZ = floor(best);
                    if (bestZ > groundZ) {
                        if (distance2 < 0.1f * 0.1f) {
                            bestZ = (int)(terrainHeight + 10.0f);
                        }
                        if (!(getBlock(x, y, groundZ).flags & BlockFlagProtected)) {
                            Block block;
                            block = m_world->columnBlock(x, y, groundZ, column.m_mountainCliffs, column.m_temperature,
                                                         column.m_humidity, column.m_firelands, column.m_undead,
                                                         column.m_enchantedForest);
                            setBlock(x, y, groundZ, block);
                        }
                        // The interior's light (noise at 0.02), its cover where noise at 0.01 is above 0.5, and a
                        // shaft one time in 2000 where the ceiling is ten blocks up.
                        float interiorLightNoise = (noise(x * 0.02 + 3484.0, y * 0.02 + 8434.0) + 1.0f) * 0.5f;
                        Block rock(plasma::Vector<3, unsigned char>(255, 255, 255), BlockAir);
                        rock.lightLevel = (unsigned char)(interiorLightNoise * 100.0f);
                        rock.flags = BlockFlagProtected;
                        for (int z = groundZ + 1; z < bestZ; z++) {
                            setBlock(x, y, z, rock);
                        }
                        if (terraceHeightNoise > 0.5f) {
                            for (int z = groundZ + 1; z < floor(base); z++) {
                                Block water(plasma::Vector<3, unsigned char>(255, 255, 255), BlockWater);
                                water.flags = BlockFlagProtected;
                                setBlock(x, y, z, water);
                            }
                        } else if (!partial) {
                            if (noise(x * 0.01 + 9443.0, y * 0.01 + 99944.0) > 0.5f) {
                                Block cover;
                                cover = m_world->coverBlock(x, y, groundZ + 1, ground, column.m_temperature,
                                                            column.m_humidity, 0.0f, 0.0f, 0.0f, column.m_firelands,
                                                            column.m_undead, column.m_enchantedForest);
                                setBlock(x, y, groundZ + 1, cover);
                            }
                        }
                        if (best - ground > 10.0f) {
                            if (fastRandom(x + 50743776, y) % 2000 == 0) {
                                Shaft shaft = {plasma::Vector<3, int>(x, y, bestZ),
                                               (int)(best - (std::max)(ground, base))};
                                shafts.push_back(shaft);
                            }
                        }
                        if (!(getBlock(x, y, bestZ).flags & BlockFlagProtected)) {
                            Block block;
                            block = m_world->columnBlock(x, y, groundZ, column.m_mountainCliffs, column.m_temperature,
                                                         column.m_humidity, column.m_firelands, column.m_undead,
                                                         column.m_enchantedForest);
                            setBlock(x, y, bestZ, block);
                        }
                    }
                }
            }

            // The caves under the column.
            if (column.m_mountainCliffs > 0.5f) {
                float density = m_world->caveDensity(x, y, info.type, info.influence);
                if (density > 0.0f) {
                    Sleep(0);
                    float caveHeight =
                        m_world->height(x, y, elevation, column.m_mountainCliffs, column.m_valleyWater,
                                        column.m_lavaChannels, column.m_poisonChannels, column.m_sandAmount,
                                        column.m_streetAmount, column.m_mountains, &info, HeightFlagSkipPlaceAndHills);
                    // The cave: its ceiling up to 40 blocks of noise (0.01) above the cave floor, its fill light
                    // (0.07) up to 127, its vertical spread 0.06 +- 0.025 (noise at 0.02), the solidness weighted by
                    // depth (a tenth per block from 0.5) and carved below 0.5; the surface colours fade in over 3.3
                    // blocks; one dark spot per five columns and 500 under deep ceilings (16 blocks).
                    float caveTop = (noise(x * 0.01 + 7474.0, y * 0.01 + 98984.0) + 1.0f) * 20.0f + caveHeight;
                    int floorZ = floor(caveHeight);
                    int z = floor(terrainHeight) + 1;
                    Block fill;
                    if (z < floor(terrainHeight)) {
                        fill.lightLevel = (unsigned char)((noise(x * 0.07 + 337.0, y * 0.07 + 743.0) + 1.0f) * 63.75f);
                        fill.flags = BlockFlagProtected;
                    }
                    Block above = getBlock(x, y, floor(terrainHeight));
                    float red = (float)above.color[0] * 2.0f;
                    float green = (float)above.color[1] * 2.0f;
                    float blue = (float)above.color[2] * 2.0f;
                    float spread = noise(x * 0.02 + 8744.0, y * 0.02 + 904.0) * 0.025f + 0.06f;
                    bool open = true;
                    int lastZ = floorZ;
                    for (int zz = floorZ + 1; zz <= z + 1; zz++) {
                        float depth = terrainHeight - (float)zz;
                        float weight = depth * 0.1f + 0.5f;
                        float normalizedCaveVerticalDistance =
                            (std::min)(1.0f, (float)fabs(((float)zz - caveTop) * spread));
                        float caveSolidWeight =
                            1.0f - (1.0f - normalizedCaveVerticalDistance * normalizedCaveVerticalDistance) * density;
                        float weightedCaveSolidness = (3.0f * caveSolidWeight * caveSolidWeight -
                                                       2.0f * caveSolidWeight * caveSolidWeight * caveSolidWeight) *
                                                      weight;
                        if (weightedCaveSolidness <= 0.5f) {
                            setBlock(x, y, zz, fill);
                            if (open) {
                                plasma::Vector<3, float> below = m_world->computeBlock(x, y, zz - 1).color;
                                float r = below[0];
                                float g = below[1];
                                float b = below[2];
                                float surfaceColorBlend = std::max(0.0f, 1.0f - depth * 0.3f);
                                surfaceColorBlend = std::min(1.0f, surfaceColorBlend);
                                unsigned char type = BlockCave;
                                if (surfaceColorBlend > 0.5f) {
                                    type = above.type;
                                }
                                r += (red - r) * surfaceColorBlend;
                                g += (green - g) * surfaceColorBlend;
                                b += (blue - b) * surfaceColorBlend;
                                float bottomCoverBlend = std::max(0.0f, 1.0f - ((float)zz - caveHeight) * 0.3f);
                                bottomCoverBlend = std::min(1.0f, bottomCoverBlend);
                                BlockProperties<float> cover =
                                    column.m_sandAmount > 0.5f || m_world->isBeach(x, y, (float)zz)
                                        ? m_world->computeBlock(x, y, zz - 1)
                                        : m_world->computeCoverBlock(x, y, zz - 1);
                                if (bottomCoverBlend > 0.5f) {
                                    type = cover.type;
                                }
                                r += (cover.color[0] - r) * bottomCoverBlend;
                                g += (cover.color[1] - g) * bottomCoverBlend;
                                b += (cover.color[2] - b) * bottomCoverBlend;
                                Block block;
                                block = BlockProperties<float>(plasma::Vector<3, float>(r, g, b), type);
                                setBlock(x, y, zz - 1, block);
                                if (hasSnowCap && type != BlockCave) {
                                    BlockProperties<float> cap(m_world->snowColor(x, y), BlockSnow);
                                    cap.flags = BlockFlagDestructible;
                                    Block snow;
                                    snow = cap;
                                    setBlock(x, y, zz, snow);
                                }
                                lastZ = zz - 1;
                            }
                            open = false;
                        } else {
                            if (!open) {
                                if ((x + y * 3) % 5 == 0 && rand() % 500 == 0 && z - zz > 16) {
                                    Plant plant;
                                    plant.type = PlantShadow;
                                    plant.position = plasma::Vector<3, Fixed>((Fixed(x))-Fixed(-0.5),
                                                                              (Fixed(y))-Fixed(-0.5), Fixed(zz + 1));
                                    plant.scale = (float)(zz - lastZ);
                                    plant.rotation = 1.0f;
                                    m_plants.push_back(plant);
                                    Block marker;
                                    marker.lightLevel = 255;
                                    setBlock(x, y, zz, marker);
                                }
                            }
                            open = true;
                        }
                    }
                }
            }
        }
    }

    // The shafts: where nothing around the top is free, the top block is opened and marked, and a type 89 plant
    // records the drop.
    for (Shaft &shaft : shafts) {
        bool free = false;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (isFree(getBlock(shaft.position[0] + dx, shaft.position[1] + dy, shaft.position[2]), true, true,
                           true)) {
                    free = true;
                }
            }
        }
        if (!free) {
            setBlock(shaft.position[0], shaft.position[1], shaft.position[2], s_airCube);
            BlockProperties<float> cap(plasma::Vector<3, float>(255.0f, 255.0f, 255.0f), BlockLightSource);
            cap.lightLevel = 255;
            cap.flags = BlockFlagProtected;
            Block block;
            block = cap;
            setBlock(shaft.position[0], shaft.position[1], shaft.position[2] + 1, block);
            Plant plant;
            plant.type = PlantShadow;
            plant.position = plasma::Vector<3, Fixed>((Fixed(shaft.position[0]))-Fixed(-0.5),
                                                      (Fixed(shaft.position[1]))-Fixed(-0.5), Fixed(shaft.position[2]));
            plant.scale = (float)shaft.depth;
            plant.rotation = 1.0f;
            m_plants.push_back(plant);
        }
    }
    CaveBuilder caves(this);
    for (plasma::Vector<3, int> &hole : holes) {
        // A rock of 5..9 blocks in each axis.
        caves.raiseRock(hole[0], hole[1], hole[2], rand() % 5 + 5, rand() % 5 + 5, rand() % 5 + 5, false);
    }

    // The structures: the 16-block cells of the zone and two cells beyond it on every side.
    for (int cellDx = -2; cellDx < 6; cellDx++) {
        for (int cellDy = -2; cellDy < 6; cellDy++) {
            int cellX = floorDivide(m_position[0] * BlocksPerZone, BlocksPerCell) + cellDx;
            int cellY = floorDivide(m_position[1] * BlocksPerZone, BlocksPerCell) + cellDy;
            bool outside = !(cellDx >= 0 && cellDy >= 0 && cellDx < 4 && cellDy < 4);
            StructureCell cell = m_world->structureCell(cellX, cellY, true);
            int dungeonLevel = m_world->dungeonZLevel(cellX, cellY);
            int dungeonTop = m_world->dungeonZLevel(cellX, cellY) + m_world->dungeonStorey(cell);
            int type = m_world->zoneBuildingType(cellX, cellY);
            if (!outside && type == BuildingFlightMasterSite) {
                // The cell's own flag pole (a type 117 entity) at a random block of its middle, on the terrain.
                StaticEntity entity;
                entity.type = StaticEntityEagleSeat;
                entity.position = plasma::Vector<3, Fixed>(
                    // Within the cell's middle half (4..11 blocks in).
                    Fixed(((long long)(rand() % 8) + 4) << FixedFractionBits) + 0.5f + Fixed(cellX * BlocksPerCell),
                    Fixed(((long long)(rand() % 8) + 4) << FixedFractionBits) + 0.5f + Fixed(cellY * BlocksPerCell),
                    Fixed(0));
                entity.position[2] = m_world->height((int)entity.position[0], (int)entity.position[1], 0) + 1.0f;
                entity.orientation = (unsigned char)(rand() % 4);
                putOnGround(entity, 0);
                addStaticEntity(entity);
            }
            int reach = buildingReach(type);
            if (cellDx < 0 && -cellDx > reach) {
                continue;
            }
            if (cellDx >= 4 && cellDx - 3 > reach) {
                continue;
            }
            if (cellDy < 0 && -cellDy > reach) {
                continue;
            }
            if (cellDy >= 4 && cellDy - 3 > reach) {
                continue;
            }
            plasma::Vector<2, int> center = m_world->streetPoint(cellX, cellY);
            if (type == BuildingNone && cell.kind == StructureCell_None) {
                continue;
            }
            // Each structure's draws are seeded by a hash of its cell (modulo 10000000).
            srand(fastRandom(cellX, cellY) % 10000000);
            if (type == BuildingFloatingIslandTreasurePavilion) {
                int level = m_world->structureLevel(cellX, cellY, type);
                CaveBuilder cave(this);
                // A floating island: a plateau four blocks under the level, two cells wide.
                cave.addPlateau(cellX * BlocksPerCell + 8, cellY * BlocksPerCell + 8, level - 4, 32, 32);
            } else {
                if (type == BuildingSanctuary || type == BuildingRaisedSanctuary) {
                    // A tower of the realm: a ring of eight pillars on its platform, a cone on top, a crown for type
                    // 70.
                    int level = m_world->structureLevel(cellX, cellY, type) + 1;
                    StructureBuilder builder(*this, cellX * BlocksPerCell, cellY * BlocksPerCell, 0, BlocksPerCell,
                                             BlocksPerCell, 0, false, nullptr);
                    plasma::Vector<3, float> color = buildingColor(type, cellX, cellY, realmId, realmForm, biome);
                    plasma::Vector<3, float> trim = buildingTrimColor(type, cellX, cellY, realmId, realmForm, biome);
                    int radius = realmTowerRadius(realmId);
                    // The sanctuary's profile: a foundation three blocks up, the material blending from the trim to
                    // the wall colour over 20 blocks, the pillar style by a hash of the realm.
                    int top = 3;
                    builder.setMaterial(trim, TextureBrick, 0.0f, color, TextureBrick, 20.0f, true);
                    if (type == BuildingRaisedSanctuary) {
                        top = 10;
                        radius -= 5;
                        builder.spiral(BlocksPerCell / 2, BlocksPerCell / 2, level - 10, radius - 1, radius + 6, 22);
                    }
                    bool trimmed = hasTowerTrim(type, realmId, realmForm, biome);
                    int ringZ = top + level;
                    builder.foundation(BlocksPerCell / 2, BlocksPerCell / 2, ringZ + 1, radius, 1,
                                       trimmed ? FillCover : FillMaterial, trimmed ? CoverPatches : CoverAll);
                    if (hasTowerBand(type, realmId, realmForm, biome)) {
                        builder.m_terrainOnly = top + 10;
                    }
                    unsigned int style = hash(realmId + 0xdaad) % 3;
                    float pillarRadius = (float)(radius - 3);
                    for (int i = 0; i < 8; i++) {
                        float angle = (float)(i * (2.0 * Pi) / 8.0);
                        switch (style) {
                        case PillarStyle_AngularColumn:
                            builder.angularColumn((int)(cosf(angle) * pillarRadius + 8.0f),
                                                  (int)(sinf(angle) * pillarRadius + 8.0f), ringZ, 2, 15, true);
                            break;
                        case PillarStyle_RoundPillar:
                            builder.roundPillar((int)(cosf(angle) * pillarRadius + 8.0f),
                                                (int)(sinf(angle) * pillarRadius + 8.0f), ringZ, 2, 15, true);
                            break;
                        default:
                            builder.angularPillar((int)(cosf(angle) * pillarRadius + 8.0f),
                                                  (int)(sinf(angle) * pillarRadius + 8.0f), ringZ, 2, 15, true);
                            break;
                        }
                    }
                }
                if (type == BuildingManaPump) {
                    // A pavilion: a stack of discs on a column of thirty discs, a lattice of beams and a ladder on
                    // each side.
                    int level = m_world->structureLevel(cellX, cellY, type) + 1;
                    StructureBuilder tower(*this, cellX * BlocksPerCell, cellY * BlocksPerCell, 0, BlocksPerCell,
                                           BlocksPerCell, 0, false, nullptr);
                    // The mana pump (a hand-authored shape): a disc, a lattice and a ladder in darkened metal (90%).
                    plasma::Vector<3, float> color = buildingColor(type, cellX, cellY, realmId, realmForm, biome);
                    plasma::Vector<3, float> dark = color;
                    dark *= 0.9f;
                    tower.setColor(dark, TexturePlain);
                    tower.cylinder(BlocksPerCell / 2, BlocksPerCell / 2, level + 8, 32, 1, FillMaterial, CoverAll, 0);
                    tower.setColor(color, TexturePlain);
                    tower.cylinder(BlocksPerCell / 2, BlocksPerCell / 2, level + 9, 32, 1, FillMaterial, CoverAll, 0);
                    tower.setColor(plasma::Vector<3, float>(140.0f, 150.0f, 180.0f), TexturePlain);
                    tower.cylinder(BlocksPerCell / 2, BlocksPerCell / 2, level + 10, 32, 1, FillMaterial, CoverAll, 0);
                    tower.cylinder(BlocksPerCell / 2, BlocksPerCell / 2, level + 11, 33, 1, FillMaterial, CoverAll, 0);
                    tower.cylinderShaft(BlocksPerCell / 2, BlocksPerCell / 2, level + 10, 31, 2, 0);
                    tower.cutBox(2, -25, level + 10, 12, 66, 2, false);
                    tower.cutBox(-25, 2, level + 10, 66, 12, 2, false);
                    for (int i = 0; i < 11.2f; i++) {
                        tower.cutBox(5 - 2 * i, -24, level + 10, 1, 66, 1, false);
                        tower.cutBox(10 + 2 * i, -24, level + 10, 1, 66, 1, false);
                        tower.cutBox(-24, 5 - 2 * i, level + 10, 66, 1, 1, false);
                        tower.cutBox(-24, 10 + 2 * i, level + 10, 66, 1, 1, false);
                    }
                    for (int i = 0; i < 30; i++) {
                        tower.setColor(plasma::Vector<3, float>(140.0f, 150.0f, 180.0f) +
                                           plasma::Vector<3, float>(20.0f, 20.0f, 20.0f) * (float)(i & 1),
                                       TexturePlain);
                        tower.cylinder(BlocksPerCell / 2, BlocksPerCell / 2, level - 10 + i, 8, 1, FillMaterial,
                                       CoverAll, 0);
                    }
                    for (int side = 0; side < 4; side++) {
                        StructureBuilder ladder(tower, 0, 0, 0, 16, 16, side, false);
                        int x = 8;
                        int y = -25;
                        ladder.toZone(x, y);
                        int ground = (int)(m_world->height(x, y, 0) - 2.0f);
                        ladder.ladder(12, -25, ground, 2, level - ground + 10);
                        ladder.ladder(4, -25, ground, 2, level - ground + 10);
                    }
                } else if (type == BuildingManaPipeEndpoint) {
                    // A plain platform.
                    StructureBuilder builder(*this, cellX * BlocksPerCell, cellY * BlocksPerCell, 0, BlocksPerCell,
                                             BlocksPerCell, 0, false, nullptr);
                    builder.setColor(buildingColor(type, cellX, cellY, realmId, realmForm, biome), TexturePlain);
                    int level = m_world->structureLevel(cellX, cellY, type);
                    builder.underpinnedBox(0, 0, level, BlocksPerCell, BlocksPerCell, 10, FillMaterial, CoverAll,
                                           false);
                } else if (type == BuildingArenaPodium) {
                    // A stepped pyramid with a resident on top.
                    StructureBuilder builder(*this, cellX * BlocksPerCell, cellY * BlocksPerCell, 0, BlocksPerCell,
                                             BlocksPerCell, 0, false, nullptr);
                    builder.setColor(buildingColor(type, cellX, cellY, realmId, realmForm, biome), TexturePlain);
                    int size = BlocksPerCell;
                    // The arena podium: tiers narrowing by two from the size, its top 15 below.
                    int height = size - 15;
                    for (int step = 0; size > 8; step++, size -= 2) {
                        int level = m_world->structureLevel(cellX, cellY, BuildingArenaPodium);
                        builder.underpinnedBox(step, step, level + step, size, size, height, FillMaterial, CoverAll,
                                               false);
                    }
                    Resident *resident = new Resident();
                    resident->hostility = HostilityFriendlyNpc;
                    resident->race = randomRace(RaceEnhancedOrcMale);
                    resident->classType = ClassArenaChampion;
                    resident->factionKind = FactionBloodaxeClan;
                    resident->angle = 270.0f;
                    fillAppearance(resident->race, resident->appearance, nullptr, false);
                    equip(resident->hostility, resident->race, 0, resident->appearance.appearanceFlags,
                          resident->factionKind, resident->classType, resident->specialization, resident->equipment,
                          resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces,
                          resident->inventory);
                    int level = m_world->structureLevel(cellX, cellY, 97);
                    resident->position = plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 8),
                                                                  Fixed(cellY * BlocksPerCell + 8), Fixed(level + 4));
                    resident->behaviours = BehaviourStand;
                    adoptResident(resident);
                } else if (type == BuildingManaPipe) {
                    // A bridge between the neighbouring cells of its type: along x when a neighbour in x shares the
                    // type, else along y, with a landing on each end.
                    StructureBuilder builder(*this, cellX * BlocksPerCell, cellY * BlocksPerCell, 0, BlocksPerCell,
                                             BlocksPerCell, 0, false, nullptr);
                    int level = m_world->structureLevel(cellX, cellY, 0);
                    if (m_world->zoneBuildingType(cellX - 1, cellY) == type ||
                        m_world->zoneBuildingType(cellX + 1, cellY) == type) {
                        // The mana pipe: landings and a tube between them on supports (140, 150, 180).
                        builder.setColor(plasma::Vector<3, float>(140.0f, 150.0f, 180.0f), TexturePlain);
                        int levelA = (std::max)(level, m_world->structureLevel(cellX - 1, cellY, 0));
                        builder.underpinnedBox(0, 5, levelA, 1, 6, 6, FillMaterial, CoverAll, false);
                        builder.box(0, 6, levelA + 6, 1, 4, 1, false);
                        int levelB = (std::max)(level, m_world->structureLevel(cellX + 1, cellY, 0));
                        builder.underpinnedBox(15, 5, levelB, 1, 6, 6, FillMaterial, CoverAll, false);
                        builder.box(15, 6, levelB + 6, 1, 4, 1, false);
                        builder.setColor(buildingColor(BuildingManaPipe, cellX, cellY, realmId, realmForm, biome),
                                         TexturePlain);
                        builder.tubeX(0, 7, level + 3, level + 3, 16, 2);
                    } else {
                        builder.setColor(plasma::Vector<3, float>(140.0f, 150.0f, 180.0f), TexturePlain);
                        int levelA = (std::max)(level, m_world->structureLevel(cellX, cellY - 1, 0));
                        builder.underpinnedBox(5, 0, levelA, 6, 1, 6, FillMaterial, CoverAll, false);
                        builder.box(6, 0, levelA + 6, 4, 1, 1, false);
                        int levelB = (std::max)(level, m_world->structureLevel(cellX, cellY + 1, 0));
                        builder.underpinnedBox(5, 15, levelB, 6, 1, 6, FillMaterial, CoverAll, false);
                        builder.box(6, 15, levelB + 6, 4, 1, 1, false);
                        builder.setColor(buildingColor(type, cellX, cellY, realmId, realmForm, biome), TexturePlain);
                        builder.tubeY(7, 0, level + 3, level + 3, 16, 2);
                    }
                } else if (type == BuildingTower) {
                    // A round tower: a cone of the town's size, storeys of rings with pillars or a fitted floor,
                    // balconies around the shaft, a crown or a dome on top.
                    int level = m_world->structureLevel(cellX, cellY, type) + 1;
                    StructureBuilder builder(*this, cellX * BlocksPerCell, cellY * BlocksPerCell, 0, BlocksPerCell,
                                             BlocksPerCell, 0, false, nullptr);
                    plasma::Vector<3, float> color =
                        buildingColor(BuildingTower, cellX, cellY, realmId, realmForm, biome);
                    plasma::Vector<3, float> trim =
                        buildingTrimColor(BuildingTower, cellX, cellY, realmId, realmForm, biome);
                    builder.setMaterial(trim, TextureBrick, 0.0f, color, TextureBrick, 20.0f, true);
                    int radius = m_world->townSizeB(cellX, cellY);
                    int height = m_world->townSizeA(cellX, cellY) - 2 * radius;
                    builder.foundation(BlocksPerCell / 2, BlocksPerCell / 2, level - 1, radius, height + 1,
                                       FillMaterial, CoverAll);
                    // The tower (a hand-authored shape): a crown of radius + 1 and six blocks, windows, the material
                    // blend over 20 blocks; one storey in four open or a gallery; floors of 10..21 blocks with an
                    // inlay of 150..249 per channel; four pillars on a 16-block grid or a central one of radius 3; a
                    // balcony spiralling (0.9 pi per storey from 1.55 pi); openings, a ground door and windows by the
                    // radius, an arched door for half the realms.
                    int top = height + level;
                    if (hasBattlements(BuildingTower, realmId, realmForm, biome)) {
                        builder.cylinder(BlocksPerCell / 2, BlocksPerCell / 2, top, radius + 1, 6, FillMaterial,
                                         CoverAll, 0);
                        builder.cylinderShaft(BlocksPerCell / 2, BlocksPerCell / 2, top + 2, radius - 1, 4, 0);
                        for (int k = -radius / 5; k <= radius / 5; k++) {
                            builder.cutBox(7 + 4 * k, 7 - radius, top + 4, 2, 2 * radius + 2, 2, false);
                            builder.cutBox(7 - radius, 7 + 4 * k, top + 4, 2 * radius + 2, 2, 2, false);
                        }
                    } else {
                        builder.cylinder(BlocksPerCell / 2, BlocksPerCell / 2, top, radius + 1, 4, FillMaterial,
                                         CoverAll, 0);
                        builder.cylinderShaft(BlocksPerCell / 2, BlocksPerCell / 2, top + 2, radius - 1, 2, 0);
                    }
                    if (hasRoof(52, realmId, realmForm, biome)) {
                        builder.setColor(roofColor(BuildingTower, cellX, cellY, realmId, realmForm, biome),
                                         TextureBrick);
                        builder.cone(8, 8, top, radius, 2 * radius);
                    }
                    int storeys = m_world->towerStoreyCount(cellX, cellY);
                    int storeyHeight = height / storeys;
                    builder.cylinderShaft(BlocksPerCell / 2, BlocksPerCell / 2, level, radius - 2,
                                          storeys * storeyHeight - 2, BlockFlagProtected);
                    builder.setMaterial(trim, TextureBrick, 0.0f, color, TextureBrick, 20.0f, true);
                    builder.m_fillType = BlockStorey0Floor;
                    builder.cylinder(BlocksPerCell / 2, BlocksPerCell / 2, level - 2, radius - 2, 2, FillMaterial,
                                     CoverPatches, 0);
                    builder.m_fillType = BlockBuilding;
                    for (int storey = 0; storey < storeys; storey++) {
                        int z = storey * storeyHeight + level;
                        int variant = 0;
                        if (rand() % 4 == 0) {
                            variant = rand() % 2 + 1;
                            if (variant == 1) {
                                builder.cylinderShaft(BlocksPerCell / 2, BlocksPerCell / 2, z, radius, storeyHeight - 2,
                                                      BlockFlagProtected);
                            }
                        }
                        int style = rand() % 3;
                        if (style == 0) {
                            // A fitted floor of three random colours.
                            int sizeX = rand() % 12 + 10;
                            int sizeY = rand() % 12 + 10;
                            plasma::Vector<3, float> floorColor((float)(rand() % 150 + 50), (float)(rand() % 150 + 50),
                                                                (float)(rand() % 150 + 50));
                            plasma::Vector<3, float> inlayColor(
                                (float)(rand() % 100 + 150), (float)(rand() % 100 + 150), (float)(rand() % 100 + 150));
                            builder.m_fillType = storey + BlockStorey0Floor;
                            builder.setColor(floorColor, TexturePlain);
                            builder.box(8 - sizeX / 2, 8 - sizeY / 2, z - 1, sizeX, sizeY, 1, false);
                            builder.setColor(inlayColor, TexturePlain);
                            builder.box(8 - sizeX / 2 + 1, 8 - sizeY / 2 + 1, z - 1, sizeX - 2, sizeY - 2, 1, false);
                            builder.setColor(floorColor, TexturePlain);
                            builder.box(8 - sizeX / 2 + 2, 8 - sizeY / 2 + 2, z - 1, sizeX - 4, sizeY - 4, 1, false);
                            builder.m_fillType = BlockBuilding;
                        } else if (style == 1) {
                            // Four pillars of one of three styles.
                            builder.setMaterial(trim, TextureBrick, 0.0f, color, TextureBrick, 20.0f, true);
                            HallPillarStyle pillarStyle = static_cast<HallPillarStyle>(rand() % 3);
                            for (int px = 0; px < 32; px += 16) {
                                for (int py = 0; py < 32; py += 16) {
                                    int x = (px / 16 * radius) / 2 - radius / 4 + 8;
                                    int y = (py / 16 * radius) / 2 - radius / 4 + 8;
                                    switch (pillarStyle) {
                                    case HallPillarStyle_AngularPillar:
                                        builder.angularPillar(x, y, z, 1, storeyHeight - 2, false);
                                        break;
                                    case HallPillarStyle_RoundPillar:
                                        builder.roundPillar(px, py, z, 1, storeyHeight - 2, false);
                                        break;
                                    case HallPillarStyle_AngularColumn:
                                        builder.angularColumn(x, y, z, 1, storeyHeight - 2, false);
                                        break;
                                    }
                                }
                            }
                        } else if (style == 2) {
                            // One wide pillar in the middle.
                            builder.setMaterial(trim, TextureBrick, 0.0f, color, TextureBrick, 20.0f, true);
                            HallPillarStyle pillarStyle = static_cast<HallPillarStyle>(rand() % 3);
                            switch (pillarStyle) {
                            case HallPillarStyle_AngularPillar:
                                builder.angularPillar(BlocksPerCell / 2, BlocksPerCell / 2, z, 3, storeyHeight - 2,
                                                      false);
                                break;
                            case HallPillarStyle_RoundPillar:
                                builder.roundPillar(BlocksPerCell / 2, BlocksPerCell / 2, z, 3, storeyHeight - 2,
                                                    false);
                                break;
                            case HallPillarStyle_AngularColumn:
                                builder.angularColumn(BlocksPerCell / 2, BlocksPerCell / 2, z, 3, storeyHeight - 2,
                                                      false);
                                break;
                            }
                        }
                        if (storey < storeys - 1) {
                            // The ceiling and a spiral of balconies up to the next storey.
                            builder.setMaterial(trim, TextureBrick, 0.0f, color, TextureBrick, 20.0f, true);
                            int count = storeyHeight - 1;
                            builder.m_guard = PlacementOnlyProtected;
                            builder.m_fillType = storey + BlockStorey1Floor;
                            builder.cylinder(BlocksPerCell / 2, BlocksPerCell / 2, z + storeyHeight - 2, radius - 2, 2,
                                             FillMaterial, CoverAll, 2);
                            builder.m_fillType = BlockBuilding;
                            for (int k = 0; k < count; k++) {
                                float angle = (float)(k * Pi / count * 0.9f + 1.55f * Pi + storey * Pi);
                                int zz = z + 1 + k;
                                float sine = sinf(angle);
                                float cosine = cosf(angle);
                                builder.box((int)((float)(radius - 4) * cosine + 8.0f - 3.0f),
                                            (int)((float)(radius - 4) * sine + 8.0f - 3.0f), zz - 3, 6, 6, 2, false);
                                if (k > count - 8) {
                                    builder.cutBox((int)(((float)radius - 4.5f) * cosine + 8.0f - 3.0f),
                                                   (int)(((float)radius - 4.5f) * sine + 8.0f - 3.0f), zz, 6, 6, 16,
                                                   false);
                                }
                            }
                            builder.m_guard = 0;
                        }
                        if (variant == 2) {
                            // A gallery: a disc with four pairs of posts.
                            builder.setMaterial(trim, TextureBrick, 0.0f, color, TextureBrick, 20.0f, true);
                            if (storey > 0) {
                                builder.m_guard = PlacementKeepSolid | PlacementKeepProtected;
                                builder.cylinder(BlocksPerCell / 2, BlocksPerCell / 2, z - 2, radius + 4, 2,
                                                 FillMaterial, CoverAll, 2);
                                builder.m_guard = 0;
                            }
                            builder.cutBox(8 - radius, 5, z, 2, 6, 6, false);
                            builder.cutBox(8 - radius, 6, z + 6, 2, 4, 1, false);
                            builder.cutBox(radius + 6, 5, z, 2, 6, 6, false);
                            builder.cutBox(radius + 6, 6, z + 6, 2, 4, 1, false);
                            builder.cutBox(5, 8 - radius, z, 6, 2, 6, false);
                            builder.cutBox(6, 8 - radius, z + 6, 4, 2, 1, false);
                            builder.cutBox(5, radius + 6, z, 6, 2, 6, false);
                            builder.cutBox(6, radius + 6, z + 6, 4, 2, 1, false);
                        } else if (variant == 1) {
                            // An open storey: two discs and eight posts.
                            builder.setMaterial(trim, TextureBrick, 0.0f, color, TextureBrick, 20.0f, true);
                            builder.m_guard = PlacementKeepProtected;
                            builder.cylinder(BlocksPerCell / 2, BlocksPerCell / 2, z - 2, radius + 3, 2, FillMaterial,
                                             CoverAll, 2);
                            builder.cylinder(BlocksPerCell / 2, BlocksPerCell / 2, z + storeyHeight - 2, radius + 3, 2,
                                             FillMaterial, CoverAll, 2);
                            builder.m_guard = 0;
                            float postRadius = (float)(radius + 1);
                            for (int i = 0; i < 8; i++) {
                                float angle = (float)(i * Pi * 0.25 + 0.7853981633974483);
                                builder.roundPillar((int)(cosf(angle) * postRadius + 8.0f),
                                                    (int)(sinf(angle) * postRadius + 8.0f), z, 2, storeyHeight - 2,
                                                    false);
                            }
                        } else {
                            // A closed storey: a wall with a door, and a lamp beside it for half of the realms.
                            if (storey == 0) {
                                builder.cutBox(8 - radius, 6, z, 2, 4, 6, false);
                                if (hash(realmId + 0x129b4) & 1) {
                                    builder.cutBox(8 - radius, 7, z + 6, 2, 2, 1, false);
                                }
                            } else {
                                builder.cutBox(8 - radius, 7, z + 1, 2, 2, 4, false);
                                StaticEntity towerWindow;
                                towerWindow.position =
                                    plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell - radius + 9),
                                                             Fixed(cellY * BlocksPerCell + 8), Fixed(z + 1));
                                towerWindow.orientation = QuarterTurn_One;
                                towerWindow.type = StaticEntityCastleWindow;
                                // The window starts settled (a second into its transition).
                                towerWindow.transitionTime = 1000.0f;
                                addStaticEntity(towerWindow);
                            }
                            builder.cutBox(radius + 6, 7, z + 1, 2, 2, 4, false);
                            StaticEntity oppositeTowerWindow;
                            oppositeTowerWindow.position =
                                plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + radius + 7),
                                                         Fixed(cellY * BlocksPerCell + 8), Fixed(z + 1));
                            oppositeTowerWindow.orientation = QuarterTurn_Three;
                            oppositeTowerWindow.type = StaticEntityCastleWindow;
                            oppositeTowerWindow.transitionTime = 1000.0f;
                            addStaticEntity(oppositeTowerWindow);
                            builder.cutBox(7, 8 - radius, z + 1, 2, 2, 4, false);
                            oppositeTowerWindow.position =
                                plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 8),
                                                         Fixed(cellY * BlocksPerCell - radius + 9), Fixed(z + 1));
                            oppositeTowerWindow.orientation = QuarterTurn_Zero;
                            addStaticEntity(oppositeTowerWindow);
                            builder.cutBox(7, radius + 6, z + 1, 2, 2, 4, false);
                            oppositeTowerWindow.position =
                                plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 8),
                                                         Fixed(cellY * BlocksPerCell + radius + 7), Fixed(z + 1));
                            oppositeTowerWindow.orientation = QuarterTurn_Zero;
                            addStaticEntity(oppositeTowerWindow);
                        }
                    }
                } else if (type == BuildingTerrace) {
                    // A terrace joined by ramps to the neighbouring cells of its type that lie higher. The terrace:
                    // grey-blue brick (180, 200, 200), cleared 20 blocks up.
                    int level = m_world->structureLevel(cellX, cellY, type);
                    StructureBuilder builder(*this, cellX * BlocksPerCell, cellY * BlocksPerCell, 0, BlocksPerCell,
                                             BlocksPerCell, 0, false, nullptr);
                    builder.setMaterial(plasma::Vector<3, float>(180.0f, 200.0f, 200.0f), TextureBrick, 0.0f,
                                        plasma::Vector<3, float>(180.0f, 200.0f, 200.0f), TextureBrick, 20.0f, true);
                    builder.underpinnedBox(0, 0, level - 1, BlocksPerCell, BlocksPerCell, 1, FillMaterial, CoverAll,
                                           false);
                    builder.cutBox(0, 0, level, BlocksPerCell, BlocksPerCell, 10, false);
                    int neighbour = m_world->zoneBuildingType(cellX - 1, cellY);
                    int neighbourLevel = m_world->structureLevel(cellX - 1, cellY, neighbour);
                    if (neighbourLevel > level && neighbour == type) {
                        builder.stairsX(0, 0, level, neighbourLevel - level, 16, neighbourLevel - level, 0, 0, 0, true);
                    }
                    neighbour = m_world->zoneBuildingType(cellX + 1, cellY);
                    neighbourLevel = m_world->structureLevel(cellX + 1, cellY, neighbour);
                    if (neighbourLevel > level && neighbour == BuildingTerrace) {
                        builder.stairsX(level - neighbourLevel + 16, 0, level, neighbourLevel - level, 16, 0,
                                        neighbourLevel - level, 0, 0, true);
                    }
                    neighbour = m_world->zoneBuildingType(cellX, cellY - 1);
                    neighbourLevel = m_world->structureLevel(cellX, cellY - 1, neighbour);
                    if (neighbourLevel > level && neighbour == BuildingTerrace) {
                        builder.stairsY(0, 0, level, 16, neighbourLevel - level, neighbourLevel - level, 0, 0, 0, true);
                    }
                    neighbour = m_world->zoneBuildingType(cellX, cellY + 1);
                    neighbourLevel = m_world->structureLevel(cellX, cellY + 1, neighbour);
                    if (neighbourLevel > level && neighbour == BuildingTerrace) {
                        builder.stairsY(0, level - neighbourLevel + 16, level, 16, neighbourLevel - level, 0,
                                        neighbourLevel - level, 0, 0, true);
                    }
                }
            }
            bool storeyed = buildingStoreyHeight(type) > 0;
            if (type == BuildingCastleGate && info.type == PlaceCastle) {
                m_requiredActivators[{{cellX, cellY}}] = m_world->notchWidth(placeX, placeY);
            }
            if (cell.kind == StructureCell_None && !storeyed) {
                if (isField(type)) {
                    // A field: a fence of posts along every side whose neighbour is open, the inner bounds shrunk by
                    // one per fence.
                    int innerMinX = cellX * BlocksPerCell;
                    int innerMaxX = cellX * BlocksPerCell + 16;
                    // Fences on three in four of the joined sides, posts where the street is weak (0.45 or less), set
                    // a hundredth and half a block in.
                    int innerMinY = cellY * BlocksPerCell;
                    int innerMaxY = cellY * BlocksPerCell + 16;
                    if (rand() % 4 != 0 && hasFence(type, m_world->zoneBuildingType(cellX, cellY - 1))) {
                        innerMinY++;
                        rand();
                        for (int i = 0; i < 8; i++) {
                            int x = cellX * BlocksPerCell + 1 + 2 * i;
                            int y = cellY * BlocksPerCell;
                            float distance = m_world->placeSquaredDistance(info.center, x, y);
                            float influence = m_world->placeInfluence(info.type, info.large, distance);
                            if (m_world->streetAmount(x, y, info.type, influence) <= 0.45f) {
                                float heightA = m_world->height(x - 1, y, 0);
                                float heightB = m_world->height(x, y, 0);
                                int z = (int)((std::max)(heightA, heightB) + 1.0f);
                                StaticEntity post;
                                int choice = rand();
                                if (type == BuildingGraveyard) {
                                    post.type = choice % 3 + StaticEntityFenceIron1;
                                } else {
                                    post.type = choice % 4 + StaticEntityFence01;
                                }
                                post.position = plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z)) +
                                                plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.01f, 0.51f, 0.0f));
                                post.orientation = QuarterTurn_Zero;
                                if (cubeAt(post.position[0], post.position[1], post.position[2]).type != BlockWater) {
                                    m_staticEntities.push_back(post);
                                }
                            }
                        }
                    }
                    if (rand() % 4 != 0 && hasFence(type, m_world->zoneBuildingType(cellX - 1, cellY))) {
                        innerMinX++;
                        rand();
                        for (int i = 0; i < 8; i++) {
                            int x = cellX * BlocksPerCell;
                            int y = cellY * BlocksPerCell + 1 + 2 * i;
                            float distance = m_world->placeSquaredDistance(info.center, x, y);
                            float influence = m_world->placeInfluence(info.type, info.large, distance);
                            if (m_world->streetAmount(x, y, info.type, influence) <= 0.45f) {
                                float heightA = m_world->height(x, y - 1, 0);
                                float heightB = m_world->height(x, y, 0);
                                int z = (int)((std::max)(heightA, heightB) + 1.0f);
                                StaticEntity post;
                                int choice = rand();
                                if (type == BuildingGraveyard) {
                                    post.type = choice % 3 + 71;
                                } else {
                                    post.type = choice % 4 + 67;
                                }
                                post.position = plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z)) +
                                                plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.51f, 0.01f, 0.0f));
                                post.orientation = QuarterTurn_One;
                                if (cubeAt(post.position[0], post.position[1], post.position[2]).type != BlockWater) {
                                    m_staticEntities.push_back(post);
                                }
                            }
                        }
                    }
                    if (rand() % 4 != 0 && hasFence(type, m_world->zoneBuildingType(cellX, cellY + 1))) {
                        innerMaxY--;
                        rand();
                        for (int i = 0; i < 8; i++) {
                            int x = cellX * BlocksPerCell + 1 + 2 * i;
                            int y = cellY * BlocksPerCell + (BlocksPerCell - 1);
                            float distance = m_world->placeSquaredDistance(info.center, x, y);
                            float influence = m_world->placeInfluence(info.type, info.large, distance);
                            if (m_world->streetAmount(x, y, info.type, influence) <= 0.45f) {
                                float heightA = m_world->height(x - 1, y, 0);
                                float heightB = m_world->height(x, y, 0);
                                int z = (int)((std::max)(heightA, heightB) + 1.0f);
                                StaticEntity post;
                                int choice = rand();
                                if (type == BuildingGraveyard) {
                                    post.type = choice % 3 + 71;
                                } else {
                                    post.type = choice % 4 + 67;
                                }
                                post.position = plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z)) +
                                                plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.01f, 0.51f, 0.0f));
                                post.orientation = QuarterTurn_Zero;
                                if (cubeAt(post.position[0], post.position[1], post.position[2]).type != BlockWater) {
                                    m_staticEntities.push_back(post);
                                }
                            }
                        }
                    }
                    if (rand() % 4 != 0 && hasFence(type, m_world->zoneBuildingType(cellX + 1, cellY))) {
                        innerMaxX--;
                        rand();
                        for (int i = 0; i < 8; i++) {
                            int x = cellX * BlocksPerCell + (BlocksPerCell - 1);
                            int y = cellY * BlocksPerCell + 1 + 2 * i;
                            float distance = m_world->placeSquaredDistance(info.center, x, y);
                            float influence = m_world->placeInfluence(info.type, info.large, distance);
                            if (m_world->streetAmount(x, y, info.type, influence) <= 0.45f) {
                                float heightA = m_world->height(x, y - 1, 0);
                                float heightB = m_world->height(x, y, 0);
                                int z = (int)((std::max)(heightA, heightB) + 1.0f);
                                StaticEntity post;
                                int choice = rand();
                                if (type == BuildingGraveyard) {
                                    post.type = choice % 3 + 71;
                                } else {
                                    post.type = choice % 4 + 67;
                                }
                                post.position = plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z)) +
                                                plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.51f, 0.01f, 0.0f));
                                post.orientation = QuarterTurn_One;
                                if (cubeAt(post.position[0], post.position[1], post.position[2]).type != BlockWater) {
                                    m_staticEntities.push_back(post);
                                }
                            }
                        }
                    }
                    if (type == BuildingGraveyard) {
                        // A paddock: at the centre of each quarter a type 127..129 entity or, for a quarter of them, a
                        // type 81 plant turned at random. The graveyard: a prop for three in four of the eight-block
                        // sites (a skull, scale 0.07), where the street is weak (0.45 or less).
                        for (int i = 0; i < 2; i++) {
                            for (int j = 0; j < 2; j++) {
                                if (rand() % 4 != 0) {
                                    int x = (i + 2 * cellX) * 8 + 4;
                                    int y = (j + 2 * cellY) * 8 + 4;
                                    float distance = m_world->placeSquaredDistance(info.center, x, y);
                                    float influence = m_world->placeInfluence(info.type, info.large, distance);
                                    if (m_world->streetAmount(x, y, info.type, influence) <= 0.45f) {
                                        int z = (int)(m_world->height(x, y, 0) + 1.0f);
                                        if (rand() % 4 != 0) {
                                            StaticEntity entity;
                                            entity.type = rand() % 3 + StaticEntityGraveStone1;
                                            entity.orientation = QuarterTurn_Zero;
                                            entity.position[0] = (Fixed(x) + 0.5);
                                            entity.position[1] = (Fixed(y) + 0.5);
                                            entity.position[2] = z;
                                            m_staticEntities.push_back(entity);
                                        } else {
                                            Plant plant;
                                            plant.position[0] = (Fixed(x) + 0.5);
                                            plant.position[1] = (Fixed(y) + 0.5);
                                            plant.position[2] = z;
                                            plant.type = PlantSkullRotten;
                                            plant.rotation = (float)rand() * 360.0f / (float)RAND_MAX;
                                            plant.scale = 0.07f;
                                            m_plants.push_back(plant);
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        // The crops: every soft column gets a ground block at the terrain height, shaded in stripes
                        // two blocks wide (along x or y by the cell); inside the fences, on an empty block, a column
                        // gets a position for the cell's list (kind 21), one of the crop plants of the field type, or
                        // (one even column in 120 of a type 27 field) a type 116 entity.
                        for (int x = cellX * BlocksPerCell; x < cellX * BlocksPerCell + 16; x++) {
                            Sleep(0);
                            for (int y = cellY * BlocksPerCell; y < cellY * BlocksPerCell + 16; y++) {
                                float distance = m_world->placeSquaredDistance(info.center, x, y);
                                float influence = m_world->placeInfluence(info.type, info.large, distance);
                                if (m_world->streetAmount(x, y, info.type, influence) > 0.5f) {
                                    continue;
                                    // The crops: soil stripes (75%, 80%) turned between the 3 x 3-cell plots, crop
                                    // sprites of scale 0.1 (0.2 for the tall ones), a cornflower for one wheat in ten,
                                    // a crop for one position in four.
                                }
                                int z = (int)m_world->height(x, y, 0);
                                int stripe = ((cellX / 3 + cellY / 3) & 1 ? x : y) / 2 % 2;
                                float shade = (float)stripe * 0.05f + 0.75f;
                                Block ground;
                                ground =
                                    BlockProperties<float>(m_world->groundColor(x, y, z) * shade, BlockGround, 0, 0);
                                setBlock(x, y, z, ground);
                                if (x < innerMinX || y < innerMinY || x >= innerMaxX || y >= innerMaxY) {
                                    continue;
                                }
                                z++;
                                if (getBlock(x, y, z).type != BlockAir) {
                                    continue;
                                }
                                int crop = PlantFlowers;
                                int cropPlantFlags = 0;
                                float cropSize = 0.1f;
                                int entityType = -1;
                                switch (type) {
                                case BuildingPumpkinField:
                                    crop = PlantPumpkinLeaves;
                                    entityType = StaticEntityBigPumpkin;
                                    break;
                                case BuildingSunflowerField:
                                    crop = PlantSunflower;
                                    cropSize = 0.2f;
                                    cropPlantFlags = PlantFlagSway;
                                    break;
                                case BuildingBeanField:
                                    crop = PlantBeanTendril;
                                    cropSize = 0.2f;
                                    break;
                                case BuildingWheatField:
                                    if (rand() % 10 == 0) {
                                        crop = PlantCornflower;
                                        cropSize = 0.05f;
                                    } else {
                                        crop = PlantWheat;
                                        cropSize = 0.2f;
                                    }
                                    cropPlantFlags = PlantFlagSway;
                                    break;
                                case BuildingCornField:
                                    crop = PlantCorn;
                                    cropSize = 0.2f;
                                    cropPlantFlags = PlantFlagSway;
                                    break;
                                case BuildingPineappleField:
                                    crop = PlantPineappleLeaves;
                                    cropSize = 0.15f;
                                    break;
                                }
                                if (rand() % 4 == 0) {
                                    m_positionsByKind[PositionKey {{cellX, cellY, PositionCrop}}].push_back(
                                        plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z)) +
                                        plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.5f, 0.5f, 0.0f)));
                                } else if (rand() % 10 == 0) {
                                    if (rand() % 12 == 0 && x % 2 == 0 && y % 2 == 0 && entityType != -1) {
                                        StaticEntity entity;
                                        entity.type = entityType;
                                        entity.position =
                                            plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z)) +
                                            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.5f, 0.5f, 0.0f));
                                        entity.orientation = (unsigned char)(rand() % 4);
                                        m_staticEntities.push_back(entity);
                                    } else if (crop != 0) {
                                        Plant plant;
                                        plant.type = crop;
                                        plant.flags = cropPlantFlags;
                                        if (crop == PlantSunflower) {
                                            plant.rotation = 0.0f;
                                        } else {
                                            plant.rotation = (float)rand() * 360.0f / (float)RAND_MAX;
                                        }
                                        plant.position =
                                            plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z)) +
                                            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.5f, 0.5f, 0.0f));
                                        plant.scale = cropSize;
                                        m_plants.push_back(plant);
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                // A house (a cell with a structure cell or with storeys): buildingHeight(type) blocks high, its
                // storeys buildingStoreyHeight(type) blocks each.
                int level = m_world->structureLevel(cellX, cellY, type);
                int height = buildingHeight(type);
                if (!hasRoof(type, realmId, realmForm, biome)) {
                    height += 2;
                }
                StructureBuilder builder(*this, cellX * BlocksPerCell, cellY * BlocksPerCell, 0, BlocksPerCell,
                                         BlocksPerCell, 0, false, nullptr);
                if (cellDx >= 0) {
                    if (hasTowerBand(type, realmId, realmForm, biome)) {
                        // The building only reshapes the terrain, ten blocks deep.
                        builder.m_terrainOnly = 10;
                    }
                }
                plasma::Vector<3, float> color = buildingColor(type, cellX, cellY, realmId, realmForm, biome);
                plasma::Vector<3, float> trim = buildingTrimColor(type, cellX, cellY, realmId, realmForm, biome);
                int kind = buildingKind(type);
                int style = -1;
                if (kind == BuildingKindHouse) {
                    style = houseStyle(realmId, realmForm, biome);
                }
                bool roofed = (kind == BuildingKindHouse && style == HouseStyle_DarkSteepRoof) ||
                              kind == BuildingKindStonePavilion || kind == BuildingKindCrypt ||
                              kind == BuildingKindDungeonShaftEntrance || kind == BuildingKindTempleEntrance;
                // How far the battlements overhang the walls.
                int overhang = 1;
                switch (kind) {
                case BuildingKindHouse:
                    switch (style) {
                    case HouseStyle_MountainSlabRoof:
                    case HouseStyle_Timber:
                    case HouseStyle_DarkSteepRoof:
                        overhang = 2;
                        break;
                    case HouseStyle_DesertFlatRoof:
                        overhang = 0;
                        break;
                    case HouseStyle_SavannahLowRoof:
                        overhang = 4;
                        break;
                    }
                    break;
                case BuildingKindStonePavilion:
                case BuildingKindDungeonShaftEntrance:
                case BuildingKindTempleEntrance:
                    overhang = 4;
                    break;
                case BuildingKindCrypt:
                case BuildingKindGuildHall:
                    overhang = 2;
                    break;
                    // The walls' material: the trim blending into the wall colour (50 lighter for pavilions and
                    // timber) over ten blocks, 25 for houses; lighter walls 30 brighter; posts raised three blocks,
                    // inset two; frames 75% as dark.
                }
                if (cellDx >= 0 && type == BuildingHousePavilion) {
                    builder.setMaterial(trim, TexturePlain, 0.0f, color + plasma::Vector<3, float>(50.0f, 50.0f, 50.0f),
                                        TexturePlain, 10.0f, true);
                } else if (cellDx >= 0 && (kind == BuildingKindTimberMineEntrance ||
                                           style == HouseStyle_PaintedTimber || style == HouseStyle_Timber)) {
                    builder.setMaterial(trim, TextureWoodZ, 0.0f, color, TextureWoodZ, 10.0f, true);
                } else if (cellDx >= 0 && style == HouseStyle_SavannahLowRoof && type != BuildingHouseCourtyard) {
                    builder.setMaterial(trim, TextureWood, 0.0f, color, TextureWood, 10.0f, true);
                } else if (cellDx >= 0 && kind == BuildingKindOldHut) {
                    builder.setMaterial(trim * plasma::Vector<3, float>(0.8f, 1.0f, 0.8f), TextureBrick, 0.0f, color,
                                        TextureBrick, 10.0f, true);
                } else if (cellDx >= 0 && kind == BuildingKindCastle) {
                    builder.setMaterial(trim, TextureBrick, 0.0f, color, TextureBrick, 25.0f, true);
                } else {
                    builder.setMaterial(trim, TextureBrick, 0.0f, color, TextureBrick, 10.0f, true);
                }
                StructureBuilder::Material wall = builder.m_material;
                StructureBuilder::Material light = wall;
                light.color0 += plasma::Vector<3, float>(30.0f, 30.0f, 30.0f);
                light.color1 += plasma::Vector<3, float>(30.0f, 30.0f, 30.0f);
                if (storeyed) {
                    if (hasPosts(type)) {
                        // A yard: a post at each free corner, two blocks under the level, and a floor of a random
                        // type.
                        builder.setColor(color, TexturePlain);
                        int postZ = level - 2;
                        if (type == BuildingLakePlatform || type == BuildingRaisedPostPlatform) {
                            postZ += 3;
                        }
                        bool yardEast = hasPosts(m_world->zoneBuildingType(cellX + 1, cellY));
                        bool yardNorth = hasPosts(m_world->zoneBuildingType(cellX, cellY + 1));
                        builder.underpinnedBox(0, 0, postZ, 2, 2, 1, FillMaterial, CoverAll, false);
                        if (!yardEast) {
                            builder.underpinnedBox(14, 0, postZ, 2, 2, 1, FillMaterial, CoverAll, false);
                        }
                        if (!yardNorth) {
                            builder.underpinnedBox(0, 14, postZ, 2, 2, 1, FillMaterial, CoverAll, false);
                        }
                        if (!yardEast && !yardNorth) {
                            builder.underpinnedBox(14, 14, postZ, 2, 2, 1, FillMaterial, CoverAll, false);
                        }
                        builder.setColor(color, rand() % 2 + TextureWoodX);
                        builder.m_fillType = BlockStorey0Floor;
                        builder.box(0, 0, level - 1, BlocksPerCell, BlocksPerCell, 1, true);
                    } else if (kind == BuildingKindStonePavilion || kind == BuildingKindDungeonShaftEntrance ||
                               kind == BuildingKindTempleEntrance || type == BuildingTreasurePavilion ||
                               type == BuildingHouseCourtyard || type == BuildingRuinedFlatRoofPavilion ||
                               type == BuildingBirdStatuePavilion || type == BuildingInscribedMenhir ||
                               type == BuildingAltar || type == BuildingManaWell || type == BuildingShrineOfFinesse ||
                               type == BuildingMourningStatuePavilion) {
                        builder.underpinnedBox(0, 0, level - 1, BlocksPerCell, BlocksPerCell, 1, FillMaterial, CoverAll,
                                               false);
                    } else {
                        // A foundation down to the lowest street level of the neighbouring houses.
                        int bottom = level + m_world->streetHeightOffset(cellX, cellY, 0);
                        if (hasStoreys(m_world->zoneBuildingType(cellX - 1, cellY))) {
                            bottom = (std::min)(bottom, m_world->structureLevel(cellX - 1, cellY, 0) +
                                                            m_world->streetHeightOffset(cellX - 1, cellY, 0));
                        }
                        if (hasStoreys(m_world->zoneBuildingType(cellX + 1, cellY))) {
                            bottom = (std::min)(bottom, m_world->structureLevel(cellX + 1, cellY, 0) +
                                                            m_world->streetHeightOffset(cellX + 1, cellY, 0));
                        }
                        if (hasStoreys(m_world->zoneBuildingType(cellX, cellY - 1))) {
                            bottom = (std::min)(bottom, m_world->structureLevel(cellX, cellY - 1, 0) +
                                                            m_world->streetHeightOffset(cellX, cellY - 1, 0));
                        }
                        if (hasStoreys(m_world->zoneBuildingType(cellX, cellY + 1))) {
                            bottom = (std::min)(bottom, m_world->structureLevel(cellX, cellY + 1, 0) +
                                                            m_world->streetHeightOffset(cellX, cellY + 1, 0));
                        }
                        builder.underpinnedBox(0, 0, bottom, BlocksPerCell, BlocksPerCell, height - bottom + level,
                                               FillMaterial, CoverAll, false);
                    }
                }
                StructureBuilder::Material frameMaterial = wall;
                frameMaterial.color0 *= 0.75f;
                frameMaterial.color1 *= 0.75f;
                frameMaterial.color2 *= 0.75f;
                bool hasWindowBoxes = kind == BuildingKindHouse && style != HouseStyle_DarkSteepRoof &&
                                      style != HouseStyle_SavannahLowRoof;
                int windowEntityType = StaticEntityWindow;
                int wallStep = 2;
                plasma::Vector<2, int> dungeon(NoCoordinate, NoCoordinate);
                if (dungeonType != PlaceNone) {
                    dungeon = m_world->dungeonPosition(placeX, placeY, info.type, info.large);
                }
                int plot = m_world->hamletPlot(cellX, cellY, info.type, dungeon);
                int shopSignPlantType = -1;
                int entranceEntityType = (style == HouseStyle_DarkSteepRoof || kind == BuildingKindCrypt)
                                             ? StaticEntityBigIronDoor
                                             : StaticEntityBigDoor;
                switch (plot) {
                case PlotInn:
                    shopSignPlantType = PlantInnSign;
                    break;
                case PlotItemShop:
                    shopSignPlantType = PlantShopSign;
                    break;
                case PlotArmorShop:
                    shopSignPlantType = PlantArmorShopSign;
                    break;
                case PlotWeaponShop:
                    shopSignPlantType = PlantWeaponShopSign;
                    break;
                case PlotIdentifiersShop:
                    shopSignPlantType = PlantIdentifierSign;
                    break;
                case PlotSmithy:
                    shopSignPlantType = PlantSmithySign;
                    break;
                case PlotCarpentersShop:
                    shopSignPlantType = PlantCarpentersshopSign;
                    break;
                case PlotTailorsShop:
                    shopSignPlantType = PlantWeavingmillSign;
                    break;
                }
                // The openings: walls every four blocks, sandstone entrances six wide.
                if ((kind >= BuildingKindStonePavilion && kind <= BuildingKindDungeonShaftEntrance) ||
                    kind == BuildingKindTempleEntrance || style == HouseStyle_DarkSteepRoof) {
                    windowEntityType = StaticEntityCastleWindow;
                    wallStep = 4;
                }
                int windowStep = 4;
                if (kind == BuildingKindCastle) {
                    if (halfChanceA(placeX, placeY)) {
                        entranceEntityType = missions.chooseRoomAt(placeX, placeY);
                    } else {
                        entranceEntityType = StaticEntityGate;
                    }
                    windowStep = 10;
                } else if (kind == BuildingKindCrypt) {
                    windowStep = 6;
                    if (rand() % 4 == 0 && missions.kindAt(placeX, placeY) != MissionKindBellPlacement &&
                        missions.kindAt(placeX, placeY) != MissionKindHarpPlacement) {
                        entranceEntityType = missions.specialRoomAt(cellX, cellY);
                    } else {
                        entranceEntityType = 5;
                    }
                } else if (kind == BuildingKindStonePavilion || kind == BuildingKindDungeonShaftEntrance ||
                           kind == BuildingKindGuildHall || kind == BuildingKindTempleEntrance) {
                    windowStep = 6;
                    entranceEntityType = StaticEntityNone;
                }
                if (type == BuildingMineEntrance || type == BuildingVaultEntrance) {
                    windowStep = 6;
                    entranceEntityType = StaticEntityNone;
                    if (halfChanceB(placeX, placeY)) {
                        entranceEntityType = missions.specialRoomAt(cellX, cellY);
                    }
                } else if (type == BuildingSandstoneTreasureChamber) {
                    windowStep = 6;
                    entranceEntityType = missions.specialRoomAt(cellX, cellY);
                }
                bool domed = false;
                if (style == HouseStyle_PlainFrame || style == HouseStyle_PaintedTimber ||
                    kind == BuildingKindTimberMineEntrance || kind == BuildingKindOldHut) {
                    if (hasRoof(type, realmId, realmForm, biome)) {
                        // The roof: a floor slab per storey in the roof's material, and the corner posts of the walls.
                        StructureBuilder::Material saved = builder.m_material;
                        domed = true;
                        if (style == HouseStyle_PlainFrame) {
                            builder.setColor(styleColor(type, realmId, realmForm, biome), TexturePlain);
                        } else if (style == HouseStyle_PaintedTimber) {
                            builder.setMaterial(0.5f * plasma::Vector<3, float>(255.0f, 255.0f, 255.0f) + 0.5f * trim,
                                                TexturePlain, 0.0f,
                                                0.5f * plasma::Vector<3, float>(255.0f, 255.0f, 255.0f) + 0.5f * color,
                                                // The old hut's roof: dark brown (60, 50, 30).
                                                TexturePlain, 0.0f, false);
                        } else if (kind == BuildingKindOldHut) {
                            builder.setColor(plasma::Vector<3, float>(60.0f, 50.0f, 30.0f), TexturePlain);
                        } else {
                            builder.setColor(color * 0.85f, TexturePlain);
                        }
                        frameMaterial = builder.m_material;
                        int floorHeight = buildingStoreyHeight(type);
                        int floors = height / floorHeight;
                        for (int i = 0; i <= floors; i++) {
                            builder.box(0, 0, level - 1 + i * floorHeight, BlocksPerCell, BlocksPerCell, 1, false);
                        }
                        bool houseWest = isHouse(m_world->zoneBuildingType(cellX - 1, cellY));
                        bool houseSouth = isHouse(m_world->zoneBuildingType(cellX, cellY - 1));
                        bool houseEast = isHouse(m_world->zoneBuildingType(cellX + 1, cellY));
                        bool houseNorth = isHouse(m_world->zoneBuildingType(cellX, cellY + 1));
                        int postLevel = level;
                        if (houseWest) {
                            postLevel = (std::min)(level, m_world->structureLevel(cellX - 1, cellY, 0));
                        }
                        if (houseSouth) {
                            postLevel = (std::min)(level, m_world->structureLevel(cellX, cellY - 1, 0));
                        }
                        builder.box(0, 0, postLevel - 1, 1, 1, height - postLevel + level, false);
                        if (!houseEast) {
                            postLevel = level;
                            if (houseSouth) {
                                postLevel = (std::min)(level, m_world->structureLevel(cellX, cellY - 1, 0));
                            }
                            builder.box(15, 0, postLevel - 1, 1, 1, height - postLevel + level, false);
                        }
                        if (!houseNorth) {
                            postLevel = level;
                            if (houseWest) {
                                postLevel = (std::min)(level, m_world->structureLevel(cellX - 1, cellY, 0));
                            }
                            builder.box(0, 15, postLevel - 1, 1, 1, height - postLevel + level, false);
                        }
                        if (!houseEast && !houseNorth) {
                            builder.box(15, 15, level - 1, 1, 1, height, false);
                        }
                        builder.setMaterial(saved);
                    }
                }
                // Battlements on the walls that no joined castle part shares. The battlements: a three-block base
                // along each side, two blocks in, merlons above.
                if (!isJoinedCastle(type, m_world->zoneBuildingType(cellX - 1, cellY)) &&
                    hasBattlements(type, realmId, realmForm, biome)) {
                    builder.box(-overhang, -overhang, level + height - overhang, 2, 2 * overhang + 16, 3, false);
                    builder.battlementY(-overhang, -overhang, level + 3 + height - overhang, 2 * overhang + 16, 2, 2,
                                        0);
                }
                if (!isJoinedCastle(type, m_world->zoneBuildingType(cellX + 1, cellY)) &&
                    hasBattlements(type, realmId, realmForm, biome)) {
                    builder.box(overhang + 14, -overhang, level + height - overhang, 2, 2 * overhang + 16, 3, false);
                    builder.battlementY(overhang + 14, -overhang, level + 3 + height - overhang, 2 * overhang + 16, 2,
                                        2, 1);
                }
                if (!isJoinedCastle(type, m_world->zoneBuildingType(cellX, cellY - 1)) &&
                    hasBattlements(type, realmId, realmForm, biome)) {
                    builder.box(-overhang, -overhang, level + height - overhang, 2 * overhang + 16, 2, 3, false);
                    builder.battlementX(-overhang, -overhang, level + 3 + height - overhang, 2 * overhang + 16, 2, 2,
                                        0);
                }
                if (!isJoinedCastle(type, m_world->zoneBuildingType(cellX, cellY + 1)) &&
                    hasBattlements(type, realmId, realmForm, biome)) {
                    builder.box(-overhang, overhang + 14, level + height - overhang, 2 * overhang + 16, 2, 3, false);
                    builder.battlementX(-overhang, overhang + 14, level + 3 + height - overhang, 2 * overhang + 16, 2,
                                        2, 1);
                }
                bool walled = hasWalls(type);
                if (cell.kind != StructureCell_None || (hasStoreys(type) && buildingStoreyHeight(type) != 0)) {
                    // The storeys.
                    int storeyHeight = 0;
                    int storeys = 0;
                    if (buildingStoreyHeight(type) != 0) {
                        storeyHeight = buildingStoreyHeight(type);
                        storeys = buildingHeight(type) / storeyHeight;
                    }
                    int westType = m_world->zoneBuildingType(cellX - 1, cellY);
                    int eastType = m_world->zoneBuildingType(cellX + 1, cellY);
                    int southType = m_world->zoneBuildingType(cellX, cellY - 1);
                    int northType = m_world->zoneBuildingType(cellX, cellY + 1);
                    int westLevel = m_world->structureLevel(cellX - 1, cellY, 0);
                    int eastLevel = m_world->structureLevel(cellX + 1, cellY, 0);
                    int southLevel = m_world->structureLevel(cellX, cellY - 1, 0);
                    int northLevel = m_world->structureLevel(cellX, cellY + 1, 0);
                    int southWestLevel = m_world->structureLevel(cellX - 1, cellY - 1, 0);
                    int southEastLevel = m_world->structureLevel(cellX + 1, cellY - 1, 0);
                    int northEastLevel = m_world->structureLevel(cellX + 1, cellY + 1, 0);
                    int northWestLevel = m_world->structureLevel(cellX - 1, cellY + 1, 0);
                    int southWestType = m_world->zoneBuildingType(cellX - 1, cellY - 1);
                    int southEastType = m_world->zoneBuildingType(cellX + 1, cellY - 1);
                    int northWestType = m_world->zoneBuildingType(cellX - 1, cellY + 1);
                    int northEastType = m_world->zoneBuildingType(cellX + 1, cellY + 1);
                    bool doorSides = hasStreetDoor(type);
                    std::vector<int> openSides;
                    if (doorSides) {
                        if (!hasStoreys(westType)) {
                            openSides.push_back(1);
                        }
                        if (!hasStoreys(eastType)) {
                            openSides.push_back(2);
                        }
                        if (!hasStoreys(southType)) {
                            openSides.push_back(4);
                        }
                        if (!hasStoreys(northType)) {
                            openSides.push_back(8);
                        }
                        m_positionsByKind[PositionKey {{cellX, cellY, PositionBuildingCentre}}].push_back(
                            plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 8), Fixed(cellY * BlocksPerCell + 8),
                                                     Fixed(level + m_world->streetHeightOffset(cellX, cellY, 0))));
                    }
                    int doorSide = -1;
                    if (!openSides.empty()) {
                        doorSide = openSides[rand() % openSides.size()];
                    }
                    int entrance;
                    if (doorSide >= 0) {
                        entrance = doorSide;
                    } else {
                        switch (rand() % 4) {
                        case 0:
                            entrance = 1;
                            break;
                        case 1:
                            entrance = 2;
                            break;
                        case 2:
                            entrance = 4;
                            break;
                        case 3:
                            entrance = 8;
                            break;
                        }
                    }
                    for (int storey = cell.kind != StructureCell_None ? -1 : 0; storey < storeys; storey++) {
                        if (storey >= 0 && !m_world->isOccupiedStorey(cellX, cellY, storey)) {
                            continue;
                        }
                        int layer = m_world->structureLayer(cellX, cellY, storey, 0);
                        int floorZ = level + m_world->streetHeightOffset(cellX, cellY, storey) + storey * storeyHeight;
                        int cellarDepth = 0;
                        int westDepth = 0;
                        int eastDepth = 0;
                        int southDepth = 0;
                        int northDepth = 0;
                        plasma::Vector<3, float> wallColor;
                        plasma::Vector<3, float> trimColor;
                        if (storey < 0) {
                            floorZ = dungeonLevel;
                            cellarDepth = dungeonCellarDepth(cell);
                            wallColor = dungeonWallColor(cell, dungeonType);
                        } else {
                            wallColor = buildingColor(type, cellX, cellY, realmId, realmForm, biome);
                        }
                        if (storey < 0) {
                            trimColor = dungeonTrimColor(cell, dungeonType);
                        } else {
                            // The room: a floor colour of 50..199 per channel; a gallery for one room in four, 8..12
                            // blocks each way; house and mine openings 10 wide, connecting ones 14.
                            trimColor = buildingColor(type, cellX, cellY, realmId, realmForm, biome);
                        }
                        plasma::Vector<3, float> floorColor((float)(rand() % 150 + 50), (float)(rand() % 150 + 50),
                                                            (float)(rand() % 150 + 50));
                        plasma::Vector<3, float> inlayColor((float)(rand() % 100 + 150), (float)(rand() % 100 + 150),
                                                            (float)(rand() % 100 + 150));
                        bool gallery = storey >= 0 && storeys == 1 &&
                                       (kind == BuildingKindHouse || kind == BuildingKindOldHut ||
                                        kind == BuildingKindGuildHall) &&
                                       type != BuildingHouseCourtyard && rand() % 4 == 0;
                        int roomX = rand() % 5 + 8;
                        int roomY = rand() % 5 + 8;
                        int wallTexture = 1;
                        if (storey < 0 && dungeonType == PlaceMines) {
                            wallTexture = 0;
                        }
                        bool occupiedWest = m_world->isOccupiedStorey(cellX - 1, cellY, storey);
                        bool occupiedEast = m_world->isOccupiedStorey(cellX + 1, cellY, storey);
                        bool occupiedSouth = m_world->isOccupiedStorey(cellX, cellY - 1, storey);
                        bool occupiedNorth = m_world->isOccupiedStorey(cellX, cellY + 1, storey);
                        bool occupiedSouthWest = m_world->isOccupiedStorey(cellX - 1, cellY - 1, storey);
                        bool occupiedSouthEast = m_world->isOccupiedStorey(cellX + 1, cellY - 1, storey);
                        bool occupiedNorthWest = m_world->isOccupiedStorey(cellX - 1, cellY + 1, storey);
                        bool occupiedNorthEast = m_world->isOccupiedStorey(cellX + 1, cellY + 1, storey);
                        int westFloor = westLevel + m_world->streetHeightOffset(cellX - 1, cellY, storey) +
                                        buildingStoreyHeight(westType) * storey;
                        int eastFloor = eastLevel + m_world->streetHeightOffset(cellX + 1, cellY, storey) +
                                        buildingStoreyHeight(eastType) * storey;
                        int southFloor = southLevel + m_world->streetHeightOffset(cellX, cellY - 1, storey) +
                                         buildingStoreyHeight(southType) * storey;
                        int northFloor = northLevel + m_world->streetHeightOffset(cellX, cellY + 1, storey) +
                                         buildingStoreyHeight(northType) * storey;
                        int southWestFloor = southWestLevel +
                                             m_world->streetHeightOffset(cellX - 1, cellY - 1, storey) +
                                             buildingStoreyHeight(southWestType) * storey;
                        int southEastFloor = southEastLevel +
                                             m_world->streetHeightOffset(cellX + 1, cellY - 1, storey) +
                                             buildingStoreyHeight(southEastType) * storey;
                        int northEastFloor = northEastLevel +
                                             m_world->streetHeightOffset(cellX + 1, cellY + 1, storey) +
                                             buildingStoreyHeight(northEastType) * storey;
                        int northWestFloor = northWestLevel +
                                             m_world->streetHeightOffset(cellX - 1, cellY + 1, storey) +
                                             buildingStoreyHeight(northWestType) * storey;
                        if (type == BuildingHousePavilion || kind == BuildingKindGuildHall) {
                            // A hall: joined to its four neighbours at their own levels.
                            occupiedWest = true;
                            occupiedEast = true;
                            occupiedSouth = true;
                            occupiedNorth = true;
                            int west = m_world->structureLevel(cellX - 1, cellY, type);
                            int east = m_world->structureLevel(cellX + 1, cellY, type);
                            int south = m_world->structureLevel(cellX, cellY - 1, type);
                            int north = m_world->structureLevel(cellX, cellY + 1, type);
                            westFloor = west + m_world->streetHeightOffset(cellX - 1, cellY, storey) +
                                        buildingStoreyHeight(westType) * storey;
                            eastFloor = east + m_world->streetHeightOffset(cellX + 1, cellY, storey) +
                                        buildingStoreyHeight(eastType) * storey;
                            southFloor = south + m_world->streetHeightOffset(cellX, cellY - 1, storey) +
                                         buildingStoreyHeight(southType) * storey;
                            northFloor = north + m_world->streetHeightOffset(cellX, cellY + 1, storey) +
                                         buildingStoreyHeight(northType) * storey;
                        }
                        int ceilingZ = level + (storey + 1) * storeyHeight;
                        int westCeiling = buildingStoreyHeight(westType) * (storey + 1) + westFloor;
                        int eastCeiling = buildingStoreyHeight(eastType) * (storey + 1) + eastFloor;
                        int southCeiling = buildingStoreyHeight(southType) * (storey + 1) + southFloor;
                        int northCeiling = buildingStoreyHeight(northType) * (storey + 1) + northFloor;
                        int southWestCeiling = buildingStoreyHeight(southWestType) * (storey + 1) + southWestFloor;
                        int southEastCeiling = buildingStoreyHeight(southEastType) * (storey + 1) + southEastFloor;
                        int northEastCeiling = buildingStoreyHeight(northEastType) * (storey + 1) + northEastFloor;
                        int northWestCeiling = buildingStoreyHeight(northWestType) * (storey + 1) + northWestFloor;
                        int baseZ = floorZ;
                        int westBase = westFloor;
                        int eastBase = eastFloor;
                        int southBase = southFloor;
                        int northBase = northFloor;
                        int southWestBase = southWestFloor;
                        int southEastBase = southEastFloor;
                        int northEastBase = northEastFloor;
                        int northWestBase = northWestFloor;
                        StructureCell westCell(0, 0, 0, 0);
                        StructureCell eastCell(0, 0, 0, 0);
                        StructureCell southCell(0, 0, 0, 0);
                        StructureCell northCell(0, 0, 0, 0);
                        StructureCell southWestCell(0, 0, 0, 0);
                        StructureCell southEastCell(0, 0, 0, 0);
                        StructureCell northEastCell(0, 0, 0, 0);
                        StructureCell northWestCell(0, 0, 0, 0);
                        if (storey < 0) {
                            // The cellar: its extents are the dungeon's.
                            westCell = m_world->structureCell(cellX - 1, cellY, true);
                            eastCell = m_world->structureCell(cellX + 1, cellY, true);
                            southCell = m_world->structureCell(cellX, cellY - 1, true);
                            northCell = m_world->structureCell(cellX, cellY + 1, true);
                            southWestCell = m_world->structureCell(cellX - 1, cellY - 1, true);
                            southEastCell = m_world->structureCell(cellX + 1, cellY - 1, true);
                            northEastCell = m_world->structureCell(cellX + 1, cellY + 1, true);
                            northWestCell = m_world->structureCell(cellX - 1, cellY + 1, true);
                            occupiedWest = westCell.kind != StructureCell_None;
                            occupiedEast = eastCell.kind != StructureCell_None;
                            occupiedSouth = southCell.kind != StructureCell_None;
                            occupiedNorth = northCell.kind != StructureCell_None;
                            occupiedSouthWest = southWestCell.kind != StructureCell_None;
                            occupiedSouthEast = southEastCell.kind != StructureCell_None;
                            occupiedNorthWest = northWestCell.kind != StructureCell_None;
                            occupiedNorthEast = northEastCell.kind != StructureCell_None;
                            westFloor = m_world->dungeonZLevel(cellX - 1, cellY);
                            eastFloor = m_world->dungeonZLevel(cellX + 1, cellY);
                            southFloor = m_world->dungeonZLevel(cellX, cellY - 1);
                            northFloor = m_world->dungeonZLevel(cellX, cellY + 1);
                            southWestFloor = m_world->dungeonZLevel(cellX - 1, cellY - 1);
                            southEastFloor = m_world->dungeonZLevel(cellX + 1, cellY - 1);
                            northEastFloor = m_world->dungeonZLevel(cellX + 1, cellY + 1);
                            northWestFloor = m_world->dungeonZLevel(cellX - 1, cellY + 1);
                            ceilingZ = dungeonTop;
                            westCeiling = m_world->dungeonZLevel(cellX - 1, cellY) + m_world->dungeonStorey(westCell);
                            eastCeiling = m_world->dungeonZLevel(cellX + 1, cellY) + m_world->dungeonStorey(eastCell);
                            southCeiling = m_world->dungeonZLevel(cellX, cellY - 1) + m_world->dungeonStorey(southCell);
                            northCeiling = m_world->dungeonZLevel(cellX, cellY + 1) + m_world->dungeonStorey(northCell);
                            southWestCeiling =
                                m_world->dungeonZLevel(cellX - 1, cellY - 1) + m_world->dungeonStorey(southWestCell);
                            southEastCeiling =
                                m_world->dungeonZLevel(cellX + 1, cellY - 1) + m_world->dungeonStorey(southEastCell);
                            northEastCeiling =
                                m_world->dungeonZLevel(cellX + 1, cellY + 1) + m_world->dungeonStorey(northEastCell);
                            northWestCeiling =
                                m_world->dungeonZLevel(cellX - 1, cellY + 1) + m_world->dungeonStorey(northWestCell);
                            westDepth = dungeonCellarDepth(westCell);
                            eastDepth = dungeonCellarDepth(eastCell);
                            southDepth = dungeonCellarDepth(southCell);
                            northDepth = dungeonCellarDepth(northCell);
                            int southWestDepth = dungeonCellarDepth(southWestCell);
                            int southEastDepth = dungeonCellarDepth(southEastCell);
                            int northEastDepth = dungeonCellarDepth(northEastCell);
                            int northWestDepth = dungeonCellarDepth(northWestCell);
                            baseZ = m_world->dungeonZ(cellX, cellY) - cellarDepth;
                            westBase = m_world->dungeonZ(cellX - 1, cellY) - westDepth;
                            eastBase = m_world->dungeonZ(cellX + 1, cellY) - eastDepth;
                            southBase = m_world->dungeonZ(cellX, cellY - 1) - southDepth;
                            northBase = m_world->dungeonZ(cellX, cellY + 1) - northDepth;
                            southWestBase = m_world->dungeonZ(cellX - 1, cellY - 1) - southWestDepth;
                            southEastBase = m_world->dungeonZ(cellX + 1, cellY - 1) - southEastDepth;
                            northEastBase = m_world->dungeonZ(cellX + 1, cellY + 1) - northEastDepth;
                            northWestBase = m_world->dungeonZ(cellX - 1, cellY + 1) - northWestDepth;
                        }
                        StructureBuilder::Material roomMaterial;
                        // The lowest base and the highest ceiling among the occupied neighbours.
                        int lowest = baseZ;
                        int highest = ceilingZ;
                        if (occupiedWest && westBase < lowest) {
                            lowest = westBase;
                        }
                        if (occupiedEast && eastBase < lowest) {
                            lowest = eastBase;
                        }
                        if (occupiedSouth && southBase < lowest) {
                            lowest = southBase;
                        }
                        if (occupiedNorth && northBase < lowest) {
                            lowest = northBase;
                        }
                        if (occupiedSouthWest && southWestBase < lowest) {
                            lowest = southWestBase;
                        }
                        if (occupiedSouthEast && southEastBase < lowest) {
                            lowest = southEastBase;
                        }
                        if (occupiedNorthEast && northEastBase < lowest) {
                            lowest = northEastBase;
                        }
                        if (occupiedNorthWest && northWestBase < lowest) {
                            lowest = northWestBase;
                        }
                        if (occupiedWest && westCeiling > highest) {
                            highest = westCeiling;
                        }
                        if (occupiedEast && eastCeiling > highest) {
                            highest = eastCeiling;
                        }
                        if (occupiedSouth && southCeiling > highest) {
                            highest = southCeiling;
                        }
                        if (occupiedNorth && northCeiling > highest) {
                            highest = northCeiling;
                        }
                        if (occupiedSouthWest && southWestCeiling > highest) {
                            highest = southWestCeiling;
                        }
                        if (occupiedSouthEast && southEastCeiling > highest) {
                            highest = southEastCeiling;
                        }
                        if (occupiedNorthEast && northEastCeiling > highest) {
                            highest = northEastCeiling;
                        }
                        if (occupiedNorthWest && northWestCeiling > highest) {
                            highest = northWestCeiling;
                        }
                        // The walls' extent along y, inset where the neighbour is open; the width of the opening
                        // towards an occupied neighbour and the walls' thickness by the type.
                        int wallY0 = occupiedSouth ? 0 : 1;
                        int wallY1 = occupiedNorth ? 16 : 15;
                        int openingWidth = 16;
                        int wallThickness = 2;
                        bool thinWalls = false;
                        if (storey >= 0) {
                            if (kind == BuildingKindHouse || kind == BuildingKindDungeonFortification ||
                                kind == BuildingKindOldHut) {
                                openingWidth = 10;
                            }
                            if (kind == BuildingKindTimberMineEntrance && type != BuildingMineDescent20 &&
                                type != BuildingMineDescent30) {
                                openingWidth = 8;
                            }
                            if (type == BuildingHousePavilion) {
                                openingWidth = 14;
                                wallThickness = 1;
                            }
                            if ((type != BuildingHousePavilion &&
                                 (type == BuildingRockLorePavilion || type == BuildingTreasurePavilion ||
                                  type == BuildingHouseCourtyard || type == BuildingRuinedPavilion ||
                                  type == BuildingRuinedFlatRoofPavilion || type == BuildingBirdStatuePavilion ||
                                  type == BuildingShrineOfLife || type == BuildingAltar ||
                                  type == BuildingFloatingIslandTreasurePavilion ||
                                  type == BuildingMourningStatuePavilion || type == BuildingLakePlatform ||
                                  type == BuildingRaisedPostPlatform || type == BuildingMountainPlatform ||
                                  type == BuildingSewerOrGrottoEntrance || type == BuildingPyramidEntrance ||
                                  type == BuildingTempleEntrance)) ||
                                kind == BuildingKindGuildHall) {
                                openingWidth = 16;
                                wallThickness = 0;
                            }
                        } else if (cell.style == DungeonCellStyle_Wide) {
                            wallThickness = 1;
                            thinWalls = true;
                        }
                        bool tall = type == BuildingCatacombsEntrance || type == BuildingSewerOrGrottoEntrance ||
                                    type == BuildingPyramidEntrance || type == BuildingTempleEntrance;
                        // The cells and types around, [dx + 1][dy + 1].
                        StructureCell cells[3][3];
                        cells[0][0] = southWestCell;
                        cells[1][0] = southCell;
                        cells[2][0] = southEastCell;
                        cells[0][1] = westCell;
                        cells[1][1] = cell;
                        cells[2][1] = eastCell;
                        cells[0][2] = northWestCell;
                        cells[1][2] = northCell;
                        cells[2][2] = northEastCell;
                        int floors[3][3];
                        floors[0][0] = southWestFloor;
                        floors[1][0] = southFloor;
                        floors[2][0] = southEastFloor;
                        floors[0][1] = westFloor;
                        floors[1][1] = floorZ;
                        floors[2][1] = eastFloor;
                        floors[0][2] = northWestFloor;
                        floors[1][2] = northFloor;
                        floors[2][2] = northEastFloor;
                        int types[3][3];
                        types[0][0] = m_world->zoneBuildingType(cellX - 1, cellY - 1);
                        types[1][0] = m_world->zoneBuildingType(cellX, cellY - 1);
                        types[2][0] = m_world->zoneBuildingType(cellX + 1, cellY - 1);
                        types[0][1] = m_world->zoneBuildingType(cellX - 1, cellY);
                        types[1][1] = m_world->zoneBuildingType(cellX, cellY);
                        types[2][1] = m_world->zoneBuildingType(cellX + 1, cellY);
                        types[0][2] = m_world->zoneBuildingType(cellX - 1, cellY + 1);
                        types[1][2] = m_world->zoneBuildingType(cellX, cellY + 1);
                        types[2][2] = m_world->zoneBuildingType(cellX + 1, cellY + 1);
                        int ceilings[3][3];
                        ceilings[0][0] = southWestCeiling;
                        ceilings[1][0] = southCeiling;
                        ceilings[2][0] = southEastCeiling;
                        ceilings[0][1] = westCeiling;
                        ceilings[1][1] = ceilingZ;
                        ceilings[2][1] = eastCeiling;
                        ceilings[0][2] = northWestCeiling;
                        ceilings[1][2] = northCeiling;
                        ceilings[2][2] = northEastCeiling;
                        // The walls' extent along x, as the one along y.
                        int wallX0 = occupiedWest ? 0 : 1;
                        int wallX1 = occupiedEast ? 16 : 15;
                        for (int x = 0; x < 16; x++) {
                            for (int y = 0; y < 16; y++) {
                                Sleep(0);
                                // A column of the room: its floor is lowered into a stair where an occupied
                                // neighbour's floor lies lower within the opening, its ceiling limited by the
                                // neighbour's next storey along the wall; the walls' corners stand where no diagonal
                                // neighbour is occupied.
                                int thickness = wallThickness;
                                bool corner = false;
                                if (x < thickness && y < thickness && !occupiedSouthWest) {
                                    corner = true;
                                }
                                if (x >= 16 - thickness && y < thickness && !occupiedSouthEast) {
                                    corner = true;
                                }
                                if (x >= 16 - thickness && y >= 16 - thickness && !occupiedNorthEast) {
                                    corner = true;
                                }
                                if (x < thickness && y >= 16 - thickness && !occupiedNorthWest) {
                                    corner = true;
                                }
                                int columnFloor = floorZ;
                                int columnCeiling = level + (storey + 1) * storeyHeight - 1;
                                if (storey < 0) {
                                    columnCeiling = dungeonTop - 1;
                                }
                                // A wall column (outside the opening, with its floor above the neighbour's, or of a
                                // hall) gets no blocks of its own.
                                if (occupiedWest) {
                                    // A stair of two blocks per step for drops of up to five, else one.
                                    bool opening = y >= (16 - openingWidth) / 2 && y < (openingWidth + 16) / 2;
                                    int drop = floorZ - westFloor;
                                    int steps = drop <= 5 ? 2 : 1;
                                    if (opening && x <= drop * steps && !tall && cellarDepth == 0 && westDepth == 0) {
                                        columnFloor = (std::min)(columnFloor, floorZ - (drop * steps - x) / steps);
                                    }
                                    if (x < thickness) {
                                        if (!opening && (floorZ > westFloor || type == BuildingHousePavilion)) {
                                            continue;
                                        }
                                        if (buildingStoreyHeight(westType) != 0) {
                                            columnCeiling = (std::min)(
                                                columnCeiling,
                                                westLevel - 1 + buildingStoreyHeight(westType) * (storey + 1));
                                        }
                                    }
                                } else if (x < thickness) {
                                    corner = true;
                                }
                                if (occupiedEast) {
                                    bool opening = y >= (16 - openingWidth) / 2 && y < (openingWidth + 16) / 2;
                                    int drop = floorZ - eastFloor;
                                    int steps = drop <= 5 ? 2 : 1;
                                    if (opening && 16 - x <= drop * steps && cellarDepth == 0 && eastDepth == 0 &&
                                        !tall) {
                                        columnFloor = (std::min)(columnFloor, floorZ - (drop * steps - 15 + x) / steps);
                                    }
                                    if (x >= 16 - thickness) {
                                        if (!opening && (floorZ > eastFloor || type == BuildingHousePavilion)) {
                                            continue;
                                        }
                                        if (buildingStoreyHeight(eastType) != 0) {
                                            columnCeiling = (std::min)(
                                                columnCeiling,
                                                eastLevel - 1 + buildingStoreyHeight(eastType) * (storey + 1));
                                        }
                                    }
                                } else if (x >= 16 - thickness) {
                                    corner = true;
                                }
                                if (occupiedSouth) {
                                    bool opening = x >= (16 - openingWidth) / 2 && x < (openingWidth + 16) / 2;
                                    int drop = floorZ - southFloor;
                                    int steps = drop <= 5 ? 2 : 1;
                                    if (opening && y <= drop * steps && cellarDepth == 0 && southDepth == 0 && !tall) {
                                        columnFloor = (std::min)(columnFloor, floorZ - (drop * steps - y) / steps);
                                    }
                                    if (y < thickness) {
                                        if (!opening && (floorZ > southFloor || type == BuildingHousePavilion)) {
                                            continue;
                                        }
                                        if (buildingStoreyHeight(southType) != 0) {
                                            columnCeiling = (std::min)(
                                                columnCeiling,
                                                southLevel - 1 + buildingStoreyHeight(southType) * (storey + 1));
                                        }
                                    }
                                } else if (y < thickness) {
                                    corner = true;
                                }
                                if (occupiedNorth) {
                                    bool opening = x >= (16 - openingWidth) / 2 && x < (openingWidth + 16) / 2;
                                    int drop = floorZ - northFloor;
                                    int steps = drop <= 5 ? 2 : 1;
                                    if (opening && 16 - y <= drop * steps && cellarDepth == 0 && northDepth == 0 &&
                                        !tall) {
                                        columnFloor = (std::min)(columnFloor, floorZ - (drop * steps - 15 + y) / steps);
                                    }
                                    if (y >= 16 - thickness) {
                                        if (!opening && (floorZ > northFloor || type == BuildingHousePavilion)) {
                                            continue;
                                        }
                                        if (buildingStoreyHeight(northType) != 0) {
                                            columnCeiling = (std::min)(
                                                columnCeiling,
                                                northLevel - 1 + buildingStoreyHeight(northType) * (storey + 1));
                                        }
                                    }
                                } else if (y >= 16 - thickness) {
                                    corner = true;
                                }
                                // The column's colours and heights. A cellar's are blended over the structure cells
                                // within 16 blocks of the column, weighted by the overlap of that window with each
                                // cell: the dungeon kinds 9..12 colour the base strongly, the others faintly; a storey
                                // takes its own.
                                double floorSum = 0.0;
                                double ceilingSum = 0.0;
                                if (storey >= 0 &&
                                    (!hasRoof(type, realmId, realmForm, biome) ||
                                     (walled && (x < wallX0 || y < wallY0 || x >= wallX1 || y >= wallY1)))) {
                                    builder.setColor(buildingColor(type, cellX, cellY, realmId, realmForm, biome),
                                                     wallTexture);
                                } else {
                                    plasma::Vector<3, double> baseSum(0.0, 0.0, 0.0);
                                    plasma::Vector<3, double> wallSum(0.0, 0.0, 0.0);
                                    plasma::Vector<3, double> trimSum(0.0, 0.0, 0.0);
                                    double weightSum = 0.0;
                                    double baseWeight = 0.0;
                                    int windowX0 = cellX * BlocksPerCell - 16 + x;
                                    int cellX0 = toCell(windowX0);
                                    int windowX1 = cellX * BlocksPerCell + 16 + x;
                                    int cellX1 = toCell(windowX1);
                                    int windowY0 = cellY * BlocksPerCell - 16 + y;
                                    int cellY0 = toCell(windowY0);
                                    int windowY1 = cellY * BlocksPerCell + 16 + y;
                                    int cellY1 = toCell(windowY1);
                                    if (storey < 0) {
                                        for (int cx = cellX0; cx <= cellX1; cx++) {
                                            for (int cy = cellY0; cy <= cellY1; cy++) {
                                                int overlapX =
                                                    (std::min)(windowX1, cx * 16 + 16) - (std::max)(windowX0, cx * 16);
                                                if (overlapX <= 0) {
                                                    continue;
                                                }
                                                int overlapY =
                                                    (std::min)(windowY1, cy * 16 + 16) - (std::max)(windowY0, cy * 16);
                                                if (overlapY <= 0) {
                                                    continue;
                                                }
                                                const StructureCell &around = cells[cx - cellX + 1][cy - cellY + 1];
                                                if (around.kind == StructureCell_None) {
                                                    continue;
                                                }
                                                // The dungeon floor colour, weighted by the overlap with each
                                                // neighbouring cell (pools have palettes of their own, the plain floor
                                                // a hundredth's weight).
                                                double weight = (double)(overlapY * overlapX) / 256.0;
                                                weight *= weight;
                                                double baseW = weight;
                                                if (around.kind == StructureCell_DeepPit) {
                                                    baseSum += weight * plasma::Vector<3, double>(10.0, 10.0, 10.0);
                                                } else if (around.kind == StructureCell_LavaPool) {
                                                    baseSum += weight * plasma::Vector<3, double>(260.0, 100.0, 10.0);
                                                } else if (around.kind == StructureCell_WaterPool) {
                                                    baseSum += weight * plasma::Vector<3, double>(20.0, 100.0, 100.0);
                                                } else if (around.kind == StructureCell_MuckPool) {
                                                    baseSum += weight * plasma::Vector<3, double>(50.0, 150.0, 20.0);
                                                } else {
                                                    baseW = weight * 0.01;
                                                    baseSum += baseW * plasma::Vector<3, double>(
                                                                           dungeonWallColor(around, dungeonType));
                                                }
                                                baseWeight += baseW;
                                                weightSum += weight;
                                                floorSum += floors[cx - cellX + 1][cy - cellY + 1] * weight;
                                                ceilingSum += ceilings[cx - cellX + 1][cy - cellY + 1] * weight;
                                                int aroundType = types[cx - cellX + 1][cy - cellY + 1];
                                                if (aroundType >= BuildingShrineDescent10 &&
                                                    aroundType <= BuildingShrineDescent30) {
                                                    trimSum +=
                                                        weight * plasma::Vector<3, double>(buildingColor(
                                                                     aroundType, cx, cy, realmId, realmForm, biome));
                                                    wallSum +=
                                                        weight * plasma::Vector<3, double>(buildingColor(
                                                                     aroundType, cx, cy, realmId, realmForm, biome));
                                                } else {
                                                    trimSum += weight * plasma::Vector<3, double>(
                                                                            dungeonTrimColor(around, dungeonType));
                                                    wallSum += weight * plasma::Vector<3, double>(
                                                                            dungeonWallColor(around, dungeonType));
                                                }
                                            }
                                        }
                                    }
                                    if (weightSum > 0.0001) {
                                        floorSum /= weightSum;
                                        ceilingSum /= weightSum;
                                        wallSum /= weightSum;
                                        trimSum /= weightSum;
                                    } else {
                                        floorSum = floorZ;
                                        ceilingSum = ceilingZ;
                                        if (storey < 0) {
                                            wallSum = dungeonWallColor(cell, dungeonType);
                                            trimSum = dungeonTrimColor(cell, dungeonType);
                                        } else {
                                            wallSum = buildingColor(type, cellX, cellY, realmId, realmForm, biome);
                                            trimSum = buildingColor(type, cellX, cellY, realmId, realmForm, biome);
                                        }
                                    }
                                    if (baseWeight > 0.0) {
                                        baseSum /= baseWeight;
                                    } else if (storey < 0) {
                                        baseSum = dungeonWallColor(cell, dungeonType);
                                        if (cell.kind == StructureCell_LavaPool) {
                                            baseSum = plasma::Vector<3, float>(260.0f, 100.0f, 10.0f);
                                        } else if (cell.kind == StructureCell_DeepPit) {
                                            baseSum = plasma::Vector<3, float>(10.0f, 10.0f, 10.0f);
                                        } else if (cell.kind == StructureCell_WaterPool) {
                                            baseSum = plasma::Vector<3, float>(20.0f, 100.0f, 100.0f);
                                        } else if (cell.kind == StructureCell_MuckPool) {
                                            baseSum = plasma::Vector<3, float>(50.0f, 150.0f, 20.0f);
                                        }
                                    } else {
                                        baseSum = buildingColor(type, cellX, cellY, realmId, realmForm, biome);
                                    }
                                    plasma::Vector<3, float> blendedTrim(trimSum);
                                    plasma::Vector<3, float> blendedWall(wallSum);
                                    plasma::Vector<3, float> blendedBase(baseSum);
                                    builder.setMaterial(blendedBase, wallTexture, (float)(floorSum - 10.0), blendedWall,
                                                        wallTexture, (float)floorSum, blendedTrim, wallTexture,
                                                        (float)ceilingSum, false);
                                    if (storey < 0) {
                                        builder.box(x, y, lowest, 1, 1, highest - lowest + 1, false);
                                    } else {
                                        builder.m_guard = PlacementKeepProtected;
                                        builder.box(x, y, lowest, 1, 1, ceilingZ - lowest + 1, false);
                                        builder.m_guard = 0;
                                    }
                                }
                                if (corner) {
                                    continue;
                                }
                                // The stair of an upper storey's entrance side: a diagonal of steps opposite the door,
                                // lowering the column's floor.
                                if (layer != 0 && storey > 0 && layer == 1) {
                                    if ((entrance == 2 && x <= 4) || (entrance == 1 && x >= 11)) {
                                        int stairStart = (16 - storeyHeight) / 2;
                                        if (y >= stairStart && y < 16 - stairStart) {
                                            columnFloor = level +
                                                          m_world->streetHeightOffset(cellX, cellY, storey - 1) +
                                                          storeyHeight * (storey - 1) - stairStart + y;
                                            Block step;
                                            step = BlockProperties<float>(builder.colorAt(x, y, columnFloor - 2),
                                                                          BlockBuilding, BlockFlagProtected, 0);
                                            builder.setCubeBlock(x, y, columnFloor - 2, step);
                                        }
                                    }
                                    if ((entrance == 8 && y <= 4) || (entrance == 4 && y >= 11)) {
                                        int stairStart = (16 - storeyHeight) / 2;
                                        if (x >= stairStart && x < 16 - stairStart) {
                                            columnFloor = level +
                                                          m_world->streetHeightOffset(cellX, cellY, storey - 1) +
                                                          storeyHeight * (storey - 1) - stairStart + x;
                                            Block step;
                                            step = BlockProperties<float>(builder.colorAt(x, y, columnFloor - 2),
                                                                          BlockBuilding, BlockFlagProtected, 0);
                                            builder.setCubeBlock(x, y, columnFloor - 2, step);
                                        }
                                    }
                                }
                                // Builds the floor block with noise shading, a chequered style colour or kind-8 wall
                                // colour, and flags and types identifying the storey. Floor light maps noise at
                                // frequency 0.05 through * 1.5 + 0.5 to the byte range.
                                int shade = (int)(std::max)(0.0f, (noise((cellX * BlocksPerCell + x) * 0.05,
                                                                         (cellY * BlocksPerCell + y) * 0.05) *
                                                                       1.5f +
                                                                   0.5f) *
                                                                      127.5f);
                                if (isHouse(type)) {
                                    if (storey >= 0) {
                                        if (kind == BuildingKindOldHut) {
                                            builder.setColor(wallColor, TextureBrick);
                                        } else {
                                            builder.setColor(styleColor(type, realmId, realmForm, biome),
                                                             modulo(cellX + cellY, 2) + 4);
                                        }
                                    }
                                } else if (storey >= 0) {
                                    builder.setMaterial(trim, TextureBrick, 0.0f, wallColor, TextureBrick, 10.0f, true);
                                }
                                unsigned char flags =
                                    storey < 0 ? BlockFlagProtected | BlockFlagDungeon : BlockFlagProtected;
                                int bottom = columnFloor;
                                if (cellarDepth != 0) {
                                    bottom = m_world->dungeonZ(cellX, cellY) - cellarDepth;
                                }
                                int floorType = BlockBuilding;
                                if (storey > 0) {
                                    floorType = storey + BlockStorey0Floor;
                                }
                                if (bottom < columnFloor) {
                                    Block block;
                                    block = BlockProperties<float>(builder.colorAt(x, y, bottom - 1), BlockBuilding,
                                                                   flags | BlockFlagNotTerrain, shade);
                                    builder.setCubeBlock(x, y, bottom - 1, block);
                                } else if (storey < 0 ||
                                           (type != BuildingShrineDescent40 && type != BuildingMineDescent40 &&
                                            type != BuildingVaultDescent40)) {
                                    Block block;
                                    block = BlockProperties<float>(builder.colorAt(x, y, bottom - 1), floorType, flags,
                                                                   shade);
                                    builder.setCubeBlock(x, y, bottom - 1, block);
                                    if (storey >= 0) {
                                        if (gallery) {
                                            // A gallery: the room's floor in the floor colour with an inlaid ring one
                                            // block inside its edge.
                                            int roomX0 = (16 - roomX) / 2;
                                            int roomX1 = (roomX + 16) / 2;
                                            int roomY0 = (16 - roomY) / 2;
                                            int roomY1 = (roomY + 16) / 2;
                                            if (x >= roomX0 && x < roomX1 && y >= roomY0 && y < roomY1) {
                                                Block inlay;
                                                if (((x == roomX0 + 1 || x == roomX1 - 2) && y >= roomY0 + 1 &&
                                                     y < roomY1 - 1) ||
                                                    ((y == roomY0 + 1 || y == roomY1 - 2) && x >= roomX0 + 1 &&
                                                     x < roomX1 - 1)) {
                                                    inlay = BlockProperties<float>(inlayColor, floorType, flags, shade);
                                                } else {
                                                    inlay = BlockProperties<float>(floorColor, floorType, flags, shade);
                                                }
                                                builder.setCubeBlock(x, y, bottom - 1, inlay);
                                            }
                                        } else if (type == BuildingCastleCell &&
                                                   m_world->castleBuildingPart(cellX, cellY, storey) ==
                                                       RoomPart_RaisedWalk) {
                                            // A castle walk: a raised floor The raised walk: pink (200, 50, 100), its
                                            // edge yellow (200, 200, 100). with a lighter edge, joined to the
                                            // neighbouring walks.
                                            plasma::Vector<3, float> walkColor(200.0f, 50.0f, 100.0f);
                                            // The neighbours' parts, each compared with a raised walk where it is
                                            // tested.
                                            int west = m_world->castleBuildingPart(cellX - 1, cellY, storey);
                                            int east = m_world->castleBuildingPart(cellX + 1, cellY, storey);
                                            int south = m_world->castleBuildingPart(cellX, cellY - 1, storey);
                                            int north = m_world->castleBuildingPart(cellX, cellY + 1, storey);
                                            int southWest = m_world->castleBuildingPart(cellX - 1, cellY - 1, storey);
                                            int southEast = m_world->castleBuildingPart(cellX + 1, cellY - 1, storey);
                                            int northEast = m_world->castleBuildingPart(cellX + 1, cellY + 1, storey);
                                            int northWest = m_world->castleBuildingPart(cellX - 1, cellY + 1, storey);
                                            // The lighter edge: one block inside the platform (x or y 4 and 11,
                                            // between 4 and 11 or out to a side that is a walk) where no walk
                                            // continues on that side.
                                            if ((!(west == RoomPart_RaisedWalk &&
                                                   (y >= 3 || southWest == RoomPart_RaisedWalk) &&
                                                   (y < 13 || northWest == RoomPart_RaisedWalk)) &&
                                                 x == 4 && (y >= 4 || south == RoomPart_RaisedWalk) &&
                                                 (y < 12 || north == RoomPart_RaisedWalk)) ||
                                                (!(east == RoomPart_RaisedWalk &&
                                                   (y >= 3 || southEast == RoomPart_RaisedWalk) &&
                                                   (y < 13 || northEast == RoomPart_RaisedWalk)) &&
                                                 x == 11 && (y >= 4 || south == RoomPart_RaisedWalk) &&
                                                 (y < 12 || north == RoomPart_RaisedWalk)) ||
                                                (!(south == RoomPart_RaisedWalk &&
                                                   (x >= 3 || southWest == RoomPart_RaisedWalk) &&
                                                   (x < 13 || southEast == RoomPart_RaisedWalk)) &&
                                                 y == 4 && (x >= 4 || west == RoomPart_RaisedWalk) &&
                                                 (x < 12 || east == RoomPart_RaisedWalk)) ||
                                                (!(north == RoomPart_RaisedWalk &&
                                                   (x >= 3 || northWest == RoomPart_RaisedWalk) &&
                                                   (x < 13 || northEast == RoomPart_RaisedWalk)) &&
                                                 y == 11 && (x >= 4 || west == RoomPart_RaisedWalk) &&
                                                 (x < 12 || east == RoomPart_RaisedWalk)) ||
                                                (southWest != RoomPart_RaisedWalk &&
                                                 ((x == 4 && y <= 4 && south == RoomPart_RaisedWalk) ||
                                                  (y == 4 && x <= 4 && west == RoomPart_RaisedWalk))) ||
                                                (southEast != RoomPart_RaisedWalk &&
                                                 ((x == 11 && y <= 4 && south == RoomPart_RaisedWalk) ||
                                                  (y == 4 && x >= 11 && east == RoomPart_RaisedWalk))) ||
                                                (northWest != RoomPart_RaisedWalk &&
                                                 ((x == 4 && y >= 11 && north == RoomPart_RaisedWalk) ||
                                                  (y == 11 && x <= 4 && west == RoomPart_RaisedWalk))) ||
                                                (northEast != RoomPart_RaisedWalk &&
                                                 ((x == 11 && y >= 11 && north == RoomPart_RaisedWalk) ||
                                                  (y == 11 && x >= 11 && east == RoomPart_RaisedWalk)))) {
                                                walkColor = plasma::Vector<3, float>(200.0f, 200.0f, 100.0f);
                                            }
                                            // The floor: the 10 x 10 platform, the bridges three blocks wide to the
                                            // walks beside, and the corners between two walks and a diagonal one.
                                            if ((x >= 3 && y >= 3 && x < 13 && y < 13) ||
                                                (x < 3 && west == RoomPart_RaisedWalk && y >= 3 && y < 13) ||
                                                (x >= 13 && east == RoomPart_RaisedWalk && y >= 3 && y < 13) ||
                                                (y < 3 && south == RoomPart_RaisedWalk && x >= 3 && x < 13) ||
                                                (y >= 13 && north == RoomPart_RaisedWalk && x >= 3 && x < 13) ||
                                                (x < 3 && y < 3 && southWest == RoomPart_RaisedWalk &&
                                                 west == RoomPart_RaisedWalk && south == RoomPart_RaisedWalk) ||
                                                (x >= 13 && y < 3 && southEast == RoomPart_RaisedWalk &&
                                                 east == RoomPart_RaisedWalk && south == RoomPart_RaisedWalk) ||
                                                (x < 3 && y >= 13 && northWest == RoomPart_RaisedWalk &&
                                                 west == RoomPart_RaisedWalk && north == RoomPart_RaisedWalk) ||
                                                (x >= 13 && y >= 13 && northEast == RoomPart_RaisedWalk &&
                                                 east == RoomPart_RaisedWalk && north == RoomPart_RaisedWalk)) {
                                                Block walk;
                                                walk = BlockProperties<float>(walkColor, floorType, flags, shade);
                                                builder.setCubeBlock(x, y, bottom - 1, walk);
                                            }
                                        }
                                    } else if (cell.roomPart == RoomPart_RaisedWalk) {
                                        plasma::Vector<3, float> walkColor(200.0f, 50.0f, 100.0f);
                                        // The lighter edge: one block inside the platform (x or y 4 and 11, between 4
                                        // and 11 or out to a side that is a walk) where no walk continues on that
                                        // side.
                                        if ((!(westCell.roomPart == RoomPart_RaisedWalk &&
                                               (y >= 3 || southWestCell.roomPart == RoomPart_RaisedWalk) &&
                                               (y < 13 || northWestCell.roomPart == RoomPart_RaisedWalk)) &&
                                             x == 4 && (y >= 4 || southCell.roomPart == RoomPart_RaisedWalk) &&
                                             (y < 12 || northCell.roomPart == RoomPart_RaisedWalk)) ||
                                            (!(eastCell.roomPart == RoomPart_RaisedWalk &&
                                               (y >= 3 || southEastCell.roomPart == RoomPart_RaisedWalk) &&
                                               (y < 13 || northEastCell.roomPart == RoomPart_RaisedWalk)) &&
                                             x == 11 && (y >= 4 || southCell.roomPart == RoomPart_RaisedWalk) &&
                                             (y < 12 || northCell.roomPart == RoomPart_RaisedWalk)) ||
                                            (!(southCell.roomPart == RoomPart_RaisedWalk &&
                                               (x >= 3 || southWestCell.roomPart == RoomPart_RaisedWalk) &&
                                               (x < 13 || southEastCell.roomPart == RoomPart_RaisedWalk)) &&
                                             y == 4 && (x >= 4 || westCell.roomPart == RoomPart_RaisedWalk) &&
                                             (x < 12 || eastCell.roomPart == RoomPart_RaisedWalk)) ||
                                            (!(northCell.roomPart == RoomPart_RaisedWalk &&
                                               (x >= 3 || northWestCell.roomPart == RoomPart_RaisedWalk) &&
                                               (x < 13 || northEastCell.roomPart == RoomPart_RaisedWalk)) &&
                                             y == 11 && (x >= 4 || westCell.roomPart == RoomPart_RaisedWalk) &&
                                             (x < 12 || eastCell.roomPart == RoomPart_RaisedWalk)) ||
                                            (southWestCell.roomPart != RoomPart_RaisedWalk &&
                                             ((x == 4 && y <= 4 && southCell.roomPart == RoomPart_RaisedWalk) ||
                                              (y == 4 && x <= 4 && westCell.roomPart == RoomPart_RaisedWalk))) ||
                                            (southEastCell.roomPart != RoomPart_RaisedWalk &&
                                             ((x == 11 && y <= 4 && southCell.roomPart == RoomPart_RaisedWalk) ||
                                              (y == 4 && x >= 11 && eastCell.roomPart == RoomPart_RaisedWalk))) ||
                                            (northWestCell.roomPart != RoomPart_RaisedWalk &&
                                             ((x == 4 && y >= 11 && northCell.roomPart == RoomPart_RaisedWalk) ||
                                              (y == 11 && x <= 4 && westCell.roomPart == RoomPart_RaisedWalk))) ||
                                            (northEastCell.roomPart != RoomPart_RaisedWalk &&
                                             ((x == 11 && y >= 11 && northCell.roomPart == RoomPart_RaisedWalk) ||
                                              (y == 11 && x >= 11 && eastCell.roomPart == RoomPart_RaisedWalk)))) {
                                            walkColor = plasma::Vector<3, float>(200.0f, 200.0f, 100.0f);
                                        }
                                        // The floor: the 10 x 10 platform, the bridges three blocks wide to the walks
                                        // beside, and the corners between two walks and a diagonal one.
                                        if ((x >= 3 && y >= 3 && x < 13 && y < 13) ||
                                            (x < 3 && westCell.roomPart == RoomPart_RaisedWalk && y >= 3 && y < 13) ||
                                            (x >= 13 && eastCell.roomPart == RoomPart_RaisedWalk && y >= 3 && y < 13) ||
                                            (y < 3 && southCell.roomPart == RoomPart_RaisedWalk && x >= 3 && x < 13) ||
                                            (y >= 13 && northCell.roomPart == RoomPart_RaisedWalk && x >= 3 &&
                                             x < 13) ||
                                            (x < 3 && y < 3 && southWestCell.roomPart == RoomPart_RaisedWalk &&
                                             westCell.roomPart == RoomPart_RaisedWalk &&
                                             southCell.roomPart == RoomPart_RaisedWalk) ||
                                            (x >= 13 && y < 3 && southEastCell.roomPart == RoomPart_RaisedWalk &&
                                             eastCell.roomPart == RoomPart_RaisedWalk &&
                                             southCell.roomPart == RoomPart_RaisedWalk) ||
                                            (x < 3 && y >= 13 && northWestCell.roomPart == RoomPart_RaisedWalk &&
                                             westCell.roomPart == RoomPart_RaisedWalk &&
                                             northCell.roomPart == RoomPart_RaisedWalk) ||
                                            (x >= 13 && y >= 13 && northEastCell.roomPart == RoomPart_RaisedWalk &&
                                             eastCell.roomPart == RoomPart_RaisedWalk &&
                                             northCell.roomPart == RoomPart_RaisedWalk)) {
                                            Block walk;
                                            walk = BlockProperties<float>(walkColor, floorType, flags, shade);
                                            builder.setCubeBlock(x, y, bottom - 1, walk);
                                        }
                                    }
                                }
                                // A cave-like cellar: the ceiling sinks towards the open sides. A cave-like ceiling:
                                // up to six blocks of noise (0.1).
                                if (thinWalls) {
                                    ceilingSum -= (noise((cellX * BlocksPerCell + x) * 0.1 + 808348.0,
                                                         (cellY * BlocksPerCell + y) * 0.1 + 8553.0) +
                                                   1.0f) *
                                                  3.0f;
                                    float edge = 1.0f;
                                    if (!occupiedWest && x < 8) {
                                        edge = (std::min)(edge, x / 8.0f);
                                    }
                                    if (!occupiedEast && x >= 8) {
                                        edge = (std::min)(edge, (15 - x) / 8.0f);
                                    }
                                    if (!occupiedSouth && y < 8) {
                                        edge = (std::min)(edge, y / 8.0f);
                                    }
                                    if (!occupiedNorth && y >= 8) {
                                        edge = (std::min)(edge, (15 - y) / 8.0f);
                                    }
                                    if (!occupiedSouthWest && x < 8 && y < 8) {
                                        edge = (std::min)(edge, (std::max)(y / 8.0f, x / 8.0f));
                                    }
                                    if (!occupiedSouthEast && x >= 8 && y < 8) {
                                        edge = (std::min)(edge, (std::max)(y / 8.0f, (15 - x) / 8.0f));
                                    }
                                    if (!occupiedNorthEast && x >= 8 && y >= 8) {
                                        edge = (std::min)(edge, (std::max)((15 - y) / 8.0f, (15 - x) / 8.0f));
                                    }
                                    if (!occupiedNorthWest && x < 8 && y >= 8) {
                                        edge = (std::min)(edge, (std::max)((15 - y) / 8.0f, x / 8.0f));
                                    }
                                    edge = 1.0f - edge;
                                    if (edge > 1.0f) {
                                        edge = 1.0f;
                                    }
                                    edge = edge * edge * edge;
                                    columnCeiling = floor((1.0f - edge) * (ceilingSum - 1.0) + edge * floorSum);
                                }
                                for (int z = bottom; z < columnCeiling - 1; z++) {
                                    builder.setCubeBlock(
                                        x, y, z,
                                        Block(plasma::Vector<3, unsigned char>(0, 0, 0), BlockAir, flags, shade));
                                }
                                // Grass on the floor of an overgrown cellar or of a trimmed tower's ground floor.
                                bool overgrown;
                                if (storey < 0) {
                                    overgrown = cell.style == DungeonCellStyle_Overgrown ||
                                                dungeonType == PlaceShrine || dungeonType == PlaceGrotto;
                                } else {
                                    overgrown = storey == 0 && hasTowerTrim(type, realmId, realmForm, biome);
                                    // Overgrown floors: grass where noise (0.1) is above 0.25.
                                }
                                if (overgrown && bottom == columnFloor &&
                                    noise((cellX * BlocksPerCell + x) * 0.1 + 89834.0,
                                          (cellY * BlocksPerCell + y) * 0.1 + 12233.0) > 0.25f) {
                                    Block grass;
                                    grass =
                                        BlockProperties<float>(m_world->harvestGrassColor(cellX * BlocksPerCell + x,
                                                                                          cellY * BlocksPerCell + y),
                                                               BlockGrass, flags, shade);
                                    builder.setCubeBlock(x, y, bottom, grass);
                                }
                                if (storey < 0 || hasRoof(type, realmId, realmForm, biome)) {
                                    Block ceiling;
                                    ceiling = BlockProperties<float>(builder.colorAt(x, y, columnCeiling - 1),
                                                                     BlockBuilding, BlockFlagProtected, shade);
                                    builder.setCubeBlock(x, y, columnCeiling - 1, ceiling);
                                }
                            }
                        }
                        // A cellar's pool: water, lava over a wallTexture floor, or muck, by the dungeon kind.
                        if (storey < 0) {
                            int poolZ = m_world->dungeonZ(cellX, cellY);
                            if (cell.kind == StructureCell_WaterPool) {
                                // The cellar pool: a floor five blocks under the surface, lava a block above it.
                                builder.water(0, 0, baseZ, 16, 16, poolZ - baseZ - 2,
                                              BlockFlagProtected | BlockFlagDungeon);
                            } else if (cell.kind == StructureCell_LavaPool) {
                                builder.underpinnedBox(0, 0, poolZ - 5, BlocksPerCell, BlocksPerCell, 1, FillMaterial,
                                                       CoverAll, false);
                                builder.lava(0, 0, poolZ - 4, 16, 16, 1, BlockFlagProtected | BlockFlagDungeon);
                                for (int px = 0; px < 16; px++) {
                                    for (int py = 0; py < 16; py++) {
                                        for (int pz = poolZ - 3; pz < poolZ; pz++) {
                                            Block wallTexture = builder.cube(px, py, pz);
                                            wallTexture.lightLevel = 0xff;
                                            builder.setCubeBlock(px, py, pz, wallTexture);
                                        }
                                    }
                                }
                            } else if (cell.kind == StructureCell_MuckPool) {
                                builder.underpinnedBox(0, 0, poolZ - 5, BlocksPerCell, BlocksPerCell, 1, FillMaterial,
                                                       CoverAll, false);
                                builder.muck(0, 0, poolZ - 4, 16, 16, 1, BlockFlagProtected | BlockFlagDungeon);
                            }
                        }
                        // The spiral stair of a layer 2 storey, from the storey below's floor (a tall type's from the
                        // dungeon level): its centre is pushed to the side whose neighbour is a layer 2 storey too.
                        // The spiral stair: radius 5 (inner 1), ten blocks of headroom.
                        int inner = 1;
                        if (layer == 2) {
                            int outer = 5;
                            int stairBase = (storey - 1) * storeyHeight + level;
                            int stairHeight = storeyHeight;
                            int stairX = m_world->structureLayer(cellX - 1, cellY, storey, 0) == 2 ? 0 : 8;
                            if (m_world->structureLayer(cellX + 1, cellY, storey, 0) == 2) {
                                stairX += 8;
                            }
                            int stairY = m_world->structureLayer(cellX, cellY - 1, storey, 0) == 2 ? 0 : 8;
                            if (m_world->structureLayer(cellX, cellY + 1, storey, 0) == 2) {
                                stairY += 8;
                            }
                            if (stairX != 8 || stairY != 8) {
                                inner = 5;
                                outer = 7;
                            }
                            if (tall && storey == 0) {
                                builder.setMaterial(dungeonWallColor(cell, dungeonType), TextureBrick,
                                                    (float)dungeonLevel, dungeonTrimColor(cell, dungeonType),
                                                    TextureBrick, (float)dungeonTop, wallColor, TextureBrick,
                                                    (float)(floorZ - 1), false);
                                builder.box(0, 0, dungeonTop - 1, BlocksPerCell, BlocksPerCell, floorZ - dungeonTop,
                                            false);
                                stairBase = dungeonLevel;
                                stairHeight = floorZ - dungeonLevel;
                            } else {
                                builder.setColor(wallColor, TextureBrick);
                            }
                            int period = 2 * inner + 1;
                            int stepZ = stairBase + 1;
                            for (int step = stairHeight - 1; step >= 0; step--) {
                                int turn = step / period % 4;
                                int along = step % period;
                                if (step == stairHeight - 1) {
                                    turn = (step - 1) / period % 4;
                                    along = (step - 1) % period + 1;
                                }
                                int stepX;
                                int stepY;
                                switch (turn) {
                                case QuarterTurn_Zero:
                                    stepX = along - inner - outer + stairX;
                                    stepY = stairY - inner - outer;
                                    break;
                                case QuarterTurn_One:
                                    stepX = stairX + inner;
                                    stepY = along - inner - outer + stairY;
                                    break;
                                case QuarterTurn_Two:
                                    stepX = stairX - along + inner;
                                    stepY = stairY + inner;
                                    break;
                                case QuarterTurn_Three:
                                    stepX = stairX - inner - outer;
                                    stepY = stairY - along + inner;
                                    break;
                                }
                                builder.cutBox(stepX, stepY, stepZ, outer, outer, 10, false);
                                builder.box(stepX, stepY, stepZ - 2, outer, outer, 2, false);
                                stepZ++;
                            }
                            builder.box(stairX - inner, stairY - inner, stairBase, 2 * inner, 2 * inner, stairHeight,
                                        false);
                        }
                        // A boss of a dungeon cell (its flags) or of a castle part (2 and 4: the kinds 8 and 10, which
                        // count the activators of their part; 3: kind 9 with an item of the region; 5: the mission's
                        // room). It is lowered onto solid ground and gets a slab across the room.
                        int part = RoomPart_None;
                        if (storey < 0 || type == BuildingCastleCell) {
                            part = storey < 0 ? cell.roomPart : m_world->castleBuildingPart(cellX, cellY, storey);
                            if (part >= RoomPart_Gate && part <= RoomPart_EmptyGate) {
                                StaticEntity gateEntity;
                                switch (part) {
                                case RoomPart_Gate:
                                    gateEntity.type = StaticEntityGate;
                                    break;
                                case RoomPart_LockedGate:
                                    gateEntity.type = StaticEntityLockedGate;
                                    gateEntity.item.category = ItemCategoryQuest;
                                    gateEntity.item.id = (modulo(placeY, 64) * 64 + modulo(placeX, 64)) % 20;
                                    gateEntity.item.region = m_world->findRegion(info.center);
                                    break;
                                case RoomPart_RuneGate:
                                    gateEntity.type = StaticEntityRuneGate;
                                    break;
                                case RoomPart_MissionRoom:
                                    gateEntity.type = missions.specialRoomAt(cellX, cellY);
                                    break;
                                case RoomPart_EmptyGate:
                                    gateEntity.type = StaticEntityNone;
                                    break;
                                }
                                if (gateEntity.type == StaticEntityGate || gateEntity.type == StaticEntityRuneGate) {
                                    if (storey < 0) {
                                        // The dungeon room (8 x 8 cells) of the activator.
                                        int partX = partIndex(cellX, CellsPerDungeonRoom);
                                        int partY = partIndex(cellY, CellsPerDungeonRoom);
                                        for (int px = partX * 8; px < partX * 8 + 8; px++) {
                                            for (int py = partY * 8; py < partY * 8 + 8; py++) {
                                                StructureCell activator = m_world->structureCell(px, py, true);
                                                if (activator.kind == StructureCell_Enemy ||
                                                    activator.kind == StructureCell_Guardian ||
                                                    activator.kind == StructureCell_EndBoss) {
                                                    m_requiredActivators[{{cellX, cellY}}]++;
                                                }
                                            }
                                        }
                                    } else if (info.type == PlaceCastle) {
                                        int castleX0 = toCell(placeX << 10);
                                        int castleX1 = toCell((placeX << 10) + 2 * BlocksPerPlace);
                                        int castleY0 = toCell(placeY << 10);
                                        int castleY1 = toCell((placeY << 10) + 2 * BlocksPerPlace);
                                        for (int px = castleX0; px < castleX1; px++) {
                                            for (int py = castleY0; py < castleY1; py++) {
                                                int castleType = m_world->zoneBuildingType(px, py);
                                                if (isCastleBuilding(castleType)) {
                                                    int castleStoreys = buildingStoreys(castleType);
                                                    for (int ps = 0; ps < castleStoreys; ps++) {
                                                        if (m_world->castleBuildingPart(px, py, ps) ==
                                                            RoomPart_Activator) {
                                                            m_requiredActivators[{{cellX, cellY}}]++;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        std::cout << "requiredActivators "
                                                  << (int)m_requiredActivators[{{cellX, cellY}}] << std::endl;
                                    }
                                }
                                if (gateEntity.type > 0) {
                                    gateEntity.transitionTime = 1000.0f;
                                    gateEntity.position = plasma::Vector<3, Fixed>(
                                        // The gate is lowered up to 30 blocks to the ground; its lintel two blocks
                                        // thick.
                                        Fixed(cellX * BlocksPerCell + 8), Fixed(cellY * BlocksPerCell + 8),
                                        Fixed(floorZ));
                                    int dropped = 0;
                                    for (int i = 0; i < 30; i++) {
                                        if (isSolid(cubeAt(gateEntity.position[0], gateEntity.position[1],
                                                           (gateEntity.position[2] - 1)))) {
                                            break;
                                        }
                                        gateEntity.position[2] -= Fixed(1);
                                        dropped--;
                                    }
                                    builder.setMaterial(wallColor, TextureBrick, (float)floorZ, trimColor, TextureBrick,
                                                        (float)ceilingZ, false);
                                    if (occupiedSouth && occupiedNorth) {
                                        gateEntity.orientation = QuarterTurn_Zero;
                                        builder.box(wallThickness, 7, floorZ + dropped + 16 - 2 * wallThickness,
                                                    16 - 2 * wallThickness, 2,
                                                    ceilingZ - floorZ - dropped - (16 - 2 * wallThickness), false);
                                    } else {
                                        gateEntity.orientation = QuarterTurn_One;
                                        builder.box(7, wallThickness, floorZ + dropped + 16 - 2 * wallThickness, 2,
                                                    16 - 2 * wallThickness,
                                                    ceilingZ - floorZ - dropped - (16 - 2 * wallThickness), false);
                                    }
                                    gateEntity.scale = (float)(16 - 2 * wallThickness);
                                    addStaticEntity(gateEntity);
                                }
                            }
                        }
                        // Along the west wall, where the neighbour has no storeys, a building (except types 25 and 26,
                        // of kinds 1, 2, 4, 5 and 8) or a type with a street door gets up to three windows, or the
                        // door first; a kind 1 house chooses its furniture from a list. The furniture stands half a
                        // block outside the wall, 3.51 blocks either side of the wall's middle, on the terrain (a type
                        // 74 piece is one of nine).
                        if (storey >= 0) {
                            builder.setMaterial(frameMaterial);
                            bool hasWindows =
                                type != BuildingHousePavilion && type != BuildingHouseCourtyard &&
                                (kind == BuildingKindHouse || kind == BuildingKindDungeonFortification ||
                                 kind == BuildingKindCrypt || kind == BuildingKindCastle || kind == BuildingKindOldHut);
                            // The exterior furniture kinds.
                            std::vector<int> exteriorFurnitureTypes;
                            if (kind == BuildingKindHouse) {
                                exteriorFurnitureTypes = {StaticEntityCrate,       StaticEntityOpenCrate,
                                                          StaticEntityBarrel,      StaticEntitySack,
                                                          StaticEntityFlowerVase1, StaticEntityStool,
                                                          StaticEntityBench};
                            }
                            if (!hasStoreys(westType) && (hasWindows || doorSides)) {
                                bool westDoor = storey == 0 && doorSides && doorSide == 1;
                                int windows = (std::min)(2, rand() % 3);
                                if (westDoor) {
                                    // Wide entrances (windows more than four apart) get a threshold.
                                    if (windowStep > 4 || windows == 0) {
                                        windows = 1;
                                    }
                                } else if (windows == 0) {
                                    if (domed) {
                                        builder.setMaterial(frameMaterial);
                                        builder.box(0, 8, floorZ, 1, 1, storeyHeight, false);
                                    }
                                    if (storey == 0) {
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furniture;
                                                    furniture.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furniture.type == StaticEntityFlowerVase1) {
                                                        furniture.type += rand() % 9;
                                                    }
                                                    furniture.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell) - 0.5f -
                                                         furniture.scaledSize()[1] * 0.5f),
                                                        (Fixed(cellY * BlocksPerCell + 8) - 3.51f), Fixed(0));
                                                    furniture.position[2] =
                                                        floor(m_world->height((int)furniture.position[0],
                                                                              (int)furniture.position[1], 0)) +
                                                        1;
                                                    furniture.orientation = QuarterTurn_Three;
                                                    if (putOnGround(furniture, true)) {
                                                        addStaticEntity(furniture);
                                                    }
                                                }
                                            } else {
                                                Plant plant;
                                                plant.type = rand() % 3 + PlantIvy;
                                                plant.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell) - 0.5f),
                                                    (Fixed(cellY * BlocksPerCell + 8) - 3.51f), Fixed(0));
                                                plant.position[2] =
                                                    // Exterior ivy: scale 0.15..0.2.
                                                    floor(m_world->height((int)plant.position[0],
                                                                          (int)plant.position[1], 0)) +
                                                    1;
                                                plant.rotation = 270.0f;
                                                plant.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plant.flags = 0;
                                                addPlant(plant);
                                            }
                                        }
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureB;
                                                    furnitureB.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureB.type == StaticEntityFlowerVase1) {
                                                        furnitureB.type += rand() % 9;
                                                    }
                                                    furnitureB.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell) - 0.5f -
                                                         furnitureB.scaledSize()[1] * 0.5f),
                                                        (Fixed(cellY * BlocksPerCell + 8) + 3.51f), Fixed(0));
                                                    furnitureB.position[2] =
                                                        floor(m_world->height((int)furnitureB.position[0],
                                                                              (int)furnitureB.position[1], 0)) +
                                                        1;
                                                    furnitureB.orientation = QuarterTurn_Three;
                                                    if (putOnGround(furnitureB, true)) {
                                                        addStaticEntity(furnitureB);
                                                    }
                                                }
                                            } else {
                                                Plant plantB;
                                                plantB.type = rand() % 3 + 54;
                                                plantB.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell) - 0.5f),
                                                    (Fixed(cellY * BlocksPerCell + 8) + 3.51f), Fixed(0));
                                                plantB.position[2] =
                                                    floor(m_world->height((int)plantB.position[0],
                                                                          (int)plantB.position[1], 0)) +
                                                    1;
                                                plantB.rotation = 270.0f;
                                                plantB.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantB.flags = 0;
                                                addPlant(plantB);
                                            }
                                        }
                                    }
                                }
                                if (storey == 0 && hasWindows && !(westDoor && windowStep > 4)) {
                                    if (windows == 1) {
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureC;
                                                    furnitureC.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureC.type == StaticEntityFlowerVase1) {
                                                        furnitureC.type += rand() % 9;
                                                    }
                                                    furnitureC.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell) - 0.5f -
                                                         furnitureC.scaledSize()[1] * 0.5f),
                                                        (Fixed(cellY * BlocksPerCell + 8) - 3.51f), Fixed(0));
                                                    furnitureC.position[2] =
                                                        floor(m_world->height((int)furnitureC.position[0],
                                                                              (int)furnitureC.position[1], 0)) +
                                                        1;
                                                    furnitureC.orientation = QuarterTurn_Three;
                                                    if (putOnGround(furnitureC, true)) {
                                                        addStaticEntity(furnitureC);
                                                    }
                                                }
                                            } else {
                                                Plant plantC;
                                                plantC.type = rand() % 3 + 54;
                                                plantC.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell) - 0.5f),
                                                    (Fixed(cellY * BlocksPerCell + 8) - 3.51f), Fixed(0));
                                                plantC.position[2] =
                                                    floor(m_world->height((int)plantC.position[0],
                                                                          (int)plantC.position[1], 0)) +
                                                    1;
                                                plantC.rotation = 270.0f;
                                                plantC.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantC.flags = 0;
                                                addPlant(plantC);
                                            }
                                        }
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureD;
                                                    furnitureD.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureD.type == StaticEntityFlowerVase1) {
                                                        furnitureD.type += rand() % 9;
                                                    }
                                                    furnitureD.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell) - 0.5f -
                                                         furnitureD.scaledSize()[1] * 0.5f),
                                                        (Fixed(cellY * BlocksPerCell + 8) + 3.51f), Fixed(0));
                                                    furnitureD.position[2] =
                                                        floor(m_world->height((int)furnitureD.position[0],
                                                                              (int)furnitureD.position[1], 0)) +
                                                        1;
                                                    furnitureD.orientation = QuarterTurn_Three;
                                                    if (putOnGround(furnitureD, true)) {
                                                        addStaticEntity(furnitureD);
                                                    }
                                                }
                                            } else {
                                                Plant plantD;
                                                plantD.type = rand() % 3 + 54;
                                                plantD.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell) - 0.5f),
                                                    (Fixed(cellY * BlocksPerCell + 8) + 3.51f), Fixed(0));
                                                plantD.position[2] =
                                                    floor(m_world->height((int)plantD.position[0],
                                                                          (int)plantD.position[1], 0)) +
                                                    1;
                                                plantD.rotation = 270.0f;
                                                plantD.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantD.flags = 0;
                                                addPlant(plantD);
                                            }
                                        }
                                    } else if (windows == 2) {
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureE;
                                                    furnitureE.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureE.type == StaticEntityFlowerVase1) {
                                                        furnitureE.type += rand() % 9;
                                                    }
                                                    // The furnishings a hundredth of a block apart.
                                                    furnitureE.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell) - 0.5f -
                                                         furnitureE.scaledSize()[1] * 0.5f),
                                                        (Fixed(cellY * BlocksPerCell + 8) - 0.01f), Fixed(0));
                                                    furnitureE.position[2] =
                                                        floor(m_world->height((int)furnitureE.position[0],
                                                                              (int)furnitureE.position[1], 0)) +
                                                        1;
                                                    furnitureE.orientation = QuarterTurn_Three;
                                                    if (putOnGround(furnitureE, true)) {
                                                        addStaticEntity(furnitureE);
                                                    }
                                                }
                                            } else {
                                                Plant plantE;
                                                plantE.type = rand() % 3 + 54;
                                                plantE.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell) - 0.5f),
                                                    (Fixed(cellY * BlocksPerCell + 8) + 0.01f), Fixed(0));
                                                plantE.position[2] =
                                                    floor(m_world->height((int)plantE.position[0],
                                                                          (int)plantE.position[1], 0)) +
                                                    1;
                                                plantE.rotation = 270.0f;
                                                plantE.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantE.flags = 0;
                                                addPlant(plantE);
                                            }
                                        }
                                    }
                                }
                                // The windows (or the door first), evenly spaced along the wall.
                                for (int i = 0; i < windows; i++) {
                                    int at = 16 / windows / 2 + i * 16 / windows;
                                    if (westDoor && i == 0) {
                                        // The door: an opening windowStep wide and high in a frame, a ramp down the
                                        // street side and the shop's sign.
                                        builder.box(0, at - windowStep / 2 - 1, floorZ, wallThickness, windowStep + 2,
                                                    windowStep + 1, false);
                                        builder.cutBox(0, at - windowStep / 2, floorZ, wallThickness, windowStep,
                                                       windowStep, false);
                                        builder.stairsX(-16, at - windowStep / 2 - 1, floorZ + 1, 16, windowStep + 2,
                                                        -16, 0, 0, 0, true);
                                        if (entranceEntityType >= 0) {
                                            StaticEntity entranceEntity;
                                            entranceEntity.transitionTime = 1000.0f;
                                            entranceEntity.type = entranceEntityType;
                                            entranceEntity.position = plasma::Vector<3, Fixed>(
                                                (Fixed(cellX * BlocksPerCell) + 0.5f),
                                                Fixed(cellY * BlocksPerCell + at), Fixed(floorZ));
                                            entranceEntity.orientation = QuarterTurn_Three;
                                            entranceEntity.scale = (float)windowStep / entranceEntity.scaledSize()[0];
                                            addStaticEntity(entranceEntity);
                                        }
                                        if (isHouse(type)) {
                                            // A house's lamp over the door (style 4: a green one).
                                            Plant lamp;
                                            lamp.type = PlantWallLantern;
                                            lamp.position = plasma::Vector<3, Fixed>(
                                                (Fixed(cellX * BlocksPerCell) - 1.0f),
                                                Fixed(cellY * BlocksPerCell + at),
                                                // The door lamp (scale 0.1, warm) and the plot's sign, 1.5 blocks
                                                // beside the door.
                                                Fixed(windowStep + floorZ));
                                            lamp.rotation = 270.0f;
                                            lamp.scale = 0.1f;
                                            lamp.color = plasma::Vector<3, float>(1.0f, 0.9f, 0.4f);
                                            if (style == HouseStyle_DarkSteepRoof) {
                                                lamp.type = PlantGreenWallLantern;
                                                lamp.color = plasma::Vector<3, float>(0.5f, 1.0f, 0.4f);
                                            }
                                            lamp.flags = PlantFlagEmissive;
                                            addPlant(lamp);
                                        }
                                        if (shopSignPlantType >= 0) {
                                            // The plot's entranceEntity beside the door, moved out of a solid block.
                                            Plant plotSign;
                                            plotSign.type = shopSignPlantType;
                                            plotSign.position = plasma::Vector<3, Fixed>(
                                                (Fixed(cellX * BlocksPerCell) - 1.0f),
                                                (Fixed(cellY * BlocksPerCell) + at - windowStep / 2 - 1.5f),
                                                Fixed(windowStep + floorZ - 1));
                                            if (isSolid(cubeAt(plotSign.position[0], plotSign.position[1],
                                                               plotSign.position[2]))) {
                                                plotSign.position[0] -= Fixed(1);
                                            }
                                            plotSign.rotation = 270.0f;
                                            plotSign.scale = 1.0f / 15.0f;
                                            plotSign.color = plasma::Vector<3, float>(1.0f, 0.9f, 0.4f);
                                            addPlant(plotSign);
                                        }
                                    } else if (hasWindows) {
                                        // A window: an opening in a frame, the window entity, and a box below it for a
                                        // styled house.
                                        builder.box(0, at - 2, floorZ, wallThickness, 4, wallStep + 2, false);
                                        builder.cutBox(0, at - 1, floorZ + 1, wallThickness, 2, wallStep, false);
                                        StaticEntity window;
                                        window.transitionTime = 1000.0f;
                                        window.type = windowEntityType;
                                        window.position = plasma::Vector<3, Fixed>(
                                            (Fixed(cellX * BlocksPerCell) + 0.5f), Fixed(cellY * BlocksPerCell + at),
                                            Fixed(floorZ + 1));
                                        window.orientation = QuarterTurn_Three;
                                        addStaticEntity(window);
                                        if (hasWindowBoxes) {
                                            StaticEntity box;
                                            box.type = rand() % 3 + StaticEntityFlowerBox01;
                                            box.position = plasma::Vector<3, Fixed>(
                                                (Fixed(cellX * BlocksPerCell) - 0.5f),
                                                Fixed(cellY * BlocksPerCell + at), Fixed(floorZ));
                                            box.orientation = QuarterTurn_Three;
                                            addStaticEntity(box);
                                        }
                                    }
                                }
                            }
                            // The east wall, the same with the shapes mirrored at x 16: the furniture faces 1 and the
                            // plants 90 degrees; the ramp runs from the floor itself.
                            if (!hasStoreys(eastType) && (hasWindows || doorSides)) {
                                bool eastDoor = storey == 0 && doorSides && doorSide == 2;
                                int windows = (std::min)(2, rand() % 3);
                                if (eastDoor) {
                                    if (windowStep > 4 || windows == 0) {
                                        windows = 1;
                                    }
                                } else if (windows == 0) {
                                    if (domed) {
                                        builder.setMaterial(frameMaterial);
                                        builder.box(15, 8, floorZ, 1, 1, storeyHeight, false);
                                    }
                                    if (storey == 0) {
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furniture;
                                                    furniture.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furniture.type == StaticEntityFlowerVase1) {
                                                        furniture.type += rand() % 9;
                                                    }
                                                    furniture.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 16) + 0.5f +
                                                         furniture.scaledSize()[1] * 0.5f),
                                                        (Fixed(cellY * BlocksPerCell + 8) - 3.51f), Fixed(0));
                                                    furniture.position[2] =
                                                        floor(m_world->height((int)furniture.position[0],
                                                                              (int)furniture.position[1], 0)) +
                                                        1;
                                                    furniture.orientation = QuarterTurn_One;
                                                    if (putOnGround(furniture, true)) {
                                                        addStaticEntity(furniture);
                                                    }
                                                }
                                            } else {
                                                Plant plant;
                                                plant.type = rand() % 3 + 54;
                                                plant.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 16) + 0.5f),
                                                    (Fixed(cellY * BlocksPerCell + 8) - 3.51f), Fixed(0));
                                                plant.position[2] = floor(m_world->height((int)plant.position[0],
                                                                                          (int)plant.position[1], 0)) +
                                                                    1;
                                                plant.rotation = 90.0f;
                                                plant.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plant.flags = 0;
                                                addPlant(plant);
                                            }
                                        }
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureB;
                                                    furnitureB.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureB.type == StaticEntityFlowerVase1) {
                                                        furnitureB.type += rand() % 9;
                                                    }
                                                    furnitureB.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 16) + 0.5f +
                                                         furnitureB.scaledSize()[1] * 0.5f),
                                                        (Fixed(cellY * BlocksPerCell + 8) + 3.51f), Fixed(0));
                                                    furnitureB.position[2] =
                                                        floor(m_world->height((int)furnitureB.position[0],
                                                                              (int)furnitureB.position[1], 0)) +
                                                        1;
                                                    furnitureB.orientation = QuarterTurn_One;
                                                    if (putOnGround(furnitureB, true)) {
                                                        addStaticEntity(furnitureB);
                                                    }
                                                }
                                            } else {
                                                Plant plantB;
                                                plantB.type = rand() % 3 + 54;
                                                plantB.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 16) + 0.5f),
                                                    (Fixed(cellY * BlocksPerCell + 8) + 3.51f), Fixed(0));
                                                plantB.position[2] =
                                                    floor(m_world->height((int)plantB.position[0],
                                                                          (int)plantB.position[1], 0)) +
                                                    1;
                                                plantB.rotation = 90.0f;
                                                plantB.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantB.flags = 0;
                                                addPlant(plantB);
                                            }
                                        }
                                    }
                                }
                                if (storey == 0 && hasWindows && !(eastDoor && windowStep > 4)) {
                                    if (windows == 1) {
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureC;
                                                    furnitureC.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureC.type == StaticEntityFlowerVase1) {
                                                        furnitureC.type += rand() % 9;
                                                    }
                                                    furnitureC.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 16) + 0.5f +
                                                         furnitureC.scaledSize()[1] * 0.5f),
                                                        (Fixed(cellY * BlocksPerCell + 8) - 3.51f), Fixed(0));
                                                    furnitureC.position[2] =
                                                        floor(m_world->height((int)furnitureC.position[0],
                                                                              (int)furnitureC.position[1], 0)) +
                                                        1;
                                                    furnitureC.orientation =
                                                        QuarterTurn_Three; // Uses the west wall's value.
                                                    if (putOnGround(furnitureC, true)) {
                                                        addStaticEntity(furnitureC);
                                                    }
                                                }
                                            } else {
                                                Plant plantC;
                                                plantC.type = rand() % 3 + 54;
                                                plantC.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 16) + 0.5f),
                                                    (Fixed(cellY * BlocksPerCell + 8) - 3.51f), Fixed(0));
                                                plantC.position[2] =
                                                    floor(m_world->height((int)plantC.position[0],
                                                                          (int)plantC.position[1], 0)) +
                                                    1;
                                                plantC.rotation = 90.0f;
                                                plantC.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantC.flags = 0;
                                                addPlant(plantC);
                                            }
                                        }
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureD;
                                                    furnitureD.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureD.type == StaticEntityFlowerVase1) {
                                                        furnitureD.type += rand() % 9;
                                                    }
                                                    furnitureD.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 16) + 0.5f +
                                                         furnitureD.scaledSize()[1] * 0.5f),
                                                        (Fixed(cellY * BlocksPerCell + 8) + 3.51f), Fixed(0));
                                                    furnitureD.position[2] =
                                                        floor(m_world->height((int)furnitureD.position[0],
                                                                              (int)furnitureD.position[1], 0)) +
                                                        1;
                                                    furnitureD.orientation = QuarterTurn_One;
                                                    if (putOnGround(furnitureD, true)) {
                                                        addStaticEntity(furnitureD);
                                                    }
                                                }
                                            } else {
                                                Plant plantD;
                                                plantD.type = rand() % 3 + 54;
                                                plantD.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 16) + 0.5f),
                                                    (Fixed(cellY * BlocksPerCell + 8) + 3.51f), Fixed(0));
                                                plantD.position[2] =
                                                    floor(m_world->height((int)plantD.position[0],
                                                                          (int)plantD.position[1], 0)) +
                                                    1;
                                                plantD.rotation = 90.0f;
                                                plantD.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantD.flags = 0;
                                                addPlant(plantD);
                                            }
                                        }
                                    } else if (windows == 2) {
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureE;
                                                    furnitureE.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureE.type == StaticEntityFlowerVase1) {
                                                        furnitureE.type += rand() % 9;
                                                    }
                                                    furnitureE.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 16) + 0.5f +
                                                         furnitureE.scaledSize()[1] * 0.5f),
                                                        (Fixed(cellY * BlocksPerCell + 8) + 0.01f), Fixed(0));
                                                    furnitureE.position[2] =
                                                        floor(m_world->height((int)furnitureE.position[0],
                                                                              (int)furnitureE.position[1], 0)) +
                                                        1;
                                                    furnitureE.orientation = QuarterTurn_One;
                                                    if (putOnGround(furnitureE, true)) {
                                                        addStaticEntity(furnitureE);
                                                    }
                                                }
                                            } else {
                                                Plant plantE;
                                                plantE.type = rand() % 3 + 54;
                                                plantE.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 16) + 0.5f),
                                                    (Fixed(cellY * BlocksPerCell + 8) + 0.01f), Fixed(0));
                                                plantE.position[2] =
                                                    floor(m_world->height((int)plantE.position[0],
                                                                          (int)plantE.position[1], 0)) +
                                                    1;
                                                plantE.rotation = 90.0f;
                                                plantE.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantE.flags = 0;
                                                addPlant(plantE);
                                            }
                                        }
                                    }
                                }
                                for (int i = 0; i < windows; i++) {
                                    int at = 16 / windows / 2 + i * 16 / windows;
                                    if (eastDoor && i == 0) {
                                        builder.box(16 - wallThickness, at - windowStep / 2 - 1, floorZ, wallThickness,
                                                    windowStep + 2, windowStep + 1, false);
                                        builder.cutBox(16 - wallThickness, at - windowStep / 2, floorZ, wallThickness,
                                                       windowStep, windowStep, false);
                                        builder.stairsX(16, at - windowStep / 2 - 1, floorZ, 16, windowStep + 2, 0, -16,
                                                        0, 0, true);
                                        if (entranceEntityType >= 0) {
                                            StaticEntity entranceEntity;
                                            entranceEntity.transitionTime = 1000.0f;
                                            entranceEntity.type = entranceEntityType;
                                            entranceEntity.position = plasma::Vector<3, Fixed>(
                                                (Fixed(cellX * BlocksPerCell) + 16 - 0.5f),
                                                Fixed(cellY * BlocksPerCell + at), Fixed(floorZ));
                                            entranceEntity.orientation = QuarterTurn_One;
                                            entranceEntity.scale = (float)windowStep / entranceEntity.scaledSize()[0];
                                            addStaticEntity(entranceEntity);
                                        }
                                        if (isHouse(type)) {
                                            Plant lamp;
                                            lamp.type = PlantWallLantern;
                                            lamp.position = plasma::Vector<3, Fixed>(
                                                (Fixed(cellX * BlocksPerCell) + 16 + 1.0f),
                                                Fixed(cellY * BlocksPerCell + at), Fixed(windowStep + floorZ));
                                            lamp.rotation = 90.0f;
                                            lamp.scale = 0.1f;
                                            lamp.color = plasma::Vector<3, float>(1.0f, 0.9f, 0.4f);
                                            if (style == HouseStyle_DarkSteepRoof) {
                                                lamp.type = PlantGreenWallLantern;
                                                lamp.color = plasma::Vector<3, float>(0.5f, 1.0f, 0.4f);
                                            }
                                            lamp.flags = PlantFlagEmissive;
                                            addPlant(lamp);
                                        }
                                        if (shopSignPlantType >= 0) {
                                            Plant plotSign;
                                            plotSign.type = shopSignPlantType;
                                            plotSign.position = plasma::Vector<3, Fixed>(
                                                (Fixed(cellX * BlocksPerCell) + 16 + 1.0f),
                                                (Fixed(cellY * BlocksPerCell) + at + windowStep / 2 + 1.5f),
                                                Fixed(windowStep + floorZ - 1));
                                            if (isSolid(cubeAt(plotSign.position[0], plotSign.position[1],
                                                               plotSign.position[2]))) {
                                                plotSign.position[0] += Fixed(1);
                                            }
                                            plotSign.rotation = 90.0f;
                                            plotSign.scale = 1.0f / 15.0f;
                                            plotSign.color = plasma::Vector<3, float>(1.0f, 0.9f, 0.4f);
                                            addPlant(plotSign);
                                        }
                                    } else if (hasWindows) {
                                        builder.box(16 - wallThickness, at - 2, floorZ, wallThickness, 4, wallStep + 2,
                                                    false);
                                        builder.cutBox(16 - wallThickness, at - 1, floorZ + 1, wallThickness, 2,
                                                       wallStep, false);
                                        StaticEntity window;
                                        window.transitionTime = 1000.0f;
                                        window.type = windowEntityType;
                                        window.position = plasma::Vector<3, Fixed>(
                                            (Fixed(cellX * BlocksPerCell) + 16 - 0.5f),
                                            Fixed(cellY * BlocksPerCell + at), Fixed(floorZ + 1));
                                        window.orientation = QuarterTurn_One;
                                        addStaticEntity(window);
                                        if (hasWindowBoxes) {
                                            StaticEntity box;
                                            box.type = rand() % 3 + 60;
                                            box.position = plasma::Vector<3, Fixed>(
                                                (Fixed(cellX * BlocksPerCell) + 16 + 0.5f),
                                                Fixed(cellY * BlocksPerCell + at), Fixed(floorZ));
                                            box.orientation = QuarterTurn_One;
                                            addStaticEntity(box);
                                        }
                                    }
                                }
                            }
                            // The south wall: the shapes along x at y 0, the furniture facing 0 and the plants at 0
                            // degrees, the ramp down to the south (rampY).
                            if (!hasStoreys(southType) && (hasWindows || doorSides)) {
                                bool southDoor = storey == 0 && doorSides && doorSide == 4;
                                int windows = (std::min)(2, rand() % 3);
                                if (southDoor) {
                                    if (windowStep > 4 || windows == 0) {
                                        windows = 1;
                                    }
                                } else if (windows == 0) {
                                    if (domed) {
                                        builder.setMaterial(frameMaterial);
                                        builder.box(8, 0, floorZ, 1, 1, storeyHeight, false);
                                    }
                                    if (storey == 0) {
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furniture;
                                                    furniture.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furniture.type == StaticEntityFlowerVase1) {
                                                        furniture.type += rand() % 9;
                                                    }
                                                    furniture.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 8) - 3.51f),
                                                        (Fixed(cellY * BlocksPerCell) - 0.5f -
                                                         furniture.scaledSize()[1] * 0.5f),
                                                        Fixed(0));
                                                    furniture.position[2] =
                                                        floor(m_world->height((int)furniture.position[0],
                                                                              (int)furniture.position[1], 0)) +
                                                        1;
                                                    furniture.orientation = QuarterTurn_Zero;
                                                    if (putOnGround(furniture, true)) {
                                                        addStaticEntity(furniture);
                                                    }
                                                }
                                            } else {
                                                Plant plant;
                                                plant.type = rand() % 3 + 54;
                                                plant.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 8) - 3.51f),
                                                    (Fixed(cellY * BlocksPerCell) - 0.5f), Fixed(0));
                                                plant.position[2] = floor(m_world->height((int)plant.position[0],
                                                                                          (int)plant.position[1], 0)) +
                                                                    1;
                                                plant.rotation = 0.0f;
                                                plant.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plant.flags = 0;
                                                addPlant(plant);
                                            }
                                        }
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureB;
                                                    furnitureB.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureB.type == StaticEntityFlowerVase1) {
                                                        furnitureB.type += rand() % 9;
                                                    }
                                                    furnitureB.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 8) + 3.51f),
                                                        (Fixed(cellY * BlocksPerCell) - 0.5f -
                                                         furnitureB.scaledSize()[1] * 0.5f),
                                                        Fixed(0));
                                                    furnitureB.position[2] =
                                                        floor(m_world->height((int)furnitureB.position[0],
                                                                              (int)furnitureB.position[1], 0)) +
                                                        1;
                                                    furnitureB.orientation = QuarterTurn_Zero;
                                                    if (putOnGround(furnitureB, true)) {
                                                        addStaticEntity(furnitureB);
                                                    }
                                                }
                                            } else {
                                                Plant plantB;
                                                plantB.type = rand() % 3 + 54;
                                                plantB.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 8) + 3.51f),
                                                    (Fixed(cellY * BlocksPerCell) - 0.5f), Fixed(0));
                                                plantB.position[2] =
                                                    floor(m_world->height((int)plantB.position[0],
                                                                          (int)plantB.position[1], 0)) +
                                                    1;
                                                plantB.rotation = 0.0f;
                                                plantB.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantB.flags = 0;
                                                addPlant(plantB);
                                            }
                                        }
                                    }
                                }
                                if (storey == 0 && hasWindows && !(southDoor && windowStep > 4)) {
                                    if (windows == 1) {
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureC;
                                                    furnitureC.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureC.type == StaticEntityFlowerVase1) {
                                                        furnitureC.type += rand() % 9;
                                                    }
                                                    furnitureC.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 8) - 3.51f),
                                                        (Fixed(cellY * BlocksPerCell) - 0.5f -
                                                         furnitureC.scaledSize()[1] * 0.5f),
                                                        Fixed(0));
                                                    furnitureC.position[2] =
                                                        floor(m_world->height((int)furnitureC.position[0],
                                                                              (int)furnitureC.position[1], 0)) +
                                                        1;
                                                    furnitureC.orientation = QuarterTurn_Zero;
                                                    if (putOnGround(furnitureC, true)) {
                                                        addStaticEntity(furnitureC);
                                                    }
                                                }
                                            } else {
                                                Plant plantC;
                                                plantC.type = rand() % 3 + 54;
                                                plantC.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 8) - 3.51f),
                                                    (Fixed(cellY * BlocksPerCell) - 0.5f), Fixed(0));
                                                plantC.position[2] =
                                                    floor(m_world->height((int)plantC.position[0],
                                                                          (int)plantC.position[1], 0)) +
                                                    1;
                                                plantC.rotation = 0.0f;
                                                plantC.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantC.flags = 0;
                                                addPlant(plantC);
                                            }
                                        }
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureD;
                                                    furnitureD.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureD.type == StaticEntityFlowerVase1) {
                                                        furnitureD.type += rand() % 9;
                                                    }
                                                    furnitureD.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 8) + 3.51f),
                                                        (Fixed(cellY * BlocksPerCell) - 0.5f -
                                                         furnitureD.scaledSize()[1] * 0.5f),
                                                        Fixed(0));
                                                    furnitureD.position[2] =
                                                        floor(m_world->height((int)furnitureD.position[0],
                                                                              (int)furnitureD.position[1], 0)) +
                                                        1;
                                                    furnitureD.orientation = QuarterTurn_Zero;
                                                    if (putOnGround(furnitureD, true)) {
                                                        addStaticEntity(furnitureD);
                                                    }
                                                }
                                            } else {
                                                Plant plantD;
                                                plantD.type = rand() % 3 + 54;
                                                plantD.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 8) + 3.51f),
                                                    (Fixed(cellY * BlocksPerCell) - 0.5f), Fixed(0));
                                                plantD.position[2] =
                                                    floor(m_world->height((int)plantD.position[0],
                                                                          (int)plantD.position[1], 0)) +
                                                    1;
                                                plantD.rotation = 0.0f;
                                                plantD.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantD.flags = 0;
                                                addPlant(plantD);
                                            }
                                        }
                                    } else if (windows == 2) {
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureE;
                                                    furnitureE.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureE.type == StaticEntityFlowerVase1) {
                                                        furnitureE.type += rand() % 9;
                                                    }
                                                    furnitureE.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 8) - 0.01f),
                                                        (Fixed(cellY * BlocksPerCell) - 0.5f -
                                                         furnitureE.scaledSize()[1] * 0.5f),
                                                        Fixed(0));
                                                    furnitureE.position[2] =
                                                        floor(m_world->height((int)furnitureE.position[0],
                                                                              (int)furnitureE.position[1], 0)) +
                                                        1;
                                                    furnitureE.orientation = QuarterTurn_Zero;
                                                    if (putOnGround(furnitureE, true)) {
                                                        addStaticEntity(furnitureE);
                                                    }
                                                }
                                            } else {
                                                Plant plantE;
                                                plantE.type = rand() % 3 + 54;
                                                plantE.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 8) + 0.01f),
                                                    (Fixed(cellY * BlocksPerCell) - 0.5f), Fixed(0));
                                                plantE.position[2] =
                                                    floor(m_world->height((int)plantE.position[0],
                                                                          (int)plantE.position[1], 0)) +
                                                    1;
                                                plantE.rotation = 0.0f;
                                                plantE.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantE.flags = 0;
                                                addPlant(plantE);
                                            }
                                        }
                                    }
                                }
                                for (int i = 0; i < windows; i++) {
                                    int at = 16 / windows / 2 + i * 16 / windows;
                                    if (southDoor && i == 0) {
                                        builder.box(at - windowStep / 2 - 1, 0, floorZ, windowStep + 2, wallThickness,
                                                    windowStep + 1, false);
                                        builder.cutBox(at - windowStep / 2, 0, floorZ, windowStep, wallThickness,
                                                       windowStep, false);
                                        builder.stairsY(at - windowStep / 2 - 1, -16, floorZ + 1, windowStep + 2, 16,
                                                        -16, 0, 0, 0, true);
                                        if (entranceEntityType >= 0) {
                                            StaticEntity entranceEntity;
                                            entranceEntity.transitionTime = 1000.0f;
                                            entranceEntity.type = entranceEntityType;
                                            entranceEntity.position = plasma::Vector<3, Fixed>(
                                                Fixed(cellX * BlocksPerCell + at),
                                                (Fixed(cellY * BlocksPerCell) + 0.5f), Fixed(floorZ));
                                            entranceEntity.orientation = QuarterTurn_Zero;
                                            entranceEntity.scale = (float)windowStep / entranceEntity.scaledSize()[0];
                                            addStaticEntity(entranceEntity);
                                        }
                                        if (isHouse(type)) {
                                            Plant lamp;
                                            lamp.type = PlantWallLantern;
                                            lamp.position = plasma::Vector<3, Fixed>(
                                                Fixed(cellX * BlocksPerCell + at),
                                                (Fixed(cellY * BlocksPerCell) - 1.0f), Fixed(windowStep + floorZ));
                                            lamp.rotation = 0.0f;
                                            lamp.scale = 0.1f;
                                            lamp.color = plasma::Vector<3, float>(1.0f, 0.9f, 0.4f);
                                            if (style == HouseStyle_DarkSteepRoof) {
                                                lamp.type = PlantGreenWallLantern;
                                                lamp.color = plasma::Vector<3, float>(0.5f, 1.0f, 0.4f);
                                            }
                                            lamp.flags = PlantFlagEmissive;
                                            addPlant(lamp);
                                        }
                                        if (shopSignPlantType >= 0) {
                                            Plant plotSign;
                                            plotSign.type = shopSignPlantType;
                                            plotSign.position = plasma::Vector<3, Fixed>(
                                                (Fixed(cellX * BlocksPerCell) + at - windowStep / 2 - 1.5f),
                                                (Fixed(cellY * BlocksPerCell) - 1.0f), Fixed(windowStep + floorZ - 1));
                                            if (isSolid(cubeAt(plotSign.position[0], plotSign.position[1],
                                                               plotSign.position[2]))) {
                                                plotSign.position[1] -= Fixed(1);
                                            }
                                            plotSign.rotation = 0.0f;
                                            plotSign.scale = 1.0f / 15.0f;
                                            plotSign.color = plasma::Vector<3, float>(1.0f, 0.9f, 0.4f);
                                            addPlant(plotSign);
                                        }
                                    } else if (hasWindows) {
                                        builder.box(at - 2, 0, floorZ, 4, wallThickness, wallStep + 2, false);
                                        builder.cutBox(at - 1, 0, floorZ + 1, 2, wallThickness, wallStep, false);
                                        StaticEntity window;
                                        window.transitionTime = 1000.0f;
                                        window.type = windowEntityType;
                                        window.position = plasma::Vector<3, Fixed>(
                                            Fixed(cellX * BlocksPerCell + at), (Fixed(cellY * BlocksPerCell) + 0.5f),
                                            Fixed(floorZ + 1));
                                        window.orientation = QuarterTurn_Zero;
                                        addStaticEntity(window);
                                        if (hasWindowBoxes) {
                                            StaticEntity box;
                                            box.type = rand() % 3 + 60;
                                            box.position = plasma::Vector<3, Fixed>(
                                                Fixed(cellX * BlocksPerCell + at),
                                                (Fixed(cellY * BlocksPerCell) - 0.5f), Fixed(floorZ));
                                            box.orientation = QuarterTurn_Zero;
                                            addStaticEntity(box);
                                        }
                                    }
                                }
                            }
                            // The north wall: the shapes at y 16, the furniture facing 2 and the plants at 180
                            // degrees; the ramp runs from the floor itself.
                            if (!hasStoreys(northType) && (hasWindows || doorSides)) {
                                bool northDoor = storey == 0 && doorSides && doorSide == 8;
                                int windows = (std::min)(2, rand() % 3);
                                if (northDoor) {
                                    if (windowStep > 4 || windows == 0) {
                                        windows = 1;
                                    }
                                } else if (windows == 0) {
                                    if (domed) {
                                        builder.setMaterial(frameMaterial);
                                        builder.box(8, 15, floorZ, 1, 1, storeyHeight, false);
                                    }
                                    if (storey == 0) {
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furniture;
                                                    furniture.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furniture.type == StaticEntityFlowerVase1) {
                                                        furniture.type += rand() % 9;
                                                    }
                                                    furniture.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 8) - 3.51f),
                                                        (Fixed(cellY * BlocksPerCell + 16) + 0.5f +
                                                         furniture.scaledSize()[1] * 0.5f),
                                                        Fixed(0));
                                                    furniture.position[2] =
                                                        floor(m_world->height((int)furniture.position[0],
                                                                              (int)furniture.position[1], 0)) +
                                                        1;
                                                    furniture.orientation = QuarterTurn_Two;
                                                    if (putOnGround(furniture, true)) {
                                                        addStaticEntity(furniture);
                                                    }
                                                }
                                            } else {
                                                Plant plant;
                                                plant.type = rand() % 3 + 54;
                                                plant.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 8) - 3.51f),
                                                    (Fixed(cellY * BlocksPerCell + 16) + 0.5f), Fixed(0));
                                                plant.position[2] = floor(m_world->height((int)plant.position[0],
                                                                                          (int)plant.position[1], 0)) +
                                                                    1;
                                                plant.rotation = 180.0f;
                                                plant.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plant.flags = 0;
                                                addPlant(plant);
                                            }
                                        }
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureB;
                                                    furnitureB.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureB.type == StaticEntityFlowerVase1) {
                                                        furnitureB.type += rand() % 9;
                                                    }
                                                    furnitureB.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 8) + 3.51f),
                                                        (Fixed(cellY * BlocksPerCell + 16) + 0.5f +
                                                         furnitureB.scaledSize()[1] * 0.5f),
                                                        Fixed(0));
                                                    furnitureB.position[2] =
                                                        floor(m_world->height((int)furnitureB.position[0],
                                                                              (int)furnitureB.position[1], 0)) +
                                                        1;
                                                    furnitureB.orientation = QuarterTurn_Two;
                                                    if (putOnGround(furnitureB, true)) {
                                                        addStaticEntity(furnitureB);
                                                    }
                                                }
                                            } else {
                                                Plant plantB;
                                                plantB.type = rand() % 3 + 54;
                                                plantB.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 8) + 3.51f),
                                                    (Fixed(cellY * BlocksPerCell + 16) + 0.5f), Fixed(0));
                                                plantB.position[2] =
                                                    floor(m_world->height((int)plantB.position[0],
                                                                          (int)plantB.position[1], 0)) +
                                                    1;
                                                plantB.rotation = 180.0f;
                                                plantB.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantB.flags = 0;
                                                addPlant(plantB);
                                            }
                                        }
                                    }
                                }
                                if (storey == 0 && hasWindows && !(northDoor && windowStep > 4)) {
                                    if (windows == 1) {
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureC;
                                                    furnitureC.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureC.type == StaticEntityFlowerVase1) {
                                                        furnitureC.type += rand() % 9;
                                                    }
                                                    furnitureC.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 8) - 3.51f),
                                                        (Fixed(cellY * BlocksPerCell + 16) + 0.5f +
                                                         furnitureC.scaledSize()[1] * 0.5f),
                                                        Fixed(0));
                                                    furnitureC.position[2] =
                                                        floor(m_world->height((int)furnitureC.position[0],
                                                                              (int)furnitureC.position[1], 0)) +
                                                        1;
                                                    furnitureC.orientation = QuarterTurn_Two;
                                                    if (putOnGround(furnitureC, true)) {
                                                        addStaticEntity(furnitureC);
                                                    }
                                                }
                                            } else {
                                                Plant plantC;
                                                plantC.type = rand() % 3 + 54;
                                                plantC.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 8) - 3.51f),
                                                    (Fixed(cellY * BlocksPerCell + 16) + 0.5f), Fixed(0));
                                                plantC.position[2] =
                                                    floor(m_world->height((int)plantC.position[0],
                                                                          (int)plantC.position[1], 0)) +
                                                    1;
                                                plantC.rotation = 180.0f;
                                                plantC.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantC.flags = 0;
                                                addPlant(plantC);
                                            }
                                        }
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureD;
                                                    furnitureD.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureD.type == StaticEntityFlowerVase1) {
                                                        furnitureD.type += rand() % 9;
                                                    }
                                                    furnitureD.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 8) + 3.51f),
                                                        (Fixed(cellY * BlocksPerCell + 16) + 0.5f +
                                                         furnitureD.scaledSize()[1] * 0.5f),
                                                        Fixed(0));
                                                    furnitureD.position[2] =
                                                        floor(m_world->height((int)furnitureD.position[0],
                                                                              (int)furnitureD.position[1], 0)) +
                                                        1;
                                                    furnitureD.orientation = QuarterTurn_Two;
                                                    if (putOnGround(furnitureD, true)) {
                                                        addStaticEntity(furnitureD);
                                                    }
                                                }
                                            } else {
                                                Plant plantD;
                                                plantD.type = rand() % 3 + 54;
                                                plantD.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 8) + 3.51f),
                                                    (Fixed(cellY * BlocksPerCell + 16) + 0.5f), Fixed(0));
                                                plantD.position[2] =
                                                    floor(m_world->height((int)plantD.position[0],
                                                                          (int)plantD.position[1], 0)) +
                                                    1;
                                                plantD.rotation = 180.0f;
                                                plantD.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantD.flags = 0;
                                                addPlant(plantD);
                                            }
                                        }
                                    } else if (windows == 2) {
                                        if (rand() % 2 != 0) {
                                            if (rand() % 2 != 0) {
                                                if (!exteriorFurnitureTypes.empty()) {
                                                    StaticEntity furnitureE;
                                                    furnitureE.type =
                                                        exteriorFurnitureTypes[rand() % exteriorFurnitureTypes.size()];
                                                    if (furnitureE.type == StaticEntityFlowerVase1) {
                                                        furnitureE.type += rand() % 9;
                                                    }
                                                    furnitureE.position = plasma::Vector<3, Fixed>(
                                                        (Fixed(cellX * BlocksPerCell + 8) + 0.01f),
                                                        (Fixed(cellY * BlocksPerCell + 16) + 0.5f +
                                                         furnitureE.scaledSize()[1] * 0.5f),
                                                        Fixed(0));
                                                    furnitureE.position[2] =
                                                        floor(m_world->height((int)furnitureE.position[0],
                                                                              (int)furnitureE.position[1], 0)) +
                                                        1;
                                                    furnitureE.orientation = QuarterTurn_Two;
                                                    if (putOnGround(furnitureE, true)) {
                                                        addStaticEntity(furnitureE);
                                                    }
                                                }
                                            } else {
                                                Plant plantE;
                                                plantE.type = rand() % 3 + 54;
                                                plantE.position = plasma::Vector<3, Fixed>(
                                                    (Fixed(cellX * BlocksPerCell + 8) + 0.01f),
                                                    (Fixed(cellY * BlocksPerCell + 16) + 0.5f), Fixed(0));
                                                plantE.position[2] =
                                                    floor(m_world->height((int)plantE.position[0],
                                                                          (int)plantE.position[1], 0)) +
                                                    1;
                                                plantE.rotation = 180.0f;
                                                plantE.scale = (float)rand() * 0.05f / (float)RAND_MAX + 0.15f;
                                                plantE.flags = 0;
                                                addPlant(plantE);
                                            }
                                        }
                                    }
                                }
                                for (int i = 0; i < windows; i++) {
                                    int at = 16 / windows / 2 + i * 16 / windows;
                                    if (northDoor && i == 0) {
                                        builder.box(at - windowStep / 2 - 1, 16 - wallThickness, floorZ, windowStep + 2,
                                                    wallThickness, windowStep + 1, false);
                                        builder.cutBox(at - windowStep / 2, 16 - wallThickness, floorZ, windowStep,
                                                       wallThickness, windowStep, false);
                                        builder.stairsY(at - windowStep / 2 - 1, 16, floorZ, windowStep + 2, 16, 0, -16,
                                                        0, 0, true);
                                        if (entranceEntityType >= 0) {
                                            StaticEntity entranceEntity;
                                            entranceEntity.transitionTime = 1000.0f;
                                            entranceEntity.type = entranceEntityType;
                                            entranceEntity.position = plasma::Vector<3, Fixed>(
                                                Fixed(cellX * BlocksPerCell + at),
                                                (Fixed(cellY * BlocksPerCell) + 16 - 0.5f), Fixed(floorZ));
                                            entranceEntity.orientation = QuarterTurn_Two;
                                            entranceEntity.scale = (float)windowStep / entranceEntity.scaledSize()[0];
                                            addStaticEntity(entranceEntity);
                                        }
                                        if (isHouse(type)) {
                                            Plant lamp;
                                            lamp.type = PlantWallLantern;
                                            lamp.position = plasma::Vector<3, Fixed>(
                                                Fixed(cellX * BlocksPerCell + at),
                                                (Fixed(cellY * BlocksPerCell) + 16 + 1.0f), Fixed(windowStep + floorZ));
                                            lamp.rotation = 180.0f;
                                            lamp.scale = 0.1f;
                                            lamp.color = plasma::Vector<3, float>(1.0f, 0.9f, 0.4f);
                                            if (style == HouseStyle_DarkSteepRoof) {
                                                lamp.type = PlantGreenWallLantern;
                                                lamp.color = plasma::Vector<3, float>(0.5f, 1.0f, 0.4f);
                                            }
                                            lamp.flags = PlantFlagEmissive;
                                            addPlant(lamp);
                                        }
                                        if (shopSignPlantType >= 0) {
                                            Plant plotSign;
                                            plotSign.type = shopSignPlantType;
                                            plotSign.position = plasma::Vector<3, Fixed>(
                                                (Fixed(cellX * BlocksPerCell) + at + windowStep / 2 + 1.5f),
                                                (Fixed(cellY * BlocksPerCell) + 16 + 1.0f),
                                                Fixed(windowStep + floorZ - 1));
                                            if (isSolid(cubeAt(plotSign.position[0], plotSign.position[1],
                                                               plotSign.position[2]))) {
                                                plotSign.position[1] += Fixed(1);
                                            }
                                            plotSign.rotation = 180.0f;
                                            plotSign.scale = 1.0f / 15.0f;
                                            plotSign.color = plasma::Vector<3, float>(1.0f, 0.9f, 0.4f);
                                            addPlant(plotSign);
                                        }
                                    } else if (hasWindows) {
                                        builder.box(at - 2, 16 - wallThickness, floorZ, 4, wallThickness, wallStep + 2,
                                                    false);
                                        builder.cutBox(at - 1, 16 - wallThickness, floorZ + 1, 2, wallThickness,
                                                       wallStep, false);
                                        StaticEntity window;
                                        window.transitionTime = 1000.0f;
                                        window.type = windowEntityType;
                                        window.position = plasma::Vector<3, Fixed>(
                                            Fixed(cellX * BlocksPerCell + at),
                                            (Fixed(cellY * BlocksPerCell) + 16 - 0.5f), Fixed(floorZ + 1));
                                        window.orientation = QuarterTurn_Two;
                                        addStaticEntity(window);
                                        if (hasWindowBoxes) {
                                            StaticEntity box;
                                            box.type = rand() % 3 + 60;
                                            box.position = plasma::Vector<3, Fixed>(
                                                Fixed(cellX * BlocksPerCell + at),
                                                (Fixed(cellY * BlocksPerCell) + 16 + 0.5f), Fixed(floorZ));
                                            box.orientation = QuarterTurn_Two;
                                            addStaticEntity(box);
                                        }
                                    }
                                }
                            }
                            // Smoke over a house (or a kind 9 cell) that is within the place's inner cells: a type 23
                            // plant (26, green, for the realm's style 4) at the roof's middle, from three blocks below
                            // the ceiling up to the first solid block above it. A ceiling lantern: in one house or
                            // guild room in four, by the doors, and on an inn's ground floor. A type 26 house restores
                            // the wall material for what follows.
                            if (!outside && (isHouse(type) || kind == BuildingKindGuildHall) &&
                                (rand() % 4 == 0 || doorSides || (plot == PlotInn && storey == 0)) &&
                                type != BuildingOldHutChimney) {
                                Plant ceilingLantern;
                                ceilingLantern.type = PlantLantern02;
                                ceilingLantern.position =
                                    plasma::Vector<3, Fixed>((Fixed(cellX * BlocksPerCell) + 8 + 0.5f),
                                                             (Fixed(cellY * BlocksPerCell) + 8 + 0.5f),
                                                             Fixed(level + (storey + 1) * storeyHeight - 3));
                                if (houseStyle(realmId, realmForm, biome) == HouseStyle_DarkSteepRoof) {
                                    ceilingLantern.type = PlantLanternGreen;
                                    ceilingLantern.color = plasma::Vector<3, float>(0.5f, 1.0f, 0.4f);
                                }
                                for (int i = 0; i < 4; i++) {
                                    if (isSolid(cubeAt(ceilingLantern.position[0], ceilingLantern.position[1],
                                                       (ceilingLantern.position[2] + 1)))) {
                                        ceilingLantern.scale = 0.1f;
                                        ceilingLantern.flags = PlantFlagEmissive;
                                        ceilingLantern.rotation = 0.0f;
                                        addPlant(ceilingLantern);
                                        break;
                                    }
                                    ceilingLantern.position[2] += Fixed(1);
                                }
                            }
                            if (type == BuildingHouseCourtyard) {
                                builder.setMaterial(wall);
                            }
                        }
                        // The room is decorated in the storey's colours (a dungeon cellar's own shade) and its
                        // entities placed; the room of a castle part (or of a cellar with those flags) gets its
                        // centrepiece: a chandelier (12), a beam across the room with a lamp (11), a dais (8) or a
                        // central pillar (1).
                        if (!outside && (!tall || storey >= 0) && m_world->streetHeightOffset(cellX, cellY, 0) > -32 &&
                            !(cell.roomPart >= RoomPart_Gate && cell.roomPart <= RoomPart_MissionRoom && storey < 0) &&
                            (storey < 0 || storeyed)) {
                            // The dungeon rooms' fixtures in palettes by cell kind (lava orange 260, 100, 10, ...).
                            plasma::Vector<3, float> roomColor = wallColor;
                            if (cell.kind == StructureCell_LavaPool) {
                                roomColor = plasma::Vector<3, float>(260.0f, 100.0f, 10.0f);
                            } else if (cell.kind == StructureCell_DeepPit) {
                                roomColor = plasma::Vector<3, float>(10.0f, 10.0f, 10.0f);
                            } else if (cell.kind == StructureCell_WaterPool) {
                                roomColor = plasma::Vector<3, float>(20.0f, 100.0f, 100.0f);
                            } else if (cell.kind == StructureCell_MuckPool) {
                                roomColor = plasma::Vector<3, float>(50.0f, 150.0f, 20.0f);
                            }
                            PlantBuilder plants(*this, cellX, cellY, storey, wallColor, trimColor, roomColor);
                            plants.decorate(wallThickness, wallThickness, floorZ, 16 - 2 * wallThickness,
                                            16 - 2 * wallThickness, ceilingZ - floorZ - 2);
                            int placeType;
                            int roomPlot;
                            if (storey < 0) {
                                placeType = dungeonType;
                                roomPlot = PlotNone;
                            } else {
                                placeType = info.type;
                                roomPlot = m_world->hamletPlot(cellX, cellY, info.type, dungeonPosition);
                            }
                            addRoomEntities(cellX, cellY, floorZ, type, storey, placeType, roomPlot);
                            int z = ceilingZ - 2;
                            switch (part) {
                            case RoomPart_Stalactite:
                                // A stalactite of radius 3..6 and height 8..12.
                                builder.setMaterial(roomColor, TexturePlain, (float)(floorZ - cellarDepth), wallColor,
                                                    TexturePlain, (float)floorZ, trimColor, TexturePlain,
                                                    (float)ceilingZ, false);
                                builder.stalactite(8, 8, z, rand() % 4 + 3, rand() % 5 + 8);
                                break;
                            case RoomPart_CrossBeam:
                                // A crossbeam of wood (150, 120, 50), down from the ceiling (at most ten blocks) to
                                // the floor on 2 x 2 posts, with a lantern (scale 0.1, three blocks down) for one in
                                // four, at a quarter of the cell.
                                builder.setColor(plasma::Vector<3, float>(150.0f, 120.0f, 50.0f), TexturePlain);
                                // Down from the ceiling to the first free block.
                                for (int i = 0; i < 10; i++) {
                                    if (isFree(builder.cube(8, 8, z - 1), true, true, true)) {
                                        break;
                                    }
                                    z--;
                                }
                                builder.underpinnedBox(7, 7, floorZ, 2, 2, z - floorZ, FillMaterial, CoverAll, false);
                                if (rand() % 2 != 0) {
                                    builder.box(0, 7, z - 2, BlocksPerCell, 2, 2, false);
                                    if (rand() % 4 == 0) {
                                        Plant lamp;
                                        lamp.type = PlantLantern02;
                                        lamp.position = plasma::Vector<3, Fixed>(
                                            (Fixed(cellX * BlocksPerCell) + 4 + rand() % 2 * 8),
                                            (Fixed(cellY * BlocksPerCell) + 8), Fixed(z - 3));
                                        lamp.scale = 0.1f;
                                        lamp.flags = PlantFlagEmissive;
                                        lamp.rotation = 0.0f;
                                        addPlant(lamp);
                                    }
                                } else {
                                    builder.box(7, 0, z - 2, 2, BlocksPerCell, 2, false);
                                    if (rand() % 4 == 0) {
                                        Plant lamp;
                                        lamp.type = PlantLantern02;
                                        lamp.position = plasma::Vector<3, Fixed>(
                                            (Fixed(cellX * BlocksPerCell) + 8),
                                            (Fixed(cellY * BlocksPerCell) + 4 + rand() % 2 * 8), Fixed(z - 3));
                                        lamp.scale = 0.1f;
                                        lamp.flags = PlantFlagEmissive;
                                        lamp.rotation = 0.0f;
                                        addPlant(lamp);
                                    }
                                }
                                break;
                            case RoomPart_Dais:
                                // The dais and the central column's cap (8 x 8, four blocks in); the ceiling found at
                                // most ten blocks down.
                                builder.setMaterial(roomColor, TextureBrick, (float)(floorZ - cellarDepth), wallColor,
                                                    TextureBrick, (float)floorZ, trimColor, TextureBrick,
                                                    (float)ceilingZ, false);
                                builder.underpinnedBox(4, 4, floorZ - 1, 8, 8, 1, FillMaterial, CoverAll, false);
                                break;
                            case RoomPart_CentralPillar:
                                builder.setMaterial(roomColor, TextureBrick, (float)(floorZ - cellarDepth), wallColor,
                                                    TextureBrick, (float)floorZ, trimColor, TextureBrick,
                                                    (float)ceilingZ, false);
                                for (int i = 0; i < 10; i++) {
                                    if (isFree(builder.cube(8, 8, z - 1), true, true, true)) {
                                        break;
                                    }
                                    z--;
                                }
                                if (cell.style == DungeonCellStyle_Overgrown) {
                                    builder.roundPillar(BlocksPerCell / 2, BlocksPerCell / 2, floorZ, 2, z - floorZ,
                                                        true);
                                } else if (dungeonType == PlaceCatacombs) {
                                    builder.roundPillar(BlocksPerCell / 2, BlocksPerCell / 2, floorZ, 2, z - floorZ,
                                                        true);
                                } else if (dungeonType == PlaceMines || dungeonType == PlaceGrotto) {
                                    builder.angularColumn(BlocksPerCell / 2, BlocksPerCell / 2, floorZ, 2, z - floorZ,
                                                          true);
                                    builder.box(4, 4, z, 8, 8, 4, false);
                                } else {
                                    builder.angularColumn(BlocksPerCell / 2, BlocksPerCell / 2, floorZ, 2, z - floorZ,
                                                          true);
                                }
                                break;
                            }
                        }
                    }
                    // The corner posts of a house: for the kinds 3, 4, 6, 9 and 10, type 16 and the styles 1, 3, 4 and
                    // 6 (not type 26), one of the three pillars (chosen by a hash of the realm) or, for a kind 1, 9 or
                    // 10 house of style 3 or 6, a square post in the darker colour, at each corner from the lowest
                    // neighbouring level up to the highest top.
                    if ((kind == BuildingKindStonePavilion || kind == BuildingKindCrypt ||
                         kind == BuildingKindDungeonShaftEntrance || kind == BuildingKindGuildHall ||
                         kind == BuildingKindTempleEntrance || type == BuildingTreasurePavilion ||
                         style == HouseStyle_MountainSlabRoof || style == HouseStyle_Timber ||
                         style == HouseStyle_DarkSteepRoof || style == HouseStyle_SavannahLowRoof) &&
                        type != BuildingHouseCourtyard) {
                        PillarStyle pillarStyle = PillarStyle_AngularPillar;
                        if (kind == BuildingKindHouse || kind == BuildingKindGuildHall ||
                            kind == BuildingKindTempleEntrance) {
                            if (style == HouseStyle_Timber || style == HouseStyle_SavannahLowRoof) {
                                pillarStyle = PillarStyle_SquarePost;
                                // The corner pillars' style by a hash of the realm; timber and posts 80% as dark.
                            }
                        } else {
                            pillarStyle = static_cast<PillarStyle>(hash(realmId + 0xdaad) % 3);
                        }
                        builder.setMaterial(trim, TextureBrick, 0.0f, color, TextureBrick, 10.0f, true);
                        if (pillarStyle == PillarStyle_SquarePost) {
                            builder.setColor(color * 0.8f, TexturePlain);
                        }
                        int westType = m_world->zoneBuildingType(cellX - 1, cellY);
                        int eastType = m_world->zoneBuildingType(cellX + 1, cellY);
                        int southType = m_world->zoneBuildingType(cellX, cellY - 1);
                        int northType = m_world->zoneBuildingType(cellX, cellY + 1);
                        int southWestType = m_world->zoneBuildingType(cellX - 1, cellY - 1);
                        int southEastType = m_world->zoneBuildingType(cellX + 1, cellY - 1);
                        int northWestType = m_world->zoneBuildingType(cellX - 1, cellY + 1);
                        int northEastType = m_world->zoneBuildingType(cellX + 1, cellY + 1);
                        int baseLevel = m_world->structureLevel(cellX, cellY, type);
                        int westLevel = m_world->structureLevel(cellX - 1, cellY, westType);
                        int eastLevel = m_world->structureLevel(cellX + 1, cellY, eastType);
                        int southLevel = m_world->structureLevel(cellX, cellY - 1, southType);
                        int northLevel = m_world->structureLevel(cellX, cellY + 1, northType);
                        int southWestLevel = m_world->structureLevel(cellX - 1, cellY - 1, southWestType);
                        int southEastLevel = m_world->structureLevel(cellX + 1, cellY - 1, southEastType);
                        int northEastLevel = m_world->structureLevel(cellX + 1, cellY + 1, northEastType);
                        int northWestLevel = m_world->structureLevel(cellX - 1, cellY + 1, northWestType);
                        // South-west.
                        int postLevel = baseLevel;
                        int postTop = baseLevel + buildingHeight(type);
                        if (hasStoreys(westType)) {
                            postLevel = (std::min)(postLevel, westLevel);
                            postTop = (std::max)(postTop, westLevel + buildingHeight(westType));
                        }
                        if (hasStoreys(southType)) {
                            postLevel = (std::min)(postLevel, southLevel);
                            postTop = (std::max)(postTop, southLevel + buildingHeight(southType));
                        }
                        if (hasStoreys(southWestType)) {
                            postLevel = (std::min)(postLevel, southWestLevel);
                            postTop = (std::max)(postTop, southWestLevel + buildingHeight(southWestType));
                        }
                        if (type != BuildingRockLorePavilion && type != BuildingTreasurePavilion &&
                            type != BuildingRuinedPavilion && type != BuildingRuinedFlatRoofPavilion &&
                            type != BuildingBirdStatuePavilion && type != BuildingShrineOfLife &&
                            type != BuildingFloatingIslandTreasurePavilion && type != BuildingSewerOrGrottoEntrance &&
                            type != BuildingPyramidEntrance && type != BuildingTempleEntrance &&
                            type != BuildingArenaPodium && kind != BuildingKindGuildHall) {
                            builder.m_guard = PlacementKeepSolid | PlacementKeepProtected;
                        }
                        switch (pillarStyle) {
                        case PillarStyle_AngularPillar:
                            builder.angularPillar(0, 0, postLevel - 1, 1, postTop - postLevel - 1, true);
                            break;
                        case PillarStyle_AngularColumn:
                            builder.angularColumn(0, 0, postLevel - 1, 1, postTop - postLevel - 1, true);
                            break;
                        case PillarStyle_RoundPillar:
                            builder.roundPillar(0, 0, postLevel - 1, 1, postTop - postLevel - 1, true);
                            break;
                        case PillarStyle_SquarePost:
                            builder.underpinnedBox(-1, -1, postLevel - 1, 2, 2, postTop - postLevel + 1, FillMaterial,
                                                   CoverAll, false);
                            break;
                        }
                        // South-east.
                        postLevel = baseLevel;
                        postTop = baseLevel + buildingHeight(type);
                        if (hasStoreys(eastType)) {
                            postLevel = (std::min)(postLevel, eastLevel);
                            postTop = (std::max)(postTop, eastLevel + buildingHeight(eastType));
                        }
                        if (hasStoreys(southType)) {
                            postLevel = (std::min)(postLevel, southLevel);
                            postTop = (std::max)(postTop, southLevel + buildingHeight(southType));
                        }
                        if (hasStoreys(southEastType)) {
                            postLevel = (std::min)(postLevel, southEastLevel);
                            postTop = (std::max)(postTop, southEastLevel + buildingHeight(southEastType));
                        }
                        switch (pillarStyle) {
                        case PillarStyle_AngularPillar:
                            builder.angularPillar(16, 0, postLevel - 1, 1, postTop - postLevel - 1, true);
                            break;
                        case PillarStyle_AngularColumn:
                            builder.angularColumn(16, 0, postLevel - 1, 1, postTop - postLevel - 1, true);
                            break;
                        case PillarStyle_RoundPillar:
                            builder.roundPillar(16, 0, postLevel - 1, 1, postTop - postLevel - 1, true);
                            break;
                        case PillarStyle_SquarePost:
                            builder.underpinnedBox(15, -1, postLevel - 1, 2, 2, postTop - postLevel + 1, FillMaterial,
                                                   CoverAll, false);
                            break;
                        }
                        // North-west.
                        postLevel = baseLevel;
                        postTop = baseLevel + buildingHeight(type);
                        if (hasStoreys(westType)) {
                            postLevel = (std::min)(postLevel, westLevel);
                            postTop = (std::max)(postTop, westLevel + buildingHeight(westType));
                        }
                        if (hasStoreys(northType)) {
                            postLevel = (std::min)(postLevel, northLevel);
                            postTop = (std::max)(postTop, northLevel + buildingHeight(northType));
                        }
                        if (hasStoreys(northWestType)) {
                            postLevel = (std::min)(postLevel, northWestLevel);
                            postTop = (std::max)(postTop, northWestLevel + buildingHeight(northWestType));
                        }
                        switch (pillarStyle) {
                        case PillarStyle_AngularPillar:
                            builder.angularPillar(0, 16, postLevel - 1, 1, postTop - postLevel - 1, true);
                            break;
                        case PillarStyle_AngularColumn:
                            builder.angularColumn(0, 16, postLevel - 1, 1, postTop - postLevel - 1, true);
                            break;
                        case PillarStyle_RoundPillar:
                            builder.roundPillar(0, 16, postLevel - 1, 1, postTop - postLevel - 1, true);
                            break;
                        case PillarStyle_SquarePost:
                            builder.underpinnedBox(-1, 15, postLevel - 1, 2, 2, postTop - postLevel + 1, FillMaterial,
                                                   CoverAll, false);
                            break;
                        }
                        // North-east.
                        postLevel = baseLevel;
                        postTop = baseLevel + buildingHeight(type);
                        if (hasStoreys(eastType)) {
                            postLevel = (std::min)(postLevel, eastLevel);
                            postTop = (std::max)(postTop, eastLevel + buildingHeight(eastType));
                        }
                        if (hasStoreys(northType)) {
                            postLevel = (std::min)(postLevel, northLevel);
                            postTop = (std::max)(postTop, northLevel + buildingHeight(northType));
                        }
                        if (hasStoreys(northEastType)) {
                            postLevel = (std::min)(postLevel, northEastLevel);
                            postTop = (std::max)(postTop, northEastLevel + buildingHeight(northEastType));
                        }
                        switch (pillarStyle) {
                        case PillarStyle_AngularPillar:
                            builder.angularPillar(16, 16, postLevel - 1, 1, postTop - postLevel - 1, true);
                            break;
                        case PillarStyle_AngularColumn:
                            builder.angularColumn(16, 16, postLevel - 1, 1, postTop - postLevel - 1, true);
                            break;
                        case PillarStyle_RoundPillar:
                            builder.roundPillar(16, 16, postLevel - 1, 1, postTop - postLevel - 1, true);
                            break;
                        case PillarStyle_SquarePost:
                            builder.underpinnedBox(15, 15, postLevel - 1, 2, 2, postTop - postLevel + 1, FillMaterial,
                                                   CoverAll, false);
                            break;
                        }
                        builder.m_guard = 0;
                    }
                }
                // Style 1 lays two slabs under the roof in the wall colour, style 3 one in the darker house colour;
                // type 88 gets a spire.
                if (hasRoof(type, realmId, realmForm, biome)) {
                    unsigned int sides = m_world->streetSides(cellX, cellY);
                    int left = (sides & JoinedSideMinusX) ? 0 : -overhang;
                    int right = (sides & JoinedSidePlusX) == 0 ? overhang + 16 : 16;
                    int bottom = (sides & JoinedSideMinusY) ? 0 : -overhang;
                    int top = (sides & JoinedSidePlusY) == 0 ? overhang + 16 : 16;
                    m_world->adjustOpenSides(cellX, cellY, type, &sides);
                    builder.setColor(roofColor(type, cellX, cellY, realmId, realmForm, biome), TexturePlain);
                    StructureBuilder::Material roof = builder.m_material;
                    builder.m_guard = PlacementKeepSolid | PlacementKeepProtected;
                    // The roof slab: three blocks over the walls, then one.
                    if (style == HouseStyle_MountainSlabRoof) {
                        builder.setMaterial(wall);
                        builder.box(-3, -3, level + height - 2, 22, 22, 2, false);
                        builder.box(-1, -1, level + height - 3, 18, 18, 1, false);
                    } else if (style == HouseStyle_Timber) {
                        builder.setColor(color * 0.8f, TexturePlain);
                        builder.box(-1, -1, level + height - 2, 18, 18, 2, false);
                    }
                    for (int x = left; x < right; x++) {
                        for (int y = bottom; y < top; y++) {
                            int roofZ = m_world->roofHeight(x, y, type, sides, style);
                            if ((x >= 0 && x < 16 && y >= 0 && y < 16) || roofed) {
                                builder.setMaterial(wall);
                                for (int z = roofed ? -2 : 0; z < roofZ; z++) {
                                    Block block;
                                    block = BlockProperties<float>(builder.colorAt(x, y, level + height + z),
                                                                   BlockBuilding, 0, 0);
                                    builder.setCubeBlock(x, y, level + height + z, block);
                                }
                            }
                            builder.setMaterial(roof);
                            int roofTop = level + height + roofZ;
                            Block block;
                            block = BlockProperties<float>(builder.colorAt(x, y, roofTop), BlockBuilding, 0, 0);
                            builder.setCubeBlock(x, y, roofTop, block);
                            if (roofZ - m_world->roofHeight(x - 1, y, type, sides, style) > 1 ||
                                roofZ - m_world->roofHeight(x + 1, y, type, sides, style) > 1 ||
                                roofZ - m_world->roofHeight(x, y - 1, type, sides, style) > 1 ||
                                roofZ - m_world->roofHeight(x, y + 1, type, sides, style) > 1) {
                                ++roofTop;
                                Block step;
                                step = BlockProperties<float>(builder.colorAt(x, y, roofTop), BlockBuilding, 0, 0);
                                builder.setCubeBlock(x, y, roofTop, step);
                            }
                            World::AreaInfo areaInfo;
                            m_world->fillAreaInfo(cellX * BlocksPerCell + x, cellY * BlocksPerCell + y, areaInfo);
                            World::BiomeSample sample;
                            m_world->biomeSample(cellX * BlocksPerCell + x, cellY * BlocksPerCell + y, sample);
                            float temperature = sample.temperature(areaInfo.type, areaInfo.influence);
                            if (m_world->hasLyingSnow(cellX * BlocksPerCell + x, cellY * BlocksPerCell + y, temperature,
                                                      (float)level, (float)level, (float)level, (float)level) != 0.0f) {
                                Block snow;
                                snow = BlockProperties<float>(
                                    m_world->snowColor(cellX * BlocksPerCell + x, cellY * BlocksPerCell + y), BlockSnow,
                                    BlockFlagDestructible, 0);
                                builder.setCubeBlock(x, y, roofTop + 1, snow);
                            }
                        }
                    }
                    // The chimney: six blocks square and 28 high, its colour darkening to half over 25 blocks.
                    if (type == BuildingOldHutChimney) {
                        builder.m_guard = 0;
                        builder.setMaterial(color, TextureBrick, (float)(level + height), color * 0.5f, TextureBrick,
                                            (float)(level + height + 25), false);
                        builder.box(5, 5, level + height - 3, 6, 6, 28, false);
                        builder.cutBox(6, 6, level + height - 3, 4, 4, 28, false);
                    }
                    builder.m_guard = 0;
                }
                builder.setMaterial(trim, TextureBrick, 0.0f, color, TextureBrick, 10.0f, true);
                // The special types: a slab with a type 0, 1 or 2 entity (53, 51, 61) or a lying item of category 30
                // (14, 16, 60) in the middle of an inner cell; four posts in the house colour for 61 and 57.
                if (type == BuildingShrineOfLife) {
                    // The pedestal: 8 x 8 blocks, two high, four in.
                    if (!outside) {
                        int z = m_world->structureLevel(cellX, cellY, type);
                        builder.box(4, 4, z, 8, 8, 2, false);
                        StaticEntity entity;
                        entity.type = StaticEntityGoddess2;
                        entity.orientation = rand() % 4;
                        entity.position = plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 8),
                                                                   Fixed(cellY * BlocksPerCell + 8), Fixed(z + 2));
                        addStaticEntity(entity);
                    }
                } else {
                    if ((type == BuildingRockTreasure || type == BuildingTreasurePavilion ||
                         type == BuildingFloatingIslandTreasurePavilion) &&
                        !outside) {
                        int z = m_world->structureLevel(cellX, cellY, type);
                        builder.box(4, 4, z, 8, 8, 2, false);
                        ZoneItem item;
                        item.rotation = (float)rand() / (float)RAND_MAX * 360.0f;
                        item.item.category = ItemCategoryGeneratedItem;
                        item.item.id = rand() % (GeneratedItemGoldBag - GeneratedItemEquipment0 + 1);
                        item.item.rarity = rand() % (ItemRarityYellow - ItemRarityPurple + 1) + ItemRarityPurple;
                        // The treasure's modifier by a hash of the cell (salts 8883, 983049823).
                        item.item.modifier = fastRandom(cellX + 8883, cellY + 983049823);
                        item.position = plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 8),
                                                                 Fixed(cellY * BlocksPerCell + 8), Fixed(z + 2));
                        item.flags |= ZoneItemFlagPerPlayer;
                        m_items.push_back(item);
                    }
                    if (type == BuildingBirdStatuePavilion) {
                        if (!outside) {
                            int z = m_world->structureLevel(cellX, cellY, type);
                            StaticEntity entity;
                            entity.type = StaticEntityBirdStatue;
                            entity.orientation = rand() % 4;
                            entity.position = plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 8),
                                                                       Fixed(cellY * BlocksPerCell + 8), Fixed(z));
                            addStaticEntity(entity);
                        }
                    } else if (type == BuildingMourningStatuePavilion) {
                        int z = m_world->structureLevel(cellX, cellY, type);
                        StructureBuilder posts(*this, cellX * BlocksPerCell, cellY * BlocksPerCell, 0, BlocksPerCell,
                                               BlocksPerCell, 0, false, nullptr);
                        // Statue and altar posts: ten blocks high.
                        plasma::Vector<3, float> postColor =
                            buildingColor(type, cellX, cellY, realmId, realmForm, biome);
                        posts.setMaterial(postColor, TextureBrick, 0.0f, postColor, TextureBrick, 20.0f, true);
                        posts.angularPillar(0, 0, z, 1, 10, true);
                        posts.angularPillar(15, 0, z, 1, 10, true);
                        posts.angularPillar(15, 15, z, 1, 10, true);
                        posts.angularPillar(0, 15, z, 1, 10, true);
                        if (!outside) {
                            StaticEntity entity;
                            entity.type = StaticEntityMourningStatue;
                            entity.orientation = rand() % 4;
                            entity.position = plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 8),
                                                                       Fixed(cellY * BlocksPerCell + 8), Fixed(z));
                            addStaticEntity(entity);
                        }
                    } else if (type == BuildingAltar) {
                        int z = m_world->structureLevel(cellX, cellY, type);
                        StructureBuilder posts(*this, cellX * BlocksPerCell, cellY * BlocksPerCell, 0, BlocksPerCell,
                                               BlocksPerCell, 0, false, nullptr);
                        plasma::Vector<3, float> postColor =
                            buildingColor(type, cellX, cellY, realmId, realmForm, biome);
                        posts.setMaterial(postColor, TextureBrick, 0.0f, postColor, TextureBrick, 20.0f, true);
                        posts.angularPillar(0, 0, z, 1, 10, true);
                        posts.angularPillar(15, 0, z, 1, 10, true);
                        posts.angularPillar(15, 15, z, 1, 10, true);
                        posts.angularPillar(0, 15, z, 1, 10, true);
                    }
                }
                // A workstation of the plot (7: furnace, anvil or 104; 8: saw or workbench; 9: loom or spinning wheel,
                // by the house's part), at a random spot of the inner 8 x 8 blocks.
                if (plot >= PlotSmithy && plot <= PlotTailorsShop) {
                    int housePart = m_world->housePart(cellX, cellY);
                    plasma::Vector<3, Fixed> position((Fixed(cellX * BlocksPerCell) + 4 + rand() % 8 + 0.5f),
                                                      (Fixed(cellY * BlocksPerCell) + 4 + rand() % 8 + 0.5f),
                                                      Fixed(level));
                    StaticEntity workstation;
                    workstation.type = StaticEntityGoddess2;
                    workstation.position = position;
                    workstation.orientation = rand() % 4;
                    switch (plot) {
                    case PlotSmithy:
                        if (housePart == 1) {
                            workstation.type = StaticEntityFurnace;
                        } else if (housePart == 2) {
                            workstation.type = StaticEntityAnvil;
                        } else if (housePart == 3) {
                            workstation.type = StaticEntityCustomizationBench;
                        }
                        break;
                    case PlotCarpentersShop:
                        if (housePart == 1) {
                            workstation.type = StaticEntitySawbench;
                        } else if (housePart == 2) {
                            workstation.type = StaticEntityWorkbench;
                        }
                        break;
                    case PlotTailorsShop:
                        if (housePart == 1) {
                            workstation.type = StaticEntityLoom;
                        } else if (housePart == 2) {
                            workstation.type = StaticEntitySpinningwheel;
                        }
                        break;
                    }
                    if (workstation.type != StaticEntityGoddess2) {
                        putOnGround(workstation, true);
                        addStaticEntity(workstation);
                    }
                }
            }
            // 5e. The random generator is seeded by the cell.
            if (type != BuildingNone) {
                // Landmarks stand on the cell's street point unless this type requires the geometric centre of the
                // cell.
                if (type == BuildingGreatOak || type == BuildingRock || type == BuildingTallRock ||
                    type == BuildingRockLorePavilion || type == BuildingRockTreasure ||
                    type == BuildingInscribedMenhir) {
                    center[0] = cellX * BlocksPerCell + 8;
                    center[1] = cellY * BlocksPerCell + 8;
                }
                float groundHeight = m_world->height(center[0], center[1], HeightFlagCaveMouths);
                bool sunken = groundHeight < 0.0f;
                if (sunken) {
                    groundHeight = 0.0f;
                }
                World::BiomeSample sample;
                m_world->biomeSample(center[0], center[1], sample);
                World::AreaInfo areaInfo;
                m_world->fillAreaInfo(center[0], center[1], areaInfo);
                int landmarkHorizontalExtent = m_world->townSizeD(cellX, cellY, 1.0f);
                // The large landmarks: 50 blocks across; a great oak three times as tall.
                int landmarkHeight = m_world->townSizeC(cellX, cellY, 1.0f);
                if (type == BuildingGreatOak) {
                    landmarkHorizontalExtent = 50;
                    landmarkHeight = landmarkHeight * 3;
                } else {
                    if (type == BuildingRock || type == BuildingRockLorePavilion) {
                        landmarkHorizontalExtent = 64;
                        landmarkHeight = 40;
                    }
                    if (type == BuildingTallRock || type == BuildingRockTreasure) {
                        landmarkHorizontalExtent = 64;
                        landmarkHeight = 80;
                    }
                }
                if (!isOutsideZone(center[0], center[1], (int)((float)landmarkHorizontalExtent * 1.25f), m_position[0],
                                   m_position[1])) {
                    // The landmark's draws are seeded by a hash of its cell (modulo 10000000).
                    srand(fastRandom(cellX, cellY) % 10000000);
                    groundHeight += 1.0f;
                    switch (type) {
                    case BuildingOak:
                    case BuildingGreatOak: {
                        LandmarkBuilder landmark(this);
                        landmark.oak(center[0], center[1], (int)groundHeight, landmarkHorizontalExtent, landmarkHeight);
                        break;
                    }
                    case BuildingEvergreen: {
                        LandmarkBuilder landmark(this);
                        landmark.evergreen(center[0], center[1], (int)groundHeight, landmarkHorizontalExtent,
                                           landmarkHeight);
                        break;
                    }
                    case BuildingBirch: {
                        LandmarkBuilder landmark(this);
                        landmark.birch(center[0], center[1], (int)groundHeight, landmarkHorizontalExtent,
                                       landmarkHeight);
                        break;
                    }
                    case BuildingJungleTree: {
                        LandmarkBuilder landmark(this);
                        landmark.jungleTree(center[0], center[1], (int)groundHeight, landmarkHorizontalExtent,
                                            landmarkHeight);
                        break;
                    }
                    case BuildingPalm: {
                        LandmarkBuilder landmark(this);
                        landmark.radialPalm(center[0], center[1], (int)groundHeight, landmarkHorizontalExtent,
                                            landmarkHeight);
                        break;
                    }
                    case BuildingOakPlaza:
                    case BuildingPalmPlaza: {
                        // The plaza pedestal: 80% of the landmark's extent. A monument on a pedestal with a statue (31
                        // in undead lands) on each side.
                        int radius = (int)((float)landmarkHorizontalExtent * 0.8f);
                        int z = (int)(groundHeight + 3.0f);
                        if (type == BuildingPalmPlaza) {
                            LandmarkBuilder landmark(this);
                            landmark.radialPalm(center[0], center[1], z, radius, landmarkHeight);
                        } else {
                            LandmarkBuilder landmark(this);
                            landmark.oak(center[0], center[1], z, radius, landmarkHeight);
                        }
                        StructureBuilder pedestal(*this, nullptr);
                        pedestal.setColor(plasma::Vector<3, float>(240.0f, 240.0f, 240.0f), TextureBrick);
                        pedestal.foundation(center[0], center[1], (int)groundHeight, 6, 3, FillMaterial, CoverAll);
                        pedestal.cylinder(center[0], center[1], (int)groundHeight, 5, 3, FillGrass, CoverAll, 0);
                        StaticEntity plazaBench;
                        plazaBench.type = StaticEntityBench;
                        // The plaza's benches (black in undead lands, above 0.5) six blocks from the centre.
                        World::BiomeSample centerSample;
                        m_world->biomeSample(center[0], center[1], centerSample);
                        if (centerSample.undead(0, 0.0f) > 0.5f) {
                            plazaBench.type = StaticEntityBenchBlack;
                        }
                        groundHeight += 2.0f;
                        plazaBench.position =
                            plasma::Vector<3, Fixed>(Fixed(center[0]), Fixed(center[1] - 6), Fixed(groundHeight));
                        plazaBench.orientation = QuarterTurn_Zero;
                        if (putOnGround(plazaBench, true)) {
                            addStaticEntity(plazaBench);
                        }
                        plazaBench.position =
                            plasma::Vector<3, Fixed>(Fixed(center[0] + 8), Fixed(center[1]), Fixed(groundHeight));
                        plazaBench.orientation = QuarterTurn_One;
                        if (putOnGround(plazaBench, true)) {
                            addStaticEntity(plazaBench);
                        }
                        plazaBench.position =
                            plasma::Vector<3, Fixed>(Fixed(center[0]), Fixed(center[1] + 8), Fixed(groundHeight));
                        plazaBench.orientation = QuarterTurn_Two;
                        if (putOnGround(plazaBench, true)) {
                            addStaticEntity(plazaBench);
                        }
                        plazaBench.position =
                            plasma::Vector<3, Fixed>(Fixed(center[0] - 6), Fixed(center[1]), Fixed(groundHeight));
                        plazaBench.orientation = QuarterTurn_Three;
                        if (putOnGround(plazaBench, true)) {
                            addStaticEntity(plazaBench);
                        }
                        break;
                    }
                    case BuildingBoulder: {
                        CaveBuilder cave(this);
                        cave.raiseBoulder(center[0], center[1], (int)groundHeight, landmarkHorizontalExtent,
                                          landmarkHorizontalExtent, landmarkHeight, 0);
                        break;
                    }
                    case BuildingHill: {
                        float streetAmount =
                            m_world->streetAmount(center[0], center[1], areaInfo.type, areaInfo.influence);
                        float mountains = sample.mountains();
                        float lava = sample.lavaWeight(areaInfo.type, areaInfo.influence);
                        float undead = sample.undead(areaInfo.type, areaInfo.influence);
                        float poisonChannels =
                            m_world->poisonChannels(center[0], center[1], streetAmount, undead, mountains, &areaInfo);
                        float lavaChannels =
                            m_world->lavaChannels(center[0], center[1], streetAmount, lava, mountains, &areaInfo);
                        float wetness = sample.wetness();
                        float valley = m_world->valleyWater(center[0], center[1], streetAmount, lava, undead, wetness,
                                                            mountains, &areaInfo);
                        float elevation = sample.biomeElevation(areaInfo.type, areaInfo.influence);
                        float mountainCliffs =
                            m_world->mountainCliffs(center[0], center[1], elevation, mountains, valley, lavaChannels,
                                                    poisonChannels, streetAmount, &areaInfo);
                        if (mountainCliffs > 0.5f) {
                            CaveBuilder cave(this);
                            cave.addPeak(center[0], center[1], (int)((float)landmarkHeight + groundHeight),
                                         2 * landmarkHorizontalExtent, 2 * landmarkHorizontalExtent);
                        } else {
                            CaveBuilder cave(this);
                            cave.addHill(center[0], center[1], (int)((float)landmarkHeight + groundHeight),
                                         2 * landmarkHorizontalExtent, 2 * landmarkHorizontalExtent);
                        }
                        break;
                    }
                    case BuildingRock:
                    case BuildingTallRock:
                    case BuildingRockLorePavilion:
                    case BuildingRockTreasure: {
                        CaveBuilder cave(this);
                        cave.addPeak(center[0], center[1], (int)((float)landmarkHeight + groundHeight),
                                     landmarkHorizontalExtent, landmarkHorizontalExtent);
                        break;
                    }
                    case BuildingCave: {
                        float strata = m_world->streetAmount(center[0], center[1], areaInfo.type, areaInfo.influence);
                        float mountains = sample.mountains();
                        float lava = sample.lavaWeight(areaInfo.type, areaInfo.influence);
                        float undead = sample.undead(areaInfo.type, areaInfo.influence);
                        float peak =
                            m_world->poisonChannels(center[0], center[1], strata, undead, mountains, &areaInfo);
                        float land = m_world->lavaChannels(center[0], center[1], strata, lava, mountains, &areaInfo);
                        float wetness = sample.wetness();
                        float valley = m_world->valleyWater(center[0], center[1], strata, lava, undead, wetness,
                                                            mountains, &areaInfo);
                        float elevation = sample.biomeElevation(areaInfo.type, areaInfo.influence);
                        float mound = m_world->mountainCliffs(center[0], center[1], elevation, mountains, valley, land,
                                                              peak, strata, &areaInfo);
                        if (mound > 0.5f) {
                            bool lined = rand() % 2 != 0;
                            int z = (int)((float)landmarkHeight + groundHeight);
                            CaveBuilder cave(this);
                            cave.addCaveRock(center[0], center[1], z, 2 * landmarkHorizontalExtent,
                                             2 * landmarkHorizontalExtent, lined);
                        } else {
                            bool flag = rand() % 2 != 0;
                            int z = (int)((float)landmarkHeight + groundHeight);
                            CaveBuilder cave(this);
                            cave.addCaveSoil(center[0], center[1], z, 2 * landmarkHorizontalExtent,
                                             2 * landmarkHorizontalExtent, flag);
                        }
                        break;
                    }
                    case BuildingStump: {
                        LandmarkBuilder landmark(this);
                        landmark.stump(center[0], center[1], (int)groundHeight, landmarkHorizontalExtent,
                                       landmarkHeight);
                        break;
                    }
                    // A fallen trunk: hollow for one in three, radius 2..5.
                    case BuildingFallenTrunk: {
                        if (rand() % 2 != 0) {
                            bool hollow = rand() % 3 == 0;
                            float trunkRadius = (float)(rand() % 4 + 2);
                            LandmarkBuilder landmark(this);
                            landmark.prostrateTrunkX(center[0], center[1], (int)groundHeight, trunkRadius,
                                                     landmarkHorizontalExtent, hollow);
                        } else {
                            bool flag = rand() % 3 == 0;
                            float scale = (float)(rand() % 4 + 2);
                            LandmarkBuilder landmark(this);
                            landmark.prostrateTrunkY(center[0], center[1], (int)groundHeight, scale,
                                                     landmarkHorizontalExtent, flag);
                        }
                        break;
                    }
                    case BuildingTreasureTrunk: {
                        if (rand() % 2 != 0) {
                            float scale = (float)(rand() % 4 + 3);
                            LandmarkBuilder landmark(this);
                            landmark.prostrateTrunkX(center[0], center[1], (int)groundHeight, scale,
                                                     landmarkHorizontalExtent, true);
                        } else {
                            float scale = (float)(rand() % 4 + 3);
                            LandmarkBuilder landmark(this);
                            landmark.prostrateTrunkY(center[0], center[1], (int)groundHeight, scale,
                                                     landmarkHorizontalExtent, true);
                        }
                        if (!outside) {
                            // A lying item of category 30 at the centre.
                            ZoneItem item;
                            item.item.category = ItemCategoryGeneratedItem;
                            item.item.id = rand() % (GeneratedItemEquipment2 - GeneratedItemEquipment0 + 1);
                            item.item.modifier = rand();
                            item.item.rarity = rand() % 3 + 2;
                            item.item.region = m_world->findRegion(center[0], center[1]);
                            item.position = plasma::Vector<3, Fixed>((Fixed(center[0]) + 0.5f),
                                                                     (Fixed(center[1]) + 0.5f), Fixed(groundHeight));
                            item.flags |= ZoneItemFlagPerPlayer;
                            m_items.push_back(item);
                        }
                        break;
                    }
                    case BuildingLoreCrypt:
                    case BuildingBirdStatuePavilion:
                    case BuildingShrineOfLife:
                    case BuildingSanctuary:
                    case BuildingRaisedSanctuary:
                        // A sunken landmark: a peak eight blocks down, twice the extent wide.
                        if (sunken) {
                            CaveBuilder cave(this);
                            cave.addPeak(center[0], center[1], -8, 2 * landmarkHorizontalExtent,
                                         2 * landmarkHorizontalExtent);
                        }
                        break;
                    case BuildingMenhir:
                    case BuildingInscribedMenhir: {
                        if (sunken) {
                            CaveBuilder cave(this);
                            cave.addPeak(center[0], center[1], -8, 2 * landmarkHorizontalExtent,
                                         2 * landmarkHorizontalExtent);
                        }
                        CaveBuilder cave(this);
                        cave.raiseRock(center[0], center[1], (int)groundHeight, 8, 8, 25, BlockFlagNotTerrain);
                        break;
                    }
                    case BuildingBuriedTreasureChamber: {
                        // The terrain within 16 blocks of the cell's centre is levelled towards 16 blocks below its
                        // height (the weight falls off with the distance), the column above it cleared and its cover
                        // block darkened by noise; four posts, a slab and a reward chest (type 22) are placed on it.
                        int x0 = cellX * BlocksPerCell + 8;
                        int y0 = cellY * BlocksPerCell + 8;
                        int base = (int)(m_world->height(x0, y0, HeightFlagCaveMouths) - 16.0f);
                        for (int x = x0 - 16; x <= x0 + 16; x++) {
                            for (int y = y0 - 16; y <= y0 + 16; y++) {
                                plasma::Vector<2, float> offset((float)(x - x0), (float)(y - y0));
                                float distance = offset.squaredLength() / 256.0f;
                                if (distance > 1.0f) {
                                    continue;
                                }
                                Field *column = field(x, y);
                                if (column == nullptr) {
                                    continue;
                                }
                                float weight = (std::max)(0.0f, (distance - 0.5f) * 2.0f);
                                weight = 1.0f - weight * weight;
                                float terrain = m_world->height(x, y, HeightFlagCaveMouths);
                                int top = floor((float)base * weight + (1.0f - weight) * terrain);
                                int ground = floor(terrain);
                                Block block;
                                block = m_world->computeBlock(x, y, top);
                                setBlock(x, y, top - 1, block);
                                if (top <= ground + 1) {
                                    for (int z = top; z <= ground + 1; z++) {
                                        if (z == ground - 1) {
                                            continue;
                                        }
                                        Block air(plasma::Vector<3, unsigned char>(0, 0, 0), BlockAir, 0, 0);
                                        setBlock(x, y, z, air);
                                    }
                                }
                                int cover = ground;
                                if (column->m_sandAmount <= 0.5f) {
                                    cover = ground - 1;
                                    // The buried treasure's cover, shaded 70..100% by noise (0.25).
                                }
                                BlockProperties<float> properties = m_world->computeBlock(x, y, cover);
                                properties.color *= abs(noise(x * 0.25, y * 0.25)) * 0.3f + 0.7f; // the CRT abs(float)
                                properties.flags |= BlockFlagBombable;
                                Block covering;
                                covering = properties;
                                setBlock(x, y, cover, covering);
                                Block under;
                                under = properties;
                                setBlock(x, y, cover - 1, under);
                            }
                        }
                        StructureBuilder posts(*this, cellX * BlocksPerCell, cellY * BlocksPerCell, 0, BlocksPerCell,
                                               BlocksPerCell, 0, false, nullptr);
                        // The buried chamber: a pedestal 8 x 8, two high, and columns ten high.
                        plasma::Vector<3, float> postColor =
                            buildingColor(type, cellX, cellY, realmId, realmForm, biome);
                        posts.setMaterial(postColor, TextureBrick, 0.0f, postColor, TextureBrick, 20.0f, true);
                        posts.box(4, 4, base, 8, 8, 2, false);
                        posts.angularColumn(0, 0, base, 1, 10, true);
                        posts.angularColumn(15, 0, base, 1, 10, true);
                        posts.angularColumn(15, 15, base, 1, 10, true);
                        posts.angularColumn(0, 15, base, 1, 10, true);
                        StaticEntity chest;
                        chest.type = StaticEntityRewardChest;
                        chest.position =
                            plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 8), Fixed(y0), Fixed(base + 2));
                        chest.orientation = rand() % 4;
                        putOnGround(chest, true);
                        Item item;
                        item.category = ItemCategoryGeneratedItem;
                        item.id = rand() % 3;
                        item.modifier = rand();
                        item.rarity = rand() % 2 + 3;
                        item.region = m_world->findRegion((int)chest.position[0], (int)chest.position[1]);
                        if (addStaticEntity(chest)) {
                            m_objectRewards[(int)m_staticEntities.size() - 1] = item;
                        }
                        break;
                    }
                    }
                }
            }
        }
    }

    // 6.
    if (info.type == PlaceVillage || info.type == PlaceGraveyard) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if ((i + j * 3) % 5 != 0) {
                    continue;
                }
                int x = m_position[0] * BlocksPerZone + BlocksPerCell / 2 + i * BlocksPerCell;
                int y = m_position[1] * BlocksPerZone + BlocksPerCell / 2 + j * BlocksPerCell;
                float influence =
                    m_world->placeInfluence(info.type, info.large, m_world->placeSquaredDistance(info.center, x, y));
                if (influence < 0.5f) {
                    continue;
                }
                if (m_world->zoneBuildingType(toCell(x), toCell(y)) != BuildingNone) {
                    continue;
                }
                float streetAmount = m_world->streetAmount(x, y, info.type, influence);
                if (streetAmount > 0.5f && streetAmount < 0.8f) {
                    StaticEntity streetLight; // Leaves orientation uninitialized; putOnGround reads it.
                    World::BiomeSample sample;
                    m_world->biomeSample(x, y, sample);
                    // Street lights by climate: dark in undead lands, the hot kind above temperature 0.85.
                    if (sample.undead(info.type, influence) > 0.5f) {
                        streetLight.type = StaticEntityStreetLightUndead;
                    } else if (sample.temperature(info.type, influence) > 0.85f) {
                        streetLight.type = StaticEntityStreetLight01;
                    } else {
                        streetLight.type = (fastRandom(placeX, placeY) & 1) + StaticEntityStreetLight02;
                    }
                    streetLight.position = plasma::Vector<3, Fixed>((Fixed(x) + 0.5f), (Fixed(y) + 0.5f), Fixed(0));
                    streetLight.position[2] =
                        m_world->height((int)streetLight.position[0], (int)streetLight.position[1], 0) + 1.0f;
                    if (putOnGround(streetLight, true)) {
                        m_staticEntities.push_back(streetLight);
                    }
                }
            }
        }
    }
    // The spots of the zone: every second column, jittered by a block, from the base of its field up: a spot in water
    // over solid ground (kind 18), every spot under a free block (kind 0), and on free terrain its kind by the ground
    // block: 5 on type 18, 7..14 on the types 23..30, 6 and 2 by the flags of the block above, else 1 above sea level
    // (with 16 for the ground types 4, 5, 9, 10 and 14 not under water, and 17 for type 6).
    for (int x = 0; x < BlocksPerZone; x += 2) {
        for (int y = 0; y < BlocksPerZone; y += 2) {
            Sleep(0);
            int px = m_position[0] * BlocksPerZone + x + rand() % 2;
            int py = m_position[1] * BlocksPerZone + y + rand() % 2;
            int cellX = toCell(px);
            int cellY = toCell(py);
            Field *column = field(px, py);
            for (int z = -1; z < (int)column->size(); z++) {
                int worldZ = column->m_baseZ + 1 + z;
                Block block = column->getBlock(z);
                Block above = column->getBlock(z + 1);
                if (column->m_valleyWater > 0.5f && isSolid(block) && above.type == BlockWater) {
                    m_positionsByKind[PositionKey {{cellX, cellY, PositionUnderwater}}].push_back(
                        plasma::Vector<3, Fixed>((Fixed(px) + 0.5), (Fixed(py) + 0.5), Fixed(worldZ)));
                }
                if (!isSolid(above)) {
                    plasma::Vector<3, Fixed> spot((Fixed(px) + 0.5), (Fixed(py) + 0.5), Fixed(worldZ));
                    m_positionsByKind[PositionKey {{cellX, cellY, PositionAny}}].push_back(spot);
                    if (isTerrain(block) && isSpaceFree(px, py, worldZ, 1)) {
                        if (block.type == BlockCave) {
                            m_positionsByKind[PositionKey {{cellX, cellY, PositionCave}}].push_back(spot);
                        } else if (block.type >= BlockStorey0Floor && block.type <= BlockStorey7Floor) {
                            m_positionsByKind[PositionKey {{cellX, cellY,
                                                            block.type - BlockStorey0Floor + PositionStorey0Floor}}]
                                .push_back(spot);
                        } else if (above.flags & BlockFlagDungeon) {
                            m_positionsByKind[PositionKey {{cellX, cellY, PositionDungeonFloor}}].push_back(spot);
                        } else if (above.flags & BlockFlagProtected) {
                            m_positionsByKind[PositionKey {{cellX, cellY, PositionSheltered}}].push_back(spot);
                        } else if (worldZ > 0 && !(block.flags & BlockFlagProtected)) {
                            m_positionsByKind[PositionKey {{cellX, cellY, PositionGround}}].push_back(spot);
                            if ((block.type == BlockGrass || block.type == BlockGround || block.type == BlockSand ||
                                 block.type == BlockSnow || block.type == BlockRock) &&
                                above.type != BlockWater) {
                                m_positionsByKind[PositionKey {{cellX, cellY, PositionNaturalGround}}].push_back(spot);
                            }
                            if (block.type == BlockBuilding) {
                                m_positionsByKind[PositionKey {{cellX, cellY, PositionBuildingFloor}}].push_back(spot);
                            }
                        }
                    }
                }
            }
        }
    }
    m_remainingPositions = m_positionsByKind;
    // The reward chests and lore of the zone's cells: a kind 5 cell gets a chest (type 22) with the place's reward (a
    // category 30 item of rarity 4, or 5 one time in ten, when the missions give none), a kind 3 cell one with an item
    // whose rarity follows the cell's side; a cell with a mission (not of the types 21, 60 and 62) gets a category 30
    // item lying at a random spot or in a chest; a cell with lore gets a type 125 entity (type 55, moved south while
    // blocked) or an entity of type 130 (37) or 118..124 by the place's realm.
    for (int cellX = toCell(m_position[0] * BlocksPerZone);
         cellX < toCell(m_position[0] * BlocksPerZone + BlocksPerZone); cellX++) {
        for (int cellY = toCell(m_position[1] * BlocksPerZone);
             cellY < toCell(m_position[1] * BlocksPerZone + BlocksPerZone); cellY++) {
            srand(fastRandom(cellX, cellY));
            StructureCell cell = m_world->structureCell(cellX, cellY, true);
            if (cell.kind == StructureCell_RewardChest) {
                StaticEntity chest;
                chest.type = StaticEntityRewardChest;
                chest.position = randomPosition(cellX, cellY, PositionDungeonFloor);
                chest.orientation = rand() % 4;
                putOnGround(chest, true);
                Item reward = missions.taskRewardAt(placeX, placeY);
                if (reward.category == ItemCategoryNone) {
                    reward.category = ItemCategoryGeneratedItem;
                    reward.modifier = rand();
                    reward.id = rand() % 3;
                    reward.rarity = ItemRarityYellow;
                    if (rand() % 10 == 0) {
                        reward.rarity++;
                    }
                    if (reward.rarity > ItemRarityYellow) {
                        reward.rarity = ItemRarityYellow;
                    }
                    reward.region = m_world->findRegion((int)chest.position[0], (int)chest.position[1]);
                }
                if (addStaticEntity(chest)) {
                    m_objectRewards[(int)m_staticEntities.size() - 1] = reward;
                }
            }
            if (cell.kind == StructureCell_LootChest) {
                StaticEntity chest;
                chest.type = StaticEntityRewardChest;
                chest.position = randomPosition(cellX, cellY, PositionDungeonFloor);
                chest.orientation = rand() % 4;
                putOnGround(chest, true);
                Item item;
                item.category = ItemCategoryGeneratedItem;
                item.id = rand() % 3;
                item.modifier = rand();
                unsigned char rarity = (cell.side >> 2) + 1;
                if (rand() % 10 == 0) {
                    rarity++;
                }
                if (rarity > ItemRarityYellow) {
                    rarity = ItemRarityYellow;
                }
                item.rarity = rarity;
                item.region = m_world->findRegion(info.center);
                if (addStaticEntity(chest)) {
                    m_objectRewards[(int)m_staticEntities.size() - 1] = item;
                }
            }
            int buildingType = m_world->zoneBuildingType(cellX, cellY);
            if (m_world->hasMission(cellX, cellY) && buildingType != BuildingTreasureTrunk &&
                buildingType != BuildingFloatingIslandTreasurePavilion &&
                buildingType != BuildingBuriedTreasureChamber) {
                Item item;
                item.category = ItemCategoryGeneratedItem;
                item.id = rand() % 3;
                item.modifier = rand();
                item.rarity = rand() % 3 + 2;
                item.region = m_world->findRegion(cellX * BlocksPerCell, cellY * BlocksPerCell);
                plasma::Vector<3, Fixed> position;
                if (buildingType == BuildingGroundTreasureSite) {
                    if (positionCount(cellX, cellY, PositionCave) != 0) {
                        position = randomPosition(cellX, cellY, PositionCave);
                    } else {
                        position = randomPosition(cellX, cellY, PositionAny);
                    }
                } else if (buildingType == BuildingUnderwaterTreasureSite) {
                    position = randomPosition(cellX, cellY, PositionUnderwater);
                } else {
                    position = randomSpawnPosition(cellX, cellY);
                }
                if (rand() % 2 == 0 && buildingType != BuildingUnderwaterTreasureSite) {
                    ZoneItem lying;
                    lying.item = item;
                    lying.position = position;
                    lying.flags |= ZoneItemFlagPerPlayer;
                    m_items.push_back(lying);
                } else {
                    StaticEntity chest;
                    chest.type = StaticEntityRewardChest;
                    chest.position = position;
                    chest.orientation = rand() % 4;
                    putOnGround(chest, true);
                    if (addStaticEntity(chest)) {
                        m_objectRewards[(int)m_staticEntities.size() - 1] = item;
                    }
                }
            }
            if (m_world->hasLore(cellX, cellY)) {
                int level = m_world->structureLevel(cellX, cellY, buildingType);
                if (buildingType == BuildingInscribedMenhir) {
                    StaticEntity lore;
                    // The lore tablet: half a block off the cell centre.
                    lore.type = StaticEntityInscribedTablet;
                    lore.orientation = QuarterTurn_Zero;
                    lore.position =
                        plasma::Vector<3, Fixed>((Fixed(cellX * BlocksPerCell + 8) + 0.51f),
                                                 (Fixed(cellY * BlocksPerCell + 8) - 0.1f), Fixed(level + 1));
                    for (int attempt = 0; attempt < 8; attempt++) {
                        lore.position[2] =
                            (std::max)(0, floor(m_world->height((int)lore.position[0], (int)lore.position[1], 0))) + 2;
                        bool blocked = false;
                        for (int dz = -1; dz <= 1; dz++) {
                            for (int dx = -1; dx <= 1; dx++) {
                                if (!isFree(cubeAt((lore.position[0] + dx), lore.position[1], (lore.position[2] + dz)),
                                            false, true, true)) {
                                    blocked = true;
                                    break;
                                }
                            }
                        }
                        if (!blocked) {
                            break;
                        }
                        lore.position[1] -= Fixed(1);
                        if (lore.position[1] < m_position[1] * BlocksPerZone) {
                            break;
                        }
                    }
                    m_staticEntities.push_back(lore);
                } else {
                    StaticEntity lore;
                    if (buildingType == BuildingLoreCrypt) {
                        lore.type = StaticEntitySarcophagus;
                        // A pedestal for one realm in four; one of the six stone tablets by a hash of the realm.
                    } else {
                        int realm = m_world->placeRealm(m_world->placeIndex(cellX * BlocksPerCell),
                                                        m_world->placeIndex(cellY * BlocksPerCell));
                        if ((hash(realm) & 3) == 0) {
                            lore.type = StaticEntityPedestal;
                        } else {
                            lore.type =
                                hash(realm + 0xda5) % (StaticEntityStoneTablet6 - StaticEntityStoneTablet1 + 1) +
                                StaticEntityStoneTablet1;
                        }
                    }
                    lore.orientation = rand() % 4;
                    lore.position = plasma::Vector<3, Fixed>(Fixed(cellX * BlocksPerCell + 8),
                                                             Fixed(cellY * BlocksPerCell + 8), Fixed(level));
                    putOnGround(lore, true);
                    addStaticEntity(lore);
                }
            }
        }
    }
    // 7.
    generatePlants();
    missions.generate(this);
    m_remainingPositions = m_positionsByKind;

    // A cell under a village's influence gets its villagers (a resident per plot of a covered house with a street
    // door, with a class by the plot and a pet for plot 10; a guild hall's master; one wanderer or pet in forty
    // cells), the origin's cell 24 the developers' household; a campfire cell its fires; a cell on the 16-cell grid by
    // a type 1 land a boat; a cell away from the zone's site a spawn group; a structure cell its residents and a
    // castle cell its court; a cell with kind 16 positions a few items (by the house: mushrooms, fruit, by the land's
    // climate); kinds 18 and 5 a lily or a flower; a cell of the types 27, 32 and 33 its crops; and one cell in
    // sixty-four or sixteen a wild creature.
    for (int cellX = toCell(m_position[0] * BlocksPerZone);
         cellX < toCell(m_position[0] * BlocksPerZone + BlocksPerZone); cellX++) {
        for (int cellY = toCell(m_position[1] * BlocksPerZone);
             cellY < toCell(m_position[1] * BlocksPerZone + BlocksPerZone); cellY++) {
            unsigned int seed = fastRandom(cellX, cellY);
            // The residents' draws are seeded by the cell's hash modulo 32767.
            srand(seed % 32767);
            int buildingType = m_world->zoneBuildingType(cellX, cellY);
            int x = cellX * BlocksPerCell + 8;
            int y = cellY * BlocksPerCell + 8;
            float influence =
                m_world->placeInfluence(info.type, info.large, m_world->placeSquaredDistance(info.center, x, y));
            // A village's residents within its core (influence above 0.4); one unassigned plot in ten gets none; guild
            // receptionists are equipped at level 3.
            if (influence > 0.4f) {
                if (info.type == PlaceVillage) {
                    unsigned int petrifiedFlag =
                        isProtectedVillage(missions, placeX, placeY) ? AppearanceFlagPetrified : 0;
                    if (buildingType == BuildingGuildHallCentre) {
                        std::cout << "GUILD HALL CENTER" << std::endl;
                    }
                    int form = 0;
                    Realm realm(realmId);
                    if (!realm.hasTyrant() && realm.hasPeople()) {
                        form = realm.form();
                    }
                    int plot = m_world->hamletPlot(cellX, cellY, info.type, dungeonPosition);
                    if (plot == PlotInn) {
                        m_residentPositions.push_back(randomPosition(cellX, cellY, PositionSheltered));
                    }
                    // The villagers' race table by the land's biome.
                    int raceTable = RaceHumanMale;
                    switch (biome) {
                    case BiomeWoodlands:
                    case BiomeSnowForest:
                        raceTable = RaceElfMale;
                        break;
                    case BiomeDarkWoods:
                    case BiomeDeadlands:
                        raceTable = RaceUndeadMale;
                        break;
                    case BiomeDesert:
                        raceTable = RaceLizardmanMale;
                        break;
                    case BiomeSavannah:
                        raceTable = RaceOrcMale;
                        break;
                    case BiomeFirelands:
                        raceTable = RaceGoblinMale;
                        break;
                    case BiomeMountains:
                        raceTable = RaceDwarfMale;
                        break;
                    case BiomeJungle:
                    case BiomeWetlands:
                        raceTable = RaceFrogmanMale;
                        break;
                    case BiomeOcean:
                        raceTable = RacePirateMale;
                        break;
                    }
                    if (m_world->isCoveredType(buildingType) && hasStreetDoor(buildingType)) {
                        // A house: its resident's class by the plot (a plot without a class gets one villager in ten),
                        // at a kind 4 position (kind 1 for plot 10), and the pet of plot 10 on the roof.
                        int kind = PositionBuildingCentre;
                        if (plot != PlotNone) {
                            int classType = ClassVillager;
                            bool wanted = true;
                            switch (plot) {
                            case PlotInn:
                                classType = ClassInnkeeper;
                                break;
                            case PlotItemShop:
                                classType = ClassItemVendor;
                                break;
                            case PlotArmorShop:
                                classType = ClassArmorVendor;
                                break;
                            case PlotWeaponShop:
                                classType = ClassWeaponVendor;
                                break;
                            case PlotIdentifiersShop:
                                classType = ClassAnalyzer;
                                break;
                            case PlotCarpentersShop:
                                classType = ClassCarpenter;
                                break;
                            case PlotTailorsShop:
                                classType = ClassTailor;
                                break;
                            case PlotSmithy:
                                classType = ClassSmith;
                                break;
                            case PlotFlightMaster:
                                classType = ClassFlightMaster;
                                kind = PositionGround;
                                break;
                            default:
                                if (rand() % 10 != 0) {
                                    wanted = false;
                                }
                                break;
                            }
                            if (wanted) {
                                Resident *resident = new Resident();
                                resident->hostility = HostilityFriendlyNpc;
                                resident->race = randomRace(raceTable);
                                resident->classType = classType;
                                resident->factionKind = form;
                                resident->realm = realmId;
                                resident->angle = (float)rand() / (float)RAND_MAX * 360.0f;
                                resident->appearance.appearanceFlags |= petrifiedFlag;
                                fillAppearance(resident->race, resident->appearance, nullptr, false);
                                equip(resident->hostility, resident->race, 0, resident->appearance.appearanceFlags,
                                      resident->factionKind, resident->classType, resident->specialization,
                                      resident->equipment, resident->specialSkills, resident->summonAppearanceMode,
                                      resident->summonRaces, resident->inventory);
                                resident->position = randomPosition(cellX, cellY, kind);
                                resident->behaviours = BehaviourStand;
                                resident->angle = (float)rand() * 360.0f / (float)RAND_MAX;
                                adoptResident(resident);
                            }
                            if (plot == PlotFlightMaster) {
                                Resident *resident = new Resident();
                                resident->hostility = HostilityNeutral;
                                resident->race = RaceEagle;
                                resident->angle = (float)rand() / (float)RAND_MAX * 360.0f;
                                resident->appearance.appearanceFlags |= petrifiedFlag;
                                fillAppearance(resident->race, resident->appearance, nullptr, false);
                                equip(resident->hostility, resident->race, 0, resident->appearance.appearanceFlags,
                                      resident->factionKind, resident->classType, resident->specialization,
                                      resident->equipment, resident->specialSkills, resident->summonAppearanceMode,
                                      resident->summonRaces, resident->inventory);
                                resident->position = randomPosition(cellX, cellY, PositionEagleSeat);
                                resident->position[2] += Fixed(resident->appearance.size[2] * 0.5f + 0.5f);
                                resident->behaviours = BehaviourStand;
                                resident->angle = (float)rand() * 360.0f / (float)RAND_MAX;
                                adoptResident(resident);
                            }
                        }
                    } else if (buildingType == BuildingGuildHallCentre) {
                        // The guild hall's master, in the middle of its cell.
                        Resident *resident = new Resident();
                        resident->hostility = HostilityFriendlyNpc;
                        resident->race = randomRace(raceTable);
                        resident->classType = ClassGuildReceptionist;
                        resident->factionKind = form;
                        resident->realm = realmId;
                        resident->angle = (float)rand() / (float)RAND_MAX * 360.0f;
                        resident->appearance.appearanceFlags |= petrifiedFlag;
                        fillAppearance(resident->race, resident->appearance, nullptr, false);
                        equip(resident->hostility, resident->race, 3, resident->appearance.appearanceFlags,
                              resident->factionKind, resident->classType, resident->specialization, resident->equipment,
                              resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces,
                              resident->inventory);
                        resident->position = plasma::Vector<3, Fixed>(
                            Fixed(cellX * BlocksPerCell + 8), Fixed(cellY * BlocksPerCell + 8),
                            Fixed(m_world->structureLevel(cellX, cellY, BuildingGuildHallCentre)));
                        resident->behaviours = BehaviourStand;
                        resident->angle = (float)rand() * 360.0f / (float)RAND_MAX;
                        adoptResident(resident);
                    } else if (rand() % 20 == 0 && modulo(cellX + cellY, 2) == 0) {
                        // A wanderer, or one time in ten a pet, at a kind 1 position.
                        Resident *resident = new Resident();
                        if (rand() % 10 != 0) {
                            resident->hostility = HostilityFriendlyNpc;
                            resident->race = randomRace(raceTable);
                            resident->classType = ClassVillager;
                            resident->factionKind = form;
                            resident->realm = realmId;
                            resident->appearance.appearanceFlags |= petrifiedFlag;
                            resident->behaviours = BehaviourWander;
                            if (rand() % 2 != 0) {
                                resident->behaviours |= BehaviourUseFurniture;
                            }
                            if (rand() % 2 != 0) {
                                resident->behaviours |= BehaviourSit;
                            }
                            if (rand() % 2 != 0) {
                                resident->behaviours |= BehaviourStand;
                            }
                        } else {
                            resident->hostility = HostilityNeutral;
                            static std::vector<int> pets = {RaceWhiteCat, RaceCat,    RacePig,     RaceShepherdDog,
                                                            RaceTerrier,  RaceCollie, RaceChicken, RaceSheep};
                            resident->race = pets[rand() % pets.size()];
                            resident->behaviours = BehaviourWander;
                        }
                        resident->angle = (float)rand() / (float)RAND_MAX * 360.0f;
                        fillAppearance(resident->race, resident->appearance, nullptr, false);
                        equip(resident->hostility, resident->race, 0, resident->appearance.appearanceFlags,
                              resident->factionKind, resident->classType, resident->specialization, resident->equipment,
                              resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces,
                              resident->inventory);
                        resident->position = randomPosition(cellX, cellY, PositionGround);
                        adoptResident(resident);
                    }
                } else if (info.type != PlaceCastle && info.type != PlaceCircleOfPower &&
                           buildingType != BuildingOldHutChimney) {
                    // The developers' household: cell 24 of the place at the origin, three named residents at kind 2
                    // positions.
                    if (placeX == 0 && placeY == 0 && buildingType == BuildingHouseEntrance) {
                        Resident *resident = new Resident();
                        resident->hostility = HostilityFriendlyNpc;
                        resident->race = RaceCreatorMale;
                        strcpy(resident->name, "Wollay");
                        fillAppearance(resident->race, resident->appearance, nullptr, false);
                        resident->appearance.appearanceFlags |= AppearanceFlagMissionResident;
                        resident->position = randomPosition(cellX, cellY, PositionSheltered);
                        resident->behaviours = BehaviourWander | BehaviourUseFurniture;
                        adoptResident(resident);
                        resident = new Resident();
                        resident->hostility = HostilityFriendlyNpc;
                        resident->race = RaceCreatorFemale;
                        strcpy(resident->name, "Pixxie");
                        fillAppearance(resident->race, resident->appearance, nullptr, false);
                        resident->appearance.appearanceFlags |= AppearanceFlagMissionResident;
                        resident->position = randomPosition(cellX, cellY, PositionSheltered);
                        resident->behaviours = BehaviourWander | BehaviourUseFurniture;
                        adoptResident(resident);
                        resident = new Resident();
                        resident->hostility = HostilityFriendlyNpc;
                        resident->race = RaceCollie;
                        strcpy(resident->name, "Joschi");
                        fillAppearance(resident->race, resident->appearance, nullptr, false);
                        resident->appearance.appearanceFlags |=
                            AppearanceFlagMissionResident | AppearanceFlagSpiritForm;
                        resident->position = randomPosition(cellX, cellY, PositionSheltered);
                        resident->behaviours = BehaviourWander;
                        adoptResident(resident);
                    }
                }
            }
            if (buildingType == BuildingCampfireSite || buildingType == BuildingCampEquipmentSite) {
                // A campfire cell: one or two fires (type 83) at kind 1 positions; from the second one on (or on a
                // type 99 cell) three in four are torches (95 below block 25, else 96), the rest a random lamp or
                // ornament.
                int count = rand() % 2 + 1;
                for (int i = 0; i < count; i++) {
                    StaticEntity fire;
                    fire.type = StaticEntityCampfire;
                    fire.position = randomPosition(cellX, cellY, PositionGround);
                    // Around a camp: a beach towel below height 25 (else a wooden mat) for three in four, an umbrella
                    // for one in three of the rest.
                    fire.orientation = rand() % 4;
                    if (i > 0 || buildingType == BuildingCampEquipmentSite) {
                        if (rand() % 4 != 0) {
                            fire.type =
                                fire.position[2] < BeachTowelMaxHeight ? StaticEntityBeachTowel : StaticEntityWoodMat;
                        } else if (fire.position[2] < BeachTowelMaxHeight && rand() % 3 == 0) {
                            fire.type = StaticEntityBeachUmbrella;
                        } else {
                            switch (rand() % 7) {
                            case 0:
                                fire.type = StaticEntityBarrel;
                                break;
                            case 1:
                                fire.type = StaticEntityCrate;
                                break;
                            case 2:
                                fire.type = StaticEntityOpenCrate;
                                break;
                            case 3:
                                fire.type = StaticEntitySack;
                                break;
                            case 4:
                                fire.type = StaticEntityTent;
                                break;
                            case 5:
                                fire.type = StaticEntityStool;
                                break;
                            case 6:
                                fire.type = rand() % (StaticEntityWagon2 - StaticEntityWagon1 + 1) + StaticEntityWagon1;
                                break;
                            }
                        }
                    }
                    addStaticEntity(fire);
                }
            }
            // A boat (category 24, id 2) on a cell of the 16-cell grid in a biome 1 region whose centre lies below
            // height 10.
            if (modulo(cellX, 16) == 0 && modulo(cellY, 16) == 0 &&
                m_world->landBiome(m_world->findRegion(cellX * BlocksPerCell, cellY * BlocksPerCell)) == BiomeOcean &&
                m_world->height(x, y, 0) < 10.0f && positionCount(cellX, cellY, PositionNaturalGround) != 0) {
                ZoneItem boat;
                boat.item.category = ItemCategorySpecial;
                boat.item.id = SpecialItemBoat;
                boat.item.region = m_world->findRegion(cellX * BlocksPerCell, cellY * BlocksPerCell);
                boat.flags |= ZoneItemFlagPerPlayer;
                boat.position = randomPosition(cellX, cellY, PositionNaturalGround);
                boat.rotation = (float)rand() * 360.0f / (float)RAND_MAX;
                m_items.push_back(boat);
            }
            // One cell in 128 outside a place (or under half its influence) on firm ground, more than four cells from
            // the zone's site, gets a spawn group.
            influence =
                m_world->placeInfluence(info.type, info.large, m_world->placeSquaredDistance(info.center, x, y));
            if (info.type == PlaceNone || influence < 0.5f) {
                // Wild spawns: one column in 128 off the streets (below 0.25), near the region's site (the zone's
                // place, 64 cells).
                if (rand() % 128 == 0 && m_world->streetAmount(x, y, info.type, influence) < 0.25f) {
                    plasma::Vector<2, int> site = m_world->siteInZone(partIndex(cellX, BlocksPerPlace / BlocksPerCell),
                                                                      partIndex(cellY, BlocksPerPlace / BlocksPerCell));
                    if (squaredLength(plasma::Vector<2, int>(site[0] - cellX, site[1] - cellY)) > 16) {
                        addSpawnGroup(cellX, cellY, false);
                    }
                }
            }
            StructureCell cell = m_world->structureCell(cellX, cellY, true);
            if (cell.kind != StructureCell_None) {
                addResidents(cellX, cellY, missions);
            }
            if (info.type == PlaceCastle) {
                addCastleResidents(cellX, cellY, missions, castlePart, castleCell);
            }
            // The items lying at the cell's kind 16 positions: a mushroom (category 11, id 22; 24 on block type 10; 29
            // one time in five) one time in forty, but at a house of the kinds 1..4 fruit (id 15, up to three; or 36,
            // one) and in a type 1 house by the seed and the climate a consumable (category 1, id 14) or id 40, at a
            // type 4 house id 43 or 41, one per eight positions. Mushrooms grow on grass only.
            if (positionCount(cellX, cellY, PositionNaturalGround) != 0) {
                int treeBuildingType = m_world->zoneBuildingType(cellX, cellY);
                bool isTreeLandmark = treeBuildingType >= BuildingOak && treeBuildingType <= BuildingPalm;
                ZoneItem lying;
                lying.item.category = ItemCategoryMaterial;
                // Items on the ground: a mana orchid for one flower in five, flowers rarely (at most two of rand() %
                // 40 - 38), up to three mushrooms at a tree; fruit for one seed in ten, apples where mild (0.3..0.75)
                // and not dry (0.2), lemons elsewhere; ice cubes below temperature 0.2.
                lying.item.id = MaterialItemHeartflower;
                int id = cubeAt(lying.position[0], lying.position[1], (lying.position[2] - 1)).type == BlockSnow
                             ? MaterialItemFrozenHeartflower
                             : MaterialItemHeartflower;
                if (rand() % 5 == 0) {
                    id = MaterialItemManaOrchid;
                }
                lying.item.id = id;
                int count = rand() % 40 - 38;
                if (isTreeLandmark && treeBuildingType != BuildingPalm) {
                    id = MaterialItemMushroom;
                    lying.item.id = id;
                    count = rand() % 4;
                    if (rand() % 2 != 0) {
                        id = MaterialItemDragonRoot;
                        lying.item.id = id;
                        count = 1;
                    }
                    if (treeBuildingType == BuildingOak) {
                        World::AreaInfo area;
                        m_world->fillAreaInfo(x, y, area);
                        World::BiomeSample sample;
                        m_world->biomeSample(x, y, sample);
                        float temperature = sample.temperature(area.type, area.influence);
                        float humidity = sample.humidity();
                        if (seed % 10 == 0) {
                            if (temperature > 0.3f && temperature < 0.75f && humidity > 0.2f) {
                                lying.item.category = ItemCategoryConsumable;
                                lying.item.id = ConsumableApple;
                                count = positionCount(cellX, cellY, PositionNaturalGround) / 8 + 1;
                            }
                        } else if (seed % 10 == 1) {
                            if (temperature > 0.6f) {
                                lying.item.category = ItemCategoryMaterial;
                                lying.item.id = MaterialItemLemon;
                                count = positionCount(cellX, cellY, PositionNaturalGround) / 8 + 1;
                            }
                        }
                    } else if (treeBuildingType == BuildingJungleTree) {
                        if (seed % 10 == 0) {
                            lying.item.id = MaterialItemCocoaBean;
                            count = positionCount(cellX, cellY, PositionNaturalGround) / 8 + 1;
                        } else if (seed % 10 == 1) {
                            lying.item.id = MaterialItemBanana;
                            count = positionCount(cellX, cellY, PositionNaturalGround) / 8 + 1;
                        }
                    }
                }
                if (id == 22 || id == 29) {
                    if (cubeAt(lying.position[0], lying.position[1], (lying.position[2] - 1)).type != BlockGrass) {
                        continue;
                    }
                }
                for (int i = 0; i < count; i++) {
                    lying.rotation = (float)rand() / (float)RAND_MAX * 360.0f;
                    lying.position = randomPosition(cellX, cellY, PositionNaturalGround);
                    m_items.push_back(lying);
                }
            }
            // A water lily (category 11, id 39) at a kind 18 position in a cold spot, one cell in ten.
            if (rand() % 10 == 0 && positionCount(cellX, cellY, PositionUnderwater) != 0) {
                plasma::Vector<3, Fixed> position = randomPosition(cellX, cellY, PositionUnderwater);
                World::AreaInfo area;
                m_world->fillAreaInfo((int)position[0], (int)position[1], area);
                World::BiomeSample sample;
                m_world->biomeSample((int)position[0], (int)position[1], sample);
                if (sample.temperature(area.type, area.influence) < 0.2f) {
                    ZoneItem iceCubeItem;
                    iceCubeItem.item.category = ItemCategoryMaterial;
                    iceCubeItem.item.id = MaterialItemIceCube;
                    iceCubeItem.rotation = (float)rand() / (float)RAND_MAX * 360.0f;
                    iceCubeItem.position = position;
                    m_items.push_back(iceCubeItem);
                }
            }
            // A flower (category 11, id 19) at a kind 5 position, one cell in thirty.
            if (rand() % 30 == 0 && positionCount(cellX, cellY, PositionCave) != 0) {
                ZoneItem shimmerMushroomItem;
                shimmerMushroomItem.position = randomPosition(cellX, cellY, PositionCave);
                shimmerMushroomItem.rotation = (float)rand() / (float)RAND_MAX * 360.0f;
                shimmerMushroomItem.item.category = ItemCategoryMaterial;
                shimmerMushroomItem.item.id = MaterialItemShimmerMushroom;
                m_items.push_back(shimmerMushroomItem);
            }
            // The crops of a field cell (types 27, 32 and 33: ids 16, 17 and 42), four to eight of them at kind 21
            // positions.
            if (buildingType == BuildingPumpkinField || buildingType == BuildingPineappleField ||
                buildingType == BuildingKaleField) {
                int cropId;
                switch (buildingType) {
                case BuildingPumpkinField:
                    cropId = MaterialItemPumpkin;
                    break;
                case BuildingPineappleField:
                    cropId = MaterialItemPineapple;
                    break;
                default:
                    cropId = MaterialItemKale;
                    break;
                }
                int count = (std::min)(positionCount(cellX, cellY, PositionCrop), rand() % 5 + 4);
                ZoneItem crop;
                crop.item.category = ItemCategoryMaterial;
                crop.item.id = cropId;
                for (int i = 0; i < count; i++) {
                    crop.position = randomPosition(cellX, cellY, PositionCrop);
                    crop.rotation = (float)rand() * 360.0f / (float)RAND_MAX;
                    m_items.push_back(crop);
                }
            }
            // A wild creature at a kind 16 position, one cell in sixty-four: its race by the climate of the spot.
            if (rand() % 64 == 0 && positionCount(cellX, cellY, PositionNaturalGround) != 0) {
                Resident *resident = new Resident();
                resident->hostility = HostilityAttackableObject;
                resident->position = randomPosition(cellX, cellY, PositionNaturalGround);
                World::BiomeSample sample;
                m_world->biomeSample((int)resident->position[0], (int)resident->position[1], sample);
                World::AreaInfo area;
                m_world->fillAreaInfo((int)resident->position[0], (int)resident->position[1], area);
                if (sample.lavaWeight(area.type, area.influence) > 0.5f) {
                    switch (rand() % 2) {
                    case 0:
                        resident->race = RaceFireScrub;
                        break;
                    case 1:
                        resident->race = RaceThornTree;
                        // Harvestable creatures by climate (below 0.25 the cold kinds); ginseng for one in ten; a
                        // silver or gold deposit for one in four.
                        break;
                    }
                } else if (sample.temperature(area.type, area.influence) < 0.25f) {
                    switch (rand() % 2) {
                    case 0:
                        resident->race = RaceSnowBush;
                        break;
                    case 1:
                        resident->race = RaceSnowBerryBush;
                        break;
                    }
                } else if (sample.humidity() < 0.2f) {
                    resident->race = RaceScrub;
                    if (rand() % 2 != 0 && sample.temperature(area.type, area.influence) > 0.8f) {
                        resident->race = RaceCactus;
                    }
                } else {
                    switch (rand() % 3) {
                    case 0:
                        resident->race = RaceBush;
                        break;
                    case 1:
                        resident->race = RaceCottonPlant;
                        break;
                    case 2:
                        resident->race = RaceCobwebScrub;
                        break;
                    }
                    if (rand() % 10 == 0) {
                        resident->race = RaceGinseng;
                    }
                }
                fillAppearance(resident->race, resident->appearance, nullptr, false);
                adoptResident(resident);
            }
            // One cell in sixteen with kind 5 positions gets a spawn group (one time in four) or a wild creature of
            // the races 131..138.
            if (rand() % 16 == 0 && positionCount(cellX, cellY, PositionCave) != 0) {
                if (rand() % 4 == 0) {
                    addSpawnGroup(cellX, cellY, true);
                } else {
                    Resident *resident = new Resident();
                    resident->hostility = HostilityAttackableObject;
                    resident->race = RaceIronDeposit;
                    if (rand() % 4 == 0) {
                        resident->race = rand() % 2 != 0 ? RaceGoldDeposit : RaceSilverDeposit;
                        if (rand() % 2 == 0) {
                            resident->race = RaceEmeraldDeposit;
                            if (rand() % 2 == 0) {
                                resident->race = RaceSapphireDeposit;
                                if (rand() % 2 == 0) {
                                    resident->race = RaceRubyDeposit;
                                    if (rand() % 2 == 0) {
                                        resident->race = RaceDiamondDeposit;
                                    }
                                }
                            }
                        }
                    }
                    fillAppearance(resident->race, resident->appearance, nullptr, false);
                    resident->position = randomPosition(cellX, cellY, PositionCave);
                    adoptResident(resident);
                }
            }
        }
    }
    if (missions.m_hasDemonPortal) {
        for (std::pair<const long long, Resident *> &entry : m_residents) {
            Resident *resident = entry.second;
            if ((resident->hostility == HostilityEnemy || resident->hostility == HostilityFriendlyNpc) &&
                !(resident->hostility == HostilityFriendlyNpc &&
                  (resident->appearance.appearanceFlags & AppearanceFlagMissionResident)) &&
                speciesType(resident->race) != SpeciesDemon) {
                if ((resident->appearance.appearanceFlags & (AppearanceFlagTinted | AppearanceFlagLarge)) ||
                    rand() % 50 == 0) {
                    resident->appearance.appearanceFlags |= AppearanceFlagPromotedResident;
                }
            }
        }
    }
    // The entities and items as generated, and the region of every natural item.
    m_initialStaticEntities = m_staticEntities;
    m_initialItems = m_items;
    for (ZoneItem &lying : m_items) {
        if (isNatural(lying.item.category)) {
            lying.item.region = m_world->findRegion((int)lying.position[0], (int)lying.position[1]);
        }
    }
    updateLists();
    updateOccupancy();
}

}
