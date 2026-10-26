// Textured quad vertex-to-pixel interface.
#ifndef TEXTURED_HLSLI
#define TEXTURED_HLSLI

struct PixelInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

#endif
