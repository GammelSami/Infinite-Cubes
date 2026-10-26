// CurveShape.cpp: animated stroked curves and their refined drawing arrays.
#pragma once
#include "plasma/VertexFlags.h"
#include "plasma/Shape.h"
#include "plasma/ContinuousArrayAttribute.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/Matrix.h"

namespace plasma {

class Drawing;

class CurveShape : public Shape {
public:
    CurveShape(Engine *engine, const std::wstring &name);
    virtual ~CurveShape();
    virtual void attributesChanged(int reason);
    virtual void draw(Node *node);
    virtual bool contains(const Vector<2, float> &point);
    virtual Vector<2, float> getMinimum();
    virtual Vector<2, float> getMaximum();
    virtual void storePositions();
    virtual void restorePositions();
    virtual void stretchPositions(const Vector<2, float> &origin, const Vector<2, float> &size,
                                  const Vector<2, float> &target, const Vector<2, float> &targetSize,
                                  const Matrix<float> &from, const Matrix<float> &to);
    virtual Shape *clone();

    void setVertexFlags(const std::vector<int> &flags);
    void setSubdivisions(int subdivisions);
    void setStrokeSegments(int segments);
    void setClosed(bool closed);
    virtual void updateDrawing();

    ContinuousArrayAttribute<Vector<2, float>> m_vertexPositions;
    ContinuousArrayAttribute<float> m_vertexWidths;
    ContinuousArrayAttribute<Vector<2, float>> m_vertexTexCoords;
    ContinuousArrayAttribute<Vector<4, float>> m_vertexColors;
    ContinuousAttribute<Vector<2, float>> m_textureTranslation;
    ContinuousAttribute<Vector<2, float>> m_texturePivot;
    ContinuousAttribute<Vector<3, float>> m_textureRotation;
    ContinuousAttribute<Matrix<float>> m_textureDeformation;
    ContinuousArrayAttribute<float> m_textureWeights;
    std::vector<int> m_unused374; // copied by Clone; nothing reads it
    bool m_closed;
    int m_strokeCapType;
    int m_subdivisions;
    int m_strokeSegments;
    std::vector<int> m_vertexFlags;
    Vector<2, float> m_minimum;
    Vector<2, float> m_maximum;
    std::vector<Vector<2, float>> m_outlinePositions; // the stroke's side pairs (centre -/+ normal)
    std::vector<Vector<2, float>> m_startCap;
    std::vector<Vector<2, float>> m_endCap;
    std::vector<Vector<4, float>> m_colors;
    std::vector<float> m_widths;
    std::vector<Vector<2, float>> m_centers; // the refined centre line
    Drawing *m_drawing;
};

void refineCurvePositions(std::vector<Vector<2, float>> &values, std::vector<int> flags, int stride, int flagStride,
                          int count, bool closed, unsigned offset);
void refineCurveColors(std::vector<Vector<4, float>> &values, std::vector<int> flags, int stride, int flagStride,
                       int count, bool closed, unsigned offset);
void refineCurveWidths(std::vector<float> &values, std::vector<int> flags, int stride, int flagStride, int count,
                       bool closed, unsigned offset);
}
