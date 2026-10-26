// cube::IntTriple: three ints in a std::array, ordered as std::array orders them. It keys the AI's path map and open
// set (World::AIManager::State), PlayerData's two position sets, Zone's occupied-block set and the places Missions'
// constructor sorts out (type, cell x, cell y). Zone's position maps have a key type of their own (Zone::PositionKey).
#pragma once
#include <array>

namespace cube {

class IntTriple {
public:
    int &operator[](int index) { return elements[index]; }
    const int &operator[](int index) const { return elements[index]; }
    bool operator<(const IntTriple &other) const
    {
        return std::lexicographical_compare(elements.begin(), elements.end(), other.elements.begin(),
                                            other.elements.end());
    }

    std::array<int, 3> elements;
};

}
