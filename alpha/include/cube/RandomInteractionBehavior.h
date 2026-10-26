// RandomInteractionBehavior chooses idle interactions with nearby objects.
#pragma once
#include "cube/Behavior.h"

namespace cube {

class RandomInteractionBehavior : public Behavior {
public:
    RandomInteractionBehavior();
    virtual bool Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates);
    virtual Behavior *clone();

    int m_timer; // milliseconds before selecting another interaction
};

}
