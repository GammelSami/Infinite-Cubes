cbuffer TextureEnabled
{
    bool textureEnabled;
};

// filter: plasma::FilterMode (D3D11Engine.h); FILTER_NONE (0) skips the mask and FILTER_SCREEN (3)
// is the remaining case.
cbuffer Filter
{
    int filter;
};
static const int FILTER_CLIP = 1;
static const int FILTER_MODULATION = 2;

cbuffer TextureParameters
{
    float textureOpacity;
    float textureBrightness;
    float textureContrast;
    float textureSaturation;
};

SamplerState MaskTextureSampler;
SamplerState ObjectTextureSampler;
Texture2D maskTexture;
Texture2D objectTexture;

#include "Interface.hlsli"

float4 main(PixelInput input) : SV_TARGET
{
    float4 color = input.color;
    if (textureEnabled)
    {
        float4 textureColor = objectTexture.Sample(ObjectTextureSampler, input.textureCoord);
        if (textureSaturation != 1)
        {
            // RGB-to-gray luminance palette weights (red 0.3, green 0.59, blue 0.11).
            float gray = dot(float3(0.3, 0.59, 0.11), textureColor.rgb);
            textureColor.rgb = lerp(textureColor.rgb, gray, 1 - textureSaturation);
        }
        if (textureBrightness != 1)
            textureColor.rgb *= textureBrightness;
        if (textureContrast != 1)
            textureColor.rgb = lerp(textureColor.rgb, 0.5, 1 - textureContrast);
        textureColor.rgb = min(textureColor.rgb, 1);
        color = lerp(color, color * textureColor, textureOpacity);
    }
    if (filter)
    {
        float4 maskColor = maskTexture.Sample(MaskTextureSampler, input.maskCoord);
        if (maskColor.a > 0)
            maskColor.rgb *= 1 / maskColor.a;
        if (filter == FILTER_CLIP)
            return lerp(color, float4(maskColor.rgb, color.a), maskColor.a);
        else if (filter == FILTER_MODULATION)
            return color * maskColor;
        else
            return float4(color.rgb, color.a * maskColor.a);
    }
    return color;
}
