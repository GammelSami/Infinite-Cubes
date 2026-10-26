#include <algorithm>
// See cube/CaveBuilder.h.
#include "cube/CaveBuilder.h"
#include "cube/MathConstants.h"

#include "cube/Block.h"
#include "cube/Field.h"
#include "cube/Noise.h"
#include "cube/World.h"
#include "cube/Zone.h"

#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <algorithm>
#include <functional>

namespace cube {

namespace {
// The cave mounds' tunnel axis runs from the centre to a point this many half-axes away in a random direction
// (addCaveRock, addCaveSoil).
const int TunnelAxisLengthPerSize = 4;
}

// The four mounds (addCaveRock, addCaveSoil, addHill, addPeak) share one noise profile. A column's top is z plus up to
// ten blocks of noise at frequency 0.01 (offsets 4394, 8974). The ellipse is tested in normalized coordinates
// (half-axes sizeX / 2, sizeY / 2), each roughened by a quarter-weight noise at frequency 0.05 (offsets 2337 for x;
// 4374, 9898 for y). The noise offsets only decorrelate the samples.

CaveBuilder::CaveBuilder(Zone *zone)
    : m_zone(zone)
{}

void CaveBuilder::addCaveRock(int x, int y, int z, int sizeX, int sizeY, bool lined)
{
    float angle = (float)((double)rand() * TwoPi / (double)RAND_MAX);
    int endX = (int)(cosf(angle) * (float)(sizeX * TunnelAxisLengthPerSize) + (float)x);
    int endY = (int)(sinf(angle) * (float)(sizeY * TunnelAxisLengthPerSize) + (float)y);
    forEachColumn(m_zone, x - sizeX, x + sizeX + 1, y - sizeY, y + sizeY + 1, [&](int blockX, int blockY) {
        int top = (int)(noise(blockX * 0.01 + 4394.0, blockY * 0.01 + 8974.0) * 10.0f + (float)z);
        Field *field = m_zone->field(blockX, blockY);
        if (!field) {
            return;
        }
        int bottom = field->m_baseZ;
        if (field->m_sandAmount > 0.5f) {
            top--;
        }
        // The tunnel's ceiling: roughNormalizedX block under the terrain value, and up to ten blocks higher within
        // eight blocks of the tunnel's axis.
        float ceiling = field->m_height - 1.0f;
        float squaredDistance = lineSquaredDistance(plasma::Vector<2, int>(x, y), plasma::Vector<2, int>(endX, endY),
                                                    Fixed(blockX), Fixed(blockY));
        if (squaredDistance < 64.0f) {
            float part = squaredDistance / 64.0f;
            ceiling += (1.0f - part * part) * 10.0f;
        }
        for (int blockZ = top; blockZ >= bottom; blockZ--) {
            Block current = m_zone->getBlock(blockX, blockY, blockZ);
            if (current.flags & BlockFlagProtected) {
                return;
            }
            if (current.flags & BlockFlagDungeon) {
                return;
            }
            float roughNormalizedX =
                noise(blockY * 0.05, blockZ * 0.05 + 2337.0) * 0.25f + (float)(blockX - x) / ((float)sizeX * 0.5f);
            float roughNormalizedY = noise(blockX * 0.05 + 4374.0, blockZ * 0.05 + 9898.0) * 0.25f +
                                     (float)(blockY - y) / ((float)sizeY * 0.5f);
            float distance = roughNormalizedX * roughNormalizedX + roughNormalizedY * roughNormalizedY;
            if (distance <= 1.0f) {
                if (ceiling > (float)blockZ) {
                    // A lined cave walls its outer shell (beyond 0.85 of the squared normalized radius) with rock,
                    // shaded by two quarter-frequency noises (offsets 89003, 10010) into 70..100% of the rock colour.
                    if (lined && distance > 0.85f) {
                        float shade = (float)fabs(noise(blockY * 0.25 + 89003.0, blockZ * 0.25 + 10010.0) +
                                                  noise(blockX * 0.25, blockZ * 0.25));
                        shade = (std::min)(1.0f, shade);

                        Block wall = BlockProperties<float>(
                            m_zone->m_world->rockColor(blockX, blockY, blockZ, field->m_temperature, field->m_humidity,
                                                       field->m_firelands, field->m_undead) *
                                (shade * 0.3f + 0.7f),
                            BlockGround, BlockFlagProtected | BlockFlagBombable, 0);
                        m_zone->setBlock(blockX, blockY, blockZ, wall);
                    } else if (blockZ == bottom) {
                        Block floor = m_zone->m_world->coverBlock(
                            blockX, blockY, blockZ, field->m_height, field->m_temperature, field->m_humidity, 0.0f,
                            0.0f, 0.0f, field->m_firelands, field->m_undead, field->m_enchantedForest);
                        m_zone->setBlock(blockX, blockY, blockZ, floor);
                    } else {
                        Block hollow(plasma::Vector<3, unsigned char>(0, 0, 0), BlockAir, BlockFlagProtected, 0);
                        m_zone->setBlock(blockX, blockY, blockZ, hollow);
                    }
                } else {
                    BlockProperties<float> properties;
                    if (blockZ == top && field->m_sandAmount <= 0.5f) {
                        properties = m_zone->m_world->coverBlock(
                            blockX, blockY, blockZ, field->m_height, field->m_temperature, field->m_humidity, 0.0f,
                            0.0f, 0.0f, field->m_firelands, field->m_undead, field->m_enchantedForest);
                    } else {
                        properties.type = BlockBuilding;
                        properties.color =
                            m_zone->m_world->rockColor(blockX, blockY, blockZ, field->m_temperature, field->m_humidity,
                                                       field->m_firelands, field->m_undead);
                    }
                    Block block = properties;
                    m_zone->setBlock(blockX, blockY, blockZ, block);
                    if (blockZ == top && m_zone->m_world->hasLyingSnow(blockX, blockY, field->m_temperature, 0.0f, 0.0f,
                                                                       0.0f, 0.0f) != 0.0f) {
                        Block snow = BlockProperties<float>(m_zone->m_world->snowColor(blockX, blockY), BlockSnow,
                                                            BlockFlagDestructible, 0);
                        m_zone->setBlock(blockX, blockY, blockZ + 1, snow);
                    }
                }
            }
        }
    });
}

void CaveBuilder::addCaveSoil(int x, int y, int z, int sizeX, int sizeY, bool lined)
{
    float angle = (float)((double)rand() * TwoPi / (double)RAND_MAX);
    int endX = (int)(cosf(angle) * (float)(sizeX * TunnelAxisLengthPerSize) + (float)x);
    int endY = (int)(sinf(angle) * (float)(sizeY * TunnelAxisLengthPerSize) + (float)y);
    forEachColumn(m_zone, x - sizeX, x + sizeX + 1, y - sizeY, y + sizeY + 1, [&](int blockX, int blockY) {
        int top = (int)(noise(blockX * 0.01 + 4394.0, blockY * 0.01 + 8974.0) * 10.0f + (float)z);
        Field *field = m_zone->field(blockX, blockY);
        if (!field) {
            return;
        }
        int bottom = field->m_baseZ;
        if (field->m_sandAmount > 0.5f) {
            top--;
        }
        float ceiling = field->m_height - 1.0f;
        float squaredDistance = lineSquaredDistance(plasma::Vector<2, int>(x, y), plasma::Vector<2, int>(endX, endY),
                                                    Fixed(blockX), Fixed(blockY));
        if (squaredDistance < 64.0f) {
            float part = squaredDistance / 64.0f;
            ceiling += (1.0f - part * part) * 10.0f;
        }
        for (int blockZ = top; blockZ >= bottom; blockZ--) {
            if (m_zone->getBlock(blockX, blockY, blockZ).flags & BlockFlagProtected) {
                return;
            }
            if (m_zone->getBlock(blockX, blockY, blockZ).flags & BlockFlagDungeon) {
                return;
            }
            float roughNormalizedX =
                noise(blockY * 0.05, blockZ * 0.05 + 2337.0) * 0.25f + (float)(blockX - x) / ((float)sizeX * 0.5f);
            float roughNormalizedY = noise(blockX * 0.05 + 4374.0, blockZ * 0.05 + 9898.0) * 0.25f +
                                     (float)(blockY - y) / ((float)sizeY * 0.5f);
            float distance = roughNormalizedX * roughNormalizedX + roughNormalizedY * roughNormalizedY;
            if (distance <= 1.0f) {
                if (ceiling > (float)blockZ) {
                    // A lined cave walls its outer shell (beyond 0.85 of the squared normalized radius) with rock,
                    // shaded by two quarter-frequency noises (offsets 89003, 10010) into 70..100% of the rock colour.
                    if (lined && distance > 0.85f) {
                        float shade = (float)fabs(noise(blockY * 0.25 + 89003.0, blockZ * 0.25 + 10010.0) +
                                                  noise(blockX * 0.25, blockZ * 0.25));
                        shade = (std::min)(1.0f, shade);

                        Block wall = BlockProperties<float>(
                            m_zone->m_world->earthColor(blockX, blockY, blockZ, field->m_temperature, field->m_humidity,
                                                        field->m_firelands, field->m_undead, field->m_enchantedForest) *
                                (shade * 0.3f + 0.7f),
                            BlockGround, BlockFlagProtected | BlockFlagBombable, 0);
                        m_zone->setBlock(blockX, blockY, blockZ, wall);
                    } else if (blockZ == bottom) {
                        Block floor = m_zone->m_world->coverBlock(
                            blockX, blockY, blockZ, field->m_height, field->m_temperature, field->m_humidity, 0.0f,
                            0.0f, 0.0f, field->m_firelands, field->m_undead, field->m_enchantedForest);
                        m_zone->setBlock(blockX, blockY, blockZ, floor);
                    } else {
                        Block hollow(plasma::Vector<3, unsigned char>(0, 0, 0), BlockAir, BlockFlagProtected, 0);
                        m_zone->setBlock(blockX, blockY, blockZ, hollow);
                    }
                } else {
                    BlockProperties<float> properties;
                    if (blockZ == top && field->m_sandAmount <= 0.5f) {
                        properties = m_zone->m_world->coverBlock(
                            blockX, blockY, blockZ, field->m_height, field->m_temperature, field->m_humidity, 0.0f,
                            0.0f, 0.0f, field->m_firelands, field->m_undead, field->m_enchantedForest);
                    } else {
                        properties.type = BlockGround;
                        properties.color =
                            m_zone->m_world->earthColor(blockX, blockY, blockZ, field->m_temperature, field->m_humidity,
                                                        field->m_firelands, field->m_undead, field->m_enchantedForest);
                    }
                    Block block = properties;
                    m_zone->setBlock(blockX, blockY, blockZ, block);
                    if (blockZ == top && m_zone->m_world->hasLyingSnow(blockX, blockY, field->m_temperature, 0.0f, 0.0f,
                                                                       0.0f, 0.0f) != 0.0f) {
                        Block snow = BlockProperties<float>(m_zone->m_world->snowColor(blockX, blockY), BlockSnow,
                                                            BlockFlagDestructible, 0);
                        m_zone->setBlock(blockX, blockY, blockZ + 1, snow);
                    }
                }
            }
        }
    });
}

void CaveBuilder::addHill(int x, int y, int z, int sizeX, int sizeY)
{
    forEachColumn(m_zone, x - sizeX, x + sizeX + 1, y - sizeY, y + sizeY + 1, [&](int blockX, int blockY) {
        int top = (int)(noise(blockX * 0.01 + 4394.0, blockY * 0.01 + 8974.0) * 10.0f + (float)z);
        Field *field = m_zone->field(blockX, blockY);
        if (!field) {
            return;
        }
        int bottom = field->m_baseZ;
        if (field->m_sandAmount > 0.5f) {
            top--;
        }
        for (int blockZ = top; blockZ >= bottom; blockZ--) {
            if (m_zone->getBlock(blockX, blockY, blockZ).flags & BlockFlagProtected) {
                return;
            }
            if (m_zone->getBlock(blockX, blockY, blockZ).flags & BlockFlagDungeon) {
                return;
            }
            float roughNormalizedX =
                noise(blockY * 0.05, blockZ * 0.05 + 2337.0) * 0.25f + (float)(blockX - x) / ((float)sizeX * 0.5f);
            float roughNormalizedY = noise(blockX * 0.05 + 4374.0, blockZ * 0.05 + 9898.0) * 0.25f +
                                     (float)(blockY - y) / ((float)sizeY * 0.5f);
            if (roughNormalizedX * roughNormalizedX + roughNormalizedY * roughNormalizedY <= 1.0f) {
                BlockProperties<float> properties;
                if (blockZ == top && field->m_sandAmount <= 0.5f) {
                    properties = m_zone->m_world->coverBlock(
                        blockX, blockY, blockZ, field->m_height, field->m_temperature, field->m_humidity, 0.0f, 0.0f,
                        0.0f, field->m_firelands, field->m_undead, field->m_enchantedForest);
                } else {
                    properties.type = BlockGround;
                    properties.color =
                        m_zone->m_world->earthColor(blockX, blockY, blockZ, field->m_temperature, field->m_humidity,
                                                    field->m_firelands, field->m_undead, field->m_enchantedForest);
                }
                Block block = properties;
                m_zone->setBlock(blockX, blockY, blockZ, block);
                if (blockZ == top && m_zone->m_world->hasLyingSnow(blockX, blockY, field->m_temperature, 0.0f, 0.0f,
                                                                   0.0f, 0.0f) != 0.0f) {
                    Block snow = BlockProperties<float>(m_zone->m_world->snowColor(blockX, blockY), BlockSnow,
                                                        BlockFlagDestructible, 0);
                    m_zone->setBlock(blockX, blockY, blockZ + 1, snow);
                }
            }
        }
    });
}

void CaveBuilder::addPeak(int x, int y, int z, int sizeX, int sizeY)
{
    forEachColumn(m_zone, x - sizeX, x + sizeX + 1, y - sizeY, y + sizeY + 1, [&](int blockX, int blockY) {
        int top = (int)(noise(blockX * 0.01 + 4394.0, blockY * 0.01 + 8974.0) * 10.0f + (float)z);
        Field *field = m_zone->field(blockX, blockY);
        if (!field) {
            return;
        }
        int bottom = field->m_baseZ;
        if (field->m_sandAmount > 0.5f) {
            top--;
        }
        for (int blockZ = top; blockZ >= bottom; blockZ--) {
            if (m_zone->getBlock(blockX, blockY, blockZ).flags & BlockFlagProtected) {
                return;
            }
            if (m_zone->getBlock(blockX, blockY, blockZ).flags & BlockFlagDungeon) {
                return;
            }
            float roughNormalizedX =
                noise(blockY * 0.05, blockZ * 0.05 + 2337.0) * 0.25f + (float)(blockX - x) / ((float)sizeX * 0.5f);
            float roughNormalizedY = noise(blockX * 0.05 + 4374.0, blockZ * 0.05 + 9898.0) * 0.25f +
                                     (float)(blockY - y) / ((float)sizeY * 0.5f);
            if (roughNormalizedX * roughNormalizedX + roughNormalizedY * roughNormalizedY <= 1.0f) {
                BlockProperties<float> properties;
                if (blockZ == top && field->m_sandAmount <= 0.5f) {
                    properties = m_zone->m_world->coverBlock(
                        blockX, blockY, blockZ, field->m_height, field->m_temperature, field->m_humidity, 0.0f, 0.0f,
                        0.0f, field->m_firelands, field->m_undead, field->m_enchantedForest);
                } else {
                    properties.type = BlockBuilding;
                    properties.color =
                        m_zone->m_world->rockColor(blockX, blockY, blockZ, field->m_temperature, field->m_humidity,
                                                   field->m_firelands, field->m_undead);
                }
                Block block = properties;
                m_zone->setBlock(blockX, blockY, blockZ, block);
                if (blockZ == top && m_zone->m_world->hasLyingSnow(blockX, blockY, field->m_temperature, 0.0f, 0.0f,
                                                                   0.0f, 0.0f) != 0.0f) {
                    Block snow = BlockProperties<float>(m_zone->m_world->snowColor(blockX, blockY), BlockSnow,
                                                        BlockFlagDestructible, 0);
                    m_zone->setBlock(blockX, blockY, blockZ + 1, snow);
                }
            }
        }
    });
}

void CaveBuilder::addPlateau(int x, int y, int z, int sizeX, int sizeY)
{
    forEachColumn(m_zone, x - sizeX, x + sizeX + 1, y - sizeY, y + sizeY + 1, [&](int blockX, int blockY) {
        Field *field = m_zone->field(blockX, blockY);
        if (!field) {
            return;
        }
        // The plateau's noise profile: the surface is z plus up to four blocks of the mounds' top noise (0.01; 4394,
        // 8974); the rim is the unit circle in coordinates normalized by the full sizes, roughened by tenth-weight
        // noise at 0.05 (offsets 8384, 2337; 3244, 2133).
        float surface = noise(blockX * 0.01 + 4394.0, blockY * 0.01 + 8974.0) * 4.0f + (float)z;
        int top = (int)surface;
        float roughNormalizedX =
            noise(blockX * 0.05 + 8384.0, blockY * 0.05 + 2337.0) * 0.1f + (float)(blockX - x) / (float)sizeX;
        float roughNormalizedY =
            noise(blockX * 0.05 + 3244.0, blockY * 0.05 + 2133.0) * 0.1f + (float)(blockY - y) / (float)sizeY;
        float rim = std::min(roughNormalizedX * roughNormalizedX + roughNormalizedY * roughNormalizedY, 1.0f);
        if (rim >= 1.0f) {
            return;
        }
        rim *= rim;
        // The underside hangs ten to twenty blocks lower at the centre and meets the surface at the rim. Its roughness
        // is two absolute noises: a fine one (0.15; 34234, 2424) at 0.3 weight and a coarse one (0.08; 1034, 545).
        float roughness = (float)fabs(noise(blockX * 0.15 + 34234.0, blockY * 0.15 + 2424.0)) * 0.3f +
                          (float)fabs(noise(blockX * 0.08 + 1034.0, blockY * 0.08 + 545.0));
        int bottom = (int)(((float)(z - 10) - (1.0f - rim) * 10.0f - roughness * 10.0f) * (1.0f - rim) + rim * surface);
        for (int blockZ = top; blockZ >= bottom; blockZ--) {
            if (m_zone->getBlock(blockX, blockY, blockZ).flags & BlockFlagProtected) {
                continue;
            }
            if (m_zone->getBlock(blockX, blockY, blockZ).flags & BlockFlagDungeon) {
                continue;
            }
            BlockProperties<float> properties;
            if (blockZ == top) {
                // Snow covers the top where the temperature is below 0.15.
                if (field->m_temperature < 0.15f) {
                    properties = BlockProperties<float>(m_zone->m_world->snowColor(blockX, blockY), BlockSnow,
                                                        BlockFlagDestructible, 0);
                } else {
                    properties = m_zone->m_world->coverBlock(
                        blockX, blockY, blockZ, field->m_height, field->m_temperature, field->m_humidity, 0.0f, 0.0f,
                        0.0f, field->m_firelands, field->m_undead, field->m_enchantedForest);
                }
            } else {
                // The ground's colour, greyer the rougher the place (luma weights 0.3, 0.59, 0.11; the divisor is kept
                // above 0.001).
                plasma::Vector<3, float> color =
                    m_zone->m_world->groundColor(blockX, blockY, blockZ) * (roughness * 0.5f + 0.5f);
                float grey = color[0] * 0.3f + color[1] * 0.59f + color[2] * 0.11f;
                float saturation = 1.0f / std::max(roughness * 0.5f + 0.5f, 0.001f);
                color = color * saturation + plasma::Vector<3, float>(grey, grey, grey) * (1.0f - saturation);
                properties = BlockProperties<float>(color, BlockGround);
            }
            if (blockZ == bottom) {
                properties.type = BlockAir;
                properties.lightLevel = 0xff;
            }
            Block block = properties;
            m_zone->setBlock(blockX, blockY, blockZ, block);
            if (blockZ == top &&
                m_zone->m_world->hasLyingSnow(blockX, blockY, field->m_temperature, 0.0f, 0.0f, 0.0f, 0.0f) != 0.0f) {
                Block snow = BlockProperties<float>(m_zone->m_world->snowColor(blockX, blockY), BlockSnow,
                                                    BlockFlagDestructible, 0);
                m_zone->setBlock(blockX, blockY, blockZ + 1, snow);
            }
        }
    });
}

bool ellipsoidContains(int x, int y, int z, int centerX, int centerY, int centerZ, int sizeX, int sizeY, int sizeZ)
{
    // The rough ellipsoid: each normalized axis (half-axes size / 2) is perturbed by noise, x by 0.25 and y by 0.75 of
    // a 0.02-frequency sample, z by half of a 0.04 one (offsets 3432, 65465; 4374, 7665; 4374, 9898). Only the part
    // above the centre counts in the height.
    float height = (std::max)(0.0f, (float)(z - centerZ));
    float roughNormalizedX =
        noise(y * 0.02 + 3432.0, z * 0.02 + 65465.0) * 0.25f + (float)(x - centerX) / ((float)sizeX * 0.5f);
    float roughNormalizedY =
        noise(x * 0.02 + 4374.0, z * 0.02 + 7665.0) * 0.75f + (float)(y - centerY) / ((float)sizeY * 0.5f);
    float roughNormalizedZ = noise(x * 0.04 + 4374.0, y * 0.04 + 9898.0) * 0.5f + height / ((float)sizeZ * 0.5f);
    roughNormalizedZ *= roughNormalizedZ;
    return roughNormalizedY * roughNormalizedY + roughNormalizedX * roughNormalizedX +
               roughNormalizedZ * roughNormalizedZ <=
           1.0f;
}

void CaveBuilder::raiseBoulder(int x, int y, int z, int sizeX, int sizeY, int sizeZ, int flags)
{
    forEachColumn(m_zone, x - sizeX, x + sizeX + 1, y - sizeY, y + sizeY + 1, [&](int blockX, int blockY) {
        Field *field = m_zone->field(blockX, blockY);
        if (!field) {
            return;
        }
        for (int blockZ = (int)field->m_height; blockZ <= z + sizeZ * 2; blockZ++) {
            if (m_zone->getBlock(blockX, blockY, blockZ).flags & BlockFlagProtected) {
                continue;
            }
            if (m_zone->getBlock(blockX, blockY, blockZ).flags & BlockFlagDungeon) {
                continue;
            }
            if (ellipsoidContains(blockX, blockY, blockZ, x, y, z, sizeX, sizeY, sizeZ)) {
                Block block = BlockProperties<float>(m_zone->m_world->rockColor(blockX, blockY, blockZ,
                                                                                field->m_temperature, field->m_humidity,
                                                                                field->m_firelands, field->m_undead),
                                                     BlockBuilding, (unsigned char)flags, 0);
                m_zone->setBlock(blockX, blockY, blockZ, block);
            }
        }
    });
}

void CaveBuilder::raiseRock(int x, int y, int z, int sizeX, int sizeY, int sizeZ, int flags)
{
    plasma::Vector<3, float> cover = m_zone->m_world->computeCoverBlock(x, y, z).color;
    for (int blockX = x - sizeX; blockX <= x + sizeX; blockX++) {
        for (int blockY = y - sizeY; blockY <= y + sizeY; blockY++) {
            Field *field = m_zone->field(blockX, blockY);
            if (!field) {
                continue;
            }
            for (int blockZ = z - sizeZ * 2; blockZ <= z + sizeZ * 2; blockZ++) {
                // ellipsoidContains' noise profile at frequency 0.05 and weights 0.25, 0.25, 0.5, over the full sizes.
                float roughNormalizedX =
                    noise(blockY * 0.05 + 3432.0, blockZ * 0.05 + 65465.0) * 0.25f + (float)(blockX - x) / (float)sizeX;
                float roughNormalizedY =
                    noise(blockX * 0.05 + 4374.0, blockZ * 0.05 + 7665.0) * 0.25f + (float)(blockY - y) / (float)sizeY;
                float roughNormalizedZ =
                    noise(blockX * 0.05 + 4374.0, blockY * 0.05 + 9898.0) * 0.5f + (float)(blockZ - z) / (float)sizeZ;
                if (roughNormalizedX * roughNormalizedX + roughNormalizedY * roughNormalizedY +
                        roughNormalizedZ * roughNormalizedZ <=
                    1.0f) {
                    // Blends from the terrain cover colour at the foot to rock colour from the middle upwards,
                    // clamping the blend factor.
                    float rock = std::max(0.0f, (float)(blockZ - z + sizeZ) / (float)sizeZ + 0.5f);
                    rock = std::min(1.0f, rock);
                    Block block = BlockProperties<float>(
                        m_zone->m_world->rockColor(blockX, blockY, blockZ, field->m_temperature, field->m_humidity,
                                                   field->m_firelands, field->m_undead) *
                                rock +
                            cover * (1.0f - rock),
                        BlockBuilding, (unsigned char)flags, 0);
                    m_zone->setBlock(blockX, blockY, blockZ, block);
                }
            }
        }
    }
}

}
