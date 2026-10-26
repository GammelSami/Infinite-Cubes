#include "cube/WorldDimensions.h"
// WalkPathBehavior follows a closed path.
#include "cube/WalkPathBehavior.h"

#include <stdlib.h>
#include "cube/Creature.h"
#include "cube/DefaultBlocks.h"
#include "cube/World.h"

namespace cube {

WalkPathBehavior::WalkPathBehavior(const WalkPathBehavior &other)
    : m_path(other.m_path),
      m_next(other.m_next),
      m_timer(0),
      m_radius(other.m_radius)
{}

WalkPathBehavior::WalkPathBehavior(float radius)
    : m_next(0),
      m_radius(radius)
{}

// Waits while the timer runs, then walks to the next point of the path: the point is first moved
// onto the ground (down until the creature would collide there, then up until it no longer does;
// at most 20 blocks each way, else the point is skipped). On arrival the creature rests 2 to 6
// seconds and the next point is taken. Otherwise a path search is started towards the point, and
// continued while one runs. False when the path is empty or a point was skipped or reached.
bool WalkPathBehavior::Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates)
{
    if (m_path.empty())
        return false;
    m_timer -= elapsed;
    if (m_timer < 0)
        m_timer = 0;
    if (m_timer != 0)
        return true;
    if (m_next < 0)
        m_next = 0;
    m_next %= m_path.size();
    creature->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, Fixed> point = m_path[m_next];
    int count = 0;
    do {
        plasma::Vector<3, float> halfHeight(0.0f, 0.0f, creature->m_entityData.appearance.size[2] * 0.5f);
        if (world->isBlocked(point + plasma::Vector<3, Fixed>(halfHeight), creature->m_entityData.appearance.size,
                             false))
            break;
        point[2] -= Fixed(1);
    } while (++count <= 20);
    if (count > 20)
        goto next;
    count = 0;
    do {
        plasma::Vector<3, float> halfHeight(0.0f, 0.0f, creature->m_entityData.appearance.size[2] * 0.5f);
        if (!world->isBlocked(point + plasma::Vector<3, Fixed>(halfHeight), creature->m_entityData.appearance.size,
                              false))
            break;
        point[2] += Fixed(1);
    } while (++count <= 20);
    if (count > 20)
        goto next;
    {
        Creature::EntityData &entity = creature->m_entityData;
        plasma::Vector<3, float> halfHeight(0.0f, 0.0f, entity.appearance.size[2] * 0.5f);
        plasma::Vector<3, float> offset(entity.position - plasma::Vector<3, Fixed>(halfHeight) - point);
        if (m_radius * m_radius > offset.squaredLength()) {
            creature->clearPath();
            entity.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_timer = rand() % 4000 + 2000;
            goto next;
        }
        if (creature->m_path.size() == 0) {
            creature->clearPath();
            plasma::Vector<3, float> lift(0.0f, 0.0f, 0.1f - entity.appearance.size[2] * 0.5f);
            creature->m_pathStart = entity.position + plasma::Vector<3, Fixed>(lift);
            creature->m_pathGoal = point;
            if (!world->zone((int)creature->m_pathGoal[0] / BlocksPerZone,
                             (int)creature->m_pathGoal[1] / BlocksPerZone))
                return false;
            world->beginPathSearch(creature);
            creature->m_pathRadius = m_radius;
            world->continuePathSearch(creature);
            world->finishPathSearch(creature);
            entity.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        }
        if (creature->m_pathNodes.size() != 0) {
            for (int step = 0; step < PathSearchRoundsPerUpdate; step++) {
                if (creature->m_path.size() > PathSearchLengthLimit)
                    break;
                if (creature->m_path.size() != 0) {
                    plasma::Vector<3, int> offset = creature->m_path.back() - toBlock(creature->m_pathGoal);
                    if (creature->m_pathRadius * creature->m_pathRadius > (float)offset.squaredLength())
                        break;
                }
                world->continuePathSearch(creature);
                world->finishPathSearch(creature);
            }
        }
        entity.binaryToggles.fastMovement = 0;
        return true;
    }
next:
    m_next++;
    m_next %= m_path.size();
    return false;
}

Behavior *WalkPathBehavior::clone()
{
    return new WalkPathBehavior(*this);
}

}
