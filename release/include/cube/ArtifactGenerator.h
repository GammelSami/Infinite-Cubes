// ArtifactGenerator builds procedural voxel models for artifacts. The constructor resizes the sprite to 24 x 24 x 24,
// chooses four random colours from (type, seed), generates the shape and rebuilds the mesh. Types 0, 1, 2, 5 and 6
// produce vessels, pendants, rings, crowns and staves; type 3 is a box and type 4 a cut gem. Colours use twice the
// byte range because Sprite::setVoxel halves them. Random values come from hash(m_random++).
#pragma once
#include "cube/ItemIds.h"
#include "cube/Noise.h"
#include "plasma/Vector.h"

namespace cube {

class Sprite;

class ArtifactGenerator {
public:
    ArtifactGenerator(Sprite *sprite, int type, int seed);

    // The models, by artifact type (Item::artifactType, modifier % 7). Type 1: a pendant (disc, ball, cube or plate,
    // each plain or framed) on a chain that sways with noise.
    void amulet();
    // Type 5: gems on a circle below a band whose top edge is cut into spikes.
    void crown();
    // Type 2: a band of one to five rings around the y axis with gems on it or one stone set on top.
    void ring();
    // Type 6: a staff with one to three collars and a cut gem on top.
    void scepter();
    // Type 3: one box of random size in the middle.
    void stone();
    // Type 0: a hollow vessel turned around the z axis, its radius a quadratic curve over the height, patterned by
    // stripes, checks or noise in the complementary hue, with a rim, a foot and up to two handles.
    void vase();

    // The solids. Each fills its voxels with a colour that runs from bottom to top over the solid's height in z. The
    // box from (x, y, z), sizeX by sizeY by sizeZ voxels.
    void box(int x, int y, int z, int sizeX, int sizeY, int sizeZ, const plasma::Vector<3, float> &bottom,
             const plasma::Vector<3, float> &top);
    // The cylinder around the z axis through (x, y), from z upwards.
    void cylinder(int x, int y, int z, int radius, int height, const plasma::Vector<3, float> &bottom,
                  const plasma::Vector<3, float> &top);
    // The cylinder around the y axis through (x, z), from y on.
    void cylinderXZ(int x, int y, int z, int radius, int length, const plasma::Vector<3, float> &bottom,
                    const plasma::Vector<3, float> &top);
    // The cut gem around (x, y, z): the voxels whose largest of |dx|, |dy|, |dz| and the four weighted sums is at most
    // radius.
    void gem(int x, int y, int z, int radius, const plasma::Vector<3, float> &bottom,
             const plasma::Vector<3, float> &top, float cutXY, float cutXZ, float cutYZ, float cutXYZ);
    // cylinderXZ without the voxels closer than radius - thickness to the axis.
    void ringXZ(int x, int y, int z, int radius, int length, int thickness, const plasma::Vector<3, float> &bottom,
                const plasma::Vector<3, float> &top);
    // The ball around (x, y, z).
    void sphere(int x, int y, int z, int radius, const plasma::Vector<3, float> &bottom,
                const plasma::Vector<3, float> &top);
    // The upper half (from z upwards) of cylinderXZ, and the same around the x axis (from x on).
    void topCylinderXZ(int x, int y, int z, int radius, int length, const plasma::Vector<3, float> &bottom,
                       const plasma::Vector<3, float> &top);
    void topCylinderYZ(int x, int y, int z, int radius, int length, const plasma::Vector<3, float> &bottom,
                       const plasma::Vector<3, float> &top);

protected:
    plasma::Vector<3, float>
        m_colors[4];       // Each channel starts at (random % 250) + 10; shape generators replace some colours.
    unsigned int m_random; // the running random counter
    Sprite *m_sprite;
    ArtifactId m_type;
    int m_seed;                  // also offsets the noise of amulet and vase
    unsigned char m_unused48[8]; // not written by the constructor; no access in the program
};

// The colour conversions of the vase (hue in degrees, saturation and value 0..1; rgb 0..1): the widely copied C
// functions, with float members and double arithmetic.
plasma::Vector<3, float> hsv2rgb(const plasma::Vector<3, float> &hsv);
plasma::Vector<3, float> rgb2hsv(const plasma::Vector<3, float> &rgb);

}
