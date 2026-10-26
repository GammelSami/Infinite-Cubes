// Averages 2 x 2 texels of a render surface (m_downsample2PixelShader).
#include "Downsample.hlsli"

float4 main(float4 color : COLOR0, float2 texCoord : TEXCOORD0) : COLOR0
{
    float2 t0 = firstTexel(texCoord, 2);
    float2 t1 = t0 + textureScale;
    float4 sum = tex2D(textureSampler, float2(t0.x, t0.y)) + tex2D(textureSampler, float2(t1.x, t0.y)) +
                 tex2D(textureSampler, float2(t0.x, t1.y)) + tex2D(textureSampler, float2(t1.x, t1.y));
    return unpremultiplied(0.25 * sum) * color;
}
