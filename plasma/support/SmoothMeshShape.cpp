// Smooth-mesh support for subdivision positions, reset behavior and vertex record allocation.
#include "EditableMesh.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/Engine.h"
#include "plasma/Texture.h"
#include "Concurrency.h"

namespace plasma {

const Vector<2, float> *SmoothMeshShape::getSubdivisionPosition(int vertex)
{
    return m_vertices[vertex].m_position;
}

void smoothDrawingReset(Drawing *) {}

extern "C" void StandIn_SmoothMeshVertexCopy(QuadVertex *destination, QuadVertex *source)
{
    // The concurrent_vector growth helpers copy-construct the vertex records.
    std::allocator<QuadVertex> allocator;
    allocator.construct(destination, *source);
}

}

#pragma comment(linker, "/INCLUDE:_StandIn_SmoothMeshVertexCopy")
