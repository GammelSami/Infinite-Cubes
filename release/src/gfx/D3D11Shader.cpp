#include <d3d11.h>
#include <math.h>
#include <algorithm>
#include "gfx/D3D11Shader.h"
#include "gfx/shaders/WorldVertexShader.h"
#include "gfx/shaders/WorldPixelShader.h"
#include "gfx/shaders/SkyPixelShader.h"
#include "gfx/shaders/WaterPixelShader.h"
#include "gfx/shaders/UnusedPixelShader.h"
#include "gfx/shaders/CloudPixelShader.h"
#include "gfx/shaders/TexturedVertexShader.h"
#include "gfx/shaders/TexturedPixelShader.h"

namespace gfx {

static const float InvalidFogGradientTranslation = -10000.0f;
static const float HorizonReferencePitch = 80.0f;
static const float FogHorizonPitchScale = -30.0f;
static const float SunProjectionDistance = 100.0f;
static const float SunHorizonPitchScale = 30.0f;

D3D11Shader::D3D11Shader(ID3D11Device *device, ID3D11DeviceContext *context)
    : m_vertexShader(nullptr),
      m_pixelShader(nullptr),
      m_skyPixelShader(nullptr),
      m_cloudPixelShader(nullptr),
      m_waterPixelShader(nullptr),
      m_unusedPixelShader(nullptr),
      m_texturedVertexShader(nullptr),
      m_texturedPixelShader(nullptr),
      m_device(device),
      m_context(context),
      m_inputLayout(nullptr),
      m_texturedInputLayout(nullptr),
      m_projectionBuffer(nullptr),
      m_viewBuffer(nullptr),
      m_worldBuffer(nullptr),
      m_materialBuffer(nullptr),
      m_pointLightsBuffer(nullptr),
      m_lightBuffer(nullptr),
      m_ambientBuffer(nullptr),
      m_whiteBuffer(nullptr),
      m_depthOffsetBuffer(nullptr),
      m_glowBuffer(nullptr),
      m_skyBuffer(nullptr),
      m_fogDistanceBuffer(nullptr),
      m_fogAngleBuffer(nullptr),
      m_sunScreenPositionBuffer(nullptr),
      m_alphaBuffer(nullptr),
      m_effectTextureBuffer(nullptr)
{
    // m_shininessBuffer is not zeroed here; initialize() sets it.
    initialize();
}

D3D11Shader::~D3D11Shader()
{
    // Releases m_vertexShader twice and never releases m_texturedVertexShader.
    if (m_vertexShader)
        m_vertexShader->Release();
    if (m_pixelShader)
        m_pixelShader->Release();
    if (m_waterPixelShader)
        m_waterPixelShader->Release();
    if (m_skyPixelShader)
        m_skyPixelShader->Release();
    if (m_cloudPixelShader)
        m_cloudPixelShader->Release();
    if (m_unusedPixelShader)
        m_unusedPixelShader->Release();
    if (m_vertexShader)
        m_vertexShader->Release();
    if (m_texturedPixelShader)
        m_texturedPixelShader->Release();
    if (m_inputLayout)
        m_inputLayout->Release();
    if (m_texturedInputLayout)
        m_texturedInputLayout->Release();
    if (m_projectionBuffer)
        m_projectionBuffer->Release();
    if (m_viewBuffer)
        m_viewBuffer->Release();
    if (m_worldBuffer)
        m_worldBuffer->Release();
    if (m_materialBuffer)
        m_materialBuffer->Release();
    if (m_pointLightsBuffer)
        m_pointLightsBuffer->Release();
    if (m_lightBuffer)
        m_lightBuffer->Release();
    if (m_ambientBuffer)
        m_ambientBuffer->Release();
    if (m_whiteBuffer)
        m_whiteBuffer->Release();
    if (m_depthOffsetBuffer)
        m_depthOffsetBuffer->Release();
    if (m_glowBuffer)
        m_glowBuffer->Release();
    if (m_skyBuffer)
        m_skyBuffer->Release();
    if (m_fogDistanceBuffer)
        m_fogDistanceBuffer->Release();
    if (m_fogAngleBuffer)
        m_fogAngleBuffer->Release();
    if (m_sunScreenPositionBuffer)
        m_sunScreenPositionBuffer->Release();
    if (m_alphaBuffer)
        m_alphaBuffer->Release();
    if (m_shininessBuffer)
        m_shininessBuffer->Release();
    if (m_effectTextureBuffer)
        m_effectTextureBuffer->Release();
}

void D3D11Shader::apply()
{
    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_pixelShader, nullptr, 0);
    m_context->IASetInputLayout(m_inputLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    setConstantBuffers();
}

void D3D11Shader::applyCloud()
{
    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_cloudPixelShader, nullptr, 0);
    m_context->IASetInputLayout(m_inputLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    setConstantBuffers();
}

void D3D11Shader::applyQuads()
{
    m_context->VSSetShader(m_texturedVertexShader, nullptr, 0);
    m_context->PSSetShader(m_texturedPixelShader, nullptr, 0);
    m_context->IASetInputLayout(m_texturedInputLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetConstantBuffers(0, 1, &m_projectionBuffer);
    m_context->VSSetConstantBuffers(1, 1, &m_viewBuffer);
    m_context->VSSetConstantBuffers(2, 1, &m_worldBuffer);
    m_context->PSSetConstantBuffers(0, 1, &m_effectTextureBuffer);
}

void D3D11Shader::applySky()
{
    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_skyPixelShader, nullptr, 0);
    m_context->IASetInputLayout(m_inputLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    setConstantBuffers();
}

void D3D11Shader::applyWater()
{
    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_waterPixelShader, nullptr, 0);
    m_context->IASetInputLayout(m_inputLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    setConstantBuffers();
}

void D3D11Shader::enableTexture(bool enabled)
{
    int data[4] = {enabled, 0, 0, 0};
    m_context->UpdateSubresource(m_effectTextureBuffer, 0, nullptr, data, 0, 0);
}

bool D3D11Shader::initialize()
{
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    m_device->CreateInputLayout(layout, 2, g_worldVertexShader, sizeof(g_worldVertexShader), &m_inputLayout);
    D3D11_INPUT_ELEMENT_DESC texturedLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    m_device->CreateInputLayout(texturedLayout, 3, g_texturedVertexShader, sizeof(g_texturedVertexShader),
                                &m_texturedInputLayout);
    m_context->IASetInputLayout(m_inputLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_device->CreateVertexShader(g_worldVertexShader, sizeof(g_worldVertexShader), nullptr, &m_vertexShader);
    m_device->CreatePixelShader(g_worldPixelShader, sizeof(g_worldPixelShader), nullptr, &m_pixelShader);
    m_device->CreatePixelShader(g_skyPixelShader, sizeof(g_skyPixelShader), nullptr, &m_skyPixelShader);
    m_device->CreatePixelShader(g_cloudPixelShader, sizeof(g_cloudPixelShader), nullptr, &m_cloudPixelShader);
    m_device->CreatePixelShader(g_waterPixelShader, sizeof(g_waterPixelShader), nullptr, &m_waterPixelShader);
    m_device->CreatePixelShader(g_unusedPixelShader, sizeof(g_unusedPixelShader), nullptr, &m_unusedPixelShader);
    m_device->CreateVertexShader(g_texturedVertexShader, sizeof(g_texturedVertexShader), nullptr,
                                 &m_texturedVertexShader);
    m_device->CreatePixelShader(g_texturedPixelShader, sizeof(g_texturedPixelShader), nullptr, &m_texturedPixelShader);

    D3D11_BUFFER_DESC description;
    ZeroMemory(&description, sizeof(description));
    description.Usage = D3D11_USAGE_DEFAULT;
    description.ByteWidth = 64;
    description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    description.CPUAccessFlags = 0;
    description.MiscFlags = 0;
    m_device->CreateBuffer(&description, nullptr, &m_projectionBuffer);
    m_device->CreateBuffer(&description, nullptr, &m_viewBuffer);
    m_device->CreateBuffer(&description, nullptr, &m_worldBuffer);
    description.ByteWidth = 16;
    m_device->CreateBuffer(&description, nullptr, &m_materialBuffer);
    description.ByteWidth = 320;
    m_device->CreateBuffer(&description, nullptr, &m_pointLightsBuffer);
    description.ByteWidth = 48;
    m_device->CreateBuffer(&description, nullptr, &m_lightBuffer);
    description.ByteWidth = 16;
    m_device->CreateBuffer(&description, nullptr, &m_whiteBuffer);
    m_device->CreateBuffer(&description, nullptr, &m_ambientBuffer);
    m_device->CreateBuffer(&description, nullptr, &m_depthOffsetBuffer);
    m_device->CreateBuffer(&description, nullptr, &m_glowBuffer);
    description.ByteWidth = 64;
    m_device->CreateBuffer(&description, nullptr, &m_skyBuffer);
    description.ByteWidth = 16;
    m_device->CreateBuffer(&description, nullptr, &m_fogDistanceBuffer);
    m_device->CreateBuffer(&description, nullptr, &m_fogAngleBuffer);
    m_device->CreateBuffer(&description, nullptr, &m_sunScreenPositionBuffer);
    m_device->CreateBuffer(&description, nullptr, &m_alphaBuffer);
    m_device->CreateBuffer(&description, nullptr, &m_shininessBuffer);
    m_device->CreateBuffer(&description, nullptr, &m_effectTextureBuffer);
    invalidate();
    return true;
}

void D3D11Shader::invalidate()
{
    for (int i = 0; i < 16; i++)
        m_worldMatrix[i] = 0.0f;
    for (int i = 0; i < 16; i++)
        m_viewMatrix[i] = 0.0f;
    for (int i = 0; i < 16; i++)
        m_projectionMatrix[i] = 0.0f;
    m_materialColor = plasma::Vector<4, float>(-1.0f, 0.0f, 0.0f, 0.0f);
    m_lightDirection = plasma::Vector<3, float>(-1.0f, 0.0f, 0.0f);
    m_lightFrontColor = plasma::Vector<4, float>(-1.0f, 0.0f, 0.0f, 0.0f);
    m_lightBackColor = plasma::Vector<4, float>(-1.0f, 0.0f, 0.0f, 0.0f);
    m_ambientColor = plasma::Vector<4, float>(-1.0f, 0.0f, 0.0f, 0.0f);
    m_white = -1.0f;
    for (int i = 0; i < 4; i++)
        m_skyColors[i] = plasma::Vector<4, float>(-1.0f, 0.0f, 0.0f, 0.0f);
    m_sunScreenPosition = plasma::Vector<4, float>(-1.0f, 0.0f, 0.0f, 0.0f);
    m_shininess = -1.0f;
    m_alpha = -1.0f;
    m_glow = -1.0f;
    m_fogScale = -1.0f;
    m_fogGradientTranslation = InvalidFogGradientTranslation;
    m_depthOffset = -1.0f;
    m_unusedCache3e0 = plasma::Vector<4, float>(-1.0f, 0.0f, 0.0f, 0.0f);
    m_pointLightColors.fill(plasma::Vector<4, float>(-1.0f, 0.0f, 0.0f, 0.0f));
    m_pointLightPositions.fill(plasma::Vector<4, float>(-1.0f, 0.0f, 0.0f, 0.0f));
}

void D3D11Shader::setAlpha(float value)
{
    if (value != m_alpha) {
        m_alpha = value;
        float data[4] = {value, 0.0f, 0.0f, 0.0f};
        m_context->UpdateSubresource(m_alphaBuffer, 0, nullptr, data, 0, 0);
    }
}

void D3D11Shader::setAmbientColor(const plasma::Vector<4, float> &color)
{
    if (color != m_ambientColor) {
        m_ambientColor = color;
        m_context->UpdateSubresource(m_ambientBuffer, 0, nullptr, &color, 0, 0);
    }
}

void D3D11Shader::setConstantBuffers()
{
    m_context->VSSetConstantBuffers(0, 1, &m_projectionBuffer);
    m_context->VSSetConstantBuffers(1, 1, &m_viewBuffer);
    m_context->VSSetConstantBuffers(2, 1, &m_worldBuffer);
    m_context->VSSetConstantBuffers(3, 1, &m_materialBuffer);
    m_context->VSSetConstantBuffers(4, 1, &m_pointLightsBuffer);
    m_context->VSSetConstantBuffers(5, 1, &m_lightBuffer);
    m_context->VSSetConstantBuffers(6, 1, &m_ambientBuffer);
    m_context->VSSetConstantBuffers(7, 1, &m_whiteBuffer);
    m_context->VSSetConstantBuffers(8, 1, &m_depthOffsetBuffer);
    m_context->VSSetConstantBuffers(9, 1, &m_glowBuffer);
    m_context->PSSetConstantBuffers(0, 1, &m_skyBuffer);
    m_context->PSSetConstantBuffers(1, 1, &m_fogDistanceBuffer);
    m_context->PSSetConstantBuffers(2, 1, &m_fogAngleBuffer);
    m_context->PSSetConstantBuffers(3, 1, &m_sunScreenPositionBuffer);
    m_context->PSSetConstantBuffers(4, 1, &m_alphaBuffer);
    m_context->PSSetConstantBuffers(5, 1, &m_shininessBuffer);
}

void D3D11Shader::setDepthOffset(float value)
{
    if (value != m_depthOffset) {
        m_depthOffset = value;
        float data[4] = {value, 0.0f, 0.0f, 0.0f};
        m_context->UpdateSubresource(m_depthOffsetBuffer, 0, nullptr, data, 0, 0);
    }
}

void D3D11Shader::setFogGradientTranslation(float value)
{
    value = (value - HorizonReferencePitch) / FogHorizonPitchScale;
    if (value != m_fogGradientTranslation) {
        m_fogGradientTranslation = value;
        float data[4] = {value, 0.0f, 0.0f, 0.0f};
        m_context->UpdateSubresource(m_fogAngleBuffer, 0, nullptr, data, 0, 0);
    }
}

void D3D11Shader::setFogScale(float value)
{
    if (value > 0.0f && 1.0f / value != m_fogScale) {
        m_fogScale = 1.0f / value;
        float data[4] = {1.0f / value, 0.0f, 0.0f, 0.0f};
        m_context->UpdateSubresource(m_fogDistanceBuffer, 0, nullptr, data, 0, 0);
    }
}

void D3D11Shader::setGlow(float value)
{
    if (value != m_glow) {
        m_glow = value;
        float data[4] = {value, 0.0f, 0.0f, 0.0f};
        m_context->UpdateSubresource(m_glowBuffer, 0, nullptr, data, 0, 0);
    }
}

void D3D11Shader::setHighlight(const plasma::Vector<4, float> &value) {}

void D3D11Shader::setLight(const plasma::Vector<4, float> &frontColor, const plasma::Vector<4, float> &backColor,
                           const plasma::Vector<3, float> &direction, const plasma::Vector<4, float> &ambientColor)
{
    plasma::Vector<3, float> normal =
        direction *
        (1.0f / sqrtf(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]));
    if (frontColor != m_lightFrontColor || backColor != m_lightBackColor || normal != m_lightDirection) {
        m_lightFrontColor = frontColor;
        m_lightBackColor = backColor;
        m_lightDirection = normal;
        struct {
            plasma::Vector<4, float> frontColor;
            plasma::Vector<4, float> backColor;
            plasma::Vector<4, float> direction;
        } data;
        data.frontColor = frontColor;
        data.backColor = backColor;
        data.direction = plasma::Vector<4, float>(normal[0], normal[1], normal[2], 1.0f);
        m_context->UpdateSubresource(m_lightBuffer, 0, nullptr, &data, 0, 0);
    }
    setAmbientColor(ambientColor);
}

void D3D11Shader::setMaterialColor(const plasma::Vector<4, float> &value)
{
    if (value != m_materialColor) {
        m_materialColor = value;
        m_context->UpdateSubresource(m_materialBuffer, 0, nullptr, &value, 0, 0);
    }
}

void D3D11Shader::setMatrices(const plasma::Matrix<float> &world, const plasma::Matrix<float> &view,
                              const plasma::Matrix<float> &projection)
{
    setWorldMatrix(world);
    setViewMatrix(view);
    setProjectionMatrix(projection);
}

void D3D11Shader::setPointLights(const std::array<plasma::Vector<4, float>, 16> &positions,
                                 const std::array<plasma::Vector<4, float>, 16> &colors)
{
    if (m_pointLightPositions != positions || m_pointLightColors != colors) {
        m_pointLightPositions = positions;
        m_pointLightColors = colors;
        struct {
            plasma::Vector<4, float> positions[10];
            plasma::Vector<4, float> colors[10];
        } data;
        for (int i = 0; i < 10; i++) {
            data.positions[i] = positions[i];
            data.colors[i] = colors[i];
        }
        m_context->UpdateSubresource(m_pointLightsBuffer, 0, nullptr, &data, 0, 0);
    }
}

void D3D11Shader::setProjectionMatrix(const plasma::Matrix<float> &matrix)
{
    if (&matrix != &m_projectionMatrix) {
        m_projectionMatrix = matrix;
        plasma::Matrix<float> transposed = matrix;
        transposed.transpose();
        m_context->UpdateSubresource(m_projectionBuffer, 0, nullptr, &transposed, 0, 0);
    }
}

void D3D11Shader::setShininess(float value)
{
    if (value != m_shininess) {
        m_shininess = value;
        float data[4] = {value, 0.0f, 0.0f, 0.0f};
        m_context->UpdateSubresource(m_shininessBuffer, 0, nullptr, data, 0, 0);
    }
}

void D3D11Shader::setSky(const plasma::Vector<4, float> &skyColor1, const plasma::Vector<4, float> &skyColor2,
                         const plasma::Vector<4, float> &fogColor, const plasma::Vector<4, float> &sunColor)
{
    if (skyColor1 != m_skyColors[0] || skyColor2 != m_skyColors[1] || fogColor != m_skyColors[2] ||
        sunColor != m_skyColors[3]) {
        m_skyColors[0] = skyColor1;
        m_skyColors[1] = skyColor2;
        m_skyColors[2] = fogColor;
        m_skyColors[3] = sunColor;
        plasma::Vector<4, float> data[4] = {skyColor1, skyColor2, fogColor, sunColor};
        m_context->UpdateSubresource(m_skyBuffer, 0, nullptr, data, 0, 0);
    }
}

void D3D11Shader::setSunScreenPosition(const plasma::Vector<3, float> &direction, const plasma::Matrix<float> &view,
                                       const plasma::Matrix<float> &projection, const float &cameraPitch, int width,
                                       int height)
{
    plasma::Vector<3, float> position = direction * SunProjectionDistance;
    plasma::Vector<3, float> viewPosition(position[0] * view[0] + position[1] * view[4] + position[2] * view[8],
                                          position[0] * view[1] + position[1] * view[5] + position[2] * view[9],
                                          position[0] * view[2] + position[1] * view[6] + position[2] * view[10]);
    plasma::Vector<3, float> projected = projection * viewPosition;
    float visible = 1.0f;
    if (projected[2] > 1.0f || projected[2] < 0.0f)
        visible = 0.0f;
    plasma::Vector<4, float> value(projected[0],
                                   projected[1] - (cameraPitch - HorizonReferencePitch) / SunHorizonPitchScale,
                                   (float)width / (float)height, visible);
    if (value != m_sunScreenPosition) {
        m_sunScreenPosition = value;
        m_context->UpdateSubresource(m_sunScreenPositionBuffer, 0, nullptr, &value, 0, 0);
    }
}

void D3D11Shader::setViewMatrix(const plasma::Matrix<float> &matrix)
{
    if (&matrix != &m_viewMatrix) {
        m_viewMatrix = matrix;
        plasma::Matrix<float> transposed = matrix;
        transposed.transpose();
        m_context->UpdateSubresource(m_viewBuffer, 0, nullptr, &transposed, 0, 0);
    }
}

void D3D11Shader::setWhite(float value)
{
    if (value != m_white) {
        m_white = value;
        float data[4] = {value, 0.0f, 0.0f, 0.0f};
        m_context->UpdateSubresource(m_whiteBuffer, 0, nullptr, data, 0, 0);
    }
}

void D3D11Shader::setWorldMatrix(const plasma::Matrix<float> &matrix)
{
    if (&matrix != &m_worldMatrix) {
        m_worldMatrix = matrix;
        plasma::Matrix<float> transposed = matrix;
        transposed.transpose();
        m_context->UpdateSubresource(m_worldBuffer, 0, nullptr, &transposed, 0, 0);
    }
}

}
