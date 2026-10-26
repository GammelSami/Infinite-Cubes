#include <algorithm>
#include "cube/Noise.h"
#include "plasma/EditFilter.h"
#include "cube/widgets/MapOverlayWidget.h"
#include <math.h>
#include <algorithm>
#include <array>
#include <functional>
#include <set>
#include <sstream>
#include <vector>
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/World.h"
#include "cube/WorldLayout.h"
#include "cube/WorldMap.h"
#include "plasma/D3D11Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
#include "plasma/Transformation.h"
#include "cube/CreatureIds.h"
#include "cube/MissionIds.h"
#include "cube/WorldIds.h"

namespace cube {

namespace {

struct Marker {
    Marker()
        : scale(1.0f),
          position(0, 0, 0),
          icon(nullptr),
          overlayIcon(nullptr),
          isShrine(false),
          isFlightTarget(false),
          hasPreview(false),
          hasRegionPreview(false),
          isFlightPoint(false),
          item(nullptr)
    {}

    // The farthest marker sorts first, so nearer ones are drawn over it.
    bool operator>(const Marker &other) const { return screenPosition[2] > other.screenPosition[2]; }

    std::wstring text;                       // the label
    std::wstring note;                       // a second label above it ("NEW")
    float scale;                             // of the icons and the text
    plasma::Vector<4, float> color;          // of the label
    plasma::Vector<3, float> screenPosition; // window x and y, then the depth
    plasma::Vector<3, int> position;         // in blocks
    plasma::Node *icon;
    plasma::Node *overlayIcon; // drawn over the icon (a solved mission's mark)
    bool isShrine;             // hovered: WorldMap::m_selectedMarker takes its position
    bool isFlightTarget;       // hovered: WorldMap::m_selectedFlightPoint takes its position
    bool hasPreview;           // hovered: the preview widget shows its item or region
    bool hasRegionPreview;     // without an item, m_hoveredRegion takes its region
    bool
        isFlightPoint; // a flight point; clear for a creature (WorldMap::m_selectedFlightTargetIsCreature is set for it when hovered)
    const Item *item;
};

}

// Whether a place of the type is shown by its own discovery (the 16-block cell in PlayerData::discoveredMarkers) and
// not by its place's: shrines (12) and the characters (14, 15). Places and missions count as discovered on the map
// only up to this zoom (blocks per map unit).
const int DiscoveredPlaceMaxZoom = 64;
// The milliseconds a destination revealed by its lore (ten entries) is marked NEW; Game.cpp gives the other new
// missions 180 s.
const float LoreMissionTimer = 18000.0f;
// The NEW note blinks with this half-period in milliseconds.
const int NewNoteBlinkInterval = 600;
// A marker's hit box: this half-extent times the marker's scale.
const float MarkerHitExtent = 25.0f;
// A marker's label: the font size and the outline width, times the scale.
const float MarkerLabelSize = 12.0f;
const float MarkerLabelOutline = 3.0f;
// The NEW note stands this far above the label (times the interface scale).
const float MarkerNoteRise = 18.0f;
// The icon of a selected or hovered marker is drawn at this scale.
const float HoveredIconScale = 1.25f;
// The icons bob: the angular rate per caret-clock millisecond and the height.
const float IconBobRate = 0.005f;
const float IconBobHeight = 5.0f;

static inline bool usesIndividualDiscovery(int type)
{
    return type == PlotMarkerShrine || type == PlotMarkerCharacter || type == PlotMarkerTreasure;
}

MapOverlayWidget::MapOverlayWidget(Game *game, plasma::Node *node)
    : plasma::Widget(game->m_engine, node, L""),
      m_game(game),
      m_hoveredRegion(NoCoordinate, NoCoordinate)
{}

// A marker's size is a span in blocks over the zoom (4 for a place, 8 times that for a shrine, 4 times for a
// character; 128 for a mission, 512 for an area name, 64 for a flight point or creature) times the overlay scale (over
// 2.5 but for places), clamped to 0.35 (places) or 0.66 up to 1; a mission or flight point below 0.3 is not shown.
void MapOverlayWidget::draw()
{
    m_game->m_worldMap->m_selectedFlightPoint = plasma::Vector<2, int>(NoCoordinate, NoCoordinate);
    m_game->m_worldMap->m_selectedMarker = plasma::Vector<2, int>(NoCoordinate, NoCoordinate);
    m_game->m_worldMap->m_hoveredRegion = plasma::Vector<2, int>(NoCoordinate, NoCoordinate);

    // One map unit is `zoom` blocks; a region is 16 of them wide.
    int zoom = (int)(powf(2.0f, (float)m_game->m_mapZoom) * BlocksPerMapUnitAtLevelZero);
    if (zoom < 1)
        zoom = 1;
    std::vector<Marker> markers;
    int regionSize = g_mapRegionSize * zoom;
    int centerY = (int)m_game->m_mapPosition[1] / regionSize;
    int centerX = (int)m_game->m_mapPosition[0] / regionSize;
    int radius = m_game->m_worldMap->m_regionRadius;
    int minX = centerX - radius;
    int maxX = centerX + radius - 1;
    int minY = centerY - radius;
    int maxY = centerY + radius - 1;
    // The 16-block cells that hold a mission marker.
    std::set<std::array<int, 2>> missionCells;

    for (int regionX = minX; regionX <= maxX; regionX++) {
        for (int regionY = minY; regionY <= maxY; regionY++) {
            WorldMapRegion *region = m_game->m_worldMap->region(regionX, regionY, m_game->m_mapZoom);
            if (!region)
                continue;

            // Shops, shrines and characters.
            for (MapMission &place : region->places) {
                int placeX = floorDivide(place.position[0], BlocksPerPlace);
                int placeY = floorDivide(place.position[1], BlocksPerPlace);
                if (m_game->m_world->isLargePlace(placeX, placeY)) {
                    placeX = floorDivide(placeX, 2) * 2;
                    placeY = floorDivide(placeY, 2) * 2;
                }
                if (!usesIndividualDiscovery(place.type) &&
                    m_game->m_world->locationType(placeX, placeY, m_game->m_world->isLargePlace(placeX, placeY))) {
                    std::array<int, 2> place = {placeX, placeY};
                    if (m_game->m_world->m_localCreature->m_playerData->discoveredPlaces.find(place) ==
                        m_game->m_world->m_localCreature->m_playerData->discoveredPlaces.end())
                        continue;
                }
                double inverse = 1.0 / (double)zoom;
                Fixed factor(inverse);
                plasma::Vector<3, Fixed> offset = plasma::Vector<3, Fixed>(place.position) - m_game->m_mapPosition;
                plasma::Vector<3, Fixed> scaled = offset * factor;
                plasma::Vector<3, float> view = m_game->m_worldMap->m_viewMatrix * plasma::Vector<3, float>(scaled);
                if (!(view[2] > 0.0f))
                    continue;
                plasma::Vector<3, float> screen = m_game->m_worldMap->m_projectionMatrix * view;
                if (!(screen[0] >= -1.0f && screen[0] <= 1.0f && screen[1] >= -1.0f && screen[1] <= 1.0f))
                    continue;
                if (usesIndividualDiscovery(place.type)) {
                    int cellX = floorDivide(place.position[0], BlocksPerCell);
                    int cellY = floorDivide(place.position[1], BlocksPerCell);
                    std::array<int, 2> cell = {cellX, cellY};
                    if (m_game->m_world->m_localCreature->m_playerData->discoveredMarkers.find(cell) ==
                        m_game->m_world->m_localCreature->m_playerData->discoveredMarkers.end())
                        continue;
                }
                plasma::Vector<3, float> position =
                    screen * plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                                      (float)-m_engine->viewportHeight() * 0.5f, 0.5f);
                position += plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                                     (float)m_engine->viewportHeight() * 0.5f, 0.0f);
                std::wstringstream text;
                float size = 4.0f / (float)zoom * m_game->m_mapOverlayScale;
                if (place.type == PlotMarkerShrine)
                    size *= 8.0f;
                if (place.type == PlotMarkerCharacter)
                    size *= 4.0f;
                size = std::min(1.0f, size);
                size = std::max(0.35f, size);
                Marker marker;
                marker.text = L"";
                marker.color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
                marker.screenPosition = position;
                marker.isShrine = false;
                marker.hasPreview = false;
                marker.scale = size;
                marker.position = place.position;
                switch (place.type) {
                case PlotArmorShop:
                    marker.icon = m_game->m_interface.m_armorShopIcon;
                    break;
                case PlotWeaponShop:
                    marker.icon = m_game->m_interface.m_weaponShopIcon;
                    break;
                case PlotItemShop:
                    marker.icon = m_game->m_interface.m_itemShopIcon;
                    break;
                case PlotInn:
                    marker.icon = m_game->m_interface.m_innIcon;
                    break;
                case PlotIdentifiersShop:
                    marker.icon = m_game->m_interface.m_identifierIcon;
                    break;
                case PlotSmithy:
                    marker.icon = m_game->m_interface.m_smithyIcon;
                    break;
                case PlotCarpentersShop:
                    marker.icon = m_game->m_interface.m_carpentersShopIcon;
                    break;
                case PlotTailorsShop:
                    marker.icon = m_game->m_interface.m_tailorsShopIcon;
                    break;
                case PlotFlightMaster:
                    marker.icon = m_game->m_interface.m_flightMasterIcon;
                    break;
                case PlotGuildHall:
                    marker.icon = m_game->m_interface.m_guildHallIcon;
                    break;
                case PlotMarkerShrine:
                    marker.icon = m_game->m_interface.m_shrineIcon;
                    marker.isShrine = true;
                    break;
                case PlotMarkerCharacter:
                    marker.icon = m_game->m_interface.m_npcIcon;
                    marker.hasPreview = true;
                    marker.hasRegionPreview = true;
                    break;
                case PlotMarkerTreasure:
                    marker.icon = m_game->m_interface.m_treasureIcon;
                    break;
                default:
                    marker.icon = m_game->m_interface.m_homeIcon;
                    break;
                }
                markers.push_back(marker);
            }

            // Missions.
            for (MapMission &mission : region->missions) {
                double inverse = 1.0 / (double)zoom;
                Fixed factor(inverse);
                // Lifted above the ground by four map units.
                plasma::Vector<3, Fixed> offset =
                    plasma::Vector<3, Fixed>(Fixed(mission.position[0]), Fixed(mission.position[1]),
                                             Fixed(mission.position[2] + zoom * 4)) -
                    m_game->m_mapPosition;
                plasma::Vector<3, Fixed> scaled = offset * factor;
                plasma::Vector<3, float> view(scaled);
                view = m_game->m_worldMap->m_viewMatrix * view;
                if (!(view[2] > 0.0f))
                    continue;
                plasma::Vector<3, float> screen = m_game->m_worldMap->m_projectionMatrix * view;
                if (!(screen[0] >= -1.0f && screen[0] <= 1.0f && screen[1] >= -1.0f && screen[1] <= 1.0f))
                    continue;
                int placeX = floorDivide(mission.position[0], BlocksPerPlace);
                int placeY = floorDivide(mission.position[1], BlocksPerPlace);
                if (m_game->m_world->isLargePlace(placeX, placeY)) {
                    placeX = floorDivide(placeX, 2) * 2;
                    placeY = floorDivide(placeY, 2) * 2;
                }
                bool discovered;
                if (zoom <= DiscoveredPlaceMaxZoom) {
                    std::array<int, 2> place = {placeX, placeY};
                    discovered = m_game->m_world->m_localCreature->m_playerData->discoveredPlaces.find(place) !=
                                 m_game->m_world->m_localCreature->m_playerData->discoveredPlaces.end();
                } else {
                    discovered = false;
                }
                bool known = m_game->m_world->m_localCreature->knowsMission(placeX, placeY);
                if (mission.kind == MissionKindArtifactReward && !known) {
                    // A destination becomes known once ten entries exist under its key.
                    unsigned int key = mission.item.formulaCategory;
                    known = m_game->m_world->m_localCreature->m_playerData->loreByRealm[key].size() >= 10;
                    if (known) {
                        // Each key is its own temporary.
                        m_game->m_world->m_localCreature->m_playerData->knownMissions.insert(
                            std::array<int, 2> {placeX, placeY});
                        m_game->m_world->m_localCreature->m_playerData
                            ->missionTimers[std::array<int, 2> {placeX, placeY}] = LoreMissionTimer;
                    }
                }
                if (!discovered && !known)
                    continue;
                plasma::Vector<3, float> position =
                    screen * plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                                      (float)-m_engine->viewportHeight() * 0.5f, 0.5f);
                position += plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                                     (float)m_engine->viewportHeight() * 0.5f, 0.0f);
                int type = mission.type;
                plasma::Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
                if (known)
                    color = rarityColor(mission.level);
                std::wstringstream text;
                float size = 128.0f / (float)zoom * m_game->m_mapOverlayScale / 2.5f;
                if (0.3f > size)
                    continue;
                size = std::max(0.66f, size);
                size = std::min(1.0f, size);
                if (discovered) {
                    text << m_game->m_speech.findPlaceName(placeX, placeY, type,
                                                           m_game->m_world->isLargePlace(placeX, placeY));
                    if (known && m_game->m_options.showRarity) {
                        text << L"\n";
                        // One star (the font's glyph for U+00B1) per level, from 0.
                        for (unsigned int i = 0; i <= mission.level; i++)
                            text << L"\xb1";
                    }
                }
                Marker marker;
                marker.item = mission.item.category ? &mission.item : nullptr;
                std::array<int, 2> place = {placeX, placeY};
                if (m_game->m_world->m_localCreature->m_playerData->missionTimers.find(place) !=
                        m_game->m_world->m_localCreature->m_playerData->missionTimers.end() &&
                    m_engine->getCaretClock() / NewNoteBlinkInterval % 2 &&
                    !m_game->m_world->m_localCreature->hasCompletedMission(placeX, placeY))
                    marker.note = L"NEW";
                marker.scale = size;
                marker.text = text.str();
                marker.color = color;
                marker.screenPosition = position;
                marker.isShrine = false;
                marker.hasPreview = false;
                plasma::Node *icon = nullptr;
                if (mission.kind && known) {
                    switch (mission.kind) {
                    case MissionKindMonster:
                    case MissionKindAborigines:
                        icon = m_game->m_interface.m_monsterIcon;
                        break;
                    case MissionKindMagicBarriers:
                        icon = m_game->m_interface.m_crystalIcon;
                        break;
                    case MissionKindFlameOfPower:
                        icon = m_game->m_interface.m_brazierIcon;
                        break;
                    case MissionKindManaPumps:
                        icon = m_game->m_interface.m_manaPumpIcon;
                        break;
                    case MissionKindHangGliderPlacement:
                        icon = m_game->m_interface.m_hangGliderIcon;
                        break;
                    case MissionKindBoatPlacement:
                        icon = m_game->m_interface.m_boatIcon;
                        break;
                    case MissionKindClimbingSpikesPlacement:
                        icon = m_game->m_interface.m_climbingSpikesIcon;
                        break;
                    case MissionKindReinsPlacement:
                        icon = m_game->m_interface.m_reinsIcon;
                        break;
                    case MissionKindSupplierGnome:
                        icon = m_game->m_interface.m_gnomeIcon;
                        break;
                    case MissionKindMagicCraftingBookReward:
                        icon = m_game->m_interface.m_crafting1Icon;
                        break;
                    case MissionKindRareCraftingBookReward:
                        icon = m_game->m_interface.m_crafting2Icon;
                        break;
                    case MissionKindEpicCraftingBookReward:
                        icon = m_game->m_interface.m_crafting3Icon;
                        break;
                    case MissionKindLegendaryCraftingBookReward:
                        icon = m_game->m_interface.m_crafting4Icon;
                        break;
                    case MissionKindArtifactReward:
                        icon = m_game->m_interface.m_destinationIcon;
                        marker.hasPreview = known;
                        break;
                    case MissionKindExchangePartner:
                        icon = m_game->m_interface.m_flowerIcon;
                        break;
                    case MissionKindKey:
                        icon = m_game->m_interface.m_keyIcon;
                        break;
                    case MissionKindTreasureSpiritPlacement:
                        icon = m_game->m_interface.m_spiritIcon;
                        break;
                    case MissionKindBellPlacement:
                        icon = m_game->m_interface.m_bellIcon;
                        break;
                    case MissionKindHarpPlacement:
                        icon = m_game->m_interface.m_harpIcon;
                        break;
                    case MissionKindWhistlePlacement:
                        icon = m_game->m_interface.m_whistleIcon;
                        break;
                    case MissionKindWitchCurse:
                        icon = m_game->m_interface.m_witchIcon;
                        break;
                    case MissionKindDemonPortal:
                        icon = m_game->m_interface.m_demonPortalIcon;
                        break;
                    default:
                        icon = m_game->m_interface.m_enemyIcon;
                        break;
                    }
                }
                marker.icon = icon;
                if (mission.kind && m_game->m_world->m_localCreature->hasCompletedMission(placeX, placeY))
                    marker.overlayIcon = m_game->m_interface.m_solvedIcon;
                marker.position = mission.position;
                int cellX = floorDivide(mission.position[0], BlocksPerCell);
                int cellY = floorDivide(mission.position[1], BlocksPerCell);
                missionCells.insert(std::array<int, 2> {cellX, cellY});
                markers.push_back(marker);
            }

            // The names of the discovered areas.
            for (MapMission &place : region->areas) {
                double inverse = 1.0 / (double)zoom;
                Fixed factor(inverse);
                plasma::Vector<3, Fixed> offset = plasma::Vector<3, Fixed>(place.position) - m_game->m_mapPosition;
                plasma::Vector<3, Fixed> scaled = offset * factor;
                plasma::Vector<3, float> view(scaled);
                view = m_game->m_worldMap->m_viewMatrix * view;
                if (!(view[2] > 0.0f))
                    continue;
                plasma::Vector<3, float> screen = m_game->m_worldMap->m_projectionMatrix * view;
                if (!(screen[0] >= -1.0f && screen[0] <= 1.0f && screen[1] >= -1.0f && screen[1] <= 1.0f))
                    continue;
                int areaX = floorDivide(place.position[0], BlocksPerLand);
                int areaY = floorDivide(place.position[1], BlocksPerLand);
                std::array<int, 2> area = {areaX, areaY};
                if (m_game->m_world->m_localCreature->m_playerData->discoveredAreas.find(area) ==
                    m_game->m_world->m_localCreature->m_playerData->discoveredAreas.end())
                    continue;
                plasma::Vector<3, float> position =
                    screen * plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                                      (float)-m_engine->viewportHeight() * 0.5f, 0.5f);
                position += plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                                     (float)m_engine->viewportHeight() * 0.5f, 0.0f);
                int biome = place.type;
                std::wstringstream text;
                plasma::Vector<4, float> color = m_game->m_speech.getLandColor(biome);
                float size = 512.0f / (float)zoom * m_game->m_mapOverlayScale / 2.5f;
                size = std::max(0.66f, size);
                size = std::min(1.0f, size);
                // An integer hash of the area seeds its generated name.
                int n = areaX + 57 * areaY;
                n = (n << 13) ^ n;
                int seed = 100000 - ((n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff) / 21474;
                text << m_game->m_speech.getText(m_game->m_speech.getLandName(biome), L"named",
                                                 randomName(seed, RaceNone));
                Marker marker;
                marker.scale = size;
                marker.text = text.str();
                marker.color = color;
                marker.screenPosition = position;
                marker.isShrine = false;
                marker.hasPreview = false;
                marker.position = place.position;
                marker.icon = nullptr;
                markers.push_back(marker);
            }

            // The flight points.
            if (m_game->m_worldMap->m_flightmasterMode) {
                for (MapMission &place : region->flightPoints) {
                    double inverse = 1.0 / (double)zoom;
                    Fixed factor(inverse);
                    plasma::Vector<3, Fixed> offset = plasma::Vector<3, Fixed>(place.position) - m_game->m_mapPosition;
                    plasma::Vector<3, Fixed> scaled = offset * factor;
                    plasma::Vector<3, float> view(scaled);
                    view = m_game->m_worldMap->m_viewMatrix * view;
                    if (!(view[2] > 0.0f))
                        continue;
                    plasma::Vector<3, float> screen = m_game->m_worldMap->m_projectionMatrix * view;
                    if (!(screen[0] >= -1.0f && screen[0] <= 1.0f && screen[1] >= -1.0f && screen[1] <= 1.0f))
                        continue;
                    plasma::Vector<3, float> position =
                        screen * plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                                          (float)-m_engine->viewportHeight() * 0.5f, 0.5f);
                    position += plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                                         (float)m_engine->viewportHeight() * 0.5f, 0.0f);
                    std::wstringstream text;
                    float size = 64.0f / (float)zoom * m_game->m_mapOverlayScale / 2.5f;
                    if (0.3f > size)
                        continue;
                    size = std::max(0.66f, size);
                    size = std::min(1.0f, size);
                    plasma::Vector<2, int> flightPoint(place.position[0], place.position[1]);
                    Marker marker;
                    marker.scale = size;
                    marker.text = text.str();
                    marker.color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
                    marker.position = place.position;
                    marker.screenPosition = position;
                    marker.isFlightTarget = true;
                    marker.hasPreview = true;
                    marker.isFlightPoint = true;
                    if (m_game->m_world->m_localCreature->knowsFlightPoint(flightPoint))
                        marker.icon = m_game->m_interface.m_flightPointIcon;
                    else
                        marker.icon = m_game->m_interface.m_crossIcon;
                    markers.push_back(marker);
                }
            }
        }
    }

    // The creatures the map tracks.
    for (std::pair<const long long, plasma::Vector<3, Fixed>> &entry : m_game->m_worldMap->m_creaturePositions) {
        double inverse = 1.0 / (double)zoom;
        Fixed factor(inverse);
        plasma::Vector<3, Fixed> offset = entry.second - m_game->m_mapPosition;
        plasma::Vector<3, Fixed> scaled = offset * factor;
        plasma::Vector<3, float> view(scaled);
        view = m_game->m_worldMap->m_viewMatrix * view;
        Creature *creature = m_game->m_world->creatureByID(entry.first);
        if (!creature || creature->m_entityData.currentAbility == AbilityMenuView)
            continue;
        if (!(view[2] > 0.0f))
            continue;
        plasma::Vector<3, float> screen = m_game->m_worldMap->m_projectionMatrix * view;
        if (!(screen[0] >= -1.0f && screen[0] <= 1.0f && screen[1] >= -1.0f && screen[1] <= 1.0f))
            continue;
        plasma::Vector<3, float> position =
            screen * plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                              (float)-m_engine->viewportHeight() * 0.5f, 0.5f);
        position += plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                             (float)m_engine->viewportHeight() * 0.5f, 0.0f);
        std::wstringstream text;
        float size = 64.0f / (float)zoom * m_game->m_mapOverlayScale / 2.5f;
        size = std::max(0.66f, size);
        size = std::min(1.0f, size);
        text << std::endl << WidenString(std::string(creature->m_entityData.name));
        Marker marker;
        marker.scale = size;
        marker.text = text.str();
        marker.color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        marker.position = plasma::Vector<3, int>(creature->m_entityData.position);
        if (creature->m_entityData.currentAbility == AbilityEagleFlight) {
            plasma::Vector<3, Fixed> destination(creature->m_entityData.destination[0],
                                                 creature->m_entityData.destination[1],
                                                 m_game->m_world->height(creature->m_entityData.destination[0],
                                                                         creature->m_entityData.destination[1], false));
            marker.position = plasma::Vector<3, int>(destination);
        }
        marker.screenPosition = position;
        marker.hasPreview = true;
        marker.isFlightPoint = false;
        if (m_game->m_worldMap->m_flightmasterMode) {
            marker.isFlightTarget = true;
            marker.icon = m_game->m_interface.m_flightPointIcon;
        }
        markers.push_back(marker);
    }

    // The treasures the character knows, by region.
    int minBlockX = minX * regionSize;
    int minRegionX = minBlockX / BlocksPerRegion;
    int maxRegionX = maxX * regionSize / BlocksPerRegion;
    int minBlockY = minY * regionSize;
    int minRegionY = minBlockY / BlocksPerRegion;
    int maxRegionY = maxY * regionSize / BlocksPerRegion;
    for (int regionX = minRegionX; regionX <= maxRegionX; regionX++) {
        for (int regionY = minRegionY; regionY <= maxRegionY; regionY++) {
            std::array<int, 2> regionKey = {regionX, regionY};
            Creature *creature = m_game->m_world->m_localCreature;
            std::map<std::array<int, 2>, std::list<MapPlace>>::iterator found =
                creature->m_playerData->treasures.find(regionKey);
            if (found == creature->m_playerData->treasures.end())
                continue;
            for (MapPlace &place : found->second) {
                if (place.position[0] < minBlockX || place.position[1] < minBlockY ||
                    place.position[0] >= (maxX + 1) * regionSize || place.position[1] >= (maxY + 1) * regionSize)
                    continue;
                double inverse = 1.0 / (double)zoom;
                Fixed factor(inverse);
                plasma::Vector<3, Fixed> offset = plasma::Vector<3, Fixed>(place.position) - m_game->m_mapPosition;
                plasma::Vector<3, Fixed> scaled = offset * factor;
                plasma::Vector<3, float> view(scaled);
                view = m_game->m_worldMap->m_viewMatrix * view;
                if (!(view[2] > 0.0f))
                    continue;
                plasma::Vector<3, float> screen = m_game->m_worldMap->m_projectionMatrix * view;
                if (!(screen[0] >= -1.0f && screen[0] <= 1.0f && screen[1] >= -1.0f && screen[1] <= 1.0f))
                    continue;
                plasma::Vector<3, float> position =
                    screen * plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                                      (float)-m_engine->viewportHeight() * 0.5f, 0.5f);
                position += plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                                     (float)m_engine->viewportHeight() * 0.5f, 0.0f);
                std::wstringstream text;
                float size = 64.0f / (float)zoom * m_game->m_mapOverlayScale / 2.5f;
                if (0.3f > size)
                    continue;
                size = std::max(0.66f, size);
                size = std::min(1.0f, size);
                Marker marker;
                marker.scale = size;
                marker.text = text.str();
                marker.color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
                marker.screenPosition = position;
                marker.isShrine = false;
                marker.hasPreview = true;
                marker.icon = m_game->m_interface.m_starIcon;
                marker.position = place.position;
                markers.push_back(marker);
            }
        }
    }

    std::sort(markers.begin(), markers.end(), std::greater<Marker>());

    plasma::ScalableFont *font = m_engine->getFontEngine()->getFont(L"resource1.dat");
    plasma::Matrix<float> matrix;
    m_node->getMatrix(matrix);
    float nodeScale = matrix[0];

    // The marker under the pointer: the nearest one whose icon or label the pointer is over.
    m_hoveredItem = Item();
    m_hoveredRegion = plasma::Vector<2, int>(0x7fffffff, 0x7fffffff);
    plasma::Vector<2, int> hovered(0x7fffffff, 0x7fffffff);
    float nearestSquaredPointerDistance = -1.0f;
    if (!m_game->m_pointerMoved && m_game->m_interface.m_mapOpen) {
        for (Marker &marker : markers) {
            if (m_game->m_interface.m_choosingShrine && !marker.isShrine)
                continue;
            float markerY = marker.screenPosition[1];
            float markerX = marker.screenPosition[0];
            plasma::Vector<2, float> pointer;
            pointer = getPointer();
            float x = pointer[0] - markerX;
            float y = pointer[1] - markerY;
            float squaredPointerDistance = x * x + y * y;
            float extent = nodeScale * MarkerHitExtent;
            float halfWidth = extent * marker.scale;
            if (!marker.text.empty()) {
                plasma::Vector<2, float> minimum;
                plasma::Vector<2, float> maximum;
                font->measure(marker.text, matrix, (float)(int)(marker.scale * MarkerLabelSize * nodeScale),
                              (float)(int)(marker.scale * MarkerLabelOutline * nodeScale), 0.0f, 0.0f, 0.0f, minimum,
                              maximum, 0, true, -1, false);
                halfWidth = (std::max)(halfWidth, (maximum[0] - minimum[0]) * 0.5f);
            }
            if (!(halfWidth > (float)fabs(x)) || !(extent * marker.scale > (float)fabs(y)))
                continue;
            if (!(0.0f > nearestSquaredPointerDistance) && !(nearestSquaredPointerDistance > squaredPointerDistance))
                continue;
            nearestSquaredPointerDistance = squaredPointerDistance;
            if (marker.isShrine)
                m_game->m_worldMap->m_selectedMarker = plasma::Vector<2, int>(marker.position[0], marker.position[1]);
            if (marker.isFlightTarget) {
                m_game->m_worldMap->m_selectedFlightPoint =
                    plasma::Vector<2, int>(marker.position[0], marker.position[1]);
                m_game->m_worldMap->m_selectedFlightTargetIsCreature = !marker.isFlightPoint;
            }
            if (marker.hasPreview) {
                if (marker.item && marker.item->category)
                    m_hoveredItem = *marker.item;
                else if (marker.hasRegionPreview)
                    m_hoveredRegion = plasma::Vector<2, int>(floorDivide(marker.position[0], BlocksPerCell),
                                                             floorDivide(marker.position[1], BlocksPerCell));
                hovered = plasma::Vector<2, int>(marker.position[0], marker.position[1]);
            }
            WorldMap *map = m_game->m_worldMap;
            map->m_cursorPosition[0] = Fixed(marker.position[0]);
            map->m_cursorPosition[1] = Fixed(marker.position[1]);
            map->m_cursorPosition[2] = Fixed(marker.position[2]);
            m_game->m_worldMap->m_hoveredRegion = m_game->m_world->findRegion(marker.position[0], marker.position[1]);
        }
    }

    for (Marker &marker : markers) {
        float scale = marker.scale;
        plasma::Vector<4, float> color = marker.color;
        if (m_game->m_interface.m_choosingShrine && marker.isShrine) {
            // Choosing a shrine: the shrines pulse between violet and magenta (red 0.5..1), the chosen one is larger.
            scale = 1.0f;
            color =
                plasma::Vector<4, float>(std::cos(m_game->m_world->m_clock * 0.01f) * 0.25f + 0.75f, 0.25f, 1.0f, 1.0f);
            if (plasma::Vector<2, int>(marker.position[0], marker.position[1]) == m_game->m_worldMap->m_selectedMarker)
                scale = 1.2f;
        }
        int y = (int)marker.screenPosition[1];
        if (marker.icon) {
            marker.icon->m_transformation->m_deformation.value().identity();
            float iconScale = marker.scale;
            if (plasma::Vector<2, int>(marker.position[0], marker.position[1]) ==
                    m_game->m_worldMap->m_selectedFlightPoint ||
                plasma::Vector<2, int>(marker.position[0], marker.position[1]) ==
                    m_game->m_worldMap->m_selectedMarker ||
                plasma::Vector<2, int>(marker.position[0], marker.position[1]) == hovered)
                iconScale = HoveredIconScale;
            plasma::Vector<2, float> iconPosition(marker.screenPosition[0] / m_game->m_interface.m_scale,
                                                  marker.screenPosition[1] / m_game->m_interface.m_scale - 10.0f);
            // The icon stands 15 units above the marker, the label 10 below it. The lookup's result is not used.
            int cellX = floorDivide(marker.position[0], BlocksPerCell);
            int cellY = floorDivide(marker.position[1], BlocksPerCell);
            if (missionCells.find(std::array<int, 2> {cellX, cellY}) != missionCells.end()) {
            }
            iconPosition[1] -= 5.0f;
            // A marker without a solved mark bobs up and down.
            if (!marker.isShrine && !marker.overlayIcon)
                iconPosition[1] += std::cos((float)m_engine->getCaretClock() * IconBobRate) * IconBobHeight;
            marker.icon->m_transformation->m_deformation.value().identity();
            marker.icon->m_transformation->m_deformation.value().scale(iconScale, iconScale);
            marker.icon->m_transformation->m_translation.value() =
                iconPosition - marker.icon->m_transformation->m_pivot.value();
            marker.icon->m_transformation->attributesChanged(true);
            marker.icon->draw(0);
            y = (int)(m_game->m_interface.m_scale * 10.0f + (float)y);
        }
        if (marker.overlayIcon) {
            marker.overlayIcon->m_transformation->m_deformation.value().identity();
            float iconScale = marker.scale;
            plasma::Vector<2, float> iconPosition(marker.screenPosition[0] / m_game->m_interface.m_scale,
                                                  marker.screenPosition[1] / m_game->m_interface.m_scale - 10.0f);
            iconPosition[1] -= 5.0f;
            iconPosition[1] += std::cos((float)m_engine->getCaretClock() * IconBobRate) * IconBobHeight;
            marker.overlayIcon->m_transformation->m_deformation.value().identity();
            marker.overlayIcon->m_transformation->m_deformation.value().scale(iconScale, iconScale);
            marker.overlayIcon->m_transformation->m_translation.value() =
                iconPosition - marker.overlayIcon->m_transformation->m_pivot.value();
            marker.overlayIcon->m_transformation->attributesChanged(true);
            marker.overlayIcon->draw(0);
        }

        // The label: a dark outline (30%) of the marker's colour, then the fill. The note: white on an orange outline,
        // then a yellow fill.
        m_engine->getFontEngine()->renderText(
            L"resource1.dat", marker.text, 0.0f, 3.0f, marker.screenPosition[0], (float)y,
            (float)(int)(scale * MarkerLabelSize), (float)(int)(scale * MarkerLabelOutline),
            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(marker.color[0] * 0.3f, marker.color[1] * 0.3f, marker.color[2] * 0.3f, 1.0f)
                .m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, -1.0f, true);
        m_engine->getFontEngine()->renderText(L"resource1.dat", marker.text, 0.0f, 3.0f, marker.screenPosition[0],
                                              (float)y, (float)(int)(scale * MarkerLabelSize), 0.0f, color.m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, -1.0f,
                                              true);
        if (!marker.note.empty()) {
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", marker.note, 0.0f, 3.0f, marker.screenPosition[0],
                (float)y - m_game->m_interface.m_scale * MarkerNoteRise, (float)(int)(scale * MarkerLabelSize),
                (float)(int)(scale * MarkerLabelOutline), plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.6f, 0.25f, 0.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, -1.0f, true);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", marker.note, 0.0f, 3.0f, marker.screenPosition[0],
                (float)y - m_game->m_interface.m_scale * MarkerNoteRise, (float)(int)(scale * MarkerLabelSize), 0.0f,
                plasma::Vector<4, float>(1.0f, 0.8f, 0.1f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 1, -1.0f, true);
        }
    }
}

}
