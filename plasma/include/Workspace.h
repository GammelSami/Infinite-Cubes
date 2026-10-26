// Workspace is the editor's single global state, g_workspace. It embeds the engine and script machine, owns the
// modules and open sheets, and keeps the interface's node tree, keyboard shortcuts and per-frame input state.
#pragma once
#include <windows.h>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <iosfwd>
#include "plasma/Matrix.h"
#include "plasma/GLEngine.h"
#include "plasma/Vector.h"
#include "abstr/Machine.h"

namespace plasma {
class Node;
class Drawing;
}

class Module;
class Sheet;
class DesignSheet;
class TextureSheet;
class NodeInterface;
class TextInterface;
class EditableMesh;
struct MeshVertex;
struct MeshHalfEdge;
namespace plasma {
class Widget;
class ListWidget;
}
class Tool;

// The modules (each in its own file; the constructors Workspace::Init calls).
class ToolModule;
class ColorModule;
class PaletteModule;
class SpecialColorModule;
class ColorManagementModule;
class StatusModule;
class ToolInfoModule;
class ProgressModule;
class PerformanceModule;
class ProceduralShapeModule;
class ColorToolModule;
class NodeToolModule;
class ShadowModule;
class ExportPDFModule;
class VertexModule;
class StampModule;
class ScaleBoxModule;
class StrokeWidthModule;
class DrawFreehandModule;
class TextureModule;
class TextModule;
class WidgetModule;
class SequenceModule;
class AnimationModule;
class ExportModule;
class FormatModule;
class NodeModule;
class MeshModule;
class HistoryModule;
class ClipboardModule;
class GridModule;
class ZoomModule;
class OptionsModule;
class OnlineModule;
class AboutModule;

// The screenshot frame (F1): its size (Screenshot), and the pointer in window pixels it is centred on (MouseMove).
extern int g_screenshotWidth;
extern int g_screenshotHeight;
extern int g_mouseX;
extern int g_mouseY;

class Workspace {
public:
    // m_serverRequest: the request under way, by the path it sends ("/Activation/<name>/?id=" to the licence server;
    // release.xml and the release's files to the download server).
    enum ServerRequest {
        REQUEST_NONE = 0,
        REQUEST_RELEASE_INFO = 1, // "/Download/Plasma/release.xml" (Init).
        REQUEST_LOGIN = 2,        // Login.
        REQUEST_LOGOUT = 3,       // Logout.
        REQUEST_RELOGIN = 4,      // Relogin (WM_TIMER).
        REQUEST_ACTIVATE = 5,     // Activate.
        REQUEST_DEACTIVATE = 6,   // Deactivate.
        REQUEST_UPDATE_FILE = 7,  // A file of the newer release (DownloadUpdate).
        REQUEST_VALIDATE = 8      // Validate.
    };

    // The keyboard shortcuts' actions (m_keyActions and its siblings), named after what DispatchAction runs for them;
    // 0 is no action.
    enum KeyAction {
        ACTION_NONE = 0,
        ACTION_UNDO = 1,
        ACTION_REDO = 2,
        ACTION_OPEN = 3,
        ACTION_CLOSE_SHEET = 4,
        ACTION_SAVE = 5,
        ACTION_SAVE_AS = 6,
        ACTION_DELETE = 7, // The current tool's DeleteSelected.
        ACTION_COPY = 8,
        ACTION_CUT = 9,
        ACTION_PASTE = 10,
        ACTION_PASTE_INSTANCE = 11,
        ACTION_DUPLICATE = 12,
        ACTION_DUPLICATE_INSTANCE = 13,

        // The arrangement commands, of the sheet tool or else the node tool.
        ACTION_SEND_TO_BACK = 14,
        ACTION_BRING_TO_FRONT = 15,
        ACTION_SEND_BACKWARD = 16,
        ACTION_BRING_FORWARD = 17,

        // ToolModule::SetTool with the tool's type.
        ACTION_SHEET_TOOL = 18,
        ACTION_NODE_TOOL = 19,
        ACTION_VERTEX_TOOL = 20,
        ACTION_EDGE_TOOL = 21,
        ACTION_FACE_TOOL = 22,
        ACTION_COLOR_TOOL = 23,
        ACTION_WIDTH_TOOL = 24,
        ACTION_EXTRUSION_TOOL = 25,
        ACTION_DRAW_TOOL = 26,
        ACTION_SPLIT_TOOL = 27,
        ACTION_TEXT_TOOL = 28,
        ACTION_WIDGET_TOOL = 29,  // Type 12: SetTool has no case for it.
        ACTION_GENERIC_TOOL = 30, // Type 7: no case in SetTool, no key.
        ACTION_DRAW_FREEHAND_TOOL = 31,
        ACTION_PROCEDURAL_TOOL = 32,
        ACTION_STAMP_TOOL = 33,
        ACTION_RESET_TOOLS = 34,      // Escape.
        ACTION_TOGGLE_SELECT_ALL = 35 // Tab.
    };

    Workspace();
    virtual ~Workspace();

    DesignSheet *getDesignSheet();   // The current sheet if it is an open design sheet.
    TextureSheet *getTextureSheet(); // The current sheet if it is a texture sheet.
    plasma::Node *getHoveredNode();
    NodeInterface *getHoveredSheetNode();
    // Every module's CurrentNodeChanged, then Redraw.
    void updateModules();
    void selectionChanged();
    void redraw();

    // Between the open design sheet's display unit and internal points (the value unchanged without an open sheet).
    float toDesignSheetInternalUnits(float value);
    float fromDesignSheetInternalUnits(float value);
    // Called after selection changes (NodeTool): the timeline module (never created), if any, refreshes (with 0).
    void refreshTimeline();

    // The selected nodes of the design sheet (DesignSheet::GetSelectedNodes).
    void getSelectedNodes(std::list<plasma::Node *> &nodes);
    // Marks the open design sheet's root transformation changed and asks for a redraw (m_redrawRequested), after the
    // node order changed (NodeTool), a release, a frame move.
    void markDesignSheetChanged();
    // Calls the function for every shown, unlocked sheet node from the root's sheet node down; the callbacks test the
    // selection themselves (NodeTool passes DeselectSheetNode).
    void forEachUnlockedSheetNode(void (*function)(NodeInterface *));

    // Calls the function for the sheet node and its shown descendants, skipping locked nodes (their children are still
    // visited).
    void forEachShownSheetNode(NodeInterface *sheetNode, void (*function)(NodeInterface *));
    // After a node moved in the hierarchy (NodeTool's arrangement commands, MoveNode for the old and new parent): a
    // node that aligns its children along its outline (NODE_ALIGN_OUTLINE, "node:align:outline") has its sheet node's
    // shape interface place them again (AlignChildrenToOutline, MeshInterface).
    void realignChildren(plasma::Node *node);
    // Moves the node under the parent, before the sibling (NodeTool).
    void moveNode(plasma::Node *node, plasma::Node *parent, plasma::Node *before);

    // Moves the selected nodes at or below node (the open design sheet's root if null) into parent.
    void moveSelectedNodes(plasma::Node *node, plasma::Node *parent);

    // The same before ("Send Behind...") or after ("Send in Front of...") the target, under its parent.
    void moveSelectedNodesBehind(plasma::Node *node, plasma::Node *target);
    void moveSelectedNodesInFrontOf(plasma::Node *node, plasma::Node *target);

    // Answers the pending "Choose Parent...", "Send Behind..." or "Send in Front of..." request with the pressed node;
    // false without one.
    bool pickRequestedNode(plasma::Node *node);

    // "Set Pivot...": the selected nodes' pivots move to the point.
    void setPivotAt(const plasma::Vector<2, float> &point);

    // The design sheet unless it is busy loading or saving (defined in DesignSheet.h).
    inline DesignSheet *getOpenDesignSheet();
    // The node of the interface icon of that name ("icon:sheet"...), if any.
    plasma::Node *findIcon(const std::wstring &name);

    // The current sheet unless it is busy loading or saving (defined in DesignSheet.h).
    inline Sheet *getOpenSheet();
    plasma::Engine *getEngine() { return &m_engine; }
    OptionsModule *getOptionsModule() const { return m_optionsModule; }
    plasma::Node *getSheetTemplate() const { return m_sheetTemplate; }
    SpecialColorModule *getSpecialColorModule() const { return m_specialColorModule; }
    ColorManagementModule *getColorManagementModule() const { return m_colorManagementModule; }
    VertexModule *getVertexModule() const { return m_vertexModule; }
    SequenceModule *getSequenceModule() const { return m_sequenceModule; }
    GridModule *getGridModule() const { return m_gridModule; }
    // Accessors for the panels shown by NodeModule's buttons.
    PaletteModule *getPaletteModule() const { return m_paletteModule; }
    ProgressModule *getProgressModule() const { return m_progressModule; }
    ColorModule *getColorModule() const { return m_colorModule; }
    NodeToolModule *getNodeToolModule() const { return m_nodeToolModule; }
    MeshModule *getMeshModule() const { return m_meshModule; }
    TextModule *getTextModule() const { return m_textModule; }
    TextureModule *getTextureModule() const { return m_textureModule; }
    // The tool module used by key handling and context menus.
    ToolModule *getToolModule() const { return m_toolModule; }
    ToolInfoModule *getToolInfoModule() const { return m_toolInfoModule; }
    // The "gui" node used by module edit callbacks.
    plasma::Node *getGuiNode() const { return m_guiNode; }
    // Whether this is the demo version.
    bool isDemo() const { return m_demo; }
    // Marks every open sheet's root transformation changed and requests a redraw.
    void markSheetsChanged();
    // Input from the window (WndProc).
    void characterEntered(int character);
    void keyReleased(unsigned short key);
    void keyPressed(unsigned short key);

    // Runs a keyboard shortcut's action.
    void dispatchAction(int action);

    // Releases the pointer buttons still down (focus lost) and resets the engine's input.
    void releaseButtons();

    // The pointer buttons, after the engine's handlers of the same names.
    void pointerReleased();
    void middleButtonReleased();
    void pointerDoubleClicked();
    void middleButtonPressed();
    void pointerWheel(int delta);

    // Updates the scene's matrices and the hover state at the pointer.
    void refreshPointer();

    // Zooms the current sheet's view about its centre: to delta times the sheet's zoom scale (absolute: the zoom
    // module) or by 1.5^delta (the mouse wheel).
    void zoomView(float delta, bool absolute);

    // The right button on the scene (WndProc).
    void rightButtonPressed();
    void rightButtonReleased();

    // The scene's context menu at the pointer, and its own commands for the selected shapes (each shared shape once):
    // "Refine", "Refine Contour" and, for meshes, "Add Border Edges".
    void showPopupMenu();
    void refine();
    void refineContour();
    void addBorderEdges();

    // Collects the drag's snap offsets (m_snapOffsets): the selection's snap points relative to m_sheetPointer.
    void collectSnapOffsets();
    // Snapping (the drag's pointer, ProceduralTool, StampTool): the snapable meshes' edges as point pairs, the point
    // snapped to them and then to the grid.
    void collectSnapSegments(plasma::Node *node, std::vector<plasma::Vector<2, float>> &segments);
    plasma::Vector<2, float> snapPoint(const std::vector<plasma::Vector<2, float>> &segments,
                                       const plasma::Vector<2, float> &point);
    plasma::Vector<2, float> snapToSegments(const std::vector<plasma::Vector<2, float>> &segments,
                                            const plasma::Vector<2, float> &point);
    plasma::Vector<2, float> snapToGrid(const plasma::Vector<2, float> &point);
    // The pointer snapped with the drag's snap offsets or by itself. The tools use the configured tool radius for
    // nearby vertices and text.
    float getToolRadius() const { return m_toolRadius; }
    const plasma::Vector<2, float> &getSceneMousePosition() const { return m_sceneMousePosition; }
    plasma::Vector<2, float> getSnappedPointer(bool offsets);

    // Ends editing the text interface (committing an emptied text's deletion) and makes the given one current.
    void setTextInterface(TextInterface *textInterface);
    // The text interface being edited if the current tool is the text tool (TextTool,; TextInterface).
    TextInterface *getEditingTextInterface();

    // The popup menu: adds a separator copied from the separator template, and closes the menu.
    plasma::Widget *addPopupMenuSeparator();

    // The menu commands that show a module's panel (WndProc).
    void showExportPDF();
    void showOptions();
    void showProductActivation();

    // Hides the software cursor (m_softwareCursor) if it is shown, when the pointer leaves the client area (WndProc's
    // WM_NCMOUSEMOVE).
    void hideSoftwareCursor();

    // Panels docked at the screen edges: kept in a list and moved under the edge's node (Workspace::Init).
    void addBottomPanel(plasma::Widget *panel);  // "gui:bottom".
    void addTopPanel(plasma::Widget *panel);     // "gui:top".
    void addBottom2Panel(plasma::Widget *panel); // "gui:bottom2".
    void addRightPanel(plasma::Widget *panel);   // "gui:right".

    // Runs the script function of that name, under the lock, and drops its result.
    void runScriptFunction(const std::string &name);

    // The view pans: the middle button or the space bar is down, outside a pointer capture.
    bool isPanning();
    // The current tool's Render, then the "icon:constrain:y" and "icon:constrain:x" icons by the scene pointer while
    // the grid does not move along that axis (InterfaceViewWidget, TextureSpaceWidget).
    void drawConstraintIcons();

    // The drag's movement since the previous pointer move, in the current sheet's root coordinates; zero while the
    // cursor sticks.
    plasma::Vector<2, float> getDragMovement();

    // Makes the sheet current: its frame plays "activate" (the previous one's "deactivate"), the window title follows
    // a design sheet, the modules are told (SheetChanged).
    void setSheet(Sheet *sheet);

    // Makes the sheet under the pointer current.
    void activateHoveredSheet();

    // Shows only the current sheet, or all of them again.
    void showCurrentSheetOnly();
    void showAllSheets();

    // Sheet menu "New Sheet" (WndProc, WinMain; SheetTool's "Create Sheet"): a new design sheet, made current.
    void newSheet();

    // Sheet menu "Set Sheet Color" ("set sheet color" operation): the current sheet's background takes the colour
    // panel's colour.
    void setSheetColor();

    // Sheet menu "Get Sheet Color": the colour panel takes the current sheet's background colour.
    void getSheetColor();

    // Sheet menu "Texture sheet": opens the texture sheet (m_textureSheet), or closes it keeping its frame's
    // placement.
    void openTextureSheet();
    void closeTextureSheet();

    // WinMain's clean-up before exit (licence logout, palette).
    void cleanupForExit();

    // WinMain's start-up script run ("initialize", "update", "button:press").
    void runStartupScript();

    // WndProc's commands, by their menu items.
    void closeSheet(); // "Close Sheet".
    bool canClose();   // WM_CLOSE, "Exit".

    // Closes every sheet: the texture sheet, then deletes the others (exit, logout, deactivation, update download, a
    // failed validation).
    void closeAllSheets();
    void pointerPressed(); // WM_LBUTTONDOWN (also the screenshot, F1).
    void updateCursor();   // WM_SETCURSOR.
    void newFile();        // "New".
    void importSVG();
    void importPDF();
    void importPGD();

    // False if there is no open design sheet or the user cancels.
    bool savePlasmaFile();
    void savePlasmaFileAs();
    void savePlasmaFileCopy();
    void importTTF();
    void openPlasmaWorkspace();
    void exportPostscript();
    void exportWeb();

    // ExportWeb's writer: the shown subtree's smooth mesh shapes, in the matrix's coordinates, as "p" (path vertices),
    // "c" (colours) and "i" (indices) lines per drawing (fill, then stroke) and "F"; a clipping node's subtree between
    // "M" and "m".
    static void writeWebNode(std::ostream &file, plasma::Node *node, const plasma::Matrix<float> &parentMatrix);

    // ExportPostscript's writers (static: they read the drawings and nodes as friends): a node's meshes and its
    // children, the children through std::for_each, a drawing's shading.
    static void writePostscriptNode(std::ostream &file, plasma::Node *node, const plasma::Matrix<float> &parentMatrix);
    static void writePostscriptDrawing(const plasma::Drawing *drawing, std::ostream &file,
                                       const plasma::Matrix<float> &matrix, bool clip);
    void mergeMeshes();
    void traceMeshes(); // ("unify meshes").
    void strokeToMesh();
    void createInnerEdges();
    void createTexturedRectangle(); // ("create textured rectangle").
    void openTexturedRectangle();
    void selectSameColor();
    void tileHorizontally();
    void tileVertically();
    void cascade();
    // Orders m_sheets as m_sheetsNode stacks their frames (SheetTool).
    void updateSheetOrder();

    // View menu "Page" (the given sheet's page; the current sheet's without one), "Drawing" and "Selection": the
    // sheet's view zooms and pans to fit them.
    void viewPage(Sheet *sheet = 0);
    void viewDrawing();
    void viewSelection();

    // Scales and moves the sheet's root so that the rectangle (in the root's coordinates) fills the sheet's view,
    // centred.
    void fitSheetView(Sheet *sheet, const plasma::Vector<2, float> &minimum, const plasma::Vector<2, float> &maximum);

    // "Save changes in <file>?" before the design sheet's document is replaced or closed; false when the user cancels.
    bool askSaveChanges();

    // Writes the open design sheet to the file on the sheet's thread; a copy keeps the sheet's own path and title.
    void saveDocument(const std::wstring &path, bool copy);

    // Reads the TrueType font into the open design sheet.
    void importFont(const std::wstring &path);

    // Selects ("select node") the shown nodes under the node that use one of the colours.
    void selectNodesWithColors(plasma::Node *node, std::list<plasma::Vector<4, float>> &colors);

    // Waits for the special colour module's worker; resets the tool module's tools.
    void waitForColorWorker();
    void resetTools();

    // "Save Plasma Workspace" (*.plw); with backup, without asking, to "\\Backup\\workspace.plw"
    // (DesignSheet::Commit's autosave).
    void savePlasmaWorkspace(bool backup);

    // Lays out the interface's panels.
    void updateLayout();

    // File menu "Open...": asks for a Plasma file and opens it.
    void openPlasmaFileDialog();
    void openPlasmaFile(const std::wstring &path);
    void activate(std::string id);
    void login(std::string id);
    void validate(std::string id);
    void logout(std::string id);
    void deactivate(std::string id);

    // Logs in again with the kept id ("/Activation/Relogin/?id="), when no request is under way (WndProc).
    void relogin();

    // The online panel's buttons (OnlineModule::Login and::Activate): each shows the progress display and, but for
    // StartLogout, runs script functions on a thread, which send the requests above.
    void startLogin();
    void startLogout();
    void startActivation();
    void startDeactivation();
    void closePopupMenu();
    // Requests the tools' context menus connect to: "Resize Tool Radius..." (several tools); NodeTool's "Choose
    // Parent...", "Send Behind...", "Send in Front of..." and "Set Pivot..." (the tool then waits for the node to be
    // picked).
    void resizeToolRadius();
    void chooseParent();
    void sendBehind();
    void sendInFrontOf();
    void setPivot();

    // The node tool's context menu "Unparent": moves the selected nodes to the root.
    void unparent();
    // The mesh tools' picking (DrawEdgeTool,; DrawFaceTool): the vertex or edge of the selected, shown, unlocked
    // meshes nearest to the point (window coordinates, through each mesh's matrix), with its mesh and node, and its
    // distance (-1 without one for a vertex, 0 for an edge). The face being drawn's own vertices and an open path's
    // closing edge are skipped.
    void findNearestVertex(EditableMesh **mesh, plasma::Node **node, const plasma::Vector<2, float> &point,
                           MeshVertex **vertex, float *distance);
    void findNearestEdge(EditableMesh **mesh, plasma::Node **node, const plasma::Vector<2, float> &point,
                         MeshHalfEdge **edge, float *distance);

    // ToPixels and FromPixels for the current sheet of any kind.
    float fromDisplayUnits(float value);
    float toDisplayUnits(float value);
    // Calls every module's UnitChanged after the unit changed (FormatModule::Update).
    void unitChanged();

    // DesignSheet::RecordModifiedNodes, then Commit.
    void recordModifiedNodes();

    // DesignSheet::SaveSelectedStates of the open design sheet.
    void saveSelectedStates();

    // DesignSheet::BeginModifySelected of the open design sheet.
    void beginModifySelected();
    // Ends BeginModifySelected's modification (DesignSheet: the interfaces flagged m_hasChanges), then Commit and
    // Redraw (MeshModule's and TextModule's edits, VertexTool).
    void endModifySelected();

    // Creates the modules and interface, registers script natives, and starts the update check. Returns false if
    // "icons.plx" cannot be read. Network requests are disabled in the offline build.
    bool init();

    // Scales the interface ("scalable") by the options' GUI zoom; saves the options when asked.
    void setInterfaceZoom(float zoom, bool save); // ("User Interface Zoom").

    // The "updatebutton" handler: downloads the newer release.
    void downloadUpdate();

    // Script natives: copy the server's response into a buffer of the system adapter; read the decrypted sheet from
    // such a buffer.
    void readResponse(int buffer);
    void readDecryptedSheet(int buffer);

    // The client area changed: the sheets' frames scale with it, then the viewport and the interface layout follow.
    void setDimensions(int width, int height);
    void mouseMove(int x, int y);

    // The frame's update (WinMain with the milliseconds since the last frame; PumpMessages with 0); the engine's
    // Advance result.
    bool update(int milliseconds);

    // Refreshes the interfaces of the shown nodes below that fit their shape to their children and changed (Update).
    void refreshFittedShapeInterfaces(plasma::Node *node);
    void draw(); // Draws the frame.

    plasma::GLEngine m_engine;

    // The request to the licence or download server under way (g_httpClient), a ServerRequest; the frame's update
    // handles its answer once it ends.
    ServerRequest m_serverRequest;
    // The downloaded update was installed: WinMain's loop ends and the program starts itself again (the frame update).
    bool m_restartAfterUpdate;
    CRITICAL_SECTION m_lock;
    std::list<Module *> m_modules; // Every module, in creation order.

    // The modules, as Workspace::Init stores them.
    ToolModule *m_toolModule;
    ColorModule *m_colorModule;
    PaletteModule *m_paletteModule;
    SpecialColorModule *m_specialColorModule;
    ColorManagementModule *m_colorManagementModule;
    StatusModule *m_statusModule;
    ToolInfoModule *m_toolInfoModule;
    ProgressModule *m_progressModule;
    PerformanceModule *m_performanceModule;
    ProceduralShapeModule *m_proceduralShapeModule;
    ColorToolModule *m_colorToolModule;
    NodeToolModule *m_nodeToolModule;
    ShadowModule *m_shadowModule;
    ExportPDFModule *m_exportPDFModule;
    VertexModule *m_vertexModule;
    StampModule *m_stampModule;
    ScaleBoxModule *m_scaleBoxModule;
    StrokeWidthModule *m_strokeWidthModule;
    DrawFreehandModule *m_drawFreehandModule;
    TextureModule *m_textureModule; // Its target picks the fill or stroke texture (MeshInterface).
    TextModule *m_textModule;
    WidgetModule *m_widgetModule;
    AnimationModule *m_animationModule; // Cleared by Workspace::Init and refreshed by animation updates.
    SequenceModule *m_sequenceModule;   // Its sequence name receives the mesh keys (MeshInterface).
    ExportModule *m_exportModule;
    FormatModule *m_formatModule;
    // The layer module, told to refresh (UpdatePanel) after "bring forward" (NodeTool). Declared as Module:
    // NodeTool.cpp calls it without LayerModule.h.
    Module *m_layerModule;
    NodeModule *m_nodeModule;
    MeshModule *m_meshModule;
    HistoryModule *m_historyModule;
    ClipboardModule *m_clipboardModule;
    GridModule *m_gridModule;
    ZoomModule *m_zoomModule;
    OptionsModule *m_optionsModule;
    OnlineModule *m_onlineModule;
    AboutModule *m_aboutModule;
    void *m_unused350;            // Cleared by the constructor; no reader found.
    plasma::Node *m_updateButton; // "updatebutton", from the engine's sources.

    // The cursor drawn in the scene (the "cursor:arrow" icon, Init): it follows the pointer unless the options use the
    // hardware cursor (MouseMove, Update); hidden by HideSoftwareCursor.
    plasma::Node *m_softwareCursor;
    plasma::Node *m_cursorArrow; // "cursor:arrow" in icons.plx.

    // The sheet UI template, loaded locally in the offline build. Each Sheet clones it for its frame and panel; the
    // template's "id" text determines the demo flag.
    plasma::Node *m_sheetTemplate;   // An unnamed root node (Init).
    plasma::Node *m_progressDisplay; // Holds "progress-display.plx".

    // The interface's node tree (constructor: Engine::MakeNode with these names).
    plasma::Node *m_guiNode;      // "gui", under the engine's root.
    plasma::Node *m_scalableNode; // "scalable", under "gui".

    // Creates left, right, bottom, bottom2 and top interface nodes, in that order.
    plasma::Node *m_guiTopNode;     // "gui:top", under "scalable".
    plasma::Node *m_guiRightNode;   // "gui:right".
    plasma::Node *m_guiBottomNode;  // "gui:bottom".
    plasma::Node *m_guiBottom2Node; // "gui:bottom2".
    plasma::Node *m_guiLeftNode;    // "gui:left".

    // The "sheet" node under the engine's root (NODE_IGNORES_POINTER); the open sheet's scene hangs below it.
    // DesignSheet::FindSheetNode gives no sheet node for its children.
    plasma::Node *m_sheetsNode;
    // Keyboard shortcuts: virtual-key code to action (DispatchAction), by the modifiers held (KeyPressed).
    std::map<int, int> m_keyActions;
    std::map<int, int> m_shiftKeyActions;
    std::map<int, int> m_controlKeyActions;
    std::map<int, int> m_controlShiftKeyActions;
    std::list<Sheet *> m_sheets; // The open sheets.
    Sheet *m_sheet;              // The current sheet.
    DesignSheet *m_designSheet;
    TextureSheet *m_textureSheet; // The texture sheet, if open.
    float m_toolRadius;           // 20: the tool radius ("Resize Tool Radius...").

    // The pointer in window (scene) coordinates, held on an axis the grid module does not move while dragging
    // (MouseMove); the tools, MeshInterface and TextInterface map it through node matrices.
    plasma::Vector<2, float> m_sceneMousePosition;
    // The pointer's last movement in window pixels (MouseMove).
    float m_pointerDeltaX;
    float m_pointerDeltaY;
    // The drag's snap points relative to m_sheetPointer (CollectSnapOffsets).
    std::list<plasma::Vector<2, float>> m_snapOffsets;
    // The text being edited, if any: it receives the typed characters and the key messages first (KeyPressed).
    TextInterface *m_textInterface;
    // The text under the pointer (within the tool radius; TextTool's FindHoveredTextInterface), cleared by MouseMove:
    // a text tool press edits it (SetTextInterface), TextInterface draws its frame highlighted.
    TextInterface *m_hoveredTextInterface;
    // A redraw is requested. WinMain keeps updating and drawing while set. Input, root transformation changes, and
    // animations request further frames.
    volatile bool m_redrawRequested;
    // A press on the scene: the drag does not start until the pointer moved this far (the options' cursor stickiness);
    // the drag's current position and the one at the previous pointer move (a press sets both to the start).
    float m_cursorStickiness;
    plasma::Vector<2, float> m_dragPosition;
    plasma::Vector<2, float> m_previousDragPosition;

    // The unsnapped pointer in the current sheet root's coordinates, set at a press (PointerPressed,
    // RightButtonPressed) and as the pointer moves: the snap offsets' origin and the tools' rotation pivot.
    plasma::Vector<2, float> m_sheetPointer;
    std::string m_applicationPath;
    // A press of the sheet tool on the current sheet's view (PointerPressed): per axis, how the frame's position and
    // size follow the drag (1, or -1 for the size at the left and top edges; 0 unchanged).
    plasma::Vector<2, float> m_frameMove;
    plasma::Vector<2, float> m_frameResize;
    plasma::Node *m_hoveredNode;
    std::list<plasma::Widget *> m_topPanels;
    std::list<plasma::Widget *> m_bottom2Panels;
    std::list<plasma::Widget *> m_unusedPanels; // Unused list; no panels are added or read.
    std::list<plasma::Widget *> m_bottomPanels;
    std::list<plasma::Widget *> m_rightPanels;
    plasma::ListWidget *m_popupMenuTemplate;        // "popup" in the engine's sources.
    plasma::Widget *m_popupMenuItem;                // The popup menu's item template.
    plasma::Widget *m_popupMenuSeparator;           // The popup menu's separator template.
    std::map<std::wstring, plasma::Node *> m_icons; // The children of "icons.plx" by name.
    plasma::ListWidget *m_popupMenu;                // The open popup menu, if any.
    bool m_currentSheetOnly;                        // ShowCurrentSheetOnly.
    plasma::Node *m_progressWidgetNode;             // A copy of the progress display with a widget.
    bool m_buttonDown;                              // A pointer button went down on the scene.

    // A left press started the tool's drag (PointerPressed); the tool's PointerReleased is due when the button goes
    // up.
    bool m_toolDragStarted;

    // The interface ("gui") node under the pointer (MouseMove, unless the left button drags elsewhere). While set, the
    // scene gets no pointer input (the tools are not called); cleared when the popup menu inside it closes.
    plasma::Node *m_guiPointerNode;
    float m_tabletPressure;  // 1: the pen pressure (WndProc's Tablet::Update; DrawFreehandTool).
    bool m_resizeToolRadius; // "Resize Tool Radius..." requested.

    // Stroke-width resize state, handled like m_resizeToolRadius by MouseMove. Cleared with it, but never set.
    bool m_resizeStrokeWidth;
    bool m_chooseParentRequested;  // "Choose Parent...".
    bool m_sendBehindRequested;    // "Send Behind...".
    bool m_sendInFrontOfRequested; // "Send in Front of...".
    bool m_setPivotRequested;      // "Set Pivot...".
    bool m_unused4CE;              // Cleared by the constructor; no reader found.

    // The texture sheet's frame placement, as fractions of the viewport (CloseTextureSheet keeps it for
    // OpenTextureSheet; Init makes the size a square of half the viewport's height).
    plasma::Vector<2, float> m_textureSheetPosition; // (0.1, 0.1).
    plasma::Vector<2, float> m_textureSheetSize;     // (0.5, 0.5).
    abstr::Machine m_machine;                        // Runs the program's script.
    std::string m_licenseKey;                        // The id Login and Validate sent.

    // Whether the sheet template was loaded as an activation. Cleared by failures, login and deactivation.
    bool m_activated;
    bool m_demo; // True; the demo flag (IsDemo).
};
