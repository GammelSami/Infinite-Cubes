// MeshShape builds polygonal shapes with animated attributes.
#include "plasma/MeshShape.h"
#include "plasma/Engine.h"
#include "plasma/Texture.h"

#include <algorithm>
#include <cmath>
#include <list>

bool isPointInTriangle(const plasma::Vector<2, float> *triangle, const plasma::Vector<2, float> &point);

namespace plasma {

MeshShape::MeshShape(Engine *engine, const std::wstring &name)
    : Shape(engine, name),
      m_vertexPositions("positions", std::vector<Vector<2, float>>()),
      m_vertexTexCoords("texCoords", std::vector<Vector<2, float>>()),
      m_vertexColors("colors", std::vector<Vector<4, float>>()),
      m_strokeColors("strokeColors", std::vector<Vector<4, float>>()),
      m_extrusionBackColors("extrusionBackColors", std::vector<Vector<4, float>>()),
      m_extrusionFrontColors("extrusionFrontColors", std::vector<Vector<4, float>>()),
      m_strokeWidths("strokeWidths", std::vector<float>()),
      m_textureTranslation("textureTranslation", Vector<2, float>(0.0f, 0.0f)),
      m_texturePivot("texturePivot", Vector<2, float>(0.0f, 0.0f)),
      m_textureRotation("textureRotation", Vector<3, float>(0.0f, 0.0f, 0.0f)),
      m_textureDeformation("textureDeformation", Matrix<float>()),
      m_textureOpacity("textureOpacity", 1.0f),
      m_textureBrightness("textureBrightness", 1.0f),
      m_textureContrast("textureContrast", 1.0f),
      m_textureSaturation("textureSaturation", 1.0f),
      m_strokeTextureOpacity("strokeTextureOpacity", 1.0f),
      m_strokeTextureBrightness("strokeTextureBrightness", 1.0f),
      m_strokeTextureContrast("strokeTextureContrast", 1.0f),
      m_strokeTextureSaturation("strokeTextureSaturation", 1.0f),
      m_strokeTextureStretch("strokeTextureStretch", 1.0f),
      m_extrusionMatrix("extrusionMatrix", Matrix<float>()),
      m_texture("texture", NO_OBJECT_ID),
      m_strokeTexture("strokeTexture", NO_OBJECT_ID),
      m_flags(MESH_FLAGS_NONE),
      m_strokeJointType(STROKE_JOINT_SHARP),
      m_strokeCapType(STROKE_CAP_NONE),
      m_strokeAlignment(STROKE_ALIGNMENT_NORMAL),
      m_fillTexture(0),
      m_outlineTexture(0)
{
    m_type = SHAPE_MESH;
    addAttribute(&m_vertexPositions);
    addAttribute(&m_vertexTexCoords);
    addAttribute(&m_vertexColors);
    addAttribute(&m_strokeColors);
    addAttribute(&m_extrusionFrontColors);
    addAttribute(&m_extrusionBackColors);
    addAttribute(&m_strokeWidths);
    addAttribute(&m_textureTranslation);
    addAttribute(&m_texturePivot);
    addAttribute(&m_textureRotation);
    addAttribute(&m_textureDeformation);
    addAttribute(&m_textureOpacity);
    addAttribute(&m_textureBrightness);
    addAttribute(&m_textureContrast);
    addAttribute(&m_textureSaturation);
    addAttribute(&m_strokeTextureOpacity);
    addAttribute(&m_strokeTextureBrightness);
    addAttribute(&m_strokeTextureContrast);
    addAttribute(&m_strokeTextureSaturation);
    addAttribute(&m_strokeTextureStretch);
    addAttribute(&m_texture);
    addAttribute(&m_strokeTexture);
    addAttribute(&m_extrusionMatrix);
    // The code matrix attributes initially copy indeterminate elements.
    m_textureDeformation.value().identity();
    m_textureMatrix.identity();
    m_extrusionMatrix.value().identity();
}

MeshShape::~MeshShape() {}

void MeshShape::fitBounds(const Vector<2, float> &minimum, const Vector<2, float> &maximum)
{
    Matrix<float> identity;
    Vector<2, float> lower;
    Vector<2, float> upper;
    bool first = true;
    identity.identity();
    if (!getVertexBounds(lower, upper, identity, first))
        return;
    // An empty mesh leaves lower and upper indeterminate in the code code.
    Vector<2, float> scale = (maximum - minimum) / (upper - lower);
    if (std::fabs(scale.m_elements[0]) < 1.0e-18f) {
        scale.m_elements[0] = 1.0f;
        lower.m_elements[0] = minimum.m_elements[0];
    }
    if (std::fabs(scale.m_elements[1]) < 1.0e-18f) {
        scale.m_elements[1] = 1.0f;
        lower.m_elements[1] = minimum.m_elements[1];
    }
    for (int vertex = 0; vertex < (int)m_vertexPositions.m_values[0]->size(); vertex++) {
        Vector<2, float> &point = m_vertexPositions.value()[vertex];
        Vector<2, float> offset = point - lower;
        Vector<2, float> stretched = offset;
        stretched.m_elements[0] *= scale.m_elements[0];
        stretched.m_elements[1] *= scale.m_elements[1];
        point = stretched + minimum;
    }
}

Vector<2, float> MeshShape::getMaximum()
{
    return m_maximum;
}

Vector<2, float> MeshShape::getMinimum()
{
    return m_minimum;
}

Shape *MeshShape::clone()
{
    MeshShape *copy = m_engine->createMeshShape(m_name);
    copy->m_vertexPositions = m_vertexPositions;
    copy->m_vertexColors = m_vertexColors;
    copy->m_strokeColors = m_strokeColors;
    copy->m_strokeWidths = m_strokeWidths;
    copy->m_vertexTexCoords = m_vertexTexCoords;
    copy->m_texture = m_texture;
    copy->m_strokeTexture = m_strokeTexture;
    copy->m_textureTranslation = m_textureTranslation;
    copy->m_textureRotation = m_textureRotation;
    copy->m_texturePivot = m_texturePivot;
    copy->m_textureDeformation = m_textureDeformation;
    copy->m_vertexFlags = m_vertexFlags;
    copy->m_vertexParameters = m_vertexParameters;
    copy->m_flags = m_flags;
    copy->m_strokeCapType = m_strokeCapType;
    copy->m_strokeJointType = m_strokeJointType;
    copy->m_strokeAlignment = m_strokeAlignment;
    copy->setFaces(m_faces);
    copy->updateBoundsAndTextures();
    return copy;
}

bool MeshShape::getVertexBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix,
                                bool &first)
{
    for (int vertex = 0; vertex < (int)m_vertexPositions.m_values[0]->size(); vertex++) {
        const Vector<2, float> &source = m_vertexPositions.value()[vertex];
        Vector<2, float> position = matrix.transform(source);
        if (first) {
            minimum = position;
            maximum = position;
            first = false;
        } else {
            minimum.m_elements[0] = (std::min)(position.m_elements[0], minimum.m_elements[0]);
            minimum.m_elements[1] = (std::min)(position.m_elements[1], minimum.m_elements[1]);
            maximum.m_elements[0] = (std::max)(position.m_elements[0], maximum.m_elements[0]);
            maximum.m_elements[1] = (std::max)(position.m_elements[1], maximum.m_elements[1]);
        }
    }
    return true;
}

bool MeshShape::contains(const Vector<2, float> &point)
{
    if (point.m_elements[0] < m_minimum.m_elements[0])
        return false;
    if (point.m_elements[1] < m_minimum.m_elements[1])
        return false;
    if (point.m_elements[0] > m_maximum.m_elements[0])
        return false;
    if (point.m_elements[1] > m_maximum.m_elements[1])
        return false;
    for (int face = 0; face < (int)m_faces.size(); face++) {
        const std::vector<int> &vertices = m_faces[face];
        if (vertices.empty())
            continue;
        const std::vector<Vector<2, float>> &positions = m_vertexPositions.value();
        Vector<2, float> points[3];
        points[0] = positions[vertices[0]];
        for (int triangle = 0; triangle < (int)vertices.size() - 2; triangle++) {
            points[1] = positions[vertices[triangle + 1]];
            points[2] = positions[vertices[triangle + 2]];
            if (::isPointInTriangle(points, point))
                return true;
        }
    }
    return false;
}

void MeshShape::restorePositions()
{
    m_vertexPositions.copyToCurrent(Attribute::STORED_ELEMENT);
}

void MeshShape::setFaces(const std::vector<std::vector<int>> &faces)
{
    m_vertexColors.resize((int)m_vertexPositions.m_values[0]->size());
    m_vertexTexCoords.resize((int)m_vertexPositions.m_values[0]->size());
    m_strokeColors.resize((int)m_vertexPositions.m_values[0]->size());
    m_strokeWidths.resize((int)m_vertexPositions.m_values[0]->size());
    m_vertexFlags.resize(m_vertexPositions.m_values[0]->size());
    m_vertexParameters.resize(m_vertexPositions.m_values[0]->size());
    m_faces = faces;
    m_boundaryEdges.clear();
    for (int face = 0; face < (int)m_faces.size(); face++) {
        const std::vector<int> &vertices = m_faces[face];
        if (vertices.size() < 2)
            continue;
        if (vertices.size() == 2) {
            m_boundaryEdges.insert(std::make_pair(vertices[0], vertices[1]));
            m_boundaryEdges.insert(std::make_pair(vertices[1], vertices[0]));
        } else {
            for (int vertex = 0; vertex < (int)vertices.size(); vertex++) {
                int a = vertices[vertex];
                int b = vertices[(vertex + 1) % vertices.size()];
                std::set<std::pair<int, int>>::iterator reversed = m_boundaryEdges.find(std::make_pair(b, a));
                if (reversed == m_boundaryEdges.end())
                    m_boundaryEdges.insert(std::make_pair(a, b));
                else
                    m_boundaryEdges.erase(reversed);
            }
        }
    }
    // The boundary edges leaving each vertex.
    std::map<int, std::list<int>> outgoing;
    for (int face = 0; face < (int)m_faces.size(); face++) {
        const std::vector<int> &vertices = m_faces[face];
        if (vertices.size() < 2)
            continue;
        for (int vertex = 0; vertex < (int)vertices.size(); vertex++) {
            int a = vertices[vertex];
            int b = vertices[(vertex + 1) % vertices.size()];
            if (m_boundaryEdges.find(std::make_pair(a, b)) != m_boundaryEdges.end())
                outgoing[a].push_back(b);
        }
    }
    // Walk the boundary edges into closed loops, removing each edge used.
    m_contours.clear();
    while (!outgoing.empty()) {
        m_contours.push_back(std::vector<int>());
        int vertex;
        if (m_faces.size() == 1 && !m_faces[0].empty() && outgoing.find(m_faces[0][0]) != outgoing.end())
            vertex = m_faces[0][0];
        else
            vertex = outgoing.begin()->first;
        int start = vertex;
        do {
            if (outgoing.find(vertex) == outgoing.end())
                break;
            std::list<int> &links = outgoing[vertex];
            int destination = links.front();
            links.pop_front();
            if (links.empty())
                outgoing.erase(vertex);
            m_contours.back().push_back(vertex);
            vertex = destination;
        } while (vertex != start);
    }
}

void MeshShape::applyVertexParameters()
{
    for (int contour = 0; contour < (int)m_contours.size(); contour++) {
        int count = (int)m_contours[contour].size();
        for (int index = 0; index < count; index++) {
            int vertex = m_contours[contour][index];
            if ((m_vertexFlags[vertex] & VERTEX_CONSTRAINT_MASK) != (1 << VERTEX_ROUNDED_CORNER))
                continue;
            int next = m_contours[contour][(index + 3) % count];
            if ((m_vertexFlags[next] & VERTEX_CONSTRAINT_MASK) != (1 << VERTEX_ROUNDED_CORNER))
                next = m_contours[contour][(index + 2) % count];
            Vector<2, float> direction = m_vertexPositions.value()[next] - m_vertexPositions.value()[vertex];
            float lengthSquared =
                direction.m_elements[0] * direction.m_elements[0] + direction.m_elements[1] * direction.m_elements[1];
            if (lengthSquared > 0.0f) {
                float scale = m_vertexParameters[vertex] / sqrtf(lengthSquared);
                m_vertexPositions.value()[m_contours[contour][(index + 1) % count]] =
                    m_vertexPositions.value()[vertex] + direction * scale;
            }
            int previous = m_contours[contour][(index + count - 3) % count];
            if ((m_vertexFlags[previous] & VERTEX_CONSTRAINT_MASK) != (1 << VERTEX_ROUNDED_CORNER))
                previous = m_contours[contour][(index + count - 2) % count];
            direction = m_vertexPositions.value()[previous] - m_vertexPositions.value()[vertex];
            lengthSquared =
                direction.m_elements[0] * direction.m_elements[0] + direction.m_elements[1] * direction.m_elements[1];
            if (lengthSquared > 0.0f) {
                float scale = m_vertexParameters[vertex] / sqrtf(lengthSquared);
                m_vertexPositions.value()[m_contours[contour][(index + count - 1) % count]] =
                    m_vertexPositions.value()[vertex] + direction * scale;
            }
        }
        for (int index = 0; index < count; index++) {
            int vertex = m_contours[contour][index];
            if ((m_vertexFlags[vertex] & VERTEX_CONSTRAINT_MASK) != (1 << VERTEX_FIXED_ON_EDGE))
                continue;
            int next = 0;
            for (int scan = index + 1; scan < index + count; scan++) {
                next = m_contours[contour][scan % count];
                if ((m_vertexFlags[next] & VERTEX_CONSTRAINT_MASK) != (1 << VERTEX_FIXED_ON_EDGE))
                    break;
            }
            int previous = 0;
            for (int scan = index + count - 1; scan >= index + 1; scan--) {
                previous = m_contours[contour][scan % count];
                if ((m_vertexFlags[previous] & VERTEX_CONSTRAINT_MASK) != (1 << VERTEX_FIXED_ON_EDGE))
                    break;
            }
            float fraction = m_vertexParameters[vertex];
            m_vertexPositions.value()[vertex] =
                m_vertexPositions.value()[next] * (1.0f - fraction) + m_vertexPositions.value()[previous] * fraction;
        }
    }
}

void MeshShape::storePositions()
{
    m_vertexPositions.copyFromCurrent(Attribute::STORED_ELEMENT);
}

void MeshShape::updateBoundsAndTextures()
{
    updateTextureMatrix();
    if (m_texture.value() == NO_OBJECT_ID)
        m_fillTexture = 0;
    else if (!m_fillTexture || m_fillTexture->m_id != m_texture.value())
        m_fillTexture = m_engine->findTexture(m_texture.value());
    if (m_strokeTexture.value() == NO_OBJECT_ID)
        m_outlineTexture = 0;
    else if (!m_outlineTexture || m_outlineTexture->m_id != m_strokeTexture.value())
        m_outlineTexture = m_engine->findTexture(m_strokeTexture.value());
    if (!m_vertexPositions.m_values[0]->size()) {
        m_minimum = Vector<2, float>(0.0f, 0.0f);
        m_maximum = Vector<2, float>(0.0f, 0.0f);
    } else {
        m_minimum = m_vertexPositions.value()[0];
        m_maximum = m_minimum;
        for (int vertex = 1; vertex < (int)m_vertexPositions.m_values[0]->size(); vertex++) {
            const Vector<2, float> &point = m_vertexPositions.value()[vertex];
            if (point.m_elements[0] < m_minimum.m_elements[0])
                m_minimum.m_elements[0] = point.m_elements[0];
            if (point.m_elements[1] < m_minimum.m_elements[1])
                m_minimum.m_elements[1] = point.m_elements[1];
            if (point.m_elements[0] > m_maximum.m_elements[0])
                m_maximum.m_elements[0] = point.m_elements[0];
            if (point.m_elements[1] > m_maximum.m_elements[1])
                m_maximum.m_elements[1] = point.m_elements[1];
        }
    }
    m_changed = true;
}

void MeshShape::updateTextureMatrix()
{
    Matrix<float> &matrix = m_textureMatrix;
    matrix.identity();
    matrix.translate(m_textureTranslation.value());
    matrix.translate(m_texturePivot.value());
    matrix.rotateX(m_textureRotation.value().m_elements[0]);
    matrix.rotateY(m_textureRotation.value().m_elements[1]);
    matrix.rotateZ(m_textureRotation.value().m_elements[2]);
    matrix *= m_textureDeformation.value();
    Vector<2, float> pivot = m_texturePivot.value() * -1.0;
    for (int row = 0; row < 4; row++)
        matrix.m_elements[12 + row] = matrix.m_elements[row + 4] * pivot.m_elements[1] +
                                      pivot.m_elements[0] * matrix.m_elements[row] + matrix.m_elements[12 + row];
}

void MeshShape::stretchPositions(const Vector<2, float> &origin, const Vector<2, float> &size,
                                 const Vector<2, float> &target, const Vector<2, float> &targetSize,
                                 const Matrix<float> &from, const Matrix<float> &to)
{
    for (int element = 0; element < m_vertexPositions.size(); element++) {
        for (int vertex = 0; vertex < (int)m_vertexPositions.m_values[0]->size(); vertex++) {
            Vector<2, float> &position = (*m_vertexPositions.m_values[element])[vertex];
            Vector<2, float> point = from.transform(position);
            Vector<2, float> fraction;
            for (int axis = 0; axis < 2; axis++) {
                float extent = size.m_elements[axis] + origin.m_elements[axis];
                fraction.m_elements[axis] =
                    origin.m_elements[axis] > point.m_elements[axis]
                        ? 0.0f
                        : (extent > point.m_elements[axis]
                               ? (point.m_elements[axis] - origin.m_elements[axis]) / size.m_elements[axis]
                               : 1.0f);
            }
            position = to.transform(point - origin + target + (targetSize - size) * fraction);
        }
    }
    updateBoundsAndTextures();
}

void MeshShape::attributesChanged(int) {}

// MeshShape itself has no drawing.
void MeshShape::draw(Node *) {}

}
