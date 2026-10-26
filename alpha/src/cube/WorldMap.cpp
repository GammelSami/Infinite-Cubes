// WorldMap caches and draws zone and landscape tiles.
#include "cube/WorldMap.h"

#include <math.h>
#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <zlib.h>
#undef compress

#include "cube/BytesIO.h"
#include "cube/Creature.h"
#include "cube/CreatureIds.h"
#include "cube/CubeShader.h"
#include "cube/Sprite.h"
#include "cube/SpriteIds.h"
#include "cube/WorldBlocks.h"

namespace cube {

// Loads the marker and tile sprites; the grids start empty.
namespace {
const int MarkerSpriteExtent = 4;
enum {
    MapTilePlains = 0
};
enum {
    MapTileVillage = 1
};
enum {
    MapTileForest = 2
};
enum {
    MapTileMountains = 3
};
enum {
    MapTileHills = 4
};
}

WorldMap::WorldMap(IDirect3DDevice9 *device, World *world, CubeShader *shader)
    : m_hoveredZone(-1, -1),
      m_selectedZone(-1, -1),
      m_canTeleport(false),
      m_device(device),
      m_shader(shader),
      m_world(world),
      m_clock(0),
      m_discovered(0),
      m_formatVersion(1)
{
    m_markerSprite = new Sprite(device, false);
    // Marker voxels use a dark gray border and a white center.
    m_markerSprite->resize(MarkerSpriteExtent, MarkerSpriteExtent, MarkerSpriteExtent);
    for (int x = 0; x < MarkerSpriteExtent; x++) {
        for (int y = 0; y < MarkerSpriteExtent; y++) {
            if (x >= 1 && y >= 1 && x <= 2 && y <= 2)
                m_markerSprite->voxel(x, y, 0) = plasma::Vector<3, unsigned char>(255, 255, 255);
            else
                m_markerSprite->voxel(x, y, 0) = plasma::Vector<3, unsigned char>(10, 10, 10);
        }
    }
    m_markerSprite->updateMesh();

    for (int x = 0; x < 1024; x++) {
        for (int y = 0; y < 1024; y++) {
            m_regions[x][y] = 0;
            m_landscape[x][y] = 0;
        }
    }

    m_skullSprite = new Sprite(device, false);
    m_skullSprite->load("skull.cub", 0, true);

    m_tileSprites.push_back(new Sprite(device, false));
    m_tileSprites[MapTilePlains]->load("map-tile-plains.cub", 0, true);
    m_tileSprites.push_back(new Sprite(device, false));
    m_tileSprites[MapTileVillage]->load("map-tile-village.cub", 0, true);
    m_tileSprites.push_back(new Sprite(device, false));
    m_tileSprites[MapTileForest]->load("map-tile-forest.cub", 0, true);
    m_tileSprites.push_back(new Sprite(device, false));
    m_tileSprites[MapTileMountains]->load("map-tile-mountains.cub", 0, true);
    m_tileSprites.push_back(new Sprite(device, false));
    m_tileSprites[MapTileHills]->load("map-tile-hills.cub", 0, true);

    InitializeCriticalSectionAndSpinCount(&m_zoneLock, 0x80000400);
    InitializeCriticalSectionAndSpinCount(&m_regionLock, 0x80000400);
}

ZoneTile::ZoneTile()
    : m_baseZ(0),
      m_sprite(0),
      unused0c(false),
      m_needsRebuild(false),
      m_appearanceTime(0),
      m_flags(0)
{}

WorldMap::~WorldMap()
{
    for (std::vector<Sprite *>::iterator it = m_tileSprites.begin(); it != m_tileSprites.end(); ++it)
        delete *it;
    clear();
    DeleteCriticalSection(&m_zoneLock);
    DeleteCriticalSection(&m_regionLock);
}

ZoneTile::~ZoneTile()
{
    delete m_sprite;
}

inline void WorldMap::unloadLandscape(int x, int y)
{
    if (x < 0 || y < 0 || x >= 1024 || y >= 1024)
        return;
    LandscapeTile *tile = m_landscape[x][y];
    if (!tile)
        return;
    saveLandscape(x, y);
    EnterCriticalSection(&m_regionLock);
    EnterCriticalSection(&m_zoneLock);
    m_landscape[x][y] = 0;
    LeaveCriticalSection(&m_zoneLock);
    LeaveCriticalSection(&m_regionLock);
    delete tile;
}

// Opens "Save/map_<world name>.db" for a named world and reads the discovered count from it.
void WorldMap::open()
{
    clear();
    if (!m_world->getName().empty()) {
        std::stringstream path;
        path << "Save/map_" << m_world->getName() << ".db";
        m_database.open(path.str());
        BytesIO data;
        if (m_database.read("discovered", data))
            data.read(&m_discovered, sizeof(m_discovered));
    }
}

namespace {
const int LoadedZoneRetentionRadius = 10;
const int LoadedLandscapeRetentionRadius = 8;
}

void WorldMap::cleanUp(int x, int y)
{
    for (int i = 0; i < 1024; i++) {
        for (int j = 0; j < 1024; j++) {
            MapRegion *region = m_regions[i][j];
            if (!region)
                continue;
            for (int tileX = 0; tileX < 64; tileX++) {
                for (int tileY = 0; tileY < 64; tileY++) {
                    // && binds first: a tile without a model is still unloaded when far in y (unloadZone ignores it).
                    if (region->m_tiles[tileX][tileY].m_sprite && abs(i * 64 + tileX - x) > LoadedZoneRetentionRadius ||
                        abs(j * 64 + tileY - y) > LoadedZoneRetentionRadius)
                        unloadZone(i * 64 + tileX, j * 64 + tileY);
                }
            }
        }
    }
    for (int i = 0; i < 1024; i++) {
        for (int j = 0; j < 1024; j++) {
            if (!m_landscape[i][j])
                continue;
            if (abs(i - x / 64) > LoadedLandscapeRetentionRadius || abs(j - y / 64) > LoadedLandscapeRetentionRadius)
                unloadRegion(i, j);
            if (abs(i - x / 64) > LoadedLandscapeRetentionRadius || abs(j - y / 64) > LoadedLandscapeRetentionRadius)
                unloadLandscape(i, j);
        }
    }
}

// The buffer is sized for the worst case (1.1 times the input plus 12 bytes), then cut to the
// deflated size.
void compress(const std::vector<char> &data, std::vector<char> &result)
{
    uLong dataSize = (uLong)data.size();
    uLongf size = (uLongf)(dataSize * 1.1) + 12;
    result.resize(size);
    ::z_compress((Bytef *)&result[0], &size, (const Bytef *)&data[0], dataSize);
    result.resize(size);
}

void WorldMap::discover(int x, int y)
{
    EnterCriticalSection(&m_regionLock);
    ZoneTile *tile = getZoneTile(x, y);
    if (tile && !(tile->m_flags & (1 << ZoneTileDiscovered))) {
        tile->m_flags |= (1 << ZoneTileDiscovered);
        m_discovered++;
    }
    LeaveCriticalSection(&m_regionLock);
}

// A creature marker with view depth, sorted farthest first.
struct SignedCreature {
    Creature *creature;
    float depth;
};

struct SignedCreatureDepthLess {
    bool operator()(const SignedCreature &a, const SignedCreature &b) const { return a.depth < b.depth; }
};

// The map in three parts. The terrain: below a zoom of 0.35 the landscape tiles (one per region,
// 32 map units = 256 blocks per unit), otherwise the zone tiles' models (32 units each, one unit =
// 8 blocks) rising into place as they are built (ZoneTile::m_appearanceTime counts down from 250),
// the undiscovered ones darker, and the zones' markers. The full map adds placeholder tiles
// (SpriteMaptile) under missing models, the region's mission and city signs. Then the creatures
// within radius zones: players by their head and hair models, enemies of a mission as skulls,
// vendors (when zoomed in) by their shop signs, each first as a black silhouette.
namespace {
const int PlaceholderHeightOffset = 100;
const int MinimapScissorOuterEdge = 320;
const int MinimapScissorInset = 20;
const int WorldCenterBlockCoordinate = 0x800000;
}

void WorldMap::render(const plasma::Vector<3, Fixed> &center, int centerX, int centerY, int width, int height,
                      const plasma::Vector<3, float> &rotation, float zoom, int milliseconds, int radius, bool full)
{
    m_clock += milliseconds;

    // A 45 degree perspective moved to (centerX, centerY) of the window.
    float focal = 1.0f / tan(0.39269909f);
    plasma::Matrix<float> projection;
    projection.m_elements[0] = -(focal / ((float)width / (float)height));
    projection.m_elements[1] = 0.0f;
    projection.m_elements[2] = 0.0f;
    projection.m_elements[3] = 0.0f;
    projection.m_elements[4] = 0.0f;
    projection.m_elements[5] = focal;
    projection.m_elements[6] = 0.0f;
    projection.m_elements[7] = 0.0f;
    projection.m_elements[8] = 0.0f;
    projection.m_elements[9] = 0.0f;
    // Perspective depth coefficients: z maps the 0.1 near plane and 1000 far plane into [0, 1].
    projection.m_elements[10] = 1.0001f;
    projection.m_elements[11] = 1.0f;
    projection.m_elements[12] = 0.0f;
    projection.m_elements[13] = 0.0f;
    projection.m_elements[14] = -0.10001f;
    projection.m_elements[15] = 0.0f;
    plasma::Matrix<float> placement;
    placement.identity();
    placement.translate((centerX - width * 0.5f) / width * 2.0f, (centerY - height * 0.5f) / height * 2.0f, -0.1f);
    projection = placement * projection;
    // The markers and signs are drawn 80 units nearer, over the terrain.
    plasma::Matrix<float> signProjection = m_projection;
    signProjection[14] -= 80.0f;

    plasma::Matrix<float> view;
    view.identity();
    view.translate(0.0f, 0.0f, 300.0f);
    view.scale(zoom, zoom, zoom);
    view.rotateZ(0.0f);
    view.rotateX(rotation[0]);
    view.rotateZ(rotation[2]);

    RECT scissor = {width - MinimapScissorOuterEdge, MinimapScissorOuterEdge, width - MinimapScissorInset,
                    MinimapScissorInset};
    m_device->SetScissorRect(&scissor);
    m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
    m_shader->apply();
    m_shader->setFogScale(1000000.0f);
    plasma::Vector<4, float> lightPositions[16];
    plasma::Vector<3, float> lightColors[16];
    for (int i = 0; i < 16; i++) {
        lightPositions[i] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
        lightColors[i] = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    }
    m_shader->setPointLights(lightPositions, lightColors);

    if (0.35f > zoom) {
        // The landscape tiles of the regions around the center's region.
        int regionX = (int)center[0] / 0x4000;
        if (center[0] < 0)
            regionX--;
        int regionY = (int)center[1] / 0x4000;
        if (center[1] < 0)
            regionY--;
        plasma::Vector<2, int> region = m_world->findNearestRegion((int)center[0], (int)center[1]);
        if (region[0] >= 0) {
            regionX = region[0];
            regionY = region[1];
        }
        // The result is not used.
        m_world->nearestRegionRecord((int)m_world->m_localPlayer->m_entityData.position[0],
                                     (int)m_world->m_localPlayer->m_entityData.position[1]);
        for (int i = -radius; i <= radius; i++) {
            for (int j = -radius; j <= radius; j++) {
                int x = regionX + i;
                int y = regionY + j;
                if (x < 0 || y < 0 || x >= 1024 || y >= 1024)
                    continue;
                LandscapeTile *tile = m_landscape[x][y];
                if (!tile || !tile->m_sprite || !tile->m_discovered)
                    continue;
                EnterCriticalSection(&m_zoneLock);
                if (tile->m_sprite) {
                    plasma::Matrix<float> model;
                    model.identity();
                    model.scale(32.0f, 32.0f, 32.0f);
                    model.translate(0.0f, 0.0f, -1.0f);
                    model.translate((float)scaled(Fixed(tile->m_origin[0] * 256) - center[0], 1.0 / 256),
                                    (float)scaled(Fixed(tile->m_origin[1] * 256) - center[1], 1.0 / 256));
                    if (tile->m_info.height < 0)
                        model.translate(0.0f, 0.0f, -0.5f);
                    else
                        model.scale(1.0f, 1.0f, tile->m_info.height / 256.0f + 1.0f);
                    // Blue, in four shades by the region's place.
                    plasma::Vector<4, float> color = plasma::Vector<4, float>(0.2f, 0.4f, 1.0f, 1.0f);
                    float shade = (x + y * 2) % 4 * 0.1f + 0.8f;
                    for (int k = 0; k < 3; k++)
                        color[k] *= shade;
                    for (int k = 0; k < 3; k++) {
                        if (color[k] > 1.0f)
                            color[k] = 1.0f;
                    }
                    m_shader->setMaterialColor(color);
                    m_shader->setMatrices(model, view, projection);
                    tile->m_sprite->draw();
                    m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                }
                LeaveCriticalSection(&m_zoneLock);
            }
        }
    } else {
        // The zone tiles around the center's zone; without a radius, its zone and the nearer neighbour on
        // each axis.
        int zoneX = (int)center[0] / 256;
        if (center[0] < 0)
            zoneX--;
        int zoneY = (int)center[1] / 256;
        if (center[1] < 0)
            zoneY--;
        int minimumI = -radius;
        int maximumI = radius;
        int minimumJ = -radius;
        int maximumJ = radius;
        if (radius == 0) {
            if ((int)center[0] % 256 < 128)
                minimumI--;
            else
                maximumI = 1;
            if ((int)center[1] % 256 < 128)
                minimumJ--;
            else
                maximumJ = 1;
        }
        for (int i = minimumI; i <= maximumI; i++) {
            for (int j = minimumJ; j <= maximumJ; j++) {
                int x = zoneX + i;
                int y = zoneY + j;
                if (x < 0 || y < 0 || x >= 0x10000 || y >= 0x10000)
                    continue;
                EnterCriticalSection(&m_zoneLock);
                ZoneTile *tile = getZoneTile(x, y);
                unsigned char flags = 0;
                if (tile)
                    flags = tile->m_flags;
                Region::Record *record = m_world->getRecord(x / 8, y / 8);
                LandscapeTile *landscape = nearestLandscape(x * 256, y * 256);

                // A flat tile at the landscape's height under a zone whose model is missing or still rising.
                if (full && (!tile || !tile->m_sprite || tile->m_appearanceTime)) {
                    Sprite *placeholder = m_world->m_spriteManager.sprite(SpriteMaptile);
                    float z = 0.0f;
                    if (landscape)
                        z = (landscape->m_info.height - PlaceholderHeightOffset) * 0.125f;
                    if (tile && tile->m_sprite)
                        z += (tile->m_appearanceTime / 250.0f - 1.0f) * 20.0f;
                    // Fallback zone palette: absent, discovered and undiscovered tile colors.
                    plasma::Vector<4, float> color(0.2f, 0.3f, 1.0f, 1.0f);
                    if (flags) {
                        if (flags & (1 << ZoneTileDiscovered))
                            color = plasma::Vector<4, float>(0.2f, 0.4f, 1.0f, 1.0f);
                        else
                            color = plasma::Vector<4, float>(0.2f, 0.35f, 1.0f, 1.0f);
                    }
                    plasma::Matrix<float> model;
                    model.identity();
                    // The center's place in its zone, in map units.
                    plasma::Vector<3, int> zone(center * Fixed(1.0 / 256));
                    plasma::Vector<3, Fixed> offset =
                        center * scaled(Fixed(32), 1.0 / 256) - plasma::Vector<3, Fixed>(zone) * Fixed(32);
                    offset *= Fixed(-1);
                    model.translate((float)offset[0], (float)offset[1]);
                    model.translate((float)(i * 32), (float)(j * 32), (float)(Fixed(z) - center[2] * 0.125f));
                    model.scale(4.0f, 4.0f, 4.0f);
                    m_shader->setMatrices(model, view, projection);
                    m_shader->setMaterialColor(color);
                    placeholder->draw();
                    m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                }

                // The zone's model, rising by a tenth of a unit per millisecond left; an undiscovered zone darker
                // unless it is the target of a mission under way.
                if (tile && tile->m_sprite) {
                    float z = tile->m_baseZ - tile->m_appearanceTime * 0.1f;
                    tile->m_appearanceTime -= milliseconds;
                    if (tile->m_appearanceTime < 0)
                        tile->m_appearanceTime = 0;
                    plasma::Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
                    plasma::Matrix<float> model;
                    model.identity();
                    plasma::Vector<3, int> zone(center * Fixed(1.0 / 256));
                    plasma::Vector<3, Fixed> offset =
                        center * scaled(Fixed(32), 1.0 / 256) - plasma::Vector<3, Fixed>(zone) * Fixed(32);
                    offset *= Fixed(-1);
                    model.translate((float)offset[0], (float)offset[1]);
                    model.translate((float)(i * 32), (float)(j * 32), (float)(Fixed(z) - center[2] * 0.125f));
                    if (!tile->m_sprite)
                        model.scale(4.0f, 4.0f, 4.0f);
                    m_shader->setMatrices(model, view, projection);
                    if (!(record && record->mission.kind && record->mission.progress == MissionInProgress &&
                          record->mission.targetZone == plasma::Vector<2, int>(x, y)) &&
                        full && !(flags & (1 << ZoneTileDiscovered))) {
                        for (int k = 0; k < 3; k++)
                            color[k] *= 0.6f;
                    }
                    m_shader->setMaterialColor(color);
                    tile->m_sprite->draw();
                    m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                }

                if (m_markerSprite && tile) {
                    for (std::list<plasma::Vector<3, int>>::iterator it = tile->m_markers.begin();
                         it != tile->m_markers.end(); ++it) {
                        plasma::Matrix<float> model;
                        model.identity();
                        model.translate(plasma::Vector<3, float>(*it - plasma::Vector<3, int>(center)));
                        model.scale(5.0f, 5.0f, 5.0f);
                        m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                        m_shader->setMatrices(model, m_view, signProjection);
                        m_markerSprite->draw();
                    }
                }
                LeaveCriticalSection(&m_zoneLock);
            }
        }
    }

    if (full) {
        // The missions under way in the center's region, turning, and the cities more than 512 blocks from
        // the player, 100 blocks over the ground; at least 2 and at most 10 times their size.
        Region *region = m_world->regionAtColumn((int)center[0], (int)center[1]);
        if (region) {
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    Region::Record &record = region->m_records[i * 8 + j];
                    if (record.mission.kind && record.mission.progress != MissionCompleted) {
                        Sprite *sign = m_world->m_spriteManager.sprite(SpriteMission);
                        plasma::Matrix<float> model;
                        model.identity();
                        if (record.mission.kind == MissionMonster)
                            model.translate((float)(Fixed(record.mission.targetZone[0] * 256 + 128) - center[0]),
                                            (float)(Fixed(record.mission.targetZone[1] * 256 + 128) - center[1]),
                                            (float)(Fixed(record.height) - center[2] + 100));
                        else
                            model.translate((float)(record.center[0] - center[0]),
                                            (float)(record.center[1] - center[1]),
                                            (float)(Fixed(record.height) - center[2] + 100));
                        model.rotateZ(m_clock * 0.02f);
                        float scale = 8.0f / zoom;
                        if (2.0f > scale)
                            scale = 2.0f;
                        else if (scale > 10.0f)
                            scale = 10.0f;
                        model.scale(scale, scale, scale);
                        model.translate(-sign->sizeX() * 0.5f, -sign->sizeY() * 0.5f, 0.0f);
                        m_shader->setMatrices(model, m_view, projection);
                        m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                        sign->draw();
                    }
                    if (record.kind == Region::Record::SceneryCity) {
                        const plasma::Vector<3, Fixed> &position = m_world->m_localPlayer->m_entityData.position;
                        plasma::Vector<2, float> distance(plasma::Vector<2, Fixed>(position[0], position[1]) -
                                                          record.center);
                        if (distance[0] * distance[0] + distance[1] * distance[1] > 262144.0f) {
                            Sprite *sign = m_world->m_spriteManager.sprite(SpriteCity);
                            plasma::Matrix<float> model;
                            model.identity();
                            model.translate((float)(record.center[0] - center[0]),
                                            (float)(record.center[1] - center[1]),
                                            (float)(Fixed(record.height) - center[2] + 100));
                            float scale = 8.0f / zoom;
                            if (2.0f > scale)
                                scale = 2.0f;
                            else if (scale > 10.0f)
                                scale = 10.0f;
                            model.scale(scale, scale, scale);
                            model.translate(-sign->sizeX() * 0.5f, -sign->sizeY() * 0.5f, 0.0f);
                            m_shader->setMatrices(model, m_view, projection);
                            m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                            sign->draw();
                        }
                    }
                }
            }
        }
    }
    m_device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    m_projection = projection;
    m_view = view;
    m_view.scale(0.125f, 0.125f, 0.125f);

    // The creatures to sign: players, enemies of a mission, and (zoomed in) vendors, within radius
    // zones (at least one).
    m_shader->apply();
    m_shader->setFogScale(1.0e9f);
    std::vector<SignedCreature> creatures;
    for (std::map<long long, Creature *>::iterator it = m_world->m_creatures.begin(); it != m_world->m_creatures.end();
         ++it) {
        Creature *creature = it->second;
        if (!creature)
            continue;
        unsigned char hostility = creature->m_entityData.hostilityType;
        if (hostility != HostilityPlayer &&
            !(hostility == HostilityEnemy &&
              (creature->m_entityData.appearance.appearanceFlags & (1 << AppearanceFlagMissionResident)))) {
            if (zoom < 1.6f)
                continue;
            unsigned char classType = creature->m_entityData.classType;
            if (classType != ClassInnkeeper && classType != ClassItemVendor && classType != ClassArmorVendor &&
                classType != ClassWeaponVendor && classType != ClassAnalyzer && classType != ClassSmith &&
                classType != ClassCarpenter && classType != ClassTailor)
                continue;
        }
        int range = (radius < 1 ? 1 : radius) * 256;
        if (fabs((float)(center[0] - creature->m_entityData.position[0])) > range ||
            fabs((float)(center[1] - creature->m_entityData.position[1])) > range)
            continue;
        plasma::Vector<3, float> point = m_view * plasma::Vector<3, float>(creature->m_entityData.position - center);
        SignedCreature signedCreature = {creature, -point[2]};
        creatures.push_back(signedCreature);
    }
    std::sort(creatures.begin(), creatures.end(), SignedCreatureDepthLess());

    const plasma::Vector<3, Fixed> &position = m_world->m_localPlayer->m_entityData.position;
    plasma::Vector<3, float> away((float)(position[0] - Fixed(WorldCenterBlockCoordinate)),
                                  (float)(position[1] - Fixed(WorldCenterBlockCoordinate)), 0.0f);
    if (away.squaredLength() > 0.0f) {
        away.normalize();
        away *= 300.0f;
    }
    m_awayPoint = plasma::Vector<3, float>((float)(Fixed(away[0]) - center[0] + position[0]),
                                           (float)(Fixed(away[1]) - center[1] + position[1]), 0.0f);
    // Creature-sign lighting: white diffuse, blue ambient, normalized (0, 0.5, 1) direction and gray
    // material.
    m_shader->setLight(
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), plasma::Vector<4, float>(0.2f, 0.3f, 0.4f, 1.0f),
        plasma::Vector<3, float>(0.0f, 0.5f, 1.0f).normalized(), plasma::Vector<4, float>(0.4f, 0.4f, 0.4f, 1.0f));

    SpriteManager &sprites = m_world->m_spriteManager;
    float outlineScale = 5.0f / zoom;
    float signScale = 4.0f / zoom;
    for (std::vector<SignedCreature>::iterator it = creatures.begin(); it != creatures.end(); ++it) {
        // A black silhouette, larger and without depth writes.
        plasma::Matrix<float> model;
        model.identity();
        model.translate(plasma::Vector<3, float>(it->creature->m_drawnPosition - center));
        model.scale(outlineScale, outlineScale, outlineScale);
        m_shader->setMaterialColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
        m_shader->getDevice()->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        if (it->creature->m_entityData.hostilityType == HostilityEnemy) {
            Sprite *skull = sprites.sprite(SpriteSkull);
            if (skull) {
                plasma::Matrix<float> turned = model;
                turned.rotateZ(it->creature->m_drawnRotation[2]);
                turned.translate(-skull->sizeX() * 0.5f, -skull->sizeY() * 0.5f, -skull->sizeZ() * 0.5f);
                m_shader->setMatrices(turned, m_view, signProjection);
                skull->draw();
            }
        } else if (it->creature->m_entityData.hostilityType == HostilityFriendlyNpc) {
            // The vendor's shop sign.
            unsigned char classType = it->creature->m_entityData.classType;
            Sprite *sign = 0;
            if (classType == ClassAnalyzer)
                sign = sprites.sprite(SpriteIdentifierSign);
            if (classType == ClassInnkeeper)
                sign = sprites.sprite(SpriteInnSign);
            if (classType == ClassItemVendor)
                sign = sprites.sprite(SpriteShopSign);
            if (classType == ClassArmorVendor)
                sign = sprites.sprite(SpriteArmorShopSign);
            if (classType == ClassWeaponVendor)
                sign = sprites.sprite(SpriteWeaponShopSign);
            if (classType == ClassSmith)
                sign = sprites.sprite(SpriteSmithySign);
            if (classType == ClassCarpenter)
                sign = sprites.sprite(SpriteCarpentersshopSign);
            if (classType == ClassTailor)
                sign = sprites.sprite(SpriteWeavingmillSign);
            if (sign) {
                plasma::Matrix<float> turned = model;
                turned.scale(0.5f, 0.5f, 0.5f);
                turned.rotateZ(-rotation[2]);
                turned.translate(-sign->sizeX() * 0.5f, -sign->sizeY() * 0.5f, -sign->sizeZ() * 0.5f);
                m_shader->setMatrices(turned, m_view, signProjection);
                sign->draw();
            }
        } else {
            short head = it->creature->m_entityData.appearance.headModel;
            if (sprites.sprite(head)) {
                plasma::Matrix<float> turned = model;
                turned.rotateZ(it->creature->m_drawnRotation[2]);
                turned.translate(-sprites.sprite(head)->sizeX() * 0.5f, -sprites.sprite(head)->sizeY() * 0.5f,
                                 -sprites.sprite(head)->sizeZ() * 0.5f);
                m_shader->setMatrices(turned, m_view, signProjection);
                sprites.sprite(it->creature->m_entityData.appearance.headModel)->draw();
            }
            short hair = it->creature->m_entityData.appearance.hairModel;
            if (sprites.sprite(hair)) {
                plasma::Matrix<float> turned = model;
                turned.rotateZ(it->creature->m_drawnRotation[2]);
                turned.translate(-sprites.sprite(hair)->sizeX() * 0.5f, -sprites.sprite(hair)->sizeY() * 0.5f,
                                 -sprites.sprite(hair)->sizeZ() * 0.5f);
                m_shader->setMatrices(turned, m_view, signProjection);
                sprites.sprite(it->creature->m_entityData.appearance.hairModel)->draw();
            }
        }

        // The sign itself, in its colours.
        model.identity();
        model.translate(plasma::Vector<3, float>(it->creature->m_drawnPosition - center));
        model.scale(signScale, signScale, signScale);
        m_shader->getDevice()->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
        if (it->creature->m_entityData.hostilityType == HostilityEnemy) {
            Sprite *skull = sprites.sprite(SpriteSkull);
            if (skull) {
                plasma::Matrix<float> turned = model;
                turned.rotateZ(it->creature->m_drawnRotation[2]);
                turned.translate(-skull->sizeX() * 0.5f, -skull->sizeY() * 0.5f, -skull->sizeZ() * 0.5f);
                m_shader->setMatrices(turned, m_view, signProjection);
                m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                skull->draw();
            }
        } else if (it->creature->m_entityData.hostilityType == HostilityFriendlyNpc) {
            // The vendor's shop sign.
            unsigned char classType = it->creature->m_entityData.classType;
            Sprite *sign = 0;
            if (classType == ClassAnalyzer)
                sign = sprites.sprite(SpriteIdentifierSign);
            if (classType == ClassInnkeeper)
                sign = sprites.sprite(SpriteInnSign);
            if (classType == ClassItemVendor)
                sign = sprites.sprite(SpriteShopSign);
            if (classType == ClassArmorVendor)
                sign = sprites.sprite(SpriteArmorShopSign);
            if (classType == ClassWeaponVendor)
                sign = sprites.sprite(SpriteWeaponShopSign);
            if (classType == ClassSmith)
                sign = sprites.sprite(SpriteSmithySign);
            if (classType == ClassCarpenter)
                sign = sprites.sprite(SpriteCarpentersshopSign);
            if (classType == ClassTailor)
                sign = sprites.sprite(SpriteWeavingmillSign);
            if (sign) {
                plasma::Matrix<float> turned = model;
                turned.scale(0.5f, 0.5f, 0.5f);
                turned.rotateZ(-rotation[2]);
                turned.translate(-sign->sizeX() * 0.5f, -sign->sizeY() * 0.5f, -sign->sizeZ() * 0.5f);
                m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                m_shader->setMatrices(turned, m_view, signProjection);
                sign->draw();
            }
        } else {
            short head = it->creature->m_entityData.appearance.headModel;
            if (sprites.sprite(head)) {
                plasma::Matrix<float> turned = model;
                turned.rotateZ(it->creature->m_drawnRotation[2]);
                turned.translate(-sprites.sprite(head)->sizeX() * 0.5f, -sprites.sprite(head)->sizeY() * 0.5f,
                                 -sprites.sprite(head)->sizeZ() * 0.5f);
                m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                m_shader->setMatrices(turned, m_view, signProjection);
                sprites.sprite(it->creature->m_entityData.appearance.headModel)->draw();
            }
            short hair = it->creature->m_entityData.appearance.hairModel;
            if (sprites.sprite(hair)) {
                plasma::Matrix<float> turned = model;
                turned.rotateZ(it->creature->m_drawnRotation[2]);
                turned.translate(-sprites.sprite(hair)->sizeX() * 0.5f, -sprites.sprite(hair)->sizeY() * 0.5f,
                                 -sprites.sprite(hair)->sizeZ() * 0.5f);
                plasma::Vector<3, unsigned char> &hairColor = it->creature->m_entityData.appearance.hairColor;
                m_shader->setMaterialColor(plasma::Vector<4, float>(hairColor[0] / 255.0f, hairColor[1] / 255.0f,
                                                                    hairColor[2] / 255.0f, 1.0f));
                m_shader->setMatrices(turned, m_view, signProjection);
                sprites.sprite(it->creature->m_entityData.appearance.hairModel)->draw();
            }
        }
    }
}

void WorldMap::enterRegions()
{
    EnterCriticalSection(&m_regionLock);
}

LandscapeTile *WorldMap::nearestLandscape(int x, int y)
{
    int minX = (x - 0x4000) / 0x4000;
    int minY = (y - 0x4000) / 0x4000;
    int maxX = (x + 0x4000) / 0x4000;
    int maxY = (y + 0x4000) / 0x4000;
    plasma::Vector<2, float> offset = m_world->landscapeOffset(x, y);
    int positionX = (int)(x + offset[0]);
    int positionY = (int)(y + offset[1]);
    LandscapeTile *nearest = 0;
    int nearestDistance = 0;
    for (int i = minX; i <= maxX; i++) {
        for (int j = minY; j <= maxY; j++) {
            if (i < 0 || j < 0 || i >= 1024 || j >= 1024)
                return 0;
            LandscapeTile *tile = m_landscape[i][j];
            if (!tile || !tile->m_sprite)
                return 0;
            int distance = (int)m_world->distanceSquared(tile->m_info.column, positionX, positionY);
            if (!nearest || distance < nearestDistance) {
                nearest = tile;
                nearestDistance = distance;
            }
        }
    }
    return nearest;
}

void WorldMap::leaveRegions()
{
    LeaveCriticalSection(&m_regionLock);
}

// Locks zones while the generation thread accesses them.
void WorldMap::enterZones()
{
    EnterCriticalSection(&m_zoneLock);
}

void WorldMap::leaveZones()
{
    LeaveCriticalSection(&m_zoneLock);
}

void WorldMap::clear()
{
    if (m_database.isOpen()) {
        BytesIO data;
        data.write(&m_discovered, sizeof(m_discovered));
        m_database.write("discovered", data);
    }
    m_discovered = 0;
    for (int x = 0; x < 1024; x++) {
        for (int y = 0; y < 1024; y++) {
            MapRegion *region = m_regions[x][y];
            if (region) {
                saveRegion(x, y);
                delete region;
                m_regions[x][y] = 0;
            }
        }
    }
    for (int x = 0; x < 1024; x++) {
        for (int y = 0; y < 1024; y++) {
            LandscapeTile *tile = m_landscape[x][y];
            if (tile) {
                saveLandscape(x, y);
                delete tile;
                m_landscape[x][y] = 0;
            }
        }
    }
    if (m_database.isOpen())
        m_database.close();
}

void WorldMap::unloadRegion(int x, int y)
{
    if (x < 0 || y < 0 || x >= 1024 || y >= 1024)
        return;
    MapRegion *region = m_regions[x][y];
    if (!region)
        return;
    saveRegion(x, y);
    if (m_database.isOpen()) {
        BytesIO data;
        data.write(&m_discovered, sizeof(m_discovered));
        m_database.write("discovered", data);
    }
    EnterCriticalSection(&m_regionLock);
    EnterCriticalSection(&m_zoneLock);
    m_regions[x][y] = 0;
    LeaveCriticalSection(&m_zoneLock);
    LeaveCriticalSection(&m_regionLock);
    delete region;
}

void WorldMap::unloadZone(int x, int y)
{
    if (x < 0 || y < 0 || x >= 0x10000 || y >= 0x10000)
        return;
    MapRegion *region = m_regions[x / 64][y / 64];
    if (!region)
        return;
    ZoneTile *tile = &region->m_tiles[x % 64][y % 64];
    Sprite *sprite = tile->m_sprite;
    if (!sprite)
        return;
    EnterCriticalSection(&m_regionLock);
    EnterCriticalSection(&m_zoneLock);
    tile->m_sprite = 0;
    tile->m_markers.clear();
    LeaveCriticalSection(&m_zoneLock);
    LeaveCriticalSection(&m_regionLock);
    delete sprite;
}

Region::Record *WorldMap::getRecord(int x, int y)
{
    if (x < 0 || y < 0 || x >= 0x2000 || y >= 0x2000)
        return 0;
    MapRegion *region = m_regions[x * 8 / 64][y * 8 / 64];
    if (!region)
        return 0;
    return &region->m_records[x % 8][y % 8];
}

ZoneTile *WorldMap::getZoneTile(int x, int y)
{
    if (x < 0 || y < 0 || x >= 0x10000 || y >= 0x10000)
        return 0;
    MapRegion *region = m_regions[x / 64][y / 64];
    if (!region)
        return 0;
    return &region->m_tiles[x % 64][y % 64];
}

// Loads the landscape tile (x, y) from the database "land<x>_<y>" when it is not cached, then
// rebuilds its model (when new or marked) from the world: it takes the world's cells and records
// into the loaded map region, and, once the world has the records of the 3 x 3 regions around it,
// it draws every zone nearer to this region's column than to its neighbours' as a voxel: white
// when discovered, light grey when seen, grey otherwise.
void WorldMap::loadLandscape(int x, int y)
{
    if (m_world->getName().empty())
        return;
    if (x < 0 || y < 0 || x >= 1024 || y >= 1024)
        return;
    LandscapeTile *tile = m_landscape[x][y];
    if (tile) {
        if (!tile->m_needsRebuild)
            return;
    } else {
        std::stringstream key;
        key << "land" << x << "_" << y;
        BytesIO data;
        if (m_database.read(key.str(), data)) {
            int version;
            data.read(&version, sizeof(version));
            tile = new LandscapeTile;
            m_landscape[x][y] = tile;
            data.read(&tile->m_origin, sizeof(tile->m_origin));
            data.read(&tile->m_discovered, sizeof(tile->m_discovered));
            plasma::Vector<3, int> size;
            data.read(&size, sizeof(size));
            if (size[0] > 0 && size[1] > 0 && size[2] > 0) {
                int compressedSize;
                data.read(&compressedSize, sizeof(compressedSize));
                if (compressedSize) {
                    std::vector<char> compressed(compressedSize);
                    data.read(&compressed[0], compressedSize);
                    std::vector<char> voxels;
                    decompress(compressed, voxels);
                    tile->m_sprite = new Sprite(m_device, false);
                    tile->m_sprite->resize(size[0], size[1], size[2]);
                    memcpy(tile->m_sprite->m_voxels, &voxels[0], voxels.size());
                    tile->m_sprite->updateMesh();
                }
            }
            data.read(&tile->m_info, sizeof(tile->m_info));
        }
    }

    World::RegionRecord *record = m_world->getRegionRecord(x, y);
    if (!record)
        return;
    MapRegion *region = m_regions[x][y];
    if (region) {
        EnterCriticalSection(&m_zoneLock);
        for (int i = 0; i < 64; i++) {
            for (int j = 0; j < 64; j++) {
                Region::Cell *cell = m_world->getCell(x * 64 + i, y * 64 + j);
                if (cell)
                    region->m_tiles[i][j].m_cell = *cell;
            }
        }
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                Region::Record *regionRecord = m_world->getRecord(x * 64 / 8 + i, y * 64 / 8 + j);
                if (regionRecord)
                    region->m_records[i][j] = *regionRecord;
            }
        }
        LeaveCriticalSection(&m_zoneLock);
    }

    for (int i = x - 1; i <= x + 1; i++) {
        for (int j = y - 1; j <= y + 1; j++) {
            if (!m_world->getRegionRecord(i, j))
                return;
        }
    }
    if (!tile) {
        tile = new LandscapeTile;
        m_landscape[x][y] = tile;
    }
    tile->m_info = *record;

    // The zones of this region and the next one around it that lie nearer to this region's column than
    // to any neighbour's.
    bool empty = true;
    plasma::Vector<2, int> maximum;
    plasma::Vector<2, int> minimum;
    for (int i = x * 64 - 64; i <= (x + 2) * 64; i++) {
        for (int j = y * 64 - 64; j <= (y + 2) * 64; j++) {
            plasma::Vector<2, float> offset = m_world->landscapeOffset(i * 256, j * 256);
            int blockX = (int)(i * 256 + offset[0]);
            int blockY = (int)(j * 256 + offset[1]);
            int distance = (int)m_world->distanceSquared(record->column, blockX, blockY);
            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    if (di == 0 && dj == 0)
                        continue;
                    World::RegionRecord *other = m_world->getRegionRecord(x + di, y + dj);
                    // Bounds require every neighbor to be available.
                    if (!other || distance > m_world->distanceSquared(other->column, blockX, blockY))
                        goto notNearest;
                }
            }
            if (empty) {
                minimum = plasma::Vector<2, int>(i, j);
                maximum = plasma::Vector<2, int>(i, j);
                empty = false;
            } else {
                if (i < minimum[0])
                    minimum[0] = i;
                if (j < minimum[1])
                    minimum[1] = j;
                if (i > maximum[0])
                    maximum[0] = i;
                if (j > maximum[1])
                    maximum[1] = j;
            }
        notNearest:;
        }
    }
    if (empty)
        return;

    tile->m_needsRebuild = 0;
    Sprite *sprite = new Sprite(m_device, false);
    sprite->resize(maximum[0] - minimum[0] + 1, maximum[1] - minimum[1] + 1, 1);
    for (int i = x * 64 - 64; i < (x + 2) * 64; i++) {
        for (int j = y * 64 - 64; j < (y + 2) * 64; j++) {
            plasma::Vector<2, float> offset = m_world->landscapeOffset(i * 256, j * 256);
            int blockX = (int)(i * 256 + offset[0]);
            int blockY = (int)(j * 256 + offset[1]);
            int distance = (int)m_world->distanceSquared(record->column, blockX, blockY);
            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    if (di == 0 && dj == 0)
                        continue;
                    World::RegionRecord *other = m_world->getRegionRecord(x + di, y + dj);
                    // The draw pass compares only available neighbors.
                    if (other && distance > m_world->distanceSquared(other->column, blockX, blockY))
                        goto notDrawn;
                }
            }
            {
                ZoneTile *zoneTile = getZoneTile(i, j);
                if (zoneTile && zoneTile->m_flags) {
                    if (zoneTile->m_flags & (1 << ZoneTileDiscovered))
                        sprite->voxel(i - minimum[0], j - minimum[1], 0) =
                            plasma::Vector<3, unsigned char>(255, 255, 255);
                    else
                        sprite->voxel(i - minimum[0], j - minimum[1], 0) =
                            plasma::Vector<3, unsigned char>(220, 220, 220);
                } else {
                    sprite->voxel(i - minimum[0], j - minimum[1], 0) = plasma::Vector<3, unsigned char>(200, 200, 200);
                }
            }
        notDrawn:;
        }
    }
    sprite->updateMesh();
    EnterCriticalSection(&m_zoneLock);
    tile->m_origin = minimum;
    delete tile->m_sprite;
    tile->m_sprite = sprite;
    LeaveCriticalSection(&m_zoneLock);
    saveLandscape(x, y);
}

void WorldMap::loadRegion(int x, int y)
{
    if (m_world->getName().empty())
        return;
    if (x < 0 || y < 0 || x >= 1024 || y >= 1024)
        return;
    if (m_regions[x][y])
        return;
    MapRegion *region = new MapRegion;
    std::stringstream key;
    key << "reg" << x << "_" << y;
    BytesIO data;
    if (m_database.read(key.str(), data)) {
        int version;
        data.read(&version, sizeof(version));
        for (int i = 0; i < 64; i++) {
            for (int j = 0; j < 64; j++) {
                ZoneTile &tile = region->m_tiles[i][j];
                data.read(&tile.m_flags, sizeof(tile.m_flags));
                data.read(&tile.m_cell.kind, sizeof(tile.m_cell.kind));
                data.read(&tile.m_cell.style, sizeof(tile.m_cell.style));
                data.read(&tile.m_cell.unused02, sizeof(tile.m_cell.unused02));
                data.read(&tile.m_cell.unused03, sizeof(tile.m_cell.unused03));
                data.read(&tile.m_cell.seed, sizeof(tile.m_cell.seed));
                data.read(&tile.m_cell.level, sizeof(tile.m_cell.level));
                data.read(&tile.m_cell.rarity, sizeof(tile.m_cell.rarity));
            }
        }
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                Region::Record &record = region->m_records[i][j];
                data.read(&record.kind, sizeof(record.kind));
                data.read(&record.style, sizeof(record.style));
                data.read(&record.seed, sizeof(record.seed));
                data.read(&record.level, sizeof(record.level));
                data.read(&record.rarity, sizeof(record.rarity));
                data.read(&record.height, sizeof(record.height));
                data.read(&record.center, sizeof(record.center));
                data.read(&record.radius, sizeof(record.radius));
            }
        }
    }
    EnterCriticalSection(&m_zoneLock);
    m_regions[x][y] = region;
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) {
            Region::Cell *cell = m_world->getCell(x * 64 + i, y * 64 + j);
            if (cell)
                region->m_tiles[i][j].m_cell = *cell;
        }
    }
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            // The region's first zone, in records of 8 x 8 zones.
            Region::Record *record = m_world->getRecord(x * 64 / 8 + i, y * 64 / 8 + j);
            if (record)
                region->m_records[i][j] = *record;
        }
    }
    LeaveCriticalSection(&m_zoneLock);
    saveRegion(x, y);
}

// Loads a cached tile, or downsamples the live zone into 8-block voxels.
namespace {
const int MinimumGroundSearchSentinel = 10000000;
const int RegionBorderMarkerSamplingStride = 4;
}

void WorldMap::loadZone(int x, int y)
{
    if (m_world->getName().empty())
        return;
    if (x < 0 || y < 0 || x >= 0x10000 || y >= 0x10000)
        return;
    MapRegion *region = m_regions[x / 64][y / 64];
    if (!region)
        return;
    ZoneTile *tile = &region->m_tiles[x % 64][y % 64];
    Region::Cell *cell = m_world->getCell(x, y);
    if (cell) {
        EnterCriticalSection(&m_zoneLock);
        tile->m_cell = *cell;
        LeaveCriticalSection(&m_zoneLock);
    }
    Region::Record *record = m_world->getRecord(x / 8, y / 8);
    if (record)
        region->m_records[(x / 8) % 8][(y / 8) % 8] = *record;
    Zone *zone = m_world->zone(x, y);
    if (tile->m_sprite && (!tile->m_needsRebuild || !zone))
        return;
    std::stringstream key;
    key << "tile" << x << "_" << y;
    BytesIO data;
    if (m_database.read(key.str(), data) && !tile->m_sprite) {
        int version;
        data.read(&version, sizeof(version));
        data.read(&tile->m_baseZ, sizeof(tile->m_baseZ));
        plasma::Vector<3, int> size;
        data.read(&size, sizeof(size));
        if (size[0] > 0 && size[1] > 0 && size[2] > 0) {
            int compressedSize;
            data.read(&compressedSize, sizeof(compressedSize));
            if (compressedSize) {
                std::vector<char> compressed(compressedSize);
                data.read(&compressed[0], compressedSize);
                std::vector<char> voxels;
                decompress(compressed, voxels);
                Sprite *sprite = new Sprite(m_device, false);
                sprite->resize(size[0], size[1], size[2]);
                memcpy(sprite->m_voxels, &voxels[0], voxels.size());
                sprite->updateMesh();
                EnterCriticalSection(&m_zoneLock);
                tile->m_flags |= (1 << ZoneTileBuilt);
                tile->m_sprite = sprite;
                tile->m_appearanceTime = 250;
                LeaveCriticalSection(&m_zoneLock);
            }
        }
        int markerCount = 0;
        data.read(&markerCount, sizeof(markerCount));
        EnterCriticalSection(&m_zoneLock);
        tile->m_markers.clear();
        LeaveCriticalSection(&m_zoneLock);
        for (int i = 0; i < markerCount; i++) {
            plasma::Vector<3, int> marker;
            data.read(&marker, sizeof(marker));
            EnterCriticalSection(&m_zoneLock);
            tile->m_markers.push_back(marker);
            LeaveCriticalSection(&m_zoneLock);
        }
        if (!tile->m_needsRebuild)
            return;
    }
    if (!zone)
        return;

    int minimum = zone->m_fields[0].m_groundZ;
    int maximum = zone->m_fields[0].m_groundZ;
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 256; j++) {
            Field &field = zone->m_fields[i + j * 256];
            if (field.m_groundZ - 1 < minimum)
                minimum = field.m_groundZ - 1;
            if (field.m_baseZ + field.m_size > maximum)
                maximum = field.m_baseZ + field.m_size;
        }
    }
    if (minimum < 0)
        minimum = 0;
    if (maximum < 0)
        maximum = 0;
    int height = (maximum - minimum) / 8 + 1;
    tile->m_baseZ = minimum / 8;
    tile->m_needsRebuild = false;
    Sprite *sprite = new Sprite(m_device, false);
    sprite->resize(32, 32, height);
    EnterCriticalSection(&m_zoneLock);
    tile->m_markers.clear();
    LeaveCriticalSection(&m_zoneLock);

    // The code accumulates four quadrant colours but does not consume them.
    plasma::Vector<3, float> quadrantColors[2][2];
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            quadrantColors[i][j] = plasma::Vector<3, float>(0, 0, 0);
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            plasma::Vector<3, float> color(0, 0, 0);
            float surfaceZ = 0;
            int lowest = MinimumGroundSearchSentinel;
            for (int dx = 0; dx < 8; dx++) {
                for (int dy = 0; dy < 8; dy++) {
                    Field &field = zone->m_fields[i * 8 + dx + (j * 8 + dy) * 256];
                    if (field.m_groundZ < lowest)
                        lowest = field.m_groundZ;
                    if (field.m_baseZ + field.m_size <= 0) {
                        int type = waterCube.type & ~BlockFlagMask;
                        // Map water and wet blocks use the fixed blue RGB (80, 100, 255).
                        color += type == BlockWater || type == BlockWet ? plasma::Vector<3, float>(80, 100, 255)
                                                                        : plasma::Vector<3, float>(waterCube.color);
                    } else {
                        int z = field.m_size - 1;
                        for (; z >= 0; z--) {
                            if (field.getBlock(z).type & ~BlockFlagMask) {
                                const Block &block = field.getBlock(z);
                                int type = block.type & ~BlockFlagMask;
                                color += type == BlockWater || type == BlockWet ? plasma::Vector<3, float>(80, 100, 255)
                                                                                : plasma::Vector<3, float>(block.color);
                                break;
                            }
                            if (field.m_baseZ + z == field.m_groundZ - 1) {
                                color +=
                                    m_world->rockColor((zone->m_position[0] * 32 + i) * 8 + dx,
                                                       (zone->m_position[1] * 32 + j) * 8 + dy, field.m_groundZ, zone);
                                break;
                            }
                        }
                        if (z < 0) {
                            color += m_world->rockColor((zone->m_position[0] * 32 + i) * 8 + dx,
                                                        (zone->m_position[1] * 32 + j) * 8 + dy, field.m_groundZ, zone);
                        }
                        surfaceZ += field.m_baseZ + z;
                    }
                }
            }
            int top = -tile->m_baseZ - (int)(surfaceZ * (-1.0f / 64)) / 8;
            color *= 1.0f / 64;
            if (i % RegionBorderMarkerSamplingStride == 0 && j % RegionBorderMarkerSamplingStride == 0) {
                World::RegionRecord *nearest = m_world->nearestRegionRecord(x * 256 + i * 8, y * 256 + j * 8);
                if (nearest != m_world->nearestRegionRecord(x * 256 + (i + RegionBorderMarkerSamplingStride) * 8,
                                                            y * 256 + j * 8) ||
                    nearest != m_world->nearestRegionRecord(x * 256 + i * 8,
                                                            y * 256 + (j + RegionBorderMarkerSamplingStride) * 8)) {
                    EnterCriticalSection(&m_zoneLock);
                    int z = zone->m_fields[i * 8 + j * 8 * 256].m_groundZ;
                    if (z < 0)
                        z = 0;
                    plasma::Vector<3, int> marker(x * 256 + i * 8, y * 256 + j * 8, z);
                    tile->m_markers.push_back(marker);
                    LeaveCriticalSection(&m_zoneLock);
                }
            }
            if (top >= 0 && top < height)
                sprite->voxel(i, j, top) = plasma::Vector<3, unsigned char>(color);
            quadrantColors[i / 16][j / 16] += color;
            int bottom = lowest / 8 - tile->m_baseZ - 1;
            if (bottom < 0)
                bottom = 0;
            for (int z = bottom; z < top; z++) {
                plasma::Vector<3, float> sum(0, 0, 0);
                int count = 0;
                for (int dx = 0; dx < 8; dx++) {
                    for (int dy = 0; dy < 8; dy++) {
                        for (int dz = 0; dz < 8; dz++) {
                            const Block &block = m_world->getBlock(x * 256 + i * 8 + dx, y * 256 + j * 8 + dy,
                                                                   (tile->m_baseZ + z) * 8 + dz, 0);
                            int type = block.type & ~BlockFlagMask;
                            if (type) {
                                count++;
                                if (type == BlockSolid)
                                    sum += m_world->rockColor(zone->m_position[0] * 256 + i * 8 + dx,
                                                              zone->m_position[1] * 256 + j * 8 + dy,
                                                              (tile->m_baseZ + z) * 8 + dz, zone);
                                else
                                    sum += type == BlockWater || type == BlockWet
                                               ? plasma::Vector<3, float>(80, 100, 255)
                                               : plasma::Vector<3, float>(block.color);
                            }
                        }
                    }
                }
                if (count)
                    sprite->voxel(i, j, z) = plasma::Vector<3, unsigned char>((1.0f / count) * sum);
            }
        }
    }
    sprite->updateMesh();
    BytesIO output;
    output.write(&m_formatVersion, sizeof(m_formatVersion));
    output.write(&tile->m_baseZ, sizeof(tile->m_baseZ));
    plasma::Vector<3, int> size = sprite->m_size;
    output.write(&size, sizeof(size));
    if (size[0] > 0 && size[1] > 0 && size[2] > 0) {
        std::vector<char> voxels;
        voxels.resize(size[0] * size[1] * size[2] * 3);
        memcpy(&voxels[0], sprite->m_voxels, voxels.size());
        std::vector<char> compressed;
        compress(voxels, compressed);
        int compressedSize = (int)compressed.size();
        output.write(&compressedSize, sizeof(compressedSize));
        output.write(&compressed[0], compressedSize);
    }
    int markerCount = (int)tile->m_markers.size();
    output.write(&markerCount, sizeof(markerCount));
    for (std::list<plasma::Vector<3, int>>::iterator it = tile->m_markers.begin(); it != tile->m_markers.end(); it++)
        output.write(&*it, sizeof(*it));
    m_database.write(key.str(), output);
    EnterCriticalSection(&m_zoneLock);
    if (tile->m_sprite)
        delete tile->m_sprite;
    else
        tile->m_appearanceTime = 250;
    tile->m_flags |= (1 << ZoneTileBuilt);
    tile->m_sprite = sprite;
    LeaveCriticalSection(&m_zoneLock);
    BytesIO discovered;
    discovered.write(&m_discovered, sizeof(m_discovered));
    m_database.write("discovered", discovered);
}

// Writes the landscape tile (x, y) to the database under "land<x>_<y>": the format version, the
// origin, m_discovered, the model's size, its voxels deflated (when not empty) and the tile's
// info.
void WorldMap::saveLandscape(int x, int y)
{
    if (!m_database.isOpen())
        return;
    LandscapeTile *tile = m_landscape[x][y];
    if (!tile || !tile->m_sprite)
        return;
    std::stringstream key;
    key << "land" << x << "_" << y;
    BytesIO data;
    data.write(&m_formatVersion, sizeof(m_formatVersion));
    data.write(&tile->m_origin, sizeof(tile->m_origin));
    data.write(&tile->m_discovered, sizeof(tile->m_discovered));
    plasma::Vector<3, int> size(tile->m_sprite->sizeX(), tile->m_sprite->sizeY(), tile->m_sprite->sizeZ());
    data.write(&size, sizeof(size));
    if (size[0] > 0 && size[1] > 0 && size[2] > 0) {
        std::vector<char> voxels(size[0] * size[1] * size[2] * 3);
        memcpy(&voxels[0], tile->m_sprite->m_voxels, voxels.size());
        std::vector<char> compressed;
        compress(voxels, compressed);
        int compressedSize = (int)compressed.size();
        data.write(&compressedSize, sizeof(compressedSize));
        data.write(&compressed[0], compressedSize);
    }
    data.write(&tile->m_info, sizeof(tile->m_info));
    m_database.write(key.str(), data);
}

// Writes the region (x, y) to the database under "reg<x>_<y>": the format version, then per zone
// tile its flags and cell, then the 8 x 8 records.
void WorldMap::saveRegion(int x, int y)
{
    if (!m_database.isOpen())
        return;
    if (!m_regions[x][y])
        return;
    std::stringstream key;
    key << "reg" << x << "_" << y;
    BytesIO data;
    data.write(&m_formatVersion, sizeof(m_formatVersion));
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 64; j++) {
            ZoneTile &tile = m_regions[x][y]->m_tiles[i][j];
            data.write(&tile.m_flags, sizeof(tile.m_flags));
            data.write(&tile.m_cell.kind, sizeof(tile.m_cell.kind));
            data.write(&tile.m_cell.style, sizeof(tile.m_cell.style));
            data.write(&tile.m_cell.unused02, sizeof(tile.m_cell.unused02));
            data.write(&tile.m_cell.unused03, sizeof(tile.m_cell.unused03));
            data.write(&tile.m_cell.seed, sizeof(tile.m_cell.seed));
            data.write(&tile.m_cell.level, sizeof(tile.m_cell.level));
            data.write(&tile.m_cell.rarity, sizeof(tile.m_cell.rarity));
        }
    }
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Region::Record &record = m_regions[x][y]->m_records[i][j];
            data.write(&record.kind, sizeof(record.kind));
            data.write(&record.style, sizeof(record.style));
            data.write(&record.seed, sizeof(record.seed));
            data.write(&record.level, sizeof(record.level));
            data.write(&record.rarity, sizeof(record.rarity));
            data.write(&record.height, sizeof(record.height));
            data.write(&record.center, sizeof(record.center));
            data.write(&record.radius, sizeof(record.radius));
        }
    }
    m_database.write(key.str(), data);
}

}
