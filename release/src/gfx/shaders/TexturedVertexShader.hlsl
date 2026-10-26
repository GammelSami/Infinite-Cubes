#include "Transforms.hlsli"

#include "Textured.hlsli"

PixelInput main(float4 position : POSITION, float4 color : COLOR, float2 texcoord : TEXCOORD)
{
    PixelInput output;
    float4x4 worldViewProjection = mul(mul(worldMatrix, viewMatrix), projMatrix);
    output.position = mul(position, worldViewProjection);
    output.color = color;
    output.texcoord = texcoord;
    return output;
}
