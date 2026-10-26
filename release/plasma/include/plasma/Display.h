// Display provides animated fill and stroke colours, blur radius, visibility and clipping for nodes. Attributes are
// public for direct access by game code.
#pragma once
#include <list>
#include <set>
#include <string>
#include "plasma/Keyable.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/DiscreteAttribute.h"
#include "plasma/Vector.h"

namespace plasma {

class Node;

// Display flags controlling blending and rendering.
enum DisplayFlags {
    DISPLAY_BLUR = 0,       // drawn blurred by m_blurRadius
    DISPLAY_MODULATION = 1, // multiplies
    DISPLAY_SCREEN = 2      // screen blending
};

// Values for Display visibility and clipping attributes.
enum DisplayVisibility {
    DISPLAY_INVISIBLE = 0,
    DISPLAY_VISIBLE = 1
};
enum DisplayClipping {
    DISPLAY_CLIPPING_DISABLED = 0,
    DISPLAY_CLIPPING_ENABLED = 1
};

class Display : public Keyable {
    friend class PLXStreamReader; // "Display.visibility" .. "Display.flags"
public:
    Display(Engine *engine, const std::wstring &name);
    virtual ~Display();

    // Unlike Transformation's, it does not call attributesChanged.
    virtual Display *clone();

    int countNodes() const { return m_nodes.size(); } // Accessor used by Node comparisons.

    DiscreteAttribute<int> m_visibility;
    DiscreteAttribute<int> m_clipping;
    ContinuousAttribute<Vector<4, float>> m_fillColor;
    ContinuousAttribute<Vector<4, float>> m_strokeColor;
    ContinuousAttribute<float> m_blurRadius;

protected:
    friend class Node;                            // registers itself in m_nodes; reads m_flags
    friend class Engine;                          // erases m_engineEntry when deleting the display
    friend class D3D11Engine;                     // reads m_flags
    friend class D3D11Drawing;                    // reads m_flags
    int m_flags;                                  // DisplayFlags; zeroed by the constructor, copied by Clone
    std::set<Node *> m_nodes;                     // the nodes that use this display
    std::list<Display *>::iterator m_engineEntry; // Registration entry in the engine's display list.
};

}
