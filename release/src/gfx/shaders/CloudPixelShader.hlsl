// The alpha names its cloud shader so (CloudPixelShader.hlsl). ps_4_0, entry point main, no compile
// flags; CMake with fxc compiles it into gfx/shaders/CloudPixelShader.h
// (g_cloudPixelShader), byte for byte the original's.
#include "World.hlsli"
#include "WorldAtmosphere.hlsli"

float4 main(PixelInput input) : SV_TARGET
{
    float3 sky = skyColorAt(input.clipPosition.xy / input.clipPosition.w);
    float3 color = 1 / rsqrt(input.color.rgb);
    float luminance = dot(float3(0.299, 0.587, 0.114), color);
    // Cloud colour curve: quarter-strength luminance-dependent saturation boost.
    float saturation = 1 + 0.25 * (1 - luminance);
    color = saturation * color + luminance * (1 - saturation);
    // Cloud fog curve: cube the fog amount, then square its complement.
    float fog = saturate(dot(input.viewPosition, input.viewPosition) * fogScale * fogScale);
    fog = 1 - pow(1 - fog * fog * fog, 2);
    color = saturate(lerp(color, sky, fog));
    return float4(gradeColor(color), alpha);
}
