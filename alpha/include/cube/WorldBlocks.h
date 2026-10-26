// Block read, write, clear and lighting helpers use the internal default blocks from
// DefaultBlocks.h. Each translation unit including these helpers gets its own copies.
#pragma once
#include "cube/DefaultBlocks.h"
#include "cube/World.h"

namespace cube {
// The light of an open (air or water) block is its colour channel, at least MinimumBlockLight;
// GameController::shade gives solid blocks that level.
enum {
    MinimumBlockLight = 5
};

inline void World::setBlock(int x, int y, int z, const Block &block, Zone *zone)
{
    Field *field = getField(x, y, zone);
    if (!field)
        return;
    if (block.type == BlockAir && z >= field->m_baseZ + field->m_size)
        return;
    int oldBaseZ = field->m_baseZ;
    if (z <= 0 && !(block.type & (BlockTypeMask | (1 << BlockFlagInterior))))
        field->setBlock(z - oldBaseZ, waterCube);
    else
        field->setBlock(z - oldBaseZ, block);
    for (int fillZ = z + 1; fillZ < oldBaseZ; fillZ++) {
        plasma::Vector<3, unsigned char> color(rockColor(x, y, fillZ, zone));
        field->setBlock(fillZ - field->m_baseZ, Block(color, BlockBuilding));
    }
}

inline const Block &World::getBlock(Fixed x, Fixed y, Fixed z, Zone *zone)
{
    // Subtracts a whole block for every negative coordinate, including negative integers.
    if (x.raw < 0)
        x -= Fixed(1);
    if (y.raw < 0)
        y -= Fixed(1);
    if (z.raw < 0)
        z -= Fixed(1);
    return getBlock((int)x, (int)y, (int)z, zone);
}

inline const Block &World::getBlock(int x, int y, int z, Zone *zone)
{
    Field *field = getField(x, y, zone);
    if (!field)
        return solidCube;
    if (z < field->m_baseZ)
        return solidCube;
    if (z >= field->m_baseZ + field->m_size)
        return z <= 0 ? waterCube : airCube;
    const Block &block = field->getBlock(z - field->m_baseZ);
    if (!(block.type & BlockTypeMask) && z <= 0 && !(block.type & (1 << BlockFlagInterior)))
        return waterCube;
    return block;
}

inline void World::destroyBlock(int x, int y, int z)
{
    Field *field = getField(x, y);
    if (!field)
        return;
    if (z < field->m_baseZ || z >= field->m_baseZ + field->m_size)
        return;
    if (z <= 0)
        field->setBlock(z - field->m_baseZ, waterCube);
    else
        field->setBlock(z - field->m_baseZ, airCube);
}

inline int World::cubeLight(int x, int y, int z, Zone *zone, int channel)
{
    const Block &block = getBlock(x, y, z, zone);
    int type = block.type & ~BlockFlagMask;
    if (type == 13)
        return 255;
    if (type != BlockAir && type != BlockWater)
        return 0;
    if (block.color[channel] < MinimumBlockLight)
        return MinimumBlockLight;
    return block.color[channel];
}

}
