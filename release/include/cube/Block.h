// cube::Block and cube::BlockProperties<T>: one voxel of the world's terrain (a block) as the zones store it, and its
// colour/type/flags record with the colour channels widened to T (the terrain interpolation works on
// BlockProperties<float>).
#pragma once
#include "cube/BlockIds.h"
#include "plasma/Vector.h"

namespace cube {

// Zero-initialises the colour, light level, type and flags.
template<typename T> struct BlockProperties {
    BlockProperties()
        : color(0, 0, 0),
          lightLevel(0),
          type(0),
          flags(0)
    {}
    BlockProperties(const plasma::Vector<3, T> &color, unsigned char type)
        : color(color),
          lightLevel(0),
          type(type),
          flags(0)
    {}
    BlockProperties(T red, T green, T blue, unsigned char type, unsigned char flags)
    {
        color[0] = red;
        color[1] = green;
        color[2] = blue;
        lightLevel = 0;
        this->type = type;
        this->flags = flags;
    }
    // Constructs a block with the supplied colour, type, flags and light level for zone generation.
    BlockProperties(const plasma::Vector<3, T> &color, unsigned char type, unsigned char flags,
                    unsigned char lightLevel)
        : color(color),
          lightLevel(lightLevel),
          type(type),
          flags(flags)
    {}

    // Converts generated colour channels to stored channels by halving them and clamping to 0..255; copies the light
    // level, type and flags.
    template<typename U> explicit BlockProperties(const BlockProperties<U> &properties)
    {
        plasma::Vector<3, U> scaled(properties.color);
        scaled = scaled * 0.5f;
        for (int i = 0; i < 3; i++) {
            if (scaled[i] < 0.0f) {
                scaled[i] = 0.0f;
            } else if (scaled[i] > 255.0f) {
                scaled[i] = 255.0f;
            }
        }
        color[0] = (T)scaled[0];
        color[1] = (T)scaled[1];
        color[2] = (T)scaled[2];
        lightLevel = properties.lightLevel;
        type = properties.type;
        flags = properties.flags;
    }

    plasma::Vector<3, T>
        color;                // RGB colour channels, stored as bytes in Block and floats during terrain interpolation.
    unsigned char lightLevel; // Light level used by World::computeLight; StructureBuilder::light saturates it at 255.
    unsigned char type;       // BlockType value stored as a byte.
    unsigned char flags;

    // Whether the block belongs to a dungeon; Game::update uses it to name the dungeon and select its music.
    bool isInDungeon() const { return (flags & BlockFlagDungeon) != 0; }

    // Whether the block is solid: anything but air and water.
    bool isSolid() const { return type != BlockAir && type != BlockWater; }

    // Whether the block is open space: air or water without a barrier, or with a barrier that spirits pass. Plant
    // placement looks for such blocks.
    bool isPassable() const
    {
        if (isSolid())
            return false;
        if (flags & BlockFlagBarrier) {
            if (flags & BlockFlagSpiritPassable)
                return true;
            return false;
        }
        return true;
    }

    // Whether the block leaves room for an entity: air or water without the barrier flag.
    bool isOpen() const { return !isSolid() && !(flags & BlockFlagBarrier); }
};

struct Block : BlockProperties<unsigned char> {
    Block() {}
    Block(const plasma::Vector<3, unsigned char> &color, unsigned char type)
        : BlockProperties<unsigned char>(color, type)
    {}
    Block(const plasma::Vector<3, unsigned char> &color, unsigned char type, unsigned char flags,
          unsigned char lightLevel)
        : BlockProperties<unsigned char>(color, type, flags, lightLevel)
    {}
    // Constructs the base directly from the supplied colour channels, light level, type and flags.
    Block(unsigned char red, unsigned char green, unsigned char blue, unsigned char type, unsigned char flags)
        : BlockProperties<unsigned char>(red, green, blue, type, flags)
    {}

    // A generated block as the zones store it (BlockProperties' converting constructor; the structure builder's shape
    // functions build their blocks with it).
    Block(const BlockProperties<float> &properties)
        : BlockProperties<unsigned char>(properties)
    {}

    Block &operator=(const BlockProperties<float> &properties)
    {
        plasma::Vector<3, float> scaled = properties.color * 0.5f;
        for (int i = 0; i < 3; i++) {
            if (scaled[i] < 0.0f) {
                scaled[i] = 0.0f;
            } else if (scaled[i] > 255.0f) {
                scaled[i] = 255.0f;
            }
        }
        color[0] = (unsigned char)scaled[0];
        color[1] = (unsigned char)scaled[1];
        color[2] = (unsigned char)scaled[2];
        lightLevel = properties.lightLevel;
        type = properties.type;
        flags = properties.flags;
        return *this;
    }
};

}
