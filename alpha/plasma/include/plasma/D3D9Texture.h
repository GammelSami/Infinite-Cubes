#pragma once
#include <d3d9.h>
#include "plasma/Texture.h"

namespace plasma {
class D3D9Engine;

class D3D9Texture : public Texture {
public:
    D3D9Texture(D3D9Engine *engine, int width, int height, const unsigned char *pixels, const TextureFormat &format,
                const std::wstring &name, bool transient);
    ~D3D9Texture();
    virtual void bind(int stage);
    virtual void upload(const unsigned char *pixels);

private:
    IDirect3DTexture9 *m_texture;
    D3D9Engine *m_d3dEngine;
};
}
