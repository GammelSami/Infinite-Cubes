// The kind (low 16 bits) and constraints (high 16 bits) of a shape vertex, shared by MeshShape, SmoothMeshShape
// and CurveShape.
#ifndef PLASMA_VERTEX_FLAGS_H
#define PLASMA_VERTEX_FLAGS_H

namespace plasma {

enum VertexKind {
    VERTEX_SMOOTH = 0,
    VERTEX_SHARP = 1,
    VERTEX_BEZIER = 2, // Bit 1 is shared with ROUND; & VERTEX_BEZIER tests either curved kind.
    VERTEX_ROUND = 3
};

// Bit indices in the packed vertex word.
enum VertexConstraintFlag {
    VERTEX_ROUNDED_CORNER = 16,
    VERTEX_FIXED_ON_EDGE = 17
};

enum VertexMasks {
    VERTEX_FREE = 0,
    VERTEX_KIND_MASK = 0xffff,
    VERTEX_CONSTRAINT_MASK = 0xffff0000
};

}

#endif
