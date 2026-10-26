// SmoothMeshShape refines polygon meshes and rebuilds fill, stroke and extrusion drawings.

#pragma once
#include <windows.h>
#include "plasma/Parallel.h"
#include "plasma/MeshShape.h"
#include "plasma/Drawing.h"
#include "plasma/QuadMesh.h"

namespace plasma {
class StaticMeshShape;

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

    int getSubdivisions() { return m_subdivisions; }
    int getStrokePattern() { return m_strokePattern; }
    float getStrokeDash() { return m_strokeDash; }
    float getStrokeGap() { return m_strokeGap; }

    void setStrokePattern(int pattern) { m_strokePattern = pattern; }
    void setStrokeDash(float dash) { m_strokeDash = dash; }
    void setStrokeGap(float gap) { m_strokeGap = gap; }
    void updateSubdivision();
    void setFaces(const std::vector<std::vector<int>> &faces, bool update);
    // Clamped to 0..6.
    void setSubdivisions(int subdivisions, bool update);
    void lock() { EnterCriticalSection(&m_lock); }
    void unlock();
    int getEffectiveSubdivisions();
    StaticMeshShape *convertToStaticMeshShape();

    bool advanceStroke(int contour, int segment, float fraction, float distance, int &resultSegment,
                       float &resultFraction);
    // First: the dash's opening pair (buildDashedStroke passes it; the body ignores it).
    int appendStrokePair(int contour, int vertex, float fraction, bool first);
    void appendRoundCap(int contour, int vertex, float fraction, int atEnd, int segments);
    void appendSquareCap(int contour, int vertex, float fraction, int reverse);
    void buildDashedStroke(int contour, float dash, float gap);
    void updateDrawingTextures();
    void updateDrawings();
    void updateSubdivisionDrawings(int positionElement, int colorElement, int textureElement);

    QuadMesh m_levels[6];

    int m_subdivisions;
    int m_strokeSegments;
    bool m_limitSubdivisions;
    bool m_updatingDrawings;
    std::vector<QuadVertex> m_vertices;
    std::vector<QuadVertex> m_edgeVertices;
    std::vector<QuadVertex> m_faceVertices;
    std::vector<std::vector<int>> m_adjacentFaces;

    typedef Vector<3, int> ControlEdge;
    std::vector<ControlEdge> m_controlEdges;
    std::vector<std::vector<Vector<4, float>>> m_contourColors;
    std::vector<std::vector<Vector<4, float>>> m_contourStrokeColors;
    std::vector<std::vector<Vector<4, float>>> m_contourFrontColors;
    std::vector<std::vector<Vector<4, float>>> m_contourBackColors;
    std::vector<std::vector<float>> m_contourWidths;
    std::vector<std::vector<Vector<2, float>>> m_contourCenters;
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

void refineContourPositions(std::vector<Vector<2, float>> &positions, std::vector<int> &types, int step, int segments,
                            bool closed, float weight, int level);
void refineContourTypedColors(std::vector<Vector<4, float>> &colors, std::vector<int> &types, int step, int segments,
                              bool closed, float weight, int level);
void refineContourWidths(std::vector<float> &widths, std::vector<int> &types, int step, int segments, bool closed,
                         float weight, int level);
void refineContourColors(std::vector<Vector<4, float>> &colors, int step, int segments, bool closed, float weight);

}
