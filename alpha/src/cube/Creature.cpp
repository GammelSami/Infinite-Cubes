// Creature construction, cleanup and state management.
#include "cube/Creature.h"
#include "cube/Behavior.h"
#include "cube/RaceIds.h"

namespace cube {

namespace {
enum {
    InitialEntityHp = 500,
};
}

Creature::Creature(const long long &id)
    : m_id(id),
      m_stepOffset(0),
      m_hitFlash(0),
      m_walkAmplitude(0),
      m_walkPhase(0),
      m_stealth(0),
      m_stamina(1.0f),
      m_ridingStamina(1.0f),
      unused119c(0),
      m_petId(0),
      m_companionId(0),
      m_opponentId(0),
      m_leapDuration(0),
      m_swingCount(0),
      m_rollTime(0),
      m_drawnPosition(Fixed(0), Fixed(0), Fixed(0)),
      m_drawnRotation(0, 0, 0),
      m_smoothingTime(0),
      m_attackPower(0),
      m_strikeLanded(0),
      m_fallStartHeight(0),
      m_criticalStrike(0),
      m_behaviorTime(0),
      m_damageDealt(0),
      m_damageDealtTime(0),
      m_timeSinceDamageDealt(0),
      unused13d4(0),
      unused13d8(1.0f),
      unused13dc(0),
      unused13e0(0),
      m_behavior(0),
      m_summonAppearanceMode(SummonCopySummoner),
      unused1404(0),
      m_nearestPathNode(),
      m_pathStartBlock(-1, -1, -1),
      m_pathRadius(0),
      m_pathSearchTime(0),
      m_interactionTarget(-1, -1, 0),
      m_scheduleReached(0),
      m_scheduleEntry(0),
      m_animationState(this),
      m_playerData(0),
      m_despawnDistance(200.0f),
      m_dungeonMissionTarget(0),
      m_shownLevel(1),
      m_shownXp(0),
      m_lastPetSkillLevel(0)
{
    // Is copied as a saved dword; its initial bits represent 1.0f.
}

Creature::EntityData::EntityData()
    : position(Fixed(0), Fixed(0), Fixed(0)),
      rotation(0, 0, 0),
      velocity(0, 0, 0),
      acceleration(0, 0, 0),
      retreat(0, 0, 0),
      headRotation(0),
      flags(0),
      hostilityType(HostilityFriendlyNpc),
      race(RaceElfMale),
      currentAbility(AbilityNone),
      timeSinceAbility(0),
      hitCombo(0),
      timeSinceHit(0),
      binaryToggles(),
      rollTime(0),
      stunTime(-StunRecoveryDelayMs),
      actionLockTime(0),
      slowedTime(0),
      sprintTime(0),
      revivalPenalty(0),
      classType(ClassNone),
      specialization(SpecializationFirst),
      charge(0),
      finalHandsRotation(0, 0, 0),
      initialHandsRotation(0, 0, 0),
      attackRotation(0, 0, 0),
      hp(InitialEntityHp),
      mp(0),
      blockPower(),
      interactionState(0),
      buildingModel(0),
      level(1),
      xp(0),
      ownerId(0),
      focusId(0),
      rarity(ItemRarityWhite),
      unused19c(-1),
      spawnSlot(-1, -1, 0),
      speechExpression(0),
      questZone(-1, -1, 0),
      manaCubeCount(),
      name()
{
    // Origin is deliberately left unset.
    memset(skillLevels, 0, sizeof(skillLevels));
}

Creature::~Creature()
{
    // The code deletes through Behavior, whose destructor is not virtual.
    if (m_behavior)
        delete m_behavior;
    delete m_playerData;
}

}
