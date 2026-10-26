// Quad-mesh subdivision records: QuadVertex, QuadEdge and QuadFace.
#pragma once
#include <windows.h>
#include "Concurrency.h"
#include "plasma/Vector.h"

namespace plasma {

// QuadVertex::m_flags, set up per subdivision level (SmoothMeshShape's UpdateSubdivisionDrawings, QuadMesh): the
// boundary bit from the topology, the others from the control vertex's MeshShape corner kind.
enum QuadVertexFlags {
    QUAD_BOUNDARY = 0,
    QUAD_SHARP = 1,
    QUAD_BEZIER = 2,
    QUAD_ROUND = 3
};

enum QuadVertexMasks {
    QUAD_FLAGS_NONE = 0
};

struct QuadVertex {
    // Default construction leaves the record's values indeterminate.
    QuadVertex() {}

    Vector<2, float> *m_position;
    Vector<4, float> *m_color;
    Vector<2, float> *m_texturePosition;
    Vector<2, float> m_positionSum;
    Vector<4, float> m_colorSum;
    Vector<2, float> m_textureSum;
    int m_count;           // Index during topology setup.
    unsigned char m_flags; // QuadVertexFlags.
};

struct QuadEdge;

struct QuadFace {
    // Default construction leaves the record's values indeterminate.
    QuadFace() {}

    QuadVertex *m_vertices[4];
    QuadVertex m_center;
    QuadEdge *m_edges[4];
    QuadFace *m_children[4];
    int m_indices[4];
    QuadFace *m_parent; // Original control face.

    // Zero on the control faces and copied to every child (QuadMesh::Subdivide); nothing reads it.
    int m_unused78;
};

struct QuadEdge {
    // Default construction leaves the record's values indeterminate.
    QuadEdge() {}

    QuadFace *m_faces[2];
    QuadVertex *m_vertices[2];
    QuadVertex m_center;
    QuadEdge *m_children[2];
};

class QuadMesh {
public:
    void subdivide(QuadMesh &source);
    void prepare();
    void clear();
    void update(float weight);

    Concurrency::concurrent_vector<QuadFace> m_faces;
    Concurrency::concurrent_vector<QuadEdge> m_edges;
    Concurrency::concurrent_vector<QuadVertex *> m_vertices;
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif

}
