#pragma once
#include <d3d9.h>
#include "plasma/Engine.h"

namespace plasma {
extern int g_drawingLayer;

// Shape pixel-shader filter selector, chosen from Display clipping and blend flags.
enum D3D9FilterMode {
    D3D9_FILTER_NONE = 0,
    D3D9_FILTER_CLIP = 1,
    D3D9_FILTER_MODULATION = 2,
    D3D9_FILTER_SCREEN = 3
};

class D3D9Drawing;
class D3D9RenderSurface;

class D3D9Engine : public Engine {
public:
    explicit D3D9Engine(IDirect3DDevice9 *device);
    virtual ~D3D9Engine();
    virtual bool setupShadersAndFrameBuffer();
    virtual Texture *createTexture(int width, int height, const unsigned char *pixels, const TextureFormat &format,
                                   const std::wstring &name, bool transient);
    virtual void drawCaret(const Vector<2, float> &, const Vector<2, float> &, const Vector<4, float> &);
    virtual void invertRectangle(const Vector<2, float> &, const Vector<2, float> &);
    virtual void drawTexturedRectangle(const Vector<2, float> &, const Vector<2, float> &, const Vector<2, float> &,
                                       const Vector<2, float> &, const Vector<4, float> &);
    virtual void fillRectangle(const Vector<2, float> &, const Vector<2, float> &, const Vector<4, float> &);
    virtual void pushIdentityMatrix() {}
    virtual void popMatrix() {}
    virtual void enableTexture2D() {}
    virtual void disableTexture2D() {}
    virtual Drawing *createDrawing();
    virtual RenderSurface *createRenderSurface(bool depthAttachment);
    virtual void getModelViewMatrix(Matrix<float> &matrix);
    virtual void loadMatrix(Matrix<float> &matrix);
    virtual void beginTextDrawing(Node *node);
    virtual void endTextDrawing(Node *) {}
    virtual void matrixLoaded();
    virtual void beginWindowDrawing();
    virtual void endWindowDrawing();
    virtual void pushNodeMatrix(Node *node);
    virtual void popNodeMatrix(Node *node);
    virtual void unused23(Node *) {}
    virtual void unused24(Node *) {}
    virtual void beginSurface(RenderSurface *, const Vector<2, float> &, const Vector<2, float> &, float scale,
                              bool alignPixels);
    virtual void endSurface();

    void restoreDeviceResources();
    void releaseDeviceResources();
    void drawSurfaceRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                              const Vector<2, float> &texturePosition, const Vector<2, float> &textureSize,
                              const Vector<4, float> &color);
    void setVertexShaderMatrix(unsigned registerIndex, const Matrix<float> &matrix, unsigned count);
    IDirect3DDevice9 *getDevice() const { return m_device; }

private:
    friend class D3D9Texture;
    friend class D3D9Drawing;
    friend class D3D9RenderSurface;
    IDirect3DVertexShader9 *m_shapeVertexShader;
    IDirect3DPixelShader9 *m_shapePixelShader;
    IDirect3DVertexDeclaration9 *m_shapeDeclaration;
    IDirect3DVertexShader9 *m_surfaceVertexShader;
    IDirect3DPixelShader9 *m_surfacePixelShader;
    IDirect3DPixelShader9 *m_downsample2PixelShader;
    IDirect3DPixelShader9 *m_downsample3PixelShader;
    IDirect3DPixelShader9 *m_downsample4PixelShader;
    IDirect3DPixelShader9 *m_horizontalBlurPixelShader;
    IDirect3DPixelShader9 *m_verticalBlurPixelShader;
    unsigned m_widgetDeformationRegister;
    unsigned m_projectionRegister;
    unsigned m_modelViewRegister;
    unsigned m_widgetBindMatrixRegister;
    unsigned m_inverseWidgetBindMatrixRegister;
    unsigned m_maskMatrixRegister;
    unsigned m_textureMatrixRegister;
    unsigned m_normalMatrixRegister;
    unsigned m_widgetBindPositionRegister;
    unsigned m_widgetBindSizeRegister;
    unsigned m_deformedWidgetPositionRegister;
    unsigned m_deformedWidgetSizeRegister;
    unsigned m_antialiasRegister;
    unsigned m_baseColorRegister;
    unsigned m_textureEnabledRegister;
    unsigned m_filterRegister;
    unsigned m_textureOpacityRegister;
    unsigned m_textureBrightnessRegister;
    unsigned m_textureContrastRegister;
    unsigned m_textureSaturationRegister;
    unsigned m_surfaceProjectionRegister;
    unsigned m_downsample2ScaleRegister;
    unsigned m_downsample3ScaleRegister;
    unsigned m_downsample4ScaleRegister;
    unsigned m_blur3ScaleRegister;
    unsigned m_blur2ScaleRegister;
    Matrix<float> m_modelViewMatrix;
    Matrix<float> m_projectionMatrix;
    float m_antialiasStrength;
    std::list<Matrix<float>> m_projectionStack;
    std::list<Matrix<float>> m_modelViewStack;
    std::list<D3D9Drawing *> m_drawings;
    IDirect3DDevice9 *m_device; // borrowed
    IDirect3DVertexBuffer9 *m_rectangleVertices;
    IDirect3DIndexBuffer9 *m_rectangleIndices;
    IDirect3DSurface9 *m_windowRenderTarget;
};
}
