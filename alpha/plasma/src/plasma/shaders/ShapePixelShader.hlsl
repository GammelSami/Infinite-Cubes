// The pixel shader of shapes and widgets (m_shapePixelShader), ported from the 2012 engine's GLSL
// shape shader. The vertex colour, optionally textured (saturation, brightness and contrast
// adjustments, then blended in by textureOpacity), then filtered by the mask texture: filter 0
// none, 1 clip (D3D9_FILTER_CLIP: the mask's colour over the shape by the mask's alpha), 2
// modulation (D3D9_FILTER_MODULATION), 3 screen (D3D9_FILTER_SCREEN: the mask's alpha only).
// Registers as D3D9Engine::setupShadersAndFrameBuffer sets them (m_textureEnabledRegister 0,
// m_filterRegister 0.. m_textureSaturationRegister 4); the samplers are bound explicitly. The grey
// of the saturation is the luminance with the weights 0.3, 0.59 and 0.11.
sampler2D maskSampler : register(s0);
sampler2D textureSampler : register(s1);
bool textureEnabled;
int filter;
float textureOpacity;
float textureBrightness;
float textureContrast;
float textureSaturation;

float4 main(float4 color : COLOR0, float2 maskCoord : TEXCOORD0, float2 texCoord : TEXCOORD1) : COLOR0
{
    float4 shapeColor;
    if (textureEnabled) {
        float4 texColor = tex2D(textureSampler, texCoord);
        if (textureSaturation != 1.0) {
            float3 weights = float3(0.3, 0.59, 0.11);
            float3 gray = dot(weights, texColor.xyz);
            texColor.xyz = lerp(texColor.xyz, gray, 1 - textureSaturation);
        }
        if (textureBrightness != 1.0)
            texColor.xyz *= textureBrightness;
        if (textureContrast != 1.0)
            texColor.xyz = lerp(texColor.xyz, float3(0.5, 0.5, 0.5), 1 - textureContrast);
        texColor.xyz = min(texColor.xyz, float3(1, 1, 1));
        shapeColor = lerp(color, color * texColor, textureOpacity);
    } else {
        // The GLSL used the colour alone; the code scales it by the contrast register (c3).
        shapeColor = color * textureContrast;
    }
    if (filter == 0)
        return shapeColor;
    float4 mask = tex2D(maskSampler, maskCoord);
    if (mask.a > 0)
        mask.xyz *= 1.0 / mask.a;
    if (filter == 1)
        return lerp(shapeColor, float4(mask.r, mask.g, mask.b, shapeColor.a), mask.a);
    else if (filter == 2)
        return mask * shapeColor;
    else
        return float4(shapeColor.r, shapeColor.g, shapeColor.b, mask.a * shapeColor.a);
}
