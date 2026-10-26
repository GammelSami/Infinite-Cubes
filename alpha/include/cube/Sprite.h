#pragma once
#include <list>
#include <map>
#include <string>
#include <vector>
#include "plasma/Vector.h"

struct IDirect3DDevice9;
struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;

namespace cube {
class Database;

// Marker channels selected by pure red, green and blue voxels in setVoxels.
enum SpriteMarker {
    SpriteMarkerRed = 0,
    SpriteMarkerGreen = 1,
    SpriteMarkerBlue = 2
};
// Number of entries, not an ID.
enum {
    SpriteMarkerCount = 3
};

// Airship red-marker positions hold the crew relative to the moving model.
enum SpritePositionList {
    SpritePositionListAirshipCrew = 0
};

// Sprite is a voxel model with graphics buffers and material state.
class Sprite {
public:
    Sprite(IDirect3DDevice9 *device, bool);
    virtual ~Sprite();
    void draw();
    bool isEmpty(const plasma::Vector<3, unsigned char> *color, bool showMarkers);
    void load(const std::string &path, Database *database, bool showMarkers);
    void resize(int x, int y, int z);
    void setVoxels(int x, int y, int z, const plasma::Vector<3, unsigned char> *colors, bool showMarkers);
    void updateMesh();
    float vertexExposure(int x, int y, int z, const plasma::Vector<3, int> &normal);

    plasma::Vector<3, unsigned char> &voxel(int x, int y, int z)
    {
        if (x < 0 || y < 0 || z < 0 || x >= m_size[0] || y >= m_size[1] || z >= m_size[2])
            return s_emptyVoxel;
        return m_voxels[(z * m_size[1] + y) * m_size[0] + x];
    }
    int sizeX() { return m_size[0]; }
    int sizeY() { return m_size[1]; }
    int sizeZ() { return m_size[2]; }

    static int s_frame;
    static plasma::Vector<3, unsigned char> s_emptyVoxel;

    std::vector<plasma::Vector<3, int>> m_markers[SpriteMarkerCount];
    std::map<int, std::list<plasma::Vector<3, int>>> m_positionLists;
    plasma::Vector<3, unsigned char> *m_voxels; // owned x*y*z colors
    IDirect3DVertexBuffer9 *m_vertexBuffer;     // owned COM reference
    IDirect3DIndexBuffer9 *m_indexBuffer;       // owned COM reference
    int m_vertexCount;
    int m_triangleCount;
    plasma::Vector<3, int> m_size;
    int m_drawnFrame;
    bool m_meshDirty;
    bool m_showMarkers;
    bool m_deferMesh;
    IDirect3DDevice9 *m_device; // borrowed
    plasma::Vector<3, unsigned char> m_tint;
};
}
