// SpawnLocationBehavior walks to the current daily-schedule location and keeps the creature there.
#pragma once
#include "cube/Behavior.h"

namespace cube {

class SpawnLocationBehavior : public Behavior {
public:
    SpawnLocationBehavior();

    virtual bool Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates);
    virtual Behavior *clone();
};

}
