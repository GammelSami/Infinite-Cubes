// Interface vertex-to-pixel colors and separate mask and object texture coordinates.
#ifndef INTERFACE_HLSLI
#define INTERFACE_HLSLI

struct PixelInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 maskCoord : TEXCOORD0;
    float2 textureCoord : TEXCOORD1;
};

#endif
