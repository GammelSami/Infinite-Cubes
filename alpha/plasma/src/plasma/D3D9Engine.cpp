#include "plasma/D3D9Engine.h"
#include "plasma/D3D9Drawing.h"
#include "plasma/D3D9RenderSurface.h"
#include "plasma/D3D9Texture.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/RenderSurface.h"
#include "plasma/Widget.h"
#include "plasma/Display.h"
#include "plasma/D3D9Vertex.h"

namespace plasma {
// The renderer's values (anonymous: this file's own): the far plane of the drawing's orthographic
// projection, and the caret's blink (visible every other CaretBlinkInterval milliseconds of the
// caret clock).
namespace {
enum {
    DrawingFarPlane = 10000000
};
enum {
    CaretBlinkInterval = 500
};
}

int g_drawingLayer;

namespace {
// Shader headers are generated from HLSL sources listed in shaders.json.
#include "plasma/shaders/ShapeVertexShader.h"
#include "plasma/shaders/ShapePixelShader.h"
#include "plasma/shaders/SurfaceVertexShader.h"
#include "plasma/shaders/SurfacePixelShader.h"
#include "plasma/shaders/Downsample2PixelShader.h"
#include "plasma/shaders/Downsample3PixelShader.h"
#include "plasma/shaders/Downsample4PixelShader.h"
#include "plasma/shaders/VerticalBlurPixelShader.h"
#include "plasma/shaders/HorizontalBlurPixelShader.h"
struct ColoredVertex {
    float x, y, z;
    D3DCOLOR color;
};
}

D3D9Engine::D3D9Engine(IDirect3DDevice9 *device)
    : m_shapeVertexShader(0),
      m_shapePixelShader(0),
      m_surfaceVertexShader(0),
      m_surfacePixelShader(0),
      m_downsample2PixelShader(0),
      m_downsample3PixelShader(0),
      // The antialiasing's default strength is 0.7.
      m_downsample4PixelShader(0),
      m_horizontalBlurPixelShader(0),
      m_verticalBlurPixelShader(0),
      m_antialiasStrength(0.7f),
      m_device(device),
      m_rectangleVertices(0),
      m_rectangleIndices(0)
{}

D3D9Engine::~D3D9Engine()
{
    if (m_rectangleVertices)
        m_rectangleVertices->Release();
    if (m_rectangleIndices)
        m_rectangleIndices->Release();
    if (m_shapeVertexShader)
        m_shapeVertexShader->Release();
    if (m_shapePixelShader)
        m_shapePixelShader->Release();
    if (m_surfaceVertexShader)
        m_surfaceVertexShader->Release();
    if (m_surfacePixelShader)
        m_surfacePixelShader->Release();
    if (m_downsample2PixelShader)
        m_downsample2PixelShader->Release();
    if (m_downsample3PixelShader)
        m_downsample3PixelShader->Release();
    if (m_downsample4PixelShader)
        m_downsample4PixelShader->Release();
    if (m_horizontalBlurPixelShader)
        m_horizontalBlurPixelShader->Release();
    if (m_verticalBlurPixelShader)
        m_verticalBlurPixelShader->Release();
}

void D3D9Engine::beginWindowDrawing()
{
    m_modelViewMatrix.identity();
    m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
    m_device->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
    m_device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
    m_device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
    m_projectionMatrix.setOrthographic(0.0f, static_cast<float>(m_viewportWidth), static_cast<float>(m_viewportHeight),
                                       0.0f, -1.0f, DrawingFarPlane);
    m_device->GetRenderTarget(0, &m_windowRenderTarget);
    if (usesDepthLayers())
        m_device->Clear(0, 0, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0, 0.0f, 0);
    g_drawingLayer = 1;
}

void D3D9Engine::pushNodeMatrix(Node *node)
{
    m_modelViewStack.push_back(m_modelViewMatrix);
    if (m_modelViewStack.empty())
        m_modelViewMatrix = node->m_transformation->m_matrix;
    else
        m_modelViewMatrix = m_modelViewStack.back() * node->m_transformation->m_matrix;
}

void D3D9Engine::beginTextDrawing(Node *node)
{
    bool filterSurface = node && m_filterSurface &&
                         (!(node->m_display->m_flags & (1 << DISPLAY_BLUR)) ||
                          node->m_display->m_clipping.value() != DISPLAY_CLIPPING_DISABLED);
    Matrix<float> identity;
    identity.identity();
    setVertexShaderMatrix(m_textureMatrixRegister, identity, 2);
    if (filterSurface)
        m_filterSurface->bindTexture();
    BOOL textureEnabled = TRUE;
    m_device->SetPixelShaderConstantB(m_textureEnabledRegister, &textureEnabled, 1);
    int filter = D3D9_FILTER_NONE;
    if (node && node->m_display->m_clipping.value() != DISPLAY_CLIPPING_DISABLED) {
        if (node->m_display->m_flags & (1 << DISPLAY_MODULATION))
            filter = D3D9_FILTER_MODULATION;
        else if (node->m_display->m_flags & (1 << DISPLAY_SCREEN))
            filter = D3D9_FILTER_SCREEN;
        else
            filter = D3D9_FILTER_CLIP;
    }
    m_device->SetPixelShaderConstantF(m_filterRegister,
                                      &Vector<4, float>(static_cast<float>(filter), 0.0f, 0.0f, 0.0f)[0], 1);
    m_device->SetPixelShaderConstantF(m_textureOpacityRegister, &Vector<4, float>(1.0f, 0.0f, 0.0f, 0.0f)[0], 1);
    m_device->SetPixelShaderConstantF(m_textureBrightnessRegister, &Vector<4, float>(1.0f, 0.0f, 0.0f, 0.0f)[0], 1);
    m_device->SetPixelShaderConstantF(m_textureContrastRegister, &Vector<4, float>(1.0f, 0.0f, 0.0f, 0.0f)[0], 1);
    m_device->SetPixelShaderConstantF(m_textureSaturationRegister, &Vector<4, float>(1.0f, 0.0f, 0.0f, 0.0f)[0], 1);
    m_device->SetVertexDeclaration(m_shapeDeclaration);
    m_device->SetVertexShader(m_shapeVertexShader);
    m_device->SetPixelShader(m_shapePixelShader);
    BOOL deformationEnabled = FALSE;
    m_device->SetVertexShaderConstantB(m_widgetDeformationRegister, &deformationEnabled, 1);
    m_device->SetVertexShaderConstantF(m_baseColorRegister, &Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)[0], 1);
}

void D3D9Engine::beginSurface(RenderSurface *surface, const Vector<2, float> &position, const Vector<2, float> &size,
                              float scale, bool alignPixels)
{
    pushSurface(surface);
    surface->begin();
    if (alignPixels) {
        Vector<2, float> fraction = Vector<2, float>(m_modelViewMatrix[12], m_modelViewMatrix[13]) * scale;
        Vector<2, int> whole(fraction);
        fraction = fraction - Vector<2, float>(whole);
        if (scale == 1.0f)
            fraction = Vector<2, float>(0.0f, 0.0f);
        m_projectionStack.push_back(m_projectionMatrix);
        Vector<2, float> minimum = position;
        Vector<2, float> maximum = size;
        float margin = (1.0f - scale) * 0.5f;
        for (int dimension = 0; dimension < 2; ++dimension)
            maximum[dimension] *= margin;
        for (int dimension = 0; dimension < 2; ++dimension)
            minimum[dimension] -= maximum[dimension];
        maximum = size;
        for (int dimension = 0; dimension < 2; ++dimension)
            maximum[dimension] += minimum[dimension];
        m_projectionMatrix.setOrthographic(minimum[0], maximum[0], maximum[1], minimum[1], -1.0f, DrawingFarPlane);
        m_projectionMatrix.translate(-fraction[0], -fraction[1], 0.0f);
        m_projectionMatrix.scale(scale, scale);
    } else {
        m_modelViewMatrix[12] = static_cast<float>(static_cast<int>(m_modelViewMatrix[12] + 0.5f));
        m_modelViewMatrix[13] = static_cast<float>(static_cast<int>(m_modelViewMatrix[13] + 0.5f));
        m_projectionStack.push_back(m_projectionMatrix);
        m_projectionMatrix.setOrthographic(position[0], size[0] + position[0], size[1] + position[1], position[1],
                                           -1.0f, DrawingFarPlane);
        m_projectionMatrix.scale(scale, scale);
    }
}

Drawing *D3D9Engine::createDrawing()
{
    return new D3D9Drawing(this);
}

RenderSurface *D3D9Engine::createRenderSurface(bool depthAttachment)
{
    return new D3D9RenderSurface(this, depthAttachment);
}

Texture *D3D9Engine::createTexture(int width, int height, const unsigned char *pixels, const TextureFormat &format,
                                   const std::wstring &name, bool transient)
{
    return new D3D9Texture(this, width, height, pixels, format, name, transient);
}

void D3D9Engine::drawCaret(const Vector<2, float> &position, const Vector<2, float> &size,
                           const Vector<4, float> &color)
{
    if ((m_caretClock / CaretBlinkInterval) % 2) {
        m_device->SetTexture(0, 0);
        m_device->SetTexture(1, 0);
        m_device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
        m_device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
        m_device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_SUBTRACT);
        m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
        m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        ColoredVertex vertices[2];
        vertices[0].x = position[0];
        vertices[0].y = position[1];
        vertices[0].z = 0.0f;
        vertices[0].color = D3DCOLOR_XRGB(0, 0, 0);
        vertices[1].x = position[0];
        vertices[1].y = position[1] + size[1];
        vertices[1].z = 0.0f;
        vertices[1].color = D3DCOLOR_XRGB(0, 0, 0);
        m_device->DrawPrimitiveUP(D3DPT_LINELIST, 1, vertices, sizeof(ColoredVertex));
        m_device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        m_device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
    }
}

void D3D9Engine::invertRectangle(const Vector<2, float> &position, const Vector<2, float> &size)
{
    m_device->SetTexture(0, 0);
    m_device->SetTexture(1, 0);
    m_device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    m_device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    m_device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_SUBTRACT);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    float x = position.m_elements[0];
    float y = position.m_elements[1];
    float right = x + size.m_elements[0];
    float bottom = y + size.m_elements[1];
    ColoredVertex vertices[4] = {{x, y, 0.0f, D3DCOLOR_XRGB(0, 0, 0)},
                                 {right, y, 0.0f, D3DCOLOR_XRGB(0, 0, 0)},
                                 {right, bottom, 0.0f, D3DCOLOR_XRGB(0, 0, 0)},
                                 {x, bottom, 0.0f, D3DCOLOR_XRGB(0, 0, 0)}};
    m_device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(ColoredVertex));
    m_device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
}

void D3D9Engine::drawTexturedRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                                       const Vector<2, float> &texturePosition, const Vector<2, float> &textureSize,
                                       const Vector<4, float> &color)
{
    if (m_rectangleVertices && m_rectangleIndices) {
        D3D9Vertex vertices[4];
        Vector<2, float> offset(0.0f, 0.0f);
        vertices[0].position = position + offset;
        vertices[0].color = color;
        vertices[0].firstNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[0].secondNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[0].texture = texturePosition;
        vertices[1].position = Vector<2, float>(position[0] + size[0] + offset[0], position[1] + offset[1]);
        vertices[1].color = color;
        vertices[1].firstNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[1].secondNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[1].texture = Vector<2, float>(texturePosition[0] + textureSize[0], texturePosition[1]);
        vertices[2].position = Vector<2, float>(position[0] + size[0] + offset[0], position[1] + size[1] + offset[1]);
        vertices[2].color = color;
        vertices[2].firstNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[2].secondNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[2].texture = texturePosition + textureSize;
        vertices[3].position = Vector<2, float>(position[0] + offset[0], position[1] + size[1] + offset[1]);
        vertices[3].color = color;
        vertices[3].firstNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[3].secondNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[3].texture = Vector<2, float>(texturePosition[0], texturePosition[1] + textureSize[1]);
        m_device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(D3D9Vertex));
    }
}

void D3D9Engine::fillRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                               const Vector<4, float> &color)
{
    BOOL textureEnabled = FALSE;
    m_device->SetPixelShaderConstantB(m_textureEnabledRegister, &textureEnabled, 1);
    BOOL deformationEnabled = FALSE;
    m_device->SetVertexShaderConstantB(m_widgetDeformationRegister, &deformationEnabled, 1);
    D3D9Vertex vertices[4];
    // Fill vertices leave texture coordinates uninitialized.
    vertices[0].position = position;
    vertices[0].color = color;
    vertices[0].firstNormal = Vector<2, float>(0.0f, 0.0f);
    vertices[0].secondNormal = Vector<2, float>(0.0f, 0.0f);
    vertices[1].position = Vector<2, float>(position[0] + size[0], position[1]);
    vertices[1].color = color;
    vertices[1].firstNormal = Vector<2, float>(0.0f, 0.0f);
    vertices[1].secondNormal = Vector<2, float>(0.0f, 0.0f);
    vertices[2].position = position + size;
    vertices[2].color = color;
    vertices[2].firstNormal = Vector<2, float>(0.0f, 0.0f);
    vertices[2].secondNormal = Vector<2, float>(0.0f, 0.0f);
    vertices[3].position = Vector<2, float>(position[0], position[1] + size[1]);
    vertices[3].color = color;
    vertices[3].firstNormal = Vector<2, float>(0.0f, 0.0f);
    vertices[3].secondNormal = Vector<2, float>(0.0f, 0.0f);
    m_device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vertices, sizeof(D3D9Vertex));
}

void D3D9Engine::drawSurfaceRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                                      const Vector<2, float> &texturePosition, const Vector<2, float> &textureSize,
                                      const Vector<4, float> &color)
{
    if (m_rectangleVertices && m_rectangleIndices) {
        D3D9Vertex *vertices;
        m_rectangleVertices->Lock(0, 4 * sizeof(D3D9Vertex), reinterpret_cast<void **>(&vertices), D3DLOCK_DISCARD);
        Vector<2, float> offset(0.0f, 0.0f);
        vertices[0].position = position + offset;
        vertices[0].color = color;
        vertices[0].firstNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[0].secondNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[0].texture = texturePosition;
        vertices[1].position = Vector<2, float>(position[0] + size[0], position[1]) + offset;
        vertices[1].color = color;
        vertices[1].firstNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[1].secondNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[1].texture = Vector<2, float>(texturePosition[0] + textureSize[0], texturePosition[1]);
        vertices[2].position = Vector<2, float>(position[0] + size[0], position[1] + size[1]) + offset;
        vertices[2].color = color;
        vertices[2].firstNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[2].secondNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[2].texture =
            Vector<2, float>(texturePosition[0] + textureSize[0], textureSize[1] + texturePosition[1]);
        vertices[3].position = Vector<2, float>(position[0], position[1] + size[1]) + offset;
        vertices[3].color = color;
        vertices[3].firstNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[3].secondNormal = Vector<2, float>(0.0f, 0.0f);
        vertices[3].texture = Vector<2, float>(texturePosition[0], textureSize[1] + texturePosition[1]);
        m_rectangleVertices->Unlock();
        setVertexShaderMatrix(m_surfaceProjectionRegister, m_projectionMatrix, 4);
        m_device->SetStreamSource(0, m_rectangleVertices, 0, sizeof(D3D9Vertex));
        m_device->SetIndices(m_rectangleIndices);
        m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);
    }
}

void D3D9Engine::endWindowDrawing()
{
    m_windowRenderTarget->Release();
}

void D3D9Engine::popNodeMatrix(Node *node)
{
    if (!m_modelViewStack.empty()) {
        m_modelViewMatrix = m_modelViewStack.back();
        m_modelViewStack.pop_back();
    }
}

void D3D9Engine::endSurface()
{
    m_surfaceStack.back()->end();
    popSurface();
    if (!m_projectionStack.empty()) {
        m_projectionMatrix = m_projectionStack.back();
        m_projectionStack.pop_back();
    }
}

void D3D9Engine::getModelViewMatrix(Matrix<float> &matrix)
{
    matrix = m_modelViewMatrix;
}

bool D3D9Engine::setupShadersAndFrameBuffer()
{
    m_shapeDeclaration = 0;
    // The shapes' vertices: a float2 position, then a float4 colour (D3D9Vertex).
    D3DVERTEXELEMENT9 declaration[] = {{0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
                                       {0, 8, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
                                       {0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
                                       {0, 32, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
                                       {0, 40, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2},
                                       D3DDECL_END()};
    if (m_device->CreateVertexDeclaration(declaration, &m_shapeDeclaration) != D3D_OK)
        return false;
    if (m_device->CreateVertexShader(shapeVertexShader, &m_shapeVertexShader) != D3D_OK)
        return false;
    if (m_device->CreatePixelShader(shapePixelShader, &m_shapePixelShader) != D3D_OK)
        return false;
    if (m_device->CreateVertexShader(surfaceVertexShader, &m_surfaceVertexShader) != D3D_OK)
        return false;
    if (m_device->CreatePixelShader(surfacePixelShader, &m_surfacePixelShader) != D3D_OK)
        return false;
    if (m_device->CreatePixelShader(downsample2PixelShader, &m_downsample2PixelShader) != D3D_OK)
        return false;
    if (m_device->CreatePixelShader(downsample3PixelShader, &m_downsample3PixelShader) != D3D_OK)
        return false;
    if (m_device->CreatePixelShader(downsample4PixelShader, &m_downsample4PixelShader) != D3D_OK)
        return false;
    if (m_device->CreatePixelShader(horizontalBlurPixelShader, &m_horizontalBlurPixelShader) != D3D_OK)
        return false;
    if (m_device->CreatePixelShader(verticalBlurPixelShader, &m_verticalBlurPixelShader) != D3D_OK)
        return false;
    m_widgetDeformationRegister = 0;
    m_projectionRegister = 0;
    m_modelViewRegister = 4;
    m_widgetBindMatrixRegister = 8;
    m_inverseWidgetBindMatrixRegister = 12;
    m_maskMatrixRegister = 16;
    m_textureMatrixRegister = 18;
    m_normalMatrixRegister = 20;
    m_widgetBindPositionRegister = 22;
    m_widgetBindSizeRegister = 23;
    m_deformedWidgetPositionRegister = 24;
    m_deformedWidgetSizeRegister = 25;
    m_antialiasRegister = 26;
    m_baseColorRegister = 27;
    m_textureEnabledRegister = 0;
    m_filterRegister = 0;
    m_textureOpacityRegister = 1;
    m_textureBrightnessRegister = 2;
    m_textureContrastRegister = 3;
    m_textureSaturationRegister = 4;
    m_surfaceProjectionRegister = 0;
    m_downsample2ScaleRegister = 0;
    m_downsample3ScaleRegister = 0;
    m_downsample4ScaleRegister = 0;
    m_blur3ScaleRegister = 0;
    m_blur2ScaleRegister = 0;
    if (m_device->CreateVertexBuffer(4 * sizeof(D3D9Vertex), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT,
                                     &m_rectangleVertices, 0) != D3D_OK)
        return false;
    // Six 32-bit indices: the rectangle's two triangles.
    if (m_device->CreateIndexBuffer(24, D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_MANAGED, &m_rectangleIndices, 0) !=
        D3D_OK)
        return false;
    int *indices;
    m_rectangleIndices->Lock(0, 24, reinterpret_cast<void **>(&indices), D3DLOCK_DISCARD);
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 2;
    indices[4] = 3;
    indices[5] = 0;
    m_rectangleIndices->Unlock();
    return true;
}

void D3D9Engine::restoreDeviceResources()
{
    if (!m_rectangleVertices)
        m_device->CreateVertexBuffer(4 * sizeof(D3D9Vertex), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT,
                                     &m_rectangleVertices, 0);
    for (std::list<D3D9Drawing *>::iterator it = m_drawings.begin(); it != m_drawings.end(); ++it)
        (*it)->restoreDeviceResources();
}

void D3D9Engine::releaseDeviceResources()
{
    if (m_rectangleVertices) {
        m_rectangleVertices->Release();
        m_rectangleVertices = 0;
    }
    for (int i = 0; i < static_cast<int>(m_renderSurfaces.size()); ++i)
        deleteRenderSurface(m_renderSurfaces[i]);
    m_renderSurfaces.clear();
    for (std::list<Widget *>::iterator it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if ((*it)->m_surface) {
            D3D9RenderSurface *surface = dynamic_cast<D3D9RenderSurface *>((*it)->m_surface);
            if (surface)
                surface->releaseDeviceResources();
        }
    }
    for (std::list<D3D9Drawing *>::iterator it = m_drawings.begin(); it != m_drawings.end(); ++it)
        (*it)->releaseDeviceResources();
    deleteRenderSurface(m_blurSurface);
    m_blurSurface = 0;
}

void D3D9Engine::loadMatrix(Matrix<float> &matrix)
{
    m_modelViewMatrix = matrix;
}

void D3D9Engine::matrixLoaded()
{
    setVertexShaderMatrix(m_projectionRegister, m_projectionMatrix, 4);
    setVertexShaderMatrix(m_modelViewRegister, m_modelViewMatrix, 4);
    Matrix<float> normalMatrix = m_modelViewMatrix;
    normalMatrix.invert();
    normalMatrix.transpose();
    setVertexShaderMatrix(m_normalMatrixRegister, normalMatrix, 2);
}

void D3D9Engine::setVertexShaderMatrix(unsigned registerIndex, const Matrix<float> &matrix, unsigned count)
{
    Matrix<float> transposed = matrix;
    transposed.transpose();
    m_device->SetVertexShaderConstantF(registerIndex, &transposed[0], count);
}

}
