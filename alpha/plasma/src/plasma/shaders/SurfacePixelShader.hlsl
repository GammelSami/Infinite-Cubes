// Draws a render-surface texel, undoing premultiplied alpha before multiplying by vertex color.
#include "Surface.hlsli"

float4 main(float4 color : COLOR0, float2 texCoord : TEXCOORD0) : COLOR0
{
    return unpremultiplied(tex2D(textureSampler, texCoord)) * color;
}
