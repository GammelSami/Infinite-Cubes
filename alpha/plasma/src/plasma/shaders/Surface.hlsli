// Surface shaders sample the premultiplied color texture from sampler zero and divide RGB by alpha
// when alpha is positive.
#ifndef SURFACE_HLSLI
#define SURFACE_HLSLI

sampler2D textureSampler : register(s0);

// A render surface holds premultiplied colour: divide it out where the texel is not transparent.
float4 unpremultiplied(float4 color)
{
    if (color.a > 0)
        color.rgb *= 1.0 / color.a;
    return color;
}

#endif
