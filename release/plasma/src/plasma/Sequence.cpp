// Inserts animation keys in frame order; addKey takes the element before the frame.
#include "plasma/Sequence.h"
#include "plasma/EditFilter.h"

namespace plasma {

Sequence::Sequence(const Sequence &other)
{
    *this = other;
}

Sequence::Sequence()
    : m_loopLength(0)
{}

Sequence &Sequence::operator=(const Sequence &other)
{
    m_loopLength = other.m_loopLength;
    m_keys = other.m_keys;
    return *this;
}

void Sequence::clear()
{
    m_keys.clear();
}

void Sequence::removeKey(int index)
{
    m_keys.erase(m_keys.begin() + index);
}

int Sequence::addKey(int element, int frame, float smoothness)
{
    int index = 0;
    for (std::vector<Key>::iterator it = m_keys.begin(); it != m_keys.end(); ++index, ++it) {
        if (frame <= it->frame) {
            m_keys.insert(it, Key(element, frame, smoothness));
            return index;
        }
    }
    m_keys.push_back(Key(element, frame, smoothness));
    return index;
}

}
