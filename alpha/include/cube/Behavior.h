// Behavior steers a creature once per update. Derived behaviors implement update and clone. This interface has no virtual destructor; SequentialBehavior owns and deletes its children.
#pragma once

namespace cube {

class Creature;
class ServerUpdates;
class World;

class Behavior {
public:
    // Steers the creature for elapsed milliseconds and appends world events to the supplied update lists. Returns true while the behavior applies; SequentialBehavior stops at the first true result.
    virtual bool Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates) = 0;
    // A new copy of the behaviour (each spawned creature gets its own).
    virtual Behavior *clone() = 0;
};

}
