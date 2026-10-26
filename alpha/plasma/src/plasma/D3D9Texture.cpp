#include "plasma/D3D9Texture.h"
#include "plasma/D3D9Engine.h"
#include <cstring>

namespace plasma {

D3D9Texture::D3D9Texture(D3D9Engine *engine, int width, int height, const unsigned char *pixels,
                         const TextureFormat &format, const std::wstring &name, bool transient)
    : Texture(engine, name, transient),
      m_texture(0),
      m_d3dEngine(engine)
{
    setPixels(width, height, pixels, format);
}

D3D9Texture::~D3D9Texture()
{
    if (m_texture)
        m_texture->Release();
}

void D3D9Texture::bind(int stage)
{
    if (!m_texture) {
        upload(&m_pixels[0]);
        if (m_transient)
            m_pixels.clear();
    }
    m_d3dEngine->m_device->SetTexture(stage, m_texture);
    switch (m_format.minFilter) {
    case TEXTURE_FILTER_NEAREST:
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        break;
    case TEXTURE_FILTER_LINEAR:
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        break;
    case TEXTURE_FILTER_MIPMAP:
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        break;
    }
    switch (m_format.maxFilter) {
    case TEXTURE_FILTER_NEAREST:
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        break;
    case TEXTURE_FILTER_LINEAR:
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        break;
    case TEXTURE_FILTER_MIPMAP:
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        break;
    }
    switch (m_format.horizontalWrap) {
    case TEXTURE_WRAP_NONE:
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
        break;
    case TEXTURE_WRAP_REPEAT:
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
        break;
    case TEXTURE_WRAP_MIRROR:
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
        break;
    }
    switch (m_format.verticalWrap) {
    case TEXTURE_WRAP_NONE:
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
        break;
    case TEXTURE_WRAP_REPEAT:
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
        break;
    case TEXTURE_WRAP_MIRROR:
        m_d3dEngine->m_device->SetSamplerState(stage, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
        break;
    }
}

void D3D9Texture::upload(const unsigned char *pixels)
{
    if (m_texture) {
        m_texture->Release();
        m_texture = 0;
    }
    int channels = 4;
    D3DFORMAT format = D3DFMT_A8R8G8B8;
    if (m_format.pixelFormat == TEXTURE_RGB) {
        channels = 3;
        format = D3DFMT_R8G8B8;
    }
    IDirect3DDevice9 *device = m_d3dEngine->m_device;
    if (m_format.minFilter == TEXTURE_FILTER_MIPMAP)
        device->CreateTexture(m_width, m_height, 0, 0, format, D3DPOOL_MANAGED, &m_texture, 0);
    else
        device->CreateTexture(m_width, m_height, 1, 0, format, D3DPOOL_MANAGED, &m_texture, 0);
    if (m_texture) {
        D3DLOCKED_RECT locked;
        m_texture->LockRect(0, &locked, 0, D3DLOCK_DISCARD);
        if (channels == 3) {
            for (int y = 0; y < m_height; ++y)
                memcpy(static_cast<unsigned char *>(locked.pBits) + y * locked.Pitch, pixels + y * m_width * 3,
                       m_width * 3);
        } else {
            for (int y = 0; y < m_height; ++y) {
                unsigned char *destination = static_cast<unsigned char *>(locked.pBits) + y * locked.Pitch;
                const unsigned char *source = pixels + y * m_width * channels;
                for (int x = 0; x < m_width; ++x) {
                    int offset = x * 4;
                    for (int channel = 0; channel < 4; ++channel)
                        destination[offset + channel] = source[offset + (channel < 3 ? 2 - channel : 3)];
                }
            }
        }
        m_texture->UnlockRect(0);
    }
}

}
