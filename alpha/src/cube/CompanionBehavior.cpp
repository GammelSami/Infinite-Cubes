#include "cube/WorldDimensions.h"
// CompanionBehavior controls pets following their owners.
#include "cube/CompanionBehavior.h"

#include "cube/Creature.h"
#include "cube/WorldBlocks.h"

namespace cube {
// A pet walking straight to its owner accelerates by OwnerFollowGain times the horizontal offset
// (at most 30) (anonymous: this file's own).
namespace {
enum {
    OwnerFollowGain = 5
};
}

CompanionBehavior::CompanionBehavior()
    : m_ownerId(0)
{}

CompanionBehavior::CompanionBehavior(long long ownerId)
    : m_ownerId(ownerId)
{}

// Follows the owner: runs (fast movement) beyond 8 blocks, stops within 4. A creature of hostility
// type 5 regains a tenth of its hit points per second. Where it can walk straight to the owner it
// does, at up to 30 blocks per second; otherwise a path search runs towards the ground below the
// owner (at most 20 blocks down, then up out of solid blocks). Does not apply while the owner is
// unknown, close, or above no ground.
bool CompanionBehavior::Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates)
{
    Creature *owner = world->creatureByID(m_ownerId);
    if (!owner)
        return false;
    Creature::EntityData &entity = creature->m_entityData;
    entity.binaryToggles.fastMovement = (owner->m_entityData.position - entity.position).squaredLength() > 8 * 8;
    if (entity.hostilityType == HostilityPet) {
        entity.hp += creature->maxHP() * (elapsed * 0.001f * 0.1f);
        if (entity.hp > creature->maxHP())
            entity.hp = creature->maxHP();
    }
    plasma::Vector<3, float> offset(owner->m_entityData.position - entity.position);
    entity.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    if (offset.squaredLength() < 4.0f * 4.0f)
        return false;
    if (world->canWalkStraight(creature, entity.position, owner->m_entityData.position,
                               entity.appearance.size[0] + owner->m_entityData.appearance.size[0])) {
        creature->clearPath();
        plasma::Vector<3, float> direction = plasma::Vector<3, float>(offset[0], offset[1], 0.0f) * OwnerFollowGain;
        if (direction.squaredLength() > 30.0f * 30.0f) {
            direction.normalize();
            direction *= 30.0f;
        }
        entity.acceleration = direction;
        return true;
    }
    plasma::Vector<3, Fixed> point = owner->m_entityData.position;
    int count = 0;
    while (!world->getBlock(point[0], point[1], point[2], 0).isSolid()) {
        point[2] = point[2] - 1;
        if (++count > 20)
            return false;
    }
    count = 0;
    do {
        if (!world->getBlock(point[0], point[1], point[2], 0).isSolid())
            break;
        point[2] = point[2] + 1;
    } while (++count <= 20);
    if (count > 20)
        return false;
    plasma::Vector<3, float> halfHeight(0.0f, 0.0f, entity.appearance.size[2] * 0.5f);
    plasma::Vector<3, float> remaining(entity.position - plasma::Vector<3, Fixed>(halfHeight) - point);
    if (remaining.squaredLength() < 4.0f * 4.0f) {
        creature->clearPath();
        entity.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        return false;
    }
    if (creature->m_path.size() == 0) {
        creature->clearPath();
        plasma::Vector<3, float> lift(0.0f, 0.0f, 0.1f - entity.appearance.size[2] * 0.5f);
        creature->m_pathStart = entity.position + plasma::Vector<3, Fixed>(lift);
        creature->m_pathGoal = point;
        if (!world->zone((int)creature->m_pathGoal[0] / BlocksPerZone, (int)creature->m_pathGoal[1] / BlocksPerZone))
            return false;
        world->beginPathSearch(creature);
        creature->m_pathRadius = PathGoalRadius;
        world->continuePathSearch(creature);
        world->finishPathSearch(creature);
        entity.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    }
    if (creature->m_pathNodes.size() != 0) {
        for (int step = 0; step < PathSearchRoundsPerUpdate; step++) {
            if (creature->m_path.size() > PathSearchLengthLimit)
                break;
            if (creature->m_path.size() != 0) {
                plasma::Vector<3, int> blockOffset = creature->m_path.back() - toBlock(creature->m_pathGoal);
                if (creature->m_pathRadius * creature->m_pathRadius > (float)blockOffset.squaredLength())
                    break;
            }
            world->continuePathSearch(creature);
            world->finishPathSearch(creature);
        }
    }
    return true;
}

Behavior *CompanionBehavior::clone()
{
    return new CompanionBehavior(*this);
}

}
