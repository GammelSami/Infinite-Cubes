// The alpha names its water shader so (WaterPixelShader.hlsl). ps_4_0, entry point main, no compile
// flags; CMake with fxc compiles it into gfx/shaders/WaterPixelShader.h
// (g_waterPixelShader), byte for byte the original's.
#include "World.hlsli"
#include "WorldAtmosphere.hlsli"

float4 main(PixelInput input) : SV_TARGET
{
    float2 projectedPosition = input.clipPosition.xy / input.clipPosition.w;
    float3 sky = skyColorAt(projectedPosition);
    float horizon = projectedPosition.y + fogGradientTranslation;
    float3 color = lerp(skyColor1.rgb, skyColor2.rgb,
        // Water sky reflection gradient: retain 80 percent of the vertical gradient.
        0.8 * smoothCurve(saturate(horizon + 1)));
    float3 reflectedLight = normalize(2 * dot(input.lightDirection, input.normal) *
        input.normal - input.lightDirection);
    // Water specular curve: quarter-strength highlight.
    float3 specularColor = skyColor1.rgb * 0.25;
    specularColor *= max(dot(reflectedLight, -normalize(input.viewPosition)), 0);
    color = saturate(input.color.b * color + specularColor);
    // Water sky fog curve: cubic distance fog.
    float fog = fogAmount(input.viewPosition);
    color = lerp(color, fogColor.rgb, fog);
    color = saturate(lerp(color, sky, fog * fog * fog));
    // Water opacity curve: near opacity 0.75 plus squared-distance gain 0.000025.
    float opacity = min(min(dot(input.viewPosition, input.viewPosition) * 0.000025, 1) + 0.75, 1);
    return float4(gradeColor(color), opacity * alpha);
}
