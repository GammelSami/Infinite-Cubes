// Sequence stores named attribute-animation keys in frame order.
#pragma once
#include <vector>

namespace plasma {

struct Key {
    Key(int element, int frame, float smoothness)
        : element(element),
          frame(frame),
          smoothness(smoothness)
    {}

    int element;
    int frame;
    float smoothness;
};

class Sequence {
public:
    Sequence();
    Sequence(const Sequence &other);
    Sequence &operator=(const Sequence &other);

    void removeKey(int index);
    void clear();
    int addKey(int element, int frame, float smoothness);

    int count() const { return (int)m_keys.size(); }
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
    int m_loopLength;
};

}
