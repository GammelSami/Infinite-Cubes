#include <d3d11.h>
#include "plasma/D3D11Engine.h"
#include "plasma/D3D11Drawing.h"
#include "plasma/D3D11RenderSurface.h"
#include "plasma/D3D11Texture.h"
#include "plasma/Node.h"
#include "plasma/Display.h"
#include "plasma/Transformation.h"
#include "plasma/EditFilter.h"

namespace plasma {

static const float UIOrthographicFarClip = 10000000.0f;

// Compiled bytecode for the interface HLSL shaders.
#include "plasma/shaders/D3D11EngineVertexShader.h"

#include "plasma/shaders/D3D11EnginePixelShader.h"

D3D11Engine::D3D11Engine(ID3D11Device *device, ID3D11DeviceContext *context)
{
    m_device = device;
    m_context = context;
    m_vertexBuffer = nullptr;
    m_indexBuffer = nullptr;
    m_vertexShader = nullptr;
    m_pixelShader = nullptr;
    m_inputLayout = nullptr;
    m_rasterizerState = nullptr;
    m_blendState = nullptr;
    m_depthStencilState = nullptr;
    for (int i = 0; i < 16; i++)
        m_shaderModelView.m_elements[i] = 0.0f;
    for (int i = 0; i < 16; i++)
        m_shaderProjection.m_elements[i] = 0.0f;
    for (int i = 0; i < 16; i++)
        m_shaderPixelScaledModelView.m_elements[i] = 0.0f;
    for (int i = 0; i < 16; i++)
        m_shaderTextureMatrix.m_elements[i] = 0.0f;
    m_widgetDeformationEnabled = true;
    for (int i = 0; i < 16; i++)
        m_widgetBindMatrix.m_elements[i] = 0.0f;
    for (int i = 0; i < 16; i++)
        m_inverseWidgetBindMatrix.m_elements[i] = 0.0f;
    m_widgetBindPos = Vector<2, float>(-100000000.0f, -10000000.0f);
    m_widgetBindSize = Vector<2, float>(-100000000.0f, -10000000.0f);
    m_deformedWidgetPos = Vector<2, float>(-100000000.0f, -10000000.0f);
    m_deformedWidgetSize = Vector<2, float>(-100000000.0f, -10000000.0f);
    m_color = Vector<4, float>(-1.0f, -1.0f, -1.0f, -1.0f);
    m_textureEnabled = true;
    m_filter = -1;
    m_textureAdjustments = Vector<4, float>(-1.0f, -1.0f, -1.0f, -1.0f);
}

D3D11Engine::~D3D11Engine()
{
    if (m_vertexShader)
        m_vertexShader->Release();
    if (m_pixelShader)
        m_pixelShader->Release();
    if (m_inputLayout)
        m_inputLayout->Release();
    if (m_rasterizerState)
        m_rasterizerState->Release();
    if (m_blendState)
        m_blendState->Release();
    if (m_depthStencilState)
        m_depthStencilState->Release();
    if (m_projectionBuffer)
        m_projectionBuffer->Release();
    if (m_modelViewBuffer)
        m_modelViewBuffer->Release();
    if (m_pixelScaledModelViewBuffer)
        m_pixelScaledModelViewBuffer->Release();
    if (m_textureMatrixBuffer)
        m_textureMatrixBuffer->Release();
    if (m_widgetDeformationEnabledBuffer)
        m_widgetDeformationEnabledBuffer->Release();
    if (m_widgetBindMatrixBuffer)
        m_widgetBindMatrixBuffer->Release();
    if (m_inverseWidgetBindMatrixBuffer)
        m_inverseWidgetBindMatrixBuffer->Release();
    if (m_widgetBindPosBuffer)
        m_widgetBindPosBuffer->Release();
    if (m_widgetBindSizeBuffer)
        m_widgetBindSizeBuffer->Release();
    if (m_deformedWidgetPosBuffer)
        m_deformedWidgetPosBuffer->Release();
    if (m_deformedWidgetSizeBuffer)
        m_deformedWidgetSizeBuffer->Release();
    if (m_colorBuffer)
        m_colorBuffer->Release();
    if (m_textureEnabledBuffer)
        m_textureEnabledBuffer->Release();
    if (m_filterBuffer)
        m_filterBuffer->Release();
    if (m_textureAdjustmentsBuffer)
        m_textureAdjustmentsBuffer->Release();
    if (m_vertexBuffer)
        m_vertexBuffer->Release();
    if (m_indexBuffer)
        m_indexBuffer->Release();
}

void D3D11Engine::applyRenderState()
{
    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_pixelShader, nullptr, 0);
    m_context->IASetInputLayout(m_inputLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetConstantBuffers(VS_BUFFER_PROJ, 1, &m_projectionBuffer);
    m_context->VSSetConstantBuffers(VS_BUFFER_WORLD, 1, &m_modelViewBuffer);
    m_context->VSSetConstantBuffers(VS_BUFFER_MASK, 1, &m_pixelScaledModelViewBuffer);
    m_context->VSSetConstantBuffers(VS_BUFFER_TEXTURE, 1, &m_textureMatrixBuffer);
    m_context->VSSetConstantBuffers(VS_BUFFER_WIDGET_DEFORMATION_ENABLED, 1, &m_widgetDeformationEnabledBuffer);
    m_context->VSSetConstantBuffers(VS_BUFFER_WIDGET_BIND_MATRIX, 1, &m_widgetBindMatrixBuffer);
    m_context->VSSetConstantBuffers(VS_BUFFER_INVERSE_WIDGET_BIND_MATRIX, 1, &m_inverseWidgetBindMatrixBuffer);
    m_context->VSSetConstantBuffers(VS_BUFFER_WIDGET_BIND_POS, 1, &m_widgetBindPosBuffer);
    m_context->VSSetConstantBuffers(VS_BUFFER_WIDGET_BIND_SIZE, 1, &m_widgetBindSizeBuffer);
    m_context->VSSetConstantBuffers(VS_BUFFER_DEFORMED_WIDGET_POS, 1, &m_deformedWidgetPosBuffer);
    m_context->VSSetConstantBuffers(VS_BUFFER_DEFORMED_WIDGET_SIZE, 1, &m_deformedWidgetSizeBuffer);
    m_context->VSSetConstantBuffers(VS_BUFFER_BASE_COLOR, 1, &m_colorBuffer);
    m_context->PSSetConstantBuffers(PS_BUFFER_TEXTURE_ENABLED, 1, &m_textureEnabledBuffer);
    m_context->PSSetConstantBuffers(PS_BUFFER_FILTER, 1, &m_filterBuffer);
    m_context->PSSetConstantBuffers(PS_BUFFER_TEXTURE_PARAMETERS, 1, &m_textureAdjustmentsBuffer);
    m_context->RSSetState(m_rasterizerState);
    m_context->OMSetBlendState(m_blendState, 0, 0xffffffff);
    m_context->OMSetDepthStencilState(m_depthStencilState, 0);
}

void D3D11Engine::beginWindowDrawing()
{
    m_modelView.identity();
    m_projection.setOrthographic(0.0f, (float)m_viewportWidth, (float)m_viewportHeight, 0.0f, -1.0f,
                                 UIOrthographicFarClip);
}

void D3D11Engine::pushNodeMatrix(Node *node)
{
    m_modelViewStack.push_back(m_modelView);
    m_modelView *= node->m_transformation->m_matrix;
}

void D3D11Engine::beginTextDrawing(Node *node)
{
    applyRenderState();
    if (node && m_filterSurface &&
        (!(node->m_display->m_flags & (1 << DISPLAY_BLUR)) ||
         node->m_display->m_clipping.value() != DISPLAY_CLIPPING_DISABLED))
        m_filterSurface->bindTexture();
    setTextureEnabled(true);
    // The pixel shader's filter, chosen as in D3D11Drawing::draw.
    int filter = FILTER_NONE;
    if (node && node->m_display->m_clipping.value() != DISPLAY_CLIPPING_DISABLED) {
        if (node->m_display->m_flags & (1 << DISPLAY_MODULATION))
            filter = FILTER_MODULATION;
        else if (node->m_display->m_flags & (1 << DISPLAY_SCREEN))
            filter = FILTER_SCREEN;
        else
            filter = FILTER_CLIP;
    }
    setFilter(filter);
    setTextureAdjustments(1.0f, 1.0f, 1.0f, 1.0f);
    setWidgetDeformationEnabled(false);
    setProjectionMatrix(m_projection);
}

void D3D11Engine::beginSurface(RenderSurface *surface, const Vector<2, float> &position, const Vector<2, float> &size,
                               float scale, bool alignPixels)
{
    pushSurface(surface);
    surface->begin();
    if (alignPixels) {
        Vector<2, float> fraction(m_modelView.m_elements[12], m_modelView.m_elements[13]);
        fraction = fraction * scale;
        fraction = fraction - Vector<2, float>(Vector<2, int>(fraction));
        if (scale == 1.0f)
            fraction = Vector<2, float>(0.0f, 0.0f);
        m_projectionStack.push_back(m_projection);
        Vector<2, float> minimum = position - size * ((1.0f - scale) * 0.5f);
        m_projection.setOrthographic(minimum[0], size[0] + minimum[0], size[1] + minimum[1], minimum[1], -1.0f,
                                     UIOrthographicFarClip);
        m_projection.translate(-fraction[0], -fraction[1], 0.0f);
        m_projection.scale(scale, scale, 1.0f);
    } else {
        m_modelView.m_elements[12] = (float)(int)(m_modelView.m_elements[12] + 0.5f);
        m_modelView.m_elements[13] = (float)(int)(m_modelView.m_elements[13] + 0.5f);
        m_projectionStack.push_back(m_projection);
        m_projection.setOrthographic(position[0], position[0] + size[0], position[1] + size[1], position[1], -1.0f,
                                     UIOrthographicFarClip);
        m_projection.scale(scale, scale, 1.0f);
    }
}

Drawing *D3D11Engine::createDrawing()
{
    return new D3D11Drawing(this);
}

RenderSurface *D3D11Engine::createRenderSurface(bool depthAttachment)
{
    return new D3D11RenderSurface(this, depthAttachment);
}

Texture *D3D11Engine::createTexture(int width, int height, const unsigned char *pixels, const TextureFormat &format,
                                    const std::wstring &name, bool transient)
{
    return new D3D11Texture(this, width, height, pixels, format, name, transient);
}

void D3D11Engine::drawCaret(const Vector<2, float> &position, const Vector<2, float> &size,
                            const Vector<4, float> &color)
{
    if (m_vertexBuffer && m_indexBuffer && (m_caretClock / 500 & 1)) {
        Matrix<float> matrix = m_modelView;
        matrix.translate(position);
        matrix.scale(size[0], size[1]);
        setModelViewMatrix(matrix);
        setTextureEnabled(false);
        setColor(color);
        UINT stride = 32;
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
        m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
        m_context->DrawIndexed(6, 0, 0);
    }
}

void D3D11Engine::drawTexturedRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                                        const Vector<2, float> &texturePosition, const Vector<2, float> &textureSize,
                                        const Vector<4, float> &color)
{
    if (m_vertexBuffer && m_indexBuffer) {
        Matrix<float> matrix = m_modelView;
        matrix.translate(position);
        matrix.scale(size[0], size[1]);
        setModelViewMatrix(matrix);
        Matrix<float> textureMatrix;
        textureMatrix.identity();
        textureMatrix.translate(texturePosition);
        textureMatrix.scale(textureSize[0], textureSize[1]);
        setTextureMatrix(textureMatrix);
        setColor(color);
        UINT stride = 32;
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
        m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
        m_context->DrawIndexed(6, 0, 0);
    }
}

void D3D11Engine::fillRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                                const Vector<4, float> &color)
{
    if (m_vertexBuffer && m_indexBuffer) {
        Matrix<float> matrix = m_modelView;
        matrix.translate(position);
        matrix.scale(size[0], size[1]);
        setModelViewMatrix(matrix);
        setTextureEnabled(false);
        setColor(color);
        UINT stride = 32;
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
        m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
        m_context->DrawIndexed(6, 0, 0);
    }
}

void D3D11Engine::popNodeMatrix(Node *node)
{
    if (!m_modelViewStack.empty()) {
        m_modelView = m_modelViewStack.back();
        m_modelViewStack.pop_back();
    }
}

void D3D11Engine::endSurface()
{
    m_surfaceStack.back()->end();
    popSurface();
    if (!m_projectionStack.empty()) {
        m_projection = m_projectionStack.back();
        m_projectionStack.pop_back();
    }
}

void D3D11Engine::getModelViewMatrix(Matrix<float> &matrix)
{
    matrix = m_modelView;
}

bool D3D11Engine::setupShadersAndFrameBuffer()
{
    D3D11_INPUT_ELEMENT_DESC layout[2] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};
    m_device->CreateInputLayout(layout, 2, g_vertexShaderCode, sizeof(g_vertexShaderCode), &m_inputLayout);
    m_device->CreateVertexShader(g_vertexShaderCode, sizeof(g_vertexShaderCode), nullptr, &m_vertexShader);
    m_device->CreatePixelShader(g_pixelShaderCode, sizeof(g_pixelShaderCode), nullptr, &m_pixelShader);

    D3D11_BUFFER_DESC constants;
    ZeroMemory(&constants, sizeof(constants));
    constants.ByteWidth = 64;
    constants.Usage = D3D11_USAGE_DEFAULT;
    constants.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constants.CPUAccessFlags = 0;
    constants.MiscFlags = 0;
    m_device->CreateBuffer(&constants, nullptr, &m_projectionBuffer);
    m_device->CreateBuffer(&constants, nullptr, &m_modelViewBuffer);
    m_device->CreateBuffer(&constants, nullptr, &m_pixelScaledModelViewBuffer);
    m_device->CreateBuffer(&constants, nullptr, &m_textureMatrixBuffer);
    m_device->CreateBuffer(&constants, nullptr, &m_widgetBindMatrixBuffer);
    m_device->CreateBuffer(&constants, nullptr, &m_inverseWidgetBindMatrixBuffer);
    constants.ByteWidth = 16;
    m_device->CreateBuffer(&constants, nullptr, &m_widgetDeformationEnabledBuffer);
    m_device->CreateBuffer(&constants, nullptr, &m_widgetBindPosBuffer);
    m_device->CreateBuffer(&constants, nullptr, &m_widgetBindSizeBuffer);
    m_device->CreateBuffer(&constants, nullptr, &m_deformedWidgetPosBuffer);
    m_device->CreateBuffer(&constants, nullptr, &m_deformedWidgetSizeBuffer);
    m_device->CreateBuffer(&constants, nullptr, &m_colorBuffer);
    constants.ByteWidth = 16;
    m_device->CreateBuffer(&constants, nullptr, &m_textureEnabledBuffer);
    m_device->CreateBuffer(&constants, nullptr, &m_filterBuffer);
    m_device->CreateBuffer(&constants, nullptr, &m_textureAdjustmentsBuffer);

    D3D11_RASTERIZER_DESC rasterizer;
    rasterizer.FillMode = D3D11_FILL_SOLID;
    rasterizer.CullMode = D3D11_CULL_NONE;
    rasterizer.FrontCounterClockwise = FALSE;
    rasterizer.DepthBias = 0;
    rasterizer.DepthBiasClamp = 0.0f;
    rasterizer.SlopeScaledDepthBias = 0.0f;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.ScissorEnable = FALSE;
    rasterizer.MultisampleEnable = TRUE;
    rasterizer.AntialiasedLineEnable = FALSE;
    m_device->CreateRasterizerState(&rasterizer, &m_rasterizerState);

    D3D11_BLEND_DESC blend;
    ZeroMemory(&blend, sizeof(blend));
    blend.RenderTarget[0].BlendEnable = TRUE;
    blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_device->CreateBlendState(&blend, &m_blendState);

    D3D11_DEPTH_STENCIL_DESC depthStencil;
    depthStencil.DepthEnable = FALSE;
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
    m_device->CreateDepthStencilState(&depthStencil, &m_depthStencilState);

    // The unit quad: position (x, y, u, v) and colour per vertex.
    D3D11_BUFFER_DESC vertexBuffer;
    ZeroMemory(&vertexBuffer, sizeof(vertexBuffer));
    vertexBuffer.ByteWidth = 128;
    vertexBuffer.Usage = D3D11_USAGE_DEFAULT;
    vertexBuffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBuffer.CPUAccessFlags = 0;
    vertexBuffer.MiscFlags = 0;
    Vector<4, float> vertices[8] = {Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f), Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f),
                                    Vector<4, float>(1.0f, 0.0f, 1.0f, 0.0f), Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f),
                                    Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f),
                                    Vector<4, float>(0.0f, 1.0f, 0.0f, 1.0f), Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)};
    D3D11_SUBRESOURCE_DATA vertexData;
    ZeroMemory(&vertexData, sizeof(vertexData));
    vertexData.pSysMem = vertices;
    m_device->CreateBuffer(&vertexBuffer, &vertexData, &m_vertexBuffer);

    D3D11_BUFFER_DESC indexBuffer;
    ZeroMemory(&indexBuffer, sizeof(indexBuffer));
    indexBuffer.ByteWidth = 24;
    indexBuffer.Usage = D3D11_USAGE_DEFAULT;
    indexBuffer.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBuffer.CPUAccessFlags = 0;
    indexBuffer.MiscFlags = 0;
    int indices[6] = {0, 1, 2, 0, 3, 2};
    D3D11_SUBRESOURCE_DATA indexData;
    ZeroMemory(&indexData, sizeof(indexData));
    indexData.pSysMem = indices;
    m_device->CreateBuffer(&indexBuffer, &indexData, &m_indexBuffer);
    return true;
}

void D3D11Engine::loadMatrix(Matrix<float> &matrix)
{
    m_modelView = matrix;
}

void D3D11Engine::setColor(const Vector<4, float> &color)
{
    if (color != m_color) {
        m_color = color;
        m_context->UpdateSubresource(m_colorBuffer, 0, nullptr, &color, 0, 0);
    }
}

void D3D11Engine::setDeformedWidgetPos(const Vector<2, float> &position)
{
    if (position != m_deformedWidgetPos) {
        m_deformedWidgetPos = position;
        Vector<4, float> value(position[0], position[1], 0.0f, 1.0f);
        m_context->UpdateSubresource(m_deformedWidgetPosBuffer, 0, nullptr, &value, 0, 0);
    }
}

void D3D11Engine::setDeformedWidgetSize(const Vector<2, float> &size)
{
    if (size != m_deformedWidgetSize) {
        m_deformedWidgetSize = size;
        Vector<4, float> value(size[0], size[1], 0.0f, 1.0f);
        m_context->UpdateSubresource(m_deformedWidgetSizeBuffer, 0, nullptr, &value, 0, 0);
    }
}

void D3D11Engine::setFilter(int filter)
{
    if (filter != m_filter) {
        m_filter = filter;
        int value[4] = {filter, 0, 0, 0};
        m_context->UpdateSubresource(m_filterBuffer, 0, nullptr, value, 0, 0);
    }
}

void D3D11Engine::setInverseWidgetBindMatrix(const Matrix<float> &matrix)
{
    if (&matrix != &m_inverseWidgetBindMatrix) {
        m_inverseWidgetBindMatrix = matrix;
        Matrix<float> transposed = matrix;
        transposed.transpose();
        m_context->UpdateSubresource(m_inverseWidgetBindMatrixBuffer, 0, nullptr, &transposed, 0, 0);
    }
}

void D3D11Engine::setPixelScaledModelViewMatrix(const Matrix<float> &matrix)
{
    if (&matrix != &m_shaderPixelScaledModelView) {
        m_shaderPixelScaledModelView = matrix;
        Matrix<float> transposed = matrix;
        transposed.transpose();
        m_context->UpdateSubresource(m_pixelScaledModelViewBuffer, 0, nullptr, &transposed, 0, 0);
    }
}

void D3D11Engine::setProjectionMatrix(const Matrix<float> &matrix)
{
    if (&matrix != &m_shaderProjection) {
        m_shaderProjection = matrix;
        Matrix<float> transposed = matrix;
        transposed.transpose();
        m_context->UpdateSubresource(m_projectionBuffer, 0, nullptr, &transposed, 0, 0);
    }
}

void D3D11Engine::setTextureEnabled(bool enabled)
{
    if (enabled != m_textureEnabled) {
        m_textureEnabled = enabled;
        int value[4] = {enabled, 0, 0, 0};
        m_context->UpdateSubresource(m_textureEnabledBuffer, 0, nullptr, value, 0, 0);
    }
}

void D3D11Engine::setTextureMatrix(const Matrix<float> &matrix)
{
    if (&matrix != &m_shaderTextureMatrix) {
        m_shaderTextureMatrix = matrix;
        Matrix<float> transposed = matrix;
        transposed.transpose();
        m_context->UpdateSubresource(m_textureMatrixBuffer, 0, nullptr, &transposed, 0, 0);
    }
}

void D3D11Engine::setTextureAdjustments(float opacity, float brightness, float contrast, float saturation)
{
    Vector<4, float> value(opacity, brightness, contrast, saturation);
    if (value != m_textureAdjustments) {
        m_textureAdjustments = Vector<4, float>(opacity, brightness, contrast, saturation);
        m_context->UpdateSubresource(m_textureAdjustmentsBuffer, 0, nullptr, &value, 0, 0);
    }
}

void D3D11Engine::setWidgetBindMatrix(const Matrix<float> &matrix)
{
    if (&matrix != &m_widgetBindMatrix) {
        m_widgetBindMatrix = matrix;
        Matrix<float> transposed = matrix;
        transposed.transpose();
        m_context->UpdateSubresource(m_widgetBindMatrixBuffer, 0, nullptr, &transposed, 0, 0);
    }
}

void D3D11Engine::setWidgetBindPos(const Vector<2, float> &position)
{
    if (position != m_widgetBindPos) {
        m_widgetBindPos = position;
        Vector<4, float> value(position[0], position[1], 0.0f, 1.0f);
        m_context->UpdateSubresource(m_widgetBindPosBuffer, 0, nullptr, &value, 0, 0);
    }
}

void D3D11Engine::setWidgetBindSize(const Vector<2, float> &size)
{
    if (size != m_widgetBindSize) {
        m_widgetBindSize = size;
        Vector<4, float> value(size[0], size[1], 0.0f, 1.0f);
        m_context->UpdateSubresource(m_widgetBindSizeBuffer, 0, nullptr, &value, 0, 0);
    }
}

void D3D11Engine::setWidgetDeformationEnabled(bool enabled)
{
    if (enabled != m_widgetDeformationEnabled) {
        m_widgetDeformationEnabled = enabled;
        int value[4] = {enabled, 0, 0, 0};
        m_context->UpdateSubresource(m_widgetDeformationEnabledBuffer, 0, nullptr, value, 0, 0);
    }
}

void D3D11Engine::setModelViewMatrix(const Matrix<float> &matrix)
{
    if (&matrix != &m_shaderModelView) {
        m_shaderModelView = matrix;
        Matrix<float> transposed = matrix;
        transposed.transpose();
        m_context->UpdateSubresource(m_modelViewBuffer, 0, nullptr, &transposed, 0, 0);
    }
}

}
