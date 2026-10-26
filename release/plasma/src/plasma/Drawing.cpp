#include "plasma/Drawing.h"
#include <cmath>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "plasma/EditFilter.h"
#include "plasma/SegmentLength.h"

namespace plasma {

Drawing::Drawing()
    : m_contourFill(false),
      m_texture(nullptr),
      m_dirty(DRAWING_DIRTY_ALL),
      m_unused12C(-1),
      m_unused1C0(nullptr),
      m_unused1D8(false)
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
    m_texture = nullptr;
    update();
}

namespace {

// The extrusion's edges are drawn back to front: the priority queue orders them by their middle's transformed distance
// along the extrusion.
struct DrawingExtrusion {
    DrawingExtrusion(std::vector<Vector<2, float>> &positions, const Matrix<float> &matrix)
        : m_positions(&positions),
          m_matrix(matrix)
    {}

    std::vector<Vector<2, float>> *m_positions;
    Matrix<float> m_matrix;
};

struct DrawingExtrusionEdge {
    DrawingExtrusionEdge(int first, int second, DrawingExtrusion &extrusion)
        : m_first(first),
          m_second(second),
          m_extrusion(&extrusion)
    {}

    DrawingExtrusionEdge(const DrawingExtrusionEdge &other)
        : m_first(other.m_first),
          m_second(other.m_second),
          m_extrusion(other.m_extrusion)
    {}

    DrawingExtrusionEdge &operator=(const DrawingExtrusionEdge &other)
    {
        m_first = other.m_first;
        m_second = other.m_second;
        m_extrusion = other.m_extrusion;
        return *this;
    }

    float priority() const;
    bool operator<(const DrawingExtrusionEdge &other) const
    {
        float firstPriority = priority();
        float secondPriority = other.priority();
        return firstPriority < secondPriority;
    }

    int m_first;
    int m_second;
    DrawingExtrusion *m_extrusion;
};

}

void Drawing::extrude(const Matrix<float> &matrix)
{
    tessellate();
    buildContours();
    m_pathIndices.clear();
    std::priority_queue<DrawingExtrusionEdge> edges;
    DrawingExtrusion extrusion(m_pathVertices, matrix);
    float area = 0.0f;
    for (int contourIndex = 0; contourIndex < (int)m_contours.size(); ++contourIndex) {
        if (m_contours[contourIndex].size() < 3)
            continue;
        int first = m_pathVertices.size();
        for (int i = 0; i < (int)m_contours[contourIndex].size(); ++i) {
            int index = m_contours[contourIndex][i];
            const Vector<2, float> position = m_pathVertices[index];
            const Vector<2, float> next =
                m_pathVertices[m_contours[contourIndex][(i + 1) % (int)m_contours[contourIndex].size()]];
            area += position.m_elements[0] * next.m_elements[1] - next.m_elements[0] * position.m_elements[1];
            m_pathVertices.push_back(position);
            m_pathColors.push_back(m_pathColors[index]);
            m_pathTextureCoordinates.push_back(Vector<2, float>(0.0f, 0.0f));
            Vector<2, float> transformed = matrix.transform(position);
            m_pathVertices.push_back(transformed);
            m_pathColors.push_back(m_pathBackColors[index]);
            m_pathTextureCoordinates.push_back(Vector<2, float>(0.0f, 0.0f));
        }
        for (int i = 0; i < (int)m_contours[contourIndex].size(); ++i)
            edges.push(DrawingExtrusionEdge(first + 2 * i, first + 2 * ((i + 1) % (int)m_contours[contourIndex].size()),
                                            extrusion));
    }
    while (!edges.empty()) {
        const DrawingExtrusionEdge &edge = edges.top();
        int first = edge.m_first;
        int second = edge.m_second;
        Vector<2, float> deformation = m_pathVertices[first + 1] - m_pathVertices[first];
        Vector<2, float> toward = m_pathVertices[second] - m_pathVertices[first];
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
        Vector<2, float> transformed = matrix.transform(m_pathVertices[i]);
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
            int next = (i + 1) % m_contours[contourIndex].size();
            // Only the upper bound is checked; negative indices are not.
            if (m_contours[contourIndex][i] >= (int)m_pathVertices.size() ||
                m_contours[contourIndex][next] >= (int)m_pathVertices.size())
                return false;
            const Vector<2, float> &second = m_pathVertices[m_contours[contourIndex][next]];
            if (((m_pathVertices[m_contours[contourIndex][i]].m_elements[1] < point.m_elements[1] &&
                  second.m_elements[1] >= point.m_elements[1]) ||
                 (second.m_elements[1] < point.m_elements[1] &&
                  m_pathVertices[m_contours[contourIndex][i]].m_elements[1] >= point.m_elements[1])) &&
                (point.m_elements[1] - m_pathVertices[m_contours[contourIndex][i]].m_elements[1]) /
                            (second.m_elements[1] - m_pathVertices[m_contours[contourIndex][i]].m_elements[1]) *
                            (second.m_elements[0] - m_pathVertices[m_contours[contourIndex][i]].m_elements[0]) +
                        m_pathVertices[m_contours[contourIndex][i]].m_elements[0] <
                    point.m_elements[0])
                inside = !inside;
        }
    }
    return inside;
}

// Squared distance from point to the segment first..second, used by intersects.
static float squaredDistanceToSegment(const Vector<2, float> &point, const Vector<2, float> &first,
                                      const Vector<2, float> &second)
{
    Vector<2, float> relative = point - first;
    Vector<2, float> direction = second - first;
    float length =
        direction.m_elements[0] * direction.m_elements[0] + direction.m_elements[1] * direction.m_elements[1];
    if (!(length < MinimumSegmentLengthSquared)) {
        float projection = direction.m_elements[0] * relative.m_elements[0];
        projection += direction.m_elements[1] * relative.m_elements[1];
        projection /= length;
        if (!(projection <= 0.0f)) {
            if (projection >= 1.0f)
                relative = point - second;
            else
                relative = relative - direction * projection;
        }
    }
    return relative.m_elements[0] * relative.m_elements[0] + relative.m_elements[1] * relative.m_elements[1];
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
    if (transformedPoint.m_elements[0] + transformedRadius < m_minimum.m_elements[0] ||
        transformedPoint.m_elements[1] + transformedRadius < m_minimum.m_elements[1] ||
        transformedPoint.m_elements[0] - transformedRadius > m_maximum.m_elements[0] ||
        transformedPoint.m_elements[1] - transformedRadius > m_maximum.m_elements[1])
        return false;
    float radiusSquared = radius * radius;
    bool inside = false;
    for (int contourIndex = 0; contourIndex < (int)m_contours.size(); ++contourIndex) {
        for (int i = 0; i < (int)m_contours[contourIndex].size(); ++i) {
            int next = (i + 1) % m_contours[contourIndex].size();
            // Only the upper bound is checked; negative indices are not.
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

namespace {

float DrawingExtrusionEdge::priority() const
{
    Vector<2, float> middle = ((*m_extrusion->m_positions)[m_first] + (*m_extrusion->m_positions)[m_second]) * 0.5f;
    Vector<2, float> transformed = m_extrusion->m_matrix.transform(middle);
    Vector<2, float> direction = transformed - middle;
    Vector<2, float> origin(m_extrusion->m_matrix.m_elements[12], m_extrusion->m_matrix.m_elements[13]);
    float lengthSquared =
        direction.m_elements[0] * direction.m_elements[0] + direction.m_elements[1] * direction.m_elements[1];
    if (lengthSquared > 0.0f) {
        float inverseLength = 1.0f / std::sqrt(lengthSquared);
        direction.m_elements[0] *= inverseLength;
        direction.m_elements[1] *= inverseLength;
    }
    Vector<2, float> relative = transformed - origin;
    return relative.dot(direction);
}

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

void __stdcall combineDrawingVertex(double *coordinates, void **vertices, float *weights, void **result, void *polygon)
{
    Drawing *drawing = static_cast<Drawing *>(polygon);
    Vector<2, float> position(0.0f, 0.0f);
    Vector<4, float> color(0.0f, 0.0f, 0.0f, 0.0f);
    Vector<4, float> backColor(0.0f, 0.0f, 0.0f, 0.0f);
    Vector<2, float> textureCoordinate(0.0f, 0.0f);
    for (int i = 0; i < 4; ++i) {
        int index = (int)(INT_PTR)vertices[i];
        position += drawing->m_pathVertices[index] * weights[i];
        color += drawing->m_pathColors[index] * weights[i];
        if (index < (int)drawing->m_pathBackColors.size())
            backColor += drawing->m_pathBackColors[index] * weights[i];
        textureCoordinate += drawing->m_pathTextureCoordinates[index] * weights[i];
    }
    int index = drawing->m_pathVertices.size();
    drawing->m_pathVertices.push_back(position);
    drawing->m_pathColors.push_back(color);
    if (!drawing->m_pathBackColors.empty())
        drawing->m_pathBackColors.push_back(backColor);
    drawing->m_pathTextureCoordinates.push_back(textureCoordinate);
    *result = (void *)(INT_PTR)index;
}

void __stdcall appendDrawingIndex(void *vertex, void *polygon)
{
    int index = (int)(INT_PTR)vertex;
    static_cast<Drawing *>(polygon)->m_pathIndices.push_back(index);
}

// GLU_TESS_EDGE_FLAG: registered so that the tessellator emits only triangles; the flags themselves are not needed.
static void __stdcall ignoreEdgeFlag(GLboolean flag) {}

void Drawing::tessellate()
{
    GLUtesselator *tessellator = gluNewTess();
    gluTessCallback(tessellator, GLU_TESS_VERTEX_DATA, (void(__stdcall *)())appendDrawingIndex);
    gluTessCallback(tessellator, GLU_TESS_COMBINE_DATA, (void(__stdcall *)())combineDrawingVertex);
    gluTessCallback(tessellator, GLU_TESS_EDGE_FLAG, (void(__stdcall *)())ignoreEdgeFlag);
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
                gluTessVertex(tessellator, coordinates.m_elements, (void *)(INT_PTR)index);
            }
            gluTessEndContour(tessellator);
        }
        gluTessEndPolygon(tessellator);
        gluDeleteTess(tessellator);
    }
    // The tessellator leaks when there are no contours (as in plasma).
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

void Drawing::reset() {}

}
