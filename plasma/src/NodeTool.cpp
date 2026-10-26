#include "NodeTool.h"
#include <windows.h>
#include "DesignSheet.h"
#include "Operations.h"
#include "CreateNodeOperation.h"
#include "ShapeInterface.h"
#include "MeshInterface.h"
#include "plasma/SmoothMeshShape.h"
#include "NodeToolModule.h"
#include "ToolModule.h"
#include "PerformanceModule.h"
#include "NodeModule.h"
#include "ShadowModule.h"
#include "TextModule.h"
#include "SequenceModule.h"
#include "plasma/Display.h"
#include "plasma/Widget.h"
#include "plasma/ListWidget.h"
#include "ClipboardModule.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/FontEngine.h"
#include <GL/glew.h>

using plasma::Node;

using plasma::NODE_HIDDEN;
using plasma::NODE_INTERNAL;
using plasma::NODE_LOCKED;

static const int BoundaryMeshSubdivisionCount = 1;

NodeTool::NodeTool(int type)
    : BoundingBoxTool(type)
{}

void NodeTool::deselectSheetNode(NodeInterface *node)
{
    if (node->m_selected) {
        node->setSelected(false);
        node->m_sheet->addOperation(new NodeSelectionOperation(node->m_sheet, "deselect node", node, false));
    }
}

void NodeTool::deleteSelected()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    m_workspace->getOpenDesignSheet()->deleteSelectedNodes(0);
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::pointerDoubleClicked()
{
    if (!m_workspace->getOpenDesignSheet() || isOverHandle())
        return;
    if (m_workspace->getHoveredSheetNode()) {
        m_workspace->getOpenDesignSheet()->setEditRoot(m_workspace->getHoveredNode(), true);
    } else {
        Node *root = m_workspace->getOpenDesignSheet()->m_root;
        m_workspace->getOpenDesignSheet()->setEditRoot(root, false);
    }
}

// Pressing the left button: on a handle of the scale box, starts dragging it; otherwise selects the hovered node
// (deselecting the others unless Ctrl or Shift is held or it is already selected), or deselects it with Ctrl. In
// the texture sheet the selected meshes keep their texture transforms; a box selection starts with Ctrl, or in
// select mode without Alt.
void NodeTool::pointerPressed()
{
    if (!m_workspace->getOpenSheet())
        return;
    if (m_workspace->getNodeToolModule()->m_scaleBox)
        beginHandleDrag();
    if (m_workspace->getDesignSheet() && !m_workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT) &&
        !isOverHandle()) {
        if (!m_workspace->getNodeToolModule()->m_scaleBox || findHandle() == NoHandle) {
            if (!m_workspace->m_engine.isKeyDown(VK_CONTROL) && !m_workspace->m_engine.isKeyDown(VK_SHIFT) &&
                (!m_workspace->getHoveredSheetNode() || !m_workspace->getHoveredSheetNode()->m_selected)) {
                m_workspace->forEachUnlockedSheetNode(deselectSheetNode);
                m_workspace->refreshTimeline();
                m_workspace->getDesignSheet()->updateCurrentNode(0);
                m_workspace->updateModules();
            }
        }
        if (m_workspace->getHoveredSheetNode()) {
            if (!m_workspace->m_engine.isKeyDown(VK_CONTROL)) {
                if (!m_workspace->getHoveredSheetNode()->m_selected) {
                    m_workspace->getHoveredSheetNode()->setSelected(true);
                    m_workspace->getDesignSheet()->addOperation(new NodeSelectionOperation(
                        m_workspace->getDesignSheet(), "select node", m_workspace->getHoveredSheetNode(), true));
                }
                m_workspace->getDesignSheet()->m_currentNode = m_workspace->getHoveredNode();
                m_workspace->updateModules();
            }
            if (m_workspace->m_engine.isKeyDown(VK_CONTROL) && !m_workspace->m_engine.isKeyDown(VK_SHIFT) &&
                m_workspace->getHoveredSheetNode()->m_selected) {
                m_workspace->getHoveredSheetNode()->setSelected(false);
                m_workspace->getDesignSheet()->addOperation(new NodeSelectionOperation(
                    m_workspace->getDesignSheet(), "deselect node", m_workspace->getHoveredSheetNode(), false));
                m_workspace->getDesignSheet()->updateCurrentNode(0);
                m_workspace->updateModules();
            }
        }
    }
    m_workspace->saveSelectedStates();
    if (m_workspace->getTextureSheet() && m_workspace->getOpenDesignSheet()) {
        for (std::map<Node *, NodeInterface *>::const_iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface &&
                it->second->m_shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(it->second->m_shapeInterface);
                mesh->storeTextureTransform();
                mesh->beginModify();
            }
        }
    }
    if (m_workspace->getDesignSheet()) {
        if ((m_workspace->m_engine.isKeyDown(VK_CONTROL) && !m_workspace->m_engine.isKeyDown(VK_SHIFT)) ||
            (m_workspace->getNodeToolModule()->m_editMode == EDIT_SELECT &&
             !m_workspace->m_engine.isKeyDown(VK_MENU))) {
            if (findHandle() == NoHandle) {
                m_selectionMode = m_workspace->getNodeToolModule()->m_selectionMode;
                beginSelection();
            }
        }
    }
    updateBox();
    isOverHandle();
    if (m_workspace->m_animationModule)
        m_workspace->m_animationModule->refresh(false);
}

bool NodeTool::selectNodesInBox(Node *node, bool select)
{
    if (!node->isShown())
        return false;
    bool changed = false;
    if (!node->isLocked()) {
        NodeInterface *sheetNode = m_workspace->getOpenDesignSheet()->findSheetNode(node);
        if (select != sheetNode->m_selected) {
            ShapeInterface *shapeInterface = sheetNode->m_shapeInterface;
            if (shapeInterface && shapeInterface->isInsideBox(this)) {
                sheetNode->setSelected(select);
                m_workspace->getOpenDesignSheet()->addOperation(
                    new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "select node", sheetNode, select));
                changed = true;
            }
        }
    }
    for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        if (selectNodesInBox(*it, select))
            changed = true;
    return changed;
}

void NodeTool::pointerReleased()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    if (m_selectionMode != SELECTION_NONE && m_workspace->getDesignSheet()) {
        endSelection();
        Workspace *workspace = m_workspace;

        // Holding Control in select mode deselects the nodes in the box.
        bool select = true;
        if (workspace->m_nodeToolModule->m_editMode == EDIT_SELECT &&
            workspace->m_engine.m_pressedKeys.find(VK_CONTROL) != workspace->m_engine.m_pressedKeys.end())
            select = false;
        Node *root = workspace->getDesignSheet()->m_root;
        if (selectNodesInBox(root, select)) {
            m_workspace->getDesignSheet()->updateCurrentNode(0);
            m_workspace->updateModules();
            m_workspace->refreshTimeline();
        }
        cancelSelection();
        updateBox();
    }
    m_workspace->getOpenDesignSheet()->endModifyChanged();
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->getOpenDesignSheet()->commit();
}

// Dragging with the left button: updates the box selection, or moves (rotates, with IsRotating) the selected nodes
// or, in the texture sheet, the selected meshes' textures; dragging a handle scales them about the anchor
// (proportionally with Ctrl). Each node or shape changes once.
void NodeTool::pointerMoved(int, int)
{
    if (!(m_workspace->m_engine.m_buttons & (1 << plasma::MOUSE_BUTTON_LEFT)))
        return;
    std::set<plasma::Keyable *> transformations;
    std::set<plasma::Shape *> shapes;
    if (m_selectionMode == SELECTION_NONE) {
        if (m_dragHandle != NoHandle) {
            plasma::Node *root = m_workspace->getOpenSheet()->m_root;
            const plasma::Vector<2, float> &movement = m_workspace->getDragMovement();
            const plasma::Matrix<float> &inverse = root->getInverseMatrix();
            m_dragPosition += plasma::Vector<2, float>(inverse[0] * movement[0] + inverse[4] * movement[1],
                                                       inverse[1] * movement[0] + inverse[5] * movement[1]);
            plasma::Vector<2, float> from = m_dragStart - m_dragAnchor;
            plasma::Vector<2, float> to = m_dragPosition - m_dragAnchor;
            if (from.m_elements[0] * from.m_elements[0] < (double)1.0e-11f) {
                from.m_elements[0] = 1.0f;
                to.m_elements[0] = 1.0f;
            }
            if (from.m_elements[1] * from.m_elements[1] < (double)1.0e-11f) {
                from.m_elements[1] = 1.0f;
                to.m_elements[1] = 1.0f;
            }
            if (m_workspace->m_engine.isKeyDown(VK_CONTROL)) {
                // Proportional: the larger of the two ratios.
                to = from * max(to.m_elements[0] / from.m_elements[0], to.m_elements[1] / from.m_elements[1]);
            }
            if (m_workspace->getTextureSheet()) {
                plasma::Vector<2, float> scale(to.m_elements[0] / from.m_elements[0],
                                               to.m_elements[1] / from.m_elements[1]);
                scaleTextureCoordinates(shapes, m_workspace->getOpenSheet()->m_root->getMatrix() * m_dragAnchor, scale);
                updateBox();
            } else {
                if (m_workspace->getDesignSheet()) {
                    for (std::map<Node *, NodeInterface *>::const_iterator it =
                             m_workspace->getDesignSheet()->sheetNodesBegin();
                         it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
                        if (it->second->m_selected && it->first->isShown()) {
                            it->second->restoreState(it->second->m_state);
                            it->second->setSelected(true);
                        }
                    }
                    plasma::Vector<2, float> scale(to.m_elements[0] / from.m_elements[0],
                                                   to.m_elements[1] / from.m_elements[1]);
                    scaleNodes(m_workspace->getDesignSheet()->m_root->getMatrix().transform(m_dragAnchor), scale,
                               transformations, 0);
                    m_workspace->m_textModule->currentNodeChanged();
                }
                updateBox();
            }
        } else {
            if (isRotating()) {
                if (m_workspace->getTextureSheet()) {
                    plasma::Vector<3, float> rotation(0.0f, 0.0f, m_workspace->m_pointerDeltaX * (double)0.4f);
                    rotateTextureCoordinates(m_workspace->getOpenSheet()->m_root->getMatrix() *
                                                 m_workspace->m_sheetPointer,
                                             shapes, rotation);
                } else if (m_workspace->getDesignSheet()) {
                    plasma::Vector<3, float> rotation(0.0f, 0.0f, m_workspace->m_pointerDeltaX * (double)0.4f);
                    rotateNodes(m_workspace->getDesignSheet()->m_root->getMatrix() * m_workspace->m_sheetPointer,
                                rotation, transformations, 0);
                }
            } else if (m_workspace->getTextureSheet()) {
                moveTextureCoordinates(shapes, m_workspace->getDragMovement());
            } else if (m_workspace->getDesignSheet()) {
                moveNodes(transformations, m_workspace->getDragMovement(), 0);
            }
        }
    } else {
        updateSelection();
    }
}

bool NodeTool::isRotating()
{
    return (m_workspace->m_engine.isKeyDown(VK_MENU) && m_workspace->m_nodeToolModule->m_editMode == EDIT_MOVE) ||
           (!m_workspace->m_engine.isKeyDown(VK_MENU) && m_workspace->m_nodeToolModule->m_editMode == EDIT_ROTATE);
}

void NodeTool::moveTextureCoordinates(std::set<plasma::Shape *> &shapes, const plasma::Vector<2, float> &offset)
{
    plasma::Matrix<float> inverse = m_workspace->getTextureSheet()->getMatrix();
    inverse.invert();
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::const_iterator it = sheet->sheetNodesBegin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown()) {
            plasma::Shape *shape = it->first->getShape();
            plasma::SmoothMeshShape *mesh = shape && shape->getType() == plasma::SHAPE_SMOOTH_MESH
                                                ? static_cast<plasma::SmoothMeshShape *>(shape)
                                                : 0;
            plasma::Shape *key = mesh;
            if (mesh && shapes.find(key) == shapes.end()) {
                shapes.insert(key);
                plasma::Vector<2, float> delta;
                const float *elements = inverse.m_elements;
                delta[0] = elements[0] * offset[0] + elements[4] * offset[1];
                delta[1] = elements[1] * offset[0] + elements[5] * offset[1];
                plasma::Vector<2, float> &translation = mesh->m_textureTranslation.value();
                translation += delta;
                mesh->lock();
                mesh->updateTextureMatrix();
                mesh->updateDrawingTextures();
                mesh->unlock();
                mesh->m_changed = true;
                it->second->m_shapeInterface->m_hasChanges = true;
            }
        }
    }
}

void NodeTool::rotateTextureCoordinates(const plasma::Vector<2, float> &pivot, std::set<plasma::Shape *> &shapes,
                                        const plasma::Vector<3, float> &rotation)
{
    plasma::Matrix<float> inverse = m_workspace->getTextureSheet()->getMatrix();
    inverse.invert();
    plasma::Vector<2, float> sheetPivot = inverse.transform(pivot);
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::const_iterator it = sheet->sheetNodesBegin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown()) {
            plasma::Shape *shape = it->first->getShape();
            plasma::SmoothMeshShape *mesh = shape && shape->getType() == plasma::SHAPE_SMOOTH_MESH
                                                ? static_cast<plasma::SmoothMeshShape *>(shape)
                                                : 0;
            if (mesh && shapes.find(shape) == shapes.end()) {
                shapes.insert(shape);
                plasma::Matrix<float> toMesh;
                toMesh = m_workspace->getTextureSheet()->getMatrix();
                toMesh *= mesh->m_textureMatrix;
                toMesh.invert();
                plasma::Matrix<float> rotationMatrix;
                rotationMatrix.identity();
                rotationMatrix *= toMesh;
                rotationMatrix.rotateZ(rotation.m_elements[2]);
                plasma::Matrix<float> fromMesh;
                fromMesh = m_workspace->getTextureSheet()->getMatrix();
                fromMesh *= mesh->m_textureMatrix;
                rotationMatrix *= fromMesh;
                rotationMatrix.m_elements[12] = 0.0f;
                rotationMatrix.m_elements[13] = 0.0f;
                rotationMatrix.m_elements[14] = 0.0f;
                plasma::Matrix<float> reverseRotation;
                reverseRotation.identity();
                reverseRotation.rotateZ(-rotation.m_elements[2]);
                mesh->m_textureDeformation.value() =
                    (reverseRotation * mesh->m_textureDeformation.value()) * rotationMatrix;
                mesh->updateTextureMatrix();
                plasma::Vector<2, float> localPivot = toMesh * pivot;
                mesh->m_textureRotation.value().m_elements[0] += rotation.m_elements[0];
                mesh->m_textureRotation.value().m_elements[1] += rotation.m_elements[1];
                mesh->m_textureRotation.value().m_elements[2] += rotation.m_elements[2];
                mesh->updateTextureMatrix();
                plasma::Vector<2, float> delta = sheetPivot - mesh->m_textureMatrix * localPivot;
                plasma::Vector<2, float> &translation = mesh->m_textureTranslation.value();
                translation[0] += delta[0];
                translation[1] += delta[1];
                mesh->updateTextureMatrix();
                mesh->lock();
                mesh->updateDrawingTextures();
                mesh->unlock();
                mesh->m_changed = true;
            }
        }
    }
}

void NodeTool::scaleTextureCoordinates(std::set<plasma::Shape *> &shapes, const plasma::Vector<2, float> &pivot,
                                       const plasma::Vector<2, float> &scale)
{
    plasma::Matrix<float> inverse = m_workspace->getTextureSheet()->getMatrix();
    inverse.invert();
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::const_iterator it = sheet->sheetNodesBegin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown()) {
            ShapeInterface *shapeInterface = it->second->m_shapeInterface;
            if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH)
                static_cast<MeshInterface *>(shapeInterface)->restoreTextureTransform();
            plasma::Shape *shape = it->first->getShape();
            plasma::SmoothMeshShape *mesh = shape && shape->getType() == plasma::SHAPE_SMOOTH_MESH
                                                ? static_cast<plasma::SmoothMeshShape *>(shape)
                                                : 0;
            plasma::Shape *key = mesh;
            if (mesh && shapes.find(key) == shapes.end()) {
                shapes.insert(key);
                plasma::Matrix<float> oldMatrix = mesh->m_textureMatrix;
                oldMatrix.invert();
                plasma::Matrix<float> toMesh = oldMatrix * inverse;
                plasma::Vector<2, float> localPivot = toMesh.transform(pivot);
                plasma::Matrix<float> linear = mesh->m_textureMatrix;
                linear.m_elements[12] = 0.0f;
                linear.m_elements[13] = 0.0f;
                linear.m_elements[14] = 0.0f;
                linear.m_elements[15] = 1.0f;
                plasma::Matrix<float> inverseLinear = linear;
                inverseLinear.invert();
                plasma::Matrix<float> rootMatrix = m_workspace->getTextureSheet()->getRoot()->m_matrix;
                rootMatrix.m_elements[12] = 0.0f;
                rootMatrix.m_elements[13] = 0.0f;
                rootMatrix.m_elements[14] = 0.0f;
                rootMatrix.m_elements[15] = 1.0f;
                plasma::Matrix<float> inverseRoot = rootMatrix;
                inverseRoot.invert();
                mesh->m_textureDeformation.value() *= inverseLinear * inverseRoot;
                mesh->m_textureDeformation.value().scale(scale.m_elements[0], scale.m_elements[1]);
                mesh->m_textureDeformation.value() *= rootMatrix * linear;
                mesh->updateTextureMatrix();
                plasma::Matrix<float> toSheet;
                toSheet = m_workspace->getTextureSheet()->getMatrix();
                toSheet *= mesh->m_textureMatrix;
                plasma::Vector<2, float> delta = inverse * pivot - inverse.transform(toSheet.transform(localPivot));
                plasma::Vector<2, float> &translation = mesh->m_textureTranslation.value();
                translation[0] += delta[0];
                translation[1] += delta[1];
                mesh->updateTextureMatrix();
                mesh->updateDrawingTextures();
                mesh->m_changed = true;
            }
        }
    }
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Hovered-node label
// placement: offset both coordinates by 15 display units from the pointer. Node overlay palette:
// transparent/gray/white label layers, black boundary stroke, white extrusion and transparent white group fill.
// Draws the box (with the scale box option), the selection rectangle or lasso and, while a node is being picked
// (Choose Parent, Send Behind, Send in Front of) or Ctrl or Shift is held, the hovered node's name beside the
// pointer.
void NodeTool::draw()
{
    glUseProgram(0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    updateBox();
    if (m_workspace->m_nodeToolModule->m_scaleBox)
        renderBox();
    renderSelection();
    Workspace *workspace = m_workspace;
    if (workspace->m_chooseParentRequested || workspace->m_sendInFrontOfRequested || workspace->m_sendBehindRequested ||
        workspace->m_engine.m_pressedKeys.find(VK_CONTROL) != workspace->m_engine.m_pressedKeys.end() ||
        workspace->m_engine.isKeyDown(VK_SHIFT)) {
        if (workspace->getHoveredNode() && workspace->getDesignSheet() &&
            m_workspace->getDesignSheet()->findSheetNode(m_workspace->getHoveredNode())) {
            plasma::Vector<2, float> position = m_workspace->m_sceneMousePosition;
            position.m_elements[0] += (double)15.0f;
            position.m_elements[1] += (double)15.0f;
            m_workspace->m_engine.getFontEngine()->renderText(
                L"verdanab.ttf", m_workspace->getHoveredNode()->getName(), 0.0f, 0.0f, position.m_elements[0],
                position.m_elements[1], 8.0f, 1.5f, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                plasma::Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f, true);
            m_workspace->m_engine.getFontEngine()->renderText(
                L"verdanab.ttf", m_workspace->getHoveredNode()->getName(), 0.0f, 0.0f, position.m_elements[0],
                position.m_elements[1], 8.0f, 0.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f, true);
        }
    }
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Empty bounds start
// with positive minima and negative maxima.
void NodeTool::updateBox()
{
    m_boxMin = plasma::Vector<2, float>(1.0e9f, 1.0e8f);
    bool first = true;
    m_boxMax.m_elements[0] = -1.0e9f;
    m_boxMax.m_elements[1] = -1.0e9f;
    if (m_workspace->getDesignSheet()) {
        if (m_workspace->m_toolModule->getCurrentTool()->getType() == TOOL_WIDGET) {
            // The widget tool: the corners of the widgets' rectangles.
            DesignSheet *sheet = m_workspace->getDesignSheet();
            std::map<Node *, NodeInterface *>::iterator it;
            if (sheet->isBusy())
                it = sheet->m_sheetNodes.end();
            else
                it = sheet->m_sheetNodes.begin();
            for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
                if (it->second->m_selected && it->first->isShown() && !it->first->isLocked() && it->first->m_widget) {
                    plasma::Vector<2, float> position = it->first->m_widget->getContentPosition();
                    plasma::Vector<2, float> size = it->first->m_widget->screenSize();
                    plasma::Vector<2, float> corners[4] = {position,
                                                           position + plasma::Vector<2, float>(1.0f, 0.0f) * size,
                                                           position + plasma::Vector<2, float>(1.0f, 1.0f) * size,
                                                           position + plasma::Vector<2, float>(0.0f, 1.0f) * size};
                    for (int i = 0; i < 4; ++i) {
                        if (first) {
                            m_boxMin = corners[i];
                            m_boxMax = corners[i];
                            first = false;
                        } else {
                            if (corners[i].m_elements[0] < m_boxMin.m_elements[0])
                                m_boxMin.m_elements[0] = corners[i].m_elements[0];
                            if (corners[i].m_elements[1] < m_boxMin.m_elements[1])
                                m_boxMin.m_elements[1] = corners[i].m_elements[1];
                            if (corners[i].m_elements[0] > m_boxMax.m_elements[0])
                                m_boxMax.m_elements[0] = corners[i].m_elements[0];
                            if (corners[i].m_elements[1] > m_boxMax.m_elements[1])
                                m_boxMax.m_elements[1] = corners[i].m_elements[1];
                        }
                    }
                }
            }
        } else {
            DesignSheet *sheet = m_workspace->getDesignSheet();
            std::map<Node *, NodeInterface *>::iterator it;
            if (sheet->isBusy())
                it = sheet->m_sheetNodes.end();
            else
                it = sheet->m_sheetNodes.begin();
            for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
                if (it->second->m_selected && it->first->isShown() && !it->first->isLocked() &&
                    it->second->m_shapeInterface) {
                    plasma::Vector<2, float> boxMin, boxMax;
                    it->second->m_shapeInterface->getBounds(&boxMin, &boxMax);
                    if (first) {
                        first = false;
                        m_boxMin = boxMin;
                        m_boxMax = boxMax;
                    } else {
                        if (boxMin.m_elements[0] < m_boxMin.m_elements[0])
                            m_boxMin.m_elements[0] = boxMin.m_elements[0];
                        if (boxMin.m_elements[1] < m_boxMin.m_elements[1])
                            m_boxMin.m_elements[1] = boxMin.m_elements[1];
                        if (boxMax.m_elements[0] > m_boxMax.m_elements[0])
                            m_boxMax.m_elements[0] = boxMax.m_elements[0];
                        if (boxMax.m_elements[1] > m_boxMax.m_elements[1])
                            m_boxMax.m_elements[1] = boxMax.m_elements[1];
                    }
                }
            }
        }
        m_boxMin.transform(m_workspace->getDesignSheet()->m_root->m_inverseMatrix);
        m_boxMax.transform(m_workspace->getDesignSheet()->m_root->m_inverseMatrix);
    } else if (m_workspace->getTextureSheet() && m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && !it->first->isLocked() &&
                it->second->m_shapeInterface) {
                it->second->getShapeInterface()->setMatrix(m_workspace->getTextureSheet()->getMatrix());
                plasma::Vector<2, float> boxMin, boxMax;
                it->second->m_shapeInterface->getBounds(&boxMin, &boxMax);
                if (first) {
                    first = false;
                    m_boxMin = boxMin;
                    m_boxMax = boxMax;
                } else {
                    if (boxMin.m_elements[0] < m_boxMin.m_elements[0])
                        m_boxMin.m_elements[0] = boxMin.m_elements[0];
                    if (boxMin.m_elements[1] < m_boxMin.m_elements[1])
                        m_boxMin.m_elements[1] = boxMin.m_elements[1];
                    if (boxMax.m_elements[0] > m_boxMax.m_elements[0])
                        m_boxMax.m_elements[0] = boxMax.m_elements[0];
                    if (boxMax.m_elements[1] > m_boxMax.m_elements[1])
                        m_boxMax.m_elements[1] = boxMax.m_elements[1];
                }
                it->second->m_shapeInterface->resetMatrix();
            }
        }
        m_boxMin.transform(m_workspace->getTextureSheet()->m_root->m_inverseMatrix);
        m_boxMax.transform(m_workspace->getTextureSheet()->m_root->m_inverseMatrix);
    }
    updateHandles();
}

void NodeTool::moveNodes(std::set<plasma::Keyable *> &transformations, const plasma::Vector<2, float> &offset,
                         Node *node)
{
    if (!node)
        node = m_workspace->getDesignSheet()->m_root;
    bool excluded = (node->m_flags & (1 << plasma::NODE_INTERNAL)) != 0;
    if (!node->isShown() || excluded)
        return;
    if (m_workspace->getDesignSheet()->findSheetNode(node)->m_selected &&
        transformations.find(node->m_transformation) == transformations.end()) {
        transformations.insert(static_cast<plasma::Keyable *>(node->m_transformation));

        // A direction uses the parent's inverse linear transform.
        plasma::Vector<2, float> delta;
        const float *elements = node->m_parent->m_inverseMatrix.m_elements;
        delta[0] = elements[0] * offset[0] + elements[4] * offset[1];
        delta[1] = elements[1] * offset[0] + elements[5] * offset[1];
        plasma::Vector<2, float> &translation = node->m_transformation->m_translation.value();
        translation.m_elements[0] += delta.m_elements[0];
        translation.m_elements[1] += delta.m_elements[1];
        node->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        if (!m_workspace->m_performanceModule->m_realtimeUpdate) {
            node->m_transformation->m_changed = false;
            node->updateMatrices();
        }
        m_workspace->getDesignSheet()->findSheetNode(node)->m_modificationPending = true;
    } else {
        for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
            moveNodes(transformations, offset, *it);
    }
}

void NodeTool::scaleNodes(const plasma::Vector<2, float> &center, const plasma::Vector<2, float> &scale,
                          std::set<plasma::Keyable *> &transformations, Node *node)
{
    if (!node)
        node = m_workspace->getDesignSheet()->m_root;
    bool excluded = (node->m_flags & (1 << plasma::NODE_INTERNAL)) != 0;
    if (!node->isShown() || excluded)
        return;
    if (m_workspace->getDesignSheet()->findSheetNode(node)->m_selected &&
        transformations.find(node->m_transformation) == transformations.end()) {
        transformations.insert(node->transformation());
        node->m_transformation->scale(center, scale, node->m_parent->m_matrix, node->m_parent->m_inverseMatrix);
        node->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        if (!m_workspace->m_performanceModule->m_realtimeUpdate) {
            node->m_transformation->m_changed = false;
            node->updateMatrices();
        }
        m_workspace->getDesignSheet()->findSheetNode(node)->m_modificationPending = true;
    } else {
        for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
            scaleNodes(center, scale, transformations, *it);
    }
}

void NodeTool::rotateNodes(const plasma::Vector<2, float> &pivot, const plasma::Vector<3, float> &rotation,
                           std::set<plasma::Keyable *> &transformations, Node *node)
{
    if (!node)
        node = m_workspace->getDesignSheet()->m_root;
    bool excluded = (node->m_flags & (1 << plasma::NODE_INTERNAL)) != 0;
    if (!node->isShown() || excluded)
        return;
    if (m_workspace->getDesignSheet()->findSheetNode(node)->m_selected &&
        transformations.find(node->m_transformation) == transformations.end()) {
        transformations.insert(node->transformation());
        node->transformation()->rotate(rotation, node->m_matrix, node->m_parent->m_inverseMatrix * pivot,
                                       node->m_inverseMatrix);
        if (!m_workspace->m_performanceModule->m_realtimeUpdate) {
            node->m_transformation->m_changed = false;
            node->updateMatrices();
        }
        m_workspace->getDesignSheet()->findSheetNode(node)->m_modificationPending = true;
    } else {
        for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
            rotateNodes(pivot, rotation, transformations, *it);
    }
}

void NodeTool::bringToFront()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    bringToFront(0);
    m_workspace->markDesignSheetChanged();
}

void NodeTool::sendToBack()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    sendToBack(0);
    m_workspace->markDesignSheetChanged();
}

void NodeTool::bringForward()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    bringForward(0);
    m_workspace->markDesignSheetChanged();
}

void NodeTool::sendBackward()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    sendBackward(0);
    m_workspace->markDesignSheetChanged();
}

bool NodeTool::bringForward(Node *node)
{
    if (!node) {
        std::list<Node *> children = m_workspace->getOpenDesignSheet()->m_root->getChildren();
        for (std::list<Node *>::reverse_iterator it = children.rbegin(); it != children.rend(); ++it)
            if (!bringForward(*it))
                return true;
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->m_layerModule->updatePanel();
        return true;
    }
    if (!node->isShown() || node->isInternal())
        return true;
    if (m_workspace->getOpenDesignSheet()->findSheetNode(node)->m_selected) {
        Node *parent = node->getParent();
        Node *nextSibling = node->getNextSibling();
        std::list<Node *>::iterator it = parent->m_children.begin();
        for (; it != parent->m_children.end(); ++it)
            if (*it == node) {
                ++it;
                break;
            }
        if (it == parent->m_children.end())
            return false;
        Node *next = *it;
        if (!next)
            return false;
        parent->removeChild(node);
        for (it = parent->m_children.begin(); it != parent->m_children.end(); ++it)
            if (*it == next) {
                ++it;
                parent->insertChild(it, node);
                m_workspace->getOpenDesignSheet()->addOperation(new NodeHierarchyOperation(
                    m_workspace->getOpenDesignSheet(), "bring forward", node, parent, nextSibling));
                break;
            }

        // The parent aligns its children after the order changes.
        m_workspace->realignChildren(parent);
        return true;
    } else {
        std::list<Node *> children = node->getChildren();
        for (std::list<Node *>::reverse_iterator it = children.rbegin(); it != children.rend(); ++it)
            if (!bringForward(*it))
                return true;
        return true;
    }
}

// The operation is named "bring forward" too.
bool NodeTool::sendBackward(Node *node)
{
    if (!node) {
        std::list<Node *> children = m_workspace->getOpenDesignSheet()->m_root->getChildren();
        for (std::list<Node *>::iterator it = children.begin(); it != children.end(); ++it)
            if (!sendBackward(*it))
                return true;
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->redraw();
        return true;
    }
    if (!node->isShown() || node->isInternal())
        return true;
    if (m_workspace->getOpenDesignSheet()->findSheetNode(node)->m_selected) {
        Node *parent = node->getParent();
        Node *nextSibling = node->getNextSibling();
        if (parent->m_children.front() == node)
            return false;
        std::list<Node *>::iterator it = parent->m_children.begin();
        for (; it != parent->m_children.end(); ++it)
            if (*it == node)
                break;
        Node *previous = it != parent->m_children.end() ? *--it : 0;
        if (!previous) {
            parent->insertChild(parent->m_children.end(), node);
        } else {
            parent->removeChild(node);
            for (it = parent->m_children.begin(); it != parent->m_children.end(); ++it)
                if (*it == previous) {
                    parent->insertChild(it, node);
                    m_workspace->getOpenDesignSheet()->addOperation(new NodeHierarchyOperation(
                        m_workspace->getOpenDesignSheet(), "bring forward", node, parent, nextSibling));
                    break;
                }
        }

        // The parent aligns its children after the order changes.
        m_workspace->realignChildren(parent);
        return true;
    } else {
        std::list<Node *> children = node->getChildren();
        for (std::list<Node *>::iterator it = children.begin(); it != children.end(); ++it)
            if (!sendBackward(*it))
                return true;
        return true;
    }
}

bool NodeTool::bringToFront(Node *node)
{
    if (!node) {
        std::list<Node *> children = m_workspace->getOpenDesignSheet()->m_root->getChildren();
        for (std::list<Node *>::iterator it = children.begin(); it != children.end(); ++it)
            if (!bringToFront(*it))
                return true;
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->redraw();
        return true;
    }
    if (!node->isShown() || node->isInternal())
        return true;
    if (m_workspace->getOpenDesignSheet()->findSheetNode(node)->m_selected) {
        Node *parent = node->getParent();
        Node *nextSibling = node->getNextSibling();
        if (parent->m_children.back() == node)
            return false;
        parent->insertChild(parent->m_children.end(), node);
        m_workspace->getOpenDesignSheet()->addOperation(
            new NodeHierarchyOperation(m_workspace->getOpenDesignSheet(), "bring to front", node, parent, nextSibling));

        // The parent aligns its children after the order changes.
        m_workspace->realignChildren(parent);
        return true;
    } else {
        std::list<Node *> children = node->getChildren();
        for (std::list<Node *>::iterator it = children.begin(); it != children.end(); ++it)
            if (!bringToFront(*it))
                return true;
        return true;
    }
}

bool NodeTool::sendToBack(Node *node)
{
    if (!node) {
        std::list<Node *> children = m_workspace->getOpenDesignSheet()->m_root->getChildren();
        for (std::list<Node *>::reverse_iterator it = children.rbegin(); it != children.rend(); ++it)
            if (!sendToBack(*it))
                return true;
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->redraw();
        return true;
    }
    if (!node->isShown() || node->isInternal())
        return true;
    if (m_workspace->getOpenDesignSheet()->findSheetNode(node)->m_selected) {
        Node *parent = node->getParent();
        Node *nextSibling = node->getNextSibling();
        if (parent->m_children.front() == node)
            return false;
        parent->removeChild(node);
        parent->insertChild(parent->m_children.begin(), node);
        m_workspace->getOpenDesignSheet()->addOperation(
            new NodeHierarchyOperation(m_workspace->getOpenDesignSheet(), "send to back", node, parent, nextSibling));

        // The parent aligns its children after the order changes.
        m_workspace->realignChildren(parent);
        return true;
    } else {
        std::list<Node *> children = node->getChildren();
        for (std::list<Node *>::reverse_iterator it = children.rbegin(); it != children.rend(); ++it)
            if (!sendToBack(*it))
                return true;
        return true;
    }
}

void NodeTool::fillContextMenu()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::list<Node *> nodes;
    m_workspace->getOpenDesignSheet()->getSelectedNodes(nodes);
    {
        std::wstring caption(L"Bring to Front");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &NodeTool::bringToFront, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Bring Forward");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &NodeTool::bringForward, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Send Backward");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &NodeTool::sendBackward, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Send to Back");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &NodeTool::sendToBack, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Send Behind...");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::sendBehind, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Send in Front of...");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::sendInFrontOf, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Lock");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &NodeTool::lockObjects, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Unlock All");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &NodeTool::unlockAll, true, true);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Group");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &NodeTool::createGroup, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Drop Shadow...");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &NodeTool::showShadowPanel, true, true);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Choose Parent...");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::chooseParent, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Unparent");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::unparent, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Detach Children");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &NodeTool::detachChildren, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Use as Root");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &NodeTool::setCurrentNodeAsRoot, true, true);
        if ((int)nodes.size() != 1)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"No Root");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &NodeTool::resetEditRoot, true, true);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Set Pivot...");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::setPivot, true, true);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Duplicate");
        ClipboardModule *clipboard = m_workspace->m_clipboardModule;
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, clipboard, &ClipboardModule::duplicate, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Duplicate Instance");
        ClipboardModule *clipboard = m_workspace->m_clipboardModule;
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, clipboard, &ClipboardModule::duplicateInstance, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Delete");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &NodeTool::deleteSelected, true, true);
        if ((int)nodes.size() <= 0)
            item->setEnabled(false);
    }
}

void NodeTool::uninstantiate()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    uninstantiate(0);
}

// Gives the selected nodes of the shown subtree (the document's root without one) their own copies of shared
// shapes, transformations and displays ("uninstantiate node"); true when anything changed.
bool NodeTool::uninstantiate(Node *node)
{
    if (!node)
        node = m_workspace->getOpenDesignSheet()->m_root;
    if (!node->isShown() || node->isInternal())
        return false;
    bool changed = false;
    if (m_workspace->getOpenDesignSheet()->findSheetNode(node)->m_selected) {
        plasma::Shape *shape = node->getShape();
        plasma::Transformation *transformation = node->transformation();
        plasma::Display *display = node->display();
        if (shape && shape->countNodes() > 1) {
            changed = true;
            node->setShape(shape->clone());
            m_workspace->getOpenDesignSheet()->findSheetNode(node)->createShapeInterface();
        }
        if (transformation->countNodes() > 1) {
            changed = true;
            node->setTransformation(transformation->clone());
        }
        if (display->countNodes() > 1) {
            changed = true;
            node->setDisplay(display->clone());
        }
        if (changed)
            m_workspace->getOpenDesignSheet()->addOperation(
                new InstanceOperation(m_workspace->getOpenDesignSheet(), "uninstantiate node",
                                      m_workspace->getOpenDesignSheet()->findSheetNode(node), shape, transformation,
                                      display, node->getShape(), node->transformation(), node->display()));
    }
    std::list<Node *> children = node->getChildren();
    for (std::list<Node *>::iterator it = children.begin(); it != children.end(); ++it)
        changed = uninstantiate(*it) || changed;
    if (node == m_workspace->getOpenDesignSheet()->m_root && changed) {
        m_workspace->redraw();
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->m_engine.resetPointer();
        m_workspace->refreshPointer();
        m_workspace->getOpenDesignSheet()->m_root->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
    return changed;
}

void NodeTool::setCurrentNodeAsRoot()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    Node *current = m_workspace->getOpenDesignSheet()->m_currentNode;
    m_workspace->getOpenDesignSheet()->setEditRoot(current, true);
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::resetEditRoot()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    Node *root = m_workspace->getOpenDesignSheet()->m_root;
    m_workspace->getOpenDesignSheet()->setEditRoot(root, false);
}

void NodeTool::rotate45Right()
{
    if (!m_workspace->getOpenSheet())
        return;
    updateBox();
    m_workspace->saveSelectedStates();
    std::set<plasma::Keyable *> transformations;
    plasma::Vector<3, float> rotation(0.0f, 0.0f, 45.0f);

    // The middle of the box, mapped by the current sheet's root matrix.
    plasma::Vector<2, float> center = (m_boxMin + m_boxMax) * 0.5f;
    const plasma::Matrix<float> &matrix = m_workspace->getOpenSheet()->m_root->m_matrix;
    float w = matrix[3] * center[0] + matrix[7] * center[1] + matrix[15];
    plasma::Vector<2, float> pivot =
        plasma::Vector<2, float>(center[0] * matrix[0] + matrix[4] * center[1] + matrix[12],
                                 matrix[1] * center[0] + center[1] * matrix[5] + matrix[13]) *
        (1 / w);
    rotateNodes(pivot, rotation, transformations, 0);
    m_workspace->m_nodeModule->showTransformation();
    m_workspace->recordModifiedNodes();
}

void NodeTool::rotate45Left()
{
    if (!m_workspace->getOpenSheet())
        return;
    updateBox();
    m_workspace->saveSelectedStates();
    std::set<plasma::Keyable *> transformations;
    plasma::Vector<3, float> rotation(0.0f, 0.0f, -45.0f);

    // The middle of the box, mapped by the current sheet's root matrix.
    plasma::Vector<2, float> center = (m_boxMin + m_boxMax) * 0.5f;
    const plasma::Matrix<float> &matrix = m_workspace->getOpenSheet()->m_root->m_matrix;
    float w = matrix[3] * center[0] + matrix[7] * center[1] + matrix[15];
    plasma::Vector<2, float> pivot =
        plasma::Vector<2, float>(center[0] * matrix[0] + matrix[4] * center[1] + matrix[12],
                                 matrix[1] * center[0] + center[1] * matrix[5] + matrix[13]) *
        (1 / w);
    rotateNodes(pivot, rotation, transformations, 0);
    m_workspace->m_nodeModule->showTransformation();
    m_workspace->recordModifiedNodes();
}

void NodeTool::rotate90Right()
{
    if (!m_workspace->getOpenSheet())
        return;
    updateBox();
    m_workspace->saveSelectedStates();
    std::set<plasma::Keyable *> transformations;
    plasma::Vector<3, float> rotation(0.0f, 0.0f, 90.0f);

    // The middle of the box, mapped by the current sheet's root matrix.
    plasma::Vector<2, float> center = (m_boxMin + m_boxMax) * 0.5f;
    const plasma::Matrix<float> &matrix = m_workspace->getOpenSheet()->m_root->m_matrix;
    float w = matrix[3] * center[0] + matrix[7] * center[1] + matrix[15];
    plasma::Vector<2, float> pivot =
        plasma::Vector<2, float>(center[0] * matrix[0] + matrix[4] * center[1] + matrix[12],
                                 matrix[1] * center[0] + center[1] * matrix[5] + matrix[13]) *
        (1 / w);
    rotateNodes(pivot, rotation, transformations, 0);
    m_workspace->m_nodeModule->showTransformation();
    m_workspace->recordModifiedNodes();
}

void NodeTool::rotate90Left()
{
    if (!m_workspace->getOpenSheet())
        return;
    updateBox();
    m_workspace->saveSelectedStates();
    std::set<plasma::Keyable *> transformations;
    plasma::Vector<3, float> rotation(0.0f, 0.0f, -90.0f);

    // The middle of the box, mapped by the current sheet's root matrix.
    plasma::Vector<2, float> center = (m_boxMin + m_boxMax) * 0.5f;
    const plasma::Matrix<float> &matrix = m_workspace->getOpenSheet()->m_root->m_matrix;
    float w = matrix[3] * center[0] + matrix[7] * center[1] + matrix[15];
    plasma::Vector<2, float> pivot =
        plasma::Vector<2, float>(center[0] * matrix[0] + matrix[4] * center[1] + matrix[12],
                                 matrix[1] * center[0] + center[1] * matrix[5] + matrix[13]) *
        (1 / w);
    rotateNodes(pivot, rotation, transformations, 0);
    m_workspace->m_nodeModule->showTransformation();
    m_workspace->recordModifiedNodes();
}

void NodeTool::rotate180()
{
    if (!m_workspace->getOpenSheet())
        return;
    updateBox();
    m_workspace->saveSelectedStates();
    std::set<plasma::Keyable *> transformations;
    plasma::Vector<3, float> rotation(0.0f, 0.0f, 180.0f);

    // The middle of the box, mapped by the current sheet's root matrix.
    plasma::Vector<2, float> center = (m_boxMin + m_boxMax) * 0.5f;
    const plasma::Matrix<float> &matrix = m_workspace->getOpenSheet()->m_root->m_matrix;
    float w = matrix[3] * center[0] + matrix[7] * center[1] + matrix[15];
    plasma::Vector<2, float> pivot =
        plasma::Vector<2, float>(center[0] * matrix[0] + matrix[4] * center[1] + matrix[12],
                                 matrix[1] * center[0] + center[1] * matrix[5] + matrix[13]) *
        (1 / w);
    rotateNodes(pivot, rotation, transformations, 0);
    m_workspace->m_nodeModule->showTransformation();
    m_workspace->recordModifiedNodes();
}

void NodeTool::flipHorizontally()
{
    if (!m_workspace->getOpenSheet())
        return;
    updateBox();
    m_workspace->saveSelectedStates();
    std::set<plasma::Keyable *> transformations;
    plasma::Vector<2, float> scale(-1.0f, 1.0f);

    // The middle of the box, mapped by the current sheet's root matrix.
    plasma::Vector<2, float> center = (m_boxMin + m_boxMax) * 0.5f;
    const plasma::Matrix<float> &matrix = m_workspace->getOpenSheet()->m_root->m_matrix;
    float perspectiveX = matrix[3];
    float perspectiveY = matrix[7];
    float perspectiveW = matrix[15];
    float w = perspectiveY * center[1] + perspectiveX * center[0] + perspectiveW;
    plasma::Vector<2, float> pivot =
        plasma::Vector<2, float>(center[0] * matrix[0] + matrix[4] * center[1] + matrix[12],
                                 matrix[1] * center[0] + center[1] * matrix[5] + matrix[13]) *
        (1 / w);
    scaleNodes(pivot, scale, transformations, 0);
    m_workspace->m_nodeModule->showTransformation();
    m_workspace->recordModifiedNodes();
}

void NodeTool::flipVertically()
{
    if (!m_workspace->getOpenSheet())
        return;
    updateBox();
    m_workspace->saveSelectedStates();
    std::set<plasma::Keyable *> transformations;
    plasma::Vector<2, float> scale(1.0f, -1.0f);

    // The middle of the box, mapped by the current sheet's root matrix.
    plasma::Vector<2, float> center = (m_boxMin + m_boxMax) * 0.5f;
    const plasma::Matrix<float> &matrix = m_workspace->getOpenSheet()->m_root->m_matrix;
    float perspectiveX = matrix[3];
    float perspectiveY = matrix[7];
    float perspectiveW = matrix[15];
    float w = perspectiveY * center[1] + perspectiveX * center[0] + perspectiveW;
    plasma::Vector<2, float> pivot =
        plasma::Vector<2, float>(center[0] * matrix[0] + matrix[4] * center[1] + matrix[12],
                                 matrix[1] * center[0] + center[1] * matrix[5] + matrix[13]) *
        (1 / w);
    scaleNodes(pivot, scale, transformations, 0);
    m_workspace->m_nodeModule->showTransformation();
    m_workspace->recordModifiedNodes();
}

// Projects the selected meshes' texture coordinates onto the box (the window menu's command).
void NodeTool::projectTextureCoordinates()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    updateBox();
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface &&
            it->second->m_shapeInterface->m_type == INTERFACE_MESH) {
            MeshInterface *mesh = static_cast<MeshInterface *>(it->second->m_shapeInterface);
            mesh->beginModify();
            mesh->fitTextureToBox(m_boxMin, m_boxMax);
            mesh->refresh();
            mesh->endModify();
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::resetTextureTransformation()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it = sheet->sheetNodesBegin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface &&
            it->second->m_shapeInterface->m_type == INTERFACE_MESH) {
            MeshInterface *mesh = static_cast<MeshInterface *>(it->second->m_shapeInterface);
            mesh->beginModify();
            mesh->m_mesh.m_shape->m_textureDeformation.value().identity();
            mesh->m_mesh.m_shape->m_textureRotation.value() = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            mesh->m_mesh.m_shape->m_textureTranslation.value() = plasma::Vector<2, float>(0.0f, 0.0f);
            mesh->m_mesh.m_shape->m_texturePivot.value() = plasma::Vector<2, float>(0.0f, 0.0f);
            mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            mesh->refresh();
            mesh->endModify();
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::lockObjects()
{
    if (!m_workspace->getOpenDesignSheet())
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown()) {
            NodeState before;
            it->second->saveState(before);
            it->first->m_flags |= (1 << NODE_LOCKED);
            it->second->setSelected(false);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "deselect node", it->second, false));
            NodeState after;
            it->second->saveState(after);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeOperation(m_workspace->getOpenDesignSheet(), "lock", it->second, before, after));
        }
    }
    m_workspace->getOpenDesignSheet()->m_currentNode = 0;
    m_workspace->updateModules();
    m_workspace->redraw();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::unlockAll()
{
    if (!m_workspace->getOpenDesignSheet())
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isLocked() && it->first->isShown()) {
            NodeState before;
            it->second->saveState(before);
            it->first->m_flags &= ~(1 << NODE_LOCKED);
            NodeState after;
            it->second->saveState(after);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeOperation(m_workspace->getOpenDesignSheet(), "unlock", it->second, before, after));
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->redraw();
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Node overlay palette:
// transparent/gray/white label layers, black boundary stroke, white extrusion and transparent white group fill.
plasma::Shape *NodeTool::createBoundaryMesh(Node *node, const plasma::Vector<4, float> &color)
{
    plasma::Matrix<float> identity;
    identity.identity();
    bool empty = true;
    plasma::Vector<2, float> minimum, maximum;

    // The code reads uninitialized bounds when the subtree has no shape.
    node->extendShapeBounds(minimum, maximum, identity, empty);
    plasma::SmoothMeshShape *mesh = node->getEngine()->createSmoothMeshShape(L"Group");
    mesh->m_vertexPositions.resize(4);
    mesh->m_vertexTexCoords.resize(4);
    mesh->m_vertexFlags.resize(4, plasma::VERTEX_SHARP);
    mesh->m_vertexColors.resize(4);
    mesh->m_strokeColors.resize(4);
    mesh->m_extrusionFrontColors.resize(4);
    mesh->m_extrusionBackColors.resize(4);
    mesh->m_vertexPositions.value()[0] = plasma::Vector<2, float>(minimum[0], minimum[1]);
    mesh->m_vertexPositions.value()[1] = plasma::Vector<2, float>(maximum[0], minimum[1]);
    mesh->m_vertexPositions.value()[2] = plasma::Vector<2, float>(maximum[0], maximum[1]);
    mesh->m_vertexPositions.value()[3] = plasma::Vector<2, float>(minimum[0], maximum[1]);
    mesh->m_vertexTexCoords.value()[0] = plasma::Vector<2, float>(0.0f, 0.0f);
    mesh->m_vertexTexCoords.value()[1] = plasma::Vector<2, float>(1.0f, 0.0f);
    mesh->m_vertexTexCoords.value()[2] = plasma::Vector<2, float>(1.0f, 1.0f);
    mesh->m_vertexTexCoords.value()[3] = plasma::Vector<2, float>(0.0f, 1.0f);
    mesh->m_vertexColors.value()[0] = color;
    mesh->m_vertexColors.value()[1] = color;
    mesh->m_vertexColors.value()[2] = color;
    mesh->m_vertexColors.value()[3] = color;
    mesh->m_strokeColors.value()[0] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    mesh->m_strokeColors.value()[1] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    mesh->m_strokeColors.value()[2] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    mesh->m_strokeColors.value()[3] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    mesh->m_extrusionFrontColors.value()[0] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    mesh->m_extrusionFrontColors.value()[1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    mesh->m_extrusionFrontColors.value()[2] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    mesh->m_extrusionFrontColors.value()[3] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    mesh->m_extrusionBackColors.value()[0] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    mesh->m_extrusionBackColors.value()[1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    mesh->m_extrusionBackColors.value()[2] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    mesh->m_extrusionBackColors.value()[3] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    mesh->setSubdivisions(BoundaryMeshSubdivisionCount, false);
    std::vector<std::vector<int>> faces(1);
    faces[0].resize(4);
    faces[0][0] = 0;
    faces[0][1] = 1;
    faces[0][2] = 2;
    faces[0][3] = 3;
    mesh->setFaces(faces, true);
    node->setShape(mesh);
    return mesh;
}

void NodeTool::showShadowPanel()
{
    m_workspace->m_shadowModule->m_panel->getNode()->m_display->m_visibility.value() = 1;
}

void NodeTool::makeTransformationsUnique(Node *node)
{
    node->setTransformation(node->m_transformation->clone());
    for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        makeTransformationsUnique(*it);
}

void NodeTool::lockSelected()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::list<Node *> nodes;
    m_workspace->getSelectedNodes(nodes);
    for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it)
        setLockedRecursively(*it, true);
    m_workspace->getOpenDesignSheet()->m_currentNode = 0;
    m_workspace->updateModules();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::unlockSelected()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::list<Node *> nodes;
    m_workspace->getSelectedNodes(nodes);
    for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it)
        setLockedRecursively(*it, false);
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::lockChildren()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::list<Node *> nodes;
    m_workspace->getSelectedNodes(nodes);
    for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it)
        for (std::list<Node *>::iterator child = (*it)->m_children.begin(); child != (*it)->m_children.end(); ++child)
            if ((*child)->isShown() && !(*child)->isLocked()) {
                m_workspace->getOpenDesignSheet()->findSheetNode(*child)->saveState();
                (*child)->m_flags |= (1 << NODE_LOCKED);
                m_workspace->getOpenDesignSheet()->findSheetNode(*child)->setSelected(false);
                m_workspace->getOpenDesignSheet()->findSheetNode(*child)->recordModification();
            }
    if (m_workspace->getOpenDesignSheet()->m_currentNode &&
        m_workspace->getOpenDesignSheet()->m_currentNode->isLocked()) {
        m_workspace->getOpenDesignSheet()->updateCurrentNode(0);
        m_workspace->updateModules();
    }
    m_workspace->getOpenDesignSheet()->commit();
}

// "Drop Shadow" (ShadowModule::Apply): behind each selected shown node, a new "<name>Shadow" node holding a locked
// instance of the node, with the shadow panel's offset, blur and colour ("drop to shadow").
void NodeTool::dropToShadow()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown()) {
            Node *shadow = m_workspace->m_engine.createNode(0, 0, 0, 0, it->first->getName() + L"Shadow");
            shadow->m_flags |= (1 << plasma::NODE_FIXED);
            shadow->m_display->m_flags |= (1 << plasma::DISPLAY_BLUR) | (1 << plasma::DISPLAY_SCREEN);
            shadow->m_display->m_clipping.value() = 1;
            ShadowModule *shadowModule = m_workspace->m_shadowModule;
            float blur = shadowModule->m_blur;
            shadow->m_display->m_blurRadius.value() = blur;
            plasma::Vector<2, float> &translation = shadow->transformation()->m_translation.value();
            translation.m_elements[0] = shadowModule->m_offsetX;
            translation.m_elements[1] = shadowModule->m_offsetY;
            shadow->transformation()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            Node *instance = it->first->instance(shadow);
            makeTransformationsUnique(instance);
            it->first->getParent()->insertChild(shadow, it->first);
            createBoundaryMesh(shadow, m_workspace->m_shadowModule->m_color);
            shadow->m_flags |= (1 << plasma::NODE_ADAPT);
            m_workspace->getOpenDesignSheet()->createSheetNodes(shadow);
            m_workspace->getOpenDesignSheet()->addOperation(
                new CreateNodeOperation(m_workspace->getOpenDesignSheet(), "drop to shadow", shadow));
            setLockedRecursively(instance, true);
        }
    }
    m_workspace->updateModules();
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->markDesignSheetChanged();
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Node overlay palette:
// transparent/gray/white label layers, black boundary stroke, white extrusion and transparent white group fill.
// "Group": a new group node in the current node's place (at the top of the document when the current node's parent
// is selected) takes the selected nodes; its pivot is the middle of their bounds and it becomes the only selected
// and the current node ("create group").
void NodeTool::createGroup()
{
    if (!m_workspace->getOpenDesignSheet() || !m_workspace->getOpenDesignSheet()->m_currentNode)
        return;
    Node *current = m_workspace->getOpenDesignSheet()->m_currentNode;
    Node *parent = current->getParent();
    Node *before = m_workspace->getOpenDesignSheet()->m_currentNode->getNextSibling();
    if (m_workspace->getOpenDesignSheet()->findSheetNode(parent)->m_selected) {
        parent = m_workspace->getOpenDesignSheet()->m_root;
        before = 0;
    }
    Node *group = m_workspace->m_engine.createNode(0, 0, 0, 0, L"Group");
    parent->insertChild(group, before);
    m_workspace->getOpenDesignSheet()->addOperation(
        new CreateNodeOperation(m_workspace->getOpenDesignSheet(), "create group", group));
    NodeInterface *sheetNode = m_workspace->getOpenDesignSheet()->createSheetNode(group);
    m_workspace->moveSelectedNodes(m_workspace->getOpenDesignSheet()->m_editRoot, group);
    plasma::Shape *shape = createBoundaryMesh(group, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 0.0f));
    plasma::Vector<2, float> sum = shape->getMinimum() + shape->getMaximum();
    plasma::Vector<2, float> center(sum.m_elements[0] * (double)0.5f, sum.m_elements[1] * (double)0.5f);
    group->m_transformation->setPivot(center);
    group->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    group->m_flags |= (1 << plasma::NODE_ADAPT) | (1 << plasma::NODE_FIXED);
    sheetNode->createShapeInterface();
    m_workspace->forEachUnlockedSheetNode(deselectSheetNode);
    sheetNode->setSelected(true);
    m_workspace->getOpenDesignSheet()->m_currentNode = group;
    m_workspace->updateModules();
    m_workspace->refreshTimeline();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::detachChildren()
{
    if (!m_workspace->getOpenDesignSheet())
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (!it->second->m_selected)
            continue;
        if (!it->first->isLocked() && it->first->isShown() && it->first->countVisibleChildren() > 0) {
            std::list<Node *> children = it->first->getChildren();
            for (std::list<Node *>::reverse_iterator child = children.rbegin(); child != children.rend(); ++child)
                if ((*child)->isShown())
                    m_workspace->moveNode(*child, it->first->getParent(), it->first->getNextSibling());
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->getOpenDesignSheet()->updateCurrentNode(0);
    m_workspace->updateModules();
    m_workspace->refreshTimeline();
}

void NodeTool::lockChildrenRecursively()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::list<Node *> nodes;
    m_workspace->getSelectedNodes(nodes);
    for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it)
        for (std::list<Node *>::iterator child = (*it)->m_children.begin(); child != (*it)->m_children.end(); ++child)
            setLockedRecursively(*child, true);
    if (m_workspace->getOpenDesignSheet()->m_currentNode &&
        m_workspace->getOpenDesignSheet()->m_currentNode->isLocked()) {
        m_workspace->getOpenDesignSheet()->updateCurrentNode(0);
        m_workspace->updateModules();
    }
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::unlockChildren()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::list<Node *> nodes;
    m_workspace->getSelectedNodes(nodes);
    for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it)
        for (std::list<Node *>::iterator child = (*it)->m_children.begin(); child != (*it)->m_children.end(); ++child)
            if ((*child)->isShown() && (*child)->isLocked()) {
                m_workspace->getOpenDesignSheet()->findSheetNode(*child)->saveState();
                (*child)->m_flags &= ~(1 << NODE_LOCKED);
                m_workspace->getOpenDesignSheet()->findSheetNode(*child)->recordModification();
            }
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::selectAll()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (!it->first->isLocked() && it->first->isShown() && !it->second->m_selected) {
            Node *root = m_workspace->getOpenDesignSheet()->m_root;
            if (it->first == root)
                continue;
            it->second->setSelected(true);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "select node", it->second, true));
        }
    }
    m_workspace->getOpenDesignSheet()->updateCurrentNode(0);
    m_workspace->updateModules();
    m_workspace->refreshTimeline();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::deselectAll()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it = sheet->sheetNodesBegin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (!it->first->isLocked() && it->first->isShown() && it->second->m_selected) {
            it->second->setSelected(false);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "deselect node", it->second, false));
        }
    }
    m_workspace->getOpenDesignSheet()->m_currentNode = 0;
    m_workspace->updateModules();
    m_workspace->refreshTimeline();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::selectParent()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::list<Node *> nodes;
    m_workspace->getSelectedNodes(nodes);
    std::set<Node *> parents;
    for (std::list<Node *>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (!m_workspace->m_engine.isKeyDown(VK_SHIFT)) {
            m_workspace->getOpenDesignSheet()->findSheetNode(*it)->setSelected(false);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "deselect node",
                                           m_workspace->getOpenDesignSheet()->findSheetNode(*it), false));
        }
        if ((*it)->getParent() != m_workspace->getOpenDesignSheet()->m_root && !(*it)->getParent()->isLocked())
            parents.insert((*it)->getParent());
    }
    m_workspace->getOpenDesignSheet()->m_currentNode = 0;
    for (std::set<Node *>::const_iterator parent = parents.begin(); parent != parents.end(); ++parent) {
        m_workspace->getOpenDesignSheet()->findSheetNode(*parent)->setSelected(true);
        m_workspace->getOpenDesignSheet()->m_currentNode = *parent;
        m_workspace->getOpenDesignSheet()->addOperation(
            new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "select node",
                                       m_workspace->getOpenDesignSheet()->findSheetNode(*parent), true));
    }
    m_workspace->updateModules();
    m_workspace->refreshTimeline();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::selectChildren()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::list<Node *> nodes;
    m_workspace->getSelectedNodes(nodes);
    for (std::list<Node *>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (!m_workspace->m_engine.isKeyDown(VK_SHIFT)) {
            m_workspace->getOpenDesignSheet()->findSheetNode(*it)->setSelected(false);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "deselect node",
                                           m_workspace->getOpenDesignSheet()->findSheetNode(*it), false));
        }
        for (std::list<Node *>::const_iterator child = (*it)->m_children.begin(); child != (*it)->m_children.end();
             ++child)
            if ((*child)->isShown() && !(*child)->isLocked()) {
                m_workspace->getOpenDesignSheet()->findSheetNode(*child)->setSelected(true);
                m_workspace->getOpenDesignSheet()->addOperation(
                    new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "select node",
                                               m_workspace->getOpenDesignSheet()->findSheetNode(*child), true));
            }
    }
    m_workspace->getOpenDesignSheet()->updateCurrentNode(0);
    m_workspace->updateModules();
    m_workspace->refreshTimeline();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::selectHierarchy()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::list<Node *> nodes;
    m_workspace->getSelectedNodes(nodes);
    for (std::list<Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it)
        selectRecursively(*it, true);
    m_workspace->redraw();
    m_workspace->refreshTimeline();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeTool::setLockedRecursively(Node *node, bool locked)
{
    if (!node->isShown())
        return;
    if (node->isLocked() != locked) {
        NodeInterface *sheetNode = m_workspace->getOpenDesignSheet()->findSheetNode(node);
        sheetNode->saveState();
        if (locked) {
            node->m_flags |= (1 << NODE_LOCKED);
            if (sheetNode->m_selected) {
                sheetNode->setSelected(false);
                m_workspace->getOpenDesignSheet()->addOperation(
                    new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "deselect node", sheetNode, false));
            }
        } else {
            node->m_flags &= ~(1 << NODE_LOCKED);
        }
        sheetNode->recordModification();
    }
    for (std::list<Node *>::const_iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        setLockedRecursively(*it, locked);
}

void NodeTool::selectRecursively(Node *node, bool select)
{
    if (!node->isShown())
        return;
    if (!node->isLocked()) {
        NodeInterface *sheetNode = m_workspace->getOpenDesignSheet()->findSheetNode(node);
        if (sheetNode->m_selected != select) {
            sheetNode->setSelected(select);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "select node", sheetNode, select));
        }
    }
    for (std::list<Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        selectRecursively(*it, select);
}

void NodeTool::beginBoxChange()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    if (m_workspace->getTextureSheet()) {
        m_workspace->beginModifySelected();
        for (std::map<Node *, NodeInterface *>::const_iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface &&
                it->second->m_shapeInterface->m_type == INTERFACE_MESH)
                static_cast<MeshInterface *>(it->second->m_shapeInterface)->storeTextureTransform();
        }
    }
    if (m_workspace->getDesignSheet())
        m_workspace->saveSelectedStates();
}

void NodeTool::endBoxChange()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    for (std::map<Node *, NodeInterface *>::const_iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
         it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_modificationPending &&
            !m_workspace->m_performanceModule->m_realtimeUpdate)
            it->first->m_transformation->m_changed = true;
    }
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->getOpenDesignSheet()->endModifyChanged();
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->redraw();
}

// The scale box changed from m_boxMin..m_boxMax to boxMin..boxMax: the selected nodes (or, in the texture sheet,
// the selected meshes' textures) get their saved state back, then are moved by the minimum's offset and scaled
// about it.
void NodeTool::setBox(const plasma::Vector<2, float> &boxMin, const plasma::Vector<2, float> &boxMax)
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    if (m_workspace->getTextureSheet()) {
        std::set<plasma::Shape *> shapes;
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<Node *, NodeInterface *>::const_iterator it = sheet->sheetNodesBegin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown()) {
                ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH)
                    static_cast<MeshInterface *>(shapeInterface)->restoreTextureTransform();
            }
        }
        updateBox();
        plasma::Vector<2, float> scale = (boxMax - boxMin) / (m_boxMax - m_boxMin);
        const plasma::Matrix<float> &rootMatrix = m_workspace->getTextureSheet()->m_root->m_matrix;
        float w = rootMatrix[3] * boxMin.m_elements[0] + boxMin.m_elements[1] * rootMatrix[7] + rootMatrix[15];
        scaleTextureCoordinates(
            shapes,
            plasma::Vector<2, float>(
                boxMin.m_elements[0] * rootMatrix[0] + boxMin.m_elements[1] * rootMatrix[4] + rootMatrix[12],
                rootMatrix[1] * boxMin.m_elements[0] + boxMin.m_elements[1] * rootMatrix[5] + rootMatrix[13]) *
                (1 / w),
            scale);
        updateBox();
        plasma::Vector<2, float> difference = boxMin - m_boxMin;
        const plasma::Matrix<float> &matrix = m_workspace->getTextureSheet()->m_root->m_matrix;
        plasma::Vector<2, float> offset(difference[0] * matrix[0] + matrix[4] * difference[1],
                                        matrix[1] * difference[0] + difference[1] * matrix[5]);
        shapes.clear();
        moveTextureCoordinates(shapes, offset);
    } else {
        for (std::map<Node *, NodeInterface *>::const_iterator it = m_workspace->getDesignSheet()->sheetNodesBegin();
             it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown()) {
                it->second->restoreState(it->second->m_state);
                it->second->setSelected(true);
            }
        }
        m_workspace->getDesignSheet()->m_root->updateMatricesRecursively();
        updateBox();
        std::set<plasma::Keyable *> transformations;
        plasma::Vector<2, float> difference = boxMin - m_boxMin;
        const plasma::Matrix<float> &matrix = m_workspace->getOpenDesignSheet()->m_root->m_matrix;
        plasma::Vector<2, float> offset(difference[0] * matrix[0] + matrix[4] * difference[1],
                                        matrix[1] * difference[0] + difference[1] * matrix[5]);
        moveNodes(transformations, offset, 0);
        transformations.clear();
        plasma::Vector<2, float> scale = (boxMax - boxMin) / (m_boxMax - m_boxMin);
        const plasma::Matrix<float> &rootMatrix = m_workspace->getDesignSheet()->m_root->m_matrix;
        float w = rootMatrix[3] * boxMin.m_elements[0] + boxMin.m_elements[1] * rootMatrix[7] + rootMatrix[15];
        scaleNodes(plasma::Vector<2, float>(
                       boxMin.m_elements[0] * rootMatrix[0] + boxMin.m_elements[1] * rootMatrix[4] + rootMatrix[12],
                       rootMatrix[1] * boxMin.m_elements[0] + boxMin.m_elements[1] * rootMatrix[5] + rootMatrix[13]) *
                       (1 / w),
                   scale, transformations, 0);
        m_workspace->getDesignSheet()->m_root->updateMatricesRecursively();
    }
    updateBox();
}

void NodeTool::rotateBox(float angle)
{
    updateBox();
    beginBoxChange();
    std::set<plasma::Keyable *> transformations;
    plasma::Vector<3, float> rotation(0.0f, 0.0f, angle);

    // The middle of the box, mapped by the current sheet's root matrix.
    plasma::Vector<2, float> center = (m_boxMin + m_boxMax) * 0.5f;
    const plasma::Matrix<float> &matrix = m_workspace->getOpenSheet()->m_root->m_matrix;
    float w = matrix[3] * center[0] + matrix[7] * center[1] + matrix[15];
    plasma::Vector<2, float> pivot =
        plasma::Vector<2, float>(center[0] * matrix[0] + matrix[4] * center[1] + matrix[12],
                                 matrix[1] * center[0] + center[1] * matrix[5] + matrix[13]) *
        (1 / w);
    rotateNodes(pivot, rotation, transformations, 0);
    m_workspace->m_nodeModule->showTransformation();
    endBoxChange();
    updateBox();
}
