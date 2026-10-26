// Implements texture upload and binding; creates the shader-resource view on first bind.
#include <d3d11.h>
#include "plasma/D3D11Texture.h"
#include "plasma/D3D11Engine.h"
#include "plasma/EditFilter.h"

namespace plasma {

D3D11Texture::D3D11Texture(D3D11Engine *engine, int width, int height, const unsigned char *pixels,
                           const TextureFormat &format, const std::wstring &name, bool transient)
    : Texture(engine, name, transient),
      m_shaderResourceView(nullptr),
      m_engine(engine)
{
    setPixels(width, height, pixels, format);
}

D3D11Texture::~D3D11Texture()
{
    if (m_shaderResourceView)
        m_shaderResourceView->Release();
}

void D3D11Texture::bind(unsigned slot)
{
    if (!m_shaderResourceView) {
        upload(&m_pixels[0]);
        // A transient texture keeps no copy of its pixels once uploaded.
        if (m_transient)
            m_pixels.clear();
    }
    m_engine->m_context->PSSetShaderResources(slot, 1, &m_shaderResourceView);
}

void D3D11Texture::upload(const unsigned char *pixels)
{
    if (m_shaderResourceView) {
        m_shaderResourceView->Release();
        m_shaderResourceView = nullptr;
    }
    // The format's pixel format, filters and wraps are ignored: always RGBA with 8 bits per channel.
    D3D11_TEXTURE2D_DESC description = {};
    description.Width = m_width;
    description.Height = m_height;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.SampleDesc.Quality = 0;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    description.CPUAccessFlags = 0;
    description.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = pixels;
    data.SysMemPitch = 4 * m_width;
    data.SysMemSlicePitch = 0;
    ID3D11Texture2D *texture;
    m_engine->m_device->CreateTexture2D(&description, &data, &texture);
    D3D11_SHADER_RESOURCE_VIEW_DESC view = {};
    view.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    view.Texture2D.MipLevels = 1;
    m_engine->m_device->CreateShaderResourceView(texture, &view, &m_shaderResourceView);
    texture->Release();
}

}
