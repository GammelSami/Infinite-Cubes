// Block stores one terrain voxel as an RGB color and a byte containing type and flags.
#pragma once
#include "plasma/Vector.h"

namespace cube {

// Block types. Spawn searches treat air and water as open space.
enum BlockType {
    BlockAir = 0,
    BlockSolid = 1,
    BlockWater = 2,
    BlockWet = 3, // Wet ground sounds like water underfoot and counts as water for pets.
    // Surface ground becomes rock, sand or snow according to terrain color and climate.
    BlockGrass = 4,
    BlockGround = 5,   // World::buildRock replaces it like grass and sand
    BlockBuilding = 6, // Stone fill for buildings, rocks and player placement.
    // Trunks and foliage both allow falling type-four particles to pass through.
    BlockTree = 7,
    BlockLeaves = 8,
    BlockSand = 9,
    BlockSnow = 10,
    BlockRoad = 11, // paved paths in generateZone
    BlockRock = 12
};

// The type byte stores BlockType in its low five bits and three flags in its high bits.
enum BlockFlag {
    // An exploding creature blows such blocks away (isDestructible); the tree builder sets it on wood
    // and foliage.
    BlockFlagDestructible = 5,
    // Air or blocks inside a building: getBlock keeps such air below sea level dry; World::update's
    // build interaction sets it.
    BlockFlagInterior = 6,
    // Set in the water default block. Field::setBlock keeps it when it stores into such a block and
    // leaves the block unchanged when the new one is air; World::setBlock uses the water default at z
    // <= 0 outside interiors.
    BlockFlagUnderwater = 7
};

// Masks separating the type from its flags.
enum BlockTypeMasks {
    BlockTypeMask = 0x1f,
    BlockFlagMask = (1 << BlockFlagDestructible) | (1 << BlockFlagInterior) | (1 << BlockFlagUnderwater)
};

struct Block {
    // Initializes every member to zero.
    Block()
        : color(0, 0, 0),
          type(0)
    {}
    // Assigns the colour, then the type.
    Block(const plasma::Vector<3, unsigned char> &color, unsigned char type)
    {
        this->color = color;
        this->type = type;
    }

    // The type without the flag bits.
    int blockType() const { return type & ~BlockFlagMask; }

    // Whether a creature collides with the block: neither air nor water.
    bool isSolid() const
    {
        int blockType = type & ~BlockFlagMask;
        return blockType != BlockAir && blockType != BlockWater;
    }

    // Bit 5 of the type: an exploding creature blows such blocks away.
    bool isDestructible() const { return (type >> BlockFlagDestructible) & 1; }
    // BlockFlagInterior.
    bool isInterior() const { return (type >> BlockFlagInterior) & 1; }
    // Whether BlockFlagUnderwater is clear.
    bool isAboveWater() const { return !((type >> BlockFlagUnderwater) & 1); }

    plasma::Vector<3, unsigned char> color; // +0 red, +1 green, +2 blue
    unsigned char type;                     // +3: BlockType in bits 0..4, BlockFlag bits above
};

}
