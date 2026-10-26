// Drawing copies path arrays into render buffers and computes their bounds. Unused members are documented individually.
#pragma once
#include <vector>
#include "plasma/Vector.h"
#include "plasma/Matrix.h"

namespace plasma {

class Texture;
class Node;
// Individual flags are bit indices; use (1 << flag) to form a mask.
enum DrawingDirtyFlags {
    DRAWING_DIRTY_VERTICES = 0,
    DRAWING_DIRTY_TEXCOORDS = 1,
    DRAWING_DIRTY_COLORS = 2,
    DRAWING_DIRTY_INDICES = 3
};

// Whole-field masks; the flags above are bit indices.
enum DrawingDirtyMasks {
    DRAWING_DIRTY_NONE = 0,
    DRAWING_DIRTY_ALL = (1 << DRAWING_DIRTY_VERTICES) | (1 << DRAWING_DIRTY_TEXCOORDS) | (1 << DRAWING_DIRTY_COLORS) |
                        (1 << DRAWING_DIRTY_INDICES)
};

class Drawing {
public:
    friend class GenericShape;
    friend class SmoothMeshShape;
    friend class MeshShape;
    friend class CurveShape;
    friend void __stdcall appendDrawingIndex(void *vertex, void *polygon);
    friend void __stdcall combineDrawingVertex(double *coordinates, void **vertices, float *weights, void **result,
                                               void *polygon);

    Drawing();
    virtual ~Drawing();

    const Vector<2, float> &getMinimum() const { return m_minimum; }
    const Vector<2, float> &getMaximum() const { return m_maximum; }
    void tessellate();
    void extrude(const Matrix<float> &matrix);
    void buildContours();
    void buildBuffers();
    void reserveInput();
    // Empty drawing hook.
    void reset();
    void clear();
    bool contains(const Vector<2, float> &point) const;
    bool intersects(const Vector<2, float> &point, float radius, const Matrix<float> &drawingMatrix,
                    const Matrix<float> &pointMatrix) const;
    bool intersects(const Matrix<float> &pointMatrix, const Vector<2, float> &point, const Matrix<float> &drawingMatrix,
                    float radius) const
    {
        return intersects(point, radius, drawingMatrix, pointMatrix);
    }
    void getTransformedBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix,
                              bool &first) const;
    virtual void update();
    virtual void draw(Node *node, const Vector<4, float> &color) = 0;

protected:
    std::vector<Vector<2, float>> m_pathVertices;
    std::vector<Vector<2, float>> m_pathTextureCoordinates;
    std::vector<Vector<4, float>> m_pathColors;
    std::vector<Vector<4, float>> m_pathBackColors;
    std::vector<int> m_pathIndices;
    std::vector<std::vector<int>> m_pathContours;
    std::vector<std::vector<int>>
        m_unused4c; // only the constructor and destructor touch it (the 2012 engine's m_unused64)
    std::vector<std::vector<int>> m_unused58; // likewise (m_unused74)
    bool m_contourFill;
    Texture *m_texture;
    Matrix<float> m_textureMatrix;
    float m_textureOpacity;
    float m_textureBrightness;
    float m_textureContrast;
    float m_textureSaturation;
    unsigned m_dirty;
    int m_unusedc0; // -1 from the constructor; nothing reads it (the 2012 engine's m_unusedE0)
    std::vector<Vector<2, float>> m_vertices;
    std::vector<Vector<2, float>> m_textureCoordinates;
    std::vector<Vector<4, float>> m_colors;
    std::vector<Vector<4, float>> m_normals;
    std::vector<int> m_indices;
    std::vector<std::vector<int>> m_contours;
    int m_unused10c; // zeroed by the constructor; nothing reads it (m_unused144)
    Vector<2, float> m_minimum;
    Vector<2, float> m_maximum;
    bool m_unused120;
};

}
