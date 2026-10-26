// Quad-mesh subdivision and its parallel tasks.
#include "plasma/QuadMesh.h"
#include <algorithm>
#include <map>
#include "Concurrency.h"

namespace plasma {

void QuadMesh::subdivide(QuadMesh &source)
{
    m_faces.resize(4 * source.m_faces.size());
    m_edges.resize(2 * (source.m_edges.size() + 2 * source.m_faces.size()));
    for (int faceIndex = 0; faceIndex < (int)source.m_faces.size(); ++faceIndex) {
        QuadFace &face = source.m_faces[faceIndex];
        for (int corner = 0; corner < 4; ++corner) {
            QuadFace &child = m_faces[4 * faceIndex + corner];
            child.m_vertices[0] = &face.m_center;
            child.m_vertices[1] = &face.m_edges[(corner + 3) % 4]->m_center;
            child.m_vertices[2] = face.m_vertices[corner];
            child.m_vertices[3] = &face.m_edges[corner]->m_center;
            child.m_children[0] = 0;
            child.m_children[1] = 0;
            child.m_children[2] = 0;
            child.m_children[3] = 0;
            if (face.m_parent)
                child.m_parent = face.m_parent;
            else
                child.m_parent = &face;
            child.m_unused78 = face.m_unused78;
            face.m_children[corner] = &child;
        }
    }
    for (int faceIndex = 0; faceIndex < (int)source.m_faces.size(); ++faceIndex) {
        QuadFace &face = source.m_faces[faceIndex];
        for (int corner = 0; corner < 4; ++corner) {
            QuadEdge &edge = m_edges[4 * faceIndex + corner];
            edge.m_faces[0] = face.m_children[corner];
            edge.m_faces[1] = face.m_children[(corner + 3) % 4];
            edge.m_vertices[0] = &face.m_center;
            edge.m_vertices[1] = &face.m_edges[(corner + 3) % 4]->m_center;
            edge.m_faces[0]->m_edges[0] = &edge;
            edge.m_faces[1]->m_edges[3] = &edge;
        }
    }
    for (int edgeIndex = 0; edgeIndex < (int)source.m_edges.size(); ++edgeIndex) {
        QuadEdge &edge = source.m_edges[edgeIndex];
        int firstChild = 2 * (edgeIndex + 2 * source.m_faces.size());
        for (int endpoint = 0; endpoint < 2; ++endpoint) {
            QuadEdge &child = m_edges[firstChild + endpoint];
            child.m_vertices[0] = &edge.m_center;
            child.m_vertices[1] = edge.m_vertices[endpoint];
            child.m_faces[0] = 0;
            child.m_faces[1] = 0;
            edge.m_children[endpoint] = &child;
            for (int corner = 0; corner < 4; ++corner) {
                for (int side = 0; side < 2; ++side) {
                    if (edge.m_faces[side] &&
                        edge.m_faces[side]->m_children[corner]->m_vertices[2] == child.m_vertices[1]) {
                        child.m_faces[side] = edge.m_faces[side]->m_children[corner];
                        if (child.m_vertices[0] == edge.m_faces[side]->m_children[corner]->m_vertices[1])
                            edge.m_faces[side]->m_children[corner]->m_edges[1] = &child;
                        if (child.m_vertices[0] == edge.m_faces[side]->m_children[corner]->m_vertices[3])
                            edge.m_faces[side]->m_children[corner]->m_edges[2] = &child;
                    }
                }
            }
        }
    }
    m_vertices.resize(source.m_edges.size() + source.m_faces.size() + source.m_vertices.size());
    int vertexIndex = 0;
    for (int i = 0; i < (int)source.m_vertices.size(); ++i)
        m_vertices[vertexIndex++] = source.m_vertices[i];
    for (int i = 0; i < (int)source.m_edges.size(); ++i) {
        source.m_edges[i].m_center.m_flags = 0;
        m_vertices[vertexIndex++] = &source.m_edges[i].m_center;
    }
    for (int i = 0; i < (int)source.m_faces.size(); ++i) {
        source.m_faces[i].m_center.m_flags = 0;
        m_vertices[vertexIndex++] = &source.m_faces[i].m_center;
    }
    for (int i = 0; i < (int)m_edges.size(); ++i) {
        QuadEdge &edge = m_edges[i];
        if (!edge.m_faces[0] || !edge.m_faces[1]) {
            edge.m_vertices[0]->m_flags |= (1 << QUAD_BOUNDARY);
            edge.m_vertices[1]->m_flags |= (1 << QUAD_BOUNDARY);
        }
    }
    for (int i = 0; i < (int)m_vertices.size(); ++i)
        m_vertices[i]->m_count = i;
    for (int i = 0; i < (int)m_faces.size(); ++i) {
        for (int corner = 0; corner < 4; ++corner)
            m_faces[i].m_indices[corner] = m_faces[i].m_vertices[corner]->m_count;
    }
}

void QuadMesh::prepare()
{
    std::map<QuadVertex *, int> vertices;
    int index = 0;
    for (int i = 0; i < (int)m_faces.size(); ++i) {
        for (int corner = 0; corner < 4; ++corner) {
            if (vertices.find(m_faces[i].m_vertices[corner]) == vertices.end())
                vertices[m_faces[i].m_vertices[corner]] = index++;
            m_faces[i].m_indices[corner] = vertices[m_faces[i].m_vertices[corner]];
        }
    }
    m_vertices.resize(vertices.size());
    for (std::map<QuadVertex *, int>::iterator i = vertices.begin(); i != vertices.end(); ++i) {
        m_vertices[i->second] = i->first;
        i->first->m_flags = 0;
    }
    for (int i = 0; i < (int)m_edges.size(); ++i) {
        QuadEdge &edge = m_edges[i];
        if (!edge.m_faces[0] || !edge.m_faces[1]) {
            edge.m_vertices[0]->m_flags |= (1 << QUAD_BOUNDARY);
            edge.m_vertices[1]->m_flags |= (1 << QUAD_BOUNDARY);
        }
    }
}

void QuadMesh::clear()
{
    m_faces.clear();
    m_edges.clear();
    m_vertices.clear();
}

// Evaluates the colour before applying the scale.
static Vector<4, float> scaleQuadColor(float scale, const Vector<4, float> &color);

void QuadMesh::update(float weight)
{
    Concurrency::parallel_for_each(m_vertices.begin(), m_vertices.end(), [](QuadVertex *vertex) {
        vertex->m_count = 0;
        vertex->m_positionSum = Vector<2, float>(0.0f, 0.0f);
        vertex->m_colorSum = Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
        vertex->m_textureSum = Vector<2, float>(0.0f, 0.0f);
    });
    float vertexWeight = 1.0 - weight;
    float faceWeight = weight;
    Concurrency::parallel_for_each(m_faces.begin(), m_faces.end(), [](QuadFace &face) {
        *face.m_center.m_position = ((*face.m_vertices[0]->m_position + *face.m_vertices[1]->m_position) +
                                     *face.m_vertices[2]->m_position + *face.m_vertices[3]->m_position) *
                                    0.25;
        *face.m_center.m_color = ((*face.m_vertices[0]->m_color + *face.m_vertices[1]->m_color) +
                                  *face.m_vertices[2]->m_color + *face.m_vertices[3]->m_color) *
                                 0.25;
        *face.m_center.m_texturePosition =
            ((*face.m_vertices[0]->m_texturePosition + *face.m_vertices[1]->m_texturePosition) +
             *face.m_vertices[2]->m_texturePosition + *face.m_vertices[3]->m_texturePosition) *
            0.25;
    });
    Concurrency::parallel_for_each(m_edges.begin(), m_edges.end(), [&](QuadEdge &edge) {
        edge.m_center.m_flags &= ~((1 << QUAD_SHARP) | (1 << QUAD_BEZIER));
        if (!edge.m_faces[0] || !edge.m_faces[1] || (edge.m_vertices[0]->m_flags & (1 << QUAD_SHARP)) ||
            (edge.m_vertices[1]->m_flags & (1 << QUAD_SHARP))) {
            *edge.m_center.m_position = (*edge.m_vertices[0]->m_position + *edge.m_vertices[1]->m_position) * 0.5;
            *edge.m_center.m_color = (*edge.m_vertices[0]->m_color + *edge.m_vertices[1]->m_color) * 0.5;
            *edge.m_center.m_texturePosition =
                (*edge.m_vertices[0]->m_texturePosition + *edge.m_vertices[1]->m_texturePosition) * 0.5;
            if ((edge.m_vertices[0]->m_flags & (1 << QUAD_BEZIER)) ||
                (edge.m_vertices[1]->m_flags & (1 << QUAD_BEZIER)))
                edge.m_center.m_flags |= (1 << QUAD_BEZIER);
        } else {
            *edge.m_center.m_position =
                (((*edge.m_vertices[0]->m_position + *edge.m_vertices[1]->m_position) * vertexWeight) +
                 ((*edge.m_faces[0]->m_center.m_position + *edge.m_faces[1]->m_center.m_position) * faceWeight)) *
                0.5;
            *edge.m_center.m_color = scaleQuadColor(
                0.5f, scaleQuadColor(vertexWeight, *edge.m_vertices[0]->m_color + *edge.m_vertices[1]->m_color) +
                          scaleQuadColor(faceWeight,
                                         *edge.m_faces[0]->m_center.m_color + *edge.m_faces[1]->m_center.m_color));
            *edge.m_center.m_texturePosition =
                (((*edge.m_vertices[0]->m_texturePosition + *edge.m_vertices[1]->m_texturePosition) * vertexWeight) +
                 ((*edge.m_faces[0]->m_center.m_texturePosition + *edge.m_faces[1]->m_center.m_texturePosition) *
                  faceWeight)) *
                0.5;
        }
    });
    std::for_each(m_edges.begin(), m_edges.end(), [](QuadEdge &edge) {
        Vector<2, float> position = (*edge.m_vertices[0]->m_position + *edge.m_vertices[1]->m_position) * 0.5;
        Vector<4, float> color = (*edge.m_vertices[0]->m_color + *edge.m_vertices[1]->m_color) * 0.5;
        Vector<2, float> texture =
            (*edge.m_vertices[0]->m_texturePosition + *edge.m_vertices[1]->m_texturePosition) * 0.5;
        for (int i = 0; i < 2; ++i) {
            QuadVertex *vertex = edge.m_vertices[i];
            if (!(vertex->m_flags & (1 << QUAD_BOUNDARY)) || !edge.m_faces[0] || !edge.m_faces[1]) {
                ++edge.m_vertices[i]->m_count;
                edge.m_vertices[i]->m_positionSum += position;
                edge.m_vertices[i]->m_colorSum += color;
                edge.m_vertices[i]->m_textureSum += texture;
            }
        }
    });
    std::for_each(m_faces.begin(), m_faces.end(), [](QuadFace &face) {
        for (int i = 0; i < 4; ++i) {
            QuadVertex *vertex = face.m_vertices[i];
            if (!(vertex->m_flags & (1 << QUAD_BOUNDARY))) {
                ++face.m_vertices[i]->m_count;
                face.m_vertices[i]->m_positionSum += *face.m_center.m_position;
                face.m_vertices[i]->m_colorSum += *face.m_center.m_color;
                face.m_vertices[i]->m_textureSum += *face.m_center.m_texturePosition;
            }
        }
    });
    Concurrency::parallel_for_each(m_vertices.begin(), m_vertices.end(), [&](QuadVertex *vertex) {
        // Interior subdivision retains one quarter of each vertex position/color/UV and distributes three quarters across
        // its accumulated neighbors.
        if (!(vertex->m_flags & (1 << QUAD_SHARP)) && vertex->m_count) {
            if (vertex->m_flags & (1 << QUAD_BOUNDARY)) {
                if (vertex->m_flags & (1 << QUAD_BEZIER)) {
                    float scale = 1.0 / (double)vertex->m_count;
                    *vertex->m_position = vertex->m_positionSum * scale;
                    *vertex->m_color = vertex->m_colorSum * scale;
                    *vertex->m_texturePosition = vertex->m_textureSum * scale;
                } else {
                    float scale = faceWeight / (double)vertex->m_count;
                    *vertex->m_position = (*vertex->m_position * vertexWeight) + (vertex->m_positionSum * scale);
                    *vertex->m_color = scaleQuadColor(vertexWeight, *vertex->m_color) + (vertex->m_colorSum * scale);
                    *vertex->m_texturePosition =
                        (*vertex->m_texturePosition * vertexWeight) + (vertex->m_textureSum * scale);
                }
            } else {
                float scale = 0.75 / (double)vertex->m_count;
                *vertex->m_position = (*vertex->m_position * 0.25) + (vertex->m_positionSum * scale);
                *vertex->m_color = (*vertex->m_color * 0.25) + (vertex->m_colorSum * scale);
                *vertex->m_texturePosition = (*vertex->m_texturePosition * 0.25) + (vertex->m_textureSum * scale);
            }
        }
    });
    Concurrency::parallel_for_each(m_edges.begin(), m_edges.end(), [](QuadEdge &edge) {
        if ((!edge.m_faces[0] || !edge.m_faces[1]) && (edge.m_vertices[0]->m_flags & (1 << QUAD_BEZIER)) &&
            (edge.m_vertices[1]->m_flags & (1 << QUAD_BEZIER))) {
            *edge.m_center.m_position = (*edge.m_vertices[0]->m_position + *edge.m_vertices[1]->m_position) * 0.5;
            *edge.m_center.m_color = (*edge.m_vertices[0]->m_color + *edge.m_vertices[1]->m_color) * 0.5;
            *edge.m_center.m_texturePosition =
                (*edge.m_vertices[0]->m_texturePosition + *edge.m_vertices[1]->m_texturePosition) * 0.5;
            edge.m_center.m_flags =
                (edge.m_center.m_flags & ~((1 << QUAD_SHARP) | (1 << QUAD_BEZIER))) | (1 << QUAD_SHARP);
        }
    });
}

static Vector<4, float> scaleQuadColor(float scale, const Vector<4, float> &color)
{
    Vector<4, float> result = color;
    for (unsigned i = 0; i < 4; ++i)
        result.m_elements[i] *= scale;
    return result;
}

}
