// RandomWalkBehavior wanders and turns back towards the origin beyond ten blocks.
#pragma once
#include "cube/Behavior.h"

namespace cube {

class RandomWalkBehavior : public Behavior {
public:
    RandomWalkBehavior();

    virtual bool Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates);
    virtual Behavior *clone();

    // Milliseconds until the next change of direction: 3000..7999.
    int m_timer;
};

}
