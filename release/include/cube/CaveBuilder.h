// CaveBuilder shapes a zone's ground into earth and rock mounds, tunnels, plateaus and boulders. Zone::generate
// creates a builder for each operation; the builder holds a pointer to that zone.
#pragma once
#include "cube/Race.h"
#include "cube/Zone.h"
#include <windows.h>
#include <algorithm>
#include <functional>

namespace cube {

class Zone;

// Tests whether a block lies in a noise-roughened ellipsoid with the lower half cut flat at the centre's height.
bool ellipsoidContains(int x, int y, int z, int centerX, int centerY, int centerZ, int sizeX, int sizeY, int sizeZ);
// Calls the function with every column (x, y) of x0 <= x < x1, y0 <= y < y1 that lies in the zone, yielding the thread
// before each (Sleep(0)).
inline void forEachColumn(Zone *zone, int x0, int x1, int y0, int y1, const std::function<void(int, int)> &function)
{
    x0 = (std::max)(x0, zone->m_position[0] * BlocksPerZone);
    y0 = (std::max)(y0, zone->m_position[1] * BlocksPerZone);
    x1 = (std::min)(x1, zone->m_position[0] * BlocksPerZone + BlocksPerZone);
    y1 = (std::min)(y1, zone->m_position[1] * BlocksPerZone + BlocksPerZone);
    for (int x = x0; x < x1; x++) {
        for (int y = y0; y < y1; y++) {
            Sleep(0);
            function(x, y);
        }
    }
}

class CaveBuilder {
public:
    explicit CaveBuilder(Zone *zone);

    // A mound over the ellipse of the half-axes (sizeX / 2, sizeY / 2) around (x, y), up to about z, of rock (type 6:
    // addCaveRock, addPeak) or earth (type 5: addCaveSoil, addHill), with the terrain's cover block and snow on top.
    // The two cave shapes leave a tunnel open along a line from the centre in a random direction, below the terrain
    // value of each column; `lined` gives its rim a wall of flagged blocks.
    void addCaveRock(int x, int y, int z, int sizeX, int sizeY, bool lined);
    void addCaveSoil(int x, int y, int z, int sizeX, int sizeY, bool lined);
    void addHill(int x, int y, int z, int sizeX, int sizeY);
    void addPeak(int x, int y, int z, int sizeX, int sizeY);
    // Builds a slab with the terrain's cover on top and an underside hanging deepest at the centre of the ellipse.
    void addPlateau(int x, int y, int z, int sizeX, int sizeY);
    // Fills the part of the ellipsoid above the terrain with rock blocks and the supplied flags.
    void raiseBoulder(int x, int y, int z, int sizeX, int sizeY, int sizeZ, int flags);
    // A rock of the half-axes (sizeX, sizeY, sizeZ) around (x, y, z), its colour running from the terrain's cover at
    // the foot to the rock's.
    void raiseRock(int x, int y, int z, int sizeX, int sizeY, int sizeZ, int flags);

    Zone *m_zone;
};

}
