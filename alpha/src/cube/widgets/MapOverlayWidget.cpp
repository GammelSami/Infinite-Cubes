#include "cube/WorldDimensions.h"
// MapOverlayWidget draws labels over the world map.
#include "cube/DefaultBlocks.h"
#include "cube/widgets/MapOverlayWidget.h"
#include "cube/GameController.h"
#include "cube/Creature.h"
#include "cube/Speech.h"
#include "cube/StatCurves.h"
#include "cube/WorldMap.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/Font.h"
#include <set>
#include <sstream>
namespace cube {

MapOverlayWidget::MapOverlayWidget(plasma::Engine *engine, plasma::Node *node, GameController *game)
    : Widget(engine, node),
      m_game(game)
{}

// The world map's names over the last WorldMap::render (its view and projection): zoomed in
// (m_mapZoom > 2), the discovered zones within 32 zones of the map's center by their cells' names,
// white below the player's level - 2, red above + 2, green between; a teleporter's cell (kind 4)
// while the map can teleport (WorldMap::m_canTeleport) violet, pink when selected, cyan under the
// pointer, which makes it the map's hovered zone (WorldMap::m_hoveredZone, reset first). Then each
// region record covering a discovered zone, once, by its name at its centre: cyan when its level
// bonus is within 0.1 of the player's, red above, white below.
void MapOverlayWidget::draw()
{
    m_game->m_worldMap.enterZones();
    m_game->m_worldMap.m_hoveredZone = plasma::Vector<2, int>(-1, -1);
    int zoneX = (int)(m_game->m_player->m_entityData.position[0] + m_game->getMapOffset()[0]) / BlocksPerZone;
    int zoneY = (int)(m_game->m_player->m_entityData.position[1] + m_game->getMapOffset()[1]) / BlocksPerZone;

    if (m_game->m_mapZoom > 2.0f) {
        for (int x = zoneX - 32; x < zoneX + 32; x++) {
            for (int y = zoneY - 32; y < zoneY + 32; y++) {
                ZoneTile *tile = m_game->m_worldMap.getZoneTile(x, y);
                if (!tile || !tile->m_cell.kind || !(tile->m_flags & (1 << ZoneTileDiscovered)))
                    continue;
                // The zone's centre on the ground, as the map draws it.
                plasma::Vector<3, Fixed> offset(m_game->getMapOffset());
                Creature *player = m_game->m_player;
                float height = m_game->m_world.terrainHeight(x * BlocksPerZone + (BlocksPerZone / 2),
                                                             y * BlocksPerZone + (BlocksPerZone / 2), 0);
                plasma::Vector<3, float> point(plasma::Vector<3, Fixed>(Fixed(x * BlocksPerZone + (BlocksPerZone / 2)),
                                                                        Fixed(y * BlocksPerZone + (BlocksPerZone / 2)),
                                                                        Fixed(height)) -
                                               player->m_entityData.position - offset);
                point = m_game->m_worldMap.m_view * point;
                // Labels appear only in front of the camera and within the screen; NaN coordinates fail these
                // positive comparisons.
                if (point[2] > 0.0f) {
                    point = m_game->m_worldMap.m_projection * point;
                    if (point[0] >= -1.0f && point[0] <= 1.0f && point[1] >= -1.0f && point[1] <= 1.0f) {
                        plasma::Vector<3, float> pixel =
                            point * (plasma::Vector<3, float>((float)m_engine->viewportWidth(),
                                                              (float)-m_engine->viewportHeight(), 1.0f) *
                                     0.5f) +
                            plasma::Vector<3, float>((float)m_engine->viewportWidth(),
                                                     (float)m_engine->viewportHeight(), 0.0f) *
                                0.5f;

                        std::wstringstream text;
                        std::wstring name = m_game->m_world.m_speech.cellName(&m_game->m_world, &tile->m_cell,
                                                                              x * BlocksPerZone + (BlocksPerZone / 2),
                                                                              y * BlocksPerZone + (BlocksPerZone / 2));
                        plasma::Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
                        unsigned char kind = tile->m_cell.kind;
                        if (kind != Region::Cell::StructureQuarter && kind != Region::Cell::StructurePortal) {
                            if (tile->m_cell.level >= m_game->m_player->m_entityData.level - 2) {
                                // Label palette: white/green/red difficulty, violet/pink/cyan teleporter states.
                                if (tile->m_cell.level > m_game->m_player->m_entityData.level + 2)
                                    color = plasma::Vector<4, float>(1.0f, 0.2f, 0.2f, 1.0f);
                                else
                                    color = plasma::Vector<4, float>(0.2f, 1.0f, 0.2f, 1.0f);
                            }
                        }
                        if (kind == Region::Cell::StructurePortal && m_game->m_worldMap.m_canTeleport) {
                            color = plasma::Vector<4, float>(0.6f, 0.25f, 1.0f, 1.0f);
                            if (plasma::Vector<2, int>(x, y) == m_game->m_worldMap.m_selectedZone)
                                color = plasma::Vector<4, float>(1.0f, 0.25f, 1.0f, 1.0f);
                            plasma::Vector<2, float> pointer = m_engine->getPointer();
                            if (pointer[0] >= pixel[0] - 100.0f && pointer[0] < pixel[0] + 100.0f &&
                                pointer[1] >= pixel[1] - 20.0f && pointer[1] < pixel[1] + 10.0f) {
                                m_game->m_worldMap.m_hoveredZone = plasma::Vector<2, int>(x, y);
                                color = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
                            }
                        }
                        m_engine->getFontEngine()->renderText(
                            L"resource1.dat", name, 0.0f, 2.0f, pixel[0], pixel[1], 10.0f, 3.0f,
                            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER),
                            -1.0f, true);
                        m_engine->getFontEngine()->renderText(
                            L"resource1.dat", name, 0.0f, 2.0f, pixel[0], pixel[1], 10.0f, 0.0f, color.m_elements,
                            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER),
                            -1.0f, true);
                    }
                }
            }
        }
    }

    std::set<Region::Record *> labelled;
    for (int x = zoneX - 32; x < zoneX + 32; x++) {
        for (int y = zoneY - 32; y < zoneY + 32; y++) {
            ZoneTile *tile = m_game->m_worldMap.getZoneTile(x, y);
            if (!tile || !(tile->m_flags & (1 << ZoneTileDiscovered)))
                continue;
            Region::Record *record = m_game->m_worldMap.getRecord(x / ZonesPerSceneryRecord, y / ZonesPerSceneryRecord);
            if (!record || !record->kind || record->kind == Region::Record::SceneryPortal)
                continue;
            if (labelled.find(record) != labelled.end())
                continue;
            if (record->influence(Fixed(x * BlocksPerZone + (BlocksPerZone / 2)),
                                  Fixed(y * BlocksPerZone + (BlocksPerZone / 2))) > 0.0f) {
                labelled.insert(record);

                Creature *player = m_game->m_player;
                float height = m_game->m_world.terrainHeight((int)record->center[0], (int)record->center[1], 0);
                plasma::Vector<3, float> point(
                    (float)(record->center[0] - player->m_entityData.position[0] - Fixed(m_game->getMapOffset()[0])),
                    (float)(record->center[1] - player->m_entityData.position[1] - Fixed(m_game->getMapOffset()[1])),
                    (float)(Fixed(height) - player->m_entityData.position[2]));
                point = m_game->m_worldMap.m_view * point;
                if (point[2] > 0.0f) {
                    point = m_game->m_worldMap.m_projection * point;
                    if (point[0] >= -1.0f && point[0] <= 1.0f && point[1] >= -1.0f && point[1] <= 1.0f) {
                        plasma::Vector<3, float> pixel =
                            point * (plasma::Vector<3, float>((float)m_engine->viewportWidth(),
                                                              (float)-m_engine->viewportHeight(), 1.0f) *
                                     0.5f) +
                            plasma::Vector<3, float>((float)m_engine->viewportWidth(),
                                                     (float)m_engine->viewportHeight(), 0.0f) *
                                0.5f;

                        std::wstring name = m_game->m_world.m_speech.regionName(&m_game->m_world, record);
                        plasma::Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
                        if (record->kind != Region::Record::SceneryCity &&
                            record->kind != Region::Record::SceneryPortal &&
                            levelBonus((float)record->level) >
                                levelBonus((float)m_game->m_player->m_entityData.level) - 0.1f) {
                            if (levelBonus((float)record->level) <=
                                levelBonus((float)m_game->m_player->m_entityData.level) + 0.1f)
                                color = plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f);
                            else
                                color = plasma::Vector<4, float>(1.0f, 0.2f, 0.2f, 1.0f);
                        }
                        m_engine->getFontEngine()->renderText(
                            L"resource1.dat", name, 0.0f, 2.0f, pixel[0], pixel[1], 12.0f, 3.0f,
                            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER),
                            -1.0f, true);
                        m_engine->getFontEngine()->renderText(
                            L"resource1.dat", name, 0.0f, 2.0f, pixel[0], pixel[1], 12.0f, 0.0f, color.m_elements,
                            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER),
                            -1.0f, true);
                    }
                }
            }
        }
    }
    m_game->m_worldMap.leaveZones();
}
}
