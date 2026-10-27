// plasma::Engine (Engine.cpp): the engine base class. It owns every engine object (through ObjectManager), keeps the
// shapes, widgets, transformations, displays, nodes, textures and fonts in lists, the roots of the node trees ("root",
// "sources", "fonts"), the font engine, the off-screen surface pool and the input state (pointer, pressed keys,
// focus). plasma::D3D11Engine is the only derived class in Cube World.
#pragma once
#include <windows.h>
#include <iosfwd>
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

std::wstring WidenString(const std::string &text);

namespace cube {
class Game;
class Interface;
}

namespace plasma {

class Object;
class Shape;
class Display;
class Transformation;
class Node;
class Movie;
class Texture;
class Widget;
class Button;
class GenericShape;
class RenderSurface;
class FontEngine;
class Font;
class PlasmaFont;
class PixelFont;
class Drawing;
struct TextureFormat;
class TextShape;
class SmoothMeshShape;
class MeshShape;
class CurveShape;
class ScrollSlider;
class ScrollButton;
class PopUpButton;
class ListWidget;
class Edit;

// The pointer buttons: Engine::isButtonDown's index and the bits of Engine::m_buttons. pointerPressed sets bit 0,
// rightButtonPressed bit 1, middleButtonPressed bit 2.
enum MouseButton {
    MOUSE_BUTTON_LEFT = 0,
    MOUSE_BUTTON_RIGHT = 1,
    MOUSE_BUTTON_MIDDLE = 2
};

// Engine::m_renderFlags.
enum RenderFlags {
    RENDER_ANTIALIAS = 0,
    RENDER_CLIP_WIDGET_SURFACES = 1,
    RENDER_BLUR = 2,
    RENDER_DEPTH_LAYERS = 3
};

// Document loading flags.
enum SceneLoadFlags {
    LOAD_SHARED = 0,         // PLA loading: skip makeUnique on top-level nodes.
    LOAD_NO_WIDGET_INIT = 1, // PLA loading: skip initializeWidgets for the page viewport.
    LOAD_INTERNAL = 2,       // Mark loaded nodes NODE_INTERNAL.
    LOAD_IGNORE_PAGE = 3,    // Skip document page properties while reading.
    LOAD_STATIC_MESHES = 4,  // Convert unanimated SmoothMeshShape objects to StaticMeshShape.
    LOAD_HINTED_TEXT = 5,    // Mark loaded text TEXT_HINTED.
    LOAD_CHECK_SIGNATURE = 6 // Require the PlasmaGraphics signature in the document header.
};

class Engine : public ObjectManager {
public:
    // Document page properties: pageWidth, pageHeight, dpi, unit and pageColor.
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

    virtual bool setupShadersAndFrameBuffer() { return true; }
    virtual Texture *createTexture(int width, int height, const unsigned char *pixels, const TextureFormat &format,
                                   const std::wstring &name = L"", bool transient = false);
    virtual void drawCaret(const Vector<2, float> &position, const Vector<2, float> &size,
                           const Vector<4, float> &color)
    {}
    virtual void invertRectangle(const Vector<2, float> &position, const Vector<2, float> &size) {}
    virtual void drawTexturedRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                                       const Vector<2, float> &texturePosition, const Vector<2, float> &textureSize,
                                       const Vector<4, float> &color)
    {}
    virtual void fillRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                               const Vector<4, float> &color)
    {}
    virtual void frameRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                                const Vector<4, float> &color)
    {}
    virtual void pushIdentityMatrix() {}
    virtual void popMatrix() {}
    virtual void enableTexture2D() {}
    virtual void disableTexture2D() {}
    virtual Drawing *createDrawing();
    virtual RenderSurface *createRenderSurface(bool depthAttachment) { return nullptr; }
    virtual void getModelViewMatrix(Matrix<float> &matrix) {}
    virtual void loadMatrix(Matrix<float> &matrix) {}
    virtual void applyRenderState() {}
    virtual void beginTextDrawing(Node *node) {}
    virtual void endTextDrawing(Node *node) {}
    virtual void matrixLoaded() {}
    virtual void beginWindowDrawing() {}
    virtual void endWindowDrawing() {}
    virtual void pushNodeMatrix(Node *node) {}
    virtual void popNodeMatrix(Node *node) {}
    // Node hooks with no implementation; their role is unknown.
    virtual void unimplementedNodeHook1(Node *node) {}
    virtual void unimplementedNodeHook2(Node *node) {}
    virtual void beginSurface(RenderSurface *surface, const Vector<2, float> &position, const Vector<2, float> &size,
                              float scale, bool alignPixels)
    {}
    virtual void endSurface() {}
    virtual void drawPixels(const unsigned char *pixels, int stride, int unusedArgument, int x, int y, int width,
                            int height)
    {}

    // Guard the object lists (the Add and Delete functions, the Texture constructors).
    void lock();
    void unlock();

    // A new node; a missing transformation or display is created.
    Node *createNode(Transformation *transformation, Shape *shape, Display *display, Node *parent,
                     const std::wstring &name = L"");
    Transformation *createTransformation(const std::wstring &name = L"");
    Display *createDisplay(const std::wstring &name = L"");
    // Deletes the movie of that name (if any) and registers a new one in m_movies.
    Movie *createMovie(const std::wstring &name = L"");
    Movie *findMovie(const std::wstring &name);

    // Returns a cached font matching file name, size, spacing and kind, or loads a new one. TTF files use bitmap or
    // outline glyphs according to pixel; other files use Plasma glyphs.
    Font *loadLegacyFont(const std::wstring &path, float size, bool transient, float stroke, bool pixel);
    PlasmaFont *loadPlasmaFont(const std::wstring &path, float size);
    PlasmaFont *loadOutlineFont(const std::wstring &path, float size, const std::wstring &characters = L"");
    // A .TTF gives an empty font that remembers the request; otherwise the image (searched in the font paths) becomes
    // a 16 x 16 glyph sheet.
    PixelFont *loadPixelFont(const std::wstring &path, float size, bool transient, float stroke,
                             const std::wstring &characters = L"");
    PixelFont *createPixelFont(Texture *texture, int columns = -1, int rows = -1, const std::wstring &characters = L"");
    // Loads a JPG, BMP, PNG, PPM, TIF or TGA file through FreeImage as an RGBA texture.
    Texture *loadImageTexture(const std::wstring &path);
    // The same with a texture format (the mod's pixel art icons filter to the nearest pixel).
    Texture *loadImageTexture(const std::wstring &path, const TextureFormat &format);
    PlasmaFont *createPlasmaFont(Node *root, const std::wstring &name);
    PixelFont *createPixelFont(const std::wstring &name = L"");
    PlasmaFont *createPlasmaFont(const std::wstring &name = L"");

    Widget *createWidget(Node *node, const std::wstring &name = L"");
    // type is Button's constructor argument (passed through in r8d).
    Button *createButton(Node *node, int type, const std::wstring &name = L"");
    GenericShape *createGenericShape(const std::wstring &source = L"", const std::wstring &name = L"");
    TextShape *createTextShape(const std::wstring &text = std::wstring(), const std::wstring &name = L"");
    SmoothMeshShape *createSmoothMeshShape(const std::wstring &name = L"");
    MeshShape *createMeshShape(const std::wstring &name = L"");
    CurveShape *createCurveShape(const std::wstring &name = L"");
    ScrollSlider *createScrollSlider(Node *node, const std::wstring &name = L"");
    ScrollButton *createScrollButton(Node *node, const std::wstring &name = L"");
    PopUpButton *createPopUpButton(Node *node, const std::wstring &name = L"");
    ListWidget *createListWidget(Node *node, const std::wstring &name = L"");
    Edit *createEdit(Node *node, const std::wstring &name = L"");

    Texture *findTexture(int id); // (m_texturesById)

    bool readPlasmaFileIntoNode(const std::wstring &path, Node *node, int flags, Page *page,
                                const std::vector<unsigned __int64> &keys);
    // Reads a PLX document.
    void readPLXFile(std::istream &stream, Node *target, int flags, Page *page,
                     const std::vector<unsigned __int64> &keys);
    // Reads legacy PLA documents; PLXStreamReader handles PLX documents.
    void readPLAFile(std::ifstream &stream, Node *target, int flags, Page *page);
    // The readers of one PLA chunk each (the stream is positioned after the chunk's identifier).
    Button *readButton(std::istream &stream);
    bool readButtonProperty(std::istream &stream, Button *button, int identifier);
    CurveShape *readCurveShape(std::istream &stream, std::map<int, std::string> &names);
    Widget *readEdit(std::istream &stream);
    GenericShape *readGenericShape(std::istream &stream, std::map<int, std::string> &names);
    Widget *readListWidget(std::istream &stream);
    bool readListWidgetProperty(std::istream &stream, ListWidget *widget, int identifier);
    Movie *readMovie(std::istream &stream, std::map<int, std::string> &names);
    Widget *readScrollButton(std::istream &stream);
    bool readScrollButtonProperty(std::istream &stream, ScrollButton *button, int identifier);
    Widget *readScrollSlider(std::istream &stream);
    bool readScrollSliderProperty(std::istream &stream, ScrollSlider *slider, int identifier);
    SmoothMeshShape *readSmoothMeshShape(std::istream &stream, std::map<int, std::string> &names,
                                         std::map<int, int> &textureIds);
    TextShape *readTextShape(std::istream &stream, std::map<int, std::string> &names, std::map<int, int> &fontIds);
    Transformation *readTransformation(std::istream &stream, std::map<int, std::string> &names);
    Widget *readWidget(std::istream &stream);
    bool readWidgetProperty(std::istream &stream, Widget *widget, int identifier);

    // The surface stack (m_surfaceStack).
    void pushSurface(RenderSurface *surface);
    RenderSurface *popSurface();

    // Each locks the engine, appends and returns the new entry.
    std::list<Object *>::iterator addObject(Object *object);
    std::list<Shape *>::iterator addShape(Shape *shape);
    std::list<Transformation *>::iterator addTransformation(Transformation *transformation);
    std::list<Display *>::iterator addDisplay(Display *display);
    std::list<Widget *>::iterator addWidget(Widget *widget);
    std::list<Node *>::iterator addNode(Node *node);
    std::list<Texture *>::iterator addTexture(Texture *texture);

    // Each removes the object from its list under the lock and deletes it.
    void deleteShape(Shape *shape);
    void deleteDisplay(Display *display);
    void deleteTransformation(Transformation *transformation);
    // Clears the focus, pop-up and pressed pop-up item first.
    void deleteWidget(Widget *widget);
    // Deletes object when non-null.
    void deleteObject(Object *object);
    // Deletes drawing if not null.
    void deleteDrawing(Drawing *drawing);
    void deleteRenderSurface(RenderSurface *surface);
    void destroyNode(Node *node);

    // Renders the root between beginWindowDrawing and endWindowDrawing and returns the render-target switch count. The
    // argument supplies NodeRenderMode flags; D3D11Renderer passes zero.
    int drawWindow(int mode);
    // Stores the frame time, advances the caret clock and the root.
    bool advance(int milliseconds);
    void setViewport(int x, int y, int width, int height, bool notify);
    void initializeWidgets();
    Node *getPointerNode();
    // Calls method on the widget of node and its descendants.
    void dispatchWidgets(Node *node, void (Widget::*method)());
    void sendEvent(Node *node, int event);
    void pointerPressed();
    void pointerReleased();
    void rightButtonPressed();
    void rightButtonReleased();
    void middleButtonPressed();
    void middleButtonReleased();
    void movePointer(float x, float y);
    void keyPressed(unsigned short key);
    void keyReleased(unsigned short key);
    void characterEntered(wchar_t character);
    // Fill and outline colour stacks.
    void pushFillColor();
    void pushOutlineColor();
    void pushExtrusionColor();
    void setFillColor(const float *color)
    {
        for (int i = 0; i < 4; i++)
            m_fillColor.m_elements[i] = color[i];
    }
    void setOutlineColor(const float *color)
    {
        for (int i = 0; i < 4; i++)
            m_outlineColor.m_elements[i] = color[i];
    }
    void setExtrusionColor(const float *color)
    {
        for (int i = 0; i < 4; i++)
            m_extrusionColor.m_elements[i] = color[i];
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
    // Shows popUp for button; a null popUp hides it.
    void setPopUpWidget(Widget *popUp, Widget *button);
    void setPointerCapture(Node *node);
    void releasePointerCapture();
    void setFocusWidget(Widget *widget);
    // Called by game input handlers.
    Widget *getFocusWidget();
    void beginOffscreenDrawing(float scale);
    void endOffscreenDrawing();
    void clearSurfaceBounds(Node *node, float scale);

    FontEngine *getFontEngine() { return m_fontEngine; }
    bool usesDepthLayers() const { return (m_renderFlags >> RENDER_DEPTH_LAYERS) & 1; }
    bool usesWidgetSurfaces() const { return (m_renderFlags >> RENDER_CLIP_WIDGET_SURFACES) & 1; }
    bool usesBlur() const { return (m_renderFlags >> RENDER_BLUR) & 1; }
    Vector<2, float> getPointer() const { return m_pointer; }
    Vector<2, float> getPreviousPointer() const { return m_previousPointer; }
    bool isKeyDown(unsigned short key) { return m_pressedKeys.find(key) != m_pressedKeys.end(); }
    // Whether the pointer button (a bit of m_buttons) is down.
    bool isButtonDown(int button) const { return (m_buttons >> button) & 1; }
    // Directly accessible input and viewport state used by game widgets.
    Node *root() const { return m_root; }
    int getFrameTime() const { return m_frameTime; }
    int getCaretClock() const { return m_caretClock; }
    int viewportWidth() const { return m_viewportWidth; }
    int viewportHeight() const { return m_viewportHeight; }

protected:
    friend class D3D11Drawing;    // Binds m_filterSurface and reads the viewport size during drawing.
    friend class Texture;         // registers itself in m_texturesById
    friend class Font;            // registers itself in m_fontList and m_fontsById
    friend class SmoothMeshShape; // Draws using the fill, outline and extrusion colours.
    friend class StaticMeshShape; // Draws using the fill and outline colours.
    friend class Button;          // Reads m_frameTime during update.
    friend class ScrollSlider;    // Reads pointer and button state on pointer movement.
    friend class ScrollButton;    // Reads m_frameTime during update.
    friend class PopUpButton;     // Reads m_pressedPopUpItem on pointer release.
    friend class Widget;          // reads the pointer, the viewport and the capture
    friend class Edit;            // Reads m_buttons on pointer movement.
    friend class Node;            // render flags, surfaces, viewport
    friend class cube::Game;
    friend class cube::Interface;
    int m_renderTargetSwitchCount;
    Page m_page;
    FontEngine *m_fontEngine;                      // new FontEngine in the constructor
    std::vector<RenderSurface *> m_renderSurfaces; // the off-screen pool (beginOffscreenDrawing)
    RenderSurface *m_blurSurface;                  // resized by drawWindow, deleted by the destructor
    int m_surfaceDepth;                            // Initially -1.
    RenderSurface *m_filterSurface;                // D3D11Engine::beginTextDrawing binds it
    std::list<RenderSurface *> m_surfaceStack;
    CRITICAL_SECTION m_lock; // Critical section with spin count 16.
    FT_Library m_freeType;
    std::list<Shape *> m_shapes;
    std::list<Widget *> m_widgets;
    std::list<Transformation *> m_transformations;
    std::list<Display *> m_displays;
    std::list<Node *> m_nodes;
    std::list<Texture *> m_textures;
    std::list<Font *> m_fontList;
    std::map<std::wstring, Movie *> m_movies;
    Node *m_root;    // "root"
    Node *m_sources; // "sources", NODE_INTERNAL
    Node *m_fonts;   // "fonts", NODE_INTERNAL
    Node *m_overlayRoot;
    Node *m_pointerNode;
    Node *m_pointerCapture; // setPointerCapture
    Widget *m_focusWidget;  // getFocusWidget, the key handlers
    Widget *m_popUpWidget;
    Vector<2, float> m_pointer; // movePointer's position
    Vector<2, float> m_previousPointer;
    int m_frameTime;                        // Advance
    int m_caretClock;                       // Advance
    std::set<unsigned short> m_pressedKeys; // keyPressed / keyReleased
    unsigned m_buttons;                     // one bit per MouseButton
    int m_lastKey;                          // not initialised; the key handlers store it
    Widget *m_pressedPopUpItem;             // cleared by deleteWidget
    int m_wheelDelta;                       // not initialised by the constructor
    int m_viewportX;                        // setViewport
    int m_viewportY;
    int m_viewportWidth;  // Initially 1000.
    int m_viewportHeight; // Initially 800.
    int m_windowWidth;
    int m_windowHeight;
    std::map<std::string, int> m_unusedStringMap;
    std::list<std::string> m_unusedStringList;
    std::map<int, Texture *> m_texturesById; // findTexture
    std::map<int, Font *> m_fontsById;
    std::list<std::wstring> m_fontPaths;
    std::list<Vector<4, float>> m_fillColorStack;
    std::list<Vector<4, float>> m_outlineColorStack;
    std::list<Vector<4, float>> m_extrusionColorStack;
    Vector<4, float> m_fillColor;      // 1, 1, 1, 1
    Vector<4, float> m_outlineColor;   // 1, 1, 1, 1
    Vector<4, float> m_extrusionColor; // 1, 1, 1, 1
    unsigned m_renderFlags;            // RenderFlags, 7
    bool m_unused28C;                  // true; no reader found
};

}
