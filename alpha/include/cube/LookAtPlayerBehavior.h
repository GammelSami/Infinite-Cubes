// LookAtPlayerBehavior stops a creature and turns it towards the nearest living player within eight blocks.
#pragma once
#include "cube/Behavior.h"

namespace cube {

class LookAtPlayerBehavior : public Behavior {
public:
    LookAtPlayerBehavior();

    virtual bool Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates);
    virtual Behavior *clone();
};

}
