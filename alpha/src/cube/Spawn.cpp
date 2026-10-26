// Spawn describes a creature to be created by a zone.
#include "cube/Spawn.h"

#include "cube/Behavior.h"
#include "cube/RaceIds.h"
#include "cube/DefaultBlocks.h"
#include "cube/World.h"

namespace cube {

// Spawn defaults to a proximity radius of 200, hostility 1, race 4 and level 1, with no behavior or schedule.
Spawn::Spawn()
    : spawnRadius(200.0f),
      hostilityType(HostilityEnemy),
      race(RaceGoblinMale),
      classType(ClassNone),
      specialization(SpecializationFirst),
      level(1),
      respawnTimeRemaining(0),
      deathDay(0),
      activeFrom(0),
      activeUntil(0),
      creatureId(0),
      unknown050(0),
      rotation(0.0f),
      rarity(ItemRarityWhite),
      speechExpression(0),
      questZone(-1, -1, 0),
      unused06c(-1),
      unused070(0),
      m_behavior(0),
      summonAppearanceMode(SummonSelectedRace),
      unused10c8(-1),
      generationKey(0),
      missionResident(false)
{
    name[0] = 0;
}

Spawn::~Spawn()
{
    if (m_behavior)
        delete m_behavior;
}

}
