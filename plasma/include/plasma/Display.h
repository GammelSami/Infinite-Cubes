// plasma::Display: a keyable set of display properties (visibility, clipping, fill and stroke colours, blur
// radius) that the engine keeps in a list.
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
class WidgetModule;
class NodeModule;
class Sheet;
class NodeInterface;
class NodeTool;
class Workspace;
class Module;
class PDFWriter;
class ExportModule;
class TextureModule;
class ColorModule;
class SpecialColorModule;
class ShadowModule;
class MeshInterface;
#ifdef PLASMA_PORTABLE
unsigned int hideUserInterface();
#else
unsigned long hideUserInterface();
#endif

namespace plasma {

class Node;

// Display::m_flags ("Display.flags"), named after the node panel's strings.
enum DisplayFlags {
    DISPLAY_BLUR = 0,
    DISPLAY_MODULATION = 1,
    DISPLAY_SCREEN = 2
};

enum DisplayFlagMasks {
    DISPLAY_FLAGS_NONE = 0
};

enum DisplayVisibility {
    DISPLAY_INVISIBLE = 0,
    DISPLAY_VISIBLE = 1
};
enum DisplayClipping {
    DISPLAY_CLIPPING_DISABLED = 0,
    DISPLAY_CLIPPING_ENABLED = 1
};

class Display : public Keyable {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    DiscreteAttribute<int> &visibility() { return m_visibility; }
#ifdef PLASMA_PORTABLE
    void enableShapeClipping()
    {
        m_clipping.value() = DISPLAY_CLIPPING_ENABLED;
        m_flags |= (1 << DISPLAY_MODULATION);
    }
#endif
    Display(Engine *engine, const std::wstring &name);
    virtual ~Display();

    // A new display of the same engine and name with copies of the attributes and flags.
    virtual Display *clone();

    float getBlurRadius() { return m_blurRadius.value(); }

    int countNodes() const { return m_nodes.size(); }

protected:
    friend class Widget;               // Hit tests read the visibility and the clipping.
    friend class Node;                 // A node registers itself in m_nodes.
    friend class ::WidgetModule;       // Hides its panel.
    friend class ::NodeModule;         // The node panel sets visibility, clipping, flags and blur.
    friend class Engine;               // DeleteShape/DeleteTransformation/DeleteDisplay erase m_engineEntry.
    friend class ::Workspace;          // Shows and hides the panels.
    friend class ::Sheet;              // Shows and hides the "progress" node.
    friend class ::Module;             // Fades the panels in and out.
    friend class ::ExportModule;       // Renders only the visible nodes.
    friend class ::NodeInterface;      // SaveState copies the attributes into a NodeState.
    friend class ::NodeTool;           // Shows the shadow panel.
    friend class ::TextureModule;      // Hides the texture selection list.
    friend class ::PDFWriter;          // Skips hidden nodes and rasterizes blurred ones.
    friend class GLDrawing;            // Textures and tints by the clipping, flags and fill (Draw).
    friend class GLEngine;             // The shader's filter texture.
    friend class ::ColorModule;        // Set the colour swatches' fill colour (SetColor).
    friend class ::SpecialColorModule; // Likewise.
    friend class ::MeshInterface;      // A clipping node limits the colour search to its shape.
    friend class ::ShadowModule;       // Likewise for its colour panel.
    friend class ::svg::SVGReader;     // A gradient-filled node clips its gradient mesh.
#ifdef PLASMA_PORTABLE
    friend unsigned int ::hideUserInterface(); // Toggles the "gui" node's visibility.
#else
    friend unsigned long ::hideUserInterface(); // Toggles the "gui" node's visibility.
#endif
    DiscreteAttribute<int> m_visibility;
    DiscreteAttribute<int> m_clipping;
    ContinuousAttribute<Vector<4, float>> m_fillColor;
    ContinuousAttribute<Vector<4, float>> m_strokeColor;
    ContinuousAttribute<float> m_blurRadius;
    int m_flags;                                  // DisplayFlags; copied by Clone.
    std::set<Node *> m_nodes;                     // The nodes that use this display.
    std::list<Display *>::iterator m_engineEntry; // This display's node in the engine's list.
};

}
