// plasma::Sequence: a named attribute animation, a list of keys in frame order. Each key shows one of the
// attribute's elements at its frame; a looping sequence (m_loopLength frames per loop) wraps from its last key
// back to the first.
#pragma once
#include <vector>

namespace plasma {

struct Key {
    Key(int element, int frame, float smoothness)
        : element(element),
          frame(frame),
          smoothness(smoothness)
    {}

    int element; // Index of the attribute element shown at this key.
    int frame;
    float smoothness; // 0: the curve leaves the key along the chord, 1: along the neighbours' tangent.
};

class Sequence {
public:
    Sequence();
    Sequence(const Sequence &other);
    Sequence &operator=(const Sequence &other);

    void removeKey(int index);
    void clear();

    // Inserts a key in frame order (before a key at the same frame) and returns its index.
    int addKey(int frame, int element, float smoothness);

    int count() const { return m_keys.size(); }

    // Wraps the index around in a looping sequence, clamps it otherwise.
    Key &operator[](int index)
    {
        if (m_loopLength)
            index = (count() + index) % count();
        else {
            if (index < 0)
                index = 0;
            if (index >= count())
                index = count() - 1;
        }
        return m_keys[index];
    }

    std::vector<Key> m_keys;
    int m_loopLength; // Frames per loop; 0 plays the keys once.
};

}
