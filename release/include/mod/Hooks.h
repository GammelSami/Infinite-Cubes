#pragma once

namespace cube {
class Creature;
class Game;
class World;
}

// The places where the release's code asks the mod. Each returns false (or does nothing) unless the alpha classes are
// on and the creature is a player whose skills classes.json tunes, so the release's own values apply otherwise.
namespace mod {

// Creature::cooldown and Creature::manaCost: the skill's "cooldown" and "mana" values at the player's points.
bool cooldown(cube::Creature *creature, int ability, int &milliseconds);
bool manaCost(cube::Creature *creature, int ability, float &cost);
// World::applyBuffSkill: the alpha's buffs with the skill's "duration" and "strength" values.
bool applyBuffSkill(cube::World *world, cube::Creature *creature, int ability);
// Creature::updateLevelAndSkills: the general skills' points as artifacts of their effects.
void addSkillEffects(cube::Creature *creature, int *artifactSkills, int effectCount);
// World::onKill: the alpha's experience for the local player's kills.
void onKill(cube::World *world, cube::Creature *killer, cube::Creature *victim);
// Game::controlCharacter, in place of the release's class skills (Shift, R and the dodge skills): the keys 1 to 3 start
// the skills of the player's class kit that have points.
void controlBar(cube::Game *game);

}
