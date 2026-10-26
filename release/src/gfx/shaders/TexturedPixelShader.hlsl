cbuffer EffectTexture
{
    bool textureEnabled;
};

SamplerState TextureSampler;
Texture2D tex;

#include "Textured.hlsli"

float4 main(PixelInput input) : SV_TARGET
{
    if (textureEnabled)
        return tex.Sample(TextureSampler, input.texcoord) * input.color;
    return input.color;
}
