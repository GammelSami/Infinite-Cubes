#pragma once
#include <d3d9.h>
#include "plasma/RenderSurface.h"

namespace plasma {
class D3D9Engine;

class D3D9RenderSurface : public RenderSurface {
public:
    D3D9RenderSurface(D3D9Engine *engine, bool depthAttachment);
    virtual ~D3D9RenderSurface();
    virtual bool resize(int width, int height);
    virtual void begin();
    virtual void end();
    virtual void bindTexture();
    virtual void draw(const Vector<2, float> &position, const Vector<2, float> &size,
                      const Vector<2, float> &texturePosition, float scale);
    virtual void drawBlurred(const Vector<2, float> &, const Vector<2, float> &, float, bool, float);
    virtual void drawDownsampled(const Vector<2, float> &, const Vector<2, float> &, const Vector<2, float> &, float,
                                 int);
    virtual void clearRect(const Vector<2, float> &, const Vector<2, float> &, bool);
    virtual void clear();
    void releaseDeviceResources();

private:
    int m_width;
    int m_height;
    D3D9Engine *m_engine;
    IDirect3DTexture9 *m_colorTexture;
    IDirect3DSurface9 *m_colorSurface;
    IDirect3DSurface9 *m_depthSurface;
    IDirect3DSurface9 *m_previousColorSurface; // retained between Begin and End
    IDirect3DSurface9 *m_previousDepthSurface;
};
}
