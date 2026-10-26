// Sky gradient, fog and sun colors.
#ifndef SKYPARAMETERS_HLSLI
#define SKYPARAMETERS_HLSLI

cbuffer Sky
{
    float4 skyColor1;
    float4 skyColor2;
    float4 fogColor;
    float4 sunColor;
};

#endif
