#include "plasma/PLXStreamWriter.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "EditableMesh.h"
#include "plasma/SmoothMeshShape.h"
#include <zlib.h>

namespace plasma {

// Returns a node's variable keys by value.
std::list<std::string> nodeVariableKeys(const Node *node);

static const int CompressionCapacityOverhead = 12;

PLXStreamWriter::PLXStreamWriter(Engine *engine, Engine::Page *page, Node *root, std::ostream &stream,
                                 unsigned __int64 key)
    : StreamWriter(stream, key),
      m_engine(engine),
      m_page(page),
      m_root(root)
{
    if (!m_root)
        m_root = engine->m_root;
}

void PLXStreamWriter::collectNodes(Node *node, std::list<Node *> &nodes)
{
    if (node->isShown() && !node->isInternal()) {
        nodes.push_back(node);
        for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
            collectNodes(*it, nodes);
    }
}

void compressPLX(std::vector<char> &output, const std::vector<unsigned char> &input)
{
    unsigned inputSize = input.size();
    uLongf length = static_cast<uLongf>(static_cast<double>(inputSize) * 1.1) + CompressionCapacityOverhead;
    output.resize(length);

    // The code ignores compress's status and resizes to the length it returned.
    compress(reinterpret_cast<Bytef *>(&output[0]), &length, reinterpret_cast<const Bytef *>(&input[0]), inputSize);
    output.resize(length);
}

void PLXStreamWriter::writeTexture(Texture *texture)
{
    beginChunk("Texture", true);
    beginChunk("Texture.wname", true);
    writeWString(texture->getName());
    endChunk();
    beginChunk("Texture.id", true);
    writeInt(texture->m_id);
    endChunk();
    beginChunk("Texture.format.pixelFormat", true);
    write4(&texture->m_format.pixelFormat);
    endChunk();
    beginChunk("Texture.format.minFilter", true);
    write4(&texture->m_format.minFilter);
    endChunk();
    beginChunk("Texture.format.maxFilter", true);
    write4(&texture->m_format.maxFilter);
    endChunk();
    beginChunk("Texture.format.horizontalWrap", true);
    write4(&texture->m_format.horizontalWrap);
    endChunk();
    beginChunk("Texture.format.verticalWrap", true);
    write4(&texture->m_format.verticalWrap);
    endChunk();
    beginChunk("Texture.width", true);
    writeInt(texture->m_width);
    endChunk();
    beginChunk("Texture.height", true);
    writeInt(texture->m_height);
    endChunk();
    const std::vector<unsigned char> &pixels = texture->m_pixels;
    if (pixels.size()) {
        beginChunk("Texture.compressedPixels", true);
        std::vector<char> compressed;
        compressPLX(compressed, pixels);
        unsigned count = compressed.size();
        writeInt(count);
        m_stream->write(&compressed[0], count);
        endChunk();
    }
    endChunk();
}

void PLXStreamWriter::writeSequences(Attribute *attribute)
{
    const std::map<std::wstring, Sequence *> &sequences = attribute->getSequences();
    for (std::map<std::wstring, Sequence *>::const_iterator it = sequences.begin(); it != sequences.end(); ++it) {
        Sequence *sequence = it->second;
        beginChunk(std::string("Attribute.sequence"), true);
        beginChunk(std::string("Attribute.sequence.wname"), true);
        writeWString(it->first);
        endChunk();
        for (int i = 0; i < sequence->count(); ++i) {
            beginChunk(std::string("Attribute.sequence.key"), true);
            beginChunk(std::string("Attribute.sequence.key.frame"), true);
            write4(&(*sequence)[i].element);
            endChunk();
            beginChunk(std::string("Attribute.sequence.key.time"), true);
            write4(&(*sequence)[i].frame);
            endChunk();
            beginChunk(std::string("Attribute.sequence.key.smoothness"), true);
            write4(&(*sequence)[i].smoothness);
            endChunk();
            endChunk();
        }
        endChunk();
    }
}

void PLXStreamWriter::writeTextShape(Shape *base)
{
    TextShape *shape = static_cast<TextShape *>(base);
    beginChunk("TextShape", true);
    beginChunk("TextShape.wname", true);
    writeWString(shape->getName());
    endChunk();
    beginChunk("TextShape.string", true);
    for (int i = 0; i < shape->m_string.size(); ++i) {
        beginChunk("Attribute.frame", true);
        writeWString(shape->m_string.m_values[i]);
        endChunk();
    }
    writeSequences(&shape->m_string);
    endChunk();
    beginChunk("TextShape.color", true);
    writeContinuousVector4(&shape->m_color);
    endChunk();
    beginChunk("TextShape.strokeColor", true);
    writeContinuousVector4(&shape->m_strokeColor);
    endChunk();
    beginChunk("TextShape.extrusionColor", true);
    writeContinuousVector4(&shape->m_extrusionColor);
    endChunk();
    beginChunk("TextShape.flags", true);
    writeInt(shape->m_flags);
    endChunk();
    beginChunk("TextShape.wfontName", true);
    writeWString(shape->m_fontName);
    endChunk();
    beginChunk("TextShape.pixelSize", true);
    write4(&shape->m_pixelSize);
    endChunk();
    beginChunk("TextShape.strokeRadius", true);
    write4(&shape->m_strokeRadius);
    endChunk();
    beginChunk("TextShape.spacing", true);
    write4(&shape->m_spacing);
    endChunk();
    beginChunk("TextShape.lineSpacing", true);
    write4(&shape->m_lineSpacing);
    endChunk();
    beginChunk("TextShape.wrapWidth", true);
    write4(&shape->m_wrapWidth);
    endChunk();
    endChunk();
}

void PLXStreamWriter::writeSmoothMeshShape(Shape *base)
{
    SmoothMeshShape *shape = static_cast<SmoothMeshShape *>(base);
    beginChunk("SmoothMeshShape", true);
    beginChunk("SmoothMeshShape.wname", true);
    writeWString(shape->getName());
    endChunk();
    const std::vector<std::vector<int>> &faces = shape->m_faces;
    for (int i = 0; i < (int)faces.size(); ++i) {
        if (faces[i].empty())
            continue;
        beginChunk("SmoothMeshShape.face", true);
        int count = faces[i].size();
        write4(&count);
        m_stream->write(reinterpret_cast<const char *>(&faces[i][0]), 4 * count);
        endChunk();
    }
    beginChunk("SmoothMeshShape.vertexFlags", true);
    const std::vector<int> &values = shape->m_vertexFlags;
    int count = values.size();
    write4(&count);
    if (count)
        m_stream->write(reinterpret_cast<const char *>(&values[0]), 4 * count);
    endChunk();
    beginChunk("SmoothMeshShape.vertexParameters", true);
    const std::vector<float> &parameters = shape->m_vertexParameters;
    int parameterCount = parameters.size();
    write4(&parameterCount);
    if (parameterCount)
        m_stream->write(reinterpret_cast<const char *>(&parameters[0]), 4 * parameterCount);
    endChunk();
    beginChunk("SmoothMeshShape.vertexPositions", true);
    writeArrayVector2(&shape->m_vertexPositions);
    endChunk();
    beginChunk("SmoothMeshShape.vertexTexCoords", true);
    writeArrayVector2(&shape->m_vertexTexCoords);
    endChunk();
    beginChunk("SmoothMeshShape.vertexColors", true);
    writeArrayVector4(&shape->m_vertexColors);
    endChunk();
    beginChunk("SmoothMeshShape.strokeColors", true);
    writeArrayVector4(&shape->m_strokeColors);
    endChunk();
    beginChunk("SmoothMeshShape.extrusionFrontColors", true);
    writeArrayVector4(&shape->m_extrusionFrontColors);
    endChunk();
    beginChunk("SmoothMeshShape.extrusionBackColors", true);
    writeArrayVector4(&shape->m_extrusionBackColors);
    endChunk();
    beginChunk("SmoothMeshShape.strokeWidths", true);
    writeArrayFloat(&shape->m_strokeWidths);
    endChunk();
    beginChunk("SmoothMeshShape.texture", true);
    writeDiscreteInt(&shape->m_texture);
    endChunk();
    beginChunk("SmoothMeshShape.strokeTexture", true);
    writeDiscreteInt(&shape->m_strokeTexture);
    endChunk();
    beginChunk("SmoothMeshShape.textureTranslation", true);
    writeContinuousVector2(&shape->m_textureTranslation);
    endChunk();
    beginChunk("SmoothMeshShape.textureRotation", true);
    writeContinuousVector3(&shape->m_textureRotation);
    endChunk();
    beginChunk("SmoothMeshShape.textureDeformation", true);
    writeContinuousMatrix(&shape->m_textureDeformation);
    endChunk();
    beginChunk("SmoothMeshShape.texturePivot", true);
    writeContinuousVector2(&shape->m_texturePivot);
    endChunk();
    beginChunk("SmoothMeshShape.textureOpacity", true);
    writeContinuousFloat(&shape->m_textureOpacity);
    endChunk();
    beginChunk("SmoothMeshShape.textureBrightness", true);
    writeContinuousFloat(&shape->m_textureBrightness);
    endChunk();
    beginChunk("SmoothMeshShape.textureContrast", true);
    writeContinuousFloat(&shape->m_textureContrast);
    endChunk();
    beginChunk("SmoothMeshShape.textureSaturation", true);
    writeContinuousFloat(&shape->m_textureSaturation);
    endChunk();
    beginChunk("SmoothMeshShape.strokeTextureOpacity", true);
    writeContinuousFloat(&shape->m_strokeTextureOpacity);
    endChunk();
    beginChunk("SmoothMeshShape.strokeTextureBrightness", true);
    writeContinuousFloat(&shape->m_strokeTextureBrightness);
    endChunk();
    beginChunk("SmoothMeshShape.strokeTextureContrast", true);
    writeContinuousFloat(&shape->m_strokeTextureContrast);
    endChunk();
    beginChunk("SmoothMeshShape.strokeTextureSaturation", true);
    writeContinuousFloat(&shape->m_strokeTextureSaturation);
    endChunk();
    beginChunk("SmoothMeshShape.strokeTextureStretch", true);
    writeContinuousFloat(&shape->m_strokeTextureStretch);
    endChunk();
    beginChunk("SmoothMeshShape.extrusionMatrix", true);
    writeContinuousMatrix(&shape->m_extrusionMatrix);
    endChunk();
    beginChunk("SmoothMeshShape.subdivisions", true);
    write4(&shape->m_subdivisions);
    endChunk();
    beginChunk("SmoothMeshShape.flags", true);
    write4(&shape->m_flags);
    endChunk();
    beginChunk("SmoothMeshShape.strokeJointType", true);
    write4(&shape->m_strokeJointType);
    endChunk();
    beginChunk("SmoothMeshShape.strokeCapType", true);
    write4(&shape->m_strokeCapType);
    endChunk();
    beginChunk("SmoothMeshShape.strokeAlignment", true);
    write4(&shape->m_strokeAlignment);
    endChunk();
    beginChunk("SmoothMeshShape.strokePattern", true);
    write4(&shape->m_strokePattern);
    endChunk();
    beginChunk("SmoothMeshShape.strokeDash", true);
    write4(&shape->m_strokeDash);
    endChunk();
    beginChunk("SmoothMeshShape.strokeGap", true);
    write4(&shape->m_strokeGap);
    endChunk();
    beginChunk("SmoothMeshShape.smoothWeight", true);
    write4(&shape->m_smoothWeight);
    endChunk();
    endChunk();
}

void PLXStreamWriter::writeGenericShape(GenericShape *shape)
{
    beginChunk(std::string("GenericShape"), true);
    beginChunk(std::string("GenericShape.wname"), true);
    writeWString(shape->getName());
    endChunk();
    beginChunk(std::string("GenericShape.wsource"), true);
    writeWString(shape->m_source);
    endChunk();
    beginChunk(std::string("GenericShape.position"), true);
    writeVector2(shape->m_position);
    endChunk();
    beginChunk(std::string("GenericShape.size"), true);
    writeVector2(shape->m_size);
    endChunk();
    endChunk();
}

void PLXStreamWriter::writeTransformation(Transformation *transformation)
{
    beginChunk(std::string("Transformation"), true);
    beginChunk(std::string("Transformation.wname"), true);
    writeWString(transformation->getName());
    endChunk();
    beginChunk(std::string("Transformation.translation"), true);
    writeContinuousVector2(&transformation->m_translation);
    endChunk();
    beginChunk(std::string("Transformation.rotation"), true);
    writeContinuousVector3(&transformation->m_rotation);
    endChunk();
    beginChunk(std::string("Transformation.pivot"), true);
    writeContinuousVector2(&transformation->m_pivot);
    endChunk();
    beginChunk(std::string("Transformation.deformation"), true);
    writeContinuousMatrix(&transformation->m_deformation);
    endChunk();
    endChunk();
}

void PLXStreamWriter::writeDisplay(Display *display)
{
    beginChunk(std::string("Display"), true);
    beginChunk(std::string("Display.wname"), true);
    writeWString(display->getName());
    endChunk();
    beginChunk(std::string("Display.visibility"), true);
    writeDiscreteInt(&display->m_visibility);
    endChunk();
    beginChunk(std::string("Display.clipping"), true);
    writeDiscreteInt(&display->m_clipping);
    endChunk();
    beginChunk(std::string("Display.fillColor"), true);
    writeContinuousVector4(&display->m_fillColor);
    endChunk();
    beginChunk(std::string("Display.strokeColor"), true);
    writeContinuousVector4(&display->m_strokeColor);
    endChunk();
    beginChunk(std::string("Display.blurRadius"), true);
    writeContinuousFloat(&display->m_blurRadius);
    endChunk();
    beginChunk(std::string("Display.flags"), true);
    write4(&display->m_flags);
    endChunk();
    endChunk();
}

bool PLXStreamWriter::writeShape(Shape *shape)
{
    if (shape) {
        int type = shape->m_type;
        if (type == SHAPE_SMOOTH_MESH) {
            writeSmoothMeshShape(shape);
            return true;
        }
        if (type == SHAPE_TEXT) {
            writeTextShape(shape);
            return true;
        }
        if (type == SHAPE_GENERIC) {
            writeGenericShape(static_cast<GenericShape *>(shape));
            return true;
        }
    }
    return false;
}

void PLXStreamWriter::writeScrollButton(Widget *widget)
{
    writeButton(static_cast<Button *>(widget));
    beginChunk("ScrollButton.direction", true);
    writeInt(static_cast<ScrollButton *>(widget)->m_direction);
    endChunk();
}

void PLXStreamWriter::writeScrollSlider(Widget *widget)
{
    writeButton(static_cast<Button *>(widget));
    beginChunk("ScrollSlider.direction", true);
    writeInt(static_cast<ScrollSlider *>(widget)->m_direction);
    endChunk();
}

void PLXStreamWriter::writePopUpButton(Widget *widget)
{
    writeButton(static_cast<Button *>(widget));
}

void PLXStreamWriter::writeButton(Button *button)
{
    writeWidget(button);
    beginChunk("Button.type", true);
    writeInt(button->m_type);
    endChunk();
}

void PLXStreamWriter::writeListWidget(Widget *widget)
{
    writeWidget(widget);
}

void PLXStreamWriter::writeWidget(Widget *widget)
{
    beginChunk("Widget.wname", true);
    writeWString(widget->getName());
    endChunk();
    beginChunk("Widget.caption", true);
    writeWString(widget->m_caption);
    endChunk();
    beginChunk("Widget.innerBindPos", true);
    {
        Vector<2, float> position = widget->getInnerBindPos();
        writeVector2(position);
    }
    endChunk();
    beginChunk("Widget.innerBindSize", true);
    {
        Vector<2, float> size = widget->getInnerBindSize();
        writeVector2(size);
    }
    endChunk();
    beginChunk("Widget.bindPos", true);
    {
        Vector<2, float> origin = widget->getBindPos();
        writeVector2(origin);
    }
    endChunk();
    beginChunk("Widget.bindSize", true);
    {
        Vector<2, float> extent = widget->getBindSize();
        writeVector2(extent);
    }
    endChunk();
    beginChunk("Widget.framePos", true);
    {
        Vector<2, float> origin = widget->getFramePos();
        writeVector2(origin);
    }
    endChunk();
    beginChunk("Widget.frameSize", true);
    {
        Vector<2, float> extent = widget->getFrameSize();
        writeVector2(extent);
    }
    endChunk();
    beginChunk("Widget.bindMatrix", true);
    writeMatrix(widget->m_bindMatrix);
    endChunk();
    beginChunk("Widget.horizontalAlignment", true);
    writeInt(widget->m_horizontalAlignment);
    endChunk();
    beginChunk("Widget.verticalAlignment", true);
    writeInt(widget->m_verticalAlignment);
    endChunk();
    beginChunk("Widget.flags", true);
    writeInt(widget->m_flags);
    endChunk();
}

void PLXStreamWriter::writeWidgetObject(Widget *widget)
{
    if (dynamic_cast<ListWidget *>(widget)) {
        beginChunk("ListWidget", true);
        writeListWidget(dynamic_cast<ListWidget *>(widget));
    } else if (dynamic_cast<ScrollButton *>(widget)) {
        beginChunk("ScrollButton", true);
        writeScrollButton(dynamic_cast<ScrollButton *>(widget));
    } else if (dynamic_cast<ScrollSlider *>(widget)) {
        beginChunk("ScrollSlider", true);
        writeScrollSlider(dynamic_cast<ScrollSlider *>(widget));
    } else if (dynamic_cast<PopUpButton *>(widget)) {
        beginChunk("PopUpButton", true);
        writePopUpButton(dynamic_cast<PopUpButton *>(widget));
    } else if (widget && widget->m_kind == WIDGET_BUTTON) {
        beginChunk("Button", true);
        writeButton(Button::cast(widget));
    } else if (widget && widget->m_kind == WIDGET_EDIT) {
        beginChunk("Edit", true);
        writeListWidget(widget && widget->m_kind == WIDGET_EDIT ? widget : 0);
    } else {
        beginChunk("Widget", true);
        writeWidget(widget);
    }
    endChunk();
}

void PLXStreamWriter::write()
{
    beginChunk("PlasmaGraphics", false);
    static const int fileVersion = 1;
    write4(&fileVersion);
    endChunk();
    if (m_obfuscationKey) {
        beginChunk("Seal", false);
        std::string signature("PlasmaGraphics");
        int length = signature.size();
        std::vector<char> sealed(signature.begin(), signature.end());
        obfuscate(&sealed[0], length, m_obfuscationKey);
        write4(&length);
        m_stream->write(&sealed[0], length);
        endChunk();
    }
    std::list<Node *> nodes;
    collectNodes(m_root, nodes);

    // Collects objects used by nodes, keyed by pointer.
    std::set<void *> shapes;
    std::set<void *> transformations;
    std::set<void *> displays;
    std::set<void *> widgets;
    std::set<void *> textures;
    for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        Node *node = *it;
        if (node->m_shape)
            shapes.insert(node->m_shape);
        if (node->m_transformation)
            transformations.insert(node->m_transformation);
        if (node->display())
            displays.insert(node->display());
        if (node->m_widget)
            widgets.insert(node->m_widget);
        Shape *shape = node->m_shape;
        if (shape && shape->getType() == SHAPE_SMOOTH_MESH) {
            DiscreteAttribute<int> *texture = &static_cast<SmoothMeshShape *>(shape)->m_texture;
            for (int i = 0; i < texture->size(); ++i) {
                Texture *image = m_engine->findTexture(texture->m_values[i]);
                if (image)
                    textures.insert(image);
            }
            DiscreteAttribute<int> *strokeTexture = &static_cast<SmoothMeshShape *>(shape)->m_strokeTexture;
            for (int i = 0; i < strokeTexture->size(); ++i) {
                Texture *image = m_engine->findTexture(strokeTexture->m_values[i]);
                if (image)
                    textures.insert(image);
            }
        }
    }
    for (std::set<void *>::iterator it = textures.begin(); it != textures.end(); ++it) {
        Texture *texture = static_cast<Texture *>(*it);
        if (!texture->m_transient)
            writeTexture(texture);
    }
    std::map<Shape *, int> shapeIds;
    shapeIds[0] = -1;
    const std::list<Shape *> &engineShapes = m_engine->m_shapes;
    int index = 0;
    for (std::list<Shape *>::const_iterator it = engineShapes.begin(); it != engineShapes.end(); ++it) {
        Shape *object = *it;
        if (shapes.find(object) != shapes.end()) {
            if (!writeShape(*it))
                shapeIds[*it] = -1;
            else
                shapeIds[*it] = index++;
        }
    }
    std::map<Transformation *, int> transformationIds;
    transformationIds[0] = -1;
    const std::list<Transformation *> &engineTransformations = m_engine->m_transformations;
    index = 0;
    for (std::list<Transformation *>::const_iterator it = engineTransformations.begin();
         it != engineTransformations.end(); ++it) {
        Transformation *object = *it;
        if (transformations.find(object) != transformations.end()) {
            writeTransformation(*it);
            transformationIds[*it] = index++;
        }
    }
    std::map<Display *, int> displayIds;
    displayIds[0] = -1;
    const std::list<Display *> &engineDisplays = m_engine->m_displays;
    index = 0;
    for (std::list<Display *>::const_iterator it = engineDisplays.begin(); it != engineDisplays.end(); ++it) {
        Display *object = *it;
        if (displays.find(object) != displays.end()) {
            writeDisplay(*it);
            displayIds[*it] = index++;
        }
    }
    m_widgetIds[0] = -1;
    const std::list<Widget *> &engineWidgets = m_engine->m_widgets;
    index = 0;
    for (std::list<Widget *>::const_iterator it = engineWidgets.begin(); it != engineWidgets.end(); ++it) {
        Widget *object = *it;
        if (widgets.find(object) != widgets.end())
            m_widgetIds[*it] = index++;
    }
    for (std::list<Widget *>::const_iterator it = engineWidgets.begin(); it != engineWidgets.end(); ++it) {
        if (widgets.find(*it) != widgets.end())
            writeWidgetObject(*it);
    }
    std::map<Node *, int> nodeIds;
    nodeIds[0] = -1;
    index = 0;
    for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (!((*it)->m_flags & (1 << NODE_HIDDEN)) && !((*it)->m_flags & (1 << NODE_INTERNAL)))
            nodeIds[*it] = index++;
    }
    for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        Node *node = *it;
        if (node->isShown() && !node->isInternal()) {
            beginChunk("Node", true);
            beginChunk("Node.wname", true);
            writeWString(node->getName());
            endChunk();
            beginChunk("Node.shape", true);
            {
                Shape *shape = node->m_shape;
                write4(&shapeIds[shape]);
            }
            endChunk();
            beginChunk("Node.transformation", true);
            {
                Transformation *transformation = node->m_transformation;
                write4(&transformationIds[transformation]);
            }
            endChunk();
            beginChunk("Node.display", true);
            {
                Display *display = node->display();
                write4(&displayIds[display]);
            }
            endChunk();
            beginChunk("Node.widget", true);
            {
                Widget *widget = node->m_widget;
                write4(&m_widgetIds[widget]);
            }
            endChunk();
            for (std::list<Node *>::iterator child = node->m_children.begin(); child != node->m_children.end();
                 ++child) {
                if (!((*child)->m_flags & (1 << NODE_HIDDEN)) && !((*child)->m_flags & (1 << NODE_INTERNAL))) {
                    beginChunk("Node.child", true);
                    write4(&nodeIds[*child]);
                    endChunk();
                }
            }
            beginChunk("Node.flags", true);
            writeInt(node->m_flags);
            endChunk();
            std::list<std::string> keys = nodeVariableKeys(node);
            for (std::list<std::string>::iterator key = keys.begin(); key != keys.end(); ++key) {
                beginChunk("Node.variable", true);
                writeString(*key);
                writeString(node->getVariable(*key));
                endChunk();
            }
            endChunk();
        }
    }
    beginChunk("pageWidth", true);
    write4(&m_page->size.m_elements[0]);
    endChunk();
    beginChunk("pageHeight", true);
    write4(&m_page->size.m_elements[1]);
    endChunk();
    beginChunk("dpi", true);
    write4(&m_page->size.m_elements[2]);
    endChunk();
    beginChunk("unit", true);
    write4(&m_page->unit);
    endChunk();
    beginChunk("pageColor", true);
    writeVector4(m_page->color);
    endChunk();
}

std::ostream &StreamWriter::write4(const void *value)
{
    return m_stream->write(static_cast<const char *>(value), 4);
}

void PLXStreamWriter::writeContinuousVector4(Attribute *attribute)
{
    for (int i = 0; i < attribute->size(); ++i) {
        beginChunk(std::string("Attribute.frame"), true);
        writeVector4(*static_cast<Vector<4, float> *>(
            &static_cast<ContinuousAttribute<Vector<4, float>> *>(attribute)->m_values[i]));
        endChunk();
    }
    writeSequences(attribute);
}

void PLXStreamWriter::writeArrayVector2(ContinuousArrayAttribute<Vector<2, float>> *attribute)
{
    {
        beginChunk(std::string("ArrayAttribute.size"), true);
        int size = static_cast<int>((*attribute->m_values.begin())->size());
        write4(&size);
        endChunk();
    }
    if (static_cast<int>((*attribute->m_values.begin())->size()) > 0) {
        for (int i = 0; i < attribute->size(); ++i) {
            beginChunk(std::string("ArrayAttribute.frame"), true);
            m_stream->write(reinterpret_cast<const char *>(&(*attribute->m_values[i])[0]),
                            8 * (*attribute->m_values.begin())->size());
            endChunk();
        }
    }
    writeSequences(attribute);
}

void PLXStreamWriter::writeArrayVector4(ContinuousArrayAttribute<Vector<4, float>> *attribute)
{
    {
        beginChunk(std::string("ArrayAttribute.size"), true);
        int size = static_cast<int>((*attribute->m_values.begin())->size());
        write4(&size);
        endChunk();
    }
    if (static_cast<int>((*attribute->m_values.begin())->size()) > 0) {
        for (int i = 0; i < attribute->size(); ++i) {
            beginChunk(std::string("ArrayAttribute.frame"), true);
            m_stream->write(reinterpret_cast<const char *>(&(*attribute->m_values[i])[0]),
                            16 * (*attribute->m_values.begin())->size());
            endChunk();
        }
    }
    writeSequences(attribute);
}

void PLXStreamWriter::writeArrayFloat(ContinuousArrayAttribute<float> *attribute)
{
    {
        beginChunk(std::string("ArrayAttribute.size"), true);
        int size = static_cast<int>((*attribute->m_values.begin())->size());
        write4(&size);
        endChunk();
    }
    if (static_cast<int>((*attribute->m_values.begin())->size()) > 0) {
        for (int i = 0; i < attribute->size(); ++i) {
            beginChunk(std::string("ArrayAttribute.frame"), true);
            m_stream->write(reinterpret_cast<const char *>(&(*attribute->m_values[i])[0]),
                            4 * (*attribute->m_values.begin())->size());
            endChunk();
        }
    }
    writeSequences(attribute);
}

void PLXStreamWriter::writeDiscreteInt(Attribute *attribute)
{
    for (int i = 0; i < attribute->size(); ++i) {
        beginChunk(std::string("Attribute.frame"), true);
        write4(&static_cast<DiscreteAttribute<int> *>(attribute)->m_values[i]);
        endChunk();
    }
    writeSequences(attribute);
}

void PLXStreamWriter::writeContinuousVector2(Attribute *attribute)
{
    for (int i = 0; i < attribute->size(); ++i) {
        beginChunk(std::string("Attribute.frame"), true);
        writeVector2(*static_cast<Vector<2, float> *>(
            &static_cast<ContinuousAttribute<Vector<2, float>> *>(attribute)->m_values[i]));
        endChunk();
    }
    writeSequences(attribute);
}

void PLXStreamWriter::writeContinuousVector3(Attribute *attribute)
{
    for (int i = 0; i < attribute->size(); ++i) {
        beginChunk(std::string("Attribute.frame"), true);
        writeVector3(*static_cast<Vector<3, float> *>(
            &static_cast<ContinuousAttribute<Vector<3, float>> *>(attribute)->m_values[i]));
        endChunk();
    }
    writeSequences(attribute);
}

void PLXStreamWriter::writeContinuousMatrix(Attribute *attribute)
{
    for (int i = 0; i < attribute->size(); ++i) {
        beginChunk(std::string("Attribute.frame"), true);
        writeMatrix(
            *static_cast<Matrix<float> *>(&static_cast<ContinuousAttribute<Matrix<float>> *>(attribute)->m_values[i]));
        endChunk();
    }
    writeSequences(attribute);
}

void PLXStreamWriter::writeContinuousFloat(Attribute *attribute)
{
    for (int i = 0; i < attribute->size(); ++i) {
        beginChunk(std::string("Attribute.frame"), true);
        write4(&static_cast<ContinuousAttribute<float> *>(attribute)->m_values[i]);
        endChunk();
    }
    writeSequences(attribute);
}

std::ostream &PLXStreamWriter::writeVector2(const Vector<2, float> &value)
{
    return m_stream->write(reinterpret_cast<const char *>(&value), 8);
}

std::ostream &PLXStreamWriter::writeMatrix(const Matrix<float> &value)
{
    return m_stream->write(reinterpret_cast<const char *>(&value), 64);
}

std::ostream &PLXStreamWriter::writeVector4(const Vector<4, float> &value)
{
    return m_stream->write(reinterpret_cast<const char *>(&value), 16);
}

std::ostream &PLXStreamWriter::writeVector3(const Vector<3, float> &value)
{
    return m_stream->write(reinterpret_cast<const char *>(&value), 12);
}

}
