// WorldMap caches two 1024-by-1024 grids: map regions with zone tiles, and coarse landscape tiles.
// It stores models in Save/map_<world name>.db and owns map marker sprites. GameController holds
// it by value.
#pragma once
#include <windows.h>
#include <list>
#include <vector>
#include "plasma/Matrix.h"
#include "plasma/Vector.h"
#include "cube/Database.h"
#include "cube/BytesIO.h"
#include "cube/Region.h"
#include "cube/Sprite.h"
#include "cube/World.h"

struct IDirect3DDevice9;

namespace cube {

class CubeShader;
class Sprite;
class World;

// Individual flags are bit indices; use (1 << flag) to form a mask.
enum ZoneTileFlag {
    ZoneTileDiscovered = 0,
    ZoneTileBuilt = 1
};

// The map tile of one zone.
class ZoneTile {
public:
    ZoneTile();
    virtual ~ZoneTile();
    // An undiscovered zone uses a green quest marker.
    bool isDiscovered() { return m_flags & (1 << ZoneTileDiscovered); }

    int m_baseZ;                                 // model base in units of 8 blocks
    Sprite *m_sprite;                            // the zone's model, owned
    bool unused0c;                               // false from the constructor; nothing reads it
    Region::Cell m_cell;                         // saved with the tile
    std::list<plasma::Vector<3, int>> m_markers; // block positions
    bool m_needsRebuild;
    int m_appearanceTime;  // 250 once the model is built
    unsigned char m_flags; // ZoneTileDiscovered and ZoneTileBuilt bits
};

// A coarse landscape tile covers one region of the world.
struct LandscapeTile {
    LandscapeTile()
        : m_discovered(false),
          m_sprite(0),
          m_needsRebuild(0)
    {}
    virtual ~LandscapeTile() { delete m_sprite; }

    // A copy of the world's record of the region, saved whole after the model; not set by the
    // constructor.
    World::RegionRecord m_info;
    // The zone of the model's first column.
    plasma::Vector<2, int> m_origin;
    bool m_discovered;
    Sprite *m_sprite;             // owned
    unsigned char m_needsRebuild; // loadLandscape rebuilds only when set
};

// 64 x 64 zone tiles and 8 x 8 region records. Allocated by the region loader, deleted by reset
// and unloadRegion.
struct MapRegion {
    ZoneTile m_tiles[64][64];
    Region::Record m_records[8][8];
};

class WorldMap {
public:
    WorldMap(IDirect3DDevice9 *device, World *world, CubeShader *shader);
    virtual ~WorldMap();

    void enterRegions();
    void leaveRegions();
    // Locks the zones.
    void enterZones();
    void leaveZones();
    // Saves and deletes every cached region and landscape tile, then closes the database (after saving
    // the discovered count).
    void clear();
    // Saves, then deletes the region (x, y) of the region grid.
    void unloadRegion(int x, int y);
    // Deletes the model of the zone tile at zone (x, y).
    void unloadZone(int x, int y);
    // Saves, then deletes the landscape tile (x, y).
    void unloadLandscape(int x, int y);
    // Drops the zone models more than 10 zones from zone (x, y) and the regions and landscape tiles
    // more than 8 regions from it.
    void cleanUp(int x, int y);
    // The landscape tile nearest to block column (x, y) (shifted by the world's landscape offset)
    // among the 3 x 3 cells around it; null unless all of them are loaded and have a model.
    LandscapeTile *nearestLandscape(int x, int y);
    Region::Record *getRecord(int x, int y);
    ZoneTile *getZoneTile(int x, int y);

    int getDiscovered() const { return m_discovered; }
    // Draws the map around center : the view (300 units away, zoomed, tilted by rotation[0] and turned
    // by rotation[2], degrees) and a 45 degree perspective centred on (centerX, centerY) of the width
    // x height window, clipped to a scissor rectangle. Below a zoom of 0.35 it draws the landscape
    // tiles within radius regions of the center, otherwise the zone tiles within radius zones (the
    // nearer neighbours when radius is 0) with their markers; full (the M key's map) adds placeholder
    // tiles for zones without a model, the region's missions and cities; then the creatures' map
    // signs. milliseconds advances m_clock.
    void render(const plasma::Vector<3, Fixed> &center, int centerX, int centerY, int width, int height,
                const plasma::Vector<3, float> &rotation, float zoom, int milliseconds, int radius, bool full);
    // Marks the zone tile at zone (x, y) discovered (counted once).
    void discover(int x, int y);
    // Opens the world's map database and reads the discovered count.
    void open();
    // Loads the region (x, y) from the database (when the world is named and the region is not loaded
    // yet), takes the world's cells and records where the world has them, and saves it back.
    void loadRegion(int x, int y);
    // Loads or rebuilds the landscape tile (x, y) (see WorldMap.cpp).
    void loadLandscape(int x, int y);
    void loadZone(int x, int y);
    void saveLandscape(int x, int y);
    void saveRegion(int x, int y);

    // The projection and the view of the last render, the view in blocks (the map's units are 8
    // blocks); markers and signs are drawn with them, and MapOverlayWidget places its labels with
    // them.
    plasma::Matrix<float> m_projection;
    plasma::Matrix<float> m_view;
    // Point 300 blocks from the player away from the world center, relative to the last map center,
    // with zero z.
    plasma::Vector<3, float> m_awayPoint;
    plasma::Vector<2, int> m_hoveredZone;  // (-1, -1)
    plasma::Vector<2, int> m_selectedZone; // (-1, -1)
    bool m_canTeleport;
    IDirect3DDevice9 *m_device;
    CubeShader *m_shader; // the constructor's third argument (GameController's shader)
    World *m_world;
    MapRegion *m_regions[1024][1024];       // owned, by region (x, y)
    LandscapeTile *m_landscape[1024][1024]; // owned
    Sprite *m_markerSprite;                 // a 4 x 4 x 4 frame
    Sprite *m_skullSprite;                  // "skull.cub"
    int m_clock;                            // milliseconds rendered (turns the mission signs)
    int m_discovered;                       // zone tiles discovered ("discovered")
    CRITICAL_SECTION m_regionLock;
    CRITICAL_SECTION m_zoneLock;
    Database m_database;
    int m_formatVersion;                 // saved first as the format version
    std::vector<Sprite *> m_tileSprites; // plains, village, forest, mountains, hills; owned
};

}
