// SequentialBehavior tries its owned behaviors in order.
#include "cube/SequentialBehavior.h"

#include "cube/Creature.h"
#include "cube/DefaultBlocks.h"
#include "cube/World.h"

namespace cube {

SequentialBehavior::SequentialBehavior(const SequentialBehavior &other)
{
    for (std::list<Behavior *>::const_iterator it = other.m_behaviors.begin(); it != other.m_behaviors.end(); ++it)
        m_behaviors.push_back((*it)->clone());
}

SequentialBehavior::SequentialBehavior() {}

SequentialBehavior::~SequentialBehavior()
{
    for (std::list<Behavior *>::iterator it = m_behaviors.begin(); it != m_behaviors.end(); ++it)
        delete *it;
}

// The first behaviour that applies steers the creature; when none does, the creature stops.
bool SequentialBehavior::Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates)
{
    for (std::list<Behavior *>::iterator it = m_behaviors.begin(); it != m_behaviors.end(); ++it) {
        if ((*it)->Update(creature, world, elapsed, updates))
            return true;
    }
    creature->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    return false;
}

Behavior *SequentialBehavior::clone()
{
    return new SequentialBehavior(*this);
}

}
