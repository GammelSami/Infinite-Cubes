#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

// The mod's classes and skills, read from mod/classes.json (in the game's working directory). The file is read again
// when it changes, so values can be tuned while the game runs. Without the file the game keeps the release's classes.
namespace mod {

// A tuned value: base + perBonus * skillBonus(points), the form of the alpha's skill formulas.
struct Value {
    float base = 0.0f;
    float perBonus = 0.0f;
    float at(int points) const;
};

// A skill: an ability of the game (by its name in cube::Ability*, empty for the general skills) and its values. The
// values a skill uses depend on its ability: "cooldown" (ms), "mana" (part of the mana bar), "duration" (ms),
// "strength", "speed" and "rise" (blocks per second), as classes.json documents.
struct Skill {
    std::string id;
    std::string name;
    std::string description;
    std::string icon;
    std::string abilityName;
    int ability = 0; // cube::Ability*, 0 for none
    int maxPoints = 0; // 0: no limit
    std::map<std::string, Value> values;
    // The value, or nullptr when the skill does not set it (the game's own value applies).
    const Value *value(const std::string &name) const;
};

// A place in a skill tree: the skill and the points another skill needs before it can be learned.
struct TreeEntry {
    std::string skill;
    std::string requires;
    int requiredPoints = 0;
};

// A class and specialization: the skills of the keys 1 to 3 and the class part of the skill tree.
struct ClassKit {
    int classType = 0;
    int specialization = 0;
    std::vector<std::string> bar;
    std::vector<TreeEntry> tree;
};

struct Progression {
    int skillPointsPerLevel = 2;
    int manaCubesPerSkillPoint = 4;
    float killXP = 20.0f;
    float bossXPFactor = 20.0f;
    float tintedXPFactor = 10.0f;
    float levelXPBase = 50.0f;
    float levelXPScale = 1000.0f;
    // The class trainer's prices in gold, times the level: a skill point given back, a change of specialization.
    int refundCost = 10;
    int specializationCost = 100;
    // Whether giving points back and changing the specialization need a trainer of the class nearby.
    bool requireTrainer = true;
};

struct ClassData {
    bool alphaClasses = false;
    Progression progression;
    std::map<std::string, Skill> skills;
    std::vector<ClassKit> kits;
    std::vector<TreeEntry> generalTree;

    const Skill *skill(const std::string &id) const;
    const ClassKit *kit(int classType, int specialization) const;
    // The skill of a tree (class kit or general) whose ability is the one given, for this class.
    const Skill *skillForAbility(int classType, int specialization, int ability) const;
};

// The current data; checks the file for changes at most twice a second. Never null.
std::shared_ptr<const ClassData> classData();
// Whether the alpha classes replace the release's class skills.
bool alphaClasses();

// The alpha's skill bonus: 0 without points, approaching 1 (1 - 1 / (points / 10 + 1)).
float skillBonus(int points);

// cube::Ability* by name ("Smash"), 0 if unknown; and the name of an ability.
int abilityByName(const std::string &name);
std::string abilityName(int ability);

}
