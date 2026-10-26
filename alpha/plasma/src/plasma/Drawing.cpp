// Drawing builds render buffers and bounds from shape paths.
#include "plasma/Drawing.h"
#include <algorithm>
#include <cmath>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "plasma/Geometry.h"

namespace plasma {
// An edge's antialiasing normal is looked for along at most MaximumNormalSearchAttempts neighbours
// while its squared length is below 1e-12 (anonymous: this file's own).
namespace {
enum {
    MaximumNormalSearchAttempts = 10
};
}

// GLU tessellation callbacks.
void __stdcall appendDrawingIndex(void *vertex, void *polygon);
void __stdcall combineDrawingVertex(double *coordinates, void **vertices, float *weights, void **result, void *polygon);
void __stdcall tessellationEdgeFlag(unsigned char flag);

namespace {

struct DrawingExtrusion {
    DrawingExtrusion(std::vector<Vector<2, float>> &positions, const Matrix<float> &matrix);

    std::vector<Vector<2, float>> *m_positions;
    Matrix<float> m_matrix;
};

struct DrawingExtrusionEdge {
    DrawingExtrusionEdge(int first, int second, DrawingExtrusion &extrusion);
    DrawingExtrusionEdge(const DrawingExtrusionEdge &other);
    void operator=(const DrawingExtrusionEdge &other);
    // Projected edge priority for extrusion ordering.
    float priority() const;
    bool operator<(const DrawingExtrusionEdge &other) const;

    int m_first;
    int m_second;
    DrawingExtrusion *m_extrusion;
};

DrawingExtrusion::DrawingExtrusion(std::vector<Vector<2, float>> &positions, const Matrix<float> &matrix)
    : m_positions(&positions),
      m_matrix(matrix)
{}

DrawingExtrusionEdge::DrawingExtrusionEdge(const DrawingExtrusionEdge &other)
    : m_first(other.m_first),
      m_second(other.m_second),
      m_extrusion(other.m_extrusion)
{}

DrawingExtrusionEdge::DrawingExtrusionEdge(int first, int second, DrawingExtrusion &extrusion)
    : m_first(first),
      m_second(second),
      m_extrusion(&extrusion)
{}

float DrawingExtrusionEdge::priority() const
{
    Vector<2, float> middle = ((*m_extrusion->m_positions)[m_first] + (*m_extrusion->m_positions)[m_second]) * 0.5;
    float w = m_extrusion->m_matrix.m_elements[3] * middle[0] + m_extrusion->m_matrix.m_elements[7] * middle[1] +
              m_extrusion->m_matrix.m_elements[15];
    Vector<2, float> projected(
        m_extrusion->m_matrix.m_elements[4] * middle[1] + middle[0] * m_extrusion->m_matrix.m_elements[0] +
            m_extrusion->m_matrix.m_elements[12],
        m_extrusion->m_matrix.m_elements[5] * middle[1] + m_extrusion->m_matrix.m_elements[1] * middle[0] +
            m_extrusion->m_matrix.m_elements[13]);
    Vector<2, float> transformed = projected * (1 / w);
    Vector<2, float> direction = transformed - middle;
    Vector<2, float> origin(m_extrusion->m_matrix.m_elements[12], m_extrusion->m_matrix.m_elements[13]);
    float lengthSquared =
        direction.m_elements[0] * direction.m_elements[0] + direction.m_elements[1] * direction.m_elements[1];
    if (lengthSquared > 0.0f) {
        float inverseLength = 1.0 / std::sqrt(lengthSquared);
        direction.m_elements[0] *= inverseLength;
        direction.m_elements[1] *= inverseLength;
    }
    Vector<2, float> relative = transformed - origin;
    float dot = relative[0] * direction[0];
    for (int axis = 1; axis < 2; ++axis)
        dot += relative[axis] * direction[axis];
    return dot;
}

bool DrawingExtrusionEdge::operator<(const DrawingExtrusionEdge &other) const
{
    double firstPriority = priority();
    double secondPriority = other.priority();
    return firstPriority < secondPriority;
}

void DrawingExtrusionEdge::operator=(const DrawingExtrusionEdge &other)
{
    m_first = other.m_first;
    m_second = other.m_second;
    m_extrusion = other.m_extrusion;
}

}

Drawing::Drawing()
    : m_contourFill(false),
      m_texture(0),
      m_dirty(DRAWING_DIRTY_ALL),
      m_unusedc0(-1),
      m_unused10c(0),
      m_unused120(false)
{}

Drawing::~Drawing() {}

void Drawing::clear()
{
    m_pathVertices.clear();
    m_pathColors.clear();
    m_pathTextureCoordinates.clear();
    m_pathIndices.clear();
    m_pathContours.clear();
    m_contours.clear();
    m_texture = 0;
    update();
}

void Drawing::extrude(const Matrix<float> &matrix)
{
    tessellate();
    buildContours();
    m_pathIndices.erase(m_pathIndices.begin(), m_pathIndices.end());
    std::priority_queue<DrawingExtrusionEdge> edges;
    DrawingExtrusion extrusion(m_pathVertices, matrix);
    float area = 0.0f;
    for (int contourIndex = 0; contourIndex < (int)m_contours.size(); ++contourIndex) {
        const std::vector<int> &contour = m_contours[contourIndex];
        if (contour.size() < 3)
            continue;
        int first = (int)m_pathVertices.size();
        for (int i = 0; i < (int)contour.size(); ++i) {
            int index = contour[i];
            const Vector<2, float> position = m_pathVertices[index];
            const Vector<2, float> next = m_pathVertices[contour[(i + 1) % (int)contour.size()]];
            float cross = position.m_elements[0] * next.m_elements[1] - next.m_elements[0] * position.m_elements[1];
            area += cross;
            m_pathVertices.push_back(position);
            m_pathColors.push_back(m_pathColors[index]);
            m_pathTextureCoordinates.push_back(Vector<2, float>(0.0f, 0.0f));
            Vector<2, float> transformed = matrix.transform(position);
            m_pathVertices.push_back(transformed);
            // The transformed side uses the alternate colour array.
            m_pathColors.push_back(m_pathBackColors[index]);
            m_pathTextureCoordinates.push_back(Vector<2, float>(0.0f, 0.0f));
        }
        for (int i = 0; i < (int)contour.size(); ++i)
            edges.push(DrawingExtrusionEdge(first + 2 * i, first + 2 * ((i + 1) % (int)contour.size()), extrusion));
    }
    while (!edges.empty()) {
        const DrawingExtrusionEdge &edge = edges.top();
        int first = edge.m_first;
        int second = edge.m_second;
        const Vector<2, float> &position = m_pathVertices[first];
        Vector<2, float> deformation = m_pathVertices[first + 1] - position;
        Vector<2, float> toward = m_pathVertices[second] - position;
        float cross =
            toward.m_elements[0] * deformation.m_elements[1] - toward.m_elements[1] * deformation.m_elements[0];
        if (cross * area < 0.0f) {
            m_pathIndices.push_back(first);
            m_pathIndices.push_back(first + 1);
            m_pathIndices.push_back(second);
            m_pathIndices.push_back(second);
            m_pathIndices.push_back(first + 1);
            m_pathIndices.push_back(second + 1);
        }
        edges.pop();
    }
    buildContours();
}

void Drawing::buildContours()
{
    m_contours.clear();
    typedef std::pair<int, int> Edge;
    std::set<Edge> boundary;
    for (int triangle = 0; triangle < (int)m_pathIndices.size(); triangle += 3) {
        for (int corner = 0; corner < 3; ++corner) {
            int first = m_pathIndices[triangle + corner];
            int second = m_pathIndices[triangle + (corner + 1) % 3];
            std::set<Edge>::iterator reversed = boundary.find(Edge(second, first));
            if (reversed == boundary.end())
                boundary.insert(Edge(first, second));
            else
                boundary.erase(reversed);
        }
    }
    std::map<int, std::list<int>> adjacency;
    for (int triangle = 0; triangle < (int)m_pathIndices.size(); triangle += 3) {
        for (int corner = 0; corner < 3; ++corner) {
            int first = m_pathIndices[triangle + corner];
            int second = m_pathIndices[triangle + (corner + 1) % 3];
            if (boundary.find(Edge(first, second)) != boundary.end())
                adjacency[first].push_back(second);
        }
    }
    while (!adjacency.empty()) {
        m_contours.push_back(std::vector<int>());
        int vertex = adjacency.begin()->first;
        const int start = vertex;
        do {
            if (adjacency.find(vertex) == adjacency.end())
                break;
            std::list<int> &nextVertices = adjacency[vertex];
            int next = nextVertices.front();
            nextVertices.pop_front();
            if (nextVertices.empty())
                adjacency.erase(vertex);
            m_contours.back().push_back(vertex);
            vertex = next;
        } while (vertex != start);
    }
}

void Drawing::getTransformedBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix,
                                   bool &first) const
{
    for (int i = 0; i < (int)m_pathVertices.size(); ++i) {
        const Vector<2, float> &point = m_pathVertices[i];
        float w = matrix[3] * point[0] + matrix[7] * point[1] + matrix[15];
        Vector<2, float> transformed = Vector<2, float>(matrix[4] * point[1] + matrix[0] * point[0] + matrix[12],
                                                        matrix[1] * point[0] + matrix[5] * point[1] + matrix[13]) *
                                       (1 / w);
        if (first) {
            minimum = transformed;
            maximum = transformed;
            first = false;
        } else {
            minimum.m_elements[0] = (std::min)(minimum.m_elements[0], transformed.m_elements[0]);
            minimum.m_elements[1] = (std::min)(minimum.m_elements[1], transformed.m_elements[1]);
            maximum.m_elements[0] = (std::max)(maximum.m_elements[0], transformed.m_elements[0]);
            maximum.m_elements[1] = (std::max)(maximum.m_elements[1], transformed.m_elements[1]);
        }
    }
}

bool Drawing::contains(const Vector<2, float> &point) const
{
    if (point.m_elements[0] < m_minimum.m_elements[0] || point.m_elements[1] < m_minimum.m_elements[1] ||
        point.m_elements[0] > m_maximum.m_elements[0] || point.m_elements[1] > m_maximum.m_elements[1])
        return false;
    bool inside = false;
    for (int contourIndex = 0; contourIndex < (int)m_contours.size(); ++contourIndex) {
        for (int i = 0; i < (int)m_contours[contourIndex].size(); ++i) {
            unsigned next = (i + 1) % m_contours[contourIndex].size();
            // Code checks the upper bound only; negative indices remain unchecked.
            if (m_contours[contourIndex][i] >= (int)m_pathVertices.size() ||
                m_contours[contourIndex][next] >= (int)m_pathVertices.size())
                return false;
            const Vector<2, float> &first = m_pathVertices[m_contours[contourIndex][i]];
            const Vector<2, float> &second = m_pathVertices[m_contours[contourIndex][next]];
            if (((first.m_elements[1] < point.m_elements[1] && second.m_elements[1] >= point.m_elements[1]) ||
                 (second.m_elements[1] < point.m_elements[1] && first.m_elements[1] >= point.m_elements[1])) &&
                (point.m_elements[1] - first.m_elements[1]) / (second.m_elements[1] - first.m_elements[1]) *
                            (second.m_elements[0] - first.m_elements[0]) +
                        first.m_elements[0] <
                    point.m_elements[0])
                inside = !inside;
        }
    }
    return inside;
}

bool Drawing::intersects(const Vector<2, float> &point, float radius, const Matrix<float> &drawingMatrix,
                         const Matrix<float> &pointMatrix) const
{
    Vector<2, float> xDirection = pointMatrix.transformDirection(Vector<2, float>(1.0f, 0.0f));
    Vector<2, float> yDirection = pointMatrix.transformDirection(Vector<2, float>(0.0f, 1.0f));
    float yLengthSquared = yDirection[0] * yDirection[0] + yDirection[1] * yDirection[1];
    float yLength = std::sqrt(yLengthSquared);
    float xLengthSquared = xDirection[0] * xDirection[0] + xDirection[1] * xDirection[1];
    float xLength = std::sqrt(xLengthSquared);
    float transformedRadius = radius * (std::max)(xLength, yLength);
    Vector<2, float> transformedPoint = pointMatrix.transform(point);
    // Ordered tests on unrounded sums : a NaN coordinate does not reject.
    if (transformedPoint.m_elements[0] + transformedRadius < m_minimum.m_elements[0] ||
        transformedPoint.m_elements[1] + transformedRadius < m_minimum.m_elements[1] ||
        transformedPoint.m_elements[0] - transformedRadius > m_maximum.m_elements[0] ||
        transformedPoint.m_elements[1] - transformedRadius > m_maximum.m_elements[1])
        return false;
    float radiusSquared = radius * radius;
    bool inside = false;
    for (int contourIndex = 0; contourIndex < (int)m_contours.size(); ++contourIndex) {
        for (int i = 0; i < (int)m_contours[contourIndex].size(); ++i) {
            unsigned next = (i + 1) % m_contours[contourIndex].size();
            // Code checks the upper bound only; negative indices remain unchecked.
            if (m_contours[contourIndex][i] >= (int)m_pathVertices.size() ||
                m_contours[contourIndex][next] >= (int)m_pathVertices.size())
                return false;
            Vector<2, float> first = drawingMatrix.transform(m_pathVertices[m_contours[contourIndex][i]]);
            Vector<2, float> second = drawingMatrix.transform(m_pathVertices[m_contours[contourIndex][next]]);
            float distance = squaredDistanceToSegment(point, first, second);
            if (radiusSquared >= distance)
                return true;
            if (((point.m_elements[1] > first.m_elements[1] && point.m_elements[1] <= second.m_elements[1]) ||
                 (point.m_elements[1] > second.m_elements[1] && point.m_elements[1] <= first.m_elements[1])) &&
                first.m_elements[0] +
                        (second.m_elements[0] - first.m_elements[0]) * ((point.m_elements[1] - first.m_elements[1]) /
                                                                        (second.m_elements[1] - first.m_elements[1])) <
                    point.m_elements[0])
                inside = !inside;
        }
    }
    return inside;
}

void Drawing::reserveInput()
{
    m_pathVertices.reserve(m_pathVertices.size());
    m_pathColors.reserve(m_pathColors.size());
    m_pathTextureCoordinates.reserve(m_pathTextureCoordinates.size());
    m_pathIndices.reserve(m_pathIndices.size());
    m_pathContours.reserve(m_pathContours.size());
    for (int i = 0; i < (int)m_pathContours.size(); ++i)
        m_pathContours[i].reserve(m_pathContours[i].size());
    m_contours.reserve(m_contours.size());
    for (int i = 0; i < (int)m_contours.size(); ++i)
        m_contours[i].reserve(m_contours[i].size());
}

void CALLBACK combineDrawingVertex(double *coordinates, void **vertices, float *weights, void **result, void *polygon)
{
    Drawing *drawing = static_cast<Drawing *>(polygon);
    Vector<2, float> position(0.0f, 0.0f);
    Vector<4, float> color(0.0f, 0.0f, 0.0f, 0.0f);
    Vector<4, float> backColor(0.0f, 0.0f, 0.0f, 0.0f);
    Vector<2, float> textureCoordinate(0.0f, 0.0f);
    for (int i = 0; i < 4; ++i) {
        int vertex = (int)reinterpret_cast<intptr_t>(vertices[i]);
        position += drawing->m_pathVertices[vertex] * weights[i];
        color += drawing->m_pathColors[vertex] * weights[i];
        if (vertex < (int)drawing->m_pathBackColors.size())
            backColor += drawing->m_pathBackColors[vertex] * weights[i];
        textureCoordinate += drawing->m_pathTextureCoordinates[vertex] * weights[i];
    }
    int index = (int)drawing->m_pathVertices.size();
    drawing->m_pathVertices.push_back(position);
    drawing->m_pathColors.push_back(color);
    if (!drawing->m_pathBackColors.empty())
        drawing->m_pathBackColors.push_back(backColor);
    drawing->m_pathTextureCoordinates.push_back(textureCoordinate);
    *result = reinterpret_cast<void *>((intptr_t)index);
}

void __stdcall tessellationEdgeFlag(unsigned char) {}

void CALLBACK appendDrawingIndex(void *vertex, void *polygon)
{
    int index = (int)reinterpret_cast<intptr_t>(vertex);
    static_cast<Drawing *>(polygon)->m_pathIndices.push_back(index);
}

void Drawing::tessellate()
{
    GLUtesselator *tessellator = gluNewTess();
    gluTessCallback(tessellator, GLU_TESS_VERTEX_DATA, reinterpret_cast<void(__stdcall *)()>(appendDrawingIndex));
    gluTessCallback(tessellator, GLU_TESS_COMBINE_DATA, reinterpret_cast<void(__stdcall *)()>(combineDrawingVertex));
    gluTessCallback(tessellator, GLU_TESS_EDGE_FLAG, reinterpret_cast<void(__stdcall *)()>(tessellationEdgeFlag));
    gluTessProperty(tessellator, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
    if (!m_pathContours.empty()) {
        m_pathIndices.clear();
        gluTessBeginPolygon(tessellator, this);
        for (int face = 0; face < (int)m_pathContours.size(); ++face) {
            gluTessBeginContour(tessellator);
            for (int i = 0; i < (int)m_pathContours[face].size(); ++i) {
                int index = m_pathContours[face][i];
                Vector<3, GLdouble> coordinates(m_pathVertices[index].m_elements[0],
                                                m_pathVertices[index].m_elements[1], 0.0);
                gluTessVertex(tessellator, coordinates.m_elements, reinterpret_cast<void *>(index));
            }
            gluTessEndContour(tessellator);
        }
        gluTessEndPolygon(tessellator);
        gluDeleteTess(tessellator);
    }
    // Code leaks the tessellator when there are no faces.
}

void Drawing::update()
{
    if (m_pathVertices.empty()) {
        m_minimum = Vector<2, float>(0.0f, 0.0f);
        m_maximum = Vector<2, float>(0.0f, 0.0f);
    } else {
        m_minimum = m_pathVertices[0];
        m_maximum = m_minimum;
        for (int i = 1; i < (int)m_pathVertices.size(); ++i) {
            const Vector<2, float> &point = m_pathVertices[i];
            if (m_minimum.m_elements[0] > point.m_elements[0])
                m_minimum.m_elements[0] = point.m_elements[0];
            if (m_minimum.m_elements[1] > point.m_elements[1])
                m_minimum.m_elements[1] = point.m_elements[1];
            if (point.m_elements[0] > m_maximum.m_elements[0])
                m_maximum.m_elements[0] = point.m_elements[0];
            if (point.m_elements[1] > m_maximum.m_elements[1])
                m_maximum.m_elements[1] = point.m_elements[1];
        }
        reserveInput();
    }
}

void Drawing::buildBuffers()
{
    m_vertices = m_pathVertices;
    m_colors = m_pathColors;
    m_textureCoordinates = m_pathTextureCoordinates;
    m_normals.resize(m_vertices.size());
    for (int i = 0; i < (int)m_normals.size(); ++i)
        m_normals[i] = Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
    m_indices = m_pathIndices;
    if (!m_contours.empty()) {
        float area = 0.0f;
        for (int i = 0; i < (int)m_pathIndices.size(); i += 3) {
            Vector<2, float> first = m_pathVertices[m_pathIndices[i]];
            Vector<2, float> second = m_pathVertices[m_pathIndices[i + 1]];
            Vector<2, float> third = m_pathVertices[m_pathIndices[i + 2]];
            Vector<3, float> a(third[0], third[1], 0.0f);
            Vector<3, float> b(second[0], second[1], 0.0f);
            Vector<3, float> origin(first[0], first[1], 0.0f);
            a -= origin;
            b -= origin;
            float cross = a.m_elements[1] * b.m_elements[0] - a.m_elements[0] * b.m_elements[1];
            area += cross;
        }
        for (int contourIndex = 0; contourIndex < (int)m_contours.size(); ++contourIndex) {
            const std::vector<int> &contour = m_contours[contourIndex];
            if (contour.size() < 3)
                continue;
            std::vector<int> pairs;
            for (int i = 0; i < (int)contour.size(); ++i) {
                const Vector<2, float> &position = m_pathVertices[contour[i]];
                size_t previous = (contour.size() + i - 1) % contour.size();
                unsigned next = (i + 1) % contour.size();
                Vector<3, float> incoming(position.m_elements[0] - m_pathVertices[contour[previous]].m_elements[0],
                                          position.m_elements[1] - m_pathVertices[contour[previous]].m_elements[1],
                                          0.0f);
                Vector<3, float> outgoing(position.m_elements[0] - m_pathVertices[contour[next]].m_elements[0],
                                          position.m_elements[1] - m_pathVertices[contour[next]].m_elements[1], 0.0f);
                Vector<3, float> negativeAxis(0.0f, 0.0f, -area);
                Vector<3, float> positiveAxis(0.0f, 0.0f, area);
                Vector<3, float> firstNormal =
                    Vector<3, float>(incoming[1] * negativeAxis[2] - incoming[2] * negativeAxis[1],
                                     incoming[2] * negativeAxis[0] - incoming[0] * negativeAxis[2],
                                     incoming[0] * negativeAxis[1] - incoming[1] * negativeAxis[0]);
                Vector<3, float> secondNormal =
                    Vector<3, float>(outgoing[1] * positiveAxis[2] - outgoing[2] * positiveAxis[1],
                                     outgoing[2] * positiveAxis[0] - outgoing[0] * positiveAxis[2],
                                     outgoing[0] * positiveAxis[1] - outgoing[1] * positiveAxis[0]);
                for (int attempt = 0; (firstNormal[0] * firstNormal[0] + firstNormal[1] * firstNormal[1] +
                                       firstNormal[2] * firstNormal[2]) < 1.0e-12f;
                     ++attempt) {
                    if (previous == (unsigned)i || attempt >= MaximumNormalSearchAttempts)
                        break;
                    previous = (contour.size() + previous - 1) % contour.size();
                    incoming = Vector<3, float>(
                        position.m_elements[0] - m_pathVertices[contour[previous]].m_elements[0],
                        position.m_elements[1] - m_pathVertices[contour[previous]].m_elements[1], 0.0f);
                    firstNormal = Vector<3, float>(incoming[1] * negativeAxis[2] - incoming[2] * negativeAxis[1],
                                                   incoming[2] * negativeAxis[0] - incoming[0] * negativeAxis[2],
                                                   incoming[0] * negativeAxis[1] - incoming[1] * negativeAxis[0]);
                }
                for (int attempt = 0; (secondNormal[0] * secondNormal[0] + secondNormal[1] * secondNormal[1] +
                                       secondNormal[2] * secondNormal[2]) < 1.0e-12f;
                     ++attempt) {
                    if (next == (unsigned)i || attempt >= MaximumNormalSearchAttempts)
                        break;
                    next = (next + 1) % contour.size();
                    outgoing =
                        Vector<3, float>(position.m_elements[0] - m_pathVertices[contour[next]].m_elements[0],
                                         position.m_elements[1] - m_pathVertices[contour[next]].m_elements[1], 0.0f);
                    // Fallback changes from the positive axis to the negative one.
                    secondNormal = Vector<3, float>(outgoing[1] * negativeAxis[2] - outgoing[2] * negativeAxis[1],
                                                    outgoing[2] * negativeAxis[0] - outgoing[0] * negativeAxis[2],
                                                    outgoing[0] * negativeAxis[1] - outgoing[1] * negativeAxis[0]);
                }
                if ((firstNormal[0] * firstNormal[0] + firstNormal[1] * firstNormal[1] +
                     firstNormal[2] * firstNormal[2]) <= 0.0f)
                    firstNormal = secondNormal;
                if ((secondNormal[0] * secondNormal[0] + secondNormal[1] * secondNormal[1] +
                     secondNormal[2] * secondNormal[2]) <= 0.0f)
                    secondNormal = firstNormal;
                float dot = firstNormal[0] * secondNormal[0] + firstNormal[1] * secondNormal[1] +
                            firstNormal[2] * secondNormal[2];
                if (dot >= 0.0f) {
                    pairs.push_back(contour[i]);
                    pairs.push_back((int)m_vertices.size());
                    m_vertices.push_back(position);
                    Vector<4, float> color = m_pathColors[contour[i]];
                    color.m_elements[3] *= 0.0f;
                    m_colors.push_back(color);
                    m_textureCoordinates.push_back(m_pathTextureCoordinates[contour[i]]);
                    // Scale the packed normal, preserving NaN arithmetic.
                    m_normals.push_back(Vector<4, float>(firstNormal.m_elements[0], firstNormal.m_elements[1],
                                                         secondNormal.m_elements[0], secondNormal.m_elements[1]) *
                                        -1.0f);
                } else {
                    pairs.push_back(contour[i]);
                    pairs.push_back((int)m_vertices.size());
                    m_vertices.push_back(position);
                    Vector<4, float> color = m_pathColors[contour[i]];
                    color.m_elements[3] *= 0.0f;
                    m_colors.push_back(color);
                    m_textureCoordinates.push_back(m_pathTextureCoordinates[contour[i]]);
                    m_normals.push_back(Vector<4, float>(firstNormal.m_elements[0], firstNormal.m_elements[1],
                                                         firstNormal.m_elements[0], firstNormal.m_elements[1]) *
                                        -1.0f);
                    pairs.push_back(-1);
                    pairs.push_back(-1);
                    pairs.push_back(contour[i]);
                    pairs.push_back((int)m_vertices.size());
                    m_vertices.push_back(position);
                    color = m_pathColors[contour[i]];
                    color.m_elements[3] *= 0.0f;
                    m_colors.push_back(color);
                    m_textureCoordinates.push_back(m_pathTextureCoordinates[contour[i]]);
                    m_normals.push_back(Vector<4, float>(secondNormal.m_elements[0], secondNormal.m_elements[1],
                                                         secondNormal.m_elements[0], secondNormal.m_elements[1]) *
                                        -1.0f);
                }
            }
            for (int i = 0; i < (int)pairs.size(); i += 2) {
                int next = (i + 2) % pairs.size();
                if (pairs[i] >= 0 && pairs[next] >= 0) {
                    m_indices.push_back(pairs[i]);
                    m_indices.push_back(pairs[i + 1]);
                    m_indices.push_back(pairs[next]);
                    m_indices.push_back(pairs[next]);
                    m_indices.push_back(pairs[i + 1]);
                    m_indices.push_back(pairs[(next + 1) % pairs.size()]);
                }
            }
        }
    }
    // Every path ends here : the triangles' index order is reversed, their winding with it.
    std::reverse(m_indices.begin(), m_indices.end());
}

// This drawing hook does nothing.
void Drawing::reset() {}

}
