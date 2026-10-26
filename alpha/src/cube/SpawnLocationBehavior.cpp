// SpawnLocationBehavior follows the current entry of a daily schedule.
#include "cube/SpawnLocationBehavior.h"

#include "cube/Creature.h"
#include "cube/WorldBlocks.h"

namespace cube {
// The schedule's walks (anonymous: this file's own): the goal radius, the nearest player's search
// range and the path left when the entry counts as reached.
namespace {
enum {
    ScheduleGoalRadius = 4
};
enum {
    PlayerSearchRange = 200
};
enum {
    ScheduleArrivalPathLength = 3
};
}

SpawnLocationBehavior::SpawnLocationBehavior() {}

// The current entry of the schedule is the last one whose time of day has come. When it changes, a
// path search starts towards its place, moved onto the ground (up out of solid blocks, then down
// to the first solid one; at most 10 blocks each way, else the behaviour does not apply). While a
// player is within 256 blocks the search continues; once the path is found (or none can be), the
// creature stops and takes the place as its origin. Does not apply without a schedule or once the
// place is reached.
bool SpawnLocationBehavior::Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates)
{
    if (creature->m_schedule.empty())
        return false;
    int entry = 0;
    for (int i = 0; i < (int)creature->m_schedule.size(); i++) {
        if (world->m_time >= creature->m_schedule[i].time)
            entry = i;
    }
    if (entry != creature->m_scheduleEntry) {
        creature->clearPath();
        creature->m_scheduleEntry = entry;
        plasma::Vector<3, float> lift(0.0f, 0.0f, 0.5f - creature->m_entityData.appearance.size[2] * 0.5f);
        creature->m_pathStart = creature->m_entityData.position + plasma::Vector<3, Fixed>(lift);
        creature->m_pathGoal = creature->m_schedule[creature->m_scheduleEntry].position;
        // Up out of the ground, then down onto it: ten blocks at most each way.
        int count = 0;
        while (
            world->getBlock(creature->m_pathGoal[0], creature->m_pathGoal[1], creature->m_pathGoal[2], 0).isSolid()) {
            creature->m_pathGoal[2] += Fixed(1);
            if (++count > 10)
                return false;
        }
        count = 0;
        while (!world->getBlock(creature->m_pathGoal[0], creature->m_pathGoal[1], creature->m_pathGoal[2] - Fixed(1), 0)
                    .isSolid()) {
            creature->m_pathGoal[2] -= Fixed(1);
            if (++count > 10)
                return false;
        }
        world->beginPathSearch(creature);
        creature->m_pathRadius = ScheduleGoalRadius;
        world->continuePathSearch(creature);
        world->finishPathSearch(creature);
        creature->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        return true;
    }
    if (creature->m_scheduleReached == creature->m_scheduleEntry)
        return false;
    float nearestDistance = PlayerSearchRange * PlayerSearchRange;
    for (std::map<long long, Creature *>::iterator it = world->m_creatures.begin(); it != world->m_creatures.end();
         ++it) {
        Creature *&other = it->second;
        if (other && other->m_entityData.hostilityType == HostilityPlayer) {
            plasma::Vector<3, float> offset(other->m_entityData.position - creature->m_entityData.position);
            float distance = offset.squaredLength();
            if (distance < nearestDistance)
                nearestDistance = distance;
        }
    }
    if (nearestDistance < 256.0f * 256.0f) {
        if (creature->m_pathNodes.size() != 0) {
            for (int step = 0; step < PathSearchRoundsPerUpdate; step++) {
                if ((int)creature->m_path.size() > PathSearchLengthLimit)
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
        if ((creature->m_path.size() != 0 && creature->m_path.back() == toBlock(creature->m_pathGoal) &&
             creature->m_path.size() < ScheduleArrivalPathLength) ||
            creature->m_path.size() == 0) {
            creature->clearPath();
            creature->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            creature->m_scheduleReached = creature->m_scheduleEntry;
            creature->m_entityData.origin = creature->m_schedule[creature->m_scheduleEntry].position;
        }
    }
    return true;
}

Behavior *SpawnLocationBehavior::clone()
{
    return new SpawnLocationBehavior;
}

}
