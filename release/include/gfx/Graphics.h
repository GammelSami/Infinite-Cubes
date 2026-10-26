// gfx::Graphics: the game's API-neutral factory for vertex and index buffers and their draw call.
#pragma once
#include <vector>
#include "gfx/VertexBuffer.h"
#include "gfx/IndexBuffer.h"

namespace gfx {

class Graphics {
public:
    virtual ~Graphics() {}

    virtual VertexBuffer *createVertexBuffer(const std::vector<Vertex> &vertices) { return new VertexBuffer; }
    virtual IndexBuffer *createIndexBuffer32(const std::vector<unsigned int> &indices) { return new IndexBuffer; }
    virtual IndexBuffer *createIndexBuffer16(const std::vector<unsigned short> &indices) { return new IndexBuffer; }
    virtual void drawIndexed(int vertexCount, int triangleCount) {}
};

}
