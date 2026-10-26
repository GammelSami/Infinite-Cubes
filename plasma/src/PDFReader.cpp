#include "plasma/FilePath.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include "pdf/PDFReader.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/SmoothMeshShape.h"
#include <zlib.h>

#include "main.h"

// Access helpers for members declared in other headers.
void clipToShape(plasma::Display *display); // Clipping on, 1 << DISPLAY_MODULATION.

namespace pdf {

static const double kPi = 3.141592653589793;

static const int InflateBufferBytes = 128000;
static const int MaxStartXrefSearchLines = 5;
static const int MaxImportedPdfObjects = 10000;
static const int DefaultPdfRootObjectNumber = 1;

Dictionary::Dictionary()
    : Object(kDictionary)
{}

GraphicsState::GraphicsState()
    : fillColor(1.0f, 1.0f, 1.0f, 1.0f),
      strokeColor(0.0f, 0.0f, 0.0f, 1.0f),
      colorSpace(kDeviceRGB),
      lineWidth(1.0f),
      parentNode(0),
      fillAlpha(1.0f),
      strokeAlpha(1.0f)
{
    ctm.identity();
}

// The device colour spaces are always available by name.
Resources::Resources()
{
    {
        const std::string name("/DeviceCMYK");
        colorSpaces[name] = ColorSpace(kDeviceCMYK);
    }
    {
        const std::string name("/DeviceRGB");
        colorSpaces[name] = ColorSpace(kDeviceRGB);
    }
    {
        const std::string name("/DeviceGray");
        colorSpaces[name] = ColorSpace(kDeviceGray);
    }
}

// Inflates a /FlateDecode stream into data (as zlib's zpipe example does).
static bool inflate(const std::vector<char> &compressed, std::vector<char> &decodedContentBytes)
{
    if (compressed.empty())
        return false;
    char buffer[InflateBufferBytes];
    z_stream stream;
    stream.zalloc = 0;
    stream.zfree = 0;
    stream.opaque = 0;
    stream.avail_in = 0;
    stream.next_in = 0;
    if (inflateInit_(&stream, ZLIB_VERSION, sizeof(z_stream)) != Z_OK)
        return false;
    stream.avail_in = compressed.size();
    stream.next_in = (Bytef *)&compressed[0];
    int result;
    do {
        stream.avail_out = sizeof(buffer);
        stream.next_out = (Bytef *)buffer;
        result = inflate(&stream, Z_NO_FLUSH);
        switch (result) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd(&stream);
            return false;
        }
        unsigned have = sizeof(buffer) - stream.avail_out;
        unsigned size = decodedContentBytes.size();
        decodedContentBytes.resize(size + have);
        memcpy(&decodedContentBytes[size], buffer, have);
    } while (result != Z_STREAM_END);
    inflateEnd(&stream);
    return true;
}

// The content-stream operands are kept as strings; an operator takes its operands from the end.
static float popNumber(std::list<std::string> &operands)
{
    float value = (float)atof(operands.back().c_str());
    operands.pop_back();
    return value;
}

static plasma::Vector<2, float> popPoint(std::list<std::string> &operands)
{
    plasma::Vector<2, float> point;
    point.m_elements[1] = (float)atof(operands.back().c_str());
    operands.pop_back();
    point.m_elements[0] = (float)atof(operands.back().c_str());
    operands.pop_back();
    return point;
}

static plasma::Vector<3, float> popRGB(std::list<std::string> &operands)
{
    plasma::Vector<3, float> rgb;
    rgb.m_elements[2] = (float)atof(operands.back().c_str());
    operands.pop_back();
    rgb.m_elements[1] = (float)atof(operands.back().c_str());
    operands.pop_back();
    rgb.m_elements[0] = (float)atof(operands.back().c_str());
    operands.pop_back();
    return rgb;
}

static plasma::Vector<4, float> popCMYK(std::list<std::string> &operands)
{
    plasma::Vector<4, float> cmyk;
    cmyk.m_elements[3] = (float)atof(operands.back().c_str());
    operands.pop_back();
    cmyk.m_elements[2] = (float)atof(operands.back().c_str());
    operands.pop_back();
    cmyk.m_elements[1] = (float)atof(operands.back().c_str());
    operands.pop_back();
    cmyk.m_elements[0] = (float)atof(operands.back().c_str());
    operands.pop_back();
    return cmyk;
}

// Min is the Windows macro: it evaluates the chosen operand again.
static plasma::Vector<4, float> cMYKToRGB(const plasma::Vector<4, float> &cmyk)
{
    plasma::Vector<3, float> cmy(min(1.0f, (1.0f - cmyk.m_elements[3]) * cmyk.m_elements[0] + cmyk.m_elements[3]),
                                 min(1.0f, (1.0f - cmyk.m_elements[3]) * cmyk.m_elements[1] + cmyk.m_elements[3]),
                                 min(1.0f, (1.0f - cmyk.m_elements[3]) * cmyk.m_elements[2] + cmyk.m_elements[3]));
    plasma::Vector<4, float> rgb;
    for (unsigned channel = 0; channel < 3; ++channel)
        rgb.m_elements[channel] = 1.0f - cmy.m_elements[channel];
    rgb.m_elements[3] = 1.0f;
    return rgb;
}

static plasma::Vector<4, float> rGBToRGBA(const plasma::Vector<3, float> &rgb)
{
    return plasma::Vector<4, float>(rgb.m_elements[0], rgb.m_elements[1], rgb.m_elements[2], 1.0f);
}

static plasma::Vector<4, float> grayToRGBA(float gray)
{
    return plasma::Vector<4, float>(gray, gray, gray, 1.0f);
}

// The colour of the components in colorSpace (at most four).
static plasma::Vector<4, float> toRGBA(const ColorSpace &colorSpace, const plasma::Vector<4, float> &components)
{
    switch (colorSpace.family) {
    case kDeviceCMYK:
        return cMYKToRGB(components);
    case kDeviceRGB:
        return rGBToRGBA(
            plasma::Vector<3, float>(components.m_elements[0], components.m_elements[1], components.m_elements[2]));
    case kDeviceGray:
        return grayToRGBA(components.m_elements[0]);
    case kDeviceN: {
        plasma::Vector<4, float> cmyk(0.0f, 0.0f, 0.0f, 0.0f);
        int count = min(4, (int)colorSpace.channels.size());
        for (int i = 0; i < count; ++i) {
            unsigned channel = colorSpace.channels[i];
            if (channel <= 3)
                cmyk.m_elements[channel] = components.m_elements[i];
        }
        return cMYKToRGB(cmyk);
    }
    default:
        return plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

// The operands of sc/scn and SC/SCN in colorSpace.
static plasma::Vector<4, float> popColor(const ColorSpace &colorSpace, std::list<std::string> &operands)
{
    switch (colorSpace.family) {
    case kDeviceCMYK: {
        return cMYKToRGB(popCMYK(operands));
    }
    case kDeviceRGB: {
        return rGBToRGBA(popRGB(operands));
    }
    case kDeviceGray: {
        return grayToRGBA(popNumber(operands));
    }
    case kDeviceN: {
        int count = colorSpace.channels.size();
        std::vector<float> values(count);
        for (int i = 0; i < count; ++i)
            values[count - 1 - i] = popNumber(operands);
        plasma::Vector<4, float> cmyk(0.0f, 0.0f, 0.0f, 0.0f);
        for (int i = 0; i < count; ++i) {
            unsigned channel = colorSpace.channels[i];
            if (channel <= 3)
                cmyk.m_elements[channel] = values[i];
        }
        return cMYKToRGB(cmyk);
    }
    default:
        return plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

PDFReader::PDFReader(plasma::Engine::Page *page, plasma::Node *parentNode, plasma::Engine *engine,
                     const std::wstring &path)
    : m_engine(engine),
      m_page(page),
      m_parentNode(parentNode),
      m_file(PLASMA_FILE_PATH(path), std::ios::binary)
{}

PDFReader::~PDFReader()
{
    // Object has no virtual destructor: the members of dictionaries, arrays and strings are not destroyed (undefined
    // behaviour, as in the code).
    for (std::list<Object *>::iterator it = m_allObjects.begin(); it != m_allObjects.end(); ++it)
        delete *it;
}

// Reads the line that ends just before the stream position, moving backwards, and leaves the position before that
// line's terminator.
static std::string readLineBackwards(std::istream &in)
{
    std::string line;
    char c = in.get();
    in.seekg(-2, std::ios::cur);
    while (c != '\n' && c != '\r' && in.good()) {
        line += c;
        c = in.get();
        in.seekg(-2, std::ios::cur);
    }
    while ((c == '\n' || c == '\r') && in.good()) {
        c = in.get();
        in.seekg(-2, std::ios::cur);
    }
    in.get();
    std::reverse(line.begin(), line.end());
    return line;
}

void showError(const std::wstring &text, const std::wstring &caption)
{
    MessageBoxW(g_window, text.c_str(), caption.c_str(), MB_OK);
}

bool PDFReader::read()
{
    std::string header;
    m_file >> header;
    if (header.substr(0, 5) != "%PDF-")
        return false;

    // The file ends with "startxref", the offset of the cross-reference table and "%%EOF".
    m_file.seekg(-2, std::ios::end);
    std::string nextLine;
    int i;
    for (i = 0; i < MaxStartXrefSearchLines; ++i) {
        std::string line = readLineBackwards(m_file);
        if (line == "startxref")
            break;
        nextLine = line;
    }
    if (i == 5)
        return false;

    int xrefOffset;
    std::stringstream(nextLine) >> xrefOffset;
    m_file.seekg(xrefOffset);
    m_rootObjectNumber = DefaultPdfRootObjectNumber;
    parseXref();

    int objectCount = m_xref.size();
    if (objectCount > MaxImportedPdfObjects) {
        std::wstringstream message;
        message << L"The PDF file contains too many objects (" << objectCount << "). Could not import file.";
        showError(message.str(), std::wstring(L"Picroma Plasma"));
        return false;
    }

    for (std::map<int, std::streampos>::iterator it = m_xref.begin(); it != m_xref.end(); ++it) {
        m_file.seekg(it->second);
        int objectNumber, generation;
        m_file >> objectNumber;
        m_file >> generation;
        nextChar();
        skipWhitespace();
        if (readToken() == "obj") {
            skipWhitespace();
            Object *object = parseObject();
            if (object) {
                m_objects[it->first] = object;
                if (object->type == kDictionary) {
                    m_file.seekg(-1, std::ios::cur);
                    std::string keyword;
                    m_file >> keyword;
                    if (keyword == "stream") {
                        if (m_file.get() == '\r')
                            m_file.get();
                        m_streamData[m_objects[it->first]] = m_file.tellg();
                    }
                }
            }
        }
    }

    Object *root = m_objects[m_rootObjectNumber];
    if (root && root->type == kDictionary) {
        Dictionary *pages;
        {
            const std::string key("/Pages");
            pages = asDictionary(resolve(static_cast<Dictionary *>(root)->entries[key]));
        }
        if (pages) {
            Array *kids;
            {
                const std::string key("/Kids");
                kids = asArray(resolve(pages->entries[key]));
            }
            if (kids) {
                for (i = 0; i < (int)kids->elements.size(); ++i) {
                    Dictionary *page = asDictionary(resolve(kids->elements[i]));
                    if (!page)
                        continue;
                    Dictionary *resources;
                    {
                        const std::string key("/Resources");
                        resources = asDictionary(resolve(page->entries[key]));
                    }
                    if (resources)
                        loadResources(resources);

                    m_pageMatrix.identity();
                    Array *mediaBox;
                    {
                        const std::string key("/MediaBox");
                        mediaBox = asArray(resolve(page->entries[key]));
                    }
                    if (mediaBox && mediaBox->elements.size() == 4) {
                        // The code leaves a coordinate uninitialized when its PDF object is not a number.
                        float box[4];
                        for (int k = 0; k < 4; ++k) {
                            Number *coordinate = asNumber(mediaBox->elements[k]);
                            if (coordinate)
                                box[k] = (float)coordinate->value;
                        }
                        m_page->size.m_elements[0] = box[2] - box[0];
                        m_page->size.m_elements[1] = box[3] - box[1];

                        // PDF's y axis points up.
                        m_pageMatrix.scale(1.0f, -1.0f);
                        m_pageMatrix.translate(0.0f, -box[3]);
                    }

                    Dictionary *contents;
                    {
                        const std::string key("/Contents");
                        contents = asDictionary(resolve(page->entries[key]));
                    }
                    if (contents) {
                        m_state = GraphicsState();

                        // The page matrix is the editor's Matrix4 (the same 16 floats).
                        memcpy(m_state.ctm.m_elements, m_pageMatrix.m, sizeof(m_pageMatrix.m));
                        m_state.parentNode = m_parentNode;
                        drawContents(contents, 0);
                    }
                }
            }
        }
    }
    return true;
}

static bool isInteger(const std::string &token)
{
    for (int i = 0; i < (int)token.size(); ++i) {
        if (token[i] < '0' || token[i] > '9')
            return false;
    }
    return true;
}

Object *PDFReader::parseObject()
{
    skipWhitespace();
    if (m_ch == '<')
        return parseDictionary();
    if (m_ch == '[')
        return parseArray();
    if (m_ch == '(')
        return parseString();
    if (m_ch == '/')
        return parseName();

    // A reference (n g R), a boolean or a number.
    Object *object = 0;
    std::streampos start = m_file.tellg();
    m_file.seekg(-1, std::ios::cur);
    std::string first;
    m_file >> first;
    std::string second;
    m_file >> second;
    nextChar();
    nextChar();
    if (m_ch == 'R' && isInteger(first) && isInteger(second)) {
        int objectNumber, generation;
        std::stringstream(first) >> objectNumber;
        std::stringstream(second) >> generation;
        nextChar();
        Reference *reference = new Reference;
        reference->objectNumber = objectNumber;
        object = reference;
        m_allObjects.push_back(object);
    } else {
        std::streamoff offset = (std::streamoff)start;
        --offset;
        m_file.seekg(offset);
        nextChar();
        std::string token = readToken();
        if (token == "true" || token == "false") {
            Boolean *boolean = new Boolean;
            object = boolean;
            m_allObjects.push_back(object);
            boolean->value = token == "true";
        } else {
            // The code leaves the value uninitialized if numeric extraction fails.
            double value;
            std::stringstream(token) >> value;
            Number *number = new Number;
            object = number;
            m_allObjects.push_back(object);
            number->value = value;
        }
    }
    return object;
}

Object *PDFReader::parseName()
{
    if (m_ch != '/')
        return 0;
    std::string token = readToken();
    Name *name = new Name;
    name->value = token;
    m_allObjects.push_back(name);
    return name;
}

// ( text ): escapes and nested parentheses are not handled.
Object *PDFReader::parseString()
{
    if (m_ch != '(')
        return 0;
    nextChar();
    std::string text;
    while (m_ch && m_ch != ')') {
        text += m_ch;
        nextChar();
    }
    if (m_ch != ')')
        return 0;
    nextChar();
    String *string = new String;
    string->value = text;
    m_allObjects.push_back(string);
    return string;
}

Dictionary *PDFReader::parseDictionary()
{
    if (m_ch != '<')
        return 0;
    nextChar();
    if (m_ch != '<')
        return 0;
    nextChar();
    Dictionary *dictionary = new Dictionary;
    m_allObjects.push_back(dictionary);
    skipWhitespace();
    while (m_ch != '>') {
        if (m_ch != '/')
            return 0;
        std::string key = readToken();
        skipWhitespace();
        Object *value = parseObject();
        if (value)
            dictionary->entries[key] = value;
        skipWhitespace();
    }
    nextChar();
    if (m_ch != '>')
        return 0;
    nextChar();
    return dictionary;
}

Array *PDFReader::parseArray()
{
    if (m_ch != '[')
        return 0;
    nextChar();
    skipWhitespace();
    Array *array = new Array;
    m_allObjects.push_back(array);
    while (m_ch != ']') {
        array->elements.push_back(parseObject());
        skipWhitespace();
    }
    nextChar();
    return array;
}

static bool isWhitespace(char c)
{
    return c > 0 && c <= ' ';
}

static bool isDelimiter(char c)
{
    return c == '/' || c == '[' || c == ']' || c == '<' || c == '>' || c == '(' || c == ')';
}

// Reads from the current character up to whitespace or a delimiter.
std::string PDFReader::readToken()
{
    std::string token;
    token += m_ch;
    nextChar();
    while (m_ch) {
        if (isWhitespace(m_ch) || isDelimiter(m_ch))
            break;
        token += m_ch;
        nextChar();
    }
    return token;
}

void PDFReader::skipWhitespace()
{
    while (isWhitespace(m_ch))
        nextChar();
}

// End of file reads as character 0.
void PDFReader::nextChar()
{
    int c = m_file.get();
    m_ch = c > 0 ? c : 0;
}

Object *PDFReader::resolve(Object *object)
{
    if (object && object->type == kReference)
        return resolve(m_objects[static_cast<Reference *>(object)->objectNumber]);
    return object;
}

// Reads a cross-reference section and its trailer, then the sections the trailer's /Prev chain names.
bool PDFReader::parseXref()
{
    std::string keyword;
    m_file >> keyword;
    if (keyword != "xref")
        return false;
    int first, count;
    m_file >> first;
    m_file >> count;
    for (int i = 0; i < count; ++i) {
        if (!m_file.good())
            return false;
        unsigned long offset;
        int generation;
        m_file >> offset;
        if (!m_file.good())
            return false;
        m_file >> generation;
        if (!m_file.good())
            return false;
        std::string use;
        m_file >> use;
        if (!m_file.good())
            return false;
        if (use == "f")
            continue;
        m_xref[first + i] = offset;
    }

    m_file >> keyword;
    if (keyword == "trailer") {
        nextChar();
        skipWhitespace();
        Dictionary *trailer = parseDictionary();
        if (trailer) {
            Reference *root;
            {
                const std::string key("/Root");
                root = asReference(trailer->entries[key]);
            }
            if (root)
                m_rootObjectNumber = root->objectNumber;
            Number *previous;
            {
                const std::string key("/Prev");
                previous = asNumber(resolve(trailer->entries[key]));
            }
            if (previous) {
                m_file.seekg((std::streamoff)previous->value);
                return parseXref();
            }
        }
    }
    return true;
}

// Reads the colour stops of a sampled exponential (/FunctionType 2: /C0 and /C1) or stitching (/FunctionType 3:
// /Functions with /Encode) function.
void PDFReader::readFunction(Dictionary *function, const ColorSpace &colorSpace, std::vector<GradientStop> &stops)
{
    Number *type;
    {
        const std::string key("/FunctionType");
        type = asNumber(resolve(function->entries[key]));
    }
    if (!type)
        return;
    if (type->value == 2.0) {
        Array *c0;
        {
            const std::string key("/C0");
            c0 = asArray(resolve(function->entries[key]));
        }
        Array *c1;
        {
            const std::string key("/C1");
            c1 = asArray(resolve(function->entries[key]));
        }
        if (c0 && c1 && c0->elements.size() <= 4 && c1->elements.size() <= 4) {
            plasma::Vector<4, float> color0(0.0f, 0.0f, 0.0f, 1.0f);
            plasma::Vector<4, float> color1(0.0f, 0.0f, 0.0f, 1.0f);
            for (int i = 0; i < (int)c0->elements.size(); ++i) {
                Number *component = asNumber(c0->elements[i]);
                if (component)
                    color0[i] = (float)component->value;
            }
            for (int i = 0; i < (int)c1->elements.size(); ++i) {
                Number *component = asNumber(c1->elements[i]);
                if (component)
                    color1[i] = (float)component->value;
            }
            color0 = toRGBA(colorSpace, color0);
            color1 = toRGBA(colorSpace, color1);
            GradientStop stop;
            stop.color = color0;
            stop.offset = 0.0f;
            stops.push_back(stop);
            stop.color = color1;
            stop.offset = 0.0f;
            stops.push_back(stop);
        }
    } else if (type->value == 3.0) {
        float encodeStart = 0.0f;
        float encodeEnd = 1.0f;
        Array *encode;
        {
            const std::string key("/Encode");
            encode = asArray(resolve(function->entries[key]));
        }
        if (encode && encode->elements.size() == 2) {
            Number *value = asNumber(encode->elements[0]);
            if (value)
                encodeStart = (float)value->value;
            value = asNumber(encode->elements[1]);
            if (value)
                encodeEnd = (float)value->value;
        }
        Array *functions;
        {
            const std::string key("/Functions");
            functions = asArray(resolve(function->entries[key]));
        }
        if (functions) {
            for (int i = 0; i < (int)functions->elements.size(); ++i) {
                std::vector<GradientStop> part;
                readFunction(asDictionary(resolve(functions->elements[i])), colorSpace, part);
                if (encodeEnd < encodeStart)
                    std::reverse(part.begin(), part.end());
                if (part.size()) {
                    // The parts share their end stops.
                    if (i == 0)
                        stops.push_back(part[0]);
                    for (int j = 1; j < (int)part.size(); ++j)
                        stops.push_back(part[j]);
                }
            }
        }
    }
}

bool PDFReader::readShading(Dictionary *dictionary, Shading &shading)
{
    Number *type;
    {
        const std::string key("/ShadingType");
        type = asNumber(resolve(dictionary->entries[key]));
    }
    if (!type)
        return false;
    Object *colorSpaceObject;
    {
        const std::string key("/ColorSpace");
        colorSpaceObject = resolve(dictionary->entries[key]);
    }
    if (!colorSpaceObject)
        return false;
    ColorSpace colorSpace;
    if (colorSpaceObject->type == kName)
        colorSpace = m_resources.colorSpaces[static_cast<Name *>(colorSpaceObject)->value];
    else
        readColorSpace(colorSpaceObject, colorSpace);
    Dictionary *function;
    {
        const std::string key("/Function");
        function = asDictionary(resolve(dictionary->entries[key]));
    }
    if (!function)
        return false;
    readFunction(function, colorSpace, shading.stops);

    int shadingType = (int)type->value;
    if (shadingType == kAxialShading) {
        shading.radial = false;
        Array *coords;
        {
            const std::string key("/Coords");
            coords = asArray(resolve(dictionary->entries[key]));
        }
        if (!coords || coords->elements.size() != 4)
            return false;
        float coordinates[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        for (int i = 0; i < 4; ++i) {
            Number *value = asNumber(coords->elements[i]);
            if (value)
                coordinates[i] = (float)value->value;
        }
        shading.start = plasma::Vector<2, float>(coordinates[0], coordinates[1]);
        shading.end = plasma::Vector<2, float>(coordinates[2], coordinates[3]);
        Array *extend;
        {
            const std::string key("/Extend");
            extend = asArray(resolve(dictionary->entries[key]));
        }
        if (extend && extend->elements.size() == 2) {
            Boolean *value = asBoolean(extend->elements[0]);
            if (value)
                shading.extend[0] = value->value;
            value = asBoolean(extend->elements[1]);
            if (value)
                shading.extend[1] = value->value;
        }
    } else if (shadingType == kRadialShading) {
        shading.radial = true;
        Array *coords;
        {
            const std::string key("/Coords");
            coords = asArray(resolve(dictionary->entries[key]));
        }
        if (!coords || coords->elements.size() != 6)
            return false;
        float coordinates[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        for (int i = 0; i < 6; ++i) {
            Number *value = asNumber(coords->elements[i]);
            if (value)
                coordinates[i] = (float)value->value;
        }
        shading.start = plasma::Vector<2, float>(coordinates[0], coordinates[1]);
        shading.radii[0] = coordinates[2];
        shading.end = plasma::Vector<2, float>(coordinates[3], coordinates[4]);
        shading.radii[1] = coordinates[5];
        Array *extend;
        {
            const std::string key("/Extend");
            extend = asArray(resolve(dictionary->entries[key]));
        }
        if (extend && extend->elements.size() == 2) {
            Boolean *value = asBoolean(extend->elements[0]);
            if (value)
                shading.extend[0] = value->value;
            value = asBoolean(extend->elements[1]);
            if (value)
                shading.extend[1] = value->value;
        }
    }
    return true;
}

// [/ICCBased stream] (by its /N components) or [/DeviceN [names]...] (the names /Cyan, /Magenta, /Yellow and
// /Black mapped onto CMYK).
void PDFReader::readColorSpace(Object *object, ColorSpace &colorSpace)
{
    colorSpace.family = kUnknownColorSpace;
    Object *resolved = resolve(object);
    if (!resolved || resolved->type != kArray)
        return;
    Array *array = static_cast<Array *>(resolved);
    if (array->elements.size() <= 1)
        return;
    Object *familyObject = array->elements[0];
    if (!familyObject || familyObject->type != kName)
        return;
    Dictionary *stream = asDictionary(resolve(array->elements[1]));
    if (static_cast<Name *>(familyObject)->value == "/ICCBased" && stream) {
        Number *components;
        {
            const std::string key("/N");
            components = asNumber(resolve(stream->entries[key]));
        }
        if (components) {
            if (components->value == 3.0)
                colorSpace.family = kDeviceRGB;
            else if (components->value == 4.0)
                colorSpace.family = kDeviceCMYK;
        }
    } else if (static_cast<Name *>(familyObject)->value == "/DeviceN") {
        Array *names = asArray(resolve(array->elements[1]));
        if (names && !names->elements.empty()) {
            colorSpace.channels.resize(names->elements.size(), 0);
            colorSpace.family = kDeviceN;
            for (int i = 0; i < (int)names->elements.size(); ++i) {
                Name *name = asName(resolve(names->elements[i]));
                if (name) {
                    if (name->value == "/Cyan")
                        colorSpace.channels[i] = 0;
                    if (name->value == "/Magenta")
                        colorSpace.channels[i] = 1;
                    if (name->value == "/Yellow")
                        colorSpace.channels[i] = 2;
                    if (name->value == "/Black")
                        colorSpace.channels[i] = 3;
                }
            }
        }
    }
}

// Replaces the resources by those of a page or form dictionary's /Resources.
void PDFReader::loadResources(Dictionary *resources)
{
    m_resources = Resources();

    Dictionary *colorSpaces;
    {
        const std::string key("/ColorSpace");
        colorSpaces = asDictionary(resolve(resources->entries[key]));
    }
    if (colorSpaces) {
        for (std::map<std::string, Object *>::iterator it = colorSpaces->entries.begin();
             it != colorSpaces->entries.end(); ++it) {
            ColorSpace colorSpace;
            readColorSpace(it->second, colorSpace);
            m_resources.colorSpaces[it->first] = colorSpace;
        }
    }

    Dictionary *xobjects;
    {
        const std::string key("/XObject");
        xobjects = asDictionary(resolve(resources->entries[key]));
    }
    if (xobjects) {
        for (std::map<std::string, Object *>::iterator it = xobjects->entries.begin(); it != xobjects->entries.end();
             ++it)
            m_resources.xobjects[it->first] = resolve(it->second);
    }

    Dictionary *shadings;
    {
        const std::string key("/Shading");
        shadings = asDictionary(resolve(resources->entries[key]));
    }
    if (shadings) {
        for (std::map<std::string, Object *>::iterator it = shadings->entries.begin(); it != shadings->entries.end();
             ++it) {
            Dictionary *dictionary = asDictionary(resolve(it->second));
            if (dictionary) {
                Shading shading;
                if (readShading(dictionary, shading))
                    m_resources.shadings[it->first] = shading;
            }
        }
    }

    Dictionary *extGStates;
    {
        const std::string key("/ExtGState");
        extGStates = asDictionary(resolve(resources->entries[key]));
    }
    if (extGStates) {
        for (std::map<std::string, Object *>::iterator it = extGStates->entries.begin();
             it != extGStates->entries.end(); ++it) {
            Dictionary *state = asDictionary(resolve(it->second));
            if (!state)
                continue;
            ExtGState values;
            Number *alpha;
            {
                const std::string key("/ca");
                alpha = asNumber(resolve(state->entries[key]));
            }
            if (alpha)
                values.fillAlpha = (float)alpha->value;
            {
                const std::string key("/CA");
                alpha = asNumber(resolve(state->entries[key]));
            }
            if (alpha)
                values.strokeAlpha = (float)alpha->value;
            m_resources.extGStates[it->first] = values;

            // A luminosity soft mask: the first shading of its group gives the opacity of the shading drawn under this state.
            Dictionary *mask;
            {
                const std::string key("/SMask");
                mask = asDictionary(resolve(state->entries[key]));
            }
            if (!mask)
                continue;
            Name *subtype;
            {
                const std::string key("/S");
                subtype = asName(resolve(mask->entries[key]));
            }
            if (!subtype || subtype->value != "/Luminosity")
                continue;
            Dictionary *group;
            {
                const std::string key("/G");
                group = asDictionary(resolve(mask->entries[key]));
            }
            if (!group)
                continue;
            Dictionary *groupResources;
            {
                const std::string key("/Resources");
                groupResources = asDictionary(resolve(group->entries[key]));
            }
            if (!groupResources)
                continue;
            Dictionary *groupShadings;
            {
                const std::string key("/Shading");
                groupShadings = asDictionary(resolve(groupResources->entries[key]));
            }
            if (groupShadings && groupShadings->entries.size()) {
                Shading shading;
                Dictionary *dictionary = asDictionary(resolve(groupShadings->entries.begin()->second));
                if (dictionary && readShading(dictionary, shading))
                    m_resources.maskShadings[it->first] = shading;
            }
        }
    }
}

// Draws a page's or a form XObject's content stream into a new group node.
plasma::Node *PDFReader::drawContents(Dictionary *contents, GraphicsState *parentState)
{
    plasma::Node *node = 0;
    Dictionary *resources;
    {
        const std::string key("/Resources");
        resources = asDictionary(resolve(contents->entries[key]));
    }
    Resources savedResources = m_resources;
    GraphicsState savedState = m_state;
    if (resources)
        loadResources(resources);

    Array *matrix;
    {
        const std::string key("/Matrix");
        matrix = asArray(resolve(contents->entries[key]));
    }
    if (matrix && matrix->elements.size() == 6) {
        float a = 1.0f, b = 0.0f, c = 0.0f, d = 1.0f, e = 0.0f, f = 0.0f;
        Number *value = asNumber(matrix->elements[0]);
        if (value)
            a = (float)value->value;
        value = asNumber(matrix->elements[1]);
        if (value)
            b = (float)value->value;
        value = asNumber(matrix->elements[2]);
        if (value)
            c = (float)value->value;
        value = asNumber(matrix->elements[3]);
        if (value)
            d = (float)value->value;
        value = asNumber(matrix->elements[4]);
        if (value)
            e = (float)value->value;
        value = asNumber(matrix->elements[5]);
        if (value)
            f = (float)value->value;
        plasma::Matrix<float> form;
        form.m_elements[2] = 0.0f;
        form.m_elements[3] = 0.0f;
        form.m_elements[6] = 0.0f;
        form.m_elements[7] = 0.0f;
        form.m_elements[8] = 0.0f;
        form.m_elements[9] = 0.0f;
        form.m_elements[11] = 0.0f;
        form.m_elements[14] = 0.0f;
        form.m_elements[10] = 1.0f;
        form.m_elements[15] = 1.0f;
        form.m_elements[0] = a;
        form.m_elements[1] = b;
        form.m_elements[4] = c;
        form.m_elements[5] = d;
        form.m_elements[12] = e;
        form.m_elements[13] = f;
        m_state.ctm *= form;
    }

    std::streamoff offset = m_streamData[contents];
    Number *length;
    {
        const std::string key("/Length");
        length = asNumber(resolve(contents->entries[key]));
    }
    if (offset && length && length->value > 0.0) {
        Name *filter;
        {
            const std::string key("/Filter");
            filter = asName(resolve(contents->entries[key]));
        }
        if (filter) {
            if (filter->value == "/FlateDecode") {
                std::vector<char> compressed;
                compressed.resize((unsigned)length->value);
                m_file.seekg(offset);
                m_file.read(&compressed[0], (int)length->value);
                std::vector<char> decodedContentBytes;
                if (inflate(compressed, decodedContentBytes) && decodedContentBytes.size()) {
                    // The code reads this unterminated byte buffer as a C string.
                    std::stringstream content((std::string(&decodedContentBytes[0])));
                    node = drawContentStream(content, parentState);
                }
            }
        } else {
            std::vector<char> contentBytes;
            contentBytes.resize((unsigned)length->value);
            m_file.seekg(offset);
            m_file.read(&contentBytes[0], (int)length->value);

            // The code reads this unterminated byte buffer as a C string.
            std::stringstream content((std::string(&contentBytes[0])));
            node = drawContentStream(content, parentState);
        }
    }
    m_contentNodes[contents] = node;
    m_resources = savedResources;
    m_state = savedState;
    return node;
}

// Sh with an axial shading: colours the vertices of the current node's mesh by their position along the axis.
void PDFReader::applyShading(const Shading &shading)
{
    if (shading.stops.size() < 2)
        return;
    plasma::Node *node = m_state.parentNode;
    if (!node)
        return;
    plasma::MeshShape *mesh = static_cast<plasma::MeshShape *>(node->m_shape);
    if (!mesh || mesh->getType() != plasma::SHAPE_SMOOTH_MESH)
        return;

    std::vector<GradientStop> stops = shading.stops;
    if (m_state.extGStateName.size() &&
        m_resources.maskShadings.find(m_state.extGStateName) != m_resources.maskShadings.end()) {
        const Shading &mask = m_resources.maskShadings[m_state.extGStateName];
        if (mask.radial == shading.radial && mask.stops.size() == shading.stops.size()) {
            for (int i = 0; i < (int)stops.size(); ++i)
                stops[i].color.m_elements[3] = mask.stops[i].color.m_elements[0];
        }
    }
    int count = stops.size();
    for (int i = 0; i < count; ++i)
        stops[i].offset = (float)(i / ((unsigned)count - 1.0));

    plasma::Matrix<float> inverse = m_state.ctm;
    inverse.invert();
    if (shading.radial)
        return;
    plasma::Vector<2, float> axis = shading.end - shading.start;
    float lengthSquared = axis.m_elements[1] * axis.m_elements[1] + axis.m_elements[0] * axis.m_elements[0];
    if (lengthSquared < 1.0e-15f)
        return;
    axis.m_elements[0] = axis.m_elements[0] / lengthSquared;
    axis.m_elements[1] = axis.m_elements[1] / lengthSquared;
    std::vector<plasma::Vector<2, float>> &positions = mesh->m_vertexPositions.value();
    for (int i = 0; i < (int)positions.size(); ++i) {
        plasma::Vector<2, float> offset = inverse.transformPoint(mesh->m_vertexPositions.value()[i]) - shading.start;
        float t = offset.m_elements[0] * axis.m_elements[0];
        t = offset.m_elements[1] * axis.m_elements[1] + t;
        if (shading.extend[0] && t < stops[0].offset) {
            mesh->m_vertexColors.value()[i] = stops[0].color;
        } else if (shading.extend[1] && stops[count - 1].offset <= t) {
            mesh->m_vertexColors.value()[i] = stops[count - 1].color;
        } else {
            for (int j = 0; j < count - 1; ++j) {
                if (stops[j].offset <= t && t <= stops[j + 1].offset) {
                    float f = (t - stops[j].offset) / (stops[j + 1].offset - stops[j].offset);
                    mesh->m_vertexColors.value()[i] = stops[j].color * (1.0f - f) + stops[j + 1].color * f;
                    break;
                }
            }
        }
    }
    mesh->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
}

// Sh with a radial shading (and the axial case): a new mesh of the shading's rings or bands, fitted to the current
// node's bounds. Layout positions, extents and spacing use display units; colour tuples style the overlays.
// Gradient extension stops extend to -1 and 2 outside the normalized interval, retaining the endpoint colors.
plasma::Node *PDFReader::makeShadingMesh(const Shading &shading)
{
    if (shading.stops.size() < 2)
        return 0;
    plasma::Vector<2, float> minimum = m_state.parentNode->getMinimum(0);
    plasma::Vector<2, float> maximum = m_state.parentNode->getMaximum(0);
    std::vector<GradientStop> stops = shading.stops;
    if (m_state.extGStateName.size() &&
        m_resources.maskShadings.find(m_state.extGStateName) != m_resources.maskShadings.end()) {
        const Shading &mask = m_resources.maskShadings[m_state.extGStateName];
        if (mask.radial == shading.radial && mask.stops.size() == shading.stops.size()) {
            for (int i = 0; i < (int)stops.size(); ++i)
                stops[i].color.m_elements[3] = mask.stops[i].color.m_elements[0];
        }
    }
    for (int i = 0; i < (int)stops.size(); ++i)
        stops[i].offset = (float)(i / (stops.size() - 1.0));

    plasma::SmoothMeshShape *mesh = m_engine->createSmoothMeshShape();
    if (shading.radial) {
        mesh->setName(std::wstring(L"Radial gradient"));
        if (shading.extend[1]) {
            stops.insert(stops.end(), stops.back());
            stops.back().offset = 2.0f;
        }

        // Two rings of four points per stop.
        for (int i = 0; i < (int)stops.size(); ++i) {
            float t = stops[i].offset;
            float radius = shading.radii[1] * t + shading.radii[0] * (1.0f - t);
            plasma::Vector<2, float> center = shading.start * (1.0f - t) + shading.end * t;
            float extent = radius * 1.6f;
            for (int ring = 0; ring < 2; ++ring) {
                for (int k = 0; k < 4; ++k) {
                    float angle = (float)(k * 0.25 * kPi * 2.0 + kPi / 4.0);
                    plasma::Vector<2, float> direction(cos(angle), sin(angle));
                    plasma::Vector<2, float> point = center;
                    point += direction * extent;
                    mesh->m_vertexPositions.append(m_state.ctm.transform(point));
                    mesh->m_vertexColors.append(stops[i].color);
                    mesh->m_vertexFlags.push_back(plasma::VERTEX_SMOOTH);
                }
            }
        }
        std::vector<std::vector<int>> faces;
        faces.resize(4 * (stops.size() - 1));
        for (int i = 0; i < (int)stops.size() - 1; ++i) {
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
    } else {
        mesh->setName(std::wstring(L"Linear gradient"));
        plasma::Vector<2, float> start = m_state.ctm.transform(shading.start);
        plasma::Vector<2, float> end = m_state.ctm.transform(shading.end);
        plasma::Vector<2, float> axis = end - start;
        float length = axis.m_elements[0] * axis.m_elements[0] + axis.m_elements[1] * axis.m_elements[1];
        if (length > 0.0f) {
            length = 1.0f / std::sqrt(length);
            axis.m_elements[0] = length * axis.m_elements[0];
            axis.m_elements[1] = length * axis.m_elements[1];
        }
        plasma::Vector<2, float> normal(axis.m_elements[1], -axis.m_elements[0]);

        // The bands reach across the node's bounds.
        plasma::Vector<2, float> center = (minimum + maximum) * 0.5;
        plasma::Vector<2, float> corner = minimum - center;
        float halfWidth =
            std::sqrt(corner.m_elements[0] * corner.m_elements[0] + corner.m_elements[1] * corner.m_elements[1]);
        plasma::Vector<2, float> across = normal * halfWidth;
        plasma::Vector<2, float> relative = start - center;
        float distance = relative.m_elements[0] * axis.m_elements[0];
        distance = relative.m_elements[1] * axis.m_elements[1] + distance;
        start = axis * distance + center;
        relative = end - center;
        distance = relative.m_elements[0] * axis.m_elements[0];
        distance = relative.m_elements[1] * axis.m_elements[1] + distance;
        end = center + axis * distance;
        if (shading.extend[0]) {
            stops.insert(stops.begin(), stops.front());
            stops.front().offset = -1.0f;
        }
        if (shading.extend[1]) {
            stops.insert(stops.end(), stops.back());
            stops.back().offset = 2.0f;
        }
        for (int i = 0; i < (int)stops.size(); ++i) {
            mesh->m_vertexColors.append(stops[i].color);
            mesh->m_vertexColors.append(stops[i].color);
            mesh->m_vertexFlags.push_back(plasma::VERTEX_SMOOTH);
            mesh->m_vertexFlags.push_back(plasma::VERTEX_SMOOTH);
            float t = stops[i].offset;
            mesh->m_vertexPositions.append(start * (1.0f - t) + end * t - across);
            mesh->m_vertexPositions.append(start * (1.0f - t) + end * t + across);
        }
        std::vector<std::vector<int>> faces;
        faces.resize(stops.size() - 1);
        int vertex = 3;
        for (int i = 0; i < (int)stops.size() - 1; ++i) {
            faces[i].resize(4);
            faces[i][0] = vertex - 3;
            faces[i][1] = vertex - 2;
            faces[i][2] = vertex;
            faces[i][3] = vertex - 1;
            vertex += 2;
        }
        mesh->setSubdivisions(0, false);
        mesh->setFaces(faces, true);
        mesh->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
    return m_engine->createNode(0, mesh, 0, m_state.parentNode, mesh->getName());
}

// F, S, b and the clip of sh: a mesh of the current path, transformed by the current matrix, with one face per
// subpath.
plasma::Node *PDFReader::makePathMesh(bool fill, bool stroke, bool closed)
{
    if (m_state.pathPoints.size() < 2)
        return 0;
    plasma::SmoothMeshShape *mesh = m_engine->createSmoothMeshShape(std::wstring(L"Mesh"));
    mesh->m_vertexPositions.resize(m_state.pathPoints.size());
    mesh->m_vertexColors.resize(m_state.pathPoints.size());
    mesh->m_vertexFlags.resize(m_state.pathPoints.size());
    mesh->m_strokeWidths.resize(m_state.pathPoints.size());
    mesh->m_strokeColors.resize(m_state.pathPoints.size());
    for (int i = 0; i < (int)m_state.pathPoints.size(); ++i) {
        mesh->m_vertexPositions.value()[i] = m_state.ctm * m_state.pathPoints[i];
        mesh->m_vertexColors.value()[i] = m_state.fillColor;
        mesh->m_vertexColors.value()[i].m_elements[3] = m_state.fillAlpha;
        mesh->m_strokeColors.value()[i] = m_state.strokeColor;
        mesh->m_strokeColors.value()[i].m_elements[3] = m_state.strokeAlpha;
        mesh->m_vertexFlags[i] = m_state.pointTypes[i];
        mesh->m_strokeWidths.value()[i] = m_state.lineWidth;
    }
    mesh->m_flags |= (1 << plasma::MESH_CONTOUR);
    if (!fill)
        mesh->m_flags |= (1 << plasma::MESH_NO_FILL);
    else
        mesh->m_flags &= ~(1 << plasma::MESH_NO_FILL);
    if (stroke)
        mesh->m_flags |= (1 << plasma::MESH_STROKE);
    else
        mesh->m_flags &= ~(1 << plasma::MESH_STROKE);
    if (!closed)
        mesh->m_flags |= (1 << plasma::MESH_OPEN);
    else
        mesh->m_flags &= ~(1 << plasma::MESH_OPEN);

    // Fewer subdivisions for longer subpaths.
    int longest = 0;
    for (int i = 0; i < (int)m_state.subpaths.size(); ++i) {
        if ((int)m_state.subpaths[i].size() > longest)
            longest = m_state.subpaths[i].size();
    }
    float ratio = 5000.0f / longest;
    int subdivisions = (int)(log(ratio) / log(2.0f));
    if (subdivisions > plasma::MaxImportedPathSubdivisions)
        subdivisions = plasma::MaxImportedPathSubdivisions;
    else if (subdivisions < 0)
        subdivisions = 0;
    mesh->setSubdivisions(subdivisions, false);
    mesh->setFaces(m_state.subpaths, true);
    mesh->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    return m_engine->createNode(0, mesh, 0, m_state.parentNode, mesh->getName());
}

ColorSpace PDFReader::findColorSpace(const std::string &name)
{
    std::map<std::string, ColorSpace>::iterator it = m_resources.colorSpaces.find(name);
    if (it == m_resources.colorSpaces.end())
        return ColorSpace();
    return it->second;
}

// Interprets a content stream: the path, colour, state and XObject operators (the others are ignored and their
// operands stay on the stack). Returns the group node of what it drew, or 0 when nothing was drawn.
plasma::Node *PDFReader::drawContentStream(std::istream &content, GraphicsState *parentState)
{
    std::list<std::string> operands;
    plasma::Node *group = m_engine->createNode(0, 0, 0, m_state.parentNode, std::wstring(L"Group"));
    GraphicsState savedState = m_state;
    m_state.fillAlpha = 1.0f;
    m_state.strokeAlpha = 1.0f;
    m_state.parentNode = group;

    // W n: the clip path, applied by the next sh.
    std::vector<std::vector<int>> clipSubpaths;
    std::vector<plasma::Vector<2, float>> clipPoints;
    std::vector<int> clipPointTypes;
    plasma::Matrix<float> clipMatrix = m_state.ctm;
    bool clipping = false;
    while (content.good()) {
        std::string token;
        content >> token;
        operands.push_back(token);
        if (token == "Do") {
            operands.pop_back();
            Object *xobject = m_resources.xobjects[operands.back()];
            if (xobject && xobject->type == kDictionary) {
                GraphicsState state = m_state;
                drawContents(static_cast<Dictionary *>(xobject), &state);
                m_state = state;
                m_state.fillAlpha = 1.0f;
                m_state.strokeAlpha = 1.0f;
            }
        }
        if (token == "gs") {
            operands.pop_back();
            m_state.extGStateName = operands.back();
            m_state.fillAlpha = m_resources.extGStates[m_state.extGStateName].fillAlpha;
            m_state.strokeAlpha = m_resources.extGStates[m_state.extGStateName].strokeAlpha;
            if (parentState)
                m_state.fillAlpha = parentState->fillAlpha * m_state.fillAlpha;
        }
        if (token == "sh") {
            operands.pop_back();
            if (clipping) {
                plasma::Matrix<float> ctm = m_state.ctm;
                m_state.fillColor = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
                m_state.pathPoints = clipPoints;
                m_state.pointTypes = clipPointTypes;
                m_state.subpaths = clipSubpaths;
                m_state.ctm = clipMatrix;
                clipping = false;
                plasma::Node *clip = makePathMesh(true, false, true);
                if (clip)
                    m_state.parentNode = clip;
                m_state.pathPoints.clear();
                m_state.pointTypes.clear();
                m_state.subpaths.clear();
                clipPoints.clear();
                clipPointTypes.clear();
                clipSubpaths.clear();
                m_state.ctm = ctm;
            }
            if (m_state.parentNode->m_shape) {
                const Shading &shading = m_resources.shadings[operands.back()];
                if (shading.radial) {
                    clipToShape(m_state.parentNode->m_display);
                    makeShadingMesh(shading);
                } else {
                    applyShading(shading);
                }
            }
        }
        if (token == "cs") {
            operands.pop_back();
            m_state.colorSpace = findColorSpace(operands.back());
            m_state.fillColor = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
        }
        if (token == "CS") {
            operands.pop_back();
            m_state.colorSpace = findColorSpace(operands.back());
            m_state.strokeColor = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
        }
        if (token == "q")
            m_stateStack.push_back(m_state);
        if (token == "Q" && m_stateStack.size()) {
            m_state = m_stateStack.back();
            m_stateStack.pop_back();
        }
        if (token == "rg") {
            operands.pop_back();
            m_state.fillColor = rGBToRGBA(popRGB(operands));
            m_state.colorSpace = ColorSpace(kDeviceRGB);
        }
        if (token == "RG") {
            operands.pop_back();
            m_state.strokeColor = rGBToRGBA(popRGB(operands));
            m_state.colorSpace = ColorSpace(kDeviceRGB);
        }
        if (token == "k") {
            operands.pop_back();
            m_state.fillColor = cMYKToRGB(popCMYK(operands));
            m_state.colorSpace = ColorSpace(kDeviceCMYK);
        }
        if (token == "K") {
            operands.pop_back();
            m_state.strokeColor = cMYKToRGB(popCMYK(operands));
            m_state.colorSpace = ColorSpace(kDeviceCMYK);
        }
        if (token == "g") {
            operands.pop_back();
            m_state.fillColor = grayToRGBA(popNumber(operands));
            m_state.colorSpace = ColorSpace(kDeviceGray);
        }
        if (token == "G") {
            operands.pop_back();
            m_state.strokeColor = grayToRGBA(popNumber(operands));
            m_state.colorSpace = ColorSpace(kDeviceGray);
        }
        if (token == "sc" || token == "scn") {
            operands.pop_back();
            m_state.fillColor = popColor(m_state.colorSpace, operands);
        }
        if (token == "SC" || token == "SCN") {
            operands.pop_back();
            m_state.strokeColor = popColor(m_state.colorSpace, operands);
        }
        if (token == "w") {
            operands.pop_back();
            m_state.lineWidth = popNumber(operands);
        }
        if (token == "cm") {
            operands.pop_back();
            float f = popNumber(operands);
            float e = popNumber(operands);
            float d = popNumber(operands);
            float c = popNumber(operands);
            float b = popNumber(operands);
            float a = popNumber(operands);
            plasma::Matrix<float> matrix;
            matrix.m_elements[2] = 0.0f;
            matrix.m_elements[3] = 0.0f;
            matrix.m_elements[6] = 0.0f;
            matrix.m_elements[7] = 0.0f;
            matrix.m_elements[8] = 0.0f;
            matrix.m_elements[9] = 0.0f;
            matrix.m_elements[10] = 1.0f;
            matrix.m_elements[15] = 1.0f;
            matrix.m_elements[11] = 0.0f;
            matrix.m_elements[14] = 0.0f;
            matrix.m_elements[0] = a;
            matrix.m_elements[1] = b;
            matrix.m_elements[4] = c;
            matrix.m_elements[5] = d;
            matrix.m_elements[12] = e;
            matrix.m_elements[13] = f;
            m_state.ctm *= matrix;
        }
        if (token == "m") {
            operands.pop_back();
            m_state.subpaths.push_back(std::vector<int>());
            plasma::Vector<2, float> point = popPoint(operands);
            m_state.subpaths.back().push_back(m_state.pathPoints.size());
            m_state.pathPoints.push_back(point);
            m_state.pointTypes.push_back(plasma::VERTEX_SHARP);
        }
        if (token == "re") {
            operands.pop_back();
            m_state.subpaths.push_back(std::vector<int>());
            plasma::Vector<2, float> size = popPoint(operands);
            plasma::Vector<2, float> origin = popPoint(operands);
            m_state.subpaths.back().push_back(m_state.pathPoints.size());
            m_state.pathPoints.push_back(origin);
            m_state.pointTypes.push_back(plasma::VERTEX_SHARP);
            m_state.subpaths.back().push_back(m_state.pathPoints.size());
            m_state.pathPoints.push_back(origin + plasma::Vector<2, float>(size.m_elements[0], 0.0f));
            m_state.pointTypes.push_back(plasma::VERTEX_SHARP);
            m_state.subpaths.back().push_back(m_state.pathPoints.size());
            m_state.pathPoints.push_back(origin + size);
            m_state.pointTypes.push_back(plasma::VERTEX_SHARP);
            m_state.subpaths.back().push_back(m_state.pathPoints.size());
            m_state.pathPoints.push_back(origin + plasma::Vector<2, float>(0.0f, size.m_elements[1]));
            m_state.pointTypes.push_back(plasma::VERTEX_SHARP);
        }

        // The path operators need a subpath begun by m or re.
        if (m_state.subpaths.size()) {
            if (token == "c") {
                operands.pop_back();
                plasma::Vector<2, float> end = popPoint(operands);
                plasma::Vector<2, float> control2 = popPoint(operands);
                plasma::Vector<2, float> control1 = popPoint(operands);
                m_state.subpaths.back().push_back(m_state.pathPoints.size());
                m_state.pathPoints.push_back(control1);
                m_state.pointTypes.push_back(plasma::VERTEX_BEZIER);
                m_state.subpaths.back().push_back(m_state.pathPoints.size());
                m_state.pathPoints.push_back(control2);
                m_state.pointTypes.push_back(plasma::VERTEX_BEZIER);
                m_state.subpaths.back().push_back(m_state.pathPoints.size());
                m_state.pathPoints.push_back(end);
                m_state.pointTypes.push_back(plasma::VERTEX_SHARP);
            }
            if (token == "v" || token == "y") {
                operands.pop_back();
                plasma::Vector<2, float> end = popPoint(operands);
                plasma::Vector<2, float> control = popPoint(operands);
                m_state.subpaths.back().push_back(m_state.pathPoints.size());
                m_state.pathPoints.push_back(control);
                m_state.pointTypes.push_back(plasma::VERTEX_BEZIER);
                m_state.subpaths.back().push_back(m_state.pathPoints.size());
                m_state.pathPoints.push_back(end);
                m_state.pointTypes.push_back(plasma::VERTEX_SHARP);
            }
            if (token == "l") {
                operands.pop_back();
                plasma::Vector<2, float> point = popPoint(operands);
                m_state.subpaths.back().push_back(m_state.pathPoints.size());
                m_state.pathPoints.push_back(point);
                m_state.pointTypes.push_back(plasma::VERTEX_SHARP);
            }
            if (token == "S") {
                operands.pop_back();
                makePathMesh(false, true, false);
                m_state.pathPoints.clear();
                m_state.pointTypes.clear();
                m_state.subpaths.clear();
                clipping = false;
            }
            if (token == "s") {
                operands.pop_back();
                makePathMesh(false, true, true);
                m_state.pathPoints.clear();
                m_state.pointTypes.clear();
                m_state.subpaths.clear();
                clipping = false;
            }
            if (token == "f" || token == "F" || token == "f*") {
                operands.pop_back();
                makePathMesh(true, false, true);
                m_state.pathPoints.clear();
                m_state.pointTypes.clear();
                m_state.subpaths.clear();
                clipping = false;
            }
            if (token == "b" || token == "B" || token == "b*" || token == "B*") {
                operands.pop_back();
                makePathMesh(true, true, true);
                m_state.pathPoints.clear();
                m_state.pointTypes.clear();
                m_state.subpaths.clear();
            }
            if (token == "h") {
                operands.pop_back();
                m_state.subpaths.push_back(std::vector<int>());
            }
            if (token == "n") {
                clipPoints = m_state.pathPoints;
                clipPointTypes = m_state.pointTypes;
                clipSubpaths = m_state.subpaths;
                clipMatrix = m_state.ctm;
                m_state.pathPoints.clear();
                m_state.pointTypes.clear();
                m_state.subpaths.clear();
            }
            if (token == "W") {
                operands.pop_back();
                clipping = true;
            }
        }
    }
    m_state = savedState;
    if (group->m_children.size())
        return group;
    m_engine->destroyNode(group);
    return 0;
}

}
