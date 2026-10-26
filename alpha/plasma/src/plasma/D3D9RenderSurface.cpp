#include "plasma/D3D9RenderSurface.h"
#include "plasma/D3D9Engine.h"

namespace plasma {

D3D9RenderSurface::D3D9RenderSurface(D3D9Engine *engine, bool depthAttachment)
    : RenderSurface(depthAttachment),
      m_engine(engine),
      m_width(0),
      m_height(0),
      m_colorTexture(0),
      m_colorSurface(0),
      m_depthSurface(0),
      m_previousColorSurface(0)
{}

D3D9RenderSurface::~D3D9RenderSurface()
{
    if (m_depthSurface)
        m_depthSurface->Release();
    if (m_colorSurface)
        m_colorSurface->Release();
    if (m_colorTexture)
        m_colorTexture->Release();
}

void D3D9RenderSurface::bindTexture()
{
    m_engine->m_device->SetTexture(0, m_colorTexture);
    m_engine->m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    m_engine->m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    m_engine->m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
}

void D3D9RenderSurface::clearRect(const Vector<2, float> &position, const Vector<2, float> &size, bool loadIdentity)
{
    Matrix<float> projection = m_engine->m_projectionMatrix;
    Vector<2, float> minimum, maximum;
    if (loadIdentity) {
        minimum = position;
        maximum = position + size;
    } else {
        Matrix<float> modelView;
        m_engine->getModelViewMatrix(modelView);
        Vector<2, float> points[4];
        points[0] = modelView.transform(position);
        points[1] = modelView.transform(Vector<2, float>(position[0] + size[0], position[1]));
        points[2] = modelView.transform(position + size);
        points[3] = modelView.transform(Vector<2, float>(position[0], position[1] + size[1]));
        minimum = maximum = points[0];
        for (int i = 1; i < 4; ++i) {
            minimum[0] = minimum[0] < points[i][0] ? minimum[0] : points[i][0];
            minimum[1] = minimum[1] < points[i][1] ? minimum[1] : points[i][1];
            maximum[0] = maximum[0] > points[i][0] ? maximum[0] : points[i][0];
            maximum[1] = maximum[1] > points[i][1] ? maximum[1] : points[i][1];
        }
    }
    minimum = projection.transform(minimum);
    maximum = projection.transform(maximum);
    // The cleared rectangle reaches a pixel beyond the bounds to the left and two to the right and
    // below.
    D3DRECT rectangle;
    rectangle.x1 = static_cast<int>((minimum[0] + 1.0f) * 0.5f * m_width) - 1;
    rectangle.y1 = static_cast<int>((minimum[1] - 1.0f) * 0.5f * -m_height);
    rectangle.x2 = static_cast<int>((maximum[0] + 1.0f) * 0.5f * m_width) + 2;
    rectangle.y2 = static_cast<int>((maximum[1] - 1.0f) * 0.5f * -m_height) + 2;
    m_engine->m_device->Clear(1, &rectangle, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0, 0.0f, 0);
}

void D3D9RenderSurface::clear()
{
    m_engine->m_device->Clear(
        0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
        D3DCOLOR_ARGB(static_cast<int>(m_clearColor[3] * 255.0f), static_cast<int>(m_clearColor[0] * 255.0f),
                      static_cast<int>(m_clearColor[1] * 255.0f), static_cast<int>(m_clearColor[2] * 255.0f)),
        0.0f, 0);
}

void D3D9RenderSurface::end()
{
    m_engine->m_device->SetRenderTarget(0, m_previousColorSurface);
    m_engine->m_device->SetDepthStencilSurface(m_previousDepthSurface);
    m_previousColorSurface->Release();
    m_previousDepthSurface->Release();
}

void D3D9RenderSurface::draw(const Vector<2, float> &position, const Vector<2, float> &size,
                             const Vector<2, float> &texturePosition, float scale)
{
    Matrix<float> saved;
    m_engine->getModelViewMatrix(saved);
    Vector<2, float> offset(saved[12] * scale, saved[13] * scale);
    Vector<2, int> whole(offset);
    offset = offset - Vector<2, float>(whole);
    if (scale == 1.0f)
        offset = Vector<2, float>(0.0f, 0.0f);
    Matrix<float> identity;
    identity.identity();
    m_engine->loadMatrix(identity);
    float x = position[0];
    float y = position[1];
    float width = size[0];
    float height = size[1];
    if (width < 0.0f)
        width = static_cast<float>(m_width);
    if (height < 0.0f)
        height = static_cast<float>(m_height);
    Vector<2, float> shift =
        Vector<2, float>(static_cast<float>(m_width), static_cast<float>(m_height)) * ((1.0f - scale) * 0.5f);
    if (scale < 0.0f) {
        scale = static_cast<float>(m_width) / size[0];
        offset = Vector<2, float>(0.0f, 0.0f);
        shift = Vector<2, float>(0.0f, 0.0f);
    }
    shift = shift - offset;
    float s = (texturePosition[0] * scale + shift[0]) / m_width;
    float t = (texturePosition[1] * scale + shift[1]) / m_height;
    float textureWidth = width * scale / m_width;
    float textureHeight = height * scale / m_height;
    m_engine->m_device->SetTexture(0, m_colorTexture);
    m_engine->m_device->SetVertexDeclaration(m_engine->m_shapeDeclaration);
    m_engine->m_device->SetVertexShader(m_engine->m_surfaceVertexShader);
    m_engine->m_device->SetPixelShader(m_engine->m_surfacePixelShader);
    m_engine->m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_engine->m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_engine->drawSurfaceRectangle(Vector<2, float>(x, y), Vector<2, float>(width, height), Vector<2, float>(s, t),
                                   Vector<2, float>(textureWidth, textureHeight), m_drawColor);
    m_engine->loadMatrix(saved);
}

void D3D9RenderSurface::drawBlurred(const Vector<2, float> &position, const Vector<2, float> &size, float radius,
                                    bool horizontal, float scale)
{
    Matrix<float> saved;
    m_engine->getModelViewMatrix(saved);
    Vector<2, float> offset(saved[12] * scale, saved[13] * scale);
    Vector<2, int> whole(offset);
    offset = offset - Vector<2, float>(whole);
    if (scale == 1.0f)
        offset = Vector<2, float>(0.0f, 0.0f);
    Matrix<float> identity;
    identity.identity();
    m_engine->loadMatrix(identity);
    float x = position[0];
    float y = position[1];
    float width = size[0];
    float height = size[1];
    Vector<2, float> shift =
        Vector<2, float>(static_cast<float>(m_width), static_cast<float>(m_height)) * ((1.0f - scale) * 0.5f) - offset;
    float s = (x * scale + shift[0] + 0.5f) / m_width;
    float t = (y * scale + shift[1] + 0.5f) / m_height;
    float textureWidth = width * scale / m_width;
    float textureHeight = height * scale / m_height;
    // Four sample steps reach the requested blur radius.
    radius *= 0.25f;
    m_engine->m_device->SetTexture(0, m_colorTexture);
    m_engine->m_device->SetVertexDeclaration(m_engine->m_shapeDeclaration);
    m_engine->m_device->SetVertexShader(m_engine->m_surfaceVertexShader);
    if (horizontal) {
        m_engine->m_device->SetPixelShader(m_engine->m_horizontalBlurPixelShader);
        Vector<4, float> blurScale(radius / m_width, 0.0f, 0.0f, 0.0f);
        m_engine->m_device->SetPixelShaderConstantF(m_engine->m_blur3ScaleRegister, &blurScale[0], 1);
    } else {
        m_engine->m_device->SetPixelShader(m_engine->m_verticalBlurPixelShader);
        // The code vertical pass also divides the radius by the width.
        Vector<4, float> blurScale(radius / m_width, 0.0f, 0.0f, 0.0f);
        m_engine->m_device->SetPixelShaderConstantF(m_engine->m_blur2ScaleRegister, &blurScale[0], 1);
    }
    m_engine->m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    m_engine->m_device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    m_engine->m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_engine->m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_engine->m_device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    m_engine->m_device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    m_engine->drawSurfaceRectangle(Vector<2, float>(x, y), Vector<2, float>(width, height), Vector<2, float>(s, t),
                                   Vector<2, float>(textureWidth, textureHeight),
                                   Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    m_engine->m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_engine->m_device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
    m_engine->m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    m_engine->loadMatrix(saved);
}

void D3D9RenderSurface::drawDownsampled(const Vector<2, float> &position, const Vector<2, float> &size,
                                        const Vector<2, float> &texturePosition, float scale, int factor)
{
    Matrix<float> saved;
    m_engine->getModelViewMatrix(saved);
    Matrix<float> identity;
    identity.identity();
    m_engine->loadMatrix(identity);
    float x = position[0];
    float y = position[1];
    float width = size[0];
    float height = size[1];
    float s = texturePosition[0] / m_width;
    float t = texturePosition[1] / m_height;
    float textureWidth = width * scale / m_width;
    float textureHeight = height * scale / m_height;
    m_engine->m_device->SetTexture(0, m_colorTexture);
    m_engine->m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    m_engine->m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    m_engine->m_device->SetVertexDeclaration(m_engine->m_shapeDeclaration);
    m_engine->m_device->SetVertexShader(m_engine->m_surfaceVertexShader);
    switch (factor) {
    case 1:
        m_engine->m_device->SetPixelShader(m_engine->m_surfacePixelShader);
        break;
    case 2:
        m_engine->m_device->SetPixelShader(m_engine->m_downsample2PixelShader);
        // The code reads the register into the vector (Get, not Set), so the downsampling shader never
        // receives the texel size.
        m_engine->m_device->GetPixelShaderConstantF(
            m_engine->m_downsample2ScaleRegister, &Vector<4, float>(1.0f / m_width, 1.0f / m_height, 0.0f, 0.0f)[0], 1);
        break;
    case 3:
        m_engine->m_device->SetPixelShader(m_engine->m_downsample3PixelShader);
        m_engine->m_device->GetPixelShaderConstantF(
            m_engine->m_downsample3ScaleRegister, &Vector<4, float>(1.0f / m_width, 1.0f / m_height, 0.0f, 0.0f)[0], 1);
        break;
    case 4:
        m_engine->m_device->SetPixelShader(m_engine->m_downsample4PixelShader);
        m_engine->m_device->GetPixelShaderConstantF(
            m_engine->m_downsample4ScaleRegister, &Vector<4, float>(1.0f / m_width, 1.0f / m_height, 0.0f, 0.0f)[0], 1);
        break;
    }
    m_engine->drawSurfaceRectangle(
        Vector<2, float>(static_cast<float>(static_cast<int>(x)), static_cast<float>(static_cast<int>(y))),
        Vector<2, float>(static_cast<float>(static_cast<int>(width)), static_cast<float>(static_cast<int>(height))),
        Vector<2, float>(s, t), Vector<2, float>(textureWidth, textureHeight), m_drawColor);
    m_engine->loadMatrix(saved);
}

void D3D9RenderSurface::begin()
{
    m_engine->m_device->GetRenderTarget(0, &m_previousColorSurface);
    m_engine->m_device->GetDepthStencilSurface(&m_previousDepthSurface);
    if (m_colorSurface) {
        m_engine->m_device->SetRenderTarget(0, m_colorSurface);
        m_engine->m_device->SetDepthStencilSurface(m_depthSurface);
    }
}

void D3D9RenderSurface::releaseDeviceResources()
{
    if (m_depthSurface) {
        m_depthSurface->Release();
        m_depthSurface = 0;
    }
    if (m_colorSurface) {
        m_colorSurface->Release();
        m_colorSurface = 0;
    }
    if (m_colorTexture) {
        m_colorTexture->Release();
        m_colorTexture = 0;
    }
}

bool D3D9RenderSurface::resize(int width, int height)
{
    if (m_colorTexture && width == m_width && height == m_height)
        return false;
    if (m_colorSurface) {
        m_colorSurface->Release();
        m_colorSurface = 0;
    }
    if (m_colorTexture) {
        m_colorTexture->Release();
        m_colorTexture = 0;
    }
    if (m_depthSurface) {
        m_depthSurface->Release();
        m_depthSurface = 0;
    }
    if (!m_colorTexture)
        m_engine->m_device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
                                          &m_colorTexture, 0);
    m_colorTexture->GetSurfaceLevel(0, &m_colorSurface);
    m_engine->m_device->CreateDepthStencilSurface(width, height, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE,
                                                  &m_depthSurface, 0);
    m_width = width;
    m_height = height;
    return true;
}

}
