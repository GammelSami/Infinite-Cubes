// CubeShader's cloud pixel shader (m_cloudPixelShader): the clouds GameController's render draws
// above humid land. The vertex colour, alpha included, fades into the sky gradient (in all four
// channels) with the distance; the gradient and the horizon are the world pixel shader's (0.6, 2,
// 0.2 sky and 0.8 fog).
#include "World.hlsli"

float4 main(PixelInput input) : COLOR0
{
    float distance = saturate(input.fog.z);
    float fade = 1 - distance * distance * distance * distance * distance;
    float height = input.fog.y;
    float4 sky = lerp(skyColor1, skyColor2, smoothCurve(saturate(0.6 * height)));
    float horizonAmount = smoothCurve(saturate(2 * height + 1));
    float4 horizon = 0.2 * skyColor1 + 0.8 * fogColor;
    sky = lerp(horizon, sky, horizonAmount);
    return lerp(input.color, sky, 1 - fade * fade);
}
