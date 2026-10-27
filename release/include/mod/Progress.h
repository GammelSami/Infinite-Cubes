#pragma once
#include <map>
#include <string>

namespace cube {
class BytesIO;
class Creature;
}

namespace mod {

// A character's alpha level, experience and skill points by skill id (PlayerData::modProgress). The release's own level
// counts the artifacts and stays theirs. Saved after the release's character data, which the release ignores.
struct Progress {
    int level = 1;
    int xp = 0;
    int levelUps = 0; // levels gained and not yet announced (AlphaHud)
    std::map<std::string, int> points;

    int pointsOf(const std::string &skill) const;
    int spent() const;
    void write(cube::BytesIO &stream) const;
    // Reads what write wrote, if the stream holds it at its position; otherwise leaves the progress empty.
    void read(cube::BytesIO &stream);
};

// The points a player has to spend: skillPointsPerLevel per alpha level after the first, one per manaCubesPerSkillPoint
// mana cubes, minus those spent.
int skillPoints(cube::Creature *player);
int totalSkillPoints(cube::Creature *player);
// The points of the player's skill with this ability (in its class kit or the general skills); -1 when the ability is
// not one of the player's skills, or the creature is no player with progress.
int abilityPoints(cube::Creature *creature, int ability);

// The alpha's experience for the next level (50 rising towards 1050 by default), and the experience for a kill.
int maxExperience(int level);
int killExperience(cube::Creature *victim);
// Adds experience and raises the alpha level while it suffices.
void addExperience(cube::Creature *player, int experience);

}
