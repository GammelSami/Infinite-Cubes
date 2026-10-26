#include "cube/World.h"
#include "cube/WorldIds.h"

namespace cube {

void World::biomeSample(int x, int y, BiomeSample &sample)
{
    sample.x = x;
    sample.y = y;
    // The position displaced by noise, up to 1024 blocks along each axis: the regions' borders are measured from it,
    // which makes them irregular (as in World::findNearestRegion). The two noises are anisotropic (frequencies 0.0003
    // and 0.0006, swapped between the axes; offsets 8432, 9438 and 567, 1223), each scaled to an eighth of a land.
    plasma::Vector<2, float> offset;
    offset[0] = noise(x * 0.0003 + 8432.0, y * 0.0006 + 9438.0) * 0.125f * (float)BlocksPerLand;
    offset[1] = noise(x * 0.0006 + 567.0, y * 0.0003 + 1223.0) * 0.125f * (float)BlocksPerLand;
    Fixed positionX = Fixed(x) + offset[0];
    Fixed positionY = Fixed(y) + offset[1];
    int minX = floorDivide(positionX.toBlock(), BlocksPerLand) - 1;
    int minY = floorDivide(positionY.toBlock(), BlocksPerLand) - 1;
    int maxX = floorDivide(positionX.toBlock(), BlocksPerLand) + 1;
    int maxY = floorDivide(positionY.toBlock(), BlocksPerLand) + 1;
    sample.nearestSquaredDistance = -1.0f;
    for (int i = minX; i <= maxX; i++) {
        for (int j = minY; j <= maxY; j++) {
            plasma::Vector<2, int> center = landCenter(i, j);
            float dx = (float)(positionX - Fixed(center[0]));
            float dy = (float)(positionY - Fixed(center[1]));
            float squaredDistance = dy * dy + dx * dx;
            int biome = landBiome(i, j);
            sample.biomes[i - minX][j - minY] = biome;
            sample.squaredDistances[i - minX][j - minY] = squaredDistance;
            if (sample.nearestSquaredDistance < 0.0f || squaredDistance < sample.nearestSquaredDistance) {
                sample.nearestSquaredDistance = squaredDistance;
                sample.nearestBiome = biome;
            }
        }
    }
}

float World::height(int x, int y, int mode)
{
    BiomeSample sample;
    biomeSample(x, y, sample);
    AreaInfo area;
    fillAreaInfo(x, y, area);
    float mountains = sample.mountains();
    float elevation = sample.biomeElevation(area.type, area.influence);
    float road = streetAmount(x, y, area.type, area.influence);
    float firelands = sample.lavaWeight(area.type, area.influence);
    float undead = sample.undead(area.type, area.influence);
    float river = valleyWater(x, y, road, firelands, undead, sample.wetness(), mountains, &area);
    float lavaRiver = lavaChannels(x, y, road, firelands, mountains, &area);
    float poisonRiver = poisonChannels(x, y, road, undead, mountains, &area);
    float cliffs = mountainCliffs(x, y, elevation, mountains, river, lavaRiver, poisonRiver, road, &area);
    float desertness = sample.desertness();
    float sand = sandAmount(x, y, sample.humidity(), cliffs, desertness);
    return height(x, y, elevation, cliffs, river, lavaRiver, poisonRiver, sand, road, mountains, &area, mode);
}

bool World::isLargePlace(int x, int y)
{
    // A hash of the cell pair (salt 3776829): one pair in two is large.
    if ((random(floorDivide(x, 2) + 3776829, floorDivide(y, 2)) & 1) == 0 || isLargePlaceAlways(x, y)) {
        return true;
    }
    return false;
}

bool World::isLargePlaceAlways(int x, int y)
{
    // The cell pairs on the diagonal lattice x + 5y = 1 (mod 7): one in seven.
    return ((floorDivide(x, 2) + floorDivide(y, 2) * 5) % 7 + 7) % 7 == 1;
}

}
