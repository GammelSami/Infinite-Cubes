#include "EditableMesh.h"
#include "plasma/SmoothMeshShape.h"
#include <cmath>

EditableMesh::EditableMesh(plasma::SmoothMeshShape *shape)
    : m_shape(shape)
{
    unsigned offset = m_vertexAttributes.size();
    m_vertexAttributes.resize(offset + sizeof(int));
    *reinterpret_cast<int *>(&m_vertexAttributes[offset]) = 0;
    m_vertexIndexOffset = offset;
    offset = m_vertexAttributes.size();
    m_vertexAttributes.resize(offset + sizeof(bool));
    m_vertexAttributes[offset] = false;
    m_vertexSelectedOffset = offset;
    offset = m_edgeAttributes.size();
    m_edgeAttributes.resize(offset + sizeof(bool));
    m_edgeAttributes[offset] = false;
    m_edgeSelectedOffset = offset;
    offset = m_faceAttributes.size();
    m_faceAttributes.resize(offset + sizeof(bool));
    m_faceAttributes[offset] = false;
    m_faceSelectedOffset = offset;
    rebuild();
}

void EditableMesh::rebuild()
{
    clear();
    std::map<int, MeshVertex *> vertices;
    for (int i = 0; i < (int)m_shape->m_vertexPositions.m_values.front()->size(); ++i) {
        MeshVertex *vertex = addVertex();
        vertexIndex(vertex) = i;
        vertices[i] = vertex;
    }
    for (int i = 0; i < (int)m_shape->m_faces.size(); ++i) {
        std::list<MeshVertex *> face;
        const std::vector<int> &indices = m_shape->m_faces[i];
        for (int j = 0; j < (int)indices.size(); ++j)
            face.push_back(vertices[indices[j]]);
        addFace(face, std::vector<char>());
    }
}

plasma::Vector<2, float> EditableMesh::getCenter()
{
    unsigned count = m_vertices.size();
    if (!count)
        return plasma::Vector<2, float>(0.0f, 0.0f);
    plasma::Vector<2, float> result(0.0f, 0.0f);
    for (VertexIterator it = m_vertices.begin(); it != m_vertices.end(); ++it) {
        int index = vertexIndex(*it);
        const plasma::Vector<2, float> &position = m_shape->m_vertexPositions.value()[index];
        result.m_elements[0] = position.m_elements[0] + result.m_elements[0];
        result.m_elements[1] = position.m_elements[1] + result.m_elements[1];
    }
    float divisor = (float)count;
    for (unsigned i = 0; i < 2; ++i)
        result.m_elements[i] /= divisor;
    return result;
}

void EditableMesh::subdivideEdge(MeshHalfEdge *edge)
{
    std::list<MeshVertex *> vertices;
    int step = 0;
    do {
        float fraction = (step + 1) * 0.5;
        float remainder = 1.0 - fraction;
        vertices.push_back(
            addShapeVertex(m_shape->m_vertexPositions.value()[vertexIndex(edge->m_vertex)] * remainder +
                               m_shape->m_vertexPositions.value()[vertexIndex(edge->m_next->m_vertex)] * fraction,
                           m_shape->m_vertexTexCoords.value()[vertexIndex(edge->m_vertex)] * remainder +
                               m_shape->m_vertexTexCoords.value()[vertexIndex(edge->m_next->m_vertex)] * fraction,
                           m_shape->m_vertexColors.value()[vertexIndex(edge->m_vertex)] * remainder +
                               m_shape->m_vertexColors.value()[vertexIndex(edge->m_next->m_vertex)] * fraction,
                           m_shape->m_strokeColors.value()[vertexIndex(edge->m_vertex)] * remainder +
                               m_shape->m_strokeColors.value()[vertexIndex(edge->m_next->m_vertex)] * fraction,
                           m_shape->m_extrusionFrontColors.value()[vertexIndex(edge->m_vertex)] * remainder +
                               m_shape->m_extrusionFrontColors.value()[vertexIndex(edge->m_next->m_vertex)] * fraction,
                           m_shape->m_extrusionBackColors.value()[vertexIndex(edge->m_vertex)] * remainder +
                               m_shape->m_extrusionBackColors.value()[vertexIndex(edge->m_next->m_vertex)] * fraction,
                           remainder * m_shape->m_strokeWidths.value()[vertexIndex(edge->m_vertex)] +
                               fraction * m_shape->m_strokeWidths.value()[vertexIndex(edge->m_next->m_vertex)]));
        ++step;
    } while (step < 1);
    for (std::list<MeshVertex *>::iterator it = vertices.begin(); it != vertices.end(); ++it)
        splitEdge(edge, *it);
}

void EditableMesh::connectVertices(const std::list<MeshVertex *> &vertices)
{
    splitFaces(vertices);
}

void EditableMesh::subdivideEdges(const std::set<MeshHalfEdge *> &edges)
{
    std::list<MeshVertex *> vertices;
    std::set<MeshHalfEdge *> opposites;
    for (std::set<MeshHalfEdge *>::const_iterator it = edges.begin(); it != edges.end(); ++it) {
        if (opposites.find(*it) == opposites.end() && (*it)->m_opposite)
            opposites.insert((*it)->m_opposite);
    }
    for (std::set<MeshHalfEdge *>::const_iterator it = edges.begin(); it != edges.end(); ++it) {
        if (opposites.find(*it) == opposites.end()) {
            subdivideEdge(*it);
            vertices.push_back(m_vertices.back());
        }
    }
    connectVertices(vertices);
}

void EditableMesh::subdivideBoundaryCorners()
{
    std::set<MeshHalfEdge *> edges;
    for (EdgeIterator it = m_edges.begin(); it != m_edges.end(); ++it) {
        MeshHalfEdge *edge = *it;
        if (!edge->m_opposite) {
            if (edge->m_next->m_opposite && edges.find(edge->m_next) == edges.end() &&
                edges.find(edge->m_next->m_opposite) == edges.end())
                edges.insert(edge->m_next);
            if (edge->m_previous->m_opposite && edges.find(edge->m_previous) == edges.end() &&
                edges.find(edge->m_previous->m_opposite) == edges.end())
                edges.insert(edge->m_previous);
        }
    }
    subdivideEdges(edges);
    updateShape();
}

MeshVertex *EditableMesh::addShapeVertex(const plasma::Vector<2, float> &position,
                                         const plasma::Vector<2, float> &texturePosition,
                                         const plasma::Vector<4, float> &fillColor,
                                         const plasma::Vector<4, float> &strokeColor,
                                         const plasma::Vector<4, float> &frontColor,
                                         const plasma::Vector<4, float> &backColor, float width)
{
    m_shape->m_vertexPositions.append(position);
    m_shape->m_vertexTexCoords.append(texturePosition);
    m_shape->m_vertexColors.append(fillColor);
    m_shape->m_strokeColors.append(strokeColor);
    m_shape->m_extrusionFrontColors.append(frontColor);
    m_shape->m_extrusionBackColors.append(backColor);
    m_shape->m_strokeWidths.append(width);
    MeshVertex *vertex = addVertex();
    *reinterpret_cast<int *>(&vertex->m_attributes[m_vertexIndexOffset]) =
        m_shape->m_vertexPositions.m_values[0]->size() - 1;
    return vertex;
}

// Rounds the result to float.
double EditableMesh::getSignedArea()
{
    float area = 0.0f;
    for (EdgeIterator it = beginEdges(); it != endEdges(); ++it) {
        if (!(*it)->m_opposite) {
            int first = vertexIndex((*it)->m_vertex);
            int second = vertexIndex((*it)->m_next->m_vertex);
            const plasma::Vector<2, float> &a = m_shape->m_vertexPositions.value()[first];
            const plasma::Vector<2, float> &b = m_shape->m_vertexPositions.value()[second];
            area = b.m_elements[1] * a.m_elements[0] - a.m_elements[1] * b.m_elements[0] + area;
        }
    }
    return (float)(area * 0.5);
}

void EditableMesh::updateShape()
{
    std::set<int> usedIndices;
    for (VertexIterator it = beginVertices(); it != endVertices(); ++it)
        usedIndices.insert(vertexIndex(*it));
    int vertexCount = m_shape->m_vertexPositions.m_values.front()->size();
    for (int i = vertexCount - 1; i >= 0; --i) {
        if (usedIndices.find(i) == usedIndices.end()) {
            m_shape->m_vertexPositions.erase(i);
            m_shape->m_vertexColors.erase(i);
            m_shape->m_vertexTexCoords.erase(i);
            m_shape->m_strokeColors.erase(i);
            m_shape->m_extrusionFrontColors.erase(i);
            m_shape->m_extrusionBackColors.erase(i);
            m_shape->m_strokeWidths.erase(i);
            m_shape->m_vertexFlags.erase(m_shape->m_vertexFlags.begin() + i);
            m_shape->m_vertexParameters.erase(m_shape->m_vertexParameters.begin() + i);
        }
    }
    int index = 0;
    for (VertexIterator it = beginVertices(); it != endVertices(); ++it)
        *reinterpret_cast<int *>(&(*it)->m_attributes[m_vertexIndexOffset]) = index++;
    std::vector<std::vector<int>> faces;
    for (FaceIterator it = beginFaces(); it != endFaces(); ++it) {
        std::vector<int> vertices;
        for (MeshFaceIterator vertex(*it); vertex.isValid(); ++vertex)
            vertices.push_back(vertexIndex(vertex.getVertex()));
        faces.push_back(vertices);
    }
    m_shape->setFaces(faces, true);
}

void EditableMesh::transform(const plasma::Matrix<float> &matrix)
{
    for (int i = 0; i < (int)m_shape->m_vertexPositions.m_values[0]->size(); ++i) {
        for (int frame = 0; frame < m_shape->m_vertexPositions.size(); ++frame) {
            plasma::Vector<2, float> &position = (*m_shape->m_vertexPositions.m_values[frame])[i];
            position.transform(matrix);
        }
    }
}

void EditableMesh::smoothBoundary()
{
    std::list<MeshHalfEdge *> boundary;
    for (EdgeIterator it = beginEdges(); it != endEdges(); it++) {
        if (!(*it)->m_opposite)
            boundary.push_back(*it);
    }
    unsigned flags = m_shape->getFlags();
    bool wasContour = (flags & (1 << plasma::MESH_CONTOUR)) != 0;
    bool wasUnfilled = (flags & (1 << plasma::MESH_NO_FILL)) != 0;
    int subdivisions = m_shape->getSubdivisions();
    if (wasContour)
        m_shape->setFlags(m_shape->getFlags() & ~(1u << plasma::MESH_CONTOUR));
    m_shape->setFlags(m_shape->getFlags() & ~(1u << plasma::MESH_NO_FILL));
    m_shape->setSubdivisions(1, true);
    MeshHalfEdge *closing = 0;
    if ((m_shape->getFlags() & (1 << plasma::MESH_OPEN)) && static_cast<int>(m_faces.size()) > 0)
        closing = (*beginFaces())->m_edge->m_previous;
    for (EdgeIterator it = boundary.begin(); it != boundary.end(); it++) {
        if (*it != closing)
            subdivideEdge(*it);
    }
    if (wasContour || !subdivisions) {
        m_shape->updateSubdivision();
        m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
    for (EdgeIterator it = boundary.begin(); it != boundary.end(); it++) {
        if (*it != closing && (*it)->m_previous != closing) {
            int index = vertexIndex((*it)->m_vertex);
            const plasma::Vector<2, float> *position = m_shape->getSubdivisionPosition(index);
            if (position)
                m_shape->m_vertexPositions.value()[index] = *position;
        }
    }
    if (wasContour)
        m_shape->setFlags(m_shape->getFlags() | (1 << plasma::MESH_CONTOUR));
    if (wasUnfilled)
        m_shape->setFlags(m_shape->getFlags() | (1 << plasma::MESH_NO_FILL));
    else
        m_shape->setFlags(m_shape->getFlags() & ~(1u << plasma::MESH_NO_FILL));
    m_shape->setSubdivisions(subdivisions, true);
    if (wasContour || !subdivisions) {
        m_shape->updateSubdivision();
        m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
    updateShape();
    bool changed = false;
    for (EdgeIterator it = beginEdges(); it != endEdges(); it++) {
        MeshHalfEdge *edge = *it;
        if (edge->m_opposite || edge == closing || edge->m_previous == closing)
            continue;
        int index = vertexIndex(edge->m_previous->m_vertex);
        int nextIndex = vertexIndex(edge->m_next->m_vertex);
        int type = m_shape->m_vertexFlags[index];
        int nextType = m_shape->m_vertexFlags[nextIndex];
        int current = vertexIndex(edge->m_vertex);
        if ((type == plasma::VERTEX_BEZIER && nextType == plasma::VERTEX_BEZIER) ||
            (type == plasma::VERTEX_ROUND && nextType == plasma::VERTEX_ROUND)) {
            m_shape->m_vertexPositions.value()[current] =
                (m_shape->m_vertexPositions.value()[index] + m_shape->m_vertexPositions.value()[nextIndex]) * 0.5;
            changed = true;
        } else if (type == plasma::VERTEX_ROUND) {
            static float weight = 1.0 / (2.0 * std::cos(0.7853982f) + 1.0);
            float otherWeight = 1.0 - weight;
            m_shape->m_vertexPositions.value()[current] = m_shape->m_vertexPositions.value()[index] * weight +
                                                          m_shape->m_vertexPositions.value()[nextIndex] * otherWeight;
            changed = true;
        } else if (nextType == plasma::VERTEX_ROUND) {
            static float weight = 1.0 / (2.0 * std::cos(0.7853982f) + 1.0);
            float otherWeight = 1.0 - weight;
            m_shape->m_vertexPositions.value()[current] = m_shape->m_vertexPositions.value()[nextIndex] * weight +
                                                          m_shape->m_vertexPositions.value()[index] * otherWeight;
            changed = true;
        } else if (type == plasma::VERTEX_BEZIER || nextType == plasma::VERTEX_BEZIER) {
            m_shape->m_vertexFlags[current] = plasma::VERTEX_BEZIER;
            changed = true;
        }
    }
    for (VertexIterator it = beginVertices(); it != endVertices(); it++) {
        int index = vertexIndex(*it);
        if (m_shape->m_vertexFlags[index] == plasma::VERTEX_ROUND)
            m_shape->m_vertexFlags[index] = plasma::VERTEX_SMOOTH;
    }
    if (changed)
        m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
}
void EditableMesh::refine()
{
    if (!m_shape->drawsFill())
        smoothBoundary();
    else {
        m_shape->bakeSubdivision();
        rebuild();
    }
}
