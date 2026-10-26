// Text with animatable contents and colors, cached bounds and a font face.
#pragma once
#include "plasma/Alignment.h"
#include <windows.h>
#include "plasma/Shape.h"
#include "plasma/DiscreteAttribute.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/Matrix.h"
#include "plasma/PixelFont.h" // TextFlags (m_flags).

namespace plasma {

class ScalableFont;
class FontEngine;

// TextShape::m_hintingFlags.
enum TextHintingFlags {
    TEXT_HINTED = 0 // Measured and drawn hinted (the PLX reader's LOAD_HINTED_TEXT).
};

enum TextHintMasks {
    TEXT_HINTING_NONE = 0
};

// The alignment argument of SetHorizontalAlignment and SetVerticalAlignment (TextModule's
// m_horizontalAlignment/m_verticalAlignment, "text:horizontal:left"... "text:vertical:bottom"); the same numbering
// as WidgetAlignment.
class TextShape : public Shape {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    TextShape(Engine *engine, const std::wstring &text, const std::wstring &name);
    virtual ~TextShape();
    virtual void attributesChanged(int reason);
    virtual void draw(Node *node);
    virtual bool intersects(const Vector<2, float> &point, float radius, const Matrix<float> &matrix,
                            const Matrix<float> &inverse);
    virtual bool contains(const Vector<2, float> &point);
    // Text is not deformed by an enclosing widget.
    virtual bool isDeformable() { return false; }
    virtual Vector<2, float> getMinimum();
    virtual Vector<2, float> getMaximum();
    virtual Shape *clone();
    virtual void updateBounds(const Matrix<float> &matrix);

    int getFlags() const { return m_flags; }
    void setFlags(int flags) { m_flags = flags; }

    void setText(const std::wstring &text);
    void lock();
    void unlock();
    // Sets horizontal alignment in TEXT_HORIZONTAL_MASK: right sets TEXT_RIGHT and centre sets TEXT_CENTER. Defined in
    // StrokeWidthModule.cpp.
    void setHorizontalAlignment(int alignment);
    // Sets vertical alignment in TEXT_VERTICAL_MASK: bottom sets TEXT_BOTTOM and middle sets TEXT_MIDDLE. Defined in
    // WindowsUI.cpp.
    void setVerticalAlignment(int alignment);

    DiscreteAttribute<std::wstring> m_string;
    ContinuousAttribute<Vector<4, float>> m_color;
    ContinuousAttribute<Vector<4, float>> m_strokeColor;
    ContinuousAttribute<Vector<4, float>> m_extrusionColor;
    float m_pixelSize;
    float m_strokeRadius;
    float m_spacing;
    float m_lineSpacing;
    std::wstring m_fontName;
    float m_wrapWidth;
    ScalableFont *m_font;
    int m_flags;             // TextFlags (plasma/PixelFont.h).
    unsigned m_hintingFlags; // TextHintingFlags.
    Vector<2, float> m_minimum;
    Vector<2, float> m_maximum;
    CRITICAL_SECTION m_lock;
};

}
