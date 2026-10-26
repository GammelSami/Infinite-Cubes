// DesignSheet: the document the editor works on. Besides the scene tree it keeps one NodeInterface (its sheet
// node, NodeInterface.h) per node, the undo and redo histories and its backup file name.
#pragma once
#include <list>
#include <map>
#include <set>
#include <string>
#include "plasma/Vector.h"
#include "Module.h"
#include "plasma/Node.h"
#include "plasma/Widget.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/DiscreteAttribute.h"

// Declare the source-defined interpolation instances before NodeState uses them.
namespace plasma {

template<>
void ContinuousAttribute<float>::interpolate(int before, int start, int end, int after, float startSmoothness,
                                             float endSmoothness, float t, float weight0, float weight1, float weight2,
                                             float weight3);

template<>
void DiscreteAttribute<int>::interpolate(int before, int start, int end, int after, float startSmoothness,
                                         float endSmoothness, float t, float weight0, float weight1, float weight2,
                                         float weight3);
}

#include "NodeInterface.h"

namespace plasma {
class Node;
class Texture;
class Widget;
}

class Operation;
class HistoryState;
class Workspace;
class DesignSheet;
class ShapeInterface;

// The sheet base class: a page shown in the workspace's "sheet:frame" widget, with its file path and the
// "progress" animation shown while it is busy. The constructor gives the layout.
class Sheet {
public:
    // Finds the "sheet:frame" and "sheet:panel" widgets in a copy of the workspace's "sheet" node and builds the view
    // widget, the root and the progress node.
    explicit Sheet(Workspace *workspace);
    virtual ~Sheet();
    // Called by Workspace::ZoomView after zooming. Sheet, DesignSheet and TextureSheet leave it empty.
    virtual void zoomChanged() {}

    // How the last load ended (m_loadResult): the loader threads (Workspace::OpenPlasmaFile, ImportSVG, ImportPDF)
    // store it with their message boxes; Workspace's update reads it once m_loadFinished is set.
    enum LoadResult {
        LOAD_SUCCEEDED = 0,
        LOAD_NEWER_VERSION = 1, // "... created in a newer version of Plasma.".
        LOAD_INVALID_FILE = 2,  // "... no valid Plasma file." or "... a different demo license.".
        LOAD_FAILED = 3         // "Could not open file.".
    };

    // The sheet's name for the window title and the sheet menu: the file name of m_path, or "New design".
    std::wstring getTitle();
    // Workspace::ShowCurrentSheetOnly: keeps the frame's view rectangle and makes the frame cover the window's client
    // area.
    void expandFrame();
    // Workspace::ShowAllSheets: restores the kept view rectangle.
    void restoreFrame();

    // Marks the sheet busy (while it loads or saves; no operations are recorded meanwhile) and plays or stops the
    // "progress" animation.
    void setBusy(bool busy, bool saving);

    // Sets the document's file path.
    void setPath(const std::wstring &path);

    // The view's surface clears to the page colour.
    void applyBackgroundColor();

    // Shades the view outside the page while the format panel is shown (InterfaceViewWidget's rendering calls it).
    void drawPageBorders();

    // Draws the grid and, along the top and left edges, the rulers in the page's unit (InterfaceViewWidget's rendering
    // calls it).
    void drawGrid();

    // Draws the overlays after the scene (InterfaceViewWidget's rendering calls it last): the progress animation
    // centred while busy, the frame's edge zones under the pointer with the sheet tool, the title and the status lines
    // (pixel preview, CMYK preview, special colouring).
    void renderOverlays();
    plasma::Engine::Page *getPage() { return &m_page; }
    plasma::Widget *getFrame() const { return m_frame; }
    plasma::Widget *getPanel() const { return m_panel; }
    float getZoomScale() const { return m_zoomScale; }

    // Waits for the sheet's worker thread to finish.
    void waitForThread();

    plasma::Engine::Page m_page; // PageOperation changes it.
    float m_gridSize;            // 10; the grid module's spacing.
    int m_loadResult;            // LoadResult; LOAD_SUCCEEDED when a load starts.

    // Set when the sheet was made for a dropped or passed file (WndProc's WM_DROPFILES, WM_COPYDATA): if the load
    // fails the sheet closes again.
    bool m_closeIfLoadFails;
    Workspace *m_workspace;
    HANDLE m_thread; // The worker thread (StartGLThread; WaitForThread).

    // Set while the sheet is busy (SetBusy); no operations are recorded during loading or saving.
    volatile bool m_busy;
    // Set by SetBusy to distinguish saving ("Saving <title>...") from loading ("Loading <title>...").
    volatile bool m_saving;

    // Set by the loader thread when it is done; Workspace's update then ends the busy state, applies the result and
    // clears it.
    bool m_loadFinished;
    plasma::Node *m_root;         // The document's scene tree.
    plasma::Node *m_progressNode; // Plays "progress" while busy.

    // The sheet's frame widget ("sheet:frame"); its node's shape plays the "activate" and "deactivate" sequences
    // (Workspace::SetSheet).
    plasma::Widget *m_frame;
    plasma::Widget *m_panel; // "sheet:panel".

    // The view widget the sheet renders into.
    plasma::Widget *m_viewWidget;
    plasma::Vector<2, float> m_savedFramePosition; // ExpandFrame keeps the frame's view rectangle.
    plasma::Vector<2, float> m_savedFrameSize;
    plasma::Node *getRoot() const { return m_root; }
    plasma::Matrix<float> getNodeMatrix(plasma::Node *node) const
    {
        plasma::Matrix<float> matrix = m_root->m_inverseMatrix;
        matrix *= node->m_matrix;
        return matrix;
    }

    float m_zoomScale;   // 1: the root scale shown as 100% zoom (ZoomModule, absolute ZoomView).
    std::wstring m_path; // The document's file.
    bool m_modified;     // Commit sets it when an operation changes the document.
};

// The texture sheet (TextureSheet): a separate view of texture coordinates.
class TextureSheet : public Sheet {
public:
    explicit TextureSheet(Workspace *workspace);
    virtual ~TextureSheet();

    plasma::Matrix<float> getMatrix();
};

// RenderWidget is the widget a sheet renders into.
class RenderWidget : public plasma::Widget {
public:
    RenderWidget(Workspace *workspace, plasma::Node *node);
    virtual void surfaceRendered();
    Workspace *m_workspace;
};

// InterfaceViewWidget, named "interfaceview", renders the design sheet's grid, scene and tool overlays.
class InterfaceViewWidget : public plasma::Widget {
public:
    InterfaceViewWidget(plasma::Engine *engine, DesignSheet *sheet, plasma::Node *node);
    virtual ~InterfaceViewWidget();
    // Renders the sheet: the grid, the page borders, the scene, the tools' radius circles and the sheet's overlays.
    virtual void draw();
    // The node's origin and the viewport's far corner in the node's space.
    virtual plasma::Vector<2, float> getMinimum();
    virtual plasma::Vector<2, float> getMaximum();

    DesignSheet *m_sheet;
};

class DesignSheet : public Sheet {
public:
    // Makes the view widget and the deleted nodes' parent and places the frame.
    explicit DesignSheet(Workspace *workspace);
    virtual ~DesignSheet();

    plasma::Node *getEditRoot() const { return m_editRoot; }
    plasma::Node *getCurrentNode() const { return m_currentNode; }
    plasma::Node *getDeletedNodes() const { return m_deletedNodes; }
    std::wstring getBackupFileName() const;

    // The first sheet node, or the end while the sheet is busy.
    std::map<plasma::Node *, NodeInterface *>::iterator sheetNodesBegin()
    {
        if (isBusy())
            return m_sheetNodes.end();
        return m_sheetNodes.begin();
    }

    // Makes the last selected shown node of the subtree the current node (without a node: of the whole tree, starting
    // over).
    void updateCurrentNode(plasma::Node *node);

    // Creates the node's sheet node, replacing an existing one.
    NodeInterface *createSheetNode(plasma::Node *node);

    // Creates sheet nodes for the subtree; internal children (IsInternal) are skipped.
    NodeInterface *createSheetNodes(plasma::Node *node);
    void deleteSheetNode(NodeInterface *sheetNode);
    void deleteSheetNodes();
    // Resets the sheet to an empty design (New, Open and the imports): deletes the sheet nodes and the undo and redo
    // histories.
    void clear();

    // Rebuilds every sheet node from the root.
    void rebuildSheetNodes();
    NodeInterface *findSheetNode(plasma::Node *node);

    void addOperation(Operation *operation);

    // Closes the current history state: pushes it on the undo stack, clears the redo stack and autosaves every
    // "backupInterval" commits.
    void commit();
    bool hasPendingOperations();

    // Reverts the last user action (re-applies the last reverted one) and restores the current node it recorded.
    void undo();
    void redo();

    // Deletes the redo history.
    void clearRedo();

    // Deletes the undo and redo histories (the edit menu's "Clear History"), the sheet nodes of deleted nodes and the
    // deleted nodes themselves.
    void clearHistory();
    bool isBusy() const;

    void getSelectedNodes(std::list<plasma::Node *> &nodes);
    // Saves the state of every shown selected sheet node with an interface (NodeInterface::SaveState) before an edit.
    void saveSelectedStates();
    // Records "modify node" for every shown sheet node whose modification is pending
    // (NodeInterface::RecordModification) and clears the flag.
    void recordModifiedNodes();
    int countSelectedNodes();

    // For each shown sheet node whose shape interface has changes (an edit or a selection change, m_hasChanges): ends
    // the interface's modification and clears the flag.
    void endModifyChanged();

    // Begins the modification of the interface of every shown selected sheet node that has one.
    void beginModifySelected();

    // After the mesh tools delete elements: calls Rebuild on the interfaces of the shown nodes whose shapes are shared
    // with a selected node (used by more than one shown node), except the drawing tools' interfaces.
    void rebuildSharedInterfaces();
    // Calls Rebuild on the interfaces of the other shown nodes that use the source interface's shape, when more than
    // one shown node uses it (MeshInterface calls it after restoring a state).
    void rebuildShapeSharers(ShapeInterface *source);
    // Deletes the selected nodes of the node's shown subtree (the root's without one; ClipboardModule::Cut, NodeTool)
    // and, from the root, records "deselect node" for the selected sheet nodes left; returns whether a node was
    // deleted.
    bool deleteSelectedNodes(plasma::Node *node);

    // Makes the node the root of editing ("set root"), recorded as an operation when asked.
    void setEditRoot(plasma::Node *root, bool record);

    // Moves the node under the deleted nodes and hides it ("delete node").
    void deleteNode(plasma::Node *node);

    plasma::Texture *m_texture; // The texture module's selected texture (TextureModule::SelectTexture, Delete).

    // ExportModule::Export: the extension last exported, and the file names last used for the drawing, the page and
    // each selection.
    std::wstring m_exportFormat;
    std::wstring m_drawingExportPath;
    std::wstring m_pageExportPath;
    std::map<std::list<plasma::Node *>, std::wstring> m_selectionExportPaths;
    // A level of detail set during rendering: 6 normally, or 3, 2 or 1 when more than 50, 100 or 200 nodes are
    // selected. It is not initialized by the constructor and is otherwise unused.
    int m_detailLevel;
    std::map<plasma::Node *, NodeInterface *> m_sheetNodes;
    HistoryState *m_history; // The operations of the current user action.
    std::list<HistoryState *> m_undoStack;
    std::list<HistoryState *> m_redoStack;
    unsigned char m_unused11C[0x120 - 0x11c]; // Unused.
    plasma::Node *m_editRoot;                 // The root of editing ("set root"); the root at first.
    plasma::Node *m_currentNode;
    plasma::Node *m_deletedNodes;  // Parent of deleted nodes ("delete node"); made by the constructor.
    std::wstring m_backupFileName; // "backup_<n>.plg" under Backup\ (Commit), kept in a backup workspace.
    int m_backupCounter;           // Commits since the last autosave (Commit).
};

inline bool DesignSheet::isBusy() const
{
    return m_busy;
}

inline DesignSheet *Workspace::getOpenDesignSheet()
{
    return m_designSheet && m_designSheet->isBusy() ? 0 : m_designSheet;
}

inline Sheet *Workspace::getOpenSheet()
{
    return m_sheet && m_sheet->m_busy ? 0 : m_sheet;
}
