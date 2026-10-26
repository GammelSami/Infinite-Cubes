#include <algorithm>
// See cube/Sprite.h.
#include <fstream>
#include <math.h>
#include "cube/BytesIO.h"
#include "cube/Database.h"
#include "cube/Noise.h"
#include "cube/Sprite.h"
#include "gfx/Graphics.h"

namespace cube {

// Returns the larger or smaller of two values, taken by value.
static inline float maxValue(float a, float b)
{
    return a > b ? a : b;
}

static inline float minValue(float a, float b)
{
    return a < b ? a : b;
}

Sprite::Sprite(gfx::Graphics &graphics, bool fullIntensity)
    : m_graphics(&graphics),
      m_voxels(nullptr),
      m_vertexBuffer(nullptr),
      m_indexBuffer(nullptr),
      m_size(0, 0, 0),
      m_drawnFrame(0),
      m_meshDirty(false),
      m_showMarkers(true),
      m_fullIntensity(fullIntensity),
      m_shadingRadius(3),
      m_tint(0, 0, 0)
{}

Sprite::Sprite(const Sprite &other)
    : m_graphics(other.m_graphics),
      m_voxels(nullptr),
      m_vertexBuffer(nullptr),
      m_indexBuffer(nullptr),
      m_size(0, 0, 0),
      m_drawnFrame(0),
      m_meshDirty(false),
      m_showMarkers(other.m_showMarkers),
      m_fullIntensity(other.m_fullIntensity),
      m_shadingRadius(other.m_shadingRadius),
      m_tint(other.m_tint)
{
    const plasma::Vector<3, unsigned char> *voxels = other.m_voxels;
    setVoxels(other.m_size[0], other.m_size[1], other.m_size[2], voxels, true, false);
    updateMesh(0);
}

Sprite::~Sprite()
{
    delete m_vertexBuffer;
    delete m_indexBuffer;
    if (m_voxels)
        delete[] m_voxels;
}

bool Sprite::bind()
{
    if (!m_vertexBuffer || !m_indexBuffer)
        return false;
    return m_vertexBuffer->bind() && m_indexBuffer->bind();
}

void Sprite::draw()
{
    if (m_vertexBuffer && m_indexBuffer && m_vertexBuffer->bind() && m_indexBuffer->bind()) {
        m_drawnFrame = s_frame;
        m_graphics->drawIndexed(m_vertexCount, m_triangleCount);
    }
}

void Sprite::drawBound()
{
    if (m_vertexBuffer && m_indexBuffer)
        m_graphics->drawIndexed(m_vertexCount, m_triangleCount);
}

void Sprite::finalize()
{
    if (m_indexBuffer)
        m_indexBuffer->prepare();
    if (m_vertexBuffer)
        m_vertexBuffer->prepare();
}

bool Sprite::isEmpty(const plasma::Vector<3, unsigned char> *voxel, bool showMarkers)
{
    return *voxel == s_emptyVoxel || (!showMarkers && (*voxel == plasma::Vector<3, unsigned char>(255, 0, 0) ||
                                                       *voxel == plasma::Vector<3, unsigned char>(0, 255, 0) ||
                                                       *voxel == plasma::Vector<3, unsigned char>(0, 0, 255)));
}

void Sprite::load(const std::string &path, Database *database, bool showMarkers, bool deferMesh)
{
    m_showMarkers = showMarkers;
    if (database) {
        BytesIO data;
        if (!database->read(path, data))
            return;
        descramble(data);
        // A blob shorter than its header leaves the sizes uninitialised.
        int sizeX;
        int sizeY;
        int sizeZ;
        data.read(&sizeX, 4);
        data.read(&sizeY, 4);
        data.read(&sizeZ, 4);
        std::vector<plasma::Vector<3, unsigned char>> voxels(sizeX * sizeY * sizeZ);
        data.read(&voxels[0], (int)voxels.size() * 3);
        setVoxels(sizeX, sizeY, sizeZ, &voxels[0], showMarkers, deferMesh);
    } else {
        std::ifstream file(path, std::ios::binary);
        if (!file.good())
            return;
        int sizeX;
        int sizeY;
        int sizeZ;
        file.read((char *)&sizeX, 4);
        file.read((char *)&sizeY, 4);
        file.read((char *)&sizeZ, 4);
        std::vector<plasma::Vector<3, unsigned char>> voxels(sizeX * sizeY * sizeZ);
        file.read((char *)&voxels[0], voxels.size() * 3);
        setVoxels(sizeX, sizeY, sizeZ, &voxels[0], showMarkers, deferMesh);
    }
    finalize();
}

float Sprite::rayCast(const plasma::Vector<3, float> &origin, const plasma::Vector<3, float> &direction,
                      float maxDistance, bool stopOutside, plasma::Vector<3, int> &voxel, plasma::Vector<3, int> &face)
{
    // Does the ray's line cross one of the six faces of the box?
    bool hit = false;
    if (fabs(direction[0]) > 0.0f) {
        plasma::Vector<3, float> front = origin + direction * -(origin[0] / direction[0]);
        if (front[1] >= 0.0f && front[2] >= 0.0f && m_size[1] > front[1] && m_size[2] > front[2])
            hit = true;
        plasma::Vector<3, float> back = origin + direction * ((m_size[0] - origin[0]) / direction[0]);
        if (back[1] >= 0.0f && back[2] >= 0.0f && m_size[1] > back[1] && m_size[2] > back[2])
            hit = true;
    }
    if (fabs(direction[1]) > 0.0f) {
        plasma::Vector<3, float> front = origin + direction * -(origin[1] / direction[1]);
        if (front[0] >= 0.0f && front[2] >= 0.0f && m_size[0] > front[0] && m_size[2] > front[2])
            hit = true;
        plasma::Vector<3, float> back = origin + direction * ((m_size[1] - origin[1]) / direction[1]);
        if (back[0] >= 0.0f && back[2] >= 0.0f && m_size[0] > back[0] && m_size[2] > back[2])
            hit = true;
    }
    if (fabs(direction[2]) > 0.0f) {
        plasma::Vector<3, float> front = origin + direction * -(origin[2] / direction[2]);
        if (front[1] >= 0.0f && front[0] >= 0.0f && m_size[1] > front[1] && m_size[0] > front[0])
            hit = true;
        plasma::Vector<3, float> back = origin + direction * ((m_size[2] - origin[2]) / direction[2]);
        if (back[1] >= 0.0f && back[0] >= 0.0f && m_size[1] > back[1] && m_size[0] > back[0])
            hit = true;
    }
    if (!hit)
        return maxDistance;

    float distance = 0.0f;
    voxel[0] = (int)origin[0];
    voxel[1] = (int)origin[1];
    voxel[2] = (int)origin[2];
    if (origin[0] < 0.0f)
        voxel[0]--;
    if (origin[1] < 0.0f)
        voxel[1]--;
    if (origin[2] < 0.0f)
        voxel[2]--;
    while (distance < maxDistance) {
        if (this->voxel(voxel[0], voxel[1], voxel[2]) != s_emptyVoxel)
            break;
        plasma::Vector<3, float> position = origin + direction * distance;
        if (stopOutside) {
            bool outside = false;
            plasma::Vector<3, int> size = m_size;
            for (int i = 0; i < 3; i++) {
                if (direction[i] < 0.0f && voxel[i] < 0) {
                    voxel[i] = -1;
                    face[i] = 1;
                    outside = true;
                } else if (direction[i] > 0.0f && voxel[i] >= size[i]) {
                    voxel[i] = size[i];
                    face[i] = -1;
                    outside = true;
                }
            }
            if (outside)
                return distance;
        }
        // The nearest voxel boundary along the ray.
        float step = 10.0f;
        int axis = 0;
        for (int i = 0; i < 3; i++) {
            if (direction[i] != 0.0f) {
                float d;
                if (direction[i] > 0.0f)
                    d = ((float)(voxel[i] + 1) - position[i]) / direction[i];
                else
                    d = ((float)voxel[i] - position[i]) / direction[i];
                if (step > d) {
                    axis = i;
                    step = d;
                }
            }
        }
        face = plasma::Vector<3, int>(0, 0, 0);
        if (direction[axis] > 0.0f) {
            voxel[axis]++;
            face[axis] = -1;
        } else {
            voxel[axis]--;
            face[axis] = 1;
        }
        distance += step;
    }
    return minValue(maxDistance, distance);
}

void Sprite::resize(int sizeX, int sizeY, int sizeZ)
{
    m_size[0] = sizeX;
    m_size[1] = sizeY;
    m_size[2] = sizeZ;
    if (m_voxels) {
        delete[] m_voxels;
        m_voxels = nullptr;
    }
    if (m_size[0] > 0 && m_size[1] > 0 && m_size[2] > 0) {
        m_voxels = new plasma::Vector<3, unsigned char>[m_size[0] * m_size[1] * sizeZ];
        memset(m_voxels, 0, m_size[0] * m_size[1] * m_size[2] * sizeof(plasma::Vector<3, unsigned char>));
    }
}

void Sprite::setVoxels(int sizeX, int sizeY, int sizeZ, const plasma::Vector<3, unsigned char> *voxels,
                       bool showMarkers, bool deferMesh)
{
    m_size[0] = sizeX;
    m_size[1] = sizeY;
    m_size[2] = sizeZ;
    if (m_voxels) {
        delete[] m_voxels;
        m_voxels = nullptr;
    }
    if (m_size[0] > 0 && m_size[1] > 0 && m_size[2] > 0) {
        m_voxels = new plasma::Vector<3, unsigned char>[m_size[0] * m_size[1] * sizeZ];
        memcpy(m_voxels, voxels, m_size[0] * m_size[1] * m_size[2] * sizeof(plasma::Vector<3, unsigned char>));
    }
    m_markers[SpriteMarkerRed].clear();
    m_markers[SpriteMarkerGreen].clear();
    m_markers[SpriteMarkerBlue].clear();
    if (!showMarkers) {
        for (int x = 0; x < m_size[0]; x++) {
            for (int y = 0; y < m_size[1]; y++) {
                for (int z = 0; z < m_size[2]; z++) {
                    plasma::Vector<3, unsigned char> color = voxel(x, y, z);
                    if (color == plasma::Vector<3, unsigned char>(255, 0, 0))
                        m_markers[SpriteMarkerRed].push_back(plasma::Vector<3, int>(x, y, z));
                    if (color == plasma::Vector<3, unsigned char>(0, 255, 0))
                        m_markers[SpriteMarkerGreen].push_back(plasma::Vector<3, int>(x, y, z));
                    if (color == plasma::Vector<3, unsigned char>(0, 0, 255))
                        m_markers[SpriteMarkerBlue].push_back(plasma::Vector<3, int>(x, y, z));
                }
            }
        }
    }
    if (!deferMesh)
        updateMesh(0);
}

plasma::Vector<4, float> Sprite::shadeVertex(int x, int y, int z, const plasma::Vector<3, float> &color,
                                             const plasma::Vector<3, int> &normal)
{
    float exposure = vertexExposure(x, y, z, normal);
    plasma::Vector<3, float> tinted =
        color + plasma::Vector<3, float>(m_tint[0] / 255.0f, m_tint[1] / 255.0f, m_tint[2] / 255.0f);
    float light = powf(exposure, 1.2f);
    tinted = tinted * light;
    float luminance = tinted[0] * 0.3f + tinted[1] * 0.59f + tinted[2] * 0.11f;
    if (luminance > 1.0f) {
        tinted = tinted * (1.0f / luminance);
        luminance = 1.0f;
    }
    // The darker the corner, the more saturated: the colour is pushed away from its grey by 3 - 2 * light.
    float saturation = 3.0f - light * 2.0f;
    plasma::Vector<3, float> shaded =
        plasma::Vector<3, float>(luminance, luminance, luminance) * (1.0f - saturation) + tinted * saturation;
    for (int i = 0; i < 3; i++) {
        if (shaded[i] < 0.0f)
            shaded[i] = 0.0f;
        else if (shaded[i] > 1.0f)
            shaded[i] = 1.0f;
    }
    plasma::Vector<4, float> result(shaded[0], shaded[1], shaded[2], 2.0f);
    if (!m_fullIntensity) {
        for (int i = 0; i < 4; i++)
            result[i] *= 0.5f;
    }
    return result;
}

void Sprite::updateMesh(int margin)
{
    std::vector<gfx::Vertex> vertices;
    int endX = m_size[0] - margin;
    int endY = m_size[1] - margin;
    for (int x = margin; x < endX; x++) {
        size_t firstX = margin;
        for (int y = margin; y < endY; y++) {
            for (int z = 0; z < m_size[2]; z++) {
                plasma::Vector<3, unsigned char> *voxel = &this->voxel(x, y, z);
                if (isEmpty(voxel, m_showMarkers))
                    continue;
                plasma::Vector<3, float> color = plasma::Vector<3, float>(*voxel) / 255.0f;
                // A quad for every side that is at the margin or borders an empty voxel (markers count as solid here).
                if (x == firstX || this->voxel(x - 1, y, z) == s_emptyVoxel) {
                    vertices.push_back(gfx::Vertex(plasma::Vector<3, int>(x, y, z), plasma::Vector<3, int>(-1, 0, 0),
                                                   shadeVertex(x, y, z, color, plasma::Vector<3, int>(-1, 0, 0))));
                    vertices.push_back(gfx::Vertex(plasma::Vector<3, int>(x, y, z + 1),
                                                   plasma::Vector<3, int>(-1, 0, 0),
                                                   shadeVertex(x, y, z + 1, color, plasma::Vector<3, int>(-1, 0, 0))));
                    vertices.push_back(
                        gfx::Vertex(plasma::Vector<3, int>(x, y + 1, z + 1), plasma::Vector<3, int>(-1, 0, 0),
                                    shadeVertex(x, y + 1, z + 1, color, plasma::Vector<3, int>(-1, 0, 0))));
                    vertices.push_back(gfx::Vertex(plasma::Vector<3, int>(x, y + 1, z),
                                                   plasma::Vector<3, int>(-1, 0, 0),
                                                   shadeVertex(x, y + 1, z, color, plasma::Vector<3, int>(-1, 0, 0))));
                }
                if (x == endX - 1 || this->voxel(x + 1, y, z) == s_emptyVoxel) {
                    vertices.push_back(gfx::Vertex(plasma::Vector<3, int>(x + 1, y, z), plasma::Vector<3, int>(1, 0, 0),
                                                   shadeVertex(x + 1, y, z, color, plasma::Vector<3, int>(1, 0, 0))));
                    vertices.push_back(
                        gfx::Vertex(plasma::Vector<3, int>(x + 1, y + 1, z), plasma::Vector<3, int>(1, 0, 0),
                                    shadeVertex(x + 1, y + 1, z, color, plasma::Vector<3, int>(1, 0, 0))));
                    vertices.push_back(
                        gfx::Vertex(plasma::Vector<3, int>(x + 1, y + 1, z + 1), plasma::Vector<3, int>(1, 0, 0),
                                    shadeVertex(x + 1, y + 1, z + 1, color, plasma::Vector<3, int>(1, 0, 0))));
                    vertices.push_back(
                        gfx::Vertex(plasma::Vector<3, int>(x + 1, y, z + 1), plasma::Vector<3, int>(1, 0, 0),
                                    shadeVertex(x + 1, y, z + 1, color, plasma::Vector<3, int>(1, 0, 0))));
                }
                if (y == margin || this->voxel(x, y - 1, z) == s_emptyVoxel) {
                    vertices.push_back(gfx::Vertex(plasma::Vector<3, int>(x, y, z), plasma::Vector<3, int>(0, -1, 0),
                                                   shadeVertex(x, y, z, color, plasma::Vector<3, int>(0, -1, 0))));
                    vertices.push_back(gfx::Vertex(plasma::Vector<3, int>(x + 1, y, z),
                                                   plasma::Vector<3, int>(0, -1, 0),
                                                   shadeVertex(x + 1, y, z, color, plasma::Vector<3, int>(0, -1, 0))));
                    vertices.push_back(
                        gfx::Vertex(plasma::Vector<3, int>(x + 1, y, z + 1), plasma::Vector<3, int>(0, -1, 0),
                                    shadeVertex(x + 1, y, z + 1, color, plasma::Vector<3, int>(0, -1, 0))));
                    vertices.push_back(gfx::Vertex(plasma::Vector<3, int>(x, y, z + 1),
                                                   plasma::Vector<3, int>(0, -1, 0),
                                                   shadeVertex(x, y, z + 1, color, plasma::Vector<3, int>(0, -1, 0))));
                }
                if (y == endY - 1 || this->voxel(x, y + 1, z) == s_emptyVoxel) {
                    vertices.push_back(gfx::Vertex(plasma::Vector<3, int>(x, y + 1, z), plasma::Vector<3, int>(0, 1, 0),
                                                   shadeVertex(x, y + 1, z, color, plasma::Vector<3, int>(0, 1, 0))));
                    vertices.push_back(
                        gfx::Vertex(plasma::Vector<3, int>(x, y + 1, z + 1), plasma::Vector<3, int>(0, 1, 0),
                                    shadeVertex(x, y + 1, z + 1, color, plasma::Vector<3, int>(0, 1, 0))));
                    vertices.push_back(
                        gfx::Vertex(plasma::Vector<3, int>(x + 1, y + 1, z + 1), plasma::Vector<3, int>(0, 1, 0),
                                    shadeVertex(x + 1, y + 1, z + 1, color, plasma::Vector<3, int>(0, 1, 0))));
                    vertices.push_back(
                        gfx::Vertex(plasma::Vector<3, int>(x + 1, y + 1, z), plasma::Vector<3, int>(0, 1, 0),
                                    shadeVertex(x + 1, y + 1, z, color, plasma::Vector<3, int>(0, 1, 0))));
                }
                if (this->voxel(x, y, z - 1) == s_emptyVoxel) {
                    vertices.push_back(gfx::Vertex(plasma::Vector<3, int>(x, y, z), plasma::Vector<3, int>(0, 0, -1),
                                                   shadeVertex(x, y, z, color, plasma::Vector<3, int>(0, 0, -1))));
                    vertices.push_back(gfx::Vertex(plasma::Vector<3, int>(x, y + 1, z),
                                                   plasma::Vector<3, int>(0, 0, -1),
                                                   shadeVertex(x, y + 1, z, color, plasma::Vector<3, int>(0, 0, -1))));
                    vertices.push_back(
                        gfx::Vertex(plasma::Vector<3, int>(x + 1, y + 1, z), plasma::Vector<3, int>(0, 0, -1),
                                    shadeVertex(x + 1, y + 1, z, color, plasma::Vector<3, int>(0, 0, -1))));
                    vertices.push_back(gfx::Vertex(plasma::Vector<3, int>(x + 1, y, z),
                                                   plasma::Vector<3, int>(0, 0, -1),
                                                   shadeVertex(x + 1, y, z, color, plasma::Vector<3, int>(0, 0, -1))));
                }
                if (this->voxel(x, y, z + 1) == s_emptyVoxel) {
                    vertices.push_back(gfx::Vertex(plasma::Vector<3, int>(x, y, z + 1), plasma::Vector<3, int>(0, 0, 1),
                                                   shadeVertex(x, y, z + 1, color, plasma::Vector<3, int>(0, 0, 1))));
                    vertices.push_back(
                        gfx::Vertex(plasma::Vector<3, int>(x + 1, y, z + 1), plasma::Vector<3, int>(0, 0, 1),
                                    shadeVertex(x + 1, y, z + 1, color, plasma::Vector<3, int>(0, 0, 1))));
                    vertices.push_back(
                        gfx::Vertex(plasma::Vector<3, int>(x + 1, y + 1, z + 1), plasma::Vector<3, int>(0, 0, 1),
                                    shadeVertex(x + 1, y + 1, z + 1, color, plasma::Vector<3, int>(0, 0, 1))));
                    vertices.push_back(
                        gfx::Vertex(plasma::Vector<3, int>(x, y + 1, z + 1), plasma::Vector<3, int>(0, 0, 1),
                                    shadeVertex(x, y + 1, z + 1, color, plasma::Vector<3, int>(0, 0, 1))));
                }
            }
        }
    }

    // Two triangles per quad: 0 1 2 and 0 2 3.
    if (vertices.size() < 0x10000) {
        std::vector<unsigned short> indices;
        indices.resize(vertices.size() / 4 * 6);
        for (int i = 0; i < (int)vertices.size() / 4; i++) {
            indices[i * 6] = i * 4;
            for (int j = 0; j < 2; j++)
                indices[i * 6 + 1 + j] = j + i * 4 + 1;
            indices[i * 6 + 3] = i * 4;
            for (int j = 0; j < 2; j++)
                indices[i * 6 + 4 + j] = j + i * 4 + 2;
        }
        m_vertexCount = (int)vertices.size();
        m_triangleCount = (int)(indices.size() / 3);
        if (!indices.empty() && !vertices.empty()) {
            gfx::VertexBuffer *vertexBuffer = m_graphics->createVertexBuffer(vertices);
            gfx::IndexBuffer *indexBuffer = m_graphics->createIndexBuffer16(indices);
            gfx::VertexBuffer *oldVertexBuffer = m_vertexBuffer;
            gfx::IndexBuffer *oldIndexBuffer = m_indexBuffer;
            m_indexBuffer = indexBuffer;
            m_vertexBuffer = vertexBuffer;
            delete oldVertexBuffer;
            delete oldIndexBuffer;
        }
    } else {
        std::vector<unsigned int> indices;
        indices.resize(vertices.size() / 4 * 6);
        for (int i = 0; i < (int)vertices.size() / 4; i++) {
            indices[i * 6] = i * 4;
            for (int j = 0; j < 2; j++)
                indices[i * 6 + 1 + j] = j + i * 4 + 1;
            indices[i * 6 + 3] = i * 4;
            for (int j = 0; j < 2; j++)
                indices[i * 6 + 4 + j] = j + i * 4 + 2;
        }
        m_vertexCount = (int)vertices.size();
        m_triangleCount = (int)(indices.size() / 3);
        if (!indices.empty() && !vertices.empty()) {
            gfx::VertexBuffer *vertexBuffer = m_graphics->createVertexBuffer(vertices);
            gfx::IndexBuffer *indexBuffer = m_graphics->createIndexBuffer32(indices);
            gfx::VertexBuffer *oldVertexBuffer = m_vertexBuffer;
            gfx::IndexBuffer *oldIndexBuffer = m_indexBuffer;
            m_indexBuffer = indexBuffer;
            m_vertexBuffer = vertexBuffer;
            delete oldVertexBuffer;
            delete oldIndexBuffer;
        }
    }
    m_meshDirty = false;
}

float Sprite::vertexExposure(int x, int y, int z, const plasma::Vector<3, int> &normal)
{
    // Along the normal only the layer in front of the face is sampled.
    int minX = -m_shadingRadius;
    int maxX = m_shadingRadius;
    int minY = -m_shadingRadius;
    int maxY = m_shadingRadius;
    int minZ = -m_shadingRadius;
    int maxZ = m_shadingRadius;
    if (normal[0] > 0) {
        minX = 0;
        maxX = 1;
    }
    if (normal[0] < 0) {
        minX = -1;
        maxX = 0;
    }
    if (normal[1] > 0) {
        minY = 0;
        maxY = 1;
    }
    if (normal[1] < 0) {
        minY = -1;
        maxY = 0;
    }
    if (normal[2] > 0) {
        minZ = 0;
        maxZ = 1;
    }
    if (normal[2] < 0) {
        minZ = -1;
        maxZ = 0;
    }
    float total = 0.0f;
    float open = 0.0f;
    for (int dx = minX; dx < maxX; dx++) {
        for (int dy = minY; dy < maxY; dy++) {
            for (int dz = minZ; dz < maxZ; dz++) {
                // The distance of the sample's centre from the corner, largest axis.
                float distance = maxValue(fabs(dz + 0.5f), maxValue(fabs(dy + 0.5f), fabs(dx + 0.5f)));
                float radius = (float)m_shadingRadius;
                float weight = (radius - distance) / radius;
                if (voxel(x + dx, y + dy, z + dz) == s_emptyVoxel)
                    open += weight;
                total += weight;
            }
        }
    }
    if (total > 0.0f)
        return maxValue(0.5f, open / total);
    return 0.0f;
}

}
