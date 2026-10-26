// CubeShader's default pixel shader (m_pixelShader): blocks, objects and creatures. The vertex
// colour is fogged towards fogColor over the camera distance, then towards the sky gradient behind
// it over the horizontal distance; shiny materials (shininess > 0, CubeShader::setShininess)
// shimmer with a cosine of the screen position and the normal. The numbers are
// hand-set: the sky fades in from a horizontal fog term of 0.6 (times 2.5), the gradient rises at
// 0.6 of the height and the horizon band at twice it, the fog starts at 0.025 (times 1.5) and
// covers at most three quarters, the shimmer is 0.3 strong (10 and 5 times the screen position),
// the horizon 0.2 sky, 0.8 fog.
#include "World.hlsli"

float4 main(PixelInput input) : COLOR0
{
    float height = input.fog.y;
    float skyAmount = saturate((input.fog.w - 0.6) * 2.5);
    float3 sky = lerp(skyColor1.rgb, skyColor2.rgb, smoothCurve(saturate(0.6 * height)));
    float fogAmount = saturate(1.5 * input.fog.z - 0.025);
    fogAmount = 1 - (1 - fogAmount) * (1 - fogAmount);
    float horizonAmount = smoothCurve(saturate(2 * height + 1));
    skyAmount = smoothCurve(skyAmount);
    skyAmount *= skyAmount;
    float shimmer = 1 + 0.3 * cos(10 * input.fog.y + 5 * input.fog.x + input.normal.y + input.normal.x);
    float3 horizon = 0.2 * skyColor1.rgb + 0.8 * fogColor.rgb;
    sky = lerp(horizon, sky, horizonAmount);
    float3 color = lerp(input.color.rgb, fogColor.rgb, fogAmount * 0.75);
    color = lerp(color, sky, skyAmount);
    if (shininess > 0)
        color *= shimmer;
    return float4(color, alpha);
}
