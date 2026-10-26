#include "cube/Shuffle.h"
// Region mission generation and progress tracking; see cube/Missions.h.
#include "cube/Noise.h"
#include "cube/CaveBuilder.h"
#include "cube/Creature.h"
#include "cube/Missions.h"
#include "cube/Realm.h"
#include "cube/World.h"
#include "cube/WorldLayout.h"
#include "cube/Zone.h"

#include <algorithm>
#include <set>
#include "cube/CreatureIds.h"
#include "cube/ItemIds.h"
#include "cube/MissionIds.h"
#include "cube/WorldIds.h"
#include "cube/ZoneIds.h"

namespace cube {

// The generators draw from hashes of the place cell, random(placeX + salt, placeY), with a salt of its own for each
// draw (39441 seeds a generator's rand(), 16786 picks the home building, ...); the salts only decorrelate the draws.
namespace {
// With no target building type a generator takes only the buildings with at least this influence (PlaceCoreInfluence
// otherwise, World.h).
const float PlaceCentreInfluence = 0.75f;
// Within a place with an environment, only its interior (influence at least this) counts as the open land around it.
const float EnvironmentCoreInfluence = 0.8f;
// The terrain is dry ground from this height up.
const float DryGroundHeight = 1.0f;
// The witch's hut: the cauldron is placed within this many cells of the place centre.
const int OldHutSearchCells = 4;
// A captive's prison is scaled to the captive's width over this, the width the prison models fit.
const float PrisonModelOccupantWidth = 0.8f;
// The items an exchange asks for.
const int ExchangeItemCount = 10;
// updateTasks: a resident or chest within this many blocks of the character completes its mission; the zones this many
// around the character's are checked for defeated mission creatures.
const int CompletionRange = 20;
const int CompletionZoneScanRadius = 3;
// The missions' buffs (Ancientpower, Manaabsorption) are renewed for a minute once less than a second remains.
const float MissionBuffRenewTime = 1000.0f;
const float MissionBuffDuration = 60000.0f;
}

Missions::Missions(World *world, const plasma::Vector<2, int> &region)
    : m_world(world),
      m_region(region),
      m_village(NoCoordinate, NoCoordinate),
      m_hasDemonPortal(false)
{
    // Every draw below hashes the seed and advances it.
    int seed = random(region[0] + 4642754, region[1]);
    int biome = world->landBiome(region[0], region[1]);
    // The places of the region: (location type, cell x, cell y) of every cell around it whose place centre lies in the
    // region, and how many there are of each type.
    std::set<IntTriple> places;
    std::map<int, int> counts;
    for (int x = floorDivide(region[0] * BlocksPerLand - BlocksPerLand, BlocksPerPlace);
         x <= floorDivide((region[0] + 2) * BlocksPerLand, BlocksPerPlace); x++) {
        for (int y = floorDivide(region[1] * BlocksPerLand - BlocksPerLand, BlocksPerPlace);
             y <= floorDivide((region[1] + 2) * BlocksPerLand, BlocksPerPlace); y++) {
            int cellX = x;
            int cellY = y;
            if (x != 0 || y != 0) {
                bool large = world->isLargePlace(x, y);
                int type = world->locationType(x, y, large);
                if (type != PlaceNone) {
                    if (large) {
                        cellX = floorDivide(cellX, 2) * 2;
                        cellY = floorDivide(cellY, 2) * 2;
                    }
                    plasma::Vector<2, int> center = world->placeCenter(cellX, cellY, large);
                    if (world->findRegion(center) == region) {
                        places.insert(IntTriple {{type, cellX, cellY}});
                        counts[type]++;
                    }
                }
            }
        }
    }
    std::set<IntTriple> allPlaces(places);

    // Towers, circles of power, mana pumps and arenas have fixed kinds, with their levels drawn in their own ranges
    // (2..4, 1..4, 1..4, 4).
    std::vector<IntTriple> fixed;
    for (auto &place : places) {
        if (place[0] == PlaceTower || place[0] == PlaceCircleOfPower || place[0] == PlaceManaPump ||
            place[0] == PlaceArena) {
            fixed.push_back(place);
        }
    }
    for (auto &place : fixed) {
        places.erase(place);
        --counts[place[0]];
        switch (place[0]) {
        case PlaceTower:
            m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindMagicBarriers;
            m_values[std::array<int, 2> {place[1], place[2]}] = hash(seed++) % 3 + 2;
            break;
        case PlaceCircleOfPower:
            m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindFlameOfPower;
            m_values[std::array<int, 2> {place[1], place[2]}] = (hash(seed++) & 3) + 1;
            break;
        case PlaceManaPump:
            m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindManaPumps;
            m_values[std::array<int, 2> {place[1], place[2]}] = (hash(seed++) & 3) + 1;
            break;
        case PlaceArena:
            m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindAborigines;
            m_values[std::array<int, 2> {place[1], place[2]}] = 4;
            break;
        default:
            m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKind_15;
            m_values[std::array<int, 2> {place[1], place[2]}] = (hash(seed++) & 3) + 1;
            break;
        }
    }

    // With two or more villages, an old hut gets the witch's curse and one of the villages is remembered.
    std::vector<std::array<int, 2>> villages;
    for (auto &place : places) {
        if (place[0] == PlaceVillage) {
            villages.push_back(std::array<int, 2> {place[1], place[2]});
        }
    }
    if (villages.size() > 1) {
        std::vector<IntTriple> candidates;
        for (auto &place : places) {
            if (place[0] == PlaceOldHut) {
                candidates.push_back(place);
            }
        }
        if (!candidates.empty()) {
            IntTriple place = candidates[hash(seed++) % candidates.size()];
            places.erase(place);
            --counts[place[0]];
            m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindWitchCurse;
            m_values[std::array<int, 2> {place[1], place[2]}] = hash(seed++) % (std::min)(5, (int)villages.size());
            const std::array<int, 2> &village = villages[hash(seed++) % villages.size()];
            m_village[0] = village[0];
            m_village[1] = village[1];
        }
    }

    // An altar gets kind 20.
    std::vector<IntTriple> altarPlaces;
    for (auto &place : places) {
        if (place[0] == PlaceAltar) {
            altarPlaces.push_back(place);
        }
    }
    if (!altarPlaces.empty()) {
        IntTriple place = altarPlaces[hash(seed++) % altarPlaces.size()];
        places.erase(place);
        --counts[place[0]];
        m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindDemonPortal;
        m_values[std::array<int, 2> {place[1], place[2]}] = hash(seed++) % 5;
        m_hasDemonPortal = true;
    }

    // Every dungeon (a castle, or a place with a dungeonType) gets kind 14 and an artifact; a region without one takes
    // an arena, else a remote place, else one of a third set of types, else any but a village.
    std::vector<IntTriple> dungeons;
    for (std::set<IntTriple>::iterator it = places.begin(); it != places.end(); ++it) {
        int type = (*it)[0];
        if (type == PlaceCastle ||
            world->dungeonType((*it)[1], (*it)[2], type, world->isLargePlace((*it)[1], (*it)[2])) != PlaceNone) {
            dungeons.push_back(*it);
        }
    }
    for (std::vector<IntTriple>::iterator it = dungeons.begin(); it != dungeons.end(); ++it) {
        places.erase(*it);
        --counts[(*it)[0]];
        m_kinds[std::array<int, 2> {(*it)[1], (*it)[2]}] = MissionKindArtifactReward;
        m_values[std::array<int, 2> {(*it)[1], (*it)[2]}] = 4;
        generateArtifact((*it)[0], (*it)[1], (*it)[2], seed);
    }
    if (dungeons.empty()) {
        std::vector<IntTriple> candidates;
        for (auto &place : places) {
            if (place[0] == PlaceArena) {
                candidates.push_back(place);
            }
        }
        if (candidates.empty()) {
            for (auto &place : places) {
                if (isRemoteType(place[0])) {
                    candidates.push_back(place);
                }
            }
        }
        if (candidates.empty()) {
            for (auto &place : places) {
                int type = place[0];
                if (type == PlaceTower || type == PlaceGraveyard || type == PlaceCave || type == PlaceAncientRuins ||
                    type == PlaceCrater || type == PlaceTree || type == PlaceIsland || type == PlaceForest ||
                    type == PlaceCanyon || type == PlacePlain || type == PlaceHill) {
                    candidates.push_back(place);
                }
            }
        }
        if (candidates.empty()) {
            for (auto &place : places) {
                if (place[0] != PlaceVillage) {
                    candidates.push_back(place);
                }
            }
        }
        if (!candidates.empty()) {
            IntTriple place = candidates[hash(seed++) % candidates.size()];
            places.erase(place);
            --counts[place[0]];
            m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindArtifactReward;
            m_values[std::array<int, 2> {place[1], place[2]}] = 4;
            generateArtifact(place[0], place[1], place[2], seed);
        }
    }

    // Kind 6 (the hang glider): a mountain, volcano, rock or rocks, else any place but a village.
    std::vector<IntTriple> hangGliderPlaces;
    for (auto &place : places) {
        if (place[0] == PlaceMountain || place[0] == PlaceVolcano || place[0] == PlaceRock || place[0] == PlaceRocks) {
            hangGliderPlaces.push_back(place);
        }
    }
    if (hangGliderPlaces.empty()) {
        for (auto &place : places) {
            if (place[0] != PlaceVillage) {
                hangGliderPlaces.push_back(place);
            }
        }
    }
    if (!hangGliderPlaces.empty()) {
        IntTriple place = hangGliderPlaces[hash(seed++) % hangGliderPlaces.size()];
        places.erase(place);
        --counts[place[0]];
        m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindHangGliderPlacement;
    }

    // Kind 7 (the boat; not in the ocean, dark woods, deadlands or firelands): a lake or an island, else any place but
    // a village.
    if (biome != BiomeOcean && biome != BiomeDarkWoods && biome != BiomeDeadlands && biome != BiomeFirelands) {
        std::vector<IntTriple> candidates;
        for (auto &place : places) {
            if (place[0] == PlaceLake || place[0] == PlaceIsland) {
                candidates.push_back(place);
            }
        }
        if (candidates.empty()) {
            for (auto &place : places) {
                if (place[0] != PlaceVillage) {
                    candidates.push_back(place);
                }
            }
        }
        if (!candidates.empty()) {
            IntTriple place = candidates[hash(seed++) % candidates.size()];
            places.erase(place);
            --counts[place[0]];
            m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindBoatPlacement;
        }
    }

    // Four places of kind 1 with the values 0..3, drawn from one list (a place can be drawn twice).
    std::vector<IntTriple> supplierGnomePlaces;
    for (auto &place : places) {
        if (place[0] != PlaceVillage) {
            supplierGnomePlaces.push_back(place);
        }
    }
    for (int i = 0; i < 4; i++) {
        if (!supplierGnomePlaces.empty()) {
            IntTriple place = supplierGnomePlaces[hash(seed++) % supplierGnomePlaces.size()];
            places.erase(place);
            --counts[place[0]];
            m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindSupplierGnome;
            m_values[std::array<int, 2> {place[1], place[2]}] = i;
        }
    }
    // The kinds 2..5 with the values 0..3: each always while eight places are left, else one time in two.
    for (int i = 0; i < 4; i++) {
        std::vector<IntTriple> candidates;
        for (auto &place : places) {
            if (place[0] != PlaceVillage) {
                candidates.push_back(place);
            }
        }
        if ((int)candidates.size() >= 8 || (hash(seed++) & 1) == 0) {
            if (!candidates.empty()) {
                IntTriple place = candidates[hash(seed++) % candidates.size()];
                places.erase(place);
                --counts[place[0]];
                m_kinds[std::array<int, 2> {place[1], place[2]}] = i + MissionKindMagicCraftingBookReward;
                m_values[std::array<int, 2> {place[1], place[2]}] = i;
            }
        }
    }
    // One place each of the kinds 9 (not in the ocean biome), 10, 11, 13, 8 and 12.
    if (biome != BiomeOcean) {
        std::vector<IntTriple> candidates;
        for (std::set<IntTriple>::iterator it = places.begin(); it != places.end(); ++it) {
            if ((*it)[0] != PlaceVillage) {
                candidates.push_back(*it);
            }
        }
        if (!candidates.empty()) {
            IntTriple place = candidates[hash(seed++) % candidates.size()];
            places.erase(place);
            --counts[place[0]];
            m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindReinsPlacement;
        }
    }
    std::vector<IntTriple> harpPlaces;
    for (auto &place : places) {
        if (place[0] != PlaceVillage) {
            harpPlaces.push_back(place);
        }
    }
    if (!harpPlaces.empty()) {
        IntTriple place = harpPlaces[hash(seed++) % harpPlaces.size()];
        places.erase(place);
        --counts[place[0]];
        m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindHarpPlacement;
    }
    std::vector<IntTriple> whistlePlaces;
    for (auto &place : places) {
        if (place[0] != PlaceVillage) {
            whistlePlaces.push_back(place);
        }
    }
    if (!whistlePlaces.empty()) {
        IntTriple place = whistlePlaces[hash(seed++) % whistlePlaces.size()];
        places.erase(place);
        --counts[place[0]];
        m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindWhistlePlacement;
    }
    std::vector<IntTriple> treasureSpiritPlaces;
    for (std::set<IntTriple>::iterator it = places.begin(); it != places.end(); ++it) {
        if ((*it)[0] != PlaceVillage) {
            treasureSpiritPlaces.push_back(*it);
        }
    }
    if (!treasureSpiritPlaces.empty()) {
        IntTriple place = treasureSpiritPlaces[hash(seed++) % treasureSpiritPlaces.size()];
        places.erase(place);
        --counts[place[0]];
        m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindTreasureSpiritPlacement;
    }
    std::vector<IntTriple> climbingSpikesPlaces;
    for (auto &place : places) {
        if ((place)[0] != PlaceVillage) {
            climbingSpikesPlaces.push_back(place);
        }
    }
    if (!climbingSpikesPlaces.empty()) {
        IntTriple place = climbingSpikesPlaces[hash(seed++) % climbingSpikesPlaces.size()];
        places.erase(place);
        --counts[place[0]];
        m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindClimbingSpikesPlacement;
    }
    std::vector<IntTriple> bellPlaces;
    for (auto &place : places) {
        if (place[0] != PlaceVillage) {
            bellPlaces.push_back(place);
        }
    }
    if (!bellPlaces.empty()) {
        IntTriple place = bellPlaces[hash(seed++) % bellPlaces.size()];
        places.erase(place);
        --counts[place[0]];
        m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKindBellPlacement;
    }

    // Half of the remote places get kind 16.
    std::vector<IntTriple> remote;
    for (auto &place : places) {
        if (isRemoteType(place[0]) && (hash(seed++) & 1) != 0) {
            remote.push_back(place);
        }
    }
    for (std::vector<IntTriple>::iterator it = remote.begin(); it != remote.end(); ++it) {
        places.erase(*it);
        --counts[(*it)[0]];
        m_kinds[std::array<int, 2> {(*it)[1], (*it)[2]}] = MissionKindMonster;
        m_values[std::array<int, 2> {(*it)[1], (*it)[2]}] = (hash(seed++) & 3) + 1;
    }

    // The places that can take a partner, as they are before the rest get kind 15.
    std::vector<IntTriple> partners;
    for (auto &place : places) {
        int type = place[0];
        if (type == PlaceMountain || type == PlaceLake || type == PlaceTree || type == PlaceForest ||
            type == PlaceCanyon || type == PlaceRocks || type == PlaceHill || type == PlaceGrove) {
            partners.push_back(place);
        }
    }
    for (auto &place : places) {
        if (place[0] != PlaceVillage) {
            m_kinds[std::array<int, 2> {place[1], place[2]}] = MissionKind_15;
            m_values[std::array<int, 2> {place[1], place[2]}] = seed % 4;
            seed++;
        }
    }

    // A place whose topic is 24 is paired with the nearest candidate within 4 cells that has no kind, or kind 15 and
    // no partner yet; the partner gets kind 18 and the place's value.
    for (auto &place : allPlaces) {
        if (determineTopic(place[1], place[2]) == MissionExchange) {
            plasma::Vector<2, float> center(world->placeCenter(place[1], place[2]));
            float nearest = -1.0f;
            std::array<int, 2> partner;
            for (std::vector<IntTriple>::iterator candidate = partners.begin(); candidate != partners.end();
                 ++candidate) {
                int x = (*candidate)[1];
                int y = (*candidate)[2];
                if (squaredLength(plasma::Vector<2, int>(x - place[1], y - place[2])) <= 16) {
                    int kind = kindAt(x, y);
                    if (kind == MissionKindNone ||
                        (kind == MissionKind_15 && m_partners.find(std::array<int, 2> {{x, y}}) == m_partners.end())) {
                        float distance =
                            (float)squaredLength(world->placeCenter(x, y) - plasma::Vector<2, int>(center));
                        if (nearest < 0.0f || nearest > distance) {
                            nearest = distance;
                            partner = std::array<int, 2> {{x, y}};
                        }
                    }
                }
            }
            if (nearest >= 0.0f) {
                m_partners[std::array<int, 2> {place[1], place[2]}] = partner;
                m_partners[std::array<int, 2>(partner)] = std::array<int, 2> {place[1], place[2]};
                m_kinds[std::array<int, 2>(partner)] = MissionKindExchangePartner;
                m_values[std::array<int, 2>(partner)] = m_values[std::array<int, 2> {place[1], place[2]}];
            }
        }
    }

    m_flags = 0;
    for (const std::pair<const std::array<int, 2>, unsigned int> &entry : m_kinds) {
        if (entry.second == 0) {
            continue;
        }
        switch (entry.second) {
        case MissionKindHarpPlacement:
            m_flags |= MissionFlagHarp;
            m_specialRooms.push_back(StaticEntityGoldenGate);
            break;
        case MissionKindWhistlePlacement:
            m_flags |= MissionFlagWhistle;
            break;
        case MissionKindBellPlacement:
            m_flags |= MissionFlagBell;
            m_specialRooms.push_back(StaticEntityGrid);
            break;
        case MissionKindMagicBarriers:
            m_flags |= MissionFlagMagicBarriers;
            m_specialRooms.push_back(StaticEntityMagicBarrier);
            break;
        }
        int kind = entry.second;
        if (!(kind == MissionKindTreasureSpiritPlacement || kind == MissionKindArtifactReward ||
              kind == MissionKind_15 || kind == MissionKindAborigines || kind == MissionKindMagicBarriers ||
              kind == MissionKindFlameOfPower || kind == MissionKindManaPumps)) {
            m_places.push_back(plasma::Vector<2, int>(entry.first[0], entry.first[1]));
        }
        if (entry.second == MissionKindArtifactReward) {
            m_artifactPlaces.push_back(plasma::Vector<2, int>(entry.first[0], entry.first[1]));
        }
    }
}

Zone::Resident::Resident()
    : angle(0.0f),
      hostility(1),
      race(RaceGoblinMale),
      level(0),
      factionKind(0),
      factionVariant(-1),
      realm(0),
      classType(0),
      specialization(0),
      id(0),
      ownerId(0),
      unused48(0),
      interactionState(0),
      behaviours(0),
      currentAbility(0),
      destination(NoCoordinate, NoCoordinate),
      requiredDefeatedCount(0),
      requiredInteractionState(0)
{
    name[0] = 0;
}

Zone::Resident::~Resident() {}

std::vector<std::string> &operator<<(std::vector<std::string> &text, const plasma::Vector<2, int> &location)
{
    text << location[0] << location[1];
    return text;
}

// A resident without an id gets the first free one from the zone's position on. A resident that had the id before is
// deleted.
void Zone::adoptResident(Resident *resident)
{
    if (resident->id == 0) {
        // The zone's coordinates, biased to be positive, packed into the high and low halves and shifted up six bits
        // for the residents' serial numbers.
        resident->id = ((long long)(m_position[1] + 0x1ffffff) << 32 | (long long)(m_position[0] + 0x1ffffff)) << 6;
        while (m_residents.find(resident->id) != m_residents.end()) {
            resident->id++;
        }
    }
    if (m_residents[resident->id]) {
        delete m_residents[resident->id];
    }
    m_residents[resident->id] = resident;
}

int Missions::chooseRoomAt(int x, int y)
{
    if (m_specialRooms.empty() || random(x + 171940, y) % 4 == 0) {
        return StaticEntityGate;
    }
    return m_specialRooms[random(x + 564613, y) % m_specialRooms.size()];
}

int Missions::determineTopic(int x, int y)
{
    World *world = m_world;
    bool large = world->isLargePlace(x, y);
    if (large) {
        x = floorDivide(x, 2) * 2;
        y = floorDivide(y, 2) * 2;
    }
    int type = world->locationType(x, y, large);
    int kind = kindAt(x, y);
    if (kind == MissionKindExchangePartner) {
        return MissionErrand;
    }
    if (kind == MissionKindMonster) {
        return MissionMonster;
    }
    if (kind == MissionKindAborigines || type == PlaceArena) {
        return MissionAborigines;
    }
    if (kind == MissionKindSupplierGnome) {
        std::vector<int> topics;
        switch (type) {
        default:
            topics.push_back(MissionFolkCamp);
            topics.push_back(MissionOgreCamp);
            topics.push_back(MissionWitchCamp);
            topics.push_back(MissionSkeletonCamp);
            topics.push_back(MissionMonstrosityCamp);
            topics.push_back(MissionNecromancerCamp);
            topics.push_back(MissionDarkCultistCamp);
            topics.push_back(MissionDemonMarauderCamp);
            break;
        case PlaceAltar:
            topics = {MissionDarkCultistCamp, MissionDemonMarauderCamp};
            break;
        case PlaceOldHut:
            topics = {MissionWitchCamp};
            break;
        case PlaceCrypt:
        case PlaceGraveyard:
            topics = {MissionSkeletonCamp, MissionMonstrosityCamp, MissionNecromancerCamp};
            break;
        }
        return topics[random(x + 983, y + 88392) % topics.size()];
    }
    Item reward = taskRewardAt(x, y);
    std::vector<int> topics;
    if (m_world->dungeonType(x, y, type, large) != PlaceNone) {
        return MissionDungeon;
    }
    switch (type) {
    case PlaceCastle:
        return MissionCastle;
    case PlaceArena:
        topics.push_back(MissionAborigines);
        break;
    case PlaceOldHut:
        topics.push_back(MissionWitchCamp);
        break;
    case PlaceFarm:
        topics.push_back(MissionHornetsNests);
        topics.push_back(MissionMolehills);
        break;
    case PlaceLake:
    case PlaceManaPump:
    case PlaceTree:
    case PlaceForest:
    case PlaceCanyon:
    case PlaceRocks:
    case PlaceHill:
    case PlaceGrove:
    case PlaceGlade:
        if (!large) {
            topics.push_back(MissionStatues);
        }
        topics.push_back(MissionSlimeInvasion);
        topics.push_back(MissionUndeadInvasion);
        topics.push_back(MissionDemonInvasion);
        topics.push_back(MissionEmpireSoldierInvasion);
        topics.push_back(MissionInsectInvasion);
        topics.push_back(MissionPlantCreatureInvasion);
        break;
    case PlaceCrypt:
    case PlaceGraveyard:
        topics.push_back(MissionUnholyTombs);
        topics.push_back(MissionBonePiles);
        topics.push_back(MissionUndeadInvasion);
        topics.push_back(MissionSkeletonCamp);
        topics.push_back(MissionMonstrosityCamp);
        topics.push_back(MissionNecromancerCamp);
        topics.push_back(MissionCult);
        topics.push_back(MissionDarkSite);
        break;
    case PlaceAltar:
        topics.push_back(MissionDarkCultistCamp);
        topics.push_back(MissionDemonMarauderCamp);
        topics.push_back(MissionDemonInvasion);
        topics.push_back(MissionDemonPortals);
        topics.push_back(MissionDemonCult);
        break;
    case PlaceBeach:
        topics.push_back(MissionSeaCrabInvasion);
        break;
    default:
        if (!large) {
            topics.push_back(MissionDemonPortals);
            topics.push_back(MissionSlimeWells);
        }
        topics.push_back(MissionSlimeInvasion);
        topics.push_back(MissionUndeadInvasion);
        topics.push_back(MissionDemonInvasion);
        topics.push_back(MissionEmpireSoldierInvasion);
        topics.push_back(MissionInsectInvasion);
        topics.push_back(MissionPlantCreatureInvasion);
        topics.push_back(MissionBand);
        topics.push_back(MissionFolkCamp);
        topics.push_back(MissionOgreCamp);
        topics.push_back(MissionWitchCamp);
        topics.push_back(MissionSkeletonCamp);
        topics.push_back(MissionMonstrosityCamp);
        topics.push_back(MissionNecromancerCamp);
        topics.push_back(MissionDarkCultistCamp);
        topics.push_back(MissionDemonMarauderCamp);
        topics.push_back(MissionCult);
        topics.push_back(MissionDarkSite);
        topics.push_back(MissionDemonCult);
        break;
    }
    // A place without a reward changes its topic with the day.
    unsigned int draw = random(x + 983, y + 88392);
    if (reward.category == ItemCategoryNone) {
        draw += m_world->m_state.m_day;
    }
    return topics[hash(draw) % topics.size()];
}

Item fetchItem(int locationType, int x, int y, int rarity, const plasma::Vector<2, int> &region)
{
    Item item;
    item.category = ItemCategoryQuest;
    if (locationType == PlaceTree) {
        item.id = ((x + y * 64) % 2 + 2) % 2 + QuestItemFruit;
    } else {
        item.id = ((x + y * 64) % 2 + 2) % 2 + QuestItemHerb;
    }
    item.region = region;
    item.modifier = random(x + 12168462, y);
    item.formulaCategory = random(x + 233402, y);
    item.rarity = rarity;
    return item;
}

// What the region's missions put into a zone, by the locationType of the zone's place (nothing for a place without
// one): - 6..13: an item of category 24, whose id goes by the locationType, lying at one of the place's buildings
// (isPlaceBuilding); - 20: generateDarkSite with the demon portal; - 19: a type 88 static entity in a building of that
// type within four cells of the centre of a type 22 place, then generateBoss with the witch; - 21: at a building of
// type 52 (Zone::generate builds it with as many storeys as World::towerStoreyCount says) the place's namesake with a
// pet of race 289 (recorded in the zone's m_missionCreatures) at the positions of the top storey, and on every storey
// of the nine cells around the building creatures of a type drawn for the storey from the creature tables, the levels
// rising from 1 to the place's; - 22: at each building of type 56 a resident who carries an item of category 22, id
// 73, and a type 126 static entity that holds the same item; - 23: creatures of the race 236 list: with a pet of race
// 240 (recorded in m_trackedMissionCreatureIds) at the buildings of type 68, alone on an eighth of the place's other
// cells, and at the buildings of type 66 with a pet of race 239 (recorded in m_missionCreatures) and, one time in two,
// a creature of race 262 that becomes the first one's owner; - every other locationType: generatePlace.
void Missions::generate(Zone *zone)
{
    zone->m_missionCreatures.clear();
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int kind = kindAt(placeX, placeY);
    if (kind == MissionKindNone) {
        return;
    }
    int type = world->locationType(placeX, placeY, large);
    int level = requestCount(placeX, placeY);
    if (kind == MissionKindHangGliderPlacement || kind == MissionKindBoatPlacement ||
        kind == MissionKindClimbingSpikesPlacement || kind == MissionKindReinsPlacement ||
        kind == MissionKindHarpPlacement || kind == MissionKindWhistlePlacement || kind == MissionKindBellPlacement ||
        kind == MissionKindTreasureSpiritPlacement) {
        World *generator = m_world;
        plasma::Vector<2, int> center = generator->placeCenter(placeX, placeY);
        std::vector<plasma::Vector<2, int>> buildings;
        int minX = floorDivide(placeX * BlocksPerPlace, BlocksPerCell);
        int maxX = large ? floorDivide(placeX * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                         : floorDivide(placeX * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
        int minY = floorDivide(placeY * BlocksPerPlace, BlocksPerCell);
        int maxY = large ? floorDivide(placeY * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                         : floorDivide(placeY * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
        int targetType = generator->missionTargetType(type, placeX, placeY, large);
        float threshold = PlaceCentreInfluence;
        if (targetType != 0) {
            threshold = PlaceCoreInfluence;
        }
        for (int x = minX; x < maxX; x++) {
            for (int y = minY; y < maxY; y++) {
                float influence =
                    m_world->placeInfluence(type, large,
                                            m_world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                          y * BlocksPerCell + BlocksPerCell / 2));
                if (influence > threshold && isPlaceBuilding(type, x, y, targetType)) {
                    buildings.push_back(plasma::Vector<2, int>(x, y));
                }
            }
        }
        if (buildings.empty()) {
            buildings.push_back(
                plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
        }
        plasma::Vector<2, int> home = buildings[random(placeX + 16786, placeY) % buildings.size()];
        int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
        int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
        int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
        int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
        if (home[0] >= zoneMinX && home[0] < zoneMaxX && home[1] >= zoneMinY && home[1] < zoneMaxY) {
            ZoneItem lying;
            lying.item.category = ItemCategorySpecial;
            switch (kind) {
            case MissionKindBoatPlacement:
                lying.item.id = SpecialItemBoat;
                break;
            case MissionKindClimbingSpikesPlacement:
                lying.item.id = SpecialItemClimbingSpikes;
                break;
            case MissionKindReinsPlacement:
                lying.item.id = SpecialItemReins;
                break;
            case MissionKindHarpPlacement:
                lying.item.id = SpecialItemHarp;
                break;
            case MissionKindWhistlePlacement:
                lying.item.id = SpecialItemWhistle;
                break;
            case MissionKindBellPlacement:
                lying.item.id = SpecialItemBell;
                break;
            case MissionKindTreasureSpiritPlacement:
                lying.item.id = SpecialItemTreasureSpirit;
                break;
            default:
                lying.item.id = SpecialItemHangGlider;
                break;
            }
            lying.item.region = m_world->findRegion(center);
            lying.flags |= ZoneItemFlagPerPlayer;
            lying.position = zone->randomSpawnPosition(home[0], home[1]);
            lying.rotation = (float)rand() * 360.0f / (float)RAND_MAX;
            zone->m_items.push_back(lying);
        }
    } else if (kind == MissionKindDemonPortal) {
        generateDarkSite(zone, RaceDemonPortal, RaceDarkCultistMale, FactionCultOfDoom);
    } else if (kind == MissionKindWitchCurse) {
        int zonePlaceX = World::placeIndex(zone->m_position[0] * BlocksPerZone);
        int zonePlaceY = World::placeIndex(zone->m_position[1] * BlocksPerZone);
        World *generator = m_world;
        bool zoneLarge = generator->isLargePlace(zonePlaceX, zonePlaceY);
        if (zoneLarge) {
            zonePlaceX = floorDivide(zonePlaceX, 2) * 2;
            zonePlaceY = floorDivide(zonePlaceY, 2) * 2;
        }
        if (generator->locationType(zonePlaceX, zonePlaceY, zoneLarge) == PlaceOldHut) {
            plasma::Vector<2, int> center = m_world->placeCenter(zonePlaceX, zonePlaceY);
            int minX = floorDivide(center[0], BlocksPerCell) - OldHutSearchCells;
            int maxX = floorDivide(center[0], BlocksPerCell) + OldHutSearchCells;
            int minY = floorDivide(center[1], BlocksPerCell) - 4;
            int maxY = floorDivide(center[1], BlocksPerCell) + 4;
            int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
            int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
            int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
            int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
            std::vector<plasma::Vector<2, int>> cells;
            for (int x = minX; x < maxX; x++) {
                for (int y = minY; y < maxY; y++) {
                    if (m_world->zoneBuildingType(x, y) == BuildingOldHutChimney) {
                        plasma::Vector<2, int> cell(x, y);
                        cells.push_back(cell);
                    }
                }
            }
            if (!cells.empty()) {
                plasma::Vector<2, int> home = cells[rand() % cells.size()];
                if (home[0] >= zoneMinX && home[0] < zoneMaxX && home[1] >= zoneMinY && home[1] < zoneMaxY) {
                    StaticEntity entity;
                    entity.type = StaticEntityCauldron;
                    entity.position = zone->choosePosition(home[0], home[1], PositionSheltered);
                    entity.orientation = random(zonePlaceX + 206496, zonePlaceY) % 4;
                    zone->putOnGround(entity, false);
                    zone->addStaticEntity(entity);
                }
            }
        }
        generateBoss(zone, RaceWitch, 0, ClassNone, 0);
    } else if (kind == MissionKindMagicBarriers) {
        int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
        int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
        int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
        int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
        int topLevel = requestCount(placeX, placeY);
        int towerX = NoCoordinate;
        int towerY = NoCoordinate;
        int storeys = 0;
        for (int x = zoneMinX; x < zoneMaxX; x++) {
            for (int y = zoneMinY; y < zoneMaxY; y++) {
                if (m_world->zoneBuildingType(x, y) == BuildingTower) {
                    towerX = x;
                    towerY = y;
                    storeys = m_world->towerStoreyCount(x, y);
                    Zone::Resident *namesake = new Zone::Resident();
                    namesake->hostility = HostilityEnemy;
                    namesake->setNamesake(placeX, placeY, type);
                    namesake->angle = (float)(rand() % 4 * 90);
                    namesake->level = topLevel;
                    namesake->appearance.appearanceFlags |= AppearanceFlagTinted | AppearanceFlagMissionResident;
                    namesake->destination = plasma::Vector<2, int>(placeX, placeY);
                    fillAppearance(namesake->race, namesake->appearance, nullptr, false);
                    equip(namesake->hostility, namesake->race, namesake->level, namesake->appearance.appearanceFlags,
                          namesake->factionKind, namesake->classType, namesake->specialization, namesake->equipment,
                          namesake->specialSkills, namesake->summonAppearanceMode, namesake->summonRaces,
                          namesake->inventory);
                    namesake->position = zone->randomPosition(x, y, PositionStorey0Floor + storeys - 1);
                    zone->adoptResident(namesake);
                    Zone::Resident *pet = new Zone::Resident();
                    pet->hostility = HostilityInteractiveObject;
                    pet->race = RaceMagicCrystal;
                    pet->angle = (float)(rand() % 4 * 90);
                    pet->level = topLevel;
                    pet->appearance.appearanceFlags |= AppearanceFlagMissionResident;
                    pet->ownerId = namesake->id;
                    pet->destination = plasma::Vector<2, int>(placeX, placeY);
                    fillAppearance(pet->race, pet->appearance, nullptr, false);
                    equip(pet->hostility, pet->race, pet->level, pet->appearance.appearanceFlags, pet->factionKind,
                          pet->classType, pet->specialization, pet->equipment, pet->specialSkills,
                          pet->summonAppearanceMode, pet->summonRaces, pet->inventory);
                    pet->position = zone->randomPosition(x, y, PositionStorey0Floor + storeys - 1);
                    zone->adoptResident(pet);
                    zone->m_missionCreatures.push_back(pet->id);
                }
            }
        }
        if (storeys != 0) {
            srand(fastRandom(placeX + 8983, placeY + 10934));
            // Each storey's creature type and the appearance flags it adds.
            std::vector<std::array<int, 2>> floors(storeys);
            for (std::vector<std::array<int, 2>>::iterator it = floors.begin(); it != floors.end(); ++it) {
                int race;
                // One of eight populations: demons, the undead, plants, slimes, casters, insects, folk or boosted
                // folk.
                switch (rand() % 8) {
                case 1:
                    race = randomDemonCreature();
                    break;
                case 2:
                    race = randomUndead();
                    break;
                case 3:
                    race = randomPlantCreature();
                    break;
                case 4:
                    race = RaceGreenSlime + rand() % (RaceBlueSlime - RaceGreenSlime + 1);
                    break;
                case 5:
                    race = randomCaster();
                    break;
                case 6:
                    race = randomInsect();
                    break;
                case 7:
                    race = randomFolkRace(BiomeNone);
                    (*it)[1] = AppearanceFlagSpiritForm;
                    break;
                default:
                    race = randomFolkRace(BiomeNone);
                    break;
                }
                (*it)[0] = race;
            }
            int bottomLevel = (std::min)(1, topLevel);
            for (int x = towerX - 1; x <= towerX + 1; x++) {
                for (int y = towerY - 1; y <= towerY + 1; y++) {
                    for (int i = 0; i < storeys; i++) {
                        if (x == towerX && y == towerY && i == storeys - 1) {
                            continue;
                        }
                        if (rand() % 2 != 0 && zone->positionCount(x, y, PositionStorey0Floor + i) != 0) {
                            Zone::Resident *creature = new Zone::Resident();
                            creature->hostility = HostilityEnemy;
                            creature->race = floors[i][0];
                            creature->angle = (float)(rand() % 4 * 90);
                            creature->level = (topLevel - bottomLevel) * i / (storeys - 1) + bottomLevel;
                            creature->appearance.appearanceFlags |= floors[i][1] | AppearanceFlagMissionResident;
                            creature->behaviours = BehaviourWander | BehaviourUseFurniture;
                            fillAppearance(creature->race, creature->appearance, nullptr, false);
                            equip(creature->hostility, creature->race, creature->level,
                                  creature->appearance.appearanceFlags, creature->factionKind, creature->classType,
                                  creature->specialization, creature->equipment, creature->specialSkills,
                                  creature->summonAppearanceMode, creature->summonRaces, creature->inventory);
                            creature->position = zone->randomPosition(x, y, PositionStorey0Floor + i);
                            zone->adoptResident(creature);
                        }
                    }
                }
            }
        }
    } else if (kind == MissionKindFlameOfPower) {
        int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
        int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
        int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
        int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
        int itemLevel = requestCount(placeX, placeY);
        for (int x = zoneMinX; x < zoneMaxX; x++) {
            for (int y = zoneMinY; y < zoneMaxY; y++) {
                if (m_world->zoneBuildingType(x, y) == BuildingCircleOfPowerSite) {
                    Item item;
                    item.category = ItemCategoryQuest;
                    item.id = QuestItemEternalEmber;
                    item.rarity = itemLevel;
                    item.region = m_region;
                    Zone::Resident *holder = new Zone::Resident();
                    holder->hostility = HostilityEnemy;
                    holder->race = randomFolkRace(BiomeNone);
                    holder->classType = ClassWarrior;
                    holder->factionKind = FactionRestlessWarrior;
                    holder->angle = (float)(rand() % 4 * 90);
                    holder->level = itemLevel;
                    holder->activatorCells.insert(std::array<int, 2> {x, y});
                    holder->appearance.appearanceFlags |=
                        AppearanceFlagTinted | AppearanceFlagMissionResident | AppearanceFlagSpiritForm;
                    holder->behaviours = BehaviourWander | BehaviourUseFurniture;
                    holder->inventory.add(item, 1);
                    fillAppearance(holder->race, holder->appearance, nullptr, false);
                    equip(holder->hostility, holder->race, holder->level, holder->appearance.appearanceFlags,
                          holder->factionKind, holder->classType, holder->specialization, holder->equipment,
                          holder->specialSkills, holder->summonAppearanceMode, holder->summonRaces, holder->inventory);
                    holder->position = zone->randomSpawnPosition(x, y);
                    zone->adoptResident(holder);
                    StaticEntity entity;
                    entity.type = StaticEntityBrazier;
                    entity.orientation = rand() % 4;
                    entity.position = zone->randomSpawnPosition(x, y);
                    entity.item = item;
                    zone->putOnGround(entity, true);
                    zone->addStaticEntity(entity);
                }
            }
        }
    } else if (kind == MissionKindManaPumps) {
        plasma::Vector<2, int> center = m_world->placeCenter(placeX, placeY);
        std::vector<plasma::Vector<2, int>> manaPipeEndpointCells;
        std::vector<plasma::Vector<2, int>> manaPumpCells;
        std::vector<plasma::Vector<2, int>> plots;
        int minX = floorDivide(placeX * BlocksPerPlace, BlocksPerCell);
        int maxX = large ? floorDivide(placeX * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                         : floorDivide(placeX * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
        int minY = floorDivide(placeY * BlocksPerPlace, BlocksPerCell);
        int maxY = large ? floorDivide(placeY * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                         : floorDivide(placeY * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
        for (int x = minX; x < maxX; x++) {
            for (int y = minY; y < maxY; y++) {
                int building = m_world->zoneBuildingType(x, y);
                if (building == BuildingManaPipeEndpoint) {
                    plasma::Vector<2, int> cell(x, y);
                    manaPipeEndpointCells.push_back(cell);
                } else if (building == BuildingManaPump) {
                    plasma::Vector<2, int> cell(x, y);
                    manaPumpCells.push_back(cell);
                } else if (m_world->placeInfluence(type, large,
                                                   m_world->placeSquaredDistance(
                                                       center, x * BlocksPerCell + BlocksPerCell / 2,
                                                       y * BlocksPerCell + BlocksPerCell / 2)) > PlaceCoreInfluence) {
                    plasma::Vector<2, int> cell(x, y);
                    plots.push_back(cell);
                }
            }
        }
        shuffleWithRand(plots.begin(), plots.end());
        // Eight patrol plots, or an eighth of them where there are more than 64 (the list is resized to eight when
        // shorter).
        plots.resize((std::max)(1, (std::max)(8, (int)plots.size() / 8)));
        int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
        int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
        int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
        int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
        for (std::vector<plasma::Vector<2, int>>::iterator it = manaPipeEndpointCells.begin();
             it != manaPipeEndpointCells.end(); ++it) {
            if ((*it)[0] >= zoneMinX && (*it)[0] < zoneMaxX && (*it)[1] >= zoneMinY && (*it)[1] < zoneMaxY) {
                Zone::Resident *owner = new Zone::Resident();
                owner->hostility = HostilityEnemy;
                owner->level = (std::max)(0, level - rand() % 2);
                owner->race = randomRace(RaceEmpireSoldierMale);
                owner->factionKind = FactionSteelEmpire;
                owner->appearance.appearanceFlags = AppearanceFlagTinted | AppearanceFlagMissionResident;
                owner->destination = plasma::Vector<2, int>(placeX, placeY);
                owner->position = zone->randomSpawnPosition((*it)[0], (*it)[1]);
                owner->behaviours = BehaviourWander;
                fillAppearance(owner->race, owner->appearance, nullptr, false);
                equip(owner->hostility, owner->race, owner->level, owner->appearance.appearanceFlags,
                      owner->factionKind, owner->classType, owner->specialization, owner->equipment,
                      owner->specialSkills, owner->summonAppearanceMode, owner->summonRaces, owner->inventory);
                zone->adoptResident(owner);
                Zone::Resident *pet = new Zone::Resident();
                pet->hostility = HostilityInteractiveObject;
                pet->level = level;
                pet->race = RaceManaInductor;
                pet->appearance.appearanceFlags = AppearanceFlagLarge | AppearanceFlagMissionResident;
                pet->destination = plasma::Vector<2, int>(placeX, placeY);
                pet->position = zone->choosePosition((*it)[0], (*it)[1], PositionAny);
                pet->angle = (float)(rand() % 4 * 90);
                pet->ownerId = owner->id;
                fillAppearance(pet->race, pet->appearance, nullptr, false);
                equip(pet->hostility, pet->race, pet->level, pet->appearance.appearanceFlags, pet->factionKind,
                      pet->classType, pet->specialization, pet->equipment, pet->specialSkills,
                      pet->summonAppearanceMode, pet->summonRaces, pet->inventory);
                zone->adoptResident(pet);
                zone->m_trackedMissionCreatureIds.push_back(pet->id);
            }
        }
        for (std::vector<plasma::Vector<2, int>>::iterator it = plots.begin(); it != plots.end(); ++it) {
            if ((*it)[0] >= zoneMinX && (*it)[0] < zoneMaxX && (*it)[1] >= zoneMinY && (*it)[1] < zoneMaxY) {
                Zone::Resident *wanderer = new Zone::Resident();
                wanderer->hostility = HostilityEnemy;
                wanderer->level = (std::max)(0, level - rand() % 2);
                wanderer->race = randomRace(RaceEmpireSoldierMale);
                wanderer->factionKind = FactionSteelEmpire;
                wanderer->appearance.appearanceFlags = AppearanceFlagMissionResident;
                wanderer->destination = plasma::Vector<2, int>(placeX, placeY);
                wanderer->position = zone->randomSpawnPosition((*it)[0], (*it)[1]);
                wanderer->behaviours = BehaviourWander;
                fillAppearance(wanderer->race, wanderer->appearance, nullptr, false);
                equip(wanderer->hostility, wanderer->race, wanderer->level, wanderer->appearance.appearanceFlags,
                      wanderer->factionKind, wanderer->classType, wanderer->specialization, wanderer->equipment,
                      wanderer->specialSkills, wanderer->summonAppearanceMode, wanderer->summonRaces,
                      wanderer->inventory);
                zone->adoptResident(wanderer);
            }
        }
        for (std::vector<plasma::Vector<2, int>>::iterator it = manaPumpCells.begin(); it != manaPumpCells.end();
             ++it) {
            if ((*it)[0] >= zoneMinX && (*it)[0] < zoneMaxX && (*it)[1] >= zoneMinY && (*it)[1] < zoneMaxY) {
                Zone::Resident *leader = new Zone::Resident();
                leader->hostility = HostilityEnemy;
                leader->level = level;
                leader->race = randomRace(RaceEmpireSoldierMale);
                leader->factionKind = FactionSteelEmpire;
                leader->appearance.appearanceFlags = AppearanceFlagTinted | AppearanceFlagMissionResident;
                leader->destination = plasma::Vector<2, int>(placeX, placeY);
                leader->position = zone->randomSpawnPosition((*it)[0], (*it)[1]);
                leader->behaviours = BehaviourWander;
                leader->requiredDefeatedCount = (int)manaPipeEndpointCells.size();
                fillAppearance(leader->race, leader->appearance, nullptr, false);
                equip(leader->hostility, leader->race, leader->level, leader->appearance.appearanceFlags,
                      leader->factionKind, leader->classType, leader->specialization, leader->equipment,
                      leader->specialSkills, leader->summonAppearanceMode, leader->summonRaces, leader->inventory);
                zone->adoptResident(leader);
                Zone::Resident *pet = new Zone::Resident();
                pet->hostility = HostilityInteractiveObject;
                pet->level = level;
                pet->race = RaceManaPump;
                pet->appearance.appearanceFlags = AppearanceFlagLarge | AppearanceFlagMissionResident;
                pet->destination = plasma::Vector<2, int>(placeX, placeY);
                pet->position = zone->choosePosition((*it)[0], (*it)[1], PositionAny);
                pet->angle = (float)(rand() % 4 * 90);
                pet->ownerId = leader->id;
                fillAppearance(pet->race, pet->appearance, nullptr, false);
                equip(pet->hostility, pet->race, pet->level, pet->appearance.appearanceFlags, pet->factionKind,
                      pet->classType, pet->specialization, pet->equipment, pet->specialSkills,
                      pet->summonAppearanceMode, pet->summonRaces, pet->inventory);
                zone->adoptResident(pet);
                zone->m_missionCreatures.push_back(pet->id);
                if (rand() % 2 != 0) {
                    Zone::Resident *master = new Zone::Resident();
                    master->hostility = HostilityEnemy;
                    master->level = level;
                    master->race = RaceManaMech;
                    master->factionKind = FactionSteelEmpire;
                    master->appearance.appearanceFlags = AppearanceFlagLarge | AppearanceFlagMissionResident;
                    master->destination = plasma::Vector<2, int>(placeX, placeY);
                    master->position = zone->randomSpawnPosition((*it)[0], (*it)[1]);
                    master->behaviours = BehaviourWander;
                    master->requiredDefeatedCount = (int)manaPipeEndpointCells.size();
                    fillAppearance(master->race, master->appearance, nullptr, false);
                    equip(master->hostility, master->race, master->level, master->appearance.appearanceFlags,
                          master->factionKind, master->classType, master->specialization, master->equipment,
                          master->specialSkills, master->summonAppearanceMode, master->summonRaces, master->inventory);
                    zone->adoptResident(master);
                    leader->ownerId = master->id;
                }
            }
        }
    } else {
        generatePlace(zone);
    }
}

// The camp of four groups of wild creatures at the centre of a place: one to three (or twice as many followers behind
// one leader) of a kind drawn for each group, the last group's kind the first one of the list, of the levels 1 to 4.
// Every group knows the creatures of the group before it, and the first creature of the last group carries the place's
// reward and is recorded in the zone's m_missionCreatures.
void Missions::generateAborigines(Zone *zone)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int type = world->locationType(placeX, placeY, large);
    srand(random(placeX + 39441, placeY));
    taskRewardAt(placeX, placeY);
    World *generator = m_world;
    plasma::Vector<2, int> center = generator->placeCenter(placeX, placeY);
    generator->missionTargetType(type, placeX, placeY, large);
    plasma::Vector<2, int> home(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell));
    int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
    int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
    int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    if (home[0] >= zoneMinX && home[0] < zoneMaxX && home[1] >= zoneMinY && home[1] < zoneMaxY) {
        std::list<long long> previous;
        for (int i = 0; i < 4; i++) {
            int count = random(placeX + i * 1106 + 196056, placeY) % 3 + 1;
            std::list<long long> group;
            std::vector<int> races;
            int factionKind = 0;
            int realm = 0;
            unsigned int flags;
            // One of ten race families per wave; the last wave is the default (giants).
            int kind = rand() % 10;
            switch (i != 3 ? kind : 0) {
            case 1:
                races = {randomFolkRace(BiomeNone)};
                flags = AppearanceFlagTinted;
                break;
            case 2:
                races = {RacePlantCreature, RaceRadishCreature, RaceOnionling, RaceDesertOnionling,
                         RaceChiling,       RaceHabanero,       RaceBloomling};
                flags = AppearanceFlagTinted;
                break;
            case 3:
                races = {RaceBarkBeetle, RaceSnoutBeetle, RaceLemonBeetle, RaceFireBeetle};
                flags = AppearanceFlagTinted;
                break;
            case 4:
                races = {RaceBlueSlime};
                flags = AppearanceFlagTinted;
                break;
            case 5:
                races = {RaceVampire,     RaceNecromancer, RaceUndeadMale,   RaceSkeletonKnight,
                         RaceSkeleton,    RaceMonstrosity, RaceZombie,       RaceFrightener,
                         RaceNecromancer, RaceSkeletonDog, RaceSkeletonHorse};
                flags = AppearanceFlagTinted;
                factionKind = FactionUnholyPact;
                break;
            case 6:
                races = {RaceDemonMarauder, RaceImp, RaceDevourer, RaceCrawler, RaceFlyingEye, RaceDarkCultistMale};
                flags = AppearanceFlagTinted;
                factionKind = FactionCultOfDoom;
                break;
            case 7:
                races = {RaceEnhancedOrcMale};
                flags = AppearanceFlagTinted;
                factionKind = FactionBloodaxeClan;
                break;
            case 8: {
                Realm people(m_world->mortalRealm(placeX, placeY, large));
                races = {people.native(rand())};
                factionKind = people.form();
                realm = people.id;
                flags = AppearanceFlagTinted;
                if (!people.hasPeople()) {
                    flags = AppearanceFlagTinted | AppearanceFlagSpiritForm;
                }
                break;
            }
            case 9:
                races = {RaceEmpireSoldierMale, RaceEmpireSoldierFemale, RaceManaMech};
                flags = AppearanceFlagTinted;
                factionKind = 3;
                break;
            default:
                races = {RaceTroll,     RaceDarkTroll, RaceYeti,      RaceCyclops,    RaceSaurian, RaceLich,
                         RaceRuneGiant, RaceGolem,     RaceSnowGolem, RaceEmberGolem, RaceMammoth};
                flags = AppearanceFlagLarge;
                count = 1;
                break;
            }
            int leaders = count;
            if (count > 1 && rand() % 2 != 0) {
                count *= 2;
                leaders = 1;
            }
            for (int j = 0; j < count; j++) {
                if (j >= leaders) {
                    flags &= ~AppearanceFlagTinted;
                }
                Zone::Resident *creature = new Zone::Resident();
                creature->hostility = HostilityEnemy;
                creature->level = i + 1;
                creature->race = randomRace(races[rand() % races.size()]);
                creature->behaviours = BehaviourWander;
                creature->appearance.appearanceFlags = flags | AppearanceFlagMissionResident;
                creature->destination = plasma::Vector<2, int>(placeX, placeY);
                creature->prerequisiteCreatureIds = previous;
                creature->factionKind = factionKind;
                creature->realm = realm;
                creature->requiredInteractionState = i * 2 + 1;
                fillAppearance(creature->race, creature->appearance, nullptr, false);
                equip(creature->hostility, creature->race, creature->level, creature->appearance.appearanceFlags,
                      creature->factionKind, creature->classType, creature->specialization, creature->equipment,
                      creature->specialSkills, creature->summonAppearanceMode, creature->summonRaces,
                      creature->inventory);
                creature->position = zone->randomSpawnPosition(home[0], home[1]);
                zone->adoptResident(creature);
                if (i == 3 && j == 0) {
                    Item reward = taskRewardAt(placeX, placeY);
                    if (reward.category != ItemCategoryNone) {
                        creature->inventory.add(reward, 1);
                    }
                    zone->m_missionCreatures.push_back(creature->id);
                }
                group.push_back(creature->id);
            }
            previous = group;
        }
    }
}

void Missions::generateArtifact(int unused, int x, int y, int &seed)
{
    Item item;
    item.category = ItemCategoryArtifact;
    item.id = hash(seed++) % (Artifact_ArtifactScepter + 1);
    item.modifier = hash(seed++);
    item.formulaCategory = m_world->mortalRealm(x, y, m_world->isLargePlace(x, y));
    item.region = plasma::Vector<2, int>(x, y);
    m_artifacts[std::array<int, 2> {x, y}] = item;
}

// A place held by a band of folk (topic 0): at one of the place's mission buildings a static entity of type 21 holding
// the band's loot (an item of category 22 with an id 0..19 by the cell), whose reward the zone keeps under the
// entity's index (m_objectRewards), and five to eight creatures of one folk race that remember the building; the first
// carries the loot.
void Missions::generateBand(Zone *zone)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int type = world->locationType(placeX, placeY, large);
    int level = requestCount(placeX, placeY);
    srand(random(placeX + 39441, placeY));
    int race = randomFolkRace(BiomeNone);
    plasma::Vector<2, int> center = m_world->placeCenter(placeX, placeY);
    Item loot;
    loot.category = ItemCategoryQuest;
    // One of the twenty quest loot items by the place cell (its coordinates modulo 64, as one index).
    loot.id = ((placeY % 64 + 64) % 64 * 64 + (placeX % 64 + 64) % 64) % 20;
    loot.region = m_world->findRegion(center);
    loot.rarity = (std::min)((int)ItemRarityYellow, level + 1);
    std::vector<plasma::Vector<2, int>> buildings;
    int minX = floorDivide(placeX * BlocksPerPlace, BlocksPerCell);
    int maxX = large ? floorDivide(placeX * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeX * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int minY = floorDivide(placeY * BlocksPerPlace, BlocksPerCell);
    int maxY = large ? floorDivide(placeY * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeY * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int targetType = m_world->missionTargetType(type, placeX, placeY, large);
    for (int x = minX; x < maxX; x++) {
        for (int y = minY; y < maxY; y++) {
            float influence =
                m_world->placeInfluence(type, large,
                                        m_world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                      y * BlocksPerCell + BlocksPerCell / 2));
            if (influence > PlaceCoreInfluence && isMissionBuilding(type, x, y, targetType)) {
                buildings.push_back(plasma::Vector<2, int>(x, y));
            }
        }
    }
    if (buildings.empty()) {
        buildings.push_back(
            plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
    }
    plasma::Vector<2, int> home = buildings[random(placeX + 16786, placeY) % buildings.size()];
    int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
    int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
    int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    if (home[0] >= zoneMinX && home[0] < zoneMaxX && home[1] >= zoneMinY && home[1] < zoneMaxY) {
        StaticEntity stash;
        stash.type = StaticEntityChestBase02;
        stash.item = loot;
        stash.position = zone->randomSpawnPosition(home[0], home[1]);
        stash.orientation = random(placeX + 206496, placeY) % 4;
        Item reward = taskRewardAt(placeX, placeY);
        if (reward.category != ItemCategoryNone) {
            zone->m_objectRewards[(int)zone->m_staticEntities.size()] = reward;
        }
        zone->putOnGround(stash, false);
        zone->addStaticEntity(stash);
        int count = random(placeX + 381262, placeY) % 4 + 5;
        for (int i = 0; i < count; i++) {
            Zone::Resident *member = new Zone::Resident();
            member->hostility = HostilityEnemy;
            member->level = level;
            member->race = randomRace(race);
            member->behaviours = BehaviourWander | BehaviourUseFurniture;
            member->appearance.appearanceFlags = AppearanceFlagMissionResident;
            member->destination = plasma::Vector<2, int>(placeX, placeY);
            if (i == 0) {
                member->appearance.appearanceFlags |= AppearanceFlagTinted;
                member->inventory.add(loot, 1);
            }
            fillAppearance(member->race, member->appearance, nullptr, false);
            equip(member->hostility, member->race, member->level, member->appearance.appearanceFlags,
                  member->factionKind, member->classType, member->specialization, member->equipment,
                  member->specialSkills, member->summonAppearanceMode, member->summonRaces, member->inventory);
            member->position = zone->randomSpawnPosition(home[0], home[1]);
            member->activatorCells.insert(std::array<int, 2> {home[0], home[1]});
            zone->adoptResident(member);
        }
    }
}

// The boss of a place (generate's for a kind 19 place, the witch): one creature of the race, of the place's level and
// with the form, class and appearance flags given, at one of the place's mission buildings, which it remembers
// (activatorCells). It carries the place's reward; without one it is recorded in the zone's m_missionCreatures.
void Missions::generateBoss(Zone *zone, int race, int factionKind, int classType, unsigned int flags)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int type = world->locationType(placeX, placeY, large);
    int level = requestCount(placeX, placeY);
    srand(random(placeX + 39441, placeY));
    Item reward = taskRewardAt(placeX, placeY);
    World *generator = m_world;
    plasma::Vector<2, int> center = generator->placeCenter(placeX, placeY);
    generator->findRegion(center);
    std::vector<plasma::Vector<2, int>> buildings;
    int minX = floorDivide(placeX * BlocksPerPlace, BlocksPerCell);
    int maxX = large ? floorDivide(placeX * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeX * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int minY = floorDivide(placeY * BlocksPerPlace, BlocksPerCell);
    int maxY = floorDivide(
        large ? placeY * BlocksPerPlace + 2 * BlocksPerPlace : placeY * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int targetType = m_world->missionTargetType(type, placeX, placeY, large);
    if (type != PlaceArena) {
        for (int x = minX; x < maxX; x++) {
            for (int y = minY; y < maxY; y++) {
                float influence =
                    m_world->placeInfluence(type, large,
                                            m_world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                          y * BlocksPerCell + BlocksPerCell / 2));
                if (influence > PlaceCoreInfluence && isMissionBuilding(type, x, y, targetType)) {
                    buildings.push_back(plasma::Vector<2, int>(x, y));
                }
            }
        }
    }
    if (buildings.empty()) {
        buildings.push_back(
            plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
    }
    plasma::Vector<2, int> home = buildings[random(placeX + 16786, placeY) % buildings.size()];
    int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
    int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
    int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    if (home[0] >= zoneMinX && home[0] < zoneMaxX && home[1] >= zoneMinY && home[1] < zoneMaxY) {
        Zone::Resident *monster = new Zone::Resident();
        monster->hostility = HostilityEnemy;
        monster->factionKind = factionKind;
        monster->classType = classType;
        monster->level = level;
        monster->race = randomRace(race);
        monster->behaviours = BehaviourWander | BehaviourUseFurniture;
        monster->appearance.appearanceFlags = AppearanceFlagMissionResident;
        monster->destination = plasma::Vector<2, int>(placeX, placeY);
        monster->appearance.appearanceFlags |= flags | AppearanceFlagTinted;
        if (reward.category != ItemCategoryNone) {
            monster->inventory.add(reward, 1);
        }
        fillAppearance(monster->race, monster->appearance, nullptr, false);
        equip(monster->hostility, monster->race, monster->level, monster->appearance.appearanceFlags,
              monster->factionKind, monster->classType, monster->specialization, monster->equipment,
              monster->specialSkills, monster->summonAppearanceMode, monster->summonRaces, monster->inventory);
        monster->position = zone->randomSpawnPosition(home[0], home[1]);
        monster->activatorCells.insert(std::array<int, 2> {home[0], home[1]});
        zone->adoptResident(monster);
        if (reward.category == ItemCategoryNone) {
            zone->m_missionCreatures.push_back(monster->id);
        }
    }
}

// A place where creatures hold a captive (the topics 9..16): at one of the place's mission buildings a static entity
// whose type goes by the camp's kind (84..87 by the cell, 88, 90, 91 or 89), scaled to the captive and holding its id,
// and the captive itself, a peaceful resident of race with the class and factionKind given, which carries the place's
// reward. The kind is drawn by the cell from {0, 3}, from {4, 2} for the keepers' forms 4 and 5, and is 1 for a list
// of keepers (witch, ogre, the undead, the demons). For the kinds 1 and 3 five to eight keepers of keeperRace with
// keeperForm stand around the captive and remember the building. For the kinds 0, 2 and 4 the entity holds the camp's
// loot (category 22, as generateBand's) and the keepers are at another of the place's mission sites: one that carries
// the loot, and in a small place four to six more.
void Missions::generateCamp(Zone *zone, int race, int classType, int captiveFactionKind, int keeperRace,
                            int keeperFactionKind)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int type = world->locationType(placeX, placeY, large);
    int level = requestCount(placeX, placeY);
    std::vector<int> kinds = {Captivity_Cage, Captivity_Pole};
    switch (keeperFactionKind) {
    case FactionCultOfDoom:
        kinds = {Captivity_TortureRack, Captivity_LegIrons};
        break;
    case FactionUnholyPact:
        kinds = {Captivity_TortureRack, Captivity_LegIrons};
        break;
    }
    if (keeperRace == RaceWitch || keeperRace == RaceOgre || keeperRace == RaceSkeleton ||
        keeperRace == RaceSkeletonKnight || keeperRace == RaceZombie || keeperRace == RaceDemonMarauder ||
        keeperRace == RaceMonstrosity) {
        kinds = {1};
    }
    int kind = kinds[random(placeX + 58095, placeY) % kinds.size()];
    bool apart = kind == Captivity_Cage || kind == Captivity_LegIrons || kind == Captivity_TortureRack;
    srand(random(placeX + 39441, placeY));
    World *generator = m_world;
    plasma::Vector<2, int> center = generator->placeCenter(placeX, placeY);
    Item loot;
    loot.category = ItemCategoryQuest;
    // One of the twenty quest loot items by the place cell (its coordinates modulo 64, as one index).
    loot.id = ((placeY % 64 + 64) % 64 * 64 + (placeX % 64 + 64) % 64) % 20;
    loot.region = generator->findRegion(center);
    loot.rarity = (std::min)((int)ItemRarityYellow, level + 1);
    std::vector<plasma::Vector<2, int>> buildings;
    int minX = floorDivide(placeX * BlocksPerPlace, BlocksPerCell);
    int maxX = large ? floorDivide(placeX * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeX * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int minY = floorDivide(placeY * BlocksPerPlace, BlocksPerCell);
    int maxY = large ? floorDivide(placeY * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeY * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int targetType = m_world->missionTargetType(type, placeX, placeY, large);
    for (int x = minX; x < maxX; x++) {
        for (int y = minY; y < maxY; y++) {
            float influence =
                m_world->placeInfluence(type, large,
                                        m_world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                      y * BlocksPerCell + BlocksPerCell / 2));
            if (influence > PlaceCoreInfluence && isMissionBuilding(type, x, y, targetType)) {
                buildings.push_back(plasma::Vector<2, int>(x, y));
            }
        }
    }
    if (buildings.empty()) {
        buildings.push_back(
            plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
    }
    plasma::Vector<2, int> home = buildings[random(placeX + 16786, placeY) % buildings.size()];
    std::vector<plasma::Vector<2, int>> sites;
    for (int x = minX; x < maxX; x++) {
        for (int y = minY; y < maxY; y++) {
            if (plasma::Vector<2, int>(x, y) == home) {
                continue;
            }
            float influence =
                m_world->placeInfluence(type, large,
                                        m_world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                      y * BlocksPerCell + BlocksPerCell / 2));
            if (influence > PlaceCoreInfluence && isMissionSite(type, x, y)) {
                sites.push_back(plasma::Vector<2, int>(x, y));
            }
        }
    }
    if (sites.empty()) {
        sites.push_back(
            plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
    }
    int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
    int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
    int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    if (home[0] >= zoneMinX && home[0] < zoneMaxX && home[1] >= zoneMinY && home[1] < zoneMaxY) {
        StaticEntity prison;
        switch (kind) {
        case Captivity_Cage:
            prison.type = StaticEntityCageBase +
                          random(placeX + 60202, placeY) % (StaticEntityCage4Base - StaticEntityCageBase + 1);
            break;
        case Captivity_Cauldron:
            prison.type = StaticEntityCauldron;
            break;
        case Captivity_LegIrons:
            prison.type = StaticEntityLegIrons;
            break;
        case Captivity_Pole:
            prison.type = StaticEntityPole;
            break;
        case Captivity_TortureRack:
            prison.type = StaticEntityTortureRack;
            break;
        }
        if (apart) {
            prison.item = loot;
        }
        prison.position = zone->randomSpawnPosition(home[0], home[1]);
        prison.orientation = random(placeX + 206496, placeY) % 4;
        zone->putOnGround(prison, false);
        Zone::Resident *captive = new Zone::Resident();
        captive->hostility = HostilityFriendlyNpc;
        captive->level = level;
        captive->race = race;
        captive->classType = classType;
        captive->factionKind = captiveFactionKind;
        captive->behaviours = BehaviourWander | BehaviourUseFurniture;
        captive->appearance.appearanceFlags = AppearanceFlagCaptive | AppearanceFlagMissionResident;
        captive->destination = plasma::Vector<2, int>(placeX, placeY);
        fillAppearance(captive->race, captive->appearance, nullptr, false);
        equip(captive->hostility, captive->race, captive->level, captive->appearance.appearanceFlags,
              captive->factionKind, captive->classType, captive->specialization, captive->equipment,
              captive->specialSkills, captive->summonAppearanceMode, captive->summonRaces, captive->inventory);
        captive->position = prison.position;
        zone->adoptResident(captive);
        Item reward = taskRewardAt(placeX, placeY);
        if (reward.category != ItemCategoryNone) {
            captive->inventory.add(reward, 1);
        }
        prison.occupantId = captive->id;
        prison.scale = captive->appearance.size[0] / PrisonModelOccupantWidth;
        zone->addStaticEntity(prison);
        if (!apart) {
            int count = random(placeX + 381262, placeY) % 4 + 5;
            for (int i = 0; i < count; i++) {
                Zone::Resident *keeper = new Zone::Resident();
                keeper->hostility = HostilityEnemy;
                keeper->level = level;
                keeper->race = randomRace(keeperRace);
                keeper->factionKind = keeperFactionKind;
                keeper->behaviours = BehaviourWander | BehaviourUseFurniture;
                keeper->appearance.appearanceFlags = AppearanceFlagMissionResident;
                keeper->destination = plasma::Vector<2, int>(placeX, placeY);
                fillAppearance(keeper->race, keeper->appearance, nullptr, false);
                equip(keeper->hostility, keeper->race, keeper->level, keeper->appearance.appearanceFlags,
                      keeper->factionKind, keeper->classType, keeper->specialization, keeper->equipment,
                      keeper->specialSkills, keeper->summonAppearanceMode, keeper->summonRaces, keeper->inventory);
                keeper->position = zone->randomSpawnPosition(home[0], home[1]);
                keeper->activatorCells.insert(std::array<int, 2> {home[0], home[1]});
                zone->adoptResident(keeper);
            }
        }
    }
    if (apart) {
        bool together = !large;
        plasma::Vector<2, int> site = sites[random(placeX + 105918, placeY) % sites.size()];
        if (site[0] >= zoneMinX && site[0] < zoneMaxX && site[1] >= zoneMinY && site[1] < zoneMaxY) {
            Zone::Resident *carrier = new Zone::Resident();
            carrier->hostility = HostilityEnemy;
            carrier->level = level;
            carrier->race = randomRace(keeperRace);
            carrier->factionKind = keeperFactionKind;
            carrier->behaviours = BehaviourWander | BehaviourUseFurniture;
            carrier->appearance.appearanceFlags = AppearanceFlagTinted | AppearanceFlagMissionResident;
            fillAppearance(carrier->race, carrier->appearance, nullptr, false);
            equip(carrier->hostility, carrier->race, carrier->level, carrier->appearance.appearanceFlags,
                  carrier->factionKind, carrier->classType, carrier->specialization, carrier->equipment,
                  carrier->specialSkills, carrier->summonAppearanceMode, carrier->summonRaces, carrier->inventory);
            carrier->position = zone->randomSpawnPosition(site[0], site[1]);
            carrier->inventory.add(loot, 1);
            zone->adoptResident(carrier);
            if (together) {
                int count = random(placeX + 381262, placeY) % 3 + 4;
                for (int i = 0; i < count; i++) {
                    Zone::Resident *keeper = new Zone::Resident();
                    keeper->hostility = HostilityEnemy;
                    keeper->level = level;
                    keeper->race = randomRace(keeperRace);
                    keeper->factionKind = keeperFactionKind;
                    keeper->behaviours = BehaviourWander | BehaviourUseFurniture;
                    keeper->appearance.appearanceFlags = AppearanceFlagMissionResident;
                    fillAppearance(keeper->race, keeper->appearance, nullptr, false);
                    equip(keeper->hostility, keeper->race, keeper->level, keeper->appearance.appearanceFlags,
                          keeper->factionKind, keeper->classType, keeper->specialization, keeper->equipment,
                          keeper->specialSkills, keeper->summonAppearanceMode, keeper->summonRaces, keeper->inventory);
                    keeper->position = zone->randomSpawnPosition(site[0], site[1]);
                    zone->adoptResident(keeper);
                }
            }
        }
        if (!together) {
            srand(random(placeX + 39441, placeY));
            shuffleWithRand(sites.begin(), sites.end());
            // Up to 32 keepers, or an eighth of the sites where there are more than 256.
            sites.resize((std::max)((std::min)(32, (int)sites.size()), (int)sites.size() / 8));
            for (std::vector<plasma::Vector<2, int>>::iterator it = sites.begin(); it != sites.end(); ++it) {
                if ((*it)[0] >= zoneMinX && (*it)[0] < zoneMaxX && (*it)[1] >= zoneMinY && (*it)[1] < zoneMaxY) {
                    // These keepers are not adopted into the zone.
                    Zone::Resident *keeper = new Zone::Resident();
                    keeper->hostility = HostilityEnemy;
                    keeper->level = level;
                    keeper->race = randomRace(keeperRace);
                    keeper->factionKind = keeperFactionKind;
                    keeper->behaviours = BehaviourWander | BehaviourUseFurniture;
                    keeper->appearance.appearanceFlags = AppearanceFlagMissionResident;
                    fillAppearance(keeper->race, keeper->appearance, nullptr, false);
                    equip(keeper->hostility, keeper->race, keeper->level, keeper->appearance.appearanceFlags,
                          keeper->factionKind, keeper->classType, keeper->specialization, keeper->equipment,
                          keeper->specialSkills, keeper->summonAppearanceMode, keeper->summonRaces, keeper->inventory);
                    keeper->position = zone->randomSpawnPosition((*it)[0], (*it)[1]);
                }
            }
        }
    }
}

void Missions::generateCastle(Zone *zone)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    world->locationType(placeX, placeY, large);
}

// A place held by a cult (the topics 30 and 32): its target, a creature of race with the place's reward, recorded in
// the zone's m_missionCreatures, and three to six guards of guardRace that belong to it, all at one of the place's
// mission buildings.
void Missions::generateCult(Zone *zone, int race, int guardRace, int factionKind)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int type = world->locationType(placeX, placeY, large);
    int level = requestCount(placeX, placeY);
    srand(random(placeX + 39441, placeY));
    randomFolkRace(BiomeNone);
    World *generator = m_world;
    plasma::Vector<2, int> center = generator->placeCenter(placeX, placeY);
    std::vector<plasma::Vector<2, int>> buildings;
    int minX = floorDivide(placeX * BlocksPerPlace, BlocksPerCell);
    int maxX = large ? floorDivide(placeX * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeX * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int minY = floorDivide(placeY * BlocksPerPlace, BlocksPerCell);
    int maxY = large ? floorDivide(placeY * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeY * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int targetType = generator->missionTargetType(type, placeX, placeY, large);
    for (int x = minX; x < maxX; x++) {
        for (int y = minY; y < maxY; y++) {
            float influence =
                m_world->placeInfluence(type, large,
                                        m_world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                      y * BlocksPerCell + BlocksPerCell / 2));
            if (influence > PlaceCoreInfluence && isMissionBuilding(type, x, y, targetType)) {
                buildings.push_back(plasma::Vector<2, int>(x, y));
            }
        }
    }
    if (buildings.empty()) {
        buildings.push_back(
            plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
    }
    plasma::Vector<2, int> home = buildings[random(placeX + 16786, placeY) % buildings.size()];
    int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
    int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
    int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    if (home[0] >= zoneMinX && home[0] < zoneMaxX && home[1] >= zoneMinY && home[1] < zoneMaxY) {
        Zone::Resident *target = new Zone::Resident();
        target->hostility = HostilityEnemy;
        target->level = level;
        target->race = randomRace(race);
        target->factionKind = factionKind;
        target->behaviours = BehaviourWander;
        target->appearance.appearanceFlags = AppearanceFlagMissionResident;
        target->classType = ClassNone;
        target->destination = plasma::Vector<2, int>(placeX, placeY);
        target->position = zone->randomSpawnPosition(home[0], home[1]);
        Item reward = taskRewardAt(placeX, placeY);
        if (reward.category != ItemCategoryNone) {
            target->inventory.add(reward, 1);
            target->appearance.appearanceFlags |= AppearanceFlagTinted | AppearanceFlagEnhanced;
        }
        fillAppearance(target->race, target->appearance, nullptr, false);
        equip(target->hostility, target->race, target->level, target->appearance.appearanceFlags, target->factionKind,
              target->classType, target->specialization, target->equipment, target->specialSkills,
              target->summonAppearanceMode, target->summonRaces, target->inventory);
        zone->adoptResident(target);
        zone->m_missionCreatures.push_back(target->id);
        int count = rand() % 4 + 3;
        for (int i = 0; i < count; i++) {
            Zone::Resident *guard = new Zone::Resident();
            guard->hostility = HostilityEnemy;
            guard->level = level;
            guard->race = randomRace(guardRace);
            guard->factionKind = factionKind;
            guard->behaviours = BehaviourSupportOwner;
            guard->appearance.appearanceFlags = AppearanceFlagMissionResident;
            guard->classType = ClassNone;
            guard->destination = plasma::Vector<2, int>(placeX, placeY);
            guard->position = zone->randomSpawnPosition(home[0], home[1]);
            guard->ownerId = target->id;
            fillAppearance(guard->race, guard->appearance, nullptr, false);
            equip(guard->hostility, guard->race, guard->level, guard->appearance.appearanceFlags, guard->factionKind,
                  guard->classType, guard->specialization, guard->equipment, guard->specialSkills,
                  guard->summonAppearanceMode, guard->summonRaces, guard->inventory);
            zone->adoptResident(guard);
        }
    }
}

// A dark site (topic 31, and generate's for a kind 20 place): an object to destroy (hostility 7; a demon portal stands
// at choosePosition's place and gets AppearanceFlagLarge) with the place's reward, recorded in the zone's
// m_missionCreatures, and three to six guards of guardRace that belong to it, all at one of the place's mission
// buildings.
void Missions::generateDarkSite(Zone *zone, int objectRace, int guardRace, int guardFactionKind)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int type = world->locationType(placeX, placeY, large);
    int level = requestCount(placeX, placeY);
    srand(random(placeX + 39441, placeY));
    randomFolkRace(BiomeNone);
    World *generator = m_world;
    plasma::Vector<2, int> center = generator->placeCenter(placeX, placeY);
    std::vector<plasma::Vector<2, int>> buildings;
    int minX = floorDivide(placeX * BlocksPerPlace, BlocksPerCell);
    int maxX = large ? floorDivide(placeX * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeX * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int minY = floorDivide(placeY * BlocksPerPlace, BlocksPerCell);
    int maxY = large ? floorDivide(placeY * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeY * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int objectType = generator->missionTargetType(type, placeX, placeY, large);
    for (int x = minX; x < maxX; x++) {
        for (int y = minY; y < maxY; y++) {
            float influence =
                m_world->placeInfluence(type, large,
                                        m_world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                      y * BlocksPerCell + BlocksPerCell / 2));
            if (influence > PlaceCoreInfluence && isMissionBuilding(type, x, y, objectType)) {
                buildings.push_back(plasma::Vector<2, int>(x, y));
            }
        }
    }
    if (buildings.empty()) {
        buildings.push_back(
            plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
    }
    plasma::Vector<2, int> home = buildings[random(placeX + 16786, placeY) % buildings.size()];
    int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
    int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
    int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    if (home[0] >= zoneMinX && home[0] < zoneMaxX && home[1] >= zoneMinY && home[1] < zoneMaxY) {
        Zone::Resident *object = new Zone::Resident();
        object->hostility = HostilityAttackableObject;
        object->level = level;
        object->race = randomRace(objectRace);
        object->behaviours = BehaviourNone;
        object->appearance.appearanceFlags = AppearanceFlagMissionResident;
        if (objectRace == RaceDemonPortal) {
            object->appearance.appearanceFlags |= AppearanceFlagLarge;
        }
        object->classType = ClassNone;
        object->destination = plasma::Vector<2, int>(placeX, placeY);
        if (objectRace == RaceDemonPortal) {
            object->position = zone->choosePosition(home[0], home[1], PositionAny);
        } else {
            object->position = zone->randomSpawnPosition(home[0], home[1]);
        }
        Item reward = taskRewardAt(placeX, placeY);
        if (reward.category != ItemCategoryNone) {
            object->inventory.add(reward, 1);
        }
        fillAppearance(object->race, object->appearance, nullptr, false);
        equip(object->hostility, object->race, object->level, object->appearance.appearanceFlags, object->factionKind,
              object->classType, object->specialization, object->equipment, object->specialSkills,
              object->summonAppearanceMode, object->summonRaces, object->inventory);
        zone->adoptResident(object);
        zone->m_missionCreatures.push_back(object->id);
        int count = rand() % 4 + 3;
        for (int i = 0; i < count; i++) {
            Zone::Resident *guard = new Zone::Resident();
            guard->hostility = HostilityEnemy;
            guard->level = level;
            guard->race = randomRace(guardRace);
            guard->factionKind = guardFactionKind;
            guard->behaviours = BehaviourSupportOwner;
            guard->appearance.appearanceFlags = AppearanceFlagMissionResident;
            guard->classType = ClassNone;
            guard->destination = plasma::Vector<2, int>(placeX, placeY);
            guard->position = zone->randomSpawnPosition(home[0], home[1]);
            guard->ownerId = object->id;
            fillAppearance(guard->race, guard->appearance, nullptr, false);
            equip(guard->hostility, guard->race, guard->level, guard->appearance.appearanceFlags, guard->factionKind,
                  guard->classType, guard->specialization, guard->equipment, guard->specialSkills,
                  guard->summonAppearanceMode, guard->summonRaces, guard->inventory);
            zone->adoptResident(guard);
        }
    }
}

void Missions::generateDungeon(Zone *zone)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    world->locationType(placeX, placeY, large);
}

void Creature::greet(int &seed, std::vector<std::string> &text)
{
    // Signed seed increments can overflow. Includes an introduction and a nicety with probability one quarter each.
    text << "hello";
    if ((hash(seed++) & 3) == 0) {
        text << "define_mob" << static_cast<int>(m_id) << m_entityData.race;
        text << "my_name_is";
    }
    if ((hash(seed++) & 3) == 0) {
        text << "new_page";
        text << "nice_to_meet_you";
    }
}

// A place where things lie about to be fetched (topic 26, and the other end of an exchange): on up to twenty of the
// place's open plots, shuffled, as many of the place's fetchItem as the plot has positions of kind 16 (32 over all
// plots at most) and one creature of race 260 to guard them.
void Missions::generateErrand(Zone *zone)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int type = world->locationType(placeX, placeY, large);
    int level = requestCount(placeX, placeY);
    srand(random(placeX + 39441, placeY));
    randomFolkRace(BiomeNone);
    World *generator = m_world;
    plasma::Vector<2, int> center = generator->placeCenter(placeX, placeY);
    Item item = fetchItem(type, placeX, placeY, level, generator->findRegion(center));
    std::vector<plasma::Vector<2, int>> plots;
    int minX = floorDivide(placeX * BlocksPerPlace, BlocksPerCell);
    int maxX = large ? floorDivide(placeX * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeX * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int minY = floorDivide(placeY * BlocksPerPlace, BlocksPerCell);
    int maxY = large ? floorDivide(placeY * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeY * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    for (int x = minX; x < maxX; x++) {
        for (int y = minY; y < maxY; y++) {
            World *world = m_world;
            float influence =
                world->placeInfluence(type, large,
                                      world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                  y * BlocksPerCell + BlocksPerCell / 2));
            if (!(influence > PlaceCoreInfluence)) {
                continue;
            }
            plasma::Vector<2, int> point = world->streetPoint(x, y);
            World::AreaInfo info;
            world->fillAreaInfo(point[0], point[1], info);
            if (isOpenBuilding(world->streetBuildingType(x, y, point, info)) && isSuitableGround(x, y)) {
                plots.push_back(plasma::Vector<2, int>(x, y));
            }
        }
    }
    if (plots.empty()) {
        plots.push_back(
            plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
    }
    shuffleWithRand(plots.begin(), plots.end());
    if (plots.size() > 20) {
        plots.resize(20);
    }
    int perPlot = (std::max)(1, 32 / (int)plots.size());
    std::vector<int> races = {RaceEnhancedOrcMale};
    for (std::vector<plasma::Vector<2, int>>::iterator plot = plots.begin(); plot != plots.end(); ++plot) {
        int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
        int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
        int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
        int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
        if ((*plot)[0] >= zoneMinX && (*plot)[0] < zoneMaxX && (*plot)[1] >= zoneMinY && (*plot)[1] < zoneMaxY) {
            int count = (std::min)(zone->positionCount((*plot)[0], (*plot)[1], PositionNaturalGround), perPlot);
            for (int i = 0; i < count; i++) {
                ZoneItem lying;
                lying.item = item;
                lying.flags = ZoneItemFlagPerPlayer;
                lying.position = zone->randomPosition((*plot)[0], (*plot)[1], PositionNaturalGround);
                lying.rotation = (float)rand() * 360.0f / (float)RAND_MAX;
                zone->m_items.push_back(lying);
            }
            Zone::Resident *guard = new Zone::Resident();
            guard->hostility = HostilityEnemy;
            guard->level = level;
            guard->race = randomRace(races[rand() % races.size()]);
            guard->factionKind = FactionBloodaxeClan;
            guard->behaviours = BehaviourWander | BehaviourUseFurniture;
            guard->appearance.appearanceFlags = AppearanceFlagMissionResident;
            guard->classType = ClassNone;
            guard->destination = plasma::Vector<2, int>(placeX, placeY);
            fillAppearance(guard->race, guard->appearance, nullptr, false);
            equip(guard->hostility, guard->race, guard->level, guard->appearance.appearanceFlags, guard->factionKind,
                  guard->classType, guard->specialization, guard->equipment, guard->specialSkills,
                  guard->summonAppearanceMode, guard->summonRaces, guard->inventory);
            if ((guard->race == RaceEmpireSoldierMale || guard->race == RaceEmpireSoldierFemale) && rand() % 2 != 0) {
                guard->position = zone->randomPosition((*plot)[0], (*plot)[1], PositionAny);
                guard->currentAbility = AbilitySlowFall;
            } else {
                guard->position = zone->randomSpawnPosition((*plot)[0], (*plot)[1]);
            }
            zone->adoptResident(guard);
        }
    }
}

// A place that takes part in an exchange (topic 24): a cauldron (static entity 88, recorded in the zone's
// m_prerequisiteEntityIndices) at one of the place's mission buildings, holding the fetchItem of the place's partner
// (its own without one), five to eight green slimes that stay at the building, and a blue slime that knows them, is
// recorded in the zone's m_missionCreatures and carries the place's reward. A place without a partner is an errand's
// place as well.
void Missions::generateExchange(Zone *zone)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int type = world->locationType(placeX, placeY, large);
    int level = requestCount(placeX, placeY);
    srand(random(placeX + 39441, placeY));
    randomFolkRace(BiomeNone);
    World *generator = m_world;
    plasma::Vector<2, int> center = generator->placeCenter(placeX, placeY);
    std::vector<plasma::Vector<2, int>> buildings;
    int minX = floorDivide(placeX * BlocksPerPlace, BlocksPerCell);
    int maxX = large ? floorDivide(placeX * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeX * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int minY = floorDivide(placeY * BlocksPerPlace, BlocksPerCell);
    int maxY = large ? floorDivide(placeY * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeY * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int targetType = generator->missionTargetType(type, placeX, placeY, large);
    for (int x = minX; x < maxX; x++) {
        for (int y = minY; y < maxY; y++) {
            float influence =
                m_world->placeInfluence(type, large,
                                        m_world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                      y * BlocksPerCell + BlocksPerCell / 2));
            if (influence > PlaceCoreInfluence && isMissionBuilding(type, x, y, targetType)) {
                buildings.push_back(plasma::Vector<2, int>(x, y));
            }
        }
    }
    if (buildings.empty()) {
        buildings.push_back(
            plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
    }
    plasma::Vector<2, int> home = buildings[random(placeX + 16786, placeY) % buildings.size()];
    int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
    int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
    int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    if (home[0] >= zoneMinX && home[0] < zoneMaxX && home[1] >= zoneMinY && home[1] < zoneMaxY) {
        StaticEntity cauldron;
        cauldron.type = StaticEntityCauldron;
        int partnerX = placeX;
        int partnerY = placeY;
        std::array<int, 2> place = {placeX, placeY};
        std::map<std::array<int, 2>, std::array<int, 2>>::iterator partner = m_partners.find(place);
        if (partner != m_partners.end()) {
            partnerX = partner->second[0];
            partnerY = partner->second[1];
        }
        plasma::Vector<2, int> region = m_world->findRegion(center);
        cauldron.item = fetchItem(type, partnerX, partnerY, requestCount(partnerX, partnerY), region);
        cauldron.itemCount = ExchangeItemCount;
        cauldron.position = zone->randomSpawnPosition(home[0], home[1]);
        cauldron.orientation = random(placeX + 206496, placeY) % 4;
        zone->putOnGround(cauldron, false);
        zone->m_prerequisiteEntityIndices.push_back((int)zone->m_staticEntities.size());
        zone->addStaticEntity(cauldron);
        std::list<long long> slimes;
        int count = rand() % 4 + 5;
        for (int i = 0; i < count; i++) {
            Zone::Resident *slime = new Zone::Resident();
            slime->hostility = HostilityEnemy;
            slime->level = level;
            slime->race = randomRace(RaceGreenSlime);
            slime->factionKind = FactionNone;
            slime->behaviours = BehaviourWander | BehaviourUseFurniture;
            slime->appearance.appearanceFlags = AppearanceFlagMissionResident;
            slime->classType = ClassNone;
            slime->destination = plasma::Vector<2, int>(placeX, placeY);
            slime->position = cauldron.position;
            slime->prerequisiteEntityCells.insert(std::array<int, 2> {home[0], home[1]});
            fillAppearance(slime->race, slime->appearance, nullptr, false);
            equip(slime->hostility, slime->race, slime->level, slime->appearance.appearanceFlags, slime->factionKind,
                  slime->classType, slime->specialization, slime->equipment, slime->specialSkills,
                  slime->summonAppearanceMode, slime->summonRaces, slime->inventory);
            zone->adoptResident(slime);
            slimes.push_back(slime->id);
        }
        Zone::Resident *keeper = new Zone::Resident();
        keeper->hostility = HostilityEnemy;
        keeper->level = level;
        keeper->race = randomRace(RaceBlueSlime);
        keeper->factionKind = FactionNone;
        keeper->behaviours = BehaviourWander | BehaviourUseFurniture;
        keeper->appearance.appearanceFlags = AppearanceFlagTinted | AppearanceFlagMissionResident;
        keeper->classType = ClassNone;
        keeper->destination = plasma::Vector<2, int>(placeX, placeY);
        keeper->position = cauldron.position;
        keeper->prerequisiteCreatureIds = slimes;
        fillAppearance(keeper->race, keeper->appearance, nullptr, false);
        equip(keeper->hostility, keeper->race, keeper->level, keeper->appearance.appearanceFlags, keeper->factionKind,
              keeper->classType, keeper->specialization, keeper->equipment, keeper->specialSkills,
              keeper->summonAppearanceMode, keeper->summonRaces, keeper->inventory);
        zone->adoptResident(keeper);
        zone->m_missionCreatures.push_back(keeper->id);
        Item reward = taskRewardAt(placeX, placeY);
        if (reward.category != ItemCategoryNone) {
            keeper->inventory.add(reward, 1);
            keeper->appearance.appearanceFlags |= AppearanceFlagTinted | AppearanceFlagEnhanced;
        }
    }
    std::array<int, 2> place = {placeX, placeY};
    if (m_partners.find(place) == m_partners.end()) {
        generateErrand(zone);
    }
}

// A place overrun by creatures (the topics 17..23, "the_location_is_full_of_creatures" and "you_must_defeat_them"):
// their chief, a creature of race with the form and flags given, at one of the place's mission buildings, carrying the
// place's reward (without one it is recorded in the zone's m_missionCreatures), and on the place's open plots
// (shuffled; an eighth of them, eight at least) a creature of one of the races with creatureFlags and the class given,
// recorded in the zone's m_trackedMissionCreatureIds. One plot in ten holds a captive as well: a peaceful one (for the
// forms 4 and 5), with a horse one time in four, or a fighter, with a companion of race 263 one time in eight.
void Missions::generateInvasion(Zone *zone, int race, unsigned int flags, const std::vector<int> &races,
                                int factionKind, int classType, unsigned int creatureFlags)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int type = world->locationType(placeX, placeY, large);
    int level = requestCount(placeX, placeY);
    srand(random(placeX + 39441, placeY));
    plasma::Vector<2, int> center = m_world->placeCenter(placeX, placeY);
    std::vector<plasma::Vector<2, int>> buildings;
    int minX = floorDivide(placeX * BlocksPerPlace, BlocksPerCell);
    int maxX = large ? floorDivide(placeX * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeX * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int minY = floorDivide(placeY * BlocksPerPlace, BlocksPerCell);
    int maxY = large ? floorDivide(placeY * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeY * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int targetType = m_world->missionTargetType(type, placeX, placeY, large);
    for (int x = minX; x < maxX; x++) {
        for (int y = minY; y < maxY; y++) {
            float influence =
                m_world->placeInfluence(type, large,
                                        m_world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                      y * BlocksPerCell + BlocksPerCell / 2));
            if (influence > PlaceCoreInfluence && isMissionBuilding(type, x, y, targetType)) {
                buildings.push_back(plasma::Vector<2, int>(x, y));
            }
        }
    }
    if (buildings.empty()) {
        buildings.push_back(
            plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
    }
    int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
    int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
    int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    plasma::Vector<2, int> home = buildings[rand() % buildings.size()];
    std::vector<plasma::Vector<2, int>> plots;
    for (int x = minX; x < maxX; x++) {
        for (int y = minY; y < maxY; y++) {
            World *world = m_world;
            float influence =
                world->placeInfluence(type, large,
                                      world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                  y * BlocksPerCell + BlocksPerCell / 2));
            if (!(influence > PlaceCoreInfluence)) {
                continue;
            }
            plasma::Vector<2, int> point = world->streetPoint(x, y);
            World::AreaInfo info;
            world->fillAreaInfo(point[0], point[1], info);
            if (isOpenBuilding(world->streetBuildingType(x, y, point, info)) && isSuitableGround(x, y)) {
                plots.push_back(plasma::Vector<2, int>(x, y));
            }
        }
    }
    if (plots.empty()) {
        plots.push_back(
            plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
    }
    shuffleWithRand(plots.begin(), plots.end());
    plots.resize((std::max)(1, (std::max)(8, (int)plots.size() / 8)));
    zone->m_trackedMissionCreatureIds.clear();
    if (home[0] >= zoneMinX && home[0] < zoneMaxX && home[1] >= zoneMinY && home[1] < zoneMaxY) {
        Zone::Resident *chief = new Zone::Resident();
        chief->hostility = HostilityEnemy;
        chief->level = level;
        chief->race = race;
        chief->factionKind = factionKind;
        chief->appearance.appearanceFlags = flags | (AppearanceFlagCaptive | AppearanceFlagMissionResident);
        chief->behaviours = BehaviourWander | BehaviourUseFurniture;
        chief->position = zone->randomSpawnPosition(home[0], home[1]);
        chief->requiredDefeatedCount = (std::max)(1, (std::min)((int)plots.size() / 2, 20));
        Item reward = taskRewardAt(placeX, placeY);
        if (reward.category != ItemCategoryNone) {
            chief->inventory.add(reward, 1);
            chief->appearance.appearanceFlags |= AppearanceFlagEnhanced;
        }
        fillAppearance(chief->race, chief->appearance, nullptr, false);
        equip(chief->hostility, chief->race, chief->level, chief->appearance.appearanceFlags, chief->factionKind,
              chief->classType, chief->specialization, chief->equipment, chief->specialSkills,
              chief->summonAppearanceMode, chief->summonRaces, chief->inventory);
        zone->adoptResident(chief);
        if (reward.category == ItemCategoryNone) {
            zone->m_missionCreatures.push_back(chief->id);
        }
    }
    for (std::vector<plasma::Vector<2, int>>::iterator plot = plots.begin(); plot != plots.end(); ++plot) {
        if ((*plot)[0] >= zoneMinX && (*plot)[0] < zoneMaxX && (*plot)[1] >= zoneMinY && (*plot)[1] < zoneMaxY) {
            srand(random((*plot)[0] + 5189520, (*plot)[1]));
            Zone::Resident *creature = new Zone::Resident();
            creature->hostility = HostilityEnemy;
            creature->level = level;
            creature->race = randomRace(races[rand() % races.size()]);
            creature->factionKind = factionKind;
            creature->behaviours = BehaviourWander | BehaviourUseFurniture;
            creature->appearance.appearanceFlags = creatureFlags | AppearanceFlagMissionResident;
            creature->classType = classType;
            creature->destination = plasma::Vector<2, int>(placeX, placeY);
            fillAppearance(creature->race, creature->appearance, nullptr, false);
            equip(creature->hostility, creature->race, creature->level, creature->appearance.appearanceFlags,
                  creature->factionKind, creature->classType, creature->specialization, creature->equipment,
                  creature->specialSkills, creature->summonAppearanceMode, creature->summonRaces, creature->inventory);
            if ((creature->race == RaceEmpireSoldierMale || creature->race == RaceEmpireSoldierFemale) &&
                rand() % 2 != 0) {
                creature->position = zone->randomPosition((*plot)[0], (*plot)[1], PositionAny);
                creature->currentAbility = AbilitySlowFall;
            } else {
                creature->position = zone->randomSpawnPosition((*plot)[0], (*plot)[1]);
            }
            zone->adoptResident(creature);
            zone->m_trackedMissionCreatureIds.push_back(creature->id);
            if (rand() % 10 == 0) {
                Zone::Resident *captive = new Zone::Resident();
                captive->hostility = HostilityFriendlyNpc;
                captive->level = level;
                if (factionKind >= FactionCultOfDoom && factionKind <= FactionUnholyPact) {
                    captive->race = randomPeacefulRace();
                    captive->classType = ClassOrderOfTheLightMember;
                    captive->factionKind = FactionOrderOfTheLight;
                } else {
                    captive->race = randomFighterRace();
                    captive->classType = ClassDruidOfMana;
                    captive->factionKind = FactionDruidsOfMana;
                }
                captive->behaviours = BehaviourWander | BehaviourUseFurniture | Behaviour_8192;
                captive->appearance.appearanceFlags = AppearanceFlagMissionResident;
                fillAppearance(captive->race, captive->appearance, nullptr, false);
                equip(captive->hostility, captive->race, captive->level, captive->appearance.appearanceFlags,
                      captive->factionKind, captive->classType, captive->specialization, captive->equipment,
                      captive->specialSkills, captive->summonAppearanceMode, captive->summonRaces, captive->inventory);
                captive->position = zone->randomSpawnPosition((*plot)[0], (*plot)[1]);
                zone->adoptResident(captive);
                if (captive->factionKind == FactionOrderOfTheLight && rand() % 4 == 0) {
                    Zone::Resident *horse = new Zone::Resident();
                    horse->hostility = HostilityNeutral;
                    horse->level = level;
                    horse->race = RaceGoldKnightHorse;
                    horse->behaviours = BehaviourWander | BehaviourFollowOwner;
                    horse->ownerId = captive->id;
                    horse->appearance.appearanceFlags = AppearanceFlagMissionResident;
                    fillAppearance(horse->race, horse->appearance, nullptr, false);
                    equip(horse->hostility, horse->race, horse->level, horse->appearance.appearanceFlags,
                          horse->factionKind, horse->classType, horse->specialization, horse->equipment,
                          horse->specialSkills, horse->summonAppearanceMode, horse->summonRaces, horse->inventory);
                    horse->position = zone->randomSpawnPosition((*plot)[0], (*plot)[1]);
                    zone->adoptResident(horse);
                }
                if (captive->factionKind == FactionDruidsOfMana && rand() % 8 == 0) {
                    Zone::Resident *companion = new Zone::Resident();
                    companion->hostility = HostilityNeutral;
                    companion->level = level;
                    companion->race = RaceTreant;
                    companion->behaviours = BehaviourWander;
                    companion->appearance.appearanceFlags = AppearanceFlagMissionResident;
                    fillAppearance(companion->race, companion->appearance, nullptr, false);
                    equip(companion->hostility, companion->race, companion->level,
                          companion->appearance.appearanceFlags, companion->factionKind, companion->classType,
                          companion->specialization, companion->equipment, companion->specialSkills,
                          companion->summonAppearanceMode, companion->summonRaces, companion->inventory);
                    companion->position = zone->randomSpawnPosition((*plot)[0], (*plot)[1]);
                    zone->adoptResident(companion);
                }
            }
        }
    }
}

// A place with a monster to defeat (topic 1): monsterRace's creature of the place, of the place's level, at one of the
// place's mission buildings (chosen by the cell; a place of type 24 has none, and its monster stands at the centre).
// It carries the place's reward; without one it is recorded in the zone's m_missionCreatures.
void Missions::generateMonster(Zone *zone)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int type = world->locationType(placeX, placeY, large);
    int level = requestCount(placeX, placeY);
    srand(random(placeX + 39441, placeY));
    int race = monsterRace(placeX, placeY, type);
    Item reward = taskRewardAt(placeX, placeY);
    World *generator = m_world;
    plasma::Vector<2, int> center = generator->placeCenter(placeX, placeY);
    std::vector<plasma::Vector<2, int>> buildings;
    int minX = floorDivide(placeX * BlocksPerPlace, BlocksPerCell);
    int maxX = large ? floorDivide(placeX * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeX * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int minY = floorDivide(placeY * BlocksPerPlace, BlocksPerCell);
    int maxY = large ? floorDivide(placeY * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeY * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int targetType = generator->missionTargetType(type, placeX, placeY, large);
    if (type != PlaceArena) {
        for (int x = minX; x < maxX; x++) {
            for (int y = minY; y < maxY; y++) {
                float influence =
                    m_world->placeInfluence(type, large,
                                            m_world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                          y * BlocksPerCell + BlocksPerCell / 2));
                if (influence > PlaceCoreInfluence && isMissionBuilding(type, x, y, targetType)) {
                    buildings.push_back(plasma::Vector<2, int>(x, y));
                }
            }
        }
    }
    if (buildings.empty()) {
        buildings.push_back(
            plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
    }
    plasma::Vector<2, int> home = buildings[random(placeX + 16786, placeY) % buildings.size()];
    int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
    int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
    int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    if (home[0] >= zoneMinX && home[0] < zoneMaxX && home[1] >= zoneMinY && home[1] < zoneMaxY) {
        Zone::Resident *monster = new Zone::Resident();
        monster->hostility = HostilityEnemy;
        monster->level = level;
        monster->race = randomRace(race);
        monster->behaviours = BehaviourWander | BehaviourUseFurniture;
        monster->appearance.appearanceFlags = AppearanceFlagMissionResident;
        monster->destination = plasma::Vector<2, int>(placeX, placeY);
        monster->appearance.appearanceFlags |= AppearanceFlagLarge;
        if (reward.category != ItemCategoryNone) {
            monster->inventory.add(reward, 1);
        }
        fillAppearance(monster->race, monster->appearance, nullptr, false);
        equip(monster->hostility, monster->race, monster->level, monster->appearance.appearanceFlags,
              monster->factionKind, monster->classType, monster->specialization, monster->equipment,
              monster->specialSkills, monster->summonAppearanceMode, monster->summonRaces, monster->inventory);
        monster->position = zone->randomSpawnPosition(home[0], home[1]);
        zone->adoptResident(monster);
        if (reward.category == ItemCategoryNone) {
            zone->m_missionCreatures.push_back(monster->id);
        }
    }
}

// What a place's mission puts into a zone, by the topic of the place.
void Missions::generatePlace(Zone *zone)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    if (m_world->isLargePlace(placeX, placeY)) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    srand(random(placeX + 559258222, placeY));
    switch (determineTopic(placeX, placeY)) {
    case MissionDungeon:
        generateDungeon(zone);
        break;
    case MissionCastle:
        generateCastle(zone);
        break;
    case MissionBand:
        generateBand(zone);
        break;
    case MissionMonster:
        generateMonster(zone);
        break;
    case MissionAborigines:
        generateAborigines(zone);
        break;
    case MissionStatues:
        generateStatues(zone);
        break;
    case MissionDemonPortals:
        generateDemonPortals(zone);
        break;
    case MissionUnholyTombs:
        generateUnholyTombs(zone);
        break;
    case MissionBonePiles:
        generateBonePiles(zone);
        break;
    case MissionSlimeWells:
        generateSlimeWells(zone);
        break;
    case MissionMolehills:
        generateMolehills(zone);
        break;
    case MissionHornetsNests:
        generateHornetsNests(zone);
        break;
    case MissionFolkCamp: {
        int race = kindAt(placeX, placeY) == MissionKindSupplierGnome ? RaceSupplierGnome : randomMobRace();
        generateCamp(zone, race, ClassNone, FactionNone, randomFolkRace(BiomeNone), FactionNone);
        break;
    }
    case MissionOgreCamp: {
        int race = kindAt(placeX, placeY) == MissionKindSupplierGnome ? RaceSupplierGnome : randomMobRace();
        generateCamp(zone, race, ClassNone, FactionNone, RaceOgre, FactionNone);
        break;
    }
    case MissionWitchCamp: {
        int race = kindAt(placeX, placeY) == MissionKindSupplierGnome ? RaceSupplierGnome : randomMobRace();
        generateCamp(zone, race, ClassNone, FactionNone, RaceWitch, FactionNone);
        break;
    }
    case MissionSkeletonCamp: {
        int race = kindAt(placeX, placeY) == MissionKindSupplierGnome ? RaceSupplierGnome : randomMobRace();
        generateCamp(zone, race, ClassNone, FactionNone, RaceSkeleton, FactionUnholyPact);
        break;
    }
    case MissionMonstrosityCamp: {
        int race = kindAt(placeX, placeY) == MissionKindSupplierGnome ? RaceSupplierGnome : randomMobRace();
        generateCamp(zone, race, ClassNone, FactionNone, RaceMonstrosity, FactionUnholyPact);
        break;
    }
    case MissionNecromancerCamp: {
        int race = kindAt(placeX, placeY) == MissionKindSupplierGnome ? RaceSupplierGnome : randomMobRace();
        generateCamp(zone, race, ClassNone, FactionNone, RaceNecromancer, FactionUnholyPact);
        break;
    }
    case MissionDarkCultistCamp: {
        int race = kindAt(placeX, placeY) == MissionKindSupplierGnome ? RaceSupplierGnome : randomMobRace();
        generateCamp(zone, race, ClassNone, FactionNone, RaceDarkCultistMale, FactionCultOfDoom);
        break;
    }
    case MissionDemonMarauderCamp: {
        int race = kindAt(placeX, placeY) == MissionKindSupplierGnome ? RaceSupplierGnome : randomMobRace();
        generateCamp(zone, race, ClassNone, FactionNone, RaceDemonMarauder, FactionCultOfDoom);
        break;
    }
    case MissionSeaCrabInvasion: {
        std::vector<int> races = {RaceSeaCrab};
        generateInvasion(zone, RaceSeaCrab, AppearanceFlagLarge, races, 0, ClassNone, 0);
        break;
    }
    case MissionSlimeInvasion: {
        // A slime of the colour that goes with the place's number.
        int race = (std::max)(0, requestCount(placeX, placeY) - 1) + RaceGreenSlime;
        std::vector<int> races = {race};
        generateInvasion(zone, race, AppearanceFlagLarge, races, 0, ClassNone, 0);
        break;
    }
    case MissionUndeadInvasion: {
        std::vector<int> races = {randomUndead(), randomUndead()};
        generateInvasion(zone, randomUndeadBoss(), AppearanceFlagTinted, races, 5, ClassNone, 0);
        break;
    }
    case MissionDemonInvasion: {
        std::vector<int> races = {randomDemonCreature(), randomDemonCreature()};
        generateInvasion(zone, randomDemonBoss(), AppearanceFlagTinted, races, 4, ClassNone, 0);
        break;
    }
    case MissionEmpireSoldierInvasion: {
        std::vector<int> races = {RaceEmpireSoldierMale};
        generateInvasion(zone, randomRace(RaceEmpireSoldierMale), AppearanceFlagTinted, races, 6, ClassNone, 0);
        break;
    }
    case MissionInsectInvasion: {
        std::vector<int> races = {randomInsect(), randomInsect()};
        generateInvasion(zone, randomInsect(), AppearanceFlagLarge, races, 0, ClassNone, 0);
        break;
    }
    case MissionPlantCreatureInvasion: {
        std::vector<int> races = {randomPlantCreature(), randomPlantCreature()};
        generateInvasion(zone, randomPlantCreature(), AppearanceFlagLarge, races, 0, ClassNone, 0);
        break;
    }
    case MissionExchange:
        generateExchange(zone);
        break;
    case MissionErrand:
        generateErrand(zone);
        break;
    case MissionCult:
        generateCult(zone, randomUndead(), RaceNecromancer, FactionUnholyPact);
        break;
    case MissionDarkSite:
        generateDarkSite(zone, random(placeX + 1932669, placeY) % 2 + RaceUnholyTomb, RaceNecromancer,
                         FactionUnholyPact);
        break;
    case MissionDemonCult:
        generateCult(zone, randomDemonCreature(), RaceDarkCultistMale, FactionCultOfDoom);
        break;
    }
}

bool Missions::hasCraftingRank(int rank)
{
    Creature *player = m_world->m_localCreature;
    if (!player) {
        return false;
    }
    if (rank == 0) {
        return true;
    }
    for (const std::pair<const std::array<int, 2>, unsigned int> &place : m_kinds) {
        if (place.second == rank + 1) {
            if (player->m_playerData->solvedMissions.find(place.first) != player->m_playerData->solvedMissions.end()) {
                return true;
            }
        }
    }
    return false;
}

void help(int &seed, std::vector<std::string> &text)
{
    // Signed seed increments can overflow.
    if (hash(seed++) & 1) {
        text << "!alert";
    }
    if (hash(seed++) & 1) {
        text << "hey_you";
        text << "new_page";
    }
    text << "help";
    if (hash(seed++) & 1) {
        text << "!sweat";
    } else {
        text << "!angry";
    }
}

bool Missions::isLavaRiver(int cellX, int cellY)
{
    int x = cellX * BlocksPerCell + BlocksPerCell / 2;
    int y = cellY * BlocksPerCell + BlocksPerCell / 2;
    World::AreaInfo info;
    m_world->fillAreaInfo(x, y, info);
    if (info.environment != 0 && info.influence < EnvironmentCoreInfluence) {
        return false;
    }
    World::BiomeSample sample;
    m_world->biomeSample(x, y, sample);
    float road = m_world->streetAmount(x, y, info.type, info.influence);
    float firelands = sample.lavaWeight(info.type, info.influence);
    float mountains = sample.mountains();
    return m_world->lavaChannels(x, y, road, firelands, mountains, &info) > ChannelThreshold;
}

bool Missions::isMissionBuilding(int locationType, int cellX, int cellY, int unused)
{
    int building = m_world->zoneBuildingType(cellX, cellY);
    switch (locationType) {
    case PlaceRock:
        return building == BuildingTallRock;
    case PlaceRocks:
        return building >= BuildingHill && building <= BuildingTallRock;
    case PlaceHouse:
    case PlaceFarm:
    case PlaceInn:
        return building >= BuildingHouseRoom && building <= BuildingHouseEntrance;
    case PlaceOldHut:
        return building >= BuildingOldHutRoom && building <= BuildingOldHutEntrance;
    case PlaceTower:
        return building == BuildingTower;
    case PlaceCrypt:
        return building >= BuildingCryptRoom && building <= BuildingCryptUpperRoom && building != BuildingLoreCrypt;
    case PlaceCamp:
    case PlaceBanditCamp:
        return building == BuildingCampEquipmentSite;
    case PlaceAltar:
        return building == BuildingAltar;
    case PlaceSanctuary:
        return building >= BuildingSanctuary && building <= BuildingRaisedSanctuary;
    default:
        return isOpenBuilding(building) && isSuitableGround(cellX, cellY);
    }
}

bool Missions::isMissionSite(int locationType, int cellX, int cellY)
{
    int building = m_world->zoneBuildingType(cellX, cellY);
    switch (locationType) {
    case PlaceCamp:
    case PlaceBanditCamp:
        return building == BuildingCampfireSite;
    default:
        return isOpenBuilding(building) && isSuitableGround(cellX, cellY);
    case PlaceRocks:
        return building >= BuildingHill && building <= BuildingTallRock;
    case PlaceRock:
        return building == BuildingHill || building == BuildingRock || building == BuildingTallRock;
    }
}

bool Missions::isPlaceBuilding(int locationType, int cellX, int cellY, int buildingType)
{
    int building = m_world->zoneBuildingType(cellX, cellY);
    if (buildingType != BuildingNone) {
        return building == buildingType;
    }
    switch (locationType) {
    default:
        return isMissionBuilding(locationType, cellX, cellY, 0);
    case PlaceLavaLake:
    case PlaceVolcano:
        return isOpenBuilding(building) && isLavaRiver(cellX, cellY);
    case PlaceLake:
        return isOpenBuilding(building) && isWaterway(cellX, cellY);
    }
}

bool isRemoteType(int type)
{
    return type == PlaceCrater || type == PlaceIsland || type == PlaceCanyon || type == PlacePlain ||
           type == PlaceHill || type == PlaceBeach;
}

bool Missions::isSuitableGround(int cellX, int cellY)
{
    int x = cellX * BlocksPerCell + BlocksPerCell / 2;
    int y = cellY * BlocksPerCell + BlocksPerCell / 2;
    World::AreaInfo info;
    m_world->fillAreaInfo(x, y, info);
    if (info.environment != 0 && info.influence < EnvironmentCoreInfluence) {
        return false;
    }
    World::BiomeSample sample;
    m_world->biomeSample(x, y, sample);
    float road = m_world->streetAmount(x, y, info.type, info.influence);
    float firelands = sample.lavaWeight(info.type, info.influence);
    float undead = sample.undead(info.type, info.influence);
    float wetlands = sample.wetness();
    float mountains = sample.mountains();
    if (m_world->valleyWater(x, y, road, firelands, undead, wetlands, mountains, &info) > ChannelThreshold) {
        return false;
    }
    if (m_world->lavaChannels(x, y, road, firelands, mountains, &info) > ChannelThreshold) {
        return false;
    }
    if (m_world->poisonChannels(x, y, road, undead, mountains, &info) > ChannelThreshold) {
        return false;
    }
    if (m_world->height(x, y, 0) < DryGroundHeight) {
        return false;
    }
    return true;
}

bool Missions::isWaterway(int cellX, int cellY)
{
    int x = cellX * BlocksPerCell + BlocksPerCell / 2;
    int y = cellY * BlocksPerCell + BlocksPerCell / 2;
    World::AreaInfo info;
    m_world->fillAreaInfo(x, y, info);
    if (info.environment != 0 && info.influence < EnvironmentCoreInfluence) {
        return false;
    }
    World::BiomeSample sample;
    m_world->biomeSample(x, y, sample);
    float road = m_world->streetAmount(x, y, info.type, info.influence);
    float firelands = sample.lavaWeight(info.type, info.influence);
    float undead = sample.undead(info.type, info.influence);
    float wetlands = sample.wetness();
    float mountains = sample.mountains();
    return m_world->valleyWater(x, y, road, firelands, undead, wetlands, mountains, &info) > ChannelThreshold;
}

int Missions::solvedCount()
{
    Creature *player = m_world->m_localCreature;
    if (!player) {
        return 0;
    }
    int count = 0;
    for (const std::pair<const std::array<int, 2>, unsigned int> &place : m_kinds) {
        if (place.second == MissionKindSupplierGnome) {
            if (player->m_playerData->solvedMissions.find(place.first) != player->m_playerData->solvedMissions.end()) {
                count++;
            }
        }
    }
    return (std::min)(4, count);
}

int monsterRace(int x, int y, int locationType)
{
    std::vector<int> races;
    if (locationType == PlaceIsland || locationType == PlaceBeach) {
        races.push_back(RaceSeaCrab);
    } else {
        races.push_back(RaceTroll);
        races.push_back(RaceDarkTroll);
        races.push_back(RaceHellDemon);
        races.push_back(RaceGolem);
        races.push_back(RaceEmberGolem);
        races.push_back(RaceSnowGolem);
        races.push_back(RaceYeti);
        races.push_back(RaceCyclops);
        races.push_back(RaceCyclops);
        races.push_back(RaceMammoth);
        races.push_back(RaceLich);
        races.push_back(RaceRuneGiant);
        races.push_back(RaceSaurian);
    }
    return races[random(x + 45328134, y) % races.size()];
}

// Draw one of the region's artifact places in the requested realm.
plasma::Vector<2, int> Missions::artifactPlace(int seed, int realm)
{
    std::vector<plasma::Vector<2, int>> places;
    for (const plasma::Vector<2, int> &place : m_artifactPlaces) {
        World *world = m_world;
        if (world->mortalRealm(place[0], place[1], world->isLargePlace(place[0], place[1])) == realm)
            places.push_back(place);
    }
    if (places.empty())
        return plasma::Vector<2, int>(NoCoordinate, NoCoordinate);
    return places[hash(seed) % places.size()];
}

plasma::Vector<2, int> Missions::randomPlace(int seed)
{
    if (m_places.empty()) {
        return plasma::Vector<2, int>(NoCoordinate, NoCoordinate);
    }
    return m_places[hash(seed) % m_places.size()];
}

int Missions::requestCount(int x, int y)
{
    std::array<int, 2> key = {x, y};
    std::map<std::array<int, 2>, unsigned int>::iterator it = m_values.find(key);
    if (it == m_values.end()) {
        return 0;
    }
    return it->second;
}

bool Zone::Resident::setNamesake(int x, int y, int type)
{
    int variant = metroNameVariant(x, y, type);
    if (variant >= 0) {
        race = variant;
        if (type >= PlaceHouse && type <= PlaceInn) {
            classType = ClassVillager;
        }
        std::wstring wideName = randomName(metroNameSeed(x, y), metroNameVariant(x, y, type));
        std::string narrowName(wideName.begin(), wideName.end());
        strcpy(name, narrowName.c_str());
        return true;
    }
    return false;
}

int Missions::specialRoomAt(int x, int y)
{
    if (m_specialRooms.empty()) {
        return -1;
    }
    return m_specialRooms[random(x + 564613, y) % m_specialRooms.size()];
}

int statueRace(int x, int y)
{
    return random(x + 1856201, y) % 3 + RaceManaPump;
}

Item Missions::taskRewardAt(int x, int y)
{
    Item item;
    switch (kindAt(x, y)) {
    case MissionKindMagicCraftingBookReward:
        item.id = SpecialItemMagicCraftingBook;
        break;
    case MissionKindRareCraftingBookReward:
        item.id = SpecialItemRareCraftingBook;
        break;
    case MissionKindEpicCraftingBookReward:
        item.id = SpecialItemEpicCraftingBook;
        break;
    case MissionKindLegendaryCraftingBookReward:
        item.id = SpecialItemLegendaryCraftingBook;
        break;
    case MissionKindArtifactReward:
        item = artifactAt(x, y);
        return item;
    default:
        return item;
    }
    item.category = ItemCategorySpecial;
    item.region = m_region;
    return item;
}

void Missions::updateTasks(World *world)
{
    if (world->m_localCreature->m_entityData.currentAbility == AbilityEagleFlight)
        return;
    for (Creature *creature : world->m_creatures) {
        if (!(creature->m_entityData.appearance.appearanceFlags & AppearanceFlagMissionResident) ||
            !creature->m_entityData.binaryToggles.missionComplete || !creature->m_entityData.interactionState)
            continue;
        Creature *player = world->m_localCreature;
        if (plasma::Vector<3, float>(creature->m_entityData.position - player->m_entityData.position).squaredLength() <
            (float)(CompletionRange * CompletionRange)) {
            int x = floorDivide((int)creature->m_entityData.origin[0], BlocksPerPlace);
            int y = floorDivide((int)creature->m_entityData.origin[1], BlocksPerPlace);
            if (world->isLargePlace(x, y)) {
                x = floorDivide(x, 2) * 2;
                y = floorDivide(y, 2) * 2;
            }
            switch (kindAt(x, y)) {
            case MissionKind_15:
                player->m_playerData->finishedMissions.insert(std::array<int, 2> {x, y});
                break;
            case MissionKindSupplierGnome:
                player->m_playerData->finishedMissions.insert(std::array<int, 2> {x, y});
                world->m_localCreature->m_playerData->solvedMissions.insert(std::array<int, 2> {x, y});
                break;
            }
        }
    }
    int firstX = floorDivide((world->m_localCreature->m_entityData.position[0] - Fixed(CompletionRange)).toBlock(),
                             BlocksPerZone);
    int lastX = floorDivide((world->m_localCreature->m_entityData.position[0] + Fixed(CompletionRange)).toBlock(),
                            BlocksPerZone);
    int firstY = floorDivide((world->m_localCreature->m_entityData.position[1] - Fixed(CompletionRange)).toBlock(),
                             BlocksPerZone);
    int lastY = floorDivide((world->m_localCreature->m_entityData.position[1] + Fixed(CompletionRange)).toBlock(),
                            BlocksPerZone);
    for (int zx = firstX; zx <= lastX; ++zx) {
        for (int zy = firstY; zy <= lastY; ++zy) {
            Zone *zone = world->zone(zx, zy);
            if (!zone)
                continue;
            for (StaticEntity &entity : zone->m_staticEntities) {
                if ((entity.type == StaticEntityChestBase02 || entity.type == StaticEntityRewardChest) &&
                    entity.item.category != ItemCategoryNone && !entity.active) {
                    if (plasma::Vector<3, float>(entity.position - world->m_localCreature->m_entityData.position)
                            .squaredLength() < (float)(CompletionRange * CompletionRange)) {
                        int x = floorDivide((int)entity.position[0], BlocksPerPlace);
                        int y = floorDivide((int)entity.position[1], BlocksPerPlace);
                        if (world->isLargePlace(x, y)) {
                            x = floorDivide(x, 2) * 2;
                            y = floorDivide(y, 2) * 2;
                        }
                        if (kindAt(x, y) == MissionKind_15)
                            world->m_localCreature->m_playerData->finishedMissions.insert(std::array<int, 2> {x, y});
                    }
                }
                if (entity.type == StaticEntityBrazier && !entity.active) {
                    int x = floorDivide((int)entity.position[0], BlocksPerPlace);
                    int y = floorDivide((int)entity.position[1], BlocksPerPlace);
                    if (world->isLargePlace(x, y)) {
                        x = floorDivide(x, 2) * 2;
                        y = floorDivide(y, 2) * 2;
                    }
                    if (kindAt(x, y) == MissionKindFlameOfPower) {
                        world->m_localCreature->m_playerData->finishedMissions.insert(std::array<int, 2> {x, y});
                        world->m_localCreature->m_playerData->solvedMissions.insert(std::array<int, 2> {x, y});
                    }
                }
            }
        }
    }

    for (int dx = -CompletionZoneScanRadius; dx <= CompletionZoneScanRadius; ++dx) {
        for (int dy = -CompletionZoneScanRadius; dy <= CompletionZoneScanRadius; ++dy) {
            Creature *player = world->m_localCreature;
            Zone *zone = world->zone(floorDivide(player->m_entityData.position[0].toBlock(), BlocksPerZone) + dx,
                                     floorDivide(player->m_entityData.position[1].toBlock(), BlocksPerZone) + dy);
            if (!zone || zone->m_missionCreatures.empty())
                continue;
            bool allDead = true;
            for (long long id : zone->m_missionCreatures)
                if (world->m_state.m_deadCreatures.find(id) == world->m_state.m_deadCreatures.end())
                    allDead = false;
            if (!allDead)
                continue;
            int x = floorDivide((int)player->m_entityData.position[0], BlocksPerPlace);
            int y = floorDivide((int)player->m_entityData.position[1], BlocksPerPlace);
            if (world->isLargePlace(x, y)) {
                x = floorDivide(x, 2) * 2;
                y = floorDivide(y, 2) * 2;
            }
            int kind = kindAt(x, y);
            if (kind == MissionKind_15 || kind == MissionKindMonster || kind == MissionKindAborigines ||
                kind == MissionKindExchangePartner) {
                player->m_playerData->finishedMissions.insert(std::array<int, 2> {x, y});
            } else {
                player->m_playerData->finishedMissions.insert(std::array<int, 2> {x, y});
                world->m_localCreature->m_playerData->solvedMissions.insert(std::array<int, 2> {x, y});
            }
        }
    }

    int magicBarrierCount = 0, magicBarriersCompleted = 0;
    int manaPumpCount = 0, manaPumpsCompleted = 0;
    int flamesOfPowerCompleted = 0, witchCursesCompleted = 0, demonPortalsCompleted = 0;
    for (const std::pair<const std::array<int, 2>, unsigned int> &mission : m_kinds) {
        if (mission.second == MissionKindArtifactReward &&
            world->m_localCreature->hasItem(artifactAt(mission.first[0], mission.first[1]), 1)) {
            world->m_localCreature->m_playerData->finishedMissions.insert(mission.first);
            world->m_localCreature->m_playerData->solvedMissions.insert(mission.first);
        }
        if (mission.second >= MissionKindMagicCraftingBookReward &&
            mission.second <= MissionKindLegendaryCraftingBookReward &&
            world->m_localCreature->hasSpecialItem(
                mission.second + (SpecialItemMagicCraftingBook - MissionKindMagicCraftingBookReward))) {
            world->m_localCreature->m_playerData->finishedMissions.insert(mission.first);
            world->m_localCreature->m_playerData->solvedMissions.insert(mission.first);
        }
        if (mission.second == MissionKindHangGliderPlacement &&
            world->m_localCreature->hasSpecialItem(SpecialItemHangGlider)) {
            world->m_localCreature->m_playerData->finishedMissions.insert(mission.first);
            world->m_localCreature->m_playerData->solvedMissions.insert(mission.first);
        }
        if (mission.second == MissionKindBoatPlacement && world->m_localCreature->hasSpecialItem(SpecialItemBoat)) {
            world->m_localCreature->m_playerData->finishedMissions.insert(mission.first);
            world->m_localCreature->m_playerData->solvedMissions.insert(mission.first);
        }
        if (mission.second == MissionKindClimbingSpikesPlacement &&
            world->m_localCreature->hasSpecialItem(SpecialItemClimbingSpikes)) {
            world->m_localCreature->m_playerData->finishedMissions.insert(mission.first);
            world->m_localCreature->m_playerData->solvedMissions.insert(mission.first);
        }
        if (mission.second == MissionKindReinsPlacement && world->m_localCreature->hasSpecialItem(SpecialItemReins)) {
            world->m_localCreature->m_playerData->finishedMissions.insert(mission.first);
            world->m_localCreature->m_playerData->solvedMissions.insert(mission.first);
        }
        if (mission.second == MissionKindTreasureSpiritPlacement &&
            world->m_localCreature->hasSpecialItem(SpecialItemTreasureSpirit)) {
            world->m_localCreature->m_playerData->finishedMissions.insert(mission.first);
            world->m_localCreature->m_playerData->solvedMissions.insert(mission.first);
        }
        if (mission.second == MissionKindBellPlacement && world->m_localCreature->hasSpecialItem(SpecialItemBell)) {
            world->m_localCreature->m_playerData->finishedMissions.insert(mission.first);
            world->m_localCreature->m_playerData->solvedMissions.insert(mission.first);
        }
        if (mission.second == MissionKindWhistlePlacement &&
            world->m_localCreature->hasSpecialItem(SpecialItemWhistle)) {
            world->m_localCreature->m_playerData->finishedMissions.insert(mission.first);
            world->m_localCreature->m_playerData->solvedMissions.insert(mission.first);
        }
        if (mission.second == MissionKindHarpPlacement && world->m_localCreature->hasSpecialItem(SpecialItemHarp)) {
            world->m_localCreature->m_playerData->finishedMissions.insert(mission.first);
            world->m_localCreature->m_playerData->solvedMissions.insert(mission.first);
        }
        if (mission.second == MissionKindExchangePartner) {
            int location = world->locationType(mission.first[0], mission.first[1],
                                               world->isLargePlace(mission.first[0], mission.first[1]));
            Creature *player = world->m_localCreature;
            Item item = fetchItem(location, mission.first[0], mission.first[1],
                                  requestCount(mission.first[0], mission.first[1]), m_region);
            if (player->m_inventory.countItem(item) >= ExchangeItemCount)
                world->m_localCreature->m_playerData->finishedMissions.insert(mission.first);
        }
        if (mission.second == MissionKindMagicBarriers) {
            ++magicBarrierCount;
            if (world->m_localCreature->hasCompletedMission(mission.first[0], mission.first[1]))
                ++magicBarriersCompleted;
        }
        if (mission.second == MissionKindManaPumps) {
            ++manaPumpCount;
            if (world->m_localCreature->hasCompletedMission(mission.first[0], mission.first[1]))
                ++manaPumpsCompleted;
        }
        if (mission.second == MissionKindFlameOfPower) {
            if (world->m_localCreature->hasCompletedMission(mission.first[0], mission.first[1]))
                ++flamesOfPowerCompleted;
        }
        if (mission.second == MissionKindWitchCurse) {
            if (world->m_localCreature->hasCompletedMission(mission.first[0], mission.first[1]))
                ++witchCursesCompleted;
        }
        if (mission.second == MissionKindDemonPortal) {
            if (world->m_localCreature->hasCompletedMission(mission.first[0], mission.first[1]))
                ++demonPortalsCompleted;
        }
    }
    world->m_localCreature->m_entityData.setToggles(1 << CreatureToggleRemovePetrification, witchCursesCompleted > 0);
    world->m_localCreature->m_entityData.setToggles(1 << CreatureToggleSuppressResidentPromotion,
                                                    demonPortalsCompleted > 0);
    Creature::Buff *buff = world->m_localCreature->findBuff(BuffAncientpower);
    if (flamesOfPowerCompleted != 0 &&
        (!buff || buff->strength != (float)flamesOfPowerCompleted || buff->timeRemaining < MissionBuffRenewTime)) {
        Creature::Buff renewal;
        renewal.type = BuffAncientpower;
        renewal.strength = (float)flamesOfPowerCompleted;
        renewal.timeRemaining = MissionBuffDuration;
        renewal.sourceId = world->m_localCreature->m_id;
        world->applyBuff(world->m_localCreature, renewal, true);
    } else if (buff && flamesOfPowerCompleted == 0) {
        world->resetBuff(world->m_localCreature, BuffAncientpower);
    }
    world->m_localCreature->m_entityData.setToggles(1 << CreatureToggleMagicBarriersDisabled,
                                                    magicBarrierCount != 0 &&
                                                        magicBarriersCompleted == magicBarrierCount);
    int manaPumpsRemaining = manaPumpCount - manaPumpsCompleted;
    buff = world->m_localCreature->findBuff(BuffManaabsorption);
    if (manaPumpsRemaining != 0 &&
        (!buff || buff->strength != (float)manaPumpsRemaining || buff->timeRemaining < MissionBuffRenewTime)) {
        Creature::Buff renewal;
        renewal.type = BuffManaabsorption;
        renewal.strength = (float)manaPumpsRemaining;
        renewal.timeRemaining = MissionBuffDuration;
        renewal.sourceId = world->m_localCreature->m_id;
        world->applyBuff(world->m_localCreature, renewal, true);
    } else if (buff && manaPumpsRemaining == 0) {
        world->resetBuff(world->m_localCreature, BuffManaabsorption);
    }
}

void Missions::generateBonePiles(Zone *zone)
{
    std::vector<int> races = {RaceSkeleton, RaceSkeletonDog, RaceSkeletonHorse, RaceSkeletonKnight, RaceNecromancer};
    generateObjects(zone, RaceBonePile, races, FactionUnholyPact, 0, ClassNone);
}

void Missions::generateDemonPortals(Zone *zone)
{
    std::vector<int> races = {RaceDarkCultistMale, RaceImp, RaceDemonMarauder, RaceFlyingEye};
    generateObjects(zone, RaceDemonPortal, races, FactionCultOfDoom, 0, ClassNone);
}

void Missions::generateHornetsNests(Zone *zone)
{
    std::vector<int> races = {RaceHornet};
    generateObjects(zone, RaceHornetsNest, races, 0, 0, ClassNone);
}

void Missions::generateMolehills(Zone *zone)
{
    std::vector<int> races = {RaceMole};
    generateObjects(zone, RaceMolehill, races, 0, 0, ClassNone);
}

// A place overrun by creatures that come out of objects: the mission's resident in one of the place's mission
// buildings, the objects to destroy (hostility 7, recorded in the zone's m_trackedMissionCreatureIds) on 1 to 12 of
// the open plots, shuffled, and creatures of the given races (hostility 1) beside each object and on one more plot in
// twelve. Objects other than the unholy tomb in 175..180 get no creatures beside them.
void Missions::generateObjects(Zone *zone, int race, const std::vector<int> &races, int factionKind, unsigned int flags,
                               int classType)
{
    int placeX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace);
    int placeY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace);
    World *world = m_world;
    bool large = world->isLargePlace(placeX, placeY);
    if (large) {
        placeX = floorDivide(placeX, 2) * 2;
        placeY = floorDivide(placeY, 2) * 2;
    }
    int type = world->locationType(placeX, placeY, large);
    int level = requestCount(placeX, placeY);
    srand(random(placeX + 39441, placeY));
    World *generator = m_world;
    plasma::Vector<2, int> center = generator->placeCenter(placeX, placeY);
    std::vector<plasma::Vector<2, int>> buildings;
    int minX = floorDivide(placeX * BlocksPerPlace, BlocksPerCell);
    int maxX = large ? floorDivide(placeX * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeX * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int minY = floorDivide(placeY * BlocksPerPlace, BlocksPerCell);
    int maxY = large ? floorDivide(placeY * BlocksPerPlace + 2 * BlocksPerPlace, BlocksPerCell)
                     : floorDivide(placeY * BlocksPerPlace + BlocksPerPlace, BlocksPerCell);
    int targetType = generator->missionTargetType(type, placeX, placeY, large);
    for (int x = minX; x < maxX; x++) {
        for (int y = minY; y < maxY; y++) {
            float influence =
                m_world->placeInfluence(type, large,
                                        m_world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                      y * BlocksPerCell + BlocksPerCell / 2));
            if (influence > PlaceCoreInfluence && isMissionBuilding(type, x, y, targetType)) {
                buildings.push_back(plasma::Vector<2, int>(x, y));
            }
        }
    }
    if (buildings.empty()) {
        buildings.push_back(
            plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
    }
    int zoneMinX = floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerCell);
    int zoneMaxX = floorDivide(zone->m_position[0] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    int zoneMinY = floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerCell);
    int zoneMaxY = floorDivide(zone->m_position[1] * BlocksPerZone + BlocksPerZone, BlocksPerCell);
    plasma::Vector<2, int> home = buildings[rand() % buildings.size()];
    std::vector<plasma::Vector<2, int>> plots;
    for (int x = minX; x < maxX; x++) {
        for (int y = minY; y < maxY; y++) {
            World *world = m_world;
            float influence =
                world->placeInfluence(type, large,
                                      world->placeSquaredDistance(center, x * BlocksPerCell + BlocksPerCell / 2,
                                                                  y * BlocksPerCell + BlocksPerCell / 2));
            if (!(influence > PlaceCoreInfluence)) {
                continue;
            }
            plasma::Vector<2, int> point = world->streetPoint(x, y);
            World::AreaInfo info;
            world->fillAreaInfo(point[0], point[1], info);
            if (isOpenBuilding(world->streetBuildingType(x, y, point, info)) && isSuitableGround(x, y)) {
                plots.push_back(plasma::Vector<2, int>(x, y));
            }
        }
    }
    if (plots.empty()) {
        plots.push_back(
            plasma::Vector<2, int>(floorDivide(center[0], BlocksPerCell), floorDivide(center[1], BlocksPerCell)));
    }
    shuffleWithRand(plots.begin(), plots.end());
    int plotCount = (int)plots.size();
    // One to five objects (at most half the plots, at least one in twenty of them), never more than 8..12.
    long long count = (std::min)(rand() % 5 + 8,
                                 (std::max)(1, (std::max)((std::min)(rand() % 5 + 1, plotCount / 2), plotCount / 20)));
    for (long long i = 0; i < count; i++) {
        zone->m_missionGroupZones.insert(std::array<int, 2> {floorDivide(plots[i][0] * 16, BlocksPerZone),
                                                             floorDivide(plots[i][1] * 16, BlocksPerZone)});
    }
    zone->m_trackedMissionCreatureIds.clear();
    if (home[0] >= zoneMinX && home[0] < zoneMaxX && home[1] >= zoneMinY && home[1] < zoneMaxY) {
        Zone::Resident *resident = new Zone::Resident();
        resident->hostility = HostilityFriendlyNpc;
        resident->level = level;
        if (!resident->setNamesake(placeX, placeY, type)) {
            if (factionKind >= FactionCultOfDoom && factionKind <= FactionUnholyPact) {
                resident->race = randomPeacefulRace();
                resident->classType = ClassOrderOfTheLightMember;
                resident->factionKind = FactionOrderOfTheLight;
            } else {
                resident->race = randomFighterRace();
                resident->classType = ClassDruidOfMana;
                resident->factionKind = FactionDruidsOfMana;
            }
        }
        resident->appearance.appearanceFlags = AppearanceFlagCaptive | AppearanceFlagMissionResident;
        resident->behaviours = BehaviourWander | BehaviourUseFurniture | Behaviour_8192;
        resident->position = zone->randomSpawnPosition(home[0], home[1]);
        fillAppearance(resident->race, resident->appearance, nullptr, false);
        equip(resident->hostility, resident->race, resident->level, resident->appearance.appearanceFlags,
              resident->factionKind, resident->classType, resident->specialization, resident->equipment,
              resident->specialSkills, resident->summonAppearanceMode, resident->summonRaces, resident->inventory);
        Item reward = taskRewardAt(placeX, placeY);
        if (reward.category != ItemCategoryNone) {
            resident->inventory.add(reward, 1);
        }
        zone->adoptResident(resident);
    }
    for (int i = 0; i < plots.size(); i++) {
        if (i < count || rand() % 12 == 0) {
            plasma::Vector<2, int> plot = plots[i];
            if (plot[0] >= zoneMinX && plot[0] < zoneMaxX && plot[1] >= zoneMinY && plot[1] < zoneMaxY) {
                srand(random(plot[0] + 5189520, plot[1]));
                if (i < count) {
                    Zone::Resident *target = new Zone::Resident();
                    target->hostility = HostilityAttackableObject;
                    target->level = level;
                    target->race = race;
                    target->appearance.appearanceFlags = AppearanceFlagMissionResident;
                    target->destination = plasma::Vector<2, int>(placeX, placeY);
                    target->position = zone->randomSpawnPosition(plot[0], plot[1]);
                    target->angle = (float)(rand() % 4 * 90);
                    fillAppearance(target->race, target->appearance, nullptr, false);
                    equip(target->hostility, target->race, target->level, target->appearance.appearanceFlags,
                          target->factionKind, target->classType, target->specialization, target->equipment,
                          target->specialSkills, target->summonAppearanceMode, target->summonRaces, target->inventory);
                    zone->adoptResident(target);
                    zone->m_trackedMissionCreatureIds.push_back(target->id);
                }
                if (race == RaceUnholyTomb || race <= RaceLavaTurtle || race >= RaceWoodChest) {
                    Zone::Resident *creature = new Zone::Resident();
                    creature->hostility = HostilityEnemy;
                    creature->level = level;
                    creature->race = randomRace(races[rand() % races.size()]);
                    creature->factionKind = factionKind;
                    creature->behaviours = BehaviourWander | BehaviourUseFurniture;
                    creature->appearance.appearanceFlags = flags | AppearanceFlagMissionResident;
                    creature->classType = classType;
                    creature->destination = plasma::Vector<2, int>(placeX, placeY);
                    fillAppearance(creature->race, creature->appearance, nullptr, false);
                    equip(creature->hostility, creature->race, creature->level, creature->appearance.appearanceFlags,
                          creature->factionKind, creature->classType, creature->specialization, creature->equipment,
                          creature->specialSkills, creature->summonAppearanceMode, creature->summonRaces,
                          creature->inventory);
                    if ((creature->race == RaceEmpireSoldierMale || creature->race == RaceEmpireSoldierFemale) &&
                        rand() % 2 != 0) {
                        creature->position = zone->randomPosition(plot[0], plot[1], PositionAny);
                        creature->currentAbility = AbilitySlowFall;
                    } else {
                        creature->position = zone->randomSpawnPosition(plot[0], plot[1]);
                    }
                    zone->adoptResident(creature);
                }
            }
        }
    }
}

void Missions::generateSlimeWells(Zone *zone)
{
    std::vector<int> races = {RaceGreenSlime, RaceBlueSlime, RacePinkSlime, RaceYellowSlime};
    generateObjects(zone, RaceSlimeWell, races, 0, 0, ClassNone);
}

void Missions::generateStatues(Zone *zone)
{
    std::vector<int> races = {RaceEmpireSoldierMale};
    generateObjects(zone,
                    statueRace(floorDivide(zone->m_position[0] * BlocksPerZone, BlocksPerPlace),
                               floorDivide(zone->m_position[1] * BlocksPerZone, BlocksPerPlace)),
                    races, FactionSteelEmpire, 0, ClassNone);
}

void Missions::generateUnholyTombs(Zone *zone)
{
    std::vector<int> races = {RaceZombie, RaceNecromancer};
    generateObjects(zone, RaceUnholyTomb, races, FactionUnholyPact, 0, ClassNone);
}

std::vector<std::string> Missions::talk(Creature *creature)
{
    // Daily dialog seed combines the day (stride 13) and creature ID (modulo 333333). Draws choose artifact rumours
    // with probability one eighth and small talk, greetings and remarks with their own odds. Signed day and seed
    // arithmetic can overflow.
    int seed =
        hash(13 * m_world->m_state.m_day + static_cast<int>(static_cast<unsigned long long>(creature->m_id) % 333333));
    std::vector<std::string> text;
    if (creature->m_entityData.binaryToggles.active) {
        help(seed, text);
        return text;
    }
    if (creature->m_entityData.race == RaceCreatorMale || creature->m_entityData.race == RaceCreatorFemale) {
        std::string greeting("hello");
        text.push_back(greeting);
        return text;
    }
    if (creature->m_entityData.classType == ClassArenaChampion) {
        if (creature->m_entityData.interactionState == InteractionStateDone) {
            text << "congratulations";
            if (hash(seed) & 1) {
                text << "new_page";
                text << "good_work";
            }
        } else {
            text << "a_new_challenger_entered_the_arena";
        }
        return text;
    }
    if (creature->m_entityData.hostilityType == HostilityFriendlyNpc &&
        (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagMissionResident)) {
        int x = m_world->placeIndex((int)creature->m_entityData.origin[0]);
        int y = m_world->placeIndex((int)creature->m_entityData.origin[1]);
        World *world = m_world;
        bool large = world->isLargePlace(x, y);
        if (large) {
            x = floorDivide(x, 2) * 2;
            y = floorDivide(y, 2) * 2;
        }
        int location = world->locationType(x, y, large);
        if (creature->m_entityData.binaryToggles.missionComplete ||
            m_world->m_localCreature->hasFinishedMission(x, y)) {
            text << "thank_you";
            if (hash(seed++) & 1) {
                text << "new_page";
                text << "good_work";
            }
            if (hash(seed) & 1) {
                text << "!love";
            }
            return text;
        }
        text << "define_location" << x << y << location;
        switch (determineTopic(x, y)) {
        case MissionHornetsNests:
            help(seed, text);
            text << "new_page";
            text << "define_model" << RaceHornet;
            text << "the_location_is_full_of_creatures";
            text << "new_page";
            text << "define_model" << RaceHornetsNest;
            text << "you_must_destroy_the_objects";
            break;
        case MissionMolehills:
            help(seed, text);
            text << "new_page";
            text << "define_model" << RaceMole;
            text << "the_location_is_full_of_creatures";
            text << "new_page";
            text << "define_model" << RaceMolehill;
            text << "you_must_destroy_the_objects";
            break;
        case MissionSlimeWells:
            help(seed, text);
            text << "new_page";
            text << "define_model" << RaceGreenSlime;
            text << "the_location_is_full_of_creatures";
            text << "new_page";
            text << "define_model" << RaceSlimeWell;
            text << "you_must_destroy_the_objects";
            break;
        case MissionUnholyTombs:
            help(seed, text);
            text << "new_page";
            text << "define_model" << RaceZombie;
            text << "the_location_is_full_of_creatures";
            text << "new_page";
            text << "define_model" << RaceUnholyTomb;
            text << "you_must_destroy_the_objects";
            break;
        case MissionBonePiles:
            help(seed, text);
            text << "new_page";
            text << "define_model" << RaceSkeleton;
            text << "the_location_is_full_of_creatures";
            text << "new_page";
            text << "define_model" << RaceBonePile;
            text << "you_must_destroy_the_objects";
            break;
        case MissionDemonPortals:
            help(seed, text);
            text << "new_page";
            text << "define_model" << RaceFamilyDemon;
            text << "the_location_is_full_of_creatures";
            text << "new_page";
            text << "define_model" << RaceDemonPortal;
            text << "you_must_destroy_the_objects";
            break;
        case MissionUndeadInvasion:
            help(seed, text);
            text << "new_page";
            text << "define_model" << RaceFamilyUnholyPactFollower;
            text << "the_location_is_full_of_creatures";
            text << "new_page";
            text << "you_must_defeat_them";
            break;
        case MissionDemonInvasion:
            help(seed, text);
            text << "new_page";
            text << "define_model" << RaceFamilyCultOfDoomAdherer;
            text << "the_location_is_full_of_creatures";
            text << "new_page";
            text << "you_must_defeat_them";
            break;
        case MissionPlantCreatureInvasion:
            help(seed, text);
            text << "new_page";
            text << "the_location_is_full_of_plant_creatures";
            text << "new_page";
            text << "you_must_defeat_them";
            break;
        case MissionInsectInvasion:
            help(seed, text);
            text << "new_page";
            text << "the_location_is_full_of_insects";
            text << "new_page";
            text << "you_must_defeat_them";
            break;
        case MissionEmpireSoldierInvasion:
            help(seed, text);
            text << "new_page";
            text << "define_model" << RaceFamilySteelEmpireForce;
            text << "the_location_is_full_of_creatures";
            text << "new_page";
            text << "you_must_defeat_them";
            break;
        case MissionStatues:
            help(seed, text);
            text << "new_page";
            text << "define_model" << -99996;
            text << "the_location_is_full_of_creatures";
            text << "new_page";
            text << "define_model" << statueRace(x, y);
            text << "you_must_destroy_the_objects";
            break;
        case MissionSlimeInvasion:
            help(seed, text);
            text << "new_page";
            text << "define_model" << 37;
            text << "the_location_is_full_of_creatures";
            text << "new_page";
            text << "you_must_defeat_them";
            break;
        }
        return text;
    }
    if ((hash(seed++) & 7) == 0) {
        int y = m_world->placeIndex((int)creature->m_entityData.origin[1]);
        int x = m_world->placeIndex((int)creature->m_entityData.origin[0]);
        Realm realm(m_world->placeRealm(x, y));
        plasma::Vector<2, int> place = artifactPlace(seed, realm.id);
        if (place[0] != NoCoordinate) {
            Item artifact = artifactAt(place[0], place[1]);
            plasma::Vector<2, int> cell(place[0], place[1]);
            int location = m_world->locationType(cell);
            int dungeon = m_world->dungeonType(place[0], place[1], location, m_world->isLargePlace(place[0], place[1]));
            if (dungeon) {
                location = dungeon;
            }
            if (creature->m_entityData.classType == ClassVillager) {
                return realm.treasureHint(artifact, place[0], place[1], location, seed,
                                          m_world->m_localCreature->hasItem(artifact, 1));
            }
            return realm.artifactRumor(artifact, place[0], place[1], location, seed,
                                       m_world->m_localCreature->hasItem(artifact, 1));
        }
    }
    if ((hash(seed++) & 3) != 0) {
        if (hash(seed++) % 3 != 0) {
            creature->greet(seed, text);
            text << "new_page";
        }
        if (creature->m_entityData.classType >= ClassWarrior && creature->m_entityData.classType <= ClassRogue) {
            if ((hash(seed++) & 3) == 0) {
                std::vector<Item> equipment;
                if (m_world->m_localCreature->m_entityData.equipment.head.category != ItemCategoryNone) {
                    equipment.push_back(m_world->m_localCreature->m_entityData.equipment.head);
                }
                if (m_world->m_localCreature->m_entityData.equipment.neck.category != ItemCategoryNone) {
                    equipment.push_back(m_world->m_localCreature->m_entityData.equipment.neck);
                }
                if (m_world->m_localCreature->m_entityData.equipment.chest.category != ItemCategoryNone) {
                    equipment.push_back(m_world->m_localCreature->m_entityData.equipment.chest);
                }
                if (m_world->m_localCreature->m_entityData.equipment.feet.category != ItemCategoryNone) {
                    equipment.push_back(m_world->m_localCreature->m_entityData.equipment.feet);
                }
                if (m_world->m_localCreature->m_entityData.equipment.hands.category != ItemCategoryNone) {
                    equipment.push_back(m_world->m_localCreature->m_entityData.equipment.hands);
                }
                if (m_world->m_localCreature->m_entityData.equipment.shoulder.category != ItemCategoryNone) {
                    equipment.push_back(m_world->m_localCreature->m_entityData.equipment.shoulder);
                }
                if (m_world->m_localCreature->m_entityData.equipment.weaponLeft.category != ItemCategoryNone) {
                    equipment.push_back(m_world->m_localCreature->m_entityData.equipment.weaponLeft);
                }
                if (m_world->m_localCreature->m_entityData.equipment.weaponRight.category != ItemCategoryNone) {
                    equipment.push_back(m_world->m_localCreature->m_entityData.equipment.weaponRight);
                }
                if (m_world->m_localCreature->m_entityData.equipment.ringLeft.category != ItemCategoryNone) {
                    equipment.push_back(m_world->m_localCreature->m_entityData.equipment.ringLeft);
                }
                if (m_world->m_localCreature->m_entityData.equipment.ringRight.category != ItemCategoryNone) {
                    equipment.push_back(m_world->m_localCreature->m_entityData.equipment.ringRight);
                }
                if (!equipment.empty()) {
                    if (hash(seed++) & 1) {
                        text << "!alert";
                    }
                    text << "define_item" << equipment[hash(seed++) % equipment.size()];
                    text << "nice_equipment_item";
                    if (hash(seed++) & 1) {
                        switch (hash(seed++) % 3) {
                        default:
                            text << "!love";
                            break;
                        case 1:
                            text << "new_page";
                            text << "im_jealous";
                            if (hash(seed) & 1) {
                                text << "!angry";
                            }
                            break;
                        case 2:
                            text << "new_page";
                            text << "nice";
                            if (hash(seed) & 1) {
                                text << "!love";
                            }
                            break;
                        }
                    }
                    return text;
                }
            }
            if ((hash(seed++) & 3) == 0) {
                switch (hash(seed++) % 2) {
                default:
                    if (hash(seed++) & 1) {
                        text << "!alert";
                    }
                    switch (m_world->m_localCreature->m_entityData.classType) {
                    case ClassWarrior:
                        text << "so_you_are_a_warrior";
                        break;
                    case ClassMage:
                        text << "so_you_are_a_mage";
                        break;
                    case ClassRanger:
                        text << "so_you_are_a_ranger";
                        break;
                    case ClassRogue:
                        text << "so_you_are_a_rogue";
                        break;
                    }
                    text << "new_page";
                    if (m_world->m_localCreature->m_entityData.classType == creature->m_entityData.classType) {
                        text << "!love";
                        if (hash(seed) & 1) {
                            text << "good_choice";
                            return text;
                        }
                    } else {
                        text << "!angry";
                    }
                    switch (creature->m_entityData.classType) {
                    case ClassWarrior:
                        text << "warriors_are_the_strongest";
                        break;
                    case ClassMage:
                        text << "mages_are_the_strongest";
                        break;
                    case ClassRanger:
                        text << "rangers_are_the_strongest";
                        break;
                    case ClassRogue:
                        text << "rogues_are_the_strongest";
                        break;
                    }
                    break;
                case 1:
                    switch (creature->m_entityData.classType) {
                    case ClassWarrior:
                        text << "i_want_to_be_the_best_warrior";
                        break;
                    case ClassMage:
                        text << "i_want_to_be_the_best_mage";
                        break;
                    case ClassRanger:
                        text << "i_want_to_be_the_best_ranger";
                        break;
                    case ClassRogue:
                        text << "i_want_to_be_the_best_rogue";
                        break;
                    }
                    break;
                }
                return text;
            }
        }
        int x = m_world->placeIndex((int)creature->m_entityData.origin[0]);
        int y = m_world->placeIndex((int)creature->m_entityData.origin[1]);
        World *world = m_world;
        bool large = world->isLargePlace(x, y);
        if (large) {
            x = floorDivide(x, 2) * 2;
            y = floorDivide(y, 2) * 2;
        }
        int location = world->locationType(x, y);
        int blockY = (int)creature->m_entityData.origin[1];
        int blockX = (int)creature->m_entityData.origin[0];
        World *locationWorld = m_world;
        float distance = locationWorld->placeSquaredDistance(locationWorld->placeCenter(x, y), blockX, blockY);
        float influence = locationWorld->placeInfluence(location, large, distance);
        if (influence > PlaceCoreInfluence && location == PlaceVillage && (hash(seed++) & 3) == 0) {
            text << "define_location" << x << y << location;
            if (creature->m_entityData.classType == ClassVillager) {
                if (hash(seed) & 1) {
                    text << "welcome_to_location";
                } else {
                    text << "how_do_you_like_our_zone";
                }
            } else {
                std::vector<std::string> activities = {"#glassflasks", "#sugarcubes", "#bombs",     "#petfood",
                                                       "#flight",      "#inn",        "#buyweapon", "#craftweapon",
                                                       "#buyarmor",    "#craftarmor", "#identify",  "#specialization",
                                                       "#villagers",   "#friends",    "#sell"};
                text << activities[hash(seed) % activities.size()];
                text << "im_in_this_zone_to_do_tasks";
            }
            return text;
        }
        switch (hash(seed++) % 19) {
        default: {
            int pet = m_world->m_petRaces[hash(seed++) % m_world->m_petRaces.size()];
            text << "define_model" << pet;
            text << "do_you_know_what_creatures_eat";
            text << "new_page";
            if (hash(seed++) & 1) {
                if (hash(seed) & 1) {
                    text << "!question";
                } else {
                    text << "!love";
                }
            }
            Item food;
            food.category = ItemCategoryPetFood;
            food.id = pet;
            text << "define_item" << food;
            text << "items_of_course";
            break;
        }
        case 1: {
            int pet = m_world->randomPet(seed++);
            text << "define_model" << pet;
            text << "my_favorite_pet_is";
            if (hash(seed) & 1) {
                text << "!love";
            }
            break;
        }
        case 2: {
            int biome = m_world->landBiome(m_region);
            int blockY = (int)creature->m_entityData.origin[1];
            int blockX = (int)creature->m_entityData.origin[0];
            Realm realm(m_world->placeRealm(m_world->placeIndex(blockX), m_world->placeIndex(blockY)));
            return realm.landHistory(seed, biome, fastRandom(m_region[0], m_region[1]));
        }
        case 3: {
            World::AreaInfo info;
            m_world->fillAreaInfo((int)creature->m_entityData.position[0], (int)creature->m_entityData.position[1],
                                  info);
            World::BiomeSample sample;
            m_world->biomeSample((int)creature->m_entityData.position[0], (int)creature->m_entityData.position[1],
                                 sample);
            float temperature = sample.temperature(info.type, info.influence);
            // Hot above 0.8, warm above 0.6, cold below 0.3.
            if (temperature > 0.8f) {
                text << "its_hot" << "!sweat";
            } else if (temperature > 0.6f) {
                text << "its_warm";
                if (hash(seed) & 1) {
                    text << "!love";
                }
            } else if (temperature < 0.3f) {
                text << "its_cold";
                if (hash(seed) & 1) {
                    text << "!angry";
                }
            } else {
                text << "its_pleasant";
                if (hash(seed) & 1) {
                    text << "!love";
                }
            }
            break;
        }
        case 4:
            if (hash(seed++) & 1) {
                text << "!alert";
            }
            text << "i_like_your_style";
            if (hash(seed) & 1) {
                text << "!love";
            }
            break;
        case 5:
            if (hash(seed++) & 1) {
                text << "!question";
            }
            text << "you_look_strange";
            if (hash(seed) & 1) {
                text << "!sweat";
            }
            break;
        case 6:
            if (hash(seed) & 1) {
                text << "!question";
            }
            text << "do_we_know_each_other";
            break;
        case 7:
            if (m_world->m_localCreature->m_entityData.equipment.pet.category == ItemCategoryNone) {
                if (hash(seed++) & 1) {
                    text << "!question";
                }
                text << "dont_you_have_a_pet";
                if (hash(seed++) & 1) {
                    if (hash(seed) & 1) {
                        text << "!angry";
                    }
                    text << "what_a_shame";
                }
            } else {
                text << "define_model" << m_world->m_localCreature->m_entityData.equipment.pet.id;
                text << "your_pet_is_adorable";
                if (hash(seed++) & 1) {
                    text << "!love";
                }
                if (hash(seed) & 1) {
                    text << "may_i_pet_it";
                }
            }
            break;
        case 8:
            text << "define_model" << m_world->randomPet(seed++);
            text << "i_have_a_new_pet";
            if (hash(seed++) & 1) {
                text << "new_page";
                switch (hash(seed++) % 2) {
                default:
                    text << "i_love_him";
                    text << "!love";
                    break;
                case 1:
                    text << "he_is_cheeky";
                    if (hash(seed) & 1) {
                        text << "!sweat";
                    } else {
                        text << "!angry";
                    }
                    break;
                }
            }
            break;
        case 9:
            text << "i_like_your_name";
            break;
        case 10:
            text << "are_you_an_adventurer";
            break;
        case 11:
            text << "i_love_hang_gliding";
            break;
        case 12:
            text << "do_you_like_hang_gliding";
            if (hash(seed++) & 1) {
                text << "new_page";
                switch (hash(seed) % 5) {
                default:
                    text << "how_does_it_feel_to_fly";
                    break;
                case 1:
                    text << "i_wish_i_could_fly";
                    break;
                case 2:
                    text << "arent_you_afraid_of_heights" << "!sweat";
                    break;
                case 3:
                    text << "how_far_can_you_fly";
                    break;
                case 4:
                    text << "how_high_can_you_fly";
                    break;
                }
            }
            break;
        case 13:
            text << "are_you_a_good_climber";
            if (hash(seed++) & 1) {
                text << "new_page";
                switch (hash(seed) % 3) {
                default:
                    text << "whats_the_tallest_mountain_that_you_climbed";
                    break;
                case 1:
                    text << "be_careful_during_climbing";
                    break;
                case 2:
                    text << "arent_you_afraid_of_heights" << "!sweat";
                    break;
                }
            }
            break;
        case 14:
            text << "do_you_like_sailing";
            if (hash(seed++) & 1) {
                text << "new_page";
                switch (hash(seed++) % 5) {
                default:
                    text << "i_wonder_whats_on_the_other_side_of_the_ocean";
                    break;
                case 1:
                    text << "i_wish_you_a_safe_journey";
                    break;
                case 2:
                    text << "id_love_to_sail_from_island_to_island";
                    break;
                case 3:
                    text << "my_relative_was_a_sailor";
                    break;
                case 4:
                    text << "i_dont_have_a_boat";
                    if (hash(seed) & 1) {
                        text << "!sweat";
                    } else {
                        text << "!angry";
                    }
                    break;
                }
            }
            break;
        case 15:
            switch (hash(seed++) % 6) {
            default:
                text << "i_love_nature";
                break;
            case 1:
                text << "i_love_the_beach";
                break;
            case 2:
                text << "i_love_the_river";
                break;
            case 3:
                text << "i_love_the_lake";
                break;
            case 4:
                text << "i_love_the_forest";
                break;
            case 5:
                text << "i_love_the_mountains";
                break;
            }
            if (hash(seed) & 1) {
                text << "!love";
            }
            break;
        case 16:
            text << "define_model" << m_world->randomPet(seed);
            text << "i_want_to_have_a_pet";
            break;
        case 17:
            if (hash(seed++) & 1) {
                text << "!sweat";
            } else {
                text << "!angry";
            }
            text << "i_dont_like_walking";
            if (hash(seed) & 1) {
                text << "new_page";
                text << "i_prefer_riding";
                text << "!love";
            }
            break;
        case 18:
            text << "define_model";
            srand(seed);
            switch (hash(seed++) % 3) {
            default:
                text << randomUndead();
                break;
            case 1:
                text << randomDemonCreature();
                break;
            case 2:
                text << randomPlantCreature();
                break;
            }
            text << "creatures_are_really_strong";
            if (hash(seed) & 1) {
                text << "!sweat";
            }
            break;
        }

        return text;
    }
    plasma::Vector<2, int> place = randomPlace(13 * m_world->m_state.m_day + static_cast<int>(creature->m_id));
    plasma::Vector<2, int> cell(place[0], place[1]);
    int location = m_world->locationType(cell);
    text << "new_page";
    text << "define_location" << place[0] << place[1] << location;
    plasma::Vector<2, int> center = m_world->placeCenter(place[0], place[1]);
    text << "define_direction" << (int)(Fixed(center[0]) - creature->m_entityData.position[0])
         << (int)(Fixed(center[1]) - creature->m_entityData.position[1]);
    int kind = kindAt(place[0], place[1]);
    Item item;
    switch (kind) {
    case MissionKindWitchCurse:
        if (m_world->m_localCreature->hasCompletedMission(place[0], place[1])) {
            text << "!alert";
            text << "so_you_defeated_the_witch";
            text << "new_page";
            text << "nice";
            if (hash(seed) & 1) {
                text << "!love";
            }
        } else {
            text << "a_witch_lives_in_location_direction_of_here";
            text << "mark_location" << place[0] << place[1];
            text << "define_location" << m_village << m_world->locationType(m_village);
            text << "she_has_petrified_the_villagers_of_the_location";
            text << "!sweat";
            text << "you_must_defeat_her_to_break_the_curse";
        }
        return text;
    case MissionKindDemonPortal:
        if (m_world->m_localCreature->hasCompletedMission(place[0], place[1])) {
            text << "!alert";
            text << "so_you_destroyed_the_demon_portal";
            text << "new_page";
            text << "nice";
            if (hash(seed) & 1) {
                text << "!love";
            }
        } else {
            text << "a_demon_portal_was_seen_in_location_direction_of_here";
            int biome = m_world->landBiome(m_region);
            text << "define_land";
            text << biome << fastRandom(m_region[0], m_region[1]);
            text << "mark_location" << place[0] << place[1];
            text << "the_demons_are_possessing_the_residents_of_the_land";
            text << "!sweat";
        }
        return text;
    case MissionKindHangGliderPlacement:
        item.category = ItemCategorySpecial;
        item.id = SpecialItemHangGlider;
        break;
    case MissionKindBoatPlacement:
        item.category = ItemCategorySpecial;
        item.id = SpecialItemBoat;
        break;
    case MissionKindClimbingSpikesPlacement:
        item.category = ItemCategorySpecial;
        item.id = SpecialItemClimbingSpikes;
        break;
    case MissionKindReinsPlacement:
        item.category = ItemCategorySpecial;
        item.id = SpecialItemReins;
        break;
    case MissionKindHarpPlacement:
        item.category = ItemCategorySpecial;
        item.id = SpecialItemHarp;
        break;
    case MissionKindWhistlePlacement:
        item.category = ItemCategorySpecial;
        item.id = SpecialItemWhistle;
        break;
    case MissionKindBellPlacement:
        item.category = ItemCategorySpecial;
        item.id = SpecialItemBell;
        break;
    case MissionKindTreasureSpiritPlacement:
        item.category = ItemCategorySpecial;
        item.id = SpecialItemTreasureSpirit;
        break;
    case MissionKindMagicCraftingBookReward:
        item.category = ItemCategorySpecial;
        item.id = SpecialItemMagicCraftingBook;
        break;
    case MissionKindRareCraftingBookReward:
        item.category = ItemCategorySpecial;
        item.id = SpecialItemRareCraftingBook;
        break;
    case MissionKindEpicCraftingBookReward:
        item.category = ItemCategorySpecial;
        item.id = SpecialItemEpicCraftingBook;
        break;
    case MissionKindLegendaryCraftingBookReward:
        item.category = ItemCategorySpecial;
        item.id = SpecialItemLegendaryCraftingBook;
        break;
    }
    if (item.category != ItemCategoryNone) {
        text << "define_item" << item;
        if (item.category == ItemCategorySpecial && m_world->m_localCreature->hasSpecialItem(item.id)) {
            if (hash(seed++) & 1) {
                text << "!alert";
            }
            text << "oh_you_already_have_the_item";
            text << "new_page";
            if (item.id == SpecialItemBell) {
                text << "new_page" << "spooky" << "!sweat";
                return text;
            }
            text << "nice";
            if (hash(seed++) & 1) {
                text << "!love";
            }
            if (item.id >= SpecialItemMagicCraftingBook && item.id <= SpecialItemLegendaryCraftingBook &&
                (hash(seed++) & 1)) {
                text << "new_page" << "im_curious_what_you_will_craft_with_it";
            }
            if (item.id == SpecialItemHangGlider) {
                if (hash(seed++) & 1) {
                    text << "new_page";
                    switch (hash(seed) % 5) {
                    default:
                        text << "how_does_it_feel_to_fly";
                        break;
                    case 1:
                        text << "i_wish_i_could_fly";
                        break;
                    case 2:
                        text << "arent_you_afraid_of_heights" << "!sweat";
                        break;
                    case 3:
                        text << "how_far_can_you_fly";
                        break;
                    case 4:
                        text << "how_high_can_you_fly";
                        break;
                    }
                }
            } else if (item.id == SpecialItemBoat) {
                if (hash(seed++) & 1) {
                    text << "new_page";
                    switch (hash(seed) & 3) {
                    default:
                        text << "i_wonder_whats_on_the_other_side_of_the_ocean";
                        break;
                    case 1:
                        text << "i_wish_you_a_safe_journey";
                        break;
                    case 2:
                        text << "id_love_to_sail_from_island_to_island";
                        break;
                    case 3:
                        text << "my_relative_was_a_sailor";
                        break;
                    }
                }
            } else if (item.id == SpecialItemClimbingSpikes) {
                if (hash(seed++) & 1) {
                    text << "new_page";
                    switch (hash(seed) % 3) {
                    default:
                        text << "whats_the_tallest_mountain_that_you_climbed";
                        break;
                    case 1:
                        text << "be_careful_during_climbing";
                        break;
                    case 2:
                        text << "arent_you_afraid_of_heights" << "!sweat";
                        break;
                    }
                }
            } else if (item.id == SpecialItemWhistle) {
                if (hash(seed++) & 1) {
                    text << "new_page";
                    switch (hash(seed) % 2) {
                    default:
                        text << "i_wonder_if_the_bird_statues_have_to_do_with_it";
                        break;
                    case 1:
                        text << "it_has_a_nice_sound";
                        break;
                    }
                }
            } else if (item.id == SpecialItemHarp) {
                if (hash(seed++) & 1) {
                    text << "new_page";
                    switch (hash(seed) % 2) {
                    default:
                        text << "it_looks_so_nice";
                        break;
                    case 1:
                        text << "it_has_a_nice_sound";
                        break;
                    }
                }
            } else if (item.id == SpecialItemReins) {
                if (hash(seed++) & 1) {
                    text << "new_page";
                    switch (hash(seed) % 5) {
                    default:
                        text << "whats_your_favorite_pet_to_ride";
                        break;
                    case 1:
                        text << "i_wish_i_could_ride";
                        break;
                    case 2:
                        text << "i_would_ride_through_the_land";
                        break;
                    case 3:
                        text << "how_fast_can_you_ride";
                        break;
                    case 4:
                        text << "where_will_you_ride_next";
                        break;
                    }
                }
            }
            return text;
        }
        if (m_world->m_localCreature->knowsMission(place[0], place[1])) {
            switch (hash(seed++) % 2) {
            default:
                text << "i_wish_you_good_luck_finding_the_item";
                if (hash(seed) & 1) {
                    text << "!love";
                }
                break;
            case 1:
                if (hash(seed++) & 1) {
                    text << "!question";
                }
                text << "havent_you_found_the_item_yet";
                text << "new_page";
                switch (hash(seed++) % 3) {
                default:
                    text << "it_should_be_in_the_location";
                    break;
                case 1:
                    if (hash(seed) & 1) {
                        text << "!angry";
                    }
                    text << "what_a_shame";
                    break;
                case 2:
                    text << "i_wish_you_good_luck_finding_it";
                    if (hash(seed) & 1) {
                        text << "!love";
                    }
                    break;
                }
                break;
            }
            return text;
        }
        creature->greet(seed, text);
        text << "new_page";
        const int race = randomRace(creature->m_entityData.race);
        text << "define_mob" << hash(seed++) << race;
        switch (hash(seed++) % 5) {
        default:
            if (hash(seed++) & 1) {
                text << "i_was_exploring_the_location";
            } else {
                text << "i_was_gathering_ingredients_in_the_location";
            }
            text << "new_page";
            if (hash(seed++) & 1) {
                text << "!alert";
            }
            text << "i_saw_an_item";
            if (hash(seed) & 1) {
                text << "new_page";
                text << "i_dont_need_it_and_left_it_there";
            }
            break;
        case 1:
            if (hash(seed++) & 1) {
                text << "!alert";
            }
            text << "i_saw_an_item_in_the_zone";
            if (hash(seed) & 1) {
                text << "new_page";
                text << "i_dont_need_it_and_left_it_there";
            }
            break;
        case 2:
            if (hash(seed++) & 1) {
                text << ((hash(seed++) & 1) ? "!question" : "!alert");
            }
            text << "do_you_need_an_item";
            text << "new_page";
            switch (hash(seed) % 3) {
            default:
                text << "i_found_one_in_the_location";
                break;
            case 1:
                text << "my_friend_found_one_in_the_location";
                break;
            case 2:
                text << "there_are_rumors_about_an_item_in_the_location";
                break;
            }
            break;
        case 3:
            if (hash(seed++) & 1) {
                if (hash(seed++) & 1) {
                    text << "!question";
                }
                text << "i_dont_know_if_its_true_but";
                text << "new_page";
            }
            switch (hash(seed) % 2) {
            default:
                text << "there_are_rumors_about_an_item_in_the_location";
                break;
            case 1:
                text << "my_friend_found_an_item_in_the_location";
                break;
            }
            break;
        case 4:
            text << "my_friend_is_an_adventurer";
            text << "new_page";
            text << "when_he_was_exploring_the_location_he_found_an_item";
            break;
        }
    } else {
        switch (kind) {
        case MissionKindExchangePartner:
            text << "define_item"
                 << fetchItem(location, place[0], place[1],
                              static_cast<unsigned char>(requestCount(place[0], place[1])), m_region);
            if (!m_world->m_localCreature->knowsMission(place[0], place[1])) {
                creature->greet(seed, text);
            }
            if (hash(seed) & 1) {
                text << "are_you_looking_for_items";
                if (hash(seed + 1) & 1) {
                    text << "!alert";
                }
                text << "new_page";
                text << "you_can_find_some_in_the_location";
            } else {
                text << "you_can_find_items_in_the_location";
            }
            break;
        case MissionKindMonster:
            text << "define_model" << monsterRace(place[0], place[1], location);
            if (m_world->m_localCreature->hasCompletedMission(place[0], place[1])) {
                switch (hash(seed) & 3) {
                case 0:
                    text << "!alert";
                    break;
                case 1:
                    text << "!question";
                    break;
                }
                text << "so_you_defeated_that_monster_in_the_location";
                text << "new_page";
                text << "nice";
                if (hash(seed + 1) & 1) {
                    text << "!love";
                }
            } else if (m_world->m_localCreature->knowsMission(place[0], place[1])) {
                text << "i_wish_you_good_luck_finding_that_monster_in_the_location";
                switch (hash(seed) & 3) {
                case 0:
                    text << "!love";
                    break;
                case 1:
                    text << "!sweat";
                    break;
                }
            } else {
                creature->greet(seed, text);
                switch (hash(seed) & 3) {
                case 0:
                    text << "!alert";
                    break;
                case 1:
                    text << "!sweat";
                    break;
                }
                text << "a_monster_was_sighted_in_the_location";
            }
            break;
        case MissionKindSupplierGnome:
            if (m_world->m_localCreature->hasCompletedMission(place[0], place[1])) {
                text << "so_you_already_helped_the_gnome_in_location";
                text << "new_page";
                text << "nice";
                if (hash(seed) & 1) {
                    text << "!love";
                }
            } else if (m_world->m_localCreature->knowsMission(place[0], place[1])) {
                switch (hash(seed) % 2) {
                default:
                    text << "i_wish_you_good_luck_rescuing_the_gnome_in_the_location";
                    if (hash(seed + 1) & 1) {
                        text << "!love";
                    }
                    break;
                case 1:
                    if (hash(seed + 1) & 1) {
                        text << "!question";
                    }
                    text << "havent_you_found_the_gnome_yet";
                    text << "new_page";
                    text << "he_should_be_in_the_location";
                    break;
                }
                return text;
            } else {
                creature->greet(seed, text);
                text << "new_page";
                switch (hash(seed++) % 2) {
                default:
                    if (hash(seed++) & 1) {
                        text << "i_was_exploring_the_location";
                    } else {
                        text << "i_was_gathering_ingredients_in_the_location";
                    }
                    text << "new_page";
                    if (hash(seed++) & 1) {
                        text << "!alert";
                    }
                    text << "i_heard_a_gnome_screaming";
                    break;
                case 1:
                    text << "a_supplier_gnome_went_missing_in_the_location";
                    break;
                }
                text << "new_page";
                text << "define_model";
                int race;
                switch (determineTopic(place[0], place[1])) {
                default:
                    race = RaceFamilyBandit;
                    break;
                case MissionOgreCamp:
                case MissionNecromancerCamp:
                    race = RaceOgre;
                    break;
                case MissionWitchCamp:
                    race = RaceWitch;
                    break;
                case MissionSkeletonCamp:
                    race = RaceSkeleton;
                    break;
                case MissionMonstrosityCamp:
                    race = RaceMonstrosity;
                    break;
                case MissionDarkCultistCamp:
                    race = RaceDarkCultistMale;
                    break;
                case MissionDemonMarauderCamp:
                    race = RaceDemonMarauder;
                    break;
                }
                text << race;
                text << "he_was_captured_by_creatures";
                switch (hash(seed) & 3) {
                case 0:
                    text << "!angry";
                    break;
                case 1:
                    text << "!sweat";
                    break;
                }
                if (hash(seed + 1) & 1) {
                    text << "new_page";
                    text << "i_hope_someone_is_going_to_help_him";
                }
            }
            break;
        }
    }
    text << "mark_location" << place[0] << place[1];

    return text;
}

}
