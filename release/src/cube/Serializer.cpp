#include "cube/Serializer.h"

#include <iostream>

#include "cube/Zone.h"
#include "cube/CreatureIds.h"

namespace cube {

Serializer::Serializer(BytesIO *stream)
    : m_stream(stream),
      m_version(11),
      m_loadedVersion(0),
      m_stateVersion(44),
      m_unused14(0),
      m_unused18(0)
{}

// Reads a character as saveCreature wrote it, into a creature that is reset first. m_loadedVersion is the format
// version of the data: sections that later versions added or dropped are read or skipped by it. The counts declared
// without a value keep what they held when the data ends early (BytesIO::read).
void Serializer::loadCreature(Creature *creature)
{
    creature->reset();
    delete creature->m_playerData;
    creature->m_playerData = new PlayerData;
    creature->m_entityData.equipment = Equipment();
    creature->m_inventory = Inventory();
    creature->m_inventory.m_inventoryTabs.resize(InventoryTabArtifacts + 1);
    creature->m_entityData = Creature::EntityData();
    creature->m_entityData.hostilityType = HostilityPlayer;

    m_stream->read(m_loadedVersion);
    m_stream->read(creature->m_entityData.position);
    m_stream->read(creature->m_entityData.rotation);
    // The saved rotation is read and dropped.
    creature->m_entityData.rotation[0] = 0.0f;
    creature->m_entityData.rotation[1] = 0.0f;
    creature->m_entityData.rotation[2] = 0.0f;
    m_stream->read(creature->m_entityData.hp);
    m_stream->read(creature->m_entityData.level);
    m_stream->read(creature->m_entityData.xp);
    m_stream->read(creature->m_entityData.classType);
    m_stream->read(creature->m_entityData.specialization);
    m_stream->read(creature->m_petWaterLevel);
    m_stream->read(creature->m_unreadSavedInt);

    readItem(creature->m_entityData.equipment.head);
    readItem(creature->m_entityData.equipment.neck);
    readItem(creature->m_entityData.equipment.chest);
    readItem(creature->m_entityData.equipment.feet);
    readItem(creature->m_entityData.equipment.hands);
    readItem(creature->m_entityData.equipment.shoulder);
    readItem(creature->m_entityData.equipment.weaponLeft);
    readItem(creature->m_entityData.equipment.weaponRight);
    readItem(creature->m_entityData.equipment.ringLeft);
    readItem(creature->m_entityData.equipment.ringRight);
    readItem(creature->m_entityData.equipment.pet);

    int nameLength = 0;
    m_stream->read(nameLength);
    m_stream->read(creature->m_entityData.name, nameLength);
    creature->m_entityData.name[nameLength] = 0; // UB: not checked against the 16 bytes of name

    delete creature->m_playerData;
    creature->m_playerData = new PlayerData;
    m_stream->read(*(CharacterStyle *)creature->m_playerData);
    fillAppearance(creature->m_entityData.race, creature->m_entityData.appearance, creature->m_playerData, false);

    int numTabs = 0;
    m_stream->read(numTabs);
    creature->m_inventory.m_inventoryTabs.resize(numTabs);
    for (int i = 0; i < numTabs; i++) {
        int numStacks = 0;
        m_stream->read(numStacks);
        creature->m_inventory.m_inventoryTabs[i].resize(numStacks);
        if (numStacks != 0)
            m_stream->read(&creature->m_inventory.m_inventoryTabs[i][0], numStacks * sizeof(ItemStack));
    }
    m_stream->read(creature->m_inventory.m_gold);
    m_stream->read(creature->m_inventory.m_manaCubes);
    // The limit does not fit an int: the value stored is its low 32 bits.
    if (creature->m_inventory.m_gold > 10000000000ULL)
        creature->m_inventory.m_gold = 10000000000ULL;

    int numFormulas = 0;
    m_stream->read(numFormulas);
    for (int i = 0; i < numFormulas; i++) {
        Item formula;
        m_stream->read(formula);
        creature->m_playerData->knownFormulas.push_back(formula);
    }

    if (m_loadedVersion >= 10) {
        int numPlaces;
        m_stream->read(numPlaces); // UB: used uninitialized when the data is shorter, as the counts below
        for (int i = 0; i < numPlaces; i++) {
            std::array<int, 2> place;
            m_stream->read(place);
            creature->m_playerData->discoveredPlaces.insert(place);
        }

        int savedCellCount;
        m_stream->read(savedCellCount);
        for (int i = 0; i < savedCellCount; i++) {
            std::array<int, 2> place;
            m_stream->read(place);
            creature->m_playerData->savedCellSet.insert(place);
        }

        int numAreas;
        m_stream->read(numAreas);
        for (int i = 0; i < numAreas; i++) {
            std::array<int, 2> area;
            m_stream->read(area);
            creature->m_playerData->discoveredAreas.insert(area);
        }

        int numCollectedPlacedItems;
        m_stream->read(numCollectedPlacedItems);
        for (int i = 0; i < numCollectedPlacedItems; i++) {
            IntTriple position;
            m_stream->read(position);
            creature->m_playerData->collectedPlacedItems.insert(position);
        }

        int savedTripleCount;
        m_stream->read(savedTripleCount);
        for (int i = 0; i < savedTripleCount; i++) {
            IntTriple position;
            m_stream->read(position);
            creature->m_playerData->savedTripleSet.insert(position);
        }

        int numMarkers;
        m_stream->read(numMarkers);
        for (int i = 0; i < numMarkers; i++) {
            std::array<int, 2> marker;
            m_stream->read(marker);
            creature->m_playerData->discoveredMarkers.insert(marker);
        }

        int numKnownMissions;
        m_stream->read(numKnownMissions);
        for (int i = 0; i < numKnownMissions; i++) {
            std::array<int, 2> place;
            m_stream->read(place);
            creature->m_playerData->knownMissions.insert(place);
        }

        int numSolvedMissions;
        m_stream->read(numSolvedMissions);
        for (int i = 0; i < numSolvedMissions; i++) {
            std::array<int, 2> place;
            m_stream->read(place);
            creature->m_playerData->solvedMissions.insert(place);
        }

        int numFinishedMissions;
        m_stream->read(numFinishedMissions);
        for (int i = 0; i < numFinishedMissions; i++) {
            std::array<int, 2> place;
            m_stream->read(place);
            creature->m_playerData->finishedMissions.insert(place);
        }

        int numTimers;
        m_stream->read(numTimers);
        for (int i = 0; i < numTimers; i++) {
            std::pair<std::array<int, 2>, float> timer;
            m_stream->read(timer);
            creature->m_playerData->missionTimers.insert(timer);
        }
    }

    // A list that versions before 4 had: a dword, a string and 24 bytes per entry, all dropped.
    if (m_loadedVersion < 4) {
        int count = 0;
        m_stream->read(count);
        for (int i = 0; i < count; i++) {
            m_stream->skip(4);
            int length = 0;
            m_stream->read(length);
            std::string text;
            text.resize(length);
            m_stream->read(&text[0], length);
            m_stream->skip(0x18);
        }
    }

    m_stream->read(creature->m_playerData->lastWorld.seed);
    std::string text;
    int length = 0;
    m_stream->read(length);
    text.resize(length);
    m_stream->read(&text[0], length);
    creature->m_playerData->lastWorld.name = text;

    int numFlightPoints = 0;
    m_stream->read(numFlightPoints);
    creature->m_playerData->knownFlightPoints.clear();
    for (int i = 0; i < numFlightPoints; i++) {
        std::array<int, 2> point;
        m_stream->read(point);
        creature->m_playerData->knownFlightPoints.insert(point);
    }

    m_stream->read(creature->m_entityData.portCity);

    int numBuffs = 0;
    m_stream->read(numBuffs);
    creature->m_buffs.clear();
    for (int i = 0; i < numBuffs; i++) {
        Creature::Buff buff;
        m_stream->read(buff);
        creature->m_buffs.push_back(buff);
    }

    int numPlatinum;
    m_stream->read(numPlatinum);
    for (int i = 0; i < numPlatinum; i++) {
        std::array<int, 2> region;
        unsigned int coins;
        m_stream->read(region);
        m_stream->read(coins);
        creature->m_playerData->platinum[region] = coins;
    }

    // The treasure markers go back to the lists of their regions.
    int numTreasures;
    m_stream->read(numTreasures);
    for (int i = 0; i < numTreasures; i++) {
        MapPlace place;
        m_stream->read(place);
        std::array<int, 2> cell;
        cell[0] = place.position[0] / BlocksPerRegion;
        cell[1] = place.position[1] / BlocksPerRegion;
        creature->m_playerData->treasures[cell].push_back(place);
    }

    int numVisitedRegions;
    m_stream->read(numVisitedRegions);
    for (int i = 0; i < numVisitedRegions; i++) {
        plasma::Vector<2, int> entry;
        m_stream->read(entry);
        creature->m_playerData->visitedRegions.push_back(entry);
    }

    int numPurchasesByShop = 0;
    m_stream->read(numPurchasesByShop);
    for (int i = 0; i < numPurchasesByShop; i++) {
        long long key;
        m_stream->read(key);
        int numItems;
        m_stream->read(numItems);
        for (int j = 0; j < numItems; j++) {
            Item item;
            m_stream->read(item);
            creature->m_playerData->purchasesByShop[key].push_back(item);
        }
    }
}

// Reads what saveWorldState wrote. A state of another format version keeps only the day and the time. The counts and
// the version are read into uninitialized locals, which keep their value when the data ends early (BytesIO::read).
void Serializer::loadWorldState(World::WorldState &state)
{
    std::cout << "loadWorldState 1" << std::endl;
    int version;
    m_stream->read(version); // UB: used uninitialized when the data is shorter
    m_stream->read(state.m_day);
    m_stream->read(state.m_time);
    std::cout << "loadWorldState 2" << std::endl;
    if (version != m_stateVersion)
        return;

    int numDeadCreatures;
    m_stream->read(numDeadCreatures);
    state.m_deadCreatures.clear();
    std::cout << "numDeadCreatures " << numDeadCreatures << std::endl;
    for (int i = 0; i < numDeadCreatures; i++) {
        long long id;
        m_stream->read(id);
        state.m_deadCreatures.insert(id);
    }

    int numDroppedRewards;
    m_stream->read(numDroppedRewards);
    state.m_droppedRewards.clear();
    std::cout << "numDroppedRewards " << numDroppedRewards << std::endl;
    for (int i = 0; i < numDroppedRewards; i++) {
        long long id;
        m_stream->read(id);
        state.m_droppedRewards.insert(id);
    }

    int numZoneStates;
    m_stream->read(numZoneStates);
    state.m_zoneStates.clear();
    std::cout << "numZoneStates " << numZoneStates << std::endl;
    for (int i = 0; i < numZoneStates; i++) {
        std::array<int, 2> key;
        m_stream->read(key[0]);
        m_stream->read(key[1]);
        World::ZoneState &zoneState = state.m_zoneStates[key];
        m_stream->read(zoneState.flags);

        int numItems;
        m_stream->read(numItems);
        for (int j = 0; j < numItems; j++) {
            ZoneItem item;
            m_stream->read(item);
            zoneState.items.push_back(item);
        }

        int numStaticEntities;
        m_stream->read(numStaticEntities);
        for (int j = 0; j < numStaticEntities; j++) {
            StaticEntity entity;
            m_stream->read(entity);
            zoneState.staticEntities.push_back(entity);
        }

        int numCubeChanges;
        m_stream->read(numCubeChanges);
        for (int j = 0; j < numCubeChanges; j++) {
            CubeChange change;
            m_stream->read(change);
            zoneState.cubeChanges.push_back(change);
        }

        int numCompletedActivators;
        m_stream->read(numCompletedActivators);
        for (int j = 0; j < numCompletedActivators; j++) {
            std::pair<std::array<int, 2>, unsigned int> entry;
            m_stream->read(entry);
            zoneState.completedActivators[entry.first] = entry.second;
        }

        m_stream->read(zoneState.defeatedMissionCreatureCount);
    }
}

bool Serializer::readItem(Item &item)
{
    m_stream->read(&item, sizeof(Item));
    return true;
}

// Writes the format version (m_version), then what loadCreature reads, in its order; every container as a count and
// its elements' bytes.
void Serializer::saveCreature(Creature *creature)
{
    m_stream->write(m_version);
    m_stream->write(creature->m_entityData.position);
    m_stream->write(creature->m_entityData.rotation);
    m_stream->write(creature->m_entityData.hp);
    m_stream->write(creature->m_entityData.level);
    m_stream->write(creature->m_entityData.xp);
    m_stream->write(creature->m_entityData.classType);
    m_stream->write(creature->m_entityData.specialization);
    m_stream->write(creature->m_petWaterLevel);
    m_stream->write(creature->m_unreadSavedInt);

    writeItem(creature->m_entityData.equipment.head);
    writeItem(creature->m_entityData.equipment.neck);
    writeItem(creature->m_entityData.equipment.chest);
    writeItem(creature->m_entityData.equipment.feet);
    writeItem(creature->m_entityData.equipment.hands);
    writeItem(creature->m_entityData.equipment.shoulder);
    writeItem(creature->m_entityData.equipment.weaponLeft);
    writeItem(creature->m_entityData.equipment.weaponRight);
    writeItem(creature->m_entityData.equipment.ringLeft);
    writeItem(creature->m_entityData.equipment.ringRight);
    writeItem(creature->m_entityData.equipment.pet);

    int nameLength = (int)strlen(creature->m_entityData.name);
    m_stream->write(nameLength);
    m_stream->write(creature->m_entityData.name, nameLength);

    m_stream->write(*(const CharacterStyle *)creature->m_playerData);

    int numTabs = (int)creature->m_inventory.m_inventoryTabs.size();
    m_stream->write(numTabs);
    for (int i = 0; i < numTabs; i++) {
        int numStacks = (int)creature->m_inventory.m_inventoryTabs[i].size();
        m_stream->write(numStacks);
        if (numStacks != 0)
            m_stream->write(&creature->m_inventory.m_inventoryTabs[i][0], numStacks * sizeof(ItemStack));
    }
    m_stream->write(creature->m_inventory.m_gold);
    m_stream->write(creature->m_inventory.m_manaCubes);

    m_stream->write((int)creature->m_playerData->knownFormulas.size());
    for (const Item &formula : creature->m_playerData->knownFormulas)
        m_stream->write(formula);

    m_stream->write((int)creature->m_playerData->discoveredPlaces.size());
    for (const std::array<int, 2> &place : creature->m_playerData->discoveredPlaces)
        m_stream->write(place);

    m_stream->write((int)creature->m_playerData->savedCellSet.size());
    for (const std::array<int, 2> &place : creature->m_playerData->savedCellSet)
        m_stream->write(place);

    m_stream->write((int)creature->m_playerData->discoveredAreas.size());
    for (const std::array<int, 2> &area : creature->m_playerData->discoveredAreas)
        m_stream->write(area);

    m_stream->write((int)creature->m_playerData->collectedPlacedItems.size());
    for (const IntTriple &position : creature->m_playerData->collectedPlacedItems)
        m_stream->write(position);

    m_stream->write((int)creature->m_playerData->savedTripleSet.size());
    for (const IntTriple &position : creature->m_playerData->savedTripleSet)
        m_stream->write(position);

    m_stream->write((int)creature->m_playerData->discoveredMarkers.size());
    for (const std::array<int, 2> &marker : creature->m_playerData->discoveredMarkers)
        m_stream->write(marker);

    m_stream->write((int)creature->m_playerData->knownMissions.size());
    for (const std::array<int, 2> &place : creature->m_playerData->knownMissions)
        m_stream->write(place);

    m_stream->write((int)creature->m_playerData->solvedMissions.size());
    for (const std::array<int, 2> &place : creature->m_playerData->solvedMissions)
        m_stream->write(place);

    m_stream->write((int)creature->m_playerData->finishedMissions.size());
    for (const std::array<int, 2> &place : creature->m_playerData->finishedMissions)
        m_stream->write(place);

    m_stream->write((int)creature->m_playerData->missionTimers.size());
    for (const std::pair<const std::array<int, 2>, float> &timer : creature->m_playerData->missionTimers)
        m_stream->write(timer);

    int number = creature->m_playerData->lastWorld.seed;
    std::string text = creature->m_playerData->lastWorld.name;
    m_stream->write(number);
    int length = (int)text.size();
    m_stream->write(length);
    m_stream->write(text.c_str(), length);

    m_stream->write((int)creature->m_playerData->knownFlightPoints.size());
    for (const std::array<int, 2> &point : creature->m_playerData->knownFlightPoints)
        m_stream->write(point);

    m_stream->write(creature->m_entityData.portCity);

    m_stream->write((int)creature->m_buffs.size());
    for (const Creature::Buff &buff : creature->m_buffs)
        m_stream->write(buff);

    m_stream->write((int)creature->m_playerData->platinum.size());
    for (const std::pair<const std::array<int, 2>, unsigned int> &coins : creature->m_playerData->platinum) {
        m_stream->write(coins.first);
        m_stream->write(coins.second);
    }

    // The treasure markers of all cells as one list.
    std::list<MapPlace> treasures;
    for (const std::pair<const std::array<int, 2>, std::list<MapPlace>> &cell : creature->m_playerData->treasures)
        treasures.insert(treasures.end(), cell.second.begin(), cell.second.end());
    m_stream->write((int)treasures.size());
    for (const MapPlace &place : treasures)
        m_stream->write(place);

    m_stream->write((int)creature->m_playerData->visitedRegions.size());
    for (const plasma::Vector<2, int> &entry : creature->m_playerData->visitedRegions)
        m_stream->write(entry);

    m_stream->write((int)creature->m_playerData->purchasesByShop.size());
    for (const std::pair<const long long, std::list<Item>> &entry : creature->m_playerData->purchasesByShop) {
        m_stream->write(entry.first);
        m_stream->write((int)entry.second.size());
        for (const Item &item : entry.second)
            m_stream->write(item);
    }
}

// Writes the format version (m_stateVersion), the day and the time, the two id sets and, for every zone record, its
// key and its members in order; every container as a count and its elements' bytes.
void Serializer::saveWorldState(const World::WorldState &state)
{
    m_stream->write(m_stateVersion);
    m_stream->write(state.m_day);
    m_stream->write(state.m_time);

    m_stream->write((int)state.m_deadCreatures.size());
    for (long long id : state.m_deadCreatures)
        m_stream->write(id);

    m_stream->write((int)state.m_droppedRewards.size());
    for (long long id : state.m_droppedRewards)
        m_stream->write(id);

    m_stream->write((int)state.m_zoneStates.size());
    for (const std::pair<const std::array<int, 2>, World::ZoneState> &entry : state.m_zoneStates) {
        m_stream->write(entry.first[0]);
        m_stream->write(entry.first[1]);
        const World::ZoneState &zoneState = entry.second;
        m_stream->write(zoneState.flags);

        m_stream->write((int)zoneState.items.size());
        for (const ZoneItem &item : zoneState.items)
            m_stream->write(item);

        m_stream->write((int)zoneState.staticEntities.size());
        for (const StaticEntity &entity : zoneState.staticEntities)
            m_stream->write(entity);

        m_stream->write((int)zoneState.cubeChanges.size());
        for (const CubeChange &change : zoneState.cubeChanges)
            m_stream->write(change);

        m_stream->write((int)zoneState.completedActivators.size());
        for (const std::pair<const std::array<int, 2>, unsigned int> &activatorCount : zoneState.completedActivators)
            m_stream->write(activatorCount);

        m_stream->write(zoneState.defeatedMissionCreatureCount);
    }
}

void Serializer::writeItem(const Item &item)
{
    m_stream->write(&item, sizeof(Item));
}

}
