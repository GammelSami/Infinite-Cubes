// ID catalog companion to Missions.h.
#pragma once

namespace cube {

// mission-kinds identifiers.
enum MissionKind : int {
    MissionKindNone =
        0, // Missions::generate / determineTopic / talk; Game::update; generate returns immediately for zero
    MissionKindSupplierGnome =
        1, // Missions::generate / determineTopic / talk; Game::update; Game update emits description_supplier_gnome; generatePlace chooses RaceSupplierGnome for camp
    MissionKindMagicCraftingBookReward =
        2, // Missions::taskRewardAt; ..2790; category 24 IDs 14..17; dictionary key MagicCraftingBook
    MissionKindRareCraftingBookReward =
        3, // Missions::taskRewardAt; ..2790; category 24 IDs 14..17; dictionary key RareCraftingBook
    MissionKindEpicCraftingBookReward =
        4, // Missions::taskRewardAt; ..2790; category 24 IDs 14..17; dictionary key EpicCraftingBook
    MissionKindLegendaryCraftingBookReward =
        5, // Missions::taskRewardAt; ..2790; category 24 IDs 14..17; dictionary key LegendaryCraftingBook
    MissionKindHangGliderPlacement =
        6,                        // Missions::generate; ..767; category 24 id 1; category 24; dictionary key HangGlider
    MissionKindBoatPlacement = 7, // Missions::generate; ..767; category 24 id 2; category 24; dictionary key Boat
    MissionKindClimbingSpikesPlacement =
        8, // Missions::generate; ..767; category 24 id 18; category 24; dictionary key ClimbingSpikes
    MissionKindReinsPlacement = 9, // Missions::generate; ..767; category 24 id 3; category 24; dictionary key Reins
    MissionKindHarpPlacement = 10, // Missions::generate; ..767; category 24 id 5; category 24; dictionary key Harp
    MissionKindWhistlePlacement =
        11,                        // Missions::generate; ..767; category 24 id 7; category 24; dictionary key Whistle
    MissionKindBellPlacement = 12, // Missions::generate; ..767; category 24 id 8; category 24; dictionary key Bell
    MissionKindTreasureSpiritPlacement =
        13, // Missions::generate; ..767; category 24 id 9; category 24; dictionary key TreasureSpirit
    MissionKindArtifactReward = 14, // Missions::taskRewardAt; artifactAt return branch
    MissionKind_15 = 15,            // Meaning unknown.
    MissionKindMonster =
        16, // Missions::generate / determineTopic / talk; Game::update; determineTopic returns MissionMonster
    MissionKindAborigines =
        17, // Missions::generate / determineTopic / talk; Game::update; determineTopic returns MissionAborigines; assigned to Arena
    MissionKindExchangePartner = 18, // Missions::Missions; ..416; paired mission partner receives kind 18
    MissionKindWitchCurse =
        19, // Missions::generate / determineTopic / talk; Game::update; talk emits so_you_defeated_the_witch, you_must_defeat_her_to_break_the_curse; generates Witch boss
    MissionKindDemonPortal =
        20, // Missions::generate / determineTopic / talk; Game::update; description_demon_portal; generates DarkSite with DemonPortal
    MissionKindMagicBarriers =
        21, // Missions::generate / determineTopic / talk; Game::update; description_magic_barriers; tower barrier mission
    MissionKindFlameOfPower =
        22, // Missions::generate / determineTopic / talk; Game::update; description_flame_of_power; brazier/EternalEmber placement
    MissionKindManaPumps =
        23, // Missions::generate / determineTopic / talk; Game::update; description_mana_pump(s); generates ManaPump/ManaInductor residents
    MissionKindKey = 24, // MapOverlayWidget::draw: key icon (MapOverlayWidget.cpp case 24)
};

// missions identifiers.
enum MissionTopic {
    MissionBand = 0,         // Missions::generatePlace
    MissionMonster = 1,      // Missions::generatePlace
    MissionStatues = 2,      // Missions::generatePlace
    MissionDemonPortals = 3, // Missions::generatePlace
    MissionUnholyTombs = 4,  // Missions::generatePlace
    MissionBonePiles = 5,    // Missions::generatePlace
    MissionSlimeWells = 6,   // Missions::generatePlace
    MissionMolehills = 7,    // Missions::generatePlace
    MissionHornetsNests = 8, // Missions::generatePlace
    MissionFolkCamp =
        9, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionOgreCamp =
        10, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionWitchCamp =
        11, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionSkeletonCamp =
        12, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionMonstrosityCamp =
        13, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionNecromancerCamp =
        14, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionDarkCultistCamp =
        15, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionDemonMarauderCamp =
        16, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionSeaCrabInvasion =
        17, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionSlimeInvasion =
        18, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionUndeadInvasion =
        19, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionDemonInvasion =
        20, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionEmpireSoldierInvasion =
        21, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionInsectInvasion =
        22, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionPlantCreatureInvasion =
        23, // Missions::generatePlace: dedicated generateCamp/generateInvasion branch selects corresponding race
    MissionExchange = 24,   // Missions::generatePlace
    Mission_25 = 25,        // Meaning unknown.
    MissionErrand = 26,     // Missions::generatePlace
    MissionAborigines = 27, // Missions::generatePlace
    MissionDungeon = 28,    // Missions::generatePlace
    MissionCastle = 29,     // Missions::generatePlace
    MissionCult = 30,       // Missions::generatePlace
    MissionDarkSite = 31,   // Missions::generatePlace
    MissionDemonCult =
        32, // generateCult with demon creatures, RaceDarkCultistMale and FactionCultOfDoom (MissionCult: undead, RaceNecromancer, FactionUnholyPact); an altar's topic; Missions::generatePlace
};

// Captive mission kinds select the prison StaticEntity. Kinds 0, 2 and 4 place the item separately from the captive;
// kinds 1 and 3 attach it to the prison.
enum CaptivityKind {
    Captivity_Cage = 0,
    Captivity_Cauldron = 1,
    Captivity_LegIrons = 2,
    Captivity_Pole = 3,
    Captivity_TortureRack = 4,
};

enum MissionFlag {
    MissionFlagBell = 0x1,          // MissionKindBellPlacement
    MissionFlagHarp = 0x2,          // MissionKindHarpPlacement
    MissionFlagWhistle = 0x4,       // MissionKindWhistlePlacement
    MissionFlagMagicBarriers = 0x8, // MissionKindMagicBarriers
};

}
