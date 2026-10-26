// PLXStreamReader loads chunked Plasma documents, numbering objects by kind as chunks are encountered. It resolves
// child-node references after reading the whole stream.
#pragma once
#include <istream>
#include <list>
#include <map>
#include <string>
#include <vector>
#include "plasma/ChunkStreamReader.h"
#include "plasma/Engine.h"
#include "plasma/Exception.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

namespace plasma {

class Attribute;
class Button;
class Display;
class GenericShape;
class Node;
class ScrollButton;
class ScrollSlider;
class Shape;
class Texture;
class Transformation;
class Widget;
template<typename T> class ContinuousArrayAttribute;

// Computes the seal key of a phrase.
inline unsigned __int64 keyFromPhrase(const std::string &phrase)
{
    unsigned __int64 result = 0x0003FFFFFFFFFFE5ULL;
    for (int i = 0; i < static_cast<int>(phrase.size()); ++i)
        result = result * 31 + static_cast<unsigned char>(phrase[i]);
    return result;
}

class PLXStreamReader : public ChunkStreamReader {
public:
    PLXStreamReader(Engine *engine, Engine::Page *page, Node *target, int flags, std::istream &stream,
                    const std::vector<unsigned __int64> &keys);

    void read();
    bool readAttributeSequence(Attribute *attribute);
    Button *readButton();
    Display *readDisplay();
    Widget *readEdit();
    GenericShape *readGenericShape();
    Widget *readListWidget();
    Node *readNode(bool isRoot);
    Widget *readPopUpButton();
    Widget *readScrollButton();
    Widget *readScrollSlider();
    Shape *readSmoothMeshShape();
    Shape *readTextShape();
    Texture *readTexture();
    Transformation *readTransformation();
    Widget *readWidget();
    bool readWidgetProperty(Widget *widget, const std::string &chunk);

    bool readButtonProperty(Button *button, const std::string &chunk);
    bool readScrollButtonProperty(ScrollButton *button, const std::string &chunk);
    bool readScrollSliderProperty(ScrollSlider *slider, const std::string &chunk);

    template<typename T> bool readArrayAttribute(ContinuousArrayAttribute<T> *attribute);
    template<typename A> bool readAttribute(A *attribute);

    std::map<int, int> m_textureIds; // file texture id -> engine texture id
    std::map<int, int> m_fontIds;    // only the -1 entry is ever stored
    std::map<int, Node *> m_nodes;
    std::map<int, Shape *> m_shapes;
    std::map<int, Transformation *> m_transformations;
    std::map<int, Display *> m_displays;
    std::map<int, Widget *> m_widgets;
    std::map<Node *, std::list<int>> m_children;
    int m_flags;                          // Engine's LOAD_ flags
    std::vector<unsigned __int64> m_keys; // the caller's seal keys
    Engine *m_engine;
    Engine::Page *m_page;
    Node *m_target; // becomes the first node of the stream
};

bool decompressPLX(const std::vector<unsigned char> &compressed, std::vector<unsigned char> &output);

}
