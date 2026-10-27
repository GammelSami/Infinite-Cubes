#include "mod/Hooks.h"
#include "mod/ClassData.h"
#include "mod/Progress.h"
#include "cube/Controls.h"
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/ServerUpdateIds.h"
#include "cube/ServerUpdates.h"
#include "cube/World.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace mod {

namespace {

// The player's skill with this ability and its points, if classes.json tunes it.
const Skill *tunedSkill(cube::Creature *creature, int ability, int &points, std::shared_ptr<const ClassData> &data)
{
    if (!creature->m_playerData || !ability)
        return nullptr;
    data = classData();
    if (!data->alphaClasses)
        return nullptr;
    const Skill *skill =
        data->skillForAbility(creature->m_entityData.classType, creature->m_entityData.specialization, ability);
    if (skill)
        points = creature->m_playerData->modProgress.pointsOf(skill->id);
    return skill;
}

// A value of the skill at the points, or the fallback when the skill does not set it.
float valueOr(const Skill &skill, const char *name, int points, float fallback)
{
    const Value *value = skill.value(name);
    return value ? value->at(points) : fallback;
}

void sendBuff(cube::World *world, cube::Creature *creature, const cube::Creature::Buff &buff)
{
    creature->setBuff(buff, true);
    cube::ServerUpdates::BuffPacket packet;
    packet.sourceId = creature->m_id;
    packet.targetId = creature->m_id;
    packet.buff = buff;
    packet.replace = 1;
    world->m_serverUpdates.m_buffPacketList.push_back(packet);
}

// Starts a skill of the bar the way the alpha did (GameController::update): buffs at once, the leaps and dashes with
// their motion, the others as the creature's current ability.
void startSkill(cube::Game *game, cube::Creature *player, const Skill &skill, int points)
{
    cube::Creature::EntityData &entity = player->m_entityData;
    int ability = skill.ability;
    switch (ability) {
    case cube::AbilityBulwark:
    case cube::AbilityWarFrenzy:
    case cube::AbilityManaShield:
    case cube::AbilitySwiftness:
    case cube::AbilityCamouflage:
    case cube::AbilityNinjutsu:
    case cube::AbilityFlameRush:
        game->m_world->applyBuffSkill(player, ability);
        if (ability == cube::AbilityBulwark)
            game->playSound(cube::SoundBulwark, entity.position, 1.0f, 1.0f, true);
        else if (ability == cube::AbilityManaShield)
            game->playSound(cube::SoundManashield, entity.position, 1.0f, 1.0f, true);
        else if (ability == cube::AbilitySwiftness)
            game->playSound(cube::SoundUpgrade2, entity.position, 1.0f, 1.0f, true);
        return;
    case cube::AbilitySmash: {
        // A leap along twice the aim, at most "speed" blocks a second, "rise" up.
        float speed = valueOr(skill, "speed", points, 50.0f);
        entity.currentAbility = (unsigned char)ability;
        entity.velocity = entity.attackRotation * 2.0f;
        player->m_leapDuration = entity.velocity.length() / speed * 1000.0f;
        if (entity.velocity.squaredLength() > speed * speed) {
            entity.velocity.normalize();
            entity.velocity *= speed;
        }
        entity.velocity[2] = valueOr(skill, "rise", points, 15.0f);
        break;
    }
    case cube::AbilityIntercept: {
        // A dash along the aim at "speed" blocks a second, at most 5 up.
        float speed = valueOr(skill, "speed", points, 50.0f);
        entity.currentAbility = (unsigned char)ability;
        entity.velocity = entity.attackRotation;
        player->m_leapDuration = entity.velocity.length() / speed * 1000.0f;
        if (entity.velocity.squaredLength() > 0.0f) {
            entity.velocity.normalize();
            entity.velocity *= speed;
            if (entity.velocity[2] > 5.0f)
                entity.velocity[2] = 5.0f;
        }
        break;
    }
    case cube::AbilityRetreat: {
        // A jump back from the aim.
        entity.currentAbility = (unsigned char)ability;
        entity.retreat = -entity.attackRotation;
        entity.retreat[2] = 0.0f;
        if (entity.retreat.squaredLength() > 0.0f) {
            entity.retreat.normalize();
            entity.retreat *= valueOr(skill, "speed", points, 20.0f);
            entity.retreat[2] = valueOr(skill, "rise", points, 5.0f);
            entity.velocity[0] += entity.retreat[0];
            entity.velocity[1] += entity.retreat[1];
        }
        entity.timeSinceAbility = 0.0f;
        entity.rollTime = 600.0f;
        player->m_skillCooldowns[ability] = (float)player->cooldown(ability);
        return;
    }
    case cube::AbilityShuriken: {
        // A roll away from the aim, throwing the stars.
        float speed = valueOr(skill, "speed", points, 30.0f);
        entity.currentAbility = (unsigned char)ability;
        entity.velocity = -entity.attackRotation;
        entity.velocity[2] = 0.0f;
        if (entity.velocity.squaredLength() > 0.0f) {
            entity.velocity.normalize();
            entity.velocity *= speed;
            entity.velocity[2] = valueOr(skill, "rise", points, 20.0f);
        }
        entity.timeSinceAbility = 0.0f;
        entity.rollTime = 600.0f;
        player->m_skillCooldowns[ability] = (float)player->cooldown(ability);
        return;
    }
    case cube::AbilityHealingStream:
        if (entity.timeSinceAbility < player->attackDuration())
            return;
        entity.currentAbility = (unsigned char)ability;
        entity.focusId = player->m_id;
        break;
    default:
        if (entity.mp < player->manaCost(ability, -1))
            return;
        entity.currentAbility = (unsigned char)ability;
        break;
    }
    entity.timeSinceAbility = 0.0f;
    entity.rollTime = 0.0f;
    player->m_skillCooldowns[ability] = (float)player->cooldown(ability);
}

}

bool cooldown(cube::Creature *creature, int ability, int &milliseconds)
{
    std::shared_ptr<const ClassData> data;
    int points = 0;
    const Skill *skill = tunedSkill(creature, ability, points, data);
    const Value *value = skill ? skill->value("cooldown") : nullptr;
    if (!value)
        return false;
    milliseconds = (std::max)(0, (int)value->at(points));
    return true;
}

bool manaCost(cube::Creature *creature, int ability, float &cost)
{
    std::shared_ptr<const ClassData> data;
    int points = 0;
    const Skill *skill = tunedSkill(creature, ability, points, data);
    const Value *value = skill ? skill->value("mana") : nullptr;
    if (!value)
        return false;
    cost = (std::max)(0.0f, value->at(points));
    return true;
}

bool applyBuffSkill(cube::World *world, cube::Creature *creature, int ability)
{
    std::shared_ptr<const ClassData> data;
    int points = 0;
    const Skill *skill = tunedSkill(creature, ability, points, data);
    if (!skill)
        return false;
    cube::Creature::Buff buff;
    buff.sourceId = creature->m_id;
    switch (ability) {
    case cube::AbilityBulwark:
        buff.type = cube::BuffBulwark;
        buff.timeRemaining = valueOr(*skill, "duration", points, 10000.0f);
        buff.strength = valueOr(*skill, "strength", points, 0.75f);
        if (creature->m_entityData.stunTime > 0.0f)
            creature->m_entityData.stunTime = 0.0f;
        break;
    case cube::AbilityManaShield:
        // The strength is a factor of the spell power.
        buff.type = cube::BuffManashield;
        buff.timeRemaining = valueOr(*skill, "duration", points, 30000.0f);
        buff.strength = creature->spellPower(false) * valueOr(*skill, "strength", points, 2.0f);
        break;
    case cube::AbilitySwiftness:
        buff.type = cube::BuffSwiftness;
        buff.timeRemaining = valueOr(*skill, "duration", points, 10000.0f);
        buff.strength = valueOr(*skill, "strength", points, 1.0f);
        break;
    case cube::AbilityCamouflage:
        buff.type = cube::BuffCamouflage;
        buff.timeRemaining = valueOr(*skill, "duration", points, 8000.0f);
        break;
    case cube::AbilityWarFrenzy:
        buff.type = cube::BuffWarfrenzy;
        buff.timeRemaining = valueOr(*skill, "duration", points, 10000.0f);
        break;
    default:
        return false;
    }
    sendBuff(world, creature, buff);
    creature->m_skillCooldowns[ability] = (float)creature->cooldown(ability);
    return true;
}

void addSkillEffects(cube::Creature *creature, int *artifactSkills, int effectCount)
{
    if (!creature->m_playerData)
        return;
    std::shared_ptr<const ClassData> data = classData();
    if (!data->alphaClasses)
        return;
    for (const TreeEntry &entry : data->generalTree) {
        const Skill *skill = data->skill(entry.skill);
        if (!skill || skill->effect < 0 || skill->effect >= effectCount)
            continue;
        int points = creature->m_playerData->modProgress.pointsOf(skill->id);
        artifactSkills[skill->effect] += (int)(points * valueOr(*skill, "artifactsPerPoint", points, 2.0f));
    }
}

void onKill(cube::World *world, cube::Creature *killer, cube::Creature *victim)
{
    if (!killer || !victim || killer != world->m_localCreature || !killer->m_playerData || !alphaClasses())
        return;
    if (victim->m_entityData.hostilityType != cube::HostilityEnemy)
        return;
    addExperience(killer, killExperience(victim));
}

void controlBar(cube::Game *game)
{
    std::shared_ptr<const ClassData> data = classData();
    cube::Creature *player = game->m_world->m_localCreature;
    if (!data->alphaClasses || !player || !player->m_playerData)
        return;
    const ClassKit *kit = data->kit(player->m_entityData.classType, player->m_entityData.specialization);
    if (!kit)
        return;
    // For test runs (like CUBE_TEST_SCRIPT): a character without skill points gets this many in each of its skills.
    // CUBE_MOD_TEST_LEVEL raises a lower level to it.
    static const char *testPoints = getenv("CUBE_MOD_TEST_POINTS");
    if (testPoints && player->m_playerData->modProgress.points.empty()) {
        for (const TreeEntry &entry : kit->tree)
            player->m_playerData->modProgress.points[entry.skill] = atoi(testPoints);
    }
    static const char *testLevel = getenv("CUBE_MOD_TEST_LEVEL");
    if (testLevel && player->m_playerData->modProgress.level < atoi(testLevel))
        player->m_playerData->modProgress.level = atoi(testLevel);
    static const int Keys[] = {cube::Controls::ButtonSkill1, cube::Controls::ButtonSkill2,
                               cube::Controls::ButtonSkill3};
    for (size_t i = 0; i < kit->bar.size() && i < sizeof(Keys) / sizeof(Keys[0]); i++) {
        if (!game->m_controlStates[Keys[i]])
            continue;
        const Skill *skill = data->skill(kit->bar[i]);
        if (!skill || !skill->ability)
            continue;
        int points = player->m_playerData->modProgress.pointsOf(skill->id);
        if (points <= 0 || !player->canUseSkill(skill->ability, true))
            continue;
        startSkill(game, player, *skill, points);
    }
}

}
