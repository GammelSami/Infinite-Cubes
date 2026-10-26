// Engine owns scene objects, lists of shapes, widgets, transformations, displays, nodes and textures, plus root nodes, fonts and input state. D3D9Engine implements its graphics interface.
#pragma once
#include <windows.h>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "plasma/ObjectManager.h"
#include "plasma/Vector.h"
#include "plasma/Matrix.h"

// FreeType's library handle (freetype/freetype.h).
typedef struct FT_LibraryRec_ *FT_Library;

#include "plasma/StringConversion.h"

namespace plasma {
enum MouseButton {
    MOUSE_BUTTON_LEFT = 0,
    MOUSE_BUTTON_RIGHT = 1,
    MOUSE_BUTTON_MIDDLE = 2
};

// Engine::m_renderFlags (antialias, widget surfaces and blur initially enabled). Individual flags
// are bit indices; use (1 << flag) to form a mask.
enum RenderFlags {
    RENDER_ANTIALIAS = 0,
    RENDER_CLIP_WIDGET_SURFACES = 1,
    RENDER_BLUR = 2,
    RENDER_DEPTH_LAYERS = 3
};

class Object;
class Texture;
class Font;
class FontEngine;
class Node;
class Widget;
class Button;
class Edit;
class ListWidget;
class PopUpButton;
class ScrollButton;
class ScrollSlider;
class RenderSurface;
struct TextureFormat;
class Drawing;
class Transformation;
class Display;
class Shape;
class GenericShape;
class TextShape;
class SmoothMeshShape;
class MeshShape;
class CurveShape;
class Movie;
class PixelFont;
class PlasmaFont;

class Engine : public ObjectManager {
public:
    // A document's page (the PLX "pageWidth", "pageHeight", "dpi", "unit" and "pageColor" chunks).
    struct Page {
        Page() {}
        Page(float width, float height, float resolution)
            : size(width, height, resolution),
              unit(0),
              color(1.0f, 1.0f, 1.0f, 0.0f)
        {}

        Vector<3, float> size; // width, height and dpi
        int unit;
        Vector<4, float> color;
    };

    Engine();
    virtual ~Engine();

    // Graphics operations are implemented by the rendering backend.
    virtual bool setupShadersAndFrameBuffer() { return true; }
    virtual Texture *createTexture(int width, int height, const unsigned char *pixels, const TextureFormat &format,
                                   const std::wstring &name, bool transient) = 0;
    virtual void drawCaret(const Vector<2, float> &position, const Vector<2, float> &size,
                           const Vector<4, float> &color) = 0;
    virtual void invertRectangle(const Vector<2, float> &position, const Vector<2, float> &size) = 0;
    virtual void drawTexturedRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                                       const Vector<2, float> &texturePosition, const Vector<2, float> &textureSize,
                                       const Vector<4, float> &color) = 0;
    virtual void fillRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                               const Vector<4, float> &color) = 0;
    virtual void frameRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                                const Vector<4, float> &color)
    {}
    // The base graphics hooks do nothing; D3D9Engine overrides them.
    virtual void pushIdentityMatrix() {}
    virtual void popMatrix() {}
    virtual void enableTexture2D() {}
    virtual void disableTexture2D() = 0;
    virtual Drawing *createDrawing() = 0;
    virtual RenderSurface *createRenderSurface(bool depthAttachment) { return 0; }
    virtual void getModelViewMatrix(Matrix<float> &matrix) {}
    virtual void loadMatrix(Matrix<float> &matrix) {}
    virtual void beginTextDrawing(Node *node) {}
    virtual void endTextDrawing(Node *node) {}
    // Uploads projection, model-view and normal matrices after loadMatrix; ScalableFont calls this hook.
    virtual void matrixLoaded() {}
    virtual void beginWindowDrawing() = 0;
    virtual void endWindowDrawing() = 0;
    virtual void pushNodeMatrix(Node *node) = 0;
    virtual void popNodeMatrix(Node *node) = 0;
    // And 24: pure here, empty in D3D9Engine, never called.
    virtual void unused23(Node *node) = 0;
    virtual void unused24(Node *node) = 0;
    virtual void beginSurface(RenderSurface *surface, const Vector<2, float> &position, const Vector<2, float> &size,
                              float scale, bool alignPixels)
    {}
    virtual void endSurface() {}
    virtual void drawPixels(const unsigned char *pixels, int stride, int unused, int x, int y, int width, int height) {}

    // Guard the object lists (the Add and Delete functions, Texture's constructor).
    void lock();
    void unlock();

    // The root of the scene.
    Node *root() { return m_root; }

    // A new node with the given parts; a missing transformation or display is created (unnamed). The
    // node becomes a child of parent when given.
    Node *createNode(Transformation *transformation, Shape *shape, Display *display, Node *parent,
                     const std::wstring &name = L"");
    // The empty default names are default arguments: createNode has no /GS cookie, which explicit
    // std::wstring temporaries would get (note).
    Transformation *createTransformation(const std::wstring &name = L"");
    Display *createDisplay(const std::wstring &name = L"");
    GenericShape *createGenericShape(const std::wstring &source = L"", const std::wstring &name = L"");
    Widget *createWidget(Node *node, const std::wstring &name = L"");
    Button *createButton(Node *node, int type, const std::wstring &name = L"");
    Edit *createEdit(Node *node, const std::wstring &name = L"");
    ListWidget *createListWidget(Node *node, const std::wstring &name = L"");
    PopUpButton *createPopUpButton(Node *node, const std::wstring &name = L"");
    ScrollButton *createScrollButton(Node *node, const std::wstring &name = L"");
    ScrollSlider *createScrollSlider(Node *node, const std::wstring &name = L"");
    TextShape *createTextShape(const std::wstring &text = std::wstring(), const std::wstring &name = L"");
    SmoothMeshShape *createSmoothMeshShape(const std::wstring &name = L"");
    MeshShape *createMeshShape(const std::wstring &name);
    CurveShape *createCurveShape(const std::wstring &name = L"");
    Movie *createMovie(const std::wstring &name);
    PlasmaFont *createPlasmaFont(const std::wstring &name);
    PixelFont *createPixelFont(Texture *texture, int columns = -1, int rows = -1, const std::wstring &characters = L"");
    // The font loaders (a TrueType file, an image of glyph cells or a plasma file of glyph nodes,
    // searched in the font paths too).
    Font *loadLegacyFont(const std::wstring &path, float size, bool transient, float stroke, bool pixel);
    PixelFont *loadPixelFont(const std::wstring &path, float size, bool transient, float stroke,
                             const std::wstring &characters = L"");
    PlasmaFont *loadOutlineFont(const std::wstring &path, float size, const std::wstring &characters = L"");
    PlasmaFont *loadPlasmaFont(const std::wstring &path, float size);
    Texture *loadImageTexture(const std::wstring &path);
    Texture *loadImageTexture(const std::wstring &path, unsigned char *data, int size);
    // A plasma font with a glyph for every one-character child of root.
    PlasmaFont *createPlasmaFont(Node *root, const std::wstring &name);
    // The movie of that name, or 0.
    Movie *findMovie(const std::wstring &name)
    {
        std::map<std::wstring, Movie *>::iterator found = m_movies.find(name);
        if (found == m_movies.end())
            return 0;
        return found->second;
    }

    // Each locks the engine, appends and returns the new entry (the object keeps it to unregister).
    std::list<Object *>::iterator addObject(Object *object);
    std::list<Shape *>::iterator addShape(Shape *shape);
    std::list<Texture *>::iterator addTexture(Texture *texture);
    std::list<Transformation *>::iterator addTransformation(Transformation *transformation);
    std::list<Display *>::iterator addDisplay(Display *display);
    std::list<Widget *>::iterator addWidget(Widget *widget);
    std::list<Node *>::iterator addNode(Node *node);

    // Each removes the object from the engine's list and deletes it.
    void deleteShape(Shape *shape);
    void deleteDisplay(Display *display);
    void deleteTransformation(Transformation *transformation);
    void deleteTexture(Texture *texture);
    void deleteWidget(Widget *widget);
    // Deletes the node and its subtree, and the parts no other node uses.
    void deleteNode(Node *node);
    void pushFillColor() { m_fillColorStack.push_back(m_fillColor); }
    void pushOutlineColor() { m_outlineColorStack.push_back(m_outlineColor); }
    void pushExtrusionColor() { m_extrusionColorStack.push_back(m_extrusionColor); }
    void setFillColor(const float *color)
    {
        m_fillColor[0] = color[0];
        m_fillColor[1] = color[1];
        m_fillColor[2] = color[2];
        m_fillColor[3] = color[3];
    }
    void setOutlineColor(const float *color)
    {
        m_outlineColor[0] = color[0];
        m_outlineColor[1] = color[1];
        m_outlineColor[2] = color[2];
        m_outlineColor[3] = color[3];
    }
    void setExtrusionColor(const float *color)
    {
        m_extrusionColor[0] = color[0];
        m_extrusionColor[1] = color[1];
        m_extrusionColor[2] = color[2];
        m_extrusionColor[3] = color[3];
    }
    void popFillColor()
    {
        if (!m_fillColorStack.empty()) {
            m_fillColor = m_fillColorStack.back();
            m_fillColorStack.pop_back();
        }
    }
    void popOutlineColor()
    {
        if (!m_outlineColorStack.empty()) {
            m_outlineColor = m_outlineColorStack.back();
            m_outlineColorStack.pop_back();
        }
    }
    void popExtrusionColor()
    {
        if (!m_extrusionColorStack.empty()) {
            m_extrusionColor = m_extrusionColorStack.back();
            m_extrusionColorStack.pop_back();
        }
    }

    // Reads a.pla (legacy) or.plg,.pld,.plx file into target; the file's directory joins the font
    // search paths.
    bool readPlasmaFileIntoNode(const std::wstring &path, Node *target, int flags, Page *page,
                                const std::vector<unsigned __int64> &keys);
    void readPLXFile(std::istream &stream, Node *target, int flags, Page *page,
                     const std::vector<unsigned __int64> &keys);
    void readPLAFile(std::ifstream &stream, Node *target, int flags, Page *page);
    // The legacy PLA readers: each takes the stream after its chunk's identifier. A Read...Property
    // function reads one property chunk and returns false when the identifier is not one of its
    // class's.
    Button *readButton(std::istream &stream);
    CurveShape *readCurveShape(std::istream &stream, std::map<int, std::string> &names);
    GenericShape *readGenericShape(std::istream &stream, std::map<int, std::string> &names);
    Movie *readMovie(std::istream &stream, std::map<int, std::string> &names);
    SmoothMeshShape *readSmoothMeshShape(std::istream &stream, std::map<int, std::string> &names,
                                         std::map<int, int> &textureIds);
    TextShape *readTextShape(std::istream &stream, std::map<int, std::string> &names, std::map<int, int> &fontIds);
    Transformation *readTransformation(std::istream &stream, std::map<int, std::string> &names);
    bool readButtonProperty(std::istream &stream, Button *button, int identifier);
    Widget *readEdit(std::istream &stream);
    Widget *readListWidget(std::istream &stream);
    bool readListWidgetProperty(std::istream &stream, ListWidget *widget, int identifier);
    Widget *readScrollButton(std::istream &stream);
    bool readScrollButtonProperty(std::istream &stream, ScrollButton *button, int identifier);
    Widget *readScrollSlider(std::istream &stream);
    bool readScrollSliderProperty(std::istream &stream, ScrollSlider *slider, int identifier);
    Widget *readWidget(std::istream &stream);
    bool readWidgetProperty(std::istream &stream, Widget *widget, int identifier);

    void deleteDrawing(Drawing *drawing);
    void deleteRenderSurface(RenderSurface *surface);

    int drawWindow();
    bool advance(int milliseconds);
    void setViewport(int x, int y, int width, int height, bool notify);
    void initializeWidgets();
    Node *getPointerNode();
    // Calls method on the widget of node and of every node below it.
    void dispatchWidgets(Node *node, void (Widget::*method)());
    void setPointerCapture(Node *node);
    void releasePointerCapture();
    // Invokes the connection the enclosing widget of node has for event.
    void sendEvent(Node *node, int event);
    void pointerPressed();
    void pointerReleased();
    void rightButtonPressed();
    void rightButtonReleased();
    void movePointer(float x, float y);
    void keyPressed(unsigned short key);
    void keyReleased(unsigned short key);

    bool isKeyDown(unsigned short key) const { return m_pressedKeys.find(key) != m_pressedKeys.end(); }
    FontEngine *getFontEngine() const { return m_fontEngine; }
    // The caret blink clock (getCaretClock).
    int getCaretClock() const { return m_caretClock; }
    // Milliseconds since the previous frame.
    int getFrameTime() const { return m_frameTime; }
    void characterEntered(int character);
    void setFocusWidget(Widget *widget);
    Widget *getFocusWidget();
    void setPopUpWidget(Widget *widget, Widget *anchor);
    Texture *findTexture(int id);

    // The surface stack (pushed and popped by D3D9Engine).
    void pushSurface(RenderSurface *surface);
    RenderSurface *popSurface();
    void beginOffscreenDrawing(float scale);
    void endOffscreenDrawing();
    void clearSurfaceBounds(Node *node, float scale);

    Vector<2, float> getPointer() const { return m_pointer; }
    Vector<2, float> getPointerDelta() const { return m_pointer - m_previousPointer; }
    bool isButtonDown(int button) const { return (m_buttons >> button) & 1; }
    int viewportWidth() const { return m_viewportWidth; }
    int viewportHeight() const { return m_viewportHeight; }

    const Vector<4, float> &getFillColor() const { return m_fillColor; }
    const Vector<4, float> &getOutlineColor() const { return m_outlineColor; }
    // Hit tests clip to widget surfaces.
    bool usesWidgetSurfaces() const { return (m_renderFlags & (1 << RENDER_CLIP_WIDGET_SURFACES)) != 0; }
    // Blurred displays are blurred (Node::draw).
    bool usesBlur() const { return (m_renderFlags >> RENDER_BLUR) & 1; }
    // Each drawing gets its own depth layer (D3D9Drawing::draw); window drawing clears the depth and
    // stencil buffers first.
    bool usesDepthLayers() const { return (m_renderFlags >> RENDER_DEPTH_LAYERS) & 1; }

protected:
    friend class SmoothMeshShape;
    friend class Texture;
    friend class D3D9Drawing;
    friend class Font;
    friend class Widget;
    friend class Button;
    friend class Node;
    friend class ScrollSlider;
    friend class ScrollButton;
    friend class PopUpButton;
    friend class Edit;

    int m_renderTargetSwitchCount; // counted by drawWindow's rendering, cleared by Advance
    Page m_page;                   // 1000 x 1000 at 72 dpi
    FontEngine *m_fontEngine;
    std::vector<RenderSurface *> m_renderSurfaces; // the off-screen pool
    RenderSurface *m_blurSurface;
    int m_surfaceDepth;             // -1 outside drawing
    RenderSurface *m_filterSurface; // the last off-screen drawing
    std::list<RenderSurface *> m_surfaceStack;
    CRITICAL_SECTION m_lock; // guards the object lists
    FT_Library m_freeType;
    std::list<Shape *> m_shapes;
    std::list<Widget *> m_widgets;
    std::list<Transformation *> m_transformations;
    std::list<Display *> m_displays;
    std::list<Node *> m_nodes;
    std::list<Texture *> m_textures;
    std::list<Font *> m_fontList;
    std::map<std::wstring, Movie *> m_movies;
    Node *m_root;
    Node *m_sources; // internal (NODE_INTERNAL)
    Node *m_fonts;   // internal
    Node *m_overlayRoot;
    Node *m_pointerNode; // the node under the pointer
    Node *m_pointerCapture;
    Widget *m_focusWidget;
    Widget *m_popUpWidget;
    Vector<2, float> m_pointer;
    Vector<2, float> m_previousPointer;
    int m_frameTime;
    int m_caretClock;
    std::set<unsigned short> m_pressedKeys;
    unsigned m_buttons; // bit mask indexed by MouseButton
    int m_lastKey;      // not initialised by the constructor
    Widget *m_pressedPopUpItem;
    int m_wheelDelta;
    int m_viewportX;
    int m_viewportY;
    int m_viewportWidth;  // 1000
    int m_viewportHeight; // 800
    int m_windowWidth;
    int m_windowHeight;
    // Only the constructor and the destructor touch the next two.
    std::map<std::string, void *> m_unused11c; // (the 2012 engine's m_unused154)
    std::list<std::string> m_unused124;
    std::map<int, Texture *> m_texturesById;
    std::map<int, Font *> m_fontsById;
    std::list<std::wstring> m_fontPaths;
    std::list<Vector<4, float>> m_fillColorStack;
    std::list<Vector<4, float>> m_outlineColorStack;
    std::list<Vector<4, float>> m_extrusionColorStack;
    Vector<4, float> m_fillColor;      // 1, 1, 1, 1
    Vector<4, float> m_outlineColor;   // 1, 1, 1, 1
    Vector<4, float> m_extrusionColor; // 1, 1, 1, 1
    unsigned m_renderFlags;            // RenderFlags bit indices; antialias, widget surfaces and blur initially enabled
    bool m_unused190;                  // true from the constructor; nothing reads it (the 2012 engine's m_unused1F4)
};
}
