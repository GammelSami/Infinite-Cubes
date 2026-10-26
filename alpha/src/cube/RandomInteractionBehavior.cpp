// RandomInteractionBehavior chooses nearby seats and beds for idle creatures.
#include "cube/DefaultBlocks.h"
#include "cube/RandomInteractionBehavior.h"
#include "cube/Creature.h"
#include "cube/CreatureIds.h"
#include "cube/House.h"
#include "cube/WorldBlocks.h"
#include <stdlib.h>
#include "cube/WorldDimensions.h"

namespace cube {
// The seats' use (anonymous: this file's own): the delay after a seat is left or chosen, the range
// within which seats are chosen and the distance at which one is used.
namespace {
enum {
    InteractionDelay = 20000
}; // milliseconds
enum {
    SeatSightRange = 200
};
enum {
    SeatUseDistance = 3
};
}

RandomInteractionBehavior::RandomInteractionBehavior()
    : m_timer(0)
{}

bool RandomInteractionBehavior::Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates)
{
    m_timer -= elapsed;
    if (m_timer < 0)
        m_timer = 0;
    Creature::EntityData &entity = creature->m_entityData;
    if (m_timer == 0) {
        if (entity.currentAbility == AbilitySit || entity.currentAbility == AbilitySleep) {
            creature->clearPath();
            creature->m_interactionTarget = plasma::Vector<3, int>(-1, -1, 0);
            entity.currentAbility = AbilityNone;
            entity.acceleration = plasma::Vector<3, float>(0, 0, 1);
            m_timer = InteractionDelay;
            return false;
        }
        entity.currentAbility = AbilityNone;
    }
    if (entity.currentAbility == AbilitySit || entity.currentAbility == AbilitySleep)
        return true;

    House *house = world->getHouse((int)entity.position[0], (int)entity.position[1], (int)entity.position[2]);
    bool hasTarget = false;
    if (house) {
        // Over the seats and beds by iterator.
        for (std::vector<plasma::Vector<3, int>>::iterator seat = house->seats.begin(); seat != house->seats.end();
             ++seat) {
            const plasma::Vector<3, int> &id = *seat;
            if (id == creature->m_interactionTarget) {
                Zone *zone = world->zone(id[0], id[1]);
                if (zone && id[2] >= 0 && id[2] < (int)zone->m_objects.size() && zone->m_objects[id[2]].occupantId == 0)
                    hasTarget = true;
            }
        }
        for (std::vector<plasma::Vector<3, int>>::iterator bed = house->beds.begin(); bed != house->beds.end(); ++bed) {
            const plasma::Vector<3, int> &id = *bed;
            if (id == creature->m_interactionTarget) {
                Zone *zone = world->zone(id[0], id[1]);
                if (zone && id[2] >= 0 && id[2] < (int)zone->m_objects.size() && zone->m_objects[id[2]].occupantId == 0)
                    hasTarget = true;
            }
        }
        if (!hasTarget && m_timer == 0) {
            std::vector<plasma::Vector<3, int>> &choices =
                world->m_time >= 6 * World::MillisecondsPerHour && world->m_time < 23 * World::MillisecondsPerHour
                    ? house->seats
                    : house->beds;
            if (!choices.empty()) {
                plasma::Vector<3, int> id = choices[rand() % choices.size()];
                Zone *zone = world->zone(id[0], id[1]);
                if (zone && id[2] >= 0 && id[2] < (int)zone->m_objects.size()) {
                    Zone::Object &object = zone->m_objects[id[2]];
                    if (object.occupantId == 0) {
                        m_timer = InteractionDelay;
                        plasma::Vector<3, float> lift(0, 0, 0.5f - entity.appearance.size[2] * 0.5f);
                        creature->m_pathStart = entity.position + plasma::Vector<3, Fixed>(lift);
                        creature->m_pathGoal = object.position;
                        creature->m_interactionTarget = id;
                        creature->m_pathRadius = PathGoalRadius;
                        world->beginPathSearch(creature);
                        hasTarget = true;
                    }
                }
            }
        }
    } else {
        int cellX = (int)entity.position[0] / BlocksPerObjectCell;
        int cellY = (int)entity.position[1] / BlocksPerObjectCell;
        for (int x = cellX - 1; x < cellX + 1; ++x) {
            for (int y = cellY - 1; y < cellY + 1; ++y) {
                if (x < 0 || y < 0 || x >= ObjectCellsPerWorld || y >= ObjectCellsPerWorld)
                    continue;
                Zone *zone = world->zone(x / ObjectCellsPerZone, y / ObjectCellsPerZone);
                if (!zone)
                    continue;
                std::list<Zone::CellRecord> *cell =
                    &zone->m_cells[x % ObjectCellsPerZone + (y % ObjectCellsPerZone) * ObjectCellsPerZone];
                if (!cell)
                    continue;
                for (std::list<Zone::CellRecord>::iterator it = cell->begin(); it != cell->end(); ++it) {
                    if (it->id == creature->m_interactionTarget && it->object->occupantId == 0)
                        hasTarget = true;
                }
            }
        }
        if (!hasTarget && m_timer == 0) {
            std::vector<plasma::Vector<3, int>> choices;
            for (int x = cellX - 1; x < cellX + 1; ++x) {
                for (int y = cellY - 1; y < cellY + 1; ++y) {
                    if (x < 0 || y < 0 || x >= ObjectCellsPerWorld || y >= ObjectCellsPerWorld)
                        continue;
                    Zone *zone = world->zone(x / ObjectCellsPerZone, y / ObjectCellsPerZone);
                    if (!zone)
                        continue;
                    std::list<Zone::CellRecord> *cell =
                        &zone->m_cells[x % ObjectCellsPerZone + (y % ObjectCellsPerZone) * ObjectCellsPerZone];
                    if (!cell)
                        continue;
                    for (std::list<Zone::CellRecord>::iterator it = cell->begin(); it != cell->end(); ++it) {
                        Zone::Object *object = it->object;
                        if ((object->type == StaticObjectBench || object->type == StaticObjectStool ||
                             object->type == StaticObjectBeachTowel) &&
                            world->inRange(entity.position, object->position, true, SeatSightRange))
                            choices.push_back(it->id);
                    }
                }
            }
            if (!choices.empty()) {
                plasma::Vector<3, int> id = choices[rand() % choices.size()];
                Zone *zone = world->zone(id[0], id[1]);
                if (zone && id[2] >= 0 && id[2] < (int)zone->m_objects.size()) {
                    Zone::Object &object = zone->m_objects[id[2]];
                    if (object.occupantId == 0) {
                        m_timer = InteractionDelay;
                        hasTarget = true;
                        plasma::Vector<3, float> lift(0, 0, 0.5f - entity.appearance.size[2] * 0.5f);
                        creature->m_pathStart = entity.position + plasma::Vector<3, Fixed>(lift);
                        creature->m_pathGoal = object.position;
                        while (
                            world
                                ->getBlock(creature->m_pathGoal[0], creature->m_pathGoal[1], creature->m_pathGoal[2], 0)
                                .isSolid())
                            creature->m_pathGoal[2] += Fixed(1);
                        while (!world
                                    ->getBlock(creature->m_pathGoal[0], creature->m_pathGoal[1],
                                               creature->m_pathGoal[2] - 1, 0)
                                    .isSolid())
                            creature->m_pathGoal[2] += Fixed(-1);
                        creature->m_interactionTarget = id;
                        creature->m_pathRadius = PathGoalRadius;
                        world->beginPathSearch(creature);
                    }
                }
            }
        }
    }
    if (!hasTarget) {
        creature->clearPath();
        creature->m_interactionTarget = plasma::Vector<3, int>(-1, -1, 0);
        return false;
    }
    if (!creature->m_pathNodes.empty()) {
        for (int step = 0; step < PathSearchRoundsPerUpdate; ++step) {
            if (creature->m_path.size() > PathSearchLengthLimit)
                break;
            if (!creature->m_path.empty()) {
                plasma::Vector<3, int> offset = creature->m_path.back() - toBlock(creature->m_pathGoal);
                if (creature->m_pathRadius * creature->m_pathRadius > (float)offset.squaredLength())
                    break;
            }
            world->continuePathSearch(creature);
            world->finishPathSearch(creature);
        }
    }
    const plasma::Vector<3, int> &id = creature->m_interactionTarget;
    Zone *zone = world->zone(id[0], id[1]);
    if (zone && id[2] >= 0 && id[2] < (int)zone->m_objects.size()) {
        plasma::Vector<3, float> offset(zone->m_objects[id[2]].position - entity.position);
        if (offset.squaredLength() < SeatUseDistance * SeatUseDistance) {
            Interaction interaction;
            interaction.position = id;
            interaction.type = InteractionUseObject;
            creature->m_interactions.push_back(interaction);
            creature->clearPath();
            creature->m_interactionTarget = plasma::Vector<3, int>(-1, -1, 0);
            entity.acceleration = plasma::Vector<3, float>(0, 0, 0);
            entity.velocity = plasma::Vector<3, float>(0, 0, 0);
            m_timer = InteractionDelay;
        }
    }
    return true;
}

Behavior *RandomInteractionBehavior::clone()
{
    return new RandomInteractionBehavior;
}

}
