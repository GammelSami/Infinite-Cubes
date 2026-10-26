// Display controls scene visibility, opacity and clipping.
#pragma once
#include <list>
#include <set>
#include "plasma/Keyable.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/DiscreteAttribute.h"
#include "plasma/Vector.h"

namespace svg {
class SVGReader;
}

namespace plasma {

class Node;
enum DisplayVisibility {
    DISPLAY_INVISIBLE = 0,
    DISPLAY_VISIBLE = 1
};
enum DisplayClipping {
    DISPLAY_CLIPPING_DISABLED = 0,
    DISPLAY_CLIPPING_ENABLED = 1
};

// Individual flags are bit indices; use (1 << flag) to form a mask.
enum DisplayFlags {
    DISPLAY_BLUR = 0,
    DISPLAY_MODULATION = 1,
    DISPLAY_SCREEN = 2
};

// Whole-field masks; the flags above are bit indices.
enum DisplayFlagMasks {
    DISPLAY_FLAGS_NONE = 0
};

class Display : public Keyable {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    Display(Engine *engine, const std::wstring &name);
    virtual ~Display();
    virtual Display *clone();

    float getBlurRadius() { return m_blurRadius.value(); }
    void setFillColor(const Vector<4, float> &color) { m_fillColor = color; }

    int countNodes() const { return (int)m_nodes.size(); }

protected:
    friend class Widget;
    friend class PopUpButton;
    friend class Edit;
    friend class Node;
    friend class Engine;
    friend class GLDrawing;
    friend class GLEngine;
    friend class D3D9Engine;
    friend class D3D9Drawing;
    DiscreteAttribute<int> m_visibility;
    DiscreteAttribute<int> m_clipping;
    ContinuousAttribute<Vector<4, float>> m_fillColor;
    ContinuousAttribute<Vector<4, float>> m_strokeColor;
    ContinuousAttribute<float> m_blurRadius;
    int m_flags;
    std::set<Node *> m_nodes;
    std::list<Display *>::iterator m_engineEntry;
};

}
