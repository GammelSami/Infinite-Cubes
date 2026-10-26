// MeshShape.cpp: animated mesh vertices, faces and boundary contours.
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

// MeshShape::m_flags ("SmoothMeshShape.flags"); 0 is a filled mesh without stroke. The mesh panel's shape modes
// combine them (MeshModule).
enum MeshFlags {
    MESH_CONTOUR = 0,
    MESH_STROKE = 1,
    MESH_NO_FILL = 2,
    MESH_OPEN = 3,
    MESH_EXTRUDED = 4
};

enum MeshFlagMasks {
    MESH_FLAGS_NONE = 0
};

// m_strokeJointType, m_strokeCapType and m_strokeAlignment ("SmoothMeshShape. strokeJointType", ".strokeCapType",
// ".strokeAlignment"), by the mesh panel's radio buttons; SmoothMeshShape::m_strokePattern (".strokePattern").
// MeshShape::m_vertexFlags ("SmoothMeshShape.vertexFlags"): the corner kind in the low half (MeshInterface's
// Smooth/Sharpen/Bezier/RoundCornerVertices), a constraint in the high half (VertexTool's SetRoundedCorner,
// SetFixedOnEdge).
enum StrokeJointType {
    STROKE_JOINT_SHARP = 0, // "mesh:joints:sharp".
    STROKE_JOINT_ROUND = 1  // "mesh:joints:round".
};
enum StrokeCapType {
    STROKE_CAP_NONE = 0,  // "mesh:caps:none".
    STROKE_CAP_ROUND = 1, // "mesh:caps:round".
    STROKE_CAP_SQUARE = 2 // "mesh:caps:square".
};
enum StrokeAlignment {
    STROKE_ALIGNMENT_INNER = -1, // "mesh:stroke:alignment:inner" (a factor on the offset).
    STROKE_ALIGNMENT_NORMAL = 0,
    STROKE_ALIGNMENT_OUTER = 1
};
enum StrokePattern {
    STROKE_PATTERN_SOLID = 0,
    STROKE_PATTERN_DASHED = 1 // "mesh:stroke:dashed": m_strokeDash and m_strokeGap.
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
    // The shape mode bits as the mesh panel reads them (MeshModule tests the shifted word).
    bool isStroked() const { return (m_flags >> MESH_STROKE) & 1; }    // MESH_STROKE.
    bool isFilled() const { return !((m_flags >> MESH_NO_FILL) & 1); } // MESH_NO_FILL clear.
    bool isClosed() const { return !((m_flags >> MESH_OPEN) & 1); }
    bool isContour() const { return m_flags & (1 << MESH_CONTOUR); }
    bool isExtruded() const { return (m_flags >> MESH_EXTRUDED) & 1; }
    bool isOpen() const { return (m_flags >> MESH_OPEN) & 1; } // MESH_OPEN.
    virtual void fitBounds(const Vector<2, float> &minimum, const Vector<2, float> &maximum);
    virtual void updateBoundsAndTextures();

    const Matrix<float> &getTextureMatrix() const { return m_textureMatrix; }
    void updateTextureMatrix();
    void applyVertexParameters();
    void updateVertexParameter(int vertex);

    std::vector<std::vector<int>> &getFaces();
    void setFaces(const std::vector<std::vector<int>> &faces);
    void setFaces(const std::vector<std::vector<int>> &faces, bool update);
    void notifyChanged(bool update);
    unsigned getFlags();
    void setFlags(unsigned flags);
    // Sets or clears MESH_EXTRUDED; defined in LayerModule.cpp.
    void setExtruded(bool extruded);
    // Stroke joint and cap settings, serialized as "SmoothMeshShape.strokeJointType" and
    // "SmoothMeshShape.strokeCapType".
    int getStrokeJointType() { return m_strokeJointType; }
    int getStrokeCapType() { return m_strokeCapType; }
    // Stroke settings used by MeshModule::Apply. Alignment is serialized as "SmoothMeshShape.strokeAlignment".
    void setStrokeJointType(int joint) { m_strokeJointType = joint; }
    void setStrokeCapType(int cap) { m_strokeCapType = cap; }
    void setStrokeAlignment(int alignment) { m_strokeAlignment = alignment; }

    ContinuousArrayAttribute<Vector<2, float>> m_vertexPositions;      // "SmoothMeshShape.vertexPositions".
    ContinuousArrayAttribute<Vector<2, float>> m_vertexTexCoords;      // "SmoothMeshShape.vertexTexCoords".
    ContinuousArrayAttribute<Vector<4, float>> m_vertexColors;         // "SmoothMeshShape.vertexColors" (fill).
    ContinuousArrayAttribute<Vector<4, float>> m_strokeColors;         // "SmoothMeshShape.strokeColors".
    ContinuousArrayAttribute<Vector<4, float>> m_extrusionBackColors;  // "SmoothMeshShape.extrusionBackColors".
    ContinuousArrayAttribute<Vector<4, float>> m_extrusionFrontColors; // "SmoothMeshShape.extrusionFrontColors".
    ContinuousArrayAttribute<float> m_strokeWidths;                    // "SmoothMeshShape.strokeWidths".
    std::vector<int> m_vertexFlags;                                    // "SmoothMeshShape.vertexFlags".
    std::vector<float> m_vertexParameters;                             // "SmoothMeshShape.vertexParameters".
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
    unsigned m_flags; // MeshFlags.
    int m_strokeJointType;
    int m_strokeCapType;
    int m_strokeAlignment;
    std::vector<std::vector<int>> m_faces;
    Vector<2, float> m_minimum;
    Vector<2, float> m_maximum;
    // Texture identifiers passed by value to texture collection maps and sets.
    Texture *getFillTexture() const { return m_fillTexture; }
    Texture *getOutlineTexture() const { return m_outlineTexture; }
    Texture *m_fillTexture;
    Texture *m_outlineTexture;
    Matrix<float> m_textureMatrix;
    std::set<std::pair<int, int>> m_boundaryEdges; // The faces' directed edges without a reverse (SetFaces).
    std::vector<std::vector<int>> m_contours;
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif

}
