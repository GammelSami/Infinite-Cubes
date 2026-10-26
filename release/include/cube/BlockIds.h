// ID catalog companion to Block.h.
#pragma once

namespace cube {

// Terrain block types.
enum BlockType {
    BlockAir = 0,
    BlockSolid = 1,
    BlockWater = 2,
    BlockWet = 3,
    BlockGrass = 4,
    BlockGround = 5,
    BlockBuilding = 6,
    BlockTree = 7,
    BlockLeaves = 8,
    BlockSand = 9, // setCube uses sandColor and stores block type 9.
    BlockSnow = 10,
    BlockSnowLeaves = 11,
    BlockType_12 = 12, // Block material meaning is unknown.
    BlockPath = 13,
    BlockRock = 14,    // World::blockProperties: rockWeight > 0.5 selects type 14 while blending rockColor
    BlockType_15 = 15, // Block material meaning is unknown.
    BlockType_16 = 16, // Block material meaning is unknown.
    BlockLava = 17,
    BlockCave = 18,
    BlockPoison = 19,
    BlockLightSource =
        20, // World::computeLight; uses the block's lightLevel as emitted light level; Renderer::renderZone keeps color unshaded
    BlockType_21 = 21,      // Block material meaning is unknown.
    BlockType_22 = 22,      // Block material meaning is unknown.
    BlockStorey0Floor = 23, // Zone::generate floorType = storey + 23
    BlockStorey1Floor = 24, // Zone::generate floorType = storey + 23
    BlockStorey2Floor = 25, // Zone::generate floorType = storey + 23
    BlockStorey3Floor = 26, // Zone::generate floorType = storey + 23
    BlockStorey4Floor = 27, // Zone::generate floorType = storey + 23
    BlockStorey5Floor = 28, // Zone::generate floorType = storey + 23
    BlockStorey6Floor = 29, // Zone::generate floorType = storey + 23
    BlockStorey7Floor = 30, // Zone::generate floorType = storey + 23
};

// Independent bits of Block::flags.
enum BlockFlag {
    BlockFlagNone = 0,         // Zero clears all block flags.
    BlockFlagDestructible = 1, // Blocks with this flag can be destroyed by performAbility.
    BlockFlagProtected = 2, // LandmarkBuilder::setBlock preserves these blocks; plant placement also tests this flag.
    BlockFlagPassableBarrier = 4, // AI collision ignores barriers with this flag; touchWall records them separately.
    BlockFlagNotTerrain = 8,      // Zone::isTerrain excludes blocks with this flag.
    BlockFlagBombable =
        16, // Marks buried-treasure cover blocks and protected walls of lined caves. Bomb blasts break blocks with this flag or BlockFlagDestructible.
    BlockFlagDungeon =
        32, // Marks protected cellar floors and pools. Game::update uses it for dungeon names and music; World::updateZonesState closes dungeon doors behind the player.
    BlockFlagBarrier = 64,         // Provides collision on air and water blocks.
    BlockFlagSpiritPassable = 128, // Allows spirits to pass the barrier.
};

enum PlacementGuard {
    PlacementKeepSolid = 1,     // a block that is neither air nor water is kept
    PlacementKeepProtected = 2, // a block with BlockFlagProtected is kept
    PlacementOnlyProtected = 4  // a block without BlockFlagProtected is kept
};

}
