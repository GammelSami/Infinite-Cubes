#include "cube/Field.h"

#include "cube/Noise.h"

namespace cube {

Field::Field()
    : m_baseZ(0)
{}

Field::~Field() {}

void Field::resize(int size, int offset)
{
    if (size == (int)m_blocks.size()) {
        return;
    }
    int count = size >= 0 ? size : 0;
    if (count == 0) {
        m_blocks.clear();
    } else {
        std::vector<Block> blocks(count);
        if (count < (int)m_blocks.size()) {
            for (int i = 0; i < count; i++) {
                blocks[i] = m_blocks[i];
            }
        } else {
            for (int i = 0; i < (int)m_blocks.size(); i++) {
                blocks[i + offset] = m_blocks[i];
            }
        }
        m_blocks = blocks;
    }
    m_baseZ -= offset;
}

}
