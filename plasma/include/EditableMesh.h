#pragma once
#include "HalfEdgeMesh.h"
#include "plasma/Vector.h"
#include "plasma/Matrix.h"
#include "plasma/Shape.h"
#include "plasma/Attribute.h"
#include "plasma/ContinuousArrayAttribute.h"

namespace plasma {
class SmoothMeshShape;
}

// Integer attributes use casts into char storage; retain its lifetime and aliasing assumptions rather than
// replacing the byte layout.
class EditableMesh : public HalfEdgeMesh {
public:
    explicit EditableMesh(plasma::SmoothMeshShape *shape);
    void rebuild();
    void subdivideEdge(MeshHalfEdge *edge);
    void subdivideEdges(const std::set<MeshHalfEdge *> &edges);
    void subdivideBoundaryCorners();
    MeshVertex *addShapeVertex(const plasma::Vector<2, float> &position,
                               const plasma::Vector<2, float> &texturePosition,
                               const plasma::Vector<4, float> &fillColor, const plasma::Vector<4, float> &strokeColor,
                               const plasma::Vector<4, float> &frontColor, const plasma::Vector<4, float> &backColor,
                               float width);
    void updateShape();
    void transform(const plasma::Matrix<float> &matrix);
    void smoothBoundary();
    void refine();
    plasma::Vector<2, float> getCenter();
    double getSignedArea();
    void connectVertices(const std::list<MeshVertex *> &vertices);

    // The vertex's index in the shape's arrays (an int attribute at m_vertexIndexOffset in the vertex's byte
    // attributes).
    int &vertexIndex(MeshVertex *vertex)
    {
        return *reinterpret_cast<int *>(&vertex->m_attributes[m_vertexIndexOffset]);
    }

    // Defined after SmoothMeshShape is complete in its header.
    inline int &vertexFlags(MeshVertex *vertex);

    unsigned m_vertexIndexOffset;
    unsigned m_vertexSelectedOffset;
    unsigned m_edgeSelectedOffset;
    unsigned m_faceSelectedOffset;
    plasma::SmoothMeshShape *m_shape;
};
