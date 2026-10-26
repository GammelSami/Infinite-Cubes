// The vertex shader of the render-surface passes (m_surfaceVertexShader): drawing a surface,
// downsampling and blurring it. Input: the shape vertex declaration (m_shapeDeclaration), of which
// it reads the position, the colour and the third texture coordinate.
float4x4 Proj;

struct Output {
    float4 position : POSITION;
    float4 color : COLOR0;
    float2 texCoord : TEXCOORD0;
};

Output main(float4 position : POSITION, float4 color : COLOR0, float2 texCoord : TEXCOORD2)
{
    Output output;
    // The same offset as the shape vertex shader's, applied before the projection.
    output.position = mul(position + float4(-0.135, -0.135, 0, 0), Proj);
    output.color = color;
    output.texCoord = texCoord;
    return output;
}
