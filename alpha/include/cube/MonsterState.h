#pragma once
#include "cube/ItemIds.h"
#include "plasma/Vector.h"

namespace cube {
// The region's roaming monster state, saved under the monster key. Region generation selects its level, race and zone; spawning uses its rarity, and monster missions adopt its race and zone.
struct MonsterState {
    enum {
        NoRace = 0
    };

    // Initializes all fields except the monster zone.
    MonsterState()
        : race(NoRace),
          level(0),
          rarity(ItemRarityWhite)
    {}
    int race; // 0 for none
    int level;
    unsigned char rarity;
    // Monster zone, saved as a pair and left unset by construction.
    plasma::Vector<2, int> zone;
};
}
