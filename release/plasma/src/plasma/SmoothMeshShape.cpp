#include <algorithm>
// It builds the control topology of the faces (updateSubdivision), smooths the mesh into its fill drawing
// (updateSubdivisionDrawings), refines the boundary contours and builds the stroke outline (attributesChanged), and
// fills the fill, extrusion and stroke drawings (updateDrawings and the stroke builders).
#include "plasma/SmoothMeshShape.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/StaticMeshShape.h"
#include "plasma/Drawing.h"
#include "plasma/Texture.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include "plasma/EditFilter.h"

namespace plasma {

static const int DefaultMeshStrokeSegments = 20;
static const int DefaultMeshSubdivisions = 3;
static const float AdaptiveSubdivisionCornerBudget = 2560.0f;
static const float MinimumStrokeProjectionDivisor = 0.0000001f;
static const float MinimumContourStrokeWidth = 0.1f;
static const float MinimumCornerNormalProjection = 0.3f;
static const float NearlyParallelCornerNormalCosine = 0.95f;
static const float CornerSegmentsPerHalfTurn = 20.0f;
static const int MaxMeshSubdivisions = 6;

// The length of a stroke segment in units of its mean width (texture distance along the stroke).
static inline float strokeDistance(float length, float firstWidth, float secondWidth)
{
    return (length + length) / (secondWidth + firstWidth);
}

// Refines a contour by inserting segment midpoints, weighting the first level towards round vertices, smoothing
// ordinary vertices and straightening midpoints between Bezier vertices. Smoothness is controlled by weight.
template<class T>
static void refineContour(std::vector<T> &values, std::vector<int> &types, int step, int segments, bool closed,
                          float weight, int level)
{
    for (int i = 0; i < segments; ++i) {
        values[(i * step + step / 2) % values.size()] =
            (values[(i * step) % values.size()] + values[((i + 1) * step) % values.size()]) * 0.5f;
        float blend = 0.5f;
        if (!level) {
            static float cornerWeight = 1.0f / (cosf(0.7853982f) * 2.0f + 1.0f);
            int firstType = types[(i * step) % types.size()] & VERTEX_KIND_MASK;
            if (firstType == VERTEX_ROUND &&
                (types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK) != VERTEX_ROUND)
                blend = cornerWeight;
            if ((types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK) == VERTEX_ROUND &&
                firstType != VERTEX_ROUND)
                blend = 1.0f - cornerWeight;
        }
        values[(i * step + step / 2) % values.size()] =
            values[(i * step) % values.size()] * blend + values[((i + 1) * step) % values.size()] * (1.0f - blend);
        if ((types[(i * step) % types.size()] & VERTEX_KIND_MASK) == VERTEX_BEZIER ||
            (types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK) == VERTEX_BEZIER ||
            (!level && ((types[(i * step) % types.size()] & VERTEX_KIND_MASK) == VERTEX_ROUND ||
                        (types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK) == VERTEX_ROUND))) {
            types[(i * step + step / 2) % types.size()] = VERTEX_BEZIER;
        } else {
            types[(i * step + step / 2) % types.size()] = VERTEX_SMOOTH;
        }
    }
    if (closed)
        ++segments;
    for (int i = 1; i < segments; ++i) {
        int type = types[(i * step) % types.size()] & VERTEX_KIND_MASK;
        if (type != VERTEX_SHARP && (type != VERTEX_ROUND || !level)) {
            if (type == VERTEX_SMOOTH) {
                values[(i * step) % values.size()] +=
                    ((values[(i * step - step / 2) % values.size()] + values[(i * step + step / 2) % values.size()]) *
                         0.5f -
                     values[(i * step) % values.size()]) *
                    weight;
            } else {
                values[(i * step) % values.size()] =
                    (values[(i * step - step / 2) % values.size()] + values[(i * step + step / 2) % values.size()]) *
                    0.5f;
            }
        }
    }
    for (int i = 0; i < segments; ++i) {
        int firstType = types[(i * step) % types.size()] & VERTEX_KIND_MASK;
        int secondType = types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK;
        if ((firstType == VERTEX_BEZIER || (!level && firstType == VERTEX_ROUND)) &&
            (secondType == VERTEX_BEZIER || (!level && secondType == VERTEX_ROUND))) {
            values[(i * step + step / 2) % values.size()] =
                (values[(i * step) % values.size()] + values[((i + 1) * step) % values.size()]) * 0.5f;
            types[(i * step + step / 2) % types.size()] = VERTEX_SHARP;
        }
    }
}

// Refines every vertex smoothly for extrusion front and back colours.
template<class T> static void refineContour(std::vector<T> &values, int step, int segments, bool closed, float weight)
{
    for (int i = 0; i < segments; ++i) {
        values[(i * step + step / 2) % values.size()] =
            (values[(i * step) % values.size()] + values[((i + 1) * step) % values.size()]) * 0.5f;
    }
    if (closed)
        ++segments;
    for (int i = 1; i < segments; ++i) {
        values[(i * step) % values.size()] +=
            ((values[(i * step - step / 2) % values.size()] + values[(i * step + step / 2) % values.size()]) * 0.5f -
             values[(i * step) % values.size()]) *
            weight;
    }
    for (int i = 0; i < segments; ++i) {
        values[(i * step + step / 2) % values.size()] =
            (values[(i * step) % values.size()] + values[((i + 1) * step) % values.size()]) * 0.5f;
    }
}

SmoothMeshShape::SmoothMeshShape(Engine *engine, const std::wstring &name)
    : MeshShape(engine, name),
      m_subdivisions(DefaultMeshSubdivisions),
      m_strokeSegments(DefaultMeshStrokeSegments),
      m_limitSubdivisions(false),
      m_updatingDrawings(false),
      m_strokeDash(1.0f),
      m_strokeGap(1.0f),
      m_strokePattern(STROKE_PATTERN_SOLID)
{
    float root = sqrt(2.0f);
    m_smoothWeight = (root - 1.0f) / (root * 0.5f);
    m_dirty = DRAWING_DIRTY_ALL;
    m_fillDrawing = nullptr;
    m_strokeDrawing = nullptr;
    m_extrusionDrawing = nullptr;
    m_type = SHAPE_SMOOTH_MESH;
}

SmoothMeshShape::~SmoothMeshShape()
{
    if (m_fillDrawing)
        m_engine->deleteDrawing(m_fillDrawing);
    if (m_strokeDrawing)
        m_engine->deleteDrawing(m_strokeDrawing);
    if (m_extrusionDrawing)
        m_engine->deleteDrawing(m_extrusionDrawing);
}

int SmoothMeshShape::getEffectiveSubdivisions()
{
    if (m_limitSubdivisions && m_subdivisions > 2) {
        int corners = 0;
        for (int i = 0; i < (int)m_faces.size(); ++i)
            corners += m_faces[i].size();
        // Zero corners and conversion overflow are left unchecked.
        int result = (int)(logf(AdaptiveSubdivisionCornerBudget / corners) / logf(4.0f)) + 1;
        if (result < m_subdivisions)
            return result;
    }
    return m_subdivisions;
}

void SmoothMeshShape::appendRoundCap(int contour, int vertex, float fraction, int atEnd, int segments)
{
    int next = (vertex + 2) % m_strokeOutlinePositions[contour].size();
    Vector<2, float> center =
        (m_strokeOutlinePositions[contour][vertex] + m_strokeOutlinePositions[contour][vertex + 1]) *
            (0.5f * (1.0f - fraction)) +
        (m_strokeOutlinePositions[contour][next] + m_strokeOutlinePositions[contour][next + 1]) * (fraction * 0.5f);
    Vector<2, float> outer = m_strokeOutlinePositions[contour][vertex] * (1.0f - fraction) +
                             m_strokeOutlinePositions[contour][next] * fraction;
    Vector<2, float> radius = outer - center;
    Vector<2, float> normal(radius[1], -radius[0]);
    Vector<2, float> texture(fraction * m_strokeOutlineTexCoords[contour][next][0] +
                                 (1.0f - fraction) * m_strokeOutlineTexCoords[contour][vertex][0],
                             0.5f);
    float length = sqrtf(normal[1] * normal[1] + normal[0] * normal[0]);
    const Vector<2, float> *textureCoordinates = &m_strokeOutlineTexCoords[contour][0];
    Vector<2, float> delta =
        (m_strokeOutlinePositions[contour][vertex] + m_strokeOutlinePositions[contour][vertex + 1]) * 0.5f -
        (m_strokeOutlinePositions[contour][next] + m_strokeOutlinePositions[contour][next + 1]) * 0.5f;
    float textureOffset = -((textureCoordinates[next][0] - textureCoordinates[vertex][0]) * length /
                            sqrtf(delta[0] * delta[0] + delta[1] * delta[1]));
    Vector<2, float> textureNormal(textureOffset, 0.0f);
    Vector<2, float> textureRadius(0.0f, -0.5f);
    Vector<4, float> color =
        m_strokeOutlineColors[contour][vertex] * (1.0f - fraction) + m_strokeOutlineColors[contour][next] * fraction;
    float startAngle = 0.0f;
    float quarter = 1.5707964f;
    if (atEnd)
        startAngle = (0.5f / segments + 1.0f) * quarter + startAngle;
    for (int i = 0; i < segments; ++i) {
        // The step uses a quarter turn in double precision.
        float angle = (i + 0.5f) * 1.5707963267948966 / (segments + 0.5f) + startAngle;
        float cosine = std::cos(angle);
        float sine = std::sin(angle);
        m_strokeDrawing->m_pathVertices.push_back(center + normal * cosine + radius * sine);
        m_strokeDrawing->m_pathVertices.push_back(center + normal * cosine - radius * sine);
        m_strokeDrawing->m_pathColors.push_back(color);
        m_strokeDrawing->m_pathColors.push_back(color);
        m_strokeDrawing->m_pathTextureCoordinates.push_back(texture + textureNormal * cosine + textureRadius * sine);
        m_strokeDrawing->m_pathTextureCoordinates.push_back(texture + textureNormal * cosine - textureRadius * sine);
    }
}

void SmoothMeshShape::appendSquareCap(int contour, int vertex, float fraction, int reverse)
{
    int next = (vertex + 2) % m_strokeOutlinePositions[contour].size();
    Vector<2, float> center =
        (m_strokeOutlinePositions[contour][vertex] + m_strokeOutlinePositions[contour][vertex + 1]) *
            (0.5f * (1.0f - fraction)) +
        (m_strokeOutlinePositions[contour][next] + m_strokeOutlinePositions[contour][next + 1]) * (fraction * 0.5f);
    Vector<2, float> outer = m_strokeOutlinePositions[contour][vertex] * (1.0f - fraction) +
                             m_strokeOutlinePositions[contour][next] * fraction;
    Vector<2, float> offset = outer - center;
    Vector<2, float> normal(offset[1], -offset[0]);
    float texture = fraction * m_strokeOutlineTexCoords[contour][next][0] +
                    (1.0f - fraction) * m_strokeOutlineTexCoords[contour][vertex][0];
    float radiusSquared = normal[1] * normal[1] + normal[0] * normal[0];
    float radius = std::sqrt(radiusSquared);
    const Vector<2, float> *textureCoordinates = &m_strokeOutlineTexCoords[contour][0];
    Vector<2, float> delta =
        (m_strokeOutlinePositions[contour][vertex] + m_strokeOutlinePositions[contour][vertex + 1]) * 0.5f -
        (m_strokeOutlinePositions[contour][next] + m_strokeOutlinePositions[contour][next + 1]) * 0.5f;
    float lengthSquared = delta[0] * delta[0] + delta[1] * delta[1];
    float textureOffset =
        (textureCoordinates[next][0] - textureCoordinates[vertex][0]) * -radius / std::sqrt(lengthSquared);
    if (reverse) {
        normal = normal * -1.0f;
        textureOffset = -textureOffset;
    }
    Vector<4, float> color =
        m_strokeOutlineColors[contour][vertex] * (1.0f - fraction) + m_strokeOutlineColors[contour][next] * fraction;
    m_strokeDrawing->m_pathVertices.push_back(center + normal + offset);
    m_strokeDrawing->m_pathVertices.push_back(center + normal - offset);
    m_strokeDrawing->m_pathColors.push_back(color);
    m_strokeDrawing->m_pathColors.push_back(color);
    m_strokeDrawing->m_pathTextureCoordinates.push_back(Vector<2, float>(textureOffset + texture, 0.0f));
    m_strokeDrawing->m_pathTextureCoordinates.push_back(Vector<2, float>(textureOffset + texture, 1.0f));
}

int SmoothMeshShape::appendStrokePair(int contour, int vertex, float fraction, bool first)
{
    int next = (vertex + 2) % m_strokeOutlinePositions[contour].size();
    m_strokeDrawing->m_pathVertices.push_back(m_strokeOutlinePositions[contour][vertex] * (1.0f - fraction) +
                                              m_strokeOutlinePositions[contour][next] * fraction);
    m_strokeDrawing->m_pathVertices.push_back(m_strokeOutlinePositions[contour][vertex + 1] * (1.0f - fraction) +
                                              m_strokeOutlinePositions[contour][next + 1] * fraction);
    m_strokeDrawing->m_pathColors.push_back(m_strokeOutlineColors[contour][vertex] * (1.0f - fraction) +
                                            m_strokeOutlineColors[contour][next] * fraction);
    m_strokeDrawing->m_pathColors.push_back(m_strokeOutlineColors[contour][vertex + 1] * (1.0f - fraction) +
                                            m_strokeOutlineColors[contour][next + 1] * fraction);
    m_strokeDrawing->m_pathTextureCoordinates.push_back(m_strokeOutlineTexCoords[contour][vertex] * (1.0f - fraction) +
                                                        m_strokeOutlineTexCoords[contour][next] * fraction);
    m_strokeDrawing->m_pathTextureCoordinates.push_back(m_strokeOutlineTexCoords[contour][vertex + 1] *
                                                            (1.0f - fraction) +
                                                        m_strokeOutlineTexCoords[contour][next + 1] * fraction);
    return 1;
}

void SmoothMeshShape::buildDashedStroke(int contour, float dash, float gap)
{
    if (m_strokeOutlinePositions[contour].empty())
        return;
    bool closed = !isOpen();
    int segments = m_contourCenters[contour].size();
    if (!closed)
        --segments;
    float totalDistance = 0.0f;
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % m_contourCenters[contour].size();
        Vector<2, float> delta = m_contourCenters[contour][i] - m_contourCenters[contour][next];
        float length = std::sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
        totalDistance += strokeDistance(length, m_contourWidths[contour][i], m_contourWidths[contour][next]);
    }
    int dashes = (int)(totalDistance / (dash + gap));
    if (!dashes)
        return;
    // Stretch the pattern so that a whole number of dashes fits.
    float scale = totalDistance / (dashes * (dash + gap));
    dash *= scale;
    gap *= scale;
    int capSegments = m_strokeSegments / 2;
    int segment = 0;
    float fraction = 0.0f;
    // advanceStroke supplies these outputs; the false path exits before use.
    int endSegment;
    float endFraction;
    for (int dashIndex = 0; dashIndex < dashes; ++dashIndex) {
        int first = m_strokeDrawing->m_pathVertices.size();
        int pairs = 0;
        if (!advanceStroke(contour, segment, fraction, dash, endSegment, endFraction))
            break;
        // This clamp is kept although the loop bound excludes dashIndex == dashes.
        if (dashIndex == dashes && endSegment >= segments) {
            endFraction = 1.0f;
            endSegment = segments - 1;
            if (endSegment < segment)
                break;
        }
        int start = (m_strokePairs[contour][(segment + 1) % m_strokePairs[contour].size()] - 2 +
                     m_strokeOutlinePositions[contour].size()) %
                    m_strokeOutlinePositions[contour].size();
        if (m_strokeCapType == STROKE_CAP_ROUND) {
            appendRoundCap(contour, start, fraction, 0, capSegments);
            pairs = capSegments;
        } else if (m_strokeCapType == STROKE_CAP_SQUARE) {
            appendSquareCap(contour, start, fraction, false);
            pairs = 1;
        }
        appendStrokePair(contour, start, fraction, true);
        ++pairs;
        int end = (m_strokePairs[contour][(endSegment + 1) % m_strokePairs[contour].size()] - 2 +
                   m_strokeOutlinePositions[contour].size()) %
                  m_strokeOutlinePositions[contour].size();
        if (segment < endSegment) {
            for (int i = m_strokePairs[contour][segment + 1]; i <= end; i += 2) {
                appendStrokePair(contour, i, 0.0f, false);
                ++pairs;
            }
        }
        fraction = endFraction;
        segment = endSegment;
        appendStrokePair(contour, end, endFraction, false);
        ++pairs;
        if (m_strokeCapType == STROKE_CAP_ROUND) {
            appendRoundCap(contour, end, fraction, 1, capSegments);
            pairs += capSegments;
        } else if (m_strokeCapType == STROKE_CAP_SQUARE) {
            appendSquareCap(contour, end, fraction, true);
            ++pairs;
        }
        advanceStroke(contour, segment, fraction, gap, endSegment, endFraction);
        fraction = endFraction;
        segment = endSegment;
        // Two triangles per pair of pairs.
        for (int i = 0; i < pairs - 1; ++i) {
            int vertex = first + 2 * i;
            m_strokeDrawing->m_pathIndices.push_back(vertex);
            m_strokeDrawing->m_pathIndices.push_back(vertex + 2);
            m_strokeDrawing->m_pathIndices.push_back(vertex + 1);
            m_strokeDrawing->m_pathIndices.push_back(vertex + 1);
            m_strokeDrawing->m_pathIndices.push_back(vertex + 2);
            m_strokeDrawing->m_pathIndices.push_back(vertex + 3);
        }
    }
}

void SmoothMeshShape::updateSubdivision()
{
    m_dirty = DRAWING_DIRTY_ALL;
    if (m_subdivisions < 1) {
        // This loop is kept although this branch runs it zero times.
        for (int i = 0; i < m_subdivisions; ++i) {
            m_levels[i].m_edges.clear();
            m_levels[i].m_faces.clear();
        }
        return;
    }
    m_vertexColors.resize(m_vertexPositions.length());
    m_vertexTexCoords.resize(m_vertexPositions.length());
    m_vertexFlags.resize(m_vertexPositions.length());
    m_vertexParameters.resize(m_vertexPositions.length());
    m_strokeColors.resize(m_vertexPositions.length());
    m_extrusionFrontColors.resize(m_vertexPositions.length());
    m_extrusionBackColors.resize(m_vertexPositions.length());
    m_strokeWidths.resize(m_vertexPositions.length());
    m_vertices.resize(m_vertexPositions.length());
    m_faceVertices.resize(m_faces.size());
    m_controlEdges.clear();
    typedef std::pair<int, int> EdgeKey;
    int edgeCount = 0;
    std::map<EdgeKey, int> controlEdges;
    for (int face = 0; face < (int)m_faces.size(); ++face) {
        std::vector<int> &vertices = m_faces[face];
        if (vertices.size() < 3)
            continue;
        for (int corner = 0; corner < (int)vertices.size(); ++corner) {
            if (controlEdges.find(EdgeKey(vertices[corner], vertices[(corner + 1) % vertices.size()])) ==
                controlEdges.end()) {
                // Both directions of the edge map to the same edge vertex.
                controlEdges[EdgeKey(vertices[corner], vertices[(corner + 1) % vertices.size()])] = edgeCount;
                controlEdges[EdgeKey(vertices[(corner + 1) % vertices.size()], vertices[corner])] = edgeCount;
                m_controlEdges.push_back(ControlEdge(face, corner, edgeCount++));
            }
        }
    }
    m_edgeVertices.resize(edgeCount);
    m_levels[0].m_faces.clear();
    m_adjacentFaces = m_faces;
    int corners = 0;
    for (int i = 0; i < (int)m_faces.size(); ++i) {
        if (m_faces[i].size() >= 3)
            corners += m_faces[i].size();
    }
    m_levels[0].m_faces.resize(corners);
    std::map<EdgeKey, int> reverseFaces;
    int quadIndex = 0;
    for (int face = 0; face < (int)m_faces.size(); ++face) {
        std::vector<int> &vertices = m_faces[face];
        if (vertices.size() < 3)
            continue;
        for (int corner = 0; corner < (int)vertices.size(); ++corner) {
            EdgeKey nextKey(vertices[corner], vertices[(corner + 1) % vertices.size()]);
            EdgeKey previousKey(vertices[corner], vertices[(vertices.size() + corner - 1) % vertices.size()]);
            QuadFace &quad = m_levels[0].m_faces[quadIndex++];
            quad.m_vertices[0] = &m_faceVertices[face];
            quad.m_vertices[1] = &m_edgeVertices[controlEdges[previousKey]];
            quad.m_vertices[2] = &m_vertices[vertices[corner]];
            quad.m_vertices[3] = &m_edgeVertices[controlEdges[nextKey]];
            quad.m_parent = nullptr;
            quad.m_unusedB8 = nullptr;
            reverseFaces[EdgeKey(vertices[(corner + 1) % vertices.size()], vertices[corner])] = face;
        }
    }
    for (int face = 0; face < (int)m_faces.size(); ++face) {
        std::vector<int> &vertices = m_faces[face];
        for (int corner = 0; corner < (int)vertices.size(); ++corner) {
            std::map<EdgeKey, int>::iterator adjacent =
                reverseFaces.find(EdgeKey(vertices[corner], vertices[(corner + 1) % vertices.size()]));
            if (adjacent == reverseFaces.end())
                m_adjacentFaces[face][corner] = -1;
            else
                m_adjacentFaces[face][corner] = adjacent->second;
        }
    }
    // One edge per quad corner (to the face centre) and two per control edge.
    int edgeTotal = edgeCount * 2;
    for (int i = 0; i < (int)m_faces.size(); ++i) {
        if (m_faces[i].size() >= 3)
            edgeTotal += m_faces[i].size();
    }
    m_levels[0].m_edges.resize(edgeTotal);
    typedef std::pair<QuadVertex *, QuadVertex *> VertexPair;
    std::map<VertexPair, QuadEdge *> edges;
    int edgeIndex = 0;
    for (int face = 0; face < (int)m_levels[0].m_faces.size(); ++face) {
        QuadFace &quad = m_levels[0].m_faces[face];
        for (int corner = 0; corner < 4; ++corner) {
            VertexPair key(quad.m_vertices[corner], quad.m_vertices[(corner + 1) % 4]);
            std::map<VertexPair, QuadEdge *>::iterator found = edges.find(key);
            if (found == edges.end()) {
                QuadEdge *edge = &m_levels[0].m_edges[edgeIndex];
                edge->m_faces[0] = &quad;
                edge->m_faces[1] = nullptr;
                edge->m_vertices[0] = key.first;
                edge->m_vertices[1] = key.second;
                edges[key] = edge;
                quad.m_edges[corner] = edge;
                // The key is stored a second time before the reverse key.
                edges[key] = edge;
                VertexPair reverseKey(quad.m_vertices[(corner + 1) % 4], quad.m_vertices[corner]);
                edges[reverseKey] = edge;
                ++edgeIndex;
            } else {
                found->second->m_faces[1] = &quad;
                quad.m_edges[corner] = found->second;
            }
        }
    }
    for (int i = 0; i < (int)m_levels[0].m_edges.size(); ++i) {
        if (!m_levels[0].m_edges[i].m_faces[1]) {
            m_levels[0].m_edges[i].m_vertices[0]->m_flags |= (1 << QUAD_BOUNDARY);
            m_levels[0].m_edges[i].m_vertices[1]->m_flags |= (1 << QUAD_BOUNDARY);
        }
    }
    m_levels[0].prepare();
    if (m_flags & (1 << MESH_CONTOUR)) {
        for (int i = 1; i < m_subdivisions; ++i) {
            m_levels[i].m_edges.clear();
            m_levels[i].m_faces.clear();
        }
    } else {
        for (int i = 0; i < m_subdivisions - 1; ++i)
            m_levels[i + 1].subdivide(m_levels[i]);
    }
}

Shape *SmoothMeshShape::clone()
{
    SmoothMeshShape *copy = m_engine->createSmoothMeshShape(m_name);
    copy->m_vertexPositions = m_vertexPositions;
    copy->m_vertexColors = m_vertexColors;
    copy->m_vertexTexCoords = m_vertexTexCoords;
    copy->m_strokeColors = m_strokeColors;
    copy->m_extrusionFrontColors = m_extrusionFrontColors;
    copy->m_extrusionBackColors = m_extrusionBackColors;
    copy->m_strokeWidths = m_strokeWidths;
    copy->m_texture = m_texture;
    copy->m_strokeTexture = m_strokeTexture;
    copy->m_textureTranslation = m_textureTranslation;
    copy->m_textureRotation = m_textureRotation;
    copy->m_texturePivot = m_texturePivot;
    copy->m_textureDeformation = m_textureDeformation;
    copy->m_textureOpacity = m_textureOpacity;
    copy->m_textureBrightness = m_textureBrightness;
    copy->m_textureContrast = m_textureContrast;
    copy->m_textureSaturation = m_textureSaturation;
    copy->m_strokeTextureOpacity = m_strokeTextureOpacity;
    copy->m_strokeTextureBrightness = m_strokeTextureBrightness;
    copy->m_strokeTextureContrast = m_strokeTextureContrast;
    copy->m_strokeTextureSaturation = m_strokeTextureSaturation;
    copy->m_strokeTextureStretch = m_strokeTextureStretch;
    copy->m_extrusionMatrix = m_extrusionMatrix;
    copy->m_subdivisions = m_subdivisions;
    copy->m_strokePattern = m_strokePattern;
    copy->m_strokeDash = m_strokeDash;
    copy->m_strokeGap = m_strokeGap;
    copy->m_vertexFlags = m_vertexFlags;
    copy->m_vertexParameters = m_vertexParameters;
    copy->m_flags = m_flags;
    copy->m_strokeCapType = m_strokeCapType;
    copy->m_strokeJointType = m_strokeJointType;
    copy->m_strokeAlignment = m_strokeAlignment;
    copy->m_smoothWeight = m_smoothWeight;
    copy->setFaces(m_faces, true);
    return copy;
}

StaticMeshShape *SmoothMeshShape::convertToStaticMeshShape()
{
    Drawing *fill = nullptr;
    Drawing *stroke = nullptr;
    Drawing *extrusion = nullptr;
    if (m_fillDrawing && isFilled()) {
        fill = m_fillDrawing;
        fill->reset();
        m_fillDrawing = nullptr;
    }
    if (m_strokeDrawing && isStroked()) {
        stroke = m_strokeDrawing;
        stroke->reset();
        m_strokeDrawing = nullptr;
    }
    if (m_extrusionDrawing && isExtruded()) {
        extrusion = m_extrusionDrawing;
        extrusion->reset();
        m_extrusionDrawing = nullptr;
    }
    return new StaticMeshShape(m_engine, fill, stroke, extrusion, m_name);
}

void SmoothMeshShape::draw(Node *node)
{
    if (m_extrusionDrawing && isExtruded())
        m_extrusionDrawing->draw(node, m_engine->m_extrusionColor);
    if (m_fillDrawing && isFilled())
        m_fillDrawing->draw(node, m_engine->m_fillColor);
    if (m_strokeDrawing && isStroked())
        m_strokeDrawing->draw(node, m_engine->m_outlineColor);
}

void SmoothMeshShape::shownChanged(bool shown)
{
    if (shown) {
        if (!m_fillDrawing) {
            m_fillDrawing = static_cast<Drawing *>(m_engine->createDrawing());
            m_strokeDrawing = static_cast<Drawing *>(m_engine->createDrawing());
            m_extrusionDrawing = static_cast<Drawing *>(m_engine->createDrawing());
            attributesChanged(true);
        }
    } else if (m_fillDrawing) {
        for (std::set<Node *>::iterator node = m_nodes.begin(); node != m_nodes.end(); ++node) {
            if ((*node)->isShown())
                return;
        }
    }
}

bool SmoothMeshShape::advanceStroke(int contour, int segment, float fraction, float distance, int &resultSegment,
                                    float &resultFraction)
{
    distance = std::max(0.0f, distance);
    int next = (segment + 1) % m_contourCenters[contour].size();
    Vector<2, float> delta = m_contourCenters[contour][segment] - m_contourCenters[contour][next];
    float length = sqrtf(delta[0] * delta[0] + delta[1] * delta[1]);
    float firstWidth = m_contourWidths[contour][segment];
    float secondWidth = m_contourWidths[contour][next];
    float partialLength = fraction * length;
    float width = (1.0f - fraction) * firstWidth + fraction * secondWidth;
    float advance = strokeDistance(partialLength, firstWidth, width) + distance;
    float divisor =
        std::max(MinimumStrokeProjectionDivisor, 1.0f - (secondWidth - firstWidth) * advance / (length * 2.0f));
    float offset = advance * firstWidth / divisor;
    if (offset <= length) {
        resultSegment = segment;
        resultFraction = offset / length;
        return true;
    }
    float remaining = length - partialLength;
    // The recursion comes before the test of the output segment.
    bool advanced = advanceStroke(contour, next, 0.0f, distance - strokeDistance(remaining, firstWidth, secondWidth),
                                  resultSegment, resultFraction);
    if (resultSegment < segment)
        return false;
    return advanced;
}

void SmoothMeshShape::releaseCache()
{
    // m_contourTypes is kept.
    for (int i = 0; i < 6; ++i)
        m_levels[i].clear();
    m_vertices.clear();
    m_edgeVertices.clear();
    m_faceVertices.clear();
    m_adjacentFaces.clear();
    m_controlEdges.clear();
    m_contourColors.clear();
    m_contourStrokeColors.clear();
    m_contourFrontColors.clear();
    m_contourBackColors.clear();
    m_contourWidths.clear();
    m_contourCenters.clear();
    m_contourTexCoords.clear();
    m_strokePairs.clear();
    m_strokeOutlinePositions.clear();
    m_strokeOutlineColors.clear();
    m_strokeOutlineTexCoords.clear();
    m_boundaryEdges.clear();
    if (m_fillDrawing)
        m_fillDrawing->reset();
    if (m_strokeDrawing)
        m_strokeDrawing->reset();
    if (m_extrusionDrawing)
        m_extrusionDrawing->reset();
}

bool SmoothMeshShape::getTransformedBounds(Vector<2, float> &minimum, Vector<2, float> &maximum,
                                           const Matrix<float> &matrix, bool &first)
{
    if (isFilled() && m_fillDrawing)
        m_fillDrawing->getTransformedBounds(minimum, maximum, matrix, first);
    if (isStroked() && m_strokeDrawing)
        m_strokeDrawing->getTransformedBounds(minimum, maximum, matrix, first);
    if (isExtruded() && m_extrusionDrawing)
        m_extrusionDrawing->getTransformedBounds(minimum, maximum, matrix, first);
    return true;
}

bool SmoothMeshShape::isCacheEmpty()
{
    return m_vertices.empty();
}

bool SmoothMeshShape::contains(const Vector<2, float> &point)
{
    if (isFilled() && m_fillDrawing && m_fillDrawing->contains(point))
        return true;
    if (isStroked() && m_strokeDrawing && m_strokeDrawing->contains(point))
        return true;
    if (isExtruded() && m_extrusionDrawing && m_extrusionDrawing->contains(point))
        return true;
    return false;
}

bool SmoothMeshShape::intersects(const Vector<2, float> &point, float radius, const Matrix<float> &drawingMatrix,
                                 const Matrix<float> &pointMatrix)
{
    if (isFilled() && m_fillDrawing && m_fillDrawing->intersects(point, radius, drawingMatrix, pointMatrix))
        return true;
    if (isStroked() && m_strokeDrawing && m_strokeDrawing->intersects(point, radius, drawingMatrix, pointMatrix))
        return true;
    if (isExtruded() && m_extrusionDrawing && m_extrusionDrawing->intersects(point, radius, drawingMatrix, pointMatrix))
        return true;
    return false;
}

void SmoothMeshShape::setFaces(const std::vector<std::vector<int>> &faces, bool update)
{
    MeshShape::setFaces(faces);
    if (update) {
        updateSubdivision();
        attributesChanged(true);
    }
}

void SmoothMeshShape::setSubdivisions(int count, bool update)
{
    m_subdivisions = count;
    if (count < 0)
        m_subdivisions = 0;
    if (m_subdivisions > MaxMeshSubdivisions)
        m_subdivisions = MaxMeshSubdivisions;
    if (update) {
        updateSubdivision();
        attributesChanged(true);
    }
}

void SmoothMeshShape::updateSubdivisionDrawings(int positionElement, int colorElement, int textureElement)
{
    int lastLevel = getEffectiveSubdivisions() - 1;
    if (!m_fillDrawing)
        return;
    if (lastLevel < 0 || m_faces.empty() || m_vertices.empty()) {
        m_fillDrawing->m_pathVertices.clear();
        m_fillDrawing->m_pathColors.clear();
        m_fillDrawing->m_pathTextureCoordinates.clear();
        m_fillDrawing->m_pathIndices.clear();
        return;
    }
    if (m_flags & (1 << MESH_CONTOUR))
        return;
    if (!isFilled())
        return;
    m_fillDrawing->m_pathVertices.resize(m_levels[lastLevel].m_vertices.size());
    m_fillDrawing->m_pathColors.resize(m_fillDrawing->m_pathVertices.size());
    m_fillDrawing->m_pathTextureCoordinates.resize(m_fillDrawing->m_pathVertices.size());
    m_fillDrawing->m_pathIndices.resize(m_levels[lastLevel].m_faces.size() * 6);
    for (int i = 0; i < (int)m_vertices.size(); ++i) {
        m_vertices[i].m_position = nullptr;
        m_vertices[i].m_texturePosition = nullptr;
        m_vertices[i].m_color = nullptr;
    }
    for (int i = 0; i < (int)m_fillDrawing->m_pathVertices.size(); ++i) {
        m_levels[lastLevel].m_vertices[i]->m_position = &m_fillDrawing->m_pathVertices[i];
        m_levels[lastLevel].m_vertices[i]->m_texturePosition = &m_fillDrawing->m_pathTextureCoordinates[i];
        m_levels[lastLevel].m_vertices[i]->m_color = &m_fillDrawing->m_pathColors[i];
    }
    for (int i = 0; i < (int)m_levels[lastLevel].m_faces.size(); ++i) {
        QuadFace &face = m_levels[lastLevel].m_faces[i];
        m_fillDrawing->m_pathIndices[6 * i] = face.m_indices[0];
        m_fillDrawing->m_pathIndices[6 * i + 1] = face.m_indices[1];
        m_fillDrawing->m_pathIndices[6 * i + 2] = face.m_indices[2];
        m_fillDrawing->m_pathIndices[6 * i + 3] = face.m_indices[2];
        m_fillDrawing->m_pathIndices[6 * i + 4] = face.m_indices[3];
        m_fillDrawing->m_pathIndices[6 * i + 5] = face.m_indices[0];
    }
    for (int i = 0; i < (int)m_vertices.size(); ++i) {
        QuadVertex &vertex = m_vertices[i];
        vertex.m_positionSum = Vector<2, float>(0.0f, 0.0f);
        vertex.m_colorSum = Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
        vertex.m_textureSum = Vector<2, float>(0.0f, 0.0f);
        vertex.m_count = 0;
        vertex.m_flags = 0;
    }
    for (int face = 0; face < (int)m_faces.size(); ++face) {
        std::vector<int> &corners = m_faces[face];
        for (int corner = 0; corner < (int)corners.size(); ++corner) {
            if (m_adjacentFaces[face][corner] < 0) {
                m_vertices[corners[corner]].m_flags |= (1 << QUAD_BOUNDARY);
                m_vertices[corners[(corner + 1) % corners.size()]].m_flags |= (1 << QUAD_BOUNDARY);
            }
        }
    }
    for (int i = 0; i < (int)m_vertices.size(); ++i) {
        QuadVertex &vertex = m_vertices[i];
        if (vertex.m_position) {
            *vertex.m_position = (*m_vertexPositions.m_values[positionElement])[i];
            *vertex.m_color = (*m_vertexColors.m_values[colorElement])[i];
            *vertex.m_texturePosition = (*m_vertexTexCoords.m_values[textureElement])[i];
            if (i < (int)m_vertexFlags.size()) {
                if ((m_vertexFlags[i] & VERTEX_KIND_MASK) == VERTEX_SHARP)
                    vertex.m_flags |= (1 << QUAD_SHARP);
                if ((m_vertexFlags[i] & VERTEX_KIND_MASK) == VERTEX_BEZIER)
                    vertex.m_flags |= (1 << QUAD_BEZIER);
                if ((m_vertexFlags[i] & VERTEX_KIND_MASK) == VERTEX_ROUND)
                    vertex.m_flags |= (1 << QUAD_BEZIER) | (1 << QUAD_ROUND);
            }
        }
    }
    for (int face = 0; face < (int)m_faces.size(); ++face) {
        if (m_faces[face].size() >= 3) {
            Vector<2, float> position(0.0f, 0.0f);
            Vector<4, float> color(0.0f, 0.0f, 0.0f, 0.0f);
            Vector<2, float> texture(0.0f, 0.0f);
            int corners = 0;
            for (int corner = 0; corner < (int)m_faces[face].size(); ++corner) {
                QuadVertex &vertex = m_vertices[m_faces[face][corner]];
                position += *vertex.m_position;
                color += *vertex.m_color;
                texture += *vertex.m_texturePosition;
                ++corners;
            }
            float scale = 1.0f / corners;
            *m_faceVertices[face].m_position = position * scale;
            *m_faceVertices[face].m_color = color * scale;
            *m_faceVertices[face].m_texturePosition = texture * scale;
        }
    }
    float vertexWeight = 1.0f - m_smoothWeight;
    float faceWeight = m_smoothWeight;
    for (int i = 0; i < (int)m_controlEdges.size(); ++i) {
        ControlEdge &control = m_controlEdges[i];
        QuadVertex &edge = m_edgeVertices[control.m_elements[2]];
        QuadVertex &first = m_vertices[m_faces[control.m_elements[0]][control.m_elements[1]]];
        QuadVertex &second = m_vertices[m_faces[control.m_elements[0]]
                                               [(control.m_elements[1] + 1) % m_faces[control.m_elements[0]].size()]];
        edge.m_flags &= ~((1 << QUAD_SHARP) | (1 << QUAD_BEZIER) | (1 << QUAD_ROUND));
        int adjacent = m_adjacentFaces[control.m_elements[0]][control.m_elements[1]];
        if (adjacent < 0 || (first.m_flags & (1 << QUAD_SHARP)) || (second.m_flags & (1 << QUAD_SHARP))) {
            float blend = 0.5f;
            static float cornerWeight = 1.0f / (cosf(0.7853982f) * 2.0f + 1.0f);
            if ((first.m_flags & (1 << QUAD_ROUND)) && !(second.m_flags & (1 << QUAD_ROUND)))
                blend = cornerWeight;
            if ((second.m_flags & (1 << QUAD_ROUND)) && !(first.m_flags & (1 << QUAD_ROUND)))
                blend = 1.0f - cornerWeight;
            *edge.m_position = *first.m_position * blend + *second.m_position * (1.0f - blend);
            *edge.m_color = *first.m_color * blend + *second.m_color * (1.0f - blend);
            *edge.m_texturePosition = *first.m_texturePosition * blend + *second.m_texturePosition * (1.0f - blend);
            if ((first.m_flags & (1 << QUAD_BEZIER)) || (second.m_flags & (1 << QUAD_BEZIER)))
                edge.m_flags |= (1 << QUAD_BEZIER);
        } else {
            QuadVertex &firstFace = m_faceVertices[control.m_elements[0]];
            QuadVertex &secondFace = m_faceVertices[adjacent];
            *edge.m_position = ((*first.m_position + *second.m_position) * vertexWeight +
                                (*firstFace.m_position + *secondFace.m_position) * faceWeight) *
                               0.5f;
            *edge.m_color = ((*firstFace.m_color + *secondFace.m_color) * faceWeight +
                             (*first.m_color + *second.m_color) * vertexWeight) *
                            0.5f;
            *edge.m_texturePosition = ((*first.m_texturePosition + *second.m_texturePosition) * vertexWeight +
                                       (*firstFace.m_texturePosition + *secondFace.m_texturePosition) * faceWeight) *
                                      0.5f;
        }
    }
    for (int face = 0; face < (int)m_faces.size(); ++face) {
        if (m_faces[face].size() >= 3) {
            for (int corner = 0; corner < (int)m_faces[face].size(); ++corner) {
                QuadVertex &first = m_vertices[m_faces[face][corner]];
                QuadVertex &second = m_vertices[m_faces[face][(corner + 1) % m_faces[face].size()]];
                float blend = 0.5f;
                static float cornerWeight = 1.0f / (cosf(0.7853982f) * 2.0f + 1.0f);
                if ((first.m_flags & (1 << QUAD_ROUND)) && !(second.m_flags & (1 << QUAD_ROUND)))
                    blend = cornerWeight;
                if ((second.m_flags & (1 << QUAD_ROUND)) && !(first.m_flags & (1 << QUAD_ROUND)))
                    blend = 1.0f - cornerWeight;
                Vector<2, float> position = *first.m_position * blend + *second.m_position * (1.0f - blend);
                Vector<4, float> color = *first.m_color * blend + *second.m_color * (1.0f - blend);
                Vector<2, float> texture =
                    *first.m_texturePosition * blend + *second.m_texturePosition * (1.0f - blend);
                if (m_adjacentFaces[face][corner] < 0 || !(first.m_flags & (1 << QUAD_BOUNDARY))) {
                    first.m_positionSum += position;
                    first.m_colorSum += color;
                    first.m_textureSum += texture;
                    ++first.m_count;
                }
                if (m_adjacentFaces[face][corner] < 0 || !(second.m_flags & (1 << QUAD_BOUNDARY))) {
                    second.m_positionSum += position;
                    second.m_colorSum += color;
                    second.m_textureSum += texture;
                    ++second.m_count;
                }
            }
        }
    }
    for (int face = 0; face < (int)m_faces.size(); ++face) {
        for (int corner = 0; corner < (int)m_faces[face].size(); ++corner) {
            QuadVertex &vertex = m_vertices[m_faces[face][corner]];
            if (!(vertex.m_flags & (1 << QUAD_BOUNDARY))) {
                vertex.m_positionSum += *m_faceVertices[face].m_position;
                vertex.m_colorSum += *m_faceVertices[face].m_color;
                vertex.m_textureSum += *m_faceVertices[face].m_texturePosition;
                ++vertex.m_count;
            }
        }
    }
    for (int i = 0; i < (int)m_vertices.size(); ++i) {
        QuadVertex &vertex = m_vertices[i];
        if ((vertex.m_flags & (1 << QUAD_SHARP)) || !vertex.m_count)
            continue;
        float scale = 1.0f / vertex.m_count;
        if (vertex.m_flags & (1 << QUAD_BOUNDARY)) {
            if (vertex.m_flags & (1 << QUAD_BEZIER)) {
                *vertex.m_position = vertex.m_positionSum * scale;
                *vertex.m_color = vertex.m_colorSum * scale;
                *vertex.m_texturePosition = vertex.m_textureSum * scale;
            } else {
                *vertex.m_position = *vertex.m_position * vertexWeight + vertex.m_positionSum * faceWeight * scale;
                *vertex.m_color = vertex.m_colorSum * faceWeight * scale + *vertex.m_color * vertexWeight;
                *vertex.m_texturePosition =
                    *vertex.m_texturePosition * vertexWeight + vertex.m_textureSum * faceWeight * scale;
            }
        } else {
            *vertex.m_position = *vertex.m_position * 0.25f + vertex.m_positionSum * 0.75f * scale;
            *vertex.m_color = *vertex.m_color * 0.25f + vertex.m_colorSum * 0.75f * scale;
            *vertex.m_texturePosition = *vertex.m_texturePosition * 0.25f + vertex.m_textureSum * 0.75f * scale;
        }
    }
    for (int i = 0; i < (int)m_controlEdges.size(); ++i) {
        ControlEdge &control = m_controlEdges[i];
        std::vector<int> &face = m_faces[control.m_elements[0]];
        QuadVertex &first = m_vertices[face[control.m_elements[1]]];
        QuadVertex &second = m_vertices[face[(control.m_elements[1] + 1) % face.size()]];
        QuadVertex &edge = m_edgeVertices[control.m_elements[2]];
        if (m_adjacentFaces[control.m_elements[0]][control.m_elements[1]] < 0 && (first.m_flags & (1 << QUAD_BEZIER)) &&
            (second.m_flags & (1 << QUAD_BEZIER))) {
            *edge.m_position = (*first.m_position + *second.m_position) * 0.5f;
            *edge.m_color = (*first.m_color + *second.m_color) * 0.5f;
            *edge.m_texturePosition = (*first.m_texturePosition + *second.m_texturePosition) * 0.5f;
            edge.m_flags = (edge.m_flags & ~((1 << QUAD_SHARP) | (1 << QUAD_BEZIER))) | (1 << QUAD_SHARP);
        }
    }
    for (int i = 0; i < (int)m_vertices.size(); ++i) {
        if (m_vertices[i].m_flags & (1 << QUAD_ROUND))
            m_vertices[i].m_flags =
                (m_vertices[i].m_flags & ~((1 << QUAD_SHARP) | (1 << QUAD_BEZIER) | (1 << QUAD_ROUND))) |
                (1 << QUAD_SHARP);
    }
    lastLevel = getEffectiveSubdivisions() - 1;
    for (int i = 0; i < lastLevel; ++i)
        m_levels[i].update(m_smoothWeight);
}

void SmoothMeshShape::attributesChanged(bool all)
{
    bool filled = isFilled();
    if (filled && !m_fillDrawing) {
        m_fillDrawing = static_cast<Drawing *>(m_engine->createDrawing());
    } else if (!filled && m_fillDrawing) {
        m_engine->deleteDrawing(m_fillDrawing);
        m_fillDrawing = nullptr;
    }
    bool stroked = isStroked();
    if (stroked && !m_strokeDrawing) {
        m_strokeDrawing = static_cast<Drawing *>(m_engine->createDrawing());
    } else if (!stroked && m_strokeDrawing) {
        m_engine->deleteDrawing(m_strokeDrawing);
        m_strokeDrawing = nullptr;
    }
    bool extruded = (m_flags >> MESH_EXTRUDED) & 1; // read like MeshShape's mode accessors
    if (extruded && !m_extrusionDrawing) {
        m_extrusionDrawing = static_cast<Drawing *>(m_engine->createDrawing());
    } else if (!extruded && m_extrusionDrawing) {
        m_engine->deleteDrawing(m_extrusionDrawing);
        m_extrusionDrawing = nullptr;
    }
    if (all) {
        m_dirty = DRAWING_DIRTY_ALL;
    } else {
        if (m_vertexPositions.m_changed) {
            m_dirty |= (1 << DRAWING_DIRTY_VERTICES);
            m_vertexPositions.m_changed = false;
        }
        if (m_vertexColors.m_changed) {
            m_dirty |= (1 << DRAWING_DIRTY_COLORS);
            m_vertexColors.m_changed = false;
        }
        if (m_vertexTexCoords.m_changed) {
            m_dirty |= (1 << DRAWING_DIRTY_TEXCOORDS);
            m_vertexTexCoords.m_changed = false;
        }
        if (m_strokeWidths.m_changed) {
            m_dirty |= (1 << DRAWING_DIRTY_VERTICES);
            m_strokeWidths.m_changed = false;
        }
        if (m_strokeColors.m_changed) {
            m_dirty |= (1 << DRAWING_DIRTY_COLORS);
            m_strokeColors.m_changed = false;
        }
        if (m_extrusionFrontColors.m_changed) {
            m_dirty |= (1 << DRAWING_DIRTY_COLORS);
            m_extrusionFrontColors.m_changed = false;
        }
        if (m_extrusionBackColors.m_changed) {
            m_dirty |= (1 << DRAWING_DIRTY_COLORS);
            m_extrusionBackColors.m_changed = false;
        }
    }
    if (m_extrusionFrontColors.length() != m_vertexPositions.length())
        m_extrusionFrontColors.resize(m_vertexPositions.length());
    if (m_extrusionBackColors.length() != m_vertexPositions.length())
        m_extrusionBackColors.resize(m_vertexPositions.length());
    int lastLevel = getEffectiveSubdivisions() - 1;
    m_dirty |= (1 << DRAWING_DIRTY_VERTICES);
    applyVertexParameters();
    if (lastLevel >= 0) {
        updateTextureMatrix();
        if (m_texture.value() == -1)
            m_fillTexture = nullptr;
        else if (!m_fillTexture || m_fillTexture->m_id != m_texture.value())
            m_fillTexture = m_engine->findTexture(m_texture.value());
        if (m_strokeTexture.value() == -1)
            m_outlineTexture = nullptr;
        else if (!m_outlineTexture || m_outlineTexture->m_id != m_strokeTexture.value())
            m_outlineTexture = m_engine->findTexture(m_strokeTexture.value());
        if (isFilled() && !(m_flags & (1 << MESH_CONTOUR)))
            updateSubdivisionDrawings(m_vertexPositions.m_current, m_vertexColors.m_current,
                                      m_vertexTexCoords.m_current);
    } else {
        MeshShape::updateBoundsAndTextures();
    }
    m_contourCenters.resize(m_contours.size());
    m_contourColors.resize(m_contours.size());
    m_contourStrokeColors.resize(m_contours.size());
    m_contourFrontColors.resize(m_contours.size());
    m_contourBackColors.resize(m_contours.size());
    m_contourWidths.resize(m_contours.size());
    m_contourTexCoords.resize(m_contours.size());
    m_contourTypes.resize(m_contours.size());
    if (isStroked()) {
        m_strokeOutlinePositions.resize(m_contours.size());
        m_strokeOutlineColors.resize(m_contours.size());
        m_strokeOutlineTexCoords.resize(m_contours.size());
        m_strokePairs.resize(m_contours.size());
    } else {
        m_strokeOutlinePositions.clear();
        m_strokeOutlineColors.clear();
        m_strokeOutlineTexCoords.clear();
        m_strokePairs.clear();
    }
    bool closed = !isOpen();
    int subdivisions = getEffectiveSubdivisions();
    int step = (int)pow(2.0, subdivisions);
    for (int contour = 0; contour < (int)m_contours.size(); ++contour) {
        int count = !isOpen() ? step * m_contours[contour].size() : step * (m_contours[contour].size() - 1) + 1;
        m_contourCenters[contour].resize(count);
        m_contourColors[contour].resize(count);
        m_contourStrokeColors[contour].resize(count);
        m_contourFrontColors[contour].resize(count);
        m_contourBackColors[contour].resize(count);
        m_contourWidths[contour].resize(count);
        m_contourTexCoords[contour].resize(count);
        if (isStroked()) {
            m_strokePairs[contour].resize(isOpen() ? count : count + 1);
            m_strokeOutlinePositions[contour].clear();
            m_strokeOutlineColors[contour].clear();
            m_strokeOutlineTexCoords[contour].clear();
        }
        m_contourTypes[contour].resize(count, 0);
    }
    for (int contour = 0; contour < (int)m_contours.size(); ++contour) {
        for (int i = 0; i < (int)m_contours[contour].size(); ++i) {
            int vertex = m_contours[contour][i];
            int destination = i * step;
            m_contourCenters[contour][destination] = m_vertexPositions.value()[vertex];
            m_contourColors[contour][destination] = m_vertexColors.value()[vertex];
            m_contourTexCoords[contour][destination] = m_vertexTexCoords.value()[vertex];
            m_contourTypes[contour][destination] = m_vertexFlags[vertex];
            if (isStroked()) {
                m_contourStrokeColors[contour][destination] = m_strokeColors.value()[vertex];
                m_contourWidths[contour][destination] = m_strokeWidths.value()[vertex];
                if (m_contourWidths[contour][destination] < MinimumContourStrokeWidth)
                    m_contourWidths[contour][destination] = MinimumContourStrokeWidth;
            }
            if (isExtruded()) {
                m_contourFrontColors[contour][destination] = m_extrusionFrontColors.value()[vertex];
                m_contourBackColors[contour][destination] = m_extrusionBackColors.value()[vertex];
            }
        }
        for (int level = 0; level < subdivisions; ++level) {
            int stride = (int)pow(2.0, subdivisions - level);
            int segments = m_contourCenters[contour].size() / stride;
            refineContour(m_contourCenters[contour], m_contourTypes[contour], stride, segments, closed, m_smoothWeight,
                          level);
            refineContour(m_contourTexCoords[contour], m_contourTypes[contour], stride, segments, closed,
                          m_smoothWeight, level);
            refineContour(m_contourColors[contour], m_contourTypes[contour], stride, segments, closed, m_smoothWeight,
                          level);
            if (isStroked()) {
                refineContour(m_contourStrokeColors[contour], m_contourTypes[contour], stride, segments, closed,
                              m_smoothWeight, level);
                refineContour(m_contourWidths[contour], m_contourTypes[contour], stride, segments, closed,
                              m_smoothWeight, level);
            }
            if (isExtruded()) {
                refineContour(m_contourFrontColors[contour], stride, segments, closed, m_smoothWeight);
                refineContour(m_contourBackColors[contour], stride, segments, closed, m_smoothWeight);
            }
        }
        if (!isStroked())
            continue;
        // The stroke outline: a pair of positions (left and right of the centre line) per sample, more at corners; the
        // texture coordinate runs along the stroke in units of its width.
        float distance = 0.0f;
        int samples = !isOpen() ? m_contourCenters[contour].size() + 1 : m_contourCenters[contour].size();
        for (int i = 0; i < (int)m_contourCenters[contour].size(); ++i) {
        }
        float alignment = (float)m_strokeAlignment;
        for (int i = 0; i < samples; ++i) {
            int vertex = i % m_contourCenters[contour].size();
            m_strokePairs[contour][i] = m_strokeOutlinePositions[contour].size();
            if (isOpen() && vertex == 0) {
                // The start of an open stroke: square to the first segment.
                Vector<2, float> tangent = m_contourCenters[contour][0] - m_contourCenters[contour][1];
                if (tangent[0] * tangent[0] + tangent[1] * tangent[1] == 0.0f) {
                    int next = step * ((step + 1) / step) % m_contourCenters[contour].size();
                    tangent = m_contourCenters[contour][0] - m_contourCenters[contour][next];
                }
                Vector<2, float> normal(-tangent[1], tangent[0]);
                normal = normal * (1.0f / sqrtf(normal[0] * normal[0] + normal[1] * normal[1]));
                normal = normal * (m_contourWidths[contour][0] * 0.5f);
                Vector<2, float> offset = normal * alignment;
                m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][0] + normal + offset);
                m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][0] - normal + offset);
                m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][0]);
                m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][0]);
                m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 0.0f));
                m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 1.0f));
            } else if (isOpen() && vertex == m_contourCenters[contour].size() - 1) {
                // The end of an open stroke: square to the last segment.
                Vector<2, float> tangent = m_contourCenters[contour][vertex - 1] - m_contourCenters[contour][vertex];
                if (tangent[0] * tangent[0] + tangent[1] * tangent[1] == 0.0f) {
                    int previous = step * ((vertex - step - 1 + m_contourCenters[contour].size()) / step) %
                                   m_contourCenters[contour].size();
                    tangent = m_contourCenters[contour][previous] - m_contourCenters[contour][vertex];
                }
                Vector<2, float> normal(-tangent[1], tangent[0]);
                normal = normal * (1.0f / sqrtf(normal[0] * normal[0] + normal[1] * normal[1]));
                normal = normal * (m_contourWidths[contour][vertex] * 0.5f);
                Vector<2, float> offset = normal * alignment;
                m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] + normal + offset);
                m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] - normal + offset);
                m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 0.0f));
                m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 1.0f));
            } else {
                int previous = (vertex + m_contourCenters[contour].size() - 1) % m_contourCenters[contour].size();
                int next = (vertex + 1) % m_contourCenters[contour].size();
                Vector<2, float> incoming = m_contourCenters[contour][previous] - m_contourCenters[contour][vertex];
                Vector<2, float> outgoing = m_contourCenters[contour][vertex] - m_contourCenters[contour][next];
                // Coincident neighbours: use the nearest control vertex instead.
                if (incoming[0] * incoming[0] + incoming[1] * incoming[1] == 0.0f) {
                    previous = step * ((vertex - step - 1 + m_contourCenters[contour].size()) / step) %
                               m_contourCenters[contour].size();
                    incoming = m_contourCenters[contour][previous] - m_contourCenters[contour][vertex];
                }
                if (outgoing[0] * outgoing[0] + outgoing[1] * outgoing[1] == 0.0f) {
                    next = step * ((vertex + step + 1) / step) % m_contourCenters[contour].size();
                    outgoing = m_contourCenters[contour][vertex] - m_contourCenters[contour][next];
                }
                bool missingIncoming = true;
                Vector<2, float> firstNormal(-incoming[1], incoming[0]);
                Vector<2, float> secondNormal(-outgoing[1], outgoing[0]);
                float lengthSquared = firstNormal[0] * firstNormal[0] + firstNormal[1] * firstNormal[1];
                if (lengthSquared > 0.0f) {
                    missingIncoming = false;
                    firstNormal = firstNormal * (1.0f / sqrtf(lengthSquared));
                }
                lengthSquared = secondNormal[0] * secondNormal[0] + secondNormal[1] * secondNormal[1];
                if (lengthSquared > 0.0f)
                    secondNormal = secondNormal * (1.0f / sqrtf(lengthSquared));
                Vector<2, float> bisector = secondNormal + firstNormal;
                lengthSquared = bisector[0] * bisector[0] + bisector[1] * bisector[1];
                if (lengthSquared > 0.0f) {
                    bisector = bisector * (1.0f / sqrtf(lengthSquared));
                    if (!(vertex % step) && (m_contourTypes[contour][vertex] & VERTEX_KIND_MASK) == VERTEX_SHARP) {
                        // A mitre: lengthen the bisector, up to a limit.
                        float projection = missingIncoming ? bisector.dot(secondNormal) : bisector.dot(firstNormal);
                        float minimumProjection = MinimumCornerNormalProjection;
                        float divisor = std::max(projection, minimumProjection);
                        bisector = bisector / divisor;
                    }
                } else {
                    bisector = Vector<2, float>(0.0f, 0.0f);
                }
                float halfWidth = m_contourWidths[contour][vertex] * 0.5f;
                bisector = bisector * halfWidth;
                Vector<2, float> offset = bisector * alignment;
                if (vertex % step || (m_contourTypes[contour][vertex] & VERTEX_KIND_MASK) != VERTEX_SHARP) {
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] + bisector + offset);
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] - bisector + offset);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 0.0f));
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 1.0f));
                } else if (secondNormal.dot(firstNormal) >= NearlyParallelCornerNormalCosine) {
                    // A nearly straight corner.
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] + bisector + offset);
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] - bisector + offset);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 0.0f));
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 1.0f));
                } else if (m_strokeJointType == STROKE_JOINT_ROUND) {
                    float direction = bisector.dot(outgoing);
                    float angle = acosf(secondNormal.dot(firstNormal)) * 57.29577951308232;
                    bool reverse = direction < 0.0f;
                    int segments = (int)((float)fabs(angle) / 180.0f * CornerSegmentsPerHalfTurn);
                    if (reverse)
                        angle = -angle;
                    angle = angle / segments;
                    firstNormal = firstNormal * halfWidth;
                    secondNormal = secondNormal * halfWidth;
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] + firstNormal +
                                                                offset);
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] - firstNormal +
                                                                offset);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 0.0f));
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 1.0f));
                    Matrix<float> rotation;
                    rotation.identity();
                    rotation.rotateZ(angle);
                    Vector<2, float> outer = firstNormal;
                    Vector<2, float> inner = firstNormal;
                    if (reverse)
                        outer = (secondNormal + firstNormal) * 0.5f;
                    else
                        inner = (secondNormal + firstNormal) * 0.5f;
                    for (int segment = 1; segment < segments; ++segment) {
                        if (reverse)
                            inner.transform(rotation);
                        else
                            outer.transform(rotation);
                        m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] + outer + offset);
                        m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] - inner + offset);
                        m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                        m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                        m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 0.0f));
                        m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 1.0f));
                    }
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] + secondNormal +
                                                                offset);
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] - secondNormal +
                                                                offset);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 0.0f));
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 1.0f));
                } else {
                    firstNormal = firstNormal * halfWidth;
                    secondNormal = secondNormal * halfWidth;
                    Vector<2, float> outer;
                    Vector<2, float> inner;
                    if (bisector.dot(outgoing) >= 0.0f) {
                        outer = bisector;
                        inner = (secondNormal + firstNormal) * 0.5f;
                    } else {
                        outer = (secondNormal + firstNormal) * 0.5f;
                        inner = bisector;
                    }
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] + firstNormal +
                                                                offset);
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] - firstNormal +
                                                                offset);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 0.0f));
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 1.0f));
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] + outer + offset);
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] - inner + offset);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 0.0f));
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 1.0f));
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] + secondNormal +
                                                                offset);
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] - secondNormal +
                                                                offset);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 0.0f));
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 1.0f));
                }
            }
            Vector<2, float> delta = m_contourCenters[contour][vertex] -
                                     m_contourCenters[contour][(vertex + 1) % m_contourCenters[contour].size()];
            float length = delta.length();
            distance += strokeDistance(length, m_contourWidths[contour][vertex],
                                       m_contourWidths[contour][(vertex + 1) % m_contourWidths[contour].size()]);
        }
    }
    // The bounds: of the stroke's outline when stroked, else of the contour centres (filled along the contour, or no
    // level yet), else of the finest level's edges.
    if (isStroked()) {
        m_minimum = Vector<2, float>(0.0f, 0.0f);
        m_maximum = Vector<2, float>(0.0f, 0.0f);
        bool first = true;
        for (int contour = 0; contour < (int)m_strokeOutlinePositions.size(); ++contour) {
            for (int i = 0; i < (int)m_strokeOutlinePositions[contour].size(); ++i) {
                Vector<2, float> point = m_strokeOutlinePositions[contour][i];
                if (first) {
                    m_minimum = point;
                    m_maximum = point;
                    first = false;
                } else {
                    if (m_minimum.m_elements[0] > point.m_elements[0])
                        m_minimum.m_elements[0] = point.m_elements[0];
                    if (m_minimum.m_elements[1] > point.m_elements[1])
                        m_minimum.m_elements[1] = point.m_elements[1];
                    if (point.m_elements[0] > m_maximum.m_elements[0])
                        m_maximum.m_elements[0] = point.m_elements[0];
                    if (point.m_elements[1] > m_maximum.m_elements[1])
                        m_maximum.m_elements[1] = point.m_elements[1];
                }
            }
        }
    } else if ((m_flags & (1 << MESH_CONTOUR)) || lastLevel < 0) {
        m_minimum = Vector<2, float>(0.0f, 0.0f);
        m_maximum = Vector<2, float>(0.0f, 0.0f);
        bool first = true;
        for (int contour = 0; contour < (int)m_contourCenters.size(); ++contour) {
            for (int i = 0; i < (int)m_contourCenters[contour].size(); ++i) {
                Vector<2, float> point = m_contourCenters[contour][i];
                if (first) {
                    m_minimum = point;
                    m_maximum = point;
                    first = false;
                } else {
                    if (m_minimum.m_elements[0] > point.m_elements[0])
                        m_minimum.m_elements[0] = point.m_elements[0];
                    if (m_minimum.m_elements[1] > point.m_elements[1])
                        m_minimum.m_elements[1] = point.m_elements[1];
                    if (point.m_elements[0] > m_maximum.m_elements[0])
                        m_maximum.m_elements[0] = point.m_elements[0];
                    if (point.m_elements[1] > m_maximum.m_elements[1])
                        m_maximum.m_elements[1] = point.m_elements[1];
                }
            }
        }
    } else {
        QuadMesh &level = m_levels[lastLevel];
        if (level.m_edges.size() == 0) {
            m_minimum = Vector<2, float>(0.0f, 0.0f);
            m_maximum = Vector<2, float>(0.0f, 0.0f);
        } else {
            m_minimum = *level.m_edges[0].m_vertices[0]->m_position;
            m_maximum = m_minimum;
            for (int i = 0; i < (int)level.m_edges.size(); ++i) {
                for (int endpoint = 0; endpoint < 2; ++endpoint) {
                    const Vector<2, float> &point = *level.m_edges[i].m_vertices[endpoint]->m_position;
                    if (m_minimum.m_elements[0] > point.m_elements[0])
                        m_minimum.m_elements[0] = point.m_elements[0];
                    if (m_minimum.m_elements[1] > point.m_elements[1])
                        m_minimum.m_elements[1] = point.m_elements[1];
                    if (point.m_elements[0] > m_maximum.m_elements[0])
                        m_maximum.m_elements[0] = point.m_elements[0];
                    if (point.m_elements[1] > m_maximum.m_elements[1])
                        m_maximum.m_elements[1] = point.m_elements[1];
                }
            }
        }
    }
    if (isExtruded()) {
        // The bounds are widened by the transformed minimum first; the second corner is the transformed maximum after
        // that widening.
        Vector<2, float> point = m_extrusionMatrix.value() * m_minimum;
        if (m_minimum.m_elements[0] > point.m_elements[0])
            m_minimum.m_elements[0] = point.m_elements[0];
        if (m_minimum.m_elements[1] > point.m_elements[1])
            m_minimum.m_elements[1] = point.m_elements[1];
        if (point.m_elements[0] > m_maximum.m_elements[0])
            m_maximum.m_elements[0] = point.m_elements[0];
        if (point.m_elements[1] > m_maximum.m_elements[1])
            m_maximum.m_elements[1] = point.m_elements[1];
        point = m_extrusionMatrix.value() * m_maximum;
        if (m_minimum.m_elements[0] > point.m_elements[0])
            m_minimum.m_elements[0] = point.m_elements[0];
        if (m_minimum.m_elements[1] > point.m_elements[1])
            m_minimum.m_elements[1] = point.m_elements[1];
        if (point.m_elements[0] > m_maximum.m_elements[0])
            m_maximum.m_elements[0] = point.m_elements[0];
        if (point.m_elements[1] > m_maximum.m_elements[1])
            m_maximum.m_elements[1] = point.m_elements[1];
    }
    m_changed = true;
    updateDrawings();
}

void SmoothMeshShape::updateDrawings()
{
    m_updatingDrawings = true;
    if (m_fillDrawing)
        m_fillDrawing->m_dirty = m_dirty;
    if (m_strokeDrawing)
        m_strokeDrawing->m_dirty = m_dirty;
    if (m_extrusionDrawing)
        m_extrusionDrawing->m_dirty = m_dirty;
    if ((m_flags & (1 << MESH_CONTOUR)) && (m_dirty & (1 << DRAWING_DIRTY_VERTICES))) {
        if (m_fillDrawing)
            m_fillDrawing->m_dirty |= (1 << DRAWING_DIRTY_INDICES);
        if (m_strokeDrawing)
            m_strokeDrawing->m_dirty |= (1 << DRAWING_DIRTY_INDICES);
        if (m_extrusionDrawing)
            m_extrusionDrawing->m_dirty |= (1 << DRAWING_DIRTY_INDICES);
    }
    m_dirty = 0;
    updateDrawingTextures();
    if (m_fillDrawing) {
        m_fillDrawing->m_contourFill = (m_flags & (1 << MESH_CONTOUR)) && isFilled();
        if (m_subdivisions == 0 && !(m_flags & (1 << MESH_CONTOUR))) {
            m_fillDrawing->m_pathVertices.resize(m_vertexPositions.length());
            m_fillDrawing->m_pathColors.resize(m_vertexPositions.length());
            m_fillDrawing->m_pathTextureCoordinates.resize(m_vertexPositions.length());
            m_fillDrawing->m_pathIndices.clear();
            for (int i = 0; i < (int)m_vertexPositions.length(); ++i) {
                m_fillDrawing->m_pathVertices[i] = m_vertexPositions.value()[i];
                m_fillDrawing->m_pathColors[i] = m_vertexColors.value()[i];
                m_fillDrawing->m_pathTextureCoordinates[i] = m_vertexTexCoords.value()[i];
            }
            // A triangle fan per face.
            for (int face = 0; face < (int)m_faces.size(); ++face) {
                std::vector<int> &vertices = m_faces[face];
                for (int corner = 1; corner < (int)vertices.size() - 1; ++corner) {
                    m_fillDrawing->m_pathIndices.push_back(vertices[0]);
                    m_fillDrawing->m_pathIndices.push_back(vertices[corner]);
                    m_fillDrawing->m_pathIndices.push_back(vertices[corner + 1]);
                }
            }
        }
    }
    if (m_subdivisions >= 0) {
        if (!(m_flags & (1 << MESH_NO_FILL)) && (m_flags & (1 << MESH_CONTOUR)) && m_fillDrawing) {
            m_fillDrawing->m_pathVertices.clear();
            m_fillDrawing->m_pathColors.clear();
            m_fillDrawing->m_pathTextureCoordinates.clear();
            m_fillDrawing->m_pathIndices.clear();
            m_fillDrawing->m_pathContours.resize(m_contourCenters.size());
            Vector<4, float> averageColor(0.0f, 0.0f, 0.0f, 0.0f);
            Vector<2, float> averagePosition(0.0f, 0.0f);
            int count = 0;
            for (int contour = 0; contour < (int)m_contourCenters.size(); ++contour) {
                for (int i = 0; i < (int)m_contourColors[contour].size(); ++i) {
                    averageColor += m_contourColors[contour][i];
                    averagePosition += m_contourCenters[contour][i];
                    ++count;
                }
            }
            // Empty and singular fits are left unchecked.
            float scale = 1.0f / count;
            averageColor = averageColor * scale;
            averagePosition = averagePosition * scale;
            float xx = 0.0f;
            float yy = 0.0f;
            float xy = 0.0f;
            Vector<4, float> colorX(0.0f, 0.0f, 0.0f, 0.0f);
            Vector<4, float> colorY(0.0f, 0.0f, 0.0f, 0.0f);
            for (int contour = 0; contour < (int)m_contourCenters.size(); ++contour) {
                for (int i = 0; i < (int)m_contourColors[contour].size(); ++i) {
                    Vector<2, float> point = m_contourCenters[contour][i] - averagePosition;
                    Vector<4, float> color = m_contourColors[contour][i] - averageColor;
                    xx += point.m_elements[0] * point.m_elements[0];
                    yy += point.m_elements[1] * point.m_elements[1];
                    xy += point.m_elements[1] * point.m_elements[0];
                    colorX += color * point.m_elements[0];
                    colorY += color * point.m_elements[1];
                }
            }
            float determinant = yy * xx - xy * xy;
            Vector<4, float> xGradient = (colorX * yy - colorY * xy) / determinant;
            Vector<4, float> yGradient = (colorY * xx - colorX * xy) / determinant;
            Vector<4, float> minimum(0.0f, 0.0f, 0.0f, 0.0f);
            Vector<4, float> maximum(1.0f, 1.0f, 1.0f, 1.0f);
            for (int contour = 0; contour < (int)m_contourCenters.size(); ++contour) {
                for (int i = 0; i < (int)m_contourCenters[contour].size(); ++i) {
                    Vector<2, float> point = m_contourCenters[contour][i] - averagePosition;
                    Vector<4, float> color =
                        averageColor + xGradient * point.m_elements[0] + yGradient * point.m_elements[1];
                    for (int component = 0; component < 4; ++component) {
                        if (minimum.m_elements[component] > color.m_elements[component])
                            minimum.m_elements[component] = color.m_elements[component];
                        if (color.m_elements[component] > maximum.m_elements[component])
                            maximum.m_elements[component] = color.m_elements[component];
                    }
                }
            }
            for (int contour = 0; contour < (int)m_contourCenters.size(); ++contour) {
                m_fillDrawing->m_pathContours[contour].resize(m_contourCenters[contour].size());
                for (int i = 0; i < (int)m_contourCenters[contour].size(); ++i) {
                    Vector<2, float> point = m_contourCenters[contour][i] - averagePosition;
                    Vector<4, float> color =
                        averageColor + xGradient * point.m_elements[0] + yGradient * point.m_elements[1];
                    color = (color - minimum) / (maximum - minimum);
                    m_fillDrawing->m_pathContours[contour][i] = m_fillDrawing->m_pathVertices.size();
                    m_fillDrawing->m_pathVertices.push_back(m_contourCenters[contour][i]);
                    m_fillDrawing->m_pathColors.push_back(color);
                    m_fillDrawing->m_pathTextureCoordinates.push_back(m_contourTexCoords[contour][i]);
                }
            }
            m_fillDrawing->tessellate();
        }
        if ((m_flags & (1 << MESH_EXTRUDED)) && m_extrusionDrawing) {
            m_extrusionDrawing->m_pathVertices.clear();
            m_extrusionDrawing->m_pathColors.clear();
            m_extrusionDrawing->m_pathBackColors.clear();
            m_extrusionDrawing->m_pathTextureCoordinates.clear();
            m_extrusionDrawing->m_pathIndices.clear();
            m_extrusionDrawing->m_pathContours.resize(m_contourCenters.size());
            for (int contour = 0; contour < (int)m_contourCenters.size(); ++contour) {
                m_extrusionDrawing->m_pathContours[contour].resize(m_contourCenters[contour].size());
                for (int i = 0; i < (int)m_contourCenters[contour].size(); ++i) {
                    m_extrusionDrawing->m_pathContours[contour][i] = m_extrusionDrawing->m_pathVertices.size();
                    m_extrusionDrawing->m_pathVertices.push_back(m_contourCenters[contour][i]);
                    m_extrusionDrawing->m_pathColors.push_back(m_contourFrontColors[contour][i]);
                    m_extrusionDrawing->m_pathBackColors.push_back(m_contourBackColors[contour][i]);
                    m_extrusionDrawing->m_pathTextureCoordinates.push_back(m_contourTexCoords[contour][i]);
                }
            }
            m_extrusionDrawing->extrude(m_extrusionMatrix.value());
            m_extrusionDrawing->m_dirty = DRAWING_DIRTY_ALL;
        }
        if ((m_flags & (1 << MESH_STROKE)) && m_strokeDrawing) {
            m_strokeDrawing->m_pathVertices.clear();
            m_strokeDrawing->m_pathColors.clear();
            m_strokeDrawing->m_pathTextureCoordinates.clear();
            m_strokeDrawing->m_pathIndices.clear();
            if (m_strokePattern == STROKE_PATTERN_SOLID || m_strokeGap == 0.0f) {
                for (int contour = 0; contour < (int)m_strokeOutlinePositions.size(); ++contour) {
                    int first = m_strokeDrawing->m_pathVertices.size();
                    if (m_flags & (1 << MESH_OPEN)) {
                        if (m_strokeCapType == STROKE_CAP_ROUND)
                            appendRoundCap(contour, 0, 0.0f, 0, m_strokeSegments);
                        else if (m_strokeCapType == STROKE_CAP_SQUARE)
                            appendSquareCap(contour, 0, 0.0f, false);
                    }
                    for (int i = 0; i < (int)m_strokeOutlinePositions[contour].size(); ++i) {
                        m_strokeDrawing->m_pathTextureCoordinates.push_back(m_strokeOutlineTexCoords[contour][i]);
                        m_strokeDrawing->m_pathColors.push_back(m_strokeOutlineColors[contour][i]);
                        m_strokeDrawing->m_pathVertices.push_back(m_strokeOutlinePositions[contour][i]);
                    }
                    if (m_flags & (1 << MESH_OPEN)) {
                        int last = m_strokeOutlinePositions[contour].size() - 4;
                        if (m_strokeCapType == STROKE_CAP_ROUND)
                            appendRoundCap(contour, last, 1.0f, 1, m_strokeSegments);
                        else if (m_strokeCapType == STROKE_CAP_SQUARE)
                            appendSquareCap(contour, last, 1.0f, true);
                    }
                    int count = m_strokeDrawing->m_pathVertices.size() - first;
                    int triangles = (m_flags & (1 << MESH_OPEN)) ? count - 2 : count;
                    for (int i = 0; i < triangles; ++i) {
                        m_strokeDrawing->m_pathIndices.push_back(first + i % count);
                        m_strokeDrawing->m_pathIndices.push_back(first + (i + (i & 1) + 1) % count);
                        m_strokeDrawing->m_pathIndices.push_back(first + (i + (i + 1) % 2 + 1) % count);
                    }
                }
            } else {
                for (int contour = 0; contour < (int)m_contourCenters.size(); ++contour)
                    buildDashedStroke(contour, m_strokeDash, m_strokeGap);
            }
            m_strokeDrawing->m_dirty = DRAWING_DIRTY_ALL;
        }
    }
    if (m_fillDrawing) {
        if (m_fillDrawing->m_dirty & (1 << DRAWING_DIRTY_INDICES))
            m_fillDrawing->buildContours();
        m_fillDrawing->update();
    }
    if (m_strokeDrawing) {
        if (m_strokeDrawing->m_dirty & (1 << DRAWING_DIRTY_INDICES))
            m_strokeDrawing->buildContours();
        m_strokeDrawing->update();
    }
    if (m_extrusionDrawing) {
        if (m_extrusionDrawing->m_dirty & (1 << DRAWING_DIRTY_INDICES))
            m_extrusionDrawing->buildContours();
        m_extrusionDrawing->update();
    }
    Matrix<float> identity;
    identity.identity();
    bool first = true;
    getTransformedBounds(m_minimum, m_maximum, identity, first);
    m_updatingDrawings = false;
}

void SmoothMeshShape::updateDrawingTextures()
{
    if (!m_fillTexture || m_fillTexture->m_id != m_texture.value())
        m_fillTexture = m_engine->findTexture(m_texture.value());
    if (!m_outlineTexture || m_outlineTexture->m_id != m_strokeTexture.value())
        m_outlineTexture = m_engine->findTexture(m_strokeTexture.value());
    if (m_fillDrawing) {
        m_fillDrawing->m_texture = m_fillTexture;
        m_fillDrawing->m_textureOpacity = m_textureOpacity.value();
        m_fillDrawing->m_textureBrightness = m_textureBrightness.value();
        m_fillDrawing->m_textureContrast = m_textureContrast.value();
        m_fillDrawing->m_textureSaturation = m_textureSaturation.value();
        m_fillDrawing->m_textureMatrix = m_textureMatrix;
    }
    if (m_strokeDrawing) {
        m_strokeDrawing->m_texture = m_outlineTexture;
        m_strokeDrawing->m_textureOpacity = m_strokeTextureOpacity.value();
        m_strokeDrawing->m_textureBrightness = m_strokeTextureBrightness.value();
        m_strokeDrawing->m_textureContrast = m_strokeTextureContrast.value();
        m_strokeDrawing->m_textureSaturation = m_strokeTextureSaturation.value();
        m_strokeDrawing->m_textureMatrix.identity();
        if (m_strokeTextureStretch.value() > 0.0f) {
            float scale = 1.0f / m_strokeTextureStretch.value();
            m_strokeDrawing->m_textureMatrix.scale(scale, 1.0f);
        }
    }
    // Extrusion uses the same texture as the fill.
    if (m_extrusionDrawing) {
        m_extrusionDrawing->m_texture = m_fillTexture;
        m_extrusionDrawing->m_textureOpacity = m_textureOpacity.value();
        m_extrusionDrawing->m_textureBrightness = m_textureBrightness.value();
        m_extrusionDrawing->m_textureContrast = m_textureContrast.value();
        m_extrusionDrawing->m_textureSaturation = m_textureSaturation.value();
        m_extrusionDrawing->m_textureMatrix = m_textureMatrix;
    }
}

}
