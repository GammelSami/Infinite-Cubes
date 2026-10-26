#include "Transforms.hlsli"

cbuffer Material
{
    float4 materialColor;
};

cbuffer PointLights
{
    float4 pointLightPositions[10];
    float4 pointLightColors[10];
};

cbuffer Light
{
    float4 lightFrontColor;
    float4 lightBackColor;
    float4 lightDirection;
};

cbuffer Ambient
{
    float4 ambientColor;
};

cbuffer White
{
    float white;
};

cbuffer DepthOffset
{
    float depthOffset;
};

cbuffer Glow
{
    float glow;
};

#include "World.hlsli"

PixelInput main(float4 position : POSITION, float4 color : COLOR)
{
    PixelInput output;
    // The fourth normalized byte identifies an axis-aligned voxel face.
    float3 normal = position.w == 0 ? float3(1, 0, 0) :
        position.w == 1.0 / 255 ? float3(-1, 0, 0) :
        position.w == 2.0 / 255 ? float3(0, 1, 0) :
        position.w == 3.0 / 255 ? float3(0, -1, 0) :
        position.w == 4.0 / 255 ? float3(0, 0, 1) : float3(0, 0, -1);
    float4x4 worldView = mul(worldMatrix, viewMatrix);
    float4x4 worldViewProjection = mul(worldView, projMatrix);
    worldViewProjection[3][2] -= depthOffset;
    output.normal = normalize(mul(normal, (float3x3)worldView));
    float3 worldNormal = normalize(mul(normal, (float3x3)worldMatrix));
    // Front light response: incidence slope 0.7, floor 0.3.
    float frontLight = max(0.7 * dot(worldNormal, lightDirection.xyz) + 0.3, 0);
    // Back light direction in model coordinates; artistic lighting vector.
    float backLight = 0.5 * abs(dot(worldNormal, normalize(float3(0.6, 0.4, 0.7)))) + 0.5;
    float3 directionalLight = saturate(frontLight * lightFrontColor.rgb + backLight * lightBackColor.rgb);
    float4 vertex = float4(position.xyz * 255, 1);
    output.position = mul(vertex, worldViewProjection);
    float3 worldPosition = mul(vertex, worldMatrix).xyz;
    output.viewPosition = mul(vertex, worldView).xyz;
    float3 lighting = directionalLight * (color.a * materialColor.a) + backLight * ambientColor.rgb;
    float3 pointLighting = 0;
    for (int light = 0; light < 10; ++light)
    {
        if (pointLightPositions[light].w > 0)
        {
            float3 distance = worldPosition - pointLightPositions[light].xyz;
            float attenuation = 1 - min(dot(distance, distance) /
                (pointLightPositions[light].w * pointLightPositions[light].w), 1);
            // Point light response: incidence gain -4, floor 0.3.
            float incidence = min(max(-4 * dot(worldNormal, normalize(distance)), 0.3), 1);
            pointLighting += incidence * (attenuation * attenuation) * pointLightColors[light].rgb;
        }
    }
    lighting = saturate(lighting + pointLighting);
    // Vertex colour intensity and saturation curve: double intensity, half-strength saturation gain
    // capped at two.
    float3 vertexColor = 2 * color.rgb;
    float saturation = min(max(1 + (ambientColor.a - lighting.g) * 0.5, 1), 2);
    float luminance = dot(float3(0.299, 0.587, 0.114), vertexColor);
    float3 surfaceColor = min(max(saturation * vertexColor + luminance * (1 - saturation), 0), 2);
    if (glow < 0)
        surfaceColor = luminance;
    float3 litColor = surfaceColor * (lighting * materialColor.rgb);
    if (glow > 0)
        litColor = lerp(litColor, vertexColor * materialColor.rgb, glow);
    output.color.rgb = saturate(lerp(litColor, 1, white));
    output.color.a = color.a;
    output.lightDirection = mul(lightDirection.xyz, (float3x3)worldView);
    output.clipPosition = output.position;
    return output;
}
