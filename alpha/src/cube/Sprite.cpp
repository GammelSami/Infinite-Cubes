// Sprite loads and draws voxel models.
#include <d3d9.h>
#include <fstream>
#include "cube/BytesIO.h"
#include "cube/Database.h"
#include <string.h>
#include "cube/Sprite.h"
#ifndef CUBE_SERVER
#include "cube/Vertex.h"
#endif

namespace cube {

// Shared fallback values for empty voxel lookups.
int Sprite::s_frame;
plasma::Vector<3, unsigned char> Sprite::s_emptyVoxel;

Sprite::Sprite(IDirect3DDevice9 *device, bool)
    : m_device(device)
{
    m_voxels = nullptr;
    m_vertexBuffer = nullptr;
    m_indexBuffer = nullptr;
    m_size = plasma::Vector<3, int>(0, 0, 0);
    m_drawnFrame = 0;
    m_meshDirty = false;
    m_showMarkers = true;
    m_tint[0] = 0;
    m_tint[1] = 0;
    m_tint[2] = 0;
    m_deferMesh = false;
}

Sprite::~Sprite()
{
#ifndef CUBE_SERVER
    // Only the client owns Direct3D vertex/index buffer references.
    if (m_vertexBuffer)
        m_vertexBuffer->Release();
    if (m_indexBuffer)
        m_indexBuffer->Release();
#endif
    if (m_voxels)
        delete[] m_voxels;
}

void Sprite::draw()
{
    if (!m_vertexBuffer) {
        if (m_deferMesh)
            m_meshDirty = true;
        return;
    }
    if (m_indexBuffer) {
        m_drawnFrame = s_frame;
        m_device->SetStreamSource(0, m_vertexBuffer, 0, 8);
        m_device->SetIndices(m_indexBuffer);
        m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_vertexCount, 0, m_triangleCount);
    }
}

bool Sprite::isEmpty(const plasma::Vector<3, unsigned char> *color, bool showMarkers)
{
    return *color == s_emptyVoxel || (!showMarkers && (*color == plasma::Vector<3, unsigned char>(255, 0, 0) ||
                                                       *color == plasma::Vector<3, unsigned char>(0, 255, 0) ||
                                                       *color == plasma::Vector<3, unsigned char>(0, 0, 255)));
}

void Sprite::load(const std::string &path, Database *database, bool showMarkers)
{
    m_showMarkers = showMarkers;
    if (database) {
        BytesIO data;
        if (!database->read(path, data))
            return;
        data.descramble();
        // A blob shorter than its header leaves the sizes uninitialised.
        int sizeX;
        int sizeY;
        int sizeZ;
        data.read(&sizeX, 4);
        data.read(&sizeY, 4);
        data.read(&sizeZ, 4);
        std::vector<plasma::Vector<3, unsigned char>> voxels(sizeX * sizeY * sizeZ);
        data.read(&voxels[0], (int)voxels.size() * 3);
        setVoxels(sizeX, sizeY, sizeZ, &voxels[0], showMarkers);
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
        setVoxels(sizeX, sizeY, sizeZ, &voxels[0], showMarkers);
    }
}

void Sprite::resize(int x, int y, int z)
{
    m_size[0] = x;
    m_size[1] = y;
    m_size[2] = z;
    if (m_voxels) {
        delete[] m_voxels;
        m_voxels = nullptr;
    }
    if (m_size[0] > 0 && m_size[1] > 0 && m_size[2] > 0) {
        m_voxels = new plasma::Vector<3, unsigned char>[m_size[0] * m_size[1] * z];
        memset(m_voxels, 0, m_size[0] * m_size[1] * m_size[2] * sizeof(*m_voxels));
    }
}

void Sprite::setVoxels(int sizeX, int sizeY, int sizeZ, const plasma::Vector<3, unsigned char> *voxels,
                       bool showMarkers)
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
#ifndef CUBE_SERVER
    if (!m_deferMesh)
        updateMesh();
#endif
}

#ifndef CUBE_SERVER

void Sprite::updateMesh()
{
    std::vector<Vertex> vertices;
    std::vector<unsigned short> indices;
    for (int x = 0; x < m_size[0]; x++) {
        for (int y = 0; y < m_size[1]; y++) {
            for (int z = 0; z < m_size[2]; z++) {
                plasma::Vector<3, unsigned char> *colorVoxel = &voxel(x, y, z);
                if (isEmpty(colorVoxel, m_showMarkers))
                    continue;
                plasma::Vector<3, float> color = plasma::Vector<3, float>(*colorVoxel) / 255.0f;
                if (isEmpty(&voxel(x - 1, y, z), true)) {
                    {
                        float exposure = vertexExposure(x, y, z, plasma::Vector<3, int>(-1, 0, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x, y, z), plasma::Vector<3, int>(-1, 0, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x, y, z + 1, plasma::Vector<3, int>(-1, 0, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x, y, z + 1), plasma::Vector<3, int>(-1, 0, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x, y + 1, z + 1, plasma::Vector<3, int>(-1, 0, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x, y + 1, z + 1), plasma::Vector<3, int>(-1, 0, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x, y + 1, z, plasma::Vector<3, int>(-1, 0, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x, y + 1, z), plasma::Vector<3, int>(-1, 0, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                }
                if (isEmpty(&voxel(x + 1, y, z), true)) {
                    {
                        float exposure = vertexExposure(x + 1, y, z, plasma::Vector<3, int>(1, 0, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x + 1, y, z), plasma::Vector<3, int>(1, 0, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x + 1, y + 1, z, plasma::Vector<3, int>(1, 0, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x + 1, y + 1, z), plasma::Vector<3, int>(1, 0, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x + 1, y + 1, z + 1, plasma::Vector<3, int>(1, 0, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x + 1, y + 1, z + 1), plasma::Vector<3, int>(1, 0, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x + 1, y, z + 1, plasma::Vector<3, int>(1, 0, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x + 1, y, z + 1), plasma::Vector<3, int>(1, 0, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                }
                if (isEmpty(&voxel(x, y - 1, z), true)) {
                    {
                        float exposure = vertexExposure(x, y, z, plasma::Vector<3, int>(0, -1, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x, y, z), plasma::Vector<3, int>(0, -1, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x + 1, y, z, plasma::Vector<3, int>(0, -1, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x + 1, y, z), plasma::Vector<3, int>(0, -1, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x + 1, y, z + 1, plasma::Vector<3, int>(0, -1, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x + 1, y, z + 1), plasma::Vector<3, int>(0, -1, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x, y, z + 1, plasma::Vector<3, int>(0, -1, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x, y, z + 1), plasma::Vector<3, int>(0, -1, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                }
                if (isEmpty(&voxel(x, y + 1, z), true)) {
                    {
                        float exposure = vertexExposure(x, y + 1, z, plasma::Vector<3, int>(0, 1, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x, y + 1, z), plasma::Vector<3, int>(0, 1, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x, y + 1, z + 1, plasma::Vector<3, int>(0, 1, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x, y + 1, z + 1), plasma::Vector<3, int>(0, 1, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x + 1, y + 1, z + 1, plasma::Vector<3, int>(0, 1, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x + 1, y + 1, z + 1), plasma::Vector<3, int>(0, 1, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x + 1, y + 1, z, plasma::Vector<3, int>(0, 1, 0));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x + 1, y + 1, z), plasma::Vector<3, int>(0, 1, 0),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                }
                if (isEmpty(&voxel(x, y, z - 1), true)) {
                    {
                        float exposure = vertexExposure(x, y, z, plasma::Vector<3, int>(0, 0, -1));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x, y, z), plasma::Vector<3, int>(0, 0, -1),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x, y + 1, z, plasma::Vector<3, int>(0, 0, -1));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x, y + 1, z), plasma::Vector<3, int>(0, 0, -1),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x + 1, y + 1, z, plasma::Vector<3, int>(0, 0, -1));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x + 1, y + 1, z), plasma::Vector<3, int>(0, 0, -1),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x + 1, y, z, plasma::Vector<3, int>(0, 0, -1));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x + 1, y, z), plasma::Vector<3, int>(0, 0, -1),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                }
                if (isEmpty(&voxel(x, y, z + 1), true)) {
                    {
                        float exposure = vertexExposure(x, y, z + 1, plasma::Vector<3, int>(0, 0, 1));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x, y, z + 1), plasma::Vector<3, int>(0, 0, 1),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x + 1, y, z + 1, plasma::Vector<3, int>(0, 0, 1));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x + 1, y, z + 1), plasma::Vector<3, int>(0, 0, 1),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x + 1, y + 1, z + 1, plasma::Vector<3, int>(0, 0, 1));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x + 1, y + 1, z + 1), plasma::Vector<3, int>(0, 0, 1),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                    {
                        float exposure = vertexExposure(x, y + 1, z + 1, plasma::Vector<3, int>(0, 0, 1));
                        plasma::Vector<3, float> tint = plasma::Vector<3, float>(m_tint) / 255.0f;
                        vertices.push_back(
                            Vertex(plasma::Vector<3, int>(x, y + 1, z + 1), plasma::Vector<3, int>(0, 0, 1),
                                   plasma::Vector<4, float>(color[0], color[1], color[2], 1.0f) * exposure +
                                       plasma::Vector<4, float>(tint[0], tint[1], tint[2], 1.0f) * (1.0f - exposure)));
                    }
                }
            }
        }
    }
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
        IDirect3DVertexBuffer9 *vertexBuffer;
        IDirect3DIndexBuffer9 *indexBuffer;
        m_device->CreateVertexBuffer((UINT)(m_vertexCount * sizeof(Vertex)), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED,
                                     &vertexBuffer, 0);
        m_device->CreateIndexBuffer((UINT)(indices.size() * sizeof(unsigned short)), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
                                    D3DPOOL_MANAGED, &indexBuffer, 0);
        // Assumes both allocations and locks succeed.
        void *data;
        vertexBuffer->Lock(0, (UINT)(vertices.size() * sizeof(Vertex)), &data, 0);
        memcpy(data, &vertices[0], vertices.size() * sizeof(Vertex));
        vertexBuffer->Unlock();
        indexBuffer->Lock(0, (UINT)(indices.size() * sizeof(unsigned short)), &data, 0);
        memcpy(data, &indices[0], indices.size() * sizeof(unsigned short));
        indexBuffer->Unlock();
        IDirect3DVertexBuffer9 *oldVertexBuffer = m_vertexBuffer;
        IDirect3DIndexBuffer9 *oldIndexBuffer = m_indexBuffer;
        m_indexBuffer = indexBuffer;
        m_vertexBuffer = vertexBuffer;
        if (oldVertexBuffer)
            oldVertexBuffer->Release();
        if (oldIndexBuffer)
            oldIndexBuffer->Release();
    }
    m_meshDirty = false;
}
#endif

// The share of open voxels around a vertex: those within 3 voxels, on the normal's open side only
// along each axis it points along.
float Sprite::vertexExposure(int x, int y, int z, const plasma::Vector<3, int> &normal)
{
    int minX = -3, maxX = 3;
    int minY = -3, maxY = 3;
    int minZ = -3, maxZ = 3;
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
    int total = 0;
    int open = 0;
    for (int dx = minX; dx < maxX; dx++) {
        for (int dy = minY; dy < maxY; dy++) {
            for (int dz = minZ; dz < maxZ; dz++) {
                if (voxel(x + dx, y + dy, z + dz) == s_emptyVoxel)
                    open++;
                total++;
            }
        }
    }
    if (total > 0)
        return (float)open / (float)total;
    return 0.0f;
}

}
