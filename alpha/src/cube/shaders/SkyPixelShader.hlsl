// CubeShader's sky pixel shader (m_skyPixelShader): the screen-filling background behind the world
// (GameController's render draws it first), a gradient from skyColor1 to skyColor2 up the screen
// that fades into a mix of skyColor1 and fogColor at the horizon (the world pixel shader's
// numbers: the gradient at 0.6 of the height, the horizon band at twice it, 0.2 sky and 0.8 fog).
#include "World.hlsli"

float4 main(PixelInput input) : COLOR0
{
    float height = input.fog.y;
    float3 sky = lerp(skyColor1.rgb, skyColor2.rgb, smoothCurve(saturate(0.6 * height)));
    float horizonAmount = smoothCurve(saturate(2 * height + 1));
    float3 horizon = 0.2 * skyColor1.rgb + 0.8 * fogColor.rgb;
    return float4(lerp(horizon, sky, horizonAmount), 1);
}
