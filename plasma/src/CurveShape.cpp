#include "plasma/CurveShape.h"
#include "plasma/MeshShape.h"
#include "plasma/Drawing.h"
#include "plasma/Engine.h"
#include "MeshInterface.h"
#include <algorithm>
#include <cmath>

namespace plasma {

static const int DefaultCurveStrokeSegments = 20;
static const int DefaultCurveSubdivisions = 4;
static const int MaxCurveSubdivisions = 100;
static const int LegacyStrokeSetterSubdivisionLimit = 1000;

CurveShape::CurveShape(Engine *engine, const std::wstring &name)
    : Shape(engine, name),
      m_vertexPositions("vertexPositions", std::vector<Vector<2, float>>()),
      m_vertexWidths("vertexWidths", std::vector<float>()),
      m_vertexTexCoords("vertexTexCoords", std::vector<Vector<2, float>>()),
      m_vertexColors("vertexColors", std::vector<Vector<4, float>>()),
      m_textureTranslation("textureTranslation"),
      m_texturePivot("texturePivot"),
      m_textureRotation("textureRotation"),
      m_textureDeformation("textureDeformation"),
      m_textureWeights("textureWeights", std::vector<float>()),
      m_closed(false),
      m_strokeCapType(STROKE_CAP_ROUND),
      m_subdivisions(DefaultCurveSubdivisions),
      m_strokeSegments(DefaultCurveStrokeSegments)
{
    // The constructor copies uninitialized vector/matrix defaults.
    addAttribute(&m_vertexPositions);
    addAttribute(&m_vertexTexCoords);
    addAttribute(&m_vertexColors);
    addAttribute(&m_vertexWidths);
    addAttribute(&m_textureTranslation);
    addAttribute(&m_texturePivot);
    addAttribute(&m_textureRotation);
    addAttribute(&m_textureDeformation);
    addAttribute(&m_textureWeights);
    m_textureDeformation.value().identity();
    m_drawing = static_cast<Drawing *>(engine->createDrawing());
}

CurveShape::~CurveShape()
{
    Engine::deleteDrawing(m_drawing);
}

void CurveShape::setVertexFlags(const std::vector<int> &flags)
{
    m_vertexFlags = flags;
}

bool CurveShape::contains(const Vector<2, float> &point)
{
    if (point.m_elements[0] < (double)m_minimum.m_elements[0])
        return false;
    if (point.m_elements[1] < (double)m_minimum.m_elements[1])
        return false;
    if (point.m_elements[0] > (double)m_maximum.m_elements[0])
        return false;
    if (point.m_elements[1] > (double)m_maximum.m_elements[1])
        return false;
    int count = (int)m_outlinePositions.size() - 2;
    if (m_closed)
        count += 2;
    for (int vertex = 0; vertex < count; vertex++) {
        // The strip test reads three consecutive vertices. Closed strips rely on their appended closing vertices,
        // including this UB if an incomplete strip is tested before AttributesChanged.
        if (::isPointInTriangle(&m_outlinePositions[vertex % m_outlinePositions.size()], point))
            return true;
    }
    if (!m_closed) {
        for (int vertex = 1; vertex < (int)m_startCap.size() - 1; vertex++) {
            Vector<2, float> points[3] = {m_startCap[0], m_startCap[vertex], m_startCap[vertex + 1]};
            if (::isPointInTriangle(points, point))
                return true;
        }
        for (int vertex = 1; vertex < (int)m_endCap.size() - 1; vertex++) {
            Vector<2, float> points[3] = {m_endCap[0], m_endCap[vertex], m_endCap[vertex + 1]};
            if (::isPointInTriangle(points, point))
                return true;
        }
    }
    return false;
}

Vector<2, float> CurveShape::getMinimum()
{
    return m_minimum;
}

Vector<2, float> CurveShape::getMaximum()
{
    return m_maximum;
}

void CurveShape::attributesChanged(int)
{
    if (static_cast<int>(m_vertexPositions.m_values[0]->size()) < 2) {
        m_outlinePositions.clear();
        m_centers.clear();
        m_colors.erase(m_colors.begin(), m_colors.end());
        m_widths.erase(m_widths.begin(), m_widths.end());
        m_startCap.clear();
        m_endCap.clear();
        return;
    }
    if (m_vertexFlags.size() != m_vertexPositions.m_values[0]->size())
        m_vertexFlags.resize(m_vertexPositions.m_values[0]->size());
    if (!m_strokeCapType || m_closed || m_strokeSegments < 2) {
        m_startCap.clear();
        m_endCap.clear();
    } else if (m_strokeCapType == 1) {
        m_startCap.resize(m_strokeSegments);
        m_endCap.resize(m_strokeSegments);
        Vector<2, float> direction = m_vertexPositions.value()[0] - m_vertexPositions.value()[1];
        // Normalizes the vector while keeping the component arithmetic in the same order.
        float inverseLength = 1.0 / std::sqrt(direction[0] * direction[0] + direction[1] * direction[1]);
        direction = direction * inverseLength;
        direction = direction * (m_vertexWidths.value()[0] * 0.5);
        Vector<2, float> normal(-direction[1], direction[0]);
        for (int segment = 0; segment < m_strokeSegments; segment++) {
            float angle = segment * (double)3.1415927f / (m_strokeSegments - 1.0);
            m_startCap[segment] = m_vertexPositions.value()[0] + normal * (float)std::cos((double)angle) +
                                  direction * (float)std::sin((double)angle);
        }
        int last = m_vertexPositions.m_values[0]->size() - 1;
        direction = m_vertexPositions.value()[last] - m_vertexPositions.value()[last - 1];
        inverseLength = 1.0 / std::sqrt(direction[0] * direction[0] + direction[1] * direction[1]);
        direction = direction * inverseLength;
        direction = direction * (m_vertexWidths.value()[last] * 0.5);
        normal = Vector<2, float>(-direction[1], direction[0]);
        for (int segment = 0; segment < m_strokeSegments; segment++) {
            float angle = segment * (double)3.1415927f / (m_strokeSegments - 1.0);
            m_endCap[segment] = m_vertexPositions.value()[last] + normal * (float)std::cos((double)angle) +
                                direction * (float)std::sin((double)angle);
        }
    }
    int steps = (int)std::pow(2.0, m_subdivisions);
    unsigned refinedCount = m_closed ? m_vertexPositions.m_values[0]->size() * steps
                                     : (m_vertexPositions.m_values[0]->size() - 1) * steps + 1;
    m_outlinePositions.resize(2 * refinedCount);
    m_colors.resize(refinedCount);
    m_widths.resize(refinedCount);
    m_centers.resize(refinedCount);
    for (int vertex = 0; vertex < (int)m_vertexPositions.m_values[0]->size(); vertex++) {
        m_centers[vertex * steps] = m_vertexPositions.value()[vertex];
        m_colors[vertex * steps] = m_vertexColors.value()[vertex];
        m_widths[vertex * steps] = m_vertexWidths.value()[vertex];
    }
    for (int level = 0; level < m_subdivisions; level++) {
        int stride = (int)std::pow(2.0, m_subdivisions - level);
        int flagStride = (int)std::pow(2.0, level);
        int segments = m_centers.size() / stride;
        refineCurvePositions(m_centers, 0, stride, flagStride, segments, m_closed, m_vertexFlags);
        refineCurveColors(m_colors, stride, flagStride, segments, m_closed, 0, m_vertexFlags);
        refineCurveWidths(0, m_widths, stride, flagStride, segments, m_closed, m_vertexFlags);

        // The code refines both sides before replacing them with the stroke offsets below, including reads of their
        // previous contents.
        refineCurvePositions(m_outlinePositions, 0, 2 * stride, flagStride, segments, m_closed, m_vertexFlags);
        refineCurvePositions(m_outlinePositions, 1, 2 * stride, flagStride, segments, m_closed, m_vertexFlags);
    }
    for (int vertex = 0; vertex < (int)m_centers.size(); vertex++) {
        Vector<2, float> normal;
        if (!m_closed && vertex == 0) {
            Vector<2, float> direction = m_centers[0] - m_centers[1];
            normal = Vector<2, float>(-direction[1], direction[0]);
            float inverseLength = 1.0 / std::sqrt(normal[0] * normal[0] + normal[1] * normal[1]);
            normal = normal * inverseLength;
        } else if (!m_closed && vertex == (int)m_centers.size() - 1) {
            Vector<2, float> direction = m_centers[vertex - 1] - m_centers[vertex];
            normal = Vector<2, float>(-direction[1], direction[0]);
            float inverseLength = 1.0 / std::sqrt(normal[0] * normal[0] + normal[1] * normal[1]);
            normal = normal * inverseLength;
        } else {
            Vector<2, float> incoming =
                m_centers[(m_centers.size() + vertex - 1) % m_centers.size()] - m_centers[vertex];
            Vector<2, float> outgoing = m_centers[vertex] - m_centers[(vertex + 1) % m_centers.size()];
            Vector<2, float> firstNormal(-incoming[1], incoming[0]);
            Vector<2, float> secondNormal(-outgoing[1], outgoing[0]);
            float inverseLength = 1.0 / std::sqrt(firstNormal[0] * firstNormal[0] + firstNormal[1] * firstNormal[1]);
            firstNormal = firstNormal * inverseLength;
            inverseLength = 1.0 / std::sqrt(secondNormal[0] * secondNormal[0] + secondNormal[1] * secondNormal[1]);
            secondNormal = secondNormal * inverseLength;
            normal = firstNormal + secondNormal;
            inverseLength = 1.0 / std::sqrt(normal[0] * normal[0] + normal[1] * normal[1]);
            normal = normal * inverseLength;
            if (vertex % steps == 0 && (m_vertexFlags[vertex / steps] & VERTEX_BEZIER)) {
                float projection = firstNormal[0] * normal[0];
                projection += firstNormal[1] * normal[1];
                normal = normal / projection;
            }
        }
        normal = normal * (m_widths[vertex] * 0.5);
        m_outlinePositions[2 * vertex] = normal + m_centers[vertex];
        m_outlinePositions[2 * vertex + 1] = m_centers[vertex] - normal;
    }
    if (!m_outlinePositions.empty()) {
        m_minimum = m_outlinePositions[0];
        m_maximum = m_minimum;
        for (int vertex = 1; vertex < (int)m_outlinePositions.size(); vertex++) {
            if (m_minimum[0] > m_outlinePositions[vertex][0])
                m_minimum[0] = m_outlinePositions[vertex][0];
            if (m_minimum[1] > m_outlinePositions[vertex][1])
                m_minimum[1] = m_outlinePositions[vertex][1];
            if (m_maximum[0] < m_outlinePositions[vertex][0])
                m_maximum[0] = m_outlinePositions[vertex][0];
            if (m_maximum[1] < m_outlinePositions[vertex][1])
                m_maximum[1] = m_outlinePositions[vertex][1];
        }
        for (int vertex = 0; vertex < (int)m_startCap.size(); vertex++) {
            if (m_minimum[0] > m_startCap[vertex][0])
                m_minimum[0] = m_startCap[vertex][0];
            if (m_minimum[1] > m_startCap[vertex][1])
                m_minimum[1] = m_startCap[vertex][1];
            if (m_maximum[0] < m_startCap[vertex][0])
                m_maximum[0] = m_startCap[vertex][0];
            if (m_maximum[1] < m_startCap[vertex][1])
                m_maximum[1] = m_startCap[vertex][1];
        }
        for (int vertex = 0; vertex < (int)m_endCap.size(); vertex++) {
            if (m_minimum[0] > m_endCap[vertex][0])
                m_minimum[0] = m_endCap[vertex][0];
            if (m_minimum[1] > m_endCap[vertex][1])
                m_minimum[1] = m_endCap[vertex][1];
            if (m_maximum[0] < m_endCap[vertex][0])
                m_maximum[0] = m_endCap[vertex][0];
            if (m_maximum[1] < m_endCap[vertex][1])
                m_maximum[1] = m_endCap[vertex][1];
        }
    } else {
        m_minimum = Vector<2, float>(0.0f, 0.0f);
        m_maximum = Vector<2, float>(0.0f, 0.0f);
    }
    m_changed = true;
}

void CurveShape::storePositions()
{
    m_vertexPositions.copyFromCurrent(Attribute::STORED_ELEMENT);
}

void CurveShape::stretchPositions(const Vector<2, float> &origin, const Vector<2, float> &size,
                                  const Vector<2, float> &target, const Vector<2, float> &targetSize,
                                  const Matrix<float> &from, const Matrix<float> &to)
{
    for (int vertex = 0; vertex < (int)m_vertexPositions.m_values[0]->size(); vertex++) {
        Vector<2, float> &position = m_vertexPositions.value()[vertex];
        Vector<2, float> point = from.transformPoint(position);
        Vector<2, float> fraction;
        if (point[0] < origin[0])
            fraction[0] = 0.0f;
        else if (point[0] < origin[0] + size[0])
            fraction[0] = (point[0] - origin[0]) / size[0];
        else
            fraction[0] = 1.0f;
        if (point[1] < origin[1])
            fraction[1] = 0.0f;
        else if (point[1] < origin[1] + size[1])
            fraction[1] = (point[1] - origin[1]) / size[1];
        else
            fraction[1] = 1.0f;
        point = point - origin + target + fraction * (targetSize - size);
        position = to.transformPoint(point);
    }
    attributesChanged(ATTRIBUTES_CHANGED_ALL);
}

Shape *CurveShape::clone()
{
    CurveShape *copy = m_engine->createCurveShape(m_name);
    copy->m_vertexPositions = m_vertexPositions;
    copy->m_vertexColors = m_vertexColors;
    copy->m_vertexTexCoords = m_vertexTexCoords;
    copy->m_vertexWidths = m_vertexWidths;
    copy->m_unknown43C = m_unknown43C;
    copy->m_textureWeights = m_textureWeights;
    copy->m_textureTranslation = m_textureTranslation;
    copy->m_textureRotation = m_textureRotation;
    copy->m_texturePivot = m_texturePivot;
    copy->m_textureDeformation = m_textureDeformation;
    copy->setVertexFlags(m_vertexFlags);
    copy->m_subdivisions = m_subdivisions;
    copy->m_closed = m_closed;
    copy->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return copy;
}

CurveShape *CurveShape::setSubdivisions(int subdivisions)
{
    m_subdivisions = subdivisions;
    if (subdivisions < 0)
        m_subdivisions = 0;
    if (m_subdivisions > MaxCurveSubdivisions)
        m_subdivisions = MaxCurveSubdivisions;
    return this;
}

CurveShape *CurveShape::setStrokeSegments(int segments)
{
    m_strokeSegments = segments;

    // The code tests and clamps subdivisions, despite storing strokeSegments.
    if (m_subdivisions < 0)
        m_subdivisions = 0;
    if (m_subdivisions > LegacyStrokeSetterSubdivisionLimit)
        m_subdivisions = LegacyStrokeSetterSubdivisionLimit;
    return this;
}

CurveShape *CurveShape::setClosed(bool closed)
{
    m_closed = closed;
    return this;
}

void CurveShape::updateDrawing()
{
    m_drawing->m_texture = 0;
    m_drawing->m_pathVertices.clear();
    m_drawing->m_pathTextureCoordinates.clear();
    m_drawing->m_pathColors.clear();
    m_drawing->m_pathIndices.clear();
    for (int vertex = 0; vertex < (int)m_colors.size(); vertex++) {
        m_drawing->m_pathTextureCoordinates.push_back(Vector<2, float>(0.0f, 0.0f));
        m_drawing->m_pathColors.push_back(m_colors[vertex]);
        m_drawing->m_pathVertices.push_back(m_outlinePositions[2 * vertex]);
        m_drawing->m_pathTextureCoordinates.push_back(Vector<2, float>(0.0f, 0.0f));
        m_drawing->m_pathColors.push_back(m_colors[vertex]);
        m_drawing->m_pathVertices.push_back(m_outlinePositions[2 * vertex + 1]);
    }
    for (int vertex = 0; vertex < (int)m_drawing->m_pathVertices.size() - 2; vertex++) {
        m_drawing->m_pathIndices.push_back(vertex);
        m_drawing->m_pathIndices.push_back(vertex + 1);
        m_drawing->m_pathIndices.push_back(vertex + 2);
    }
    if (m_closed && !m_outlinePositions.empty()) {
        m_drawing->m_pathIndices.push_back(m_drawing->m_pathVertices.size() - 2);
        m_drawing->m_pathIndices.push_back(m_drawing->m_pathVertices.size() - 1);
        m_drawing->m_pathIndices.push_back(0);
        m_drawing->m_pathIndices.push_back(m_drawing->m_pathVertices.size() - 1);
        m_drawing->m_pathIndices.push_back(0);
        m_drawing->m_pathIndices.push_back(1);
    }
    if (!m_colors.empty()) {
        // The code gates both caps on the first cap's presence.
        if (!m_startCap.empty()) {
            int start = m_drawing->m_pathVertices.size();
            for (int vertex = 0; vertex < (int)m_startCap.size(); vertex++) {
                m_drawing->m_pathTextureCoordinates.push_back(Vector<2, float>(0.0f, 0.0f));
                m_drawing->m_pathColors.push_back(m_colors.front());
                m_drawing->m_pathVertices.push_back(m_startCap[vertex]);
            }
            for (int vertex = 1; vertex < (int)m_startCap.size() - 1; vertex++) {
                m_drawing->m_pathIndices.push_back(start);
                m_drawing->m_pathIndices.push_back(start + vertex);
                m_drawing->m_pathIndices.push_back(start + vertex + 1);
            }
            start = m_drawing->m_pathVertices.size();
            for (int vertex = 0; vertex < (int)m_endCap.size(); vertex++) {
                m_drawing->m_pathTextureCoordinates.push_back(Vector<2, float>(0.0f, 0.0f));
                m_drawing->m_pathColors.push_back(m_colors.back());
                m_drawing->m_pathVertices.push_back(m_endCap[vertex]);
            }
            for (int vertex = 1; vertex < (int)m_endCap.size() - 1; vertex++) {
                m_drawing->m_pathIndices.push_back(start);
                m_drawing->m_pathIndices.push_back(start + vertex);
                m_drawing->m_pathIndices.push_back(start + vertex + 1);
            }
        }
    }
    m_drawing->update();
}

void refineCurvePositions(std::vector<Vector<2, float>> &values, unsigned offset, int stride, int flagStride, int count,
                          bool closed, std::vector<int> flags)
{
    // The code applies the modulus before adding offset on destination indices; invalid offsets and empty arrays
    // remain unchecked.
    unsigned vertex = offset;
    for (int index = 0; index < count; index++) {
        Vector<2, float> sum = values[vertex % values.size()] + values[(vertex + stride) % values.size()];
        Vector<2, float> average = sum;
        average.m_elements[0] = average.m_elements[0] * 0.5;
        average.m_elements[1] = 0.5 * average.m_elements[1];
        values[offset + (stride / 2 - offset + vertex) % values.size()] = average;
        vertex += stride;
    }
    if (closed)
        ++count;
    unsigned step = stride;
    for (int index = 1; index < count; index++) {
        if (index % flagStride || !(flags[index / flagStride % flags.size()] & VERTEX_BEZIER)) {
            static float weight = ((float)std::sqrt(2.0) - 1.0) / ((float)std::sqrt(2.0) * 0.5);
            Vector<2, float> sum = values[(offset + step - stride / 2) % values.size()] +
                                   values[(stride / 2 + offset + step) % values.size()];
            Vector<2, float> average = sum;
            average.m_elements[0] = average.m_elements[0] * 0.5;
            average.m_elements[1] = 0.5 * average.m_elements[1];
            Vector<2, float> delta = average - values[(step + offset) % values.size()];
            Vector<2, float> adjustment = delta * weight;
            Vector<2, float> &destination = values[offset + step % values.size()];
            destination.m_elements[0] += adjustment.m_elements[0];
            destination.m_elements[1] += adjustment.m_elements[1];
        }
        step += stride;
    }
}

void refineCurveColors(std::vector<Vector<4, float>> &values, int stride, int flagStride, int count, bool closed,
                       unsigned offset, std::vector<int> flags)
{
    // The code applies the modulus before adding offset on destination indices; invalid offsets and empty arrays
    // remain unchecked.
    unsigned vertex = offset;
    for (int index = 0; index < count; index++) {
        Vector<4, float> sum = values[(vertex + stride) % values.size()] + values[vertex % values.size()];
        Vector<4, float> average = sum;
        for (int axis = 0; axis < 4; axis++)
            average.m_elements[axis] = average.m_elements[axis] * 0.5;
        values[offset + (stride / 2 - offset + vertex) % values.size()] = average;
        vertex += stride;
    }
    if (closed)
        ++count;
    unsigned step = stride;
    for (int index = 1; index < count; index++) {
        if (index % flagStride || !(flags[index / flagStride % flags.size()] & VERTEX_BEZIER)) {
            static float weight = ((float)std::sqrt(2.0) - 1.0) / ((float)std::sqrt(2.0) * 0.5);
            Vector<4, float> sum = values[(offset + step + stride / 2) % values.size()] +
                                   values[(offset + step - stride / 2) % values.size()];
            Vector<4, float> average = sum;
            for (int axis = 0; axis < 4; axis++)
                average.m_elements[axis] = average.m_elements[axis] * 0.5;
            Vector<4, float> delta = average - values[(step + offset) % values.size()];
            Vector<4, float> adjustment = delta * weight;
            Vector<4, float> &destination = values[offset + step % values.size()];
            for (int axis = 0; axis < 4; axis++)
                destination.m_elements[axis] += adjustment.m_elements[axis];
        }
        step += stride;
    }
}

void refineCurveWidths(unsigned offset, std::vector<float> &values, int stride, int flagStride, int count, bool closed,
                       std::vector<int> flags)
{
    // The code applies the modulus before adding offset on destination indices; invalid offsets and empty arrays
    // remain unchecked.
    unsigned vertex = offset;
    for (int index = 0; index < count; index++) {
        float midpoint = (values[(vertex + stride) % values.size()] + values[vertex % values.size()]) * 0.5;
        values[offset + (vertex + stride / 2 - offset) % values.size()] = midpoint;
        vertex += stride;
    }
    if (closed)
        ++count;
    unsigned step = stride;
    for (int index = 1; index < count; index++) {
        if (index % flagStride || !(flags[index / flagStride % flags.size()] & VERTEX_BEZIER)) {
            static float weight = ((float)std::sqrt(2.0) - 1.0) / ((float)std::sqrt(2.0) * 0.5);
            values[offset + step % values.size()] = ((values[(offset + step - stride / 2) % values.size()] +
                                                      values[(offset + step + stride / 2) % values.size()]) *
                                                         0.5 -
                                                     values[(offset + step) % values.size()]) *
                                                        weight +
                                                    values[offset + step % values.size()];
        }
        step += stride;
    }
}

void CurveShape::restorePositions()
{
    m_vertexPositions.copyToCurrent(Attribute::STORED_ELEMENT);
}

void CurveShape::draw(Node *) {}

}
