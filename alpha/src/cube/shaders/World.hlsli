// Shared by CubeShader's pixel shaders : their constants, the interpolants the world vertex shader
// writes, and the curve the sky gradients use. The constant names are  (each shader's
// CTAB keeps them); the registers follow this declaration order, as CubeShader::initialize sets
// them (skyColor1 0, skyColor2 1, fogColor 2, alpha 3, shininess 4).
#ifndef WORLD_HLSLI
#define WORLD_HLSLI

float4 skyColor1;
float4 skyColor2;
float4 fogColor;
float alpha;
float shininess;

// WorldVertexShader's outputs but the position.
struct PixelInput {
    float4 color : COLOR0;
    // X, y: the projected position, y raised by fogGradientTranslation (the height on the sky
    // gradient); z: the squared distance to the camera, w: its horizontal part, both times fogScale².
    float4 fog : TEXCOORD0;
    float3 normal : TEXCOORD1;          // the view-space normal, doubled
    float3 lightDirection : TEXCOORD2;  // lightDirection, unchanged
    float3 cameraDirection : TEXCOORD3; // from the vertex to cameraPosition, world space
};

// 3x² - 2x³, smoothstep's curve on [0, 1].
float smoothCurve(float amount)
{
    return 3 * amount * amount - 2 * amount * amount * amount;
}

#endif
