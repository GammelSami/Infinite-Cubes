#pragma once
namespace cube {
// World grid extents per side: 256 blocks per zone, 64 zones per region and 1024 regions per
// World, giving 65536 zones and 16777216 blocks.
enum WorldDimensions {
    BlocksPerZone = 256,
    ZonesPerRegion = 64,
    ZonesPerSceneryRecord = 8,
    BlocksPerSceneryRecord = BlocksPerZone * ZonesPerSceneryRecord,
    BlocksPerRegion = BlocksPerZone * ZonesPerRegion,
    SceneryRecordsPerRegionAxis = ZonesPerRegion / ZonesPerSceneryRecord,
    RegionsPerWorld = 1024,
    ZonesPerWorld = RegionsPerWorld * ZonesPerRegion,
    BlocksPerWorld = ZonesPerWorld * BlocksPerZone,
    BlocksPerChunk = 32,
    ChunksPerWorld = BlocksPerWorld / BlocksPerChunk,
    // The objects are listed by cells of 8 x 8 block columns (World::getCellObjects).
    BlocksPerObjectCell = 8,
    ObjectCellsPerZone = BlocksPerZone / BlocksPerObjectCell,
    ObjectCellsPerWorld = BlocksPerWorld / BlocksPerObjectCell
};
}
