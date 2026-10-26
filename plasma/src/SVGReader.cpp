// SVG import (namespace svg); see svg/SVGReader.h.
#include "svg/SVGReader.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>

#include "main.h"
#include "plasma/Display.h"
#include "plasma/Node.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/Transformation.h"

std::wstring widenString(const std::string &text);

namespace svg {

static const int MaximumImportedNodeCount = 10000;
static const int RoundedShapeSubdivisions = 4;

SVGReader::SVGReader(plasma::Engine *engine, const std::wstring &path, plasma::Engine::Page *page, plasma::Node *parent)
    : m_engine(engine),
      m_page(page),
      m_parent(parent)
{
    m_document.load_file(narrowString(path).c_str());
}

SVGReader::~SVGReader()
{
    for (std::map<std::string, Gradient *>::iterator it = m_gradients.begin(); it != m_gradients.end(); ++it)
        delete it->second;
}

// The element count of the subtree at node, node included.
static int countNodes(const pugi::xml_node &node)
{
    int count = 1;
    for (pugi::xml_node_iterator it = node.begin(); it != node.end(); it++)
        count += countNodes(*it);
    return count;
}

void showError(const std::wstring &text, const std::wstring &caption)
{
    MessageBoxW(g_window, text.c_str(), caption.c_str(), MB_OK);
}

void SVGReader::read()
{
    int nodeCount = countNodes(m_document.root()) - 1;
    if (nodeCount > MaximumImportedNodeCount) {
        std::wstringstream message;
        message << L"The SVG file contains too many nodes (" << nodeCount << "). Could not import file.";
        showError(message.str(), std::wstring(L"Picroma Plasma"));
        return;
    }
    parseNode(m_document.root(), m_parent, Style());
}

std::string getStyleValue(const std::string &style, const std::string &key)
{
    int pos = style.find(key + ":");
    if (pos < 0)
        return "";
    unsigned int start = style.find(":", pos) + 1;
    int end = style.find(";", pos);
    if (end < 0)
        end = style.size();
    return style.substr(start, end - start);
}

Color parseHexColor(const std::string &text)
{
    if (text.size() != 7)
        return Color(1.0f, 1.0f, 1.0f, 1.0f);
    int red = strtol(text.substr(1, 2).c_str(), NULL, 16);
    int green = strtol(text.substr(3, 2).c_str(), NULL, 16);
    int blue = strtol(text.substr(5, 2).c_str(), NULL, 16);
    return Color((float)(red / 255.0), (float)(green / 255.0), (float)(blue / 255.0), 1.0f);
}

Color parseColor(const std::string &text)
{
    if (text.size() < 1)
        return Color(1.0f, 1.0f, 1.0f, 1.0f);
    if (text[0] == '#')
        return parseHexColor(text);
    if (text.find("rgb") == 0) {
        // The code ignores failed conversions and leaves missing components uninitialised.
        int red, green, blue;
        sscanf(text.substr(text.find("(")).c_str(), "(%d,%d,%d)", &red, &green, &blue);
        return Color((float)(red / 255.0), (float)(green / 255.0), (float)(blue / 255.0), 1.0f);
    }
    return Color(1.0f, 1.0f, 1.0f, 1.0f);
}

// True for the characters a path-data number is made of.
static inline bool isNumberChar(unsigned char c)
{
    return (c >= '0' && c <= '9') || c == '.' || c == '-' || c == 'e' || c == 'E';
}

Point readPoint(int &pos, const std::string &data)
{
    // The number scans assume valid input and do not check the end of the string.
    while (!isNumberChar(data[pos]))
        pos++;
    int xStart = pos;
    pos++;
    while (isNumberChar(data[pos])) {
        if (data[pos] == '-' && data[pos - 1] != 'e' && data[pos - 1] != 'E')
            break;
        pos++;
    }
    int xEnd = pos;

    while (!isNumberChar(data[pos]))
        pos++;
    int yStart = pos;
    pos++;
    while (isNumberChar(data[pos])) {
        if (data[pos] == '-' && data[pos - 1] != 'e' && data[pos - 1] != 'E')
            break;
        pos++;
    }

    const std::string &y = data.substr(yStart, pos - yStart);
    const std::string &x = data.substr(xStart, xEnd - xStart);
    const char *yText = y.c_str();
    const char *xText = x.c_str();
    float xValue = (float)atof(xText);
    float yValue = (float)atof(yText);
    return Point(xValue, yValue);
}

float readNumber(int &pos, const std::string &data)
{
    // The number scans assume valid input and do not check the end of the string.
    while (!isNumberChar(data[pos]))
        pos++;
    int start = pos;
    pos++;
    while (isNumberChar(data[pos])) {
        if (data[pos] == '-')
            break;
        pos++;
    }
    return (float)atof(data.substr(start, pos - start).c_str());
}

// A command letter in path or points data (not an exponent).
static inline bool isCommandChar(unsigned char c)
{
    return c >= 'A' && c <= 'z' && c != 'e' && c != 'E';
}

// The "d" attribute of a <path>: appends the points (absolute) and their types (2 for a curve's control points, 1
// otherwise) and starts a contour of point indices at each moveto. A command's arguments may repeat. Arcs are not
// supported; z only ends the command.
static void parsePath(const std::string &data, std::vector<Point> &points, std::vector<std::vector<int>> &contours,
                      std::vector<int> &types)
{
    Point current(0.0f, 0.0f);
    int last = data.size() - 1;
    int pos = 0;
    if (last < 0)
        return;
    do {
        char command = data[pos];
        int count = 0;
        int type = plasma::VERTEX_SMOOTH;
        bool relative = false;
        bool horizontal = false;
        bool vertical = false;
        switch (command) {
        case 'm':
            relative = true;
        case 'M':
            contours.push_back(std::vector<int>());
            type = plasma::VERTEX_SHARP;
            count = 1;
            break;
        case 'l':
            relative = true;
        case 'L':
            type = plasma::VERTEX_SHARP;
            count = 1;
            break;
        case 'h':
            relative = true;
        case 'H':
            type = plasma::VERTEX_SHARP;
            horizontal = true;
            count = 1;
            break;
        case 'v':
            relative = true;
        case 'V':
            type = plasma::VERTEX_SHARP;
            vertical = true;
            count = 1;
            break;
        case 'c':
            relative = true;
        case 'C':
            type = plasma::VERTEX_BEZIER;
            count = 3;
            break;
        case 's':
            relative = true;
        case 'S':

            // The first control point mirrors the previous curve's second.
            if (contours.back().size() >= 2) {
                Point control = points[contours.back()[contours.back().size() - 2]];
                Point end = points[contours.back()[contours.back().size() - 1]];
                Point reflected = end + end - control;
                contours.back().push_back(points.size());
                points.push_back(reflected);
                types.push_back(plasma::VERTEX_BEZIER);
            }
            type = plasma::VERTEX_BEZIER;
            count = 2;
            break;
        case 'z':
            relative = true;
            break;
        }
        pos++;
        Point start = current;
        if (count <= 0)
            continue;

        // The repetitions are counted but the count is not used.
        int repeats = 0;
        for (;;) {
            repeats++;
            start = current;
            for (int i = 0; i < count; i++) {
                contours.back().push_back(points.size());
                if (horizontal) {
                    current.m_elements[0] = readNumber(pos, data);
                    if (relative)
                        current.m_elements[0] += start.m_elements[0];
                } else if (vertical) {
                    current.m_elements[1] = readNumber(pos, data);
                    if (relative)
                        current.m_elements[1] += start.m_elements[1];
                } else {
                    current = readPoint(pos, data);
                    if (relative) {
                        current.m_elements[0] += start.m_elements[0];
                        current.m_elements[1] += start.m_elements[1];
                    }
                }
                points.push_back(current);
                if (i == count - 1)
                    types.push_back(plasma::VERTEX_SHARP);
                else
                    types.push_back(type);
            }
            while (pos <= last && !isNumberChar(data[pos]) && !isCommandChar(data[pos]))
                pos++;
            if (pos > last)
                break;
            if (isCommandChar(data[pos]))
                break;
        }
    } while (pos <= last);
}

Gradient *SVGReader::findGradient(const std::string &paint)
{
    if (paint.find("url") != 0)
        return NULL;
    unsigned int start = paint.find("#") + 1;
    unsigned int end = paint.find(")");
    std::string id = paint.substr(start, end - start);
    return m_gradients[id];
}

plasma::Matrix<float> parseTransform(const std::string &text)
{
    plasma::Matrix<float> matrix;
    matrix.identity();
    int length = text.size();
    int end;
    for (int start = 0; start < length; start = end) {
        end = text.find("(", start + 1);
        if (end > start) {
            std::string name = text.substr(start, end - start);
            int open = end;
            end = text.find(")", open + 1);
            std::string arguments = text.substr(open + 1, end - open - 1);
            if (name == "translate") {
                float translateX, translateY;

                // The parsing leaves missing coordinates uninitialised.
                sscanf(arguments.c_str(), "%g,%g", &translateX, &translateY);
                matrix.m_elements[12] =
                    matrix.m_elements[0] * translateX + matrix.m_elements[4] * translateY + matrix.m_elements[12];
                matrix.m_elements[13] =
                    matrix.m_elements[1] * translateX + matrix.m_elements[5] * translateY + matrix.m_elements[13];
                matrix.m_elements[14] =
                    matrix.m_elements[2] * translateX + matrix.m_elements[6] * translateY + matrix.m_elements[14];
                matrix.m_elements[15] =
                    matrix.m_elements[3] * translateX + matrix.m_elements[7] * translateY + matrix.m_elements[15];
            }
            if (name == "scale") {
                float scaleX, scaleY;

                // The parsing leaves missing coordinates uninitialised.
                sscanf(arguments.c_str(), "%g,%g", &scaleX, &scaleY);
                matrix.scale(scaleX, scaleY);
            }
            if (name == "matrix") {
                float values[6];

                // A failed fallback scan also leaves missing components uninitialised.
                if (sscanf(arguments.c_str(), "%f,%f,%f,%f,%f,%f", &values[0], &values[1], &values[2], &values[3],
                           &values[4], &values[5]) != 6)
                    sscanf(arguments.c_str(), "%f %f %f %f %f %f", &values[0], &values[1], &values[2], &values[3],
                           &values[4], &values[5]);
                matrix.identity();
                matrix.m_elements[0] = values[0];
                matrix.m_elements[1] = values[1];
                matrix.m_elements[4] = values[2];
                matrix.m_elements[5] = values[3];
                matrix.m_elements[12] = values[4];
                matrix.m_elements[13] = values[5];
            }
            end++;
        }
        if (end < 0)
            break;
    }
    return matrix;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Gradient palette: red
// signals a degenerate axis, blue an unmatched range; unspecified gradient stops start opaque gray.
Color sampleGradient(const Point &start, const Point &point, const Point &end, const std::vector<GradientStop> &stops)
{
    Point axis = end - start;
    float lengthSquared = axis.m_elements[0] * axis.m_elements[0] + axis.m_elements[1] * axis.m_elements[1];
    if (lengthSquared < 1e-12f)
        return Color(1.0f, 0.0f, 0.0f, 1.0f);
    for (unsigned int i = 0; i < 2; ++i)
        axis.m_elements[i] /= lengthSquared;
    Point offset = point - start;

    // Round the first component product before accumulating the second.
    float t = offset[0] * axis[0];
    for (unsigned int component = 1; component < 2; ++component)
        t += offset[component] * axis[component];
    if (t < stops.front().offset)
        return stops.front().color;
    if (t >= stops.back().offset)
        return stops.back().color;
    int last = stops.size() - 1;
    for (int i = 0; i < last; i++) {
        if (stops[i].offset <= t && stops[i + 1].offset >= t) {
            float f = (t - stops[i].offset) / (stops[i + 1].offset - stops[i].offset);
            return stops[i].color * (1.0f - f) + stops[i + 1].color * f;
        }
    }
    return Color(0.0f, 0.0f, 1.0f, 1.0f);
}

static const double kPi = 3.141592653589793;

plasma::Node *SVGReader::makeRadialGradient(const plasma::Matrix<float> &transform,
                                            const std::vector<GradientStop> &stops, const Point &center, float radius)
{
    plasma::SmoothMeshShape *mesh = m_engine->createSmoothMeshShape();
    mesh->setName(std::wstring(L"Radial gradient"));

    // The last stop's colour continues to twice the radius.
    std::vector<GradientStop> rings = stops;
    rings.push_back(rings.back());
    rings.back().offset += 1.0f;

    // Two rings of four points per stop.
    int count = rings.size();
    for (int i = 0; i < count; ++i) {
        float t = rings[i].offset;
        float extent = t * radius;
        extent *= (double)1.6f;
        for (int ring = 0; ring < 2; ++ring) {
            for (int k = 0; k < 4; ++k) {
                float angle = (float)(k * 0.25 * kPi * 2.0 + kPi / 4.0);
                Point direction((float)cos((double)angle), (float)sin((double)angle));
                Point scaledDirection = direction * extent;
                Point point = center + scaledDirection;
                const float wx = transform.m_elements[3];
                const float wy = transform.m_elements[7];
                const float w0 = transform.m_elements[15];
                float w = point.m_elements[1] * wy + point.m_elements[0] * wx + w0;
                const float yx = transform.m_elements[1];
                const float yy = transform.m_elements[5];
                const float y0 = transform.m_elements[13];
                const float xx = transform.m_elements[0];
                const float xy = transform.m_elements[4];
                const float x0 = transform.m_elements[12];
                Point transformed(point.m_elements[1] * xy + point.m_elements[0] * xx + x0,
                                  point.m_elements[1] * yy + point.m_elements[0] * yx + y0);
                mesh->m_vertexPositions.append(transformed * (1.0f / w));
                mesh->m_vertexColors.append(rings[i].color);
                mesh->m_vertexFlags.push_back(plasma::VERTEX_SMOOTH);
            }
        }
    }
    std::vector<std::vector<int>> faces;
    faces.resize(4 * (count - 1));
    for (int i = 0; i < count - 1; ++i) {
        for (int k = 0; k < 4; ++k) {
            std::vector<int> &face = faces[i * 4 + k];
            face.resize(4);
            face[0] = k + 8 * i + 4;
            face[1] = k + 8 * i + 8;
            face[2] = (k + 1) % 4 + 8 * i + 8;
            face[3] = (k + 1) % 4 + 8 * i + 4;
        }
    }
    mesh->setSubdivisions(plasma::ImportedRadialGradientSubdivisions, false);
    mesh->setFaces(faces, true);
    mesh->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    return m_engine->createNode(0, mesh, 0, 0, mesh->getName());
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Gradient palette: red
// signals a degenerate axis, blue an unmatched range; unspecified gradient stops start opaque gray.
void SVGReader::parseNode(const pugi::xml_node &node, plasma::Node *parent, const Style &inherited)
{
    std::string name = node.name();
    Style style = inherited;
    plasma::Node *created = NULL;
    if (name == "g") {
        plasma::Node *group = m_engine->createNode(0, 0, 0, parent, L"");
        created = group;
        parent = group;
        std::string id = node.attribute("id").value();
        if (id.empty())
            group->setName(std::wstring(L"Group"));
        else
            group->setName(widenString(id));
    }

    std::string transform = node.attribute("transform").value();
    plasma::Matrix<float> matrix;
    matrix.identity();
    if (transform.size()) {
        matrix = parseTransform(transform);
        style.m_transform *= matrix;
    }

    if (name == "linearGradient" || name == "radialGradient") {
        std::string id = node.attribute("id").value();
        if (id.size()) {
            Gradient *gradient = new Gradient;
            style.m_gradient = gradient;
            gradient->m_linear = name == "linearGradient";
            if (m_gradients[id])
                delete m_gradients[id];
            m_gradients[id] = gradient;
            std::string href = node.attribute("xlink:href").value();
            if (href.size())
                gradient->m_href = m_gradients[href.substr(1, href.size() - 1)];
            if (node.attribute("x1"))
                gradient->m_start.m_elements[0] = node.attribute("x1").as_float();
            if (node.attribute("y1"))
                gradient->m_start.m_elements[1] = node.attribute("y1").as_float();
            if (node.attribute("x2"))
                gradient->m_end.m_elements[0] = node.attribute("x2").as_float();
            if (node.attribute("y2"))
                gradient->m_end.m_elements[1] = node.attribute("y2").as_float();
            if (node.attribute("cx"))
                gradient->m_start.m_elements[0] = node.attribute("cx").as_float();
            if (node.attribute("cy"))
                gradient->m_start.m_elements[1] = node.attribute("cy").as_float();
            if (node.attribute("fx"))
                gradient->m_start.m_elements[0] = node.attribute("fx").as_float();
            if (node.attribute("fy"))
                gradient->m_start.m_elements[1] = node.attribute("fy").as_float();
            if (node.attribute("r"))
                gradient->m_radius = node.attribute("r").as_float();
            if (node.attribute("gradientTransform")) {
                std::string value = node.attribute("gradientTransform").value();
                gradient->m_transform = parseTransform(value);
            }
        }
    }

    if (node.attribute("stroke-width"))
        style.m_strokeWidth = node.attribute("stroke-width").as_float();
    if (node.attribute("stroke-linecap")) {
        if (!strcmp(node.attribute("stroke-linecap").value(), "round"))
            style.m_lineCap = kLineCapRound;
    }
    if (node.attribute("fill")) {
        std::string fill = node.attribute("fill").value();
        if (fill == "none") {
            style.m_hasFill = false;
        } else {
            Gradient *gradient = findGradient(fill);
            if (gradient)
                style.m_fillGradient = gradient;
            style.m_fill = parsePaintColor(fill);
            style.m_hasFill = true;
        }
    }
    if (node.attribute("stroke")) {
        std::string stroke = node.attribute("stroke").value();
        if (stroke == "none") {
            style.m_hasStroke = false;
        } else {
            style.m_hasStroke = true;
            style.m_stroke = parsePaintColor(stroke);
        }
    }

    if (name == "stop" && style.m_gradient) {
        GradientStop stop;
        stop.color = Color(0.5f, 0.5f, 0.5f, 1.0f);
        std::string value = node.attribute("style").value();
        if (value.size()) {
            std::string color = getStyleValue(value, "stop-color");
            if (color.size())
                stop.color = parseColor(color);
            std::string opacity = getStyleValue(value, "stop-opacity");
            if (opacity.size())
                stop.color.m_elements[3] = (float)atof(opacity.c_str());
        }
        stop.offset = node.attribute("offset").as_float();
        style.m_gradient->m_stops.push_back(stop);
    }

    if (node.attribute("opacity"))
        style.m_fill.m_elements[3] = node.attribute("opacity").as_float();

    // Style="key:value;..." overrides the presentation attributes.
    for (pugi::xml_attribute_iterator it = node.attributes_begin(); it != node.attributes_end(); it++) {
        if (strcmp((*it).name(), "style"))
            continue;
        std::string fill = getStyleValue((*it).value(), "fill");
        if (fill.size() && fill != "none") {
            Gradient *gradient = findGradient(fill);
            if (gradient)
                style.m_fillGradient = gradient;
            style.m_fill = parsePaintColor(fill);
            style.m_hasFill = true;
        } else {
            style.m_hasFill = fill != "none";
        }
        std::string fillOpacity = getStyleValue((*it).value(), "fill-opacity");
        if (fillOpacity.size())
            style.m_fill.m_elements[3] *= (float)atof(fillOpacity.c_str());
        std::string opacity = getStyleValue((*it).value(), "opacity");
        if (opacity.size())
            style.m_fill.m_elements[3] *= (float)atof(opacity.c_str());
        std::string stroke = getStyleValue((*it).value(), "stroke");
        if (!stroke.size() || stroke == "none") {
            style.m_hasStroke = false;
        } else {
            style.m_stroke = parsePaintColor(stroke);
            style.m_hasStroke = true;
        }
        std::string strokeWidth = getStyleValue((*it).value(), "stroke-width");
        if (strokeWidth.size())
            style.m_strokeWidth = (float)atof(strokeWidth.c_str());
    }

    std::wstring nodeName = L"Node";
    if (node.attribute("id")) {
        std::string id = node.attribute("id").value();
        nodeName = widenString(id);
    }

    plasma::SmoothMeshShape *shape = NULL;
    if (name == "rect") {
        float x = node.attribute("x").as_float();
        float y = node.attribute("y").as_float();
        float width = node.attribute("width").as_float();
        float height = node.attribute("height").as_float();
        float rx = node.attribute("rx").as_float();
        float ry = node.attribute("ry").as_float();
        shape = m_engine->createSmoothMeshShape(nodeName);
        int subdivisions;

        // The code requires both radii to be ordered and positive for rounded corners.
        if (!(rx > 0.0f && ry > 0.0f)) {
            shape->m_vertexPositions.resize(4);
            shape->m_vertexFlags.resize(4, plasma::VERTEX_SHARP);
            shape->m_vertexPositions.value()[0] = Point(x, y);
            shape->m_vertexPositions.value()[1] = Point(x + width, y);
            shape->m_vertexPositions.value()[2] = Point(x + width, y + height);
            shape->m_vertexPositions.value()[3] = Point(x, y + height);
            subdivisions = 0;
        } else {
            // Each corner is a control point between two edge points.
            shape->m_vertexPositions.resize(12);
            shape->m_vertexFlags.resize(12, plasma::VERTEX_SHARP);
            shape->m_vertexColors.resize(12);
            shape->m_strokeWidths.resize(12);
            shape->m_strokeColors.resize(12);
            shape->m_vertexFlags[0] = plasma::VERTEX_BEZIER;
            shape->m_vertexFlags[3] = plasma::VERTEX_BEZIER;
            shape->m_vertexFlags[6] = plasma::VERTEX_BEZIER;
            shape->m_vertexFlags[9] = plasma::VERTEX_BEZIER;
            shape->m_vertexPositions.value()[0] = Point(x, y);
            shape->m_vertexPositions.value()[1] = Point(x + rx, y);
            shape->m_vertexPositions.value()[2] = Point(x + width - rx, y);
            shape->m_vertexPositions.value()[3] = Point(x + width, y);
            shape->m_vertexPositions.value()[4] = Point(x + width, y + ry);
            shape->m_vertexPositions.value()[5] = Point(x + width, y + height - ry);
            shape->m_vertexPositions.value()[6] = Point(x + width, y + height);
            shape->m_vertexPositions.value()[7] = Point(x + width - rx, y + height);
            shape->m_vertexPositions.value()[8] = Point(x + rx, y + height);
            shape->m_vertexPositions.value()[9] = Point(x, y + height);
            shape->m_vertexPositions.value()[10] = Point(x, y + height - ry);
            shape->m_vertexPositions.value()[11] = Point(x, y + ry);
            subdivisions = RoundedShapeSubdivisions;
        }
        shape->setSubdivisions(subdivisions, false);
        if (style.m_hasStroke)
            shape->m_flags |= (1 << plasma::MESH_STROKE);
        else
            shape->m_flags &= ~(1 << plasma::MESH_STROKE);
        shape->m_flags |= (1 << plasma::MESH_CONTOUR);
        if (style.m_hasFill)
            shape->m_flags &= ~(1 << plasma::MESH_NO_FILL);
        else
            shape->m_flags |= (1 << plasma::MESH_NO_FILL);
        if (style.m_hasStroke)
            shape->m_flags |= (1 << plasma::MESH_STROKE);
        else
            shape->m_flags &= ~(1 << plasma::MESH_STROKE);
        std::vector<std::vector<int>> faces;
        faces.resize(1);
        faces[0].resize(shape->m_vertexPositions.m_values[0]->size());
        for (int i = 0; i < (int)shape->m_vertexPositions.m_values[0]->size(); i++)
            faces[0][i] = i;
        shape->setFaces(faces, true);
        created = m_engine->createNode(0, shape, 0, parent, nodeName);
    }

    if (name == "ellipse" || name == "circle") {
        float cx = node.attribute("cx").as_float();
        float cy = node.attribute("cy").as_float();
        float rx = node.attribute(name == "circle" ? "r" : "rx").as_float();
        float ry = node.attribute(name == "circle" ? "r" : "ry").as_float();
        shape = m_engine->createSmoothMeshShape(nodeName);
        shape->m_vertexPositions.resize(4);
        shape->m_vertexFlags.resize(4, plasma::VERTEX_SMOOTH);
        shape->m_vertexPositions.value()[0] = Point(cx - rx, cy - ry);
        shape->m_vertexPositions.value()[1] = Point(cx + rx, cy - ry);
        shape->m_vertexPositions.value()[2] = Point(cx + rx, cy + ry);
        shape->m_vertexPositions.value()[3] = Point(cx - rx, cy + ry);
        shape->setSubdivisions(4, false);
        if (style.m_hasStroke)
            shape->m_flags |= (1 << plasma::MESH_STROKE);
        else
            shape->m_flags &= ~(1 << plasma::MESH_STROKE);
        shape->m_flags |= (1 << plasma::MESH_CONTOUR);
        if (style.m_hasFill)
            shape->m_flags &= ~(1 << plasma::MESH_NO_FILL);
        else
            shape->m_flags |= (1 << plasma::MESH_NO_FILL);
        if (style.m_hasStroke)
            shape->m_flags |= (1 << plasma::MESH_STROKE);
        else
            shape->m_flags &= ~(1 << plasma::MESH_STROKE);
        std::vector<std::vector<int>> faces;
        faces.resize(1);
        faces[0].resize(shape->m_vertexPositions.m_values[0]->size());
        for (int i = 0; i < (int)shape->m_vertexPositions.m_values[0]->size(); i++)
            faces[0][i] = i;
        shape->setFaces(faces, true);

        // The subdivided square of the corners shrinks towards the centre; 1.12 brings it close to the ellipse.
        Point center(cx, cy);
        for (int i = 0; i < (int)shape->m_vertexPositions.m_values[0]->size(); i++)
            shape->m_vertexPositions.value()[i] = center + (shape->m_vertexPositions.value()[i] - center) * 1.12f;
        shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        created = m_engine->createNode(0, shape, 0, parent, nodeName);
    }

    if (name == "path") {
        std::vector<Point> points;
        std::vector<int> types;
        std::vector<std::vector<int>> contours;
        for (pugi::xml_attribute_iterator it = node.attributes_begin(); it != node.attributes_end(); it++) {
            if (!strcmp((*it).name(), "d")) {
                std::string data = (*it).value();
                parsePath(data, points, contours, types);
            }
        }
        if (points.size()) {
            shape = m_engine->createSmoothMeshShape(nodeName);
            shape->m_vertexPositions.resize(points.size());
            shape->m_vertexFlags.resize(points.size());

            // Fewer subdivisions for longer contours.
            int longest = 0;
            for (int i = 0; i < (int)contours.size(); i++) {
                if ((int)contours[i].size() > longest)
                    longest = contours[i].size();
            }
            float ratio = 5000.0f / longest;
            int subdivisions = (int)(log(ratio) / log(2.0f));
            if (subdivisions > plasma::MaxImportedPathSubdivisions)
                subdivisions = plasma::MaxImportedPathSubdivisions;
            else if (subdivisions < 0)
                subdivisions = 0;
            shape->setSubdivisions(subdivisions, false);
            shape->m_flags |= (1 << plasma::MESH_CONTOUR);
            if (style.m_hasFill)
                shape->m_flags &= ~(1 << plasma::MESH_NO_FILL);
            else
                shape->m_flags |= (1 << plasma::MESH_NO_FILL);
            if (style.m_hasStroke)
                shape->m_flags |= (1 << plasma::MESH_STROKE);
            else
                shape->m_flags &= ~(1 << plasma::MESH_STROKE);

            // An unfilled stroke is an open line with the given caps.
            if (!style.m_hasFill && style.m_hasStroke) {
                shape->m_flags |= (1 << plasma::MESH_OPEN);
                if (style.m_lineCap >= 0)
                    shape->m_strokeCapType = style.m_lineCap;
            }
            for (int i = 0; i < (int)points.size(); i++) {
                shape->m_vertexPositions.value()[i] = points[i];
                shape->m_vertexFlags[i] = types[i];
            }
            shape->setFaces(contours, false);
            created = m_engine->createNode(0, shape, 0, parent, nodeName);
        }
    }

    if (name == "polygon") {
        std::vector<Point> points;
        for (pugi::xml_attribute_iterator it = node.attributes_begin(); it != node.attributes_end(); it++) {
            if (!strcmp((*it).name(), "points")) {
                std::string value = (*it).value();
                int pos = 0;
                int length = value.size();
                while (pos < length) {
                    points.push_back(readPoint(pos, value));
                    while (pos < length && !isNumberChar(value[pos]))
                        pos++;
                }
            }
        }
        if (points.size()) {
            std::vector<int> types(points.size(), 1);
            std::vector<std::vector<int>> faces;
            faces.resize(1);
            faces[0].resize(points.size());
            for (int i = 0; i < (int)points.size(); i++)
                faces[0][i] = i;
            shape = m_engine->createSmoothMeshShape(nodeName);
            shape->m_vertexPositions.resize(points.size());
            shape->m_vertexFlags.resize(points.size());
            shape->setSubdivisions(0, false);
            shape->m_flags |= (1 << plasma::MESH_CONTOUR);
            if (style.m_hasFill)
                shape->m_flags &= ~(1 << plasma::MESH_NO_FILL);
            else
                shape->m_flags |= (1 << plasma::MESH_NO_FILL);
            if (style.m_hasStroke)
                shape->m_flags |= (1 << plasma::MESH_STROKE);
            else
                shape->m_flags &= ~(1 << plasma::MESH_STROKE);

            // An unfilled stroke is an open line with the given caps.
            if (!style.m_hasFill && style.m_hasStroke) {
                shape->m_flags |= (1 << plasma::MESH_OPEN);
                if (style.m_lineCap >= 0)
                    shape->m_strokeCapType = style.m_lineCap;
            }
            for (int i = 0; i < (int)points.size(); i++) {
                shape->m_vertexPositions.value()[i] = points[i];
                shape->m_vertexFlags[i] = types[i];
            }
            shape->setFaces(faces, false);
            created = m_engine->createNode(0, shape, 0, parent, nodeName);
        }
    }

    if (created) {
        created->m_transformation->m_deformation.value() *= matrix;
        created->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }

    if (shape) {
        int count = shape->m_vertexPositions.m_values[0]->size();
        shape->m_vertexColors.resize(count);
        shape->m_strokeColors.resize(shape->m_vertexPositions.m_values[0]->size());
        shape->m_strokeWidths.resize(shape->m_vertexPositions.m_values[0]->size());
        shape->m_vertexTexCoords.resize(shape->m_vertexPositions.m_values[0]->size());
        plasma::Matrix<float> textureMatrix;
        textureMatrix.identity();
        for (int i = 0; i < (int)shape->m_vertexPositions.m_values[0]->size(); i++) {
            shape->m_vertexTexCoords.value()[i] = textureMatrix.transform(shape->m_vertexPositions.value()[i]);
            shape->m_vertexColors.value()[i] = style.m_fill;
            shape->m_strokeWidths.value()[i] = style.m_strokeWidth;
            shape->m_strokeColors.value()[i] = style.m_stroke;
        }

        Gradient *gradient = style.m_fillGradient;
        if (gradient && plasma::SmoothMeshShape::hasFill(shape->m_flags)) {
            Gradient *source = gradient->m_href ? gradient->m_href : gradient;
            if (source->m_stops.size() > 1) {
                if (gradient->m_linear) {
                    matrix = gradient->m_transform;
                    matrix.invert();
                    for (int i = 0; i < (int)shape->m_vertexPositions.m_values[0]->size(); i++) {
                        Point point = matrix.transform(shape->m_vertexPositions.value()[i]);
                        shape->m_vertexColors.value()[i] =
                            sampleGradient(gradient->m_start, point, gradient->m_end, source->m_stops);
                    }
                } else if (created) {
                    // The stroke moves to a copy of the node: the radial gradient's mesh is clipped by the shape.
                    if (plasma::SmoothMeshShape::hasStroke(shape->m_flags)) {
                        plasma::Node *outline = created->clone(created->m_parent);
                        plasma::Shape *copy = outline->m_shape;
                        if (copy && copy->getType() == plasma::SHAPE_SMOOTH_MESH)
                            static_cast<plasma::MeshShape *>(copy)->m_flags |= (1 << plasma::MESH_NO_FILL);
                        shape->m_flags &= ~(1 << plasma::MESH_STROKE);
                    }
                    plasma::Node *fill = makeRadialGradient(gradient->m_transform, source->m_stops, gradient->m_start,
                                                            gradient->m_radius);
                    created->m_display->m_clipping.value() = 1;
                    created->m_display->m_flags |= (1 << plasma::DISPLAY_MODULATION);
                    created->addChild(fill);
                    for (int i = 0; i < (int)shape->m_vertexPositions.m_values[0]->size(); i++)
                        shape->m_vertexColors.value()[i] = Color(1.0f, 1.0f, 1.0f, 1.0f);
                }
            }
        }
        shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }

    for (pugi::xml_node_iterator it = node.begin(); it != node.end(); it++)
        parseNode(*it, parent, style);
}

Color parsePaintColor(const std::string &text)
{
    if (text.find("url") == 0)
        return Color(1.0f, 1.0f, 1.0f, 1.0f);
    return parseColor(text);
}

}
