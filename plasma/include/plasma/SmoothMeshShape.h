// Subdivision topology and fill, stroke and extrusion drawings.
#pragma once
#include <windows.h>
#include "Concurrency.h"
#include "EditableMesh.h"
#include "plasma/MeshShape.h"
#include "plasma/Drawing.h"
#include "plasma/StaticMeshShape.h"
#include "plasma/QuadMesh.h"

namespace plasma {

// Subdivision limits shared by the PDF and SVG smooth-mesh importers.
enum {
    ImportedRadialGradientSubdivisions = 3,
    MaxImportedPathSubdivisions = 3
};

class SmoothMeshShape : public MeshShape {
public:
    SmoothMeshShape(Engine *engine, const std::wstring &name);

    ~SmoothMeshShape();
    virtual void attributesChanged(int reason);
    virtual void draw(Node *node);
    virtual bool intersects(const Vector<2, float> &point, float radius, const Matrix<float> &drawingMatrix,
                            const Matrix<float> &pointMatrix);
    virtual bool contains(const Vector<2, float> &point);
    virtual bool getTransformedBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix,
                                      bool &first);
    virtual Shape *clone();
    virtual void shownChanged(bool shown);
    virtual void releaseCache();
    virtual bool isCacheEmpty();

    // Subdivision and stroke-pattern settings, serialized as "SmoothMeshShape.subdivisions", ".strokePattern",
    // ".strokeDash" and ".strokeGap".
    int getSubdivisions() { return m_subdivisions; }
    int getStrokePattern() { return m_strokePattern; }
    float getStrokeDash() { return m_strokeDash; }
    float getStrokeGap() { return m_strokeGap; }
    // Setters for the subdivision and stroke-pattern settings used by MeshModule::Apply.
    void setStrokePattern(int pattern) { m_strokePattern = pattern; }
    void setStrokeDash(float dash) { m_strokeDash = dash; }
    void setStrokeGap(float gap) { m_strokeGap = gap; }
    void setSubdivisions(int count, bool update);
    void setStrokeSegments(int count);
    void updateSubdivision();
    const Vector<2, float> *getSubdivisionPosition(int vertex);
    void setFaces(const std::vector<std::vector<int>> &faces, bool update);
    void notifyChanged(bool update);
    void lock();
    void unlock();
    int getEffectiveSubdivisions();
    StaticMeshShape *convertToStaticMeshShape();

    bool advanceStroke(int contour, int segment, float fraction, float distance, int &resultSegment,
                       float &resultFraction);
    int appendStrokePair(int contour, int vertex, float fraction);
    void appendRoundCap(int contour, int vertex, float fraction, int atEnd, int segments);
    void appendSquareCap(int contour, int vertex, float fraction, int reverse);
    void buildDashedStroke(int contour, float dash, float gap);
    static bool hasFill(unsigned flags) { return !(flags & (1 << MESH_NO_FILL)); }

    static bool hasStroke(unsigned flags) { return (flags & (1 << MESH_STROKE)) != 0; }

    static bool hasExtrusion(unsigned flags) { return (flags & (1 << MESH_EXTRUDED)) != 0; }

    void updateDrawingTextures();
    void updateDrawings();
    void updateSubdivisionDrawings(int positionElement, int colorElement, int textureElement);
    void bakeSubdivision();

    QuadMesh m_levels[6];

    // MeshShape flags used for drawing dispatch.
    bool drawsExtrusion() const { return (m_flags & (1 << MESH_EXTRUDED)) != 0; }

    bool drawsFill() const { return !(m_flags & (1 << MESH_NO_FILL)); }

    bool drawsStroke() const { return (m_flags & (1 << MESH_STROKE)) != 0; }

    int m_subdivisions;
    int m_strokeSegments;
    bool m_limitSubdivisions;
    bool m_updatingDrawings; // Set during UpdateDrawings; never read.
    std::vector<QuadVertex> m_vertices;
    std::vector<QuadVertex> m_edgeVertices;
    std::vector<QuadVertex> m_faceVertices;
    std::vector<std::vector<int>> m_adjacentFaces;
    // Face, corner and edge-vertex indices.
    typedef Vector<3, int> ControlEdge;
    std::vector<ControlEdge> m_controlEdges;
    std::vector<std::vector<Vector<4, float>>> m_contourColors;
    std::vector<std::vector<Vector<4, float>>> m_contourStrokeColors;
    std::vector<std::vector<Vector<4, float>>> m_contourFrontColors;
    std::vector<std::vector<Vector<4, float>>> m_contourBackColors;
    std::vector<std::vector<float>> m_contourWidths;
    std::vector<std::vector<Vector<2, float>>> m_contourCenters; // Refined positions.
    std::vector<std::vector<Vector<2, float>>> m_contourTexCoords;
    std::vector<std::vector<int>> m_strokePairs;
    std::vector<std::vector<Vector<2, float>>> m_strokeOutlinePositions;
    std::vector<std::vector<Vector<4, float>>> m_strokeOutlineColors;
    std::vector<std::vector<Vector<2, float>>> m_strokeOutlineTexCoords;
    float m_strokeDash;
    float m_strokeGap;
    int m_strokePattern;
    std::vector<std::vector<int>> m_contourTypes;
    float m_smoothWeight;
    unsigned m_dirty;
    Drawing *m_fillDrawing;
    Drawing *m_strokeDrawing;
    Drawing *m_extrusionDrawing;
    CRITICAL_SECTION m_lock;
};

void smoothDrawingReset(Drawing *drawing);

void refineContourPositions(std::vector<int> &types, int step, std::vector<Vector<2, float>> &positions, int segments,
                            bool closed, float weight, int level);
void refineContourTypedColors(std::vector<int> &types, int step, std::vector<Vector<4, float>> &colors, int segments,
                              bool closed, float weight, int level);
void refineContourWidths(std::vector<float> &widths, std::vector<int> &types, int step, int segments, bool closed,
                         float weight, int level);
void refineContourColors(int step, std::vector<Vector<4, float>> &colors, int segments, bool closed, float weight);
double strokeDistance(float length, float firstWidth, float secondWidth);

}

inline int &EditableMesh::vertexFlags(MeshVertex *vertex)
{
    return m_shape->m_vertexFlags[vertexIndex(vertex)];
}
