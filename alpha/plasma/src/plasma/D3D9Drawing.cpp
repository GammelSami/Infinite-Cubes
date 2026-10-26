#include "plasma/D3D9Drawing.h"
#include "plasma/D3D9Engine.h"
#include "plasma/D3D9Vertex.h"
#include "plasma/Display.h"
#include "plasma/Node.h"
#include "plasma/Shape.h"
#include "plasma/Texture.h"
#include "plasma/RenderSurface.h"
#include "plasma/Widget.h"
#include <cstring>

namespace plasma {

D3D9Drawing::D3D9Drawing(D3D9Engine *engine)
    : m_engine(engine),
      m_vertexBuffer(0),
      m_indexBuffer(0),
      m_vertexData(0),
      m_indexData(0)
{
    m_engine->m_drawings.push_back(this);
}

D3D9Drawing::~D3D9Drawing()
{
    if (m_vertexBuffer)
        m_vertexBuffer->Release();
    if (m_indexBuffer)
        m_indexBuffer->Release();
    if (m_vertexData)
        delete[] m_vertexData;
    if (m_indexData)
        delete[] m_indexData;
    if (!m_engine->isDestroying())
        m_engine->m_drawings.remove(this);
}

void D3D9Drawing::releaseDeviceResources()
{
    if (m_vertexBuffer) {
        m_vertexBuffer->Release();
        m_vertexBuffer = 0;
    }
    if (m_indexBuffer) {
        m_indexBuffer->Release();
        m_indexBuffer = 0;
    }
}

void D3D9Drawing::draw(Node *node, const Vector<4, float> &color)
{
    if (!m_vertexBuffer)
        return;
    if (m_engine->m_filterSurface && (!node || !(node->m_display->m_flags & (1 << DISPLAY_BLUR)) ||
                                      node->m_display->m_clipping.value() != DISPLAY_CLIPPING_DISABLED))
        m_engine->m_filterSurface->bindTexture();
    else
        m_engine->m_device->SetTexture(0, 0);
    m_engine->m_modelViewStack.push_back(m_engine->m_modelViewMatrix);
    if (node && m_engine->usesDepthLayers()) {
        m_engine->m_modelViewMatrix.translate(0.0f, 0.0f, g_drawingLayer * -2.0f);
        m_engine->m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
        m_engine->m_device->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATER);
        ++g_drawingLayer;
    } else {
        m_engine->m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
    }
    m_engine->m_device->SetStreamSource(0, m_vertexBuffer, 0, sizeof(D3D9Vertex));
    m_engine->m_device->SetVertexDeclaration(m_engine->m_shapeDeclaration);
    m_engine->m_device->SetVertexShader(m_engine->m_shapeVertexShader);
    m_engine->m_device->SetPixelShader(m_engine->m_shapePixelShader);
    m_engine->setVertexShaderMatrix(m_engine->m_projectionRegister, m_engine->m_projectionMatrix, 4);
    m_engine->setVertexShaderMatrix(m_engine->m_modelViewRegister, m_engine->m_modelViewMatrix, 4);
    Matrix<float> normalMatrix = m_engine->m_modelViewMatrix;
    normalMatrix.invert();
    normalMatrix.transpose();
    m_engine->setVertexShaderMatrix(m_engine->m_normalMatrixRegister, normalMatrix, 2);
    m_engine->m_device->SetVertexShaderConstantF(
        m_engine->m_antialiasRegister,
        Vector<4, float>((m_engine->m_renderFlags & (1 << RENDER_ANTIALIAS)) ? m_engine->m_antialiasStrength : 0.0f,
                         0.0f, 0.0f, 0.0f)
            .m_elements,
        1);
    bool deformation = false;
    if (node) {
        Vector<4, float> combined = node->m_display->m_fillColor.value();
        combined *= color;
        m_engine->m_device->SetVertexShaderConstantF(m_engine->m_baseColorRegister, combined.m_elements, 1);
        if (node->m_enclosingWidget && node->m_shape && node->m_shape->isDeformable() &&
            node->m_enclosingWidget->getClampedVisibleExtent() != node->m_enclosingWidget->getInnerBindSize()) {
            Widget *widget = node->m_enclosingWidget;
            Matrix<float> bind = widget->m_bindMatrix * widget->m_node->m_inverseMatrix * node->m_matrix;
            Matrix<float> inverseBind = node->m_inverseMatrix * widget->m_node->m_matrix * widget->m_inverseBindMatrix;
            m_engine->setVertexShaderMatrix(m_engine->m_widgetBindMatrixRegister, bind, 4);
            m_engine->setVertexShaderMatrix(m_engine->m_inverseWidgetBindMatrixRegister, inverseBind, 4);
            BOOL enabled = TRUE;
            m_engine->m_device->SetVertexShaderConstantB(m_engine->m_widgetDeformationRegister, &enabled, 1);
            m_engine->m_device->SetVertexShaderConstantF(m_engine->m_widgetBindPositionRegister,
                                                         Vector<4, float>(node->m_enclosingWidget->getInnerBindPos()[0],
                                                                          node->m_enclosingWidget->getInnerBindPos()[1],
                                                                          0.0f, 0.0f)
                                                             .m_elements,
                                                         1);
            m_engine->m_device->SetVertexShaderConstantF(
                m_engine->m_widgetBindSizeRegister,
                Vector<4, float>(node->m_enclosingWidget->getInnerBindSize()[0],
                                 node->m_enclosingWidget->getInnerBindSize()[1], 0.0f, 0.0f)
                    .m_elements,
                1);
            m_engine->m_device->SetVertexShaderConstantF(
                m_engine->m_deformedWidgetPositionRegister,
                Vector<4, float>(node->m_enclosingWidget->getScrollPosition()[0],
                                 node->m_enclosingWidget->getScrollPosition()[1], 0.0f, 0.0f)
                    .m_elements,
                1);
            m_engine->m_device->SetVertexShaderConstantF(
                m_engine->m_deformedWidgetSizeRegister,
                Vector<4, float>(node->m_enclosingWidget->getClampedVisibleExtent()[0],
                                 node->m_enclosingWidget->getClampedVisibleExtent()[1], 0.0f, 0.0f)
                    .m_elements,
                1);
            deformation = true;
        }
    } else {
        m_engine->m_device->SetVertexShaderConstantF(m_engine->m_baseColorRegister, color.m_elements, 1);
    }
    if (!deformation) {
        BOOL enabled = FALSE;
        m_engine->m_device->SetVertexShaderConstantB(m_engine->m_widgetDeformationRegister, &enabled, 1);
    }
    if (m_texture) {
        m_texture->bind(1);
        m_engine->setVertexShaderMatrix(m_engine->m_textureMatrixRegister, m_textureMatrix, 2);
    }
    BOOL textureEnabled = m_texture != 0;
    m_engine->m_device->SetPixelShaderConstantB(m_engine->m_textureEnabledRegister, &textureEnabled, 1);
    int filter = D3D9_FILTER_NONE;
    if (node && node->m_display->m_clipping.value() != DISPLAY_CLIPPING_DISABLED) {
        if (node->m_display->m_flags & (1 << DISPLAY_MODULATION))
            filter = D3D9_FILTER_MODULATION;
        else if (node->m_display->m_flags & (1 << DISPLAY_SCREEN))
            filter = D3D9_FILTER_SCREEN;
        else
            filter = D3D9_FILTER_CLIP;
    }
    m_engine->m_device->SetPixelShaderConstantF(
        m_engine->m_filterRegister, Vector<4, float>(static_cast<float>(filter), 0.0f, 0.0f, 0.0f).m_elements, 1);
    m_engine->m_device->SetPixelShaderConstantF(m_engine->m_textureOpacityRegister,
                                                Vector<4, float>(m_textureOpacity, 0.0f, 0.0f, 0.0f).m_elements, 1);
    m_engine->m_device->SetPixelShaderConstantF(m_engine->m_textureBrightnessRegister,
                                                Vector<4, float>(m_textureBrightness, 0.0f, 0.0f, 0.0f).m_elements, 1);
    m_engine->m_device->SetPixelShaderConstantF(m_engine->m_textureContrastRegister,
                                                Vector<4, float>(m_textureContrast, 0.0f, 0.0f, 0.0f).m_elements, 1);
    m_engine->m_device->SetPixelShaderConstantF(m_engine->m_textureSaturationRegister,
                                                Vector<4, float>(m_textureSaturation, 0.0f, 0.0f, 0.0f).m_elements, 1);
    Matrix<float> mask;
    mask.identity();
    mask.scale(1.0f / m_engine->m_viewportWidth, 1.0f / m_engine->m_viewportHeight);
    mask *= m_engine->m_modelViewMatrix;
    m_engine->setVertexShaderMatrix(m_engine->m_maskMatrixRegister, mask, 2);
    m_engine->m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
    if (m_indexBuffer) {
        m_engine->m_device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
        m_engine->m_device->SetIndices(m_indexBuffer);
        m_engine->m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, (UINT)m_vertices.size(), 0,
                                                 (UINT)(m_indices.size() / 3));
        m_engine->m_device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    }
    m_engine->m_modelViewMatrix = m_engine->m_modelViewStack.back();
    m_engine->m_modelViewStack.pop_back();
    m_engine->m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
}

void D3D9Drawing::restoreDeviceResources()
{
    if (m_vertexData && !m_vertexBuffer) {
        m_engine->m_device->CreateVertexBuffer((UINT)(m_vertices.size() * sizeof(D3D9Vertex)),
                                               D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT,
                                               &m_vertexBuffer, 0);
        void *vertexBytes;
        m_vertexBuffer->Lock(0, (UINT)(m_vertices.size() * sizeof(D3D9Vertex)), &vertexBytes, D3DLOCK_DISCARD);
        memcpy(vertexBytes, m_vertexData, m_vertices.size() * sizeof(D3D9Vertex));
        m_vertexBuffer->Unlock();
    }
    if (m_indexData && !m_indexBuffer) {
        m_engine->m_device->CreateIndexBuffer((UINT)(m_indices.size() * sizeof(int)),
                                              D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_DEFAULT,
                                              &m_indexBuffer, 0);
        void *indexBytes;
        m_indexBuffer->Lock(0, (UINT)(m_indices.size() * sizeof(int)), &indexBytes, D3DLOCK_DISCARD);
        memcpy(indexBytes, m_indexData, m_indices.size() * sizeof(int));
        m_indexBuffer->Unlock();
    }
}

void D3D9Drawing::update()
{
    Drawing::update();
    if (m_dirty & ((1 << DRAWING_DIRTY_VERTICES) | (1 << DRAWING_DIRTY_INDICES)))
        m_dirty |= DRAWING_DIRTY_ALL;
    if (!m_pathVertices.empty()) {
        buildBuffers();
        if (m_vertexBuffer && m_vertexCount != static_cast<int>(m_vertices.size())) {
            m_vertexBuffer->Release();
            m_vertexBuffer = 0;
        }
        if (m_indexBuffer && m_indexCount != static_cast<int>(m_indices.size())) {
            m_indexBuffer->Release();
            m_indexBuffer = 0;
        }
        if (!m_vertexBuffer) {
            m_vertexCount = (int)m_vertices.size();
            m_engine->m_device->CreateVertexBuffer((UINT)(m_vertices.size() * sizeof(D3D9Vertex)),
                                                   D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT,
                                                   &m_vertexBuffer, 0);
        }
        if (m_vertexData) {
            delete[] m_vertexData;
            m_vertexData = 0;
        }
        if (m_indexData) {
            delete[] m_indexData;
            m_indexData = 0;
        }
        D3D9Vertex *vertices;
        m_vertexBuffer->Lock(0, (UINT)(m_vertices.size() * sizeof(D3D9Vertex)), reinterpret_cast<void **>(&vertices),
                             D3DLOCK_DISCARD);
        for (int i = 0; i < static_cast<int>(m_vertices.size()); ++i) {
            vertices[i].position = m_vertices[i];
            vertices[i].color = m_colors[i];
            vertices[i].firstNormal = Vector<2, float>(m_normals[i][0], m_normals[i][1]);
            vertices[i].secondNormal = Vector<2, float>(m_normals[i][2], m_normals[i][3]);
            vertices[i].texture = m_textureCoordinates[i];
        }
        m_vertexData = new unsigned char[m_vertices.size() * sizeof(D3D9Vertex)];
        memcpy(m_vertexData, vertices, m_vertices.size() * sizeof(D3D9Vertex));
        m_vertexBuffer->Unlock();
        if (!m_indices.empty()) {
            if (!m_indexBuffer) {
                m_engine->m_device->CreateIndexBuffer((UINT)(m_indices.size() * sizeof(int)),
                                                      D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX32,
                                                      D3DPOOL_DEFAULT, &m_indexBuffer, 0);
                m_indexCount = (int)m_indices.size();
            }
            void *indices;
            m_indexBuffer->Lock(0, (UINT)(m_indices.size() * sizeof(int)), &indices, D3DLOCK_DISCARD);
            memcpy(indices, &m_indices[0], m_indices.size() * sizeof(int));
            m_indexData = new unsigned char[m_indices.size() * sizeof(int)];
            memcpy(m_indexData, &m_indices[0], m_indices.size() * sizeof(int));
            m_indexBuffer->Unlock();
        }
    }
}

}
