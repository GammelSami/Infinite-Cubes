// MeshShape holds animated polygon vertices, texture coordinates, fill and stroke colours and widths, faces and
// texture parameters. It draws nothing directly; SmoothMeshShape renders its subdivided mesh.
#pragma once
#include <set>
#include <utility>
#include <vector>
#include "plasma/Shape.h"
#include "plasma/ContinuousArrayAttribute.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/DiscreteAttribute.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

namespace plasma {

class Engine;
class Node;
class Texture;

// MeshShape::m_flags.
enum MeshFlags {
    MESH_CONTOUR = 0, // filled along the smooth outline, not subdivided
    MESH_STROKE = 1,  // stroked
    MESH_NO_FILL = 2, // stroke only: no fill drawing
    MESH_OPEN = 3,    // the contour's last segment is left out, with caps
    MESH_EXTRUDED = 4 // extruded
};

// MeshShape::m_vertexFlags stores a corner kind in the low half and constraints in the high half. VERTEX_BEZIER and
// VERTEX_ROUND share bit 1: CurveShape tests that bit for off-curve or round vertices, while SmoothMeshShape compares
// the complete kind value.
enum VertexKind {
    VERTEX_SMOOTH = 0,
    VERTEX_SHARP = 1,
    VERTEX_BEZIER = 2, // an off-curve control point
    VERTEX_ROUND = 3
};
// The constraint: bit numbers in the high half.
enum VertexConstraintFlag {
    VERTEX_ROUNDED_CORNER = 16,
    VERTEX_FIXED_ON_EDGE = 17
};
enum VertexMasks {
    VERTEX_FREE = 0,
    VERTEX_KIND_MASK = 0xffff,
    VERTEX_CONSTRAINT_MASK = 0xffff0000
};

// m_strokeJointType, m_strokeCapType, m_strokeAlignment (values from plasma).
enum StrokeJointType {
    STROKE_JOINT_SHARP = 0,
    STROKE_JOINT_ROUND = 1
};
enum StrokeCapType {
    STROKE_CAP_NONE = 0,
    STROKE_CAP_ROUND = 1,
    STROKE_CAP_SQUARE = 2
};
enum StrokeAlignment {
    STROKE_ALIGNMENT_INNER = -1, // a factor on the stroke offset
    STROKE_ALIGNMENT_NORMAL = 0,
    STROKE_ALIGNMENT_OUTER = 1
};

class MeshShape : public Shape {
public:
    MeshShape(Engine *engine, const std::wstring &name);
    virtual ~MeshShape();

    virtual void attributesChanged(bool all);
    virtual void draw(Node *node);
    virtual bool contains(const Vector<2, float> &point);
    virtual Vector<2, float> getMinimum();
    virtual Vector<2, float> getMaximum();
    virtual bool getVertexBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix,
                                 bool &first);
    virtual void storePositions();
    virtual void restorePositions();
    virtual void stretchPositions(const Vector<2, float> &origin, const Vector<2, float> &size,
                                  const Vector<2, float> &target, const Vector<2, float> &targetSize,
                                  const Matrix<float> &from, const Matrix<float> &to);
    virtual Shape *clone();
    virtual void fitBounds(const Vector<2, float> &minimum, const Vector<2, float> &maximum);
    virtual void updateBoundsAndTextures();

    void setFaces(const std::vector<std::vector<int>> &faces);
    void updateTextureMatrix();
    void applyVertexParameters();

    // Accessors for the mode bits of m_flags.
    bool isStroked() const { return (m_flags >> MESH_STROKE) & 1; }
    bool isFilled() const { return !((m_flags >> MESH_NO_FILL) & 1); }
    bool isOpen() const { return (m_flags >> MESH_OPEN) & 1; }
    bool isExtruded() const { return (m_flags >> MESH_EXTRUDED) & 1; }

    // The data members are public as in plasma (SmoothMeshShape and the engine read them directly).
    ContinuousArrayAttribute<Vector<2, float>> m_vertexPositions;      // "positions"
    ContinuousArrayAttribute<Vector<2, float>> m_vertexTexCoords;      // "texCoords"
    ContinuousArrayAttribute<Vector<4, float>> m_vertexColors;         // "colors" (fill)
    ContinuousArrayAttribute<Vector<4, float>> m_strokeColors;         // "strokeColors"
    ContinuousArrayAttribute<Vector<4, float>> m_extrusionBackColors;  // "extrusionBackColors"
    ContinuousArrayAttribute<Vector<4, float>> m_extrusionFrontColors; // "extrusionFrontColors"
    ContinuousArrayAttribute<float> m_strokeWidths;                    // "strokeWidths"
    std::vector<int> m_vertexFlags;                                    // VertexKind | VertexConstraintFlag bits
    std::vector<float> m_vertexParameters;
    ContinuousAttribute<Vector<2, float>> m_textureTranslation; // "textureTranslation"
    ContinuousAttribute<Vector<2, float>> m_texturePivot;       // "texturePivot"
    ContinuousAttribute<Vector<3, float>> m_textureRotation;    // "textureRotation"
    ContinuousAttribute<Matrix<float>> m_textureDeformation;    // "textureDeformation"
    ContinuousAttribute<float> m_textureOpacity;                // "textureOpacity" (1.0)
    ContinuousAttribute<float> m_textureBrightness;             // "textureBrightness" (1.0)
    ContinuousAttribute<float> m_textureContrast;               // "textureContrast" (1.0)
    ContinuousAttribute<float> m_textureSaturation;             // "textureSaturation" (1.0)
    ContinuousAttribute<float> m_strokeTextureOpacity;          // "strokeTextureOpacity" (1.0)
    ContinuousAttribute<float> m_strokeTextureBrightness;       // "strokeTextureBrightness" (1.0)
    ContinuousAttribute<float> m_strokeTextureContrast;         // "strokeTextureContrast" (1.0)
    ContinuousAttribute<float> m_strokeTextureSaturation;       // "strokeTextureSaturation" (1.0)
    ContinuousAttribute<float> m_strokeTextureStretch;          // "strokeTextureStretch" (1.0)
    ContinuousAttribute<Matrix<float>> m_extrusionMatrix;       // "extrusionMatrix"
    DiscreteAttribute<int> m_texture;                           // "texture" (-1: none)
    DiscreteAttribute<int> m_strokeTexture;                     // "strokeTexture" (-1: none)
    unsigned m_flags;                                           // MeshFlags; copied by clone
    int m_strokeJointType;                                      // StrokeJointType; copied by Clone
    int m_strokeCapType;                                        // StrokeCapType; copied by Clone
    int m_strokeAlignment;                                      // StrokeAlignment; copied by Clone
    std::vector<std::vector<int>> m_faces;                      // vertex indices per face (setFaces)
    Vector<2, float> m_minimum;                                 // Cached minimum bound.
    Vector<2, float> m_maximum;                                 // Cached maximum bound.
    Texture *m_fillTexture;                                     // from m_texture
    Texture *m_outlineTexture;                                  // from m_strokeTexture
    Matrix<float> m_textureMatrix;                              // updateTextureMatrix
    std::set<std::pair<int, int>> m_boundaryEdges;
    std::vector<std::vector<int>> m_contours; // boundary loops (setFaces)
};

}
