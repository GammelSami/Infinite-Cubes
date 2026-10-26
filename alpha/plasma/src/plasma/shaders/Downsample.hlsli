// Downsampling shaders average a factor-by-factor texel block, for factors two, three and four.
// Coordinates are normalized and textureScale is one texel; the current draw code reads the
// register instead of uploading it.
#ifndef DOWNSAMPLE_HLSLI
#define DOWNSAMPLE_HLSLI

#include "Surface.hlsli"

float2 textureScale;

// The texture coordinate of the centre of the first texel of the block the output pixel covers.
float2 firstTexel(float2 texCoord, float factor)
{
    return (factor * floor(texCoord / textureScale) + 0.5) * textureScale;
}

#endif
