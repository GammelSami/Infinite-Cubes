// MeshShape builds polygonal shapes with animated attributes.

#pragma once
#include "plasma/VertexFlags.h"
#include <map>
#include <set>
#include <vector>
#include "plasma/Shape.h"
#include "plasma/ContinuousArrayAttribute.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/DiscreteAttribute.h"
#include "plasma/Matrix.h"

namespace plasma {

class Texture;

// Individual flags are bit indices; use (1 << flag) to form a mask.
enum MeshFlags {
    MESH_CONTOUR = 0,
    MESH_STROKE = 1,
    MESH_NO_FILL = 2,
    MESH_OPEN = 3,
    MESH_EXTRUDED = 4
};

// Whole-field masks; the flags above are bit indices.
enum MeshFlagMasks {
    MESH_FLAGS_NONE = 0
};

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
    STROKE_ALIGNMENT_INNER = -1,
    STROKE_ALIGNMENT_NORMAL = 0,
    STROKE_ALIGNMENT_OUTER = 1
};
enum StrokePattern {
    STROKE_PATTERN_SOLID = 0,
    STROKE_PATTERN_DASHED = 1
};

class MeshShape : public Shape {
public:
    MeshShape(Engine *engine, const std::wstring &name);
    virtual ~MeshShape();
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
    virtual void attributesChanged(int reason);

    bool isStroked() const { return (m_flags >> MESH_STROKE) & 1; }
    bool isFilled() const { return !((m_flags >> MESH_NO_FILL) & 1); }
    bool isClosed() const { return !((m_flags >> MESH_OPEN) & 1); }
    bool isContour() const { return m_flags & (1 << MESH_CONTOUR); }
    bool isExtruded() const { return (m_flags >> MESH_EXTRUDED) & 1; }
    bool isOpen() const { return (m_flags >> MESH_OPEN) & 1; }
    virtual void fitBounds(const Vector<2, float> &minimum, const Vector<2, float> &maximum);
    virtual void updateBoundsAndTextures();

    const Matrix<float> &getTextureMatrix() const { return m_textureMatrix; }
    void updateTextureMatrix();
    void applyVertexParameters();

    void setFaces(const std::vector<std::vector<int>> &faces);

    int getStrokeJointType() { return m_strokeJointType; }
    int getStrokeCapType() { return m_strokeCapType; }

    void setStrokeJointType(int joint) { m_strokeJointType = joint; }
    void setStrokeCapType(int cap) { m_strokeCapType = cap; }
    void setStrokeAlignment(int alignment) { m_strokeAlignment = alignment; }

    ContinuousArrayAttribute<Vector<2, float>> m_vertexPositions;
    ContinuousArrayAttribute<Vector<2, float>> m_vertexTexCoords;
    ContinuousArrayAttribute<Vector<4, float>> m_vertexColors;
    ContinuousArrayAttribute<Vector<4, float>> m_strokeColors;
    ContinuousArrayAttribute<Vector<4, float>> m_extrusionBackColors;
    ContinuousArrayAttribute<Vector<4, float>> m_extrusionFrontColors;
    ContinuousArrayAttribute<float> m_strokeWidths;
    std::vector<int> m_vertexFlags;
    std::vector<float> m_vertexParameters;
    ContinuousAttribute<Vector<2, float>> m_textureTranslation;
    ContinuousAttribute<Vector<2, float>> m_texturePivot;
    ContinuousAttribute<Vector<3, float>> m_textureRotation;
    ContinuousAttribute<Matrix<float>> m_textureDeformation;
    ContinuousAttribute<float> m_textureOpacity;
    ContinuousAttribute<float> m_textureBrightness;
    ContinuousAttribute<float> m_textureContrast;
    ContinuousAttribute<float> m_textureSaturation;
    ContinuousAttribute<float> m_strokeTextureOpacity;
    ContinuousAttribute<float> m_strokeTextureBrightness;
    ContinuousAttribute<float> m_strokeTextureContrast;
    ContinuousAttribute<float> m_strokeTextureSaturation;
    ContinuousAttribute<float> m_strokeTextureStretch;
    ContinuousAttribute<Matrix<float>> m_extrusionMatrix;
    DiscreteAttribute<int> m_texture;
    DiscreteAttribute<int> m_strokeTexture;
    unsigned m_flags;
    int m_strokeJointType;
    int m_strokeCapType;
    int m_strokeAlignment;
    std::vector<std::vector<int>> m_faces;
    Vector<2, float> m_minimum;
    Vector<2, float> m_maximum;

    Texture *getFillTexture() const { return m_fillTexture; }
    Texture *getOutlineTexture() const { return m_outlineTexture; }
    Texture *m_fillTexture;
    Texture *m_outlineTexture;
    Matrix<float> m_textureMatrix;
    std::set<std::pair<int, int>> m_boundaryEdges;
    std::vector<std::vector<int>> m_contours;
};
}
