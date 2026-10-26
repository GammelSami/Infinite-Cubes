#include <algorithm>
// The shape functions call Sleep(0) in their column loops: the zones are generated on a worker thread, which yields
// between columns.
#include "cube/StructureBuilder.h"
#include "cube/MathConstants.h"
#include "cube/Noise.h"
#include "cube/ServerUpdates.h"
#include "cube/World.h"
#include "cube/WorldLayout.h"
#include "cube/Zone.h"
#include <windows.h>
#include "cube/BlockIds.h"
#include "cube/WorldIds.h"

namespace cube {
StructureBuilder::StructureBuilder(Zone &zone, int x, int y, int z, int sizeX, int sizeY, int rotation, bool mirrored,
                                   ServerUpdates *updates)
    : m_zone(&zone),
      m_updates(updates),
      m_fillColor0(255.0f, 255.0f, 255.0f),
      m_fillColor1(255.0f, 255.0f, 255.0f),
      m_fillTexture(TexturePlain),
      m_guard(0),
      m_terrainOnly(0),
      m_fillType(BlockBuilding)
{
    setPlacement(x, y, z, sizeX, sizeY, rotation, mirrored);
}

StructureBuilder::StructureBuilder(Zone &zone, ServerUpdates *updates)
    : m_zone(&zone),
      m_updates(updates),
      m_fillColor0(255.0f, 255.0f, 255.0f),
      m_fillColor1(255.0f, 255.0f, 255.0f),
      m_fillTexture(TexturePlain),
      m_guard(0),
      m_terrainOnly(0),
      m_fillType(BlockBuilding)
{
    m_placement.identity();
}

StructureBuilder::StructureBuilder(const StructureBuilder &parent, int x, int y, int z, int sizeX, int sizeY,
                                   int rotation, bool mirrored)
    : m_zone(parent.m_zone),
      m_updates(parent.m_updates),
      m_material(parent.m_material),
      m_fillColor0(255.0f, 255.0f, 255.0f),
      m_fillColor1(255.0f, 255.0f, 255.0f),
      m_fillTexture(TexturePlain),
      m_guard(0),
      m_terrainOnly(0),
      m_fillType(BlockBuilding)
{
    m_material.level0 -= (float)z;
    m_material.level1 -= (float)z;
    m_material.level2 -= (float)z;
    setPlacement(parent, x, y, z, sizeX, sizeY, rotation, mirrored);
}

void StructureBuilder::angularColumn(int x, int y, int z, int halfWidth, int height, bool grounded)
{
    // The profile: a 2-block base one block wider on each side, the shaft, a 3-block capital one block wider and a
    // 1-block abacus two wider.
    int left = x - halfWidth;
    int front = y - halfWidth;
    int width = halfWidth * 2;
    if (grounded) {
        underpinnedBox(left - 1, front - 1, z, width + 2, width + 2, 2, FillMaterial, CoverAll, false);
    } else {
        box(left - 1, front - 1, z, width + 2, width + 2, 2, false);
    }
    box(left, front, z + 2, width, width, height - 2, false);
    box(left - 1, front - 1, z + height - 3, width + 2, width + 2, 3, false);
    box(left - 2, front - 2, z + height - 1, width + 4, width + 4, 1, false);
}

void StructureBuilder::angularPillar(int x, int y, int z, int halfWidth, int height, bool grounded)
{
    // The profile: a 2-block base and a 2-block capital, each one block wider on each side than the shaft.
    int left = x - halfWidth;
    int front = y - halfWidth;
    int width = halfWidth * 2;
    if (grounded) {
        underpinnedBox(left - 1, front - 1, z, width + 2, width + 2, 2, FillMaterial, CoverAll, false);
    } else {
        box(left - 1, front - 1, z, width + 2, width + 2, 2, false);
    }
    box(left, front, z + 2, width, width, height - 2, false);
    box(left - 1, front - 1, z + height - 2, width + 2, width + 2, 2, false);
}

void StructureBuilder::battlementX(int x, int y, int z, int length, int size, int height, int unused)
{
    for (int i = x; i < x + length; i++) {
        if ((i - x) / size % 2 != 1) {
            for (int j = y; j < y + size; j++) {
                if (isInZone(i, j)) {
                    for (int k = z; k < z + height; k++) {
                        setCubeBlock(i, j, k, Block(BlockProperties<float>(colorAt(i, j, k), BlockBuilding, 0, 0)));
                    }
                }
            }
        }
    }
}

void StructureBuilder::battlementY(int x, int y, int z, int length, int size, int height, int unused)
{
    for (int j = y; j < y + length; j++) {
        if ((j - y) / size % 2 != 1) {
            for (int i = x; i < x + size; i++) {
                if (isInZone(i, j)) {
                    for (int k = z; k < z + height; k++) {
                        setCubeBlock(i, j, k, Block(BlockProperties<float>(colorAt(i, j, k), BlockBuilding, 0, 0)));
                    }
                }
            }
        }
    }
}

void StructureBuilder::box(int x, int y, int z, int sizeX, int sizeY, int sizeZ, bool relative)
{
    for (int i = x; i < x + sizeX; i++) {
        for (int j = y; j < y + sizeY; j++) {
            Sleep(0);
            if (isInZone(i, j)) {
                for (int k = z; k < z + sizeZ; k++) {
                    setCubeBlock(i, j, k,
                                 Block(BlockProperties<float>(colorAt(i, j, k), (unsigned char)m_fillType,
                                                              BlockFlagProtected, 0)));
                }
            }
        }
    }
}

plasma::Vector<3, float> StructureBuilder::brickColor(const plasma::Vector<3, float> &color, int x, int y, int z)
{
    toZone(x, y, z);
    plasma::Vector<3, float> base = color;
    plasma::Vector<3, float> result = base;
    // One noise sample per brick: the coordinates are quantized to two-block bricks, offset by row (z), and spread by
    // the strides 154 and 53; a brick whose noise is above 0.7 gets up to 10% lighter.
    float brickNoise = noise((double)((z + x) / 2 * 154 + z * 53), (double)((z + y) / 2 * 154 + z * 53));
    if (brickNoise > 0.7f) {
        result = base * ((brickNoise - 0.7f) / 0.3f * 0.1f + 1.0f);
    }
    return result;
}

void StructureBuilder::chain(int x, int y, int z, int length)
{
    // Alternating links: every other slice is a link along y in the dark iron (50, 55, 60), with a cross piece along x
    // in the light iron (60, 65, 70); every fourth slice is a link along x, the others along y.
    for (int i = 0; i < length; i++) {
        if ((i + 1) & 1) {
            setCubeBlock(x, y, z + i, Block(50, 55, 60, BlockSolid, BlockFlagProtected));
            if (i > 0) {
                setCubeBlock(x + 1, y, z + i, Block(60, 65, 70, BlockSolid, BlockFlagProtected));
                setCubeBlock(x - 1, y, z + i, Block(60, 65, 70, BlockSolid, BlockFlagProtected));
            }
            setCubeBlock(x, y + 1, z + i, Block(50, 55, 60, BlockSolid, BlockFlagProtected));
            setCubeBlock(x, y - 1, z + i, Block(50, 55, 60, BlockSolid, BlockFlagProtected));
        } else if ((i + 1) % 4 == 0) {
            setCubeBlock(x - 1, y, z + i, Block(60, 65, 70, BlockSolid, BlockFlagProtected));
            setCubeBlock(x + 1, y, z + i, Block(60, 65, 70, BlockSolid, BlockFlagProtected));
        } else {
            setCubeBlock(x, y + 1, z + i, Block(50, 55, 60, BlockSolid, BlockFlagProtected));
            setCubeBlock(x, y - 1, z + i, Block(50, 55, 60, BlockSolid, BlockFlagProtected));
        }
    }
}

plasma::Vector<3, float> StructureBuilder::colorAt(int x, int y, int z)
{
    float level = (float)z;
    if (m_material.relative) {
        level -= m_zone->m_world->height(zoneX(x, y), zoneY(x, y), HeightFlagSkipRiverBeds);
    }
    if (m_material.level0 >= level) {
        return texturedColor(m_material.color0, x, y, z, m_material.texture0, m_material.seed);
    }
    if (m_material.level1 > level) {
        if (m_material.level0 >= m_material.level1) {
            return texturedColor(m_material.color1, x, y, z, m_material.texture1, m_material.seed);
        }
        float blend = (level - m_material.level0) / (m_material.level1 - m_material.level0);
        return texturedColor(m_material.color0, x, y, z, m_material.texture0, m_material.seed) * (1.0f - blend) +
               texturedColor(m_material.color1, x, y, z, m_material.texture1, m_material.seed) * blend;
    }
    if (m_material.color2 == plasma::Vector<3, float>(0.0f, 0.0f, 0.0f)) {
        return texturedColor(m_material.color1, x, y, z, m_material.texture1, m_material.seed);
    }
    if (m_material.level2 > level) {
        if (m_material.level1 >= m_material.level2) {
            return texturedColor(m_material.color2, x, y, z, m_material.texture2, m_material.seed);
        }
        float blend = (level - m_material.level1) / (m_material.level2 - m_material.level1);
        return texturedColor(m_material.color1, x, y, z, m_material.texture1, m_material.seed) * (1.0f - blend) +
               texturedColor(m_material.color2, x, y, z, m_material.texture2, m_material.seed) * blend;
    }
    return texturedColor(m_material.color2, x, y, z, m_material.texture2, m_material.seed);
}

void StructureBuilder::cone(int centerX, int centerY, int z, int radius, int height)
{
    for (int i = centerX - radius; i <= centerX + radius; i++) {
        for (int j = centerY - radius; j <= centerY + radius; j++) {
            Sleep(0);
            if (isInZone(i, j)) {
                plasma::Vector<2, float> offset((float)(i - centerX) + 0.5f, (float)(j - centerY) + 0.5f);
                float squaredDistance = offset.dot(offset);
                if (!(squaredDistance >= (float)(radius * radius))) {
                    int top = (int)((1.0f - sqrtf(squaredDistance) / (float)radius) * (float)height + (float)(z + 1));
                    for (int k = z; k < top; k++) {
                        setCubeBlock(i, j, k,
                                     Block(BlockProperties<float>(colorAt(i, j, k), (unsigned char)m_fillType, 0, 0)));
                    }
                }
            }
        }
    }
}

BlockProperties<float> World::computeCoverBlock(int x, int y, int z)
{
    BiomeSample sample;
    biomeSample(x, y, sample);
    AreaInfo info;
    fillAreaInfo(x, y, info);
    float elevation = sample.biomeElevation(info.type, info.influence);
    float mountains = sample.mountains();
    float desertness = sample.desertness();
    float firelands = sample.lavaWeight(info.type, info.influence);
    float undead = sample.undead(info.type, info.influence);
    float road = streetAmount(x, y, info.type, info.influence);
    float wetlands = sample.wetness();
    float river = valleyWater(x, y, road, firelands, undead, wetlands, mountains, &info);
    float lavaRiver = lavaChannels(x, y, road, firelands, mountains, &info);
    float poisonRiver = poisonChannels(x, y, road, undead, mountains, &info);
    float cliffs = mountainCliffs(x, y, elevation, mountains, river, lavaRiver, poisonRiver, road, &info);
    float humidity = sample.humidity();
    float sand = sandAmount(x, y, humidity, cliffs, desertness);
    float enchantedForest = sample.enclave(info.type, info.influence);
    float temperature = sample.temperature(info.type, info.influence);
    float terrainHeight =
        height(x, y, elevation, cliffs, river, lavaRiver, poisonRiver, sand, road, mountains, &info, 0);
    return coverBlock(x, y, z, terrainHeight, temperature, humidity, sand, 0.0f, road, firelands, undead,
                      enchantedForest);
}

Block StructureBuilder::cube(int x, int y, int z)
{
    toZone(x, y, z);
    return m_zone->getBlock(x, y, z);
}

void StructureBuilder::cutBox(int x, int y, int z, int sizeX, int sizeY, int sizeZ, unsigned char flags)
{
    for (int i = x; i < x + sizeX; i++) {
        Sleep(0);
        for (int j = y; j < y + sizeY; j++) {
            if (isInZone(i, j)) {
                for (int k = z; k < z + sizeZ; k++) {
                    setCubeBlock(
                        i, j, k,
                        Block(plasma::Vector<3, unsigned char>(0, 0, 0), BlockAir, flags | BlockFlagProtected, 0));
                }
            }
        }
    }
}

void StructureBuilder::cylinder(int centerX, int centerY, int z, int radius, int height, int fill, int cover,
                                int unused)
{
    for (int i = centerX - radius; i <= centerX + radius; i++) {
        for (int j = centerY - radius; j <= centerY + radius; j++) {
            Sleep(0);
            if (isInZone(i, j)) {
                float dx = (float)(i - centerX) + 0.5f;
                float dy = (float)(j - centerY) + 0.5f;
                if (!(dx * dx + dy * dy >= (float)(radius * radius))) {
                    int top = z + height;
                    bool covered = hasCover(i, j, top - 1, cover);
                    if (!covered) {
                        top--;
                    }
                    for (int k = z; k < top; k++) {
                        if (k < top - 1) {
                            setCubeBlock(i, j, k,
                                         Block(BlockProperties<float>(colorAt(i, j, k), (unsigned char)m_fillType,
                                                                      BlockFlagProtected, 0)));
                        } else if (!covered) {
                            setCubeBlock(i, j, k,
                                         Block(BlockProperties<float>(colorAt(i, j, k), (unsigned char)m_fillType,
                                                                      BlockFlagProtected, 0)));
                        } else {
                            setCube(i, j, k, fill, BlockFlagProtected);
                        }
                    }
                }
            }
        }
    }
}

void StructureBuilder::cylinderShaft(int centerX, int centerY, int z, int radius, int height, int flags)
{
    for (int i = centerX - radius; i <= centerX + radius; i++) {
        for (int j = centerY - radius; j <= centerY + radius; j++) {
            Sleep(0);
            if (isInZone(i, j)) {
                plasma::Vector<2, float> offset((float)(i - centerX) + 0.5f, (float)(j - centerY) + 0.5f);
                if (!(offset.dot(offset) >= (float)(radius * radius))) {
                    int top = z + height;
                    // The shaft's light varies by noise (frequency 0.05) between none and full, -1..1 mapped by * 1.5
                    // + 0.5 to bytes.
                    double noiseY = zoneY(i, j) * 0.05;
                    float brightness = (noise(zoneX(i, j) * 0.05, noiseY) * 1.5f + 0.5f) * 127.5f;
                    int lightLevel = (int)std::max(brightness, 0.0f);
                    for (int k = z; k < top; k++) {
                        setCubeBlock(i, j, k,
                                     Block(plasma::Vector<3, unsigned char>(0, 0, 0), BlockAir, flags,
                                           (unsigned char)lightLevel));
                    }
                }
            }
        }
    }
}

void StructureBuilder::foundation(int centerX, int centerY, int z, int radius, int height, int fill, int cover)
{
    for (int i = centerX - radius; i <= centerX + radius; i++) {
        for (int j = centerY - radius; j <= centerY + radius; j++) {
            Sleep(0);
            if (isInZone(i, j)) {
                plasma::Vector<2, float> offset((float)(i - centerX) + 0.5f, (float)(j - centerY) + 0.5f);
                if (!(offset.dot(offset) >= (float)(radius * radius))) {
                    int k = z;
                    for (;; k--) {
                        Block below = zoneBlock(i, j, k);
                        if (below.isSolid() && !(below.flags & BlockFlagDestructible)) {
                            break;
                        }
                    }
                    int top = z + height;
                    bool covered = hasCover(i, j, top - 1, cover);
                    if (!covered) {
                        top--;
                    }
                    for (; k < top; k++) {
                        if (k < top - 1) {
                            setCubeBlock(
                                i, j, k,
                                Block(BlockProperties<float>(colorAt(i, j, k), BlockBuilding, BlockFlagProtected, 0)));
                        } else if (!covered) {
                            setCubeBlock(
                                i, j, k,
                                Block(BlockProperties<float>(colorAt(i, j, k), BlockBuilding, BlockFlagProtected, 0)));
                        } else {
                            setCube(i, j, k, fill, BlockFlagProtected);
                        }
                    }
                }
            }
        }
    }
}

plasma::Vector<3, float> World::groundColor(int x, int y, int z)
{
    BiomeSample sample;
    biomeSample(x, y, sample);
    int cellX = floorDivide(x, BlocksPerPlace);
    int cellY = floorDivide(y, BlocksPerPlace);
    bool large = isLargePlace(cellX, cellY);
    int type = locationType(cellX, cellY, large);
    float influence;
    if (type != PlaceNone) {
        float squaredDistance = placeSquaredDistance(placeCenter(cellX, cellY, large), x, y);
        int radius = SmallPlaceRadius;
        if (large) {
            radius = LargePlaceRadius;
        }
        float squaredRadius = (float)(radius * radius);
        if (squaredDistance >= squaredRadius) {
            influence = 0.0f;
        } else {
            influence = (1.0f - squaredDistance / squaredRadius) * (1.0f - squaredDistance / squaredRadius);
        }
    } else {
        influence = 0.0f;
    }
    float enchantedForest = sample.enclave(type, influence);
    float undead = sample.undead(type, influence);
    float firelands = sample.lavaWeight(type, influence);
    float humidity = sample.humidity();
    float temperature = sample.temperature(type, influence);
    return earthColor(x, y, z, temperature, humidity, firelands, undead, enchantedForest);
}

bool StructureBuilder::hasCover(int x, int y, int z, int cover)
{
    if (cover == CoverNone) {
        return false;
    }
    toZone(x, y, z);
    switch (cover) {
    case CoverPatches:
        // Half the columns: positive noise at frequency 0.06 (offsets 7631, 983).
        return noise(x * 0.06 + 7631.0, y * 0.06 + 983.0) > 0.0f;
    case CoverWorn:
        // Most blocks: noise over two-block cells (frequency 0.1, row stride 53) above -0.3.
        return noise((double)((z + x) / 2 * 2) * 0.1 + (double)(z * 53),
                     (double)((z + y) / 2 * 2) * 0.1 + (double)(z * 53)) > -0.3f;
    }
    return true;
}

// Turns the placement by quarter turns about the z axis: columns 0 and 1 become combinations of each other with the
// cosine and sine of the angle.
static void integerRotate(plasma::Matrix<int> &matrix, int turns)
{
    int cosine = 1;
    int sine = 0;
    switch (turns % 4) {
    case QuarterTurn_One:
        cosine = 0;
        sine = -1;
        break;
    case QuarterTurn_Two:
        cosine = -1;
        break;
    case QuarterTurn_Three:
        cosine = 0;
        sine = 1;
        break;
    }
    {
        int x = matrix(0, 0);
        matrix(0, 0) = sine * matrix(0, 1) + cosine * x;
        matrix(0, 1) = cosine * matrix(0, 1) - sine * x;
    }
    {
        int x = matrix(1, 0);
        matrix(1, 0) = sine * matrix(1, 1) + cosine * x;
        matrix(1, 1) = cosine * matrix(1, 1) - sine * x;
    }
    {
        int x = matrix(2, 0);
        matrix(2, 0) = sine * matrix(2, 1) + cosine * x;
        matrix(2, 1) = cosine * matrix(2, 1) - sine * x;
    }
    {
        int x = matrix(3, 0);
        matrix(3, 0) = sine * matrix(3, 1) + cosine * x;
        matrix(3, 1) = cosine * matrix(3, 1) - sine * x;
    }
}

void StructureBuilder::ladder(int x, int y, int z, int width, int height)
{
    for (int n = 0; n < width; n++) {
        int i = x - width / 2 + n;
        for (int k = z; k < z + height; k++) {
            float shade = (float)(k % 2) * 0.1f + 0.9f;
            setCubeBlock(i, y, k, Block(BlockProperties<float>(colorAt(i, y, k) * shade, BlockType_16, 0, 0)));
        }
    }
}

void StructureBuilder::lava(int x, int y, int z, int sizeX, int sizeY, int sizeZ, int flags)
{
    for (int i = x; i < x + sizeX; i++) {
        for (int j = y; j < y + sizeY; j++) {
            Sleep(0);
            if (isInZone(i, j)) {
                for (int k = z; k < z + sizeZ; k++) {
                    if (!zoneBlock(i, j, k).isSolid()) {
                        setCubeBlock(i, j, k,
                                     Block(BlockProperties<float>(m_zone->m_world->lavaColor(zoneX(i, j), zoneY(i, j)),
                                                                  BlockLava, flags, 255)));
                    }
                }
            }
        }
    }
}

void StructureBuilder::leaves(int x, int y, int z, float radius)
{
    // The foliage's silhouette: a sphere squashed to half height, bulged by two noises (frequencies 0.1 and 0.04,
    // weights 0.3 and 0.5) sampled at a random offset below 1000.
    int seedX = rand() % 1000;
    int seedY = rand() % 1000;
    for (int i = (int)((float)x - radius - 1.0f); (float)i < radius + (float)x + 2.0f; i++) {
        for (int j = (int)((float)y - radius - 1.0f); (float)j < radius + 2.0f + (float)y; j++) {
            if (isInZone(i, j)) {
                float bulge = noise(i * 0.1 + seedX, j * 0.1 + seedY) * 0.3f;
                bulge += noise(i * 0.04 + seedX, j * 0.04 + seedY) * 0.5;
                for (int k = (int)((float)z - radius - 1.0f); (float)k < (float)z + (radius + 2.0f); k++) {
                    plasma::Vector<3, float> offset((float)(i - x) + 0.5f, (float)(j - y) + 0.5f,
                                                    (float)((k - z) * 2) + 0.5f + bulge * 0.5f * radius);
                    if (!(offset.squaredLength() > radius * radius)) {
                        setCubeBlock(
                            i, j, k,
                            Block(BlockProperties<float>(colorAt(i, j, k), BlockLeaves, BlockFlagProtected, 0)));
                    }
                }
            }
        }
    }
}

void StructureBuilder::light(int centerX, int centerY, int centerZ, int radius, unsigned char light)
{
    for (int i = centerX - radius; i < centerX + radius; i++) {
        for (int j = centerY - radius; j < centerY + radius; j++) {
            if (isInZone(i, j)) {
                for (int k = centerZ - radius; k < centerZ + radius; k++) {
                    int squaredDistance =
                        (j - centerY) * (j - centerY) + (i - centerX) * (i - centerX) + (k - centerZ) * (k - centerZ);
                    if (squaredDistance < radius * radius) {
                        Block lit = zoneBlock(i, j, k);
                        if (lit.flags & BlockFlagProtected) {
                            // The falloff divides by the radius and multiplies by it again.
                            int sum = lit.lightLevel + (light - light * squaredDistance / radius * radius);
                            // The colour is halved by the conversion on every pass.
                            BlockProperties<float> brighter(plasma::Vector<3, float>(lit.color), lit.type, lit.flags,
                                                            (unsigned char)sum);
                            if (sum > 255) {
                                brighter.lightLevel = 255;
                            }
                            setCubeBlock(i, j, k, Block(brighter));
                        }
                    }
                }
            }
        }
    }
}

plasma::Vector<3, float> StructureBuilder::lowRockColor(int x, int y, int z)
{
    toZone(x, y, z);
    return m_zone->m_world->rockColor(x, y, z) * 0.3f;
}

void StructureBuilder::muck(int x, int y, int z, int sizeX, int sizeY, int sizeZ, int flags)
{
    for (int i = x; i < x + sizeX; i++) {
        for (int j = y; j < y + sizeY; j++) {
            Sleep(0);
            if (isInZone(i, j)) {
                Sleep(0);
                for (int k = z; k < z + sizeZ; k++) {
                    if (!zoneBlock(i, j, k).isSolid()) {
                        setCubeBlock(i, j, k,
                                     Block(BlockProperties<float>(m_zone->m_world->muckColor(zoneX(i, j), zoneY(i, j)),
                                                                  BlockPoison, flags, 0)));
                    }
                }
            }
        }
    }
}

plasma::Vector<3, float> StructureBuilder::rockColor(int x, int y, int z)
{
    toZone(x, y, z);
    return m_zone->m_world->rockColor(x, y, z);
}

plasma::Vector<3, float> World::rockColor(int x, int y, int z)
{
    BiomeSample sample;
    biomeSample(x, y, sample);
    int cellX = floorDivide(x, BlocksPerPlace);
    int cellY = floorDivide(y, BlocksPerPlace);
    bool large = isLargePlace(cellX, cellY);
    int type = locationType(cellX, cellY, large);
    float influence;
    if (type != PlaceNone) {
        float squaredDistance = placeSquaredDistance(placeCenter(cellX, cellY, large), x, y);
        int radius = SmallPlaceRadius;
        if (large) {
            radius = LargePlaceRadius;
        }
        float squaredRadius = (float)(radius * radius);
        if (squaredDistance >= squaredRadius) {
            influence = 0.0f;
        } else {
            influence = (1.0f - squaredDistance / squaredRadius) * (1.0f - squaredDistance / squaredRadius);
        }
    } else {
        influence = 0.0f;
    }
    float undead = sample.undead(type, influence);
    float firelands = sample.lavaWeight(type, influence);
    float humidity = sample.humidity();
    float temperature = sample.temperature(type, influence);
    return rockColor(x, y, z, temperature, humidity, firelands, undead);
}

void StructureBuilder::roundPillar(int x, int y, int z, int radius, int height, bool grounded)
{
    // angularColumn's profile in cylinders: a 2-block base of radius + 1, the shaft, a 3-block capital of radius + 1
    // and a 1-block top of radius + 2.
    if (grounded) {
        foundation(x, y, z, radius + 1, 2, FillMaterial, CoverAll);
    } else {
        cylinder(x, y, z, radius + 1, 2, 0, 0, 0);
    }
    cylinder(x, y, z + 2, radius, height - 2, 0, 0, 0);
    cylinder(x, y, z + height - 3, radius + 1, 3, 0, 0, 0);
    cylinder(x, y, z + height - 1, radius + 2, 1, 0, 0, 0);
}

void StructureBuilder::setColor(const plasma::Vector<3, float> &color, int texture)
{
    m_material.color0 = color;
    m_material.texture0 = texture;
    m_material.level0 = 0.0f;
    m_material.color1 = color;
    m_material.texture1 = texture;
    m_material.level1 = 0.0f;
    m_material.color2 = color;
    m_material.texture2 = texture;
    m_material.level2 = 0.0f;
}

void StructureBuilder::setCube(int x, int y, int z, int fill, int flags)
{
    switch (fill) {
    case FillBlend: {
        int zoneX = x;
        int zoneY = y;
        int zoneZ = z;
        toZone(zoneX, zoneY, zoneZ);
        // The two fill colours blended by noise (frequency 0.02; offsets 958, 8744).
        float blend = (noise(zoneX * 0.02 + 958.0, zoneY * 0.02 + 8744.0) + 1.0f) * 0.5f;
        plasma::Vector<3, float> color = m_fillColor0 * (1.0f - blend) + m_fillColor1 * blend;
        // The zone's coordinates are passed where the builder's are expected.
        m_zone->setBlock(zoneX, zoneY, zoneZ,
                         Block(BlockProperties<float>(texturedColor(color, zoneX, zoneY, zoneZ, m_fillTexture, 0),
                                                      (unsigned char)m_fillType, flags, 0)));
        return;
    }
    case FillGrass: {
        int zoneX = x;
        int zoneY = y;
        toZone(zoneX, zoneY);
        setCubeBlock(
            x, y, z,
            Block(BlockProperties<float>(m_zone->m_world->harvestGrassColor(zoneX, zoneY), BlockGrass, flags, 0)));
        break;
    }
    case FillSand: {
        int zoneX = x;
        int zoneY = y;
        toZone(zoneX, zoneY);
        setCubeBlock(x, y, z,
                     Block(BlockProperties<float>(m_zone->m_world->sandColor(zoneX, zoneY), BlockSand, flags, 0)));
        break;
    }
    case FillLowRock:
        setCubeBlock(x, y, z, Block(BlockProperties<float>(lowRockColor(x, y, z), BlockRock, flags, 0)));
        break;
    case FillRock:
        setCubeBlock(x, y, z, Block(BlockProperties<float>(rockColor(x, y, z), BlockBuilding, flags, 0)));
        break;
    case FillCover: {
        int zoneX = x;
        int zoneY = y;
        int zoneZ = z;
        toZone(zoneX, zoneY, zoneZ);
        Block cover(m_zone->m_world->computeCoverBlock(zoneX, zoneY, zoneZ));
        cover.flags |= flags;
        m_zone->setBlock(zoneX, zoneY, zoneZ, cover);
        return;
    }
    case FillFoundation:
        if (cube(x, y, z).isSolid() && cube(x, y, z + 1).isSolid()) {
            setCubeBlock(x, y, z, Block(BlockProperties<float>(colorAt(x, y, z), BlockBuilding, flags, 0)));
        } else {
            int zoneX = x;
            int zoneY = y;
            int zoneZ = z;
            toZone(zoneX, zoneY, zoneZ);
            BlockProperties<float> cover = m_zone->m_world->computeCoverBlock(zoneX, zoneY, zoneZ);
            cover.flags |= flags;
            m_zone->setBlock(zoneX, zoneY, zoneZ, Block(cover));
        }
        break;
    case FillChecker: {
        // Four-block squares of white and grey (100).
        plasma::Vector<3, float> color;
        if ((x / 4 + y / 4) & 1) {
            color = plasma::Vector<3, float>(255.0f, 255.0f, 255.0f);
        } else {
            color = plasma::Vector<3, float>(100.0f, 100.0f, 100.0f);
        }
        setCubeBlock(x, y, z, Block(BlockProperties<float>(color, BlockBuilding, flags, 0)));
        break;
    }
    default:
        setCubeBlock(x, y, z, Block(BlockProperties<float>(colorAt(x, y, z), (unsigned char)m_fillType, flags, 0)));
        break;
    }
}

void StructureBuilder::setCubeBlock(int x, int y, int z, const Block &block)
{
    toZone(x, y, z);
    if (m_terrainOnly > 0) {
        float limit = m_zone->m_world->roughness(x, y, m_terrainOnly, 10) +
                      m_zone->m_world->height(x, y, HeightFlagSkipRiverBeds);
        if ((float)z > limit) {
            return;
        }
    }
    if (m_guard != 0) {
        Block old = m_zone->getBlock(x, y, z);
        if ((m_guard & PlacementKeepSolid) && old.isSolid()) {
            return;
        }
        if ((m_guard & PlacementKeepProtected) && (old.flags & BlockFlagProtected)) {
            return;
        }
        if ((m_guard & PlacementOnlyProtected) && !(old.flags & BlockFlagProtected)) {
            return;
        }
    }
    m_zone->setBlock(x, y, z, block);
    if (m_updates != nullptr) {
        CubeChange change;
        change.newBlock = block;
        change.position = plasma::Vector<3, int>(x, y, z);
        m_updates->m_cubeChangeList.push_back(change);
    }
}

void StructureBuilder::setMaterial(const plasma::Vector<3, float> &color, int texture, float level,
                                   const plasma::Vector<3, float> &color1, int texture1, float level1,
                                   const plasma::Vector<3, float> &color2, int texture2, float level2, bool relative)
{
    m_material.color0 = color;
    m_material.level0 = level;
    m_material.texture0 = texture;
    m_material.color1 = color1;
    m_material.texture1 = texture1;
    m_material.level1 = level1;
    m_material.color2 = color2;
    m_material.texture2 = texture2;
    m_material.relative = relative;
    m_material.level2 = level2;
}

void StructureBuilder::setMaterial(const plasma::Vector<3, float> &color, int texture, float level,
                                   const plasma::Vector<3, float> &color1, int texture1, float level1, bool relative)
{
    m_material.color0 = color;
    m_material.level0 = level;
    m_material.texture0 = texture;
    m_material.color1 = color1;
    m_material.texture1 = texture1;
    m_material.level1 = level1;
    m_material.color2 = color1;
    m_material.relative = relative;
    m_material.level2 = level1;
    m_material.texture2 = texture1;
}

void StructureBuilder::setPlacement(const StructureBuilder &parent, int x, int y, int z, int sizeX, int sizeY,
                                    int rotation, bool mirrored)
{
    setPlacement(x, y, z, sizeX, sizeY, rotation, mirrored);
    m_placement = parent.m_placement * m_placement;
}

void StructureBuilder::setPlacement(int x, int y, int z, int sizeX, int sizeY, int rotation, bool mirrored)
{
    plasma::Matrix<int> &placement = m_placement;
    placement.identity();
    placement.translate(x, y, z);
    switch (rotation % 4) {
    case QuarterTurn_One:
        placement.translate(0, sizeY - 1, 0);
        break;
    case QuarterTurn_Two:
        placement.translate(sizeX - 1, sizeY - 1, 0);
        break;
    case QuarterTurn_Three:
        placement.translate(sizeX - 1, 0, 0);
        break;
    }
    integerRotate(placement, rotation);
    if (mirrored) {
        if (rotation & 1)
            placement.translate(sizeY - 1, 0, 0);
        else
            placement.translate(sizeX - 1, 0, 0);
        placement[0] = -placement[0];
        placement[1] = -placement[1];
        placement[2] = -placement[2];
        placement[3] = -placement[3];
    }
}

void StructureBuilder::sphere(int x, int y, int z, float radius)
{
    for (int i = (int)((float)x - radius - 1.0f); (float)i < radius + (float)x + 2.0f; i++) {
        for (int j = (int)((float)y - radius - 1.0f); (float)j < radius + 2.0f + (float)y; j++) {
            if (isInZone(i, j)) {
                for (int k = (int)((float)z - radius - 1.0f); (float)k < (float)z + (radius + 2.0f); k++) {
                    plasma::Vector<3, float> offset((float)(i - x) + 0.5f, (float)(j - y) + 0.5f,
                                                    (float)(k - z) + 0.5f);
                    if (!(offset.squaredLength() > radius * radius)) {
                        setCubeBlock(i, j, k,
                                     Block(BlockProperties<float>(colorAt(i, j, k), BlockTree, BlockFlagProtected, 0)));
                    }
                }
            }
        }
    }
}

void StructureBuilder::spiral(int centerX, int centerY, int z, int innerRadius, int outerRadius, int height)
{
    for (int i = centerX - outerRadius; i <= centerX + outerRadius; i++) {
        for (int j = centerY - outerRadius; j <= centerY + outerRadius; j++) {
            Sleep(0);
            if (isInZone(i, j)) {
                float dx = (float)(i - centerX) - 0.5f;
                float dy = (float)(j - centerY) - 0.5f;
                float squaredDistance = dy * dy + dx * dx;
                if (!(squaredDistance >= (float)(outerRadius * outerRadius)) &&
                    !((float)(innerRadius * innerRadius) > squaredDistance)) {
                    float angle = acosf(dx / std::sqrt(squaredDistance));
                    int k = z;
                    for (;; k--) {
                        Block below = zoneBlock(i, j, k);
                        if (below.isSolid() && !(below.flags & BlockFlagDestructible)) {
                            break;
                        }
                    }
                    int top = (int)((float)height * angle / Pi + z);
                    for (; k < top; k++) {
                        setCubeBlock(i, j, k,
                                     Block(BlockProperties<float>(colorAt(i, j, k), (unsigned char)m_fillType, 0, 0)));
                    }
                }
            }
        }
    }
}

void StructureBuilder::stairsX(int x, int y, int z, int length, int width, int startHeight, int endHeight, int fill,
                               int cover, bool unused)
{
    for (int i = x; i < x + length; i++) {
        for (int j = y; j < y + width; j++) {
            if (isInZone(i, j)) {
                int k = z;
                for (int below = z - 1;; below--) {
                    if (zoneBlock(i, j, below).isSolid()) {
                        break;
                    }
                    k--;
                }
                int top = startHeight + z + (endHeight - startHeight) * (i - x) / length;
                bool covered = hasCover(i, j, top - 1, cover);
                int limit = top;
                if (!covered) {
                    limit--;
                }
                for (; k < limit; k++) {
                    if (k == limit - 1) {
                        if (!covered) {
                            setCubeBlock(i, j, k,
                                         Block(BlockProperties<float>(colorAt(i, j, k), (unsigned char)m_fillType,
                                                                      BlockFlagProtected, 0)));
                        } else {
                            setCube(i, j, k, fill, BlockFlagProtected);
                        }
                    } else {
                        setCubeBlock(i, j, k,
                                     Block(BlockProperties<float>(colorAt(i, j, k), (unsigned char)m_fillType,
                                                                  BlockFlagProtected, 0)));
                    }
                }
            }
        }
    }
}

void StructureBuilder::stairsY(int x, int y, int z, int width, int length, int startHeight, int endHeight, int fill,
                               int cover, bool unused)
{
    for (int i = x; i < x + width; i++) {
        for (int j = y; j < y + length; j++) {
            if (isInZone(i, j)) {
                int k = z;
                for (int below = z - 1;; below--) {
                    if (zoneBlock(i, j, below).isSolid()) {
                        break;
                    }
                    k--;
                }
                int top = startHeight + z + (endHeight - startHeight) * (j - y) / length;
                bool covered = hasCover(i, j, top - 1, cover);
                int limit = top;
                if (!covered) {
                    limit--;
                }
                for (; k < limit; k++) {
                    if (k == limit - 1) {
                        if (!covered) {
                            setCubeBlock(i, j, k,
                                         Block(BlockProperties<float>(colorAt(i, j, k), (unsigned char)m_fillType,
                                                                      BlockFlagProtected, 0)));
                        } else {
                            setCube(i, j, k, fill, BlockFlagProtected);
                        }
                    } else {
                        setCubeBlock(i, j, k,
                                     Block(BlockProperties<float>(colorAt(i, j, k), (unsigned char)m_fillType,
                                                                  BlockFlagProtected, 0)));
                    }
                }
            }
        }
    }
}

void StructureBuilder::stalactite(int x, int y, int z, int radius, int height)
{
    // A cone widening upwards to the radius, its outline displaced by up to 1.5 blocks of noise (frequency 0.1; offset
    // 843 for y), the bounds widened by the same 1.5.
    int step = 0;
    for (int k = z - height; k < z + 2; k++) {
        float reach = (float)step / (float)height * (float)radius;
        for (int i = (int)((float)x - reach - 1.5f); (float)x + reach + 1.5f >= (float)i; i++) {
            for (int j = (int)((float)y - reach - 1.5f); reach + 1.5f + (float)y >= (float)j; j++) {
                if (isInZone(i, j)) {
                    double level = k * 0.1;
                    float dx = noise(zoneY(i, j) * 0.1, level) * 1.5f + (float)(i - x);
                    float dy = noise(zoneX(i, j) * 0.1, level + 843.0) * 1.5f + (float)(j - y);
                    if (!(dy * dy + dx * dx > reach * reach)) {
                        setCubeBlock(i, j, k,
                                     Block(BlockProperties<float>(colorAt(i, j, k), BlockBuilding,
                                                                  BlockFlagProtected | BlockFlagDungeon, 0)));
                    }
                }
            }
        }
        step++;
    }
}

plasma::Vector<3, float> StructureBuilder::texturedColor(const plasma::Vector<3, float> &color, int x, int y, int z,
                                                         int texture, int seed)
{
    if (texture == TextureBrick) {
        return brickColor(color, x, y, z);
    }
    if (texture == TextureWoodX) {
        plasma::Vector<3, float> base = color;
        return base * (noise((double)(x * 77), (double)(seed * 97)) * 0.15f + 1.0f);
    }
    if (texture == TextureWood) {
        return woodColor(color, x, y, z, seed);
    }
    if (texture == TextureWoodY) {
        return woodColorY(color, x, y, z, seed);
    }
    if (texture == TextureWoodZ) {
        return woodColorZ(color, x, y, z, seed);
    }
    if (texture == TextureRock) {
        toZone(x, y, z);
        return m_zone->m_world->rockColor(x, y, z);
    }
    if (texture == TextureGround) {
        toZone(x, y, z);
        return m_zone->m_world->groundColor(x, y, z);
    }
    return color;
}

void StructureBuilder::toZone(int &x, int &y, int &z)
{
    const plasma::Matrix<int> &m = m_placement;
    int zoneX = m[4] * y + x * m[0] + m[8] * z + m[12];
    int zoneY = m[1] * x + m[5] * y + m[9] * z + m[13];
    int zoneZ = m[2] * x + m[6] * y + m[10] * z + m[14];
    x = zoneX;
    y = zoneY;
    z = zoneZ;
}

void StructureBuilder::toZone(int &x, int &y)
{
    const plasma::Matrix<int> &m = m_placement;
    int zoneX = m[4] * y + x * m[0] + m[12];
    int zoneY = m[1] * x + m[5] * y + m[13];
    x = zoneX;
    y = zoneY;
}

void StructureBuilder::toZoneDirection(int &x, int &y)
{
    plasma::Vector<2, int> direction = m_placement.transformDirection(plasma::Vector<2, int>(x, y));
    x = direction[0];
    y = direction[1];
}

int StructureBuilder::toZoneRotation(int rotation)
{
    if (m_placement[0] == 1 || m_placement[0] == -1) {
        if (m_placement[5] == 1)
            return rotation;
        return (rotation + 2) % 4;
    }
    if (m_placement[4] == 1)
        return (rotation + 3) % 4;
    return (rotation + 1) % 4;
}

void StructureBuilder::tubeX(int x, int y, int z, int top, int length, int radius)
{
    for (int i = x; i < x + length; i++) {
        int center = z + (i - x) * (top - z) / length;
        for (int j = y - radius; j <= y + radius; j++) {
            if (isInZone(i, j)) {
                for (int k = center - radius; k <= center + radius; k++) {
                    float dz = (float)(k - center) - 0.5f;
                    float dy = (float)(j - y) - 0.5f;
                    if (!(dz * dz + dy * dy > (float)(radius * radius))) {
                        setCubeBlock(i, j, k,
                                     Block(BlockProperties<float>(colorAt(i, j, k) * ((float)(i % 2) * 0.1f + 0.9f),
                                                                  (unsigned char)m_fillType, 0, 0)));
                    }
                }
            }
        }
    }
}

void StructureBuilder::tubeY(int x, int y, int z, int top, int length, int radius)
{
    for (int j = y; j < y + length; j++) {
        int center = z + (j - y) * (top - z) / length;
        for (int i = x - radius; i <= x + radius; i++) {
            if (isInZone(i, j)) {
                for (int k = center - radius; k <= center + radius; k++) {
                    float dz = (float)(k - center) - 0.5f;
                    float dx = (float)(i - x) - 0.5f;
                    if (!(dz * dz + dx * dx > (float)(radius * radius))) {
                        setCubeBlock(i, j, k,
                                     Block(BlockProperties<float>(colorAt(i, j, k) * ((float)(j % 2) * 0.1f + 0.9f),
                                                                  (unsigned char)m_fillType, 0, 0)));
                    }
                }
            }
        }
    }
}

void StructureBuilder::underpinnedBox(int x, int y, int z, int sizeX, int sizeY, int height, int fill, int cover,
                                      bool unused)
{
    for (int i = x; i < x + sizeX; i++) {
        for (int j = y; j < y + sizeY; j++) {
            Sleep(0);
            if (isInZone(i, j)) {
                int k = z;
                for (;; k--) {
                    Block below = zoneBlock(i, j, k);
                    if (below.isSolid() && !(below.flags & BlockFlagDestructible)) {
                        break;
                    }
                }
                int top = z + height;
                bool covered = hasCover(i, j, top - 1, cover);
                if (!covered) {
                    top--;
                }
                for (; k < top; k++) {
                    if (k < top - 1) {
                        setCubeBlock(i, j, k,
                                     Block(BlockProperties<float>(colorAt(i, j, k), (unsigned char)m_fillType,
                                                                  BlockFlagProtected, 0)));
                    } else if (!covered) {
                        setCubeBlock(i, j, k,
                                     Block(BlockProperties<float>(colorAt(i, j, k), (unsigned char)m_fillType,
                                                                  BlockFlagProtected, 0)));
                    } else {
                        setCube(i, j, k, fill, BlockFlagProtected);
                    }
                }
            }
        }
    }
}

void StructureBuilder::water(int x, int y, int z, int sizeX, int sizeY, int sizeZ, int flags)
{
    for (int i = x; i < x + sizeX; i++) {
        for (int j = y; j < y + sizeY; j++) {
            Sleep(0);
            if (isInZone(i, j)) {
                for (int k = z; k < z + sizeZ; k++) {
                    if (!zoneBlock(i, j, k).isSolid()) {
                        setCubeBlock(i, j, k, Block(127, 127, 127, BlockWater, flags));
                    }
                }
            }
        }
    }
}

// The wood textures: up to 15% lighter or darker along a grain, by noise of the coordinates across the grain (stride
// 77) and the seed (stride 97). texturedColor's TextureWoodX and woodColorY/woodColorZ use one axis each.
plasma::Vector<3, float> StructureBuilder::woodColor(const plasma::Vector<3, float> &color, int x, int y, int z,
                                                     int seed)
{
    plasma::Vector<3, float> base = color;
    return base * (noise((double)(seed + x * 77), (double)(seed + y * 97)) * 0.15f + 1.0f);
}

plasma::Vector<3, float> StructureBuilder::woodColorY(const plasma::Vector<3, float> &color, int x, int y, int z,
                                                      int seed)
{
    plasma::Vector<3, float> base = color;
    return base * (noise((double)(y * 77), (double)(seed * 97)) * 0.15f + 1.0f);
}

plasma::Vector<3, float> StructureBuilder::woodColorZ(const plasma::Vector<3, float> &color, int x, int y, int z,
                                                      int seed)
{
    plasma::Vector<3, float> base = color;
    return base * (noise((double)(z * 77), (double)(seed * 97)) * 0.15f + 1.0f);
}

}
