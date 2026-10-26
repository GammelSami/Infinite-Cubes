// PLXStreamReader loads scene objects, textures, fonts and animation from PLX chunks.
#include "plasma/PLXStreamReader.h"
#include "plasma/Widget.h"
#include "plasma/Node.h"
#include "plasma/Texture.h"
#include "plasma/TextShape.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/StaticMeshShape.h"
#include <cstring>
#include "plasma/Attribute.h"
#include "plasma/Display.h"
#include "plasma/Transformation.h"
#include "plasma/GenericShape.h"
#include "plasma/Button.h"
#include "plasma/Edit.h"
#include "plasma/ListWidget.h"
#include "plasma/PopUpButton.h"
#include "plasma/ScrollButton.h"
#include "plasma/ScrollSlider.h"

#include "plasma/StringConversion.h"
#include <zlib.h>

namespace plasma {
// The newest PlasmaGraphics version read (anonymous: this file's own).
namespace {
enum {
    MaximumPlasmaGraphicsVersion = 1
};
}

template<typename T> bool PLXStreamReader::readArrayAttribute(ContinuousArrayAttribute<T> *attribute)
{
    enterChunk();
    int frame = 0;
    bool finished;
    while (!(finished = finishChunk())) {
        std::string chunk = parseChunkHeader(true);
        if (chunk == "ArrayAttribute.size") {
            enterChunk();
            attribute->resize(readValue<int>());
            finishChunk();
        } else if (chunk == "ArrayAttribute.frame") {
            enterChunk();
            if (frame == attribute->size())
                attribute->duplicateElement();
            // Forms element zero even for an empty array.
            m_stream->read(reinterpret_cast<char *>(&(*attribute->m_values[frame])[0]),
                           sizeof(T) * attribute->m_values[0]->size());
            ++frame;
            finishChunk();
        } else if (chunk == "Attribute.sequence") {
            readAttributeSequence(attribute);
        } else {
            skipChunk();
        }
    }
    return finished;
}

template<typename A> bool PLXStreamReader::readAttribute(A *attribute)
{
    enterChunk();
    int frame = 0;
    bool finished;
    while (!(finished = finishChunk())) {
        std::string chunk = parseChunkHeader(true);
        if (chunk == "Attribute.frame") {
            enterChunk();
            if (frame == attribute->size())
                attribute->duplicateElement();
            m_stream->read(reinterpret_cast<char *>(&attribute->m_values[frame]), sizeof(attribute->m_values[frame]));
            ++frame;
            finishChunk();
        } else if (chunk == "Attribute.sequence") {
            readAttributeSequence(attribute);
        } else {
            skipChunk();
        }
    }
    return finished;
}

PLXStreamReader::PLXStreamReader(Engine *engine, Engine::Page *page, Node *target, int flags, std::istream &stream,
                                 const std::vector<unsigned __int64> &keys)
    : ChunkStreamReader(stream),
      m_flags(flags),
      m_keys(keys),
      m_engine(engine),
      m_page(page),
      m_target(target)
{}

void PLXStreamReader::read()
{
    m_nodes[NO_OBJECT_ID] = 0;
    m_shapes[NO_OBJECT_ID] = 0;
    m_transformations[NO_OBJECT_ID] = 0;
    m_displays[NO_OBJECT_ID] = 0;
    m_widgets[NO_OBJECT_ID] = 0;
    m_fontIds[NO_OBJECT_ID] = NO_OBJECT_ID;
    m_textureIds[NO_OBJECT_ID] = NO_OBJECT_ID;
    int shapeIndex = 0;
    int transformationIndex = 0;
    int displayIndex = 0;
    int widgetIndex = 0;
    int nodeIndex = 0;
    // Both lists are constructed and never filled.
    std::list<PLAFontReference> fonts;
    std::list<int> children;
    if (m_flags & (1 << LOAD_CHECK_SIGNATURE)) {
        std::streampos position = m_stream->tellg();
        readValue<int>();
        readValue<int>();
        readValue<int>();
        readValue<int>();
        char signature[15] = "              ";
        m_stream->read(signature, 14);
        m_stream->seekg(position);
        if (strcmp(signature, "PlasmaGraphics"))
            throw InvalidFileFormatException("");
    }
    m_obfuscationKey = 0;
    while (!m_stream->eof()) {
        std::string chunk = parseChunkHeader(true);
        if (m_stream->eof())
            break;
        if (chunk == "PlasmaGraphics") {
            enterChunk();
            if (readValue<int>() > MaximumPlasmaGraphicsVersion)
                throw InvalidVersionException("");
            finishChunk();
        } else if (chunk == "Seal") {
            enterChunk();
            int length = readValue<int>();
            std::vector<char> sealed(length);
            m_stream->read(&sealed[0], length);
            bool valid = false;
            // The caller's keys, then no key, then the default phrase's key.
            std::vector<unsigned __int64> keys(m_keys);
            unsigned __int64 key = 0;
            keys.push_back(key);
            keys.push_back(keyFromPhrase("PlasmaGraphics"));
            for (int i = 0; i < (int)keys.size(); ++i) {
                std::vector<char> clear(sealed);
                deobfuscate(&clear[0], length, keys[i]);
                std::string text(clear.begin(), clear.end());
                if (text == "PlasmaGraphics") {
                    m_obfuscationKey = keys[i];
                    valid = true;
                    break;
                }
            }
            finishChunk();
            if (!valid)
                throw InvalidDemoLicenseException("");
        } else if (chunk == "Texture") {
            readTexture();
        } else if (chunk == "TextShape") {
            Shape *&shapeSlot = m_shapes[shapeIndex++];
            shapeSlot = readTextShape();
        } else if (chunk == "SmoothMeshShape") {
            Shape *&shapeSlot = m_shapes[shapeIndex++];
            shapeSlot = readSmoothMeshShape();
        } else if (chunk == "GenericShape") {
            Shape *&shapeSlot = m_shapes[shapeIndex++];
            shapeSlot = readGenericShape();
        } else if (chunk == "Transformation") {
            Transformation *&transformationSlot = m_transformations[transformationIndex++];
            transformationSlot = readTransformation();
        } else if (chunk == "Display") {
            Display *&displaySlot = m_displays[displayIndex++];
            displaySlot = readDisplay();
        } else if (chunk == "Widget") {
            Widget *&widgetSlot = m_widgets[widgetIndex++];
            widgetSlot = readWidget();
        } else if (chunk == "Button") {
            Widget *&widgetSlot = m_widgets[widgetIndex++];
            widgetSlot = readButton();
        } else if (chunk == "ListWidget") {
            Widget *&widgetSlot = m_widgets[widgetIndex++];
            widgetSlot = readListWidget();
        } else if (chunk == "Edit") {
            Widget *&widgetSlot = m_widgets[widgetIndex++];
            widgetSlot = readEdit();
        } else if (chunk == "ScrollButton") {
            Widget *&widgetSlot = m_widgets[widgetIndex++];
            widgetSlot = readScrollButton();
        } else if (chunk == "ScrollSlider") {
            Widget *&widgetSlot = m_widgets[widgetIndex++];
            widgetSlot = readScrollSlider();
        } else if (chunk == "PopUpButton") {
            Widget *&widgetSlot = m_widgets[widgetIndex++];
            widgetSlot = readPopUpButton();
        } else if (chunk == "Node") {
            Node *&node = m_nodes[nodeIndex];
            node = readNode(nodeIndex == 0);
            ++nodeIndex;
        } else if (!(m_flags & (1 << LOAD_IGNORE_PAGE)) && chunk == "pageWidth") {
            enterChunk();
            m_page->size.m_elements[0] = readValue<float>();
            finishChunk();
        } else if (!(m_flags & (1 << LOAD_IGNORE_PAGE)) && chunk == "pageHeight") {
            enterChunk();
            m_page->size.m_elements[1] = readValue<float>();
            finishChunk();
        } else if (!(m_flags & (1 << LOAD_IGNORE_PAGE)) && chunk == "dpi") {
            enterChunk();
            m_page->size.m_elements[2] = readValue<float>();
            finishChunk();
        } else if (!(m_flags & (1 << LOAD_IGNORE_PAGE)) && chunk == "unit") {
            enterChunk();
            m_page->unit = readValue<int>();
            finishChunk();
        } else if (!(m_flags & (1 << LOAD_IGNORE_PAGE)) && chunk == "pageColor") {
            enterChunk();
            m_page->color = readValue<Vector<4, float>>();
            finishChunk();
        } else {
            skipChunk();
        }
    }
    for (std::map<int, Node *>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if (it->second) {
            std::list<int> &children = m_children[it->second];
            for (std::list<int>::iterator child = children.begin(); child != children.end(); ++child)
                it->second->addChild(m_nodes[*child]);
        }
    }
}

bool PLXStreamReader::readAttributeSequence(Attribute *attribute)
{
    enterChunk();
    Sequence *sequence = 0;
    bool finished;
    while (!(finished = finishChunk())) {
        std::string chunk = parseChunkHeader(true);
        if (chunk == "Attribute.sequence.name") {
            enterChunk();
            sequence = attribute->newSequence(widenString(readString()));
            finishChunk();
        } else if (chunk == "Attribute.sequence.wname") {
            enterChunk();
            sequence = attribute->newSequence(readWString());
            finishChunk();
        } else if (chunk == "Attribute.sequence.key") {
            enterChunk();
            int time = 0;
            int element = 0;
            float smoothness = 0.0f;
            while (!finishChunk()) {
                std::string keyChunk = parseChunkHeader(true);
                if (keyChunk == "Attribute.sequence.key.frame") {
                    enterChunk();
                    element = readValue<int>();
                    finishChunk();
                } else if (keyChunk == "Attribute.sequence.key.time") {
                    enterChunk();
                    time = readValue<int>();
                    finishChunk();
                } else if (keyChunk == "Attribute.sequence.key.smoothness") {
                    enterChunk();
                    smoothness = readValue<float>();
                    finishChunk();
                } else {
                    skipChunk();
                }
            }
            // A key before the sequence's name dereferences a null sequence,.
            sequence->addKey(element, time, smoothness);
        } else {
            skipChunk();
        }
    }
    return finished;
}

Button *PLXStreamReader::readButton()
{
    enterChunk();
    Button *widget = m_engine->createButton(0, Button::Push);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (!readButtonProperty(widget, chunk))
            skipChunk();
    }
    return widget;
}

// Reads this chunk into its owner.
bool PLXStreamReader::readButtonProperty(Button *button, const std::string &chunk)
{
    if (chunk == "Button.type") {
        enterChunk();
        button->m_type = readValue<int>();
        finishChunk();
        return true;
    }
    return readWidgetProperty(button, chunk);
}

Display *PLXStreamReader::readDisplay()
{
    enterChunk();
    Display *display = m_engine->createDisplay();
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (chunk == "Display.name") {
            enterChunk();
            display->setName(widenString(readString()));
            finishChunk();
        } else if (chunk == "Display.wname") {
            enterChunk();
            display->setName(readWString());
            finishChunk();
        } else if (chunk == "Display.visibility") {
            readAttribute(&display->m_visibility);
        } else if (chunk == "Display.clipping") {
            readAttribute(&display->m_clipping);
        } else if (chunk == "Display.strokeColor") {
            readAttribute(&display->m_strokeColor);
        } else if (chunk == "Display.fillColor") {
            readAttribute(&display->m_fillColor);
        } else if (chunk == "Display.blurRadius") {
            readAttribute(&display->m_blurRadius);
        } else if (chunk == "Display.flags") {
            enterChunk();
            display->m_flags = readValue<int>();
            finishChunk();
        } else {
            skipChunk();
        }
    }
    return display;
}

Widget *PLXStreamReader::readEdit()
{
    enterChunk();
    Widget *widget = m_engine->createEdit(0);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (!readWidgetProperty(widget, chunk))
            skipChunk();
    }
    return widget;
}

GenericShape *PLXStreamReader::readGenericShape()
{
    enterChunk();
    GenericShape *shape = m_engine->createGenericShape();
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (chunk == "GenericShape.name") {
            enterChunk();
            shape->setName(widenString(readString()));
            finishChunk();
        } else if (chunk == "GenericShape.wname") {
            enterChunk();
            shape->setName(readWString());
            finishChunk();
        } else if (chunk == "GenericShape.source") {
            enterChunk();
            shape->setSource(widenString(readString()));
            finishChunk();
        } else if (chunk == "GenericShape.wsource") {
            enterChunk();
            shape->setSource(readWString());
            finishChunk();
        } else if (chunk == "GenericShape.position") {
            enterChunk();
            shape->m_position = readValue<Vector<2, float>>();
            finishChunk();
        } else if (chunk == "GenericShape.size") {
            enterChunk();
            shape->m_size = readValue<Vector<2, float>>();
            finishChunk();
        } else {
            skipChunk();
        }
    }
    shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return shape;
}

Widget *PLXStreamReader::readListWidget()
{
    enterChunk();
    Widget *widget = m_engine->createListWidget(0);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (!readWidgetProperty(widget, chunk))
            skipChunk();
    }
    return widget;
}

Node *PLXStreamReader::readNode(bool isRoot)
{
    enterChunk();
    std::wstring name;
    Shape *shape = 0;
    Transformation *transformation = 0;
    Display *display = 0;
    Widget *widget = 0;
    std::list<int> children;
    std::list<std::pair<std::string, std::string>> variables;
    unsigned flags; // stays uninitialised if the chunk is missing
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (chunk == "Node.name") {
            enterChunk();
            name = widenString(readString());
            finishChunk();
        } else if (chunk == "Node.wname") {
            enterChunk();
            name = readWString();
            finishChunk();
        } else if (chunk == "Node.shape") {
            enterChunk();
            shape = m_shapes[readValue<int>()];
            finishChunk();
        } else if (chunk == "Node.transformation") {
            enterChunk();
            transformation = m_transformations[readValue<int>()];
            finishChunk();
        } else if (chunk == "Node.display") {
            enterChunk();
            display = m_displays[readValue<int>()];
            finishChunk();
        } else if (chunk == "Node.widget") {
            enterChunk();
            widget = m_widgets[readValue<int>()];
            finishChunk();
        } else if (chunk == "Node.child") {
            enterChunk();
            children.push_back(readValue<int>());
            finishChunk();
        } else if (chunk == "Node.flags") {
            enterChunk();
            flags = readValue<int>();
            finishChunk();
        } else if (chunk == "Node.variable") {
            enterChunk();
            // The second argument is read first: the pair is (value, key).
            variables.push_back(std::make_pair(readString(), readString()));
            finishChunk();
        } else {
            skipChunk();
        }
    }
    Node *node;
    if (isRoot) {
        node = m_target;
    } else {
        node = m_engine->createNode(transformation, shape, display, 0, name);
        node->setWidget(widget);
        node->m_flags = flags;
        for (std::list<std::pair<std::string, std::string>>::iterator it = variables.begin(); it != variables.end();
             ++it)
            node->setVariable(it->second, it->first);
        if (m_flags & (1 << LOAD_INTERNAL))
            node->m_flags |= (1 << NODE_INTERNAL);
    }
    m_children[node] = children;
    return node;
}

Widget *PLXStreamReader::readPopUpButton()
{
    enterChunk();
    PopUpButton *widget = m_engine->createPopUpButton(0);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (!readButtonProperty(widget, chunk))
            skipChunk();
    }
    return widget;
}

Widget *PLXStreamReader::readScrollButton()
{
    enterChunk();
    ScrollButton *widget = m_engine->createScrollButton(0);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (!readScrollButtonProperty(widget, chunk))
            skipChunk();
    }
    return widget;
}

// Reads this chunk into its owner.
bool PLXStreamReader::readScrollButtonProperty(ScrollButton *button, const std::string &chunk)
{
    if (chunk == "ScrollButton.direction") {
        enterChunk();
        button->m_direction = readValue<int>();
        finishChunk();
        return true;
    }
    return readButtonProperty(button, chunk);
}

Widget *PLXStreamReader::readScrollSlider()
{
    enterChunk();
    ScrollSlider *widget = m_engine->createScrollSlider(0);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (!readScrollSliderProperty(widget, chunk))
            skipChunk();
    }
    return widget;
}

// Reads this chunk into its owner.
bool PLXStreamReader::readScrollSliderProperty(ScrollSlider *slider, const std::string &chunk)
{
    if (chunk == "ScrollSlider.direction") {
        enterChunk();
        slider->m_direction = readValue<int>();
        finishChunk();
        return true;
    }
    return readButtonProperty(slider, chunk);
}

Shape *PLXStreamReader::readSmoothMeshShape()
{
    enterChunk();
    SmoothMeshShape *shape = m_engine->createSmoothMeshShape();
    std::vector<std::vector<int>> faces;
    int subdivisions = shape->m_subdivisions;
    float smoothWeight = shape->m_smoothWeight;
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (chunk == "SmoothMeshShape.name") {
            enterChunk();
            shape->setName(widenString(readString()));
            finishChunk();
        } else if (chunk == "SmoothMeshShape.wname") {
            enterChunk();
            shape->setName(readWString());
            finishChunk();
        } else if (chunk == "SmoothMeshShape.face") {
            enterChunk();
            int count = readValue<int>();
            std::vector<int> face(count);
            m_stream->read(reinterpret_cast<char *>(&face[0]), count * sizeof(int));
            faces.push_back(face);
            finishChunk();
        } else if (chunk == "SmoothMeshShape.vertexFlags") {
            enterChunk();
            int count = readValue<int>();
            if (count > 0) {
                std::vector<int> &values = shape->m_vertexFlags;
                values.resize(count);
                m_stream->read(reinterpret_cast<char *>(&values[0]), count * sizeof(int));
            }
            finishChunk();
        } else if (chunk == "SmoothMeshShape.vertexParameters") {
            enterChunk();
            int count = readValue<int>();
            if (count > 0) {
                std::vector<float> &values = shape->m_vertexParameters;
                values.resize(count);
                m_stream->read(reinterpret_cast<char *>(&values[0]), count * sizeof(float));
            }
            finishChunk();
        } else if (chunk == "SmoothMeshShape.vertexPositions") {
            readArrayAttribute(&shape->m_vertexPositions);
        } else if (chunk == "SmoothMeshShape.vertexTexCoords") {
            readArrayAttribute(&shape->m_vertexTexCoords);
        } else if (chunk == "SmoothMeshShape.vertexColors") {
            readArrayAttribute(&shape->m_vertexColors);
        } else if (chunk == "SmoothMeshShape.strokeColors") {
            readArrayAttribute(&shape->m_strokeColors);
        } else if (chunk == "SmoothMeshShape.extrusionFrontColors") {
            readArrayAttribute(&shape->m_extrusionFrontColors);
        } else if (chunk == "SmoothMeshShape.extrusionBackColors") {
            readArrayAttribute(&shape->m_extrusionBackColors);
        } else if (chunk == "SmoothMeshShape.strokeWidths") {
            readArrayAttribute(&shape->m_strokeWidths);
        } else if (chunk == "SmoothMeshShape.texture") {
            DiscreteAttribute<int> *attribute = &shape->m_texture;
            readAttribute(attribute);
            for (int i = 0; i < attribute->size(); ++i) {
                int &id = shape->m_texture.m_values[i];
                id = m_textureIds[id];
            }
        } else if (chunk == "SmoothMeshShape.strokeTexture") {
            DiscreteAttribute<int> *attribute = &shape->m_strokeTexture;
            readAttribute(attribute);
            for (int i = 0; i < attribute->size(); ++i) {
                int &id = shape->m_strokeTexture.m_values[i];
                id = m_textureIds[id];
            }
        } else if (chunk == "SmoothMeshShape.textureTranslation") {
            readAttribute(&shape->m_textureTranslation);
        } else if (chunk == "SmoothMeshShape.textureRotation") {
            readAttribute(&shape->m_textureRotation);
        } else if (chunk == "SmoothMeshShape.textureDeformation") {
            readAttribute(&shape->m_textureDeformation);
        } else if (chunk == "SmoothMeshShape.texturePivot") {
            readAttribute(&shape->m_texturePivot);
        } else if (chunk == "SmoothMeshShape.textureOpacity") {
            readAttribute(&shape->m_textureOpacity);
        } else if (chunk == "SmoothMeshShape.textureBrightness") {
            readAttribute(&shape->m_textureBrightness);
        } else if (chunk == "SmoothMeshShape.textureContrast") {
            readAttribute(&shape->m_textureContrast);
        } else if (chunk == "SmoothMeshShape.textureSaturation") {
            readAttribute(&shape->m_textureSaturation);
        } else if (chunk == "SmoothMeshShape.strokeTextureOpacity") {
            readAttribute(&shape->m_strokeTextureOpacity);
        } else if (chunk == "SmoothMeshShape.strokeTextureBrightness") {
            readAttribute(&shape->m_strokeTextureBrightness);
        } else if (chunk == "SmoothMeshShape.strokeTextureContrast") {
            readAttribute(&shape->m_strokeTextureContrast);
        } else if (chunk == "SmoothMeshShape.strokeTextureSaturation") {
            readAttribute(&shape->m_strokeTextureSaturation);
        } else if (chunk == "SmoothMeshShape.strokeTextureStretch") {
            readAttribute(&shape->m_strokeTextureStretch);
        } else if (chunk == "SmoothMeshShape.extrusionMatrix") {
            readAttribute(&shape->m_extrusionMatrix);
        } else if (chunk == "SmoothMeshShape.subdivisions") {
            enterChunk();
            subdivisions = readValue<int>();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.smoothWeight") {
            enterChunk();
            smoothWeight = readValue<float>();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.flags") {
            enterChunk();
            shape->m_flags = readValue<int>();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.strokeJointType") {
            enterChunk();
            shape->m_strokeJointType = readValue<int>();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.strokeCapType") {
            enterChunk();
            shape->m_strokeCapType = readValue<int>();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.strokeAlignment") {
            enterChunk();
            shape->m_strokeAlignment = readValue<int>();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.strokePattern") {
            enterChunk();
            shape->m_strokePattern = readValue<int>();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.strokeDash") {
            enterChunk();
            shape->setStrokeDash(readValue<float>());
            finishChunk();
        } else if (chunk == "SmoothMeshShape.strokeGap") {
            enterChunk();
            shape->setStrokeGap(readValue<float>());
            finishChunk();
        } else {
            skipChunk();
        }
    }
    shape->m_smoothWeight = smoothWeight;
    shape->setFaces(faces, false);
    shape->setSubdivisions(subdivisions, true);
    if ((m_flags & (1 << LOAD_STATIC_MESHES)) && !shape->countSequences()) {
        shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
        StaticMeshShape *result = shape->convertToStaticMeshShape();
        result->updateDrawing();
        m_engine->deleteShape(shape);
        return result;
    }
    return shape;
}

Shape *PLXStreamReader::readTextShape()
{
    enterChunk();
    TextShape *shape = m_engine->createTextShape();
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (chunk == "TextShape.name") {
            enterChunk();
            shape->setName(widenString(readString()));
            finishChunk();
        } else if (chunk == "TextShape.wname") {
            enterChunk();
            shape->setName(readWString());
            finishChunk();
        } else if (chunk == "TextShape.string") {
            enterChunk();
            int frame = 0;
            while (!finishChunk()) {
                std::string frameChunk = parseChunkHeader(true);
                if (frameChunk == "Attribute.frame") {
                    enterChunk();
                    if (frame == shape->m_string.size())
                        shape->m_string.duplicateElement();
                    shape->m_string.m_values[frame] = readWString();
                    ++frame;
                    finishChunk();
                } else if (frameChunk == "Attribute.sequence") {
                    readAttributeSequence(&shape->m_string);
                } else {
                    skipChunk();
                }
            }
        } else if (chunk == "TextShape.color") {
            readAttribute(&shape->m_color);
        } else if (chunk == "TextShape.strokeColor") {
            readAttribute(&shape->m_strokeColor);
        } else if (chunk == "TextShape.extrusionColor") {
            readAttribute(&shape->m_extrusionColor);
        } else if (chunk == "TextShape.flags") {
            enterChunk();
            shape->m_flags = readValue<int>();
            finishChunk();
        } else if (chunk == "TextShape.pixelSize") {
            enterChunk();
            shape->m_pixelSize = readValue<float>();
            finishChunk();
        } else if (chunk == "TextShape.strokeRadius") {
            enterChunk();
            shape->m_strokeRadius = readValue<float>();
            finishChunk();
        } else if (chunk == "TextShape.spacing") {
            enterChunk();
            shape->m_spacing = readValue<float>();
            finishChunk();
        } else if (chunk == "TextShape.lineSpacing") {
            enterChunk();
            shape->m_lineSpacing = readValue<float>();
            finishChunk();
        } else if (chunk == "TextShape.wrapWidth") {
            enterChunk();
            shape->m_wrapWidth = readValue<float>();
            finishChunk();
        } else if (chunk == "TextShape.fontSize") {
            enterChunk();
            shape->m_pixelSize = static_cast<float>(readValue<int>());
            finishChunk();
        } else if (chunk == "TextShape.strokeWidth") {
            enterChunk();
            shape->m_strokeRadius = readValue<int>() * 0.5f;
            finishChunk();
        } else if (chunk == "TextShape.fontName") {
            enterChunk();
            shape->m_fontName = widenString(readString());
            finishChunk();
        } else if (chunk == "TextShape.wfontName") {
            enterChunk();
            shape->m_fontName = readWString();
            finishChunk();
        } else if (chunk == "TextShape.font") {
            std::wstring fileName;
            float pixelSize = 0.0f;
            float strokeRadius = 0.0f;
            enterChunk();
            while (!finishChunk()) {
                std::string fontChunk = parseChunkHeader(true);
                if (fontChunk == "TextShape.font.fileName") {
                    enterChunk();
                    fileName = widenString(readString());
                    finishChunk();
                } else if (fontChunk == "TextShape.font.wfileName") {
                    enterChunk();
                    fileName = readWString();
                    finishChunk();
                } else if (fontChunk == "TextShape.font.pixelSize") {
                    enterChunk();
                    pixelSize = static_cast<float>(readValue<int>());
                    finishChunk();
                } else if (fontChunk == "TextShape.font.glowRadius") {
                    enterChunk();
                    strokeRadius = readValue<int>() * 0.5f;
                    finishChunk();
                } else if (fontChunk == "TextShape.font.size") {
                    enterChunk();
                    pixelSize = readValue<float>();
                    finishChunk();
                } else if (fontChunk == "TextShape.font.strokeWidth") {
                    enterChunk();
                    strokeRadius = readValue<float>() * 0.5f;
                    finishChunk();
                } else if (fontChunk == "TextShape.font.strokeGlow" || fontChunk == "TextShape.font.pixelFont") {
                    enterChunk();
                    readValue<int>();
                    finishChunk();
                }
                // Unrecognised chunks inside the legacy font are not skipped.
            }
            shape->m_fontName = fileName;
            shape->m_pixelSize = pixelSize;
            shape->m_strokeRadius = strokeRadius;
        } else {
            skipChunk();
        }
    }
    if (m_flags & (1 << LOAD_HINTED_TEXT))
        shape->m_hintingFlags |= (1 << TEXT_HINTED);
    shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return shape;
}

Texture *PLXStreamReader::readTexture()
{
    enterChunk();
    std::wstring name;
    int id = NO_OBJECT_ID;
    TextureFormat format;
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (chunk == "Texture.name") {
            enterChunk();
            name = widenString(readString());
            finishChunk();
        } else if (chunk == "Texture.wname") {
            enterChunk();
            name = readWString();
            finishChunk();
        } else if (chunk == "Texture.id") {
            enterChunk();
            id = readValue<int>();
            finishChunk();
        } else if (chunk == "Texture.format.pixelFormat") {
            enterChunk();
            format.pixelFormat = readValue<int>();
            finishChunk();
        } else if (chunk == "Texture.format.minFilter") {
            enterChunk();
            format.minFilter = readValue<int>();
            finishChunk();
        } else if (chunk == "Texture.format.maxFilter") {
            enterChunk();
            format.maxFilter = readValue<int>();
            finishChunk();
        } else if (chunk == "Texture.format.horizontalWrap") {
            enterChunk();
            format.horizontalWrap = readValue<int>();
            finishChunk();
        } else if (chunk == "Texture.format.verticalWrap") {
            enterChunk();
            format.verticalWrap = readValue<int>();
            finishChunk();
        } else if (chunk == "Texture.width") {
            enterChunk();
            width = readValue<int>();
            finishChunk();
        } else if (chunk == "Texture.height") {
            enterChunk();
            height = readValue<int>();
            finishChunk();
        } else if (chunk == "Texture.pixels") {
            enterChunk();
            int count = readValue<int>();
            if (count > 0) {
                pixels.resize(count);
                m_stream->read(reinterpret_cast<char *>(&pixels[0]), count);
            }
            finishChunk();
        } else if (chunk == "Texture.compressedPixels") {
            enterChunk();
            int count = readValue<int>();
            if (count > 0) {
                std::vector<unsigned char> compressed(count);
                m_stream->read(reinterpret_cast<char *>(&compressed[0]), count);
                decompressPLX(compressed, pixels);
            }
            finishChunk();
        } else {
            skipChunk();
        }
    }
    if (id == NO_OBJECT_ID)
        return 0;
    // The element-zero pointer is formed even when the pixel vector is empty.
    Texture *texture = m_engine->createTexture(width, height, &pixels[0], format, name, false);
    int textureId = texture->m_id;
    m_textureIds[id] = textureId;
    return texture;
}

Transformation *PLXStreamReader::readTransformation()
{
    enterChunk();
    Transformation *transformation = m_engine->createTransformation();
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (chunk == "Transformation.name") {
            enterChunk();
            transformation->setName(widenString(readString()));
            finishChunk();
        } else if (chunk == "Transformation.wname") {
            enterChunk();
            transformation->setName(readWString());
            finishChunk();
        } else if (chunk == "Transformation.translation") {
            readAttribute(&transformation->m_translation);
        } else if (chunk == "Transformation.rotation") {
            readAttribute(&transformation->m_rotation);
        } else if (chunk == "Transformation.pivot") {
            readAttribute(&transformation->m_pivot);
        } else if (chunk == "Transformation.deformation") {
            readAttribute(&transformation->m_deformation);
        } else {
            skipChunk();
        }
    }
    transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return transformation;
}

Widget *PLXStreamReader::readWidget()
{
    enterChunk();
    Widget *widget = m_engine->createWidget(0);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader(true);
        if (!readWidgetProperty(widget, chunk))
            skipChunk();
    }
    return widget;
}

bool PLXStreamReader::readWidgetProperty(Widget *widget, const std::string &chunk)
{
    if (chunk == "Widget.name") {
        enterChunk();
        widget->setName(widenString(readString()));
        finishChunk();
        return true;
    } else if (chunk == "Widget.wname") {
        enterChunk();
        widget->setName(readWString());
        finishChunk();
        return true;
    } else if (chunk == "Widget.caption") {
        enterChunk();
        widget->setCaption(readWString(), false);
        finishChunk();
        return true;
    } else if (chunk == "Widget.innerBindPos") {
        enterChunk();
        widget->m_innerBindPos = readValue<Vector<2, float>>();
        finishChunk();
        return true;
    } else if (chunk == "Widget.innerBindSize") {
        enterChunk();
        widget->m_innerBindSize = readValue<Vector<2, float>>();
        finishChunk();
        return true;
    } else if (chunk == "Widget.bindPos") {
        enterChunk();
        widget->m_bindPos = readValue<Vector<2, float>>();
        finishChunk();
        return true;
    } else if (chunk == "Widget.bindSize") {
        enterChunk();
        widget->m_bindSize = readValue<Vector<2, float>>();
        finishChunk();
        return true;
    } else if (chunk == "Widget.framePos") {
        enterChunk();
        widget->m_framePos = readValue<Vector<2, float>>();
        finishChunk();
        return true;
    } else if (chunk == "Widget.frameSize") {
        enterChunk();
        widget->m_frameSize = readValue<Vector<2, float>>();
        finishChunk();
        return true;
    } else if (chunk == "Widget.bindMatrix") {
        enterChunk();
        widget->setBindMatrix(readValue<Matrix<float>>());
        finishChunk();
        return true;
    } else if (chunk == "Widget.horizontalAlignment") {
        enterChunk();
        widget->m_horizontalAlignment = readValue<int>();
        finishChunk();
        return true;
    } else if (chunk == "Widget.verticalAlignment") {
        enterChunk();
        widget->m_verticalAlignment = readValue<int>();
        finishChunk();
        return true;
    } else if (chunk == "Widget.flags") {
        enterChunk();
        widget->m_flags = readValue<int>();
        finishChunk();
        return true;
    } else {
        return false;
    }
}

// Inflates compressed texture pixels into a byte vector using the zlib streaming loop.
bool decompressPLX(const std::vector<unsigned char> &compressed, std::vector<unsigned char> &output)
{
    const int kChunkSize = 128000;
    unsigned char chunk[kChunkSize];
    z_stream stream;

    if (compressed.empty()) {
        return false;
    }
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;
    if (inflateInit(&stream) != Z_OK) {
        return false;
    }
    stream.avail_in = (uInt)compressed.size();
    stream.next_in = (Bytef *)&compressed[0];
    int status;
    do {
        stream.avail_out = kChunkSize;
        stream.next_out = chunk;
        status = inflate(&stream, Z_NO_FLUSH);
        switch (status) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd(&stream);
            return false;
        }
        size_t produced = kChunkSize - stream.avail_out;
        unsigned int previousSize = (unsigned int)output.size();
        output.resize(output.size() + produced);
        memcpy(&output[previousSize], chunk, produced);
    } while (status != Z_STREAM_END);
    inflateEnd(&stream);
    return status == Z_STREAM_END;
}

}
