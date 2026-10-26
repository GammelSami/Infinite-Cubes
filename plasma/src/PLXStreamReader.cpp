#include "plasma/PLXStreamReader.h"
#include "plasma/PLXStreamWriter.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "EditableMesh.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/StaticMeshShape.h"
#include <cstring>

#include <zlib.h>

namespace plasma {

// Legacy font-reference record shared with Engine.cpp.
struct PLAFontReference {
    PLAFontReference()
        : size(0),
          stroke(0.0f),
          id(-1)
    {}

    std::string name;
    std::string source;
    int size;
    float stroke;
    int id;
};

static const int MaximumSupportedPLXVersion = 1;

InvalidFileFormatException::InvalidFileFormatException(const std::string &fileName)
    : IOException(fileName, fileName + " has invalid format.")
{}

InvalidVersionException::InvalidVersionException(const std::string &fileName)
    : IOException(fileName, fileName + " has invalid version.")
{}

InvalidDemoLicenseException::InvalidDemoLicenseException(const std::string &fileName)
    : IOException(fileName, fileName + " was created with a different demo license.")
{}

PLXStreamReader::PLXStreamReader(Engine *engine, Engine::Page *page, Node *target, int flags, std::istream &stream,
                                 const std::vector<unsigned __int64> &keys)
    : ChunkStreamReader(stream),
      m_flags(flags),
      m_keys(keys),
      m_engine(engine),
      m_page(page),
      m_target(target)
{}

bool decompressPLX(const std::vector<char> &compressed, std::vector<char> &output)
{
    if (compressed.empty())
        return false;
    z_stream stream;
    stream.zalloc = 0;
    stream.zfree = 0;
    stream.opaque = 0;
    stream.avail_in = 0;
    stream.next_in = 0;
    if (inflateInit_(&stream, ZLIB_VERSION, sizeof(stream)))
        return false;
    stream.avail_in = compressed.size();
    stream.next_in = reinterpret_cast<unsigned char *>(const_cast<char *>(&compressed[0]));
    enum {
        inflateOutputChunkBytes = 128000
    };
    char buffer[inflateOutputChunkBytes];
    int status;
    do {
        stream.avail_out = sizeof(buffer);
        stream.next_out = reinterpret_cast<unsigned char *>(buffer);
        status = inflate(&stream, Z_NO_FLUSH);

        // Z_NEED_DICT, Z_DATA_ERROR or Z_MEM_ERROR (zlib's zpipe example).
        if (status >= Z_MEM_ERROR && (status <= Z_DATA_ERROR || status == Z_NEED_DICT)) {
            inflateEnd(&stream);
            return false;
        }
        unsigned previousSize = output.size();
        unsigned produced = sizeof(buffer) - stream.avail_out;
        output.resize(output.size() + produced);
        memcpy(&output[0] + previousSize, buffer, produced);

        // The code keeps retrying Z_BUF_ERROR, even for truncated input.
    } while (status != Z_STREAM_END);
    inflateEnd(&stream);
    return true;
}

Texture *PLXStreamReader::readTexture()
{
    enterChunk();
    std::wstring name;
    int id = -1;
    TextureFormat format;
    int width = 0;
    int height = 0;
    std::vector<char> pixels;
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
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
            id = readInt();
            finishChunk();
        } else if (chunk == "Texture.format.pixelFormat") {
            enterChunk();
            format.pixelFormat = readInt();
            finishChunk();
        } else if (chunk == "Texture.format.minFilter") {
            enterChunk();
            format.minFilter = readInt();
            finishChunk();
        } else if (chunk == "Texture.format.maxFilter") {
            enterChunk();
            format.maxFilter = readInt();
            finishChunk();
        } else if (chunk == "Texture.format.horizontalWrap") {
            enterChunk();
            format.horizontalWrap = readInt();
            finishChunk();
        } else if (chunk == "Texture.format.verticalWrap") {
            enterChunk();
            format.verticalWrap = readInt();
            finishChunk();
        } else if (chunk == "Texture.width") {
            enterChunk();
            width = readInt();
            finishChunk();
        } else if (chunk == "Texture.height") {
            enterChunk();
            height = readInt();
            finishChunk();
        } else if (chunk == "Texture.pixels") {
            enterChunk();
            int count = readInt();
            if (count > 0) {
                pixels.resize(count);
                m_stream->read(&pixels[0], count);
            }
            finishChunk();
        } else if (chunk == "Texture.compressedPixels") {
            enterChunk();
            int count = readInt();
            if (count > 0) {
                std::vector<char> compressed(count);
                m_stream->read(&compressed[0], count);
                decompressPLX(compressed, pixels);
            }
            finishChunk();
        } else {
            skipChunk();
        }
    }
    if (id != -1) {
        // The code also forms the element-zero pointer when the pixel vector is empty.
        Texture *texture = m_engine->createTexture(width, height, reinterpret_cast<const unsigned char *>(&pixels[0]),
                                                   format, name, false);
        m_textureIds[id] = texture->m_id;
        return texture;
    }
    return 0;
}

bool PLXStreamReader::readAttributeSequence(Attribute *attribute)
{
    enterChunk();
    Sequence *sequence = 0;
    bool finished = finishChunk();
    if (!finished) {
        do {
            {
                std::string chunk = parseChunkHeader();
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
                    float smoothness = 0.0f;
                    int time = 0;
                    int element = 0;
                    while (!finishChunk()) {
                        std::string keyChunk = parseChunkHeader();
                        if (keyChunk == "Attribute.sequence.key.frame") {
                            enterChunk();
                            element = readInt();
                            finishChunk();
                        } else if (keyChunk == "Attribute.sequence.key.time") {
                            enterChunk();
                            time = readInt();
                            finishChunk();
                        } else if (keyChunk == "Attribute.sequence.key.smoothness") {
                            enterChunk();
                            smoothness = readFloat();
                            finishChunk();
                        } else {
                            skipChunk();
                        }
                    }

                    // The code dereferences a null sequence if keys precede its name.
                    sequence->addKey(time, element, smoothness);
                } else {
                    skipChunk();
                }
            }
            finished = finishChunk();
        } while (!finished);
    }
    return finished;
}

Shape *PLXStreamReader::readTextShape()
{
    enterChunk();
    TextShape *shape = m_engine->createTextShape();
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
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
            DiscreteAttribute<std::wstring> *attribute = &shape->m_string;
            int frame = 0;
            while (!finishChunk()) {
                std::string frameChunk = parseChunkHeader();
                if (frameChunk == "Attribute.frame") {
                    enterChunk();
                    if (frame == attribute->size())
                        attribute->duplicateElement();
                    attribute->m_values[frame] = readWString();
                    ++frame;
                    finishChunk();
                } else if (frameChunk == "Attribute.sequence") {
                    readAttributeSequence(attribute);
                } else {
                    skipChunk();
                }
            }
        } else if (chunk == "TextShape.color") {
            readContinuousVector4(&shape->m_color);
        } else if (chunk == "TextShape.strokeColor") {
            readContinuousVector4(&shape->m_strokeColor);
        } else if (chunk == "TextShape.extrusionColor") {
            readContinuousVector4(&shape->m_extrusionColor);
        } else if (chunk == "TextShape.flags") {
            enterChunk();
            shape->m_flags = readInt();
            finishChunk();
        } else if (chunk == "TextShape.pixelSize") {
            enterChunk();
            shape->m_pixelSize = readFloat();
            finishChunk();
        } else if (chunk == "TextShape.strokeRadius") {
            enterChunk();
            shape->m_strokeRadius = readFloat();
            finishChunk();
        } else if (chunk == "TextShape.spacing") {
            enterChunk();
            shape->m_spacing = readFloat();
            finishChunk();
        } else if (chunk == "TextShape.lineSpacing") {
            enterChunk();
            shape->m_lineSpacing = readFloat();
            finishChunk();
        } else if (chunk == "TextShape.wrapWidth") {
            enterChunk();
            shape->m_wrapWidth = readFloat();
            finishChunk();
        } else if (chunk == "TextShape.fontSize") {
            enterChunk();
            shape->m_pixelSize = static_cast<float>(readInt());
            finishChunk();
        } else if (chunk == "TextShape.strokeWidth") {
            enterChunk();
            shape->m_strokeRadius = readInt() * 0.5;
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
                std::string fontChunk = parseChunkHeader();
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
                    pixelSize = static_cast<float>(readInt());
                    finishChunk();
                } else if (fontChunk == "TextShape.font.glowRadius") {
                    enterChunk();
                    strokeRadius = readInt() * 0.5;
                    finishChunk();
                } else if (fontChunk == "TextShape.font.size") {
                    enterChunk();
                    pixelSize = readFloat();
                    finishChunk();
                } else if (fontChunk == "TextShape.font.strokeWidth") {
                    enterChunk();
                    strokeRadius = readFloat() * 0.5;
                    finishChunk();
                } else if (fontChunk == "TextShape.font.strokeGlow" || fontChunk == "TextShape.font.pixelFont") {
                    enterChunk();
                    readInt();
                    finishChunk();
                }

                // The code does not skip unknown chunks inside the legacy font.
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

Shape *PLXStreamReader::readSmoothMeshShape()
{
    enterChunk();
    SmoothMeshShape *shape = m_engine->createSmoothMeshShape();
    std::vector<std::vector<int>> faces;
    float smoothWeight = shape->m_smoothWeight;
    int subdivisions = shape->m_subdivisions;
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
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
            int count = readInt();
            std::vector<int> face(count);
            m_stream->read(reinterpret_cast<char *>(&face[0]), count * sizeof(int));
            faces.push_back(face);
            finishChunk();
        } else if (chunk == "SmoothMeshShape.vertexFlags") {
            enterChunk();
            int count = readInt();
            if (count > 0) {
                std::vector<int> &values = shape->m_vertexFlags;
                values.resize(count);
                m_stream->read(reinterpret_cast<char *>(&values[0]), count * sizeof(int));
            }
            finishChunk();
        } else if (chunk == "SmoothMeshShape.vertexParameters") {
            enterChunk();
            int count = readInt();
            if (count > 0) {
                std::vector<float> &values = shape->m_vertexParameters;
                values.resize(count);
                m_stream->read(reinterpret_cast<char *>(&values[0]), count * sizeof(float));
            }
            finishChunk();
        } else if (chunk == "SmoothMeshShape.vertexPositions") {
            readArrayVector2(&shape->m_vertexPositions);
        } else if (chunk == "SmoothMeshShape.vertexTexCoords") {
            readArrayVector2(&shape->m_vertexTexCoords);
        } else if (chunk == "SmoothMeshShape.vertexColors") {
            readArrayVector4(&shape->m_vertexColors);
        } else if (chunk == "SmoothMeshShape.strokeColors") {
            readArrayVector4(&shape->m_strokeColors);
        } else if (chunk == "SmoothMeshShape.extrusionFrontColors") {
            readArrayVector4(&shape->m_extrusionFrontColors);
        } else if (chunk == "SmoothMeshShape.extrusionBackColors") {
            readArrayVector4(&shape->m_extrusionBackColors);
        } else if (chunk == "SmoothMeshShape.strokeWidths") {
            readArrayFloat(&shape->m_strokeWidths);
        } else if (chunk == "SmoothMeshShape.texture") {
            DiscreteAttribute<int> *attribute = &shape->m_texture;
            readDiscreteInt(attribute);
            for (int i = 0; i < attribute->size(); ++i) {
                int &id = shape->m_texture.m_values[i];
                id = m_textureIds[id];
            }
        } else if (chunk == "SmoothMeshShape.strokeTexture") {
            DiscreteAttribute<int> *attribute = &shape->m_strokeTexture;
            readDiscreteInt(attribute);
            for (int i = 0; i < attribute->size(); ++i) {
                int &id = shape->m_strokeTexture.m_values[i];
                id = m_textureIds[id];
            }
        } else if (chunk == "SmoothMeshShape.textureTranslation") {
            readContinuousVector2(&shape->m_textureTranslation);
        } else if (chunk == "SmoothMeshShape.textureRotation") {
            readContinuousVector3(&shape->m_textureRotation);
        } else if (chunk == "SmoothMeshShape.textureDeformation") {
            readContinuousMatrix(&shape->m_textureDeformation);
        } else if (chunk == "SmoothMeshShape.texturePivot") {
            readContinuousVector2(&shape->m_texturePivot);
        } else if (chunk == "SmoothMeshShape.textureOpacity") {
            readContinuousFloat(&shape->m_textureOpacity);
        } else if (chunk == "SmoothMeshShape.textureBrightness") {
            readContinuousFloat(&shape->m_textureBrightness);
        } else if (chunk == "SmoothMeshShape.textureContrast") {
            readContinuousFloat(&shape->m_textureContrast);
        } else if (chunk == "SmoothMeshShape.textureSaturation") {
            readContinuousFloat(&shape->m_textureSaturation);
        } else if (chunk == "SmoothMeshShape.strokeTextureOpacity") {
            readContinuousFloat(&shape->m_strokeTextureOpacity);
        } else if (chunk == "SmoothMeshShape.strokeTextureBrightness") {
            readContinuousFloat(&shape->m_strokeTextureBrightness);
        } else if (chunk == "SmoothMeshShape.strokeTextureContrast") {
            readContinuousFloat(&shape->m_strokeTextureContrast);
        } else if (chunk == "SmoothMeshShape.strokeTextureSaturation") {
            readContinuousFloat(&shape->m_strokeTextureSaturation);
        } else if (chunk == "SmoothMeshShape.strokeTextureStretch") {
            readContinuousFloat(&shape->m_strokeTextureStretch);
        } else if (chunk == "SmoothMeshShape.extrusionMatrix") {
            readContinuousMatrix(&shape->m_extrusionMatrix);
        } else if (chunk == "SmoothMeshShape.subdivisions") {
            enterChunk();
            subdivisions = readInt();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.smoothWeight") {
            enterChunk();
            smoothWeight = readFloat();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.flags") {
            enterChunk();
            shape->m_flags = readInt();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.strokeJointType") {
            enterChunk();
            shape->m_strokeJointType = readInt();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.strokeCapType") {
            enterChunk();
            shape->m_strokeCapType = readInt();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.strokeAlignment") {
            enterChunk();
            shape->m_strokeAlignment = readInt();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.strokePattern") {
            enterChunk();
            shape->m_strokePattern = readInt();
            finishChunk();
        } else if (chunk == "SmoothMeshShape.strokeDash") {
            enterChunk();
            shape->setStrokeDash(readFloat());
            finishChunk();
        } else if (chunk == "SmoothMeshShape.strokeGap") {
            enterChunk();
            shape->setStrokeGap(readFloat());
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

GenericShape *PLXStreamReader::readGenericShape()
{
    enterChunk();
    GenericShape *shape = m_engine->createGenericShape();
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
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
            shape->m_position = readVector2();
            finishChunk();
        } else if (chunk == "GenericShape.size") {
            enterChunk();
            shape->m_size = readVector2();
            finishChunk();
        } else {
            skipChunk();
        }
    }
    shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return shape;
}

Transformation *PLXStreamReader::readTransformation()
{
    enterChunk();
    Transformation *transformation = m_engine->createTransformation();
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
        if (chunk == "Transformation.name") {
            enterChunk();
            transformation->setName(widenString(readString()));
            finishChunk();
        } else if (chunk == "Transformation.wname") {
            enterChunk();
            transformation->setName(readWString());
            finishChunk();
        } else if (chunk == "Transformation.translation") {
            readContinuousVector2(&transformation->m_translation);
        } else if (chunk == "Transformation.rotation") {
            readContinuousVector3(&transformation->m_rotation);
        } else if (chunk == "Transformation.pivot") {
            readContinuousVector2(&transformation->m_pivot);
        } else if (chunk == "Transformation.deformation") {
            readContinuousMatrix(&transformation->m_deformation);
        } else {
            skipChunk();
        }
    }
    transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return transformation;
}

Display *PLXStreamReader::readDisplay()
{
    enterChunk();
    Display *display = m_engine->createDisplay();
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
        if (chunk == "Display.name") {
            enterChunk();
            display->setName(widenString(readString()));
            finishChunk();
        } else if (chunk == "Display.wname") {
            enterChunk();
            display->setName(readWString());
            finishChunk();
        } else if (chunk == "Display.visibility") {
            readDiscreteInt(&display->m_visibility);
        } else if (chunk == "Display.clipping") {
            readDiscreteInt(&display->m_clipping);
        } else if (chunk == "Display.strokeColor") {
            readContinuousVector4(&display->m_strokeColor);
        } else if (chunk == "Display.fillColor") {
            readContinuousVector4(&display->m_fillColor);
        } else if (chunk == "Display.blurRadius") {
            readContinuousFloat(&display->m_blurRadius);
        } else if (chunk == "Display.flags") {
            enterChunk();
            display->m_flags = readInt();
            finishChunk();
        } else {
            skipChunk();
        }
    }
    return display;
}

Widget *PLXStreamReader::readWidget()
{
    enterChunk();
    Widget *widget = m_engine->createWidget(0);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
        if (!readWidgetProperty(chunk, widget))
            skipChunk();
    }
    return widget;
}

Button *PLXStreamReader::readButton()
{
    enterChunk();
    Button *widget = m_engine->createButton(0);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
        if (!readButtonProperty(chunk, widget))
            skipChunk();
    }
    return widget;
}

Widget *PLXStreamReader::readListWidget()
{
    enterChunk();
    Widget *widget = m_engine->createListWidget(0);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
        if (!readListWidgetProperty(widget, chunk))
            skipChunk();
    }
    return widget;
}

Widget *PLXStreamReader::readEdit()
{
    enterChunk();
    Widget *widget = m_engine->createEdit(0);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
        if (!readListWidgetProperty(widget, chunk))
            skipChunk();
    }
    return widget;
}

Widget *PLXStreamReader::readScrollButton()
{
    enterChunk();
    Widget *widget = m_engine->createScrollButton(0);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
        if (!readScrollButtonProperty(chunk, widget))
            skipChunk();
    }
    return widget;
}

Widget *PLXStreamReader::readPopUpButton()
{
    enterChunk();
    Widget *widget = m_engine->createPopUpButton(0);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
        if (!readPopUpButtonProperty(chunk, widget))
            skipChunk();
    }
    return widget;
}

Widget *PLXStreamReader::readScrollSlider()
{
    enterChunk();
    Widget *widget = m_engine->createScrollSlider(0);
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
        if (!readScrollSliderProperty(chunk, widget))
            skipChunk();
    }
    return widget;
}

bool PLXStreamReader::readWidgetProperty(const std::string &chunk, Widget *widget)
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
        widget->m_innerBindPos = readVector2();
        finishChunk();
        return true;
    } else if (chunk == "Widget.innerBindSize") {
        enterChunk();
        widget->m_innerBindSize = readVector2();
        finishChunk();
        return true;
    } else if (chunk == "Widget.bindPos") {
        enterChunk();
        widget->m_bindPos = readVector2();
        finishChunk();
        return true;
    } else if (chunk == "Widget.bindSize") {
        enterChunk();
        widget->m_bindSize = readVector2();
        finishChunk();
        return true;
    } else if (chunk == "Widget.framePos") {
        enterChunk();
        widget->m_framePos = readVector2();
        finishChunk();
        return true;
    } else if (chunk == "Widget.frameSize") {
        enterChunk();
        widget->m_frameSize = readVector2();
        finishChunk();
        return true;
    } else if (chunk == "Widget.bindMatrix") {
        enterChunk();
        widget->m_inverseBindMatrix = widget->m_bindMatrix = readMatrix();
        widget->m_inverseBindMatrix.invert();
        finishChunk();
        return true;
    } else if (chunk == "Widget.horizontalAlignment") {
        enterChunk();
        widget->m_horizontalAlignment = readInt();
        finishChunk();
        return true;
    } else if (chunk == "Widget.verticalAlignment") {
        enterChunk();
        widget->m_verticalAlignment = readInt();
        finishChunk();
        return true;
    } else if (chunk == "Widget.flags") {
        enterChunk();
        widget->m_flags = readInt();
        finishChunk();
        return true;
    } else {
        return false;
    }
}

bool PLXStreamReader::readButtonProperty(const std::string &chunk, Button *button)
{
    if (chunk == "Button.type") {
        enterChunk();
        button->m_type = readInt();
        finishChunk();
        return true;
    }
    return readWidgetProperty(chunk, button);
}

bool PLXStreamReader::readListWidgetProperty(Widget *widget, const std::string &chunk)
{
    return readWidgetProperty(chunk, widget);
}

bool PLXStreamReader::readScrollButtonProperty(const std::string &chunk, Widget *widget)
{
    if (chunk == "ScrollButton.direction") {
        enterChunk();
        static_cast<ScrollButton *>(widget)->m_direction = readInt();
        finishChunk();
        return true;
    }
    return readButtonProperty(chunk, static_cast<Button *>(widget));
}

bool PLXStreamReader::readScrollSliderProperty(const std::string &chunk, Widget *widget)
{
    if (chunk == "ScrollSlider.direction") {
        enterChunk();
        static_cast<ScrollSlider *>(widget)->m_direction = readInt();
        finishChunk();
        return true;
    }
    return readButtonProperty(chunk, static_cast<Button *>(widget));
}

bool PLXStreamReader::readPopUpButtonProperty(const std::string &chunk, Widget *widget)
{
    return readButtonProperty(chunk, static_cast<Button *>(widget));
}

void PLXStreamReader::read()
{
    m_nodes[-1] = 0;
    m_shapes[-1] = 0;
    m_transformations[-1] = 0;
    m_displays[-1] = 0;
    m_widgets[-1] = 0;
    m_fontIds[-1] = -1;
    m_textureIds[-1] = -1;
    int shapeIndex = 0;
    int transformationIndex = 0;
    int displayIndex = 0;
    int widgetIndex = 0;
    int nodeIndex = 0;

    // The code constructs both legacy lists, although PLX chunks never fill them.
    std::list<PLAFontReference> fonts;
    std::list<int> children;
    if (m_flags & (1 << LOAD_CHECK_SIGNATURE)) {
        std::streampos position = m_stream->tellg();
        readInt();
        readInt();
        readInt();
        readInt();
        char signature[15] = "              ";
        m_stream->read(signature, 14);
        m_stream->seekg(position);
        if (strcmp(signature, "PlasmaGraphics"))
            throw InvalidFileFormatException(std::string(""));
    }
    m_obfuscationKey = 0;
    while (!m_stream->eof()) {
        std::string chunk = parseChunkHeader();
        if (m_stream->eof())
            break;
        if (chunk == "PlasmaGraphics") {
            enterChunk();
            if (readInt() > MaximumSupportedPLXVersion)
                throw InvalidVersionException(std::string(""));
            finishChunk();
        } else if (chunk == "Seal") {
            enterChunk();
            int length = readInt();
            std::vector<char> sealed;
            sealed.resize(length);
            m_stream->read(&sealed[0], length);
            bool valid = false;
            std::vector<unsigned __int64> keys(m_keys);
            unsigned __int64 key = 0;
            keys.push_back(key);
            keys.push_back(keyFromPhrase(std::string("PlasmaGraphics")));
            int count = (int)keys.size();
            for (int i = 0; i < count; ++i) {
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
                throw InvalidDemoLicenseException(std::string(""));
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
            m_page->size.m_elements[0] = readFloat();
            finishChunk();
        } else if (!(m_flags & (1 << LOAD_IGNORE_PAGE)) && chunk == "pageHeight") {
            enterChunk();
            m_page->size.m_elements[1] = readFloat();
            finishChunk();
        } else if (!(m_flags & (1 << LOAD_IGNORE_PAGE)) && chunk == "dpi") {
            enterChunk();
            m_page->size.m_elements[2] = readFloat();
            finishChunk();
        } else if (!(m_flags & (1 << LOAD_IGNORE_PAGE)) && chunk == "unit") {
            enterChunk();
            m_page->unit = readInt();
            finishChunk();
        } else if (!(m_flags & (1 << LOAD_IGNORE_PAGE)) && chunk == "pageColor") {
            enterChunk();
            m_page->color = readVector4();
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

InvalidFileFormatException::InvalidFileFormatException(const InvalidFileFormatException &other)
    : IOException(other)
{}

IOException::IOException(const IOException &other)
    : Exception(other),
      m_fileName(other.m_fileName)
{}

InvalidVersionException::InvalidVersionException(const InvalidVersionException &other)
    : IOException(other)
{}

InvalidDemoLicenseException::InvalidDemoLicenseException(const InvalidDemoLicenseException &other)
    : IOException(other)
{}

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
    unsigned flags; // The code leaves flags uninitialised if the chunk is missing.
    while (!finishChunk()) {
        std::string chunk = parseChunkHeader();
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
            shape = m_shapes[readInt()];
            finishChunk();
        } else if (chunk == "Node.transformation") {
            enterChunk();
            transformation = m_transformations[readInt()];
            finishChunk();
        } else if (chunk == "Node.display") {
            enterChunk();
            display = m_displays[readInt()];
            finishChunk();
        } else if (chunk == "Node.widget") {
            enterChunk();
            widget = m_widgets[readInt()];
            finishChunk();
        } else if (chunk == "Node.child") {
            enterChunk();
            children.push_back(readInt());
            finishChunk();
        } else if (chunk == "Node.flags") {
            enterChunk();
            flags = readInt();
            finishChunk();
        } else if (chunk == "Node.variable") {
            enterChunk();

            // Reads the stored value before the key, so the key and value are swapped when stored.
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
            node->setVariable(it->first, it->second);
        if (m_flags & (1 << LOAD_INTERNAL))
            node->m_flags |= (1 << NODE_INTERNAL);
    }
    m_children[node] = children;
    return node;
}

int ChunkStreamReader::readInt()
{
    int value;
#ifdef PLASMA_PORTABLE
    value = 0;
#endif

    // A short read leaves the unread bytes uninitialised, as in the code.
    m_stream->read(reinterpret_cast<char *>(&value), sizeof(value));
    return value;
}

float ChunkStreamReader::readFloat()
{
    float value;
#ifdef PLASMA_PORTABLE
    value = 0;
#endif

    // A short read leaves the unread bytes uninitialised, as in the code.
    m_stream->read(reinterpret_cast<char *>(&value), sizeof(value));
    return value;
}

bool PLXStreamReader::readContinuousVector4(Attribute *attribute)
{
    enterChunk();
    int frame = 0;
    bool finished = finishChunk();
    while (!finished) {
        {
            std::string chunk = parseChunkHeader();
            if (chunk == "Attribute.frame") {
                enterChunk();
                if (frame == attribute->size())
                    attribute->duplicateElement();
                m_stream->read(reinterpret_cast<char *>(
                                   &static_cast<ContinuousAttribute<Vector<4, float>> *>(attribute)->m_values[frame]),
                               16);
                ++frame;
                finishChunk();
            } else if (chunk == "Attribute.sequence") {
                readAttributeSequence(attribute);
            } else {
                skipChunk();
            }
        }
        finished = finishChunk();
    }
    return finished;
}

bool PLXStreamReader::readArrayVector2(ContinuousArrayAttribute<Vector<2, float>> *attribute)
{
    enterChunk();
    int frame = 0;
    bool finished = finishChunk();
    while (!finished) {
        {
            std::string chunk = parseChunkHeader();
            if (chunk == "ArrayAttribute.size") {
                enterChunk();
                attribute->resize(readInt());
                finishChunk();
            } else if (chunk == "ArrayAttribute.frame") {
                enterChunk();
                if (frame == attribute->size())
                    attribute->duplicateElement();
                m_stream->read(reinterpret_cast<char *>(&(*attribute->m_values[frame])[0]),
                               8 * attribute->m_values[0]->size());
                ++frame;
                finishChunk();
            } else if (chunk == "Attribute.sequence") {
                readAttributeSequence(attribute);
            } else {
                skipChunk();
            }
        }
        finished = finishChunk();
    }
    return finished;
}

bool PLXStreamReader::readArrayVector4(ContinuousArrayAttribute<Vector<4, float>> *attribute)
{
    enterChunk();
    int frame = 0;
    bool finished = finishChunk();
    while (!finished) {
        {
            std::string chunk = parseChunkHeader();
            if (chunk == "ArrayAttribute.size") {
                enterChunk();
                attribute->resize(readInt());
                finishChunk();
            } else if (chunk == "ArrayAttribute.frame") {
                enterChunk();
                if (frame == attribute->size())
                    attribute->duplicateElement();
                m_stream->read(reinterpret_cast<char *>(&(*attribute->m_values[frame])[0]),
                               16 * attribute->m_values[0]->size());
                ++frame;
                finishChunk();
            } else if (chunk == "Attribute.sequence") {
                readAttributeSequence(attribute);
            } else {
                skipChunk();
            }
        }
        finished = finishChunk();
    }
    return finished;
}

bool PLXStreamReader::readArrayFloat(ContinuousArrayAttribute<float> *attribute)
{
    enterChunk();
    int frame = 0;
    bool finished = finishChunk();
    while (!finished) {
        {
            std::string chunk = parseChunkHeader();
            if (chunk == "ArrayAttribute.size") {
                enterChunk();
                attribute->resize(readInt());
                finishChunk();
            } else if (chunk == "ArrayAttribute.frame") {
                enterChunk();
                if (frame == attribute->size())
                    attribute->duplicateElement();
                m_stream->read(reinterpret_cast<char *>(&(*attribute->m_values[frame])[0]),
                               4 * attribute->m_values[0]->size());
                ++frame;
                finishChunk();
            } else if (chunk == "Attribute.sequence") {
                readAttributeSequence(attribute);
            } else {
                skipChunk();
            }
        }
        finished = finishChunk();
    }
    return finished;
}

bool PLXStreamReader::readDiscreteInt(Attribute *attribute)
{
    enterChunk();
    int frame = 0;
    bool finished = finishChunk();
    while (!finished) {
        {
            std::string chunk = parseChunkHeader();
            if (chunk == "Attribute.frame") {
                enterChunk();
                if (frame == attribute->size())
                    attribute->duplicateElement();
                m_stream->read(
                    reinterpret_cast<char *>(&static_cast<DiscreteAttribute<int> *>(attribute)->m_values[frame]), 4);
                ++frame;
                finishChunk();
            } else if (chunk == "Attribute.sequence") {
                readAttributeSequence(attribute);
            } else {
                skipChunk();
            }
        }
        finished = finishChunk();
    }
    return finished;
}

bool PLXStreamReader::readContinuousVector2(Attribute *attribute)
{
    enterChunk();
    int frame = 0;
    bool finished = finishChunk();
    while (!finished) {
        {
            std::string chunk = parseChunkHeader();
            if (chunk == "Attribute.frame") {
                enterChunk();
                if (frame == attribute->size())
                    attribute->duplicateElement();
                m_stream->read(reinterpret_cast<char *>(
                                   &static_cast<ContinuousAttribute<Vector<2, float>> *>(attribute)->m_values[frame]),
                               8);
                ++frame;
                finishChunk();
            } else if (chunk == "Attribute.sequence") {
                readAttributeSequence(attribute);
            } else {
                skipChunk();
            }
        }
        finished = finishChunk();
    }
    return finished;
}

bool PLXStreamReader::readContinuousVector3(Attribute *attribute)
{
    enterChunk();
    int frame = 0;
    bool finished = finishChunk();
    while (!finished) {
        {
            std::string chunk = parseChunkHeader();
            if (chunk == "Attribute.frame") {
                enterChunk();
                if (frame == attribute->size())
                    attribute->duplicateElement();
                m_stream->read(reinterpret_cast<char *>(
                                   &static_cast<ContinuousAttribute<Vector<3, float>> *>(attribute)->m_values[frame]),
                               12);
                ++frame;
                finishChunk();
            } else if (chunk == "Attribute.sequence") {
                readAttributeSequence(attribute);
            } else {
                skipChunk();
            }
        }
        finished = finishChunk();
    }
    return finished;
}

bool PLXStreamReader::readContinuousMatrix(Attribute *attribute)
{
    enterChunk();
    int frame = 0;
    bool finished = finishChunk();
    while (!finished) {
        {
            std::string chunk = parseChunkHeader();
            if (chunk == "Attribute.frame") {
                enterChunk();
                if (frame == attribute->size())
                    attribute->duplicateElement();
                m_stream->read(reinterpret_cast<char *>(
                                   &static_cast<ContinuousAttribute<Matrix<float>> *>(attribute)->m_values[frame]),
                               64);
                ++frame;
                finishChunk();
            } else if (chunk == "Attribute.sequence") {
                readAttributeSequence(attribute);
            } else {
                skipChunk();
            }
        }
        finished = finishChunk();
    }
    return finished;
}

bool PLXStreamReader::readContinuousFloat(Attribute *attribute)
{
    enterChunk();
    int frame = 0;
    bool finished = finishChunk();
    while (!finished) {
        {
            std::string chunk = parseChunkHeader();
            if (chunk == "Attribute.frame") {
                enterChunk();
                if (frame == attribute->size())
                    attribute->duplicateElement();
                m_stream->read(
                    reinterpret_cast<char *>(&static_cast<ContinuousAttribute<float> *>(attribute)->m_values[frame]),
                    4);
                ++frame;
                finishChunk();
            } else if (chunk == "Attribute.sequence") {
                readAttributeSequence(attribute);
            } else {
                skipChunk();
            }
        }
        finished = finishChunk();
    }
    return finished;
}

Vector<2, float> PLXStreamReader::readVector2()
{
    Vector<2, float> value;
    m_stream->read(reinterpret_cast<char *>(&value), 8);
    return value;
}

Matrix<float> PLXStreamReader::readMatrix()
{
    Matrix<float> value;
    m_stream->read(reinterpret_cast<char *>(value.m_elements), 64);
    return value;
}

Vector<4, float> PLXStreamReader::readVector4()
{
    Vector<4, float> value;
    m_stream->read(reinterpret_cast<char *>(&value), 16);
    return value;
}

}
