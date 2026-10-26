// The horizontal pass of the render-surface blur (m_horizontalBlurPixelShader): nine taps
// blurScale apart (a quarter of the blur radius over the surface width, set by the C++), weighted
// as in the 2012 engine's GLSL blur.
#include "Surface.hlsli"

float blurScale;

float4 main(float2 texCoord : TEXCOORD0) : COLOR0
{
    float4 sum = float4(0, 0, 0, 0);
    sum += tex2D(textureSampler, float2(texCoord.x - 4.0 * blurScale, texCoord.y)) * 0.05;
    sum += tex2D(textureSampler, float2(texCoord.x - 3.0 * blurScale, texCoord.y)) * 0.09;
    sum += tex2D(textureSampler, float2(texCoord.x - 2.0 * blurScale, texCoord.y)) * 0.12;
    sum += tex2D(textureSampler, float2(texCoord.x - blurScale, texCoord.y)) * 0.15;
    sum += tex2D(textureSampler, float2(texCoord.x, texCoord.y)) * 0.18;
    sum += tex2D(textureSampler, float2(texCoord.x + blurScale, texCoord.y)) * 0.15;
    sum += tex2D(textureSampler, float2(texCoord.x + 2.0 * blurScale, texCoord.y)) * 0.12;
    sum += tex2D(textureSampler, float2(texCoord.x + 3.0 * blurScale, texCoord.y)) * 0.09;
    sum += tex2D(textureSampler, float2(texCoord.x + 4.0 * blurScale, texCoord.y)) * 0.05;
    return sum;
}
