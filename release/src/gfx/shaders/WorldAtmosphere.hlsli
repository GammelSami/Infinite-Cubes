// World pixel atmosphere buffers and distance fog.
#ifndef WORLDATMOSPHERE_HLSLI
#define WORLDATMOSPHERE_HLSLI

#include "SkyParameters.hlsli"

cbuffer FogDistance
{
    float fogScale;
};

cbuffer FogAngle
{
    float fogGradientTranslation;
};

cbuffer SunPos
{
    float4 sunPos;
};

cbuffer Alpha
{
    float alpha;
};

#include "Sky.hlsli"

float fogAmount(float3 viewPosition)
{
    float distanceSquared = dot(viewPosition, viewPosition);
    return smoothCurve(min(pow(distanceSquared * (fogScale * fogScale),
        // Atmospheric fog exponent curve: distance scale -280, exponent range 0.75.
        1 - 0.75 * exp(-280 * fogScale)), 1));
}

#endif
