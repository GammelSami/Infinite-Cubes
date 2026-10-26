// WorldMap stores explored terrain at multiple zoom levels, generates its map models in the background and keeps the
// cursor, hovered marker and flight-selection state used by Game.
#pragma once
#include "cube/MissionIds.h"
#include <windows.h>
#include <array>
#include <list>
#include <map>
#include <vector>
#include "plasma/Matrix.h"
#include "plasma/Vector.h"
#include "cube/Fixed.h"
#include "cube/Item.h"
#include "cube/StructureCell.h"

namespace plasma {
class D3D11Engine;
}

namespace gfx {
class SpriteBatch;
}

namespace cube {

class World;

class Creature;
class Sprite;

// The side of a zoom level 0 map region in blocks, before the zoom factor.
static int g_mapRegionSize = 16;

struct MapPlace {
    unsigned char type; // Selects the map icon (2..15) or biome name.
    plasma::Vector<3, int> position;
};

// Site information used by render.
struct MapMission {
    MapMission()
        : kind(MissionKindNone),
          level(0)
    {}

    int type; // passed to Speech::GetPlaceName
    plasma::Vector<3, int> position;
    MissionKind kind;   // 0 none, 1..24 select the icon
    unsigned int level; // Rarity colour and star count.
    Item item;          // the reward shown by the preview widget when its category is not 0
};

struct MapCell {
    StructureCell structure;         // Generated place information stored with the marker.
    plasma::Vector<2, int> position; // the cell in blocks (cell * 16)
    int dungeonZLevel; // Dungeon height from World::dungeonZLevel; render divides it by zoom to place the marker vertically.
};

// One cell of the map at one zoom level, as WorldMap::region returns it.
struct WorldMapRegion {
    WorldMapRegion();
    ~WorldMapRegion();

    Sprite *sprite; // the region's terrain as a voxel model, owned
    int baseHeight;
    plasma::Vector<2, int> cacheTimes;
    std::list<MapMission> places;         // shops, shrines, characters
    std::list<MapMission> unusedMissions; // the region generator never fills it; nothing reads it
    std::list<MapMission> missions;
    std::list<MapMission> areas;        // the named biome areas (type: the biome)
    std::list<MapMission> flightPoints; // shown in flight master mode
    std::list<MapCell> sites;
    std::map<std::array<int, 2>, std::list<plasma::Vector<3, int>>> positionsByPlace;
    // Integer state value.
    int generationTime;
};

// Except for previewSprite, these sprite members are only initialised and deleted; nothing assigns a sprite to them or
// reads them.
struct WorldMapZoom {
    WorldMapZoom();

    Sprite *previewSprite; // Owned by the map and deleted by ~WorldMap.
    Sprite *unusedSprite;  // deleted by ~WorldMap like previewSprite; always 0
    std::map<std::array<int, 2>, Sprite *> unusedSprites;
    plasma::Vector<2, int> unused20; // (0, 0)
    plasma::Vector<2, int> unused28; // (0, 0)
    int unused30;                    // Initially -1.
    bool unused34;                   // false
    bool unused35;                   // false
    std::list<plasma::Vector<2, int>> unused38;
    std::list<plasma::Vector<2, int>> unused48;
    std::map<int, std::list<plasma::Vector<3, int>>> unused58;
};

class WorldMap {
public:
    WorldMap(World *world, plasma::D3D11Engine *engine);
    virtual ~WorldMap();

    void cleanUp();
    // Deletes every region (under the regions lock for the map itself).
    void clear();
    void enter();
    void leave();
    // Draws the map: sets the projection (a 45 degree perspective, mirrored in x, shifted so that the window pixel
    // (cursorX, cursorY) is at the centre of projection) and the view (300 units back, scaled by viewScaleFactor,
    // turned by angles[0] about x and angles[2] about z), then fills batch with the regions of the zoom level within
    // m_regionRadius of center (fading in over 200 ms), the sites of the hovered place, the flight points of the
    // hovered region, the creatures (sorted far to near, with a black outline, pulsing while hostile) and a dotted
    // line from the character to m_selectedFlightPoint or to its flight destination. The region under the engine's
    // pointer is ray cast for m_cursorPosition and m_hoveredRegion unless allowBlocking is set, which also halves the
    // radius. frameTime and controlKeyDown are not read.
    void render(gfx::SpriteBatch &batch, const plasma::Vector<3, Fixed> &center, int cursorX, int cursorY, int width,
                int height, const plasma::Vector<3, float> &angles, int zoom, float viewScaleFactor, int frameTime,
                bool controlKeyDown, bool passiveView);
    // Keeps a copy of a creature (its entity data and animation state timers) under its id, to show it on the map;
    // unmarkCreature drops it.
    void markCreature(Creature *creature);
    void unmarkCreature(Creature *creature);
    // Whether the creature ID is in m_creatures.
    bool isMarked(Creature *creature);
    bool update(const plasma::Vector<3, Fixed> &position, int level);

    // The region at (x, y) of the zoom level, or 0.
    WorldMapRegion *region(int x, int y, int level)
    {
        std::array<int, 3> key = {x, y, level};
        std::map<std::array<int, 3>, WorldMapRegion *>::iterator it = m_regions.find(key);
        if (it == m_regions.end())
            return nullptr;
        return it->second;
    }

    Creature *lookupCreature(long long id)
    {
        std::map<long long, Creature *>::iterator it = m_creatures.find(id);
        return it != m_creatures.end() ? it->second : nullptr;
    }

    World *m_world;
    plasma::D3D11Engine *m_engine;
    plasma::Matrix<float> m_projectionMatrix;
    plasma::Matrix<float> m_viewMatrix;
    // Opaque record storage.
    WorldMapZoom m_unusedZoom; // never read or filled
    plasma::Vector<3, Fixed> m_cursorPosition;
    plasma::Vector<2, int> m_hoveredRegion;
    plasma::Vector<2, int> m_selectedLand;
    std::map<long long, plasma::Vector<3, Fixed>> m_creaturePositions;
    std::map<long long, Creature *> m_creatures;
    long long m_markedCreatureId; // Initially zero.
    plasma::Vector<2, int> m_selectedFlightPoint;
    // Set when the hovered flight target is a creature's marker (in flight master mode) rather than a flight point
    // (MapOverlayWidget::draw stores !Marker::isFlightPoint): such a target needs no known flight point.
    bool m_selectedFlightTargetIsCreature;
    plasma::Vector<2, int> m_selectedMarker;
    bool m_flightmasterMode;
    int m_totalGenerationTime;
    // Opaque element storage.
    std::vector<WorldMapZoom> m_levels;
    std::map<std::array<int, 3>, WorldMapRegion *> m_regions;
    int m_frameCounter; // 0 after construction; counts the render calls and stamps the regions drawn
    int m_regionRadius;
    CRITICAL_SECTION m_regionsCriticalSection;
    CRITICAL_SECTION m_unusedLock; // Initialised by the constructor and deleted by ~WorldMap; never entered.
};

// A terrain colour faded towards a tint by height: the nearer z is to lowest (as a fraction of the way to highest),
// the more the colour moves to its own luminance times a tint that goes from blue (0, 40, 140) to orange (130, 60, 0)
// with (a - 0.8) * (1 - b) * 8.
plasma::Vector<3, float> heightTint(const plasma::Vector<3, float> &color, int regionSize, int z, int lowest,
                                    int highest, float temperature, float humidity, float river);
// value / 16, rounded down: the 16-block cell of a block coordinate. Called by the map's region generator and by
// Zone::generate.
int toCell(int value);

}
