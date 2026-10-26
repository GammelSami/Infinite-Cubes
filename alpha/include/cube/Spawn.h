// Spawn describes a creature before it exists. Zones own spawns; each stores appearance, equipment, inventory, behavior and daily schedule. Destruction releases its behavior and containers.
#pragma once
#include <vector>
#include "cube/Creature.h"

namespace cube {

class Behavior;

struct Spawn {
    Spawn();
    virtual ~Spawn();

    float spawnRadius; // Player proximity required for spawning, in blocks.
    // Spawn position follows the latest due daily-schedule location.
    plasma::Vector<3, Fixed> position;
    // Creature identity and level defaults: hostility one, race four, zero class and specialization,
    // level one.
    int hostilityType;
    int race;
    unsigned char classType;
    unsigned char specialization;
    int level; // Copied into the item level.
    // Saved with the zone and cleared when the spawned creature is deleted.
    int respawnTimeRemaining; // milliseconds until respawn
    int deathDay;             // World day when killed, saved with the zone.
    // Creates the creature between the configured times of day; equal endpoints mean always. The
    // following state is copied into entity data.
    int activeFrom;
    int activeUntil;
    long long creatureId;
    unsigned char unknown050; // entity toggle mask 8
    float rotation;           // the creature's rotation about z
    unsigned char rarity;     // EntityData rarity.
    // Speech expression initialized to zero and assigned as an integer by house generation.
    int speechExpression;             // EntityData::speechExpression (a byte there)
    plasma::Vector<3, int> questZone; // EntityData::questZone
    int unused06c;                    // -1 from the constructor; nothing reads it
    int unused070;                    // 0 from the constructor; nothing reads it
    Creature::Appearance appearance;
    Equipment equipment;
    Creature::StatMultipliers multipliers;
    Inventory inventory;
    // Owned behavior, deleted without virtual dispatch because Behavior has no virtual destructor.
    Behavior *m_behavior;
    // Daily schedule entries hold a place and time.
    std::vector<Creature::ScheduleEntry> schedule;
    // Special abilities, summon appearance and summon races configured by World::equip.
    std::vector<int> specialSkills; // ability ids
    int summonAppearanceMode;       // 0
    std::vector<int> summonRaces;   // race ids
    int unused10c8;                 // -1 from the constructor; copied into EntityData::unused19c, which nothing reads
    char name[0x14];                // copied into the creature's name
    long long generationKey;        // Assigned by house generation.
    bool missionResident;           // false initially
};

}
