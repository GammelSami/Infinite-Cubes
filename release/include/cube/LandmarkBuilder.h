// LandmarkBuilder generates trees, stumps and fallen trunks in a zone. It keeps the zone, two height and colour
// endpoints and a colour-pattern mode; Zone::generate chooses the shape by building type.
#pragma once
#include "plasma/Vector.h"

namespace cube {

class Zone;
struct Block;

// a random warm color used by the standalone structures.
plasma::Vector<3, float> randomWoodColor();

// LandmarkBuilder::m_colorPattern: how colorAt colours a block.
enum LandmarkColorPattern {
    LandmarkColorGradient = 0,    // the vertical gradient only
    LandmarkColorBirchFlecks = 1, // one block in eight dark grey (a birch's bark)
};

class LandmarkBuilder {
public:
    explicit LandmarkBuilder(Zone *zone);

    // branching tree with a broad crown of leaf clumps.
    void oak(int x, int y, int z, int width, int height); // types 1, 8, 6 (with sizeA * 0.8)
    // thirty tapering radial leaf branches, with snow in cold climates.
    void evergreen(int x, int y, int z, int width, int height); // type 2
    // pale spotted bark and a crown of leaf ellipsoids.
    void birch(int x, int y, int z, int width, int height); // type 3
    // broad crown with hanging leaf strands.
    void jungleTree(int x, int y, int z, int width, int height); // type 4
    // curved, banded trunk and five to seven drooping radial fronds.
    void radialPalm(int x, int y, int z, int width, int height); // types 5, 7 (with sizeA * 0.8)
    // four spreading roots and a short round trunk.
    void stump(int x, int y, int z, int width, int height); // type 19
    // fallen trunks along X and Y, with an optional hollow. Types 20 and 21, one of the two at random, with a scale of
    // 2..5 (20) or 3..6 (21) and a flag (rand() % 3 == 0 for 20, true for 21).
    void prostrateTrunkX(int x, int y, int z, float radius, int length, bool hollow);
    void prostrateTrunkY(int x, int y, int z, float radius, int length, bool hollow);

    // a tapering branch of spheres, stepped in 16.16 dots. The vertical step is multiplied by the progress along the
    // branch.
    plasma::Vector<3, int> buildBranch(int x, int y, int z, const plasma::Vector<3, float> &direction,
                                       float firstRadius, float lastRadius, int count);
    void sphere(int x, int y, int z, float radius);

    // vertical color interpolation, with sparse dark bark spots.
    plasma::Vector<3, float> colorAt(int x, int y, int z);

    // cubic radius flare and linearly tapered trunk, respectively.
    void stumpFlare(int x, int y, int z, float firstRadius, float lastRadius, int height);
    void trunk(int x, int y, int z, float firstRadius, float lastRadius, int height);

    // a one-block curved branch, colored at its starting point.
    plasma::Vector<3, int> twig(int x, int y, int z, const plasma::Vector<3, float> &direction, int count);

    // horizontal disk and flag2-preserving block write.
    void disk(int x, int y, int z, float radius, const Block &block);
    void setBlock(int x, int y, int z, const Block &block);

    // noise-warped leaf ellipsoid with hashed hanging strands.
    void hangingLeaves(int x, int y, int z, float radius, float halfHeight);

    // leaf ellipsoid displaced vertically by column noise.
    void ovalLeaves(int x, int y, int z, float radius, float halfHeight, bool snow);

    // Sets the heights and colours between which colorAt interpolates.
    void setGradient(int lowerHeight, int upperHeight, const plasma::Vector<3, float> &lowerColor,
                     const plasma::Vector<3, float> &upperColor)
    {
        m_lowerHeight = lowerHeight;
        m_upperHeight = upperHeight;
        m_lowerColor = lowerColor;
        m_upperColor = upperColor;
    }

    Zone *m_zone;
    int m_lowerHeight;                     // colorAt lower blend endpoint
    int m_upperHeight;                     // colorAt upper blend endpoint
    plasma::Vector<3, float> m_lowerColor; // lower endpoint RGB, initially white
    plasma::Vector<3, float> m_upperColor; // upper endpoint RGB, initially white
    int m_colorPattern;                    // a LandmarkColorPattern; initially LandmarkColorGradient
};

}
