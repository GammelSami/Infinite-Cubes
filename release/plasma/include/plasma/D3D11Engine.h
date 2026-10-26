// D3D11Engine renders Plasma shapes, text and interface rectangles. It manages model-view and projection matrices and
// stacks, registered drawings and GPU resources. Constant-buffer setters cache the last uploaded value and call
// UpdateSubresource when it changes.
#pragma once
#include <list>
#include "plasma/Engine.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

// Direct3D 11 interfaces (d3d11.h); only pointers are held.
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11RasterizerState;
struct ID3D11BlendState;
struct ID3D11DepthStencilState;

namespace plasma {

class D3D11Drawing;
class D3D11RenderSurface;
class D3D11Texture;

enum VertexShaderBufferSlot {
    VS_BUFFER_PROJ = 0,    // m_projectionBuffer
    VS_BUFFER_WORLD = 1,   // m_modelViewBuffer
    VS_BUFFER_MASK = 2,    // m_pixelScaledModelViewBuffer
    VS_BUFFER_TEXTURE = 3, // m_textureMatrixBuffer
    VS_BUFFER_WIDGET_DEFORMATION_ENABLED = 4,
    VS_BUFFER_WIDGET_BIND_MATRIX = 5,
    VS_BUFFER_INVERSE_WIDGET_BIND_MATRIX = 6,
    VS_BUFFER_WIDGET_BIND_POS = 7,
    VS_BUFFER_WIDGET_BIND_SIZE = 8,
    VS_BUFFER_DEFORMED_WIDGET_POS = 9,
    VS_BUFFER_DEFORMED_WIDGET_SIZE = 10,
    VS_BUFFER_BASE_COLOR = 11 // m_colorBuffer
};
enum PixelShaderBufferSlot {
    PS_BUFFER_TEXTURE_ENABLED = 0,
    PS_BUFFER_FILTER = 1,
    PS_BUFFER_TEXTURE_PARAMETERS = 2 // m_textureAdjustmentsBuffer
};

// Pixel-shader filter mode set by the node's display: none without clipping, modulation or screen when requested,
// otherwise alpha masking with the filter surface.
enum FilterMode {
    FILTER_NONE = 0,
    FILTER_CLIP = 1,       // the mask's colour over the shape by the mask's alpha
    FILTER_MODULATION = 2, // multiplied by the mask
    FILTER_SCREEN = 3      // the shape's alpha multiplied by the mask's alpha
};

class D3D11Engine : public Engine {
public:
    D3D11Engine(ID3D11Device *device, ID3D11DeviceContext *context);
    virtual ~D3D11Engine();

    virtual bool setupShadersAndFrameBuffer();
    virtual Texture *createTexture(int width, int height, const unsigned char *pixels, const TextureFormat &format,
                                   const std::wstring &name = L"", bool transient = false);
    virtual void drawCaret(const Vector<2, float> &position, const Vector<2, float> &size,
                           const Vector<4, float> &color);
    virtual void drawTexturedRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                                       const Vector<2, float> &texturePosition, const Vector<2, float> &textureSize,
                                       const Vector<4, float> &color);
    virtual void fillRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                               const Vector<4, float> &color);
    virtual Drawing *createDrawing();
    virtual RenderSurface *createRenderSurface(bool depthAttachment);
    virtual void getModelViewMatrix(Matrix<float> &matrix);
    virtual void loadMatrix(Matrix<float> &matrix);
    virtual void applyRenderState();
    virtual void beginTextDrawing(Node *node);
    virtual void beginWindowDrawing();
    virtual void pushNodeMatrix(Node *node);
    virtual void popNodeMatrix(Node *node);
    virtual void beginSurface(RenderSurface *surface, const Vector<2, float> &position, const Vector<2, float> &size,
                              float scale, bool alignPixels);
    virtual void endSurface();

    // Constant-buffer setters cache and upload their values. Matrix setters compare the argument's address with the
    // cache rather than its contents, so external calls upload every time; matrices are transposed before upload.
    void setProjectionMatrix(const Matrix<float> &matrix); // VS b0, m_shaderProjection
    void setModelViewMatrix(const Matrix<float> &matrix);  // VS b1, m_shaderModelView
    // Sets the model-view matrix scaled by inverse viewport width and height for D3D11Drawing.
    void setPixelScaledModelViewMatrix(const Matrix<float> &matrix); // VS b2, m_shaderPixelScaledModelView
    void setTextureMatrix(const Matrix<float> &matrix);              // VS b3, m_shaderTextureMatrix
    void setWidgetDeformationEnabled(bool enabled);                  // VS b4
    void setWidgetBindMatrix(const Matrix<float> &matrix);           // VS b5
    void setInverseWidgetBindMatrix(const Matrix<float> &matrix);    // VS b6
    void setWidgetBindPos(const Vector<2, float> &position);         // VS b7
    void setWidgetBindSize(const Vector<2, float> &size);            // VS b8
    void setDeformedWidgetPos(const Vector<2, float> &position);     // VS b9
    void setDeformedWidgetSize(const Vector<2, float> &size);        // VS b10
    void setColor(const Vector<4, float> &color);                    // VS b11
    void setTextureEnabled(bool enabled);                            // PS b0
    void setFilter(int filter);                                      // PS b1, a FilterMode
    void setTextureAdjustments(float opacity, float brightness, float contrast, float saturation); // PS b2

protected:
    friend class D3D11Drawing;       // Registers drawings, draws with m_context and pops m_modelViewStack.
    friend class D3D11RenderSurface; // Creates views with m_device, binds them with m_context and reads m_projection.
    friend class D3D11Texture;       // Creates and updates textures with m_device and m_context.

    // Not initialised by the constructor; set by beginWindowDrawing and loadMatrix, read by getModelViewMatrix and the
    // draw functions.
    Matrix<float> m_modelView;
    // Set by beginWindowDrawing and beginSurface; passed to setProjectionMatrix by beginTextDrawing and
    // D3D11RenderSurface::draw.
    Matrix<float> m_projection;
    std::list<Matrix<float>> m_projectionStack; // beginSurface pushes, endSurface pops
    std::list<Matrix<float>> m_modelViewStack;  // pushNodeMatrix pushes, popNodeMatrix and D3D11Drawing pop
    std::list<D3D11Drawing *> m_drawings;       // D3D11Drawing's constructor appends itself
    ID3D11Device *m_device;                     // constructor argument; not released
    ID3D11DeviceContext *m_context;             // constructor argument (the immediate context); not released

    ID3D11Buffer *m_vertexBuffer; // Quad vertex buffer; vertex stride 32 bytes.
    ID3D11Buffer *m_indexBuffer;  // Quad index buffer with six 32-bit indices.
    ID3D11VertexShader *m_vertexShader;
    ID3D11PixelShader *m_pixelShader;
    ID3D11InputLayout *m_inputLayout; // Input layout for POSITION and COLOR.
    ID3D11RasterizerState *m_rasterizerState;
    ID3D11BlendState *m_blendState;
    ID3D11DepthStencilState *m_depthStencilState;
    // Not zeroed by the constructor.
    ID3D11Buffer *m_projectionBuffer;               // VS b0, 64 bytes
    ID3D11Buffer *m_modelViewBuffer;                // VS b1, 64 bytes
    ID3D11Buffer *m_pixelScaledModelViewBuffer;     // VS b2, 64 bytes
    ID3D11Buffer *m_textureMatrixBuffer;            // VS b3, 64 bytes
    ID3D11Buffer *m_widgetDeformationEnabledBuffer; // VS b4, 16 bytes
    ID3D11Buffer *m_widgetBindMatrixBuffer;         // VS b5, 64 bytes
    ID3D11Buffer *m_inverseWidgetBindMatrixBuffer;  // VS b6, 64 bytes
    ID3D11Buffer *m_widgetBindPosBuffer;            // VS b7, 16 bytes
    ID3D11Buffer *m_widgetBindSizeBuffer;           // VS b8, 16 bytes
    ID3D11Buffer *m_deformedWidgetPosBuffer;        // VS b9, 16 bytes
    ID3D11Buffer *m_deformedWidgetSizeBuffer;       // VS b10, 16 bytes
    ID3D11Buffer *m_colorBuffer;                    // VS b11, 16 bytes
    ID3D11Buffer *m_textureEnabledBuffer;           // PS b0, 16 bytes
    ID3D11Buffer *m_filterBuffer;                   // PS b1, 16 bytes
    ID3D11Buffer *m_textureAdjustmentsBuffer;       // PS b2, 16 bytes

    // The constant-buffer values as last uploaded (GLEngine's "uniform values, as last set"). The constructor zeroes
    // the matrices and sets the others to values no caller passes, so the first set uploads.
    Matrix<float> m_shaderModelView;            // setModelViewMatrix; zeroed
    Matrix<float> m_shaderProjection;           // setProjectionMatrix; zeroed
    Matrix<float> m_shaderPixelScaledModelView; // setPixelScaledModelViewMatrix; zeroed
    Matrix<float> m_shaderTextureMatrix;        // setTextureMatrix; zeroed
    bool m_widgetDeformationEnabled;            // setWidgetDeformationEnabled; true at construction
    Matrix<float> m_widgetBindMatrix;           // setWidgetBindMatrix; zeroed
    Matrix<float> m_inverseWidgetBindMatrix;    // setInverseWidgetBindMatrix; zeroed
    Vector<2, float> m_widgetBindPos;           // setWidgetBindPos; (-1e8, -1e7) at construction
    Vector<2, float> m_widgetBindSize;          // setWidgetBindSize; (-1e8, -1e7)
    Vector<2, float> m_deformedWidgetPos;       // setDeformedWidgetPos; (-1e8, -1e7)
    Vector<2, float> m_deformedWidgetSize;      // setDeformedWidgetSize; (-1e8, -1e7)
    Vector<4, float> m_color;                   // setColor; -1, -1, -1, -1
    bool m_textureEnabled;                      // setTextureEnabled; true at construction
    int m_filter;                               // setFilter; -1 at construction
    Vector<4, float> m_textureAdjustments;
};

}
