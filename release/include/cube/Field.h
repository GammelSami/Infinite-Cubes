// Field stores one vertical terrain column and its sampled terrain parameters.
#pragma once
#include <vector>
#include "cube/Block.h"

namespace cube {

class Field {
public:
    Field();
    virtual ~Field();

    // Resizes the column, preserving existing blocks. When growing, moves old blocks up by offset, zeroes the new
    // blocks below them and lowers m_baseZ by offset. A negative size gives an empty column; an unchanged size does
    // nothing.
    void resize(int size, int offset);
    // The block at an index relative to m_baseZ: a black solid block below the column and a black empty one above it.
    Block getBlock(int index)
    {
        if (index < 0) {
            static Block below;
            below = Block(plasma::Vector<3, unsigned char>(0, 0, 0), BlockSolid);
            return below;
        }
        if (index >= (int)m_blocks.size()) {
            static Block above;
            above = Block(plasma::Vector<3, unsigned char>(0, 0, 0), BlockAir);
            return above;
        }
        return m_blocks[index];
    }
    // Stores the block at an index relative to m_baseZ, growing the column upwards, or downwards for a negative index
    // (the block then becomes the lowest one).
    void setBlock(int index, const Block &block)
    {
        if (index < 0) {
            resize((int)m_blocks.size() - index, -index);
            m_blocks[0] = block;
        } else {
            if (index >= (int)m_blocks.size()) {
                resize(index + 1, 0);
            }
            m_blocks[index] = block;
        }
    }

    size_t size() const { return m_blocks.size(); }

    float m_height;
    float m_temperature;
    float m_humidity;
    float m_mountainCliffs;
    float m_firelands;
    float m_undead;
    float m_enchantedForest;
    float m_mountains; // Zone::generate stores BiomeSample::mountains here
    float m_sandAmount;
    float m_valleyWater;    // Zone::generate stores World::valleyWater here
    float m_lavaChannels;   // World::lavaChannels
    float m_poisonChannels; // World::poisonChannels
    float m_streetAmount;
    // Zero after construction; resize lowers it when it grows the column downwards.
    int m_baseZ;
    std::vector<Block> m_blocks;
};

}
