// Engine owns engine objects through ObjectManager. It keeps shapes, widgets, transformations, displays, nodes and
// textures in lists, along with the scene roots, font engine and input state. GLEngine provides the OpenGL
// backend.
#pragma once
#include <windows.h>
#include <list>
#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "plasma/ObjectManager.h"
#include "plasma/Vector.h"
#include "plasma/Matrix.h"

// FreeType's library handle.
typedef struct FT_LibraryRec_ *FT_Library;

class Workspace;
class ColorTool;
class PaletteWidget;
class ColorPickerWidget;
class FontWidget;
class NodeTreeWidget;
class InterfaceViewWidget;
class Module;
class OptionsModule;
class NodeTool;
class ProceduralShapeModule;
class StampModule;
class TextInterface;
class ColorModule;
class SpecialColorModule;
class ScaleBoxModule;
class PaletteModule;
class DrawFaceTool;
class ExportModule;
class EdgeTool;
class FaceTool;
class VertexTool;
class Sheet;
class DesignSheet;
class TextureModule;
class TextModule;
class BoundingBoxTool;
struct HWND__;
namespace plasma {

// Shared outline-import raster dimensions (pixels).
enum {
    ImportedGlyphPixelHeight = 100
};

class Engine;
}
long __stdcall wndProc(HWND__ *window, unsigned int message, unsigned int wparam, long lparam);

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
class GenericWidget;
class GenericShape;
class RenderSurface;
class Engine;
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

// The flags argument of ReadPlasmaFileIntoNode, ReadPLAFile and the PLXStreamReader (PLXStreamReader::m_flags).
enum SceneLoadFlags {
    LOAD_SHARED = 0,
    LOAD_NO_WIDGET_INIT = 1,
    LOAD_INTERNAL = 2,
    LOAD_IGNORE_PAGE = 3,
    LOAD_STATIC_MESHES = 4,
    LOAD_HINTED_TEXT = 5,
    LOAD_CHECK_SIGNATURE = 6
};

// The pointer buttons: Engine::IsButtonDown's index and the bits of Engine::m_buttons (PointerPressed,
// RightButtonPressed, MiddleButtonPressed).
enum MouseButton {
    MOUSE_BUTTON_LEFT = 0,
    MOUSE_BUTTON_RIGHT = 1,
    MOUSE_BUTTON_MIDDLE = 2
};

// Engine::m_renderFlags (all set initially).
enum RenderFlags {
    RENDER_ANTIALIAS = 0,
    RENDER_CLIP_WIDGET_SURFACES = 1,
    RENDER_BLUR = 2
};

class Engine : public ObjectManager {
    // Reads the embedded engine's input state for the editor.
    friend class ::Workspace;
    friend class ::Module;
    friend class ::OptionsModule;
    friend class ::NodeTool;
    friend class ::ProceduralShapeModule;
    friend class ::StampModule;
    friend class ::TextInterface;
    friend class ::ColorModule;
    friend class ::SpecialColorModule;
    friend class ::ScaleBoxModule;
    friend class ::PaletteModule;
    friend class ::DrawFaceTool;
    friend class ::ColorTool;    // Reads the pointer to pick a colour (PickColor).
    friend class ::ExportModule; // Renders the export tiles: viewport size, render flags.
    friend class ::EdgeTool;
    friend class ::FaceTool;
    friend class ::VertexTool;
    friend class ::Sheet;               // Reads the window size.
    friend class ::DesignSheet;         // Reads the viewport size.
    friend class ::TextureModule;       // The sources, textures and shapes.
    friend class ::TextModule;          // Moves the font list into the root.
    friend class ::InterfaceViewWidget; // Reads the viewport height.
    friend class ::BoundingBoxTool;     // Reads the font engine (RenderBox).
    friend long __stdcall ::wndProc(HWND__ *window, unsigned int message, unsigned int wparam,
                                    long lparam); // Likewise.
    friend int WINAPI ::WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR commandLine,
                                int showCommand); // The root's flags.
    friend class PLXStreamReader;
    friend class PLXStreamWriter;
    friend class Texture;
    friend class Font;            // Registers each font in the font list and id map.
    friend class Node;            // Render and HitTest: render flags, surfaces, viewport.
    friend class GLDrawing;       // Binds the filter surface (Draw).
    friend class SmoothMeshShape; // Draws with the current colours.
public:
    const Vector<4, float> &getFillColor() const { return m_fillColor; }
    const Vector<4, float> &getOutlineColor() const { return m_outlineColor; }
    struct Page;

    Engine();
    virtual ~Engine();

    // Virtual rendering operations implemented by GLEngine.
    virtual void setupShadersAndFrameBuffer() {}
    virtual Texture *createTexture(int width, int height, const unsigned char *pixels, const TextureFormat &format,
                                   const std::wstring &name = L"", bool transient = false) = 0;
    virtual void drawCaret(const Vector<2, float> &position, const Vector<2, float> &size,
                           const Vector<4, float> &color) = 0;
    virtual void invertRectangle(const Vector<2, float> &position, const Vector<2, float> &size) = 0;
    virtual void drawTexturedRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                                       const Vector<2, float> &texturePosition, const Vector<2, float> &textureSize,
                                       const Vector<4, float> &color) = 0;
    virtual void fillRectangle(const Vector<2, float> &position, const Vector<2, float> &size,
                               const Vector<4, float> &color) = 0;
    virtual void frameRectangle(const Vector<2, float> &, const Vector<2, float> &, const Vector<4, float> &) {}
    virtual void pushIdentityMatrix() {}
    virtual void popMatrix() {}
    virtual void enableTexture2D() {}
    virtual void disableTexture2D() = 0;
    virtual Drawing *createDrawing() = 0;
    virtual RenderSurface *createRenderSurface(int) { return 0; }
    virtual void getModelViewMatrix(Matrix<float> &) {}
    virtual void loadMatrix(Matrix<float> &) {}
    virtual void beginTextDrawing(Node *) {}
    virtual void endTextDrawing(Node *) {}
    virtual void beginWindowDrawing() = 0;
    virtual void endWindowDrawing() = 0;
    virtual void pushNodeMatrix(Node *) = 0;
    virtual void popNodeMatrix(Node *) = 0;
    // Empty in GLEngine and currently unused.
    virtual void unused22(Node *) = 0;
    virtual void unused23(Node *) = 0;
    virtual void beginSurface(RenderSurface *, const Vector<2, float> &, const Vector<2, float> &, float scale, bool) {}
    virtual void endSurface() {}
    virtual void drawPixels(const unsigned char *, int stride, int, int x, int y, int width, int height) {}

    // Guard the object lists (the Add and Remove functions, Texture::Texture).
    void lock();
    void unlock();

    // A new node named name with the given parts; a missing transformation or display is created (unnamed). The node
    // becomes a child of parent when one is given.
    Node *createNode(Transformation *transformation, Shape *shape, Display *display, Node *parent,
                     const std::wstring &name = L"");

    // Creates unnamed objects when no name is supplied.
    Transformation *createTransformation(const std::wstring &name = L"");
    Display *createDisplay(const std::wstring &name = L"");

    // A new movie named name replaces the one of that name (deleted).
    Movie *createMovie(const std::wstring &name = L"");

    // Keeps the path and passes it to the font engine.
    void addFontPath(const std::wstring &path);

    // A TrueType path loads as a pixel font by default; pixel=false loads an outline font.
    Font *loadLegacyFont(const std::wstring &path, float size, float stroke, bool transient = false, bool pixel = true);
    PlasmaFont *loadPlasmaFont(const std::wstring &path, float size);
    Texture *loadImageTexture(const std::wstring &path);
    PixelFont *createPixelFont(const std::wstring &name = L"");
    PlasmaFont *createPlasmaFont(const std::wstring &name = L"");
    PlasmaFont *createPlasmaFont(Node *root, const std::wstring &name);
    PlasmaFont *loadOutlineFont(const std::wstring &path, float size, const std::wstring &characters = L"");
    PixelFont *createPixelFont(Texture *texture, const std::wstring &characters = L"");

    // The character list is unused.
    PixelFont *loadPixelFont(const std::wstring &path, float size, bool transient, float stroke,
                             const std::wstring &characters = L"");
    Movie *findMovie(const std::wstring &name); // 0 when there is none.
    Node *findNode(const std::wstring &name);   // The first of that name, or 0.

    void setDimensions(int width, int height);

    // The node that receives the pointer events instead of the node under the pointer, until released.
    void setPointerCapture(Node *node);
    // Releases the capture and sends the pointer events for the node under the pointer.
    void releasePointerCapture();
    Widget *getFocusWidget();

    // Invokes the connection the enclosing widget of node has for event; nothing for a node with NODE_INACTIVE or a
    // disabled widget's enabled-only connection.
    static void sendEvent(Node *node, int event);

    // Each makes a new widget shown with node (Widget.cpp, Button.cpp and GenericWidget.cpp construct them); the
    // engine keeps them in m_widgets.
    Widget *createWidget(Node *node, const std::wstring &name = L"");
    Button *createButton(Node *node, const std::wstring &name = L"");

    // A widget with its own node and generic shape (shape names the shape's kind, e.g. "panel"), a child of parent, at
    // position with size.
    GenericWidget *createGenericWidget(const std::wstring &name, float x, float y, float width, float height,
                                       Node *parent, const std::wstring &shape);
    GenericShape *createGenericShape(const std::wstring &source = L"", const std::wstring &name = L"");

    Texture *findTexture(int id);

    // Factories used by the PLX reader. PopUpButton and ListWidget are made without a node.
    TextShape *createTextShape(const std::wstring &text = std::wstring(), const std::wstring &name = L"");
    SmoothMeshShape *createSmoothMeshShape(const std::wstring &name = L"");

    // The shape copies (MeshShape::Clone, CurveShape::Clone) and the PLA curve reader make their shapes through these.
    MeshShape *createMeshShape(const std::wstring &name = L"");
    CurveShape *createCurveShape(const std::wstring &name = L"");
    ScrollSlider *createScrollSlider(Node *node, const std::wstring &name = L"");
    ScrollButton *createScrollButton(Node *node, const std::wstring &name = L"");
    PopUpButton *createPopUpButton(Node *node, const std::wstring &name = L"");
    ListWidget *createListWidget(Node *node, const std::wstring &name = L"");
    Edit *createEdit(Node *node, const std::wstring &name = L"");

    bool readPlasmaFileIntoNode(const std::wstring &path, Node *node, int flags, Page *page,
                                const std::vector<unsigned __int64> &keys);
    void writePlasmaFile(const std::wstring &path, Node *root, Page *page, unsigned __int64 key);
    void readPLAFile(std::ifstream &stream, Node *target, int flags, Page *page);
    void writePLA(std::ostream &stream, Node *root, Page *page);
    bool readSources(const std::wstring &path, unsigned __int64 key);
    void resolveSources(Node *node = 0);
    void resolveSources(Node *sources, Node *node);
    void collectSourceNodes(Node *node, std::map<std::wstring, Node *> &sources);
    void resolveSourceNodes(std::map<std::wstring, Node *> &sources, Node *node);
    void writePLX(std::ostream &stream, Node *root, Page *page, unsigned __int64 key);
    void readPLXFile(std::istream &stream, Node *target, int flags, Page *page,
                     const std::vector<unsigned __int64> &keys);

    // The surface stack (pushed and popped by GLEngine).
    void pushSurface(RenderSurface *surface);
    RenderSurface *popSurface();
    RenderSurface *topSurface();

    // Each locks the engine, appends and returns the new entry (the object keeps it to unregister).
    std::list<Object *>::iterator addObject(Object *object);
    std::list<Shape *>::iterator addShape(Shape *shape);
    std::list<Transformation *>::iterator addTransformation(Transformation *transformation);
    std::list<Display *>::iterator addDisplay(Display *display);
    std::list<Widget *>::iterator addWidget(Widget *widget);

    // Each removes the object from the engine's list and deletes it.
    void deleteShape(Shape *shape);
    void deleteDisplay(Display *display);
    void deleteTransformation(Transformation *transformation);
    static void deleteDrawing(Drawing *drawing); // Shared delete helper.
    static void deleteObject(Object *object);    // Delete object.

    // The font renderers' colour stacks. Each Push saves the current colour and each Set replaces it; the callers push
    // all their colours before setting any (ScalableFont::DrawGlyphs, PlasmaFont).
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
    // Restores one saved colour, if any.
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
    void deleteTexture(Texture *texture);
    static void deleteRenderSurface(RenderSurface *surface);
    void destroyNode(Node *node);
    std::list<Node *>::iterator addNode(Node *node);
    std::list<Texture *>::iterator addTexture(Texture *texture);

    void deleteWidget(Widget *widget);
    Widget *findWidget(const std::wstring &name);
    void setViewport(int x, int y, int width, int height, bool notify);
    void initializeWidgets();
    bool advance(int milliseconds);
    Node *hitTest(const Vector<2, float> &position);
    Node *getPointerNode();
    void dispatchWidgets(Node *node, void (Widget::*method)());
    void pointerPressed();
    void pointerReleased();
    void pointerDoubleClicked();
    void rightButtonPressed();
    void rightButtonReleased();
    void middleButtonPressed();
    void middleButtonReleased();
    void pointerWheel(int delta);
    void movePointer(float x, float y);
    void keyPressed(unsigned short key);
    void keyReleased(unsigned short key);
    // Whether the pointer button's bit is set in m_buttons.
    bool isButtonDown(int button) const { return (m_buttons >> button) & 1; }
    // The viewport size used for workspace layout.
    int viewportWidth() const { return m_viewportWidth; }
    int viewportHeight() const { return m_viewportHeight; }
    // A copy of the pointer position.
    Vector<2, float> getPointer() const { return m_pointer; }
    // Whether the key, specified by virtual-key code, is held.
    bool isKeyDown(unsigned short key) { return m_pressedKeys.find(key) != m_pressedKeys.end(); }
    // The font engine used by text shapes.
    FontEngine *getFontEngine() { return m_fontEngine; }
    void characterEntered(int character);
    void setFocusWidget(Widget *widget);
    void resetPointer();
    void resetInput();
    void setPopUpWidget(Widget *widget, Widget *anchor);
    int drawWindow();
    void beginOffscreenDrawing(float scale);
    void endOffscreenDrawing();
    void clearSurfaceBounds(Node *node);
    static void writePLATransformation(std::ostream &stream, Transformation *transformation);
    Transformation *readPLATransformation(std::istream &stream, std::map<int, std::string> &names);

protected:
    friend class Widget;              // Reads the pointer, the viewport and the capture.
    friend class Button;              // Reads the frame time.
    friend class Edit;                // The pointer and the buttons.
    friend class ListWidget;          // The sources.
    friend class PopUpButton;         // The moving widget.
    friend class ScrollButton;        // The frame time.
    friend class ScrollSlider;        // The pointer and the buttons.
    friend class ::PaletteWidget;     // The pointer and the capture.
    friend class ::ColorPickerWidget; // The pointer and the buttons.
    friend class ::FontWidget;        // The viewport height.
    friend class ::NodeTreeWidget;    // The pointer and the render flags.

    int m_renderTargetSwitchCount;

public:
    // A document's page settings: width, height, DPI, display unit and background colour, serialized as PLX page
    // chunks. Each editor sheet also keeps these settings.
    struct Page {
        Page() {}
        // A page with the given size and resolution, a white background, and pixel display units.
        Page(float width, float height, float resolution)
            : size(width, height, resolution),
              unit(0),
              color(1.0f, 1.0f, 1.0f, 0.0f)
        {}

        Vector<3, float> size;  // Width, height and dpi.
        int unit;               // The display unit (FormatModule's UNIT_*).
        Vector<4, float> color; // Background colour; DesignSheet::ApplyBackgroundColor uses RGB.
    };

protected:
    Page m_page;
    FontEngine *m_fontEngine;
    std::vector<RenderSurface *> m_renderSurfaces; // Resized with the viewport.
    RenderSurface *m_blurSurface;                  // Likewise; the blur's second (ping-pong) surface (Node::Render).
    int m_surfaceDepth;                            // -1.
    RenderSurface *m_filterSurface; // The shape shader's "filterTexture" (unit 1): the last off-screen drawing.
    std::list<RenderSurface *> m_surfaceStack;
    CRITICAL_SECTION m_lock; // Guards the object lists.
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
    Node *m_sources; // Internal (NODE_INTERNAL).
    Node *m_fonts;   // Internal (NODE_INTERNAL).
    Node *m_overlayRoot;
    Node *m_pointerNode;    // Cleared when the node is destroyed.
    Node *m_pointerCapture; // Likewise.
    Widget *m_focusWidget;  // Cleared when the widget is destroyed.
    Widget *m_popUpWidget;
    Vector<2, float> m_pointer; // The pointer position.
    Vector<2, float> m_previousPointer;
    int m_frameTime;
    int m_caretClock;
    std::set<unsigned short> m_pressedKeys;
    unsigned m_buttons;         // MouseButtonMask bits.
    int m_lastKey;              // Not initialised by the constructor.
    Widget *m_pressedPopUpItem; // The enabled WIDGET_POPUP_ITEM widget last pressed; its release closes the pop-up.
    int m_wheelDelta;
    int m_viewportX;
    int m_viewportY;
    int m_viewportWidth;  // Initially 1000 pixels.
    int m_viewportHeight; // Initially 800 pixels.
    int m_windowWidth;
    int m_windowHeight;
    // Initialized and destroyed, but otherwise unused.
    std::map<std::string, void *> m_unused154;
    std::list<std::string> m_unused164;
    std::map<int, Texture *> m_texturesById;
    std::map<int, Font *> m_fontsById;
    std::list<std::wstring> m_fontPaths;
    std::list<Vector<4, float>> m_fillColorStack;
    std::list<Vector<4, float>> m_outlineColorStack;
    std::list<Vector<4, float>> m_extrusionColorStack;
    Vector<4, float> m_fillColor;      // Initially opaque white.
    Vector<4, float> m_outlineColor;   // Initially opaque white.
    Vector<4, float> m_extrusionColor; // Initially opaque white.

    // Hit tests clip to widget surfaces (Node::HitTest).
    bool usesWidgetSurfaces() const { return (m_renderFlags & (1 << RENDER_CLIP_WIDGET_SURFACES)) != 0; }
    // RENDER_BLUR enables blurred node surfaces. Pointer-button releases enable it again after interactive edits.
    void enableBlur() { m_renderFlags |= (1 << RENDER_BLUR); }
    unsigned m_renderFlags; // RenderFlags, all three set.

    // Temporarily cleared and restored during thumbnail and tile rendering; otherwise unused.
    bool m_unused1F4;
};

}
