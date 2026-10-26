// SmoothMeshShape refines contours and builds fill, stroke and extrusion drawings.
#include "plasma/SmoothMeshShape.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Texture.h"
#include "plasma/StaticMeshShape.h"
#include <algorithm>
#include <cmath>

namespace plasma {
// A smooth mesh's tessellation and stroke (anonymous: this file's own): the defaults, the corners'
// vertex budget of the automatic subdivision, its bound, the stroke's smallest divisor and width,
// the cosine above which a corner counts as straight and the segments of a rounded corner per half
// turn.
namespace {
enum {
    DefaultSubdivisions = 3
};
enum {
    DefaultStrokeSegments = 20
};
enum {
    MaximumSubdivisions = 6
};
enum {
    CornerSegmentsPerHalfTurn = 20
};
}

static inline float strokeDistance(float length, float firstWidth, float secondWidth)
{
    float distance = (length * 2.0f) / (firstWidth + secondWidth);
    return distance;
}

void refineContourWidths(std::vector<float> &widths, std::vector<int> &types, int step, int segments, bool closed,
                         float weight, int level)
{
    // Read the type as an integer masked to the low sixteen bits.
    for (int i = 0; i < segments; ++i) {
        widths[(i * step + step / 2) % widths.size()] =
            (widths[((i + 1) * step) % widths.size()] + widths[(i * step) % widths.size()]) * 0.5f;
        float blend = 0.5f;
        if (!level) {
            static float cornerWeight = 1.0f / (std::cos(0.7853982f) * 2.0f + 1.0f);
            int firstType = types[(i * step) % types.size()] & VERTEX_KIND_MASK;
            if (firstType == VERTEX_ROUND &&
                (types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK) != VERTEX_ROUND)
                blend = cornerWeight;
            if ((types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK) == VERTEX_ROUND &&
                firstType != VERTEX_ROUND)
                blend = 1.0f - cornerWeight;
        }
        widths[(i * step + step / 2) % widths.size()] =
            widths[((i + 1) * step) % widths.size()] * (1.0f - blend) + blend * widths[(i * step) % widths.size()];
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
                widths[(i * step) % widths.size()] +=
                    ((widths[(i * step + step / 2) % widths.size()] + widths[(i * step - step / 2) % widths.size()]) *
                         0.5f -
                     widths[(i * step) % widths.size()]) *
                    weight;
            } else {
                widths[(i * step) % widths.size()] =
                    (widths[(i * step + step / 2) % widths.size()] + widths[(i * step - step / 2) % widths.size()]) *
                    0.5f;
            }
        }
    }
    for (int i = 0; i < segments; ++i) {
        int firstType = types[(i * step) % types.size()] & VERTEX_KIND_MASK;
        int secondType = types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK;
        if ((firstType == VERTEX_BEZIER || (!level && firstType == VERTEX_ROUND)) &&
            (secondType == VERTEX_BEZIER || (!level && secondType == VERTEX_ROUND))) {
            widths[(i * step + step / 2) % widths.size()] =
                (widths[((i + 1) * step) % widths.size()] + widths[(i * step) % widths.size()]) * 0.5f;
            types[(i * step + step / 2) % types.size()] = VERTEX_SHARP;
        }
    }
}

void refineContourPositions(std::vector<Vector<2, float>> &positions, std::vector<int> &types, int step, int segments,
                            bool closed, float weight, int level)
{
    for (int i = 0; i < segments; ++i) {
        positions[(i * step + step / 2) % positions.size()] =
            (positions[(i * step) % positions.size()] + positions[((i + 1) * step) % positions.size()]) * 0.5f;
        float blend = 0.5f;
        if (!level) {
            static float cornerWeight = 1.0f / (std::cos(0.7853982f) * 2.0f + 1.0f);
            int firstType = types[(i * step) % types.size()] & VERTEX_KIND_MASK;
            if (firstType == VERTEX_ROUND &&
                (types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK) != VERTEX_ROUND)
                blend = cornerWeight;
            if ((types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK) == VERTEX_ROUND &&
                firstType != VERTEX_ROUND)
                blend = 1.0f - cornerWeight;
        }
        positions[(i * step + step / 2) % positions.size()] =
            positions[(i * step) % positions.size()] * blend +
            positions[((i + 1) * step) % positions.size()] * (1.0f - blend);
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
                positions[(i * step) % positions.size()] += ((positions[(i * step - step / 2) % positions.size()] +
                                                              positions[(i * step + step / 2) % positions.size()]) *
                                                                 0.5f -
                                                             positions[(i * step) % positions.size()]) *
                                                            weight;
            } else {
                positions[(i * step) % positions.size()] = (positions[(i * step - step / 2) % positions.size()] +
                                                            positions[(i * step + step / 2) % positions.size()]) *
                                                           0.5f;
            }
        }
    }
    for (int i = 0; i < segments; ++i) {
        int firstType = types[(i * step) % types.size()] & VERTEX_KIND_MASK;
        int secondType = types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK;
        if ((firstType == VERTEX_BEZIER || (!level && firstType == VERTEX_ROUND)) &&
            (secondType == VERTEX_BEZIER || (!level && secondType == VERTEX_ROUND))) {
            positions[(i * step + step / 2) % positions.size()] =
                (positions[(i * step) % positions.size()] + positions[((i + 1) * step) % positions.size()]) * 0.5f;
            types[(i * step + step / 2) % types.size()] = VERTEX_SHARP;
        }
    }
}

void refineContourTypedColors(std::vector<Vector<4, float>> &colors, std::vector<int> &types, int step, int segments,
                              bool closed, float weight, int level)
{
    // Read the type as an integer masked to the low sixteen bits.
    for (int i = 0; i < segments; ++i) {
        colors[(i * step + step / 2) % colors.size()] =
            (colors[(i * step) % colors.size()] + colors[((i + 1) * step) % colors.size()]) * 0.5f;
        float blend = 0.5f;
        if (!level) {
            static float cornerWeight = 1.0f / (std::cos(0.7853982f) * 2.0f + 1.0f);
            int firstType = types[(i * step) % types.size()] & VERTEX_KIND_MASK;
            if (firstType == VERTEX_ROUND &&
                (types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK) != VERTEX_ROUND)
                blend = cornerWeight;
            if ((types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK) == VERTEX_ROUND &&
                firstType != VERTEX_ROUND)
                blend = 1.0f - cornerWeight;
        }
        colors[(i * step + step / 2) % colors.size()] =
            colors[(i * step) % colors.size()] * blend + colors[((i + 1) * step) % colors.size()] * (1.0f - blend);
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
                colors[(i * step) % colors.size()] +=
                    ((colors[(i * step - step / 2) % colors.size()] + colors[(i * step + step / 2) % colors.size()]) *
                         0.5f -
                     colors[(i * step) % colors.size()]) *
                    weight;
            } else {
                colors[(i * step) % colors.size()] =
                    (colors[(i * step - step / 2) % colors.size()] + colors[(i * step + step / 2) % colors.size()]) *
                    0.5f;
            }
        }
    }
    for (int i = 0; i < segments; ++i) {
        int firstType = types[(i * step) % types.size()] & VERTEX_KIND_MASK;
        int secondType = types[((i + 1) * step) % types.size()] & VERTEX_KIND_MASK;
        if ((firstType == VERTEX_BEZIER || (!level && firstType == VERTEX_ROUND)) &&
            (secondType == VERTEX_BEZIER || (!level && secondType == VERTEX_ROUND))) {
            colors[(i * step + step / 2) % colors.size()] =
                (colors[(i * step) % colors.size()] + colors[((i + 1) * step) % colors.size()]) * 0.5f;
            types[(i * step + step / 2) % types.size()] = VERTEX_SHARP;
        }
    }
}

void refineContourColors(std::vector<Vector<4, float>> &colors, int step, int segments, bool closed, float weight)
{
    for (int i = 0; i < segments; ++i) {
        colors[(i * step + step / 2) % colors.size()] =
            (colors[(i * step) % colors.size()] + colors[((i + 1) * step) % colors.size()]) * 0.5f;
    }
    if (closed)
        ++segments;
    for (int i = 1; i < segments; ++i) {
        colors[(i * step) % colors.size()] +=
            ((colors[(i * step - step / 2) % colors.size()] + colors[(i * step + step / 2) % colors.size()]) * 0.5f -
             colors[(i * step) % colors.size()]) *
            weight;
    }
    for (int i = 0; i < segments; ++i) {
        colors[(i * step + step / 2) % colors.size()] =
            (colors[(i * step) % colors.size()] + colors[((i + 1) * step) % colors.size()]) * 0.5f;
    }
}

SmoothMeshShape::SmoothMeshShape(Engine *engine, const std::wstring &name)
    : MeshShape(engine, name),
      m_subdivisions(DefaultSubdivisions),
      m_strokeSegments(DefaultStrokeSegments),
      m_limitSubdivisions(false),
      m_updatingDrawings(false),
      m_strokeDash(1.0f),
      m_strokeGap(1.0f),
      m_strokePattern(STROKE_PATTERN_SOLID)
{
    float root = sqrt(2.0f);
    m_smoothWeight = (root - 1.0f) / (root * 0.5f);
    m_dirty = DRAWING_DIRTY_ALL;
    m_fillDrawing = 0;
    m_strokeDrawing = 0;
    m_extrusionDrawing = 0;
    m_type = SHAPE_SMOOTH_MESH;
    InitializeCriticalSectionAndSpinCount(&m_lock, 0x80000400);
}

SmoothMeshShape::~SmoothMeshShape()
{
    if (m_fillDrawing)
        m_engine->deleteDrawing(m_fillDrawing);
    if (m_strokeDrawing)
        m_engine->deleteDrawing(m_strokeDrawing);
    if (m_extrusionDrawing)
        m_engine->deleteDrawing(m_extrusionDrawing);
    DeleteCriticalSection(&m_lock);
}

int SmoothMeshShape::getEffectiveSubdivisions()
{
    if (m_limitSubdivisions) {
        int subdivisions = m_subdivisions;
        if (subdivisions > 2) {
            int corners = 0;
            for (int face = 0; face < (int)m_faces.size(); ++face)
                corners += (int)m_faces[face].size();
            // Leaves zero-corner division and conversion overflow unchecked.
            float budget = 2560.0f / corners;
            float numerator = log((double)budget);
            float denominator = log(4.0);
            int result = (int)(numerator / denominator) + 1;
            if (result < subdivisions)
                return result;
        }
    }
    return m_subdivisions;
}

void SmoothMeshShape::appendRoundCap(int contour, int vertex, float fraction, int atEnd, int segments)
{
    unsigned next = (vertex + 2) % m_strokeOutlinePositions[contour].size();
    Vector<2, float> center =
        (m_strokeOutlinePositions[contour][vertex] + m_strokeOutlinePositions[contour][vertex + 1]) *
            ((1.0f - fraction) * 0.5f) +
        (m_strokeOutlinePositions[contour][next] + m_strokeOutlinePositions[contour][next + 1]) * (fraction * 0.5f);
    Vector<2, float> outer = m_strokeOutlinePositions[contour][vertex] * (1.0f - fraction) +
                             m_strokeOutlinePositions[contour][next] * fraction;
    Vector<2, float> radius = outer - center;
    Vector<2, float> normal(radius[1], -radius[0]);
    Vector<2, float> texture((1.0f - fraction) * m_strokeOutlineTexCoords[contour][vertex][0] +
                                 fraction * m_strokeOutlineTexCoords[contour][next][0],
                             0.5f);
    float length = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1]);
    const Vector<2, float> *textureCoordinates = &m_strokeOutlineTexCoords[contour][0];
    Vector<2, float> delta =
        (m_strokeOutlinePositions[contour][vertex] + m_strokeOutlinePositions[contour][vertex + 1]) * 0.5f -
        (m_strokeOutlinePositions[contour][next] + m_strokeOutlinePositions[contour][next + 1]) * 0.5f;
    float textureOffset = (textureCoordinates[next][0] - textureCoordinates[vertex][0]) * -length /
                          std::sqrt(delta[1] * delta[1] + delta[0] * delta[0]);
    Vector<2, float> textureNormal(textureOffset, 0.0f);
    Vector<2, float> textureRadius(0.0f, -0.5f);
    Vector<4, float> color =
        m_strokeOutlineColors[contour][vertex] * (1.0f - fraction) + m_strokeOutlineColors[contour][next] * fraction;
    float startAngle = 0.0f;
    float quarter = 1.5707964f;
    if (atEnd)
        startAngle = (0.5f / segments + 1.0f) * quarter + startAngle;
    for (int i = 0; i < segments; ++i) {
        // The starting angle is float; each step multiplies by double-precision half-pi.
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
    unsigned next = (vertex + 2) % m_strokeOutlinePositions[contour].size();
    Vector<2, float> center =
        (m_strokeOutlinePositions[contour][vertex] + m_strokeOutlinePositions[contour][vertex + 1]) *
            ((1.0f - fraction) * 0.5f) +
        (m_strokeOutlinePositions[contour][next] + m_strokeOutlinePositions[contour][next + 1]) * (fraction * 0.5f);
    Vector<2, float> outer = m_strokeOutlinePositions[contour][vertex] * (1.0f - fraction) +
                             m_strokeOutlinePositions[contour][next] * fraction;
    Vector<2, float> offset = outer - center;
    Vector<2, float> normal(offset[1], -offset[0]);
    float texture = (1.0f - fraction) * m_strokeOutlineTexCoords[contour][vertex][0] +
                    fraction * m_strokeOutlineTexCoords[contour][next][0];
    float radiusSquared = normal[0] * normal[0] + normal[1] * normal[1];
    float radius = std::sqrt(radiusSquared);
    const Vector<2, float> *textureCoordinates = &m_strokeOutlineTexCoords[contour][0];
    Vector<2, float> delta =
        (m_strokeOutlinePositions[contour][vertex] + m_strokeOutlinePositions[contour][vertex + 1]) * 0.5f -
        (m_strokeOutlinePositions[contour][next] + m_strokeOutlinePositions[contour][next + 1]) * 0.5f;
    float lengthSquared = delta[0] * delta[0] + delta[1] * delta[1];
    float textureOffset =
        (textureCoordinates[next][0] - textureCoordinates[vertex][0]) * -radius / std::sqrt(lengthSquared);
    if (reverse) {
        normal = normal * -1.0;
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
    unsigned next = (vertex + 2) % m_strokeOutlinePositions[contour].size();
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
    int segments = (int)m_contourCenters[contour].size();
    float totalDistance = 0.0f;
    if (!closed)
        --segments;
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
    // AdvanceStroke supplies these outputs; the false path exits before use.
    int endSegment;
    float endFraction;
    for (int dashIndex = 0; dashIndex < dashes; ++dashIndex) {
        int first = (int)m_strokeDrawing->m_pathVertices.size();
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
        int start = (int)((m_strokePairs[contour][(segment + 1) % m_strokePairs[contour].size()] - 2 +
                           m_strokeOutlinePositions[contour].size()) %
                          m_strokeOutlinePositions[contour].size());
        if (m_strokeCapType == STROKE_CAP_ROUND) {
            appendRoundCap(contour, start, fraction, 0, capSegments);
            pairs = capSegments;
        } else if (m_strokeCapType == STROKE_CAP_SQUARE) {
            appendSquareCap(contour, start, fraction, false);
            pairs = 1;
        }
        appendStrokePair(contour, start, fraction, true);
        ++pairs;
        int end = (int)((m_strokePairs[contour][(endSegment + 1) % m_strokePairs[contour].size()] - 2 +
                         m_strokeOutlinePositions[contour].size()) %
                        m_strokeOutlinePositions[contour].size());
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
        // Retains this loop even though this branch normally runs it zero times.
        for (int i = 0; i < m_subdivisions; ++i) {
            m_levels[i].m_edges.clear();
            m_levels[i].m_faces.clear();
        }
        return;
    }
    m_vertexColors.resize((int)m_vertexPositions.m_values[0]->size());
    m_vertexTexCoords.resize((int)m_vertexPositions.m_values[0]->size());
    m_vertexFlags.resize(m_vertexPositions.m_values[0]->size());
    m_vertexParameters.resize(m_vertexPositions.m_values[0]->size());
    m_strokeColors.resize((int)m_vertexPositions.m_values[0]->size());
    m_extrusionFrontColors.resize((int)m_vertexPositions.m_values[0]->size());
    m_extrusionBackColors.resize((int)m_vertexPositions.m_values[0]->size());
    m_strokeWidths.resize((int)m_vertexPositions.m_values[0]->size());
    m_vertices.resize(m_vertexPositions.m_values[0]->size());
    m_faceVertices.resize(m_faces.size());
    m_controlEdges.clear();
    typedef std::pair<int, int> EdgeKey;
    std::map<EdgeKey, int> controlEdges;
    unsigned edgeCount = 0;
    for (int face = 0; face < (int)m_faces.size(); ++face) {
        std::vector<int> &vertices = m_faces[face];
        if (vertices.size() < 3)
            continue;
        for (int corner = 0; corner < (int)vertices.size(); ++corner) {
            unsigned next = (corner + 1) % vertices.size();
            EdgeKey key(vertices[corner], vertices[next]);
            if (controlEdges.find(key) == controlEdges.end()) {
                controlEdges[key] = edgeCount;
                controlEdges[EdgeKey(key.second, key.first)] = edgeCount;
                ControlEdge edge;
                edge.m_elements[0] = face;
                edge.m_elements[1] = corner;
                edge.m_elements[2] = edgeCount++;
                m_controlEdges.push_back(edge);
            }
        }
    }
    m_edgeVertices.resize(edgeCount);
    m_levels[0].m_faces.clear();
    m_adjacentFaces = m_faces;
    unsigned corners = 0;
    for (int i = 0; i < (int)m_faces.size(); ++i) {
        if (m_faces[i].size() >= 3)
            corners += (int)m_faces[i].size();
    }
    m_levels[0].m_faces.resize(corners);
    std::map<EdgeKey, int> reverseFaces;
    unsigned quadIndex = 0;
    for (int face = 0; face < (int)m_faces.size(); ++face) {
        std::vector<int> &vertices = m_faces[face];
        if (vertices.size() < 3)
            continue;
        for (int corner = 0; corner < (int)vertices.size(); ++corner) {
            unsigned next = (corner + 1) % vertices.size();
            size_t previous = (vertices.size() + corner - 1) % vertices.size();
            QuadFace &quad = m_levels[0].m_faces[quadIndex++];
            quad.m_vertices[0] = &m_faceVertices[face];
            quad.m_vertices[1] = &m_edgeVertices[controlEdges[EdgeKey(vertices[corner], vertices[previous])]];
            quad.m_vertices[2] = &m_vertices[vertices[corner]];
            quad.m_vertices[3] = &m_edgeVertices[controlEdges[EdgeKey(vertices[corner], vertices[next])]];
            quad.m_parent = 0;
            quad.m_unused78 = 0;
            reverseFaces[EdgeKey(vertices[next], vertices[corner])] = face;
        }
    }
    for (int face = 0; face < (int)m_faces.size(); ++face) {
        std::vector<int> &vertices = m_faces[face];
        for (int corner = 0; corner < (int)vertices.size(); ++corner) {
            EdgeKey key(vertices[corner], vertices[(corner + 1) % vertices.size()]);
            std::map<EdgeKey, int>::iterator adjacent = reverseFaces.find(key);
            m_adjacentFaces[face][corner] = adjacent == reverseFaces.end() ? -1 : adjacent->second;
        }
    }
    m_levels[0].m_edges.resize(corners + edgeCount * 2);
    typedef std::pair<QuadVertex *, QuadVertex *> VertexPair;
    std::map<VertexPair, QuadEdge *> edges;
    unsigned edgeIndex = 0;
    for (int face = 0; face < (int)m_levels[0].m_faces.size(); ++face) {
        QuadFace &quad = m_levels[0].m_faces[face];
        for (int corner = 0; corner < 4; ++corner) {
            QuadVertex *first = quad.m_vertices[corner];
            QuadVertex *second = quad.m_vertices[(corner + 1) % 4];
            VertexPair key(first, second);
            std::map<VertexPair, QuadEdge *>::iterator found = edges.find(key);
            if (found == edges.end()) {
                QuadEdge *edge = &m_levels[0].m_edges[edgeIndex++];
                edge->m_faces[0] = &quad;
                edge->m_faces[1] = 0;
                edge->m_vertices[0] = first;
                edge->m_vertices[1] = second;
                edges[key] = edge;
                quad.m_edges[corner] = edge;
                // Stores the key a second time before the reverse key.
                edges[VertexPair(first, second)] = edge;
                edges[VertexPair(second, first)] = edge;
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
    Drawing *fill = 0;
    Drawing *stroke = 0;
    Drawing *extrusion = 0;
    if (m_fillDrawing && isFilled()) {
        fill = m_fillDrawing;
        fill->reset();
        m_fillDrawing = 0;
    }
    if (m_strokeDrawing && isStroked()) {
        stroke = m_strokeDrawing;
        stroke->reset();
        m_strokeDrawing = 0;
    }
    if (m_extrusionDrawing && isExtruded()) {
        extrusion = m_extrusionDrawing;
        extrusion->reset();
        m_extrusionDrawing = 0;
    }
    return new StaticMeshShape(m_engine, fill, stroke, extrusion, m_name);
}

void SmoothMeshShape::draw(Node *node)
{
    lock();
    if (m_extrusionDrawing && isExtruded())
        m_extrusionDrawing->draw(node, m_engine->m_extrusionColor);
    if (m_fillDrawing && isFilled())
        m_fillDrawing->draw(node, m_engine->m_fillColor);
    if (m_strokeDrawing && isStroked())
        m_strokeDrawing->draw(node, m_engine->m_outlineColor);
    unlock();
}

void SmoothMeshShape::shownChanged(bool shown)
{
    if (shown) {
        if (!m_fillDrawing) {
            m_fillDrawing = m_engine->createDrawing();
            m_strokeDrawing = m_engine->createDrawing();
            m_extrusionDrawing = m_engine->createDrawing();
            attributesChanged(ATTRIBUTES_CHANGED_ALL);
        }
    } else if (m_fillDrawing) {
        for (std::set<Node *>::iterator node = m_nodes.begin(); node != m_nodes.end(); ++node) {
            if ((static_cast<unsigned char>(~((*node)->m_flags >> NODE_HIDDEN)) & 1) != 0)
                break;
        }
    }
}

bool SmoothMeshShape::advanceStroke(int contour, int segment, float fraction, float distance, int &resultSegment,
                                    float &resultFraction)
{
    unsigned next = (segment + 1) % m_contourCenters[contour].size();
    if (distance <= 0.0f)
        distance = 0.0f;
    Vector<2, float> delta = m_contourCenters[contour][segment] - m_contourCenters[contour][next];
    float squaredLength = delta.m_elements[0] * delta.m_elements[0] + delta.m_elements[1] * delta.m_elements[1];
    float length = std::sqrt(squaredLength);
    float firstWidth = m_contourWidths[contour][segment];
    float secondWidth = m_contourWidths[contour][next];
    float partialLength = length * fraction;
    float width = fraction * secondWidth + (1.0f - fraction) * firstWidth;
    float advance = strokeDistance(partialLength, firstWidth, width) + distance;
    float divisor = 1.0f - (secondWidth - firstWidth) * advance / (length * 2.0f);
    if (divisor < 0.0000001f)
        divisor = 0.0000001f;
    float offset = advance * firstWidth / divisor;
    if (offset <= length) {
        resultSegment = segment;
        resultFraction = offset / length;
        return true;
    }
    float remaining = length - partialLength;
    float nextDistance = distance - strokeDistance(remaining, firstWidth, secondWidth);
    // Recurses before testing the output segment, including NaN input.
    bool advanced = advanceStroke(contour, next, 0.0f, nextDistance, resultSegment, resultFraction);
    return resultSegment < segment ? false : advanced;
}

void SmoothMeshShape::releaseCache()
{
    // Keeps m_contourTypes.
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

void SmoothMeshShape::unlock()
{
    LeaveCriticalSection(&m_lock);
}

void SmoothMeshShape::setFaces(const std::vector<std::vector<int>> &faces, bool update)
{
    MeshShape::setFaces(faces);
    if (update) {
        updateSubdivision();
        attributesChanged(ATTRIBUTES_CHANGED_ALL);
    }
}

void SmoothMeshShape::setSubdivisions(int count, bool update)
{
    m_subdivisions = count;
    if (count < 0)
        m_subdivisions = 0;
    if (m_subdivisions > MaximumSubdivisions)
        m_subdivisions = MaximumSubdivisions;
    if (update) {
        updateSubdivision();
        attributesChanged(ATTRIBUTES_CHANGED_ALL);
    }
}

void SmoothMeshShape::updateSubdivisionDrawings(int positionElement, int colorElement, int textureElement)
{
    int lastLevel = getEffectiveSubdivisions() - 1;
    if (!m_fillDrawing)
        return;
    if (lastLevel < 0 || m_faces.empty() || m_vertices.empty()) {
        m_fillDrawing->m_pathVertices.clear();
        m_fillDrawing->m_pathColors.erase(m_fillDrawing->m_pathColors.begin(), m_fillDrawing->m_pathColors.end());
        m_fillDrawing->m_pathTextureCoordinates.clear();
        m_fillDrawing->m_pathIndices.clear();
        return;
    }
    if ((m_flags & (1 << MESH_CONTOUR)) || !isFilled())
        return;
    QuadMesh &subdivision = m_levels[lastLevel];
    m_fillDrawing->m_pathVertices.resize(subdivision.m_vertices.size());
    m_fillDrawing->m_pathColors.resize(m_fillDrawing->m_pathVertices.size());
    m_fillDrawing->m_pathTextureCoordinates.resize(m_fillDrawing->m_pathVertices.size());
    m_fillDrawing->m_pathIndices.resize(subdivision.m_faces.size() * 6);
    for (int i = 0; i < (int)m_vertices.size(); ++i) {
        m_vertices[i].m_position = 0;
        m_vertices[i].m_texturePosition = 0;
        m_vertices[i].m_color = 0;
    }
    for (int i = 0; i < (int)m_fillDrawing->m_pathVertices.size(); ++i) {
        subdivision.m_vertices[i]->m_position = &m_fillDrawing->m_pathVertices[i];
        subdivision.m_vertices[i]->m_texturePosition = &m_fillDrawing->m_pathTextureCoordinates[i];
        subdivision.m_vertices[i]->m_color = &m_fillDrawing->m_pathColors[i];
    }
    for (int i = 0; i < (int)subdivision.m_faces.size(); ++i) {
        QuadFace &face = subdivision.m_faces[i];
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
        vertex.m_flags = QUAD_FLAGS_NONE;
    }
    for (int face = 0; face < (int)m_faces.size(); ++face) {
        for (int corner = 0; corner < (int)m_faces[face].size(); ++corner) {
            if (m_adjacentFaces[face][corner] < 0) {
                m_vertices[m_faces[face][corner]].m_flags |= (1 << QUAD_BOUNDARY);
                m_vertices[m_faces[face][(corner + 1) % m_faces[face].size()]].m_flags |= (1 << QUAD_BOUNDARY);
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
        std::vector<int> &face = m_faces[control.m_elements[0]];
        QuadVertex &edge = m_edgeVertices[control.m_elements[2]];
        QuadVertex &first = m_vertices[face[control.m_elements[1]]];
        QuadVertex &second = m_vertices[face[(control.m_elements[1] + 1) % face.size()]];
        edge.m_flags &= ~((1 << QUAD_SHARP) | (1 << QUAD_BEZIER) | (1 << QUAD_ROUND));
        int adjacent = m_adjacentFaces[control.m_elements[0]][control.m_elements[1]];
        if (adjacent < 0 || (first.m_flags & (1 << QUAD_SHARP)) || (second.m_flags & (1 << QUAD_SHARP))) {
            float blend = 0.5f;
            static float cornerWeight = 1.0f / (std::cos(0.7853982f) * 2.0f + 1.0f);
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
                               0.5;
            *edge.m_color = ((*firstFace.m_color + *secondFace.m_color) * faceWeight +
                             (*first.m_color + *second.m_color) * vertexWeight) *
                            0.5;
            *edge.m_texturePosition = ((*first.m_texturePosition + *second.m_texturePosition) * vertexWeight +
                                       (*firstFace.m_texturePosition + *secondFace.m_texturePosition) * faceWeight) *
                                      0.5;
        }
    }
    for (int face = 0; face < (int)m_faces.size(); ++face) {
        if (m_faces[face].size() >= 3) {
            for (int corner = 0; corner < (int)m_faces[face].size(); ++corner) {
                QuadVertex &first = m_vertices[m_faces[face][corner]];
                QuadVertex &second = m_vertices[m_faces[face][(corner + 1) % m_faces[face].size()]];
                float blend = 0.5f;
                static float cornerWeight = 1.0f / (std::cos(0.7853982f) * 2.0f + 1.0f);
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
            *vertex.m_position = *vertex.m_position * 0.25 + vertex.m_positionSum * 0.75f * scale;
            *vertex.m_color = *vertex.m_color * 0.25 + vertex.m_colorSum * 0.75f * scale;
            *vertex.m_texturePosition = *vertex.m_texturePosition * 0.25 + vertex.m_textureSum * 0.75f * scale;
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
            *edge.m_position = (*first.m_position + *second.m_position) * 0.5;
            *edge.m_color = (*first.m_color + *second.m_color) * 0.5;
            *edge.m_texturePosition = (*first.m_texturePosition + *second.m_texturePosition) * 0.5;
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

// Rebuilds fill, stroke and extrusion drawings after attribute changes, refining contours per subdivision level and updating outlines and bounds.
void SmoothMeshShape::attributesChanged(int reason)
{
    lock();
    bool filled = isFilled();
    if (filled && !m_fillDrawing) {
        m_fillDrawing = static_cast<Drawing *>(m_engine->createDrawing());
    } else if (!filled && m_fillDrawing) {
        m_engine->deleteDrawing(m_fillDrawing);
        m_fillDrawing = 0;
    }
    bool stroked = isStroked();
    if (stroked && !m_strokeDrawing) {
        m_strokeDrawing = static_cast<Drawing *>(m_engine->createDrawing());
    } else if (!stroked && m_strokeDrawing) {
        m_engine->deleteDrawing(m_strokeDrawing);
        m_strokeDrawing = 0;
    }
    bool extruded = (m_flags >> MESH_EXTRUDED) & 1; // MESH_EXTRUDED, read like MeshShape's mode accessors
    if (extruded && !m_extrusionDrawing) {
        m_extrusionDrawing = static_cast<Drawing *>(m_engine->createDrawing());
    } else if (!extruded && m_extrusionDrawing) {
        m_engine->deleteDrawing(m_extrusionDrawing);
        m_extrusionDrawing = 0;
    }
    if (reason != ATTRIBUTES_CHANGED_VALUES) {
        m_dirty = DRAWING_DIRTY_ALL;
    } else {
        if (m_vertexPositions.m_changed) {
            m_vertexPositions.m_changed = false;
            m_dirty |= (1 << DRAWING_DIRTY_VERTICES);
        }
        if (m_vertexColors.m_changed) {
            m_vertexColors.m_changed = false;
            m_dirty |= (1 << DRAWING_DIRTY_COLORS);
        }
        if (m_vertexTexCoords.m_changed) {
            m_vertexTexCoords.m_changed = false;
            m_dirty |= (1 << DRAWING_DIRTY_TEXCOORDS);
        }
        if (m_strokeWidths.m_changed) {
            m_strokeWidths.m_changed = false;
            m_dirty |= (1 << DRAWING_DIRTY_VERTICES);
        }
        if (m_strokeColors.m_changed) {
            m_strokeColors.m_changed = false;
            m_dirty |= (1 << DRAWING_DIRTY_COLORS);
        }
        if (m_extrusionFrontColors.m_changed) {
            m_extrusionFrontColors.m_changed = false;
            m_dirty |= (1 << DRAWING_DIRTY_COLORS);
        }
        if (m_extrusionBackColors.m_changed) {
            m_extrusionBackColors.m_changed = false;
            m_dirty |= (1 << DRAWING_DIRTY_COLORS);
        }
    }
    if (m_vertexPositions.m_values[0]->size() != m_extrusionFrontColors.m_values[0]->size())
        m_extrusionFrontColors.resize((int)m_vertexPositions.m_values[0]->size());
    if (m_vertexPositions.m_values[0]->size() != m_extrusionBackColors.m_values[0]->size())
        m_extrusionBackColors.resize((int)m_vertexPositions.m_values[0]->size());
    int lastLevel = getEffectiveSubdivisions() - 1;
    m_dirty |= (1 << DRAWING_DIRTY_VERTICES);
    applyVertexParameters();
    if (lastLevel < 0) {
        MeshShape::updateBoundsAndTextures();
    } else {
        updateTextureMatrix();
        if (m_texture.value() == -1)
            m_fillTexture = 0;
        else if (!m_fillTexture || m_fillTexture->m_id != m_texture.value())
            m_fillTexture = m_engine->findTexture(m_texture.value());
        if (m_strokeTexture.value() == -1)
            m_outlineTexture = 0;
        else if (!m_outlineTexture || m_outlineTexture->m_id != m_strokeTexture.value())
            m_outlineTexture = m_engine->findTexture(m_strokeTexture.value());
        if (isFilled() && !(isContour()))
            updateSubdivisionDrawings(m_vertexPositions.m_current, m_vertexColors.m_current,
                                      m_vertexTexCoords.m_current);
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
        m_strokeOutlinePositions.erase(m_strokeOutlinePositions.begin(), m_strokeOutlinePositions.end());
        m_strokeOutlineColors.erase(m_strokeOutlineColors.begin(), m_strokeOutlineColors.end());
        m_strokeOutlineTexCoords.erase(m_strokeOutlineTexCoords.begin(), m_strokeOutlineTexCoords.end());
        m_strokePairs.erase(m_strokePairs.begin(), m_strokePairs.end());
    }
    bool closed = isClosed();
    int subdivisions = getEffectiveSubdivisions();
    int step = (int)pow(2.0, subdivisions);
    for (int contour = 0; contour < (int)m_contours.size(); ++contour) {
        int count = (int)(isClosed() ? step * m_contours[contour].size() : step * (m_contours[contour].size() - 1) + 1);
        m_contourCenters[contour].resize(count);
        m_contourColors[contour].resize(count);
        m_contourStrokeColors[contour].resize(count);
        m_contourFrontColors[contour].resize(count);
        m_contourBackColors[contour].resize(count);
        m_contourWidths[contour].resize(count);
        m_contourTexCoords[contour].resize(count);
        if (m_flags & (1 << MESH_STROKE)) {
            m_strokePairs[contour].resize(closed ? count + 1 : count);
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
            if (m_flags & (1 << MESH_STROKE)) {
                m_contourStrokeColors[contour][destination] = m_strokeColors.value()[vertex];
                m_contourWidths[contour][destination] = m_strokeWidths.value()[vertex];
                if (m_contourWidths[contour][destination] < 0.1f)
                    m_contourWidths[contour][destination] = 0.1f;
            }
            if (m_flags & (1 << MESH_EXTRUDED)) {
                m_contourFrontColors[contour][destination] = m_extrusionFrontColors.value()[vertex];
                m_contourBackColors[contour][destination] = m_extrusionBackColors.value()[vertex];
            }
        }
        for (int level = 0; level < subdivisions; ++level) {
            unsigned stride = (unsigned)pow(2.0, subdivisions - level);
            int segments = (int)(m_contourCenters[contour].size() / stride);
            refineContourPositions(m_contourCenters[contour], m_contourTypes[contour], stride, segments, closed,
                                   m_smoothWeight, level);
            refineContourPositions(m_contourTexCoords[contour], m_contourTypes[contour], stride, segments, closed,
                                   m_smoothWeight, level);
            refineContourTypedColors(m_contourColors[contour], m_contourTypes[contour], stride, segments, closed,
                                     m_smoothWeight, level);
            if (m_flags & (1 << MESH_STROKE)) {
                refineContourTypedColors(m_contourStrokeColors[contour], m_contourTypes[contour], stride, segments,
                                         closed, m_smoothWeight, level);
                refineContourWidths(m_contourWidths[contour], m_contourTypes[contour], stride, segments, closed,
                                    m_smoothWeight, level);
            }
            if (m_flags & (1 << MESH_EXTRUDED)) {
                refineContourColors(m_contourFrontColors[contour], stride, segments, closed, m_smoothWeight);
                refineContourColors(m_contourBackColors[contour], stride, segments, closed, m_smoothWeight);
            }
        }
        if (!(m_flags & (1 << MESH_STROKE)))
            continue;
        float distance = 0.0f;
        int count = (int)m_contourCenters[contour].size();
        int samples = closed ? count + 1 : count;
        float alignment = (float)m_strokeAlignment;
        for (int i = 0; i < samples; ++i) {
            unsigned vertex = i % m_contourCenters[contour].size();
            m_strokePairs[contour][i] = (int)m_strokeOutlinePositions[contour].size();
            if (!closed && (vertex == 0 || vertex == count - 1)) {
                Vector<2, float> tangent;
                if (vertex == 0) {
                    tangent = m_contourCenters[contour][0] - m_contourCenters[contour][1];
                    if (tangent.m_elements[0] * tangent.m_elements[0] + tangent.m_elements[1] * tangent.m_elements[1] ==
                        0.0f) {
                        unsigned next = step * ((step + 1) / step) % m_contourCenters[contour].size();
                        tangent = m_contourCenters[contour][0] - m_contourCenters[contour][next];
                    }
                } else {
                    tangent = m_contourCenters[contour][vertex - 1] - m_contourCenters[contour][vertex];
                    if (tangent.m_elements[0] * tangent.m_elements[0] + tangent.m_elements[1] * tangent.m_elements[1] ==
                        0.0f) {
                        unsigned previous =
                            step * ((count - step + vertex - 1) / step) % m_contourCenters[contour].size();
                        tangent = m_contourCenters[contour][previous] - m_contourCenters[contour][vertex];
                    }
                }
                Vector<2, float> normal(-tangent.m_elements[1], tangent.m_elements[0]);
                float inverseLength = 1.0f / std::sqrt(normal.m_elements[0] * normal.m_elements[0] +
                                                       normal.m_elements[1] * normal.m_elements[1]);
                normal = normal * inverseLength;
                normal = normal * (m_contourWidths[contour][vertex] * 0.5f);
                Vector<2, float> offset = normal * alignment;
                m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] + normal + offset);
                m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] - normal + offset);
                m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 0.0f));
                m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 1.0f));
            } else {
                unsigned previous = (count + vertex - 1) % m_contourCenters[contour].size();
                unsigned next = (vertex + 1) % m_contourCenters[contour].size();
                Vector<2, float> incoming = m_contourCenters[contour][previous] - m_contourCenters[contour][vertex];
                Vector<2, float> outgoing = m_contourCenters[contour][vertex] - m_contourCenters[contour][next];
                if (incoming.m_elements[0] * incoming.m_elements[0] + incoming.m_elements[1] * incoming.m_elements[1] ==
                    0.0f) {
                    previous = step * ((count - step + vertex - 1) / step) % m_contourCenters[contour].size();
                    incoming = m_contourCenters[contour][previous] - m_contourCenters[contour][vertex];
                }
                if (outgoing.m_elements[0] * outgoing.m_elements[0] + outgoing.m_elements[1] * outgoing.m_elements[1] ==
                    0.0f) {
                    next = step * ((vertex + step + 1) / step) % m_contourCenters[contour].size();
                    outgoing = m_contourCenters[contour][vertex] - m_contourCenters[contour][next];
                }
                bool missingIncoming = true;
                Vector<2, float> firstNormal(-incoming.m_elements[1], incoming.m_elements[0]);
                Vector<2, float> secondNormal(-outgoing.m_elements[1], outgoing.m_elements[0]);
                float lengthSquared = firstNormal.m_elements[0] * firstNormal.m_elements[0] +
                                      firstNormal.m_elements[1] * firstNormal.m_elements[1];
                if (lengthSquared > 0.0f) {
                    missingIncoming = false;
                    firstNormal = firstNormal * (1.0f / std::sqrt(lengthSquared));
                }
                lengthSquared = secondNormal.m_elements[0] * secondNormal.m_elements[0] +
                                secondNormal.m_elements[1] * secondNormal.m_elements[1];
                if (lengthSquared > 0.0f)
                    secondNormal = secondNormal * (1.0f / std::sqrt(lengthSquared));
                Vector<2, float> bisector = firstNormal + secondNormal;
                lengthSquared =
                    bisector.m_elements[0] * bisector.m_elements[0] + bisector.m_elements[1] * bisector.m_elements[1];
                bool corner = !(vertex % step) && (unsigned short)m_contourTypes[contour][vertex] == 1;
                if (lengthSquared > 0.0f) {
                    bisector = bisector * (1.0f / std::sqrt(lengthSquared));
                    if (corner) {
                        float projection = missingIncoming ? bisector.dot(secondNormal) : bisector.dot(firstNormal);
                        float minimumProjection = 0.3f;
                        float divisor = (std::max)(minimumProjection, projection);
                        bisector = bisector / divisor;
                    }
                } else {
                    bisector = Vector<2, float>(0.0f, 0.0f);
                }
                float halfWidth = m_contourWidths[contour][vertex] * 0.5f;
                bisector = bisector * halfWidth;
                Vector<2, float> offset = bisector * alignment;
                float normalDot = secondNormal.dot(firstNormal);
                if (!corner || !(normalDot < 0.95f)) {
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] + bisector + offset);
                    m_strokeOutlinePositions[contour].push_back(m_contourCenters[contour][vertex] - bisector + offset);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineColors[contour].push_back(m_contourStrokeColors[contour][vertex]);
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 0.0f));
                    m_strokeOutlineTexCoords[contour].push_back(Vector<2, float>(distance, 1.0f));
                } else if (m_strokeJointType == STROKE_JOINT_ROUND) {
                    float angle = acos(normalDot);
                    angle = angle * 57.29577951308232;
                    int segments = (int)(fabs(angle) / 180.0f * CornerSegmentsPerHalfTurn);
                    bool reverse = bisector.dot(outgoing) < 0.0f;
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
                        outer = (firstNormal + secondNormal) * 0.5;
                    else
                        inner = (firstNormal + secondNormal) * 0.5;
                    for (int segment = 1; segment < segments; ++segment) {
                        if (reverse)
                            inner = rotation * inner;
                        else
                            outer = rotation * outer;
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
                    float direction = bisector.dot(outgoing);
                    Vector<2, float> outer;
                    Vector<2, float> inner;
                    if (direction < 0.0f) {
                        outer = (firstNormal + secondNormal) * 0.5;
                        inner = bisector;
                    } else {
                        outer = bisector;
                        inner = (firstNormal + secondNormal) * 0.5;
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
            float length =
                std::sqrt(delta.m_elements[0] * delta.m_elements[0] + delta.m_elements[1] * delta.m_elements[1]);
            distance += strokeDistance(length, m_contourWidths[contour][vertex],
                                       m_contourWidths[contour][(vertex + 1) % m_contourWidths[contour].size()]);
        }
    }
    if ((m_flags & (1 << MESH_STROKE)) || (isContour()) || lastLevel < 0) {
        m_minimum = Vector<2, float>(0.0f, 0.0f);
        m_maximum = Vector<2, float>(0.0f, 0.0f);
        bool first = true;
        const std::vector<std::vector<Vector<2, float>>> &contours =
            (m_flags & (1 << MESH_STROKE)) ? m_strokeOutlinePositions : m_contourCenters;
        for (int contour = 0; contour < (int)contours.size(); ++contour) {
            for (int i = 0; i < (int)contours[contour].size(); ++i) {
                Vector<2, float> point = contours[contour][i];
                if (first) {
                    m_minimum = point;
                    m_maximum = point;
                    first = false;
                } else {
                    if (m_minimum.m_elements[0] > point.m_elements[0])
                        m_minimum.m_elements[0] = point.m_elements[0];
                    if (m_minimum.m_elements[1] > point.m_elements[1])
                        m_minimum.m_elements[1] = point.m_elements[1];
                    if (m_maximum.m_elements[0] < point.m_elements[0])
                        m_maximum.m_elements[0] = point.m_elements[0];
                    if (m_maximum.m_elements[1] < point.m_elements[1])
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
                    if (m_maximum.m_elements[0] < point.m_elements[0])
                        m_maximum.m_elements[0] = point.m_elements[0];
                    if (m_maximum.m_elements[1] < point.m_elements[1])
                        m_maximum.m_elements[1] = point.m_elements[1];
                }
            }
        }
    }
    if (m_flags & (1 << MESH_EXTRUDED)) {
        // Expands the bounds with the transformed minimum, then uses the already-expanded maximum for the
        // second transformed corner.
        Vector<2, float> point = m_extrusionMatrix.value() * m_minimum;
        if (m_minimum.m_elements[0] > point.m_elements[0])
            m_minimum.m_elements[0] = point.m_elements[0];
        if (m_minimum.m_elements[1] > point.m_elements[1])
            m_minimum.m_elements[1] = point.m_elements[1];
        if (m_maximum.m_elements[0] < point.m_elements[0])
            m_maximum.m_elements[0] = point.m_elements[0];
        if (m_maximum.m_elements[1] < point.m_elements[1])
            m_maximum.m_elements[1] = point.m_elements[1];
        point = m_extrusionMatrix.value() * m_maximum;
        if (m_minimum.m_elements[0] > point.m_elements[0])
            m_minimum.m_elements[0] = point.m_elements[0];
        if (m_minimum.m_elements[1] > point.m_elements[1])
            m_minimum.m_elements[1] = point.m_elements[1];
        if (m_maximum.m_elements[0] < point.m_elements[0])
            m_maximum.m_elements[0] = point.m_elements[0];
        if (m_maximum.m_elements[1] < point.m_elements[1])
            m_maximum.m_elements[1] = point.m_elements[1];
    }
    unlock();
    m_changed = true;
    updateDrawings();
}

void SmoothMeshShape::updateDrawings()
{
    lock();
    m_updatingDrawings = true;
    if (m_fillDrawing)
        m_fillDrawing->m_dirty = m_dirty;
    if (m_strokeDrawing)
        m_strokeDrawing->m_dirty = m_dirty;
    if (m_extrusionDrawing)
        m_extrusionDrawing->m_dirty = m_dirty;
    if ((isContour()) && (m_dirty & (1 << DRAWING_DIRTY_VERTICES))) {
        if (m_fillDrawing)
            m_fillDrawing->m_dirty |= (1 << DRAWING_DIRTY_INDICES);
        if (m_strokeDrawing)
            m_strokeDrawing->m_dirty |= (1 << DRAWING_DIRTY_INDICES);
        if (m_extrusionDrawing)
            m_extrusionDrawing->m_dirty |= (1 << DRAWING_DIRTY_INDICES);
    }
    m_dirty = DRAWING_DIRTY_NONE;
    updateDrawingTextures();
    if (m_fillDrawing) {
        m_fillDrawing->m_contourFill = (isContour()) && isFilled();
        if (m_subdivisions == 0 && !(isContour())) {
            m_fillDrawing->m_pathVertices.resize(m_vertexPositions.m_values[0]->size());
            m_fillDrawing->m_pathColors.resize(m_vertexPositions.m_values[0]->size());
            m_fillDrawing->m_pathTextureCoordinates.resize(m_vertexPositions.m_values[0]->size());
            m_fillDrawing->m_pathIndices.clear();
            for (int i = 0; i < (int)m_vertexPositions.m_values[0]->size(); ++i) {
                m_fillDrawing->m_pathVertices[i] = m_vertexPositions.value()[i];
                m_fillDrawing->m_pathColors[i] = m_vertexColors.value()[i];
                m_fillDrawing->m_pathTextureCoordinates[i] = m_vertexTexCoords.value()[i];
            }
            for (int face = 0; face < (int)m_faces.size(); ++face) {
                for (int corner = 1; corner < (int)m_faces[face].size() - 1; ++corner) {
                    m_fillDrawing->m_pathIndices.push_back(m_faces[face][0]);
                    m_fillDrawing->m_pathIndices.push_back(m_faces[face][corner]);
                    m_fillDrawing->m_pathIndices.push_back(m_faces[face][corner + 1]);
                }
            }
        }
    }
    if (m_subdivisions >= 0) {
        if (!(m_flags & (1 << MESH_NO_FILL)) && (isContour()) && m_fillDrawing) {
            m_fillDrawing->m_pathVertices.clear();
            m_fillDrawing->m_pathColors.erase(m_fillDrawing->m_pathColors.begin(), m_fillDrawing->m_pathColors.end());
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
            // Leaves empty and singular fits unchecked.
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
                        if (maximum.m_elements[component] < color.m_elements[component])
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
                    m_fillDrawing->m_pathContours[contour][i] = (int)m_fillDrawing->m_pathVertices.size();
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
                    m_extrusionDrawing->m_pathContours[contour][i] = (int)m_extrusionDrawing->m_pathVertices.size();
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
            m_strokeDrawing->m_pathColors.erase(m_strokeDrawing->m_pathColors.begin(),
                                                m_strokeDrawing->m_pathColors.end());
            m_strokeDrawing->m_pathTextureCoordinates.clear();
            m_strokeDrawing->m_pathIndices.clear();
            if (m_strokePattern == STROKE_PATTERN_SOLID || m_strokeGap == 0.0f) {
                for (int contour = 0; contour < (int)m_strokeOutlinePositions.size(); ++contour) {
                    int first = (int)m_strokeDrawing->m_pathVertices.size();
                    if (!isClosed()) {
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
                    if (!isClosed()) {
                        int last = (int)(m_strokeOutlinePositions[contour].size() - 4);
                        if (m_strokeCapType == STROKE_CAP_ROUND)
                            appendRoundCap(contour, last, 1.0f, 1, m_strokeSegments);
                        else if (m_strokeCapType == STROKE_CAP_SQUARE)
                            appendSquareCap(contour, last, 1.0f, true);
                    }
                    int count = (int)(m_strokeDrawing->m_pathVertices.size() - first);
                    int triangles = (!isClosed()) ? count - 2 : count;
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
    unlock();
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
