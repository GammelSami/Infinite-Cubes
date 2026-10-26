// Interior wall and ceiling fixtures.
#include "cube/PlantBuilder.h"
#include "cube/Fixed.h"
#include "cube/Noise.h"
#include "cube/World.h"
#include "cube/Zone.h"
#include <math.h>
#include <stdlib.h>
#include "cube/BlockIds.h"
#include "cube/ItemIds.h"
#include "cube/WorldIds.h"
#include "cube/ZoneIds.h"

namespace cube {

namespace {
// The wall units' material: the room's colour this far below the floor, fading to the wall's at the floor and the
// trim's at the top.
const int WallUnitMaterialFadeDepth = 20;
// How far a fixture is moved to the nearest open block (or wall).
const int SurfaceSearchSteps = 10;
// A wall fixture needs this many blocks of wall behind it.
const int FixtureWallHeight = 5;
// One in this many: a chosen chandelier is placed, a skylight is cut, an alcove gets a candle, a wall unit is one of
// the rare kinds.
const int ChandelierOdds = 20;
const int SkylightOdds = 8;
const int AlcoveCandleOdds = 3;
const int RareWallUnitOdds = 5;
}

// Fixture recipes define each wall unit's extents, placement, colours and random variation.

void PlantBuilder::ceilingChain(int x, int y, int z, int floor)
{
    StructureBuilder part(*this, x, y, 0, 1, 1, rand() % 4, false);
    int length = rand() % 10 + 2;
    part.chain(0, 0, z - length, length);
}

void PlantBuilder::ceilingHook(int x, int y, int z, int floor)
{
    StructureBuilder part(*this, x, y, 0, 1, 1, rand() % 4, false);
    int length = rand() % 10 + 2;
    // A chain ending in a steel (100, 110, 120) hook: a three-block shank and a two-block crook.
    int bottom = z - length;
    part.chain(0, 0, bottom + 3, length - 3);
    part.setColor(plasma::Vector<3, float>(100.0f, 110.0f, 120.0f), TexturePlain);
    part.box(0, 0, bottom, 1, 1, 3, false);
    part.box(1, 0, bottom, 2, 1, 1, false);
    part.box(2, 0, bottom + 1, 1, 1, 1, false);
}

void PlantBuilder::ceilingPlant(int x, int y, int z)
{
    if (m_plantKinds.empty()) {
        return;
    }
    if (rand() % 2 != 0) {
        return;
    }
    for (int i = 0; i < SurfaceSearchSteps; i++) {
        if (cube(x, y, z - 1).isPassable()) {
            break;
        }
        z--;
    }
    CeilingPlantKind kind = static_cast<CeilingPlantKind>(m_plantKinds[rand() % m_plantKinds.size()]);
    if (kind == CeilingPlant_Chandelier && rand() % ChandelierOdds != 0) {
        return;
    }
    int zoneX = x;
    int zoneY = y;
    int zoneZ = z;
    toZone(zoneX, zoneY, zoneZ);
    // Centred under the block, a hundredth of a block below the ceiling. A liana is scaled 0.08..0.14 and hangs by its
    // model's 20 units; a chandelier (scale 0.15, a warm light) hangs four blocks down.
    Zone::Plant plant;
    plant.position = plasma::Vector<3, Fixed>(Fixed(zoneX) - -0.5, Fixed(zoneY) - -0.5, Fixed(zoneZ) - -0.01);
    plant.rotation = (float)(rand() % 4 * 90);
    switch (kind) {
    case CeilingPlant_Liana:
        plant.type = PlantLiana;
        plant.scale = (float)rand() * 0.06f / (float)RAND_MAX + 0.08f;
        plant.position[2] -= Fixed(plant.scale * 20.0f);
        break;
    case CeilingPlant_Chandelier:
        plant.position[2] -= Fixed(4);
        plant.scale = 0.15f;
        plant.color[0] = 0.55f;
        plant.color[1] = 0.5f;
        plant.color[2] = 0.4f;
        plant.type = PlantChandelier;
        plant.flags = PlantFlagEmissive;
        break;
    default:
        return;
    }
    m_zone->m_plants.push_back(plant);
}

void PlantBuilder::ceilingShrub(int x, int y, int z, int floor)
{
    // A ball of leaves (size 6..11, green to yellow-green) jittered under the ceiling, a leaf canopy two blocks down,
    // and for one in four a dim light (brightness 200..249, radius 3..5) with a beam of light (turned 2..4).
    float size = (float)rand() * 5.0f / (float)RAND_MAX + 6.0f;
    setMaterial(plasma::Vector<3, float>(50.0f, 200.0f, 50.0f), TexturePlain, (float)z - size * 0.5f,
                plasma::Vector<3, float>(220.0f, 240.0f, 60.0f), TexturePlain, (float)z, false);
    int ballZ = z - rand() % 3;
    int ballY = y - rand() % 5;
    leaves(x - rand() % 5 + 2, ballY + 2, ballZ, size);
    int clearLevel = clearLevelAbove(x, y, z);
    int roll = rand() % 4;
    if (roll == 0) {
        unsigned char brightness = rand() % 50 - 56;
        light(x, y, clearLevel, rand() % 3 + 3, brightness);
    }
    toZone(x, y, z);
    Zone::Plant plant;
    plant.position = plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z - 2));
    plant.rotation = 0.0f;
    plant.color[1] = 200.0f / 255.0f;
    plant.type = PlantLeafCanopy;
    plant.color[0] = 50.0f / 255.0f;
    plant.color[2] = 50.0f / 255.0f;
    // scale is not set.
    m_zone->m_plants.push_back(plant);
    if (roll == 0) {
        int originX = 0;
        int originY = 0;
        toZone(originX, originY, clearLevel);
        Zone::Plant beam;
        beam.type = PlantShadow;
        beam.position = plasma::Vector<3, Fixed>(Fixed(x) - -0.5, Fixed(y) - -0.5, Fixed(z));
        beam.scale = (float)(z - clearLevel);
        beam.rotation = (float)(rand() % 3 + 2);
        m_zone->m_plants.push_back(beam);
    }
}

void PlantBuilder::ceilingSkylight(int x, int y, int z, int floor)
{
    // A rough opening (radius 1, its outline displaced by noise at 0.3) within two blocks of the point, through a
    // ceiling at most two thick, closed by a light pane, with a beam of light down to the floor.
    if (rand() % SkylightOdds != 0) {
        return;
    }
    x = rand() % 5 + x - 2;
    y = rand() % 5 + y - 2;
    for (int i = 0; i < SurfaceSearchSteps; i++) {
        if (cube(x, y, z - 1).isPassable()) {
            break;
        }
        z--;
    }
    for (int dx = -2; dx <= 2; dx++) {
        for (int dy = -2; dy <= 2; dy++) {
            float roughFootprintX = noise((x + dx) * 0.3, 0.0) + (float)dx;
            float roughFootprintY = noise((dy + y) * 0.3, 0.0) + (float)dy;
            if (roughFootprintY * roughFootprintY + roughFootprintX * roughFootprintX > 1.0f &&
                (float)dy * (float)dy + (float)dx * (float)dx > 1.0f) {
                continue;
            }
            if (cube(x + dx, y + dy, z + 2).isPassable()) {
                return;
            }
        }
    }
    for (int dx = -2; dx <= 2; dx++) {
        for (int dy = -2; dy <= 2; dy++) {
            float roughFootprintX = noise((x + dx) * 0.3, 0.0) + (float)dx;
            float roughFootprintY = noise((dy + y) * 0.3, 0.0) + (float)dy;
            if (roughFootprintY * roughFootprintY + roughFootprintX * roughFootprintX > 1.0f &&
                (float)dy * (float)dy + (float)dx * (float)dx > 1.0f) {
                continue;
            }
            for (int n = 0; n < 2; n++) {
                if (n == 1) {
                    if (cube(x + dx, y + dy, z + 1).isSolid() && cube(x + dx + 1, y + dy, z + 1).isSolid() &&
                        cube(x + dx - 1, y + dy, z + 1).isSolid() && cube(x + dx, y + dy + 1, z + 1).isSolid() &&
                        cube(x + dx, y + dy - 1, z + 1).isSolid()) {
                        Block pane(127, 127, 127, BlockLightSource, BlockFlagProtected | BlockFlagDungeon);
                        pane.lightLevel = 255;
                        setCubeBlock(x + dx, y + dy, z + 1, pane);
                    }
                } else {
                    Block opening(127, 127, 127, BlockAir, BlockFlagProtected | BlockFlagDungeon);
                    opening.lightLevel = 255;
                    setCubeBlock(x + dx, y + dy, z + n, opening);
                }
            }
        }
    }
    z += 2;
    toZone(x, y, z);
    Zone::Plant beam;
    beam.position = plasma::Vector<3, Fixed>(Fixed(x) - -0.49, Fixed(y) - -0.49, Fixed(z) - 1.03);
    beam.type = PlantShadow;
    beam.rotation = 0.7f;
    beam.scale = (float)(z - floor - 1);
    m_zone->m_plants.push_back(beam);
}

void PlantBuilder::ceilingStalactite(int x, int y, int z, int floor)
{
    // StructureBuilder::stalactite's cone (radius 3..5, up to ten blocks longer than its radius), where its outline
    // would not break through.
    float radius = (float)rand() * 2.0f / (float)RAND_MAX + 3.0f;
    int height = (int)((float)rand() * 10.0f / (float)RAND_MAX + radius);
    for (int i = (int)((float)x - radius - 1.5f); (float)x + radius + 1.5f >= (float)i; i++) {
        for (int j = (int)((float)y - radius - 1.5f); radius + 1.5f + (float)y >= (float)j; j++) {
            double level = (z + 1) * 0.1;
            float dx = noise(j * 0.1, level) * 1.5f + (float)(i - x);
            float dy = noise(i * 0.1, level + 843.0) * 1.5f + (float)(j - y);
            if (!(dy * dy + dx * dx > radius * radius)) {
                if (cube(i, j, z + 2).isPassable()) {
                    return;
                }
            }
        }
    }
    setMaterial(m_wallColor, TexturePlain, (float)(z - height), m_trimColor, m_trimTexture, (float)z, false);
    int step = 0;
    for (int k = z - height; k < z + 2; k++) {
        float reach = (float)step / (float)height * radius;
        for (int i = (int)((float)x - reach - 1.5f); (float)x + reach + 1.5f >= (float)i; i++) {
            for (int j = (int)((float)y - reach - 1.5f); reach + 1.5f + (float)y >= (float)j; j++) {
                double level = k * 0.1;
                float dx = noise(j * 0.1, level) * 1.5f + (float)(i - x);
                float dy = noise(i * 0.1, level + 843.0) * 1.5f + (float)(j - y);
                if (!(dx * dx + dy * dy > reach * reach)) {
                    setCubeBlock(i, j, k,
                                 Block(BlockProperties<float>(colorAt(i, j, k), BlockBuilding,
                                                              BlockFlagProtected | BlockFlagDungeon, 0)));
                }
            }
        }
        step++;
    }
}

void PlantBuilder::chooseCeilingFixture(int x, int y, int z, int floor)
{
    if (m_ceilingKinds.empty()) {
        return;
    }
    switch (m_ceilingKinds[rand() % m_ceilingKinds.size()]) {
    case CeilingStalactite:
        ceilingStalactite(x, y, z, floor);
        break;
    case CeilingChain:
        ceilingChain(x, y, z, floor);
        break;
    case CeilingHook:
        ceilingHook(x, y, z, floor);
        break;
    case CeilingShrub:
        ceilingShrub(x, y, z, floor);
        break;
    case CeilingSkylight:
        ceilingSkylight(x, y, z, floor);
        break;
    }
}

int PlantBuilder::clearLevelAbove(int x, int y, int z)
{
    for (int i = 0; i < 100; i++) {
        if (!cube(x, y, z + i).isSolid()) {
            return z + i;
        }
    }
    return 0;
}

void PlantBuilder::decorate(int x, int y, int z, int sizeX, int sizeY, int sizeZ)
{
    if (rand() % 2) {
        wallFixture(x + sizeX / 2, y, z, z + sizeZ, QuarterTurn_Zero);
    } else {
        wallUnit(x + sizeX / 2, y, z, z + sizeZ, QuarterTurn_Zero, false);
    }
    if (rand() % 2) {
        wallFixture(x + sizeX / 2, y + sizeY - 1, z, z + sizeZ, QuarterTurn_Two);
    } else {
        wallUnit(x + sizeX / 2 - 1, y + sizeY - 1, z, z + sizeZ, QuarterTurn_Two, true);
    }
    if (rand() % 2) {
        wallFixture(x, y + sizeY / 2, z, z + sizeZ, QuarterTurn_One);
    } else {
        wallUnit(x, y + sizeY / 2, z, z + sizeZ, QuarterTurn_One, false);
    }
    if (rand() % 2) {
        wallFixture(x + sizeX - 1, y + sizeY / 2, z, z + sizeZ, QuarterTurn_Three);
    } else {
        wallUnit(x + sizeX - 1, y + sizeY / 2 - 1, z, z + sizeZ, QuarterTurn_Three, true);
    }
    for (int i = x; i < x + sizeX; i++) {
        for (int j = y; j < y + sizeY; j++) {
            if (i % 5 == 3) {
                if (j % 5 == 3 && rand() % 5 == 0) {
                    ceilingPlant(i, j, z + sizeZ);
                }
            } else if (i % 5 == 1 && j % 5 == 1 && rand() % 5 == 0) {
                chooseCeilingFixture(i, j, z + sizeZ, z);
            }
        }
    }
}

BuildingType World::zoneBuildingType(int x, int y)
{
    plasma::Vector<2, int> point = streetPoint(x, y);
    AreaInfo info;
    fillAreaInfo(point[0], point[1], info);
    return streetBuildingType(x, y, point, info);
}

void PlantBuilder::wallAlcoves(int x, int y, int z, int top, int rotation, bool mirrored)
{
    StructureBuilder part(*this, x, y, 0, 1, 1, rotation, mirrored);
    part.setMaterial(m_roomColor, TextureBrick, (float)(z - WallUnitMaterialFadeDepth), m_wallColor, TextureBrick,
                     (float)z, m_trimColor, TextureBrick, (float)top, false);
    // A nine-block wall face, its middle seven a block deeper.
    part.box(-4, 0, z - 1, 9, 1, 1, false);
    part.box(-4, 0, z, 9, 1, top - z, false);
    part.box(-3, 1, z, 7, 1, top - z, false);
    // An alcove every five blocks, from a block above the floor.
    for (int i = 0; i < (top - z) / 5; i++) {
        int level = z + 1 + i * 5;
        if (rand() % 2 == 0) {
            part.cutBox(-2, 0, level, 5, 2, 3, 0);
            if (rand() % AlcoveCandleOdds == 0) {
                ZoneItem zoneItem;
                int itemX = rand() % 2;
                int itemY = rand() % 2;
                int itemZ = level;
                part.toZone(itemX, itemY, itemZ);
                zoneItem.position = plasma::Vector<3, Fixed>(Fixed(itemX + 0.5), Fixed(itemY + 0.5), Fixed(itemZ));
                zoneItem.rotation = (float)(rand() % 4 * 90);
                zoneItem.item.category = ItemCategoryCandle;
                zoneItem.item.id = CandleItemCandle;
                zoneItem.item.modifier = rand() % 3;
                m_zone->m_items.push_back(zoneItem);
            }
        }
    }
}

void PlantBuilder::wallBracket(int x, int y, int z, int top, int rotation, bool mirrored)
{
    StructureBuilder part(*this, x, y, 0, 1, 1, rotation, mirrored);
    part.setMaterial(m_roomColor, TextureBrick, (float)(z - WallUnitMaterialFadeDepth), m_wallColor, TextureBrick,
                     (float)z, m_trimColor, TextureBrick, (float)top, false);
    // A 2 x 2 post widening in four steps into a corbel under the ceiling.
    part.underpinnedBox(0, 0, z - 1, 2, 2, 1, FillMaterial, CoverAll, false);
    part.box(0, 0, z, 2, 2, top - z - 5, false);
    part.box(-1, 0, top - 5, 4, 2, 2, false);
    part.box(-2, 0, top - 3, 6, 2, 1, false);
    part.box(-3, 0, top - 2, 8, 2, 1, false);
    part.box(-5, 0, top - 1, 12, 2, 1, false);
}

void PlantBuilder::wallButtress(int x, int y, int z, int top, int rotation, bool mirrored)
{
    StructureBuilder part(*this, x, y, 0, 1, 1, rotation, mirrored);
    part.setMaterial(m_roomColor, TextureBrick, (float)(z - WallUnitMaterialFadeDepth), m_wallColor, TextureBrick,
                     (float)z, m_trimColor, TextureBrick, (float)top, false);
    // A stepped foot, a 2 x 1 shaft and a beam along the ceiling.
    part.underpinnedBox(-1, 0, z - 1, 4, 2, 1, FillMaterial, CoverAll, false);
    part.box(-2, 0, z, 6, 3, 1, false);
    part.box(-1, 0, z + 1, 4, 2, 1, false);
    part.box(0, 0, z + 2, 2, 1, top - z - 2, false);
    part.box(-1, 0, top - 5, 4, 2, 1, false);
    part.box(0, 1, top - 3, 2, 1, 1, false);
    part.box(0, 1, top - 2, 2, 2, 1, false);
    part.box(0, 1, top - 1, 2, 9, 1, false);
}

void PlantBuilder::wallColumn(int x, int y, int z, int top, int rotation, bool mirrored)
{
    StructureBuilder part(*this, x, y, 0, 1, 1, rotation, mirrored);
    part.setMaterial(m_roomColor, TextureBrick, (float)(z - WallUnitMaterialFadeDepth), m_wallColor, TextureBrick,
                     (float)z, m_trimColor, TextureBrick, (float)top, false);
    // A half column: a base and a capital one block wider than its shaft.
    part.underpinnedBox(-1, 0, z, 4, 2, 1, FillMaterial, CoverAll, false);
    part.box(0, 0, z + 1, 2, 1, top - z - 2, false);
    part.box(-1, 0, top - 1, 4, 2, 1, false);
}

void PlantBuilder::wallColumnWide(int x, int y, int z, int top, int rotation, bool mirrored)
{
    StructureBuilder part(*this, x, y, 0, 1, 1, rotation, mirrored);
    part.setMaterial(m_roomColor, TextureBrick, (float)(z - WallUnitMaterialFadeDepth), m_wallColor, TextureBrick,
                     (float)z, m_trimColor, TextureBrick, (float)top, false);
    // A half column with a capital widening in three steps.
    part.underpinnedBox(-1, 0, z, 4, 2, 1, FillMaterial, CoverAll, false);
    part.box(0, 0, z + 1, 2, 1, top - z - 4, false);
    part.box(-1, 0, top - 4, 4, 2, 2, false);
    part.box(-2, 0, top - 2, 6, 3, 1, false);
    part.box(-4, 0, top - 1, 10, 4, 1, false);
}

void PlantBuilder::wallFixture(int x, int y, int z, int top, int rotation)
{
    if (m_fixtureKinds.empty() || rand() % 2) {
        return;
    }
    StructureBuilder part(*this, x, y, 0, 1, 1, rotation, false);
    WallFixtureKind kind = static_cast<WallFixtureKind>(m_fixtureKinds[rand() % m_fixtureKinds.size()]);
    switch (kind) {
    case WallFixture_Torch:
    case WallFixture_Skull:
    case WallFixture_Chain:
    case WallFixture_Painting:
        z += rand() % 4 + 2; // two to five blocks up
        break;
    case WallFixture_Crest:
        z += 2;
        break;
    }
    int plantX = 0;
    int plantZ = z;
    int plantY = 0;
    for (int i = 0; i < SurfaceSearchSteps; i++) {
        if (part.cube(plantX, plantY - 1, plantZ).isSolid()) {
            break;
        }
        plantY--;
    }
    for (int i = 0; i < SurfaceSearchSteps; i++) {
        if (part.cube(plantX, plantY, plantZ).isPassable()) {
            break;
        }
        plantY++;
    }
    for (int i = -1; i <= 1; i++) {
        for (int k = plantZ; k < plantZ + FixtureWallHeight; k++) {
            Block behind = part.cube(i + plantX, plantY - 1, k);
            if (!behind.isSolid()) {
                return;
            }
        }
    }
    Block wall = part.cube(plantX, plantY - 1, plantZ);
    if (!wall.isSolid() || !(wall.flags & BlockFlagProtected) || part.cube(plantX, plantY, plantZ).isSolid() ||
        part.cube(plantX, plantY, plantZ + 1).isSolid()) {
        return;
    }
    part.toZone(plantX, plantY, plantZ);
    // Centred on the block, a hundredth of a block up. The scale is the model's scale times the width in blocks: a
    // torch (warm light), skull or chain 1/16 x 1.5, ivy 0.2 x 1..1.6, a crest 0.04 x 3, a painting 1 x 0.1..0.2; the
    // fixture is moved out by half its overhang.
    Zone::Plant plant;
    plant.rotation = 0.0f;
    Fixed positionZ = Fixed(plantZ) - -0.01;
    Fixed positionY = Fixed(plantY) - -0.5;
    Fixed positionX = Fixed(plantX) - -0.5;
    plant.position = plasma::Vector<3, Fixed>(positionX, positionY, positionZ);
    float scale;
    float width;
    switch (kind) {
    case WallFixture_Torch:
        plant.color[0] = 0.6f;
        plant.color[2] = 0.2f;
        plant.type = PlantTorchRedYellowFlame;
        plant.flags = PlantFlagEmissive;
        plant.color[1] = 0.5f;
        scale = 0.0625f;
        width = 1.5f;
        break;
    case WallFixture_WallIvy:
        scale = 0.2f;
        plant.type = rand() % 4 + PlantWallIvy1;
        width = (float)rand() * 0.6f / (float)RAND_MAX + 1.0f;
        break;
    case WallFixture_Crest:
        scale = 0.04f;
        width = 3.0f;
        plant.type = PlantCrest1;
        break;
    case WallFixture_Skull:
        plant.type = PlantWallSkull;
        scale = 0.0625f;
        width = 1.5f;
        break;
    case WallFixture_Chain:
        plant.type = PlantCastleChain;
        scale = 0.0625f;
        width = 1.5f;
        break;
    case WallFixture_Painting:
        scale = 1.0f;
        plant.type = rand() % 4 + PlantPainting01;
        width = (float)rand() * 0.1f / (float)RAND_MAX + 0.1f;
        break;
    default:
        return;
    }
    int turns = part.toZoneRotation(0);
    int directionX = 0;
    int directionY = 1;
    plant.scale = scale * width;
    plant.rotation = (float)(turns * 90);
    part.toZoneDirection(directionX, directionY);
    float overhang = width - 1.0f;
    plant.position[0] = positionX + overhang * (float)directionX * 0.5f;
    plant.position[1] = positionY + overhang * (float)directionY * 0.5f;
    plant.position[2] = positionZ + overhang * 0.0f * 0.5f;
    m_zone->m_plants.push_back(plant);
}

void PlantBuilder::wallFurnace(int x, int y, int z, int top, int rotation, bool mirrored)
{
    StructureBuilder part(*this, x, y, 0, 1, 1, rotation, mirrored);
    part.setMaterial(m_roomColor, TextureBrick, (float)(z - WallUnitMaterialFadeDepth), m_wallColor, TextureBrick,
                     (float)z, m_trimColor, TextureBrick, (float)top, false);
    // A furnace: a 9 x 3 hearth stepping into a chimney, its opening barred by a grate glowing red (120, 60, 60) to
    // grey.
    part.underpinnedBox(-4, 0, z - 1, 9, 3, 1, FillMaterial, CoverAll, false);
    part.box(-4, 0, z, 9, 3, 4, false);
    part.box(-3, 0, z + 4, 7, 3, 1, false);
    part.box(-2, 0, z + 5, 5, 3, 1, false);
    part.box(-1, 0, z + 6, 3, 3, top - z - 6, false);
    part.cutBox(-3, 0, z, 7, 3, 3, 0);
    part.cutBox(-2, 0, z + 3, 5, 3, 1, 0);
    part.setMaterial(plasma::Vector<3, float>(120.0f, 60.0f, 60.0f), TexturePlain, (float)z,
                     plasma::Vector<3, float>(80.0f, 80.0f, 80.0f), TexturePlain, (float)(z + 4), false);
    for (int i = -2; i < 4; i += 2) {
        part.box(i, 1, z, 1, 1, 4, false);
    }
    part.setColor(plasma::Vector<3, float>(1.0f, 1.0f, 1.0f), TexturePlain);
    part.box(-3, 0, z, 7, 1, 4, false);
}

void PlantBuilder::wallLavaSpout(int x, int y, int z, int top, int rotation, bool mirrored)
{
    StructureBuilder part(*this, x, y, 0, 1, 1, rotation, mirrored);
    // A basin of lava one to four blocks up (dark brown to grey stone), overflowing to the floor for two in three.
    int level = rand() % 4 + z + 1;
    part.setMaterial(plasma::Vector<3, float>(60.0f, 50.0f, 30.0f), TexturePlain, (float)level,
                     plasma::Vector<3, float>(80.0f, 80.0f, 80.0f), TexturePlain, (float)(level + 4), false);
    part.box(-1, 0, level, 3, 3, 1, false);
    part.box(-2, 0, level + 1, 5, 3, 2, false);
    part.cutBox(-1, 0, level + 1, 3, 3, 2, 0);
    part.setColor(plasma::Vector<3, float>(1.0f, 1.0f, 1.0f), TexturePlain);
    part.box(-1, -1, level + 1, 3, 1, 2, false);
    part.setMaterial(m_roomColor, TexturePlain, 0.0f, plasma::Vector<3, float>(60.0f, 150.0f, 20.0f), TexturePlain,
                     (float)level, false);
    part.lava(-1, 0, level + 1, 3, 2, 1, 0);
    if (rand() % 3 != 0) {
        int offset = rand() % 3 - 1;
        part.lava(offset, 2, level + 1, 1, 1, 1, 0);
        part.lava(offset, 3, 0, 1, 1, level + 2, 0);
    }
}

void PlantBuilder::wallPier(int x, int y, int z, int top, int rotation, bool mirrored)
{
    StructureBuilder part(*this, x, y, 0, 1, 1, rotation, mirrored);
    part.setMaterial(m_roomColor, TextureBrick, (float)(z - WallUnitMaterialFadeDepth), m_wallColor, TextureBrick,
                     (float)z, m_trimColor, TextureBrick, (float)top, false);
    // A pier: a stepped two-part base and cap around a 4 x 2 shaft.
    part.underpinnedBox(-2, 0, z, 6, 2, 1, FillMaterial, CoverAll, false);
    part.underpinnedBox(-1, 2, z, 4, 1, 1, FillMaterial, CoverAll, false);
    part.box(-1, 0, z + 1, 4, 1, top - z - 2, false);
    part.box(0, 1, z + 1, 2, 1, top - z - 2, false);
    part.box(-2, 0, top - 1, 6, 2, 1, false);
    part.box(-1, 2, top - 1, 4, 1, 1, false);
}

void PlantBuilder::wallStove(int x, int y, int z, int top, int rotation, bool mirrored)
{
    StructureBuilder part(*this, x, y, 0, 1, 1, rotation, mirrored);
    part.setMaterial(m_roomColor, TextureBrick, (float)(z - WallUnitMaterialFadeDepth), m_wallColor, TextureBrick,
                     (float)z, m_trimColor, TextureBrick, (float)top, false);
    // A stove: the furnace's hearth without the chimney, its grate green (80, 120, 80) to grey.
    part.underpinnedBox(-4, 0, z - 1, 9, 3, 1, FillMaterial, CoverAll, false);
    part.box(-4, 0, z, 9, 3, 4, false);
    part.box(-3, 0, z + 4, 7, 3, 1, false);
    part.box(-2, 0, z + 5, 5, 3, 1, false);
    part.cutBox(-3, 0, z, 7, 3, 3, 0);
    part.cutBox(-2, 0, z + 3, 5, 3, 1, 0);
    part.setMaterial(plasma::Vector<3, float>(80.0f, 120.0f, 80.0f), TexturePlain, (float)z,
                     plasma::Vector<3, float>(80.0f, 80.0f, 80.0f), TexturePlain, (float)(z + 4), false);
    for (int i = -2; i < 4; i += 2) {
        part.box(i, 1, z, 1, 1, 4, false);
    }
    part.setColor(plasma::Vector<3, float>(1.0f, 1.0f, 1.0f), TexturePlain);
    part.box(-3, 0, z, 7, 1, 4, false);
}

void PlantBuilder::wallTrough(int x, int y, int z, int top, int rotation, bool mirrored)
{
    StructureBuilder part(*this, x, y, 0, 1, 1, rotation, mirrored);
    // A trough one to four blocks up (green to blue-grey), filled for two in three, and for two in three of those
    // spilling up to nine blocks.
    int level = rand() % 4 + z + 1;
    part.setMaterial(plasma::Vector<3, float>(80.0f, 120.0f, 40.0f), TexturePlain, (float)level,
                     plasma::Vector<3, float>(80.0f, 120.0f, 130.0f), TexturePlain, (float)(level + 4), false);
    part.box(-1, 0, level, 3, 3, 1, false);
    part.box(-2, 0, level + 1, 5, 3, 3, false);
    part.box(-1, 0, level + 4, 3, 3, 1, false);
    part.cutBox(-1, 0, level + 1, 3, 3, 3, 0);
    part.setColor(plasma::Vector<3, float>(1.0f, 1.0f, 1.0f), TexturePlain);
    part.box(-1, -1, level + 1, 3, 1, 3, false);
    part.setMaterial(m_roomColor, TexturePlain, 0.0f, plasma::Vector<3, float>(60.0f, 150.0f, 20.0f), TexturePlain,
                     (float)level, false);
    if (rand() % 3 != 0) {
        part.box(-1, 0, level + 1, 3, 2, 1, false);
        if (rand() % 3 != 0) {
            int offset = rand() % 3 - 1;
            part.box(offset, 2, level + 1, 1, 1, 1, false);
            int length = rand() % 10;
            part.box(offset, 3, level - length + 2, 1, 1, length, false);
        }
    }
}

void PlantBuilder::wallUnit(int x, int y, int z, int top, int rotation, bool mirrored)
{
    if (m_wallKinds.empty()) {
        return;
    }
    StructureBuilder part(*this, x, y, 0, 1, 1, rotation, mirrored);
    for (int i = -2; i <= 2; i++) {
        for (int k = z + 1; k < top; k++) {
            Block behind = part.cube(i, -1, k);
            if (!behind.isSolid() || !(behind.flags & BlockFlagProtected)) {
                return;
            }
        }
    }
    for (;;) {
        if (cube(x, y, z).isPassable()) {
            break;
        }
        if (z >= top) {
            return;
        }
        z++;
    }
    if (z >= top) {
        return;
    }
    int kind = m_wallKinds[rand() % m_wallKinds.size()];
    if (!m_rareWallKinds.empty() && rand() % RareWallUnitOdds == 0) {
        kind = m_rareWallKinds[rand() % m_rareWallKinds.size()];
    }
    switch (kind) {
    case WallColumn:
        wallColumn(x, y, z, top, rotation, mirrored);
        break;
    case WallPier:
        wallPier(x, y, z, top, rotation, mirrored);
        break;
    case WallButtress:
        wallButtress(x, y, z, top, rotation, mirrored);
        break;
    case WallColumnWide:
        wallColumnWide(x, y, z, top, rotation, mirrored);
        break;
    case WallBracket:
        wallBracket(x, y, z, top, rotation, mirrored);
        break;
    case WallAlcoves:
        wallAlcoves(x, y, z, top, rotation, mirrored);
        break;
    case WallStove:
        wallStove(x, y, z, top, rotation, mirrored);
        break;
    case WallTrough:
        wallTrough(x, y, z, top, rotation, mirrored);
        break;
    case WallLavaSpout:
        wallLavaSpout(x, y, z, top, rotation, mirrored);
        break;
    case WallFurnace:
        wallFurnace(x, y, z, top, rotation, mirrored);
        break;
    case WallTree:
        wallsideTree(x, y, z, top, rotation, mirrored);
        break;
    }
}

void PlantBuilder::wallsideTree(int x, int y, int z, int top, int rotation, bool mirrored)
{
    // A tree against the wall: its root six blocks out, a trunk of radius 2..4 (bark 171..200, 121..150, 80..99
    // darkening to (40, 40, 10), shaded by up to a quarter towards its sides), three root knots and three balls of
    // leaves (size 3..8, green to yellow-green) in its upper half.
    int rootX = x;
    int rootY = y;
    switch (rotation % 4) {
    case QuarterTurn_Zero:
        rootY = y + 6;
        break;
    case QuarterTurn_One:
        rootX = x + 6;
        break;
    case QuarterTurn_Two:
        rootY = y - 6;
        break;
    case QuarterTurn_Three:
        rootX = x - 6;
        break;
    }
    int base = clearLevelAbove(rootX, rootY, z);
    for (; base >= 0; base--) {
        if (cube(rootX, rootY, base).isSolid()) {
            break;
        }
    }
    base++;
    int height = top - base;
    int radius = rand() % 3 + 2;
    StructureBuilder part(*this, x, y, 0, 1, 1, rotation, mirrored);
    plasma::Vector<3, float> dark(40.0f, 40.0f, 10.0f);
    plasma::Vector<3, float> bark(
        plasma::Vector<3, unsigned char>(200 - rand() % 30, 150 - rand() % 30, rand() % 20 + 80));
    part.setMaterial(bark, TexturePlain, (float)base, dark, TexturePlain, (float)top, false);
    int centerX = rand() % 2;
    int centerY = rand() % 5 + 1;
    for (int i = centerX - radius; i <= centerX + radius; i++) {
        for (int j = centerY - radius; j <= centerY + radius; j++) {
            float dx = (float)i + 0.5f - (float)centerX;
            float dy = (float)j + 0.5f - (float)centerY;
            if (!(dy * dy + dx * dx > (float)(radius * radius))) {
                float shade = 1.0f - fabsf((float)i / (float)radius) * 0.25f;
                for (int k = base; k < top; k++) {
                    part.setCubeBlock(i, j, k,
                                      Block(BlockProperties<float>(part.colorAt(i, j, k) * shade, BlockTree,
                                                                   BlockFlagProtected | BlockFlagDungeon, 0)));
                }
            }
        }
    }
    part.sphere(centerX - radius - 1, centerY, base + rand() % 2, 2.0f);
    part.sphere(centerX + radius, centerY, base + rand() % 2, 2.0f);
    int knotZ = base + rand() % 2;
    part.sphere(centerX, centerY + rand() % 2 + radius, knotZ, 2.0f);
    for (int n = 0; n < 3; n++) {
        int leafZ = base + height / 2 + rand() % (height / 2 + 1);
        float size = (float)rand() * 5.0f / (float)RAND_MAX + 3.0f;
        part.setMaterial(plasma::Vector<3, float>(50.0f, 200.0f, 50.0f), TexturePlain, (float)leafZ - size,
                         plasma::Vector<3, float>(220.0f, 240.0f, 60.0f), TexturePlain, (float)leafZ + size, false);
        int leafY = radius + centerY + rand() % 5;
        part.leaves(rand() % 7 - 3, leafY, leafZ, size);
    }
}

PlantBuilder::PlantBuilder(Zone &zone, int cellX, int cellY, int storey, const plasma::Vector<3, float> &wallColor,
                           const plasma::Vector<3, float> &trimColor, const plasma::Vector<3, float> &roomColor)
    : StructureBuilder(zone, cellX * BlocksPerCell, cellY * BlocksPerCell, 0, BlocksPerCell, BlocksPerCell, 0, false,
                       nullptr),
      m_storey(storey),
      m_wallColor(wallColor),
      m_trimColor(trimColor),
      m_roomColor(roomColor),
      m_unused1a4(1),
      m_trimTexture(1),
      m_unused1ac(0)
{
    World *world = zone.m_world;
    if (storey < 0) {
        StructureCell cell = world->structureCell(cellX, cellY, true);
        if (cell.style == 2) {
            if (!cell.roomPart) {
                m_wallKinds.push_back(WallButtress);
                m_wallKinds.push_back(WallBracket);
            }
            if (cell.kind == StructureCell_Room) {
                m_fixtureKinds.push_back(WallFixture_WallIvy);
            }
            m_ceilingKinds.push_back(CeilingSkylight);
            return;
        }
        int placeX = floorDivide(cellX * BlocksPerCell, BlocksPerPlace);
        int placeY = floorDivide(cellY * BlocksPerCell, BlocksPerPlace);
        bool large = zone.m_world->isLargePlace(placeX, placeY);
        int placeType = zone.m_world->locationType(placeX, placeY, large);
        switch (zone.m_world->dungeonType(placeX, placeY, placeType, large)) {
        case PlaceSewers:
            if (!cell.roomPart) {
                m_wallKinds.push_back(WallStove);
                m_wallKinds.push_back(WallTrough);
                m_wallKinds.push_back(WallColumn);
                m_wallKinds.push_back(WallBracket);
            }
            m_fixtureKinds.push_back(WallFixture_Chain);
            break;
        case PlaceMines:
            m_fixtureKinds.push_back(WallFixture_Chain);
            break;
        case PlaceCatacombs:
            if (!cell.roomPart) {
                m_wallKinds.push_back(WallAlcoves);
                m_wallKinds.push_back(WallColumnWide);
                m_wallKinds.push_back(WallBracket);
            }
            m_fixtureKinds.push_back(WallFixture_Chain);
            m_fixtureKinds.push_back(WallFixture_Skull);
            break;
        case PlaceShrine:
            if (!cell.roomPart) {
                m_wallKinds.push_back(WallPier);
            }
            if (cell.kind == StructureCell_Room) {
                m_fixtureKinds.push_back(WallFixture_WallIvy);
            }
            m_plantKinds.push_back(CeilingPlant_Liana);
            break;
        case PlaceGrotto:
            m_plantKinds.push_back(CeilingPlant_Liana);
            m_ceilingKinds.push_back(CeilingStalactite);
            break;
        case PlaceVault:
            if (!cell.roomPart) {
                m_wallKinds.push_back(WallAlcoves);
                m_wallKinds.push_back(WallColumn);
                m_wallKinds.push_back(WallBracket);
            }
            break;
        case PlaceTemple:
            if (!cell.roomPart) {
                m_wallKinds.push_back(WallAlcoves);
                m_wallKinds.push_back(WallButtress);
                m_wallKinds.push_back(WallBracket);
            }
            m_plantKinds.push_back(CeilingPlant_Liana);
            break;
        case PlacePyramid:
            if (!cell.roomPart) {
                m_wallKinds.push_back(WallAlcoves);
                m_wallKinds.push_back(WallPier);
                m_wallKinds.push_back(WallBracket);
            }
            break;
        }
        if (cell.kind == StructureCell_Room) {
            m_fixtureKinds.push_back(WallFixture_Torch);
        }
        m_ceilingKinds.push_back(CeilingSkylight);
    } else {
        int type = world->zoneBuildingType(cellX, cellY);
        if (type >= BuildingCryptRoom && type <= BuildingCryptUpperRoom) {
            m_fixtureKinds.push_back(WallFixture_Chain);
            m_fixtureKinds.push_back(WallFixture_Torch);
            m_wallKinds.push_back(WallButtress);
            m_wallKinds.push_back(WallAlcoves);
        } else if (type == BuildingCastleCell) {
            m_plantKinds.push_back(CeilingPlant_Chandelier);
            m_fixtureKinds.push_back(WallFixture_Chain);
            m_fixtureKinds.push_back(WallFixture_Torch);
            m_wallKinds.push_back(WallColumn);
        }
    }
}

}
