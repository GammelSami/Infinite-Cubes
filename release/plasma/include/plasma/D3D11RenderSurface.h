// D3D11RenderSurface renders into a texture with render-target and shader-resource views. It saves and restores the
// previous render target, depth target and viewport around surface drawing.
#pragma once
#include <d3d11.h>
#include "plasma/RenderSurface.h"

namespace plasma {

class D3D11Engine;

class D3D11RenderSurface : public RenderSurface {
public:
    D3D11RenderSurface(D3D11Engine *engine, bool depthAttachment);
    virtual ~D3D11RenderSurface();

    virtual bool resize(int width, int height);
    virtual void begin();
    virtual void end();
    virtual void bindTexture();
    virtual void draw(const Vector<2, float> &position, const Vector<2, float> &size,
                      const Vector<2, float> &texturePosition, float scale);
    virtual void drawBlurred(const Vector<2, float> &position, const Vector<2, float> &size, float radius,
                             bool horizontal, float scale);
    virtual void drawDownsampled(const Vector<2, float> &position, const Vector<2, float> &size,
                                 const Vector<2, float> &texturePosition, float scale, int factor);

protected:
    int m_width; // Width and height, updated by resize.
    int m_height;
    D3D11Engine *m_engine;
    ID3D11RenderTargetView *m_renderTargetView;      // CreateRenderTargetView (device) in Resize
    ID3D11ShaderResourceView *m_shaderResourceView;  // CreateShaderResourceView (device) in Resize
    ID3D11RenderTargetView *m_savedRenderTargetView; // OMGetRenderTargets (context) in Begin
    ID3D11DepthStencilView *m_savedDepthStencilView; // likewise; rebound and released by End
    D3D11_VIEWPORT m_savedViewport;                  // RSGetViewports (context) in Begin; restored by End
};

}
