// World grid dimensions in terrain blocks. Fixed coordinates count 1/65536 of a block. These namespace-scope integers
// can be used directly in coordinate arithmetic; zone * BlocksPerZone converts a zone coordinate to blocks.
#pragma once

namespace cube {

const int BlocksPerZone = 64;                               // a zone is 64 x 64 blocks (columns)
const int BlocksPerCell = 16;                               // a structure cell is 16 x 16 blocks
const int BlocksPerPlace = 1024;                            // a place cell is 1024 x 1024 blocks (16 x 16 zones)
const int ZonesPerRegion = 64;                              // a region is 64 x 64 zones
const int BlocksPerRegion = BlocksPerZone * ZonesPerRegion; // 4096: a region is 4096 x 4096 blocks
const int BlocksPerLand = 8192;                             // a land is 8192 x 8192 blocks

// Times of day are written as hours of it.
const int MillisecondsPerHour = 3600000;
const int MillisecondsPerDay = 24 * MillisecondsPerHour;

}
