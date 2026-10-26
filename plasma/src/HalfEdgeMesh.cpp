#include "HalfEdgeMesh.h"
#include <cstring>

HalfEdgeMesh::HalfEdgeMesh() {}

HalfEdgeMesh::~HalfEdgeMesh()
{
    clear();
}

MeshVertex *HalfEdgeMesh::addVertex(const std::vector<char> &attributes)
{
    MeshVertex *vertex = new MeshVertex(attributes.empty() ? m_vertexAttributes : attributes);
    m_vertices.push_back(vertex);
    vertex->m_meshPosition = --m_vertices.end();
    return vertex;
}

MeshHalfEdge *HalfEdgeMesh::addHalfEdge(MeshVertex *vertex, MeshFace *face, const std::vector<char> &attributes)
{
    MeshHalfEdge *edge = new MeshHalfEdge(attributes.empty() ? m_edgeAttributes : attributes);
    edge->m_face = face;
    edge->m_vertex = vertex;
    vertex->m_edges.push_back(edge);
    m_edges.push_back(edge);
    edge->m_meshPosition = --m_edges.end();
    return edge;
}

MeshFace *HalfEdgeMesh::addFace(const std::list<MeshVertex *> &vertices, const std::vector<char> &attributes)
{
    if (vertices.empty())
        return 0;
    std::set<std::pair<MeshVertex *, MeshVertex *>> directedEdges;
    for (EdgeIterator it = m_edges.begin(); it != m_edges.end(); ++it) {
        std::pair<MeshVertex *, MeshVertex *> directedEdge = std::make_pair(
            static_cast<MeshVertex *>((*it)->m_vertex), static_cast<MeshVertex *>((*it)->m_next->m_vertex));
        directedEdges.insert(directedEdge);
    }
    for (std::list<MeshVertex *>::const_iterator it = vertices.begin(); it != vertices.end(); ++it) {
        std::list<MeshVertex *>::const_iterator next = it;
        if (++next == vertices.end())
            next = vertices.begin();
        if (directedEdges.find(std::make_pair(*it, *next)) != directedEdges.end())
            return 0;
    }
    std::set<MeshVertex *> uniqueVertices;
    for (std::list<MeshVertex *>::const_iterator it = vertices.begin(); it != vertices.end(); ++it)
        uniqueVertices.insert(*it);
    if (uniqueVertices.size() != vertices.size())
        return 0;
    MeshFace *face = new MeshFace(attributes.empty() ? m_faceAttributes : attributes);
    m_faces.push_back(face);
    face->m_meshPosition = --m_faces.end();
    std::list<MeshHalfEdge *> edges;
    for (std::list<MeshVertex *>::const_iterator it = vertices.begin(); it != vertices.end(); ++it)
        edges.push_back(addHalfEdge(*it, face, std::vector<char>()));
    face->m_edge = edges.front();
    MeshHalfEdge *previous = edges.back();
    for (EdgeIterator it = edges.begin(); it != edges.end(); ++it) {
        previous->m_next = *it;
        (*it)->m_previous = previous;
        previous = *it;
    }
    previous = edges.back();
    for (EdgeIterator it = edges.begin(); it != edges.end(); ++it) {
        for (EdgeIterator candidate = (*it)->m_vertex->m_edges.begin(); candidate != (*it)->m_vertex->m_edges.end();
             ++candidate) {
            if ((*candidate)->m_next->m_vertex == previous->m_vertex && (*candidate)->m_face != face) {
                (*candidate)->m_opposite = previous;
                previous->m_opposite = *candidate;
                break;
            }
        }
        previous = *it;
    }
    for (MeshFaceIterator it(face); it.isValid(); ++it) {
        MeshHalfEdge *edge = it.getEdge();
        if (edge->m_opposite && !edge->m_attributes.empty())
            std::memcpy(&edge->m_attributes[0], &edge->m_opposite->m_attributes[0], m_edgeAttributes.size());
    }
    return face;
}

void HalfEdgeMesh::removeFace(MeshFace *face)
{
    m_faces.erase(face->m_meshPosition);
    std::list<MeshHalfEdge *> edges;
    for (MeshFaceIterator it(face); it.isValid(); ++it) {
        MeshHalfEdge *const edge = it.getEdge();
        MeshHalfEdge *opposite = edge->m_opposite;
        if (opposite)
            opposite->m_opposite = 0;
        edge->m_vertex->removeEdge(edge);
        m_edges.erase(edge->m_meshPosition);
        edges.push_back(it.getEdge());
    }
    for (EdgeIterator it = edges.begin(); it != edges.end(); ++it)
        delete *it;
    delete face;
}

void HalfEdgeMesh::removeVertex(MeshVertex *vertex)
{
    std::set<MeshFace *> faces;
    for (EdgeIterator it = vertex->m_edges.begin(); it != vertex->m_edges.end(); ++it) {
        if ((*it)->m_face)
            faces.insert((*it)->m_face);
    }
    std::list<MeshFace *> faceList;
    for (std::set<MeshFace *>::iterator it = faces.begin(); it != faces.end(); ++it)
        faceList.push_back(*it);

    // The code also constructs this unused list before collecting boundaries.
    std::list<std::list<MeshVertex *>> unusedBoundaries;
    std::list<std::list<MeshVertex *>> boundaries;
    std::list<MeshHalfEdge *> edges;
    getBoundaryEdges(edges, faceList);
    orderBoundaryEdges(edges);
    boundaries.push_back(std::list<MeshVertex *>());
    for (EdgeIterator it = edges.begin(); it != edges.end(); ++it) {
        if ((*it)->m_vertex != vertex)
            boundaries.back().push_back((*it)->m_vertex);
    }
    for (std::set<MeshFace *>::iterator it = faces.begin(); it != faces.end(); ++it)
        removeFace(*it);
    for (std::list<std::list<MeshVertex *>>::iterator it = boundaries.begin(); it != boundaries.end(); ++it) {
        if (it->size() > 1)
            addFace(*it, std::vector<char>());
    }
    m_vertices.erase(vertex->m_meshPosition);
    delete vertex;
}

void HalfEdgeMesh::removeEdgeFaces(MeshHalfEdge *edge)
{
    if (edge->m_opposite) {
        MeshFace *face = edge->m_opposite->m_face;
        if (face)
            removeFace(face);
    }
    if (edge->m_face)
        removeFace(edge->m_face);
}

void HalfEdgeMesh::removeVertices(const std::list<MeshVertex *> &vertices)
{
    for (std::list<MeshVertex *>::const_iterator it = vertices.begin(); it != vertices.end(); ++it)
        removeVertex(*it);
}

void HalfEdgeMesh::dissolveEdges(const std::list<MeshHalfEdge *> &edges)
{
    std::set<MeshFace *> faces;
    for (std::list<MeshHalfEdge *>::const_iterator it = edges.begin(); it != edges.end(); ++it) {
        if ((*it)->m_face)
            faces.insert((*it)->m_face);
        if ((*it)->m_opposite && (*it)->m_opposite->m_face)
            faces.insert((*it)->m_opposite->m_face);
    }
    std::list<MeshFace *> faceList;
    for (std::set<MeshFace *>::iterator it = faces.begin(); it != faces.end(); ++it)
        faceList.push_back(*it);
    std::list<std::list<MeshFace *>> groups;
    std::list<std::list<MeshVertex *>> boundaries;
    getConnectedFaceGroups(faceList, groups);
    for (std::list<std::list<MeshFace *>>::iterator it = groups.begin(); it != groups.end(); ++it) {
        std::list<MeshHalfEdge *> boundary;
        getBoundaryEdges(boundary, *it);
        orderBoundaryEdges(boundary);
        boundaries.push_back(std::list<MeshVertex *>());
        for (EdgeIterator edge = boundary.begin(); edge != boundary.end(); ++edge)
            boundaries.back().push_back((*edge)->m_vertex);
    }
    for (std::set<MeshFace *>::iterator it = faces.begin(); it != faces.end(); ++it)
        removeFace(*it);
    for (std::list<std::list<MeshVertex *>>::iterator it = boundaries.begin(); it != boundaries.end(); ++it)
        addFace(*it, std::vector<char>());
}

void HalfEdgeMesh::removeFaces(const std::list<MeshFace *> &faces)
{
    std::set<MeshVertex *> vertices;
    for (std::list<MeshFace *>::const_iterator it = faces.begin(); it != faces.end(); ++it) {
        for (MeshFaceIterator vertex(*it); vertex.isValid(); ++vertex) {
            MeshVertex *current = vertex.getVertex();
            vertices.insert(current);
        }
        removeFace(*it);
    }
    for (std::set<MeshVertex *>::iterator it = vertices.begin(); it != vertices.end(); ++it) {
        if ((*it)->m_edges.empty())
            removeVertex(*it);
    }
}

void HalfEdgeMesh::splitEdge(MeshHalfEdge *edge, MeshVertex *vertex)
{
    MeshHalfEdge *newEdge = addHalfEdge(vertex, edge->m_face, std::vector<char>());
    newEdge->m_attributes = edge->m_attributes;
    newEdge->m_next = edge->m_next;
    newEdge->m_previous = edge;
    edge->m_next->m_previous = newEdge;
    edge->m_next = newEdge;
    if (edge->m_opposite) {
        MeshHalfEdge *opposite = addHalfEdge(vertex, edge->m_opposite->m_face, std::vector<char>());
        opposite->m_attributes = edge->m_opposite->m_attributes;
        opposite->m_next = edge->m_opposite->m_next;
        opposite->m_previous = edge->m_opposite;
        edge->m_opposite->m_next->m_previous = opposite;
        edge->m_opposite->m_next = opposite;
        newEdge->m_opposite = edge->m_opposite;
        edge->m_opposite->m_opposite = newEdge;
        edge->m_opposite = opposite;
        opposite->m_opposite = edge;
    }
}

MeshFace *HalfEdgeMesh::splitFace(MeshVertex *first, MeshVertex *second, MeshFace *face)
{
    for (EdgeIterator it = beginEdges(); it != endEdges(); ++it) {
        if ((*it)->m_vertex == first && (*it)->m_next->m_vertex == second)
            return 0;
    }
    bool firstSide = false;
    std::list<MeshVertex *> firstVertices;
    std::list<MeshVertex *> secondVertices;
    for (MeshFaceIterator it(face); it.isValid(); ++it) {
        if (firstSide) {
            firstVertices.push_back(it.getVertex());
            MeshVertex *vertex = it.getVertex();
            if (vertex == first || vertex == second) {
                firstSide = false;
                secondVertices.push_back(it.getVertex());
            }
        } else {
            secondVertices.push_back(it.getVertex());
            MeshVertex *vertex = it.getVertex();
            if (vertex == first || vertex == second) {
                firstSide = true;
                firstVertices.push_back(it.getVertex());
            }
        }
    }
    removeFace(face);
    addFace(firstVertices, std::vector<char>());
    return addFace(secondVertices, std::vector<char>());
}

void HalfEdgeMesh::splitFaceAtVertices(const std::set<MeshVertex *> &vertices, MeshFace *face, MeshVertex *first)
{
restart:
    if (face) {
        for (MeshFaceIterator it(face); it.isValid(); ++it) {
            MeshHalfEdge *edge = it.getEdge();
            if (vertices.find(edge->m_vertex) != vertices.end()) {
                if (!first)
                    first = edge->m_vertex;
                for (MeshFaceIterator candidate(edge); candidate.isValid(); ++candidate) {
                    MeshVertex *vertex = candidate.getVertex();
                    if (vertex != edge->m_vertex && vertex != edge->m_next->m_vertex &&
                        vertex != edge->m_previous->m_vertex) {
                        MeshVertex *candidateVertex = candidate.getVertex();
                        if (vertices.find(candidateVertex) == vertices.end())
                            continue;
                        face = splitFace(it.getEdge()->m_vertex, candidate.getVertex(), face);
                        goto restart;
                    }
                }
            }
        }
    }
}

void HalfEdgeMesh::splitFaces(const std::list<MeshVertex *> &vertices)
{
    std::set<MeshVertex *> vertexSet;
    std::set<MeshFace *> faces;
    for (std::list<MeshVertex *>::const_iterator it = vertices.begin(); it != vertices.end(); ++it) {
        vertexSet.insert(*it);
        for (EdgeIterator edge = (*it)->m_edges.begin(); edge != (*it)->m_edges.end(); ++edge)
            faces.insert((*edge)->m_face);
    }
    for (std::set<MeshFace *>::iterator it = faces.begin(); it != faces.end(); ++it)
        splitFaceAtVertices(vertexSet, *it, 0);
}

void HalfEdgeMesh::clear()
{
    for (VertexIterator it = beginVertices(); it != endVertices(); ++it)
        delete *it;
    for (EdgeIterator it = beginEdges(); it != endEdges(); ++it)
        delete *it;
    for (FaceIterator it = beginFaces(); it != endFaces(); ++it)
        delete *it;
    m_vertices.clear();
    m_edges.clear();
    m_faces.clear();
}

unsigned HalfEdgeMesh::getVertexCount()
{
    return m_vertices.size();
}

unsigned HalfEdgeMesh::getEdgeCount()
{
    return m_edges.size();
}

unsigned HalfEdgeMesh::getFaceCount()
{
    return m_faces.size();
}

HalfEdgeMesh::VertexIterator HalfEdgeMesh::beginVertices()
{
    return m_vertices.begin();
}

HalfEdgeMesh::VertexIterator HalfEdgeMesh::endVertices()
{
    return m_vertices.end();
}

HalfEdgeMesh::EdgeIterator HalfEdgeMesh::beginEdges()
{
    return m_edges.begin();
}

HalfEdgeMesh::EdgeIterator HalfEdgeMesh::endEdges()
{
    return m_edges.end();
}

HalfEdgeMesh::FaceIterator HalfEdgeMesh::beginFaces()
{
    return m_faces.begin();
}

HalfEdgeMesh::FaceIterator HalfEdgeMesh::endFaces()
{
    return m_faces.end();
}

void HalfEdgeMesh::getConnectedFaceGroups(const std::list<MeshFace *> &faces, std::list<std::list<MeshFace *>> &groups)
{
    std::set<MeshFace *> visited;
    std::set<MeshFace *> faceSet;
    for (std::list<MeshFace *>::const_iterator it = faces.begin(); it != faces.end(); ++it)
        faceSet.insert(*it);
    for (std::list<MeshFace *>::const_iterator it = faces.begin(); it != faces.end(); ++it) {
        if (visited.find(*it) == visited.end()) {
            groups.push_back(std::list<MeshFace *>());
            std::list<std::list<MeshFace *>>::iterator group = --groups.end();
            std::list<MeshFace *> pending;
            pending.push_back(*it);
            do {
                MeshFace *face = pending.back();
                visited.insert(face);
                pending.pop_back();
                group->push_back(face);
                for (MeshFaceIterator edge(face); edge.isValid(); ++edge) {
                    MeshFace *const adjacent = edge.getAdjacentFace();
                    if (adjacent && faceSet.find(adjacent) != faceSet.end() && visited.find(adjacent) == visited.end())
                        pending.push_back(edge.getAdjacentFace());
                }
            } while (!pending.empty());
        }
    }
}

void HalfEdgeMesh::getBoundaryEdges(std::list<MeshHalfEdge *> &edges, const std::list<MeshFace *> &faces)
{
    std::set<MeshFace *> faceSet;
    for (std::list<MeshFace *>::const_iterator it = faces.begin(); it != faces.end(); ++it)
        faceSet.insert(*it);
    for (std::list<MeshFace *>::const_iterator it = faces.begin(); it != faces.end(); ++it) {
        for (MeshFaceIterator edge(*it); edge.isValid(); ++edge) {
            MeshHalfEdge *const current = edge.getEdge();
            MeshHalfEdge *opposite = current->m_opposite;
            if (!opposite || faceSet.find(opposite->m_face) == faceSet.end())
                edges.push_back(edge.getEdge());
        }
    }
}

void HalfEdgeMesh::orderBoundaryEdges(std::list<MeshHalfEdge *> &edges)
{
    if (!edges.empty()) {
        std::set<MeshHalfEdge *> remaining;
        for (EdgeIterator it = edges.begin(); it != edges.end(); ++it)
            remaining.insert(*it);
        edges.clear();
        edges.push_back(*remaining.begin());
        for (int i = 1; i < (int)remaining.size(); ++i) {
            for (std::set<MeshHalfEdge *>::iterator it = remaining.begin(); it != remaining.end(); ++it) {
                if ((*it)->m_vertex == edges.back()->m_next->m_vertex) {
                    edges.push_back(*it);
                    break;
                }
            }
        }
    }
}

MeshFaceIterator::MeshFaceIterator(MeshHalfEdge *edge)
    : m_first(edge),
      m_current(0)
{}

bool MeshFaceIterator::isValid()
{
    return m_current != m_first;
}

MeshVertex *MeshFaceIterator::getVertex()
{
    MeshHalfEdge *edge = m_current;
    if (!edge)
        edge = m_first;
    return edge->m_vertex;
}

MeshFaceIterator &MeshFaceIterator::operator++()
{
    if (!m_current)
        m_current = m_first->m_next;
    else
        m_current = m_current->m_next;
    return *this;
}

MeshHalfEdge *MeshFaceIterator::getEdge()
{
    if (!m_current)
        return m_first;
    return m_current;
}

MeshFaceIterator::MeshFaceIterator(MeshFace *face)
    : m_first(face->m_edge),
      m_current(0)
{}

MeshFace *MeshFaceIterator::getAdjacentFace()
{
    MeshHalfEdge *edge = m_current;
    if (!edge)
        edge = m_first;
    MeshHalfEdge *opposite = edge->m_opposite;
    if (opposite)
        return opposite->m_face;
    return 0;
}

// A copy of the vertex list (Workspace::TraceMeshes).
std::list<MeshVertex *> HalfEdgeMesh::getVertices()
{
    return m_vertices;
}

float squaredDistanceToSegment(const plasma::Vector<2, float> &point, const plasma::Vector<2, float> &first,
                               const plasma::Vector<2, float> &second)
{
    plasma::Vector<2, float> relative = point - first;
    plasma::Vector<2, float> direction = second - first;
    float squaredLength =
        direction.m_elements[0] * direction.m_elements[0] + direction.m_elements[1] * direction.m_elements[1];
    if (squaredLength < 1.0e-20f)
        return relative.m_elements[0] * relative.m_elements[0] + relative.m_elements[1] * relative.m_elements[1];
    float projection = direction.m_elements[0] * relative.m_elements[0];
    projection += direction.m_elements[1] * relative.m_elements[1];
    projection /= squaredLength;
    if (projection <= 0.0f)
        return relative.m_elements[0] * relative.m_elements[0] + relative.m_elements[1] * relative.m_elements[1];
    if (projection >= 1.0f) {
        relative = point - second;
        return relative.m_elements[0] * relative.m_elements[0] + relative.m_elements[1] * relative.m_elements[1];
    }
    direction = direction * projection;
    relative = relative - direction;
    return relative.m_elements[0] * relative.m_elements[0] + relative.m_elements[1] * relative.m_elements[1];
}
