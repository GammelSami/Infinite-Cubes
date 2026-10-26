// ID catalog companion to ServerUpdates.h.
#pragma once

namespace cube {

// hit-types identifiers.
enum HitType : unsigned char {
    HitTypeDamage = 0,   // Game::drawImpact / update..4638,5746..5784
    HitTypeHeal = 1,     // Game::drawImpact / update..4638,5746..5784
    HitTypeMiss = 3,     // Game::drawImpact: literal MISS; World::applyHit returns without applying damage
    HitTypeDodge = 4,    // Game::drawImpact / update..4638,5746..5784
    HitTypeMana = 5,     // Game::drawImpact / update..4638,5746..5784
    HitTypeTameness = 6, // Game::drawImpact / update..4638,5746..5784
};

enum DamageElement {
    ElementPhysical =
        0, // attackCreature subtracts armor() for 0 and resistance() for any other element, and plays the weapon's hit sound only for 0; melee strikes, arrows and boomerangs pass 0
    ElementFire =
        1, // Fire damage: plays SoundFireHit and is used by fire spells, breaths, lava puddles, bolts and traps. A fire ground spot glows orange and weakens defence.
    ElementWater =
        2, // attackCreature plays SoundWatersplashHit; the water swirls, vortexes, rays and beams, a water mage's bolts, AbilityWaterPuddle and the water bubble pass it; a projectile of it heals the creatures its owner cannot damage and never attacks (updateStepProjectiles); its ground spot is blue
    ElementQuicksand =
        4, // Quicksand trap ground spot: a brown ring that weakens defence, plays SoundPickup and never attacks.
    ElementFlameRush =
        5, // the orb a creature leaves under itself during AbilityFlameRush or BuffFlamerush (World::moveCreature makes one when none of its own lies within three blocks); drawn with a fire ring and the fire colour
    ElementPoison =
        6, // Poison-vial and poison-buff damage. Ground spots apply BuffPoisoned for 4000 ms, shown in green with SoundWatersplashHit.
    ElementLeech =
        7, // the beams of a Specialization2 caster (performAbility); attackCreature heals the attacker by a tenth of the damage dealt
};

// The values of ServerUpdates::m_worldEventList (held as ints).
enum WorldEvent {
    WorldEventNewDay =
        0, // World::updateTime adds it when the day rolls over, World::passInteractions when a character sleeps at an innkeeper; World::updateStep calls rewindDay and onNewDay for it
};

// projectiles identifiers.
enum ProjectileType : int {
    ProjectileArrow = 0,  // Renderer::renderEntities selects sprite 1025; SpriteManager::loadSprites; loads arrow.cub
    ProjectileBullet = 1, // Renderer::renderEntities selects sprite 1026; SpriteManager::loadSprites; loads bullet.cub
    ProjectileShuriken =
        2, // Renderer::renderEntities selects sprite 1027; SpriteManager::loadSprites; loads shuriken.cub
    ProjectileSpellBolt =
        3, // Renderer::renderEntities: sprite-free spiral bolt colored fire/blue by caster specialization; World::performAbility creates this type in Firebolt/Waterbolt branches
    ProjectileFireball =
        4, // Renderer::renderEntities selects sprite 1028; SpriteManager::loadSprites; loads fireball.cub
    ProjectileBoomerang = 5, // Renderer::renderEntities: uses owner WeaponBoomerang model
    ProjectileGroundSpot =
        6, // Renderer::renderEntities: emits GroundSpot with radius=sizeInBlocks and kind/color by element
    ProjectileRock =
        7, // Renderer::renderEntities selects sprite 2820; SpriteManager::loadSprites; loads iron-deposit.cub
    ProjectileEagle = 8, // Renderer::renderEntities selects sprite 3637; SpriteManager::loadSprites; loads eagle.cub
    ProjectileSteelEmpireAirship =
        9, // Renderer::renderEntities selects sprite 3507; SpriteManager::loadSprites; loads steel-empire-airship.cub
    ProjectileMedicine =
        10, // Renderer::renderEntities selects sprite 2769; SpriteManager::loadSprites; loads medicine.cub
};

// particles identifiers.
enum ParticleType : int {
    ParticleLitCube = 0,    // generic cube.cub renderer, uses world lighting, emitted by non-water splash
    ParticleFire = 1,       // packet handler forces orange/yellow color; emitted for lava and fire hits
    ParticleGlow = 2,       // cube.cub rendered with glow independent of world light; preserves packet color
    ParticleStationary = 3, // Game::updateParticles excludes type 3 from motion integration
    ParticleLeaf = 4,       // emitted from canopy, passes through tree/leaves blocks
    ParticleDebris = 5, // emitted in ground block color for footsteps and block destruction; block collision enabled
    ParticleWaterSplash = 6,   // emitted by water landing/wake, descent subdivides collision
    ParticleMusicNote = 7,     // sprite 3629 note.cub; emitted by instruments
    ParticleShrinkingCube = 8, // sprite 2630 cube.cub, size decreases with age/lifetime
};

// sounds identifiers.
enum SoundId {
    SoundNone = -1,  // World::setStaticActive: inactive fire trap sets -1; sound packet only emitted when sound != -1
    SoundHit = 0,    // Game::soundId switch loads hit.wav
    SoundBlade1 = 1, // Game::soundId switch loads blade1.wav
    SoundBlade2 = 2, // Game::soundId switch loads blade2.wav
    SoundLongBlade1 = 3,          // Game::soundId switch loads long-blade1.wav
    SoundLongBlade2 = 4,          // Game::soundId switch loads long-blade2.wav
    SoundHit1 = 5,                // Game::soundId switch loads hit1.wav
    SoundHit2 = 6,                // Game::soundId switch loads hit2.wav
    SoundPunch1 = 7,              // Game::soundId switch loads punch1.wav
    SoundPunch2 = 8,              // Game::soundId switch loads punch2.wav
    SoundHitArrow = 9,            // Game::soundId switch loads hit-arrow.wav
    SoundHitArrowCritical = 10,   // Game::soundId switch loads hit-arrow-critical.wav
    SoundSmash1 = 11,             // Game::soundId switch loads smash1.wav
    SoundSlamGround = 12,         // Game::soundId switch loads slam-ground.wav
    SoundSmashHit2 = 13,          // Game::soundId switch loads smash-hit2.wav
    SoundSmashJump = 14,          // Game::soundId switch loads smash-jump.wav
    SoundSwing = 15,              // Game::soundId switch loads swing.wav
    SoundShieldSwing = 16,        // Game::soundId switch loads shield-swing.wav
    SoundSwingSlow = 17,          // Game::soundId switch loads swing-slow.wav
    SoundSwingSlow2 = 18,         // Game::soundId switch loads swing-slow2.wav
    SoundArrowDestroy = 19,       // Game::soundId switch loads arrow-destroy.wav
    SoundBlade120 = 20,           // Game::soundId switch loads blade1.wav
    SoundPunch221 = 21,           // Game::soundId switch loads punch2.wav
    SoundSalvo2 = 22,             // Game::soundId switch loads salvo2.wav
    SoundSwordHit03 = 23,         // Game::soundId switch loads sword-hit03.wav
    SoundBlock = 24,              // Game::soundId switch loads block.wav
    SoundShieldSlam = 25,         // Game::soundId switch loads shield-slam.wav
    SoundRoll = 26,               // Game::soundId switch loads roll.wav
    SoundDestroy2 = 27,           // Game::soundId switch loads destroy2.wav
    SoundCry = 28,                // Game::soundId switch loads cry.wav
    SoundLevelUp = 29,            // Game::soundId switch loads level-up.wav
    SoundSkillUp = 30,            // Game::soundId switch loads skill-up.wav
    SoundQuestComplete = 31,      // Game::soundId switch loads quest-complete.wav
    SoundQuestProgress = 32,      // Game::soundId switch loads quest-progress.wav
    SoundNewQuest = 33,           // Game::soundId switch loads new-quest.wav
    SoundWaterSplash01 = 34,      // Game::soundId switch loads water-splash01.wav
    SoundStep2 = 35,              // Game::soundId switch loads step2.wav
    SoundStepWater = 36,          // Game::soundId switch loads step-water.wav
    SoundStepWater2 = 37,         // Game::soundId switch loads step-water2.wav
    SoundStepWater3 = 38,         // Game::soundId switch loads step-water3.wav
    SoundChannel2 = 39,           // Game::soundId switch loads channel2.wav
    SoundChannelHit = 40,         // Game::soundId switch loads channel-hit.wav
    SoundFireball = 41,           // Game::soundId switch loads fireball.wav
    SoundFireHit = 42,            // Game::soundId switch loads fire-hit.wav
    SoundMagic02 = 43,            // Game::soundId switch loads magic02.wav
    SoundWatersplash = 44,        // Game::soundId switch loads watersplash.wav
    SoundWatersplashHit = 45,     // Game::soundId switch loads watersplash-hit.wav
    SoundLichScream = 46,         // Game::soundId switch loads lich-scream.wav
    SoundDrink2 = 47,             // Game::soundId switch loads drink2.wav
    SoundPickup = 48,             // Game::soundId switch loads pickup.wav
    SoundDisenchant2 = 49,        // Game::soundId switch loads disenchant2.wav
    SoundUpgrade2 = 50,           // Game::soundId switch loads upgrade2.wav
    SoundSwirl = 51,              // Game::soundId switch loads swirl.wav
    SoundHumanVoice01 = 52,       // Game::soundId switch loads human-voice01.wav
    SoundHumanVoice02 = 53,       // Game::soundId switch loads human-voice02.wav
    SoundGate = 54,               // Game::soundId switch loads gate.wav
    SoundSpikeTrap = 55,          // Game::soundId switch loads spike-trap.wav
    SoundFireTrap = 56,           // Game::soundId switch loads fire-trap.wav
    SoundLever = 57,              // Game::soundId switch loads lever.wav
    SoundCharge2 = 58,            // Game::soundId switch loads charge2.wav
    SoundMagic0259 = 59,          // Game::soundId switch loads magic02.wav
    SoundDrop = 60,               // Game::soundId switch loads drop.wav
    SoundDropCoin = 61,           // Game::soundId switch loads drop-coin.wav
    SoundDropItem = 62,           // Game::soundId switch loads drop-item.wav
    SoundMaleGroan = 63,          // Game::soundId switch loads male-groan.wav
    SoundFemaleGroan = 64,        // Game::soundId switch loads female-groan.wav
    SoundMaleGroan65 = 65,        // Game::soundId switch loads male-groan.wav
    SoundFemaleGroan66 = 66,      // Game::soundId switch loads female-groan.wav
    SoundGoblinMaleGroan = 67,    // Game::soundId switch loads goblin-male-groan.wav
    SoundGoblinFemaleGroan = 68,  // Game::soundId switch loads goblin-female-groan.wav
    SoundLizardMaleGroan = 69,    // Game::soundId switch loads lizard-male-groan.wav
    SoundLizardFemaleGroan = 70,  // Game::soundId switch loads lizard-female-groan.wav
    SoundDwarfMaleGroan = 71,     // Game::soundId switch loads dwarf-male-groan.wav
    SoundDwarfFemaleGroan = 72,   // Game::soundId switch loads dwarf-female-groan.wav
    SoundOrcMaleGroan = 73,       // Game::soundId switch loads orc-male-groan.wav
    SoundOrcFemaleGroan = 74,     // Game::soundId switch loads orc-female-groan.wav
    SoundUndeadMaleGroan = 75,    // Game::soundId switch loads undead-male-groan.wav
    SoundUndeadFemaleGroan = 76,  // Game::soundId switch loads undead-female-groan.wav
    SoundFrogmanMaleGroan = 77,   // Game::soundId switch loads frogman-male-groan.wav
    SoundFrogmanFemaleGroan = 78, // Game::soundId switch loads frogman-female-groan.wav
    SoundMonsterGroan = 79,       // Game::soundId switch loads monster-groan.wav
    SoundTrollGroan = 80,         // Game::soundId switch loads troll-groan.wav
    SoundMoleGroan = 81,          // Game::soundId switch loads mole-groan.wav
    SoundSlimeGroan = 82,         // Game::soundId switch loads slime-groan.wav
    SoundZombieGroan = 83,        // Game::soundId switch loads zombie-groan.wav
    SoundExplosion = 84,          // Game::soundId switch loads Explosion.wav
    SoundPunch285 = 85,           // Game::soundId switch loads punch2.wav
    SoundMenuOpen2 = 86,          // Game::soundId switch loads menu-open2.wav
    SoundMenuClose2 = 87,         // Game::soundId switch loads menu-close2.wav
    SoundMenuSelect = 88,         // Game::soundId switch loads menu-select.wav
    SoundMenuTab = 89,            // Game::soundId switch loads menu-tab.wav
    SoundMenuGrabItem = 90,       // Game::soundId switch loads menu-grab-item.wav
    SoundMenuDropItem = 91,       // Game::soundId switch loads menu-drop-item.wav
    SoundCraft = 92,              // Game::soundId switch loads craft.wav
    SoundCraftProc = 93,          // Game::soundId switch loads craft-proc.wav
    SoundAbsorb = 94,             // Game::soundId switch loads absorb.wav
    SoundManashield = 95,         // Game::soundId switch loads manashield.wav
    SoundBulwark = 96,            // Game::soundId switch loads bulwark.wav
    SoundBird1 = 97,              // Game::soundId switch loads bird1.wav
    SoundBird2 = 98,              // Game::soundId switch loads bird2.wav
    SoundBird3 = 99,              // Game::soundId switch loads bird3.wav
    SoundCricket1 = 100,          // Game::soundId switch loads cricket1.wav
    SoundCricket2 = 101,          // Game::soundId switch loads cricket2.wav
    SoundOwl1 = 102,              // Game::soundId switch loads owl1.wav
    SoundOwl2 = 103,              // Game::soundId switch loads owl2.wav
    SoundRiver1 = 104,            // Game::soundId switch loads river1.wav
    SoundRiver2 = 105,            // Game::soundId switch loads river2.wav
    SoundRiver3 = 106,            // Game::soundId switch loads river3.wav
    SoundHeroicShout = 107,       // Game::soundId switch loads heroic-shout.wav
    SoundMachine = 108,           // Game::soundId switch loads machine.wav
    SoundFlute1 = 109,            // Game::soundId switch loads flute1.wav
    SoundFlute2 = 110,            // Game::soundId switch loads flute2.wav
    SoundHarp1 = 111,             // Game::soundId switch loads harp1.wav
    SoundHarp2 = 112,             // Game::soundId switch loads harp2.wav
    SoundFiddle1 = 113,           // Game::soundId switch loads fiddle1.wav
    SoundWhistle1 = 114,          // Game::soundId switch loads whistle1.wav
    SoundWhistle2 = 115,          // Game::soundId switch loads whistle2.wav
    SoundBell1 = 116,             // Game::soundId switch loads bell1.wav
    SoundBell2 = 117,             // Game::soundId switch loads bell2.wav
    SoundLute1 = 118,             // Game::soundId switch loads lute1.wav
    SoundNightingale1 = 119,      // Game::soundId switch loads nightingale1.wav
    SoundWind = 120,              // Game::soundId switch loads wind.wav
    SoundEagle = 121,             // Game::soundId switch loads eagle.wav
    SoundSpeechEnd = 122,         // Game::soundId switch loads speech-end.wav
    SoundSpeechNext = 123,        // Game::soundId switch loads speech-next.wav
    SoundTamersWhistle = 124,     // Game::soundId switch loads tamers-whistle.wav
};

}
