#pragma once

namespace gfx {

// State creation in constructor confirms all table meanings.
enum RendererMode {
    RendererMode_OpaqueFront = 0,
    RendererMode_OpaqueBack = 1,
    RendererMode_OpaqueTwoSided = 2,
    RendererMode_DepthFront = 3,
    RendererMode_DepthBack = 4,
    RendererMode_DepthTwoSided = 5,
    RendererMode_AlphaFront = 6,
    RendererMode_AlphaBack = 7,
    RendererMode_Sky = 8,
    RendererMode_Water = 9,
    RendererMode_Clouds = 10,
};

enum RasterizerSlot {
    RasterizerFront = 0,    // solid, cull front, clockwise front faces (the opaque/depth/alpha "Front" modes)
    RasterizerBack = 1,     // the same with counter-clockwise front faces (the "Back" modes)
    RasterizerTwoSided = 2, // no culling (the "TwoSided" modes, sky, water, ground spots)
    RasterizerSlotCount = 3,
};
enum DepthStencilSlot {
    DepthTestWrite = 0,   // depth test LESS_EQUAL with depth writes (every mode but the sky)
    DepthDisabled = 1,    // no depth test (the sky)
    DepthTestNoWrite = 2, // depth test without writes (ground spots)
    DepthStencilSlotCount = 3,
};
enum BlendSlot {
    BlendAlpha = 0,     // SRC_ALPHA / INV_SRC_ALPHA (alpha modes, water, ground spots)
    BlendOpaque = 1,    // no blending (opaque modes, sky)
    BlendDepthOnly = 2, // no blending, colour writes disabled (the "Depth" modes)
    BlendSlotCount = 3,
};

enum ChunkPass {
    ChunkPassSolid = 0,
    ChunkPassWater = 1,
    ChunkPass_2 = 2, // never filled
    ChunkPassLava = 3,
    ChunkPass_4 = 4, // never filled; drawn with the solid faces
    ChunkPass_5 = 5, // never filled; drawn with the solid faces
    ChunkPassCount = 6,
};

enum FrustumPlane {
    FrustumLeft = 0,
    FrustumRight = 1,
    FrustumBottom = 2,
    FrustumTop = 3,
    FrustumNear = 4,
    FrustumFar = 5,
    FrustumPlaneCount = 6,
};

}
