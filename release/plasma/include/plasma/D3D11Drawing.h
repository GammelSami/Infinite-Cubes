// D3D11Drawing keeps tessellated triangles in Direct3D 11 vertex and 32-bit index buffers. It uses the engine's shape
// shader and registers itself with the engine.
#pragma once
#include "plasma/Drawing.h"

struct ID3D11Buffer;

namespace plasma {

class D3D11Engine;

class D3D11Drawing : public Drawing {
public:
    explicit D3D11Drawing(D3D11Engine *engine);
    virtual ~D3D11Drawing();

    // Drawing::update, then rebuilds both buffers from the path arrays (ID3D11Device::CreateBuffer with
    // D3D11_BIND_VERTEX_BUFFER and D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_DEFAULT, initial data).
    virtual void update();
    // Does nothing without both buffers and a non-zero index count.
    virtual void draw(Node *node, const Vector<4, float> &color);

protected:
    struct Vertex {
        Vector<2, float> m_position;          // from m_pathVertices
        Vector<2, float> m_textureCoordinate; // from m_pathTextureCoordinates
        Vector<4, float> m_color;             // from m_pathColors
    };

    D3D11Engine *m_engine;
    ID3D11Buffer *m_vertexBuffer;
    ID3D11Buffer *m_indexBuffer; // created only when m_pathIndices is not empty
    unsigned m_vertexCount;      // written by Update; no reader found
    unsigned m_indexCount;       // written by Update, read by Draw
};

}
