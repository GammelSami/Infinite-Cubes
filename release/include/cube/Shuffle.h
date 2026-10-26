#pragma once
#include <algorithm>
#include <cstdlib>

namespace cube {
// The rand-based shuffle used before C++17 removed std::random_shuffle.
template<class Iterator> void shuffleWithRand(Iterator first, Iterator last)
{
    if (first == last)
        return;
    for (Iterator current = first + 1; current != last; ++current) {
        const auto selected = std::rand() % (current - first + 1);
        if (selected != current - first)
            std::iter_swap(current, first + selected);
    }
}
}
