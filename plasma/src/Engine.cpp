#include "plasma/FilePath.h"
#include "plasma/Engine.h"
#include "plasma/FileIds.h"
#ifdef PLASMA_PORTABLE
#include "Utf16.h"
#endif
#include "EditableMesh.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/CurveShape.h"
#include "plasma/ScrollButton.h"
#include "plasma/ScrollSlider.h"
#include "plasma/PopUpButton.h"
#include "plasma/ListWidget.h"
#include "plasma/Edit.h"
#include "plasma/TextShape.h"
#include "plasma/PLXStreamReader.h"
#include "plasma/PLXStreamWriter.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <sstream>
#include "main.h"
#include "plasma/ContinuousArrayAttribute.h"
#include "plasma/Texture.h"
#include "plasma/Shape.h"
#include "plasma/GenericWidget.h"
#include "plasma/GenericShape.h"
#include "plasma/Button.h"
#include "plasma/Widget.h"
#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
#include "plasma/Display.h"
#include "plasma/Movie.h"
#include "plasma/PixelFont.h"
#include "plasma/PlasmaFont.h"
#include "plasma/Node.h"
#include "plasma/RenderSurface.h"
#include "plasma/Drawing.h"
#include "plasma/Transformation.h"

// FreeType (freetype/freetype.h).
#ifndef PLASMA_PORTABLE
extern "C" int FT_Init_FreeType(FT_Library *library);
extern "C" int FT_Done_FreeType(FT_Library library);
extern "C" int FT_New_Face(FT_Library library, const char *path, long index, FT_Face *face);
extern "C" int FT_Done_Face(FT_Face face);
extern "C" int FT_Set_Pixel_Sizes(FT_Face face, unsigned width, unsigned height);
extern "C" int FT_Load_Char(FT_Face face, unsigned long character, long flags);

#endif

// Image loading uses FreeImage-owned bitmap storage.
#include <FreeImage.h>

std::wstring widenString(const std::string &text);

namespace plasma {

void prefixSourceNodeNames(Node *node, const std::wstring &prefix);

// The legacy PLA chunks store a four-byte identifier and a patched length.
static std::list<__int64> g_plaChunkPositions;

void writePLAShape(std::ostream &stream, Shape *shape);

class CurveShape;

void writePLACurveShape(std::ostream &stream, CurveShape *shape);
void writePLAMeshShape(std::ostream &stream, MeshShape *shape);
void writePLASmoothMeshShape(std::ostream &stream, SmoothMeshShape *shape);
void writePLAShape(std::ostream &stream, Shape *shape);

// Transient legacy font references: two strings, size, stroke and id.
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

SmoothMeshShape *__fastcall readPLAMesh(std::map<int, int> &, Engine *, std::istream &, std::map<int, std::string> &);
CurveShape *__fastcall ReadPLACurve(Engine *, std::istream &, std::map<int, std::string> &);
TextShape *__fastcall readPLATextShape(Engine *, std::istream &, std::map<int, std::string> &);
GenericShape *readPLAGenericShape(std::istream &, Engine *);
Widget *__stdcall ReadPLAEdit(std::istream &, Engine *);
Widget *__stdcall ReadPLAScrollButton(std::istream &, Engine *);
Widget *__stdcall ReadPLAScrollSlider(std::istream &, Engine *);
Widget *__stdcall ReadPLAListWidget(std::istream &, Engine *);
Movie *__stdcall ReadPLAMovie(std::istream &, Engine *, std::map<int, std::string> &);
bool readPLAScrollButtonProperty(std::istream &, ScrollButton *, int);
bool readPLAScrollSliderProperty(std::istream &, ScrollSlider *, int);

// Legacy PLA helper declarations, in file order below.
std::ostream &writePLAValue(std::ostream &stream, const void *value);
int readPLAInt(std::istream &stream);
float readPLAFloat(std::istream &stream);
void writePLAString(std::ostream &stream, const std::string &text);
void writePLAWString(std::ostream &stream, const std::wstring &text);
void beginPLAChunk(std::ostream &stream, int identifier);
void endPLAChunk(std::ostream &stream);
void skipPLAChunk(std::istream &stream);
std::streampos readPLAChunkEnd(std::istream &stream);
std::string readPLAString(std::istream &stream);
std::wstring readPLAWString(std::istream &stream);
void writePLASequences(std::ostream &stream, Attribute *attribute);
void readPLASequences(std::istream &stream, Attribute *attribute, std::map<int, std::string> &names);
std::ostream &writePLAVector2(std::ostream &stream, const Vector<2, float> &value);
std::ostream &writePLAMatrix(std::ostream &stream, const Matrix<float> &value);
std::ostream &writePLAVector3(std::ostream &stream, const Vector<3, float> &value);
std::ostream &writePLAVector4(std::ostream &stream, const Vector<4, float> &value);
Vector<2, float> readPLAVector2(std::istream &stream);
Matrix<float> readPLAMatrix(std::istream &stream);
void writePLAIntAttribute(std::ostream &stream, DiscreteAttribute<int> *attribute);
void readPLAIntAttribute(std::istream &stream, DiscreteAttribute<int> *attribute, std::map<int, std::string> &names);
void writePLAVector2Attribute(std::ostream &stream, ContinuousAttribute<Vector<2, float>> *attribute);
void readPLAVector2Attribute(std::istream &stream, ContinuousAttribute<Vector<2, float>> *attribute,
                             std::map<int, std::string> &names);
void writePLAVector3Attribute(std::ostream &stream, ContinuousAttribute<Vector<3, float>> *attribute);
void readPLAVector3Attribute(std::istream &stream, ContinuousAttribute<Vector<3, float>> *attribute,
                             std::map<int, std::string> &names);
void writePLAMatrixAttribute(std::ostream &stream, ContinuousAttribute<Matrix<float>> *attribute);
void readPLAMatrixAttribute(std::istream &stream, ContinuousAttribute<Matrix<float>> *attribute,
                            std::map<int, std::string> &names);
void writePLAVector4Attribute(std::ostream &stream, ContinuousAttribute<Vector<4, float>> *attribute);
void readPLAVector4Attribute(std::istream &stream, ContinuousAttribute<Vector<4, float>> *attribute,
                             std::map<int, std::string> &names);
void writePLAVector2ArrayAttribute(std::ostream &stream, ContinuousArrayAttribute<Vector<2, float>> *attribute);
void readPLAVector2ArrayAttribute(std::istream &stream, ContinuousArrayAttribute<Vector<2, float>> *attribute,
                                  std::map<int, std::string> &names);
void writePLAVector4ArrayAttribute(std::ostream &stream, ContinuousArrayAttribute<Vector<4, float>> *attribute);
void readPLAVector4ArrayAttribute(std::istream &stream, ContinuousArrayAttribute<Vector<4, float>> *attribute,
                                  std::map<int, std::string> &names);
void writePLAFloatArrayAttribute(std::ostream &stream, ContinuousArrayAttribute<float> *attribute);
void readPLAFloatArrayAttribute(std::istream &stream, ContinuousArrayAttribute<float> *attribute,
                                std::map<int, std::string> &names);
template<class T> static T *findByName(const std::wstring &name, const std::list<T *> &objects);
void collectPLANodes(Node *node, std::list<Node *> &nodes);
void writePLAWidgetProperties(std::ostream &stream, Widget *widget);
void writePLAWidget(std::ostream &stream, Widget *widget);
void writePLAButtonProperties(std::ostream &stream, Button *button);
void writePLAButton(std::ostream &stream, Button *button);
void writePLAPopUpButton(std::ostream &stream, PopUpButton *button);
void writePLAListWidget(std::ostream &stream, ListWidget *widget);
bool __fastcall readPLAWidgetProperty(std::istream &stream, Widget *widget, int identifier);
bool readPLAButtonProperty(std::istream &stream, Button *button, int identifier);
bool readPLAListWidgetProperty(std::istream &stream, ListWidget *widget, int identifier);
Widget *__stdcall ReadPLAWidget(std::istream &stream, Engine *engine);
Button *__stdcall ReadPLAButton(std::istream &stream, Engine *engine);

void writePLATexture(std::ostream &, Texture *);
void writePLAMovie(std::ostream &, Movie *);
void writePLAScrollButtonProperties(std::ostream &, ScrollButton *, std::map<Widget *, int> &);
void writePLAScrollButton(std::ostream &, ScrollButton *, std::map<Widget *, int> &);
void writePLAScrollSliderProperties(std::ostream &, ScrollSlider *, std::map<Widget *, int> &);
void writePLAScrollSlider(std::ostream &, ScrollSlider *, std::map<Widget *, int> &);
void writePLAWidgetObject(std::ostream &, Widget *, std::map<Widget *, int> &);

static const int DefaultViewportWidth = 1000;
static const int DefaultViewportHeight = 800;
static const unsigned long CriticalSectionSpinCount = 0x400;
static const unsigned long PreallocateCriticalSectionEvent = 0x80000000;
static const int PixelFontCellSpacingDivisor = 16;
static const int EmptyGlyphCellExtentDivisor = 3;

Engine::Engine()
    : m_page(1000.0f, 1000.0f, 72.0f),
      m_blurSurface(0),
      m_surfaceDepth(-1),
      m_filterSurface(0),
      m_overlayRoot(0),
      m_pointerNode(0),
      m_pointerCapture(0),
      m_focusWidget(0),
      m_popUpWidget(0),
      m_pointer(0.0f, 0.0f),
      m_previousPointer(0.0f, 0.0f),
      m_frameTime(0),
      m_caretClock(0),
      m_buttons(0),
      m_pressedPopUpItem(0),
      m_viewportX(0),
      m_viewportY(0),
      m_viewportWidth(DefaultViewportWidth),
      m_viewportHeight(DefaultViewportHeight),
      m_windowWidth(0),
      m_windowHeight(0),
      m_fillColor(1.0f, 1.0f, 1.0f, 1.0f),
      m_outlineColor(1.0f, 1.0f, 1.0f, 1.0f),
      m_extrusionColor(1.0f, 1.0f, 1.0f, 1.0f),
      m_renderFlags((1 << RENDER_ANTIALIAS) | (1 << RENDER_CLIP_WIDGET_SURFACES) | (1 << RENDER_BLUR)),
      m_unused1F4(true)
{
    // Allocate the critical section's event up front.
    InitializeCriticalSectionAndSpinCount(&m_lock, PreallocateCriticalSectionEvent | CriticalSectionSpinCount);
    m_fontEngine = new FontEngine(this);
    FT_Init_FreeType(&m_freeType);
    m_root = createNode(0, 0, 0, 0, L"root");
    m_sources = createNode(0, 0, 0, 0, L"sources");
    m_sources->m_flags |= (1 << NODE_INTERNAL);
    m_fonts = createNode(0, 0, 0, 0, L"fonts");
    m_fonts->m_flags |= (1 << NODE_INTERNAL);
}

Engine::~Engine()
{
    delete m_fontEngine;
    FT_Done_FreeType(m_freeType);
    for (int i = 0; i < (int)m_renderSurfaces.size(); i++)
        delete m_renderSurfaces[i];
    delete m_blurSurface;
    DeleteCriticalSection(&m_lock);
}

void Engine::lock()
{
    EnterCriticalSection(&m_lock);
}

void Engine::unlock()
{
    LeaveCriticalSection(&m_lock);
}

Node *Engine::createNode(Transformation *transformation, Shape *shape, Display *display, Node *parent,
                         const std::wstring &name)
{
    if (!transformation)
        transformation = createTransformation();
    if (!display)
        display = createDisplay();
    Node *node = new Node(this, transformation, shape, display, name);
    if (parent)
        parent->addChild(node);
    return node;
}

Transformation *Engine::createTransformation(const std::wstring &name)
{
    return new Transformation(this, name);
}

Display *Engine::createDisplay(const std::wstring &name)
{
    return new Display(this, name);
}

void Engine::destroyNode(Node *node)
{
    if (node->getEngine() != this)
        return;
    if (node == m_pointerCapture)
        releasePointerCapture();
    if (node == m_pointerNode)
        m_pointerNode = 0;
    std::list<Node *> children = node->m_children;
    for (std::list<Node *>::iterator it = children.begin(); it != children.end(); ++it)
        destroyNode(*it);
    if (node->m_parent)
        node->m_parent->removeChild(node);
    Transformation *transformation = node->m_transformation;
    if (transformation) {
        if (transformation->m_nodes.size() == 1)
            deleteTransformation(transformation);
        else
            transformation->m_nodes.erase(node);
    }
    Shape *shape = node->m_shape;
    if (shape) {
        if (shape->m_nodes.size() == 1)
            deleteShape(shape);
        else
            shape->m_nodes.erase(node);
    }
    Display *display = node->m_display;
    if (display) {
        if (display->m_nodes.size() == 1)
            deleteDisplay(display);
        else
            display->m_nodes.erase(node);
    }
    if (node->m_widget)
        deleteWidget(node->m_widget);
    lock();
    m_nodes.erase(node->m_engineEntry);
    unlock();
    delete node;
}

void Engine::deleteShape(Shape *shape)
{
    lock();
    m_shapes.erase(shape->m_engineEntry);
    unlock();
    delete shape;
}

void Engine::deleteDisplay(Display *display)
{
    lock();
    m_displays.erase(display->m_engineEntry);
    unlock();
    delete display;
}

void Engine::deleteDrawing(Drawing *drawing)
{
    delete drawing;
}

void Engine::deleteObject(Object *object)
{
    delete object;
}

void Engine::deleteRenderSurface(RenderSurface *surface)
{
    delete surface;
}

void Engine::deleteTransformation(Transformation *transformation)
{
    lock();
    m_transformations.erase(transformation->m_engineEntry);
    unlock();
    delete transformation;
}

void Engine::deleteTexture(Texture *texture)
{
    lock();
    m_textures.erase(texture->m_engineEntry);
    m_texturesById.erase(texture->m_id);
    unlock();
    delete texture;
}

int Engine::drawWindow()
{
    beginWindowDrawing();
    if (m_renderSurfaces.empty()) {
        m_renderSurfaces.resize(2);
        for (int i = 0; i < static_cast<int>(m_renderSurfaces.size()); ++i)
            m_renderSurfaces[i] = createRenderSurface(0);
    }
    if (m_blurSurface)
        m_blurSurface->resize(m_viewportWidth, m_viewportHeight);
    for (int i = 0; i < static_cast<int>(m_renderSurfaces.size()); ++i)
        m_renderSurfaces[i]->resize(m_viewportWidth, m_viewportHeight);
    m_renderTargetSwitchCount = 0;
    m_surfaceDepth = 0;
    m_root->draw(1 << NODE_RENDER_CULL_SCREEN);
    endWindowDrawing();
    return m_renderTargetSwitchCount;
}

Node *Engine::hitTest(const Vector<2, float> &position)
{
    return m_root->hitTest(position, 0);
}

bool Engine::advance(int milliseconds)
{
    m_caretClock += milliseconds;
    m_frameTime = milliseconds;
    m_renderTargetSwitchCount = 0;
    return m_root->animate(milliseconds);
}

Movie *Engine::createMovie(const std::wstring &name)
{
    delete findMovie(name);
    Movie *movie = new Movie(this, name);
    m_movies[name] = movie;
    return movie;
}

void Engine::addFontPath(const std::wstring &path)
{
    m_fontPaths.push_back(path);
    m_fontEngine->addFontPath(path);
}

PixelFont *Engine::createPixelFont(const std::wstring &name)
{
    return new PixelFont(this, name);
}

PlasmaFont *Engine::createPlasmaFont(const std::wstring &name)
{
    return new PlasmaFont(this, name);
}

PlasmaFont *Engine::createPlasmaFont(Node *root, const std::wstring &name)
{
    PlasmaFont *font = createPlasmaFont(name);
    for (std::list<Node *>::iterator it = root->m_children.begin(); it != root->m_children.end(); ++it) {
        if ((*it)->getName().size() == 1 && (*it)->m_shape) {
            std::wstring character = (*it)->getName();
            Shape *minimumShape = (*it)->m_shape;
            const Matrix<float> &minimumMatrix = (*it)->m_transformation->m_matrix;
            Vector<2, float> minimum = minimumMatrix * minimumShape->getMinimum();
            Shape *maximumShape = (*it)->m_shape;
            const Matrix<float> &maximumMatrix = (*it)->m_transformation->m_matrix;
            Vector<2, float> maximum = maximumMatrix * maximumShape->getMaximum();
            if (!(*it)->isHitTestable())
                (*it)->setShape(0);
            Vector<2, float> glyphSize = maximum - minimum;
            Vector<2, float> offset = minimum - (*it)->m_transformation->m_translation.value();
            font->setGlyph(character[0], offset, glyphSize, *it, maximum.m_elements[0] - minimum.m_elements[0]);
        }
    }
    return font;
}

PlasmaFont *Engine::loadPlasmaFont(const std::wstring &path, float size)
{
    int slash = path.find_last_of(L'/');
    int backslash = path.find_last_of(L'\\');
    int separator = (std::max)(slash, backslash);
    std::wstring basename = path.substr(separator + 1, path.size() - separator - 1);
    std::wstring directory;
    std::ifstream stream;
    stream.open(PLASMA_FILE_PATH(path.c_str()), std::ios::in | std::ios::binary);
    if (!stream.is_open()) {
        for (std::list<std::wstring>::iterator it = m_fontPaths.begin(); it != m_fontPaths.end(); ++it) {
            stream.open(PLASMA_FILE_PATH((*it + L"/" + basename).c_str()), std::ios::in | std::ios::binary);
            if (stream.is_open()) {
                directory = *it;
                break;
            }
        }
    }
    if (!stream.is_open()) {
        PlasmaFont *font = createPlasmaFont(basename);
        font->m_path = basename;
        font->m_size = size;
        return font;
    }
    stream.close();
    Node *fontNode = 0;
    for (std::list<Node *>::iterator it = m_fonts->m_children.begin(); it != m_fonts->m_children.end(); ++it) {
        if ((*it)->getName() == basename) {
            fontNode = *it;
            break;
        }
    }
    if (directory.empty())
        directory = path;
    else
        directory += L"/" + basename;
    if (!fontNode) {
        fontNode = createNode(0, 0, 0, m_fonts, basename);
        readPlasmaFileIntoNode(directory, fontNode,
                               (1 << LOAD_SHARED) | (1 << LOAD_NO_WIDGET_INIT) | (1 << LOAD_INTERNAL),
                               static_cast<Page *>(0), std::vector<unsigned __int64>(1, 0));
        fontNode->m_flags |= (1 << NODE_INTERNAL);
    }
    PlasmaFont *font = createPlasmaFont(fontNode, basename);
    font->m_path = basename;
    font->m_size = size;
    font->m_spacing = 0.0f;
    return font;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
PlasmaFont *Engine::loadOutlineFont(const std::wstring &path, float size, const std::wstring &characters)
{
    int slash = path.find_last_of(L'/');
    int backslash = path.find_last_of(L'\\');
    int separator = (std::max)(slash, backslash);
    std::wstring basename = path.substr(separator + 1, path.size() - separator - 1);
    for (std::list<Font *>::iterator it = m_fontList.begin(); it != m_fontList.end(); ++it) {
        PlasmaFont *font = dynamic_cast<PlasmaFont *>(*it);
        if (font && font->m_path == basename) {
            PlasmaFont *copy = font->clone();
            copy->m_path = basename;
            copy->m_size = size;
            copy->m_spacing = 0.0f;
            return copy;
        }
    }
    std::ifstream stream;
    stream.open(PLASMA_FILE_PATH(path.c_str()), std::ios::in | std::ios::binary);
    std::wstring directory;
    if (!stream.is_open()) {
        for (std::list<std::wstring>::iterator it = m_fontPaths.begin(); it != m_fontPaths.end(); ++it) {
            stream.open(PLASMA_FILE_PATH((*it + L"/" + basename).c_str()), std::ios::in | std::ios::binary);
            if (stream.is_open()) {
                directory = *it;
                break;
            }
        }
    }
    if (!stream.is_open()) {
        PlasmaFont *font = createPlasmaFont(basename);
        font->m_path = basename;
        font->m_size = size;
        return font;
    }
    stream.close();
    if (directory.size())
        directory += L"/" + basename;
    else
        directory = path;
    FT_Face face;
    if (FT_New_Face(m_freeType, narrowString(directory).c_str(), 0, &face))
        return 0;
    std::string source = std::string(face->family_name) + " " + face->style_name;
    FT_Set_Pixel_Sizes(face, plasma::ImportedGlyphPixelHeight, plasma::ImportedGlyphPixelHeight);
    std::wstring glyphs = characters;
    if (glyphs.empty())
        glyphs =
            L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ`1234567890 .,-_:!?<>/\\|[]()=+~@#$%^&*()'\"|\u00b0";
    Node *root = createNode(0, 0, 0, m_fonts, basename);
    root->m_flags |= (1 << NODE_INTERNAL);
    for (int character = 0; character < static_cast<int>(glyphs.size()); ++character) {
        wchar_t name[2] = {glyphs[character], 0};
        FT_Load_Char(face, glyphs[character], FT_LOAD_NO_BITMAP | FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING);
        SmoothMeshShape *background = createSmoothMeshShape(name);
        Node *glyphNode = createNode(0, background, 0, root, name);
        glyphNode->m_flags |= (1 << NODE_INTERNAL);
        background->setSubdivisions(0, true);
        background->m_vertexPositions.resize(4);
        background->m_vertexColors.resize(4);
        background->m_vertexPositions.value()[0] =
            Vector<2, float>(face->glyph->bitmap_left * (double)0.015625f, -100.0f);
        background->m_vertexPositions.value()[1] =
            Vector<2, float>(face->glyph->advance.x * (double)0.015625f, -100.0f);
        background->m_vertexPositions.value()[2] = Vector<2, float>(face->glyph->advance.x * (double)0.015625f, 0.0f);
        background->m_vertexPositions.value()[3] = Vector<2, float>(face->glyph->bitmap_left * (double)0.015625f, 0.0f);
        background->m_vertexColors.value()[0] = Vector<4, float>(0.6f, 0.6f, 0.6f, 0.5f);
        background->m_vertexColors.value()[1] = Vector<4, float>(0.8f, 0.8f, 0.8f, 0.5f);
        background->m_vertexColors.value()[2] = Vector<4, float>(0.7f, 0.7f, 0.7f, 0.5f);
        background->m_vertexColors.value()[3] = Vector<4, float>(0.5f, 0.5f, 0.5f, 0.5f);
        std::vector<std::vector<int>> backgroundFaces(1);
        backgroundFaces[0].resize(4);
        backgroundFaces[0][0] = 0;
        backgroundFaces[0][1] = 1;
        backgroundFaces[0][2] = 2;
        backgroundFaces[0][3] = 3;
        background->setFaces(backgroundFaces, true);
        SmoothMeshShape *shape = createSmoothMeshShape(name);
        Node *outlineNode = createNode(0, shape, 0, glyphNode, name);
        outlineNode->m_flags |= (1 << NODE_INTERNAL);

        // The outline indices use the first vertex-position frame.
        std::vector<std::vector<int>> faces;
        int start = 0;
        float width = 4.0f;
        FT_Outline &outline = face->glyph->outline;
        for (int contour = 0; contour < outline.n_contours; ++contour) {
            int last = outline.contours[contour];
            Vector<2, float> previous(static_cast<float>(outline.points[last].x),
                                      static_cast<float>(-static_cast<double>(outline.points[last].y)));
            previous = previous * 0.015625f;
            Vector<2, float> previousDelta(0.0f, 0.0f);
            bool previousOffCurve = !(outline.tags[last] & 1);
            std::vector<int> vertices;
            for (int point = start; point <= outline.contours[contour]; ++point) {
                Vector<2, float> position(static_cast<float>(outline.points[point].x),
                                          static_cast<float>(-static_cast<double>(outline.points[point].y)));
                position = position * 0.015625f;
                Vector<2, float> delta = position - previous;
                float distanceSquared =
                    delta.m_elements[0] * delta.m_elements[0] + delta.m_elements[1] * delta.m_elements[1];
                if (!(distanceSquared < 0.001f)) {
                    if (outline.tags[point] & 2)
                        std::cout << "FT_CURVE_TAG_CUBIC" << std::endl;
                    if (delta.m_elements[0] * previousDelta.m_elements[1] -
                            previousDelta.m_elements[0] * delta.m_elements[1] ==
                        0.0f) {
                        float dot = delta.m_elements[0] * previousDelta.m_elements[0];
                        dot += delta.m_elements[1] * previousDelta.m_elements[1];
                        if (dot < 0.0f) {
                            shape->m_vertexPositions.erase(shape->m_vertexPositions.m_values[0]->size() - 1);
                            shape->m_vertexTexCoords.erase(shape->m_vertexPositions.m_values[0]->size());
                            shape->m_vertexColors.erase(shape->m_vertexPositions.m_values[0]->size());
                            shape->m_strokeColors.erase(shape->m_vertexPositions.m_values[0]->size());
                            shape->m_strokeWidths.erase(shape->m_vertexPositions.m_values[0]->size());
                            if (!shape->m_vertexFlags.empty())
                                shape->m_vertexFlags.pop_back();
                            if (!vertices.empty())
                                vertices.pop_back();
                        }
                    }
                    previousDelta = delta;
                    if (previousOffCurve && !(outline.tags[point] & FT_CURVE_TAG_ON)) {
                        vertices.push_back(shape->m_vertexPositions.m_values[0]->size());
                        Vector<2, float> midpoint = (previous + position) * 0.5f;
                        shape->m_vertexPositions.append(midpoint);
                        shape->m_vertexColors.append(Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                        shape->m_vertexTexCoords.append(Vector<2, float>(0.0f, 0.0f));
                        shape->m_strokeColors.append(Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                        shape->m_strokeWidths.append(width);
                        shape->m_vertexFlags.push_back(VERTEX_SHARP);
                    }
                    vertices.push_back(shape->m_vertexPositions.m_values[0]->size());
                    shape->m_vertexPositions.append(position);
                    shape->m_vertexColors.append(Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                    shape->m_vertexTexCoords.append(Vector<2, float>(0.0f, 0.0f));
                    shape->m_strokeColors.append(Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                    shape->m_strokeWidths.append(width);
                    shape->m_vertexFlags.push_back(2 - ((outline.tags[point] & FT_CURVE_TAG_ON) != 0));
                    previous = position;
                    previousOffCurve = !(outline.tags[point] & FT_CURVE_TAG_ON);
                }
            }
            faces.push_back(vertices);
            start = outline.contours[contour] + 1;
        }
        shape->m_flags =
            (shape->m_flags & ~((1 << MESH_CONTOUR) | (1 << MESH_NO_FILL) | (1 << MESH_OPEN))) | (1 << MESH_CONTOUR);
        shape->setFaces(faces, true);
        glyphNode->m_flags |= (1 << NODE_LOCKED) | (1 << NODE_INACTIVE);
        glyphNode->getMaximum(0);
    }
    FT_Done_Face(face);
    PlasmaFont *font = createPlasmaFont(root, widenString(source));
    font->m_path = basename;
    font->m_size = size;
    font->m_spacing = 0.0f;
    return font;
}

// Pixel-font atlas metrics: the em is two thirds of a 16-by-16 atlas cell; integer truncation is intentional.
PixelFont *Engine::createPixelFont(Texture *texture, const std::wstring &characters)
{
    unsigned short character = 0;
    std::wstring glyphs = characters;
    {
        // The code replaces the supplied character list with code units 0..255.
        std::wstringstream stream;
        do {
            stream << static_cast<wchar_t>(character++);
        } while (character < 256);
        // Move-assigns the temporary string.
        glyphs = stream.str();
    }
    int glyphCount = glyphs.size();
    PixelFont *font = createPixelFont(texture->getName());
    font->setTexture(texture);
    font->m_size = static_cast<float>(2 * (texture->m_width / 16) / 3);
    Vector<2, int> cellSize(texture->m_width / 16, texture->m_height / 16);
    int spacing = cellSize[0] / PixelFontCellSpacingDivisor - 1;
    Vector<2, float> offset(0.0f, 0.0f);
    for (int i = 0; i < glyphCount; ++i) {
        Vector<2, int> origin(i % 16 * cellSize[0], i / 16 * cellSize[1]);
        Vector<2, int> minimum(cellSize[0] - 1, cellSize[1] - 1);
        Vector<2, int> maximum(0, 0);
        for (int x = 0; x < cellSize[0]; ++x) {
            for (int y = 0; y < cellSize[1]; ++y) {
                if (origin[0] + x < texture->m_width && origin[1] + y < texture->m_height &&
                    texture->m_pixels[4 * ((origin[1] + y) * texture->m_width + origin[0] + x) + 3]) {
                    if (x > maximum[0])
                        maximum[0] = x;
                    if (y > maximum[1])
                        maximum[1] = y;
                    if (x < minimum[0])
                        minimum[0] = x;
                    if (y < minimum[1])
                        minimum[1] = y;
                }
            }
        }
        if (maximum[0] < minimum[0] || maximum[1] < minimum[1]) {
            minimum = Vector<2, int>(0, 0);
            maximum = Vector<2, int>(texture->m_width / 16 / EmptyGlyphCellExtentDivisor,
                                     texture->m_height / 16 / EmptyGlyphCellExtentDivisor);
        }
        minimum -= Vector<2, int>(1, 1);
        maximum += Vector<2, int>(1, 1);
        Vector<2, int> dimensions = maximum - minimum + Vector<2, int>(1, 1);
        Vector<2, int> textureOrigin = minimum + origin;
        Vector<2, float> inverseSize((double)1.0f / texture->m_width, (double)1.0f / texture->m_height);
        Vector<2, float> size(dimensions);
        offset[1] = static_cast<float>(minimum[1]);
        Vector<2, float> textureSize = size * inverseSize;
        Vector<2, float> texturePosition = inverseSize * Vector<2, float>(textureOrigin);
        font->setGlyph(glyphs[i], offset, size, texturePosition, textureSize,
                       static_cast<float>(spacing + dimensions[0]));
    }
    return font;
}

Font *Engine::loadLegacyFont(const std::wstring &path, float size, float stroke, bool transient, bool pixel)
{
    int slash = path.find_last_of(L'/');
    int backslash = path.find_last_of(L'\\');
    int separator = (std::max)(slash, backslash);
    std::wstring basename = path.substr(separator + 1, path.size() - separator - 1);

    // Reads each font through the iterator after comparing the path.
    for (std::list<Font *>::iterator it = m_fontList.begin(); it != m_fontList.end(); ++it) {
        if ((*it)->m_path == basename && (*it)->size() == size && (*it)->m_transient == transient &&
            (*it)->getSpacing() == stroke && dynamic_cast<PixelFont *>(*it)) {
            return *it;
        }
    }
    try {
        int dot = path.find_last_of(L'.');
        std::wstring extension = path.substr(dot + 1, path.size() - dot - 1);

        // The code applies narrow toupper to UTF-16 code units. Non-ASCII code units outside the unsigned-char range
        // reproduce its undefined use.
        std::transform(extension.begin(), extension.end(), extension.begin(), toupper);
        if (extension == L"TTF" && pixel)
            return loadPixelFont(path, size, transient, stroke);
        if (extension == L"TTF")
            return loadOutlineFont(path, size);
        return loadPlasmaFont(path, size);
    } catch (...) {
        return 0;
    }
}

PixelFont *Engine::loadPixelFont(const std::wstring &path, float size, bool transient, float stroke,
                                 const std::wstring &)
{
    int dot = path.find_last_of(L'.');
    std::wstring extension = path.substr(dot + 1, path.size() - dot - 1);
    int slash = path.find_last_of(L'/');
    int backslash = path.find_last_of(L'\\');
    int separator = (std::max)(slash, backslash);

    // Preserve narrow toupper on UTF-16 code units, including its domain UB.
    std::transform(extension.begin(), extension.end(), extension.begin(), toupper);
    std::wstring basename = path.substr(separator + 1, path.size() - separator - 1);
    if (extension == L"TTF") {
        PixelFont *font = createPixelFont(basename);
        font->m_path = basename;
        font->m_size = size;
        font->m_spacing = stroke;
        font->m_transient = transient;
        return font;
    }
    Texture *texture = loadImageTexture(path);
    if (!texture) {
        for (std::list<std::wstring>::iterator it = m_fontPaths.begin(); it != m_fontPaths.end(); ++it) {
            texture = loadImageTexture(*it + L"/" + basename);
            if (texture)
                break;
        }
    }
    if (!texture) {
        PixelFont *font = createPixelFont(path);
        font->m_path = path;
        return font;
    }
    TextureFormat format;
    format.minFilter = TEXTURE_FILTER_NEAREST;
    format.maxFilter = TEXTURE_FILTER_NEAREST;
    texture->setFormat(format);

    // The code changes this after creation, retaining the assigned texture ID.
    texture->m_transient = true;
    PixelFont *font = createPixelFont(texture);
    if (font)
        font->m_path = basename;
    return font;
}

Texture *Engine::loadImageTexture(const std::wstring &path)
{
    int dot = path.find_last_of(L'.');
    std::wstring extension = path.substr(dot + 1, path.size() - dot - 1);

    // The code uses narrow toupper rather than a wide-character conversion.
    std::transform(extension.begin(), extension.end(), extension.begin(), toupper);
    enum ImageFormat {
        FREEIMAGE_UNKNOWN = -1,
        FREEIMAGE_BMP = 0,
        FREEIMAGE_JPEG = 2,
        FREEIMAGE_PNG = 13,
        FREEIMAGE_PPM = 14,
        FREEIMAGE_TARGA = 17,
        FREEIMAGE_TIFF = 18
    };
    int imageFormat = FREEIMAGE_UNKNOWN;
    if (extension == L"JPG")
        imageFormat = FREEIMAGE_JPEG;
    if (extension == L"BMP")
        imageFormat = FREEIMAGE_BMP;
    if (extension == L"PNG")
        imageFormat = FREEIMAGE_PNG;
    if (extension == L"PPM")
        imageFormat = FREEIMAGE_PPM;
    if (extension == L"TIF")
        imageFormat = FREEIMAGE_TIFF;
    if (extension == L"TGA")
        imageFormat = FREEIMAGE_TARGA;
    else if (imageFormat == FREEIMAGE_UNKNOWN)
        return 0;
    FIBITMAP *bitmap = FreeImage_LoadU(static_cast<FREE_IMAGE_FORMAT>(imageFormat), path.c_str(), 0);
    if (!bitmap)
        return 0;
    int width = FreeImage_GetWidth(bitmap);
    int height = FreeImage_GetHeight(bitmap);
    std::vector<char> pixels(4 * width * height);

    // The code divides by width without testing zero and assumes RGB or RGBA scanlines. Other bit depths retain its
    // out-of-bounds pixel reads.
    unsigned bytesPerPixel = FreeImage_GetLine(bitmap) / FreeImage_GetWidth(bitmap);
    int destination = 0;
    for (int y = 0; y < static_cast<int>(FreeImage_GetHeight(bitmap)); ++y) {
        const unsigned char *source = FreeImage_GetScanLine(bitmap, FreeImage_GetHeight(bitmap) - y - 1);
        for (int x = 0; x < static_cast<int>(FreeImage_GetWidth(bitmap)); ++x) {
            pixels[destination] = source[2];
            pixels[destination + 1] = source[1];
            pixels[destination + 2] = source[0];
            if (bytesPerPixel == 3)
                pixels[destination + 3] = static_cast<char>(255);
            else
                pixels[destination + 3] = source[3];
            destination += 4;
            source += bytesPerPixel;
        }
    }
    FreeImage_Unload(bitmap);
    return createTexture(width, height, reinterpret_cast<const unsigned char *>(&pixels[0]), TextureFormat());
}

Movie *Engine::findMovie(const std::wstring &name)
{
    std::map<std::wstring, Movie *>::iterator entry = m_movies.find(name);
    if (entry == m_movies.end())
        return 0;
    return entry->second;
}

Node *Engine::findNode(const std::wstring &name)
{
    return findByName(name, m_nodes);
}

Widget *Engine::findWidget(const std::wstring &name)
{
    return findByName(name, m_widgets);
}

MeshShape *Engine::createMeshShape(const std::wstring &name)
{
    return new MeshShape(this, name);
}

TextShape *Engine::createTextShape(const std::wstring &text, const std::wstring &name)
{
    return new TextShape(this, text, name);
}

GenericShape *Engine::createGenericShape(const std::wstring &source, const std::wstring &name)
{
    return new GenericShape(this, source, name);
}

SmoothMeshShape *Engine::createSmoothMeshShape(const std::wstring &name)
{
    return new SmoothMeshShape(this, name);
}

CurveShape *Engine::createCurveShape(const std::wstring &name)
{
    return new CurveShape(this, name);
}

Widget *Engine::createWidget(Node *node, const std::wstring &name)
{
    return new Widget(this, node, name);
}

Button *Engine::createButton(Node *node, const std::wstring &name)
{
    return new Button(this, node, 0, name);
}

ScrollSlider *Engine::createScrollSlider(Node *node, const std::wstring &name)
{
    return new ScrollSlider(this, node, name);
}

ScrollButton *Engine::createScrollButton(Node *node, const std::wstring &name)
{
    return new ScrollButton(this, node, name);
}

PopUpButton *Engine::createPopUpButton(Node *, const std::wstring &name)
{
    return new PopUpButton(this, name);
}

ListWidget *Engine::createListWidget(Node *, const std::wstring &name)
{
    return new ListWidget(this, name);
}

Edit *Engine::createEdit(Node *node, const std::wstring &name)
{
    return new Edit(this, node, name);
}

void Engine::deleteWidget(Widget *widget)
{
    if (widget == m_focusWidget)
        m_focusWidget = 0;
    if (widget == m_popUpWidget)
        m_popUpWidget = 0;
    if (widget == m_pressedPopUpItem)
        m_pressedPopUpItem = 0;
    lock();
    m_widgets.erase(widget->m_engineEntry);
    unlock();
    delete widget;
}

void Engine::sendEvent(Node *node, int event)
{
    if (node && node->isHitTestable()) {
        Widget *widget = node->m_enclosingWidget;
        if (widget) {
            int lookupEvent = event;
            Widget::Connection *connection = widget->m_connections[lookupEvent];
            if (connection && (widget->m_enabled || !connection->m_enabledOnly))
                connection->invoke();
        }
    }
}

Node *Engine::getPointerNode()
{
    if (m_pointerCapture)
        return m_pointerCapture;
    return hitTest(m_pointer);
}

void Engine::initializeWidgets()
{
    m_root->initializeWidgets();
    for (std::list<Widget *>::iterator it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        (*it)->initialize();
        Widget::Connection *connection = (*it)->m_connections[WIDGET_EVENT_INITIALIZED];
        if (connection)
            connection->invoke();
    }
}

void Engine::setViewport(int x, int y, int width, int height, bool notify)
{
    m_viewportX = x;
    m_viewportY = y;
    m_viewportWidth = width;
    m_viewportHeight = height;
    if (notify) {
        std::list<Widget *>::iterator it = m_widgets.begin();
        if (it != m_widgets.end()) {
            do {
                (*it)->viewportChanged();
                Widget::Connection *connection = (*it)->m_connections[WIDGET_EVENT_VIEWPORT_CHANGED];
                if (connection)
                    connection->invoke();
                ++it;
            } while (it != m_widgets.end());
        }
    }
}

void Engine::setDimensions(int width, int height)
{
    m_windowWidth = width;
    m_windowHeight = height;
}

void Engine::dispatchWidgets(Node *node, void (Widget::*method)())
{
    if (node->m_display->m_visibility.value() && node->isShown() && node->isHitTestable()) {
        if (node->m_widget)
            (node->m_widget->*method)();
        for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
            dispatchWidgets(*it, method);
    }
}

void Engine::pointerPressed()
{
    m_buttons |= (1 << MOUSE_BUTTON_LEFT);
    Node *node = getPointerNode();
    if (node && node->m_enclosingWidget && node->m_enclosingWidget->isPopUpItem() &&
        node->m_enclosingWidget->m_enabled) {
        m_pressedPopUpItem = node->m_enclosingWidget;
        dispatchWidgets(m_root, &Widget::popUpItemPressedAnywhere);
    } else {
        m_pressedPopUpItem = 0;
    }
    if (m_popUpWidget && !m_popUpWidget->m_node->isAncestorOf(node))
        setPopUpWidget(0, 0);
    if (!node || node->m_widget != m_focusWidget)
        setFocusWidget(0);
    if (node) {
        Widget *widget = node->m_enclosingWidget;
        if (widget && widget->m_enabled)
            widget->pointerPressed();
        sendEvent(node, WIDGET_EVENT_POINTER_PRESSED);
    }
    dispatchWidgets(m_root, &Widget::pointerPressedAnywhere);
}

void Engine::pointerReleased()
{
    m_buttons &= ~(unsigned)(1 << MOUSE_BUTTON_LEFT);
    Node *node = getPointerNode();
    if (m_popUpWidget && m_pressedPopUpItem)
        setPopUpWidget(0, 0);
    if (node) {
        Widget *widget = node->m_enclosingWidget;
        if (widget && widget->m_enabled)
            widget->pointerReleased();
        sendEvent(node, WIDGET_EVENT_POINTER_RELEASED);
    }
    node = getPointerNode();
    if (node) {
        Node *hit = hitTest(m_pointer);
        Widget *underPointer = hit ? hit->m_enclosingWidget : 0;
        Widget *widget = node->m_enclosingWidget;
        if (widget != underPointer) {
            if (widget && widget->m_enabled)
                widget->pointerLeft();
            sendEvent(node, WIDGET_EVENT_POINTER_LEFT);
        }
    }
    dispatchWidgets(m_root, &Widget::pointerReleasedAnywhere);
}

void Engine::pointerDoubleClicked()
{
    m_buttons |= (1 << MOUSE_BUTTON_LEFT);
    Node *node = getPointerNode();
    if (!node || node->m_widget != m_focusWidget)
        setFocusWidget(0);
    if (node) {
        Widget *widget = node->m_enclosingWidget;
        if (widget && widget->m_enabled)
            widget->pointerDoubleClicked();
        sendEvent(node, WIDGET_EVENT_POINTER_DOUBLE_CLICKED);
    }
    if (m_popUpWidget && !m_popUpWidget->m_node->isAncestorOf(node))
        setPopUpWidget(0, 0);
    dispatchWidgets(m_root, &Widget::pointerDoubleClickedAnywhere);
}

void Engine::rightButtonPressed()
{
    m_buttons |= (1 << MOUSE_BUTTON_RIGHT);
    Node *node = getPointerNode();
    if (!node || node->m_widget != m_focusWidget)
        setFocusWidget(0);
    if (node) {
        Widget *widget = node->m_enclosingWidget;
        if (widget && widget->m_enabled)
            widget->rightButtonPressed();
        sendEvent(node, WIDGET_EVENT_RIGHT_BUTTON_PRESSED);
    }
    dispatchWidgets(m_root, &Widget::rightButtonPressedAnywhere);
}

void Engine::rightButtonReleased()
{
    m_buttons &= ~(unsigned)(1 << MOUSE_BUTTON_RIGHT);
    Node *node = getPointerNode();
    if (node) {
        Widget *widget = node->m_enclosingWidget;
        if (widget && widget->m_enabled)
            widget->rightButtonReleased();
        sendEvent(node, WIDGET_EVENT_RIGHT_BUTTON_RELEASED);
    }
    dispatchWidgets(m_root, &Widget::rightButtonReleasedAnywhere);
}

void Engine::middleButtonPressed()
{
    m_buttons |= (1 << MOUSE_BUTTON_MIDDLE);
    Node *node = getPointerNode();
    if (!node || node->m_widget != m_focusWidget)
        setFocusWidget(0);
    if (node) {
        Widget *widget = node->m_enclosingWidget;
        if (widget && widget->m_enabled)
            widget->middleButtonPressed();
        sendEvent(node, WIDGET_EVENT_MIDDLE_BUTTON_PRESSED);
    }
    dispatchWidgets(m_root, &Widget::middleButtonPressedAnywhere);
}

void Engine::middleButtonReleased()
{
    m_buttons &= ~(unsigned)(1 << MOUSE_BUTTON_MIDDLE);
    Node *node = getPointerNode();
    if (node) {
        Widget *widget = node->m_enclosingWidget;
        if (widget && widget->m_enabled)
            widget->middleButtonReleased();
        sendEvent(node, WIDGET_EVENT_MIDDLE_BUTTON_RELEASED);
    }
    dispatchWidgets(m_root, &Widget::middleButtonReleasedAnywhere);
}

void Engine::pointerWheel(int delta)
{
    m_wheelDelta = delta;
    Node *node = getPointerNode();
    if (node) {
        Widget *widget = node->m_enclosingWidget;
        if (widget && widget->m_enabled)
            widget->pointerWheel();
        sendEvent(node, WIDGET_EVENT_POINTER_WHEEL);
    }
    dispatchWidgets(m_root, &Widget::pointerWheelAnywhere);
}

void Engine::movePointer(float x, float y)
{
    m_previousPointer = m_pointer;
    m_pointer.m_elements[0] = x;
    m_pointer.m_elements[1] = y;
    Node *node = getPointerNode();
    Widget *previous = m_pointerNode ? m_pointerNode->m_enclosingWidget : 0;
    Widget *widget = node ? node->m_enclosingWidget : 0;
    if (previous != widget) {
        if (previous && previous->m_enabled)
            previous->pointerLeft();
        sendEvent(m_pointerNode, WIDGET_EVENT_POINTER_LEFT);
        if (widget && widget->m_enabled)
            widget->pointerEntered();
        sendEvent(node, WIDGET_EVENT_POINTER_ENTERED);
    }
    if (node) {
        if (widget && widget->m_enabled)
            widget->pointerMoved();
        sendEvent(node, WIDGET_EVENT_POINTER_MOVED);
    }
    m_pointerNode = node;
    dispatchWidgets(m_root, &Widget::pointerMovedAnywhere);
}

void Engine::keyPressed(unsigned short key)
{
    m_lastKey = key;
    m_pressedKeys.insert(key);
    if (m_focusWidget && m_focusWidget->m_enabled)
        m_focusWidget->keyPressed(key);
}

void Engine::keyReleased(unsigned short key)
{
    m_lastKey = key;
    m_pressedKeys.erase(key);
    if (m_focusWidget && m_focusWidget->m_enabled)
        m_focusWidget->keyReleased(key);
}

void Engine::characterEntered(int character)
{
    if (m_focusWidget && m_focusWidget->m_enabled)
        m_focusWidget->characterEntered(character);
}

void Engine::setPointerCapture(Node *node)
{
    m_pointerCapture = node;
}

void Engine::releasePointerCapture()
{
    m_pointerCapture = 0;
    movePointer(m_pointer[0], m_pointer[1]);
}

void Engine::setFocusWidget(Widget *widget)
{
    if (m_focusWidget) {
        if (widget == m_focusWidget)
            return;
        m_focusWidget->m_changed = true;
        sendEvent(m_focusWidget->m_node, WIDGET_EVENT_FOCUS_LOST);
    }
    m_focusWidget = widget;
    if (widget)
        sendEvent(widget->m_node, WIDGET_EVENT_FOCUS_GAINED);
}

Widget *Engine::getFocusWidget()
{
    return m_focusWidget;
}

void Engine::resetPointer()
{
    m_pointerCapture = 0;
    m_pointerNode = 0;
    movePointer(m_pointer[0], m_pointer[1]);
}

void writePLAString(std::ostream &stream, const std::string &text)
{
    int length = text.size();
    stream.write(reinterpret_cast<const char *>(&length), 4);
    stream.write(text.c_str(), length);
}

void writePLAWString(std::ostream &stream, const std::wstring &text)
{
#ifdef PLASMA_PORTABLE
    portable::WriteUtf16(stream, text);
#else
    int length = text.size();
    stream.write(reinterpret_cast<const char *>(&length), 4);
    stream.write(reinterpret_cast<const char *>(text.c_str()), length * 2);
#endif
}

void beginPLAChunk(std::ostream &stream, int identifier)
{
    writePLAValue(stream, &identifier);
    __int64 position = stream.tellp();
    g_plaChunkPositions.push_back(position);
    {
        int length = 0;
        writePLAValue(stream, &length);
    }
}

void endPLAChunk(std::ostream &stream)
{
    __int64 end = stream.tellp();
    __int64 start = g_plaChunkPositions.back();
    g_plaChunkPositions.pop_back();
    stream.seekp(start);
    {
        int length = static_cast<int>(end - start - 4);
        writePLAValue(stream, &length);
    }
    stream.seekp(end);
}

void skipPLAChunk(std::istream &stream)
{
    int length;
    stream.read(reinterpret_cast<char *>(&length), 4);
    stream.seekg(stream.tellg() + std::streampos(length));
}

std::streampos readPLAChunkEnd(std::istream &stream)
{
    int length;
    stream.read(reinterpret_cast<char *>(&length), 4);
    return stream.tellg() + std::streampos(length);
}

std::string readPLAString(std::istream &stream)
{
    // The code trusts the signed count, including malformed allocation/indexing UB.
    int length = readPLAInt(stream);
    if (!length)
        return std::string("");
    char *buffer = new char[length + 1];
    buffer[length] = 0;
    stream.read(buffer, length);
    std::string value(buffer);
    delete[] buffer;
    return value;
}

std::wstring readPLAWString(std::istream &stream)
{
#ifdef PLASMA_PORTABLE
    return portable::ReadUtf16(stream, readPLAInt(stream));
#else

    // The code trusts the signed count, including malformed allocation/indexing UB.
    int length = readPLAInt(stream);
    if (!length)
        return std::wstring(L"");
    wchar_t *buffer = new wchar_t[length + 1];
    buffer[length] = 0;
    stream.read(reinterpret_cast<char *>(buffer), length * 2);
    std::wstring value(buffer);
    delete[] buffer;
    return value;
#endif
}

void writePLASequences(std::ostream &stream, Attribute *attribute)
{
    const std::map<std::wstring, Sequence *> &sequences = attribute->getSequences();
    for (std::map<std::wstring, Sequence *>::const_iterator it = sequences.begin(); it != sequences.end(); ++it) {
        Sequence *sequence = it->second;
        beginPLAChunk(stream, 3);
        beginPLAChunk(stream, 3);
        writePLAString(stream, narrowString(it->first));
        endPLAChunk(stream);
        for (int i = 0; i < sequence->count(); ++i) {
            beginPLAChunk(stream, 2);
            beginPLAChunk(stream, 1);
            writePLAValue(stream, &(*sequence)[i].element);
            endPLAChunk(stream);
            beginPLAChunk(stream, 2);
            writePLAValue(stream, &(*sequence)[i].frame);
            endPLAChunk(stream);
            beginPLAChunk(stream, 3);
            writePLAValue(stream, &(*sequence)[i].smoothness);
            endPLAChunk(stream);
            endPLAChunk(stream);
        }
        endPLAChunk(stream);
    }
}

void readPLASequences(std::istream &stream, Attribute *attribute, std::map<int, std::string> &names)
{
    std::streampos end = readPLAChunkEnd(stream);
    Sequence *sequence = 0;
    while (stream.tellg() != end && !stream.eof()) {
        int identifier = readPLAInt(stream);
        switch (identifier) {
        case 1:
            readPLAChunkEnd(stream);
            sequence = attribute->newSequence(widenString(names[readPLAInt(stream)]));
            break;
        case 3:
            readPLAChunkEnd(stream);
            sequence = attribute->newSequence(widenString(readPLAString(stream)));
            break;
        case 2: {
            std::streampos keyEnd = readPLAChunkEnd(stream);
            int frame = 0;
            int element = 0;
            float smoothness = 0.0f;
            while (stream.tellg() != keyEnd && !stream.eof()) {
                switch (readPLAInt(stream)) {
                case 1:
                    readPLAChunkEnd(stream);
                    element = readPLAInt(stream);
                    break;
                case 2:
                    readPLAChunkEnd(stream);
                    frame = readPLAInt(stream);
                    break;
                case 3:
                    readPLAChunkEnd(stream);
                    smoothness = readPLAFloat(stream);
                    break;
                default:
                    skipPLAChunk(stream);
                    break;
                }
            }

            // A key before a sequence name dereferences null in the code.
            sequence->addKey(frame, element, smoothness);
            break;
        }
        default:
            skipPLAChunk(stream);
            break;
        }
    }
}

void writePLAMeshShape(std::ostream &stream, MeshShape *shape)
{
    beginPLAChunk(stream, 2);
    beginPLAChunk(stream, 1);
    writePLAString(stream, narrowString(shape->getName()));
    endPLAChunk(stream);
    for (int i = 0; i < static_cast<int>(shape->m_faces.size()); ++i) {
        beginPLAChunk(stream, 2);
        {
            unsigned int count = shape->m_faces[i].size();
            writePLAValue(stream, &count);
        }

        // The writer takes the first element even for an empty face.
        stream.write(reinterpret_cast<const char *>(&shape->m_faces[i][0]), shape->m_faces[i].size() * 4);
        endPLAChunk(stream);
    }
    beginPLAChunk(stream, 3);
    {
        unsigned int count = shape->m_vertexFlags.size();
        writePLAValue(stream, &count);
    }
    if (!shape->m_vertexFlags.empty())
        stream.write(reinterpret_cast<const char *>(&shape->m_vertexFlags[0]), shape->m_vertexFlags.size() * 4);
    endPLAChunk(stream);
    beginPLAChunk(stream, 4);
    writePLAVector2ArrayAttribute(stream, &shape->m_vertexPositions);
    endPLAChunk(stream);
    beginPLAChunk(stream, 5);
    writePLAVector2ArrayAttribute(stream, &shape->m_vertexTexCoords);
    endPLAChunk(stream);
    beginPLAChunk(stream, 6);
    writePLAVector4ArrayAttribute(stream, &shape->m_vertexColors);
    endPLAChunk(stream);
    beginPLAChunk(stream, 14);
    writePLAVector4ArrayAttribute(stream, &shape->m_strokeColors);
    endPLAChunk(stream);
    beginPLAChunk(stream, 15);
    writePLAFloatArrayAttribute(stream, &shape->m_strokeWidths);
    endPLAChunk(stream);
    beginPLAChunk(stream, 13);
    writePLAValue(stream, &shape->m_flags);
    endPLAChunk(stream);
    endPLAChunk(stream);
}

void writePLASmoothMeshShape(std::ostream &stream, SmoothMeshShape *shape)
{
    beginPLAChunk(stream, 3);
    beginPLAChunk(stream, MeshChunk_Name);
    writePLAString(stream, narrowString(shape->getName()));
    endPLAChunk(stream);
    for (int i = 0; i < static_cast<int>(shape->m_faces.size()); ++i) {
        beginPLAChunk(stream, MeshChunk_Face);
        {
            unsigned int count = shape->m_faces[i].size();
            writePLAValue(stream, &count);
        }

        // The writer takes the first element even for an empty face.
        stream.write(reinterpret_cast<const char *>(&shape->m_faces[i][0]), shape->m_faces[i].size() * 4);
        endPLAChunk(stream);
    }
    beginPLAChunk(stream, MeshChunk_VertexFlags);
    {
        unsigned int count = shape->m_vertexFlags.size();
        writePLAValue(stream, &count);
    }
    if (!shape->m_vertexFlags.empty())
        stream.write(reinterpret_cast<const char *>(&shape->m_vertexFlags[0]), shape->m_vertexFlags.size() * 4);
    endPLAChunk(stream);
    beginPLAChunk(stream, MeshChunk_Positions);
    writePLAVector2ArrayAttribute(stream, &shape->m_vertexPositions);
    endPLAChunk(stream);
    beginPLAChunk(stream, MeshChunk_TexCoords);
    writePLAVector2ArrayAttribute(stream, &shape->m_vertexTexCoords);
    endPLAChunk(stream);
    beginPLAChunk(stream, MeshChunk_Colors);
    writePLAVector4ArrayAttribute(stream, &shape->m_vertexColors);
    endPLAChunk(stream);
    beginPLAChunk(stream, MeshChunk_StrokeColors);
    writePLAVector4ArrayAttribute(stream, &shape->m_strokeColors);
    endPLAChunk(stream);
    beginPLAChunk(stream, MeshChunk_StrokeWidths);
    writePLAFloatArrayAttribute(stream, &shape->m_strokeWidths);
    endPLAChunk(stream);
    beginPLAChunk(stream, MeshChunk_Texture);
    writePLAIntAttribute(stream, &shape->m_texture);
    endPLAChunk(stream);
    beginPLAChunk(stream, MeshChunk_TextureTranslation);
    writePLAVector2Attribute(stream, &shape->m_textureTranslation);
    endPLAChunk(stream);
    beginPLAChunk(stream, MeshChunk_TextureRotation);
    writePLAVector3Attribute(stream, &shape->m_textureRotation);
    endPLAChunk(stream);
    beginPLAChunk(stream, MeshChunk_TextureDeformation);
    writePLAMatrixAttribute(stream, &shape->m_textureDeformation);
    endPLAChunk(stream);
    beginPLAChunk(stream, MeshChunk_TexturePivot);
    writePLAVector2Attribute(stream, &shape->m_texturePivot);
    endPLAChunk(stream);
    beginPLAChunk(stream, MeshChunk_Subdivisions);
    {
        int subdivisions = shape->getSubdivisions();
        writePLAValue(stream, &subdivisions);
    }
    endPLAChunk(stream);
    beginPLAChunk(stream, MeshChunk_Flags);
    writePLAValue(stream, &shape->m_flags);
    endPLAChunk(stream);
    beginPLAChunk(stream, MeshChunk_SmoothWeight);
    {
        float weight = shape->m_smoothWeight;
        writePLAValue(stream, &weight);
    }
    endPLAChunk(stream);
    endPLAChunk(stream);
}

void writePLACurveShape(std::ostream &stream, CurveShape *shape)
{
    {
        beginPLAChunk(stream, 10);
        beginPLAChunk(stream, CurveChunk_Name);
        writePLAString(stream, narrowString(shape->getName()));
        endPLAChunk(stream);
    }
    beginPLAChunk(stream, CurveChunk_VertexFlags);
    {
        unsigned int count = shape->m_vertexFlags.size();
        writePLAValue(stream, &count);
    }
    if (!shape->m_vertexFlags.empty())
        stream.write(reinterpret_cast<const char *>(&shape->m_vertexFlags[0]), shape->m_vertexFlags.size() * 4);
    endPLAChunk(stream);
    beginPLAChunk(stream, CurveChunk_Positions);
    writePLAVector2ArrayAttribute(stream, &shape->m_vertexPositions);
    endPLAChunk(stream);
    beginPLAChunk(stream, CurveChunk_TexCoords);
    writePLAVector2ArrayAttribute(stream, &shape->m_vertexTexCoords);
    endPLAChunk(stream);
    beginPLAChunk(stream, CurveChunk_Colors);
    writePLAVector4ArrayAttribute(stream, &shape->m_vertexColors);
    endPLAChunk(stream);
    beginPLAChunk(stream, CurveChunk_Widths);
    writePLAFloatArrayAttribute(stream, &shape->m_vertexWidths);
    endPLAChunk(stream);
    {
        beginPLAChunk(stream, CurveChunk_Subdivisions);
        int value = shape->m_subdivisions;
        writePLAValue(stream, &value);
        endPLAChunk(stream);
    }
    {
        beginPLAChunk(stream, CurveChunk_Closed);
        int value = shape->m_closed;
        writePLAValue(stream, &value);
        endPLAChunk(stream);
    }
    beginPLAChunk(stream, CurveChunk_StrokeCap);
    {
        int value = shape->m_strokeCapType;
        writePLAValue(stream, &value);
    }
    endPLAChunk(stream);
    {
        beginPLAChunk(stream, CurveChunk_StrokeSegments);
        int value = shape->m_strokeSegments;
        writePLAValue(stream, &value);
        endPLAChunk(stream);
    }
    endPLAChunk(stream);
}

void writePLATextShape(std::ostream &stream, TextShape *shape)
{
    beginPLAChunk(stream, 4);
    beginPLAChunk(stream, 1);
    writePLAString(stream, narrowString(shape->getName()));
    endPLAChunk(stream);
    beginPLAChunk(stream, 2);
    for (int i = 0; i < shape->m_string.size(); ++i) {
        beginPLAChunk(stream, 1);
        writePLAWString(stream, shape->m_string.m_values[i]);
        endPLAChunk(stream);
    }
    writePLASequences(stream, &shape->m_string);
    endPLAChunk(stream);
    beginPLAChunk(stream, 3);
    writePLAVector4Attribute(stream, &shape->m_color);
    endPLAChunk(stream);
    endPLAChunk(stream);
}

void writePLAGenericShape(std::ostream &stream, GenericShape *shape)
{
    beginPLAChunk(stream, 24);
    beginPLAChunk(stream, 1);
    writePLAString(stream, narrowString(shape->getName()));
    endPLAChunk(stream);
    beginPLAChunk(stream, 2);
    writePLAString(stream, narrowString(shape->m_source));
    endPLAChunk(stream);
    beginPLAChunk(stream, 3);
    writePLAVector2(stream, shape->m_position);
    endPLAChunk(stream);
    beginPLAChunk(stream, 4);
    writePLAVector2(stream, shape->m_size);
    endPLAChunk(stream);
    endPLAChunk(stream);
}

void writePLAShape(std::ostream &stream, Shape *shape)
{
    if (dynamic_cast<CurveShape *>(shape)) {
        writePLACurveShape(stream, dynamic_cast<CurveShape *>(shape));
    } else if (shape && shape->getType() == SHAPE_SMOOTH_MESH) {
        writePLASmoothMeshShape(stream, static_cast<SmoothMeshShape *>(shape));
    } else if (dynamic_cast<MeshShape *>(shape)) {
        writePLAMeshShape(stream, dynamic_cast<MeshShape *>(shape));
    } else if (shape) {
        if (shape->getType() == SHAPE_TEXT) {
            writePLATextShape(stream, static_cast<TextShape *>(shape));
        } else if (shape->getType() == SHAPE_GENERIC) {
            writePLAGenericShape(stream, static_cast<GenericShape *>(shape));
        }
    }
}

void writePLAWidgetProperties(std::ostream &stream, Widget *widget)
{
    beginPLAChunk(stream, WidgetChunk_Name);
    writePLAString(stream, narrowString(widget->getName()));
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_Caption);
    writePLAWString(stream, widget->m_caption);
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_InnerBindPosition);
    writePLAVector2(stream, widget->getInnerBindPos());
    endPLAChunk(stream);

    // The writer emits the position chunk twice.
    beginPLAChunk(stream, 6);
    writePLAVector2(stream, widget->getInnerBindPos());
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_InnerBindSize);
    writePLAVector2(stream, widget->getInnerBindSize());
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_BindPosition);
    writePLAVector2(stream, widget->getBindPos());
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_BindSize);
    writePLAVector2(stream, widget->getBindSize());
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_FramePosition);
    writePLAVector2(stream, widget->getFramePos());
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_FrameSize);
    writePLAVector2(stream, widget->getFrameSize());
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_BindMatrix);
    writePLAMatrix(stream, widget->m_bindMatrix);
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_HorizontalAlignment);
    {
        int value = widget->m_horizontalAlignment;
        writePLAValue(stream, &value);
    }
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_VerticalAlignment);
    {
        int value = widget->m_verticalAlignment;
        writePLAValue(stream, &value);
    }
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_StretchX);
    {
        int value = widget->stretchesX();
        writePLAValue(stream, &value);
    }
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_StretchY);
    {
        int value = widget->stretchesY();
        writePLAValue(stream, &value);
    }
    endPLAChunk(stream);
}

void writePLAWidget(std::ostream &stream, Widget *widget)
{
    beginPLAChunk(stream, 12);
    writePLAWidgetProperties(stream, widget);
    endPLAChunk(stream);
}

void writePLAButtonProperties(std::ostream &stream, Button *button)
{
    writePLAWidgetProperties(stream, button);
    beginPLAChunk(stream, WidgetChunk_ButtonType);
    int value = button->m_type;
    writePLAValue(stream, &value);
    endPLAChunk(stream);
    beginPLAChunk(stream, 16);
    value = button->m_checked != 0;
    writePLAValue(stream, &value);
    endPLAChunk(stream);
}

void writePLAButton(std::ostream &stream, Button *button)
{
    beginPLAChunk(stream, 13);
    writePLAButtonProperties(stream, button);
    endPLAChunk(stream);
}

void writePLAScrollButtonProperties(std::ostream &stream, ScrollButton *button, std::map<Widget *, int> &widgetIds)
{
    writePLAWidgetProperties(stream, button);
    beginPLAChunk(stream, WidgetChunk_ScrollDirection);
    {
        int direction = button->m_direction;
        writePLAValue(stream, &direction);
    }
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_ScrollStep);
    {
        float step = button->m_step;
        writePLAValue(stream, &step);
    }
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_ScrollTargetId);
    {
        Widget *target = button->m_target;
        writePLAValue(stream, &widgetIds[target]);
    }
    endPLAChunk(stream);
}

void writePLAScrollButton(std::ostream &stream, ScrollButton *button, std::map<Widget *, int> &widgetIds)
{
    beginPLAChunk(stream, 17);
    writePLAScrollButtonProperties(stream, button, widgetIds);
    endPLAChunk(stream);
}

void writePLAScrollSliderProperties(std::ostream &stream, ScrollSlider *slider, std::map<Widget *, int> &widgetIds)
{
    writePLAWidgetProperties(stream, slider);
    beginPLAChunk(stream, WidgetChunk_SliderDirection);
    {
        int direction = slider->m_direction;
        writePLAValue(stream, &direction);
    }
    endPLAChunk(stream);
    beginPLAChunk(stream, WidgetChunk_SliderTargetId);
    {
        Widget *target = slider->m_target;
        writePLAValue(stream, &widgetIds[target]);
    }
    endPLAChunk(stream);
}

void writePLAScrollSlider(std::ostream &stream, ScrollSlider *slider, std::map<Widget *, int> &widgetIds)
{
    beginPLAChunk(stream, 18);
    writePLAScrollSliderProperties(stream, slider, widgetIds);
    endPLAChunk(stream);
}

void writePLAPopUpButton(std::ostream &stream, PopUpButton *button)
{
    beginPLAChunk(stream, 14);
    writePLAWidgetProperties(stream, button);
    endPLAChunk(stream);
}

void writePLAListWidget(std::ostream &stream, ListWidget *widget)
{
    beginPLAChunk(stream, 19);
    writePLAWidgetProperties(stream, widget);
    endPLAChunk(stream);
}

void writePLAWidgetObject(std::ostream &stream, Widget *widget, std::map<Widget *, int> &widgetIds)
{
    if (dynamic_cast<ListWidget *>(widget)) {
        writePLAListWidget(stream, dynamic_cast<ListWidget *>(widget));
    } else if (dynamic_cast<ScrollButton *>(widget)) {
        writePLAScrollButton(stream, dynamic_cast<ScrollButton *>(widget), widgetIds);
    } else if (dynamic_cast<ScrollSlider *>(widget)) {
        writePLAScrollSlider(stream, dynamic_cast<ScrollSlider *>(widget), widgetIds);
    } else if (Button::cast(widget)) {
        writePLAButton(stream, static_cast<Button *>(widget));
    } else if (widget && widget->m_kind == WIDGET_EDIT) { // The code: an Edit takes the pop-up button writer.
        writePLAPopUpButton(stream, static_cast<PopUpButton *>(widget));
    } else {
        // The code UB: null falls through to the ordinary widget writer.
        writePLAWidget(stream, widget);
    }
}

void Engine::writePLATransformation(std::ostream &stream, Transformation *transformation)
{
    beginPLAChunk(stream, 1);
    beginPLAChunk(stream, TransformationChunk_Name);
    writePLAString(stream, narrowString(transformation->getName()));
    endPLAChunk(stream);
    beginPLAChunk(stream, TransformationChunk_Translation);
    writePLAVector2Attribute(stream, &transformation->m_translation);
    endPLAChunk(stream);
    beginPLAChunk(stream, TransformationChunk_Rotation);
    writePLAVector3Attribute(stream, &transformation->m_rotation);
    endPLAChunk(stream);
    beginPLAChunk(stream, TransformationChunk_Pivot);
    writePLAVector2Attribute(stream, &transformation->m_pivot);
    endPLAChunk(stream);
    beginPLAChunk(stream, TransformationChunk_Deformation);
    writePLAMatrixAttribute(stream, &transformation->m_deformation);
    endPLAChunk(stream);
    endPLAChunk(stream);
}

void writePLAMovie(std::ostream &stream, Movie *movie)
{
    beginPLAChunk(stream, 8);
    beginPLAChunk(stream, 1);
    writePLAString(stream, narrowString(movie->getName()));
    endPLAChunk(stream);
    for (int i = 0; i < static_cast<int>(movie->m_entries.size()); ++i) {
        beginPLAChunk(stream, 2);
        beginPLAChunk(stream, 3);
        writePLAString(stream, narrowString(movie->m_entries[i].name));
        endPLAChunk(stream);
        beginPLAChunk(stream, 2);
        int value = movie->m_entries[i].value;
        writePLAValue(stream, &value);
        endPLAChunk(stream);
        endPLAChunk(stream);
    }
    endPLAChunk(stream);
}

Movie *__stdcall ReadPLAMovie(std::istream &stream, Engine *engine, std::map<int, std::string> &names)
{
    const std::streampos end = readPLAChunkEnd(stream);
    Movie *movie = 0;
    while (stream.tellg() != end && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case 1:
            readPLAChunkEnd(stream);
            movie = engine->createMovie(widenString(readPLAString(stream)));
            break;
        case 2: {
            const std::streampos entryEnd = readPLAChunkEnd(stream);
            std::string name;

            // The code malformed-file UB: a missing value chunk leaves this uninitialised, and an entry before the name
            // dereferences null.
            int value;
            while (stream.tellg() != entryEnd && !stream.eof()) {
                switch (readPLAInt(stream)) {
                case 1:
                    readPLAChunkEnd(stream);
                    name = names[readPLAInt(stream)];
                    break;
                case 2:
                    readPLAChunkEnd(stream);
                    value = readPLAInt(stream);
                    break;
                case 3:
                    readPLAChunkEnd(stream);
                    name = readPLAString(stream);
                    break;
                default:
                    skipPLAChunk(stream);
                    break;
                }
            }
            movie->addEntry(widenString(name), value);
            break;
        }
        default:
            skipPLAChunk(stream);
            break;
        }
    }
    return movie;
}

void writePLATexture(std::ostream &stream, Texture *texture)
{
    beginPLAChunk(stream, 5);
    beginPLAChunk(stream, TextureChunk_Name);
    writePLAString(stream, narrowString(texture->getName()));
    endPLAChunk(stream);
    beginPLAChunk(stream, TextureChunk_Id);
    int value = texture->m_id;
    writePLAValue(stream, &value);
    endPLAChunk(stream);
    beginPLAChunk(stream, TextureChunk_PixelFormat);
    writePLAValue(stream, &texture->m_format.pixelFormat);
    endPLAChunk(stream);
    beginPLAChunk(stream, TextureChunk_MinFilter);
    writePLAValue(stream, &texture->m_format.minFilter);
    endPLAChunk(stream);
    beginPLAChunk(stream, TextureChunk_MaxFilter);
    writePLAValue(stream, &texture->m_format.maxFilter);
    endPLAChunk(stream);
    beginPLAChunk(stream, TextureChunk_HorizontalWrap);
    writePLAValue(stream, &texture->m_format.horizontalWrap);
    endPLAChunk(stream);
    beginPLAChunk(stream, TextureChunk_VerticalWrap);
    writePLAValue(stream, &texture->m_format.verticalWrap);
    endPLAChunk(stream);
    beginPLAChunk(stream, TextureChunk_Width);
    value = texture->m_width;
    writePLAValue(stream, &value);
    endPLAChunk(stream);
    beginPLAChunk(stream, TextureChunk_Height);
    value = texture->m_height;
    writePLAValue(stream, &value);
    endPLAChunk(stream);
    beginPLAChunk(stream, TextureChunk_Pixels);
    int count = static_cast<int>(texture->m_pixels.size());
    writePLAValue(stream, &count);

    // The code takes the first element even for an empty pixel buffer.
    stream.write(reinterpret_cast<const char *>(&texture->m_pixels[0]), texture->m_pixels.size());
    endPLAChunk(stream);
    endPLAChunk(stream);
}

void collectPLANodes(Node *node, std::list<Node *> &nodes)
{
    if (node->isShown() && !node->isInternal()) {
        nodes.push_back(node);
        for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
            collectPLANodes(*it, nodes);
    }
}

void Engine::writePlasmaFile(const std::wstring &path, Node *root, Page *page, unsigned __int64 key)
{
    int dot = path.find_last_of(L'.');
    std::wstring extension = path.substr(dot + 1, path.size() - dot - 1);

    // The code applies narrow toupper to wchar_t values, including its out-of-domain UB.
    std::transform(extension.begin(), extension.end(), extension.begin(), toupper);
    std::ofstream stream(PLASMA_FILE_PATH(path.c_str()), std::ios::binary);
    if (extension == L"PLA") {
        writePLA(stream, root, page);
    } else {
        if (extension != L"PLG" && extension != L"PLD" && extension != L"PLX")
            return;
        writePLX(stream, root, page, key);
    }
}

void Engine::writePLX(std::ostream &stream, Node *root, Page *page, unsigned __int64 key)
{
    if (!page)
        page = &m_page;
    PLXStreamWriter(this, page, root, stream, key).write();
}

void Engine::writePLA(std::ostream &stream, Node *root, Page *page)
{
    if (!page)
        page = &m_page;
    if (!root)
        root = m_root;
    std::list<Node *> nodes;
    collectPLANodes(root, nodes);
    std::set<Shape *> shapes;
    std::set<Transformation *> transformations;
    std::set<Widget *> widgets;
    for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->getShape())
            shapes.insert((*it)->getShape());
        if ((*it)->transformation())
            transformations.insert((*it)->transformation());
        if ((*it)->widget())
            widgets.insert((*it)->widget());
    }
    for (std::list<Texture *>::iterator it = m_textures.begin(); it != m_textures.end(); ++it) {
        if (!(*it)->m_transient)
            writePLATexture(stream, *it);
    }
    std::map<Shape *, int> shapeIds;
    shapeIds[0] = -1;
    int index = 0;
    for (std::list<Shape *>::iterator it = m_shapes.begin(); it != m_shapes.end(); ++it) {
        if (shapes.find(*it) != shapes.end()) {
            writePLAShape(stream, *it);
            shapeIds[*it] = index++;
        }
    }
    std::map<Transformation *, int> transformationIds;
    transformationIds[0] = -1;
    index = 0;
    for (std::list<Transformation *>::iterator it = m_transformations.begin(); it != m_transformations.end(); ++it) {
        if (transformations.find(*it) != transformations.end()) {
            writePLATransformation(stream, *it);
            transformationIds[*it] = index++;
        }
    }
    std::map<Widget *, int> widgetIds;
    widgetIds[0] = -1;
    index = 0;
    for (std::list<Widget *>::iterator it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (widgets.find(*it) != widgets.end())
            widgetIds[*it] = index++;
    }
    for (std::list<Widget *>::iterator it = m_widgets.begin(); it != m_widgets.end(); ++it) {
        if (widgets.find(*it) != widgets.end())
            writePLAWidgetObject(stream, *it, widgetIds);
    }
    std::map<Node *, int> nodeIds;
    nodeIds[0] = -1;
    index = 0;
    for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->isShown() && !(*it)->isInternal())
            nodeIds[*it] = index++;
    }
    for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (!(*it)->isShown() || (*it)->isInternal())
            continue;
        beginPLAChunk(stream, 7);
        beginPLAChunk(stream, 1);
        writePLAString(stream, narrowString((*it)->getName()));
        endPLAChunk(stream);
        beginPLAChunk(stream, 4);
        writePLAValue(stream, &shapeIds[(*it)->getShape()]);
        endPLAChunk(stream);
        beginPLAChunk(stream, 5);
        writePLAValue(stream, &transformationIds[(*it)->transformation()]);
        endPLAChunk(stream);
        beginPLAChunk(stream, 6);
        writePLAValue(stream, &widgetIds[(*it)->widget()]);
        endPLAChunk(stream);
        beginPLAChunk(stream, 2);
        writePLAValue(stream, &nodeIds[(*it)->getParent()]);
        endPLAChunk(stream);
        for (std::list<Node *>::iterator child = (*it)->m_children.begin(); child != (*it)->m_children.end(); ++child) {
            if ((*child)->isShown() && !(*child)->isInternal()) {
                beginPLAChunk(stream, 3);
                writePLAValue(stream, &nodeIds[*child]);
                endPLAChunk(stream);
            }
        }
        beginPLAChunk(stream, 7);
        writePLAValue(stream, &(*it)->m_flags);
        endPLAChunk(stream);
        for (std::list<std::string>::iterator key = (*it)->m_variableKeys.begin(); key != (*it)->m_variableKeys.end();
             ++key) {
            beginPLAChunk(stream, 10);
            writePLAString(stream, *key);
            writePLAString(stream, (*it)->getVariable(*key));
            endPLAChunk(stream);
        }
        endPLAChunk(stream);
    }
    for (std::map<std::wstring, Movie *>::iterator movie = m_movies.begin(); movie != m_movies.end(); ++movie)
        writePLAMovie(stream, movie->second);
    beginPLAChunk(stream, 20);
    writePLAValue(stream, &page->size.m_elements[0]);
    endPLAChunk(stream);
    beginPLAChunk(stream, 21);
    writePLAValue(stream, &page->size.m_elements[1]);
    endPLAChunk(stream);
    beginPLAChunk(stream, 22);
    writePLAValue(stream, &page->size.m_elements[2]);
    endPLAChunk(stream);
    beginPLAChunk(stream, 23);
    writePLAValue(stream, &page->unit);
    endPLAChunk(stream);
}

SmoothMeshShape *__fastcall readPLAMesh(std::map<int, int> &textureIds, Engine *engine, std::istream &stream,
                                        std::map<int, std::string> &names)
{
    std::streampos end = readPLAChunkEnd(stream);
    SmoothMeshShape *shape = engine->createSmoothMeshShape();
    std::vector<std::vector<int>> faces;
    int subdivisions = shape->m_subdivisions;
    float smoothWeight = shape->m_smoothWeight;
    while (stream.tellg() != end && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case MeshChunk_Name:
            readPLAChunkEnd(stream);
            shape->setName(widenString(readPLAString(stream)));
            break;
        case MeshChunk_Face: {
            readPLAChunkEnd(stream);
            int count = readPLAInt(stream);
            std::vector<int> face(count);

            // The code takes the first element even when count is zero.
            stream.read(reinterpret_cast<char *>(&face[0]), count * sizeof(int));
            faces.push_back(face);
            break;
        }
        case MeshChunk_VertexFlags: {
            readPLAChunkEnd(stream);
            int count = readPLAInt(stream);
            if (count > 0) {
                shape->m_vertexFlags.resize(count);
                stream.read(reinterpret_cast<char *>(&shape->m_vertexFlags[0]), count * sizeof(int));
            }
            break;
        }
        case MeshChunk_Positions:
            readPLAVector2ArrayAttribute(stream, &shape->m_vertexPositions, names);
            break;
        case MeshChunk_TexCoords:
            readPLAVector2ArrayAttribute(stream, &shape->m_vertexTexCoords, names);
            break;
        case MeshChunk_Colors:
            readPLAVector4ArrayAttribute(stream, &shape->m_vertexColors, names);
            break;
        case MeshChunk_StrokeColors:
            readPLAVector4ArrayAttribute(stream, &shape->m_strokeColors, names);
            break;
        case MeshChunk_StrokeWidths:
            readPLAFloatArrayAttribute(stream, &shape->m_strokeWidths, names);
            break;
        case MeshChunk_Texture: {
            readPLAIntAttribute(stream, &shape->m_texture, names);
            for (int i = 0; i < shape->m_texture.size(); ++i)
                shape->m_texture.m_values[i] = textureIds[shape->m_texture.m_values[i]];
            break;
        }
        case MeshChunk_TextureTranslation:
            readPLAVector2Attribute(stream, &shape->m_textureTranslation, names);
            break;
        case MeshChunk_TextureRotation:
            readPLAVector3Attribute(stream, &shape->m_textureRotation, names);
            break;
        case MeshChunk_TextureDeformation:
            readPLAMatrixAttribute(stream, &shape->m_textureDeformation, names);
            break;
        case MeshChunk_TexturePivot:
            readPLAVector2Attribute(stream, &shape->m_texturePivot, names);
            break;
        case MeshChunk_Subdivisions:
            readPLAChunkEnd(stream);
            subdivisions = readPLAInt(stream);
            break;
        case MeshChunk_SmoothWeight:
            readPLAChunkEnd(stream);
            smoothWeight = readPLAFloat(stream);
            break;
        case MeshChunk_Flags:
            readPLAChunkEnd(stream);
            shape->m_flags = readPLAInt(stream);
            break;
        default:
            skipPLAChunk(stream);
            break;
        }
    }
    shape->m_smoothWeight = smoothWeight;
    shape->setFaces(faces, false);
    shape->setSubdivisions(subdivisions, true);
    return shape;
}

CurveShape *__fastcall ReadPLACurve(Engine *engine, std::istream &stream, std::map<int, std::string> &names)
{
    const std::streampos chunkEnd = readPLAChunkEnd(stream);
    CurveShape *shape = engine->createCurveShape();
    std::vector<int> vertexTypes;
    int subdivisions = shape->m_subdivisions;
    bool closed = shape->m_closed;
    int strokeCapType = shape->m_strokeCapType;
    int strokeSegments = shape->m_strokeSegments;
    while (stream.tellg() != chunkEnd && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case CurveChunk_Name:
            readPLAChunkEnd(stream);
            shape->setName(widenString(readPLAString(stream)));
            break;
        case CurveChunk_VertexFlags: {
            readPLAChunkEnd(stream);
            int count = readPLAInt(stream);
            if (count > 0) {
                vertexTypes.resize(count);
                stream.read(reinterpret_cast<char *>(&vertexTypes[0]), count * sizeof(int));
            }
            break;
        }
        case CurveChunk_Positions:
            readPLAVector2ArrayAttribute(stream, &shape->m_vertexPositions, names);
            break;
        case CurveChunk_TexCoords:
            readPLAVector2ArrayAttribute(stream, &shape->m_vertexTexCoords, names);
            break;
        case CurveChunk_Colors:
            readPLAVector4ArrayAttribute(stream, &shape->m_vertexColors, names);
            break;
        case CurveChunk_Widths:
            readPLAFloatArrayAttribute(stream, &shape->m_vertexWidths, names);
            break;
        case CurveChunk_Subdivisions:
            readPLAChunkEnd(stream);
            subdivisions = readPLAInt(stream);
            break;
        case CurveChunk_Closed:
            readPLAChunkEnd(stream);
            closed = readPLAInt(stream) != 0;
            break;
        case CurveChunk_StrokeCap:
            readPLAChunkEnd(stream);
            strokeCapType = readPLAInt(stream);
            break;
        case CurveChunk_StrokeSegments:
            readPLAChunkEnd(stream);
            strokeSegments = readPLAInt(stream);
            break;
        default:
            skipPLAChunk(stream);
            break;
        }
    }
    shape->setVertexFlags(vertexTypes);
    shape->setSubdivisions(subdivisions);
    shape->setClosed(closed);
    shape->m_strokeCapType = strokeCapType;
    shape->setStrokeSegments(strokeSegments);
    shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return shape;
}

Transformation *Engine::readPLATransformation(std::istream &stream, std::map<int, std::string> &names)
{
    const std::streampos chunkEnd = readPLAChunkEnd(stream);
    Transformation *transformation = createTransformation();
    while (stream.tellg() != chunkEnd && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case 1:
            readPLAChunkEnd(stream);
            transformation->setName(widenString(readPLAString(stream)));
            break;
        case 2:
            readPLAVector2Attribute(stream, &transformation->m_translation, names);
            break;
        case 3:
            readPLAVector3Attribute(stream, &transformation->m_rotation, names);
            break;
        case 6:
            readPLAMatrixAttribute(stream, &transformation->m_deformation, names);
            break;
        case 5:
            readPLAVector2Attribute(stream, &transformation->m_pivot, names);
            break;
        default:
            skipPLAChunk(stream);
            break;
        }
    }
    transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return transformation;
}

// Reads one property chunk of an engine PLA widget. Returns false if the identifier is not a widget property.
bool __fastcall readPLAWidgetProperty(std::istream &stream, Widget *widget, int identifier)
{
    switch (identifier) {
    case WidgetChunk_Name:
        readPLAChunkEnd(stream);
        widget->setName(widenString(readPLAString(stream)));
        return true;
    case WidgetChunk_Caption:
        readPLAChunkEnd(stream);
        widget->setCaption(readPLAWString(stream), false);
        return true;
    case WidgetChunk_InnerBindPosition: {
        readPLAChunkEnd(stream);
        widget->m_innerBindPos = readPLAVector2(stream);
        return true;
    }
    case WidgetChunk_InnerBindSize: {
        readPLAChunkEnd(stream);
        widget->m_innerBindSize = readPLAVector2(stream);
        return true;
    }
    case WidgetChunk_BindPosition: {
        readPLAChunkEnd(stream);
        widget->m_bindPos = readPLAVector2(stream);
        return true;
    }
    case WidgetChunk_BindSize: {
        readPLAChunkEnd(stream);
        widget->m_bindSize = readPLAVector2(stream);
        return true;
    }
    case WidgetChunk_FramePosition: {
        readPLAChunkEnd(stream);
        widget->m_framePos = readPLAVector2(stream);
        return true;
    }
    case WidgetChunk_FrameSize: {
        readPLAChunkEnd(stream);
        widget->m_frameSize = readPLAVector2(stream);
        return true;
    }
    case WidgetChunk_BindMatrix: {
        readPLAChunkEnd(stream);
        widget->m_bindMatrix = readPLAMatrix(stream);
        widget->m_inverseBindMatrix = widget->m_bindMatrix;
        widget->m_inverseBindMatrix.invert();
        return true;
    }
    case WidgetChunk_StretchX:
        readPLAChunkEnd(stream);
        widget->setStretchesX(readPLAInt(stream) == 1);
        return true;
    case WidgetChunk_StretchY:
        readPLAChunkEnd(stream);
        widget->setStretchesY(readPLAInt(stream) == 1);
        return true;
    case WidgetChunk_HorizontalAlignment:
        readPLAChunkEnd(stream);
        widget->m_horizontalAlignment = readPLAInt(stream);
        return true;
    case WidgetChunk_VerticalAlignment:
        readPLAChunkEnd(stream);
        widget->m_verticalAlignment = readPLAInt(stream);
        return true;
    default:
        return false;
    }
}

bool readPLAButtonProperty(std::istream &stream, Button *button, int identifier)
{
    // The reader does not consume the writer's checked-state chunk 16.
    if (identifier != 15)
        return readPLAWidgetProperty(stream, button, identifier);
    readPLAChunkEnd(stream);
    button->m_type = readPLAInt(stream);
    return true;
}

bool readPLAScrollButtonProperty(std::istream &stream, ScrollButton *button, int identifier)
{
    switch (identifier) {
    case WidgetChunk_ScrollDirection:
        readPLAChunkEnd(stream);
        button->m_direction = readPLAInt(stream);
        return true;
    case WidgetChunk_ScrollStep:
        readPLAChunkEnd(stream);
        button->m_step = readPLAFloat(stream);
        return true;
    case WidgetChunk_ScrollTargetId:
        readPLAChunkEnd(stream);
        button->m_targetId = readPLAInt(stream);
        return true;
    default:
        return readPLAWidgetProperty(stream, button, identifier);
    }
}

bool readPLAScrollSliderProperty(std::istream &stream, ScrollSlider *slider, int identifier)
{
    switch (identifier) {
    case WidgetChunk_SliderDirection:
        readPLAChunkEnd(stream);
        slider->m_direction = readPLAInt(stream) != 0;
        return true;
    case WidgetChunk_SliderTargetId:
        readPLAChunkEnd(stream);
        slider->m_targetId = readPLAInt(stream);
        return true;
    default:
        return readPLAWidgetProperty(stream, slider, identifier);
    }
}

bool readPLAListWidgetProperty(std::istream &stream, ListWidget *widget, int identifier)
{
    return readPLAWidgetProperty(stream, widget, identifier);
}

Widget *__stdcall ReadPLAWidget(std::istream &stream, Engine *engine)
{
    const std::streampos end = readPLAChunkEnd(stream);
    Widget *widget = engine->createWidget(0);
    while (stream.tellg() != end && !stream.eof()) {
        int identifier = readPLAInt(stream);
        if (!readPLAWidgetProperty(stream, widget, identifier))
            skipPLAChunk(stream);
    }
    return widget;
}

Button *__stdcall ReadPLAButton(std::istream &stream, Engine *engine)
{
    const std::streampos end = readPLAChunkEnd(stream);
    Button *button = engine->createButton(0);
    while (stream.tellg() != end && !stream.eof()) {
        int identifier = readPLAInt(stream);
        if (!readPLAButtonProperty(stream, button, identifier))
            skipPLAChunk(stream);
    }
    return button;
}

Widget *__stdcall ReadPLAEdit(std::istream &stream, Engine *engine)
{
    const std::streampos end = readPLAChunkEnd(stream);
    Edit *widget = engine->createEdit(0);
    while (stream.tellg() != end && !stream.eof()) {
        int identifier = readPLAInt(stream);
        if (!readPLAWidgetProperty(stream, widget, identifier))
            skipPLAChunk(stream);
    }
    return widget;
}

Widget *__stdcall ReadPLAScrollButton(std::istream &stream, Engine *engine)
{
    const std::streampos end = readPLAChunkEnd(stream);
    ScrollButton *widget = engine->createScrollButton(0);
    while (stream.tellg() != end && !stream.eof()) {
        int identifier = readPLAInt(stream);
        if (!readPLAScrollButtonProperty(stream, widget, identifier))
            skipPLAChunk(stream);
    }
    return widget;
}

Widget *__stdcall ReadPLAScrollSlider(std::istream &stream, Engine *engine)
{
    const std::streampos end = readPLAChunkEnd(stream);
    ScrollSlider *widget = engine->createScrollSlider(0);
    while (stream.tellg() != end && !stream.eof()) {
        int identifier = readPLAInt(stream);
        if (!readPLAScrollSliderProperty(stream, widget, identifier))
            skipPLAChunk(stream);
    }
    return widget;
}

Widget *__stdcall ReadPLAListWidget(std::istream &stream, Engine *engine)
{
    const std::streampos end = readPLAChunkEnd(stream);
    ListWidget *widget = engine->createListWidget(0);
    while (stream.tellg() != end && !stream.eof()) {
        int identifier = readPLAInt(stream);
        if (!readPLAListWidgetProperty(stream, widget, identifier))
            skipPLAChunk(stream);
    }
    return widget;
}

TextShape *__fastcall readPLATextShape(Engine *engine, std::istream &stream, std::map<int, std::string> &names)
{
    const std::streampos end = readPLAChunkEnd(stream);

    // The PLA text reader constructs a wstring from a null pointer, reading address zero while finding its length.
    TextShape *shape = engine->createTextShape(0);
    while (stream.tellg() != end && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case 1:
            readPLAChunkEnd(stream);
            shape->setName(widenString(readPLAString(stream)));
            break;
        case 2: {
            const std::streampos stringsEnd = readPLAChunkEnd(stream);
            int index = 0;
            while (stream.tellg() != stringsEnd && !stream.eof()) {
                switch (readPLAInt(stream)) {
                case 1:
                    readPLAChunkEnd(stream);
                    if (index == shape->m_string.size())
                        shape->m_string.duplicateElement();
                    shape->m_string.m_values[index] = readPLAWString(stream);
                    ++index;
                    break;
                case 3:
                    readPLASequences(stream, &shape->m_string, names);
                    break;
                default:
                    skipPLAChunk(stream);
                    break;
                }
            }
            break;
        }
        case 3:
            readPLAVector4Attribute(stream, &shape->m_color, names);
            break;
        default:
            skipPLAChunk(stream);
            break;
        }
    }
    shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return shape;
}

GenericShape *readPLAGenericShape(std::istream &stream, Engine *engine)
{
    const std::streampos end = readPLAChunkEnd(stream);
    GenericShape *shape = engine->createGenericShape();
    while (stream.tellg() != end && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case GenericShapeChunk_Name:
            readPLAChunkEnd(stream);
            shape->setName(widenString(readPLAString(stream)));
            break;
        case GenericShapeChunk_Source:
            readPLAChunkEnd(stream);
            shape->setSource(widenString(readPLAString(stream)));
            break;
        case GenericShapeChunk_Position: {
            readPLAChunkEnd(stream);
            shape->m_position = readPLAVector2(stream);
            break;
        }
        case GenericShapeChunk_Size: {
            readPLAChunkEnd(stream);
            shape->m_size = readPLAVector2(stream);
            break;
        }
        default:
            skipPLAChunk(stream);
            break;
        }
    }
    shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return shape;
}

bool Engine::readPlasmaFileIntoNode(const std::wstring &path, Node *target, int flags, Page *page,
                                    const std::vector<unsigned __int64> &keys)
{
    int slash = path.find_last_of(L'/');
    int backslash = path.find_last_of(L'\\');
    int separator = (std::max)(slash, backslash);
    std::ifstream stream(PLASMA_FILE_PATH(path.c_str()), std::ios::binary);
    if (!stream.is_open())
        return false;
    std::wstring directory = path.substr(0, separator);
    m_fontPaths.push_front(directory);
    int dot = path.find_last_of(L'.');
    std::wstring extension = path.substr(dot + 1, path.size() - dot - 1);

    // The code applies narrow toupper to wchar_t values, including its out-of-domain UB.
    std::transform(extension.begin(), extension.end(), extension.begin(), toupper);
    stream.seekg(0);
    if (extension == L"PLA") {
        readPLAFile(stream, target, flags, page);
        stream.close();
        return true;
    }
    if (extension == L"PLG" || extension == L"PLD" || extension == L"PLX") {
        readPLXFile(stream, target, flags, page, keys);
        stream.close();
        return true;
    }
    stream.close();
    return false;
}

void Engine::readPLXFile(std::istream &stream, Node *target, int flags, Page *page,
                         const std::vector<unsigned __int64> &keys)
{
    if (!page)
        page = &m_page;
    PLXStreamReader(this, page, target, flags, stream, keys).read();
}

void Engine::readPLAFile(std::ifstream &stream, Node *target, int flags, Page *page)
{
    if (!page)
        page = &m_page;
    std::map<int, Node *> nodes;
    std::map<int, Shape *> shapes;
    std::map<int, Transformation *> transformations;
    std::map<int, Widget *> widgets;
    std::map<int, std::string> names;
    std::map<int, int> textureIds;
    std::map<int, int> fontIds;
    nodes[-1] = 0;
    shapes[-1] = 0;
    fontIds[-1] = -1;
    textureIds[-1] = -1;
    transformations[-1] = 0;
    widgets[-1] = 0;
    page->size.m_elements[2] = 100.0f;
    page->unit = 0;
    int shapeIndex = 0;
    int transformationIndex = 0;
    int widgetIndex = 0;
    int nodeIndex = 0;
    std::list<PLAFontReference> fonts;
    std::list<TextShape *> textShapes;
    while (!stream.eof()) {
        int identifier = readPLAInt(stream);
        if (stream.eof())
            break;
        switch (identifier) {
        case 9: {
            const std::streampos end = readPLAChunkEnd(stream);
            std::string name;
            int id = -1;
            while (stream.tellg() != end && !stream.eof()) {
                int nameIdentifier = readPLAInt(stream);
                if (nameIdentifier == 1) {
                    readPLAChunkEnd(stream);
                    id = readPLAInt(stream);
                } else if (nameIdentifier == 3) {
                    readPLAChunkEnd(stream);
                    name = readPLAString(stream);
                } else {
                    skipPLAChunk(stream);
                }
            }
            if (id != -1)
                names[id] = name;
            break;
        }
        case 5: {
            const std::streampos end = readPLAChunkEnd(stream);
            std::string name;
            int id = -1;
            TextureFormat format;
            int width = 0;
            int height = 0;
            std::vector<unsigned char> pixels;
            while (stream.tellg() != end && !stream.eof()) {
                switch (readPLAInt(stream)) {
                case 1:
                    readPLAChunkEnd(stream);
                    name = readPLAString(stream);
                    break;
                case 2:
                    readPLAChunkEnd(stream);
                    id = readPLAInt(stream);
                    break;
                case 3:
                    readPLAChunkEnd(stream);
                    format.pixelFormat = readPLAInt(stream);
                    break;
                case 4:
                    readPLAChunkEnd(stream);
                    format.minFilter = readPLAInt(stream);
                    break;
                case 5:
                    readPLAChunkEnd(stream);
                    format.maxFilter = readPLAInt(stream);
                    break;
                case 6:
                    readPLAChunkEnd(stream);
                    width = readPLAInt(stream);
                    break;
                case 7:
                    readPLAChunkEnd(stream);
                    height = readPLAInt(stream);
                    break;
                case 8: {
                    readPLAChunkEnd(stream);
                    int count = readPLAInt(stream);
                    if (count > 0) {
                        pixels.resize(count);
                        stream.read(reinterpret_cast<char *>(pixels.data()), count);
                    }
                    break;
                }
                case 9:
                    readPLAChunkEnd(stream);
                    format.horizontalWrap = readPLAInt(stream);
                    break;
                case 10:
                    readPLAChunkEnd(stream);
                    format.verticalWrap = readPLAInt(stream);
                    break;
                default:
                    skipPLAChunk(stream);
                    break;
                }
            }
            if (id != -1) {
                Texture *texture = createTexture(width, height, pixels.data(), format, widenString(name), false);
                textureIds[id] = texture->m_id;
            }
            break;
        }
        case 6: {
            const std::streampos end = readPLAChunkEnd(stream);
            PLAFontReference font;
            while (stream.tellg() != end && !stream.eof()) {
                switch (readPLAInt(stream)) {
                case 1:
                    readPLAChunkEnd(stream);
                    font.name = readPLAString(stream);
                    break;
                case 2:
                    readPLAChunkEnd(stream);
                    font.source = readPLAString(stream);
                    break;
                case 3:
                    readPLAChunkEnd(stream);
                    font.size = readPLAInt(stream);
                    break;
                case 4:
                    readPLAChunkEnd(stream);
                    font.id = readPLAInt(stream);
                    break;
                default:
                    skipPLAChunk(stream);
                    break;
                }
            }
            fonts.push_back(font);
            break;
        }
        case 4: {
            TextShape *shape = readPLATextShape(this, stream, names);
            textShapes.push_back(shape);
            shapes[shapeIndex++] = textShapes.back();
            break;
        }
        case 3:
            shapes[shapeIndex++] = readPLAMesh(textureIds, this, stream, names);
            break;
        case 24:
            shapes[shapeIndex++] = readPLAGenericShape(stream, this);
            break;
        case 10:
            shapes[shapeIndex++] = ReadPLACurve(this, stream, names);
            break;
        case 1:
            transformations[transformationIndex++] = readPLATransformation(stream, names);
            break;
        case 11:
            widgets[widgetIndex++] = ReadPLAWidget(stream, this);
            break;
        case 12:

            // Both legacy widget tags use this reader.
            widgets[widgetIndex++] = ReadPLAWidget(stream, this);
            break;
        case 13:
            widgets[widgetIndex++] = ReadPLAButton(stream, this);
            break;
        case 14:

            // The legacy reader treats tag 14 as Edit although the popup writer also emits this tag.
            widgets[widgetIndex++] = ReadPLAEdit(stream, this);
            break;
        case 19:
            widgets[widgetIndex++] = ReadPLAListWidget(stream, this);
            break;
        case 17:
            widgets[widgetIndex++] = ReadPLAScrollButton(stream, this);
            break;
        case 18:
            widgets[widgetIndex++] = ReadPLAScrollSlider(stream, this);
            break;
        case 7: {
            const std::streampos end = readPLAChunkEnd(stream);
            std::string name;
            Shape *shape = 0;
            Transformation *transformation = 0;
            Widget *widget = 0;
            std::list<int> childIds;

            // The legacy display attributes are read but discarded.
            DiscreteAttribute<int> visibility("", 0);
            DiscreteAttribute<int> clipping("", 0);

            // The code copies an uninitialised vector before setting the current colour.
            ContinuousAttribute<Vector<4, float>> fill("", Vector<4, float>());
            fill.value() = Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
            visibility.value() = 1;
            clipping.value() = 0;

            // Keep file order and repeated keys; SetProperty applies the last value while preserving the first insertion in
            // m_variableKeys.
            std::list<std::pair<std::string, std::string>> properties;

            // The code malformed-file UB: flags are uninitialised without tag 7.
            unsigned nodeFlags;
            while (stream.tellg() != end && !stream.eof()) {
                switch (readPLAInt(stream)) {
                case 1:
                    readPLAChunkEnd(stream);
                    name = readPLAString(stream);
                    break;
                case 2:
                    readPLAChunkEnd(stream);
                    readPLAInt(stream); // Parent id is superseded by child lists.
                    break;
                case 3:
                    readPLAChunkEnd(stream);
                    childIds.push_back(readPLAInt(stream));
                    break;
                case 4:
                    readPLAChunkEnd(stream);
                    shape = shapes[readPLAInt(stream)];
                    break;
                case 5:
                    readPLAChunkEnd(stream);
                    transformation = transformations[readPLAInt(stream)];
                    break;
                case 6:
                    readPLAChunkEnd(stream);
                    widget = widgets[readPLAInt(stream)];
                    break;
                case 7:
                    readPLAChunkEnd(stream);
                    nodeFlags = readPLAInt(stream);
                    break;
                case 8:
                    readPLAIntAttribute(stream, &visibility, names);
                    break;
                case 9:
                    readPLAIntAttribute(stream, &clipping, names);
                    break;
                case 10: {
                    readPLAChunkEnd(stream);
                    std::string key = readPLAString(stream);
                    std::string value = readPLAString(stream);
                    properties.push_back(std::make_pair(std::move(key), std::move(value)));
                    break;
                }
                case 11:
                    readPLAVector4Attribute(stream, &fill, names);
                    break;
                default:
                    skipPLAChunk(stream);
                    break;
                }
            }
            Node *node;
            if (nodeIndex) {
                node = createNode(transformation, shape, 0, 0, widenString(name));
                node->setWidget(widget);
                node->m_flags = nodeFlags;
                for (std::list<std::pair<std::string, std::string>>::iterator property = properties.begin();
                     property != properties.end(); ++property)
                    node->setVariable(property->first, property->second);
                if (flags & (1 << LOAD_INTERNAL))
                    node->m_flags |= (1 << NODE_INTERNAL);
            } else {
                node = target;
            }
            node->m_pendingChildIds = childIds;
            nodes[nodeIndex++] = node;
            break;
        }
        case 8:
            ReadPLAMovie(stream, this, names);
            break;
        case 20:
            readPLAChunkEnd(stream);
            page->size.m_elements[0] = readPLAFloat(stream);
            break;
        case 21:
            readPLAChunkEnd(stream);
            page->size.m_elements[1] = readPLAFloat(stream);
            break;
        case 22:
            readPLAChunkEnd(stream);
            page->size.m_elements[2] = readPLAFloat(stream);
            break;
        case 23:
            readPLAChunkEnd(stream);
            page->unit = readPLAInt(stream);
            break;
        default:
            skipPLAChunk(stream);
            break;
        }
    }
    stream.close();
    for (std::map<int, Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        Node *node = it->second;
        if (node) {
            std::list<int> &childIds = node->m_pendingChildIds;
            for (std::list<int>::iterator child = childIds.begin(); child != childIds.end(); ++child)
                node->addChild(nodes[*child]);
            childIds.clear();
        }
    }
    if (!(flags & (1 << LOAD_SHARED))) {
        for (std::map<int, Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            Node *node = it->second;
            if (node && (!node->m_parent || node->m_parent == target))
                node->makeUnique();
        }
    }
    for (std::list<PLAFontReference>::iterator it = fonts.begin(); it != fonts.end(); ++it) {
        Font *font = loadLegacyFont(widenString(it->source), static_cast<float>(it->size), it->stroke);
        if (font) {
            unsigned id = font->m_id;
            fontIds[it->id] = id;

            // The code reloads the font ID after inserting the legacy mapping.
            m_fontsById[font->m_id] = font;
        }
    }

    // The code iterates this list after font remapping without touching entries.
    for (std::list<TextShape *>::iterator it = textShapes.begin(); it != textShapes.end(); ++it) {
    }
    for (std::map<int, Widget *>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
        if (ScrollButton *button = dynamic_cast<ScrollButton *>(it->second))
            button->setTarget(widgets[button->m_targetId]);
        if (ScrollSlider *slider = dynamic_cast<ScrollSlider *>(it->second))
            slider->setTarget(widgets[slider->m_targetId]);
    }
    if (!(flags & (1 << LOAD_NO_WIDGET_INIT))) {
        int x = m_viewportX;
        int y = m_viewportY;
        int width = m_viewportWidth;
        int height = m_viewportHeight;
        setViewport(0, 0, static_cast<int>(page->size.m_elements[0]), static_cast<int>(page->size.m_elements[1]),
                    false);
        initializeWidgets();
        setViewport(x, y, width, height, true);
    }
}

Texture *Engine::findTexture(int id)
{
    std::map<int, Texture *>::iterator found = m_texturesById.find(id);
    if (found == m_texturesById.end())
        return 0;
    return found->second;
}

bool Engine::readSources(const std::wstring &path, unsigned __int64 key)
{
    std::vector<unsigned __int64> keys(1, key);
    return readPlasmaFileIntoNode(path, m_sources,
                                  (1 << LOAD_SHARED) | (1 << LOAD_NO_WIDGET_INIT) | (1 << LOAD_INTERNAL) |
                                      (1 << LOAD_IGNORE_PAGE) | (1 << LOAD_STATIC_MESHES) | (1 << LOAD_HINTED_TEXT),
                                  static_cast<Page *>(0), keys);
}

void Engine::resolveSources(Node *node)
{
    if (!node)
        node = m_root;
    resolveSources(m_sources, node);
}

void Engine::resolveSources(Node *sources, Node *node)
{
    std::map<std::wstring, Node *> sourceNodes;
    collectSourceNodes(sources, sourceNodes);
    resolveSourceNodes(sourceNodes, node);
}

void Engine::collectSourceNodes(Node *node, std::map<std::wstring, Node *> &sources)
{
    if (node->isSource())
        sources[node->getName()] = node;
    for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        collectSourceNodes(*it, sources);
}

void prefixSourceNodeNames(Node *node, const std::wstring &prefix)
{
    if (node->getName() != L"caption") {
        node->setName(prefix + node->getName());
        if (node->m_widget)
            node->m_widget->setName(node->getName());
        if (node->m_shape)
            node->m_shape->setName(node->getName());
        if (node->m_transformation)
            node->m_transformation->setName(node->getName());
    }
    for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        prefixSourceNodeNames(*it, prefix);
}

void Engine::resolveSourceNodes(std::map<std::wstring, Node *> &sources, Node *node)
{
    if (!node->isShown())
        return;
    if (node->m_shape && node->m_shape->getType() == SHAPE_GENERIC) {
        GenericShape *shape = static_cast<GenericShape *>(node->m_shape);

        // Work on a copy because destroying an old instance changes children.
        std::list<Node *> children = node->getChildren();
        for (std::list<Node *>::iterator it = children.begin(); it != children.end(); ++it) {
            if ((*it)->isSource())
                destroyNode(*it);
        }
        shape->m_instanceNode = 0;
        Node *source = sources[shape->m_source];
        if (source) {
            Node *instance = source->instance(0);
            instance->makeAnimatedUnique();
            node->insertChild(node->m_children.begin(), instance);
            prefixSourceNodeNames(instance, node->getName() + L":");
            instance->m_transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
            Widget *widget = instance->m_widget;
            if (widget) {
                if (node->m_widget)
                    widget->setViewRect(Vector<2, float>(0.0f, 0.0f), node->m_widget->getViewSize());
            } else {
                const Matrix<float> &inverse = node->m_transformation->m_inverseMatrix;
                Vector<2, float> delta = inverse * node->getMinimum(0) - instance->getMinimum(0);
                Vector<2, float> &translation = instance->m_transformation->m_translation.value();
                translation += delta;
                instance->m_transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
            }
            shape->m_instanceNode = instance;
            shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
            if (node->m_widget)
                node->m_widget->updateCaption();
        }
    }
    std::list<Node *> children = node->getChildren();
    for (std::list<Node *>::iterator it = children.begin(); it != children.end(); ++it)
        resolveSourceNodes(sources, *it);
}

void Engine::setPopUpWidget(Widget *widget, Widget *anchor)
{
    if (m_popUpWidget)
        m_popUpWidget->m_node->m_display->m_visibility.value() = 0;
    m_popUpWidget = widget;
    if (widget) {
        Node *parent = m_overlayRoot;
        Node *popupNode = widget->getNode();
        if (!parent)
            parent = m_root;
        popupNode->setParent(parent);
        if (anchor) {
            m_popUpWidget->setViewPosition(
                anchor->getContentPosition() + Vector<2, float>(0.0f, anchor->getViewHeight()), true);
        }
        if (m_popUpWidget->viewX() < 0.0f)
            m_popUpWidget->moveTo(0.0f, m_popUpWidget->viewY(), true);
        if (m_popUpWidget->viewY() < 0.0f)
            m_popUpWidget->moveTo(m_popUpWidget->viewX(), 0.0f, true);
        if (static_cast<double>(m_popUpWidget->viewX()) + m_popUpWidget->viewWidth() > viewportWidth())
            m_popUpWidget->moveTo(static_cast<float>(viewportWidth()) - m_popUpWidget->viewWidth(),
                                  m_popUpWidget->viewY(), true);
        if (static_cast<double>(m_popUpWidget->viewY()) + m_popUpWidget->getViewHeight() > viewportHeight()) {
            if (anchor) {
                m_popUpWidget->setViewPosition(
                    anchor->getContentPosition() - Vector<2, float>(0.0f, widget->getViewHeight()), true);
            } else {
                m_popUpWidget->moveTo(m_popUpWidget->viewX(),
                                      static_cast<float>(viewportHeight()) - m_popUpWidget->getViewHeight(), true);
            }
        }
        if (m_popUpWidget)
            m_popUpWidget->m_node->m_display->m_visibility.value() = 1;
    }
}

void Engine::beginOffscreenDrawing(float scale)
{
    ++m_surfaceDepth;
    if (m_surfaceDepth >= static_cast<int>(m_renderSurfaces.size())) {
        do {
            m_renderSurfaces.push_back(createRenderSurface(0));
            m_renderSurfaces.back()->resize(m_viewportWidth, m_viewportHeight);
        } while (m_surfaceDepth >= static_cast<int>(m_renderSurfaces.size()));
    }
    beginSurface(m_renderSurfaces[m_surfaceDepth], Vector<2, float>(0.0f, 0.0f),
                 Vector<2, float>(Vector<2, int>(m_viewportWidth, m_viewportHeight)), scale, true);
}

void Engine::endOffscreenDrawing()
{
    // An empty surface pool makes the remainder operation undefined.
    if (m_surfaceDepth % m_renderSurfaces.size() < m_renderSurfaces.size()) {
        endSurface();
        m_filterSurface = m_renderSurfaces[m_surfaceDepth % m_renderSurfaces.size()];
        --m_surfaceDepth;
    } else {
        --m_surfaceDepth;
    }
}

void Engine::clearSurfaceBounds(Node *node)
{
    Matrix<float> matrix;
    getModelViewMatrix(matrix);
    Vector<2, float> minimum;
    Vector<2, float> maximum;
    node->getBounds(minimum, maximum, matrix);
    m_renderSurfaces[m_surfaceDepth % m_renderSurfaces.size()]->clearRect(minimum, maximum - minimum, false);
}

void Engine::pushSurface(RenderSurface *surface)
{
    m_surfaceStack.push_back(surface);
}

RenderSurface *Engine::popSurface()
{
    if (m_surfaceStack.empty())
        return 0;
    RenderSurface *surface = m_surfaceStack.back();
    m_surfaceStack.pop_back();
    return surface;
}

RenderSurface *Engine::topSurface()
{
    if (m_surfaceStack.empty())
        return 0;
    return m_surfaceStack.back();
}

GenericWidget *Engine::createGenericWidget(const std::wstring &name, float x, float y, float width, float height,
                                           Node *parent, const std::wstring &shape)
{
    return new GenericWidget(this, name, x, y, width, height, parent, shape);
}

void Engine::resetInput()
{
    m_pressedKeys.clear();
    m_buttons = 0;
    releasePointerCapture();
    m_focusWidget = 0;
}

std::list<Object *>::iterator Engine::addObject(Object *object)
{
    lock();
    m_objects.push_back(object);
    std::list<Object *>::iterator entry = --m_objects.end();
    unlock();
    return entry;
}

std::list<Shape *>::iterator Engine::addShape(Shape *shape)
{
    lock();
    m_shapes.push_back(shape);
    std::list<Shape *>::iterator entry = --m_shapes.end();
    unlock();
    return entry;
}

std::list<Transformation *>::iterator Engine::addTransformation(Transformation *transformation)
{
    lock();
    m_transformations.push_back(transformation);
    std::list<Transformation *>::iterator entry = --m_transformations.end();
    unlock();
    return entry;
}

std::list<Display *>::iterator Engine::addDisplay(Display *display)
{
    lock();
    m_displays.push_back(display);
    std::list<Display *>::iterator entry = --m_displays.end();
    unlock();
    return entry;
}

std::list<Widget *>::iterator Engine::addWidget(Widget *widget)
{
    lock();
    m_widgets.push_back(widget);
    std::list<Widget *>::iterator entry = --m_widgets.end();
    unlock();
    return entry;
}

std::list<Node *>::iterator Engine::addNode(Node *node)
{
    lock();
    m_nodes.push_back(node);
    std::list<Node *>::iterator entry = --m_nodes.end();
    unlock();
    return entry;
}

std::list<Texture *>::iterator Engine::addTexture(Texture *texture)
{
    lock();
    m_textures.push_back(texture);
    std::list<Texture *>::iterator entry = --m_textures.end();
    unlock();
    return entry;
}

// The first of objects named name, or 0 (also used for the widgets).
template<class T> static T *findByName(const std::wstring &name, const std::list<T *> &objects)
{
    for (typename std::list<T *>::const_iterator it = objects.begin(); it != objects.end(); ++it)
        if ((*it)->getName().compare(name) == 0)
            return *it;
    return 0;
}

std::ostream &writePLAValue(std::ostream &stream, const void *value)
{
    return stream.write(static_cast<const char *>(value), 4);
}

int readPLAInt(std::istream &stream)
{
    // A short read leaves part or all of the local uninitialised.
    int value;
    stream.read(reinterpret_cast<char *>(&value), 4);
    return value;
}

float readPLAFloat(std::istream &stream)
{
    // A short read leaves part or all of the local uninitialised.
    float value;
    stream.read(reinterpret_cast<char *>(&value), 4);
    return value;
}

void writePLAVector2ArrayAttribute(std::ostream &stream, ContinuousArrayAttribute<Vector<2, float>> *attribute)
{
    beginPLAChunk(stream, 2);
    {
        int count = attribute->m_values[0]->size();
        writePLAValue(stream, &count);
    }
    endPLAChunk(stream);
    if (static_cast<int>(attribute->m_values[0]->size()) > 0) {
        for (int i = 0; i < attribute->size(); ++i) {
            beginPLAChunk(stream, 1);
            stream.write(reinterpret_cast<const char *>(attribute->m_values[i]->data()),
                         attribute->m_values[0]->size() * 8);
            endPLAChunk(stream);
        }
    }
    writePLASequences(stream, attribute);
}

void writePLAVector4ArrayAttribute(std::ostream &stream, ContinuousArrayAttribute<Vector<4, float>> *attribute)
{
    beginPLAChunk(stream, 2);
    {
        int count = attribute->m_values[0]->size();
        writePLAValue(stream, &count);
    }
    endPLAChunk(stream);
    if (static_cast<int>(attribute->m_values[0]->size()) > 0) {
        for (int i = 0; i < attribute->size(); ++i) {
            beginPLAChunk(stream, 1);
            stream.write(reinterpret_cast<const char *>(attribute->m_values[i]->data()),
                         attribute->m_values[0]->size() * 16);
            endPLAChunk(stream);
        }
    }
    writePLASequences(stream, attribute);
}

void writePLAFloatArrayAttribute(std::ostream &stream, ContinuousArrayAttribute<float> *attribute)
{
    beginPLAChunk(stream, 2);
    {
        int count = attribute->m_values[0]->size();
        writePLAValue(stream, &count);
    }
    endPLAChunk(stream);
    if (static_cast<int>(attribute->m_values[0]->size()) > 0) {
        for (int i = 0; i < attribute->size(); ++i) {
            beginPLAChunk(stream, 1);
            stream.write(reinterpret_cast<const char *>(attribute->m_values[i]->data()),
                         attribute->m_values[0]->size() * 4);
            endPLAChunk(stream);
        }
    }
    writePLASequences(stream, attribute);
}

void writePLAIntAttribute(std::ostream &stream, DiscreteAttribute<int> *attribute)
{
    for (int i = 0; i < attribute->size(); ++i) {
        beginPLAChunk(stream, 1);
        writePLAValue(stream, &attribute->m_values[i]);
        endPLAChunk(stream);
    }
    writePLASequences(stream, attribute);
}

void writePLAVector2Attribute(std::ostream &stream, ContinuousAttribute<Vector<2, float>> *attribute)
{
    for (int i = 0; i < attribute->size(); ++i) {
        beginPLAChunk(stream, 1);
        writePLAVector2(stream, attribute->m_values[i]);
        endPLAChunk(stream);
    }
    writePLASequences(stream, attribute);
}

void writePLAVector3Attribute(std::ostream &stream, ContinuousAttribute<Vector<3, float>> *attribute)
{
    for (int i = 0; i < attribute->size(); ++i) {
        beginPLAChunk(stream, 1);
        writePLAVector3(stream, attribute->m_values[i]);
        endPLAChunk(stream);
    }
    writePLASequences(stream, attribute);
}

void writePLAMatrixAttribute(std::ostream &stream, ContinuousAttribute<Matrix<float>> *attribute)
{
    for (int i = 0; i < attribute->size(); ++i) {
        beginPLAChunk(stream, 1);
        writePLAMatrix(stream, attribute->m_values[i]);
        endPLAChunk(stream);
    }
    writePLASequences(stream, attribute);
}

void writePLAVector4Attribute(std::ostream &stream, ContinuousAttribute<Vector<4, float>> *attribute)
{
    for (int i = 0; i < attribute->size(); ++i) {
        beginPLAChunk(stream, 1);
        writePLAVector4(stream, attribute->m_values[i]);
        endPLAChunk(stream);
    }
    writePLASequences(stream, attribute);
}

std::ostream &writePLAVector2(std::ostream &stream, const Vector<2, float> &value)
{
    return stream.write(reinterpret_cast<const char *>(&value), 8);
}

std::ostream &writePLAMatrix(std::ostream &stream, const Matrix<float> &value)
{
    return stream.write(reinterpret_cast<const char *>(&value), 64);
}

void readPLAVector2ArrayAttribute(std::istream &stream, ContinuousArrayAttribute<Vector<2, float>> *attribute,
                                  std::map<int, std::string> &names)
{
    std::streampos end = readPLAChunkEnd(stream);
    int index = 0;
    while (stream.tellg() != end && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case 1:
            readPLAChunkEnd(stream);
            if (index == attribute->size())
                attribute->duplicateElement();
            stream.read(reinterpret_cast<char *>(attribute->m_values[index]->data()),
                        attribute->m_values[0]->size() * 8);
            index++;
            break;
        case 2: {
            readPLAChunkEnd(stream);
            int count = readPLAInt(stream);
            attribute->resize(count);
            break;
        }
        case 3:
            readPLASequences(stream, attribute, names);
            break;
        default:
            skipPLAChunk(stream);
            break;
        }
    }
}

void readPLAVector4ArrayAttribute(std::istream &stream, ContinuousArrayAttribute<Vector<4, float>> *attribute,
                                  std::map<int, std::string> &names)
{
    std::streampos end = readPLAChunkEnd(stream);
    int index = 0;
    while (stream.tellg() != end && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case 1:
            readPLAChunkEnd(stream);
            if (index == attribute->size())
                attribute->duplicateElement();
            stream.read(reinterpret_cast<char *>(attribute->m_values[index]->data()),
                        attribute->m_values[0]->size() * 16);
            index++;
            break;
        case 2: {
            readPLAChunkEnd(stream);
            int count = readPLAInt(stream);
            attribute->resize(count);
            break;
        }
        case 3:
            readPLASequences(stream, attribute, names);
            break;
        default:
            skipPLAChunk(stream);
            break;
        }
    }
}

void readPLAFloatArrayAttribute(std::istream &stream, ContinuousArrayAttribute<float> *attribute,
                                std::map<int, std::string> &names)
{
    std::streampos end = readPLAChunkEnd(stream);
    int index = 0;
    while (stream.tellg() != end && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case 1:
            readPLAChunkEnd(stream);
            if (index == attribute->size())
                attribute->duplicateElement();
            stream.read(reinterpret_cast<char *>(attribute->m_values[index]->data()),
                        attribute->m_values[0]->size() * 4);
            index++;
            break;
        case 2: {
            readPLAChunkEnd(stream);
            int count = readPLAInt(stream);
            attribute->resize(count);
            break;
        }
        case 3:
            readPLASequences(stream, attribute, names);
            break;
        default:
            skipPLAChunk(stream);
            break;
        }
    }
}

void readPLAIntAttribute(std::istream &stream, DiscreteAttribute<int> *attribute, std::map<int, std::string> &names)
{
    std::streampos end = readPLAChunkEnd(stream);
    int index = 0;
    while (stream.tellg() != end && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case 1:
            readPLAChunkEnd(stream);
            if (index == attribute->size())
                attribute->duplicateElement();
            stream.read(reinterpret_cast<char *>(&attribute->m_values[index]), 4);
            ++index;
            break;
        case 3:
            readPLASequences(stream, attribute, names);
            break;
        default:
            skipPLAChunk(stream);
            break;
        }
    }
}

void readPLAVector2Attribute(std::istream &stream, ContinuousAttribute<Vector<2, float>> *attribute,
                             std::map<int, std::string> &names)
{
    std::streampos end = readPLAChunkEnd(stream);
    int index = 0;
    while (stream.tellg() != end && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case 1:
            readPLAChunkEnd(stream);
            if (index == attribute->size())
                attribute->duplicateElement();
            stream.read(reinterpret_cast<char *>(&attribute->m_values[index]), 8);
            ++index;
            break;
        case 3:
            readPLASequences(stream, attribute, names);
            break;
        default:
            skipPLAChunk(stream);
            break;
        }
    }
}

void readPLAVector3Attribute(std::istream &stream, ContinuousAttribute<Vector<3, float>> *attribute,
                             std::map<int, std::string> &names)
{
    std::streampos end = readPLAChunkEnd(stream);
    int index = 0;
    while (stream.tellg() != end && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case 1:
            readPLAChunkEnd(stream);
            if (index == attribute->size())
                attribute->duplicateElement();
            stream.read(reinterpret_cast<char *>(&attribute->m_values[index]), 12);
            ++index;
            break;
        case 3:
            readPLASequences(stream, attribute, names);
            break;
        default:
            skipPLAChunk(stream);
            break;
        }
    }
}

void readPLAMatrixAttribute(std::istream &stream, ContinuousAttribute<Matrix<float>> *attribute,
                            std::map<int, std::string> &names)
{
    std::streampos end = readPLAChunkEnd(stream);
    int index = 0;
    while (stream.tellg() != end && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case 1:
            readPLAChunkEnd(stream);
            if (index == attribute->size())
                attribute->duplicateElement();
            stream.read(reinterpret_cast<char *>(&attribute->m_values[index]), 64);
            ++index;
            break;
        case 3:
            readPLASequences(stream, attribute, names);
            break;
        default:
            skipPLAChunk(stream);
            break;
        }
    }
}

Vector<2, float> readPLAVector2(std::istream &stream)
{
    Vector<2, float> value;
    stream.read(reinterpret_cast<char *>(&value), 8);
    return value;
}

Matrix<float> readPLAMatrix(std::istream &stream)
{
    Matrix<float> value;
    stream.read(reinterpret_cast<char *>(&value), 64);
    return value;
}

void readPLAVector4Attribute(std::istream &stream, ContinuousAttribute<Vector<4, float>> *attribute,
                             std::map<int, std::string> &names)
{
    std::streampos end = readPLAChunkEnd(stream);
    int index = 0;
    while (stream.tellg() != end && !stream.eof()) {
        switch (readPLAInt(stream)) {
        case 1:
            readPLAChunkEnd(stream);
            if (index == attribute->size())
                attribute->duplicateElement();
            stream.read(reinterpret_cast<char *>(&attribute->m_values[index]), 16);
            ++index;
            break;
        case 3:
            readPLASequences(stream, attribute, names);
            break;
        default:
            skipPLAChunk(stream);
            break;
        }
    }
}

std::ostream &writePLAVector3(std::ostream &stream, const Vector<3, float> &value)
{
    return stream.write(reinterpret_cast<const char *>(&value), 12);
}

std::ostream &writePLAVector4(std::ostream &stream, const Vector<4, float> &value)
{
    return stream.write(reinterpret_cast<const char *>(&value), 16);
}

}
