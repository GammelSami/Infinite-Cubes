// PLXStreamReader loads scene objects, textures, fonts and animation from PLX chunks.
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

// Flags consumed by the engine's scene readers. Individual flags are bit indices; use (1 << flag)
// to form a mask.
enum SceneLoadFlags {
    LOAD_SHARED = 0,
    LOAD_NO_WIDGET_INIT = 1,
    LOAD_INTERNAL = 2,
    LOAD_IGNORE_PAGE = 3,
    LOAD_STATIC_MESHES = 4,
    LOAD_HINTED_TEXT = 5,
    LOAD_CHECK_SIGNATURE = 6
};

// Legacy PLA font entry; PLX retains an empty list for this record.
struct PLAFontReference {
    std::string name;
    std::string source;
    int size;
    float stroke;
    int id;
};

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

// Computes a rolling phrase hash: begin with the fixed seed, multiply by PhraseKeyMultiplier and add each byte.
enum {
    PhraseKeyMultiplier = 31
};

inline unsigned __int64 keyFromPhrase(const std::string &phrase)
{
    unsigned __int64 result = 0x0003FFFFFFFFFFE5ULL;
    for (int i = 0; i < static_cast<int>(phrase.size()); ++i)
        result = result * PhraseKeyMultiplier + static_cast<unsigned char>(phrase[i]);
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

    std::map<int, int> m_textureIds;
    std::map<int, int> m_fontIds;
    std::map<int, Node *> m_nodes;
    std::map<int, Shape *> m_shapes;
    std::map<int, Transformation *> m_transformations;
    std::map<int, Display *> m_displays;
    std::map<int, Widget *> m_widgets;
    std::map<Node *, std::list<int>> m_children;
    int m_flags;
    std::vector<unsigned __int64> m_keys;
    Engine *m_engine;
    Engine::Page *m_page;
    Node *m_target;
};

bool decompressPLX(const std::vector<unsigned char> &compressed, std::vector<unsigned char> &output);

}
