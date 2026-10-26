// CubeShader's fifth pixel shader (m_unusedPixelShader): created by CubeShader::initialize and
// released by the destructor, but no method binds it. It tints the vertex colour blue and fades it
// into skyColor2, then into a pale gradient, with the distance. The 2019 release keeps a
// descendant, also never bound (upstream/cubeworld-release UnusedPixelShader.hlsl). Its numbers:
// the tint (0.25, 0.25, 1), the fade into skyColor2 at 1.25 times the distance, the pale
// gradient's (0.8, 1, 1).
#include "World.hlsli"

float4 main(PixelInput input) : COLOR0
{
    float distance = saturate(input.fog.z);
    float3 sky = lerp(skyColor1.rgb, skyColor2.rgb, saturate(input.fog.y));
    sky = lerp(float3(0.8, 1, 1), sky, saturate(input.fog.y + 1));
    float3 color = input.color.rgb * float3(0.25, 0.25, 1);
    color = lerp(color, skyColor2.rgb, min(1, distance * 1.25));
    if (distance > 0)
        color = lerp(color, sky, distance * distance * distance);
    return float4(color, 1);
}
