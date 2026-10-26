#include <d3d11.h>
#include "gfx/D3D11Graphics.h"

namespace gfx {

D3D11Graphics::D3D11Graphics(ID3D11Device *device, ID3D11DeviceContext *context)
    : m_device(device),
      m_context(context)
{}

D3D11IndexBuffer::~D3D11IndexBuffer()
{
    m_buffer->Release();
}

D3D11VertexBuffer::~D3D11VertexBuffer()
{
    m_buffer->Release();
}

bool D3D11IndexBuffer::bind()
{
    m_context->IASetIndexBuffer(m_buffer, (DXGI_FORMAT)m_format, 0);
    return true;
}

bool D3D11VertexBuffer::bind()
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_buffer, &stride, &offset);
    return true;
}

IndexBuffer *D3D11Graphics::createIndexBuffer16(const std::vector<unsigned short> &indices)
{
    D3D11_BUFFER_DESC description;
    ZeroMemory(&description, sizeof(description));
    description.Usage = D3D11_USAGE_DEFAULT;
    description.ByteWidth = sizeof(unsigned short) * (UINT)indices.size();
    description.BindFlags = D3D11_BIND_INDEX_BUFFER;
    description.CPUAccessFlags = 0;
    description.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA data;
    ZeroMemory(&data, sizeof(data));
    data.pSysMem = &indices[0];
    ID3D11Buffer *buffer;
    m_device->CreateBuffer(&description, &data, &buffer);
    return new D3D11IndexBuffer(m_context, buffer, DXGI_FORMAT_R16_UINT);
}

IndexBuffer *D3D11Graphics::createIndexBuffer32(const std::vector<unsigned int> &indices)
{
    D3D11_BUFFER_DESC description;
    ZeroMemory(&description, sizeof(description));
    description.Usage = D3D11_USAGE_DEFAULT;
    description.ByteWidth = sizeof(unsigned int) * (UINT)indices.size();
    description.BindFlags = D3D11_BIND_INDEX_BUFFER;
    description.CPUAccessFlags = 0;
    description.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA data;
    ZeroMemory(&data, sizeof(data));
    data.pSysMem = &indices[0];
    ID3D11Buffer *buffer;
    m_device->CreateBuffer(&description, &data, &buffer);
    return new D3D11IndexBuffer(m_context, buffer, DXGI_FORMAT_R32_UINT);
}

VertexBuffer *D3D11Graphics::createVertexBuffer(const std::vector<Vertex> &vertices)
{
    D3D11_BUFFER_DESC description;
    ZeroMemory(&description, sizeof(description));
    description.Usage = D3D11_USAGE_DEFAULT;
    description.ByteWidth = sizeof(Vertex) * (UINT)vertices.size();
    description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    description.CPUAccessFlags = 0;
    description.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA data;
    ZeroMemory(&data, sizeof(data));
    data.pSysMem = &vertices[0];
    ID3D11Buffer *buffer;
    m_device->CreateBuffer(&description, &data, &buffer);
    return new D3D11VertexBuffer(m_context, buffer);
}

void D3D11Graphics::drawIndexed(int vertexCount, int triangleCount)
{
    m_context->DrawIndexed(3 * triangleCount, 0, 0);
}

}
