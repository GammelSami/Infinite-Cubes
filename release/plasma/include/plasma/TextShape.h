// plasma::TextShape: text with animatable contents and colours, drawn with a ScalableFont looked up by file name, with
// cached bounds.
#pragma once
#include <windows.h>
#include "plasma/TextIds.h"
#include <string>
#include "plasma/Shape.h"
#include "plasma/DiscreteAttribute.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

namespace plasma {

class ScalableFont;

class TextShape : public Shape {
public:
    TextShape(Engine *engine, const std::wstring &text, const std::wstring &name);
    virtual ~TextShape();
    virtual void attributesChanged(bool all);
    virtual void draw(Node *node);
    virtual bool intersects(const Vector<2, float> &point, float radius, const Matrix<float> &matrix,
                            const Matrix<float> &inverse);
    virtual bool contains(const Vector<2, float> &point);
    virtual bool isDeformable() { return false; }
    virtual Vector<2, float> getMinimum();
    virtual Vector<2, float> getMaximum();
    virtual Shape *clone();
    virtual void updateBounds(const Matrix<float> &matrix);

    void setText(const std::wstring &text);

    // Public as in plasma: the engine's PLX reader and game code set the text settings directly.
    DiscreteAttribute<std::wstring> m_string;               // "string"
    ContinuousAttribute<Vector<4, float>> m_color;          // "colors"
    ContinuousAttribute<Vector<4, float>> m_strokeColor;    // "strokeColors"
    ContinuousAttribute<Vector<4, float>> m_extrusionColor; // "extrusionColors"
    float m_pixelSize;
    float m_strokeRadius;
    float m_spacing;
    float m_lineSpacing;
    std::wstring m_fontName; // the font file the font engine loads
    float m_wrapWidth;
    ScalableFont *m_font;       // loaded by attributesChanged
    int m_flags;                // TextFlags (plasma/Font.h)
    unsigned m_hintingFlags;    // TextHintingFlags
    Vector<2, float> m_minimum; // getMinimum, Contains, Intersects
    Vector<2, float> m_maximum; // getMaximum, Contains, Intersects
    CRITICAL_SECTION m_lock;
};

}
