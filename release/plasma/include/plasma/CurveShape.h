// plasma::CurveShape: an animated stroked curve (vertex positions, widths, texture coordinates and colours, plus a
// texture transform), refined into outline, cap, colour, width and centre-line arrays and drawn with one Drawing.
#pragma once
#include <string>
#include <vector>
#include "plasma/Shape.h"
#include "plasma/ContinuousArrayAttribute.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

namespace plasma {

class Drawing;

class CurveShape : public Shape {
public:
    CurveShape(Engine *engine, const std::wstring &name);
    virtual ~CurveShape();
    virtual void attributesChanged(bool all);
    virtual void draw(Node *node) {}
    virtual bool contains(const Vector<2, float> &point);
    virtual Vector<2, float> getMinimum();
    virtual Vector<2, float> getMaximum();
    virtual void storePositions();
    virtual void restorePositions();
    virtual void stretchPositions(const Vector<2, float> &origin, const Vector<2, float> &size,
                                  const Vector<2, float> &target, const Vector<2, float> &targetSize,
                                  const Matrix<float> &from, const Matrix<float> &to);
    virtual Shape *clone();
    virtual void updateDrawing();

    void setVertexFlags(const std::vector<int> &flags);
    void setSubdivisions(int subdivisions); // clamped to 0..100
    void setStrokeSegments(int segments);
    void setClosed(bool closed);

    ContinuousArrayAttribute<Vector<2, float>> m_vertexPositions; // "vertexPositions"
    ContinuousArrayAttribute<float> m_vertexWidths;               // "vertexWidths"
    ContinuousArrayAttribute<Vector<2, float>> m_vertexTexCoords; // "vertexTexCoords"
    ContinuousArrayAttribute<Vector<4, float>> m_vertexColors;    // "vertexColors"
    ContinuousAttribute<Vector<2, float>> m_textureTranslation;   // "textureTranslation"
    ContinuousAttribute<Vector<2, float>> m_texturePivot;         // "texturePivot"
    ContinuousAttribute<Vector<3, float>> m_textureRotation;      // "textureRotation"
    ContinuousAttribute<Matrix<float>> m_textureDeformation;      // "textureDeformation"
    ContinuousArrayAttribute<float> m_textureWeights;             // "textureWeights"
    // Opaque, trivially copied element type.
    std::vector<unsigned long long> m_unused560;
    bool m_closed;                                    // setClosed
    int m_strokeCapType;                              // 1 from the constructor
    int m_subdivisions;                               // setSubdivisions; 4 from the constructor
    int m_strokeSegments;                             // setStrokeSegments; 20 from the constructor
    std::vector<int> m_vertexFlags;                   // setVertexFlags
    Vector<2, float> m_minimum;                       // getMinimum
    Vector<2, float> m_maximum;                       // getMaximum
    std::vector<Vector<2, float>> m_outlinePositions; // the stroke's side pairs
    std::vector<Vector<2, float>> m_startCap;
    std::vector<Vector<2, float>> m_endCap;
    std::vector<Vector<4, float>> m_colors;  // Outline data.
    std::vector<float> m_widths;             // Stroke index data.
    std::vector<Vector<2, float>> m_centers; // the refined centre line
    Drawing *m_drawing;
};

}
