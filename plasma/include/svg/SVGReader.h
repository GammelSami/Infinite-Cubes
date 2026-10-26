// SVGReader loads an SVG file with pugixml, then walks its element tree to turn groups, gradients and shapes into
// engine nodes.
#pragma once
#include <map>
#include <string>
#include <vector>
#include "plasma/Engine.h"
#include "plasma/Vector.h"
#include "plasma/Matrix.h"
#include "../../vendor/pugixml-0.9/src/pugixml.hpp"

namespace plasma {
class Engine;
class Node;
}

namespace svg {

// Shows the reader's message box with text and caption.
void showError(const std::wstring &text, const std::wstring &caption);

typedef plasma::Vector<2, float> Point;
typedef plasma::Vector<4, float> Color;

// "#rrggbb"; any other length gives opaque white.
Color parseHexColor(const std::string &text);

// "#rrggbb" or "rgb(r,g,b)" with 0-255 components; anything else (empty, named colours) gives opaque white.
Color parseColor(const std::string &text);

// A fill or stroke value: a colour, or opaque white for "url(#...)" paint servers (the gradient is looked up
// separately).
Color parsePaintColor(const std::string &text);

// Path data: skips to the next number at pos and reads one coordinate pair (a '-' starts the next number unless it
// follows an exponent).
Point readPoint(int &pos, const std::string &data);

// Path data: skips to the next number at pos and reads it; any '-' ends it.
float readNumber(int &pos, const std::string &data);

// A transform attribute: translate(x,y), scale(x,y) and matrix(a,b,c,d,e,f) (comma or space separated), applied
// left to right; other functions are ignored.
plasma::Matrix<float> parseTransform(const std::string &text);

// The value of key in a style attribute ("key:value;key:value"), or "".
std::string getStyleValue(const std::string &style, const std::string &key);

// One gradient stop.
struct GradientStop {
    Color color;
    float offset;
};

// A <linearGradient> or <radialGradient>. The radial centre (cx, cy) and focus (fx, fy) share the storage of the
// linear start point (x1, y1).
struct Gradient {
    Gradient()
        : m_href(NULL),
          m_linear(true),
          m_start(0, 0),
          m_end(0, 0),
          m_radius(50.0f)
    {
        m_transform.identity();
    }

    std::vector<GradientStop> m_stops;
    Gradient *m_href; // xlink:href, the gradient whose stops apply.
    bool m_linear;
    Point m_start;                     // X1, y1 (cx, cy and fx, fy of a radial gradient).
    Point m_end;                       // X2, y2.
    float m_radius;                    // R.
    plasma::Matrix<float> m_transform; // GradientTransform.
};

// Style::m_lineCap values (copied into MeshShape::m_strokeCapType, "SmoothMeshShape.strokeCapType", when given).
enum {
    kLineCapUnset = -1, // No stroke-linecap: the shape keeps its own.
    kLineCapRound = 1   // Stroke-linecap="round".
};

// The inherited presentation attributes, passed down the element tree by value.
struct Style {
    Style()
        : m_strokeWidth(1.0f),
          m_gradient(NULL),
          m_fillGradient(NULL),
          m_unknown0c(0),
          m_lineCap(kLineCapUnset),
          m_fill(0.0f, 0.0f, 0.0f, 1.0f),
          m_stroke(0.0f, 0.0f, 0.0f, 1.0f),
          m_hasFill(true),
          m_hasStroke(false)
    {
        m_transform.identity();
    }

    float m_strokeWidth;
    Gradient *m_gradient;     // The gradient whose <stop> children are being read.
    Gradient *m_fillGradient; // Fill="url(#id)".
    int m_unknown0c;          // Zeroed by the constructor, never read or written by the reader.
    int m_lineCap;            // Stroke-linecap: 1 for "round", -1 when not given.
    Color m_fill;
    Color m_stroke;
    bool m_hasFill;
    bool m_hasStroke;
    plasma::Matrix<float> m_transform;
};

// The colour at point of a linear gradient from start to end: the stops' colours interpolated by the point's
// projection onto the gradient axis (red for a degenerate axis, blue when no stop interval contains it).
Color sampleGradient(const Point &start, const Point &point, const Point &end, const std::vector<GradientStop> &stops);

class SVGReader {
public:
    // Loads path; the caller then calls Read.
    SVGReader(plasma::Engine *engine, const std::wstring &path, plasma::Engine::Page *page, plasma::Node *parent);
    virtual ~SVGReader();

    // Imports the loaded document under the parent node; refuses documents with more than 10000 elements.
    void read();

private:
    // Imports node and its subtree under parent: <g> becomes a group node; <rect>, <ellipse>, <circle>, <path> and
    // <polygon> become smooth mesh shapes; gradients are collected for the url(#id) paints.
    void parseNode(const pugi::xml_node &node, plasma::Node *parent, const Style &inherited);

    // The gradient a "url(#id)" paint refers to, NULL for other paints.
    Gradient *findGradient(const std::string &paint);

    // A node with a mesh of four-point rings, one pair per stop, for a radial gradient of the given centre and radius.
    plasma::Node *makeRadialGradient(const plasma::Matrix<float> &transform, const std::vector<GradientStop> &stops,
                                     const Point &center, float radius);

    plasma::Engine *m_engine;
    plasma::Engine::Page
        *m_page; // The sheet's page settings (Workspace passes &sheet->m_page); stored, not read by this file.
    plasma::Node *m_parent;
    pugi::xml_document m_document;
    std::map<std::string, Gradient *> m_gradients; // By id.
};

}
