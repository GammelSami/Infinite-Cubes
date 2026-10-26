// QuadMesh holds one subdivision level of vertices, edges and faces. SmoothMeshShape keeps six levels: the control
// level comes from polygon faces and each subsequent level subdivides the preceding one.
#pragma once
#include "Parallel.h"
#include "plasma/Vector.h"

namespace plasma {

// QuadVertex::m_flags.
enum QuadVertexFlags {
    QUAD_BOUNDARY = 0, // on an edge with only one face
    QUAD_SHARP = 1,    // VERTEX_SHARP: kept in place, its edges split linearly
    QUAD_BEZIER = 2,   // VERTEX_BEZIER (and VERTEX_ROUND)
    QUAD_ROUND = 3     // VERTEX_ROUND (with QUAD_BEZIER)
};

struct QuadVertex {
    Vector<2, float> *m_position;
    Vector<4, float> *m_color;
    Vector<2, float> *m_texturePosition;
    Vector<2, float> m_positionSum;
    Vector<4, float> m_colorSum;
    Vector<2, float> m_textureSum;
    int m_count;           // summed neighbours; the index during Subdivide
    unsigned char m_flags; // QuadVertexFlags
};

struct QuadEdge;

struct QuadFace {
    QuadVertex *m_vertices[4];
    QuadVertex m_center;
    QuadEdge *m_edges[4];
    QuadFace *m_children[4]; // set by the next level's Subdivide
    int m_indices[4];        // m_vertices' indices in m_vertices of the mesh (Prepare)
    QuadFace *m_parent;      // the control face (0 on control faces)
    void *m_unusedB8;
};

struct QuadEdge {
    QuadFace *m_faces[2]; // 0 on a boundary side
    QuadVertex *m_vertices[2];
    QuadVertex m_center;
    QuadEdge *m_children[2];
};

// Subdivision mesh topology and interpolated vertex data.
class QuadMesh {
public:
    void subdivide(QuadMesh &source);
    void prepare();
    void clear();
    void update(float weight); // Weight of the face centre; 1 - weight weighs the edge vertices.

    Concurrency::concurrent_vector<QuadFace> m_faces;
    Concurrency::concurrent_vector<QuadEdge> m_edges;
    Concurrency::concurrent_vector<QuadVertex *> m_vertices;
};

}
