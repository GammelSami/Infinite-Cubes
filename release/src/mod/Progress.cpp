#include "mod/Progress.h"
#include "mod/ClassData.h"
#include "cube/BytesIO.h"
#include "cube/Creature.h"
#include <algorithm>

namespace mod {

namespace {

// Marks the mod's block after a saved character: "CMOD" and the block's version.
const int ProgressMagic = 0x444F4D43;
const int ProgressVersion = 2; // 2 added the level and the experience

}

int Progress::pointsOf(const std::string &skill) const
{
    std::map<std::string, int>::const_iterator it = points.find(skill);
    return it == points.end() ? 0 : it->second;
}

int Progress::spent() const
{
    int total = 0;
    for (const std::pair<const std::string, int> &entry : points)
        total += entry.second;
    return total;
}

void Progress::write(cube::BytesIO &stream) const
{
    stream.write(ProgressMagic);
    stream.write(ProgressVersion);
    stream.write(level);
    stream.write(xp);
    stream.write((int)points.size());
    for (const std::pair<const std::string, int> &entry : points) {
        stream.write((int)entry.first.size());
        stream.write(entry.first.data(), (int)entry.first.size());
        stream.write(entry.second);
    }
}

void Progress::read(cube::BytesIO &stream)
{
    *this = Progress();
    int magic = 0;
    stream.read(magic);
    int version = 0;
    stream.read(version);
    if (magic != ProgressMagic || version < 1)
        return;
    if (version >= 2) {
        stream.read(level);
        stream.read(xp);
        level = (std::max)(1, level);
    }
    int count = 0;
    stream.read(count);
    for (int i = 0; i < count && !stream.atEnd(); i++) {
        int length = 0;
        stream.read(length);
        if (length < 0 || length > 256)
            return;
        std::string skill(length, '\0');
        stream.read(&skill[0], length);
        int value = 0;
        stream.read(value);
        points[skill] = value;
    }
}

int totalSkillPoints(cube::Creature *player)
{
    const Progression &progression = classData()->progression;
    if (!player->m_playerData)
        return 0;
    int points = (std::max)(0, player->m_playerData->modProgress.level - 1) * progression.skillPointsPerLevel;
    if (progression.manaCubesPerSkillPoint > 0)
        points += player->m_inventory.m_manaCubes / progression.manaCubesPerSkillPoint;
    return points;
}

int skillPoints(cube::Creature *player)
{
    if (!player->m_playerData)
        return 0;
    return totalSkillPoints(player) - player->m_playerData->modProgress.spent();
}

int abilityPoints(cube::Creature *creature, int ability)
{
    if (!creature->m_playerData || !ability)
        return -1;
    std::shared_ptr<const ClassData> data = classData();
    const Skill *skill =
        data->skillForAbility(creature->m_entityData.classType, creature->m_entityData.specialization, ability);
    if (!skill)
        return -1;
    return creature->m_playerData->modProgress.pointsOf(skill->id);
}

int maxExperience(int level)
{
    const Progression &progression = classData()->progression;
    float bonus = 1.0f - 1.0f / ((level - 1.0f) * 0.05f + 1.0f);
    return (int)(bonus * progression.levelXPScale + progression.levelXPBase);
}

int killExperience(cube::Creature *victim)
{
    const Progression &progression = classData()->progression;
    unsigned int flags = victim->m_entityData.appearance.appearanceFlags;
    if (flags & cube::AppearanceFlagSummoned)
        return 0;
    // The release has no creature levels: a victim is worth its life against the player's base life (100).
    float xp = progression.killXP * (std::min)((std::max)(victim->maxHP() / 100.0f, 0.2f), 5.0f);
    if (flags & cube::AppearanceFlagLarge)
        xp *= progression.bossXPFactor;
    if (flags & cube::AppearanceFlagTinted)
        xp *= progression.tintedXPFactor;
    return (std::max)(1, (int)xp);
}

void addExperience(cube::Creature *player, int experience)
{
    if (!player->m_playerData)
        return;
    Progress &progress = player->m_playerData->modProgress;
    progress.xp += experience;
    while (progress.xp >= maxExperience(progress.level)) {
        progress.xp -= maxExperience(progress.level);
        progress.level++;
        progress.levelUps++;
        player->m_entityData.hp = player->maxHP();
    }
}

}
