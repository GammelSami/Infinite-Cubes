#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include "cube/WorldIds.h"
#include "cube/MathConstants.h"
#include "gfx/RendererIds.h"
#include "cube/RendererIds.h"
#include <math.h>
#include <algorithm>
#include <utility>
#include "gfx/Renderer.h"
#include "gfx/ByteGrid.h"
#include "gfx/Graphics.h"
#include "gfx/IndexBuffer.h"
#include "gfx/VertexBuffer.h"
#include "cube/Fixed.h"
#include "cube/Game.h"
#include "cube/Sprite.h"
#include "cube/Item.h"
#include "cube/SpriteManager.h"
#include "cube/Noise.h"
#include "cube/World.h"
#include "cube/WorldMap.h"
#include "cube/Zone.h"
#include "cube/CreatureIds.h"
#include "cube/BlockIds.h"
#include "cube/ItemIds.h"
#include "cube/ServerUpdateIds.h"
#include "cube/ZoneIds.h"
#include "cube/SpriteIds.h"
#include "plasma/EditFilter.h"

namespace gfx {

static const float DistanceFadeStartFraction = 0.9f;
static const float DistanceFadeWidthFraction = 0.1f;
static const float ZoneCullMarginBlocks = 64.0f;
static const int GroundItemLightElevation = 3;
static const float GroundItemLightRadius = 16.0f;
static const float PlantLightVisibilityRadius = 16.0f;
static const float LampDaylightCutoff = 0.4f;
static const float ComboLightRadius = 10.0f;
static const float RightWeaponComboLightGain = 0.3f;
static const float LeftWeaponComboLightGain = 0.5f;
static const float ArmorComboLightGain = 0.1f;
static const float TreasureSpiritScale = 0.04f;
static const float DebugBlockScale = 0.4f;
static const float CreatureLightSampleDownwardBias = 0.1f;
static const int ChargedSpellImpactSteps = 80;
static const float OverheadSignWidth = 0.8f;
static const float DroppedItemYTumbleGain = 0.7f;
static const float LookedAtItemTintIncrement = 0.25f;
static const float ProjectileSpriteSizeDivisor = 14.0f;
static const float CreaturePreviewDepth = 17.0f;
static const float PreviewFogDistance = 10000.0f;
static const float ImpactLengthSquaredEpsilon = 0.00001f;
static const float ChestPreviewSizeDivisorGain = 1.5f;
static const float StaticEntityOpeningRate = 0.001f;
static const float StreetlightDaylightCutoff = 0.4f;
static const float DarkCloudUndeadThreshold = 0.3f;
static const float VoxelAmbientExposurePower = 1.2f;
static const float MapPitchClamp = -120.0f;
static const float MapFogDistance = 20000.0f;
static const float ParticleMinimumBlockLight = 0.2f;
static const float MusicNoteParticleScale = 0.03f;
static const float MeshBudgetVisibilityRange = 100000.0f;
static const float WaterAlphaRecoveryRate = 0.05f;
static const float SunOrbitHorizontalAmplitude = -0.4f;
static const float UnderwaterMaximumFogDistance = 150.0f;
static const float NightSkyDaylightCutoff = 0.5f;
static const float UndeadDaylightReduction = 0.75f;
static const float StarFieldFogDistance = 100000.0f;
static const float ChunkSphereRadiusExpansion = 1.5f;
static const float ProjectileSpinRate = 0.5f;
static const float BoomerangSpinRate = -1.5f;
static const float PreviewFieldOfView = 0.78539819f;
static const float PreviewNearClip = 0.1f;
static const float PreviewFarClip = 1000.0f;
static const float ItemOutlineDisplacement = 0.1f;
static const float ItemOutlineDepthOffset = 0.1f;
static const float StaticEntityClosingRate = 0.001f;
static const float TrapClosingRate = 0.01f;
static const float MagicBarrierOpacity = 0.75f;
static const float InactiveMagicBarrierOpacityGain = 0.2f;
static const float RuneGateShellLift = 0.01f;
static const float RuneGateShellWidthGain = 1.05f;
static const float CloudLatticeSpacing = 200.0f;
static const int CloudLatticeSpacingBlocks = 200;
static const int CloudLatticeStaggerBlocks = 100;
static const int CloudSearchMargin = 2;
static const float CloudVisibilityRadius = 250.0f;
static const float CloudVisibilityMargin = 200.0f;
static const float CloudNearFadeOuterRadius = 180.0f;
static const float CloudNearFadeInnerRadius = 80.0f;
static const float CloudDiscardAlpha = 0.01f;
static const float CloudNearFadeRate = 0.01f;
static const double RotationVectorLengthEpsilon = 0.0001;
static const double RotationAxisSquaredLengthEpsilon = 0.0001;
static const float FlightPointMapScaleBase = 4.0f;
static const float FlightPointHighlightRate = 0.005f;
static const int FlightPointHighlightRadius = 2048;
static const int TriangleBudgetMultiplier = 3;
static const int TriangleBudgetShift = 12;
static const float ViewDistanceSafetyInset = 44.8f;
static const float MinimumViewDistance = 0.001f;
static const float EagleFlightViewDelayMs = 1000.0f;
static const float EagleFlightMinimumViewDistance = 64.0f;
static const float EagleFlightViewBlendRate = 0.005f;
static const float ShadedMaximumViewDistance = 256.0f;
static const float ViewDistanceGrowthRate = 0.0005f;
static const float MaximumEffectViewDistance = 200.0f;
static const float MaximumCreatureViewDistance = 256.0f;
static const float EffectViewDistanceFraction = 0.2f;
static const float CreatureViewDistanceFraction = 0.5f;
static const float EnvironmentLightBlendRate = 0.001f;
static const float FogColorBlendRate = 0.0005f;
static const float SunColorBlendRate = 0.002f;
static const float InterfaceSpriteFieldOfView = 0.87266469f;
static const float InterfaceSpriteNearClip = 0.05f;
static const int ChunkLightReach = 8;
static const float BeamVisibilityRadius = 200.0f;
static const float MaximumBeamRange = 100.0f;
static const float SupportBeamWidth = 0.1f;
static const float ElementalBeamWidth = 0.25f;

// The vector scaled to length 1.
static inline plasma::Vector<3, float> normalized(const plasma::Vector<3, float> &vector)
{
    return vector * (1.0f / sqrtf(vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2]));
}

// A direction turned by the matrix's upper 3x3 part (no translation).
static inline plasma::Vector<3, float> rotate(const plasma::Matrix<float> &m, const plasma::Vector<3, float> &direction)
{
    return plasma::Vector<3, float>(m[4] * direction[1] + m[0] * direction[0] + m[8] * direction[2],
                                    m[1] * direction[0] + m[5] * direction[1] + m[9] * direction[2],
                                    m[6] * direction[1] + m[2] * direction[0] + m[10] * direction[2]);
}

// The point on top of a static entity: its position raised by its height.
static inline plasma::Vector<3, cube::Fixed> top(const cube::StaticEntity &entity)
{
    return entity.position +
           plasma::Vector<3, cube::Fixed>(cube::Fixed(0.0f), cube::Fixed(0.0f), cube::Fixed(entity.scaledSize()[2]));
}

// The squared length of a world offset in blocks.
static inline float squaredBlocks(const plasma::Vector<3, cube::Fixed> &offset)
{
    plasma::Vector<3, float> blocks = plasma::Vector<3, float>(offset);
    return blocks[0] * blocks[0] + blocks[1] * blocks[1] + blocks[2] * blocks[2];
}

// A number scrambled into another (one round of a 13 / 17 / 5 shift generator): the plants get their phase from their
// index with it.
static inline unsigned int scramble(unsigned int value)
{
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

// The offset of a point 0.3 blocks in front of something that faces the angle (degrees), at the given height in
// blocks.
static inline plasma::Vector<3, cube::Fixed> inFront(float angle, float height)
{
    float radians = (float)(angle / 180.0f * 3.14159265358979323846);
    return plasma::Vector<3, cube::Fixed>(cube::Fixed(sinf(radians) * 0.3f), cube::Fixed(cosf(radians) * -0.3f),
                                          cube::Fixed(height));
}

// A world position raised or lowered by a height in blocks.
static inline plasma::Vector<3, cube::Fixed> raised(const plasma::Vector<3, cube::Fixed> &position, float blocks)
{
    return position + plasma::Vector<3, cube::Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, blocks));
}
static inline plasma::Vector<3, cube::Fixed> lowered(const plasma::Vector<3, cube::Fixed> &position, float blocks)
{
    return position - plasma::Vector<3, cube::Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, blocks));
}

// Half a sprite's size: the centre of its model.
static inline plasma::Vector<3, float> center(cube::Sprite *sprite)
{
    return plasma::Vector<3, float>((float)sprite->sizeX() * 0.5f, (float)sprite->sizeY() * 0.5f,
                                    (float)sprite->sizeZ() * 0.5f);
}

// Turns the matrix by the angle between the two directions, around the axis that is perpendicular to both; nothing
// when either is (nearly) zero or they are parallel. Defined below.
void rotateTowards(plasma::Matrix<float> &matrix, const plasma::Vector<3, float> &from,
                   const plasma::Vector<3, float> &to);

Renderer::Renderer(cube::World *world, cube::WorldMap *worldMap)
    : m_world(world),
      m_worldMap(worldMap),
      m_zoneRadius(0),
      m_viewDistance(0.0f),
      m_waterAlpha(1.0f),
      m_skyColors {plasma::Vector<4, float>(0.0f, 0.0f, 1.0f, 1.0f), plasma::Vector<4, float>(0.0f, 0.0f, 1.0f, 1.0f),
                   plasma::Vector<4, float>(0.0f, 0.0f, 1.0f, 1.0f), plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f)},
      // Initial lighting palette: cool ambient, warm front light and neutral back light (RGBA).
      m_dungeonShade(0.0f, 0.0f, 0.0f, 0.0f),
      m_ambientColor(0.3f, 0.3f, 0.7f, 1.0f),
      m_lightFrontColor(0.7f, 0.7f, 0.3f, 1.0f),
      m_lightBackColor(0.5f, 0.5f, 0.5f, 1.0f)
{}

Renderer::~Renderer() {}

// Perceived vertex brightness from RGB channels (0..255), with the vertex passed by value and colour converted to
// floats.
static inline float brightness(Vertex vertex)
{
    plasma::Vector<4, float> color(vertex.color);
    return color[0] * 0.299f + color[1] * 0.587f + color[2] * 0.114f;
}

// Appends the two triangles of the quad that the last four vertices form, split along the diagonal whose two ends
// differ less in brightness, so that the smooth shading does not show the other diagonal. The zone mesher calls it
// after the four vertices of each face.
void addQuad(std::vector<unsigned int> &indices, const std::vector<Vertex> &vertices)
{
    int count = (int)vertices.size();
    if ((float)fabs(brightness(vertices[count - 4]) - brightness(vertices[count - 2])) <
        (float)fabs(brightness(vertices[count - 3]) - brightness(vertices[count - 1]))) {
        indices.push_back(count - 4);
        indices.push_back(count - 3);
        indices.push_back(count - 2);
        indices.push_back(count - 4);
        indices.push_back(count - 2);
        indices.push_back(count - 1);
    } else {
        indices.push_back(count - 4);
        indices.push_back(count - 3);
        indices.push_back(count - 1);
        indices.push_back(count - 2);
        indices.push_back(count - 1);
        indices.push_back(count - 3);
    }
}

// Appends the two triangles of the quad whose four vertices begin at first, split along the first diagonal. The zone
// mesher uses it for the one quad of open sea.
void addQuad(std::vector<unsigned int> &indices, int first)
{
    indices.push_back(first);
    indices.push_back(first + 1);
    indices.push_back(first + 2);
    indices.push_back(first);
    indices.push_back(first + 2);
    indices.push_back(first + 3);
}

float Renderer::attenuation(const plasma::Vector<3, cube::Fixed> &position, float range)
{
    plasma::Vector<3, cube::Fixed> offset = position - m_cameraPosition;
    plasma::Vector<3, float> distance((float)offset[0], (float)offset[1], (float)offset[2]);
    float squaredDistance = distance.dot(distance);
    if (squaredDistance > DistanceFadeStartFraction * DistanceFadeStartFraction * range * range)
        return std::max(1.0f - (sqrtf(squaredDistance) - range * DistanceFadeStartFraction) /
                                   (range * DistanceFadeWidthFraction),
                        0.0f);
    return 1.0f;
}

bool Renderer::canMesh(int x, int y)
{
    m_world->enterZones();
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (!m_world->zone(x + dx, y + dy)) {
                m_world->leaveZones();
                return false;
            }
        }
    }
    m_world->leaveZones();
    return true;
}

void Renderer::clearPointLights()
{
    std::array<plasma::Vector<4, float>, 16> lights;
    for (plasma::Vector<4, float> &light : lights)
        light = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
    setPointLights(lights, lights);
}

void Renderer::collectLights(std::list<cube::Light> &lights)
{
    for (cube::Zone *zone : m_zones) {
        // The items on the ground that shine, near the camera only.
        if ((m_effectViewDistance + ZoneCullMarginBlocks) * (m_effectViewDistance + ZoneCullMarginBlocks) >
            zone->m_chunk.squaredCameraDistance) {
            for (cube::ZoneItem &item : zone->m_items) {
                if (item.hiddenTimeRemaining != 0)
                    continue;
                plasma::Vector<3, float> shine = item.item.lightColor();
                if (!(shine[0] * shine[0] + shine[1] * shine[1] + shine[2] * shine[2] > 0.0f))
                    continue;
                if (!m_world->isGroundItemAvailable(item))
                    continue;
                plasma::Vector<3, cube::Fixed> position =
                    item.position + plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(GroundItemLightElevation));
                if (!isSphereVisible(position, GroundItemLightRadius, m_effectViewDistance))
                    continue;
                Light light;
                light.position = position;
                // Instantfire light profile: radius in blocks and orange RGB intensity. Lich light profile: radius in
                // blocks and blue RGB intensity. Brazier light profile: radius in blocks and warm RGB intensity.
                // Ghost-brazier light profile: radius in blocks and blue RGB intensity. Unholy-bonfire light profile:
                // radius in blocks and cyan RGB intensity. Spirit-form light profile: radius in blocks and blue RGB
                // intensity. Promoted-resident light profile: radius in blocks and red RGB intensity. Boosted
                // appearance light profile: radius in blocks and purple RGB intensity. Shrine light profile: radius in
                // blocks and pale yellow RGB intensity. Angel light profile: radius in blocks and blue RGB intensity.
                // Magic crystal light profile: raised origin, radius in blocks and cyan RGB intensity. Emerald deposit
                // light profile: raised origin, radius in blocks and green RGB intensity. Sapphire deposit light
                // profile: raised origin, radius in blocks and blue RGB intensity. Ruby deposit light profile: raised
                // origin, radius in blocks and red RGB intensity. Diamond deposit light profile: raised origin, radius
                // in blocks and pale yellow RGB intensity.
                light.radius = GroundItemLightRadius;
                light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                light.color = item.item.lightColor();
                light.color *= attenuation(light.position, m_effectViewDistance);
                m_lights.push_back(light);
            }
        }

        for (cube::Zone::Plant &plant : zone->m_lightPlants) {
            // Type 76 is seen from afar: the first one within the view distance and 32 blocks lights the zone, and the
            // zone's other plants are left out.
            if (plant.type == cube::PlantWarmLight) {
                plasma::Vector<3, float> offset = plasma::Vector<3, float>(plant.position - m_cameraPosition);
                float squared = offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2];
                if (powf(m_viewDistance + 32.0f, 2.0f) > squared) {
                    Light light;
                    light.position = plant.position + plasma::Vector<3, cube::Fixed>(
                                                          cube::Fixed(0.0f), cube::Fixed(0.0f), cube::Fixed(0.0f));
                    // Warm-plant radius flicker curve: spatial noise phase, clock frequency, oscillation amplitude and
                    // mean radius in blocks. Ordinary plant radius flicker curve: spatial noise phase, clock
                    // frequency, oscillation amplitude and mean radius in blocks.
                    light.radius = cosf(cube::noise((double)cube::Fixed(light.position[0].m_raw * 10),
                                                    (double)cube::Fixed(light.position[1].m_raw * 10)) *
                                            10.0f +
                                        m_world->m_clock * 0.01f) *
                                       2.0f +
                                   30.0f;
                    light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                    light.color = plant.color;
                    m_lights.push_back(light);
                    break;
                }
            }
            // The lamps flicker; types 23..25 fade with the daylight and are off by day.
            if (!(plant.flags & cube::PlantFlagEmissive) ||
                !isSphereVisible(plant.position, PlantLightVisibilityRadius, m_viewDistance))
                continue;
            if (plant.type >= cube::PlantLantern02 && plant.type <= cube::PlantGreenWallLantern &&
                plant.daylight * m_daylight / 255.0f > LampDaylightCutoff)
                continue;
            Light light;
            light.position = plant.position +
                             plasma::Vector<3, cube::Fixed>(cube::Fixed(0.0f), cube::Fixed(0.0f), cube::Fixed(0.0f));
            light.radius = cosf(cube::noise((double)cube::Fixed(light.position[0].m_raw * 10),
                                            (double)cube::Fixed(light.position[1].m_raw * 10)) *
                                    10.0f +
                                m_world->m_clock * 0.01f) *
                               0.5f +
                           15.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plant.color;
            if (plant.type >= cube::PlantLantern02 && plant.type <= cube::PlantGreenWallLantern)
                light.color *= 1.0f - plant.daylight * m_daylight / 255.0f;
            m_lights.push_back(light);
        }

        // The static entities that shine, each from its top.
        for (cube::StaticEntity &entity : zone->m_lightEntities) {
            if (entity.type == cube::StaticEntityStreetLight02 || entity.type == cube::StaticEntityStreetLight) {
                // Streetlight profile: culling radius and light radius in blocks, daylight cutoff and warm RGB
                // intensity.
                if (isSphereVisible(top(entity), 50.0f, m_viewDistance) && m_daylight < StreetlightDaylightCutoff) {
                    Light light;
                    light.position = top(entity);
                    // Campfire creature light profile: radius in blocks and orange RGB intensity.
                    light.radius = 32.0f;
                    light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                    light.color = plasma::Vector<3, float>(1.7f, 1.5f, 0.8f);
                    m_lights.push_back(light);
                }
            }
            if (entity.type == cube::StaticEntityStreetLightUndead) {
                // Undead streetlight profile: culling/light radii in blocks, daylight cutoff and green RGB intensity.
                if (isSphereVisible(top(entity), 20.0f, m_viewDistance) && m_daylight < StreetlightDaylightCutoff) {
                    Light light;
                    light.position = top(entity);
                    // SkullSlater light profile: radius in blocks and cyan RGB intensity. Runestone light profile:
                    // radius in blocks and half-strength normalized hair colour. Rune portal light profile: radius in
                    // blocks and blue RGB intensity. Casting light curve: radius in blocks, cosine pulse amplitude and
                    // warm RGB base intensity.
                    light.radius = 20.0f;
                    light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                    light.color = plasma::Vector<3, float>(0.3f, 2.0f, 0.5f);
                    m_lights.push_back(light);
                }
            }
            // Campfire profile: raised light origin, culling/light radii in blocks and warm RGB intensity.
            if (entity.type == cube::StaticEntityCampfire && isSphereVisible(entity.position, 32.0f, m_viewDistance)) {
                Light light;
                light.position =
                    entity.position + plasma::Vector<3, cube::Fixed>(cube::Fixed(0.0f), cube::Fixed(0.0f),
                                                                     cube::Fixed(entity.scaledSize()[2] + 1.0f));
                light.radius = 32.0f;
                light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                light.color = plasma::Vector<3, float>(1.5f, 1.0f, 0.4f);
                m_lights.push_back(light);
            }
            // Types 0 and 126 only where the player has discovered their 16-block cell. Discovered goddess profile:
            // culling/light radii in blocks and RGB intensity.
            if (entity.type == cube::StaticEntityGoddess2 && isSphereVisible(entity.position, 32.0f, m_viewDistance)) {
                std::array<int, 2> cell = {cube::floorDivide(entity.position[0].toBlock(), cube::BlocksPerCell),
                                           cube::floorDivide(entity.position[1].toBlock(), cube::BlocksPerCell)};
                if (m_world->m_localCreature->m_playerData->discoveredMarkers.find(cell) !=
                    m_world->m_localCreature->m_playerData->discoveredMarkers.end()) {
                    Light light;
                    light.position =
                        entity.position + plasma::Vector<3, cube::Fixed>(cube::Fixed(0.0f), cube::Fixed(0.0f),
                                                                         cube::Fixed(entity.scaledSize()[2] + 1.0f));
                    light.radius = 32.0f;
                    light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                    light.color = plasma::Vector<3, float>(0.25f, 0.3f, 0.5f);
                    m_lights.push_back(light);
                }
            }
            // Discovered brazier profile: culling/light radii in blocks and RGB intensity.
            if (entity.type == cube::StaticEntityBrazier && isSphereVisible(entity.position, 32.0f, m_viewDistance)) {
                std::array<int, 2> cell = {cube::floorDivide(entity.position[0].toBlock(), cube::BlocksPerCell),
                                           cube::floorDivide(entity.position[1].toBlock(), cube::BlocksPerCell)};
                if (m_world->m_localCreature->m_playerData->discoveredMarkers.find(cell) !=
                    m_world->m_localCreature->m_playerData->discoveredMarkers.end()) {
                    Light light;
                    light.position =
                        entity.position + plasma::Vector<3, cube::Fixed>(cube::Fixed(0.0f), cube::Fixed(0.0f),
                                                                         cube::Fixed(entity.scaledSize()[2] + 1.0f));
                    light.radius = 32.0f;
                    light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                    light.color = plasma::Vector<3, float>(1.5f, 1.0f, 0.4f);
                    m_lights.push_back(light);
                }
            }
            // StreetLight01 profile: culling/light radii in blocks, daylight cutoff and RGB intensity.
            if (entity.type == cube::StaticEntityStreetLight01 &&
                isSphereVisible(entity.position, 32.0f, m_viewDistance) && m_daylight < StreetlightDaylightCutoff) {
                Light light;
                light.position = top(entity);
                light.radius = 20.0f;
                light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                light.color = plasma::Vector<3, float>(1.2f, 0.8f, 0.4f);
                m_lights.push_back(light);
            }
            // Druid-light profile: culling/light radii in blocks and RGB intensity.
            if ((entity.type == cube::StaticEntityDruidLight || entity.type == cube::StaticEntityDruidLight2) &&
                isSphereVisible(entity.position, 32.0f, m_viewDistance)) {
                Light light;
                light.position = top(entity);
                light.radius = 20.0f;
                light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                light.color = plasma::Vector<3, float>(0.6f, 0.7f, 1.3f);
                m_lights.push_back(light);
            }
            // DruidLight3 profile: culling/light radii in blocks, daylight cutoff and RGB intensity.
            if (entity.type == cube::StaticEntityDruidLight3 &&
                isSphereVisible(entity.position, 32.0f, m_viewDistance) && m_daylight < StreetlightDaylightCutoff) {
                Light light;
                light.position = top(entity);
                light.radius = 20.0f;
                light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                // Torch light RGB palette: warm red-biased intensity, allowed above one.
                light.color = plasma::Vector<3, float>(1.2f, 1.0f, 0.5f);
                m_lights.push_back(light);
            }
            // Golden-gate profile: culling/light radii in blocks and RGB intensity.
            if (entity.type == cube::StaticEntityGoldenGate &&
                isSphereVisible(entity.position, 32.0f, m_viewDistance)) {
                Light light;
                light.position = top(entity);
                light.radius = 20.0f;
                light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                light.color = plasma::Vector<3, float>(1.2f, 1.0f, 0.5f);
                m_lights.push_back(light);
            }
        }
    }

    // The projectiles of types 3 and 4, by their size.
    for (cube::ServerUpdates::BulletPacket &projectile : m_world->m_projectiles) {
        if (projectile.projectileType != cube::ProjectileSpellBolt &&
            projectile.projectileType != cube::ProjectileFireball)
            continue;
        // Projectile light profile: culling radius in blocks and warm RGB intensity.
        if (!isSphereVisible(projectile.position, 16.0f, m_effectViewDistance))
            continue;
        Light light;
        light.position = projectile.position;
        light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
        light.color = plasma::Vector<3, float>(projectile.sizeInBlocks * 0.5f, projectile.sizeInBlocks * 0.5f,
                                               projectile.sizeInBlocks * 0.2f);
        light.radius = 16.0f;
        m_lights.push_back(light);
    }

    // The game's lights (the flashes of hits), fading with their age.
    for (cube::Light flash : lights) {
        if (!isSphereVisible(flash.position, flash.radius, m_effectViewDistance))
            continue;
        Light light;
        light.position = flash.position;
        light.radius = flash.radius;
        light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
        light.color = flash.color * std::max(0.0f, 1.0f - (float)flash.age / (float)flash.duration);
        m_lights.push_back(light);
    }

    for (cube::Creature *creature : m_world->m_creatures) {
        cube::Creature::EntityData &data = creature->m_entityData;
        if (!(data.hp > 0.0f))
            continue;

        // A lamp in a hand, the lantern toggle, buff 9 and race 185 light from a point beside the creature, at three
        // tenths of its height.
        if (data.equipment.weaponRight.id == cube::WeaponTorch || data.equipment.weaponLeft.id == cube::WeaponTorch) {
            plasma::Matrix<float> turn;
            turn.identity();
            turn.rotateZ(data.rotation[2]);
            Light light;
            light.position = creature->m_drawnPosition +
                             plasma::Vector<3, cube::Fixed>(
                                 turn * plasma::Vector<3, float>(
                                            0.5f, 1.0f, data.appearance.size[2] * 0.3f - creature->m_stepOffset));
            light.radius = data.lampDiameter;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(1.2f, 1.0f, 0.5f);
            m_lights.push_back(light);
        }
        if (data.binaryToggles.lampOn) {
            plasma::Matrix<float> turn;
            turn.identity();
            turn.rotateZ(data.rotation[2]);
            Light light;
            light.position = creature->m_drawnPosition +
                             plasma::Vector<3, cube::Fixed>(
                                 turn * plasma::Vector<3, float>(
                                            0.5f, 1.0f, data.appearance.size[2] * 0.3f - creature->m_stepOffset));
            light.radius = data.lampDiameter;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            // Lamp light RGB palette: warm muted intensity.
            light.color = plasma::Vector<3, float>(0.8f, 0.7f, 0.4f);
            m_lights.push_back(light);
        }
        if (creature->findBuff(cube::BuffInstantfire)) {
            plasma::Matrix<float> turn;
            turn.identity();
            turn.rotateZ(data.rotation[2]);
            Light light;
            light.position = creature->m_drawnPosition +
                             plasma::Vector<3, cube::Fixed>(
                                 turn * plasma::Vector<3, float>(
                                            0.5f, 1.0f, data.appearance.size[2] * 0.3f - creature->m_stepOffset));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.6f, 0.4f, 0.0f);
            m_lights.push_back(light);
        }
        if (data.race == cube::RaceCampfire) {
            plasma::Matrix<float> turn;
            turn.identity();
            turn.rotateZ(data.rotation[2]);
            Light light;
            light.position = creature->m_drawnPosition +
                             plasma::Vector<3, cube::Fixed>(
                                 turn * plasma::Vector<3, float>(
                                            0.5f, 1.0f, data.appearance.size[2] * 0.3f - creature->m_stepOffset));
            light.radius = 32.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.6f, 0.4f, 0.0f);
            m_lights.push_back(light);
        }

        // The races that glow, from their feet.
        if (data.race == cube::RaceLich) {
            Light light;
            light.position =
                creature->m_drawnPosition + plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.0f, 0.4f, 0.8f);
            m_lights.push_back(light);
        }
        if (data.race == cube::RaceBrazier && !data.interactionState) {
            Light light;
            light.position =
                creature->m_drawnPosition + plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.8f, 0.6f, 0.1f);
            m_lights.push_back(light);
        }
        if (data.race == cube::RaceGhostBrazier && !data.interactionState) {
            Light light;
            light.position =
                creature->m_drawnPosition + plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.0f, 0.4f, 0.8f);
            m_lights.push_back(light);
        }
        if (data.race == cube::RaceUnholyBonfire && !data.interactionState) {
            Light light;
            light.position =
                creature->m_drawnPosition + plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.0f, 0.8f, 0.3f);
            m_lights.push_back(light);
        }
        if (data.race == cube::RaceSkullSlater && !data.interactionState) {
            Light light;
            light.position =
                creature->m_drawnPosition + plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset));
            light.radius = 20.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.0f, 0.8f, 0.3f);
            m_lights.push_back(light);
        }
        // Three appearance flags (the first also with buff 19).
        if ((data.appearance.appearanceFlags & cube::AppearanceFlagSpiritForm) ||
            creature->findBuff(cube::BuffSpiritworld)) {
            Light light;
            light.position =
                creature->m_drawnPosition + plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.05f, 0.2f, 0.3f);
            m_lights.push_back(light);
        }
        if (data.appearance.appearanceFlags & cube::AppearanceFlagPromotedResident) {
            Light light;
            light.position =
                creature->m_drawnPosition + plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.25f, 0.1f, 0.1f);
            m_lights.push_back(light);
        }
        if (data.appearance.appearanceFlags & cube::AppearanceFlagTinted) {
            Light light;
            light.position =
                creature->m_drawnPosition + plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.25f, 0.1f, 0.5f);
            m_lights.push_back(light);
        }
        if (data.race == cube::RaceShrineOfTheLight) {
            Light light;
            light.position = creature->m_drawnPosition +
                             plasma::Vector<3, cube::Fixed>(cube::Fixed(0.0f), cube::Fixed(0.0f),
                                                            cube::Fixed(data.appearance.size[2] * 0.5f)) +
                             plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.5f, 0.5f, 0.3f);
            m_lights.push_back(light);
        }
        // The races 264..267 glow in half their hair colour.
        if (data.race >= cube::RaceFireRunestone && data.race <= cube::RaceWindRunestone && !data.interactionState) {
            Light light;
            light.position =
                creature->m_drawnPosition + plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset));
            light.radius = 20.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>((float)data.appearance.hairColor[0] / 255.0f * 0.5f,
                                                   (float)data.appearance.hairColor[1] / 255.0f * 0.5f,
                                                   (float)data.appearance.hairColor[2] / 255.0f * 0.5f);
            m_lights.push_back(light);
        }
        if (data.race == cube::RaceRunePortal && data.interactionState) {
            Light light;
            light.position =
                creature->m_drawnPosition + plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset));
            light.radius = 20.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.1f, 0.2f, 0.4f);
            m_lights.push_back(light);
        }
        if (data.race == cube::RaceAngel) {
            Light light;
            light.position =
                creature->m_drawnPosition + plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.0f, 0.25f, 0.5f);
            m_lights.push_back(light);
        }
        // Five races glow from a block and a half above their position.
        if (data.race == cube::RaceMagicCrystal) {
            Light light;
            light.position = data.position +
                             plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset)) +
                             plasma::Vector<3, cube::Fixed>(cube::Fixed(0.0f), cube::Fixed(0.0f), cube::Fixed(1.5f));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.3f, 0.5f, 0.6f);
            m_lights.push_back(light);
        }
        if (data.race == cube::RaceEmeraldDeposit) {
            Light light;
            light.position = data.position +
                             plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset)) +
                             plasma::Vector<3, cube::Fixed>(cube::Fixed(0.0f), cube::Fixed(0.0f), cube::Fixed(1.5f));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.1f, 0.5f, 0.1f);
            m_lights.push_back(light);
        }
        if (data.race == cube::RaceSapphireDeposit) {
            Light light;
            light.position = data.position +
                             plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset)) +
                             plasma::Vector<3, cube::Fixed>(cube::Fixed(0.0f), cube::Fixed(0.0f), cube::Fixed(1.5f));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.1f, 0.1f, 0.5f);
            m_lights.push_back(light);
        }
        if (data.race == cube::RaceRubyDeposit) {
            Light light;
            light.position = data.position +
                             plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset)) +
                             plasma::Vector<3, cube::Fixed>(cube::Fixed(0.0f), cube::Fixed(0.0f), cube::Fixed(1.5f));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.5f, 0.1f, 0.2f);
            m_lights.push_back(light);
        }
        if (data.race == cube::RaceDiamondDeposit) {
            Light light;
            light.position = data.position +
                             plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset)) +
                             plasma::Vector<3, cube::Fixed>(cube::Fixed(0.0f), cube::Fixed(0.0f), cube::Fixed(1.5f));
            light.radius = 16.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            light.color = plasma::Vector<3, float>(0.8f, 0.8f, 0.6f);
            m_lights.push_back(light);
        }

        // The abilities that glow while they run: a pulsing warm light a block in front of the creature, or at the
        // ability's target.
        unsigned char ability = data.currentAbility;
        if (((ability == cube::AbilitySupportRay ||
              (ability >= cube::AbilityFireBeam && ability <= cube::AbilityWaterBeam) ||
              ability == cube::AbilityFireballCharge) &&
             (float)creature->attackDuration() > data.timeSinceAbility) ||
            ((ability == cube::AbilityFireball || ability == cube::AbilityMonsterFirebolt ||
              ability == cube::AbilityWaterball || ability == cube::AbilityMonsterWaterbolt ||
              ability == cube::AbilityFireMissiles || ability == cube::AbilityDjinnExplosion ||
              ability == cube::AbilityFireExplosion || ability == cube::AbilityLavaPuddle ||
              ability == cube::AbilitySummon) &&
             (float)creature->anticipationTime() > data.timeSinceAbility)) {
            plasma::Matrix<float> turn;
            turn.identity();
            turn.rotateZ(creature->m_drawnRotation[2]);
            Light light;
            light.position = creature->m_drawnPosition +
                             plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset)) +
                             plasma::Vector<3, cube::Fixed>(rotate(turn, plasma::Vector<3, float>(0.0f, 1.0f, 0.0f)));
            light.radius = 20.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            float pulse =
                cosf((float)(2 * data.timeSinceAbility * cube::Pi / creature->attackDuration())) * 0.1f + 1.0f;
            light.color = plasma::Vector<3, float>(0.3f, 0.2f, 0.1f) * pulse;
            m_lights.push_back(light);
        }
        if (((data.currentAbility >= cube::AbilityFireSwirl && data.currentAbility <= cube::AbilityWaterVortex) ||
             data.currentAbility == cube::AbilityHealingStream ||
             data.currentAbility == cube::AbilityCenteredHealingStream) &&
            (float)creature->attackDuration() > data.timeSinceAbility &&
            // Targeted-ability light curve: culling/radius in blocks, clock pulse and warm RGB base intensity. Swirl
            // effect curve: visibility radius, anticipation turn gain, pre/post-anticipation samples and cube scale.
            isSphereVisible(data.position + plasma::Vector<3, cube::Fixed>(creature->m_drawnAttackRotation), 4.0f,
                            m_creatureViewDistance)) {
            Light light;
            light.position = data.position + plasma::Vector<3, cube::Fixed>(0, 0, cube::Fixed(-creature->m_stepOffset));
            if (data.currentAbility != cube::AbilityCenteredHealingStream)
                light.position += plasma::Vector<3, cube::Fixed>(creature->m_drawnAttackRotation);
            light.radius = 20.0f;
            light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
            float pulse = cosf(data.timeSinceAbility * 0.01f) * 0.1f + 1.0f;
            light.color = plasma::Vector<3, float>(0.3f, 0.2f, 0.1f) * pulse;
            m_lights.push_back(light);
        }

        // With a hit combo, the equipment that has an effect colour glows in it: the weapons at the start of their
        // trails, the shoulder and chest pieces at the creature.
        float combo = (float)data.hitCombo / (float)creature->maxCombo();
        if (combo == 0.0f)
            continue;
        if (data.equipment.weaponRight.category == cube::ItemCategoryWeapon) {
            plasma::Vector<4, float> effect = data.equipment.weaponRight.effectColor();
            if (effect[3] > 0.0f) {
                Light light;
                light.position = plasma::Vector<3, cube::Fixed>(-creature->m_animationState.m_originX,
                                                                -creature->m_animationState.m_originY, cube::Fixed(0)) +
                                 plasma::Vector<3, cube::Fixed>(creature->m_animationState.m_trails[0][0]);
                light.radius = ComboLightRadius;
                light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                plasma::Vector<3, float> glow(effect[0], effect[1], effect[2]);
                light.color = combo * glow * RightWeaponComboLightGain;
                m_lights.push_back(light);
            }
        }
        if (data.equipment.weaponLeft.category == cube::ItemCategoryWeapon) {
            plasma::Vector<4, float> effect = data.equipment.weaponLeft.effectColor();
            if (effect[3] > 0.0f) {
                Light light;
                light.position = plasma::Vector<3, cube::Fixed>(-creature->m_animationState.m_originX,
                                                                -creature->m_animationState.m_originY, cube::Fixed(0)) +
                                 plasma::Vector<3, cube::Fixed>(creature->m_animationState.m_trails[2][0]);
                light.radius = ComboLightRadius;
                light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                plasma::Vector<3, float> glow(effect[0], effect[1], effect[2]);
                light.color = combo * glow * LeftWeaponComboLightGain;
                m_lights.push_back(light);
            }
        }
        if (data.equipment.shoulder.category != cube::ItemCategoryNone) {
            plasma::Vector<4, float> effect = data.equipment.shoulder.effectColor();
            if (effect[3] > 0.0f) {
                Light light;
                light.position = data.position;
                light.radius = ComboLightRadius;
                light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                plasma::Vector<3, float> glow(effect[0], effect[1], effect[2]);
                light.color = combo * glow * ArmorComboLightGain;
                m_lights.push_back(light);
            }
        }
        if (data.equipment.chest.category != cube::ItemCategoryNone) {
            plasma::Vector<4, float> effect = data.equipment.chest.effectColor();
            if (effect[3] > 0.0f) {
                Light light;
                light.position = data.position;
                light.radius = ComboLightRadius;
                light.squaredDistance = (float)(m_cameraPosition - light.position).squaredLength();
                plasma::Vector<3, float> glow(effect[0], effect[1], effect[2]);
                light.color = combo * glow * ArmorComboLightGain;
                m_lights.push_back(light);
            }
        }
    }

    // Nearest first, and each light to the zones it reaches, sixteen a zone at most.
    std::sort(m_lights.begin(), m_lights.end(),
              [this](const Light &a, const Light &b) { return a.squaredDistance < b.squaredDistance; });
    for (Light &light : m_lights) {
        int x0 = cube::zoneCoordinate(light.position[0] - light.radius);
        int y0 = cube::zoneCoordinate(light.position[1] - light.radius);
        int x1 = cube::zoneCoordinate(light.position[0] + light.radius);
        int y1 = cube::zoneCoordinate(light.position[1] + light.radius);
        for (int x = x0; x <= x1; x++) {
            for (int y = y0; y <= y1; y++) {
                cube::Zone *zone = m_world->zone(x, y);
                if (!zone || zone->m_chunk.lightCount >= 16)
                    continue;
                plasma::Vector<3, float> position = plasma::Vector<3, float>(
                    light.position + plasma::Vector<3, cube::Fixed>(m_drawOriginX, m_drawOriginY, cube::Fixed(0)));
                zone->m_chunk.lightPositions[zone->m_chunk.lightCount] =
                    plasma::Vector<4, float>(position[0], position[1], position[2], light.radius);
                zone->m_chunk.lightColors[zone->m_chunk.lightCount] =
                    plasma::Vector<4, float>(light.color[0], light.color[1], light.color[2], 0.0f);
                zone->m_chunk.lightCount++;
            }
        }
    }
}

// The header describes what is collected.
void Renderer::collectSprites(const plasma::Vector<3, int> &lookedAt, int flags)
{
    // Twelve shear matrices that sway with the time, each a little out of step with the last; the plants that move in
    // the wind take one each.
    std::vector<plasma::Matrix<float>> sways(12);
    for (int i = 0; i < (int)sways.size(); i++) {
        sways[i].identity();
        sways[i].rotateY(-90.0f);
        plasma::Matrix<float> shearX;
        shearX.identity();
        // Plant wind pose curves: clock frequencies, per-plant phase offsets and shear amplitudes.
        shearX[1] =
            cosf(m_world->m_clock * 0.004f + (float)(i * 30) + 84.0f) + cosf(m_world->m_clock * 0.0027f + 93.0f);
        sways[i] *= shearX;
        sways[i].rotateY(90.0f);
        sways[i].rotateX(-90.0f);
        plasma::Matrix<float> shearY;
        shearY.identity();
        shearY[4] = (cosf(m_world->m_clock * 0.005f + (float)(i * 30)) + cosf(m_world->m_clock * 0.003f)) * 0.8f;
        sways[i] *= shearY;
        sways[i].rotateX(90.0f);
    }

    // The zones around the camera that have a mesh and are in view, with their plants.
    float viewDistance = m_viewDistance;
    int cameraZoneX = cube::floorDivide(m_cameraPosition[0].toBlock(), cube::BlocksPerZone);
    int cameraZoneY = cube::floorDivide(m_cameraPosition[1].toBlock(), cube::BlocksPerZone);
    if (!(flags & RenderFlagOnlyPlayer)) {
        for (int x = cameraZoneX - m_zoneRadius; x <= cameraZoneX + m_zoneRadius; x++) {
            for (int y = cameraZoneY - m_zoneRadius; y <= cameraZoneY + m_zoneRadius; y++) {
                cube::Zone *zone = m_world->zone(x, y);
                if (!zone || zone->m_chunk.chunkBuffers.empty())
                    continue;
                zone->m_chunk.lightCount = 0;
                for (plasma::Vector<4, float> &position : zone->m_chunk.lightPositions)
                    position = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
                for (plasma::Vector<4, float> &color : zone->m_chunk.lightColors)
                    color = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
                bool visible =
                    isBoxVisible(zone->m_chunk.boundsMin, zone->m_chunk.boundsMax, viewDistance + ZoneCullMarginBlocks);
                if (zone->m_chunk.chunkBuffers.empty() || !visible)
                    continue;
                m_zones.push_back(zone);
                zone->m_chunk.squaredCameraDistance = squaredBlocks(zone->m_chunk.center - m_cameraPosition);
                if (!((m_effectViewDistance + ZoneCullMarginBlocks) * (m_effectViewDistance + ZoneCullMarginBlocks) >
                      zone->m_chunk.squaredCameraDistance))
                    continue;

                int index = 0;
                for (cube::Zone::Plant &plant : zone->m_plants) {
                    index++;
                    // Type 89 has no sprite: it is a shadow on the ground.
                    if (plant.type == cube::PlantShadow) {
                        GroundShadow shadow;
                        shadow.position = plant.position;
                        shadow.height = plant.scale;
                        shadow.size = plant.rotation;
                        shadow.squaredDistance = squaredBlocks(plant.position - m_cameraPosition);
                        if (m_effectViewDistance * m_effectViewDistance > shadow.squaredDistance)
                            m_shadows.push_back(shadow);
                        continue;
                    }
                    cube::Sprite *sprite = m_world->m_spriteManager->decorationSprite(plant.type);
                    if (!sprite)
                        continue;
                    float radius = (float)(std::max)(sprite->sizeX(), sprite->sizeZ()) * plant.scale;
                    float halfHeight = (float)sprite->sizeZ() * 0.5f * plant.scale;
                    if (!isSphereVisible(raised(plant.position, halfHeight), radius, m_effectViewDistance))
                        continue;

                    plasma::Matrix<float> model;
                    model.identity();
                    model.translate((float)(plant.position[0] + m_drawOriginX),
                                    (float)(plant.position[1] + m_drawOriginY), (float)plant.position[2]);
                    model.scale(plant.scale, plant.scale, plant.scale);
                    // Flag 4: the top sways (the shear is applied to a model a tenth of a block high).
                    if (plant.flags & cube::PlantFlagSway) {
                        float flat = 0.1f / ((float)sprite->sizeZ() * plant.scale);
                        model.scale(1.0f, 1.0f, 1.0f / flat);
                        model *= sways[index % sways.size()];
                        model.scale(1.0f, 1.0f, flat);
                    }
                    if (plant.flags & cube::PlantFlagSwingY) {
                        // Hanging-plant swing curve: scrambled phase modulo 1000, clock frequency and angle amplitude
                        // in degrees.
                        float phase = (float)(int)(scramble(index) % 1000);
                        model.translate(0.0f, 0.0f, (float)sprite->sizeZ());
                        model.rotateY(cosf(m_world->m_clock * 0.004f + phase) * 30.0f);
                        model.translate(0.0f, 0.0f, (float)-sprite->sizeZ());
                    }
                    if (plant.flags & cube::PlantFlagSwingX) {
                        float phase = (float)(int)(scramble(index) % 1000);
                        model.translate(0.0f, 0.0f, (float)sprite->sizeZ());
                        model.rotateX(cosf(m_world->m_clock * 0.004f + phase) * 30.0f);
                        model.translate(0.0f, 0.0f, (float)-sprite->sizeZ());
                    }
                    model.rotateZ(plant.rotation);
                    model.translate((float)sprite->sizeX() * -0.5f, (float)sprite->sizeY() * -0.5f, 0.0f);

                    // Flag 1: it shines (full light, and the glow value); otherwise it takes the light stored with it.
                    plasma::Vector<4, float> color = m_tint;
                    if (plant.flags & cube::PlantFlagEmissive)
                        color[3] = 1.0f;
                    else
                        color[3] *= plant.daylight / 255.0f;
                    halfHeight = (float)sprite->sizeZ() * 0.5f * plant.scale;
                    // Flag 8 puts it into the second batch.
                    if (plant.flags & cube::PlantFlagUnderwater) {
                        m_underwaterPlantBatch.add(
                            sprite, model, color, attenuation(raised(plant.position, halfHeight), m_effectViewDistance),
                            (float)(plant.flags & cube::PlantFlagEmissive), 0.0f, 0.0f, 0.0f);
                    } else {
                        m_worldBatch.add(sprite, model, color,
                                         attenuation(raised(plant.position, halfHeight), m_effectViewDistance),
                                         (float)(plant.flags & cube::PlantFlagEmissive), 0.0f, 0.0f, 0.0f);
                    }

                    // The types 64..67 have a ring of cubes rising in front of them (0.3 blocks along their facing),
                    // each in its colours.
                    switch (plant.type) {
                    case cube::PlantTorchRedYellowFlame:
                        // Torch-plant ring profiles: flame elevation per plant scale, RGB endpoints, radius/cube size
                        // in blocks and sample count.
                        m_boxes.push_back(Box(
                            plant.position + inFront(plant.rotation, plant.scale * 10.0f),
                            // Instantfire ring profile: RGB endpoints, radius from creature size, cube size and sample
                            // count.
                            plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f),
                            plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f), (unsigned int)m_world->m_clock, 0.5f,
                            0.15f, 20));
                        break;
                    case cube::PlantTorchBlueCyanFlame:
                        m_boxes.push_back(Box(plant.position + inFront(plant.rotation, plant.scale),
                                              plasma::Vector<4, float>(0.0f, 0.0f, 1.0f, 1.0f),
                                              plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f),
                                              (unsigned int)m_world->m_clock, 0.5f, 0.15f, 20));
                        break;
                    case cube::PlantTorchGreenCyanFlame:
                        m_boxes.push_back(
                            Box(plant.position + inFront(plant.rotation, plant.scale),
                                // Poisoned ring profile: RGB endpoints, radius from creature size, cube size and sample
                                // count.
                                plasma::Vector<4, float>(0.0f, 1.0f, 0.0f, 1.0f),
                                plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f), (unsigned int)m_world->m_clock, 0.5f,
                                0.15f, 20));
                        break;
                    case cube::PlantTorchYellow:
                        m_boxes.push_back(Box(plant.position + inFront(plant.rotation, plant.scale),
                                              plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f),
                                              plasma::Vector<4, float>(1.0f, 0.0f, 0.5f, 1.0f),
                                              (unsigned int)m_world->m_clock, 0.5f, 0.15f, 20));
                        break;
                    }
                }
            }
        }
    }
    // Nearest zone first.
    std::sort(m_zones.begin(), m_zones.end(), [](cube::Zone *a, cube::Zone *b) {
        return a->m_chunk.squaredCameraDistance < b->m_chunk.squaredCameraDistance;
    });

    // A marker (sprite 3202) at the offset the player's character keeps in PlayerData::treasureSpiritOffset, when
    // there is one.
    if (m_world->m_localCreature) {
        const plasma::Vector<3, float> &offset = m_world->m_localCreature->m_playerData->treasureSpiritOffset;
        if (offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2] > 0.0f) {
            // The sprite pointer is dereferenced without a null check.
            cube::Sprite *sprite = m_world->m_spriteManager->sprite(cube::SpriteSpiribit);
            plasma::Matrix<float> model;
            model.identity();
            model.translate(plasma::Vector<3, float>(
                m_world->m_localCreature->m_entityData.position +
                plasma::Vector<3, cube::Fixed>(m_drawOriginX, m_drawOriginY,
                                               cube::Fixed(-m_world->m_localCreature->m_stepOffset))));
            model.translate(m_world->m_localCreature->m_playerData->treasureSpiritOffset[0],
                            m_world->m_localCreature->m_playerData->treasureSpiritOffset[1],
                            m_world->m_localCreature->m_playerData->treasureSpiritOffset[2]);
            model.scale(TreasureSpiritScale, TreasureSpiritScale, TreasureSpiritScale);
            model.translate((float)sprite->sizeX() * -0.5f, (float)sprite->sizeY() * -0.5f,
                            (float)sprite->sizeZ() * -0.5f);
            m_worldBatch.add(m_world->m_spriteManager->sprite(cube::SpriteSpiribit), model,
                             // Shrine ring profile: white/yellow RGB endpoints, cube size and sample count.
                             plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    for (cube::Creature *creature : m_world->m_creatures) {
        // RenderFlagHidePlayer leaves the player's character out; RenderFlagOnlyPlayer shows nothing else.
        if ((flags & RenderFlagHidePlayer) && creature == m_world->m_localCreature)
            continue;
        if ((flags & RenderFlagOnlyPlayer) && creature != m_world->m_localCreature)
            continue;
        if (!(creature->m_entityData.hp > 0.0f))
            continue;

        if (flags & RenderFlagDebugCreatureBlocks) {
            for (plasma::Vector<3, int> &block : creature->m_debugBlocks) {
                plasma::Matrix<float> model;
                model.identity();
                model.translate(plasma::Vector<3, float>(
                    plasma::Vector<3, cube::Fixed>(m_drawOriginX, m_drawOriginY, cube::Fixed(0)) +
                    plasma::Vector<3, cube::Fixed>(cube::Fixed(block[0]), cube::Fixed(block[1]),
                                                   cube::Fixed(block[2]))));
                model.translate(0.5f, 0.5f, 0.0f);
                model.scale(DebugBlockScale, DebugBlockScale, DebugBlockScale);
                model.translate(-0.5f, -0.5f, 0.0f);
                m_worldBatch.add(m_world->m_spriteManager->sprite(cube::SpriteCube), model,
                                 plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            }
            plasma::Matrix<float> model;
            model.identity();
            model.translate(
                plasma::Vector<3, float>(plasma::Vector<3, cube::Fixed>(m_drawOriginX, m_drawOriginY, cube::Fixed(0)) +
                                         creature->m_entityData.origin));
            model.translate(0.5f, 0.5f, 0.0f);
            model.scale(0.5f, 0.5f, 0.5f);
            model.translate(-0.5f, -0.5f, 0.0f);
            m_worldBatch.add(m_world->m_spriteManager->sprite(cube::SpriteCube), model,
                             plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        }

        cube::Creature::AnimationState &animation = creature->m_animationState;

        // The abilities 31..34: a spiral of cubes around the point aimed at, which widens while the ability lasts; a
        // second, wider one for 32 and 34. Red to yellow, or blue for specialization 1.
        if (creature->m_entityData.currentAbility >= cube::AbilityFireSwirl &&
            creature->m_entityData.currentAbility <= cube::AbilityWaterVortex &&
            (float)creature->attackDuration() > creature->m_entityData.timeSinceAbility &&
            isSphereVisible(creature->m_entityData.position +
                                plasma::Vector<3, cube::Fixed>(creature->m_drawnAttackRotation),
                            4.0f, m_creatureViewDistance)) {
            float turns = creature->m_entityData.timeSinceAbility / (float)creature->anticipationTime() * 0.1f;
            float radius = (creature->m_entityData.timeSinceAbility + creature->m_entityData.timeSinceAbility) /
                           (float)creature->attackDuration();
            int count = 43;
            if (creature->m_entityData.timeSinceAbility > (float)creature->anticipationTime()) {
                turns += turns;
                radius += radius;
                count = 86;
            }
            plasma::Vector<4, float> startColor(1.0f, 0.0f, 0.0f, 1.0f);
            plasma::Vector<4, float> endColor(1.0f, 1.0f, 0.0f, 1.0f);
            if (creature->m_entityData.specialization == cube::SpecializationWater) {
                // Water swirl RGB endpoints: deep blue to pale blue.
                startColor = plasma::Vector<4, float>(0.0f, 0.1f, 1.0f, 1.0f);
                endColor = plasma::Vector<4, float>(0.5f, 0.7f, 1.0f, 1.0f);
            }
            m_cubes.push_back(Cube(plasma::Vector<3, cube::Fixed>(creature->m_drawnAttackRotation) +
                                       lowered(creature->m_entityData.position, radius * 0.5f),
                                   startColor, endColor, (unsigned int)creature->m_entityData.timeSinceAbility, radius,
                                   creature->m_entityData.timeSinceAbility * 0.4f / (float)creature->attackDuration(),
                                   turns, count));
            if (creature->m_entityData.currentAbility == cube::AbilityFireVortex ||
                creature->m_entityData.currentAbility == cube::AbilityWaterVortex) {
                m_cubes.push_back(
                    Cube(plasma::Vector<3, cube::Fixed>(creature->m_drawnAttackRotation) +
                             lowered(creature->m_entityData.position, radius),
                         startColor, endColor, (unsigned int)creature->m_entityData.timeSinceAbility, radius + radius,
                         creature->m_entityData.timeSinceAbility * 0.4f / (float)creature->attackDuration(),
                         turns * 0.5f, count + count));
            }
        }

        // The abilities 134 and 135: two rings of rising cubes, 2.5 and 5 blocks wide, at the point aimed at (134) or
        // at the creature.
        if ((creature->m_entityData.currentAbility == cube::AbilityHealingStream ||
             creature->m_entityData.currentAbility == cube::AbilityCenteredHealingStream) &&
            (float)creature->attackDuration() > creature->m_entityData.timeSinceAbility) {
            plasma::Vector<4, float> startColor(1.0f, 0.0f, 0.0f, 1.0f);
            plasma::Vector<4, float> endColor(1.0f, 1.0f, 0.0f, 1.0f);
            if (creature->m_entityData.specialization == cube::SpecializationWater) {
                startColor = plasma::Vector<4, float>(0.0f, 0.1f, 1.0f, 1.0f);
                endColor = plasma::Vector<4, float>(0.5f, 0.7f, 1.0f, 1.0f);
            }
            plasma::Vector<3, cube::Fixed> position = creature->m_drawnPosition;
            if (creature->m_entityData.currentAbility != cube::AbilityCenteredHealingStream)
                position += plasma::Vector<3, cube::Fixed>(creature->m_drawnAttackRotation);
            m_boxes.push_back(Box(lowered(position, 2.5f), startColor, endColor,
                                  (unsigned int)creature->m_entityData.timeSinceAbility, 2.5f, 0.3f, 40));
            m_boxes.push_back(Box(lowered(position, 2.5f), startColor, endColor,
                                  (unsigned int)creature->m_entityData.timeSinceAbility, 5.0f, 0.3f, 40));
        }

        // The beams (ability 29 and, while they last, 104..107), once the anticipation is over and unless the creature
        // is stunned: a line from the head (AppearanceFlagHeadAimOrigin) or the right hand to where the ray along the
        // aim ends.
        if ((creature->m_entityData.currentAbility == cube::AbilitySupportRay ||
             (creature->m_entityData.currentAbility >= cube::AbilityFireBeam &&
              creature->m_entityData.currentAbility <= cube::AbilityWaterBeam &&
              (float)creature->attackDuration() > creature->m_entityData.timeSinceAbility)) &&
            creature->m_entityData.timeSinceAbility >= (float)creature->anticipationTime() &&
            creature->m_entityData.stunTime <= 0.0f &&
            isSphereVisible(creature->m_entityData.position, BeamVisibilityRadius, m_creatureViewDistance)) {
            plasma::Vector<3, float> direction = creature->m_drawnAttackRotation.normalized();
            plasma::Vector<3, cube::Fixed> eye = creature->sightPosition();
            float range = MaximumBeamRange;
            if (creature->m_entityData.currentAbility == cube::AbilitySupportRay) {
                range = std::sqrt(creature->m_drawnAttackRotation[0] * creature->m_drawnAttackRotation[0] +
                                  creature->m_drawnAttackRotation[1] * creature->m_drawnAttackRotation[1] +
                                  creature->m_drawnAttackRotation[2] * creature->m_drawnAttackRotation[2]);
            }
            float reach = m_world->rayDistance(eye, direction, range, false, true, nullptr);
            plasma::Vector<3, cube::Fixed> target = eye + plasma::Vector<3, cube::Fixed>(reach * direction);

            plasma::Vector<3, cube::Fixed> origin;
            if ((creature->m_entityData.appearance.appearanceFlags & cube::AppearanceFlagHeadAimOrigin) &&
                animation.m_headSprite) {
                origin = plasma::Vector<3, cube::Fixed>(-animation.m_originX, -animation.m_originY, cube::Fixed(0)) +
                         plasma::Vector<3, cube::Fixed>(animation.m_headMatrix * center(animation.m_headSprite));
            } else if (animation.m_handsSprite) {
                origin = plasma::Vector<3, cube::Fixed>(-animation.m_originX, -animation.m_originY, cube::Fixed(0)) +
                         plasma::Vector<3, cube::Fixed>(animation.m_rightHandMatrix * center(animation.m_handsSprite));
            } else {
                origin = plasma::Vector<3, cube::Fixed>(-animation.m_originX, -animation.m_originY, cube::Fixed(0)) +
                         plasma::Vector<3, cube::Fixed>(animation.m_rightHandMatrix *
                                                        plasma::Vector<3, float>(0.0f, 0.0f, 0.0f));
            }

            // The colour pulses with the time: for ability 29 by the faction (Cult of Doom orange, Unholy Pact green,
            // else blue), otherwise by the specialization.
            float worldClock = m_world->m_clock;
            float red;
            float green;
            float blue;
            if (creature->m_entityData.currentAbility == cube::AbilitySupportRay) {
                switch (creature->m_entityData.factionKind) {
                case cube::FactionCultOfDoom:
                    // Faction and specialization beam palettes: cosine pulse frequencies, channel amplitudes and base
                    // RGB intensities.
                    green = (float)(cosf(worldClock * 0.03f) * 0.3f + 0.7);
                    red = 1.0f;
                    blue = 0.4f;
                    break;
                case cube::FactionUnholyPact:
                    blue = cosf(worldClock * 0.01f) * 0.3f + 0.4f;
                    red = cosf(worldClock * 0.01f) * 0.3f + 0.3f;
                    green = 1.0f;
                    break;
                default:
                    green = cosf(worldClock * 0.01f) * 0.2f + 0.7f;
                    red = cosf(worldClock * 0.01f) * 0.2f + 0.2f;
                    blue = 1.0f;
                    break;
                }
            } else {
                switch (creature->m_entityData.specialization) {
                case cube::SpecializationWater:
                    green = cosf(worldClock * 0.01f) * 0.3f + 0.3f;
                    red = cosf(worldClock * 0.01f) * 0.3f + 0.4f;
                    blue = 1.0f;
                    break;
                case cube::Specialization2:
                    blue = cosf(worldClock * 0.01f) * 0.3f + 0.4f;
                    red = cosf(worldClock * 0.01f) * 0.3f + 0.3f;
                    green = 1.0f;
                    break;
                default:
                    green = (float)(cosf(worldClock * 0.03f) * 0.3f + 0.7);
                    red = 1.0f;
                    blue = 0.4f;
                    break;
                }
            }
            float width = SupportBeamWidth;
            if (creature->m_entityData.currentAbility == cube::AbilityFireRay ||
                creature->m_entityData.currentAbility == cube::AbilityWaterRay)
                width = ElementalBeamWidth;
            m_lines.push_back(Line(origin, plasma::Vector<3, float>(target - origin),
                                   plasma::Vector<4, float>(red, green, blue, 1.0f), width));
        }

        // Nothing more for a creature out of view. Race 302 is as tall as its attackRotation[2] says.
        float radius = (std::max)(
            creature->m_entityData.appearance.size[2],
            (std::max)(creature->m_entityData.appearance.size[0],
                       // Creature visibility envelope: horizontal size gain and climbing-plant extension along attack
                       // direction.
                       creature->m_entityData.appearance.size[1]) *
                0.6f);
        if (!(creature->m_entityData.race == cube::RaceClimbingPlant
                  ? isSphereVisible(raised(creature->m_drawnPosition, creature->m_entityData.attackRotation[2] * 0.5f),
                                    creature->m_entityData.attackRotation[2] * 1.2f + radius, m_creatureViewDistance)
                  : isSphereVisible(raised(creature->m_drawnPosition, -creature->m_stepOffset), radius,
                                    m_creatureViewDistance)))
            continue;

        // The animation of the ability in progress.
        animation.m_animationTime = creature->m_entityData.timeSinceAbility;
        switch (creature->m_entityData.currentAbility) {
        case cube::AbilityDrink:
            animation.m_animation = cube::AnimationDrink;
            break;
        case cube::AbilityEat:
            animation.m_animation = cube::AnimationEat;
            break;
        case cube::AbilityEatPetFood:
            animation.m_animation = cube::AnimationEatPetFood;
            break;
        case cube::AbilityPresentItem:
            animation.m_animation = cube::AnimationPresentItem;
            break;
        case cube::Ability_125:
            animation.m_animation = cube::Animation_79;
            break;
        case cube::AbilityFloat:
            animation.m_animation = cube::AnimationFloat;
            break;
        case cube::AbilityUseSpecialItem:
            // By what the creature holds (actionItem), when that is of category 24.
            if (creature->m_entityData.actionItem.category == cube::ItemCategorySpecial &&
                creature->m_entityData.actionItem.id == cube::SpecialItemFlute)
                animation.m_animation = cube::AnimationPlayFlute;
            else if (creature->m_entityData.actionItem.category == cube::ItemCategorySpecial &&
                     creature->m_entityData.actionItem.id == cube::SpecialItemHarp)
                animation.m_animation = cube::AnimationPlayHarp;
            else if (creature->m_entityData.actionItem.category == cube::ItemCategorySpecial &&
                     creature->m_entityData.actionItem.id == cube::SpecialItemFiddle)
                animation.m_animation = cube::AnimationPlayFiddle;
            else if (creature->m_entityData.actionItem.category == cube::ItemCategorySpecial &&
                     creature->m_entityData.actionItem.id == cube::SpecialItemWhistle)
                animation.m_animation = cube::AnimationPlayWhistle;
            else if (creature->m_entityData.actionItem.category == cube::ItemCategorySpecial &&
                     creature->m_entityData.actionItem.id == cube::SpecialItemBell)
                animation.m_animation = cube::AnimationPlayBell;
            else if (creature->m_entityData.actionItem.category == cube::ItemCategorySpecial &&
                     creature->m_entityData.actionItem.id == cube::SpecialItemLute)
                animation.m_animation = cube::AnimationPlayLute;
            else if (creature->m_entityData.actionItem.category == cube::ItemCategorySpecial &&
                     creature->m_entityData.actionItem.id == cube::SpecialItemNightingale)
                animation.m_animation = cube::AnimationPlayNightingale;
            else
                animation.m_animation = cube::AnimationPresentItem;
            break;
        case cube::AbilityCallPet:
            animation.m_animation = cube::AnimationCallPet;
            break;
        case cube::AbilityFeedClimbingPlant:
            animation.m_animation = cube::AnimationFeedClimbingPlant;
            break;
        case cube::AbilityOfferPetFood:
            animation.m_animation = cube::AnimationOfferPetFood;
            break;
        case cube::AbilitySit:
            animation.m_animation = cube::AnimationSit;
            break;
        case cube::AbilityLieDown:
            animation.m_animation = cube::AnimationLieDown;
            break;
        case cube::AbilityTortureRackPose:
            animation.m_animation = cube::AnimationTortureRackPose;
            break;
        case cube::AbilityPolePose:
            animation.m_animation = cube::AnimationPolePose;
            break;
        case cube::AbilityCeilingChainPose:
            animation.m_animation = cube::AnimationCeilingChainPose;
            break;
        case cube::AbilitySleep:
        case cube::Ability_90:
            animation.m_animation = cube::AnimationSleep;
            break;
        case cube::AbilityAssassinate:
            animation.m_animation = cube::AnimationAssassinate;
            break;
        case cube::AbilityRockFist:
            animation.m_animation = cube::AnimationRockFist;
            break;
        case cube::AbilityRockFistCharge:
            animation.m_animation = cube::AnimationRockFistCharge;
            break;
        case cube::AbilitySneak:
            animation.m_animation = cube::AnimationSneak;
            break;
        case cube::AbilityDiveBite:
            animation.m_animation = cube::AnimationDiveBite;
            break;
        case cube::AbilityTwoHandedSweep:
        case cube::AbilityCrushStrike:
            animation.m_animation = cube::AnimationTwoHandedSweep;
            break;
        case cube::AbilityTwoHandedSpin:
            animation.m_animation = cube::AnimationTwoHandedSpin;
            break;
        case cube::AbilityGuardianCrushStrike:
        case cube::AbilityTwoHandedSlam:
            animation.m_animation = cube::AnimationGuardianCrushStrike;
            break;
        case cube::AbilityTwoHandedThrust:
            animation.m_animation = cube::AnimationTwoHandedThrust;
            break;
        case cube::AbilityCrush:
            animation.m_animation = cube::AnimationCrush;
            break;
        case cube::AbilityMonsterCharge:
            animation.m_animation = cube::AnimationMonsterCharge;
            break;
        case cube::AbilitySwirl:
            animation.m_animation = cube::AnimationSwirl;
            break;
        case cube::AbilityChargedBoomerang:
            animation.m_animation = cube::AnimationChargedBoomerang;
            break;
        case cube::AbilityParry:
            animation.m_animation = cube::AnimationParry;
            break;
        case cube::AbilityHornCharge:
            animation.m_animation = cube::AnimationHornCharge;
            break;
        case cube::AbilityTailStrike:
            animation.m_animation = cube::AnimationTailStrike;
            break;
        case cube::AbilityScream:
            animation.m_animation = cube::AnimationScream;
            break;
        case cube::AbilityHeroicShout:
            animation.m_animation = cube::AnimationHeroicShout;
            break;
        case cube::AbilityStomp:
            animation.m_animation = cube::AnimationStomp;
            break;
        case cube::AbilityClawStrike:
            animation.m_animation = cube::AnimationClawStrike;
            break;
        case cube::AbilityDashRight:
            animation.m_animation = cube::AnimationDashRight;
            break;
        case cube::AbilityDashLeft:
            animation.m_animation = cube::AnimationDashLeft;
            break;
        case cube::AbilityDashForward:
            animation.m_animation = cube::AnimationDashForward;
            break;
        case cube::AbilityTwoHandedSwing:
            animation.m_animation = cube::AnimationTwoHandedSwing;
            break;
        case cube::AbilityShellBlock:
        case cube::AbilityShellSpin:
            animation.m_animation = cube::AnimationShell;
            break;
        case cube::AbilityLeftSlash:
            animation.m_animation = cube::AnimationLeftSlash;
            break;
        case cube::AbilityRightSlash:
            animation.m_animation = cube::AnimationRightSlash;
            break;
        case cube::Ability_5:
        case cube::AbilityLeftDaggerPierce:
            animation.m_animation = cube::AnimationRightBackhand;
            break;
        case cube::AbilityPerforate:
            animation.m_animation = cube::AnimationPerforate;
            break;
        case cube::Ability_4:
        case cube::AbilityShieldAttack:
        case cube::AbilityRightDaggerPierce:
        case cube::Ability_63:
            animation.m_animation = cube::AnimationLeftBackhand;
            break;
        case cube::AbilityLeftStab:
            animation.m_animation = cube::AnimationLeftStab;
            break;
        case cube::AbilityRightStab:
            animation.m_animation = cube::AnimationRightStab;
            break;
        case cube::AbilityHeadbutt:
            animation.m_animation = cube::AnimationHeadbutt;
            break;
        case cube::AbilityShoot:
        case cube::AbilityChargedShotRelease:
            animation.m_animation = cube::AnimationDrawBow;
            break;
        case cube::AbilityThrowBoomerang:
            animation.m_animation = cube::AnimationThrowBoomerang;
            break;
        case cube::AbilityThrowRock:
            animation.m_animation = cube::AnimationThrowRock;
            break;
        case cube::AbilityFirebolt:
        case cube::AbilityWaterbolt:
            animation.m_animation = cube::AnimationLeftCast;
            break;
        case cube::AbilityBraceletFirebolt:
        case cube::AbilityBraceletWaterbolt:
            animation.m_animation = cube::AnimationRightCast;
            break;
        case cube::AbilityFireball:
        case cube::AbilityWaterball:
        case cube::AbilityFireMissiles:
            animation.m_animation = cube::AnimationFireball;
            break;
        case cube::AbilityMonsterFirebolt:
        case cube::AbilityMonsterWaterbolt:
        case cube::AbilityWaterSalvo:
        case cube::Ability_48:
        case cube::AbilityFireBeam:
        case cube::AbilityFireRay:
        case cube::AbilityWaterRay:
        case cube::AbilityWaterBeam:
            animation.m_animation = cube::AnimationFirebolt;
            break;
        case cube::AbilityFireBreath:
            animation.m_animation = cube::AnimationFireBreath;
            break;
        case cube::AbilitySupportRay:
            animation.m_animation = cube::AnimationSupportRay;
            break;
        case cube::AbilityFireballCharge:
            animation.m_animation = cube::AnimationFireballCharge;
            break;
        case cube::AbilityBlockJab:
            animation.m_animation = cube::AnimationBlockJab;
            break;
        case cube::AbilityIntercept:
            // By the weapons: ids 6 and 7 in the right hand, 13 in the left, another weapon in the left, none.
            if (creature->m_entityData.equipment.weaponRight.category == cube::ItemCategoryWeapon &&
                (creature->m_entityData.equipment.weaponRight.id == cube::WeaponBow ||
                 creature->m_entityData.equipment.weaponRight.id == cube::WeaponCrossbow))
                animation.m_animation = cube::AnimationShoot;
            else if (creature->m_entityData.equipment.weaponLeft.category == cube::ItemCategoryWeapon &&
                     creature->m_entityData.equipment.weaponLeft.id == cube::WeaponShield)
                animation.m_animation = cube::AnimationBlock;
            else if (creature->m_entityData.equipment.weaponLeft.category == cube::ItemCategoryWeapon)
                animation.m_animation = cube::AnimationAmbush;
            else
                animation.m_animation = cube::AnimationIntercept;
            break;
        case cube::AbilityChargedShot:
        case cube::AbilitySalvo:
        case cube::AbilitySalvoRelease:
            animation.m_animation = cube::AnimationShoot;
            break;
        case cube::AbilityBlock:
            animation.m_animation = cube::AnimationBlock;
            break;
        case cube::AbilityMonsterChargeStrike:
        case cube::AbilityAmbush:
            animation.m_animation = cube::AnimationAmbush;
            break;
        case cube::AbilityCrossedWeaponSweep:
            animation.m_animation = cube::AnimationCrossedWeaponSweep;
            break;
        case cube::AbilityKick:
        case cube::AbilityRangerKick:
            animation.m_animation = cube::AnimationKick;
            break;
        case cube::AbilityLeftSweep:
            animation.m_animation = cube::AnimationLeftSweep;
            break;
        case cube::AbilityRightSweep:
            animation.m_animation = cube::AnimationRightSweep;
            break;
        case cube::AbilityOverheadChop:
            animation.m_animation = cube::AnimationOverheadChop;
            break;
        case cube::AbilitySwirlStrike:
        case cube::AbilityBloomlingAttack:
            animation.m_animation = cube::AnimationSwirlStrike;
            break;
        case cube::AbilitySmashLeap:
        case cube::AbilitySmash:
            animation.m_animation = cube::AnimationSmash;
            break;
        case cube::AbilityCyclone:
        case cube::AbilityWarriorJumpAttack:
            animation.m_animation = cube::AnimationCyclone;
            break;
        case cube::AbilityFireSwirl:
        case cube::AbilityFireVortex:
        case cube::AbilityWaterSwirl:
        case cube::AbilityWaterVortex:
        case cube::AbilityHeal:
        case cube::AbilityTeleport:
        case cube::AbilityDjinnExplosion:
        case cube::AbilityFireExplosion:
        case cube::AbilityLavaPuddle:
        case cube::AbilityWaterPuddle:
        case cube::AbilitySummon:
        case cube::AbilityPortCity:
        case cube::Ability_132:
        case cube::AbilitySelfKill:
        case cube::AbilityMarkCreature:
        case cube::AbilityUnmarkCreature:
            animation.m_animation = cube::AnimationCastSpell;
            break;
        case cube::AbilityHealingStream:
        case cube::AbilityCenteredHealingStream:
        case cube::AbilityWaterBubbles:
            animation.m_animation = cube::AnimationHealingStream;
            break;
        case cube::Ability_36:
            animation.m_animation = cube::Animation_68;
            break;
        case cube::AbilityQuake:
            animation.m_animation = cube::AnimationQuake;
            break;
        case cube::AbilityShuriken:
        case cube::AbilityThrowPoisonVial:
            animation.m_animation = cube::AnimationThrow;
            break;
        case cube::AbilityLunge:
            if (creature->m_entityData.equipment.weaponLeft.category == cube::ItemCategoryNone)
                animation.m_animation = cube::AnimationGuardianCrushStrike;
            else
                animation.m_animation = cube::AnimationAmbush;
            break;
        case cube::AbilityBulwark:
        case cube::AbilityManaShield:
            animation.m_animation = cube::AnimationBulwark;
            break;
        case cube::AbilityBlockStrike:
            animation.m_animation = cube::AnimationBlockStrike;
            break;
        case cube::AbilityRidePet:
        case cube::AbilityRideNpcMount:
            animation.m_animation = cube::AnimationRide;
            break;
        case cube::AbilitySailBoat:
            animation.m_animation = cube::AnimationSailBoat;
            break;
        case cube::AbilityCarryCreature:
            animation.m_animation = cube::AnimationCarryCreature;
            break;
        case cube::AbilityThrowCreature:
            animation.m_animation = cube::AnimationThrowCreature;
            break;
        case cube::AbilityRestHeal:
            animation.m_animation = cube::AnimationRestHeal;
            break;
        case cube::AbilityFlightToPoint:
            animation.m_animation = cube::AnimationFlightToPoint;
            break;
        case cube::AbilityWave:
            animation.m_animation = cube::AnimationWave;
            break;
        case cube::AbilityDance:
            animation.m_animation = cube::AnimationDance;
            break;
        case cube::AbilityPetAnimal:
            animation.m_animation = cube::AnimationPetAnimal;
            break;
        default:
            animation.m_animation = cube::AnimationNone;
            break;
        }
        if (creature->m_entityData.actionLockTime != 0.0f)
            animation.m_animation = cube::AnimationActionLocked;

        // The creature's sprites, in the light of the block just below its position (race 276 glows) and tinted by its
        // buffs.
        float light = (float)m_world->cubeLight(
                          creature->m_entityData.position[0], creature->m_entityData.position[1],
                          creature->m_entityData.position[2] - cube::Fixed(CreatureLightSampleDownwardBias), 0) /
                      255.0f;
        if (creature->m_entityData.race == cube::RaceWraith)
            light = 1.0f;
        plasma::Vector<4, float> color(m_tint[0], m_tint[1], m_tint[2], light);
        color[3] *= m_tint[3];
        for (cube::Creature::Buff &buff : creature->m_buffs) {
            switch (buff.type) {
            case cube::BuffPoisoned:
                // Buff tint palettes: RGBA multipliers for poison, red buffs, shadow clone and frozen state.
                color *= plasma::Vector<4, float>(0.5f, 1.5f, 0.5f, 1.0f);
                break;
            case cube::BuffBulwark:
            case cube::BuffWarfrenzy:
            case cube::BuffForcedAggroTarget:
                color *= plasma::Vector<4, float>(1.0f, 0.5f, 0.5f, 1.0f);
                break;
            case cube::BuffShadowclone:
                color *= plasma::Vector<4, float>(0.2f, 0.1f, 0.3f, 1.0f);
                break;
            case cube::BuffFrozen:
                color *= plasma::Vector<4, float>(0.5f, 0.5f, 1.5f, 1.0f);
                break;
            }
        }
        // Stealth, AppearanceFlagSpiritForm and BuffSpiritworld make it translucent. Creature opacity/tint profile:
        // stealth fade gain, spirit and water-bubble opacity, and promoted-resident red tint.
        float alpha = 1.0f - creature->m_entityData.stealth * 0.8f;
        if ((creature->m_entityData.appearance.appearanceFlags & cube::AppearanceFlagSpiritForm) ||
            creature->findBuff(cube::BuffSpiritworld))
            alpha *= 0.5f;
        if (creature->m_entityData.appearance.appearanceFlags & cube::AppearanceFlagPromotedResident)
            color *= plasma::Vector<4, float>(1.0f, 0.5f, 0.5f, 1.0f);
        if (creature->m_entityData.race == cube::RaceWaterBubble)
            alpha *= 0.75f;
        cube::Creature *mount = m_world->creatureByID(creature->m_petId);
        animation.addSprites(m_worldBatch, m_world->m_spriteManager, m_frameTime, color,
                             attenuation(creature->m_drawnPosition, m_creatureViewDistance) * alpha, m_drawOriginX,
                             m_drawOriginY, 0, mount, 0.0f, 1.0f);
        animation.m_spritesInitialized = 1;

        // Draws stars above a stunned creature (sprite 3493) or ability 88's ring at its feet (sprite 3495). Neither
        // sprite is checked for null. The pose uses stun persistence, model scale and clock-driven spin.
        if (creature->m_entityData.stunTime > -3000.0f) {
            cube::Sprite *sprite = m_world->m_spriteManager->sprite(cube::SpriteStars);
            plasma::Matrix<float> model;
            model.identity();
            model.translate((float)(creature->m_drawnPosition[0] + m_drawOriginX),
                            (float)(creature->m_drawnPosition[1] + m_drawOriginY),
                            (float)(creature->m_drawnPosition[2] - cube::Fixed(creature->m_stepOffset) +
                                    cube::Fixed(creature->m_entityData.appearance.size[2] * 0.5f)));
            model.scale(creature->m_entityData.appearance.size[0] * 0.1f,
                        creature->m_entityData.appearance.size[0] * 0.1f,
                        creature->m_entityData.appearance.size[0] * 0.1f);
            model.rotateZ((float)((int)m_world->m_clock % 360000) * 0.2f);
            model.translate((float)sprite->sizeX() * -0.5f, (float)sprite->sizeY() * -0.5f, 0.0f);
            m_worldBatch.add(sprite, model, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 1.0f, 0.0f, 0.0f,
                             0.0f);
        } else if (creature->m_entityData.currentAbility == cube::AbilitySleep) {
            cube::Sprite *sprite = m_world->m_spriteManager->sprite(cube::SpriteZzz);
            plasma::Matrix<float> model;
            model.identity();
            model.translate(
                (float)(creature->m_entityData.position[0] + m_drawOriginX),
                (float)(creature->m_entityData.position[1] + m_drawOriginY),
                (float)(creature->m_entityData.position[2] - cube::Fixed(creature->m_stepOffset) +
                        // Sleep icon pose curve: lift from creature height, pulsing scale and rocking angle in
                        // degrees.
                        cube::Fixed(creature->m_entityData.appearance.size[2] * 0.1f)));
            float pulse = (cosf(creature->m_entityData.timeSinceAbility * 0.005f) * 0.1f + 1.0f) *
                          (creature->m_entityData.appearance.size[0] * 0.07f);
            model.scale(pulse, pulse, pulse);
            model.rotateZ(cosf(creature->m_entityData.timeSinceAbility * 0.001f) * 40.0f + (m_cameraAngle[2] + 180.0f));
            m_worldBatch.add(sprite, model, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 1.0f, 0.0f, 0.0f,
                             0.0f);
        }

        // The abilities 38, 39, 44 and 45 while they are prepared: a spiral of cubes from the head
        // (AppearanceFlagHeadAimOrigin), the left hand (weapon id 12 on an odd combo count) or the right hand along
        // the attack direction, growing with the anticipation. Red, or blue for specialization 1.
        if ((creature->m_entityData.currentAbility == cube::AbilityFireball ||
             creature->m_entityData.currentAbility == cube::AbilityMonsterFirebolt ||
             creature->m_entityData.currentAbility == cube::AbilityWaterball ||
             creature->m_entityData.currentAbility == cube::AbilityMonsterWaterbolt) &&
            (float)creature->anticipationTime() > creature->m_entityData.timeSinceAbility) {
            // Charged spell impact palettes: fire/water RGB triples from inner to outer layer. Spell-bolt spiral
            // profile: fire/water RGB layers, radius, visual-size gain, turn gain and sample count.
            plasma::Vector<4, float> innerColor(1.0f, 0.0f, 0.0f, 1.0f);
            plasma::Vector<4, float> middleColor(1.0f, 0.25f, 0.0f, 1.0f);
            plasma::Vector<4, float> outerColor(1.0f, 0.5f, 0.0f, 1.0f);
            if (creature->m_entityData.specialization == cube::SpecializationWater) {
                innerColor = plasma::Vector<4, float>(0.0f, 0.0f, 1.0f, 1.0f);
                middleColor = plasma::Vector<4, float>(0.0f, 0.25f, 1.0f, 1.0f);
                outerColor = plasma::Vector<4, float>(0.25f, 0.5f, 1.0f, 1.0f);
            }
            float progress = creature->m_entityData.timeSinceAbility / (float)creature->anticipationTime();
            if (creature->m_entityData.currentAbility == cube::AbilityMonsterFirebolt)
                progress *= 0.5f;
            plasma::Vector<3, cube::Fixed> origin = creature->sightPosition();
            if ((creature->m_entityData.appearance.appearanceFlags & cube::AppearanceFlagHeadAimOrigin) &&
                animation.m_headSprite) {
                origin = plasma::Vector<3, cube::Fixed>(-animation.m_originX, -animation.m_originY, cube::Fixed(0)) +
                         plasma::Vector<3, cube::Fixed>(animation.m_headMatrix * center(animation.m_headSprite));
            } else if (creature->m_entityData.equipment.weaponRight.id == cube::WeaponBracelet &&
                       (creature->m_entityData.hitCombo & 1)) {
                if (animation.m_handsSprite) {
                    origin =
                        plasma::Vector<3, cube::Fixed>(-animation.m_originX, -animation.m_originY, cube::Fixed(0)) +
                        plasma::Vector<3, cube::Fixed>(animation.m_leftHandMatrix * center(animation.m_handsSprite));
                } else {
                    origin =
                        plasma::Vector<3, cube::Fixed>(-animation.m_originX, -animation.m_originY, cube::Fixed(0)) +
                        plasma::Vector<3, cube::Fixed>(animation.m_leftHandMatrix *
                                                       plasma::Vector<3, float>(0.0f, 0.0f, 0.0f));
                }
            } else if (animation.m_handsSprite) {
                origin = plasma::Vector<3, cube::Fixed>(-animation.m_originX, -animation.m_originY, cube::Fixed(0)) +
                         plasma::Vector<3, cube::Fixed>(animation.m_rightHandMatrix * center(animation.m_handsSprite));
            }
            // Copies the impact rotation through m_elements, element by element.
            m_impacts.push_back(Impact(origin, creature->m_entityData.attackRotation.normalized() * progress,
                                       plasma::Vector<3, float>(creature->m_entityData.rotation.m_elements[0],
                                                                creature->m_entityData.rotation.m_elements[1],
                                                                creature->m_entityData.rotation.m_elements[2]),
                                       innerColor, middleColor, outerColor,
                                       (int)creature->m_entityData.timeSinceAbility, 1.0f, progress, 1.0f,
                                       ChargedSpellImpactSteps, false));
        }

        // The buffs that show: a ring of rising cubes around the creature (buff 6: cubes arcing over it), each in its
        // colours.
        for (cube::Creature::Buff &buff : creature->m_buffs) {
            switch (buff.type) {
            case cube::BuffManashield:
                m_particles.push_back(Particle(
                    lowered(creature->m_drawnPosition, creature->m_stepOffset),
                    // Mana-shield rising-cube profile: RGB endpoints, radius from creature size, cube size and sample
                    // count.
                    plasma::Vector<4, float>(0.2f, 0.2f, 1.0f, 1.0f), plasma::Vector<4, float>(0.2f, 0.75f, 1.0f, 1.0f),
                    (unsigned int)m_world->m_clock, creature->m_entityData.appearance.size[0] + 0.5f, 0.05f, 40));
                break;
            case cube::BuffInstantfire:
                m_boxes.push_back(Box(lowered(creature->m_drawnPosition, creature->m_stepOffset),
                                      plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f),
                                      plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f), (unsigned int)m_world->m_clock,
                                      creature->m_entityData.appearance.size[0], 0.1f, 20));
                break;
            case cube::BuffInstantshoot:
                m_boxes.push_back(Box(
                    lowered(creature->m_drawnPosition, creature->m_stepOffset),
                    // Instantshoot ring profile: RGB endpoints, radius from creature size, cube size and sample count.
                    plasma::Vector<4, float>(1.0f, 0.0f, 1.0f, 1.0f), plasma::Vector<4, float>(1.0f, 0.5f, 1.0f, 1.0f),
                    (unsigned int)m_world->m_clock, creature->m_entityData.appearance.size[0], 0.1f, 20));
                break;
            case cube::BuffDodge:
                m_boxes.push_back(Box(
                    lowered(creature->m_drawnPosition, creature->m_stepOffset),
                    // Dodge ring profile: RGB endpoints, radius from creature size, cube size and sample count.
                    // Shadowclone ring profile: RGB endpoints, radius from creature size, cube size and sample count.
                    plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f), plasma::Vector<4, float>(0.5f, 0.0f, 1.0f, 1.0f),
                    (unsigned int)m_world->m_clock, creature->m_entityData.appearance.size[0], 0.1f, 20));
                break;
            case cube::BuffShadowclone:
                m_boxes.push_back(Box(lowered(creature->m_drawnPosition, creature->m_stepOffset),
                                      plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f),
                                      plasma::Vector<4, float>(0.3f, 0.0f, 0.6f, 1.0f), (unsigned int)m_world->m_clock,
                                      creature->m_entityData.appearance.size[0], 0.1f, 20));
                break;
            case cube::BuffPoisoned:
                m_boxes.push_back(Box(lowered(creature->m_drawnPosition, creature->m_stepOffset),
                                      plasma::Vector<4, float>(0.0f, 1.0f, 0.0f, 1.0f),
                                      plasma::Vector<4, float>(0.5f, 1.0f, 0.2f, 1.0f), (unsigned int)m_world->m_clock,
                                      creature->m_entityData.appearance.size[0], 0.1f, 20));
                break;
            }
        }

        // Ability 136: a wide spiral five blocks along the attack direction, cut off at the first block in its way.
        if (creature->m_entityData.currentAbility == cube::AbilityFireBreath &&
            (float)creature->attackDuration() > creature->m_entityData.timeSinceAbility) {
            float progress = creature->m_entityData.timeSinceAbility /
                             (float)(creature->anticipationTime() + creature->strikeTime());
            m_impacts.push_back(Impact(
                creature->m_drawnPosition, creature->m_entityData.attackRotation.normalized() * 5.0f,
                plasma::Vector<3, float>(creature->m_entityData.rotation.m_elements[0],
                                         creature->m_entityData.rotation.m_elements[1],
                                         creature->m_entityData.rotation.m_elements[2]),
                plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f), plasma::Vector<4, float>(1.0f, 0.5f, 0.0f, 1.0f),
                plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f),
                // Fire-breath spiral profile: width, progress curve, cube scale and 100 samples.
                (int)creature->m_entityData.timeSinceAbility, 2.0f, progress, 1.0f, 100, true));
        }

        // The races that are always ringed: 278 in white, and, unless interactionState is set, 185 and 221 in fire
        // colours, 222 in blue and 223 in green.
        if (creature->m_entityData.race == cube::RaceShrineOfTheLight) {
            m_boxes.push_back(Box(lowered(creature->m_drawnPosition, creature->m_stepOffset),
                                  plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f),
                                  plasma::Vector<4, float>(1.0f, 1.0f, 0.5f, 1.0f),
                                  (unsigned int)creature->m_entityData.timeSinceAbility,
                                  creature->m_entityData.appearance.size[0], 0.05f, 20));
        }
        if (creature->m_entityData.interactionState == 0) {
            switch (creature->m_entityData.race) {
            case cube::RaceUnholyBonfire: {
                // Unholy-bonfire ring profile: ground lift, green RGB endpoints, radius/cube size and sample count.
                // Ghost-brazier ring profile: ground lift, blue RGB endpoints, radius/cube size and sample count.
                // Brazier ring profile: ground lift, flame RGB endpoints, radius/cube size and sample count.
                plasma::Vector<3, cube::Fixed> offset =
                    plasma::Vector<3, cube::Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, 0.1f));
                m_boxes.push_back(Box(
                    creature->m_entityData.position + offset, plasma::Vector<4, float>(0.2f, 1.0f, 0.2f, 1.0f),
                    plasma::Vector<4, float>(0.25f, 1.0f, 0.5f, 1.0f), (unsigned int)m_world->m_clock, 0.8f, 0.2f, 20));
                break;
            }
            case cube::RaceGhostBrazier: {
                plasma::Vector<3, cube::Fixed> offset =
                    plasma::Vector<3, cube::Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, 0.1f));
                m_boxes.push_back(Box(
                    creature->m_entityData.position + offset, plasma::Vector<4, float>(0.0f, 0.2f, 1.0f, 1.0f),
                    plasma::Vector<4, float>(0.25f, 0.5f, 1.0f, 1.0f), (unsigned int)m_world->m_clock, 0.8f, 0.2f, 20));
                break;
            }
            case cube::RaceBrazier: {
                plasma::Vector<3, cube::Fixed> offset =
                    plasma::Vector<3, cube::Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, 0.1f));
                m_boxes.push_back(Box(
                    creature->m_entityData.position + offset, plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f),
                    plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f), (unsigned int)m_world->m_clock, 0.8f, 0.2f, 20));
                break;
            }
            case cube::RaceCampfire: {
                plasma::Vector<3, cube::Fixed> position = creature->m_entityData.position;
                // Campfire ring profile: ground lift, flame RGB endpoints, radius/cube size and sample count.
                position += plasma::Vector<3, cube::Fixed>(cube::Fixed(0.0f), cube::Fixed(0.0f), cube::Fixed(0.1f));
                m_boxes.push_back(Box(position, plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f),
                                      plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f), (unsigned int)m_world->m_clock,
                                      0.8f, 0.2f, 20));
                break;
            }
            }
        }

        // The abilities 25 and 26: a short pale spiral one block along the attack direction, as wide as half the
        // charge.
        if (creature->m_entityData.currentAbility == cube::AbilityChargedShot ||
            creature->m_entityData.currentAbility == cube::AbilitySalvo) {
            plasma::Vector<3, cube::Fixed> step =
                plasma::Vector<3, cube::Fixed>(creature->m_entityData.attackRotation.normalized());
            float width = creature->m_entityData.charge * 0.5f;
            m_impacts.push_back(Impact(
                creature->m_entityData.position + step, creature->m_entityData.attackRotation.normalized(),
                plasma::Vector<3, float>(creature->m_entityData.rotation.m_elements[0],
                                         creature->m_entityData.rotation.m_elements[1],
                                         creature->m_entityData.rotation.m_elements[2]),
                // Charged-shot spiral profile: three pale-blue RGB layers and 20 samples. Knockback trail spiral
                // profile: three pale-blue RGB layers and 20 samples.
                plasma::Vector<4, float>(0.5f, 0.5f, 1.0f, 1.0f), plasma::Vector<4, float>(0.75f, 0.75f, 1.0f, 1.0f),
                plasma::Vector<4, float>(0.9f, 0.9f, 1.0f, 1.0f), (int)creature->m_entityData.timeSinceAbility, width,
                width, 1.0f, 20, false));
        }

        // The abilities 96 and 97: four rings in fire colours over the creature, which rise and widen during the
        // anticipation and close again right after it.
        if ((creature->m_entityData.currentAbility == cube::AbilityDjinnExplosion ||
             creature->m_entityData.currentAbility == cube::AbilityFireExplosion) &&
            (float)creature->attackDuration() > creature->m_entityData.timeSinceAbility) {
            float progress = creature->m_entityData.timeSinceAbility / (float)creature->anticipationTime();
            if (progress > 1.0f)
                // Explosion ring curves: closing slope, four rising radii, elevation offset and per-ring sample
                // growth.
                progress = std::max(0.0f, 20.0f - progress * 19.0f);
            for (int i = 0; i < 4; i++) {
                plasma::Vector<3, cube::Fixed> offset = plasma::Vector<3, cube::Fixed>(
                    plasma::Vector<3, float>(0.0f, 0.0f, creature->m_stepOffset + 1.0f + progress * 4.0f));
                m_boxes.push_back(Box(creature->m_entityData.position - offset,
                                      plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f),
                                      plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f), (unsigned int)m_world->m_clock,
                                      (float)i * 2.0f * progress + 1.0f, progress * 0.5f, 20 + i * 13));
            }
        }

        // A creature whose weapon trails have moved gets them drawn.
        if (animation.m_trails[0][0] != animation.m_trails[0][15] ||
            animation.m_trails[2][0] != animation.m_trails[2][15])
            m_trailCreatures.push_back(creature);

        // A soft round shadow under the creature when it is near, except for hostility type 8, gates and the races
        // 248..250, 268, 275 and 282..285.
        if (creature->m_entityData.hostilityType != cube::HostilityInteractiveObject &&
            creature->m_entityData.race != cube::RaceCeilingChain &&
            !(creature->m_entityData.race >= cube::RaceFootprints &&
              creature->m_entityData.race <= cube::RaceWaterBubble) &&
            !creature->isGate() &&
            !(creature->m_entityData.race >= cube::RaceWall && creature->m_entityData.race <= cube::RaceMagicBarrier) &&
            creature->m_entityData.race != cube::RaceRunePortal) {
            float range = m_effectViewDistance;
            // Creature shadow profile: squared-range fraction, opacity and size gain.
            if (range * 0.25f * range >
                plasma::Vector<3, float>(creature->m_drawnPosition - m_cameraPosition).squaredLength()) {
                GroundSpot spot;
                spot.kind = 0;
                spot.color = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f,
                                                      attenuation(creature->m_drawnPosition, range * 0.5f) * 0.3f);
                spot.position =
                    creature->m_drawnPosition + plasma::Vector<3, cube::Fixed>(plasma::Vector<3, float>(
                                                    0.0f, 0.0f, creature->m_entityData.appearance.size[2] * -0.5f));
                spot.radius = 1.5f;
                m_groundSpots.push_back(spot);
            }
        }

        // The sign over the creature's head: by its class, the map's mark (orange for the map's chosen creature), the
        // sprite the animation state shows for two seconds (m_emoteSprite, popping up and fading; 3569 sinks, 3566 and
        // 3567 alternate and rock), or buff 7's, sized by the buff.
        color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        cube::Sprite *sign = nullptr;
        unsigned char classType = creature->m_entityData.classType;
        if (classType >= cube::ClassItemVendor && classType <= cube::ClassArmorVendor)
            sign = m_world->m_spriteManager->sprite(cube::SpriteIconVendor);
        if (classType == cube::ClassGuildReceptionist)
            sign = m_world->m_spriteManager->sprite(cube::SpriteMission);
        if (classType == cube::ClassAnalyzer)
            sign = m_world->m_spriteManager->sprite(cube::SpriteIconAnalyze);
        if (classType == cube::Class_139)
            sign = m_world->m_spriteManager->sprite(cube::SpriteMission);
        if (m_worldMap->isMarked(creature)) {
            sign = m_world->m_spriteManager->sprite(cube::SpriteQuestTarget);
            if (creature->m_id == m_worldMap->m_markedCreatureId)
                // Selected map-target sign RGBA palette: orange highlight.
                color = plasma::Vector<4, float>(1.0f, 0.25f, 0.0f, 1.0f);
            else
                color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        }
        float signScale = 1.0f;
        float signLift = 0.0f;
        float signTilt = 0.0f;
        if (animation.m_emoteSprite) {
            sign = animation.m_emoteSprite;
            color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
            int age = animation.m_emoteTime;
            // Emote pose curves: millisecond pop-in/fade intervals, sinking, rocking and atlas-frame cadence.
            if (age < 200) {
                double angle = (float)age / 200.0f * 3.14159265358979323846;
                signScale = sinf((float)(angle * 0.6f)) / sinf((float)(3.14159265358979323846 * 0.6f));
                signLift = sinf((float)angle) * 0.5f;
            }
            if (age > 1800)
                signScale = 1.0f - (float)(age - 1800) / 200.0f;
            cube::SpriteManager *sprites = m_world->m_spriteManager;
            if (animation.m_emoteSprite == sprites->sprite(cube::SpriteEmoteSweat)) {
                signLift += 0.5f - (float)age * 0.0005f;
            } else if (animation.m_emoteSprite == sprites->sprite(cube::SpriteAngry) ||
                       animation.m_emoteSprite == sprites->sprite(cube::SpriteAngry2)) {
                signTilt = sinf((float)age * 0.01f) * 10.0f + 45.0f;
                sign = sprites->sprite(age / 250 % 2 + cube::SpriteAngry);
            }
        } else {
            cube::Creature::Buff *buff = creature->findBuff(cube::BuffTameness);
            if (buff) {
                sign = m_world->m_spriteManager->sprite(cube::SpriteHeartSlot3568);
                signScale = buff->strength + 0.5f;
            }
        }
        if (sign) {
            plasma::Matrix<float> model;
            model.identity();
            cube::Fixed height(creature->m_entityData.appearance.size[2] * 0.5f + 1.0f - creature->m_stepOffset +
                               signLift);
            model.translate(
                plasma::Vector<3, float>(creature->m_entityData.position +
                                         plasma::Vector<3, cube::Fixed>(m_drawOriginX, m_drawOriginY, height)));
            model.rotate(creature->m_entityData.rotation[2] + 0.0f, 0.0f, 0.0f, 1.0f);
            model.rotateY(signTilt);
            float scale = OverheadSignWidth / (float)sign->sizeX() * signScale;
            model.scale(scale, scale, scale);
            model.translate((float)sign->sizeX() * -0.5f, (float)sign->sizeY() * -0.5f, (float)sign->sizeZ() * -0.5f);
            m_worldBatch.add(sign, model, color, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    // The items on the ground and the static entities of the seven by seven zones around the player.
    int playerZoneX = cube::zoneCoordinate(m_world->m_localCreature->m_entityData.position[0]);
    int playerZoneY = cube::zoneCoordinate(m_world->m_localCreature->m_entityData.position[1]);
    if (flags & RenderFlagOnlyPlayer)
        return;

    for (int x = playerZoneX - 3; x <= playerZoneX + 3; x++) {
        for (int y = playerZoneY - 3; y <= playerZoneY + 3; y++) {
            cube::Zone *zone = m_world->zone(x, y);
            if (!zone)
                continue;

            int index = -1;
            for (cube::ZoneItem &entry : zone->m_items) {
                index++;
                if (entry.hiddenTimeRemaining != 0)
                    continue;
                cube::Item item = entry.item.transformed(m_world->m_localCreature);
                cube::Sprite *sprite = m_world->m_spriteManager->itemSprite(&item);
                if (!sprite)
                    continue;
                float scale = item.scale();
                if (!isSphereVisible(entry.position, (float)(std::max)(sprite->sizeX(), sprite->sizeZ()) * scale,
                                     m_effectViewDistance))
                    continue;
                // A dropped item (flags counts down from 500) flies an arc five blocks high.
                plasma::Vector<3, cube::Fixed> position = entry.position;
                if (entry.dropTime > 0.0f) {
                    float flight = (entry.dropTime - 250.0f) / 250.0f;
                    position[2] += cube::Fixed((1.0f - flight * flight) * 5.0f);
                }
                if (!m_world->isGroundItemAvailable(entry))
                    continue;

                plasma::Matrix<float> model;
                model.identity();
                model.translate((float)(position[0] + m_drawOriginX), (float)(position[1] + m_drawOriginY),
                                (float)position[2]);
                model.scale(scale, scale, scale);
                // In the light of its block, except what shines by itself.
                plasma::Vector<4, float> color(
                    1.0f, 1.0f, 1.0f,
                    (float)m_world->cubeLight(entry.position[0], entry.position[1], entry.position[2], 0) / 255.0f);
                if ((item.category == cube::ItemCategoryMaterial &&
                     (item.id == cube::MaterialItemShimmerMushroom || item.id == cube::MaterialItemSpirit)) ||
                    item.category == cube::ItemCategoryCandle)
                    color[3] = 1.0f;

                // How it lies or stands: by category, with the items 63 and 64 of category 22 shown as the categories
                // 3 and 4 and everything whose own category is 30 as that.
                int kind = item.category;
                if (item.category == cube::ItemCategoryQuest) {
                    if (item.id == cube::QuestItemWeapon)
                        kind = cube::ItemCategoryWeapon;
                    else if (item.id == cube::QuestItemChestArmor)
                        kind = cube::ItemCategoryChest;
                }
                if (entry.item.category == cube::ItemCategoryGeneratedItem)
                    kind = cube::ItemCategoryGeneratedItem;
                float time = m_world->m_clock;
                float wave;
                switch (kind) {
                case cube::ItemCategoryWeapon:
                case cube::ItemCategoryChest:
                    model.rotateZ(entry.rotation);
                    model.translate(0.0f, 0.0f, (float)sprite->sizeX() * 0.5f);
                    model.rotateY(90.0f);
                    model.translate((float)sprite->sizeX() * -0.5f, (float)sprite->sizeY() * -0.5f,
                                    (float)sprite->sizeZ() * -0.5f);
                    break;
                case cube::ItemCategoryCoin:
                case cube::ItemCategoryPlatinumCoin:
                    // The glowing kinds brighten and dim (once in 900 ms).
                    wave = cosf((float)((time + time) * 0.2f * 3.14159265358979323846 / 180.0)) * 0.5f + 0.5f;
                    color *= wave * wave + 1.0f;
                    // Dropped coin/special-item pose curves: spin rates, model tilt and vertical presentation offset.
                    model.rotateZ(time * 0.2f + entry.rotation);
                    model.translate(0.0f, 0.0f, (float)sprite->sizeX() * 0.5f);
                    model.rotateY(90.0f);
                    model.translate((float)sprite->sizeX() * -0.5f, (float)sprite->sizeY() * -0.5f,
                                    (float)sprite->sizeZ() * -0.5f);
                    break;
                case cube::ItemCategoryManaCube:
                    wave = cosf((float)((time + time) * 0.2f * 3.14159265358979323846 / 180.0)) * 0.5f + 0.5f;
                    color *= wave * wave + 1.0f;
                    model.rotateZ(time * 0.1f + entry.rotation);
                    model.translate(0.0f, 0.0f, (float)sprite->sizeX() * 0.85f);
                    model.rotateY(45.0f);
                    model.rotateX(45.0f);
                    model.translate((float)sprite->sizeX() * -0.5f, (float)sprite->sizeY() * -0.5f,
                                    (float)sprite->sizeZ() * -0.5f);
                    break;
                case cube::ItemCategoryArtifact:
                case cube::ItemCategorySpecial:
                case cube::ItemCategoryGlyph:
                case cube::ItemCategoryGeneratedItem:
                    wave = cosf((float)((time + time) * 0.2f * 3.14159265358979323846 / 180.0)) * 0.5f + 0.5f;
                    color *= wave * wave + 1.0f;
                    model.rotateZ(time * 0.1f + entry.rotation);
                    model.translate(0.0f, 0.0f, (float)sprite->sizeX() * 0.85f);
                    model.translate((float)sprite->sizeX() * -0.5f, (float)sprite->sizeY() * -0.5f,
                                    (float)sprite->sizeZ() * -0.5f);
                    break;
                default:
                    if (item.category == cube::ItemCategoryMaterial && item.id == cube::MaterialItemSpirit) {
                        // Spirit dropped-item pose: spin rate, tilt in degrees and scale gain.
                        model.rotateZ(time * 0.02f);
                        model.translate(0.0f, 0.0f, (float)sprite->sizeZ());
                        model.rotateX(45.0f);
                        model.rotateY(45.0f);
                        model.scale(5.0f, 5.0f, 5.0f);
                        model.translate((float)sprite->sizeX() * -0.5f, (float)sprite->sizeY() * -0.5f,
                                        (float)sprite->sizeZ() * -0.5f);
                    } else {
                        model.rotateZ(entry.rotation);
                        model.translate((float)sprite->sizeX() * -0.5f, (float)sprite->sizeY() * -0.5f, 0.0f);
                    }
                    break;
                }
                // A dropped item tumbles around its centre.
                if (entry.dropTime != 0.0f) {
                    model.translate((float)sprite->sizeX() * 0.5f, (float)sprite->sizeY() * 0.5f,
                                    (float)sprite->sizeZ() * 0.5f);
                    model.rotateX(entry.dropTime);
                    model.rotateY(entry.dropTime * DroppedItemYTumbleGain);
                    model.translate((float)sprite->sizeX() * -0.5f, (float)sprite->sizeY() * -0.5f,
                                    (float)sprite->sizeZ() * -0.5f);
                }

                // The item looked at (zone and index) is brightened.
                plasma::Vector<4, float> tint = color;
                if (plasma::Vector<3, int>(x, y, index) == lookedAt)
                    tint += plasma::Vector<4, float>(LookedAtItemTintIncrement, LookedAtItemTintIncrement,
                                                     LookedAtItemTintIncrement, LookedAtItemTintIncrement);
                float alpha = attenuation(entry.position, m_effectViewDistance) * item.alpha();
                m_worldBatch.add(sprite, model, tint, alpha, 0.0f, item.shininess(), 0.0f, 0.0f);
                if (item.category == cube::ItemCategoryWeapon) {
                    m_worldBatch.addSpirits(&item, m_world->m_spriteManager->sprite(cube::SpriteCube), model, tint,
                                            0.0f, alpha, 0.0f, item.shininess(), 0.0f, 0.0f);
                }

                // Equipment that lies still and was dropped for the player (ownerSeed is the character's seed) is
                // ringed in its rarity's colour, in a named world.
                if (entry.dropTime == 0.0f && item.isEquipment() && !m_world->m_worldName.empty()) {
                    cube::Creature *player = m_world->m_localCreature;
                    if (entry.ownerSeed == player->m_entityData.seed) {
                        m_boxes.push_back(
                            Box(entry.position, item.textColor(player->m_entityData.region),
                                // Owned-equipment rarity-ring profile: colour/radius gains, cube size and sample
                                // count.
                                item.textColor(player->m_entityData.region) * 0.9f, (unsigned int)m_world->m_clock,
                                scale * 10.0f, 0.1f, 8));
                    }
                }
            }

            for (cube::StaticEntity &entity : zone->m_staticEntities) {
                plasma::Vector<3, float> size = entity.scaledSize();
                float radius = std::max(size[1], size[0]);
                radius = std::max(size[2], radius);
                plasma::Vector<3, cube::Fixed> middle = entity.position;
                middle[2] += cube::Fixed(size[2] * 0.5);
                if (!isSphereVisible(middle, radius + 1.0f, m_effectViewDistance))
                    continue;
                placeStaticEntity(entity);
                // A fire ring over the campfire (83), over type 126 while it is not active, and over type 65 at night.
                switch (entity.type) {
                case cube::StaticEntityCampfire:
                    m_boxes.push_back(Box(entity.position + plasma::Vector<3, cube::Fixed>(
                                                                plasma::Vector<3, float>(0.0f, 0.0f, size[2] * 0.5f)),
                                          plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f),
                                          plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f),
                                          (unsigned int)m_world->m_clock, 0.8f, 0.2f, 20));
                    // Falls through: an inactive campfire gets the ring twice.
                case cube::StaticEntityBrazier:
                    if (entity.active == 0) {
                        plasma::Vector<3, cube::Fixed> rise =
                            plasma::Vector<3, cube::Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, size[2] * 0.5f));
                        m_boxes.push_back(Box(entity.position + rise, plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f),
                                              plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f),
                                              (unsigned int)m_world->m_clock, 0.8f, 0.2f, 20));
                    }
                    break;
                case cube::StaticEntityStreetLight01:
                    // Night streetlight ring profile: daylight cutoff, radius/cube size and sample count.
                    if (0.4f > m_daylight) {
                        m_boxes.push_back(Box(entity.position + plasma::Vector<3, cube::Fixed>(
                                                                    plasma::Vector<3, float>(0.0f, 0.0f, size[2])),
                                              plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f),
                                              plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f),
                                              (unsigned int)m_world->m_clock, 0.5f, 0.2f, 20));
                    }
                    break;
                }
            }
        }
    }

    // The projectiles: a sprite by type, turned along the flight, and the effects of the types that have some. The
    // types 8 and 9 are seen as far as the view reaches.
    for (cube::ServerUpdates::BulletPacket &projectile : m_world->m_projectiles) {
        float range = m_effectViewDistance;
        if (projectile.projectileType == cube::ProjectileEagle ||
            projectile.projectileType == cube::ProjectileSteelEmpireAirship)
            range = m_viewDistance;
        if (!isSphereVisible(projectile.position, projectile.sizeInBlocks, range))
            continue;
        float alpha = attenuation(projectile.position, range);
        cube::Sprite *sprite;
        switch (projectile.projectileType) {
        // Arrows, bullets and the like (0..2) trail a pale spiral when knockback is set.
        case cube::ProjectileArrow:
            sprite = m_world->m_spriteManager->sprite(cube::SpriteArrow);
            if (projectile.knockback != 0.0f) {
                plasma::Vector<3, cube::Fixed> step = plasma::Vector<3, cube::Fixed>(projectile.velocity.normalized());
                m_impacts.push_back(
                    Impact(projectile.position + step, projectile.velocity.normalized(),
                           plasma::Vector<3, float>(0.0f, 0.0f, 0.0f), plasma::Vector<4, float>(0.5f, 0.5f, 1.0f, 1.0f),
                           plasma::Vector<4, float>(0.75f, 0.75f, 1.0f, 1.0f),
                           plasma::Vector<4, float>(0.9f, 0.9f, 1.0f, 1.0f), (int)projectile.currentTime,
                           projectile.knockback * 0.5f, projectile.knockback, 1.0f, 20, false));
            }
            break;
        case cube::ProjectileBullet:
            sprite = m_world->m_spriteManager->sprite(cube::SpriteBullet);
            if (projectile.knockback != 0.0f) {
                plasma::Vector<3, cube::Fixed> step = plasma::Vector<3, cube::Fixed>(projectile.velocity.normalized());
                m_impacts.push_back(
                    Impact(projectile.position + step, projectile.velocity.normalized(),
                           plasma::Vector<3, float>(0.0f, 0.0f, 0.0f), plasma::Vector<4, float>(0.5f, 0.5f, 1.0f, 1.0f),
                           plasma::Vector<4, float>(0.75f, 0.75f, 1.0f, 1.0f),
                           plasma::Vector<4, float>(0.9f, 0.9f, 1.0f, 1.0f), (int)projectile.currentTime,
                           projectile.knockback * 0.5f, projectile.knockback, 1.0f, 20, false));
            }
            break;
        case cube::ProjectileShuriken:
            sprite = m_world->m_spriteManager->sprite(cube::SpriteShuriken);
            if (projectile.knockback != 0.0f) {
                plasma::Vector<3, cube::Fixed> step = plasma::Vector<3, cube::Fixed>(projectile.velocity.normalized());
                m_impacts.push_back(
                    Impact(projectile.position + step, projectile.velocity.normalized(),
                           plasma::Vector<3, float>(0.0f, 0.0f, 0.0f), plasma::Vector<4, float>(0.5f, 0.5f, 1.0f, 1.0f),
                           plasma::Vector<4, float>(0.75f, 0.75f, 1.0f, 1.0f),
                           plasma::Vector<4, float>(0.9f, 0.9f, 1.0f, 1.0f), (int)projectile.currentTime,
                           projectile.knockback * 0.5f, projectile.knockback, 1.0f, 20, false));
            }
            break;
        case cube::ProjectileMedicine:
            sprite = m_world->m_spriteManager->sprite(cube::SpriteMedicine);
            break;
        // Type 5 is the owner's weapon when that is a boomerang (category 3, id 8).
        case cube::ProjectileBoomerang:
            if (m_world->creatureByID(projectile.ownerId) &&
                m_world->creatureByID(projectile.ownerId)->m_entityData.equipment.weaponRight.category ==
                    cube::ItemCategoryWeapon &&
                m_world->creatureByID(projectile.ownerId)->m_entityData.equipment.weaponRight.id ==
                    cube::WeaponBoomerang) {
                sprite = m_world->m_spriteManager->itemSprite(
                    &m_world->creatureByID(projectile.ownerId)->m_entityData.equipment.weaponRight);
            } else {
                sprite = m_world->m_spriteManager->sprite(cube::SpriteArrow);
            }
            break;
        case cube::ProjectileRock:
            sprite = m_world->m_spriteManager->sprite(cube::SpriteIronDeposit);
            break;
        case cube::ProjectileEagle:
            sprite = m_world->m_spriteManager->sprite(cube::SpriteEagle);
            break;
        case cube::ProjectileSteelEmpireAirship:
            sprite = m_world->m_spriteManager->sprite(cube::SpriteSteelEmpireAirship);
            break;
        // Type 3 has no sprite: a spiral six times its size along the flight, in fire colours or, for an owner of
        // specialization 1, in blue.
        case cube::ProjectileSpellBolt: {
            plasma::Vector<3, float> length = projectile.velocity.normalized() * projectile.visualSize * 6.0f;
            plasma::Vector<4, float> innerColor(1.0f, 0.0f, 0.0f, 1.0f);
            plasma::Vector<4, float> middleColor(1.0f, 0.25f, 0.0f, 1.0f);
            plasma::Vector<4, float> outerColor(1.0f, 0.5f, 0.0f, 1.0f);
            if (m_world->creatureByID(projectile.ownerId) &&
                m_world->creatureByID(projectile.ownerId)->m_entityData.specialization == cube::SpecializationWater) {
                innerColor = plasma::Vector<4, float>(0.0f, 0.0f, 1.0f, 1.0f);
                middleColor = plasma::Vector<4, float>(0.0f, 0.25f, 1.0f, 1.0f);
                outerColor = plasma::Vector<4, float>(0.25f, 0.5f, 1.0f, 1.0f);
            }
            plasma::Vector<3, cube::Fixed> half = plasma::Vector<3, cube::Fixed>(length * 0.5f);
            m_impacts.push_back(Impact(projectile.position - half, length, plasma::Vector<3, float>(0.0f, 0.0f, 0.0f),
                                       innerColor, middleColor, outerColor, (int)projectile.currentTime, 0.1f,
                                       projectile.visualSize * 5.0f, 0.75f, 20, false));
            continue;
        }
        // Type 4: two fire rings around its sprite.
        case cube::ProjectileFireball:
            sprite = m_world->m_spriteManager->sprite(cube::SpriteFireball);
            m_boxes.push_back(Box(projectile.position, plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f),
                                  plasma::Vector<4, float>(1.0f, 0.5f, 0.0f, 1.0f),
                                  (unsigned int)projectile.currentTime, 1.0f, 0.3f, 40));
            m_boxes.push_back(Box(projectile.position, plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f),
                                  plasma::Vector<4, float>(1.0f, 0.5f, 0.0f, 1.0f),
                                  (unsigned int)projectile.currentTime, 0.5f, 0.3f, 40));
            break;
        // Type 6 is a mark on the ground of the projectile's size: the ring texture growing solid (ElementQuicksand)
        // or a pulsing spot in the colour of its element, with a fire ring for ElementFlameRush.
        case cube::ProjectileGroundSpot: {
            if (projectile.element == cube::ElementFlameRush) {
                m_boxes.push_back(Box(projectile.position, plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f),
                                      plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f),
                                      (unsigned int)projectile.currentTime, 0.8f, 0.2f, 20));
            }
            GroundSpot spot;
            spot.position = projectile.position;
            spot.radius = projectile.sizeInBlocks;
            if (projectile.element == cube::ElementQuicksand) {
                spot.kind = 1;
                // Quicksand ground-spot palette and opacity curve: warm RGB with a linear fade-in.
                spot.color =
                    plasma::Vector<4, float>(0.8f, 0.6f, 0.2f, (std::min)(projectile.currentTime * 0.002f, 1.0f));
            } else {
                spot.kind = 0;
                if (projectile.element == cube::ElementFire || projectile.element == cube::ElementFlameRush) {
                    // Element ground-spot pulse palettes: channel frequencies, amplitudes and base RGB intensities.
                    spot.color =
                        plasma::Vector<4, float>(1.0f, cosf(projectile.currentTime * 0.01f) * 0.2f + 0.2f, 0.0f, 1.0f);
                } else if (projectile.element == cube::ElementPoison) {
                    spot.color =
                        plasma::Vector<4, float>(cosf(projectile.currentTime * 0.005f) * 0.2f + 0.2f, 0.8f, 0.1f, 1.0f);
                } else {
                    spot.color =
                        plasma::Vector<4, float>(0.0f, cosf(projectile.currentTime * 0.01f) * 0.2f + 0.6f, 1.0f, 1.0f);
                }
            }
            m_groundSpots.push_back(spot);
            continue;
        }
        default:
            continue;
        }
        if (!sprite)
            continue;

        // The types 3 and 4 glow; the others take the light of their block.
        float light = 1.0f;
        if (projectile.projectileType != cube::ProjectileSpellBolt &&
            projectile.projectileType != cube::ProjectileFireball) {
            light =
                (float)m_world->cubeLight(projectile.position[0], projectile.position[1], projectile.position[2], 0) /
                255.0f;
        }
        plasma::Vector<4, float> color = m_tint * plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, light);
        plasma::Matrix<float> model;
        model.identity();
        model.translate(plasma::Vector<3, float>(
            projectile.position + plasma::Vector<3, cube::Fixed>(m_drawOriginX, m_drawOriginY, cube::Fixed(0))));
        // The boomerang spins, the types 7 and 10 roll around the axis across their flight, type 2 turns flat, and
        // everything else points where it flies.
        if (projectile.projectileType == cube::ProjectileBoomerang) {
            model.rotateY(std::min(90.0f, projectile.currentTime * ProjectileSpinRate));
            model.rotateX(projectile.currentTime * BoomerangSpinRate);
        } else if (projectile.projectileType == cube::ProjectileRock) {
            model.rotate(projectile.currentTime * ProjectileSpinRate,
                         projectile.velocity.cross(plasma::Vector<3, float>(0.0f, 0.0f, 1.0f)));
        } else if (projectile.projectileType == cube::ProjectileMedicine) {
            model.rotate(projectile.currentTime * ProjectileSpinRate,
                         projectile.velocity.cross(plasma::Vector<3, float>(0.0f, 0.0f, 1.0f)));
        } else if (projectile.projectileType == cube::ProjectileShuriken) {
            model.rotateZ(projectile.currentTime * ProjectileSpinRate);
        } else if (projectile.velocity.squaredLength() > 0.0f) {
            rotateTowards(model, plasma::Vector<3, float>(0.0f, 1.0f, 0.0f), projectile.velocity.normalized());
        }
        model.scale(projectile.visualSize / ProjectileSpriteSizeDivisor,
                    projectile.visualSize / ProjectileSpriteSizeDivisor,
                    projectile.visualSize / ProjectileSpriteSizeDivisor);
        model.translate((float)sprite->sizeX() * -0.5f, (float)sprite->sizeY() * -0.5f, (float)sprite->sizeZ() * -0.5f);
        m_worldBatch.add(sprite, model, color, alpha, 0.0f, 0.0f, 0.0f, 0.0f);
    }

    // The shadows are drawn farthest first.
    std::sort(m_shadows.begin(), m_shadows.end(),
              [this](const GroundShadow &a, const GroundShadow &b) { return a.squaredDistance > b.squaredDistance; });
}

void Renderer::drawBox(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<4, float> &startColor,
                       const plasma::Vector<4, float> &endColor, unsigned int time, float radius, float scale,
                       int count)
{
    for (int i = 0; i < count; i++) {
        // Rising-ring particle curves: per-index phase salt, lifetime/pulse/orbit periods and angular distribution.
        // Expanding-spiral particle curves: phase salt, lifetime/pulse/orbit periods and quadratic turn gain.
        // Arcing-particle curves: phase salt, lifetime/pulse/orbit periods and slowed pulse clock.
        float progress = (float)((time + i * 443) % 2000) / 2000.0f;
        float pulse = cosf((float)(i * 13 + (float)(time % 500) / 500.0f * 2.0f * cube::Pi));
        float fade = (progress - 0.5f) * 2.0f;
        float size = (pulse * 0.5f + 1.0f) * scale * (1.0f - fade * fade);
        float angle = (float)(((float)i / 10.0f + (float)(time % 10000) * 0.0001f) * 2.0f * cube::Pi);
        setMaterialColor(startColor * (1.0f - progress) + endColor * progress);

        plasma::Matrix<float> model;
        model.identity();
        model.translate(cosf(angle) * radius, sinf(angle) * radius, progress * radius * 2.0f);
        model.translate(plasma::Vector<3, float>((float)(position[0] + m_drawOriginX),
                                                 (float)(position[1] + m_drawOriginY), (float)position[2]));
        // Particle tumble pose: progress-to-angle gains of 40/30/10 degrees about x/y/z. Particle tumble pose:
        // progress-to-angle gains of 40/30/10 degrees about x/y/z. Particle tumble pose: progress-to-angle gains of
        // 40/30/10 degrees about x/y/z.
        model.rotateX(progress * 40.0f);
        model.rotateY(progress * 30.0f);
        model.rotateZ(progress * 10.0f);
        model.scale(size, size, size);
        model.translate((float)m_world->m_spriteManager->sprite(cube::SpriteCube)->m_size[0] * -0.5f,
                        (float)m_world->m_spriteManager->sprite(cube::SpriteCube)->m_size[1] * -0.5f,
                        (float)m_world->m_spriteManager->sprite(cube::SpriteCube)->m_size[2] * -0.5f);
        setWorldMatrix(model);
        m_world->m_spriteManager->sprite(cube::SpriteCube)->drawBound();
    }
}

void Renderer::drawCreature(cube::Creature::AnimationState *animationState, float x, float y,
                            const plasma::Vector<3, float> &rotation, float scale)
{
    SpriteBatch batch;
    batch.m_position = plasma::Vector<2, float>(x, y);
    batch.m_matrix.identity();
    batch.m_matrix.translate(0.0f, 0.0f, CreaturePreviewDepth);
    batch.m_matrix.rotateX(rotation[0]);
    batch.m_matrix.rotateY(rotation[1]);
    batch.m_matrix.rotateZ(rotation[2]);
    batch.m_matrix.scale(scale, scale, scale);

    // The model is built with the creature's drawn position at the origin.
    plasma::Vector<3, cube::Fixed> position = animationState->m_creature->m_drawnPosition;
    animationState->m_creature->m_drawnPosition = plasma::Vector<3, cube::Fixed>(0, 0, 0);
    animationState->addSprites(batch, m_world->m_spriteManager, m_frameTime,
                               plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, cube::Fixed(0LL),
                               cube::Fixed(0LL), 0, nullptr, 0.0f, 1.0f);
    animationState->m_creature->m_drawnPosition = position;

    setAlpha(1.0f);
    setWhite(0.0f);
    setDepthOffset(0.0f);
    setShininess(0.0f);
    setGlow(0.0f);
    setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    // Interface sprite lighting palette: warm front, neutral back and cool ambient RGBA.
    setFogScale(PreviewFogDistance);
    clearPointLights();
    // Creature-preview lighting rig: front/back/ambient RGBA palettes and fixed normalized direction.
    setLight(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), plasma::Vector<4, float>(0.2f, 0.2f, 0.2f, 1.0f),
             normalized(plasma::Vector<3, float>(0.2f, 0.4f, -1.0f)), plasma::Vector<4, float>(0.4f, 0.4f, 0.4f, 1.0f));
    setSky(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f),
           plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));

    plasma::Matrix<float> perspective;
    perspectiveFov(perspective, PreviewFieldOfView, -((float)m_width / (float)m_height), PreviewNearClip,
                   PreviewFarClip);
    plasma::Matrix<float> identity;
    identity.identity();
    setMatrices(identity, identity, perspective);
    renderSprites(batch, SpritePassFlagScreenPosition);
}

void Renderer::drawCube(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<4, float> &startColor,
                        const plasma::Vector<4, float> &endColor, unsigned int time, float radius, float scale,
                        float turns, int count)
{
    for (int i = 0; i < count; i++) {
        float progress = (float)((time + i * 443) % 2000) / 2000.0f;
        float pulse = cosf((float)(i * 13 + (float)(time % 500) / 500.0f * 2.0f * cube::Pi));
        float fade = (progress - 0.5f) * 2.0f;
        float size = (pulse * 0.5f + 1.0f) * scale * (1.0f - fade * fade);
        float angle = (float)(progress * progress * 4.0f * cube::Pi * turns +
                              ((float)i / 10.0f + (float)(time % 10000) * 0.0001f) * 2.0f * cube::Pi);
        setMaterialColor(startColor * (1.0f - progress) + endColor * progress);

        float distance = progress * radius;
        plasma::Matrix<float> model;
        model.identity();
        model.translate(cosf(angle) * distance, sinf(angle) * distance, distance * 2.0f);
        model.translate(plasma::Vector<3, float>((float)(position[0] + m_drawOriginX),
                                                 (float)(position[1] + m_drawOriginY), (float)position[2]));
        model.rotateX(progress * 40.0f);
        model.rotateY(progress * 30.0f);
        model.rotateZ(progress * 10.0f);
        model.scale(size, size, size);
        model.translate((float)m_world->m_spriteManager->sprite(cube::SpriteCube)->m_size[0] * -0.5f,
                        (float)m_world->m_spriteManager->sprite(cube::SpriteCube)->m_size[1] * -0.5f,
                        (float)m_world->m_spriteManager->sprite(cube::SpriteCube)->m_size[2] * -0.5f);
        setWorldMatrix(model);
        m_world->m_spriteManager->sprite(cube::SpriteCube)->drawBound();
    }
}

void Renderer::drawCubes(const cube::Item *item, const plasma::Matrix<float> &model, const plasma::Matrix<float> &view,
                         const plasma::Matrix<float> &projection, const plasma::Vector<4, float> &color, float glow)
{
    if (item->category == cube::ItemCategoryWeapon && item->spiritCount) {
        for (int i = 0; i < item->spiritCount; i++) {
            plasma::Matrix<float> matrix = model;
            plasma::Vector<3, float> position(item->spirits[i].position);
            matrix.translate(position[0], position[1], position[2]);
            plasma::Vector<4, float> cubeColor = cube::materialColor(item->spirits[i].material, color, glow);
            setMaterialColor(cubeColor);
            setMatrices(matrix, view, projection);
            // Draws through the sprite pointer without a test; it is null when the sprite manager holds fewer sprites.
            m_world->m_spriteManager->sprite(cube::SpriteCube)->draw();
        }
    }
}

void Renderer::drawImpact(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<3, float> &vector,
                          const plasma::Vector<3, float> &rotation, const plasma::Vector<4, float> &innerColor,
                          const plasma::Vector<4, float> &middleColor, const plasma::Vector<4, float> &outerColor,
                          int time, float width, float scale, float frequency, int count, bool clipped)
{
    plasma::Vector<3, float> direction = vector;
    float lengthSquared = direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2];
    // Two unit vectors across the direction: the cubes circle in their plane.
    plasma::Vector<3, float> side = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f).cross(direction);
    if (lengthSquared < ImpactLengthSquaredEpsilon)
        side = plasma::Vector<3, float>(0.0f, 1.0f, 0.0f).cross(direction);
    side = normalized(side);
    plasma::Vector<3, float> up = normalized(side.cross(direction));
    float length = std::sqrt(lengthSquared);
    if (clipped) {
        // Not beyond the first block in the way.
        float range =
            std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
        float distance = m_world->rayDistance(position, direction * (1.0f / range), range, false, true, nullptr);
        direction = direction * (1.0f / length) * distance;
    }

    for (int i = 0; i < count; i++) {
        // Three cubes per step along the vector, each circling it at its own speed.
        float progress = ((float)i + 1.0f + (float)(time % count) / (float)count) / (float)count;
        plasma::Vector<3, cube::Fixed> point = position;
        point[1] += m_drawOriginY;
        point[0] += m_drawOriginX;
        point = point + plasma::Vector<3, cube::Fixed>(direction * progress);
        plasma::Vector<3, float> center = plasma::Vector<3, float>(point);
        // Impact layer curves: primary helix and secondary orbit rates, layer offsets, radial gains and cube-size
        // falloff.
        float turn = progress + 0.1f;
        float radius = turn * width;
        double phase = turn * cube::Pi;

        float angle = (float)((phase * 2.0 * length - time * 0.005f * cube::Pi) * frequency);
        plasma::Vector<3, float> offset = center + side * radius * cosf(angle) + up * radius * sinf(angle);
        offset = offset + (side * radius * cosf(angle * -0.3f) + up * radius * sinf(angle * -0.3f));
        plasma::Matrix<float> model;
        model.identity();
        model.translate(offset);
        model.rotate(rotation[2], plasma::Vector<3, float>(0.0f, 0.0f, 1.0f));
        turn *= length;
        float spin = turn * 360.0f;
        model.rotateX(spin);
        model.rotateY(spin);
        model.rotateZ(turn * -360.0f);
        float size = (cosf(angle * 0.9f) * 0.1f + 0.2f) * (radius + 0.2f) * scale;
        model.scale(size, size, size);
        model.translate(plasma::Vector<3, float>(-0.5f, -0.5f, -0.5f));
        setWorldMatrix(model);
        setMaterialColor(innerColor);
        m_world->m_spriteManager->sprite(cube::SpriteCube)->drawBound();

        radius *= 1.01f;
        angle = (float)((phase * 3.0 * length - time * 0.004f * cube::Pi) * frequency);
        offset = center + side * radius * cosf(angle) + up * radius * sinf(angle);
        offset = side * radius * 0.5f * cosf(angle * 0.3f) + up * radius * 0.5f * sinf(angle * 0.3f) + offset;
        model.identity();
        model.translate(offset);
        model.rotate(rotation[2], plasma::Vector<3, float>(0.0f, 0.0f, 1.0f));
        model.rotateX(turn * 360.0f * 2.0f);
        model.rotateY(turn * 360.0f * 2.0f);
        model.rotateZ(turn * -360.0f * 2.0f);
        size = (cosf(angle * 0.9f) * 0.1f + 0.2f) * (radius + 0.2f) * scale;
        model.scale(size, size, size);
        model.translate(plasma::Vector<3, float>(-0.5f, -0.5f, -0.5f));
        setWorldMatrix(model);
        setMaterialColor(middleColor);
        m_world->m_spriteManager->sprite(cube::SpriteCube)->drawBound();

        radius *= 1.01f;
        angle = (float)((phase * 3.0 * length - time * 0.006f * cube::Pi) * frequency);
        offset = center + side * radius * cosf(angle) + up * radius * sinf(angle);
        offset = side * radius * 0.5f * cosf(angle * 0.3f) + up * radius * 0.5f * sinf(angle * 0.3f) + offset;
        model.identity();
        model.translate(offset);
        model.rotate(rotation[2], plasma::Vector<3, float>(0.0f, 0.0f, 1.0f));
        model.rotateX(turn * 360.0f * 3.0f);
        model.rotateY(turn * 360.0f * 3.0f);
        model.rotateZ(turn * -360.0f * 3.0f);
        size = (cosf(angle * 0.9f) * 0.1f + 0.2f) * (radius + 0.2f) * scale;
        model.scale(size, size, size);
        model.translate(plasma::Vector<3, float>(-0.5f, -0.5f, -0.5f));
        setWorldMatrix(model);
        setMaterialColor(outerColor);
        m_world->m_spriteManager->sprite(cube::SpriteCube)->drawBound();
    }
}

void Renderer::drawItem(const cube::Item *item, float x, float y, const plasma::Vector<3, float> &rotation, float scale,
                        float depth, bool outlined)
{
    if (item) {
        cube::Sprite *sprite = m_world->m_spriteManager->itemSprite(item);
        if (item->category == cube::ItemCategoryNone)
            sprite = m_world->m_spriteManager->sprite(cube::SpriteUnknown);
        if (sprite) {
            // The model is scaled to its largest dimension (flat models count four more; chest pieces are shown
            // smaller).
            int size = std::max(sprite->m_size[1], sprite->m_size[0]);
            size = std::max(sprite->m_size[2], size);
            if (sprite->m_size[2] < 4)
                size += 4;
            if (item->category == cube::ItemCategoryChest)
                size = (int)((float)size * ChestPreviewSizeDivisorGain);

            plasma::Matrix<float> model;
            model.identity();
            model.translate(0.0f, 0.0f, 1.0f);
            model.rotateX(rotation[0]);
            model.rotateY(rotation[1]);
            model.rotateZ(rotation[2]);
            if (item->category == cube::ItemCategoryRing) {
                model.rotateX(90.0f);
                model.rotateZ(-90.0f);
            }
            model.scale(scale, scale, scale);
            model.scale(1024.0f / (float)m_height, 1024.0f / (float)m_height, 1024.0f / (float)m_height);
            model.scale(1.0f / (float)size, 1.0f / (float)size, 1.0f / (float)size);
            model.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                            (float)sprite->m_size[2] * -0.5f);
            plasma::Matrix<float> view;
            view.identity();

            plasma::Matrix<float> perspective;
            perspectiveFov(perspective, PreviewFieldOfView, -((float)m_width / (float)m_height), PreviewNearClip,
                           PreviewFarClip);
            float width = (float)m_width;
            float height = (float)m_height;
            plasma::Matrix<float> offset;
            offset.identity();
            offset.translate((x - width * 0.5f) / width + (x - width * 0.5f) / width,
                             (y - height * 0.5f) / height * -2.0f, depth);
            setAlpha(1.0f);
            setFogScale(PreviewFogDistance);

            float depthOffset = 0.0f;
            if (outlined) {
                // The outline: the model in black, shifted down and right.
                plasma::Matrix<float> shifted = view;
                shifted.translate(scale * -ItemOutlineDisplacement, scale * -ItemOutlineDisplacement);
                setMatrices(model, shifted, offset * perspective);
                setMaterialColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
                setDepthOffset(0.0f);
                depthOffset = ItemOutlineDepthOffset;
                sprite->draw();
            }

            setMatrices(model, view, offset * perspective);
            setWhite(0.0f);
            setDepthOffset(depthOffset);
            setShininess(item->shininess());
            setGlow(0.0f);
            setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
            clearPointLights();
            // Item-preview lighting rig: front/back/ambient RGBA palettes and fixed normalized direction.
            setLight(plasma::Vector<4, float>(0.7f, 0.7f, 0.2f, 1.0f), plasma::Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f),
                     normalized(plasma::Vector<3, float>(1.0f, 0.4f, -1.0f)),
                     plasma::Vector<4, float>(0.3f, 0.3f, 0.8f, 1.0f));
            sprite->draw();
            drawCubes(item, model, view, offset * perspective, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0.0f);
        }
    }
}

void Renderer::drawLine(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<3, float> &vector,
                        const plasma::Vector<4, float> &color, float width)
{
    // The cube sprite, turned from the y axis to the vector, as long as the vector and centred on it.
    plasma::Vector<3, float> axis(0.0f, 1.0f, 0.0f);
    plasma::Matrix<float> model;
    model.identity();
    model.translate((float)(position[0] + m_drawOriginX), (float)(position[1] + m_drawOriginY), (float)position[2]);
    rotateTowards(model, axis, vector);
    float length = sqrtf(vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2]);
    model.scale(width, length, width);
    model.translate(-0.5f, 0.0f, -0.5f);
    setWorldMatrix(model);
    setMaterialColor(color);
    m_world->m_spriteManager->sprite(cube::SpriteCube)->drawBound();
}

void Renderer::drawParticle(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<4, float> &startColor,
                            const plasma::Vector<4, float> &endColor, unsigned int time, float radius, float scale,
                            int count)
{
    for (int i = 0; i < count; i++) {
        float progress = (float)((time + i * 443) % 2000) / 2000.0f;
        float arc = (float)(progress * cube::Pi);
        float height = cosf(arc) * radius;
        float distance = sinf(arc) * radius;
        float pulse = cosf((float)(i * 13 + (float)(time / 8 % 500) / 500.0f * 2.0f * cube::Pi));
        float fade = (progress - 0.5f) * 2.0f;
        float size = (pulse * 0.5f + 1.0f) * scale * (1.0f - fade * fade);
        float angle = (float)(((float)i / 10.0f + (float)(time % 10000) * 0.0001f) * 2.0f * cube::Pi);
        setMaterialColor(startColor * (1.0f - progress) + endColor * progress);

        plasma::Matrix<float> model;
        model.identity();
        model.translate(cosf(angle) * distance, sinf(angle) * distance, height);
        model.translate(plasma::Vector<3, float>((float)(position[0] + m_drawOriginX),
                                                 (float)(position[1] + m_drawOriginY), (float)position[2]));
        model.rotateX(progress * 40.0f);
        model.rotateY(progress * 30.0f);
        model.rotateZ(progress * 10.0f);
        model.scale(size, size, size);
        model.translate((float)m_world->m_spriteManager->sprite(cube::SpriteCube)->m_size[0] * -0.5f,
                        (float)m_world->m_spriteManager->sprite(cube::SpriteCube)->m_size[1] * -0.5f,
                        (float)m_world->m_spriteManager->sprite(cube::SpriteCube)->m_size[2] * -0.5f);
        setWorldMatrix(model);
        m_world->m_spriteManager->sprite(cube::SpriteCube)->drawBound();
    }
}

void Renderer::drawSprite(cube::Sprite *sprite, const plasma::Matrix<float> &model, const plasma::Matrix<float> &view,
                          const plasma::Matrix<float> &projection, const plasma::Vector<4, float> &color)
{
    if (sprite) {
        setMatrices(model, view, projection);
        setAlpha(1.0f);
        setWhite(0.0f);
        setDepthOffset(0.0f);
        setShininess(0.0f);
        setGlow(0.0f);
        setMaterialColor(color);
        setFogScale(PreviewFogDistance);
        clearPointLights();
        // A fixed light from the upper front left, no fog. Sprite lighting rig: front/back/ambient RGBA palettes and
        // fixed normalized direction.
        plasma::Vector<4, float> backgroundColor(0.4f, 0.4f, 0.4f, 1.0f);
        plasma::Vector<3, float> direction(0.2f, -0.4f, -1.0f);
        direction *=
            1.0f / sqrtf(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
        setLight(plasma::Vector<4, float>(1.1f, 1.1f, 0.9f, 0.0f), plasma::Vector<4, float>(0.5f, 0.6f, 0.8f, 1.0f),
                 direction, backgroundColor);
        sprite->draw();
    }
}

void Renderer::drawSpriteModel(cube::Sprite *sprite, float x, float y, const plasma::Vector<3, float> &rotation,
                               float scale, float depthOffset)
{
    if (sprite) {
        // The model turned about its centre, one unit behind the origin, in units of 1 / 1024 of the target's height.
        plasma::Matrix<float> model;
        model.identity();
        model.translate(0.0f, 0.0f, 1.0f);
        model.rotateX(rotation[0]);
        model.rotateY(rotation[1]);
        model.rotateZ(rotation[2]);
        model.scale(scale, scale, scale);
        model.scale(1024.0f / (float)m_height, 1024.0f / (float)m_height, 1024.0f / (float)m_height);
        model.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                        (float)sprite->m_size[2] * -0.5f);
        plasma::Matrix<float> view;
        view.identity();

        // A 45 degree perspective (mirrored in x, depth 0.1..1000), shifted so that the model appears at the window
        // position (x, y).
        plasma::Matrix<float> perspective;
        perspectiveFov(perspective, PreviewFieldOfView, -((float)m_width / (float)m_height), PreviewNearClip,
                       PreviewFarClip);
        float width = (float)m_width;
        float height = (float)m_height;
        plasma::Matrix<float> offset;
        offset.identity();
        offset.translate((x - width * 0.5f) / width + (x - width * 0.5f) / width, (y - height * 0.5f) / height * -2.0f,
                         0.0f);
        plasma::Matrix<float> projection = offset * perspective;

        setMatrices(model, view, projection);
        setAlpha(1.0f);
        setWhite(0.0f);
        setDepthOffset(depthOffset);
        setShininess(0.0f);
        setGlow(0.0f);
        setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        setFogScale(PreviewFogDistance);
        clearPointLights();

        // The light comes from a fixed direction in view space. The model matrix's variable is reused for the inverse
        // view matrix.
        plasma::Matrix<float> &inverseView = model;
        inverseView = view;
        inverseView.invert();
        // Sprite-preview lighting rig: front/back/ambient RGBA palettes and direction rotated from view space.
        plasma::Vector<4, float> backgroundColor(0.3f, 0.6f, 1.0f, 1.0f);
        setLight(plasma::Vector<4, float>(1.0f, 1.0f, 0.8f, 1.0f), plasma::Vector<4, float>(0.3f, 0.6f, 1.0f, 1.0f),
                 rotate(inverseView, normalized(plasma::Vector<3, float>(0.3f, 0.8f, -1.0f))), backgroundColor);
        sprite->draw();
    }
}

void Renderer::extractFrustum()
{
    plasma::Matrix<float> m = m_projectionMatrix;
    m *= m_viewMatrix;
    m_frustum[FrustumLeft][0] = m[3] + m[0];
    m_frustum[FrustumLeft][1] = m[7] + m[4];
    m_frustum[FrustumLeft][2] = m[11] + m[8];
    m_frustum[FrustumLeft][3] = m[15] + m[12];
    m_frustum[FrustumRight][0] = m[3] - m[0];
    m_frustum[FrustumRight][1] = m[7] - m[4];
    m_frustum[FrustumRight][2] = m[11] - m[8];
    m_frustum[FrustumRight][3] = m[15] - m[12];
    m_frustum[FrustumBottom][0] = m[1] + m[3];
    m_frustum[FrustumBottom][1] = m[5] + m[7];
    m_frustum[FrustumBottom][2] = m[9] + m[11];
    m_frustum[FrustumBottom][3] = m[13] + m[15];
    m_frustum[FrustumTop][0] = m[3] - m[1];
    m_frustum[FrustumTop][1] = m[7] - m[5];
    m_frustum[FrustumTop][2] = m[11] - m[9];
    m_frustum[FrustumTop][3] = m[15] - m[13];
    m_frustum[FrustumFar][0] = m[3] - m[2];
    m_frustum[FrustumFar][1] = m[7] - m[6];
    m_frustum[FrustumFar][2] = m[11] - m[10];
    m_frustum[FrustumFar][3] = m[15] - m[14];
    m_frustum[FrustumNear][0] = m[2] + m[3];
    m_frustum[FrustumNear][1] = m[6] + m[7];
    m_frustum[FrustumNear][2] = m[10] + m[11];
    m_frustum[FrustumNear][3] = m[14] + m[15];
}

bool Renderer::isBoxVisible(const plasma::Vector<3, cube::Fixed> &minimum,
                            const plasma::Vector<3, cube::Fixed> &maximum, float range)
{
    plasma::Vector<3, cube::Fixed> center = (minimum + maximum) * cube::Fixed(0.5);
    plasma::Vector<3, cube::Fixed> offset = m_cameraPosition - center;
    offset[2] = cube::Fixed(0);
    // The distance is computed before the range is converted.
    cube::Fixed squaredDistance = offset.squaredLength();
    if (squaredDistance > cube::Fixed(range * range))
        return false;
    plasma::Vector<3, cube::Fixed> low = minimum + plasma::Vector<3, cube::Fixed>(m_drawOriginX, m_drawOriginY, 0);
    plasma::Vector<3, cube::Fixed> high = maximum + plasma::Vector<3, cube::Fixed>(m_drawOriginX, m_drawOriginY, 0);
    plasma::Vector<3, float> lowPoint((float)low[0], (float)low[1], (float)low[2]);
    plasma::Vector<3, float> highPoint((float)high[0], (float)high[1], (float)high[2]);
    for (int i = 0; i < FrustumPlaneCount; i++) {
        // The top plane is not tested.
        if (i == FrustumTop)
            continue;
        // The corner furthest along the plane's normal.
        float x = m_frustum[i][0] <= 0.0f ? lowPoint[0] : highPoint[0];
        float y = m_frustum[i][1] <= 0.0f ? lowPoint[1] : highPoint[1];
        float z = m_frustum[i][2] <= 0.0f ? lowPoint[2] : highPoint[2];
        if (m_frustum[i][0] * x + m_frustum[i][1] * y + m_frustum[i][2] * z + m_frustum[i][3] < 0.0f)
            return false;
    }
    return true;
}

bool Renderer::isSphereVisible(const plasma::Vector<3, cube::Fixed> &position, float radius, float range)
{
    plasma::Vector<3, cube::Fixed> offset = m_cameraPosition - position;
    cube::Fixed squaredDistance = offset.squaredLength();
    if (squaredDistance > cube::Fixed(range * range))
        return false;
    plasma::Vector<3, cube::Fixed> world = position + plasma::Vector<3, cube::Fixed>(m_drawOriginX, m_drawOriginY, 0);
    plasma::Vector<3, float> point((float)world[0], (float)world[1], (float)world[2]);
    for (int i = 0; i < FrustumPlaneCount; i++) {
        if (m_frustum[i][0] * point[0] + m_frustum[i][1] * point[1] + m_frustum[i][2] * point[2] + m_frustum[i][3] +
                radius <
            0.0f)
            return false;
    }
    return true;
}

// The factor that scales a sprite to an entity's extent: the larger of the entity's x and y extents, each over the
// sprite's x size.
static inline float fitScale(const plasma::Vector<3, float> &size, cube::Sprite *sprite)
{
    float y = size[1] / (float)sprite->m_size[0];
    float x = size[0] / (float)sprite->m_size[0];
    return std::max(y, x);
}

// How far a static entity that opens (a door, a chest's lid, a gate) is open, 0..1: transitionTime counts the
// milliseconds since active changed; it opens while active is clear and closes while it is set.
static inline float openness(const cube::StaticEntity &entity, float closingRate)
{
    if (entity.active)
        return std::max(0.0f, 1.0f - entity.transitionTime * closingRate);
    return std::min(1.0f, entity.transitionTime * StaticEntityOpeningRate);
}

void Renderer::placeStaticEntity(const cube::StaticEntity &entity)
{
    float glow = 0.0f;
    plasma::Vector<3, float> size = entity.scaledSize();
    plasma::Vector<3, cube::Fixed> center = entity.position;
    center[2] += cube::Fixed(size[2] * 0.5);
    float alpha = attenuation(center, m_effectViewDistance);
    float light = (float)m_world->cubeLight(entity.position[0], entity.position[1], entity.position[2], 0) / 255.0f;
    plasma::Vector<4, float> color(m_tint[0], m_tint[1], m_tint[2], light * m_tint[3]);
    cube::Sprite *part = m_world->m_spriteManager->partSprite(entity.type);

    // The two chests: the body, then part 23 as the lid (below).
    if (entity.type == cube::StaticEntityChestBase02 || entity.type == cube::StaticEntityRewardChest) {
        float fit = fitScale(size, part);
        float scale = cube::entityScale(entity.type) * fit;
        plasma::Matrix<float> model;
        model.identity();
        model.translate((float)(entity.position[0] + m_drawOriginX), (float)(entity.position[1] + m_drawOriginY),
                        (float)entity.position[2]);
        model.scale(scale, scale, scale);
        model.rotateZ((float)(entity.orientation * 90));
        model.translate((float)part->m_size[0] * -0.5f, (float)part->m_size[1] * -0.5f, 0.0f);
        m_worldBatch.add(part, model, color, alpha, 0.0f, 0.0f, 0.0f, 0.0f);
        part = m_world->m_spriteManager->partSprite(cube::StaticEntityChestTop02);
    }

    // The four doors: the leaf turns up to 90 degrees about a point 0.9 of its size from its corner.
    if (entity.type >= cube::StaticEntityCageBase && entity.type <= cube::StaticEntityCage4Base) {
        cube::Sprite *door = m_world->m_spriteManager->objectSprite(entity.type);
        if (door) {
            float fit = fitScale(size, door);
            float scale = cube::entityScale(entity.type) * fit;
            float open = openness(entity, StaticEntityClosingRate);
            plasma::Matrix<float> model;
            model.identity();
            model.translate((float)(entity.position[0] + m_drawOriginX), (float)(entity.position[1] + m_drawOriginY),
                            (float)entity.position[2]);
            model.scale(scale, scale, scale);
            model.rotateZ((float)(entity.orientation * 90));
            int width = door->m_size[0];
            int depth = door->m_size[1];
            model.translate((float)width * -0.5f, (float)depth * -0.5f, 0.0f);
            model.translate((float)width * 0.9f, (float)depth * 0.9f, 0.0f);
            model.rotateZ(open * -90.0f);
            model.translate((float)-width * 0.9f, (float)-depth * 0.9f, 0.0f);
            m_worldBatch.add(door, model, color, alpha, 0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    // Type 130: object sprite 130 on top of the entity's part.
    if (entity.type == cube::StaticEntitySarcophagus) {
        cube::Sprite *top = m_world->m_spriteManager->objectSprite(cube::StaticEntitySarcophagus);
        if (top) {
            float fit = fitScale(size, top);
            float scale = cube::entityScale(entity.type) * fit;
            plasma::Matrix<float> model;
            model.identity();
            model.translate((float)(entity.position[0] + m_drawOriginX), (float)(entity.position[1] + m_drawOriginY),
                            (float)entity.position[2]);
            model.scale(scale, scale, scale);
            model.rotateZ((float)(entity.orientation * 90));
            // Reads the part's height before it tests the part below.
            model.translate((float)top->m_size[0] * -0.5f, (float)top->m_size[1] * -0.5f, (float)part->m_size[2]);
            m_worldBatch.add(top, model, color, alpha, 0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    if (!part)
        return;
    float fit = fitScale(size, part);
    float scale = cube::entityScale(entity.type) * fit;
    plasma::Matrix<float> model;
    model.identity();
    // What rises and sinks when used: 8, 9, 10 and 16 by 0.8 of their height, 19 by its height, 18 downwards by its
    // height and one block.
    if ((entity.type >= cube::StaticEntityGate && entity.type <= cube::StaticEntityRuneGate) ||
        entity.type == cube::StaticEntityGoldenGate) {
        glow = 1.0f;
        model.translate(0.0f, 0.0f, size[2] * 0.8f * openness(entity, StaticEntityClosingRate));
    }
    if (entity.type == cube::StaticEntityGrid)
        glow = 1.0f;
    if (entity.type == cube::StaticEntitySpikeTrap) {
        glow = 1.0f;
        model.translate(0.0f, 0.0f, -(size[2] * openness(entity, TrapClosingRate)) - 1.0f);
    }
    if (entity.type == cube::StaticEntityStompTrap)
        model.translate(0.0f, 0.0f, size[2] * openness(entity, TrapClosingRate));
    model.translate((float)(entity.position[0] + m_drawOriginX), (float)(entity.position[1] + m_drawOriginY),
                    (float)entity.position[2]);
    model.scale(scale, scale, scale);
    model.rotateZ((float)(entity.orientation * 90));
    model.translate((float)part->m_size[0] * -0.5f, (float)part->m_size[1] * -0.5f, 0.0f);
    // The doors 3..6 turn up to 90 degrees about the vertical edge at the middle of their depth.
    if (entity.type >= cube::StaticEntityDoor && entity.type <= cube::StaticEntityWindow) {
        float open = openness(entity, StaticEntityClosingRate);
        model.translate(0.0f, (float)part->m_size[1] * 0.5f, 0.0f);
        model.rotateZ(open * 90.0f);
        model.translate(0.0f, (float)part->m_size[1] * -0.5f, 0.0f);
    }
    if (entity.type == cube::StaticEntityMagicBarrier) {
        alpha *= MagicBarrierOpacity;
        if (!entity.active)
            alpha *= InactiveMagicBarrierOpacityGain;
    }
    // The parts that tip over about an axis along x: 105 about its top edge, a chest's lid, 20 about its middle from
    // -30 to 30 degrees.
    if (entity.type == cube::StaticEntityHatch) {
        float open = openness(entity, StaticEntityClosingRate);
        model.translate((float)part->m_size[0] * 0.5f, 0.0f, (float)part->m_size[2]);
        model.rotateX(open * 90.0f);
        model.translate((float)part->m_size[0] * -0.5f, 0.0f, (float)-part->m_size[2]);
    }
    if (entity.type == cube::StaticEntityChestBase02 || entity.type == cube::StaticEntityRewardChest) {
        float open = openness(entity, StaticEntityClosingRate);
        // Chest-lid pose: hinge raised 9 model units, opening up to 80 degrees, then a -1 model-unit local offset.
        model.translate(0.0f, 0.0f, 9.0f);
        model.rotateX(open * 80.0f);
        model.translate(0.0f, 0.0f, -1.0f);
    }
    if (entity.type == cube::StaticEntityLever) {
        float open = openness(entity, StaticEntityClosingRate);
        model.translate(0.0f, (float)part->m_size[1] * 0.5f, -0.5f);
        model.rotateX(open * 60.0f - 30.0f);
        model.translate(0.0f, (float)part->m_size[1] * -0.5f, 0.0f);
    }
    // 63..66 are drawn at full light when it is dark.
    if (entity.type >= cube::StaticEntityStreetLight02 && entity.type <= cube::StaticEntityStreetLightUndead &&
        m_daylight < StreetlightDaylightCutoff)
        color[3] = 1.0f;
    m_worldBatch.add(part, model, color, alpha, 0.0f, glow, 0.0f, 0.0f);

    // Type 10 gets the sprite 3554 around it, slightly wider, at full light.
    if (entity.type == cube::StaticEntityRuneGate) {
        cube::Sprite *shell = m_world->m_spriteManager->sprite(cube::SpriteRuneGateGlow);
        model.translate((float)shell->m_size[0] * 0.5f, (float)shell->m_size[1] * 0.5f,
                        (float)shell->m_size[2] * 0.5f + RuneGateShellLift);
        model.scale(RuneGateShellWidthGain, RuneGateShellWidthGain, 1.0f);
        model.translate((float)shell->m_size[0] * -0.5f, (float)shell->m_size[1] * -0.5f,
                        (float)shell->m_size[2] * -0.5f);
        color[3] = 1.0f;
        m_worldBatch.add(shell, model, color, alpha, 0.0f, glow, 0.0f, 0.0f);
    }
}

void Renderer::renderChunks(const plasma::Vector<4, float> &frontColor, const plasma::Vector<4, float> &backColor,
                            const plasma::Vector<3, float> &direction, const plasma::Vector<4, float> &ambientColor)
{
    activateMode(RendererMode_OpaqueFront);
    setWhite(0.0f);
    for (cube::Zone *zone : m_zones) {
        if (zone->m_chunk.chunkBuffers.empty())
            continue;
        setPointLights(zone->m_chunk.lightPositions, zone->m_chunk.lightColors);
        for (ChunkBuffer *buffer : zone->m_chunk.chunkBuffers) {
            if (buffer->indexBuffers[ChunkPassSolid]) {
                plasma::Matrix<float> model;
                model.identity();
                model.translate((float)((cube::Fixed(zone->m_position[0] * cube::BlocksPerZone)) + m_drawOriginX),
                                (float)((cube::Fixed(zone->m_position[1] * cube::BlocksPerZone)) + m_drawOriginY),
                                (float)buffer->baseZ);
                setWorldMatrix(model);
                if (buffer->vertexBuffer->bind() && buffer->indexBuffers[ChunkPassSolid]->bind()) {
                    m_world->m_spriteManager->graphics()->drawIndexed(buffer->vertexCount,
                                                                      buffer->triangleCounts[ChunkPassSolid]);
                    endChunkBufferDraw();
                }
            }
            if (buffer->indexBuffers[ChunkPass_4]) {
                plasma::Matrix<float> model;
                model.identity();
                model.translate((float)((cube::Fixed(zone->m_position[0] * cube::BlocksPerZone)) + m_drawOriginX),
                                (float)((cube::Fixed(zone->m_position[1] * cube::BlocksPerZone)) + m_drawOriginY),
                                (float)buffer->baseZ);
                setWorldMatrix(model);
                if (buffer->vertexBuffer->bind() && buffer->indexBuffers[ChunkPass_4]->bind()) {
                    m_world->m_spriteManager->graphics()->drawIndexed(buffer->vertexCount,
                                                                      buffer->triangleCounts[ChunkPass_4]);
                    endChunkBufferDraw();
                }
            }
            if (buffer->indexBuffers[ChunkPass_5]) {
                plasma::Matrix<float> model;
                model.identity();
                model.translate((float)((cube::Fixed(zone->m_position[0] * cube::BlocksPerZone)) + m_drawOriginX),
                                (float)((cube::Fixed(zone->m_position[1] * cube::BlocksPerZone)) + m_drawOriginY),
                                (float)buffer->baseZ);
                setWorldMatrix(model);
                if (buffer->vertexBuffer->bind() && buffer->indexBuffers[ChunkPass_5]->bind()) {
                    m_world->m_spriteManager->graphics()->drawIndexed(buffer->vertexCount,
                                                                      buffer->triangleCounts[ChunkPass_5]);
                    endChunkBufferDraw();
                }
            }
            if (buffer->indexBuffers[ChunkPassLava]) {
                // A fixed warm light from straight above for this buffer. Lava chunk lighting rig: warm front/back
                // RGBA palette and neutral ambient, from directly above.
                plasma::Vector<4, float> fixedBackground(0.4f, 0.4f, 0.4f, 1.0f);
                plasma::Vector<3, float> down(0.0f, 0.0f, 1.0f);
                setLight(plasma::Vector<4, float>(1.0f, 1.0f, 0.5f, 1.0f),
                         plasma::Vector<4, float>(0.5f, 0.3f, 0.0f, 1.0f), down, fixedBackground);
                plasma::Matrix<float> model;
                model.identity();
                model.translate((float)((cube::Fixed(zone->m_position[0] * cube::BlocksPerZone)) + m_drawOriginX),
                                (float)((cube::Fixed(zone->m_position[1] * cube::BlocksPerZone)) + m_drawOriginY),
                                (float)buffer->baseZ);
                setWorldMatrix(model);
                if (buffer->vertexBuffer->bind() && buffer->indexBuffers[ChunkPassLava]->bind()) {
                    m_world->m_spriteManager->graphics()->drawIndexed(buffer->vertexCount,
                                                                      buffer->triangleCounts[ChunkPassLava]);
                    endChunkBufferDraw();
                }
                setLight(frontColor, backColor, direction, ambientColor);
            }
        }
    }
}

void Renderer::renderClouds(const plasma::Vector<4, float> &skyColor1, const plasma::Vector<4, float> &skyColor2,
                            const plasma::Vector<4, float> &frontColor, const plasma::Vector<4, float> &backColor,
                            const plasma::Vector<3, float> &direction, const plasma::Vector<4, float> &ambientColor)
{
    // A cloud near the camera, drawn after the others with its alpha.
    struct Cloud {
        float alpha;
        plasma::Matrix<float> matrix;
        cube::Sprite *sprite;
    };

    activateMode(RendererMode_Clouds);
    setWhite(0.0f);
    setGlow(0.0f);
    clearPointLights();
    setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    setViewMatrix(m_viewMatrix);
    setProjectionMatrix(m_projectionMatrix);
    std::list<Cloud> nearClouds;
    setAlpha(1.0f);
    for (int i = -CloudSearchMargin - (int)(m_viewDistance / CloudLatticeSpacing);
         i <= CloudSearchMargin - (int)(m_viewDistance / -CloudLatticeSpacing); i++) {
        for (int j = -CloudSearchMargin - (int)(m_viewDistance / CloudLatticeSpacing);
             j <= CloudSearchMargin - (int)(m_viewDistance / -CloudLatticeSpacing); j++) {
            int cellX = (int)m_cameraPosition[0] / CloudLatticeSpacingBlocks + i;
            int cellY = (int)m_cameraPosition[1] / CloudLatticeSpacingBlocks + j;
            plasma::Vector<3, cube::Fixed> position;
            position[1] = cube::Fixed((cellX % 2 + cellY * 2) * CloudLatticeStaggerBlocks);
            position[0] = cube::Fixed(cellX * CloudLatticeSpacingBlocks);
            int y = (int)position[1];
            int x = (int)position[0];
            cube::World::BiomeSample sample;
            m_world->biomeSample(x, y, sample);
            if (sample.humidity() * 0.5f < (cube::noise((double)position[0], (double)position[1]) + 1.0f) * 0.5f)
                continue;
            position[2] = cube::Fixed(m_world->height(x, y, cube::HeightFlagSkipPlaceShape));
            if (position[2] < cube::Fixed(0LL))
                position[2] = cube::Fixed(0LL);
            // Cloud altitude noise profile: cell-coordinate salts 17/19, amplitude 30 and base height 200 blocks.
            position[2] += cube::Fixed(cube::noise((double)(cellX * 17), (double)(cellY * 19)) * 30.0f + 200.0f);
            if (!isSphereVisible(position, CloudVisibilityRadius, m_viewDistance + CloudVisibilityMargin))
                continue;
            cube::World::AreaInfo info;
            m_world->fillAreaInfo(x, y, info);
            cube::Sprite *sprite;
            if (sample.undead(info.type, info.influence) > DarkCloudUndeadThreshold)
                sprite = m_world->m_spriteManager->sprite(cube::SpriteCloudDark);
            else
                sprite = m_world->m_spriteManager->sprite(cube::SpriteCloud02);

            plasma::Matrix<float> model;
            model.identity();
            model.translate(plasma::Vector<3, float>(
                position + plasma::Vector<3, cube::Fixed>(m_drawOriginX, m_drawOriginY, cube::Fixed(0))));
            // Cloud scale noise profile: cell-coordinate salts 15/23, amplitude 2 and baseline scale 7.
            float scale = cube::noise((double)(cellX * 15), (double)(cellY * 23)) * 2.0f + 7.0f;
            model.scale(scale, scale, scale);
            model.rotateZ((float)((cellX + cellY) * 90));
            model.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f, 0.0f);
            setWorldMatrix(model);

            plasma::Vector<3, float> offset = plasma::Vector<3, float>(m_cameraPosition - position);
            float squaredDistance = offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2];
            if (squaredDistance < CloudNearFadeOuterRadius * CloudNearFadeOuterRadius) {
                float alpha = (std::min)(
                    1.0f, (std::max)(0.0f, (sqrtf(squaredDistance) - CloudNearFadeInnerRadius) * CloudNearFadeRate));
                if (alpha < CloudDiscardAlpha)
                    continue;
                if (alpha < 1.0f) {
                    Cloud cloud;
                    cloud.alpha = alpha;
                    cloud.matrix = model;
                    cloud.sprite = sprite;
                    nearClouds.push_back(cloud);
                    continue;
                }
            }
            sprite->draw();
        }
    }
    setAlpha(0.0f);
    for (Cloud &cloud : nearClouds) {
        setWorldMatrix(cloud.matrix);
        cloud.sprite->draw();
    }
    for (Cloud &cloud : nearClouds) {
        setAlpha(cloud.alpha);
        setWorldMatrix(cloud.matrix);
        cloud.sprite->draw();
    }
    setLight(frontColor, backColor, direction, ambientColor);
}

void Renderer::renderEffects()
{
    activateMode(RendererMode_OpaqueFront);
    setWhite(0.0f);
    setGlow(1.0f);
    m_world->m_spriteManager->sprite(cube::SpriteCube)->bind();
    for (Impact &impact : m_impacts) {
        drawImpact(impact.position, impact.direction, impact.rotation, impact.innerColor, impact.middleColor,
                   impact.outerColor, impact.time, impact.width, impact.scale, impact.frequency, impact.count,
                   impact.clipped);
    }
    for (Box &box : m_boxes) {
        drawBox(box.position, box.startColor, box.endColor, box.time, box.radius, box.scale, box.count);
    }
    for (Particle &particle : m_particles) {
        drawParticle(particle.position, particle.startColor, particle.endColor, particle.time, particle.radius,
                     particle.scale, particle.count);
    }
    for (Cube &cube : m_cubes) {
        drawCube(cube.position, cube.startColor, cube.endColor, cube.time, cube.radius, cube.scale, cube.turns,
                 cube.count);
    }
    for (Line &line : m_lines)
        drawLine(line.position, line.vector, line.color, line.width);
}

void Renderer::renderTranslucentSprites(SpriteBatch &batch, int flags)
{
    // Each list twice: first in the modes 3 and 4 with only the matrices and the depth offset set, then in the modes 6
    // and 7 with every shader value of the instance.
    if (!batch.m_translucent.empty()) {
        activateMode(RendererMode_DepthFront);
        for (SpriteBatch::TranslucentInstance &translucent : batch.m_translucent) {
            cube::Sprite *sprite = translucent.sprite;
            setWorldMatrix(translucent.instance.matrix);
            if (flags & SpritePassFlagScreenPosition) {
                float width = (float)m_width;
                float height = (float)m_height;
                plasma::Matrix<float> projection;
                projection.identity();
                projection.translate((translucent.instance.position[0] - width * 0.5f) / width * 2.0f,
                                     (translucent.instance.position[1] - height * 0.5f) / height * -2.0f, -0.5f);
                projection *= m_projectionMatrix;
                setProjectionMatrix(projection);
            }
            setDepthOffset(translucent.instance.parameters[3]);
            sprite->draw();
        }
    }
    if (!batch.m_mirroredTranslucent.empty()) {
        activateMode(RendererMode_DepthBack);
        for (SpriteBatch::TranslucentInstance &translucent : batch.m_mirroredTranslucent) {
            cube::Sprite *sprite = translucent.sprite;
            setWorldMatrix(translucent.instance.matrix);
            if (flags & SpritePassFlagScreenPosition) {
                float width = (float)m_width;
                float height = (float)m_height;
                plasma::Matrix<float> projection;
                projection.identity();
                projection.translate((translucent.instance.position[0] - width * 0.5f) / width * 2.0f,
                                     (translucent.instance.position[1] - height * 0.5f) / height * -2.0f, -0.5f);
                projection *= m_projectionMatrix;
                setProjectionMatrix(projection);
            }
            setDepthOffset(translucent.instance.parameters[3]);
            sprite->draw();
        }
    }
    if (!batch.m_translucent.empty()) {
        activateMode(RendererMode_AlphaFront);
        for (SpriteBatch::TranslucentInstance &translucent : batch.m_translucent) {
            cube::Sprite *sprite = translucent.sprite;
            SpriteBatch::Instance &instance = translucent.instance;
            if (flags & SpritePassFlagZoneLights) {
                plasma::Vector<3, float> center =
                    instance.matrix * plasma::Vector<3, float>((float)sprite->m_size[0] * 0.5f,
                                                               (float)sprite->m_size[1] * 0.5f,
                                                               (float)sprite->m_size[2] * 0.5f);
                int zoneX = cube::floorDivide((cube::Fixed(center[0]) - m_drawOriginX).toBlock(), cube::BlocksPerZone);
                int zoneY = cube::floorDivide((cube::Fixed(center[1]) - m_drawOriginY).toBlock(), cube::BlocksPerZone);
                cube::Zone *zone = m_world->zone(zoneX, zoneY);
                if (zone)
                    setPointLights(zone->m_chunk.lightPositions, zone->m_chunk.lightColors);
                else
                    clearPointLights();
            }
            setWorldMatrix(instance.matrix);
            if (flags & SpritePassFlagScreenPosition) {
                float width = (float)m_width;
                float height = (float)m_height;
                plasma::Matrix<float> projection;
                projection.identity();
                projection.translate((instance.position[0] - width * 0.5f) / width * 2.0f,
                                     (instance.position[1] - height * 0.5f) / height * -2.0f, -0.5f);
                projection *= m_projectionMatrix;
                setProjectionMatrix(projection);
            }
            setMaterialColor(instance.color);
            setShininess(instance.parameters[1]);
            setGlow(instance.parameters[0]);
            setAlpha(instance.alpha);
            setWhite(instance.parameters[2]);
            setDepthOffset(instance.parameters[3]);
            sprite->draw();
        }
    }
    if (!batch.m_mirroredTranslucent.empty()) {
        activateMode(RendererMode_AlphaBack);
        for (SpriteBatch::TranslucentInstance &translucent : batch.m_mirroredTranslucent) {
            cube::Sprite *sprite = translucent.sprite;
            if (flags & SpritePassFlagZoneLights) {
                plasma::Vector<3, float> center =
                    translucent.instance.matrix * plasma::Vector<3, float>((float)sprite->m_size[0] * 0.5f,
                                                                           (float)sprite->m_size[1] * 0.5f,
                                                                           (float)sprite->m_size[2] * 0.5f);
                int zoneX = cube::floorDivide((cube::Fixed(center[0]) - m_drawOriginX).toBlock(), cube::BlocksPerZone);
                int zoneY = cube::floorDivide((cube::Fixed(center[1]) - m_drawOriginY).toBlock(), cube::BlocksPerZone);
                cube::Zone *zone = m_world->zone(zoneX, zoneY);
                if (zone)
                    setPointLights(zone->m_chunk.lightPositions, zone->m_chunk.lightColors);
                else
                    clearPointLights();
            }
            setWorldMatrix(translucent.instance.matrix);
            if (flags & SpritePassFlagScreenPosition) {
                float width = (float)m_width;
                float height = (float)m_height;
                plasma::Matrix<float> projection;
                projection.identity();
                projection.translate((translucent.instance.position[0] - width * 0.5f) / width * 2.0f,
                                     (translucent.instance.position[1] - height * 0.5f) / height * -2.0f, -0.5f);
                projection *= m_projectionMatrix;
                setProjectionMatrix(projection);
            }
            setMaterialColor(translucent.instance.color);
            setShininess(translucent.instance.parameters[1]);
            setGlow(translucent.instance.parameters[0]);
            setAlpha(translucent.instance.alpha);
            setWhite(translucent.instance.parameters[2]);
            setDepthOffset(translucent.instance.parameters[3]);
            sprite->draw();
        }
    }
}

void Renderer::renderWater()
{
    activateMode(RendererMode_DepthTwoSided);
    setWhite(0.0f);
    setGlow(0.0f);
    setMaterialColor(m_tint);
    clearPointLights();
    setAlpha(m_waterAlpha);
    for (cube::Zone *zone : m_zones) {
        if (!zone->m_chunk.chunkBuffers.empty()) {
            for (ChunkBuffer *buffer : zone->m_chunk.chunkBuffers) {
                if (buffer->indexBuffers[ChunkPassWater]) {
                    plasma::Matrix<float> model;
                    model.identity();
                    model.translate((float)((cube::Fixed(zone->m_position[0] * cube::BlocksPerZone)) + m_drawOriginX),
                                    (float)((cube::Fixed(zone->m_position[1] * cube::BlocksPerZone)) + m_drawOriginY),
                                    (float)buffer->baseZ);
                    setWorldMatrix(model);
                    if (buffer->vertexBuffer->bind() && buffer->indexBuffers[ChunkPassWater]->bind()) {
                        m_world->m_spriteManager->graphics()->drawIndexed(buffer->vertexCount,
                                                                          buffer->triangleCounts[ChunkPassWater]);
                    }
                }
            }
        }
    }
    activateMode(RendererMode_Water);
    setViewMatrix(m_viewMatrix);
    setProjectionMatrix(m_projectionMatrix);
    for (cube::Zone *zone : m_zones) {
        if (!zone->m_chunk.chunkBuffers.empty()) {
            for (ChunkBuffer *buffer : zone->m_chunk.chunkBuffers) {
                if (buffer->indexBuffers[ChunkPassWater]) {
                    setPointLights(zone->m_chunk.lightPositions, zone->m_chunk.lightColors);
                    plasma::Matrix<float> model;
                    model.identity();
                    model.translate((float)((cube::Fixed(zone->m_position[0] * cube::BlocksPerZone)) + m_drawOriginX),
                                    (float)((cube::Fixed(zone->m_position[1] * cube::BlocksPerZone)) + m_drawOriginY),
                                    (float)buffer->baseZ);
                    setWorldMatrix(model);
                    if (buffer->vertexBuffer->bind() && buffer->indexBuffers[ChunkPassWater]->bind()) {
                        m_world->m_spriteManager->graphics()->drawIndexed(buffer->vertexCount,
                                                                          buffer->triangleCounts[ChunkPassWater]);
                    }
                }
            }
        }
    }
    setAlpha(1.0f);
}

void Renderer::resetViewDistance()
{
    m_viewDistance = 0.0f;
}

void rotateTowards(plasma::Matrix<float> &matrix, const plasma::Vector<3, float> &from,
                   const plasma::Vector<3, float> &to)
{
    float fromLength = sqrtf(from[0] * from[0] + from[1] * from[1] + from[2] * from[2]);
    if (fabsf(fromLength) < RotationVectorLengthEpsilon)
        return;
    float toLength = sqrtf(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
    if (fabsf(toLength) < RotationVectorLengthEpsilon)
        return;
    plasma::Vector<3, float> axis(to[2] * from[1] - to[1] * from[2], to[0] * from[2] - to[2] * from[0],
                                  to[1] * from[0] - to[0] * from[1]);
    if (fabsf(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]) < RotationAxisSquaredLengthEpsilon)
        return;
    float angle = acosf((to[0] * from[0] + to[1] * from[1] + to[2] * from[2]) / (toLength * fromLength));
    matrix.rotate(angle * 57.29578f, axis[0], axis[1], axis[2]);
}

// Whether a block is neither air nor water.
static inline bool blocksLight(const cube::Block &block)
{
    return block.isSolid();
}

plasma::Vector<4, float> Renderer::shade(cube::ZoneNeighborhood &zones, ByteGrid &grid, int x, int y, int z,
                                         const plasma::Vector<4, float> &color, const plasma::Vector<3, int> &direction,
                                         bool plain)
{
    if (plain)
        return color;
    // The blocks around the corner on the face's side: one layer along an axis the direction points along, three
    // blocks each way otherwise.
    int fromX = -3;
    int toX = 3;
    if (direction[0] > 0) {
        fromX = 0;
        toX = 1;
    }
    if (direction[0] < 0) {
        fromX = -1;
        toX = 0;
    }
    int fromY = -3;
    int toY = 3;
    if (direction[1] > 0) {
        fromY = 0;
        toY = 1;
    }
    if (direction[1] < 0) {
        fromY = -1;
        toY = 0;
    }
    int fromZ = -3;
    int toZ = 3;
    if (direction[2] > 0) {
        fromZ = 0;
        toZ = 1;
    }
    if (direction[2] < 0) {
        fromZ = -1;
        toZ = 0;
    }
    float open = 0.0f;
    float weightSum = 0.0f;
    float openSum = 0.0f;
    float lightSum = 0.0f;
    float lightWeightSum = 0.0f;
    for (int i = fromX; i < toX; i++) {
        for (int j = fromY; j < toY; j++) {
            for (int k = fromZ; k < toZ; k++) {
                float distanceY = (float)fabs((float)j + 0.5f);
                float distanceX = (float)fabs((float)i + 0.5f);
                float distance = std::max(distanceY, distanceX);
                float distanceZ = (float)fabs((float)k + 0.5f);
                distance = std::max(distanceZ, distance);
                cube::Block block = zones.cube(x + i, y + j, z + k);
                float weight = (3.0f - distance) / 3.0f;
                weightSum += weight;
                openSum += (float)!blocksLight(block) * weight;
                if (!blocksLight(block)) {
                    unsigned char light = grid.lookup(x + i, y + j, z + k);
                    lightWeightSum += weight;
                    lightSum += (float)light * weight;
                }
            }
        }
    }
    if (weightSum > 0.0f)
        open = openSum / weightSum;
    float light = 0.0f;
    if (lightWeightSum > 0.0f)
        light = lightSum / lightWeightSum;
    plasma::Vector<3, float> rgb(color[0], color[1], color[2]);
    float exposure = powf(open, VoxelAmbientExposurePower);
    rgb *= exposure;
    float gray = rgb[0] * 0.3f + rgb[1] * 0.59f + rgb[2] * 0.11f;
    if (gray > 1.0f) {
        rgb *= 1.0f / gray;
        gray = 1.0f;
    }
    // Dark corners keep more than their share of colour: the saturation goes from 3 in full shadow to 1 in the open.
    float saturation = 3.0f - exposure * 2.0f;
    plasma::Vector<3, float> shaded =
        plasma::Vector<3, float>(gray, gray, gray) * (1.0f - saturation) + rgb * saturation;
    for (int i = 0; i < 3; i++) {
        if (shaded[i] < 0.0f)
            shaded[i] = 0.0f;
        else if (shaded[i] > 1.0f)
            shaded[i] = 1.0f;
    }
    return plasma::Vector<4, float>(shaded[0], shaded[1], shaded[2], light / 255.0f);
}

// Draws the world map and manages its selected-flight-point highlight.
inline void Renderer::renderMap(const plasma::Vector<3, cube::Fixed> &center, int zoom, float unused,
                                const plasma::Vector<2, float> &cursor, float viewScaleFactor, bool controlKeyDown,
                                bool passiveView, float frameTime)
{
    // The strength of the highlight on the selected flight point, 0..1: eased towards 1 while one is selected, reset
    // when none is.
    static float highlight;
    m_worldMap->enter();
    activateMode(RendererMode_OpaqueFront);
    m_worldMap->render(m_mapBatch, center, (int)((float)m_width * cursor[0]), (int)((float)m_height * cursor[1]),
                       m_width, m_height,
                       plasma::Vector<3, float>(std::min(-m_cameraAngle[0], MapPitchClamp), 0.0f, -m_cameraAngle[2]),
                       zoom, viewScaleFactor, (int)m_frameTime, controlKeyDown, passiveView);
    // Inverts a copy of the map's view matrix and does not use the result.
    plasma::Matrix<float> inverseView = m_worldMap->m_viewMatrix;
    inverseView.invert();
    setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    // Map lighting rig: normalized fixed direction, warm front RGBA and neutral ambient palette.
    plasma::Vector<3, float> direction(0.2f, 0.5f, 1.0f);
    direction *= 1.0f / sqrtf(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
    plasma::Vector<4, float> lightColor(0.75f, 0.7f, 0.3f, 1.0f);
    plasma::Vector<4, float> ambientColor(0.5f, 0.5f, 0.5f, 1.0f);
    plasma::Vector<4, float> backgroundColor(0.25f, 0.3f, 0.7f, 1.0f);
    setLight(lightColor, ambientColor, direction, backgroundColor);
    clearPointLights();
    setFogScale(MapFogDistance);
    setViewMatrix(m_worldMap->m_viewMatrix);
    setProjectionMatrix(m_worldMap->m_projectionMatrix);
    if (m_worldMap->m_selectedFlightPoint[0] >= 0) {
        int scale = (int)(powf(2.0f, (float)zoom) * FlightPointMapScaleBase);
        highlight += cube::blendFactor(frameTime, FlightPointHighlightRate) * (1.0f - highlight);
        float inverseScale = 1.0f / (float)scale;
        plasma::Vector<3, cube::Fixed> offset =
            plasma::Vector<3, cube::Fixed>(cube::Fixed(m_worldMap->m_selectedFlightPoint[0]),
                                           cube::Fixed(m_worldMap->m_selectedFlightPoint[1]), cube::Fixed(0)) -
            center;
        plasma::Vector<3, float> blocks((float)offset[0], (float)offset[1], (float)offset[2]);
        setHighlight(plasma::Vector<4, float>(blocks[0] * inverseScale, blocks[1] * inverseScale, highlight,
                                              (float)(FlightPointHighlightRadius / scale)));
    } else {
        highlight = 0.0f;
    }
    renderSprites(m_mapBatch, 0);
    renderTranslucentSprites(m_mapBatch, 0);
    setHighlight(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f));
    m_worldMap->leave();
}

void Renderer::renderParticles(std::list<cube::Particle> &particles)
{
    activateMode(RendererMode_OpaqueTwoSided);
    setWhite(0.0f);
    for (cube::Particle &particle : particles) {
        if (!isSphereVisible(particle.position, particle.size, m_effectViewDistance))
            continue;
        float light = 1.0f;
        float glow = 0.0f;
        float shininess = 0.0f;
        if (particle.type == cube::ParticleLitCube || (unsigned)(particle.type - cube::ParticleLeaf) <= 2)
            light =
                (float)m_world->cubeLight(particle.position[0], particle.position[1], particle.position[2], 0) / 255.0f;
        else
            glow = 1.0f;
        plasma::Vector<4, float> brightness(1.0f, 1.0f, 1.0f, std::max(ParticleMinimumBlockLight, light));
        plasma::Vector<4, float> color = particle.color * m_tint;
        color *= brightness;
        if (!(particle.type == cube::ParticleLitCube || (unsigned)(particle.type - cube::ParticleLeaf) <= 2))
            color[3] = 1.0f;

        cube::Sprite *sprite;
        float scale;
        switch (particle.type) {
        case cube::ParticleMusicNote:
            sprite = m_world->m_spriteManager->sprite(cube::SpriteNote);
            scale = (1.0f - (float)particle.age / (float)particle.lifetime) * MusicNoteParticleScale;
            shininess = 1.0f;
            break;
        case cube::ParticleShrinkingCube:
            sprite = m_world->m_spriteManager->sprite(cube::SpriteCube);
            scale = 1.0f - (float)particle.age / (float)particle.lifetime;
            break;
        default:
            sprite = m_world->m_spriteManager->sprite(cube::SpriteCube);
            scale = 1.0f;
            break;
        }
        setMaterialColor(color);
        setGlow(glow);
        setShininess(shininess);

        plasma::Matrix<float> model;
        model.identity();
        model.translate((float)(particle.position[0] + m_drawOriginX), (float)(particle.position[1] + m_drawOriginY),
                        (float)particle.position[2]);
        model.rotate(particle.rotation[2], 0.0f, 0.0f, 1.0f);
        model.rotate(particle.rotation[1], 0.0f, 1.0f, 0.0f);
        model.rotate(particle.rotation[0], 1.0f, 0.0f, 0.0f);
        model.scale(scale * particle.size, scale * particle.size, scale * particle.size);
        model *= particle.matrix;
        model.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                        (float)sprite->m_size[2] * -0.5f);
        setWorldMatrix(model);
        sprite->draw();
    }
}

void Renderer::renderSprites(SpriteBatch &batch, int flags)
{
    activateMode(RendererMode_OpaqueFront);
    if (!(flags & SpritePassFlagZoneLights))
        clearPointLights();
    for (auto &entry : batch.m_instances) {
        cube::Sprite *sprite = entry.first;
        if (sprite->bind()) {
            for (SpriteBatch::Instance &instance : entry.second) {
                if (flags & SpritePassFlagZoneLights) {
                    plasma::Vector<3, float> center =
                        instance.matrix * plasma::Vector<3, float>((float)sprite->m_size[0] * 0.5f,
                                                                   (float)sprite->m_size[1] * 0.5f,
                                                                   (float)sprite->m_size[2] * 0.5f);
                    int zoneX =
                        cube::floorDivide((cube::Fixed(center[0]) - m_drawOriginX).toBlock(), cube::BlocksPerZone);
                    int zoneY =
                        cube::floorDivide((cube::Fixed(center[1]) - m_drawOriginY).toBlock(), cube::BlocksPerZone);
                    cube::Zone *zone = m_world->zone(zoneX, zoneY);
                    if (zone)
                        setPointLights(zone->m_chunk.lightPositions, zone->m_chunk.lightColors);
                    else
                        clearPointLights();
                }
                setWorldMatrix(instance.matrix);
                if (flags & SpritePassFlagScreenPosition) {
                    float width = (float)m_width;
                    float height = (float)m_height;
                    plasma::Matrix<float> projection;
                    projection.identity();
                    projection.translate((instance.position[0] - width * 0.5f) / width * 2.0f,
                                         (instance.position[1] - height * 0.5f) / height * -2.0f, -0.5f);
                    projection *= m_projectionMatrix;
                    setProjectionMatrix(projection);
                }
                setMaterialColor(instance.color);
                setShininess(instance.parameters[1]);
                setGlow(instance.parameters[0]);
                setAlpha(instance.alpha);
                setWhite(instance.parameters[2]);
                setDepthOffset(instance.parameters[3]);
                sprite->drawBound();
            }
            endChunkBufferDraw();
        }
    }
    if (batch.m_mirroredInstances.size()) {
        activateMode(RendererMode_OpaqueBack);
        for (auto &entry : batch.m_mirroredInstances) {
            cube::Sprite *sprite = entry.first;
            if (sprite->bind()) {
                for (SpriteBatch::Instance &instance : entry.second) {
                    if (flags & SpritePassFlagZoneLights) {
                        plasma::Vector<3, float> center =
                            instance.matrix * plasma::Vector<3, float>((float)sprite->m_size[0] * 0.5f,
                                                                       (float)sprite->m_size[1] * 0.5f,
                                                                       (float)sprite->m_size[2] * 0.5f);
                        int zoneX =
                            cube::floorDivide((cube::Fixed(center[0]) - m_drawOriginX).toBlock(), cube::BlocksPerZone);
                        int zoneY =
                            cube::floorDivide((cube::Fixed(center[1]) - m_drawOriginY).toBlock(), cube::BlocksPerZone);
                        cube::Zone *zone = m_world->zone(zoneX, zoneY);
                        if (zone)
                            setPointLights(zone->m_chunk.lightPositions, zone->m_chunk.lightColors);
                        else
                            clearPointLights();
                    }
                    setWorldMatrix(instance.matrix);
                    if (flags & SpritePassFlagScreenPosition) {
                        float width = (float)m_width;
                        float height = (float)m_height;
                        plasma::Matrix<float> offset;
                        offset.identity();
                        offset.translate((instance.position[0] - width * 0.5f) / width * 2.0f,
                                         (instance.position[1] - height * 0.5f) / height * -2.0f, -0.5f);
                        setProjectionMatrix(offset * m_projectionMatrix);
                    }
                    setMaterialColor(instance.color);
                    setShininess(instance.parameters[1]);
                    setGlow(instance.parameters[0]);
                    setAlpha(instance.alpha);
                    setWhite(instance.parameters[2]);
                    setDepthOffset(instance.parameters[3]);
                    sprite->drawBound();
                }
            }
        }
    }
}

void Renderer::updateViewDistance(float frameTime)
{
    float squaredDistance = (float)(m_zoneRadius * cube::BlocksPerZone);
    m_dungeonShade = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
    squaredDistance *= squaredDistance;
    cube::Fixed cameraX = m_cameraPosition[0];
    int zoneX = cube::floorDivide(cameraX.toBlock(), cube::BlocksPerZone);
    cube::Fixed cameraY = m_cameraPosition[1];
    int zoneY = cube::floorDivide(cameraY.toBlock(), cube::BlocksPerZone);

    // Not farther than the nearest zone that has no mesh yet.
    for (int x = zoneX - m_zoneRadius; x <= zoneX + m_zoneRadius; x++) {
        for (int y = zoneY - m_zoneRadius; y <= zoneY + m_zoneRadius; y++) {
            cube::Zone *zone = m_world->zone(x, y);
            if (zone && !zone->m_chunk.chunkBuffers.empty())
                continue;
            cube::Fixed centerX((cube::Fixed(x) + 0.5).m_raw * cube::BlocksPerZone);
            cube::Fixed centerY((cube::Fixed(y) + 0.5).m_raw * cube::BlocksPerZone);
            cube::Fixed dx = centerX - cameraX;
            cube::Fixed dy = centerY - cameraY;
            double squared = (double)dx * (double)dx + (double)dy * (double)dy;
            if (squaredDistance > squared)
                squaredDistance = (float)squared;
        }
    }

    // Not farther than the zones whose triangles fit the budget, nearest first.
    std::vector<std::pair<float, int>> zones;
    for (int x = zoneX - m_zoneRadius; x <= zoneX + m_zoneRadius; x++) {
        for (int y = zoneY - m_zoneRadius; y <= zoneY + m_zoneRadius; y++) {
            cube::Zone *zone = m_world->zone(x, y);
            if (!zone || zone->m_chunk.chunkBuffers.empty())
                continue;
            if (!isBoxVisible(zone->m_chunk.boundsMin, zone->m_chunk.boundsMax, MeshBudgetVisibilityRange))
                continue;
            cube::Fixed dx = zone->m_chunk.center[0] - m_cameraPosition[0];
            cube::Fixed dy = zone->m_chunk.center[1] - m_cameraPosition[1];
            double squared = (double)dy * (double)dy + (double)dx * (double)dx;
            if (squaredDistance >= squared) {
                std::pair<float, int> entry((float)squared, zone->m_chunk.triangleCount);
                zones.push_back(entry);
            }
        }
    }
    std::sort(zones.begin(), zones.end(),
              [](const std::pair<float, int> &a, const std::pair<float, int> &b) { return a.first < b.first; });
    long long budget = (long long)m_zoneRadius * m_zoneRadius * TriangleBudgetMultiplier << TriangleBudgetShift;
    long long triangles = 0;
    float previous = 0.0f;
    for (std::vector<std::pair<float, int>>::iterator zone = zones.begin(); zone != zones.end(); ++zone) {
        triangles += zone->second;
        if (triangles >= budget) {
            squaredDistance = (std::min)(zone->first, previous);
            break;
        }
        previous = zone->first;
    }
    float distance = sqrtf(squaredDistance) - ViewDistanceSafetyInset;
    if (distance < MinimumViewDistance)
        distance = MinimumViewDistance;

    // Inside a dungeon with a shade of its own the distance stays short.
    bool shaded = false;
    cube::Creature *creature = m_world->m_localCreature;
    cube::Block block = m_world->getBlockAtDot(creature->m_entityData.position[0], creature->m_entityData.position[1],
                                               creature->m_entityData.position[2]);
    if (block.isInDungeon() && (block.flags & cube::BlockFlagProtected)) {
        plasma::Vector<3, float> shade = m_world->dungeonShade((int)m_world->m_localCreature->m_entityData.position[0],
                                                               (int)m_world->m_localCreature->m_entityData.position[1]);
        m_dungeonShade[0] = shade[0] / 255.0f;
        m_dungeonShade[1] = shade[1] / 255.0f;
        m_dungeonShade[2] = shade[2] / 255.0f;
        if (shade != plasma::Vector<3, float>(0.0f, 0.0f, 0.0f))
            shaded = true;
    }
    bool inSpiritworld = m_world->m_localCreature->findBuff(cube::BuffSpiritworld) != nullptr;
    cube::Block cameraBlock = m_world->getBlockAtDot(m_cameraPosition[0], m_cameraPosition[1], m_cameraPosition[2]);
    m_waterAlpha += (1.0f - m_waterAlpha) * cube::blendFactor(frameTime, WaterAlphaRecoveryRate);
    if (m_world->m_localCreature->m_entityData.currentAbility == cube::AbilityEagleFlight &&
        m_world->m_localCreature->m_entityData.timeSinceAbility > EagleFlightViewDelayMs) {
        m_viewDistance += ((std::max)(+EagleFlightMinimumViewDistance, distance) - m_viewDistance) *
                          cube::blendFactor(frameTime, EagleFlightViewBlendRate);
    } else if ((shaded || inSpiritworld) && distance > ShadedMaximumViewDistance) {
        m_viewDistance +=
            (ShadedMaximumViewDistance - m_viewDistance) * cube::blendFactor(frameTime, ViewDistanceGrowthRate);
    } else if (distance > m_viewDistance) {
        m_viewDistance += (distance - m_viewDistance) * cube::blendFactor(frameTime, ViewDistanceGrowthRate);
    } else {
        m_viewDistance = distance;
    }
}

// The frame profile: frames counted and milliseconds summed per stage, reset after 20 frames (nothing reads them).
static unsigned int s_frameCount;
static unsigned int s_setupTime;
static unsigned int s_skyTime;
static unsigned int s_collectTime;
static unsigned int s_chunkTime;
static unsigned int s_spriteTime;
static unsigned int s_waterTime;
static unsigned int s_worldTime;
static unsigned int s_overlayTime;
static unsigned int s_interfaceTime;

void Renderer::render(const plasma::Vector<3, cube::Fixed> &cameraPosition,
                      const plasma::Vector<3, cube::Fixed> &focusPosition, const plasma::Vector<3, float> &cameraAngle,
                      int width, int height, float step, plasma::Matrix<float> &worldViewMatrix,
                      plasma::Matrix<float> &viewMatrix, plasma::Matrix<float> &projectionMatrix, int zoneRadius,
                      const plasma::Vector<3, cube::Fixed> &mapPosition, int mapZoom, float interfaceScale,
                      std::list<cube::Particle> &particles, std::list<cube::Light> &lights,
                      std::vector<cube::MenuEntry> &usableItems, float ringRadius, float ringRotation,
                      const int *lookedAtBlock, const plasma::Vector<2, float> &mapOverlayPosition,
                      float mapOverlayScale, int flags)
{
    unsigned int start = timeGetTime();
    m_cameraPosition = cameraPosition;
    m_cameraAngle = cameraAngle;
    m_drawOriginX = -focusPosition[0];
    m_drawOriginY = -focusPosition[1];
    m_frameTime = step;
    m_worldViewMatrix = worldViewMatrix;
    m_viewMatrix = viewMatrix;
    m_projectionMatrix = projectionMatrix;
    m_zoneRadius = zoneRadius;
    m_width = width;
    m_height = height;

    // Daylight from the time of day: 0.05 at midnight, 1 from the morning to the afternoon. Daylight curve: fourth
    // power of the symmetric daily parabola, doubled, with a midnight floor of 0.05.
    float daylight =
        powf(1.0f - powf(m_world->m_state.m_time * 2.0f / cube::MillisecondsPerDay - 1.0f, 2.0f), 4.0f) * 2.0f;
    m_daylight = (std::min)(1.0f, (std::max)(0.05f, daylight));
    extractFrustum();
    updateViewDistance(step);

    m_zones.clear();
    m_lights.clear();
    m_worldBatch.clear();
    m_underwaterPlantBatch.clear();
    m_mapBatch.clear();
    m_screenBatch.clear();
    m_impacts.clear();
    m_boxes.clear();
    m_particles.clear();
    m_cubes.clear();
    m_lines.clear();
    m_trailCreatures.clear();
    m_groundSpots.clear();
    m_shadows.clear();
    m_effectViewDistance =
        (std::min)(+MaximumEffectViewDistance, (std::min)(m_viewDistance, (float)(m_zoneRadius * cube::BlocksPerZone) *
                                                                              EffectViewDistanceFraction));
    m_creatureViewDistance = (std::min)(
        +MaximumCreatureViewDistance,
        (std::min)(m_viewDistance, (float)(m_zoneRadius * cube::BlocksPerZone) * CreatureViewDistanceFraction));

    // Under water everything is tinted blue.
    m_tint = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    unsigned char cameraBlockType =
        m_world->getBlockAtDot(m_cameraPosition[0], m_cameraPosition[1], m_cameraPosition[2]).type;
    if (cameraBlockType == cube::BlockWater)
        m_tint = plasma::Vector<4, float>(0.5f, 0.5f, 1.0f, 1.0f);

    // The light and sky colours move towards those of the place.
    plasma::Vector<4, float> skyColor1;
    plasma::Vector<4, float> skyColor2;
    plasma::Vector<4, float> fogColor;
    plasma::Vector<4, float> sunColor;
    plasma::Vector<4, float> light;
    plasma::Vector<4, float> backLightColor;
    plasma::Vector<4, float> ambientColor;
    m_world->harvestEnvironmentColors(m_cameraPosition, skyColor1, skyColor2, fogColor, sunColor, light, backLightColor,
                                      ambientColor);
    plasma::Vector<4, float> lightDifference = light - m_lightFrontColor;
    float factor = cube::blendFactor(step, EnvironmentLightBlendRate);
    m_lightFrontColor += lightDifference * factor;
    m_lightBackColor += (backLightColor - m_lightBackColor) * factor;
    m_ambientColor += (ambientColor - m_ambientColor) * factor;
    if (m_dungeonShade == plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f)) {
        m_skyColors[2] += (fogColor - m_skyColors[2]) * cube::blendFactor(step, FogColorBlendRate);
        m_skyColors[3] += (sunColor - m_skyColors[3]) * cube::blendFactor(step, SunColorBlendRate);
        cube::blend(m_skyColors[0], skyColor1, step, EnvironmentLightBlendRate);
        cube::blend(m_skyColors[1], skyColor2, step, EnvironmentLightBlendRate);
    } else {
        // A dungeon's shade instead of the sky.
        cube::blend(m_skyColors[2], m_dungeonShade, step, FogColorBlendRate);
        m_skyColors[3] += (plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f) - m_skyColors[3]) *
                          cube::blendFactor(step, FogColorBlendRate);
        cube::blend(m_skyColors[0], m_dungeonShade, step, EnvironmentLightBlendRate);
        cube::blend(m_skyColors[1], m_dungeonShade, step, EnvironmentLightBlendRate);
    }

    // The sun goes round once a day.
    float sunAngle = (float)(m_world->m_state.m_time * 2.0f * cube::Pi / cube::MillisecondsPerDay);
    float sunX = sinf(sunAngle);
    float sunY = sinf(sunAngle * 0.5f) * SunOrbitHorizontalAmplitude;
    float sunZ = -cosf(sunAngle);
    plasma::Vector<3, float> sunDirection = normalized(plasma::Vector<3, float>(sunX, sunY, sunZ));

    beginScene();
    activateMode(RendererMode_OpaqueFront);
    setWhite(0.0f);
    setMaterialColor(m_tint);
    setAlpha(1.0f);
    setLight(m_lightFrontColor, m_lightBackColor, sunDirection, m_ambientColor);
    setShininess(0.0f);
    setGlow(0.0f);
    setDepthOffset(0.0f);
    setSky(m_skyColors[0], m_skyColors[1], m_skyColors[2], m_skyColors[3]);
    setFogScale(cameraBlockType == cube::BlockWater ? (std::min)(+UnderwaterMaximumFogDistance, m_viewDistance)
                                                    : m_viewDistance);
    setFogGradientTranslation(cameraAngle[0]);
    setSunScreenPosition(cameraBlockType == cube::BlockWater ? plasma::Vector<3, float>(0.0f, 0.0f, -1.0f)
                                                             : sunDirection,
                         m_viewMatrix, m_projectionMatrix, cameraAngle[0], width, height);
    unsigned int afterSetup = timeGetTime();

    int worldHidden = flags & RenderFlagOnlyPlayer;
    if (!worldHidden) {
        drawSky();
        if (m_daylight < NightSkyDaylightCutoff && cameraBlockType != cube::BlockWater)
            drawNightSky();
    }

    // Undead land is darker.
    cube::World::BiomeSample sample;
    m_world->biomeSample((int)m_cameraPosition[0], (int)m_cameraPosition[1], sample);
    cube::World::AreaInfo info;
    m_world->fillAreaInfo((int)m_cameraPosition[0], (int)m_cameraPosition[1], info);
    m_daylight *= 1.0f - sample.undead(info.type, info.influence) * UndeadDaylightReduction;
    float modelScale = 1024.0f / (float)m_height;

    unsigned int afterSky = timeGetTime();
    // The block looked at arrives as the address of three ints (Game's member); collectSprites compares it as a
    // vector.
    collectSprites(*(const plasma::Vector<3, int> *)lookedAtBlock, flags);
    collectLights(lights);
    unsigned int afterCollect = timeGetTime();

    activateMode(RendererMode_OpaqueFront);
    setViewMatrix(m_viewMatrix);
    setProjectionMatrix(m_projectionMatrix);
    setAmbientColor(m_ambientColor);
    renderChunks(m_lightFrontColor, m_lightBackColor, sunDirection, m_ambientColor);
    unsigned int afterChunks = timeGetTime();
    renderSprites(m_worldBatch, SpritePassFlagZoneLights);
    renderSprites(m_underwaterPlantBatch, SpritePassFlagZoneLights);
    renderTranslucentSprites(m_underwaterPlantBatch, SpritePassFlagZoneLights);
    unsigned int afterSprites = timeGetTime();
    renderEffects();
    if (!worldHidden)
        renderParticles(particles);
    setLight(m_lightFrontColor, m_lightBackColor, sunDirection, m_ambientColor);
    renderWater();
    unsigned int afterWater = timeGetTime();
    drawGroundSpots();
    activateMode(RendererMode_DepthFront);
    setProjectionMatrix(m_projectionMatrix);
    renderTranslucentSprites(m_worldBatch, SpritePassFlagZoneLights);

    if (!worldHidden) {
        renderClouds(skyColor1, skyColor2, m_lightFrontColor, m_lightBackColor, sunDirection, m_ambientColor);

        // The stars, at night and on a long flight: fifty rings of cubes around the camera, fading in below a daylight
        // of 0.4.
        bool flying = m_world->isFlyingFar();
        if (m_daylight < 0.4f || flying) {
            setFogScale(StarFieldFogDistance);
            float alpha = 1.0f;
            if (!flying)
                alpha = (std::min)(1.0f, (1.0f - m_daylight / 0.4f) * 4.0f);
            setAlpha(alpha);
            setGlow(1.0f);
            for (int ring = 0; ring < 50; ring++) {
                int count = (int)(50.0f - sqrtf((float)ring) * 7.0f);
                for (int i = 0; i < count; i++) {
                    unsigned int seed = cube::random(i, ring);
                    if (seed & 1)
                        continue;
                    plasma::Matrix<float> model;
                    model.identity();
                    seed /= 2;
                    model.rotateZ((float)i * 360.0f / (float)count + (float)(ring * 13));
                    model.rotateX((float)ring * 90.0f / 50.0f);
                    model.translate(
                        plasma::Vector<3, float>(0.0f, (float)((m_zoneRadius * cube::BlocksPerZone) - 10), 0.0f));
                    float size = (float)(seed % 1000) / 500.0f + 1.0f;
                    model.scale(size, size, size);
                    setWorldMatrix(model);
                    m_world->m_spriteManager->sprite(cube::SpriteCube)->draw();
                }
            }
        }
    }
    drawShadows(m_lightFrontColor);
    drawTrails();
    unsigned int afterWorld = timeGetTime();

    // The start menu's logo, swaying.
    if (flags & RenderFlagLogo) {
        // Logo HUD pose curve: clock sway period, relative screen origin, tilt and model scale.
        float sway = cosf((float)((float)(afterWorld % 10000) * 0.0001f * 2.0f * cube::Pi));
        m_screenBatch.addAt(m_world->m_spriteManager->sprite(cube::SpriteCubeworld), (float)m_width * 0.5f,
                            (sway * 0.01f * 5.0f + 0.25f) * (float)m_height,
                            plasma::Vector<3, float>(20.0f - sway * 5.0f, 180.0f, 0.0f),
                            interfaceScale * 0.011f * modelScale, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f),
                            0.0f, 0.0f, 0.0f);
    }

    if (!m_world->m_worldName.empty() && !(flags & RenderFlagHideHud)) {
        renderMap(mapPosition, mapZoom, interfaceScale, mapOverlayPosition, mapOverlayScale,
                  (flags & RenderFlagMapModifier) != 0, !(flags & RenderFlagMapOpen), step);

        // The compass, the turning sprite 2785 with its shadow, and the special items of the character: each black and
        // lowered first.
        m_screenBatch.addAt(
            m_world->m_spriteManager->sprite(cube::SpriteCompass2), (float)m_width - interfaceScale * 60.0f,
            interfaceScale * 20.0f, plasma::Vector<3, float>(-120.0f, 0.0f, -m_cameraAngle[2]),
            interfaceScale * 0.003f * modelScale, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 0.0f, 0.0f);
        m_screenBatch.addAt(
            m_world->m_spriteManager->sprite(cube::SpriteGoldCoin), interfaceScale * 50.0f, interfaceScale * 57.0f,
            plasma::Vector<3, float>(-30.0f, (float)((int)m_world->m_clock % 10000) * 0.0001f * 360.0f, 0.0f),
            interfaceScale * 0.004f * modelScale, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f), 0.0f, 1.0f, -0.5f);
        m_screenBatch.addAt(
            m_world->m_spriteManager->sprite(cube::SpriteGoldCoin), interfaceScale * 45.0f, interfaceScale * 52.0f,
            plasma::Vector<3, float>(-30.0f, (float)((int)m_world->m_clock % 10000) * 0.0001f * 360.0f, 0.0f),
            interfaceScale * 0.004f * modelScale, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 1.0f, 0.0f);

        int x = 225;
        if (m_world->m_localCreature->m_playerData->contextItem.category != cube::ItemCategoryNone) {
            cube::Sprite *sprite =
                m_world->m_spriteManager->itemSprite(&m_world->m_localCreature->m_playerData->contextItem);
            int sizeX = sprite->m_size[0];
            int sizeY = sprite->m_size[1];
            int sizeZ = sprite->m_size[2];
            float scale = 0.035f / (float)(std::max)(16, (std::max)((std::max)(sizeX, sizeY), sizeZ)) * interfaceScale *
                          modelScale;
            m_screenBatch.addAt(
                sprite, interfaceScale * 230.0f, interfaceScale * 57.0f,
                plasma::Vector<3, float>(-120.0f, 0.0f, (float)((int)m_world->m_clock % 10000) * 0.0001f * 360.0f),
                scale, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f), 0.0f,
                m_world->m_localCreature->m_playerData->contextItem.shininess(), -0.5f);
            m_screenBatch.addAt(
                sprite, interfaceScale * 225.0f, interfaceScale * 52.0f,
                plasma::Vector<3, float>(-120.0f, 0.0f, (float)((int)m_world->m_clock % 10000) * 0.0001f * 360.0f),
                scale, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0.0f,
                m_world->m_localCreature->m_playerData->contextItem.shininess(), 0.0f);
            x = 325;
        }
        // The category 24 items of this region in the second inventory tab (ids 14 to 17 excepted).
        if (m_world->m_localCreature->m_inventory.m_inventoryTabs.size() > 1) {
            for (cube::ItemStack &stack : m_world->m_localCreature->m_inventory.m_inventoryTabs[1]) {
                if (stack.quantity == 0 || stack.item.category != cube::ItemCategorySpecial)
                    continue;
                if (!(stack.item.region == m_world->m_localCreature->m_entityData.region))
                    continue;
                if ((unsigned)(stack.item.id - cube::SpecialItemMagicCraftingBook) <=
                    cube::SpecialItemLegendaryCraftingBook - cube::SpecialItemMagicCraftingBook)
                    continue;
                cube::Sprite *sprite = m_world->m_spriteManager->itemSprite(&stack.item);
                int sizeX = sprite->m_size[0];
                int sizeY = sprite->m_size[1];
                int sizeZ = sprite->m_size[2];
                float scale = 0.035f / (float)(std::max)(16, (std::max)((std::max)(sizeX, sizeY), sizeZ)) *
                              interfaceScale * modelScale;
                m_screenBatch.addAt(
                    sprite, (float)(x + 5) * interfaceScale, interfaceScale * 57.0f,
                    plasma::Vector<3, float>(-120.0f, 0.0f, (float)((int)m_world->m_clock % 10000) * 0.0001f * 360.0f),
                    scale, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f), 0.0f, stack.item.shininess(), -0.5f);
                m_screenBatch.addAt(
                    sprite, (float)x * interfaceScale, interfaceScale * 52.0f,
                    plasma::Vector<3, float>(-120.0f, 0.0f, (float)((int)m_world->m_clock % 10000) * 0.0001f * 360.0f),
                    scale, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, stack.item.shininess(), 0.0f);
                x += 50;
            }
        }
    }

    // The ring menu: the entries on a circle around the middle of the window, a creature as its model and an item as
    // its sprite.
    if (!usableItems.empty()) {
        int count = (int)usableItems.size();
        // Ring-menu preview pose: clock rotation, fixed yaw, depth, slot spacing and model scales.
        plasma::Vector<3, float> rotation(225.0f, 0.0f, (float)((int)m_world->m_clock % 10000) * 0.0001f * 360.0f);
        float turn = (float)((int)m_world->m_clock % 10000) * 0.0001f * 360.0f * 2.0f;
        for (int i = 0; i < count; i++) {
            float angle = (float)((double)i * cube::TwoPi / (double)count + (double)ringRotation * cube::Pi / 180.0 -
                                  cube::HalfPi);
            float x = cosf(angle) * ringRadius + (float)m_width * 0.5f;
            float y = sinf(angle) * ringRadius + (float)m_height * 0.5f;
            cube::Creature *creature = usableItems[i].creature;
            if (creature) {
                plasma::Vector<3, cube::Fixed> savedDrawnPosition = creature->m_drawnPosition;
                plasma::Vector<2, float> savedPosition = m_screenBatch.m_position;
                plasma::Matrix<float> savedMatrix = m_screenBatch.m_matrix;
                float scale = interfaceScale * 0.8f / creature->m_entityData.appearance.size[0];
                m_screenBatch.m_position = plasma::Vector<2, float>(x, y);
                m_screenBatch.m_matrix.identity();
                m_screenBatch.m_matrix.translate(0.0f, 0.0f, CreaturePreviewDepth);
                m_screenBatch.m_matrix.rotateX(240.0f);
                m_screenBatch.m_matrix.rotateY(0.0f);
                m_screenBatch.m_matrix.rotateZ(turn);
                m_screenBatch.m_matrix.scale(scale, scale, scale);
                creature->m_drawnPosition = plasma::Vector<3, cube::Fixed>(0, 0, 0);
                cube::Fixed originX(0LL);
                cube::Fixed originY(0LL);
                creature->m_animationState.addSprites(m_screenBatch, m_world->m_spriteManager, step,
                                                      plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, originX,
                                                      originY, 0, nullptr, 0.0f, 1.0f);
                creature->m_drawnPosition = savedDrawnPosition;
                m_screenBatch.m_position = savedPosition;
                m_screenBatch.m_matrix = savedMatrix;
            } else {
                cube::Item *item = usableItems[i].item;
                if (item) {
                    cube::Sprite *sprite = m_world->m_spriteManager->itemSprite(item);
                    if (sprite) {
                        int sizeY = sprite->m_size[1];
                        int sizeZ = sprite->m_size[2];
                        int largest = (std::max)(sprite->sizeX(), (std::max)(sizeY, sizeZ));
                        m_screenBatch.addAt(
                            sprite, x, y, rotation, modelScale * interfaceScale * 0.09f / (float)largest,
                            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, item->shininess(), 0.0f);
                    }
                }
            }
        }
    }

    // The interface's sprites, with a fixed light and no view matrix.
    plasma::Matrix<float> identity;
    identity.identity();
    setViewMatrix(identity);
    perspectiveFov(m_projectionMatrix, InterfaceSpriteFieldOfView, -((float)m_width / (float)m_height),
                   InterfaceSpriteNearClip, (float)(m_zoneRadius * cube::BlocksPerZone));
    setProjectionMatrix(m_projectionMatrix);
    setAlpha(1.0f);
    setWhite(0.0f);
    setDepthOffset(0.0f);
    setShininess(0.0f);
    setGlow(0.0f);
    setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    setFogScale(PreviewFogDistance);
    clearPointLights();
    setLight(plasma::Vector<4, float>(0.7f, 0.7f, 0.2f, 1.0f), plasma::Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f),
             normalized(plasma::Vector<3, float>(1.0f, 1.0f, -1.0f)), plasma::Vector<4, float>(0.3f, 0.3f, 0.8f, 1.0f));
    renderSprites(m_screenBatch, SpritePassFlagScreenPosition);
    renderTranslucentSprites(m_screenBatch, SpritePassFlagScreenPosition);
    unsigned int afterOverlay = timeGetTime();
    drawInterface();
    unsigned int afterInterface = timeGetTime();
    endScene();

    s_frameCount++;
    s_setupTime += afterSetup - start;
    s_skyTime += afterSky - start;
    s_collectTime += afterCollect - start;
    s_chunkTime += afterChunks - start;
    s_spriteTime += afterSprites - afterChunks;
    s_waterTime += afterWater - afterChunks;
    s_worldTime += afterWorld - afterChunks;
    s_overlayTime += afterOverlay - afterChunks;
    s_interfaceTime += afterInterface - afterChunks;
    if (s_frameCount > 20) {
        s_frameCount = 0;
        s_setupTime = 0;
        s_skyTime = 0;
        s_collectTime = 0;
        s_chunkTime = 0;
        s_spriteTime = 0;
        s_waterTime = 0;
        s_worldTime = 0;
        s_overlayTime = 0;
        s_interfaceTime = 0;
    }
}

// Whether a block shows the face towards a neighbour: towards the open, and towards water unless the block is water
// itself.
static inline bool isFaceOpen(const cube::Block &block, int neighbourType)
{
    return neighbourType == cube::BlockAir || (block.type != cube::BlockWater && neighbourType == cube::BlockWater);
}

// The colour of every water face.
static inline plasma::Vector<4, float> waterColor()
{
    // Water-face RGBA palette: muted cyan at full opacity.
    return plasma::Vector<4, float>(0.25f, 0.5f, 0.5f, 1.0f);
}

void Renderer::updateChunk(int x, int y)
{
    // One list of vertices per slice of 250 blocks of height (a vertex holds its position in bytes), and the indices
    // of each pass over it. Nothing selects the passes 2, 4 and 5 here.
    std::vector<std::vector<Vertex>> vertices;
    std::vector<std::vector<unsigned int>> solidIndices;
    std::vector<std::vector<unsigned int>> waterIndices;
    std::vector<std::vector<unsigned int>> indices2;
    std::vector<std::vector<unsigned int>> lavaIndices;
    std::vector<std::vector<unsigned int>> indices4;
    std::vector<std::vector<unsigned int>> indices5;
    int x0 = x * cube::BlocksPerZone;
    int y0 = y * cube::BlocksPerZone;

    m_world->enterZones();
    cube::Zone *zone = m_world->zone(x, y);
    cube::ZoneNeighborhood zones(m_world, x, y);
    m_world->leaveZones();
    if (!zone)
        return;

    int y1 = y0 + cube::BlocksPerZone;
    int x1 = x0 + cube::BlocksPerZone;
    ByteGrid grid(zones, x0, y0, x1, y1, ChunkLightReach);
    computeLight(zones, grid, x0, y0, x1, y1, ChunkLightReach);

    // The plants take their light from the grid (the byte at their block); those that stand in water get
    // PlantFlagUnderwater.
    for (cube::Zone::Plant &plant : zone->m_plants) {
        plant.daylight =
            (float)grid.lookup(plant.position[0].toBlock(), plant.position[1].toBlock(), plant.position[2].toBlock());
        if (zones.cube(plant.position[0], plant.position[1], plant.position[2]).type == cube::BlockWater)
            plant.flags |= cube::PlantFlagUnderwater;
    }
    for (cube::Zone::Plant &plant : zone->m_lightPlants) {
        plant.daylight =
            (float)grid.lookup(plant.position[0].toBlock(), plant.position[1].toBlock(), plant.position[2].toBlock());
        if (zones.cube(plant.position[0], plant.position[1], plant.position[2]).type == cube::BlockWater)
            plant.flags |= cube::PlantFlagUnderwater;
    }

    // The lowest and the highest block that shows a face.
    int minZ = 0;
    int maxZ = 0;
    bool first = true;
    for (int bx = x0; bx < x1; bx++) {
        Sleep(0);
        for (int by = y0; by < y1; by++) {
            cube::Field *field = zones.field(bx, by);
            if (!field)
                continue;
            int low = field->m_baseZ;
            int top = (int)field->m_blocks.size() + field->m_baseZ;
            cube::Field *neighbour = zones.field(bx - 1, by);
            if (neighbour)
                low = std::min(neighbour->m_baseZ, low);
            neighbour = zones.field(bx + 1, by);
            if (neighbour)
                low = std::min(neighbour->m_baseZ, low);
            neighbour = zones.field(bx, by - 1);
            if (neighbour)
                low = std::min(neighbour->m_baseZ, low);
            neighbour = zones.field(bx, by + 1);
            if (neighbour) {
                low = std::min(neighbour->m_baseZ, low);
                maxZ = std::max((int)neighbour->m_blocks.size() + neighbour->m_baseZ, maxZ);
            }
            for (int z = low - 1; z < top; z++) {
                cube::Block block = zones.cube(bx, by, z);
                if (block.type == cube::BlockAir)
                    continue;
                if (isFaceOpen(block, zones.cube(bx - 1, by, z).type) ||
                    isFaceOpen(block, zones.cube(bx + 1, by, z).type) ||
                    isFaceOpen(block, zones.cube(bx, by - 1, z).type) ||
                    isFaceOpen(block, zones.cube(bx, by + 1, z).type) ||
                    isFaceOpen(block, zones.cube(bx, by, z - 1).type) ||
                    isFaceOpen(block, zones.cube(bx, by, z + 1).type)) {
                    if (first || z < minZ)
                        minZ = z;
                    if (first || z > maxZ)
                        maxZ = z;
                    first = false;
                }
            }
        }
    }
    if (maxZ < 1)
        maxZ = 1;

    int slices = (maxZ - minZ) / 250 + 1;
    vertices.resize(slices);
    solidIndices.resize(slices);
    waterIndices.resize(slices);
    indices2.resize(slices);
    lavaIndices.resize(slices);
    indices4.resize(slices);
    indices5.resize(slices);

    // A zone of open sea (every column water at height 0 under a fully lit block: computeLight keeps the light of an
    // open block in its first colour byte) gets one quad for its whole surface.
    bool openSea = false;
    if (minZ <= 0 && maxZ >= 0) {
        openSea = true;
        for (int bx = x0; bx < x1; bx++) {
            for (int by = y0; by < y1; by++) {
                if (zones.cube(bx, by, 0).type != cube::BlockWater || zones.cube(bx, by, 1).color[0] < 255) {
                    openSea = false;
                    break;
                }
            }
            if (!openSea)
                break;
        }
    }
    if (openSea) {
        int slice = -minZ / 250;
        int sliceBase = minZ + slice * 250;
        addQuad(waterIndices[slice], (int)vertices[slice].size());
        vertices[slice].push_back(
            Vertex(plasma::Vector<3, int>(0, 0, 1 - sliceBase), plasma::Vector<3, int>(0, 0, 1), waterColor()));
        vertices[slice].push_back(Vertex(plasma::Vector<3, int>(cube::BlocksPerZone, 0, 1 - sliceBase),
                                         plasma::Vector<3, int>(0, 0, 1), waterColor()));
        vertices[slice].push_back(
            Vertex(plasma::Vector<3, int>(cube::BlocksPerZone, cube::BlocksPerZone, 1 - sliceBase),
                   plasma::Vector<3, int>(0, 0, 1), waterColor()));
        vertices[slice].push_back(Vertex(plasma::Vector<3, int>(0, cube::BlocksPerZone, 1 - sliceBase),
                                         plasma::Vector<3, int>(0, 0, 1), waterColor()));
    }

    // The faces: for every block of every column, a quad towards each neighbour that leaves it open, each corner
    // shaded. A water block's faces all take the normal of a level surface, for the shading and for the vertex.
    typedef plasma::Vector<3, int> Normal;
    for (int bx = x0; bx < x1; bx++) {
        for (int by = y0; by < y1; by++) {
            Sleep(0);
            cube::Field *field = zones.field(bx, by);
            if (!field)
                continue;
            int localX = bx - x0;
            int localY = by - y0;
            int top = (int)field->m_blocks.size() + field->m_baseZ;
            int low = field->m_baseZ;
            cube::Field *neighbour = zones.field(bx - 1, by);
            if (neighbour)
                low = std::min(neighbour->m_baseZ, low);
            neighbour = zones.field(bx + 1, by);
            if (neighbour)
                low = std::min(neighbour->m_baseZ, low);
            neighbour = zones.field(bx, by - 1);
            if (neighbour)
                low = std::min(neighbour->m_baseZ, low);
            neighbour = zones.field(bx, by + 1);
            if (neighbour)
                low = std::min(neighbour->m_baseZ, low);

            // Where a neighbour's column begins lower, the blocks under this column show; they are not stored, so the
            // world makes them again from the column's terrain values.
            float height, heightWest, heightEast, heightSouth, heightNorth;
            float cliffs, temperature, humidity, sand, fade, path;
            float firelands, undead, enchantedForest, lava, poison;
            if (low < field->m_baseZ) {
                cliffs = field->m_mountainCliffs;
                temperature = field->m_temperature;
                humidity = field->m_humidity;
                firelands = field->m_firelands;
                undead = field->m_undead;
                enchantedForest = field->m_enchantedForest;
                sand = field->m_sandAmount;
                path = field->m_streetAmount;
                fade = 0.0f;
                lava = 0.0f;
                poison = 0.0f;
                height = field->m_height;
                heightWest = m_world->height(bx - 1, by, 0);
                heightEast = m_world->height(bx + 1, by, 0);
                heightSouth = m_world->height(bx, by - 1, 0);
                heightNorth = m_world->height(bx, by + 1, 0);
            }

            for (int z = low - 1; z < top; z++) {
                int slice = (z - minZ) / 250;
                int sliceBase = slice * 250 + minZ;
                cube::Block block = zones.cube(bx, by, z);
                if (block.type == cube::BlockAir)
                    continue;
                // A type 20 block shines (computeLight) and keeps its colour.
                bool plain = block.type == cube::BlockLightSource;
                plasma::Vector<4, float> color(block.color[0], block.color[1], block.color[2], 255.0f);
                color = color / plasma::Vector<4, float>(255.0f, 255.0f, 255.0f, 255.0f);
                if (z < field->m_baseZ) {
                    cube::Block surface(m_world->surfaceBlock(bx, by, z, height, heightWest, heightEast, heightSouth,
                                                              heightNorth, cliffs, temperature, humidity, sand, fade,
                                                              path, firelands, undead, enchantedForest, lava, poison));
                    plasma::Vector<4, float> surfaceColor(surface.color[0], surface.color[1], surface.color[2], 255.0f);
                    surfaceColor = surfaceColor / plasma::Vector<4, float>(255.0f, 255.0f, 255.0f, 255.0f);
                    color = surfaceColor;
                }

                // Selects the face's drawing pass; water and lava have separate lists and other types share a list.
                std::vector<unsigned int> *indices;
                if (block.type == cube::BlockWater) {
                    color = waterColor();
                    if (openSea && z == 0)
                        continue;
                    indices = &waterIndices[slice];
                } else if (block.type == cube::BlockSnow || block.type == cube::BlockSnowLeaves) {
                    indices = &solidIndices[slice];
                } else if (block.type == cube::BlockPoison) {
                    indices = &solidIndices[slice];
                } else if (block.type == cube::BlockLava) {
                    indices = &lavaIndices[slice];
                } else if (block.type == cube::BlockSand || block.type == cube::BlockPath) {
                    indices = &solidIndices[slice];
                } else {
                    indices = &solidIndices[slice];
                }

                if (isFaceOpen(block, zones.cube(bx - 1, by, z).type)) {
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX, localY, z - sliceBase),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(-1, 0, 0),
                               shade(zones, grid, bx, by, z, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(-1, 0, 0), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX, localY, z - sliceBase + 1),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(-1, 0, 0),
                               shade(zones, grid, bx, by, z + 1, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(-1, 0, 0), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX, localY + 1, z - sliceBase + 1),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(-1, 0, 0),
                               shade(zones, grid, bx, by + 1, z + 1, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(-1, 0, 0), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX, localY + 1, z - sliceBase),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(-1, 0, 0),
                               shade(zones, grid, bx, by + 1, z, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(-1, 0, 0), plain)));
                    addQuad(*indices, vertices[slice]);
                }
                if (isFaceOpen(block, zones.cube(bx + 1, by, z).type)) {
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX + 1, localY, z - sliceBase),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(1, 0, 0),
                               shade(zones, grid, bx + 1, by, z, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(1, 0, 0), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX + 1, localY + 1, z - sliceBase),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(1, 0, 0),
                               shade(zones, grid, bx + 1, by + 1, z, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(1, 0, 0), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX + 1, localY + 1, z - sliceBase + 1),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(1, 0, 0),
                               shade(zones, grid, bx + 1, by + 1, z + 1, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(1, 0, 0), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX + 1, localY, z - sliceBase + 1),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(1, 0, 0),
                               shade(zones, grid, bx + 1, by, z + 1, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(1, 0, 0), plain)));
                    addQuad(*indices, vertices[slice]);
                }
                if (isFaceOpen(block, zones.cube(bx, by - 1, z).type)) {
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX, localY, z - sliceBase),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, -1, 0),
                               shade(zones, grid, bx, by, z, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, -1, 0), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX + 1, localY, z - sliceBase),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, -1, 0),
                               shade(zones, grid, bx + 1, by, z, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, -1, 0), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX + 1, localY, z - sliceBase + 1),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, -1, 0),
                               shade(zones, grid, bx + 1, by, z + 1, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, -1, 0), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX, localY, z - sliceBase + 1),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, -1, 0),
                               shade(zones, grid, bx, by, z + 1, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, -1, 0), plain)));
                    addQuad(*indices, vertices[slice]);
                }
                if (isFaceOpen(block, zones.cube(bx, by + 1, z).type)) {
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX, localY + 1, z - sliceBase),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 1, 0),
                               shade(zones, grid, bx, by + 1, z, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 1, 0), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX, localY + 1, z - sliceBase + 1),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 1, 0),
                               shade(zones, grid, bx, by + 1, z + 1, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 1, 0), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX + 1, localY + 1, z - sliceBase + 1),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 1, 0),
                               shade(zones, grid, bx + 1, by + 1, z + 1, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 1, 0), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX + 1, localY + 1, z - sliceBase),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 1, 0),
                               shade(zones, grid, bx + 1, by + 1, z, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 1, 0), plain)));
                    addQuad(*indices, vertices[slice]);
                }
                if (isFaceOpen(block, zones.cube(bx, by, z - 1).type)) {
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX, localY, z - sliceBase),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 0, -1),
                               shade(zones, grid, bx, by, z, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 0, -1), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX, localY + 1, z - sliceBase),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 0, -1),
                               shade(zones, grid, bx, by + 1, z, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 0, -1), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX + 1, localY + 1, z - sliceBase),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 0, -1),
                               shade(zones, grid, bx + 1, by + 1, z, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 0, -1), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX + 1, localY, z - sliceBase),
                               block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 0, -1),
                               shade(zones, grid, bx + 1, by, z, color,
                                     block.type == cube::BlockWater ? Normal(0, 0, 1) : Normal(0, 0, -1), plain)));
                    addQuad(*indices, vertices[slice]);
                }
                if (isFaceOpen(block, zones.cube(bx, by, z + 1).type)) {
                    // A wet block under the open carries a film of water.
                    if (block.type == cube::BlockWet && zones.cube(bx, by, z + 1).type == cube::BlockAir) {
                        vertices[slice].push_back(
                            Vertex(plasma::Vector<3, int>(localX, localY, z - sliceBase + 1), Normal(0, 0, 1),
                                   shade(zones, grid, bx, by, z + 1, waterColor(), Normal(0, 0, 1), plain)));
                        vertices[slice].push_back(
                            Vertex(plasma::Vector<3, int>(localX + 1, localY, z - sliceBase + 1), Normal(0, 0, 1),
                                   shade(zones, grid, bx + 1, by, z + 1, waterColor(), Normal(0, 0, 1), plain)));
                        vertices[slice].push_back(
                            Vertex(plasma::Vector<3, int>(localX + 1, localY + 1, z - sliceBase + 1), Normal(0, 0, 1),
                                   shade(zones, grid, bx + 1, by + 1, z + 1, waterColor(), Normal(0, 0, 1), plain)));
                        vertices[slice].push_back(
                            Vertex(plasma::Vector<3, int>(localX, localY + 1, z - sliceBase + 1), Normal(0, 0, 1),
                                   shade(zones, grid, bx, by + 1, z + 1, waterColor(), Normal(0, 0, 1), plain)));
                        addQuad(waterIndices[slice], vertices[slice]);
                    }
                    vertices[slice].push_back(Vertex(plasma::Vector<3, int>(localX, localY, z - sliceBase + 1),
                                                     Normal(0, 0, 1),
                                                     shade(zones, grid, bx, by, z + 1, color, Normal(0, 0, 1), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX + 1, localY, z - sliceBase + 1), Normal(0, 0, 1),
                               shade(zones, grid, bx + 1, by, z + 1, color, Normal(0, 0, 1), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX + 1, localY + 1, z - sliceBase + 1), Normal(0, 0, 1),
                               shade(zones, grid, bx + 1, by + 1, z + 1, color, Normal(0, 0, 1), plain)));
                    vertices[slice].push_back(
                        Vertex(plasma::Vector<3, int>(localX, localY + 1, z - sliceBase + 1), Normal(0, 0, 1),
                               shade(zones, grid, bx, by + 1, z + 1, color, Normal(0, 0, 1), plain)));
                    addQuad(*indices, vertices[slice]);
                }

                // A column that ends below height 0 goes on with the block at 0, where the sea is.
                if (z == top - 1 && top < 1) {
                    z = -1;
                    top = 1;
                }
            }
        }
    }

    plasma::Vector<3, cube::Fixed> boundsMin =
        plasma::Vector<3, cube::Fixed>(cube::Fixed(x0), cube::Fixed(y0), cube::Fixed(minZ));
    plasma::Vector<3, cube::Fixed> boundsMax(cube::Fixed(x0 + cube::BlocksPerZone + 1),
                                             cube::Fixed(y0 + cube::BlocksPerZone + 1),
                                             std::max(cube::Fixed(0), cube::Fixed(maxZ + 1)));

    // The buffers of each slice; sixteen-bit indices where the slice's vertices can all be named with them.
    std::vector<VertexBuffer *> vertexBuffers(slices, nullptr);
    std::vector<IndexBuffer *> solidBuffers(slices, nullptr);
    std::vector<IndexBuffer *> waterBuffers(slices, nullptr);
    std::vector<IndexBuffer *> buffers2(slices, nullptr);
    std::vector<IndexBuffer *> lavaBuffers(slices, nullptr);
    std::vector<IndexBuffer *> buffers4(slices, nullptr);
    std::vector<IndexBuffer *> buffers5(slices, nullptr);
    for (int i = 0; i < slices; i++) {
        if (!vertices[i].empty())
            vertexBuffers[i] = m_world->m_spriteManager->graphics()->createVertexBuffer(vertices[i]);
        if (!solidIndices[i].empty()) {
            if (vertices[i].size() < 0x10000) {
                std::vector<unsigned short> narrow(solidIndices[i].begin(), solidIndices[i].end());
                solidBuffers[i] = m_world->m_spriteManager->graphics()->createIndexBuffer16(narrow);
            } else {
                solidBuffers[i] = m_world->m_spriteManager->graphics()->createIndexBuffer32(solidIndices[i]);
            }
        }
        if (!waterIndices[i].empty()) {
            if (vertices[i].size() < 0x10000) {
                std::vector<unsigned short> narrow(waterIndices[i].begin(), waterIndices[i].end());
                waterBuffers[i] = m_world->m_spriteManager->graphics()->createIndexBuffer16(narrow);
            } else {
                waterBuffers[i] = m_world->m_spriteManager->graphics()->createIndexBuffer32(waterIndices[i]);
            }
        }
        if (!indices2[i].empty())
            buffers2[i] = m_world->m_spriteManager->graphics()->createIndexBuffer32(indices2[i]);
        if (!lavaIndices[i].empty()) {
            if (vertices[i].size() < 0x10000) {
                std::vector<unsigned short> narrow(lavaIndices[i].begin(), lavaIndices[i].end());
                lavaBuffers[i] = m_world->m_spriteManager->graphics()->createIndexBuffer16(narrow);
            } else {
                lavaBuffers[i] = m_world->m_spriteManager->graphics()->createIndexBuffer32(lavaIndices[i]);
            }
        }
        if (!indices4[i].empty())
            buffers4[i] = m_world->m_spriteManager->graphics()->createIndexBuffer32(indices4[i]);
        if (!indices5[i].empty())
            buffers5[i] = m_world->m_spriteManager->graphics()->createIndexBuffer32(indices5[i]);
    }

    // The zone's chunk takes the new buffers under the mesh lock; the buffers of an earlier pass that the chunk thread
    // has not taken yet are deleted.
    Chunk &chunk = zone->m_chunk;
    std::list<ChunkBuffer *> oldBuffers(chunk.newChunkBuffers);
    m_world->enterZoneMeshes();
    chunk.center = (boundsMin + boundsMax) * cube::Fixed(0.5);
    chunk.radius = plasma::Vector<3, float>(chunk.center - boundsMax).length() * ChunkSphereRadiusExpansion;
    chunk.boundsMin = boundsMin;
    chunk.boundsMax = boundsMax;
    chunk.newChunkBuffers.clear();
    chunk.triangleCount = 0;
    int baseZ = minZ;
    for (int i = 0; i < slices; i++) {
        if (vertexBuffers[i]) {
            ChunkBuffer *buffer = new ChunkBuffer;
            buffer->vertexBuffer = vertexBuffers[i];
            buffer->indexBuffers[ChunkPassSolid] = solidBuffers[i];
            buffer->indexBuffers[ChunkPassWater] = waterBuffers[i];
            buffer->indexBuffers[ChunkPass_2] = buffers2[i];
            buffer->indexBuffers[ChunkPassLava] = lavaBuffers[i];
            buffer->indexBuffers[ChunkPass_4] = buffers4[i];
            buffer->indexBuffers[ChunkPass_5] = buffers5[i];
            buffer->vertexCount = (int)vertices[i].size();
            buffer->triangleCounts[ChunkPassSolid] = (int)(solidIndices[i].size() / 3);
            buffer->baseZ = baseZ;
            buffer->triangleCounts[ChunkPassWater] = (int)(waterIndices[i].size() / 3);
            buffer->triangleCounts[ChunkPass_2] = (int)(indices2[i].size() / 3);
            buffer->triangleCounts[ChunkPassLava] = (int)(lavaIndices[i].size() / 3);
            buffer->triangleCounts[ChunkPass_4] = (int)(indices4[i].size() / 3);
            buffer->triangleCounts[ChunkPass_5] = (int)(indices5[i].size() / 3);
            chunk.newChunkBuffers.push_back(buffer);
            chunk.triangleCount += buffer->triangleCounts[ChunkPassSolid] + buffer->triangleCounts[ChunkPassWater] * 2 +
                                   buffer->triangleCounts[ChunkPassLava];
        }
        baseZ += 250;
    }
    chunk.needsRemesh = false;
    m_world->leaveZoneMeshes();
    for (ChunkBuffer *buffer : oldBuffers)
        delete buffer;
}

}
