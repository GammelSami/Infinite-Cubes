// StructureBuilder writes a structure into one zone using an integer placement matrix. Shapes use local coordinates
// and are placed with a quarter turn and optional mirroring. A child builder inherits its parent's zone and material,
// allowing parts of a structure to use their own placement.
#pragma once
#include "cube/StructureBuilderIds.h"
#include "cube/Block.h"
#include "cube/Zone.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

namespace cube {

class ServerUpdates;

class StructureBuilder {
public:
    // Material for generated blocks: up to three textured colour layers stacked by height.
    struct Material {
        Material()
            : color0(255.0f, 255.0f, 255.0f),
              color1(255.0f, 255.0f, 255.0f),
              color2(0.0f, 0.0f, 0.0f),
              texture0(0),
              texture1(0),
              texture2(0),
              level0(0.0f),
              level1(0.0f),
              level2(0.0f),
              seed(0),
              relative(false)
        {}

        plasma::Vector<3, float> color0;
        plasma::Vector<3, float> color1;
        plasma::Vector<3, float> color2;
        int texture0; // a StructureTexture each
        int texture1;
        int texture2;
        float level0; // heights in the builder's coordinates
        float level1;
        float level2;
        int seed;      // varies the wood textures (texturedColor's last argument)
        bool relative; // the levels are heights above the terrain
    };

    StructureBuilder(Zone &zone, int x, int y, int z, int sizeX, int sizeY, int rotation, bool mirrored,
                     ServerUpdates *updates);
    StructureBuilder(Zone &zone, ServerUpdates *updates);
    // It takes parent's zone and material; the material's levels become relative to z.
    StructureBuilder(const StructureBuilder &parent, int x, int y, int z, int sizeX, int sizeY, int rotation,
                     bool mirrored);

    // Two square pillars of 2 * halfWidth blocks about (x, y): a base slab (an underpinnedBox when grounded), the
    // shaft and a capital of one wider slab; the column has a second, wider slab on top.
    void angularColumn(int x, int y, int z, int halfWidth, int height, bool grounded);
    void angularPillar(int x, int y, int z, int halfWidth, int height, bool grounded);
    // A row of merlons along x or y: every other stretch of size blocks of the length, size deep and height high
    // (BlockBuilding). Zone::generate passes a last argument (0 or 1 by the wall's side) that the bodies do not read.
    void battlementX(int x, int y, int z, int length, int size, int height, int unused);
    void battlementY(int x, int y, int z, int length, int size, int height, int unused);
    // A box of the material (m_fillType blocks with BlockFlagProtected). The last argument is not read (false at every
    // call seen).
    void box(int x, int y, int z, int sizeX, int sizeY, int sizeZ, bool unused);
    // The colour with spots up to a tenth brighter where a noise of the zone position exceeds 0.7 (TextureBrick).
    plasma::Vector<3, float> brickColor(const plasma::Vector<3, float> &color, int x, int y, int z);
    // A hanging chain of length blocks upwards from (x, y, z): links of two grey tones, block type 1 with
    // BlockFlagProtected.
    void chain(int x, int y, int z, int length);
    // The material's colour for the block at (x, y, z): the layer its height reaches (above the terrain when the
    // material is relative), textured, the layers blended between their levels.
    plasma::Vector<3, float> colorAt(int x, int y, int z);
    // A cone of the radius about (centerX, centerY), height blocks high at its centre, from z upwards (m_fillType
    // blocks).
    void cone(int centerX, int centerY, int z, int radius, int height);
    Block cube(int x, int y, int z);
    // A box of empty blocks with the flags and BlockFlagProtected.
    void cutBox(int x, int y, int z, int sizeX, int sizeY, int sizeZ, unsigned char flags);
    // A cylinder of the material; where hasCover(cover) holds its top layer is written by setCube(fill) one block
    // higher. The last argument is not read (0 at every call seen).
    void cylinder(int centerX, int centerY, int z, int radius, int height, int fill, int cover, int unused);
    // A cylinder of empty blocks whose light value (Block::lightLevel) is a noise of the column, with the flags.
    void cylinderShaft(int centerX, int centerY, int z, int radius, int height, int flags);
    // cylinder for a structure on uneven ground: each column starts at the first solid block without
    // BlockFlagDestructible at or below z (BlockBuilding).
    void foundation(int centerX, int centerY, int z, int radius, int height, int fill, int cover);
    // Whether the column's top is covered (a Cover; the point is taken to the zone for the noise).
    bool hasCover(int x, int y, int z, int cover);
    // A ladder of width blocks about x, height blocks high: block type 16 in the material's colour, every other rung a
    // tenth darker.
    void ladder(int x, int y, int z, int width, int height);
    // Fills the empty and water blocks of the box with lava (World::lavaColor, BlockLava, light 255) or with muck
    // (World::muckColor, BlockPoison), with the flags.
    void lava(int x, int y, int z, int sizeX, int sizeY, int sizeZ, int flags);
    // A flattened ball of BlockLeaves of the radius about (x, y, z), its centre moved by a noise of the column (two
    // rand() calls seed it).
    void leaves(int x, int y, int z, float radius);
    // Adds light to the blocks with BlockFlagProtected within the radius of the centre, falling off with the squared
    // distance.
    void light(int centerX, int centerY, int centerZ, int radius, unsigned char light);
    // World::rockColor of the block at three tenths of its brightness.
    plasma::Vector<3, float> lowRockColor(int x, int y, int z);
    void muck(int x, int y, int z, int sizeX, int sizeY, int sizeZ, int flags);
    // World::rockColor of the block (TextureRock, FillRock).
    plasma::Vector<3, float> rockColor(int x, int y, int z);
    // The pillars' round counterpart: cylinders of the radius (a foundation as its base when grounded).
    void roundPillar(int x, int y, int z, int radius, int height, bool grounded);
    void setColor(const plasma::Vector<3, float> &color, int texture);
    // Writes the block at (x, y, z) by a Fill, with the flags.
    void setCube(int x, int y, int z, int fill, int flags);
    // Writes the block through the placement unless m_guard or m_terrainOnly forbid it, and records the change in
    // m_updates.
    void setCubeBlock(int x, int y, int z, const Block &block);
    void setMaterial(const plasma::Vector<3, float> &color, int texture, float level,
                     const plasma::Vector<3, float> &color1, int texture1, float level1,
                     const plasma::Vector<3, float> &color2, int texture2, float level2, bool relative);
    void setMaterial(const plasma::Vector<3, float> &color, int texture, float level,
                     const plasma::Vector<3, float> &color1, int texture1, float level1, bool relative);
    void setPlacement(const StructureBuilder &parent, int x, int y, int z, int sizeX, int sizeY, int rotation,
                      bool mirrored);
    void setPlacement(int x, int y, int z, int sizeX, int sizeY, int rotation, bool mirrored);
    // A ball of BlockTree of the radius about (x, y, z).
    void sphere(int x, int y, int z, float radius);
    // A ring between the two radii whose height above z grows with the angle about the centre up to the height (half a
    // turn of a spiral ramp; the top of a type 70 tower), each column from the ground up.
    void spiral(int centerX, int centerY, int z, int innerRadius, int outerRadius, int height);
    // A flight of length by width blocks along x or y, rising from startHeight to endHeight above z, each column
    // filled from the first solid block below z; its top layer as cylinder's. The last argument is not read.
    void stairsX(int x, int y, int z, int length, int width, int startHeight, int endHeight, int fill, int cover,
                 bool unused);
    void stairsY(int x, int y, int z, int width, int length, int startHeight, int endHeight, int fill, int cover,
                 bool unused);
    // Zone::generate hangs one from the ceiling of a castle room.
    void stalactite(int x, int y, int z, int radius, int height);
    // The colour as a Texture renders it at the block; seed varies the wood textures.
    plasma::Vector<3, float> texturedColor(const plasma::Vector<3, float> &color, int x, int y, int z, int texture,
                                           int seed);
    void toZone(int &x, int &y, int &z);
    void toZone(int &x, int &y);
    void toZoneDirection(int &x, int &y);
    int toZoneRotation(int rotation);
    // A round beam of the radius along x or y for the length, its axis rising from z to top (m_fillType blocks, every
    // other slice a tenth darker).
    void tubeX(int x, int y, int z, int top, int length, int radius);
    void tubeY(int x, int y, int z, int top, int length, int radius);
    // box for a structure on uneven ground, as foundation is to cylinder. The last argument is not read.
    void underpinnedBox(int x, int y, int z, int sizeX, int sizeY, int height, int fill, int cover, bool unused);
    // Fills the empty blocks of the box with water (BlockWater, the colour 127, 127, 127), with the flags.
    void water(int x, int y, int z, int sizeX, int sizeY, int sizeZ, int flags);
    // The colour with a grain of up to 15 percent by noise: over x and y offset by the seed (TextureWood), and along y
    // or z by the seed.
    plasma::Vector<3, float> woodColor(const plasma::Vector<3, float> &color, int x, int y, int z, int seed);
    plasma::Vector<3, float> woodColorY(const plasma::Vector<3, float> &color, int x, int y, int z, int seed);
    plasma::Vector<3, float> woodColorZ(const plasma::Vector<3, float> &color, int x, int y, int z, int seed);

    // The zone's block coordinates of the builder's column (x, y).
    int zoneX(int x, int y) const
    {
        const plasma::Matrix<int> &m = m_placement;
        return m[0] * x + m[4] * y + m[12];
    }
    int zoneY(int x, int y) const
    {
        const plasma::Matrix<int> &m = m_placement;
        return m[1] * x + m[5] * y + m[13];
    }
    // Whether the builder's column (x, y) lies in the builder's zone.
    bool isInZone(int x, int y) const
    {
        const plasma::Matrix<int> &m = m_placement;
        return m_zone->contains(m[0] * x + m[4] * y + m[12], m[1] * x + m[5] * y + m[13]);
    }
    // Returns the zone block at the builder's local coordinates.
    Block zoneBlock(int x, int y, int z) const
    {
        const plasma::Matrix<int> &m = m_placement;
        return m_zone->getBlock(m[0] * x + m[4] * y + m[8] * z + m[12], m[1] * x + m[5] * y + m[9] * z + m[13],
                                m[2] * x + m[6] * y + m[10] * z + m[14]);
    }

    void setMaterial(const Material &material) { m_material = material; }

    Zone *m_zone;             // the zone written to
    ServerUpdates *m_updates; // receives a CubeChange for every block setCubeBlock writes, or null
    Material m_material;

    // FillBlend's two colours and texture, and FillMaterial's block type. Not copied from the parent: every builder
    // starts with white and BlockBuilding.
    plasma::Vector<3, float> m_fillColor0;
    plasma::Vector<3, float> m_fillColor1;
    int m_fillTexture;
    int m_guard;
    int m_terrainOnly;
    int m_fillType; // BlockBuilding (6) by default; read as a byte

    plasma::Matrix<int> m_placement;
};

}
