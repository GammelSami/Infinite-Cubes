// Drawing tessellates shape paths into vertex, texture-coordinate, colour and index arrays, with texture adjustments.
// D3D11Drawing uploads and renders these arrays.
#pragma once
#include <vector>
#include "plasma/Vector.h"
#include "plasma/Matrix.h"

namespace plasma {

class Texture;
class Node;

enum DrawingDirtyFlags {
    DRAWING_DIRTY_VERTICES = 0,
    DRAWING_DIRTY_TEXCOORDS = 1,
    DRAWING_DIRTY_COLORS = 2,
    DRAWING_DIRTY_INDICES = 3
};
enum DrawingDirtyMasks {
    DRAWING_DIRTY_ALL = (1 << DRAWING_DIRTY_VERTICES) | (1 << DRAWING_DIRTY_TEXCOORDS) | (1 << DRAWING_DIRTY_COLORS) |
                        (1 << DRAWING_DIRTY_INDICES)
};

// The GLU tessellation callbacks registered by tessellate.
void __stdcall appendDrawingIndex(void *vertex, void *polygon);
void __stdcall combineDrawingVertex(double *coordinates, void **vertices, float *weights, void **result, void *polygon);

class Drawing {
    friend class GenericShape;
    friend class CurveShape;
    friend class StaticMeshShape;
    friend class SmoothMeshShape;
    friend void __stdcall appendDrawingIndex(void *vertex, void *polygon);
    friend void __stdcall combineDrawingVertex(double *coordinates, void **vertices, float *weights, void **result,
                                               void *polygon);

public:
    Drawing();
    virtual ~Drawing();

    // Recomputes the bounds of the paths (m_minimum/m_maximum) and reserves the input arrays. D3D11Drawing's override
    // also rebuilds its buffers.
    virtual void update();
    virtual void draw(Node *node, const Vector<4, float> &color) {}

    void tessellate();
    void extrude(const Matrix<float> &matrix);
    void buildContours();
    void reserveInput();
    // This operation has an empty implementation.
    void reset();
    void clear();
    bool contains(const Vector<2, float> &point) const;
    bool intersects(const Vector<2, float> &point, float radius, const Matrix<float> &drawingMatrix,
                    const Matrix<float> &pointMatrix) const;
    void getTransformedBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix,
                              bool &first) const;

protected:
    // Input arrays; tessellation writes triangles to m_pathIndices. D3D11Drawing::update interleaves m_pathVertices,
    // m_pathTextureCoordinates and m_pathColors into its vertex buffer and uploads m_pathIndices.
    std::vector<Vector<2, float>> m_pathVertices;
    std::vector<Vector<2, float>> m_pathTextureCoordinates;
    std::vector<Vector<4, float>> m_pathColors;
    std::vector<Vector<4, float>> m_pathBackColors; // (Extrude's transformed side)
    std::vector<unsigned int> m_pathIndices;
    std::vector<std::vector<int>> m_pathContours; // (Tessellate's input contours)
    std::vector<std::vector<int>> m_unused98;
    std::vector<std::vector<int>> m_unusedB0;
    bool m_contourFill; // cleared by the constructor (plasma: set on a filled contour shape's fill drawing; never read)
    Texture *m_texture;
    Matrix<float> m_textureMatrix; // passed to the engine by D3D11Drawing::draw
    float m_textureOpacity;        // the four go to the engine's shape constants
    float m_textureBrightness;
    float m_textureContrast;
    float m_textureSaturation;
    unsigned m_dirty; // DrawingDirtyFlags; all at first
    int m_unused12C;  // Initially -1; no other access found.
    std::vector<Vector<2, float>> m_vertices;
    std::vector<Vector<2, float>> m_textureCoordinates;
    std::vector<Vector<4, float>> m_colors;
    std::vector<Vector<4, float>> m_normals;
    std::vector<int> m_indices;
    std::vector<std::vector<int>> m_contours;
    void *m_unused1C0;
    Vector<2, float> m_minimum; // not initialised by the constructor; set by Update
    Vector<2, float> m_maximum;
    bool m_unused1D8; // cleared by the constructor; no other access found
};

}
