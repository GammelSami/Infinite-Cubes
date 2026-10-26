#pragma once
#include "cube/ItemIds.h"
#include "plasma/Vector.h"

namespace cube {
// MissionState::kind, the mission's kind: named after the dictionary keys objective:<kind> that
// GameController::missionText selects for each value (6 has no text).
enum MissionKind {
    MissionNone = 0,
    MissionMonster = 1,
    MissionRareBoss = 2,
    MissionVillageMonster = 3,
    MissionVillageRareBoss = 4,
    MissionDungeon = 5,
    MissionInvasion = 7,
    MissionSceneryInvasion = 8,
    MissionVillageInvasion = 9,
    MissionCaveInvasion = 10,
    MissionRiverInvasion = 11,
    MissionGang = 12,
    MissionGangBoss = 13
};

// Mission progress is idle until the player enters its area, then active until completion.
enum MissionProgress {
    MissionNotStarted = 0,
    MissionInProgress = 1,
    MissionCompleted = 2
};

// Saved region mission state, serialized under the mission key. The constructor zeroes its fields; serialization skips the leading unused word.
struct MissionState {
    MissionState()
        : unused00(0),
          seed(0),
          kind(MissionNone),
          race(0),
          level(0),
          rarity(ItemRarityWhite),
          progress(MissionNotStarted),
          killCount(0),
          targetKillCount(0),
          targetZone(0, 0)
    {}

    int unused00; // zeroed by the constructor, copied with the state, not serialized; nothing reads it
    int seed;     // rand (World::generateMission); seeds the target's name and its residents (World::update srands
                  // seed + creature id)
    int kind;     // a MissionKind; 0 for none
    int race;     // the creature to defeat (randomHostileRace, randomMonsterRace, randomCreatureRace or the region's
                  // monster)
    int level;    // The region’s highest level, also used for residents and rewards.
    // Reward rarity from zero to four, copied from the region record.
    unsigned char rarity;
    // One while a counted mission is active, two after completion.
    unsigned char progress;
    int killCount;       // counted mission kills
    int targetKillCount; // Kills required for completion.
    // Mission zone used by map and HUD markers; (-1, -1) for an invasion.
    plasma::Vector<2, int> targetZone;
};
}
