#pragma once
#include <list>
#include <set>
#include <vector>
#include "plasma/Vector.h"

struct MeshHalfEdge;
struct MeshFace;

// Elements own their byte attributes; the mesh's byte vectors hold defaults.
struct MeshVertex {
    explicit MeshVertex(const std::vector<char> &attributes);

    // The element owner's incidence removal helper.
    void removeEdge(MeshHalfEdge *edge) { m_edges.remove(edge); }

    std::vector<char> m_attributes;
    std::list<MeshVertex *>::iterator m_meshPosition;
    std::list<MeshHalfEdge *> m_edges;
};

struct MeshHalfEdge {
    explicit MeshHalfEdge(const std::vector<char> &attributes);
    std::vector<char> m_attributes;
    std::list<MeshHalfEdge *>::iterator m_meshPosition;
    MeshVertex *m_vertex;
    MeshHalfEdge *m_opposite;
    MeshHalfEdge *m_next;
    MeshHalfEdge *m_previous;
    MeshFace *m_face;
};

struct MeshFace {
    explicit MeshFace(const std::vector<char> &attributes);
    std::vector<char> m_attributes;
    std::list<MeshFace *>::iterator m_meshPosition;
    MeshHalfEdge *m_edge;
};

// Null current marks the initial position; after one circuit current == first.
class MeshFaceIterator {
public:
    explicit MeshFaceIterator(MeshHalfEdge *edge);
    explicit MeshFaceIterator(MeshFace *face);
    bool isValid();
    MeshVertex *getVertex();
    MeshFaceIterator &operator++();
    MeshHalfEdge *getEdge();
    MeshFace *getAdjacentFace();

    MeshHalfEdge *m_first;
    MeshHalfEdge *m_current;
};

class HalfEdgeMesh {
public:
    typedef std::list<MeshVertex *>::iterator VertexIterator;
    typedef std::list<MeshHalfEdge *>::iterator EdgeIterator;
    typedef std::list<MeshFace *>::iterator FaceIterator;

    HalfEdgeMesh();
    virtual ~HalfEdgeMesh();
    MeshVertex *addVertex(const std::vector<char> &attributes = std::vector<char>());
    MeshHalfEdge *addHalfEdge(MeshVertex *vertex, MeshFace *face, const std::vector<char> &attributes);
    MeshFace *addFace(const std::list<MeshVertex *> &vertices, const std::vector<char> &attributes);
    void removeFace(MeshFace *face);
    void removeVertex(MeshVertex *vertex);
    void removeEdgeFaces(MeshHalfEdge *edge);
    void dissolveEdges(const std::list<MeshHalfEdge *> &edges);
    void getConnectedFaceGroups(const std::list<MeshFace *> &faces, std::list<std::list<MeshFace *>> &groups);
    void removeVertices(const std::list<MeshVertex *> &vertices);
    void removeFaces(const std::list<MeshFace *> &faces);
    void splitEdge(MeshHalfEdge *edge, MeshVertex *vertex);
    MeshFace *splitFace(MeshVertex *first, MeshVertex *second, MeshFace *face);
    void splitFaceAtVertices(const std::set<MeshVertex *> &vertices, MeshFace *face, MeshVertex *first);
    void splitFaces(const std::list<MeshVertex *> &vertices);
    void clear();
    unsigned getVertexCount();
    unsigned getEdgeCount();
    unsigned getFaceCount();
    VertexIterator beginVertices();
    VertexIterator endVertices();
    EdgeIterator beginEdges();
    EdgeIterator endEdges();
    FaceIterator beginFaces();
    FaceIterator endFaces();
    std::list<MeshVertex *> getVertices();
    void getBoundaryEdges(std::list<MeshHalfEdge *> &edges, const std::list<MeshFace *> &faces);
    void orderBoundaryEdges(std::list<MeshHalfEdge *> &edges);

    std::list<MeshVertex *> m_vertices;
    std::list<MeshHalfEdge *> m_edges;
    std::list<MeshFace *> m_faces;
    std::vector<char> m_vertexAttributes;
    std::vector<char> m_edgeAttributes;
    std::vector<char> m_faceAttributes;
};

// Squared distance avoids a square root in the mesh tools' hit tests.
float squaredDistanceToSegment(const plasma::Vector<2, float> &point, const plasma::Vector<2, float> &first,
                               const plasma::Vector<2, float> &second);

// The number of the face's corners (GenericInterface.cpp).
int countCorners(MeshFace *face);
