#include <algorithm>
#include "gfx/RendererIds.h"
#include <d3d11.h>
#include <math.h>
#include <time.h>
#include <algorithm>
#include "gfx/D3D11Renderer.h"
#include "cube/World.h"
#include "cube/MathConstants.h"
#include "gfx/VertexBuffer.h"
#include "plasma/D3D11Engine.h"
#include "plasma/EditFilter.h"

namespace gfx {

static const float RingTextureWaveColorAmplitude = 30.0f;
static const float RingTextureMaximumAlpha = 200.0f;
static const float GroundSpotSurfaceLift = 0.0035f;
static const int GroundShadowMaximumOpacity = 100;
static const float TrailOuterMaximumOpacity = 50.0f;
static const float TrailInnerMaximumOpacity = 200.0f;

D3D11Renderer::D3D11Renderer(cube::World *world, cube::WorldMap *worldMap, plasma::D3D11Engine *engine,
                             ID3D11Device *device, ID3D11DeviceContext *context)
    : Renderer(world, worldMap),
      m_engine(engine),
      m_device(device),
      m_context(context),
      m_shader(device, context),
      m_dynamicVertexBuffer(nullptr),
      m_dynamicIndexBuffer(nullptr),
      m_spotTexture(nullptr),
      m_ringTexture(nullptr)
{
    D3D11_RASTERIZER_DESC rasterizer;
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_FRONT;
    rasterizer.FrontCounterClockwise = FALSE;
    rasterizer.DepthBias = 0;
    rasterizer.DepthBiasClamp = 0.0f;
    rasterizer.SlopeScaledDepthBias = 0.0f;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.ScissorEnable = FALSE;
    rasterizer.MultisampleEnable = FALSE;
    rasterizer.AntialiasedLineEnable = FALSE;
    m_device->CreateRasterizerState(&rasterizer, &m_rasterizerStates[RasterizerFront]);
    rasterizer.FrontCounterClockwise = TRUE;
    m_device->CreateRasterizerState(&rasterizer, &m_rasterizerStates[RasterizerBack]);
    rasterizer.CullMode = D3D11_CULL_NONE;
    m_device->CreateRasterizerState(&rasterizer, &m_rasterizerStates[RasterizerTwoSided]);

    D3D11_DEPTH_STENCIL_DESC depthStencil;
    depthStencil.DepthEnable = TRUE;
    depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencil.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    depthStencil.StencilEnable = FALSE;
    depthStencil.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    depthStencil.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    depthStencil.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencil.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthStencil.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencil.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    depthStencil.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencil.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depthStencil.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencil.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    m_device->CreateDepthStencilState(&depthStencil, &m_depthStencilStates[DepthTestWrite]);
    depthStencil.DepthEnable = FALSE;
    m_device->CreateDepthStencilState(&depthStencil, &m_depthStencilStates[DepthDisabled]);
    depthStencil.DepthEnable = TRUE;
    depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    m_device->CreateDepthStencilState(&depthStencil, &m_depthStencilStates[DepthTestNoWrite]);

    D3D11_BLEND_DESC blend;
    ZeroMemory(&blend, sizeof(blend));
    blend.RenderTarget[0].BlendEnable = TRUE;
    blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_device->CreateBlendState(&blend, &m_blendStates[BlendAlpha]);
    blend.RenderTarget[0].BlendEnable = FALSE;
    m_device->CreateBlendState(&blend, &m_blendStates[BlendOpaque]);
    blend.RenderTarget[0].RenderTargetWriteMask = 0;
    m_device->CreateBlendState(&blend, &m_blendStates[BlendDepthOnly]);

    // The unit quad in the z = 0 plane that drawSky draws.
    D3D11_BUFFER_DESC vertexDescription;
    ZeroMemory(&vertexDescription, sizeof(vertexDescription));
    vertexDescription.Usage = D3D11_USAGE_DEFAULT;
    vertexDescription.ByteWidth = sizeof(Vertex) * 4;
    vertexDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDescription.CPUAccessFlags = 0;
    vertexDescription.MiscFlags = 0;
    Vertex vertices[4] = {
        Vertex(plasma::Vector<3, int>(0, 0, 0), plasma::Vector<3, int>(0, 0, 1),
               plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)),
        Vertex(plasma::Vector<3, int>(1, 0, 0), plasma::Vector<3, int>(0, 0, 1),
               plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)),
        Vertex(plasma::Vector<3, int>(1, 1, 0), plasma::Vector<3, int>(0, 0, 1),
               plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)),
        Vertex(plasma::Vector<3, int>(0, 1, 0), plasma::Vector<3, int>(0, 0, 1),
               plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)),
    };
    D3D11_SUBRESOURCE_DATA vertexData;
    ZeroMemory(&vertexData, sizeof(vertexData));
    vertexData.pSysMem = vertices;
    m_device->CreateBuffer(&vertexDescription, &vertexData, &m_quadVertexBuffer);

    D3D11_BUFFER_DESC indexDescription;
    ZeroMemory(&indexDescription, sizeof(indexDescription));
    indexDescription.Usage = D3D11_USAGE_DEFAULT;
    indexDescription.ByteWidth = sizeof(unsigned int) * 6;
    indexDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexDescription.CPUAccessFlags = 0;
    indexDescription.MiscFlags = 0;
    int indices[] = {0, 1, 2, 0, 2, 3};
    D3D11_SUBRESOURCE_DATA indexData;
    ZeroMemory(&indexData, sizeof(indexData));
    indexData.pSysMem = indices;
    m_device->CreateBuffer(&indexDescription, &indexData, &m_quadIndexBuffer);

    // The buffers drawDynamic fills: 256 textured vertices and 256 indices.
    ZeroMemory(&vertexDescription, sizeof(vertexDescription));
    vertexDescription.Usage = D3D11_USAGE_DYNAMIC;
    vertexDescription.ByteWidth = sizeof(TexturedVertex) * 256;
    vertexDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    m_device->CreateBuffer(&vertexDescription, nullptr, &m_dynamicVertexBuffer);
    ZeroMemory(&indexDescription, sizeof(indexDescription));
    indexDescription.Usage = D3D11_USAGE_DYNAMIC;
    indexDescription.ByteWidth = sizeof(unsigned int) * 256;
    indexDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    m_device->CreateBuffer(&indexDescription, nullptr, &m_dynamicIndexBuffer);

    D3D11_TEXTURE2D_DESC textureDescription;
    ZeroMemory(&textureDescription, sizeof(textureDescription));
    textureDescription.Width = 256;
    textureDescription.Height = 256;
    textureDescription.MipLevels = 1;
    textureDescription.ArraySize = 1;
    textureDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDescription.SampleDesc.Count = 1;
    textureDescription.SampleDesc.Quality = 0;
    textureDescription.Usage = D3D11_USAGE_DEFAULT;
    textureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDescription.CPUAccessFlags = 0;
    textureDescription.MiscFlags = 0;
    std::vector<plasma::Vector<4, unsigned char>> texels(256 * 256); // R8G8B8A8

    // A soft round spot: white, with alpha falling from the centre.
    plasma::Vector<4, unsigned char> texel;
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            float dx = (float)x - 127.5f;
            float dy = (float)y - 127.5f;
            float value = std::max(0.0f, 1.0f - (dx * dx + dy * dy) / (128.0f * 128.0f));
            texel[0] = 255;
            texel[1] = 255;
            texel[2] = 255;
            value *= value * value;
            texel[3] = (unsigned char)(value * 255.0f);
            texels[y * 256 + x] = texel;
        }
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
    D3D11_SUBRESOURCE_DATA textureData;
    ZeroMemory(&textureData, sizeof(textureData));
    textureData.pSysMem = &texels[0];
    textureData.SysMemPitch = 256 * sizeof(texel);
    textureData.SysMemSlicePitch = 0;
    ID3D11Texture2D *texture;
    m_device->CreateTexture2D(&textureDescription, &textureData, &texture);
    ZeroMemory(&viewDescription, sizeof(viewDescription));
    viewDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    viewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    viewDescription.Texture2D.MipLevels = 1;
    m_device->CreateShaderResourceView(texture, &viewDescription, &m_spotTexture);
    texture->Release();

    // Concentric rings that fade out towards the edge.
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            float dx = (float)x - 127.5f;
            float dy = (float)y - 127.5f;
            float radius = std::min(1.0f, sqrtf(dx * dx + dy * dy) / 128.0f);
            float wave = cosf((float)(radius * 10.0f * cube::Pi)) * RingTextureWaveColorAmplitude;
            float fade = 1.0f - radius * radius * radius;
            // Ring texture RGB palette: (220,190,200), with half wave amplitude in green and blue.
            texel[0] = (unsigned char)(wave + 220.0f);
            texel[1] = (unsigned char)(wave * 0.5f + 190.0f);
            texel[2] = (unsigned char)(wave * 0.5f + 200.0f);
            texel[3] = (unsigned char)(fade * RingTextureMaximumAlpha);
            texels[y * 256 + x] = texel;
        }
    }
    // This texture is not released (the view keeps it alive either way).
    m_device->CreateTexture2D(&textureDescription, &textureData, &texture);
    m_device->CreateShaderResourceView(texture, &viewDescription, &m_ringTexture);
}

D3D11Renderer::~D3D11Renderer()
{
    // The dynamic buffers are not released.
    m_rasterizerStates[RasterizerFront]->Release();
    m_rasterizerStates[RasterizerBack]->Release();
    m_rasterizerStates[RasterizerTwoSided]->Release();
    m_depthStencilStates[DepthTestWrite]->Release();
    m_depthStencilStates[DepthDisabled]->Release();
    m_depthStencilStates[DepthTestNoWrite]->Release();
    m_blendStates[BlendAlpha]->Release();
    m_blendStates[BlendOpaque]->Release();
    m_blendStates[BlendDepthOnly]->Release();
    m_quadVertexBuffer->Release();
    m_quadIndexBuffer->Release();
    m_spotTexture->Release();
    m_ringTexture->Release();
}

void D3D11Renderer::activateMode(int mode)
{
    switch (mode) {
    case RendererMode_OpaqueFront:
        m_context->OMSetDepthStencilState(m_depthStencilStates[DepthTestWrite], 0);
        m_context->RSSetState(m_rasterizerStates[RasterizerFront]);
        m_context->OMSetBlendState(m_blendStates[BlendOpaque], 0, 0xffffffff);
        m_shader.apply();
        break;
    case RendererMode_OpaqueBack:
        m_context->OMSetDepthStencilState(m_depthStencilStates[DepthTestWrite], 0);
        m_context->RSSetState(m_rasterizerStates[RasterizerBack]);
        m_context->OMSetBlendState(m_blendStates[BlendOpaque], 0, 0xffffffff);
        m_shader.apply();
        break;
    case RendererMode_OpaqueTwoSided:
        m_context->OMSetDepthStencilState(m_depthStencilStates[DepthTestWrite], 0);
        m_context->RSSetState(m_rasterizerStates[RasterizerTwoSided]);
        m_context->OMSetBlendState(m_blendStates[BlendOpaque], 0, 0xffffffff);
        m_shader.apply();
        break;
    case RendererMode_DepthFront:
        m_context->OMSetDepthStencilState(m_depthStencilStates[DepthTestWrite], 0);
        m_context->RSSetState(m_rasterizerStates[RasterizerFront]);
        m_context->OMSetBlendState(m_blendStates[BlendDepthOnly], 0, 0xffffffff);
        m_shader.apply();
        break;
    case RendererMode_DepthBack:
        m_context->OMSetDepthStencilState(m_depthStencilStates[DepthTestWrite], 0);
        m_context->OMSetBlendState(m_blendStates[BlendDepthOnly], 0, 0xffffffff);
        m_context->RSSetState(m_rasterizerStates[RasterizerBack]);
        m_shader.apply();
        break;
    case RendererMode_DepthTwoSided:
        m_context->OMSetDepthStencilState(m_depthStencilStates[DepthTestWrite], 0);
        m_context->OMSetBlendState(m_blendStates[BlendDepthOnly], 0, 0xffffffff);
        m_context->RSSetState(m_rasterizerStates[RasterizerTwoSided]);
        m_shader.apply();
        break;
    case RendererMode_AlphaFront:
        m_context->OMSetDepthStencilState(m_depthStencilStates[DepthTestWrite], 0);
        m_context->RSSetState(m_rasterizerStates[RasterizerFront]);
        m_context->OMSetBlendState(m_blendStates[BlendAlpha], 0, 0xffffffff);
        m_shader.apply();
        break;
    case RendererMode_AlphaBack:
        m_context->OMSetDepthStencilState(m_depthStencilStates[DepthTestWrite], 0);
        m_context->RSSetState(m_rasterizerStates[RasterizerBack]);
        m_context->OMSetBlendState(m_blendStates[BlendAlpha], 0, 0xffffffff);
        m_shader.apply();
        break;
    case RendererMode_Sky:
        m_context->OMSetDepthStencilState(m_depthStencilStates[DepthDisabled], 0);
        m_context->RSSetState(m_rasterizerStates[RasterizerTwoSided]);
        m_context->OMSetBlendState(m_blendStates[BlendOpaque], 0, 0xffffffff);
        m_shader.applySky();
        break;
    case RendererMode_Water:
        m_context->RSSetState(m_rasterizerStates[RasterizerTwoSided]);
        m_context->OMSetDepthStencilState(m_depthStencilStates[DepthTestWrite], 0);
        m_context->OMSetBlendState(m_blendStates[BlendAlpha], 0, 0xffffffff);
        m_shader.applyWater();
        break;
    case RendererMode_Clouds:
        m_context->OMSetDepthStencilState(m_depthStencilStates[DepthTestWrite], 0);
        m_context->RSSetState(m_rasterizerStates[RasterizerFront]);
        m_context->OMSetBlendState(m_blendStates[BlendAlpha], 0, 0xffffffff);
        m_shader.applyCloud();
        break;
    }
}

void D3D11Renderer::drawDynamic(const std::vector<TexturedVertex> &vertices, const std::vector<int> &indices)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    m_context->Map(m_dynamicVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, &vertices[0], vertices.size() * sizeof(TexturedVertex));
    m_context->Unmap(m_dynamicVertexBuffer, 0);
    m_context->Map(m_dynamicIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, &indices[0], indices.size() * sizeof(int));
    m_context->Unmap(m_dynamicIndexBuffer, 0);
    UINT stride = sizeof(TexturedVertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_dynamicVertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(m_dynamicIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_context->DrawIndexed((UINT)indices.size(), 0, 0);
}

void D3D11Renderer::drawGroundSpots()
{
    m_shader.applyQuads();
    m_shader.enableTexture(true);
    m_context->OMSetBlendState(m_blendStates[BlendAlpha], 0, 0xffffffff);
    m_context->OMSetDepthStencilState(m_depthStencilStates[DepthTestNoWrite], 0);
    m_context->RSSetState(m_rasterizerStates[RasterizerTwoSided]);
    for (GroundSpot &spot : m_groundSpots) {
        plasma::Vector<3, int> block = cube::fixedToBlock(spot.position);
        float radius = spot.radius;
        float scale = 1.0f / (radius + radius);
        if (spot.kind == 0)
            m_context->PSSetShaderResources(0, 1, &m_spotTexture);
        else
            m_context->PSSetShaderResources(0, 1, &m_ringTexture);
        int extent = (int)(radius + 1.0f);
        plasma::Vector<4, unsigned char> color((int)(spot.color[0] * 255.0f), (int)(spot.color[1] * 255.0f),
                                               (int)(spot.color[2] * 255.0f), (int)(spot.color[3] * 255.0f));
        for (int dx = -extent; dx <= extent; dx++) {
            for (int dy = -extent; dy <= extent; dy++) {
                // The quad lies on the first block below the spot that is neither air nor water; none within 100
                // blocks, or a spot inside the ground, draws nothing.
                int x = dx + block[0];
                int y = dy + block[1];
                int z = block[2];
                int steps = 0;
                cube::Block below = m_world->getBlock(x, y, z);
                if (below.isSolid())
                    continue;
                while (!below.isSolid()) {
                    z--;
                    below = m_world->getBlock(x, y, z);
                    steps++;
                    if (steps > 100)
                        break;
                }
                if (steps > 100)
                    continue;
                z++;
                float u = ((float)dx + radius - (float)(spot.position[0] - cube::Fixed(block[0]))) * scale;
                float v = ((float)dy + radius - (float)(spot.position[1] - cube::Fixed(block[1]))) * scale;
                plasma::Matrix<float> world;
                world.identity();
                world.translate((float)(cube::Fixed(block[0]) + m_drawOriginX),
                                (float)(cube::Fixed(block[1]) + m_drawOriginY), (float)z + GroundSpotSurfaceLift);
                m_shader.setWorldMatrix(world);
                std::vector<int> indices = {0, 1, 2, 0, 3, 2};
                TexturedVertex vertexData[4] = {
                    {plasma::Vector<4, float>((float)dx, (float)dy, 0.0f, 1.0f), color, plasma::Vector<2, float>(u, v)},
                    {plasma::Vector<4, float>((float)(dx + 1), (float)dy, 0.0f, 1.0f), color,
                     plasma::Vector<2, float>(u + scale, v)},
                    {plasma::Vector<4, float>((float)(dx + 1), (float)(dy + 1), 0.0f, 1.0f), color,
                     plasma::Vector<2, float>(u + scale, v + scale)},
                    {plasma::Vector<4, float>((float)dx, (float)(dy + 1), 0.0f, 1.0f), color,
                     plasma::Vector<2, float>(u, v + scale)},
                };
                std::vector<TexturedVertex> vertices(vertexData, vertexData + 4);
                drawDynamic(vertices, indices);
            }
        }
    }
}

void D3D11Renderer::drawInterface()
{
    m_engine->drawWindow(0);
}

void D3D11Renderer::drawShadows(const plasma::Vector<4, float> &lightColor)
{
    m_shader.applyQuads();
    m_shader.enableTexture(false);
    m_context->OMSetBlendState(m_blendStates[BlendAlpha], 0, 0xffffffff);
    m_context->OMSetDepthStencilState(m_depthStencilStates[DepthTestNoWrite], 0);
    m_context->RSSetState(m_rasterizerStates[RasterizerTwoSided]);
    plasma::Matrix<float> world;
    world.identity();
    m_shader.setWorldMatrix(world);
    float lift = 0.0f;
    for (GroundShadow &shadow : m_shadows) {
        // The shadow points away from the sun's compass direction.
        float angle = (float)(m_cameraAngle[2] / 180.0f * cube::Pi);
        float sine = std::sin(angle);
        float cosine = std::cos(angle);
        // Shadow opacity is multiplied by the C runtime's daylight-saving flag (_daylight). In a time zone without
        // daylight saving, shadows are invisible.
        int alpha =
            (int)(attenuation(shadow.position, m_effectViewDistance) * (float)(GroundShadowMaximumOpacity * _daylight));
        std::vector<int> indices = {0, 1, 2, 2, 1, 3};

        // The near edge, size wide, at the shadow's position, and the far edge, transparent, height lower and size *
        // height / 4 further.
        TexturedVertex vertexData[4] = {
            {plasma::Vector<4, float>((float)(m_drawOriginX + shadow.position[0] - cosine * 0.5f * shadow.size),
                                      (float)(shadow.position[1] + m_drawOriginY - sine * 0.5f * shadow.size),
                                      (float)shadow.position[2], 1.0f),
             plasma::Vector<4, unsigned char>(255, 255, 255, alpha), plasma::Vector<2, float>(0.0f, 0.0f)},
            {plasma::Vector<4, float>((float)(m_drawOriginX + shadow.position[0] + cosine * 0.5f * shadow.size),
                                      (float)(shadow.position[1] + m_drawOriginY + sine * 0.5f * shadow.size),
                                      (float)shadow.position[2], 1.0f),
             plasma::Vector<4, unsigned char>(255, 255, 255, alpha), plasma::Vector<2, float>(0.0f, 0.0f)},
            {plasma::Vector<4, float>(
                 (float)(m_drawOriginX + shadow.position[0] - cosine * shadow.size * 0.25f * shadow.height +
                         cube::Fixed((long long)lift)),
                 (float)(shadow.position[1] + m_drawOriginY - sine * shadow.size * 0.25f * shadow.height +
                         cube::Fixed((long long)lift)),
                 (float)(shadow.position[2] - shadow.height), 1.0f),
             plasma::Vector<4, unsigned char>(255, 255, 255, 0), plasma::Vector<2, float>(0.0f, 0.0f)},
            {plasma::Vector<4, float>(
                 (float)(m_drawOriginX + shadow.position[0] + cosine * shadow.size * 0.25f * shadow.height +
                         cube::Fixed((long long)lift)),
                 (float)(shadow.position[1] + m_drawOriginY + sine * shadow.size * 0.25f * shadow.height +
                         cube::Fixed((long long)lift)),
                 (float)(shadow.position[2] - shadow.height), 1.0f),
             plasma::Vector<4, unsigned char>(255, 255, 255, 0), plasma::Vector<2, float>(0.0f, 0.0f)},
        };
        std::vector<TexturedVertex> vertices(vertexData, vertexData + 4);
        drawDynamic(vertices, indices);
    }
}

void D3D11Renderer::drawSky()
{
    activateMode(RendererMode_Sky);
    // Passes two uninitialised point-light arrays; the sky shader does not use them.
    std::array<plasma::Vector<4, float>, 16> lightPositions;
    std::array<plasma::Vector<4, float>, 16> lightColors;
    m_shader.setPointLights(lightPositions, lightColors);
    m_shader.setShininess(0.0f);
    m_shader.setAlpha(1.0f);

    // The unit quad scaled to (-1, -1)..(1, 1) at depth 100, seen through an orthographic projection of the square
    // (-0.5, -0.5)..(0.5, 0.5) with depth 0.1..10000.
    plasma::Matrix<float> projection;
    projection.identity();
    projection[0] = 2.0f / (0.5f - -0.5f);
    projection[5] = 2.0f / (0.5f - -0.5f);
    projection[10] = 1.0f / (10000.0f - 0.1f);
    projection[11] = -0.1f / (10000.0f - 0.1f);
    plasma::Matrix<float> view;
    view.identity();
    plasma::Matrix<float> world;
    world.identity();
    world.translate(-1.0f, -1.0f, 100.0f);
    world.scale(2.0f, 2.0f, 2.0f);
    m_shader.setMatrices(world, view, projection);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_quadVertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(m_quadIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_context->DrawIndexed(6, 0, 0);
}

void D3D11Renderer::drawTrails()
{
    m_shader.applyQuads();
    m_shader.enableTexture(false);
    m_context->OMSetBlendState(m_blendStates[BlendAlpha], 0, 0xffffffff);
    m_context->OMSetDepthStencilState(m_depthStencilStates[DepthTestNoWrite], 0);
    m_context->RSSetState(m_rasterizerStates[RasterizerTwoSided]);
    plasma::Matrix<float> world;
    world.identity();
    m_shader.setWorldMatrix(world);

    // A ribbon of fifteen quads between two trails of sixteen points.
    std::vector<TexturedVertex> vertices(32);
    std::vector<int> indices(90);
    for (int i = 0; i < 15; i++) {
        indices[i * 6 + 0] = i * 2;
        indices[i * 6 + 1] = i * 2 + 1;
        indices[i * 6 + 2] = i * 2 + 2;
        indices[i * 6 + 3] = i * 2 + 1;
        indices[i * 6 + 4] = i * 2 + 2;
        indices[i * 6 + 5] = i * 2 + 3;
    }
    float lift = 0.0f;
    for (cube::Creature *creature : m_trailCreatures) {
        // The creature's drawn position relative to the draw origin, in blocks. Each trail point is copied and moved
        // by it in place.
        plasma::Vector<2, float> offset((float)(-creature->m_drawnPosition[0] - m_drawOriginX),
                                        (float)(-creature->m_drawnPosition[1] - m_drawOriginY));
        plasma::Vector<3, unsigned char> outerColor;
        plasma::Vector<3, unsigned char> innerColor;
        innerColor = plasma::Vector<3, unsigned char>(255, 255, 255);
        outerColor = plasma::Vector<3, unsigned char>(255, 255, 255);
        if (!(creature->m_animationState.m_trails[0][0] == creature->m_animationState.m_trails[0][15])) {
            for (int i = 0; i < 16; i++) {
                float t = (1.0f - (float)i / 15.0f - 0.5f) * 2.0f;
                float fade = (1.0f - t * t) * (1.0f - t * t);
                plasma::Vector<3, float> outerPoint = creature->m_animationState.m_trails[1][i];
                outerPoint[0] -= offset[0];
                outerPoint[1] -= offset[1];
                outerPoint[2] += lift;
                plasma::Vector<3, float> innerPoint = creature->m_animationState.m_trails[0][i];
                innerPoint[0] -= offset[0];
                innerPoint[1] -= offset[1];
                innerPoint[2] += lift;
                vertices[2 * i].position = plasma::Vector<4, float>(outerPoint[0], outerPoint[1], outerPoint[2], 1.0f);
                vertices[2 * i].color = plasma::Vector<4, unsigned char>(outerColor[0], outerColor[1], outerColor[2],
                                                                         (int)(fade * TrailOuterMaximumOpacity));
                vertices[2 * i + 1].position =
                    plasma::Vector<4, float>(innerPoint[0], innerPoint[1], innerPoint[2], 1.0f);
                vertices[2 * i + 1].color = plasma::Vector<4, unsigned char>(
                    innerColor[0], innerColor[1], innerColor[2], (int)(fade * TrailInnerMaximumOpacity));
            }
            drawDynamic(vertices, indices);
        }
        if (!(creature->m_animationState.m_trails[2][0] == creature->m_animationState.m_trails[2][15])) {
            for (int i = 0; i < 16; i++) {
                float t = (1.0f - (float)i / 15.0f - 0.5f) * 2.0f;
                float fade = (1.0f - t * t) * (1.0f - t * t);
                plasma::Vector<3, float> outerPoint = creature->m_animationState.m_trails[3][i];
                outerPoint[0] -= offset[0];
                outerPoint[1] -= offset[1];
                outerPoint[2] += lift;
                plasma::Vector<3, float> innerPoint = creature->m_animationState.m_trails[2][i];
                innerPoint[0] -= offset[0];
                innerPoint[1] -= offset[1];
                innerPoint[2] += lift;
                vertices[2 * i].position = plasma::Vector<4, float>(outerPoint[0], outerPoint[1], outerPoint[2], 1.0f);
                vertices[2 * i].color = plasma::Vector<4, unsigned char>(outerColor[0], outerColor[1], outerColor[2],
                                                                         (int)(fade * TrailOuterMaximumOpacity));
                vertices[2 * i + 1].position =
                    plasma::Vector<4, float>(innerPoint[0], innerPoint[1], innerPoint[2], 1.0f);
                vertices[2 * i + 1].color = plasma::Vector<4, unsigned char>(
                    innerColor[0], innerColor[1], innerColor[2], (int)(fade * TrailInnerMaximumOpacity));
            }
            drawDynamic(vertices, indices);
        }
    }
}

void D3D11Renderer::setAlpha(float value)
{
    m_shader.setAlpha(value);
}

void D3D11Renderer::setAmbientColor(const plasma::Vector<4, float> &color)
{
    m_shader.setAmbientColor(color);
}

void D3D11Renderer::setDepthOffset(float value)
{
    m_shader.setDepthOffset(value);
}

void D3D11Renderer::setFogGradientTranslation(float value)
{
    m_shader.setFogGradientTranslation(value);
}

void D3D11Renderer::setFogScale(float value)
{
    m_shader.setFogScale(value);
}

void D3D11Renderer::setGlow(float value)
{
    m_shader.setGlow(value);
}

void D3D11Renderer::setHighlight(const plasma::Vector<4, float> &value)
{
    m_shader.setHighlight(value);
}

void D3D11Renderer::setLight(const plasma::Vector<4, float> &frontColor, const plasma::Vector<4, float> &backColor,
                             const plasma::Vector<3, float> &direction, const plasma::Vector<4, float> &ambientColor)
{
    m_shader.setLight(frontColor, backColor, direction, ambientColor);
}

void D3D11Renderer::setMaterialColor(const plasma::Vector<4, float> &value)
{
    m_shader.setMaterialColor(value);
}

void D3D11Renderer::setMatrices(const plasma::Matrix<float> &world, const plasma::Matrix<float> &view,
                                const plasma::Matrix<float> &projection)
{
    m_shader.setMatrices(world, view, projection);
}

void D3D11Renderer::setPointLights(const std::array<plasma::Vector<4, float>, 16> &positions,
                                   const std::array<plasma::Vector<4, float>, 16> &colors)
{
    m_shader.setPointLights(positions, colors);
}

void D3D11Renderer::setProjectionMatrix(const plasma::Matrix<float> &matrix)
{
    m_shader.setProjectionMatrix(matrix);
}

void D3D11Renderer::setShininess(float value)
{
    m_shader.setShininess(value);
}

void D3D11Renderer::setSky(const plasma::Vector<4, float> &skyColor1, const plasma::Vector<4, float> &skyColor2,
                           const plasma::Vector<4, float> &fogColor, const plasma::Vector<4, float> &sunColor)
{
    m_shader.setSky(skyColor1, skyColor2, fogColor, sunColor);
}

void D3D11Renderer::setSunScreenPosition(const plasma::Vector<3, float> &direction, const plasma::Matrix<float> &view,
                                         const plasma::Matrix<float> &projection, const float &cameraPitch, int width,
                                         int height)
{
    m_shader.setSunScreenPosition(direction, view, projection, cameraPitch, width, height);
}

void D3D11Renderer::setViewMatrix(const plasma::Matrix<float> &matrix)
{
    m_shader.setViewMatrix(matrix);
}

void D3D11Renderer::setWhite(float value)
{
    m_shader.setWhite(value);
}

void D3D11Renderer::setWorldMatrix(const plasma::Matrix<float> &matrix)
{
    m_shader.setWorldMatrix(matrix);
}

}
