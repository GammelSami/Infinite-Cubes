#pragma once
#include <d3d9.h>
#include "plasma/Drawing.h"

namespace plasma {
class D3D9Engine;

class D3D9Drawing : public Drawing {
public:
    explicit D3D9Drawing(D3D9Engine *engine);
    virtual ~D3D9Drawing();
    virtual void update();
    virtual void draw(Node *node, const Vector<4, float> &color);
    void releaseDeviceResources();
    void restoreDeviceResources();

private:
    D3D9Engine *m_engine;
    IDirect3DVertexBuffer9 *m_vertexBuffer;
    IDirect3DIndexBuffer9 *m_indexBuffer;
    unsigned char *m_vertexData; // interleaved vertex bytes, allocated with new[]
    unsigned char *m_indexData;
    int m_vertexCount;
    int m_indexCount;
};
}
