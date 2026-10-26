// ZoneNeighborhood holds a zone and its eight neighbours, allowing block and column queries beyond one zone's border
// without using the world's zone map. A missing neighbour is null.
#pragma once
#include "cube/Block.h"
#include "cube/Noise.h"
#include "cube/World.h"
#include "cube/Zone.h"
#include "plasma/Vector.h"

namespace gfx {
class ByteGrid;
}

namespace cube {

class ZoneNeighborhood {
public:
    ZoneNeighborhood(World *world, int x, int y)
        : m_position(x, y)
    {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++)
                m_zones[dx + 1][dy + 1] = world->zone(x + dx, y + dy);
        }
    }

    Block cube(int x, int y, int z)
    {
        int zoneY = floorDivide(y, BlocksPerZone);
        int zoneX = floorDivide(x, BlocksPerZone);
        int j = zoneY - m_position[1] + 1;
        int i = zoneX - m_position[0] + 1;
        if (i >= 0 && j >= 0 && i <= 2 && j <= 2 && m_zones[i][j])
            return m_zones[i][j]->getBlock(x, y, z);
        return s_solidCube;
    }

    Block cube(Fixed x, Fixed y, Fixed z) { return cube(x.toBlock(), y.toBlock(), z.toBlock()); }

    Field *field(int x, int y)
    {
        int zoneY = floorDivide(y, BlocksPerZone);
        int zoneX = floorDivide(x, BlocksPerZone);
        int j = zoneY - m_position[1] + 1;
        int i = zoneX - m_position[0] + 1;
        if (i >= 0 && j >= 0 && i <= 2 && j <= 2 && m_zones[i][j]) {
            Zone *zone = m_zones[i][j];
            int zoneBlockX = zone->m_position[0] * BlocksPerZone;
            if (x >= zoneBlockX) {
                int zoneBlockY = zone->m_position[1] * BlocksPerZone;
                if (y >= zoneBlockY && x < zoneBlockX + BlocksPerZone && y < zoneBlockY + BlocksPerZone)
                    return &zone->m_fields[(long long)(x - zoneBlockX) * BlocksPerZone + (y - zoneBlockY)];
            }
        }
        return nullptr;
    }

    plasma::Vector<2, int> m_position; // the centre zone's coordinates
    Zone *m_zones[3][3];               // null where a neighbour is not loaded
};

}
