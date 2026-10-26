// Creature-family expansion and random race selection.
#pragma once
#include <vector>

namespace cube {
void appendRaceVariants(int race, std::vector<int> &variants);
int randomCaster();
int randomCreature();
int randomDemonBoss();
int randomDemonCreature();
int randomFighterRace();
int randomFolkRace(int biome);
int randomInsect();
int randomMobRace();
int randomPeacefulRace();
int randomPlantCreature();
int randomRace(int race);
int randomUndead();
int randomUndeadBoss();
}
