#include "cube/Shuffle.h"
#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
// See cube/World.h.
#include "cube/World.h"
#include "cube/WorldLayout.h"
#include "cube/Sight.h"
#include "cube/MathConstants.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "cube/Creature.h"
#include "cube/Missions.h"
#include "cube/Noise.h"
#include "cube/Realm.h"
#include "cube/Serializer.h"
#include "cube/Zone.h"
#include "gfx/ByteGrid.h"
#include "cube/CreatureIds.h"
#include "cube/BlockIds.h"
#include "cube/ItemIds.h"
#include "cube/ServerUpdateIds.h"
#include "mod/Hooks.h"
#include "cube/WorldIds.h"
#include "cube/ZoneIds.h"

namespace cube {

namespace {
// Times are in milliseconds, distances in blocks. applyBuffSkill: the buffs' durations and strengths.
const float BulwarkDuration = 10000.0f;
const float BulwarkStrength = 0.5f;
const float ManaShieldDuration = 30000.0f;
const float ManaShieldSpellPowerFactor = 4.0f; // the shield's strength per spell power
const float FlameRushDuration = 10000.0f;
const float FlameRushStrength = 2.0f;
const float SwiftnessDuration = 10000.0f;
const float SwiftnessStrength = 2.0f;
const float CamouflageDuration = 20000.0f;
const float NinjutsuDuration = 20000.0f;
const float WarFrenzyDuration = 10000.0f;
// applyHit: a hit costs the target this much stealth and wakes a sleeper stunned for this long; a stunned target is
// pushed a tenth as far.
const float HitStealthLoss = 0.25f;
const float WakeStunTime = 3000.0f;
const float StunnedKnockbackFactor = 0.1f;
// applyHit: the threat a hit adds: the damage times a factor by the attacker (ranged 1, a turtle 20, a guardian with a
// two-handed weapon 80, else 2), plus a bias; healing adds a tenth of the healing.
const float HitThreatBias = 0.01f;
const float GuardianHitThreatBias = 0.1f;
const float TurtleThreatFactor = 20.0f;
const float GuardianThreatFactor = 80.0f;
const float MeleeThreatFactor = 2.0f;
const float HealingThreatFactor = 0.1f;
// Creatures this far from every player are despawned.
const float DespawnDistance = 320.0f;
// A dropped item lands after 500..799 ms.
const int ItemDropTime = 500;
const int ItemDropTimeSpread = 300;
// dropLoot: one in this many: a carried item, an equipped pet, a weapon or armour, a consumable, leftovers, a dewdrop
// from a plant; dropReward: a gem per tier.
const int CarriedItemDropOdds = 16;
const int PetDropOdds = 50;
const int EquipmentDropOdds = 20;
const int ConsumableDropOdds = 8;
const int LeftoversDropOdds = 20;
const int DewdropOdds = 8;
const int RewardGemOdds = 8;
// dropReward: the coins are ten times the tier's prize scale (once or twice).
const float RewardCoinFactor = 10.0f;
// The dungeons lie this far under the lowest terrain of their room.
const int DungeonDepth = 40;
// The dungeons' storey heights by cell style.
const int WideDungeonStoreyHeight = 20;
const int DungeonStoreyHeight = 16;
// findRegion: a block within a place belongs to the place's region where the place's influence is at least this.
const float PlaceRegionInfluence = 0.25f;
// flightDuration: 2.5 ms per block, between 10 and 30 seconds.
const double FlightTimePerBlock = 2.5;
const int MinFlightTime = 10000;
const int MaxFlightTime = 30000;
// A biome's weight falls off with the squared distance beyond the nearest biome at this rate per block (a desert's
// more slowly).
const float BiomeBlendRate = 0.0004f;
const float DesertBlendRate = 0.0003f;
// A flight counts as far while more than 1000 blocks remain of a route of more than 16384.
const double FarFlightRemaining = 1000.0;
const double FarFlightLength = 16384.0;
// A land's centre lies at least this far inside it.
const int LandCenterMargin = 1024;
// onAttack: creatures within this range of the target that could fight the attacker start with this threat against it.
const float AllyAlertRange = 8.0f;
const float AllyAlertThreat = 0.5f;
// The world's start: the centre column of the land at 2^25 + 2^13 blocks.
const int WorldStartCoordinate = 0x2002000;
// A sitting or sleeping creature collides over this square footprint.
const float SittingFootprint = 0.8f;
// The colours fade into the spirit world's over the first three seconds of its minute-long buff.
const float SpiritworldDuration = 60000.0f;
const float SpiritworldFadeTime = 3000.0f;
// Local creatures (pets, summons) take IDs counting down from below this.
const long long LocalCreatureIdStart = (-9223372036854775807LL - 1) + 1000;
// updateTamed: a pet is dismissed when its owner glides for more than three seconds; it then waits five seconds.
const float PetGlideDismissTime = 3000.0f;
const float DismissedPetAbilityTime = 5000.0f;
// updateTime: the world's clock runs ten times as fast as real time (a hundred times while a player sleeps, healing 5%
// of the maximum HP per second); a world without a name shows 9 o'clock; a new world starts at 10 o'clock.
const float TimeScale = 10.0f;
const float RestingTimeScale = 100.0f;
const float RestRegenerationRate = 0.05f;
const int PreviewHour = 9;
const int StartHour = 10;
// init: the range of the world's seeded random values.
const int SeedRange = 100000;
// inspectCreatures: the smoothing rate of a drawn creature's position, the animation time it catches up with, and the
// aim's blend rates.
const float DrawnSmoothingRate = 0.02f;
const float SmoothingAnimationCatchUp = 32.0f;
const float PlayerAimBlendRate = 0.05f;
const float AimBlendRate = 0.01f;
// moveCreature: the speed limits in blocks a second: walking, a sprint's factor, the jump attack, large and tinted
// creatures, nearly still (while feeding a climbing plant or in a sweep or stomp); dashes and charges.
const float WalkSpeed = 6.0f;
const float SprintSpeedFactor = 1.3f;
const float JumpAttackSpeed = 9.0f;
const float LargeCreatureSpeed = 8.0f;
const float NearlyStillSpeed = 0.1f;
const float ChargeSpeed = 50.0f;
const float ChargePush = 20.0f;
const float DashSpeed = 60.0f;
const float DashPush = 20.0f;
const float ShellSpinRate = 720.0f; // degrees a second
const float ShellSpinSpeed = 12.0f;
// moveCreature: the speed through the medium: lava, poison, snow and foliage; while stepping up, and on paths.
const float LavaMediumSpeed = 0.35f;
const float PoisonMediumSpeed = 0.25f;
const float SnowMediumSpeed = 0.98f;
const float FoliageMediumSpeed = 0.85f;
const float SteppingSpeedFactor = 0.8f;
const float PathSpeedFactor = 1.1f;
// Movement speed factors for riding, fast movement, Swiftness, Flame Rush, slow, frozen and weak effects, and
// sprinting.
const float RidePetSpeedFactor = 1.5f;
const double FastFloatSpeedFactor = 1.5;
const double FastSpeedFactor = 2.0;
const float SwiftnessSpeedFactor = 2.0f;
const float FlameRushSpeedFactor = 1.3f;
const float SlowedSpeedFactor = 0.5f;
const float FrozenSpeedFactor = 0.5f;
const float WeakenedSpeedFactor = 0.1f;
const float SprintBoostSpeedFactor = 1.5f;
// moveCreature: Flame Rush's orb is five blocks across.
const float FlameRushOrbSize = 5.0f;
// moveCreature: a charging dash ends after half a second.
const float DashTimeLimit = 500.0f;
// moveCreature: the charge per millisecond and tempo (a fireball's slower) and War Frenzy's factor (a sniper's combo
// bonus cap 4 stays literal: std::min takes it by reference).
const float FireballChargeRate = 0.0005f;
const float ChargeRate = 0.00075f;
const float WarFrenzyChargeFactor = 10.0f;
// moveCreature: a retreat faces its aim after a second.
const float RetreatFacingDelay = 1000.0f;
// moveCreature: gravity, in blocks a second squared.
const float Gravity = 30.0f;
// updateTerritory: the zones within this many blocks of a player are kept populated; a resident counts the defeats in
// the zones this many around.
const int TerritoryRange = 256;
const int DefeatCountZoneRadius = 8;
// updateTerritory: a promoted resident turned hostile grows by a tenth.
const float PromotedSizeFactor = 1.1f;
// passInteractions: a pet follows its owner from beyond 200 blocks (squared); items are picked up within four blocks
// (squared); innkeepers rent rooms from 18 to 6 o'clock; golden gates within eight blocks (squared) open.
const float PetFollowDistanceSquared = 40000.0f;
const float PickupRangeSquared = 16.0f;
const int InnNightStartHour = 18;
const int InnNightEndHour = 6;
const float GoldenGateRangeSquared = 64.0f;
// passInteractions: a heroic shout reaches 50 blocks (squared): it angers the enemies of the allies there and
// regenerates the allies (5% for ten seconds).
const float HeroicShoutRangeSquared = 2500.0f;
const float HeroicShoutRegenDuration = 10000.0f;
const float HeroicShoutRegenStrength = 0.05f;
// attackCreature: a leech hit heals a tenth of its damage; an ambush poisons for three seconds with a fifth of its
// damage; a stunned target cannot be stunned again until three seconds after its stun; a rock fist lifts its target by
// 18 over its width.
const float LeechFraction = 0.1f;
const float AmbushPoisonDuration = 3000.0f;
const float AmbushPoisonStrength = 0.2f;
const float StunImmunityTime = 3000.0f;
const float RockFistLift = 18.0f;
// updateZonesState: the treasure spirit points to undiscovered markers within 400 blocks (squared); the zones three
// around each player are updated; an arena champion watches the zones 64 blocks around it.
const float TreasureSpiritRangeSquared = 160000.0f;
const int ActiveZoneRadius = 3;
const int ArenaChampionReach = 64;
// updateZonesState: an occupant leaves its furniture beyond four blocks (squared); activators within 16 blocks
// (squared) of a player in a dungeon count; a neighbour zone that is not loaded keeps an activator's residents from
// all being defeated.
const float FurnitureLeaveDistanceSquared = 16.0f;
const float ActivatorRangeSquared = 256.0f;
const int UnloadedResidentCount = 10000000;
// updateZonesState: traps reset after two seconds, trigger for players within 20 blocks (squared, Fixed), hit within
// five, pushing 20 out and 5 up for a tenth of the maximum HP (a stomp trap twice as far and five times as hard).
const float TrapResetTime = 2000.0f;
const int TrapTriggerRangeSquared = 400;
const int TrapHitRangeSquared = 25;
const float TrapPush = 20.0f;
const float TrapLift = 5.0f;
const float TrapDamageFraction = 0.1f;
const float StompTrapDamageFactor = 5.0f;
// performAbility: the elixirs' buffs last ten minutes and raise their value by a fifth; tameness lasts five seconds.
const float ElixirDuration = 600000.0f;
const float ElixirStrength = 1.2f;
const float TamenessDuration = 5000.0f;
// updateStepProjectiles: a poison projectile poisons for four seconds, the others weaken defence for 200 ms; a scout's
// shot gives Instant Shoot for one in seven (0.15) for 30 seconds; blasts reach five blocks (ten for fireballs and
// medicine).
const float ProjectilePoisonDuration = 4000.0f;
const float WeakenDuration = 200.0f;
const float InstantShootChance = 0.15f;
const float InstantShootDuration = 30000.0f;
const float BlastRadius = 5.0f;
const float LargeBlastRadius = 10.0f;
// updateSurvival: a bomb burns down at 0.025% of its maximum HP per ms; a water bubble loses 0.005% of its mana per ms
// and bursts below 0.1; the hit flash fades at this rate; poison ticks every 400 ms; the block power recovers over two
// seconds; climbing costs 0.025% stamina per ms; stealth grows at 0.005% per ms leaning forward and fades ten times as
// fast; drowning costs 5% of the maximum HP every half second.
const float BombFuseRate = 0.00025f;
const float WaterBubbleDrainRate = 0.00005f;
const float WaterBubbleMinMana = 0.1f;
const float HitFlashFadeRate = 0.0075f;
const int PoisonTickInterval = 400;
const float BlockPowerRecoveryTime = 2000.0f;
const float ClimbStaminaRate = 0.00025f;
const float StealthGainRate = 0.00005f;
const float StealthLossRate = 0.0005f;
const int DrowningInterval = 500;
const double DrowningDamageFraction = 0.05;
}

// The permutation of Ken Perlin's reference noise implementation; the noise functions index the doubled copy that
// installNoise makes.
static const unsigned char p[256] = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,  69,  142,
    8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203,
    117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136, 171, 168, 68,  175, 74,  165,
    71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,
    55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,
    18,  169, 200, 196, 135, 130, 116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250,
    124, 123, 5,   202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189,
    28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,
    129, 22,  39,  253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,
    242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,
    181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,  222, 114,
    67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180,
};

// 3t^2 - 2t^3: the smooth step between 0 and 1 for t in 0..1.
static inline float smoothStep(float t)
{
    return 3.0f * t * t - 2.0f * t * t * t;
}

// smoothStep made odd, for t in -1..1.
static inline float signedSmoothStep(float t)
{
    if (t < 0.0f) {
        float result = smoothStep(-t);
        return -result;
    }
    return smoothStep(t);
}

// Whether light passes through a block: air and water.
static inline bool isTransparent(const Block &block)
{
    return !block.isSolid();
}

// Whether a biome is one of the four forests.
static inline bool isForest(int biome)
{
    switch (biome) {
    case BiomeWoodlands:
    case BiomeJungle:
    case BiomeDarkWoods:
    case BiomeSnowForest:
        return true;
    }
    return false;
}

// One round of Marsaglia's 32-bit xorshift (13, 17, 5) as a hash of n. The colour functions use it with a counter as
// n.
static inline unsigned int xorshift(unsigned int n)
{
    n ^= n << 13;
    n ^= n >> 17;
    n ^= n << 5;
    return n;
}

// The largest integer not above value.
static inline int floorToInt(const float &value)
{
    if (value < 0.0f && value != (float)(int)value) {
        return (int)value - 1;
    }
    return (int)value;
}

// Abilities whose held state suppresses mana drain; indices are offsets from AbilityBlock.
enum ManaAbilityMembership : unsigned long long {
    ManaDrainSuppressedAbilities =
        (1ULL << (AbilityBlock - AbilityBlock)) | (1ULL << (AbilityChargedShot - AbilityBlock)) |
        (1ULL << (AbilitySalvo - AbilityBlock)) | (1ULL << (AbilityChargedBoomerang - AbilityBlock)) |
        (1ULL << (AbilityCrush - AbilityBlock)) | (1ULL << (AbilitySwirl - AbilityBlock)) |
        (1ULL << (AbilityMonsterCharge - AbilityBlock))
};

World::World(SpriteManager *spriteManager)
    : m_spriteManager(spriteManager),
      m_aiManager(this),
      m_spawnTable(this),
      m_isClient(false),
      m_clock(0),
      m_localCreature(nullptr)
{
    installNoise();
    m_petRaces.push_back(RaceBunny);
    m_petRaces.push_back(RaceMole);
    m_petRaces.push_back(RaceFly);
    m_petRaces.push_back(RaceCrow);
    m_petRaces.push_back(RaceSheep);
    m_petRaces.push_back(RaceCat);
    m_petRaces.push_back(RacePig);
    m_petRaces.push_back(RaceHorse);
    m_petRaces.push_back(RaceTurtle);
    m_petRaces.push_back(RaceHornet);
    m_petRaces.push_back(RacePeacock);
    m_petRaces.push_back(RaceBarkBeetle);
    m_petRaces.push_back(RaceSnoutBeetle);
    m_petRaces.push_back(RaceLemonBeetle);
    m_petRaces.push_back(RaceCollie);
    m_petRaces.push_back(RaceOwl);
    m_petRaces.push_back(RaceChicken);
    m_petRaces.push_back(RaceMonkey);
    m_petRaces.push_back(RaceScottishTerrier);
    m_petRaces.push_back(RaceCrocodile);
    m_petRaces.push_back(RaceTerrier);
    m_petRaces.push_back(RaceCamel);
    m_petRaces.push_back(RaceCrab);
    m_petRaces.push_back(RacePlainRunner);
    m_petRaces.push_back(RaceDesertRunner);
    m_petRaces.push_back(RaceLeafRunner);
    m_petRaces.push_back(RaceSnowRunner);
    m_petRaces.push_back(RaceDuckbill);
    m_petRaces.push_back(RacePorcupine);
    m_petRaces.push_back(RaceSeagull);
    m_petRaces.push_back(RaceParrot);
    m_petRaces.push_back(RaceBat);
    m_petRaces.push_back(RaceMosquito);
    m_petRaces.push_back(RaceBiter);
    m_petRaces.push_back(RaceBabyMammoth);
    m_petRaces.push_back(RaceBabyElephant);
    m_petRaces.push_back(RaceRadishCreatureSprout);
    m_petRaces.push_back(RacePlantCreatureSprout);
    m_petRaces.push_back(RaceBeaver);
    m_petRaces.push_back(RaceKoala);
    m_petRaces.push_back(RaceBumblebee);
    m_petRaces.push_back(RaceFireBeetle);
    m_petRaces.push_back(RaceFlamingo);
    m_petRaces.push_back(RaceCaterpillar);
    m_petRaces.push_back(RaceEarthCaterpillar);
    m_petRaces.push_back(RaceSnail);

    m_petFoodByRace[RaceFrightener] = {PetFoodBlueJelly, PetFoodGreenJelly, PetFoodPinkJelly, PetFoodYellowJelly};
    m_petFoodByRace[RaceZombie] = {PetFoodBlueJelly, PetFoodGreenJelly, PetFoodPinkJelly, PetFoodYellowJelly};
    m_petFoodByRace[RaceSkeleton] = {PetFoodBlueJelly, PetFoodGreenJelly, PetFoodPinkJelly, PetFoodYellowJelly};
    m_petFoodByRace[RaceSkeletonKnight] = {PetFoodBlueJelly, PetFoodGreenJelly, PetFoodPinkJelly, PetFoodYellowJelly};
    m_petFoodByRace[RaceIceling] = {PetFoodSoftice, PetFoodWaterIce, PetFoodMelonIceCream, PetFoodChocolateIceCream};
    m_petFoodByRace[RaceSnowGnobold] = {PetFoodSoftice, PetFoodWaterIce, PetFoodMelonIceCream,
                                        PetFoodChocolateIceCream};
    m_petFoodByRace[RacePolarGnoll] = {PetFoodSoftice, PetFoodWaterIce, PetFoodMelonIceCream, PetFoodChocolateIceCream};
    m_petFoodByRace[RaceWitch] = {PetFoodStrawberryCake, PetFoodChocolateCake, PetFoodChocolateCupcake,
                                  PetFoodVanillaCupcake};

    Item item;
    item.category = ItemCategoryMaterial;
    item.id = MaterialItemCube;
    item.material = ItemMaterialIron;
    m_ironCubeItems.push_back(item);
    item.material = ItemMaterialWood;
    m_woodAndPlantItems.push_back(item);
    item.id = MaterialItemDewdrop;
    item.material = ItemMaterialNone;
    m_woodAndPlantItems.push_back(item);
    item.id = MaterialItemFiber;
    item.material = ItemMaterialPlant;
    m_woodAndPlantItems.push_back(item);
    item.id = MaterialItemMushroom;
    item.material = ItemMaterialNone;
    m_woodAndPlantItems.push_back(item);
    item.id = MaterialItemShimmerMushroom;
    item.material = ItemMaterialNone;
    m_woodAndPlantItems.push_back(item);

    InitializeCriticalSectionAndSpinCount(&m_zoneMeshCriticalSection, 16);
    InitializeCriticalSectionAndSpinCount(&m_zonesLock, 16);
    InitializeCriticalSectionAndSpinCount(&m_unusedLock, 16);
}

World::~World()
{
    for (Creature *creature : m_creatures) {
        delete creature;
    }
    for (std::pair<const unsigned long long, Zone *> &entry : m_zones) {
        if (entry.second) {
            storeZone(entry.second);
            delete entry.second;
        }
    }
    DeleteCriticalSection(&m_zoneMeshCriticalSection);
    DeleteCriticalSection(&m_zonesLock);
    DeleteCriticalSection(&m_unusedLock);
}

void World::addCreature(Creature *creature)
{
    m_creatures.push_back(creature);
    m_creaturesById[creature->m_id] = creature;
}

// The ties are broken by hashes of the cell with salts of their own (4502227, 49842171), one in two.
void World::adjustOpenSides(int x, int y, int type, unsigned int *sides)
{
    int kind = buildingKind(type);
    if (type == BuildingHousePavilion || kind == BuildingKindHouse || kind == BuildingKindStonePavilion ||
        kind == BuildingKindCrypt || kind == BuildingKindTimberMineEntrance || kind == BuildingKindOldHut) {
        bool minusXOpen = (*sides & JoinedSideMinusX) != 0;
        bool plusXOpen = (*sides & JoinedSidePlusX) != 0;
        bool minusYOpen = (*sides & JoinedSideMinusY) != 0;
        bool plusYOpen = (*sides & JoinedSidePlusY) != 0;
        if (minusXOpen != plusXOpen) {
            if (minusYOpen != plusYOpen && (random(x + 4502227, y) & 1)) {
                minusYOpen = true;
                plusYOpen = true;
            } else {
                minusXOpen = true;
                plusXOpen = true;
            }
        }
        if (minusYOpen != plusYOpen) {
            minusYOpen = true;
            plusYOpen = true;
        }
        if (!minusXOpen && !plusXOpen && !minusYOpen && !plusYOpen) {
            if (random(x + 49842171, y) & 1) {
                minusXOpen = true;
                plusXOpen = true;
            } else {
                minusYOpen = true;
                plusYOpen = true;
            }
        }
        if (minusXOpen) {
            *sides |= JoinedSideMinusX;
        }
        if (plusXOpen) {
            *sides |= JoinedSidePlusX;
        }
        if (minusYOpen) {
            *sides |= JoinedSideMinusY;
        }
        if (plusYOpen) {
            *sides |= JoinedSidePlusY;
        }
    }
}

void World::applyBuff(Creature *creature, const Creature::Buff &buff, bool replace)
{
    creature->setBuff(buff, replace);
    ServerUpdates::BuffPacket packet;
    packet.sourceId = creature->m_id;
    packet.targetId = creature->m_id;
    packet.buff = buff;
    packet.replace = replace;
    m_serverUpdates.m_buffPacketList.push_back(packet);
}

void World::applyBuffSkill(Creature *creature, int skill)
{
    if (mod::applyBuffSkill(this, creature, skill)) {
        return;
    }
    Creature::Buff buff;
    buff.sourceId = creature->m_id;
    ServerUpdates::BuffPacket packet;
    switch (skill) {
    case AbilityBulwark:
        buff.type = BuffBulwark;
        buff.timeRemaining = BulwarkDuration;
        buff.strength = BulwarkStrength;
        creature->setBuff(buff, true);
        if (creature->m_entityData.stunTime > 0.0f) {
            creature->m_entityData.stunTime = 0.0f;
        }
        packet.sourceId = creature->m_id;
        packet.targetId = creature->m_id;
        packet.buff = buff;
        packet.replace = 1;
        m_serverUpdates.m_buffPacketList.push_back(packet);
        break;
    case AbilityManaShield:
        buff.type = BuffManashield;
        buff.timeRemaining = ManaShieldDuration;
        buff.strength = creature->spellPower(false) * ManaShieldSpellPowerFactor;
        creature->setBuff(buff, true);
        packet.sourceId = creature->m_id;
        packet.targetId = creature->m_id;
        packet.buff = buff;
        packet.replace = 1;
        m_serverUpdates.m_buffPacketList.push_back(packet);
        break;
    case AbilityFlameRush:
        buff.type = BuffFlamerush;
        buff.timeRemaining = FlameRushDuration;
        buff.strength = FlameRushStrength;
        creature->setBuff(buff, true);
        packet.sourceId = creature->m_id;
        packet.targetId = creature->m_id;
        packet.buff = buff;
        packet.replace = 1;
        m_serverUpdates.m_buffPacketList.push_back(packet);
        break;
    case AbilitySwiftness:
        buff.type = BuffSwiftness;
        buff.timeRemaining = SwiftnessDuration;
        buff.strength = SwiftnessStrength;
        creature->setBuff(buff, true);
        packet.sourceId = creature->m_id;
        packet.targetId = creature->m_id;
        packet.buff = buff;
        packet.replace = 1;
        m_serverUpdates.m_buffPacketList.push_back(packet);
        break;
    case AbilityCamouflage:
        buff.type = BuffCamouflage;
        buff.timeRemaining = CamouflageDuration;
        creature->setBuff(buff, true);
        packet.sourceId = creature->m_id;
        packet.targetId = creature->m_id;
        packet.buff = buff;
        m_serverUpdates.m_buffPacketList.push_back(packet);
        break;
    case AbilityNinjutsu:
        buff.type = BuffNinjutsu;
        buff.timeRemaining = NinjutsuDuration;
        creature->setBuff(buff, true);
        packet.sourceId = creature->m_id;
        packet.targetId = creature->m_id;
        packet.buff = buff;
        packet.replace = 1;
        m_serverUpdates.m_buffPacketList.push_back(packet);
        break;
    case AbilityWarFrenzy:
        buff.type = BuffWarfrenzy;
        buff.timeRemaining = WarFrenzyDuration;
        creature->setBuff(buff, true);
        packet.sourceId = creature->m_id;
        packet.targetId = creature->m_id;
        packet.buff = buff;
        packet.replace = 1;
        m_serverUpdates.m_buffPacketList.push_back(packet);
        break;
    }
    creature->m_skillCooldowns[skill] = (float)creature->cooldown(skill);
}

void World::applyEvents(const ServerUpdates &events)
{
    m_serverUpdates.m_soundPacketList = events.m_soundPacketList;
    m_serverUpdates.m_hitPacketList = events.m_hitPacketList;
    for (std::list<ServerUpdates::HitPacket>::const_iterator hit = events.m_hitPacketList.begin();
         hit != events.m_hitPacketList.end(); ++hit) {
        std::cout << hit->damage << " damage from " << hit->attackerId << " to " << hit->targetId << std::endl;
        applyHit(*hit);
        if (hit->damage > 0.0f) {
            Creature *target = creatureByID(hit->targetId);
            if (target) {
                target->m_hitFlash = 1.0f;
            }
        }
    }
    for (std::list<ServerUpdates::BuffPacket>::const_iterator buff = events.m_buffPacketList.begin();
         buff != events.m_buffPacketList.end(); ++buff) {
        if (creatureByID(buff->targetId)) {
            creatureByID(buff->targetId)->setBuff(buff->buff, false);
        }
        m_serverUpdates.m_buffPacketList.push_back(*buff);
    }
    for (const ServerUpdates::BulletPacket &bullet : events.m_bulletPacketList) {
        m_projectiles.push_back(bullet);
        m_serverUpdates.m_bulletPacketList.push_back(bullet);
    }
}

// A hit from applyEvents or the world's own fights: threat for the attacker in the target's table (m_aggro), the push,
// the stun, then the damage (or the healing, when it is negative), and when the target dies its sound, its loot, the
// kill packets and the removal of the target from every creature's table. hitType 1 and 3 do nothing, 4 is a dodge, 5
// is taken by the target's shield buffs.
void World::applyHit(const ServerUpdates::HitPacket &hit)
{
    Creature *target = creatureByID(hit.targetId);
    if (!target || target->m_entityData.hp <= 0.0f) {
        return;
    }
    if (!m_localCreature && target->m_entityData.hostilityType == HostilityPlayer) {
        return;
    }
    if (hit.hitType == HitTypeHeal) {
        return;
    }
    Creature *attacker = creatureByID(hit.attackerId);
    if (hit.damage > 0.0f) {
        target->m_entityData.stealth -= HitStealthLoss;
        if (target->m_entityData.stealth < 0.0f) {
            target->m_entityData.stealth = 0.0f;
        }
        if (target->m_entityData.currentAbility == AbilitySleep) {
            target->m_entityData.currentAbility = AbilityNone;
            target->m_entityData.stunTime = WakeStunTime;
        }
    }
    if (hit.damage >= 0.0f && attacker) {
        attacker->m_opponentId = hit.targetId;
        target->m_opponentId = hit.attackerId;
        // A hit by a creature that is neither a player (0) nor of type 6 counts a thousandth.
        float scale = 1.0f;
        if (attacker->m_entityData.hostilityType != HostilityPlayer &&
            attacker->m_entityData.hostilityType != HostilityPet) {
            scale = 0.001f;
        }
        attacker->m_hitCounts[hit.targetId] += 1.0f;
        if (attacker->isAttackingFromRange()) {
            target->m_aggro[hit.attackerId] += scale * hit.damage + HitThreatBias;
        } else if (attacker->m_entityData.race == RaceTurtle) {
            target->m_aggro[hit.attackerId] += hit.damage * TurtleThreatFactor * scale + HitThreatBias;
        } else if (attacker->m_entityData.equipment.weaponRight.isTwoHanded() &&
                   attacker->m_entityData.specialization == SpecializationGuardian) {
            target->m_aggro[hit.attackerId] += hit.damage * GuardianThreatFactor * scale + GuardianHitThreatBias;
        } else {
            target->m_aggro[hit.attackerId] += hit.damage * MeleeThreatFactor * scale + HitThreatBias;
        }
        onAttack(attacker, target);
    }
    if (target->m_entityData.appearance.appearanceFlags & AppearanceFlagMissionResident) {
        // Such a creature takes hits only once a player is among its attackers, and then only from players and type 6.
        bool attackedByPlayer = false;
        for (std::map<long long, float>::iterator it = target->m_aggro.begin(); it != target->m_aggro.end(); ++it) {
            if (it->second > 0.0f) {
                Creature *creature = creatureByID(it->first);
                if (creature && creature->m_entityData.hostilityType == HostilityPlayer) {
                    attackedByPlayer = true;
                    break;
                }
            }
        }
        if (!attackedByPlayer) {
            return;
        }
        if (!attacker) {
            return;
        }
        if (attacker->m_entityData.hostilityType != HostilityPlayer &&
            attacker->m_entityData.hostilityType != HostilityPet) {
            return;
        }
    }
    if (hit.hitType == HitTypeMiss) {
        return;
    }
    if (hit.damage >= 0.0f) {
        plasma::Vector<3, float> push = hit.direction;
        if (target->m_entityData.stunTime > 0.0f && hit.stunTime <= 0) {
            push[0] *= StunnedKnockbackFactor;
            push[1] *= StunnedKnockbackFactor;
            push[2] *= StunnedKnockbackFactor;
        }
        if (target->m_entityData.hostilityType != HostilityAttackableObject &&
            ((!m_isClient && target->m_entityData.hostilityType != HostilityPlayer) || target == m_localCreature) &&
            push[0] * push[0] + push[1] * push[1] + push[2] * push[2] >=
                target->m_entityData.retreat[0] * target->m_entityData.retreat[0] +
                    target->m_entityData.retreat[1] * target->m_entityData.retreat[1] +
                    target->m_entityData.retreat[2] * target->m_entityData.retreat[2]) {
            target->m_entityData.retreat = push;
        }
        if (hit.stunTime > 0) {
            target->m_entityData.velocity = plasma::Vector<3, float>(0, 0, 0);
        }
    }
    if (hit.actionLockTime > 0) {
        target->m_entityData.actionLockTime =
            (std::max)(target->m_entityData.actionLockTime, (float)hit.actionLockTime);
    }
    if (hit.hitType == HitTypeDodge) {
        target->applyDodgeBonus(m_serverUpdates);
        return;
    }
    if (hit.stunTime > 0) {
        target->m_entityData.stunTime = (std::max)(target->m_entityData.stunTime, (float)hit.stunTime);
    }
    if (m_isClient && target != m_localCreature) {
        return;
    }
    if (hit.damage > 0.0f) {
        if (hit.hitType == HitTypeMana) {
            // The hit is taken by the target's buffs of type 6 (a shield).
            for (std::list<Creature::Buff>::iterator buff = target->m_buffs.begin(); buff != target->m_buffs.end();
                 ++buff) {
                if (buff->type == BuffManashield) {
                    buff->strength -= hit.damage;
                    if (buff->strength <= 0.0f) {
                        buff->strength = 0.0f;
                        buff->timeRemaining = 0.0f;
                    }
                    ServerUpdates::BuffPacket packet;
                    packet.sourceId = hit.attackerId;
                    packet.targetId = hit.targetId;
                    packet.buff = *buff;
                    m_serverUpdates.m_buffPacketList.push_back(packet);
                }
            }
        } else if (target->canBeDamaged() && (!attacker || attacker->canBeDamaged())) {
            target->m_entityData.hp -= hit.damage;
        }
    } else if (hit.damage < 0.0f) {
        // Healing; the creatures that fight the target turn on the healer.
        target->m_entityData.hp -= hit.damage;
        if (target->m_entityData.hp > target->maxHP()) {
            target->m_entityData.hp = target->maxHP();
        }
        if (!m_isClient) {
            for (Creature *other : m_creatures) {
                // attacker is not tested here
                if (attacker->canDoDamage(other) && other->m_aggro.find(target->m_id) != other->m_aggro.end()) {
                    other->m_aggro[attacker->m_id] -= hit.damage * HealingThreatFactor;
                    onAttack(attacker, other);
                }
            }
        }
    }
    if (target->m_entityData.hp <= 0.0f) {
        target->m_entityData.hp = 0.0f;
        ServerUpdates::SoundPacket sound;
        sound.position = target->m_entityData.position;
        sound.speed = 1.0f;
        if (target->m_entityData.race == RaceWaterBubble) {
            sound.sound = SoundWatersplashHit;
        } else {
            sound.sound = SoundDestroy2;
            std::cout << "DIE SOUND" << std::endl;
        }
        m_serverUpdates.m_soundPacketList.push_back(sound);
        target->m_entityData.timeSinceAbility = 0.0f;
        dropLoot(target);
        if (target->m_entityData.binaryToggles.missionComplete) {
            dropReward(target->m_entityData.position,
                       (std::min)((int)ItemRarityYellow, target->m_entityData.level + 1));
        }
        long long ownerId = target->m_petId;
        if (ownerId != 0 && creatureByID(ownerId)) {
            creatureByID(ownerId)->m_entityData.hp = 0.0f;
        }
        std::list<Creature *> killers;
        for (std::map<long long, float>::iterator it = target->m_aggro.begin(); it != target->m_aggro.end(); ++it) {
            if (it->second > 0.0f && creatureByID(it->first) &&
                creatureByID(it->first)->m_entityData.hostilityType == HostilityPlayer) {
                killers.push_back(creatureByID(it->first));
                break;
            }
        }
        if (!m_isClient && target->m_groupMembers.size() > 1) {
            for (long long id : target->m_groupMembers) {
                m_state.m_deadCreatures.find(id); // the result is not used
            }
        }
        if (target->m_aggro.empty()) {
            ServerUpdates::KillPacket kill;
            kill.killerId = 0;
            kill.targetId = hit.targetId;
            m_serverUpdates.m_killPacketList.push_back(kill);
        } else {
            for (const std::pair<const long long, float> &entry : target->m_aggro) {
                ServerUpdates::KillPacket kill;
                kill.killerId = entry.first;
                kill.targetId = hit.targetId;
                m_serverUpdates.m_killPacketList.push_back(kill);
            }
        }
        // Every creature forgets the dead one and its owner.
        for (Creature *other : m_creatures) {
            if (other->m_opponentId == hit.targetId || other->m_opponentId == target->m_petId) {
                other->m_opponentId = 0;
            }
            std::list<std::map<long long, float>::iterator> stale;
            for (std::map<long long, float>::iterator entry = other->m_aggro.begin(); entry != other->m_aggro.end();
                 ++entry) {
                if (entry->first == hit.targetId || entry->first == target->m_petId) {
                    stale.push_back(entry);
                }
            }
            for (std::list<std::map<long long, float>::iterator>::iterator entry = stale.begin(); entry != stale.end();
                 ++entry) {
                other->m_aggro.erase(*entry);
            }
        }
    }
}

// A room's kind is drawn from a hash of the room (salt 444868, one of eight; two of the eight leave it plain), its
// flags one in two each (salts 702845 uneven floor, 18482581 pillars, 921518 raised walks).
Corridor basicCorridor(int x, int y, int direction, int startX, int startY, int type, int regionX, int regionY)
{
    if (isCorridor(x, y, direction, startX, startY, regionX, regionY)) {
        Corridor corridor;
        corridor.kind = Corridor_Room;
        corridor.flags = 0;
        bool wide = type == PlaceMines || type == PlaceGrotto;
        corridor.side = CorridorSide_Any;
        corridor.width = wide;
        unsigned char length = (unsigned char)dungeonWayLength(x, y, direction, startX, startY, regionX, regionY);
        if (length == 1) {
            corridor.kind = Corridor_FirstRoom;
        } else if (length == 2) {
            corridor.kind = Corridor_SecondRoom;
        } else {
            if (isHallNearEnd(x, y, direction, startX, startY, regionX, regionY)) {
                corridor.kind = Corridor_EndBossRoom;
            } else if (isHallEnd(x, y, direction, startX, startY, regionX, regionY)) {
                corridor.kind = Corridor_RewardRoom;
            } else if (abs(x - startX) > 0 || abs(y - startY) > 0) {
                switch (random(x + 444868, y) & 7) {
                case 0:
                    corridor.kind = Corridor_CrossPit;
                    break;
                case 1:
                    corridor.kind = Corridor_CrossWater;
                    break;
                case 2:
                    corridor.kind = Corridor_RimPit;
                    break;
                case 3:
                    corridor.kind = Corridor_RimLava;
                    break;
                case 4:
                    corridor.kind = Corridor_EnemyRoom;
                    break;
                case 5:
                    corridor.kind = Corridor_GuardianRoom;
                    break;
                }
                if (random(x + 702845, y) & 1) {
                    corridor.flags |= CorridorFlagUnevenFloor;
                }
            }
        }
        if (random(x + 18482581, y) & 1) {
            corridor.flags |= CorridorFlagPillars;
        }
        if (type < PlaceSewers || type > PlaceGrotto || type == PlaceMines) {
            if (random(x + 921518, y) & 1) {
                corridor.flags |= CorridorFlagRaisedWalks;
            }
        }
        corridor.length = length;
        return corridor;
    }
    return Corridor();
}

void World::beforeServerUpdates() {}

// The base elevation (a hand-tuned curve): 100 blocks inland, falling towards the ocean within a land's span (by a
// smoothstep of 3 - 4t); mountains add up to 200 (noise at frequency 0.0015, offsets 6667, 33322), fading over the
// distance at 0.0003 per block; detail noise adds up to 20 (0.001; offsets 247412, 8128321); canyons and hills lower
// or raise it by 50 blocks times their influence.
float World::BiomeSample::biomeElevation(int areaType, float areaInfluence)
{
    float oceanSquaredDistance = -1.0f;
    float mountainsSquaredDistance = -1.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (biomes[i][j] == BiomeOcean) {
                if (oceanSquaredDistance < 0.0f || oceanSquaredDistance > squaredDistances[i][j]) {
                    oceanSquaredDistance = squaredDistances[i][j];
                }
            } else if (biomes[i][j] == BiomeMountains) {
                if (mountainsSquaredDistance < 0.0f || mountainsSquaredDistance > squaredDistances[i][j]) {
                    mountainsSquaredDistance = squaredDistances[i][j];
                }
            }
        }
    }
    float result = 100.0f;
    if (oceanSquaredDistance >= 0.0f && oceanSquaredDistance >= nearestSquaredDistance) {
        float t = std::max(
            1.0f - (oceanSquaredDistance - nearestSquaredDistance) / (float)(BlocksPerLand * BlocksPerLand), 0.0f);
        result = signedSmoothStep(std::min(3.0f - t * 4.0f, 1.0f)) * 100.0f;
    }
    if (mountainsSquaredDistance >= 0.0f && mountainsSquaredDistance >= nearestSquaredDistance) {
        float t = std::max(1.0f - (mountainsSquaredDistance - nearestSquaredDistance) * 0.0003f * 0.0003f, 0.0f);
        result += (noise(x * 0.0015 + 6667.0, y * 0.0015 + 33322.0) + 1.0f) * (smoothStep(t) * 100.0f);
    }
    result += noise(x * 0.001 + 247412.0, y * 0.001 + 8128321.0) * 20.0f;
    if (areaType == PlaceCanyon) {
        result -= areaInfluence * 50.0f;
    } else if (areaType == PlaceHill) {
        result += areaInfluence * 50.0f;
    } else if (areaType != PlaceNone && areaInfluence > 0.0f && result < 1.0f) {
        result += smoothStep(std::min(areaInfluence + areaInfluence, 1.0f)) * (1.0f - result);
    }
    return result;
}

// The regions' border noise: noise at frequency 0.001 (offsets 8473, 9438 plus the world's offset) with a hashed
// jitter (noise of x * 65, y * 77, weight 0.05), lowered by 0.2 and raised by a tenth of the weight's complement; none
// above a weight of 0.75.
float World::borderNoise(int x, int y, float weight)
{
    if (weight > 0.75f) {
        return 0.0f;
    }
    return noise(x * 0.001 + 8473.0 + m_regionNoiseOffset[0], y * 0.001 + 9438.0 + m_regionNoiseOffset[1]) +
           noise(x * 65, y * 77) * 0.05f - 0.2f + (1.0f - weight) * 0.1f;
}

int World::boughtCount(const Item &item, long long shopId)
{
    if (!m_localCreature) {
        return 0;
    }
    PlayerData *player = m_localCreature->m_playerData;
    std::map<long long, std::list<Item>>::iterator it = player->purchasesByShop.find(shopId);
    if (it == player->purchasesByShop.end()) {
        return 0;
    }
    int count = 0;
    for (Item &bought : it->second) {
        count = bought == item ? count + 1 : count;
    }
    return count;
}

float World::cameraRay(const plasma::Vector<3, Fixed> &position, const plasma::Vector<3, float> &direction, float range,
                       bool throughSolid, bool stopAtBarriers, Creature *other)
{
    float distance = 0.0f;
    plasma::Vector<3, int> block = fixedToBlock(position);
    int steps = 0;
    Zone *zone = this->zone(floorDivide(block[0], BlocksPerZone), floorDivide(block[1], BlocksPerZone));
    while (range > distance) {
        if (!zone) {
            break;
        }
        int x = block[0];
        int y = block[1];
        int z = block[2];
        if (zone->getBlock(x, y, z).isSolid()) {
            break;
        }
        if (steps > RayMaxCrossings) {
            break;
        }
        steps++;
        plasma::Vector<3, Fixed> current = position + plasma::Vector<3, Fixed>(direction * distance);
        float step = RayStepBound;
        int axis = 0;
        for (int i = 0; i < 3; i++) {
            if (direction[i] * direction[i] < RayAxisEpsilon)
                continue;
            float t;
            if (direction[i] > 0.0f)
                t = (float)((Fixed(block[i] + 1) - current[i]) / direction[i]);
            else
                t = (float)((Fixed(block[i]) - current[i]) / direction[i]);
            if (t < step) {
                axis = i;
                step = t;
            }
        }
        if (direction[axis] > 0.0f) {
            block[axis]++;
        } else {
            block[axis]--;
        }
        plasma::Vector<2, int> zonePosition(floorDivide(block[0], BlocksPerZone), floorDivide(block[1], BlocksPerZone));
        if (zonePosition != zone->m_position) {
            zone = this->zone(zonePosition[0], zonePosition[1]);
        }
        distance += step;
    }
    return std::min(range, distance);
}

bool World::canSee(Creature *creature, Creature *other, float range)
{
    if (other->m_entityData.stealth > 0.0f) {
        range *= std::max(MinStealthRangeFactor, 1.0f - other->m_entityData.stealth);
    }
    return inRange(creature->m_entityData.position, other->m_entityData.position, false, range, other);
}

// Position the creature attached to this carrier: a held creature (128), or the occupant of a cage, seat or other
// furnishing (131, races 187/188). An occupant that accelerates (squared above 0.001) gets off.
void World::carryCreature(Creature *carrier, float milliseconds)
{
    if (carrier->m_entityData.currentAbility == AbilityCarryCreature) {
        Creature *held = creatureByID(carrier->m_entityData.ownerId);
        if (held && held->m_entityData.hp > 0.0f &&
            (held->isPassive() ||
             (held->m_entityData.hostilityType == HostilityPet && carrier->m_petId == held->m_id))) {
            held->m_entityData.binaryToggles.noTerrainCollision = 1;
            held->m_entityData.currentAbility = AbilityNone;
            held->m_drawnPosition =
                carrier->m_drawnPosition +
                plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f,
                                                                  carrier->m_entityData.appearance.size[2] * 0.5f +
                                                                      held->m_entityData.appearance.size[2] * 0.25f));
            held->m_drawnRotation = carrier->m_drawnRotation;
            held->m_entityData.position =
                carrier->m_entityData.position +
                plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f,
                                                                  carrier->m_entityData.appearance.size[2] * 0.5f +
                                                                      held->m_entityData.appearance.size[2] * 0.25f));
            held->m_entityData.rotation = carrier->m_entityData.rotation;
            held->m_stepOffset = carrier->m_stepOffset;
            if (!(held->m_entityData.appearance.appearanceFlags & AppearanceFlagFlying)) {
                held->m_walkAmplitude = 0.0f;
                held->m_walkPhase = 0.0f;
            }
            held->m_entityData.velocity = carrier->m_entityData.velocity;
            held->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        } else {
            carrier->m_entityData.focusId = 0;
            carrier->m_entityData.currentAbility = AbilityNone;
        }
        return;
    }
    if (carrier->m_entityData.race != RaceSleepingMat && carrier->m_entityData.race != RaceBeachTowel &&
        carrier->m_entityData.currentAbility != AbilityCarryRider)
        return;
    Creature *occupant = creatureByID(carrier->m_entityData.focusId);
    if (!occupant)
        return;
    if (carrier->m_entityData.currentAbility != AbilityCarryRider &&
        occupant->m_entityData.acceleration.squaredLength() >= 0.001f) {
        carrier->m_entityData.focusId = 0;
        return;
    }
    if (!(occupant->m_entityData.hp > 0.0f) || !(occupant->m_entityData.acceleration.squaredLength() < 0.001f ||
                                                 carrier->m_entityData.currentAbility == AbilityCarryRider))
        return;
    occupant->m_stepOffset = carrier->m_stepOffset;
    occupant->m_entityData.velocity = carrier->m_entityData.velocity;
    occupant->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    occupant->m_walkAmplitude = 0.0f;
    occupant->m_walkPhase = 0.0f;
    // Cages and leg irons hold the occupant sitting.
    if (carrier->isCage()) {
        occupant->m_entityData.position =
            carrier->m_entityData.position +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              0.03f - carrier->m_entityData.appearance.size[2] * 0.5f +
                                                                  occupant->m_entityData.appearance.size[2] * 0.5f));
        occupant->m_drawnPosition =
            carrier->m_drawnPosition +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              0.03f - carrier->m_entityData.appearance.size[2] * 0.5f +
                                                                  occupant->m_entityData.appearance.size[2] * 0.5f));
        occupant->m_entityData.currentAbility = AbilitySit;
    } else if (carrier->m_entityData.race == RaceTortureRack) {
        occupant->m_entityData.position =
            carrier->m_entityData.position +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              carrier->m_entityData.appearance.size[2] * 0.5f + 0.03f +
                                                                  occupant->m_entityData.appearance.size[2] * 0.25f));
        occupant->m_drawnPosition =
            carrier->m_drawnPosition +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              carrier->m_entityData.appearance.size[2] * 0.5f + 0.03f +
                                                                  occupant->m_entityData.appearance.size[2] * 0.25f));
        occupant->m_entityData.currentAbility = AbilityTortureRackPose;
        occupant->m_entityData.timeSinceAbility = 0.0f;
    } else if (carrier->m_entityData.race == RaceLegIrons) {
        occupant->m_entityData.position =
            carrier->m_entityData.position +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              0.03f - carrier->m_entityData.appearance.size[2] * 0.5f +
                                                                  occupant->m_entityData.appearance.size[2] * 0.5f));
        occupant->m_drawnPosition =
            carrier->m_drawnPosition +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              0.03f - carrier->m_entityData.appearance.size[2] * 0.5f +
                                                                  occupant->m_entityData.appearance.size[2] * 0.5f));
        occupant->m_entityData.currentAbility = AbilitySit;
    } else if (carrier->m_entityData.race == RacePole) {
        occupant->m_entityData.position =
            carrier->m_entityData.position +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              0.03f - carrier->m_entityData.appearance.size[2] * 0.5f +
                                                                  occupant->m_entityData.appearance.size[2] * 0.5f));
        occupant->m_drawnPosition =
            carrier->m_drawnPosition +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              0.03f - carrier->m_entityData.appearance.size[2] * 0.5f +
                                                                  occupant->m_entityData.appearance.size[2] * 0.5f));
        occupant->m_entityData.currentAbility = AbilityPolePose;
        occupant->m_entityData.timeSinceAbility = 0.0f;
    } else if (carrier->m_entityData.race == RaceCeilingChain) {
        occupant->m_entityData.position =
            carrier->m_entityData.position +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              0.03f - carrier->m_entityData.appearance.size[2] * 0.5f -
                                                                  occupant->m_entityData.appearance.size[2] * 0.2f));
        occupant->m_drawnPosition =
            carrier->m_drawnPosition +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              0.03f - carrier->m_entityData.appearance.size[2] * 0.5f -
                                                                  occupant->m_entityData.appearance.size[2] * 0.2f));
        occupant->m_entityData.currentAbility = AbilityCeilingChainPose;
    } else if (carrier->m_entityData.race == RaceBrazier || carrier->m_entityData.race == RaceGhostBrazier) {
        occupant->m_entityData.position =
            carrier->m_entityData.position +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              carrier->m_entityData.appearance.size[2] * 0.5f + 0.03f +
                                                                  occupant->m_entityData.appearance.size[2] * 0.5f));
        occupant->m_drawnPosition =
            carrier->m_drawnPosition +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              carrier->m_entityData.appearance.size[2] * 0.5f + 0.03f +
                                                                  occupant->m_entityData.appearance.size[2] * 0.5f));
        occupant->m_entityData.currentAbility = AbilityNone;
        occupant->m_entityData.binaryToggles.active = 1;
        return;
    } else if (carrier->m_entityData.race == RaceSocket) {
        // A socket turns what it holds.
        occupant->m_entityData.position =
            carrier->m_entityData.position +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              carrier->m_entityData.appearance.size[2] * 0.5f + 0.03f +
                                                                  occupant->m_entityData.appearance.size[2] * 0.5f));
        occupant->m_entityData.rotation[2] += milliseconds * 0.01f;
        occupant->m_drawnRotation = occupant->m_entityData.rotation;
        occupant->m_drawnPosition = occupant->m_entityData.position;
        occupant->m_entityData.currentAbility = AbilityNone;
        occupant->m_entityData.binaryToggles.active = 1;
        return;
    } else if (carrier->m_entityData.currentAbility == AbilityCarryRider) {
        occupant->m_entityData.position =
            carrier->m_entityData.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                 0.0f, 0.0f, carrier->m_entityData.appearance.size[2] * 0.5f));
        occupant->m_drawnPosition =
            carrier->m_drawnPosition + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                           0.03f, 0.03f, carrier->m_entityData.appearance.size[2] * 0.5f + 0.03f));
        occupant->m_entityData.currentAbility = AbilityNone;
    } else {
        occupant->m_entityData.position =
            carrier->m_entityData.position +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              carrier->m_entityData.appearance.size[2] * 0.5f + 0.03f +
                                                                  occupant->m_entityData.appearance.size[2] * 0.25f));
        occupant->m_drawnPosition =
            carrier->m_drawnPosition +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.03f, 0.03f,
                                                              carrier->m_entityData.appearance.size[2] * 0.5f + 0.03f +
                                                                  occupant->m_entityData.appearance.size[2] * 0.25f));
        occupant->m_entityData.currentAbility = AbilitySleep;
        occupant->m_drawnRotation = carrier->m_drawnRotation;
        occupant->m_entityData.rotation = carrier->m_entityData.rotation;
        return;
    }
    occupant->m_drawnRotation = carrier->m_drawnRotation;
    occupant->m_entityData.rotation = carrier->m_entityData.rotation;
    occupant->m_entityData.binaryToggles.active = 1;
}

int World::castleDirection(int x, int y)
{
    return random(x + 120149, y) & 3;
}

static inline int castleGate(int regionX, int regionY)
{
    switch (random(regionX + 0x55a05c, regionY) % 3) {
    case 1:
        return RoomPart_RuneGate;
    case 2:
        return RoomPart_LockedGate;
    default:
        return RoomPart_Gate;
    }
}

int World::castleBuildingPart(int x, int y, int floor)
{
    if (zoneBuildingType(x, y) == BuildingCastleTower) {
        if (floor <= 0 || modulo(x, 2) != 0 || modulo(y, 2) != 0)
            return RoomPart_None;
        int regionX = floorDivide(floorDivide(x * BlocksPerCell, BlocksPerPlace), 2) * 2;
        int regionY = floorDivide(floorDivide(y * BlocksPerCell, BlocksPerPlace), 2) * 2;
        if (initialFloors(regionX, regionY) != 3)
            return RoomPart_Activator;
        plasma::Vector<2, int> center = placeCenter(regionX, regionY, isLargePlace(regionX, regionY));
        int centerX = floorDivide(center[0], BlocksPerCell);
        int centerY = floorDivide(center[1], BlocksPerCell);
        switch (random(regionX + 0x100d7, regionY) & 3) {
        case 0:
            return x < centerX && y < centerY ? RoomPart_Activator : RoomPart_None;
        case 1:
            return x >= centerX && y < centerY ? RoomPart_Activator : RoomPart_None;
        case 2:
            return x >= centerX && y >= centerY ? RoomPart_Activator : RoomPart_None;
        case 3:
            return x < centerX && y >= centerY ? RoomPart_Activator : RoomPart_None;
        }
        return RoomPart_None;
    }
    if (!isOccupiedStorey(x, y, floor))
        return RoomPart_None;
    int regionX = floorDivide(floorDivide(x * BlocksPerCell, BlocksPerPlace), 2) * 2;
    int regionY = floorDivide(floorDivide(y * BlocksPerCell, BlocksPerPlace), 2) * 2;
    plasma::Vector<2, int> center = placeCenter(regionX, regionY, isLargePlace(regionX, regionY));
    int centerX = floorDivide(center[0], BlocksPerCell);
    int centerY = floorDivide(center[1], BlocksPerCell);
    int style = random(regionX + 0x1d555, regionY) & 3;
    int rotation = random(regionX + 0x5d932, regionY) & 3;
    int size = (random(regionX + 0xc0aa, regionY) & 3) + 15;
    int part = castlePart(x, y, style, rotation, size, centerX, centerY);
    if (part == 4) {
        if (x == centerX && y == centerY)
            return RoomPart_None;
        return (random(x + 0x4f690, y) & 7) != 0;
    }
    if (part == 1) {
        if (castlePart(x + 1, y, style, rotation, size, centerX, centerY) == 4 ||
            castlePart(x - 1, y, style, rotation, size, centerX, centerY) == 4 ||
            castlePart(x, y + 1, style, rotation, size, centerX, centerY) == 4 ||
            castlePart(x, y - 1, style, rotation, size, centerX, centerY) == 4)
            return RoomPart_MissionRoom;
    } else if (part == 2) {
        if (castlePart(x + 1, y, style, rotation, size, centerX, centerY) == 3 ||
            castlePart(x - 1, y, style, rotation, size, centerX, centerY) == 3 ||
            castlePart(x, y + 1, style, rotation, size, centerX, centerY) == 3 ||
            castlePart(x, y - 1, style, rotation, size, centerX, centerY) == 3) {
            return castleGate(regionX, regionY);
        }
    }
    if ((random(floorDivide(x, 3) + 0x559dbf, floorDivide(y, 3)) & 7) == 0)
        return RoomPart_RaisedWalk;
    if ((random(x + 0x4f690, y) & 7) == 1)
        return RoomPart_CentralPillar;
    if ((random(x + 0x9b61c, y) & 7) == 0 && part == 2 && castleGate(regionX, regionY) == RoomPart_Gate)
        return RoomPart_Activator;
    return RoomPart_None;
}

// The castle's plan (hand-authored): the gate band's position (salts 4499283, 5085256) three cells into a side, the
// approach (salt 201190) four to six cells in, an inner ring three to five cells in with a sparse five-cell pattern,
// and the court six cells in.
int World::castlePart(int x, int y, int direction, int rotation, int size, int centerX, int centerY)
{
    switch (rotation) {
    case CastleTransform_Clockwise: {
        int oldY = y;
        y = centerX - x + centerY;
        x = oldY - centerY + centerX;
        break;
    }
    case CastleTransform_AntiDiagonalReflection: {
        int oldY = y;
        y = centerX - x + centerY;
        x = centerX - oldY + centerY;
        break;
    }
    case CastleTransform_Counterclockwise: {
        int oldY = y;
        y = x - centerX + centerY;
        x = centerX - oldY + centerY;
        break;
    }
    }
    int minX = centerX - size / 2;
    int minY = centerY - size / 2;
    int maxX = minX + size;
    int maxY = minY + size;
    int cellX = floorDivide(floorDivide(x * BlocksPerCell, BlocksPerPlace), 2) * 2;
    int cellY = floorDivide(floorDivide(y * BlocksPerCell, BlocksPerPlace), 2) * 2;
    int result = 0;
    int offset = random(cellX + 4499283, cellY) % (size - 8) - size / 2 + 4;
    switch (random(cellX + 5085256, cellY) & 3) {
    case 1:
        if (x == centerX - offset && y >= maxY - 4 && y <= maxY - 2) {
            result = 2;
        }
        break;
    case 2:
        if (y == centerY - offset && x >= minX + 1 && x <= minX + 3) {
            result = 2;
        }
        break;
    case 3:
        if (y == centerY - offset && x >= maxX - 4 && x <= maxX - 2) {
            result = 2;
        }
        break;
    default:
        if (x == centerX - offset && y >= minY + 1 && y <= minY + 3) {
            result = 2;
        }
        break;
    }
    switch (random(cellX + 201190, cellY) & 3) {
    case 1:
        if (x == centerX && y >= maxY - 6 && y <= maxY - 5) {
            result = 1;
        }
        break;
    case 2:
        if (y == centerY && x >= minX + 4 && x <= minX + 5) {
            result = 1;
        }
        break;
    case 3:
        if (y == centerY && x >= maxX - 6 && x <= maxX - 5) {
            result = 1;
        }
        break;
    default:
        if (x == centerX && y >= minY + 4 && y <= minY + 5) {
            result = 1;
        }
        break;
    }
    if (x > minX + 1 && x < maxX - 2 && y > minY + 1 && y < maxY - 2) {
        if (x >= minX + 3 && x <= maxX - 4 && y >= minY + 3 && y <= maxY - 4 &&
            (x <= minX + 4 || y <= minY + 4 || x >= maxX - 5 || y >= maxY - 5)) {
            if (result == 0 && ((x + y + x * 2) % 5 + 5) % 5 == 0 && (random(x, y) & 1)) {
                return 0;
            }
            return 3;
        }
        if (x >= minX + 6 && x <= maxX - 7 && y >= minY + 6 && y <= maxY - 7) {
            return 4;
        }
        return result;
    }
    if (result == 0 && ((x + y + x * 2) % 5 + 5) % 5 == 0 && (random(x, y) & 1)) {
        return 0;
    }
    return 2;
}

int World::castleRotation(int x, int y)
{
    return random(x + 383282, y) & 3;
}

int World::castleSize(int x, int y)
{
    return (random(x + 49322, y) & 3) + 15;
}

int castleZoneType(int x, int y, int cellX, int cellY, int direction, int rotation, int size, int centerX, int centerY)
{
    switch (rotation) {
    case CastleTransform_Clockwise: {
        int oldY = y;
        y = centerX - x + centerY;
        x = oldY - centerY + centerX;
        break;
    }
    case CastleTransform_AntiDiagonalReflection: {
        int oldY = y;
        y = centerX - x + centerY;
        x = centerX - oldY + centerY;
        break;
    }
    case CastleTransform_Counterclockwise: {
        int oldY = y;
        y = x - centerX + centerY;
        x = centerX - oldY + centerY;
        break;
    }
    }
    int minX = centerX - size / 2;
    int minY = centerY - size / 2;
    int maxX = minX + size;
    int maxY = minY + size;
    if (x < minX || y < minY || x >= maxX || y >= maxY) {
        return BuildingNone;
    }
    if ((x < minX + 2 && (y < minY + 2 || y >= maxY - 2)) || (x >= maxX - 2 && (y >= maxY - 2 || y < minY + 2))) {
        return BuildingCastleTower;
    }
    if (x == minX && y == centerY) {
        return BuildingCastleGate;
    }
    return BuildingCastleCell;
}

// The caves' density: a ridged noise (frequencies 0.1 and 0.01, offsets 7007, 6000 and 1838, 88233, the fine one at a
// tenth's weight, scaled by 4) opened by a quarter-weight noise (43255, 6678), fading out in mountain and volcano
// places.
float World::caveDensity(int x, int y, int areaType, float areaInfluence)
{
    if ((areaType == PlaceMountain || areaType == PlaceVolcano) && areaInfluence > 0.5f) {
        return 0.0f;
    }
    double fy = y;
    double fx = x;
    double scaledY = fy * 0.01;
    double scaledX = fx * 0.01;
    float ridge = noise(fx * 0.1 + 7007.0, fy * 0.1 + 6000.0) * 0.1f;
    ridge += noise(scaledX + 1838.0, scaledY + 88233.0);
    ridge = (float)fabs(ridge) * 4.0f;
    float offset = noise(scaledX + 43255.0, scaledY + 6678.0) * 0.25f;
    float closed = offset + (1.0f - (std::min)(1.0f, ridge));
    closed = std::max(0.0f, closed);
    float open = 1.0f - std::min(1.0f, closed);
    float result = 1.0f - open * open * open;
    if ((areaType == PlaceMountain || areaType == PlaceVolcano) && areaInfluence > 0.0001f) {
        result *= std::max(1.0f - (areaInfluence + areaInfluence), 0.0f);
    }
    return result;
}

void World::changeLocalCreatureId(long long id)
{
    if (m_localCreature) {
        m_creaturesById.erase(m_localCreature->m_id);
        m_localCreature->m_id = id;
        if (m_creaturesById[id]) {
            delete m_creaturesById[id];
        }
        m_creaturesById[id] = m_localCreature;
    }
}

BlockProperties<float> World::columnBlock(int x, int y, int z, float cliffs, float temperature, float humidity,
                                          float firelands, float undead, float enchantedForest)
{
    if (cliffs > 0.5f) {
        return BlockProperties<float>(rockColor(x, y, z, temperature, humidity, firelands, undead), BlockBuilding);
    }
    return BlockProperties<float>(earthColor(x, y, z, temperature, humidity, firelands, undead, enchantedForest),
                                  BlockGround);
}

// Raises a light level to a neighbour's, which counts as 25 at least.
static inline void brighten(unsigned int &level, unsigned int neighbor)
{
    if (neighbor < 25)
        neighbor = 25;
    if (neighbor > level)
        level = neighbor;
}

// Every row of columns yields the processor once (Sleep(0)): the zone mesher runs on a thread of its own.
void computeLight(ZoneNeighborhood &zones, gfx::ByteGrid &light, int x0, int y0, int x1, int y1, int margin)
{
    gfx::ByteGrid spread(zones, x0, y0, x1, y1, margin);

    // Each column from the top: full light under the open sky; from the first solid block down, the smallest own light
    // of the solid blocks passed, raised again by a transparent block's own light. A block of type 20 gives its own
    // light and opens the sky again. A solid block gets the level that reaches it.
    for (int x = x0 - margin; x < x1 + margin; x++) {
        Sleep(0);
        for (int y = y0 - margin; y < y1 + margin; y++) {
            Field *field = zones.field(x, y);
            if (field) {
                bool sky = true;
                int level = 255;
                for (int i = (int)field->m_blocks.size() - 1; i >= 0; i--) {
                    Block block = field->getBlock(i);
                    if (block.type == BlockLightSource) {
                        sky = true;
                        level = block.lightLevel;
                        light.set(x, y, field->m_baseZ + i, (unsigned char)level);
                        spread.column(x, y).set(field->m_baseZ + i, (unsigned char)level);
                    } else if (!isTransparent(block)) {
                        sky = false;
                        light.set(x, y, field->m_baseZ + i, (unsigned char)level);
                        spread.column(x, y).set(field->m_baseZ + i, (unsigned char)level);
                        level = (std::min)(level, (int)block.lightLevel);
                    } else if (sky) {
                        level = 255;
                        light.set(x, y, field->m_baseZ + i, (unsigned char)level);
                        spread.column(x, y).set(field->m_baseZ + i, (unsigned char)level);
                    } else {
                        level = (std::max)(level, (int)block.lightLevel);
                        light.set(x, y, field->m_baseZ + i, (unsigned char)level);
                        spread.column(x, y).set(field->m_baseZ + i, (unsigned char)level);
                    }
                }
            }
        }
    }

    // The light spreads: every transparent block that is not fully lit takes 85 % of its brightest neighbour of the
    // six (25 at least), once per round, and the result becomes the next round's light. The margin bounds the rounds
    // (three fewer than its width).
    for (int round = 0; round < margin - 3; round++) {
        for (int x = x0 - margin + 1; x < x1 + margin - 1; x++) {
            Sleep(0);
            for (int y = y0 - margin + 1; y < y1 + margin - 1; y++) {
                Field *field = zones.field(x, y);
                if (field) {
                    for (int i = 0; i < (int)field->m_blocks.size(); i++) {
                        int z = field->m_baseZ + i;
                        Block block = field->getBlock(i);
                        if (isTransparent(block) && light.lookup(x, y, z) != 255) {
                            // Reads the height again from here on.
                            unsigned int level = 0;
                            brighten(level, light.lookup(x - 1, y, field->m_baseZ + i));
                            if (level < 255)
                                brighten(level, light.lookup(x + 1, y, field->m_baseZ + i));
                            if (level < 255)
                                brighten(level, light.lookup(x, y - 1, field->m_baseZ + i));
                            if (level < 255)
                                brighten(level, light.lookup(x, y + 1, field->m_baseZ + i));
                            if (level < 255)
                                brighten(level, light.lookup(x, y, field->m_baseZ + i - 1));
                            if (level < 255)
                                brighten(level, light.lookup(x, y, field->m_baseZ + i + 1));
                            spread.set(x, y, field->m_baseZ + i, (unsigned char)((int)level * 85 / 100));
                        }
                    }
                }
            }
        }
        for (int x = x0 - margin; x < x1 + margin; x++) {
            Sleep(0);
            for (int y = y0 - margin; y < y1 + margin; y++) {
                Field *field = zones.field(x, y);
                if (field) {
                    for (int i = 0; i < (int)field->m_blocks.size(); i++) {
                        Block block = field->getBlock(i);
                        if (isTransparent(block))
                            light.set(x, y, field->m_baseZ + i, spread.lookup(x, y, field->m_baseZ + i));
                    }
                }
            }
        }
    }

    // The transparent blocks of the rectangle itself keep their light in the first colour byte.
    for (int x = x0; x < x1; x++) {
        Sleep(0);
        for (int y = y0; y < y1; y++) {
            Field *field = zones.field(x, y);
            if (field) {
                for (int i = 0; i < (int)field->m_blocks.size(); i++) {
                    Block block = field->getBlock(i);
                    if (isTransparent(block)) {
                        block.color[0] = light.lookup(x, y, field->m_baseZ + i);
                        field->setBlock(i, block);
                    }
                }
            }
        }
    }
}

// The cover palette (hand-authored): the grass colour shifted by humidity (80 per unit from 0.5), blended to grey-blue
// undead tones (noise at 0.01 and 0.006, offsets 4343, 983; 30 and 128) and violet enclave tones, and to rock in the
// firelands (rock beyond 0.5).
BlockProperties<float> World::coverBlock(int x, int y, int z, float height, float temperature, float humidity,
                                         float sand, float fade, float path, float firelands, float undead,
                                         float enchantedForest)
{
    BlockProperties<float> block(plasma::Vector<3, float>(0.0f, 0.0f, 0.0f), BlockGrass);
    plasma::Vector<3, float> color;
    color = harvestGrassColor(x, y);
    float shift = (0.5f - humidity) * 80.0f;
    color[0] += shift;
    if (humidity > 0.5f) {
        color[2] -= shift;
    }
    if (undead > 0.001f) {
        float undeadNoiseShade = (noise(x * 0.01, y * 0.01) + 1.0f) * 30.0f;
        float undeadNoiseTint = (noise(x * 0.006 + 4343.0, y * 0.006 + 983.0) + 1.0f) * 30.0f;
        color = plasma::Vector<3, float>(undeadNoiseShade + 128.0f - undeadNoiseTint, 128.0f,
                                         std::max(undeadNoiseTint, undeadNoiseShade) + 128.0f) *
                    undead +
                color * (1.0f - undead);
    }
    if (enchantedForest > 0.001f) {
        float undeadNoiseShade = (noise(x * 0.01, y * 0.01) + 1.0f) * 20.0f;
        float undeadNoiseTint = (noise(x * 0.006 + 4343.0, y * 0.006 + 983.0) + 1.0f) * 100.0f;
        color = plasma::Vector<3, float>(undeadNoiseShade + 128.0f - undeadNoiseTint * 0.5f, 60.0f,
                                         undeadNoiseShade * 0.5f + 128.0f + undeadNoiseTint) *
                    enchantedForest +
                color * (1.0f - enchantedForest);
    }
    if (firelands > 0.001f) {
        color = rockColor(x, y, z, temperature, humidity, firelands, undead) * firelands + color * (1.0f - firelands);
        if (firelands > 0.5f) {
            block.type = BlockRock;
        }
    }
    block.color = color;
    return block;
}

// A glowing puff per fire or ice spirit of the item: particles of size 0.1, one plus three per unit of strength, ten
// more (at twice the acceleration) for a burst.
void World::createSpiritParticles(const plasma::Vector<3, Fixed> &position,
                                  const plasma::Vector<3, float> &acceleration, float strength, bool burst,
                                  const Item &item)
{
    for (int i = 0; i < item.spiritCount; ++i) {
        if (item.spirits[i].material >= ItemMaterialFireSpirit) {
            ServerUpdates::ParticlePacket packet;
            packet.position = position;
            packet.acceleration = acceleration;
            packet.size = 0.1f;
            packet.count = (int)(strength * 3.0f + 1.0f);
            packet.type = ParticleGlow;
            packet.color = materialColor(item.spirits[i].material, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f),
                                         strength * 0.5f);
            if (burst) {
                packet.count += 10;
                packet.acceleration *= 2.0f;
            }
            m_serverUpdates.m_particlePacketList.push_back(packet);
        }
    }
}

float World::BiomeSample::desertness()
{
    float distance = -1.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if ((biomes[i][j] == BiomeDesert || biomes[i][j] == BiomeFirelands) &&
                (distance < 0.0f || distance > squaredDistances[i][j])) {
                distance = squaredDistances[i][j];
            }
        }
    }
    return weight(distance, DesertBlendRate);
}

void World::despawnFarCreatures()
{
    std::list<Creature *> players;
    for (Creature *creature : m_creatures) {
        if (creature->m_entityData.hostilityType == HostilityPlayer) {
            players.push_back(creature);
        }
    }
    std::list<Creature *> farCreatures;
    for (Creature *creature : m_creatures) {
        if (creature->m_entityData.hostilityType != HostilityPlayer) {
            bool isFar = true;
            for (Creature *player : players) {
                isFar =
                    distanceExceeds2D(creature->m_entityData.position, player->m_entityData.position, DespawnDistance);
                if (!isFar) {
                    break;
                }
            }
            if (isFar) {
                farCreatures.push_back(creature);
            }
        }
    }
    for (Creature *creature : farCreatures) {
        removeCreature(creature);
    }
}

void World::destroyBlock(Fixed x, Fixed y, Fixed z)
{
    destroyBlock(x.toBlock(), y.toBlock(), z.toBlock());
}

bool distanceExceeds2D(plasma::Vector<3, Fixed> a, plasma::Vector<3, Fixed> b, float distance)
{
    if (fabs((double)(a - b)[0]) > distance || fabs((double)(a - b)[1]) > distance)
        return true;
    plasma::Vector<3, Fixed> difference = a - b;
    plasma::Vector<2, double> d((double)difference[0], (double)difference[1]);
    return d.squaredLength() > distance * distance;
}

void World::dropItem(const Item &source, const plasma::Vector<3, Fixed> &position, float angle, float unused,
                     int quantity, Creature *creature, long long ownerId)
{
    Item item(source);
    ZoneItem zoneItem;
    zoneItem.position = position;
    zoneItem.rotation = angle;
    zoneItem.dropTime = (float)(rand() % ItemDropTimeSpread + ItemDropTime);
    zoneItem.dropDay = m_state.m_day;
    if (creature) {
        zoneItem.destination = creature->m_entityData.destination;
    }
    if (item.hasRegion() && creature && creature->m_entityData.hostilityType != HostilityPlayer) {
        // The item of a creature belongs to the region of its place when the creature stands on a block with
        // BlockFlagDungeon, else to that of the creature's home position.
        int x;
        int y;
        if (getBlockAtDot(creature->m_entityData.position[0], creature->m_entityData.position[1],
                          creature->m_entityData.position[2])
                .flags &
            BlockFlagDungeon) {
            int cellY = floorDivide((int)creature->m_entityData.position[1], BlocksPerPlace);
            int cellX = floorDivide((int)creature->m_entityData.position[0], BlocksPerPlace);
            plasma::Vector<2, int> center = placeCenter(cellX, cellY, isLargePlace(cellX, cellY));
            x = center[0];
            y = center[1];
        } else {
            y = (int)creature->m_entityData.origin[1];
            x = (int)creature->m_entityData.origin[0];
        }
        item.region = findRegion(x, y);
    }
    zoneItem.quantity = quantity;
    zoneItem.flags = 0;
    if (item.category == ItemCategoryArtifact || item.category == ItemCategorySpecial)
        zoneItem.flags = ZoneItemFlagPerPlayer;
    if ((!creature || creature->m_entityData.hostilityType != HostilityPlayer) &&
        (item.category == ItemCategoryFormula || item.category == ItemCategoryWeapon ||
         item.category == ItemCategoryChest || item.category == ItemCategoryHands ||
         item.category == ItemCategoryFeet || item.category == ItemCategoryShoulder ||
         item.category == ItemCategoryNeck || item.category == ItemCategoryRing || item.category == ItemCategoryCoin ||
         item.category == ItemCategoryManaCube || item.category == ItemCategoryLeftovers ||
         item.category == ItemCategoryQuest || item.category == ItemCategoryEyePatch ||
         (item.category == ItemCategoryConsumable && item.id == ConsumablePortalStone))) {
        // Equipment and the like is dropped for one player: one of those the creature fought (or, without such a
        // creature, of those within 256 blocks) whose class can use it, else any player.
        std::vector<Creature *> candidates;
        if (creature && creature->m_entityData.hostilityType == HostilityEnemy) {
            for (std::map<long long, float>::iterator it = creature->m_aggro.begin(); it != creature->m_aggro.end();
                 ++it) {
                Creature *candidate = creatureByID(it->first);
                if (candidate && candidate->m_entityData.hostilityType == HostilityPlayer &&
                    item.fitsClass(candidate->m_entityData.classType)) {
                    candidates.push_back(candidate);
                }
            }
        } else {
            for (Creature *candidate : m_creatures) {
                if (candidate->m_entityData.hostilityType == HostilityPlayer &&
                    item.fitsClass(candidate->m_entityData.classType)) {
                    if (plasma::Vector<3, float>(position - candidate->m_entityData.position).squaredLength() <
                        65536.0f) {
                        candidates.push_back(candidate);
                    }
                }
            }
        }
        if (candidates.empty()) {
            for (Creature *candidate : m_creatures) {
                if (candidate->m_entityData.hostilityType == HostilityPlayer) {
                    candidates.push_back(candidate);
                }
            }
        }
        if (candidates.empty()) {
            return;
        }
        Creature *owner = candidates[rand() % candidates.size()];
        if (item.fitsClass(owner->m_entityData.classType)) {
            if ((item.category >= ItemCategoryWeapon && item.category <= ItemCategoryRing) ||
                item.category == ItemCategoryEyePatch) {
                // One time in twenty the rarity rises, up to four times in a row, to at most 4.
                if (rand() % 20 == 0) {
                    item.rarity++;
                    if (rand() % 20 == 0) {
                        item.rarity++;
                        if (rand() % 20 == 0) {
                            item.rarity++;
                            if (rand() % 20 == 0) {
                                item.rarity++;
                            }
                        }
                    }
                }
                item.rarity = (std::min)(4, (int)item.rarity);
            }
            zoneItem.ownerSeed = ownerId != 0 ? ownerId : owner->m_entityData.seed;
        }
    }
    int zoneY = floorDivide(zoneItem.position[1].toBlock(), BlocksPerZone);
    int zoneX = floorDivide(zoneItem.position[0].toBlock(), BlocksPerZone);
    Zone *zone = this->zone(zoneX, zoneY);
    if (zone) {
        zoneItem.item = item;
        zoneItem.rotation = (float)rand() * 360.0f / (float)RAND_MAX;
        // The item comes to lie on the first block below it that is neither air nor water.
        int z = zoneItem.position[2].toBlock();
        while (!getBlock(zoneItem.position[0].toBlock(), zoneItem.position[1].toBlock(), z).isSolid()) {
            z--;
        }
        zoneItem.position[2] = Fixed(z + 1);
        zone->m_items.push_back(zoneItem);
        m_zonesWithChangedItems.insert(zone);
    }
}

// The drops below land up to a block away from the creature or position along x and y, turned by a random angle. What
// a creature drops when it dies: a piece of its equipment, its pet, a random weapon or piece of armour, a consumable
// or pet food, coins by its level, one to three ingredients by its race and, for the hostility types 1 and 7, what it
// carries.
void World::dropLoot(Creature *creature)
{
    if (creature->m_entityData.hostilityType == HostilityPlayer ||
        creature->m_entityData.hostilityType == HostilityFriendlyNpc ||
        (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagSummoned)) {
        return;
    }
    // One piece of what the creature wears, one time in sixteen (always with AppearanceFlagTinted).
    std::vector<Item *> drops;
    if (creature->m_entityData.equipment.weaponLeft.category != ItemCategoryNone &&
        creature->m_entityData.equipment.weaponLeft.id != WeaponHandAttacks &&
        creature->m_entityData.equipment.weaponLeft.id != WeaponBiteAttack) {
        drops.push_back(&creature->m_entityData.equipment.weaponLeft);
    }
    if (creature->m_entityData.equipment.weaponRight.category != ItemCategoryNone &&
        creature->m_entityData.equipment.weaponRight.id != WeaponHandAttacks &&
        creature->m_entityData.equipment.weaponRight.id != WeaponBiteAttack) {
        drops.push_back(&creature->m_entityData.equipment.weaponRight);
    }
    if (creature->m_entityData.equipment.chest.category != ItemCategoryNone &&
        creature->m_entityData.equipment.chest.id != ChestBackpack) {
        drops.push_back(&creature->m_entityData.equipment.chest);
    }
    if (creature->m_entityData.equipment.shoulder.category != ItemCategoryNone) {
        drops.push_back(&creature->m_entityData.equipment.shoulder);
    }
    if (creature->m_entityData.equipment.hands.category != ItemCategoryNone) {
        drops.push_back(&creature->m_entityData.equipment.hands);
    }
    if (creature->m_entityData.equipment.feet.category != ItemCategoryNone) {
        drops.push_back(&creature->m_entityData.equipment.feet);
    }
    if (creature->m_entityData.equipment.neck.category != ItemCategoryNone) {
        drops.push_back(&creature->m_entityData.equipment.neck);
    }
    if (!drops.empty() && (rand() % CarriedItemDropOdds == 0 ||
                           (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagTinted))) {
        Item item(*drops[rand() % drops.size()]);
        dropItem(item,
                 creature->m_entityData.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                       1.0f - (float)rand() * 2.0f / (float)RAND_MAX,
                                                       1.0f - (float)rand() * 2.0f / (float)RAND_MAX, 0.0f)),
                 (float)rand() * 360.0f / (float)RAND_MAX, 1.0f, 1, creature, 0);
    }
    if (creature->m_entityData.equipment.pet.category != ItemCategoryNone && rand() % PetDropOdds == 0) {
        dropItem(creature->m_entityData.equipment.pet,
                 creature->m_entityData.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                       1.0f - (float)rand() * 2.0f / (float)RAND_MAX,
                                                       1.0f - (float)rand() * 2.0f / (float)RAND_MAX, 0.0f)),
                 (float)rand() * 360.0f / (float)RAND_MAX, 1.0f, 1, creature, 0);
    }
    // Only hostile creatures (types 1 and 7) drop equipment, pet food, their special drop and coins.
    if (creature->m_entityData.hostilityType == HostilityEnemy ||
        creature->m_entityData.hostilityType == HostilityAttackableObject) {
        if ((creature->m_entityData.hostilityType == HostilityEnemy &&
             (rand() % EquipmentDropOdds == 0 ||
              (creature->m_entityData.appearance.appearanceFlags & (AppearanceFlagTinted | AppearanceFlagLarge)))) ||
            (creature->m_entityData.hostilityType == HostilityAttackableObject && rand() % EquipmentDropOdds == 0)) {
            Item item;
            switch (rand() % 2) {
            case 0:
                item = randomWeapon(creature->m_entityData.level, -1, false);
                break;
            case 1:
                item = randomArmor(creature->m_entityData.level, -1, false);
                break;
            }
            if (item.rarity < ItemRarityYellow &&
                (creature->m_entityData.appearance.appearanceFlags & (AppearanceFlagTinted | AppearanceFlagLarge))) {
                item.rarity++;
            }
            dropItem(item,
                     creature->m_entityData.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                           1.0f - (float)rand() * 2.0f / (float)RAND_MAX,
                                                           1.0f - (float)rand() * 2.0f / (float)RAND_MAX, 0.0f)),
                     (float)rand() * 360.0f / (float)RAND_MAX, 1.0f, 1, creature, 0);
        }
        if (rand() % ConsumableDropOdds == 0) {
            Item item;
            switch (rand() % 5) {
            case 0:
                item = randomPetFood(0);
                break;
            case 1:
                item.category = ItemCategoryMaterial;
                item.id = MaterialItemFlask;
                item.material = ItemMaterialGlass;
                break;
            case 2:
                item.category = ItemCategoryConsumable;
                item.id = ConsumableGinsengSoup;
                break;
            case 3:
                item.category = ItemCategoryConsumable;
                item.id = ConsumableSnowBerryMash;
                break;
            case 4:
                item.category = ItemCategoryConsumable;
                item.id = ConsumableBomb;
                break;
            }
            dropItem(item,
                     creature->m_entityData.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                           1.0f - (float)rand() * 2.0f / (float)RAND_MAX,
                                                           1.0f - (float)rand() * 2.0f / (float)RAND_MAX, 0.0f)),
                     (float)rand() * 360.0f / (float)RAND_MAX, 1.0f, 1, creature, 0);
        }
        if ((creature->m_entityData.hostilityType == HostilityEnemy &&
             (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagHandAttacks)) ||
            (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagBiteAttack)) {
            if (rand() % LeftoversDropOdds == 0) {
                Item item;
                item.category = ItemCategoryLeftovers;
                item.rarity = (std::max)(1, creature->m_entityData.level);
                item.modifier = rand();
                dropItem(item,
                         creature->m_entityData.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                               1.0f - (float)rand() * 2.0f / (float)RAND_MAX,
                                                               1.0f - (float)rand() * 2.0f / (float)RAND_MAX, 0.0f)),
                         (float)rand() * 360.0f / (float)RAND_MAX, 0.75f, 1, creature, 0);
            }
        }
        if (creature->m_entityData.hostilityType == HostilityEnemy) {
            // Coins by the creature's level, ten and twenty times as many with AppearanceFlagTinted and
            // AppearanceFlagLarge.
            float coins = prizeScale((float)creature->m_entityData.level) * ((float)rand() / (float)RAND_MAX + 1.0f);
            if (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagTinted) {
                coins *= 10.0f;
            }
            if (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagLarge) {
                coins *= 20.0f;
            }
            for (int i = 0; (float)i < coins; i++) {
                Item item;
                item.category = ItemCategoryCoin;
                item.id = CoinCoin;
                item.material = ItemMaterialGold;
                dropItem(item,
                         creature->m_entityData.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                               1.0f - (float)rand() * 2.0f / (float)RAND_MAX,
                                                               1.0f - (float)rand() * 2.0f / (float)RAND_MAX, 0.0f)),
                         (float)rand() * 360.0f / (float)RAND_MAX, 0.75f, 1, creature, 0);
            }
        }
    }
    // One to three ingredients by the creature's race.
    int count = rand() % 3 + 1;
    for (int i = 0; i < count; i++) {
        Item item;
        float dropScalar = 0.75f;
        switch (creature->m_entityData.race) {
        case RaceIronDeposit:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemNugget;
            item.material = ItemMaterialIron;
            dropScalar = 3.0f;
            break;
        case RaceSilverDeposit:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemNugget;
            item.material = ItemMaterialSilver;
            dropScalar = 3.0f;
            break;
        case RaceGoldDeposit:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemNugget;
            item.material = ItemMaterialGold;
            dropScalar = 3.0f;
            break;
        case RaceEmeraldDeposit:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemEmerald;
            item.rarity = ItemRarityGreen;
            dropScalar = 3.0f;
            break;
        case RaceSapphireDeposit:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemSapphire;
            item.rarity = ItemRarityBlue;
            dropScalar = 3.0f;
            break;
        case RaceRubyDeposit:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemRuby;
            item.rarity = ItemRarityPurple;
            dropScalar = 3.0f;
            break;
        case RaceDiamondDeposit:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemDiamond;
            item.rarity = ItemRarityYellow;
            dropScalar = 3.0f;
            break;
        case RaceSandstoneDeposit:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemNugget;
            item.material = ItemMaterialSandstone;
            dropScalar = 0.8f;
            break;
        case RaceIceCrystalDeposit:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemCrystal;
            item.material = ItemMaterialIce;
            dropScalar = 0.8f;
            break;
        case RaceCactus:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemPricklyPear;
            dropScalar = 0.8f;
            break;
        case RaceBush:
        case RaceSnowBush:
        case RaceScrub:
        case RaceFireScrub:
            if (rand() % 2 != 0) {
                item.category = ItemCategoryMaterial;
                item.id = MaterialItemLog;
                item.material = ItemMaterialWood;
            } else {
                item.category = ItemCategoryMaterial;
                item.id = MaterialItemFiber;
                item.material = ItemMaterialPlant;
            }
            dropScalar = 0.8f;
            break;
        case RaceSnowBerryBush:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemSnowBerry;
            item.material = ItemMaterialNone;
            dropScalar = 0.8f;
            break;
        case RaceCobwebScrub:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemCobweb;
            item.material = ItemMaterialNone;
            dropScalar = 0.8f;
            break;
        case RaceCottonPlant:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemCapsule;
            item.material = ItemMaterialCotton;
            dropScalar = 0.8f;
            break;
        case RaceRadishCreature:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemRadishSlice;
            dropScalar = 0.8f;
            break;
        case RaceOnionling:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemOnionSlice;
            dropScalar = 0.8f;
            break;
        case RaceGinseng:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemGinsengRoot;
            dropScalar = 0.8f;
            break;
        case RaceParrot:
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemFeather;
            item.material = ItemMaterialParrot;
            dropScalar = 0.8f;
            break;
        }
        if (rand() % DewdropOdds == 0 && creature->m_entityData.race >= RaceBush &&
            creature->m_entityData.race <= RaceGinseng && creature->m_entityData.race != RaceFireScrub) {
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemDewdrop;
            item.material = ItemMaterialNone;
            item.rarity = ItemRarityWhite;
        } else if (item.category == ItemCategoryNone) {
            break;
        }
        dropItem(item,
                 creature->m_entityData.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                       1.0f - (float)rand() * 2.0f / (float)RAND_MAX,
                                                       1.0f - (float)rand() * 2.0f / (float)RAND_MAX, 0.0f)),
                 (float)rand() * 360.0f / (float)RAND_MAX, dropScalar, 1, creature, 0);
    }
    if (creature->m_entityData.hostilityType == HostilityEnemy ||
        creature->m_entityData.hostilityType == HostilityAttackableObject) {
        // Everything the creature carries, except items of category 1 with id 1.
        if (!creature->m_inventory.m_inventoryTabs.empty()) {
            for (int tab = 0; tab < (int)creature->m_inventory.m_inventoryTabs.size(); tab++) {
                for (std::vector<ItemStack>::iterator stack = creature->m_inventory.m_inventoryTabs[tab].begin();
                     stack != creature->m_inventory.m_inventoryTabs[tab].end(); ++stack) {
                    if (stack->item.category != ItemCategoryNone && stack->quantity != 0) {
                        for (int i = 0; i < stack->quantity; i++) {
                            if (stack->item.category != ItemCategoryConsumable ||
                                stack->item.id != ConsumableLifePotion) {
                                dropItem(stack->item,
                                         creature->m_entityData.position +
                                             plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                 1.0f - (float)rand() * 2.0f / (float)RAND_MAX,
                                                 1.0f - (float)rand() * 2.0f / (float)RAND_MAX, 0.0f)),
                                         (float)rand() * 360.0f / (float)RAND_MAX, 0.75f, 1, creature, 0);
                            }
                        }
                    }
                }
            }
        }
    }
}

// The reward at a position for a tier: a weapon or a piece of armour of the tier for every player within 100 blocks
// (owned by that player), coins, and one time in eight per tier an ingredient 32.. of that tier.
void World::dropReward(const plasma::Vector<3, Fixed> &position, int tier)
{
    plasma::Vector<2, int> region = findRegion((int)position[0], (int)position[1]);
    // A weapon or a piece of armour for every player within 100 blocks.
    for (Creature *creature : m_creatures) {
        if (creature->m_entityData.hostilityType == HostilityPlayer) {
            if (plasma::Vector<3, float>(creature->m_entityData.position - position).squaredLength() < 10000.0f) {
                Item item = rand() % 2 != 0 ? randomWeapon(tier, creature->m_entityData.classType, false)
                                            : randomArmor(tier, creature->m_entityData.classType, false);
                item.region = region;
                dropItem(item,
                         position + plasma::Vector<3, Fixed>(
                                        plasma::Vector<3, float>(1.0f - (float)rand() * 2.0f / (float)RAND_MAX,
                                                                 1.0f - (float)rand() * 2.0f / (float)RAND_MAX, 0.0f)),
                         (float)rand() * 360.0f / (float)RAND_MAX, 1.0f, 1, nullptr, creature->m_entityData.seed);
            }
        }
    }
    int coins = (int)(((float)rand() / (float)RAND_MAX + 1.0f) * (prizeScale((float)tier) * RewardCoinFactor));
    for (int i = 0; i < coins; i++) {
        Item item;
        item.category = ItemCategoryCoin;
        item.id = CoinCoin;
        item.material = ItemMaterialGold;
        dropItem(item,
                 position + plasma::Vector<3, Fixed>(
                                plasma::Vector<3, float>(1.0f - (float)rand() * 2.0f / (float)RAND_MAX,
                                                         1.0f - (float)rand() * 2.0f / (float)RAND_MAX, 0.0f)),
                 (float)rand() * 360.0f / (float)RAND_MAX, 0.75f, 1, nullptr, 0);
    }
    for (int i = 1; i <= tier; i++) {
        if (rand() % RewardGemOdds == 0) {
            Item item;
            item.category = ItemCategoryMaterial;
            item.id = i + (MaterialItemEmerald - 1);
            item.rarity = i;
            dropItem(item,
                     position + plasma::Vector<3, Fixed>(
                                    plasma::Vector<3, float>(1.0f - (float)rand() * 2.0f / (float)RAND_MAX,
                                                             1.0f - (float)rand() * 2.0f / (float)RAND_MAX, 0.0f)),
                     (float)rand() * 360.0f / (float)RAND_MAX, 0.75f, 1, nullptr, 0);
        }
    }
}

plasma::Vector<2, int> World::dungeonPosition(int x, int y, int type, bool large)
{
    if (large) {
        x = floorDivide(x, 2) * 2;
        y = floorDivide(y, 2) * 2;
    }
    plasma::Vector<2, int> position = placeCenter(x, y, large);
    if (dungeonType(x, y, type, large) == PlaceNone) {
        return plasma::Vector<2, int>(floorDivide(position[0], BlocksPerCell), floorDivide(position[1], BlocksPerCell));
    }
    // The centre cell of the dungeon room under the place's centre, moved up to two rooms each way (hashes with salts
    // 566485, 293184), and kept within the place's cells.
    position[0] = floorDivide(position[0], BlocksPerDungeonRoom) * CellsPerDungeonRoom + CellsPerDungeonRoom / 2;
    position[1] = floorDivide(position[1], BlocksPerDungeonRoom) * CellsPerDungeonRoom + CellsPerDungeonRoom / 2;
    if (type != PlaceGraveyard && type != PlaceMines && type != PlaceGrotto && type != PlacePyramid &&
        type != PlaceTemple && type != PlaceVault) {
        position[0] = position[0] + random(x + 566485, y) % 5 * CellsPerDungeonRoom - 2 * CellsPerDungeonRoom;
        position[1] = position[1] + random(x + 293184, y) % 5 * CellsPerDungeonRoom - 2 * CellsPerDungeonRoom;
    }
    position[0] = (std::min)((std::max)(position[0], floorDivide(x << 10, BlocksPerCell)),
                             floorDivide(x << 10, BlocksPerCell) +
                                 (!large ? BlocksPerPlace / BlocksPerCell : 2 * BlocksPerPlace / BlocksPerCell) - 1);
    position[1] = (std::min)((std::max)(position[1], floorDivide(y << 10, BlocksPerCell)),
                             floorDivide(y << 10, BlocksPerCell) +
                                 (!large ? BlocksPerPlace / BlocksPerCell : 2 * BlocksPerPlace / BlocksPerCell) - 1);
    return position;
}

// A noise colour per channel (frequency 0.01, offsets 9434, 123123; 7658, 3256; 6577, 74765), 127 +- 100.
plasma::Vector<3, float> World::dungeonShade(int x, int y)
{
    double scaledY = y * 0.01;
    double scaledX = x * 0.01;
    return plasma::Vector<3, float>(noise(scaledX + 9434.0, scaledY + 123123.0) * 100.0f + 127.0f,
                                    noise(scaledX + 7658.0, scaledY + 3256.0) * 100.0f + 127.0f,
                                    noise(scaledX + 6577.0, scaledY + 74765.0) * 100.0f + 127.0f);
}

int World::dungeonStorey(StructureCell cell)
{
    return cell.style == DungeonCellStyle_Wide ? WideDungeonStoreyHeight : DungeonStoreyHeight;
}

// The dungeon of a large place, where it varies drawn from a hash of the place (salt 404176): catacombs or a shrine
// for one in two, sewers under one village in eight.
int World::dungeonType(int x, int y, int type, bool large)
{
    if (!large) {
        return PlaceNone;
    }
    int cellX = floorDivide(x, 2) * 2;
    int cellY = floorDivide(y, 2) * 2;
    switch (type) {
    case PlaceGraveyard:
        return random(cellX + 404176, cellY) % 2 == 0 ? PlaceCatacombs : PlaceNone;
    case PlaceMines:
        return PlaceMines;
    case PlaceVault:
        return PlaceVault;
    case PlaceGrotto:
        return PlaceGrotto;
    case PlaceAncientRuins:
        return random(cellX + 404176, cellY) % 2 == 0 ? PlaceShrine : PlaceNone;
    case PlaceTemple:
        return PlaceTemple;
    case PlacePyramid:
        return PlacePyramid;
    case PlaceVillage:
        return random(cellX + 404176, cellY) % 8 == 0 ? PlaceSewers : PlaceNone;
    }
    return PlaceNone;
}

// A side room for one plain room in five (salt 464132), off a neighbouring room (salt 348768), a guardian room for one
// in three (salt 5746801) else a loot room (of width 2, the widest); the pillar and raised-walk flags as for the
// corridor's rooms.
Corridor dungeonWay(int x, int y, int direction, int startX, int startY, int type, int regionX, int regionY)
{
    Corridor result = basicCorridor(x, y, direction, startX, startY, type, regionX, regionY);
    if (result.kind == Corridor_None) {
        if (random(x + 464132, y) % 5 == 0) {
            int sides[4];
            int count = 0;
            if (basicCorridor(x - 1, y, direction, startX, startY, type, regionX, regionY).kind != 0) {
                sides[count++] = CorridorSide_West;
            }
            if (basicCorridor(x + 1, y, direction, startX, startY, type, regionX, regionY).kind != 0) {
                sides[count++] = CorridorSide_East;
            }
            if (basicCorridor(x, y - 1, direction, startX, startY, type, regionX, regionY).kind != 0) {
                sides[count++] = CorridorSide_South;
            }
            if (basicCorridor(x, y + 1, direction, startX, startY, type, regionX, regionY).kind != 0) {
                sides[count++] = CorridorSide_North;
            }
            if (count != 0) {
                result.side = sides[random(x + 348768, y) % count];
                switch (result.side) {
                case CorridorSide_West:
                    result.length = basicCorridor(x - 1, y, direction, startX, startY, type, regionX, regionY).length;
                    break;
                case CorridorSide_East:
                    result.length = basicCorridor(x + 1, y, direction, startX, startY, type, regionX, regionY).length;
                    break;
                case CorridorSide_South:
                    result.length = basicCorridor(x, y - 1, direction, startX, startY, type, regionX, regionY).length;
                    break;
                case CorridorSide_North:
                    result.length = basicCorridor(x, y + 1, direction, startX, startY, type, regionX, regionY).length;
                    break;
                }
                result.flags = CorridorFlagPillars;
                result.kind = random(x + 5746801, y) % 3 == 0 ? Corridor_GuardianRoom : Corridor_LootRoom;
                if (result.kind == Corridor_LootRoom) {
                    result.width = 2;
                } else {
                    result.width = type == PlaceMines || type == PlaceGrotto;
                }
                if (random(x + 18482581, y) & 1) {
                    result.flags |= CorridorFlagPillars;
                }
                if (type < PlaceSewers || type > PlaceGrotto || type == PlaceMines) {
                    if (random(x + 921518, y) & 1) {
                        result.flags |= CorridorFlagRaisedWalks;
                    }
                }
            }
        }
    }
    return result;
}

int dungeonWayLength(int x, int y, int direction, int startX, int startY, int regionX, int regionY)
{
    if (direction == DungeonDirectionWest) {
        if (x <= startX) {
            int length = 0;
            for (int i = 0; i <= startX - x; i++) {
                int currentBend = dungeonWayY(startX - i, startX, startY, regionX, regionY);
                int nextBend = dungeonWayY(startX - i - 1, startX, startY, regionX, regionY);
                int transverseSteps = i == startX - x ? y - currentBend : nextBend - currentBend;
                length += abs(transverseSteps) + 1;
            }
            return length;
        }
    }
    if (direction == DungeonDirectionEast) {
        if (x >= startX) {
            int length = 0;
            for (int i = 0; i <= x - startX; i++) {
                int currentBend = dungeonWayY(startX + i, startX, startY, regionX, regionY);
                int nextBend = dungeonWayY(startX + i + 1, startX, startY, regionX, regionY);
                int transverseSteps = i == x - startX ? y - currentBend : nextBend - currentBend;
                length += abs(transverseSteps) + 1;
            }
            return length;
        }
    }
    if (direction == DungeonDirectionSouth) {
        if (y <= startY) {
            int length = 0;
            for (int i = 0; i <= startY - y; i++) {
                int currentBend = dungeonWayX(startY - i, startX, startY, regionX, regionY);
                int nextBend = dungeonWayX(startY - i - 1, startX, startY, regionX, regionY);
                int transverseSteps = i == startY - y ? x - currentBend : nextBend - currentBend;
                length += abs(transverseSteps) + 1;
            }
            return length;
        }
    }
    if (direction == DungeonDirectionNorth) {
        if (y >= startY) {
            int length = 0;
            for (int i = 0; i <= y - startY; i++) {
                int currentBend = dungeonWayX(startY + i, startX, startY, regionX, regionY);
                int nextBend = dungeonWayX(startY + i + 1, startX, startY, regionX, regionY);
                int transverseSteps = i == y - startY ? x - currentBend : nextBend - currentBend;
                length += abs(transverseSteps) + 1;
            }
            return length;
        }
    }
    return 0;
}

// The corridor meanders: three blocks of noise per three rows (offsets 76643 for x, 10984 for y).
int dungeonWayX(int y, int startX, int startY, int regionX, int regionY)
{
    if (y == startY) {
        return startX;
    }
    return (int)(noise((y / 3) * 0.5, 76643.0) * 3.0f + startX);
}

int dungeonWayY(int x, int startX, int startY, int regionX, int regionY)
{
    if (x == startX) {
        return startY;
    }
    return (int)(noise((x / 3) * 0.5, 10984.0) * 3.0f + startY);
}

int World::dungeonZ(int x, int y)
{
    int cellX = floorDivide(x * BlocksPerCell, BlocksPerPlace);
    int cellY = floorDivide(y * BlocksPerCell, BlocksPerPlace);
    bool large = isLargePlace(cellX, cellY);
    int type = locationType(cellX, cellY, large);
    plasma::Vector<2, int> dungeon = dungeonPosition(cellX, cellY, type, large);
    int dungeonX = floorDivide(dungeon[0], CellsPerDungeonRoom);
    int dungeonY = floorDivide(dungeon[1], CellsPerDungeonRoom);
    int squareX = floorDivide(x, CellsPerDungeonRoom);
    int squareY = floorDivide(y, CellsPerDungeonRoom);
    if (squareX == dungeonX && squareY == dungeonY && type != PlacePyramid && type != PlaceTemple) {
        return structureLevel(dungeon[0], dungeon[1], 0) - DungeonDepth;
    }
    // The room's blocks (the room index shifted by 7: BlocksPerDungeonRoom).
    int minX = squareX << 7;
    int minY = squareY << 7;
    int maxX = minX + BlocksPerDungeonRoom;
    int maxY = minY + BlocksPerDungeonRoom;
    float center = height((maxX + minX) / 2, (maxY + minY) / 2, HeightFlagSkipPlaceShape | HeightFlagSkipPlaceAndHills);
    float corner11 = height(maxX, maxY, HeightFlagSkipPlaceShape | HeightFlagSkipPlaceAndHills);
    float corner01 = height(minX, maxY, HeightFlagSkipPlaceShape | HeightFlagSkipPlaceAndHills);
    float corner10 = height(maxX, minY, HeightFlagSkipPlaceShape | HeightFlagSkipPlaceAndHills);
    float corner00 = height(minX, minY, HeightFlagSkipPlaceAndHills);
    return (int)((std::min)(corner00, (std::min)(corner10, (std::min)(corner01, (std::min)(corner11, center)))) -
                 (float)DungeonDepth);
}

int World::dungeonZLevel(int x, int y)
{
    int height = dungeonZ(x, y);
    // A room's edge cells meet the neighbouring room half way.
    if ((x % CellsPerDungeonRoom + CellsPerDungeonRoom) % CellsPerDungeonRoom == 0) {
        int neighbour = dungeonZ(x - 1, y);
        if (neighbour < height) {
            neighbour = height;
        }
        height = (neighbour + height) / 2;
    } else if ((y % CellsPerDungeonRoom + CellsPerDungeonRoom) % CellsPerDungeonRoom == 0) {
        int neighbour = dungeonZ(x, y - 1);
        if (neighbour < height) {
            neighbour = height;
        }
        height = (neighbour + height) / 2;
    } else if ((x % CellsPerDungeonRoom + CellsPerDungeonRoom) % CellsPerDungeonRoom == CellsPerDungeonRoom - 1) {
        int neighbour = dungeonZ(x + 1, y);
        if (neighbour < height) {
            neighbour = height;
        }
        height = (neighbour + height) / 2;
    } else if ((y % CellsPerDungeonRoom + CellsPerDungeonRoom) % CellsPerDungeonRoom == CellsPerDungeonRoom - 1) {
        int neighbour = dungeonZ(x, y + 1);
        if (neighbour < height) {
            neighbour = height;
        }
        height = (neighbour + height) / 2;
    }
    int cellX = floorDivide(x * BlocksPerCell, BlocksPerPlace);
    int cellY = floorDivide(y * BlocksPerCell, BlocksPerPlace);
    bool large = isLargePlace(cellX, cellY);
    int type = locationType(cellX, cellY, large);
    if (large) {
        cellX = floorDivide(cellX, 2) * 2;
        cellY = floorDivide(cellY, 2) * 2;
    }
    int dungeon = dungeonType(cellX, cellY, type, large);
    if (dungeon != 0) {
        plasma::Vector<2, int> center = placeCenter(cellX, cellY, large);
        int direction = DungeonDirectionWest;
        int distance = 0;
        if (center[0] - (cellX << 11) > distance) {
            distance = center[0] - (cellX << 11);
        }
        direction = center[1] - (cellY << 11) <= distance ? DungeonDirectionWest : DungeonDirectionSouth;
        if (center[1] - (cellY << 11) > distance) {
            distance = center[1] - (cellY << 11);
        }
        if (((cellX + 2) << 10) - center[0] > distance) {
            direction = DungeonDirectionEast;
            distance = ((cellX + 2) << 10) - center[0];
        }
        if ((cellY + 2) * 1024 - center[1] > distance) {
            direction = DungeonDirectionNorth;
        }
        Corridor corridor = dungeonWay(floorDivide(x, CellsPerDungeonRoom), floorDivide(y, CellsPerDungeonRoom),
                                       direction, floorDivide(center[0], BlocksPerDungeonRoom),
                                       floorDivide(center[1], BlocksPerDungeonRoom), dungeon, cellX, cellY);
        // An uneven floor: up to five blocks higher per 3 x 3 cells (salt 505832).
        if (corridor.flags & CorridorFlagUnevenFloor) {
            return random(x / 3 + 505832, y / 3) % 6 + height;
        }
    }
    return height;
}

// The colour of earth at a block: orange-brown with two noise shades, blended towards the sand tone below z = 100 and
// where the lower of 1 - a and b falls under 0.3, towards a tinted rockColor where a is under 0.2, towards two
// noise-shaded tones by d and e, and towards rockColor by c.
plasma::Vector<3, float> World::earthColor(int x, int y, int z, float temperature, float humidity, float firelands,
                                           float undead, float enchantedForest)
{
    float shade = (noise(x * 0.015 + 1883.0, y * 0.015 + 98984.0) + 1.0f) * 0.5f;
    plasma::Vector<3, float> color =
        plasma::Vector<3, float>(260.0f, 160.0f, 10.0f) + plasma::Vector<3, float>(150.0f, 150.0f, 0.0f) * shade;
    double scaledX = x * 0.01;
    double scaledY = y * 0.01;
    float light = noise(scaledX + 431.0, scaledY + 873.0) * 30.0f + 30.0f;
    color += plasma::Vector<3, float>(light, light, light);
    if (z < 100) {
        float depth = std::max(0.0f, z / 100.0f);
        depth = depth * depth;
        color += (sandColor(x, y) - color) * (1.0f - depth);
    }
    float dry = std::min(1.0f - temperature, humidity);
    if (dry < 0.3f) {
        float keep = std::min(1.0f, dry / 0.3f);
        color = color * keep + sandColor(x, y) * (1.0f - keep);
    }
    if (temperature < 0.2f) {
        float rocky = std::max(0.0f, 1.0f - temperature * 5.0f);
        color *= 1.0f - rocky;
        color += (rockColor(x, y, z, temperature, humidity, firelands, undead) +
                  plasma::Vector<3, float>(200.0f, 280.0f, 400.0f)) *
                 (rocky * 0.5f);
    }
    if (undead > 0.001f) {
        float first = (noise(scaledX, scaledY) + 1.0f) * 20.0f;
        float second = (noise(x * 0.006 + 4343.0, y * 0.006 + 983.0) + 1.0f) * 20.0f;
        color = plasma::Vector<3, float>(first * 0.5f + 128.0f - second * 0.5f, 128.0f - second * 0.5f,
                                         first + 128.0f + second) *
                    undead +
                color * (1.0f - undead);
    }
    if (enchantedForest > 0.001f) {
        float first = (noise(scaledX, scaledY) + 1.0f) * 100.0f;
        float second = (noise(x * 0.006 + 4343.0, y * 0.006 + 983.0) + 1.0f) * 50.0f;
        color = plasma::Vector<3, float>(first + 128.0f - second * 0.5f, 128.0f, first * 0.5f + 128.0f + second) *
                    enchantedForest +
                color * (1.0f - enchantedForest);
    }
    if (firelands > 0.001f) {
        color = color * (1.0f - firelands) + rockColor(x, y, z, temperature, humidity, firelands, undead) * firelands;
    }
    return color;
}

float World::BiomeSample::enclave(int areaType, float areaInfluence)
{
    if (areaType == PlaceEnchantedForest) {
        return areaInfluence;
    }
    return 0.0f;
}

void World::enterZoneMeshes()
{
    EnterCriticalSection(&m_zoneMeshCriticalSection);
}

void World::enterZones()
{
    EnterCriticalSection(&m_zonesLock);
}

// A place's environment by a hash of the place with a salt of its own (5359179 one in ten for the monuments, 19718899
// one of ten otherwise).
int World::environmentType(int x, int y, int type, bool large)
{
    if (large) {
        x = floorDivide(x, 2) * 2;
        y = floorDivide(y, 2) * 2;
    }
    switch (type) {
    case PlaceMountain:
    case PlaceLake:
    case PlaceLavaLake:
    case PlaceManaPump:
    case PlaceCrater:
    case PlaceVolcano:
    case PlaceIsland:
    case PlaceForest:
    case PlaceCanyon:
    case PlacePlain:
    case PlaceHill:
    case PlaceGlade:
    case PlaceBeach:
    case PlaceMines:
    case PlaceGrotto:
    case PlaceVault:
        return EnvironmentTypeNone;
    case PlaceFarm:
    case PlaceVillage:
    case PlaceGraveyard:
    case PlaceCave:
    case PlaceAncientRuins:
    case PlaceRock:
    case PlaceShrine:
    case PlacePyramid:
    case PlaceTemple:
        return random(x + 5359179, y) % 10 != 0 ? EnvironmentTypeNone : EnvironmentTypeElementalRing;
    default:
        switch (random(x + 19718899, y) % 10) {
        case 0:
            return EnvironmentTypeElementalRing;
        case 1:
            return EnvironmentTypeRaisedPlateau;
        case 2:
            return EnvironmentTypeMoat;
        case 3:
            return EnvironmentTypeSpiralHill;
        }
        return EnvironmentTypeNone;
    }
}

// The shops restock daily: their draws are seeded by the shop's ID plus the day times 13 (and a salt per shop kind:
// 4332 armour, 87734 weapons); the stock is one to four items per tier (armour), one or two (weapons), three to seven
// gems, one of six pet foods.
void World::fillArmorShop(Inventory &shop, Creature *shopkeeper)
{
    shop.m_inventoryTabs.clear();
    std::vector<Item> items;
    int level = mostLevelTier();
    srand((int)shopkeeper->m_id + (m_state.m_day * 13 + 4332));
    for (int tier = 0; tier <= level; tier++) {
        int count = rand() % 4 + 1;
        for (int i = 0; i < count; i++) {
            Item item = randomArmor(tier, m_localCreature->m_entityData.classType, false);
            plasma::Vector<2, int> block((int)shopkeeper->m_entityData.origin[0],
                                         (int)shopkeeper->m_entityData.origin[1]);
            item.region = findRegion(block[0], block[1]);
            item.modifier = rand();
            items.push_back(item);
        }
    }
    std::sort(items.begin(), items.end(), [](const Item &a, const Item &b) { return a.rarity < b.rarity; });
    for (Item &item : items) {
        shop.add(item, (std::max)(0, 1 - boughtCount(item, shopkeeper->m_id)), 0);
    }
}

void World::fillFormulaShop(Inventory &shop, const long long &shopId)
{
    srand((int)shopId + m_state.m_day * 13);
    shop.m_inventoryTabs.clear();
    Item item;
    item.category = ItemCategoryMaterial;
    item.id = MaterialItemEmerald;
    item.rarity = ItemRarityGreen;
    int count = rand() % 5 + 3;
    count -= boughtCount(item, shopId);
    shop.add(item, std::max(count, 0), 0);
    item.id = MaterialItemSapphire;
    item.rarity = ItemRarityBlue;
    count = rand() % 5 + 2;
    count -= boughtCount(item, shopId);
    shop.add(item, std::max(count, 0), 0);
    item.id = MaterialItemRuby;
    item.rarity = ItemRarityPurple;
    count = rand() % 5 + 1;
    count -= boughtCount(item, shopId);
    shop.add(item, std::max(count, 0), 0);
    item.id = MaterialItemDiamond;
    item.rarity = ItemRarityYellow;
    count = rand() % 5;
    count -= boughtCount(item, shopId);
    shop.add(item, std::max(count, 0), 0);
}

void World::fillGeneralShop(Inventory &shop, const long long &shopId)
{
    srand((int)shopId + m_state.m_day * 13);
    shop.m_inventoryTabs.clear();
    Item item;
    item.category = ItemCategoryMaterial;
    item.id = MaterialItemFlask;
    item.material = ItemMaterialGlass;
    shop.add(item, -1, 0);
    item.category = ItemCategoryMaterial;
    item.id = MaterialItemSugarCube;
    item.material = ItemMaterialNone;
    item.rarity = ItemRarityWhite;
    shop.add(item, -1, 0);
    item.category = ItemCategoryPetFood;
    item.material = ItemMaterialNone;
    item.rarity = ItemRarityWhite;
    switch (rand() % 6) {
    case 0:
        item.id = PetFoodCottonCandy;
        break;
    case 1:
        item.id = PetFoodCarrot;
        break;
    case 2:
        item.id = PetFoodBubbleGum;
        break;
    case 3:
        item.id = PetFoodWaffle;
        break;
    case 4:
        item.id = PetFoodCandy;
        break;
    case 5:
        item.id = PetFoodChocolateDonut;
        break;
    }
    int count = 1 - boughtCount(item, shopId);
    shop.add(item, std::max(count, 0), 0);
    item.category = ItemCategoryConsumable;
    item.id = ConsumableBomb;
    shop.add(item, -1, 0);
}

void World::fillWeaponShop(Inventory &shop, Creature *shopkeeper)
{
    shop.m_inventoryTabs.clear();
    std::vector<Item> items;
    int level = mostLevelTier();
    srand((int)shopkeeper->m_id + (m_state.m_day * 13 + 87734));
    for (int tier = 0; tier <= level; tier++) {
        int count = rand() % 2 + 1;
        for (int i = 0; i < count; i++) {
            Item item = randomWeapon(tier, m_localCreature->m_entityData.classType, false);
            plasma::Vector<2, int> block((int)shopkeeper->m_entityData.origin[0],
                                         (int)shopkeeper->m_entityData.origin[1]);
            item.region = findRegion(block[0], block[1]);
            item.modifier = rand();
            items.push_back(item);
        }
    }
    std::sort(items.begin(), items.end(), [](const Item &a, const Item &b) { return a.rarity < b.rarity; });
    for (Item &item : items) {
        shop.add(item, (std::max)(0, 1 - boughtCount(item, shopkeeper->m_id)), 0);
    }
}

// The position displaced as in biomeSample (warp noise at 0.0003 and 0.0006, offsets 8432, 9438 and 567, 1223, an
// eighth of a land).
plasma::Vector<2, int> World::findNearestRegion(int x, int y)
{
    plasma::Vector<2, float> offset;
    offset[0] = noise(x * 0.0003 + 8432.0, y * 0.0006 + 9438.0) * 0.125f * BlocksPerLand;
    offset[1] = noise(x * 0.0006 + 567.0, y * 0.0003 + 1223.0) * 0.125f * BlocksPerLand;
    Fixed positionX = Fixed(x) + offset[0];
    Fixed positionY = Fixed(y) + offset[1];
    int minX = floorDivide(positionX.toBlock(), BlocksPerLand) - 1;
    int minY = floorDivide(positionY.toBlock(), BlocksPerLand) - 1;
    int maxX = floorDivide(positionX.toBlock(), BlocksPerLand) + 1;
    int maxY = floorDivide(positionY.toBlock(), BlocksPerLand) + 1;
    float best = -1.0f;
    plasma::Vector<2, int> result(-1, -1);
    for (int i = minX; i <= maxX; i++) {
        for (int j = minY; j <= maxY; j++) {
            plasma::Vector<2, int> center = landCenter(i, j);
            float dx = (float)(positionX - Fixed(center[0]));
            float dy = (float)(positionY - Fixed(center[1]));
            float squaredDistance = dy * dy + dx * dx;
            if (best < 0.0f || squaredDistance < best) {
                best = squaredDistance;
                result = plasma::Vector<2, int>(i, j);
            }
        }
    }
    return result;
}

plasma::Vector<2, int> World::findRegion(const plasma::Vector<3, Fixed> &position)
{
    if ((getBlockAtDot(position[0], position[1], position[2]).flags & BlockFlagDungeon) != 0) {
        plasma::Vector<2, int> cell(floorDivide((int)position[0], BlocksPerPlace),
                                    floorDivide((int)position[1], BlocksPerPlace));
        plasma::Vector<2, int> center = placeCenter(cell[0], cell[1], isLargePlace(cell[0], cell[1]));
        return findRegion(center[0], center[1]);
    }
    return findRegion((int)position[0], (int)position[1]);
}

plasma::Vector<2, int> World::findRegion(int x, int y)
{
    int cellX = floorDivide(x, BlocksPerPlace);
    int cellY = floorDivide(y, BlocksPerPlace);
    bool large = isLargePlace(cellX, cellY);
    if (locationType(cellX, cellY, large) == PlaceNone) {
        return findNearestRegion(x, y);
    }
    plasma::Vector<2, int> center = placeCenter(cellX, cellY, large);
    float squaredDistance = placeSquaredDistance(center, x, y);
    int radius = large ? LargePlaceRadius : SmallPlaceRadius;
    float squaredRadius = (float)(radius * radius);
    if (squaredDistance >= squaredRadius ||
        (1.0f - squaredDistance / squaredRadius) * (1.0f - squaredDistance / squaredRadius) < PlaceRegionInfluence) {
        return findNearestRegion(x, y);
    }
    return findNearestRegion(center[0], center[1]);
}

// The region noise: frequency 0.0015 plus the world's offset, a hashed jitter (noise of 37x, 37y at 0.05), lowered by
// 0.2 and raised by a tenth of the weight's complement.
float World::findRegionNoise(int x, int y, float weight)
{
    return noise(x * 0.0015 + m_regionNoiseOffset[0], y * 0.0015 + m_regionNoiseOffset[1]) +
           noise(x * 37, y * 37) * 0.05f - 0.2f + (1.0f - weight) * 0.1f;
}

int World::flightDuration(const plasma::Vector<2, int> &from, const plasma::Vector<2, int> &flightPoint)
{
    plasma::Vector<2, int> d = from - flightPoint;
    int time = (int)(sqrt((double)d[0] * d[0] + (double)d[1] * d[1]) * FlightTimePerBlock);
    if (time < MinFlightTime) {
        return MinFlightTime;
    }
    if (time > MaxFlightTime) {
        return MaxFlightTime;
    }
    return time;
}

// A coin per 100 blocks of the shortest leg (half a leg from a known flight point), at least 100; std::max takes the
// 100 by reference, so it stays literal.
int World::flightPrice(const plasma::Vector<2, int> &flightPoint)
{
    Creature *creature = m_localCreature;
    plasma::Vector<2, double> target(flightPoint);
    plasma::Vector<2, double> position((double)creature->m_entityData.position[0],
                                       (double)creature->m_entityData.position[1]);
    plasma::Vector<2, double> d = position - target;
    double best = sqrt(d[0] * d[0] + d[1] * d[1]);
    std::set<std::array<int, 2>> &known = creature->m_playerData->knownFlightPoints;
    for (std::set<std::array<int, 2>>::iterator it = known.begin(); it != known.end(); ++it) {
        plasma::Vector<2, double> e(plasma::Vector<2, int>((*it)[0], (*it)[1]) - flightPoint);
        double distance = sqrt(e[0] * e[0] + e[1] * e[1]) * 0.5;
        if (best < 0.0 || distance < best) {
            best = distance;
        }
    }
    return (int)(float)(std::max)(100, (int)std::max(0.0, best) / 100);
}

float World::BiomeSample::forest()
{
    float distance = -1.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (isForest(biomes[i][j]) && (distance < 0.0f || distance > squaredDistances[i][j])) {
                distance = squaredDistances[i][j];
            }
        }
    }
    return weight(distance, BiomeBlendRate);
}

void World::gatherObstacles()
{
    m_obstacles.clear();
    for (Creature *creature : m_creatures) {
        if (creature->isObstacle()) {
            m_obstacles.push_back(creature);
        }
    }
}

// The foliage palette (hand-authored): green (0, 180, 100) to yellow-green (255, 320, 0), snow-white below
// SnowLineTemperature, tinted towards hashed greys and blues in undead lands (above 0.01) and violets in enclaves
// (above 0.3).
void World::groundColors(int x, int y, plasma::Vector<3, float> &foliageBottom, plasma::Vector<3, float> &foliageTop,
                         float temperature, float undead, float enchantedForest)
{
    unsigned int seed = xorshift(xorshift(y) + x);
    foliageBottom = plasma::Vector<3, float>(0.0f, 180.0f, 100.0f);
    foliageTop = plasma::Vector<3, float>(255.0f, 320.0f, 0.0f);
    if (temperature < SnowLineTemperature) {
        foliageBottom = plasma::Vector<3, float>(200.0f, 280.0f, 400.0f);
        foliageTop = foliageBottom;
    }
    if (undead > 0.01f) {
        foliageBottom +=
            (plasma::Vector<3, float>(100.0f, 100.0f, (int)(xorshift(seed++) % 80) + 140) - foliageBottom) * undead;
        foliageTop += (plasma::Vector<3, float>(60.0f, 60.0f, xorshift(seed++) % 30 + 60) - foliageTop) * undead;
    }
    if (enchantedForest > 0.3f) {
        // Increments seed twice in one argument list, which is unsequenced and has undefined behavior.
        foliageBottom +=
            (plasma::Vector<3, float>(xorshift(seed++) % 55 + 200, 0.0f, xorshift(seed++) % 200) - foliageBottom) *
            enchantedForest;
        foliageTop +=
            (plasma::Vector<3, float>(100.0f, xorshift(seed++) % 50 + 100, xorshift(seed++) % 55 + 200) - foliageTop) *
            enchantedForest;
    }
}

// A crater ring five blocks wide around the moat's 75% radius, its depth 40 (20 at the rim) scaled by the influence
// beyond a quarter.
bool World::groundCrater(int x, int y, const AreaInfo &info, float weight, float *floor, float *terrainHeight)
{
    if (weight < 0.5f) {
        return false;
    }
    if (info.environment != EnvironmentTypeMoat) {
        return false;
    }
    float squaredDistance = info.squaredDistance;
    int radius = info.radius;
    if (squaredDistance > (float)(radius * radius)) {
        return false;
    }
    float ring = (sqrtf(squaredDistance) / (float)radius - 0.5f) * 5.0f;
    ring = std::min(1.0f, ring * ring);
    float depth = std::min(1.0f, 1.0f - (1.0f - weight) / (ring * 0.75f + 0.25f));
    if (depth < 0.5f) {
        return false;
    }
    float terrain = height(x, y, HeightFlagSkipPlaceShape);
    *terrainHeight = terrain;
    *floor = terrain - 1.0f - (ring * 40.0f + 20.0f) * (depth - 0.5f);
    return true;
}

// groundColors' palette with hashed variations per channel (the ranges are the palette's).
void World::groundHashedColors(int x, int y, plasma::Vector<3, float> &foliageBottom,
                               plasma::Vector<3, float> &foliageTop, float undead, float enchantedForest)
{
    unsigned int seed = xorshift(xorshift(y) + x);
    // Builds byte colours from hash values; multiple seed increments in one argument list are unsequenced.
    foliageBottom = plasma::Vector<3, float>(plasma::Vector<3, unsigned char>(
        xorshift(seed++) % 10, xorshift(seed++) % 50 + 100, xorshift(seed++) % 80 + 50));
    foliageTop = plasma::Vector<3, float>(plasma::Vector<3, unsigned char>(
        xorshift(seed++) % 10, xorshift(seed++) % 50 + 100, xorshift(seed++) % 80 + 50));
    if (undead > 0.01f) {
        foliageBottom +=
            (plasma::Vector<3, float>(100.0f, 100.0f, (int)(xorshift(seed++) % 80) + 140) - foliageBottom) * undead;
        foliageTop += (plasma::Vector<3, float>(60.0f, 60.0f, xorshift(seed++) % 30 + 60) - foliageTop) * undead;
    }
    if (enchantedForest > 0.3f) {
        // Increments seed twice in one argument list, which is unsequenced and has undefined behavior.
        foliageBottom +=
            (plasma::Vector<3, float>(xorshift(seed++) % 55 + 200, 0.0f, xorshift(seed++) % 200) - foliageBottom) *
            enchantedForest;
        foliageTop +=
            (plasma::Vector<3, float>(100.0f, xorshift(seed++) % 50 + 100, xorshift(seed++) % 55 + 200) - foliageTop) *
            enchantedForest;
    }
}

void World::groundlessFormula(int id, Formula &formula)
{
    formula = Formula();
}

int World::hamletPlot(int x, int y, int environment, const plasma::Vector<2, int> &dungeon)
{
    if (environment != EnvironmentTypeMoat) {
        return PlotNone;
    }
    int plotX = floorDivide(x, CellsPerPlot);
    int plotY = floorDivide(y, CellsPerPlot);
    int cellX = floorDivide(x * BlocksPerCell, BlocksPerPlace);
    int cellY = floorDivide(y * BlocksPerCell, BlocksPerPlace);
    cellX = floorDivide(cellX, 2) * 2;
    cellY = floorDivide(cellY, 2) * 2;
    int blockX = plotX * BlocksPerPlot;
    int blockY = plotY * BlocksPerPlot;
    bool large = isLargePlace(cellX, cellY);
    plasma::Vector<2, int> center = placeCenter(cellX, cellY, large);
    int originX = floorDivide(floorDivide(center[0], BlocksPerCell), CellsPerPlot) - 2;
    int endX = originX + 4;
    int originY = floorDivide(floorDivide(center[1], BlocksPerCell), CellsPerPlot) - 2;
    if (plotX >= originX && plotX < endX && plotY >= originY && plotY < originY + 4) {
        int type = m_hamletPlotLayouts[random(cellX, cellY) % m_hamletPlotLayouts.size()]
                                      [plotX - originX + (plotY - originY) * 4];
        if (type != 0) {
            return type;
        }
    }
    float squaredDistance =
        placeSquaredDistance(placeCenter(cellX, cellY, large), blockX + BlocksPerCell, blockY + BlocksPerCell);
    int radius = large ? LargePlaceRadius : SmallPlaceRadius;
    float squaredRadius = (float)(radius * radius);
    if (!(squaredDistance >= squaredRadius)) {
        float influence = (1.0f - squaredDistance / squaredRadius) * (1.0f - squaredDistance / squaredRadius);
        if (!(influence < 0.5f) && !(streetAmount(blockX + 16, blockY + 16, 5, influence) > 0.4f)) {
            int value = random(plotX + 1161615, plotY) % 1000;
            if ((float)value > influence * 800.0f) {
                return (value & 1) != 0 ? PlotNone : PlotField;
            }
            return PlotHouse;
        }
    }
    return PlotNone;
}

// The harvest palette (hand-authored): hashed (seed salts 43243, 983) warm and dry tones by temperature (from 0.2) and
// humidity (shifted by up to 40 at 0.5), cold, undead and enclave tones by the same gates as groundColors.
void World::harvestColors(int x, int y, plasma::Vector<3, float> &foliageBottom, plasma::Vector<3, float> &foliageTop,
                          float temperature, float humidity, float undead, float enchantedForest)
{
    // Multiple seed increments in one argument list are unsequenced and have undefined behavior.
    unsigned int seed = xorshift(x * 100 + 43243) + xorshift(y * 100 + 983);
    if ((xorshift(seed++) % 1000) / 1000.0f < (temperature - 0.2f) * 5.0f || xorshift(seed++) % 3 == 0) {
        foliageBottom =
            plasma::Vector<3, float>(xorshift(seed++) % 20, xorshift(seed++) % 60 + 100, xorshift(seed++) % 30);
        float shift = (0.5f - humidity) * 80.0f;
        foliageBottom[0] += shift;
        if (humidity > 0.5f) {
            foliageBottom[2] -= shift;
        }
        int lighter = xorshift(seed++) % 100 + 50;
        foliageTop = foliageBottom + plasma::Vector<3, float>(lighter, lighter, 0.0f);
        foliageBottom[2] += 80.0f;
        foliageBottom[1] -= 20.0f;
        foliageBottom[0] -= 20.0f;
    } else if (humidity < 0.5f) {
        foliageBottom =
            plasma::Vector<3, float>(xorshift(seed++) % 55 + 200, xorshift(seed++) % 150, xorshift(seed++) % 50);
        int lighter = xorshift(seed++) % 100 + 60;
        foliageTop = foliageBottom + plasma::Vector<3, float>(lighter, lighter, 0.0f);
    } else {
        switch (xorshift(seed++) % 2) {
        case 0: {
            foliageBottom = plasma::Vector<3, float>(xorshift(seed++) % 100 + 150, 0.0f, xorshift(seed++) % 230);
            int shade = xorshift(seed++) % 80;
            foliageBottom += plasma::Vector<3, float>(shade, shade, shade);
            int lighter = xorshift(seed++) % 50 + 100;
            foliageTop = foliageBottom + plasma::Vector<3, float>(lighter, lighter, lighter);
            break;
        }
        case 1: {
            foliageBottom = plasma::Vector<3, float>(xorshift(seed++) % 50 + 180, xorshift(seed++) % 50 + 120,
                                                     xorshift(seed++) % 50 + 240);
            int lighter = xorshift(seed++) % 50 + 100;
            foliageTop = foliageBottom + plasma::Vector<3, float>(lighter, lighter, lighter);
            break;
        }
        }
    }
    if (undead > 0.01f) {
        foliageBottom +=
            (plasma::Vector<3, float>(100.0f, 100.0f, (int)(xorshift(seed++) % 80) + 140) - foliageBottom) * undead;
        foliageTop += (plasma::Vector<3, float>(60.0f, 60.0f, xorshift(seed++) % 30 + 60) - foliageTop) * undead;
    }
    if (hasLyingSnow(x, y, temperature, 0.0f, 0.0f, 0.0f, 0.0f) != 0.0f) {
        foliageBottom = plasma::Vector<3, float>(200.0f, 280.0f, 400.0f);
        foliageTop = foliageBottom;
    }
    if (enchantedForest > 0.3f) {
        foliageBottom +=
            (plasma::Vector<3, float>(xorshift(seed++) % 55 + 200, 0.0f, xorshift(seed++) % 200) - foliageBottom) *
            enchantedForest;
        foliageTop +=
            (plasma::Vector<3, float>(100.0f, xorshift(seed++) % 50 + 100, xorshift(seed++) % 55 + 200) - foliageTop) *
            enchantedForest;
    }
}

// The sky, fog and light colours (a hand-authored palette): the daylight curve (exponents 3 and 4), tints for humid
// (0.6, 0.65, 0.8), hot and dry, lava, undead and enchanted places blended by their weights (from 0.001), the sun's
// colours by daylight band, dungeon and coast fades (20 blocks), and the spirit world's colours.
void World::harvestEnvironmentColors(const plasma::Vector<3, Fixed> &position, plasma::Vector<4, float> &skyColor1,
                                     plasma::Vector<4, float> &skyColor2, plasma::Vector<4, float> &fogColor,
                                     plasma::Vector<4, float> &sunColor, plasma::Vector<4, float> &light,
                                     plasma::Vector<4, float> &backLightColor, plasma::Vector<4, float> &ambientColor)
{
    typedef plasma::Vector<4, float> Color;
    float day = std::pow(
        1.0f - std::pow((float)fabs((double)((m_state.m_time + m_state.m_time) / MillisecondsPerDay - 1.0f)), 3.0f),
        4.0f);
    Color skyColor1Sunset(1, 0.95f, 0.5f, 1);
    Color skyColor1Dusk(1, 0.5f, 0.1f, 1);
    Color skyColor1Day(0, 0.6f, 1, 1);
    Color skyColor2Day(0, 0, 0.8f, 1);
    Color fogColorDay(0, 0.1f, 1, 1);
    Color skyColor2Sunset(0.3f, 0.5f, 1, 1);
    Color skyColor1Night(0, 0.03f, 0.15f, 1);
    Color skyColor2Dusk(0.5f, 0.2f, 0.8f, 1);
    Color fogColorSunset(0.7f, 0.6f, 0.3f, 1);
    Color fogColorDusk(1, 0.5f, 0.2f, 1);
    Color fogColorNight(0, 0.1f, 0.1f, 1);
    BiomeSample sample;
    biomeSample((int)position[0], (int)position[1], sample);
    AreaInfo area;
    fillAreaInfo((int)position[0], (int)position[1], area);
    float temperature = sample.temperature(area.type, area.influence);
    float humidity = sample.humidity();
    float terrainHeight = height((int)position[0], (int)position[1], false);
    if (terrainHeight < 20.0f) {
        float coastal = (std::max)(terrainHeight / 20.0f, 0.0f);
        humidity *= coastal;
        temperature *= coastal;
    }
    if (humidity > 0.5f) {
        float wet = (humidity - 0.5f) * 2.0f;
        fogColorDay += (Color(0, 0.6f, 0.65f, 1) - fogColorDay) * wet;
        skyColor1Day += (Color(0, 0.6f, 0.8f, 1) - skyColor1Day) * wet;
        skyColor2Day += (Color(0, 0.5f, 0.5f, 1) - skyColor2Day) * wet;
    }
    if (temperature > 0.8f && humidity < 0.4f) {
        float dry = (std::min)(1.0f, (1.0f - humidity / 0.4f) * ((temperature - 0.8f) / 0.2f) * 4.0f);
        skyColor1Day += (Color(0.6f, 0.8f, 1, 1) - skyColor1Day) * dry;
        skyColor2Day += (Color(0.1f, 0.4f, 1, 1) - skyColor2Day) * dry;
        fogColorDay += (Color(0.8f, 0.5f, 0.7f, 1) - fogColorDay) * dry;
    }
    float lava = sample.lavaWeight(area.type, area.influence);
    if (lava > 0.001f) {
        skyColor1Day = Color(1, 0, 0, 1) * lava + skyColor1Day * (1.0f - lava);
        skyColor1Sunset = Color(0.8f, 0.1f, 0, 1) * lava + skyColor1Sunset * (1.0f - lava);
        skyColor1Dusk = Color(0.8f, 0.1f, 0, 1) * lava + skyColor1Dusk * (1.0f - lava);
        skyColor1Night = Color(0.06f, 0, 0.03f, 1) * lava + skyColor1Night * (1.0f - lava);
        skyColor2Day = Color(0.4f, 0.2f, 0.2f, 1) * lava + skyColor2Day * (1.0f - lava);
        skyColor2Sunset = Color(0.2f, 0.2f, 0.2f, 1) * lava + skyColor2Sunset * (1.0f - lava);
        skyColor2Dusk = Color(0.2f, 0.2f, 0.2f, 1) * lava + skyColor2Dusk * (1.0f - lava);
        fogColorDay = Color(0.3f, 0, 0, 1) * lava + fogColorDay * (1.0f - lava);
        fogColorSunset = Color(0.3f, 0, 0, 1) * lava + fogColorSunset * (1.0f - lava);
        fogColorDusk = Color(0.3f, 0.1f, 0, 1) * lava + fogColorDusk * (1.0f - lava);
        fogColorNight = Color(0.1f, 0, 0, 1) * lava + fogColorNight * (1.0f - lava);
    }
    float undead = sample.undead(area.type, area.influence);
    if (undead > 0.001f) {
        skyColor1Day = Color(0.2f, 0.6f, 0.4f, 1) * undead + skyColor1Day * (1.0f - undead);
        skyColor1Sunset = Color(0.1f, 0.6f, 0.3f, 1) * undead + skyColor1Sunset * (1.0f - undead);
        skyColor1Dusk = Color(0.3f, 0.1f, 0.4f, 1) * undead + skyColor1Dusk * (1.0f - undead);
        skyColor2Day = Color(0, 0.1f, 0.2f, 1) * undead + skyColor2Day * (1.0f - undead);
        skyColor2Sunset = Color(0, 0.05f, 0.1f, 1) * undead + skyColor2Sunset * (1.0f - undead);
        skyColor2Dusk = Color(0.2f, 0.2f, 0.2f, 1) * undead + skyColor2Dusk * (1.0f - undead);
        fogColorDay = Color(0.4f, 0.1f, 0.8f, 1) * undead + fogColorDay * (1.0f - undead);
        fogColorSunset = Color(0.4f, 0.1f, 0.8f, 1) * undead + fogColorSunset * (1.0f - undead);
        fogColorDusk = Color(0.4f, 0.2f, 0.5f, 1) * undead + fogColorDusk * (1.0f - undead);
        fogColorNight = Color(0.1f, 0.15f, 0.2f, 1) * undead + fogColorNight * (1.0f - undead);
    }
    float enclave = area.type == PlaceEnchantedForest ? area.influence : 0.0f;
    if (enclave > 0.001f) {
        skyColor1Day = Color(1, 0.6f, 1, 1) * enclave + skyColor1Day * (1.0f - enclave);
        skyColor1Sunset = Color(0.8f, 0.2f, 1, enclave) * enclave + skyColor1Sunset * (1.0f - enclave);
        skyColor2Day = Color(0, 0.6f, 1, 1) * enclave + skyColor2Day * (1.0f - enclave);
        skyColor2Sunset = Color(0, 0.5f, 1, 1) * enclave + skyColor2Sunset * (1.0f - enclave);
        skyColor2Dusk = Color(0.1f, 0.5f, 0.8f, 1) * enclave + skyColor2Dusk * (1.0f - enclave);
        fogColorDay = Color(0.6f, 0.3f, 1, 1) * enclave + fogColorDay * (1.0f - enclave);
        fogColorSunset = Color(0.6f, 0.3f, 1, 1) * enclave + fogColorSunset * (1.0f - enclave);
    }
    if (isFlyingFar()) {
        fogColorDay = skyColor1Day;
        fogColorSunset = skyColor1Sunset;
        fogColorNight = skyColor1Night;
        skyColor1Day = Color(0, 0, 0, 1);
        skyColor2Day = Color(0, 0, 0, 1);
        skyColor1Sunset = Color(0, 0, 0, 1);
        skyColor2Sunset = Color(0, 0, 0, 1);
        skyColor1Night = Color(0, 0, 0, 1);
    }
    skyColor1 = Color(0.5f, 0.8f, 1, 1) * day + Color(0, 0, 0.2f, 1) * (1.0f - day);
    skyColor2 = Color(0, 0, 1, 1) * day + Color(0, 0, 0, 1) * (1.0f - day);
    if (day > 0.75f) {
        skyColor1 = skyColor1Day;
        skyColor2 = skyColor2Day;
        fogColor = fogColorDay;
        sunColor = Color(1.5f, 1.3f, 0.4f, 1);
    } else if (day > 0.6f) {
        float t = (day - 0.6f) / 0.15f;
        skyColor1 = skyColor1Day * t + skyColor1Sunset * (1.0f - t);
        skyColor2 = skyColor2Day * t + skyColor2Sunset * (1.0f - t);
        fogColor = fogColorDay * t + fogColorSunset * (1.0f - t);
        sunColor = Color(1.5f, 1.3f, 0.4f, 1) * t + Color(1.5f, 0.85f, 0.25f, 1) * (1.0f - t);
    } else if (day > 0.2f) {
        float t = (day - 0.2f) / 0.4f;
        skyColor1 = skyColor1Sunset * t + skyColor1Dusk * (1.0f - t);
        skyColor2 = skyColor2Sunset * t + skyColor2Dusk * (1.0f - t);
        fogColor = fogColorSunset * t + fogColorDusk * (1.0f - t);
        sunColor = Color(1.5f, 0.85f, 0.25f, 1) * t + Color(1.5f, 0.85f, 0.25f, 1) * (1.0f - t);
    } else {
        float t = (std::max)(1.0f - (1.0f - day / 0.2f) * 4.0f, 0.0f);
        skyColor1 = skyColor1Night * (1.0f - t) + skyColor1Dusk * t;
        skyColor2 = Color(0, 0, 0, 1) * (1.0f - t) + skyColor2Dusk * t;
        fogColor = fogColorNight * (1.0f - t) + fogColorDusk * t;
        sunColor = Color(0, 0, 0, 0) * (1.0f - t) + Color(1.5f, 0.85f, 0.25f, 1) * t;
    }
    Color lightDay(0.75f, 0.7f, 0.3f, 1);
    Color lightDusk(0.65f, 0.6f, 0.15f, 1);
    Color lightNight(0.04f, 0.05f, 0.1f, 1);
    Color ambientColorDay(0.25f, 0.3f, 0.7f, 1);
    Color ambientColorDusk(0.15f, 0.2f, 0.75f, 1);
    Color ambientColorNight(0.05f, 0.075f, 0.1f, 1);
    if (getBlockAtDot(position[0], position[1], position[2]).flags & BlockFlagDungeon) {
        int x = (int)position[0];
        int y = (int)position[1];
        int type = dungeonType(x >= 0 ? x / BlocksPerPlace : (x + 1) / BlocksPerPlace - 1,
                               y >= 0 ? y / BlocksPerPlace : (y + 1) / BlocksPerPlace - 1, area.type, area.large);
        ambientColorDay =
            type == PlaceShrine || type == PlaceGrotto ? Color(0.1f, 0.3f, 0.4f, 1) : Color(0.25f, 0.1f, 0.5f, 1);
        lightDay = Color(1.5f, 1.5f, 1.5f, 1) - ambientColorDay;
        ambientColorDusk = ambientColorDay * 0.9f;
        lightDusk = Color(1, 1, 1, 1) - ambientColorDusk;
    }
    if (humidity < 0.3f) {
        ambientColorDay += (Color(0.3f, 0.3f, 0.2f, 1) - ambientColorDay) * (1.0f - humidity / 0.3f);
        lightDay = Color(1, 1, 1, 1) - ambientColorDay;
    }
    if (lava > 0.001f) {
        lightDay = Color(1.1f, 1, 0.1f, 1) * lava + lightDay * (1.0f - lava);
        lightDusk = Color(1, 1, 0, 1) * lava + lightDusk * (1.0f - lava);
        lightNight = Color(1, 0.8f, 0.1f, 1) * lava + lightNight * (1.0f - lava);
        ambientColorDay = Color(0.4f, 0.3f, 0.2f, 1) * lava + ambientColorDay * (1.0f - lava);
        ambientColorDusk = Color(0.3f, 0.2f, 0.1f, 1) * lava + ambientColorDusk * (1.0f - lava);
        ambientColorNight = Color(0.1f, 0.05f, 0, 1) * lava + ambientColorNight * (1.0f - lava);
    }
    if (undead > 0.001f) {
        lightDay = Color(0.2f, 0.2f, 0.9f, 1) * undead + lightDay * (1.0f - undead);
        lightDusk = Color(0.2f, 0.4f, 0.4f, 1) * undead + lightDusk * (1.0f - undead);
        ambientColorNight = Color(0.1f, 0.1f, 0.2f, 1) * undead + ambientColorNight * (1.0f - undead);
        ambientColorDay = Color(0.2f, 0.2f, 0.3f, 1) * undead + ambientColorDay * (1.0f - undead);
        ambientColorDusk = Color(0.2f, 0.2f, 0.3f, 1) * undead + ambientColorDusk * (1.0f - undead);
        // Blends the night ambient colour a second time; the night light is unchanged.
        ambientColorNight = Color(0, 0.01f, 0.01f, 1) * undead + ambientColorNight * (1.0f - undead);
    }
    if (enclave > 0.001f) {
        lightDay = Color(0.9f, 0.7f, 0.6f, 1) * enclave + lightDay * (1.0f - enclave);
        lightDusk = Color(0.9f, 0.7f, 0.6f, 1) * enclave + lightDusk * (1.0f - enclave);
        ambientColorDay = Color(0.3f, 0.5f, 0.5f, 1) * enclave + ambientColorDay * (1.0f - enclave);
        ambientColorDusk = Color(0.3f, 0.5f, 0.5f, 1) * enclave + ambientColorDusk * (1.0f - enclave);
    }
    if (day > 0.5f) {
        float night = 1.0f - (day - 0.5f) * 2.0f;
        float t = 1.0f - night * night * night * night;
        light = lightDay * t + lightDusk * (1.0f - t);
        backLightColor = Color(0.5f, 0.5f, 0.5f, 1) * t + Color(0.25f, 0.25f, 0.25f, 1) * (1.0f - t);
        ambientColor = ambientColorDay * t + ambientColorDusk * (1.0f - t);
    } else {
        float t = day * 2.0f;
        light = lightNight * (1.0f - t) + lightDusk * t;
        backLightColor = Color(0.05f, 0.05f, 0.3f, 1) * (1.0f - t) + Color(0.25f, 0.25f, 0.25f, 1) * t;
        ambientColor = ambientColorNight * (1.0f - t) + ambientColorDusk * t;
    }
    ambientColor[3] = day;
    if (getBlockAtDot(position[0], position[1], position[2]).type == BlockWater) {
        fogColor = skyColor2;
        skyColor1 = skyColor2;
    }
    if (m_localCreature && m_localCreature->findBuff(BuffSpiritworld)) {
        float t = (std::min)(1.0f, (SpiritworldDuration - m_localCreature->findBuff(BuffSpiritworld)->timeRemaining) /
                                       SpiritworldFadeTime);
        light += (Color(0.05f, 0.1f, 1, 1) - light) * t;
        backLightColor += (Color(0.05f, 0.1f, 1, 1) - backLightColor) * t;
        ambientColor += (Color(0.01f, 0.05f, 0.2f, 1) - ambientColor) * t;
        fogColor += (Color(0.9f, 1, 1, 1) - fogColor) * t;
        skyColor1 += (Color(0.9f, 1, 1, 1) - skyColor1) * t;
        skyColor2 += (Color(0, 0, 0, 1) - skyColor2) * t;
    }
}

// Two noises (frequency 0.005, offsets 343, 9233; 0.02, offsets 6654, 778) blend a hand-authored grass palette.
plasma::Vector<3, float> World::harvestGrassColor(int x, int y)
{
    float grassPaletteWeight = (noise(x * 0.005 + 343.0, y * 0.005 + 9233.0) + 1.0f) * 0.5f;
    grassPaletteWeight = grassPaletteWeight * grassPaletteWeight;
    float lightDarkBlend = (noise(x * 0.02 + 6654.0, y * 0.02 + 778.0) + 1.0f) * 0.5f;
    plasma::Vector<3, float> dark(80.0f - grassPaletteWeight * 60.0f, 200.0f - grassPaletteWeight * 50.0f,
                                  grassPaletteWeight * 80.0f + 40.0f);
    plasma::Vector<3, float> light(220.0f - grassPaletteWeight * 100.0f, 260.0f, 30.0f);
    plasma::Vector<3, float> color = light * lightDarkBlend + dark * (1.0f - lightDarkBlend);
    color += plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    return color;
}

bool World::hasCraftingRank(int rank)
{
    if (!m_localCreature)
        return false;
    return Missions(this, m_localCreature->m_entityData.region).hasCraftingRank(rank);
}

bool World::hasLore(int x, int y)
{
    switch (zoneBuildingType(x, y)) {
    case BuildingRockLorePavilion:
    case BuildingLoreCrypt:
    case BuildingInscribedMenhir:
    case BuildingSanctuary:
    case BuildingRaisedSanctuary:
        return true;
    }
    return false;
}

// Snow lies below temperature 0.3 on level ground (height steps under 2): always below 0.1, else where noise (0.05;
// offsets 8411122, 884646) at 0.05 weight plus 0.2 exceeds the temperature.
float World::hasLyingSnow(int x, int y, float temperature, float heightWest, float heightEast, float heightSouth,
                          float heightNorth)
{
    int result;
    if (temperature < 0.3f && (float)fabs(heightWest - heightEast) < 2.0f &&
        (float)fabs(heightSouth - heightNorth) < 2.0f &&
        (temperature < 0.1f || noise(x * 0.05 + 8411122.0, y * 0.05 + 884646.0) * 0.05f + 0.2f > temperature)) {
        result = 1;
    } else {
        result = 0;
    }
    return (float)result;
}

bool World::hasMission(int x, int y)
{
    int type = zoneBuildingType(x, y);
    return type == BuildingRockTreasure || type == BuildingCave || type == BuildingGroundTreasureSite ||
           type == BuildingUnderwaterTreasureSite || type == BuildingTreasureTrunk ||
           type == BuildingFloatingIslandTreasurePavilion || type == BuildingBuriedTreasureChamber ||
           type == BuildingSandstoneTreasureChamber;
}

// Computes a column's terrain height from biome elevation, place shape, hills, cliffs, channels, roads and beaches.
// HeightFlagSkipPlaceShape suppresses place shaping; HeightFlagSkipPlaceAndHills also suppresses hills and type-10
// rise. HeightFlagCaveMouths enables cave openings and HeightFlagSkipRiverBeds suppresses river beds. mountains is
// unused. The recipe combines 10..35-block hills, mountain rises up to 180 blocks, noise terraces in 20-block steps,
// plateaus, moats, spirals, peaks and ridges.
float World::height(int x, int y, float elevation, float cliffs, float river, float lavaRiver, float poisonRiver,
                    float sand, float road, float mountains, AreaInfo *info, int flags)
{
    float level = elevation;
    float roughWeight = 1.0f;
    float hillWeight = 1.0f;
    double fineY = y * 0.005;
    double fineX = x * 0.005;
    float amplitude = (noise(fineX + 3363.0, fineY + 7874.0) + 1.0f) * 25.0f - 10.0f;
    bool lowWater = false;
    if (info->type != PlaceNone) {
        float influence = info->influence;
        if (influence > 0.0001f) {
            float target = locationWaterWeight(*info);
            if (target < 10.0f) {
                target = 10.0f;
                lowWater = true;
            }
            if (target >= 1.0f) {
                float smooth = smoothStep(std::min(1.0f, influence + influence));
                int type = info->type;
                if (type == PlaceMountain) {
                    if ((flags & HeightFlagSkipPlaceAndHills) == 0) {
                        level = influence * influence * 180.0f + level;
                    }
                } else if (type == PlaceCrater) {
                    float radius = (float)info->radius;
                    float rim = std::min(influence + influence, 1.0f);
                    float inner = std::max(influence - 0.5f + (influence - 0.5f), 0.0f);
                    roughWeight = 1.0f - smooth;
                    level = rim * rim * 0.1f * radius + level;
                    level -= (1.0f - (1.0f - inner) * (1.0f - inner)) * radius * 0.25f;
                } else if (type == PlaceVolcano) {
                    float rim = std::min(influence / 0.85f, 1.0f);
                    float inner = std::max((influence - 0.85f) / 0.15f, 0.0f);
                    roughWeight = 1.0f - smooth;
                    level = rim * rim * rim * 180.0f + level;
                    level -= (1.0f - (1.0f - inner) * (1.0f - inner)) * 20.0f;
                } else if (type != PlaceManaPump && type != PlaceRock && type != PlaceCanyon && type != PlaceBeach) {
                    roughWeight = 1.0f - smooth;
                    if (amplitude > 10.0f) {
                        amplitude += (10.0f - amplitude) * smooth;
                    }
                    if (type == PlaceCastle || type == PlaceLake || type == PlaceLavaLake || type == PlaceArena) {
                        hillWeight = roughWeight;
                        level = (target - level) * smooth + level;
                    } else {
                        level = (target - level) * influence + level;
                    }
                }
            }
        }
    }
    level = std::max(-256.0f, level);
    float pathWeight = std::min(road + road, 1.0f);
    double coarseX = x * 0.015;
    float flat = (1.0f - std::min(lavaRiver + lavaRiver, 1.0f)) * (1.0f - std::min(river + river, 1.0f)) *
                 (1.0f - std::min(poisonRiver + poisonRiver, 1.0f));
    float hills = 1.0f - (1.0f - pathWeight * 0.5f) * flat * hillWeight;
    float low = (std::min)(1.0f, (std::max)(0.0f, level * 0.05f));
    double coarseY = y * 0.015;
    float hillNoise = (noise(coarseX + 4447657.0, coarseY + 87878.0) + 1.0f) * 0.5f;
    hills *= hills;
    level += hillNoise * ((1.0f - hills * hills) * low * amplitude);
    float high = (std::max)(0.0f, (std::min)(1.0f, level * 0.1f));
    double middleY = y * 0.01;
    double middleX = x * 0.01;
    float rough = noise(coarseX + 234234.0, coarseY + 12233.0);
    rough += noise(middleX + 10943.0, middleY + 98433.0);
    rough *= (1.0f - (1.0f - (1.0f - pathWeight) * flat * roughWeight)) * high;
    if (rough > 0.0f && cliffs < 0.5f) {
        float extra = 0.0f;
        if (info->type == PlaceCanyon)
            extra = std::min(1.0f, info->influence + info->influence);
        float edge = 1.0f - std::min(rough * 20.0f, 1.0f);
        float step = (std::min)(1.0f, (std::max)(0.0f, noise(fineX + 7743.0, fineY + 98834.0) + extra));
        float open = std::max(1.0f - (cliffs + cliffs), 0.0f);
        level += (1.0f - edge * edge) * open * 20.0f * step * step;
        if (rough > 0.5f) {
            edge = 1.0f - std::min((rough - 0.5f) * 20.0f, 1.0f);
            step = (std::min)(1.0f, (std::max)(0.0f, noise(fineX + 6777.0, fineY + 8887.0) + extra));
            level += (1.0f - edge * edge) * open * 20.0f * step * step;
        }
        if (rough > 0.75f) {
            edge = 1.0f - std::min((rough - 0.75f) * 20.0f, 1.0f);
            step = (std::min)(1.0f, (std::max)(0.0f, noise(fineX + 2133.0, fineY + 444.0) + extra));
            level += (1.0f - edge * edge) * open * 20.0f * step * step;
        }
    }
    float ground = level;
    if ((flags & HeightFlagSkipPlaceAndHills) == 0) {
        if (info->type != PlaceNone && (flags & HeightFlagSkipPlaceShape) == 0) {
            int kind = info->environment;
            if (kind != EnvironmentTypeNone) {
                if (kind == EnvironmentTypeRaisedPlateau) {
                    float radius = (float)info->radius;
                    float outer = radius * 0.7f;
                    float inner = radius * 0.5f;
                    if (outer * outer > info->squaredDistance) {
                        outer -= inner;
                        float t = 1.0f - (sqrtf(info->squaredDistance) - inner) / outer;
                        t = std::min(1.0f, t);
                        level += smoothStep(t) * outer * 0.75f;
                    }
                } else if (kind == EnvironmentTypeMoat && !lowWater) {
                    float radius = (float)info->radius;
                    float squaredDistance = info->squaredDistance;
                    int inner = (int)(radius * 0.3f);
                    int outer = (int)(radius * 0.75f);
                    if ((float)(outer * outer) > squaredDistance) {
                        double noiseY = y * 0.02 + 8743.0;
                        double noiseX = x * 0.02 + 98453466.0;
                        float width = noise(noiseX, noiseY) * 25.0f + 30.0f;
                        if (squaredDistance > ((float)outer - width) * ((float)outer - width)) {
                            float t = ((float)outer - sqrtf(squaredDistance)) / width;
                            level -= t * t * 100.0f;
                        } else if (squaredDistance > (float)(inner * inner)) {
                            float t = (sqrtf(squaredDistance) - (float)inner) / (noise(noiseX, noiseY) * 25.0f + 30.0f);
                            t = std::min(t, 1.0f);
                            level -= t * t * 100.0f;
                        }
                    }
                } else if (kind == EnvironmentTypeSpiralHill ||
                           (kind == EnvironmentTypeSpiralDepression && !lowWater)) {
                    float reach = (float)info->radius * 0.6f;
                    if (reach * reach > info->influence) {
                        // A spiral ramp around the place's centre: the angle of the column, turned and mirrored by the
                        // cell's hashes, and the turn it lies on.
                        plasma::Vector<2, float> offset = morphOffset(x, y);
                        float offsetX = (float)((((long long)x - info->center[0]) << FixedFractionBits) +
                                                (long long)(offset[0] * FixedUnitsPerBlock)) *
                                        (1.0f / FixedUnitsPerBlock);
                        float offsetY = (float)((((long long)y - info->center[1]) << FixedFractionBits) +
                                                (long long)(offset[1] * FixedUnitsPerBlock)) *
                                        (1.0f / FixedUnitsPerBlock);
                        float distance = sqrtf(offsetY * offsetY + offsetX * offsetX);
                        offsetX /= distance;
                        offsetY /= distance;
                        float angle = acosf(offsetX);
                        if (offsetY < 0.0f) {
                            angle = (float)(TwoPi - angle);
                        }
                        int cellX = floorDivide(x, BlocksPerPlace);
                        int cellY = floorDivide(y, BlocksPerPlace);
                        if (info->large) {
                            cellX = floorDivide(cellX, 2) * 2;
                            cellY = floorDivide(cellY, 2) * 2;
                        }
                        float turn = (float)(random(cellX + 426158892, cellY) % 1000000) / 1000000.0f;
                        angle = (float)((turn + turn) * Pi + angle);
                        if (angle > TwoPi) {
                            angle = (float)(angle - TwoPi);
                        }
                        if (random(cellX + 1897655455, cellY) & 1) {
                            angle = (float)(TwoPi - angle);
                        }
                        float pitch = (float)(random(cellX + 5127708, cellY) % 20 + 25);
                        float cap = std::min(reach / pitch * 15.0f, 150.0f);
                        float slope = reach / 6.0f;
                        float turns = ((float)(reach - angle / TwoPi * pitch) - distance) / pitch;
                        if (turns >= -0.25f) {
                            float whole = (float)floorToInt(turns);
                            float part = turns - whole - 0.75f;
                            float step;
                            if (part > 0.0f) {
                                step = smoothStep(std::min(1.0f, part * 4.0f));
                            } else {
                                step = 0.0f;
                            }
                            step += whole;
                            if (step > 0.0f) {
                                angle = (float)(step * TwoPi + angle);
                            }
                            if (turns < 0.0f) {
                                angle *= turns * 4.0f + 1.0f;
                            }
                            if (angle < 1.0f) {
                                angle *= angle;
                            }
                            float rise = sqrtf(angle) * slope;
                            rise = std::min(cap, rise);
                            if (kind == EnvironmentTypeSpiralDepression) {
                                level -= rise;
                            } else {
                                level += rise;
                            }
                        }
                    }
                }
            }
        }
        if (cliffs > 0.5f) {
            float peak = std::min(1.0f, std::max(0.0f, (cliffs - 0.75f) * 4.0f));
            float steep = cliffs - 0.5f + (cliffs - 0.5f);
            float ridge = (std::max)(0.0f, (std::min)(1.0f, (1.0f - peak) * (peak * 4.0f)));
            level = (noise(middleX + 8757.0, middleY + 9087.0) * 0.25f + 0.75f) * (steep * steep * 100.0f) + level;
            float ridgeHeight = hillRidge(x, y);
            level += ridgeHeight * ridge * 16.0f;
        }
    }
    if ((flags & HeightFlagSkipRiverBeds) == 0) {
        if (river > 0.5f) {
            float shallow = 1.0f - (river - 0.5f + (river - 0.5f));
            float depth = (std::min)(1.0f, (std::max)(0.0f, (level + 5.0f) * 0.1f));
            float bed = (noise(x * 0.025 + 35.0, y * 0.025 + 8933.0) + 1.0f) * 0.5f *
                        (1.0f - shallow * shallow * shallow) * 9.5f;
            level -= bed * depth + 2.0f;
        } else if (lavaRiver > 0.5f) {
            level += -2.0f;
        } else if (poisonRiver > 0.5f) {
            level += -2.0f;
        } else if (sand > 0.5f || road > 0.5f || isBeach(x, y, level)) {
            level += -1.0f;
            if (cliffs > 0.5f) {
                level += -1.0f;
            }
        }
    }
    if ((flags & HeightFlagCaveMouths) != 0 && cliffs > 0.5f) {
        float density = caveDensity(x, y, info->type, info->influence);
        if (density > 0.0f) {
            float middle = (noise(middleX + 7474.0, middleY + 98984.0) + 1.0f) * 20.0f + ground;
            float scale = noise(x * 0.02 + 8744.0, y * 0.02 + 904.0) * 0.025f + 0.06f;
            float z = level;
            while (z > ground) {
                z = (float)floorToInt(z);
                float t = (float)fabs((z - middle) * scale);
                t = std::min(t, 1.0f);
                float solid = 1.0f - (1.0f - t * t) * density;
                if (smoothStep(solid) * ((level - z) * 0.1f + 0.5f) > 0.5f) {
                    break;
                }
                z += -1.0f;
            }
            return z;
        }
    }
    return level;
}

// Ridges of two absolute noises (frequency 0.08, offset 432; 0.05 at 0.04 weight).
float World::hillRidge(int x, int y)
{
    float result = 0.0f;
    double fine = noise(x * 0.08 + 432.0, y * 0.08 + 432.0) * 0.05;
    float ridge = 1.0 - fabs(fine + noise(x * 0.04 + 432.0, y * 0.04 + 432.0));
    result += (1.0f - ridge) * (1.0f - ridge);
    return result;
}

// A house takes 2 x 2 of its plot's 3 x 3 cells, in the corner drawn by a hash of the plot (salt 369738); housePart
// gives each of the four cells its quarter.
plasma::Vector<2, int> World::houseCorner(int x, int y)
{
    switch (random(x + 369738, y) & 3) {
    case 1:
        return plasma::Vector<2, int>(x * 2 + 1 + x, y * 3);
    case 2:
        return plasma::Vector<2, int>(x * 2 + 1 + x, y * 2 + 1 + y);
    case 3:
        return plasma::Vector<2, int>(x * 3, y * 2 + 1 + y);
    }
    return plasma::Vector<2, int>(x * 3, y * 3);
}

int World::housePart(int x, int y)
{
    int cellX = floorDivide(x, 3);
    int cellY = floorDivide(y, 3);
    int corner = random(cellX + 369738, cellY) & 3;
    if (x == cellX * 3 && y == cellY * 3) {
        return (4 - corner) % 4;
    }
    if (x == cellX * 3 + 1) {
        if (y == cellY * 3) {
            return (5 - corner) % 4;
        }
        if (y == cellY * 3 + 1) {
            return (6 - corner) % 4;
        }
    }
    if (x == cellX * 3 && y == cellY * 2 + 1 + cellY) {
        return (7 - corner) % 4;
    }
    return -1;
}

// Humidity: the dry, savannah and wet biomes' weights (BiomeBlendRate) plus noise (0.0002; offsets 6654, 98883) at
// 0.35, scaled by 1.5 about 0.4.
float World::BiomeSample::humidity()
{
    float nearestDryBiomeSquaredDistance = -1.0f;
    float nearestWetBiomeSquaredDistance = -1.0f;
    float nearestSavannahSquaredDistance = -1.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if ((biomes[i][j] == BiomeDesert || biomes[i][j] == BiomeFirelands) &&
                (nearestDryBiomeSquaredDistance < 0.0f || nearestDryBiomeSquaredDistance > squaredDistances[i][j])) {
                nearestDryBiomeSquaredDistance = squaredDistances[i][j];
            }
            if (biomes[i][j] == BiomeSavannah &&
                (nearestSavannahSquaredDistance < 0.0f || nearestSavannahSquaredDistance > squaredDistances[i][j])) {
                nearestSavannahSquaredDistance = squaredDistances[i][j];
            } else if ((biomes[i][j] == BiomeJungle || biomes[i][j] == BiomeWetlands) &&
                       (nearestWetBiomeSquaredDistance < 0.0f ||
                        nearestWetBiomeSquaredDistance > squaredDistances[i][j])) {
                nearestWetBiomeSquaredDistance = squaredDistances[i][j];
            }
        }
    }
    float dryBiomeWeight = weight(nearestDryBiomeSquaredDistance, BiomeBlendRate);
    float savannahWeight = weight(nearestSavannahSquaredDistance, BiomeBlendRate);
    float wetBiomeWeight = weight(nearestWetBiomeSquaredDistance, BiomeBlendRate);
    return (std::max)(0.0f, (std::min)(1.0f, noise(x * 0.0002 + 6654.0, y * 0.0002 + 98883.0) * 0.35f +
                                                 (wetBiomeWeight * 1.5f + 0.5f) - savannahWeight * 0.4f -
                                                 dryBiomeWeight * 2.0f));
}

// Points that (nearly) coincide are in line; the ray may stop a hundredth of a block short.
bool World::inClearLine(const plasma::Vector<3, Fixed> &a, const plasma::Vector<3, Fixed> &b, bool throughSolid,
                        bool stopAtBarriers, float range, Creature *other)
{
    if (distanceExceeds(a, b, range)) {
        return false;
    }
    plasma::Vector<3, float> direction = plasma::Vector<3, float>(b - a);
    float squaredLength = direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2];
    if (squaredLength < 0.0001f) {
        return true;
    }
    float length = sqrtf(squaredLength);
    direction /= length;
    return cameraRay(a, direction, length, throughSolid, stopAtBarriers, other) >= length - 0.01f;
}

bool World::inRange(const plasma::Vector<3, Fixed> &a, const plasma::Vector<3, Fixed> &b, bool stopAtBarriers,
                    float range, Creature *other)
{
    if (distanceExceeds(a, b, range)) {
        return false;
    }
    plasma::Vector<3, float> direction = plasma::Vector<3, float>(b - a);
    float squaredLength = direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2];
    if (squaredLength < 0.0001f) {
        return true;
    }
    float length = sqrtf(squaredLength);
    direction /= length;
    return rayDistance(a, direction, length, false, stopAtBarriers, other) >= length - 0.01f;
}

void World::initCompanion(long long ownerId, Creature *companion)
{
    srand(companion->m_entityData.race);
    fillAppearance(companion->m_entityData.race, companion->m_entityData.appearance, nullptr, false);
    companion->equipByRace();
    companion->m_entityData.ownerId = ownerId;
    companion->m_behaviours = BehaviourFollowOwner;
    Creature *owner = creatureByID(ownerId);
    if (owner) {
        owner->m_petId = companion->m_id;
    }
}

// Three or four floors by a hash of the place (salt 5611612).
int initialFloors(int x, int y)
{
    switch (random(x + 5611612, y) % 3) {
    case 1:
        return 4;
    case 2:
        return 3;
    }
    return 2;
}

void installNoise()
{
    for (int i = 0; i < 512; i++) {
        perm[i] = p[i & 255];
        permMod12[i] = perm[i] % 12;
    }
}

// A beach: below 20 blocks where noise (0.02; offsets 7743, 8977) puts the shore at up to ten blocks.
bool World::isBeach(int x, int y, float height)
{
    if (height >= 20.0f) {
        return false;
    }
    if (height <= 0.0f) {
        return true;
    }
    return (noise(x * 0.02 + 7743.0, y * 0.02 + 8977.0) + 1.0f) * 10.0f > height;
}

// The callers apply it to the block getBlock returns.
static inline bool blocksBody(const Block &block, bool barriers, bool passesGates)
{
    return block.isSolid() ||
           (barriers && (block.flags & BlockFlagBarrier) && (!passesGates || !(block.flags & BlockFlagSpiritPassable)));
}

// The converse for a ray, without gates.
static inline bool letsBodyPass(const Block &block, bool barriers)
{
    return !block.isSolid() && !(barriers && (block.flags & BlockFlagBarrier));
}

bool World::isBlocked(const plasma::Vector<3, Fixed> &position, const plasma::Vector<3, float> &size,
                      bool ignorePassable, bool testDecorations)
{
    plasma::Vector<3, int> first = fixedToBlock(position - plasma::Vector<3, Fixed>(size * 0.5f));
    plasma::Vector<3, int> last = fixedToBlock(position + plasma::Vector<3, Fixed>(size * 0.5f));
    for (int x = first[0]; x <= last[0]; ++x) {
        for (int y = first[1]; y <= last[1]; ++y) {
            for (int z = first[2]; z <= last[2]; ++z) {
                if (x > first[0] && x < last[0] && y > first[1] && y < last[1] && z > first[2] && z < last[2])
                    continue;
                if (blocksBody(getBlock(x, y, z), testDecorations, ignorePassable))
                    return true;
            }
        }
    }
    return false;
}

bool isCorridor(int x, int y, int direction, int startX, int startY, int regionX, int regionY)
{
    if (direction == DungeonDirectionWest) {
        if (x <= startX) {
            int a = dungeonWayY(x, startX, startY, regionX, regionY);
            int b = dungeonWayY(x - 1, startX, startY, regionX, regionY);
            if (a > b) {
                std::swap(a, b);
            }
            return y >= a && y <= b;
        }
    } else if (direction == DungeonDirectionEast) {
        if (x >= startX) {
            int a = dungeonWayY(x, startX, startY, regionX, regionY);
            int b = dungeonWayY(x + 1, startX, startY, regionX, regionY);
            if (a > b) {
                std::swap(a, b);
            }
            return y >= a && y <= b;
        }
    } else if (direction == DungeonDirectionSouth) {
        if (y <= startY) {
            int a = dungeonWayX(y, startX, startY, regionX, regionY);
            int b = dungeonWayX(y - 1, startX, startY, regionX, regionY);
            if (a > b) {
                std::swap(a, b);
            }
            return x >= a && x <= b;
        }
    } else if (direction == DungeonDirectionNorth) {
        if (y >= startY) {
            int a = dungeonWayX(y, startX, startY, regionX, regionY);
            int b = dungeonWayX(y + 1, startX, startY, regionX, regionY);
            if (a > b) {
                std::swap(a, b);
            }
            return x >= a && x <= b;
        }
    }
    return false;
}

bool World::isCourtType(int type)
{
    return type == BuildingCastleCell || type == BuildingCastleGate || type == BuildingHouseEntrance;
}

bool World::isCoveredType(int type)
{
    return (type >= BuildingHouseRoom && type <= BuildingHouseCourtyard) || type == BuildingFlightMasterSite;
}

bool World::isFlyingFar()
{
    Creature *creature = m_localCreature;
    if (creature != nullptr && creature->m_entityData.currentAbility == AbilityEagleFlight) {
        // Raw differences stored in two-element long long vectors.
        plasma::Vector<2, long long> target((long long)creature->m_entityData.destination[0] << FixedFractionBits,
                                            (long long)creature->m_entityData.destination[1] << FixedFractionBits);
        plasma::Vector<2, long long> d(creature->m_entityData.position[0].m_raw - target[0],
                                       creature->m_entityData.position[1].m_raw - target[1]);
        plasma::Vector<2, double> distance((double)d[0] / FixedUnitsPerBlock, (double)d[1] / FixedUnitsPerBlock);
        if (distance.length() > FarFlightRemaining) {
            plasma::Vector<2, long long> s(creature->m_entityData.origin[0].m_raw - target[0],
                                           creature->m_entityData.origin[1].m_raw - target[1]);
            plasma::Vector<2, double> flown((double)s[0] / FixedUnitsPerBlock, (double)s[1] / FixedUnitsPerBlock);
            if (flown.length() > FarFlightLength) {
                return true;
            }
        }
    }
    return false;
}

bool World::isGroundItemAvailable(const ZoneItem &item)
{
    if (item.item.category == ItemCategoryNone || item.quantity == 0)
        return false;
    if (m_localCreature) {
        if (item.item.category == ItemCategoryArtifact && m_localCreature->hasItem(item.item, 1))
            return false;
        if (item.item.category == ItemCategoryGeneratedItem) {
            PlayerData *player = m_localCreature->m_playerData;
            if (player) {
                std::array<int, 2> cell = {{tileCoordinate(item.position[0]), tileCoordinate(item.position[1])}};
                if (player->discoveredMarkers.find(cell) != player->discoveredMarkers.end())
                    return false;
            }
        }
        if (item.item.category == ItemCategorySpecial && m_localCreature->hasSpecialItem(item.item.id))
            return false;
        if (item.ownerSeed != 0 && item.ownerSeed != m_localCreature->m_entityData.seed)
            return false;
    }
    return true;
}

// A hall ends at the place's edge room: the room index of the first or last block of the place (the place index
// shifted by 10: BlocksPerPlace; 2047 the last block of a large place), BlocksPerDungeonRoom blocks per room;
// isHallNearEnd one room before.
bool isHallEnd(int x, int y, int direction, int startX, int startY, int regionX, int regionY)
{
    if (direction == DungeonDirectionWest) {
        if (x == floorDivide(regionX << 10, BlocksPerDungeonRoom)) {
            return y == dungeonWayY(x - 1, startX, startY, regionX, regionY);
        }
    } else if (direction == DungeonDirectionSouth) {
        if (y == floorDivide(regionY << 10, BlocksPerDungeonRoom)) {
            return x == dungeonWayX(y - 1, startX, startY, regionX, regionY);
        }
    } else if (direction == DungeonDirectionEast) {
        if (x == floorDivide((regionX << 10) + 2047, BlocksPerDungeonRoom)) {
            return y == dungeonWayY(x + 1, startX, startY, regionX, regionY);
        }
    } else if (direction == DungeonDirectionNorth) {
        if (y == floorDivide((regionY << 10) + 2047, BlocksPerDungeonRoom)) {
            return x == dungeonWayX(y + 1, startX, startY, regionX, regionY);
        }
    }
    return false;
}

// Tests for a static entity of the requested type within eight blocks of the player, optionally requiring it to be
// active.
bool World::isHallNear(int type, bool requireActive)
{
    if (!m_localCreature)
        return false;
    int firstX = floorDivide((m_localCreature->m_entityData.position[0] - Fixed(8)).toBlock(), BlocksPerZone);
    int lastX = floorDivide((m_localCreature->m_entityData.position[0] + Fixed(8)).toBlock(), BlocksPerZone);
    int firstY = floorDivide((m_localCreature->m_entityData.position[1] - Fixed(8)).toBlock(), BlocksPerZone);
    int lastY = floorDivide((m_localCreature->m_entityData.position[1] + Fixed(8)).toBlock(), BlocksPerZone);
    for (int x = firstX; x <= lastX; ++x) {
        for (int y = firstY; y <= lastY; ++y) {
            Zone *zone = this->zone(x, y);
            if (!zone)
                continue;
            for (StaticEntity &entity : zone->m_staticEntities) {
                if (entity.type != type || (!entity.active && requireActive))
                    continue;
                plasma::Vector<3, float> delta(entity.position - m_localCreature->m_entityData.position);
                if (delta.squaredLength() <= 64.0f)
                    return true;
            }
        }
    }
    return false;
}

bool isHallNearEnd(int x, int y, int direction, int startX, int startY, int regionX, int regionY)
{
    if (direction == DungeonDirectionWest) {
        if (x == floorDivide(regionX << 10, BlocksPerDungeonRoom) + 1) {
            return y == dungeonWayY(x - 1, startX, startY, regionX, regionY);
        }
    } else if (direction == DungeonDirectionSouth) {
        if (y == floorDivide(regionY << 10, BlocksPerDungeonRoom) + 1) {
            return x == dungeonWayX(y - 1, startX, startY, regionX, regionY);
        }
    } else if (direction == DungeonDirectionEast) {
        if (x == floorDivide((regionX << 10) + 2047, BlocksPerDungeonRoom) - 1) {
            return y == dungeonWayY(x + 1, startX, startY, regionX, regionY);
        }
    } else if (direction == DungeonDirectionNorth) {
        if (y == floorDivide((regionY << 10) + 2047, BlocksPerDungeonRoom) - 1) {
            return x == dungeonWayX(y + 1, startX, startY, regionX, regionY);
        }
    }
    return false;
}

bool World::isJoined(int x, int y, int otherX, int otherY)
{
    int type = zoneBuildingType(x, y);
    int otherType = zoneBuildingType(otherX, otherY);
    int height = structureLevel(x, y, type);
    int otherHeight = structureLevel(otherX, otherY, otherType);
    if (type == otherType && height + buildingHeight(type) == otherHeight + buildingHeight(otherType)) {
        return true;
    }
    return false;
}

bool World::isOccupiedStorey(int zoneX, int zoneY, int storey)
{
    plasma::Vector<2, int> point = streetPoint(zoneX, zoneY);
    AreaInfo info;
    fillAreaInfo(point[0], point[1], info);
    int type = streetBuildingType(zoneX, zoneY, point, info);
    if (storey >= buildingStoreys(type)) {
        return false;
    }
    if (type == BuildingCastleCell) {
        int cellX = floorDivide(floorDivide(zoneX * BlocksPerCell, BlocksPerPlace), 2) * 2;
        int cellY = floorDivide(floorDivide(zoneY * BlocksPerCell, BlocksPerPlace), 2) * 2;
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        int direction = castleDirection(cellX, cellY);
        int rotation = castleRotation(cellX, cellY);
        int size = castleSize(cellX, cellY);
        return castlePart(zoneX, zoneY, direction, rotation, size, centerX, centerY) != 0;
    }
    return true;
}

bool World::isPlotInner(int plotX, int plotY, int type, const plasma::Vector<2, int> &zone)
{
    if (plotX == floorDivide(zone[0], CellsPerPlot) && plotY == floorDivide(zone[1], CellsPerPlot)) {
        return false;
    }
    int blockY = plotY * BlocksPerPlot + BlocksPerCell;
    int blockX = plotX * BlocksPerPlot + BlocksPerCell;
    int cellX = floorDivide(blockX, BlocksPerPlace);
    int cellY = floorDivide(blockY, BlocksPerPlace);
    bool large = isLargePlace(cellX, cellY);
    float squaredDistance = placeSquaredDistance(placeCenter(cellX, cellY, large), blockX, blockY);
    if (type != 0) {
        int radius = large ? LargePlaceRadius : SmallPlaceRadius;
        float squaredRadius = (float)(radius * radius);
        if (!(squaredDistance >= squaredRadius) &&
            !((1.0f - squaredDistance / squaredRadius) * (1.0f - squaredDistance / squaredRadius) < 0.5f)) {
            return true;
        }
    }
    return false;
}

ZoneItem *World::item(const plasma::Vector<3, int> &id)
{
    Zone *zone = this->zone(id[0], id[1]);
    if (zone == nullptr) {
        return nullptr;
    }
    int index = id[2];
    if (index < 0 || index >= (int)zone->m_items.size()) {
        return nullptr;
    }
    return &zone->m_items[index];
}

// A land's biome by a hash of the land (salt 737112), one of fifty.
BiomeIdentifier World::landBiome(int x, int y)
{
    if (x >= -1 && x <= 1 && y >= -1 && y <= 1) {
        return BiomeHills;
    }
    BiomeIdentifier biome = BiomeOcean;
    switch (random(x + 737112, y) % 50) {
    case 1:
    case 2:
    case 3:
    case 4:
        biome = BiomeHills;
        break;
    case 5:
    case 6:
    case 7:
        biome = BiomePlains;
        break;
    case 8:
    case 9:
        biome = BiomeDesert;
        break;
    case 10:
    case 11:
        biome = BiomeSavannah;
        break;
    case 12:
    case 13:
        biome = BiomeJungle;
        break;
    case 14:
        biome = BiomeFirelands;
        break;
    case 15:
    case 16:
        biome = BiomeSnowlands;
        break;
    case 17:
    case 18:
        biome = BiomeSnowForest;
        break;
    case 19:
    case 20:
    case 21:
    case 22:
        biome = BiomeWoodlands;
        break;
    case 23:
    case 24:
    case 25:
        biome = BiomeWetlands;
        break;
    case 26:
    case 27:
        biome = BiomeMountains;
        break;
    case 28:
    case 29:
        biome = BiomeDeadlands;
        break;
    case 30:
    case 31:
        biome = BiomeDarkWoods;
        break;
    }
    return biome;
}

plasma::Vector<2, int> World::landCenter(int x, int y)
{
    // A hashed point (salts 58014, 460082) of the land (its index shifted by 13: BlocksPerLand) at least
    // LandCenterMargin inside it.
    return plasma::Vector<2, int>(
        (x << 13) + LandCenterMargin + random(x + 58014, y) % (BlocksPerLand - 2 * LandCenterMargin),
        random(x + 460082, y) % (BlocksPerLand - 2 * LandCenterMargin) + (y << 13) + LandCenterMargin);
}

// Lava channels (a hand-tuned noise profile): three noises (frequencies 0.0023, 0.00045, 0.01 at weights 0.5, 1, 0.1)
// ridged by 5 and shaped above 0.5, scaled by the firelands beyond 0.5; lava lakes from influence 0.4 (over 0.6),
// volcanoes within 0.3 of their radius, elemental rings between 0.75 and 1 of it, suppressed on roads (0.4) and in
// other places by 10 times their influence.
float World::lavaChannels(int x, int y, float road, float firelands, float mountains, AreaInfo *info)
{
    float result = 0.0f;
    if (firelands > 0.5f) {
        float n = noise(x * 0.0023 + 330003.0, y * 0.0023 + 323322.0) * 0.5f;
        n += noise(x * 0.00045 + 88989.0, y * 0.00045 + 1001.0);
        n += noise(x * 0.01 + 558587.0, y * 0.01 + 198743.0) * 0.1f;
        float ridge = fabs(n) * 5.0f;
        if (ridge > 0.5f) {
            ridge = (ridge - 0.5f) * 0.5f;
            ridge = ridge * ridge * ridge + 0.5f;
        }
        float peak = std::max(1.0f - ridge, 0.0f);
        result = peak * peak * ((firelands - 0.5f) * 2.0f) * (1.0f - mountains);
    }
    if (info->type != PlaceNone) {
        if (info->type == PlaceLavaLake) {
            result += (1.0f - result) * (std::max)(0.0f, (info->influence - 0.4f) / 0.6f);
        } else if (info->type == PlaceVolcano) {
            float inner = (info->radius * 0.3f) * (info->radius * 0.3f);
            if (inner > info->squaredDistance)
                result += 1.0f - info->squaredDistance / inner;
        } else {
            float outside = std::max(0.0f, (info->influence - 0.1f) * 10.0f);
            outside = 1.0f - std::min(1.0f, outside);
            result *= 1.0f - (1.0f - outside * outside);
            if (info->environment == EnvironmentTypeElementalRing) {
                if (landBiome(findRegion(info->center[0], info->center[1])) == BiomeFirelands) {
                    if ((float)(info->radius * info->radius) > info->squaredDistance) {
                        float radius = (float)info->radius;
                        float edge = (float)fabs((sqrtf(info->squaredDistance) - radius * 0.75f) / (radius * 0.25f));
                        edge = std::min(edge, 1.0f);
                        result += (1.0f - edge * edge) * (1.0f - edge * edge) * (1.0f - result);
                    }
                }
            }
        }
    }
    float high = std::max(0.0f, (road - 0.4f) * 10.0f);
    high = std::min(1.0f, high);
    return (1.0f - high) * result;
}

// Lava's colour: red 255, green 50 +- 20 by two noises (0.1, 0.05).
plasma::Vector<3, float> World::lavaColor(int x, int y)
{
    return plasma::Vector<3, float>(255.0f, (noise(x * 0.1, y * 0.1) + noise(x * 0.05, y * 0.05)) * 50.0f + 50.0f,
                                    20.0f);
}

float World::BiomeSample::lavaWeight(int areaType, float areaInfluence)
{
    float distance = -1.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (biomes[i][j] == BiomeFirelands && (distance < 0.0f || distance > squaredDistances[i][j])) {
                distance = squaredDistances[i][j];
            }
        }
    }
    float result = 0.0f;
    if (distance >= 0.0f && distance >= nearestSquaredDistance) {
        result = std::max(1.0f - (distance - nearestSquaredDistance) * BiomeBlendRate * BiomeBlendRate, 0.0f);
        if (!(1.0f > result)) {
            return result;
        }
    }
    if (areaType == PlaceCrater) {
        return std::min(1.0f, areaInfluence + areaInfluence + result);
    }
    if (areaType == PlaceVolcano) {
        return std::min(1.0f, result + areaInfluence);
    }
    return result;
}

void World::leaveZoneMeshes()
{
    LeaveCriticalSection(&m_zoneMeshCriticalSection);
}

void World::leaveZones()
{
    LeaveCriticalSection(&m_zonesLock);
}

void World::loadState()
{
    BytesIO data;
    if (m_database.read("state", data)) {
        Serializer(&data).loadWorldState(m_state);
    }
}

// The type for a place whose centre lies at the given marine weight: on land the given type, in shallow water (above
// -10) the type 36, deeper the other one.
static inline int coastType(float sea, int land, int deep)
{
    // Tests the land branch first.
    if (!(sea < 0.0f))
        return land;
    if (sea > -10.0f)
        return PlaceBeach;
    return deep;
}

// The kind of place of a 1024-block cell, from a hash of the cell: 1 at the origin; a large place (a 2 x 2 group of
// cells) is 5 in one of seven groups and otherwise one of 19 kinds; a small one exists in three of four cells (not at
// sea in half of them) and is one of 24 kinds. Several kinds depend on the biome of the place's centre and on how far
// it lies from the sea. The places: a lattice of large ones (one cell in 7 along x + 5y, salt 2805729 and 19 weights),
// small ones by a hash (salt 2805729, 4 ways, 24 weights), lakes of all kinds at 10 blocks up at least, canyons at 50.
int World::locationType(int x, int y, bool large)
{
    if (x == 0 && y == 0)
        return PlaceHouse;
    if (large) {
        x = floorDivide(x, 2) * 2;
        y = floorDivide(y, 2) * 2;
        if (((floorDivide(x, 2) + floorDivide(y, 2) * 5) % 7 + 7) % 7 == 1)
            return PlaceVillage;
        switch ((int)random(x + 2805729, y) % 19) {
        case 0:
            return PlaceMountain;
        case 1: {
            plasma::Vector<2, int> center = placeCenter(x, y, isLargePlace(x, y));
            plasma::Vector<2, int> region = findNearestRegion(center[0], center[1]);
            switch (landBiome(region[0], region[1])) {
            case BiomeJungle:
                return PlaceTemple;
            case BiomeDesert:
                return PlacePyramid;
            case BiomeSavannah:
                return PlaceTemple;
            }
            return PlaceCastle;
        }
        case 2:
            return PlaceArena;
        case 3:
            return PlaceAncientRuins;
        case 4:
            return PlaceMines;
        case 5:
            return PlaceGrotto;
        case 6:
            return PlaceVault;
        case 7:
            return PlaceVolcano;
        case 8:
            return PlaceRock;
        case 9: {
            plasma::Vector<2, int> center = placeCenter(x, y, large);
            if (marineWeight(center[0], center[1]) < 10.0f)
                return PlaceNone;
            return PlaceLavaLake;
        }
        case 10: {
            bool big = isLargePlace(x, y);
            plasma::Vector<2, int> center = placeCenter(x, y, big);
            if (marineWeight(center[0], center[1]) < 10.0f)
                return PlaceNone;
            plasma::Vector<2, int> place = placeCenter(x, y, big);
            plasma::Vector<2, int> region = findNearestRegion(place[0], place[1]);
            return (landBiome(region[0], region[1]) == BiomeFirelands) + PlaceLake;
        }
        case 11:
            return PlaceGraveyard;
        case 12: {
            plasma::Vector<2, int> center = placeCenter(x, y, isLargePlace(x, y));
            plasma::Vector<2, int> region = findNearestRegion(center[0], center[1]);
            int biome = landBiome(region[0], region[1]);
            if (biome >= BiomeWoodlands && biome <= BiomeSnowForest) {
                plasma::Vector<2, int> place = placeCenter(x, y, large);
                return coastType(marineWeight(place[0], place[1]), PlaceGlade, PlaceIsland);
            }
            return PlaceForest;
        }
        case 13: {
            plasma::Vector<2, int> center = placeCenter(x, y, large);
            if (marineWeight(center[0], center[1]) < 50.0f)
                return PlaceNone;
            return PlaceCanyon;
        }
        case 14:
            return PlaceRocks;
        case 15: {
            plasma::Vector<2, int> center = placeCenter(x, y, large);
            return coastType(marineWeight(center[0], center[1]), PlacePlain, PlaceIsland);
        }
        case 16: {
            plasma::Vector<2, int> center = placeCenter(x, y, large);
            return coastType(marineWeight(center[0], center[1]), PlaceHill, PlaceIsland);
        }
        case 17:
            return PlaceDarkForest;
        case 18:
            return PlaceEnchantedForest;
        }
        return PlaceNone;
    }
    int hash = (int)random(x + 2805729, y);
    if (hash % 4 == 0)
        return PlaceNone;
    hash /= 4;
    if ((hash & 1) == 0) {
        plasma::Vector<2, int> center = placeCenter(x, y, isLargePlace(x, y));
        if (marineWeight(center[0], center[1]) < 10.0f)
            return PlaceNone;
    }
    switch (hash / 2 % 24) {
    case 0:
        return PlaceTower;
    case 1:
        return PlaceManaPump;
    case 2: {
        const plasma::Vector<2, int> &center = placeCenter(x, y, false);
        plasma::Vector<2, int> region = findNearestRegion(center[0], center[1]);
        return (landBiome(region[0], region[1]) == BiomeFirelands) + PlaceTree;
    }
    case 3:
        return PlaceRock;
    case 4:
        return PlaceOldHut;
    case 5:
        return PlaceHouse;
    case 6:
    case 20:
        return PlaceGraveyard;
    case 7: {
        plasma::Vector<2, int> center = placeCenter(x, y, false);
        return coastType(marineWeight(center[0], center[1]), PlaceCrater, PlaceNone);
    }
    case 8: {
        bool big = isLargePlace(x, y);
        plasma::Vector<2, int> center = placeCenter(x, y, big);
        float sea = marineWeight(center[0], center[1]);
        if (sea < 0.0f)
            return coastType(sea, PlaceNone, PlaceNone);
        plasma::Vector<2, int> place = placeCenter(x, y, big);
        plasma::Vector<2, int> region = findNearestRegion(place[0], place[1]);
        return (landBiome(region[0], region[1]) == BiomeFirelands) + PlaceLake;
    }
    case 9: {
        plasma::Vector<2, int> center = placeCenter(x, y, false);
        return coastType(marineWeight(center[0], center[1]), PlaceLavaLake, PlaceNone);
    }
    case 10:
        return PlaceFarm;
    case 11:
        return PlaceCircleOfPower;
    case 12:
        return PlaceRocks;
    case 13:
        return PlaceCrypt;
    case 14: {
        plasma::Vector<2, int> center = placeCenter(x, y, false);
        return coastType(marineWeight(center[0], center[1]), PlacePlain, PlaceIsland);
    }
    case 15: {
        plasma::Vector<2, int> center = placeCenter(x, y, isLargePlace(x, y));
        plasma::Vector<2, int> region = findNearestRegion(center[0], center[1]);
        int biome = landBiome(region[0], region[1]);
        if (biome < BiomeWoodlands || biome > BiomeSnowForest || biome == BiomeJungle)
            return PlaceGrove;
        plasma::Vector<2, int> place = placeCenter(x, y, false);
        return coastType(marineWeight(place[0], place[1]), PlaceGlade, PlaceIsland);
    }
    case 16:
        return PlaceGrove;
    case 17: {
        plasma::Vector<2, int> center = placeCenter(x, y, false);
        return coastType(marineWeight(center[0], center[1]), PlaceHill, PlaceIsland);
    }
    case 18:
        return PlaceCamp;
    case 19:
        return PlaceAncientRuins;
    case 21:
        return PlaceSanctuary;
    case 22:
        return PlaceAltar;
    case 23:
        return PlaceMenhir;
    }
    return PlaceNone;
}

float World::locationWaterWeight(const AreaInfo &info)
{
    BiomeSample sample;
    biomeSample(info.center[0], info.center[1], sample);
    return sample.biomeElevation(info.type, 1.0f);
}

float World::marineWeight(int x, int y)
{
    BiomeSample sample;
    biomeSample(x, y, sample);
    return sample.biomeElevation(0, 0.0f);
}

// The masonry floors (hand-authored shapes): spiral and plateau rises (0.6, 0.7, 0.75 of the radius; a hashed spiral,
// salt 5127708), a pyramid entrance strip, the arena's centre, rim (radius 135 squared 18225 / 133 17689 / 134 17956)
// and aisles.
int World::masonryFloor(int x, int y, const AreaInfo &info, const plasma::Vector<2, int> &zone, float height,
                        float base)
{
    if (info.environment == EnvironmentTypeSpiralHill) {
        int cellX = floorDivide(x, BlocksPerPlace);
        int cellY = floorDivide(y, BlocksPerPlace);
        if (info.large) {
            cellX = floorDivide(cellX, 2) * 2;
            cellY = floorDivide(cellY, 2) * 2;
        }
        float raise = info.radius * 0.6f / (random(cellX + 5127708, cellY) % 20 + 25) * 15.0f;
        base += std::min(raise, 150.0f);
    } else if (info.environment == EnvironmentTypeRaisedPlateau) {
        base += (info.radius * 0.7f - info.radius * 0.5f) * 0.75f;
    }
    if (info.type == PlacePyramid && abs(x - zone[0] * 16) < 8 && y < (zone[1] + 1) * 16) {
        return (int)(base + 15.0f);
    }
    if (info.type == PlaceArena) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        int squaredDistance =
            (centerY * 16 - y + 8) * (centerY * 16 - y + 8) + (centerX * 16 - x + 8) * (centerX * 16 - x + 8);
        if (squaredDistance < 18225) {
            if (squaredDistance < 3600)
                return floorToInt(height);
            if (squaredDistance >= 17956)
                return (int)(base + 30.0f);
            if (squaredDistance >= 17689)
                return (int)(base + 29.0f);
            if (floorDivide(x, BlocksPerCell) == centerX || floorDivide(y, BlocksPerCell) == centerY)
                return floorToInt(base) + 12;
        }
    }
    return floorToInt(height);
}

// The pavement palettes of arenas (sand within 60 blocks, squared 3600), pyramids and temples; the bricks brightened
// by noise as StructureBuilder::brickColor (strides 154, 53; above 0.7, up to 10%).
plasma::Vector<3, float> World::masonryPavement(int x, int y, int z, const AreaInfo &info)
{
    float red;
    float green;
    float blue;
    switch (info.type) {
    case PlaceArena: {
        int centerX = floorDivide(info.center[0], BlocksPerCell) * 16 + 8;
        int dy = y - floorDivide(info.center[1], BlocksPerCell) * 16 - 8;
        if ((x - centerX) * (x - centerX) + dy * dy < 3600) {
            return sandColor(x, y);
        }
        red = 180.0f;
        green = 170.0f;
        blue = 120.0f;
        break;
    }
    case PlacePyramid:
        red = 260.0f;
        green = 250.0f;
        blue = 130.0f;
        break;
    case PlaceTemple:
        red = 180.0f;
        green = 170.0f;
        blue = 90.0f;
        break;
    default:
        return plasma::Vector<3, float>(255.0f, 255.0f, 255.0f);
    }
    float shade = noise((x + z) / 2 * 154 + z * 53, (y + z) / 2 * 154 + z * 53);
    if (shade > 0.7f) {
        float factor = (shade - 0.7f) / 0.3f * 0.1f + 1.0f;
        red *= factor;
        green *= factor;
        blue *= factor;
    }
    return plasma::Vector<3, float>(red, green, blue);
}

// The place names are seeded by hashes of the place (salt 380529; 583478 for the owner race, one of 15).
int metroNameSeed(int x, int y)
{
    return random(x + 380529, y);
}

int metroNameVariant(int x, int y, int type)
{
    switch (type) {
    case PlaceHouse:
    case PlaceFarm:
    case PlaceInn:
        switch (random(x + 583478, y) & 15) {
        case 1:
            return RaceElfFemale;
        case 2:
            return RaceHumanMale;
        case 3:
            return RaceHumanFemale;
        case 4:
            return RaceGoblinMale;
        case 5:
            return RaceGoblinFemale;
        case 6:
            return RaceLizardmanMale;
        case 7:
            return RaceLizardmanFemale;
        case 8:
            return RaceDwarfMale;
        case 9:
            return RaceDwarfFemale;
        case 10:
            return RaceOrcMale;
        case 11:
            return RaceOrcFemale;
        case 12:
            return RaceFrogmanMale;
        case 13:
            return RaceFrogmanFemale;
        case 14:
            return RaceUndeadMale;
        case 15:
            return RaceUndeadFemale;
        }
        return RaceElfMale;
    case PlaceTower:
        switch (random(x + 583478, y) & 1) {
        case 1:
            return RaceSorcerer;
        }
        return RaceWizard;
    case PlaceOldHut:
        return RaceWitch;
    }
    return RaceNone;
}

int World::missionTargetType(int type, int x, int y, bool large)
{
    switch (type) {
    case PlaceLavaLake:
    case PlaceVolcano:
    case PlaceIsland:
    case PlaceForest:
    case PlaceCanyon:
    case PlaceHill:
        switch (random(x + 11408345, y) & 3) {
        case 1:
            return (random(x + 5703892, y) & 1) + BuildingSanctuary;
        }
        break;
    case PlaceMountain:
        switch (random(x + 11408345, y) & 3) {
        case 1:
            return (random(x + 5703892, y) & 1) + 69;
        case 2:
            return BuildingMountainPlatform;
        }
        break;
    case PlaceLake:
        switch (random(x + 11408345, y) & 3) {
        case 1:
            return (random(x + 5703892, y) & 1) + 69;
        case 2:
            return BuildingLakePlatform;
        }
        break;
    }
    return BuildingNone;
}

// The monuments' tops (hand-authored shapes): spirals and plateaus as masonryFloor, the arena's walls and steps
// (radius 128, squared 16384), the temple's stepped footprint (24, 12, 6 cells; 3 per step) to an apex of 100.
int World::monumentTop(int x, int y, const AreaInfo &info, const plasma::Vector<2, int> &zone, float height, float base)
{
    if (info.environment == EnvironmentTypeSpiralHill) {
        int cellX = floorDivide(x, BlocksPerPlace);
        int cellY = floorDivide(y, BlocksPerPlace);
        if (info.large) {
            cellX = floorDivide(cellX, 2) * 2;
            cellY = floorDivide(cellY, 2) * 2;
        }
        float raise = info.radius * 0.6f / (random(cellX + 5127708, cellY) % 20 + 25) * 15.0f;
        base += std::min(raise, 150.0f);
    } else if (info.environment == EnvironmentTypeRaisedPlateau) {
        base += (info.radius * 0.7f - info.radius * 0.5f) * 0.75f;
    }
    if (info.type == PlaceArena) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        centerX = centerX * 16 + 8;
        centerY = centerY * 16 + 8;
        int dx = centerX - x;
        int dy = centerY - y;
        int squaredDistance = dx * dx + dy * dy;
        if (squaredDistance < 18225) {
            if (squaredDistance < 3600) {
                return (int)(base + 1.0f);
            }
            if (squaredDistance >= 16384) {
                return (int)(base + 40.0f);
            }
            int distanceX = abs(x - centerX);
            if (distanceX >= 20 && abs(y - centerY) >= 20) {
                return (int)((float)((int)std::max(sqrtf((float)squaredDistance) * 0.5f - 30.0f, 0.0f) / 2 * 2 + 2) +
                             base + 1.0f);
            }
            int distance = (std::max)(distanceX, abs(y - centerY));
            return (int)(std::max(distance * 0.5f - 30.0f, 0.0f) + base + 1.0f);
        }
        return floorToInt(height) - 1;
    }
    int distanceY = abs(y - zone[1] * 16);
    int distanceX = abs(x - zone[0] * 16);
    int distance = (std::max)(distanceX, distanceY);
    if (info.type == PlaceTemple) {
        distance = std::max(distance, 24);
        if (distanceX > 12 && distanceY > 12) {
            distance = distance / 6 * 6 - 3;
        }
    }
    float top = base + 100.0f - distance;
    const float &highest = (std::max)(height, top);
    return floorToInt(highest);
}

// placeSquaredDistance's border noise: two anisotropic noises (0.005 and 0.01; offsets 18843, 9432 and 299, 90650) of
// up to 32 blocks.
plasma::Vector<2, float> World::morphOffset(int x, int y)
{
    return plasma::Vector<2, float>(noise(x * 0.005 + 18843.0, y * 0.01 + 9432.0) * 32.0f,
                                    noise(x * 0.01 + 299.0, y * 0.005 + 90650.0) * 32.0f);
}

// A realm by hashes of the place: an adjacent one (salt 5209343, one in 30) or its own (name seed salt 31737378).
int World::mortalRealm(int regionX, int regionY, bool large)
{
    if (large) {
        regionX = floorDivide(regionX, 2) * 2;
        regionY = floorDivide(regionY, 2) * 2;
    }
    int x = floorDivide(regionX, BlocksPerCell);
    int y = floorDivide(regionY, BlocksPerCell);
    switch (random(regionX + 5209343, regionY) % 30) {
    case 1:
        return random(x - 1 + 31737378, y);
    case 2:
        return random(x + 1 + 31737378, y);
    case 3:
        return random(x + 31737378, y - 1);
    case 4:
        return random(x + 31737378, y + 1);
    case 5:
        return random(x - 1 + 31737378, y - 1);
    case 6:
        return random(x + 1 + 31737378, y - 1);
    case 7:
        return random(x - 1 + 31737378, y + 1);
    case 8:
        return random(x + 1 + 31737378, y + 1);
    default:
        return random(x + 31737378, y);
    }
}

int World::mostLevelTier()
{
    if (!m_localCreature) {
        return 0;
    }
    return Missions(this, m_localCreature->m_entityData.region).solvedCount();
}

// The cliffs (a hand-tuned noise profile): base and ridge noises (0.0015, 0.001, 0.006 with their offsets) at 0.2,
// modulated in mountain places (0.025), fading below 80 blocks and turning into caves above 4 (0.05 per block).
float World::mountainCliffs(int x, int y, float elevation, float mountains, float river, float lavaRiver,
                            float poisonRiver, float road, AreaInfo *info)
{
    float cliffBaseNoise = noise(x * 0.0015 + 565.0, y * 0.0015 + 8877.0);
    float base = std::max(0.0f, cliffBaseNoise + cliffBaseNoise + (mountains + 1.0f));
    float coarseRidgeNoise = noise(x * 0.001 + 34234.0, y * 0.001 + 3245.0);
    coarseRidgeNoise *= noise(x * 0.0015 + 545.0, y * 0.0015 + 654665.0);
    float fineRidgeNoise = noise(x * 0.006 + 55545.0, y * 0.006 + 6566.0);
    float ridge = fabs(fineRidgeNoise * 0.2f + (coarseRidgeNoise + coarseRidgeNoise));
    ridge = std::max(1.0f - ridge, 0.0f);
    float keep = 1.0f;
    int type = info->type;
    if (type != PlaceNone) {
        float influence = info->influence;
        if (influence > 0.0f) {
            if (type == PlaceMountain || type == PlaceVolcano) {
                ridge += (noise(x * 0.025 + 84333.0, y * 0.025 + 98111.0) + 1.0f) * (influence * 0.5f) * (1.0f - ridge);
                base += influence + influence;
            } else {
                switch (type) {
                case PlaceManaPump:
                case PlaceRock:
                case PlaceCanyon:
                case PlaceBeach:
                    break;
                default:
                    float t = std::max(0.0f, influence + influence);
                    t = std::min(1.0f, t);
                    keep = 1.0f - smoothStep(t);
                    break;
                }
            }
        }
    }
    float low = (std::min)(1.0f, (std::max)(0.0f, (elevation + 80.0f) * 0.05f));
    if (base <= 0.0f)
        return 0.0f;
    float t = std::min(1.0f, base);
    float others = 1.0f - (1.0f - std::min(lavaRiver + lavaRiver, 1.0f)) * (1.0f - std::min(river + river, 1.0f)) *
                              (1.0f - std::min(poisonRiver + poisonRiver, 1.0f)) *
                              (1.0f - std::min(road + road, 1.0f)) * keep;
    float value = (1.0f - smoothStep(others)) * smoothStep(t) * (low * ridge);
    if (value > 0.5f) {
        float excess = value - 0.5f;
        float clamped = (std::max)(0.0f, (std::min)(1.0f, excess * 4.0f));
        float cave = caveDensity(x, y, type, info->influence);
        cave = smoothStep(cave);
        value = (1.0f - (1.0f - (1.0f - cave) * (1.0f - cave)) * (1.0f - clamped * clamped)) * excess + 0.5f;
    }
    return value;
}

bool World::isBlocked(const plasma::Vector<3, Fixed> &position, const plasma::Vector<3, float> &size, bool unusedFlag)
{
    plasma::Vector<3, int> first = fixedToBlock(position - plasma::Vector<3, Fixed>(size * 0.5f));
    plasma::Vector<3, int> last = fixedToBlock(position + plasma::Vector<3, Fixed>(size * 0.5f));
    for (int x = first[0]; x <= last[0]; ++x) {
        for (int y = first[1]; y <= last[1]; ++y) {
            for (int z = first[2]; z <= last[2]; ++z) {
                if (x > first[0] && x < last[0] && y > first[1] && y < last[1] && z > first[2] && z < last[2])
                    continue;
                Block block = getBlock(x, y, z);
                if (block.isSolid() || ((block.flags & BlockFlagBarrier) && !(block.flags & BlockFlagSpiritPassable)) ||
                    block.type == BlockWater)
                    return true;
            }
        }
    }
    return false;
}

float World::BiomeSample::mountains()
{
    float distance = -1.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (biomes[i][j] == BiomeMountains && (distance < 0.0f || distance > squaredDistances[i][j])) {
                distance = squaredDistances[i][j];
            }
        }
    }
    return weight(distance, BiomeBlendRate);
}

// The muck's colour: dark green (30, 120 +- 20, 40) by noise at 0.06.
plasma::Vector<3, float> World::muckColor(int x, int y)
{
    return plasma::Vector<3, float>(30.0f, noise(x * 0.06, y * 0.06) * 20.0f + 120.0f, 40.0f);
}

plasma::Vector<3, Fixed> World::nearestShrine(const plasma::Vector<3, Fixed> &position)
{
    double bestDistance = -1.0;
    plasma::Vector<2, int> nearest(tileCoordinate(position[0]), tileCoordinate(position[1]));
    const std::set<std::array<int, 2>> &markers = m_localCreature->m_playerData->discoveredMarkers;
    for (std::array<int, 2> marker : markers) {
        if (findRegion(marker[0] * BlocksPerCell + BlocksPerCell / 2, marker[1] * BlocksPerCell + BlocksPerCell / 2) !=
            m_localCreature->m_entityData.region)
            continue;
        plasma::Vector<2, Fixed> offset =
            plasma::Vector<2, Fixed>(Fixed(marker[0] * BlocksPerCell + BlocksPerCell / 2),
                                     Fixed(marker[1] * BlocksPerCell + BlocksPerCell / 2)) -
            plasma::Vector<2, Fixed>(m_localCreature->m_entityData.position[0],
                                     m_localCreature->m_entityData.position[1]);
        double dx = (double)offset[0];
        double dy = (double)offset[1];
        double distance = dx * dx + dy * dy;
        if ((bestDistance < 0.0 || bestDistance > distance) &&
            zoneBuildingType(marker[0], marker[1]) == BuildingShrineOfLife) {
            bestDistance = distance;
            nearest = plasma::Vector<2, int>(marker[0], marker[1]);
        }
    }
    if (bestDistance < 0.0) {
        // The sites (siteInZone) of the places within a land's span.
        int firstX = floorDivide(tileCoordinate(position[0] - Fixed(BlocksPerLand)), BlocksPerPlace / BlocksPerCell);
        int lastX = floorDivide(tileCoordinate(position[0] + Fixed(BlocksPerLand)), BlocksPerPlace / BlocksPerCell);
        int firstY = floorDivide(tileCoordinate(position[1] - Fixed(BlocksPerLand)), BlocksPerPlace / BlocksPerCell);
        int lastY = floorDivide(tileCoordinate(position[1] + Fixed(BlocksPerLand)), BlocksPerPlace / BlocksPerCell);
        for (int regionX = firstX; regionX <= lastX; ++regionX) {
            for (int regionY = firstY; regionY <= lastY; ++regionY) {
                plasma::Vector<2, int> cell(regionX * 64 + 1 + random(regionX + 148289, regionY) % 62,
                                            regionY * 64 + 1 + random(regionX + 1464073, regionY) % 62);
                if (findRegion(cell[0] * BlocksPerCell + BlocksPerCell / 2,
                               cell[1] * BlocksPerCell + BlocksPerCell / 2) != m_localCreature->m_entityData.region)
                    continue;
                plasma::Vector<2, Fixed> offset =
                    plasma::Vector<2, Fixed>(Fixed(cell[0] * BlocksPerCell + BlocksPerCell / 2),
                                             Fixed(cell[1] * BlocksPerCell + BlocksPerCell / 2)) -
                    plasma::Vector<2, Fixed>(m_localCreature->m_entityData.position[0],
                                             m_localCreature->m_entityData.position[1]);
                double dx = (double)offset[0];
                double dy = (double)offset[1];
                double distance = dx * dx + dy * dy;
                if ((bestDistance < 0.0 || bestDistance > distance) &&
                    zoneBuildingType(cell[0], cell[1]) == BuildingShrineOfLife) {
                    bestDistance = distance;
                    nearest = cell;
                }
            }
        }
    }
    int level = structureLevel(nearest[0], nearest[1], 0);
    // Coordinate conversion shifts signed negative block coordinates.
    return plasma::Vector<3, Fixed>(Fixed(nearest[0] * 16 + 6), Fixed(nearest[1] * BlocksPerCell + BlocksPerCell / 2),
                                    Fixed(level));
}

// The notch width by hashes of the place (salts 1163573, 247674), one of three.
int World::notchWidth(int x, int y)
{
    if ((random(x + 1163573, y) & 1) == 0) {
        return (random(x + 247674, y) & 3) + 3;
    }
    return 1;
}

StaticEntity *World::object(const plasma::Vector<3, int> &id)
{
    Zone *zone = this->zone(id[0], id[1]);
    if (!zone) {
        return nullptr;
    }
    int index = id[2];
    if (index < 0 || index >= (int)zone->m_staticEntities.size()) {
        return nullptr;
    }
    return &zone->m_staticEntities[index];
}

void World::onAttack(Creature *attacker, Creature *target)
{
    if (target->m_entityData.hp <= 0.0f) {
        return;
    }
    ServerUpdates::AggroPacket packet;
    packet.creatureId = target->m_id;
    packet.attackerId = attacker->m_id;
    packet.aggro = target->m_aggro[attacker->m_id];
    m_serverUpdates.m_aggroPacketList.push_back(packet);
    for (Creature *other : m_creatures) {
        if (other && other != attacker && other != target && other->canDoDamage(attacker) &&
            !distanceExceeds(other->m_entityData.position, target->m_entityData.position, AllyAlertRange)) {
            if (other->m_aggro[attacker->m_id] == 0.0f) {
                other->m_aggro[attacker->m_id] = AllyAlertThreat;
                ServerUpdates::AggroPacket call;
                call.creatureId = other->m_id;
                call.attackerId = attacker->m_id;
                call.aggro = other->m_aggro[attacker->m_id];
                m_serverUpdates.m_aggroPacketList.push_back(call);
            }
        }
    }
}

void World::onKill(const ServerUpdates::KillPacket &kill)
{
    if (m_localCreature && kill.killerId == m_localCreature->m_id) {
        // The release does not use the victim; the mod gives the alpha's experience for it.
        Creature *victim = creatureByID(kill.targetId);
        mod::onKill(this, m_localCreature, victim);
    }
}

// Poison channels: lavaChannels' profile with noises of their own (0.0025, 0.0005, 0.01), scaled by the undead lands,
// suppressed in places (0.1, 10) and on roads (0.4).
float World::poisonChannels(int x, int y, float road, float undead, float mountains, AreaInfo *info)
{
    float result = 0.0f;
    if (undead > 0.5f) {
        float n = noise(x * 0.0025 + 5253.0, y * 0.0025 + 809834.0) * 0.5f;
        n += noise(x * 0.0005 + 6664.0, y * 0.0005 + 7567567.0);
        n += noise(x * 0.01 + 80084.0, y * 0.01 + 5664.0) * 0.1f;
        float ridge = fabs(n) * 5.0f;
        if (ridge > 0.5f) {
            ridge = (ridge - 0.5f) * 0.5f;
            ridge = ridge * ridge * ridge + 0.5f;
        }
        result = std::max(1.0f - ridge, 0.0f);
        result *= result;
        result *= (undead - 0.5f) * 2.0f;
        result *= 1.0f - mountains;
    }
    if (info->type != PlaceNone) {
        float outside = std::max(0.0f, (info->influence - 0.1f) * 10.0f);
        outside = 1.0f - std::min(1.0f, outside);
        result *= 1.0f - (1.0f - outside * outside);
    }
    float high = std::max(0.0f, (road - 0.4f) * 10.0f);
    high = std::min(1.0f, high);
    return (1.0f - high) * result;
}

// The candidates are the four armour pieces (categories 7, 4, 5, 6) in the material of the class (iron 1 for the
// warrior and unlisted classes, 26, 27, 28 for the classes 3, 2, 4; without plain, one time in three each of two other
// materials) and an amulet and a ring (8, 9) of material 11 or 12; one of them is returned. For the classes 2..4 and
// the unlisted ones the material choice is replaced by the plain one and the shoulder piece is entered twice.
Item World::randomArmor(int tier, int classType, bool plain)
{
    std::vector<Item> items;
    Item item;
    item.rarity = tier;
    if (classType < ClassRanger || classType > ClassRogue) {
        if (plain) {
            item.material = ItemMaterialIron;
        } else {
            switch (rand() % 3) {
            case 1:
                item.material = ItemMaterialBronze;
                break;
            case 2:
                item.material = ItemMaterialSteel;
                break;
            default:
                item.material = ItemMaterialIron;
                break;
            }
        }
        item.category = ItemCategoryShoulder;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryChest;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryHands;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryFeet;
        item.modifier = rand();
        items.push_back(item);
    }
    if ((classType != ClassWarrior && classType != ClassRanger && classType != ClassMage && classType != ClassRogue) ||
        classType == ClassMage) {
        if (plain) {
            item.material = ItemMaterialSilk;
        } else {
            switch (rand() % 3) {
            case 1:
                item.material = ItemMaterialFlannell;
                break;
            case 2:
                item.material = ItemMaterialFilament;
                break;
            default:
                item.material = ItemMaterialSilk;
                break;
            }
        }
        item.material = ItemMaterialSilk; // Replaces the preceding choice.
        item.category = ItemCategoryShoulder;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryShoulder;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryChest;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryHands;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryFeet;
        item.modifier = rand();
        items.push_back(item);
    }
    if ((classType != ClassWarrior && classType != ClassRanger && classType != ClassMage && classType != ClassRogue) ||
        classType == ClassRanger) {
        if (plain) {
            item.material = ItemMaterialLinen;
        } else {
            switch (rand() % 3) {
            case 1:
                item.material = ItemMaterialFelt;
                break;
            case 2:
                item.material = ItemMaterialMail;
                break;
            default:
                item.material = ItemMaterialLinen;
                break;
            }
        }
        item.material = ItemMaterialLinen; // Replaces the preceding choice.
        item.category = ItemCategoryShoulder;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryShoulder;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryChest;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryHands;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryFeet;
        item.modifier = rand();
        items.push_back(item);
    }
    if (classType < ClassWarrior || classType > ClassMage) {
        if (plain) {
            item.material = ItemMaterialCotton;
        } else {
            switch (rand() % 3) {
            case 1:
                item.material = ItemMaterialCalico;
                break;
            case 2:
                item.material = ItemMaterialRivet;
                break;
            default:
                item.material = ItemMaterialCotton;
                break;
            }
        }
        item.material = ItemMaterialCotton; // Replaces the preceding choice.
        item.category = ItemCategoryShoulder;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryShoulder;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryChest;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryHands;
        item.modifier = rand();
        items.push_back(item);
        item.category = ItemCategoryFeet;
        item.modifier = rand();
        items.push_back(item);
    }
    item.material = rand() % 2 == 0 ? ItemMaterialGold : ItemMaterialSilver;
    item.category = ItemCategoryNeck;
    item.modifier = rand();
    items.push_back(item);
    item.material = rand() % 2 == 0 ? ItemMaterialGold : ItemMaterialSilver;
    item.category = ItemCategoryRing;
    item.modifier = rand();
    items.push_back(item);
    item = items[rand() % items.size()];
    return item;
}

Item World::randomPetFood(int race)
{
    std::map<int, std::vector<int>>::iterator it = m_petFoodByRace.find(race);
    std::vector<int> ids;
    if (rand() % 2 == 0 && it != m_petFoodByRace.end() && !it->second.empty()) {
        ids = it->second;
    } else {
        ids = m_petRaces;
    }
    Item item;
    item.category = ItemCategoryPetFood;
    item.id = ids[rand() % ids.size()];
    return item;
}

// The candidates are the weapon ids of the class (0..2, 15..17 and 13 for the warrior; 3, 5, 4 for the rogue; 6, 8 for
// the ranger; 10, 11, 12 for the mage; all of them for an unlisted class), of material 1, then 2, and 11 or 12 for id
// 12; one of them is returned.
Item World::randomWeapon(int tier, int classType, bool unused)
{
    std::vector<Item> items;
    Item item;
    item.rarity = tier;
    item.material = ItemMaterialIron;
    item.category = ItemCategoryWeapon;
    if (classType < ClassRanger || classType > ClassRogue) {
        item.id = rand() % (WeaponMace - WeaponSword + 1);
        item.modifier = rand();
        items.push_back(item);
        item.material = ItemMaterialIron;
        item.id = rand() % (WeaponGreatmace - WeaponGreatsword + 1) + WeaponGreatsword;
        item.modifier = rand();
        items.push_back(item);
        item.id = WeaponShield;
        item.modifier = rand();
        items.push_back(item);
    }
    if (classType < ClassWarrior || classType > ClassMage) {
        item.id = WeaponDagger;
        item.modifier = rand();
        items.push_back(item);
        item.id = WeaponLongsword;
        item.modifier = rand();
        items.push_back(item);
        item.id = WeaponFist;
        item.modifier = rand();
        items.push_back(item);
    }
    item.material = ItemMaterialWood;
    if ((classType != ClassWarrior && classType != ClassRanger && classType != ClassMage && classType != ClassRogue) ||
        classType == ClassRanger) {
        item.id = WeaponBow;
        item.modifier = rand();
        items.push_back(item);
        item.id = WeaponCrossbow; // not added: the next weapon replaces it
        item.modifier = rand();
        item.id = WeaponBoomerang;
        item.modifier = rand();
        items.push_back(item);
    }
    if ((classType != ClassWarrior && classType != ClassRanger && classType != ClassMage && classType != ClassRogue) ||
        classType == ClassMage) {
        item.id = WeaponStaff;
        item.modifier = rand();
        items.push_back(item);
        item.id = WeaponWand;
        item.modifier = rand();
        items.push_back(item);
        item.id = WeaponBracelet;
        item.material = rand() % 2 == 0 ? ItemMaterialSilver : ItemMaterialGold;
        item.modifier = rand();
        items.push_back(item);
    }
    item = items[rand() % items.size()];
    return item;
}

// As cameraRay, with the test of the blocks in the caller's hands: the ray goes on while a block is open (empty or
// water and, with stopAtBarriers, without BlockFlagBarrier) when throughSolid is false, and while it is not when
// throughSolid is true. The last parameter is not read.
float World::rayDistance(const plasma::Vector<3, Fixed> &position, const plasma::Vector<3, float> &direction,
                         float range, bool throughSolid, bool stopAtBarriers, Creature *other)
{
    float distance = 0.0f;
    plasma::Vector<3, int> cell = fixedToBlock(position);
    int steps = 0;
    Zone *zone = this->zone(floorDivide(cell[0], BlocksPerZone), floorDivide(cell[1], BlocksPerZone));
    while (range > distance) {
        if (!zone) {
            break;
        }
        int x = cell[0];
        int y = cell[1];
        int z = cell[2];
        if (!letsBodyPass(zone->getBlock(x, y, z), stopAtBarriers) != throughSolid || steps > 500) {
            break;
        }
        steps++;
        plasma::Vector<3, Fixed> current = position + plasma::Vector<3, Fixed>(direction * distance);
        float step = 10.0f;
        int axis = 0;
        for (int i = 0; i < 3; i++) {
            if (direction[i] * direction[i] < 0.000001f)
                continue;
            float t;
            if (direction[i] > 0.0f)
                t = (float)((Fixed(cell[i] + 1) - current[i]) / direction[i]);
            else
                t = (float)((Fixed(cell[i]) - current[i]) / direction[i]);
            if (step > t) {
                axis = i;
                step = t;
            }
        }
        if (direction[axis] > 0.0f) {
            cell[axis]++;
        } else {
            cell[axis]--;
        }
        plasma::Vector<2, int> zonePosition(floorDivide(cell[0], BlocksPerZone), floorDivide(cell[1], BlocksPerZone));
        if (zonePosition != zone->m_position) {
            zone = this->zone(zonePosition[0], zonePosition[1]);
        }
        distance += step;
    }
    return std::min(range, distance);
}

void World::removeCreature(Creature *creature)
{
    if (creature) {
        for (std::list<Creature *>::iterator it = m_creatures.begin(); it != m_creatures.end(); ++it) {
            if (*it == creature) {
                m_creatures.erase(it);
                break;
            }
        }
        m_creaturesById.erase(creature->m_id);
        delete creature;
    }
}

void World::removeCreatures()
{
    for (Creature *creature : m_creatures) {
        delete creature;
    }
    m_creatures.clear();
    m_creaturesById.clear();
    m_obstacles.clear();
}

// The roofs' geometry: an edge 14 blocks wide, flat roofs 3 or 8 high, joined edges and pitches by 8 and 7.
int World::roofHeight(int x, int y, int type, int sides, int kind)
{
    if (type == BuildingTempleEntrance) {
        return 2;
    }
    if (buildingKind(type) == BuildingKindHouse && kind == HouseStyle_DesertFlatRoof) {
        if (x < 2 || y < 2 || x >= 14 || y >= 14) {
            return 3;
        }
        return 2;
    }
    int distanceY = (int)(float)fabs((float)(y - 8) + 0.5f);
    int distanceX = (int)(float)fabs((float)(x - 8) + 0.5f);
    int distance = (std::max)(distanceX, distanceY);
    if (type != BuildingCryptUpperRoom && kind != HouseStyle_SavannahLowRoof) {
        if (type == BuildingDungeonDomedHall || type == BuildingDungeonTower || type == BuildingCastleTower) {
            distance = -8;
            if ((sides & JoinedSideMinusX) != 0 && x - 8 > distance) {
                distance = x - 8;
            }
            if ((sides & JoinedSidePlusX) != 0 && distance < 7 - x) {
                distance = 7 - x;
            }
            if ((sides & JoinedSideMinusY) != 0 && distance < y - 8) {
                distance = y - 8;
            }
            if ((sides & JoinedSidePlusY) != 0 && distance < 7 - y) {
                distance = 7 - y;
            }
        } else {
            if (((sides & JoinedSideMinusX) != 0 && x < 8) || ((sides & JoinedSidePlusX) != 0 && x >= 8)) {
                distance = (std::min)(distance, distanceY);
            }
            if (((sides & JoinedSideMinusY) != 0 && y < 8) || ((sides & JoinedSidePlusY) != 0 && y >= 8)) {
                distance = (std::min)(distance, distanceX);
            }
        }
    }
    int height = 8 - distance;
    if (kind == HouseStyle_DarkSteepRoof) {
        height *= 2;
    }
    if (type == BuildingCryptUpperRoom || type == BuildingDungeonTower || type == BuildingCastleTower ||
        type == BuildingVaultEntrance || type == BuildingGuildHallCentre || type == BuildingArenaTower ||
        type == BuildingSandstoneTreasureChamber || buildingKind(type) == BuildingKindOldHut) {
        height *= 2;
    } else if (type == BuildingHousePavilion || type == BuildingMineEntrance ||
               buildingKind(type) == BuildingKindStonePavilion || kind == HouseStyle_SavannahLowRoof) {
        height /= 2;
    }
    return height;
}

// The colour of rock at a block: grey with a noise shade, towards orange above a = 0.6, darkened by d and mixed with a
// noise-veined red by c. The second weight is not read. Remove every non-player creature at the change of day and
// rebuild the id index from the surviving players.
void World::resetBuff(Creature *creature, int type)
{
    Creature::Buff *buff = creature->findBuff(type);
    if (buff) {
        buff->timeRemaining = 0.0f;
        ServerUpdates::BuffPacket packet;
        packet.sourceId = creature->m_id;
        packet.targetId = creature->m_id;
        packet.buff = *buff;
        m_serverUpdates.m_buffPacketList.push_back(packet);
    }
}

void World::rewindDay()
{
    std::list<std::list<Creature *>::iterator> others;
    for (std::list<Creature *>::iterator creature = m_creatures.begin(); creature != m_creatures.end(); ++creature) {
        if ((*creature)->m_entityData.hostilityType != HostilityPlayer)
            others.push_back(creature);
    }
    for (std::list<Creature *>::iterator creature : others) {
        delete *creature;
        m_creatures.erase(creature);
    }
    m_creaturesById.clear();
    for (Creature *creature : m_creatures)
        m_creaturesById[creature->m_id] = creature;
    m_obstacles.clear();
}

// The rock palette (hand-authored): grey by noise (0.4, 0.1; offset 98984) around 210, hot rock browner (above 0.6,
// 0.3 humid), darkened in undead lands (0.25 to 0.15), with fireland veins by noise (0.04, 0.02).
plasma::Vector<3, float> World::rockColor(int x, int y, int z, float temperature, float humidity, float firelands,
                                          float undead)
{
    double scaledZ = z * 0.4;
    double scaledY = y * 0.1;
    double scaledX = x * 0.1;
    float shade = noise(scaledY + 98984.0, scaledZ);
    shade += noise(x * 0.1, scaledZ);
    float blue = 210.0f;
    float red = 210.0f;
    float green = 210.0f;
    shade = (shade * 0.5f + 1.0f) * 0.5f;
    if (temperature >= 0.5f) {
        if (temperature > 0.6f) {
            float t = std::min(1.0f, (temperature - 0.6f) / 0.3f);
            blue = t * -130.0f + 210.0f;
            red = t * 110.0f + 210.0f;
            green = t * 10.0f + 210.0f;
        }
    }
    plasma::Vector<3, float> base(red, green, blue);
    plasma::Vector<3, float> color = base + base * (shade * 0.3f);
    if (undead > 0.001f) {
        color *= 1.0f - undead * 0.25f;
        color[0] *= 1.0f - undead * 0.15f;
        color[1] *= 1.0f - undead * 0.25f;
    }
    if (firelands > 0.001f) {
        float vein = noise(x * 0.04 + 8773.0, y * 0.04 + 99808.0) * 0.5f;
        vein += noise(x * 0.02, y * 0.02);
        vein = 1.0f - (float)fabs(std::min(1.0f, vein));
        color *= 1.0f - firelands;
        vein = vein * (vein * vein * vein * vein);
        float spot = noise(scaledX, scaledY);
        color +=
            plasma::Vector<3, float>(vein * 300.0f + 30.0f, (spot + 1.0f) * vein * 50.0f + 30.0f, 30.0f) * firelands;
    }
    return color;
}

// base plus amplitude times noise at 0.05 (offsets 9076, 8998).
float World::roughness(int x, int y, int base, int amplitude)
{
    return noise(x * 0.05 + 9076.0, y * 0.05 + 8998.0) * amplitude + base;
}

// Sand (a hand-tuned noise profile): noises at 0.01 and 0.02 (with offsets), biased by humidity (1.5 about 0.3) and
// the cliffs.
float World::sandAmount(int x, int y, float humidity, float cliffs, float desertness)
{
    double result = (noise(x * 0.01 + 647646.0, y * 0.01 + 84373.0) + (1.5 - humidity)) * 0.3f + desertness;
    result += noise(x * 0.02 + 43234.0, y * 0.02 + 5345345.0) * 0.2;
    result += noise(x * 0.05 + 77878.0, y * 0.05 + 9888.0) * 0.1;
    return result;
}

// Sand's colour: (340, 250, 100) shaded up to 70 by noise at 0.03.
plasma::Vector<3, float> World::sandColor(int x, int y)
{
    float shade = noise(x * 0.03 + m_sandShadeOffset[0], y * 0.03 + m_sandShadeOffset[1]);
    plasma::Vector<3, float> color(340.0f, 250.0f, 100.0f);
    shade = (shade + 1.0f) * 0.5f * 70.0f;
    color += plasma::Vector<3, float>(shade, shade, shade);
    return color;
}

void World::saveState(const WorldState &state)
{
    if (m_worldName.size() != 0) {
        BytesIO data;
        Serializer(&data).saveWorldState(state);
        m_database.write("state", data);
    }
}

plasma::Vector<2, int> World::siteInZone(int x, int y)
{
    return plasma::Vector<2, int>((x << 6) + 1 + random(x + 148289, y) % 62,
                                  random(x + 1464073, y) % 62 + (y << 6) + 1);
}

float World::BiomeSample::snow()
{
    float distance = -1.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if ((biomes[i][j] == BiomeSnowlands || biomes[i][j] == BiomeSnowForest) &&
                (distance < 0.0f || distance > squaredDistances[i][j])) {
                distance = squaredDistances[i][j];
            }
        }
    }
    return weight(distance, BiomeBlendRate);
}

// Snow: (200, 280, 400), components above 255 brightening.
plasma::Vector<3, float> World::snowColor(int x, int y)
{
    // Takes the block's position (every caller passes it) and ignores it.
    return plasma::Vector<3, float>(200.0f, 280.0f, 400.0f);
}

plasma::Vector<3, Fixed> World::startPosition()
{
    Fixed center = Fixed(WorldStartCoordinate) - Fixed(-0.5);
    return plasma::Vector<3, Fixed>(center, center, Fixed(height((int)center, (int)center, 0)));
}

// Keeps what has to outlive the zone in the world state: its items and static entities when their flags are set, and
// its changed blocks, its m_completedActivators and m_defeatedMissionCreatureCount when they hold something or the
// zone has a record already. The key is a temporary built from the zone's position before each lookup.
void World::storeZone(Zone *zone)
{
    if (zone->m_itemsChanged) {
        m_state.m_zoneStates[std::array<int, 2> {zone->m_position[0], zone->m_position[1]}].flags |= ZoneStateFlagItems;
        m_state.m_zoneStates[std::array<int, 2> {zone->m_position[0], zone->m_position[1]}].items = zone->m_items;
    }
    if (zone->m_staticEntitiesChanged) {
        m_state.m_zoneStates[std::array<int, 2> {zone->m_position[0], zone->m_position[1]}].flags |=
            ZoneStateFlagStaticEntities;
        m_state.m_zoneStates[std::array<int, 2> {zone->m_position[0], zone->m_position[1]}].staticEntities =
            zone->m_staticEntities;
    }
    if (!zone->m_cubeChanges.empty() || m_state.m_zoneStates.find(std::array<int, 2> {
                                            zone->m_position[0], zone->m_position[1]}) != m_state.m_zoneStates.end())
        m_state.m_zoneStates[std::array<int, 2> {zone->m_position[0], zone->m_position[1]}].cubeChanges =
            zone->m_cubeChanges;
    if (!zone->m_completedActivators.empty() ||
        m_state.m_zoneStates.find(std::array<int, 2> {zone->m_position[0], zone->m_position[1]}) !=
            m_state.m_zoneStates.end())
        m_state.m_zoneStates[std::array<int, 2> {zone->m_position[0], zone->m_position[1]}].completedActivators =
            zone->m_completedActivators;
    if (zone->m_defeatedMissionCreatureCount != 0 ||
        m_state.m_zoneStates.find(std::array<int, 2> {zone->m_position[0], zone->m_position[1]}) !=
            m_state.m_zoneStates.end())
        m_state.m_zoneStates[std::array<int, 2> {zone->m_position[0], zone->m_position[1]}]
            .defeatedMissionCreatureCount = zone->m_defeatedMissionCreatureCount;
}

// The width in blocks over which a road between two places lowers streetAmount.
static float s_roadWidth = 250.0f;

// 1 at s_roadWidth blocks or more from a road and falling to 0 on it, from the squared distance.
static inline float roadWeight(float squaredDistance)
{
    float closeness = 1.0f - std::min(1.0f, squaredDistance / (s_roadWidth * s_roadWidth));
    closeness *= closeness;
    closeness *= closeness;
    closeness *= closeness;
    closeness *= closeness;
    return 1.0f - closeness;
}

// The street amount of a block column (World.h): 1 on the roads that join neighbouring places and falling to 0 within
// half a block's weight of them (the product of the road weights of the 3 x 3 cells around the column, whose position
// is first displaced by up to 160 blocks of noise outside places). Each cell has a road to its western and to its
// southern neighbour when the hashes of the cell say so (constants 531813 and 2648358; a large place's hash is that of
// its even cell). In villages (type 5) a noise pattern replaces the roads towards the centre; in the listed place
// types the roads fade with the place's influence. The streets (hand-tuned): positions warped by noise (0.0025, 0.005;
// up to 128 and 32 blocks), links to the neighbouring places by hashes (salts 531813 east-west, 2648358 north-south),
// a village pattern at 0.01 within influence 0.4 (from 0.0001), and a knee at 120 compressing by 119.5.
float World::streetAmount(int x, int y, int areaType, float areaInfluence)
{
    float warpX = noise(x * 0.0025 + 2346.0, y * 0.005 + 64565.0) * 128.0f;
    warpX += noise(x * 0.005 + 4234.0, y * 0.01 + 5434545.0) * 32.0f;
    float warpY = noise(x * 0.005 + 23432.0, y * 0.0025 + 2342344.0) * 128.0f;
    warpY += noise(x * 0.01 + 54356.0, y * 0.005 + 7677.0) * 32.0f;
    float outside = 1.0f - areaInfluence;
    long long positionX = ((long long)x << FixedFractionBits) + (long long)(warpX * outside * FixedUnitsPerBlock);
    long long positionY = ((long long)y << FixedFractionBits) + (long long)(warpY * outside * FixedUnitsPerBlock);
    int cellX = floorDivide((int)(positionX / FixedUnitsPerBlock), BlocksPerPlace);
    int cellY = floorDivide((int)(positionY / FixedUnitsPerBlock), BlocksPerPlace);
    float result = 1.0f;
    for (int i = cellX - 1; i <= cellX + 1; i++) {
        for (int j = cellY - 1; j <= cellY + 1; j++) {
            bool large = isLargePlace(i, j);
            plasma::Vector<2, int> center = placeCenter(i, j, large);
            float weight = 1.0f;
            int hashX = i;
            int hashY = j;
            if (large) {
                hashX = floorDivide(hashX, 2) * 2;
                hashY = floorDivide(hashY, 2) * 2;
            }
            if (random(hashX + 531813, hashY) & 1) {
                plasma::Vector<2, int> other = placeCenter(i - 1, j, isLargePlace(i - 1, j));
                if (center != other) {
                    plasma::Vector<2, int> end = other;
                    float spanX = (float)(end[0] - center[0]);
                    float spanY = (float)(end[1] - center[1]);
                    float offsetX =
                        (float)(positionX - ((long long)center[0] << FixedFractionBits)) * (1.0f / FixedUnitsPerBlock);
                    float offsetY =
                        (float)(positionY - ((long long)center[1] << FixedFractionBits)) * (1.0f / FixedUnitsPerBlock);
                    float along = spanX * offsetX + spanY * offsetY;
                    float squaredDistance;
                    if (along <= 0.0f) {
                        squaredDistance = offsetY * offsetY + offsetX * offsetX;
                    } else {
                        float length = spanY * spanY + spanX * spanX;
                        if (along >= length) {
                            float endX = (float)(positionX - ((long long)other[0] << FixedFractionBits)) *
                                         (1.0f / FixedUnitsPerBlock);
                            float endY = (float)(positionY - ((long long)other[1] << FixedFractionBits)) *
                                         (1.0f / FixedUnitsPerBlock);
                            squaredDistance = endY * endY + endX * endX;
                        } else {
                            along /= length;
                            offsetY -= spanY * along;
                            offsetX -= spanX * along;
                            squaredDistance = offsetX * offsetX + offsetY * offsetY;
                        }
                    }
                    weight = roadWeight(squaredDistance);
                }
            }
            hashX = i + 1;
            hashY = j;
            if (isLargePlace(i + 1, j)) {
                hashX = floorDivide(hashX, 2) * 2;
                hashY = floorDivide(hashY, 2) * 2;
            }
            if (random(hashX + 531813, hashY) & 1) {
                plasma::Vector<2, int> other = placeCenter(i + 1, j, isLargePlace(i + 1, j));
                if (center != other) {
                    plasma::Vector<2, int> end = other;
                    float spanX = (float)(end[0] - center[0]);
                    float spanY = (float)(end[1] - center[1]);
                    float offsetX =
                        (float)(positionX - ((long long)center[0] << FixedFractionBits)) * (1.0f / FixedUnitsPerBlock);
                    float offsetY =
                        (float)(positionY - ((long long)center[1] << FixedFractionBits)) * (1.0f / FixedUnitsPerBlock);
                    float along = spanX * offsetX + spanY * offsetY;
                    float squaredDistance;
                    if (along <= 0.0f) {
                        squaredDistance = offsetY * offsetY + offsetX * offsetX;
                    } else {
                        float length = spanY * spanY + spanX * spanX;
                        if (along >= length) {
                            float endX = (float)(positionX - ((long long)other[0] << FixedFractionBits)) *
                                         (1.0f / (float)FixedUnitsPerBlock);
                            float endY = (float)(positionY - ((long long)other[1] << FixedFractionBits)) *
                                         (1.0f / (float)FixedUnitsPerBlock);
                            squaredDistance = endY * endY + endX * endX;
                        } else {
                            along /= length;
                            offsetY -= spanY * along;
                            offsetX -= spanX * along;
                            squaredDistance = offsetX * offsetX + offsetY * offsetY;
                        }
                    }
                    weight *= roadWeight(squaredDistance);
                }
            }
            hashX = i;
            hashY = j;
            if (large) {
                hashX = floorDivide(hashX, 2) * 2;
                hashY = floorDivide(hashY, 2) * 2;
            }
            if (random(hashX + 2648358, hashY) & 1) {
                plasma::Vector<2, int> other = placeCenter(i, j - 1, isLargePlace(i, j - 1));
                if (center != other) {
                    plasma::Vector<2, int> end = other;
                    float spanX = (float)(end[0] - center[0]);
                    float spanY = (float)(end[1] - center[1]);
                    float offsetX = (float)(positionX - ((long long)center[0] << 16)) * (1.0f / 65536.0f);
                    float offsetY = (float)(positionY - ((long long)center[1] << 16)) * (1.0f / 65536.0f);
                    float along = spanX * offsetX + spanY * offsetY;
                    float squaredDistance;
                    if (along <= 0.0f) {
                        squaredDistance = offsetY * offsetY + offsetX * offsetX;
                    } else {
                        float length = spanY * spanY + spanX * spanX;
                        if (along >= length) {
                            float endX = (float)(positionX - ((long long)other[0] << 16)) * (1.0f / 65536.0f);
                            float endY = (float)(positionY - ((long long)other[1] << 16)) * (1.0f / 65536.0f);
                            squaredDistance = endY * endY + endX * endX;
                        } else {
                            along /= length;
                            offsetY -= spanY * along;
                            offsetX -= spanX * along;
                            squaredDistance = offsetX * offsetX + offsetY * offsetY;
                        }
                    }
                    weight *= roadWeight(squaredDistance);
                }
            }
            hashX = i;
            hashY = j + 1;
            if (isLargePlace(i, j + 1)) {
                hashX = floorDivide(hashX, 2) * 2;
                hashY = floorDivide(hashY, 2) * 2;
            }
            if (random(hashX + 2648358, hashY) & 1) {
                plasma::Vector<2, int> other = placeCenter(i, j + 1, isLargePlace(i, j + 1));
                if (center != other) {
                    plasma::Vector<2, int> end = other;
                    float spanX = (float)(end[0] - center[0]);
                    float spanY = (float)(end[1] - center[1]);
                    float offsetX = (float)(positionX - ((long long)center[0] << 16)) * (1.0f / 65536.0f);
                    float offsetY = (float)(positionY - ((long long)center[1] << 16)) * (1.0f / 65536.0f);
                    float along = spanX * offsetX + spanY * offsetY;
                    float squaredDistance;
                    if (along <= 0.0f) {
                        squaredDistance = offsetY * offsetY + offsetX * offsetX;
                    } else {
                        float length = spanY * spanY + spanX * spanX;
                        if (along >= length) {
                            float endX = (float)(positionX - ((long long)other[0] << 16)) * (1.0f / 65536.0f);
                            float endY = (float)(positionY - ((long long)other[1] << 16)) * (1.0f / 65536.0f);
                            squaredDistance = endY * endY + endX * endX;
                        } else {
                            along /= length;
                            offsetY -= spanY * along;
                            offsetX -= spanX * along;
                            squaredDistance = offsetX * offsetX + offsetY * offsetY;
                        }
                    }
                    weight *= roadWeight(squaredDistance);
                }
            }
            result *= weight;
        }
    }
    if (areaType == PlaceVillage) {
        if (areaInfluence > 0.0001f) {
            float inner = std::max(0.0f, (areaInfluence - 0.4f) * 10.0f);
            inner = 1.0f - std::min(1.0f, inner);
            float pattern = (float)fabs(noise(x * 0.01, y * 0.01));
            pattern = std::min(pattern + pattern, 1.0f);
            result *= 1.0f - (1.0f - pattern * pattern * (pattern * pattern)) * (1.0f - inner * inner);
        }
    } else if (areaType == PlaceMountain || areaType == PlaceLake || areaType == PlaceLavaLake ||
               areaType == PlaceManaPump || areaType == PlaceCrater || areaType == PlaceVolcano ||
               areaType == PlaceRock || areaType == PlacePlain || areaType == PlaceHill || areaType == PlaceGrove ||
               areaType == PlaceGlade) {
        result = 1.0f - (1.0f - result) * outside;
    }
    result *= 120.0f;
    if (result > 0.5f) {
        result = (result - 0.5f) / 119.5f * 0.5f + 0.5f;
    }
    return std::max(1.0f - result, 0.0f);
}

// In the villages (types 1 to 4) up to three squares beside the centre square carry a building: the neighbour along x,
// the one along y and the diagonal one, on the sides (hashes 463276 and 1160520 of the cell) and with the presence
// (438504, 729652) that the cell's hashes choose. The building type (1..100, 0 for none) of the 16-block zone square
// (zoneX, zoneY): first what the cell's dungeon and the place's mission put there, then the buildings of the place by
// its type (around the place's centre square), then the region's site, and at last the scattered buildings of the open
// land, chosen by the biome values at point (a block position inside the square) where the ground is high, dry and off
// the roads.
BuildingType World::streetBuildingType(int zoneX, int zoneY, const plasma::Vector<2, int> &point, const AreaInfo &info)
{
    int blockX = zoneX * BlocksPerCell;
    int cellX = floorDivide(blockX, BlocksPerPlace);
    int blockY = zoneY * BlocksPerCell;
    int cellY = floorDivide(blockY, BlocksPerPlace);
    bool large = info.large;
    if (large) {
        cellX = floorDivide(cellX, 2) * 2;
        cellY = floorDivide(cellY, 2) * 2;
    }
    plasma::Vector<2, int> dungeon(0, 0);
    int dungeonKind = dungeonType(cellX, cellY, info.type, large);
    if (dungeonKind != 0) {
        dungeon = dungeonPosition(cellX, cellY, info.type, large);
        if (dungeonKind == PlaceShrine) {
            if (zoneX == dungeon[0] + 4 && zoneY == dungeon[1]) {
                return BuildingShrineEntrance;
            }
            if (zoneX == dungeon[0] + 3 && zoneY == dungeon[1]) {
                return BuildingShrineDescent10;
            }
            if (zoneX == dungeon[0] + 2 && zoneY == dungeon[1]) {
                return BuildingShrineDescent20;
            }
            if (zoneX == dungeon[0] + 1 && zoneY == dungeon[1]) {
                return BuildingShrineDescent30;
            }
            if (zoneX == dungeon[0] && zoneY == dungeon[1]) {
                return BuildingShrineDescent40;
            }
        } else if (dungeonKind == PlaceMines) {
            if (zoneX == dungeon[0] + 4 && zoneY == dungeon[1]) {
                return BuildingMineEntrance;
            }
            if (zoneX == dungeon[0] + 3 && zoneY == dungeon[1]) {
                return BuildingMineDescent10;
            }
            if (zoneX == dungeon[0] + 2 && zoneY == dungeon[1]) {
                return BuildingMineDescent20;
            }
            if (zoneX == dungeon[0] + 1 && zoneY == dungeon[1]) {
                return BuildingMineDescent30;
            }
            if (zoneX == dungeon[0] && zoneY == dungeon[1]) {
                return BuildingMineDescent40;
            }
        } else if (dungeonKind == PlaceVault) {
            if (zoneX == dungeon[0] + 4 && zoneY == dungeon[1]) {
                return BuildingVaultEntrance;
            }
            if (zoneX == dungeon[0] + 3 && zoneY == dungeon[1]) {
                return BuildingVaultDescent10;
            }
            if (zoneX == dungeon[0] + 2 && zoneY == dungeon[1]) {
                return BuildingVaultDescent20;
            }
            if (zoneX == dungeon[0] + 1 && zoneY == dungeon[1]) {
                return BuildingVaultDescent30;
            }
            if (zoneX == dungeon[0] && zoneY == dungeon[1]) {
                return BuildingVaultDescent40;
            }
        } else if (dungeonKind == PlaceGrotto) {
            if ((zoneX == dungeon[0] || zoneX == dungeon[0] - 1) && (zoneY == dungeon[1] || zoneY == dungeon[1] - 1)) {
                return BuildingSewerOrGrottoEntrance;
            }
        } else if (dungeonKind == PlacePyramid) {
            if ((zoneX == dungeon[0] || zoneX == dungeon[0] - 1) && (zoneY == dungeon[1] || zoneY == dungeon[1] - 1)) {
                return BuildingPyramidEntrance;
            }
        } else if (dungeonKind == PlaceTemple) {
            if ((zoneX == dungeon[0] || zoneX == dungeon[0] - 1) && (zoneY == dungeon[1] || zoneY == dungeon[1] - 1)) {
                return BuildingTempleEntrance;
            }
        } else if (dungeonKind == PlaceSewers) {
            if ((zoneX == dungeon[0] || zoneX == dungeon[0] - 1) && (zoneY == dungeon[1] || zoneY == dungeon[1] - 1)) {
                return BuildingSewerOrGrottoEntrance;
            }
        }
    }
    if (info.type != PlaceNone && info.influence > 0.5f) {
        // The place's mission target, up to three squares from the centre, keeps the squares within its reach free.
        int target = missionTargetType(info.type, cellX, cellY, info.large);
        if (target != 0) {
            int targetX = floorDivide(info.center[0], BlocksPerCell) + random(cellX + 561365, cellY) % 7 - 3;
            int targetY = floorDivide(info.center[1], BlocksPerCell) + random(cellX + 66165, cellY) % 7 - 3;
            if (zoneX == targetX && zoneY == targetY) {
                return static_cast<BuildingType>(target);
            }
            int reach = buildingReach(target);
            if ((zoneY - targetY) * (zoneY - targetY) + (zoneX - targetX) * (zoneX - targetX) <
                (reach + 1) * (reach + 1)) {
                return BuildingNone;
            }
        }
    }
    if (info.type == PlaceVillage) {
        int plotX = floorDivide(zoneX, 3);
        int plotY = floorDivide(zoneY, 3);
        int plot = hamletPlot(zoneX, zoneY, EnvironmentTypeMoat, dungeon);
        if (plot != PlotNone) {
            if (plot == PlotField) {
                BiomeSample sample;
                biomeSample(point[0], point[1], sample);
                if (sample.undead(info.type, info.influence) > 0.5f) {
                    return BuildingGraveyard;
                }
                return static_cast<BuildingType>(random(plotX + 5202113, plotY) %
                                                     (BuildingKaleField - BuildingPumpkinField + 1) +
                                                 BuildingPumpkinField);
            } else if ((zoneX % 3 + 3) % 3 != 2 && (zoneY % 3 + 3) % 3 != 2) {
                plasma::Vector<2, int> corner = houseCorner(plotX, plotY);
                if (plot == PlotInn)
                    return static_cast<BuildingType>(plasma::Vector<2, int>(zoneX, zoneY) != corner
                                                         ? BuildingHouseUpperRoom
                                                         : BuildingHouseEntrance);
                if (plot == PlotGuildHall)
                    return static_cast<BuildingType>(
                        plasma::Vector<2, int>(zoneX, zoneY) == corner ? BuildingGuildHallCentre : BuildingGuildHall);
                if (plot == PlotFlightMaster)
                    return static_cast<BuildingType>(
                        plasma::Vector<2, int>(zoneX, zoneY) == corner ? BuildingFlightMasterSite : BuildingNone);
                if (plot < PlotSmithy || plot > PlotTailorsShop) {
                    bool built;
                    if (zoneX == corner[0]) {
                        built = zoneY == corner[1] || random(plotX + 4560941, plotY) % 3 == 0;
                    } else {
                        built = zoneY == corner[1] && random(plotX + 277965, plotY) % 3 == 0;
                    }
                    if (built || (random(plotX + 63380, plotY) & 7) == 0) {
                        if (plasma::Vector<2, int>(zoneX, zoneY) != corner) {
                            return static_cast<BuildingType>(
                                random(zoneX + 1222933, zoneY) % 10 != 0 ? BuildingHouseRoom : BuildingHouseUpperRoom);
                        }
                        return BuildingHouseEntrance;
                    }
                } else {
                    int part = housePart(zoneX, zoneY);
                    if (plot == PlotSmithy && part == 1) {
                        return BuildingHouseCourtyard;
                    }
                    if (part < (plot == PlotSmithy ? 4 : 3) || (random(zoneX + 64712, zoneY) & 1) != 0) {
                        switch (random(zoneX + 3214343, zoneY) % 3) {
                        case 0:
                            return BuildingHouseCourtyard;
                        case 1:
                            return static_cast<BuildingType>(plasma::Vector<2, int>(zoneX, zoneY) != corner
                                                                 ? BuildingHouseRoom
                                                                 : BuildingHouseEntrance);
                        case 2:
                            return BuildingHousePavilion;
                        }
                    }
                }
            }
        } else {
            if (zoneX % 2 == 0 && zoneY % 2 == 0 && random(zoneX + 467931, zoneY) % 10 == 0 && info.influence > 0.5f) {
                BiomeSample sample;
                biomeSample(point[0], point[1], sample);
                return static_cast<BuildingType>(
                    sample.temperature(info.type, info.influence) > 0.75f ? BuildingPalmPlaza : BuildingOakPlaza);
            }
        }
    }
    if (info.type == PlaceAncientRuins) {
        int plotX = floorDivide(zoneX, 3);
        int plotY = floorDivide(zoneY, 3);
        if (isPlotInner(plotX, plotY, PlaceAncientRuins, dungeon)) {
            unsigned int hash = random(plotX, plotY);
            if (hash & 1) {
                hash /= 2;
                if (hash % 20 == 0) {
                    if ((zoneX % 3 + 3) % 3 == 1 && (zoneY % 3 + 3) % 3 == 1) {
                        return static_cast<BuildingType>((hash / 20 & 1) + BuildingSanctuary);
                    }
                } else if ((zoneX % 3 + 3) % 3 != 2 && (zoneY % 3 + 3) % 3 != 2) {
                    return static_cast<BuildingType>((random(zoneX + 5064334, zoneY) & 1) + BuildingRuinedPavilion);
                }
            }
        }
    }
    if (info.type == PlaceCircleOfPower) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        if (zoneX == centerX) {
            if (zoneY == centerY) {
                return BuildingInscribedMenhir;
            }
            if (zoneY == centerY + 1) {
                return BuildingCircleOfPowerSite;
            }
        }
        if ((zoneX == centerX - 4 && zoneY == centerY) || (zoneX == centerX + 4 && zoneY == centerY) ||
            (zoneX == centerX && (zoneY == centerY - 4 || zoneY == centerY + 4)) ||
            (zoneX == centerX - 3 && zoneY == centerY - 3) ||
            (zoneX == centerX + 3 && (zoneY == centerY - 3 || zoneY == centerY + 3)) ||
            (zoneX == centerX - 3 && zoneY == centerY + 3)) {
            return BuildingMenhir;
        }
        if (placeInfluence(PlaceCircleOfPower, info.large,
                           placeSquaredDistance(info.center, blockX + BlocksPerCell / 2, blockY + BlocksPerCell / 2)) >
            0.5f) {
            return BuildingNone;
        }
    }
    if (info.type == PlaceCamp || info.type == PlaceBanditCamp) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        if (zoneX == centerX && zoneY == centerY) {
            return BuildingCampfireSite;
        }
        if ((zoneY - centerY) * (zoneY - centerY) + (zoneX - centerX) * (zoneX - centerX) < 9 &&
            (random(zoneX + 521109, zoneY) & 3) == 0) {
            return BuildingCampEquipmentSite;
        }
    }
    if (info.type == PlacePyramid) {
        if (abs(zoneX - dungeon[0]) <= 7 && abs(zoneY - dungeon[1]) < 7) {
            return BuildingClearedMonumentGround;
        }
    }
    if (info.type == PlaceTemple) {
        if (abs(zoneX - dungeon[0]) <= 7 && abs(zoneY - dungeon[1]) < 7) {
            return BuildingClearedMonumentGround;
        }
    }
    if (info.type == PlaceArena) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        if (abs(zoneX - centerX) == 6 && abs(zoneY - centerY) == 6) {
            return BuildingArenaTower;
        }
        if (zoneX == centerX - 3 && zoneY == centerY) {
            return BuildingArenaPodium;
        }
        if ((zoneY - centerY) * (zoneY - centerY) + (zoneX - centerX) * (zoneX - centerX) <= 112) {
            return BuildingClearedMonumentGround;
        }
    }
    if (info.type == PlaceManaPump) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        if (zoneX == centerX && zoneY == centerY) {
            return BuildingManaPump;
        }
        bool wide = info.large;
        int size = 15;
        if (wide)
            size = 31;
        int edge = size - 1;
        if (zoneX == centerX || zoneY == centerY) {
            if (abs(zoneX - centerX) <= edge && abs(zoneY - centerY) <= edge) {
                if (abs(zoneX - centerX) == edge) {
                    return BuildingManaPipeEndpoint;
                }
                int period = 64;
                if (wide)
                    period = 128;
                int phaseX = (period + zoneX % period) % period;
                if (phaseX == 0) {
                    return BuildingManaPipeEndpoint;
                }
                if (phaseX == period - 1) {
                    return BuildingManaPipeEndpoint;
                }
                if (abs(zoneY - centerY) == edge) {
                    return BuildingManaPipeEndpoint;
                }
                int phaseY = (period + zoneY % period) % period;
                if (phaseY == 0 || phaseY == period - 1) {
                    return BuildingManaPipeEndpoint;
                }
                return BuildingManaPipe;
            }
        }
        if ((abs(zoneX - centerX) <= 1 || abs(zoneY - centerY) <= 1) && abs(zoneX - centerX) <= size &&
            abs(zoneY - centerY) <= size) {
            return BuildingNone;
        }
        if (placeInfluence(PlaceManaPump, wide, placeSquaredDistance(info.center, blockX + 8, blockY + 8)) > 0.5f) {
            return BuildingNone;
        }
    }
    if (info.type == PlaceSanctuary) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        if (zoneX == centerX && zoneY == centerY) {
            return static_cast<BuildingType>((random(cellX, cellY) & 1) + BuildingSanctuary);
        }
        if ((zoneY - centerY) * (zoneY - centerY) + (zoneX - centerX) * (zoneX - centerX) < 9) {
            return BuildingNone;
        }
    }
    if (info.type == PlaceMenhir) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        if (zoneX == centerX && zoneY == centerY) {
            return BuildingInscribedMenhir;
        }
    }
    if (info.type == PlaceAltar) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        if (zoneX == centerX && zoneY == centerY) {
            return BuildingAltar;
        }
        if ((zoneY - centerY) * (zoneY - centerY) + (zoneX - centerX) * (zoneX - centerX) < 9) {
            return BuildingNone;
        }
    }
    if (info.type == PlaceHouse || info.type == PlaceInn) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        if (zoneX == centerX && zoneY == centerY) {
            return BuildingHouseEntrance;
        }
        if (abs(zoneX - centerX) < 2 && abs(zoneY - centerY) < 2) {
            int sideX = centerX + (random(cellX + 463276, cellY) & 1) * 2 - 1;
            int sideY = (random(cellX + 1160520, cellY) & 1) * 2 - 1;
            if ((zoneX == sideX && zoneY == centerY) || (zoneX == centerX && zoneY == sideY + centerY) ||
                (zoneX == sideX && zoneY == sideY + centerY)) {
                bool alongX = (random(cellX + 438504, cellY) & 1) != 0;
                bool alongY = (random(cellX + 729652, cellY) & 1) != 0;
                if ((alongX && zoneX == sideX && zoneY == centerY) ||
                    (alongY && zoneX == centerX && zoneY == sideY + centerY) ||
                    ((alongX || alongY) && zoneX == sideX && zoneY == sideY + centerY)) {
                    return static_cast<BuildingType>((random(zoneX + 146189, zoneY) & 1) + BuildingHouseRoom);
                }
            }
        }
    }
    if (info.type == PlaceCrypt) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        if (zoneX == centerX && zoneY == centerY) {
            return BuildingLoreCrypt;
        }
        if (abs(zoneX - centerX) < 2 && abs(zoneY - centerY) < 2) {
            int sideX = centerX + (random(cellX + 463276, cellY) & 1) * 2 - 1;
            int sideY = (random(cellX + 1160520, cellY) & 1) * 2 - 1;
            if ((zoneX == sideX && zoneY == centerY) || (zoneX == centerX && zoneY == sideY + centerY) ||
                (zoneX == sideX && zoneY == sideY + centerY)) {
                bool alongX = (random(cellX + 438504, cellY) & 1) != 0;
                bool alongY = (random(cellX + 729652, cellY) & 1) != 0;
                if ((alongX && zoneX == sideX && zoneY == centerY) ||
                    (alongY && zoneX == centerX && zoneY == sideY + centerY) ||
                    ((alongX || alongY) && zoneX == sideX && zoneY == sideY + centerY)) {
                    switch (random(zoneX + 146189, zoneY) & 1) {
                    default:
                        return BuildingCryptRoom;
                    case 1:
                        return BuildingCryptUpperRoom;
                    }
                }
            }
        }
    }
    if (info.type == PlaceFarm) {
        int plotY = floorDivide(zoneY, 3) * (3 * BlocksPerCell);
        int plotX = floorDivide(zoneX, 3) * (3 * BlocksPerCell);
        float influence = placeInfluence(
            PlaceFarm, info.large, placeSquaredDistance(info.center, plotX + BlocksPerCell, plotY + BlocksPerCell));
        if (influence > 0.5f) {
            int centerX = floorDivide(info.center[0], BlocksPerCell);
            int centerY = floorDivide(info.center[1], BlocksPerCell);
            if (zoneX == centerX && zoneY == centerY) {
                return BuildingHouseEntrance;
            }
            if (abs(zoneX - centerX) < 2 && abs(zoneY - centerY) < 2) {
                int sideX = centerX + (random(cellX + 463276, cellY) & 1) * 2 - 1;
                int sideY = (random(cellX + 1160520, cellY) & 1) * 2 - 1;
                if ((zoneX == sideX && zoneY == centerY) || (zoneX == centerX && zoneY == sideY + centerY) ||
                    (zoneX == sideX && zoneY == sideY + centerY)) {
                    bool alongX = (random(cellX + 438504, cellY) & 1) != 0;
                    bool alongY = (random(cellX + 729652, cellY) & 1) != 0;
                    if ((alongX && zoneX == sideX && zoneY == centerY) ||
                        (alongY && zoneX == centerX && zoneY == sideY + centerY) ||
                        ((alongX || alongY) && zoneX == sideX && zoneY == sideY + centerY)) {
                        return static_cast<BuildingType>((random(zoneX + 146189, zoneY) & 1) + 22);
                    }
                }
            } else {
                int hashX = floorDivide(zoneX, 3);
                int hashY = floorDivide(zoneY, 3);
                if ((random(hashX + 4847604, hashY) & 3) != 0 &&
                    streetAmount(plotX + 16, plotY + 16, 2, influence) < 0.4f) {
                    return static_cast<BuildingType>(random(hashX + 5202113, hashY) % 7 + 27);
                }
            }
        }
    }
    if (info.type == PlaceTower) {
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        if ((zoneY - centerY) * (zoneY - centerY) + (zoneX - centerX) * (zoneX - centerX) < 16) {
            if (zoneX == floorDivide(info.center[0], BlocksPerCell) &&
                zoneY == floorDivide(info.center[1], BlocksPerCell)) {
                return BuildingTower;
            }
            return BuildingNone;
        }
    }
    if (info.type == PlaceCastle) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        int result = castleZoneType(zoneX, zoneY, cellX, cellY, castleDirection(cellX, cellY),
                                    castleRotation(cellX, cellY), castleSize(cellX, cellY), centerX, centerY);
        if (result != 0) {
            return static_cast<BuildingType>(result);
        }
    }
    if (info.type == PlaceCave) {
        bool wide = info.large;
        float influence = placeInfluence(PlaceCave, wide,
                                         placeSquaredDistance(placeCenter(cellX, cellY, wide), blockX + 8, blockY + 8));
        if (influence > 0.5f) {
            if (zoneX % 5 == 0 || zoneY % 5 == 0 || random(zoneX / 5, zoneY / 5) % 5 == 0) {
                if (!(height(blockX + 8, blockY + 8, 0) < 1.0f) &&
                    !(streetAmount(blockX + 8, blockY + 8, info.type, influence) > 0.5f)) {
                    if ((zoneX + zoneY * 2 + zoneY) % 5 == 0) {
                        return BuildingDungeonTower;
                    }
                    return BuildingDungeonHall;
                }
                return BuildingNone;
            }
            // Tests a second pattern here (every fifth row and column from 2, or another random fifth of the cells)
            // and builds nothing either way.
            if (zoneX % 5 == 2 || zoneY % 5 == 2 || random(zoneX / 5 + 561894, zoneY / 5) % 5 == 0) {
                return BuildingNone;
            }
            return BuildingNone;
        }
    }
    if (info.type == PlaceOldHut) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        if (zoneX == centerX && zoneY == centerY) {
            return BuildingOldHutEntrance;
        }
        if ((zoneX == centerX + 1 && zoneY == centerY) || (zoneX == centerX && zoneY == centerY + 1)) {
            return BuildingOldHutRoom;
        }
        if (zoneX == centerX + 1 && zoneY == centerY + 1) {
            return BuildingOldHutChimney;
        }
    }
    if (info.type == PlaceTree) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        if (zoneX == centerX && zoneY == centerY) {
            return BuildingGreatOak;
        }
    }
    if (info.type == PlaceRock) {
        int centerX = floorDivide(info.center[0], BlocksPerCell);
        int centerY = floorDivide(info.center[1], BlocksPerCell);
        if (abs(centerX - zoneX) < 2 && abs(centerY - zoneY) < 2) {
            if (zoneX == centerX && zoneY == centerY) {
                return BuildingTallRock;
            }
            return BuildingRock;
        }
    }
    if (info.type == PlaceGraveyard) {
        int plotY = floorDivide(zoneY, 3);
        int plotX = floorDivide(zoneX, 3);
        bool wide = info.large;
        int groundY = plotY * 48 + 16;
        int groundX = plotX * 48 + 16;
        float influence = placeInfluence(PlaceGraveyard, wide, placeSquaredDistance(info.center, groundX, groundY));
        if (influence > 0.5f) {
            int centerY = floorDivide(info.center[1], 128) * 8 + 4;
            int centerX = floorDivide(info.center[0], 128) * 8 + 4;
            if (abs(zoneX - centerX) <= 1 && abs(zoneY - centerY) <= 1) {
                if ((zoneX == centerX || zoneX == centerX - 1) && (zoneY == centerY || zoneY == centerY - 1) &&
                    dungeonType(cellX, cellY, 9, wide) != PlaceNone) {
                    return BuildingCatacombsEntrance;
                }
                switch (random(cellX + 576141, cellY) & 1) {
                case 0:
                    centerX++;
                    break;
                case 1:
                    centerY++;
                    break;
                }
                if (zoneX == centerX && zoneY == centerY) {
                    return BuildingCryptEntrance;
                }
                if ((random(zoneX + 1222933, zoneY) & 7) == 1) {
                    return BuildingCryptUpperRoom;
                }
                return BuildingCryptRoom;
            }
            if ((random(floorDivide(zoneX, 3) + 4847604, floorDivide(zoneY, 3)) & 3) != 0 &&
                streetAmount(groundX, groundY, 9, influence) < 0.4f) {
                return BuildingGraveyard;
            }
        }
    }
    // The region's site: one special building in every 64 x 64 squares, with free squares around it.
    int regionX = floorDivide(zoneX, ZonesPerRegion);
    int regionY = floorDivide(zoneY, ZonesPerRegion);
    plasma::Vector<2, int> site = siteInZone(regionX, regionY);
    int reach = 1;
    int building;
    if (((regionX + regionY) % 2) & 1) {
        building = BuildingShrineOfLife;
    } else {
        switch (random(site[0] + 568865, site[1]) % 11) {
        case 1:
            building = BuildingCave;
            reach = 2;
            break;
        case 2:
            building = BuildingLoreCrypt;
            reach = 2;
            break;
        case 3:
            building = BuildingInscribedMenhir;
            break;
        case 4:
            building = BuildingSanctuary;
            reach = 2;
            break;
        case 5:
            building = BuildingRaisedSanctuary;
            reach = 2;
            break;
        case 6:
            building = BuildingRockLorePavilion;
            break;
        case 7:
            building = BuildingRockTreasure;
            break;
        case 8:
            building = BuildingBuriedTreasureChamber;
            break;
        case 9:
            building = BuildingTreasureTrunk;
            break;
        case 10:
            building = BuildingSandstoneTreasureChamber;
            reach = 2;
            break;
        default:
            building = BuildingFloatingIslandTreasurePavilion;
            break;
        }
    }
    if (plasma::Vector<2, int>(zoneX, zoneY) == site) {
        if (building == BuildingTreasureTrunk && height(site[0] * BlocksPerCell + BlocksPerCell / 2,
                                                        site[1] * BlocksPerCell + BlocksPerCell / 2, 0) < 0.0f) {
            return BuildingUnderwaterTreasureSite;
        }
        return static_cast<BuildingType>(building);
    }
    bool nearSite = abs(zoneX - site[0]) <= reach && abs(zoneY - site[1]) <= reach;
    if (building == BuildingFloatingIslandTreasurePavilion) {
        // Type 60 has a second building four squares along x, on the side that stays in the same 64-square group.
        plasma::Vector<2, int> second = site;
        second[0] += 4;
        if (floorDivide(second[0], ZonesPerRegion) != floorDivide(zoneX, ZonesPerRegion)) {
            second = plasma::Vector<2, int>(site[0] - 4, site[1]);
        }
        if (plasma::Vector<2, int>(zoneX, zoneY) == second) {
            return BuildingBirdStatuePavilion;
        }
        if (abs(zoneX - second[0]) < 2 && abs(zoneY - second[1]) < 2) {
            return BuildingNone;
        }
    }
    if (nearSite) {
        return BuildingNone;
    }
    int type = info.type;
    if (!(type != PlaceNone && info.influence > 0.5f) || type == PlaceCanyon || type == PlaceRocks) {
        // Outside places (and inside canyons and rocks, more often): a rare building of type 10..12, or a cave
        // entrance.
        int rarity = 600;
        if (type == PlaceCanyon && info.influence > 0.5f) {
            rarity = 50;
        } else if (type == PlaceRocks && info.influence > 0.5f) {
            rarity = 20;
        }
        if (random(zoneX + 569118, zoneY) % rarity == ItemRarityWhite) {
            if (height(point[0], point[1], 0) < 1.0f) {
                return BuildingNone;
            }
            float road = streetAmount(point[0], point[1], info.type, info.influence);
            if (road > 0.4f ||
                valleyWater(point[0], point[1], road, 0.0f, 0.0f, 0.0f, 0.0f, (AreaInfo *)&info) > 0.4f) {
                return BuildingNone;
            }
            if ((info.type == PlaceCanyon || info.type == PlaceRocks) && info.influence > 0.5f) {
                return static_cast<BuildingType>(
                    random(zoneX + 50356050, zoneY) % (BuildingTallRock - BuildingHill + 1) + BuildingHill);
            }
            return static_cast<BuildingType>((random(zoneX + 50356050, zoneY) & 1) + BuildingHill);
        }
        if (random(zoneX + 123608, zoneY) % 200 == 0) {
            float influence = info.influence;
            if (caveDensity(point[0], point[1], type, influence) > 0.4f || (type != PlaceNone && influence > 0.5f)) {
                return BuildingNone;
            }
            if (height(point[0], point[1], 0) < 1.0f) {
                return BuildingNone;
            }
            float road = streetAmount(point[0], point[1], info.type, info.influence);
            if (road > 0.5f) {
                return BuildingNone;
            }
            if (valleyWater(point[0], point[1], road, 0.0f, 0.0f, 0.0f, 0.0f, (AreaInfo *)&info) > 0.5f) {
                return BuildingNone;
            }
            return BuildingBoulder;
        }
    }
    // The street buildings 1..5, 19 and 20: one square in four where the settlement density (noise, raised by forest)
    // is above a half.
    if ((type == PlaceLavaLake || type == PlaceCrater || type == PlaceVolcano) && info.influence > 0.5f) {
        return BuildingNone;
    }
    if ((random(zoneX + 584105, zoneY) & 3) != 0) {
        return BuildingNone;
    }
    BiomeSample sample;
    biomeSample(point[0], point[1], sample);
    float density = (noise(zoneX * 0.08 + 8989.0, zoneY * 0.08 + 60466.0) + 1.0f) * 0.25f;
    density += sample.forest() * 0.35f;
    density += noise(zoneX * 2 + 98984, zoneY * 2 + 6076574) * 0.5f;
    type = info.type;
    if ((type == PlaceForest || type == PlaceDarkForest || type == PlaceEnchantedForest || type == PlaceGrove) &&
        info.influence > 0.5f) {
        density = noise(zoneX * 2 + 98984, zoneY * 2 + 676574) * 0.5f + 0.75f;
    } else if (sample.desertness() > 0.5f) {
        return BuildingNone;
    }
    if (type == PlacePlain && info.influence > 0.5f) {
        density *= 0.7f;
    }
    if (type == PlaceBeach && info.influence > 0.5f) {
        density *= 0.6f;
    }
    if (type == PlaceGlade && info.influence > 0.5f) {
        return BuildingNone;
    }
    if (!(density > 0.5f)) {
        return BuildingNone;
    }
    float ground = height(point[0], point[1], 0);
    if (ground < 1.0f) {
        return BuildingNone;
    }
    float road = streetAmount(point[0], point[1], info.type, info.influence);
    if (road > 0.5f) {
        return BuildingNone;
    }
    if (valleyWater(point[0], point[1], road, 0.0f, 0.0f, sample.wetness(), 0.0f, (AreaInfo *)&info) > 0.5f) {
        return BuildingNone;
    }
    unsigned int hash = random(zoneX + 302796, zoneY);
    if (hash % 30 == 0) {
        return static_cast<BuildingType>((random(zoneX + 24878966, zoneY) & 1) != 0 ? BuildingStump
                                                                                    : BuildingFallenTrunk);
    }
    float warmth = sample.temperature(info.type, info.influence);
    if ((warmth > 0.75f && (ground < 20.0f || (hash & 7) == 0)) ||
        (warmth > 0.6f && ground < 20.0f && (hash & 3) == 0)) {
        return BuildingPalm;
    }
    if (warmth > 0.8f && sample.humidity() > 0.8f) {
        return BuildingJungleTree;
    }
    if (borderNoise(blockX, blockY, warmth) > 0.6f) {
        return BuildingBirch;
    }
    if (warmth < 0.8f && random(zoneX + 49845325, zoneY) % 10 == 0) {
        return BuildingBirch;
    }
    if (warmth < 0.7f) {
        if (findRegionNoise(blockX, blockY, warmth) > 0.3f) {
            return BuildingEvergreen;
        }
        if (warmth < 0.8f && random(zoneX + 311040, zoneY) % 10 == 0) {
            return BuildingEvergreen;
        }
    }
    return BuildingOak;
}

// A castle part's height: four blocks per step, floor noise 1.5 and 5.
int World::streetHeightOffset(int zoneX, int zoneY, int storey)
{
    switch (zoneBuildingType(zoneX, zoneY)) {
    case BuildingShrineDescent10:
    case BuildingMineDescent10:
    case BuildingVaultDescent10:
        return -10;
    case BuildingShrineDescent20:
    case BuildingMineDescent20:
    case BuildingVaultDescent20:
        return -20;
    case BuildingShrineDescent30:
    case BuildingMineDescent30:
    case BuildingVaultDescent30:
        return -30;
    case BuildingShrineDescent40:
    case BuildingMineDescent40:
    case BuildingVaultDescent40:
        return -40;
    case BuildingCastleCell:
    case BuildingCastleGate: {
        int cellX = floorDivide(floorDivide(zoneX * BlocksPerCell, BlocksPerPlace), 2) * 2;
        int cellY = floorDivide(floorDivide(zoneY * BlocksPerCell, BlocksPerPlace), 2) * 2;
        plasma::Vector<2, int> center = placeCenter(cellX, cellY, isLargePlace(cellX, cellY));
        int centerX = floorDivide(center[0], BlocksPerCell);
        int centerY = floorDivide(center[1], BlocksPerCell);
        int direction = castleDirection(cellX, cellY);
        int rotation = castleRotation(cellX, cellY);
        int size = castleSize(cellX, cellY);
        int part = castlePart(zoneX, zoneY, direction, rotation, size, centerX, centerY);
        if (part == 0 || part == 1 || part == 4) {
            return 0;
        }
        return (int)((noise(floorDivide(zoneX, 4) * 1.5, floorDivide(zoneY, 4) * 1.5) + 1.0f) * 5.0f);
    }
    }
    return 0;
}

// The street point: a hashed block (salts 5619113, 639859) of the cell, four blocks in, one in seven.
plasma::Vector<2, int> World::streetPoint(int zoneX, int zoneY)
{
    return plasma::Vector<2, int>(zoneX * 16 + 4 + (random(zoneX + 5619113, zoneY) & 7),
                                  zoneY * 16 + 4 + (random(zoneX + 639859, zoneY) & 7));
}

int World::streetSides(int x, int y)
{
    int sides = 0;
    if (isJoined(x, y, x - 1, y)) {
        sides = JoinedSideMinusX;
    }
    if (isJoined(x, y, x + 1, y)) {
        sides |= JoinedSidePlusX;
    }
    if (isJoined(x, y, x, y - 1)) {
        sides |= JoinedSideMinusY;
    }
    if (isJoined(x, y, x, y + 1)) {
        sides |= JoinedSidePlusY;
    }
    return sides;
}

int World::structureLayer(int x, int y, int level, int type)
{
    if (type == BuildingNone) {
        type = zoneBuildingType(x, y);
    }
    if ((type == BuildingCastleTower && level > 0) ||
        ((type == BuildingCatacombsEntrance || type == BuildingSewerOrGrottoEntrance ||
          type == BuildingPyramidEntrance || type == BuildingTempleEntrance) &&
         level == 0)) {
        return 2;
    }
    if (type == BuildingCastleTower || level <= 0) {
        return 0;
    }
    return 1;
}

// A structure's level: castles and towers at the place centre; dungeons at the highest of 3 x 3 samples over their two
// cells; the floating island and rock treasures 80 and the rock lore pavilion 44 above the terrain; the temple
// entrance 24 under the monument apex (100 above the elevation); flooded cells at the village height, others at the
// highest corner.
int World::structureLevel(int x, int y, int type)
{
    if (type == BuildingNone) {
        type = zoneBuildingType(x, y);
    }
    if (isCastleBuilding(type) || type == BuildingTower) {
        int cellX = floorDivide(x * BlocksPerCell, BlocksPerPlace);
        int cellY = floorDivide(y * BlocksPerCell, BlocksPerPlace);
        bool large = isLargePlace(cellX, cellY);
        locationType(cellX, cellY, large);
        plasma::Vector<2, int> center = placeCenter(cellX, cellY, large);
        return (int)(height(center[0], center[1], 0) + 1.0f);
    }
    if (isDungeonBuilding(type)) {
        int result = 0;
        int startY = partIndex(y, 2) * (2 * BlocksPerCell);
        int startX = partIndex(x, 2) * (2 * BlocksPerCell);
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                int level = floorToInt(height(startX + i * BlocksPerCell, startY + j * BlocksPerCell, 0));
                if (result < level) {
                    result = level;
                }
            }
        }
        return result;
    }
    int blockX = x * BlocksPerCell;
    int blockY = y * BlocksPerCell;
    int centerX = blockX + BlocksPerCell / 2;
    int centerY = blockY + BlocksPerCell / 2;
    if (type == BuildingFloatingIslandTreasurePavilion) {
        return (int)(std::max(height(centerX, centerY, 0), 0.0f) + 80.0f);
    }
    if (type == BuildingRockLorePavilion) {
        return (int)(std::max(height(centerX, centerY, 0), 0.0f) + 44.0f);
    }
    if (type == BuildingRockTreasure) {
        return (int)(std::max(height(centerX, centerY, 0), 0.0f) + 80.0f);
    }
    if (isPlainBuilding(type)) {
        return (int)height(centerX, centerY, 0);
    }
    BiomeSample sample;
    biomeSample(centerX, centerY, sample);
    float mountains = sample.mountains();
    AreaInfo info;
    fillAreaInfo(centerX, centerY, info);
    if (type == BuildingTempleEntrance) {
        biomeSample(info.center[0], info.center[1], sample);
        return (int)(sample.biomeElevation(info.type, 1.0f) + 100.0f - 24.0f);
    }
    if (type == BuildingPyramidEntrance) {
        biomeSample(info.center[0], info.center[1], sample);
        return (int)(sample.biomeElevation(info.type, 1.0f) + 1.0f);
    }
    if (type == BuildingSewerOrGrottoEntrance) {
        if (zoneBuildingType(x + 1, y) == BuildingSewerOrGrottoEntrance) {
            blockX += BlocksPerCell;
        }
        if (zoneBuildingType(x, y + 1) == BuildingSewerOrGrottoEntrance) {
            blockY += BlocksPerCell;
        }
        return (int)(height(blockX, blockY, 0) + 2.0f);
    }
    float wetlands = sample.wetness();
    float undead = sample.undead(info.type, info.influence);
    float firelands = sample.lavaWeight(info.type, info.influence);
    float road = streetAmount(centerX, centerY, info.type, info.influence);
    if (valleyWater(centerX, centerY, road, firelands, undead, wetlands, mountains, &info) > ChannelThreshold) {
        float level = villageHeight(centerX, centerY, sample.biomeElevation(info.type, info.influence), &info) + 1.0f;
        return (std::max)(0, floorToInt(level) / 2 * 2 + 1);
    }
    float heightA = height(blockX + BlocksPerCell, blockY + BlocksPerCell, 0);
    float heightB = height(blockX, blockY + BlocksPerCell, 0);
    float heightC = height(blockX + BlocksPerCell, blockY, 0);
    float heightD = height(blockX, blockY, 0);
    float top = height(centerX, centerY, 0);
    const float &highest = (std::max)(heightD, (std::max)(heightC, (std::max)(heightB, heightA)));
    top = std::max(top, highest);
    return floorToInt(std::max(top + 1.0f, 0.0f) * 0.5f * 2.0f + 2.0f + 0.0f);
}

// The surface: lava, poison, sand, paths and beaches above 0.5 of their weights, faded towards grey (128) by the fade,
// and cliff exposure blended at three per block of rise.
BlockProperties<float> World::surfaceBlock(int x, int y, int z, float height, float heightWest, float heightEast,
                                           float heightSouth, float heightNorth, float cliffs, float temperature,
                                           float humidity, float sand, float fade, float path, float firelands,
                                           float undead, float enchantedForest, float lava, float poison)
{
    if (lava > 0.5f) {
        BlockProperties<float> block(lavaColor(x, y), BlockLava);
        block.lightLevel = 255;
        return block;
    }
    if (poison > 0.5f) {
        BlockProperties<float> block(muckColor(x, y), BlockPoison);
        block.lightLevel = 255;
        return block;
    }
    float riseWest = heightWest - height;
    float riseEast = heightEast - height;
    float riseSouth = heightSouth - height;
    float riseNorth = heightNorth - height;
    if ((z < floorToInt(height) && z < floorToInt(heightWest) && z < floorToInt(heightEast) &&
         z < floorToInt(heightSouth) && z < floorToInt(heightNorth)) ||
        sand > 0.5f || path > 0.5f || isBeach(x, y, height)) {
        BlockProperties<float> block =
            columnBlock(x, y, z, cliffs, temperature, humidity, firelands, undead, enchantedForest);
        if (fade > 0.0f) {
            block.color += (plasma::Vector<3, float>(128.0f, 128.0f, 128.0f) - block.color) * fade;
        }
        if (path > 0.5f) {
            block.type = BlockPath;
        }
        return block;
    }
    float lowest = std::min(riseWest, 0.0f);
    float highest = std::max(riseWest, 0.0f);
    lowest = std::min(riseEast, lowest);
    highest = std::max(riseEast, highest);
    lowest = std::min(riseSouth, lowest);
    highest = std::max(riseSouth, highest);
    lowest = std::min(riseNorth, lowest);
    highest = std::max(riseNorth, highest);
    BlockProperties<float> block =
        coverBlock(x, y, z, height, temperature, humidity, sand, fade, path, firelands, undead, enchantedForest);
    if ((z < floorToInt(height) && lowest < -1.0f) || highest > 1.0f) {
        float steepness;
        if (z < floorToInt(height) && lowest < -1.0f) {
            steepness = -lowest;
        } else {
            steepness = highest;
        }
        float blend = std::min(1.0f, (steepness - 1.0f) * 3.0f);
        blend = std::max(0.0f, blend);
        BlockProperties<float> ground =
            columnBlock(x, y, z, cliffs, temperature, humidity, firelands, undead, enchantedForest);
        block.color = ground.color * blend + block.color * (1.0f - blend);
        if (blend > 0.5f) {
            block.type = ground.type;
            block.flags = ground.flags;
        }
    }
    return block;
}

// Temperature: the hot and cold biomes' weights (BiomeBlendRate) plus noise (0.0002; offsets 4234234, 55543) at 0.35,
// a quarter of the lava weight.
float World::BiomeSample::temperature(int areaType, float areaInfluence)
{
    float nearestHotBiomeSquaredDistance = -1.0f;
    float nearestColdBiomeSquaredDistance = -1.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if ((biomes[i][j] == BiomeJungle || biomes[i][j] == BiomeDesert || biomes[i][j] == BiomeSavannah ||
                 biomes[i][j] == BiomeFirelands) &&
                (nearestHotBiomeSquaredDistance < 0.0f || nearestHotBiomeSquaredDistance > squaredDistances[i][j])) {
                nearestHotBiomeSquaredDistance = squaredDistances[i][j];
            }
            if ((biomes[i][j] == BiomeSnowlands || biomes[i][j] == BiomeSnowForest) &&
                (nearestColdBiomeSquaredDistance < 0.0f || nearestColdBiomeSquaredDistance > squaredDistances[i][j])) {
                nearestColdBiomeSquaredDistance = squaredDistances[i][j];
            }
        }
    }
    float hotBiomeWeight = weight(nearestHotBiomeSquaredDistance, BiomeBlendRate);
    float coldBiomeWeight = weight(nearestColdBiomeSquaredDistance, BiomeBlendRate);
    if (areaType == PlaceVolcano) {
        hotBiomeWeight += areaInfluence;
        coldBiomeWeight *= 1.0f - areaInfluence;
    } else if (areaType == PlaceLavaLake || areaType == PlaceCrater) {
        hotBiomeWeight += areaInfluence * 0.25f;
        coldBiomeWeight *= 1.0f - areaInfluence;
    }
    return (std::max)(0.0f, (std::min)(1.0f, noise(x * 0.0002 + 4234234.0, y * 0.0002 + 55543.0) * 0.35f +
                                                 (hotBiomeWeight * 2.0f + 0.5f) - coldBiomeWeight));
}

int World::towerStoreyCount(int x, int y)
{
    return 4;
}

// The landmark sizes by hashes of the cell: a tower rises 40..139 (salt 381114), its radius 10..29 (2446601); a
// landmark is 10..21 high (19718956) and 10..29 wide (50056340), times the scale.
int World::townSizeA(int x, int y)
{
    return random(x + 381114, y) % 40 + 100;
}

int World::townSizeB(int x, int y)
{
    return random(x + 2446601, y) % 10 + 20;
}

int World::townSizeC(int x, int y, float scale)
{
    return random(x + 19718956, y) % 10 + 12;
}

int World::townSizeD(int x, int y, float scale)
{
    return random(x + 50056340, y) % 10 + 20;
}

float World::BiomeSample::undead(int areaType, float areaInfluence)
{
    float distance = -1.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if ((biomes[i][j] == BiomeDeadlands || biomes[i][j] == BiomeDarkWoods) &&
                (distance < 0.0f || distance > squaredDistances[i][j])) {
                distance = squaredDistances[i][j];
            }
        }
    }
    float result = 0.0f;
    if (distance >= 0.0f && distance >= nearestSquaredDistance) {
        result = std::max(1.0f - (distance - nearestSquaredDistance) * BiomeBlendRate * BiomeBlendRate, 0.0f);
        if (!(1.0f > result)) {
            return result;
        }
    }
    if (areaType == PlaceDarkForest) {
        return std::min(1.0f, result + areaInfluence);
    }
    return result;
}

void World::updateCollision(Creature *creature)
{
    if (creature->m_entityData.hp < 0.0f || creature->m_entityData.binaryToggles.active ||
        creature->m_entityData.binaryToggles.attachedToFurniture)
        return;
    plasma::Vector<3, float> size = creature->m_entityData.appearance.size;
    if (creature->m_entityData.currentAbility == AbilitySit || creature->m_entityData.currentAbility == AbilitySleep) {
        size[0] = SittingFootprint;
        size[1] = SittingFootprint;
    }
    plasma::Vector<3, Fixed> halfSize(size * 0.5f);
    plasma::Vector<3, int> first = fixedToBlock(creature->m_entityData.position - halfSize);
    plasma::Vector<3, int> last = fixedToBlock(creature->m_entityData.position + halfSize);
    for (int x = first[0]; x <= last[0]; ++x)
        for (int y = first[1]; y <= last[1]; ++y)
            if (!field(x, y))
                return;
    bool ignorePassable = creature->findBuff(BuffSpiritworld) != nullptr;
    bool testDecorations = !(creature->m_entityData.flags & CreatureFlagInsideBarrier) &&
                           !creature->m_entityData.binaryToggles.active &&
                           !creature->m_entityData.binaryToggles.attachedToFurniture;
    if (!isBlocked(creature->m_entityData.position, size, ignorePassable, testDecorations))
        return;
    if (isBlocked(creature->m_entityData.position + plasma::Vector<3, Fixed>(Fixed(1), Fixed(0), Fixed(0)), size,
                  ignorePassable, testDecorations)) {
        if (isBlocked(creature->m_entityData.position + plasma::Vector<3, Fixed>(Fixed(-1), Fixed(0), Fixed(0)), size,
                      ignorePassable, testDecorations)) {
            if (isBlocked(creature->m_entityData.position + plasma::Vector<3, Fixed>(Fixed(0), Fixed(1), Fixed(0)),
                          size, ignorePassable, testDecorations)) {
                if (isBlocked(creature->m_entityData.position + plasma::Vector<3, Fixed>(Fixed(0), Fixed(-1), Fixed(0)),
                              size, ignorePassable, testDecorations)) {
                    if (isBlocked(creature->m_entityData.position +
                                      plasma::Vector<3, Fixed>(Fixed(2), Fixed(0), Fixed(0)),
                                  size, ignorePassable, testDecorations)) {
                        if (isBlocked(creature->m_entityData.position +
                                          plasma::Vector<3, Fixed>(Fixed(-2), Fixed(0), Fixed(0)),
                                      size, ignorePassable, testDecorations)) {
                            if (isBlocked(creature->m_entityData.position +
                                              plasma::Vector<3, Fixed>(Fixed(0), Fixed(2), Fixed(0)),
                                          size, ignorePassable, testDecorations)) {
                                if (isBlocked(creature->m_entityData.position +
                                                  plasma::Vector<3, Fixed>(Fixed(0), Fixed(-2), Fixed(0)),
                                              size, ignorePassable, testDecorations)) {
                                    creature->m_entityData.position[2] += Fixed(1);
                                    creature->m_stepOffset = 0.0f;
                                } else {
                                    creature->m_entityData.position[1] += Fixed(-2);
                                }
                            } else {
                                creature->m_entityData.position[1] += Fixed(2);
                            }
                        } else {
                            creature->m_entityData.position[0] += Fixed(-2);
                        }
                    } else {
                        creature->m_entityData.position[0] += Fixed(2);
                    }
                } else {
                    creature->m_entityData.position[1] += Fixed(-1);
                }
            } else {
                creature->m_entityData.position[1] += Fixed(1);
            }
        } else {
            creature->m_entityData.position[0] += Fixed(-1);
        }
    } else {
        creature->m_entityData.position[0] += Fixed(1);
    }
}

// The rivers and wetlands (a hand-tuned noise profile): channel noises (0.0005, 0.01, 0.0025, tributaries at 0.1)
// ridged five blocks wide; lakes from influence 0.4 (over 0.6), elemental rings between 0.75 and 1, suppressed on
// roads (0.4) and in places (0.1, 10).
float World::valleyWater(int x, int y, float road, float firelands, float undead, float wetlands, float mountains,
                         AreaInfo *info)
{
    float result = 0.0f;
    if (firelands < 0.5f && undead < 0.5f) {
        double fineY = y * 0.0005;
        double coarseY = y * 0.01;
        double middleY = y * 0.0025;
        double fineX = x * 0.0005;
        double middleX = x * 0.0025;
        double coarseX = x * 0.01;
        float waterChannelNoise = noise(middleX + 23123.0, middleY + 42432.0) * 0.5f;
        waterChannelNoise += noise(fineX + 23123.0, fineY + 42432.0);
        waterChannelNoise += noise(coarseX + 23123.0, coarseY + 42432.0) * 0.1f;
        float ridge = fabs(waterChannelNoise);
        if (wetlands > 0.00001f) {
            float wetlandTributaryNoise = noise(middleX + 53455.0, middleY + 6547.0) * 0.5f;
            wetlandTributaryNoise += noise(fineX + 5553445.0, fineY + 123321.0);
            wetlandTributaryNoise += noise(coarseX + 6455445.0, coarseY + 33223.0) * 0.1f;
            ridge *= 1.0f - (1.0f - (float)fabs(wetlandTributaryNoise)) * wetlands;
        }
        ridge *= 5.0f;
        if (ridge > 0.5f) {
            ridge = (ridge - 0.5f) * 0.5f;
            ridge = ridge * ridge * ridge + 0.5f;
        }
        float valley = std::max(1.0f - ridge, 0.0f);
        result = (1.0f - std::min(firelands + firelands, 1.0f)) * (valley * valley) *
                 (1.0f - std::min(undead + undead, 1.0f));
    }
    if (info->type != PlaceNone) {
        float influence = info->influence;
        if (info->type == PlaceLake) {
            result += (1.0f - result) * (std::max)(0.0f, (influence - 0.4f) / 0.6f);
        } else {
            float outside = std::max(0.0f, (influence - 0.1f) * 10.0f);
            outside = 1.0f - std::min(1.0f, outside);
            result *= 1.0f - (1.0f - outside * outside);
            if (info->environment == EnvironmentTypeElementalRing) {
                if (landBiome(findRegion(info->center[0], info->center[1])) != BiomeFirelands) {
                    if ((float)(info->radius * info->radius) > info->squaredDistance) {
                        float radius = (float)info->radius;
                        float edge = (float)fabs((sqrtf(info->squaredDistance) - radius * 0.75f) / (radius * 0.25f));
                        edge = std::min(edge, 1.0f);
                        result += (1.0f - edge * edge) * (1.0f - edge * edge) * (1.0f - result);
                    }
                }
            }
        }
    }
    float high = std::max(0.0f, (road - 0.4f) * 10.0f);
    high = std::min(1.0f, high);
    return (1.0f - high) * result;
}

// A village is flattened to its height where the influence is above 0.0001, at least ten blocks up.
float World::villageHeight(int x, int y, float height, const AreaInfo *info)
{
    int type = info->type;
    if (type == PlaceNone) {
        return height;
    }
    if (type == PlaceMountain || type == PlaceManaPump || type == PlaceVolcano || type == PlaceRock ||
        type == PlaceCanyon || type == PlaceBeach) {
        return height;
    }
    float influence = info->influence;
    if (type == PlaceCastle || type == PlaceLake || type == PlaceLavaLake) {
        float t = std::max(0.0f, influence * 2.0f);
        influence = smoothStep(std::min(1.0f, t));
    }
    if (influence > 0.0001f) {
        height += ((std::max)(10.0f, locationWaterWeight(*info)) - height) * influence;
    }
    return height;
}

float World::BiomeSample::wetness()
{
    float distance = -1.0f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (biomes[i][j] == BiomeWetlands && (distance < 0.0f || distance > squaredDistances[i][j])) {
                distance = squaredDistances[i][j];
            }
        }
    }
    return weight(distance, BiomeBlendRate);
}

// Divides value by size, rounding down; used by structureLevel.
int partIndex(int value, int size)
{
    return value >= 0 ? value / size : (value + 1) / size - 1;
}

void World::updateChanges()
{
    for (std::pair<const unsigned long long, Zone *> &entry : m_zones) {
        Zone *zone = entry.second;
        if (zone) {
            zone->m_awaitingData = true;
            if (zone->m_cubeChanges.size()) {
                // The changed blocks get their original content back, newest change first.
                for (std::list<CubeChange>::reverse_iterator change = zone->m_cubeChanges.rbegin();
                     change != zone->m_cubeChanges.rend(); ++change) {
                    zone->setBlock(change->position[0], change->position[1], change->position[2],
                                   change->originalBlock);
                }
                std::cout << "undo " << zone->m_cubeChanges.size() << " changes" << std::endl;
                zone->m_cubeChanges.clear();
                zone->m_chunk.needsRemesh = true;
                zone->m_chunk.remeshTime = timeGetTime();
            }
        }
    }
}

// Zones more than twice the radius away count as at the capped distance (4 r^2).
bool World::removeFarZone(int radius)
{
    EnterCriticalSection(&m_zoneMeshCriticalSection);
    int zoneY = floorDivide(m_localCreature->m_entityData.position[1].toBlock(), BlocksPerZone);
    int zoneX = floorDivide(m_localCreature->m_entityData.position[0].toBlock(), BlocksPerZone);
    LeaveCriticalSection(&m_zoneMeshCriticalSection);

    // The loaded zone farthest from the player's, beyond the radius.
    std::unordered_map<unsigned long long, Zone *>::iterator farthest = m_zones.end();
    double farthestDistance = radius * radius;
    EnterCriticalSection(&m_zonesLock);
    for (std::unordered_map<unsigned long long, Zone *>::iterator entry = m_zones.begin(); entry != m_zones.end();
         ++entry) {
        Zone *zone = entry->second;
        if (zone) {
            int dx = zoneX - zone->m_position[0];
            int dy = zoneY - zone->m_position[1];
            double distance = (double)dx * (double)dx + (double)dy * (double)dy;
            if (abs(dx) > 2 * radius || abs(dy) > 2 * radius)
                distance = radius * radius * 4;
            if (distance > farthestDistance) {
                farthestDistance = distance;
                farthest = entry;
            }
        }
    }
    Zone *zone = nullptr;
    if (farthest != m_zones.end()) {
        zone = farthest->second;
        EnterCriticalSection(&m_zoneMeshCriticalSection);
        if (zone)
            storeZone(zone);
        m_zones.erase(farthest);
        LeaveCriticalSection(&m_zoneMeshCriticalSection);
    }
    LeaveCriticalSection(&m_zonesLock);
    if (zone) {
        delete zone;
        return true;
    }
    return false;
}

void World::onNewDay()
{
    m_state.m_deadCreatures.clear();
    m_state.m_zoneStates.clear();
    if (m_localCreature) {
        // The day's keys (category 22) leave the inventory.
        for (std::vector<ItemStack> &tab : m_localCreature->m_inventory.m_inventoryTabs) {
            for (ItemStack &stack : tab) {
                if (stack.item.category == ItemCategoryQuest) {
                    stack.item = Item();
                    stack.quantity = 0;
                }
            }
        }
        m_localCreature->m_playerData->collectedPlacedItems.clear();
        m_localCreature->m_playerData->finishedMissions.clear();
        m_localCreature->m_playerData->purchasesByShop.clear();
    }
}

bool World::requireZones(const std::set<std::array<int, 2>> &zones)
{
    std::list<std::unordered_map<unsigned long long, Zone *>::iterator> outside;
    EnterCriticalSection(&m_zonesLock);
    for (std::unordered_map<unsigned long long, Zone *>::iterator entry = m_zones.begin(); entry != m_zones.end();
         ++entry) {
        // Reads the zone's position without testing the pointer, which is null for a zone that is being generated.
        std::array<int, 2> position = {entry->second->m_position[0], entry->second->m_position[1]};
        if (zones.find(position) == zones.end())
            outside.push_back(entry);
    }
    LeaveCriticalSection(&m_zonesLock);
    for (std::unordered_map<unsigned long long, Zone *>::iterator &entry : outside) {
        Zone *zone = entry->second;
        EnterCriticalSection(&m_zonesLock);
        EnterCriticalSection(&m_zoneMeshCriticalSection);
        m_zones.erase(entry);
        storeZone(zone);
        LeaveCriticalSection(&m_zoneMeshCriticalSection);
        LeaveCriticalSection(&m_zonesLock);
        delete zone;
    }
    return true;
}

void World::retainLocalCreature()
{
    std::list<std::list<Creature *>::iterator> others;
    for (std::list<Creature *>::iterator creature = m_creatures.begin(); creature != m_creatures.end(); ++creature) {
        if (*creature != m_localCreature)
            others.push_back(creature);
    }
    for (std::list<Creature *>::iterator creature : others) {
        delete *creature;
        m_creatures.erase(creature);
    }
    m_creaturesById.clear();
    for (Creature *creature : m_creatures)
        m_creaturesById[creature->m_id] = creature;
    m_obstacles.clear();
}

// A start site: a block 8 and 40 blocks into the cell, on dry land (no river, lava or poison channel above 0.4), in a
// plain biome and on level ground (height steps under 4 over 8 blocks).
plasma::Vector<2, int> World::randomStartCell()
{
    // From a random point 100,000 to 200,000 zones out, eastwards zone by zone until a site satisfies every condition
    // below.
    double angle = rand() * TwoPi / (double)RAND_MAX;
    int distance = rand() % 100000 + 100000;
    int zoneX = (int)(cos(angle) * distance);
    int zoneY = (int)(sin(angle) * distance);
    for (;;) {
        plasma::Vector<2, int> cell = siteInZone(++zoneX, zoneY);
        if (zoneBuildingType(cell[0], cell[1]) != BuildingShrineOfLife)
            continue;
        plasma::Vector<2, int> position = BlocksPerCell * cell;
        int x = position[0] + 8;
        int y = position[1] + 40;

        if (height(x, y, 0) < 0.0f)
            continue;
        plasma::Vector<2, int> region = findRegion(x, y);
        int biome = landBiome(region[0], region[1]);
        if (biome != BiomeHills && biome != BiomeMountains && biome != BiomePlains)
            continue;
        BiomeSample sample;
        biomeSample(x, y, sample);
        AreaInfo info;
        fillAreaInfo(x, y, info);
        if (info.type != PlaceVillage)
            continue;
        // In a village, on plain land: no lava, undead land, mountains, desert, valley, strata or peak, and nearly
        // level ground.
        float lava = sample.lavaWeight(PlaceVillage, info.influence);
        if (lava > 0.0f)
            continue;
        float undead = sample.undead(PlaceVillage, info.influence);
        if (undead > 0.0f)
            continue;
        float mountains = sample.mountains();
        if (mountains > 0.0f)
            continue;
        if (sample.desertness() > 0.0f)
            continue;
        float wetness = sample.wetness();
        float road = streetAmount(x, y, 5, info.influence);
        if (valleyWater(x, y, road, lava, undead, wetness, mountains, &info) > 0.4f)
            continue;
        if (lavaChannels(x, y, road, lava, mountains, &info) > 0.4f)
            continue;
        if (poisonChannels(x, y, road, undead, mountains, &info) > 0.4f)
            continue;
        if (fabsf(height(x - 8, y, 0) - height(x + 8, y, 0)) > 4.0f)
            continue;
        if (fabsf(height(x, y - 8, 0) - height(x, y + 8, 0)) > 4.0f)
            continue;
        // Not the place of the region's first mission.
        int placeX = floorDivide(x, BlocksPerPlace);
        int placeY = floorDivide(y, BlocksPerPlace);
        if (info.large) {
            placeX = floorDivide(placeX, 2) * 2;
            placeY = floorDivide(placeY, 2) * 2;
        }
        Missions missions(this, region);
        plasma::Vector<2, int> village = missions.m_village;
        if (village == plasma::Vector<2, int>(placeX, placeY))
            continue;
        return cell;
    }
}

// Keep equipped pets and their owners in sync, spawning missing pets on the host.
void World::updateSwath(int firstX, int firstY, int lastX, int lastY)
{
    if (firstX < 0)
        firstX = 0;
    if (firstY < 0)
        firstY = 0;
    if (lastX < 0)
        lastX = 0;
    if (lastY < 0)
        lastY = 0;
    firstX = floorDivide(firstX, BlocksPerZone);
    lastX = floorDivide(lastX, BlocksPerZone);
    firstY = floorDivide(firstY, BlocksPerZone);
    lastY = floorDivide(lastY, BlocksPerZone);
    for (int x = firstX; x <= lastX; ++x) {
        for (int y = firstY; y <= lastY; ++y) {
            Zone *zone = this->zone(x, y);
            if (!zone)
                continue;
            for (int i = (int)zone->m_plants.size() - 1; i >= 0; --i) {
                const Zone::Plant &plant = zone->m_plants[i];
                if (plant.flags & PlantFlagNeedsSupport) {
                    plasma::Vector<3, Fixed> support(plant.position[0], plant.position[1],
                                                     plant.position[2] - Fixed(1));
                    Block block = getBlock(support[0].toBlock(), support[1].toBlock(), support[2].toBlock());
                    if (block.type == BlockAir)
                        zone->m_plants.erase(zone->m_plants.begin() + i);
                }
            }
        }
    }
}

void World::updateTamed()
{
    if (!m_worldName.empty()) {
        for (Creature *owner : m_creatures) {
            if (owner->m_entityData.hostilityType == HostilityPlayer &&
                owner->m_entityData.equipment.pet.category == ItemCategoryPet) {
                if (!owner->m_petId || !creatureByID(owner->m_petId)) {
                    owner->m_petId = 0;
                    for (Creature *candidate : m_creatures) {
                        if (candidate->m_entityData.ownerId == owner->m_id) {
                            owner->m_petId = candidate->m_id;
                            break;
                        }
                    }
                }
                if (m_isClient)
                    continue;
                bool changed = false;
                if (owner->m_petId) {
                    Creature *pet = creatureByID(owner->m_petId);
                    if (pet) {
                        if (pet->m_entityData.hp <= 0.0f)
                            owner->m_hitCounts.clear();
                        if (owner->m_lastPetSyncValue != owner->m_entityData.petRefreshValue ||
                            !(owner->m_entityData.equipment.pet == owner->m_lastPetItem)) {
                            changed = true;
                            if (owner->m_entityData.equipment.pet.id != owner->m_lastPetItem.id)
                                pet->reset();
                            pet->m_entityData.race = owner->m_entityData.equipment.pet.id;
                            if (owner->m_entityData.equipment.pet.spiritCount < 16) {
                                for (int i = 0; i < owner->m_entityData.equipment.pet.spiritCount; ++i)
                                    pet->m_entityData.name[i] = owner->m_entityData.equipment.pet.spirits[i].material;
                                pet->m_entityData.name[owner->m_entityData.equipment.pet.spiritCount] = 0;
                            }
                            std::cout << "changepet" << std::endl;
                            pet->m_entityData.ownerId = owner->m_id;
                            if (owner->m_entityData.equipment.pet.id != owner->m_lastPetItem.id) {
                                pet->m_entityData.appearance = Creature::EntityData::Appearance();
                                initCompanion(owner->m_id, pet);
                            }
                        }
                        if (owner->findBuff(BuffSpiritworld) && !pet->findBuff(BuffSpiritworld)) {
                            Creature::Buff buff = *owner->findBuff(BuffSpiritworld);
                            applyBuff(pet, buff, true);
                        }
                        float oldMaxHP = pet->maxHP();
                        pet->m_entityData.level = owner->m_entityData.level;
                        pet->m_entityData.hp = (pet->maxHP() / oldMaxHP) * pet->m_entityData.hp;
                    }
                }
                owner->m_lastPetSyncValue = owner->m_entityData.petRefreshValue;
                owner->m_lastPetItem = owner->m_entityData.equipment.pet;
                if (changed || owner->m_petId)
                    continue;
                unsigned char ability = owner->m_entityData.currentAbility;
                if (ability == AbilityHangGlide || ability == AbilitySailBoat || ability == AbilityFlightToPoint ||
                    ability == AbilityEagleFlight || ability == AbilityMenuView)
                    continue;
                long long id = LocalCreatureIdStart;
                for (Creature *creature : m_creatures)
                    if (creature->m_id < id) {
                        id = creature->m_id;
                    }
                --id;
                owner->m_petId = id;
                Creature *pet = new Creature(id);
                addCreature(pet);
                pet->reset();
                pet->m_entityData.ownerId = owner->m_id;
                pet->m_entityData.hostilityType =
                    owner->m_entityData.hostilityType == HostilityPlayer ? HostilityPet : HostilityEnemy;
                pet->m_copiedToPet = owner->m_copiedToPet;
                pet->m_entityData.race = owner->m_entityData.equipment.pet.id;
                pet->m_entityData.position = owner->m_entityData.position;
                pet->m_entityData.hp = pet->maxHP();
                pet->m_entityData.factionKind = owner->m_entityData.factionKind;
                if (owner->m_entityData.equipment.pet.spiritCount < 16) {
                    for (int i = 0; i < owner->m_entityData.equipment.pet.spiritCount; ++i)
                        pet->m_entityData.name[i] = owner->m_entityData.equipment.pet.spirits[i].material;
                    pet->m_entityData.name[owner->m_entityData.equipment.pet.spiritCount] = 0;
                }
                if (owner->m_entityData.appearance.appearanceFlags & AppearanceFlagTinted)
                    pet->m_entityData.appearance.appearanceFlags |= AppearanceFlagTinted;
                initCompanion(owner->m_id, pet);
                std::cout << "spawned pet " << owner->m_entityData.name << std::endl;
                if (owner->m_entityData.hostilityType == HostilityEnemy) {
                    if (owner->isMarked())
                        pet->m_entityData.appearance.appearanceFlags |= AppearanceFlagMarked;
                    else
                        pet->m_entityData.appearance.appearanceFlags &= ~AppearanceFlagMarked;
                }
            } else {
                Creature *pet = creatureByID(owner->m_petId);
                bool hasTamenessBuff = false;
                if (pet) {
                    for (const Creature::Buff &buff : pet->m_buffs) {
                        if (buff.type == BuffTameness) {
                            hasTamenessBuff = true;
                            break;
                        }
                    }
                }
                if (hasTamenessBuff)
                    continue;
                owner->m_petId = 0;
                if (pet)
                    pet->m_entityData.hp = 0.0f;
            }
        }
    }
    if (m_localCreature && m_localCreature->m_entityData.equipment.pet.formulaCategory < ItemCategoryConsumable)
        m_localCreature->m_entityData.equipment.pet.formulaCategory = ItemCategoryConsumable;
    if (m_isClient)
        return;
    for (Creature *pet : m_creatures) {
        if (!pet->m_entityData.ownerId || (pet->m_entityData.appearance.appearanceFlags & AppearanceFlagSummoned))
            continue;
        Creature *owner = creatureByID(pet->m_entityData.ownerId);
        if (!owner || owner->m_entityData.hostilityType != HostilityPlayer)
            continue;
        pet->m_entityData.region = owner->m_entityData.region;
        if (owner->m_entityData.hostilityType == HostilityPlayer &&
            (owner->m_petId != pet->m_id || (owner->isGliding() && owner->m_glideTime > PetGlideDismissTime) ||
             owner->m_entityData.currentAbility == AbilitySailBoat)) {
            pet->reset();
            pet->m_entityData.hp = 0.0f;
            pet->m_entityData.hostilityType = HostilityFriendlyNpc;
            pet->m_entityData.timeSinceAbility = DismissedPetAbilityTime;
            pet->m_entityData.ownerId = 0;
            continue;
        }
        float oldMaxHP = pet->maxHP();
        if (owner->m_entityData.equipment.weaponLeft.category) {
            pet->m_entityData.equipment.weaponLeft.rarity = owner->m_entityData.equipment.weaponLeft.rarity;
            pet->m_entityData.equipment.weaponLeft.region = owner->m_entityData.equipment.weaponLeft.region;
        } else {
            pet->m_entityData.equipment.weaponLeft.rarity = ItemRarityWhite;
        }
        if (owner->m_entityData.equipment.weaponRight.category) {
            pet->m_entityData.equipment.weaponRight.rarity = owner->m_entityData.equipment.weaponRight.rarity;
            pet->m_entityData.equipment.weaponRight.region = owner->m_entityData.equipment.weaponRight.region;
        } else {
            pet->m_entityData.equipment.weaponRight.rarity = ItemRarityWhite;
        }
        if (owner->m_entityData.equipment.chest.category)
            pet->m_entityData.equipment.chest.rarity = owner->m_entityData.equipment.chest.rarity;
        else
            pet->m_entityData.equipment.chest.rarity = ItemRarityWhite;
        pet->m_entityData.equipment.chest.region = owner->m_entityData.equipment.chest.region;
        if (owner->m_entityData.equipment.weaponLeft.isRenowned() &&
            owner->m_entityData.equipment.weaponLeft.getRarity(owner->m_entityData.region) ==
                owner->m_entityData.equipment.weaponLeft.rarity)
            pet->m_entityData.equipment.weaponLeft.region = pet->m_entityData.region;
        if (owner->m_entityData.equipment.weaponRight.isRenowned() &&
            owner->m_entityData.equipment.weaponRight.getRarity(owner->m_entityData.region) ==
                owner->m_entityData.equipment.weaponRight.rarity)
            pet->m_entityData.equipment.weaponRight.region = pet->m_entityData.region;
        if (owner->m_entityData.equipment.chest.isRenowned() &&
            owner->m_entityData.equipment.chest.getRarity(owner->m_entityData.region) ==
                owner->m_entityData.equipment.chest.rarity)
            pet->m_entityData.equipment.chest.region = pet->m_entityData.region;
        float newMaxHP = pet->maxHP();
        if (oldMaxHP > 0.0f)
            pet->m_entityData.hp = (newMaxHP / oldMaxHP) * pet->m_entityData.hp;
    }
}

// Advance the clock, accelerating it while a living player rests (ability 88). The local resting player also recovers
// five percent of max HP/sec.
void World::updateTime(float milliseconds)
{
    bool resting = false;
    for (Creature *creature : m_creatures) {
        if (creature->m_entityData.hostilityType == HostilityPlayer &&
            creature->m_entityData.currentAbility == AbilitySleep && creature->m_entityData.hp > 0.0f) {
            if (creature == m_localCreature) {
                m_localCreature->m_entityData.hp +=
                    m_localCreature->maxHP() * (milliseconds * 0.001f * RestRegenerationRate);
                if (m_localCreature->m_entityData.hp > m_localCreature->maxHP())
                    m_localCreature->m_entityData.hp = m_localCreature->maxHP();
            }
            resting = true;
        }
    }
    m_clock += milliseconds;
    if (resting)
        m_state.m_time += milliseconds * RestingTimeScale;
    else
        m_state.m_time += milliseconds * TimeScale;
    if (m_worldName.empty())
        m_state.m_time = PreviewHour * MillisecondsPerHour;
    while (m_state.m_time > MillisecondsPerDay) {
        ++m_state.m_day;
        m_state.m_time -= MillisecondsPerDay;
        if (!m_isClient) {
            rewindDay();
            onNewDay();
            m_serverUpdates.m_worldEventList.push_back(WorldEventNewDay);
        }
    }
}

void World::updateZones()
{
    // The zones whose items or static entities changed: each is flagged and its list queued for the players.
    for (Zone *const &zone : m_zonesWithChangedItems) {
        zone->m_itemsChanged = true;
        m_serverUpdates.m_itemListPacketList.push_back(ServerUpdates::ItemListPacket(*zone));
    }
    for (Zone *const &zone : m_zonesWithChangedObjects) {
        zone->m_staticEntitiesChanged = true;
        m_serverUpdates.m_staticEntityPacketList.push_back(ServerUpdates::Packet6(*zone));
    }
    for (Zone *zone : m_zonesWithChangedObjects)
        zone->updateOccupancy();
    m_zonesWithChangedItems.clear();
    m_zonesWithChangedObjects.clear();
}

void World::removeZones()
{
    EnterCriticalSection(&m_zonesLock);
    std::unordered_map<unsigned long long, Zone *> zones = m_zones;
    m_zones.clear();
    LeaveCriticalSection(&m_zonesLock);
    for (std::pair<const unsigned long long, Zone *> &entry : zones) {
        if (entry.second) {
            EnterCriticalSection(&m_zoneMeshCriticalSection);
            storeZone(entry.second);
            LeaveCriticalSection(&m_zoneMeshCriticalSection);
            delete entry.second;
        }
    }
}

void World::spawnLocalCreature()
{
    // The lore the character has found, per realm, is rebuilt from the discovered places that are of a lore kind.
    m_localCreature->m_playerData->loreByRealm.clear();
    for (const std::array<int, 2> &place : m_localCreature->m_playerData->discoveredMarkers) {
        int type = zoneBuildingType(place[0], place[1]);
        if (type == BuildingRockLorePavilion || type == BuildingLoreCrypt || type == BuildingInscribedMenhir ||
            type == BuildingSanctuary || type == BuildingRaisedSanctuary) {
            int y = floorDivide(place[1] * BlocksPerCell, BlocksPerPlace);
            int x = floorDivide(place[0] * BlocksPerCell, BlocksPerPlace);
            m_localCreature->m_playerData->loreByRealm[mortalRealm(x, y, isLargePlace(x, y))].insert(place);
        }
    }
}

// The ingredients scale with the rarity: 5, 10, 8, 4 and 3 per item kind for equipment, 5 for elixirs (a hand-authored
// recipe table).
bool World::groundedFormula(const Item &item, Formula &formula, bool unused)
{
    formula = Formula();
    if (item.category == ItemCategoryGlyph) {
        groundlessFormula(item.id, formula);
        return true;
    }
    formula.result = item;
    // Every ingredient is a material item (category 11) of some id.
    Ingredient ingredient;
    ingredient.item.category = ItemCategoryMaterial;
    switch (item.category) {
    case ItemCategoryWeapon:
        switch (item.id) {
        case WeaponSword:
        case WeaponAxe:
        case WeaponMace:
        case WeaponDagger:
        case WeaponShield:
            ingredient.quantity = 5 * (item.rarity + 1);
            ingredient.item.material = ItemMaterialIron;
            ingredient.item.id = MaterialItemCube;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationAnvil;
            break;
        case WeaponBow:
            ingredient.quantity = 10 * (item.rarity + 1);
            ingredient.item.id = MaterialItemCube;
            ingredient.item.material = ItemMaterialWood;
            formula.ingredients.push_back(ingredient);
            ingredient.quantity = item.rarity + 1;
            ingredient.item.material = ItemMaterialLinen;
            ingredient.item.id = MaterialItemYarn;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationWorkbench;
            break;
        case WeaponCrossbow:
            ingredient.item.id = MaterialItemCube;
            ingredient.item.material = ItemMaterialIron;
            ingredient.quantity = 2 * (item.rarity + 1);
            formula.ingredients.push_back(ingredient);
            ingredient.quantity = 8 * (item.rarity + 1);
            ingredient.item.id = MaterialItemCube;
            ingredient.item.material = ItemMaterialWood;
            formula.ingredients.push_back(ingredient);
            ingredient.quantity = item.rarity + 1;
            ingredient.item.material = ItemMaterialLinen;
            ingredient.item.id = MaterialItemYarn;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationWorkbench;
            break;
        case WeaponBoomerang:
        case WeaponStaff:
        case WeaponWand:
            ingredient.item.id = MaterialItemCube;
            ingredient.item.material = ItemMaterialWood;
            ingredient.quantity = 10 * (item.rarity + 1);
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationWorkbench;
            break;
        case WeaponBracelet:
            ingredient.item.id = MaterialItemCube;
            ingredient.quantity = 5 * (item.rarity + 1);
            ingredient.item.material = item.material;
            formula.workstationRequired = WorkstationAnvil;
            formula.ingredients.push_back(ingredient);
            break;
        case WeaponFist:
            ingredient.item.id = MaterialItemCube;
            ingredient.item.material = ItemMaterialIron;
            ingredient.quantity = 4 * (item.rarity + 1);
            formula.ingredients.push_back(ingredient);
            ingredient.item.id = MaterialItemYarn;
            ingredient.item.material = ItemMaterialCotton;
            ingredient.quantity = 2 * (item.rarity + 1);
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationAnvil;
            break;
        case WeaponLongsword:
        case WeaponGreatsword:
        case WeaponGreataxe:
        case WeaponGreatmace:
            ingredient.quantity = 10 * (item.rarity + 1);
            ingredient.item.material = ItemMaterialIron;
            ingredient.item.id = MaterialItemCube;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationAnvil;
            break;
        }
        break;
    case ItemCategoryChest:
        ingredient.quantity = 5 * (item.rarity + 1) * 2;
        if (item.material != ItemMaterialIron) {
            ingredient.item.material = item.material;
            ingredient.item.id = MaterialItemYarn;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationLoom;
        } else {
            ingredient.item.id = MaterialItemCube;
            ingredient.item.material = ItemMaterialIron;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationAnvil;
        }
        break;
    case ItemCategoryShoulder:
        ingredient.quantity = 3 * (item.rarity + 1) * 2;
        if (item.material != ItemMaterialIron) {
            ingredient.item.material = item.material;
            ingredient.item.id = MaterialItemYarn;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationLoom;
        } else {
            ingredient.item.id = MaterialItemCube;
            ingredient.item.material = ItemMaterialIron;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationAnvil;
        }
        break;
    case ItemCategoryHands:
    case ItemCategoryFeet:
        ingredient.quantity = 5 * (item.rarity + 1);
        if (item.material != ItemMaterialIron) {
            ingredient.item.material = item.material;
            ingredient.item.id = MaterialItemYarn;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationLoom;
        } else {
            ingredient.item.id = MaterialItemCube;
            ingredient.item.material = ItemMaterialIron;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationAnvil;
        }
        break;
    case ItemCategoryNeck:
    case ItemCategoryRing:
        ingredient.item.id = MaterialItemCube;
        ingredient.quantity = 5 * (item.rarity + 1);
        ingredient.item.material = item.material;
        formula.ingredients.push_back(ingredient);
        break;
    case ItemCategoryMaterial:
        switch (item.id) {
        case MaterialItemYarn:
            switch (item.material) {
            case ItemMaterialSilk:
                ingredient.item.id = MaterialItemCobweb;
                ingredient.item.material = ItemMaterialNone;
                ingredient.quantity = 1;
                formula.ingredients.push_back(ingredient);
                formula.workstationRequired = WorkstationSpinningWheel;
                break;
            case ItemMaterialLinen:
                ingredient.item.id = MaterialItemFiber;
                ingredient.item.material = ItemMaterialPlant;
                ingredient.quantity = 1;
                formula.ingredients.push_back(ingredient);
                formula.workstationRequired = WorkstationSpinningWheel;
                break;
            case ItemMaterialCotton:
                ingredient.item.id = MaterialItemCapsule;
                ingredient.item.material = ItemMaterialCotton;
                ingredient.quantity = 1;
                formula.ingredients.push_back(ingredient);
                formula.workstationRequired = WorkstationSpinningWheel;
                break;
            }
            break;
        case MaterialItemCube:
            ingredient.quantity = 1;
            if (item.material == ItemMaterialWood) {
                formula.workstationRequired = WorkstationSaw;
                ingredient.item.material = item.material;
                ingredient.item.id = MaterialItemLog;
            } else {
                formula.workstationRequired = WorkstationFurnace;
                ingredient.item.material = item.material;
                ingredient.item.id = MaterialItemNugget;
            }
            formula.ingredients.push_back(ingredient);
            break;
        case MaterialItemHeartflower:
            ingredient.item.id = MaterialItemFrozenHeartflower;
            ingredient.item.material = ItemMaterialNone;
            ingredient.quantity = 1;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationCampfire;
            break;
        case MaterialItemWaterFlask:
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemFlask;
            ingredient.item.material = ItemMaterialGlass;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationWater;
            break;
        }
        break;
    case ItemCategoryElixir:
        switch (item.id) {
        case ElixirElixirOfLife:
            ingredient.item.id = MaterialItemHeartflower;
            ingredient.quantity = 5 * (item.rarity + 1);
            formula.ingredients.push_back(ingredient);
            ingredient.item.id = MaterialItemGinsengRoot;
            ingredient.quantity = item.rarity + 1;
            formula.ingredients.push_back(ingredient);
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemWaterFlask;
            formula.ingredients.push_back(ingredient);
            break;
        case ElixirElixirOfPower:
            ingredient.item.id = MaterialItemMushroom;
            ingredient.quantity = 5 * (item.rarity + 1);
            formula.ingredients.push_back(ingredient);
            ingredient.item.id = MaterialItemDragonRoot;
            ingredient.quantity = 2 * (item.rarity + 1);
            formula.ingredients.push_back(ingredient);
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemWaterFlask;
            formula.ingredients.push_back(ingredient);
            break;
        case ElixirElixirOfToughness:
            ingredient.item.id = MaterialItemMushroom;
            ingredient.quantity = 5 * (item.rarity + 1);
            formula.ingredients.push_back(ingredient);
            ingredient.item.id = MaterialItemDewdrop;
            ingredient.quantity = 2 * (item.rarity + 1);
            formula.ingredients.push_back(ingredient);
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemWaterFlask;
            formula.ingredients.push_back(ingredient);
            break;
        case ElixirElixirOfSanity:
            ingredient.item.id = MaterialItemShimmerMushroom;
            ingredient.quantity = 5 * (item.rarity + 1);
            formula.ingredients.push_back(ingredient);
            ingredient.item.id = MaterialItemManaOrchid;
            ingredient.quantity = 2 * (item.rarity + 1);
            formula.ingredients.push_back(ingredient);
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemWaterFlask;
            formula.ingredients.push_back(ingredient);
            break;
        case ElixirLemonade:
            ingredient.quantity = 2;
            ingredient.item.id = MaterialItemSugarCube;
            formula.ingredients.push_back(ingredient);
            ingredient.quantity = 2;
            ingredient.item.id = MaterialItemIceCube;
            formula.ingredients.push_back(ingredient);
            ingredient.quantity = 3;
            ingredient.item.id = MaterialItemLemon;
            formula.ingredients.push_back(ingredient);
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemWaterFlask;
            formula.ingredients.push_back(ingredient);
            break;
        case ElixirHotChocolate:
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemSugarCube;
            formula.ingredients.push_back(ingredient);
            ingredient.quantity = 3;
            ingredient.item.id = MaterialItemCocoaBean;
            formula.ingredients.push_back(ingredient);
            ingredient.item.id = MaterialItemWaterFlask;
            ingredient.quantity = 1;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationCampfire;
            break;
        case ElixirGreenSmoothie:
            ingredient.quantity = 2;
            ingredient.item.id = MaterialItemBanana;
            formula.ingredients.push_back(ingredient);
            ingredient.quantity = 2;
            ingredient.item.id = MaterialItemKale;
            formula.ingredients.push_back(ingredient);
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemWaterFlask;
            formula.ingredients.push_back(ingredient);
            break;
        }
        break;
    case ItemCategoryConsumable:
        switch (item.id) {
        case ConsumableLifePotion:
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemHeartflower;
            formula.ingredients.push_back(ingredient);
            switch (item.rarity) {
            case ItemRarityGreen:
                ingredient.item.id = MaterialItemMindflower;
                ingredient.item.rarity = ItemRarityGreen;
                formula.ingredients.push_back(ingredient);
                break;
            case ItemRarityBlue:
                ingredient.item.id = MaterialItemManaOrchid;
                ingredient.item.rarity = ItemRarityBlue;
                formula.ingredients.push_back(ingredient);
                break;
            case ItemRarityPurple:
                ingredient.item.id = MaterialItemSpiritflower;
                ingredient.item.rarity = ItemRarityPurple;
                formula.ingredients.push_back(ingredient);
                break;
            case ItemRarityYellow:
                ingredient.item.id = MaterialItemSoulFlower;
                ingredient.item.rarity = ItemRarityYellow;
                formula.ingredients.push_back(ingredient);
                break;
            }
            ingredient.item.rarity = ItemRarityWhite;
            ingredient.item.id = MaterialItemWaterFlask;
            formula.ingredients.push_back(ingredient);
            break;
        case ConsumableEssenceOfWisdom:
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemManaOrchid;
            ingredient.item.rarity = ItemRarityGreen;
            formula.ingredients.push_back(ingredient);
            ingredient.item.rarity = ItemRarityWhite;
            ingredient.item.id = MaterialItemWaterFlask;
            formula.ingredients.push_back(ingredient);
            break;
        case ConsumableGreaterLifePotion:
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemHeartflower;
            formula.ingredients.push_back(ingredient);
            ingredient.item.id = MaterialItemSoulFlower;
            ingredient.item.rarity = ItemRarityGreen;
            formula.ingredients.push_back(ingredient);
            ingredient.item.rarity = ItemRarityWhite;
            ingredient.item.id = MaterialItemWaterFlask;
            formula.ingredients.push_back(ingredient);
            break;
        case ConsumableCactusPotion:
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemPricklyPear;
            formula.ingredients.push_back(ingredient);
            ingredient.item.id = MaterialItemFlask;
            ingredient.item.material = ItemMaterialGlass;
            formula.ingredients.push_back(ingredient);
            break;
        case ConsumableGinsengSoup:
            ingredient.quantity = 4;
            ingredient.item.id = MaterialItemGinsengRoot;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationCampfire;
            break;
        case ConsumableMushroomSpit:
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemMushroom;
            formula.ingredients.push_back(ingredient);
            ingredient.item.id = MaterialItemOnionSlice;
            ingredient.quantity = 1;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationCampfire;
            break;
        case ConsumableSnowBerryMash:
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemSnowBerry;
            formula.ingredients.push_back(ingredient);
            break;
        case ConsumablePineappleSlice:
            ingredient.quantity = 1;
            ingredient.item.id = MaterialItemPineapple;
            formula.ingredients.push_back(ingredient);
            break;
        case ConsumablePumpkinMuffin:
            ingredient.item.id = MaterialItemPumpkin;
            ingredient.quantity = 1;
            formula.ingredients.push_back(ingredient);
            formula.workstationRequired = WorkstationCampfire;
            break;
        }
        break;
    }
    if (item.category == ItemCategoryConsumable || item.category == ItemCategoryElixir)
        return true;
    // A rare result also takes spirit cubes of its rarity: four for a chest piece or a two-handed weapon, two
    // otherwise.
    switch (formula.result.rarity) {
    case ItemRarityGreen:
        if (item.category == ItemCategoryChest || item.isTwoHanded())
            ingredient.quantity = 4;
        else
            ingredient.quantity = 2;
        ingredient.item.id = MaterialItemEmerald;
        ingredient.item.rarity = ItemRarityGreen;
        break;
    case ItemRarityBlue:
        if (item.category == ItemCategoryChest || item.isTwoHanded())
            ingredient.quantity = 4;
        else
            ingredient.quantity = 2;
        ingredient.item.id = MaterialItemSapphire;
        ingredient.item.rarity = ItemRarityBlue;
        break;
    case ItemRarityPurple:
        if (item.category == ItemCategoryChest || item.isTwoHanded())
            ingredient.quantity = 4;
        else
            ingredient.quantity = 2;
        ingredient.item.id = MaterialItemRuby;
        ingredient.item.rarity = ItemRarityPurple;
        break;
    case ItemRarityYellow:
        if (item.category == ItemCategoryChest || item.isTwoHanded())
            ingredient.quantity = 4;
        else
            ingredient.quantity = 2;
        ingredient.item.id = MaterialItemDiamond;
        ingredient.item.rarity = ItemRarityYellow;
        break;
    default:
        return true;
    }
    ingredient.item.material = ItemMaterialNone;
    formula.ingredients.push_back(ingredient);
    return true;
}

void World::init(int seed, const std::string &name)
{
    m_database.close();
    m_seed = seed;
    m_worldName = name;
    srand(seed);
    std::cout << "SRAND initialize" << std::endl;
    // 64 hamlet layouts: the plot types 2..11 and six empty plots in a random order (hamletPlot picks one per place).
    for (int i = 0; i < 64; i++) {
        std::vector<int> plots(16, 0);
        plots[0] = PlotInn;
        plots[1] = PlotItemShop;
        plots[2] = PlotArmorShop;
        plots[3] = PlotWeaponShop;
        plots[4] = PlotIdentifiersShop;
        plots[5] = PlotSmithy;
        plots[6] = PlotCarpentersShop;
        plots[7] = PlotTailorsShop;
        plots[8] = PlotFlightMaster;
        plots[9] = PlotGuildHall;
        shuffleWithRand(plots.begin(), plots.end());
        m_hamletPlotLayouts.push_back(plots);
    }
    m_generationSeed = rand() % SeedRange;
    for (int i = 0; i < 4; i++)
        m_seededRandBuf0[i] = rand() % SeedRange;
    for (int i = 0; i < 20; i++)
        m_seededRandBuf3[i] = rand() % SeedRange;
    for (int i = 0; i < 2; i++)
        m_seededRandBuf4[i] = rand() % SeedRange;
    for (int i = 0; i < 2; i++)
        m_seededRandBuf5[i] = rand() % SeedRange;
    for (int i = 0; i < 2; i++)
        m_seededRandBuf6[i] = rand() % SeedRange;
    for (int i = 0; i < 2; i++)
        m_seededRandBuf7[i] = rand() % SeedRange;
    for (int i = 0; i < 8; i++)
        m_seededRandBuf8[i] = rand() % SeedRange;
    for (int i = 0; i < 6; i++)
        m_seededRandBuf9[i] = rand() % SeedRange;
    for (int i = 0; i < 6; i++)
        m_seededRandBuf10[i] = rand() % SeedRange;
    for (int i = 0; i < 10; i++)
        m_seededRandBuf11[i] = rand() % SeedRange;
    for (int i = 0; i < 4; i++)
        m_sandShadeOffset[i] = rand() % SeedRange;
    for (int i = 0; i < 2; i++)
        m_seededRandBuf13[i] = rand() % SeedRange;
    for (int i = 0; i < 4; i++)
        m_seededRandBuf1[i] = rand() % SeedRange;
    for (int i = 0; i < 2; i++)
        m_seededRandBuf14[i] = rand() % SeedRange;
    for (int i = 0; i < 2; i++)
        m_regionNoiseOffset[i] = rand() % SeedRange;
    m_unusedVector388.clear();
    m_unusedVector3a0.clear();
    m_state = WorldState();
    m_state.m_day = 1;
    m_state.m_time = StartHour * MillisecondsPerHour;
    if (!m_isClient && name.size()) {
        std::stringstream path;
        path << "Save/world.db";
        m_database.open(path.str());
        loadState();
    }
}

// The wind (a hand-tuned profile): noises at 0.02 sampled at raw fixed-point phases, of amplitude 8 in x and y and
// half in z.
plasma::Vector<3, float> World::wind(const plasma::Vector<3, Fixed> &position, const plasma::Vector<3, float> &velocity)
{
    double positionX = (double)position[0].m_raw;
    double positionY = (double)position[1].m_raw;
    const long long reverseY = (long long)(positionY * -0.02);
    const long long reverseX = (long long)(positionX * -0.02);
    float up = noise((0x108a0000LL - reverseX) * (1.0 / 65536.0), (0x14e10000LL - reverseY) * (1.0 / 65536.0)) + 0.5f;
    float y = noise((0x0d7f0000LL - reverseX) * (1.0 / 65536.0), (0x20f60000LL - reverseY) * (1.0 / 65536.0)) * 8.0f;
    float x =
        noise((long long)(positionX * 0.02) * (1.0 / 65536.0), (long long)(positionY * 0.02) * (1.0 / 65536.0)) * 8.0f;
    return plasma::Vector<3, float>(x, y, up * 2.0f);
}

// The second pass of a world tick over all creatures. First the drawn position and rotation (m_drawnPosition,
// m_drawnRotation) follow the simulated ones while a remote update is being smoothed (m_smoothingTime > 0) and equal
// them otherwise, and m_drawnAttackRotation follows the attack rotation. Then the creatures that are attached to
// another one are put in place: ability 145 keeps its offset to the target, a rider (ability 140) sits on its mount,
// and a creature of hostility type 6 whose owner uses ability 118 is held above the owner.
void World::inspectCreatures(float milliseconds)
{
    for (Creature *&creature : m_creatures) {
        if (creature->m_smoothingTime > 0.0f) {
            float factor = 0.0f;
            blend(factor, 1.0f, milliseconds, DrawnSmoothingRate);
            // A 16.16 fixed-point product per axis.
            Fixed scale(factor);
            plasma::Vector<3, Fixed> delta = creature->m_entityData.position - creature->m_drawnPosition;
            creature->m_drawnPosition += delta * scale;
            creature->m_drawnPosition[2] +=
                (creature->m_entityData.position[2] - creature->m_drawnPosition[2]) * factor;
            creature->m_drawnRotation[0] =
                interpolateAngle(creature->m_drawnRotation[0], creature->m_entityData.rotation[0], factor);
            creature->m_drawnRotation[1] =
                interpolateAngle(creature->m_drawnRotation[1], creature->m_entityData.rotation[1], factor);
            creature->m_drawnRotation[2] =
                interpolateAngle(creature->m_drawnRotation[2], creature->m_entityData.rotation[2], factor);
            creature->m_entityData.timeSinceAbility += factor * SmoothingAnimationCatchUp;
            if (creature->m_stepOffset < 0.0f)
                creature->m_stepOffset = 0.0f;
        } else {
            creature->m_drawnPosition = creature->m_entityData.position;
            creature->m_drawnRotation = creature->m_entityData.rotation;
        }
        blend(creature->m_drawnAttackRotation, creature->m_entityData.attackRotation, milliseconds,
              creature->m_entityData.hostilityType == HostilityPlayer ? PlayerAimBlendRate : AimBlendRate);
    }
    for (Creature *creature : m_creatures) {
        carryCreature(creature, milliseconds);
        if (creature->m_entityData.currentAbility == AbilityAttachToTarget && creature->m_entityData.focusId != 0) {
            if (creatureByID(creature->m_entityData.focusId) &&
                creatureByID(creature->m_entityData.focusId)->m_entityData.hp > 0.0f) {
                Creature *target = creatureByID(creature->m_entityData.focusId);
                creature->m_entityData.position =
                    target->m_entityData.position + plasma::Vector<3, Fixed>(creature->m_entityData.attackRotation);
                creature->m_drawnPosition =
                    target->m_drawnPosition + plasma::Vector<3, Fixed>(creature->m_entityData.attackRotation);
            }
        }
        if (creature->m_entityData.currentAbility == AbilityAssassinate) {
            if (creature->m_entityData.timeSinceAbility < (float)creature->attackDuration() &&
                creature->m_entityData.focusId != 0 && creatureByID(creature->m_entityData.focusId) &&
                creatureByID(creature->m_entityData.focusId)->m_entityData.hp > 0.0f) {
                Creature *mount = creatureByID(creature->m_entityData.focusId);
                plasma::Vector<3, float> seat = riderOffset(mount->m_entityData.race);
                // The rider's seat three quarters up the mount, a hundredth of a block above its feet.
                seat[2] += mount->m_entityData.appearance.size[2] * 0.75f;
                float angle = (float)(mount->m_entityData.rotation[2] * Pi / 180.0 + HalfPi);
                plasma::Vector<3, float> forward;
                forward[0] = cosf(angle);
                forward[1] = sinf(angle);
                forward[2] = 0.0f;
                float back =
                    (std::min)(mount->m_entityData.appearance.size[0], mount->m_entityData.appearance.size[1]) * -0.5f;
                creature->m_entityData.position =
                    mount->m_entityData.position -
                    plasma::Vector<3, Fixed>(
                        plasma::Vector<3, float>(0.0f, 0.0f,
                                                 mount->m_entityData.appearance.size[2] * 0.5f -
                                                     creature->m_entityData.appearance.size[2] * 0.5f + 0.01f)) +
                    plasma::Vector<3, Fixed>(seat) + plasma::Vector<3, Fixed>(forward * back);
                creature->m_drawnPosition = mount->m_drawnPosition -
                                            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                0.0f, 0.0f,
                                                mount->m_entityData.appearance.size[2] * 0.5f -
                                                    creature->m_entityData.appearance.size[2] * 0.5f + 0.01f)) +
                                            plasma::Vector<3, Fixed>(seat) + plasma::Vector<3, Fixed>(forward * back);
                creature->m_entityData.rotation = mount->m_entityData.rotation;
                creature->m_stepOffset = mount->m_stepOffset;
                creature->m_walkAmplitude = 0.0f;
                creature->m_walkPhase = 0.0f;
                creature->m_entityData.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                creature->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                creature->m_entityData.retreat = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                creature->m_entityData.rollTime = 0.0f;
            } else {
                std::cout << "FOCUS " << creature->m_entityData.focusId << std::endl;
                creature->m_entityData.currentAbility = AbilityNone;
                if (creature->m_entityData.focusId != 0 && creatureByID(creature->m_entityData.focusId) &&
                    creatureByID(creature->m_entityData.focusId)->m_entityData.hp > 0.0f) {
                    creature->m_entityData.position =
                        creatureByID(creature->m_entityData.focusId)->m_entityData.position;
                }
            }
        }
        if (creature->m_entityData.hostilityType == HostilityPet) {
            if (creatureByID(creature->m_entityData.ownerId) &&
                creatureByID(creature->m_entityData.ownerId)->m_entityData.currentAbility == AbilityRidePet) {
                Creature *owner = creatureByID(creature->m_entityData.ownerId);
                creature->m_drawnPosition =
                    owner->m_drawnPosition + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                 0.0f, 0.0f,
                                                 creature->m_entityData.appearance.size[2] * 0.5f -
                                                     owner->m_entityData.appearance.size[2] * 0.5f + 0.01f));
                creature->m_drawnRotation = owner->m_drawnRotation;
                creature->m_entityData.position =
                    owner->m_entityData.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                       0.0f, 0.0f,
                                                       creature->m_entityData.appearance.size[2] * 0.5f -
                                                           owner->m_entityData.appearance.size[2] * 0.5f + 0.01f));
                creature->m_entityData.rotation = owner->m_entityData.rotation;
                creature->m_smoothingTime = 0.0f;
                creature->m_stepOffset = owner->m_stepOffset;
                creature->m_walkAmplitude = owner->m_walkAmplitude;
                creature->m_walkPhase = owner->m_walkPhase * owner->m_entityData.appearance.size[0] /
                                        creature->m_entityData.appearance.size[0];
                creature->m_entityData.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                creature->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            }
        }
    }
}

// A grave tells who rests there. Any other lore object tells, for half of the zones, of the artifact hidden in the
// realm in one of the nine regions around (if one of them has such a place), otherwise the legend of the realm's
// ruler. A lore object tells of an artifact for one in two (salt 635012) in a neighbouring region (salts 369272,
// 512436, -1..1 each way; the artifact place by salt 585627), else its ruler's legend.
std::vector<std::string> World::storyScript(int x, int y)
{
    int cellY = floorDivide(y * BlocksPerCell, BlocksPerPlace);
    int cellX = floorDivide(x * BlocksPerCell, BlocksPerPlace);
    Realm realm(mortalRealm(cellX, cellY, isLargePlace(cellX, cellY)));
    if (zoneBuildingType(x, y) != BuildingLoreCrypt) {
        if (random(x + 635012, y) % 2 == 1) {
            Missions missions(this,
                              findRegion(x * BlocksPerCell + BlocksPerCell / 2, y * BlocksPerCell + BlocksPerCell / 2) +
                                  plasma::Vector<2, int>(random(x + 369272, y) % 3 - 1, random(x + 512436, y) % 3 - 1));
            plasma::Vector<2, int> place = missions.artifactPlace(random(x + 585627, y), realm.id);
            if (place[0] != NoCoordinate) {
                Item artifact = missions.artifactAt(place[0], place[1]);
                int type = locationType(place[0], place[1], isLargePlace(place[0], place[1]));
                int dungeon = dungeonType(place[0], place[1], type, isLargePlace(place[0], place[1]));
                if (dungeon != 0)
                    type = dungeon;
                return realm.artifactTale(artifact, place[0], place[1], type, random(x, y));
            }
        }
        plasma::Vector<2, int> region =
            findRegion(x * BlocksPerCell + BlocksPerCell / 2, y * BlocksPerCell + BlocksPerCell / 2);
        return realm.rulerLegend(random(x, y), landBiome(region[0], region[1]), random(region[0], region[1]));
    }
    return realm.tombstone(random(x, y));
}

// The 16-block cell's dungeon room or connecting passage. Room margins and door offsets agree with the neighbouring
// 128-block rooms; the last byte carries the corridor's length, rather than its directional side.
StructureCell World::structureCell(int x, int y, bool flag)
{
    int building = zoneBuildingType(x, y);
    int typeAtCell = building;
    if (typeAtCell == 0)
        typeAtCell = zoneBuildingType(x, y);
    if (typeAtCell == BuildingCatacombsEntrance || typeAtCell == BuildingSewerOrGrottoEntrance ||
        typeAtCell == BuildingPyramidEntrance || typeAtCell == BuildingTempleEntrance)
        return StructureCell(StructureCell_Room, RoomPart_None, 0, 0);
    int regionX = floorDivide(x * BlocksPerCell, BlocksPerPlace);
    int regionY = floorDivide(y * BlocksPerCell, BlocksPerPlace);
    bool large = isLargePlace(regionX, regionY);
    if (large) {
        regionX = floorDivide(regionX, 2) * 2;
        regionY = floorDivide(regionY, 2) * 2;
    }
    int placeType = locationType(regionX, regionY, large);
    int type = dungeonType(regionX, regionY, placeType, large);
    if (!type)
        return StructureCell(StructureCell_None, 0, 0, 0);
    plasma::Vector<2, int> center = placeCenter(regionX, regionY, large);
    int direction = NeighbourSide_South;
    int longest = (std::max)(0, center[0] - (regionX << 11));
    if (center[1] - (regionY << 11) > longest)
        longest = center[1] - (regionY << 11);
    else
        direction = NeighbourSide_West;
    if (((regionX + 2) << 10) - center[0] > longest) {
        direction = NeighbourSide_East;
        longest = ((regionX + 2) << 10) - center[0];
    }
    if (((regionY + 2) << 10) - center[1] > longest)
        direction = NeighbourSide_North;
    int roomX = floorDivide(x, 8);
    int roomY = floorDivide(y, 8);
    plasma::Vector<2, int> start = dungeonPosition(regionX, regionY, placeType, large);
    int startX = floorDivide(start[0], 8);
    int startY = floorDivide(start[1], 8);
    Corridor way = dungeonWay(roomX, roomY, direction, startX, startY, type, regionX, regionY);
    if (!way.kind)
        return StructureCell(StructureCell_None, 0, 0, 0);
    int left = random(roomX + 5137033, roomY) % 3;
    int right = random(roomX + 138570, roomY) % 3;
    int bottom = random(roomX + 1156159, roomY) % 3;
    int top = random(roomX + 1374023, roomY) % 3;
    int localX = (x % 8 + 8) % 8;
    int localY = (y % 8 + 8) % 8;
    int leftDoor = (std::max)(bottom, (int)(random(roomX + 1156158, roomY) % 3)) + 1;
    int leftTop = (std::max)(top, (int)(random(roomX + 1374022, roomY) % 3));
    leftDoor += random(roomX + 507672, roomY) % (7 - leftTop - leftDoor);
    int bottomDoor = (std::max)(left, (int)(random(roomX + 5137033, roomY - 1) % 3)) + 1;
    int bottomRight = (std::max)(right, (int)(random(roomX + 138570, roomY - 1) % 3));
    bottomDoor += random(roomX + 81944, roomY) % (7 - bottomRight - bottomDoor);
    int rightDoor = (std::max)(bottom, (int)(random(roomX + 1156160, roomY) % 3)) + 1;
    int rightTop = (std::max)(top, (int)(random(roomX + 1374024, roomY) % 3));
    rightDoor += random(roomX + 507673, roomY) % (7 - rightTop - rightDoor);
    int topDoor = (std::max)(left, (int)(random(roomX + 5137033, roomY + 1) % 3)) + 1;
    int topRight = (std::max)(right, (int)(random(roomX + 138570, roomY + 1) % 3));
    topDoor += random(roomX + 82001, roomY) % (7 - topRight - topDoor);
    if (roomX == startX && roomY == startY) {
        int entranceX = (start[0] % 8 + 8) % 8;
        if (entranceX <= left + 1)
            left = 0;
        if (entranceX >= 6 - right)
            right = 0;
        int entranceY = (start[1] % 8 + 8) % 8;
        if (entranceY <= bottom + 1)
            bottom = 0;
        if (entranceY >= 6 - top)
            top = 0;
    }
    if (localX <= left) {
        Corridor neighbour = dungeonWay(roomX - 1, roomY, direction, startX, startY, type, regionX, regionY);
        if (!neighbour.kind || abs((int)way.length - (int)neighbour.length) >= 2 || !(way.side <= CorridorSide_West) ||
            (neighbour.side != CorridorSide_East && neighbour.side != CorridorSide_Any) ||
            !(floorDivide(floorDivide((roomX - 1) * 128, BlocksPerPlace), 2) * 2 == regionX) ||
            !(localY == (int)leftDoor))
            return StructureCell(StructureCell_None, 0, 0, 0);
        if (localX == 0) {
            if (way.width == DungeonCellStyle_Overgrown && neighbour.width != DungeonCellStyle_Overgrown)
                return StructureCell(StructureCell_Doorway, RoomPart_MissionRoom, neighbour.width, neighbour.length);
            if ((way.kind == Corridor_GuardianRoom || way.kind == Corridor_EndBossRoom ||
                 way.kind == Corridor_EnemyRoom) &&
                neighbour.length > way.length)
                return StructureCell(StructureCell_Doorway, RoomPart_Gate, way.width, way.length);
            if (way.kind == Corridor_FirstRoom && neighbour.length > way.length)
                return StructureCell(StructureCell_Doorway, RoomPart_EmptyGate, way.width, way.length);
        }
        return StructureCell(StructureCell_Doorway, RoomPart_None, way.width, way.length);
    }
    if (localY <= bottom) {
        Corridor neighbour = dungeonWay(roomX, roomY - 1, direction, startX, startY, type, regionX, regionY);
        if (!neighbour.kind || abs((int)way.length - (int)neighbour.length) >= 2 ||
            !(way.side == CorridorSide_South || way.side == CorridorSide_Any) ||
            (neighbour.side != CorridorSide_North && neighbour.side != CorridorSide_Any) ||
            !(floorDivide(floorDivide((roomY - 1) * 128, BlocksPerPlace), 2) * 2 == regionY) ||
            !(localX == (int)bottomDoor))
            return StructureCell(StructureCell_None, 0, 0, 0);
        if (localY == 0) {
            if (way.width == DungeonCellStyle_Overgrown && neighbour.width != DungeonCellStyle_Overgrown)
                return StructureCell(StructureCell_Doorway, RoomPart_MissionRoom, neighbour.width, neighbour.length);
            if ((way.kind == Corridor_GuardianRoom || way.kind == Corridor_EndBossRoom ||
                 way.kind == Corridor_EnemyRoom) &&
                neighbour.length > way.length)
                return StructureCell(StructureCell_Doorway, RoomPart_Gate, way.width, way.length);
            if (way.kind == Corridor_FirstRoom && neighbour.length > way.length)
                return StructureCell(StructureCell_Doorway, RoomPart_EmptyGate, way.width, way.length);
        }
        return StructureCell(StructureCell_Doorway, RoomPart_None, way.width, way.length);
    }
    if (localX >= 7 - right) {
        Corridor neighbour = dungeonWay(roomX + 1, roomY, direction, startX, startY, type, regionX, regionY);
        if (!neighbour.kind || abs((int)way.length - (int)neighbour.length) >= 2 ||
            !(way.side == CorridorSide_East || way.side == CorridorSide_Any) ||
            (neighbour.side != CorridorSide_West && neighbour.side != CorridorSide_Any) ||
            !(floorDivide(floorDivide((roomX + 1) * 128, BlocksPerPlace), 2) * 2 == regionX) ||
            !(localY == (int)rightDoor))
            return StructureCell(StructureCell_None, 0, 0, 0);
        if (localX == 7) {
            if (way.width == DungeonCellStyle_Overgrown && neighbour.width != DungeonCellStyle_Overgrown)
                return StructureCell(StructureCell_Doorway, RoomPart_MissionRoom, neighbour.width, neighbour.length);
            if ((way.kind == Corridor_GuardianRoom || way.kind == Corridor_EndBossRoom ||
                 way.kind == Corridor_EnemyRoom) &&
                neighbour.length > way.length)
                return StructureCell(StructureCell_Doorway, RoomPart_Gate, way.width, way.length);
            if (way.kind == Corridor_FirstRoom && neighbour.length > way.length)
                return StructureCell(StructureCell_Doorway, RoomPart_EmptyGate, way.width, way.length);
        }
        return StructureCell(StructureCell_Doorway, RoomPart_None, way.width, way.length);
    }
    if (localY >= 7 - top) {
        Corridor neighbour = dungeonWay(roomX, roomY + 1, direction, startX, startY, type, regionX, regionY);
        if (!neighbour.kind || abs((int)way.length - (int)neighbour.length) >= 2 ||
            !(way.side == CorridorSide_North || way.side == CorridorSide_Any) ||
            (neighbour.side != CorridorSide_South && neighbour.side != CorridorSide_Any) ||
            !(floorDivide(floorDivide((roomY + 1) * 128, BlocksPerPlace), 2) * 2 == regionY) ||
            !(localX == (int)topDoor))
            return StructureCell(StructureCell_None, 0, 0, 0);
        if (localY == 7) {
            if (way.width == DungeonCellStyle_Overgrown && neighbour.width != DungeonCellStyle_Overgrown)
                return StructureCell(StructureCell_Doorway, RoomPart_MissionRoom, neighbour.width, neighbour.length);
            if ((way.kind == Corridor_GuardianRoom || way.kind == Corridor_EndBossRoom ||
                 way.kind == Corridor_EnemyRoom) &&
                neighbour.length > way.length)
                return StructureCell(StructureCell_Doorway, RoomPart_Gate, way.width, way.length);
            if (way.kind == Corridor_FirstRoom && neighbour.length > way.length)
                return StructureCell(StructureCell_Doorway, RoomPart_EmptyGate, way.width, way.length);
        }
        return StructureCell(StructureCell_Doorway, RoomPart_None, way.width, way.length);
    }
    int middleX = (left - right + 8) / 2;
    int middleY = (bottom - top + 8) / 2;
    int kind = random(x + 131845018, y) % 3 != 0 ? StructureCell_Room : StructureCell_Enemy;
    int decorations = RoomPart_None;
    if (!skipsCellDecorations(building)) {
        if ((way.flags & CorridorFlagPillars) && random(x + 4623053, y) % 3 == 0) {
            decorations = RoomPart_CentralPillar;
        } else if ((way.flags & CorridorFlagRaisedWalks) && random(x / 2 + 501351, y / 2) % 2 == 0) {
            decorations = RoomPart_RaisedWalk;
        } else if (type == PlaceMines) {
            if (random(x / 2 + 501351, y / 2) % 2 == 0)
                decorations = RoomPart_CrossBeam;
        } else if (type == PlaceGrotto) {
            if (random(x / 2 + 501351, y / 2) % 2 == 0)
                decorations = RoomPart_Stalactite;
        }
    }
    if ((way.kind == Corridor_FirstRoom || way.kind == Corridor_SecondRoom) && localX == left + 1 &&
        localY == bottom + 1)
        return StructureCell(StructureCell_Room, RoomPart_EntranceMarker, way.width, way.length);
    if (way.kind == Corridor_CrossPit) {
        if ((localX <= middleX && localY == (int)leftDoor &&
             dungeonWay(roomX - 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localX >= middleX && localY == (int)rightDoor &&
             dungeonWay(roomX + 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY <= middleY && localX == (int)bottomDoor &&
             dungeonWay(roomX, roomY - 1, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY >= middleY && localX == (int)topDoor &&
             dungeonWay(roomX, roomY + 1, direction, startX, startY, type, regionX, regionY).kind) ||
            (localX == middleX && localY == middleY))
            return StructureCell((unsigned char)kind, (unsigned char)decorations, way.width, way.length);
        return StructureCell(StructureCell_DeepPit, RoomPart_None, way.width, way.length);
    }
    if (way.kind == Corridor_CrossLava) {
        if ((localX <= middleX && localY == (int)leftDoor &&
             dungeonWay(roomX - 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localX >= middleX && localY == (int)rightDoor &&
             dungeonWay(roomX + 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY <= middleY && localX == (int)bottomDoor &&
             dungeonWay(roomX, roomY - 1, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY >= middleY && localX == (int)topDoor &&
             dungeonWay(roomX, roomY + 1, direction, startX, startY, type, regionX, regionY).kind) ||
            (localX == middleX && localY == middleY))
            return StructureCell((unsigned char)kind, (unsigned char)decorations, way.width, way.length);
        return StructureCell(StructureCell_LavaPool, RoomPart_None, way.width, way.length);
    }
    if (way.kind == Corridor_CrossWater) {
        if ((localX <= middleX && localY == (int)leftDoor &&
             dungeonWay(roomX - 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localX >= middleX && localY == (int)rightDoor &&
             dungeonWay(roomX + 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY <= middleY && localX == (int)bottomDoor &&
             dungeonWay(roomX, roomY - 1, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY >= middleY && localX == (int)topDoor &&
             dungeonWay(roomX, roomY + 1, direction, startX, startY, type, regionX, regionY).kind) ||
            (localX == middleX && localY == middleY))
            return StructureCell((unsigned char)kind, (unsigned char)decorations, way.width, way.length);
        return StructureCell(StructureCell_WaterPool, RoomPart_None, way.width, way.length);
    }
    if (way.kind == Corridor_CrossMuck) {
        if ((localX <= middleX && localY == (int)leftDoor &&
             dungeonWay(roomX - 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localX >= middleX && localY == (int)rightDoor &&
             dungeonWay(roomX + 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY <= middleY && localX == (int)bottomDoor &&
             dungeonWay(roomX, roomY - 1, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY >= middleY && localX == (int)topDoor &&
             dungeonWay(roomX, roomY + 1, direction, startX, startY, type, regionX, regionY).kind) ||
            (localX == middleX && localY == middleY))
            return StructureCell((unsigned char)kind, (unsigned char)decorations, way.width, way.length);
        return StructureCell(StructureCell_MuckPool, RoomPart_None, way.width, way.length);
    }
    if (way.kind == Corridor_RimPit) {
        if ((localX == left + 1 && localY == (int)leftDoor &&
             dungeonWay(roomX - 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localX == 6 - right && localX >= middleX && localY == (int)rightDoor &&
             dungeonWay(roomX + 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY == bottom + 1 && localX == (int)bottomDoor &&
             dungeonWay(roomX, roomY - 1, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY == 6 - top && localY >= middleY && localX == (int)topDoor &&
             dungeonWay(roomX, roomY + 1, direction, startX, startY, type, regionX, regionY).kind))
            return StructureCell((unsigned char)kind, (unsigned char)decorations, way.width, way.length);
        // Three in four pool cells get flags 8.
        if (random(x + 347061, y) % 4 != 0)
            return StructureCell(StructureCell_DeepPit, RoomPart_Dais, way.width, way.length);
        return StructureCell(StructureCell_DeepPit, 0, way.width, way.length);
    }
    if (way.kind == Corridor_RimLava) {
        if ((localX == left + 1 && localY == (int)leftDoor &&
             dungeonWay(roomX - 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localX == 6 - right && localX >= middleX && localY == (int)rightDoor &&
             dungeonWay(roomX + 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY == bottom + 1 && localX == (int)bottomDoor &&
             dungeonWay(roomX, roomY - 1, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY == 6 - top && localY >= middleY && localX == (int)topDoor &&
             dungeonWay(roomX, roomY + 1, direction, startX, startY, type, regionX, regionY).kind))
            return StructureCell((unsigned char)kind, (unsigned char)decorations, way.width, way.length);
        if (random(x + 347061, y) % 4 != 0)
            return StructureCell(StructureCell_LavaPool, RoomPart_Dais, way.width, way.length);
        return StructureCell(StructureCell_LavaPool, 0, way.width, way.length);
    }
    if (way.kind == Corridor_RimMuck) {
        if ((localX == left + 1 && localY == (int)leftDoor &&
             dungeonWay(roomX - 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localX == 6 - right && localX >= middleX && localY == (int)rightDoor &&
             dungeonWay(roomX + 1, roomY, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY == bottom + 1 && localX == (int)bottomDoor &&
             dungeonWay(roomX, roomY - 1, direction, startX, startY, type, regionX, regionY).kind) ||
            (localY == 6 - top && localY >= middleY && localX == (int)topDoor &&
             dungeonWay(roomX, roomY + 1, direction, startX, startY, type, regionX, regionY).kind))
            return StructureCell((unsigned char)kind, (unsigned char)decorations, way.width, way.length);
        if (random(x + 347061, y) % 4 != 0)
            return StructureCell(StructureCell_MuckPool, RoomPart_Dais, way.width, way.length);
        return StructureCell(StructureCell_MuckPool, 0, way.width, way.length);
    }
    if (way.kind == Corridor_GuardianRoom) {
        if (localX == middleX && localY == middleY)
            return StructureCell(StructureCell_Guardian,
                                 decorations == RoomPart_RaisedWalk ? RoomPart_RaisedWalk : RoomPart_None, way.width,
                                 way.length);
        return StructureCell(StructureCell_Room, (unsigned char)decorations, way.width, way.length);
    }
    if (way.kind == Corridor_LootRoom) {
        if (localX == middleX && localY == middleY)
            return StructureCell(StructureCell_LootChest,
                                 decorations == RoomPart_RaisedWalk ? RoomPart_RaisedWalk : RoomPart_None, way.width,
                                 way.length);
        return StructureCell(StructureCell_Room, (unsigned char)decorations, way.width, way.length);
    }
    if (way.kind == Corridor_EndBossRoom) {
        if (localX == middleX && localY == middleY)
            return StructureCell(StructureCell_EndBoss,
                                 decorations == RoomPart_RaisedWalk ? RoomPart_RaisedWalk : RoomPart_None, way.width,
                                 way.length);
    }
    if (way.kind == Corridor_RewardRoom) {
        if (localX == middleX && localY == middleY)
            return StructureCell(StructureCell_RewardChest,
                                 decorations == RoomPart_RaisedWalk ? RoomPart_RaisedWalk : RoomPart_None, way.width,
                                 way.length);
        return StructureCell(StructureCell_Room, (unsigned char)decorations, way.width, way.length);
    }
    if (way.kind == Corridor_EnemyRoom) {
        return StructureCell(StructureCell_Enemy, (unsigned char)decorations, way.width, way.length);
    }
    return StructureCell((unsigned char)kind, (unsigned char)decorations, way.width, way.length);
}

// File-static helpers of the movement function below (they go with it). Whether a block stops a creature: anything but
// air and water, and the blocks with BlockFlagBarrier (gates) unless the creature passes them (buff 19) and they also
// have BlockFlagSpiritPassable.
static inline bool isBarrier(const Block &block, bool passesGates)
{
    return block.isSolid() ||
           ((block.flags & BlockFlagBarrier) && (!passesGates || !(block.flags & BlockFlagSpiritPassable)));
}

// How far from a creature's centre the wall it leans on is looked for on a horizontal axis: a quarter block beyond the
// player's box, and half a block beyond the box rounded to whole blocks for the others.
static inline Fixed wallReach(const Creature::EntityData &data, int axis)
{
    float half = data.appearance.size[axis] * 0.5f;
    if (data.hostilityType == HostilityPlayer)
        return Fixed(half) + Fixed(0.25f);
    return Fixed((int)(half + 0.5f)) + Fixed(0.5f);
}

// A barrier block found at the creature's side sets CreatureFlagAtWall and CreatureFlagWallAboveFeet above the lowest
// layer of the creature's box, CreatureFlagAtSpecialWall for BlockType_16 and CreatureFlagAtPassableWall for
// BlockFlagPassableBarrier. The creature's wall normal (m_wallNormal) becomes that of the wall it heads into most
// directly.
static inline void touchWall(Creature *creature, const Block &block, bool aboveFeet,
                             const plasma::Vector<3, float> &heading, const plasma::Vector<3, float> &normal,
                             float &best)
{
    if (aboveFeet)
        creature->m_entityData.flags |= CreatureFlagWallAboveFeet;
    creature->m_entityData.flags |= CreatureFlagAtWall;
    if (block.type == BlockType_16)
        creature->m_entityData.flags |= CreatureFlagAtSpecialWall;
    if (block.flags & BlockFlagPassableBarrier)
        creature->m_entityData.flags |= CreatureFlagAtPassableWall;
    float towards = heading.dot(normal * -1.0f);
    if (towards > best) {
        best = towards;
        creature->m_wallNormal = normal;
    }
}

// The host moves its creatures (hostilityType not 0) and every machine its own player.
void World::moveCreature(Creature *creature, float milliseconds)
{
    if (creature == m_localCreature || (!m_isClient && creature->m_entityData.hostilityType != HostilityPlayer)) {
        // Toggle 2 (facing the aim) while an attack runs; toggle 6 ends.
        creature->m_entityData.binaryToggles.faceAim = 0;
        int ability = creature->m_entityData.currentAbility;
        if (ability != AbilityNone && ability != AbilityCyclone && ability != AbilityHangGlide &&
            ability != AbilitySlowFall && ability != AbilityQuicksandTrap && ability != AbilityFlightToPoint &&
            ability != AbilityWarriorJumpAttack &&
            (ability != AbilityWave || creature->m_entityData.hostilityType != HostilityPlayer) &&
            ability != AbilityRidePet && ability != AbilitySailBoat && ability != AbilityPresentItem &&
            ability != Ability_125 && ability != AbilityCarryCreature && ability != AbilityThrowCreature &&
            ability != AbilityFlameRush && ability != AbilityEagleFlight && ability != AbilityMenuView &&
            ability != AbilityPetAnimal && ability != AbilityDance && ability != AbilitySneak &&
            ability != AbilitySprint && ability != AbilityDash && ability != AbilityDrink && ability != AbilityEat &&
            ability != AbilityOfferPetFood && ability != AbilitySit && ability != AbilitySleep &&
            ability != AbilityLieDown && ability != Ability_90 && ability != AbilitySwirlStrike) {
            // The charged abilities keep it up however long they last.
            if (creature->m_entityData.timeSinceAbility < (float)(creature->attackDuration() + 200) ||
                creature->m_entityData.currentAbility == AbilityCyclone ||
                creature->m_entityData.currentAbility == AbilityBlock ||
                creature->m_entityData.currentAbility == AbilityChargedShot ||
                creature->m_entityData.currentAbility == AbilitySalvo ||
                creature->m_entityData.currentAbility == AbilityChargedBoomerang ||
                creature->m_entityData.currentAbility == AbilitySupportRay ||
                creature->m_entityData.currentAbility == AbilityFireballCharge ||
                creature->m_entityData.currentAbility == AbilityCrush ||
                creature->m_entityData.currentAbility == AbilitySwirl ||
                creature->m_entityData.currentAbility == AbilityMonsterCharge ||
                creature->m_entityData.currentAbility == AbilityRockFistCharge) {
                // The abilities that aim by themselves leave faceAim alone.
                if (!(creature->m_entityData.currentAbility == AbilityHeal ||
                      creature->m_entityData.currentAbility == AbilityCyclone ||
                      creature->m_entityData.currentAbility == AbilityDjinnExplosion ||
                      creature->m_entityData.currentAbility == AbilityFireExplosion ||
                      creature->m_entityData.currentAbility == AbilityLavaPuddle ||
                      creature->m_entityData.currentAbility == AbilitySummon ||
                      creature->m_entityData.currentAbility == AbilityPortCity ||
                      creature->m_entityData.currentAbility == AbilitySelfKill ||
                      creature->m_entityData.currentAbility == AbilityCenteredHealingStream ||
                      creature->m_entityData.currentAbility == AbilityFireBreath ||
                      creature->m_entityData.currentAbility == AbilityUseSpecialItem ||
                      creature->m_entityData.currentAbility == AbilityAssassinate ||
                      creature->m_entityData.currentAbility == AbilityShadowClone ||
                      creature->m_entityData.currentAbility == AbilityWaterBubbles ||
                      creature->m_entityData.currentAbility == AbilityHeroicShout ||
                      creature->m_entityData.currentAbility == AbilityCallPet)) {
                    if (!creature->m_entityData.binaryToggles.active &&
                        !creature->m_entityData.binaryToggles.noTerrainCollision &&
                        !creature->m_entityData.binaryToggles.attachedToFurniture)
                        creature->m_entityData.binaryToggles.faceAim = 1;
                }
                creature->m_entityData.binaryToggles.fastMovement = 0;
            }
        }
        if (creature->m_entityData.currentAbility == AbilitySelfKill &&
            creature->m_entityData.acceleration.squaredLength() > 0.0f)
            creature->m_entityData.currentAbility = AbilityNone;
    }

    float seconds = milliseconds * 0.001f;
    if (creature->m_entityData.currentAbility == AbilitySailBoat &&
        (creature->m_entityData.flags & CreatureFlagOnGround))
        creature->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    plasma::Vector<3, float> push = creature->m_entityData.acceleration * seconds;
    if (creature->m_entityData.stunTime > 0.0f)
        push = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    if (!creature->isWraithTargeting() && creature->m_entityData.currentAbility != AbilityHangGlide &&
        creature->m_entityData.currentAbility != AbilityClimb &&
        (creature->m_entityData.flags & CreatureFlagPartlyAboveWater) && push[2] > 0.0f)
        push[2] = 0.0f;

    // The speed limit, in blocks a second.
    float speed = WalkSpeed;
    if (creature->m_entityData.currentAbility == AbilitySprint && creature->m_stamina > 0.0f)
        speed = WalkSpeed * SprintSpeedFactor;
    if (creature->m_entityData.currentAbility == AbilityWarriorJumpAttack &&
        creature->m_entityData.timeSinceAbility < (float)creature->attackDuration())
        speed = JumpAttackSpeed;
    if (creature->isLarge() || (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagTinted))
        speed = LargeCreatureSpeed;
    if (creature->m_entityData.currentAbility == AbilityFeedClimbingPlant &&
        creature->m_entityData.timeSinceAbility < (float)creature->attackDuration())
        speed = NearlyStillSpeed;
    if (creature == m_localCreature || (creature->m_entityData.hostilityType != HostilityPlayer && !m_isClient)) {
        // The speed scale of what the creature moves through, by its CreatureFlagInWater.
        creature->m_entityData.mediumSpeed = 1.0f;
        if (creature->m_entityData.currentAbility == AbilitySailBoat &&
            (creature->m_entityData.flags & CreatureFlagInWater)) {
            creature->m_entityData.mediumSpeed = creature->sailingSpeed();
        } else if (creature->m_entityData.currentAbility != AbilitySailBoat &&
                   (creature->m_entityData.flags & CreatureFlagInWater) && !creature->isBoosted() &&
                   !creature->isLarge()) {
            creature->m_entityData.mediumSpeed = creature->swimmingSpeed();
        } else if ((creature->m_entityData.flags & CreatureFlagInLava) && !creature->isBoosted() &&
                   !creature->isLarge()) {
            creature->m_entityData.mediumSpeed = LavaMediumSpeed;
        } else if ((creature->m_entityData.flags & CreatureFlagInPoison) && !creature->isBoosted() &&
                   !creature->isLarge()) {
            creature->m_entityData.mediumSpeed = PoisonMediumSpeed;
        } else if ((creature->m_entityData.flags & CreatureFlagOnSnow) && !creature->isBoosted() &&
                   !creature->isLarge()) {
            creature->m_entityData.mediumSpeed = SnowMediumSpeed;
        } else if (creature->m_entityData.flags & CreatureFlagOnFoliage) {
            creature->m_entityData.mediumSpeed = FoliageMediumSpeed;
        }
        if (creature->m_stepOffset > 0.1f)
            creature->m_entityData.mediumSpeed *= SteppingSpeedFactor;
        if (creature->m_entityData.flags & CreatureFlagOnPath)
            creature->m_entityData.mediumSpeed *= PathSpeedFactor;
        creature->m_entityData.lampDiameter = creature->lampDiameter();
    }
    speed *= creature->m_entityData.mediumSpeed;

    if (creature == m_localCreature && creature->m_entityData.currentAbility == AbilityRidePet) {
        // Riding ends in water; a living pet as the mount is faster.
        if (creature->m_entityData.flags & CreatureFlagInWater) {
            creature->m_entityData.currentAbility = AbilityNone;
        } else {
            long long mount = creature->m_petId;
            if (creatureByID(mount) && creatureByID(mount)->m_entityData.hp > 0.0f &&
                isPet(creatureByID(mount)->m_entityData.race) && !(creature->m_entityData.flags & CreatureFlagInWater))
                speed *= RidePetSpeedFactor;
        }
    }
    if (creature->m_entityData.currentAbility == AbilityHangGlide) {
        // Hang gliding: slow at first, and the pitch follows the course.
        bonusFromLevel(creature->m_entityData.actionItem.modifier);
        if (!(creature->m_entityData.flags & CreatureFlagOnGround)) {
            float glide = std::max(0.0f, creature->m_glideTime * 0.001f - 0.5f);
            glide = std::min(1.0f, glide);
            speed *= (1.0f - glide) * 2.0f + glide * 3.0f;
            plasma::Vector<3, double> course(creature->m_entityData.velocity);
            double slope = 1.0 / sqrt(course.squaredLength()) * course[2];
            float share = blendFactor(milliseconds, 0.1f);
            creature->m_entityData.rotation[0] =
                interpolateAngle(creature->m_entityData.rotation[0], (float)(asin(slope) * DegreesPerRadian), share);
        }
    }
    if (creature->m_entityData.currentAbility == AbilitySneak && (creature->m_entityData.flags & CreatureFlagInWater))
        creature->m_entityData.currentAbility = AbilityNone;
    if (creature->m_entityData.binaryToggles.fastMovement && creature->m_entityData.currentAbility != AbilitySneak &&
        (creature->m_entityData.currentAbility != AbilityClimb ||
         !(creature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagAtWall))) &&
        (creature->m_entityData.currentAbility != AbilityHangGlide ||
         (creature->m_entityData.flags & CreatureFlagOnGround))) {
        if (creature->m_entityData.currentAbility == AbilityFloat)
            speed *= FastFloatSpeedFactor;
        else
            speed *= FastSpeedFactor;
    }
    if (creature->findBuff(BuffSwiftness))
        speed *= SwiftnessSpeedFactor;
    if (creature->findBuff(BuffFlamerush) ||
        (creature->m_entityData.currentAbility == AbilityFlameRush && creature->m_entityData.mp > 0.0f)) {
        // The creature's own orb: a projectile of ElementFlameRush that stays at its middle; made when none is within
        // three blocks.
        speed *= FlameRushSpeedFactor;
        bool found = false;
        for (std::list<ServerUpdates::BulletPacket>::iterator it = m_projectiles.begin(); it != m_projectiles.end();
             ++it) {
            if (it->element == ElementFlameRush && it->ownerId == creature->m_id) {
                plasma::Vector<3, float> middle(0.0f, 0.0f, creature->m_entityData.appearance.size[2] * 0.5f);
                plasma::Vector<3, Fixed> offset =
                    it->position - creature->m_entityData.position + plasma::Vector<3, Fixed>(middle);
                if (plasma::Vector<3, float>(offset).squaredLength() < 9.0f) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            ServerUpdates::BulletPacket orb;
            orb.knockback = 0.0f;
            orb.projectileType = ProjectileGroundSpot;
            orb.powered = 0;
            orb.strength = 0;
            orb.damage = creature->damageScale(creature->m_entityData.currentAbility) * creature->attackPower(false);
            orb.sizeInBlocks = FlameRushOrbSize;
            orb.ownerId = creature->m_id;
            orb.element = ElementFlameRush;
            plasma::Vector<3, float> feet(0.0f, 0.0f, 0.01f - creature->m_entityData.appearance.size[2] * 0.5f);
            orb.position = creature->m_entityData.position + plasma::Vector<3, Fixed>(feet);
            orb.visualSize = 1.0f;
            orb.currentTime = 0;
            orb.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_projectiles.push_back(orb);
            m_serverUpdates.m_bulletPacketList.push_back(orb);
        }
    }

    // The dashes: 50 (and 55 while it strikes, 52 and 82 for half a second) at speed 40.
    if (creature->m_entityData.currentAbility == AbilityIntercept ||
        (creature->m_entityData.currentAbility == AbilitySmash &&
         creature->m_entityData.timeSinceAbility < (float)(creature->anticipationTime() + creature->strikeTime())) ||
        (creature->m_entityData.currentAbility == AbilityRetreat && creature->m_entityData.timeSinceAbility < 500.0f))
        speed = 40.0f;
    bool hit = false;
    // Charging: an interception, or the first 200 ms of a melee attack.
    unsigned char ability = creature->m_entityData.currentAbility;
    bool charging =
        ability == AbilityIntercept ||
        ((ability == AbilityRightSlash || ability == AbilityLeftSlash || ability == Ability_4 || ability == Ability_5 ||
          ability == AbilityPerforate || ability == AbilityRightStab || ability == AbilityLeftStab ||
          ability == AbilityShieldAttack || ability == AbilityBlockJab || ability == AbilitySwirlStrike ||
          ability == AbilityCrossedWeaponSweep || ability == AbilityLeftSweep || ability == AbilityRightSweep ||
          ability == AbilityOverheadChop || ability == AbilityMonsterChargeStrike || ability == AbilityAmbush ||
          ability == AbilityLeftDaggerPierce || ability == AbilityRightDaggerPierce || ability == AbilityKick ||
          ability == AbilityTwoHandedThrust || ability == Ability_63 || ability == AbilityTwoHandedSpin ||
          ability == AbilityTwoHandedSlam || ability == AbilityTwoHandedSwing) &&
         creature->m_entityData.timeSinceAbility < 200.0f);
    if (creature->m_entityData.currentAbility == AbilityDash) {
        if (creature->m_entityData.timeSinceAbility < 500.0f)
            speed = 40.0f;
        else
            creature->m_entityData.currentAbility = AbilityNone;
    }
    if ((creature == m_localCreature || !m_isClient) && charging) {
        // The first 200 ms of a melee attack (and the whole dash 50) carry the creature forwards until it reaches
        // something it can damage.
        for (Creature *other : m_creatures) {
            if (other == creature || !(other->m_entityData.hp > 0.0f) || !creature->canDoDamage(other))
                continue;
            float reach =
                creature->m_entityData.appearance.size[0] + 1.0f + other->m_entityData.appearance.size[0] * 0.7f;
            plasma::Vector<3, Fixed> offset = creature->m_entityData.position - other->m_entityData.position;
            // Keeps only the horizontal offset, so the height compared below is always zero.
            plasma::Vector<3, Fixed> flat(offset[0], offset[1], Fixed(0));
            Fixed squaredDistance = flat[0] * flat[0] + flat[1] * flat[1];
            if (squaredDistance < Fixed(reach * reach) &&
                other->m_entityData.appearance.size[2] * 0.5f + creature->m_entityData.appearance.size[2] + 1.0f >
                    (float)fabs((float)flat[2]))
                hit = true;
        }
        if (hit || creature->m_entityData.timeSinceAbility > DashTimeLimit) {
            if (creature->m_entityData.currentAbility == AbilityIntercept) {
                // The dash ends in the weapon's strike.
                creature->m_strikeStrength = creature->m_entityData.mp;
                creature->m_entityData.mp = 0.0f;
                creature->m_entityData.currentAbility = AbilityNone;
                creature->m_entityData.timeSinceAbility = 0.0f;
                creature->m_criticalStrike = 0;
                if (creature->m_entityData.equipment.weaponRight.category == ItemCategoryWeapon) {
                    if (creature->m_entityData.equipment.weaponRight.id == WeaponLongsword)
                        creature->m_entityData.currentAbility = AbilityPerforate;
                    else
                        creature->m_entityData.currentAbility =
                            creature->m_entityData.equipment.weaponRight.id == WeaponDagger ? AbilityAmbush
                                                                                            : AbilityKick;
                    creature->m_entityData.timeSinceAbility = (float)(creature->anticipationTime() + 1);
                    if (creature->findBuff(BuffDodge)) {
                        Creature::Buff buff;
                        buff.type = BuffDodge;
                        buff.strength = 0.0f;
                        buff.timeRemaining = 0.0f;
                        buff.sourceId = 0;
                        creature->setBuff(buff, false);
                        ServerUpdates::BuffPacket packet;
                        packet.buff = buff;
                        packet.sourceId = creature->m_id;
                        packet.targetId = creature->m_id;
                        m_serverUpdates.m_buffPacketList.push_back(packet);
                        creature->m_criticalStrike = 1;
                    }
                }
            }
            if (creature->m_entityData.currentAbility == AbilitySmashLeap) {
                creature->m_strikeStrength = 0.0f;
                creature->m_entityData.currentAbility = AbilitySmash;
            }
        }
        if (!hit) {
            plasma::Matrix<float> turn;
            turn.identity();
            turn.rotateZ(creature->m_entityData.rotation[2]);
            push += turn.transformDirection(plasma::Vector<3, float>(0.0f, speed * 2.0f, 0.0f));
        }
    }

    if (creature->m_entityData.currentAbility == AbilityEat) {
        if (creature->m_entityData.timeSinceAbility < (float)creature->m_entityData.actionItem.cooldown())
            speed = 0.0f;
        else
            creature->m_entityData.currentAbility = AbilityNone;
    }
    if (creature->m_entityData.rollTime != 0.0f) {
        // Rolling ends the ability, except the three that roll themselves and a ninja's attacks 6, 18 and 21.
        if ((creature->m_entityData.currentAbility != AbilityRetreat &&
             creature->m_entityData.currentAbility != AbilitySprint &&
             creature->m_entityData.currentAbility != AbilityShuriken) ||
            creature->m_entityData.timeSinceAbility > (float)creature->attackDuration()) {
            if (creature->m_entityData.classType == ClassRogue &&
                creature->m_entityData.specialization == SpecializationNinja) {
                if (creature->m_entityData.currentAbility != AbilityPerforate &&
                    creature->m_entityData.currentAbility != AbilityAmbush &&
                    creature->m_entityData.currentAbility != AbilityKick)
                    creature->m_entityData.currentAbility = AbilityNone;
            } else {
                creature->m_entityData.currentAbility = AbilityNone;
                creature->m_entityData.charge = 0.0f;
            }
        }
    }

    // The abilities that move the creature themselves; `keepsFacing` keeps it from turning to its course or aim at the
    // end.
    bool keepsFacing = false;
    if ((creature->m_entityData.currentAbility == AbilityHeadbutt ||
         creature->m_entityData.currentAbility == AbilityHornCharge ||
         creature->m_entityData.currentAbility == AbilityLunge) &&
        creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime())
        speed = 0.0f;
    if ((creature->m_entityData.currentAbility == AbilityHeadbutt ||
         creature->m_entityData.currentAbility == AbilityHornCharge ||
         creature->m_entityData.currentAbility == AbilityRockFist ||
         creature->m_entityData.currentAbility == AbilityLunge) &&
        creature->m_entityData.timeSinceAbility > (float)creature->anticipationTime() &&
        creature->m_entityData.timeSinceAbility <
            (float)(creature->anticipationTime() + creature->strikeTime() * 3 / 4)) {
        plasma::Matrix<float> turn;
        turn.identity();
        turn.rotateZ(creature->m_entityData.rotation[2]);
        plasma::Vector<3, float> forwards = turn.transformDirection(plasma::Vector<3, float>(0.0f, 1.0f, 0.0f));
        push[0] = forwards[0] * ChargePush;
        push[1] = forwards[1] * ChargePush;
        speed = ChargeSpeed;
        keepsFacing = true;
    }
    if (creature->m_entityData.currentAbility == AbilityCyclone ||
        (creature->m_entityData.currentAbility == AbilityWarriorJumpAttack &&
         creature->m_entityData.timeSinceAbility < (float)creature->attackDuration()))
        keepsFacing = true;
    if (creature->m_entityData.currentAbility == AbilityDashLeft &&
        creature->m_entityData.timeSinceAbility > (float)creature->anticipationTime() &&
        creature->m_entityData.timeSinceAbility < (float)(creature->anticipationTime() + creature->strikeTime())) {
        plasma::Matrix<float> turn;
        turn.identity();
        turn.rotateZ(creature->m_entityData.rotation[2]);
        push = turn.transformDirection(plasma::Vector<3, float>(-1.0f, 0.0f, 0.0f)) * DashPush;
        speed = DashSpeed;
        keepsFacing = true;
    }
    if (creature->m_entityData.currentAbility == AbilityDashRight &&
        creature->m_entityData.timeSinceAbility > (float)creature->anticipationTime() &&
        creature->m_entityData.timeSinceAbility < (float)(creature->anticipationTime() + creature->strikeTime())) {
        plasma::Matrix<float> turn;
        turn.identity();
        turn.rotateZ(creature->m_entityData.rotation[2]);
        push = turn.transformDirection(plasma::Vector<3, float>(1.0f, 0.0f, 0.0f)) * DashPush;
        speed = DashSpeed;
        keepsFacing = true;
    }
    if (creature->m_entityData.currentAbility == AbilityDashForward &&
        creature->m_entityData.timeSinceAbility > (float)creature->anticipationTime() &&
        creature->m_entityData.timeSinceAbility < (float)(creature->anticipationTime() + creature->strikeTime())) {
        plasma::Matrix<float> turn;
        turn.identity();
        turn.rotateZ(creature->m_entityData.rotation[2]);
        push = turn.transformDirection(plasma::Vector<3, float>(0.0f, 1.0f, 0.0f)) * DashPush;
        speed = DashSpeed;
        keepsFacing = true;
    }
    if (creature->m_entityData.currentAbility == AbilityShellBlock &&
        creature->m_entityData.timeSinceAbility < (float)creature->attackDuration()) {
        speed = 0.0f;
        keepsFacing = true;
    }
    if (creature->m_entityData.currentAbility == AbilityShellSpin &&
        creature->m_entityData.timeSinceAbility < (float)creature->attackDuration()) {
        creature->m_entityData.rotation[2] += seconds * ShellSpinRate;
        speed = ShellSpinSpeed;
        keepsFacing = true;
    }
    if ((creature->m_entityData.currentAbility == AbilityHeadbutt ||
         creature->m_entityData.currentAbility == AbilityHornCharge ||
         creature->m_entityData.currentAbility == AbilityRockFist ||
         creature->m_entityData.currentAbility == AbilityLunge) &&
        creature->m_entityData.timeSinceAbility > (float)creature->anticipationTime() &&
        creature->m_entityData.timeSinceAbility < (float)creature->attackDuration())
        keepsFacing = true;
    if (creature->m_entityData.hostilityType == Hostility_5)
        speed = 30.0f;
    if ((creature->m_entityData.currentAbility == AbilityTwoHandedSweep ||
         creature->m_entityData.currentAbility == AbilityStomp) &&
        creature->m_entityData.timeSinceAbility < (float)creature->attackDuration() &&
        !(creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime()) &&
        !(creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime())) {
        speed = NearlyStillSpeed;
        keepsFacing = true;
    }

    // Charging: the charge grows with the tempo while the ability is held, up to the mana.
    if (creature->m_entityData.currentAbility == AbilityFireballCharge)
        creature->m_entityData.charge += creature->tempo() * (milliseconds * FireballChargeRate);
    if ((creature->m_entityData.currentAbility == AbilityBlock ||
         creature->m_entityData.currentAbility == AbilityChargedShot ||
         creature->m_entityData.currentAbility == AbilitySalvo ||
         creature->m_entityData.currentAbility == AbilityChargedBoomerang ||
         creature->m_entityData.currentAbility == AbilityCrush ||
         creature->m_entityData.currentAbility == AbilitySwirl ||
         creature->m_entityData.currentAbility == AbilityMonsterCharge ||
         creature->m_entityData.currentAbility == AbilityRockFistCharge) &&
        creature->m_entityData.timeSinceAbility >= (float)creature->anticipationTime()) {
        float gain = creature->tempo() * (milliseconds * ChargeRate);
        if (creature->m_entityData.classType == ClassRanger &&
            creature->m_entityData.specialization == SpecializationSniper)
            gain *= (std::min)(4.0f, (float)creature->m_entityData.hitCombo / (float)creature->maxCombo()) + 1.0f;
        if (creature->findBuff(BuffWarfrenzy))
            gain *= WarFrenzyChargeFactor;
        if (creature->findBuff(BuffInstantshoot))
            gain = creature->m_entityData.mp;
        if (creature->m_entityData.currentAbility == AbilityRockFistCharge) {
            // Ability 147 turns stamina into charge, and the charge raises the mana.
            float used = std::min(creature->m_stamina, gain);
            creature->m_stamina -= used;
            creature->m_entityData.charge += used;
            if (creature->m_entityData.charge > 1.0f)
                creature->m_entityData.charge = 1.0f;
            if (creature->m_entityData.charge > creature->m_entityData.mp)
                creature->m_entityData.mp = creature->m_entityData.charge;
        } else {
            creature->m_entityData.charge += gain;
        }
    }
    if (creature->m_entityData.charge > creature->m_entityData.mp)
        creature->m_entityData.charge = creature->m_entityData.mp;

    if (creature->m_entityData.slowedTime != 0.0f)
        speed *= SlowedSpeedFactor;
    if (creature->findBuff(BuffFrozen))
        speed *= FrozenSpeedFactor;
    if (creature->findBuff(BuffDefenseWeakened))
        speed *= WeakenedSpeedFactor;
    if (creature->m_entityData.sprintTime != 0.0f)
        speed *= SprintBoostSpeedFactor;
    if (creature->m_entityData.binaryToggles.faceAim && creature->m_entityData.rollTime == 0.0f &&
        (creature->m_entityData.currentAbility != AbilityRetreat ||
         creature->m_entityData.timeSinceAbility > RetreatFacingDelay)) {
        // Facing the aim: moving away from it is slower, down to half the speed straight backwards.
        plasma::Vector<2, float> course(creature->m_entityData.velocity[0], creature->m_entityData.velocity[1]);
        plasma::Vector<2, float> aim(creature->m_entityData.attackRotation[0],
                                     creature->m_entityData.attackRotation[1]);
        if (aim.dot(course) < 0.0f) {
            aim.normalize();
            speed *= course.normalized().dot(aim) * 0.5f + 1.0f;
        }
    }
    if (creature->isWraithTargeting())
        speed = 10.0f;
    if (creature->m_entityData.currentAbility == AbilityClimb) {
        speed *= 0.5f;
        creature->m_entityData.retreat = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    }
    // The time in the air with the glider.
    if (creature->m_entityData.currentAbility == AbilityHangGlide &&
        !(creature->m_entityData.flags & CreatureFlagOnGround))
        creature->m_glideTime += milliseconds;
    else
        creature->m_glideTime = 0.0f;

    creature->m_entityData.velocity += push;
    if (creature->m_entityData.currentAbility == AbilityClimb && (creature->m_entityData.flags & CreatureFlagAtWall)) {
        if (creature->m_entityData.velocity.squaredLength() > speed * speed)
            creature->m_entityData.velocity = creature->m_entityData.velocity.normalized() * speed;
    } else {
        plasma::Vector<2, float> course(creature->m_entityData.velocity[0], creature->m_entityData.velocity[1]);
        if (course.squaredLength() > speed * speed) {
            course = course.normalized() * speed;
            creature->m_entityData.velocity[0] = course[0];
            creature->m_entityData.velocity[1] = course[1];
        }
    }

    // Gravity. m_fallStartHeight keeps the height the creature last rose or floated at, in blocks. A climber at a wall
    // slows to a halt (0.0025); the share of gravity: half for the carried and the jump attack, a tenth for fliers and
    // floaters, 0.035 for a slow fall, and a glider's falling from all of it to a quarter over its time in the air.
    if (creature->m_entityData.currentAbility == AbilityClimb && (creature->m_entityData.flags & CreatureFlagAtWall)) {
        blend(creature->m_entityData.velocity, plasma::Vector<3, float>(0.0f, 0.0f, 0.0f), milliseconds, 0.0025f);
        if (creature->m_entityData.velocity[2] > -10.0f)
            creature->m_fallStartHeight = (float)creature->m_entityData.position[2];
    } else if (!creature->m_entityData.binaryToggles.active &&
               !creature->m_entityData.binaryToggles.noTerrainCollision &&
               !creature->m_entityData.binaryToggles.attachedToFurniture &&
               creature->m_entityData.currentAbility != AbilityFlightToPoint) {
        Creature *carrier = creatureByID(creature->m_entityData.focusId);
        if (!creature->isWraithTargeting() && creature->m_entityData.currentAbility != AbilityEagleFlight &&
            creature->m_entityData.currentAbility != AbilityMenuView) {
            if (creature->m_entityData.currentAbility == AbilityCarryCreature && carrier &&
                (carrier->m_entityData.appearance.appearanceFlags & AppearanceFlagFlying) &&
                creature->m_entityData.stunTime <= 0.0f) {
                creature->m_entityData.velocity[2] -= seconds * 0.5f * Gravity;
                creature->m_fallStartHeight = (float)creature->m_entityData.position[2];
            } else if (((creature->m_entityData.appearance.appearanceFlags & AppearanceFlagFlying) &&
                        creature->m_entityData.stunTime <= 0.0f) ||
                       creature->m_entityData.currentAbility == AbilityFloat ||
                       ((creature->m_entityData.currentAbility == AbilityHealingStream ||
                         creature->m_entityData.currentAbility == AbilityCenteredHealingStream) &&
                        creature->m_entityData.timeSinceAbility < (float)creature->attackDuration())) {
                creature->m_entityData.velocity[2] -= seconds * 0.1f * Gravity;
                creature->m_fallStartHeight = (float)creature->m_entityData.position[2];
            } else if (creature->m_entityData.currentAbility == AbilityWarriorJumpAttack &&
                       creature->m_entityData.velocity[2] < 0.0f &&
                       creature->m_entityData.timeSinceAbility < (float)creature->attackDuration()) {
                creature->m_entityData.velocity[2] -= seconds * 0.5f * Gravity;
                creature->m_fallStartHeight = (float)creature->m_entityData.position[2];
            } else if (creature->m_entityData.currentAbility == AbilitySlowFall &&
                       creature->m_entityData.stunTime <= 0.0f) {
                creature->m_entityData.velocity[2] -= seconds * 0.035f * Gravity;
                creature->m_fallStartHeight = (float)creature->m_entityData.position[2];
            } else if (creature->m_entityData.currentAbility == AbilityHangGlide) {
                // The glider carries fully at first and a quarter less with the time in the air.
                float glide = std::max(0.0f, creature->m_glideTime * 0.003f - 0.5f);
                float sink = 1.0f - std::min(1.0f, glide) * 0.75f;
                creature->m_entityData.velocity[2] -= sink * seconds * Gravity;
                creature->m_fallStartHeight = (float)creature->m_entityData.position[2];
            } else {
                if (creature->m_entityData.velocity[2] >= 0.0f)
                    creature->m_fallStartHeight = (float)creature->m_entityData.position[2];
                creature->m_entityData.velocity[2] -= seconds * Gravity;
            }
        }
    }

    if (creature == m_localCreature && creature->m_entityData.currentAbility == AbilityHangGlide) {
        // The player's glider: it ends in water, a hard landing stuns, and the wind carries it.
        if (creature->m_entityData.flags & CreatureFlagInWater) {
            creature->m_entityData.currentAbility = AbilityNone;
        } else if (!(creature->m_entityData.flags & CreatureFlagOnGround)) {
            if (creature->m_glideTime > 1000.0f && (creature->m_entityData.flags & CreatureFlagAtWall) &&
                creature->m_entityData.velocity.squaredLength() > 64.0f &&
                (creature->m_wallNormal & creature->m_entityData.velocity) < -4.0f) {
                creature->m_entityData.stunTime = (float)creature->maxStunTime();
                creature->m_entityData.currentAbility = AbilityNone;
                ServerUpdates::SoundPacket sound;
                sound.position = creature->m_entityData.position;
                sound.speed = 1.0f;
                sound.sound = SoundSwordHit03;
                m_serverUpdates.m_soundPacketList.push_back(sound);
            }
            creature->m_entityData.velocity +=
                wind(creature->m_entityData.position, creature->m_entityData.velocity) * seconds;
        } else if ((float)fabs(creature->m_entityData.rotation[1]) > 40.0f) {
            creature->m_entityData.stunTime = (float)creature->maxStunTime();
            creature->m_entityData.currentAbility = AbilityNone;
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = 1.0f;
            sound.sound = SoundSwordHit03;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
    }
    if (creature->m_entityData.stunTime > 0.0f)
        creature->m_entityData.charge = 0.0f;

    // The roll: the creature leans into its turns, by the angle between where it heads and where it accelerates to; a
    // glider leans much more.
    float lean = 0.0f;
    plasma::Vector<2, float> steer(creature->m_entityData.acceleration[0], creature->m_entityData.acceleration[1]);
    if (steer.squaredLength() > 0.01) {
        plasma::Vector<2, float> course(creature->m_entityData.velocity[0], creature->m_entityData.velocity[1]);
        if (course.squaredLength() > 0.01f) {
            steer.normalize();
            course.normalize();
            float turn = course[1] * steer[0] - course[0] * steer[1];
            turn = std::max(-1.0f, turn);
            lean = (float)(asinf(std::min(1.0f, turn)) / Pi * 180.0 * 0.5);
        }
    }
    if (creature->m_entityData.appearance.size[0] > 2.0f)
        lean *= 2.0f / creature->m_entityData.appearance.size[0];
    if (creature->m_entityData.binaryToggles.faceAim)
        lean = 0.0f;
    if (creature->isGliding()) {
        lean *= 40.0f;
        lean = std::min(90.0f, lean);
        lean = std::max(-90.0f, lean);
        blend(creature->m_entityData.rotation[1], lean, milliseconds, 0.002f);
    } else {
        blend(creature->m_entityData.rotation[1], lean, milliseconds, 0.005f);
    }

    // The way of this step, in blocks.
    plasma::Vector<3, float> way = (creature->m_entityData.velocity + creature->m_entityData.retreat) * seconds;
    if ((creature->m_entityData.flags & CreatureFlagAtWall) && creature->m_entityData.currentAbility == AbilityClimb)
        way = way - creature->m_wallNormal * (seconds * 0.01f);

    // The other creatures push the creature out of their boxes.
    if (!(creature->m_entityData.appearance.appearanceFlags & AppearanceFlagNoCreatureCollision) &&
        creature->m_entityData.currentAbility != AbilitySit && creature->m_entityData.currentAbility != AbilitySleep &&
        creature->m_entityData.currentAbility != AbilityLieDown &&
        creature->m_entityData.currentAbility != AbilityRideNpcMount &&
        creature->m_entityData.currentAbility != AbilityRestHeal &&
        creature->m_entityData.currentAbility != AbilityAttachToTarget && !creature->isWraithTargeting() &&
        ((creature->m_entityData.currentAbility != AbilityAssassinate &&
          creature->m_entityData.currentAbility != AbilityRockFist) ||
         !(creature->m_entityData.timeSinceAbility < (float)creature->attackDuration()))) {
        for (Creature *other : m_creatures) {
            // Not the creature it carries or is carried by.
            if ((creature->m_entityData.currentAbility == AbilityCarryCreature ||
                 creature->m_entityData.currentAbility == AbilityThrowCreature) &&
                creature->m_entityData.focusId == other->m_id)
                continue;
            if (other->m_entityData.currentAbility == AbilityCarryRider &&
                other->m_entityData.focusId == creature->m_id)
                continue;
            if (other == creature || (other->m_entityData.appearance.appearanceFlags & AppearanceFlagObstacle) ||
                !(other->m_entityData.hp > 0.0f))
                continue;
            // Hostility types 7 and 8 (the obstacles) only collide with each other, except race 285; the player passes
            // types 0, 3 and 6.
            if ((creature->m_entityData.hostilityType == HostilityInteractiveObject ||
                 (creature->m_entityData.hostilityType == HostilityAttackableObject &&
                  creature->m_entityData.race != RaceWaterBubble)) &&
                other->m_entityData.hostilityType != HostilityAttackableObject &&
                other->m_entityData.hostilityType != HostilityInteractiveObject)
                continue;
            if (creature->m_entityData.hostilityType == HostilityPlayer &&
                (other->m_entityData.hostilityType == HostilityPlayer ||
                 other->m_entityData.hostilityType == HostilityFriendlyNpc ||
                 other->m_entityData.hostilityType == HostilityPet))
                continue;
            if (other->m_entityData.currentAbility == AbilityRideNpcMount ||
                other->m_entityData.currentAbility == AbilityAssassinate || other->isWraithTargeting() ||
                (other->m_entityData.appearance.appearanceFlags & AppearanceFlagNoCreatureCollision))
                continue;
            if (other->m_entityData.race == RaceWaterBubble && creature->m_entityData.race != RaceWaterBubble)
                continue;
            plasma::Vector<3, float> otherSize = other->m_entityData.appearance.size;
            if (other->m_entityData.hostilityType != HostilityAttackableObject &&
                other->m_entityData.hostilityType != HostilityInteractiveObject)
                otherSize += plasma::Vector<3, float>(1.0f, 1.0f, 0.0f);
            // The boxes, each of its size around its position, must meet.
            plasma::Vector<3, Fixed> reach = plasma::Vector<3, Fixed>(creature->m_entityData.appearance.size * 0.5f);
            plasma::Vector<3, Fixed> otherReach = plasma::Vector<3, Fixed>(otherSize * 0.5f);
            if (creature->m_entityData.position[0] + reach[0] < other->m_entityData.position[0] - otherReach[0] ||
                creature->m_entityData.position[0] - reach[0] >= other->m_entityData.position[0] + otherReach[0] ||
                creature->m_entityData.position[1] + reach[1] < other->m_entityData.position[1] - otherReach[1] ||
                creature->m_entityData.position[1] - reach[1] >= other->m_entityData.position[1] + otherReach[1] ||
                creature->m_entityData.position[2] + reach[2] < other->m_entityData.position[2] - otherReach[2] ||
                creature->m_entityData.position[2] - reach[2] >= other->m_entityData.position[2] + otherReach[2])
                continue;
            plasma::Vector<3, float> away =
                plasma::Vector<3, float>(creature->m_entityData.position - other->m_entityData.position);
            if (away.squaredLength() < 0.0001f) {
                float angle = (float)(rand() * TwoPi / (double)RAND_MAX);
                away = plasma::Vector<3, float>(cosf(angle), sinf(angle), 0.0f);
            }
            away[2] = 0.0f;
            if (away.squaredLength() > 0.0f) {
                away.normalize();
                if (creature->m_entityData.stunTime > 0.0f)
                    away *= 0.1f;
                // A larger creature pushes harder.
                float weight = other->m_entityData.appearance.size[0] / creature->m_entityData.appearance.size[0];
                plasma::Vector<3, float> pushed = way + away * weight;
                blend(way, pushed, milliseconds, 0.001f);
            }
        }
    }

    if ((!m_isClient || creature == m_localCreature) &&
        !(creature->m_entityData.appearance.appearanceFlags & AppearanceFlagNoTerrainCollision) &&
        !creature->m_entityData.binaryToggles.noTerrainCollision &&
        creature->m_entityData.currentAbility != AbilityFlightToPoint && !creature->isWraithTargeting())
        updateCollision(creature);

    // The way is gone in steps of at most two thirds of a block.
    int steps = (int)(sqrtf(way.squaredLength()) * 1.5f + 1.0f);
    way *= 1.0f / (float)steps;
    bool walking =
        creature->m_entityData.currentAbility != AbilitySit && creature->m_entityData.currentAbility != AbilitySleep &&
        creature->m_entityData.currentAbility != AbilityClimb && !creature->m_entityData.binaryToggles.active &&
        !creature->m_entityData.binaryToggles.attachedToFurniture;

    // The obstacle creatures (m_unused50) whose box the creature is in before it moves.
    std::set<Creature *> entered;
    for (Creature *obstacle : m_obstacles) {
        plasma::Vector<3, Fixed> reach = plasma::Vector<3, Fixed>(creature->m_entityData.appearance.size * 0.5f);
        plasma::Vector<3, Fixed> obstacleReach =
            plasma::Vector<3, Fixed>(obstacle->m_entityData.appearance.size * 0.5f);
        if (creature->m_entityData.position[0] + reach[0] >= obstacle->m_entityData.position[0] - obstacleReach[0] &&
            creature->m_entityData.position[0] - reach[0] < obstacle->m_entityData.position[0] + obstacleReach[0] &&
            creature->m_entityData.position[1] + reach[1] >= obstacle->m_entityData.position[1] - obstacleReach[1] &&
            creature->m_entityData.position[1] - reach[1] < obstacle->m_entityData.position[1] + obstacleReach[1] &&
            creature->m_entityData.position[2] + reach[2] >= obstacle->m_entityData.position[2] - obstacleReach[2] &&
            creature->m_entityData.position[2] - reach[2] < obstacle->m_entityData.position[2] + obstacleReach[2])
            entered.insert(obstacle);
    }

    // Each step moves the three axes one after the other and takes an axis back when the creature's box then meets a
    // block or an obstacle creature. A horizontal step into a block one high is taken upwards instead (`walking`).
    plasma::Vector<3, Fixed> start = creature->m_entityData.position;
    bool onObstacle = false;
    bool passesGates = creature->findBuff(BuffSpiritworld) != nullptr;
    for (int step = 0; step < steps; step++) {
        for (int axis = 0; axis < 3; axis++) {
            plasma::Vector<3, Fixed> previous = creature->m_entityData.position;
            float distance = way[axis];
            creature->m_entityData.position[axis] += Fixed(distance);
            if (creature->isWraithTargeting())
                continue;

            // The blocks on the surface of the creature's box (a boat's box, ability 87 with toggle 13, is 0.8 wide).
            bool blocked = false;
            plasma::Vector<3, float> size = creature->m_entityData.appearance.size;
            if (creature->m_entityData.currentAbility == AbilitySit &&
                creature->m_entityData.binaryToggles.attachedToFurniture) {
                size[0] = 0.8f;
                size[1] = 0.8f;
            }
            plasma::Vector<3, float> half = size * 0.5f;
            plasma::Vector<3, int> low = fixedToBlock(creature->m_entityData.position - plasma::Vector<3, Fixed>(half));
            plasma::Vector<3, int> high =
                fixedToBlock(creature->m_entityData.position + plasma::Vector<3, Fixed>(half));
            for (int x = low[0]; x <= high[0]; x++) {
                for (int y = low[1]; y <= high[1]; y++) {
                    for (int z = low[2]; z <= high[2]; z++) {
                        if (x > low[0] && x < high[0] && y > low[1] && y < high[1] && z > low[2] && z < high[2])
                            continue;
                        bool held = !(creature->m_entityData.flags & CreatureFlagInsideBarrier);
                        if (blocksBody(getBlock(x, y, z), held, passesGates))
                            blocked = true;
                    }
                    if (blocked)
                        break;
                }
                if (blocked)
                    break;
            }

            if (!blocked || axis == 2) {
                // The obstacle creatures the box sweeps into on this axis, unless the creature already stood in them.
                plasma::Vector<3, Fixed> sweepLow = creature->m_entityData.position;
                plasma::Vector<3, Fixed> sweepHigh = previous;
                for (int i = 0; i < 3; i++) {
                    if (sweepHigh[i] < sweepLow[i])
                        std::swap(sweepLow[i], sweepHigh[i]);
                }
                bool stopped = false;
                for (Creature *obstacle : m_obstacles) {
                    // Race 251 lets creatures with buff 19 or AppearanceFlagSpiritForm through.
                    if (obstacle->m_entityData.race == RaceGrid &&
                        (creature->findBuff(BuffSpiritworld) ||
                         (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagSpiritForm)))
                        continue;
                    plasma::Vector<3, Fixed> center = obstacle->m_entityData.position;
                    plasma::Vector<3, float> obstacleSize = obstacle->m_entityData.appearance.size;
                    if (obstacle->m_entityData.race == RaceClimbingPlant) {
                        // Race 302 is taller by its attackRotation[2].
                        obstacleSize[2] += obstacle->m_entityData.attackRotation[2];
                        center[2] += Fixed(obstacle->m_entityData.attackRotation[2] * 0.5f);
                    }
                    if ((previous[axis] - center[axis]) * distance >= Fixed(0))
                        continue;
                    plasma::Vector<3, Fixed> reach =
                        plasma::Vector<3, Fixed>(creature->m_entityData.appearance.size * 0.5f);
                    plasma::Vector<3, Fixed> obstacleReach = plasma::Vector<3, Fixed>(obstacleSize * 0.5f);
                    if (sweepHigh[0] + reach[0] < center[0] - obstacleReach[0] ||
                        sweepLow[0] - reach[0] >= center[0] + obstacleReach[0] ||
                        sweepHigh[1] + reach[1] < center[1] - obstacleReach[1] ||
                        sweepLow[1] - reach[1] >= center[1] + obstacleReach[1] ||
                        sweepHigh[2] + reach[2] < center[2] - obstacleReach[2] ||
                        sweepLow[2] - reach[2] >= center[2] + obstacleReach[2])
                        continue;
                    if (entered.find(obstacle) == entered.end()) {
                        stopped = true;
                        break;
                    }
                }
                if (stopped) {
                    onObstacle = true;
                    walking = false;
                } else if (!blocked) {
                    continue;
                }
            }

            if (axis == 2) {
                // Landing (or the ceiling): the thud, and for the player the damage of a fall of more than 20 blocks
                // from the height it last rose at: half its health for every 20 blocks beyond the first 20.
                if ((!m_isClient || creature == m_localCreature) && creature->m_entityData.velocity[2] < -2.0f) {
                    ServerUpdates::SoundPacket sound;
                    sound.sound = SoundStep2;
                    sound.speed = (float)rand() * 0.2f / (float)RAND_MAX + 1.0f;
                    sound.position = creature->m_entityData.position;
                    sound.volume = 0.5f;
                    if (creature->m_entityData.hostilityType == HostilityPlayer)
                        sound.sourceId = creature->m_id;
                    m_serverUpdates.m_soundPacketList.push_back(sound);
                }
                // The fall height in raw units: formed before 20 blocks is converted.
                if (!(creature->m_entityData.flags & CreatureFlagInWater) &&
                    creature->m_entityData.velocity[2] < -5.0f &&
                    (Fixed(creature->m_fallStartHeight) - creature->m_entityData.position[2]).m_raw >
                        Fixed(20.0f).m_raw &&
                    creature == m_localCreature) {
                    Fixed fall = Fixed(creature->m_fallStartHeight) - creature->m_entityData.position[2];
                    Fixed excess = (fall / 20.0f) - Fixed(1);
                    float damage = (float)(excess * creature->maxHP() * 0.5f);
                    creature->m_entityData.hp -= damage;
                    creature->m_entityData.stunTime =
                        (std::max)(creature->m_entityData.stunTime, (float)creature->maxStunTime());
                    ServerUpdates::SoundPacket sound;
                    sound.position = creature->m_entityData.position;
                    sound.speed = 1.0f;
                    sound.sound = SoundSwordHit03;
                    m_serverUpdates.m_soundPacketList.push_back(sound);
                    if (creature->m_entityData.hp < 0.0f)
                        creature->m_entityData.hp = 0.0f;
                    ServerUpdates::HitPacket hit;
                    hit.attackerId = 0;
                    hit.targetId = creature->m_id;
                    hit.damage = damage;
                    hit.position = creature->m_entityData.position;
                    hit.critical = 0;
                    m_serverUpdates.m_hitPacketList.push_back(hit);
                }
                creature->m_entityData.position[2] -= Fixed(way[2]);
                creature->m_entityData.velocity[2] = 0.0f;
                creature->m_entityData.retreat[2] = 0.0f;
                creature->m_fallStartHeight = (float)creature->m_entityData.position[2];
                continue;
            }

            if (creature->m_entityData.binaryToggles.noTerrainCollision)
                continue;
            if (walking) {
                // Is the box free 1.01 blocks higher?
                plasma::Vector<3, Fixed> halfDots =
                    plasma::Vector<3, Fixed>(creature->m_entityData.appearance.size * 0.5f);
                plasma::Vector<3, int> raisedLow =
                    fixedToBlock(creature->m_entityData.position +
                                 plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, 1.01f)) - halfDots);
                plasma::Vector<3, int> raisedHigh =
                    fixedToBlock(creature->m_entityData.position +
                                 plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, 1.01f)) +
                                 plasma::Vector<3, Fixed>(creature->m_entityData.appearance.size * 0.5f));
                bool free = true;
                for (int x = raisedLow[0]; x <= raisedHigh[0] && free; x++) {
                    for (int y = raisedLow[1]; y <= raisedHigh[1] && free; y++) {
                        for (int z = raisedLow[2]; z <= raisedHigh[2]; z++) {
                            if (isBarrier(getBlock(x, y, z), passesGates)) {
                                free = false;
                                break;
                            }
                        }
                    }
                }
                if (free) {
                    // The feet go to 1.01 above the bottom of their block; m_stepOffset keeps the height gained, which
                    // the drawn model catches up with.
                    Fixed before = creature->m_entityData.position[2];
                    float halfHeight = creature->m_entityData.appearance.size[2] * 0.5f;
                    creature->m_entityData.position[2] =
                        Fixed(halfHeight) + Fixed((before - Fixed(halfHeight)).toBlock()) - Fixed(-1.01);
                    if (creature->m_entityData.currentAbility != AbilityRockFist ||
                        creature->m_entityData.timeSinceAbility > (float)creature->attackDuration())
                        creature->m_stepOffset =
                            (float)(creature->m_entityData.position[2] - (before - Fixed(creature->m_stepOffset)));
                    if (creature->m_stepOffset > 1.0f)
                        creature->m_stepOffset = 1.0f;
                    continue;
                }
            }
            creature->m_entityData.position[axis] -= Fixed(distance);
        }
    }

    // The flags are found anew from where the creature now is: CreatureFlagOnGround on the ground, 1 in water, 2 at a
    // wall, 4 not wholly under water, 5 held up, 6 in a gate block, 7 lava, 9 poison, 10 snow, 11 leaves, 13 a path
    // below; 8, 12 and 14 by the wall.
    bool wasInWater = (creature->m_entityData.flags & CreatureFlagInWater) != 0;
    bool wasInLava = (creature->m_entityData.flags & CreatureFlagInLava) != 0;
    bool wasInPoison = (creature->m_entityData.flags & CreatureFlagInPoison) != 0;
    bool wasAtWall = (creature->m_entityData.flags & CreatureFlagAtWall) != 0;
    bool wasOnGround = (creature->m_entityData.flags & CreatureFlagOnGround) != 0;
    plasma::Vector<3, float> wallNormal = creature->m_wallNormal;
    creature->m_entityData.flags = CreatureFlagNone;
    if (creature->m_entityData.stunTime <= 0.0f &&
        plasma::Vector<2, float>(creature->m_entityData.velocity[0], creature->m_entityData.velocity[1])
                .squaredLength() > 16.0f &&
        creature->m_entityData.acceleration.squaredLength() > 16.0f &&
        way.squaredLength() * 0.01f > plasma::Vector<3, float>(start - creature->m_entityData.position).squaredLength())
        creature->m_entityData.flags |= CreatureFlagMovementBlocked;

    plasma::Vector<3, int> low = fixedToBlock(creature->m_entityData.position -
                                              plasma::Vector<3, Fixed>(creature->m_entityData.appearance.size * 0.5f));
    plasma::Vector<3, int> high = fixedToBlock(creature->m_entityData.position +
                                               plasma::Vector<3, Fixed>(creature->m_entityData.appearance.size * 0.5f));
    bool wet = false;
    int surface = low[2];
    for (int x = low[0]; x <= high[0]; x++) {
        for (int y = low[1]; y <= high[1]; y++) {
            for (int z = low[2]; z <= high[2]; z++) {
                Block block = getBlock(x, y, z);
                if (block.type == BlockWater) {
                    creature->m_entityData.flags |= CreatureFlagInWater;
                    wet = true;
                    surface = std::max(z + 1, surface);
                }
                if (block.flags & BlockFlagBarrier)
                    creature->m_entityData.flags |= CreatureFlagInsideBarrier;
            }
        }
    }
    if (creature->m_entityData.flags & CreatureFlagInWater) {
        if (!(creature->m_entityData.appearance.appearanceFlags & AppearanceFlagNoTerrainCollision) &&
            creature->experiencesBuoyancy()) {
            // Buoyancy by the depth of the feet in half heights: boats and the obstacle types float higher.
            float depth = (float)(Fixed(surface) - creature->m_entityData.position[2]) /
                          (creature->m_entityData.appearance.size[2] * 0.5f);
            float lift = 0.0f;
            if (depth > 0.0f) {
                if (depth < 1.0f)
                    lift = depth + 1.0f;
                else
                    lift = 1.0f;
                if (creature->m_entityData.currentAbility == AbilitySailBoat ||
                    ((creature->m_entityData.hostilityType == HostilityAttackableObject ||
                      creature->m_entityData.hostilityType == HostilityInteractiveObject) &&
                     !(creature->m_entityData.appearance.appearanceFlags &
                       (AppearanceFlagFish | AppearanceFlagNoTerrainCollision))))
                    lift = depth * 1.5f + 1.0f;
            }
            creature->m_entityData.velocity[2] += seconds * Gravity * lift;
        }
        if (creature->m_entityData.flags & CreatureFlagInWater) {
            for (int x = low[0]; x <= high[0]; x++) {
                for (int y = low[1]; y <= high[1]; y++) {
                    for (int z = low[2]; z <= high[2]; z++) {
                        if (getBlock(x, y, z).type != BlockWater) {
                            creature->m_entityData.flags |= CreatureFlagPartlyAboveWater;
                            break;
                        }
                    }
                    if (creature->m_entityData.flags & CreatureFlagPartlyAboveWater)
                        break;
                }
                if (creature->m_entityData.flags & CreatureFlagPartlyAboveWater)
                    break;
            }
        }
    }

    // The friction of the ground is halved while the creature steers.
    float friction = 0.005f;
    if (creature->m_entityData.acceleration.squaredLength() > 0.0f)
        friction = 0.0025f;
    if (!(creature->m_entityData.flags & CreatureFlagInWater)) {
        // The ground: a barrier within 0.2 blocks below the box.
        int below =
            (creature->m_entityData.position[2] - Fixed(creature->m_entityData.appearance.size[2] * 0.5f) - Fixed(0.2f))
                .toBlock();
        for (int x = low[0]; x <= high[0]; x++) {
            for (int y = low[1]; y <= high[1]; y++) {
                if (isBarrier(getBlock(x, y, below), passesGates))
                    creature->m_entityData.flags |= CreatureFlagOnGround;
                if (getBlock(x, y, below).type == BlockWet)
                    wet = true;
            }
            if (creature->m_entityData.flags & CreatureFlagOnGround)
                break;
        }
    }
    // What the creature stands in or on, from the layer a block below the box; any ordinary block there cancels them.
    int under =
        (creature->m_entityData.position[2] - Fixed(creature->m_entityData.appearance.size[2] * 0.5f) - Fixed(1))
            .toBlock();
    bool ordinary = false;
    for (int x = low[0]; x <= high[0] && !ordinary; x++) {
        for (int y = low[1]; y <= high[1]; y++) {
            Block block = getBlock(x, y, under);
            if (block.type == BlockLava) {
                creature->m_entityData.flags |= CreatureFlagInLava;
            } else if (block.type == BlockPoison) {
                creature->m_entityData.flags |= CreatureFlagInPoison;
            } else if (block.type == BlockSnow || block.type == BlockSnowLeaves) {
                creature->m_entityData.flags |= CreatureFlagOnSnow;
            } else if (block.type == BlockLeaves || block.type == BlockType_12) {
                creature->m_entityData.flags |= CreatureFlagOnFoliage;
            } else if (block.type == BlockPath) {
                creature->m_entityData.flags |= CreatureFlagOnPath;
            } else if (block.type != BlockAir) {
                creature->m_entityData.flags &= ~(CreatureFlagInLava | CreatureFlagInPoison | CreatureFlagOnSnow |
                                                  CreatureFlagOnFoliage | CreatureFlagOnPath);
                ordinary = true;
                break;
            }
        }
    }
    if (creature->m_entityData.currentAbility == AbilityFloat)
        creature->m_entityData.flags &= ~(CreatureFlagInLava | CreatureFlagInPoison);
    if (wet && creature->m_entityData.hostilityType == HostilityPet) {
        // A pet in the wet refreshes its owner's m_petWaterLevel.
        Creature *owner = creatureByID(creature->m_entityData.ownerId);
        if (owner && owner->m_petWaterLevel < 1.0f) {
            owner->m_petWaterLevel += seconds * 0.05f;
            if (owner->m_petWaterLevel > 1.0f)
                owner->m_petWaterLevel = 1.0f;
        }
    }

    if (!onObstacle && wasOnGround &&
        (creature->m_entityData.currentAbility != AbilityRockFist ||
         creature->m_entityData.timeSinceAbility > (float)creature->attackDuration()) &&
        walking && way[0] * way[0] + way[1] * way[1] > 0.0f &&
        !(creature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater)) && way[2] <= 0.0f) {
        // Walking off a step of up to a block: the creature is put down on the ground below (m_stepOffset as for a
        // step up).
        int floor =
            (creature->m_entityData.position[2] - Fixed(creature->m_entityData.appearance.size[2] * 0.5f) - Fixed(1.2f))
                .toBlock();
        for (int x = low[0]; x <= high[0]; x++) {
            for (int y = low[1]; y <= high[1]; y++) {
                if (getBlock(x, y, floor).isSolid()) {
                    creature->m_entityData.flags |= CreatureFlagOnGround;
                    Fixed shown = creature->m_entityData.position[2] - Fixed(creature->m_stepOffset);
                    creature->m_entityData.position[2] =
                        Fixed(creature->m_entityData.appearance.size[2] * 0.5f) + (Fixed(1.01f) + Fixed(floor));
                    creature->m_stepOffset = (float)(creature->m_entityData.position[2] - shown);
                    break;
                }
            }
            if (creature->m_entityData.flags & CreatureFlagOnGround)
                break;
        }
    }

    if (creature->m_entityData.currentAbility == AbilitySailBoat &&
        (creature->m_entityData.flags & CreatureFlagInWater) &&
        creature->m_entityData.actionItem.category == ItemCategorySpecial &&
        creature->m_entityData.actionItem.id == SpecialItemBoat) {
        // A boat: little friction sideways, and the steering moves the `retreat` part of the speed, by the boat's
        // modifier.
        float share = blendFactor(milliseconds, friction * 0.1f);
        creature->m_entityData.velocity[0] += (0.0f - creature->m_entityData.velocity[0]) * share;
        creature->m_entityData.velocity[1] += (0.0f - creature->m_entityData.velocity[1]) * share;
        blend(creature->m_entityData.velocity[2], 0.0f, milliseconds, friction);
        plasma::Vector<3, float> drive = creature->m_entityData.acceleration;
        if (drive.squaredLength() > 144.0f)
            drive = drive.normalized() * 12.0f;
        blend(creature->m_entityData.retreat, drive * bonusFromLevel(creature->m_entityData.actionItem.modifier),
              milliseconds, 0.005f);
    }
    if (creature->m_entityData.actionLockTime == 0.0f) {
        if (creature->m_entityData.flags & CreatureFlagOnGround) {
            // Friction on the ground, except in the dashes and before the strike of the leaping attacks (7, 8, 11,
            // 18..21).
            if (creature->m_entityData.currentAbility != AbilityIntercept &&
                creature->m_entityData.currentAbility != AbilityDash &&
                (creature->m_entityData.currentAbility != AbilitySmash ||
                 creature->m_entityData.timeSinceAbility >
                     (float)(creature->anticipationTime() + creature->strikeTime())) &&
                ((creature->m_entityData.currentAbility != AbilityRightStab &&
                  creature->m_entityData.currentAbility != AbilityLeftStab &&
                  creature->m_entityData.currentAbility != AbilityBlockJab &&
                  creature->m_entityData.currentAbility != AbilityAmbush &&
                  creature->m_entityData.currentAbility != AbilityLeftDaggerPierce &&
                  creature->m_entityData.currentAbility != AbilityRightDaggerPierce &&
                  creature->m_entityData.currentAbility != AbilityKick) ||
                 !(creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime())))
                blend(creature->m_entityData.velocity, plasma::Vector<3, float>(0.0f, 0.0f, 0.0f), milliseconds,
                      friction);
        }
        if ((creature->m_entityData.flags & CreatureFlagOnGround) ||
            creature->m_entityData.currentAbility == AbilityHangGlide ||
            creature->m_entityData.currentAbility == AbilityCarryCreature)
            blend(creature->m_entityData.retreat, plasma::Vector<3, float>(0.0f, 0.0f, 0.0f), milliseconds, 0.005f);
    }
    if ((creature->m_entityData.flags & CreatureFlagInWater) &&
        creature->m_entityData.currentAbility != AbilitySailBoat)
        blend(creature->m_entityData.retreat, plasma::Vector<3, float>(0.0f, 0.0f, 0.0f), milliseconds, 0.005f);
    blend(creature->m_entityData.retreat, plasma::Vector<3, float>(0.0f, 0.0f, 0.0f), milliseconds, 0.00001f);

    // The walls at the four sides of the box, and the wall normal.
    plasma::Vector<3, float> heading(creature->m_entityData.acceleration[0], creature->m_entityData.acceleration[1],
                                     0.0f);
    if (heading.squaredLength() > 0.0f)
        heading.normalize();
    creature->m_wallNormal = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    float best = -2.0f;
    int wall = (creature->m_entityData.position[1] - wallReach(creature->m_entityData, 1)).toBlock();
    for (int x = low[0]; x <= high[0]; x++) {
        for (int z = low[2]; z <= high[2]; z++) {
            Block block = getBlock(x, wall, z);
            if (isBarrier(block, passesGates))
                touchWall(creature, block, z > low[2], heading, plasma::Vector<3, float>(0.0f, 1.0f, 0.0f), best);
        }
    }
    wall = (wallReach(creature->m_entityData, 1) + creature->m_entityData.position[1]).toBlock();
    for (int x = low[0]; x <= high[0]; x++) {
        for (int z = low[2]; z <= high[2]; z++) {
            Block block = getBlock(x, wall, z);
            if (isBarrier(block, passesGates))
                touchWall(creature, block, z > low[2], heading, plasma::Vector<3, float>(0.0f, -1.0f, 0.0f), best);
        }
    }
    wall = (creature->m_entityData.position[0] - wallReach(creature->m_entityData, 0)).toBlock();
    for (int y = low[1]; y <= high[1]; y++) {
        for (int z = low[2]; z <= high[2]; z++) {
            Block block = getBlock(wall, y, z);
            if (isBarrier(block, passesGates))
                touchWall(creature, block, z > low[2], heading, plasma::Vector<3, float>(1.0f, 0.0f, 0.0f), best);
        }
    }
    wall = (wallReach(creature->m_entityData, 0) + creature->m_entityData.position[0]).toBlock();
    for (int y = low[1]; y <= high[1]; y++) {
        for (int z = low[2]; z <= high[2]; z++) {
            Block block = getBlock(wall, y, z);
            if (isBarrier(block, passesGates))
                touchWall(creature, block, z > low[2], heading, plasma::Vector<3, float>(-1.0f, 0.0f, 0.0f), best);
        }
    }
    // The obstacle creatures of race 302 can be climbed over their height (attackRotation[2]) when it is more than a
    // block.
    for (Creature *obstacle : m_obstacles) {
        float reach =
            obstacle->m_entityData.appearance.size[0] * 0.5f + creature->m_entityData.appearance.size[0] * 0.5f + 0.1f;
        plasma::Vector<3, Fixed> offset = creature->m_entityData.position - obstacle->m_entityData.position;
        plasma::Vector<2, float> away((float)offset[0], (float)offset[1]);
        if (obstacle->m_entityData.race == RaceClimbingPlant && obstacle->m_entityData.attackRotation[2] > 1.0f &&
            reach > (std::max)((float)fabs(away[0]), (float)fabs(away[1])) &&
            creature->m_entityData.position[2] >= obstacle->m_entityData.position[2] &&
            creature->m_entityData.position[2] <
                Fixed(obstacle->m_entityData.attackRotation[2]) + obstacle->m_entityData.position[2]) {
            creature->m_entityData.flags |=
                (CreatureFlagAtWall | CreatureFlagAtSpecialWall | CreatureFlagWallAboveFeet);
            away.normalize();
            creature->m_wallNormal[0] = away[0];
            creature->m_wallNormal[1] = away[1];
        }
    }

    if (creature->m_entityData.flags & CreatureFlagInWater) {
        // In water.
        if (creature->m_entityData.currentAbility == AbilitySailBoat) {
            // A boat pitches by the water three blocks ahead and behind.
            float angle = (float)(creature->m_entityData.rotation[2] * Pi / 180.0 + HalfPi);
            plasma::Vector<3, Fixed> ahead =
                plasma::Vector<3, Fixed>(plasma::Vector<3, float>(cosf(angle) * 3.0f, sinf(angle) * 3.0f, 0.0f));
            int tilt = 0;
            if (getBlockAtDot(creature->m_entityData.position[0] + ahead[0],
                              creature->m_entityData.position[1] + ahead[1], creature->m_entityData.position[2])
                    .type == BlockWater)
                tilt++;
            if (getBlockAtDot(creature->m_entityData.position[0] + ahead[0],
                              creature->m_entityData.position[1] + ahead[1],
                              creature->m_entityData.position[2] - Fixed(1))
                    .type == BlockAir)
                tilt--;
            if (getBlockAtDot(creature->m_entityData.position[0] - ahead[0],
                              creature->m_entityData.position[1] - ahead[1], creature->m_entityData.position[2])
                    .type == BlockWater)
                tilt--;
            if (getBlockAtDot(creature->m_entityData.position[0] - ahead[0],
                              creature->m_entityData.position[1] - ahead[1],
                              creature->m_entityData.position[2] - Fixed(1))
                    .type == BlockAir)
                tilt++;
            if (tilt < 0) {
                float &pitch = creature->m_entityData.rotation[0];
                pitch += (-20.0f - pitch) * blendFactor(milliseconds, 0.003f);
            } else if (tilt > 0) {
                blend(creature->m_entityData.rotation[0], 20.0f, milliseconds, 0.003f);
            } else {
                blend(creature->m_entityData.rotation[0], 0.0f, milliseconds, 0.003f);
            }
        } else {
            blend(creature->m_entityData.velocity, plasma::Vector<3, float>(0.0f, 0.0f, 0.0f), milliseconds, 0.005f);
            if (creature->m_entityData.hostilityType != HostilityAttackableObject &&
                creature->m_entityData.hostilityType != HostilityInteractiveObject &&
                !(creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped)) {
                float &pitch = creature->m_entityData.rotation[0];
                pitch += (-60.0f - pitch) * blendFactor(milliseconds, 0.01f);
            }
        }
        if (!wasInWater && creature->m_entityData.velocity[2] < -3.0f) {
            // The splash of falling in.
            ServerUpdates::ParticlePacket splash;
            splash.position = creature->m_entityData.position;
            splash.size = 0.4f;
            splash.color = plasma::Vector<4, float>(0.2f, 0.7f, 1.0f, 1.0f);
            splash.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 10.0f);
            splash.count = 15;
            splash.type = ParticleWaterSplash;
            m_serverUpdates.m_particlePacketList.push_back(splash);
            if (creature == m_localCreature ||
                (!m_isClient && creature->m_entityData.hostilityType != HostilityPlayer)) {
                ServerUpdates::SoundPacket sound;
                sound.sound = SoundWaterSplash01;
                sound.speed = (float)rand() / (float)RAND_MAX + 0.9f;
                sound.position = creature->m_entityData.position;
                sound.volume = 1.0f;
                sound.sourceId = creature->m_id;
                m_serverUpdates.m_soundPacketList.push_back(sound);
            }
        }
        if (creature == m_localCreature && !creature->findBuff(BuffColdresistance) &&
            (!creature->findBuff(BuffFrozen) || creature->findBuff(BuffFrozen)->timeRemaining <= 9800.0f)) {
            // Cold water chills the player (buff 29, renewed every 200 ms) unless buff 27 protects it.
            AreaInfo info;
            fillAreaInfo((int)creature->m_entityData.position[0], (int)creature->m_entityData.position[1], info);
            BiomeSample sample;
            biomeSample((int)creature->m_entityData.position[0], (int)creature->m_entityData.position[1], sample);
            if (sample.temperature(info.type, info.influence) < 0.2f) {
                Creature::Buff buff;
                buff.timeRemaining = 10000.0f;
                buff.type = BuffFrozen;
                buff.sourceId = 0;
                buff.strength = 0.75f;
                creature->setBuff(buff, true);
                ServerUpdates::BuffPacket packet;
                packet.sourceId = creature->m_id;
                packet.targetId = creature->m_id;
                packet.buff = buff;
                m_serverUpdates.m_buffPacketList.push_back(packet);
            }
        }
    } else if (creature->m_entityData.flags & CreatureFlagInLava) {
        // In lava: a tenth of the player's health every 200 ms of the world's clock m_clock unless buff 26 protects
        // it.
        if (creature->m_entityData.hostilityType != HostilityAttackableObject &&
            creature->m_entityData.hostilityType != HostilityInteractiveObject &&
            !(creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped)) {
            float &pitch = creature->m_entityData.rotation[0];
            pitch += (-20.0f - pitch) * blendFactor(milliseconds, 0.01f);
        }
        if (!wasInLava && creature->m_entityData.velocity[2] < -3.0f) {
            ServerUpdates::ParticlePacket splash;
            splash.position = creature->m_entityData.position;
            splash.size = 0.4f;
            splash.color = plasma::Vector<4, float>(1.0f, 0.3f, 0.0f, 1.0f);
            splash.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 10.0f);
            splash.type = ParticleFire;
            splash.count = 15;
            m_serverUpdates.m_particlePacketList.push_back(splash);
            ServerUpdates::SoundPacket sound;
            sound.sound = SoundFireHit;
            sound.speed = (float)rand() / (float)RAND_MAX + 0.9f;
            sound.position = creature->m_entityData.position;
            sound.volume = 1.0f;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        if (creature == m_localCreature && creature->m_entityData.hp > 0.0f &&
            !creature->findBuff(BuffHeatresistance) && (int)(m_clock - milliseconds) / 200 != (int)m_clock / 200) {
            float damage = (float)(creature->maxHP() * 0.1);
            creature->m_entityData.hp -= damage;
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = 1.0f;
            sound.sound = SoundFireHit;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            if (creature->m_entityData.hp < 0.0f)
                creature->m_entityData.hp = 0.0f;
            ServerUpdates::HitPacket hit;
            hit.attackerId = 0;
            hit.targetId = creature->m_id;
            hit.damage = damage;
            hit.position = creature->m_entityData.position;
            hit.critical = 0;
            m_serverUpdates.m_hitPacketList.push_back(hit);
        }
    } else if (creature->m_entityData.flags & CreatureFlagInPoison) {
        // In poison: the player gets buff 4 (a hundredth of its health, three seconds) unless it has it or buff 28.
        if (creature->m_entityData.hostilityType != HostilityAttackableObject &&
            creature->m_entityData.hostilityType != HostilityInteractiveObject &&
            !(creature->m_entityData.appearance.appearanceFlags & AppearanceFlagQuadruped)) {
            float &pitch = creature->m_entityData.rotation[0];
            pitch += (-20.0f - pitch) * blendFactor(milliseconds, 0.01f);
        }
        if (!wasInPoison && creature->m_entityData.velocity[2] < -3.0f) {
            ServerUpdates::ParticlePacket splash;
            splash.position = creature->m_entityData.position;
            splash.size = 0.4f;
            splash.color = plasma::Vector<4, float>(0.5f, 0.4f, 0.3f, 1.0f);
            splash.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 10.0f);
            splash.count = 15;
            splash.type = ParticleLitCube;
            m_serverUpdates.m_particlePacketList.push_back(splash);
            ServerUpdates::SoundPacket sound;
            sound.sound = SoundWaterSplash01;
            sound.speed = (float)rand() / (float)RAND_MAX + 0.6f;
            sound.position = creature->m_entityData.position;
            sound.volume = 1.0f;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        if (creature == m_localCreature && !creature->findBuff(BuffPlagueresistance) &&
            !creature->findBuff(BuffPoisoned)) {
            Creature::Buff buff;
            buff.strength = 0.0f;
            buff.timeRemaining = 3000.0f;
            buff.type = BuffPoisoned;
            buff.sourceId = 0;
            buff.strength = creature->maxHP() * 0.01f;
            applyBuff(creature, buff, true);
        }
    } else if (!creature->isGliding() && creature->m_entityData.rollTime == 0.0f) {
        blend(creature->m_entityData.rotation[0], 0.0f, milliseconds, 0.01f);
    }

    // The player lets go of the wall it climbed (ability 122) when it no longer touches one: steering sends it off
    // twice as fast as the wall's normal.
    if (creature == m_localCreature && wallNormal.dot(heading) <= 0.0f &&
        creature->m_entityData.currentAbility == AbilityClimb && wasAtWall &&
        !(creature->m_entityData.flags & CreatureFlagAtWall) &&
        plasma::Vector<2, float>(creature->m_entityData.acceleration[0], creature->m_entityData.acceleration[1])
                .squaredLength() > 0.0f)
        creature->m_entityData.velocity = wallNormal * -2.0f;

    if ((creature->m_entityData.currentAbility == AbilityRockFist ||
         creature->m_entityData.currentAbility == AbilityHornCharge) &&
        creature->m_entityData.timeSinceAbility >= (float)creature->anticipationTime() &&
        creature->m_entityData.timeSinceAbility < (float)(creature->anticipationTime() + creature->strikeTime())) {
        // The rush of abilities 70 and 148: every 50 ms the ground under the feet throws particles of its colour (or
        // water) ahead.
        if (creature->m_entityData.currentAbility == AbilityRockFist) {
            creature->m_walkAmplitude = 0.0f;
            creature->m_walkPhase = 0.0f;
        }
        Fixed below =
            creature->m_entityData.position[2] - Fixed(creature->m_entityData.appearance.size[2] * 0.5f) - Fixed(0.5f);
        Block ground = getBlockAtDot(creature->m_entityData.position[0], creature->m_entityData.position[1], below);
        if (ground.type != BlockAir && (int)(milliseconds + creature->m_entityData.timeSinceAbility) / 50 !=
                                           (int)creature->m_entityData.timeSinceAbility / 50) {
            ServerUpdates::ParticlePacket dust;
            float ahead = creature->m_entityData.appearance.size[2] * 0.5f + 2.0f;
            plasma::Vector<3, float> feet(0.0f, 0.0f, 0.5f - creature->m_entityData.appearance.size[2] * 0.5f);
            dust.position = creature->m_entityData.position + plasma::Vector<3, Fixed>(feet) +
                            plasma::Vector<3, Fixed>(creature->m_entityData.velocity.normalized() * ahead);
            dust.size = 0.3f;
            dust.color[3] = 1.0f;
            if (ground.type == BlockWater) {
                dust.color[0] = 0.2f;
                dust.color[1] = 0.7f;
                dust.color[2] = 1.0f;
                dust.type = ParticleWaterSplash;
            } else {
                dust.color[0] = (float)ground.color[0] * (1.0f / 128.0f);
                dust.color[1] = (float)ground.color[1] * (1.0f / 128.0f);
                dust.color[2] = (float)ground.color[2] * (1.0f / 128.0f);
                dust.type = ParticleDebris;
            }
            dust.acceleration = plasma::Vector<3, float>((float)rand() * 4.0f / (float)RAND_MAX - 2.0f,
                                                         (float)rand() * 4.0f / (float)RAND_MAX - 2.0f, 2.0f) +
                                creature->m_entityData.velocity * 0.5f;
            dust.count = 8;
            m_serverUpdates.m_particlePacketList.push_back(dust);
            if ((!m_isClient && creature->m_entityData.hostilityType != HostilityPlayer) ||
                creature == m_localCreature) {
                ServerUpdates::SoundPacket sound;
                sound.position = creature->m_entityData.position;
                sound.speed = (float)rand() * 0.2f / (float)RAND_MAX + 0.8f;
                sound.sound = SoundBlade1;
                sound.sourceId = creature->m_id;
                m_serverUpdates.m_soundPacketList.push_back(sound);
            }
        }
        return;
    }

    // The walk cycle the animation reads: m_walkAmplitude its amplitude (risen by the time on the move, eased back to
    // 0) and m_walkPhase its phase, advanced by the way gone; every half cycle on the ground is a footstep.
    if (creature->m_entityData.currentAbility == AbilitySailBoat &&
        (creature->m_entityData.flags & CreatureFlagPartlyAboveWater) &&
        creature->m_entityData.velocity.squaredLength() > 16.0f) {
        // A boat under way: its wake every 100 ms.
        if ((int)(creature->m_entityData.timeSinceAbility - milliseconds) / 100 !=
            (int)creature->m_entityData.timeSinceAbility / 100) {
            ServerUpdates::ParticlePacket wake;
            plasma::Vector<3, float> behind(creature->m_entityData.velocity[0] * 0.1f,
                                            creature->m_entityData.velocity[1] * 0.1f, 0.0f);
            wake.position = creature->m_entityData.position - plasma::Vector<3, Fixed>(behind);
            wake.size = 0.25f;
            wake.color = plasma::Vector<4, float>(0.2f, 0.7f, 1.0f, 1.0f);
            wake.acceleration = plasma::Vector<3, float>(creature->m_entityData.velocity[0] * -0.25f,
                                                         creature->m_entityData.velocity[1] * -0.25f,
                                                         (float)rand() * 3.0f / (float)RAND_MAX + 2.0f);
            wake.count = rand() % 4 + 5;
            wake.type = ParticleWaterSplash;
            m_serverUpdates.m_particlePacketList.push_back(wake);
            ServerUpdates::SoundPacket sound;
            sound.sound = rand() % 3 + SoundStepWater;
            sound.speed = (float)rand() * 1.5f / (float)RAND_MAX + 0.6f;
            sound.position = creature->m_entityData.position;
            sound.volume = 0.75f;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
    } else if (creature->m_entityData.currentAbility == AbilityFloat ||
               (creature->m_entityData.currentAbility == Ability_125 &&
                creature->m_entityData.timeSinceAbility < 5000.0f) ||
               ((creature->m_entityData.currentAbility == AbilityFireExplosion ||
                 creature->m_entityData.currentAbility == AbilityHealingStream ||
                 creature->m_entityData.currentAbility == AbilityCenteredHealingStream ||
                 creature->m_entityData.currentAbility == AbilityWaterBubbles) &&
                creature->m_entityData.timeSinceAbility < (float)creature->attackDuration()) ||
               creature->m_entityData.currentAbility == AbilityDash) {
        // These abilities hold the cycle.
    } else if (creature->m_entityData.currentAbility == AbilityFlightToPoint &&
               creature->m_entityData.timeSinceAbility > 2000.0f) {
        // The flight to a flight point beats its wings.
        creature->m_walkPhase += seconds * 10.0f;
        creature->m_walkAmplitude += seconds;
    } else if (((creature->m_entityData.flags & CreatureFlagOnGround) ||
                (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagFlying) ||
                (creature->m_entityData.currentAbility == AbilityClimb &&
                 (creature->m_entityData.flags & CreatureFlagAtWall)) ||
                (creature->m_entityData.flags & CreatureFlagInWater)) &&
               creature->m_entityData.velocity.squaredLength() > 0.5f &&
               creature->m_entityData.currentAbility != AbilitySailBoat &&
               creature->m_entityData.currentAbility != AbilityEagleFlight &&
               creature->m_entityData.currentAbility != AbilityMenuView) {
        if (creature->m_entityData.currentAbility == AbilityClimb &&
            (creature->m_entityData.flags & CreatureFlagAtWall)) {
            // Climbing.
            float pace = sqrtf(creature->m_entityData.velocity.squaredLength()) * seconds * 6.0f;
            creature->m_walkPhase += pace * sqrtf(0.8f / creature->m_entityData.appearance.size[0]);
            creature->m_walkAmplitude += seconds * 0.2f;
        } else {
            float before = creature->m_walkPhase;
            float pace =
                sqrtf(plasma::Vector<2, float>(creature->m_entityData.velocity[0], creature->m_entityData.velocity[1])
                          .squaredLength());
            if (creature->m_entityData.flags & CreatureFlagOnGround) {
                // On the ground; the dashes and the leaping attacks before their strike barely move the legs, ability
                // 80 twice as fast.
                float advance;
                if (creature->m_entityData.currentAbility == AbilityIntercept ||
                    (creature->m_entityData.currentAbility == AbilitySmash &&
                     creature->m_entityData.timeSinceAbility < (float)creature->attackDuration()) ||
                    ((creature->m_entityData.currentAbility == AbilityRightStab ||
                      creature->m_entityData.currentAbility == AbilityLeftStab ||
                      creature->m_entityData.currentAbility == AbilityBlockJab ||
                      creature->m_entityData.currentAbility == AbilityAmbush ||
                      creature->m_entityData.currentAbility == AbilityLeftDaggerPierce ||
                      creature->m_entityData.currentAbility == AbilityRightDaggerPierce ||
                      creature->m_entityData.currentAbility == AbilityKick) &&
                     creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime())) {
                    advance = sqrtf(0.8f / creature->m_entityData.appearance.size[0]) *
                              (milliseconds * (0.001f * 0.1f) * pace * 1.5f);
                } else {
                    advance = sqrtf(0.8f / creature->m_entityData.appearance.size[0]) * (pace * seconds * 1.5f);
                    if (creature->m_entityData.currentAbility == AbilitySneak)
                        advance *= 2.0f;
                }
                creature->m_walkPhase += advance;
                creature->m_walkAmplitude += seconds;
                if ((!m_isClient || creature == m_localCreature) && creature->m_entityData.rollTime == 0.0f &&
                    !(creature->m_entityData.flags & CreatureFlagInWater) && pace > 5.0f &&
                    (int)(before / Pi) < (int)(creature->m_walkPhase / Pi)) {
                    // A footstep: splashing on wet ground.
                    ServerUpdates::SoundPacket sound;
                    Fixed below = creature->m_entityData.position[2] -
                                  Fixed(creature->m_entityData.appearance.size[2] * 0.5f) - Fixed(0.5f);
                    if (getBlockAtDot(creature->m_entityData.position[0], creature->m_entityData.position[1], below)
                            .type == BlockWet) {
                        sound.sound = rand() % 3 + 36;
                        sound.speed = 1.0f;
                    } else {
                        sound.sound = SoundStep2;
                        sound.speed = (float)rand() * 0.2f / (float)RAND_MAX + 1.0f;
                    }
                    sound.position = creature->m_entityData.position;
                    sound.volume = 1.0f;
                    if (creature->m_entityData.hostilityType == HostilityPlayer)
                        sound.sourceId = creature->m_id;
                    m_serverUpdates.m_soundPacketList.push_back(sound);
                }
            } else {
                // Swimming (or a creature with AppearanceFlagQuadruped in the air): three strokes in four at the
                // surface splash.
                creature->m_walkPhase =
                    sqrtf(0.8f / creature->m_entityData.appearance.size[0]) * (milliseconds * 0.002f * pace * 1.5f) +
                    before;
                creature->m_walkAmplitude += seconds;
                if (!m_isClient && (creature->m_entityData.flags & CreatureFlagPartlyAboveWater) &&
                    (int)(before / Pi) < (int)(creature->m_walkPhase / Pi) && rand() % 4 != 0) {
                    ServerUpdates::ParticlePacket wake;
                    plasma::Vector<3, float> course(creature->m_entityData.velocity[0],
                                                    creature->m_entityData.velocity[1], 0.0f);
                    wake.position = creature->m_entityData.position - plasma::Vector<3, Fixed>(0.1f * course);
                    wake.size = 0.25f;
                    wake.color = plasma::Vector<4, float>(0.2f, 0.7f, 1.0f, 1.0f);
                    wake.acceleration = plasma::Vector<3, float>(creature->m_entityData.velocity[0] * -0.25f,
                                                                 creature->m_entityData.velocity[1] * -0.25f,
                                                                 (float)rand() * 3.0f / (float)RAND_MAX + 2.0f);
                    wake.count = rand() % 4 + 5;
                    wake.type = ParticleWaterSplash;
                    m_serverUpdates.m_particlePacketList.push_back(wake);
                    ServerUpdates::SoundPacket sound;
                    sound.sound = rand() % 3 + 36;
                    sound.speed = (float)rand() * 1.5f / (float)RAND_MAX + 0.6f;
                    sound.position = creature->m_entityData.position;
                    sound.volume = 0.75f;
                    m_serverUpdates.m_soundPacketList.push_back(sound);
                }
            }
        }
    } else if (!(creature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater)) &&
               (creature->m_entityData.currentAbility != AbilityClimb ||
                !(creature->m_entityData.flags & CreatureFlagAtWall))) {
        // In the air.
        creature->m_walkPhase = 0.0f;
        creature->m_walkAmplitude += sqrtf(0.8f / creature->m_entityData.appearance.size[0]) * seconds;
    }

    // The time in the air, in milliseconds.
    if (!(creature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater)) &&
        (creature->m_entityData.currentAbility != AbilityClimb || !(creature->m_entityData.flags & CreatureFlagAtWall)))
        creature->m_airTime += milliseconds;
    else
        creature->m_airTime = 0.0f;
    if (creature->m_walkAmplitude > 1.0f)
        creature->m_walkAmplitude = 1.0f;
    blend(creature->m_walkAmplitude, 0.0f, milliseconds, 0.005f);
    if (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagPetrified) {
        // AppearanceFlagPetrified: a fixed phase by the creature's id.
        creature->m_walkPhase = (float)(hash((unsigned int)creature->m_id) % 10000);
        creature->m_walkAmplitude = 0.1f;
    }

    // The height the drawn model lags behind (after a step) eases to 0; lava and poison sink it by three quarters of a
    // block.
    float sunk = 0.0f;
    if (creature->m_entityData.flags & (CreatureFlagInLava | CreatureFlagInPoison))
        sunk = 0.75f;
    if (creature->m_entityData.currentAbility == AbilityRockFist &&
        creature->m_entityData.timeSinceAbility < (float)creature->attackDuration())
        creature->m_stepOffset = sunk;
    else if (creature->m_stepOffset < 0.0f)
        blend(creature->m_stepOffset, sunk, milliseconds, 0.01f);
    else
        blend(creature->m_stepOffset, sunk, milliseconds, 0.02f);

    // Where the creature looks.
    if (creature->m_entityData.currentAbility == AbilityClimb && (creature->m_entityData.flags & CreatureFlagAtWall)) {
        // Climbing: up, and at the wall.
        blend(creature->m_entityData.headRotation, 45.0f, milliseconds, 0.005f);
        plasma::Vector<3, float> into = creature->m_wallNormal * -1.0f;
        into.normalize();
        // The yaw that faces a unit direction: 0 towards +y, positive to the left.
        if (into[1] > 1.0f)
            into[1] = 1.0f;
        else if (into[1] < -1.0f)
            into[1] = -1.0f;
        if (into[0] > 0.0f)
            blend(creature->m_entityData.rotation[2],
                  interpolateAngle(creature->m_entityData.rotation[2], (float)(-acosf(into[1]) / Pi * 180.0), 1.0f),
                  milliseconds, 0.005f);
        else
            blend(creature->m_entityData.rotation[2],
                  interpolateAngle(creature->m_entityData.rotation[2], (float)(acosf(into[1]) / Pi * 180.0), 1.0f),
                  milliseconds, 0.005f);
        return;
    }
    creature->m_entityData.headRotation *= expf(milliseconds * -0.01f);
    if (creature->m_entityData.binaryToggles.faceAim && creature->m_entityData.stunTime <= 0.0f) {
        // Along the aim (m_drawnAttackRotation), the head by its slope.
        plasma::Vector<2, float> aim(creature->m_drawnAttackRotation[0], creature->m_drawnAttackRotation[1]);
        if (aim.squaredLength() > 0.0f && !keepsFacing) {
            aim.normalize();
            if (aim[1] > 1.0f)
                aim[1] = 1.0f;
            else if (aim[1] < -1.0f)
                aim[1] = -1.0f;
            if (aim[0] > 0.0f)
                blend(creature->m_entityData.rotation[2],
                      interpolateAngle(creature->m_entityData.rotation[2], (float)(-acosf(aim[1]) / Pi * 180.0), 1.0f),
                      milliseconds, 0.1f);
            else
                blend(creature->m_entityData.rotation[2],
                      interpolateAngle(creature->m_entityData.rotation[2], (float)(acosf(aim[1]) / Pi * 180.0), 1.0f),
                      milliseconds, 0.1f);
        }
        plasma::Vector<3, float> look = creature->m_drawnAttackRotation;
        if (look.squaredLength() > 0.0f) {
            look.normalize();
            blend(creature->m_entityData.headRotation,
                  interpolateAngle(creature->m_entityData.headRotation, (float)(asinf(look[2]) / Pi * 180.0), 1.0f),
                  milliseconds, 0.005f);
        }
    } else if (creature->m_entityData.rollTime == 0.0f) {
        // Along its course while it steers that way.
        plasma::Vector<2, float> course(creature->m_entityData.velocity[0], creature->m_entityData.velocity[1]);
        if (course.squaredLength() > 0.02f &&
            (creature->m_entityData.velocity & creature->m_entityData.acceleration) > 0.0f && !keepsFacing) {
            course.normalize();
            if (course[1] > 1.0f)
                course[1] = 1.0f;
            else if (course[1] < -1.0f)
                course[1] = -1.0f;
            if (course[0] > 0.0f)
                blend(
                    creature->m_entityData.rotation[2],
                    interpolateAngle(creature->m_entityData.rotation[2], (float)(-acosf(course[1]) / Pi * 180.0), 1.0f),
                    milliseconds, 0.1f);
            else
                blend(
                    creature->m_entityData.rotation[2],
                    interpolateAngle(creature->m_entityData.rotation[2], (float)(acosf(course[1]) / Pi * 180.0), 1.0f),
                    milliseconds, 0.1f);
        }
    }
}

// Rebuild the local region's mission updates.
void World::updateTasks()
{
    if (m_localCreature)
        Missions(this, m_localCreature->m_entityData.region).updateTasks(this);
}

// Instantiate nearby zone residents whose prerequisites are satisfied, and release the regional appearance flags
// selected by player toggles.
void World::updateTerritory(float milliseconds)
{
    if (m_isClient)
        return;
    std::set<Zone *> zones;
    std::list<Creature *> players;
    std::set<std::array<int, 2>> petrificationRemovedRegions, promotionSuppressedRegions;
    for (Creature *player : m_creatures) {
        if (player->m_entityData.hostilityType != HostilityPlayer)
            continue;
        players.push_back(player);
        for (int x =
                 (int)floorDivide((player->m_entityData.position[0] - Fixed(TerritoryRange)).toBlock(), BlocksPerZone);
             x <= (int)floorDivide((player->m_entityData.position[0] + Fixed(TerritoryRange)).toBlock(), BlocksPerZone);
             ++x) {
            for (int y = (int)floorDivide((player->m_entityData.position[1] - Fixed(TerritoryRange)).toBlock(),
                                          BlocksPerZone);
                 y <=
                 (int)floorDivide((player->m_entityData.position[1] + Fixed(TerritoryRange)).toBlock(), BlocksPerZone);
                 ++y) {
                Zone *zone = this->zone(x, y);
                if (zone)
                    zones.insert(zone);
            }
        }
        if (player->m_entityData.binaryToggles.removePetrification)
            petrificationRemovedRegions.insert(
                std::array<int, 2> {{player->m_entityData.region[0], player->m_entityData.region[1]}});
        if (player->m_entityData.binaryToggles.suppressResidentPromotion)
            promotionSuppressedRegions.insert(
                std::array<int, 2> {{player->m_entityData.region[0], player->m_entityData.region[1]}});
    }
    for (Zone *zone : zones) {
        zone->update();
        bool firstFlying = true;
        bool emitProjectile = false;
        for (const auto &entry : zone->m_residents) {
            Zone::Resident *resident = entry.second;
            if (creatureByID(resident->id)) {
                firstFlying = firstFlying && resident->currentAbility != AbilitySlowFall;
                continue;
            }
            if (m_state.m_deadCreatures.find(resident->id) != m_state.m_deadCreatures.end())
                continue;
            bool nearPlayer = false;
            for (Creature *player : players) {
                if (!distanceExceeds(resident->position, player->m_entityData.position, 256.0f)) {
                    nearPlayer = true;
                    break;
                }
            }
            if (!nearPlayer)
                continue;
            if (resident->requiredDefeatedCount) {
                int count = 0;
                for (int x = zone->m_position[0] - DefeatCountZoneRadius;
                     x <= zone->m_position[0] + DefeatCountZoneRadius; ++x) {
                    for (int y = zone->m_position[1] - DefeatCountZoneRadius;
                         y <= zone->m_position[1] + DefeatCountZoneRadius; ++y) {
                        Zone *neighbour = this->zone(x, y);
                        if (neighbour) {
                            count += neighbour->m_defeatedMissionCreatureCount;
                        } else {
                            std::array<int, 2> position = {{x, y}};
                            auto state = m_state.m_zoneStates.find(position);
                            if (state != m_state.m_zoneStates.end())
                                count += state->second.defeatedMissionCreatureCount;
                        }
                    }
                }
                if (count < resident->requiredDefeatedCount)
                    continue;
            }
            if (!resident->prerequisiteEntityCells.empty()) {
                bool freeCell = false;
                bool prerequisiteClear = true;
                for (const auto &cell : resident->prerequisiteEntityCells) {
                    Zone *cellZone = this->zone((int)floorDivide(cell[0] * BlocksPerCell, BlocksPerZone),
                                                (int)floorDivide(cell[1] * BlocksPerCell, BlocksPerZone));
                    if (!cellZone)
                        break;
                    bool occupied = false;
                    for (int index : cellZone->m_prerequisiteEntityIndices) {
                        if (index < 0 || (size_t)index >= cellZone->m_staticEntities.size())
                            continue;
                        const StaticEntity &entity = cellZone->m_staticEntities[index];
                        if (entity.active && floorDivide(entity.position[0].toBlock(), BlocksPerCell) == cell[0] &&
                            floorDivide(entity.position[1].toBlock(), BlocksPerCell) == cell[1]) {
                            occupied = true;
                            break;
                        }
                    }
                    if (occupied)
                        prerequisiteClear = false;
                    if (!occupied && prerequisiteClear) {
                        freeCell = true;
                        break;
                    }
                }
                if (!freeCell)
                    continue;
            }
            if (resident->requiredInteractionState) {
                bool found = false;
                for (Creature *creature : m_creatures) {
                    if (creature->m_entityData.classType == ClassArenaChampion &&
                        creature->m_entityData.interactionState == resident->requiredInteractionState) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    continue;
            }
            bool prerequisitesMet = true;
            for (long long id : resident->prerequisiteCreatureIds) {
                if (m_state.m_deadCreatures.find(id) == m_state.m_deadCreatures.end()) {
                    prerequisitesMet = false;
                    break;
                }
            }
            if (!prerequisitesMet)
                continue;
            Creature *creature = new Creature(resident->id);
            creature->m_entityData.origin = resident->position;
            creature->m_entityData.portCity =
                plasma::Vector<2, int>((int)floorDivide(resident->position[0].toBlock(), BlocksPerCell),
                                       (int)floorDivide(resident->position[1].toBlock(), BlocksPerCell));
            creature->m_entityData.position = resident->position;
            creature->m_entityData.rotation[2] = resident->angle;
            creature->m_entityData.hostilityType = (unsigned char)resident->hostility;
            creature->m_entityData.race = resident->race;
            creature->m_entityData.factionKind = (unsigned char)resident->factionKind;
            creature->m_entityData.factionVariant = resident->factionVariant;
            creature->m_entityData.factionSeed = resident->realm;
            creature->m_entityData.level = resident->level;
            creature->m_entityData.classType = resident->classType;
            creature->m_entityData.specialization = resident->specialization;
            creature->m_entityData.interactionState = resident->interactionState;
            creature->m_entityData.appearance = resident->appearance;
            memcpy(creature->m_entityData.name, resident->name, 16);
            creature->m_entityData.equipment = resident->equipment;
            creature->m_inventory = resident->inventory;
            creature->m_behaviours = resident->behaviours;
            creature->m_specialSkills = resident->specialSkills;
            creature->m_summonAppearanceMode = resident->summonAppearanceMode;
            creature->m_summonRaces = resident->summonRaces;
            creature->m_entityData.ownerId = resident->ownerId;
            creature->m_entityData.destination = resident->destination;
            creature->m_entityData.region = findRegion((int)resident->position[0], (int)resident->position[1]);
            creature->m_entityData.hp = creature->maxHP();
            creature->m_entityData.currentAbility = (unsigned char)resident->currentAbility;
            if (creature->m_entityData.currentAbility == AbilitySlowFall) {
                creature->m_entityData.position[2] += Fixed(rand() % 50 + 80); // 80..129 blocks up
                if (firstFlying)
                    emitProjectile = true;
                firstFlying = false;
            }
            creature->m_groupMembers = resident->groupMembers;
            if (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagPromotedResident) {
                std::array<int, 2> region = {{creature->m_entityData.region[0], creature->m_entityData.region[1]}};
                if (promotionSuppressedRegions.find(region) != promotionSuppressedRegions.end()) {
                    creature->m_entityData.appearance.appearanceFlags &= ~AppearanceFlagPromotedResident;
                } else {
                    creature->m_entityData.hostilityType = HostilityEnemy;
                    creature->m_entityData.appearance.size *= PromotedSizeFactor;
                    creature->m_entityData.appearance.appearanceFlags &= ~AppearanceFlagMarked;
                }
            }
            addCreature(creature);
        }
        if (emitProjectile) {
            float angle = (float)(((float)(hash(m_state.m_day) % 10000) / 10000.0f * 2.0f) * Pi);
            Fixed altitude = Fixed(height(zone->m_position[0] * BlocksPerZone + BlocksPerZone / 2,
                                          zone->m_position[1] * BlocksPerZone + BlocksPerZone / 2, 0) +
                                   400.0f);
            // Shifts signed world coordinates, including negatives.
            plasma::Vector<2, Fixed> center(Fixed(zone->m_position[0] * BlocksPerZone + BlocksPerZone / 2),
                                            Fixed(zone->m_position[1] * BlocksPerZone + BlocksPerZone / 2));
            Fixed startX = center[0] + cosf(angle) * 200.0f;
            Fixed startY = center[1] + sinf(angle) * 200.0f;
            plasma::Vector<3, Fixed> position(startX, startY, altitude);
            bool found = false;
            for (const auto &projectile : m_projectiles) {
                if (projectile.projectileType == ProjectileSteelEmpireAirship &&
                    !distanceExceeds2D(projectile.position, position, 200.0f)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                ServerUpdates::BulletPacket projectile;
                projectile.knockback = 0.0f;
                projectile.projectileType = ProjectileSteelEmpireAirship;
                projectile.timeToLive = 20000;
                projectile.velocity =
                    plasma::Vector<3, float>((float)(center[0] - position[0]), (float)(center[1] - position[1]), 0.0f)
                        .normalized() *
                    20.0f;
                projectile.currentTime = 0;
                projectile.sizeInBlocks = 80.0f;
                projectile.position = position;
                projectile.visualSize = 20.0f;
                m_projectiles.push_back(projectile);
                m_serverUpdates.m_bulletPacketList.push_back(projectile);
            }
        }
    }
    for (Creature *creature : m_creatures) {
        if (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagPetrified) {
            std::array<int, 2> region = {{creature->m_entityData.region[0], creature->m_entityData.region[1]}};
            if (petrificationRemovedRegions.find(region) != petrificationRemovedRegions.end())
                creature->m_entityData.appearance.appearanceFlags &= ~AppearanceFlagPetrified;
        }
    }
}

void World::setStaticActive(StaticEntity *entity, bool active)
{
    if (entity->active == active)
        return;
    entity->active = active;
    entity->transitionTime = 1000.0f - entity->transitionTime;
    if (entity->transitionTime < 0.0f)
        entity->transitionTime = 0.0f;
    int sound;
    switch (entity->type) {
    case StaticEntityGate:
    case StaticEntityLockedGate:
    case StaticEntityGoldenGate:
        sound = SoundGate;
        break;
    case StaticEntityFireTrap:
        if (!active)
            sound = SoundNone;
        else
            sound = SoundFireTrap;
        break;
    case StaticEntitySpikeTrap:
        if (!active)
            sound = SoundNone;
        else
            sound = SoundSpikeTrap;
        break;
    default:
        sound = SoundLever;
        break;
    }
    if (sound < SoundHit)
        return;
    ServerUpdates::SoundPacket packet;
    packet.position = entity->position;
    packet.sound = sound;
    m_serverUpdates.m_soundPacketList.push_back(packet);
}

// Processes queued interactions on the host: looting, picking up and dropping items, operating objects, recalling a
// pet and summoned effects.
void World::passInteractions(Creature *creature, float milliseconds)
{
    if (m_isClient)
        return;
    for (std::list<Interaction>::iterator it = creature->m_interactions.begin(); it != creature->m_interactions.end();
         ++it) {
        Interaction &interaction = *it;
        switch (interaction.type) {
        case InteractionActivate: {
            Creature *target = creatureByID(interaction.creatureId);
            if (!target)
                break;
            int race = target->m_entityData.race;
            if (race == RaceLockedGate || (target->m_entityData.appearance.appearanceFlags & AppearanceFlagUsable)) {
                target->m_entityData.interactionState = 1;
                if (race == RaceClimbingPlant) {
                    target->m_entityData.currentAbility = AbilityGrowClimbingPlant;
                    target->m_entityData.timeSinceAbility = 0.0f;
                }
            } else if (target->m_entityData.classType == ClassArenaChampion &&
                       target->m_entityData.interactionState < InteractionStateDone) {
                if (!(target->m_entityData.interactionState & 1)) {
                    unsigned char stage = ++target->m_entityData.interactionState;
                    std::cout << "Arena Master: " << (int)stage << std::endl;
                }
            } else if (target->m_entityData.binaryToggles.missionComplete && !target->m_entityData.interactionState) {
                target->m_entityData.interactionState = InteractionStateDone;
                target->m_entityData.binaryToggles.missionComplete = 0;
                if (m_state.m_droppedRewards.find(target->m_id) == m_state.m_droppedRewards.end()) {
                    m_state.m_droppedRewards.insert(target->m_id);
                    // Level + 1 rewards, at most four, up to a block away.
                    dropReward(target->m_entityData.position +
                                   plasma::Vector<3, Fixed>(
                                       plasma::Vector<3, float>(1.0f - (float)rand() * 2.0f / (float)RAND_MAX,
                                                                1.0f - (float)rand() * 2.0f / (float)RAND_MAX, 0.0f)),
                               (std::min)(4, target->m_entityData.level + 1));
                    for (std::vector<ItemStack> &tab : target->m_inventory.m_inventoryTabs) {
                        for (ItemStack &stack : tab) {
                            float y = 1.0f - (float)rand() * 2.0f / (float)RAND_MAX;
                            float x = 1.0f - (float)rand() * 2.0f / (float)RAND_MAX;
                            plasma::Vector<3, Fixed> position =
                                target->m_entityData.position +
                                plasma::Vector<3, Fixed>(plasma::Vector<3, float>(x, y, 0.0f));
                            dropItem(stack.item, position, (float)rand() * 360.0f / (float)RAND_MAX, 1.0f,
                                     stack.quantity, nullptr, 0);
                        }
                    }
                }
            }
            break;
        }
        case InteractionWhistle: {
            Creature *pet = creatureByID(creature->m_petId);
            if (pet) {
                pet->m_aggro.clear();
                plasma::Vector<3, Fixed> offset = pet->m_entityData.position - creature->m_entityData.position;
                float x = (float)offset[0];
                float y = (float)offset[1];
                float z = (float)offset[2];
                if (x * x + y * y + z * z > PetFollowDistanceSquared)
                    pet->m_entityData.position = creature->m_entityData.position;
            }
            creature->m_hitCounts.clear();
            creature->m_opponentId = 0;
            break;
        }
        case InteractionPickUp: {
            Zone *zone = this->zone(interaction.position[0], interaction.position[1]);
            int index = interaction.position[2];
            if (!zone || index < 0 || index >= (int)zone->m_items.size())
                break;
            ZoneItem &item = zone->m_items[index];
            plasma::Vector<3, Fixed> offset = item.position - creature->m_entityData.position;
            float x = (float)offset[0];
            float y = (float)offset[1];
            float z = (float)offset[2];
            if (!(x * x + y * y + z * z <= PickupRangeSquared) || !(item.item == interaction.item))
                break;
            ServerUpdates::SoundPacket sound;
            sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundPickup;
            if (item.item.category == ItemCategoryMaterial && item.item.id == MaterialItemDewdrop) {
                sound.sound = SoundWatersplashHit;
                sound.speed *= 2.0f;
            }
            sound.position = creature->m_entityData.position;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            ServerUpdates::ItemPickupPacket pickup;
            pickup.creatureId = creature->m_id;
            pickup.item = std::move(item.item);
            pickup.quantity = (unsigned short)item.quantity;
            pickup.position = item.position;
            pickup.flags = (signed char)item.flags;
            m_serverUpdates.m_itemPickupPacketList.push_back(pickup);
            if (!(item.flags & ZoneItemFlagPerPlayer)) {
                m_zonesWithChangedItems.insert(zone);
                item = ZoneItem();
            }
            break;
        }
        case InteractionTalk: {
            if (creature->m_entityData.currentAbility == AbilitySneak) {
                creature->m_entityData.currentAbility = AbilityNone;
                creature->m_entityData.stealth = 0.0f;
            }
            Creature *target = creatureByID(interaction.creatureId);
            if (target && target->m_entityData.classType == ClassInnkeeper &&
                (m_state.m_time >= InnNightStartHour * MillisecondsPerHour ||
                 m_state.m_time <= InnNightEndHour * MillisecondsPerHour)) {
                ServerUpdates::SoundPacket sound;
                sound.sound = SoundQuestComplete;
                sound.position = creature->m_entityData.position;
                sound.volume = 0.75f;
                m_serverUpdates.m_soundPacketList.push_back(sound);
                int event = WorldEventNewDay;
                m_serverUpdates.m_worldEventList.push_back(event);
                ++m_state.m_day;
                m_state.m_time = 7 * MillisecondsPerHour;
                break;
            }
            Zone *zone = this->zone(interaction.position[0], interaction.position[1]);
            int index = interaction.position[2];
            if (!zone || index < 0 || index >= (int)zone->m_staticEntities.size())
                break;
            StaticEntity &entity = zone->m_staticEntities[index];
            int type = entity.type;
            if ((type == StaticEntityStool || type == StaticEntityDruidChair ||
                 (unsigned)(type - StaticEntityBench) <= (StaticEntityBed - StaticEntityBench) ||
                 (unsigned)(type - StaticEntityBeachTowel) <= (StaticEntityWoodMat - StaticEntityBeachTowel)) &&
                !entity.occupantId &&
                (creature->m_entityData.currentAbility == AbilityNone ||
                 (unsigned char)(creature->m_entityData.currentAbility - AbilitySit) <= 1 ||
                 creature->m_entityData.currentAbility == AbilityRidePet ||
                 creature->m_entityData.timeSinceAbility > (float)creature->attackDuration())) {
                long long previous = entity.occupantId;
                entity.occupantId = creature->m_id;
                if (previous != creature->m_id)
                    m_zonesWithChangedObjects.insert(zone);
                creature->m_entityData.currentAbility = AbilityNone;
                creature->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            } else if (entity.item.category != ItemCategoryNone) {
                if (entity.active && interaction.item == entity.item) {
                    setStaticActive(&entity, false);
                    m_zonesWithChangedObjects.insert(zone);
                    std::map<int, Item>::iterator reward = zone->m_objectRewards.find(index);
                    if (reward != zone->m_objectRewards.end() && reward->second.category != ItemCategoryNone) {
                        float y = 1.0f - (float)rand() * 2.0f / (float)RAND_MAX;
                        float x = 1.0f - (float)rand() * 2.0f / (float)RAND_MAX;
                        plasma::Vector<3, Fixed> position =
                            entity.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(x, y, 0.0f));
                        dropItem(reward->second, position, (float)rand() * 360.0f / (float)RAND_MAX, 1.0f, 1, nullptr,
                                 0);
                    }
                    if ((unsigned)(entity.type - StaticEntityChestBase02) <= 1)
                        dropReward(entity.position, entity.item.rarity);
                }
            } else if (type == StaticEntityDoor || type == StaticEntityBigDoor || type == StaticEntityBigIronDoor ||
                       type == StaticEntityWindow || type == StaticEntityLever || type == StaticEntityChestBase02 ||
                       type == StaticEntityHatch ||
                       (unsigned)(type - StaticEntityCageBase) <= (StaticEntityCage4Base - StaticEntityCageBase)) {
                unsigned char previous = entity.active;
                setStaticActive(&entity, previous == 0);
                if (previous != entity.active)
                    m_zonesWithChangedObjects.insert(zone);
            }
            break;
        }
        case InteractionUseItem: {
            if (interaction.item.category != ItemCategoryConsumable || interaction.item.id != ConsumableBomb)
                break;
            long long id = LocalCreatureIdStart;
            for (Creature *other : m_creatures)
                if (other->m_id < id) {
                    id = other->m_id;
                }
            --id;
            Creature *spawn = new Creature(id);
            addCreature(spawn);
            spawn->m_entityData.position = creature->m_entityData.position;
            spawn->m_entityData.race = RaceBomb;
            spawn->m_entityData.hp = spawn->maxHP();
            spawn->m_entityData.hostilityType = HostilityAttackableObject;
            spawn->m_entityData.appearance.appearanceFlags |= AppearanceFlagPassive | AppearanceFlagNoTerrainCollision;
            fillAppearance(spawn->m_entityData.race, spawn->m_entityData.appearance, nullptr, 0);
            break;
        }
        case InteractionUseSpecialItem: {
            if (interaction.item.category != ItemCategorySpecial || interaction.item.id != SpecialItemHarp)
                break;
            for (std::list<Creature *>::iterator it = m_creatures.begin(); it != m_creatures.end(); ++it) {
                Creature *other = *it;
                plasma::Vector<3, Fixed> offset = other->m_entityData.position - creature->m_entityData.position;
                float x = (float)offset[0];
                float y = (float)offset[1];
                float z = (float)offset[2];
                // Within 20 blocks; written so a NaN distance passes.
                if (!(x * x + y * y + z * z > 400.0f) && other->m_entityData.race == RaceRuneGate

                    && !other->m_entityData.interactionState) {
                    other->m_entityData.interactionState = 1;
                    ServerUpdates::SoundPacket sound;
                    sound.sound = SoundHarp2;
                    sound.position = creature->m_entityData.position;
                    m_serverUpdates.m_soundPacketList.push_back(sound);
                }
            }
            int maxX = zoneCoordinate(creature->m_entityData.position[0] + Fixed(20));
            int minY = zoneCoordinate(creature->m_entityData.position[1] - Fixed(20));
            int maxY = zoneCoordinate(creature->m_entityData.position[1] + Fixed(20));
            int minX = zoneCoordinate(creature->m_entityData.position[0] - Fixed(20));
            for (int x = minX; x <= maxX; ++x) {
                for (int y = minY; y <= maxY; ++y) {
                    Zone *zone = this->zone(x, y);
                    if (!zone)
                        continue;
                    for (StaticEntity &entity : zone->m_staticEntities) {
                        if (entity.type != StaticEntityGoldenGate || !entity.active)
                            continue;
                        plasma::Vector<3, float> offset(entity.position - creature->m_entityData.position);
                        if (offset.squaredLength() < GoldenGateRangeSquared) {
                            unsigned char previous = entity.active;
                            setStaticActive(&entity, false);
                            if (previous != entity.active)
                                m_zonesWithChangedObjects.insert(zone);
                            ServerUpdates::SoundPacket sound;
                            sound.sound = SoundHarp2;
                            sound.position = creature->m_entityData.position;
                            m_serverUpdates.m_soundPacketList.push_back(sound);
                        }
                    }
                }
            }
            break;
        }
        case InteractionDrop: {
            float y = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
            float x = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
            plasma::Vector<3, Fixed> position =
                creature->m_entityData.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(x, y, 0.0f));
            dropItem(interaction.item, position, 0.0f, 1.0f, 1, creature, 0);
            ServerUpdates::SoundPacket sound;
            sound.sound = SoundDrop;
            sound.position = creature->m_entityData.position;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        }
        case InteractionShadowClone: {
            long long id = LocalCreatureIdStart;
            int count = 0;
            for (Creature *other : m_creatures) {
                if (other->m_entityData.ownerId == creature->m_id)
                    ++count;
                if (other->m_id < id) {
                    id = other->m_id;
                }
            }
            if (count > 2)
                break;
            --id;
            Creature *spawn = new Creature(id);
            spawn->m_entityData = std::move(creature->m_entityData);
            spawn->m_entityData.hostilityType =
                creature->m_entityData.hostilityType == HostilityPlayer ? HostilityNeutral : HostilityEnemy;
            spawn->m_behaviours = BehaviourFollowOwner | BehaviourMirrorOwner;
            spawn->m_entityData.appearance.appearanceFlags |= AppearanceFlagSummoned;
            spawn->m_entityData.currentAbility = AbilityNone;
            spawn->m_entityData.ownerId = creature->m_id;
            spawn->m_entityData.level = creature->m_entityData.level;
            Creature::Buff buff;
            buff.type = BuffShadowclone;
            buff.timeRemaining = (float)shelterDuration(creature->skillLevel(AbilityShadowClone));
            buff.strength = rampScale(creature->skillLevel(AbilityShadowClone));
            spawn->setBuff(buff, false);
            ServerUpdates::BuffPacket packet;
            packet.sourceId = spawn->m_id;
            packet.targetId = spawn->m_id;
            packet.buff = buff;
            packet.replace = 1;
            m_serverUpdates.m_buffPacketList.push_back(packet);
            addCreature(spawn);
            break;
        }
        case InteractionWaterBubbles: {
            long long id = LocalCreatureIdStart;
            for (Creature *other : m_creatures)
                if (other->m_id < id) {
                    id = other->m_id;
                }
            --id;
            Creature *spawn = new Creature(id);
            spawn->m_entityData.race = RaceWaterBubble;
            spawn->m_entityData.hostilityType = HostilityAttackableObject;
            spawn->m_entityData.position = creature->m_entityData.position;
            spawn->m_entityData.ownerId = creature->m_id;
            spawn->m_entityData.level = creature->m_entityData.level;
            fillAppearance(spawn->m_entityData.race, spawn->m_entityData.appearance, nullptr, 0);
            float power = (float)rand() * 0.5f / (float)RAND_MAX + 0.75f; // 75..125% (the size 75..150%)
            spawn->m_entityData.blockPower = creature->spellPower(true) * power;
            float scale = (float)rand() * 0.75f / (float)RAND_MAX + 0.75f;
            spawn->m_entityData.appearance.size *= scale;
            spawn->m_entityData.appearance.appearanceFlags |= AppearanceFlagSummoned;
            spawn->m_entityData.hp = 1.0f;
            spawn->m_entityData.mp = 1.0f;
            spawn->m_behaviours = BehaviourAttachToEnemy;
            ServerUpdates::SoundPacket sound;
            sound.sound = SoundPickup;
            sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 0.8f;
            sound.position = spawn->m_entityData.position;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            spawn->m_entityData.appearance.appearanceFlags |= AppearanceFlagSummoned;
            addCreature(spawn);
            break;
        }
        case InteractionHeroicShout: {
            ServerUpdates::SoundPacket sound;
            sound.sound = SoundHeroicShout;
            sound.position = creature->m_entityData.position;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            std::set<Creature *> targets;
            for (Creature *other : m_creatures) {
                if ((unsigned char)(other->m_entityData.hostilityType - HostilityAttackableObject) <= 1)
                    continue;
                if (creature->canDoDamage(other)) {
                    for (const std::pair<const long long, float> &entry : other->m_aggro) {
                        Creature *ally = creatureByID(entry.first);
                        if (!creature->canDoDamage(ally)) {
                            // Dereferences the looked-up creature without a null check.
                            plasma::Vector<3, Fixed> offset =
                                creature->m_entityData.position - ally->m_entityData.position;
                            float x = (float)offset[0];
                            float y = (float)offset[1];
                            float z = (float)offset[2];
                            if (x * x + y * y + z * z < HeroicShoutRangeSquared)
                                targets.insert(other);
                        }
                    }
                } else {
                    plasma::Vector<3, Fixed> offset = creature->m_entityData.position - other->m_entityData.position;
                    float x = (float)offset[0];
                    float y = (float)offset[1];
                    float z = (float)offset[2];
                    if (x * x + y * y + z * z < HeroicShoutRangeSquared) {
                        Creature::Buff buff;
                        buff.type = BuffRegeneration;
                        buff.sourceId = creature->m_id;
                        buff.timeRemaining = HeroicShoutRegenDuration;
                        buff.strength = HeroicShoutRegenStrength;
                        other->setBuff(buff, true);
                        ServerUpdates::BuffPacket packet;
                        packet.sourceId = creature->m_id;
                        packet.targetId = other->m_id;
                        packet.buff = buff;
                        packet.replace = 0;
                        m_serverUpdates.m_buffPacketList.push_back(packet);
                    }
                }
            }
            for (Creature *target : targets) {
                Creature::Buff buff;
                buff.strength = 0.0f;
                buff.type = BuffForcedAggroTarget;
                buff.sourceId = creature->m_id;
                buff.timeRemaining = 5000.0f;
                target->setBuff(buff, true);
                ServerUpdates::BuffPacket packet;
                packet.sourceId = creature->m_id;
                packet.targetId = target->m_id;
                packet.buff = buff;
                packet.replace = 0;
                m_serverUpdates.m_buffPacketList.push_back(packet);
            }
            break;
        }
        }
    }
    if (!m_isClient)
        creature->m_interactions.clear();
}

bool World::attackCreature(Creature *target, Creature *attacker, float damage, bool critical, bool powered,
                           float stunChance, const plasma::Vector<3, Fixed> &position,
                           const plasma::Vector<3, float> &direction, int element, int ability,
                           ServerUpdates::BulletPacket *projectile, bool allowBlocking)
{
    if (target->m_entityData.hp <= 0.0f || target->m_entityData.hostilityType == HostilityInteractiveObject ||
        target->m_entityData.race == RaceWraith)
        return false;
    if (m_isClient && (target == m_localCreature || attacker != m_localCreature))
        return false;
    if (attacker && attacker == m_localCreature && speciesType(target->m_entityData.race) == SpeciesDemon)
        damage *= 1.0f + attacker->demonDamageBonus();
    float originalDamage = damage;
    if (attacker) {
        if ((attacker->m_entityData.currentAbility == AbilityQuake ||
             attacker->m_entityData.currentAbility == AbilityStomp) &&
            !(target->m_entityData.flags & CreatureFlagOnGround))
            return false;
        if (attacker->m_entityData.currentAbility == AbilitySupportRay) {
            ServerUpdates::ParticlePacket particle;
            particle.position = target->m_entityData.position;
            particle.size = (float)rand() * 0.1f / (float)RAND_MAX + 0.1f;
            particle.type = ParticleFire;
            particle.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 10.0f);
            particle.count = 8;
            particle.color = plasma::Vector<4, float>(0.6f, 0.6f, 1.0f, 1.0f);
            m_serverUpdates.m_particlePacketList.push_back(particle);
        }
        if (ability != AbilityAssassinate &&
            (target->m_entityData.binaryToggles.dodge || (target->m_entityData.flags & CreatureFlagMovementBlocked) ||
             (target->m_entityData.currentAbility == Ability_125 && target->m_entityData.timeSinceAbility < 5000.0f))) {
            ServerUpdates::SoundPacket sound;
            sound.position = target->m_entityData.position;
            sound.speed = (float)rand() * 0.3f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundBlock;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            ServerUpdates::HitPacket hit;
            hit.position = target->m_entityData.position;
            hit.attackerId = attacker->m_id;
            hit.targetId = target->m_id;
            hit.damage = damage;
            hit.direction = direction;
            hit.hitType = HitTypeMiss;
            m_serverUpdates.m_hitPacketList.push_back(hit);
            if (!m_isClient)
                applyHit(hit);
            return false;
        }
        if (allowBlocking &&
            ((target->isBlocking() && target->m_entityData.blockPower > 0.0f) ||
             ((target->m_entityData.currentAbility == AbilityShellBlock ||
               target->m_entityData.currentAbility == AbilityShellSpin ||
               target->m_entityData.currentAbility == AbilityRestHeal) &&
              target->m_entityData.timeSinceAbility >= (float)target->anticipationTime(AbilityUseCurrent) &&
              target->m_entityData.timeSinceAbility <
                  (float)(target->anticipationTime(AbilityUseCurrent) + target->strikeTime(AbilityUseCurrent))))) {
            float manaCost = damage / target->blockPower();
            if (target->m_entityData.currentAbility == AbilityShellBlock ||
                target->m_entityData.currentAbility == AbilityShellSpin ||
                target->m_entityData.currentAbility == AbilityRestHeal)
                manaCost = 0.0f;
            if (target->m_entityData.currentAbility == AbilityCyclone)
                manaCost *= 0.5f;
            std::cout << "mana cost " << manaCost << std::endl;
            if (!m_isClient) {
                float gain = target->manaGeneration() * (std::max)(0.0f, (1.0f - manaCost) * 0.5f);
                if (target->isBlocking()) {
                    target->m_entityData.mp += gain;
                    if (target->m_entityData.currentAbility != AbilityCyclone)
                        target->m_entityData.charge += gain;
                }
                if (target->m_entityData.mp > 1.0f)
                    target->m_entityData.mp = 1.0f;
                if (target->m_entityData.charge > target->m_entityData.mp)
                    target->m_entityData.charge = target->m_entityData.mp;
            }
            ServerUpdates::HitPacket hit;
            hit.position = target->m_entityData.position;
            hit.attackerId = attacker->m_id;
            hit.targetId = target->m_id;
            hit.damage = damage;
            hit.direction = direction * 0.5f;
            hit.hitType = HitTypeHeal;
            target->m_entityData.blockPower -= manaCost;
            if (target->m_entityData.blockPower < -1.0f)
                target->m_entityData.blockPower = -1.0f;
            m_serverUpdates.m_hitPacketList.push_back(hit);
            ServerUpdates::SoundPacket sound;
            sound.position = target->m_entityData.position;
            sound.speed = (float)rand() * 0.3f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundBlock;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            if (!m_isClient)
                applyHit(hit);
            return false;
        }
        float absorption = 0.0f;
        for (const Creature::Buff &buff : target->m_buffs) {
            if (buff.type == BuffManashield)
                absorption += buff.strength;
        }
        float combo = (float)attacker->m_entityData.hitCombo / (float)attacker->maxCombo();
        float armorPiercing = (std::min)(1.0f, combo);
        if (absorption > 0.0f) {
            ServerUpdates::HitPacket hit;
            hit.position = target->m_entityData.position;
            hit.attackerId = attacker->m_id;
            hit.targetId = target->m_id;
            hit.damage = damage;
            hit.direction = direction * 0.5f;
            hit.hitType = HitTypeMana;
            m_serverUpdates.m_hitPacketList.push_back(hit);
            ServerUpdates::SoundPacket sound;
            sound.position = target->m_entityData.position;
            sound.speed = (float)rand() * 0.3f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundAbsorb;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            if (!m_isClient)
                applyHit(hit);
            damage -= absorption;
            if (damage <= 0.0f)
                return true;
        }
        float protection = element != ElementPhysical ? target->resistance() : target->armor();
        damage = std::max(0.0f, damage - protection * (1.0f - armorPiercing));
        if (element == ElementLeech && damage > 0.0f) {
            float healing = damage * LeechFraction;
            ServerUpdates::HitPacket hit;
            hit.position = attacker->m_entityData.position;
            hit.attackerId = attacker->m_id;
            hit.targetId = attacker->m_id;
            hit.damage = -healing;
            m_serverUpdates.m_hitPacketList.push_back(hit);
            attacker->m_entityData.hp += healing;
        }
        combo = (float)attacker->m_entityData.hitCombo / (float)attacker->maxCombo();
        if (combo > 0.25f) {
            plasma::Vector<3, float> impulse = direction;
            if (impulse.squaredLength() > 0.0f)
                impulse *= 1.0f / sqrtf(impulse.squaredLength());
            impulse[0] *= 4.0f;
            impulse[1] *= 4.0f;
            impulse[2] = 5.0f;
            std::list<Item *> weapons = attacker->attackWeapons();
            for (Item *weapon : weapons)
                createSpiritParticles(target->m_entityData.position, impulse, combo, powered, *weapon);
        }
        if (powered && combo > 0.0f) {
            std::list<Item *> weapons = attacker->attackWeapons();
            if (!weapons.empty()) {
                float healing = 0.0f;
                float strength = 0.1f / (float)weapons.size();
                for (Item *weapon : weapons) {
                    for (int i = 0; i < weapon->spiritCount; ++i) {
                        switch (weapon->spirits[i].material) {
                        case ItemMaterialFireSpirit:
                            damage += strength * combo;
                            break;
                        case ItemMaterialUnholy: {
                            float bonus = strength * combo;
                            healing += bonus + bonus;
                            damage += bonus * 0.5f;
                            break;
                        }
                        case ItemMaterialIceSpirit:
                            damage += strength * 0.25f * combo;
                            target->m_entityData.slowedTime += combo * 500.0f;
                            break;
                        case ItemMaterialWind:
                            damage += strength * 0.25f * combo;
                            attacker->m_entityData.sprintTime += combo * 500.0f;
                            break;
                        }
                    }
                }
                if (target->m_entityData.slowedTime > 5000.0f)
                    target->m_entityData.slowedTime = 5000.0f;
                if (attacker->m_entityData.sprintTime > 5000.0f)
                    attacker->m_entityData.sprintTime = 5000.0f;
                if (healing > 0.0f) {
                    ServerUpdates::HitPacket hit;
                    hit.position = attacker->m_entityData.position;
                    hit.attackerId = attacker->m_id;
                    hit.targetId = attacker->m_id;
                    hit.damage = -healing;
                    m_serverUpdates.m_hitPacketList.push_back(hit);
                    attacker->m_entityData.hp += healing;
                }
            }
        }
        if (attacker->findBuff(BuffWarfrenzy)) {
            float healing = damage * 0.5f;
            ServerUpdates::HitPacket hit;
            hit.position = attacker->m_entityData.position;
            hit.attackerId = attacker->m_id;
            hit.targetId = attacker->m_id;
            hit.damage = -healing;
            m_serverUpdates.m_hitPacketList.push_back(hit);
            attacker->m_entityData.hp += healing;
            attacker->m_entityData.hp = (std::min)(attacker->m_entityData.hp, attacker->maxHP());
        }
        if (attacker->m_entityData.currentAbility == AbilitySupportRay) {
            if (target->m_hitFlash < 0.3f)
                target->m_hitFlash = 0.3f;
        } else if (allowBlocking) {
            target->m_hitFlash = 1.0f;
        }
        std::list<Item *> weapons = attacker->attackWeapons();
        if (element != ElementPhysical) {
            ServerUpdates::SoundPacket sound;
            sound.position = target->m_entityData.position;
            if (element == ElementWater) {
                sound.speed = 1.0f;
                sound.sound = SoundWatersplashHit;
            } else {
                sound.speed = 1.5f;
                sound.sound = SoundFireHit;
            }
            m_serverUpdates.m_soundPacketList.push_back(sound);
        } else if (ability != AbilitySupportRay) {
            ServerUpdates::SoundPacket sound;
            sound.position = target->m_entityData.position;
            int weaponType = weapons.empty() ? -1 : weapons.front()->id;
            if (attacker->m_entityData.currentAbility == AbilityHeadbutt)
                weaponType = WeaponGreatmace;
            sound.speed = (float)rand() * 0.2f / (float)RAND_MAX + 0.9f;
            int soundId = SoundPunch2;
            if (critical) {
                switch (weaponType) {
                case WeaponSword:
                case WeaponAxe:
                    sound.speed += 0.1f;
                case WeaponGreatsword:
                case WeaponGreataxe:
                    soundId = SoundBlade2;
                    break;
                case WeaponDagger:
                case WeaponFist:
                    break;
                case WeaponLongsword:
                    soundId = SoundLongBlade2;
                    break;
                case WeaponBow:
                case WeaponCrossbow:
                case WeaponBoomerang:
                    soundId = SoundHitArrowCritical;
                    break;
                default:
                    soundId = SoundHit2;
                    break;
                }
            } else {
                switch (weaponType) {
                case WeaponDagger:
                case WeaponFist:
                    soundId = SoundPunch1;
                    break;
                case WeaponSword:
                case WeaponAxe:
                    sound.speed += 0.1f;
                case WeaponGreatsword:
                case WeaponGreataxe:
                    soundId = SoundBlade1;
                    break;
                case WeaponLongsword:
                    soundId = SoundLongBlade1;
                    break;
                case WeaponBow:
                case WeaponCrossbow:
                case WeaponBoomerang:
                    soundId = SoundHitArrow;
                    break;
                default:
                    soundId = SoundHit1;
                    break;
                }
            }
            attacker->hasTwoHandedWeapon();
            if (attacker->m_entityData.currentAbility == AbilitySmash)
                sound.sound = SoundSmash1;
            else
                sound.sound = attacker->m_entityData.currentAbility == AbilityBlockJab ? SoundHit1 : soundId;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
    } else {
        rand();
        target->m_hitFlash = 0.5f;
    }
    float speedSquared = target->m_entityData.velocity.squaredLength();
    if (direction.squaredLength() > speedSquared)
        speedSquared = direction.squaredLength();
    if (target->m_entityData.velocity.squaredLength() > speedSquared) {
        plasma::Vector<3, float> velocity = target->m_entityData.velocity;
        velocity.normalize();
        target->m_entityData.velocity = velocity * sqrtf(speedSquared);
    }
    ServerUpdates::HitPacket hit;
    hit.position = target->m_entityData.position;
    hit.attackerId = attacker ? attacker->m_id : 0;
    for (const Creature::Buff &buff : target->m_buffs) {
        if (buff.type == BuffBulwark)
            damage *= buff.strength;
    }
    hit.targetId = target->m_id;
    hit.damage = damage;
    hit.critical = critical;
    hit.stunTime = 0;
    hit.direction = direction;
    hit.powered = powered;
    if (ability == AbilityAmbush) {
        Creature::Buff buff;
        buff.type = BuffPoisoned;
        buff.timeRemaining = AmbushPoisonDuration;
        buff.sourceId = attacker->m_id;
        buff.strength = originalDamage * AmbushPoisonStrength;
        target->m_buffs.push_back(buff);
        ServerUpdates::BuffPacket packet;
        packet.sourceId = attacker->m_id;
        packet.targetId = target->m_id;
        packet.buff = buff;
        m_serverUpdates.m_buffPacketList.push_back(packet);
    }
    if (hit.critical && target->m_entityData.hostilityType != HostilityAttackableObject) {
        ServerUpdates::SoundPacket sound;
        sound.position = target->m_entityData.position;
        sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 1.0f;
        switch (target->m_entityData.race) {
        case RaceElfMale:
            sound.sound = SoundMaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceHumanMale:
        case RaceWizard:
        case RaceNomadMale:
        case RaceSorcerer:
        case RacePirateMale:
        case RaceDarkCultistMale:
        case RaceNecromancer:
        case RaceEmpireSoldierMale:
            sound.sound = SoundMaleGroan65;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceDwarfMale:
            sound.sound = SoundDwarfMaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceGoblinMale:
            sound.sound = SoundGoblinMaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceUndeadMale:
            sound.sound = SoundUndeadMaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceLizardmanMale:
            sound.sound = SoundLizardMaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceHumanFemale:
        case RaceWitch:
        case RaceNomadFemale:
        case RacePirateFemale:
        case RaceDarkCultistFemale:
        case RaceEmpireSoldierFemale:
            sound.sound = SoundFemaleGroan66;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceElfFemale:
            sound.sound = SoundFemaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceDwarfFemale:
            sound.sound = SoundDwarfFemaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceGoblinFemale:
            sound.sound = SoundGoblinFemaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceUndeadFemale:
            sound.sound = SoundUndeadFemaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceLizardmanFemale:
            sound.sound = SoundLizardFemaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceFrogmanMale:
            sound.sound = SoundFrogmanMaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceFrogmanFemale:
            sound.sound = SoundFrogmanFemaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceOgre:
        case RaceTroll:
        case RaceDarkTroll:
        case RaceYeti:
        case RaceCyclops:
        case RaceEnhancedPirateMale:
            sound.sound = SoundTrollGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceGnoll:
        case RaceGnobold:
        case RaceMole:
            sound.sound = SoundMoleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceGreenSlime:
        case RacePinkSlime:
        case RaceYellowSlime:
        case RaceBlueSlime:
            sound.sound = SoundSlimeGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceZombie:
            sound.sound = SoundZombieGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceSaurian:
            sound.sound = SoundMonsterGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceOrcMale:
            sound.sound = SoundOrcMaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceOrcFemale:
            sound.sound = SoundOrcFemaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceEnhancedOrcMale:
            sound.sound = SoundOrcMaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        case RaceEnhancedOrcFemale:
            sound.sound = SoundOrcFemaleGroan;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            break;
        }
    }
    if (attacker && target->m_entityData.stunTime < -StunImmunityTime &&
        target->m_entityData.hostilityType != HostilityAttackableObject) {
        float chance = stunChance;
        switch (attacker->m_entityData.currentAbility) {
        case AbilityCrossedWeaponSweep:
        case AbilityTwoHandedThrust:
        case AbilityHeadbutt:
        case AbilityHornCharge:
        case AbilityLunge:
            chance = 1.0f;
            break;
        }
        if (target->isLarge())
            chance *= 0.15f;
        if (!attacker->isLarge() && attacker->m_entityData.hostilityType != HostilityPlayer)
            chance *= 0.1f;
        if (attacker->m_entityData.currentAbility == AbilityQuake ||
            attacker->m_entityData.currentAbility == AbilityStomp)
            chance = 1.0f;
        if (target->m_entityData.currentAbility == AbilitySleep)
            chance = 1.0f;
        chance *= 0.9f;
        for (const Creature::Buff &buff : target->m_buffs) {
            if (buff.type == BuffBulwark)
                chance = 0.0f;
        }
        if (chance > (float)rand() / (float)RAND_MAX) {
            hit.stunTime = target->maxStunTime();
            hit.direction[2] = 10.0f;
            ServerUpdates::SoundPacket sound;
            sound.position = target->m_entityData.position;
            sound.speed = 1.0f;
            sound.sound = SoundSwordHit03;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
    }
    if (ability == AbilityRockFist)
        hit.direction[2] += RockFistLift / target->m_entityData.appearance.size[0];
    if (attacker && attacker->m_entityData.currentAbility == AbilitySupportRay)
        hit.supportRayLight = 1;
    m_serverUpdates.m_hitPacketList.push_back(hit);
    if (!m_isClient)
        applyHit(hit);
    return true;
}

// Recounts the nearby zones' completed groups, advances dropped items and static entities, and attaches creatures to
// the static entities they use.
void World::updateZonesState(float milliseconds)
{
    // Nearby zones, deduplicated before updating their residents and items.
    std::set<Zone *> zones;
    std::set<Zone *> unusedZones;
    std::list<Creature *> players;
    float nearestDistance = TreasureSpiritRangeSquared;
    plasma::Vector<3, float> nearest(0.0f, 0.0f, 0.0f);
    if (m_localCreature) {
        if (m_localCreature->hasSpecialItem(SpecialItemTreasureSpirit)) {
            Creature *player = m_localCreature;
            int firstX = zoneCoordinate(player->m_entityData.position[0] - Fixed(400));
            int lastX = zoneCoordinate(player->m_entityData.position[0] + Fixed(400));
            int firstY = zoneCoordinate(player->m_entityData.position[1] - Fixed(400));
            int lastY = zoneCoordinate(player->m_entityData.position[1] + Fixed(400));
            for (int x = firstX; x <= lastX; ++x) {
                for (int y = firstY; y <= lastY; ++y) {
                    Zone *zone = this->zone(x, y);
                    if (!zone)
                        continue;
                    for (plasma::Vector<3, Fixed> &position : zone->m_treasurePositions) {
                        std::array<int, 2> cell = {{floorDivide(position[0].toBlock(), BlocksPerCell),
                                                    floorDivide(position[1].toBlock(), BlocksPerCell)}};
                        if (player->m_playerData->discoveredMarkers.find(cell) ==
                            player->m_playerData->discoveredMarkers.end()) {
                            plasma::Vector<3, float> offset(position - player->m_entityData.position);
                            offset[2] += 2.0f;
                            float distance = offset.squaredLength();
                            if (nearestDistance > distance) {
                                nearestDistance = distance;
                                nearest = offset;
                            }
                        }
                    }
                }
            }
            float distance = nearest[0] * nearest[0] + nearest[1] * nearest[1] + nearest[2] * nearest[2];
            if (distance > 0.0f) {
                nearest *= 1.0f / sqrtf(distance);
                float reach = rayDistance(player->m_entityData.position, nearest, 4.0f, false, true, nullptr) - 0.5f;
                float clock = m_clock;
                plasma::Vector<3, float> target = nearest * reach;
                target[0] += cosf(clock * 0.001f) * 0.5f;
                target[1] += sinf(clock * 0.001f) * 0.5f;
                target[2] += sinf(clock * 0.003f) * 0.5f;
                // The marker drifts towards the target, in place.
                plasma::Vector<3, float> &marker = m_localCreature->m_playerData->treasureSpiritOffset;
                marker += (target - marker) * blendFactor(milliseconds, 0.005f);
            } else {
                player->m_playerData->treasureSpiritOffset = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            }
        } else {
            m_localCreature->m_playerData->treasureSpiritOffset = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        }
    }
    for (std::list<Creature *>::iterator entry = m_creatures.begin(); entry != m_creatures.end(); ++entry) {
        Creature *creature = *entry;
        creature->m_entityData.binaryToggles.active = 0;
        creature->m_entityData.binaryToggles.attachedToFurniture = 0;
        if (creature->m_entityData.hostilityType == HostilityPlayer) {
            players.push_back(creature);
            int centerX = floorDivide(creature->m_entityData.position[0].toBlock(), BlocksPerZone);
            int centerY = floorDivide(creature->m_entityData.position[1].toBlock(), BlocksPerZone);
            for (int x = centerX - ActiveZoneRadius; x <= centerX + ActiveZoneRadius; ++x) {
                for (int y = centerY - ActiveZoneRadius; y <= centerY + ActiveZoneRadius; ++y) {
                    Zone *zone = this->zone(x, y);
                    if (zone)
                        zones.insert(zone);
                }
            }
        }
        if (creature->m_entityData.classType == ClassArenaChampion &&
            creature->m_entityData.interactionState != InteractionStateDone) {
            int firstX =
                floorDivide((creature->m_entityData.origin[0] - Fixed(ArenaChampionReach)).toBlock(), BlocksPerZone);
            int lastX =
                floorDivide((creature->m_entityData.origin[0] + Fixed(ArenaChampionReach)).toBlock(), BlocksPerZone);
            int firstY =
                floorDivide((creature->m_entityData.origin[1] - Fixed(ArenaChampionReach)).toBlock(), BlocksPerZone);
            int lastY =
                floorDivide((creature->m_entityData.origin[1] + Fixed(ArenaChampionReach)).toBlock(), BlocksPerZone);
            for (int x = firstX; x <= lastX; ++x) {
                for (int y = firstY; y <= lastY; ++y) {
                    Zone *zone = this->zone(x, y);
                    if (!zone)
                        continue;
                    std::map<unsigned char, bool> completed;
                    int lastStage = 0;
                    for (std::pair<const long long, Zone::Resident *> resident : zone->m_residents) {
                        if (resident.second->requiredInteractionState &&
                            m_state.m_deadCreatures.find(resident.first) != m_state.m_deadCreatures.end()) {
                            unsigned char stage = (unsigned char)resident.second->requiredInteractionState;
                            completed[stage] = true;
                        }
                        lastStage = (std::max)(lastStage, resident.second->requiredInteractionState);
                    }
                    for (std::pair<const long long, Zone::Resident *> resident : zone->m_residents) {
                        if (resident.second->requiredInteractionState &&
                            m_state.m_deadCreatures.find(resident.first) == m_state.m_deadCreatures.end()) {
                            unsigned char stage = (unsigned char)resident.second->requiredInteractionState;
                            completed[stage] = false;
                        }
                    }
                    for (std::pair<const unsigned char, bool> &stage : completed) {
                        if (stage.second)
                            creature->m_entityData.interactionState = (unsigned char)(std::max)(
                                (int)creature->m_entityData.interactionState, (int)stage.first + 1);
                    }
                    if (lastStage && creature->m_entityData.interactionState > lastStage)
                        creature->m_entityData.interactionState = InteractionStateDone;
                }
            }
        }
    }
    for (Zone *zone : zones) {
        zone->m_completedActivators.clear();
        for (auto &resident : zone->m_residents) {
            if (resident.second->activatorCells.size() != 0 &&
                m_state.m_deadCreatures.find(resident.first) != m_state.m_deadCreatures.end()) {
                for (std::set<std::array<int, 2>>::iterator cell = resident.second->activatorCells.begin();
                     cell != resident.second->activatorCells.end(); ++cell)
                    ++zone->m_completedActivators[*cell];
            }
        }
        zone->m_defeatedMissionCreatureCount = 0;
        for (auto &id : zone->m_trackedMissionCreatureIds) {
            if (m_state.m_deadCreatures.find(id) != m_state.m_deadCreatures.end())
                ++zone->m_defeatedMissionCreatureCount;
        }
    }
    for (Zone *zone : zones) {
        if (!m_isClient) {
            for (auto &id : zone->m_missionCreatures) {
                Creature *creature = creatureByID(id);
                if (creature && creature->m_entityData.hostilityType != HostilityInteractiveObject)
                    creature->m_entityData.binaryToggles.missionComplete = 1;
            }
        }
        for (std::vector<ZoneItem>::iterator item = zone->m_items.begin(); item != zone->m_items.end(); ++item) {
            if (item->item.category == ItemCategoryPainting) {
                if (getBlockAtDot(item->position[0], item->position[1] - Fixed(1), item->position[2]).isSolid()) {
                    item->position[1] = Fixed((int)item->position[1]) + 0.2f;
                    item->rotation = 180.0f;
                }
                if (getBlockAtDot(item->position[0], item->position[1] + Fixed(1), item->position[2]).isSolid()) {
                    item->position[1] = Fixed((int)item->position[1]) + Fixed(1) - 0.2f;
                    item->rotation = 0.0f;
                }
                if (getBlockAtDot(item->position[0] - Fixed(1), item->position[1], item->position[2]).isSolid()) {
                    item->position[0] = Fixed((int)item->position[0]) + 0.2f;
                    item->rotation = 90.0f;
                }
                if (getBlockAtDot(item->position[0] + Fixed(1), item->position[1], item->position[2]).isSolid()) {
                    item->position[0] = Fixed((int)item->position[0]) + Fixed(1) - 0.2f;
                    item->rotation = 270.0f;
                }
            }
            if (item->hiddenTimeRemaining > 0) {
                int remaining = (int)(item->hiddenTimeRemaining - milliseconds);
                item->hiddenTimeRemaining = std::max(0, remaining);
            }
            if (item->dropTime > 0.0f) {
                item->dropTime -= milliseconds;
                if (item->dropTime <= 0.0f) {
                    item->dropTime = 0.0f;
                    if (!m_isClient) {
                        ServerUpdates::SoundPacket sound;
                        sound.position = item->position;
                        sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 1.0f;
                        sound.sound = (item->item.category != ItemCategoryCoin) + SoundDropCoin;
                        m_serverUpdates.m_soundPacketList.push_back(sound);
                    }
                    if (m_localCreature &&
                        ((unsigned char)(item->item.category - ItemCategoryWeapon) <=
                             ItemCategoryRing - ItemCategoryWeapon ||
                         item->item.category == ItemCategoryEyePatch) &&
                        item->ownerSeed == m_localCreature->m_entityData.seed) {
                        ServerUpdates::SoundPacket sound;
                        sound.position = item->position;
                        sound.sound = SoundCraftProc;
                        m_serverUpdates.m_soundPacketList.push_back(sound);
                    }
                }
            }
        }
        for (auto &entity : zone->m_staticEntities) {
            float elapsed = entity.transitionTime;
            elapsed += milliseconds;
            entity.transitionTime = elapsed;
            if (m_localCreature && (entity.type == StaticEntityRewardChest || entity.type == StaticEntityGoddess2)) {
                std::array<int, 2> cell = {{tileCoordinate(entity.position[0]), tileCoordinate(entity.position[1])}};
                bool active = m_localCreature->m_playerData->discoveredMarkers.find(cell) ==
                              m_localCreature->m_playerData->discoveredMarkers.end();
                if (entity.active != (unsigned char)active) {
                    entity.active = active;
                    entity.transitionTime = 0;
                    elapsed = 0.0f;
                }
            }
            if (!m_isClient && entity.type == StaticEntityMagicBarrier && entity.active) {
                for (std::list<Creature *>::iterator player = players.begin(); player != players.end(); ++player) {
                    if ((*player)->m_entityData.binaryToggles.magicBarriersDisabled) {
                        plasma::Vector<2, int> region = findRegion(entity.position);
                        if (region == (*player)->m_entityData.region) {
                            entity.active = 0;
                            m_zonesWithChangedObjects.insert(zone);
                        }
                    }
                }
            }
            if (entity.occupantId) {
                Creature *occupant = creatureByID(entity.occupantId);
                // A restraint (cage, cauldron, rack, irons, pole, chain) holds its occupant while active; other
                // furniture takes a creature that is not stunned, not busy and keeps to the terrain.
                if (occupant && (((entity.type >= StaticEntityCageBase && entity.type <= StaticEntityCeilingChain) &&
                                  entity.active) ||
                                 (!(entity.type >= StaticEntityCageBase && entity.type <= StaticEntityCeilingChain) &&
                                  occupant->m_entityData.stunTime <= 0.0f &&
                                  (occupant->m_entityData.currentAbility == AbilityNone ||
                                   occupant->m_entityData.currentAbility == AbilityRidePet ||
                                   occupant->m_entityData.timeSinceAbility > occupant->attackDuration()) &&
                                  !occupant->m_entityData.binaryToggles.noTerrainCollision))) {
                    switch (entity.type) {
                    case StaticEntityStool:
                    case StaticEntityBench:
                    case StaticEntityBenchBlack:
                    case StaticEntityDruidChair: {
                        occupant->m_entityData.position =
                            entity.position +
                            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                0.0f, 0.0f, occupant->m_entityData.appearance.size[2] * 0.5f + entity.scaledSize()[2]));
                        occupant->m_drawnPosition = occupant->m_entityData.position;
                        occupant->m_entityData.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                        occupant->m_entityData.rotation[2] = (float)(90 * (entity.orientation + 2));
                        occupant->m_entityData.currentAbility = AbilitySit;
                        occupant->m_entityData.binaryToggles.attachedToFurniture = 1;
                        break;
                    }
                    case StaticEntityBed:
                    case StaticEntityBeachTowel:
                    case StaticEntityWoodMat: {
                        occupant->m_entityData.position =
                            entity.position +
                            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                0.0f, 0.0f, occupant->m_entityData.appearance.size[2] * 0.5f + entity.scaledSize()[2]));
                        occupant->m_drawnPosition = occupant->m_entityData.position;
                        occupant->m_entityData.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                        occupant->m_entityData.rotation[2] = (float)(90 * entity.orientation);
                        occupant->m_entityData.currentAbility = AbilitySleep;
                        occupant->m_entityData.binaryToggles.attachedToFurniture = 1;
                        break;
                    }
                    case StaticEntityCageBase:
                    case StaticEntityCage2Base:
                    case StaticEntityCage3Base:
                    case StaticEntityCage4Base:
                        occupant->m_entityData.position =
                            entity.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                  0.0f, 0.0f, occupant->m_entityData.appearance.size[2] * 0.5f));
                        occupant->m_drawnPosition = occupant->m_entityData.position;
                        occupant->m_entityData.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                        occupant->m_entityData.rotation[2] = (float)(90 * (entity.orientation + 2));
                        occupant->m_entityData.currentAbility = AbilitySit;
                        occupant->m_entityData.binaryToggles.active = 1;
                        break;
                    case StaticEntityCauldron: {
                        Creature::EntityData &data = occupant->m_entityData;
                        data.position =
                            entity.position +
                            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                0.0f, 0.0f, entity.scaledSize()[2] * 0.8f + data.appearance.size[2] * 0.5f));
                        occupant->m_drawnPosition = data.position;
                        data.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                        data.rotation[2] = (float)(90 * (entity.orientation + 2));
                        data.currentAbility = AbilitySit;
                        data.binaryToggles.active = 1;
                        break;
                    }
                    case StaticEntityLegIrons:
                        occupant->m_entityData.position =
                            entity.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                                  0.0f, 0.0f, occupant->m_entityData.appearance.size[2] * 0.5f));
                        occupant->m_drawnPosition = occupant->m_entityData.position;
                        occupant->m_entityData.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                        occupant->m_entityData.rotation[2] = (float)(90 * entity.orientation);
                        occupant->m_entityData.currentAbility = AbilitySit;
                        occupant->m_entityData.binaryToggles.active = 1;
                        break;
                    case StaticEntityPole: {
                        occupant->m_entityData.position =
                            entity.position +
                            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                0.0f, 0.0f,
                                occupant->m_entityData.appearance.size[2] * 0.5f + entity.scaledSize()[2] * 0.25f));
                        occupant->m_drawnPosition = occupant->m_entityData.position;
                        occupant->m_entityData.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                        occupant->m_entityData.rotation[2] = (float)(90 * entity.orientation);
                        occupant->m_entityData.currentAbility = AbilityPolePose;
                        occupant->m_entityData.binaryToggles.active = 1;
                        break;
                    }
                    case StaticEntityTortureRack: {
                        occupant->m_entityData.position =
                            entity.position +
                            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(
                                0.0f, 0.0f,
                                occupant->m_entityData.appearance.size[2] * 0.5f + entity.scaledSize()[2] * 0.9f));
                        occupant->m_drawnPosition = occupant->m_entityData.position;
                        occupant->m_entityData.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                        occupant->m_entityData.rotation[2] = (float)(90 * entity.orientation);
                        occupant->m_entityData.currentAbility = AbilityTortureRackPose;
                        occupant->m_entityData.timeSinceAbility = 0.0f;
                        occupant->m_entityData.binaryToggles.active = 1;
                        break;
                    }
                    }
                    if (!(entity.type >= StaticEntityCageBase && entity.type <= StaticEntityCeilingChain)) {
                        plasma::Vector<3, float> offset(occupant->m_entityData.position - entity.position);
                        if ((offset.squaredLength() > FurnitureLeaveDistanceSquared ||
                             occupant->m_entityData.acceleration.squaredLength() > 0.0f ||
                             (unsigned char)(occupant->m_entityData.currentAbility - AbilitySit) > 2) &&
                            !m_isClient) {
                            occupant->m_entityData.binaryToggles.active = 0;
                            occupant->m_entityData.binaryToggles.attachedToFurniture = 0;
                            occupant->m_entityData.currentAbility = AbilityNone;
                            if (entity.occupantId) {
                                entity.occupantId = 0;
                                m_zonesWithChangedObjects.insert(zone);
                            }
                        }
                    }
                    if (occupant->m_entityData.binaryToggles.active)
                        occupant->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                } else if (!m_isClient &&
                           (!(entity.type >= StaticEntityCageBase && entity.type <= StaticEntityCeilingChain) ||
                            !entity.active)) {
                    if (occupant) {
                        occupant->m_entityData.binaryToggles.active = 0;
                        occupant->m_entityData.binaryToggles.attachedToFurniture = 0;
                        occupant->m_entityData.currentAbility = AbilityNone;
                    }
                    if (entity.occupantId) {
                        entity.occupantId = 0;
                        m_zonesWithChangedObjects.insert(zone);
                    }
                }
            }
            if (m_isClient)
                continue;
            if ((entity.type == StaticEntityGate || entity.type == StaticEntityRuneGate) && entity.active) {
                unsigned char wasActive = entity.active;
                std::array<int, 2> cell = {{floorDivide(entity.position[0].toBlock(), BlocksPerCell),
                                            floorDivide(entity.position[1].toBlock(), BlocksPerCell)}};
                std::map<std::array<int, 2>, unsigned int>::iterator found = zone->m_requiredActivators.find(cell);
                if (found != zone->m_requiredActivators.end()) {
                    int required = found->second;
                    int completed = 0;
                    int firstX = zoneCoordinate(entity.position[0] - Fixed(BlocksPerPlace));
                    int firstY = zoneCoordinate(entity.position[1] - Fixed(BlocksPerPlace));
                    int lastX = zoneCoordinate(entity.position[0] + Fixed(BlocksPerPlace));
                    int lastY = zoneCoordinate(entity.position[1] + Fixed(BlocksPerPlace));
                    for (int x = firstX; x <= lastX; ++x) {
                        for (int y = firstY; y <= lastY; ++y) {
                            Zone *member = this->zone(x, y);
                            if (member) {
                                std::map<std::array<int, 2>, unsigned int>::iterator count =
                                    member->m_completedActivators.find(cell);
                                if (count != member->m_completedActivators.end())
                                    completed += count->second;
                            } else {
                                std::array<int, 2> key = {{x, y}};
                                std::map<std::array<int, 2>, ZoneState>::iterator saved =
                                    m_state.m_zoneStates.find(key);
                                if (saved != m_state.m_zoneStates.end()) {
                                    std::map<std::array<int, 2>, unsigned int>::iterator count =
                                        saved->second.completedActivators.find(cell);
                                    if (count != saved->second.completedActivators.end())
                                        completed += count->second;
                                }
                            }
                        }
                    }
                    if (completed >= required) {
                        setStaticActive(&entity, false);
                        if (wasActive != entity.active)
                            m_zonesWithChangedObjects.insert(zone);
                    } else {
                        unsigned char depth =
                            structureCell(floorDivide(entity.position[0].toBlock(), BlocksPerCell),
                                          floorDivide(entity.position[1].toBlock(), BlocksPerCell), true)
                                .side;
                        for (std::list<Creature *>::iterator player = players.begin(); player != players.end();
                             ++player) {
                            Creature *creature = *player;
                            plasma::Vector<3, float> offset(creature->m_entityData.position - entity.position);
                            if (offset.squaredLength() < ActivatorRangeSquared) {
                                if (getBlockAtDot(creature->m_entityData.position[0],
                                                  creature->m_entityData.position[1],
                                                  creature->m_entityData.position[2])
                                        .flags &
                                    BlockFlagDungeon) {
                                    int x = floorDivide(creature->m_entityData.position[0].toBlock(), BlocksPerCell);
                                    int y = floorDivide(creature->m_entityData.position[1].toBlock(), BlocksPerCell);
                                    if (structureCell(x, y, true).side > depth) {
                                        unsigned char before = entity.active;
                                        setStaticActive(&entity, false);
                                        if (before != entity.active)
                                            m_zonesWithChangedObjects.insert(zone);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if ((entity.type == StaticEntityCauldron || entity.type == StaticEntityPole) && entity.active) {
                unsigned char before = entity.active;
                int firstX = zone->m_position[0] - 2;
                int firstY = zone->m_position[1] - 2;
                int lastX = zone->m_position[0] + 2;
                int lastY = zone->m_position[1] + 2;
                std::array<int, 2> cell = {{tileCoordinate(entity.position[0]), tileCoordinate(entity.position[1])}};
                int completed = 0;
                int residents = 0;
                for (int x = firstX; x <= lastX; ++x) {
                    for (int y = firstY; y <= lastY; ++y) {
                        Zone *member = this->zone(x, y);
                        if (member) {
                            for (std::pair<const long long, Zone::Resident *> &resident : member->m_residents) {
                                if (resident.second->activatorCells.find(cell) !=
                                    resident.second->activatorCells.end()) {
                                    ++residents;
                                    if (m_state.m_deadCreatures.find(resident.first) != m_state.m_deadCreatures.end())
                                        ++completed;
                                }
                            }
                        } else
                            residents = UnloadedResidentCount;
                    }
                }
                if (completed > 0 && completed == residents) {
                    setStaticActive(&entity, false);
                    if (before != entity.active)
                        m_zonesWithChangedObjects.insert(zone);
                }
            }
            if (entity.type == StaticEntityFireTrap || entity.type == StaticEntitySpikeTrap ||
                entity.type == StaticEntityStompTrap) {
                elapsed = entity.transitionTime;
                if (elapsed > TrapResetTime) {
                    unsigned char before = entity.active;
                    if (before) {
                        setStaticActive(&entity, false);
                        if (before != entity.active)
                            m_zonesWithChangedObjects.insert(zone);
                    } else {
                        for (Creature *creature : m_creatures) {
                            if (creature->m_entityData.hostilityType == HostilityPlayer) {
                                Fixed dx = entity.position[0] - creature->m_entityData.position[0];
                                Fixed dy = entity.position[1] - creature->m_entityData.position[1];
                                Fixed dz = entity.position[2] - creature->m_entityData.position[2];
                                if (dz * dz + dy * dy + dx * dx < Fixed(TrapTriggerRangeSquared)) {
                                    setStaticActive(&entity, true);
                                    if (entity.active)
                                        m_zonesWithChangedObjects.insert(zone);
                                    break;
                                }
                            }
                        }
                    }
                }
                elapsed = entity.transitionTime;
                if (entity.active && ((entity.type <= StaticEntitySpikeTrap &&
                                       (int)m_clock / 200 != (int)(m_clock + milliseconds) / 200) ||
                                      (entity.type == StaticEntityStompTrap && elapsed == 0.0f))) {
                    for (Creature *creature : m_creatures) {
                        Fixed dx = entity.position[0] - creature->m_entityData.position[0];
                        Fixed dy = entity.position[1] - creature->m_entityData.position[1];
                        Fixed dz = entity.position[2] - creature->m_entityData.position[2];
                        if (dx * dx + dy * dy + dz * dz >= Fixed(TrapHitRangeSquared) ||
                            creature->m_entityData.hostilityType == HostilityEnemy)
                            continue;
                        plasma::Vector<3, float> direction(
                            (float)(creature->m_entityData.position[0] - entity.position[0]),
                            (float)(creature->m_entityData.position[1] - entity.position[1]), 0.0f);
                        if (direction[0] * direction[0] + direction[1] * direction[1] + 0.0f > 0.0f)
                            direction.normalize();
                        direction[0] *= TrapPush;
                        direction[1] *= TrapPush;
                        direction[2] = TrapLift;
                        if (entity.type == StaticEntityStompTrap)
                            direction += direction;
                        float damage = creature->maxHP() * TrapDamageFraction;
                        if (entity.type == StaticEntityStompTrap)
                            damage *= StompTrapDamageFactor;
                        attackCreature(creature, nullptr, damage, false, false, 0.0f, entity.position, direction,
                                       entity.type == StaticEntityFireTrap, AbilityNone, nullptr, true);
                        if (entity.type != StaticEntityFireTrap) {
                            ServerUpdates::SoundPacket sound;
                            sound.position = creature->m_entityData.position;
                            sound.sound = SoundBlade1;
                            sound.speed = (float)rand() * 0.3f / (float)RAND_MAX + 1.0f;
                            m_serverUpdates.m_soundPacketList.push_back(sound);
                        }
                    }
                }
            }
        }
    }
}

// Each ability has its own particle and sound profile, strike reach and timing, and projectile speed and size. Damage
// rolls range from 75% to 125%. Most effects fire when the update crosses anticipationTime or strikeTime. The host
// runs non-player abilities; each client runs its own player's abilities.
void World::performAbility(Creature *creature, float milliseconds)
{
    switch (creature->m_entityData.currentAbility) {
    case AbilityDrink:
    case AbilityEat:
    case AbilityRestHeal: {
        // Eating or drinking the item in hand (an elixir gives its buff when the ability starts, anything else heals
        // over its cooldown); 130 heals over the attack duration.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (creature->m_entityData.actionItem.category != ItemCategoryConsumable &&
            creature->m_entityData.currentAbility != AbilityRestHeal) {
            if (creature->m_entityData.timeSinceAbility != 0.0f)
                break;
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = 1.0f;
            sound.sound = SoundDrink2;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            if (creature->m_entityData.actionItem.category != ItemCategoryElixir)
                break;
            // The elixirs' buffs, ten minutes each.
            Creature::Buff buff;
            switch (creature->m_entityData.actionItem.id) {
            case ElixirElixirOfPower:
                buff.type = BuffAttackflask;
                buff.strength = ElixirStrength;
                buff.timeRemaining = ElixirDuration;
                break;
            case ElixirElixirOfToughness:
                buff.type = BuffArmorflask;
                buff.strength = ElixirStrength;
                buff.timeRemaining = ElixirDuration;
                break;
            case ElixirElixirOfSanity:
                buff.type = BuffResistanceflask;
                buff.strength = ElixirStrength;
                buff.timeRemaining = ElixirDuration;
                break;
            case ElixirLemonade:
                buff.type = BuffHeatresistance;
                buff.timeRemaining = ElixirDuration;
                break;
            case ElixirHotChocolate:
                buff.type = BuffColdresistance;
                buff.timeRemaining = ElixirDuration;
                resetBuff(creature, BuffFrozen);
                break;
            case ElixirGreenSmoothie:
                buff.type = BuffPlagueresistance;
                buff.timeRemaining = ElixirDuration;
                break;
            default:
                buff.type = BuffLifeflask;
                buff.strength = ElixirStrength;
                buff.timeRemaining = ElixirDuration;
                break;
            }
            if (buff.type != BuffNone) {
                creature->setBuff(buff, true);
                ServerUpdates::BuffPacket packet;
                packet.sourceId = creature->m_id;
                packet.targetId = creature->m_id;
                packet.buff = buff;
                packet.replace = 0;
                m_serverUpdates.m_buffPacketList.push_back(packet);
            }
            break;
        }
        int cooldown = creature->m_entityData.actionItem.cooldown();
        if (creature->m_entityData.currentAbility == AbilityRestHeal) {
            cooldown = creature->attackDuration();
            // The first bite, 50 ms in.
            if (creature->m_entityData.timeSinceAbility < 50.0f &&
                creature->m_entityData.timeSinceAbility + milliseconds >= 50.0f) {
                ServerUpdates::ParticlePacket particles;
                particles.position = creature->m_entityData.position;
                particles.size = 0.4f;
                particles.color = plasma::Vector<4, float>(0.6f, 0.5f, 0.3f, 1.0f);
                particles.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 5.0f);
                particles.count = 15;
                m_serverUpdates.m_particlePacketList.push_back(particles);
                ServerUpdates::SoundPacket sound;
                sound.sound = SoundPickup;
                sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 0.8f;
                sound.position = creature->m_entityData.position;
                m_serverUpdates.m_soundPacketList.push_back(sound);
            }
        }
        // Every 200 ms of the cooldown heals a share of the full health (half of it for anyone but the player), every
        // 400 ms a gulp.
        if ((float)cooldown > creature->m_entityData.timeSinceAbility &&
            (int)(creature->m_entityData.timeSinceAbility + milliseconds) / 200 !=
                (int)creature->m_entityData.timeSinceAbility / 200) {
            float scale = 200.0f / (float)cooldown;
            if (creature->m_entityData.hostilityType != HostilityPlayer)
                scale *= 0.5f;
            float heal = creature->maxHP() * scale;
            if (heal > 0.0f) {
                ServerUpdates::HitPacket hit;
                hit.attackerId = -1;
                hit.targetId = creature->m_id;
                hit.position = creature->m_entityData.position;
                hit.damage = -heal;
                hit.critical = 0;
                m_serverUpdates.m_hitPacketList.push_back(hit);
                creature->m_entityData.hp += heal;
                if (creature->m_entityData.hp > creature->maxHP())
                    creature->m_entityData.hp = creature->maxHP();
            }
            if ((int)(creature->m_entityData.timeSinceAbility + milliseconds) / 400 !=
                (int)creature->m_entityData.timeSinceAbility / 400) {
                ServerUpdates::SoundPacket sound;
                sound.position = creature->m_entityData.position;
                sound.speed = 1.0f;
                sound.sound = SoundDrink2;
                m_serverUpdates.m_soundPacketList.push_back(sound);
            }
        }
        break;
    }
    case AbilityPortCity: {
        // The flight ends at the anticipation time: the player lands at the flight point's platform, and the mount
        // with it.
        if (creature != m_localCreature)
            break;
        if (!(creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
              creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()))
            break;
        const plasma::Vector<2, int> &flightPoint = m_localCreature->m_entityData.portCity;
        m_localCreature->m_entityData.position =
            plasma::Vector<3, Fixed>(Fixed(flightPoint[0] * BlocksPerCell + 6),
                                     Fixed(flightPoint[1] * BlocksPerCell + BlocksPerCell / 2), Fixed(0));
        m_localCreature->m_entityData.position[2] = Fixed(structureLevel(flightPoint[0], flightPoint[1], 0));
        Creature *mount = creatureByID(creature->m_petId);
        if (mount)
            mount->m_entityData.position = creature->m_entityData.position;
        break;
    }
    case AbilitySelfKill:
        // The player dies at the anticipation time.
        if (creature != m_localCreature)
            break;
        if (!(creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
              creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()))
            break;
        creature->m_entityData.hp = 0.0f;
        break;
    case AbilitySmashLeap:
    case AbilityIntercept:
    case AbilityDash: {
        // A jump: its stamina as it starts, its sound.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (creature->m_entityData.timeSinceAbility == 0.0f) {
            creature->m_stamina -= creature->staminaCost(creature->m_entityData.currentAbility);
            if (creature->m_stamina < 0.0f)
                creature->m_stamina = 0.0f;
        }
        if (creature->m_entityData.timeSinceAbility <= 0.0f &&
            creature->m_entityData.timeSinceAbility + milliseconds > 0.0f) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundSmashJump;
            sound.sourceId = creature->m_id;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        break;
    }
    case AbilityBlock: {
        // Charging: a sound at the anticipation time, then a tick every 200 ms whose pitch rises with the charge.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
            creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundShieldSwing;
            sound.sourceId = creature->m_id;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        if ((int)(creature->m_entityData.timeSinceAbility + milliseconds) / 200 !=
            (int)creature->m_entityData.timeSinceAbility / 200) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = creature->m_entityData.charge * 0.5f + 1.0f;
            sound.sound = SoundCharge2;
            sound.sourceId = creature->m_id;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        break;
    }
    case AbilityRightSlash:
    case AbilityLeftSlash:
    case Ability_4:
    case Ability_5:
    case AbilityPerforate:
    case AbilityRightStab:
    case AbilityLeftStab:
    case AbilityShieldAttack:
    case AbilityBlockJab:
    case AbilitySwirlStrike:
    case AbilityCrossedWeaponSweep:
    case AbilityLeftSweep:
    case AbilityRightSweep:
    case AbilityOverheadChop:
    case AbilityMonsterChargeStrike:
    case AbilityAmbush:
    case AbilityLeftDaggerPierce:
    case AbilityRightDaggerPierce:
    case AbilityKick:
    case AbilityRangerKick:
    case AbilityFireSwirl:
    case AbilityFireVortex:
    case AbilityWaterSwirl:
    case AbilityWaterVortex:
    case AbilityDiveBite:
    case AbilitySmash:
    case AbilityTwoHandedSweep:
    case AbilityTwoHandedThrust:
    case AbilityCrushStrike:
    case AbilityGuardianCrushStrike:
    case Ability_63:
    case AbilityTwoHandedSpin:
    case AbilityTwoHandedSlam:
    case AbilityTwoHandedSwing:
    case AbilityHeadbutt:
    case AbilityHornCharge:
    case AbilityTailStrike:
    case AbilityShellSpin:
    case AbilityStomp:
    case AbilityClawStrike:
    case AbilityDashForward:
    case AbilityDashLeft:
    case AbilityDashRight:
    case AbilityCyclone:
    case AbilityDjinnExplosion:
    case AbilityFireExplosion:
    case AbilityQuake:
    case AbilityLunge:
    case AbilityBlockStrike:
    case AbilityWraithAttack:
    case AbilityRockFist:
    case AbilityBloomlingAttack:
    case AbilityWarriorJumpAttack: {
        // The strikes: melee attacks and the spells that hit around a point. The strike's centre lies ahead of the
        // creature (or at the aimed place, or at the creature itself) and reaches `range` blocks; between strikeStart
        // and strikeEnd every living creature within reach is struck once (m_struckCreatures holds those already
        // struck, m_dodgingCreatures those that were rolling and dodge). Before that come the strike's sounds and
        // effects, the blocks it breaks, and what the ability costs.
        plasma::Matrix<float> turn;
        turn.identity();
        turn.rotateZ(creature->m_entityData.rotation[2]);
        float forward = 1.5f;
        float range = creature->m_entityData.appearance.size[0] * 1.5f;
        if (creature->m_entityData.currentAbility == AbilityRightSlash ||
            creature->m_entityData.currentAbility == AbilityLeftSlash ||
            creature->m_entityData.currentAbility == Ability_4 || creature->m_entityData.currentAbility == Ability_5 ||
            creature->m_entityData.currentAbility == AbilityRightStab ||
            creature->m_entityData.currentAbility == AbilityLeftStab ||
            creature->m_entityData.currentAbility == AbilityShieldAttack ||
            creature->m_entityData.currentAbility == AbilityLeftSweep ||
            creature->m_entityData.currentAbility == AbilityRightSweep ||
            creature->m_entityData.currentAbility == AbilityLeftDaggerPierce ||
            creature->m_entityData.currentAbility == AbilityRightDaggerPierce) {
            forward = 1.0f;
            range = creature->m_entityData.appearance.size[0] + creature->m_entityData.appearance.size[0];
        }
        if (creature->m_entityData.currentAbility == AbilityKick ||
            creature->m_entityData.currentAbility == AbilityRangerKick)
            range *= 2.25f;
        if (creature->m_entityData.currentAbility == AbilityCyclone ||
            creature->m_entityData.currentAbility == AbilityWarriorJumpAttack) {
            forward = 0.0f;
            range *= 1.75f;
        }
        if (creature->m_entityData.currentAbility == AbilityHeadbutt ||
            creature->m_entityData.currentAbility == AbilityTailStrike)
            forward = 0.0f;
        if (creature->m_entityData.currentAbility == AbilityRockFist) {
            forward = 0.0f;
            range += range;
        }
        if (creature->m_entityData.currentAbility == AbilityBlockStrike) {
            forward = 0.0f;
            range += range;
        }
        if (creature->m_entityData.currentAbility == AbilityDjinnExplosion ||
            creature->m_entityData.currentAbility == AbilityFireExplosion) {
            forward = 0.0f;
            range = 8.0f;
        }
        if (creature->m_entityData.currentAbility == AbilityFireSwirl ||
            creature->m_entityData.currentAbility == AbilityWaterSwirl) {
            forward = 0.0f;
            range = 3.0f;
        }
        if (creature->m_entityData.currentAbility == AbilityFireVortex ||
            creature->m_entityData.currentAbility == AbilityWaterVortex) {
            forward = 0.0f;
            range = 5.0f;
        }
        if (creature->m_entityData.currentAbility == AbilityQuake ||
            creature->m_entityData.currentAbility == AbilityStomp) {
            forward = 0.0f;
            range = 20.0f;
        }
        if (creature->m_entityData.currentAbility == AbilityWraithAttack ||
            creature->m_entityData.currentAbility == AbilityShellSpin ||
            (creature->m_entityData.currentAbility >= AbilityDashForward &&
             creature->m_entityData.currentAbility <= AbilityDashRight)) {
            forward = 0.0f;
            range *= 1.2f;
        }
        plasma::Vector<3, Fixed> center =
            creature->m_entityData.position + plasma::Vector<3, Fixed>(turn.transformDirection(plasma::Vector<3, float>(
                                                  0.0f, forward * creature->m_entityData.appearance.size[0], 0.0f)));
        if (creature->m_entityData.currentAbility >= AbilityFireSwirl &&
            creature->m_entityData.currentAbility <= AbilityWaterVortex)
            center = creature->m_entityData.position + plasma::Vector<3, Fixed>(creature->m_entityData.attackRotation);
        if (creature->m_entityData.currentAbility == AbilityClawStrike ||
            creature->m_entityData.currentAbility == AbilityHornCharge) {
            center[2] -=
                Fixed((creature->m_entityData.appearance.size[2] - creature->m_entityData.appearance.size[0]) * 0.5f);
        }
        if (creature->m_entityData.currentAbility == AbilitySwirlStrike ||
            creature->m_entityData.currentAbility == AbilitySmash ||
            creature->m_entityData.currentAbility == AbilityGuardianCrushStrike ||
            creature->m_entityData.currentAbility == AbilityBloomlingAttack) {
            center = creature->m_entityData.position;
            range *= 3.0f;
        }
        // The strike lasts from 100 ms before the middle of the strike time to the middle, within the strike time.
        int strikeStart = creature->strikeTime() / 2 - 100 + creature->anticipationTime();
        int strikeEnd = creature->strikeTime() / 2 + creature->anticipationTime();
        if (strikeStart < creature->anticipationTime())
            strikeStart = creature->anticipationTime();
        if (strikeEnd >= creature->anticipationTime() + creature->strikeTime())
            strikeEnd = creature->anticipationTime() - 1 + creature->strikeTime();
        if (creature->m_entityData.currentAbility == AbilityHeadbutt ||
            creature->m_entityData.currentAbility == AbilityHornCharge)
            strikeStart = creature->strikeTime() / 3 + creature->anticipationTime();
        if (creature->m_entityData.currentAbility == AbilityLunge) {
            strikeStart = creature->strikeTime() / 5 + creature->anticipationTime();
            strikeEnd = creature->anticipationTime() + creature->strikeTime();
        }
        if (creature->m_entityData.currentAbility == AbilityRockFist) {
            strikeStart = creature->anticipationTime();
            strikeEnd = creature->anticipationTime() + creature->strikeTime();
        }
        bool newStrike = false;
        if (creature->m_entityData.currentAbility == AbilityQuake ||
            creature->m_entityData.currentAbility == AbilityWraithAttack ||
            creature->m_entityData.currentAbility == AbilityBloomlingAttack ||
            creature->m_entityData.currentAbility == AbilityWarriorJumpAttack ||
            creature->m_entityData.currentAbility == AbilityShellSpin ||
            creature->m_entityData.currentAbility == AbilityCyclone ||
            creature->m_entityData.currentAbility == AbilityPerforate ||
            creature->m_entityData.currentAbility == AbilitySwirlStrike ||
            creature->m_entityData.currentAbility == AbilityFireSwirl ||
            creature->m_entityData.currentAbility == AbilityFireVortex ||
            creature->m_entityData.currentAbility == AbilityWaterSwirl ||
            creature->m_entityData.currentAbility == AbilityWaterVortex) {
            // These strike again and again through the strike time, each strike 100 ms long; a new strike forgets whom
            // the last hit.
            if (creature->m_entityData.timeSinceAbility >= (float)creature->anticipationTime() &&
                creature->m_entityData.timeSinceAbility <
                    (float)(creature->anticipationTime() + creature->strikeTime())) {
                int period = 250;
                if (creature->m_entityData.currentAbility == AbilityQuake)
                    period = 1000;
                if (creature->m_entityData.currentAbility == AbilitySwirlStrike)
                    period = creature->strikeTime() / 3;
                if (creature->m_entityData.currentAbility == AbilityBloomlingAttack)
                    period = creature->strikeTime() / 3;
                if (creature->m_entityData.currentAbility == AbilityPerforate)
                    period = creature->strikeTime() / 3;
                if (creature->m_entityData.currentAbility == AbilityFireSwirl ||
                    creature->m_entityData.currentAbility == AbilityWaterSwirl)
                    period = creature->strikeTime() / 6;
                if (creature->m_entityData.currentAbility == AbilityFireVortex ||
                    creature->m_entityData.currentAbility == AbilityWaterVortex)
                    period = creature->strikeTime() / 12;
                int elapsed = (int)(creature->m_entityData.timeSinceAbility - (float)creature->anticipationTime());
                strikeStart = creature->anticipationTime() + (elapsed - elapsed % period);
                strikeEnd = strikeStart + 100;
                int ahead =
                    (int)(creature->m_entityData.timeSinceAbility + milliseconds - (float)creature->anticipationTime());
                int nextStart = creature->anticipationTime() + (ahead - ahead % period);
                if (nextStart != strikeStart && nextStart < creature->anticipationTime() + creature->strikeTime()) {
                    creature->m_strikeCombo = creature->m_entityData.hitCombo;
                    newStrike = true;
                    creature->m_struckCreatures.clear();
                    creature->m_dodgingCreatures.clear();
                }
                if (creature->m_entityData.timeSinceAbility + milliseconds > (float)strikeEnd)
                    newStrike = true;
                if (!((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) &&
                      creature != m_localCreature) &&
                    creature->m_entityData.currentAbility != AbilityWraithAttack &&
                    creature->m_entityData.currentAbility != AbilityPerforate) {
                    // Each strike's swing.
                    if ((float)creature->anticipationTime() > creature->m_entityData.timeSinceAbility - milliseconds ||
                        (int)(creature->m_entityData.timeSinceAbility - (float)creature->anticipationTime() +
                              milliseconds) /
                                period !=
                            (int)(creature->m_entityData.timeSinceAbility - (float)creature->anticipationTime()) /
                                period) {
                        ServerUpdates::SoundPacket sound;
                        sound.position = creature->m_entityData.position;
                        if (creature->m_entityData.binaryToggles.faceAim) {
                            sound.position = creature->m_entityData.position +
                                             plasma::Vector<3, Fixed>(creature->m_entityData.attackRotation);
                        }
                        sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
                        sound.sound = SoundSwing;
                        sound.sourceId = creature->m_id;
                        m_serverUpdates.m_soundPacketList.push_back(sound);
                        if (creature->m_entityData.currentAbility == AbilityQuake) {
                            ServerUpdates::SoundPacket rumble;
                            rumble.position = creature->m_entityData.position;
                            rumble.speed = (float)rand() * 0.1f / (float)RAND_MAX + 0.5f;
                            rumble.sound = SoundPunch285;
                            rumble.sourceId = creature->m_id;
                            m_serverUpdates.m_soundPacketList.push_back(rumble);
                        }
                    }
                }
            } else {
                strikeStart = creature->anticipationTime();
                strikeEnd = strikeStart;
            }
        }
        if (creature->m_entityData.timeSinceAbility > (float)strikeStart &&
            (float)strikeEnd >= creature->m_entityData.timeSinceAbility && !m_isClient &&
            creature->m_entityData.currentAbility != AbilityStomp &&
            creature->m_entityData.currentAbility != AbilityDjinnExplosion &&
            creature->m_entityData.currentAbility != AbilityQuake &&
            creature->m_entityData.currentAbility != AbilityWraithAttack) {
            // On the host the strike breaks the breakable blocks within its reach (four to eight blocks) of the
            // centre.
            bool broken = false;
            int reach = (int)range;
            if (reach > 8)
                reach = 8;
            else if (reach < 4)
                reach = 4;
            for (int x = -reach; x <= reach; x++) {
                for (int y = -reach; y <= reach; y++) {
                    for (int z = -reach; z <= reach; z++) {
                        plasma::Vector<3, Fixed> point =
                            center + plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z));
                        if (!(plasma::Vector<3, float>(point - center).squaredLength() < (float)(reach * reach)))
                            continue;
                        Block block = getBlockAtDot(point[0], point[1], point[2]);
                        if (!(block.flags & BlockFlagDestructible))
                            continue;
                        // The block bursts in its own colour and is replaced by air.
                        ServerUpdates::ParticlePacket particles;
                        particles.position = point;
                        particles.size = 0.5f;
                        particles.color = plasma::Vector<4, float>((float)block.color[0] * (1.0f / 128.0f),
                                                                   (float)block.color[1] * (1.0f / 128.0f),
                                                                   (float)block.color[2] * (1.0f / 128.0f), 1.0f);
                        particles.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 10.0f);
                        particles.count = 3;
                        m_serverUpdates.m_particlePacketList.push_back(particles);
                        destroyBlock(point[0], point[1], point[2]);
                        CubeChange change;
                        change.position = fixedToBlock(point);
                        change.day = m_state.m_day;
                        change.originalBlock = block;
                        change.newBlock = s_airCube;
                        m_serverUpdates.m_cubeChangeList.push_back(change);
                        broken = true;
                    }
                }
            }
            if (broken) {
                ServerUpdates::SoundPacket sound;
                sound.position = creature->m_entityData.position;
                sound.speed = (float)rand() * 0.4f / (float)RAND_MAX + 0.5f;
                sound.sound = SoundRoll;
                m_serverUpdates.m_soundPacketList.push_back(sound);
                Fixed extent = Fixed(range);
                updateSwath((int)(center[0] - extent), (int)(center[1] - extent), (int)(center[0] + extent),
                            (int)(center[1] + extent));
            }
        }
        if (!m_isClient && creature->m_entityData.hostilityType == HostilityPlayer && creature != m_localCreature)
            break;
        if (creature->m_entityData.classType == ClassRogue &&
            creature->m_entityData.specialization == SpecializationNinja &&
            (creature->m_entityData.currentAbility == AbilityPerforate ||
             creature->m_entityData.currentAbility == AbilityAmbush ||
             creature->m_entityData.currentAbility == AbilityKick) &&
            creature->m_entityData.timeSinceAbility == 0.0f) {
            creature->m_entityData.rollTime = creature->m_entityData.mp * 600.0f * creature->m_entityData.mp;
        }
        if (creature->m_entityData.currentAbility == AbilityCyclone ||
            creature->m_entityData.currentAbility == AbilityWarriorJumpAttack) {
            // The whirl: two turns a second while the attack lasts.
            if (creature->m_entityData.timeSinceAbility < (float)creature->attackDuration()) {
                creature->m_entityData.rotation[2] = interpolateAngle(
                    creature->m_entityData.rotation[2],
                    milliseconds * 0.00025f * 360.0f * 8.0f + creature->m_entityData.rotation[2], 1.0f);
            }
        }
        int castTime = creature->anticipationTime();
        if (creature->m_entityData.currentAbility == AbilityDjinnExplosion ||
            creature->m_entityData.currentAbility == AbilityFireExplosion) {
            if (creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime() &&
                (int)(creature->m_entityData.timeSinceAbility + milliseconds) / 200 !=
                    (int)creature->m_entityData.timeSinceAbility / 200) {
                ServerUpdates::SoundPacket sound;
                sound.position = creature->m_entityData.position;
                sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
                sound.sound = SoundChannel2;
                m_serverUpdates.m_soundPacketList.push_back(sound);
            }
            if (creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime() &&
                creature->m_entityData.timeSinceAbility + milliseconds >= (float)creature->anticipationTime()) {
                // The colour is not set.
                ServerUpdates::ParticlePacket particles;
                particles.position = creature->m_entityData.position;
                particles.size = 0.25f;
                particles.type = ParticleFire;
                particles.spread = 20.0f;
                particles.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
                particles.count = 80;
                ServerUpdates::SoundPacket sound;
                sound.position = creature->m_entityData.position;
                sound.speed = 1.0f;
                sound.sound = SoundExplosion;
                m_serverUpdates.m_soundPacketList.push_back(sound);
                m_serverUpdates.m_particlePacketList.push_back(particles);
            }
        } else if (creature->m_entityData.currentAbility >= AbilityFireSwirl &&
                   creature->m_entityData.currentAbility <= AbilityWaterVortex) {
            if (creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime() &&
                (int)(creature->m_entityData.timeSinceAbility + milliseconds) / 200 !=
                    (int)creature->m_entityData.timeSinceAbility / 200) {
                ServerUpdates::SoundPacket sound;
                sound.position = creature->m_entityData.position;
                sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
                sound.sound = SoundChannel2;
                m_serverUpdates.m_soundPacketList.push_back(sound);
            }
            if (creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime() &&
                creature->m_entityData.timeSinceAbility + milliseconds >= (float)creature->anticipationTime()) {
                // A burst at the aimed place, blue for specialization 1 (otherwise the colour is not set).
                ServerUpdates::ParticlePacket particles;
                particles.position =
                    creature->m_entityData.position + plasma::Vector<3, Fixed>(creature->m_entityData.attackRotation);
                particles.size = 0.25f;
                if (creature->m_entityData.specialization == SpecializationWater) {
                    particles.type = ParticleGlow;
                    particles.color = plasma::Vector<4, float>(0.0f, 0.2f, 1.0f, 1.0f);
                } else {
                    particles.type = ParticleFire;
                }
                particles.spread = 8.0f;
                particles.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 2.0f);
                particles.count = 20;
                ServerUpdates::SoundPacket sound;
                sound.position = creature->m_entityData.position;
                sound.speed = 1.0f;
                sound.sound =
                    creature->m_entityData.specialization == SpecializationWater ? SoundWatersplash : SoundFireball;
                m_serverUpdates.m_soundPacketList.push_back(sound);
                m_serverUpdates.m_particlePacketList.push_back(particles);
            }
        } else if (creature->m_entityData.timeSinceAbility == 0.0f && creature->hasTwoHandedWeapon() &&
                   creature->m_entityData.currentAbility != AbilityQuake) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundSwingSlow;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        if (creature->m_entityData.currentAbility == AbilityBloomlingAttack &&
            creature->m_entityData.timeSinceAbility <= 100.0f &&
            creature->m_entityData.timeSinceAbility + milliseconds > 100.0f)
            creature->m_entityData.velocity[2] += 8.0f;
        // The guard is tested again here and below.
        if (!((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature) &&
            creature->m_entityData.timeSinceAbility <= (float)castTime &&
            creature->m_entityData.timeSinceAbility + milliseconds > (float)castTime &&
            creature->m_entityData.currentAbility != AbilityQuake) {
            // The swing at the anticipation time.
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.2f / (float)RAND_MAX + 0.9f;
            sound.sourceId = creature->m_id;
            if (creature->m_entityData.currentAbility == AbilityBlockJab) {
                sound.sound = SoundShieldSwing;
            } else if (creature->m_entityData.currentAbility == AbilitySwirlStrike) {
                sound.sound = SoundSwirl;
            } else if (creature->m_entityData.currentAbility == AbilityBloomlingAttack) {
                sound.sound = SoundSwirl;
                sound.speed *= 2.0f;
            } else if (creature->m_entityData.currentAbility == AbilityWarriorJumpAttack) {
                sound.sound = SoundSwirl;
                sound.speed *= 2.0f;
            } else if (creature->m_entityData.currentAbility == AbilitySmash) {
                sound.sound = SoundSmashJump;
            } else {
                sound.speed *= 1.5f;
                if (creature->hasTwoHandedWeapon() || creature->m_entityData.currentAbility == AbilityHeadbutt ||
                    creature->m_entityData.currentAbility == AbilityHornCharge ||
                    creature->m_entityData.currentAbility == AbilityRockFist ||
                    creature->m_entityData.currentAbility == AbilityLunge) {
                    sound.sound = SoundSwing;
                    sound.speed *= 0.5f;
                } else {
                    sound.sound = SoundSwing;
                }
            }
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        if (!((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature) &&
            (creature->m_entityData.currentAbility == AbilityTwoHandedSweep ||
             creature->m_entityData.currentAbility == AbilityTwoHandedThrust ||
             creature->m_entityData.currentAbility == AbilityCrushStrike ||
             creature->m_entityData.currentAbility == AbilityStomp)) {
            if (creature->m_entityData.timeSinceAbility <
                    (float)(creature->strikeTime() / 2 + creature->anticipationTime()) &&
                creature->m_entityData.timeSinceAbility + milliseconds >=
                    (float)(creature->strikeTime() / 2 + creature->anticipationTime())) {
                ServerUpdates::SoundPacket sound;
                sound.position = creature->m_entityData.position;
                sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
                sound.sound = SoundSlamGround;
                sound.sourceId = creature->m_id;
                m_serverUpdates.m_soundPacketList.push_back(sound);
            }
        }
        if (creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime()) {
            // Until the wind-up is over: whoever rolled out of the last strike (in m_dodgingCreatures, not in
            // m_struckCreatures) dodged it.
            creature->m_strikeCombo = creature->m_entityData.hitCombo;
            for (auto &dodgerId : creature->m_dodgingCreatures) {
                if (creature->m_struckCreatures.find(dodgerId) != creature->m_struckCreatures.end())
                    continue;
                Creature *other = creatureByID(dodgerId);
                if (!other)
                    continue;
                other->applyDodgeBonus(m_serverUpdates);
                ServerUpdates::HitPacket dodge;
                dodge.hitType = HitTypeDodge;
                dodge.damage = 0.0f;
                dodge.critical = 0;
                dodge.attackerId = creature->m_id;
                dodge.targetId = other->m_id;
                dodge.position = other->m_entityData.position;
                m_serverUpdates.m_hitPacketList.push_back(dodge);
            }
            creature->m_struckCreatures.clear();
            creature->m_dodgingCreatures.clear();
            creature->m_strikeLanded = 0;
        }
        if (creature->m_entityData.currentAbility == AbilityStomp &&
            creature->m_entityData.timeSinceAbility <
                (float)(creature->strikeTime() / 2 + creature->anticipationTime()) &&
            creature->m_entityData.timeSinceAbility + milliseconds >=
                (float)(creature->strikeTime() / 2 + creature->anticipationTime())) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 0.5f;
            sound.sound = SoundPunch285;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        if (creature->m_entityData.timeSinceAbility >= (float)strikeEnd) {
            // The strike is over: the same for those who rolled out of it.
            for (auto &dodgerId : creature->m_dodgingCreatures) {
                if (creature->m_struckCreatures.find(dodgerId) != creature->m_struckCreatures.end())
                    continue;
                Creature *other = creatureByID(dodgerId);
                if (!other)
                    continue;
                other->applyDodgeBonus(m_serverUpdates);
                ServerUpdates::HitPacket dodge;
                dodge.hitType = HitTypeDodge;
                dodge.damage = 0.0f;
                dodge.critical = 0;
                dodge.attackerId = creature->m_id;
                dodge.targetId = other->m_id;
                dodge.position = other->m_entityData.position;
                m_serverUpdates.m_hitPacketList.push_back(dodge);
            }
            creature->m_dodgingCreatures.clear();
        }
        if (!creature->m_strikeLanded && creature->m_entityData.timeSinceAbility + milliseconds >=
                                             (float)(creature->anticipationTime() + creature->strikeTime())) {
            // The strike time ends without a hit: the combo ends.
            if (creature->m_entityData.timeSinceAbility < (float)creature->attackDuration() &&
                creature->m_entityData.rollTime == 0.0f) {
                float tempo = creature->tempo();
                creature->m_entityData.hitCombo = 0;
                creature->m_entityData.timeSinceAbility *= tempo / creature->tempo();
            }
        }
        if (creature->m_entityData.timeSinceAbility == 0.0f) {
            creature->m_stamina -= creature->staminaCost(creature->m_entityData.currentAbility);
            if (creature->m_stamina < 0.0f)
                creature->m_stamina = 0.0f;
        }
        // The strike's strength (m_strikeStrength): the ability's mana cost, for 6, 18 and 21 all the mana (which ends
        // buff 11 and makes the strike critical), or the charge.
        if (creature->m_entityData.currentAbility == AbilityFireVortex ||
            creature->m_entityData.currentAbility == AbilityWaterVortex) {
            if (creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
                creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()) {
                creature->m_strikeStrength = 0.1f;
                creature->m_entityData.mp -= creature->manaCost(creature->m_entityData.currentAbility, -1);
                if (creature->m_entityData.mp < 0.0f)
                    creature->m_entityData.mp = 0.0f;
            }
        } else if (creature->m_entityData.timeSinceAbility == 0.0f) {
            creature->m_criticalStrike = 0;
            creature->m_strikeStrength = 0.0f;
            if (creature->manaCost(creature->m_entityData.currentAbility, -1) > 0.0f) {
                creature->m_strikeStrength = creature->manaCost(creature->m_entityData.currentAbility, -1);
                creature->m_entityData.mp -= creature->m_strikeStrength;
            } else if (creature->m_entityData.currentAbility == AbilityPerforate ||
                       creature->m_entityData.currentAbility == AbilityAmbush ||
                       creature->m_entityData.currentAbility == AbilityKick) {
                creature->m_strikeStrength = creature->m_entityData.mp;
                creature->m_entityData.mp = 0.0f;
                if (creature->findBuff(BuffDodge)) {
                    Creature::Buff buff;
                    buff.type = BuffDodge;
                    creature->setBuff(buff, false);
                    ServerUpdates::BuffPacket packet;
                    packet.buff = buff;
                    packet.sourceId = creature->m_id;
                    packet.targetId = creature->m_id;
                    m_serverUpdates.m_buffPacketList.push_back(packet);
                    creature->m_criticalStrike = 1;
                }
            } else if (creature->m_entityData.currentAbility == AbilitySmash ||
                       creature->m_entityData.currentAbility == AbilityRockFist ||
                       creature->m_entityData.currentAbility == AbilityRangerKick ||
                       creature->m_entityData.currentAbility == AbilityDjinnExplosion ||
                       creature->m_entityData.currentAbility == AbilityFireExplosion) {
                creature->m_strikeStrength = 1.0f;
            }
        }
        if (creature->m_entityData.charge > 0.0f) {
            creature->m_strikeStrength = creature->m_entityData.charge;
            creature->m_entityData.mp -= creature->m_entityData.charge;
            creature->m_strikeLanded = 0;
            if (creature->m_entityData.currentAbility == AbilitySwirlStrike ||
                creature->m_entityData.currentAbility == AbilityPerforate)
                creature->m_strikeStrength *= 0.5f;
        }
        creature->m_entityData.charge = 0.0f;
        if (creature->m_entityData.mp < 0.0f)
            creature->m_entityData.mp = 0.0f;
        bool powered = creature->m_strikeStrength > 0.0f ||
                       creature->m_entityData.currentAbility == AbilitySwirlStrike ||
                       creature->m_entityData.currentAbility == AbilityCyclone ||
                       creature->m_entityData.currentAbility == AbilityWarriorJumpAttack;
        if (!(creature->m_entityData.timeSinceAbility > (float)strikeStart &&
              (float)strikeEnd >= creature->m_entityData.timeSinceAbility))
            break;
        float unchargedDamageFactor = 1.0f;
        if (creature->m_entityData.currentAbility == AbilitySwirlStrike ||
            creature->m_entityData.currentAbility == AbilityCrushStrike ||
            creature->m_entityData.currentAbility == Ability_63)
            unchargedDamageFactor = 0.0f;
        bool hit = false;
        if (creature->m_entityData.currentAbility == AbilityFireVortex &&
            creature->m_entityData.timeSinceAbility >= (float)creature->anticipationTime() &&
            creature->findBuff(BuffInstantfire)) {
            Creature::Buff buff;
            buff.type = BuffInstantfire;
            creature->setBuff(buff, false);
            ServerUpdates::BuffPacket packet;
            packet.buff = buff;
            packet.sourceId = creature->m_id;
            packet.targetId = creature->m_id;
            m_serverUpdates.m_buffPacketList.push_back(packet);
        }
        for (Creature *other : m_creatures) {
            if (other->m_entityData.hp <= 0.0f)
                continue;
            if (creature->m_struckCreatures.find(other->m_id) != creature->m_struckCreatures.end())
                continue;
            float radius = other->m_entityData.appearance.size[0] * 0.7f + range;
            plasma::Vector<3, Fixed> origin = creature->m_entityData.position;
            if (creature->m_entityData.currentAbility >= AbilityFireSwirl &&
                creature->m_entityData.currentAbility <= AbilityWaterVortex)
                origin = center + plasma::Vector<3, Fixed>(0, 0, Fixed(range));
            // The horizontal distance is compared in 16.16 fixed point.
            plasma::Vector<3, Fixed> offset = center - other->m_entityData.position;
            plasma::Vector<2, Fixed> flat(offset[0], offset[1]);
            if (flat[0] * flat[0] + flat[1] * flat[1] >= Fixed(radius * radius))
                continue;
            float above = (float)(center[2] - other->m_entityData.position[2]);
            if (!(other->m_entityData.appearance.size[2] * 0.5f + creature->m_entityData.appearance.size[2] + range >
                  (float)fabs((double)above)))
                continue;
            if (!inRange(origin, other->m_entityData.position, true, 200.0f, other))
                continue;
            if (creature->canDoDamage(other)) {
                if (other->m_entityData.rollTime != 0.0f) {
                    // A rolling creature may dodge the strike.
                    creature->m_dodgingCreatures.insert(other->m_id);
                    continue;
                }
                plasma::Vector<3, float> away =
                    plasma::Vector<3, float>(other->m_entityData.position - creature->m_entityData.position);
                away[2] = 0.0f;
                if (away.squaredLength() > 0.01f)
                    away.normalize();
                int element = ElementPhysical;
                if (creature->m_entityData.currentAbility == AbilityFireSwirl ||
                    creature->m_entityData.currentAbility == AbilityFireVortex)
                    element = ElementFire;
                if (creature->m_entityData.currentAbility == AbilityWaterSwirl ||
                    creature->m_entityData.currentAbility == AbilityWaterVortex)
                    element = ElementWater;
                bool critical;
                if (creature->m_criticalStrike) {
                    critical = true;
                } else {
                    float roll = (float)rand() / (float)RAND_MAX;
                    critical = creature->critical(other, element != ElementPhysical) > roll;
                }
                float knock = creature->knockback(creature->m_entityData.currentAbility);
                float damage;
                if (creature->m_entityData.currentAbility == AbilityDjinnExplosion ||
                    creature->m_entityData.currentAbility == AbilityFireExplosion ||
                    (creature->m_entityData.currentAbility >= AbilityFireSwirl &&
                     creature->m_entityData.currentAbility <= AbilityWaterVortex)) {
                    float power =
                        creature->damageScale(creature->m_entityData.currentAbility) * creature->spellPower(false);
                    damage = (1.25f - (float)rand() * 0.5f / (float)RAND_MAX) * power;
                } else {
                    float power =
                        creature->damageScale(creature->m_entityData.currentAbility) * creature->attackPower(false);
                    // Adds a zero term to the random scale here.
                    float scale = 1.25f - (float)rand() * 0.5f / (float)RAND_MAX + 0.0f;
                    damage = scale *
                             ((creature->m_strikeStrength * creature->m_strikeStrength * 5.0f + unchargedDamageFactor) *
                              power);
                }
                if (creature->m_entityData.currentAbility == AbilityRangerKick) {
                    knock = rampSpeed(creature->skillLevel(AbilityRangerKick));
                    damage *= rampScale(22);
                }
                if (critical)
                    damage += damage;
                // The push grows with the attacker's size against the target's, up to 1.75 times.
                float ratio = sqrtf(creature->m_entityData.appearance.size[0] / other->m_entityData.appearance.size[0]);
                away[2] = 0.25f;
                away *= (std::min)(1.75f, ratio);
                away *= knock;
                // The first creature a strike hits counts for the combo, unless the attacker is stunned.
                bool first = creature->m_struckCreatures.size() == 0 && creature->m_entityData.stunTime <= 0.0f;
                if (attackCreature(other, creature, damage, critical, powered, creature->m_strikeStrength,
                                   creature->m_entityData.position, away, element,
                                   creature->m_entityData.currentAbility, nullptr, true)) {
                    if (creature->m_entityData.currentAbility == AbilityFireSwirl && creature->hasRandomChance(0.25f) &&
                        creature->m_entityData.specialization == SpecializationFire) {
                        // Buff 9 for thirty seconds.
                        Creature::Buff buff;
                        buff.type = BuffInstantfire;
                        buff.timeRemaining = 30000.0f;
                        creature->setBuff(buff, false);
                        ServerUpdates::BuffPacket packet;
                        packet.buff = buff;
                        packet.sourceId = creature->m_id;
                        packet.targetId = creature->m_id;
                        m_serverUpdates.m_buffPacketList.push_back(packet);
                        ServerUpdates::SoundPacket sound;
                        sound.sound = SoundUpgrade2;
                        sound.position = creature->m_entityData.position;
                        m_serverUpdates.m_soundPacketList.push_back(sound);
                    }
                    if (creature->m_entityData.classType == ClassWarrior) {
                        // Class 1 may gain buff 2 for eight seconds, the likelier the slower the attack (up to a
                        // tenth).
                        float chance = (float)creature->attackDuration() * 0.01f;
                        if (creature->hasRandomChance((std::min)(1.0f, chance) * 0.1f)) {
                            Creature::Buff buff;
                            buff.type = BuffWarfrenzy;
                            buff.timeRemaining = 8000.0f;
                            creature->setBuff(buff, false);
                            ServerUpdates::BuffPacket packet;
                            packet.buff = buff;
                            packet.sourceId = creature->m_id;
                            packet.targetId = creature->m_id;
                            m_serverUpdates.m_buffPacketList.push_back(packet);
                            ServerUpdates::SoundPacket sound;
                            sound.sound = SoundBulwark;
                            sound.position = creature->m_entityData.position;
                            m_serverUpdates.m_soundPacketList.push_back(sound);
                        }
                    }
                    if (first) {
                        creature->m_entityData.hitCombo++;
                        creature->onHit();
                        creature->m_entityData.timeSinceHit = 0.0f;
                    }
                    if (powered && creature->m_entityData.classType == ClassRogue &&
                        creature->m_entityData.specialization == SpecializationAssassin) {
                        creature->m_entityData.stealth += creature->m_strikeStrength;
                        if (creature->m_entityData.stealth > 1.0f)
                            creature->m_entityData.stealth = 1.0f;
                    }
                    if ((!m_isClient || creature == m_localCreature) && !hit && !powered) {
                        // The first plain hit of a strike generates mana.
                        float gain = (1.0f - (float)rand() * 2.0f / (float)RAND_MAX) * 0.05f + 0.1f;
                        if (creature->m_entityData.currentAbility == AbilityTwoHandedThrust)
                            gain = gain + gain;
                        gain *= (float)creature->attackDuration() / 500.0f;
                        if (rand() % 8 == 0)
                            gain *= 1.5f;
                        creature->m_entityData.mp = creature->manaGeneration() * gain + creature->m_entityData.mp;
                        if (creature->m_entityData.mp > 1.0f)
                            creature->m_entityData.mp = 1.0f;
                    }
                    hit = true;
                }
                creature->m_struckCreatures.insert(other->m_id);
                creature->m_strikeLanded = 1;
            }
            if ((creature->m_entityData.currentAbility == AbilityWaterSwirl ||
                 creature->m_entityData.currentAbility == AbilityWaterVortex) &&
                newStrike && !creature->canDoDamage(other) &&
                other->m_entityData.hostilityType != HostilityInteractiveObject &&
                creature->m_entityData.specialization == SpecializationWater) {
                // Specialization 1 heals the friendly creatures in reach with every new strike (critical is the
                // critical flag).
                ServerUpdates::HitPacket heal;
                heal.position = other->m_entityData.position;
                float scale = (float)rand() * 0.5f / (float)RAND_MAX - 1.25f;
                heal.damage = creature->spellPower(false) * scale * 0.1f;
                float roll = (float)rand() / (float)RAND_MAX;
                heal.critical = creature->critical(other, true) > roll;
                heal.attackerId = creature->m_id;
                heal.targetId = other->m_id;
                m_serverUpdates.m_hitPacketList.push_back(heal);
                ServerUpdates::SoundPacket sound;
                sound.position = creature->m_entityData.position;
                sound.speed = 1.0f;
                sound.sound = SoundWatersplashHit;
                m_serverUpdates.m_soundPacketList.push_back(sound);
                if (m_isClient && (creature != m_localCreature || other != m_localCreature))
                    continue;
                applyHit(heal);
            }
        }
        // The call's result is not used.
        if (hit && creature->m_entityData.equipment.weaponLeft.id != WeaponShield &&
            (creature->m_entityData.equipment.weaponRight.category == ItemCategoryNone ||
             creature->m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon))
            creature->attackDuration();
        break;
    }
    case AbilityAssassinate: {
        // Riding down: sixteen strikes over the strike time at the mount's target, each for mana, with the attack
        // power.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (!(creature->m_entityData.timeSinceAbility > (float)creature->anticipationTime()))
            break;
        if (!(creature->m_entityData.timeSinceAbility <=
              (float)(creature->anticipationTime() + creature->strikeTime())))
            break;
        int period = (int)((float)creature->strikeTime() * 0.0625f);
        if ((int)(creature->m_entityData.timeSinceAbility + milliseconds - (float)creature->anticipationTime()) /
                period ==
            (int)(creature->m_entityData.timeSinceAbility - (float)creature->anticipationTime()) / period)
            break;
        if (!(creature->m_entityData.mp >= creature->manaCost(creature->m_entityData.currentAbility, -1)))
            break;
        Creature *target = creatureByID(creature->m_entityData.focusId);
        if (!target)
            break;
        float damage = creature->damageScale(creature->m_entityData.currentAbility) * creature->attackPower(false);
        damage = (1.25f - (float)rand() * 0.5f / (float)RAND_MAX) * damage;
        float roll = (float)rand() / (float)RAND_MAX;
        bool critical = creature->critical(target, false) > roll;
        if (critical)
            damage = damage + damage;
        if (attackCreature(target, creature, damage, critical, true, 0.0f, creature->m_entityData.position,
                           plasma::Vector<3, float>(0.0f, 0.0f, 0.0f), ElementPhysical,
                           creature->m_entityData.currentAbility, nullptr, true)) {
            creature->m_entityData.hitCombo++;
            creature->onHit();
            creature->m_entityData.timeSinceHit = 0.0f;
        }
        break;
    }
    case AbilityQuicksandTrap: {
        // A ramp (a projectile of ElementQuicksand) laid as the ability starts, with the range and duration of skill
        // 142's level.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (creature->m_entityData.timeSinceAbility != 0.0f)
            break;
        ServerUpdates::BulletPacket bullet;
        bullet.knockback = 0.0f;
        bullet.projectileType = ProjectileGroundSpot;
        bullet.powered = 0;
        bullet.strength = 0;
        bullet.damage = creature->damageScale(creature->m_entityData.currentAbility) * creature->attackPower(false);
        bullet.sizeInBlocks = rampRange(creature->skillLevel(AbilityQuicksandTrap));
        bullet.ownerId = creature->m_id;
        bullet.element = ElementQuicksand;
        bullet.position = creature->m_entityData.position;
        bullet.visualSize = 1.0f;
        bullet.currentTime = 0;
        bullet.timeToLive = rampDuration(creature->skillLevel(AbilityQuicksandTrap));
        bullet.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        m_projectiles.push_back(bullet);
        m_serverUpdates.m_bulletPacketList.push_back(bullet);
        break;
    }
    case AbilityHeal: {
        // Healing: a tick every 200 ms of the wind-up, then eight heals over the strike time, each for mana, of twice
        // the spell power on the target (or the healer itself).
        if (creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime() &&
            (int)(creature->m_entityData.timeSinceAbility + milliseconds) / 200 !=
                (int)creature->m_entityData.timeSinceAbility / 200) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundChannel2;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (!(creature->m_entityData.timeSinceAbility > (float)creature->anticipationTime()))
            break;
        if (!(creature->m_entityData.timeSinceAbility <=
              (float)(creature->anticipationTime() + creature->strikeTime())))
            break;
        int period = (int)((float)creature->strikeTime() * 0.125f);
        if ((int)(creature->m_entityData.timeSinceAbility + milliseconds - (float)creature->anticipationTime()) /
                period ==
            (int)(creature->m_entityData.timeSinceAbility - (float)creature->anticipationTime()) / period)
            break;
        if (!(creature->m_entityData.mp >= creature->manaCost(creature->m_entityData.currentAbility, -1)))
            break;
        creature->m_entityData.mp -= creature->manaCost(creature->m_entityData.currentAbility, -1);
        Creature *target = creatureByID(creature->m_entityData.focusId);
        if (!target)
            target = creature;
        ServerUpdates::HitPacket hit;
        hit.position = target->m_entityData.position;
        float scale = 1.25f - (float)rand() * 0.5f / (float)RAND_MAX;
        hit.damage = creature->spellPower(false) * -2.0f * scale;
        hit.critical = 0;
        hit.attackerId = creature->m_id;
        hit.targetId = target->m_id;
        m_serverUpdates.m_hitPacketList.push_back(hit);
        ServerUpdates::SoundPacket sound;
        sound.position = creature->m_entityData.position;
        sound.speed = 1.0f;
        sound.sound = SoundWatersplash;
        m_serverUpdates.m_soundPacketList.push_back(sound);
        if (m_isClient && (creature != m_localCreature || target != m_localCreature))
            break;
        applyHit(hit);
        break;
    }
    case AbilityHealingStream:
    case AbilityCenteredHealingStream: {
        // A healing aura: without the mana it is skipped to its end. A tick every 200 ms, and sixteen times over the
        // wind-up and strike time a heal for mana of half the spell power on every friendly creature within five
        // blocks of the caster (134: of the caster's hand).
        if (creature->m_entityData.mp < creature->manaCost(creature->m_entityData.currentAbility, -1)) {
            if (creature->m_entityData.timeSinceAbility <
                (float)(creature->anticipationTime() + creature->strikeTime()))
                creature->m_entityData.timeSinceAbility =
                    (float)(creature->anticipationTime() + creature->strikeTime());
        }
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (!(creature->m_entityData.timeSinceAbility <=
              (float)(creature->anticipationTime() + creature->strikeTime())))
            break;
        if ((int)(creature->m_entityData.timeSinceAbility + milliseconds) / 200 !=
            (int)creature->m_entityData.timeSinceAbility / 200) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundChannel2;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        int period = (int)((float)(creature->anticipationTime() + creature->strikeTime()) * 0.0625f);
        if ((int)(creature->m_entityData.timeSinceAbility + milliseconds) / period ==
            (int)creature->m_entityData.timeSinceAbility / period)
            break;
        if (!(creature->m_entityData.mp >= creature->manaCost(creature->m_entityData.currentAbility, -1)))
            break;
        creature->m_entityData.mp -= creature->manaCost(creature->m_entityData.currentAbility, -1);
        for (Creature *other : m_creatures) {
            if (creature->canDoDamage(other) || other->m_entityData.hostilityType == HostilityInteractiveObject)
                continue;
            plasma::Vector<3, Fixed> center = creature->m_entityData.position;
            if (creature->m_entityData.currentAbility == AbilityHealingStream)
                center = center + plasma::Vector<3, Fixed>(creature->m_entityData.attackRotation);
            plasma::Vector<3, float> offset(other->m_entityData.position - center);
            if (!(offset.squaredLength() < 25.0f))
                continue;
            ServerUpdates::HitPacket hit;
            hit.position = other->m_entityData.position;
            float scale = 1.25f - (float)rand() * 0.5f / (float)RAND_MAX;
            hit.damage = creature->spellPower(false) * -0.5f * scale;
            hit.critical = 0;
            hit.attackerId = creature->m_id;
            hit.targetId = other->m_id;
            m_serverUpdates.m_hitPacketList.push_back(hit);
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = 1.0f;
            sound.sound = SoundWatersplash;
            m_serverUpdates.m_soundPacketList.push_back(sound);
            if (m_isClient && (creature != m_localCreature || other != m_localCreature))
                continue;
            applyHit(hit);
        }
        break;
    }
    case AbilityFireBreath: {
        // A beam: without the mana it is skipped to its end. Over the wind-up and strike time a tick every 200 ms and,
        // twenty times, a strike for mana along the aim as far as it reaches, hitting every creature within a radius
        // that widens along the beam (each creature once, m_struckCreatures).
        if (creature->m_entityData.mp < creature->manaCost(creature->m_entityData.currentAbility, -1)) {
            if (creature->m_entityData.timeSinceAbility <
                (float)(creature->anticipationTime() + creature->strikeTime()))
                creature->m_entityData.timeSinceAbility =
                    (float)(creature->anticipationTime() + creature->strikeTime());
        }
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (!(creature->m_entityData.timeSinceAbility <=
              (float)(creature->anticipationTime() + creature->strikeTime())))
            break;
        if ((int)(creature->m_entityData.timeSinceAbility + milliseconds) / 200 !=
            (int)creature->m_entityData.timeSinceAbility / 200) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundChannel2;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        int period = (int)((float)(creature->anticipationTime() + creature->strikeTime()) / 20.0f);
        if ((int)(creature->m_entityData.timeSinceAbility + milliseconds) / period ==
            (int)creature->m_entityData.timeSinceAbility / period)
            break;
        if (!(creature->m_entityData.mp >= creature->manaCost(creature->m_entityData.currentAbility, -1)))
            break;
        creature->m_entityData.mp -= creature->manaCost(creature->m_entityData.currentAbility, -1);
        creature->m_struckCreatures.clear();
        float reach = rayDistance(creature->m_entityData.position, creature->m_entityData.attackRotation.normalized(),
                                  5.0f, false, true, nullptr);
        for (int step = 0; step <= 5; step++) {
            plasma::Vector<3, Fixed> point =
                creature->m_entityData.position +
                plasma::Vector<3, Fixed>(creature->m_entityData.attackRotation.normalized() *
                                         (reach * (float)step / 5.0f));
            float radius = (float)step / 5.0f * 5.0f + 1.0f;
            for (Creature *other : m_creatures) {
                if (!creature->canDoDamage(other) || other->m_entityData.hostilityType == HostilityInteractiveObject)
                    continue;
                if (creature->m_struckCreatures.find(other->m_id) != creature->m_struckCreatures.end())
                    continue;
                plasma::Vector<3, float> offset(other->m_entityData.position - point);
                if (!(offset.squaredLength() < radius * radius))
                    continue;
                ServerUpdates::SoundPacket sound;
                sound.position = creature->m_entityData.position;
                sound.speed = 1.0f;
                sound.sound = SoundFireHit;
                m_serverUpdates.m_soundPacketList.push_back(sound);
                plasma::Vector<3, float> direction = creature->m_entityData.attackRotation.normalized() * 8.0f;
                float roll = (float)rand() / (float)RAND_MAX;
                float damage = creature->damageScale(AbilityFireBreath) * creature->spellPower(false) *
                               creature->m_entityData.timeSinceAbility /
                               (float)(creature->anticipationTime() + creature->strikeTime());
                bool critical = creature->critical(other, true) > roll;
                damage = (1.25f - (float)rand() * 0.5f / (float)RAND_MAX) * damage;
                attackCreature(other, creature, damage, critical, true, 0.05f, point, direction, ElementFire,
                               AbilityNone, nullptr, true);
            }
        }
        break;
    }
    case AbilityBulwark: {
        // The three self-buffs: the buff and a sound as they start.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (creature->m_entityData.timeSinceAbility != 0.0f)
            break;
        applyBuffSkill(creature, BuffBulwark);
        ServerUpdates::SoundPacket sound;
        sound.position = creature->m_entityData.position;
        sound.speed = 1.0f;
        sound.sound = SoundBulwark;
        m_serverUpdates.m_soundPacketList.push_back(sound);
        break;
    }
    case AbilitySwiftness: {
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (creature->m_entityData.timeSinceAbility != 0.0f)
            break;
        applyBuffSkill(creature, BuffSwiftness);
        ServerUpdates::SoundPacket sound;
        sound.position = creature->m_entityData.position;
        sound.speed = 1.0f;
        sound.sound = SoundUpgrade2;
        m_serverUpdates.m_soundPacketList.push_back(sound);
        break;
    }
    case AbilityManaShield: {
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (creature->m_entityData.timeSinceAbility != 0.0f)
            break;
        applyBuffSkill(creature, BuffManashield);
        ServerUpdates::SoundPacket sound;
        sound.position = creature->m_entityData.position;
        sound.speed = 1.0f;
        sound.sound = SoundManashield;
        m_serverUpdates.m_soundPacketList.push_back(sound);
        break;
    }
    case AbilityLavaPuddle:
    case AbilityWaterPuddle: {
        // A spell charged with a tick every 200 ms, cast at the anticipation time as a projectile of ElementFire (98)
        // or ElementWater (99) with twice the spell power.
        if (creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime() &&
            (int)(creature->m_entityData.timeSinceAbility + milliseconds) / 200 !=
                (int)creature->m_entityData.timeSinceAbility / 200) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundChannel2;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (!(creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
              creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()))
            break;
        ServerUpdates::SoundPacket sound;
        sound.position = creature->m_entityData.position;
        sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
        sound.sound = SoundFireball;
        m_serverUpdates.m_soundPacketList.push_back(sound);
        ServerUpdates::BulletPacket bullet;
        bullet.knockback = 0.0f;
        bullet.projectileType = ProjectileGroundSpot;
        bullet.powered = 0;
        bullet.strength = 0;
        float damage = creature->damageScale(creature->m_entityData.currentAbility) * creature->spellPower(false);
        bullet.damage = damage + damage;
        bullet.sizeInBlocks = 8.0f;
        bullet.ownerId = creature->m_id;
        bullet.element = creature->m_entityData.currentAbility == AbilityWaterPuddle ? ElementWater : ElementFire;
        bullet.position = creature->m_entityData.position;
        bullet.visualSize = 1.0f;
        bullet.currentTime = 0;
        bullet.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        m_projectiles.push_back(bullet);
        m_serverUpdates.m_bulletPacketList.push_back(bullet);
        break;
    }
    case AbilityTeleport: {
        // A teleport at the anticipation time, for mana: to where the aim reaches (as far as the aim vector is long),
        // or the free place nearest to that within four blocks sideways and two below to six above; from there the
        // creature sinks to the ground or to water.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (!(creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
              creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()))
            break;
        creature->m_entityData.mp -= creature->manaCost(creature->m_entityData.currentAbility, -1);
        if (creature->m_entityData.mp < 0.0f)
            creature->m_entityData.mp = 0.0f;
        if (!(creature->m_entityData.attackRotation.squaredLength() > 0.0f))
            break;
        float range = sqrtf(creature->m_entityData.attackRotation.squaredLength());
        plasma::Vector<3, float> aim = creature->m_entityData.attackRotation.normalized();
        float reach = rayDistance(creature->m_entityData.position, aim, range, false, true, nullptr);
        plasma::Vector<3, Fixed> destination = creature->m_entityData.position + plasma::Vector<3, Fixed>(reach * aim);
        float nearest = 400.0f;
        plasma::Vector<3, Fixed> place = destination;
        bool ghost = creature->findBuff(BuffSpiritworld) != nullptr;
        for (int x = -4; x <= 4; x++) {
            for (int y = -4; y <= 4; y++) {
                for (int z = -2; z <= 6; z++) {
                    plasma::Vector<3, Fixed> candidate =
                        destination + plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z));
                    if (isBlocked(candidate, creature->m_entityData.appearance.size, ghost, true))
                        continue;
                    float distance = plasma::Vector<3, float>(candidate - destination).squaredLength();
                    if (nearest > distance) {
                        nearest = distance;
                        place = candidate;
                    }
                }
            }
        }
        if (!(nearest < 400.0f))
            break;
        creature->m_entityData.position = place;
        for (;;) {
            plasma::Vector<3, Fixed> below = creature->m_entityData.position - plasma::Vector<3, Fixed>(0, 0, Fixed(1));
            if (isBlocked(below, creature->m_entityData.appearance.size, ghost, true))
                break;
            Fixed feet = creature->m_entityData.position[2] - Fixed(creature->m_entityData.appearance.size[2] * 0.5f);
            if (getBlockAtDot(creature->m_entityData.position[0], creature->m_entityData.position[1], feet - Fixed(1))
                    .type == BlockWater)
                break;
            creature->m_entityData.position[2] -= Fixed(1);
        }
        creature->m_fallStartHeight = (float)creature->m_entityData.position[2];
        break;
    }
    case AbilitySummon:
    case AbilitySpawnerSummon: {
        // Summoning: at the anticipation time, while fewer than three summoned creatures (AppearanceFlagSummoned)
        // follow the caster, a new creature with an id below every other's.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (!(creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
              creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()))
            break;
        long long id = (long long)0x8000000000000000 + 1000;
        int summoned = 0;
        for (Creature *other : m_creatures) {
            if (other->m_entityData.ownerId == creature->m_id &&
                (other->m_entityData.appearance.appearanceFlags & AppearanceFlagSummoned))
                summoned++;
            if (other->m_id < id)
                id = other->m_id;
        }
        if (summoned > 2)
            break;
        id--;
        Creature *summon = new Creature(id);
        if (creature->m_entityData.hostilityType == HostilityEnemy ||
            creature->m_entityData.hostilityType == HostilityAttackableObject)
            summon->m_entityData.hostilityType = HostilityEnemy;
        else
            summon->m_entityData.hostilityType = HostilityNeutral;
        summon->m_entityData.level = creature->m_entityData.level;
        int race = RaceGreenSlime;
        if (!creature->m_summonRaces.empty())
            race = creature->m_summonRaces[rand() % creature->m_summonRaces.size()];
        bool flagged = creature->m_entityData.hostilityType == HostilityAttackableObject ||
                       (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagMarked);
        switch (creature->m_summonAppearanceMode) {
        case SummonCopySummoner:
            summon->m_entityData.race = creature->m_entityData.race;
            summon->m_entityData.equipment = creature->m_entityData.equipment;
            summon->m_entityData.appearance = creature->m_entityData.appearance;
            summon->m_entityData.classType = creature->m_entityData.classType;
            break;
        case SummonCopyOpponent: {
            Creature *model = creatureByID(creature->m_opponentId);
            if (!model)
                model = creature;
            summon->m_entityData.race = model->m_entityData.race;
            summon->m_entityData.equipment = model->m_entityData.equipment;
            summon->m_entityData.appearance = model->m_entityData.appearance;
            summon->m_entityData.classType = model->m_entityData.classType;
            break;
        }
        default:
            summon->m_entityData.race = race;
            fillAppearance(summon->m_entityData.race, summon->m_entityData.appearance, nullptr, flagged);
            equip(summon->m_entityData.hostilityType, summon->m_entityData.race, summon->m_entityData.level,
                  summon->m_entityData.appearance.appearanceFlags, summon->m_entityData.factionKind,
                  summon->m_entityData.classType, summon->m_entityData.specialization, summon->m_entityData.equipment,
                  summon->m_specialSkills, summon->m_summonAppearanceMode, summon->m_summonRaces, summon->m_inventory);
            break;
        }
        summon->m_entityData.appearance.appearanceFlags &= ~AppearanceFlagTinted;
        summon->m_entityData.appearance.appearanceFlags |= AppearanceFlagSummoned;
        summon->m_entityData.level = creature->m_entityData.level;
        summon->m_entityData.hp = summon->maxHP();
        summon->m_entityData.position = creature->m_entityData.position;
        summon->m_entityData.origin = creature->m_entityData.position;
        summon->m_entityData.factionKind = creature->m_entityData.factionKind;
        summon->m_behaviours = creature->m_entityData.currentAbility == AbilitySummon
                                   ? BehaviourFollowOwner | BehaviourUseFurniture | BehaviourWander
                                   : BehaviourRoam;
        summon->m_entityData.ownerId = creature->m_id;
        addCreature(summon);
        break;
    }
    case AbilityShadowClone: {
        // The three crafting-station abilities queue an interaction for the creature: 11 as 143 starts (unless
        // AppearanceFlagSummoned), 13 as 149 passes its anticipation time, 12 eight times over 144's anticipation
        // time.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (creature->m_entityData.appearance.appearanceFlags & AppearanceFlagSummoned)
            break;
        if (creature->m_entityData.timeSinceAbility != 0.0f)
            break;
        Interaction interaction;
        interaction.type = InteractionShadowClone;
        creature->m_interactions.push_back(interaction);
        break;
    }
    case AbilityHeroicShout: {
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (!(creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
              creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()))
            break;
        Interaction interaction;
        interaction.type = InteractionHeroicShout;
        creature->m_interactions.push_back(interaction);
        break;
    }
    case AbilityWaterBubbles: {
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (!(creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime()))
            break;
        int period = creature->anticipationTime() / 8;
        if ((int)(creature->m_entityData.timeSinceAbility + milliseconds) / period ==
            (int)creature->m_entityData.timeSinceAbility / period)
            break;
        Interaction interaction;
        interaction.type = InteractionWaterBubbles;
        creature->m_interactions.push_back(interaction);
        break;
    }
    case AbilityChargedShot:
    case AbilitySalvo:
    case AbilityChargedBoomerang:
    case AbilityFireballCharge:
    case AbilityCrush:
    case AbilitySwirl:
    case AbilityMonsterCharge:
    case AbilityRockFistCharge: {
        // Held abilities tick every 200 ms with the charge's pitch.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if ((int)(creature->m_entityData.timeSinceAbility + milliseconds) / 200 ==
            (int)creature->m_entityData.timeSinceAbility / 200)
            break;
        ServerUpdates::SoundPacket sound;
        sound.position = creature->m_entityData.position;
        sound.speed = creature->m_entityData.charge * 0.5f + 1.0f;
        sound.sound = SoundCharge2;
        sound.sourceId = creature->m_id;
        m_serverUpdates.m_soundPacketList.push_back(sound);
        break;
    }
    case AbilitySupportRay: {
        // A supporting ray (AI::checkSupport aims it at an ally): the charge rises with the time, and every 200 ms a
        // burst in the faction colour leaves the hand and the target is healed by a tenth of its full health (negative
        // damage).
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        creature->m_entityData.charge += milliseconds * 0.0002f;
        if (creature->m_entityData.charge > 1.0f)
            creature->m_entityData.charge = 1.0f;
        if ((int)(creature->m_entityData.timeSinceAbility + milliseconds) / 200 ==
            (int)creature->m_entityData.timeSinceAbility / 200)
            break;
        ServerUpdates::ParticlePacket particles;
        particles.position =
            creature->m_entityData.position + plasma::Vector<3, Fixed>(creature->m_entityData.attackRotation);
        particles.count = 6;
        particles.size = 0.15f;
        particles.type = ParticleGlow;
        // The colour's fourth component is never set: the packet copies it uninitialised.
        plasma::Vector<4, float> color;
        switch (creature->m_entityData.factionKind) {
        case FactionCultOfDoom:
            color[0] = 1.0f;
            color[1] = 0.4f;
            color[2] = 0.3f;
            break;
        case FactionUnholyPact:
            color[0] = 0.2f;
            color[1] = 1.0f;
            color[2] = 0.4f;
            break;
        default:
            color[0] = 0.2f;
            color[1] = 0.6f;
            color[2] = 1.0f;
            break;
        }
        particles.color = color;
        particles.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 10.0f);
        m_serverUpdates.m_particlePacketList.push_back(particles);
        Creature *target = creatureByID(creature->m_entityData.focusId);
        if (!target || !(target->m_entityData.hp > 0.0f))
            break;
        ServerUpdates::HitPacket hit;
        hit.position = target->m_entityData.position;
        hit.damage = target->maxHP() * -0.1f;
        hit.critical = 0;
        hit.attackerId = creature->m_id;
        hit.targetId = target->m_id;
        m_serverUpdates.m_hitPacketList.push_back(hit);
        if (m_isClient && (creature != m_localCreature || target != m_localCreature))
            break;
        m_serverUpdates.m_hitPacketList.push_back(hit);
        applyHit(hit);
        break;
    }
    case AbilityFloat: {
        // A sound every half second, its pitch and volume at random.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if ((int)(creature->m_entityData.timeSinceAbility + milliseconds) / 500 ==
            (int)creature->m_entityData.timeSinceAbility / 500)
            break;
        ServerUpdates::SoundPacket sound;
        sound.position = creature->m_entityData.position;
        sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 0.5f;
        sound.volume = (float)rand() * 0.25f / (float)RAND_MAX + 0.5f;
        sound.sound = SoundChannel2;
        m_serverUpdates.m_soundPacketList.push_back(sound);
        break;
    }
    case AbilityFireBeam:
    case AbilityFireRay:
    case AbilityWaterRay:
    case AbilityWaterBeam: {
        // Casts a ray up to 200 blocks at anticipation time, repeating every 200 ms for abilities 105 and 106. It
        // breaks destructible blocks within four blocks of the endpoint and strikes each living creature along its
        // path with spell power. A miss leaves sparks and ends the combo.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (creature->m_entityData.currentAbility == AbilityBlockStrike) {
            if (creature->m_entityData.timeSinceAbility == 0.0f) {
                creature->m_entityData.mp -= creature->m_entityData.charge;
                if (creature->m_entityData.mp < 0.0f)
                    creature->m_entityData.mp = 0.0f;
                creature->m_strikeStrength = creature->m_entityData.charge;
                creature->m_entityData.charge = 0.0f;
            }
        } else {
            if (creature->m_entityData.currentAbility == AbilitySupportRay) {
                creature->m_entityData.mp -= milliseconds * 0.0002f;
            } else if (creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
                       creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime() &&
                       (creature->m_entityData.currentAbility == AbilityFireRay ||
                        creature->m_entityData.currentAbility == AbilityWaterRay)) {
                creature->m_entityData.mp -= creature->manaCost(creature->m_entityData.currentAbility, -1);
                if (creature->findBuff(BuffInstantfire)) {
                    Creature::Buff buff;
                    buff.type = BuffInstantfire;
                    creature->setBuff(buff, false);
                    ServerUpdates::BuffPacket packet;
                    packet.buff = buff;
                    packet.sourceId = creature->m_id;
                    packet.targetId = creature->m_id;
                    m_serverUpdates.m_buffPacketList.push_back(packet);
                }
            }
            if (creature->m_entityData.mp < 0.0f) {
                creature->m_entityData.mp = 0.0f;
                break;
            }
        }
        if (creature->m_entityData.timeSinceAbility + milliseconds < (float)creature->anticipationTime())
            break;
        if (creature->m_entityData.timeSinceAbility >= (float)(creature->anticipationTime() + creature->strikeTime()))
            break;
        if ((creature->m_entityData.currentAbility == AbilityFireBeam ||
             creature->m_entityData.currentAbility == AbilityWaterBeam) &&
            creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
            creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundMagic02;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        } else if (creature->m_entityData.currentAbility == AbilityBlockStrike &&
                   creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
                   creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundShieldSwing;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        } else if ((creature->m_entityData.currentAbility == AbilityFireRay ||
                    creature->m_entityData.currentAbility == AbilityWaterRay) &&
                   (int)(creature->m_entityData.timeSinceAbility + milliseconds) / 200 !=
                       (int)creature->m_entityData.timeSinceAbility / 200) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundChannel2;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        bool hit = false;
        if (!(creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
              creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime())) {
            // After the first ray only 105, 106 and 29 go on, every 200 ms of the time since the anticipation time.
            if (creature->m_entityData.currentAbility != AbilityFireRay &&
                creature->m_entityData.currentAbility != AbilityWaterRay &&
                creature->m_entityData.currentAbility != AbilitySupportRay)
                break;
            if ((int)(creature->m_entityData.timeSinceAbility - (float)creature->anticipationTime() + milliseconds) /
                    200 ==
                (int)(creature->m_entityData.timeSinceAbility - (float)creature->anticipationTime()) / 200)
                break;
            if (!(creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()))
                break;
        }
        plasma::Vector<3, float> aim = creature->m_entityData.attackRotation;
        if (aim.squaredLength() > 0.0f)
            aim.normalize();
        plasma::Vector<3, Fixed> eye = creature->sightPosition();
        float reach =
            rayDistance(eye, creature->m_entityData.attackRotation.normalized(), 200.0f, false, true, nullptr);
        if (creature->m_entityData.currentAbility == AbilityBlockStrike)
            reach = 10.0f;
        creature->m_struckCreatures.clear();
        creature->m_dodgingCreatures.clear();
        if (reach < 200.0f) {
            plasma::Vector<3, Fixed> impact =
                eye + plasma::Vector<3, Fixed>(creature->m_entityData.attackRotation.normalized() * reach);
            bool broken = false;
            for (int x = -4; x <= 4; x++) {
                for (int y = -4; y <= 4; y++) {
                    for (int z = -4; z <= 4; z++) {
                        plasma::Vector<3, Fixed> point =
                            impact + plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z));
                        if (!(plasma::Vector<3, float>(point - impact).squaredLength() < 16.0f))
                            continue;
                        Block block = getBlockAtDot(point[0], point[1], point[2]);
                        if (!(block.flags & BlockFlagDestructible))
                            continue;
                        // The block bursts in its own colour and is replaced by air.
                        ServerUpdates::ParticlePacket particles;
                        particles.position = point;
                        particles.size = 0.5f;
                        particles.color = plasma::Vector<4, float>((float)block.color[0] * (1.0f / 128.0f),
                                                                   (float)block.color[1] * (1.0f / 128.0f),
                                                                   (float)block.color[2] * (1.0f / 128.0f), 1.0f);
                        particles.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 10.0f);
                        particles.count = 3;
                        m_serverUpdates.m_particlePacketList.push_back(particles);
                        destroyBlock(point[0], point[1], point[2]);
                        CubeChange change;
                        change.position = fixedToBlock(point);
                        change.day = m_state.m_day;
                        change.originalBlock = block;
                        change.newBlock = s_airCube;
                        m_serverUpdates.m_cubeChangeList.push_back(change);
                        broken = true;
                    }
                }
            }
            if (broken) {
                ServerUpdates::SoundPacket sound;
                sound.position = impact;
                sound.speed = (float)rand() * 0.4f / (float)RAND_MAX + 0.5f;
                sound.sound = SoundRoll;
                m_serverUpdates.m_soundPacketList.push_back(sound);
                updateSwath((int)(impact[0] - Fixed(4)), (int)(impact[1] - Fixed(4)), (int)(impact[0] + Fixed(4)),
                            (int)(impact[1] + Fixed(4)));
            }
        }
        if (creature->m_entityData.classType == ClassMage &&
            creature->m_entityData.specialization == SpecializationWater) {
            // This specialization leaves a projectile of type 6 where the ray ends.
            ServerUpdates::BulletPacket bullet;
            bullet.projectileType = ProjectileGroundSpot;
            bullet.powered = 0;
            bullet.knockback = 0.0f;
            bullet.strength = 0.0f;
            bullet.damage = creature->spellPower(false) * 0.1f;
            bullet.sizeInBlocks = 4.0f;
            bullet.ownerId = creature->m_id;
            bullet.element = ElementWater;
            bullet.position =
                eye + plasma::Vector<3, Fixed>(creature->m_entityData.attackRotation.normalized() * reach);
            bullet.visualSize = 1.0f;
            bullet.currentTime = 0;
            bullet.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            m_projectiles.push_back(bullet);
            m_serverUpdates.m_bulletPacketList.push_back(bullet);
        }
        // The eye moves along the ray a block at a time.
        for (int step = 0; reach > (float)step; step++) {
            for (Creature *other : m_creatures) {
                if (other == creature || other->m_entityData.hp <= 0.0f)
                    continue;
                if (creature->m_struckCreatures.find(other->m_id) != creature->m_struckCreatures.end())
                    continue;
                if (other->m_entityData.rollTime != 0.0f)
                    continue;
                float radius = other->m_entityData.appearance.size[0] * 0.5f + 1.0f;
                if (creature->m_entityData.currentAbility == AbilityBlockStrike)
                    radius += (float)step;
                // The horizontal distance is compared in 16.16 fixed point.
                plasma::Vector<3, Fixed> offset = eye - other->m_entityData.position;
                plasma::Vector<2, Fixed> flat(offset[0], offset[1]);
                if (flat[0] * flat[0] + flat[1] * flat[1] >= Fixed(radius * radius))
                    continue;
                float above = (float)(eye[2] - other->m_entityData.position[2]);
                if (!(other->m_entityData.appearance.size[2] * 0.5f + 1.0f > (float)fabs((double)above)))
                    continue;
                creature->m_struckCreatures.insert(other->m_id);
                float roll = (float)rand() / (float)RAND_MAX;
                bool critical = creature->critical(other, true) > roll;
                // The direction to the creature is worked out and not used.
                plasma::Vector<3, float> away =
                    plasma::Vector<3, float>(other->m_entityData.position - creature->m_entityData.position);
                away[2] = 0.0f;
                if (away.squaredLength() > 0.01f)
                    away.normalize();
                away[2] = 0.25f;
                float damage =
                    creature->damageScale(creature->m_entityData.currentAbility) * creature->spellPower(false);
                damage = ((float)rand() * 0.05f / (float)RAND_MAX + 1.0f) * damage;
                if (creature->m_entityData.currentAbility == AbilityBlockStrike) {
                    float power = creature->damageScale(AbilityBlockStrike) * creature->attackPower(false);
                    // Adds a zero term to the random scale here.
                    float scale = 1.25f - (float)rand() * 0.5f / (float)RAND_MAX + 0.0f;
                    damage = scale * (creature->m_strikeStrength * creature->m_strikeStrength * 10.0f * power);
                }
                if (critical)
                    damage += damage;
                if (!creature->canDoDamage(other))
                    continue;
                float stun = 0.0f;
                plasma::Vector<3, float> direction(0.0f, 0.0f, 0.0f);
                if (creature->m_entityData.currentAbility == AbilityFireRay ||
                    creature->m_entityData.currentAbility == AbilityWaterRay)
                    stun = 0.1f;
                if (creature->m_entityData.currentAbility == AbilityBlockStrike) {
                    stun = creature->m_strikeStrength;
                    direction = creature->m_entityData.attackRotation.normalized() * 10.0f;
                    direction[2] = 5.0f;
                }
                int element = ElementPhysical;
                if (creature->m_entityData.currentAbility == AbilityFireBeam ||
                    creature->m_entityData.currentAbility == AbilityFireRay ||
                    creature->m_entityData.currentAbility == AbilitySupportRay)
                    element = ElementFire;
                if (creature->m_entityData.currentAbility == AbilityWaterRay ||
                    creature->m_entityData.currentAbility == AbilityWaterBeam)
                    element = ElementWater;
                if (creature->m_entityData.specialization == Specialization2)
                    element = ElementLeech;
                bool sustained = creature->m_entityData.currentAbility == AbilityFireRay ||
                                 creature->m_entityData.currentAbility == AbilityWaterRay ||
                                 creature->m_entityData.currentAbility == AbilityBlockStrike;
                if (attackCreature(other, creature, damage, critical, sustained, stun, eye, direction, element,
                                   AbilityNone, nullptr, true) &&
                    creature->m_entityData.currentAbility == AbilityFireBeam && creature->hasRandomChance(0.25f)) {
                    // A quarter of 104's hits give the caster buff 9 for thirty seconds.
                    Creature::Buff buff;
                    buff.type = BuffInstantfire;
                    buff.timeRemaining = 30000.0f;
                    creature->setBuff(buff, false);
                    ServerUpdates::BuffPacket packet;
                    packet.buff = buff;
                    packet.sourceId = creature->m_id;
                    packet.targetId = creature->m_id;
                    m_serverUpdates.m_buffPacketList.push_back(packet);
                    ServerUpdates::SoundPacket sound;
                    sound.sound = SoundUpgrade2;
                    sound.position = creature->m_entityData.position;
                    m_serverUpdates.m_soundPacketList.push_back(sound);
                }
                if (creature->m_entityData.classType == ClassMage &&
                    creature->m_entityData.specialization == SpecializationWater &&
                    creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
                    creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()) {
                    ServerUpdates::BulletPacket bullet;
                    bullet.projectileType = ProjectileGroundSpot;
                    bullet.powered = 0;
                    bullet.knockback = 0.0f;
                    bullet.strength = 0.0f;
                    bullet.damage = creature->spellPower(false) * 0.1f;
                    bullet.sizeInBlocks = 4.0f;
                    bullet.ownerId = creature->m_id;
                    bullet.element = ElementWater;
                    bullet.position = eye;
                    bullet.visualSize = 1.0f;
                    bullet.currentTime = 0;
                    bullet.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                    m_projectiles.push_back(bullet);
                    m_serverUpdates.m_bulletPacketList.push_back(bullet);
                }
                hit = true;
            }
            eye += plasma::Vector<3, Fixed>(aim);
        }
        if (hit) {
            creature->m_entityData.hitCombo++;
            creature->onHit();
            creature->m_entityData.timeSinceHit = 0.0f;
        }
        if (creature->m_struckCreatures.size() != 0 || creature->m_entityData.currentAbility == AbilityBlockStrike)
            break;
        // Nothing was hit: sparks where the ray ends, and the combo ends.
        ServerUpdates::ParticlePacket particles;
        particles.position = creature->m_entityData.position +
                             plasma::Vector<3, Fixed>(creature->m_entityData.attackRotation.normalized() * reach);
        particles.count = 5;
        particles.size = 0.1f;
        particles.type = ParticleFire;
        // The colour's fourth component is never set: the packet copies it uninitialised.
        plasma::Vector<4, float> color;
        color[0] = 1.0f;
        color[1] = 0.2f;
        color[2] = 0.5f;
        particles.color = color;
        particles.acceleration = aim * -1.0f;
        m_serverUpdates.m_particlePacketList.push_back(particles);
        float tempo = creature->tempo();
        creature->m_entityData.hitCombo = 0;
        creature->m_entityData.timeSinceAbility *= tempo / creature->tempo();
        break;
    }
    case AbilityScream: {
        // The host only: a sound at the anticipation time.
        if (m_isClient)
            break;
        if (!(creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime() &&
              creature->m_entityData.timeSinceAbility + milliseconds >= (float)creature->anticipationTime()))
            break;
        ServerUpdates::SoundPacket sound;
        sound.position = creature->m_entityData.position;
        sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 0.6f;
        sound.sound = SoundLichScream;
        m_serverUpdates.m_soundPacketList.push_back(sound);
        break;
    }
    case AbilityCarryCreature: {
        // Feeding (the other creature eats, ability 130): a creature cannot be its own target; the host ends the
        // other's meal 50 ms in, and the feeder's sound plays then.
        if (creature->m_entityData.ownerId == creature->m_id) {
            creature->m_entityData.ownerId = 0;
            creature->m_entityData.currentAbility = AbilityNone;
        }
        if (!(creature->m_entityData.timeSinceAbility < 50.0f &&
              creature->m_entityData.timeSinceAbility + milliseconds >= 50.0f))
            break;
        if (!m_isClient) {
            Creature *other = creatureByID(creature->m_entityData.focusId);
            if (other && other->m_entityData.currentAbility == AbilityRestHeal)
                other->m_entityData.currentAbility = AbilityNone;
        }
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        ServerUpdates::SoundPacket sound;
        sound.sound = SoundPickup;
        sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 1.0f;
        sound.position = creature->m_entityData.position;
        sound.sourceId = creature->m_id;
        m_serverUpdates.m_soundPacketList.push_back(sound);
        break;
    }
    case AbilityFeedClimbingPlant: {
        // The host only: feeding a pet. Crumbs fall from the mouth every 400 ms after the first 200 ms of the attack
        // duration; after 1.8 seconds the fed creature, if it is race 302 and not yet fed, starts its ability 153.
        if (m_isClient)
            break;
        if (creature->m_entityData.timeSinceAbility > 200.0f &&
            creature->m_entityData.timeSinceAbility < (float)creature->attackDuration() &&
            (int)(creature->m_entityData.timeSinceAbility + milliseconds) / 400 !=
                (int)creature->m_entityData.timeSinceAbility / 400) {
            plasma::Matrix<float> turn;
            turn.identity();
            turn.rotateZ(creature->m_entityData.rotation[2]);
            plasma::Vector<3, float> mouth(0.0f, creature->m_entityData.appearance.size[1] * 0.6f,
                                           0.1f - creature->m_entityData.appearance.size[2] * 0.5f);
            ServerUpdates::ParticlePacket particles;
            particles.position =
                creature->m_entityData.position + plasma::Vector<3, Fixed>(turn.transformDirection(mouth));
            particles.size = 0.2f;
            particles.color = plasma::Vector<4, float>(0.6f, 0.5f, 0.3f, 1.0f);
            particles.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 5.0f);
            particles.count = 5;
            m_serverUpdates.m_particlePacketList.push_back(particles);
            ServerUpdates::SoundPacket sound;
            sound.sound = SoundPickup;
            sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 0.5f;
            sound.position = creature->m_entityData.position;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        if (!(creature->m_entityData.timeSinceAbility > 1800.0f))
            break;
        Creature *other = creatureByID(creature->m_entityData.ownerId);
        if (!other || other->m_entityData.race != RaceClimbingPlant || other->m_entityData.interactionState != 0)
            break;
        other->m_entityData.interactionState = 1;
        if (other->m_entityData.race == RaceClimbingPlant) {
            other->m_entityData.currentAbility = AbilityGrowClimbingPlant;
            other->m_entityData.timeSinceAbility = 0.0f;
        }
        break;
    }
    case AbilityUseSpecialItem: {
        // The player uses the special item in hand; most act five seconds in. Item 7 near a static entity of type 1
        // (within eight blocks) starts the flight (ability 152) to a building of type 60 among the 21 x 21 cells
        // around the entity; item 4 near an entity of type 0 marks its cell as discovered and counts it in the item's
        // modifier; item 13 switches the entities of type 9 nearby after 200 ms; every other item becomes an
        // interaction of type 2, and item 8 also gives buff 19 for a minute to the player and the mount.
        if (creature != m_localCreature)
            break;
        if (creature->m_entityData.actionItem.category == ItemCategorySpecial &&
            creature->m_entityData.actionItem.id == SpecialItemWhistle &&
            creature->m_entityData.timeSinceAbility < 5000.0f &&
            creature->m_entityData.timeSinceAbility + milliseconds >= 5000.0f) {
            for (int zoneX = -1; zoneX <= 1; zoneX++) {
                for (int zoneY = -1; zoneY <= 1; zoneY++) {
                    Zone *zone = this->zone(zoneCoordinate(creature->m_entityData.position[0]) + zoneX,
                                            zoneCoordinate(creature->m_entityData.position[1]) + zoneY);
                    for (StaticEntity &entity : zone->m_staticEntities) {
                        int cellX = tileCoordinate(entity.position[0]);
                        int cellY = tileCoordinate(entity.position[1]);
                        if (entity.type != StaticEntityBirdStatue ||
                            !(plasma::Vector<3, float>(entity.position - creature->m_entityData.position)
                                  .squaredLength() < 64.0f))
                            continue;
                        creature->m_abilityTargetPosition = plasma::Vector<3, Fixed>(0, 0, 0);
                        for (int x = cellX - 10; x <= cellX + 10; x++) {
                            for (int y = cellY - 10; y <= cellY + 10; y++) {
                                if (zoneBuildingType(x, y) != BuildingFloatingIslandTreasurePavilion)
                                    continue;
                                creature->m_abilityTargetPosition = plasma::Vector<3, Fixed>(
                                    Fixed(x * BlocksPerCell + 24), Fixed(y * BlocksPerCell + 24),
                                    Fixed(structureLevel(x, y, 0)));
                                creature->m_entityData.currentAbility = AbilityFlightToPoint;
                                creature->m_entityData.timeSinceAbility = 0.0f;
                                creature->m_entityData.origin = creature->m_entityData.position;
                            }
                        }
                        ServerUpdates::SoundPacket sound;
                        sound.sound = SoundWhistle2;
                        sound.position = creature->m_entityData.position;
                        m_serverUpdates.m_soundPacketList.push_back(sound);
                        break;
                    }
                }
            }
            break;
        }
        if (creature->m_entityData.actionItem.category == ItemCategorySpecial &&
            creature->m_entityData.actionItem.id == SpecialItemFlute &&
            creature->m_entityData.timeSinceAbility < 5000.0f &&
            creature->m_entityData.timeSinceAbility + milliseconds >= 5000.0f) {
            for (int zoneX = -1; zoneX <= 1; zoneX++) {
                for (int zoneY = -1; zoneY <= 1; zoneY++) {
                    Zone *zone = this->zone(zoneCoordinate(creature->m_entityData.position[0]) + zoneX,
                                            zoneCoordinate(creature->m_entityData.position[1]) + zoneY);
                    for (StaticEntity &entity : zone->m_staticEntities) {
                        int cellX = tileCoordinate(entity.position[0]);
                        int cellY = tileCoordinate(entity.position[1]);
                        if (entity.type != StaticEntityGoddess2 ||
                            !(plasma::Vector<3, float>(entity.position - creature->m_entityData.position)
                                  .squaredLength() < 64.0f))
                            continue;
                        std::array<int, 2> cell = {cellX, cellY};
                        if (m_localCreature->m_playerData->discoveredMarkers.find(cell) !=
                            m_localCreature->m_playerData->discoveredMarkers.end())
                            continue;
                        m_localCreature->m_playerData->discoveredMarkers.insert(std::array<int, 2> {{cellX, cellY}});
                        for (std::vector<ItemStack> &tab : m_localCreature->m_inventory.m_inventoryTabs) {
                            for (ItemStack &stack : tab) {
                                if (stack.quantity != 0 && stack.item.category == ItemCategorySpecial &&
                                    stack.item.id == SpecialItemFlute)
                                    stack.item.modifier++;
                            }
                        }
                        ServerUpdates::SoundPacket sound;
                        sound.sound = SoundFlute2;
                        sound.position = creature->m_entityData.position;
                        m_serverUpdates.m_soundPacketList.push_back(sound);
                    }
                }
            }
            break;
        }
        if (creature->m_entityData.timeSinceAbility == 0.0f) {
            // The item's own sound as the ability starts.
            ServerUpdates::SoundPacket sound;
            if (creature->m_entityData.actionItem.category == ItemCategorySpecial &&
                creature->m_entityData.actionItem.id == SpecialItemFlute) {
                sound.sound = SoundFlute1;
            } else if (creature->m_entityData.actionItem.category == ItemCategorySpecial &&
                       creature->m_entityData.actionItem.id == SpecialItemHarp) {
                sound.sound = SoundHarp1;
            } else if (creature->m_entityData.actionItem.category == ItemCategorySpecial &&
                       creature->m_entityData.actionItem.id == SpecialItemFiddle) {
                sound.sound = SoundFiddle1;
            } else if (creature->m_entityData.actionItem.category == ItemCategorySpecial &&
                       creature->m_entityData.actionItem.id == SpecialItemWhistle) {
                sound.sound = SoundWhistle1;
            } else if (creature->m_entityData.actionItem.category == ItemCategorySpecial &&
                       creature->m_entityData.actionItem.id == SpecialItemBell) {
                sound.sound = SoundBell1;
            } else if (creature->m_entityData.actionItem.category == ItemCategorySpecial &&
                       creature->m_entityData.actionItem.id == SpecialItemLute) {
                sound.sound = SoundLute1;
            } else if (creature->m_entityData.actionItem.category == ItemCategorySpecial &&
                       creature->m_entityData.actionItem.id == SpecialItemNightingale) {
                sound.sound = SoundNightingale1;
            } else {
                sound.sound = SoundDrop;
                sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 1.0f;
            }
            sound.position = creature->m_entityData.position;
            sound.sourceId = creature->m_id;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        if (creature->m_entityData.actionItem.category == ItemCategorySpecial) {
            // The instruments (4..11 but 9) send up a note every 500 ms.
            if (creature->m_entityData.actionItem.id >= SpecialItemFlute &&
                creature->m_entityData.actionItem.id <= SpecialItemNightingale &&
                creature->m_entityData.actionItem.id != SpecialItemTreasureSpirit &&
                creature->m_entityData.timeSinceAbility < 5000.0f &&
                (int)(creature->m_entityData.timeSinceAbility + milliseconds) / 500 !=
                    (int)creature->m_entityData.timeSinceAbility / 500) {
                ServerUpdates::ParticlePacket particles;
                plasma::Matrix<float> turn;
                turn.identity();
                turn.rotateZ(creature->m_entityData.rotation[2]);
                plasma::Vector<3, float> hand(0.0f, creature->m_entityData.appearance.size[1] * 0.6f,
                                              creature->m_entityData.appearance.size[2] * 0.0f);
                particles.position =
                    creature->m_entityData.position + plasma::Vector<3, Fixed>(turn.transformDirection(hand));
                particles.type = ParticleMusicNote;
                particles.size = 1.0f;
                particles.color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
                particles.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f);
                particles.spread = 0.5f;
                particles.count = 1;
                m_serverUpdates.m_particlePacketList.push_back(particles);
            }
            if (creature->m_entityData.actionItem.category == ItemCategorySpecial &&
                creature->m_entityData.actionItem.id == SpecialItemGateKey &&
                creature->m_entityData.timeSinceAbility < 200.0f &&
                creature->m_entityData.timeSinceAbility + milliseconds >= 200.0f) {
                for (int zoneX = -1; zoneX <= 1; zoneX++) {
                    for (int zoneY = -1; zoneY <= 1; zoneY++) {
                        Zone *zone = this->zone(zoneCoordinate(creature->m_entityData.position[0]) + zoneX,
                                                zoneCoordinate(creature->m_entityData.position[1]) + zoneY);
                        for (StaticEntity &entity : zone->m_staticEntities) {
                            if (entity.type != StaticEntityLockedGate || !entity.active)
                                continue;
                            if (!(plasma::Vector<3, float>(entity.position - creature->m_entityData.position)
                                      .squaredLength() < 64.0f))
                                continue;
                            unsigned char before = entity.active;
                            setStaticActive(&entity, false);
                            // The zones whose entities changed, by address.
                            if (before != entity.active)
                                m_zonesWithChangedObjects.insert(zone);
                        }
                    }
                }
                break;
            }
        }
        if (!(creature->m_entityData.timeSinceAbility < 5000.0f &&
              creature->m_entityData.timeSinceAbility + milliseconds >= 5000.0f))
            break;
        Interaction interaction;
        interaction.type = InteractionUseSpecialItem;
        interaction.item = creature->m_entityData.actionItem;
        creature->m_interactions.push_back(interaction);
        if (creature->m_entityData.actionItem.category != ItemCategorySpecial ||
            creature->m_entityData.actionItem.id != SpecialItemBell)
            break;
        ServerUpdates::SoundPacket sound;
        sound.sound = SoundBell2;
        sound.position = creature->m_entityData.position;
        m_serverUpdates.m_soundPacketList.push_back(sound);
        Creature::Buff buff;
        buff.timeRemaining = 60000.0f;
        buff.type = BuffSpiritworld;
        buff.sourceId = 0;
        applyBuff(creature, buff, true);
        Creature *mount = creatureByID(creature->m_petId);
        if (mount)
            applyBuff(mount, buff, true);
        break;
    }
    case AbilityFlightToPoint: {
        // The player's flight to a flight point after two seconds: along a cubic curve from where it started (origin)
        // through 20 blocks above the flight point (m_abilityTargetPosition) over twenty seconds; within eight blocks
        // of the flight point the ability ends.
        if (creature != m_localCreature)
            break;
        if (!(creature->m_entityData.timeSinceAbility > 2000.0f))
            break;
        plasma::Vector<3, Fixed> start = creature->m_entityData.origin;
        plasma::Vector<3, Fixed> destination = creature->m_abilityTargetPosition;
        if (plasma::Vector<3, float>(creature->m_entityData.position - destination).squaredLength() < 64.0f) {
            creature->m_entityData.currentAbility = AbilityNone;
            break;
        }
        float progress = (creature->m_entityData.timeSinceAbility - 2000.0f) / 20000.0f;
        plasma::Vector<3, float> a(0.0f, 0.0f, 0.0f);
        plasma::Vector<3, float> b(0.0f, 0.0f, 0.0f);
        Fixed rise = 20 + destination[2] - start[2];
        b[2] += rise;
        plasma::Vector<3, float> c = plasma::Vector<3, float>(destination - start);
        c[2] += 20.0f;
        plasma::Vector<3, float> d = plasma::Vector<3, float>(destination - start);
        plasma::Vector<3, float> ab = a + (b - a) * progress;
        plasma::Vector<3, float> bc = b + (c - b) * progress;
        plasma::Vector<3, float> cd = c + (d - c) * progress;
        plasma::Vector<3, float> abc = ab + (bc - ab) * progress;
        plasma::Vector<3, float> bcd = bc + (cd - bc) * progress;
        plasma::Vector<3, Fixed> point =
            start + plasma::Vector<3, Fixed>(abc) + plasma::Vector<3, Fixed>((bcd - abc) * progress);
        float factor = blendFactor((double)milliseconds, 0.01f);
        creature->m_entityData.position +=
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(point - creature->m_entityData.position) * factor);
        creature->m_drawnPosition = point;
        creature->m_entityData.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        creature->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        creature->m_entityData.retreat = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        creature->m_fallStartHeight = (float)creature->m_entityData.position[2];
        break;
    }
    case AbilityMenuView: {
        // The player's flight on the eagle: it climbs towards 150 blocks above the land (at least 50 above the
        // terrain) and circles.
        if (creature != m_localCreature)
            break;
        float land = biomeElevationAt((int)creature->m_entityData.position[0], (int)creature->m_entityData.position[1]);
        float terrain =
            height((int)creature->m_entityData.position[0], (int)creature->m_entityData.position[1], 0) + 50.0f;
        float altitude = (std::max)(terrain, land) + 150.0f;
        float factor = blendFactor((double)milliseconds, 0.001f);
        creature->m_entityData.position[2] +=
            ((std::max)(150.0f, altitude) - creature->m_entityData.position[2]) * factor;
        creature->m_entityData.velocity = plasma::Vector<3, float>(10.0f, 5.0f, 0.0f);
        creature->m_entityData.retreat = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        creature->m_fallStartHeight = (float)creature->m_entityData.position[2];
        float angle = creature->m_entityData.timeSinceAbility * 0.001f;
        creature->m_entityData.acceleration =
            creature->m_entityData.velocity + 5.0f * plasma::Vector<3, float>(cosf(angle), sinf(angle), 0.0f);
        break;
    }
    case AbilityEagleFlight: {
        // The player's flight on the eagle to a flight point (destination, in blocks) from where it started (origin),
        // both 200 blocks above the land. The eagle eases out of the start and into the destination, 200 blocks above
        // the land and 50 above the terrain (500 blocks up on a far flight), and sinks to the destination's height
        // over the last tenth. After flightDuration the player is put down there with the reins (special item 1) in
        // hand and ability 120, and the eagle flies on as a projectile of type 8.
        if (creature != m_localCreature)
            break;
        plasma::Vector<3, Fixed> start = creature->m_entityData.origin;
        plasma::Vector<3, Fixed> destination(Fixed(creature->m_entityData.destination[0]),
                                             Fixed(creature->m_entityData.destination[1]), Fixed(0));
        start[2] = Fixed(((std::max)(0.0f, biomeElevationAt((int)start[0], (int)start[1])) + 200.0f));
        destination[2] = Fixed(((std::max)(0.0f, biomeElevationAt((int)destination[0], (int)destination[1])) + 200.0f));
        plasma::Vector<2, int> to((int)destination[0], (int)destination[1]);
        plasma::Vector<2, int> from((int)start[0], (int)start[1]);
        int duration = flightDuration(from, to);
        if (creature->m_entityData.timeSinceAbility >= (float)duration) {
            creature->m_entityData.currentAbility = AbilityNone;
            creature->m_entityData.position[0] = destination[0];
            creature->m_entityData.position[1] = destination[1];
            for (ItemStack &stack : creature->m_inventory.m_inventoryTabs[1]) {
                if (stack.quantity > 0 && stack.item.category == ItemCategorySpecial &&
                    stack.item.id == SpecialItemHangGlider) {
                    creature->m_entityData.currentAbility = AbilityHangGlide;
                    creature->m_entityData.actionItem = stack.item;
                    break;
                }
            }
            if (creature->m_entityData.currentAbility == AbilityNone) {
                creature->m_entityData.currentAbility = AbilityHangGlide;
                creature->m_entityData.actionItem.category = ItemCategorySpecial;
                creature->m_entityData.actionItem.id = SpecialItemHangGlider;
            }
            creature->m_entityData.timeSinceAbility = 0.0f;
            ServerUpdates::BulletPacket bullet;
            bullet.projectileType = ProjectileEagle;
            bullet.knockback = 0.0f;
            bullet.velocity = destination - start;
            bullet.velocity[2] = 0.0f;
            bullet.velocity.normalize();
            bullet.velocity *= 20.0f;
            bullet.currentTime = 0;
            bullet.sizeInBlocks = 0.0f;
            bullet.position = creature->m_entityData.position;
            bullet.visualSize = creature->m_entityData.appearance.size[0] * 1.5f;
            m_projectiles.push_back(bullet);
            m_serverUpdates.m_bulletPacketList.push_back(bullet);
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.sound = SoundEagle;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        } else {
            double progress = (double)creature->m_entityData.timeSinceAbility / (double)(duration - 1);
            progress = 1.0 - progress * progress;
            progress = 1.0 - progress * progress;
            plasma::Vector<3, Fixed> point = start + (destination - start) * Fixed(progress);
            if (progress > 0.5)
                point = destination + (start - destination) * Fixed(1.0 - progress);
            float land = biomeElevationAt((int)point[0], (int)point[1]) + 200.0f;
            float terrain =
                height((int)creature->m_entityData.position[0], (int)creature->m_entityData.position[1], 0) + 50.0f;
            float lowest = 200.0f;
            point[2] = Fixed((std::max)(lowest, (std::max)(terrain, land)));
            if (isFlyingFar())
                point[2] = Fixed(500);
            creature->m_entityData.position[0] = point[0];
            creature->m_entityData.position[1] = point[1];
            Fixed rise = point[2] - creature->m_entityData.position[2];
            creature->m_entityData.position[2] += rise * blendFactor((double)milliseconds, 0.005f);
            if (progress > 0.9f) {
                Fixed descent = destination[2] - creature->m_entityData.position[2];
                creature->m_entityData.position[2] += ((descent * (progress - 0.9f)) / 0.1f);
            }
            creature->m_drawnPosition = creature->m_entityData.position;
            creature->m_entityData.retreat = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            creature->m_fallStartHeight = (float)creature->m_entityData.position[2];
        }
        // The eagle heads for the destination and weaves in a circle.
        plasma::Vector<3, double> heading = plasma::Vector<3, double>(destination - start);
        heading[2] = 0.0;
        heading.normalize();
        heading *= 10.0;
        creature->m_entityData.velocity = heading;
        float angle = creature->m_entityData.timeSinceAbility * 0.001f;
        creature->m_entityData.acceleration =
            creature->m_entityData.velocity + 5.0f * plasma::Vector<3, float>(cosf(angle), sinf(angle), 0.0f);
        break;
    }
    case AbilityThrowCreature: {
        // Throwing the carried creature: a creature cannot carry itself, the throw lasts two seconds, its sound plays
        // 50 ms in, and the host sends the carried creature off to the left of the thrower.
        if (creature->m_entityData.focusId == creature->m_id) {
            creature->m_entityData.focusId = 0;
            creature->m_entityData.currentAbility = AbilityNone;
        }
        if (creature->m_entityData.timeSinceAbility > 2000.0f)
            creature->m_entityData.currentAbility = AbilityNone;
        if (!(creature->m_entityData.timeSinceAbility < 50.0f &&
              creature->m_entityData.timeSinceAbility + milliseconds >= 50.0f))
            break;
        if (!((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)) {
            ServerUpdates::SoundPacket sound;
            sound.sound = SoundSwing;
            sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 2.0f;
            sound.position = creature->m_entityData.position;
            sound.sourceId = creature->m_id;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        if (m_isClient)
            break;
        Creature *other = creatureByID(creature->m_entityData.focusId);
        if (!other)
            break;
        float angle = (float)(creature->m_entityData.rotation[2] * Pi / 180.0 + HalfPi);
        other->m_entityData.retreat =
            creature->m_entityData.velocity + plasma::Vector<3, float>(cosf(angle) * 10.0f, sinf(angle) * 10.0f, 5.0f);
        break;
    }
    case AbilityEatPetFood: {
        // The host only: taming. At the anticipation time, if the pet holds the food of the creature's race, the
        // tameness (buff 7) grows by 0.2; at 1 the creature becomes the pet's owner's companion with a fresh id below
        // every other, hostility 6 and a plain appearance. The hit packet carries the tameness.
        if (m_isClient)
            break;
        if (!(creature->m_entityData.timeSinceAbility < (float)creature->anticipationTime() &&
              creature->m_entityData.timeSinceAbility + milliseconds >= (float)creature->anticipationTime()))
            break;
        Creature *owner = creatureByID(creature->m_opponentId);
        if (!owner)
            break;
        if (owner->m_entityData.actionItem.category != ItemCategoryPetFood ||
            owner->m_entityData.actionItem.id != creature->m_entityData.race)
            break;
        ServerUpdates::SoundPacket sound;
        sound.position = creature->m_entityData.position;
        sound.speed = (float)rand() * 0.2f / (float)RAND_MAX + 0.9f;
        sound.sound = SoundDrink2;
        m_serverUpdates.m_soundPacketList.push_back(sound);
        float tameness = 0.0f;
        for (std::list<Creature::Buff>::iterator it = creature->m_buffs.begin(); it != creature->m_buffs.end(); it++) {
            if (it->type == BuffTameness)
                tameness = it->strength;
        }
        creature->m_entityData.hitCombo++;
        creature->m_entityData.timeSinceHit = 0.0f;
        tameness += 0.2f;
        if (tameness >= 1.0f) {
            tameness = 1.0f;
            m_state.m_deadCreatures.insert(creature->m_id);
            long long id = LocalCreatureIdStart;
            for (Creature *other : m_creatures) {
                if (other->m_id < id)
                    id = other->m_id;
            }
            id--;
            m_creaturesById.erase(creature->m_id);
            creature->m_id = id;
            m_creaturesById[id] = creature;
            creature->m_entityData.hostilityType = HostilityPet;
            creature->m_entityData.appearance = Creature::EntityData::Appearance();
            initCompanion(owner->m_id, creature);
        }
        ServerUpdates::HitPacket hit;
        hit.damage = tameness;
        hit.hitType = HitTypeTameness;
        hit.critical = 0;
        hit.attackerId = creature->m_id;
        hit.targetId = owner->m_id;
        hit.position = creature->m_entityData.position;
        m_serverUpdates.m_hitPacketList.push_back(hit);
        Creature::Buff buff;
        buff.type = BuffTameness;
        buff.timeRemaining = TamenessDuration;
        buff.strength = tameness;
        buff.sourceId = creature->m_entityData.ownerId;
        applyBuff(creature, buff, true);
        break;
    }
    case AbilityShoot:
    case AbilityChargedShotRelease:
    case AbilityThrowBoomerang:
    case AbilityShuriken:
    case AbilityThrowPoisonVial: {
        // A shot: stamina as the ability starts; at the anticipation time the mana (or, for an ability without a mana
        // cost, the charge) is paid, the charge becomes the shot's strength (m_strikeStrength) and a projectile leaves
        // along the aim. Ability 27 shoots a second time 300 ms later if it was charged; 160 drops its projectile on
        // the aimed place from above.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (creature->m_entityData.timeSinceAbility == 0.0f) {
            creature->m_stamina -= creature->staminaCost(creature->m_entityData.currentAbility);
            if (creature->m_stamina < 0.0f)
                creature->m_stamina = 0.0f;
        }
        if (creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
            creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()) {
            if (creature->manaCost(creature->m_entityData.currentAbility, -1) > 0.0f)
                creature->m_entityData.mp -= creature->manaCost(creature->m_entityData.currentAbility, -1);
            else
                creature->m_entityData.mp -= creature->m_entityData.charge;
            if (creature->m_entityData.mp < 0.0f)
                creature->m_entityData.mp = 0.0f;
            creature->m_strikeStrength = creature->m_entityData.charge;
            creature->m_entityData.charge = 0.0f;
        }
        if (!(creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
              creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime())) {
            if (creature->m_entityData.currentAbility != AbilityThrowBoomerang)
                break;
            if (!(creature->m_strikeStrength > 0.0f))
                break;
            if (!(creature->m_entityData.timeSinceAbility <= (float)(creature->anticipationTime() + 300) &&
                  creature->m_entityData.timeSinceAbility + milliseconds > (float)(creature->anticipationTime() + 300)))
                break;
        }
        ServerUpdates::BulletPacket bullet;
        bullet.velocity = 100.0f * creature->m_entityData.attackRotation.normalized();
        bullet.currentTime = 0;
        bullet.sizeInBlocks = 0.5f;
        float strength;
        if (creature->m_entityData.currentAbility == AbilityThrowBoomerang) {
            strength = creature->m_strikeStrength * 0.2f;
            bullet.sizeInBlocks = creature->m_strikeStrength + creature->m_strikeStrength + 2.0f;
        } else {
            strength = creature->m_strikeStrength;
        }
        bullet.knockback = strength;
        if (creature->manaCost(creature->m_entityData.currentAbility, -1) > 0.0f) {
            strength = creature->manaCost(creature->m_entityData.currentAbility, -1);
            bullet.knockback = strength;
        }
        bullet.strength = strength;
        bullet.powered =
            creature->m_strikeStrength > 0.0f || creature->m_entityData.currentAbility == AbilityChargedShotRelease;
        bullet.position =
            creature->m_entityData.position + plasma::Vector<3, Fixed>(milliseconds * 0.001f * bullet.velocity);
        bullet.ownerId = creature->m_id;
        if (bullet.powered && creature->findBuff(BuffInstantshoot)) {
            Creature::Buff buff;
            buff.type = BuffInstantshoot;
            buff.strength = 0.0f;
            creature->setBuff(buff, false);
            ServerUpdates::BuffPacket packet;
            packet.buff = buff;
            packet.sourceId = creature->m_id;
            packet.targetId = creature->m_id;
            m_serverUpdates.m_buffPacketList.push_back(packet);
        }
        if (creature->m_entityData.currentAbility == AbilityShuriken) {
            bullet.projectileType = ProjectileShuriken;
        } else if (creature->m_entityData.currentAbility == AbilityThrowPoisonVial) {
            bullet.projectileType = ProjectileMedicine;
            bullet.element = ElementPoison;
            bullet.visualSize = 1.0f;
            bullet.knockback = 0.0f;
            bullet.sizeInBlocks = 1.0f;
            bullet.powered = 1;
            bullet.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 50.0f);
            bullet.targetPosition =
                creature->m_entityData.position + plasma::Vector<3, Fixed>(creature->m_entityData.attackRotation);
        } else if (creature->m_entityData.currentAbility == AbilityThrowBoomerang) {
            bullet.projectileType = ProjectileBoomerang;
            bullet.visualSize = creature->m_entityData.appearance.weaponScale;
        } else {
            bullet.projectileType =
                creature->m_entityData.classType == ClassMonsterGunner ? ProjectileBullet : ProjectileArrow;
        }
        float chargeFactor = 5.0f;
        if (creature->m_entityData.currentAbility == AbilityThrowBoomerang)
            chargeFactor = 1.0f;
        float unchargedDamageFactor = 1.0f;
        if (bullet.knockback > 0.0f)
            unchargedDamageFactor = 0.1f;
        if (creature->m_entityData.currentAbility == AbilityChargedShotRelease)
            unchargedDamageFactor = 0.0f;
        float damage = creature->damageScale(creature->m_entityData.currentAbility) * creature->attackPower(false);
        // Adds a zero term to the random scale here.
        float scale = 1.25f - (float)rand() * 0.5f / (float)RAND_MAX + 0.0f;
        bullet.damage =
            scale *
            ((creature->m_strikeStrength * creature->m_strikeStrength * chargeFactor + unchargedDamageFactor) * damage);
        m_projectiles.push_back(bullet);
        m_serverUpdates.m_bulletPacketList.push_back(bullet);
        ServerUpdates::SoundPacket sound;
        sound.position = creature->m_entityData.position;
        sound.sourceId = creature->m_id;
        if (creature->m_entityData.currentAbility == AbilityShoot ||
            creature->m_entityData.currentAbility == AbilityChargedShotRelease) {
            float pitch = (float)rand() * 0.5f / (float)RAND_MAX;
            if (bullet.powered) {
                sound.sound = SoundSalvo2;
                sound.speed = pitch + 1.0f;
            } else {
                sound.sound = SoundSwing;
                sound.speed = pitch + 2.0f;
            }
        } else if (creature->m_entityData.currentAbility == AbilityThrowBoomerang ||
                   creature->m_entityData.currentAbility == AbilityThrowPoisonVial) {
            sound.sound = SoundSwing;
            sound.speed = (float)(rand() * 0.51 / (double)RAND_MAX + 0.7f);
        } else {
            sound.sound = SoundSalvo2;
            sound.speed = (float)rand() * 0.5f / (float)RAND_MAX + 1.0f;
        }
        m_serverUpdates.m_soundPacketList.push_back(sound);
        break;
    }
    case AbilityRetreat: {
        // Hiding: full stealth and the stamina as it starts.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (creature->m_entityData.timeSinceAbility != 0.0f)
            break;
        creature->m_entityData.stealth = 1.0f;
        creature->m_stamina -= creature->staminaCost(creature->m_entityData.currentAbility);
        if (creature->m_stamina < 0.0f)
            creature->m_stamina = 0.0f;
        break;
    }
    case AbilitySalvoRelease: {
        // A charged shot: at the anticipation time the charge becomes the shot's strength (m_strikeStrength) and is
        // paid in mana, and buff 10 ends; then four shots over the strike time along the aim, each a projectile whose
        // damage grows with the square of the charge.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
            creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()) {
            creature->m_strikeStrength = creature->m_entityData.charge;
            creature->m_entityData.mp -= creature->m_entityData.charge;
            creature->m_entityData.charge = 0.0f;
            if (creature->findBuff(BuffInstantshoot)) {
                Creature::Buff buff;
                buff.type = BuffInstantshoot;
                buff.strength = 0.0f;
                creature->setBuff(buff, false);
                ServerUpdates::BuffPacket packet;
                packet.buff = buff;
                packet.sourceId = creature->m_id;
                packet.targetId = creature->m_id;
                m_serverUpdates.m_buffPacketList.push_back(packet);
            }
        }
        int period = creature->strikeTime() / 4;
        if (!(creature->m_entityData.timeSinceAbility >= (float)creature->anticipationTime()))
            break;
        if (!(creature->m_entityData.timeSinceAbility < (float)(creature->anticipationTime() + creature->strikeTime())))
            break;
        if ((int)(creature->m_entityData.timeSinceAbility + milliseconds) / period ==
            (int)creature->m_entityData.timeSinceAbility / period)
            break;
        if (!(creature->m_entityData.attackRotation.squaredLength() > 0.0f))
            break;
        float unchargedDamageFactor = 1.0f;
        if (!((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.5f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundSalvo2;
            sound.sourceId = creature->m_id;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        ServerUpdates::BulletPacket bullet;
        bullet.velocity = 150.0f * creature->m_entityData.attackRotation.normalized();
        bullet.currentTime = 0;
        bullet.sizeInBlocks = 0.5f;
        bullet.knockback = creature->m_strikeStrength * 0.25f;
        bullet.strength = 0.0f;
        bullet.powered = 1;
        bullet.position =
            creature->m_entityData.position + plasma::Vector<3, Fixed>(milliseconds * 0.001f * bullet.velocity);
        bullet.ownerId = creature->m_id;
        if (creature->m_entityData.currentAbility == AbilityShuriken)
            bullet.projectileType = ProjectileShuriken;
        else
            bullet.projectileType =
                creature->m_entityData.classType == ClassMonsterGunner ? ProjectileBullet : ProjectileArrow;
        if (creature->m_entityData.currentAbility == AbilitySalvoRelease)
            unchargedDamageFactor = 0.1f;
        float damage = creature->damageScale(creature->m_entityData.currentAbility) * creature->attackPower(false);
        // Adds a zero term to the random scale here.
        float scale = 1.25f - (float)rand() * 0.5f / (float)RAND_MAX + 0.0f;
        bullet.damage =
            scale * ((creature->m_strikeStrength * creature->m_strikeStrength * 5.0f + unchargedDamageFactor) * damage);
        m_projectiles.push_back(bullet);
        m_serverUpdates.m_bulletPacketList.push_back(bullet);
        break;
    }
    case AbilityFireball:
    case AbilityMonsterFirebolt:
    case AbilityFirebolt:
    case AbilityBraceletFirebolt:
    case AbilityWaterbolt:
    case AbilityBraceletWaterbolt:
    case AbilityWaterball:
    case AbilityMonsterWaterbolt:
    case AbilityWaterSalvo:
    case AbilityFireMissiles:
    case Ability_48:
    case AbilityThrowRock: {
        // The magic projectiles: a charging sound every 200 ms until 200 ms before the anticipation time, the mana at
        // that time, and a projectile along the aim then (46..48: three, a third of the strike time apart). The
        // projectile grows with the charge, which is paid in mana as it leaves; 123 and 47 strike with five times the
        // attack power (47 from above the aimed place), the others with the spell power.
        if ((m_isClient || creature->m_entityData.hostilityType == HostilityPlayer) && creature != m_localCreature)
            break;
        if (creature->m_entityData.timeSinceAbility <= (float)(creature->anticipationTime() - 200) &&
            (int)(creature->m_entityData.timeSinceAbility + milliseconds) / 200 !=
                (int)creature->m_entityData.timeSinceAbility / 200) {
            ServerUpdates::SoundPacket sound;
            sound.position = creature->m_entityData.position;
            sound.speed = (float)rand() * 0.25f / (float)RAND_MAX + 1.0f;
            sound.sound = SoundChannel2;
            sound.sourceId = creature->m_id;
            m_serverUpdates.m_soundPacketList.push_back(sound);
        }
        int castTime = creature->anticipationTime();
        if (creature->m_entityData.currentAbility >= AbilityWaterSalvo &&
            creature->m_entityData.currentAbility <= Ability_48) {
            int period = creature->strikeTime() / 3;
            int elapsed =
                (int)(creature->m_entityData.timeSinceAbility + milliseconds - (float)creature->anticipationTime());
            castTime = creature->anticipationTime() + (elapsed - elapsed % period);
            if (castTime < creature->anticipationTime())
                castTime = creature->anticipationTime();
        }
        if (creature->m_entityData.timeSinceAbility <= (float)creature->anticipationTime() &&
            creature->m_entityData.timeSinceAbility + milliseconds > (float)creature->anticipationTime()) {
            if (creature->m_entityData.attackRotation.squaredLength() > 0.0f)
                creature->m_entityData.mp -= creature->manaCost(creature->m_entityData.currentAbility, -1);
        }
        if (!(creature->m_entityData.timeSinceAbility < (float)(creature->anticipationTime() + creature->strikeTime())))
            break;
        if (!(creature->m_entityData.timeSinceAbility <= (float)castTime &&
              creature->m_entityData.timeSinceAbility + milliseconds > (float)castTime))
            break;
        if (!(creature->m_entityData.attackRotation.squaredLength() > 0.0f))
            break;
        ServerUpdates::BulletPacket bullet;
        bullet.velocity = creature->m_entityData.equipment.weaponRight.id == WeaponBracelet
                              ? 100.0f * creature->m_entityData.attackRotation.normalized()
                              : 50.0f * creature->m_entityData.attackRotation.normalized();
        bullet.currentTime = 0;
        bullet.sizeInBlocks = creature->m_entityData.charge * 4.0f + 0.5f;
        bullet.position = creature->m_entityData.position;
        bullet.knockback = creature->m_entityData.charge;
        bullet.visualSize = creature->m_entityData.charge + 0.5f;
        if (creature->m_entityData.race == RaceDragon) {
            bullet.position[1] += Fixed(creature->m_entityData.appearance.size[1]);
            bullet.position[2] += Fixed(creature->m_entityData.appearance.size[2] * 0.5f);
        }
        bullet.ownerId = creature->m_id;
        if (creature->m_entityData.currentAbility == AbilityThrowRock) {
            bullet.projectileType = ProjectileRock;
            float damage = creature->damageScale(creature->m_entityData.currentAbility) * creature->attackPower(false);
            bullet.damage = (1.25f - (float)rand() * 0.5f / (float)RAND_MAX) * damage * 5.0f;
            bullet.visualSize = 4.0f;
            bullet.knockback = 1.0f;
            bullet.powered = 1;
            bullet.velocity = 100.0f * creature->m_entityData.attackRotation.normalized();
        } else if (creature->m_entityData.currentAbility == AbilityFireMissiles) {
            bullet.projectileType = ProjectileFireball;
            float damage = creature->damageScale(creature->m_entityData.currentAbility) * creature->attackPower(false);
            bullet.damage = (1.25f - (float)rand() * 0.5f / (float)RAND_MAX) * damage * 5.0f;
            bullet.visualSize = 3.0f;
            bullet.knockback = 0.5f;
            bullet.sizeInBlocks = 5.0f;
            bullet.powered = 1;
            bullet.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 50.0f);
            bullet.targetPosition =
                creature->m_entityData.position + plasma::Vector<3, Fixed>(creature->m_entityData.attackRotation);
        } else {
            bullet.projectileType = ProjectileSpellBolt;
            if ((creature->m_entityData.currentAbility >= AbilityMonsterFirebolt &&
                 creature->m_entityData.currentAbility <= AbilityBraceletWaterbolt) ||
                creature->m_entityData.currentAbility == AbilityMonsterWaterbolt) {
                float damage =
                    creature->damageScale(creature->m_entityData.currentAbility) * creature->spellPower(false);
                bullet.damage = (1.25f - (float)rand() * 0.5f / (float)RAND_MAX) * damage;
            } else if (creature->m_entityData.currentAbility == AbilityWaterSalvo ||
                       creature->m_entityData.currentAbility == Ability_48) {
                float damage =
                    creature->damageScale(creature->m_entityData.currentAbility) * creature->spellPower(false);
                bullet.damage = (1.25f - (float)rand() * 0.5f / (float)RAND_MAX) * damage;
                bullet.visualSize += 0.5f;
                bullet.knockback = 0.25f;
            } else {
                float damage =
                    creature->damageScale(creature->m_entityData.currentAbility) * creature->spellPower(false);
                bullet.damage = (1.25f - (float)rand() * 0.5f / (float)RAND_MAX) * damage;
                bullet.visualSize += 1.0f;
                bullet.knockback = 1.0f;
            }
        }
        if (creature->m_entityData.classType == ClassMage || creature->m_entityData.classType == ClassMonsterCaster)
            bullet.element = creature->m_entityData.specialization == SpecializationWater ? ElementWater : ElementFire;
        switch (creature->m_entityData.currentAbility) {
        case AbilityFireball:
        case AbilityWaterball:
        case AbilityWaterSalvo:
        case Ability_48:
            bullet.powered = 1;
            break;
        default:
            bullet.powered = 0;
            break;
        }
        m_projectiles.push_back(bullet);
        if (creature->m_entityData.charge > 0.0f) {
            creature->m_entityData.mp -= creature->m_entityData.charge;
            if (creature->m_entityData.mp < 0.0f)
                creature->m_entityData.mp = 0.0f;
            creature->m_entityData.charge = 0.0f;
        }
        if (bullet.powered && creature->findBuff(BuffInstantfire)) {
            Creature::Buff buff;
            buff.type = BuffInstantfire;
            buff.strength = 0.0f;
            creature->setBuff(buff, false);
            ServerUpdates::BuffPacket packet;
            packet.buff = buff;
            packet.sourceId = creature->m_id;
            packet.targetId = creature->m_id;
            m_serverUpdates.m_buffPacketList.push_back(packet);
        }
        m_serverUpdates.m_bulletPacketList.push_back(bullet);
        ServerUpdates::SoundPacket sound;
        sound.position = creature->m_entityData.position;
        sound.speed = 1.0f;
        sound.volume = 0.9f;
        if (bullet.element == ElementWater) {
            sound.sound = SoundWatersplash;
            if (creature->m_entityData.currentAbility >= AbilityWaterbolt &&
                creature->m_entityData.currentAbility <= AbilityMonsterWaterbolt &&
                creature->m_entityData.currentAbility != AbilityWaterball)
                sound.speed = 1.25f;
        } else {
            sound.sound = SoundFireball;
            if (creature->m_entityData.currentAbility >= AbilityMonsterFirebolt &&
                creature->m_entityData.currentAbility <= AbilityBraceletFirebolt)
                sound.speed = 2.0f;
        }
        m_serverUpdates.m_soundPacketList.push_back(sound);
        break;
    }
    default:
        break;
    }
    // The combo ends after four seconds without a hit: the time in the ability is rescaled from the combo's tempo to
    // the plain one and the cooldowns of the skills that are kept at full restart. Then the cooldowns run down.
    if (!m_isClient || creature == m_localCreature) {
        creature->m_entityData.timeSinceHit += milliseconds;
        if (creature->m_entityData.timeSinceHit > 4000.0f) {
            float tempo = creature->tempo();
            creature->m_entityData.hitCombo = 0;
            creature->m_entityData.timeSinceAbility *= tempo / creature->tempo();
            for (std::map<int, float>::iterator it = creature->m_skillCooldowns.begin();
                 it != creature->m_skillCooldowns.end(); ++it) {
                if (creature->keepsCooldown(it->first))
                    it->second = (float)creature->cooldown(it->first);
            }
        }
    }
    for (std::map<int, float>::iterator it = creature->m_skillCooldowns.begin(); it != creature->m_skillCooldowns.end();
         ++it) {
        if (!creature->keepsCooldown(it->first)) {
            it->second -= milliseconds;
            if (it->second < 0.0f)
                it->second = 0.0f;
        }
    }
}

// Process dead non-player creatures: death bursts and blast damage, revive owned pets after twenty seconds, and remove
// other corpses after one second.
void World::despawnDeadCreatures(float milliseconds)
{
    std::list<Creature *> removed;
    for (Creature *creature : m_creatures) {
        if (!(creature->m_entityData.hp <= 0.0f) || creature->m_entityData.hostilityType == HostilityPlayer)
            continue;
        m_state.m_deadCreatures.insert(creature->m_id);
        int race = creature->m_entityData.race;
        if (creature->m_entityData.timeSinceAbility == 0.0f &&
            (race == RaceWaterBubble || race == RaceBomb || race == RaceBarrelTnt)) {
            ServerUpdates::ParticlePacket burst;
            burst.position = creature->m_entityData.position;
            burst.count = 20;
            burst.size = 0.25f;
            burst.color = race == RaceWaterBubble ? plasma::Vector<4, float>(0.0f, 0.5f, 1.0f)
                                                  : plasma::Vector<4, float>(1.0f, 0.2f, 0.5f);
            burst.type = race == RaceWaterBubble ? ParticleGlow : ParticleFire;
            burst.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 20.0f);
            m_serverUpdates.m_particlePacketList.push_back(burst);
            if (creature->m_entityData.race == RaceBomb) {
                bool destroyed = false;
                for (int x = -8; x <= 8; ++x) {
                    for (int y = -8; y <= 8; ++y) {
                        for (int z = -8; z <= 8; ++z) {
                            // Shifts signed negative block offsets.
                            plasma::Vector<3, Fixed> position = creature->m_entityData.position +
                                                                plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(z));
                            plasma::Vector<3, Fixed> delta = position - creature->m_entityData.position;
                            plasma::Vector<3, float> offset(delta);
                            if (!(offset.squaredLength() < 64.0f))
                                continue;
                            Block block = getBlockAtDot(position[0], position[1], position[2]);
                            if (!(block.flags & (BlockFlagDestructible | BlockFlagBombable)))
                                continue;
                            ServerUpdates::ParticlePacket debris;
                            debris.position = position + plasma::Vector<3, Fixed>(Fixed(0.5), Fixed(0.5), Fixed(0.5));
                            debris.size = 0.5f;
                            debris.color = plasma::Vector<4, float>(block.color[0] / 128.0f, block.color[1] / 128.0f,
                                                                    block.color[2] / 128.0f, 1.0f);
                            debris.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 10.0f);
                            debris.count = 3;
                            debris.type = ParticleDebris;
                            m_serverUpdates.m_particlePacketList.push_back(debris);
                            destroyBlock(position[0], position[1], position[2]);
                            CubeChange change;
                            change.position = fixedToBlock(position);
                            change.day = m_state.m_day;
                            change.originalBlock = block;
                            change.newBlock = s_airCube;
                            m_serverUpdates.m_cubeChangeList.push_back(change);
                            destroyed = true;
                        }
                    }
                }
                if (destroyed) {
                    ServerUpdates::SoundPacket sound;
                    sound.position = creature->m_entityData.position;
                    sound.speed = (float)rand() * 0.4f / (float)RAND_MAX + 0.5f;
                    sound.sound = SoundBlade1;
                    m_serverUpdates.m_soundPacketList.push_back(sound);
                    updateSwath((int)(creature->m_entityData.position[0] - Fixed(8)),
                                (int)(creature->m_entityData.position[1] - Fixed(8)),
                                (int)(creature->m_entityData.position[0] + Fixed(8)),
                                (int)(creature->m_entityData.position[1] + Fixed(8)));
                }
            }
            // The blast centre, copied once for the distance tests below.
            plasma::Vector<3, Fixed> position = creature->m_entityData.position;
            for (Creature *target : m_creatures) {
                if (target == creature || !creature->canDoDamage(target) || target->m_entityData.hp <= 0.0f ||
                    target->m_entityData.rollTime != 0.0f ||
                    creature->m_struckCreatures.find(target->m_id) != creature->m_struckCreatures.end())
                    continue;
                if (distanceExceeds2D(position, target->m_entityData.position,
                                      target->m_entityData.appearance.size[0] * 0.7f + 8.0f))
                    continue;
                float dz = (float)fabs((double)(float)(position[2] - target->m_entityData.position[2]));
                if (!(dz < target->m_entityData.appearance.size[2] * 0.5f + creature->m_entityData.appearance.size[2] +
                               8.0f))
                    continue;
                plasma::Vector<3, float> direction(
                    (float)(target->m_entityData.position[0] - creature->m_entityData.position[0]),
                    (float)(target->m_entityData.position[1] - creature->m_entityData.position[1]), 0.0f);
                if (direction.squaredLength() > 0.01f)
                    direction.normalize();
                bool critical = rand() % 4 == 0;
                direction = plasma::Vector<3, float>(direction[0] * 10.0f, direction[1] * 10.0f, 2.5f);
                float damage = creature->m_entityData.blockPower;
                if (critical)
                    damage += damage;
                creature->m_struckCreatures.insert(target->m_id);
                if (target->m_entityData.hostilityType == HostilityInteractiveObject)
                    continue;
                if (creature->m_entityData.race == RaceWaterBubble) {
                    damage *= creature->m_entityData.mp;
                    if (!(damage > 0.0f))
                        continue;
                    Creature *owner = creatureByID(creature->m_entityData.ownerId);
                    if (!owner)
                        continue;
                    if (!owner->canDoDamage(target)) {
                        if (target->m_entityData.hostilityType != HostilityAttackableObject &&
                            target->m_entityData.hp > 0.0f) {
                            ServerUpdates::HitPacket hit;
                            hit.position = target->m_entityData.position;
                            hit.damage = -damage;
                            hit.critical = critical;
                            hit.attackerId = creature->m_entityData.ownerId;
                            hit.targetId = target->m_id;
                            m_serverUpdates.m_hitPacketList.push_back(hit);
                            if (!m_isClient || (creature->m_entityData.ownerId == m_localCreature->m_id &&
                                                target == m_localCreature)) {
                                m_serverUpdates.m_hitPacketList.push_back(hit);
                                applyHit(hit);
                            }
                        }
                    } else {
                        attackCreature(target, owner, damage, critical, true, 0.0f, creature->m_entityData.position,
                                       direction, ElementWater, AbilityNone, nullptr, true);
                    }
                } else {
                    attackCreature(target, creature, damage, critical, true, 0.0f, creature->m_entityData.position,
                                   direction, ElementFire, AbilityNone, nullptr, true);
                }
            }
        }
        creature->m_entityData.timeSinceAbility += milliseconds;
        if (creature->m_entityData.ownerId && creature->m_entityData.hostilityType == HostilityPet) {
            if (creatureByID(creature->m_entityData.ownerId) && creature->m_entityData.timeSinceAbility >= 20000.0f &&
                creatureByID(creature->m_entityData.ownerId)->m_entityData.hp >= 0.0f) {
                creature->m_entityData.hp = creature->maxHP();
                creature->m_entityData.position = creatureByID(creature->m_entityData.ownerId)->m_entityData.position;
                creature->m_opponentId = 0;
                creature->m_aggro.clear();
            }
        } else if (!(creature->m_entityData.timeSinceAbility < 1000.0f)) {
            removed.push_back(creature);
            for (Creature *other : m_creatures) {
                other->m_aggro.erase(creature->m_id);
                other->m_hitCounts.erase(creature->m_id);
            }
        }
    }
    for (Creature *creature : removed) {
        if (creature) {
            for (std::list<Creature *>::iterator it = m_creatures.begin(); it != m_creatures.end(); ++it) {
                if (*it == creature) {
                    m_creatures.erase(it);
                    break;
                }
            }
            m_creaturesById.erase(creature->m_id);
            delete creature;
        }
    }
}

// Advances projectile motion, contact damage and terrain impacts, then removes expired projectiles.
void World::updateStepProjectiles(float milliseconds)
{
    typedef std::list<ServerUpdates::BulletPacket>::iterator ProjectileIterator;
    std::list<ProjectileIterator> expired;
    for (ProjectileIterator iterator = m_projectiles.begin(); iterator != m_projectiles.end(); ++iterator) {
        ServerUpdates::BulletPacket &projectile = *iterator;
        std::map<long long, Creature *>::iterator ownerEntry = m_creaturesById.find(projectile.ownerId);
        Creature *owner = ownerEntry == m_creaturesById.end() ? nullptr : ownerEntry->second;
        if (projectile.projectileType != ProjectileSpellBolt && projectile.projectileType != ProjectileEagle &&
            projectile.projectileType != ProjectileSteelEmpireAirship) {
            if (projectile.projectileType == ProjectileFireball) {
                float x = (float)(projectile.targetPosition[0] - projectile.position[0]);
                float y = (float)(projectile.targetPosition[1] - projectile.position[1]);
                float z = (float)(projectile.targetPosition[2] - projectile.position[2]);
                float squared = y * y + x * x + z * z;
                if (squared > 1.0f) {
                    float inverse = 1.0f / sqrtf(squared);
                    x *= inverse;
                    y *= inverse;
                    z *= inverse;
                }
                float oldX = projectile.velocity[0];
                float deltaY = y * 50.0f - projectile.velocity[1];
                float deltaZ = z * 50.0f - projectile.velocity[2];
                float factor = blendFactor(milliseconds, 0.002f);
                projectile.velocity[0] = oldX + factor * (x * 50.0f - oldX);
                projectile.velocity[1] += factor * deltaY;
                projectile.velocity[2] += factor * deltaZ;
                if (projectile.currentTime > 500.0f) {
                    x = (float)(projectile.targetPosition[0] - projectile.position[0]) - 0.0f;
                    y = (float)(projectile.targetPosition[1] - projectile.position[1]) - 0.0f;
                    z = (float)(projectile.targetPosition[2] - projectile.position[2]) - 0.0f;
                    factor = blendFactor(milliseconds, 0.005f);
                    projectile.position[0] += Fixed(factor * x + 0.0f);
                    projectile.position[1] += Fixed(factor * y + 0.0f);
                    projectile.position[2] += Fixed(factor * z + 0.0f);
                }
            } else if (projectile.projectileType == ProjectileMedicine) {
                float x = (float)(projectile.targetPosition[0] - projectile.position[0]);
                float y = (float)(projectile.targetPosition[1] - projectile.position[1]);
                float z = (float)(projectile.targetPosition[2] - projectile.position[2]);
                float squared = x * x + y * y + z * z;
                if (squared > 1.0f) {
                    float inverse = 1.0f / sqrtf(squared);
                    x *= inverse;
                    y *= inverse;
                    z *= inverse;
                }
                float oldX = projectile.velocity[0];
                float deltaY = y * 20.0f - projectile.velocity[1];
                float deltaZ = z * 20.0f - projectile.velocity[2];
                float factor = blendFactor(milliseconds, 0.002f);
                projectile.velocity[0] = oldX + (x * 20.0f - oldX) * factor;
                projectile.velocity[1] += deltaY * factor;
                projectile.velocity[2] += deltaZ * factor;
                if (projectile.currentTime > 500.0f) {
                    x = (float)(projectile.targetPosition[0] - projectile.position[0]) - 0.0f;
                    y = (float)(projectile.targetPosition[1] - projectile.position[1]) - 0.0f;
                    z = (float)(projectile.targetPosition[2] - projectile.position[2]) - 0.0f;
                    factor = blendFactor(milliseconds, 0.005f);
                    projectile.position[0] += Fixed(x * factor + 0.0f);
                    projectile.position[1] += Fixed(y * factor + 0.0f);
                    projectile.position[2] += Fixed(z * factor + 0.0f);
                }
            } else if (projectile.projectileType == ProjectileBoomerang) {
                if (!owner || projectile.currentTime > 1200.0f) {
                    expired.push_back(iterator);
                    continue;
                }
                if (projectile.currentTime > 1100.0f) {
                    Fixed fraction = Fixed((projectile.currentTime - 1100.0f) / 100.0f);
                    projectile.position += (owner->m_entityData.position - projectile.position) * fraction * fraction;
                } else if (projectile.currentTime > 800.0f) {
                    projectile.velocity =
                        plasma::Vector<3, float>(owner->m_entityData.position - projectile.position) * 10.0f;
                } else {
                    plasma::Vector<3, float> desired;
                    desired[0] = (float)(Fixed((long long)(int)(owner->m_entityData.attackRotation[0] *
                                                                (float)FixedUnitsPerBlock)) +
                                         owner->m_entityData.position[0] - projectile.position[0]) *
                                 5.0f;
                    desired[1] = (float)(Fixed(owner->m_entityData.attackRotation[1]) +
                                         owner->m_entityData.position[1] - projectile.position[1]) *
                                 5.0f;
                    desired[2] = (float)(Fixed(owner->m_entityData.attackRotation[2]) +
                                         owner->m_entityData.position[2] - projectile.position[2]) *
                                 5.0f;
                    blend(projectile.velocity, desired, milliseconds, 0.05f);
                }
            } else {
                projectile.velocity[2] -= milliseconds * 0.001f * 30.0f * 0.25f;
            }
        }
        float frameSeconds = milliseconds * 0.001f;
        float deltaX = projectile.velocity[0] * frameSeconds;
        float deltaY = projectile.velocity[1] * frameSeconds;
        float deltaZ = projectile.velocity[2] * frameSeconds;
        projectile.currentTime += milliseconds;
        if (projectile.projectileType != ProjectileBoomerang && projectile.currentTime > (float)projectile.timeToLive) {
            if (projectile.projectileType != ProjectileGroundSpot && creatureByID(projectile.ownerId)) {
                float hasteBefore = creatureByID(projectile.ownerId)->tempo();
                creatureByID(projectile.ownerId)->m_entityData.hitCombo = 0;
                Creature *creature = creatureByID(projectile.ownerId);
                creature->m_entityData.timeSinceAbility =
                    (hasteBefore / creatureByID(projectile.ownerId)->tempo()) * creature->m_entityData.timeSinceAbility;
            }
            expired.push_back(iterator);
            continue;
        }
        if (projectile.projectileType == ProjectileSteelEmpireAirship) {
            projectile.position[0] += Fixed(deltaX);
            projectile.position[1] += Fixed(deltaY);
            projectile.position[2] += Fixed(deltaZ);
            float fraction = (projectile.currentTime + projectile.currentTime) / (float)projectile.timeToLive;
            float arc = 1.0f - fraction;
            arc *= arc;
            arc *= arc;
            float ground = (std::max)(
                0.0f, height((int)projectile.position[0], (int)projectile.position[1], HeightFlagSkipPlaceShape));
            Fixed difference = Fixed(arc * 1000.0f) + Fixed(150) + Fixed(ground) - projectile.position[2];
            projectile.position[2] += difference * blendFactor(milliseconds, 0.01f);
            continue;
        }
        if (projectile.projectileType == ProjectileEagle) {
            projectile.position[0] += Fixed(deltaX);
            projectile.position[1] += Fixed(deltaY);
            projectile.position[2] += Fixed(deltaZ);
            continue;
        }
        int steps = (int)(sqrtf(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ) * 2.0f + 1.0f);
        deltaX /= (float)steps;
        deltaY /= (float)steps;
        deltaZ /= (float)steps;
        float directionX = projectile.velocity[0];
        float directionY = projectile.velocity[1];
        float squaredSpeed =
            directionX * directionX + directionY * directionY + projectile.velocity[2] * projectile.velocity[2];
        if (squaredSpeed > 0.0f) {
            float inverse = 1.0f / sqrtf(squaredSpeed);
            directionX *= inverse;
            directionY *= inverse;
        }
        plasma::Vector<3, float> knockback(directionX * (projectile.knockback * 5.0f),
                                           directionY * (projectile.knockback * 5.0f), projectile.knockback * 3.0f);
        for (int step = 0; step < steps; ++step) {
            bool contact = false;
            bool successful = false;
            Creature *contactCreature = nullptr;
            for (std::list<Creature *>::iterator entry = m_creatures.begin(); entry != m_creatures.end(); ++entry) {
                Creature *target = *entry;
                Creature::EntityData &data = target->m_entityData;
                if (data.hp <= 0.0f || projectile.projectileType == ProjectileFireball ||
                    projectile.projectileType == ProjectileMedicine ||
                    ((projectile.projectileType != ProjectileGroundSpot || projectile.element != ElementWater) &&
                     owner && (target == owner || !owner->canDoDamage(target))))
                    continue;
                float halfX = data.appearance.size[0] * 0.5f;
                float halfY = data.appearance.size[1] * 0.5f;
                float halfZ = data.appearance.size[2] * 0.5f;
                Fixed radius = Fixed(projectile.sizeInBlocks);
                if (projectile.position[0] + radius < data.position[0] - halfX ||
                    projectile.position[1] + radius < data.position[1] - halfY ||
                    projectile.position[2] + radius < data.position[2] - halfZ ||
                    projectile.position[0] - radius >= data.position[0] + halfX ||
                    projectile.position[1] - radius >= data.position[1] + halfY ||
                    projectile.position[2] - radius >= data.position[2] + halfZ ||
                    !inRange(projectile.position, data.position, true, 200.0f, target)) {
                    // A creature the projectile does not touch still ends the search once an earlier one was struck.
                    if (contact)
                        break;
                    continue;
                }
                if (data.rollTime != 0.0f) {
                    if (owner && owner->canDoDamage(target))
                        owner->m_dodgingCreatures.insert(target->m_id);
                    continue;
                }
                if (!m_isClient &&
                    (projectile.element == ElementQuicksand || projectile.element == ElementFire ||
                     projectile.element == ElementPoison) &&
                    projectile.projectileType == ProjectileGroundSpot) {
                    if (projectile.element != ElementPoison || !target->findBuff(BuffPoisoned)) {
                        Creature::Buff buff;
                        if (projectile.element == ElementPoison) {
                            buff.type = BuffPoisoned;
                            buff.strength = projectile.damage;
                            buff.timeRemaining = ProjectilePoisonDuration;
                        } else {
                            buff.type = BuffDefenseWeakened;
                            buff.timeRemaining = WeakenDuration;
                        }
                        target->setBuff(buff, true);
                        ServerUpdates::BuffPacket packet;
                        if (owner)
                            packet.sourceId = owner->m_id;
                        packet.targetId = target->m_id;
                        packet.buff = buff;
                        m_serverUpdates.m_buffPacketList.push_back(packet);
                    }
                    if ((int)projectile.currentTime / 500 != (int)(projectile.currentTime - milliseconds) / 500) {
                        ServerUpdates::ParticlePacket particle;
                        particle.position = data.position;
                        particle.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 5.0f);
                        particle.size = 0.4f;
                        particle.count = 15;
                        if (projectile.element == ElementFire) {
                            particle.type = ParticleFire;
                            particle.color = plasma::Vector<4, float>(1.0f, 0.5f, 0.3f, 1.0f);
                        } else if (projectile.element == ElementPoison) {
                            particle.type = ParticleGlow;
                            particle.color = plasma::Vector<4, float>(0.2f, 1.0f, 0.1f, 1.0f);
                        } else {
                            particle.color = plasma::Vector<4, float>(0.6f, 0.5f, 0.3f, 1.0f);
                        }
                        m_serverUpdates.m_particlePacketList.push_back(particle);
                        ServerUpdates::SoundPacket sound;
                        if (projectile.element == ElementQuicksand)
                            sound.sound = SoundPickup;
                        else if (projectile.element == ElementPoison)
                            sound.sound = SoundWatersplashHit;
                        sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 0.8f;
                        sound.position = data.position;
                        m_serverUpdates.m_soundPacketList.push_back(sound);
                    }
                }
                if (projectile.element == ElementQuicksand)
                    continue;
                if (((projectile.projectileType == ProjectileArrow || projectile.projectileType == ProjectileBullet ||
                      projectile.projectileType == ProjectileShuriken ||
                      projectile.projectileType == ProjectileSpellBolt || projectile.projectileType == ProjectileRock ||
                      projectile.projectileType == ProjectileFireball) &&
                     !projectile.powered) ||
                    (projectile.projectileType == ProjectileGroundSpot &&
                     (int)projectile.currentTime / 500 != (int)(projectile.currentTime - milliseconds) / 500) ||
                    (projectile.currentTime > 300.0f && projectile.projectileType == ProjectileBoomerang &&
                     (int)projectile.currentTime / 100 != (int)(projectile.currentTime - milliseconds) / 100)) {
                    if (projectile.projectileType != ProjectileGroundSpot &&
                        projectile.projectileType != ProjectileBoomerang)
                        contact = true;
                    if (!projectile.powered && projectile.projectileType != ProjectileGroundSpot &&
                        projectile.projectileType != ProjectileRock &&
                        projectile.projectileType != ProjectileSpellBolt && owner) {
                        int random = rand();
                        float gain = (1.0f - (float)(random + random) / (float)RAND_MAX) * 0.05f + 0.1f;
                        if (projectile.projectileType == ProjectileBoomerang)
                            gain *= 0.2f;
                        gain *= (float)owner->attackDuration() / 300.0f;
                        float mana = owner->manaGeneration() * gain;
                        if (rand() % 8 == 0)
                            mana += mana;
                        if (!m_isClient || owner == m_localCreature) {
                            owner->m_entityData.mp += mana;
                            if (owner->m_entityData.mp > 1.0f)
                                owner->m_entityData.mp = 1.0f;
                        }
                    }
                    bool critical = false;
                    if (owner) {
                        float random = (float)rand() / (float)RAND_MAX;
                        critical = owner->critical(target, projectile.element != ElementPhysical) > random;
                    }
                    float damage = projectile.damage;
                    if (critical)
                        damage += damage;
                    if (owner && (owner == m_localCreature || (!m_isClient && owner->m_entityData.hostilityType))) {
                        if (owner->hasRandomChance(InstantShootChance) && !projectile.powered) {
                            if (owner->m_entityData.classType == ClassRanger &&
                                owner->m_entityData.specialization == SpecializationScout) {
                                Creature::Buff buff;
                                buff.type = BuffInstantshoot;
                                buff.timeRemaining = InstantShootDuration;
                                owner->setBuff(buff, false);
                                ServerUpdates::BuffPacket packet;
                                packet.sourceId = owner->m_id;
                                packet.targetId = owner->m_id;
                                packet.buff = buff;
                                m_serverUpdates.m_buffPacketList.push_back(packet);
                                ServerUpdates::SoundPacket sound;
                                sound.position = owner->m_entityData.position;
                                sound.sound = SoundUpgrade2;
                                m_serverUpdates.m_soundPacketList.push_back(sound);
                            }
                            if (projectile.projectileType == ProjectileSpellBolt && projectile.element == ElementFire) {
                                Creature::Buff buff;
                                buff.type = BuffInstantfire;
                                buff.timeRemaining = 30000.0f;
                                owner->setBuff(buff, false);
                                ServerUpdates::BuffPacket packet;
                                packet.sourceId = owner->m_id;
                                packet.targetId = owner->m_id;
                                packet.buff = buff;
                                m_serverUpdates.m_buffPacketList.push_back(packet);
                                ServerUpdates::SoundPacket sound;
                                sound.position = owner->m_entityData.position;
                                sound.sound = SoundUpgrade2;
                                m_serverUpdates.m_soundPacketList.push_back(sound);
                            }
                        }
                    }
                    damage = damage < 0.0f ? 0.0f : damage;
                    if (owner && owner->m_entityData.hostilityType == HostilityPlayer && owner != m_localCreature)
                        continue;
                    if (projectile.element == ElementWater && owner && !owner->canDoDamage(target) &&
                        data.hostilityType != HostilityAttackableObject &&
                        data.hostilityType != HostilityInteractiveObject) {
                        ServerUpdates::HitPacket hit;
                        hit.attackerId = projectile.ownerId;
                        hit.targetId = target->m_id;
                        hit.damage = -damage;
                        hit.critical = critical;
                        hit.position = data.position;
                        m_serverUpdates.m_hitPacketList.push_back(hit);
                        if (!m_isClient || (owner == m_localCreature && target == m_localCreature)) {
                            m_serverUpdates.m_hitPacketList.push_back(hit);
                            applyHit(hit);
                        }
                    } else if (projectile.element != ElementWater) {
                        contactCreature = target;
                        if (attackCreature(target, owner, damage, critical, projectile.powered, projectile.knockback,
                                           projectile.position, knockback, projectile.element, AbilityNone, &projectile,
                                           true))
                            successful = true;
                    }
                    float slowedTime = (float)projectile.slowedTime;
                    if (slowedTime > data.slowedTime)
                        data.slowedTime = slowedTime;
                }
                if (projectile.projectileType == ProjectileArrow || projectile.projectileType == ProjectileBullet ||
                    projectile.projectileType == ProjectileShuriken ||
                    projectile.projectileType == ProjectileMedicine ||
                    projectile.projectileType == ProjectileSpellBolt ||
                    projectile.projectileType == ProjectileFireball || projectile.projectileType == ProjectileRock) {
                    contact = true;
                    break;
                }
                if (contact)
                    break;
            }
            if (projectile.projectileType == ProjectileGroundSpot)
                continue;
            if (projectile.projectileType == ProjectileBoomerang &&
                (int)projectile.currentTime / 100 != (int)(projectile.currentTime - milliseconds) / 100) {
                ServerUpdates::SoundPacket sound;
                sound.position = projectile.position;
                sound.sound = SoundSwing;
                sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 1.5f;
                sound.volume = 0.7f;
                m_serverUpdates.m_soundPacketList.push_back(sound);
            }
            if (projectile.projectileType == ProjectileMedicine &&
                (int)projectile.currentTime / 200 != (int)(projectile.currentTime - milliseconds) / 200) {
                ServerUpdates::SoundPacket sound;
                sound.position = projectile.position;
                sound.sound = SoundSwing;
                sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 1.0f;
                sound.volume = 0.7f;
                m_serverUpdates.m_soundPacketList.push_back(sound);
            }
            Fixed movedX(deltaX);
            Fixed movedY(deltaY);
            Fixed movedZ(deltaZ);
            projectile.position[0] += movedX;
            projectile.position[1] += movedY;
            projectile.position[2] += movedZ;
            bool wall = false;
            if (projectile.projectileType != ProjectileFireball && projectile.projectileType != ProjectileMedicine) {
                if (getBlockAtDot(projectile.position[0], projectile.position[1], projectile.position[2]).isSolid()) {
                    projectile.position[0] -= movedX;
                    projectile.position[1] -= movedY;
                    projectile.position[2] -= movedZ;
                    wall = true;
                    if (projectile.projectileType == ProjectileArrow || projectile.projectileType == ProjectileBullet ||
                        projectile.projectileType == ProjectileShuriken) {
                        ServerUpdates::ParticlePacket particle;
                        particle.position = projectile.position;
                        particle.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 10.0f);
                        particle.color = plasma::Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f);
                        particle.size = 0.1f;
                        particle.count = 4;
                        m_serverUpdates.m_particlePacketList.push_back(particle);
                    }
                } else {
                    float distance = sqrtf(projectile.velocity[0] * projectile.velocity[0] +
                                           projectile.velocity[1] * projectile.velocity[1] +
                                           projectile.velocity[2] * projectile.velocity[2]) *
                                     frameSeconds;
                    plasma::Vector<3, float> backward = projectile.velocity.normalized() * -1.0f;
                    if (distance > rayDistance(projectile.position, backward, distance, false, true, nullptr))
                        wall = true;
                }
                if (wall && !m_isClient) {
                    bool destroyed = false;
                    for (int x = -4; x <= 4; ++x) {
                        for (int y = -4; y <= 4; ++y) {
                            for (int z = -4; z <= 4; ++z) {
                                plasma::Vector<3, Fixed> position(projectile.position[0] + (Fixed(x)),
                                                                  projectile.position[1] + (Fixed(y)),
                                                                  projectile.position[2] + (Fixed(z)));
                                float offsetX = (float)(Fixed(x));
                                float offsetY = (float)(position[1] - projectile.position[1]);
                                float offsetZ = (float)(position[2] - projectile.position[2]);
                                if (offsetX * offsetX + offsetY * offsetY + offsetZ * offsetZ >= 16.0f)
                                    continue;
                                Block original = getBlockAtDot(position[0], position[1], position[2]);
                                if (!(original.flags & BlockFlagDestructible))
                                    continue;
                                ServerUpdates::ParticlePacket particle;
                                particle.position = position;
                                particle.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 10.0f);
                                particle.color = plasma::Vector<4, float>(original.color[0] * 0.0078125f,
                                                                          original.color[1] * 0.0078125f,
                                                                          original.color[2] * 0.0078125f, 1.0f);
                                particle.size = 0.5f;
                                particle.count = 3;
                                m_serverUpdates.m_particlePacketList.push_back(particle);
                                destroyBlock(position[0], position[1], position[2]);
                                CubeChange change;
                                change.position = fixedToBlock(position);
                                change.originalBlock = original;
                                change.newBlock = s_airCube;
                                change.day = m_state.m_day;
                                m_serverUpdates.m_cubeChangeList.push_back(change);
                                destroyed = true;
                            }
                        }
                    }
                    if (destroyed) {
                        ServerUpdates::SoundPacket sound;
                        sound.position = projectile.position;
                        sound.sound = SoundRoll;
                        sound.speed = (float)rand() * 0.4f / (float)RAND_MAX + 0.5f;
                        m_serverUpdates.m_soundPacketList.push_back(sound);
                        updateSwath((int)(projectile.position[0] - Fixed(4)), (int)(projectile.position[1] - Fixed(4)),
                                    (int)(projectile.position[0] + Fixed(4)), (int)(projectile.position[1] + Fixed(4)));
                    }
                }
            }
            if (!contact && wall && projectile.projectileType != ProjectileGroundSpot && owner) {
                float hasteBefore = owner->tempo();
                owner->m_entityData.hitCombo = 0;
                owner->m_entityData.timeSinceAbility =
                    (hasteBefore / owner->tempo()) * owner->m_entityData.timeSinceAbility;
            }
            if (projectile.projectileType != ProjectileBoomerang) {
                if (contact || wall ||
                    ((projectile.projectileType == ProjectileFireball ||
                      projectile.projectileType == ProjectileMedicine) &&
                     !distanceExceeds(projectile.position, projectile.targetPosition, 2.0f) &&
                     projectile.currentTime >= 500.0f)) {
                    if (projectile.projectileType == ProjectileSpellBolt ||
                        projectile.projectileType == ProjectileFireball ||
                        projectile.projectileType == ProjectileMedicine || projectile.powered) {
                        if (projectile.projectileType == ProjectileSpellBolt ||
                            projectile.projectileType == ProjectileFireball ||
                            projectile.projectileType == ProjectileMedicine) {
                            ServerUpdates::ParticlePacket particle;
                            if (projectile.projectileType == ProjectileFireball) {
                                particle.count = 20;
                                particle.size = 0.3f;
                                ServerUpdates::SoundPacket sound;
                                sound.position = projectile.position;
                                sound.sound = SoundExplosion;
                                sound.speed = (float)rand() * 0.4f / (float)RAND_MAX + 1.0f;
                                m_serverUpdates.m_soundPacketList.push_back(sound);
                            } else if (projectile.projectileType == ProjectileMedicine) {
                                particle.count = 20;
                                particle.size = 0.3f;
                                ServerUpdates::SoundPacket sound;
                                sound.position = projectile.position;
                                sound.sound = SoundWaterSplash01;
                                sound.speed = (float)rand() * 0.4f / (float)RAND_MAX + 1.5f;
                                m_serverUpdates.m_soundPacketList.push_back(sound);
                            } else {
                                particle.count = 8;
                                particle.size = 0.2f;
                            }
                            particle.position = projectile.position;
                            particle.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 10.0f);
                            if (projectile.projectileType == ProjectileFireball || projectile.element == ElementFire) {
                                particle.type = ParticleFire;
                                particle.color = plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f);
                            } else {
                                particle.type = ParticleGlow;
                                if (projectile.projectileType == ProjectileMedicine)
                                    particle.color = plasma::Vector<4, float>(0.3f, 0.8f, 0.2f, 1.0f);
                                else
                                    particle.color = plasma::Vector<4, float>(0.25f, 0.5f, 1.0f, 1.0f);
                            }
                            m_serverUpdates.m_particlePacketList.push_back(particle);
                        }
                        if (!contact) {
                            ServerUpdates::SoundPacket sound;
                            sound.position = projectile.position;
                            sound.speed = (float)rand() * 0.4f / (float)RAND_MAX + 1.0f;
                            if (projectile.projectileType == ProjectileSpellBolt ||
                                projectile.projectileType == ProjectileFireball ||
                                projectile.projectileType == ProjectileMedicine)
                                sound.sound = projectile.element == ElementWater || projectile.element == ElementPoison
                                                  ? SoundWatersplashHit
                                                  : SoundFireHit;
                            else
                                sound.sound = SoundBlade120;
                            m_serverUpdates.m_soundPacketList.push_back(sound);
                        }
                        float blastRadius = BlastRadius;
                        if (projectile.projectileType == ProjectileFireball ||
                            projectile.projectileType == ProjectileMedicine)
                            blastRadius = LargeBlastRadius;
                        for (std::list<Creature *>::iterator entry = m_creatures.begin(); entry != m_creatures.end();
                             ++entry) {
                            Creature *target = *entry;
                            if (target == contactCreature)
                                continue;
                            if (owner) {
                                if (!m_isClient && owner->m_entityData.hostilityType == HostilityPlayer &&
                                    owner != m_localCreature)
                                    break;
                                if (target == owner || !owner->canDoDamage(target) || target->m_entityData.hp <= 0.0f)
                                    continue;
                            }
                            float halfX = target->m_entityData.appearance.size[0] * 0.5f;
                            float halfY = target->m_entityData.appearance.size[1] * 0.5f;
                            float halfZ = target->m_entityData.appearance.size[2] * 0.5f;
                            Fixed radius(blastRadius);
                            if (projectile.position[0] + radius < target->m_entityData.position[0] - Fixed(halfX) ||
                                projectile.position[1] + radius < target->m_entityData.position[1] - Fixed(halfY) ||
                                projectile.position[2] + radius < target->m_entityData.position[2] - Fixed(halfZ) ||
                                projectile.position[0] - radius >= target->m_entityData.position[0] + Fixed(halfX) ||
                                projectile.position[1] - radius >= target->m_entityData.position[1] + Fixed(halfY) ||
                                projectile.position[2] - radius >= target->m_entityData.position[2] + Fixed(halfZ) ||
                                !inRange(projectile.position, target->m_entityData.position, true, 200.0f, target))
                                continue;
                            if (target->m_entityData.rollTime == 0.0f || !owner) {
                                bool critical = false;
                                if (owner) {
                                    float random = (float)rand() / (float)RAND_MAX;
                                    critical = owner->critical(target, projectile.element != ElementPhysical) > random;
                                }
                                float damage = projectile.damage;
                                if (critical)
                                    damage += damage;
                                if (attackCreature(target, owner, damage, critical, projectile.powered,
                                                   projectile.knockback, projectile.position, knockback,
                                                   projectile.element, AbilityNone, &projectile, true))
                                    successful = true;
                            } else
                                owner->m_dodgingCreatures.insert(target->m_id);
                        }
                    }
                    if (successful && projectile.projectileType != ProjectileGroundSpot && owner) {
                        ++owner->m_entityData.hitCombo;
                        owner->onHit();
                        owner->m_entityData.timeSinceHit = 0.0f;
                    }
                    if ((projectile.projectileType == ProjectileSpellBolt && projectile.element == ElementWater) ||
                        projectile.projectileType == ProjectileMedicine) {
                        if (projectile.projectileType == ProjectileMedicine)
                            projectile.sizeInBlocks = 16.0f;
                        else
                            projectile.sizeInBlocks = projectile.knockback * 5.0f + 5.0f;
                        projectile.projectileType = ProjectileGroundSpot;
                        projectile.currentTime = projectile.powered ? 0.0f : 3000.0f;
                        projectile.damage *= 0.05f;
                        projectile.knockback = 0.0f;
                        projectile.powered = 0;
                    } else
                        expired.push_back(iterator);
                    break;
                }
            } else {
                if (successful && projectile.projectileType != ProjectileGroundSpot && owner) {
                    ++owner->m_entityData.hitCombo;
                    owner->onHit();
                    owner->m_entityData.timeSinceHit = 0.0f;
                }
                if (wall) {
                    projectile.velocity[0] = 0.0f;
                    projectile.velocity[1] = 0.0f;
                    projectile.velocity[2] = 0.0f;
                    continue;
                }
            }
            if (wall) {
                ServerUpdates::SoundPacket sound;
                sound.position = projectile.position;
                sound.sound = SoundArrowDestroy;
                sound.speed = (float)rand() * 0.4f / (float)RAND_MAX + 1.0f;
                m_serverUpdates.m_soundPacketList.push_back(sound);
                expired.push_back(iterator);
                break;
            }
        }
    }
    for (std::list<ProjectileIterator>::iterator iterator = expired.begin(); iterator != expired.end(); ++iterator)
        m_projectiles.erase(*iterator);
}

// Advances status effects, attached-creature poses, ability timers and the creature's health, mana, stealth and
// stamina. The survival rates per millisecond: mana generation and drain, the stamina of gliding, sprinting and
// whirling, stealth's mana; the hit flash, the poison and drowning ticks; the sounds' pitches are their profiles.
void World::updateSurvival(Creature *creature, float milliseconds)
{
    Creature::EntityData &data = creature->m_entityData;
    float previousMaxHP = creature->maxHP();
    if (!m_isClient) {
        int opponents = 0;
        for (auto &threat : creature->m_aggro) {
            Creature *opponent = creatureByID(threat.first);
            if (opponent && opponent->m_entityData.hp > 0.0f && opponent->m_entityData.hostilityType == HostilityPlayer)
                ++opponents;
        }
        if (opponents > 1) {
            float multiplier = (opponents - 1) * 0.5f + 1.0f;
            Creature::Buff *current = creature->findBuff(BuffMaxHpMultiplier);
            if (!current || current->strength != multiplier) {
                Creature::Buff buff;
                buff.type = BuffMaxHpMultiplier;
                buff.strength = multiplier;
                buff.timeRemaining = 10000.0f;
                buff.sourceId = creature->m_id;
                creature->setBuff(buff, true);
                ServerUpdates::BuffPacket packet;
                packet.sourceId = creature->m_id;
                packet.targetId = creature->m_id;
                packet.buff = buff;
                packet.replace = 1;
                m_serverUpdates.m_buffPacketList.push_back(packet);
            }
        }
        if (data.hp > creature->maxHP())
            data.hp = creature->maxHP();
        if (data.hostilityType == HostilityInteractiveObject && data.ownerId != 0 &&
            m_state.m_deadCreatures.find(data.ownerId) != m_state.m_deadCreatures.end())
            data.hostilityType = HostilityAttackableObject;
        if (data.appearance.appearanceFlags & AppearanceFlagCaptive) {
            Zone *zone = this->zone(floorDivide(data.position[0].toBlock(), BlocksPerZone),
                                    floorDivide(data.position[1].toBlock(), BlocksPerZone));
            if (zone) {
                bool complete = true;
                if (zone->m_missionGroupZones.size() != 0) {
                    for (std::set<std::array<int, 2>>::iterator coord = zone->m_missionGroupZones.begin();
                         coord != zone->m_missionGroupZones.end() && complete; ++coord) {
                        Zone *member = this->zone((*coord)[0], (*coord)[1]);
                        if (!member) {
                            complete = false;
                            break;
                        }
                        for (auto &id : member->m_trackedMissionCreatureIds) {
                            if (m_state.m_deadCreatures.find(id) == m_state.m_deadCreatures.end()) {
                                complete = false;
                                break;
                            }
                        }
                    }
                } else if (data.binaryToggles.active) {
                    data.binaryToggles.missionComplete = 0;
                    complete = false;
                }
                if (complete)
                    data.binaryToggles.missionComplete = 1;
            }
        }
        if (data.race == RaceClimbingPlant && data.interactionState) {
            if (!getBlockAtDot(data.position[0], data.position[1],
                               data.position[2] + Fixed(data.attackRotation[2]) + Fixed(1))
                     .isSolid() &&
                !(data.attackRotation[2] > 100.0f)) {
                float height = milliseconds * 0.01f + data.attackRotation[2];
                if ((int)height / 2 != (int)data.attackRotation[2] / 2) {
                    ServerUpdates::SoundPacket sound;
                    sound.position = data.position;
                    sound.sound = SoundDrop;
                    sound.speed = (float)rand() * 0.4f / (float)RAND_MAX + 0.8f;
                    m_serverUpdates.m_soundPacketList.push_back(sound);
                }
                data.attackRotation[2] = height;
            }
        }
    }
    if (data.currentAbility == AbilityRideNpcMount) {
        long long id = data.focusId;
        if (id && creatureByID(id)) {
            if (creatureByID(id)->m_entityData.hp > 0.0f) {
                if (creatureByID(id)->m_entityData.stunTime <= 0.0f) {
                    Creature *owner = creatureByID(id);
                    plasma::Vector<3, float> offset = riderOffset(owner->m_entityData.race);
                    offset[2] += owner->m_entityData.appearance.size[2] * 0.75f;
                    plasma::Vector<3, Fixed> shift(offset);
                    Fixed correction((owner->m_entityData.appearance.size[2] * 0.5f - data.appearance.size[2] * 0.5f) +
                                     0.01f);
                    data.position = shift + owner->m_entityData.position - plasma::Vector<3, Fixed>(0, 0, correction);
                    creature->m_drawnPosition =
                        shift + owner->m_drawnPosition - plasma::Vector<3, Fixed>(0, 0, correction);
                    data.rotation[0] = owner->m_entityData.rotation[0];
                    data.rotation[1] = owner->m_entityData.rotation[1];
                    data.rotation[2] = owner->m_entityData.rotation[2];
                    creature->m_stepOffset = owner->m_stepOffset;
                    creature->m_walkAmplitude = owner->m_walkAmplitude;
                    creature->m_walkPhase = owner->m_walkPhase;
                    data.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                    data.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                } else
                    data.currentAbility = AbilityNone;
            } else
                data.currentAbility = AbilityNone;
        } else
            data.currentAbility = AbilityNone;
    }
    if (data.currentAbility == AbilityAttachToTarget) {
        long long id = data.focusId;
        if (id && creatureByID(id) && creatureByID(id)->m_entityData.hp > 0.0f) {
            Creature *target = creatureByID(id);
            plasma::Vector<3, Fixed> offset(Fixed(data.attackRotation[0]), Fixed(data.attackRotation[1]),
                                            Fixed(data.attackRotation[2]));
            data.position = offset + target->m_entityData.position;
            creature->m_drawnPosition = offset + target->m_drawnPosition;
            if (target->m_entityData.slowedTime < 100.0f)
                target->m_entityData.slowedTime = 100.0f;
        }
    }
    if (data.currentAbility == AbilityAssassinate) {
        float elapsed = data.timeSinceAbility;
        long long id = data.focusId;
        if (creature->attackDuration() > elapsed && id && creatureByID(id) &&
            creatureByID(id)->m_entityData.hp > 0.0f) {
            Creature *target = creatureByID(id);
            plasma::Vector<3, float> offset = riderOffset(target->m_entityData.race);
            offset[2] += target->m_entityData.appearance.size[2] * 0.75f;
            float angle = (float)(target->m_entityData.rotation[2] * Pi / 180.0 + HalfPi);
            plasma::Vector<3, float> heading(cosf(angle), sinf(angle), 0.0f);
            float radius =
                (std::min)(target->m_entityData.appearance.size[0], target->m_entityData.appearance.size[1]) * -0.5f;
            plasma::Vector<3, Fixed> forward(Fixed(heading[0] * radius), Fixed(radius * heading[1]),
                                             Fixed(radius * heading[2]));
            plasma::Vector<3, Fixed> shift(offset);
            Fixed correction((target->m_entityData.appearance.size[2] * 0.5f - data.appearance.size[2] * 0.5f) + 0.01f);
            data.position =
                forward + shift + target->m_entityData.position - plasma::Vector<3, Fixed>(0, 0, correction);
            creature->m_drawnPosition =
                forward + shift + target->m_drawnPosition - plasma::Vector<3, Fixed>(0, 0, correction);
            data.rotation[0] = target->m_entityData.rotation[0];
            data.rotation[1] = target->m_entityData.rotation[1];
            data.rotation[2] = target->m_entityData.rotation[2];
            creature->m_stepOffset = target->m_stepOffset;
            creature->m_walkAmplitude = 0.0f;
            creature->m_walkPhase = 0.0f;
            data.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            data.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            data.retreat = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            data.rollTime = 0.0f;
        } else {
            std::cout << "FOCUS " << data.focusId << std::endl;
            data.currentAbility = AbilityNone;
            long long targetId = data.focusId;
            if (targetId && creatureByID(targetId) && creatureByID(targetId)->m_entityData.hp > 0.0f)
                data.position = creatureByID(targetId)->m_entityData.position;
        }
    }
    if (data.hostilityType == HostilityPet) {
        long long id = data.ownerId;
        if (creatureByID(id) && creatureByID(id)->m_entityData.currentAbility == AbilityRidePet) {
            Creature *owner = creatureByID(id);
            Fixed correction((data.appearance.size[2] * 0.5f - owner->m_entityData.appearance.size[2] * 0.5f) + 0.01f);
            creature->m_drawnPosition = owner->m_drawnPosition + plasma::Vector<3, Fixed>(0, 0, correction);
            creature->m_drawnRotation = owner->m_drawnRotation;
            data.position = owner->m_entityData.position + plasma::Vector<3, Fixed>(0, 0, correction);
            data.rotation[0] = owner->m_entityData.rotation[0];
            data.rotation[1] = owner->m_entityData.rotation[1];
            data.rotation[2] = owner->m_entityData.rotation[2];
            creature->m_smoothingTime = 0.0f;
            creature->m_stepOffset = owner->m_stepOffset;
            creature->m_walkAmplitude = owner->m_walkAmplitude;
            creature->m_walkPhase =
                owner->m_walkPhase * owner->m_entityData.appearance.size[0] / data.appearance.size[0];
            data.velocity = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            data.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        }
    }
    if (data.hp > 0.0f) {
        if (data.race == RaceBomb && !m_isClient) {
            data.hp -= creature->maxHP() * BombFuseRate * milliseconds;
            if (data.hp < 0.0f) {
                data.hp = 0.0f;
                data.timeSinceAbility = 0.0f;
            }
        }
        if (data.race == RaceWaterBubble && !m_isClient) {
            data.mp -= milliseconds * WaterBubbleDrainRate;
            if (data.mp < WaterBubbleMinMana) {
                data.hp = 0.0f;
                data.timeSinceAbility = 0.0f;
                ServerUpdates::SoundPacket sound;
                sound.position = data.position;
                sound.sound = SoundWatersplashHit;
                m_serverUpdates.m_soundPacketList.push_back(sound);
            }
        }
    }
    data.actionLockTime -= milliseconds;
    if (data.actionLockTime < 0.0f)
        data.actionLockTime = 0.0f;
    data.slowedTime -= milliseconds;
    if (data.slowedTime < 0.0f)
        data.slowedTime = 0.0f;
    data.sprintTime -= milliseconds;
    if (data.sprintTime < 0.0f)
        data.sprintTime = 0.0f;
    // Revival penalty moves towards zero over a 180-second decay.
    if (data.revivalPenalty < 0.0f) {
        data.revivalPenalty += milliseconds * 0.001f / 180.0f;
        if (data.revivalPenalty > 0.0f)
            data.revivalPenalty = 0.0f;
    }
    creature->m_hitFlash += blendFactor(milliseconds, HitFlashFadeRate) * (0.0f - creature->m_hitFlash);
    std::list<std::list<Creature::Buff>::iterator> expired;
    if (data.hp <= 0.0f)
        creature->m_buffs.clear();
    for (std::list<Creature::Buff>::iterator buff = creature->m_buffs.begin(); buff != creature->m_buffs.end();
         ++buff) {
        buff->timeRemaining -= milliseconds;
        if (buff->type == Buff_30 || buff->type == Buff_31) {
            if (buff->strength < 0.00001f) {
                expired.push_back(buff);
                continue;
            }
        } else if (buff->timeRemaining <= 0.0f) {
            if (buff->type == BuffShadowclone)
                data.hp = 0.0f;
            expired.push_back(buff);
            continue;
        }
        switch (buff->type) {
        case BuffBulwark:
            if (data.stunTime > 0.0f)
                data.stunTime = 0.0f;
            break;
        case BuffPoisoned:
            if (!m_isClient &&
                (int)buff->timeRemaining / PoisonTickInterval !=
                    (int)(milliseconds + buff->timeRemaining) / PoisonTickInterval &&
                buff->strength > 0.0f) {
                Creature *attacker = creatureByID(buff->sourceId);
                plasma::Vector<3, float> direction(0.0f, 0.0f, 0.0f);
                attackCreature(creature, attacker, buff->strength, false, false, 0.0f,
                               creature->m_abilityTargetPosition, direction, ElementPoison, AbilityNone, nullptr,
                               false);
            }
            break;
        case BuffRegeneration:
            if (!m_isClient && (int)buff->timeRemaining / 1000 != (int)(milliseconds + buff->timeRemaining) / 1000) {
                float healing = creature->maxHP() * buff->strength;
                ServerUpdates::HitPacket hit;
                hit.attackerId = buff->sourceId;
                hit.targetId = creature->m_id;
                hit.damage = -healing;
                hit.position = data.position;
                m_serverUpdates.m_hitPacketList.push_back(hit);
                data.hp += healing;
                if (data.hp > creature->maxHP())
                    data.hp = creature->maxHP();
            }
            break;
        }
    }
    for (std::list<std::list<Creature::Buff>::iterator>::iterator buff = expired.begin(); buff != expired.end(); ++buff)
        creature->m_buffs.erase(*buff);
    if (!m_isClient && data.hostilityType != HostilityPlayer) {
        float currentMaxHP = creature->maxHP();
        if (previousMaxHP > 0.0f) {
            data.hp = currentMaxHP / previousMaxHP * data.hp;
            data.hp = (std::min)(currentMaxHP, data.hp);
        }
    }
    data.stunTime -= milliseconds;
    if (data.stunTime > 0.0f) {
        data.timeSinceAbility = 0.0f;
        data.currentAbility = AbilityNone;
    }
    bool retainTargets = false;
    if ((creature == m_localCreature || (data.hostilityType != HostilityPlayer && !m_isClient)) && data.hp > 0.0f) {
        if (!(data.timeSinceAbility >= 0.0f) && data.currentAbility != AbilityNone)
            retainTargets = true;
        else {
            data.timeSinceAbility += milliseconds;
            if (data.currentAbility == AbilitySmash &&
                data.timeSinceAbility > creature->anticipationTime(AbilityUseCurrent) &&
                data.timeSinceAbility < creature->attackDuration()) {
                if (data.timeSinceAbility <
                    creature->anticipationTime(AbilityUseCurrent) + creature->strikeTime(AbilityUseCurrent))
                    data.velocity[2] = -60.0f;
            }
        }
    }
    if (!retainTargets && data.currentAbility == AbilityNone) {
        for (auto &id : creature->m_dodgingCreatures) {
            if (creature->m_struckCreatures.find(id) == creature->m_struckCreatures.end()) {
                Creature *target = creatureByID(id);
                if (target) {
                    target->applyDodgeBonus(m_serverUpdates);
                    ServerUpdates::HitPacket hit;
                    hit.attackerId = creature->m_id;
                    hit.targetId = target->m_id;
                    hit.position = target->m_entityData.position;
                    hit.hitType = HitTypeDodge;
                    m_serverUpdates.m_hitPacketList.push_back(hit);
                }
            }
        }
        creature->m_struckCreatures.clear();
        creature->m_dodgingCreatures.clear();
    }
    if (data.currentAbility == AbilityCarryCreature) {
        long long id = data.ownerId;
        if (!creatureByID(id) || creatureByID(id)->m_entityData.hp <= 0.0f)
            data.currentAbility = AbilityNone;
    }
    if (data.currentAbility != AbilityFlameRush) {
        bool changeMana = true;
        if (creature->isCaster()) {
            unsigned char ability = data.currentAbility;
            if (ability == AbilitySupportRay)
                changeMana = false;
            else if (ability == AbilityFireRay || ability == AbilityWaterRay || ability == AbilityHealingStream ||
                     ability == AbilityCenteredHealingStream || ability == AbilityFireBreath ||
                     ability == AbilityFireVortex || ability == AbilityWaterVortex || ability == AbilityHeal ||
                     ability == AbilityFireball || ability == AbilityWaterball || ability == AbilityWaterSalvo ||
                     ability == Ability_48) {
                float elapsed = data.timeSinceAbility;
                if (creature->attackDuration() > elapsed)
                    changeMana = false;
            }
            if (changeMana)
                data.mp += creature->manaGeneration() * (milliseconds * 0.0001f);
        } else {
            unsigned char ability = data.currentAbility - AbilityBlock;
            if (ability <= AbilitySalvoRelease && (ManaDrainSuppressedAbilities & (1ULL << ability)))
                changeMana = false;
            if (changeMana)
                data.mp -= milliseconds * 0.00005f;
        }
    } else {
        data.mp -= milliseconds * 0.0002f;
        if (data.mp <= 0.0f)
            data.mp = 0.0f;
    }
    if (data.mp < 0.0f)
        data.mp = 0.0f;
    if (data.mp > 1.0f)
        data.mp = 1.0f;
    if (data.hostilityType != HostilityAttackableObject) {
        if (!creature->isBlocking() || data.currentAbility == AbilityCyclone ||
            data.currentAbility == AbilityWarriorJumpAttack)
            data.blockPower += milliseconds / BlockPowerRecoveryTime;
        if (data.blockPower > 1.0f)
            data.blockPower = 1.0f;
    }
    if (data.acceleration[0] * data.acceleration[0] + data.acceleration[1] * data.acceleration[1] +
            data.acceleration[2] * data.acceleration[2] >
        0.0f)
        data.binaryToggles.leanForwardPose = 0;
    if ((data.currentAbility >= AbilitySit && data.currentAbility <= Ability_90) &&
        data.acceleration[0] * data.acceleration[0] + data.acceleration[1] * data.acceleration[1] +
                data.acceleration[2] * data.acceleration[2] >
            0.0f)
        data.currentAbility = AbilityNone;
    if (data.currentAbility == AbilityClimb && creature == m_localCreature &&
        !creature->hasSpecialItem(SpecialItemClimbingSpikes)) {
        if (!(data.flags & CreatureFlagAtSpecialWall) &&
            data.acceleration[0] * data.acceleration[0] + data.acceleration[1] * data.acceleration[1] +
                    data.acceleration[2] * data.acceleration[2] >
                0.0f &&
            data.currentAbility != AbilityHangGlide &&
            (data.flags & (CreatureFlagOnGround | CreatureFlagAtWall)) == CreatureFlagAtWall) {
            creature->m_stamina -= milliseconds * ClimbStaminaRate;
            if (creature->m_stamina <= 0.0f) {
                creature->m_stamina = 0.0f;
                data.currentAbility = AbilityNone;
                data.actionLockTime = 800.0f;
            }
        }
    }
    if (data.currentAbility == AbilitySlowFall && (data.flags & CreatureFlagOnGround))
        data.currentAbility = AbilityNone;
    unsigned char ability = data.currentAbility;
    bool stealthSkill = ability == AbilitySneak;
    if (!stealthSkill && data.classType == ClassRogue && data.specialization == SpecializationAssassin &&
        data.rollTime > 0.0f)
        stealthSkill = true;
    if (!stealthSkill && data.classType == ClassRanger && data.specialization == SpecializationSniper &&
        (ability == AbilityChargedShot || ability == AbilitySalvo || ability == AbilityChargedBoomerang ||
         data.rollTime > 0.0f) &&
        creature->skillLevel(AbilitySneak) > 0)
        stealthSkill = true;
    if (!stealthSkill) {
        if (data.binaryToggles.leanForwardPose) {
            data.stealth += milliseconds * StealthGainRate;
            if (data.stealth > 1.0f)
                data.stealth = 1.0f;
        } else {
            data.stealth -= milliseconds * StealthLossRate;
            if (data.stealth < 0.0f)
                data.stealth = 0.0f;
        }
    } else if (!(data.flags & CreatureFlagInWater)) {
        float daylight = 1.0f - powf((m_state.m_time + m_state.m_time) / MillisecondsPerDay - 1.0f, 2.0f);
        float light = powf(daylight, 2.0f);
        light = cubeLight(data.position[0], data.position[1], data.position[2], 0) / 255.0f * light;
        float generation;
        if (data.rollTime > 0.0f) {
            int level = creature->skillLevel(AbilitySneak);
            generation = skillScale(level) - light * 0.75f;
        } else {
            float speed = sqrtf(data.velocity[0] * data.velocity[0] + data.velocity[1] * data.velocity[1] +
                                data.velocity[2] * data.velocity[2]);
            int level = creature->skillLevel(AbilitySneak);
            generation = skillScale(level) - light * 0.75f - speed * 0.05f;
        }
        data.stealth += generation * (milliseconds * 0.0005f);
        if (data.stealth < 0.0f)
            data.stealth = 0.0f;
        if (data.stealth > 1.0f)
            data.stealth = 1.0f;
    } else if (data.currentAbility == AbilitySneak)
        data.currentAbility = AbilityNone;
    if (creature->findBuff(BuffCamouflage))
        data.stealth = 1.0f;
    data.mp += creature->manaGeneration() * (milliseconds * 0.00025f * data.stealth);
    if (data.mp > 1.0f)
        data.mp = 1.0f;
    ability = data.currentAbility;
    if (ability == AbilityHangGlide && !(data.flags & CreatureFlagOnGround)) {
        if (data.binaryToggles.fastMovement)
            creature->m_stamina -= milliseconds * 0.0001625f;
        else if (creature->m_stamina > 0.001f) {
            float descent = data.velocity[2] * -0.1f;
            float amount = (std::min)(1.0f, (std::max)(0.0f, descent));
            creature->m_stamina += milliseconds * 0.0001f * amount;
        }
    } else if (ability != AbilityClimb) {
        if (ability == AbilitySprint && data.acceleration[0] * data.acceleration[0] +
                                                data.acceleration[1] * data.acceleration[1] +
                                                data.acceleration[2] * data.acceleration[2] >
                                            0.01f)
            creature->m_stamina -= milliseconds * 0.001f * 0.25f;
        else if (ability == AbilityCyclone) {
            creature->m_stamina -= milliseconds * 0.00025f;
            if (creature->m_stamina <= 0.0f)
                data.currentAbility = AbilityNone;
        } else if (creature == m_localCreature && (data.flags & CreatureFlagInWater) &&
                   !(data.flags & CreatureFlagPartlyAboveWater)) {
            creature->m_stamina -= creature->divingSkill() * (milliseconds * 0.001f);
            if (creature->m_stamina <= 0.0f && m_localCreature->m_entityData.hp > 0.0f &&
                (int)(m_clock - milliseconds) / DrowningInterval != (int)m_clock / DrowningInterval) {
                float damage = (float)(creature->maxHP() * DrowningDamageFraction);
                data.hp -= damage;
                ServerUpdates::SoundPacket sound;
                sound.position = data.position;
                sound.sound = SoundDrink2;
                sound.speed = (float)rand() * 0.1f / (float)RAND_MAX + 0.5f;
                m_serverUpdates.m_soundPacketList.push_back(sound);
                if (data.hp < 0.0f)
                    data.hp = 0.0f;
                ServerUpdates::HitPacket hit;
                hit.attackerId = 0;
                hit.targetId = creature->m_id;
                hit.damage = damage;
                hit.position = data.position;
                m_serverUpdates.m_hitPacketList.push_back(hit);
            }
        } else if (ability != AbilityRockFistCharge && data.actionLockTime == 0.0f) {
            float elapsed = data.timeSinceAbility;
            if (ability != AbilityWarriorJumpAttack || elapsed > creature->attackDuration())
                creature->m_stamina += creature->regeneration() * (milliseconds * 0.0001f);
        }
    }
    if (creature->m_stamina < 0.0f)
        creature->m_stamina = 0.0f;
    if (creature->m_stamina > 1.0f)
        creature->m_stamina = 1.0f;
    float rollTimeRemaining = data.rollTime;
    creature->m_rollTime = rollTimeRemaining;
    if (((!m_isClient && data.hostilityType != HostilityPlayer) || creature == m_localCreature) &&
        rollTimeRemaining >= 590.0f && rollTimeRemaining - milliseconds < 590.0f) {
        ServerUpdates::SoundPacket sound;
        sound.position = data.position;
        sound.sound = SoundRoll;
        sound.speed = (float)rand() * 0.2f / (float)RAND_MAX + 0.9f;
        sound.sourceId = creature->m_id;
        m_serverUpdates.m_soundPacketList.push_back(sound);
    }
    if (data.rollTime != 0.0f)
        data.rollTime -= milliseconds;
    if (data.rollTime < 0.0f)
        data.rollTime = 0.0f;
}

// One step of the world: the clock, the obstacles, the static entities, the local player's mission timers, (on the
// host) the creatures of the loaded zones and the hits, the pets and the player's level. Then what the local player
// discovers where it stands (the place, the area, the map around a structure), the special item it could use here and
// the quest item lying near it, the simulation of every creature, the new day, the player's region, the strikes, and
// the block changes handed to their zones.
void World::updateStep(float milliseconds)
{
    updateTime(milliseconds);
    gatherObstacles();
    updateZonesState(milliseconds);
    if (m_localCreature) {
        // The timers of the newly known missions run out.
        std::map<std::array<int, 2>, float> timers;
        for (std::pair<const std::array<int, 2>, float> &timer : m_localCreature->m_playerData->missionTimers) {
            timer.second -= milliseconds;
            if (timer.second > 0.0f)
                timers.insert(timer);
        }
        m_localCreature->m_playerData->missionTimers = timers;
    }
    if (!m_isClient) {
        updateTerritory(milliseconds);
        despawnFarCreatures();
        despawnDeadCreatures(milliseconds);
    }
    updateTamed();
    if (m_localCreature)
        m_localCreature->updateLevelAndSkills();
    if (m_localCreature && m_localCreature->m_entityData.currentAbility != AbilityFlightToPoint &&
        m_localCreature->m_entityData.currentAbility != AbilityEagleFlight &&
        m_localCreature->m_entityData.currentAbility != AbilityMenuView && !m_worldName.empty()) {
        int cellX = floorDivide((int)m_localCreature->m_entityData.position[0], BlocksPerPlace);
        int cellY = floorDivide((int)m_localCreature->m_entityData.position[1], BlocksPerPlace);
        bool large = isLargePlace(cellX, cellY);
        if (large) {
            cellX = floorDivide(cellX, 2) * 2;
            cellY = floorDivide(cellY, 2) * 2;
        }
        int type = locationType(cellX, cellY, large);
        if (type != PlaceNone) {
            Creature *player = m_localCreature;
            if (placeInfluence(type, large,
                               placeSquaredDistance(placeCenter(cellX, cellY, large),
                                                    player->m_entityData.position[0].toBlock(),
                                                    player->m_entityData.position[1].toBlock())) > PlaceCoreInfluence) {
                player->m_playerData->discoveredPlaces.insert(std::array<int, 2> {cellX, cellY});
            }
        }
        plasma::Vector<2, int> area = findRegion(m_localCreature->m_entityData.position[0].toBlock(),
                                                 m_localCreature->m_entityData.position[1].toBlock());
        if (area[0] != NoCoordinate) {
            m_localCreature->m_playerData->discoveredAreas.insert(std::array<int, 2> {area[0], area[1]});
        }
        // Inside a structure the map of its cells (16 blocks each) within five cells becomes known, fading with the
        // distance.
        if (getBlockAtDot(m_localCreature->m_entityData.position[0], m_localCreature->m_entityData.position[1],
                          m_localCreature->m_entityData.position[2])
                .flags &
            BlockFlagDungeon) {
            Creature *player = m_localCreature;
            int centerX = tileCoordinate(player->m_entityData.position[0]);
            int centerY = tileCoordinate(player->m_entityData.position[1]);
            for (int x = centerX - 5; x <= centerX + 5; x++) {
                for (int y = centerY - 5; y <= centerY + 5; y++) {
                    float sight = 1.0f - sqrtf((float)(y - centerY) * (float)(y - centerY) +
                                               (float)(x - centerX) * (float)(x - centerX)) /
                                             5.0f;
                    // A NaN sight value passes this comparison.
                    if (!(sight <= 0.0f) && structureCell(x, y, true).kind != 0) {
                        unsigned char &seen = m_localCreature->m_playerData->placeVisibility[std::array<int, 2> {x, y}];
                        seen = (std::max)(seen, (unsigned char)(long long)(sight * 255.0f));
                    }
                }
            }
        }
        findRegion((int)m_localCreature->m_entityData.position[0], (int)m_localCreature->m_entityData.position[1]);
        // An ability that needs a special item ends without the item.
        if (m_localCreature->m_entityData.currentAbility == AbilityRidePet &&
            !m_localCreature->hasSpecialItem(SpecialItemReins))
            m_localCreature->m_entityData.currentAbility = AbilityNone;
        if (m_localCreature->m_entityData.currentAbility == AbilityHangGlide &&
            (m_localCreature->m_entityData.flags & CreatureFlagOnGround) &&
            !m_localCreature->hasSpecialItem(SpecialItemHangGlider))
            m_localCreature->m_entityData.currentAbility = AbilityNone;
        if (m_localCreature->m_entityData.currentAbility == AbilitySailBoat &&
            !m_localCreature->hasSpecialItem(SpecialItemBoat))
            m_localCreature->m_entityData.currentAbility = AbilityNone;
        // The special item (category 24) the player could use here.
        m_localCreature->m_playerData->usableSpecialItem = Item();
        if (m_localCreature->m_entityData.rollTime == 0.0f &&
            m_localCreature->m_entityData.timeSinceAbility > (float)m_localCreature->attackDuration() &&
            m_localCreature->m_entityData.currentAbility != AbilityClimb) {
            if (m_localCreature->m_entityData.currentAbility == AbilityHangGlide) {
                m_localCreature->m_playerData->usableSpecialItem.category = ItemCategorySpecial;
                m_localCreature->m_playerData->usableSpecialItem.id = SpecialItemHangGlider;
            } else if (m_localCreature->m_entityData.currentAbility == AbilitySailBoat) {
                m_localCreature->m_playerData->usableSpecialItem.category = ItemCategorySpecial;
                m_localCreature->m_playerData->usableSpecialItem.id = SpecialItemBoat;
            }
            if (m_localCreature->hasSpecialItem(SpecialItemWhistle) && isHallNear(StaticEntityBirdStatue, false) &&
                (m_localCreature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater))) {
                m_localCreature->m_playerData->usableSpecialItem.category = ItemCategorySpecial;
                m_localCreature->m_playerData->usableSpecialItem.id = SpecialItemWhistle;
            } else if (m_localCreature->hasSpecialItem(SpecialItemGateKey) &&
                       isHallNear(StaticEntityLockedGate, false) &&
                       (m_localCreature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater))) {
                m_localCreature->m_playerData->usableSpecialItem.category = ItemCategorySpecial;
                m_localCreature->m_playerData->usableSpecialItem.id = SpecialItemGateKey;
            } else if (m_localCreature->hasSpecialItem(SpecialItemFlute) && isHallNear(StaticEntityGoddess2, true) &&
                       (m_localCreature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater))) {
                m_localCreature->m_playerData->usableSpecialItem.category = ItemCategorySpecial;
                m_localCreature->m_playerData->usableSpecialItem.id = SpecialItemFlute;
            } else if (m_localCreature->hasSpecialItem(SpecialItemBell) &&
                       (isHallNear(StaticEntityMourningStatue, false) || isHallNear(StaticEntityGrid, false)) &&
                       (m_localCreature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater))) {
                m_localCreature->m_playerData->usableSpecialItem.category = ItemCategorySpecial;
                m_localCreature->m_playerData->usableSpecialItem.id = SpecialItemBell;
            } else if (m_localCreature->hasSpecialItem(SpecialItemHarp) && isHallNear(StaticEntityGoldenGate, true) &&
                       (m_localCreature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater))) {
                m_localCreature->m_playerData->usableSpecialItem.category = ItemCategorySpecial;
                m_localCreature->m_playerData->usableSpecialItem.id = SpecialItemHarp;
            } else if (m_localCreature->hasSpecialItem(SpecialItemBoat) &&
                       (m_localCreature->m_entityData.flags & CreatureFlagInWater) &&
                       !(m_localCreature->m_entityData.flags & CreatureFlagAtWall) &&
                       !(getBlockAtDot(m_localCreature->m_entityData.position[0],
                                       m_localCreature->m_entityData.position[1],
                                       m_localCreature->m_entityData.position[2])
                             .flags &
                         BlockFlagDungeon)) {
                m_localCreature->m_playerData->usableSpecialItem.category = ItemCategorySpecial;
                m_localCreature->m_playerData->usableSpecialItem.id = SpecialItemBoat;
            } else if (m_localCreature->hasSpecialItem(SpecialItemHangGlider) &&
                       !(m_localCreature->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater)) &&
                       m_localCreature->m_entityData.currentAbility != AbilityFloat &&
                       (m_localCreature->m_entityData.currentAbility == AbilityNone ||
                        m_localCreature->m_entityData.timeSinceAbility >= (float)m_localCreature->attackDuration()) &&
                       !(getBlockAtDot(m_localCreature->m_entityData.position[0],
                                       m_localCreature->m_entityData.position[1],
                                       m_localCreature->m_entityData.position[2])
                             .flags &
                         BlockFlagDungeon)) {
                m_localCreature->m_playerData->usableSpecialItem.category = ItemCategorySpecial;
                m_localCreature->m_playerData->usableSpecialItem.id = SpecialItemHangGlider;
            }
        }
        // The quest item (category 22) lying or standing in the nine zones around the player, and how many of it the
        // player carries.
        m_localCreature->m_playerData->contextItem = Item();
        m_localCreature->m_playerData->contextItemCount = 0;
        int zoneX = floorDivide(m_localCreature->m_entityData.position[0].toBlock(), BlocksPerZone);
        int zoneY = floorDivide(m_localCreature->m_entityData.position[1].toBlock(), BlocksPerZone);
        for (int x = zoneX - 1; x <= zoneX + 1; x++) {
            for (int y = zoneY - 1; y <= zoneY + 1; y++) {
                Zone *zone = this->zone(x, y);
                if (zone) {
                    for (ZoneItem &item : zone->m_items) {
                        if (item.item.category == ItemCategoryQuest)
                            m_localCreature->m_playerData->contextItem = item.item;
                    }
                    for (StaticEntity &entity : zone->m_staticEntities) {
                        if (entity.item.category != ItemCategoryNone && entity.active)
                            m_localCreature->m_playerData->contextItem = entity.item;
                    }
                }
            }
        }
        m_localCreature->m_playerData->contextItemCount =
            m_localCreature->m_inventory.countItem(m_localCreature->m_playerData->contextItem);
        if (m_localCreature->m_playerData->contextItemCount < 1)
            m_localCreature->m_playerData->contextItem = Item();
        m_localCreature->m_playerData->deathMarkerTime -= milliseconds;
        m_localCreature->m_playerData->deathMarkerTime =
            (std::max)(0.0f, m_localCreature->m_playerData->deathMarkerTime);
        // The marker is reached within 20 blocks.
        plasma::Vector<3, Fixed> toMarker =
            m_localCreature->m_playerData->deathPosition - m_localCreature->m_entityData.position;
        float markerX = (float)toMarker[0];
        float markerY = (float)toMarker[1];
        float markerZ = (float)toMarker[2];
        if (markerX * markerX + markerY * markerY + markerZ * markerZ < 400.0f)
            m_localCreature->m_playerData->deathMarkerTime = 0.0f;
    }
    for (Creature *creature : m_creatures)
        creature->m_entityData.binaryToggles.noTerrainCollision = 0;
    for (Creature *creature : m_creatures) {
        if (creature->m_entityData.hp > 0.0f) {
            moveCreature(creature, milliseconds);
            performAbility(creature, milliseconds);
            passInteractions(creature, milliseconds);
        }
        updateSurvival(creature, milliseconds);
    }
    if (!m_isClient) {
        for (int &event : m_serverUpdates.m_worldEventList) {
            if (event == WorldEventNewDay) {
                rewindDay();
                onNewDay();
                break;
            }
        }
    }
    if (m_localCreature) {
        // The player's region: inside a structure that of its place's centre.
        plasma::Vector<2, int> block;
        if (getBlockAtDot(m_localCreature->m_entityData.position[0], m_localCreature->m_entityData.position[1],
                          m_localCreature->m_entityData.position[2])
                .flags &
            BlockFlagDungeon) {
            int cellY = floorDivide((int)m_localCreature->m_entityData.position[1], BlocksPerPlace);
            int cellX = floorDivide((int)m_localCreature->m_entityData.position[0], BlocksPerPlace);
            block = placeCenter(cellX, cellY, isLargePlace(cellX, cellY));
        } else {
            block = plasma::Vector<2, int>((int)m_localCreature->m_entityData.position[0],
                                           (int)m_localCreature->m_entityData.position[1]);
        }
        m_localCreature->m_entityData.region = findRegion(block[0], block[1]);
        // The regions the player has been in, the latest first.
        if (m_localCreature->m_entityData.currentAbility != AbilityEagleFlight &&
            m_localCreature->m_entityData.currentAbility != AbilityMenuView) {
            std::list<plasma::Vector<2, int>> &visited = m_localCreature->m_playerData->visitedRegions;
            if (visited.empty()) {
                visited.push_front(m_localCreature->m_entityData.region);
            } else if (!(visited.front() == m_localCreature->m_entityData.region)) {
                for (std::list<plasma::Vector<2, int>>::iterator it = visited.begin(); it != visited.end(); ++it) {
                    if (*it == m_localCreature->m_entityData.region) {
                        visited.erase(it);
                        break;
                    }
                }
                m_localCreature->m_playerData->visitedRegions.push_front(m_localCreature->m_entityData.region);
            }
        }
    }
    updateStepProjectiles(milliseconds);
    for (CubeChange &change : m_serverUpdates.m_cubeChangeList) {
        Zone *zone =
            this->zone(floorDivide(change.position[0], BlocksPerZone), floorDivide(change.position[1], BlocksPerZone));
        if (zone)
            zone->m_cubeChanges.push_back(change);
    }
    updateTasks();
}

Zone *World::generateZone(int x, int y)
{
    Zone *zone = this->zone(x, y);
    if (zone)
        return zone;
    srand((y << 10) + m_generationSeed + x); // a zone's seed: its y shifted by 10, plus x
    zone = new Zone(this, plasma::Vector<2, int>(x, y));
    zone->generate();
    if (!m_isClient) {
        // What the world state kept of the zone when it was unloaded.
        std::array<int, 2> position = {x, y};
        std::map<std::array<int, 2>, ZoneState>::iterator state = m_state.m_zoneStates.find(position);
        if (state != m_state.m_zoneStates.end()) {
            if (state->second.flags & ZoneStateFlagItems) {
                zone->m_itemsChanged = true;
                zone->m_items = state->second.items;
            }
            if (state->second.flags & ZoneStateFlagStaticEntities) {
                zone->m_staticEntitiesChanged = true;
                zone->m_staticEntities = state->second.staticEntities;
            }
            zone->m_cubeChanges = state->second.cubeChanges;
            for (CubeChange &change : state->second.cubeChanges)
                zone->setBlock(change.position[0], change.position[1], change.position[2], change.newBlock);
            zone->m_defeatedMissionCreatureCount = state->second.defeatedMissionCreatureCount;
            zone->m_completedActivators = state->second.completedActivators;
        }
    }
    EnterCriticalSection(&m_zonesLock);
    EnterCriticalSection(&m_zoneMeshCriticalSection);
    m_zones[((unsigned long long)(unsigned)y << 32) | (unsigned)x] = zone;
    LeaveCriticalSection(&m_zoneMeshCriticalSection);
    LeaveCriticalSection(&m_zonesLock);
    return zone;
}

std::vector<std::string> World::talkScript(Creature *creature)
{
    if (creature->m_entityData.classType == ClassGuildReceptionist) {
        std::vector<std::string> script;
        script << "hello" << "how_can_i_help_you";
        script << "define_specialization" << m_localCreature->m_entityData.classType
               << (1 - m_localCreature->m_entityData.specialization) << "change_specialization?";
        script << "goodbye?";
        return script;
    } else if (creature->m_entityData.classType == ClassInnkeeper) {
        std::vector<std::string> script;
        script << "define_number" << 10;
        script << "hello" << "new_page";
        if (!(m_state.m_time >= InnNightStartHour * MillisecondsPerHour ||
              m_state.m_time <= InnNightEndHour * MillisecondsPerHour)) {
            script << "you_can_rest_here_between_six_and_six";
        } else if (m_localCreature->m_inventory.m_gold < 10) {
            script << "you_dont_have_enough_gold_to_rest_here";
        } else {
            script << "how_can_i_help_you";
            script << "rest_inn?";
            script << "goodbye?";
        }
        return script;
    } else {
        Missions missions(this,
                          findRegion((int)creature->m_entityData.origin[0], (int)creature->m_entityData.origin[1]));
        std::vector<std::string> script = missions.talk(creature);
        return script;
    }
}

}
