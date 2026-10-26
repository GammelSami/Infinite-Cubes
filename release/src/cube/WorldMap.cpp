#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include "cube/Noise.h"
#include "plasma/EditFilter.h"
#include "cube/WorldMap.h"

#include "Parallel.h"
#include <stdlib.h>
#include <algorithm>
#include <array>
#include <set>

#include "cube/CaveBuilder.h"
#include "cube/Creature.h"
#include "cube/Fixed.h"
#include "cube/Missions.h"
#include "cube/Realm.h"
#include "cube/Sprite.h"
#include "cube/SpriteManager.h"
#include "cube/World.h"
#include "cube/WorldLayout.h"
#include "gfx/Renderer.h"
#include "gfx/SpriteBatch.h"
#include "plasma/D3D11Engine.h"
#include "cube/CreatureIds.h"
#include "cube/BlockIds.h"
#include "cube/SpriteIds.h"
#include "cube/MissionIds.h"
#include "cube/WorldIds.h"

namespace cube {

// How many regions around the map position are generated and shown in each direction (halved in flight master mode),
// and the width in blocks of the border generated around a region.
static int g_mapRegionRadius = 8;
static int g_mapRegionBorder = 3;
// 4 * 65536; not read by any function of the file.
int s_mapUnit = (int)(pow(2.0, 16.0) * 4.0);

namespace {
const int MapZoomLevelCount = 9;
// cleanUp keeps the regions drawn most recently, up to this many.
const int MapRegionCacheCapacity = 1000;
// update generates at most this many missing regions, the nearest first.
const int MapRegionsPerUpdate = 8;
// render skips a viewport lower than this.
const float MinViewportHeight = 0.0001f;
// The length of the cursor's ray into a region's model.
const float MapPickRayLength = 5000.0f;
// The angular rate (per clock millisecond) of the pulsing markers: the hovered region's flight points, the tombstone
// and hostile creatures.
const float MapMarkerPulseRate = 0.006f;
// A creature marker's scale, divided by the view's.
const float MapCreatureMarkerScale = 0.4f;
// The flight route: a dot every five map units (scaled by the view), each 1.5 units wide (over a shadow twice as
// wide).
const int FlightRouteDotSpacing = 5;
const float FlightRouteDotScale = 1.5f;
}

WorldMap::WorldMap(World *world, plasma::D3D11Engine *engine)
    : m_world(world),
      m_engine(engine),
      m_hoveredRegion(NoCoordinate, NoCoordinate),
      m_selectedLand(NoCoordinate, NoCoordinate),
      m_markedCreatureId(0),
      m_selectedFlightPoint(NoCoordinate, NoCoordinate),
      m_selectedMarker(NoCoordinate, NoCoordinate),
      m_flightmasterMode(false),
      m_frameCounter(0),
      m_regionRadius(0)
{
    InitializeCriticalSectionAndSpinCount(&m_regionsCriticalSection, 16);
    InitializeCriticalSectionAndSpinCount(&m_unusedLock, 16);
    m_levels.resize(MapZoomLevelCount);
}

WorldMapRegion::WorldMapRegion()
    : sprite(nullptr),
      cacheTimes(0, 0)
{}

WorldMapZoom::WorldMapZoom()
    : previewSprite(nullptr),
      unusedSprite(nullptr),
      unused20(0, 0),
      unused28(0, 0),
      unused30(-1),
      unused34(false),
      unused35(false)
{}

WorldMap::~WorldMap()
{
    for (std::pair<const long long, Creature *> &entry : m_creatures) {
        delete entry.second;
    }
    DeleteCriticalSection(&m_regionsCriticalSection);
    DeleteCriticalSection(&m_unusedLock);
    for (WorldMapZoom &level : m_levels) {
        delete level.previewSprite;
        delete level.unusedSprite;
        for (std::pair<const std::array<int, 2>, Sprite *> entry : level.unusedSprites) {
            delete entry.second;
        }
    }
    for (std::pair<const std::array<int, 3>, WorldMapRegion *> &entry : m_regions) {
        delete entry.second;
    }
}

WorldMapRegion::~WorldMapRegion()
{
    delete sprite;
}

void WorldMap::cleanUp()
{
    std::list<std::map<std::array<int, 3>, WorldMapRegion *>::iterator> unused;
    std::vector<std::map<std::array<int, 3>, WorldMapRegion *>::iterator> used;
    std::map<std::array<int, 3>, WorldMapRegion *>::iterator it;
    for (it = m_regions.begin(); it != m_regions.end(); ++it) {
        if (it->second == nullptr) {
            unused.push_back(it);
        } else {
            used.push_back(it);
        }
    }
    std::sort(used.begin(), used.end(),
              [](const std::map<std::array<int, 3>, WorldMapRegion *>::iterator &a,
                 const std::map<std::array<int, 3>, WorldMapRegion *>::iterator &b) {
                  return a->second->cacheTimes[0] > b->second->cacheTimes[0];
              });
    for (int i = MapRegionCacheCapacity; i < (int)used.size(); i++) {
        Sleep(0);
        it = used[i];
        WorldMapRegion *region = it->second;
        EnterCriticalSection(&m_regionsCriticalSection);
        it->second = nullptr;
        LeaveCriticalSection(&m_regionsCriticalSection);
        delete region;
        unused.push_back(it);
    }
    for (std::map<std::array<int, 3>, WorldMapRegion *>::iterator &entry : unused) {
        EnterCriticalSection(&m_regionsCriticalSection);
        m_regions.erase(entry);
        LeaveCriticalSection(&m_regionsCriticalSection);
        Sleep(0);
    }
}

void WorldMap::clear()
{
    std::map<std::array<int, 3>, WorldMapRegion *> regions = m_regions;
    EnterCriticalSection(&m_regionsCriticalSection);
    m_regions.clear();
    LeaveCriticalSection(&m_regionsCriticalSection);
    for (std::map<std::array<int, 3>, WorldMapRegion *>::iterator it = regions.begin(); it != regions.end(); ++it) {
        delete it->second;
    }
}

void WorldMap::enter()
{
    EnterCriticalSection(&m_regionsCriticalSection);
}

plasma::Vector<3, float> heightTint(const plasma::Vector<3, float> &color, int regionSize, int z, int lowest,
                                    int highest, float temperature, float humidity, float river)
{
    // The low ground is tinted towards a palette by temperature: warm brown (130, 60, 0) where it is hot (from 0.8,
    // eight times as fast) and dry, else cool blue (0, 40, 140), at the colour's luma (Rec. 601 weights, over 128), by
    // up to 0.8 with the cube of the depth below `highest`.
    plasma::Vector<3, float> result = color;
    float fade = (float)(z - lowest) / (float)(highest - lowest);
    fade = std::max(0.0f, fade);
    fade = std::min(1.0f, fade);
    fade = 1.0f - fade;
    fade = fade * fade * fade;
    float warmth = (std::max)(0.0f, (std::min)(1.0f, (temperature - 0.8f) * (1.0f - humidity) * 8.0f));
    plasma::Vector<3, float> tint = plasma::Vector<3, float>(130.0f, 60.0f, 0.0f) * warmth +
                                    plasma::Vector<3, float>(0.0f, 40.0f, 140.0f) * (1.0f - warmth);
    plasma::Vector<3, float> scaled = result / 128.0f;
    float luminance = scaled[0] * 0.299f + scaled[1] * 0.587f + scaled[2] * 0.114f;
    result += (tint * luminance - result) * (fade * 0.8f);
    return result;
}

void WorldMap::leave()
{
    LeaveCriticalSection(&m_regionsCriticalSection);
}

void WorldMap::markCreature(Creature *creature)
{
    if (m_creatures[creature->m_id] == nullptr) {
        m_creatures[creature->m_id] = new Creature(creature->m_id);
    }
    m_creatures[creature->m_id]->m_entityData = creature->m_entityData;
    m_creatures[creature->m_id]->m_walkAmplitude = creature->m_walkAmplitude;
    m_creatures[creature->m_id]->m_walkPhase = creature->m_walkPhase;
    m_creatures[creature->m_id]->m_animationState.m_animation = creature->m_animationState.m_animation;
    m_creatures[creature->m_id]->m_animationState.m_animationTime = creature->m_animationState.m_animationTime;
}

struct MapCreature {
    // The character itself (race -1: drawn with sprite 3559 and always pulsing; its hostility is never read).
    MapCreature(const plasma::Vector<3, Fixed> &position, float rotation, int race, float depth)
        : position(position),
          rotation(rotation),
          race(race),
          scale(1.0f),
          depth(depth)
    {}
    // A creature of the world.
    MapCreature(const plasma::Vector<3, Fixed> &position, float rotation, int race, int hostility,
                const Creature::EntityData::Appearance &appearance, float depth)
        : position(position),
          rotation(rotation),
          race(race),
          hostility(hostility),
          appearance(appearance),
          scale(1.0f),
          depth(depth)
    {}

    plasma::Vector<3, Fixed> position;           // Creature::m_drawnPosition, the drawn position
    float rotation;                              // Creature::m_drawnRotation[2], the drawn rotation about z
    int race;                                    // EntityData::race, or -1 for the character
    int hostility;                               // EntityData::hostilityType; a hostile creature's marker pulses
    Creature::EntityData::Appearance appearance; // the head, hair and chest models and the hair colour are drawn
    float scale;                                 // 1 when constructed; render sets 0.4 / the view scale
    float depth;                                 // the view-space depth, the sort key (far first)
};

void WorldMap::render(gfx::SpriteBatch &batch, const plasma::Vector<3, Fixed> &center, int cursorX, int cursorY,
                      int width, int height, const plasma::Vector<3, float> &angles, int zoom, float viewScaleFactor,
                      int frameTime, bool controlKeyDown, bool passiveView)
{
    m_cursorPosition = plasma::Vector<3, Fixed>(Fixed(NoCoordinate), Fixed(NoCoordinate), Fixed(-1));
    m_creaturePositions.clear();
    int level = zoom >= 0 ? zoom : 0;
    if (level >= (int)m_levels.size()) {
        level = (int)m_levels.size() - 1;
    }
    int scale = (int)(powf(2.0f, (float)level) * BlocksPerMapUnitAtLevelZero);
    if (scale < 1) {
        scale = 1;
    }
    if ((float)height < MinViewportHeight) {
        return;
    }
    int regionSize = g_mapRegionSize * scale;

    // A perspective of 45 degrees, mirrored in x (the aspect negated), with the near plane at 0.1 and the far plane at
    // 1000, then shifted so that the cursor pixel is the centre of projection (and moved back by the near distance).
    gfx::perspectiveFov(m_projectionMatrix, 0.78539819f, -((float)width / (float)height), 0.1f, 1000.0f);
    plasma::Matrix<float> shift;
    shift.identity();
    shift.translate(((float)cursorX - (float)width * 0.5f) / (float)width * 2.0f,
                    ((float)cursorY - (float)height * 0.5f) / (float)height * 2.0f, -0.1f);
    shift *= m_projectionMatrix;
    m_projectionMatrix = shift;

    // The view: 300 units back from the map, scaled (by the caller's factor times 0.6 * 80 / 120 * 16 over the region
    // radius), tilted about x and turned about z.
    m_viewMatrix.identity();
    m_viewMatrix.translate(0.0f, 0.0f, 300.0f);
    float viewScale = viewScaleFactor * 0.6f * 80.0f / 120.0f * 16.0f / (float)g_mapRegionRadius;
    m_viewMatrix.scale(viewScale, viewScale, viewScale);
    m_viewMatrix.rotateZ(0.0f);
    m_viewMatrix.rotateX(angles[0]);
    m_viewMatrix.rotateZ(angles[2]);

    // The regions around the centre, in region units of the zoom level.
    int mapRegionX = (int)center[0] / regionSize;
    int mapRegionY = (int)center[1] / regionSize;
    m_frameCounter++;
    unsigned int now = timeGetTime();
    m_regionRadius = g_mapRegionRadius;
    if (passiveView) {
        m_regionRadius /= 2;
    }
    int firstX = mapRegionX - m_regionRadius;
    int lastX = mapRegionX + m_regionRadius - 1;
    int firstY = mapRegionY - m_regionRadius;
    int lastY = mapRegionY + m_regionRadius - 1;
    Block block = m_world->getBlockAtDot(m_world->m_localCreature->m_entityData.position[0],
                                         m_world->m_localCreature->m_entityData.position[1],
                                         m_world->m_localCreature->m_entityData.position[2]);
    m_totalGenerationTime = 0;
    for (int i = firstX; i <= lastX; i++) {
        for (int j = firstY; j <= lastY; j++) {
            WorldMapRegion *region = m_regions[std::array<int, 3> {{i, j, level}}];
            if (region == nullptr || region->sprite == nullptr) {
                continue;
            }
            m_totalGenerationTime += region->generationTime;
            // The region's model stands 3 * scale blocks before its cell (the generator's border) and baseHeight *
            // scale blocks up.
            plasma::Vector<3, Fixed> offset =
                plasma::Vector<3, Fixed>(i * regionSize - 3 * scale, j * regionSize - 3 * scale,
                                         region->baseHeight * scale) -
                center;
            plasma::Vector<3, float> translation = plasma::Vector<3, float>(offset) / (float)scale;
            plasma::Matrix<float> matrix;
            matrix.identity();
            matrix.translate(translation[0], translation[1], translation[2]);
            // Fades in over 200 ms after the region was generated (fully opaque from 0.99).
            float alpha = std::max(0.0f, (float)(now - region->cacheTimes[1]) * 0.005f);
            if (alpha > 0.99) {
                alpha = 1.0f;
            }
            // The batch's last argument is the drawing layer: the regions 0.5, the sites 1.5, the flight points 1.6,
            // the creatures 2 and up, the flight route 2.5 over its shadow at 2.
            batch.add(region->sprite, matrix, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), alpha, 0.0f, 0.0f, 0.0f,
                      0.5f);
            region->cacheTimes[0] = m_frameCounter;
            if (!passiveView) {
                // The cursor: the engine's pointer, unprojected through the region's matrices and cast into its model.
                plasma::Matrix<float> inverse = m_projectionMatrix;
                inverse *= m_viewMatrix;
                inverse *= matrix;
                inverse.invert();
                plasma::Vector<3, float> origin =
                    inverse * plasma::Vector<3, float>(
                                  m_engine->getPointer()[0] / (float)m_engine->viewportWidth() * 2.0f - 1.0f,
                                  1.0f - m_engine->getPointer()[1] / (float)m_engine->viewportHeight() * 2.0f, 1.0f);
                plasma::Vector<3, float> direction =
                    inverse * plasma::Vector<3, float>(
                                  m_engine->getPointer()[0] / (float)m_engine->viewportWidth() * 2.0f - 1.0f,
                                  1.0f - m_engine->getPointer()[1] / (float)m_engine->viewportHeight() * 2.0f, 0.0f) -
                    origin;
                direction *= 1.0f / sqrtf(direction[0] * direction[0] + direction[1] * direction[1] +
                                          direction[2] * direction[2]);
                plasma::Vector<3, int> voxel;
                plasma::Vector<3, int> face;
                float distance = region->sprite->rayCast(origin, direction, MapPickRayLength, false, voxel, face);
                if (distance < MapPickRayLength) {
                    plasma::Vector<3, Fixed> hit((direction * distance + origin) * (float)scale);
                    m_cursorPosition = hit + plasma::Vector<3, Fixed>(i * regionSize - g_mapRegionBorder * scale,
                                                                      j * regionSize - g_mapRegionBorder * scale,
                                                                      region->baseHeight * scale); // as above
                    m_hoveredRegion = m_world->findRegion((int)m_cursorPosition[0], (int)m_cursorPosition[1]);
                }
            }
            if (m_selectedLand[0] != NoCoordinate) {
                m_hoveredRegion = m_selectedLand;
            }
            // The sites of the hovered place, while the character stands on a block with BlockFlagDungeon and the
            // region is fully faded in.
            if ((block.flags & BlockFlagDungeon) && alpha == 1.0f) {
                float siteScale = (float)(BlocksPerCell / scale);
                for (const MapCell &site : region->sites) {
                    std::array<int, 2> place = {{toCell(site.position[0]), toCell(site.position[1])}};
                    std::map<std::array<int, 2>, unsigned char>::iterator known =
                        m_world->m_localCreature->m_playerData->placeVisibility.find(place);
                    if (known == m_world->m_localCreature->m_playerData->placeVisibility.end()) {
                        continue;
                    }
                    plasma::Matrix<float> siteMatrix;
                    siteMatrix.identity();
                    plasma::Vector<2, int> cell(site.position[0] / scale, site.position[1] / scale);
                    plasma::Vector<3, Fixed> siteOffset =
                        plasma::Vector<3, Fixed>(Fixed(cell[0] * scale), Fixed(cell[1] * scale),
                                                 Fixed(site.dungeonZLevel / scale * scale)) -
                        center;
                    plasma::Vector<3, float> translation = plasma::Vector<3, float>(siteOffset) * (float)(1.0 / scale);
                    siteMatrix.translate(translation[0], translation[1], translation[2]);
                    siteMatrix.scale(siteScale, siteScale, siteScale);
                    // White, or the colour of a pit or pool (flattened to half height); drawn at 75% of the site's
                    // visibility.
                    plasma::Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
                    if (site.structure.kind == StructureCell_DeepPit) {
                        color = plasma::Vector<4, float>(0.2f, 0.2f, 0.2f, 1.0f);
                        siteMatrix.scale(1.0f, 1.0f, 0.5f);
                    }
                    if (site.structure.kind == StructureCell_LavaPool) {
                        color = plasma::Vector<4, float>(1.0f, 0.3f, 0.2f, 1.0f);
                        siteMatrix.scale(1.0f, 1.0f, 0.5f);
                    }
                    if (site.structure.kind == StructureCell_WaterPool) {
                        color = plasma::Vector<4, float>(0.2f, 0.5f, 1.0f, 1.0f);
                        siteMatrix.scale(1.0f, 1.0f, 0.5f);
                    }
                    if (site.structure.kind == StructureCell_MuckPool) {
                        color = plasma::Vector<4, float>(0.3f, 0.5f, 0.1f, 1.0f);
                        siteMatrix.scale(1.0f, 1.0f, 0.5f);
                    }
                    batch.add(m_world->m_spriteManager->sprite(SpriteCube), siteMatrix, color,
                              (float)(unsigned char)known->second * 0.75f / 255.0f, 0.0f, 0.0f, 0.0f, 1.5f);
                }
            }
        }
    }

    // The flight points of the hovered region, pulsing (0..0.8 opaque).
    if (m_hoveredRegion[0] != NoCoordinate) {
        float pulse = (cosf(m_world->m_clock * MapMarkerPulseRate) + 1.0f) * 0.4f;
        for (int i = firstX; i <= lastX; i++) {
            for (int j = firstY; j <= lastY; j++) {
                WorldMapRegion *region = m_regions[std::array<int, 3> {{i, j, level}}];
                if (region == nullptr || region->sprite == nullptr) {
                    continue;
                }
                std::array<int, 2> place = {{m_hoveredRegion[0], m_hoveredRegion[1]}};
                std::map<std::array<int, 2>, std::list<plasma::Vector<3, int>>>::iterator positions =
                    region->positionsByPlace.find(place);
                if (positions == region->positionsByPlace.end()) {
                    continue;
                }
                float markerScale = 2.0f / viewScaleFactor;
                float inverseScale = (float)(1.0 / scale);
                for (const plasma::Vector<3, int> &position : positions->second) {
                    Sprite *sprite = m_world->m_spriteManager->sprite(SpriteCube);
                    plasma::Matrix<float> matrix;
                    matrix.identity();
                    plasma::Vector<3, Fixed> offset =
                        plasma::Vector<3, Fixed>(Fixed(position[0]), Fixed(position[1]), Fixed(position[2])) - center;
                    plasma::Vector<3, float> translation = plasma::Vector<3, float>(offset) * inverseScale;
                    matrix.translate(translation[0], translation[1], translation[2]);
                    matrix.scale(markerScale, markerScale, markerScale);
                    matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                                     (float)sprite->m_size[2] * -0.5f);
                    batch.add(sprite, matrix, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), pulse, 0.0f, 0.0f, 0.0f,
                              1.6f);
                }
            }
        }
    }

    // The creatures: passive ones, and hostile ones whose toggle 8 is set while the character has not completed the
    // mission at their destination. Their depth in view space orders the drawing.
    plasma::Matrix<float> view = m_viewMatrix;
    std::vector<MapCreature> creatures;
    float layer = 2.0f;
    for (Creature *creature : m_world->m_creatures) {
        if (creature->m_entityData.currentAbility == AbilityMenuView) {
            continue;
        }
        if (creature->m_entityData.hostilityType == HostilityPlayer ||
            (creature->m_entityData.binaryToggles.missionComplete &&
             !m_world->m_localCreature->hasCompletedMission(creature->m_entityData.destination[0],
                                                            creature->m_entityData.destination[1]))) {
            double inverse = 1.0 / scale;
            Fixed inverseScale(inverse);
            plasma::Vector<3, Fixed> offset = creature->m_entityData.position - center;
            plasma::Vector<3, float> scaled(offset * inverseScale);
            float depth = view[2] * scaled[0] + view[6] * scaled[1] + view[10] * scaled[2];
            MapCreature marker(creature->m_drawnPosition, creature->m_drawnRotation[2], creature->m_entityData.race,
                               creature->m_entityData.hostilityType, creature->m_entityData.appearance, depth);
            creatures.push_back(marker);
            creatures.back().scale = MapCreatureMarkerScale / viewScale;
        }
        // Where the map shows a passive creature: its position, moved to the edge of the shown square along x and y
        // when it lies beyond.
        if (creature->m_entityData.hostilityType == HostilityPlayer && creature != m_world->m_localCreature &&
            creature->m_entityData.currentAbility != AbilityMenuView) {
            plasma::Vector<3, Fixed> position = creature->m_entityData.position;
            int distanceX = (int)(position[0] - center[0]);
            if (abs(distanceX) >= g_mapRegionSize * scale * m_regionRadius) {
                // A double, narrowed where the vector is divided by it.
                double distance = fabs((double)(int)(position[0] - center[0]));
                plasma::Vector<3, Fixed> offset = position - center;
                position =
                    center + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(offset) / distance *
                                                      (float)m_regionRadius * (float)g_mapRegionSize * (float)scale);
            }
            int distanceY = (int)(position[1] - center[1]);
            if (abs(distanceY) >= g_mapRegionSize * scale * m_regionRadius) {
                double distance = fabs((double)(int)(position[1] - center[1]));
                plasma::Vector<3, Fixed> offset = position - center;
                position =
                    center + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(offset) / distance *
                                                      (float)m_regionRadius * (float)g_mapRegionSize * (float)scale);
            }
            m_creaturePositions[creature->m_id] = position;
        }
    }
    // The character, at PlayerData::deathPosition while deathMarkerTime is positive.
    if (m_world->m_localCreature != nullptr && m_world->m_localCreature->m_playerData->deathMarkerTime > 0.0f) {
        double inverse = 1.0 / scale;
        Fixed inverseScale(inverse);
        plasma::Vector<3, Fixed> offset = m_world->m_localCreature->m_playerData->deathPosition - center;
        plasma::Vector<3, float> scaled(offset * inverseScale);
        float depth = view[2] * scaled[0] + view[6] * scaled[1] + view[10] * scaled[2];
        MapCreature marker(m_world->m_localCreature->m_playerData->deathPosition, 0.0f, RaceNone, depth);
        creatures.push_back(marker);
        creatures.back().scale = MapCreatureMarkerScale / viewScale;
    }
    std::sort(creatures.begin(), creatures.end(),
              [](const MapCreature &a, const MapCreature &b) { return a.depth > b.depth; });
    for (MapCreature &marker : creatures) {
        // The sprites of the marker: the head (or the chest when the head has no model) and the hair in its colour;
        // the character's is sprite 3559.
        std::list<std::pair<Sprite *, plasma::Vector<4, float>>> sprites;
        float pulse = 0.0f;
        if (marker.race == RaceNone) {
            sprites.push_back(std::make_pair(m_world->m_spriteManager->sprite(SpriteTombstone),
                                             plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)));
            pulse = (cosf(m_world->m_clock * MapMarkerPulseRate) + 1.0f) * 0.5f;
        } else {
            if (m_world->m_spriteManager->sprite(marker.appearance.headModel) != nullptr) {
                sprites.push_back(std::make_pair(m_world->m_spriteManager->sprite(marker.appearance.headModel),
                                                 plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)));
            } else {
                sprites.push_back(std::make_pair(m_world->m_spriteManager->sprite(marker.appearance.chestModel),
                                                 plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)));
            }
            sprites.push_back(
                std::make_pair(m_world->m_spriteManager->sprite(marker.appearance.hairModel),
                               plasma::Vector<4, float>((float)marker.appearance.hairColor[0] / 255.0f,
                                                        (float)marker.appearance.hairColor[1] / 255.0f,
                                                        (float)marker.appearance.hairColor[2] / 255.0f, 1.0f)));
            if (marker.hostility != HostilityPlayer) {
                pulse = (cosf(m_world->m_clock * MapMarkerPulseRate) + 1.0f) * 0.5f;
            }
        }
        plasma::Vector<3, Fixed> position = marker.position;
        int distanceX = (int)(position[0] - center[0]);
        if (abs(distanceX) >= g_mapRegionSize * scale * m_regionRadius) {
            double distance = fabs((double)(int)(position[0] - center[0]));
            plasma::Vector<3, Fixed> offset = position - center;
            position = center + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(offset) / distance *
                                                         (float)m_regionRadius * (float)g_mapRegionSize * (float)scale);
        }
        int distanceY = (int)(position[1] - center[1]);
        if (abs(distanceY) >= g_mapRegionSize * scale * m_regionRadius) {
            double distance = fabs((double)(int)(position[1] - center[1]));
            plasma::Vector<3, Fixed> offset = position - center;
            position = center + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(offset) / distance *
                                                         (float)m_regionRadius * (float)g_mapRegionSize * (float)scale);
        }
        for (std::pair<Sprite *, plasma::Vector<4, float>> &entry : sprites) {
            Sprite *sprite = entry.first;
            plasma::Vector<4, float> color = entry.second;
            if (sprite == nullptr) {
                continue;
            }
            plasma::Matrix<float> matrix;
            matrix.identity();
            plasma::Vector<3, Fixed> offset = position - center;
            plasma::Vector<3, float> translation = plasma::Vector<3, float>(offset) * (float)(1.0 / scale);
            matrix.translate(translation[0], translation[1], translation[2]);
            matrix.scale(marker.scale, marker.scale, marker.scale);
            matrix.rotateZ(marker.rotation);
            matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                             (float)sprite->m_size[2] * -0.5f);
            // A black outline one unit around the sprite, then the sprite.
            for (int dx = -1; dx < 2; dx++) {
                for (int dy = -1; dy < 2; dy++) {
                    for (int dz = -1; dz < 2; dz++) {
                        if (dx != 0 || dy != 0 || dz != 0) {
                            plasma::Matrix<float> outline = matrix;
                            outline.translate((float)dx, (float)dy, (float)dz);
                            batch.add(sprite, outline, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f), 1.0f, 0.0f,
                                      0.0f, pulse, layer);
                        }
                    }
                }
            }
            batch.add(sprite, matrix, color, 1.0f, 0.0f, 0.0f, 0.0f, layer + 0.5f);
        }
        layer += 1.0f;
    }

    // A dotted line from the character to the selected block, or to its flight destination while it flies (ability
    // 156), rising in an arc.
    plasma::Vector<2, int> target = m_selectedFlightPoint;
    if (target[0] == NoCoordinate) {
        Creature *local = m_world->m_localCreature;
        if (local->m_entityData.currentAbility != AbilityEagleFlight) {
            return;
        }
        target = local->m_entityData.destination;
        if (target[0] == NoCoordinate) {
            return;
        }
    }
    int targetZ = (int)m_world->height(target[0], target[1], 0);
    plasma::Vector<2, int> start((int)m_world->m_localCreature->m_entityData.position[0],
                                 (int)m_world->m_localCreature->m_entityData.position[1]);
    int startZ = (int)m_world->m_localCreature->m_entityData.position[2];
    {
        int distanceX = (int)((Fixed(target[0]))-center[0]);
        if (abs(distanceX) >= m_regionRadius * g_mapRegionSize * scale) {
            plasma::Vector<3, int> centerBlock(center);
            double distance = fabs((double)(int)((Fixed(target[0]))-center[0]));
            plasma::Vector<3, Fixed> moved =
                center +
                plasma::Vector<3, Fixed>(plasma::Vector<3, float>((float)(target[0] - centerBlock[0]),
                                                                  (float)(target[1] - centerBlock[1]),
                                                                  (float)(targetZ - centerBlock[2])) /
                                         distance * (float)m_regionRadius * (float)g_mapRegionSize * (float)scale);
            target = plasma::Vector<2, int>((int)moved[0], (int)moved[1]);
            targetZ = (int)moved[2];
        }
        int distanceY = (int)((Fixed(target[1]))-center[1]);
        if (abs(distanceY) >= m_regionRadius * g_mapRegionSize * scale) {
            plasma::Vector<3, int> centerBlock(center);
            double distance = fabs((double)(int)((Fixed(target[1]))-center[1]));
            plasma::Vector<3, Fixed> moved =
                center +
                plasma::Vector<3, Fixed>(plasma::Vector<3, float>((float)(target[0] - centerBlock[0]),
                                                                  (float)(target[1] - centerBlock[1]),
                                                                  (float)(targetZ - centerBlock[2])) /
                                         distance * (float)m_regionRadius * (float)g_mapRegionSize * (float)scale);
            target = plasma::Vector<2, int>((int)moved[0], (int)moved[1]);
            targetZ = (int)moved[2];
        }
        int startDistanceX = (int)((Fixed(start[0]))-center[0]);
        if (abs(startDistanceX) >= m_regionRadius * g_mapRegionSize * scale) {
            plasma::Vector<3, int> centerBlock(center);
            double distance = fabs((double)(int)((Fixed(start[0]))-center[0]));
            plasma::Vector<3, Fixed> moved =
                center +
                plasma::Vector<3, Fixed>(plasma::Vector<3, float>((float)(start[0] - centerBlock[0]),
                                                                  (float)(start[1] - centerBlock[1]),
                                                                  (float)(startZ - centerBlock[2])) /
                                         distance * (float)m_regionRadius * (float)g_mapRegionSize * (float)scale);
            start = plasma::Vector<2, int>((int)moved[0], (int)moved[1]);
            startZ = (int)moved[2];
        }
        int startDistanceY = (int)((Fixed(start[1]))-center[1]);
        if (abs(startDistanceY) >= m_regionRadius * g_mapRegionSize * scale) {
            plasma::Vector<3, int> centerBlock(center);
            double distance = fabs((double)(int)((Fixed(start[1]))-center[1]));
            plasma::Vector<3, Fixed> moved =
                center +
                plasma::Vector<3, Fixed>(plasma::Vector<3, float>((float)(start[0] - centerBlock[0]),
                                                                  (float)(start[1] - centerBlock[1]),
                                                                  (float)(startZ - centerBlock[2])) /
                                         distance * (float)m_regionRadius * (float)g_mapRegionSize * (float)scale);
            start = plasma::Vector<2, int>((int)moved[0], (int)moved[1]);
            startZ = (int)moved[2];
        }
    }
    plasma::Vector<3, double> direction((double)(start[0] - target[0]), (double)(start[1] - target[1]),
                                        (double)(startZ - targetZ));
    int spacing = (int)((float)(scale * FlightRouteDotSpacing) / viewScaleFactor);
    double length = direction.length();
    int count = (int)(length / (double)spacing);
    direction *= 1.0 / length;
    if (count < 0) {
        return;
    }
    for (int k = count; k >= 0; k--) {
        plasma::Vector<3, int> advance(direction * (double)(k * spacing));
        // The dot's height follows an arc from the target's height up to the character's.
        float along = (float)k / (float)count;
        float arc = 1.0f - (1.0f - along) * (1.0f - along);
        Fixed z(targetZ + advance[2]);
        plasma::Vector<3, Fixed> dot(Fixed(target[0] + advance[0]), Fixed(target[1] + advance[1]),
                                     z + Fixed(arc) * (Fixed(startZ) - z));
        if (abs((int)(dot[0] - center[0])) >= regionSize * m_regionRadius ||
            abs((int)(dot[1] - center[1])) >= regionSize * m_regionRadius) {
            continue;
        }
        Sprite *sprite = m_world->m_spriteManager->sprite(SpriteCube);
        float dotScale = FlightRouteDotScale / viewScaleFactor;
        plasma::Matrix<float> matrix;
        matrix.identity();
        plasma::Vector<3, Fixed> offset = dot - center;
        plasma::Vector<3, float> translation = plasma::Vector<3, float>(offset) * (float)(1.0 / scale);
        matrix.translate(translation[0], translation[1], translation[2]);
        matrix.scale(dotScale, dotScale, dotScale);
        matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                         (float)sprite->m_size[2] * -0.5f);
        batch.add(sprite, matrix, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 0.0f, 0.0f, 0.0f, 2.5f);
        // The same dot twice as large in black behind it.
        dotScale *= 2.0f;
        matrix.identity();
        offset = dot - center;
        translation = plasma::Vector<3, float>(offset) * (float)(1.0 / scale);
        matrix.translate(translation[0], translation[1], translation[2]);
        matrix.scale(dotScale, dotScale, dotScale);
        matrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                         (float)sprite->m_size[2] * -0.5f);
        batch.add(sprite, matrix, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0.0f, 0.0f, 0.0f, 2.0f);
    }
}

int toCell(int value)
{
    return floorDivide(value, BlocksPerCell);
}

void WorldMap::unmarkCreature(Creature *creature)
{
    Creature *marked = m_creatures[creature->m_id];
    m_creatures.erase(creature->m_id);
    delete marked;
}

bool WorldMap::update(const plasma::Vector<3, Fixed> &position, int level)
{
    if (level < 0 || level >= (int)m_levels.size()) {
        return false;
    }
    if (m_world->m_worldName.size() == 0) {
        return false;
    }
    int scale = (int)(pow(2.0f, (float)level) * BlocksPerMapUnitAtLevelZero);
    if (scale < 1) {
        scale = 1;
    }
    int regionSize = g_mapRegionSize * scale;
    int x = floorDivide((int)position[0], regionSize);
    int y = floorDivide((int)position[1], regionSize);
    std::vector<plasma::Vector<3, int>> missing;
    for (int i = x - g_mapRegionRadius; i < x + g_mapRegionRadius; i++) {
        for (int j = y - g_mapRegionRadius; j < y + g_mapRegionRadius; j++) {
            if (m_regions[std::array<int, 3> {{i, j, level}}] == nullptr) {
                int squaredDistance = (i - x) * (i - x) + (j - y) * (j - y);
                missing.push_back(plasma::Vector<3, int>(i, j, squaredDistance));
            }
        }
    }
    if (missing.empty()) {
        return false;
    }
    std::sort(missing.begin(), missing.end(),
              [](const plasma::Vector<3, int> &a, const plasma::Vector<3, int> &b) { return a[2] < b[2]; });
    if (missing.size() > MapRegionsPerUpdate) {
        missing.resize(MapRegionsPerUpdate);
    }
    int step;
    if (scale < 32) {
        step = (std::min)(2, 32 / scale);
    } else {
        step = 1;
    }
    // Builds one map region: the voxel terrain and structures, plus marker lists, then stores it in m_regions.
    Concurrency::parallel_for_each(
        missing.begin(), missing.end(), [this, &regionSize, &scale, &step, &level](const plasma::Vector<3, int> &cell) {
            int mapRegionX = cell[0];
            int mapRegionY = cell[1];
            WorldMapRegion *region = new WorldMapRegion;
            region->cacheTimes[1] = timeGetTime();
            region->sprite = m_world->m_spriteManager->createSprite(true);
            // First pass: the terrain height of every column of the region and its border, and the lowest and highest
            // block the model needs (buildings and monuments rise above the terrain).
            std::vector<int> heights((g_mapRegionSize + 2 * g_mapRegionBorder) *
                                     (g_mapRegionSize + 2 * g_mapRegionBorder));
            bool first = true;
            int lowest = 0;
            int highest = 0;
            for (int i = -g_mapRegionBorder; i < g_mapRegionBorder + g_mapRegionSize; i++) {
                Sleep(0);
                for (int j = -g_mapRegionBorder; j < g_mapRegionBorder + g_mapRegionSize; j++) {
                    int x = mapRegionX * regionSize + i * scale;
                    int y = mapRegionY * regionSize + j * scale;
                    World::BiomeSample sample;
                    m_world->biomeSample(x, y, sample);
                    World::AreaInfo info;
                    m_world->fillAreaInfo(x, y, info);
                    float elevation = sample.biomeElevation(info.type, info.influence);
                    float road = m_world->streetAmount(x, y, info.type, info.influence);
                    float lava = sample.lavaWeight(info.type, info.influence);
                    float mountains = sample.mountains();
                    float undead = sample.undead(info.type, info.influence);
                    float wetness = sample.wetness();
                    float river = m_world->valleyWater(x, y, road, lava, undead, wetness, mountains, &info);
                    float lavaRiver = m_world->lavaChannels(x, y, road, lava, mountains, &info);
                    float poisonRiver = m_world->poisonChannels(x, y, road, undead, mountains, &info);
                    float desertness = sample.desertness();
                    float cliffs =
                        m_world->mountainCliffs(x, y, elevation, mountains, river, lavaRiver, poisonRiver, road, &info);
                    float terrain = m_world->height(x, y, elevation, cliffs, river, lavaRiver, poisonRiver,
                                                    m_world->sandAmount(x, y, sample.humidity(), cliffs, desertness),
                                                    road, mountains, &info, HeightFlagCaveMouths);
                    if (terrain > 0.0f && river > 0.5f) {
                        // Passes elevation as the height.
                        terrain = m_world->villageHeight(x, y, elevation, &info);
                    }
                    int top = floor(terrain + 0.999f);
                    if (terrain <= 0.0f) {
                        top = 0;
                    }
                    int rise = 2;
                    for (int di = -step; di <= step; di++) {
                        for (int dj = -step; dj <= step; dj++) {
                            int buildingCellX = di + toCell(x);
                            int buildingCellY = toCell(y) + dj;
                            World *world = m_world;
                            plasma::Vector<2, int> point = world->streetPoint(buildingCellX, buildingCellY);
                            World::AreaInfo pointInfo;
                            world->fillAreaInfo(point[0], point[1], pointInfo);
                            int type = world->streetBuildingType(buildingCellX, buildingCellY, point, pointInfo);
                            if (type == BuildingNone) {
                                continue;
                            }
                            int reach = buildingReach(type);
                            if (abs(di) > reach || abs(dj) > reach) {
                                continue;
                            }
                            if (type != BuildingBoulder && type > BuildingNone && type < BuildingTreasurePavilion) {
                                plasma::Vector<2, int> center = m_world->streetPoint(buildingCellX, buildingCellY);
                                if (type == BuildingGreatOak || type == BuildingRock || type == BuildingTallRock) {
                                    center = plasma::Vector<2, int>(BlocksPerCell * buildingCellX + BlocksPerCell / 2,
                                                                    BlocksPerCell * buildingCellY + BlocksPerCell / 2);
                                }
                                int size = m_world->townSizeC(buildingCellX, buildingCellY, 1.0f);
                                if (type == BuildingEvergreen) {
                                    size = (int)(size * 1.5f);
                                } else if (type == BuildingGreatOak) {
                                    size *= 3;
                                } else {
                                    if (type == BuildingRock || type == BuildingRockLorePavilion) {
                                        size = 40;
                                    }
                                    if (type == BuildingTallRock || type == BuildingRockTreasure) {
                                        size = 80;
                                    }
                                }
                                rise = (std::max)(rise, size);
                            } else {
                                int offset = buildingHeight(type);
                                if (type == BuildingDungeonTower || type == BuildingCastleTower) {
                                    offset += 16;
                                }
                                if (type == BuildingTower) {
                                    offset = m_world->townSizeA(buildingCellX, buildingCellY) + offset;
                                } else if (type == BuildingManaPump) {
                                    offset += 15;
                                } else if (type == BuildingManaPipeEndpoint) {
                                    offset += 10;
                                } else if (type == BuildingManaPipe) {
                                    offset += 8;
                                }
                                rise = (std::max)(rise, offset);
                            }
                        }
                    }
                    if (m_world->isCourtType(info.type)) {
                        float water = (std::max)(m_world->locationWaterWeight(info), 10.0f);
                        plasma::Vector<2, int> dungeon = m_world->dungeonPosition(
                            floorDivide(x, BlocksPerPlace), floorDivide(y, BlocksPerPlace), info.type, info.large);
                        rise = (std::max)(rise,
                                          m_world->monumentTop(x, y, info, dungeon, terrain, water) - floor(terrain));
                    }
                    int ceiling = (int)m_world->height(x, y, elevation, cliffs, river, lavaRiver, poisonRiver,
                                                       m_world->sandAmount(x, y, sample.humidity(), cliffs, desertness),
                                                       road, mountains, &info, HeightFlagSkipPlaceShape);
                    if (top > ceiling) {
                        ceiling = top;
                    }
                    ceiling += rise;
                    if (first) {
                        lowest = top;
                        highest = ceiling;
                        first = false;
                    } else {
                        if (top < lowest) {
                            lowest = top;
                        }
                        if (ceiling > highest) {
                            highest = ceiling;
                        }
                    }
                    heights[i + g_mapRegionBorder +
                            (g_mapRegionBorder + j) * (g_mapRegionSize + 2 * g_mapRegionBorder)] = top;
                }
            }
            region->sprite->resize(g_mapRegionSize + 2 * g_mapRegionBorder, g_mapRegionSize + 2 * g_mapRegionBorder,
                                   (std::min)(255, highest / scale - lowest / scale + 8));
            // Second pass: the model, column by column. Each column is drawn from the lowest of its four neighbours'
            // tops up to its own top, then the structures standing on it.
            float farFade = (std::max)(0.0f, (std::min)(1.0f, (float)(level - 2) * 0.2f));
            for (int i = -g_mapRegionBorder; i < g_mapRegionBorder + g_mapRegionSize; i++) {
                Sleep(0);
                for (int j = -g_mapRegionBorder; j < g_mapRegionBorder + g_mapRegionSize; j++) {
                    int x = mapRegionX * regionSize + i * scale;
                    int y = mapRegionY * regionSize + j * scale;
                    int top = heights[i + g_mapRegionBorder +
                                      (g_mapRegionBorder + j) * (g_mapRegionSize + 2 * g_mapRegionBorder)];
                    // The voxel of the column's top; the structures move it.
                    int z = top / scale - lowest / scale;
                    plasma::Vector<2, int> land = m_world->findRegion(x, y);
                    int biome = m_world->landBiome(land[0], land[1]);
                    // A column on the border of its land is listed under the land (render shows the list as flight
                    // points).
                    if (i >= 0 && i < g_mapRegionSize && j >= 0 && j < g_mapRegionSize) {
                        if (m_world->findRegion(x + scale, y) != land || m_world->findRegion(x, y + scale) != land ||
                            m_world->findRegion(x + scale, y + scale) != land ||
                            m_world->findRegion(x - scale, y) != land || m_world->findRegion(x, y - scale) != land ||
                            m_world->findRegion(x - scale, y - scale) != land) {
                            region->positionsByPlace[std::array<int, 2> {{land[0], land[1]}}].push_back(
                                plasma::Vector<3, int>(x, y, top));
                        }
                    }
                    bool water = false;
                    World::BiomeSample sample;
                    m_world->biomeSample(x, y, sample);
                    World::AreaInfo info;
                    m_world->fillAreaInfo(x, y, info);
                    int bottom = z;
                    if (i > -2) {
                        bottom = (std::min)(
                            bottom, heights[i + g_mapRegionBorder - 1 +
                                            (g_mapRegionBorder + j) * (g_mapRegionSize + 2 * g_mapRegionBorder)] /
                                            scale -
                                        lowest / scale + 1);
                    }
                    if (j > -2) {
                        bottom = (std::min)(
                            bottom, heights[i + g_mapRegionBorder +
                                            (g_mapRegionBorder + j - 1) * (g_mapRegionSize + 2 * g_mapRegionBorder)] /
                                            scale -
                                        lowest / scale + 1);
                    }
                    if (i < g_mapRegionSize + 1) {
                        bottom = (std::min)(
                            bottom, heights[i + g_mapRegionBorder + 1 +
                                            (g_mapRegionBorder + j) * (g_mapRegionSize + 2 * g_mapRegionBorder)] /
                                            scale -
                                        lowest / scale + 1);
                    }
                    if (j < g_mapRegionSize + 1) {
                        bottom = (std::min)(
                            bottom, heights[i + g_mapRegionBorder +
                                            (g_mapRegionBorder + j + 1) * (g_mapRegionSize + 2 * g_mapRegionBorder)] /
                                            scale -
                                        lowest / scale + 1);
                    }
                    BlockProperties<float> block;
                    float road = m_world->streetAmount(x, y, info.type, info.influence);
                    float lava = sample.lavaWeight(info.type, info.influence);
                    float mountains = sample.mountains();
                    float undead = sample.undead(info.type, info.influence);
                    float wetness = sample.wetness();
                    float river = m_world->valleyWater(x, y, road, lava, undead, wetness, mountains, &info);
                    float lavaRiver = m_world->lavaChannels(x, y, road, lava, mountains, &info);
                    float poisonRiver = m_world->poisonChannels(x, y, road, undead, mountains, &info);
                    float elevation = sample.biomeElevation(info.type, info.influence);
                    float desertness = sample.desertness();
                    sample.snow();
                    float temperature = sample.temperature(info.type, info.influence);
                    float humidity = sample.humidity();
                    float cliffs =
                        m_world->mountainCliffs(x, y, elevation, mountains, river, lavaRiver, poisonRiver, road, &info);
                    float sandWeight = m_world->sandAmount(x, y, humidity, cliffs, desertness);
                    float enclave = sample.enclave(info.type, info.influence);
                    float terrain = m_world->height(x, y, elevation, cliffs, river, lavaRiver, poisonRiver, sandWeight,
                                                    road, mountains, &info, 0);
                    float heightWest = m_world->height(x - 1, y, 0);
                    float heightEast = m_world->height(x + 1, y, 0);
                    float heightSouth = m_world->height(x, y - 1, 0);
                    float heightNorth = m_world->height(x, y + 1, 0);
                    if (top <= 0 || river > 0.5f) {
                        // Water (a sea column, or a river in a valley): bluer with depth, greener in the undead lands,
                        // and at the far zoom levels faded towards the ground's colour.
                        float depth = terrain;
                        depth = (std::min)(depth, 200.0f);
                        float shallow = (float)fabs(1.0f - (std::max)(0.0f, (float)fabs(depth) * 0.005f + 1.0f));
                        shallow = std::min(1.0f, shallow);
                        if (top > 0) {
                            float riverDepth = (std::min)(1.0f, (river - 0.5f) * 3.0f);
                            shallow *=
                                riverDepth * 3.0f * riverDepth - (riverDepth + riverDepth) * riverDepth * riverDepth;
                        }
                        block = BlockProperties<float>(
                            plasma::Vector<3, float>(0.0f, 180.0f - shallow * 100.0f, 255.0f), BlockWater);
                        if (depth > 0.0f) {
                            float murk = undead * (std::min)(1.0f, (std::max)(0.0f, depth * 0.01f));
                            block.color[1] = (220.0f - block.color[1]) * murk + block.color[1];
                            block.color[2] = 255.0f - murk * 80.0f;
                            if (scale > 32) {
                                plasma::Vector<3, float> ground;
                                if (m_world->hasLyingSnow(x, y, temperature, heightWest, heightEast, heightSouth,
                                                          heightNorth) != 0.0f) {
                                    ground = m_world->snowColor(x, y);
                                } else {
                                    ground =
                                        m_world
                                            ->surfaceBlock(x, y, top, terrain, heightWest, heightEast, heightSouth,
                                                           heightNorth, cliffs, temperature, humidity, sandWeight, 0.0f,
                                                           road, lava, undead, enclave, lavaRiver, poisonRiver)
                                            .color;
                                }
                                block.color += (ground - block.color) * (std::min)(1.0f, (float)(scale - 32) * 0.0025f);
                            }
                        }
                        water = true;
                    }
                    if (!water || top > 0 && scale >= 128) {
                        z++;
                    }
                    for (int k = bottom; k <= z; k++) {
                        // The block the voxel stands for.
                        int blockZ = top - (z - k) * scale;
                        if (!water) {
                            block = m_world->surfaceBlock(x, y, blockZ, terrain, heightWest, heightEast, heightSouth,
                                                          heightNorth, cliffs, temperature, humidity, sandWeight, 0.0f,
                                                          road, lava, undead, enclave, lavaRiver, poisonRiver);
                            if (cliffs > 0.5f && scale < 128 &&
                                m_world->caveDensity(x, y, info.type, info.influence) > 0.0f) {
                                // Over a cave the surface is blended with the block under it and the cave's cover, as
                                // Zone::generate blends a cave's floor.
                                float caveHeight =
                                    m_world->height(x, y, elevation, cliffs, river, lavaRiver, poisonRiver, sandWeight,
                                                    road, mountains, &info, HeightFlagSkipPlaceAndHills);
                                float blockHeight = (float)blockZ;
                                block = m_world->surfaceBlock(x, y, floor(terrain), terrain, heightWest, heightEast,
                                                              heightSouth, heightNorth, cliffs, temperature, humidity,
                                                              sandWeight, 0.0f, road, lava, undead, enclave, lavaRiver,
                                                              poisonRiver);
                                BlockProperties<float> below = m_world->computeBlock(x, y, (int)(blockHeight - 1.0f));
                                plasma::Vector<3, float> color = below.color;
                                float surfaceBlend =
                                    std::min(1.0f, std::max(0.0f, 1.0f - (terrain - blockHeight - 1.0f) * 0.3f));
                                color += (block.color - color) * surfaceBlend;
                                float caveCoverBlend =
                                    std::min(1.0f, std::max(0.0f, 1.0f - (blockHeight - caveHeight) * 0.3f));
                                BlockProperties<float> cover;
                                if (sandWeight > 0.5f || m_world->isBeach(x, y, blockHeight)) {
                                    cover = m_world->computeBlock(x, y, (int)(blockHeight - 1.0f));
                                } else {
                                    cover = m_world->computeCoverBlock(x, y, (int)(blockHeight - 1.0f));
                                }
                                color += (cover.color - color) * caveCoverBlend;
                                block.color = color;
                            }
                        }
                        region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k, Block(block).color);
                    }
                    if (m_world->isCourtType(info.type)) {
                        // The masonry of a court: from its floor to its top.
                        float base = (std::max)(10.0f, m_world->locationWaterWeight(info));
                        float floorHeight = (float)m_world->masonryFloor(
                            x, y, info,
                            m_world->dungeonPosition(floorDivide(x, BlocksPerPlace), floorDivide(y, BlocksPerPlace),
                                                     info.type, info.large),
                            terrain, base);
                        float topHeight = (float)m_world->monumentTop(
                            x, y, info,
                            m_world->dungeonPosition(floorDivide(x, BlocksPerPlace), floorDivide(y, BlocksPerPlace),
                                                     info.type, info.large),
                            terrain, base);
                        if (topHeight > terrain) {
                            for (int k = floor(floorHeight) / scale - lowest / scale + 1;
                                 k <= floor(topHeight) / scale - lowest / scale + 1; k++) {
                                region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                                         m_world->masonryPavement(x, y, top, info));
                            }
                        }
                    }
                    if (0.5f > road && !water && 0.5f > sandWeight &&
                        m_world->hasLyingSnow(x, y, temperature, heightWest, heightEast, heightSouth, heightNorth) !=
                            0.0f) {
                        region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, z,
                                                 m_world->snowColor(x, y));
                    }
                    if (scale < 2048) {
                        float craterFloor;
                        float craterTop;
                        if (m_world->groundCrater(x, y, info, road, &craterFloor, &craterTop)) {
                            for (int k = floor(craterFloor) / scale - lowest / scale + 1;
                                 k < floor(craterTop) / scale - lowest / scale + 2; k++) {
                                region->sprite->setVoxel(
                                    g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                    m_world
                                        ->columnBlock(x, y, top, cliffs, temperature, humidity, lava, undead, enclave)
                                        .color);
                            }
                        }
                    }
                    // Builds structures around the column according to each zone cell's building type.
                    for (int di = -step; di <= step; di++) {
                        for (int dj = -step; dj <= step; dj++) {
                            int buildingCellX = di + toCell(x);
                            int buildingCellY = toCell(y) + dj;
                            plasma::Vector<2, int> point = m_world->streetPoint(buildingCellX, buildingCellY);
                            World::AreaInfo pointInfo;
                            m_world->fillAreaInfo(point[0], point[1], pointInfo);
                            int type = m_world->streetBuildingType(buildingCellX, buildingCellY, point, pointInfo);
                            int reach = buildingReach(type);
                            if (abs(di) > reach || abs(dj) > reach) {
                                continue;
                            }
                            if (type >= BuildingOak && type <= BuildingPalmPlaza || type == BuildingGreatOak) {
                                // Types 1..8, round about the cell's point (the centre for type 8): a trunk and a
                                // crown in the colours of the vegetation there.
                                if (type == BuildingGreatOak) {
                                    point = plasma::Vector<2, int>(BlocksPerCell * buildingCellX + BlocksPerCell / 2,
                                                                   BlocksPerCell * buildingCellY + BlocksPerCell / 2);
                                }
                                float radius = (float)m_world->townSizeD(buildingCellX, buildingCellY, 1.0f) * 0.75f;
                                if (type == BuildingBirch) {
                                    radius *= 0.75f;
                                } else if (type == BuildingGreatOak) {
                                    radius *= 2.0f;
                                }
                                float dy = (float)(point[1] - y);
                                float dx = (float)(point[0] - x);
                                float squared = dx * dx + dy * dy;
                                float radiusSquared = radius * radius;
                                if (radiusSquared >= squared) {
                                    World::BiomeSample pointSample;
                                    m_world->biomeSample(point[0], point[1], pointSample);
                                    World::AreaInfo treePointInfo;
                                    m_world->fillAreaInfo(point[0], point[1], treePointInfo);
                                    float treePointDesertness = pointSample.desertness();
                                    float treePointLava =
                                        pointSample.lavaWeight(treePointInfo.type, treePointInfo.influence);
                                    float treePointUndead =
                                        pointSample.undead(treePointInfo.type, treePointInfo.influence);
                                    float treePointTemperature =
                                        pointSample.temperature(treePointInfo.type, treePointInfo.influence);
                                    float treePointStreets = m_world->streetAmount(
                                        point[0], point[1], treePointInfo.type, treePointInfo.influence);
                                    float treePointMountains = pointSample.mountains();
                                    float treePointWetness = pointSample.wetness();
                                    float treePointWater = m_world->valleyWater(
                                        point[0], point[1], treePointStreets, treePointLava, treePointUndead,
                                        treePointWetness, treePointMountains, &treePointInfo);
                                    float treePointLavaChannels =
                                        m_world->lavaChannels(point[0], point[1], treePointStreets, treePointLava,
                                                              treePointMountains, &treePointInfo);
                                    float treePointPoisonChannels =
                                        m_world->poisonChannels(point[0], point[1], treePointStreets, treePointUndead,
                                                                treePointMountains, &treePointInfo);
                                    float treePointElevation =
                                        pointSample.biomeElevation(treePointInfo.type, treePointInfo.influence);
                                    float treePointCliffs = m_world->mountainCliffs(
                                        point[0], point[1], treePointElevation, treePointMountains, treePointWater,
                                        treePointLavaChannels, treePointPoisonChannels, treePointStreets,
                                        &treePointInfo);
                                    float treePointHumidity = pointSample.humidity();
                                    float treePointSand = m_world->sandAmount(point[0], point[1], treePointHumidity,
                                                                              treePointCliffs, treePointDesertness);
                                    float ground =
                                        m_world->height(point[0], point[1], treePointElevation, treePointCliffs,
                                                        treePointWater, 0.0f, 0.0f, treePointSand, treePointStreets,
                                                        treePointMountains, &treePointInfo, HeightFlagCaveMouths);
                                    float treePointEnclave =
                                        pointSample.enclave(treePointInfo.type, treePointInfo.influence);
                                    int size =
                                        (int)((float)m_world->townSizeC(buildingCellX, buildingCellY, 1.0f) * 2.0f);
                                    if (type == BuildingGreatOak) {
                                        size *= 2;
                                    }
                                    float normalizedSquaredRadius = squared / radiusSquared;
                                    if (treePointTemperature >= 0.7f && 0.3f > treePointHumidity) {
                                        normalizedSquaredRadius = 1.0f - (1.0f - normalizedSquaredRadius) * 0.5f;
                                    }
                                    int crownBottom;
                                    int crownTop;
                                    if (type == BuildingEvergreen) {
                                        size = (int)((float)size * 1.5f);
                                        crownBottom = (int)(((float)(size / 3) + ground) / (float)scale -
                                                            (float)(lowest / scale));
                                        crownTop = (int)((ground + (float)size -
                                                          sqrtf(normalizedSquaredRadius) * 2.0f * (float)size / 3.0f) /
                                                             (float)scale -
                                                         (float)(lowest / scale));
                                    } else {
                                        float slope = normalizedSquaredRadius * (float)size * 0.25f;
                                        crownBottom = (int)(((float)(size / 2) + ground + slope) / (float)scale -
                                                            (float)(lowest / scale));
                                        crownTop = (int)((ground + (float)size - slope) / (float)scale -
                                                         (float)(lowest / scale));
                                    }
                                    crownBottom++;
                                    crownTop++;
                                    if (type == BuildingPalm || type == BuildingPalmPlaza) {
                                        crownBottom = crownTop - 1;
                                        int offsetX = floorDivide(point[0] - x, scale);
                                        int offsetY = floorDivide(point[1] - y, scale);
                                        if (abs(offsetX) != abs(offsetY) && offsetX != 0 && offsetY != 0) {
                                            crownBottom += crownTop;
                                        }
                                    }
                                    if (type == BuildingJungleTree) {
                                        unsigned int ragged = random(x + 464111, y);
                                        if (ragged % 4 == 0) {
                                            crownBottom -= ragged / 4 % (std::max)(1, size / scale / 2 + 1);
                                        }
                                    }
                                    if (scale > 16) {
                                        crownBottom = (std::max)(z + 1, crownBottom);
                                        crownTop = (std::max)(z + 1, crownTop);
                                    }
                                    float trunk = (std::max)(1.0f, radius * 0.2f);
                                    if (trunk * trunk > squared ||
                                        point / scale == plasma::Vector<2, int>(x, y) / scale) {
                                        // The bark: brown (200, 150, 100), or white for a birch.
                                        unsigned char red = 200;
                                        unsigned char green = 150;
                                        unsigned char blue = 100;
                                        if (type == BuildingBirch) {
                                            red = 255;
                                            green = 255;
                                            blue = 255;
                                        }
                                        for (int k = z + 1; k < crownBottom; k++) {
                                            region->sprite->setVoxel(
                                                g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                                plasma::Vector<3, float>((float)red, (float)green, (float)blue));
                                        }
                                    }
                                    plasma::Vector<3, float> foliageBottom;
                                    plasma::Vector<3, float> foliageTop;
                                    switch (type) {
                                    case BuildingEvergreen:
                                        m_world->groundHashedColors(point[0], point[1], foliageBottom, foliageTop,
                                                                    treePointUndead, treePointEnclave);
                                        break;
                                    case BuildingBirch:
                                        m_world->groundColors(point[0], point[1], foliageBottom, foliageTop,
                                                              treePointTemperature, treePointUndead, treePointEnclave);
                                        break;
                                    default:
                                        m_world->harvestColors(point[0], point[1], foliageBottom, foliageTop,
                                                               treePointTemperature, treePointHumidity, treePointUndead,
                                                               treePointEnclave);
                                        break;
                                    }
                                    plasma::Vector<3, float> crown = foliageBottom * 0.5f + foliageTop * 0.5f;
                                    for (int k = crownBottom; k <= crownTop; k++) {
                                        region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                                                 crown);
                                        if (0.15f > treePointTemperature) {
                                            region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                                                     m_world->snowColor(x, y));
                                        }
                                    }
                                }
                            }
                            if (scale >= 256) {
                                continue;
                            }
                            // The realm and its form decide the colours and styles of the buildings.
                            int realm = 0;
                            int form = 0;
                            if (type != BuildingNone && !isPlainBuilding(type)) {
                                int placeY = World::placeIndex(y);
                                int placeX = World::placeIndex(x);
                                World *world = m_world;
                                realm = world->mortalRealm(placeX, placeY, world->isLargePlace(placeX, placeY));
                                form = Realm(realm).form();
                            }
                            if (type == BuildingTower) {
                                // A round tower with a crown or a dome.
                                int radius = m_world->townSizeB(buildingCellX, buildingCellY);
                                double dx = (double)(BlocksPerCell * buildingCellX) + 0.5 +
                                            (double)(BlocksPerCell / 2) - (double)x;
                                double dy = (double)(BlocksPerCell * buildingCellY) + 0.5 +
                                            (double)(BlocksPerCell / 2) - (double)y;
                                int squared = (int)(dy * dy + dx * dx);
                                if (squared > radius * radius) {
                                    continue;
                                }
                                int height = 2 * radius;
                                int shaft = m_world->townSizeA(buildingCellX, buildingCellY) - height;
                                int level =
                                    m_world->structureLevel(buildingCellX, buildingCellY, BuildingTower) + shaft;
                                int towerTop = level / scale - lowest / scale;
                                if (squared >= (radius - scale) * (radius - scale)) {
                                    // The rim: a battlement of every other voxel in the crowned style.
                                    if (!hasBattlements(BuildingTower, realm, form, biome) ||
                                        modulo(floorDivide(x, scale) + floorDivide(y, scale), 2) == 0) {
                                        towerTop++;
                                    }
                                }
                                plasma::Vector<3, float> wall =
                                    buildingColor(BuildingTower, buildingCellX, buildingCellY, realm, form, biome);
                                for (int k = z + 1; k <= towerTop; k++) {
                                    region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k, wall);
                                }
                                if (!hasRoof(BuildingTower, realm, form, biome)) {
                                    continue;
                                }
                                plasma::Vector<3, float> roof =
                                    roofColor(BuildingTower, buildingCellX, buildingCellY, realm, form, biome);
                                int roofTop = (int)(((1.0f - sqrtf((float)squared) / (float)radius) * (float)height +
                                                     (float)level) /
                                                        (float)scale -
                                                    (float)(lowest / scale));
                                for (int k = towerTop; k <= roofTop; k++) {
                                    region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k, roof);
                                }
                                continue;
                            }
                            if (type == BuildingManaPump) {
                                // A round platform with a pale core.
                                float dx = (float)(BlocksPerCell * buildingCellX - x + BlocksPerCell / 2);
                                float dy = (float)(BlocksPerCell * buildingCellY - y + BlocksPerCell / 2);
                                int squared = (int)(dy * dy + dx * dx);
                                if (squared <= 1024) {
                                    int platformTop =
                                        (m_world->structureLevel(buildingCellX, buildingCellY, type) + 10) / scale -
                                        lowest / scale;
                                    if (platformTop < z) {
                                        platformTop = z;
                                    }
                                    plasma::Vector<3, float> color =
                                        buildingColor(type, buildingCellX, buildingCellY, realm, form, biome);
                                    if (squared <= 64) {
                                        platformTop++;
                                        color = plasma::Vector<3, float>(140.0f, 150.0f, 180.0f);
                                    }
                                    for (int k = z; k <= platformTop; k++) {
                                        region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                                                 color);
                                    }
                                }
                            }
                            if (type == BuildingSanctuary || type == BuildingRaisedSanctuary) {
                                // A realm's tower: a disc with merlons on the diagonals and the axes, grown over at
                                // the top.
                                int radius = realmTowerRadius(realm);
                                if (type == BuildingRaisedSanctuary) {
                                    radius = 28;
                                }
                                float dx = (float)(BlocksPerCell * buildingCellX - x + BlocksPerCell / 2);
                                float dy = (float)(BlocksPerCell * buildingCellY - y + BlocksPerCell / 2);
                                if ((int)(dy * dy + dx * dx) <= radius * radius) {
                                    int rise = type == BuildingRaisedSanctuary ? 10 : 3;
                                    int towerLevel = m_world->structureLevel(buildingCellX, buildingCellY, type) + rise;
                                    bool merlon = false;
                                    if (scale < 16) {
                                        int offsetX =
                                            abs(floorDivide(x, scale) -
                                                floorDivide(BlocksPerCell * buildingCellX + BlocksPerCell / 2, scale));
                                        int offsetY =
                                            abs(floorDivide(y, scale) -
                                                floorDivide(BlocksPerCell * buildingCellY + BlocksPerCell / 2, scale));
                                        if (offsetX == (int)((float)radius * 0.7f) / scale &&
                                                offsetY == (int)((float)radius * 0.7f) / scale ||
                                            offsetX == (radius - 1) / scale && offsetY == 0 ||
                                            offsetX == 0 && offsetY == (radius - 1) / scale) {
                                            towerLevel += 15;
                                            if (hasTowerBand(type, realm, form, biome)) {
                                                towerLevel = (std::max)(
                                                    towerLevel - 15,
                                                    (std::min)(
                                                        towerLevel,
                                                        (int)(m_world->roughness(x, y, rise + 10, 10) + terrain)));
                                            }
                                            merlon = true;
                                        }
                                    }
                                    int towerTop = towerLevel / scale - lowest / scale + 1;
                                    plasma::Vector<3, float> wall =
                                        buildingColor(type, buildingCellX, buildingCellY, realm, form, biome);
                                    for (int k = z; k <= towerTop; k++) {
                                        // The trim grows in patches: the noise of StructureBuilder's CoverPatches.
                                        if (k == towerTop && !merlon && hasTowerTrim(type, realm, form, biome) &&
                                            noise(x * 0.06 + 7631.0, y * 0.06 + 983.0) > 0.0f) {
                                            region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                                                     m_world
                                                                         ->coverBlock(x, y, k, terrain, temperature,
                                                                                      humidity, 0.0f, 0.0f, road, lava,
                                                                                      undead, enclave)
                                                                         .color);
                                        } else {
                                            region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                                                     wall);
                                        }
                                    }
                                }
                            }
                            if (type == BuildingManaPipe) {
                                // A bridge: the line through the middle of the cell towards a neighbouring bridge
                                // cell.
                                if (scale < 32) {
                                    int cellPartX = modulo(floorDivide(x, scale), 16 / scale);
                                    int cellPartY = modulo(floorDivide(y, scale), 16 / scale);
                                    if (cellPartY == 16 / scale / 2 &&
                                            (m_world->zoneBuildingType(buildingCellX - 1, buildingCellY) ==
                                                 BuildingManaPipe ||
                                             m_world->zoneBuildingType(buildingCellX + 1, buildingCellY) ==
                                                 BuildingManaPipe) ||
                                        cellPartX == 16 / scale / 2 &&
                                            (m_world->zoneBuildingType(buildingCellX, buildingCellY - 1) ==
                                                 BuildingManaPipe ||
                                             m_world->zoneBuildingType(buildingCellX, buildingCellY + 1) ==
                                                 BuildingManaPipe)) {
                                        // A checker of 90% and 100% of the colour.
                                        plasma::Vector<3, float> color =
                                            buildingColor(BuildingManaPipe, buildingCellX, buildingCellY, realm, form,
                                                          biome) *
                                            ((float)((cellPartY + cellPartX) % 2) * 0.1f + 0.9f);
                                        region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, z + 1,
                                                                 color);
                                    }
                                }
                            } else if (type >= BuildingPumpkinField && type <= BuildingKaleField && 0.5f >= road &&
                                       0.5f >= river) {
                                // A field: striped ground (75% and 80% of the ground colour), the stripes turned from
                                // plot (3 x 3 cells) to plot, and crops on three in four of every third voxel.
                                int stripe;
                                if (modulo(floorDivide(buildingCellY, 3) + floorDivide(buildingCellX, 3), 2) != 0) {
                                    stripe = x;
                                } else {
                                    stripe = y;
                                }
                                plasma::Vector<3, float> color =
                                    m_world->groundColor(x, y, top) * ((float)(stripe / scale % 2) * 0.05f + 0.75f);
                                region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, z, color);
                                if (modulo(floorDivide(x, scale), 3) == 1 && modulo(floorDivide(y, scale), 3) == 1 &&
                                    random(x, y) % 4 != 0) {
                                    z++;
                                    region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, z,
                                                             plasma::Vector<3, float>(100.0f, 200.0f, 100.0f));
                                }
                            }
                            if (type == BuildingBuriedTreasureChamber) {
                                // A round patch of rough ground.
                                if (top > 0) {
                                    int centerX = BlocksPerCell * buildingCellX + BlocksPerCell / 2;
                                    int centerY = BlocksPerCell * buildingCellY + BlocksPerCell / 2;
                                    if ((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY) < 256) {
                                        float shade = (float)fabs(noise(x * 0.25, y * 0.25));
                                        plasma::Vector<3, float> color =
                                            m_world->computeBlock(x, y, top).color * (shade * 0.3f + 0.7f);
                                        region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, z,
                                                                 color);
                                    }
                                }
                            } else if ((type == BuildingTreasurePavilion || type == BuildingHouseCourtyard ||
                                        type == BuildingRuinedFlatRoofPavilion || type == BuildingBirdStatuePavilion ||
                                        type == BuildingInscribedMenhir || type == BuildingAltar ||
                                        type == BuildingMourningStatuePavilion || type == BuildingLakePlatform ||
                                        type == BuildingRaisedPostPlatform || type == BuildingMountainPlatform) &&
                                       di == 0 && dj == 0) {
                                // A low building filling its cell: a wall up to its level, towers on the corners of
                                // some types, only corner posts for types 63..65.
                                int level = m_world->structureLevel(buildingCellX, buildingCellY, type);
                                plasma::Vector<3, float> wall =
                                    buildingColor(type, buildingCellX, buildingCellY, realm, form, biome);
                                int wallTop = (level + 1) / scale - lowest / scale;
                                z--;
                                bool tower = false;
                                if (scale <= 16) {
                                    int cellPartX = modulo(floorDivide(x, scale), 16 / scale);
                                    int cellPartY = modulo(floorDivide(y, scale), 16 / scale);
                                    if ((type == BuildingTreasurePavilion || type == BuildingRuinedFlatRoofPavilion ||
                                         type == BuildingBirdStatuePavilion || type == BuildingAltar ||
                                         type == BuildingMourningStatuePavilion) &&
                                        (cellPartX == 0 || cellPartX == 16 / scale - 1) &&
                                        (cellPartY == 0 || cellPartY == 16 / scale - 1)) {
                                        int towerLevel = level + 10;
                                        if (hasTowerBand(type, realm, form, biome)) {
                                            towerLevel = (std::min)(towerLevel,
                                                                    (int)(m_world->roughness(x, y, 10, 10) + terrain));
                                        }
                                        wallTop = towerLevel / scale - lowest / scale + 1;
                                        tower = true;
                                    }
                                    if (type >= BuildingLakePlatform && type <= BuildingMountainPlatform &&
                                        !((cellPartX == 0 || cellPartX == 16 / scale - 1) &&
                                          (cellPartY == 0 || cellPartY == 16 / scale - 1))) {
                                        z = wallTop - 1;
                                    }
                                }
                                while (z < wallTop) {
                                    z++;
                                    region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, z, wall);
                                }
                                // The trim, in patches of noise (frequency 0.1) above 0.25.
                                if (!tower && hasTowerTrim(type, realm, form, biome) &&
                                    noise(x * 0.1 + 89834.0, y * 0.1 + 12233.0) > 0.25f) {
                                    region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, wallTop,
                                                             m_world
                                                                 ->coverBlock(x, y, level, terrain, temperature,
                                                                              humidity, 0.0f, 0.0f, road, lava, undead,
                                                                              enclave)
                                                                 .color);
                                }
                            }
                            if (type == BuildingInscribedMenhir) {
                                // A boulder over the cell's centre.
                                float dy = (float)(BlocksPerCell * buildingCellY + BlocksPerCell / 2 - y) - 0.5f;
                                float dx = (float)(BlocksPerCell * buildingCellX + BlocksPerCell / 2 - x) - 0.5f;
                                float squared = dy * dy + dx * dx;
                                if (squared <= 25.0f || scale > 8) {
                                    float bulge = 1.0f - squared / 25.0f * (squared / 25.0f);
                                    int rockTop = (int)((bulge * 20.0f +
                                                         (float)m_world->structureLevel(buildingCellX, buildingCellY,
                                                                                        BuildingInscribedMenhir)) /
                                                            (float)scale -
                                                        (float)(lowest / scale)) +
                                                  2;
                                    for (int k = z; k <= rockTop; k++) {
                                        region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                                                 m_world->rockColor(x, y, k * scale + lowest,
                                                                                    temperature, humidity, lava,
                                                                                    undead));
                                    }
                                }
                            } else if (type == BuildingMenhir) {
                                // A larger one over the cell's street point.
                                float dy = (float)(point[1] - y) - 0.5f;
                                float dx = (float)(point[0] - x) - 0.5f;
                                float squared = dy * dy + dx * dx;
                                if (squared <= 64.0f || scale > 8) {
                                    float bulge = 1.0f - squared / 64.0f * (squared / 64.0f);
                                    int rockTop =
                                        (int)((bulge * 25.0f + (float)m_world->structureLevel(
                                                                   buildingCellX, buildingCellY, BuildingMenhir)) /
                                                  (float)scale -
                                              (float)(lowest / scale)) +
                                        2;
                                    for (int k = z; k <= rockTop; k++) {
                                        region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                                                 m_world->rockColor(x, y, k * scale + lowest,
                                                                                    temperature, humidity, lava,
                                                                                    undead));
                                    }
                                }
                            }
                            if ((hasRoof(type, realm, form, biome) || isCastleBuilding(type) ||
                                 isDungeonBuilding(type)) &&
                                di == 0 && dj == 0 && type != BuildingFloatingIslandTreasurePavilion) {
                                // A house, a castle part or a dungeon entrance: walls up to the building's height
                                // (open inside for the hollow types), battlements, and a roof.
                                int level = m_world->structureLevel(buildingCellX, buildingCellY, type);
                                int height = buildingHeight(type);
                                plasma::Vector<3, float> wall =
                                    buildingColor(type, buildingCellX, buildingCellY, realm, form, biome);
                                int wallLevel = height + level;
                                bool band = hasTowerBand(type, realm, form, biome);
                                if (band) {
                                    wallLevel =
                                        (std::min)(wallLevel, (int)(m_world->roughness(x, y, 10, 10) + terrain));
                                }
                                int wallTop = wallLevel / scale - lowest / scale;
                                if (scale <= 16) {
                                    int cellPartX = modulo(floorDivide(x, scale), 16 / scale);
                                    int cellPartY = modulo(floorDivide(y, scale), 16 / scale);
                                    if ((type == BuildingRuinedPavilion || type == BuildingShrineOfLife ||
                                         type == BuildingSewerOrGrottoEntrance || type == BuildingTempleEntrance ||
                                         type == BuildingRockLorePavilion) &&
                                        !((cellPartX == 0 || cellPartX == 16 / scale - 1) &&
                                          (cellPartY == 0 || cellPartY == 16 / scale - 1))) {
                                        wallTop = level / scale - lowest / scale + 1;
                                    }
                                }
                                while (z <= wallTop) {
                                    z++;
                                    region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, z, wall);
                                }
                                z = (height + level) / scale - lowest / scale;
                                if (scale < 16 && hasBattlements(type, realm, form, biome)) {
                                    // Merlons on every other voxel of the sides that join no other castle part.
                                    int cellPartX = modulo(floorDivide(x, scale), 16 / scale);
                                    bool merlon =
                                        modulo(floorDivide(y, scale), 2) == 0 &&
                                        (cellPartX == 0 &&
                                             !isJoinedCastle(
                                                 type, m_world->zoneBuildingType(buildingCellX - 1, buildingCellY)) ||
                                         cellPartX == 16 / scale - 1 &&
                                             !isJoinedCastle(
                                                 type, m_world->zoneBuildingType(buildingCellX + 1, buildingCellY)));
                                    if (!merlon) {
                                        // Does not round y down here.
                                        int cellPartY = modulo(y / scale, 16 / scale);
                                        merlon = modulo(floorDivide(x, scale), 2) == 0 &&
                                                 (cellPartY == 0 &&
                                                      !isJoinedCastle(type, m_world->zoneBuildingType(
                                                                                buildingCellX, buildingCellY - 1)) ||
                                                  cellPartY == 16 / scale - 1 &&
                                                      !isJoinedCastle(type, m_world->zoneBuildingType(
                                                                                buildingCellX, buildingCellY + 1)));
                                    }
                                    if (merlon) {
                                        z++;
                                        region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, z, wall);
                                    }
                                }
                                if (hasRoof(type, realm, form, biome)) {
                                    unsigned int sides = m_world->streetSides(buildingCellX, buildingCellY);
                                    m_world->adjustOpenSides(buildingCellX, buildingCellY, type, &sides);
                                    int half = scale / 2;
                                    int roofLevel = m_world->roofHeight(x + half - BlocksPerCell * buildingCellX,
                                                                        y + half - BlocksPerCell * buildingCellY, type,
                                                                        sides, houseStyle(realm, form, biome)) +
                                                    height + level;
                                    if (band) {
                                        roofLevel =
                                            (std::min)(roofLevel, (int)(m_world->roughness(x, y, 10, 10) + terrain));
                                    }
                                    int roofTop = roofLevel / scale - lowest / scale + 1;
                                    plasma::Vector<3, float> roof =
                                        roofColor(type, buildingCellX, buildingCellY, realm, form, biome);
                                    while (z < roofTop) {
                                        z++;
                                        region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, z, roof);
                                    }
                                }
                            }
                            if (type == BuildingGraveyard) {
                                // A graveyard: a stone on every fourth voxel.
                                if (di == 0 && dj == 0 && 0.45f > road && modulo(floorDivide(x, scale), 4) == 2 &&
                                    modulo(floorDivide(y, scale), 4) == 2 && random(x, y) % 10 != 0) {
                                    z++;
                                    region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, z,
                                                             plasma::Vector<3, float>(100.0f, 100.0f, 100.0f));
                                }
                            } else if (type == BuildingBoulder || type == BuildingHill || type == BuildingCave ||
                                       type == BuildingRock ||
                                       type >= BuildingTallRock && type <= BuildingRockTreasure) {
                                // A rock or a hill around the cell's point (the cell's centre for types 11..14).
                                if (type >= BuildingRock && type <= BuildingRockTreasure) {
                                    point = plasma::Vector<2, int>(BlocksPerCell * buildingCellX + BlocksPerCell / 2,
                                                                   BlocksPerCell * buildingCellY + BlocksPerCell / 2);
                                }
                                float radius = (float)m_world->townSizeD(buildingCellX, buildingCellY, 1.0f);
                                float size = (float)m_world->townSizeC(buildingCellX, buildingCellY, 1.0f);
                                if (type == BuildingRock || type == BuildingRockLorePavilion) {
                                    radius = 32.0f;
                                    size = 40.0f;
                                }
                                if (type == BuildingTallRock || type == BuildingRockTreasure) {
                                    radius = 32.0f;
                                    size = 80.0f;
                                }
                                float dy = (float)(point[1] - y);
                                float dx = (float)(point[0] - x);
                                if (radius * radius < dx * dx + dy * dy) {
                                    continue;
                                }
                                World::BiomeSample hillSample;
                                m_world->biomeSample(point[0], point[1], hillSample);
                                World::AreaInfo hillInfo;
                                m_world->fillAreaInfo(point[0], point[1], hillInfo);
                                float hillDesertness = hillSample.desertness();
                                float hillLava = hillSample.lavaWeight(hillInfo.type, hillInfo.influence);
                                float hillUndead = hillSample.undead(hillInfo.type, hillInfo.influence);
                                hillSample.temperature(hillInfo.type, hillInfo.influence);
                                float hillStreets =
                                    m_world->streetAmount(point[0], point[1], hillInfo.type, hillInfo.influence);
                                float hillMountains = hillSample.mountains();
                                float hillWetness = hillSample.wetness();
                                float hillWater =
                                    m_world->valleyWater(point[0], point[1], hillStreets, hillLava, hillUndead,
                                                         hillWetness, hillMountains, &hillInfo);
                                float hillLavaChannels = m_world->lavaChannels(point[0], point[1], hillStreets,
                                                                               hillLava, hillMountains, &hillInfo);
                                float hillPoisonChannels = m_world->poisonChannels(
                                    point[0], point[1], hillStreets, hillUndead, hillMountains, &hillInfo);
                                float hillElevation = hillSample.biomeElevation(hillInfo.type, hillInfo.influence);
                                float hillCliffs = m_world->mountainCliffs(point[0], point[1], hillElevation,
                                                                           hillMountains, hillWater, hillLavaChannels,
                                                                           hillPoisonChannels, hillStreets, &hillInfo);
                                float hillHumidity = hillSample.humidity();
                                float hillSand =
                                    m_world->sandAmount(point[0], point[1], hillHumidity, hillCliffs, hillDesertness);
                                float ground =
                                    (std::max)(0.0f, m_world->height(point[0], point[1], hillElevation, hillCliffs,
                                                                     hillWater, 0.0f, 0.0f, hillSand, hillStreets,
                                                                     hillMountains, &hillInfo, 0));
                                if (type == BuildingBoulder) {
                                    // A rock: an ellipsoid on the ground.
                                    int rockTop = (int)((ground + radius) / (float)scale - (float)(lowest / scale)) + 1;
                                    for (int k = z; k <= rockTop; k++) {
                                        int blockZ = k * scale + lowest;
                                        if (ellipsoidContains(x, y, blockZ, point[0], point[1], (int)ground,
                                                              (int)radius, (int)radius, (int)size)) {
                                            region->sprite->setVoxel(
                                                g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                                m_world->rockColor(x, y, blockZ, temperature, humidity, lava, undead));
                                        }
                                    }
                                    continue;
                                }
                                // The top noise of CaveBuilder's mounds.
                                int hillTop =
                                    (int)((noise(x * 0.01 + 4394.0, y * 0.01 + 8974.0) * 10.0f + (ground + size)) /
                                              (float)scale -
                                          (float)(lowest / scale)) +
                                    1;
                                if (hillCliffs > 0.5f || type >= BuildingRock && type <= BuildingRockTreasure) {
                                    for (int k = z; k < hillTop; k++) {
                                        region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                                                 m_world->rockColor(x, y, k * scale + lowest,
                                                                                    temperature, humidity, lava,
                                                                                    undead));
                                    }
                                } else {
                                    for (int k = z; k < hillTop; k++) {
                                        region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                                                 m_world->earthColor(x, y, k * scale + lowest,
                                                                                     temperature, humidity, lava,
                                                                                     undead, enclave));
                                    }
                                }
                                if (m_world->hasLyingSnow(x, y, temperature, 0.0f, 0.0f, 0.0f, 0.0f) != 0.0f) {
                                    region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, hillTop,
                                                             m_world->snowColor(x, y));
                                } else if (0.5f > sandWeight) {
                                    region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, hillTop,
                                                             m_world
                                                                 ->coverBlock(x, y, top, terrain, temperature, humidity,
                                                                              0.0f, 0.0f, road, lava, undead, enclave)
                                                                 .color);
                                }
                            }
                        }
                    }
                    // Everything above the ground is tinted by the height of the terrain, more at the far zoom levels.
                    if (top > 0) {
                        int k = top / scale - lowest / scale;
                        if (water) {
                            k++;
                        }
                        for (; k < region->sprite->sizeZ(); k++) {
                            if (region->sprite->voxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k) !=
                                plasma::Vector<3, unsigned char>(0, 0, 0)) {
                                plasma::Vector<3, float> color(
                                    region->sprite->voxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k));
                                plasma::Vector<3, float> tinted =
                                    heightTint(color, scale, (int)terrain, 0, 500, temperature, humidity, river);
                                plasma::Vector<3, float> mixed = farFade * tinted + (1.0f - farFade) * color;
                                region->sprite->setVoxel(g_mapRegionBorder + i, g_mapRegionBorder + j, k,
                                                         plasma::Vector<3, unsigned char>(mixed));
                            }
                        }
                    }
                }
            }
            // Third pass: the voxels under an overhang are darkened.
            for (int vx = 0; vx < region->sprite->sizeX(); vx++) {
                Sleep(0);
                for (int vy = 0; vy < region->sprite->sizeY(); vy++) {
                    bool open = true;
                    bool covered = false;
                    for (int vz = region->sprite->sizeZ() - 1; vz >= 0; vz--) {
                        plasma::Vector<3, unsigned char> color = region->sprite->voxel(vx, vy, vz);
                        if (color == plasma::Vector<3, unsigned char>(0, 0, 0)) {
                            if (!open) {
                                covered = true;
                            }
                        } else {
                            if (covered) {
                                plasma::Vector<3, unsigned char> shaded = color;
                                shaded /= 2;
                                region->sprite->setVoxel(vx, vy, vz, shaded);
                            }
                            open = false;
                        }
                    }
                }
            }
            region->sprite->updateMesh(g_mapRegionBorder);
            region->baseHeight = lowest / scale;
            // The markers. The places (1024-block cells, or 2048 for a large place) whose centre lies in the region,
            // with their mission; at the far levels only the kind 14 ones.
            if (scale <= 256) {
                int firstX = floorDivide(mapRegionX * regionSize, BlocksPerPlace);
                int lastX = floorDivide((mapRegionX + 1) * regionSize, BlocksPerPlace);
                int firstY = floorDivide(mapRegionY * regionSize, BlocksPerPlace);
                int lastY = floorDivide((mapRegionY + 1) * regionSize, BlocksPerPlace);
                std::set<std::array<int, 2>> listed;
                for (int cellPlaceX = firstX; cellPlaceX <= lastX; cellPlaceX++) {
                    for (int cellPlaceY = firstY; cellPlaceY <= lastY; cellPlaceY++) {
                        int placeX = cellPlaceX;
                        int placeY = cellPlaceY;
                        World *world = m_world;
                        bool large = world->isLargePlace(cellPlaceX, cellPlaceY);
                        if (large) {
                            placeX = 2 * floorDivide(cellPlaceX, 2);
                            placeY = 2 * floorDivide(cellPlaceY, 2);
                        }
                        int type = world->locationType(placeX, placeY, large);
                        if (type == PlaceNone) {
                            continue;
                        }
                        plasma::Vector<2, int> center = m_world->placeCenter(placeX, placeY, large);
                        if (center[0] < mapRegionX * regionSize || center[0] >= (mapRegionX + 1) * regionSize ||
                            center[1] < mapRegionY * regionSize || center[1] >= (mapRegionY + 1) * regionSize) {
                            continue;
                        }
                        if (listed.find(std::array<int, 2> {placeX, placeY}) != listed.end()) {
                            continue;
                        }
                        MapMission place;
                        place.type = type;
                        place.position =
                            plasma::Vector<3, int>(center[0], center[1], (int)m_world->height(center[0], center[1], 0));
                        Missions missions(m_world, m_world->findRegion(center[0], center[1]));
                        place.kind = static_cast<MissionKind>(missions.kindAt(placeX, placeY));
                        place.level = missions.requestCount(placeX, placeY);
                        place.item = missions.artifactAt(placeX, placeY);
                        if (place.kind == MissionKindArtifactReward || scale <= 64) {
                            region->missions.push_back(place);
                            listed.insert(std::array<int, 2> {placeX, placeY});
                        }
                    }
                }
            }
            // The name of the land (BlocksPerLand cells), at its centre.
            if (scale >= 128) {
                int landX = floorDivide(mapRegionX * regionSize, BlocksPerLand);
                int landY = floorDivide(mapRegionY * regionSize, BlocksPerLand);
                int landBiome = m_world->landBiome(landX, landY);
                plasma::Vector<2, int> center = m_world->landCenter(landX, landY);
                if (center[0] >= mapRegionX * regionSize && center[0] < (mapRegionX + 1) * regionSize &&
                    center[1] >= mapRegionY * regionSize && center[1] < (mapRegionY + 1) * regionSize) {
                    MapMission area;
                    area.type = landBiome;
                    area.position =
                        plasma::Vector<3, int>(center[0], center[1], (int)m_world->height(center[0], center[1], 0));
                    region->areas.push_back(area);
                }
            }
            // The flight point of the 2048-block cell: a hashed block in it (the salts 292054 and 98800 draw x and y
            // independently).
            if (scale <= 256) {
                int flightCellX = floorDivide(mapRegionX * regionSize, 2048);
                int flightCellY = floorDivide(mapRegionY * regionSize, 2048);
                int flightX = random(flightCellX + 292054, flightCellY) % 2048 + flightCellX * 2048;
                int flightY = random(flightCellX + 98800, flightCellY) % 2048 + flightCellY * 2048;
                if (flightX >= mapRegionX * regionSize && flightX < (mapRegionX + 1) * regionSize &&
                    flightY >= mapRegionY * regionSize && flightY < (mapRegionY + 1) * regionSize) {
                    MapMission flightPoint;
                    flightPoint.type = 0;
                    flightPoint.position =
                        plasma::Vector<3, int>(flightX, flightY, (int)m_world->height(flightX, flightY, 0));
                    region->flightPoints.push_back(flightPoint);
                }
            }
            // The type 53 building of each 1024-block cell (marker 12).
            if (scale <= 512) {
                for (int placeX = floorDivide(mapRegionX * regionSize, BlocksPerPlace);
                     placeX <= floorDivide((mapRegionX + 1) * regionSize, BlocksPerPlace); placeX++) {
                    for (int placeY = floorDivide(mapRegionY * regionSize, BlocksPerPlace);
                         placeY <= floorDivide((mapRegionY + 1) * regionSize, BlocksPerPlace); placeY++) {
                        plasma::Vector<2, int> site = m_world->siteInZone(placeX, placeY);
                        plasma::Vector<2, int> block = site * 16;
                        int blockX = block[0] + 8;
                        int blockY = block[1] + 8;
                        if (blockX >= mapRegionX * regionSize && blockX < (mapRegionX + 1) * regionSize &&
                            blockY >= mapRegionY * regionSize && blockY < (mapRegionY + 1) * regionSize &&
                            m_world->zoneBuildingType(site[0], site[1]) == BuildingShrineOfLife) {
                            MapMission place;
                            place.type = PlotMarkerShrine;
                            place.position =
                                plasma::Vector<3, int>(blockX, blockY,
                                                       m_world->structureLevel(floorDivide(blockX, BlocksPerCell),
                                                                               floorDivide(blockY, BlocksPerCell), 0));
                            region->places.push_back(place);
                        }
                    }
                }
            }
            // The lore objects (marker 14) and mission cells (15).
            if (scale <= 64) {
                for (int buildingCellX = floorDivide(mapRegionX * regionSize, 16);
                     buildingCellX <= floorDivide((mapRegionX + 1) * regionSize, 16); buildingCellX++) {
                    for (int buildingCellY = floorDivide(mapRegionY * regionSize, 16);
                         buildingCellY <= floorDivide((mapRegionY + 1) * regionSize, 16); buildingCellY++) {
                        int blockX = BlocksPerCell * buildingCellX + BlocksPerCell / 2;
                        int blockY = BlocksPerCell * buildingCellY + BlocksPerCell / 2;
                        if (blockX < mapRegionX * regionSize || blockX >= (mapRegionX + 1) * regionSize ||
                            blockY < mapRegionY * regionSize || blockY >= (mapRegionY + 1) * regionSize) {
                            continue;
                        }
                        if (m_world->hasLore(buildingCellX, buildingCellY)) {
                            MapMission place;
                            place.type = PlotMarkerCharacter;
                            place.position = plasma::Vector<3, int>(
                                blockX, blockY, m_world->structureLevel(buildingCellX, buildingCellY, 0));
                            region->places.push_back(place);
                        } else if (m_world->hasMission(buildingCellX, buildingCellY)) {
                            MapMission place;
                            place.type = PlotMarkerTreasure;
                            place.position = plasma::Vector<3, int>(
                                blockX, blockY, m_world->structureLevel(buildingCellX, buildingCellY, 0));
                            region->places.push_back(place);
                        }
                    }
                }
            }
            // The structure cells (render colours them by kind) and, at the near levels, the hamlets' plots (markers
            // 2..12).
            if (scale < 64) {
                for (int a = 0; a < regionSize / 16; a++) {
                    Sleep(0);
                    for (int b = 0; b < regionSize / 16; b++) {
                        int buildingCellX = a + floorDivide(mapRegionX * regionSize, 16);
                        int buildingCellY = b + floorDivide(mapRegionY * regionSize, 16);
                        StructureCell structure = m_world->structureCell(buildingCellX, buildingCellY, true);
                        if (structure.kind != StructureCell_None) {
                            MapCell site;
                            site.structure = structure;
                            site.dungeonZLevel = m_world->dungeonZLevel(buildingCellX, buildingCellY);
                            site.position =
                                plasma::Vector<2, int>(BlocksPerCell * buildingCellX, BlocksPerCell * buildingCellY);
                            region->sites.push_back(site);
                        }
                        if (scale > 16) {
                            continue;
                        }
                        int type = m_world->zoneBuildingType(buildingCellX, buildingCellY);
                        if (!(type >= BuildingHouseRoom && type <= BuildingHouseCourtyard ||
                              type == BuildingGuildHallCentre || type == BuildingFlightMasterSite)) {
                            continue;
                        }
                        // Once per house: at the cell that is its corner.
                        if (plasma::Vector<2, int>(buildingCellX, buildingCellY) !=
                            m_world->houseCorner(floorDivide(buildingCellX, 3), floorDivide(buildingCellY, 3))) {
                            continue;
                        }
                        int blockX = BlocksPerCell * buildingCellX;
                        int blockY = BlocksPerCell * buildingCellY;
                        int placeX = floorDivide(blockX, BlocksPerPlace);
                        int placeY = floorDivide(blockY, BlocksPerPlace);
                        World *world = m_world;
                        bool large = world->isLargePlace(placeX, placeY);
                        int location = world->locationType(placeX, placeY, large);
                        plasma::Vector<2, int> dungeon(0, 0);
                        if (m_world->dungeonType(placeX, placeY, location, large) != PlaceNone) {
                            dungeon = m_world->dungeonPosition(placeX, placeY, location, large);
                        }
                        int plot = m_world->hamletPlot(buildingCellX, buildingCellY, location, dungeon);
                        if (plot != PlotNone && plot != PlotHouse && plot != PlotField) {
                            plasma::Vector<2, int> block(blockX + 8, blockY + 8);
                            int blockZ = (int)(m_world->height(block[0], block[1], 0) + 20.0f);
                            MapMission place;
                            place.position = plasma::Vector<3, int>(block[0], block[1], blockZ);
                            place.type = plot;
                            region->places.push_back(place);
                        }
                    }
                }
            }
            region->generationTime = timeGetTime() - region->cacheTimes[1];
            // The finished region is published under the regions lock.
            EnterCriticalSection(&m_regionsCriticalSection);
            m_regions[std::array<int, 3> {{mapRegionX, mapRegionY, level}}] = region;
            LeaveCriticalSection(&m_regionsCriticalSection);
        });
    return true;
}

}
