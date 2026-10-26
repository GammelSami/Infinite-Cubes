#include "World.hlsli"
#include "SkyParameters.hlsli"

// Explicit slots also set reflection flags; sharing the implicit world bindings changes DXBC.
cbuffer FogAngle : register(b2)
{
    float fogGradientTranslation;
};

cbuffer SunPos : register(b3)
{
    float4 sunPos;
};

#include "Sky.hlsli"

float4 main(PixelInput input) : SV_TARGET
{
    float3 sky = saturate(skyColorAt(input.clipPosition.xy / input.clipPosition.w));
    return float4(gradeColor(sky), 1);
}
