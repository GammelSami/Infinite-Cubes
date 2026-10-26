// Text with animatable contents and colors, cached bounds and a font face.
#pragma once
#include "plasma/Alignment.h"
#include <string>
#include <windows.h>
#include "plasma/Shape.h"
#include "plasma/DiscreteAttribute.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/Matrix.h"
#include "plasma/Font.h" // TextFlags (m_flags)

namespace plasma {

class ScalableFont;
class FontEngine;

// TextShape::m_hintingFlags. Individual flags are bit indices; use (1 << flag) to form a mask.
enum TextHintingFlags {
    TEXT_HINTED = 0 // measured and drawn hinted (the PLX reader's LOAD_HINTED_TEXT)
};

// Whole-field masks; the flags above are bit indices.
enum TextHintMasks {
    TEXT_FLAGS2_NONE = 0
};

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

    void setText(const std::wstring &text, Node *node);
    void lock();
    void unlock();

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
    int m_flags;             // TextFlags (plasma/Font.h)
    unsigned m_hintingFlags; // TextHintingFlags
    Vector<2, float> m_minimum;
    Vector<2, float> m_maximum;
    CRITICAL_SECTION m_lock;
};

}

// Node's text collector needs the complete TextShape type.
#include "plasma/Node.h"

namespace plasma {

// The text shapes of the node and its descendants, depth first.
inline void Node::collectTextShapes(std::list<TextShape *> &shapes)
{
    if (m_shape) {
        TextShape *text = dynamic_cast<TextShape *>(m_shape);
        if (text)
            shapes.push_back(text);
    }
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->collectTextShapes(shapes);
}

}
