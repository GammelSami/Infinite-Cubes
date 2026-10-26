// SequentialBehavior owns a list of behaviors tried in order. The first applicable behavior ends the update.
#pragma once
#include <list>
#include "cube/Behavior.h"

namespace cube {

class SequentialBehavior : public Behavior {
public:
    // A copy holds a clone of each of the other's behaviours.
    SequentialBehavior(const SequentialBehavior &other);
    SequentialBehavior();

    virtual bool Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates);
    virtual Behavior *clone();
    // Deletes the owned behaviors.
    virtual ~SequentialBehavior();

    std::list<Behavior *> m_behaviors; // tried first to last
};

}
