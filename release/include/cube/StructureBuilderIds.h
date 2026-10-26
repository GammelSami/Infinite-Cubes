// ID catalog for StructureBuilder.h.
#pragma once

namespace cube {

// Dispatch identifiers.
enum StructureTexture {
    TexturePlain = 0,  // the colour itself
    TextureBrick = 1,  // brighter spots by noise (brickColor)
    TextureRock = 2,   // World::rockColor of the block; the colour is not used
    TextureGround = 3, // World::groundColor of the block
    TextureWoodX = 4,  // stripes by noise along x
    TextureWoodY = 5,  // along y (woodColorY)
    TextureWoodZ = 6,  // along z (woodColorZ)
    TextureWood = 7    // by noise over x and y (woodColor)
};

// Dispatch identifiers.
enum StructureFill {
    FillMaterial = 0,  // the material's colour (colorAt) with m_fillType
    FillBlend = 1,     // a noise blend of m_fillColor0 and m_fillColor1 with m_fillTexture
    FillGrass = 2,     // World::harvestGrassColor, BlockGrass
    FillLowRock = 3,   // lowRockColor, block type 14
    FillSand = 4,      // World::sandColor, block type 9
    FillRock = 5,      // rockColor, BlockBuilding
    FillCover = 6,     // World::computeCoverBlock: the terrain's own cover
    FillChecker = 8,   // a grey and white checkerboard of 4 x 4 blocks
    FillFoundation = 9 // the material inside solid ground, the terrain's cover elsewhere
};

// Dispatch identifiers.
enum StructureCover {
    CoverAll = 0, // every column
    CoverNone = 1,
    CoverPatches = 2, // where a noise of the column is positive
    CoverWorn = 3     // all but the spots where a finer noise falls below -0.3
};

}
