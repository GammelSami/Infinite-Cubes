// Field holds one column of a zone's terrain.
#include "cube/Field.h"

#include "cube/DefaultBlocks.h"

#include "cube/Fixed.h"

namespace cube {

Field::Field()
    : humidity(0),
      temperature(0),
      firelands(0),
      m_baseZ(0),
      m_groundZ(0),
      m_blocks(0),
      m_size(0)
{}

Field::~Field()
{
    if (m_blocks)
        delete[] m_blocks;
}

void Field::resize(int size, int offset)
{
    if (size == m_size)
        return;
    if (size < 0)
        size = 0;
    if (size == 0) {
        if (m_blocks)
            delete[] m_blocks;
        m_blocks = 0;
    } else {
        Block *blocks = new Block[size];
        if (size < m_size) {
            for (int i = 0; i < size; i++)
                blocks[i] = m_blocks[i];
        } else {
            for (int i = 0; i < m_size; i++)
                blocks[i + offset] = m_blocks[i];
        }
        if (m_blocks)
            delete[] m_blocks;
        m_blocks = blocks;
    }
    m_baseZ -= offset;
    m_size = size;
}

}
