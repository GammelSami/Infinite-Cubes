#pragma once
#include <vector>
#include <string.h>

namespace cube {
// Byte buffer with a current read and write position.
class BytesIO {
public:
    BytesIO()
        : m_offset(0)
    {}
    int size() const { return (int)m_data.size(); }
    void read(void *buffer, int count)
    {
        if (m_offset + count > (int)m_data.size()) {
            m_offset = (int)m_data.size();
        } else {
            memcpy(buffer, &m_data[m_offset], count);
            m_offset += count;
        }
    }
    bool atEnd() const { return (unsigned int)m_offset >= m_data.size(); }
    // Advances the offset, at most to the end.
    void skip(int count) { m_offset = m_offset + count > size() ? size() : m_offset + count; }
    // Serialized writes grow the buffer, then advance its current offset.
    void write(const void *buffer, int count)
    {
        m_data.resize(m_data.size() + count);
        memcpy(&m_data[m_offset], buffer, count);
        m_offset += count;
    }
    // The bytes, or null when there are none.
    char *data() { return m_data.empty() ? 0 : &m_data[0]; }
    // Deflates the bytes in place.
    void compress();
    void decompress();
    void descramble();

    std::vector<char> m_data;
    int m_offset;
};

// Deflates data into result with zlib's compress.
void compress(const std::vector<char> &data, std::vector<char> &result);

// Inflates a zlib buffer. It takes the bytes, not a BytesIO: WorldMap's tile loaders pass a
// std::vector<char> they construct with its size.
bool decompress(const std::vector<char> &input, std::vector<char> &output);
}
