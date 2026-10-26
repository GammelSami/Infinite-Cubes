// Identifiers used by creature predicates.
#pragma once
namespace cube {
// The spells are named by the cast bar's labels (GameController:update: "Fire Swirl", "Water
// drop", "Teleport to City",...); the same label on several values is told apart by the weapon
// whose ability button carries it (wand or bracelet, GameController::update's m_barAbilities). 106
// is started by "[R] Ride" on the player's pet and 107 by key G with the boat in water.
enum AbilityId {
    AbilityNone = 0,
    AbilityRightSlash = 1,
    AbilityLeftSlash = 2,
    Ability3 = 3,
    Ability4 = 4,
    AbilityPerforate = 5,
    AbilityRightStab = 6,
    AbilityLeftStab = 7,
    AbilityBlock = 8,
    AbilityShieldAttack = 9,
    AbilityBlockJab = 10,
    AbilitySwirlStrike = 11,
    AbilityCrossedWeaponSweep = 12,
    AbilityLeftSweep = 13,
    AbilityRightSweep = 14,
    AbilityOverheadChop = 15,
    AbilityMonsterChargeStrike = 16,
    AbilityAmbush = 17,
    AbilityLeftDaggerPierce = 18,
    AbilityRightDaggerPierce = 19,
    AbilityKick = 20,
    AbilityRangerKick = 21,
    AbilityShoot = 22,
    AbilityChargedShotRelease = 23,
    AbilityChargedShot = 24,
    AbilitySalvo = 25,
    AbilityThrowBoomerang = 26,
    AbilityChargeBoomerang = 27,
    AbilitySupportRay = 28,
    Ability29 = 29,
    AbilityFireSwirl = 30,
    AbilityFireVortex = 31,
    AbilityWaterSwirl = 32,
    AbilityWaterVortex = 33,
    AbilityHealingStream = 34,
    Ability35 = 35,
    AbilityChargeFireball = 36,
    AbilityFireball = 37,
    AbilityWandFirebolt = 38,
    AbilityFirebolt = 39,
    AbilityBraceletFirebolt = 40,
    AbilityWaterDrop = 41,
    AbilityBraceletWaterDrop = 42,
    AbilityWaterSplash = 43,
    AbilityWandWaterDrop = 44,
    AbilityWaterSalvo = 45,
    AbilityFireSalvo = 46,
    Ability47 = 47,
    AbilityIntercept = 48,
    AbilityTeleport = 49,
    AbilityRetreat = 50,
    AbilityForwardRoll = 51,
    Ability52 = 52,
    AbilitySmash = 54,
    AbilitySalvoRelease = 55,
    AbilityTwoHandedSweep = 57,
    AbilityTwoHandedThrust = 58,
    AbilityCrush = 59,
    AbilityCrushStrike = 60,
    AbilityGuardianCrushStrike = 61,
    Ability62 = 62,
    AbilitySwirl = 63,
    AbilityMonsterCharge = 64,
    AbilityTwoHandedSpin = 65,
    AbilityTwoHandedSlam = 66,
    AbilityTwoHandedSwing = 67,
    AbilityHeadbutt = 68,
    AbilityHornCharge = 69,
    AbilityTailStrike = 70,
    AbilityShellBlock = 71,
    AbilityShellSpin = 72,
    AbilityScream = 73,
    AbilityStomp = 74,
    AbilityBite = 75,
    AbilityDashForward = 76,
    AbilityDashLeft = 77,
    AbilityDashRight = 78,
    AbilitySneak = 79,
    AbilityDrink = 80,
    AbilityEat = 81,
    AbilityOfferPetFood = 82,
    AbilitySit = 83,
    AbilitySleep = 84,
    Ability85 = 85,
    AbilityCyclone = 86,
    AbilityExplosion = 87,
    AbilityFireExplosion = 88,
    AbilityLava = 89,
    AbilityWaterCloud = 90,
    AbilityQuake = 91,
    AbilitySummon = 92,
    Ability93 = 93,
    AbilityFireBeam = 94,
    AbilityFireRay = 95,
    AbilityShuriken = 96,
    AbilityCamouflage = 97,
    AbilityParry = 98,
    AbilityAim = 99,
    AbilitySwiftness = 100,
    AbilityBulwark = 101,
    AbilityWarFrenzy = 102,
    AbilityManaShield = 103,
    AbilityBlockStrike = 104,
    AbilityTeleportToCity = 105,
    AbilityRidePet = 106,
    AbilitySailBoat = 107,
    AbilityThrowRock = 108,
    AbilityPresentItem = 109,
    AbilityPetFood = 110,
};
// Animation poses selected by rendering. Two-handed slam alternates with the spin; water cloud
// creates a stationary eight-block cloud. Unused cases remain opaque.
enum AnimationId {
    AnimationNone = 0,
    AnimationSmash = 1,
    AnimationTwoHandedSweep = 2,
    AnimationTwoHandedThrust = 3,
    AnimationCrush = 4,
    AnimationMonsterCharge = 5,
    AnimationSwirl = 6,
    AnimationTwoHandedRest = 7,
    AnimationParry = 8,
    AnimationBlock = 9,
    AnimationBlockJab = 10,
    AnimationSupportRay = 11,
    AnimationLeftSlash = 15,
    AnimationRightSlash = 16,
    AnimationRightBackhand = 17,
    AnimationLeftBackhand = 18,
    AnimationLeftStab = 19,
    AnimationRightStab = 20,
    AnimationDrawBow = 21,
    AnimationShoot = 22,
    AnimationFireball = 23,
    AnimationLeftCast = 24,
    AnimationRightCast = 25,
    AnimationFirebolt = 26,
    AnimationFireballCharge = 27,
    AnimationSwirlStrike = 28,
    AnimationCrossedWeaponSweep = 29,
    AnimationLeftSweep = 30,
    AnimationRightSweep = 31,
    AnimationOverheadChop = 32,
    AnimationPerforate = 33,
    AnimationIntercept = 34,
    AnimationAmbush = 35,
    AnimationTwoHandedSwing = 37,
    AnimationHeadbutt = 38,
    AnimationHornCharge = 39,
    AnimationTailStrike = 40,
    AnimationShell = 41,
    AnimationScream = 42,
    AnimationStomp = 43,
    AnimationClawStrike = 44,
    AnimationDashForward = 45,
    AnimationDashLeft = 46,
    AnimationDashRight = 47,
    AnimationTwoHandedSpin = 48,
    AnimationGuardianCrushStrike = 49,
    AnimationKick = 50,
    AnimationDrink = 51,
    AnimationEat = 52,
    AnimationOfferPetFood = 53,
    AnimationSneak = 54,
    AnimationSleep = 55,
    AnimationActionLocked = 56,
    AnimationSit = 57,
    AnimationRide = 58,
    AnimationSailBoat = 59,
    AnimationCyclone = 60,
    AnimationCastSpell = 61,
    AnimationHealingStream = 62,
    AnimationQuake = 63,
    AnimationThrow = 65,
    AnimationThrowBoomerang = 66,
    AnimationChargedBoomerang = 67,
    AnimationBulwark = 68,
    AnimationBlockStrike = 69,
    AnimationDiveBite = 70,
    AnimationThrowRock = 71,
    AnimationPresentItem = 72,
    AnimationEatPetFood = 73,
};
// Character-creation menu choices, converted to RaceId by fillAppearance.
enum CharacterRaceChoice {
    CharacterRaceChoiceHuman,
    CharacterRaceChoiceElf,
    CharacterRaceChoiceDwarf,
    CharacterRaceChoiceOrc,
    CharacterRaceChoiceGoblin,
    CharacterRaceChoiceLizard,
    CharacterRaceChoiceUndead,
    CharacterRaceChoiceFrogman
};
// AddSprites hiddenParts: omitted model pieces. Individual flags are bit indices; use (1 << flag)
// to form a mask.
enum HiddenCreaturePart {
    HiddenCreatureHead = 0,
    HiddenCreatureHair = 1,
    HiddenCreatureShoulders = 2,
    HiddenCreatureChest = 3,
    HiddenCreatureLeftHand = 4,
    HiddenCreatureRightHand = 5,
    HiddenCreatureWeapons = 6,
    HiddenCreatureLeftFoot = 7,
    HiddenCreatureRightFoot = 8
};
// World::update copies the selected race, the summoner, or its opponent into a summoned creature.
// Creature and Spawn choose different defaults.
enum SummonAppearanceMode {
    SummonSelectedRace = 0,
    SummonCopySummoner = 1,
    SummonCopyOpponent = 2
};
// Number of entries, not an ID.
enum {
    SummonAppearanceModeCount = 3
};
// EntityData::flags, set by the world's movement step (names; moveCharacter confirms the three
// uses). Individual flags are bit indices; use (1 << flag) to form a mask.
enum CreatureFlag {
    CreatureFlagOnGround = 0,
    CreatureFlagInWater = 1,
    CreatureFlagAtWall = 2,
    CreatureFlagPartlyAboveWater = 4, // World::update: no rising unless climbing
    CreatureFlagMovementBlocked = 5,  // World::update: stuck (pushing hard, moving a hundredth of that)
    CreatureFlagHitObject = 6         // World::update: ran into a zone object
};
// Bit indices of Creature::BinaryToggles, used by EntityData::setToggles. The bit-field declaration
// in Creature.h fixes the numbering; bit 3's meaning remains unverified (the spawn state copies
// it).
enum CreatureToggle {
    CreatureToggleClimbing = 0,
    CreatureToggleActive = 1,
    CreatureToggleFaceAim = 2,
    CreatureToggle3 = 3,
    CreatureToggleGliding = 4,
    CreatureToggleCombatSideOverride = 5,
    CreatureToggleFastMovement = 6,
    CreatureToggleDodge = 7,
    CreatureToggleMissionComplete = 8,
    CreatureToggleLampOn = 9,
    CreatureToggleSneaking = 10
};
// EntityData:classType: the four player classes and, from 0x80, the roles of non-player
// characters. 0x89 opens the adaption panel (GameController::openInteraction), where the release
// has an alchemist.
enum ClassType {
    ClassNone = 0,
    ClassWarrior = 1,
    ClassRanger = 2,
    ClassMage = 3,
    ClassRogue = 4,
    ClassItemVendor = 0x80,
    ClassWeaponVendor = 0x81,
    ClassArmorVendor = 0x82,
    ClassAnalyzer = 0x83,
    ClassInnkeeper = 0x84,
    ClassSmith = 0x85,
    ClassCarpenter = 0x86,
    ClassTailor = 0x87,
    ClassArchaeologist = 0x88,
    ClassAdapter = 0x89
};
// EntityData::specialization, per class (values repeat across classes).
enum Specialization {
    // UI order for player choices, independent of their class-specific names.
    SpecializationFirst = 0,
    SpecializationSecond = 1,
    SpecializationBerserker = 0,
    SpecializationGuardian = 1, // ClassWarrior
    SpecializationSniper = 0,
    SpecializationScout = 1, // ClassRanger
    SpecializationFire = 0,
    SpecializationWater = 1, // ClassMage
    SpecializationAssassin = 0,
    SpecializationNinja = 1, // ClassRogue
    Specialization2 = 2      // World::equip gives it to wizards and witches
};
// Number of entries, not an ID.
enum {
    PlayerSpecializationCount = 2
};
enum HostilityType {
    HostilityPlayer = 0,
    HostilityEnemy = 1,
    HostilityFriendlyNpc = 3,
    HostilityType4 = 4, // World::update: moves at 30 blocks per second
    HostilityPet = 5,
    HostilityAttackableObject = 6
};
// Buff wire values. Descriptive effects: type 11 forces criticalChance to 1; type 12 boosts tempo.
// World::attackCreature gives useAbility creates war frenzy/camouflage; update accumulates
// tameness, gates rejected feeding, skips spell wind-up with instant fire and fills charge with
// instant shoot.
enum BuffType {
    BuffBulwark = 1,
    BuffWarFrenzy = 2,
    BuffCamouflage = 3,
    BuffPoisoned = 4,
    BuffManashield = 6,
    BuffTameness = 7,
    BuffTamingAnger = 8,
    BuffInstantFire = 9,
    BuffInstantShoot = 10,
    BuffGuaranteedCritical = 11,
    BuffTempoBoost = 12
};
// Appearance flags control four-legged and flying motion, fish models, head-based aim, natural
// attack and armor bonuses, and class-master interactions. Each flag is a bit index; form masks
// with (1 << flag).
enum AppearanceFlag {
    AppearanceFlagQuadruped = 0, // World::update: no forward tilt in water
    AppearanceFlagFlying = 1,
    AppearanceFlagHeadAimOrigin = 2, // Creature::sightPosition: offset from the body
    AppearanceFlagHandAttacks = 3,
    AppearanceFlagBiteAttack = 4,
    AppearanceFlagUnarmored = 5,
    AppearanceFlagClassMaster = 6,
    AppearanceFlagFish = 8, // World::update: a swimmer; everything but water is solid
    AppearanceFlagHasDialog =
        7, // World::update: ends the search for an innkeeper at such a creature // World::update: no random
           // walk for such spawns
    AppearanceFlagMissionLevel = 12, // World::update: a mission square's resident (level and rarity from the mission)
    AppearanceFlagTinted =
        9, // the elite creatures: drawn lighter in the creature's own colour (Creature::color) with a light of
           // its own; more HP, damage and armour, a special skill (WorldGeneration); bit 9, one name in both
    AppearanceFlagSuppressHairDebris = 10, // render breaks a dying creature into particles without its hair
    AppearanceFlagSummoned = 11,           // Summoned creatures drop no loot.
    // Only player hits count for this creature, and its death advances the mission.
    AppearanceFlagMissionResident = 13
};
}
