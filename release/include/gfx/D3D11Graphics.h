// D3D11Graphics creates GPU vertex and index buffers with D3D11_USAGE_DEFAULT and no CPU access, then draws through
// the immediate context. Its buffer classes own their device buffers and retain the shared context.
#pragma once
#include <vector>
#include "gfx/Graphics.h"
#include "gfx/IndexBuffer.h"
#include "gfx/VertexBuffer.h"

// Direct3D 11 interfaces (d3d11.h); only pointers are held.
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;

namespace gfx {

class D3D11VertexBuffer : public VertexBuffer {
public:
    D3D11VertexBuffer(ID3D11DeviceContext *context, ID3D11Buffer *buffer)
        : m_context(context),
          m_buffer(buffer)
    {}
    // Releases m_buffer (no null test), then VertexBuffer's (trivial) destructor.
    virtual ~D3D11VertexBuffer();

    virtual bool bind();

protected:
    ID3D11DeviceContext *m_context; // D3D11Graphics::m_context, not released
    ID3D11Buffer *m_buffer;         // created by the factory, released by the destructor
};

class D3D11IndexBuffer : public IndexBuffer {
public:
    D3D11IndexBuffer(ID3D11DeviceContext *context, ID3D11Buffer *buffer, int format)
        : m_context(context),
          m_buffer(buffer),
          m_format(format)
    {}
    // Releases m_buffer (no null test).
    virtual ~D3D11IndexBuffer();

    virtual bool bind();

protected:
    ID3D11DeviceContext *m_context; // D3D11Graphics::m_context, not released
    ID3D11Buffer *m_buffer;         // released by the destructor
    int m_format;
};

class D3D11Graphics : public Graphics {
public:
    D3D11Graphics(ID3D11Device *device, ID3D11DeviceContext *context);

    // The CreateBuffer result is not checked.
    virtual VertexBuffer *createVertexBuffer(const std::vector<Vertex> &vertices);
    virtual IndexBuffer *createIndexBuffer32(const std::vector<unsigned int> &indices);
    virtual IndexBuffer *createIndexBuffer16(const std::vector<unsigned short> &indices);
    virtual void drawIndexed(int vertexCount, int triangleCount);

protected:
    ID3D11Device *m_device;
    ID3D11DeviceContext *m_context; // constructor argument (the immediate context)
};

}
