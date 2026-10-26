// RandomWalkBehavior selects wandering directions.
#include "cube/RandomWalkBehavior.h"

#include <stdlib.h>
#include "cube/Creature.h"
#include "cube/DefaultBlocks.h"
#include "cube/World.h"

namespace cube {

RandomWalkBehavior::RandomWalkBehavior()
    : m_timer(0)
{}

// Every 3 to 8 seconds the creature picks a new direction: towards its origin when it is more than
// 10 blocks away (squared planar distance exceeds 100), otherwise a random one, at 10 blocks per
// second.
bool RandomWalkBehavior::Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates)
{
    Creature::EntityData &entity = creature->m_entityData;
    entity.binaryToggles.fastMovement = 0;
    m_timer -= elapsed;
    if (m_timer < 0)
        m_timer = 0;
    if (m_timer == 0) {
        plasma::Vector<2, Fixed> offset;
        offset[0] = entity.position[0] - entity.origin[0];
        offset[1] = entity.position[1] - entity.origin[1];
        if (offset.squaredLength() > Fixed(100)) {
            entity.acceleration = entity.origin - entity.position;
            entity.acceleration[2] = 0;
            entity.acceleration.normalize();
            entity.acceleration *= 10.0f;
        } else {
            float x = rand() * 2.0f / float(RAND_MAX) - 1.0f;
            float y = rand() * 2.0f / float(RAND_MAX) - 1.0f;
            entity.acceleration = plasma::Vector<3, float>(x, y, 0.0f) * 10.0f;
        }
        m_timer = rand() % 5000 + 3000;
        entity.currentAbility = AbilityNone;
    }
    return true;
}

// A new behaviour with its own timer, not a copy of this one's.
Behavior *RandomWalkBehavior::clone()
{
    return new RandomWalkBehavior;
}

}
