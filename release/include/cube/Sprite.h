// Sprite owns a voxel model's RGB colours (x fastest), marker positions and graphics vertex and index buffers. Pure
// red, green and blue voxels are markers and are omitted from the mesh unless m_showMarkers is set. updateMesh builds
// the buffers from the voxel data.
#pragma once
#include <list>
#include <map>
#include <string>
#include <vector>
#include "plasma/Vector.h"

namespace gfx {
class Graphics;
class IndexBuffer;
class VertexBuffer;
}

namespace cube {

// Marker voxels by colour: red (255, 0, 0), green (0, 255, 0) and blue (0, 0, 255).
enum SpriteMarker {
    SpriteMarkerRed = 0,
    SpriteMarkerGreen = 1,
    SpriteMarkerBlue = 2
};
// Number of entries, not an ID.
enum {
    SpriteMarkerCount = 3
};

class Database;

class Sprite {
public:
    // fullIntensity: stored in m_fullIntensity.
    Sprite(gfx::Graphics &graphics, bool fullIntensity);
    // Copies the settings and the voxels and builds a mesh of its own; the marker positions are not copied (setVoxels
    // is told to skip them).
    Sprite(const Sprite &other);
    virtual ~Sprite();

    // Binds the vertex and the index buffer; false when there is no mesh or a buffer refuses.
    bool bind();
    // Binds the buffers, notes the frame and draws the mesh.
    void draw();
    // Draws the mesh with whatever is bound (the renderer's shadow and outline passes bind once and draw several
    // times).
    void drawBound();
    void finalize();
    // Whether the voxel colour counts as empty: the empty colour, or a marker colour unless showMarkers.
    bool isEmpty(const plasma::Vector<3, unsigned char> *voxel, bool showMarkers);
    // Reads a .cub model (three ints x, y, z, then x * y * z colours): from the blob named path of database when there
    // is one (scrambled as descramble undoes), otherwise from the file. A missing blob or file leaves the sprite as it
    // is, except m_showMarkers. deferMesh: do not build the mesh (SpriteManager::loadSprites edits the voxels first).
    void load(const std::string &path, Database *database, bool showMarkers, bool deferMesh);
    // Walks the voxels along the ray from origin (voxel units, direction a unit vector) for at most maxDistance and
    // stops in the first voxel that is not the empty colour: voxel is its position and face the unit step back out of
    // it (the side that was entered). Returns the distance walked, at most maxDistance; maxDistance, with nothing
    // written, when the ray's line misses every face of the sprite's box. With stopOutside the walk also ends as soon
    // as it is outside the box and heading away from it, with voxel clamped to one cell outside.
    float rayCast(const plasma::Vector<3, float> &origin, const plasma::Vector<3, float> &direction, float maxDistance,
                  bool stopOutside, plasma::Vector<3, int> &voxel, plasma::Vector<3, int> &face);
    // Sets the size and replaces the voxels by empty (zero) ones; the mesh is not rebuilt.
    void resize(int sizeX, int sizeY, int sizeZ);
    // Sets the size and copies the voxels; unless showMarkers, collects the marker positions; unless deferMesh,
    // rebuilds the mesh.
    void setVoxels(int sizeX, int sizeY, int sizeZ, const plasma::Vector<3, unsigned char> *voxels, bool showMarkers,
                   bool deferMesh);
    // The colour of a face's vertex at a voxel corner: the voxel colour plus the tint, darkened by how enclosed the
    // corner is, with alpha 2.
    plasma::Vector<4, float> shadeVertex(int x, int y, int z, const plasma::Vector<3, float> &color,
                                         const plasma::Vector<3, int> &normal);
    void updateMesh(int margin);
    // The share (0.5 to 1; 0 when nothing is sampled) of empty voxels around a corner on the open side of normal,
    // weighted by closeness; m_shadingRadius voxels along the axes the normal does not point along.
    float vertexExposure(int x, int y, int z, const plasma::Vector<3, int> &normal);

    // Direct voxel accessors; edits require a later updateMesh call to rebuild geometry.
    void setFullIntensity(bool value) { m_fullIntensity = value; }
    void setTint(const plasma::Vector<3, unsigned char> &tint) { m_tint = tint; }
    // Sets one voxel's colour; nothing happens outside the model.
    void setVoxel(int x, int y, int z, const plasma::Vector<3, unsigned char> &color)
    {
        if (x < 0 || y < 0 || z < 0 || x >= m_size[0] || y >= m_size[1] || z >= m_size[2])
            return;
        m_voxels[(z * m_size[1] + y) * m_size[0] + x] = color;
    }
    // The voxel from a colour of twice the byte range: each channel is halved and clamped to 0..255.
    void setVoxel(int x, int y, int z, const plasma::Vector<3, float> &color)
    {
        plasma::Vector<3, float> c;
        c = color * 0.5f;
        for (int i = 0; i < 3; i++) {
            if (c[i] < 0.0f)
                c[i] = 0.0f;
            else if (c[i] > 255.0f)
                c[i] = 255.0f;
        }
        setVoxel(x, y, z,
                 plasma::Vector<3, unsigned char>((unsigned char)c[0], (unsigned char)c[1], (unsigned char)c[2]));
    }
    int sizeX() { return m_size[0]; }
    int sizeY() { return m_size[1]; }
    int sizeZ() { return m_size[2]; }
    // The colour at a position, or the empty colour outside the model.
    plasma::Vector<3, unsigned char> &voxel(int x, int y, int z)
    {
        if (x < 0 || y < 0 || z < 0 || x >= m_size[0] || y >= m_size[1] || z >= m_size[2])
            return s_emptyVoxel;
        return m_voxels[(z * m_size[1] + y) * m_size[0] + x];
    }

    // Never written.
    static plasma::Vector<3, unsigned char> s_emptyVoxel;
    // The frame being drawn; draw copies it to m_drawnFrame. Written by other files.
    static int s_frame;

public:
    // Attachment points.
    std::vector<plasma::Vector<3, int>> m_markers[SpriteMarkerCount]; // by SpriteMarker
    std::map<int, std::list<plasma::Vector<3, int>>> m_positionLists;
    gfx::Graphics *m_graphics;
    // m_size.x * y * z colours, x fastest, new[] by resize and setVoxels, freed by the destructor. A colour equal to
    // s_emptyVoxel is empty.
    plasma::Vector<3, unsigned char> *m_voxels;
    gfx::VertexBuffer *m_vertexBuffer; // built by updateMesh; owned
    gfx::IndexBuffer *m_indexBuffer;   // 16- or 32-bit indices (updateMesh chooses by vertex count); owned
    int m_vertexCount;                 // (updateMesh); passed to the draw call
    int m_triangleCount;               // index count / 3 (updateMesh)
    plasma::Vector<3, int> m_size;     // voxels per axis
    int m_drawnFrame;                  // s_frame at the last draw
    bool m_meshDirty;                  // false in the constructors; cleared at the end of updateMesh
    // Whether marker voxels are meshed (isEmpty treats them as empty when false); set by load.
    bool m_showMarkers;
    bool m_fullIntensity;
    int m_shadingRadius; // 3; the sampling radius of vertexExposure
    plasma::Vector<3, unsigned char> m_tint;
};

}
