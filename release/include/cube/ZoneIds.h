// ID catalog companion to Zone.h.
#pragma once

namespace cube {

// static-entities identifiers.
enum StaticEntityType {
    StaticEntityNone = -1,          // Zone::generate: special room branch assigns -1 instead of an entity kind
    StaticEntityGoddess2 = 0,       // partSprite(type) Model 3405: goddess2.cub
    StaticEntityBirdStatue = 1,     // partSprite(type) Model 3630: bird-statue.cub
    StaticEntityMourningStatue = 2, // partSprite(type) Model 3633: mourning-statue.cub
    StaticEntityDoor = 3,           // partSprite(type) Model 3406: door.cub
    StaticEntityBigDoor = 4,        // partSprite(type) Model 3407: big-door.cub
    StaticEntityBigIronDoor = 5,    // partSprite(type) Model 3408: big-iron-door.cub
    StaticEntityWindow = 6,         // partSprite(type) Model 3409: window.cub
    StaticEntityCastleWindow = 7,   // partSprite(type) Model 3410: castle-window.cub
    StaticEntityGate = 8,           // partSprite(type) Model 3411: gate.cub
    StaticEntityLockedGate = 9,     // partSprite(type) Model 3551: locked-gate.cub
    StaticEntityRuneGate = 10,      // partSprite(type) Model 3553: rune-gate.cub
    StaticEntityStoneGate = 11,     // partSprite(type) Model 3555: stone-gate.cub
    StaticEntityStoneWall = 12,     // partSprite(type) Model 2938: stone-wall.cub
    StaticEntitySpiderwebWall = 13, // partSprite(type) Model 3557: spiderweb-wall.cub
    StaticEntityMagicBarrier = 14,  // partSprite(type) Model 3558: magic-barrier.cub
    StaticEntityGrid = 15,          // partSprite(type) Model 3634: grid.cub
    StaticEntityGoldenGate = 16,    // partSprite(type) Model 3552: golden-gate.cub
    StaticEntityFireTrap =
        17, // World::setStaticActive: active transition uses SoundFireTrap; updateStaticEntities type 17 inflicts fire damage
    StaticEntitySpikeTrap = 18,   // partSprite(type) Model 3412: spike-trap.cub
    StaticEntityStompTrap = 19,   // partSprite(type) Model 3413: stomp-trap.cub
    StaticEntityLever = 20,       // partSprite(type) Model 3414: lever.cub
    StaticEntityChestBase02 = 21, // partSprite(type) Model 3415: chest-base02.cub
    StaticEntityRewardChest =
        22, // Zone::generate places it on StructureCell_RewardChest cells; opened once (PlayerData::discoveredMarkers), its item from Zone::m_objectRewards, listed in m_treasurePositions; partSprite(type) Model 3415: chest-base02.cub (the sprite of StaticEntityChestBase02)
    StaticEntityChestTop02 = 23,     // partSprite(type) Model 3419: chest-top02.cub
    StaticEntityTable = 24,          // partSprite(type) Model 3423: table.cub
    StaticEntityStoneTable = 25,     // partSprite(type) Model 3424: stone-table.cub
    StaticEntity_26 = 26,            // Meaning unknown.
    StaticEntityStoneStool = 27,     // partSprite(type) Model 3426: stone-stool.cub
    StaticEntityStool = 28,          // partSprite(type) Model 3428: stool.cub
    StaticEntitySandstoneStool = 29, // partSprite(type) Model 3427: sandstone-stool.cub
    StaticEntityBench = 30,          // partSprite(type) Model 3429: bench.cub
    StaticEntityBenchBlack = 31,     // partSprite(type) Model 3430: bench-black.cub
    StaticEntityBed = 32,            // partSprite(type) Model 3431: bed.cub
    StaticEntityBedtable = 33,       // partSprite(type) Model 3432: bedtable.cub
    StaticEntityMarketStand1 = 34,   // partSprite(type) Model 3434: market-stand1.cub
    StaticEntityMarketStand2 = 35,   // partSprite(type) Model 3435: market-stand2.cub
    StaticEntityMarketStand3 = 36,   // partSprite(type) Model 3436: market-stand3.cub
    StaticEntityBarrel = 37,         // partSprite(type) Model 3437: barrel.cub
    StaticEntityCrate = 38,          // partSprite(type) Model 3438: crate.cub
    StaticEntityOpenCrate = 39,      // partSprite(type) Model 3439: open-crate.cub
    StaticEntitySack = 40,           // partSprite(type) Model 3440: sack.cub
    StaticEntityShelter = 41,        // partSprite(type) Model 3441: shelter.cub
    StaticEntityCupboard = 42,       // partSprite(type) Model 3433: cupboard.cub
    StaticEntityDesktop = 43,        // partSprite(type) Model 3442: desktop.cub
    StaticEntityCounter = 44,        // partSprite(type) Model 3443: counter.cub
    StaticEntityShelf1 = 45,         // partSprite(type) Model 3444: shelf1.cub
    StaticEntityShelf2 = 46,         // partSprite(type) Model 3445: shelf2.cub
    StaticEntityShelf3 = 47,         // partSprite(type) Model 3446: shelf3.cub
    StaticEntityCastleShelf1 = 48,   // partSprite(type) Model 3447: castle-shelf1.cub
    StaticEntityCastleShelf2 = 49,   // partSprite(type) Model 3448: castle-shelf2.cub
    StaticEntityCastleShelf3 = 50,   // partSprite(type) Model 3449: castle-shelf3.cub
    StaticEntityStoneShelf1 = 51,    // partSprite(type) Model 3450: stone-shelf1.cub
    StaticEntityStoneShelf2 = 52,    // partSprite(type) Model 3451: stone-shelf2.cub
    StaticEntityStoneShelf3 =
        53, // SpriteManager::loadSprites m_buildingPartSprites assignment selects sprite 3452; SpriteManager::loadSprites; loads stone-shelf3.cub
    StaticEntitySandstoneShelf1 = 54, // partSprite(type) Model 3453: sandstone-shelf1.cub
    StaticEntitySandstoneShelf2 = 55, // partSprite(type) Model 3454: sandstone-shelf2.cub
    StaticEntitySandstoneShelf3 = 56, // partSprite(type) Model 3455: sandstone-shelf3.cub
    StaticEntityCorpse = 57,          // partSprite(type) Model 3456: corpse.cub
    StaticEntityRunestone =
        58, // SpriteManager::loadSprites m_buildingPartSprites assignment selects sprite 3457; SpriteManager::loadSprites; loads runestone.cub
    StaticEntityArtifact =
        59, // SpriteManager::loadSprites m_buildingPartSprites assignment selects sprite 2634; SpriteManager::loadSprites; loads artifact.cub
    StaticEntityFlowerBox01 = 60,         // partSprite(type) Model 3458: flower-box01.cub
    StaticEntityFlowerBox02 = 61,         // partSprite(type) Model 3459: flower-box02.cub
    StaticEntityFlowerBox03 = 62,         // partSprite(type) Model 3460: flower-box03.cub
    StaticEntityStreetLight02 = 63,       // partSprite(type) Model 3461: street-light02.cub
    StaticEntityStreetLight = 64,         // partSprite(type) Model 3462: street-light.cub
    StaticEntityStreetLight01 = 65,       // partSprite(type) Model 3464: street-light01.cub
    StaticEntityStreetLightUndead = 66,   // partSprite(type) Model 3465: street-light-undead.cub
    StaticEntityFence01 = 67,             // partSprite(type) Model 3466: fence01.cub
    StaticEntityFence02 = 68,             // partSprite(type) Model 3467: fence02.cub
    StaticEntityFence03 = 69,             // partSprite(type) Model 3468: fence03.cub
    StaticEntityFence04 = 70,             // partSprite(type) Model 3469: fence04.cub
    StaticEntityFenceIron1 = 71,          // partSprite(type) Model 3470: fence-iron1.cub
    StaticEntityFenceIron2 = 72,          // partSprite(type) Model 3471: fence-iron2.cub
    StaticEntityFenceIron3 = 73,          // partSprite(type) Model 3472: fence-iron3.cub
    StaticEntityFlowerVase1 = 74,         // partSprite(type) Model 3186: flower-vase1.cub
    StaticEntityFlowerVase2 = 75,         // partSprite(type) Model 3187: flower-vase2.cub
    StaticEntityFlowerVase3 = 76,         // partSprite(type) Model 3188: flower-vase3.cub
    StaticEntityFlowerVase4 = 77,         // partSprite(type) Model 3189: flower-vase4.cub
    StaticEntityFlowerVase5 = 78,         // partSprite(type) Model 3190: flower-vase5.cub
    StaticEntityFlowerVase6 = 79,         // partSprite(type) Model 3191: flower-vase6.cub
    StaticEntityFlowerVase7 = 80,         // partSprite(type) Model 3192: flower-vase7.cub
    StaticEntityFlowerVase8 = 81,         // partSprite(type) Model 3193: flower-vase8.cub
    StaticEntityFlowerVase9 = 82,         // partSprite(type) Model 3194: flower-vase9.cub
    StaticEntityCampfire = 83,            // partSprite(type) Model 3497: campfire.cub
    StaticEntityCageBase = 84,            // partSprite(type) Model 3525: cage-base.cub
    StaticEntityCage2Base = 85,           // partSprite(type) Model 3527: cage2-base.cub
    StaticEntityCage3Base = 86,           // partSprite(type) Model 3529: cage3-base.cub
    StaticEntityCage4Base = 87,           // partSprite(type) Model 3531: cage4-base.cub
    StaticEntityCauldron = 88,            // partSprite(type) Model 3533: cauldron.cub
    StaticEntityTortureRack = 89,         // partSprite(type) Model 3599: torture-rack.cub
    StaticEntityLegIrons = 90,            // partSprite(type) Model 3600: leg-irons.cub
    StaticEntityPole = 91,                // partSprite(type) Model 3601: pole.cub
    StaticEntityCeilingChain = 92,        // partSprite(type) Model 3602: ceiling-chain.cub
    StaticEntityTent = 93,                // partSprite(type) Model 3498: tent.cub
    StaticEntityBeachUmbrella = 94,       // partSprite(type) Model 3499: beach-umbrella.cub
    StaticEntityBeachTowel = 95,          // partSprite(type) Model 3500: beach-towel.cub
    StaticEntityWoodMat = 96,             // partSprite(type) Model 3501: wood-mat.cub
    StaticEntity_97 = 97,                 // Meaning unknown.
    StaticEntityFurnace = 98,             // partSprite(type) Model 3473: furnace.cub
    StaticEntityAnvil = 99,               // partSprite(type) Model 3474: anvil.cub
    StaticEntitySpinningwheel = 100,      // partSprite(type) Model 3478: spinningwheel.cub
    StaticEntityLoom = 101,               // partSprite(type) Model 3479: loom.cub
    StaticEntitySawbench = 102,           // partSprite(type) Model 3475: sawbench.cub
    StaticEntityWorkbench = 103,          // partSprite(type) Model 3476: workbench.cub
    StaticEntityCustomizationBench = 104, // partSprite(type) Model 3477: customization-bench.cub
    StaticEntityHatch = 105,              // partSprite(type) Model 3514: hatch.cub
    StaticEntityGoldKnightBanner = 106,   // partSprite(type) Model 3536: gold-knight-banner.cub
    StaticEntityGoldKnightBox = 107,      // partSprite(type) Model 3538: gold-knight-box.cub
    StaticEntitySoldierTent = 108,        // partSprite(type) Model 3539: soldier-tent.cub
    StaticEntityHorseTrough1 = 109,       // partSprite(type) Model 3540: horse-trough1.cub
    StaticEntityHorseTrough2 = 110,       // partSprite(type) Model 3541: horse-trough2.cub
    StaticEntityHorseTrough3 = 111,       // partSprite(type) Model 3542: horse-trough3.cub
    StaticEntityDruidLight = 112,         // partSprite(type) Model 3609: druid-light.cub
    StaticEntityDruidLight2 = 113,        // partSprite(type) Model 3610: druid-light2.cub
    StaticEntityDruidLight3 = 114,        // partSprite(type) Model 3611: druid-light3.cub
    StaticEntityDruidChair = 115,         // partSprite(type) Model 3612: druid-chair.cub
    StaticEntityBigPumpkin = 116,         // partSprite(type) Model 3197: big-pumpkin.cub
    StaticEntityEagleSeat = 117,          // partSprite(type) Model 3480: eagle-seat.cub
    StaticEntityPedestal = 118,           // partSprite(type) Model 2814: pedestal.cub
    StaticEntityStoneTablet1 = 119,       // partSprite(type) Model 2808: stone-tablet1.cub
    StaticEntityStoneTablet2 = 120,       // partSprite(type) Model 2809: stone-tablet2.cub
    StaticEntityStoneTablet3 = 121,       // partSprite(type) Model 2810: stone-tablet3.cub
    StaticEntityStoneTablet4 = 122,       // partSprite(type) Model 2811: stone-tablet4.cub
    StaticEntityStoneTablet5 = 123,       // partSprite(type) Model 2812: stone-tablet5.cub
    StaticEntityStoneTablet6 = 124,       // partSprite(type) Model 2813: stone-tablet6.cub
    StaticEntityInscribedTablet = 125,    // partSprite(type) Model 2815: inscribed-tablet.cub
    StaticEntityBrazier = 126,            // partSprite(type) Model 3534: brazier.cub
    StaticEntityGraveStone1 = 127,        // partSprite(type) Model 3349: grave-stone1.cub
    StaticEntityGraveStone2 = 128,        // partSprite(type) Model 3350: grave-stone2.cub
    StaticEntityGraveStone3 = 129,        // partSprite(type) Model 3351: grave-stone3.cub
    StaticEntitySarcophagus = 130,        // partSprite selects model 3618, sarcophagus.cub.
    StaticEntityWagon1 = 131,             // partSprite(type) Model 3543: wagon1.cub
    StaticEntityWagon2 = 132,             // partSprite(type) Model 3544: wagon2.cub
    StaticEntityWitchWagon = 133,         // partSprite(type) Model 3545: witch-wagon.cub
};

// plants identifiers.
enum PlantType {
    PlantFlowers = 0,             // Model 3318: flowers.cub
    PlantFlowers2 = 1,            // Model 3319: flowers2.cub
    PlantFlowers3 = 2,            // Model 3320: flowers3.cub
    PlantFlowers4 = 3,            // Model 3321: flowers4.cub
    PlantGrass = 4,               // Model 3322: grass.cub
    PlantGrass2 = 5,              // Model 3323: grass2.cub
    PlantGrass3 = 6,              // Model 3324: grass3.cub
    PlantGrassDry1 = 7,           // Model 3325: grass-dry1.cub
    PlantGrassDry2 = 8,           // Model 3326: grass-dry2.cub
    PlantGrassDark1 = 9,          // Model 3327: grass-dark1.cub
    PlantGrassDark2 = 10,         // Model 3328: grass-dark2.cub
    PlantSavannahPlant1 = 11,     // Model 3329: savannah-plant1.cub
    PlantSavannahPlant2 = 12,     // Model 3330: savannah-plant2.cub
    PlantDarkPlant1 = 13,         // Model 3331: dark-plant1.cub
    PlantDarkPlant2 = 14,         // Model 3332: dark-plant2.cub
    PlantUnderwaterPlant = 15,    // Model 3377: underwater-plant.cub
    PlantCoral = 16,              // Model 3379: coral.cub
    PlantAlga = 17,               // Model 3378: alga.cub
    PlantLavaFlower = 18,         // Model 3333: lava-flower.cub
    PlantLavaGrass = 19,          // Model 3334: lava-grass.cub
    PlantThornPlant = 20,         // Model 3335: thorn-plant.cub
    PlantEchinacea2 = 21,         // Model 3336: echinacea2.cub
    PlantLeaf = 22,               // Model 3337: leaf.cub
    PlantLantern02 = 23,          // Model 3338: lantern02.cub
    PlantWallLantern = 24,        // Model 3339: wall-lantern.cub
    PlantGreenWallLantern = 25,   // Model 3340: green-wall-lantern.cub
    PlantLanternGreen = 26,       // Model 3341: lantern-green.cub
    PlantTorch = 27,              // Model 3342: torch.cub
    PlantStone = 28,              // Model 3343: stone.cub
    PlantStone2 = 29,             // Model 3344: stone2.cub
    PlantChristmasTree = 30,      // Model 3376: christmas-tree.cub
    PlantRiverSound = 31,         // Game::updateEnvironment; plays SoundRiver1..3
    PlantTendril = 32,            // Model 3345: tendril.cub
    PlantTulipsColorful = 33,     // Model 3346: tulips-colorful.cub
    PlantCornflower = 34,         // Model 3347: cornflower.cub
    PlantReed = 35,               // Model 3348: reed.cub
    PlantPumpkinLeaves = 36,      // Model 3355: pumpkin-leaves.cub
    PlantPineappleLeaves = 37,    // Model 3356: pineapple-leaves.cub
    PlantSunflower = 38,          // Model 3357: sunflower.cub
    PlantBeanTendril = 39,        // Model 3358: bean-tendril.cub
    PlantDesertFlower01 = 40,     // Model 3359: desert-flower01.cub
    PlantDesertFlower02 = 41,     // Model 3360: desert-flower02.cub
    PlantWheat = 42,              // Model 3361: wheat.cub
    PlantCorn = 43,               // Model 3362: corn.cub
    PlantWaterLily01 = 44,        // Model 3363: water-lily01.cub
    PlantWaterLily02 = 45,        // Model 3364: water-lily02.cub
    PlantInnSign = 46,            // Model 3365: inn-sign.cub
    PlantShopSign = 47,           // Model 3366: shop-sign.cub
    PlantWeaponShopSign = 48,     // Model 3367: weapon-shop-sign.cub
    PlantArmorShopSign = 49,      // Model 3368: armor-shop-sign.cub
    PlantIdentifierSign = 50,     // Model 3369: identifier-sign.cub
    PlantSmithySign = 51,         // Model 3370: smithy-sign.cub
    PlantCarpentersshopSign = 52, // Model 3371: carpentersshop-sign.cub
    PlantWeavingmillSign = 53,    // Model 3372: weavingmill-sign.cub
    PlantIvy = 54,                // Model 3373: ivy.cub
    PlantWallRosesRed = 55,       // Model 3374: wall-roses-red.cub
    PlantWallRosesWhite = 56,     // Model 3375: wall-roses-white.cub
    PlantIncaArt1 = 57,           // Model 3380: inca-art1.cub
    PlantIncaArt2 = 58,           // Model 3381: inca-art2.cub
    PlantIncaArt3 = 59,           // Model 3382: inca-art3.cub
    PlantIncaArt4 = 60,           // Model 3383: inca-art4.cub
    PlantCrest1 = 61,             // Model 3384: crest1.cub
    PlantWallSkull = 62,          // Model 3386: wall-skull.cub
    PlantCastleChain = 63,        // Model 3385: castle-chain.cub
    PlantTorchYellow = 64,        // Model 3390: torch-yellow.cub
    PlantTorchGreenCyanFlame =
        65, // Renderer::collectSprites's ring of cubes in front of it, green to cyan; Model 3390: torch-yellow.cub
    PlantTorchBlueCyanFlame = 66, // the ring blue to cyan; Model 3390: torch-yellow.cub
    PlantTorchRedYellowFlame =
        67,               // the ring red to yellow; PlantBuilder's WallFixture_Torch; Model 3390: torch-yellow.cub
    PlantPainting01 = 68, // Model 3122: painting01.cub
    PlantPainting02 = 69, // Model 3123: painting02.cub
    PlantPainting03 = 70, // Model 3124: painting03.cub
    PlantPainting04 = 71, // Model 3125: painting04.cub
    PlantLiana = 72,      // Model 3391: liana.cub
    PlantChandelier = 73, // Model 3392: chandelier.cub
    PlantCobwebs = 74,    // Model 3393: cobwebs.cub
    PlantCobwebs2 = 75,   // Model 3394: cobwebs2.cub
    PlantWarmLight =
        76, // Game::updateEnvironment; Renderer::renderLights creates warm light; no plant sprite semantics established
    PlantLeafCanopy =
        77, // Game::updateEnvironment; PlantBuilder::buildTree places foliage canopy; Game creates falling leaves
    PlantBirdAndOwlSounds = 78,     // Game::updateEnvironment; SoundBird1 by day, SoundOwl1..2 by night
    PlantBird2Sound = 79,           // Game::updateEnvironment; plays SoundBird2 by day
    PlantBird3Sound = 80,           // Game::updateEnvironment; plays SoundBird3 by day
    PlantSkullRotten = 81,          // Model 3352: skull-rotten.cub
    PlantMoss1 = 82,                // Model 3353: moss1.cub
    PlantMoss2 = 83,                // Model 3354: moss2.cub
    PlantGoldKnightWallBanner = 84, // Model 3537: gold-knight-wall-banner.cub
    PlantWallIvy1 = 85,             // Model 3624: wall-ivy1.cub
    PlantWallIvy2 = 86,             // Model 3625: wall-ivy2.cub
    PlantWallIvy3 = 87,             // Model 3626: wall-ivy3.cub
    PlantWallIvy4 = 88,             // Model 3627: wall-ivy4.cub
    PlantShadow =
        89, // Game::updateEnvironment; Renderer::renderEntities: no sprite, emits GroundShadow from scale/rotation
    PlantStreetPennon1 = 90, // Model 3395: street-pennon1.cub
    PlantStreetPennon2 = 91, // Model 3396: street-pennon2.cub
    PlantStreetPennon3 = 92, // Model 3397: street-pennon3.cub
    PlantStreetPennon4 = 93, // Model 3398: street-pennon4.cub
    PlantStreetPennon5 = 94, // Model 3399: street-pennon5.cub
    PlantGateBanner1 = 95,   // Model 3400: gate-banner1.cub
    PlantGateBanner2 = 96,   // Model 3401: gate-banner2.cub
    PlantGateBanner3 = 97,   // Model 3402: gate-banner3.cub
    PlantGateBanner4 = 98,   // Model 3403: gate-banner4.cub
    PlantGateBanner5 = 99,   // Model 3404: gate-banner5.cub
};

// Names from the blocks each kind is collected over.
enum PositionKind {
    PositionAny = 0,            // every column top that is not solid
    PositionGround = 1,         // terrain above sea level whose block is not protected
    PositionSheltered = 2,      // terrain under a protected block (BlockFlagProtected above)
    PositionEagleSeat = 3,      // randomPosition: the top of the zone's StaticEntityEagleSeat
    PositionBuildingCentre = 4, // the centre of a building cell at its storey level (Zone.cpp, house generator)
    PositionCave = 5,           // terrain whose block is BlockCave
    PositionDungeonFloor = 6,   // terrain under a block with BlockFlagDungeon (dungeon residents, reward chests)
    PositionStorey0Floor = 7,   // block type 23 (kinds 7..14 are block types 23..30 minus 16)
    PositionNaturalGround = 16, // grass, ground, type 9, snow or rock not under water
    PositionBuildingFloor = 17, // terrain whose block is BlockBuilding
    PositionUnderwater = 18,    // solid blocks under water in valleys
    PositionCrop = 21           // field cells' crop spots (one in four)
};

// The constructor fills m_plantKinds with those choices.
enum CeilingPlantKind {
    CeilingPlant_Liana = 0,
    CeilingPlant_Chandelier = 1,
};

// PlantBuilder::fixture switch at base assigns painting model range 85..88, yellow torch, crest, skull, castle chain,
// wall-plant range 68..71. The constructor fills m_fixtureKinds.
enum WallFixtureKind {
    WallFixture_WallIvy = 0,
    WallFixture_Torch = 1,
    WallFixture_Crest = 2,
    WallFixture_Skull = 3,
    WallFixture_Chain = 4,
    WallFixture_Painting = 5,
};

enum RoomFurnishing {
    RoomFurnishing_None = 0,
    RoomFurnishing_Shop = 1,
    RoomFurnishing_Table = 2,
    RoomFurnishing_Bed = 3,
};

// Spawn-group size flags. Leader probability is 1/8 for flag 4, 1/2 for flag 8 and 1/20 for other sizes.
enum SpawnGroupSizeFlag {
    SpawnGroupSizeSingle = 1,
    SpawnGroupSizeSmall = 2,
    SpawnGroupSizeSmallLeader = 4,
    SpawnGroupSizeCamp = 8,
    SpawnGroupSizeLarge = 16,
};

enum PlantFlag {
    PlantFlagEmissive = 0x1,     // full brightness, glow and light source
    PlantFlagNeedsSupport = 0x2, // removed when the block below is air
    PlantFlagSway = 0x4,         // wind shear of the sprite
    PlantFlagUnderwater = 0x8,   // water block selects the second sprite batch
    PlantFlagSwingY = 0x10,      // swing about the top along Y
    PlantFlagSwingX = 0x20,      // swing about the top along X
};

enum ZoneItemFlag {
    ZoneItemFlagPerPlayer = 0x2, // kept in the zone and collected once per player position
};

enum SpawnTerrainFlag {
    SpawnTerrainPlain = 0x1,      // no exclusive biome condition matched
    SpawnTerrainSnow = 0x2,       // temperature below 0.2; snow animals
    SpawnTerrainUndead = 0x4,     // undead biome weight above 0.5
    SpawnTerrainMountain = 0x8,   // mountains weight above 0.5
    SpawnTerrainDesert = 0x10,    // hot dry land; desert races
    SpawnTerrainJungle = 0x20,    // hot humid land; parrot and leaf runner
    SpawnTerrain_64 = 0x40,       // present in table but never produced by chooseGroup
    SpawnTerrainWoodlands = 0x80, // landBiome equals BiomeWoodlands
    SpawnTerrainLava = 0x100,     // lava weight above 0.5, overwrites previous terrain
    SpawnTerrainEnclave = 0x200,  // enclave weight above 0.5
};

enum SpawnLocationFlag {
    SpawnLocationRiverEdge = 0x1,         // river strictly between 0.25 and 0.5
    SpawnLocationRiver = 0x2,             // river above 0.5
    SpawnLocationCoast = 0x4,             // height strictly between 0 and 10
    SpawnLocationStreetBuilding = 0x8,    // streetBuildingType is 1 through 4
    SpawnLocationCliffs = 0x10,           // cliffs above 0.75
    SpawnLocationSpecial = 0x20,          // the explicit special argument
    SpawnLocationUnderwater = 0x40,       // height at or below zero
    SpawnLocationLavaRiverEdge = 0x80,    // lavaRiver strictly between 0.25 and 0.5
    SpawnLocationLavaRiver = 0x100,       // lavaRiver above 0.5
    SpawnLocationPoisonRiver = 0x200,     // poisonRiver above 0.5
    SpawnLocationPoisonRiverEdge = 0x400, // poisonRiver strictly between 0.25 and 0.5
    SpawnLocationPlain = 0x800,           // no location condition and not special
};

}
