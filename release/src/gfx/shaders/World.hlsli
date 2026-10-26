// World vertex-to-pixel interface, including the projected and view-space positions.
#ifndef WORLD_HLSLI
#define WORLD_HLSLI

struct PixelInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float4 clipPosition : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 lightDirection : TEXCOORD2;
    float3 viewPosition : TEXCOORD3;
};

#endif
