// Serializer reads and writes saved zones, region missions, monsters and client characters in a versioned format.
#include "cube/DefaultBlocks.h"
#include "cube/Serializer.h"
#include "cube/Creature.h"
#include "cube/CreatureIds.h"
#include "cube/Zone.h"
#include "cube/Spawn.h"
#include "cube/WorldBlocks.h"

namespace cube {
// A saved zone keeps the items dropped and the blocks changed in the last SavedZoneDays days (and
// those without a day) (anonymous: this file's own).
namespace {
enum {
    SavedZoneDays = 3
};
}

Serializer::Serializer(BytesIO *stream)
    : m_stream(stream),
      m_version(1),
      m_loadedVersion(0)
{}

void Serializer::loadCreature(Creature *creature)
{
    creature->reset();
    delete creature->m_playerData;
    creature->m_playerData = new Creature::PlayerData;
    creature->m_entityData.equipment = Equipment();
    creature->m_inventory = Inventory();
    creature->m_inventory.m_inventoryTabs.resize(InventoryTabCount);
    creature->m_entityData = Creature::EntityData();
    creature->m_entityData.hostilityType = HostilityPlayer;
    m_stream->read(&m_loadedVersion, sizeof(m_loadedVersion));
    m_stream->read(&creature->m_entityData.position, sizeof(creature->m_entityData.position));
    m_stream->read(&creature->m_entityData.rotation, sizeof(creature->m_entityData.rotation));
    m_stream->read(&creature->m_entityData.hp, sizeof(creature->m_entityData.hp));
    m_stream->read(&creature->m_entityData.xp, sizeof(creature->m_entityData.xp));
    m_stream->read(&creature->m_entityData.level, sizeof(creature->m_entityData.level));
    m_stream->read(&creature->m_entityData.classType, sizeof(creature->m_entityData.classType));
    m_stream->read(&creature->m_entityData.specialization, sizeof(creature->m_entityData.specialization));
    m_stream->read(&creature->m_ridingStamina, sizeof(creature->m_ridingStamina));
    m_stream->read(&creature->unused119c, sizeof(creature->unused119c));
    m_stream->read(&creature->m_entityData.equipment.head, sizeof(creature->m_entityData.equipment.head));
    m_stream->read(&creature->m_entityData.equipment.neck, sizeof(creature->m_entityData.equipment.neck));
    m_stream->read(&creature->m_entityData.equipment.chest, sizeof(creature->m_entityData.equipment.chest));
    m_stream->read(&creature->m_entityData.equipment.feet, sizeof(creature->m_entityData.equipment.feet));
    m_stream->read(&creature->m_entityData.equipment.hands, sizeof(creature->m_entityData.equipment.hands));
    m_stream->read(&creature->m_entityData.equipment.shoulder, sizeof(creature->m_entityData.equipment.shoulder));
    m_stream->read(&creature->m_entityData.equipment.weaponLeft, sizeof(creature->m_entityData.equipment.weaponLeft));
    m_stream->read(&creature->m_entityData.equipment.weaponRight, sizeof(creature->m_entityData.equipment.weaponRight));
    m_stream->read(&creature->m_entityData.equipment.ringLeft, sizeof(creature->m_entityData.equipment.ringLeft));
    m_stream->read(&creature->m_entityData.equipment.ringRight, sizeof(creature->m_entityData.equipment.ringRight));
    m_stream->read(&creature->m_entityData.equipment.lamp, sizeof(creature->m_entityData.equipment.lamp));
    m_stream->read(&creature->m_entityData.equipment.special, sizeof(creature->m_entityData.equipment.special));
    m_stream->read(&creature->m_entityData.equipment.pet, sizeof(creature->m_entityData.equipment.pet));

    int nameLength = 0;
    m_stream->read(&nameLength, sizeof(nameLength));
    m_stream->read(creature->m_entityData.name, nameLength);
    // Save lengths are not checked against the name's 16-byte capacity.
    creature->m_entityData.name[nameLength] = 0;
    delete creature->m_playerData;
    creature->m_playerData = new Creature::PlayerData;
    m_stream->read(static_cast<Creature::CharacterStyle *>(creature->m_playerData), sizeof(Creature::CharacterStyle));
    fillAppearance(creature->m_entityData.race, creature->m_entityData.appearance, creature->m_playerData);

    int numTabs = 0;
    m_stream->read(&numTabs, sizeof(numTabs));
    creature->m_inventory.m_inventoryTabs.resize(numTabs);
    for (int i = 0; i < numTabs; i++) {
        int numStacks = 0;
        m_stream->read(&numStacks, sizeof(numStacks));
        creature->m_inventory.m_inventoryTabs[i].resize(numStacks);
        if (numStacks != 0)
            m_stream->read(&creature->m_inventory.m_inventoryTabs[i][0], numStacks * sizeof(ItemStack));
    }
    m_stream->read(&creature->m_inventory.m_copperBalance, sizeof(creature->m_inventory.m_copperBalance));
    m_stream->read(&creature->m_inventory.m_platinumCoins, sizeof(creature->m_inventory.m_platinumCoins));

    int numFormulas = 0;
    m_stream->read(&numFormulas, sizeof(numFormulas));
    for (int i = 0; i < numFormulas; i++) {
        Item formula;
        m_stream->read(&formula, sizeof(formula));
        creature->m_playerData->knownFormulas.push_back(formula);
    }
    int numWorlds = 0;
    m_stream->read(&numWorlds, sizeof(numWorlds));
    for (int i = 0; i < numWorlds; i++) {
        // Short reads leave this seed and position uninitialized.
        int seed;
        m_stream->read(&seed, sizeof(seed));
        int length = 0;
        m_stream->read(&length, sizeof(length));
        std::string name;
        name.resize(length);
        m_stream->read(&name[0], length);
        plasma::Vector<3, Fixed> position;
        m_stream->read(&position, sizeof(position));
        creature->m_playerData->worldPositions[std::make_pair(seed, name)] = position;
    }
    m_stream->read(&creature->m_playerData->lastWorld.first, sizeof(creature->m_playerData->lastWorld.first));
    std::string name;
    int length = 0;
    m_stream->read(&length, sizeof(length));
    name.resize(length);
    m_stream->read(&name[0], length);
    creature->m_playerData->lastWorld.second = name;
    m_stream->read(&creature->m_entityData.manaCubeCount, sizeof(creature->m_entityData.manaCubeCount));
    int skillCount = 0;
    m_stream->read(&skillCount, sizeof(skillCount));
    if (skillCount == 11)
        m_stream->read(creature->m_entityData.skillLevels, sizeof(creature->m_entityData.skillLevels));
}

void Serializer::loadMission(MissionState &mission)
{
    m_stream->read(&m_loadedVersion, sizeof(m_loadedVersion));
    m_stream->read(&mission.seed, sizeof(mission.seed));
    m_stream->read(&mission.kind, sizeof(mission.kind));
    m_stream->read(&mission.race, sizeof(mission.race));
    m_stream->read(&mission.level, sizeof(mission.level));
    m_stream->read(&mission.rarity, sizeof(mission.rarity));
    m_stream->read(&mission.progress, sizeof(mission.progress));
    m_stream->read(&mission.killCount, sizeof(mission.killCount));
    m_stream->read(&mission.targetKillCount, sizeof(mission.targetKillCount));
    m_stream->read(&mission.targetZone, sizeof(mission.targetZone));
}

void Serializer::loadMonster(MonsterState &monster)
{
    m_stream->read(&m_loadedVersion, sizeof(m_loadedVersion));
    m_stream->read(&monster.race, sizeof(monster.race));
    m_stream->read(&monster.level, sizeof(monster.level));
    m_stream->read(&monster.rarity, sizeof(monster.rarity));
    m_stream->read(&monster.zone, sizeof(monster.zone));
}

void Serializer::loadZone(World *world, Zone *zone)
{
    // Short reads can leave record coordinates and item rotation unset.
    m_stream->read(&m_loadedVersion, sizeof(m_loadedVersion));
    int itemCount = 0;
    m_stream->read(&itemCount, sizeof(itemCount));
    std::vector<Zone::ZoneItem> items;
    for (int i = 0; i < itemCount; i++) {
        Zone::ZoneItem item;
        m_stream->read(&item.item, sizeof(item.item));
        m_stream->read(&item.position, sizeof(item.position));
        m_stream->read(&item.rotation, sizeof(item.rotation));
        m_stream->read(&item.scale, sizeof(item.scale));
        m_stream->read(&item.flags, sizeof(item.flags));
        m_stream->read(&item.dropTime, sizeof(item.dropTime));
        m_stream->read(&item.hiddenTimeRemaining, sizeof(item.hiddenTimeRemaining));
        m_stream->read(&item.dropDay, sizeof(item.dropDay));
        if (item.dropDay >= world->m_day - SavedZoneDays || item.dropDay < 0)
            items.push_back(item);
    }
    if (items != zone->m_items) {
        zone->m_modified = true;
        zone->m_items = items;
    }
    int blockCount = 0;
    m_stream->read(&blockCount, sizeof(blockCount));
    for (int i = 0; i < blockCount; i++) {
        Zone::CubeChange change;
        m_stream->read(&change, sizeof(change));
        if (change.day >= world->m_day - SavedZoneDays || change.day < 0) {
            world->setBlock(change.position[0], change.position[1], change.position[2], change.block, zone);
            zone->m_blockChanges.push_back(change);
        }
    }
    if (blockCount > 0) {
        std::list<std::list<Zone::Plant>::iterator> unsupported;
        for (std::list<Zone::Plant>::iterator it = zone->m_plants.begin(); it != zone->m_plants.end(); ++it) {
            if ((it->flags & (1 << PlantFlagNeedsSupport)) &&
                !(world->getBlock(it->position[0], it->position[1], it->position[2] - 1, zone).type & BlockTypeMask))
                unsupported.push_back(it);
        }
        for (std::list<std::list<Zone::Plant>::iterator>::iterator it = unsupported.begin(); it != unsupported.end();
             ++it)
            zone->m_plants.erase(*it);
    }
    int spawnCount = 0;
    m_stream->read(&spawnCount, sizeof(spawnCount));
    if (spawnCount == (int)zone->m_spawns.size()) {
        for (int i = 0; i < spawnCount; i++) {
            m_stream->read(&zone->m_spawns[i]->respawnTimeRemaining, sizeof(zone->m_spawns[i]->respawnTimeRemaining));
            m_stream->read(&zone->m_spawns[i]->deathDay, sizeof(zone->m_spawns[i]->deathDay));
        }
    }
    int objectCount = 0;
    m_stream->read(&objectCount, sizeof(objectCount));
    if (objectCount == (int)zone->m_objects.size()) {
        for (int i = 0; i < objectCount; i++)
            m_stream->read(&zone->m_objects[i].active, sizeof(zone->m_objects[i].active));
    }
}

void Serializer::saveCreature(Creature *creature)
{
    m_stream->write(&m_version, sizeof(m_version));
    m_stream->write(&creature->m_entityData.position, sizeof(creature->m_entityData.position));
    m_stream->write(&creature->m_entityData.rotation, sizeof(creature->m_entityData.rotation));
    m_stream->write(&creature->m_entityData.hp, sizeof(creature->m_entityData.hp));
    m_stream->write(&creature->m_entityData.xp, sizeof(creature->m_entityData.xp));
    m_stream->write(&creature->m_entityData.level, sizeof(creature->m_entityData.level));
    m_stream->write(&creature->m_entityData.classType, sizeof(creature->m_entityData.classType));
    m_stream->write(&creature->m_entityData.specialization, sizeof(creature->m_entityData.specialization));
    m_stream->write(&creature->m_ridingStamina, sizeof(creature->m_ridingStamina));
    m_stream->write(&creature->unused119c, sizeof(creature->unused119c));
    m_stream->write(&creature->m_entityData.equipment.head, sizeof(creature->m_entityData.equipment.head));
    m_stream->write(&creature->m_entityData.equipment.neck, sizeof(creature->m_entityData.equipment.neck));
    m_stream->write(&creature->m_entityData.equipment.chest, sizeof(creature->m_entityData.equipment.chest));
    m_stream->write(&creature->m_entityData.equipment.feet, sizeof(creature->m_entityData.equipment.feet));
    m_stream->write(&creature->m_entityData.equipment.hands, sizeof(creature->m_entityData.equipment.hands));
    m_stream->write(&creature->m_entityData.equipment.shoulder, sizeof(creature->m_entityData.equipment.shoulder));
    m_stream->write(&creature->m_entityData.equipment.weaponLeft, sizeof(creature->m_entityData.equipment.weaponLeft));
    m_stream->write(&creature->m_entityData.equipment.weaponRight,
                    sizeof(creature->m_entityData.equipment.weaponRight));
    m_stream->write(&creature->m_entityData.equipment.ringLeft, sizeof(creature->m_entityData.equipment.ringLeft));
    m_stream->write(&creature->m_entityData.equipment.ringRight, sizeof(creature->m_entityData.equipment.ringRight));
    m_stream->write(&creature->m_entityData.equipment.lamp, sizeof(creature->m_entityData.equipment.lamp));
    m_stream->write(&creature->m_entityData.equipment.special, sizeof(creature->m_entityData.equipment.special));
    m_stream->write(&creature->m_entityData.equipment.pet, sizeof(creature->m_entityData.equipment.pet));

    int nameLength = (int)strlen(creature->m_entityData.name);
    m_stream->write(&nameLength, sizeof(nameLength));
    m_stream->write(creature->m_entityData.name, nameLength);
    m_stream->write(static_cast<const Creature::CharacterStyle *>(creature->m_playerData),
                    sizeof(Creature::CharacterStyle));

    int numTabs = (int)creature->m_inventory.m_inventoryTabs.size();
    m_stream->write(&numTabs, sizeof(numTabs));
    for (int i = 0; i < numTabs; i++) {
        int numStacks = (int)creature->m_inventory.m_inventoryTabs[i].size();
        m_stream->write(&numStacks, sizeof(numStacks));
        if (numStacks != 0)
            m_stream->write(&creature->m_inventory.m_inventoryTabs[i][0], numStacks * sizeof(ItemStack));
    }
    m_stream->write(&creature->m_inventory.m_copperBalance, sizeof(creature->m_inventory.m_copperBalance));
    m_stream->write(&creature->m_inventory.m_platinumCoins, sizeof(creature->m_inventory.m_platinumCoins));

    int numFormulas = (int)creature->m_playerData->knownFormulas.size();
    m_stream->write(&numFormulas, sizeof(numFormulas));
    for (std::list<Item>::const_iterator it = creature->m_playerData->knownFormulas.begin();
         it != creature->m_playerData->knownFormulas.end(); ++it)
        m_stream->write(&*it, sizeof(*it));

    int numWorlds = (int)creature->m_playerData->worldPositions.size();
    m_stream->write(&numWorlds, sizeof(numWorlds));
    for (Creature::PlayerData::WorldPositions::const_iterator it = creature->m_playerData->worldPositions.begin();
         it != creature->m_playerData->worldPositions.end(); ++it) {
        int seed = it->first.first;
        std::string name = it->first.second;
        m_stream->write(&seed, sizeof(seed));
        int length = (int)name.size();
        m_stream->write(&length, sizeof(length));
        m_stream->write(name.c_str(), length);
        m_stream->write(&it->second, sizeof(it->second));
    }
    int seed = creature->m_playerData->lastWorld.first;
    std::string name = creature->m_playerData->lastWorld.second;
    m_stream->write(&seed, sizeof(seed));
    int length = (int)name.size();
    m_stream->write(&length, sizeof(length));
    m_stream->write(name.c_str(), length);
    m_stream->write(&creature->m_entityData.manaCubeCount, sizeof(creature->m_entityData.manaCubeCount));
    int skillCount = 11;
    m_stream->write(&skillCount, sizeof(skillCount));
    m_stream->write(creature->m_entityData.skillLevels, sizeof(creature->m_entityData.skillLevels));
}

void Serializer::saveMission(const MissionState &mission)
{
    m_stream->write(&m_version, sizeof(m_version));
    m_stream->write(&mission.seed, sizeof(mission.seed));
    m_stream->write(&mission.kind, sizeof(mission.kind));
    m_stream->write(&mission.race, sizeof(mission.race));
    m_stream->write(&mission.level, sizeof(mission.level));
    m_stream->write(&mission.rarity, sizeof(mission.rarity));
    m_stream->write(&mission.progress, sizeof(mission.progress));
    m_stream->write(&mission.killCount, sizeof(mission.killCount));
    m_stream->write(&mission.targetKillCount, sizeof(mission.targetKillCount));
    m_stream->write(&mission.targetZone, sizeof(mission.targetZone));
}

void Serializer::saveMonster(const MonsterState &monster)
{
    m_stream->write(&m_version, sizeof(m_version));
    m_stream->write(&monster.race, sizeof(monster.race));
    m_stream->write(&monster.level, sizeof(monster.level));
    m_stream->write(&monster.rarity, sizeof(monster.rarity));
    m_stream->write(&monster.zone, sizeof(monster.zone));
}

void Serializer::saveZone(const Zone *zone)
{
    m_stream->write(&m_version, sizeof(m_version));
    if (zone->m_modified) {
        int itemCount = (int)zone->m_items.size();
        m_stream->write(&itemCount, sizeof(itemCount));
        for (std::vector<Zone::ZoneItem>::const_iterator it = zone->m_items.begin(); it != zone->m_items.end(); ++it) {
            m_stream->write(&it->item, sizeof(it->item));
            m_stream->write(&it->position, sizeof(it->position));
            m_stream->write(&it->rotation, sizeof(it->rotation));
            m_stream->write(&it->scale, sizeof(it->scale));
            m_stream->write(&it->flags, sizeof(it->flags));
            m_stream->write(&it->dropTime, sizeof(it->dropTime));
            m_stream->write(&it->hiddenTimeRemaining, sizeof(it->hiddenTimeRemaining));
            m_stream->write(&it->dropDay, sizeof(it->dropDay));
        }
    } else {
        int itemCount = 0;
        m_stream->write(&itemCount, sizeof(itemCount));
    }
    int recordCount = (int)zone->m_blockChanges.size();
    m_stream->write(&recordCount, sizeof(recordCount));
    if (zone->m_blockChanges.size())
        m_stream->write(&zone->m_blockChanges[0], recordCount * sizeof(Zone::CubeChange));
    int spawnCount = (int)zone->m_spawns.size();
    m_stream->write(&spawnCount, sizeof(spawnCount));
    for (int i = 0; i < spawnCount; i++) {
        m_stream->write(&zone->m_spawns[i]->respawnTimeRemaining, sizeof(zone->m_spawns[i]->respawnTimeRemaining));
        m_stream->write(&zone->m_spawns[i]->deathDay, sizeof(zone->m_spawns[i]->deathDay));
    }
    int count = (int)zone->m_objects.size();
    m_stream->write(&count, sizeof(count));
    for (int i = 0; i < count; i++)
        m_stream->write(&zone->m_objects[i].active, sizeof(zone->m_objects[i].active));
}

}
