// Implements off-screen drawing with Direct3D 11 render-target and shader-resource views.
#include <iostream>
#include "plasma/D3D11RenderSurface.h"
#include "plasma/D3D11Engine.h"
#include "plasma/EditFilter.h"

namespace plasma {

D3D11RenderSurface::D3D11RenderSurface(D3D11Engine *engine, bool depthAttachment)
    : RenderSurface(depthAttachment),
      m_engine(engine),
      m_width(0),
      m_height(0),
      m_renderTargetView(nullptr),
      m_shaderResourceView(nullptr),
      m_savedRenderTargetView(nullptr),
      m_savedDepthStencilView(nullptr)
{}

D3D11RenderSurface::~D3D11RenderSurface()
{
    if (m_renderTargetView)
        m_renderTargetView->Release();
    if (m_shaderResourceView)
        m_shaderResourceView->Release();
    if (m_savedRenderTargetView)
        m_savedRenderTargetView->Release();
    if (m_savedDepthStencilView)
        m_savedDepthStencilView->Release();
}

void D3D11RenderSurface::bindTexture()
{
    m_engine->m_context->PSSetShaderResources(0, 1, &m_shaderResourceView);
}

void D3D11RenderSurface::end()
{
    m_engine->m_context->OMSetRenderTargets(1, &m_savedRenderTargetView, m_savedDepthStencilView);
    if (m_savedRenderTargetView)
        m_savedRenderTargetView->Release();
    if (m_savedDepthStencilView)
        m_savedDepthStencilView->Release();
    m_engine->m_context->RSSetViewports(1, &m_savedViewport);
}

void D3D11RenderSurface::draw(const Vector<2, float> &position, const Vector<2, float> &size,
                              const Vector<2, float> &texturePosition, float scale)
{
    Matrix<float> modelView;
    m_engine->getModelViewMatrix(modelView);
    // The fraction of a pixel the translation has after scaling; dropped at the surface's own scale.
    float textureScale = scale;
    Vector<2, float> fraction(modelView.m_elements[12] * scale, modelView.m_elements[13] * scale);
    fraction = fraction - Vector<2, float>(Vector<2, int>(fraction));
    if (scale == 1.0f) {
        fraction[0] = 0.0f;
        fraction[1] = 0.0f;
    }
    Matrix<float> identity;
    identity.identity();
    m_engine->loadMatrix(identity);
    // A negative size component means the surface's size.
    Vector<2, float> drawPosition = position;
    Vector<2, float> drawSize = size;
    if (size.m_elements[0] < 0.0f)
        drawSize.m_elements[0] = m_width;
    if (drawSize.m_elements[1] < 0.0f)
        drawSize.m_elements[1] = m_height;
    float width = m_width;
    float margin = (1.0f - scale) * 0.5f;
    float height = m_height;
    float marginX = margin * width;
    float marginY = margin * height;
    if (scale < 0.0f) {
        fraction[0] = 0.0f;
        textureScale = width / size.m_elements[0];
        fraction[1] = 0.0f;
        marginX = 0.0f;
        marginY = 0.0f;
    }
    Vector<2, float> textureOrigin((textureScale * texturePosition.m_elements[0] + (marginX - fraction[0])) / width,
                                   (textureScale * texturePosition.m_elements[1] + (marginY - fraction[1])) / height);
    Vector<2, float> textureSize(drawSize.m_elements[0] * textureScale / width,
                                 drawSize.m_elements[1] * textureScale / height);
    m_engine->applyRenderState();
    m_engine->setTextureEnabled(true);
    m_engine->setFilter(FILTER_NONE);
    m_engine->setTextureAdjustments(1.0f, 1.0f, 1.0f, 1.0f);
    m_engine->setWidgetDeformationEnabled(false);
    m_engine->setProjectionMatrix(m_engine->m_projection);
    m_engine->m_context->PSSetShaderResources(1, 1, &m_shaderResourceView);
    m_engine->drawTexturedRectangle(drawPosition, drawSize, textureOrigin, textureSize, m_drawColor);
    m_engine->loadMatrix(modelView);
}

void D3D11RenderSurface::drawBlurred(const Vector<2, float> &position, const Vector<2, float> &size, float radius,
                                     bool horizontal, float scale)
{
    draw(position, size, Vector<2, float>(0.0f, 0.0f), 1.0f);
}

void D3D11RenderSurface::drawDownsampled(const Vector<2, float> &position, const Vector<2, float> &size,
                                         const Vector<2, float> &texturePosition, float scale, int factor)
{
    draw(position, size, Vector<2, float>(0.0f, 0.0f), 1.0f);
}

void D3D11RenderSurface::begin()
{
    m_engine->m_context->OMGetRenderTargets(1, &m_savedRenderTargetView, &m_savedDepthStencilView);
    m_engine->m_context->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
    UINT count = 1;
    m_engine->m_context->RSGetViewports(&count, &m_savedViewport);
    D3D11_VIEWPORT viewport;
    viewport.Width = (float)m_width;
    viewport.Height = (float)m_height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    m_engine->m_context->RSSetViewports(1, &viewport);
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    m_engine->m_context->ClearRenderTargetView(m_renderTargetView, clearColor);
}

bool D3D11RenderSurface::resize(int width, int height)
{
    if (m_renderTargetView && width == m_width && height == m_height)
        return false;
    m_width = width;
    m_height = height;
    std::cout << "resize rendersurface " << width << " " << height << std::endl;
    if (m_renderTargetView) {
        m_renderTargetView->Release();
        m_renderTargetView = nullptr;
    }
    if (m_shaderResourceView) {
        m_shaderResourceView->Release();
        m_shaderResourceView = nullptr;
    }
    D3D11_TEXTURE2D_DESC description = {};
    description.Width = width;
    description.Height = height;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.SampleDesc.Quality = 0;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    description.CPUAccessFlags = 0;
    description.MiscFlags = 0;
    ID3D11Texture2D *texture;
    m_engine->m_device->CreateTexture2D(&description, nullptr, &texture);
    D3D11_SHADER_RESOURCE_VIEW_DESC view = {};
    view.Format = description.Format;
    view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    view.Texture2D.MostDetailedMip = 0;
    view.Texture2D.MipLevels = 1;
    m_engine->m_device->CreateShaderResourceView(texture, &view, &m_shaderResourceView);
    D3D11_RENDER_TARGET_VIEW_DESC target = {};
    target.Format = description.Format;
    target.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    target.Texture2D.MipSlice = 0;
    m_engine->m_device->CreateRenderTargetView(texture, &target, &m_renderTargetView);
    texture->Release();
    return true;
}

}
