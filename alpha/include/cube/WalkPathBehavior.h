// WalkPathBehavior moves a creature between waypoints on a closed path.
#pragma once
#include <vector>
#include "plasma/Vector.h"
#include "cube/Behavior.h"
#include "cube/Fixed.h"

namespace cube {

class WalkPathBehavior : public Behavior {
public:
    // An empty path; radius is how near a point counts as reached.
    explicit WalkPathBehavior(float radius);
    // Copies path, next point and radius, resetting the timer to zero.
    WalkPathBehavior(const WalkPathBehavior &other);

    virtual bool Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates);
    virtual Behavior *clone();

    std::vector<plasma::Vector<3, Fixed>> m_path; // world positions, walked in order and then from the start again
    int m_next;                                   // Next waypoint index, kept in range by update.
    // Milliseconds to wait before moving; left unset by construction and cleared by copying.
    int m_timer;
    float m_radius; // in blocks
};

}
