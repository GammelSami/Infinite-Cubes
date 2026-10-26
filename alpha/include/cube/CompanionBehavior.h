// CompanionBehavior keeps a pet with its owner.
#pragma once
#include "cube/Behavior.h"

namespace cube {

class CompanionBehavior : public Behavior {
public:
    CompanionBehavior();
    explicit CompanionBehavior(long long ownerId);

    virtual bool Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates);
    virtual Behavior *clone();

    // Padding (the id is 8-byte aligned; the copy skips it).
    long long m_ownerId; // the creature followed; 0 when constructed without one
};

}
