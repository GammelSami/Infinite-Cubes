#include <algorithm>
#include <sstream>
#include "DesignSheet.h"
#include "FormatModule.h"
#include "HistoryState.h"
#include "OptionsModule.h"
#include "Operations.h"
#include "plasma/Node.h"
#include "plasma/PLXStreamReader.h"
#include "plasma/Shape.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/Texture.h"
#include "plasma/Transformation.h"
#include "ShapeInterface.h"
#include "MeshInterface.h"
#include "ToolModule.h"
#include <set>

using plasma::Node;

typedef std::map<Node *, NodeInterface *> SheetNodeMap;

DesignSheet::DesignSheet(Workspace *workspace)
    : Sheet(workspace),
      m_texture(0),
      m_currentNode(0),
      m_backupCounter(0)
{
    Node *viewNode = workspace->m_engine.createNode(0, 0, 0, m_panel->getNode());
    (new InterfaceViewWidget(&workspace->m_engine, this, viewNode))->moveTo(0.0f, 0.0f, true);
    m_deletedNodes = workspace->m_engine.createNode(0, 0, 0, 0);
    m_editRoot = m_root;
    m_history = new HistoryState(this);

    // The frame covers three quarters of the window's width and 80% of its height, centred, or 50 units below and
    // right of the last design sheet's frame.
    float width = (float)workspace->m_engine.m_viewportWidth;
    float frameWidth = 0.75 * width;
    float height = (float)workspace->m_engine.m_viewportHeight;
    float frameHeight = (double)0.8f * height;
    plasma::Vector<2, float> position((width - frameWidth) * 0.5, 0.5 * (height - frameHeight));
    std::for_each(workspace->m_sheets.begin(), workspace->m_sheets.end(), [&position](Sheet *sheet) {
        if (DesignSheet *designSheet = dynamic_cast<DesignSheet *>(sheet)) {
            // Assigned from the returned position.
            plasma::Vector<2, float> framePosition;
            framePosition = designSheet->m_frame->getViewPosition();
            framePosition.m_elements[0] += 50.0;
            framePosition.m_elements[1] += 50.0;
            position = framePosition;
        }
    });
    m_frame->resizeTo((float)(int)frameWidth, (float)(int)frameHeight);

    // At whole units.
    plasma::Vector<2, int> wholePosition;
    for (int i = 0; i < 2; i++)
        wholePosition.m_elements[i] = (int)position.m_elements[i];
    m_frame->setViewPosition(plasma::Vector<2, float>(wholePosition), true);
    rebuildSheetNodes();
}

DesignSheet::~DesignSheet()
{
    m_workspace->m_engine.destroyNode(m_deletedNodes);
    for (SheetNodeMap::iterator it = m_sheetNodes.begin(); it != m_sheetNodes.end(); ++it)
        delete it->second;
    for (std::list<HistoryState *>::iterator it = m_undoStack.begin(); it != m_undoStack.end(); ++it)
        delete *it;
    for (std::list<HistoryState *>::iterator it = m_redoStack.begin(); it != m_redoStack.end(); ++it)
        delete *it;
    delete m_history;
}

std::wstring DesignSheet::getBackupFileName() const
{
    return m_backupFileName;
}

void DesignSheet::clear()
{
    for (SheetNodeMap::iterator it = m_sheetNodes.begin(); it != m_sheetNodes.end(); ++it)
        delete it->second;
    m_sheetNodes.clear();
    for (std::list<HistoryState *>::iterator it = m_undoStack.begin(); it != m_undoStack.end(); ++it)
        delete *it;
    m_undoStack.clear();
    for (std::list<HistoryState *>::iterator it = m_redoStack.begin(); it != m_redoStack.end(); ++it)
        delete *it;
    m_redoStack.clear();
    delete m_history;
    m_deletedNodes->destroyChildren();
    m_editRoot = m_root;
    m_history = new HistoryState(this);
    m_currentNode = 0;
    m_modified = false;
    m_path = L"";
    m_root->destroyChildren();
    m_root->m_parent->m_transformation->m_changed = true;
    m_gridSize = 10.0f;
    m_page.unit = UNIT_PX;
    m_page.size.m_elements[0] = 1000.0f;
    m_page.size.m_elements[1] = 1000.0f;
    m_page.size.m_elements[2] = 72.0f;
    m_page.color.m_elements[0] = 1.0f;
    m_page.color.m_elements[1] = 1.0f;
    m_page.color.m_elements[2] = 1.0f;
    m_page.color.m_elements[3] = 0.0f;
    applyBackgroundColor();
    m_root->m_transformation->m_translation.value() = plasma::Vector<2, float>(0.0f, 0.0f);
    m_root->m_transformation->m_deformation.value().identity();
    m_root->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    rebuildSheetNodes();
}

void DesignSheet::updateCurrentNode(Node *node)
{
    if (isBusy())
        return;
    if (!node) {
        node = m_root;
        m_currentNode = 0;
    }
    if (!node->isShown())
        return;
    if (findSheetNode(node) && findSheetNode(node)->m_selected)
        m_currentNode = node;
    for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        updateCurrentNode(*it);
}

NodeInterface *DesignSheet::createSheetNode(Node *node)
{
    SheetNodeMap::iterator it = m_sheetNodes.find(node);
    if (it != m_sheetNodes.end())
        delete it->second;
    NodeInterface *sheetNode = new NodeInterface(this, node);
    m_sheetNodes[node] = sheetNode;
    node->m_userData = sheetNode;
    return sheetNode;
}

NodeInterface *DesignSheet::createSheetNodes(Node *node)
{
    for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        if (!(*it)->isInternal())
            createSheetNodes(*it);
    return createSheetNode(node);
}

void DesignSheet::deleteSheetNode(NodeInterface *sheetNode)
{
    Node *node = sheetNode->m_node;
    SheetNodeMap::iterator it = m_sheetNodes.find(node);
    if (it != m_sheetNodes.end())
        m_sheetNodes.erase(it);
    delete sheetNode;
}

NodeInterface *DesignSheet::findSheetNode(Node *node)
{
    if (node->m_parent == m_workspace->m_sheetsNode)
        return 0;
    return static_cast<NodeInterface *>(node->m_userData);
}

void DesignSheet::rebuildSheetNodes()
{
    deleteSheetNodes();
    createSheetNodes(m_root);
}

void DesignSheet::deleteSheetNodes()
{
    std::list<NodeInterface *> sheetNodes;
    for (SheetNodeMap::iterator it = m_sheetNodes.begin(); it != m_sheetNodes.end(); ++it)
        sheetNodes.push_back(it->second);
    for (std::list<NodeInterface *>::iterator it = sheetNodes.begin(); it != sheetNodes.end(); ++it)
        deleteSheetNode(*it);
}

void DesignSheet::rebuildSharedInterfaces()
{
    // Shapes are kept as void-pointer keys.
    std::set<void *> shapes;
    for (SheetNodeMap::iterator it = m_sheetNodes.begin(); it != m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected && it->first->m_shape &&
            it->first->m_shape->countShownNodes() > 1)
            shapes.insert(static_cast<void *>(it->first->m_shape));
    }

    // The interfaces the drawing tools work with are left alone.
    for (SheetNodeMap::iterator it = m_sheetNodes.begin(); it != m_sheetNodes.end(); ++it)
        if (it->first->isShown() && it->second->m_shapeInterface && shapes.find(it->first->m_shape) != shapes.end() &&
            it->second->m_shapeInterface != m_workspace->m_toolModule->m_drawEdgeTool.m_mesh &&
            it->second->m_shapeInterface != m_workspace->m_toolModule->m_drawFaceTool.m_meshInterface)
            it->second->m_shapeInterface->rebuild();
}

void DesignSheet::rebuildShapeSharers(ShapeInterface *source)
{
    if (source->m_node->m_node->m_shape->countShownNodes() <= 1)
        return;
    for (SheetNodeMap::iterator it = m_sheetNodes.begin(); it != m_sheetNodes.end(); ++it)
        if (it->first->isShown() && it->first->m_shape == source->m_node->m_node->m_shape &&
            it->second->m_shapeInterface != source)
            it->second->m_shapeInterface->rebuild();
}

void DesignSheet::addOperation(Operation *operation)
{
    if (!isBusy())
        m_history->add(operation);
}

void DesignSheet::commit()
{
    if (isBusy() || !hasPendingOperations())
        return;
    m_undoStack.push_back(m_history);
    m_history->m_redoCurrentNode = m_currentNode;
    m_modified = m_modified || m_history->changesDocument();
    m_history = new HistoryState(this);
    m_history->m_undoCurrentNode = m_currentNode;
    clearRedo();

    // The autosave ("backupEnabled", "backupInterval"): every n-th commit the document is written to
    // Backup\backup_N.plg under the application's directory, numbered past the other open sheets' backups.
    if (!m_workspace->getOptionsModule()->m_backupEnabled)
        return;
    if (++m_backupCounter < m_workspace->getOptionsModule()->m_backupInterval)
        return;
    m_backupCounter = 0;
    if (m_backupFileName.empty()) {
        int number = 0;
        m_backupFileName = L"backup_0.plg";
        for (std::list<Sheet *>::iterator it = m_workspace->m_sheets.begin(); it != m_workspace->m_sheets.end(); ++it) {
            if (*it == this)
                continue;
            DesignSheet *sheet = dynamic_cast<DesignSheet *>(*it);
            if (sheet && sheet->getBackupFileName() == m_backupFileName)
                ++number;
            std::wstringstream name;
            name << L"backup_" << number << L".plg";
            m_backupFileName = name.str();
        }
    }
    std::wstringstream path;
    path << widenString(m_workspace->m_applicationPath) << L"\\Backup\\" << m_backupFileName;
    std::wstring backupPath = path.str();

    // Sheet::m_page has the layout of the engine's page settings.
    m_workspace->m_engine.writePlasmaFile(backupPath, m_root, &m_page, 0);
    m_workspace->savePlasmaWorkspace(true);
}

void DesignSheet::clearRedo()
{
    if (isBusy())
        return;
    for (std::list<HistoryState *>::iterator it = m_redoStack.begin(); it != m_redoStack.end(); ++it)
        delete *it;
    m_redoStack.clear();
}

void DesignSheet::clearHistory()
{
    if (isBusy())
        return;
    for (std::list<HistoryState *>::iterator it = m_undoStack.begin(); it != m_undoStack.end(); ++it)
        delete *it;
    for (std::list<HistoryState *>::iterator it = m_redoStack.begin(); it != m_redoStack.end(); ++it)
        delete *it;

    // The deleted (hidden) nodes' sheet nodes go; only the history kept them.
    std::list<NodeInterface *> hidden;
    SheetNodeMap::iterator it;
    if (isBusy())
        it = m_sheetNodes.end();
    else
        it = m_sheetNodes.begin();
    for (; it != m_sheetNodes.end(); ++it)
        if (!it->first->isShown())
            hidden.push_back(it->second);
    for (std::list<NodeInterface *>::iterator node = hidden.begin(); node != hidden.end(); ++node)
        deleteSheetNode(*node);
    if (isBusy())
        it = m_sheetNodes.end();
    else
        it = m_sheetNodes.begin();
    for (; it != m_sheetNodes.end(); ++it)
        if (it->second->m_shapeInterface)
            it->second->m_shapeInterface->discardModifyState();
    m_undoStack.clear();
    m_redoStack.clear();
    m_deletedNodes->destroyChildren();
}

void DesignSheet::undo()
{
    if (isBusy() || m_undoStack.empty())
        return;
    m_workspace->resetTools();
    m_undoStack.back()->undo();
    m_currentNode = m_undoStack.back()->m_undoCurrentNode;
    m_workspace->updateModules();
    m_redoStack.push_back(m_undoStack.back());
    m_undoStack.pop_back();
    m_history->m_undoCurrentNode = m_currentNode;

    // Draw the scene again; refresh the editing interfaces of the selected nodes whose shapes changed.
    m_root->m_transformation->m_changed = true;
    // Starts with the first sheet node, or the end while the sheet is busy.
    SheetNodeMap::iterator it;
    if (isBusy())
        it = m_sheetNodes.end();
    else
        it = m_sheetNodes.begin();
    for (; it != m_sheetNodes.end(); ++it) {
        NodeInterface *sheetNode = it->second;
        if (sheetNode->m_selected) {
            ShapeInterface *shapeInterface = sheetNode->m_shapeInterface;
            if (shapeInterface && it->first->m_shape->m_changed)
                shapeInterface->refresh();
        }
    }
    m_workspace->m_formatModule->update();
    m_workspace->refreshTimeline();
    m_workspace->redraw();
}

void DesignSheet::redo()
{
    if (isBusy() || m_redoStack.empty())
        return;
    m_workspace->resetTools();
    m_redoStack.back()->redo();
    m_currentNode = m_redoStack.back()->m_redoCurrentNode;
    m_workspace->updateModules();
    m_undoStack.push_back(m_redoStack.back());
    m_history->m_undoCurrentNode = m_currentNode;
    m_redoStack.pop_back();

    // Draw the scene again; refresh the editing interfaces of the selected nodes whose shapes changed.
    m_root->m_transformation->m_changed = true;
    // Starts with the first sheet node, or the end while the sheet is busy.
    SheetNodeMap::iterator it;
    if (isBusy())
        it = m_sheetNodes.end();
    else
        it = m_sheetNodes.begin();
    for (; it != m_sheetNodes.end(); ++it) {
        NodeInterface *sheetNode = it->second;
        if (sheetNode->m_selected) {
            ShapeInterface *shapeInterface = sheetNode->m_shapeInterface;
            if (shapeInterface && it->first->m_shape->m_changed)
                shapeInterface->refresh();
        }
    }
    m_workspace->m_formatModule->update();
    m_workspace->refreshTimeline();
    m_workspace->redraw();
}

bool DesignSheet::hasPendingOperations()
{
    if (!m_history)
        return false;
    return !m_history->isEmpty();
}

void DesignSheet::getSelectedNodes(std::list<Node *> &nodes)
{
    nodes.clear();
    if (isBusy())
        return;
    for (SheetNodeMap::iterator it = m_sheetNodes.begin(); it != m_sheetNodes.end(); ++it)
        if (it->second->m_selected)
            nodes.push_back(it->first);
}

void DesignSheet::beginModifySelected()
{
    if (isBusy())
        return;
    for (SheetNodeMap::iterator it = m_sheetNodes.begin(); it != m_sheetNodes.end(); ++it)
        if (it->first->isShown() && it->second->m_selected && it->second->m_shapeInterface)
            it->second->m_shapeInterface->beginModify();
}

void DesignSheet::endModifyChanged()
{
    if (isBusy())
        return;
    for (SheetNodeMap::iterator it = m_sheetNodes.begin(); it != m_sheetNodes.end(); ++it)
        if (it->first->isShown() && it->second->m_shapeInterface && it->second->m_shapeInterface->m_hasChanges) {
            it->second->m_shapeInterface->endModify();
            it->second->m_shapeInterface->m_hasChanges = false;
        }
}

void DesignSheet::saveSelectedStates()
{
    if (isBusy())
        return;
    for (SheetNodeMap::iterator it = m_sheetNodes.begin(); it != m_sheetNodes.end(); ++it)
        if (it->first->isShown() && it->second->m_selected && it->second->m_shapeInterface)
            it->second->saveState();
}

void DesignSheet::recordModifiedNodes()
{
    if (isBusy())
        return;
    for (SheetNodeMap::iterator it = m_sheetNodes.begin(); it != m_sheetNodes.end(); ++it)
        if (it->first->isShown() && it->second->m_modificationPending) {
            it->second->recordModification();
            it->second->m_modificationPending = false;
        }
}

int DesignSheet::countSelectedNodes()
{
    if (isBusy())
        return 0;
    int count = 0;
    for (SheetNodeMap::iterator it = m_sheetNodes.begin(); it != m_sheetNodes.end(); ++it)
        if (it->second->m_selected && it->first->isShown())
            ++count;
    return count;
}

void DesignSheet::setEditRoot(Node *root, bool record)
{
    if (record)
        addOperation(new NodeRootOperation(this, "set root", m_editRoot, root));
    m_editRoot = root;
}

void DesignSheet::deleteNode(Node *node)
{
    addOperation(new DeleteNodeOperation(m_workspace->getOpenDesignSheet(), "delete node", node, node->getParent(),
                                         node->getNextSibling()));
    if (node->isAncestorOf(m_editRoot))
        setEditRoot(m_root, true);
    node->setParent(m_deletedNodes);
    node->setShownRecursively(false);
}

bool DesignSheet::deleteSelectedNodes(Node *node)
{
    if (isBusy())
        return false;
    if (!node)
        node = m_root;
    if (!node->isShown() || node->isInternal())
        return false;
    if (findSheetNode(node)->m_selected) {
        deleteNode(node);
        return true;
    }
    bool deleted = false;
    std::list<Node *> children = node->getChildren();
    for (std::list<Node *>::iterator it = children.begin(); it != children.end(); ++it)
        deleted = deleteSelectedNodes(*it) || deleted;
    if (node == m_root && deleted) {
        // The deleted nodes' selection is cleared ("deselect node").
        SheetNodeMap::iterator it;
        if (isBusy())
            it = m_sheetNodes.end();
        else
            it = m_sheetNodes.begin();
        for (; it != m_sheetNodes.end(); ++it) {
            if (it->second->m_selected) {
                NodeState before;
                it->second->saveState(before);
                it->second->setSelected(false);
                NodeState after;
                it->second->saveState(after);
                addOperation(
                    new NodeOperation(m_workspace->getOpenDesignSheet(), "deselect node", it->second, before, after));
            }
        }
        m_currentNode = 0;
        m_workspace->updateModules();
        m_workspace->redraw();
        m_workspace->m_engine.resetPointer();
        m_workspace->refreshPointer();
        m_root->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
    return deleted;
}

// Operations defined by the design sheet.
ApplyTextureOperation::ApplyTextureOperation(DesignSheet *sheet, const std::string &name,
                                             plasma::SmoothMeshShape *shape, plasma::Texture *oldTexture,
                                             float oldOpacity, float oldBrightness, float oldContrast,
                                             float oldSaturation, float oldStretch, plasma::Texture *newTexture,
                                             float newOpacity, float newBrightness, float newContrast,
                                             float newSaturation, float newStretch, bool stroke)
    : Operation(sheet, name, true),
      m_shape(shape),
      m_oldOpacity(oldOpacity),
      m_newOpacity(newOpacity),
      m_oldBrightness(oldBrightness),
      m_newBrightness(newBrightness),
      m_oldContrast(oldContrast),
      m_newContrast(newContrast),
      m_oldSaturation(oldSaturation),
      m_newSaturation(newSaturation),
      m_oldStretch(oldStretch),
      m_newStretch(newStretch),
      m_oldTexture(oldTexture),
      m_newTexture(newTexture),
      m_stroke(stroke)
{}

void ApplyTextureOperation::execute()
{
    if (m_newTexture) {
        if (m_stroke)
            m_shape->m_strokeTexture.value() = m_newTexture->m_id;
        else
            m_shape->m_texture.value() = m_newTexture->m_id;
    } else if (m_stroke) {
        m_shape->m_strokeTexture.value() = -1;
    } else {
        m_shape->m_texture.value() = -1;
    }
    if (m_stroke) {
        m_shape->m_strokeTextureOpacity.value() = m_newOpacity;
        m_shape->m_strokeTextureBrightness.value() = m_newBrightness;
        m_shape->m_strokeTextureContrast.value() = m_newContrast;
        m_shape->m_strokeTextureSaturation.value() = m_newSaturation;
    } else {
        m_shape->m_textureOpacity.value() = m_newOpacity;
        m_shape->m_textureBrightness.value() = m_newBrightness;
        m_shape->m_textureContrast.value() = m_newContrast;
        m_shape->m_textureSaturation.value() = m_newSaturation;
    }
    m_shape->m_strokeTextureStretch.value() = m_newStretch;
    m_shape->lock();
    m_shape->updateDrawingTextures();
    m_shape->unlock();
}

void ApplyTextureOperation::undo()
{
    if (m_oldTexture) {
        if (m_stroke)
            m_shape->m_strokeTexture.value() = m_oldTexture->m_id;
        else
            m_shape->m_texture.value() = m_oldTexture->m_id;
    } else if (m_stroke) {
        m_shape->m_strokeTexture.value() = -1;
    } else {
        m_shape->m_texture.value() = -1;
    }
    if (m_stroke) {
        m_shape->m_strokeTextureOpacity.value() = m_oldOpacity;
        m_shape->m_strokeTextureBrightness.value() = m_oldBrightness;
        m_shape->m_strokeTextureContrast.value() = m_oldContrast;
        m_shape->m_strokeTextureSaturation.value() = m_oldSaturation;
    } else {
        m_shape->m_textureOpacity.value() = m_oldOpacity;
        m_shape->m_textureBrightness.value() = m_oldBrightness;
        m_shape->m_textureContrast.value() = m_oldContrast;
        m_shape->m_textureSaturation.value() = m_oldSaturation;
    }
    m_shape->m_strokeTextureStretch.value() = m_oldStretch;
    m_shape->lock();
    m_shape->updateDrawingTextures();
    m_shape->unlock();
}

void ApplyTextureOperation::forgetTexture(plasma::Texture *texture)
{
    if (texture == m_oldTexture)
        m_oldTexture = 0;
    if (texture == m_newTexture)
        m_newTexture = 0;
}

InstanceOperation::InstanceOperation(DesignSheet *sheet, const std::string &name, NodeInterface *node,
                                     plasma::Shape *oldShape, plasma::Transformation *oldTransformation,
                                     plasma::Display *oldDisplay, plasma::Shape *newShape,
                                     plasma::Transformation *newTransformation, plasma::Display *newDisplay)
    : Operation(sheet, name, true),
      m_oldShape(oldShape),
      m_oldTransformation(oldTransformation),
      m_oldDisplay(oldDisplay),
      m_newShape(newShape),
      m_newTransformation(newTransformation),
      m_newDisplay(newDisplay),
      m_node(node)
{
    // Parks the new parts on a node under the sheet's deleted nodes.
    sheet->m_workspace->m_engine.createNode(m_newTransformation, m_newShape, m_newDisplay, sheet->m_deletedNodes);
}

void InstanceOperation::execute()
{
    m_node->m_node->setShape(m_newShape);
    m_node->m_node->setTransformation(m_newTransformation);
    m_node->m_node->setDisplay(m_newDisplay);
    m_node->createShapeInterface();
}

void InstanceOperation::undo()
{
    m_node->m_node->setShape(m_oldShape);
    m_node->m_node->setTransformation(m_oldTransformation);
    m_node->m_node->setDisplay(m_oldDisplay);
    m_node->createShapeInterface();
}

MeshEdgeSelectionOperation::MeshEdgeSelectionOperation(DesignSheet *sheet, const std::string &name, MeshInterface *mesh,
                                                       const std::vector<bool> &before, const std::vector<bool> &after)
    : Operation(sheet, name, false),
      m_mesh(mesh),
      m_before(before),
      m_after(after)
{}

void MeshEdgeSelectionOperation::execute()
{
    m_mesh->applyEdgeSelection();
    m_mesh->refresh();
}

void MeshEdgeSelectionOperation::undo()
{
    m_mesh->applyEdgeSelection();
    m_mesh->refresh();
}

MeshFaceSelectionOperation::MeshFaceSelectionOperation(DesignSheet *sheet, const std::string &name, MeshInterface *mesh,
                                                       const std::vector<bool> &before, const std::vector<bool> &after)
    : Operation(sheet, name, false),
      m_mesh(mesh),
      m_before(before),
      m_after(after)
{}

void MeshFaceSelectionOperation::execute()
{
    m_mesh->applyFaceSelection();
    m_mesh->refresh();
}

void MeshFaceSelectionOperation::undo()
{
    m_mesh->applyFaceSelection();
    m_mesh->refresh();
}

MeshVertexSelectionOperation::MeshVertexSelectionOperation(DesignSheet *sheet, const std::string &name,
                                                           MeshInterface *mesh, const std::vector<bool> &before,
                                                           const std::vector<bool> &after)
    : Operation(sheet, name, false),
      m_mesh(mesh),
      m_before(before),
      m_after(after)
{}

void MeshVertexSelectionOperation::execute()
{
    int index = 0;
    for (HalfEdgeMesh::VertexIterator it = m_mesh->m_mesh.beginVertices(); it != m_mesh->m_mesh.endVertices(); ++it)
        m_mesh->setVertexSelected(*it, m_after[index++]);
}

void MeshVertexSelectionOperation::undo()
{
    int index = 0;
    for (HalfEdgeMesh::VertexIterator it = m_mesh->m_mesh.beginVertices(); it != m_mesh->m_mesh.endVertices(); ++it)
        m_mesh->setVertexSelected(*it, m_before[index++]);
}

NodeSelectionOperation::NodeSelectionOperation(DesignSheet *sheet, const std::string &name, NodeInterface *node,
                                               bool select)
    : Operation(sheet, name, false),
      m_select(select),
      m_node(node)
{}

void NodeSelectionOperation::execute()
{
    m_node->setSelected(m_select);
}

void NodeSelectionOperation::undo()
{
    m_node->setSelected(!m_select);
}

PageOperation::PageOperation(DesignSheet *sheet, const std::string &name, const plasma::Engine::Page &before,
                             const plasma::Engine::Page &after)
    : Operation(sheet, name, true)
{
    m_before = before;
    m_after = after;
}

void PageOperation::execute()
{
    m_sheet->m_page.size.m_elements[0] = m_after.size.m_elements[0];
    m_sheet->m_page.size.m_elements[1] = m_after.size.m_elements[1];
    m_sheet->m_page.unit = m_after.unit;
    m_sheet->m_page.color = m_after.color;
    m_sheet->applyBackgroundColor();
}

void PageOperation::undo()
{
    m_sheet->m_page.size.m_elements[0] = m_before.size.m_elements[0];
    m_sheet->m_page.size.m_elements[1] = m_before.size.m_elements[1];
    m_sheet->m_page.unit = m_before.unit;
    m_sheet->m_page.color = m_before.color;
    m_sheet->applyBackgroundColor();
}

namespace plasma {

// Cubic attribute interpolation uses quarter-chord and quarter-neighbor-difference tangents; retain the
// float/double operand precision.
template<>
void ContinuousAttribute<float>::interpolate(int before, int start, int end, int after, float startSmoothness,
                                             float endSmoothness, float t, float weight0, float weight1, float weight2,
                                             float weight3)
{
    const float &startValue = m_values[start];
    const float &endValue = m_values[end];
    float chord = (endValue - startValue) * (double)0.25f;
    float endTangent = (m_values[after] - startValue) * (double)0.25f;
    float endControl = endValue - ((1.0 - endSmoothness) * chord + endTangent * endSmoothness);
    float startTangent = (endValue - m_values[before]) * (double)0.25f;
    float startControl = startTangent * startSmoothness + chord * (1.0 - startSmoothness) + startValue;
    m_values[m_current] = startValue * weight0 + startControl * weight1 + endControl * weight2 + endValue * weight3;
}

template<>
void DiscreteAttribute<int>::interpolate(int before, int start, int end, int after, float startSmoothness,
                                         float endSmoothness, float t, float weight0, float weight1, float weight2,
                                         float weight3)
{
    if (t < 1.0f)
        m_values[m_current] = m_values[start];
    else
        m_values[m_current] = m_values[end];
}

}
