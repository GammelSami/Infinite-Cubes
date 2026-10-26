#include "gfx/Chunk.h"
#include "gfx/IndexBuffer.h"
#include "gfx/VertexBuffer.h"

namespace gfx {

Chunk::Chunk()
    : remeshTime(0),
      needsRemesh(false),
      lightCount(0),
      triangleCount(0)
{}

ChunkBuffer::ChunkBuffer()
{
    vertexBuffer = nullptr;
    indexBuffers[ChunkPassSolid] = nullptr;
    indexBuffers[ChunkPassWater] = nullptr;
    indexBuffers[ChunkPass_2] = nullptr;
    indexBuffers[ChunkPassLava] = nullptr;
    indexBuffers[ChunkPass_4] = nullptr;
    indexBuffers[ChunkPass_5] = nullptr;
    baseZ = 0;
}

Chunk::~Chunk()
{
    for (ChunkBuffer *buffer : chunkBuffers)
        delete buffer;
    for (ChunkBuffer *buffer : newChunkBuffers)
        delete buffer;
}

ChunkBuffer::~ChunkBuffer()
{
    delete vertexBuffer;
    delete indexBuffers[ChunkPassSolid];
    delete indexBuffers[ChunkPassWater];
    delete indexBuffers[ChunkPass_2];
    delete indexBuffers[ChunkPassLava];
    delete indexBuffers[ChunkPass_4];
    delete indexBuffers[ChunkPass_5];
}

}
