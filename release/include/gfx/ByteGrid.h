// ByteGrid stores one byte per terrain block for a rectangular set of world columns and a surrounding margin.
#ifndef GFX_BYTEGRID_H
#define GFX_BYTEGRID_H

#include <vector>
#include "cube/ZoneNeighborhood.h"

namespace gfx {

// One ByteGrid column, with bytes from m_baseZ upwards, like cube::Field.
struct ByteColumn {
    ByteColumn()
        : m_baseZ(0)
    {}

    // Stores the byte of the block at height z.
    void set(int z, unsigned char value) { m_bytes[z - m_baseZ] = value; }

    int m_baseZ;
    std::vector<unsigned char> m_bytes;
};

class ByteGrid {
public:
    // Allocates columns for [x0, x1) by [y0, y1), extended by margin on all sides. Each loaded zone column supplies
    // its base height and block count; its bytes start at zero.
    ByteGrid(cube::ZoneNeighborhood &zones, int x0, int y0, int x1, int y1, int margin)
        : m_x0(x0),
          m_x1(x1),
          m_y0(y0),
          m_y1(y1),
          m_margin(margin)
    {
        m_columns = new ByteColumn[width() * width()];
        for (int x = x0 - margin; x < x1 + margin; x++) {
            for (int y = y0 - margin; y < y1 + margin; y++) {
                ByteColumn &target = column(x, y);
                cube::Field *field = zones.field(x, y);
                if (field) {
                    target.m_baseZ = field->m_baseZ;
                    target.m_bytes.resize((int)field->m_blocks.size());
                }
            }
        }
    }
    ~ByteGrid() { delete[] m_columns; }

    // The byte of a block: 0 below the column, 255 above it and outside the rectangle.
    unsigned char lookup(int x, int y, int z)
    {
        if (x < m_x0 - m_margin || y < m_y0 - m_margin || x >= m_x1 + m_margin || y >= m_y1 + m_margin)
            return 255;
        ByteColumn &source = column(x, y);
        if (z < source.m_baseZ)
            return 0;
        if (z >= source.m_baseZ + (int)source.m_bytes.size())
            return 255;
        return source.m_bytes[z - source.m_baseZ];
    }
    // Stores the byte of a block of a loaded column.
    void set(int x, int y, int z, unsigned char value)
    {
        ByteColumn &target = column(x, y);
        target.m_bytes[z - target.m_baseZ] = value;
    }

    // The columns in a row, margins included.
    int width() const { return m_x1 + 2 * m_margin - m_x0; }

    ByteColumn &column(int x, int y) { return m_columns[(x + m_margin) + ((y + m_margin) - m_y0) * width() - m_x0]; }

    int m_x0;
    int m_x1;
    int m_y0;
    int m_y1;
    int m_margin;
    ByteColumn *m_columns; // new[]
};

} // namespace gfx

namespace cube {

// Computes transparent-block light levels (0..255) over the rectangle and its margin, writes them into light and
// stores each interior block's level in its first colour byte.
void computeLight(ZoneNeighborhood &zones, gfx::ByteGrid &light, int x0, int y0, int x1, int y1, int margin);

}

#endif
