// D3D11Texture creates a shader-resource view from image pixels on first use. It retains the view and releases the
// temporary ID3D11Texture2D after creating that view.
#pragma once
#include "plasma/Texture.h"

struct ID3D11ShaderResourceView;

namespace plasma {

class D3D11Engine;

class D3D11Texture : public Texture {
public:
    D3D11Texture(D3D11Engine *engine, int width, int height, const unsigned char *pixels, const TextureFormat &format,
                 const std::wstring &name, bool transient);
    virtual ~D3D11Texture();

    // Uploads the pixels first if there is no view yet (and then clears them if the texture is transient), then
    // ID3D11DeviceContext::PSSetShaderResources(slot, 1, &m_shaderResourceView).
    virtual void bind(unsigned slot);
    // Releases any old view, creates a width x height R8G8B8A8_UNORM ID3D11Texture2D (one mip level,
    // D3D11_BIND_SHADER_RESOURCE) with the pixels as initial data (pitch 4 * width), creates a TEXTURE2D view of it
    // into m_shaderResourceView and releases the texture.
    virtual void upload(const unsigned char *pixels);

protected:
    ID3D11ShaderResourceView *m_shaderResourceView; // 0 until the first Bind
    D3D11Engine *m_engine;
};

}
