// The PLX scene reader. Objects are numbered as their chunks are encountered; nodes' child references are resolved
// after the whole stream has been read.
#pragma once
#include <istream>
#include <list>
#include <map>
#include <string>
#include <vector>
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Attribute.h"
#include "plasma/Widget.h"
#include "plasma/Button.h"
#include "plasma/Texture.h"
#include "plasma/ChunkStreamReader.h"

// Widens a string character by character; implemented in support/StrokeWidthModule.cpp.
std::wstring widenString(const std::string &text);

namespace plasma {

class PLXStreamReader;
class PLXStreamWriter;
template<typename T> class ContinuousArrayAttribute;

class Exception {
public:
    explicit Exception(const std::string &message)
        : m_message(message)
    {}

    Exception(const Exception &other)
        : m_message(other.m_message)
    {}

    virtual const std::string &getMessage() const { return m_message; }

    std::string m_message;
};

class IOException : public Exception {
public:
    IOException(const std::string &fileName, const std::string &message)
        : Exception(message),
          m_fileName(fileName)
    {}

    IOException(const IOException &other);

    std::string m_fileName;
};

class InvalidFileFormatException : public IOException {
public:
    explicit InvalidFileFormatException(const std::string &fileName);
    InvalidFileFormatException(const InvalidFileFormatException &other);
};

class InvalidVersionException : public IOException {
public:
    explicit InvalidVersionException(const std::string &fileName);
    InvalidVersionException(const InvalidVersionException &other);
};

class InvalidDemoLicenseException : public IOException {
public:
    explicit InvalidDemoLicenseException(const std::string &fileName);
    InvalidDemoLicenseException(const InvalidDemoLicenseException &other);
};

class PLXStreamReader : public ChunkStreamReader {
public:
    PLXStreamReader(Engine *engine, Engine::Page *page, Node *target, int flags, std::istream &stream,
                    const std::vector<unsigned __int64> &keys);
    void read();
    Texture *readTexture();
    bool readAttributeSequence(Attribute *attribute);
    Shape *readTextShape();
    Shape *readSmoothMeshShape();
    GenericShape *readGenericShape();
    Transformation *readTransformation();
    Display *readDisplay();
    Widget *readWidget();
    Button *readButton();
    Widget *readListWidget();
    Widget *readEdit();
    Widget *readScrollButton();
    Widget *readPopUpButton();
    Widget *readScrollSlider();
    bool readWidgetProperty(const std::string &chunk, Widget *widget);
    bool readButtonProperty(const std::string &chunk, Button *button);
    bool readListWidgetProperty(Widget *widget, const std::string &chunk);
    bool readScrollButtonProperty(const std::string &chunk, Widget *widget);
    bool readScrollSliderProperty(const std::string &chunk, Widget *widget);
    bool readPopUpButtonProperty(const std::string &chunk, Widget *widget);
    Node *readNode(bool isRoot);

    bool readContinuousVector4(Attribute *attribute);
    bool readArrayVector2(ContinuousArrayAttribute<Vector<2, float>> *attribute);
    bool readArrayVector4(ContinuousArrayAttribute<Vector<4, float>> *attribute);
    bool readArrayFloat(ContinuousArrayAttribute<float> *attribute);
    bool readDiscreteInt(Attribute *attribute);
    bool readContinuousVector2(Attribute *attribute);
    bool readContinuousVector3(Attribute *attribute);
    bool readContinuousMatrix(Attribute *attribute);
    bool readContinuousFloat(Attribute *attribute);
    Vector<2, float> readVector2();
    Matrix<float> readMatrix();
    Vector<4, float> readVector4();

    std::map<int, int> m_textureIds;
    std::map<int, int> m_fontIds; // Font identifier mapping, currently unused.
    std::map<int, Node *> m_nodes;
    std::map<int, Shape *> m_shapes;
    std::map<int, Transformation *> m_transformations;
    std::map<int, Display *> m_displays;
    std::map<int, Widget *> m_widgets;
    // Child ids keyed by node pointer.
    std::map<void *, std::list<int>> m_children;
    int m_flags;
    std::vector<unsigned __int64> m_keys;
    Engine *m_engine;
    Engine::Page *m_page;
    Node *m_target;
};

bool decompressPLX(const std::vector<char> &compressed, std::vector<char> &output);

// KeyFromPhrase is defined in Workspace.cpp and Deobfuscate in ChunkStreamReader.cpp. Obfuscate is only declared
// here; the writer uses StreamWriter::Obfuscate.
void deobfuscate(char *data, int length, const unsigned __int64 &key);
void obfuscate(char *data, int length, const unsigned __int64 &key);
unsigned __int64 keyFromPhrase(const std::string &phrase);

}
