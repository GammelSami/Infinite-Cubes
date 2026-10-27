#include "mod/ClassData.h"
#include "cube/CreatureIds.h"
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>

namespace mod {

namespace {

const char *const DataPath = "mod/classes.json";
const auto CheckInterval = std::chrono::milliseconds(500);

// The abilities classes.json can name: the alpha's class skills and the release's, which can be mixed into kits.
#define ABILITY(name) {#name, cube::Ability##name}
const std::pair<const char *, int> Abilities[] = {
    ABILITY(Smash),         ABILITY(Cyclone),       ABILITY(WarFrenzy),     ABILITY(Bulwark),
    ABILITY(RangerKick),    ABILITY(Retreat),       ABILITY(Swiftness),     ABILITY(Aim),
    ABILITY(HealingStream), ABILITY(FireExplosion), ABILITY(ManaShield),    ABILITY(Teleport),
    ABILITY(Intercept),     ABILITY(Sneak),         ABILITY(Camouflage),    ABILITY(Shuriken),
    ABILITY(HeroicShout),   ABILITY(FireMissiles),  ABILITY(WaterBubbles),  ABILITY(ShadowClone),
    ABILITY(QuicksandTrap), ABILITY(Ninjutsu),      ABILITY(FlameRush),     ABILITY(ThrowPoisonVial),
};
#undef ABILITY

// The release's artifact effects (cube::ArtifactEffect_*), which the general skills strengthen.
const char *const Effects[] = {"ClimbingSpeed",    "SwimmingSpeed", "DivingStamina", "RidingSpeed",
                               "HangGlidingSpeed", "SailingSpeed",  "LampRadius"};

int effectByName(const std::string &name)
{
    for (int i = 0; i < (int)(sizeof(Effects) / sizeof(Effects[0])); i++) {
        if (name == Effects[i])
            return i;
    }
    return -1;
}

int classByName(const std::string &name)
{
    if (name == "Warrior")
        return cube::ClassWarrior;
    if (name == "Ranger")
        return cube::ClassRanger;
    if (name == "Mage")
        return cube::ClassMage;
    if (name == "Rogue")
        return cube::ClassRogue;
    return 0;
}

// The specializations are numbered within their class: the first of each pair is 0.
int specializationByName(const std::string &name)
{
    if (name == "Guardian" || name == "Scout" || name == "Water" || name == "Ninja")
        return 1;
    return 0;
}

std::vector<TreeEntry> readTree(const nlohmann::json &tree)
{
    std::vector<TreeEntry> entries;
    for (const nlohmann::json &item : tree) {
        TreeEntry entry;
        entry.skill = item.value("skill", "");
        entry.requires = item.value("requires", "");
        entry.requiredPoints = item.value("points", entry.requires.empty() ? 0 : 5);
        entries.push_back(entry);
    }
    return entries;
}

std::shared_ptr<const ClassData> parse(const nlohmann::json &root)
{
    std::shared_ptr<ClassData> data = std::make_shared<ClassData>();
    data->alphaClasses = root.value("alphaClasses", true);
    if (root.contains("progression")) {
        const nlohmann::json &p = root["progression"];
        Progression &progression = data->progression;
        progression.skillPointsPerLevel = p.value("skillPointsPerLevel", progression.skillPointsPerLevel);
        progression.manaCubesPerSkillPoint = p.value("manaCubesPerSkillPoint", progression.manaCubesPerSkillPoint);
        progression.killXP = p.value("killXP", progression.killXP);
        progression.bossXPFactor = p.value("bossXPFactor", progression.bossXPFactor);
        progression.tintedXPFactor = p.value("tintedXPFactor", progression.tintedXPFactor);
        progression.levelXPBase = p.value("levelXPBase", progression.levelXPBase);
        progression.levelXPScale = p.value("levelXPScale", progression.levelXPScale);
        progression.refundCost = p.value("refundCost", progression.refundCost);
        progression.specializationCost = p.value("specializationCost", progression.specializationCost);
        progression.requireTrainer = p.value("requireTrainer", progression.requireTrainer);
    }
    // value() returns a copy: items() of a temporary would outlive it.
    const nlohmann::json skills = root.value("skills", nlohmann::json::object());
    for (auto &entry : skills.items()) {
        const nlohmann::json &item = entry.value();
        Skill skill;
        skill.id = entry.key();
        skill.name = item.value("name", skill.id);
        skill.description = item.value("description", "");
        skill.icon = item.value("icon", "");
        skill.abilityName = item.value("ability", "");
        skill.ability = abilityByName(skill.abilityName);
        if (!skill.abilityName.empty() && !skill.ability)
            std::cout << "mod: skill " << skill.id << ": unknown ability " << skill.abilityName << std::endl;
        skill.maxPoints = item.value("maxPoints", 0);
        skill.effectName = item.value("effect", "");
        skill.effect = effectByName(skill.effectName);
        if (!skill.effectName.empty() && skill.effect < 0)
            std::cout << "mod: skill " << skill.id << ": unknown effect " << skill.effectName << std::endl;
        const nlohmann::json values = item.value("values", nlohmann::json::object());
        for (auto &value : values.items()) {
            Value tuned;
            tuned.base = value.value().value("base", 0.0f);
            tuned.perBonus = value.value().value("perBonus", 0.0f);
            skill.values[value.key()] = tuned;
        }
        data->skills[skill.id] = skill;
    }
    for (const nlohmann::json &item : root.value("classes", nlohmann::json::array())) {
        ClassKit kit;
        kit.classType = classByName(item.value("class", ""));
        kit.specialization = specializationByName(item.value("specialization", ""));
        kit.bar = item.value("bar", std::vector<std::string>());
        kit.tree = readTree(item.value("tree", nlohmann::json::array()));
        data->kits.push_back(kit);
    }
    data->generalTree = readTree(root.value("generalSkills", nlohmann::json::array()));
    return data;
}

struct Loader {
    std::mutex mutex;
    std::shared_ptr<const ClassData> data = std::make_shared<ClassData>();
    std::filesystem::file_time_type loadedTime;
    bool loaded = false;
    std::chrono::steady_clock::time_point lastCheck;

    void check()
    {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (loaded && now - lastCheck < CheckInterval)
            return;
        lastCheck = now;
        std::error_code error;
        std::filesystem::file_time_type time = std::filesystem::last_write_time(DataPath, error);
        if (error || (loaded && time == loadedTime))
            return;
        loaded = true;
        loadedTime = time;
        try {
            std::ifstream file(DataPath);
            data = parse(nlohmann::json::parse(file, nullptr, true, true));
            std::cout << "mod: read " << DataPath << " (" << data->skills.size() << " skills, " << data->kits.size()
                      << " classes)" << std::endl;
        } catch (const std::exception &exception) {
            // A file saved half-way or with a mistake keeps the previous data.
            std::cout << "mod: " << DataPath << ": " << exception.what() << std::endl;
        }
    }
};

Loader &loader()
{
    static Loader instance;
    return instance;
}

}

float Value::at(int points) const
{
    return base + perBonus * skillBonus(points);
}

const Value *Skill::value(const std::string &name) const
{
    std::map<std::string, Value>::const_iterator it = values.find(name);
    return it == values.end() ? nullptr : &it->second;
}

const Skill *ClassData::skill(const std::string &id) const
{
    std::map<std::string, Skill>::const_iterator it = skills.find(id);
    return it == skills.end() ? nullptr : &it->second;
}

const ClassKit *ClassData::kit(int classType, int specialization) const
{
    for (const ClassKit &kit : kits) {
        if (kit.classType == classType && kit.specialization == specialization)
            return &kit;
    }
    return nullptr;
}

const Skill *ClassData::skillForAbility(int classType, int specialization, int ability) const
{
    if (const ClassKit *classKit = kit(classType, specialization)) {
        for (const TreeEntry &entry : classKit->tree) {
            const Skill *found = skill(entry.skill);
            if (found && found->ability == ability)
                return found;
        }
    }
    for (const TreeEntry &entry : generalTree) {
        const Skill *found = skill(entry.skill);
        if (found && found->ability == ability)
            return found;
    }
    return nullptr;
}

std::shared_ptr<const ClassData> classData()
{
    Loader &instance = loader();
    std::lock_guard<std::mutex> lock(instance.mutex);
    instance.check();
    return instance.data;
}

bool alphaClasses()
{
    return classData()->alphaClasses;
}

float skillBonus(int points)
{
    if (points < 1)
        return 0.0f;
    return 1.0f - 1.0f / (points * 0.1f + 1.0f);
}

int abilityByName(const std::string &name)
{
    for (const std::pair<const char *, int> &ability : Abilities) {
        if (name == ability.first)
            return ability.second;
    }
    return 0;
}

std::string abilityName(int ability)
{
    for (const std::pair<const char *, int> &entry : Abilities) {
        if (entry.second == ability)
            return entry.first;
    }
    return "";
}

}
