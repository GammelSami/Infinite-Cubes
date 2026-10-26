#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
// Creature AI navigation, support, combat and update operations.
#include "cube/Noise.h"
#include "cube/World.h"
#include "cube/Sight.h"
#include <iostream>
#include "cube/CreatureIds.h"
#include "cube/BlockIds.h"
#include "cube/ItemIds.h"
#include "cube/SpriteIds.h"
#include "cube/ZoneIds.h"

namespace cube {

enum AiAbilityMembership : unsigned long long {
    AiTimeoutExemptAbilities = (1ULL << (AbilitySneak - AbilitySneak)) |
                               (1ULL << (AbilityOfferPetFood - AbilitySneak)) | (1ULL << (AbilitySit - AbilitySneak)) |
                               (1ULL << (AbilitySleep - AbilitySneak)) | (1ULL << (AbilityLieDown - AbilitySneak)) |
                               (1ULL << (AbilityClimb - AbilitySneak)) | (1ULL << (AbilitySlowFall - AbilitySneak)),
    AiFixedRangeSpecialAbilities =
        (1ULL << (AbilityHeadbutt - AbilityHeadbutt)) | (1ULL << (AbilityHornCharge - AbilityHeadbutt)) |
        (1ULL << (AbilityQuake - AbilityHeadbutt)) | (1ULL << (AbilityLunge - AbilityHeadbutt)),
    AiDodgeBlockingAbilities = (1ULL << (AbilityBlock - AbilityBlock)) | (1ULL << (AbilityCrush - AbilityBlock)) |
                               (1ULL << (AbilitySwirl - AbilityBlock)) |
                               (1ULL << (AbilityMonsterCharge - AbilityBlock)),
    AiChargingAbilities = (1ULL << (AbilityBlock - AbilityBlock)) | (1ULL << (AbilityChargedShot - AbilityBlock)) |
                          (1ULL << (AbilitySalvo - AbilityBlock)) | (1ULL << (AbilityChargedBoomerang - AbilityBlock)) |
                          (1ULL << (AbilityCrush - AbilityBlock)) | (1ULL << (AbilitySwirl - AbilityBlock)),
};

enum AiAbilityIntervalMask {
    AiFourAbilityIntervalMask = 0xfc
};

namespace {
// Times are in milliseconds, distances in blocks. A creature's special skills wait this long after one is used (or
// skipped), and up to this long after losing its target.
const int SpecialSkillDelay = 10000;
// The path search's nodes per update: halved and grown as walks succeed or fail, between these bounds; a player AI
// searches this many at once.
const int MinSearchBudget = 50;
const int MaxSearchBudget = 500;
const int PlayerAiSearchBudget = 2000;
// Walks and paths start this far above a creature's feet.
const float FootLift = 0.1f;
// canWalk tests a straight walk up to this far.
const float MaxWalkProbeDistance = 50.0f;
// idle: creatures lower than this (but the master glider) fallen off a ledge head back home; the acceleration of that
// and of wandering.
const float HomeRecoveryMaxHeight = 6.0f;
const float IdleAcceleration = 10.0f;
// idle: the squared distance (in Fixed units of blocks squared) from home beyond which a creature walks back (32
// blocks; 4 for a sleeper).
const int HomeLeashSquared = 1024;
const int SleepLeashSquared = 16;
// How near a creature walks to its home, roam point or furniture.
const float ApproachRange = 5.0f;
// idle: furniture is looked for in the zones this many blocks around.
const int FurnitureScanRadius = 64;
// idle: the behaviours' durations, a base plus a random spread.
const int IdleWaitTime = 1000;
const int IdleWaitSpread = 2000;
const int WanderTime = 3000;
const int WanderSpread = 5000;
const int RestTime = 5000;
const int RestSpread = 8000;
const int RoamTime = 20000;
const int RoamSpread = 10000;
// idle: a roaming creature picks a cell up to this many cells away.
const int RoamCellRadius = 5;
// The master glider's take-off speed.
const float GliderLaunchSpeed = 20.0f;
// watchOrGreetPlayer: players within ten blocks (squared) are watched.
const float PlayerAttentionRangeSquared = 100.0f;
// The range of canSee for noticing players and enemies.
const float SightRange = 20.0f;
// targetSkill: enemies within five blocks (squared) are attached to.
const float SkillTargetRangeSquared = 25.0f;
// The probe for a followed creature's next step: this far ahead of it, searched this many blocks down and up for a
// floor.
const float ProbeAheadDistance = 2.0f;
const int ProbeSearchDepth = 3;
// update: the acceleration along a path; the range of canWalk for its steps; the steps before an unsupported one that
// slow the creature (to a tenth) or make it climb.
const float PathAcceleration = 80.0f;
const float PathStepRange = 2.0f;
const int SupportLookahead = 4;
const float UnsupportedStepSlowdown = 0.1f;
const float ClimbAcceleration = 20.0f;
// A player AI follows a found path only when it is longer than this.
const int MinPlayerAiPathLength = 10;
// A creature within five blocks (squared) of its furniture uses it.
const float InteractionRangeSquared = 25.0f;
// A mission resident speaks to a newly seen player at most this often.
const int ResidentTalkCooldown = 4000;
// Humanoids carry their lamps lit outside these hours.
const int LampOffHour = 7;
const int LampOnHour = 20;
// The range of the line-of-sight test to a target or an owner.
const float CombatSightRange = 200.0f;
// checkSupport: a supporter stays within this range of its owner.
const float SupportRange = 30.0f;
// A chase is given up after this long, and the next target is looked for after this delay.
const int ChaseTimeout = 15000;
const int RetargetDelay = 3000;
// checkSupport: the support buff (owner's maximum HP times 1.1 for three seconds), renewed once less than half a
// second remains.
const int SupportBuffRenewTime = 500;
const float SupportBuffStrength = 1.1f;
const int SupportBuffDuration = 3000;
// A pet called by its owner joins it (from beyond 100 blocks, squared) while the call is this young.
const int PetCallTeleportTime = 500;
const int PetCallTimeLimit = 1000;
const float PetTeleportDistanceSquared = 10000.0f;
// A pet regenerates this fraction of its maximum HP per second.
const float PetRegenerationRate = 0.1f;
// escort, handleTarget and mirrorOwnerActions: the squared distance beyond which a follower runs, within which it
// stops, and its goTo range.
const float EscortSprintDistanceSquared = 64.0f;
const float EscortStopDistanceSquared = 16.0f;
const float EscortRange = 4.0f;
const float FollowerSprintDistanceSquared = 256.0f;
const float FollowerStopDistanceSquared = 64.0f;
const float FollowerRange = 8.0f;
const int MirrorSprintDistanceSquared = 64;
const float MirrorStopDistanceSquared = 25.0f;
const float MirrorRange = 5.0f;
// combat: an unfinished ability is dropped after this long; drinking and eating hold for these times.
const float StaleAbilityTimeout = 10000.0f;
const float DrinkTime = 3000.0f;
const float EatTime = 5000.0f;
// combat: a life potion is drunk below this fraction of the maximum HP.
const float PotionHealthFraction = 0.25f;
// combat: the least aggro a preferred or new target has.
const float MinTargetAggro = 0.1f;
// combat: a wraith without a target returns home from 30 blocks (squared).
const float WraithLeashSquared = 900.0f;
// combat: attack ranges: ranged attacks and rays, fire breath and riding a mana mech, the fixed-range special skills.
const float RangedAttackRange = 50.0f;
const float CloseAttackRange = 4.0f;
const float SpecialSkillRange = 20.0f;
// combat: offered pet food is eaten this many times before taming anger sets in, for this long.
const int PetFeedingCount = 5;
const float TamingAngerDuration = 20000.0f;
// combat: a smash leaps at this speed (its duration from the distance) and this upward speed.
const float SmashFlightSpeed = 50.0f;
const float SmashLaunchSpeed = 20.0f;
// combat: casters, dragons and ranged weapons attack within 60 blocks (squared).
const float RangedRangeSquared = 3600.0f;
// combat: melee attacks within three times the two widths.
const float MeleeRangeFactor = 3.0f;
// combat: one recovered creature in this many, narrower than DodgeMaxWidth, dodges: at this speed, with this lift,
// rolling for this long.
const int DodgeOdds = 10;
const float DodgeMaxWidth = 2.0f;
const float DodgeSpeed = 20.0f;
const float DodgeLift = 5.0f;
const float DodgeRollTime = 600.0f;
// Creatures with little mana use a plain attack; ranged charges release at the smaller of 0.8 and this fraction of
// mana.
const double ChargeManaThreshold = 0.8;
const float RangedChargeManaFraction = 0.9f;
}

inline World::AIManager::PathNode::PathNode()
{
    // Leaves all path-record fields uninitialized on map insertion.
}

World::AIManager::AIManager(World *world)
    : m_world(world)
{}

World::AIManager::State::State()
    : idleTime(0),
      chaseTime(0),
      retargetDelay(0),
      playerInSight(false),
      wasPassive(false),
      lastSightingTime(0),
      preferredTargetId(0),
      specialSkillDelay(SpecialSkillDelay),
      acceleration(0, 0, 0),
      attackRotation(0, 0, 0),
      unusedCell(-1, -1),
      probeAhead(false),
      moving(false),
      unsupportedStep(false),
      inSpiritWorld(false),
      searchBudget(MinSearchBudget),
      speed(0),
      flags(0),
      binaryToggles(0),
      position(0, 0, 0),
      size(1, 1, 1),
      best(),
      startBlock(0, 0, 0),
      targetId(0)
{
    // Leaves the path endpoints, range, unused108 and furnitureObjectKey uninitialized.
}

World::AIManager::State::~State() {}

float blendFactor(double frameTime, float rate)
{
    // Steps of ten milliseconds, each moving the value towards the step's share of the frame by rate * 0.02.
    int steps = (int)(frameTime * 100.0);
    double value = 0.0;
    for (int i = 0; i < steps; i++)
        value += ((i + 1.0) / steps - value) * (rate * 0.02);
    return value;
}

// Whether a block stops a walker: anything but air and water, and the blocks with BlockFlagBarrier that lack
// BlockFlagPassableBarrier, unless the walker passes breakables and the block also has BlockFlagSpiritPassable. The
// callers apply it to the block getBlock returns.
static inline bool blocksWalker(const Block &block, bool passSpiritPassableBarriers)
{
    if (block.isSolid())
        return true;
    if (block.flags & BlockFlagBarrier) {
        if (block.flags & BlockFlagPassableBarrier)
            return false;
        if (!passSpiritPassableBarriers || !(block.flags & BlockFlagSpiritPassable))
            return true;
    }
    return false;
}

bool World::AIManager::canWalk(const plasma::Vector<3, float> &size, const plasma::Vector<3, Fixed> &from,
                               const plasma::Vector<3, Fixed> &to, float range, bool passSpiritPassableBarriers,
                               unsigned int toggles)
{
    int z = (from[2] + Fixed(FootLift - size[2] * 0.5f)).toBlock();
    int y = from[1].toBlock();
    int x = from[0].toBlock();
    plasma::Vector<3, int> block(x, y, z);
    plasma::Vector<3, float> direction((float)(to[0] - from[0]), (float)(to[1] - from[1]), 0.0f);
    float length = sqrtf(direction.squaredLength());
    direction /= length;
    float distance = std::min(MaxWalkProbeDistance, length);
    float travelled = 0.0f;
    bool supported = false;
    while (travelled < distance) {
        Sleep(0);
        plasma::Vector<3, Fixed> position = from;
        position[0] += Fixed(direction[0] * travelled);
        position[1] += Fixed(direction[1] * travelled);
        double halfX = (double)size[0] * 0.5;
        double halfY = (double)size[1] * 0.5;
        int bottom = Fixed(block[2]).toBlock();
        int minY = (position[1] - Fixed(halfY)).toBlock();
        int minX = (position[0] - Fixed(halfX)).toBlock();
        int top = Fixed((float)block[2] + size[2] + 1.0f).toBlock();
        int maxY = (position[1] + halfY).toBlock();
        int maxX = (position[0] + halfX).toBlock();
        if (travelled > 0.0f) {
            for (int bx = minX; bx <= maxX; ++bx)
                for (int by = minY; by <= maxY; ++by)
                    for (int bz = bottom + 1; bz <= top; ++bz) {
                        if (blocksWalker(m_world->getBlock(bx, by, bz), passSpiritPassableBarriers))
                            return false;
                    }
        }
        bool stepUp = false;
        for (int bx = minX; bx <= maxX; ++bx)
            for (int by = minY; by <= maxY; ++by) {
                if (m_world->getBlock(bx, by, block[2]).isSolid()) {
                    for (int bz = block[2] + 1; bz < top - bottom + block[2] + 1; ++bz) {
                        Block cube = m_world->getBlock(bx, by, bz);
                        if ((cube.isSolid() ||
                             ((cube.flags & BlockFlagBarrier) && !(cube.flags & BlockFlagPassableBarrier) &&
                              (!passSpiritPassableBarriers || !(cube.flags & BlockFlagSpiritPassable)))) &&
                            travelled > 0.0f)
                            return false;
                    }
                    stepUp = true;
                }
            }
        if (stepUp)
            ++block[2];
        supported = false;
        bool closeGround = false;
        for (int bx = minX; bx <= maxX; ++bx)
            for (int by = minY; by <= maxY; ++by) {
                if (m_world->getBlock(bx, by, block[2] - 1).isSolid() ||
                    m_world->getBlock(bx, by, block[2]).type == BlockWater)
                    supported = true;
                if ((toggles & (1 << CreatureTogglePlayerAi)) && m_world->getBlock(bx, by, block[2] - 2).isSolid())
                    closeGround = true;
            }
        if ((toggles & (1 << CreatureTogglePlayerAi)) && !closeGround)
            return false;
        if (m_world->getBlock(block[0], block[1], block[2]).type == BlockWater) {
            int centreHeight = (int)(size[2] * 0.5f + (float)block[2]);
            long long targetHeight = to[2].m_raw / FixedUnitsPerBlock;
            if (centreHeight > targetHeight)
                --block[2];
            else if (centreHeight < targetHeight)
                ++block[2];
        } else if (!supported) {
            --block[2];
        }
        float advance = RayStepBound;
        int axis = 0;
        for (int i = 0; i < 2; ++i) {
            if (direction[i] * direction[i] < RayAxisEpsilon)
                continue;
            float step;
            if (direction[i] > 0.0f)
                step = (float)(Fixed(block[i] + 1) - position[i]) / direction[i];
            else
                step = (float)(Fixed(block[i]) - position[i]) / direction[i];
            if (advance > step) {
                axis = i;
                advance = step;
            }
        }
        if (direction[axis] > 0.0f)
            ++block[axis];
        else
            --block[axis];
        travelled += advance;
    }
    plasma::Vector<3, Fixed> end(Fixed(block[0]) + 0.5, Fixed((double)block[1] + 0.5),
                                 Fixed(size[2] * 0.5f + (float)block[2]));
    if (supported || m_world->getBlock(block[0], block[1], block[2]).type == BlockWater) {
        float dx = (float)(end[0] - to[0]);
        float dy = (float)(end[1] - to[1]);
        if (dy * dy + dx * dx > range * range || inRange(end, to, range))
            return true;
    }
    return false;
}

void World::AIManager::State::clearPath()
{
    nodes.clear();
    frontier.clear();
    path.clear();
    best = nodes.end();
    objectTargets.clear();
}

void World::AIManager::finishPathSearch(State &state)
{
    plasma::Vector<3, int> start = fixedToBlock(state.start);
    plasma::Vector<3, int> destination = fixedToBlock(state.destination);
    state.path.clear();
    plasma::Vector<3, int> position = destination;
    if (state.best != state.nodes.end())
        position = plasma::Vector<3, int>(state.best->first[0], state.best->first[1], state.best->first[2]);
    state.path.push_front(position);
    int count = 0;
    while (position != start) {
        if (count > (int)state.nodes.size())
            break;
        ++count;
        PathNode *node = state.pathNode(position[0], position[1], position[2]);
        if (!node)
            break;
        plasma::Vector<3, int> previous = node->parent;
        if (position == previous)
            break;
        position = previous;
        state.path.push_front(previous);
    }
}

bool World::AIManager::idle(Creature *creature, int milliseconds)
{
    if (creature->m_entityData.binaryToggles.active || creature->m_entityData.binaryToggles.noTerrainCollision)
        return false;
    creature->m_entityData.setToggles(1 << CreatureToggleFastMovement, false);
    State &state = m_states[creature->m_id];
    state.idleTime -= milliseconds;
    if (state.idleTime < 0)
        state.idleTime = 0;
    if (creature->m_entityData.appearance.size[2] < HomeRecoveryMaxHeight &&
        creature->m_entityData.classType != ClassMasterGlider &&
        (creature->m_entityData.flags & CreatureFlagOnGround)) {
        plasma::Vector<3, Fixed> bottom =
            creature->m_entityData.position - plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                  0.0f, 0.0f, creature->m_entityData.appearance.size[2] * 0.5f));
        if (!m_world->getBlockAtDot(bottom[0], bottom[1], bottom[2]).isSolid() &&
            !m_world->getBlockAtDot(bottom[0], bottom[1], bottom[2] - Fixed(1)).isSolid() &&
            !m_world->getBlockAtDot(bottom[0], bottom[1], bottom[2] - Fixed(2)).isSolid()) {
            plasma::Vector<3, float> direction(creature->m_entityData.origin - creature->m_entityData.position);
            if (direction.squaredLength() > 0.0f) {
                direction[2] = 0.0f;
                if (direction.squaredLength() > 0.01f) {
                    direction *= 1.0f / sqrtf(direction.squaredLength());
                    state.acceleration = direction * IdleAcceleration;
                    creature->m_entityData.velocity = plasma::Vector<3, float>(0, 0, 0);
                }
            }
        }
    }
    unsigned int behaviours = creature->m_behaviours;
    bool sleeps = (behaviours & BehaviourSleep) != 0;
    Fixed dx = creature->m_entityData.position[0] - creature->m_entityData.origin[0];
    Fixed dy = creature->m_entityData.position[1] - creature->m_entityData.origin[1];
    // Squares signed dot differences without guarding overflow.
    if (dx * dx + dy * dy > Fixed(HomeLeashSquared) || (sleeps && dx * dx + dy * dy > Fixed(SleepLeashSquared))) {
        state.destination = creature->m_entityData.origin;
        state.start = state.position;
        state.start[2] -= Fixed(state.size[2] * 0.5f - FootLift);
        state.range = ApproachRange;
        state.speed = 0.0f;
        startPath(state);
        if (!creature->m_entityData.currentAbility) {
            if (creature->m_entityData.classType == ClassMasterSailor &&
                (creature->m_entityData.flags & CreatureFlagInWater)) {
                Creature::EntityData &entity = creature->m_entityData;
                entity.currentAbility = AbilitySailBoat;
                entity.timeSinceAbility = 0.0f;
                entity.actionItem.category = ItemCategorySpecial;
                entity.actionItem.id = SpecialItemBoat;
                unsigned int modifier = (unsigned int)creature->m_id;
                modifier ^= modifier << 13;
                modifier ^= modifier >> 17;
                modifier ^= modifier << 5;
                entity.actionItem.modifier = modifier % 10 + 1;
            }
            if (!creature->m_entityData.currentAbility && creature->m_entityData.classType == ClassMasterGlider &&
                !(creature->m_entityData.flags & CreatureFlagInWater)) {
                creature->m_entityData.currentAbility = AbilityHangGlide;
                creature->m_entityData.timeSinceAbility = 0.0f;
                creature->m_entityData.actionItem.category = ItemCategorySpecial;
                creature->m_entityData.actionItem.id = SpecialItemHangGlider;
                unsigned int modifier = (unsigned int)creature->m_id;
                modifier ^= modifier << 13;
                modifier ^= modifier >> 17;
                modifier ^= modifier << 5;
                creature->m_entityData.actionItem.modifier = modifier % 10 + 1;
            }
        }
    } else if (!state.idleTime && (creature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater))) {
        if (sleeps) {
            state.acceleration = plasma::Vector<3, float>(0, 0, 0);
            creature->m_entityData.currentAbility = AbilitySleep;
            creature->m_entityData.binaryToggles.active = creature->m_entityData.binaryToggles.faceAim = false;
            creature->m_entityData.binaryToggles.toggle3 = creature->m_entityData.binaryToggles.noTerrainCollision =
                false;
            creature->m_entityData.binaryToggles.combatSideOverride =
                creature->m_entityData.binaryToggles.fastMovement = false;
            creature->m_entityData.binaryToggles.dodge = creature->m_entityData.binaryToggles.missionComplete = false;
            creature->m_entityData.binaryToggles.playerAi = creature->m_entityData.binaryToggles.leanForwardPose =
                false;
            creature->m_entityData.binaryToggles.suppressResidentPromotion =
                creature->m_entityData.binaryToggles.wraithTargeting = false;
            creature->m_entityData.binaryToggles.attachedToFurniture =
                creature->m_entityData.binaryToggles.magicBarriersDisabled = false;
            creature->m_entityData.binaryToggles.removePetrification = false;
            return true;
        }
        std::vector<int> choices;
        std::vector<plasma::Vector<3, int>> objects;
        if (behaviours & BehaviourUseFurniture) {
            int minX = (int)(creature->m_entityData.position[0] - Fixed(FurnitureScanRadius)) / BlocksPerZone;
            int maxX = (int)(creature->m_entityData.position[0] + Fixed(FurnitureScanRadius)) / BlocksPerZone;
            int minY = (int)(creature->m_entityData.position[1] - Fixed(FurnitureScanRadius)) / BlocksPerZone;
            int maxY = (int)(creature->m_entityData.position[1] + Fixed(FurnitureScanRadius)) / BlocksPerZone;
            for (int x = minX; x <= maxX; ++x)
                for (int y = minY; y <= maxY; ++y) {
                    Zone *zone = m_world->zone(x, y);
                    if (zone) {
                        for (int index = 0; index < (int)zone->m_staticEntities.size(); ++index) {
                            const StaticEntity &object = zone->m_staticEntities[index];
                            int type = object.type;
                            if (!object.occupantId &&
                                (type == StaticEntityStool || type == StaticEntityBeachTowel ||
                                 type == StaticEntityWoodMat || type == StaticEntityBench ||
                                 type == StaticEntityBenchBlack || type == StaticEntityDruidChair)) {
                                plasma::Vector<3, int> id(x, y, index);
                                objects.push_back(id);
                            }
                        }
                    }
                }
            if (!objects.empty())
                choices.push_back(BehaviourUseFurniture);
        }
        if (creature->m_behaviours & BehaviourWander) {
            int choice = BehaviourWander;
            choices.push_back(choice);
        }
        if (creature->m_behaviours & BehaviourSit) {
            int choice = BehaviourSit;
            choices.push_back(choice);
        }
        if (creature->m_behaviours & BehaviourRestHeal) {
            int choice = BehaviourRestHeal;
            choices.push_back(choice);
        }
        if (creature->m_behaviours & BehaviourStand)
            choices.push_back(BehaviourStand);
        if (creature->m_behaviours & BehaviourRoam)
            choices.push_back(BehaviourRoam);
        if (choices.empty()) {
            state.acceleration = plasma::Vector<3, float>(0, 0, 0);
            state.idleTime = rand() % IdleWaitSpread + IdleWaitTime;
            creature->m_entityData.currentAbility = AbilityNone;
        } else {
            switch (choices[rand() % choices.size()]) {
            case BehaviourWander: {
                creature->m_entityData.binaryToggles.fastMovement = false;
                int sample = rand();
                float y = (float)(sample + sample) / (float)RAND_MAX - 1.0f;
                sample = rand();
                state.acceleration[0] = ((float)(sample + sample) / (float)RAND_MAX - 1.0f) * IdleAcceleration;
                state.acceleration[1] = y * IdleAcceleration;
                state.acceleration[2] = 0.0f;
                state.idleTime = rand() % WanderSpread + WanderTime;
                creature->m_entityData.currentAbility = AbilityNone;
                break;
            }
            case BehaviourSit:
                state.acceleration = plasma::Vector<3, float>(0, 0, 0);
                state.idleTime = rand() % RestSpread + RestTime;
                creature->m_entityData.currentAbility = AbilitySit;
                break;
            case BehaviourStand:
                state.acceleration = plasma::Vector<3, float>(0, 0, 0);
                state.idleTime = rand() % RestSpread + RestTime;
                creature->m_entityData.currentAbility = AbilityNone;
                break;
            case BehaviourUseFurniture:
                creature->m_entityData.binaryToggles.fastMovement = false;
                creature->m_entityData.currentAbility = AbilityNone;
                if (!objects.empty()) {
                    state.idleTime = rand() % RestSpread + RestTime;
                    plasma::Vector<3, int> id = objects[rand() % objects.size()];
                    StaticEntity *object = m_world->object(id);
                    if (object) {
                        if (m_world->inRange(creature->m_entityData.position, object->position, false, ApproachRange,
                                             nullptr)) {
                            Interaction interaction;
                            interaction.type = InteractionTalk;
                            interaction.position = id;
                            creature->m_interactions.push_back(interaction);
                            creature->m_entityData.acceleration = plasma::Vector<3, float>(0, 0, 0);
                            state.acceleration = plasma::Vector<3, float>(0, 0, 0);
                        } else {
                            state.destination = object->position;
                            state.start = state.position;
                            state.start[2] -= Fixed(state.size[2] * 0.5f - FootLift);
                            state.range = ApproachRange;
                            state.objectTargets.insert(object);
                            state.furnitureObjectKey = id;
                            state.speed = 0.0f;
                            startPath(state);
                        }
                    }
                }
                break;
            case BehaviourRoam: {
                int offsetX = rand() % (2 * RoamCellRadius + 1) - RoamCellRadius;
                int cellX = tileCoordinate(creature->m_entityData.position[0]) + offsetX;
                int offsetY = rand() % (2 * RoamCellRadius + 1) - RoamCellRadius;
                int cellY = tileCoordinate(creature->m_entityData.position[1]) + offsetY;
                // Multiplies cell coordinates in 32 bits before zone division.
                Zone *zone = m_world->zone(floorDivide(cellX * BlocksPerCell, BlocksPerZone),
                                           floorDivide(cellY * BlocksPerCell, BlocksPerZone));
                if (zone) {
                    creature->m_entityData.binaryToggles.fastMovement = false;
                    state.idleTime = rand() % RoamSpread + RoamTime;
                    creature->m_entityData.currentAbility = AbilityNone;
                    state.destination = zone->randomSpawnPosition(cellX, cellY);
                    state.start = state.position;
                    state.start[2] -= Fixed(state.size[2] * 0.5f - FootLift);
                    state.range = ApproachRange;
                    state.speed = 0.0f;
                    startPath(state);
                }
                break;
            }
            case BehaviourRestHeal:
                state.acceleration = plasma::Vector<3, float>(0, 0, 0);
                state.idleTime = rand() % RestSpread + RestTime;
                creature->m_entityData.currentAbility = AbilityRestHeal;
                creature->m_entityData.timeSinceAbility = 0.0f;
                break;
            }
        }
        if (!creature->m_entityData.currentAbility) {
            if (creature->m_entityData.classType == ClassMasterSailor &&
                (creature->m_entityData.flags & CreatureFlagInWater)) {
                creature->m_entityData.currentAbility = AbilitySailBoat;
                creature->m_entityData.timeSinceAbility = 0.0f;
                creature->m_entityData.actionItem.category = ItemCategorySpecial;
                creature->m_entityData.actionItem.id = SpecialItemBoat;
                unsigned int modifier = (unsigned int)creature->m_id;
                modifier ^= modifier << 13;
                modifier ^= modifier >> 17;
                modifier ^= modifier << 5;
                creature->m_entityData.actionItem.modifier = modifier % 10 + 1;
            }
            if (!creature->m_entityData.currentAbility && creature->m_entityData.classType == ClassMasterGlider &&
                (creature->m_entityData.flags & CreatureFlagOnGround)) {
                creature->m_entityData.currentAbility = AbilityHangGlide;
                creature->m_entityData.timeSinceAbility = 0.0f;
                creature->m_entityData.actionItem.category = ItemCategorySpecial;
                creature->m_entityData.actionItem.id = SpecialItemHangGlider;
                unsigned int modifier = (unsigned int)creature->m_id;
                modifier ^= modifier << 13;
                modifier ^= modifier >> 17;
                modifier ^= modifier << 5;
                creature->m_entityData.actionItem.modifier = modifier % 10 + 1;
                creature->m_entityData.velocity[2] = GliderLaunchSpeed;
            }
        }
        if (creature->m_entityData.currentAbility == AbilityHangGlide)
            creature->m_entityData.binaryToggles.fastMovement = true;
    }
    return true;
}

bool World::AIManager::inRange(const plasma::Vector<3, Fixed> &from, const plasma::Vector<3, Fixed> &to, float range)
{
    plasma::Vector<3, float> direction(to - from);
    float squared = direction.squaredLength();
    // Points that (nearly) coincide are in range; the ray may stop a hundredth of a block short.
    if (squared < 0.0001f)
        return true;
    if (squared > range * range)
        return false;
    float length = sqrtf(squared);
    direction /= length;
    return rayLength(from, direction, length, true) >= length - 0.01f;
}

bool World::AIManager::State::isOpen(int x, int y, int z)
{
    return frontier.find(IntTriple {{x, y, z}}) != frontier.end();
}

bool World::AIManager::watchOrGreetPlayer(Creature *creature, int milliseconds)
{
    Creature::EntityData &entity = creature->m_entityData;
    if (entity.binaryToggles.active || entity.binaryToggles.noTerrainCollision)
        return false;
    Creature *target = nullptr;
    float nearest = PlayerAttentionRangeSquared;
    for (auto other : m_world->m_creatures) {
        if (other->m_entityData.hostilityType == HostilityPlayer && other->m_entityData.hp > 0.0f &&
            m_world->canSee(creature, other, SightRange)) {
            plasma::Vector<3, float> difference(other->m_entityData.position - entity.position);
            if (nearest > difference.squaredLength()) {
                target = other;
                nearest = difference.squaredLength();
            }
        }
    }
    if (!target || (entity.currentAbility == AbilitySit || entity.currentAbility == AbilitySleep))
        return false;
    if (target->m_entityData.currentAbility == AbilityAssassinate &&
        target->m_entityData.timeSinceAbility < (float)target->attackDuration() &&
        target->m_entityData.focusId == creature->m_id)
        return false;
    if (entity.timeSinceAbility > (float)creature->attackDuration()) {
        State &state = m_states[creature->m_id];
        // A friendly NPC waves one time in three.
        if (entity.hostilityType != HostilityFriendlyNpc || entity.currentAbility == AbilityWave ||
            entity.currentAbility == AbilityWatchPlayer || rand() % 3 != 0)
            entity.currentAbility = AbilityWatchPlayer;
        else
            entity.currentAbility = AbilityWave;
        entity.timeSinceAbility = 0.0f;
        entity.attackRotation = target->m_entityData.position - entity.position;
        state.acceleration = plasma::Vector<3, float>(0, 0, 0);
        state.clearPath();
        state.targetId = 0;
        state.speed = 0.0f;
    }
    if (entity.currentAbility == AbilityWave)
        entity.attackRotation = target->m_entityData.position - entity.position;
    return true;
}

World::AIManager::PathNode *World::AIManager::State::pathNode(int x, int y, int z)
{
    auto found = nodes.find(IntTriple {{x, y, z}});
    return found == nodes.end() ? nullptr : &found->second;
}

float World::AIManager::rayLength(const plasma::Vector<3, Fixed> &from, const plasma::Vector<3, float> &direction,
                                  float length, bool passSpiritPassableBarriers)
{
    float travelled = 0.0f;
    plasma::Vector<3, int> block = fixedToBlock(from);
    int count = 0;
    Zone *zone = m_world->zone(floorDivide(block[0], BlocksPerZone), floorDivide(block[1], BlocksPerZone));
    while (length > travelled) {
        if (!zone)
            break;
        if (blocksWalker(zone->getBlock(block[0], block[1], block[2]), passSpiritPassableBarriers))
            break;
        if (count > RayMaxCrossings)
            break;
        ++count;
        plasma::Vector<3, Fixed> position = from + plasma::Vector<3, Fixed>(direction * travelled);
        float step = RayStepBound;
        int axis = 0;
        for (int i = 0; i < 3; ++i) {
            if (direction[i] * direction[i] < RayAxisEpsilon)
                continue;
            float distance;
            if (direction[i] > 0.0f)
                distance = (float)((Fixed(block[i] + 1) - position[i]) / direction[i]);
            else
                distance = (float)((Fixed(block[i]) - position[i]) / direction[i]);
            if (step > distance) {
                axis = i;
                step = distance;
            }
        }
        if (direction[axis] > 0.0f)
            block[axis]++;
        else
            block[axis]--;
        plasma::Vector<2, int> zonePosition(floorDivide(block[0], BlocksPerZone), floorDivide(block[1], BlocksPerZone));
        if (zonePosition != zone->m_position)
            zone = m_world->zone(zonePosition[0], zonePosition[1]);
        travelled += step;
    }
    return (std::min)(travelled, length);
}

void World::AIManager::State::setPathNode(int x, int y, int z, const PathNode &node)
{
    IntTriple position = {{x, y, z}};
    nodes[position] = node;
    frontier.insert(position);
    if (best == nodes.end() || node.heuristic < best->second.heuristic)
        best = nodes.find(position);
}

void World::AIManager::startPath(State &state)
{
    state.clearPath();
    state.startBlock = fixedToBlock(state.start);
    plasma::Vector<3, int> start = fixedToBlock(state.start);
    state.unused108 = 0;
    plasma::Vector<3, int> difference = start - fixedToBlock(state.destination);
    int dx = abs(difference[0]);
    int dy = abs(difference[1]);
    int dz = abs(difference[2]);
    // The octile distance in tenths of a block (10 straight, 14 diagonal), doubled: walkPath's heuristic.
    int horizontal = dx > dy ? 10 * dx + 4 * dy : 4 * dx + 10 * dy;
    PathNode node;
    node.movementCost = 1;
    node.heuristic = (int)(float)(2 * (horizontal + 10 * dz));
    node.totalCost = node.heuristic;
    node.parent = start;
    state.setPathNode(start[0], start[1], start[2], node);
    state.objectTargets.clear();
}

bool World::AIManager::targetSkill(Creature *creature, int milliseconds)
{
    creature->m_entityData.currentAbility = AbilityNone;
    Creature *target = m_world->creatureByID(creature->m_entityData.focusId);
    if (target && target->m_entityData.hp > 0.0f) {
        creature->m_entityData.currentAbility = AbilityAttachToTarget;
        return true;
    } else {
        float nearest = SkillTargetRangeSquared;
        for (auto other : m_world->m_creatures) {
            if (other->m_entityData.hostilityType == HostilityEnemy) {
                plasma::Vector<3, float> difference(other->m_entityData.position - creature->m_entityData.position);
                if (nearest > difference.squaredLength()) {
                    nearest = difference.squaredLength();
                    creature->m_entityData.focusId = other->m_id;
                    creature->m_entityData.currentAbility = AbilityAttachToTarget;
                    float z =
                        (1.0f - ((float)rand() * 2.0f) / (float)RAND_MAX) *
                            ((other->m_entityData.appearance.size[2] - creature->m_entityData.appearance.size[2]) *
                             0.5f) +
                        creature->m_entityData.appearance.size[2] * 0.5f;
                    float y = (1.0f - ((float)rand() * 2.0f) / (float)RAND_MAX) *
                              (other->m_entityData.appearance.size[1] * 0.5f);
                    float x = (1.0f - ((float)rand() * 2.0f) / (float)RAND_MAX) *
                              (other->m_entityData.appearance.size[0] * 0.5f);
                    creature->m_entityData.attackRotation = plasma::Vector<3, float>(x, y, z);
                }
            }
        }
    }
    return true;
}

bool World::AIManager::walkPath(State &state)
{
    if (state.nodes.empty())
        return false;
    plasma::Vector<3, int> goal = fixedToBlock(state.destination);
    if (state.frontier.empty())
        return false;
    auto selected = state.frontier.begin();
    PathNode *currentNode = nullptr;
    int lowest = -1;
    for (auto it = state.frontier.begin(); it != state.frontier.end(); ++it) {
        Sleep(0);
        PathNode *node = state.pathNode((*it)[0], (*it)[1], (*it)[2]);
        if (node && (!currentNode || node->totalCost < lowest)) {
            lowest = node->totalCost;
            selected = it;
            currentNode = node;
        }
    }
    plasma::Vector<3, int> current((*selected)[0], (*selected)[1], (*selected)[2]);
    state.frontier.erase(selected);
    if (current == goal)
        return false;

    // The node's centre at half the creature's height, added as vectors.
    plasma::Vector<3, Fixed> centre = plasma::Vector<3, Fixed>(current) +
                                      plasma::Vector<3, Fixed>(Fixed(0.5), Fixed(0.5), Fixed(state.size[2] * 0.5));

    // The footprint's bottom a hundredth of a block up, its centre sampled a thousandth up.
    plasma::Vector<3, int> low = fixedToBlock(centre - plasma::Vector<3, Fixed>(state.size * 0.5f) +
                                              plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, 0.01f)));

    plasma::Vector<3, int> high = fixedToBlock(centre + plasma::Vector<3, Fixed>(state.size * 0.5f));
    --low[0];
    --low[1];
    --low[2];
    ++high[0];
    ++high[1];
    high[2] += 2;
    // Checks one layer around the creature's footprint: floor below, walls beside and ceiling above. Each flag records
    // a solid block at an edge or corner.
    bool wallWest = false, wallEast = false, wallSouth = false, wallNorth = false;
    bool supported = false;
    bool supportedBelow = false;
    bool floorWest = false, floorEast = false, floorSouth = false, floorNorth = false;
    bool floorSouthWest = false, floorSouthEast = false, floorNorthWest = false, floorNorthEast = false;
    bool wallSouthWest = false, wallSouthEast = false, wallNorthWest = false, wallNorthEast = false;
    bool ceiling = false;

    m_world->enterZones();
    unsigned char type = m_world->getBlockAtDot(centre[0], centre[1], centre[2] + 0.001).type;
    for (int x = low[0]; x <= high[0]; ++x)
        for (int y = low[1]; y <= high[1]; ++y)
            for (int z = low[2]; z <= high[2]; ++z) {
                if (x > low[0] && x < high[0] && y > low[1] && y < high[1] && z > low[2] && z < high[2])
                    continue;
                Block block = m_world->getBlock(x, y, z);
                if (x > low[0] && x < high[0] && y > low[1] && y < high[1] && z == low[2]) {
                    if (block.isSolid() ||
                        ((block.flags & BlockFlagBarrier) &&
                         (!state.inSpiritWorld || !(block.flags & BlockFlagSpiritPassable))) ||
                        block.type == BlockWater)
                        supported = true;
                    Block below = m_world->getBlock(x, y, z - 1);
                    if (below.isSolid() || ((below.flags & BlockFlagBarrier) &&
                                            (!state.inSpiritWorld || !(below.flags & BlockFlagSpiritPassable))))
                        supportedBelow = true;
                }
                if (block.isSolid() || ((block.flags & BlockFlagBarrier) && !(block.flags & BlockFlagPassableBarrier) &&
                                        (!state.inSpiritWorld || !(block.flags & BlockFlagSpiritPassable)))) {
                    if (x == low[0] && y == low[1] && z == low[2])
                        floorSouthWest = true;
                    if (x == high[0] && y == low[1] && z == low[2])
                        floorSouthEast = true;
                    if (x == low[0] && y == high[1] && z == low[2])
                        floorNorthWest = true;
                    if (x == high[0] && y == high[1] && z == low[2])
                        floorNorthEast = true;
                    if (x == low[0] && y == low[1] && z > low[2] && z < high[2])
                        wallSouthWest = true;
                    if (x == high[0] && y == low[1] && z > low[2] && z < high[2])
                        wallSouthEast = true;
                    if (x == low[0] && y == high[1] && z > low[2] && z < high[2])
                        wallNorthWest = true;
                    if (x == high[0] && y == high[1] && z > low[2] && z < high[2])
                        wallNorthEast = true;
                    if (x > low[0] && x < high[0] && y > low[1] && y < high[1] && z == high[2])
                        ceiling = true;
                    if (x == low[0] && y > low[1] && y < high[1] && z == low[2])
                        floorWest = true;
                    if (x == high[0] && y > low[1] && y < high[1] && z == low[2])
                        floorEast = true;
                    if (y == low[1] && x > low[0] && x < high[0] && z == low[2])
                        floorSouth = true;
                    if (y == high[1] && x > low[0] && x < high[0] && z == low[2])
                        floorNorth = true;
                    if (x == low[0] && y > low[1] && y < high[1] && z > low[2] && z < high[2])
                        wallWest = true;
                    if (x == high[0] && y > low[1] && y < high[1] && z > low[2] && z < high[2])
                        wallEast = true;
                    if (y == low[1] && x > low[0] && x < high[0] && z > low[2] && z < high[2])
                        wallSouth = true;
                    if (y == high[1] && x > low[0] && x < high[0] && z > low[2] && z < high[2])
                        wallNorth = true;
                }
            }
    m_world->leaveZones();
    if (!supportedBelow && (state.binaryToggles & (1 << CreatureTogglePlayerAi)) && state.nodes.size() > 1)
        return true;

    // Expand the block into its neighbours, each written out in full: a neighbour not yet in the path map is added
    // with its heuristic (twice the 10/14-weighted octile distance to the destination block); an open neighbour takes
    // the new cost when it is lower. A step costs more without support underfoot. A diagonal is tried only when both
    // of its axial neighbours were updated.
    bool west = false;
    if (!wallWest &&
        (supported || floorWest || wallSouth || wallNorth || wallSouthWest || wallNorthWest || type == BlockWater)) {
        plasma::Vector<3, int> next(current[0] - 1, current[1], current[2]);
        PathNode *node = state.pathNode(next[0], next[1], next[2]);
        if (!node) {
            plasma::Vector<3, int> distance = next - fixedToBlock(state.destination);
            int distanceX = abs(distance[0]);
            int distanceY = abs(distance[1]);
            int distanceZ = abs(distance[2]);
            int horizontal = distanceX > distanceY ? 10 * distanceX + 4 * distanceY : 4 * distanceX + 10 * distanceY;
            int heuristic = 2 * (horizontal + 10 * distanceZ);
            int cost = currentNode->movementCost + 10;
            if (!supported)
                cost += 10;
            PathNode added;
            added.movementCost = cost;
            added.heuristic = heuristic;
            added.totalCost = cost + heuristic;
            added.parent = current;
            state.setPathNode(next[0], next[1], next[2], added);
            west = true;
        } else if (state.isOpen(next[0], next[1], next[2])) {
            int heuristic = node->heuristic;
            int cost = currentNode->movementCost + 10;
            if (!supported)
                cost += 10;
            if (cost < node->movementCost) {
                PathNode improved;
                improved.movementCost = cost;
                improved.heuristic = heuristic;
                improved.totalCost = cost + heuristic;
                improved.parent = current;
                state.setPathNode(next[0], next[1], next[2], improved);
                west = true;
            }
        }
    }
    bool east = false;
    if (!wallEast &&
        (supported || floorEast || wallSouth || wallNorth || wallSouthEast || wallNorthEast || type == BlockWater)) {
        plasma::Vector<3, int> next(current[0] + 1, current[1], current[2]);
        PathNode *node = state.pathNode(next[0], next[1], next[2]);
        if (!node) {
            plasma::Vector<3, int> distance = next - fixedToBlock(state.destination);
            int distanceX = abs(distance[0]);
            int distanceY = abs(distance[1]);
            int distanceZ = abs(distance[2]);
            int horizontal = distanceX > distanceY ? 10 * distanceX + 4 * distanceY : 4 * distanceX + 10 * distanceY;
            int heuristic = 2 * (horizontal + 10 * distanceZ);
            int cost = currentNode->movementCost + 10;
            if (!supported)
                cost += 10;
            PathNode added;
            added.movementCost = cost;
            added.heuristic = heuristic;
            added.totalCost = cost + heuristic;
            added.parent = current;
            state.setPathNode(next[0], next[1], next[2], added);
            east = true;
        } else if (state.isOpen(next[0], next[1], next[2])) {
            int heuristic = node->heuristic;
            int cost = currentNode->movementCost + 10;
            if (!supported)
                cost += 10;
            if (cost < node->movementCost) {
                PathNode improved;
                improved.movementCost = cost;
                improved.heuristic = heuristic;
                improved.totalCost = cost + heuristic;
                improved.parent = current;
                state.setPathNode(next[0], next[1], next[2], improved);
                east = true;
            }
        }
    }
    bool south = false;
    if (!wallSouth &&
        (supported || floorSouth || wallWest || wallEast || wallSouthWest || wallSouthEast || type == BlockWater)) {
        plasma::Vector<3, int> next(current[0], current[1] - 1, current[2]);
        PathNode *node = state.pathNode(next[0], next[1], next[2]);
        if (!node) {
            plasma::Vector<3, int> distance = next - fixedToBlock(state.destination);
            int distanceX = abs(distance[0]);
            int distanceY = abs(distance[1]);
            int distanceZ = abs(distance[2]);
            int horizontal = distanceX > distanceY ? 10 * distanceX + 4 * distanceY : 4 * distanceX + 10 * distanceY;
            int heuristic = 2 * (horizontal + 10 * distanceZ);
            int cost = currentNode->movementCost + 10;
            if (!supported)
                cost += 10;
            PathNode added;
            added.movementCost = cost;
            added.heuristic = heuristic;
            added.totalCost = cost + heuristic;
            added.parent = current;
            state.setPathNode(next[0], next[1], next[2], added);
            south = true;
        } else if (state.isOpen(next[0], next[1], next[2])) {
            int heuristic = node->heuristic;
            int cost = currentNode->movementCost + 10;
            if (!supported)
                cost += 10;
            if (cost < node->movementCost) {
                PathNode improved;
                improved.movementCost = cost;
                improved.heuristic = heuristic;
                improved.totalCost = cost + heuristic;
                improved.parent = current;
                state.setPathNode(next[0], next[1], next[2], improved);
                south = true;
            }
        }
    }
    bool north = false;
    if (!wallNorth &&
        (supported || floorNorth || wallWest || wallEast || wallNorthWest || wallNorthEast || type == BlockWater)) {
        plasma::Vector<3, int> next(current[0], current[1] + 1, current[2]);
        PathNode *node = state.pathNode(next[0], next[1], next[2]);
        if (!node) {
            plasma::Vector<3, int> distance = next - fixedToBlock(state.destination);
            int distanceX = abs(distance[0]);
            int distanceY = abs(distance[1]);
            int distanceZ = abs(distance[2]);
            int horizontal = distanceX > distanceY ? 10 * distanceX + 4 * distanceY : 4 * distanceX + 10 * distanceY;
            int heuristic = 2 * (horizontal + 10 * distanceZ);
            int cost = currentNode->movementCost + 10;
            if (!supported)
                cost += 10;
            PathNode added;
            added.movementCost = cost;
            added.heuristic = heuristic;
            added.totalCost = cost + heuristic;
            added.parent = current;
            state.setPathNode(next[0], next[1], next[2], added);
            north = true;
        } else if (state.isOpen(next[0], next[1], next[2])) {
            int heuristic = node->heuristic;
            int cost = currentNode->movementCost + 10;
            if (!supported)
                cost += 10;
            if (cost < node->movementCost) {
                PathNode improved;
                improved.movementCost = cost;
                improved.heuristic = heuristic;
                improved.totalCost = cost + heuristic;
                improved.parent = current;
                state.setPathNode(next[0], next[1], next[2], improved);
                north = true;
            }
        }
    }
    if (!wallSouthWest && west && south && (supported || floorSouthWest || type == BlockWater)) {
        plasma::Vector<3, int> next(current[0] - 1, current[1] - 1, current[2]);
        PathNode *node = state.pathNode(next[0], next[1], next[2]);
        if (!node) {
            plasma::Vector<3, int> distance = next - fixedToBlock(state.destination);
            int distanceX = abs(distance[0]);
            int distanceY = abs(distance[1]);
            int distanceZ = abs(distance[2]);
            int horizontal = distanceX > distanceY ? 10 * distanceX + 4 * distanceY : 4 * distanceX + 10 * distanceY;
            int heuristic = 2 * (horizontal + 10 * distanceZ);
            int cost = currentNode->movementCost + 14;
            if (!supported)
                cost += 10;
            PathNode added;
            added.movementCost = cost;
            added.heuristic = heuristic;
            added.totalCost = cost + heuristic;
            added.parent = current;
            state.setPathNode(next[0], next[1], next[2], added);
        } else if (state.isOpen(next[0], next[1], next[2])) {
            int heuristic = node->heuristic;
            int cost = currentNode->movementCost + 14;
            if (!supported)
                cost += 10;
            if (cost < node->movementCost) {
                PathNode improved;
                improved.movementCost = cost;
                improved.heuristic = heuristic;
                improved.totalCost = cost + heuristic;
                improved.parent = current;
                state.setPathNode(next[0], next[1], next[2], improved);
            }
        }
    }
    if (!wallSouthEast && east && south && (supported || floorSouthEast || type == BlockWater)) {
        plasma::Vector<3, int> next(current[0] + 1, current[1] - 1, current[2]);
        PathNode *node = state.pathNode(next[0], next[1], next[2]);
        if (!node) {
            plasma::Vector<3, int> distance = next - fixedToBlock(state.destination);
            int distanceX = abs(distance[0]);
            int distanceY = abs(distance[1]);
            int distanceZ = abs(distance[2]);
            int horizontal = distanceX > distanceY ? 10 * distanceX + 4 * distanceY : 4 * distanceX + 10 * distanceY;
            int heuristic = 2 * (horizontal + 10 * distanceZ);
            int cost = currentNode->movementCost + 14;
            if (!supported)
                cost += 10;
            PathNode added;
            added.movementCost = cost;
            added.heuristic = heuristic;
            added.totalCost = cost + heuristic;
            added.parent = current;
            state.setPathNode(next[0], next[1], next[2], added);
        } else if (state.isOpen(next[0], next[1], next[2])) {
            int heuristic = node->heuristic;
            int cost = currentNode->movementCost + 14;
            if (!supported)
                cost += 10;
            if (cost < node->movementCost) {
                PathNode improved;
                improved.movementCost = cost;
                improved.heuristic = heuristic;
                improved.totalCost = cost + heuristic;
                improved.parent = current;
                state.setPathNode(next[0], next[1], next[2], improved);
            }
        }
    }
    if (!wallNorthWest && west && north && (supported || floorNorthWest || type == BlockWater)) {
        plasma::Vector<3, int> next(current[0] - 1, current[1] + 1, current[2]);
        PathNode *node = state.pathNode(next[0], next[1], next[2]);
        if (!node) {
            plasma::Vector<3, int> distance = next - fixedToBlock(state.destination);
            int distanceX = abs(distance[0]);
            int distanceY = abs(distance[1]);
            int distanceZ = abs(distance[2]);
            int horizontal = distanceX > distanceY ? 10 * distanceX + 4 * distanceY : 4 * distanceX + 10 * distanceY;
            int heuristic = 2 * (horizontal + 10 * distanceZ);
            int cost = currentNode->movementCost + 14;
            if (!supported)
                cost += 10;
            PathNode added;
            added.movementCost = cost;
            added.heuristic = heuristic;
            added.totalCost = cost + heuristic;
            added.parent = current;
            state.setPathNode(next[0], next[1], next[2], added);
        } else if (state.isOpen(next[0], next[1], next[2])) {
            int heuristic = node->heuristic;
            int cost = currentNode->movementCost + 14;
            if (!supported)
                cost += 10;
            if (cost < node->movementCost) {
                PathNode improved;
                improved.movementCost = cost;
                improved.heuristic = heuristic;
                improved.totalCost = cost + heuristic;
                improved.parent = current;
                state.setPathNode(next[0], next[1], next[2], improved);
            }
        }
    }
    if (!wallNorthEast && east && north && (supported || floorNorthEast || type == BlockWater)) {
        plasma::Vector<3, int> next(current[0] + 1, current[1] + 1, current[2]);
        PathNode *node = state.pathNode(next[0], next[1], next[2]);
        if (!node) {
            plasma::Vector<3, int> distance = next - fixedToBlock(state.destination);
            int distanceX = abs(distance[0]);
            int distanceY = abs(distance[1]);
            int distanceZ = abs(distance[2]);
            int horizontal = distanceX > distanceY ? 10 * distanceX + 4 * distanceY : 4 * distanceX + 10 * distanceY;
            int heuristic = 2 * (horizontal + 10 * distanceZ);
            int cost = currentNode->movementCost + 14;
            if (!supported)
                cost += 10;
            PathNode added;
            added.movementCost = cost;
            added.heuristic = heuristic;
            added.totalCost = cost + heuristic;
            added.parent = current;
            state.setPathNode(next[0], next[1], next[2], added);
        } else if (state.isOpen(next[0], next[1], next[2])) {
            int heuristic = node->heuristic;
            int cost = currentNode->movementCost + 14;
            if (!supported)
                cost += 10;
            if (cost < node->movementCost) {
                PathNode improved;
                improved.movementCost = cost;
                improved.heuristic = heuristic;
                improved.totalCost = cost + heuristic;
                improved.parent = current;
                state.setPathNode(next[0], next[1], next[2], improved);
            }
        }
    }
    if (!supported) {
        plasma::Vector<3, int> next(current[0], current[1], current[2] - 1);
        PathNode *node = state.pathNode(next[0], next[1], next[2]);
        if (!node) {
            plasma::Vector<3, int> distance = next - fixedToBlock(state.destination);
            int distanceX = abs(distance[0]);
            int distanceY = abs(distance[1]);
            int distanceZ = abs(distance[2]);
            int horizontal = distanceX > distanceY ? 10 * distanceX + 4 * distanceY : 4 * distanceX + 10 * distanceY;
            int heuristic = 2 * (horizontal + 10 * distanceZ);
            int cost = currentNode->movementCost;
            PathNode added;
            added.movementCost = cost;
            added.heuristic = heuristic;
            added.totalCost = cost + heuristic;
            added.parent = current;
            state.setPathNode(next[0], next[1], next[2], added);
        } else if (state.isOpen(next[0], next[1], next[2])) {
            int heuristic = node->heuristic;
            int cost = currentNode->movementCost;
            if (cost < node->movementCost) {
                PathNode improved;
                improved.movementCost = cost;
                improved.heuristic = heuristic;
                improved.totalCost = cost + heuristic;
                improved.parent = current;
                state.setPathNode(next[0], next[1], next[2], improved);
            }
        }
    }
    if (!ceiling && (supported || !(state.binaryToggles & (1 << CreatureTogglePlayerAi))) &&
        (wallWest || wallEast || wallSouth || wallNorth || type == BlockWater)) {
        plasma::Vector<3, int> next(current[0], current[1], current[2] + 1);
        PathNode *node = state.pathNode(next[0], next[1], next[2]);
        if (!node) {
            plasma::Vector<3, int> distance = next - fixedToBlock(state.destination);
            int distanceX = abs(distance[0]);
            int distanceY = abs(distance[1]);
            int distanceZ = abs(distance[2]);
            int horizontal = distanceX > distanceY ? 10 * distanceX + 4 * distanceY : 4 * distanceX + 10 * distanceY;
            int heuristic = 2 * (horizontal + 10 * distanceZ);
            int cost = currentNode->movementCost + 14;
            if (!supported)
                cost += 14;
            PathNode added;
            added.movementCost = cost;
            added.heuristic = heuristic;
            added.totalCost = cost + heuristic;
            added.parent = current;
            state.setPathNode(next[0], next[1], next[2], added);
        } else if (state.isOpen(next[0], next[1], next[2])) {
            int heuristic = node->heuristic;
            int cost = currentNode->movementCost + 14;
            if (!supported)
                cost += 14;
            if (cost < node->movementCost) {
                PathNode improved;
                improved.movementCost = cost;
                improved.heuristic = heuristic;
                improved.totalCost = cost + heuristic;
                improved.parent = current;
                state.setPathNode(next[0], next[1], next[2], improved);
            }
        }
    }
    return true;
}

int zoneCoordinate(Fixed position)
{
    int block = position.toBlock();
    return block >= 0 ? block / BlocksPerZone : (block + 1) / BlocksPerZone - 1;
}

// AI behavior updates.
void World::AIManager::goTo(State &state, Creature *target, float range, bool probeAhead, bool matchSpeed)
{
    state.destination = target->m_entityData.position;
    if (state.probeAhead) {
        plasma::Matrix<float> rotation;
        rotation.identity();
        rotation.rotateZ(target->m_entityData.rotation[2]);
        plasma::Vector<3, Fixed> probe =
            state.destination + plasma::Vector<3, Fixed>(rotation.transformDirection(
                                    plasma::Vector<3, float>(ProbeAheadDistance, 0.0f, 0.0f)));
        for (int i = 0; i < ProbeSearchDepth; ++i) {
            if (m_world->getBlockAtDot(probe[0], probe[1], probe[2] - Fixed(1)).isSolid())
                break;
            probe[2] -= Fixed(1);
        }
        for (int i = 0; i < ProbeSearchDepth; ++i) {
            if (!m_world->getBlockAtDot(probe[0], probe[1], probe[2]).isSolid())
                break;
            probe[2] += Fixed(1);
        }
        if (m_world->getBlockAtDot(probe[0], probe[1], probe[2] - Fixed(1)).isSolid() &&
            !m_world->getBlockAtDot(probe[0], probe[1], probe[2]).isSolid())
            state.destination = probe;
    }
    while (!m_world->isBlocked(state.destination - plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, 0.5f)),
                               target->m_entityData.appearance.size, false))
        state.destination[2] -= Fixed(1);
    state.destination[2] -= Fixed(target->m_entityData.appearance.size[2] * 0.5f - FootLift);
    state.start = state.position;
    state.start[2] -= Fixed(state.size[2] * 0.5f - FootLift);
    state.range = (range + state.size[0]) + target->m_entityData.appearance.size[0];
    state.targetId = target->m_id;
    state.probeAhead = probeAhead;
    state.speed = matchSpeed && !probeAhead ? target->m_entityData.acceleration.length() : 0.0f;
    startPath(state);
}

void World::AIManager::update(int milliseconds)
{
    m_world->enterZoneMeshes();
    std::list<plasma::Vector<3, Fixed>> playerPositions;
    for (auto creature : m_world->m_creatures) {
        if (m_world->m_isClient && creature != m_world->m_localCreature)
            continue;
        if (creature->m_entityData.hostilityType == HostilityPlayer)
            playerPositions.push_back(creature->m_entityData.position);
        if ((creature->m_entityData.hostilityType != HostilityPlayer ||
             creature->m_entityData.binaryToggles.playerAi) &&
            !(creature->m_entityData.hp <= 0.0f) && !(creature->m_entityData.stunTime > 0.0f)) {
            State &state = m_states[creature->m_id];
            state.position = creature->m_entityData.position;
            state.size = creature->m_entityData.appearance.size;
            state.flags = creature->m_entityData.flags;
            // The sixteen toggles copied as the one word they are (see BinaryToggles).
            state.binaryToggles = *(const unsigned short *)&creature->m_entityData.binaryToggles;
            state.attackRotation = creature->m_entityData.attackRotation;
            state.interactions.clear();
            state.inSpiritWorld = creature->findBuff(BuffSpiritworld) != nullptr;
            updateCreature(creature, milliseconds);
            state.attackRotation = creature->m_entityData.attackRotation;
        }
    }
    m_world->leaveZoneMeshes();

    for (auto &entry : m_states) {
        Sleep(0);
        State &state = entry.second;
        state.moving = false;
        state.unsupportedStep = false;
        if (state.binaryToggles & (1 << CreatureToggleActive)) {
            state.clearPath();
            state.targetId = 0;
            state.speed = 0.0f;
            continue;
        }
        if ((float)state.searchBudget > (float)MaxSearchBudget)
            state.searchBudget = MaxSearchBudget;
        if (state.frontier.empty())
            continue;

        plasma::Vector<3, Fixed> destination = state.destination;
        m_world->enterZoneMeshes();
        Creature *target = m_world->creatureByID(state.targetId);
        if (target) {
            destination = target->m_entityData.position;
            if (state.probeAhead) {
                float angle = target->m_entityData.rotation[2] * 0.017453292f;
                float c = cosf(angle);
                float s = sinf(angle);
                // (2, 0, 0) turned by the yaw: the rows of the identity rotated about z (Matrix::rotateZ written out;
                // the products with zero stay, for their NaN behaviour).
                float x0 = c + s * 0.0f, y0 = c * 0.0f - s;
                float x1 = c * 0.0f + s, y1 = c - s * 0.0f;
                float x2 = c * 0.0f + s * 0.0f, y2 = c * 0.0f - s * 0.0f;
                plasma::Vector<3, float> forward(ProbeAheadDistance, 0.0f, 0.0f);
                plasma::Vector<3, float> ahead(y0 * forward[1] + x0 * forward[0] + 0.0f,
                                               x1 * forward[0] + y1 * forward[1] + 0.0f,
                                               y2 * forward[1] + x2 * forward[0] + 0.0f);
                plasma::Vector<3, Fixed> probe = destination + plasma::Vector<3, Fixed>(ahead);
                for (int i = 0; i < ProbeSearchDepth; ++i) {
                    if (m_world->getBlockAtDot(probe[0], probe[1], probe[2] - Fixed(1)).isSolid())
                        break;
                    probe[2] -= Fixed(1);
                }
                for (int i = 0; i < ProbeSearchDepth; ++i) {
                    if (!m_world->getBlockAtDot(probe[0], probe[1], probe[2]).isSolid())
                        break;
                    probe[2] += Fixed(1);
                }
                if (m_world->getBlockAtDot(probe[0], probe[1], probe[2] - Fixed(1)).isSolid() &&
                    !m_world->getBlockAtDot(probe[0], probe[1], probe[2]).isSolid())
                    destination = probe;
            }
            m_world->leaveZoneMeshes();
            while (
                !m_world->isBlocked(destination - plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, 0.5f)),
                                    target->m_entityData.appearance.size, false))
                destination[2] -= Fixed(1);
            destination[2] -= Fixed(target->m_entityData.appearance.size[2] * 0.5f - FootLift);
        } else {
            m_world->leaveZoneMeshes();
        }

        m_world->enterZones();
        plasma::Vector<3, Fixed> centre(destination[0], destination[1], destination[2] + state.size[2] * 0.5f);
        if (canWalk(state.size, state.position, centre, state.range, state.inSpiritWorld, state.binaryToggles)) {
            state.clearPath();
            state.targetId = 0;
            plasma::Vector<3, float> direction(destination - state.position);
            float speed = PathAcceleration;
            // Near the target (ten ranges) match its speed, nearer (two) halve it; moving once the acceleration is
            // above 0.001.
            if (state.speed > 0.01f && state.range * state.range * 10.0f * 10.0f > direction.squaredLength())
                speed = state.speed;
            if (state.range * state.range * 4.0f > direction.squaredLength())
                speed *= 0.5f;
            if (m_world->getBlockAtDot(state.position[0], state.position[1], state.position[2]).type != BlockWater)
                direction[2] = 0.0f;
            if (direction.squaredLength() > 0.0f)
                direction *= 1.0f / sqrtf(direction.squaredLength());
            state.acceleration = direction * speed;
            if (state.acceleration.squaredLength() > 0.001f)
                state.moving = true;
            state.searchBudget += state.searchBudget / -4;
            if (state.searchBudget < MinSearchBudget)
                state.searchBudget = MinSearchBudget;
        } else {
            int budget = state.searchBudget;
            if (state.binaryToggles & (1 << CreatureTogglePlayerAi))
                budget = PlayerAiSearchBudget;
            state.start = state.position;
            state.start[2] -= Fixed(state.size[2] * 0.5f - FootLift);
            state.destination = destination;
            startPath(state);
            for (int i = 0; i < budget; ++i) {
                if (!state.path.empty()) {
                    plasma::Vector<3, int> difference = state.path.back() - fixedToBlock(state.destination);
                    if (state.size[0] * state.size[0] >= (float)difference.squaredLength())
                        break;
                }
                walkPath(state);
                finishPathSearch(state);
                Sleep(0);
            }
            if (!state.path.empty()) {
                plasma::Vector<3, int> difference = state.path.back() - fixedToBlock(state.destination);
                if ((float)difference.squaredLength() > state.size[0] * state.size[0])
                    state.searchBudget += state.searchBudget / 10;
            }
            state.acceleration = plasma::Vector<3, float>(0, 0, 0);
            if (!(state.binaryToggles & (1 << CreatureTogglePlayerAi)) || state.path.size() > MinPlayerAiPathLength) {
                int remaining = (int)state.path.size();
                for (auto step = state.path.rbegin(); step != state.path.rend(); ++step) {
                    Sleep(0);
                    --remaining;
                    plasma::Vector<3, Fixed> next(Fixed((*step)[0]) + Fixed(0.5f), Fixed((*step)[1]) + Fixed(0.5f),
                                                  Fixed((*step)[2]) + Fixed(state.size[2] * 0.5f));
                    if (remaining >= 2 && !canWalk(state.size, state.position, next, PathStepRange, state.inSpiritWorld,
                                                   state.binaryToggles))
                        continue;
                    state.acceleration = next - state.position;
                    if (m_world->getBlockAtDot(state.position[0], state.position[1], state.position[2]).type !=
                            BlockWater &&
                        state.position[2] < next[2])
                        state.acceleration[2] = 0.0f;
                    if (state.acceleration.squaredLength() > 0.0f)
                        state.acceleration *= 1.0f / sqrtf(state.acceleration.squaredLength());
                    state.acceleration *= PathAcceleration;
                    if (remaining < SupportLookahead && step.base() != state.path.end()) {
                        const plasma::Vector<3, int> &following = *step.base();
                        if (!m_world->getBlock(following[0], following[1], following[2] - 1).isSolid()) {
                            state.unsupportedStep = true;
                            state.acceleration *= UnsupportedStepSlowdown;
                        }
                        if (Fixed(state.size[2] * 0.5f) + Fixed(following[2]) > state.position[2])

                            state.acceleration[2] = ClimbAcceleration;
                    }
                    break;
                }
            }
        }
        m_world->leaveZones();
    }

    m_world->enterZoneMeshes();
    for (auto creature : m_world->m_creatures) {
        if (m_world->m_isClient && creature != m_world->m_localCreature)
            continue;
        if ((creature->m_entityData.hostilityType != HostilityPlayer ||
             creature->m_entityData.binaryToggles.playerAi) &&
            !(creature->m_entityData.hp <= 0.0f) && !(creature->m_entityData.stunTime > 0.0f)) {
            creature->m_entityData.binaryToggles.dodge = 0;
            auto found = m_states.find(creature->m_id);
            if (found == m_states.end())
                continue;
            State &state = found->second;
            creature->m_entityData.acceleration = state.acceleration;
            if (state.unsupportedStep)
                creature->m_entityData.currentAbility = AbilityClimb;
            else if (creature->m_entityData.currentAbility == AbilityClimb)
                creature->m_entityData.currentAbility = AbilityNone;
            creature->m_interactions.insert(creature->m_interactions.end(), state.interactions.begin(),
                                            state.interactions.end());
            if (state.furnitureObjectKey[2] >= 0) {
                StaticEntity *object = m_world->object(state.furnitureObjectKey);
                if (object) {
                    plasma::Vector<3, float> distance(creature->m_entityData.position - object->position);
                    if (distance.squaredLength() < InteractionRangeSquared) {
                        Interaction interaction;
                        interaction.type = InteractionTalk;
                        interaction.position = state.furnitureObjectKey;
                        creature->m_interactions.push_back(interaction);
                        creature->m_entityData.acceleration = plasma::Vector<3, float>(0, 0, 0);
                        state.acceleration = plasma::Vector<3, float>(0, 0, 0);
                        state.furnitureObjectKey = plasma::Vector<3, int>(-1, -1, -1);
                    }
                }
            }
        }
    }
    std::list<long long> removed;
    for (auto &entry : m_states) {
        Creature *creature = m_world->creatureByID(entry.first);
        if (!creature || (m_world->m_isClient && m_world->creatureByID(entry.first) != m_world->m_localCreature))
            removed.push_back(entry.first);
    }
    m_world->leaveZoneMeshes();
    for (auto id : removed)
        m_states.erase(id);
}

void World::AIManager::updateCreature(Creature *creature, int milliseconds)
{
    unsigned int flags = creature->m_entityData.appearance.appearanceFlags;
    if (flags & AppearanceFlagPetrified)
        return;
    if (creature->m_entityData.classType == ClassSpawner) {
        if (creature->m_entityData.timeSinceAbility > (float)creature->attackDuration()) {
            creature->m_entityData.currentAbility = AbilitySpawnerSummon;
            creature->m_entityData.timeSinceAbility = 0.0f;
        }
        return;
    }
    unsigned char hostility = creature->m_entityData.hostilityType;
    if ((unsigned char)(hostility - HostilityAttackableObject) > 1 && creature->m_entityData.ownerId) {
        Creature *other = m_world->creatureByID(creature->m_entityData.ownerId);
        if (other && other->m_entityData.hp > 0.0f && other->m_entityData.stunTime <= 0.0f) {
            int race = other->m_entityData.race;
            if (race == RaceManaMech || race == RaceTreant || race == RaceSkullSlater) {
                creature->m_entityData.currentAbility = AbilityRideNpcMount;
                creature->m_entityData.focusId = other->m_id;
                return;
            }
        }
    }
    if ((flags & AppearanceFlagMissionResident) && hostility == HostilityFriendlyNpc) {
        unsigned int time = timeGetTime();
        bool passive = creature->m_entityData.binaryToggles.active;
        State &state = m_states[creature->m_id];
        if (!passive && state.wasPassive) {
            state.playerInSight = false;
            state.lastSightingTime = 0;
        }
        bool wasNear = state.playerInSight;
        state.playerInSight = false;
        for (auto other : m_world->m_creatures) {
            if (!other->m_entityData.hostilityType && m_world->canSee(creature, other, SightRange)) {
                state.playerInSight = true;
                break;
            }
        }
        if (state.playerInSight && !wasNear && time - state.lastSightingTime > ResidentTalkCooldown) {
            m_world->m_serverUpdates.m_talkingCreatureList.push_back(creature->m_id);
            state.lastSightingTime = time;
        }
        state.wasPassive = passive;
    }
    if (creature->m_entityData.binaryToggles.active)
        return;
    if (!(creature->m_behaviours & BehaviourMirrorOwner) || !mirrorOwnerActions(creature, milliseconds)) {
        if ((unsigned char)(creature->m_entityData.hostilityType - HostilityAttackableObject) > 1) {
            if ((creature->m_behaviours & BehaviourSupportOwner) && checkSupport(creature, milliseconds))
                return;
            if (!combat(creature, milliseconds)) {
                if (creature->m_behaviours & BehaviourFollowOwner) {
                    escort(creature, milliseconds);
                } else if (!watchOrGreetPlayer(creature, milliseconds) && !handleTarget(creature, milliseconds)) {
                    idle(creature, milliseconds);
                }
            }
        } else if (creature->m_behaviours & BehaviourAttachToEnemy) {
            targetSkill(creature, milliseconds);
        }
    }
    if (creature->m_entityData.acceleration.squaredLength() > 0.0f &&
        (creature->m_entityData.flags & CreatureFlagAtPassableWall)) {
        double halfX = (double)creature->m_entityData.appearance.size[0] * 0.5;
        int maxX = zoneCoordinate(creature->m_entityData.position[0] + halfX + Fixed(1));
        double halfY = (double)creature->m_entityData.appearance.size[1] * 0.5;
        int minY = zoneCoordinate(creature->m_entityData.position[1] - Fixed(halfY) - Fixed(1));
        int maxY = zoneCoordinate(creature->m_entityData.position[1] + halfY + Fixed(1));
        int minX = zoneCoordinate(creature->m_entityData.position[0] - Fixed(halfX) - Fixed(1));
        for (int x = minX; x <= maxX; ++x)
            for (int y = minY; y <= maxY; ++y) {
                Zone *zone = m_world->zone(x, y);
                if (!zone)
                    continue;
                int index = -1;
                for (auto &object : zone->m_staticEntities) {
                    ++index;
                    if (((unsigned)(object.type - StaticEntityDoor) > (StaticEntityBigIronDoor - StaticEntityDoor) &&
                         object.type != StaticEntityHatch) ||
                        !object.active)
                        continue;
                    plasma::Vector<3, float> size = object.scaledSize();
                    if (object.orientation & 1)
                        std::swap(size[0], size[1]);
                    Fixed creatureX(creature->m_entityData.appearance.size[0] * 0.5f);
                    Fixed objectX(size[0] * 0.5f);
                    if (creature->m_entityData.position[0] + creatureX + Fixed(1) < object.position[0] - objectX ||
                        creature->m_entityData.position[0] - creatureX - Fixed(1) >= object.position[0] + objectX)
                        continue;
                    Fixed creatureY(creature->m_entityData.appearance.size[1] * 0.5f);
                    Fixed objectY(size[1] * 0.5f);
                    if (creature->m_entityData.position[1] + creatureY + Fixed(1) < object.position[1] - objectY ||
                        creature->m_entityData.position[1] - creatureY - Fixed(1) >= object.position[1] + objectY)
                        continue;
                    Fixed creatureZ(creature->m_entityData.appearance.size[2] * 0.5f);
                    if (creature->m_entityData.position[2] + creatureZ >= object.position[2] &&
                        creature->m_entityData.position[2] - creatureZ < object.position[2] + Fixed(size[2])) {
                        Interaction interaction;
                        interaction.position = plasma::Vector<3, int>(x, y, index);
                        interaction.type = InteractionTalk;
                        creature->m_interactions.push_back(interaction);
                    }
                }
            }
    }
    float time = m_world->m_state.m_time;
    if (creature->m_entityData.currentAbility == AbilitySleep ||
        (time >= LampOffHour * MillisecondsPerHour && time < LampOnHour * MillisecondsPerHour) ||
        speciesType(creature->m_entityData.race) != SpeciesHumanoid)
        creature->m_entityData.binaryToggles.lampOn = 0;
    else
        creature->m_entityData.binaryToggles.lampOn = 1;
}

bool World::AIManager::checkSupport(Creature *creature, int milliseconds)
{
    if (creature->m_entityData.binaryToggles.active || creature->m_entityData.binaryToggles.noTerrainCollision)
        return false;
    State &state = m_states[creature->m_id];
    Creature *target = m_world->creatureByID(creature->m_entityData.ownerId);
    if (!target || target->m_entityData.hp <= 0)
        return false;
    bool visible = m_world->inRange(creature->m_entityData.position, target->m_entityData.position, true,
                                    CombatSightRange, nullptr);
    float range = SupportRange;
    if (!visible)
        range = 1.0f;
    if (plasma::Vector<3, float>(creature->m_entityData.position - target->m_entityData.position).squaredLength() >
            range * range ||
        !visible) {
        if (creature->m_entityData.stunTime <= 0)
            state.chaseTime += milliseconds;
        if (state.chaseTime > ChaseTimeout && creature->m_entityData.race != RaceWraith) {
            creature->m_aggro.clear();
            state.clearPath();
            state.targetId = 0;
            state.retargetDelay = RetargetDelay;
            state.chaseTime = 0;
            return false;
        }
        goTo(state, target, range, false, false);
        return creature->m_entityData.currentAbility == AbilitySupportRay &&
               creature->m_entityData.timeSinceAbility < creature->attackDuration();
    }
    if (creature->m_entityData.currentAbility != AbilitySupportRay) {
        if (creature->m_entityData.timeSinceAbility >= creature->attackDuration()) {
            creature->m_entityData.currentAbility = AbilitySupportRay;
            creature->m_entityData.focusId = target->m_id;
            creature->m_entityData.timeSinceAbility = 0;
        }
        if (creature->m_entityData.currentAbility != AbilitySupportRay)
            return false;
    }
    creature->m_entityData.attackRotation =
        plasma::Vector<3, float>(target->m_entityData.position - creature->m_entityData.position);
    for (std::list<Creature::Buff>::iterator buff = target->m_buffs.begin(); buff != target->m_buffs.end(); ++buff) {
        if (buff->sourceId == creature->m_id && buff->type == BuffMaxHpMultiplier &&
            buff->timeRemaining > SupportBuffRenewTime)
            return creature->m_entityData.currentAbility == AbilitySupportRay &&
                   creature->m_entityData.timeSinceAbility < creature->attackDuration();
    }
    float maximum = target->maxHP();
    Creature::Buff buff;
    buff.type = BuffMaxHpMultiplier;
    buff.strength = SupportBuffStrength;
    buff.timeRemaining = SupportBuffDuration;
    buff.sourceId = creature->m_id;
    target->setBuff(buff, true);
    ServerUpdates::BuffPacket packet;
    packet.sourceId = creature->m_id;
    packet.targetId = target->m_id;
    packet.buff = buff;
    packet.replace = 1;
    // Leaves packet.unused2c uninitialized before copying the packet.
    m_world->m_serverUpdates.m_buffPacketList.push_back(packet);
    target->m_entityData.hp *= target->maxHP() / maximum;
    return true;
}

bool World::AIManager::escort(Creature *creature, int milliseconds)
{
    Creature *target = m_world->creatureByID(creature->m_entityData.ownerId);
    if (!target)
        return false;
    if (target->m_entityData.currentAbility == AbilityCallPet &&
        target->m_entityData.timeSinceAbility < PetCallTeleportTime &&
        target->m_entityData.timeSinceAbility < PetCallTimeLimit) {
        plasma::Vector<3, float> difference(target->m_entityData.position - creature->m_entityData.position);
        if (difference.squaredLength() > PetTeleportDistanceSquared)
            creature->m_entityData.position = target->m_entityData.position;
    }
    if (creature->m_entityData.hostilityType == HostilityPet) {
        creature->m_entityData.hp += creature->maxHP() * ((milliseconds * 0.001f) * PetRegenerationRate);
        if (creature->m_entityData.hp > creature->maxHP())
            creature->m_entityData.hp = creature->maxHP();
    }
    if (target->m_entityData.currentAbility == AbilitySneak)
        creature->m_entityData.currentAbility = AbilitySneak;
    else if (creature->m_entityData.currentAbility == AbilitySneak)
        creature->m_entityData.currentAbility = AbilityNone;
    if (target->findBuff(BuffCamouflage) && !creature->findBuff(BuffCamouflage))
        m_world->applyBuff(creature, *target->findBuff(BuffCamouflage), true);
    State &state = m_states[creature->m_id];
    plasma::Matrix<float> rotation;
    rotation.identity();
    rotation.rotateZ(target->m_entityData.rotation[2]);
    plasma::Vector<3, Fixed> probe =
        target->m_entityData.position +
        plasma::Vector<3, Fixed>(rotation.transformDirection(plasma::Vector<3, float>(ProbeAheadDistance, 0.0f, 0.0f)));
    for (int i = 0; i < ProbeSearchDepth; ++i) {
        if (m_world->getBlockAtDot(probe[0], probe[1], probe[2] - Fixed(1)).isSolid())
            break;
        probe[2] -= Fixed(1);
    }
    for (int i = 0; i < ProbeSearchDepth; ++i) {
        if (!m_world->getBlockAtDot(probe[0], probe[1], probe[2]).isSolid())
            break;
        probe[2] += Fixed(1);
    }
    plasma::Vector<3, Fixed> position = target->m_entityData.position;
    if (m_world->getBlockAtDot(probe[0], probe[1], probe[2] - Fixed(1)).isSolid() &&
        !m_world->getBlockAtDot(probe[0], probe[1], probe[2]).isSolid())
        position = probe;
    creature->m_entityData.setToggles(
        1 << CreatureToggleFastMovement,
        plasma::Vector<3, float>(position - creature->m_entityData.position).squaredLength() >
                EscortSprintDistanceSquared ||
            target->m_entityData.binaryToggles.fastMovement);
    if (plasma::Vector<3, float>(position - creature->m_entityData.position).squaredLength() <
        EscortStopDistanceSquared) {
        state.acceleration = plasma::Vector<3, float>(0, 0, 0);
        state.clearPath();
        state.targetId = 0;
        state.speed = 0;
        return false;
    }
    goTo(state, target, EscortRange, creature->m_entityData.hostilityType == HostilityPet, true);
    return true;
}

bool World::AIManager::handleTarget(Creature *creature, int milliseconds)
{
    for (auto id : creature->m_groupMembers) {
        if (id == creature->m_id)
            break;
        Creature *target = m_world->creatureByID(id);
        if (target && target->m_entityData.hp > 0) {
            if (target->m_entityData.currentAbility == AbilityCallPet &&
                target->m_entityData.timeSinceAbility < PetCallTeleportTime &&
                target->m_entityData.timeSinceAbility < PetCallTimeLimit) {
                plasma::Vector<3, float> difference(target->m_entityData.position - creature->m_entityData.position);
                if (difference.squaredLength() > PetTeleportDistanceSquared)
                    creature->m_entityData.position = target->m_entityData.position;
            }
            if (creature->m_entityData.hostilityType == HostilityPet) {
                creature->m_entityData.hp += creature->maxHP() * ((milliseconds * 0.001f) * PetRegenerationRate);
                if (creature->m_entityData.hp > creature->maxHP())
                    creature->m_entityData.hp = creature->maxHP();
            }
            if (target->m_entityData.currentAbility == AbilitySneak)
                creature->m_entityData.currentAbility = AbilitySneak;
            else if (creature->m_entityData.currentAbility == AbilitySneak)
                creature->m_entityData.currentAbility = AbilityNone;
            if (target->findBuff(BuffCamouflage) && !creature->findBuff(BuffCamouflage))
                m_world->applyBuff(creature, *target->findBuff(BuffCamouflage), true);
            State &state = m_states[creature->m_id];
            plasma::Matrix<float> rotation;
            rotation.identity();
            rotation.rotateZ(target->m_entityData.rotation[2]);
            plasma::Vector<3, Fixed> probe =
                target->m_entityData.position + plasma::Vector<3, Fixed>(rotation.transformDirection(
                                                    plasma::Vector<3, float>(ProbeAheadDistance, 0.0f, 0.0f)));
            for (int i = 0; i < ProbeSearchDepth; ++i) {
                if (m_world->getBlockAtDot(probe[0], probe[1], probe[2] - Fixed(1)).isSolid())
                    break;
                probe[2] -= Fixed(1);
            }
            for (int i = 0; i < ProbeSearchDepth; ++i) {
                if (!m_world->getBlockAtDot(probe[0], probe[1], probe[2]).isSolid())
                    break;
                probe[2] += Fixed(1);
            }
            plasma::Vector<3, Fixed> position = target->m_entityData.position;
            if (m_world->getBlockAtDot(probe[0], probe[1], probe[2] - Fixed(1)).isSolid() &&
                !m_world->getBlockAtDot(probe[0], probe[1], probe[2]).isSolid())
                position = probe;
            creature->m_entityData.setToggles(
                1 << CreatureToggleFastMovement,
                plasma::Vector<3, float>(position - creature->m_entityData.position).squaredLength() >
                        FollowerSprintDistanceSquared ||
                    target->m_entityData.binaryToggles.fastMovement);
            if (plasma::Vector<3, float>(position - creature->m_entityData.position).squaredLength() <
                FollowerStopDistanceSquared) {
                state.acceleration = plasma::Vector<3, float>(0, 0, 0);
                state.clearPath();
                state.targetId = 0;
                state.speed = 0;
                return false;
            }
            goTo(state, target, FollowerRange, false, true);
            return true;
        }
    }
    return false;
}

bool World::AIManager::mirrorOwnerActions(Creature *creature, int milliseconds)
{
    Creature *target = m_world->creatureByID(creature->m_entityData.ownerId);
    if (!target)
        return false;
    creature->m_entityData.binaryToggles = target->m_entityData.binaryToggles;
    creature->m_entityData.currentAbility = target->m_entityData.currentAbility;
    creature->m_entityData.timeSinceAbility = target->m_entityData.timeSinceAbility;
    creature->m_entityData.mp = target->m_entityData.mp;
    creature->m_entityData.charge = target->m_entityData.charge;
    creature->m_entityData.actionItem = target->m_entityData.actionItem;
    if (target->m_entityData.rollTime > 0 && creature->m_entityData.rollTime <= 0) {
        creature->m_entityData.rollTime = target->m_entityData.rollTime;
        creature->m_entityData.velocity = target->m_entityData.velocity;
    }
    creature->m_entityData.attackRotation = target->m_entityData.attackRotation;
    creature->m_entityData.attackRotation = target->m_entityData.position +
                                            plasma::Vector<3, Fixed>(target->m_entityData.attackRotation) -
                                            creature->m_entityData.position;
    // Squares signed dot-coordinate differences without overflow checks.
    creature->m_entityData.setToggles(
        1 << CreatureToggleFastMovement,
        (target->m_entityData.position - creature->m_entityData.position).squaredLength() >
            Fixed(MirrorSprintDistanceSquared));
    if (creature->m_entityData.hostilityType == HostilityPet) {
        creature->m_entityData.hp += creature->maxHP() * ((milliseconds * 0.001f) * PetRegenerationRate);
        if (creature->m_entityData.hp > creature->maxHP())
            creature->m_entityData.hp = creature->maxHP();
    }
    plasma::Vector<3, float> difference(target->m_entityData.position - creature->m_entityData.position);
    State &state = m_states[creature->m_id];
    float distance = (difference[0] * difference[0] + difference[1] * difference[1]) + difference[2] * difference[2];
    if (distance < MirrorStopDistanceSquared) {
        state.acceleration = plasma::Vector<3, float>(0, 0, 0);
        creature->m_entityData.acceleration = plasma::Vector<3, float>(0, 0, 0);
        return false;
    }
    if (creature->m_entityData.hostilityType == HostilityPet && distance > PetTeleportDistanceSquared &&
        (target->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater)) &&
        creature->m_entityData.currentAbility != AbilitySailBoat) {
        creature->m_entityData.position = target->m_entityData.position;
        return false;
    }
    goTo(state, target, MirrorRange, false, true);
    return true;
}

bool World::AIManager::combat(Creature *creature, int milliseconds)
{
    if (creature->m_entityData.binaryToggles.active || creature->m_entityData.binaryToggles.noTerrainCollision)
        return false;
    creature->m_entityData.setToggles(1 << CreatureToggleWraithTargeting, false);
    unsigned char abilityOffset = creature->m_entityData.currentAbility - AbilitySneak;
    if (abilityOffset > (AbilitySlowFall - AbilitySneak) || !(AiTimeoutExemptAbilities & (1ULL << abilityOffset))) {
        if (creature->m_entityData.timeSinceAbility > StaleAbilityTimeout)
            creature->m_entityData.currentAbility = AbilityNone;
    }
    if (creature->m_entityData.rollTime != 0.0f)
        return true;
    if (creature->m_entityData.currentAbility == AbilityDrink && creature->m_entityData.timeSinceAbility < DrinkTime)
        return true;
    if (creature->m_entityData.currentAbility == AbilityEat && creature->m_entityData.timeSinceAbility < EatTime)
        return true;
    if (creature->m_entityData.currentAbility == AbilityRestHeal) {
        if (creature->m_entityData.timeSinceAbility < (float)creature->attackDuration()) {
            creature->m_entityData.acceleration = plasma::Vector<3, float>(0, 0, 0);
            return true;
        }
        creature->m_entityData.currentAbility = AbilityNone;
    }
    if (creature->m_entityData.hp < creature->maxHP() * PotionHealthFraction &&
        creature->m_entityData.timeSinceAbility > (float)creature->attackDuration()) {
        for (auto &tab : creature->m_inventory.m_inventoryTabs) {
            for (auto &stack : tab) {
                if (!stack.quantity || stack.item.category != ItemCategoryConsumable ||
                    stack.item.id != ConsumableLifePotion)
                    continue;
                creature->m_entityData.currentAbility = AbilityDrink;
                creature->m_entityData.timeSinceAbility = 0.0f;
                creature->m_entityData.actionItem = std::move(stack.item);
                if (--stack.quantity <= 0) {
                    stack.quantity = 0;
                    stack.item.category = ItemCategoryNone;
                    stack.item.id = NoneCleared;
                }
                // Drinking, the creature drifts randomly (-2..2 on each axis).
                float y = (float)rand() * 4.0f / (float)RAND_MAX - 2.0f;
                creature->m_entityData.acceleration[0] = (float)rand() * 4.0f / (float)RAND_MAX - 2.0f;
                creature->m_entityData.acceleration[1] = y;
                creature->m_entityData.acceleration[2] = 0.0f;
                return true;
            }
        }
    }
    State &state = m_states[creature->m_id];
    if (state.preferredTargetId && m_world->creatureByID(state.preferredTargetId) && creature->m_entityData.hp > 0.0f) {
        if (creature->m_aggro[state.preferredTargetId] < MinTargetAggro)
            creature->m_aggro[state.preferredTargetId] = MinTargetAggro;
    }
    if (creature->m_entityData.race == RaceWraith) {
        std::list<long long> removed;
        for (const auto &entry : creature->m_aggro) {
            Creature *other = m_world->creatureByID(entry.first);
            if (other && other->m_entityData.hostilityType)
                removed.push_back(entry.first);
        }
        for (const auto &id : removed)
            creature->m_aggro.erase(id);
    }
    std::list<long long> removed;
    for (const auto &entry : creature->m_aggro) {
        if (entry.first != state.preferredTargetId) {
            Creature *other = m_world->creatureByID(entry.first);
            if (other && (other->m_entityData.hp <= 0.0f || other->m_entityData.binaryToggles.active))
                removed.push_back(entry.first);
        }
    }
    for (const auto &id : removed)
        creature->m_aggro.erase(id);
    long long targetId = creature->maxAggroTarget();
    Creature *target = m_world->creatureByID(targetId);
    if (!target && creature->m_entityData.race == RaceWraith) {
        plasma::Vector<3, float> difference(creature->m_entityData.origin - creature->m_entityData.position);
        if (difference.squaredLength() > WraithLeashSquared)
            creature->m_entityData.position = creature->m_entityData.origin;
    }
    if (creature->m_entityData.hostilityType == HostilityPet) {
        auto owner = m_world->m_creaturesById.find(creature->m_entityData.ownerId);
        if (owner != m_world->m_creaturesById.end() && owner->second) {
            target = nullptr;
            if (owner->second->m_entityData.currentAbility == AbilityRidePet)
                return false;
            float highest = 0.0f;
            for (const auto &entry : owner->second->m_hitCounts) {
                if (entry.second > highest) {
                    if (m_world->creatureByID(entry.first) &&
                        m_world->creatureByID(entry.first)->m_entityData.hp > 0.0f) {
                        target = m_world->creatureByID(entry.first);
                        highest = entry.second;
                    }
                }
            }
        }
    }
    if (!target && creature->m_entityData.hp < creature->maxHP() * 0.5f &&
        creature->m_entityData.hostilityType == HostilityFriendlyNpc &&
        !(creature->m_entityData.appearance.appearanceFlags & AppearanceFlagSummoned)) {
        creature->m_entityData.currentAbility = AbilityEat;
        creature->m_entityData.timeSinceAbility = 0.0f;
        creature->m_entityData.actionItem.category = ItemCategoryConsumable;
        creature->m_entityData.actionItem.id = ConsumableGinsengSoup;
        creature->m_entityData.acceleration = plasma::Vector<3, float>(0, 0, 0);
        return true;
    }
    state.specialSkillDelay -= milliseconds;
    if (state.specialSkillDelay < 0)
        state.specialSkillDelay = 0;
    bool pacified = false;
    for (const auto &buff : creature->m_buffs) {
        if (buff.type == BuffTamingAnger) {
            pacified = true;
            break;
        }
    }
    if (!target && !(creature->m_entityData.hostilityType == HostilityPet && creature->m_entityData.ownerId) &&
        state.retargetDelay <= 0) {
        for (auto other : m_world->m_creatures) {
            if (other == creature || other->m_entityData.hp <= 0.0f || other->m_entityData.binaryToggles.active ||
                other->m_entityData.hostilityType == HostilityPet ||
                (creature->m_entityData.race == RaceWraith && other->m_entityData.hostilityType) ||
                (state.preferredTargetId && other->m_id != state.preferredTargetId) || pacified)
                continue;
            if ((areEnemies(creature, other) ||
                 (!creature->m_entityData.ownerId && other->m_entityData.currentAbility == AbilityOfferPetFood &&
                  other->m_entityData.actionItem.category == ItemCategoryPetFood &&
                  other->m_entityData.actionItem.id == creature->m_entityData.race)) &&
                m_world->canSee(creature, other, SightRange) &&
                (!creature->isMarked() || (other->m_entityData.currentAbility == AbilityOfferPetFood &&
                                           other->m_entityData.actionItem.category == ItemCategoryPetFood &&
                                           other->m_entityData.actionItem.id == creature->m_entityData.race)))
                target = other;
        }
        if (target) {
            if (creature->m_aggro[target->m_id] < MinTargetAggro) {
                creature->m_aggro[target->m_id] = MinTargetAggro;
                m_world->onAttack(target, creature);
            }
        }
    }
    if (target && creature->m_entityData.race == RaceWraith)
        creature->m_entityData.setToggles(1 << CreatureToggleWraithTargeting, true);
    // Fast movement while chasing: before the first attack, or once the current one has recovered.
    creature->m_entityData.setToggles(
        1 << CreatureToggleFastMovement,
        target && (!creature->m_entityData.currentAbility ||
                   creature->m_entityData.timeSinceAbility > (float)(creature->holdTime(AbilityUseCurrent) / 2 +
                                                                     creature->anticipationTime(AbilityUseCurrent) +
                                                                     creature->strikeTime(AbilityUseCurrent))));
    state.retargetDelay -= milliseconds;
    if (state.retargetDelay < 0)
        state.retargetDelay = 0;
    if (!target) {
        state.chaseTime = 0;
        state.specialSkillDelay = rand() % SpecialSkillDelay;
        return false;
    }
    if ((unsigned char)(creature->m_entityData.currentAbility - AbilitySit) <= 1) {
        creature->m_entityData.currentAbility = AbilityNone;
        creature->m_entityData.timeSinceAbility = (float)creature->attackDuration();
    }
    plasma::Vector<3, float> difference(target->m_entityData.position - creature->m_entityData.position);
    float contactRange = (creature->m_entityData.appearance.size[0] + creature->m_entityData.appearance.size[0]) +
                         target->m_entityData.appearance.size[0];
    float range = contactRange;
    bool visible = m_world->inRange(creature->m_entityData.position, target->m_entityData.position, true,
                                    CombatSightRange, nullptr);
    if (target->m_entityData.currentAbility == AbilityAssassinate &&
        target->m_entityData.timeSinceAbility < (float)target->attackDuration()) {
        creature->m_aggro[target->m_id] = 0.0f;
        return false;
    }
    if (creature->isWraithTargeting()) {
        visible = true;
        range = 0.0f;
        if (creature->m_entityData.timeSinceAbility > (float)creature->attackDuration()) {
            creature->m_entityData.currentAbility = AbilityWraithAttack;
            creature->m_entityData.timeSinceAbility = 0.0f;
        }
    }
    if (creature->isAttackingFromRange() && visible) {
        if (((creature->m_entityData.currentAbility - AbilityCyclone) & AiFourAbilityIntervalMask) ||
            creature->m_entityData.currentAbility == AbilityFireExplosion ||
            !(creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime(AbilityUseCurrent)))
            range = RangedAttackRange;
    }
    if (creature->m_entityData.currentAbility == AbilityFireRay ||
        creature->m_entityData.currentAbility == AbilitySupportRay ||
        creature->m_entityData.currentAbility == AbilityFireMissiles ||
        creature->m_entityData.currentAbility == AbilityThrowRock) {
        if (creature->m_entityData.timeSinceAbility < (float)creature->attackDuration())
            range = RangedAttackRange;
    }
    if (creature->m_entityData.currentAbility == AbilityFireBreath)
        range = CloseAttackRange;
    if (creature->m_entityData.currentAbility == AbilityShellSpin)
        range = target->m_entityData.appearance.size[0] + creature->m_entityData.appearance.size[0];
    Creature *pet = m_world->creatureByID(creature->m_petId);
    if (pet && pet->m_entityData.race == RaceManaMech && pet->m_entityData.hp > 0.0f)
        range = CloseAttackRange;
    if (target->m_entityData.stealth > 0.0f) {
        // Takes the larger value.
        float factor = std::max(MinStealthRangeFactor, 1.0f - target->m_entityData.stealth);
        range = std::max(contactRange, (factor * factor) * range);
    }
    int specialSkill = AbilityNone;
    if (!state.specialSkillDelay && !creature->m_specialSkills.empty()) {
        specialSkill = creature->m_specialSkills[rand() % creature->m_specialSkills.size()];
        if (rand() % 2) {
            specialSkill = AbilityNone;
            state.specialSkillDelay = SpecialSkillDelay;
        } else {
            unsigned int offset = specialSkill - AbilityHeadbutt;
            if (offset <= (AbilityLunge - AbilityHeadbutt) && (AiFixedRangeSpecialAbilities & (1ULL << offset)))
                range = SpecialSkillRange;
            if (specialSkill == AbilityCarryCreature) {
                if (creature->m_entityData.flags & CreatureFlagInWater)
                    specialSkill = AbilityNone;
            } else if (specialSkill == AbilityCyclone || specialSkill == AbilityLavaPuddle) {
                range = (creature->m_entityData.appearance.size[0] + creature->m_entityData.appearance.size[0]) +
                        target->m_entityData.appearance.size[0];
            }
        }
    }
    if (!visible)
        range = 1.0f;
    float squaredDistance = difference.squaredLength();
    if (creature->m_entityData.currentAbility != AbilitySmash &&
        (squaredDistance > range * range || !visible ||
         !(creature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater)))) {
        if (creature->m_entityData.stunTime <= 0.0f)
            state.chaseTime += milliseconds;
        if (state.chaseTime > ChaseTimeout && creature->m_entityData.race != RaceWraith) {
            creature->m_aggro.clear();
            state.clearPath();
            state.targetId = 0;
            state.retargetDelay = RetargetDelay;
            state.chaseTime = 0;
            return false;
        }
        goTo(state, target, range, false, false);
        return true;
    }
    state.chaseTime = 0;
    state.clearPath();
    state.targetId = 0;
    state.speed = 0.0f;
    state.acceleration = plasma::Vector<3, float>(0, 0, 0);
    if (!pacified &&
        !(creature->m_entityData.appearance.appearanceFlags & (AppearanceFlagTinted | AppearanceFlagLarge)) &&
        target->m_entityData.currentAbility == AbilityOfferPetFood &&
        target->m_entityData.actionItem.category == ItemCategoryPetFood &&
        target->m_entityData.actionItem.id == creature->m_entityData.race &&
        creature->m_entityData.hostilityType != HostilityPet &&
        (!creature->m_entityData.ownerId ||
         (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagSummoned)) &&
        range * range >= squaredDistance) {
        creature->m_opponentId = target->m_id;
        if (creature->m_entityData.timeSinceAbility > (float)creature->attackDuration()) {
            if (creature->m_entityData.currentAbility != AbilityEatPetFood)
                creature->m_entityData.hitCombo = 0;
            if (creature->m_entityData.hitCombo < PetFeedingCount) {
                creature->m_entityData.currentAbility = AbilityEatPetFood;
                creature->m_entityData.timeSinceAbility = 0.0f;
            } else {
                Creature::Buff copied;
                bool hasBuff = false;
                for (const auto &buff : creature->m_buffs) {
                    if (buff.type == BuffTameness) {
                        hasBuff = true;
                        copied = buff;
                    }
                }
                if (hasBuff) {
                    copied.strength = 0.0f;
                    copied.timeRemaining = 0.0f;
                    m_world->applyBuff(creature, copied, true);
                }
                copied.type = BuffTamingAnger;
                copied.timeRemaining = TamingAngerDuration;
                copied.strength = 0.0f;
                copied.sourceId = target->m_id;
                m_world->applyBuff(creature, copied, true);
            }
        }
        return true;
    }
    if (!visible) {
        if (creature->m_entityData.currentAbility == AbilityFireRay ||
            creature->m_entityData.currentAbility == AbilitySupportRay)
            creature->m_entityData.currentAbility = AbilityNone;
        return true;
    }
    difference = target->m_entityData.position - creature->m_entityData.position;
    squaredDistance = difference.squaredLength();
    pet = m_world->creatureByID(creature->m_petId);
    if (pet && pet->m_entityData.race == RaceManaMech && pet->m_entityData.hp > 0.0f) {
        creature->m_entityData.currentAbility = AbilityRidePet;
        creature->m_entityData.timeSinceAbility = 0.0f;
        creature->m_entityData.attackRotation = difference;
        return true;
    }
    if (creature->m_entityData.currentAbility == AbilitySmash) {
        if (creature->m_entityData.timeSinceAbility > (float)creature->attackDuration() &&
            (creature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater)))
            creature->m_entityData.currentAbility = AbilityNone;
        creature->m_entityData.attackRotation = difference;
        return true;
    }
    if (specialSkill) {
        std::cout << "SUPER SKILL " << specialSkill << std::endl;
        if (specialSkill != AbilityRestHeal || (creature->m_entityData.flags & CreatureFlagOnGround)) {
            state.specialSkillDelay = SpecialSkillDelay;
            creature->m_entityData.currentAbility = (unsigned char)specialSkill;
            creature->m_entityData.timeSinceAbility = 0.0f;
            if (specialSkill == AbilitySmash) {
                // Doubles fixed-point differences with signed shifts before dividing.
                plasma::Vector<3, Fixed> flight = target->m_entityData.position - creature->m_entityData.position;
                // Doubles the fixed-point flight displacement and converts each coordinate to blocks.
                creature->m_entityData.velocity = flight * plasma::Vector<3, Fixed>(Fixed(2), Fixed(2), Fixed(2));

                creature->m_leapDuration =
                    (sqrtf(creature->m_entityData.velocity.squaredLength()) / SmashFlightSpeed) * 1000.0f;
                creature->m_entityData.velocity[2] = SmashLaunchSpeed;
            }
        }
        creature->m_entityData.attackRotation = difference;
        return true;
    }
    if (creature->isWraithTargeting()) {
        creature->m_entityData.attackRotation = difference;
        return true;
    }
    if (creature->m_entityData.race == RaceMammoth) {
        if (creature->m_entityData.timeSinceAbility > (float)creature->attackDuration()) {
            std::vector<int> skills;
            skills.push_back(AbilityHornCharge);
            skills.push_back(AbilityStomp);
            skills.push_back(AbilityScream);
            creature->m_entityData.currentAbility = (unsigned char)skills[rand() % skills.size()];
            creature->m_entityData.timeSinceAbility = 0.0f;
        }
        creature->m_entityData.attackRotation = difference;
        return true;
    }
    if (creature->m_entityData.race == RaceRuneGiant) {
        if (creature->m_entityData.timeSinceAbility > (float)creature->attackDuration()) {
            std::vector<int> skills;
            skills.push_back(AbilityLeftStab);
            skills.push_back(AbilityRightStab);
            skills.push_back(AbilityKick);
            if (creature->m_entityData.appearance.headModel == SpriteRuneGiantHeadLaser)
                skills.push_back(AbilityFireRay);
            else
                skills.push_back(AbilityCyclone);
            creature->m_entityData.currentAbility = (unsigned char)skills[rand() % skills.size()];
            creature->m_entityData.timeSinceAbility = 0.0f;
        }
        if (creature->m_entityData.currentAbility == AbilityFireRay)
            difference = target->m_entityData.position - creature->sightPosition();
        creature->m_entityData.attackRotation = difference;
        return true;
    }
    if (creature->m_entityData.race == RaceSaurian) {
        if (creature->m_entityData.timeSinceAbility > (float)creature->attackDuration()) {
            std::vector<int> skills;
            if (creature->m_entityData.appearance.headModel == SpriteMonsterHeadReptileHorn1 ||
                creature->m_entityData.appearance.headModel == SpriteMonsterHeadReptileHorn2 ||
                creature->m_entityData.appearance.headModel == SpriteMonsterHeadReptileHorn3)
                skills.push_back(AbilityHornCharge);
            else
                skills.push_back(AbilityHeadbutt);
            if (creature->m_entityData.appearance.tailModel >= SpriteMonsterTailReptileSpike1 &&
                creature->m_entityData.appearance.tailModel <= SpriteMonsterTailReptileSpike2)
                skills.push_back(AbilityTailStrike);
            if (creature->m_entityData.appearance.handsModel == SpriteMonsterFootReptileClaw1 ||
                creature->m_entityData.appearance.handsModel == SpriteMonsterFootReptileClaw2)
                skills.push_back(AbilityClawStrike);
            else
                skills.push_back(AbilityStomp);
            if (creature->m_entityData.appearance.chestModel >= SpriteMonsterBodyReptileSpiked1 &&
                creature->m_entityData.appearance.chestModel <= SpriteMonsterBodyReptileSpiked2) {
                skills.push_back(AbilityDashLeft);
                skills.push_back(AbilityDashRight);
            }
            if (creature->m_entityData.appearance.chestModel >= SpriteMonsterBodyReptileCrest1 &&
                creature->m_entityData.appearance.chestModel <= SpriteMonsterBodyReptileCrest5)
                skills.push_back(AbilityDashForward);
            if ((unsigned char)(creature->m_entityData.currentAbility - AbilityShellBlock) > 1 &&
                creature->m_entityData.appearance.chestModel == SpriteMonsterBodyReptileShell && rand() % 2)
                skills.push_back(AbilityShellBlock);
            if ((unsigned char)(creature->m_entityData.currentAbility - AbilityShellBlock) > 1 &&
                creature->m_entityData.appearance.chestModel == SpriteMonsterBodyReptileShellSpiked && rand() % 2)
                skills.push_back(AbilityShellSpin);
            if (creature->m_entityData.currentAbility != AbilityScream)
                skills.push_back(AbilityScream);
            if (!skills.empty()) {
                creature->m_entityData.currentAbility = (unsigned char)skills[rand() % skills.size()];
                creature->m_entityData.timeSinceAbility = 0.0f;
            }
        }
        creature->m_entityData.attackRotation = difference;
        return true;
    }
    if (creature->isCaster()) {
        if (squaredDistance < RangedRangeSquared) {
            if (creature->m_entityData.mp > 0.5f && rand() % 2)
                creature->performSpecialAttack(false);
            else
                creature->performAttack(false);
            if ((unsigned char)(creature->m_entityData.currentAbility - AbilityFireSwirl) <= 3) {
                float previousTime = creature->m_entityData.timeSinceAbility - (float)milliseconds;
                if ((float)creature->anticipationTime(AbilityUseCurrent) >= previousTime)
                    creature->m_abilityTargetPosition = target->m_drawnPosition;
                else
                    difference = creature->m_abilityTargetPosition - creature->m_entityData.position;
            }
            // A support ray beyond five blocks circles its aim (radius 1.5, 0.005 radians per millisecond).
            if (creature->m_entityData.currentAbility == AbilitySupportRay && squaredDistance > 25.0f) {
                float angle = creature->m_entityData.timeSinceAbility * 0.005f;
                difference[0] += cosf(angle) * 1.5f;
                difference[1] += sinf(angle) * 1.5f;
            }
        }
        creature->m_entityData.attackRotation = difference;
        return true;
    }
    // A dragon within eight blocks (squared 64) bites, rising (30) while at most eight above or diving (10) every two
    // seconds; further away it throws fireballs at 40% of the target's height. A flying biter does the same with a
    // rise of 5 at most two above.
    if (creature->m_entityData.race == RaceDragon) {
        if (squaredDistance < 64.0f) {
            creature->m_entityData.attackRotation = target->m_entityData.position - creature->m_entityData.position;
            if (creature->m_entityData.timeSinceAbility > 2000.0f) {
                if (rand() % 2 && (creature->m_entityData.position[2] - target->m_entityData.position[2]).m_raw <=
                                      Fixed(8.0f).m_raw) {
                    creature->m_entityData.velocity[2] = 30.0f;
                } else {
                    creature->m_entityData.timeSinceAbility = 0.0f;
                    creature->m_entityData.currentAbility = AbilityDiveBite;
                    creature->m_entityData.velocity = target->m_entityData.position - creature->m_entityData.position;
                    if (creature->m_entityData.velocity.squaredLength() > 0.0f) {
                        creature->m_entityData.velocity.normalize();
                        creature->m_entityData.velocity *= 10.0f;
                    }
                }
            }
        } else if (squaredDistance < RangedRangeSquared &&
                   creature->m_entityData.timeSinceAbility > (float)creature->attackDuration()) {
            creature->m_entityData.timeSinceAbility = 0.0f;
            creature->m_entityData.currentAbility = AbilityFireball;
            plasma::Vector<3, Fixed> aim = target->m_entityData.position - creature->m_entityData.position;
            aim[2] += Fixed(target->m_entityData.appearance.size[2] * 0.4f);
            creature->m_entityData.attackRotation = aim;
        }
        creature->m_entityData.attackRotation = difference;
        return true;
    }
    if (creature->m_entityData.race == RaceBloomling) {
        creature->performAttack(false);
        creature->m_entityData.attackRotation = difference;
        return true;
    }
    if ((creature->m_entityData.appearance.appearanceFlags & AppearanceFlagFlying) &&
        (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagBiteAttack)) {
        if (squaredDistance < 64.0f) {
            creature->m_entityData.attackRotation = target->m_entityData.position - creature->m_entityData.position;
            if (creature->m_entityData.timeSinceAbility > 2000.0f) {
                if (rand() % 2 && (creature->m_entityData.position[2] - target->m_entityData.position[2]).m_raw <=
                                      Fixed(2.0f).m_raw) {
                    creature->m_entityData.velocity[2] = 5.0f;
                } else {
                    creature->m_entityData.timeSinceAbility = 0.0f;
                    creature->m_entityData.currentAbility = AbilityDiveBite;
                    creature->m_entityData.velocity = target->m_entityData.position - creature->m_entityData.position;
                    if (creature->m_entityData.velocity.squaredLength() > 0.0f) {
                        creature->m_entityData.velocity.normalize();
                        creature->m_entityData.velocity *= 10.0f;
                    }
                }
            }
        }
        creature->m_entityData.attackRotation = difference;
        return true;
    }
    bool rangedWeapon = creature->m_entityData.classType == ClassMonsterGunner ||
                        creature->m_entityData.classType == ClassMonsterArcher ||
                        (unsigned int)(creature->m_entityData.equipment.weaponRight.id - WeaponBow) <= 2;
    bool dodge = false;
    if (!rangedWeapon) {
        float meleeRange =
            (target->m_entityData.appearance.size[0] + creature->m_entityData.appearance.size[0]) * MeleeRangeFactor;
        if (!(squaredDistance < meleeRange * meleeRange)) {
            creature->m_entityData.attackRotation = difference;
            return true;
        }
        unsigned char offset = creature->m_entityData.currentAbility - AbilityBlock;
        // Dodging requires a small, recovered creature on the ground. Negated size and time tests also reject NaN
        // values.
        if ((offset <= (AbilityMonsterCharge - AbilityBlock) && (AiDodgeBlockingAbilities & (1ULL << offset))) ||
            creature->m_entityData.rollTime != 0.0f || rand() % DodgeOdds != 0 || creature->isLarge() ||
            !(creature->m_entityData.appearance.size[0] < DodgeMaxWidth) ||
            !(creature->m_entityData.timeSinceAbility >= (float)creature->attackDuration()) ||
            !(creature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater))) {
            offset = creature->m_entityData.currentAbility - AbilityBlock;
            if ((double)creature->m_entityData.mp < ChargeManaThreshold &&
                (offset > (AbilitySwirl - AbilityBlock) || !(AiChargingAbilities & (1ULL << offset)))) {
                creature->performAttack(false);
                creature->m_entityData.attackRotation = difference;
                return true;
            }
            if (creature->m_entityData.rollTime != 0.0f ||
                !(creature->m_entityData.timeSinceAbility > (float)creature->attackDuration())) {
                creature->m_entityData.attackRotation = difference;
                return true;
            }
            int skill = creature->nextSpecialAttackSkill(false);
            if (skill == AbilityPerforate) {
                creature->m_entityData.currentAbility = AbilityPerforate;
                creature->m_entityData.timeSinceAbility = 0.0f;
            } else if (skill == AbilityCrush) {
                if (creature->m_entityData.currentAbility == AbilityCrush &&
                    creature->m_entityData.charge >= creature->m_entityData.mp) {
                    if (creature->m_entityData.specialization == SpecializationGuardian) {
                        creature->m_entityData.currentAbility = AbilityGuardianCrushStrike;
                        creature->m_entityData.timeSinceAbility = 0.0f;
                    } else {
                        creature->m_entityData.currentAbility = AbilityCrushStrike;
                        creature->m_entityData.timeSinceAbility = (float)creature->anticipationTime(AbilityUseCurrent);
                    }
                } else {
                    if (creature->m_entityData.currentAbility != AbilityCrush)
                        creature->m_entityData.timeSinceAbility = 0.0f;
                    creature->m_entityData.currentAbility = AbilityCrush;
                }
            } else if (skill == AbilityBlock) {
                if (creature->m_entityData.currentAbility == AbilityBlock &&
                    creature->m_entityData.charge >= creature->m_entityData.mp) {
                    creature->m_entityData.currentAbility = AbilityBlockStrike;
                    creature->m_entityData.timeSinceAbility = 0.0f;
                } else {
                    if (creature->m_entityData.currentAbility != AbilityBlock)
                        creature->m_entityData.timeSinceAbility = 0.0f;
                    creature->m_entityData.currentAbility = AbilityBlock;
                }
            } else if (skill == AbilityMonsterCharge) {
                if (creature->m_entityData.currentAbility == AbilityMonsterCharge &&
                    creature->m_entityData.charge >= creature->m_entityData.mp) {
                    creature->m_entityData.currentAbility = AbilityMonsterChargeStrike;
                    creature->m_entityData.timeSinceAbility = (float)creature->anticipationTime(AbilityUseCurrent);
                } else {
                    if (creature->m_entityData.currentAbility != AbilityMonsterCharge)
                        creature->m_entityData.timeSinceAbility = 0.0f;
                    creature->m_entityData.currentAbility = AbilityMonsterCharge;
                }
            } else if (skill == AbilitySwirl) {
                if (creature->m_entityData.currentAbility == AbilitySwirl &&
                    creature->m_entityData.charge >= creature->m_entityData.mp) {
                    creature->m_entityData.currentAbility = AbilitySwirlStrike;
                    creature->m_entityData.timeSinceAbility = (float)creature->anticipationTime(AbilityUseCurrent);
                } else if (creature->m_entityData.currentAbility != AbilitySwirl) {
                    creature->m_entityData.timeSinceAbility = 0.0f;
                    creature->m_entityData.currentAbility = AbilitySwirl;
                }
            } else if (skill == AbilitySalvo) {
                if (creature->m_entityData.currentAbility == AbilitySalvo &&
                    creature->m_entityData.charge >= creature->m_entityData.mp) {
                    creature->m_entityData.currentAbility = AbilitySalvoRelease;
                    creature->m_entityData.timeSinceAbility = (float)creature->anticipationTime(AbilityUseCurrent);
                } else if (creature->m_entityData.currentAbility != AbilitySalvo) {
                    creature->m_entityData.timeSinceAbility = 0.0f;
                    creature->m_entityData.currentAbility = AbilitySalvo;
                }
            } else if (skill == AbilityChargedShot) {
                if (creature->m_entityData.currentAbility == AbilityChargedShot &&
                    creature->m_entityData.charge >= creature->m_entityData.mp) {
                    creature->m_entityData.currentAbility = AbilityShoot;
                    creature->m_entityData.timeSinceAbility = (float)creature->anticipationTime(AbilityUseCurrent);
                } else if (creature->m_entityData.currentAbility != AbilityChargedShot) {
                    creature->m_entityData.timeSinceAbility = 0.0f;
                    creature->m_entityData.currentAbility = AbilityChargedShot;
                }
            } else if (skill == AbilityChargedBoomerang) {
                if (creature->m_entityData.currentAbility == AbilityChargedBoomerang) {
                    if (creature->m_entityData.charge >= creature->m_entityData.mp) {
                        creature->m_entityData.currentAbility = AbilityThrowBoomerang;
                        creature->m_entityData.timeSinceAbility = (float)creature->anticipationTime(AbilityUseCurrent);
                    }
                } else {
                    creature->m_entityData.timeSinceAbility = 0.0f;
                    creature->m_entityData.currentAbility = AbilityThrowBoomerang;
                }
            } else {
                creature->performSpecialAttack(false);
            }
        } else {
            dodge = true;
        }
    } else {
        if (!(squaredDistance < RangedRangeSquared)) {
            creature->m_entityData.attackRotation = difference;
            return true;
        }
        // Negated size and time tests reject NaN values.
        if ((((creature->m_entityData.currentAbility - AbilityChargedShot) & AiFourAbilityIntervalMask) == 0 &&
             creature->m_entityData.currentAbility != AbilityThrowBoomerang) ||
            creature->m_entityData.rollTime != 0.0f || rand() % DodgeOdds != 0 || creature->isLarge() ||
            !(creature->m_entityData.appearance.size[0] < DodgeMaxWidth) ||
            !(creature->m_entityData.timeSinceAbility >= (float)creature->attackDuration()) ||
            !(creature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater))) {
            if ((double)creature->m_entityData.mp < ChargeManaThreshold &&
                (((creature->m_entityData.currentAbility - AbilityChargedShot) & AiFourAbilityIntervalMask) != 0 ||
                 creature->m_entityData.currentAbility == AbilityThrowBoomerang)) {
                creature->performAttack(false);
                creature->m_entityData.attackRotation = difference;
                return true;
            }
            if (creature->m_entityData.rollTime != 0.0f ||
                !(creature->m_entityData.timeSinceAbility > (float)creature->attackDuration())) {
                creature->m_entityData.attackRotation = difference;
                return true;
            }
            int skill = creature->nextSpecialAttackSkill(false);
            if (skill == AbilityChargedShot) {
                if (creature->m_entityData.currentAbility == AbilityChargedShot &&
                    creature->m_entityData.charge >=
                        (std::min)(0.8f, creature->m_entityData.mp * RangedChargeManaFraction)) {
                    creature->m_entityData.currentAbility = AbilityShoot;
                    creature->m_entityData.timeSinceAbility = 0.0f;
                } else {
                    if (creature->m_entityData.currentAbility != AbilityChargedShot)
                        creature->m_entityData.timeSinceAbility = 0.0f;
                    creature->m_entityData.currentAbility = AbilityChargedShot;
                }
            } else if (skill == AbilityChargedBoomerang) {
                if (creature->m_entityData.currentAbility == AbilityChargedBoomerang &&
                    creature->m_entityData.charge >=
                        (std::min)(0.8f, creature->m_entityData.mp * RangedChargeManaFraction)) {
                    creature->m_entityData.currentAbility = AbilityThrowBoomerang;
                    creature->m_entityData.timeSinceAbility = 0.0f;
                } else {
                    if (creature->m_entityData.currentAbility != AbilityChargedBoomerang)
                        creature->m_entityData.timeSinceAbility = 0.0f;
                    creature->m_entityData.currentAbility = AbilityChargedBoomerang;
                }
            } else if (skill == AbilitySalvo) {
                if (creature->m_entityData.currentAbility == AbilitySalvo &&
                    creature->m_entityData.charge >=
                        (std::min)(0.8f, creature->m_entityData.mp * RangedChargeManaFraction)) {
                    creature->m_entityData.currentAbility = AbilitySalvoRelease;
                    creature->m_entityData.timeSinceAbility = 0.0f;
                } else {
                    if (creature->m_entityData.currentAbility != AbilitySalvo)
                        creature->m_entityData.timeSinceAbility = 0.0f;
                    creature->m_entityData.currentAbility = AbilitySalvo;
                }
            } else {
                creature->performSpecialAttack(false);
            }
        } else {
            dodge = true;
        }
    }
    if (dodge) {
        plasma::Vector<3, float> direction(
            (float)(target->m_entityData.position[0] - creature->m_entityData.position[0]),
            (float)(target->m_entityData.position[1] - creature->m_entityData.position[1]), 0.0f);
        if (direction.squaredLength() > 0.0f) {
            direction.normalize();
            switch (rand() % 4) {
            case 0:
                creature->m_entityData.velocity = direction * DodgeSpeed;
                break;
            case 1:
                creature->m_entityData.velocity = direction * -DodgeSpeed;
                break;
            case 2:
                creature->m_entityData.velocity = direction.cross(plasma::Vector<3, float>(0, 0, 1)) * DodgeSpeed;
                break;
            case 3:
                creature->m_entityData.velocity = direction.cross(plasma::Vector<3, float>(0, 0, 1)) * -DodgeSpeed;
                break;
            }
            creature->m_entityData.velocity[2] += DodgeLift;
            creature->m_entityData.rollTime = DodgeRollTime;
        }
    }
    creature->m_entityData.attackRotation = difference;
    return true;
}

}
