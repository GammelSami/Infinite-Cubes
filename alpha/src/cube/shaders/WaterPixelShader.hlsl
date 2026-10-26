// CubeShader's water pixel shader (m_waterPixelShader): the chunks' water faces, which
// GameController's render draws last. A water colour that turns from turquoise to blue up the
// screen, scaled by the vertex colour's blue, plus a specular highlight of the sky's first colour
// (a half vector between the light, raised by 2 along z, and the camera; the vertex alpha masks
// it), fogged and faded into the sky gradient with the distance. Water grows opaque with the
// distance (0.8 near, plus 0.0001 of the squared camera distance). The sky blends in from a fog
// term of 0.5; the gradient and the horizon are the world pixel shader's (0.6, 2, 0.2 sky and 0.8
// fog); the highlight's exponent is 10.
#include "World.hlsli"

float4 main(PixelInput input) : COLOR0
{
    float2 amounts = float2(input.fog.z - 0.5, input.fog.y + 1);
    float depthAmount = saturate(amounts.y);
    float skyAmount = saturate(amounts.x + amounts.x);
    float3 sky = lerp(skyColor1.rgb, skyColor2.rgb, smoothCurve(saturate(0.6 * input.fog.y)));
    float horizonAmount = smoothCurve(saturate(2 * input.fog.y + 1));
    float3 horizon = 0.2 * skyColor1.rgb + 0.8 * fogColor.rgb;
    float3 water = lerp(float3(0, 0.6, 1), float3(0, 0.1, 1), depthAmount);
    float3 halfway = normalize(normalize(input.lightDirection + float3(0, 0, 2)) + normalize(input.cameraDirection));
    float3 specular = pow(saturate(halfway.z), 10) * input.color.a * skyColor1.rgb;
    float3 color = input.color.b * water + specular;
    sky = lerp(horizon, sky, horizonAmount);
    color = lerp(color, fogColor.rgb, saturate(input.fog.z));
    // The opacity is computed here, in the return: as an earlier statement it changes the schedule.
    return float4(lerp(color, sky, skyAmount), dot(input.cameraDirection, input.cameraDirection) * 0.0001 + 0.8);
}
