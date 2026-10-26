#pragma once

// Shared world layout dimensions and thresholds for places, structures, channels, snow and map rendering.

#include "cube/WorldDimensions.h"

namespace cube {

const int CellsPerPlot = 3;                             // a hamlet plot (a house and its yard) is 3 x 3 cells
const int BlocksPerPlot = CellsPerPlot * BlocksPerCell; // Hamlet plot width in terrain blocks.
const int CellsPerDungeonRoom = 8;                      // a dungeon room is 8 x 8 cells
const int BlocksPerDungeonRoom = CellsPerDungeonRoom * BlocksPerCell; // Dungeon room width in terrain blocks.

// A block or cell coordinate that marks no position: no selection on the map, no place or village found, no
// destination (INT_MAX).
const int NoCoordinate = 0x7fffffff;

// A place's radius in blocks, by its size: its influence (1 - d^2 / r^2)^2 falls to 0 there (World::placeInfluence).
// Its border is displaced by up to PlaceBorderNoiseReach blocks of noise (World::placeSquaredDistance), so a place
// reaches that much further.
const int SmallPlaceRadius = 250;
const int LargePlaceRadius = 500;
const int PlaceBorderNoiseReach = 32;
const int SmallPlaceReach =
    SmallPlaceRadius + PlaceBorderNoiseReach; // Small-place radius including border noise, in blocks.
const int LargePlaceReach =
    LargePlaceRadius + PlaceBorderNoiseReach; // Large-place radius including border noise, in blocks.

// A cell or point belongs to a place's core where the place's influence is above this (the mission generators; a place
// counts as discovered).
const float PlaceCoreInfluence = 0.5f;

// A column lies in a river, lava or poison channel where valleyWater, lavaChannels or poisonChannels is above this.
const float ChannelThreshold = 0.5f;

// Below this temperature foliage is snowy: groundColors' white foliage, the trees' snowy leaves, the plateau's snow
// cover and the map's tree crowns.
const float SnowLineTemperature = 0.15f;

// The world map: blocks per map unit at zoom level 0; level n has 2^n times as many.
const float BlocksPerMapUnitAtLevelZero = 4.0f;

// A mission resident's interaction state once its task is done: an arena champion beaten, a completed mission's reward
// handed out.
const int InteractionStateDone = 255;

// the direction a dungeon's corridor runs from the place centre to the edge of the place farthest from it
// (World::dungeonZLevel picks it; dungeonWayLength, isCorridor, isHallEnd and isHallNearEnd test x against the west
// (0) and east (1) edges and y against the south (2) and north (3) ones).
enum DungeonDirection {
    DungeonDirectionWest = 0,
    DungeonDirectionEast = 1,
    DungeonDirectionSouth = 2,
    DungeonDirectionNorth = 3,
};

}
