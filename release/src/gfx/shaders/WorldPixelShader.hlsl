#include "World.hlsli"
#include "WorldAtmosphere.hlsli"

cbuffer Shininess
{
    float shininess;
};

float4 main(PixelInput input) : SV_TARGET
{
    float2 projectedPosition = input.clipPosition.xy / input.clipPosition.w;
    float3 sky = skyColorAt(projectedPosition);
    float3 color = input.color.rgb;
    if (shininess > 0)
        // World shininess shimmer curve: amplitude 0.3, spatial frequencies 10/5/2.
        color *= 1 + 0.3 * cos(10 * (projectedPosition.y + fogGradientTranslation) +
            5 * projectedPosition.x +
            2 * (input.normal.y + input.normal.x));
    float luminance = dot(float3(0.299, 0.587, 0.114), color);
    // World colour curve: quarter-strength luminance-dependent saturation boost.
    float saturation = 1 + 0.25 * (1 - luminance);
    color = saturation * color + luminance * (1 - saturation);
    // World sky fog curve: fourth power of the fog amount.
    float fog = fogAmount(input.viewPosition);
    color = lerp(color, fogColor.rgb, fog);
    color = saturate(lerp(color, sky, fog * fog * fog * fog));
    return float4(gradeColor(color), alpha);
}
