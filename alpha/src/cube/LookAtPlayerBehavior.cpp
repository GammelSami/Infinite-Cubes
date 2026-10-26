// LookAtPlayerBehavior turns creatures towards nearby players.
#include "cube/LookAtPlayerBehavior.h"

#include "cube/Creature.h"
#include "cube/DefaultBlocks.h"
#include "cube/World.h"

namespace cube {

LookAtPlayerBehavior::LookAtPlayerBehavior() {}

// Applies while a living player (hostility type 0) is within 8 blocks: the creature stops,
// abandons its path and faces the nearest one, unless its current ability is 83 or 84 (not yet
// identified).
bool LookAtPlayerBehavior::Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates)
{
    Creature *nearest = 0;
    float nearestDistance = 8.0f * 8.0f;
    for (std::map<long long, Creature *>::iterator it = world->m_creatures.begin(); it != world->m_creatures.end();
         ++it) {
        Creature *&other = it->second;
        if (other && other->m_entityData.hostilityType == HostilityPlayer && other->m_entityData.hp >= 0.0f) {
            plasma::Vector<3, float> offset(other->m_entityData.position - creature->m_entityData.position);
            float distance = offset.squaredLength();
            if (distance < nearestDistance) {
                nearest = other;
                nearestDistance = distance;
            }
        }
    }
    Creature::EntityData &entity = creature->m_entityData;
    entity.binaryToggles.faceAim = 0;
    if (nearest && entity.currentAbility != AbilitySit && entity.currentAbility != AbilitySleep) {
        entity.binaryToggles.faceAim = 1;
        entity.attackRotation = nearest->m_entityData.position - entity.position;
        entity.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        creature->clearPath();
        return true;
    }
    return false;
}

Behavior *LookAtPlayerBehavior::clone()
{
    return new LookAtPlayerBehavior;
}

}
