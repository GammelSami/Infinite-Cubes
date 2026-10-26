// BytesIO is a byte buffer with a read position, used for database blobs and multiplayer packets. Copies start with
// their read position at zero.
#pragma once
#include <string.h>
#include <list>
#include <vector>

namespace cube {

class BytesIO {
public:
    BytesIO()
        : m_offset(0)
    {}
    // Copies the data and resets the read position to zero.
    BytesIO(const BytesIO &other)
        : m_data(other.m_data),
          m_offset(0)
    {}

    int size() const { return (int)m_data.size(); }
    bool empty() const { return size() == 0; }
    bool atEnd() const { return (size_t)m_offset >= m_data.size(); }
    void seek(int offset) { m_offset = offset; }
    // The first byte, or null for an empty buffer.
    char *data() { return m_data.empty() ? nullptr : &m_data[0]; }
    void resize(int size) { m_data.resize(size); }
    void clear()
    {
        m_offset = 0;
        m_data.clear();
    }
    // Moves the offset forward, at most to the end.
    void skip(int count) { m_offset = m_offset + count > size() ? size() : m_offset + count; }
    // Copies size bytes from the offset and advances it; at the end of the data nothing is copied and the offset moves
    // to the end.
    void read(void *buffer, int size)
    {
        if (m_offset + size > (int)m_data.size()) {
            m_offset = (int)m_data.size();
        } else {
            memcpy(buffer, &m_data[m_offset], size);
            m_offset += size;
        }
    }

    // Reads a value at the current position. If insufficient data remains, leaves value unchanged and moves the
    // position to the end. Returns true in either case.
    template<class T> bool read(T &value)
    {
        read(&value, sizeof(T));
        return true;
    }
    // Reads a count and that many elements, appending them to list.
    template<class T> void read(std::list<T> &list)
    {
        int count;
        read(&count, sizeof(count));
        for (int i = 0; i < count; ++i) {
            T value;
            read(&value, sizeof(T));
            list.push_back(value);
        }
    }

    // Grows the data by the value's size and stores it at the offset.
    template<class T> void write(const T &value) { write(&value, sizeof(T)); }
    // Writes the count and the elements of list.
    template<class T> void write(const std::list<T> &list)
    {
        int count = (int)list.size();
        write(&count, sizeof(count));
        for (typename std::list<T>::const_iterator it = list.begin(); it != list.end(); ++it)
            write(&*it, sizeof(T));
    }
    void write(const void *buffer, int size)
    {
        m_data.resize(m_data.size() + size);
        memcpy(&m_data[m_offset], buffer, size);
        m_offset += size;
    }

    // Copies size bytes from offset when the data holds that range.
    void read(void *buffer, unsigned int size, unsigned int offset)
    {
        if (offset + size <= this->size()) {
            seek(offset);
            read(buffer, size);
        }
    }

    std::vector<char> m_data;
    int m_offset; // the read position
};

}
