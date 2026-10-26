// Direct3D 11 drawing uses an interleaved vertex buffer and the engine's constant-buffer setters for shader
// parameters.
#include <d3d11.h>
#include <vector>
#include "plasma/D3D11Drawing.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Node.h"
#include "plasma/RenderSurface.h"
#include "plasma/Shape.h"
#include "plasma/Texture.h"
#include "plasma/Widget.h"
#include "plasma/EditFilter.h"

namespace plasma {

// Local drawing helper.
int g_drawingLayer;

D3D11Drawing::D3D11Drawing(D3D11Engine *engine)
    : m_engine(engine),
      m_vertexBuffer(nullptr),
      m_indexBuffer(nullptr)
{
    m_engine->m_drawings.push_back(this);
}

D3D11Drawing::~D3D11Drawing()
{
    if (m_vertexBuffer)
        m_vertexBuffer->Release();
    if (m_indexBuffer)
        m_indexBuffer->Release();
    // The engine's own destructor deletes its objects; its list is going away then.
    if (!m_engine->isDestroying())
        m_engine->m_drawings.remove(this);
}

void D3D11Drawing::draw(Node *node, const Vector<4, float> &color)
{
    if (!m_vertexBuffer || !m_indexBuffer || !m_indexCount)
        return;
    m_engine->applyRenderState();
    if (m_engine->m_filterSurface) {
        if (!node || !(node->m_display->m_flags & (1 << DISPLAY_BLUR)) ||
            node->m_display->m_clipping.value() != DISPLAY_CLIPPING_DISABLED)
            m_engine->m_filterSurface->bindTexture();
    }
    m_engine->m_modelViewStack.push_back(m_engine->m_modelView);
    if (node && m_engine->usesDepthLayers()) {
        // Every drawing of a node lies two units nearer than the one before.
        m_engine->m_modelView.translate(0.0f, 0.0f, g_drawingLayer * -2.0f);
        ++g_drawingLayer;
    }
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_engine->m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_engine->m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_engine->setProjectionMatrix(m_engine->m_projection);
    m_engine->setModelViewMatrix(m_engine->m_modelView);
    // A deformable shape inside a widget whose visible extent differs from its bind size is drawn deformed by the
    // vertex shader: it gets the matrices between the node's and the widget's bind space and the bind and deformed
    // rectangles.
    bool deformed = false;
    if (node) {
        // The node's fill colour modulates the colour asked for.
        m_engine->setColor(node->m_display->m_fillColor.value() * color);
        if (node->m_enclosingWidget && node->m_shape && node->m_shape->isDeformable()) {
            Widget *widget = node->m_enclosingWidget;
            if (widget->getClampedVisibleExtent() != widget->getInnerBindSize()) {
                Matrix<float> inverseMatrix;
                node->getInverseMatrix(inverseMatrix);
                Matrix<float> matrix;
                node->getMatrix(matrix);
                Matrix<float> widgetInverseMatrix;
                node->m_enclosingWidget->m_node->getInverseMatrix(widgetInverseMatrix);
                widget = node->m_enclosingWidget;
                Matrix<float> widgetMatrix;
                widget->m_node->getMatrix(widgetMatrix);
                Matrix<float> bindMatrix = widget->m_bindMatrix * widgetInverseMatrix * matrix;
                Matrix<float> inverseBindMatrix = inverseMatrix * widgetMatrix * widget->m_inverseBindMatrix;
                m_engine->setWidgetBindMatrix(bindMatrix);
                m_engine->setInverseWidgetBindMatrix(inverseBindMatrix);
                m_engine->setWidgetDeformationEnabled(true);
                m_engine->setWidgetBindPos(node->m_enclosingWidget->getInnerBindPos());
                m_engine->setWidgetBindSize(node->m_enclosingWidget->getInnerBindSize());
                m_engine->setDeformedWidgetPos(node->m_enclosingWidget->getScrollPosition());
                m_engine->setDeformedWidgetSize(node->m_enclosingWidget->getClampedVisibleExtent());
                deformed = true;
            }
        }
    } else {
        m_engine->setColor(color);
    }
    if (!deformed)
        m_engine->setWidgetDeformationEnabled(false);
    if (m_texture) {
        m_texture->bind(1);
        m_engine->setTextureMatrix(m_textureMatrix);
    }
    m_engine->setTextureEnabled(m_texture != nullptr);
    // The pixel shader's filter: none unless the node is clipped by the filter surface; then modulation, screen or
    // plain masking.
    int filter = FILTER_NONE;
    if (node && node->m_display->m_clipping.value() != DISPLAY_CLIPPING_DISABLED) {
        if (node->m_display->m_flags & (1 << DISPLAY_MODULATION))
            filter = FILTER_MODULATION;
        else if (node->m_display->m_flags & (1 << DISPLAY_SCREEN))
            filter = FILTER_SCREEN;
        else
            filter = FILTER_CLIP;
    }
    m_engine->setFilter(filter);
    m_engine->setTextureAdjustments(m_textureOpacity, m_textureBrightness, m_textureContrast, m_textureSaturation);
    D3D11Engine *engine = m_engine;
    Matrix<float> pixelScaled;
    pixelScaled.identity();
    pixelScaled.scale(1.0f / engine->m_viewportWidth, 1.0f / engine->m_viewportHeight);
    pixelScaled *= engine->m_modelView;
    engine->setPixelScaledModelViewMatrix(pixelScaled);
    m_engine->m_context->DrawIndexed(m_indexCount, 0, 0);
    m_engine->m_modelView = m_engine->m_modelViewStack.back();
    m_engine->m_modelViewStack.pop_back();
}

void D3D11Drawing::update()
{
    Drawing::update();
    if (m_dirty & ((1 << DRAWING_DIRTY_VERTICES) | (1 << DRAWING_DIRTY_INDICES)))
        m_dirty |= DRAWING_DIRTY_ALL;
    if (!m_pathVertices.empty()) {
        std::vector<Vertex> vertices(m_pathVertices.size());
        for (int i = 0; i < (int)m_pathVertices.size(); i++) {
            Vertex vertex;
            vertex.m_position = m_pathVertices[i];
            vertex.m_textureCoordinate = m_pathTextureCoordinates[i];
            vertex.m_color = m_pathColors[i];
            vertices[i] = vertex;
        }
        if (m_vertexBuffer) {
            m_vertexBuffer->Release();
            m_vertexBuffer = nullptr;
        }
        if (m_indexBuffer) {
            m_indexBuffer->Release();
            m_indexBuffer = nullptr;
        }
        if (!m_vertexBuffer) {
            m_vertexCount = vertices.size();
            D3D11_BUFFER_DESC description = {};
            description.Usage = D3D11_USAGE_DEFAULT;
            description.ByteWidth = sizeof(Vertex) * m_vertexCount;
            description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            description.CPUAccessFlags = 0;
            description.MiscFlags = 0;
            D3D11_SUBRESOURCE_DATA data = {};
            data.pSysMem = &vertices[0];
            m_engine->m_device->CreateBuffer(&description, &data, &m_vertexBuffer);
        }
        if (!m_pathIndices.empty() && !m_indexBuffer) {
            D3D11_BUFFER_DESC description = {};
            description.Usage = D3D11_USAGE_DEFAULT;
            description.ByteWidth = sizeof(unsigned int) * m_pathIndices.size();
            description.BindFlags = D3D11_BIND_INDEX_BUFFER;
            description.CPUAccessFlags = 0;
            description.MiscFlags = 0;
            D3D11_SUBRESOURCE_DATA data = {};
            data.pSysMem = &m_pathIndices[0];
            m_engine->m_device->CreateBuffer(&description, &data, &m_indexBuffer);
            m_indexCount = m_pathIndices.size();
        }
    }
}

}
