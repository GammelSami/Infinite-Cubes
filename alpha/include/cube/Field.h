// Field stores a column of terrain blocks from m_baseZ upwards, with humidity, temperature and firelands parameters.
#pragma once
#include "cube/Block.h"

namespace cube {

class Field {
public:
    Field();
    virtual ~Field();

    // Gives the column size blocks (none for a negative size), keeping the old ones: at their index
    // when the column shrinks, moved up by offset when it grows (the new blocks are zero), and lowers
    // m_baseZ by offset. Nothing happens when the size is unchanged. This is the column resize
    // operation in both builds.
    void resize(int size, int offset);

    // Indexed relative to m_baseZ. Out-of-range reads use local fallback blocks, refreshed on each
    // read.
    const Block &getBlock(int index)
    {
        if (index < 0) {
            static Block solid;
            solid = Block(plasma::Vector<3, unsigned char>(0, 0, 0), BlockSolid);
            return solid;
        }
        if (index >= m_size) {
            static Block air;
            air = Block(plasma::Vector<3, unsigned char>(255, 255, 255), BlockAir);
            return air;
        }
        return m_blocks[index];
    }

    // Returns a mutable block relative to m_baseZ. Out-of-range accesses refresh black fallback blocks: solid below the column, air above it.
    Block &getMutableBlock(int index)
    {
        if (index < 0) {
            static Block below;
            below = Block(plasma::Vector<3, unsigned char>(0, 0, 0), BlockSolid);
            return below;
        }
        if (index >= m_size) {
            static Block above;
            above = Block(plasma::Vector<3, unsigned char>(0, 0, 0), BlockAir);
            return above;
        }
        return m_blocks[index];
    }

    // Grows the column as needed; a block under water keeps its water flag and is replaced only by a
    // non-air block.
    void setBlock(int index, const Block &block)
    {
        if (index < 0) {
            resize(m_size - index, -index);
            m_blocks[0] = block;
        } else {
            if (index >= m_size)
                resize(index + 1, 0);
            if (!m_blocks[index].isAboveWater()) {
                if (block.type & BlockTypeMask) {
                    m_blocks[index] = block;
                    m_blocks[index].type |= (1 << BlockFlagUnderwater);
                }
            } else {
                m_blocks[index] = block;
            }
        }
    }

    int size() const { return m_size; }
    int getBaseZ() const { return m_baseZ; }

    // Terrain humidity, temperature and firelands values, initially zero. The zone generator fills them; terrain queries read them back.
    float humidity;
    float temperature;
    float firelands;
    // Height of m_blocks[0]. Generation sets it; downward growth lowers it.
    int m_baseZ;
    int m_groundZ;   // ground reference before cliffs and structures
    Block *m_blocks; // m_size blocks, from new Block[]
    int m_size;
};

}
