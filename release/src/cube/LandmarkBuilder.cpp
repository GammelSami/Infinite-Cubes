#include <algorithm>
// Procedural landmark and tree shapes.
#include "cube/LandmarkBuilder.h"
#include "cube/MathConstants.h"
#include "cube/World.h"
#include "cube/WorldLayout.h"
#include "cube/Zone.h"
#include "cube/CaveBuilder.h"
#include <cmath>
#include <algorithm>
#include <list>
#include <cstdlib>
#include "cube/BlockIds.h"
#include "cube/ZoneIds.h"

namespace cube {

namespace {
// One tree in this many (or one foliage tip) gets a bird or owl's sounds.
const int TreeBirdOdds = 10;
}

// Tree shapes use fixed proportions, branch counts, spacings and palettes, with random variation in each recipe.

LandmarkBuilder::LandmarkBuilder(Zone *zone)
    : m_zone(zone),
      m_lowerHeight(0),
      m_upperHeight(0),
      m_lowerColor(255.0f, 255.0f, 255.0f),
      m_upperColor(255.0f, 255.0f, 255.0f),
      m_colorPattern(LandmarkColorGradient)
{}

void LandmarkBuilder::birch(int x, int y, int z, int width, int height)
{
    // A birch: a white flecked trunk (radius 5% of the width), four roots of three quarters of its radius, five twigs
    // (a third of the width) spaced over the upper half, and oval foliage of 50..75% of the half width at the tips.
    setGradient(z, z + height, plasma::Vector<3, float>(255, 300, 300), plasma::Vector<3, float>(255, 300, 300));
    m_colorPattern = LandmarkColorBirchFlecks;
    float radius = (std::max)(1.0f, (float)width * 0.05f);
    trunk(x, y, z, radius, radius, height);
    float rootRadius = (std::max)(1.0f, radius * 0.75f);
    int rootLength = (int)(radius + radius);
    buildBranch(x, y, z, plasma::Vector<3, float>(1, 0, -1), rootRadius, rootRadius, rootLength);
    buildBranch(x, y, z, plasma::Vector<3, float>(-1, 0, -1), rootRadius, rootRadius, rootLength);
    buildBranch(x, y, z, plasma::Vector<3, float>(0, 1, -1), rootRadius, rootRadius, rootLength);
    buildBranch(x, y, z, plasma::Vector<3, float>(0, -1, -1), rootRadius, rootRadius, rootLength);
    std::list<plasma::Vector<3, int>> tips;
    tips.push_back(plasma::Vector<3, int>(x, y, z + height));
    for (int branch = 0; branch < 5; ++branch) {
        float angle = (float)((double)rand() * TwoPi / (double)RAND_MAX);
        float dy = sinf(angle);
        float dx = cosf(angle);
        tips.push_back(
            twig(x, y, z + height / 2 + branch * (height / 2) / 4, plasma::Vector<3, float>(dx, dy, 0.5f), width / 3));
    }
    m_colorPattern = LandmarkColorGradient;
    plasma::Vector<3, float> foliage, foliageTop;
    float temperature;
    {
        World::BiomeSample sample;
        m_zone->m_world->biomeSample(x, y, sample);
        World::AreaInfo area;
        m_zone->m_world->fillAreaInfo(x, y, area);
        float influence = area.influence;
        int areaType = area.type;
        temperature = sample.temperature(areaType, influence);
        float enclave = sample.enclave(areaType, influence);
        float undead = sample.undead(areaType, influence);
        m_zone->m_world->groundColors(x, y, foliage, foliageTop, temperature, undead, enclave);
    }
    for (std::list<plasma::Vector<3, int>>::const_iterator tip = tips.begin(); tip != tips.end(); ++tip) {
        float leafRadius = ((float)rand() * 0.25f / (float)RAND_MAX + 0.5f) * ((float)width * 0.5f);
        float halfHeight = leafRadius * 0.5f;
        setGradient((int)((float)(*tip)[2] - halfHeight), (int)((float)(*tip)[2] + halfHeight), foliage, foliageTop);
        ovalLeaves((*tip)[0], (*tip)[1], (*tip)[2] + 1, leafRadius, halfHeight, temperature < SnowLineTemperature);
        if (rand() % TreeBirdOdds == 0) {
            Zone::Plant plant;
            plant.type = rand() % 3 + PlantBirdAndOwlSounds;
            plant.position = *tip;
            m_zone->addPlant(plant);
        }
    }
}

plasma::Vector<3, int> LandmarkBuilder::buildBranch(int x, int y, int z, const plasma::Vector<3, float> &direction,
                                                    float firstRadius, float lastRadius, int count)
{
    plasma::Vector<3, Fixed> position = plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z));
    for (int step = 0; step < count; ++step) {
        float progress = (float)step / (float)(count - 1);
        sphere((int)position[0], (int)position[1], (int)position[2],
               (1.0f - progress) * firstRadius + progress * lastRadius);
        position[0] += Fixed(direction[0]);
        position[1] += Fixed(direction[1]);
        position[2] += Fixed(progress * direction[2]);
    }
    return fixedToBlock(position);
}

plasma::Vector<3, float> LandmarkBuilder::colorAt(int x, int y, int z)
{
    // The birch's flecks: a hash of the block (salt 1234), one in eight.
    if (m_colorPattern == LandmarkColorBirchFlecks && (hash(x + 1234 + hash(y + hash(z))) & 7) == 0)
        return plasma::Vector<3, float>(50.0f, 50.0f, 50.0f);
    if (z <= m_lowerHeight)
        return m_lowerColor;
    if (z >= m_upperHeight)
        return m_upperColor;
    float progress = (float)(z - m_lowerHeight) / (float)(m_upperHeight - m_lowerHeight);
    return m_lowerColor * (1.0f - progress) + m_upperColor * progress;
}

void LandmarkBuilder::disk(int x, int y, int z, float radius, const Block &block)
{
    forEachColumn(m_zone, x - (int)radius - 1, x + (int)radius + 2, y - (int)radius - 1, y + (int)radius + 2,
                  [&](int blockX, int blockY) {
                      float dx = (float)(blockX - x);
                      float dy = (float)(blockY - y);
                      if (dy * dy + dx * dx <= radius * radius)
                          setBlock(blockX, blockY, z, block);
                  });
}

void LandmarkBuilder::evergreen(int x, int y, int z, int width, int height)
{
    plasma::Vector<3, float> foliage, foliageTop;
    float temperature;
    {
        World::BiomeSample sample;
        m_zone->m_world->biomeSample(x, y, sample);
        World::AreaInfo area;
        m_zone->m_world->fillAreaInfo(x, y, area);
        float influence = area.influence;
        int areaType = area.type;
        float enclave = sample.enclave(areaType, influence);
        temperature = sample.temperature(areaType, influence);
        float undead = sample.undead(areaType, influence);
        m_zone->m_world->groundHashedColors(x, y, foliage, foliageTop, undead, enclave);
    }
    plasma::Vector<3, float> topWood = randomWoodColor();
    plasma::Vector<3, float> lowerWood = randomWoodColor();
    setGradient(z, z + height, lowerWood, topWood);
    // An evergreen: the height raised by 1.5 half widths, a flared trunk (15% of the half width), and 30 branches
    // spiralling up (0.3 pi apart) from 20..40% of the height, each drooping (0.3) and shorter towards the top, drawn
    // as discs of leaves.
    int halfWidth = (int)((float)width * 0.5f);
    height = (int)((double)halfWidth * 1.5 + (double)height);
    float trunkRadius = std::max((float)halfWidth * 0.15f, 1.0f);
    stumpFlare(x, y, z - 1, trunkRadius, 0.5f, height);
    float angle = (float)((double)rand() * TwoPi / (double)RAND_MAX);
    setGradient(z + height / 4, z + height, foliage, foliageTop);
    float firstBranch = (float)rand() * 0.2f / (float)RAND_MAX + 0.2f;
    for (int branch = 0; branch < 30; ++branch) {
        angle = (float)((double)angle + 0.3f * Pi);
        float progress = (float)branch * (1.0f - firstBranch) / 30.0f + firstBranch;
        float dx = cosf(angle) * (float)halfWidth;
        float dy = sinf(angle) * (float)halfWidth;
        float remaining = 1.0f - progress;
        float spread = remaining + remaining;
        plasma::Vector<3, Fixed> center(plasma::Vector<3, double>(x, y, (float)height * progress + (float)z));
        plasma::Vector<3, Fixed> tip = center + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                    dx * spread, dy * spread, (float)-halfWidth * 0.3f * spread));
        int count = height / 3;
        for (int step = count; step > 0; --step) {
            float fraction = (float)step / (float)count;
            float inward = 1.0f - fraction;
            plasma::Vector<3, Fixed> point =
                tip + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(center - tip) * inward);
            float innerRadius = inward * trunkRadius * remaining;
            float radius = std::max(fraction + fraction + (innerRadius + innerRadius), 2.0f);
            if (temperature < SnowLineTemperature) {
                Block snow(BlockProperties<float>(m_zone->m_world->snowColor((int)point[0], (int)point[1]), BlockSnow,
                                                  BlockFlagDestructible, 0));
                disk((int)point[0], (int)point[1], (int)(point[2] + Fixed(1)), radius, snow);
            }
            int blockX = (int)point[0];
            int blockY = (int)point[1];
            int blockZ = (int)point[2];
            Block leaves(
                BlockProperties<float>(colorAt(blockX, blockY, blockZ), BlockLeaves, BlockFlagDestructible, 0));
            disk(blockX, blockY, blockZ, radius, leaves);
        }
        setBlock(x, y, z + height + 1, Block(BlockProperties<float>(colorAt(x, y, z + height + 1), BlockLeaves)));
        setBlock(x, y, z + height, Block(BlockProperties<float>(colorAt(x, y, z + height), BlockLeaves)));
        setBlock(x, y, z + height - 1, Block(BlockProperties<float>(colorAt(x, y, z + height), BlockLeaves)));
    }
    if (rand() % TreeBirdOdds == 0) {
        Zone::Plant plant;
        plant.type = rand() % 3 + PlantBirdAndOwlSounds;
        plant.position = plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z + height - 1));
        m_zone->addPlant(plant);
    }
}

// ovalLeaves' ellipsoid (its height displaced by 0.3 of a noise of two cycles per radius, scanned two blocks beyond),
// with strands hanging from one leaf in 20: up to nine blocks long, doubled for one in four (hash salts 738 and 1993
// draw the length and the doubling independently).
void LandmarkBuilder::hangingLeaves(int x, int y, int z, float radius, float halfHeight)
{
    forEachColumn(m_zone, x - (int)radius - 1, x + (int)radius + 2, y - (int)radius - 1, y + (int)radius + 2,
                  [&](int blockX, int blockY) {
                      float displacement =
                          noise((double)blockX * 2.0 / (double)radius, (double)blockY * 2.0 / (double)radius) * 0.3f;
                      for (int dz = (int)(-halfHeight - 2.0f); (float)dz <= halfHeight + 2.0f; ++dz) {
                          float vertical = (float)dz / halfHeight + displacement;
                          float dx = ((float)(blockX - x) + 0.5f) / radius;
                          float dy = ((float)(blockY - y) + 0.5f) / radius;
                          if (dy * dy + dx * dx + vertical * vertical > 1.0f)
                              continue;
                          Block previous = m_zone->getBlock(blockX, blockY, z + dz);
                          if (previous.type != BlockAir && previous.type != BlockWater)
                              continue;
                          Block block(BlockProperties<float>(colorAt(blockX, blockY, z + dz), BlockLeaves,
                                                             BlockFlagDestructible, 0));
                          setBlock(blockX, blockY, z + dz, block);
                          int branchZ = z + dz;
                          if (hash(blockX + hash(blockY + hash(branchZ))) % 20 == 0) {
                              int length = hash(blockX + 738 + hash(blockY + hash(branchZ))) % 10;
                              if ((hash(blockX + hash(blockY + 1993 + hash(branchZ))) & 3) == 0)
                                  length += length;
                              for (int drop = 0; drop < length; ++drop) {
                                  m_zone->getBlock(blockX, blockY, branchZ - drop - 1);
                                  Block leaf(BlockProperties<float>(colorAt(blockX, blockY, branchZ), BlockLeaves,
                                                                    BlockFlagDestructible, 0));
                                  setBlock(blockX, blockY, branchZ - drop - 1, leaf);
                              }
                          }
                      }
                  });
}

void LandmarkBuilder::jungleTree(int x, int y, int z, int width, int height)
{
    plasma::Vector<3, float> foliage, foliageTop;
    {
        World::BiomeSample sample;
        m_zone->m_world->biomeSample(x, y, sample);
        World::AreaInfo area;
        m_zone->m_world->fillAreaInfo(x, y, area);
        float influence = area.influence;
        int areaType = area.type;
        float temperature = sample.temperature(areaType, influence);
        float humidity = sample.humidity();
        float enclave = sample.enclave(areaType, influence);
        float undead = sample.undead(areaType, influence);
        m_zone->m_world->harvestColors(x, y, foliage, foliageTop, temperature, humidity, undead, enclave);
    }
    // A jungle tree: a brown trunk (10% of the width) darkening to half the foliage colour, four roots, five branches
    // (half the width) over the upper half, hanging foliage of 75..100% of the half width, and a bird seed below
    // 100000.
    setGradient(z, z + height, plasma::Vector<3, float>(200, 150, 80), foliage * 0.5f);
    float radius = (std::max)(1.0f, (float)width * 0.1f);
    trunk(x, y, z, radius, radius, height);
    float rootRadius = (std::max)(1.0f, radius * 0.75f);
    int rootLength = (int)(radius + radius);
    buildBranch(x, y, z + 1, plasma::Vector<3, float>(1, 0, -1), rootRadius, rootRadius, rootLength);
    buildBranch(x, y, z + 1, plasma::Vector<3, float>(-1, 0, -1), rootRadius, rootRadius, rootLength);
    buildBranch(x, y, z + 1, plasma::Vector<3, float>(0, 1, -1), rootRadius, rootRadius, rootLength);
    buildBranch(x, y, z + 1, plasma::Vector<3, float>(0, -1, -1), rootRadius, rootRadius, rootLength);
    float branchRadius = (std::max)(1.0f, radius * 0.5f);
    std::list<plasma::Vector<3, int>> tips;
    tips.push_back(plasma::Vector<3, int>(x, y, z + height));
    for (int branch = 0; branch < 5; ++branch) {
        float angle = (float)((double)rand() * TwoPi / (double)RAND_MAX);
        float dy = sinf(angle);
        float dx = cosf(angle);
        tips.push_back(buildBranch(x, y, z + height / 2 + branch * (height / 2) / 4,
                                   plasma::Vector<3, float>(dx, dy, 1), branchRadius, branchRadius, width / 2));
    }
    int seed = rand() % 100000;
    for (const auto &tip : tips) {
        float leafRadius = ((float)rand() * 0.25f / (float)RAND_MAX + 0.75f) * ((float)width * 0.5f);
        float halfHeight = leafRadius * 0.5f;
        setGradient(tip[2] - height / 2, (int)((float)tip[2] + halfHeight), foliage, foliageTop);
        hangingLeaves(tip[0], tip[1], tip[2] + 1, leafRadius, halfHeight);
        if (m_zone->contains(tip[0], tip[1]) && hash(seed++) % TreeBirdOdds == 0) {
            Zone::Plant plant;
            plant.type = hash(seed++) % 3 + PlantBirdAndOwlSounds;
            plant.position = tip;
            m_zone->addPlant(plant);
        }
    }
}

void LandmarkBuilder::oak(int x, int y, int z, int width, int height)
{
    plasma::Vector<3, float> foliage, foliageTop;
    float temperature;
    float humidity;
    {
        World::BiomeSample sample;
        m_zone->m_world->biomeSample(x, y, sample);
        World::AreaInfo area;
        m_zone->m_world->fillAreaInfo(x, y, area);
        float influence = area.influence;
        int areaType = area.type;
        float enclave = sample.enclave(areaType, influence);
        temperature = sample.temperature(areaType, influence);
        humidity = sample.humidity();
        float undead = sample.undead(areaType, influence);
        m_zone->m_world->harvestColors(x, y, foliage, foliageTop, temperature, humidity, undead, enclave);
    }
    plasma::Vector<3, float> topWood = randomWoodColor();
    plasma::Vector<3, float> lowerWood = randomWoodColor();
    // An oak: a trunk (10% of the width) tapering to 80%, four roots, five branches (eight above a width of 30, ten
    // above a height of 50) of half the width up to 15, flat leaves (0.15) where it is hot (0.7) and dry (0.3) else
    // 0.35..0.7, and above a leaf radius of 16 five clumps of radius 6..12 jittered by up to 5 blocks.
    setGradient(z, z + height, lowerWood, topWood);
    float radius = (std::max)(1.0f, (float)width * 0.1f);
    trunk(x, y, z, radius, radius * 0.8f, height);
    float rootRadius = (std::max)(1.0f, radius * 0.75f);
    int rootLength = (int)(radius + radius);
    buildBranch(x, y, z + 1, plasma::Vector<3, float>(1, 0, -1), rootRadius, rootRadius, rootLength);
    buildBranch(x, y, z + 1, plasma::Vector<3, float>(-1, 0, -1), rootRadius, rootRadius, rootLength);
    buildBranch(x, y, z + 1, plasma::Vector<3, float>(0, 1, -1), rootRadius, rootRadius, rootLength);
    buildBranch(x, y, z + 1, plasma::Vector<3, float>(0, -1, -1), rootRadius, rootRadius, rootLength);
    float branchRadius = (std::max)(1.0f, radius * 0.5f);
    std::list<plasma::Vector<3, int>> tips;
    tips.push_back(plasma::Vector<3, int>(x, y, z + height));
    int branchCount = width <= 30 ? 5 : 8;
    if (height > 50)
        branchCount = 10;
    for (int branch = 0; branch < branchCount; ++branch) {
        float angle = (float)((double)rand() * TwoPi / (double)RAND_MAX);
        float dy = sinf(angle);
        float dx = cosf(angle);
        tips.push_back(buildBranch(x, y, z + height / 2 + branch * (height / 2) / (branchCount - 1),
                                   plasma::Vector<3, float>(dx, dy, 1), branchRadius, branchRadius,
                                   (std::min)(15, width / 2)));
    }
    float leafAspect;
    if (temperature >= 0.7f && humidity < 0.3f)
        leafAspect = 0.15f;
    else
        leafAspect = (float)rand() * 0.35f / (float)RAND_MAX + 0.35f;
    int seed = rand() % 100000;
    for (const auto &tip : tips) {
        float leafRadius = (float)width * 0.5f * 0.75f;
        float halfHeight = leafRadius * leafAspect;
        setGradient((int)((float)tip[2] - halfHeight), (int)((float)tip[2] + halfHeight), foliage, foliageTop);
        if (leafRadius > 16.0f) {
            for (int clump = 0; clump < 5; ++clump) {
                int clumpRadius = rand() % 7 + 6;
                int clumpZ = (int)((float)tip[2] + (float)rand() * 5.0f / (float)RAND_MAX - 2.5f + 1.0f);
                int clumpY = (int)((float)tip[1] + (float)(rand() * 2) * 5.0f / (float)RAND_MAX - 5.0f);
                int clumpX = (int)((float)tip[0] + (float)(rand() * 2) * 5.0f / (float)RAND_MAX - 5.0f);
                ovalLeaves(clumpX, clumpY, clumpZ, (float)clumpRadius, leafAspect * (float)clumpRadius,
                           temperature < SnowLineTemperature);
            }
        } else {
            ovalLeaves(tip[0], tip[1], tip[2] + 1, leafRadius, halfHeight, temperature < SnowLineTemperature);
        }
        if (!m_zone->contains(tip[0], tip[1]))
            continue;
        if (hash(seed++) % TreeBirdOdds == 0) {
            Zone::Plant plant;
            plant.type = hash(seed++) % 3 + PlantBirdAndOwlSounds;
            plant.position = tip;
            m_zone->addPlant(plant);
        }
        Zone::Plant canopy;
        canopy.type = PlantLeafCanopy;
        canopy.color = foliage / 255.0f;
        canopy.position = tip;
        m_zone->addPlant(canopy);
    }
}

// An ellipsoid of leaves whose height is displaced by 0.3 of a noise of two cycles per radius, scanned two blocks
// beyond its half height.
void LandmarkBuilder::ovalLeaves(int x, int y, int z, float radius, float halfHeight, bool snow)
{
    forEachColumn(m_zone, x - (int)radius - 1, x + (int)radius + 2, y - (int)radius - 1, y + (int)radius + 2,
                  [&](int blockX, int blockY) {
                      float displacement =
                          noise((double)blockX * 2.0 / (double)radius, (double)blockY * 2.0 / (double)radius) * 0.3f;
                      for (int dz = (int)(-halfHeight - 2.0f); (float)dz <= halfHeight + 2.0f; ++dz) {
                          float vertical = (float)dz / halfHeight + displacement;
                          float dx = ((float)(blockX - x) + 0.5f) / radius;
                          float dy = ((float)(blockY - y) + 0.5f) / radius;
                          if (dy * dy + dx * dx + vertical * vertical > 1.0f)
                              continue;
                          Block previous = m_zone->getBlock(blockX, blockY, z + dz);
                          if (previous.type != BlockAir && previous.type != BlockWater)
                              continue;
                          Block block(BlockProperties<float>(colorAt(blockX, blockY, z + dz),
                                                             snow ? BlockSnowLeaves : BlockLeaves,
                                                             BlockFlagDestructible, 0));
                          setBlock(blockX, blockY, z + dz, block);
                      }
                  });
}

// A fallen trunk along x (prostrateTrunkY: along y), sunk two blocks: its lower half tinted towards green (20, 100,
// 30), up to two twigs at a random block of its length (height -1..1), growth rings alternating 130% and 150% of the
// colour inside the bark, leaves of radius 1..2.
void LandmarkBuilder::prostrateTrunkX(int x, int y, int z, float radius, int length, bool hollow)
{
    z = (int)((radius - 2.0f) + (float)z);
    World::BiomeSample sample;
    m_zone->m_world->biomeSample(x, y, sample);
    World::AreaInfo area;
    m_zone->m_world->fillAreaInfo(x, y, area);
    float temperature = sample.temperature(area.type, area.influence);
    float humidity = sample.humidity();
    plasma::Vector<3, float> wood = randomWoodColor();
    setGradient((int)((float)z - radius), (int)(radius + (float)z),
                (wood + plasma::Vector<3, float>(20, 100, 30)) * 0.5f, wood);
    std::list<plasma::Vector<3, int>> tips;
    if (rand() % 2 != 0) {
        float vertical = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        float axial = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        plasma::Vector<3, float> direction(axial, 1, vertical);
        int branchZ = z - 1 + rand() % 3;
        int branchX = x + 1 + (rand() % (length - 2) - length / 2);
        tips.push_back(twig(branchX, y + (int)radius + 1, branchZ, direction, (int)radius));
    }
    if (rand() % 2 != 0) {
        float vertical = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        float axial = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        plasma::Vector<3, float> direction(axial, -1, vertical);
        int branchZ = z - 1 + rand() % 3;
        int branchX = x + 1 + (rand() % (length - 2) - length / 2);
        tips.push_back(twig(branchX, y - (int)radius, branchZ, direction, (int)radius));
    }
    forEachColumn(
        m_zone, x - length / 2, x + length / 2, y - (int)radius - 1, y + (int)radius + 2, [&](int blockX, int blockY) {
            for (int blockZ = z - (int)radius - 1; blockZ < z + (int)radius + 2; ++blockZ) {
                float transverse = (float)(blockY - y) - 0.5f;
                float vertical = (float)(blockZ - z) - 0.5f;
                float distanceSquared = vertical * vertical + transverse * transverse;
                if (distanceSquared > radius * radius)
                    continue;
                if (distanceSquared < (radius - 1.0f) * (radius - 1.0f)) {
                    if (hollow) {
                        setBlock(blockX, blockY, blockZ, Block(plasma::Vector<3, unsigned char>(0, 0, 0), BlockAir));
                    } else {
                        float brightness = (float)((int)sqrtf(distanceSquared) % 2) * 0.2f + 1.3f;
                        Block block(BlockProperties<float>(colorAt(blockX, blockY, blockZ) * brightness, BlockTree));
                        setBlock(blockX, blockY, blockZ, block);
                    }
                } else {
                    Block block(BlockProperties<float>(colorAt(blockX, blockY, blockZ), BlockTree));
                    setBlock(blockX, blockY, blockZ, block);
                }
            }
        });
    float enclave = sample.enclave(area.type, area.influence);
    float undead = sample.undead(area.type, area.influence);
    plasma::Vector<3, float> foliage, foliageTop;
    m_zone->m_world->harvestColors(x, y, foliage, foliageTop, temperature, humidity, undead, enclave);
    for (std::list<plasma::Vector<3, int>>::const_iterator tip = tips.begin(); tip != tips.end(); ++tip) {
        float leafRadius = (float)rand() / (float)RAND_MAX + 1.0f;
        setGradient((int)((float)(*tip)[2] - leafRadius), (int)((float)(*tip)[2] + leafRadius), foliage, foliageTop);
        ovalLeaves((*tip)[0], (*tip)[1], (*tip)[2], leafRadius, leafRadius, false);
    }
}

void LandmarkBuilder::prostrateTrunkY(int x, int y, int z, float radius, int length, bool hollow)
{
    z = (int)((radius - 2.0f) + (float)z);
    World::BiomeSample sample;
    m_zone->m_world->biomeSample(x, y, sample);
    World::AreaInfo area;
    m_zone->m_world->fillAreaInfo(x, y, area);
    float temperature = sample.temperature(area.type, area.influence);
    float humidity = sample.humidity();
    plasma::Vector<3, float> wood = randomWoodColor();
    setGradient((int)((float)z - radius), (int)(radius + (float)z),
                (wood + plasma::Vector<3, float>(20, 100, 30)) * 0.5f, wood);
    std::list<plasma::Vector<3, int>> tips;
    if (rand() % 2 != 0) {
        float vertical = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        float axial = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        plasma::Vector<3, float> direction(1, axial, vertical);
        int branchZ = z - 1 + rand() % 3;
        int branchY = y + 1 + (rand() % (length - 2) - length / 2);
        tips.push_back(twig(x + (int)radius + 1, branchY, branchZ, direction, (int)radius));
    }
    if (rand() % 2 != 0) {
        float vertical = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        float axial = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        plasma::Vector<3, float> direction(-1, axial, vertical);
        int branchZ = z - 1 + rand() % 3;
        int branchY = y + 1 + (rand() % (length - 2) - length / 2);
        tips.push_back(twig(x - (int)radius, branchY, branchZ, direction, (int)radius));
    }
    forEachColumn(
        m_zone, x - (int)radius - 1, x + (int)radius + 2, y - length / 2, y + length / 2, [&](int blockX, int blockY) {
            for (int blockZ = z - (int)radius - 1; blockZ < z + (int)radius + 2; ++blockZ) {
                float transverse = (float)(blockX - x) - 0.5f;
                float vertical = (float)(blockZ - z) - 0.5f;
                float distanceSquared = vertical * vertical + transverse * transverse;
                if (distanceSquared > radius * radius)
                    continue;
                if (distanceSquared < (radius - 1.0f) * (radius - 1.0f)) {
                    if (hollow) {
                        setBlock(blockX, blockY, blockZ, Block(plasma::Vector<3, unsigned char>(0, 0, 0), BlockAir));
                    } else {
                        float brightness = (float)((int)sqrtf(distanceSquared) % 2) * 0.2f + 1.3f;
                        Block block(BlockProperties<float>(colorAt(blockX, blockY, blockZ) * brightness, BlockTree));
                        setBlock(blockX, blockY, blockZ, block);
                    }
                } else {
                    Block block(BlockProperties<float>(colorAt(blockX, blockY, blockZ), BlockTree));
                    setBlock(blockX, blockY, blockZ, block);
                }
            }
        });
    float enclave = sample.enclave(area.type, area.influence);
    float undead = sample.undead(area.type, area.influence);
    plasma::Vector<3, float> foliage, foliageTop;
    m_zone->m_world->harvestColors(x, y, foliage, foliageTop, temperature, humidity, undead, enclave);
    for (std::list<plasma::Vector<3, int>>::const_iterator tip = tips.begin(); tip != tips.end(); ++tip) {
        float leafRadius = (float)rand() / (float)RAND_MAX + 1.0f;
        setGradient((int)((float)(*tip)[2] - leafRadius), (int)((float)(*tip)[2] + leafRadius), foliage, foliageTop);
        ovalLeaves((*tip)[0], (*tip)[1], (*tip)[2], leafRadius, leafRadius, false);
    }
}

void LandmarkBuilder::radialPalm(int x, int y, int z, int width, int height)
{
    plasma::Vector<3, float> foliage, foliageTop;
    {
        World::BiomeSample sample;
        m_zone->m_world->biomeSample(x, y, sample);
        World::AreaInfo area;
        m_zone->m_world->fillAreaInfo(x, y, area);
        float influence = area.influence;
        int areaType = area.type;
        float temperature = sample.temperature(areaType, influence);
        float humidity = sample.humidity();
        // A palm: 20% taller, a brown trunk (10% of the width) bending a quarter of the width and tapering by 75%,
        // every second pair of levels brighter (85%, 100%), and five to seven fronds of half the width, five blocks
        // wide at the base, drooping by a cubic curve.
        height = (int)((float)height * 1.2f);
        float enclave = sample.enclave(areaType, influence);
        float undead = sample.undead(areaType, influence);
        m_zone->m_world->harvestColors(x, y, foliage, foliageTop, temperature, humidity, undead, enclave);
    }
    setGradient(z, z + height, plasma::Vector<3, float>(200, 150, 80), plasma::Vector<3, float>(200, 150, 80));
    float radius = (std::max)(1.0f, (float)width * 0.1f);
    float angle = (float)((double)rand() * TwoPi / (double)RAND_MAX);
    float bendY = sinf(angle);
    float bendX = cosf(angle);
    float bend = (float)rand() / (float)RAND_MAX;
    bendX *= bend;
    bendY = bend * bendY;
    plasma::Vector<3, Fixed> apex;
    for (int level = -3; level < height; ++level) {
        float progress = std::max(0.0f, (float)level / (float)(height - 1));
        float layerRadius = radius - progress * 0.75f * radius;
        if (layerRadius < 1.0f)
            layerRadius = 1.0f;
        float remaining = 1.0f - progress;
        float displacement = (1.0f - remaining * remaining) * 0.25f * (float)width;
        long long centerX =
            ((long long)x << FixedFractionBits) + (long long)(bendX * displacement * FixedUnitsPerBlock);
        long long centerY =
            ((long long)y << FixedFractionBits) + (long long)(bendY * displacement * FixedUnitsPerBlock);
        long long radiusDots = (long long)(layerRadius * FixedUnitsPerBlock);
        forEachColumn(m_zone, (int)((centerX - radiusDots - FixedUnitsPerBlock) / FixedUnitsPerBlock),
                      (int)((centerX + radiusDots + 2 * FixedUnitsPerBlock) / FixedUnitsPerBlock),
                      (int)((centerY - radiusDots - FixedUnitsPerBlock) / FixedUnitsPerBlock),
                      (int)((centerY + radiusDots + 2 * FixedUnitsPerBlock) / FixedUnitsPerBlock),
                      [&](int blockX, int blockY) {
                          float dx = (float)(Fixed(blockX) - Fixed(centerX) + Fixed(0.5f));
                          float dy = (float)(Fixed(blockY) - Fixed(centerY) + Fixed(0.5f));
                          if (dy * dy + dx * dx > layerRadius * layerRadius)
                              return;
                          // Every second pair of levels is brighter.
                          Block block(BlockProperties<float>(colorAt(blockX, blockY, z + level) *
                                                                 ((float)((level / 2 % 2 + 2) % 2) * 0.15f + 0.85f),
                                                             BlockTree));
                          setBlock(blockX, blockY, z + level, block);
                      });
        apex = plasma::Vector<3, Fixed>(Fixed((int)(centerX / FixedUnitsPerBlock)),
                                        Fixed((int)(centerY / FixedUnitsPerBlock)), Fixed(z + level));
    }
    int frondCount = rand() % 3 + 5;
    setGradient(z + height / 2, z + height, foliage, foliageTop);
    for (int frond = 0; frond < frondCount; ++frond) {
        float direction = (float)((double)((float)frond + (float)frond) * Pi / (double)frondCount + (double)angle);
        float dx = cosf(direction);
        float dy = sinf(direction);
        int drop = rand() % (height / 2) + height / 2;
        int length = width / 2;
        for (int step = 0; step < length; ++step) {
            float progress = (float)step / (float)length;
            float curve = progress * progress;
            curve = progress * curve;
            float nextProgress = (float)(step + 1) / (float)length;
            Fixed centerX = apex[0] + Fixed(dx * (float)step);
            Fixed centerY = apex[1] + Fixed(dy * (float)step);
            int upperZ = (int)(apex[2] - Fixed((float)drop * curve * 0.5f) + Fixed(1));
            int lowerZ =
                (int)(apex[2] - Fixed(nextProgress * nextProgress * nextProgress * (float)drop * 0.5f) + Fixed(1));
            float span = (1.0f - curve) * 5.0f;
            for (int blockZ = lowerZ; blockZ <= upperZ; ++blockZ) {
                for (int cross = (int)(span * -0.5f); (float)cross <= span * 0.5f; ++cross) {
                    int blockX = (int)(centerX - Fixed(dy * (float)cross));
                    int blockY = (int)(centerY + Fixed(dx * (float)cross));
                    if (m_zone->getBlock(blockX, blockY, blockZ).isOpen()) {
                        Block leaf(BlockProperties<float>(colorAt(blockX, blockY, blockZ), BlockLeaves));
                        setBlock(blockX, blockY, blockZ, leaf);
                    }
                }
            }
        }
    }
}

plasma::Vector<3, float> randomWoodColor()
{
    // A warm brown: (260, 160..179, 60) plus up to (40, 40, 30) of a random brightness.
    float brightness = (float)rand() / (float)RAND_MAX;
    plasma::Vector<3, float> variation(40.0f * brightness, 40.0f * brightness, 30.0f * brightness);
    plasma::Vector<3, float> color(260, (float)(rand() % 20 + 160), 60);
    color += variation;
    return color;
}

void LandmarkBuilder::setBlock(int x, int y, int z, const Block &block)
{
    Block previous = m_zone->getBlock(x, y, z);
    if (!(previous.flags & BlockFlagProtected))
        m_zone->setBlock(x, y, z, block);
}

void LandmarkBuilder::sphere(int x, int y, int z, float radius)
{
    forEachColumn(m_zone, x - (int)radius - 1, x + (int)radius + 2, y - (int)radius - 1, y + (int)radius + 2,
                  [&](int blockX, int blockY) {
                      for (int dz = (int)(-radius - 1.0f); (float)dz <= radius + 1.0f; ++dz) {
                          float dx = (float)(blockX - x) + 0.5f;
                          float dy = (float)(blockY - y) + 0.5f;
                          float height = (float)dz + 0.5f;
                          if (!(dy * dy + dx * dx + height * height > radius * radius)) {
                              Block block(BlockProperties<float>(colorAt(blockX, blockY, z + dz), BlockTree));
                              setBlock(blockX, blockY, z + dz, block);
                          }
                      }
                  });
}

void LandmarkBuilder::stump(int x, int y, int z, int width, int height)
{
    World::BiomeSample sample;
    m_zone->m_world->biomeSample(x, y, sample);
    World::AreaInfo area;
    m_zone->m_world->fillAreaInfo(x, y, area);
    float influence = area.influence;
    int areaType = area.type;
    float temperature = sample.temperature(areaType, influence);
    float humidity = sample.humidity();
    plasma::Vector<3, float> wood = randomWoodColor();
    float enclave = sample.enclave(areaType, influence);
    float undead = sample.undead(areaType, influence);
    plasma::Vector<3, float> foliage, foliageTop;
    m_zone->m_world->harvestColors(x, y, foliage, foliageTop, temperature, humidity, undead, enclave);
    // A stump: the wood tinted violet (150, 100, 200) in an enclave, its top a quarter wood and three quarters foliage
    // at half brightness; four roots (75% of the radius, 10% of the width) and a trunk one radius high.
    wood += (plasma::Vector<3, float>(150.0f, 100.0f, 200.0f) - wood) * enclave;
    plasma::Vector<3, float> top = (wood * 0.25f + foliage * 0.75f) * 0.5f;
    setGradient(z, z + height, wood, top);
    float radius = (std::max)(1.0f, (float)width * 0.1f);
    float rootRadius = (std::max)(1.0f, radius * 0.75f);

    int rootLength = (int)(radius + radius);
    buildBranch(x, y, z + 1, plasma::Vector<3, float>(1, 0, -1), rootRadius, rootRadius, rootLength);
    buildBranch(x, y, z + 1, plasma::Vector<3, float>(-1, 0, -1), rootRadius, rootRadius, rootLength);
    buildBranch(x, y, z + 1, plasma::Vector<3, float>(0, 1, -1), rootRadius, rootRadius, rootLength);
    buildBranch(x, y, z + 1, plasma::Vector<3, float>(0, -1, -1), rootRadius, rootRadius, rootLength);
    trunk(x, y, z, radius, radius, (int)(radius + 1.0f));
}

void LandmarkBuilder::stumpFlare(int x, int y, int z, float firstRadius, float lastRadius, int height)
{
    for (int level = 0; level < height; ++level) {
        // Height 1 causes division by zero.
        float remaining = 1.0f - (float)level / (float)(height - 1);
        float progress = 1.0f - remaining * remaining * remaining;
        float radius = (1.0f - progress) * firstRadius + progress * lastRadius;
        for (int dx = (int)-radius; (float)dx <= radius; ++dx) {
            for (int dy = (int)-radius; (float)dy <= radius; ++dy) {
                if (!((float)dy * (float)dy + (float)dx * (float)dx > radius * radius)) {
                    Block block(BlockProperties<float>(colorAt(x + dx, y + dy, z + level), BlockTree));
                    Block previous = m_zone->getBlock(x + dx, y + dy, z + level);
                    if (!(previous.flags & BlockFlagProtected))
                        m_zone->setBlock(x + dx, y + dy, z + level, block);
                }
            }
        }
    }
}

// A trunk of discs from firstRadius to lastRadius (at least one block), the growth rings inside the bark alternating
// 130% and 150% of the colour.
void LandmarkBuilder::trunk(int x, int y, int z, float firstRadius, float lastRadius, int height)
{
    for (int level = 0; level < height; ++level) {
        // Height 1 causes division by zero.
        float progress = (float)level / (float)(height - 1);
        float radius = (1.0f - progress) * firstRadius + progress * lastRadius;
        if (radius < 1.0f)
            radius = 1.0f;
        forEachColumn(
            m_zone, x - (int)radius - 1, x + (int)radius + 2, y - (int)radius - 1, y + (int)radius + 2,
            [&](int blockX, int blockY) {
                float dx = (float)(blockX - x) + 0.5f;
                float dy = (float)(blockY - y) + 0.5f;
                float distanceSquared = dy * dy + dx * dx;
                if (!(distanceSquared > radius * radius)) {
                    if (distanceSquared < (radius - 1.0f) * (radius - 1.0f)) {
                        float brightness = (float)((int)sqrtf(distanceSquared) % 2) * 0.2f + 1.3f;
                        setBlock(
                            blockX, blockY, z + level,
                            Block(BlockProperties<float>(colorAt(blockX, blockY, z + level) * brightness, BlockTree)));
                    } else {
                        setBlock(blockX, blockY, z + level,
                                 Block(BlockProperties<float>(colorAt(blockX, blockY, z + level), BlockTree)));
                    }
                }
            });
    }
}

plasma::Vector<3, int> LandmarkBuilder::twig(int x, int y, int z, const plasma::Vector<3, float> &direction, int count)
{
    plasma::Vector<3, Fixed> position = plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z));
    for (int step = 0; step < count; ++step) {
        float progress = (float)step / (float)(count - 1);
        Block block(BlockProperties<float>(colorAt(x, y, z), BlockTree));
        setBlock((int)position[0], (int)position[1], (int)position[2], block);
        position[0] += Fixed(direction[0]);
        position[1] += Fixed(direction[1]);
        position[2] += Fixed(progress * direction[2]);
    }
    return fixedToBlock(position);
}

}
