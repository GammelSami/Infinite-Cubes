#pragma once

namespace gfx {

// Renderer::render / collectSprites and Game::update
enum RenderFlag {
    RenderFlagHidePlayer = 0x1,          // omit the local creature
    RenderFlagMapOpen = 0x2,             // full map rather than minimap
    RenderFlagDebugCreatureBlocks = 0x8, // yellow block list and red origin cubes
    RenderFlagMapModifier = 0x10,        // control key passed to map rendering
    RenderFlagHideHud = 0x20,            // skip map compass and special item HUD
    RenderFlagLogo = 0x40,               // draw the Cube World logo
    RenderFlagOnlyPlayer = 0x80,         // skip world sky and other creatures
};

enum SpritePassFlag {
    SpritePassFlagScreenPosition = 0x1, // offset projection by each instance screen position
    SpritePassFlagZoneLights = 0x2,     // load point lights from the instance zone
};

}
