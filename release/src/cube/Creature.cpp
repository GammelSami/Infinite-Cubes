#include <algorithm>
#include "cube/Creature.h"

#include <algorithm>
#include <cstdlib>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cube/ServerUpdates.h"
#include "cube/World.h"
#include "plasma/Matrix.h"
#include "cube/CreatureIds.h"
#include "cube/ItemIds.h"
#include "cube/SpriteIds.h"
#include "mod/ClassData.h"
#include "mod/Hooks.h"
#include "mod/Progress.h"

namespace cube {

Creature::Creature(const long long &id)
    : m_id(id),
      m_stepOffset(0),
      m_hitFlash(0),
      m_walkAmplitude(0),
      m_walkPhase(0),
      m_stamina(1.0f),
      m_petWaterLevel(1.0f),
      m_unreadSavedInt(0),
      m_petId(0),
      m_opponentId(0),
      m_leapDuration(0),
      m_strikeCombo(0),
      m_rollTime(0),
      m_drawnPosition(0, 0, 0),
      m_drawnRotation(0, 0, 0),
      m_drawnAttackRotation(0, 0, 0),
      m_smoothingTime(0),
      m_artifactSkills {0},
      m_strikeStrength(0),
      m_strikeLanded(0),
      m_fallStartHeight(0),
      m_glideTime(0),
      m_criticalStrike(0),
      m_airTime(0),
      m_damageDealt(0),
      m_unusedBc4(0),
      m_resetOnHit(0),
      m_behaviours(0),
      m_summonAppearanceMode(SummonCopySummoner),
      m_unusedC18(0),
      m_animationState(this),
      m_playerData(nullptr),
      m_copiedToPet(200.0f),
      m_unused1540(0),
      m_lastPetSyncValue(0)
{}

Creature::EntityData::Appearance::Appearance()
    : unsetPair(0, 0),
      hairColor(255, 255, 255),
      appearanceFlags(0),
      size(1.0f, 1.0f, 1.0f),
      headModel(-1),
      hairModel(-1),
      handsModel(-1),
      feetModel(-1),
      chestModel(-1),
      tailModel(-1),
      shoulderModel(-1),
      wingsModel(-1),
      headScale(1.01f),
      chestScale(1.0f),
      handsScale(1.0f),
      feetScale(0.98f),
      shoulderModelScale(1.0f),
      weaponScale(0.95f),
      tailScale(0.8f),
      shoulderArmorScale(1.0f),
      wingScale(1.0f),
      chestRotation(0),
      handsRotation(0, 0, 0),
      feetRotation(0),
      wingsRotation(0),
      tailRotation(0),
      chestPosition(0, 0, -5.0f),
      headPosition(0, 0.5f, 5.0f),
      handsPosition(6.0f, 0, 0),
      feetPosition(3.0f, 1.0f, -10.5f),
      tailPosition(0, -8.0f, 2.0f),
      wingsPosition(0, 0, 0)
{}

// Initialises all eleven equipment items.
Creature::EntityData::EntityData()
    : position(0, 0, 0),
      rotation(0, 0, 0),
      velocity(0, 0, 0),
      acceleration(0, 0, 0),
      retreat(0, 0, 0),
      headRotation(0),
      flags(0),
      hostilityType(3),
      race(RaceElfMale),
      currentAbility(0),
      timeSinceAbility(0),
      hitCombo(0),
      timeSinceHit(0),
      binaryToggles(),
      rollTime(0),
      stunTime(-3000.0f),
      actionLockTime(0),
      slowedTime(0),
      sprintTime(0),
      revivalPenalty(0),
      level(1),
      xp(0),
      classType(0),
      specialization(0),
      region(0, 0),
      charge(0),
      finalHandsRotation(0, 0, 0),
      initialHandsRotation(0, 0, 0),
      attackRotation(0, 0, 0),
      hp(500.0f),
      blockPower(1.0f),
      mp(0),
      stealth(0),
      petRefreshValue(0),
      mediumSpeed(1.0f),
      lampDiameter(10.0f),
      interactionState(0),
      buildingModel(0),
      ownerId(0),
      focusId(0),
      destination(0x7fffffff, 0x7fffffff),
      portCity(-1, -1),
      seed(0),
      factionKind(0),
      factionVariant(0),
      factionSeed(0)
{
    memset(name, 0, sizeof(name));
}

Creature::~Creature()
{
    delete m_playerData;
}

int Creature::anticipationTime(int ability)
{
    if (ability < 0) {
        ability = m_entityData.currentAbility;
    }
    switch (ability) {
    case AbilityNone:
    case AbilityWatchPlayer:
    case AbilityBlock:
    case AbilitySwirlStrike:
    case AbilityRetreat:
    case AbilitySalvoRelease:
    case AbilityCrushStrike:
    case AbilityGuardianCrushStrike:
    case Ability_63:
    case AbilitySneak:
    case AbilitySprint:
    case AbilityDash:
    case AbilityCyclone:
    case AbilityParry:
    case AbilityBlockStrike:
    case AbilityRidePet:
    case AbilityHangGlide:
    case AbilitySailBoat:
    case AbilityClimb:
    case AbilityFlameRush:
    case AbilityHeroicShout:
    case AbilityCallPet:
    case AbilityBloomlingAttack:
    case AbilityWarriorJumpAttack:
        return 0;
    case AbilityPortCity:
    case AbilityWave:
        return 3000;
    case AbilityUseSpecialItem:
    case AbilityPetAnimal:
    case AbilityDance:
        return 5000;
    case AbilityCrossedWeaponSweep:
    case AbilityLeftSweep:
    case AbilityOverheadChop:
    case AbilityMonsterChargeStrike:
    case AbilityFirebolt:
    case AbilityBraceletFirebolt:
    case AbilityWaterbolt:
    case AbilityBraceletWaterbolt:
    case AbilityTwoHandedSwing:
    case AbilityRockFist:
        return (int)(200.0f / tempo());
    case AbilityChargedShotRelease:
    case AbilityChargedShot:
    case AbilitySalvo:
    case AbilityChargedBoomerang:
    case AbilityFireballCharge:
    case AbilityCrush:
    case AbilitySwirl:
    case AbilityMonsterCharge:
    case AbilityRockFistCharge:
        return (int)(50.0f / tempo());
    case AbilityDjinnExplosion:
        return (int)(5000.0f / tempo());
    case AbilityQuake:
    case AbilityFireRay:
    case AbilityWaterRay:
        return (int)(1000.0f / tempo());
    case AbilityMonsterFirebolt:
    case AbilityMonsterWaterbolt:
    case AbilityTwoHandedSpin:
    case AbilityTwoHandedSlam:
    case AbilityFireBeam:
    case AbilityWaterBeam:
        return (int)(500.0f / tempo());
    case AbilityFireball:
    case AbilityWaterball:
    case AbilityLavaPuddle:
        if (m_entityData.equipment.weaponRight.id == WeaponBracelet) {
            return (int)(600.0f / tempo());
        }
        return (int)(1200.0f / tempo());
    case AbilityHealingStream:
    case AbilityCenteredHealingStream:
    case AbilityFireBreath:
        return (int)(2000.0f / tempo());
    case AbilityWaterSalvo:
    case Ability_48:
        return (int)(1200.0f / tempo());
    case AbilityWaterBubbles:
        return (int)(1500.0f / tempo());
    case AbilityFireSwirl:
    case AbilityWaterSwirl:
    case AbilityTwoHandedSweep:
    case AbilityTwoHandedThrust:
    case AbilityLunge:
        return (int)(800.0f / tempo());
    case AbilityFireVortex:
    case AbilityWaterVortex:
    case AbilityHeal:
        return (int)(1600.0f / tempo());
    case AbilityLeftSlash:
    case Ability_4:
    case Ability_5:
    case AbilityPerforate:
    case AbilityRightStab:
    case AbilityLeftStab:
    case AbilityBlockJab:
    case AbilityRightSweep:
    case AbilityAmbush:
    case AbilityLeftDaggerPierce:
    case AbilityRightDaggerPierce:
    case AbilityKick:
    case AbilityRangerKick:
        return (int)(100.0f / tempo());
    case AbilityRightSlash:
    case AbilityShieldAttack:
    case AbilityThrowBoomerang:
    case AbilityHeadbutt:
    case AbilityHornCharge:
    case AbilityTailStrike:
    case AbilityScream:
    case AbilityStomp:
    case AbilityClawStrike:
    case AbilityDashForward:
    case AbilityDashLeft:
    case AbilityDashRight:
        return (int)(300.0f / tempo());
    case AbilityShuriken:
        if (findBuff(BuffNinjutsu)) {
            return 100;
        }
        return (int)(200.0f / tempo());
    case AbilitySupportRay:
    case AbilityIntercept:
    case AbilityBulwark:
    case AbilityAssassinate:
    case AbilityQuicksandTrap:
    case AbilityShadowClone:
        return 100;
    case AbilitySmash:
        return 400;
    case AbilityShellBlock:
    case AbilityShellSpin:
        return 200;
    case AbilitySelfKill:
        return 60000;
    case AbilityWraithAttack:
        return 10;
    case AbilityFeedClimbingPlant:
        return 2000;
    case AbilityFlightToPoint:
        return 120000;
    case AbilityShoot:
        return (int)(400.0f / tempo());
    default:
        return (int)(400.0f / tempo());
    }
}

void Creature::applyDodgeBonus(ServerUpdates &updates)
{
    if (m_entityData.rollTime != 0.0f && m_entityData.classType == ClassRogue &&
        m_entityData.specialization == SpecializationNinja) {
        m_entityData.mp += 0.25f;
        if (m_entityData.mp > 1.0f) {
            m_entityData.mp = 1.0f;
        }
        Buff buff;
        buff.type = BuffDodge;
        buff.timeRemaining = 30000.0f;
        setBuff(buff, false);
        ServerUpdates::BuffPacket packet;
        packet.replace = 0;
        packet.buff = buff;
        packet.sourceId = m_id;
        packet.targetId = m_id;
        updates.m_buffPacketList.push_back(packet);
    }
}

bool areEnemies(const Creature *a, const Creature *b)
{
    const Creature::EntityData &dataA = a->m_entityData;
    const Creature::EntityData &dataB = b->m_entityData;
    if ((dataA.hostilityType == HostilityPlayer || dataA.hostilityType == HostilityFriendlyNpc ||
         dataA.hostilityType == HostilityPet) &&
        (dataB.hostilityType == HostilityPlayer || dataB.hostilityType == HostilityFriendlyNpc ||
         dataB.hostilityType == HostilityPet)) {
        return false;
    }
    if (dataA.classType == ClassArenaChampion || dataB.classType == ClassArenaChampion) {
        return false;
    }
    if (dataA.race == RaceEagle || dataB.race == RaceEagle) {
        return false;
    }
    if (dataA.binaryToggles.active || dataB.binaryToggles.active) {
        return false;
    }
    if (dataA.hostilityType == HostilityAttackableObject || dataB.hostilityType == HostilityAttackableObject) {
        return false;
    }
    if (dataA.hostilityType == HostilityInteractiveObject || dataB.hostilityType == HostilityInteractiveObject) {
        return false;
    }
    if (dataA.hostilityType == HostilityPlayer && dataA.binaryToggles.dodge) {
        return false;
    }
    if (dataB.hostilityType == HostilityPlayer && dataB.binaryToggles.dodge) {
        return false;
    }
    if (dataA.binaryToggles.combatSideOverride || dataB.binaryToggles.combatSideOverride) {
        return true;
    }
    bool sideA = false;
    bool sideB = true;
    if (dataA.hostilityType != HostilityEnemy || dataA.appearance.hasFlag(AppearanceFlagMarked)) {
        sideA = true;
    }
    if (dataB.hostilityType == HostilityEnemy && !dataB.appearance.hasFlag(AppearanceFlagMarked)) {
        sideB = false;
    }
    if (sideA != sideB) {
        return true;
    }
    return false;
}

float Creature::armor()
{
    float armor = 0.0f;
    Equipment &equipment = m_entityData.equipment;
    if (equipment.chest.category == ItemCategoryChest &&
        (m_entityData.hostilityType != HostilityPlayer || equipment.chest.fitsClass(m_entityData.classType))) {
        armor += equipment.chest.armor(this);
    }
    if (equipment.feet.category == ItemCategoryFeet &&
        (m_entityData.hostilityType != HostilityPlayer || equipment.feet.fitsClass(m_entityData.classType))) {
        armor += equipment.feet.armor(this);
    }
    if (equipment.hands.category == ItemCategoryHands &&
        (m_entityData.hostilityType != HostilityPlayer || equipment.hands.fitsClass(m_entityData.classType))) {
        armor += equipment.hands.armor(this);
    }
    if (equipment.shoulder.category == ItemCategoryShoulder &&
        (m_entityData.hostilityType != HostilityPlayer || equipment.shoulder.fitsClass(m_entityData.classType))) {
        armor += equipment.shoulder.armor(this);
    }
    if (m_entityData.appearance.appearanceFlags & (AppearanceFlagTinted | AppearanceFlagEnhanced)) {
        armor *= 1.2f;
    }
    if (m_entityData.appearance.hasFlag(AppearanceFlagLarge)) {
        armor *= 2.0f;
    }
    if (findBuff(BuffDefenseWeakened)) {
        armor *= 0.25f;
    }
    Buff *buff = findBuff(BuffArmorflask);
    if (buff) {
        armor *= buff->strength;
    }
    return armor;
}

int Creature::attackDuration()
{
    return anticipationTime() + strikeTime() + holdTime();
}

float Creature::attackPower(bool allWeapons)
{
    float power = 5.0f;
    std::list<Item *> weapons = allWeapons ? equippedWeapons() : attackWeapons();
    for (std::list<Item *>::iterator it = weapons.begin(); it != weapons.end(); ++it) {
        power += (*it)->weaponDamage(this);
    }
    power = (m_entityData.stealth * 0.2f + 1.0f) * power;
    switch (m_entityData.currentAbility) {
    case AbilityDjinnExplosion:
        power *= 50.0f;
        break;
    case AbilityQuake:
        power *= 2.0f;
        break;
    case AbilityLunge:
        power *= 5.0f;
        break;
    case AbilityShuriken:
        power *= 3.0f;
        break;
    }
    const EntityData::Appearance &appearance = m_entityData.appearance;
    if (appearance.hasFlag(AppearanceFlagTinted)) {
        power *= 1.5f;
    }
    if (appearance.hasFlag(AppearanceFlagEnhanced)) {
        power *= 1.2f;
    }
    if (appearance.hasFlag(AppearanceFlagLarge)) {
        power *= 2.0f;
    }
    Buff *buff = findBuff(BuffAttackflask);
    if (buff) {
        power *= buff->strength;
    }
    buff = findBuff(BuffAncientpower);
    if (buff) {
        power *= buff->strength * 0.1f + 1.0f;
    }
    if (appearance.appearanceFlags & AppearanceFlagPromotedResident) {
        power *= 1.1f;
    }
    return power;
}

std::list<Item *> Creature::attackWeapons()
{
    std::list<Item *> weapons;
    if (m_entityData.equipment.weaponRight.category == ItemCategoryWeapon &&
        (m_entityData.equipment.weaponRight.id == WeaponBow ||
         m_entityData.equipment.weaponRight.id == WeaponCrossbow) &&
        canEquipItem(m_entityData.equipment.weaponRight)) {
        weapons.push_back(&m_entityData.equipment.weaponRight);
        return weapons;
    }
    if (m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon &&
        (m_entityData.equipment.weaponLeft.id == WeaponBow || m_entityData.equipment.weaponLeft.id == WeaponCrossbow) &&
        canEquipItem(m_entityData.equipment.weaponLeft)) {
        weapons.push_back(&m_entityData.equipment.weaponLeft);
        return weapons;
    }
    switch (m_entityData.currentAbility) {
    case AbilityLeftSlash:
    case Ability_5:
    case AbilityLeftStab:
    case AbilityBlock:
    case AbilityBlockJab:
    case AbilityLeftDaggerPierce:
    case AbilityFirebolt:
    case AbilityWaterbolt:
        if (m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon &&
            canEquipItem(m_entityData.equipment.weaponLeft)) {
            weapons.push_back(&m_entityData.equipment.weaponLeft);
        }
        break;
    case AbilityBraceletFirebolt:
    case AbilityBraceletWaterbolt:
        if (m_entityData.equipment.weaponRight.category == ItemCategoryWeapon &&
            canEquipItem(m_entityData.equipment.weaponRight)) {
            weapons.push_back(&m_entityData.equipment.weaponRight);
        }
        break;
    case AbilitySwirlStrike:
    case AbilityCrossedWeaponSweep:
    case AbilityMonsterChargeStrike:
    case AbilityAmbush:
    case AbilityKick:
    case AbilitySmash:
    case AbilityCyclone:
    case AbilityQuake:
    case AbilityShuriken:
    case AbilityAssassinate:
    case AbilityRockFist:
    case AbilityBloomlingAttack:
        if (m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon &&
            canEquipItem(m_entityData.equipment.weaponLeft)) {
            weapons.push_back(&m_entityData.equipment.weaponLeft);
        }
        if (m_entityData.equipment.weaponRight.category == ItemCategoryWeapon &&
            canEquipItem(m_entityData.equipment.weaponRight)) {
            weapons.push_back(&m_entityData.equipment.weaponRight);
        }
        break;
    default:
        if (m_entityData.equipment.weaponRight.category == ItemCategoryWeapon &&
            canEquipItem(m_entityData.equipment.weaponRight)) {
            weapons.push_back(&m_entityData.equipment.weaponRight);
        }
        if (m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon &&
            m_entityData.equipment.weaponLeft.id == WeaponBracelet && canEquipItem(m_entityData.equipment.weaponLeft)) {
            weapons.push_back(&m_entityData.equipment.weaponLeft);
        }
        break;
    }
    return weapons;
}

float Creature::blockPower()
{
    float power = 5.0f;
    Equipment &equipment = m_entityData.equipment;
    if (equipment.weaponLeft.category == ItemCategoryWeapon && equipment.weaponLeft.id == WeaponShield &&
        canEquipItem(equipment.weaponLeft)) {
        if (m_entityData.classType == ClassWarrior && m_entityData.specialization == SpecializationGuardian) {
            power = equipment.weaponRight.weaponDamage(this) * 6.0f + power;
        } else {
            power = equipment.weaponRight.weaponDamage(this) * 4.0f + power;
        }
    } else {
        if (equipment.weaponLeft.category == ItemCategoryWeapon && canEquipItem(equipment.weaponLeft)) {
            power = equipment.weaponLeft.weaponDamage(this) + power;
        }
        if (equipment.weaponRight.category == ItemCategoryWeapon && canEquipItem(equipment.weaponRight)) {
            power += equipment.weaponRight.weaponDamage(this);
        }
    }
    const EntityData::Appearance &appearance = m_entityData.appearance;
    if (appearance.hasFlag(AppearanceFlagTinted)) {
        power *= 2.0f;
    }
    if (appearance.hasFlag(AppearanceFlagEnhanced)) {
        power *= 1.2f;
    }
    if (appearance.hasFlag(AppearanceFlagLarge)) {
        power *= 4.0f;
    }
    if (appearance.appearanceFlags & AppearanceFlagPromotedResident) {
        power *= 1.1f;
    }
    return power;
}

float bonusFromLevel(int level)
{
    return (1.0f - 1.0f / ((level - 1.0f) / 50.0f + 1.0f)) * 1.8f + 1.2f;
}

bool Creature::canBeDamaged()
{
    unsigned char hostility = m_entityData.hostilityType;
    return hostility == HostilityPlayer || hostility == HostilityEnemy || hostility == HostilityFriendlyNpc ||
           hostility == HostilityPet ||
           (hostility == HostilityAttackableObject &&
            !(m_entityData.race >= RaceScarecrow && m_entityData.race <= RaceDummy));
}

bool Creature::canDoDamage(Creature *other)
{
    if (this == other) {
        return false;
    }
    if (m_entityData.appearance.appearanceFlags & AppearanceFlagPetrified) {
        return false;
    }
    if (other->m_entityData.appearance.appearanceFlags & AppearanceFlagPetrified) {
        return false;
    }
    if (m_entityData.hostilityType == HostilityAttackableObject ||
        other->m_entityData.hostilityType == HostilityAttackableObject) {
        return true;
    }
    if (m_entityData.classType != ClassArenaChampion && other->m_entityData.classType != ClassArenaChampion &&
        m_entityData.race != RaceEagle && other->m_entityData.race != RaceEagle &&
        m_entityData.hostilityType != HostilityInteractiveObject &&
        other->m_entityData.hostilityType != HostilityInteractiveObject &&
        ((m_entityData.hostilityType == HostilityEnemy &&
          (other->m_entityData.hostilityType != HostilityEnemy || isMarked() != other->isMarked())) ||
         (other->m_entityData.hostilityType == HostilityEnemy &&
          (m_entityData.hostilityType != HostilityEnemy || isMarked() != other->isMarked())) ||
         m_entityData.binaryToggles.combatSideOverride || other->m_entityData.binaryToggles.combatSideOverride)) {
        return true;
    }
    return false;
}

bool Creature::canEquipItem(const Item &item)
{
    if (m_entityData.hostilityType != HostilityPlayer) {
        return true;
    }
    return item.fitsClass(m_entityData.classType) ? true : false;
}

bool Creature::canTakeItem(const Item &item)
{
    if (item.category == ItemCategoryConsumable || item.category == ItemCategoryMaterial ||
        item.category == ItemCategoryPetFood || item.category == ItemCategorySpecial) {
        int quantity = 0;
        if (m_inventory.m_cursorItem == item) {
            quantity = m_inventory.m_cursorCount;
        }
        for (std::vector<std::vector<ItemStack>>::iterator tab = m_inventory.m_inventoryTabs.begin();
             tab != m_inventory.m_inventoryTabs.end(); ++tab) {
            for (std::vector<ItemStack>::iterator stack = tab->begin(); stack != tab->end(); ++stack) {
                if (stack->item == item) {
                    quantity += stack->quantity;
                }
            }
        }
        if (item.category == ItemCategoryPetFood && quantity >= 1) {
            return false;
        }
        if (item.category == ItemCategorySpecial && quantity >= 1) {
            return false;
        }
        if (item.category == ItemCategoryConsumable) {
            if (item.id == ConsumableGreaterLifePotion && quantity >= 5) {
                return false;
            }
            if (item.id == ConsumableApple && quantity >= 5) {
                return false;
            }
        }
        if (item.category == ItemCategoryMaterial && item.id >= MaterialItemIceCube && item.id <= MaterialItemBanana &&
            quantity >= 10) {
            return false;
        }
        if (quantity >= 50) {
            return false;
        }
    }
    return true;
}

bool Creature::canUseSkill(int skill, bool flag)
{
    if (skill == m_entityData.currentAbility && attackDuration() > m_entityData.timeSinceAbility) {
        return false;
    }
    if (m_entityData.stunTime > 0.0f && skill != AbilityBulwark) {
        return false;
    }
    if (skill == AbilitySneak &&
        (m_entityData.currentAbility == AbilitySneak || attackDuration() > m_entityData.timeSinceAbility)) {
        return false;
    }
    if (m_entityData.currentAbility == AbilityIntercept) {
        return false;
    }
    float mp = m_entityData.mp;
    if (manaCost(skill, -1) > mp) {
        return false;
    }
    if (staminaCost(skill) > m_stamina) {
        return false;
    }
    std::map<int, float>::iterator cooldown = m_skillCooldowns.find(skill);
    if (cooldown != m_skillCooldowns.end() && cooldown->second != 0.0f) {
        return false;
    }
    if (skill != AbilitySupportRay) {
        return true;
    }
    return mp > 0.0f;
}

float Creature::climbingSpeed()
{
    if (!(m_entityData.appearance.appearanceFlags & (AppearanceFlagTinted | AppearanceFlagLarge))) {
        if (m_entityData.hostilityType != HostilityPlayer) {
            return (1.0f - 1.0f / (m_entityData.level * 10 / 20.0f + 1.0f) + 1.0f) * 0.5f;
        }
        return (1.0f - 1.0f / (m_artifactSkills[ArtifactEffect_ClimbingSpeed] / 20.0f + 1.0f) + 1.0f) * 0.5f;
    }
    return 1.0f;
}

int Creature::cooldown(int skill)
{
    int tuned = 0;
    if (mod::cooldown(this, skill, tuned)) {
        return tuned;
    }
    switch (skill) {
    case AbilityIntercept:
        if (findBuff(BuffNinjutsu)) {
            return 1800;
        }
        return 16000;
    case AbilityTeleport:
    case AbilitySmash:
    case AbilityManaShield:
        return 16000;
    case AbilityShuriken:
        if (findBuff(BuffNinjutsu)) {
            return 1800;
        }
        return 16000;
    case AbilityFireMissiles:
    case AbilityCamouflage:
    case AbilitySwiftness:
    case AbilityBulwark:
    case AbilityWarFrenzy:
    case AbilityQuicksandTrap:
    case AbilityWaterBubbles:
    case AbilityFlameRush:
        return 40000;
    case AbilityCyclone:
    case AbilityLavaPuddle:
    case AbilityFireBreath:
    case AbilityHeroicShout:
        return 30000;
    case AbilityRangerKick:
    case AbilityRetreat:
    case AbilityFireExplosion:
        return 12000;
    case AbilityAim:
        return 4000;
    case AbilityShellSpin:
        return 15000;
    case AbilityAssassinate:
    case AbilityNinjutsu:
    case AbilityShadowClone:
        return 60000;
    case AbilityRockFist:
        return 20000;
    }
    return 0;
}

float Creature::critical(Creature *otherCreature, bool otherCreatureBasedOnResistance)
{
    float chance = m_entityData.stealth * 0.5f + 0.0f;
    if (m_entityData.currentAbility == AbilityAssassinate && attackDuration() > m_entityData.timeSinceAbility) {
        chance += 0.5f;
    }
    Equipment &equipment = m_entityData.equipment;
    if (equipment.weaponLeft.category == ItemCategoryWeapon) {
        chance += equipment.weaponLeft.critical(this);
    }
    if (equipment.weaponRight.category == ItemCategoryWeapon) {
        chance += equipment.weaponRight.critical(this);
    }
    if (equipment.chest.category == ItemCategoryChest) {
        chance += equipment.chest.critical(this);
    }
    if (equipment.feet.category == ItemCategoryFeet) {
        chance += equipment.feet.critical(this);
    }
    if (equipment.hands.category == ItemCategoryHands) {
        chance += equipment.hands.critical(this);
    }
    if (equipment.shoulder.category == ItemCategoryShoulder) {
        chance += equipment.shoulder.critical(this);
    }
    if (equipment.neck.category == ItemCategoryNeck) {
        chance += equipment.neck.critical(this);
    }
    if (equipment.ringLeft.category == ItemCategoryRing) {
        chance += equipment.ringLeft.critical(this);
    }
    if (equipment.ringRight.category == ItemCategoryRing) {
        chance += equipment.ringRight.critical(this);
    }
    if (otherCreature) {
        if (otherCreatureBasedOnResistance) {
            chance -= otherCreature->magicDamageReduction();
        } else {
            chance -= otherCreature->damageReduction();
        }
    }
    return chance;
}

float Creature::damageReduction()
{
    float reduction = armor() / powf(2.0f, 4.0f) * 0.05f;
    if (findBuff(BuffDefenseWeakened)) {
        reduction *= 0.25f;
    }
    return reduction;
}

float Creature::damageScale(int skill)
{
    switch (skill) {
    case AbilityFireball:
    case AbilityWaterball:
    case AbilityTwoHandedSweep:
    case AbilityTwoHandedThrust:
    case AbilityHeadbutt:
    case AbilityThrowPoisonVial:
        return 2.0f;
    case AbilityDiveBite:
        return 3.0f;
    case AbilitySalvoRelease:
        return 0.25f;
    case AbilityFireMissiles:
    case AbilityFireRay:
    case AbilityWaterRay:
        return 0.75f;
    case AbilityPerforate:
    case AbilityThrowBoomerang:
        return 0.5f;
    case AbilityFireSwirl:
    case AbilityWaterSwirl:
        return 0.4f;
    case AbilityFireVortex:
    case AbilityWaterVortex:
        return 0.6f;
    case AbilityShellSpin:
        return 0.2f;
    case AbilityRangerKick:
        return 0.1f;
    case AbilityKick:
    case AbilityMonsterFirebolt:
    case AbilityFirebolt:
    case AbilityBraceletFirebolt:
    case AbilityWaterbolt:
    case AbilityBraceletWaterbolt:
    case AbilityMonsterWaterbolt:
    case AbilityWaterSalvo:
    case Ability_48:
    case AbilitySmash:
    case AbilityTwoHandedSwing:
    case AbilityCyclone:
    case AbilityFireBeam:
    case AbilityWaterBeam:
    case AbilityFireBreath:
    case AbilityRockFist:
        return 1.0f;
    }
    return 1.0f;
}

float Creature::demonDamageBonus()
{
    if ((int)m_inventory.m_inventoryTabs.size() <= 1) {
        return 0.0f;
    }
    float bonus = 0.0f;
    for (ItemStack &stack : m_inventory.m_inventoryTabs[1]) {
        if (stack.quantity > 0) {
            bonus += stack.item.demonDamageBonus();
        }
    }
    return bonus;
}

float Creature::divingSkill()
{
    if (m_entityData.hostilityType != HostilityPlayer) {
        return 0.0f;
    }
    return (1.0f - (1.0f - 1.0f / (m_artifactSkills[ArtifactEffect_DivingStamina] / 20.0f + 1.0f))) * 0.1f;
}

// The seed of an equipment slot's modifier for the creatures whose equipment must not change between visits (hostility
// type 6).
static inline unsigned int slotSeed(unsigned int value)
{
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}
void equip(int hostilityType, int race, int rarity, unsigned int flags, int factionKind, unsigned char &classType,
           unsigned char &specialization, Equipment &equipment, std::vector<int> &specialSkills,
           int &summonAppearanceMode, std::vector<int> &summonRaces, Inventory &inventory)
{
    equipment = Equipment();

    if (speciesType(race) == SpeciesHumanoid) {
        int count = rand() % 4 - 1;
        if (count > 0) {
            Item item;
            item.category = ItemCategoryConsumable;
            item.id = ConsumableLifePotion;
            inventory.add(item, count, -1);
        }
    }

    if (classType != ClassVillager) {
        if (classType == ClassItemVendor || classType == ClassWeaponVendor || classType == ClassArmorVendor ||
            classType == ClassAnalyzer || classType == ClassInnkeeper || classType == ClassSmith ||
            classType == ClassCarpenter || classType == ClassTailor || classType == Class_139 ||
            classType == ClassGuildReceptionist) {
            equipment.chest.category = ItemCategoryChest;
            equipment.chest.material = ItemMaterialClothing;
            equipment.chest.id = ChestChestArmor;
        } else if (race == RaceOldMan && classType == ClassNone) {
            equipment.chest.category = ItemCategoryChest;
            equipment.chest.material = ItemMaterialClothing;
            equipment.chest.id = ChestChestArmor;
        } else if (race == RaceEmpress) {
            equipment.weaponRight.category = ItemCategoryWeapon;
            equipment.weaponRight.material = ItemMaterialSteel;
            equipment.weaponRight.id = WeaponStaff;
        } else if (race == RaceAngel) {
        } else if (race == RaceLegendaryPaladin) {
            equipment.weaponRight.category = ItemCategoryWeapon;
            equipment.weaponRight.material = ItemMaterialSteel;
            equipment.weaponRight.id = WeaponGreatsword;
            equipment.chest.material = ItemMaterialGold;
            equipment.hands.category = ItemCategoryHands;
            equipment.hands.material = ItemMaterialGold;
            equipment.feet.category = ItemCategoryFeet;
            equipment.feet.material = ItemMaterialGold;
        } else if (race == RaceManaMech) {
            equipment.chest.material = ItemMaterialSteel;
            equipment.hands.material = ItemMaterialSteel;
            equipment.feet.material = ItemMaterialSteel;
        } else if (race == RaceArchdruid) {
            equipment.weaponRight.category = ItemCategoryWeapon;
            equipment.weaponRight.material = ItemMaterialDruid;
            equipment.weaponRight.id = WeaponStaff;
        } else if (race == RaceGhostPreacher) {
        } else if (race == RaceHalfDemon) {
            equipment.weaponRight.category = ItemCategoryWeapon;
            equipment.weaponRight.material = ItemMaterialObsidian;
            equipment.weaponRight.id = WeaponLongsword;
            equipment.shoulder.category = ItemCategoryShoulder;
            equipment.shoulder.material = ItemMaterialObsidian;
            equipment.chest.category = ItemCategoryChest;
            equipment.chest.material = ItemMaterialObsidian;
            equipment.hands.category = ItemCategoryHands;
            equipment.hands.material = ItemMaterialObsidian;
            equipment.feet.category = ItemCategoryFeet;
            equipment.feet.material = ItemMaterialObsidian;
        } else if (classType == Class_144) {
            if (!(flags & AppearanceFlagUnarmored)) {
                equipment.chest.category = ItemCategoryChest;
                equipment.chest.id = ChestChestArmor;
                equipment.chest.material = ItemMaterialLight;
            }
        } else if (classType == ClassDruidOfMana) {
            if (!(flags & AppearanceFlagUnarmored)) {
                equipment.chest.category = ItemCategoryChest;
                equipment.chest.id = ChestChestArmor;
                equipment.chest.material = ItemMaterialDruid;
                equipment.weaponRight.category = ItemCategoryWeapon;
                equipment.weaponRight.id = WeaponStaff;
                equipment.weaponRight.material = ItemMaterialWood;
                equipment.weaponRight.modifier = rand();
            }
        } else if (classType == ClassArchaeologist) {
            equipment.chest.category = ItemCategoryChest;
            equipment.chest.id = ChestBackpack;
            equipment.chest.material = ItemMaterialClothing;
            equipment.weaponRight.category = ItemCategoryWeapon;
            equipment.weaponRight.id = WeaponMagnifier;
        } else {
            equipment.weaponLeft.category = ItemCategoryNone;
            equipment.weaponLeft.id = NoneCleared;
            equipment.weaponRight.category = ItemCategoryNone;
            equipment.weaponRight.id = NoneCleared;
            equipment.shoulder.category = ItemCategoryNone;
            equipment.shoulder.id = NoneCleared;
            equipment.chest.category = ItemCategoryNone;
            equipment.chest.id = NoneCleared;
            equipment.feet.category = ItemCategoryNone;
            equipment.feet.id = NoneCleared;
            equipment.hands.category = ItemCategoryNone;
            equipment.hands.id = NoneCleared;
            equipment.neck.category = ItemCategoryNone;
            equipment.neck.id = NoneCleared;

            if (hostilityType == HostilityAttackableObject || hostilityType == HostilityInteractiveObject) {
            } else if (!(flags & (AppearanceFlagHandAttacks | AppearanceFlagBiteAttack))) {
                if (race == RaceRuneGiant) {
                    equipment.weaponRight.category = ItemCategoryNone;
                    equipment.weaponRight.id = NoneCleared;
                    equipment.weaponLeft.category = ItemCategoryNone;
                    equipment.weaponLeft.id = NoneCleared;
                } else if (race == RaceLich) {
                    specialSkills.push_back(AbilityFireRay);
                    specialSkills.push_back(AbilityFireMissiles);
                    specialSkills.push_back(AbilitySummon);
                    summonAppearanceMode = SummonSelectedRace;
                    summonRaces.push_back(RaceSkeleton);
                    summonRaces.push_back(RaceZombie);
                } else if (race == RaceTroll || race == RaceYeti) {
                    equipment.weaponRight.category = ItemCategoryWeapon;
                    equipment.weaponRight.id = WeaponGreatmace;
                    equipment.weaponRight.material = ItemMaterialWood;
                    equipment.weaponLeft.category = ItemCategoryNone;
                    equipment.weaponLeft.id = NoneCleared;
                } else if (race == RaceHellDemon) {
                    equipment.weaponRight.category = ItemCategoryWeapon;
                    equipment.weaponRight.id = WeaponLongsword;
                    equipment.weaponRight.material = ItemMaterialObsidian;
                    equipment.weaponLeft.category = ItemCategoryNone;
                    equipment.weaponLeft.id = NoneCleared;
                } else if (race == RaceOgre) {
                    equipment.weaponRight.category = ItemCategoryWeapon;
                    equipment.weaponRight.id = WeaponGreatmace;
                    equipment.weaponRight.material = ItemMaterialBone;
                    equipment.weaponLeft.category = ItemCategoryNone;
                    equipment.weaponLeft.id = NoneCleared;
                } else if (race == RaceCyclops) {
                    specialSkills.push_back(AbilityCyclone);
                    specialSkills.push_back(AbilityLunge);
                    equipment.weaponRight.category = ItemCategoryWeapon;
                    equipment.weaponRight.id = WeaponFist;
                    equipment.weaponRight.material = ItemMaterialIron;
                    equipment.weaponLeft.category = ItemCategoryWeapon;
                    equipment.weaponLeft.id = WeaponFist;
                    equipment.weaponLeft.material = ItemMaterialIron;
                } else if (race == RaceDarkTroll) {
                    specialSkills.push_back(AbilityCyclone);
                    specialSkills.push_back(AbilityLunge);
                    specialSkills.push_back(AbilityQuake);
                    equipment.weaponRight.category = ItemCategoryWeapon;
                    equipment.weaponRight.id = WeaponMace;
                    equipment.weaponRight.material = ItemMaterialBone;
                    equipment.weaponLeft.category = ItemCategoryWeapon;
                    equipment.weaponLeft.id = WeaponMace;
                    equipment.weaponLeft.material = ItemMaterialBone;
                } else if (race == RaceDjinn) {
                    specialSkills.push_back(AbilityDjinnExplosion);
                    equipment.weaponRight.category = ItemCategoryWeapon;
                    equipment.weaponRight.id = WeaponBracelet;
                    equipment.weaponRight.material = ItemMaterialGold;
                    equipment.weaponLeft.category = ItemCategoryWeapon;
                    equipment.weaponLeft.id = WeaponBracelet;
                    equipment.weaponLeft.material = ItemMaterialGold;
                } else if (race == RaceMinotaur) {
                    equipment.weaponRight.category = ItemCategoryWeapon;
                    equipment.weaponRight.id = WeaponSword;
                    equipment.weaponRight.material = ItemMaterialIron;
                    equipment.weaponLeft.category = ItemCategoryWeapon;
                    equipment.weaponLeft.id = WeaponSword;
                    equipment.weaponLeft.material = ItemMaterialIron;
                } else if (race == RaceRockling || race == RaceGolem || race == RaceEmberGolem ||
                           race == RaceSnowGolem) {
                } else if (race == RaceWizard || race == RaceWitch || race == RaceNecromancer || race == RaceSorcerer ||
                           race == RaceSeeker) {
                    if (race == RaceNecromancer) {
                        specialSkills.push_back(AbilitySummon);
                        summonAppearanceMode = SummonSelectedRace;
                        summonRaces.push_back(RaceSkeleton);
                        summonRaces.push_back(RaceZombie);
                        summonRaces.push_back(RaceSkeletonKnight);
                    }
                    specialSkills.push_back(AbilityFireMissiles);
                    specialization = race == RaceSorcerer ? SpecializationFire : Specialization2;
                    equipment.weaponRight.category = ItemCategoryWeapon;
                    equipment.weaponRight.id = WeaponStaff + rand() % 2;
                    equipment.weaponRight.material = ItemMaterialWood;
                    equipment.weaponLeft.category = ItemCategoryNone;
                    equipment.weaponLeft.id = NoneCleared;
                    classType = ClassNone;
                } else {
                    if (classType == ClassNone) {
                        classType = rand() % 4 + 1;
                    }
                    if (race == RaceVampire) {
                        classType = ClassRogue;
                    } else if (race == RaceSkeletonKnight) {
                        classType = ClassWarrior;
                    }

                    switch (classType) {
                    case ClassWarrior:
                        specialization = rand() % 2;
                        switch (rand() % 3) {
                        case 0:
                            equipment.weaponRight.category = ItemCategoryWeapon;
                            equipment.weaponRight.id = WeaponGreatsword + rand() % 3;
                            equipment.weaponRight.material = ItemMaterialIron;
                            equipment.weaponLeft.category = ItemCategoryNone;
                            equipment.weaponLeft.id = NoneCleared;
                            break;
                        case 1:
                            equipment.weaponRight.category = ItemCategoryWeapon;
                            equipment.weaponRight.id = WeaponSword + rand() % 3;
                            equipment.weaponRight.material = ItemMaterialIron;
                            equipment.weaponLeft.category = ItemCategoryWeapon;
                            equipment.weaponLeft.id = WeaponSword + rand() % 3;
                            equipment.weaponLeft.material = ItemMaterialIron;
                            break;
                        case 2:
                            equipment.weaponRight.category = ItemCategoryWeapon;
                            equipment.weaponRight.id = WeaponSword + rand() % 3;
                            equipment.weaponRight.material = ItemMaterialIron;
                            equipment.weaponLeft.category = ItemCategoryWeapon;
                            equipment.weaponLeft.id = WeaponShield;
                            equipment.weaponLeft.material = ItemMaterialIron;
                            break;
                        }
                        break;
                    case ClassOrderOfTheLightMember:
                        switch (rand() % 3) {
                        case 0:
                            equipment.weaponRight.category = ItemCategoryWeapon;
                            equipment.weaponRight.id = WeaponGreatsword + rand() % 3;
                            equipment.weaponRight.material = ItemMaterialIron;
                            equipment.weaponLeft.category = ItemCategoryNone;
                            equipment.weaponLeft.id = NoneCleared;
                            break;
                        case 1:
                            equipment.weaponRight.category = ItemCategoryWeapon;
                            equipment.weaponRight.id = WeaponSword + rand() % 3;
                            equipment.weaponRight.material = ItemMaterialIron;
                            equipment.weaponLeft.category = ItemCategoryWeapon;
                            equipment.weaponLeft.id = WeaponSword + rand() % 3;
                            equipment.weaponLeft.material = ItemMaterialIron;
                            break;
                        case 2:
                            equipment.weaponRight.category = ItemCategoryWeapon;
                            equipment.weaponRight.id = WeaponSword + rand() % 3;
                            equipment.weaponRight.material = ItemMaterialIron;
                            equipment.weaponLeft.category = ItemCategoryWeapon;
                            equipment.weaponLeft.id = WeaponShield;
                            equipment.weaponLeft.material = ItemMaterialIron;
                            break;
                        }
                        break;
                    case ClassRanger:
                        specialization = rand() % 2;
                        equipment.weaponRight.category = ItemCategoryWeapon;
                        equipment.weaponRight.id = WeaponBow + rand() % 3;
                        equipment.weaponRight.material = ItemMaterialWood;
                        break;
                    case ClassMage:
                        specialization = rand() % 2;
                        switch (rand() % 3) {
                        case 0:
                            equipment.weaponRight.category = ItemCategoryWeapon;
                            equipment.weaponRight.id = WeaponStaff;
                            equipment.weaponRight.material = ItemMaterialWood;
                            equipment.weaponLeft.category = ItemCategoryNone;
                            equipment.weaponLeft.id = NoneCleared;
                            break;
                        case 1:
                            equipment.weaponRight.category = ItemCategoryWeapon;
                            equipment.weaponRight.id = WeaponWand;
                            equipment.weaponRight.material = ItemMaterialWood;
                            equipment.weaponLeft.category = ItemCategoryNone;
                            equipment.weaponLeft.id = NoneCleared;
                            break;
                        case 2:
                            equipment.weaponRight.category = ItemCategoryWeapon;
                            equipment.weaponRight.id = WeaponBracelet;
                            equipment.weaponRight.material = ItemMaterialGold;
                            equipment.weaponLeft.category = ItemCategoryWeapon;
                            equipment.weaponLeft.id = WeaponBracelet;
                            equipment.weaponLeft.material = ItemMaterialGold;
                            break;
                        }
                        break;
                    case ClassRogue: {
                        specialization = rand() % 2;
                        equipment.weaponRight.category = ItemCategoryWeapon;
                        int weapon = WeaponDagger + rand() % 3;
                        equipment.weaponRight.id = weapon;
                        equipment.weaponRight.material = ItemMaterialIron;
                        if (weapon == WeaponLongsword) {
                            equipment.weaponLeft.category = ItemCategoryNone;
                            equipment.weaponLeft.id = NoneCleared;
                            equipment.weaponLeft.material = ItemMaterialNone;
                        } else {
                            equipment.weaponLeft.category = ItemCategoryWeapon;
                            equipment.weaponLeft.id = weapon;
                            equipment.weaponLeft.material = ItemMaterialIron;
                        }
                        break;
                    }
                    case ClassObsidianKnight:
                        switch (rand() % 4) {
                        case 0:
                            equipment.weaponRight.category = ItemCategoryWeapon;
                            equipment.weaponRight.id = WeaponGreatsword + rand() % 3;
                            equipment.weaponRight.material = ItemMaterialObsidian;
                            equipment.weaponLeft.category = ItemCategoryNone;
                            equipment.weaponLeft.id = NoneCleared;
                            break;
                        case 1:
                            equipment.weaponRight.category = ItemCategoryWeapon;
                            equipment.weaponRight.id = rand() % 2 == 0 ? WeaponAxe : WeaponSword;
                            equipment.weaponRight.material = ItemMaterialObsidian;
                            equipment.weaponLeft.category = ItemCategoryWeapon;
                            equipment.weaponLeft.id = rand() % 2 == 0 ? WeaponAxe : WeaponSword;
                            equipment.weaponLeft.material = ItemMaterialObsidian;
                            break;
                        case 2:
                            equipment.weaponRight.category = ItemCategoryWeapon;
                            equipment.weaponRight.id = WeaponLongsword;
                            equipment.weaponRight.material = ItemMaterialObsidian;
                            break;
                        case 3:
                            equipment.weaponRight.category = ItemCategoryWeapon;
                            equipment.weaponRight.id = WeaponStaff;
                            equipment.weaponRight.material = ItemMaterialObsidian;
                            break;
                        }
                        break;
                    }

                    if (!(flags & AppearanceFlagUnarmored)) {
                        std::vector<int> materials;
                        int weapon = equipment.weaponRight.id;
                        if (weapon == WeaponStaff || weapon == WeaponWand || weapon == WeaponBracelet) {
                            if (factionKind == FactionSteelEmpire) {
                                materials.push_back(ItemMaterialFilament);
                            } else {
                                materials.push_back(ItemMaterialSilk);
                                materials.push_back(ItemMaterialFlannell);
                                materials.push_back(ItemMaterialFilament);
                            }
                        } else if (weapon == WeaponBow || weapon == WeaponCrossbow || weapon == WeaponBoomerang) {
                            if (factionKind == FactionSteelEmpire) {
                                materials.push_back(ItemMaterialMail);
                            } else {
                                materials.push_back(ItemMaterialLinen);
                                materials.push_back(ItemMaterialFelt);
                                materials.push_back(ItemMaterialMail);
                            }
                        } else if (weapon == WeaponDagger || weapon == WeaponFist || weapon == WeaponLongsword) {
                            if (factionKind == FactionSteelEmpire) {
                                materials.push_back(ItemMaterialRivet);
                            } else {
                                materials.push_back(ItemMaterialCotton);
                                materials.push_back(ItemMaterialCalico);
                                materials.push_back(ItemMaterialRivet);
                            }
                        } else {
                            if (factionKind == FactionSteelEmpire) {
                                materials.push_back(ItemMaterialSteel);
                            } else {
                                materials.push_back(ItemMaterialIron);
                                materials.push_back(ItemMaterialBronze);
                                materials.push_back(ItemMaterialSteel);
                            }
                        }
                        if (classType == ClassOrderOfTheLightMember) {
                            materials.clear();
                            materials.push_back(ItemMaterialGold);
                        } else if (classType == ClassObsidianKnight) {
                            materials.clear();
                            materials.push_back(ItemMaterialObsidian);
                        }

                        equipment.chest.category = ItemCategoryChest;
                        equipment.chest.material = materials[rand() % materials.size()];
                        equipment.shoulder.category = ItemCategoryShoulder;
                        equipment.shoulder.material = materials[rand() % materials.size()];
                        equipment.hands.category = ItemCategoryHands;
                        equipment.hands.material = materials[rand() % materials.size()];
                        equipment.feet.category = ItemCategoryFeet;
                        equipment.feet.material = materials[rand() % materials.size()];
                        equipment.neck.category = ItemCategoryNeck;
                        equipment.neck.material = rand() % 2 == 0 ? ItemMaterialSilver : ItemMaterialGold;
                        equipment.ringRight.category = ItemCategoryRing;
                        equipment.ringRight.material = rand() % 2 == 0 ? ItemMaterialSilver : ItemMaterialGold;
                        equipment.ringLeft.category = ItemCategoryRing;
                        equipment.ringLeft.material = rand() % 2 == 0 ? ItemMaterialSilver : ItemMaterialGold;
                    }
                }
            } else {
                equipment.weaponRight.category = ItemCategoryNone;
                equipment.weaponRight.id = NoneCleared;
                equipment.weaponLeft.category = ItemCategoryNone;
                equipment.weaponLeft.id = NoneCleared;
            }
        }
    } else {
        equipment.chest.category = ItemCategoryChest;
        equipment.chest.material = ItemMaterialClothing;
        equipment.chest.id = ChestBody1 + rand() % 5;
    }

    if (flags & AppearanceFlagHandAttacks) {
        equipment.weaponRight.category = ItemCategoryWeapon;
        equipment.weaponRight.id = WeaponHandAttacks;
        equipment.weaponLeft.category = ItemCategoryWeapon;
        equipment.weaponLeft.id = WeaponHandAttacks;
    }
    if (flags & AppearanceFlagBiteAttack) {
        equipment.weaponRight.category = ItemCategoryWeapon;
        equipment.weaponRight.id = WeaponBiteAttack;
        equipment.weaponLeft = Item();
    }
    if (flags & AppearanceFlagUnarmored) {
        equipment.chest.category = ItemCategoryChest;
        equipment.chest.id = ChestBackpack;
    }

    switch (race) {
    case RaceDemonMarauder:
        classType = ClassMonster;
        specialSkills.push_back(AbilityFireBreath);
        break;
    case RaceChiling:
        classType = ClassMonster;
        specialSkills.push_back(AbilityFireBreath);
        break;
    case RaceChilingSprout:
        classType = ClassMonster;
        specialSkills.push_back(AbilityFireBreath);
        break;
    case RaceHabanero:
        classType = ClassMonster;
        specialSkills.push_back(AbilityFireBreath);
        break;
    case RaceImp:
    case RaceHellDemon:
        specialSkills.push_back(AbilityFireBreath);
        break;
    case RaceTroll:
        specialSkills.push_back(AbilitySmash);
        specialSkills.push_back(AbilityQuake);
        specialSkills.push_back(AbilityLunge);
        break;
    case RaceDarkTroll:
        specialSkills.push_back(AbilitySmash);
        specialSkills.push_back(AbilityQuake);
        specialSkills.push_back(AbilityLunge);
        break;
    case RaceYeti:
        specialSkills.push_back(AbilitySmash);
        specialSkills.push_back(AbilityQuake);
        specialSkills.push_back(AbilityLunge);
        break;
    case RaceSkeleton:
    case RaceWerewolf:
    case RaceVampire:
        specialSkills.push_back(AbilityLunge);
        break;
    case RaceSpitter:
        classType = ClassMonsterCaster;
        specialization = SpecializationWater;
        break;
    case RaceOgre:
        specialSkills.push_back(AbilityCyclone);
        classType = Class_148;
        break;
    case RaceDjinn:
        specialSkills.push_back(AbilitySummon);
        specialSkills.push_back(AbilityFireBreath);
        break;
    case RaceSkullBull:
        specialSkills.push_back(AbilityHornCharge);
        break;
    case RaceTurtle:
    case RaceLavaTurtle:
    case RaceBlueTurtle:
        specialSkills.push_back(AbilityShellSpin);
        break;
    case RaceBabyElephant:
    case RaceBabyMammoth:
        specialSkills.push_back(AbilityStomp);
        break;
    case RacePlantCreature:
        classType = Class_145;
        specialSkills.push_back(AbilityRestHeal);
        break;
    case RaceOnionling:
    case RaceDesertOnionling:
        specialSkills.push_back(AbilityRestHeal);
        break;
    case RaceRadishCreature:
    case RaceBiter:
        classType = Class_147;
        specialSkills.push_back(AbilityRestHeal);
        break;
    case RaceMole:
        classType = ClassMonster;
        specialSkills.push_back(AbilityRestHeal);
        break;
    case RaceManaMech:
        classType = ClassMonster;
        specialSkills.push_back(AbilityCyclone);
        break;
    case RaceMinotaur:
        specialSkills.push_back(AbilityCyclone);
        break;
    case RaceMonstrosity:
        classType = ClassMonster;
        specialSkills.push_back(AbilitySmash);
        break;
    case RaceFlyingEye:
        specialSkills.push_back(AbilityFireRay);
        specialization = 0;
        break;
    case RaceSquirrel:
        classType = ClassMonster;
        break;
    case RaceSnoutBeetle:
        classType = ClassMonsterGunner;
        break;
    case RaceDemonPortal:
        classType = ClassSpawner;
        summonAppearanceMode = SummonSelectedRace;
        summonRaces.push_back(RaceImp);
        break;
    case RaceSlimeWell:
        classType = ClassSpawner;
        summonAppearanceMode = SummonSelectedRace;
        summonRaces.push_back(RaceBlueSlime);
        summonRaces.push_back(RaceGreenSlime);
        summonRaces.push_back(RacePinkSlime);
        summonRaces.push_back(RaceYellowSlime);
        break;
    case RaceHornetsNest:
        classType = ClassSpawner;
        summonAppearanceMode = SummonSelectedRace;
        summonRaces.push_back(RaceHornet);
        break;
    case RaceUnholyTomb:
        classType = ClassSpawner;
        summonAppearanceMode = SummonSelectedRace;
        summonRaces.push_back(RaceZombie);
        break;
    case RaceBonePile:
        classType = ClassSpawner;
        summonAppearanceMode = SummonSelectedRace;
        summonRaces.push_back(RaceSkeleton);
        break;
    case RaceMolehill:
        classType = ClassSpawner;
        summonAppearanceMode = SummonSelectedRace;
        summonRaces.push_back(RaceMole);
        break;
    case RaceEmberGolem:
        classType = ClassMonster;
        specialSkills.push_back(AbilityFireBreath);
        specialSkills.push_back(AbilityFireMissiles);
        break;
    case RaceRockling:
        classType = ClassMonster;
        specialSkills.push_back(AbilityThrowRock);
        break;
    case RaceGolem:
        classType = ClassMonster;
        specialSkills.push_back(AbilityThrowRock);
        break;
    }

    if (flags & AppearanceFlagTinted) {
        std::vector<int> choices;
        choices.push_back(AbilitySummon);
        choices.push_back(AbilityLunge);
        choices.push_back(AbilityCyclone);
        choices.push_back(AbilityLavaPuddle);
        choices.push_back(AbilityDjinnExplosion);
        choices.push_back(AbilitySmash);
        choices.push_back(AbilityManaShield);
        choices.push_back(AbilityBulwark);
        choices.push_back(AbilityFireMissiles);
        specialSkills.push_back(choices[rand() % choices.size()]);
        if (specialSkills.back() == AbilitySummon) {
            // Select one of the three summon appearance modes, SummonSelectedRace through SummonCopyOpponent.
            summonAppearanceMode = rand() % 3;
            summonRaces.push_back(RaceSkeleton);
            summonRaces.push_back(RaceZombie);
        }
    }

    if (hostilityType == HostilityPet) {
        // Equipment modifier noise salts: race plus a distinct multiple of 100 per equipment slot.
        equipment.weaponLeft.modifier = slotSeed(race);
        equipment.weaponRight.modifier = slotSeed(race + 100);
        equipment.shoulder.modifier = slotSeed(race + 200);
        equipment.chest.modifier = slotSeed(race + 300);
        equipment.hands.modifier = slotSeed(race + 400);
        equipment.feet.modifier = slotSeed(race + 500);
        equipment.neck.modifier = slotSeed(race + 600);
        equipment.ringLeft.modifier = slotSeed(race + 700);
        equipment.ringRight.modifier = slotSeed(race + 800);
    } else {
        equipment.weaponLeft.modifier = rand();
        equipment.weaponRight.modifier = rand();
        equipment.shoulder.modifier = rand();
        equipment.chest.modifier = rand();
        equipment.hands.modifier = rand();
        equipment.feet.modifier = rand();
        equipment.neck.modifier = rand();
        equipment.ringLeft.modifier = rand();
        equipment.ringRight.modifier = rand();
    }

    equipment.weaponRight.rarity = rarity;
    equipment.weaponLeft.rarity = rarity;
    equipment.shoulder.rarity = rarity;
    equipment.chest.rarity = rarity;
    equipment.hands.rarity = rarity;
    equipment.feet.rarity = rarity;
    equipment.neck.rarity = rarity;
    equipment.ringLeft.rarity = rarity;
    equipment.ringRight.rarity = rarity;

    equipment.weaponRight.modifier = rand();
    equipment.weaponLeft.modifier = rand();
    equipment.shoulder.modifier = rand();
    equipment.chest.modifier = rand();
    equipment.hands.modifier = rand();
    equipment.feet.modifier = rand();
    equipment.neck.modifier = rand();
    equipment.ringLeft.modifier = rand();
    equipment.ringRight.modifier = rand();
}

void Creature::equipByRace()
{
    equip(m_entityData.hostilityType, m_entityData.race, m_entityData.level, m_entityData.appearance.appearanceFlags,
          m_entityData.factionKind, m_entityData.classType, m_entityData.specialization, m_entityData.equipment,
          m_specialSkills, m_summonAppearanceMode, m_summonRaces, m_inventory);
}

std::list<Item *> Creature::equippedWeapons()
{
    std::list<Item *> weapons;
    if (m_entityData.equipment.weaponRight.category == ItemCategoryWeapon) {
        weapons.push_back(&m_entityData.equipment.weaponRight);
    }
    if (m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon) {
        weapons.push_back(&m_entityData.equipment.weaponLeft);
    }
    return weapons;
}

bool Creature::experiencesBuoyancy()
{
    return m_entityData.race < RaceGoldDeposit || m_entityData.race > RaceIceCrystalDeposit;
}

// Sets sprite-table indices and Appearance flags for each race. Sizes are expressed as base dimensions multiplied by
// race-specific scale factors.
void fillAppearance(int &race, Creature::EntityData::Appearance &appearance, const CharacterStyle *style,
                    bool allowMarked)
{
    CharacterStyle randomStyle;
    if (!style) {
        randomStyle.hairColor = plasma::Vector<3, unsigned char>(rand() % 256, rand() % 256, rand() % 256);
        randomStyle.face = rand() % 100;
        randomStyle.haircut = rand() % 100;
        style = &randomStyle;
    } else {
        // The race choices of CharacterStyleWidget: Human, Elf, Dwarf, Orc, Goblin, Lizard, Undead, Frogman.
        switch (style->race) {
        case CharacterRaceChoiceHuman:
            race = style->gender % 2 + RaceHumanMale;
            break;
        case CharacterRaceChoiceElf:
            race = style->gender % 2 + RaceElfMale;
            break;
        case CharacterRaceChoiceDwarf:
            race = style->gender % 2 + RaceDwarfMale;
            break;
        case CharacterRaceChoiceOrc:
            race = style->gender % 2 + RaceOrcMale;
            break;
        case CharacterRaceChoiceGoblin:
            race = style->gender % 2 + RaceGoblinMale;
            break;
        case CharacterRaceChoiceLizard:
            race = style->gender % 2 + RaceLizardmanMale;
            break;
        case CharacterRaceChoiceUndead:
            race = style->gender % 2 + RaceUndeadMale;
            break;
        case CharacterRaceChoiceFrogman:
            race = style->gender % 2 + RaceFrogmanMale;
            break;
        }
    }

    appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.2f;

    switch (race) {
    case RaceElfMale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteElfHeadM01 + style->face % 4;
        appearance.hairModel = SpriteElfHairM01 + style->haircut % 10;
        appearance.chestModel = SpriteBody4Slot1;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        break;
    case RaceHumanMale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteHumanHeadM01 + style->face % 6;
        appearance.hairModel = SpriteHumanHairM01 + style->haircut % 15;
        appearance.chestModel = SpriteBody4Slot1;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = appearance.headModel > SpriteHumanHeadM03 ? SpriteBrownHand : SpriteHand2;
        break;
    case RaceHumanFemale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteHumanHeadF01 + style->face % 6;
        appearance.hairModel = SpriteHumanHairF01 + style->haircut % 9;
        appearance.chestModel = SpriteBody4Slot1;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = appearance.headModel > SpriteHumanHeadF03 ? SpriteBrownHand : SpriteHand2;
        break;
    case RaceFrogmanMale:
        appearance.hairColor = plasma::Vector<3, unsigned char>(255, 255, 255);
        appearance.headModel = SpriteFrogmanHeadM01 + style->face % 5;
        appearance.hairModel = SpriteFrogmanHairM01 + style->haircut % 5;
        appearance.chestModel = SpriteBody4Slot1;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteFrogmanHand;
        break;
    case RaceFrogmanFemale:
        appearance.hairColor = plasma::Vector<3, unsigned char>(255, 255, 255);
        appearance.headModel = SpriteFrogmanHeadF01 + style->face % 4;
        appearance.hairModel = SpriteFrogmanHairF01 + style->haircut % 4;
        appearance.chestModel = SpriteBody4Slot1;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteFrogmanHand;
        break;
    case RaceVampire:
        appearance.headModel = SpriteVampireHead;
        appearance.hairModel = SpriteVampireEyes;
        appearance.chestModel = SpriteCottonChest5;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteVampireHand;
        appearance.appearanceFlags |= AppearanceFlagFlying | AppearanceFlagSuppressHairDebris;
        appearance.wingsModel = SpriteBatHand;
        appearance.wingScale = 2.0f;
        appearance.wingsRotation = 60.0f;
        break;
    case RaceFlyingEye:
        appearance.chestModel = SpriteFlyingEyeHead;
        appearance.appearanceFlags |= AppearanceFlagFlying | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        appearance.wingsModel = SpriteFlyingEyeHand;
        appearance.wingsPosition = plasma::Vector<3, float>(3.0f, -5.0f, -3.5f);
        appearance.wingsRotation = 50.0f;
        break;
    case RaceElfFemale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteElfHeadF01 + style->face % 6;
        appearance.hairModel = SpriteElfHairF01 + style->haircut % 11;
        appearance.chestModel = SpriteBody2Slot0;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        break;
    case RaceBandit:
        appearance.headModel = SpriteElfHeadFemale;
        appearance.hairModel = -1;
        appearance.chestModel = SpriteBody2Slot0;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        break;
    case RaceOldMan:
        appearance.headModel = SpriteOldHead;
        appearance.chestModel = SpriteBody2Slot0;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        break;
    case RaceGoblinMale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteGoblinHeadM01 + style->face % 5;
        appearance.hairModel = SpriteGoblinHairM01 + style->haircut % 6;
        appearance.chestModel = SpriteBody2Slot0;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteGoblinHand;
        appearance.weaponScale = 1.2f;
        appearance.size /= 1.2f;
        break;
    case RaceGoblinFemale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteGoblinHeadF01 + style->face % 5;
        appearance.hairModel = SpriteGoblinHairF01 + style->haircut % 6;
        appearance.chestModel = SpriteBody2Slot0;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteGoblinHand;
        appearance.weaponScale = 1.2f;
        appearance.size /= 1.2f;
        break;
    case RaceBullterrier:
        appearance.hairColor = plasma::Vector<3, unsigned char>(rand() % 256, rand() % 256, rand() % 256);
        appearance.headModel = SpriteBullterrierHead1 + rand() % 3;
        appearance.chestModel = SpriteBody2Slot0;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteBrownHand;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 5.0f);
        appearance.headScale = 0.9f;
        appearance.weaponScale = 1.2f;
        break;
    case RaceLizardmanMale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteLizardHeadM01 + style->face % 2;
        appearance.hairModel = SpriteLizardHairM01 + style->haircut % 6;
        appearance.chestModel = SpriteLizardBody;
        appearance.feetModel = SpriteLizardFoot;
        appearance.handsModel = SpriteLizardHand;
        break;
    case RaceLizardmanFemale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteLizardHeadF01 + style->face % 5;
        appearance.hairModel = SpriteLizardHairM01 + style->haircut % 6;
        appearance.chestModel = SpriteLizardBody;
        appearance.feetModel = SpriteLizardFoot;
        appearance.handsModel = SpriteLizardHand;
        break;
    case RaceGnobold:
        appearance.headModel = SpriteGnoboldHead;
        appearance.chestModel = SpriteZombieBody;
        appearance.feetModel = SpriteGnoboldFoot;
        appearance.handsModel = SpriteGnoboldHand;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceSnowGnobold:
        appearance.headModel = SpriteSnowGnoboldHead;
        appearance.chestModel = SpriteSnowGnoboldBody;
        appearance.feetModel = SpriteSnowGnoboldFoot;
        appearance.handsModel = SpriteSnowGnoboldHand;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceInsectoid:
        appearance.headModel = SpriteInsectoidHead;
        appearance.chestModel = SpriteInsectoidBody;
        appearance.feetModel = SpriteInsectoidFoot;
        appearance.handsModel = SpriteInsectoidHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 0.0f, 1.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceMantis:
        appearance.headModel = SpriteMantisHead;
        appearance.chestModel = SpriteMantisBody;
        appearance.feetModel = SpriteMantisFoot;
        appearance.handsModel = SpriteMantisHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 0.0f, 1.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceGnoll:
        appearance.headModel = SpriteGnollHead;
        appearance.chestModel = SpriteGnollBody;
        appearance.feetModel = SpriteGnollFoot;
        appearance.handsModel = SpriteGnollHand;
        appearance.size /= 1.2f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceGnome:
        appearance.headModel = SpriteGnomeHead;
        appearance.chestModel = SpriteGnomeBody;
        appearance.feetModel = SpriteGnomeFoot;
        appearance.handsModel = SpriteHand2;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, -7.5f, 4.5f);
        appearance.headPosition[2] = 4.0f;
        appearance.feetPosition[1] = 4.0f;
        appearance.handsScale *= 1.2;
        appearance.feetScale *= 1.2f;
        appearance.size /= 1.2f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceSupplierGnome:
        appearance.headModel = SpriteSupplierGnomeHead;
        appearance.chestModel = SpriteSupplierGnomeBody;
        appearance.feetModel = SpriteGnomeFoot;
        appearance.handsModel = SpriteHand2;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, -7.5f, 4.5f);
        appearance.headPosition[2] = 4.0f;
        appearance.feetPosition[1] = 4.0f;
        appearance.handsScale *= 1.2;
        appearance.feetScale *= 1.2f;
        appearance.size /= 1.2f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RacePolarGnoll:
        appearance.headModel = SpritePolarGnollHead;
        appearance.chestModel = SpritePolarGnollBody;
        appearance.feetModel = SpritePolarGnollFoot;
        appearance.handsModel = SpritePolarGnollHand;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.5f, 4.0f);
        appearance.headScale = 1.2f;
        appearance.size /= 1.2f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceMermaid:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteMermaidHeadF01 + style->face % 3;
        appearance.hairModel = SpriteMermaidHairF01 + style->haircut % 3;
        appearance.chestModel = SpriteMermaidBody;
        appearance.handsModel = SpriteMermaidHandSlot1563;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceMerman:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteMermanHeadM01 + style->face % 3;
        appearance.hairModel = SpriteMermanHairM01 + style->haircut % 3;
        appearance.chestModel = SpriteMermanBody;
        appearance.handsModel = SpriteMermaidHandSlot1563;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceEmpireSoldierMale:
        appearance.headModel = SpriteEmpireSoldierHeadM;
        appearance.hairModel = SpriteEmpireSoldierHair01 + style->haircut % 4;
        appearance.chestModel = SpriteBody4Slot1;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        break;
    case RaceEmpireSoldierFemale:
        appearance.headModel = SpriteEmpireSoldierHeadF;
        appearance.hairModel = SpriteEmpireSoldierHair01 + style->haircut % 4;
        appearance.chestModel = SpriteBody4Slot1;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        break;
    case RaceEmpress:
        appearance.headModel = SpriteEmpressHead;
        appearance.chestModel = SpriteEmpressBody;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteUndeadHand;
        appearance.size *= 1.5f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceHalfDemon:
        appearance.headModel = SpriteHalfdemonHead;
        appearance.chestModel = SpriteEmpressBody;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHalfdemonHand;
        appearance.size *= 1.5f;
        break;
    case RaceArchdruid:
        appearance.headModel = SpriteArchdruidHead;
        appearance.chestModel = SpriteArchdruidBody;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        appearance.appearanceFlags |= AppearanceFlagUnarmored | AppearanceFlagSpiritForm;
        appearance.size *= 1.5f;
        break;
    case RaceGhostPreacher:
        appearance.headModel = SpriteGhostPreacherHead;
        appearance.chestModel = SpriteGhostPreacherBody;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteUndeadHand;
        appearance.size *= 2.0f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        appearance.headScale = 0.8f;
        appearance.handsScale = 0.7f;
        break;
    case RaceLegendaryPaladin:
        appearance.headModel = SpriteLegendaryPaladinHead;
        appearance.chestModel = SpriteLegendaryPaladinBody;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        appearance.headScale = 0.7f;
        appearance.headPosition[1] = 2.0f;
        appearance.headPosition[2] = 6.0f;
        appearance.chestPosition[2] = -4.0f;
        appearance.handsPosition[2] = 1.0f;
        appearance.feetPosition[2] = -12.5f;
        appearance.size[2] = 2.5f;
        appearance.size *= 2.0f;
        break;
    case RaceAngel:
        appearance.headModel = SpriteAngelHead;
        appearance.chestModel = SpriteAngelBody;
        appearance.wingsModel = SpriteAngelWings;
        appearance.handsModel = SpriteHand2;
        appearance.wingsRotation = 80.0f;
        appearance.wingScale = 1.5f;
        appearance.wingsPosition[2] -= 1.0f;
        appearance.size[2] = 2.8f;
        appearance.size *= 1.3f;
        appearance.appearanceFlags |= AppearanceFlagFlying;
        break;
    case RaceMonkey:
        appearance.headModel = SpriteMonkeyHead;
        appearance.chestModel = SpriteMonkeyBody;
        appearance.feetModel = SpriteMonkeyFoot;
        appearance.handsModel = SpriteMonkeyHand;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.5f, 2.0f);
        appearance.feetPosition = plasma::Vector<3, float>(3.5f, 1.0f, -8.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        appearance.headScale = 1.05f;
        appearance.chestScale = 0.8f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.7f) * 0.8f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceMakake:
        appearance.headModel = SpriteMakakeHead;
        appearance.chestModel = SpriteMakakeBody;
        appearance.feetModel = SpriteMakakeFoot;
        appearance.handsModel = SpriteMakakeHand;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 2.5f, -1.0f);
        appearance.feetPosition = plasma::Vector<3, float>(2.0f, 1.0f, -9.5f);
        appearance.handsPosition = plasma::Vector<3, float>(3.5f, 1.0f, -2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -5.5f);
        appearance.headScale = 0.55f;
        appearance.chestScale = 0.7f;
        appearance.feetScale = 0.5f;
        appearance.handsScale = 0.6f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.7f) * 1.5f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceDwarfMale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteDwarfHeadM01 + style->face % 5;
        appearance.hairModel = SpriteDwarfHairM01 + style->haircut % 3;
        appearance.chestModel = SpriteDwarfBody;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        appearance.weaponScale = 1.2f;
        appearance.headScale = 0.9f;
        appearance.size /= 1.2f;
        break;
    case RaceDwarfFemale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteDwarfHeadF01 + style->face % 5;
        appearance.hairModel = SpriteDwarfHairF01 + style->haircut % 6;
        appearance.chestModel = SpriteDwarfBodyFemale;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        appearance.weaponScale = 1.2f;
        appearance.headScale = 0.9f;
        appearance.size /= 1.2f;
        break;
    case RaceOrcMale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteOrcHeadM01 + style->face % 5;
        appearance.hairModel = SpriteOrcHairM01 + style->haircut % 8;
        appearance.chestModel = SpriteBody2Slot0;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteOrcHand;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.9f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.3f;
        break;
    case RaceOrcFemale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteOrcHeadF01 + style->face % 5;
        appearance.hairModel = SpriteOrcHairF01 + style->haircut % 4;
        appearance.chestModel = SpriteBody2Slot0;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteOrcHand;
        appearance.shoulderArmorScale = 1.1f;
        appearance.headScale = 0.8f;
        appearance.chestScale = 0.95f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.5f, 4.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.3f;
        break;
    case RaceEnhancedOrcMale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteBloodaxeOrcHeadM;
        appearance.hairModel = SpriteOrcHairM01 + style->haircut % 8;
        appearance.chestModel = SpriteBody2Slot0;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteBloodaxeOrcHand;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.9f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.3f;
        appearance.appearanceFlags |= AppearanceFlagEnhanced;
        break;
    case RaceEnhancedOrcFemale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteBloodaxeOrcHeadF;
        appearance.hairModel = SpriteOrcHairF01 + style->haircut % 4;
        appearance.chestModel = SpriteBody2Slot0;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteBloodaxeOrcHand;
        appearance.shoulderArmorScale = 1.1f;
        appearance.headScale = 0.8f;
        appearance.chestScale = 0.95f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.5f, 4.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.3f;
        appearance.appearanceFlags |= AppearanceFlagEnhanced;
        break;
    case RaceTroll:
        appearance.headModel = SpriteTrollHead;
        appearance.chestModel = SpriteTrollBody;
        appearance.feetModel = SpriteTrollFoot;
        appearance.handsModel = SpriteTrollHand;
        appearance.shoulderModel = SpriteTrollArm;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.6f;
        appearance.feetScale = 0.75f;
        appearance.shoulderModelScale = 0.9f;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 3.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 3.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored | AppearanceFlagLarge;
        break;
    case RaceManaMech:
        appearance.chestModel = SpriteManaMechBody;
        appearance.feetModel = SpriteManaMechFoot;
        appearance.handsModel = SpriteManaMechHand;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.6f;
        appearance.feetScale = 0.75f;
        appearance.shoulderModelScale = 0.9f;
        appearance.handsPosition = plasma::Vector<3, float>(10.0f, 5.0f, 5.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 3.0f);
        appearance.feetPosition = plasma::Vector<3, float>(4.0f, 0.0f, -10.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 3.0f);
        appearance.size = plasma::Vector<3, float>(4.0f, 4.0f, 10.0f);
        appearance.appearanceFlags |=
            AppearanceFlagHandAttacks | AppearanceFlagUnarmored | AppearanceFlagLarge | AppearanceFlagFullShininess;
        break;
    case RaceTreant:
        appearance.chestModel = SpriteTreantBody;
        appearance.feetModel = SpriteTreantFoot;
        appearance.handsModel = SpriteTreantHand;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.6f;
        appearance.feetScale = 0.75f;
        appearance.shoulderModelScale = 0.9f;
        appearance.handsPosition = plasma::Vector<3, float>(10.0f, 5.0f, 1.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 3.0f);
        appearance.feetPosition = plasma::Vector<3, float>(4.0f, 0.0f, -10.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 3.0f);
        appearance.size = plasma::Vector<3, float>(4.0f, 4.0f, 10.0f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceMonstrosity:
        appearance.headModel = SpriteMonstrosityHead;
        appearance.hairModel = SpriteMonstrosityHeadFeature;
        appearance.chestModel = SpriteMonstrosityBody;
        appearance.feetModel = SpriteMonstrosityFoot;
        appearance.handsModel = SpriteMonstrosityHand;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.8f;
        appearance.feetScale = 0.77f;
        appearance.handsScale = 0.95f;
        appearance.handsPosition = plasma::Vector<3, float>(11.0f, 5.0f, 3.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 3.0f);
        appearance.feetPosition = plasma::Vector<3, float>(4.0f, 3.0f, -12.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 7.5f, 7.5f);
        appearance.size = plasma::Vector<3, float>(1.6f, 1.6f, 4.8f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored |
                                      AppearanceFlagSuppressHairDebris | AppearanceFlagEnhanced;
        break;
    case RaceEnhancedPirateMale:
        appearance.headModel = SpritePirateBossHead + rand() % 2;
        appearance.chestModel = SpritePirateBossBody;
        appearance.feetModel = SpritePirateBossFoot;
        appearance.handsModel = SpritePirateBossHand;
        appearance.shoulderModel = SpritePirateBossArm;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.6f;
        appearance.feetScale = 0.75f;
        appearance.shoulderModelScale = 0.9f;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 3.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 4.0f, 5.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.5f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored | AppearanceFlagEnhanced;
        break;
    case RaceHellDemon:
        appearance.headModel = SpriteHellDemonHead;
        appearance.chestModel = SpriteHellDemonBody;
        appearance.feetModel = SpriteHellDemonFoot;
        appearance.handsModel = SpriteHellDemonHand;
        appearance.shoulderModel = SpriteHellDemonArm;
        appearance.shoulderArmorScale = 1.1f;
        appearance.headScale = 0.6f;
        appearance.feetScale = 0.75f;
        appearance.shoulderModelScale = 0.85f;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 3.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 1.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored | AppearanceFlagLarge;
        break;
    case RaceGolem:
        appearance.headModel = SpriteGolemHead;
        appearance.chestModel = SpriteGolemBody;
        appearance.feetModel = SpriteGolemFoot;
        appearance.handsModel = SpriteGolemHand;
        appearance.shoulderModel = SpriteGolemArm;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.6f;
        appearance.feetScale = 0.755f;
        appearance.shoulderModelScale = 0.9f;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 3.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 4.0f, 3.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored | AppearanceFlagLarge;
        break;
    case RaceEmberGolem:
        appearance.headModel = SpriteGolemEmberHead;
        appearance.chestModel = SpriteGolemEmberBody;
        appearance.feetModel = SpriteGolemEmberFoot;
        appearance.handsModel = SpriteGolemEmberHand;
        appearance.shoulderModel = SpriteGolemEmberArm;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.61f;
        appearance.feetScale = 0.755f;
        appearance.shoulderModelScale = 0.9f;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 3.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 4.0f, 3.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored | AppearanceFlagLarge;
        break;
    case RaceSnowGolem:
        appearance.headModel = SpriteGolemSnowHead;
        appearance.chestModel = SpriteGolemSnowBody;
        appearance.feetModel = SpriteGolemSnowFoot;
        appearance.handsModel = SpriteGolemSnowHand;
        appearance.shoulderModel = SpriteGolemSnowArm;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.61f;
        appearance.feetScale = 0.755f;
        appearance.shoulderModelScale = 0.9f;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 3.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 4.0f, 3.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored | AppearanceFlagLarge;
        break;
    case RaceWerewolf:
        appearance.headModel = SpriteWerewolfHead;
        appearance.chestModel = SpriteWerewolfBody;
        appearance.feetModel = SpriteWerewolfFoot;
        appearance.handsModel = SpriteWerewolfHand;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.8f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 3.0f);
        appearance.handsPosition = plasma::Vector<3, float>(5.0f, 4.0f, 0.0f);
        appearance.handsRotation = plasma::Vector<3, float>(0.0f, -45.0f, 0.0f);
        appearance.handsScale = 0.6f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 3.0f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceDemonMarauder:
        appearance.headModel = SpriteDemonMarauderHead2;
        appearance.chestModel = SpriteDemonMarauderBody;
        appearance.feetModel = SpriteDemonMarauderFoot;
        appearance.handsModel = SpriteDemonMarauderHand;
        appearance.shoulderModel = SpriteDemonMarauderArm;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.82f;
        appearance.feetScale = 0.9f;
        appearance.shoulderModelScale = 0.7f;
        appearance.chestScale = 1.2f;
        appearance.handsScale = 0.6f;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 5.0f, 3.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 5.0f, 6.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -2.0f);
        appearance.feetPosition = plasma::Vector<3, float>(3.5f, 1.0f, -10.5f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 2.0f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceDarkTroll:
        appearance.headModel = SpriteDarkTrollHead;
        appearance.hairModel = SpriteDarkTrollEyes;
        appearance.chestModel = SpriteDarkTrollBody;
        appearance.feetModel = SpriteDarkTrollFoot;
        appearance.handsModel = SpriteDarkTrollHand;
        appearance.shoulderModel = SpriteDarkTrollArm;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.6f;
        appearance.feetScale = 0.75f;
        appearance.shoulderModelScale = 0.9f;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 3.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 3.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored | AppearanceFlagSuppressHairDebris | AppearanceFlagLarge;
        break;
    case RaceYeti:
        appearance.headModel = SpriteYetiHead;
        appearance.chestModel = SpriteYetiBody;
        appearance.feetModel = SpriteYetiFoot;
        appearance.handsModel = SpriteYetiHand;
        appearance.shoulderModel = SpriteYetiArm;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.6f;
        appearance.feetScale = 0.75f;
        appearance.shoulderModelScale = 0.9f;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 3.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 3.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored | AppearanceFlagLarge;
        break;
    case RaceCyclops:
        appearance.headModel = SpriteCyclopsHead;
        appearance.chestModel = SpriteCyclopsBody;
        appearance.feetModel = SpriteCyclopsFoot;
        appearance.handsModel = SpriteCyclopsHand;
        appearance.shoulderModel = SpriteCyclopsArm;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.6f;
        appearance.feetScale = 0.75f;
        appearance.shoulderModelScale = 0.9f;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 3.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 3.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored | AppearanceFlagLarge;
        break;
    case RaceRuneGiant:
        appearance.headModel = SpriteRuneGiantHeadLaser;
        appearance.chestModel = SpriteRuneGiantBody01 + rand() % 2;
        appearance.handsModel = SpriteRuneGiantHand01;
        appearance.feetModel = SpriteRuneGiantFoot01;
        appearance.shoulderArmorScale = 1.2f;
        appearance.headScale = 0.7f;
        appearance.shoulderModelScale = 0.9f;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 0.0f, 5.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 4.0f, 8.0f);
        appearance.feetScale = 0.8f;
        appearance.handsScale = 1.5f;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, 1.0f, -9.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f);
        appearance.appearanceFlags |=
            AppearanceFlagHeadAimOrigin | AppearanceFlagHandAttacks | AppearanceFlagUnarmored | AppearanceFlagLarge;
        break;
    case RaceUndeadMale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteUndeadHeadM01 + style->face % 6;
        appearance.hairModel = SpriteUndeadHairM01 + style->haircut % 6;
        appearance.chestModel = SpriteBody2Slot0;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteUndeadHand;
        appearance.headScale = 0.9f;
        break;
    case RaceUndeadFemale:
        appearance.hairColor = style->hairColor;
        appearance.headModel = SpriteUndeadHeadF01 + style->face % 6;
        appearance.hairModel = SpriteUndeadHairF01 + style->haircut % 6;
        appearance.chestModel = SpriteBody2Slot0;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteUndeadHand;
        appearance.headScale = 0.9f;
        break;
    case RaceSkeleton:
        appearance.headModel = SpriteSkeletonHead;
        appearance.hairModel = SpriteSkeletonEyesSlot447;
        appearance.chestModel = SpriteSkeletonBody;
        appearance.feetModel = SpriteSkeletonFootSlot450;
        appearance.handsModel = SpriteSkeletonHand;
        appearance.headScale = 0.8f;
        appearance.handsRotation = plasma::Vector<3, float>(0.0f, -90.0f, 0.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -2.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 4.0f, 7.0f);
        appearance.handsPosition = plasma::Vector<3, float>(5.0f, 3.0f, 0.0f);
        appearance.appearanceFlags |=
            AppearanceFlagHandAttacks | AppearanceFlagUnarmored | AppearanceFlagSuppressHairDebris;
        break;
    case RaceSkeletonKnight:
        appearance.headModel = SpriteSkeletonKnightHead1 + style->face % 4;
        appearance.hairModel = SpriteSkeletonEyesSlot455;
        appearance.handsModel = SpriteSkeletonKnightHand;
        appearance.chestModel = SpriteSkeletonKnightBody;
        appearance.feetModel = SpriteSkeletonFootSlot458;
        appearance.headScale = 0.8f;
        appearance.handsRotation = plasma::Vector<3, float>(0.0f, -90.0f, 0.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -2.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 4.0f, 7.0f);
        appearance.handsPosition = plasma::Vector<3, float>(5.0f, 3.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored | AppearanceFlagSuppressHairDebris;
        break;
    case RaceSanta:
        appearance.headModel = SpriteSantaHead;
        appearance.chestModel = SpriteSantaBody;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        appearance.headScale = 0.8f;
        appearance.size *= 2.0f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceWizard:
        appearance.headModel = SpriteWizardHead;
        appearance.chestModel = SpriteWizardBody;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceNecromancer:
        appearance.headModel = SpriteNecromancerHead;
        appearance.chestModel = SpriteNecromancerBody;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceSorcerer:
        appearance.headModel = SpriteSorcererHead;
        appearance.hairModel = SpriteSorcererEyes;
        appearance.chestModel = SpriteSorcererBody;
        appearance.handsModel = SpriteSorcererHand;
        appearance.appearanceFlags |= AppearanceFlagFlying | AppearanceFlagUnarmored | AppearanceFlagSuppressHairDebris;
        break;
    case RaceSeeker:
        appearance.headModel = SpriteSeekerHead;
        appearance.hairModel = SpriteSeekerEyes;
        appearance.chestModel = SpriteSeekerBody;
        appearance.feetModel = SpriteSeekerFoot;
        appearance.handsModel = SpriteSeekerHand;
        appearance.headScale = 1.1f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored | AppearanceFlagSuppressHairDebris;
        break;
    case RaceWitch:
        appearance.headModel = SpriteWitchHead;
        appearance.chestModel = SpriteWitchBody;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 4.0f);
        appearance.headScale = 0.8f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceZombie:
        appearance.headModel = SpriteZombieHead;
        appearance.hairModel = -1;
        appearance.chestModel = SpriteZombieBody;
        appearance.feetModel = SpriteFootSlot345;
        appearance.handsModel = SpriteZombieHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 3.0f, -2.0f);
        appearance.handsRotation = plasma::Vector<3, float>(0.0f, -90.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceSpikeCreature:
        appearance.headModel = SpriteSpikeHead;
        appearance.chestModel = SpriteSpikeBody;
        appearance.feetModel = SpriteSpikeFoot;
        appearance.handsModel = SpriteSpikeHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 0.0f, -2.0f);
        appearance.chestScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceAnubis:
        appearance.headModel = SpriteAnubisHead;
        appearance.chestModel = SpriteAnubisBody;
        appearance.feetModel = SpriteAnubisFoot;
        appearance.handsModel = SpriteAnubisHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 0.0f, -2.0f);
        appearance.chestScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceHorus:
        appearance.headModel = SpriteHorusHead;
        appearance.chestModel = SpriteHorusBody;
        appearance.feetModel = SpriteHorusFoot;
        appearance.handsModel = SpriteHorusHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 0.0f, -2.0f);
        appearance.chestScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceJester:
        appearance.headModel = SpriteJesterHead;
        appearance.chestModel = SpriteJesterBody;
        appearance.feetModel = SpriteJesterFoot;
        appearance.handsModel = SpriteJesterHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 0.0f, -2.0f);
        appearance.chestScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceDarkJester:
        appearance.headModel = SpriteDarkJesterHead;
        appearance.chestModel = SpriteDarkJesterBody;
        appearance.feetModel = SpriteDarkJesterFoot;
        appearance.handsModel = SpriteDarkJesterHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 0.0f, -2.0f);
        appearance.chestScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceSpectrino:
        appearance.headModel = SpriteSpectrinoHead;
        appearance.chestModel = SpriteSpectrinoBody;
        appearance.feetModel = SpriteSpectrinoFoot;
        appearance.handsModel = SpriteSpectrinoHand;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceNomadMale:
        appearance.headModel = SpriteDesertNomadHead1 + rand() % 3;
        appearance.chestModel = SpriteDesertNomadBody;
        appearance.feetModel = SpriteDesertNomadFoot;
        appearance.handsModel =
            appearance.headModel != SpriteDesertNomadHead1 ? SpriteDesertNomadHand2 : SpriteDesertNomadHand;
        appearance.headScale = 0.9f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.8f, 6.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceNomadFemale:
        appearance.headModel = SpriteDesertNomadFemaleHead1 + rand() % 3;
        appearance.chestModel = SpriteDesertNomadBody;
        appearance.feetModel = SpriteDesertNomadFoot;
        appearance.handsModel =
            appearance.headModel != SpriteDesertNomadFemaleHead1 ? SpriteDesertNomadHand2 : SpriteDesertNomadHand;
        appearance.headScale = 0.9f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.8f, 6.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceDarkCultistMale:
        appearance.headModel = SpriteDarkCultistHead1 + rand() % 5;
        appearance.chestModel = SpriteDarkCultistBody;
        appearance.feetModel = SpriteDarkCultistFoot;
        appearance.handsModel = SpriteDarkCultistHand;
        appearance.headScale = 0.9f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.8f, 6.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceDarkCultistFemale:
        appearance.headModel = SpriteDarkCultistFemaleHead1 + rand() % 5;
        appearance.chestModel = SpriteDarkCultistBody;
        appearance.feetModel = SpriteDarkCultistFoot;
        appearance.handsModel = SpriteDarkCultistHand;
        appearance.headScale = 0.9f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.8f, 6.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RacePirateMale:
        appearance.headModel = SpritePirateHead1 + rand() % 9;
        appearance.chestModel = SpritePirateBody1 + rand() % 9;
        appearance.handsModel = SpritePirateHand;
        appearance.feetModel = SpritePirateFoot;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RacePirateFemale:
        appearance.headModel = SpritePirateFemaleHead1 + rand() % 9;
        appearance.chestModel = SpritePirateBody1 + rand() % 9;
        appearance.handsModel = SpritePirateHand;
        appearance.feetModel = SpritePirateFoot;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceDjinn:
        appearance.headModel = SpriteDjinnHead;
        appearance.chestModel = SpriteDjinnBody;
        appearance.feetModel = SpriteDjinnFoot;
        appearance.handsModel = SpriteDjinnHand;
        appearance.headScale = 0.8f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.8f, 6.0f);
        appearance.feetScale = 0.8f;
        appearance.feetPosition = plasma::Vector<3, float>(3.5f, 1.0f, -10.8f);
        appearance.size *= 1.5f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceMinotaur:
        appearance.headModel = SpriteMinotaurHead;
        appearance.chestModel = SpriteMinotaurBody;
        appearance.feetModel = SpriteMinotaurFoot;
        appearance.handsModel = SpriteMinotaurHand;
        appearance.shoulderModel = SpriteMinotaurArm;
        appearance.shoulderModelScale = 0.65f;
        appearance.headScale = 0.5f;
        appearance.feetScale = 0.9f;
        appearance.handsScale = 0.7f;
        appearance.chestScale = 0.9f;
        appearance.weaponScale = 0.9f;
        appearance.handsPosition = plasma::Vector<3, float>(7.0f, 0.0f, 1.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 4.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, 1.0f, -7.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -2.0f);
        appearance.size = plasma::Vector<3, float>(3.2f, 3.2f, 5.8f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored | AppearanceFlagEnhanced;
        break;
    case RaceImp:
        appearance.headModel = SpriteImpHead;
        appearance.hairModel = SpriteImpEyes;
        appearance.chestModel = SpriteImpBody;
        appearance.feetModel = SpriteImpFoot;
        appearance.handsModel = SpriteImpHand;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 5.0f, 0.0f);
        appearance.chestScale = 0.9f;
        appearance.handsRotation[0] = -90.0f;
        appearance.handsRotation[1] = 30.0f;
        appearance.appearanceFlags |=
            AppearanceFlagHandAttacks | AppearanceFlagUnarmored | AppearanceFlagSuppressHairDebris;
        break;
    case RaceFrog:
        appearance.headModel = SpriteFrogHead;
        appearance.chestModel = SpriteFrogBody;
        appearance.feetModel = SpriteFrogFoot;
        appearance.handsModel = SpriteFrogHand;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 0.0f, 2.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 1.0f, -8.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -6.0f);
        appearance.handsScale = 1.2f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RacePlantCreature:
        appearance.headModel = SpritePlantCreatureHead;
        appearance.chestModel = SpritePlantCreatureBody;
        appearance.feetModel = SpritePlantCreatureFoot;
        appearance.handsModel = SpritePlantCreatureHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 3.0f, 0.0f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, 2.0f, -10.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 7.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        appearance.handsScale = 0.8f;
        appearance.feetScale = 0.7f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.5f;
        appearance.handsRotation = plasma::Vector<3, float>(-45.0f, 45.0f, -45.0f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceIceling:
        appearance.headModel = SpriteIcelingHead;
        appearance.chestModel = SpriteIcelingBody;
        appearance.feetModel = SpriteIcelingFoot;
        appearance.handsModel = SpriteIcelingHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 3.0f, -3.0f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, 2.0f, -10.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 0.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -8.0f);
        appearance.handsScale = 0.6f;
        appearance.feetScale = 0.6f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.5f;
        appearance.handsRotation = plasma::Vector<3, float>(-45.0f, 45.0f, -45.0f);
        appearance.weaponScale = 0.8f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceRadishCreature:
        appearance.chestModel = SpriteRadishCreatureHead;
        appearance.feetModel = SpriteRadishCreatureFoot;
        appearance.handsModel = SpriteRadishCreatureHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 3.0f, 0.0f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, 2.0f, -8.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 7.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 5.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.5f;
        appearance.handsRotation = plasma::Vector<3, float>(-45.0f, 45.0f, -45.0f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceRadishCreatureSprout:
        appearance.chestModel = SpriteRadishCreatureSproutHead;
        appearance.feetModel = SpriteRadishCreatureSproutFoot;
        appearance.handsModel = SpriteRadishCreatureSproutHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 3.0f, 3.0f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, 0.0f, -3.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 7.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 5.0f);
        appearance.size = plasma::Vector<3, float>(1.2f, 1.2f, 1.2f);
        appearance.handsRotation = plasma::Vector<3, float>(-45.0f, 45.0f, -45.0f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RacePlantCreatureSprout:
        appearance.chestModel = SpritePlantCreatureSproutHead;
        appearance.feetModel = SpritePlantCreatureSproutFoot;
        appearance.handsModel = SpritePlantCreatureSproutHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 3.0f, 3.0f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, 0.0f, -3.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 7.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 5.0f);
        appearance.size = plasma::Vector<3, float>(1.2f, 1.2f, 1.2f);
        appearance.handsRotation = plasma::Vector<3, float>(-45.0f, 45.0f, -45.0f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceOnionling:
        appearance.chestModel = SpriteOnionlingHead;
        appearance.feetModel = SpriteOnionlingFoot;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, 0.0f, -12.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.5f;
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceChiling:
        appearance.chestModel = SpriteChilingHead;
        appearance.handsModel = SpriteChilingHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 4.0f, -2.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -2.6f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 2.0f;
        appearance.handsScale = 0.8f;
        appearance.handsRotation = plasma::Vector<3, float>(-45.0f, 45.0f, -45.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceHabanero:
        appearance.chestModel = SpriteHabaneroHead;
        appearance.handsModel = SpriteHabaneroHand;
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 4.0f, -2.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -2.6f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 2.0f;
        appearance.handsScale = 0.8f;
        appearance.handsRotation = plasma::Vector<3, float>(-45.0f, 45.0f, -45.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceBloomling:
        appearance.chestModel = SpriteBloomlingHead;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -5.5f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 2.0f;
        appearance.appearanceFlags |= AppearanceFlagFlying | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceChilingSprout:
        appearance.chestModel = SpriteChilingSprout;
        appearance.handsModel = SpriteChilingSproutHand;
        appearance.handsPosition = plasma::Vector<3, float>(5.0f, 3.0f, -5.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -3.4f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.6f;
        appearance.handsScale = 0.8f;
        appearance.handsRotation = plasma::Vector<3, float>(-45.0f, 45.0f, -45.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceDesertOnionling:
        appearance.chestModel = SpriteDesertOnionling;
        appearance.feetModel = SpriteDesertOnionlingFoot;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, 0.0f, -12.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.5f;
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceDevourer:
        appearance.chestModel = SpriteDevourerHead;
        appearance.feetModel = SpriteDevourerFoot;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, 0.0f, -11.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, -12.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.5f;
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceDuckbill:
        appearance.headModel = SpriteDuckbillHead;
        appearance.chestModel = SpriteDuckbillBody;
        appearance.feetModel = SpriteDuckbillFoot;
        appearance.handsModel = SpriteDuckbillFoot;
        appearance.tailModel = SpriteDuckbillTail;
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 8.5f, 1.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.6f);
        appearance.handsPosition = plasma::Vector<3, float>(4.0f, 1.0f, -3.7f);
        appearance.feetPosition = plasma::Vector<3, float>(4.0f, -3.0f, -3.7f);
        appearance.chestScale = 0.9f;
        appearance.headScale = 0.73f;
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.5f, -2.9f);
        appearance.tailScale = 0.8f;
        appearance.tailRotation = -80.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceCrocodile:
        appearance.headModel = SpriteCrocodileHead;
        appearance.chestModel = SpriteCrocodileBody;
        appearance.feetModel = SpriteCrocodileFoot;
        appearance.handsModel = SpriteCrocodileFoot;
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 17.5f, 2.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, -9.0f, 2.6f);
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 9.0f, -1.7f);
        appearance.feetPosition = plasma::Vector<3, float>(6.0f, -9.0f, -1.7f);
        appearance.chestScale = 2.0f;
        appearance.headScale = 1.1f;
        appearance.feetScale = 1.6f;
        appearance.handsScale = 1.6f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceMole:
        appearance.headModel = SpriteMoleHead;
        appearance.chestModel = SpriteMoleBody;
        appearance.feetModel = SpriteMoleFoot;
        appearance.handsModel = SpriteMoleHand;
        appearance.handsPosition = plasma::Vector<3, float>(9.0f, 0.0f, 2.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 1.0f, -8.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -6.0f);
        appearance.handsRotation[1] = -30.0f;
        appearance.handsScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        appearance.size /= 1.2f;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceBeaver:
        appearance.headModel = SpriteBeaverHead;
        appearance.chestModel = SpriteBeaverBody;
        appearance.feetModel = SpriteBeaverFoot;
        appearance.handsModel = SpriteBeaverHand;
        appearance.handsPosition = plasma::Vector<3, float>(7.0f, 0.0f, 2.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 1.0f, -8.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 3.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -6.0f);
        appearance.handsRotation[1] = -30.0f;
        appearance.handsScale = 0.7f;
        appearance.chestScale = 1.2f;
        appearance.feetScale = 0.8f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        appearance.size /= 1.2f;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceBiter:
        appearance.headModel = SpriteBiterHead;
        appearance.chestModel = SpriteBiterBody;
        appearance.feetModel = SpriteBiterFoot;
        appearance.handsModel = SpriteBiterHand;
        appearance.handsPosition = plasma::Vector<3, float>(9.0f, 0.0f, 2.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 1.0f, -8.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 3.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -5.0f);
        appearance.handsRotation[1] = -30.0f;
        appearance.handsScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        appearance.size *= 1.1f;
        break;
    case RaceKoala:
        appearance.headModel = SpriteKoalaHead;
        appearance.chestModel = SpriteKoalaBody;
        appearance.feetModel = SpriteKoalaFoot;
        appearance.handsModel = SpriteKoalaHand;
        appearance.handsPosition = plasma::Vector<3, float>(9.0f, 0.0f, 2.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 1.0f, -8.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 3.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -5.0f);
        appearance.handsRotation[1] = -30.0f;
        appearance.handsScale = 0.9f;
        appearance.headScale = 1.2f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        appearance.size *= 1.1f;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceSquirrel:
        appearance.headModel = SpriteSquirrelHead;
        appearance.chestModel = SpriteSquirrelBody;
        appearance.feetModel = SpriteSquirrelFoot;
        appearance.handsModel = SpriteSquirrelHand;
        appearance.handsPosition = plasma::Vector<3, float>(7.0f, 0.0f, 0.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 1.0f, -8.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 4.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, -5.0f, -5.0f);
        appearance.handsRotation[1] = -30.0f;
        appearance.handsScale = 0.75f;
        appearance.feetScale = 0.8f;
        appearance.headScale = 1.2f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        appearance.size *= 0.7f;
        break;
    case RaceRaccoon:
        appearance.headModel = SpriteRaccoonHead;
        appearance.chestModel = SpriteRaccoonBody;
        appearance.feetModel = SpriteRaccoonFoot;
        appearance.handsModel = SpriteRaccoonHand;
        appearance.handsPosition = plasma::Vector<3, float>(7.0f, 0.0f, 0.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 1.0f, -8.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 4.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, -5.0f, -5.0f);
        appearance.handsRotation[1] = -30.0f;
        appearance.handsScale = 0.75f;
        appearance.feetScale = 0.8f;
        appearance.headScale = 1.2f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        appearance.size *= 0.9f;
        break;
    case RaceOwl:
        appearance.headModel = SpriteOwlHead;
        appearance.wingsModel = SpriteOwlHand;
        appearance.feetModel = SpriteOwlFoot;
        appearance.handsPosition = plasma::Vector<3, float>(9.0f, 0.0f, 2.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 0.0f, -11.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, -2.0f);
        appearance.handsScale = 0.9f;
        appearance.chestRotation = -10.0f;
        appearance.wingsRotation = 80.0f;
        appearance.wingsPosition = plasma::Vector<3, float>(3.0f, -5.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagFlying | AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        appearance.size /= 1.2f;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceSpitter:
        appearance.chestModel = SpriteSquidBody;
        appearance.feetModel = SpriteSquidFoot;
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 1.0f, -5.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack;
        break;
    case RaceBunny:
        appearance.chestModel = SpriteBunnyBody;
        appearance.feetModel = SpriteBunnyFoot;
        appearance.handsPosition = plasma::Vector<3, float>(9.0f, 0.0f, 8.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 1.0f, -2.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 8.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.handsScale = 0.9f;
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.1f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RacePorcupine:
        appearance.chestModel = SpritePorcupineBody;
        appearance.feetModel = SpritePorcupineFoot;
        appearance.handsPosition = plasma::Vector<3, float>(9.0f, 0.0f, 8.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 1.0f, -2.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 8.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.handsScale = 0.9f;
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.1f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceOgre:
        appearance.hairColor = plasma::Vector<3, unsigned char>(rand() % 256, rand() % 256, rand() % 256);
        appearance.headModel = SpriteOgreHead;
        appearance.chestModel = SpriteOgreBody;
        appearance.feetModel = SpriteOgreFoot;
        appearance.handsModel = SpriteOgreHand;
        appearance.headScale = 0.61f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 4.0f, 6.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -2.0f);
        appearance.handsPosition = plasma::Vector<3, float>(7.5f, 0.0f, 3.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 2.5f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored | AppearanceFlagEnhanced;
        break;
    case RaceRockling:
        appearance.headModel = SpriteRocklingHead;
        appearance.handsModel = SpriteRocklingHand;
        appearance.feetModel = SpriteRocklingFoot;
        appearance.weaponScale = 0.65f;
        appearance.headScale = 0.8f;
        appearance.feetScale = 0.85f;
        appearance.handsScale = 0.85f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.handsPosition = plasma::Vector<3, float>(7.5f, 0.0f, 5.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -1.0f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, 1.0f, -6.5f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.2f) * 3.0f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RacePenguin:
        appearance.headModel = SpritePenguinHead;
        appearance.chestModel = SpritePenguinBody;
        appearance.feetModel = SpritePenguinFoot;
        appearance.handsModel = SpritePenguinHand;
        appearance.handsPosition = plasma::Vector<3, float>(8.0f, 0.0f, 2.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 1.0f, -8.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -6.0f);
        appearance.handsScale = 0.9f;
        appearance.handsRotation = plasma::Vector<3, float>(-60.0f, -45.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        appearance.size /= 1.2f;
        break;
    case RaceGreenSlime:
        appearance.chestModel = SpriteSlimeGreen;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestScale = 1.5f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 0.8f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RacePinkSlime:
        appearance.chestModel = SpriteSlimePink;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestScale = 1.5f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 0.8f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceYellowSlime:
        appearance.chestModel = SpriteSlimeYellow;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestScale = 1.5f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 0.8f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceBlueSlime:
        appearance.chestModel = SpriteSlimeBlue;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestScale = 1.5f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 0.8f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceSkullSlater:
        appearance.headModel = SpriteSlaterBody2;
        appearance.hairModel = SpriteSlaterHead2;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestScale = 0.5f;
        appearance.headScale = 0.5f;
        appearance.size = plasma::Vector<3, float>(9.0f, 9.0f, 8.4f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored |
                                      AppearanceFlagSuppressHairDebris;
        break;
    case RaceFrightener:
        appearance.headModel = SpriteFrightenerHead;
        appearance.hairModel = SpriteFrightenerEyes;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagFlying;
        appearance.headScale = 0.8f;
        appearance.size = plasma::Vector<3, float>(1.4f, 1.4f, 2.5f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagHandAttacks | AppearanceFlagUnarmored |
                                      AppearanceFlagSuppressHairDebris;
        break;
    case RaceSandHorror:
        appearance.headModel = SpriteSandHorrorHead;
        appearance.handsModel = SpriteSandHorrorHand;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 0.0f, 2.0f);
        appearance.appearanceFlags |= AppearanceFlagFlying;
        appearance.headScale = 0.8f;
        appearance.handsScale = 0.5f;
        appearance.size = plasma::Vector<3, float>(1.4f, 1.4f, 2.5f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceWraith:
        appearance.headModel = SpriteWraithHead;
        appearance.hairModel = SpriteWraithEyes;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagFlying | AppearanceFlagSpiritForm;
        appearance.size = plasma::Vector<3, float>(4.0f, 4.0f, 4.0f);
        appearance.headScale = 0.5f;
        appearance.handsScale = 0.5f;
        appearance.handsRotation[1] = -45.0f;
        appearance.handsPosition = plasma::Vector<3, float>(7.0f, 3.0f, -4.0f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagFlying | AppearanceFlagHandAttacks |
                                      AppearanceFlagUnarmored | AppearanceFlagSuppressHairDebris;
        break;
    case RaceSapphireFish:
        appearance.chestModel = SpriteFish1;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 0.8f) * ((float)rand() * 0.5f / (float)RAND_MAX + 1.2f);
        appearance.chestScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagFish;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceLemonFish:
        appearance.chestModel = SpriteFish2;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 0.8f) * ((float)rand() * 0.5f / (float)RAND_MAX + 1.2f);
        appearance.chestScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagFish;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceShark:
        appearance.chestModel = SpriteShark;
        appearance.chestScale = 1.2f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.size = plasma::Vector<3, float>(2.4f, 2.4f, 6.0f);
        appearance.appearanceFlags |= AppearanceFlagFish;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceLanternFish:
        appearance.headModel = SpriteLanternFish;
        appearance.hairModel = SpriteLanternFishEyes;
        appearance.headScale = 1.2f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.size = plasma::Vector<3, float>(2.4f, 2.4f, 6.0f);
        appearance.appearanceFlags |= AppearanceFlagFish;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored |
                                      AppearanceFlagSuppressHairDebris;
        break;
    case RaceMawFish:
        appearance.chestModel = SpriteMawfish;
        appearance.chestScale = 1.2f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.size = plasma::Vector<3, float>(2.4f, 2.4f, 6.0f);
        appearance.appearanceFlags |= AppearanceFlagFish;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceBlowfish:
        appearance.chestModel = SpriteBlowfish;
        appearance.chestScale = 1.2f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.size = plasma::Vector<3, float>(2.4f, 2.4f, 6.0f);
        appearance.appearanceFlags |= AppearanceFlagFish;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RacePiranha:
        appearance.chestModel = SpritePiranha;
        appearance.chestScale = 1.2f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.size = plasma::Vector<3, float>(2.4f, 2.4f, 6.0f);
        appearance.appearanceFlags |= AppearanceFlagFish;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceSeahorse:
        appearance.chestModel = SpriteSeahorse;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 2.0f);
        appearance.appearanceFlags |= AppearanceFlagFish;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceCrab:
        appearance.headModel = SpriteCrabHead;
        appearance.chestModel = SpriteCrabBody;
        appearance.feetModel = SpriteCrabFoot;
        appearance.handsModel = SpriteCrabHand;
        appearance.handsPosition = plasma::Vector<3, float>(10.0f, 0.0f, 8.0f);
        appearance.feetPosition = plasma::Vector<3, float>(6.0f, 1.0f, -5.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 5.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.handsRotation = plasma::Vector<3, float>(90.0f, 30.0f, 0.0f);
        appearance.headScale = 0.75f;
        appearance.handsScale = 1.2f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.2f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceSeaCrab:
        appearance.headModel = SpriteCrabHeadBlue;
        appearance.chestModel = SpriteCrabBodyBlue;
        appearance.feetModel = SpriteCrabFootBlue;
        appearance.handsModel = SpriteCrabHandBlue;
        appearance.handsPosition = plasma::Vector<3, float>(10.0f, 0.0f, 8.0f);
        appearance.feetPosition = plasma::Vector<3, float>(6.0f, 1.0f, -5.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 5.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.handsRotation = plasma::Vector<3, float>(90.0f, 30.0f, 0.0f);
        appearance.headScale = 0.75f;
        appearance.handsScale = 1.2f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.2f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceBarkBeetle:
        appearance.headModel = SpriteBarkbeetleHead;
        appearance.feetModel = SpriteBarkbeetleFoot;
        appearance.feetPosition = plasma::Vector<3, float>(3.5f, 1.0f, -2.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 1.0f);
        appearance.headScale = 0.75f;
        appearance.size = plasma::Vector<3, float>(1.6f, 1.6f, 1.6f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceCaterpillar:
        appearance.headModel = SpriteCaterpillarHead;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, -1.5f);
        appearance.headScale = 0.75f;
        appearance.size = plasma::Vector<3, float>(1.6f, 1.6f, 1.6f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceSnail:
        appearance.chestModel = SpriteSnailHead;
        appearance.hairModel = SpriteSnailHouse;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, -1.4f, 3.0f);
        appearance.headScale = 1.1f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.2f);
        appearance.size = plasma::Vector<3, float>(1.6f, 1.6f, 1.6f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceEarthCaterpillar:
        appearance.headModel = SpriteEarthCaterpillarHead;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, -1.5f);
        appearance.headScale = 0.75f;
        appearance.size = plasma::Vector<3, float>(1.6f, 1.6f, 1.6f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceFireBeetle:
        appearance.headModel = SpriteFirebeetleHead;
        appearance.feetModel = SpriteFirebeetleFoot;
        appearance.feetPosition = plasma::Vector<3, float>(3.5f, 1.0f, -2.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 1.0f);
        appearance.headScale = 0.9f;
        appearance.size = plasma::Vector<3, float>(1.6f, 1.6f, 1.6f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceCrawler:
        appearance.headModel = SpriteCrawlerHead;
        appearance.chestModel = SpriteCrawlerBody;
        appearance.feetModel = SpriteCrawlerFoot;
        appearance.feetPosition = plasma::Vector<3, float>(3.5f, 1.0f, -2.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 6.0f, 1.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 6.0f);
        appearance.headScale = 0.9f;
        appearance.size = plasma::Vector<3, float>(1.6f, 1.6f, 1.6f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceSnoutBeetle:
        appearance.headModel = SpriteSnoutbeetleHead;
        appearance.feetModel = SpriteSnoutbeetleFoot;
        appearance.feetPosition = plasma::Vector<3, float>(3.5f, 1.0f, -2.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 1.0f);
        appearance.headScale = 0.9f;
        appearance.size = plasma::Vector<3, float>(1.6f, 1.6f, 1.6f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceLemonBeetle:
        appearance.headModel = SpriteLemonbeetleHead;
        appearance.feetModel = SpriteLemonbeetleFoot;
        appearance.feetPosition = plasma::Vector<3, float>(3.5f, 1.0f, -2.5f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 1.0f);
        appearance.headScale = 0.9f;
        appearance.size = plasma::Vector<3, float>(1.6f, 1.6f, 1.6f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceLich:
        appearance.headModel = SpriteLichHead;
        appearance.chestModel = SpriteLichBody;
        appearance.handsModel = SpriteLichHand;
        appearance.feetModel = -1;
        appearance.shoulderModel = SpriteLichArm;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 3.0f);
        appearance.handsPosition = plasma::Vector<3, float>(6.0f, 5.0f, 6.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 1.0f, 11.0f);
        appearance.headScale = 0.4f;
        appearance.handsScale = 0.75f;
        appearance.handsRotation = plasma::Vector<3, float>(0.0f, -90.0f, 0.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f);
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceHorse:
        appearance.headModel = SpritePonyHead;
        appearance.chestModel = SpritePonyBody;
        appearance.feetModel = SpritePonyFoot;
        appearance.handsModel = SpritePonyFoot;
        appearance.tailModel = SpritePonyTail;
        appearance.headScale = 0.8f;
        appearance.size = plasma::Vector<3, float>(1.8f, 1.8f, 2.0f) * 0.8f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 8.0f, 6.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 3.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 3.0f, -3.25f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -5.0f, -3.25f);
        appearance.feetScale = 0.8f;
        appearance.handsScale = 0.8f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -7.0f, -4.0f);
        appearance.tailScale = 1.0f;
        appearance.chestScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceGoldKnightHorse:
        appearance.headModel = SpriteGoldKnightHorseHead;
        appearance.chestModel = SpriteGoldKnightHorseBody;
        appearance.feetModel = SpriteGoldKnightHorseFoot;
        appearance.handsModel = SpriteGoldKnightHorseFoot;
        appearance.tailModel = SpriteGoldKnightHorseTail;
        appearance.headScale = 0.8f;
        appearance.size = plasma::Vector<3, float>(1.8f, 1.8f, 2.0f) * 0.8f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 8.0f, 6.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 3.0f, -3.25f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -5.0f, -3.25f);
        appearance.feetScale = 0.8f;
        appearance.handsScale = 0.8f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -7.0f, -4.0f);
        appearance.tailScale = 1.0f;
        appearance.chestScale = 0.91f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceSkeletonHorse:
        appearance.headModel = SpriteSkullHorseHead;
        appearance.hairModel = SpriteSkullHorseEyes;
        appearance.chestModel = SpriteSkullHorseBody;
        appearance.feetModel = SpriteSkullHorseFoot;
        appearance.handsModel = SpriteSkullHorseFoot;
        appearance.tailModel = SpriteSkullHorseTail;
        appearance.headScale = 0.8f;
        appearance.size = plasma::Vector<3, float>(1.8f, 1.8f, 2.0f) * 0.8f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 8.0f, 6.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 3.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 3.0f, -3.25f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -5.0f, -3.25f);
        appearance.feetScale = 0.8f;
        appearance.handsScale = 0.8f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -7.0f, -4.0f);
        appearance.tailScale = 1.0f;
        appearance.chestScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored |
                                      AppearanceFlagSuppressHairDebris;
        break;
    case RaceZebra:
        appearance.headModel = SpriteZebraHead;
        appearance.chestModel = SpriteZebraBody;
        appearance.feetModel = SpriteZebraFoot;
        appearance.handsModel = SpriteZebraFoot;
        appearance.tailModel = SpriteZebraTail;
        appearance.headScale = 0.75f;
        appearance.size = plasma::Vector<3, float>(1.8f, 1.8f, 2.0f) * 1.0f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 8.0f, 6.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.5f, 4.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 3.0f, -3.25f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -3.25f);
        appearance.feetScale = 0.8f;
        appearance.handsScale = 0.8f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -7.0f, -4.0f);
        appearance.tailScale = 1.0f;
        appearance.chestScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceCamel:
        appearance.headModel = SpriteCamelHead;
        appearance.chestModel = SpriteCamelBody;
        appearance.feetModel = SpriteCamelFoot;
        appearance.handsModel = SpriteCamelFoot;
        appearance.headScale = 0.65f;
        appearance.size = plasma::Vector<3, float>(1.8f, 1.8f, 3.0f) * 1.0f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 12.0f, 4.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 3.0f, -5.75f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -5.75f);
        appearance.feetScale = 0.75f;
        appearance.handsScale = 0.8f;
        appearance.chestScale = 0.95f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceMoose:
        appearance.headModel = SpriteMooseHead;
        appearance.chestModel = SpriteMooseBody;
        appearance.feetModel = SpriteMooseFoot;
        appearance.handsModel = SpriteMooseFoot;
        appearance.headScale = 0.65f;
        appearance.size = plasma::Vector<3, float>(1.8f, 1.8f, 3.0f) * 1.0f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 11.0f, 4.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 3.0f, -5.75f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -5.75f);
        appearance.feetScale = 0.7f;
        appearance.handsScale = 0.8f;
        appearance.chestScale = 1.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceManaDeer:
        appearance.headModel = SpriteManaDeerHead;
        appearance.chestModel = SpriteManaDeerBody;
        appearance.feetModel = SpriteManaDeerFoot;
        appearance.handsModel = SpriteManaDeerHand;
        appearance.headScale = 0.65f;
        appearance.size = plasma::Vector<3, float>(1.8f, 1.8f, 3.0f) * 1.0f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 8.0f, 14.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 1.0f, 5.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 3.0f, -5.75f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -5.75f);
        appearance.feetScale = 0.7f;
        appearance.handsScale = 0.8f;
        appearance.chestScale = 0.8f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceCow:
        appearance.headModel = SpriteCowHead;
        appearance.chestModel = SpriteCowBody;
        appearance.feetModel = SpriteCowFoot;
        appearance.handsModel = SpriteCowFoot;
        appearance.headScale = 0.65f;
        appearance.size = plasma::Vector<3, float>(1.8f, 1.8f, 3.0f) * 1.0f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 10.0f, 1.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -2.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 3.0f, -6.75f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -6.75f);
        appearance.feetScale = 0.75f;
        appearance.handsScale = 0.8f;
        appearance.chestScale = 1.2f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceCollie:
        appearance.headModel = SpriteCollieHead;
        appearance.chestModel = SpriteCollieBody;
        appearance.feetModel = SpriteCollieFoot;
        appearance.handsModel = SpriteCollieHand;
        appearance.tailModel = SpriteCollieTail;
        appearance.headScale = 0.9f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, 6.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -2.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -1.0f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -1.0f);
        appearance.tailScale = 1.0f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceSavageDog:
        appearance.headModel = SpriteSavageDogHead;
        appearance.chestModel = SpriteSavageDogBody;
        appearance.feetModel = SpriteSavageDogFoot;
        appearance.handsModel = SpriteSavageDogHand;
        appearance.tailModel = SpriteSavageDogTail;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 19.0f, 6.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, -1.0f, 4.0f);
        appearance.handsPosition = plasma::Vector<3, float>(4.0f, 5.0f, -3.5f);
        appearance.feetPosition = plasma::Vector<3, float>(4.0f, -5.0f, -3.0f);
        appearance.headScale = 1.1f;
        appearance.chestScale = 1.5f;
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.2f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -1.0f);
        appearance.tailScale = 0.7f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceAlphaDog:
        appearance.headModel = SpriteAlphaDogHead;
        appearance.chestModel = SpriteAlphaDogBody;
        appearance.feetModel = SpriteAlphaDogFoot;
        appearance.handsModel = SpriteAlphaDogHand;
        appearance.tailModel = SpriteAlphaDogTail;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f) * 2.0f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 19.0f, 6.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, -1.0f, 4.0f);
        appearance.handsPosition = plasma::Vector<3, float>(4.0f, 5.0f, -3.5f);
        appearance.feetPosition = plasma::Vector<3, float>(4.0f, -5.0f, -3.0f);
        appearance.headScale = 1.1f;
        appearance.chestScale = 1.5f;
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.2f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -1.0f);
        appearance.tailScale = 0.7f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceLionMale:
        appearance.headModel = SpriteLionHeadMale;
        appearance.chestModel = SpriteLionBody;
        appearance.feetModel = SpriteLionFoot;
        appearance.handsModel = SpriteLionFoot;
        appearance.tailModel = SpriteLionTail;
        appearance.headScale = 0.7f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f) * 2.3f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 12.0f, 4.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -2.7f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -2.7f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -2.0f);
        appearance.tailScale = 0.9f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceLionFemale:
        appearance.headModel = SpriteLionHeadFemale;
        appearance.chestModel = SpriteLionBody;
        appearance.feetModel = SpriteLionFoot;
        appearance.handsModel = SpriteLionFoot;
        appearance.tailModel = SpriteLionTail;
        appearance.headScale = 0.7f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f) * 2.1f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 12.0f, 4.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -2.7f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -2.7f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -2.0f);
        appearance.tailScale = 0.9f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceSnowLionMale:
        appearance.headModel = SpriteSnowLionHeadMale;
        appearance.chestModel = SpriteSnowLionBody;
        appearance.feetModel = SpriteSnowLionFoot;
        appearance.handsModel = SpriteSnowLionFoot;
        appearance.tailModel = SpriteSnowLionTail;
        appearance.headScale = 0.7f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f) * 2.3f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 12.0f, 4.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -2.7f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -2.7f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -2.0f);
        appearance.tailScale = 0.9f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceSnowLionFemale:
        appearance.headModel = SpriteSnowLionHeadFemale;
        appearance.chestModel = SpriteSnowLionBody;
        appearance.feetModel = SpriteSnowLionFoot;
        appearance.handsModel = SpriteSnowLionFoot;
        appearance.tailModel = SpriteSnowLionTail;
        appearance.headScale = 0.7f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f) * 2.1f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 12.0f, 4.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -2.7f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -2.7f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -2.0f);
        appearance.tailScale = 0.9f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceSilverLion:
        appearance.headModel = SpriteSilverLionHead;
        appearance.chestModel = SpriteSilverLionBody;
        appearance.feetModel = SpriteSilverLionFoot;
        appearance.handsModel = SpriteSilverLionFoot;
        appearance.tailModel = SpriteSilverLionTail;
        appearance.headScale = 0.7f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f) * 2.1f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 12.5f, 4.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 4.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -2.7f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -2.7f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.chestScale = 1.1f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -4.0f);
        appearance.tailScale = 0.6f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceShepherdDog:
        appearance.headModel = SpriteShepherdDogHead;
        appearance.chestModel = SpriteShepherdDogBody;
        appearance.feetModel = SpriteShepherdDogFoot;
        appearance.handsModel = SpriteShepherdDogHand;
        appearance.tailModel = SpriteShepherdDogTail;
        appearance.headScale = 0.9f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, 6.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -2.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -1.0f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -1.0f);
        appearance.tailScale = 1.0f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceSkeletonDog:
        appearance.headModel = SpriteSkeletonDogHead;
        appearance.chestModel = SpriteSkeletonDogBody;
        appearance.feetModel = SpriteSkeletonDogFoot;
        appearance.handsModel = SpriteSkeletonDogHand;
        appearance.headScale = 0.75f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 12.0f, 5.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 4.0f);
        appearance.handsPosition = plasma::Vector<3, float>(2.5f, 3.0f, -3.6f);
        appearance.feetPosition = plasma::Vector<3, float>(2.5f, -6.0f, -1.6f);
        appearance.feetScale = 0.75f;
        appearance.handsScale = 0.75f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceSkullBull:
        appearance.headModel = SpriteSkullBullHead;
        appearance.chestModel = SpriteSkullBullBody;
        appearance.feetModel = SpriteSkullBullFoot;
        appearance.handsModel = SpriteSkullBullHand;
        appearance.tailModel = SpriteSkullBullTail;
        appearance.headScale = 0.7f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 12.5f, 3.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 2.5f, -4.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -4.0f);
        appearance.feetScale = 0.8f;
        appearance.handsScale = 0.8f;
        appearance.chestScale = 1.2f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -2.5f);
        appearance.tailScale = 1.0f;
        appearance.tailRotation = -30.0f;
        appearance.size = plasma::Vector<3, float>(2.8f, 2.8f, 4.0f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceAlpaca:
        appearance.headModel = SpriteAlpacaHead;
        appearance.chestModel = SpriteAlpacaBody;
        appearance.feetModel = SpriteAlpacaFoot;
        appearance.handsModel = SpriteAlpacaHand;
        appearance.headScale = 0.8f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, 5.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -8.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -9.0f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceBrownAlpaca:
        appearance.headModel = SpriteAlpacaBrownHead;
        appearance.chestModel = SpriteAlpacaBrownBody;
        appearance.feetModel = SpriteAlpacaBrownFoot;
        appearance.handsModel = SpriteAlpacaBrownHand;
        appearance.headScale = 0.8f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, 5.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -8.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -9.0f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceTurtle:
        appearance.headModel = SpriteTurtleHead;
        appearance.chestModel = SpriteTurtleBody;
        appearance.feetModel = SpriteTurtleFoot;
        appearance.handsModel = SpriteTurtleFoot;
        appearance.headScale = 0.65f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 13.0f, 0.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 3.5f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -3.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -3.5f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f);
        appearance.feetScale = 0.8f;
        appearance.handsScale = 0.8f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, 4.5f);
        appearance.tailScale = 1.0f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceLavaTurtle:
        appearance.headModel = SpriteLavaTurtleHead;
        appearance.chestModel = SpriteLavaTurtleBody;
        appearance.feetModel = SpriteLavaTurtleFoot;
        appearance.handsModel = SpriteLavaTurtleFoot;
        appearance.headScale = 0.65f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 13.0f, 0.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 3.75f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -3.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -3.5f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f);
        appearance.feetScale = 0.8f;
        appearance.handsScale = 0.8f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, 4.5f);
        appearance.tailScale = 1.0f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceBlueTurtle:
        appearance.headModel = SpriteBlueTurtleHead;
        appearance.chestModel = SpriteBlueTurtleBody;
        appearance.feetModel = SpriteBlueTurtleFoot;
        appearance.handsModel = SpriteBlueTurtleFoot;
        appearance.headScale = 0.65f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 13.0f, 0.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 3.75f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -3.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -3.5f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.1f) * 2.0f;
        appearance.feetScale = 0.8f;
        appearance.handsScale = 0.8f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, 4.5f);
        appearance.tailScale = 1.0f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceTerrier:
        appearance.headModel = SpriteDogHead2;
        appearance.chestModel = SpriteDogBody2;
        appearance.feetModel = SpriteDogFoot2;
        appearance.handsModel = SpriteDogHand2;
        appearance.tailModel = SpriteDogTail2;
        appearance.headScale = 0.9f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, -1.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -5.5f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -8.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.1f, -8.5f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.chestScale = 1.05f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -10.0f, 3.0f);
        appearance.tailScale = 0.8f;
        appearance.tailRotation = -120.0f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.3f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceScottishTerrier:
        appearance.headModel = SpriteScottishTerrierHead;
        appearance.chestModel = SpriteScottishTerrierBody;
        appearance.feetModel = SpriteScottishTerrierFoot;
        appearance.handsModel = SpriteScottishTerrierHand;
        appearance.tailModel = SpriteScottishTerrierTail;
        appearance.headScale = 0.9f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, 5.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.5f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -2.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.1f, -2.5f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.chestScale = 1.05f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -7.0f, 2.0f);
        appearance.tailScale = 0.8f;
        appearance.tailRotation = -120.0f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 0.7f) * 1.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceWolf:
        appearance.headModel = SpriteWolfHead;
        appearance.chestModel = SpriteWolfBody;
        appearance.feetModel = SpriteWolfFoot;
        appearance.handsModel = SpriteWolfHand;
        appearance.tailModel = SpriteWolfTail;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 11.0f, -1.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -3.5f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 3.0f, -7.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.1f, -7.5f);
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -5.5f, -1.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.7f) * 1.9f;
        appearance.chestScale = 1.2f;
        appearance.headScale = 0.8f;
        appearance.feetScale = 0.9f;
        appearance.handsScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceGiraffe:
        appearance.headModel = SpriteGiraffeHead;
        appearance.chestModel = SpriteGiraffeBody;
        appearance.feetModel = SpriteGiraffeFootSlot557;
        appearance.handsModel = SpriteGiraffeFootSlot556;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 10.5f, 14.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.5f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 3.0f, -6.0f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.1f, -5.5f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.7f) * 3.6f;
        appearance.chestScale = 1.2f;
        appearance.headScale = 0.9f;
        appearance.feetScale = 0.9f;
        appearance.handsScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceAntelope:
        appearance.headModel = SpriteAntelopeHead;
        appearance.chestModel = SpriteAntelopeBody;
        appearance.feetModel = SpriteAntelopeFootSlot561;
        appearance.handsModel = SpriteAntelopeFootSlot560;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, 8.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.handsPosition = plasma::Vector<3, float>(2.5f, 2.0f, -7.0f);
        appearance.feetPosition = plasma::Vector<3, float>(2.5f, -4.5f, -7.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.7f) * 1.6f;
        appearance.chestScale = 1.1f;
        appearance.headScale = 0.9f;
        appearance.feetScale = 0.7f;
        appearance.handsScale = 0.7f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceHyena:
        appearance.headModel = SpriteHyenaHead;
        appearance.chestModel = SpriteHyenaBody;
        appearance.feetModel = SpriteHyenaFoot;
        appearance.handsModel = SpriteHyenaHand;
        appearance.tailModel = SpriteHyenaTail;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 14.0f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -0.5f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -5.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.1f, -6.5f);
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -5.5f, -1.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.7f) * 1.9f;
        appearance.chestScale = 1.15f;
        appearance.headScale = 0.8f;
        appearance.feetScale = 0.9f;
        appearance.handsScale = 0.9f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RacePanther:
        appearance.headModel = SpritePantherHead;
        appearance.chestModel = SpritePantherBody;
        appearance.feetModel = SpritePantherFootSlot570;
        appearance.handsModel = SpritePantherFootSlot569;
        appearance.tailModel = SpritePantherTail;
        appearance.headScale = 0.6f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f) * 2.3f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 11.0f, 2.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -2.7f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -2.7f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -2.0f);
        appearance.tailScale = 0.9f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceSnowLeopard:
        appearance.headModel = SpriteSnowLeopardHead;
        appearance.chestModel = SpriteSnowLeopardBody;
        appearance.feetModel = SpriteSnowLeopardFoot;
        appearance.handsModel = SpriteSnowLeopardHand;
        appearance.tailModel = SpriteSnowLeopardTail;
        appearance.headScale = 0.6f;
        appearance.chestScale = 1.01f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.0f) * 2.3f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 11.0f, 2.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -3.7f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -3.7f);
        appearance.feetScale = 0.9f;
        appearance.handsScale = 0.9f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -7.0f, -2.0f);
        appearance.tailScale = 0.75f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceCat:
        appearance.headModel = SpriteCatHead;
        appearance.chestModel = SpriteCatBody;
        appearance.feetModel = SpriteCatFoot;
        appearance.handsModel = SpriteCatHand;
        appearance.tailModel = SpriteCatTail;
        appearance.headScale = 0.9f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, -1.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -8.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.1f, -8.5f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -1.0f);
        appearance.tailScale = 1.1f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceBrownCat:
        appearance.headModel = SpriteCatHead2;
        appearance.chestModel = SpriteCatBody2;
        appearance.feetModel = SpriteCatFoot2;
        appearance.handsModel = SpriteCatHand2;
        appearance.tailModel = SpriteCatTail2;
        appearance.headScale = 0.9f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, -1.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -8.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.1f, -8.5f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -1.0f);
        appearance.tailScale = 1.1f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceWhiteCat:
        appearance.headModel = SpriteCatHead3;
        appearance.chestModel = SpriteCatBody3;
        appearance.feetModel = SpriteCatFoot3;
        appearance.handsModel = SpriteCatHand3;
        appearance.tailModel = SpriteCatTail3;
        appearance.headScale = 0.9f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, -1.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -8.5f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.1f, -8.5f);
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -1.0f);
        appearance.tailScale = 1.1f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RacePig:
        appearance.headModel = SpritePigHead;
        appearance.chestModel = SpritePigBody;
        appearance.feetModel = SpritePigFoot;
        appearance.handsModel = SpritePigFoot;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 0.8f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, 1.9f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -4.2f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -4.2f);
        appearance.chestScale = 1.2f;
        appearance.headScale = 0.85f;
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -1.0f);
        appearance.tailScale = 1.0f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceWarthog:
        appearance.headModel = SpriteWarthogHead;
        appearance.chestModel = SpriteWarthogBody;
        appearance.feetModel = SpriteWarthogFoot;
        appearance.handsModel = SpriteWarthogFoot;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 0.8f) * 1.4f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 10.0f, 1.9f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -4.2f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -4.2f);
        appearance.chestScale = 1.1f;
        appearance.headScale = 0.9f;
        appearance.feetScale = 0.9f;
        appearance.handsScale = 0.9f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -1.0f);
        appearance.tailScale = 1.0f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceSheep:
        appearance.headModel = SpriteSheepHead;
        appearance.chestModel = SpriteSheepBody;
        appearance.feetModel = SpriteSheepFoot;
        appearance.handsModel = SpriteSheepFoot;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 0.8f) * 1.5f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 4.0f, 1.9f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 4.0f, -4.2f);
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -4.2f);
        appearance.chestScale = 1.2f;
        appearance.headScale = 0.85f;
        appearance.feetScale = 1.0f;
        appearance.handsScale = 1.0f;
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -8.0f, -1.0f);
        appearance.tailScale = 1.0f;
        appearance.tailRotation = -30.0f;
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceDragon:
        appearance.headModel = SpriteDragonHead;
        appearance.chestModel = SpriteDragonBody;
        appearance.feetModel = SpriteDragonFoot;
        appearance.handsModel = SpriteDragonFoot;
        appearance.tailModel = SpriteDragonTail;
        appearance.wingsModel = SpriteDragonWing;
        appearance.headScale = 0.7f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 8.5f, 2.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.handsPosition = plasma::Vector<3, float>(4.0f, 6.0f, -9.0f);
        appearance.feetPosition = plasma::Vector<3, float>(4.0f, -3.0f, -9.0f);
        appearance.feetScale = 1.2f;
        appearance.handsScale = 1.2f;
        appearance.chestScale = 1.2f;
        appearance.wingScale = 1.4f;
        appearance.wingsPosition = plasma::Vector<3, float>(4.0f, 8.0f, 0.0f);
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -6.0f, 0.0f);
        appearance.appearanceFlags |=
            AppearanceFlagQuadruped | AppearanceFlagFlying | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceHornet:
        appearance.headModel = SpriteHornetHead;
        appearance.chestModel = SpriteHornetBody;
        appearance.feetModel = SpriteHornetFoot;
        appearance.wingsModel = SpriteHornetHand;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -1.0f, -7.0f);
        appearance.appearanceFlags |= AppearanceFlagFlying;
        appearance.chestRotation = -45.0f;
        appearance.feetRotation = 30.0f;
        appearance.wingsRotation = 45.0f;
        appearance.wingsPosition = plasma::Vector<3, float>(1.0f, -6.0f, 1.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceInsectGuard:
        appearance.headModel = SpriteInsectGuardHead;
        appearance.chestModel = SpriteInsectGuardBody;
        appearance.feetModel = SpriteInsectGuardFoot;
        appearance.wingsModel = SpriteInsectGuardWing;
        appearance.handsModel = SpriteInsectGuardHand;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -1.0f, -7.0f);
        appearance.appearanceFlags |= AppearanceFlagFlying;
        appearance.chestRotation = -45.0f;
        appearance.feetRotation = 30.0f;
        appearance.wingsRotation = 45.0f;
        appearance.wingsPosition = plasma::Vector<3, float>(1.0f, -6.0f, 1.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceFly:
        appearance.headModel = SpriteFlyHead;
        appearance.chestModel = SpriteFlyBody;
        appearance.feetModel = SpriteFlyFoot;
        appearance.wingsModel = SpriteFlyHand;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -1.0f, -7.0f);
        appearance.appearanceFlags |= AppearanceFlagFlying;
        appearance.chestRotation = -45.0f;
        appearance.feetRotation = 30.0f;
        appearance.wingsRotation = 45.0f;
        appearance.wingsPosition = plasma::Vector<3, float>(1.0f, -6.0f, 1.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 4.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceBumblebee:
        appearance.headModel = SpriteBumblebee;
        appearance.wingsModel = SpriteBumblebeeHand;
        appearance.feetModel = SpriteBumblebeeFoot;
        appearance.appearanceFlags |= AppearanceFlagFlying;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, 4.0f, -8.0f);
        appearance.chestRotation = -45.0f;
        appearance.wingsRotation = 45.0f;
        appearance.headScale = 1.2f;
        appearance.wingScale = 0.6f;
        appearance.feetScale = 1.0f;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.5f) * 1.25f;
        appearance.wingsPosition = plasma::Vector<3, float>(6.5f, 3.0f, -1.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceMidge:
        appearance.headModel = SpriteMidgeHead;
        appearance.chestModel = SpriteMidgeBody;
        appearance.feetModel = SpriteMidgeFoot;
        appearance.wingsModel = SpriteMidgeHand;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -1.0f, -5.0f);
        appearance.appearanceFlags |= AppearanceFlagFlying;
        appearance.chestRotation = -45.0f;
        appearance.feetRotation = 30.0f;
        appearance.wingsRotation = 45.0f;
        appearance.wingsPosition = plasma::Vector<3, float>(1.0f, -5.0f, -1.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 4.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceMosquito:
        appearance.headModel = SpriteMosquitoHead;
        appearance.chestModel = SpriteMosquitoBody;
        appearance.feetModel = SpriteMosquitoFoot;
        appearance.wingsModel = SpriteMosquitoHand;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -1.0f, -7.0f);
        appearance.appearanceFlags |= AppearanceFlagFlying;
        appearance.chestRotation = -45.0f;
        appearance.feetRotation = 30.0f;
        appearance.wingsRotation = 45.0f;
        appearance.wingsPosition = plasma::Vector<3, float>(1.0f, -6.0f, 1.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 4.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceCrow:
        appearance.headModel = SpriteCrowHead;
        appearance.chestModel = SpriteCrowBody;
        appearance.feetModel = SpriteCrowFoot;
        appearance.wingsModel = SpriteCrowHand;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -10.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 6.0f);
        appearance.appearanceFlags |= AppearanceFlagFlying;
        appearance.chestRotation = -45.0f;
        appearance.wingsRotation = 45.0f;
        appearance.wingsPosition = plasma::Vector<3, float>(3.0f, -5.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceChicken:
        appearance.headModel = SpriteChickenHead;
        appearance.chestModel = SpriteChickenBody;
        appearance.feetModel = SpriteChickenFoot;
        appearance.wingsModel = SpriteChickenHand;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, 0.0f, -10.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 2.0f);
        appearance.chestScale = 1.3f;
        appearance.appearanceFlags |= AppearanceFlagFlying;
        appearance.chestRotation = 0.0f;
        appearance.wingsRotation = 45.0f;
        appearance.wingsPosition = plasma::Vector<3, float>(3.0f, -2.0f, -5.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.65f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceSeagull:
        appearance.headModel = SpriteSeagullHead;
        appearance.chestModel = SpriteSeagullBody;
        appearance.feetModel = SpriteSeagullFoot;
        appearance.wingsModel = SpriteSeagullHand;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -10.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 5.0f, 8.0f);
        appearance.appearanceFlags |= AppearanceFlagFlying;
        appearance.chestRotation = -45.0f;
        appearance.wingsRotation = 45.0f;
        appearance.wingsPosition = plasma::Vector<3, float>(3.0f, -5.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceParrot:
        if (rand() % 2) {
            appearance.headModel = SpriteParrotHead;
            appearance.chestModel = SpriteParrotBody;
            appearance.feetModel = SpriteParrotFoot;
            appearance.wingsModel = SpriteParrotHand;
        } else {
            appearance.headModel = SpriteParrotBlueHead;
            appearance.chestModel = SpriteParrotBlueBody;
            appearance.feetModel = SpriteParrotBlueFoot;
            appearance.wingsModel = SpriteParrotBlueHand;
        }
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -10.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 3.0f, 6.0f);
        appearance.appearanceFlags |= AppearanceFlagFlying;
        appearance.chestRotation = -45.0f;
        appearance.wingsRotation = 45.0f;
        appearance.wingsPosition = plasma::Vector<3, float>(3.0f, -5.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceBat:
        appearance.headModel = SpriteBatHead;
        appearance.chestModel = SpriteBatBody;
        appearance.feetModel = SpriteBatFoot;
        appearance.wingsModel = SpriteBatHand;
        appearance.feetPosition = plasma::Vector<3, float>(3.0f, -4.0f, -10.0f);
        appearance.appearanceFlags |= AppearanceFlagFlying;
        appearance.chestRotation = -45.0f;
        appearance.wingsRotation = 45.0f;
        appearance.wingsPosition = plasma::Vector<3, float>(3.0f, -5.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RacePlainRunner:
        appearance.chestModel = SpritePlainrunnerBody;
        appearance.feetModel = SpritePlainrunnerFoot;
        appearance.handsModel = -1;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 3.0f, 6.0f);
        appearance.feetPosition = plasma::Vector<3, float>(4.0f, 0.0f, -1.5f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.5f;
        appearance.feetScale = 1.5f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceLeafRunner:
        appearance.chestModel = SpriteLeafrunnerBody;
        appearance.feetModel = SpriteLeafrunnerFoot;
        appearance.handsModel = -1;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 3.0f, 6.0f);
        appearance.feetPosition = plasma::Vector<3, float>(4.0f, 0.0f, -1.5f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.5f;
        appearance.feetScale = 1.5f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceSnowRunner:
        appearance.chestModel = SpriteSnowrunnerBody;
        appearance.feetModel = SpriteSnowrunnerFoot;
        appearance.handsModel = -1;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 3.0f, 6.0f);
        appearance.feetPosition = plasma::Vector<3, float>(4.0f, 0.0f, -1.5f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.5f;
        appearance.feetScale = 1.5f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceDesertRunner:
        appearance.chestModel = SpriteDesertrunnerBody;
        appearance.feetModel = SpriteDesertrunnerFoot;
        appearance.handsModel = -1;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 3.0f, 6.0f);
        appearance.feetPosition = plasma::Vector<3, float>(4.0f, 0.0f, -1.5f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.8f) * 1.5f;
        appearance.feetScale = 1.5f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RacePeacock:
        appearance.chestModel = SpritePeacockBody;
        appearance.feetModel = SpritePeacockFoot;
        appearance.headModel = SpritePeacockHead;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 4.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 4.0f, 8.0f);
        appearance.feetPosition = plasma::Vector<3, float>(4.0f, 0.0f, -6.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 2.5f) * 1.5f;
        appearance.chestScale = 0.7f;
        appearance.headScale = 0.5f;
        appearance.feetScale = 1.5f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceFlamingo:
        appearance.chestModel = SpriteFlamingoBody;
        appearance.feetModel = SpriteFlamingoFoot;
        appearance.headModel = SpriteFlamingoHead;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 4.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 10.0f, 13.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 0.0f, -7.5f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 2.5f) * 1.5f;
        appearance.chestScale = 1.0f;
        appearance.headScale = 0.7f;
        appearance.feetScale = 1.35f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceToucan:
        appearance.chestModel = SpriteToucanBody;
        appearance.feetModel = SpriteToucanFoot;
        appearance.headModel = SpriteToucanHead;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 20.0f, 7.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 2.0f, -7.5f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 2.5f) * 1.5f;
        appearance.chestScale = 1.5f;
        appearance.headScale = 1.1f;
        appearance.feetScale = 1.35f;
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceOstrich:
        appearance.chestModel = SpriteOstrichBody;
        appearance.feetModel = SpriteOstrichFoot;
        appearance.headModel = SpriteOstrichHead;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, -1.0f, 1.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, 10.0f);
        appearance.feetPosition = plasma::Vector<3, float>(5.0f, 0.0f, -8.0f);
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 2.5f) * 1.5f;
        appearance.chestScale = 0.9f;
        appearance.headScale = 0.6f;
        appearance.feetScale = 1.3f;
        appearance.appearanceFlags |= AppearanceFlagHandAttacks | AppearanceFlagUnarmored;
        break;
    case RaceBush:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestModel = SpriteBush;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceSnowBush:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestModel = SpriteSnowBush;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceCobwebScrub:
        appearance.size = plasma::Vector<3, float>(0.5f, 0.5f, 1.7f);
        appearance.chestModel = SpriteCobwebscrub;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestScale = 1.5f;
        break;
    case RaceSnowBerryBush:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestModel = SpriteBerryBush;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceCottonPlant:
        appearance.size = plasma::Vector<3, float>(0.5f, 0.5f, 1.5f);
        appearance.chestModel = SpriteCottonPlant;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceEgg:
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 1.2f);
        appearance.chestModel = SpriteEgg1Slot2889;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -1.5f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceBomb:
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 0.8f);
        appearance.chestModel = SpriteBomb1;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.chestScale = 0.8f;
        break;
    case RaceWaterBubble:
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 1.5f);
        appearance.chestModel = SpriteWaterBubble;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestScale = 1.0f;
        appearance.appearanceFlags |= AppearanceFlagFlying | AppearanceFlagNoTerrainCollision;
        break;
    case RaceScrub:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestModel = rand() % 2 ? SpriteScrub : SpriteScrubGreen;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        break;
    case RaceFireScrub:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestModel = SpriteFireScrub;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
        break;
    case RaceGinseng:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.0f);
        appearance.chestModel = SpriteGinseng;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceDemonIdol:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 2.1f);
        appearance.chestModel = SpriteIdol1 + rand() % 5;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceMagicCrystal:
        appearance.size = plasma::Vector<3, float>(1.2f, 1.2f, 2.1f);
        appearance.chestModel = SpriteMagicCrystal1 + rand() % 4;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceFireRunestone:
        appearance.size = plasma::Vector<3, float>(3.0f, 3.0f, 4.0f) * 1.7f;
        appearance.headPosition[2] = -2.0f;
        appearance.headScale = 0.6f;
        appearance.hairColor = plasma::Vector<3, unsigned char>(255, 20, 20);
        appearance.headModel = SpriteFireRunestone;
        appearance.hairModel = SpriteRunestoneGlow;
        appearance.appearanceFlags |= AppearanceFlagSuppressHairDebris;
        break;
    case RaceWaterRunestone:
        appearance.size = plasma::Vector<3, float>(3.0f, 3.0f, 4.0f) * 1.7f;
        appearance.headPosition[2] = -2.0f;
        appearance.headScale = 0.6f;
        appearance.hairColor = plasma::Vector<3, unsigned char>(60, 80, 255);
        appearance.headModel = SpriteWaterRunestone;
        appearance.hairModel = SpriteRunestoneGlow;
        appearance.appearanceFlags |= AppearanceFlagSuppressHairDebris;
        break;
    case RaceIceRunestone:
        appearance.size = plasma::Vector<3, float>(3.0f, 3.0f, 4.0f) * 1.7f;
        appearance.headPosition[2] = -2.0f;
        appearance.headScale = 0.6f;
        appearance.hairColor = plasma::Vector<3, unsigned char>(20, 200, 255);
        appearance.headModel = SpriteIceRunestone;
        appearance.hairModel = SpriteRunestoneGlow;
        appearance.appearanceFlags |= AppearanceFlagSuppressHairDebris;
        break;
    case RaceWindRunestone:
        appearance.size = plasma::Vector<3, float>(3.0f, 3.0f, 4.0f) * 1.7f;
        appearance.headPosition[2] = -2.0f;
        appearance.headScale = 0.6f;
        appearance.hairColor = plasma::Vector<3, unsigned char>(200, 200, 220);
        appearance.headModel = SpriteWindRunestone;
        appearance.hairModel = SpriteRunestoneGlow;
        appearance.appearanceFlags |= AppearanceFlagSuppressHairDebris;
        break;
    case RaceBirdStatue:
        appearance.size = plasma::Vector<3, float>(3.0f, 3.0f, 6.0f);
        appearance.chestModel = SpriteBirdStatue;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceMourningStatue:
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 4.5f);
        appearance.chestModel = SpriteMourningStatue;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestScale = 0.85f;
        break;
    case RaceEagle:
        appearance.size = plasma::Vector<3, float>(1.6f, 1.6f, 3.7f);
        appearance.chestModel = SpriteEagleSitting;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 3.8f, -5.0f);
        appearance.appearanceFlags |= AppearanceFlagBiteAttack | AppearanceFlagUnarmored |
                                      AppearanceFlagNoTerrainCollision | AppearanceFlagNoCreatureCollision;
        break;
    case RaceClimbingPlant:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.0f);
        appearance.chestModel = SpriteClimbingPlantSoil;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestScale = 2.0f;
        break;
    case RaceRunePortal:
        appearance.size = plasma::Vector<3, float>(20.0f, 4.0f, 16.0f);
        appearance.headScale = 0.17777778f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, -1.0f);
        appearance.headModel = SpriteRunePortal;
        appearance.hairModel = SpriteRunePortalFeature;
        appearance.appearanceFlags |= AppearanceFlagSuppressHairDebris;
        break;
    case RaceCactus:
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 4.0f);
        appearance.chestModel = SpriteCactus1;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceVase:
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 1.25f);
        appearance.chestModel = SpriteVase + rand() % 4;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceSign:
        appearance.size = plasma::Vector<3, float>(0.5f, 0.5f, 2.0f);
        appearance.chestModel = SpriteSign;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -6.0f);
        appearance.chestScale = 2.0f;
        break;
    case RaceSwimmingSign:
        appearance.size = plasma::Vector<3, float>(0.5f, 0.5f, 3.75f);
        appearance.chestModel = SpriteSwimmingSign;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 15.0f);
        appearance.chestScale = 2.0f;
        break;
    case RaceFirTree:
        appearance.chestModel = SpriteFirTree;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        break;
    case RaceThornTree:
        appearance.chestModel = SpriteThornTree;
        appearance.size = plasma::Vector<3, float>(3.0f, 3.0f, 12.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        break;
    case RaceGoldDeposit:
        appearance.chestModel = SpriteGoldDeposit;
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        break;
    case RaceIronDeposit:
        appearance.chestModel = SpriteIronDeposit;
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        break;
    case RaceSilverDeposit:
        appearance.chestModel = SpriteSilverDeposit;
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        break;
    case RaceSandstoneDeposit:
        appearance.chestModel = SpriteSandstoneDeposit;
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        break;
    case RaceEmeraldDeposit:
        appearance.chestModel = SpriteEmeraldDeposit;
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        break;
    case RaceSapphireDeposit:
        appearance.chestModel = SpriteSapphireDeposit;
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        break;
    case RaceRubyDeposit:
        appearance.chestModel = SpriteRubyDeposit;
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.appearanceFlags |= AppearanceFlagNoTerrainCollision;
        break;
    case RaceDiamondDeposit:
        appearance.chestModel = SpriteDiamondDeposit;
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        break;
    case RaceIceCrystalDeposit:
        appearance.chestModel = SpriteIceCrystalDeposit;
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        break;
    case RaceScarecrow:
        appearance.chestModel = SpriteScarecrow;
        appearance.size = plasma::Vector<3, float>(0.8f, 0.8f, 4.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestScale = 2.0f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceAim:
        appearance.chestModel = SpriteAim;
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceDemonPortal:
        appearance.chestModel = SpriteDemonPortal;
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 3.3f) * 1.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceSlimeWell:
        appearance.chestModel = SpriteSlimewell;
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 2.0f) * 1.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceHornetsNest:
        appearance.chestModel = SpriteHornetsNest;
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 2.0f) * 1.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceUnholyTomb:
        appearance.chestModel = SpriteUnholyTomb;
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 1.5f) * 1.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceBonePile:
        appearance.chestModel = SpriteBonePile;
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 2.0f) * 1.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceMolehill:
        appearance.chestModel = SpriteMolehill;
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 1.5f) * 1.25f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceEarthPile:
        appearance.chestModel = SpriteEarthPile;
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 1.5f) * 1.0f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceDummy:
        appearance.chestModel = SpriteDummy;
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 2.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceBloodaxeClanBanner:
        appearance.chestModel = SpriteBloodaxeClanBanner;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 4.4f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceStoneTablet:
        appearance.chestModel = SpriteStoneTablet1 + rand() % 6;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 2.7f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RacePedestal:
        appearance.chestModel = SpritePedestal;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.8f);
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceSocket:
        appearance.chestModel = SpriteSocket;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.8f);
        appearance.chestScale = 2.0f;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceMammoth:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.12f);
        appearance.headModel = SpriteMammothHead;
        appearance.chestModel = SpriteMammothBody;
        appearance.feetModel = SpriteMammothFoot;
        appearance.handsModel = SpriteMammothFoot;
        appearance.chestScale = 0.7f;
        appearance.headScale = 0.4f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, 2.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -4.0f, -3.0f);
        appearance.feetScale = 0.5f;
        appearance.handsScale = 0.5f;
        appearance.handsPosition = plasma::Vector<3, float>(3.0f, 3.0f, -4.1f);
        appearance.feetPosition = plasma::Vector<3, float>(2.9f, -3.0f, -4.1f);
        appearance.appearanceFlags |=
            AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored | AppearanceFlagLarge;
        break;
    case RaceElephant:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.12f) * ((float)rand() / (float)RAND_MAX + 6.0f);
        appearance.headModel = SpriteElephantHead;
        appearance.chestModel = SpriteElephantBody;
        appearance.feetModel = SpriteElephantFoot;
        appearance.handsModel = SpriteElephantFoot;
        appearance.chestScale = 0.7f;
        appearance.headScale = 0.4f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.5f, 0.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -4.0f, -3.0f);
        appearance.feetScale = 0.45f;
        appearance.handsScale = 0.5f;
        appearance.handsPosition = plasma::Vector<3, float>(2.5f, 2.0f, -4.0f);
        appearance.feetPosition = plasma::Vector<3, float>(2.5f, -3.0f, -4.0f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceBabyElephant:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.12f) * 2.5f;
        appearance.headModel = SpriteElephantBabyHead;
        appearance.chestModel = SpriteElephantBabyBody;
        appearance.feetModel = SpriteElephantBabyFoot;
        appearance.handsModel = SpriteElephantBabyFoot;
        appearance.chestScale = 0.7f;
        appearance.headScale = 0.5f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 5.0f, 1.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -4.0f, -3.0f);
        appearance.feetScale = 0.45f;
        appearance.handsScale = 0.5f;
        appearance.handsPosition = plasma::Vector<3, float>(2.5f, 3.0f, -4.0f);
        appearance.feetPosition = plasma::Vector<3, float>(2.5f, -3.0f, -4.0f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceBabyMammoth:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.12f) * 2.5f;
        appearance.headModel = SpriteMammothBabyHead;
        appearance.chestModel = SpriteMammothBabyBody;
        appearance.feetModel = SpriteMammothBabyFoot;
        appearance.handsModel = SpriteMammothBabyFoot;
        appearance.chestScale = 0.7f;
        appearance.headScale = 0.5f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 5.0f, 1.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -4.0f, -3.0f);
        appearance.feetScale = 0.45f;
        appearance.handsScale = 0.5f;
        appearance.handsPosition = plasma::Vector<3, float>(2.5f, 3.0f, -4.0f);
        appearance.feetPosition = plasma::Vector<3, float>(2.5f, -3.0f, -4.0f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        if (allowMarked) {
            appearance.appearanceFlags |= AppearanceFlagMarked;
        }
        break;
    case RaceRhino:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.12f) * ((float)rand() * 0.5f / (float)RAND_MAX + 4.0f);
        appearance.headModel = SpriteRhinoHead;
        appearance.chestModel = SpriteRhinoBody;
        appearance.feetModel = SpriteRhinoFoot;
        appearance.handsModel = SpriteRhinoFoot;
        appearance.chestScale = 0.7f;
        appearance.headScale = 0.4f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 9.0f, 0.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -4.0f, -3.0f);
        appearance.feetScale = 0.45f;
        appearance.handsScale = 0.5f;
        appearance.handsPosition = plasma::Vector<3, float>(2.5f, 3.0f, -4.0f);
        appearance.feetPosition = plasma::Vector<3, float>(2.5f, -3.0f, -4.0f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored;
        break;
    case RaceSaurian:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.12f) * 1.3f;
        appearance.headModel = SpriteMonsterHeadReptileLizard + rand() % 5;
        appearance.chestModel = SpriteMonsterBodyReptileShell + rand() % 9;
        appearance.handsModel = SpriteMonsterFootReptileNormal + rand() % 3;
        appearance.feetModel = appearance.handsModel;
        appearance.tailModel = SpriteMonsterTailReptileNormal + rand() % 2;
        appearance.tailScale = 0.7f;
        appearance.chestScale = 0.7f;
        appearance.headScale = (float)rand() * 0.1f / (float)RAND_MAX + 0.4f;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 8.0f, (float)rand() * 0.5f / (float)RAND_MAX - 1.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
        appearance.tailPosition = plasma::Vector<3, float>(0.0f, -4.0f, -3.0f);
        appearance.feetScale = 0.5f;
        appearance.handsScale = 0.5f;
        appearance.handsPosition = plasma::Vector<3, float>((float)rand() / (float)RAND_MAX + 3.0f, 3.0f, -4.1f);
        appearance.feetPosition = plasma::Vector<3, float>((float)rand() / (float)RAND_MAX + 3.0f, -3.0f, -4.1f);
        appearance.appearanceFlags |=
            AppearanceFlagQuadruped | AppearanceFlagBiteAttack | AppearanceFlagUnarmored | AppearanceFlagLarge;
        break;
    case RaceWoodChest:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 1.5f);
        appearance.headModel = SpriteChestTop02;
        appearance.headScale = 0.5f;
        appearance.chestModel = SpriteChestBase02;
        appearance.chestScale = 0.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -1.75f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.75f);
        break;
    case RaceObsidianChest:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 1.5f);
        appearance.headModel = SpriteChestTopObsidian;
        appearance.headScale = 0.5f;
        appearance.chestModel = SpriteChestBaseObsidian;
        appearance.chestScale = 0.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -1.75f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.75f);
        break;
    case RaceBoneChest:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 1.5f);
        appearance.headModel = SpriteChestTopBone;
        appearance.headScale = 0.5f;
        appearance.chestModel = SpriteChestBaseBone;
        appearance.chestScale = 0.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -1.75f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.75f);
        break;
    case RaceSkullChest:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 1.5f);
        appearance.headModel = SpriteChestTopUndead1;
        appearance.headScale = 0.5f;
        appearance.chestModel = SpriteChestBaseUndead1;
        appearance.chestScale = 0.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -1.75f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 1.75f);
        break;
    case RaceSarcophagus:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.6f);
        appearance.headModel = SpriteSarcophagusTop;
        appearance.headScale = 0.75f;
        appearance.chestModel = SpriteSarcophagus;
        appearance.chestScale = 0.75f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -2.625f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 4.875f);
        break;
    case RaceCage1:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.1f) * 2.5f;
        appearance.headModel = SpriteCageDoor;
        appearance.chestModel = SpriteCageBase;
        appearance.headScale = 0.503f;
        appearance.chestScale = 0.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceCage2:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.1f) * 2.5f;
        appearance.headModel = SpriteCage2Door;
        appearance.chestModel = SpriteCage2Base;
        appearance.headScale = 0.503f;
        appearance.chestScale = 0.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceCage3:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.1f) * 2.5f;
        appearance.headModel = SpriteCage3Door;
        appearance.chestModel = SpriteCage3Base;
        appearance.headScale = 0.503f;
        appearance.chestScale = 0.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceCage4:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.1f) * 2.5f;
        appearance.headModel = SpriteCage4Door;
        appearance.chestModel = SpriteCage4Base;
        appearance.headScale = 0.503f;
        appearance.chestScale = 0.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceTortureRack:
        appearance.size = plasma::Vector<3, float>(2.1f, 2.1f, 1.4f);
        appearance.chestScale = 0.6f;
        appearance.chestModel = SpriteTortureRack;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceLegIrons:
        appearance.size = plasma::Vector<3, float>(1.8f, 1.8f, 1.6f);
        appearance.chestScale = 0.6f;
        appearance.chestModel = SpriteLegIrons;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagFullShininess;
        break;
    case RacePole:
        appearance.size = plasma::Vector<3, float>(1.8f, 1.8f, 1.8f);
        appearance.chestScale = 0.6f;
        appearance.chestModel = SpritePole;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceCampfire:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 1.0f);
        appearance.chestModel = SpriteCampfire;
        appearance.chestScale = 0.45f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceCauldron:
        appearance.size = plasma::Vector<3, float>(3.0f, 3.0f, 3.0f);
        appearance.chestModel = SpriteCauldron;
        appearance.chestScale = 0.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceLever:
        appearance.size = plasma::Vector<3, float>(3.0f, 3.0f, 3.0f);
        appearance.headModel = SpriteLeverSlot3550;
        appearance.chestModel = SpriteLeverBase;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -3.1f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, -0.6f);
        appearance.appearanceFlags |= AppearanceFlagUsable;
        break;
    case RaceLockedLever:
        appearance.size = plasma::Vector<3, float>(3.0f, 3.0f, 3.0f);
        appearance.headModel = SpriteLeverSlot3550;
        appearance.chestModel = SpriteLockedLeverBase;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -3.1f);
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, -0.6f);
        appearance.appearanceFlags |= AppearanceFlagUsable;
        break;
    case RaceBrazier:
    case RaceGhostBrazier:
    case RaceUnholyBonfire:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 1.15f);
        appearance.chestModel = SpriteBrazier;
        appearance.chestScale = 0.75f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceWagon:
        appearance.size = plasma::Vector<3, float>(5.0f, 5.0f, 5.0f);
        appearance.chestModel = SpriteWagon1 + rand() % 2;
        appearance.chestScale = 0.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceWitchWagon:
        appearance.size = plasma::Vector<3, float>(5.0f, 5.0f, 3.0f);
        appearance.chestModel = SpriteWitchWagon;
        appearance.chestScale = 0.2f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceSack:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestModel = SpriteSack;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceCrate:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestScale = 1.1f;
        appearance.chestModel = SpriteCrate;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceGate:
        appearance.size = plasma::Vector<3, float>(10.0f, 10.0f, 10.0f);
        appearance.chestScale = 0.56f;
        appearance.chestModel = SpriteGate;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagFullShininess;
        break;
    case RaceLockedGate:
        appearance.size = plasma::Vector<3, float>(10.0f, 10.0f, 10.0f);
        appearance.chestScale = 0.4148148f;
        appearance.chestModel = SpriteLockedGate;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagFullShininess;
        break;
    case RaceRuneGate:
        appearance.size = plasma::Vector<3, float>(10.0f, 10.0f, 10.0f);
        appearance.headScale = 0.4148148f;
        appearance.headModel = SpriteRuneGate;
        appearance.hairModel = SpriteRuneGateGlow;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagSuppressHairDebris;
        break;
    case RaceStoneGate:
        appearance.size = plasma::Vector<3, float>(10.0f, 10.0f, 10.0f);
        appearance.chestScale = 0.4148148f;
        appearance.chestModel = SpriteStoneGate;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceSpiderweb:
        appearance.size = plasma::Vector<3, float>(10.0f, 10.0f, 10.0f);
        appearance.chestScale = 0.4148148f;
        appearance.chestModel = SpriteSpiderwebWall;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceMagicBarrier:
        appearance.size = plasma::Vector<3, float>(10.0f, 10.0f, 10.0f);
        appearance.chestScale = 0.4148148f;
        appearance.chestModel = SpriteMagicBarrier;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagSpiritForm;
        break;
    case RaceWall:
        appearance.size = plasma::Vector<3, float>(10.0f, 10.0f, 10.0f);
        appearance.chestScale = 0.56f;
        appearance.chestModel = SpriteObstacleWall;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceGrid:
        appearance.size = plasma::Vector<3, float>(10.0f, 10.0f, 10.0f);
        appearance.chestScale = 0.4148148f;
        appearance.chestModel = SpriteGrid;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagFullShininess;
        break;
    case RaceRuneCube:
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 1.5f);
        appearance.headScale = 1.8f;
        appearance.headModel = SpriteRuneCube;
        appearance.hairModel = SpriteRuneCubeGlow;
        appearance.headPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagSuppressHairDebris;
        break;
    case RaceTreasure:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestModel = SpriteTreasure1 + rand() % 5;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceTreasureStatue:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestModel = SpriteTreasureStatue1 + rand() % 5;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceTreasureJewel:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestModel = SpriteTreasureJewel1 + rand() % 1;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceTreasureVase:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestModel = SpriteTreasureVase1 + rand() % 2;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceShrineOfTheLight:
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 5.4f);
        appearance.chestModel = SpriteShrineOfTheLight;
        appearance.chestScale = 0.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RacePirateBoat:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 1.0f);
        appearance.chestModel = SpritePirateBoat;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 5.0f);
        appearance.appearanceFlags |= AppearanceFlagQuadruped;
        break;
    case RaceBarrel:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.05f);
        appearance.chestModel = SpriteBarrel;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestScale = 0.8f;
        break;
    case RaceBarrelTnt:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.05f);
        appearance.chestModel = SpriteBarrelTnt;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestScale = 0.8f;
        break;
    case RaceTent:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 2.0f);
        appearance.chestModel = SpriteTent;
        appearance.chestScale = 1.0f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceSleepingMat:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 0.6f);
        appearance.chestModel = SpriteWoodMat;
        appearance.chestScale = 0.75f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.appearanceFlags |= AppearanceFlagNoCreatureCollision;
        break;
    case RaceBeachTowel:
        appearance.size = plasma::Vector<3, float>(2.0f, 2.0f, 0.6f);
        appearance.chestModel = SpriteBeachTowel;
        appearance.chestScale = 0.75f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -1.4f);
        appearance.appearanceFlags |= AppearanceFlagNoCreatureCollision;
        break;
    case RaceSunshade:
        appearance.size = plasma::Vector<3, float>(3.0f, 3.0f, 3.0f);
        appearance.chestModel = SpriteBeachUmbrella;
        appearance.chestScale = 0.5f;
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        break;
    case RaceTombstone:
        appearance.size = plasma::Vector<3, float>(1.0f, 1.0f, 1.3f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestModel = SpriteTombstone;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceManaPump:
        appearance.size = plasma::Vector<3, float>(4.0f, 4.0f, 5.0f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -4.0f);
        appearance.chestModel = SpriteManaPump;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceManaInductor:
        appearance.size = plasma::Vector<3, float>(4.0f, 4.0f, 3.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestModel = SpriteManaInductor;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceManaGenerator:
        appearance.size = plasma::Vector<3, float>(4.0f, 4.0f, 3.8f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestModel = SpriteManaGenerator;
        appearance.appearanceFlags |= AppearanceFlagUnarmored;
        break;
    case RaceCeilingChain:
        appearance.size = plasma::Vector<3, float>(1.5f, 1.5f, 4.5f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        appearance.chestModel = SpriteCeilingChain;
        appearance.appearanceFlags |= AppearanceFlagUnarmored | AppearanceFlagNoTerrainCollision;
        break;
    case RaceFootprints:
        appearance.size = plasma::Vector<3, float>(0.5f, 0.5f, 0.2f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -1.8f);
        appearance.chestModel = SpriteFootprints;
        appearance.appearanceFlags |= AppearanceFlagNoCreatureCollision;
        break;
    case RaceHorseFootprints:
        appearance.size = plasma::Vector<3, float>(0.5f, 0.5f, 0.2f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -1.8f);
        appearance.chestModel = SpriteHorseFootprints;
        appearance.appearanceFlags |= AppearanceFlagNoCreatureCollision;
        break;
    case RaceMonsterFootprints:
        appearance.size = plasma::Vector<3, float>(0.5f, 0.5f, 0.2f);
        appearance.chestPosition = plasma::Vector<3, float>(0.0f, 0.0f, -1.8f);
        appearance.chestModel = SpriteMonsterFootprints;
        appearance.appearanceFlags |= AppearanceFlagNoCreatureCollision;
        break;
    case RaceCreatorMale:
        // converts to the colour (1, 1, 1), nearly black
        appearance.hairColor = plasma::Vector<3, float>(1.0f, 1.0f, 1.0f);
        appearance.headModel = SpriteHead2;
        appearance.chestModel = SpriteBody4Slot1;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        break;
    case RaceCreatorFemale:
        // converts to the colour (1, 1, 1), nearly black
        appearance.hairColor = plasma::Vector<3, float>(1.0f, 1.0f, 1.0f);
        appearance.headModel = SpriteElfHeadFemale;
        appearance.chestModel = SpriteBody3Slot2;
        appearance.feetModel = SpriteFootSlot615;
        appearance.handsModel = SpriteHand2;
        break;
    }

    // Three flags enlarge the creature, up to a width of eight blocks.
    if (appearance.appearanceFlags & AppearanceFlagEnhanced) {
        appearance.size *= 1.5f;
        if (appearance.size[0] > 8.0f) {
            appearance.size *= 8.0f / appearance.size[0];
        }
    }
    if (appearance.appearanceFlags & AppearanceFlagTinted) {
        appearance.size *= 1.5f;
        if (appearance.size[0] > 8.0f) {
            appearance.size *= 8.0f / appearance.size[0];
        }
    }
    if (appearance.appearanceFlags & AppearanceFlagLarge) {
        appearance.size *= 7.0f;
        if (appearance.size[0] > 8.0f) {
            appearance.size *= 8.0f / appearance.size[0];
        }
    }
}

Creature::Buff *Creature::findBuff(int type)
{
    for (std::list<Buff>::iterator it = m_buffs.begin(); it != m_buffs.end(); ++it) {
        if (it->type == type) {
            return &*it;
        }
    }
    return nullptr;
}

float Creature::hangGlidingSpeed()
{
    if (m_entityData.hostilityType != HostilityPlayer) {
        return (1.0f - 1.0f / (m_entityData.level * 10 / 20.0f + 1.0f)) * 1.9f + 1.1f;
    }
    return (1.0f - 1.0f / (m_artifactSkills[ArtifactEffect_HangGlidingSpeed] / 20.0f + 1.0f)) * 1.9f + 1.1f;
}

bool Creature::hasCompletedMission(int x, int y)
{
    PlayerData *data = m_playerData;
    if (!data) {
        return false;
    }
    std::array<int, 2> place = {x, y};
    std::array<int, 2> finishedPlace;
    return data->solvedMissions.find(place) != data->solvedMissions.end() ||
           (finishedPlace[0] = x, finishedPlace[1] = y,
            data->finishedMissions.find(finishedPlace) != data->finishedMissions.end());
}

bool Creature::hasFinishedMission(int x, int y)
{
    PlayerData *data = m_playerData;
    if (!data) {
        return false;
    }
    std::array<int, 2> place = {x, y};
    return data->finishedMissions.find(place) != data->finishedMissions.end();
}

bool Creature::hasItem(const Item &item, int count)
{
    int total = 0;
    for (std::vector<ItemStack> &tab : m_inventory.m_inventoryTabs) {
        for (ItemStack &stack : tab) {
            if (stack.item == item) {
                total += stack.quantity;
            }
        }
    }
    return total >= count;
}

bool Creature::hasRandomChance(float chance)
{
    if (m_entityData.equipment.weaponRight.category == ItemCategoryWeapon) {
        switch (m_entityData.equipment.weaponRight.id) {
        case WeaponSword:
        case WeaponAxe:
        case WeaponMace:
            chance *= 0.5f;
            break;
        case WeaponBoomerang:
            chance *= 0.3f;
            break;
        case WeaponStaff:
            chance *= 0.3f;
            break;
        case WeaponBracelet:
            chance *= 0.5f;
            break;
        }
    }
    return chance > rand() / (float)RAND_MAX;
}

bool Creature::hasSpecialItem(int id)
{
    if (id == SpecialItemFlute) {
        return true;
    }
    Item item;
    item.category = ItemCategorySpecial;
    item.id = id;
    item.region = m_entityData.region;
    return hasItem(item, 1);
}

bool Creature::hasTwoHandedWeapon()
{
    return m_entityData.equipment.weaponRight.isTwoHanded() || m_entityData.equipment.weaponLeft.isTwoHanded();
}

int Creature::holdTime(int skill)
{
    if (skill < 0) {
        skill = m_entityData.currentAbility;
    }
    switch (skill) {
    case AbilityWatchPlayer:
    case AbilityBlock:
    case AbilityChargedShot:
    case AbilitySalvo:
    case AbilityChargedBoomerang:
    case AbilitySupportRay:
    case AbilityIntercept:
    case AbilityCrush:
    case AbilitySwirl:
    case AbilitySneak:
    case AbilitySprint:
    case AbilityDash:
    case AbilityCyclone:
    case AbilityRidePet:
    case AbilityHangGlide:
    case AbilitySailBoat:
    case AbilityClimb:
    case AbilityUseSpecialItem:
    case AbilityFlameRush:
    case AbilityFeedClimbingPlant:
    case AbilityCallPet:
    case AbilityWarriorJumpAttack:
    case AbilityWave:
    case AbilityPetAnimal:
    case AbilityDance:
        return 0;
    case AbilityNone:
    case AbilityRetreat:
        return 100;
    case AbilityShuriken:
        if (findBuff(BuffNinjutsu)) {
            return 50;
        }
        return 100;
    case Ability_4:
    case Ability_5:
    case AbilityPerforate:
    case AbilityTwoHandedSweep:
    case AbilityTwoHandedThrust:
    case Ability_63:
        return (int)(300.0f / tempo());
    case AbilitySwirlStrike:
    case AbilityShoot:
    case AbilityThrowBoomerang:
    case AbilityFireSwirl:
    case AbilityFireVortex:
    case AbilityWaterSwirl:
    case AbilityWaterVortex:
    case AbilityHeal:
    case AbilityFireball:
    case AbilityMonsterFirebolt:
    case AbilityFirebolt:
    case AbilityBraceletFirebolt:
    case AbilityWaterbolt:
    case AbilityBraceletWaterbolt:
    case AbilityWaterball:
    case AbilityMonsterWaterbolt:
    case AbilityWaterSalvo:
    case Ability_48:
    case AbilityCrushStrike:
    case AbilityGuardianCrushStrike:
    case AbilityTwoHandedSwing:
    case AbilityFireBeam:
    case AbilityFireRay:
    case AbilityWaterRay:
    case AbilityWaterBeam:
    case AbilityBlockStrike:
    case AbilityHealingStream:
    case AbilityCenteredHealingStream:
    case AbilityFireBreath:
    case AbilityWaterBubbles:
    case AbilityBloomlingAttack:
        return (int)(100.0f / tempo());
    case AbilityLeftStab:
    case AbilityRightSweep:
    case AbilityLeftDaggerPierce:
    case AbilityTwoHandedSpin:
    case AbilityTwoHandedSlam:
        return (int)(200.0f / tempo());
    case AbilityHeadbutt:
    case AbilityHornCharge:
    case AbilityLunge:
        return (int)(800.0f / tempo());
    case AbilityBlockJab:
        return 600;
    case AbilityOverheadChop:
        return (int)(400.0f / tempo());
    case AbilitySmash:
        return 400;
    case AbilityShellBlock:
    case AbilityShellSpin:
        return 200;
    case AbilityChargedShotRelease:
        return (int)(10.0f / tempo());
    case AbilityWraithAttack:
        return 10;
    default:
        return (int)(500.0f / tempo());
    }
}

float Creature::interactionRange()
{
    if (m_entityData.currentAbility == AbilityCeilingChainPose) {
        return m_entityData.appearance.size[0] * 0.5f + 40.0f;
    }
    return (std::max)(m_entityData.appearance.size[2],
                      (std::max)(m_entityData.appearance.size[0], m_entityData.appearance.size[1])) *
               0.5f +
           5.0f;
}

bool Creature::isAttackingFromRange()
{
    if (isCaster()) {
        return true;
    }
    if (m_entityData.classType == ClassMonsterGunner || m_entityData.classType == ClassMonsterArcher) {
        return true;
    }
    // Weapon ids 6, 7, 8, 10 and 11 in either hand.
    if (m_entityData.equipment.weaponRight.category == ItemCategoryWeapon &&
        m_entityData.equipment.weaponRight.id != WeaponArrow &&
        m_entityData.equipment.weaponRight.id > WeaponLongsword &&
        m_entityData.equipment.weaponRight.id < WeaponBracelet) {
        return true;
    }
    if (m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon &&
        m_entityData.equipment.weaponLeft.id != WeaponArrow && m_entityData.equipment.weaponLeft.id > WeaponLongsword &&
        m_entityData.equipment.weaponLeft.id < WeaponBracelet) {
        return true;
    }
    return false;
}

bool Creature::isBlocking()
{
    if (m_entityData.actionLockTime > 0.0f) {
        return false;
    }
    // SkillBlock, SkillCyclone while it lasts, skill 110, and SkillCrush or SkillSwirl of a class 1 creature with
    // specialization 1.
    if (!(m_entityData.currentAbility == AbilityBlock ||
          (m_entityData.currentAbility == AbilityCyclone && attackDuration() > m_entityData.timeSinceAbility) ||
          m_entityData.currentAbility == AbilityParry ||
          ((m_entityData.currentAbility == AbilityCrush || m_entityData.currentAbility == AbilitySwirl) &&
           m_entityData.classType == ClassWarrior && m_entityData.specialization == SpecializationGuardian))) {
        return false;
    }
    return true;
}

bool Creature::isBoosted()
{
    return (m_entityData.appearance.appearanceFlags & AppearanceFlagTinted) != 0;
}

bool Creature::isCage()
{
    return m_entityData.hostilityType == HostilityInteractiveObject && m_entityData.race >= RaceCage1 &&
           m_entityData.race <= RaceCage4;
}

bool Creature::isCaster()
{
    if (m_entityData.classType == ClassMage || m_entityData.classType == ClassMonsterBeamCaster ||
        m_entityData.classType == ClassMonsterCaster) {
        return true;
    }
    if (m_entityData.race == RaceLich || m_entityData.race == RaceSpitter) {
        return true;
    }
    if (m_entityData.equipment.weaponRight.category == ItemCategoryWeapon &&
        m_entityData.equipment.weaponRight.id >= WeaponStaff &&
        m_entityData.equipment.weaponRight.id <= WeaponBracelet) {
        return true;
    }
    return false;
}

bool Creature::isChest()
{
    return m_entityData.race >= RaceWoodChest && m_entityData.race <= RaceBoneChest;
}

bool isFemale(unsigned int race)
{
    return race == RaceElfFemale || race == RaceHumanFemale || race == RaceGoblinFemale ||
           race == RaceLizardmanFemale || race == RaceDwarfFemale || race == RaceOrcFemale ||
           race == RaceFrogmanFemale || race == RaceUndeadFemale || race == RaceNomadFemale ||
           race == RaceDarkCultistFemale;
}

bool Creature::isGate()
{
    return m_entityData.hostilityType == HostilityInteractiveObject && m_entityData.race >= RaceGate &&
           m_entityData.race <= RaceStoneGate;
}

bool Creature::isGliding()
{
    return m_entityData.currentAbility == AbilityHangGlide && !(m_entityData.flags & CreatureFlagOnGround) &&
           m_entityData.stunTime <= 0.0f && m_entityData.rollTime <= 0.0f;
}

int isHarmfulSkill(int skill)
{
    return !(skill == AbilityCamouflage || skill == AbilityAim || skill == AbilitySwiftness ||
             skill == AbilityBulwark || skill == AbilityWarFrenzy || skill == AbilityManaShield ||
             skill == AbilityHealingStream || skill == AbilityCenteredHealingStream || skill == AbilityNinjutsu ||
             skill == AbilityQuicksandTrap || skill == AbilityShadowClone || skill == AbilityWaterBubbles ||
             skill == AbilityFlameRush || skill == AbilityHeroicShout || skill == AbilitySneak ||
             skill == AbilityTeleport || skill == AbilityRetreat);
}

int isHealingSkill(int skill)
{
    return skill == AbilityHealingStream || skill == AbilityCenteredHealingStream;
}

bool Creature::isLarge()
{
    return (m_entityData.appearance.appearanceFlags & AppearanceFlagLarge) != 0;
}

bool Creature::isLever()
{
    return m_entityData.race == RaceLever || m_entityData.race == RaceLockedLever;
}

bool Creature::isMarked()
{
    return (m_entityData.appearance.appearanceFlags & AppearanceFlagMarked) != 0;
}

bool Creature::isObstacle()
{
    return (m_entityData.appearance.appearanceFlags & AppearanceFlagObstacle) &&
           !(isGate() && m_entityData.interactionState) && m_entityData.hp > 0.0f;
}

bool Creature::isPassive()
{
    return !m_entityData.binaryToggles.active && ((m_entityData.appearance.appearanceFlags & AppearanceFlagPassive) ||
                                                  m_entityData.currentAbility == AbilityRestHeal);
}

bool isPet(int race)
{
    switch (race) {
    case RaceCollie:
    case RaceShepherdDog:
    case RaceAlpaca:
    case RaceBrownAlpaca:
    case RaceTurtle:
    case RaceTerrier:
    case RaceScottishTerrier:
    case RaceWolf:
    case RacePanther:
    case RaceCat:
    case RaceBrownCat:
    case RaceWhiteCat:
    case RacePig:
    case RaceSheep:
    case RaceBunny:
    case RacePorcupine:
    case RaceGreenSlime:
    case RacePinkSlime:
    case RaceYellowSlime:
    case RaceBlueSlime:
    case RacePlainRunner:
    case RaceLeafRunner:
    case RaceSnowRunner:
    case RaceDesertRunner:
    case RacePeacock:
    case RaceDuckbill:
    case RaceCrocodile:
    case RaceHorse:
    case RaceCamel:
    case RaceCow:
    case RaceBarkBeetle:
    case RaceFireBeetle:
    case RaceSnoutBeetle:
    case RaceLemonBeetle:
    case RaceBumblebee:
    case RaceZebra:
    case RaceLionMale:
    case RaceLionFemale:
    case RaceSkeletonHorse:
    case RaceSkeletonDog:
    case RaceBabyElephant:
    case RaceBabyMammoth:
    case RaceGoldKnightHorse:
    case RaceFlamingo:
    case RaceToucan:
    case RaceSilverLion:
    case RaceBlueTurtle:
    case RaceManaMech:
    case RaceTreant:
    case RaceSkullSlater:
    case RaceCaterpillar:
    case RaceEarthCaterpillar:
    case RaceSnail:
        return true;
    }
    return false;
}

bool Creature::isWraithTargeting()
{
    return m_entityData.binaryToggles.wraithTargeting;
}

float Creature::knockback(int skill)
{
    if (m_entityData.hostilityType == HostilityPet || m_entityData.hostilityType == HostilityFriendlyNpc) {
        return 0.0f;
    }
    switch (skill) {
    case Ability_4:
    case Ability_5:
    case AbilityRightStab:
    case AbilityLeftStab:
    case AbilityLeftDaggerPierce:
    case AbilityRightDaggerPierce:
    case Ability_63:
        return 3.0f / tempo();
    case AbilityRightSlash:
    case AbilityLeftSlash:
    case AbilityShieldAttack:
    case AbilityLeftSweep:
    case AbilityRightSweep:
    case AbilityOverheadChop:
    case AbilityCrushStrike:
    case AbilityGuardianCrushStrike:
        return 6.0f / tempo();
    case AbilitySwirlStrike:
    case AbilityDjinnExplosion:
        return 10.0f;
    case AbilitySmash:
        return 5.0f;
    case AbilityAmbush:
    case AbilityKick:
        return 6.0f;
    case AbilityRangerKick:
    case AbilityFireExplosion:
        return 30.0f;
    case AbilityPerforate:
        return 4.0f;
    case AbilityTwoHandedSpin:
    case AbilityTwoHandedSlam:
        return 5.0f / tempo();
    case AbilityFireSwirl:
    case AbilityFireVortex:
    case AbilityWaterSwirl:
    case AbilityWaterVortex:
        return 0.0f;
    default:
        return 2.0f / tempo();
    }
}

bool Creature::knowsFlightPoint(const plasma::Vector<2, int> &flightPoint)
{
    PlayerData *data = m_playerData;
    if (!data) {
        return false;
    }
    std::array<int, 2> place = {flightPoint[0], flightPoint[1]};
    return data->knownFlightPoints.find(place) != data->knownFlightPoints.end();
}

bool Creature::knowsFormula(const Item &item)
{
    PlayerData *data = m_playerData;
    if (!data) {
        return false;
    }
    for (Item &formula : data->knownFormulas) {
        if (item == formula) {
            return true;
        }
    }
    return false;
}

bool Creature::knowsMission(int x, int y)
{
    PlayerData *data = m_playerData;
    if (!data) {
        return false;
    }
    std::array<int, 2> place = {x, y};
    std::array<int, 2> solvedPlace;
    return data->knownMissions.find(place) != data->knownMissions.end() ||
           (solvedPlace[0] = x, solvedPlace[1] = y,
            data->solvedMissions.find(solvedPlace) != data->solvedMissions.end());
}

float Creature::lampDiameter()
{
    if (m_entityData.hostilityType != HostilityPlayer) {
        return (1.0f - 1.0f / (m_entityData.level * 10 / 20.0f + 1.0f)) * 12.0f + 8.0f;
    }
    return (1.0f - 1.0f / (m_artifactSkills[ArtifactEffect_LampRadius] / 20.0f + 1.0f)) * 12.0f + 8.0f;
}

float Creature::magicDamageReduction()
{
    float reduction = resistance() / powf(2.0f, 4.0f) * 0.05f;
    if (findBuff(BuffDefenseWeakened)) {
        reduction *= 0.25f;
    }
    return reduction;
}

float Creature::manaCost(int skill, int unusedArgument)
{
    float tuned = 0.0f;
    if (mod::manaCost(this, skill, tuned)) {
        return tuned;
    }
    switch (skill) {
    case AbilityFireVortex:
    case AbilityWaterVortex:
    case AbilityFireball:
    case AbilityWaterball:
    case AbilityWaterSalvo:
    case Ability_48:
    case AbilityFireRay:
    case AbilityWaterRay:
        if (!findBuff(BuffInstantfire)) {
            return 0.3f;
        }
        break;
    case AbilityHeal:
        return 0.0625f;
    case AbilityHealingStream:
    case AbilityCenteredHealingStream:
    case AbilityFireBreath:
        return 0.03125f;
    case AbilityTeleport:
        return 1.0f;
    case AbilityFireExplosion:
        return 0.5f;
    }
    return 0.0f;
}

float Creature::manaGeneration()
{
    Buff *buff = findBuff(BuffManaabsorption);
    if (buff) {
        return 1.0f - buff->strength * 0.1f;
    }
    return 1.0f;
}

long long Creature::maxAggroTarget()
{
    Buff *buff = findBuff(BuffForcedAggroTarget);
    if (buff) {
        return buff->sourceId;
    }
    long long target = 0;
    float maximum = 0.0f;
    for (std::map<long long, float>::iterator it = m_aggro.begin(); it != m_aggro.end(); ++it) {
        if (it->first != 0 && it->second >= maximum) {
            maximum = it->second;
            target = it->first;
        }
    }
    return target;
}

int maxExperience(int level)
{
    return (int)(pow(2.0, (level - 1) * 0.5f) * 10.0);
}

float Creature::maxHP()
{
    float hp = 100.0f;
    if (m_entityData.classType == ClassWarrior && m_entityData.specialization == SpecializationGuardian) {
        hp = 125.0f;
    }
    Equipment &equipment = m_entityData.equipment;
    if (equipment.weaponLeft.category == ItemCategoryWeapon) {
        hp += equipment.weaponLeft.hp(this);
    }
    if (equipment.weaponRight.category == ItemCategoryWeapon) {
        hp += equipment.weaponRight.hp(this);
    }
    if (equipment.chest.category == ItemCategoryChest) {
        hp += equipment.chest.hp(this);
    }
    if (equipment.feet.category == ItemCategoryFeet) {
        hp += equipment.feet.hp(this);
    }
    if (equipment.hands.category == ItemCategoryHands) {
        hp += equipment.hands.hp(this);
    }
    if (equipment.shoulder.category == ItemCategoryShoulder) {
        hp += equipment.shoulder.hp(this);
    }
    if (m_entityData.appearance.hasFlag(AppearanceFlagTinted)) {
        hp *= 10.0f;
    }
    if (m_entityData.appearance.hasFlag(AppearanceFlagEnhanced)) {
        hp *= 1.5f;
    }
    if (m_entityData.appearance.hasFlag(AppearanceFlagLarge)) {
        hp *= 15.0f;
    }
    if (Buff *buff = findBuff(BuffLifeflask)) {
        hp *= buff->strength;
    }
    for (Buff &buff : m_buffs) {
        if (buff.type == BuffMaxHpMultiplier) {
            hp *= buff.strength;
        }
    }
    if (m_entityData.appearance.appearanceFlags & AppearanceFlagPromotedResident) {
        hp *= 1.1f;
    }
    return hp;
}

int Creature::maxStunTime()
{
    return m_entityData.hostilityType == HostilityPlayer ? 1500 : 3000;
}

// The second skill in each pair continues the combo when the first skill is current and has not exceeded its duration
// by more than 200 ms.
int Creature::nextAttackSkill(bool alternative)
{
    if (alternative || findBuff(BuffNinjutsu)) {
        switch (m_entityData.classType) {
        case ClassMage:
            if (m_entityData.specialization == SpecializationWater) {
                return AbilityCenteredHealingStream;
            }
            return AbilityFireExplosion;
        case ClassRogue:
            if (m_entityData.specialization == SpecializationNinja) {
                return AbilityShuriken;
            }
            break;
        }
    }
    if (m_entityData.race == RaceBloomling) {
        return AbilityBloomlingAttack;
    }
    switch (m_entityData.classType) {
    case ClassMonsterGunner:
    case ClassMonsterArcher:
        return AbilityShoot;
    case ClassMonsterBeamCaster:
        return AbilityFireBeam;
    case ClassMonsterCaster:
        if (m_entityData.specialization == SpecializationWater) {
            return AbilityMonsterWaterbolt;
        }
        return AbilityMonsterFirebolt;
    case Class_145:
    case ClassMonster:
        if (m_entityData.currentAbility == AbilityRightStab && attackDuration() + 200 > m_entityData.timeSinceAbility) {
            return AbilityLeftStab;
        }
        return AbilityRightStab;
    case Class_147:
        if (m_entityData.currentAbility == AbilityRightSlash &&
            attackDuration() + 200 > m_entityData.timeSinceAbility) {
            return AbilityLeftSlash;
        }
        return AbilityRightSlash;
    case Class_148:
        if (m_entityData.currentAbility == AbilityTwoHandedSpin &&
            attackDuration() + 200 > m_entityData.timeSinceAbility) {
            return AbilityTwoHandedSlam;
        }
        return AbilityTwoHandedSpin;
    }
    Item &left = m_entityData.equipment.weaponLeft;
    Item &right = m_entityData.equipment.weaponRight;
    if (right.id == WeaponBiteAttack) {
        return AbilityClawStrike;
    }
    if (right.id == WeaponHandAttacks) {
        if (m_entityData.currentAbility == AbilityRightSlash &&
            attackDuration() + 200 > m_entityData.timeSinceAbility) {
            return AbilityLeftSlash;
        }
        return AbilityRightSlash;
    }
    if (isCaster()) {
        if (right.id == WeaponWand) {
            if (m_entityData.specialization == SpecializationWater) {
                return AbilityWaterBeam;
            }
            return AbilityFireBeam;
        }
        if (right.id == WeaponStaff) {
            if (m_entityData.specialization == SpecializationWater) {
                return AbilityWaterSwirl;
            }
            return AbilityFireSwirl;
        }
        if (m_entityData.specialization == SpecializationWater) {
            if (m_entityData.currentAbility == AbilityBraceletWaterbolt &&
                attackDuration() + 200 > m_entityData.timeSinceAbility) {
                return AbilityWaterbolt;
            }
            return AbilityBraceletWaterbolt;
        }
        if (m_entityData.currentAbility == AbilityBraceletFirebolt &&
            attackDuration() + 200 > m_entityData.timeSinceAbility) {
            return AbilityFirebolt;
        }
        return AbilityBraceletFirebolt;
    }
    if (m_entityData.race == RaceSnoutBeetle) {
        return AbilityShoot;
    }
    if (right.category == ItemCategoryWeapon && (right.id == WeaponBow || right.id == WeaponCrossbow)) {
        return AbilityShoot;
    }
    if (left.category == ItemCategoryWeapon && (left.id == WeaponBow || left.id == WeaponCrossbow)) {
        return AbilityShoot;
    }
    if (right.id == WeaponDagger) {
        if (m_entityData.currentAbility == AbilityRightDaggerPierce &&
            attackDuration() + 200 > m_entityData.timeSinceAbility) {
            return AbilityLeftDaggerPierce;
        }
        return AbilityRightDaggerPierce;
    }
    if (right.id == WeaponFist || right.category == ItemCategoryNone) {
        if (m_entityData.currentAbility == AbilityRightStab && attackDuration() + 200 > m_entityData.timeSinceAbility) {
            return AbilityLeftStab;
        }
        return AbilityRightStab;
    }
    if (right.id == WeaponLongsword) {
        if (m_entityData.currentAbility == AbilityRightSweep &&
            attackDuration() + 200 > m_entityData.timeSinceAbility) {
            return AbilityLeftSweep;
        }
        return AbilityRightSweep;
    }
    if (right.category == ItemCategoryWeapon && right.id == WeaponBoomerang) {
        return AbilityThrowBoomerang;
    }
    if (hasTwoHandedWeapon()) {
        if (m_entityData.currentAbility == AbilityTwoHandedSweep &&
            attackDuration() + 200 > m_entityData.timeSinceAbility) {
            return AbilityTwoHandedSwing;
        }
        if (m_entityData.currentAbility == AbilityTwoHandedSwing &&
            attackDuration() + 200 > m_entityData.timeSinceAbility) {
            return AbilityTwoHandedThrust;
        }
        return AbilityTwoHandedSweep;
    }
    if (left.category == ItemCategoryWeapon && left.id == WeaponShield) {
        if (m_entityData.currentAbility == AbilityBlockJab && attackDuration() + 200 > m_entityData.timeSinceAbility) {
            return AbilityShieldAttack;
        }
        return AbilityBlockJab;
    }
    if (m_entityData.currentAbility == AbilityRightSlash && attackDuration() + 200 > m_entityData.timeSinceAbility) {
        return AbilityLeftSlash;
    }
    return AbilityRightSlash;
}

int Creature::nextSpecialAttackSkill(bool alternative)
{
    if (alternative || findBuff(BuffNinjutsu)) {
        switch (m_entityData.classType) {
        case ClassMage:
            if (m_entityData.specialization == SpecializationWater) {
                return AbilityHealingStream;
            }
            break;
        case ClassRogue:
            return AbilityIntercept;
        }
    }
    if (m_entityData.race == RaceBloomling) {
        return AbilityBloomlingAttack;
    }
    switch (m_entityData.classType) {
    case ClassMonsterGunner:
        return AbilityChargedShot;
    case ClassMonsterArcher:
        return AbilityChargedShotRelease;
    case ClassMonsterBeamCaster:
        return AbilityFireRay;
    case ClassMonsterCaster:
        if (m_entityData.specialization == SpecializationWater) {
            return AbilityWaterball;
        }
        return AbilityFireball;
    case Class_145:
        return AbilityKick;
    case ClassMonster:
        return AbilityMonsterCharge;
    case Class_147:
    case Class_148:
        return AbilitySwirl;
    }
    Item &left = m_entityData.equipment.weaponLeft;
    Item &right = m_entityData.equipment.weaponRight;
    if (right.id == WeaponBiteAttack) {
        return AbilityClawStrike;
    }
    if (right.id == WeaponHandAttacks) {
        if (m_entityData.currentAbility == AbilityRightSlash &&
            attackDuration() + 200 > m_entityData.timeSinceAbility) {
            return AbilityLeftSlash;
        }
        return AbilityRightSlash;
    }
    if ((left.category == ItemCategoryWeapon && left.id == WeaponBow) ||
        (right.category == ItemCategoryWeapon && right.id == WeaponBow)) {
        return AbilitySalvo;
    }
    if ((left.category == ItemCategoryWeapon && left.id == WeaponCrossbow) ||
        (right.category == ItemCategoryWeapon && right.id == WeaponCrossbow)) {
        return AbilityChargedShot;
    }
    if ((left.category == ItemCategoryWeapon && left.id == WeaponBoomerang) ||
        (right.category == ItemCategoryWeapon && right.id == WeaponBoomerang)) {
        return AbilityChargedBoomerang;
    }
    if (isCaster()) {
        if (right.id == WeaponWand) {
            if (m_entityData.specialization == SpecializationWater) {
                return AbilityWaterRay;
            }
            return AbilityFireRay;
        }
        if (right.id == WeaponStaff) {
            if (m_entityData.specialization == SpecializationWater) {
                return AbilityWaterVortex;
            }
            return AbilityFireVortex;
        }
        if (m_entityData.specialization == SpecializationWater) {
            return AbilityWaterball;
        }
        return AbilityFireball;
    }
    if (right.category == ItemCategoryWeapon && right.id == WeaponLongsword) {
        return AbilityPerforate;
    }
    if (ItemCategoryWeapon == right.category && right.id == WeaponDagger) {
        return AbilityAmbush;
    }
    if (left.category == ItemCategoryWeapon && left.id == WeaponShield) {
        return AbilityBlock;
    }
    if (hasTwoHandedWeapon()) {
        return AbilityCrush;
    }
    if (right.id == WeaponFist || right.category == ItemCategoryNone) {
        return AbilityKick;
    }
    return AbilitySwirl;
}

void Creature::onHit()
{
    for (std::map<int, float>::iterator it = m_skillCooldowns.begin(); it != m_skillCooldowns.end(); ++it) {
    }
}

bool Creature::performAttack(bool alternative)
{
    if (m_entityData.rollTime != 0.0f || m_entityData.currentAbility == AbilityIntercept) {
        return false;
    }
    int duration = attackDuration();
    if (m_entityData.hitCombo != 0 && m_entityData.currentAbility != AbilityFireSwirl &&
        m_entityData.currentAbility != AbilityWaterSwirl) {
        duration -= holdTime();
    }
    int skill = nextAttackSkill(alternative);
    if (m_entityData.timeSinceAbility >= duration) {
        if (manaCost(skill, -1) > m_entityData.mp) {
            return false;
        }
        if (staminaCost(skill) > m_stamina) {
            return false;
        }
        m_entityData.currentAbility = skill;
        m_entityData.timeSinceAbility = 0.0f;
        return true;
    }
    return false;
}

// The skills 6, 18 and 21 need 0.1 mp (as 50 does) and are the ones a class 4 creature of specialization 1 can start
// while rolling. The skills 9, 25, 26, 28, 60 and 64 (and 6, 18, 21 during the current skill) interrupt the current
// skill at once, except after the skills 12, 61, 95 and 116.
bool Creature::performSpecialAttack(bool alternative)
{
    if (m_entityData.currentAbility == AbilityRockFistCharge || m_entityData.currentAbility == AbilityIntercept ||
        m_entityData.currentAbility == AbilityOfferPetFood) {
        return false;
    }
    int duration = attackDuration();
    if (m_entityData.hitCombo != 0) {
        duration -= holdTime();
    }
    int skill = nextSpecialAttackSkill(alternative);
    if (skill == AbilityBlock || skill == AbilityChargedShot || skill == AbilitySalvo ||
        skill == AbilityChargedBoomerang || skill == AbilityCrush || skill == AbilitySwirl ||
        ((skill == AbilityPerforate || skill == AbilityAmbush || skill == AbilityKick) &&
         attackDuration() > m_entityData.timeSinceAbility)) {
        if (skill == m_entityData.currentAbility) {
            return false;
        }
        if (m_entityData.currentAbility != AbilityCrushStrike && m_entityData.currentAbility != AbilityCyclone &&
            m_entityData.currentAbility != AbilityBlockStrike && m_entityData.currentAbility != AbilitySwirlStrike) {
            duration = 0;
            if (m_entityData.currentAbility == AbilityNone) {
                duration = 200;
            }
        }
    }
    bool usableWhileRolling = m_entityData.classType == ClassRogue &&
                              m_entityData.specialization == SpecializationNinja &&
                              (skill == AbilityPerforate || skill == AbilityAmbush || skill == AbilityKick);
    if (m_entityData.timeSinceAbility > duration && (m_entityData.rollTime == 0.0f || usableWhileRolling)) {
        if (manaCost(skill, -1) > m_entityData.mp) {
            return false;
        }
        if ((skill == AbilityPerforate || skill == AbilityAmbush || skill == AbilityKick ||
             skill == AbilityIntercept) &&
            0.1f > m_entityData.mp) {
            return false;
        }
        m_entityData.currentAbility = nextSpecialAttackSkill(alternative);
        m_entityData.timeSinceAbility = 0.0f;
        if (m_entityData.currentAbility == AbilityFireVortex || m_entityData.currentAbility == AbilityWaterVortex ||
            m_entityData.currentAbility == AbilityFireball || m_entityData.currentAbility == Ability_48 ||
            m_entityData.currentAbility == AbilityFireRay) {
            if (findBuff(BuffInstantfire)) {
                m_entityData.timeSinceAbility = anticipationTime();
            }
        }
        if (!usableWhileRolling) {
            m_entityData.rollTime = 0.0f;
        }
        return true;
    }
    return false;
}

int Creature::platinum()
{
    if (!m_playerData) {
        return 0;
    }
    std::array<int, 2> region = {m_entityData.region[0], m_entityData.region[1]};
    std::map<std::array<int, 2>, unsigned int>::const_iterator it = m_playerData->platinum.find(region);
    if (it == m_playerData->platinum.end()) {
        return 0;
    }
    return it->second;
}

float prizeScale(float level)
{
    return (level - 1.0f) * 0.1f + 1.0f;
}

int rampDuration(int level)
{
    return (int)((1.0f - 1.0f / ((level - 1.0f) / 50.0f + 1.0f)) * 10000.0f + 10000.0f);
}

float rampRange(int level)
{
    return (1.0f - 1.0f / ((level - 1.0f) / 50.0f + 1.0f)) * 5.0f + 5.0f;
}

float rampScale(int level)
{
    return (1.0f - 1.0f / ((level - 1.0f) / 50.0f + 1.0f)) * 0.5f + 0.5f;
}

float rampSpeed(int level)
{
    return (1.0f - 1.0f / ((level - 1.0f) / 50.0f + 1.0f)) * 15.0f + 15.0f;
}

// A value in 1..100000 from two integers (the integer noise of libnoise's IntValueNoise with its constants 60493,
// 19990303 and 1376312589).
static inline unsigned int hashToRange(int x, int y)
{
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return 100000 - ((n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff) / 21474;
}
std::wstring randomName(int seed, int race)
{
    static std::wstring defaultFirst[30] = {L"Kro", L"Ari",  L"As",  L"Ge",  L"Kur", L"Ro",  L"Iko",  L"La",
                                            L"Des", L"Var",  L"An",  L"The", L"Ga",  L"Lan", L"Dura", L"Dama",
                                            L"Se",  L"Thal", L"Nar", L"San", L"Tel", L"O",   L"Ar",   L"Mar",
                                            L"Gro", L"Yu",   L"Ja",  L"Lan", L"Cor", L"Co"};
    static std::wstring defaultSecond[30] = {L"no",  L"mi",   L"la",  L"ria",  L"ron", L"ion",  L"kor", L"lon",
                                             L"rok", L"tar",  L"dan", L"gor",  L"dar", L"dara", L"lan", L"ka",
                                             L"gor", L"rior", L"ria", L"mor",  L"sen", L"dor",  L"ion", L"kin",
                                             L"len", L"ber",  L"lur", L"more", L"ris", L"nis"};
    static std::wstring humanMaleFirst[10] = {L"De",  L"Ro",   L"As",  L"Ge", L"An",
                                              L"Pat", L"Wolf", L"Ale", L"Mi", L"Ben"};
    static std::wstring humanMaleSecond[10] = {L"rek", L"man",    L"gram", L"rald", L"rick",
                                               L"ram", L"sander", L"kal",  L"ton",  L"ny"};
    static std::wstring humanFemaleFirst[10] = {L"Sa",   L"Jen",  L"Kla", L"Mi", L"La",
                                                L"Auri", L"Mela", L"Alu", L"Ve", L"Est"};
    static std::wstring humanFemaleSecond[10] = {L"rah", L"ny", L"ria",   L"elle", L"na",
                                                 L"ga",  L"ma", L"riana", L"rona", L"una"};
    static std::wstring elfMaleFirst[10] = {L"El", L"Li", L"Dri", L"Elan", L"Le", L"Ly", L"A", L"Tho", L"Da", L"Les"};
    static std::wstring elfMaleSecond[10] = {L"ric",    L"kun",  L"zic",  L"dor",    L"las",
                                             L"sander", L"reon", L"reas", L"lundra", L"andor"};
    static std::wstring elfFemaleFirst[10] = {L"Ar", L"A", L"Si", L"Mer", L"Lu", L"Ari", L"Su", L"Sira", L"Hia", L"Li"};
    static std::wstring elfFemaleSecond[10] = {L"weya", L"luna",  L"laya", L"leya", L"ma",
                                               L"lia",  L"matra", L"vy",   L"zyna", L"ly"};
    static std::wstring goblinMaleFirst[10] = {L"Gra", L"Xe", L"Ur",  L"Bel",  L"Chu",
                                               L"Ki",  L"Go", L"Zim", L"Kubo", L"Raz"};
    static std::wstring goblinMaleSecond[10] = {L"zic", L"nax", L"tik", L"tuk", L"ruk",
                                                L"bo",  L"rix", L"mi",  L"tok", L"nor"};
    static std::wstring goblinFemaleFirst[10] = {L"Kur", L"Xe", L"Ki",  L"Am", L"Zifa",
                                                 L"Zu",  L"Ma", L"Chi", L"Zi", L"Mia"};
    static std::wstring goblinFemaleSecond[10] = {L"ra", L"lia",  L"bi", L"ba", L"ly",
                                                  L"ki", L"bara", L"mi", L"zy", L"xa"};
    static std::wstring lizardmanMaleFirst[10] = {L"Cho",  L"Zer", L"Kaz", L"Kraz", L"Zel",
                                                  L"Drak", L"Lo",  L"Raz", L"Spi",  L"Zen"};
    static std::wstring lizardmanMaleSecond[10] = {L"rux", L"rek", L"zic", L"tac", L"mec",
                                                   L"kor", L"rax", L"zor", L"ro",  L"go"};
    static std::wstring lizardmanFemaleFirst[10] = {L"Zi",   L"Zer", L"Az",  L"Ki",  L"Iza",
                                                    L"Drak", L"Li",  L"May", L"Spi", L"Zan"};
    static std::wstring lizardmanFemaleSecond[10] = {L"rya", L"ri",   L"zia", L"ki", L"mah",
                                                     L"ira", L"maya", L"ana", L"ra", L"ya"};
    static std::wstring dwarfMaleFirst[10] = {L"Tor", L"Gem",  L"Bar", L"Me",   L"As",
                                              L"Tem", L"Arak", L"Ur",  L"Grim", L"Xor"};
    static std::wstring dwarfMaleSecond[10] = {L"lox", L"bart", L"kor", L"thos", L"kun",
                                               L"bur", L"thor", L"lok", L"li",   L"bor"};
    static std::wstring dwarfFemaleFirst[10] = {L"Mum", L"Grun", L"Brun", L"Hei",  L"Mo",
                                                L"Hil", L"Ur",   L"Lum",  L"Grim", L"Xor"};
    static std::wstring dwarfFemaleSecond[10] = {L"pie",  L"hild", L"di",  L"muna", L"ki",
                                                 L"trud", L"sa",   L"axa", L"ira",  L"ika"};
    static std::wstring orcMaleFirst[10] = {L"Uz", L"Ur", L"Chu", L"Ku", L"Mor", L"Ura", L"Ak", L"Ur", L"Or", L"Gor"};
    static std::wstring orcMaleSecond[10] = {L"ku",  L"ruk",   L"muk",  L"thak", L"dok",
                                             L"tor", L"rorok", L"chak", L"kaz",  L"rack"};
    static std::wstring orcFemaleFirst[10] = {L"Uz", L"Ur", L"Chu", L"Ku", L"Mor", L"Ura", L"Ak", L"Ur", L"Or", L"Gor"};
    static std::wstring orcFemaleSecond[10] = {L"ka",  L"rua",   L"mua",   L"thara", L"daka",
                                               L"tah", L"rorah", L"chaka", L"kaya",  L"rana"};
    static std::wstring undeadMaleFirst[10] = {L"Chu", L"Dro", L"Al",  L"Dem", L"Mor",
                                               L"Mur", L"Cro", L"Zul", L"Dra", L"The"};
    static std::wstring undeadMaleSecond[10] = {L"lu",  L"kor",   L"card", L"morius", L"enius",
                                                L"tus", L"demar", L"lus",  L"ruul",   L"zad"};
    static std::wstring undeadFemaleFirst[10] = {L"Xu",  L"Dra", L"Al",  L"Dem", L"Mor",
                                                 L"Myr", L"Cra", L"Zul", L"Dri", L"Thu"};
    static std::wstring undeadFemaleSecond[10] = {L"la",   L"kira",  L"cara", L"moria", L"ena",
                                                  L"tana", L"diria", L"laza", L"rah",   L"zazah"};
    static std::wstring frogmanMaleFirst[10] = {L"Qua", L"Rib", L"Quib", L"Zib", L"Il",
                                                L"Wok", L"Wib", L"Moko", L"Sli", L"Bul"};
    static std::wstring frogmanMaleSecond[10] = {L"rik", L"bit", L"ble", L"bik", L"wak",
                                                 L"wok", L"wib", L"luk", L"mey", L"bak"};
    static std::wstring frogmanFemaleFirst[10] = {L"Qua", L"Rib", L"Quib", L"Zib", L"Il",
                                                  L"Wok", L"Wib", L"Moko", L"Sli", L"Bul"};
    static std::wstring frogmanFemaleSecond[10] = {L"ria",  L"bia",   L"bla", L"bia",  L"waka",
                                                   L"woka", L"wibba", L"lua", L"maya", L"ba"};
    std::wstring name;
    unsigned int first = hashToRange(seed, race);
    unsigned int second = hashToRange(race + 3852967, seed);
    switch (race) {
    case RaceElfMale:
        return elfMaleFirst[first % 10] + elfMaleSecond[second % 10];
    case RaceElfFemale:
        return elfFemaleFirst[first % 10] + elfFemaleSecond[second % 10];
    case RaceHumanMale:
    case RaceOldMan:
    case RaceWizard:
    case RaceNomadMale:
    case RacePirateMale:
    case RaceEnhancedPirateMale:
        return humanMaleFirst[first % 10] + humanMaleSecond[second % 10];
    case RaceHumanFemale:
    case RaceWitch:
    case RaceNomadFemale:
    case RacePirateFemale:
        return humanFemaleFirst[first % 10] + humanFemaleSecond[second % 10];
    case RaceGoblinMale:
        return goblinMaleFirst[first % 10] + goblinMaleSecond[second % 10];
    case RaceGoblinFemale:
        return goblinFemaleFirst[first % 10] + goblinFemaleSecond[second % 10];
    case RaceLizardmanMale:
        return lizardmanMaleFirst[first % 10] + lizardmanMaleSecond[second % 10];
    case RaceLizardmanFemale:
        return lizardmanFemaleFirst[first % 10] + lizardmanFemaleSecond[second % 10];
    case RaceDwarfMale:
        return dwarfMaleFirst[first % 10] + dwarfMaleSecond[second % 10];
    case RaceDwarfFemale:
        return dwarfFemaleFirst[first % 10] + dwarfFemaleSecond[second % 10];
    case RaceOrcMale:
    case RaceEnhancedOrcMale:
        return orcMaleFirst[first % 10] + orcMaleSecond[second % 10];
    case RaceOrcFemale:
    case RaceEnhancedOrcFemale:
        return orcFemaleFirst[first % 10] + orcFemaleSecond[second % 10];
    case RaceFrogmanMale:
        return frogmanMaleFirst[first % 10] + frogmanMaleSecond[second % 10];
    case RaceFrogmanFemale:
        return frogmanFemaleFirst[first % 10] + frogmanFemaleSecond[second % 10];
    case RaceUndeadMale:
    case RaceSkeleton:
    case RaceZombie:
    case RaceVampire:
    case RaceSkeletonKnight:
    case RaceMonstrosity:
        return undeadMaleFirst[first % 10] + undeadMaleSecond[second % 10];
    case RaceUndeadFemale:
        return undeadFemaleFirst[first % 10] + undeadFemaleSecond[second % 10];
    default:
        return defaultFirst[first % 30] + defaultSecond[second % 30];
    }
}

plasma::Vector<4, float> rarityColor(int rarity)
{
    int clamped = rarity;
    if (clamped < ItemRarityGrey) {
        clamped = ItemRarityGrey;
    }
    if (clamped > ItemRarityYellow) {
        clamped = ItemRarityYellow;
    }
    switch (clamped) {
    case ItemRarityGrey:
        return plasma::Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f);
    case ItemRarityWhite:
        return plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    case ItemRarityGreen:
        return plasma::Vector<4, float>(0.0f, 1.0f, 0.0f, 1.0f);
    case ItemRarityBlue:
        return plasma::Vector<4, float>(0.3f, 0.45f, 1.0f, 1.0f);
    case ItemRarityPurple:
        return plasma::Vector<4, float>(0.55f, 0.2f, 1.0f, 1.0f);
    case ItemRarityYellow:
        return plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f);
    default:
        return plasma::Vector<4, float>(1.0f, 0.2f, 0.2f, 1.0f);
    }
}

float Creature::regeneration()
{
    float regeneration = 1.0f;
    Equipment &equipment = m_entityData.equipment;
    if (equipment.weaponLeft.category == ItemCategoryWeapon) {
        regeneration += equipment.weaponLeft.regeneration(this);
    }
    if (equipment.weaponRight.category == ItemCategoryWeapon) {
        regeneration += equipment.weaponRight.regeneration(this);
    }
    if (equipment.chest.category == ItemCategoryChest) {
        regeneration += equipment.chest.regeneration(this);
    }
    if (equipment.feet.category == ItemCategoryFeet) {
        regeneration += equipment.feet.regeneration(this);
    }
    if (equipment.hands.category == ItemCategoryHands) {
        regeneration += equipment.hands.regeneration(this);
    }
    if (equipment.shoulder.category == ItemCategoryShoulder) {
        regeneration += equipment.shoulder.regeneration(this);
    }
    if (equipment.neck.category == ItemCategoryNeck) {
        regeneration += equipment.neck.regeneration(this);
    }
    if (equipment.ringLeft.category == ItemCategoryRing) {
        regeneration += equipment.ringLeft.regeneration(this);
    }
    if (equipment.ringRight.category == ItemCategoryRing) {
        regeneration += equipment.ringRight.regeneration(this);
    }
    return regeneration;
}

void Creature::removeItem(const Item &item, int count)
{
    for (std::vector<ItemStack> &tab : m_inventory.m_inventoryTabs) {
        for (ItemStack &stack : tab) {
            if (stack.item == item) {
                int removed = std::min(count, stack.quantity);
                stack.quantity -= removed;
                count -= removed;
                if (stack.quantity == 0) {
                    stack.item = Item();
                }
            }
        }
    }
}

void Creature::reset()
{
    m_entityData.reset();
    m_opponentId = 0;
    m_petId = 0;
    m_strikeCombo = 0;
    m_rollTime = 0;
    m_lastPetSyncValue = 0;
    m_interactions.clear();
    m_aggro.clear();
    m_hitCounts.clear();
    m_skillCooldowns.clear();
    m_damageDealt = 0;
    m_unusedBc4 = 0;
    m_resetOnHit = 0;
    m_hitFlash = 0.0f;
    m_walkAmplitude = 0.0f;
    m_stamina = 1.0f;
    m_smoothingTime = 0;
    m_walkPhase = 0.0f;
    m_fallStartHeight = 0;
    m_glideTime = 0;
    m_leapDuration = 0;
    m_strikeStrength = 0.0f;
    m_criticalStrike = 0;
    m_inventory.m_inventoryTabs.clear();
    m_unusedC18 = 0;
    m_unusedRecords.clear();
    m_buffs.clear();
}

void Creature::EntityData::reset()
{
    velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    retreat = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    currentAbility = AbilityNone;
    timeSinceAbility = 0.0f;
    stunTime = -3000.0f;
    actionLockTime = 0.0f;
    slowedTime = 0.0f;
    sprintTime = 0.0f;
    revivalPenalty = 0;
    memset(&equipment, 0, sizeof(equipment));
    binaryToggles = EntityData::BinaryToggles();
    interactionState = 0;
    buildingModel = 0;
    hitCombo = 0;
    timeSinceHit = 0.0f;
    charge = 0.0f;
    blockPower = 0.0f;
    mp = 0.0f;
    stealth = 0.0f;
    petRefreshValue = 0;
    mediumSpeed = 1.0f;
    lampDiameter = 10.0f;
    ownerId = 0;
    focusId = 0;
    portCity = plasma::Vector<2, int>(-1, -1);
    finalHandsRotation[0] = 0;
    finalHandsRotation[1] = 0;
    finalHandsRotation[2] = 0;
    initialHandsRotation[0] = 0;
    initialHandsRotation[1] = 0;
    initialHandsRotation[2] = 0;
    destination = plasma::Vector<2, int>(-1, -1);
    level = 1;
    xp = 0;
    classType = ClassNone;
    specialization = 0;
    appearance = Appearance();
    memset(name, 0, sizeof(name));
    seed = 0;
    factionKind = FactionNone;
    factionVariant = 0;
    factionSeed = 0;
}

float Creature::resistance()
{
    float resistance = 0.0f;
    Equipment &equipment = m_entityData.equipment;
    if (equipment.chest.category == ItemCategoryChest &&
        (m_entityData.hostilityType != HostilityPlayer || equipment.chest.fitsClass(m_entityData.classType))) {
        resistance += equipment.chest.resistance(this);
    }
    if (equipment.feet.category == ItemCategoryFeet &&
        (m_entityData.hostilityType != HostilityPlayer || equipment.feet.fitsClass(m_entityData.classType))) {
        resistance += equipment.feet.resistance(this);
    }
    if (equipment.hands.category == ItemCategoryHands &&
        (m_entityData.hostilityType != HostilityPlayer || equipment.hands.fitsClass(m_entityData.classType))) {
        resistance += equipment.hands.resistance(this);
    }
    if (equipment.shoulder.category == ItemCategoryShoulder &&
        (m_entityData.hostilityType != HostilityPlayer || equipment.shoulder.fitsClass(m_entityData.classType))) {
        resistance += equipment.shoulder.resistance(this);
    }
    if (m_entityData.appearance.appearanceFlags & (AppearanceFlagTinted | AppearanceFlagEnhanced)) {
        resistance *= 1.2f;
    }
    if (m_entityData.appearance.hasFlag(AppearanceFlagLarge)) {
        resistance *= 2.0f;
    }
    if (findBuff(BuffDefenseWeakened)) {
        resistance *= 0.25f;
    }
    Buff *buff = findBuff(BuffResistanceflask);
    if (buff) {
        resistance *= buff->strength;
    }
    return resistance;
}

plasma::Vector<3, float> riderOffset(int race)
{
    switch (race) {
    case RaceTerrier:
    case RaceCat:
        return plasma::Vector<3, float>(0.0f, 0.0f, -0.9f);
    case RaceCrocodile:
    case RacePirateBoat:
        return plasma::Vector<3, float>(0.0f, 0.0f, 0.1f);
    case RaceDuckbill:
    case RaceBabyElephant:
    case RaceBabyMammoth:
        return plasma::Vector<3, float>(0.0f, 0.0f, -0.3f);
    case RacePig:
        return plasma::Vector<3, float>(0.0f, 0.0f, -0.2f);
    case RaceBumblebee:
        return plasma::Vector<3, float>(0.0f, 0.0f, -0.35f);
    case RaceToucan:
        return plasma::Vector<3, float>(0.0f, 0.0f, -0.6f);
    case RaceFlamingo:
        return plasma::Vector<3, float>(0.0f, -0.3f, -0.3f);
    case RaceSilverLion:
        return plasma::Vector<3, float>(0.0f, 0.0f, 0.25f);
    case RaceBlueTurtle:
        return plasma::Vector<3, float>(0.0f, 0.0f, 0.4f);
    case RaceManaMech:
        return plasma::Vector<3, float>(0.0f, 0.0f, -1.0f);
    case RaceTreant:
        return plasma::Vector<3, float>(0.0f, 0.0f, -2.0f);
    case RaceSkullSlater:
        return plasma::Vector<3, float>(0.0f, 0.0f, -1.6f);
    case RaceCaterpillar:
        return plasma::Vector<3, float>(0.0f, 0.0f, -0.4f);
    case RaceEarthCaterpillar:
        return plasma::Vector<3, float>(0.0f, 0.0f, -0.4f);
    case RaceSnail:
        return plasma::Vector<3, float>(0.0f, 0.0f, 0.4f);
    default:
        return plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    }
}

float Creature::ridingSpeed()
{
    if (m_entityData.hostilityType != HostilityPlayer) {
        return (1.0f - 1.0f / (m_entityData.level * 10 / 20.0f + 1.0f)) * 1.4f + 1.1f;
    }
    return (1.0f - 1.0f / (m_artifactSkills[ArtifactEffect_RidingSpeed] / 20.0f + 1.0f)) * 1.4f + 1.1f;
}

float Creature::sailingSpeed()
{
    if (m_entityData.hostilityType != HostilityPlayer) {
        return (1.0f - 1.0f / (m_entityData.level * 10 / 20.0f + 1.0f)) * 1.9f + 1.1f;
    }
    return (1.0f - 1.0f / (m_artifactSkills[ArtifactEffect_SailingSpeed] / 20.0f + 1.0f)) * 1.9f + 1.1f;
}

void Creature::setBuff(const Buff &buff, bool replace)
{
    if (replace || buff.type == BuffBulwark || buff.type == BuffWarfrenzy || buff.type == BuffCamouflage ||
        buff.type == BuffManashield || buff.type == BuffTameness || buff.type == BuffInstantfire ||
        buff.type == BuffInstantshoot || buff.type == BuffDodge) {
        for (std::list<Buff>::iterator it = m_buffs.begin(); it != m_buffs.end(); ++it) {
            if (it->type == buff.type && it->sourceId == buff.sourceId) {
                *it = buff;
                return;
            }
        }
    }
    m_buffs.push_back(buff);
}

int shelterDuration(int level)
{
    return (int)((1.0f - 1.0f / ((level - 1.0f) / 50.0f + 1.0f)) * 15000.0f + 15000.0f);
}

plasma::Vector<3, Fixed> Creature::sightPosition()
{
    if (m_entityData.appearance.appearanceFlags & AppearanceFlagHeadAimOrigin) {
        plasma::Matrix<float> rotation;
        rotation.identity();
        rotation.rotateZ(m_entityData.rotation[2]);
        plasma::Vector<3, float> offset =
            rotation * plasma::Vector<3, float>(0.0f, m_entityData.appearance.size[1] * 0.5f,
                                                m_entityData.appearance.size[2] * 0.35f);
        return m_entityData.position + plasma::Vector<3, Fixed>(offset);
    }
    return m_entityData.position;
}

int Creature::skillLevel(int skill)
{
    return 1000;
}

float skillScale(int level)
{
    return (1.0f - 1.0f / ((level - 1.0f) / 50.0f + 1.0f)) * 0.4f + 0.8f;
}

int Creature::specialSkill()
{
    int skill = AbilityNone;
    switch (m_entityData.classType) {
    case ClassWarrior:
        skill = m_entityData.specialization == SpecializationBerserker ? AbilityRockFist : AbilityHeroicShout;
        break;
    case ClassRanger:
        skill = m_entityData.specialization == SpecializationSniper ? AbilityShadowClone : AbilityQuicksandTrap;
        break;
    case ClassMage:
        skill = m_entityData.specialization == SpecializationFire ? AbilityFireMissiles : AbilityWaterBubbles;
        break;
    case ClassRogue:
        skill = m_entityData.specialization == SpecializationAssassin ? AbilityCamouflage : AbilityNinjutsu;
        break;
    }
    return skill;
}

int speciesType(int race)
{
    switch (race) {
    case RaceElfMale:
        return SpeciesHumanoid;
    case RaceElfFemale:
        return SpeciesHumanoid;
    case RaceHumanMale:
        return SpeciesHumanoid;
    case RaceHumanFemale:
        return SpeciesHumanoid;
    case RaceGoblinMale:
        return SpeciesHumanoid;
    case RaceGoblinFemale:
        return SpeciesHumanoid;
    case RaceBullterrier:
        return SpeciesHumanoid;
    case RaceLizardmanMale:
        return SpeciesHumanoid;
    case RaceLizardmanFemale:
        return SpeciesHumanoid;
    case RaceDwarfMale:
        return SpeciesHumanoid;
    case RaceDwarfFemale:
        return SpeciesHumanoid;
    case RaceOrcMale:
        return SpeciesHumanoid;
    case RaceOrcFemale:
        return SpeciesHumanoid;
    case RaceFrogmanMale:
        return SpeciesHumanoid;
    case RaceFrogmanFemale:
        return SpeciesHumanoid;
    case RaceUndeadMale:
        return SpeciesHumanoid;
    case RaceUndeadFemale:
        return SpeciesHumanoid;
    case RaceSkeleton:
        return SpeciesUndead;
    case RaceOldMan:
        return SpeciesHumanoid;
    case RaceCollie:
        return SpeciesAnimal;
    case RaceShepherdDog:
        return SpeciesAnimal;
    case RaceSkullBull:
        return SpeciesAnimal;
    case RaceAlpaca:
        return SpeciesAnimal;
    case RaceBrownAlpaca:
        return SpeciesAnimal;
    case RaceEgg:
        return SpeciesAnimal;
    case RaceTurtle:
        return SpeciesAnimal;
    case RaceTerrier:
        return SpeciesAnimal;
    case RaceScottishTerrier:
        return SpeciesAnimal;
    case RaceWolf:
        return SpeciesAnimal;
    case RacePanther:
        return SpeciesAnimal;
    case RaceCat:
        return SpeciesAnimal;
    case RaceBrownCat:
        return SpeciesAnimal;
    case RaceWhiteCat:
        return SpeciesAnimal;
    case RacePig:
        return SpeciesAnimal;
    case RaceSheep:
        return SpeciesAnimal;
    case RaceBunny:
        return SpeciesAnimal;
    case RacePorcupine:
        return SpeciesAnimal;
    case RaceFrightener:
        return SpeciesUndead;
    case RaceSandHorror:
        return SpeciesUndead;
    case RaceWizard:
        return SpeciesHumanoid;
    case RaceBandit:
        return SpeciesHumanoid;
    case RaceWitch:
        return SpeciesHumanoid;
    case RaceOgre:
        return SpeciesHumanoid;
    case RaceGnoll:
        return SpeciesHumanoid;
    case RacePolarGnoll:
        return SpeciesHumanoid;
    case RaceMonkey:
        return SpeciesAnimal;
    case RaceGnobold:
        return SpeciesHumanoid;
    case RaceInsectoid:
        return SpeciesHumanoid;
    case RaceHornet:
        return SpeciesAnimal;
    case RaceInsectGuard:
        return SpeciesHumanoid;
    case RaceCrow:
        return SpeciesAnimal;
    case RaceChicken:
        return SpeciesAnimal;
    case RaceSeagull:
        return SpeciesAnimal;
    case RaceParrot:
        return SpeciesAnimal;
    case RaceBat:
        return SpeciesAnimal;
    case RaceFly:
        return SpeciesAnimal;
    case RaceMidge:
        return SpeciesAnimal;
    case RaceMosquito:
        return SpeciesAnimal;
    case RacePlainRunner:
        return SpeciesAnimal;
    case RaceLeafRunner:
        return SpeciesAnimal;
    case RaceSnowRunner:
        return SpeciesAnimal;
    case RaceDesertRunner:
        return SpeciesAnimal;
    case RacePeacock:
        return SpeciesAnimal;
    case RaceFrog:
        return SpeciesAnimal;
    case RacePlantCreature:
        return SpeciesPlantCreature;
    case RaceRadishCreature:
        return SpeciesPlantCreature;
    case RaceOnionling:
        return SpeciesPlantCreature;
    case RaceDesertOnionling:
        return SpeciesPlantCreature;
    case RaceDevourer:
        return SpeciesDemon;
    case RaceDuckbill:
        return SpeciesAnimal;
    case RaceCrocodile:
        return SpeciesAnimal;
    case RaceSpikeCreature:
        return SpeciesHumanoid;
    case RaceAnubis:
        return SpeciesHumanoid;
    case RaceHorus:
        return SpeciesHumanoid;
    case RaceJester:
        return SpeciesHumanoid;
    case RaceSpectrino:
        return SpeciesHumanoid;
    case RaceDjinn:
        return SpeciesHumanoid;
    case RaceMinotaur:
        return SpeciesDemon;
    case RaceNomadMale:
        return SpeciesHumanoid;
    case RaceNomadFemale:
        return SpeciesHumanoid;
    case RaceImp:
        return SpeciesDemon;
    case RaceMole:
        return SpeciesAnimal;
    case RaceBiter:
        return SpeciesAnimal;
    case RaceKoala:
        return SpeciesAnimal;
    case RaceSquirrel:
        return SpeciesAnimal;
    case RaceRaccoon:
        return SpeciesAnimal;
    case RaceOwl:
        return SpeciesAnimal;
    case RacePenguin:
        return SpeciesAnimal;
    case RaceSanta:
        return SpeciesHumanoid;
    case RaceZombie:
        return SpeciesUndead;
    case RaceVampire:
        return SpeciesUndead;
    case RaceHorse:
        return SpeciesAnimal;
    case RaceCamel:
        return SpeciesAnimal;
    case RaceCow:
        return SpeciesAnimal;
    case RaceBarkBeetle:
        return SpeciesAnimal;
    case RaceFireBeetle:
        return SpeciesAnimal;
    case RaceSnoutBeetle:
        return SpeciesAnimal;
    case RaceLemonBeetle:
        return SpeciesAnimal;
    case RaceCrab:
        return SpeciesAnimal;
    case RaceSeaCrab:
        return SpeciesAnimal;
    case RaceHellDemon:
        return SpeciesDemon;
    case RaceMammoth:
        return SpeciesAnimal;
    case RaceLich:
        return SpeciesUndead;
    case RaceBush:
        return SpeciesPlant;
    case RaceSnowBush:
        return SpeciesPlant;
    case RaceSnowBerryBush:
        return SpeciesPlant;
    case RaceCottonPlant:
        return SpeciesPlant;
    case RaceScrub:
        return SpeciesPlant;
    case RaceCobwebScrub:
        return SpeciesPlant;
    case RaceFireScrub:
        return SpeciesPlant;
    case RaceGinseng:
        return SpeciesPlant;
    case RaceCactus:
        return SpeciesPlant;
    case RaceFirTree:
        return SpeciesPlant;
    case RaceThornTree:
        return SpeciesPlant;
    case RaceGoldDeposit:
        return SpeciesObject;
    case RaceIronDeposit:
        return SpeciesObject;
    case RaceSilverDeposit:
        return SpeciesObject;
    case RaceSandstoneDeposit:
        return SpeciesObject;
    case RaceEmeraldDeposit:
        return SpeciesObject;
    case RaceSapphireDeposit:
        return SpeciesObject;
    case RaceRubyDeposit:
        return SpeciesObject;
    case RaceDiamondDeposit:
        return SpeciesObject;
    case RaceIceCrystalDeposit:
        return SpeciesObject;
    case RaceScarecrow:
        return SpeciesObject;
    case RaceAim:
        return SpeciesObject;
    case RaceDummy:
        return SpeciesObject;
    case RaceVase:
        return SpeciesObject;
    case RaceBomb:
        return SpeciesObject;
    case RaceSapphireFish:
        return SpeciesAnimal;
    case RaceLemonFish:
        return SpeciesAnimal;
    case RaceSeahorse:
        return SpeciesAnimal;
    case RaceMermaid:
        return SpeciesHumanoid;
    case RaceMerman:
        return SpeciesHumanoid;
    case RaceShark:
        return SpeciesAnimal;
    case RaceBumblebee:
        return SpeciesAnimal;
    case RaceLanternFish:
        return SpeciesAnimal;
    case RaceMawFish:
        return SpeciesAnimal;
    case RacePiranha:
        return SpeciesAnimal;
    case RaceBlowfish:
        return SpeciesAnimal;
    case RaceBeaver:
        return SpeciesAnimal;
    case RaceMoose:
        return SpeciesAnimal;
    case RaceZebra:
        return SpeciesAnimal;
    case RaceElephant:
        return SpeciesAnimal;
    case RaceRhino:
        return SpeciesAnimal;
    case RaceOstrich:
        return SpeciesAnimal;
    case RaceLionMale:
        return SpeciesAnimal;
    case RaceLionFemale:
        return SpeciesAnimal;
    case RaceSkeletonKnight:
        return SpeciesUndead;
    case RaceSkeletonHorse:
        return SpeciesUndead;
    case RaceSorcerer:
        return SpeciesHumanoid;
    case RaceHyena:
        return SpeciesAnimal;
    case RaceGiraffe:
        return SpeciesAnimal;
    case RaceWarthog:
        return SpeciesAnimal;
    case RaceAntelope:
        return SpeciesAnimal;
    case RaceMakake:
        return SpeciesAnimal;
    case RaceSkeletonDog:
        return SpeciesUndead;
    case RaceLavaTurtle:
        return SpeciesAnimal;
    case RaceDemonPortal:
        return SpeciesObject;
    case RaceSlimeWell:
        return SpeciesObject;
    case RaceHornetsNest:
        return SpeciesObject;
    case RaceUnholyTomb:
        return SpeciesObject;
    case RaceBonePile:
        return SpeciesObject;
    case RaceMolehill:
        return SpeciesObject;
    case RaceWoodChest:
        return SpeciesObject;
    case RaceSkullChest:
        return SpeciesObject;
    case RaceObsidianChest:
        return SpeciesObject;
    case RaceBoneChest:
        return SpeciesObject;
    case RaceCampfire:
        return SpeciesObject;
    case RaceTent:
        return SpeciesObject;
    case RaceSleepingMat:
        return SpeciesObject;
    case RaceBeachTowel:
        return SpeciesObject;
    case RaceSunshade:
        return SpeciesObject;
    case RaceSign:
        return SpeciesObject;
    case RaceMantis:
        return SpeciesHumanoid;
    case RaceRadishCreatureSprout:
        return SpeciesPlantCreature;
    case RacePlantCreatureSprout:
        return SpeciesPlantCreature;
    case RaceCage1:
        return SpeciesObject;
    case RaceCage2:
        return SpeciesObject;
    case RaceCage3:
        return SpeciesObject;
    case RaceCage4:
        return SpeciesObject;
    case RaceCauldron:
        return SpeciesObject;
    case RaceSeeker:
        return SpeciesHumanoid;
    case RaceBabyElephant:
        return SpeciesAnimal;
    case RaceBabyMammoth:
        return SpeciesAnimal;
    case RacePirateMale:
        return SpeciesHumanoid;
    case RacePirateFemale:
        return SpeciesHumanoid;
    case RaceTreasure:
        return SpeciesObject;
    case RaceTreasureStatue:
        return SpeciesObject;
    case RaceTreasureJewel:
        return SpeciesObject;
    case RaceTreasureVase:
        return SpeciesObject;
    case RaceEnhancedPirateMale:
        return SpeciesHumanoid;
    case RaceGoldKnightHorse:
        return SpeciesAnimal;
    case RaceFlamingo:
        return SpeciesAnimal;
    case RaceToucan:
        return SpeciesAnimal;
    case RaceWagon:
        return SpeciesObject;
    case RaceSilverLion:
        return SpeciesAnimal;
    case RaceRuneCube:
        return SpeciesObject;
    case RaceBlueTurtle:
        return SpeciesAnimal;
    case RaceSnowGnobold:
        return SpeciesHumanoid;
    case RaceSavageDog:
        return SpeciesAnimal;
    case RaceSnowLionMale:
        return SpeciesAnimal;
    case RaceSnowLionFemale:
        return SpeciesAnimal;
    case RaceSnowLeopard:
        return SpeciesAnimal;
    case RaceBrazier:
        return SpeciesObject;
    case RaceGhostBrazier:
        return SpeciesObject;
    case RaceUnholyBonfire:
        return SpeciesObject;
    case RaceAlphaDog:
        return SpeciesAnimal;
    case RaceFlyingEye:
        return SpeciesDemon;
    case RaceDemonMarauder:
        return SpeciesDemon;
    case RaceDarkCultistMale:
        return SpeciesHumanoid;
    case RaceDarkCultistFemale:
        return SpeciesHumanoid;
    case RaceCrawler:
        return SpeciesDemon;
    case RaceNecromancer:
        return SpeciesHumanoid;
    case RaceDemonIdol:
        return SpeciesObject;
    case RaceBloodaxeClanBanner:
        return SpeciesObject;
    case RaceStoneTablet:
        return SpeciesObject;
    case RaceLever:
        return SpeciesObject;
    case RaceLockedLever:
        return SpeciesObject;
    case RaceEmpireSoldierMale:
        return SpeciesHumanoid;
    case RaceEmpireSoldierFemale:
        return SpeciesHumanoid;
    case RaceTombstone:
        return SpeciesObject;
    case RaceManaPump:
        return SpeciesObject;
    case RaceManaInductor:
        return SpeciesObject;
    case RaceManaGenerator:
        return SpeciesObject;
    case RaceSack:
        return SpeciesObject;
    case RaceCrate:
        return SpeciesObject;
    case RaceGate:
        return SpeciesObject;
    case RaceLockedGate:
        return SpeciesObject;
    case RaceRuneGate:
        return SpeciesObject;
    case RaceStoneGate:
        return SpeciesObject;
    case RaceWall:
        return SpeciesObject;
    case RaceSpiderweb:
        return SpeciesObject;
    case RaceMagicBarrier:
        return SpeciesObject;
    case RaceGrid:
        return SpeciesObject;
    case RaceEmpress:
        return SpeciesHumanoid;
    case RaceArchdruid:
        return SpeciesHumanoid;
    case RaceHalfDemon:
        return SpeciesHumanoid;
    case RaceGhostPreacher:
        return SpeciesUndead;
    case RaceLegendaryPaladin:
        return SpeciesHumanoid;
    case RaceAngel:
        return SpeciesHumanoid;
    case RaceEnhancedOrcMale:
        return SpeciesHumanoid;
    case RaceEnhancedOrcFemale:
        return SpeciesHumanoid;
    case RaceManaMech:
        return SpeciesObject;
    case RaceTreant:
        return SpeciesPlantCreature;
    case RaceFireRunestone:
        return SpeciesObject;
    case RaceWaterRunestone:
        return SpeciesObject;
    case RaceIceRunestone:
        return SpeciesObject;
    case RaceWindRunestone:
        return SpeciesObject;
    case RaceRunePortal:
        return SpeciesObject;
    case RaceMonstrosity:
        return SpeciesUndead;
    case RaceSkullSlater:
        return SpeciesAnimal;
    case RaceTortureRack:
        return SpeciesObject;
    case RaceWitchWagon:
        return SpeciesObject;
    case RaceLegIrons:
        return SpeciesObject;
    case RacePole:
        return SpeciesObject;
    case RaceCeilingChain:
        return SpeciesObject;
    case RaceWraith:
        return SpeciesUndead;
    case RaceManaDeer:
        return SpeciesAnimal;
    case RaceShrineOfTheLight:
        return SpeciesObject;
    case RacePirateBoat:
        return SpeciesObject;
    case RaceBarrel:
        return SpeciesObject;
    case RaceBarrelTnt:
        return SpeciesObject;
    case RaceFootprints:
        return SpeciesObject;
    case RaceHorseFootprints:
        return SpeciesObject;
    case RaceMonsterFootprints:
        return SpeciesObject;
    case RaceWaterBubble:
        return SpeciesObject;
    case RaceSarcophagus:
        return SpeciesObject;
    case RaceChiling:
        return SpeciesPlantCreature;
    case RaceChilingSprout:
        return SpeciesPlantCreature;
    case RaceMagicCrystal:
        return SpeciesObject;
    case RaceHabanero:
        return SpeciesPlantCreature;
    case RaceBloomling:
        return SpeciesPlantCreature;
    case RaceEarthPile:
        return SpeciesObject;
    case RaceCaterpillar:
        return SpeciesAnimal;
    case RaceEarthCaterpillar:
        return SpeciesAnimal;
    case RaceSnail:
        return SpeciesAnimal;
    case RaceDarkJester:
        return SpeciesHumanoid;
    case RaceSwimmingSign:
        return SpeciesObject;
    case RacePedestal:
        return SpeciesObject;
    case RaceSocket:
        return SpeciesObject;
    case RaceBirdStatue:
        return SpeciesObject;
    case RaceMourningStatue:
        return SpeciesObject;
    case RaceClimbingPlant:
        return SpeciesPlant;
    case RaceEagle:
        return SpeciesAnimal;
    case RaceGnome:
        return SpeciesHumanoid;
    case RaceSupplierGnome:
        return SpeciesHumanoid;
    default:
        return SpeciesNone;
    }
}

float Creature::spellPower(bool allWeapons)
{
    float power = 5.0f;
    std::list<Item *> weapons = allWeapons ? equippedWeapons() : attackWeapons();
    for (std::list<Item *>::iterator it = weapons.begin(); it != weapons.end(); ++it) {
        power += (*it)->weaponDamage(this);
    }
    const EntityData::Appearance &appearance = m_entityData.appearance;
    if (appearance.hasFlag(AppearanceFlagTinted)) {
        power *= 1.5f;
    }
    if (appearance.hasFlag(AppearanceFlagEnhanced)) {
        power *= 1.2f;
    }
    if (appearance.hasFlag(AppearanceFlagLarge)) {
        power *= 4.0f;
    }
    Buff *buff = findBuff(BuffAttackflask);
    if (buff) {
        power *= buff->strength;
    }
    if (appearance.appearanceFlags & AppearanceFlagPromotedResident) {
        power *= 1.1f;
    }
    return power;
}

float Creature::staminaCost(int skill)
{
    if (findBuff(BuffNinjutsu)) {
        return 0.0f;
    }
    switch (skill) {
    case AbilityRetreat:
    case AbilitySmash:
    case AbilityThrowPoisonVial:
        return 1.0f;
    case AbilityRangerKick:
        return 0.5f;
    case AbilityIntercept:
    case AbilityDash:
    case AbilityShuriken:
        return 0.25f;
    default:
        return 0.0f;
    }
}

void Creature::stopActions()
{
    m_fallStartHeight = 0;
    m_glideTime = 0;
    m_strikeCombo = 0;
    m_rollTime = 0;
    m_entityData.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    m_entityData.retreat = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    m_entityData.currentAbility = AbilityNone;
    m_entityData.timeSinceAbility = 0.0f;
    m_entityData.hitCombo = 0;
    m_entityData.stunTime = -3000.0f;
    m_entityData.actionLockTime = 0.0f;
    m_entityData.binaryToggles = EntityData::BinaryToggles();
    m_entityData.mp = 0.0f;
    m_entityData.timeSinceHit = 0.0f;
    m_entityData.revivalPenalty = 0;
    m_entityData.finalHandsRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    m_entityData.initialHandsRotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    m_entityData.mediumSpeed = 1.0f;
    m_opponentId = 0;
    m_hitFlash = 0.0f;
    m_stamina = 1.0f;
    m_aggro.clear();
    m_hitCounts.clear();
    m_skillCooldowns.clear();
    m_smoothingTime = 0;
    m_leapDuration = 0;
    m_walkAmplitude = 0.0f;
    m_walkPhase = 0.0f;
    m_strikeStrength = 0.0f;
    m_criticalStrike = 0;
    m_buffs.clear();
}

int Creature::strikeTime(int ability)
{
    if (ability < 0) {
        ability = m_entityData.currentAbility;
    }
    switch (ability) {
    case AbilityNone:
    case AbilityWatchPlayer:
    case AbilityBlock:
    case AbilityChargedShot:
    case AbilitySalvo:
    case AbilityChargedBoomerang:
    case AbilityTeleport:
    case AbilityCrush:
    case AbilitySwirl:
    case AbilitySneak:
    case AbilitySprint:
    case AbilityDash:
    case AbilityRidePet:
    case AbilityHangGlide:
    case AbilitySailBoat:
    case AbilityClimb:
    case AbilityHealingStream:
    case AbilityCenteredHealingStream:
    case AbilityFireBreath:
    case AbilityUseSpecialItem:
    case AbilityWaterBubbles:
    case AbilityFlameRush:
    case AbilityFeedClimbingPlant:
    case AbilityWave:
    case AbilityPetAnimal:
    case AbilityDance:
        return 0;
    case AbilityHeadbutt:
    case AbilityHornCharge:
    case AbilityCallPet:
    case AbilityWarriorJumpAttack:
        return 1000;
    case AbilityRetreat:
    case AbilityShuriken:
        if (findBuff(BuffNinjutsu)) {
            return 250;
        }
        return 500;
    case AbilityDashForward:
    case AbilityDashLeft:
    case AbilityDashRight:
        return 500;
    case AbilitySmash:
        return 100;
    case Ability_4:
    case Ability_5:
    case AbilityFireball:
    case AbilityWaterball:
    case Ability_63:
        return (int)(100.0f / tempo());
    case AbilityRightSlash:
    case AbilityLeftSlash:
    case AbilityShieldAttack:
    case AbilityLeftDaggerPierce:
    case AbilityRightDaggerPierce:
    case AbilityTwoHandedSwing:
        return (int)(200.0f / tempo());
    case AbilityRightStab:
    case AbilityLeftStab:
        return (int)(150.0f / tempo());
    case AbilityBlockJab:
        return 200;
    case AbilityPerforate:
    case AbilityCrossedWeaponSweep:
    case AbilityMonsterChargeStrike:
    case AbilityAmbush:
    case AbilityKick:
    case AbilityRangerKick:
    case AbilityTwoHandedSpin:
    case AbilityTwoHandedSlam:
        return (int)(400.0f / tempo());
    case AbilityLeftSweep:
    case AbilityRightSweep:
    case AbilityOverheadChop:
        return (int)(250.0f / tempo());
    case AbilityShellBlock:
    case AbilityShellSpin:
    case AbilityRestHeal:
        return 3000;
    case AbilityCyclone:
        return 20000;
    case AbilityAssassinate:
        return (int)(5000.0f / tempo());
    case AbilityWraithAttack:
        return 5000;
    case AbilityQuake:
        return 6000;
    case AbilityFireSwirl:
    case AbilityWaterSwirl:
    case AbilityScream:
    case AbilityLunge:
        return 600;
    case AbilityRockFist:
        return (int)(m_strikeStrength * 1000.0f);
    case AbilityWaterSalvo:
    case Ability_48:
    case AbilitySalvoRelease:
    case AbilityFireBeam:
    case AbilityWaterBeam:
        return (int)(500.0f / tempo());
    case AbilitySupportRay:
    case AbilityFireRay:
    case AbilityWaterRay:
        return 2000;
    case AbilityFireMissiles:
        return (int)(1000.0f / tempo());
    case AbilityFireVortex:
    case AbilityWaterVortex:
        return 1200;
    case AbilityThrowBoomerang:
    case AbilityHeal:
        return (int)(1200.0f / tempo());
    case AbilityShoot:
    case AbilityChargedShotRelease:
        return (int)(50.0f / tempo());
    case AbilitySwirlStrike:
    case AbilityMonsterFirebolt:
    case AbilityFirebolt:
    case AbilityBraceletFirebolt:
    case AbilityWaterbolt:
    case AbilityBraceletWaterbolt:
    case AbilityMonsterWaterbolt:
    case AbilityClawStrike:
    case AbilityBlockStrike:
    case AbilityBloomlingAttack:
        return (int)(300.0f / tempo());
    default:
        return (int)(300.0f / tempo());
    }
}

float Creature::swimmingSpeed()
{
    if (!(m_entityData.appearance.appearanceFlags & (AppearanceFlagTinted | AppearanceFlagLarge))) {
        if (m_entityData.hostilityType != HostilityPlayer) {
            return (1.0f - 1.0f / (m_entityData.level * 10 / 20.0f + 1.0f) + 1.0f) * 0.5f;
        }
        return (1.0f - 1.0f / (m_artifactSkills[ArtifactEffect_SwimmingSpeed] / 20.0f + 1.0f) + 1.0f) * 0.5f;
    }
    return 1.0f;
}

float Creature::tempo()
{
    float tempo = 1.0f;
    if (m_entityData.classType == ClassWarrior && m_entityData.specialization == SpecializationBerserker) {
        tempo = std::min((float)m_entityData.hitCombo / maxCombo(), 1.0f) * 0.5f + 1.0f;
    }
    if (m_entityData.classType == ClassMage && m_entityData.specialization == SpecializationWater) {
        tempo += std::min((float)m_entityData.hitCombo / maxCombo(), 1.0f);
    }
    if (m_entityData.classType == ClassRanger && m_entityData.specialization == SpecializationSniper) {
        tempo += std::min((float)m_entityData.hitCombo / maxCombo(), 1.0f) * 0.5f;
    }
    Equipment &equipment = m_entityData.equipment;
    if (equipment.weaponLeft.category == ItemCategoryWeapon) {
        tempo += equipment.weaponLeft.tempo(this);
    }
    if (equipment.weaponRight.category == ItemCategoryWeapon) {
        tempo += equipment.weaponRight.tempo(this);
    }
    if (equipment.chest.category == ItemCategoryChest) {
        tempo += equipment.chest.tempo(this);
    }
    if (equipment.feet.category == ItemCategoryFeet) {
        tempo += equipment.feet.tempo(this);
    }
    if (equipment.hands.category == ItemCategoryHands) {
        tempo += equipment.hands.tempo(this);
    }
    if (equipment.shoulder.category == ItemCategoryShoulder) {
        tempo += equipment.shoulder.tempo(this);
    }
    if (equipment.neck.category == ItemCategoryNeck) {
        tempo += equipment.neck.tempo(this);
    }
    if (equipment.ringLeft.category == ItemCategoryRing) {
        tempo += equipment.ringLeft.tempo(this);
    }
    if (equipment.ringRight.category == ItemCategoryRing) {
        tempo += equipment.ringRight.tempo(this);
    }
    if (findBuff(BuffSwiftness)) {
        tempo *= 2.0f;
    }
    const EntityData::Appearance &appearance = m_entityData.appearance;
    if (appearance.hasFlag(AppearanceFlagTinted)) {
        tempo *= 0.75f;
    }
    if (appearance.hasFlag(AppearanceFlagLarge)) {
        tempo *= 0.6f;
    }
    if (appearance.appearanceFlags & AppearanceFlagPromotedResident) {
        tempo *= 1.1f;
    }
    return tempo;
}

int Creature::totalXP()
{
    int total = 0;
    for (int level = 1; level < m_entityData.level; level++) {
        total += (int)(pow(2.0, (level - 1) * 0.5f) * 10.0);
    }
    return total + m_entityData.xp;
}

void Creature::updateLevelAndSkills()
{
    for (int &skill : m_artifactSkills) {
        skill = AbilityNone;
    }
    m_entityData.level = 1;
    if ((int)m_inventory.m_inventoryTabs.size() > InventoryTabArtifacts) {
        for (ItemStack &stack : m_inventory.m_inventoryTabs[InventoryTabArtifacts]) {
            if (stack.item.category == ItemCategoryArtifact) {
                m_entityData.level++;
                int type = stack.item.artifactType();
                if (type >= ArtifactEffect_ClimbingSpeed && type <= ArtifactEffect_LampRadius) {
                    m_artifactSkills[type]++;
                }
            }
        }
    }
    mod::addSkillEffects(this, m_artifactSkills.data(), (int)m_artifactSkills.size());
}

// Whether a skill's cooldown is kept when the creature's abilities are reset: never.
bool Creature::keepsCooldown(int skill)
{
    return false;
}

}
