// ChunkBuffer owns a mesh slice's GPU buffers. Chunk groups those slices with their bounds, lights and pending
// replacement buffers for a terrain zone.
#pragma once
#include <array>
#include <list>
#include "plasma/Vector.h"
#include "cube/Fixed.h"
#include "gfx/RendererIds.h"

namespace gfx {

class IndexBuffer;
class VertexBuffer;

struct ChunkBuffer {
    ChunkBuffer();
    virtual ~ChunkBuffer();

    VertexBuffer *vertexBuffer;                // created by Graphics; deleted by the destructor
    IndexBuffer *indexBuffers[ChunkPassCount]; // indexed by ChunkPass (gfx/RendererIds.h)
    int vertexCount;                           // vertices / 8 bytes; Graphics::DrawIndexed's first argument
    int triangleCounts[ChunkPassCount];        // Triangle counts for each index buffer (index count / 3).
    // Zeroed by the constructor.
    int baseZ;
};

struct Chunk {
    Chunk();
    virtual ~Chunk();

    std::list<ChunkBuffer *> chunkBuffers;    // drawn by the renderer
    std::list<ChunkBuffer *> newChunkBuffers; // filled by updateChunk, moved to chunkBuffers by Game::ChunkThread

    plasma::Vector<3, cube::Fixed> boundsMin; // zone x, y and the lowest z, << 16
    plasma::Vector<3, cube::Fixed> boundsMax; // x +, y + and the highest z + 1 (at least 0), << 16
    plasma::Vector<3, cube::Fixed> center;    // (boundsMin + boundsMax) / 2
    float radius;                             // |boundsMax - center| in blocks, times 1.5
    float squaredCameraDistance;

    int remeshTime;
    bool needsRemesh; // set with remeshTime; cleared by updateChunk

    std::array<plasma::Vector<4, float>, 16> lightPositions;
    std::array<plasma::Vector<4, float>, 16> lightColors;
    int lightCount; // compared with 16, incremented per light
    int triangleCount;
};

}
