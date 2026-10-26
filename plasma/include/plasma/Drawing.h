// plasma::Drawing: the geometry of a shape prepared for drawing: paths are tessellated into triangles kept as
// vertex, texture coordinate, colour and index arrays, with the texture and its adjustments. GLDrawing (the only
// derived class) keeps the arrays in GL buffer objects and draws them. Element types follow tessellation, contour
// extraction, extrusion and GL buffer uploads.
#pragma once
#include <vector>
#include "plasma/Vector.h"
#include "plasma/Matrix.h"

class PDFWriter;
class Workspace;
class ProceduralShapeModule;
class MeshInterface;

namespace plasma {

class Texture;
class Node;

// Drawing::m_dirty (and SmoothMeshShape::m_dirty, copied into its drawings): which GL buffers GLDrawing::Update
// uploads again.
enum DrawingDirtyFlags {
    DRAWING_DIRTY_VERTICES = 0,
    DRAWING_DIRTY_TEXCOORDS = 1,
    DRAWING_DIRTY_COLORS = 2,
    DRAWING_DIRTY_INDICES = 3
};

enum DrawingDirtyMasks {
    DRAWING_DIRTY_NONE = 0,
    DRAWING_DIRTY_ALL = (1 << DRAWING_DIRTY_VERTICES) | (1 << DRAWING_DIRTY_TEXCOORDS) | (1 << DRAWING_DIRTY_COLORS) |
                        (1 << DRAWING_DIRTY_INDICES)
};

class Drawing {
public:
    friend class GenericShape;
    friend class StaticMeshShape;
    friend class SmoothMeshShape;
    friend class MeshShape;
    friend class CurveShape;
    friend class ::PDFWriter;             // Masks need three path vertices.
    friend class ::Workspace;             // The web export writes the path vertices, colours and indices.
    friend class ::MeshInterface;         // Box and colour tests over the path vertices.
    friend class ::ProceduralShapeModule; // The procedural shape's boundary radius.

    // GLU tessellation callbacks.
    friend void __stdcall appendDrawingIndex(void *vertex, void *polygon);
    friend void __stdcall combineDrawingVertex(double *coordinates, void **vertices, float *weights, void **result,
                                               void *polygon);

    Drawing();
    virtual ~Drawing();

    void tessellate();
    void extrude(const Matrix<float> &matrix);
    void buildContours();
    void buildBuffers();
    void reserveInput();
    void clear();
    bool contains(const Vector<2, float> &point) const;
    bool intersects(const Vector<2, float> &point, float radius, const Matrix<float> &drawingMatrix,
                    const Matrix<float> &pointMatrix) const;

    // An overload with the argument order used by support callers.
    bool intersects(const Matrix<float> &pointMatrix, const Vector<2, float> &point, const Matrix<float> &drawingMatrix,
                    float radius) const
    {
        return intersects(point, radius, drawingMatrix, pointMatrix);
    }
    void getTransformedBounds(const Matrix<float> &matrix, Vector<2, float> &minimum, Vector<2, float> &maximum,
                              bool &first) const;

    // Recomputes the bounds of the paths and reserves the input arrays (GLDrawing's override rebuilds and uploads the
    // buffers m_dirty selects).
    virtual void update();
    virtual void draw(Node *node, const Vector<4, float> &color) = 0;

protected:
    // Input arrays; tessellation writes triangles to m_pathIndices.
    std::vector<Vector<2, float>> m_pathVertices;
    std::vector<Vector<2, float>> m_pathTextureCoordinates;
    std::vector<Vector<4, float>> m_pathColors;
    std::vector<Vector<4, float>> m_pathBackColors;
    std::vector<unsigned int> m_pathIndices;
    std::vector<std::vector<int>> m_pathContours;
    // Constructed and destroyed, but otherwise unused.
    std::vector<std::vector<int>> m_unused64;
    std::vector<std::vector<int>> m_unused74;
    bool m_contourFill;            // Set on a fill drawing of a filled MESH_CONTOUR shape; never read.
    Texture *m_texture;            // 0 without a texture.
    Matrix<float> m_textureMatrix; // Loaded as GL_TEXTURE's matrix.
    float m_textureOpacity;        // The shape shader's texture uniforms.
    float m_textureBrightness;
    float m_textureContrast;
    float m_textureSaturation;
    unsigned m_dirty; // DrawingDirtyFlags; all at first.
    int m_unusedE0;   // -1 from the constructor; no other access.
    std::vector<Vector<2, float>> m_vertices;
    std::vector<Vector<2, float>> m_textureCoordinates;
    std::vector<Vector<4, float>> m_colors;
    std::vector<Vector<4, float>> m_normals; // Two edge normals per vertex (antialiasing).
    std::vector<unsigned int> m_indices;     // Triangles.
    std::vector<std::vector<int>> m_contours;
    int m_unused144;            // Zeroed by the constructor; no other access.
    Vector<2, float> m_minimum; // Not initialised by the constructor.
    Vector<2, float> m_maximum; // Not initialised by the constructor.
    bool m_unused158;           // Cleared by the constructor; no other access.
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif

}
