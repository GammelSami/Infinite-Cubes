cbuffer Proj
{
    float4x4 projMatrix;
};

cbuffer World
{
    float4x4 worldMatrix;
};

cbuffer Mask
{
    float4x4 maskMatrix;
};

cbuffer Texture
{
    float4x4 textureMatrix;
};

cbuffer WidgetDeformationEnabled
{
    bool widgetDeformationEnabled;
};

cbuffer WidgetBindMatrix
{
    float4x4 widgetBindMatrix;
};

cbuffer InverseWidgetBindMatrix
{
    float4x4 inverseWidgetBindMatrix;
};

cbuffer WidgetBindPos
{
    float2 widgetBindPos;
};

cbuffer WidgetBindSize
{
    float2 widgetBindSize;
};

cbuffer DeformedWidgetPos
{
    float2 deformedWidgetPos;
};

cbuffer DeformedWidgetSize
{
    float2 deformedWidgetSize;
};

cbuffer BaseColor
{
    float4 baseColor;
};

#include "Interface.hlsli"

PixelInput main(float4 position : POSITION, float4 color : COLOR)
{
    PixelInput output;
    float4 vertex = float4(position.xy, 0, 1);
    if (widgetDeformationEnabled)
    {
        float4 bindPosition = mul(vertex, widgetBindMatrix);
        float2 amount;
        amount.x = bindPosition.x < widgetBindPos.x ? 0 :
            (bindPosition.x < widgetBindPos.x + widgetBindSize.x ?
            (bindPosition.x - widgetBindPos.x) / widgetBindSize.x : 1);
        amount.y = bindPosition.y < widgetBindPos.y ? 0 :
            (bindPosition.y < widgetBindPos.y + widgetBindSize.y ?
            (bindPosition.y - widgetBindPos.y) / widgetBindSize.y : 1);
        bindPosition.xy = bindPosition.xy + deformedWidgetPos - widgetBindPos +
            amount * (deformedWidgetSize - widgetBindSize);
        vertex = mul(bindPosition, inverseWidgetBindMatrix);
    }
    float4 worldPosition = mul(vertex, worldMatrix);
    // Interface vertex pixel alignment offset, applied equally to screen x and y.
    worldPosition.xy -= 0.135;
    output.position = mul(worldPosition, projMatrix);
    output.color = color * baseColor;
    output.maskCoord = mul(vertex, maskMatrix).xy;
    output.textureCoord = mul(float4(position.zw, 0, 1), textureMatrix).xy;
    return output;
}
