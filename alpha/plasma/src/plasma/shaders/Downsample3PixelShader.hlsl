// Averages 3 x 3 texels of a render surface (m_downsample3PixelShader).
#include "Downsample.hlsli"

float4 main(float4 color : COLOR0, float2 texCoord : TEXCOORD0) : COLOR0
{
    float2 t0 = firstTexel(texCoord, 3);
    float2 t1 = t0 + textureScale;
    float2 t2 = t0 + 2 * textureScale;
    float4 sum = tex2D(textureSampler, float2(t0.x, t0.y)) + tex2D(textureSampler, float2(t1.x, t0.y)) +
                 tex2D(textureSampler, float2(t2.x, t0.y)) +
                 tex2D(textureSampler, float2(t0.x, t1.y)) + tex2D(textureSampler, float2(t1.x, t1.y)) +
                 tex2D(textureSampler, float2(t2.x, t1.y)) +
                 tex2D(textureSampler, float2(t0.x, t2.y)) + tex2D(textureSampler, float2(t1.x, t2.y)) +
                 tex2D(textureSampler, float2(t2.x, t2.y));
    return unpremultiplied(1 / 9.0 * sum) * color;
}
