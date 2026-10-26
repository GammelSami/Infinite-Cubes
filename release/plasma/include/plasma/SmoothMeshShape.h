// SmoothMeshShape renders a subdivided quad mesh with optional stroke and extrusion.
#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "plasma/MeshShape.h"
#include "plasma/QuadMesh.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

namespace plasma {

class Drawing;
class Engine;
class Node;
class Shape;
class StaticMeshShape;

// SmoothMeshShape::m_strokePattern (values from plasma).
enum StrokePattern {
    STROKE_PATTERN_SOLID = 0,
    STROKE_PATTERN_DASHED = 1 // m_strokeDash and m_strokeGap apply
};

class SmoothMeshShape : public MeshShape {
public:
    SmoothMeshShape(Engine *engine, const std::wstring &name);
    virtual ~SmoothMeshShape();

    virtual void attributesChanged(bool all);
    virtual void draw(Node *node);
    virtual bool intersects(const Vector<2, float> &point, float radius, const Matrix<float> &matrix,
                            const Matrix<float> &inverse);
    virtual bool contains(const Vector<2, float> &point);
    virtual bool getTransformedBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix,
                                      bool &first);
    virtual Shape *clone();
    virtual void shownChanged(bool shown);
    virtual void releaseCache();
    virtual bool isCacheEmpty();

    int getSubdivisions() { return m_subdivisions; }
    int getStrokePattern() { return m_strokePattern; }
    float getStrokeDash() { return m_strokeDash; }
    float getStrokeGap() { return m_strokeGap; }
    void setStrokePattern(int pattern) { m_strokePattern = pattern; }
    void setStrokeDash(float dash) { m_strokeDash = dash; }
    void setStrokeGap(float gap) { m_strokeGap = gap; }

    void setSubdivisions(int count, bool update);
    void setFaces(const std::vector<std::vector<int>> &faces, bool update);
    void updateSubdivision();
    int getEffectiveSubdivisions();
    StaticMeshShape *convertToStaticMeshShape();

    bool advanceStroke(int contour, int segment, float fraction, float distance, int &resultSegment,
                       float &resultFraction); // (recursive)
    int appendStrokePair(int contour, int vertex, float fraction, bool first);
    void appendRoundCap(int contour, int vertex, float fraction, int atEnd, int segments); // (sinf/cosf)
    void appendSquareCap(int contour, int vertex, float fraction, int reverse);
    void buildDashedStroke(int contour, float dash, float gap);

    void updateDrawingTextures(); // textures through the engine
    void updateDrawings();        // sets m_updatingDrawings while it runs
    void updateSubdivisionDrawings(int positionElement, int colorElement, int textureElement);

    // Face, corner and edge-vertex indices.
    typedef Vector<3, int> ControlEdge;

    // Public as in plasma: the engine and the PLX loader reach them directly.
    QuadMesh m_levels[6];               // Constructed with the shape.
    int m_subdivisions;                 // 3; setSubdivisions
    int m_strokeSegments;               // Round-cap segment count, initially 20.
    bool m_limitSubdivisions;           // tested by getEffectiveSubdivisions
    bool m_updatingDrawings;            // Set and cleared during updates; no reader found.
    std::vector<QuadVertex> m_vertices; // Mesh record storage.
    std::vector<QuadVertex> m_edgeVertices;
    std::vector<QuadVertex> m_faceVertices;
    std::vector<std::vector<int>> m_adjacentFaces;
    std::vector<ControlEdge> m_controlEdges;
    std::vector<std::vector<Vector<4, float>>> m_contourColors;
    std::vector<std::vector<Vector<4, float>>> m_contourStrokeColors;
    std::vector<std::vector<Vector<4, float>>> m_contourFrontColors;
    std::vector<std::vector<Vector<4, float>>> m_contourBackColors;
    std::vector<std::vector<float>> m_contourWidths;
    std::vector<std::vector<Vector<2, float>>> m_contourCenters; // refined positions
    std::vector<std::vector<Vector<2, float>>> m_contourTexCoords;
    std::vector<std::vector<int>> m_strokePairs;
    std::vector<std::vector<Vector<2, float>>> m_strokeOutlinePositions;
    std::vector<std::vector<Vector<4, float>>> m_strokeOutlineColors;
    std::vector<std::vector<Vector<2, float>>> m_strokeOutlineTexCoords;
    float m_strokeDash;  // Initially 1.0.
    float m_strokeGap;   // Initially 1.0.
    int m_strokePattern; // StrokePattern, initially solid (0).
    std::vector<std::vector<int>> m_contourTypes;
    float m_smoothWeight;        // Read and written during drawing updates.
    unsigned m_dirty;            // DrawingDirtyFlags set by the constructor.
    Drawing *m_fillDrawing;      // drawn unless MESH_NO_FILL
    Drawing *m_strokeDrawing;    // drawn with MESH_STROKE
    Drawing *m_extrusionDrawing; // drawn with MESH_EXTRUDED
    CRITICAL_SECTION m_lock;
};

}
