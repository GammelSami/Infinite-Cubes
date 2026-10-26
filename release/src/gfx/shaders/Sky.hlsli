// Sky gradient, sun and final color grading; requires the sky, angle and sun buffers.
#ifndef SKY_HLSLI
#define SKY_HLSLI

float smoothCurve(float amount)
{
    return 3 * amount * amount - 2 * amount * amount * amount;
}

float3 skyColorAt(float2 projectedPosition)
{
    projectedPosition.y += fogGradientTranslation;
    // Sun disc and halo profile: inverse squared radius and fourth-power halo falloff.
    float sunX = (sunPos.x - projectedPosition.x) * sunPos.z;
    float sunY = sunPos.y - projectedPosition.y;
    // Sky gradient and horizon palette: linear slopes, offsets and fog-colour mixing weights.
    float3 sky = lerp(skyColor1.rgb, skyColor2.rgb,
        smoothCurve(saturate(0.4 * projectedPosition.y + 0.2)));
    sky = lerp(0.4 * skyColor1.rgb + 0.6 * fogColor.rgb, sky,
        smoothCurve(saturate(2 * projectedPosition.y + 1.5)));
    float sunAmount = (1 - min(4 * dot(float2(sunX, sunY), float2(sunX, sunY)), 1)) * sunPos.w;
    return lerp(sunColor.rgb, sky, 1 - pow(sunAmount, 4));
}

float3 gradeColor(float3 color)
{
    float luminance = dot(float3(0.299, 0.587, 0.114), color);
    // Desaturation palette: green/blue weights 0.65/0.35.
    color.gb = color.gb * 0.65 + luminance * 0.35;
    return 3 * color * color - 2 * color * color * color;
}

#endif
