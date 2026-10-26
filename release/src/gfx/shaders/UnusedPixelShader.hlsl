#include "World.hlsli"
#include "WorldAtmosphere.hlsli"

float4 main(PixelInput input) : SV_TARGET
{
    float distanceFog = saturate(dot(input.viewPosition, input.viewPosition) * fogScale * fogScale);
    float3 color = input.color.rgb * float3(0.25, 0.25, 1);
    color = lerp(color, skyColor2.rgb, min(distanceFog * 1.25, 1));
    if (distanceFog > 0)
        color = lerp(color, skyColor2.rgb, distanceFog * distanceFog * distanceFog);
    float luminance = dot(float3(0.299, 0.587, 0.114), color);
    color.gb = color.gb * 0.65 + luminance * 0.35;
    color = (color - 0.5) * 1.25 + 0.5;
    return float4(color, 1);
}
