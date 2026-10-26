// pdf::PDFReader imports PDF page vector content into a plasma scene. The importer constructs it, calls Read, then
// destroys it.
#pragma once
#include <fstream>
#include <list>
#include <map>
#include <string>
#include <vector>
#include "plasma/Engine.h"
#include "plasma/Vector.h"
#include "plasma/Matrix4.h"
#include "plasma/Matrix.h"
#include "pdf/PDFObjects.h"

namespace plasma {
class Engine;
class Node;

}

namespace pdf {

// Shows the reader's message box with text and caption.
void showError(const std::wstring &text, const std::wstring &caption);

enum ColorSpaceFamily {
    kUnknownColorSpace = 0, // Not read, or not supported (ReadColorSpace).
    kDeviceCMYK = 1,
    kDeviceRGB = 2,
    kDeviceGray = 3,
    kDeviceN = 4 // Components mapped onto CMYK by channel index.
};

struct ColorSpace {
    ColorSpace()
        : family(kUnknownColorSpace)
    {}
    ColorSpace(ColorSpaceFamily family)
        : family(family)
    {}

    ColorSpaceFamily family;
    std::vector<char> channels; // DeviceN: CMYK index (0-3) of each component.
};

// One colour stop of a sampled shading function.
struct GradientStop {
    plasma::Vector<4, float> color;
    float offset;
};

// The /ShadingType values ReadShading supports.
enum ShadingType {
    kAxialShading = 2,
    kRadialShading = 3
};

// An axial (/ShadingType 2: /Coords [x0 y0 x1 y1]) or radial (/ShadingType 3: /Coords [x0 y0 r0 x1 y1 r1]) shading
// with the colour stops of its /Function.
struct Shading {
    Shading()
        : radial(false)
    {
        start.m_elements[0] = 0.0f;
        start.m_elements[1] = 0.0f;
        end.m_elements[0] = 0.0f;
        end.m_elements[1] = 0.0f;
        radii[0] = 0.0f;
        radii[1] = 1.0f;
        extend[0] = false;
        extend[1] = false;
    }

    std::vector<GradientStop> stops;
    plasma::Vector<2, float> start;
    plasma::Vector<2, float> end;
    float radii[2]; // Of the start and end circles.
    bool extend[2]; // /Extend: continue the end colours beyond the stops.
    bool radial;
};

// The opacities an /ExtGState entry sets (/ca and /CA).
struct ExtGState {
    ExtGState()
        : fillAlpha(1.0f),
          strokeAlpha(1.0f)
    {}

    float fillAlpha;
    float strokeAlpha;
};

// The named resources of the page or form being drawn.
struct Resources {
    Resources();

    std::map<std::string, Object *> xobjects;
    std::map<std::string, ColorSpace> colorSpaces;
    std::map<std::string, Shading> shadings;
    std::map<std::string, Shading> maskShadings;
    std::map<std::string, ExtGState> extGStates;
};

struct GraphicsState {
    GraphicsState();

    plasma::Vector<4, float> fillColor;
    plasma::Vector<4, float> strokeColor;
    plasma::Matrix<float> ctm;
    ColorSpace colorSpace; // Set by cs/CS and the colour operators.
    float lineWidth;       // W.

    // The current path: its points, a type per point (1 a corner, 2 a control point of a curve) and per subpath the
    // indices of its points.
    std::vector<plasma::Vector<2, float>> pathPoints;
    std::vector<int> pointTypes;
    std::vector<std::vector<int>> subpaths;
    plasma::Node *parentNode;
    std::string extGStateName; // The operand of the last gs.
    float fillAlpha;
    float strokeAlpha;
};

class PDFReader {
public:
    PDFReader(plasma::Engine::Page *page, plasma::Node *parentNode, plasma::Engine *engine, const std::wstring &path);
    virtual ~PDFReader();

    bool read();

private:
    void nextChar();
    void skipWhitespace();
    std::string readToken();
    Object *parseObject();
    Object *parseName();
    Object *parseString();
    Dictionary *parseDictionary();
    Array *parseArray();
    Object *resolve(Object *object);
    bool parseXref();
    void readFunction(Dictionary *function, const ColorSpace &colorSpace, std::vector<GradientStop> &stops);
    bool readShading(Dictionary *dictionary, Shading &shading);
    void readColorSpace(Object *object, ColorSpace &colorSpace);
    void loadResources(Dictionary *resources);
    plasma::Node *drawContents(Dictionary *contents, GraphicsState *parentState);
    void applyShading(const Shading &shading);
    plasma::Node *makeShadingMesh(const Shading &shading);
    plasma::Node *makePathMesh(bool fill, bool stroke, bool closed);
    ColorSpace findColorSpace(const std::string &name);
    plasma::Node *drawContentStream(std::istream &content, GraphicsState *parentState);

    plasma::Engine *m_engine;
    plasma::Engine::Page *m_page; // Its width and height are set from the MediaBox.
    plasma::Node *m_parentNode;
    std::ifstream m_file;
    char m_ch; // The lexer's current character.
    std::list<Object *> m_allObjects;
    std::map<int, std::streampos> m_xref; // Object number -> offset.
    std::map<int, Object *> m_objects;
    std::map<Object *, std::streampos> m_streamData;
    std::map<Object *, plasma::Node *> m_contentNodes; // The group node drawn for each content stream.
    int m_rootObjectNumber;
    std::list<GraphicsState> m_stateStack;
    GraphicsState m_state;
    std::map<std::string, std::string> m_unusedStringMap; // Constructed and destroyed only.
    plasma::Matrix4 m_pageMatrix;
    Resources m_resources;
};

}
