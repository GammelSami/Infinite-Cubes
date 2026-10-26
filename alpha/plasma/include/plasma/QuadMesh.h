// QuadMesh stores a polygon control mesh and refines its faces, edges and vertices.

#pragma once
#include <windows.h>
#include "plasma/Parallel.h"
#include "plasma/Vector.h"

namespace plasma {

// Individual flags are bit indices; use (1 << flag) to form a mask.
enum QuadVertexFlags {
    QUAD_BOUNDARY = 0,
    QUAD_SHARP = 1,
    QUAD_BEZIER = 2,
    QUAD_ROUND = 3
};

// Whole-field masks; the flags above are bit indices.
enum QuadVertexMasks {
    QUAD_FLAGS_NONE = 0
};

// Growth leaves these records uninitialized until topology setup fills them.
struct QuadVertex {
    QuadVertex() {}

    Vector<2, float> *m_position;
    Vector<4, float> *m_color;
    Vector<2, float> *m_texturePosition;
    Vector<2, float> m_positionSum;
    Vector<4, float> m_colorSum;
    Vector<2, float> m_textureSum;
    int m_count;           // index during topology setup
    unsigned char m_flags; // QuadVertexFlags
};

struct QuadEdge;

struct QuadFace {
    QuadFace() {}

    QuadVertex *m_vertices[4];
    QuadVertex m_center;
    QuadEdge *m_edges[4];
    QuadFace *m_children[4];
    int m_indices[4];
    QuadFace *m_parent; // The control face before subdivision.

    int m_unused78; // copied to child faces
};

struct QuadEdge {
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
