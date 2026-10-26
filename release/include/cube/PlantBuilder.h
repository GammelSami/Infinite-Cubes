// PlantBuilder decorates a storey's room with wall units, ceiling fixtures, hanging plants, props and items chosen
// from kind lists. Zone::generate supplies the wall, trim and room colours and calls decorate for each furnished
// storey.
#pragma once
#include "cube/PlantBuilderIds.h"
#include "cube/StructureBuilder.h"
#include "plasma/Vector.h"

#include <vector>

namespace cube {

class Zone;

class PlantBuilder : public StructureBuilder {
public:
    // What the kinds in the four lists select (the cases of the choosing functions; names from the bodies).

    // For a storey at or above ground (storey >= 0) the lists follow World::zoneBuildingType of the cell (35..38 and
    // 45); for a cellar they follow the cell's structure cell and the dungeon type of its place (40..47).
    PlantBuilder(Zone &zone, int cellX, int cellY, int storey, const plasma::Vector<3, float> &wallColor,
                 const plasma::Vector<3, float> &trimColor, const plasma::Vector<3, float> &roomColor);

    // The ceiling fixtures at the builder's column (x, y) below the ceiling z; floor is the room's lowest level, which
    // only the skylight reads. A chain of 2..11 links; a hook on a chain; a hanging plant (a Zone::Plant of type 72 or
    // 73 by m_plantKinds); a ball of leaves with a type 77 prop, one in four with a light and a type 89 beam; a lit
    // opening of 5 x 5 blocks to the block above the ceiling with a type 89 beam down to the floor; a stalactite in
    // the wall's colours.
    void ceilingChain(int x, int y, int z, int floor);
    void ceilingHook(int x, int y, int z, int floor);
    void ceilingPlant(int x, int y, int z);
    void ceilingShrub(int x, int y, int z, int floor);
    void ceilingSkylight(int x, int y, int z, int floor);
    void ceilingStalactite(int x, int y, int z, int floor);
    // One of the fixtures by a random entry of m_ceilingKinds.
    void chooseCeilingFixture(int x, int y, int z, int floor);
    // The first empty or water level at or above z in the column, at most 100 blocks up; 0 when there is none.
    int clearLevelAbove(int x, int y, int z);
    void decorate(int x, int y, int z, int sizeX, int sizeY, int sizeZ);
    // The wall units, built by a part builder at (x, y) turned by rotation quarter turns, from z up to top, in the
    // room's, wall's and trim's colours: alcoves with an item (category 19) in one of three; stone supports of several
    // profiles; a furnace and a stove with a grate; a spout of lava; a trough; a tree growing out of the wall with
    // three crowns of leaves.
    void wallAlcoves(int x, int y, int z, int top, int rotation, bool mirrored);
    void wallBracket(int x, int y, int z, int top, int rotation, bool mirrored);
    void wallButtress(int x, int y, int z, int top, int rotation, bool mirrored);
    void wallColumn(int x, int y, int z, int top, int rotation, bool mirrored);
    void wallColumnWide(int x, int y, int z, int top, int rotation, bool mirrored);
    // A prop mounted on the wall behind (x, y) by a random entry of m_fixtureKinds (Zone::Plant types 85..88, 67, 61,
    // 62, 63 and 68..71), when the wall is solid there. top is not read.
    void wallFixture(int x, int y, int z, int top, int rotation);
    void wallFurnace(int x, int y, int z, int top, int rotation, bool mirrored);
    void wallLavaSpout(int x, int y, int z, int top, int rotation, bool mirrored);
    void wallPier(int x, int y, int z, int top, int rotation, bool mirrored);
    void wallStove(int x, int y, int z, int top, int rotation, bool mirrored);
    void wallTrough(int x, int y, int z, int top, int rotation, bool mirrored);
    // One of the wall units by a random entry of m_wallKinds (one in five from m_rareWallKinds when it has any), where
    // the wall behind is solid over five columns.
    void wallUnit(int x, int y, int z, int top, int rotation, bool mirrored);
    void wallsideTree(int x, int y, int z, int top, int rotation, bool mirrored);

    std::vector<int> m_wallKinds;     // WallKinds
    std::vector<int> m_ceilingKinds;  // CeilingKinds
    std::vector<int> m_rareWallKinds; // WallKinds; the constructor leaves it empty
    std::vector<int> m_fixtureKinds;  // wallFixture's kinds 0..5
    std::vector<int> m_plantKinds;    // ceilingPlant's kinds 0 and 1
    int m_storey;
    Material m_unusedMaterial;
    plasma::Vector<3, float> m_wallColor;
    plasma::Vector<3, float> m_trimColor;
    plasma::Vector<3, float> m_roomColor;
    // By the order of the colours above they might be the wall and room textures, but nothing reads them (setMaterial
    // gets the literal 0 for the wall).
    int m_unused1a4;   // Initially 1.
    int m_trimTexture; // Initially 1.
    int m_unused1ac;   // Initially 0.
};

}
