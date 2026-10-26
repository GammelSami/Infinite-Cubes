// World, view and projection matrices used by the gfx vertex shaders.
#ifndef TRANSFORMS_HLSLI
#define TRANSFORMS_HLSLI

cbuffer Proj
{
    float4x4 projMatrix;
};

cbuffer View
{
    float4x4 viewMatrix;
};

cbuffer World
{
    float4x4 worldMatrix;
};

#endif
