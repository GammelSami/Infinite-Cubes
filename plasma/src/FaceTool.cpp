#include "FaceTool.h"
#include <windows.h>
#include <set>
#include "DesignSheet.h"
#include "ShapeInterface.h"
#include "plasma/Node.h"
#include "plasma/Shape.h"
#include <GL/glew.h>
#include "plasma/ListWidget.h"
#include "VertexModule.h"
#include "MeshInterface.h"
#include "CreateNodeOperation.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/SmoothMeshShape.h"

using plasma::Node;

FaceTool::FaceTool()
    : BoundingBoxTool(TOOL_FACE)
{}

// The pointer went down: starts dragging a handle, or else selects the faces under the pointer; then keeps the
// selected shapes' positions and starts a selection rectangle or lasso (Ctrl held, or the select mode without
// Alt).
void FaceTool::pointerPressed()
{
    if (m_workspace->m_vertexModule->m_scaleBoxEnabled)
        beginHandleDrag();
    if (!isOverHandle() || !m_workspace->m_vertexModule->m_scaleBoxEnabled) {
        if (m_workspace->getDesignSheet()) {
            m_workspace->getDesignSheet()->beginModifySelected();
            DesignSheet *sheet = m_workspace->getDesignSheet();
            std::map<Node *, NodeInterface *>::iterator it;
            if (sheet->isBusy())
                it = sheet->m_sheetNodes.end();
            else
                it = sheet->m_sheetNodes.begin();
            for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it)
                if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface)
                    it->second->m_shapeInterface->selectFaces();
        } else if (m_workspace->getTextureSheet() && m_workspace->getOpenDesignSheet()) {
            m_workspace->getOpenDesignSheet()->beginModifySelected();
            DesignSheet *sheet = m_workspace->getOpenDesignSheet();
            std::map<Node *, NodeInterface *>::iterator it;
            if (sheet->isBusy())
                it = sheet->m_sheetNodes.end();
            else
                it = sheet->m_sheetNodes.begin();
            for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
                if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                    it->second->getShapeInterface()->setMatrix(m_workspace->getTextureSheet()->getMatrix());
                    it->second->m_shapeInterface->selectFaces();
                    it->second->m_shapeInterface->resetMatrix();
                }
            }
        }
    }
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
            it->second->m_shapeInterface->storePositions();
            it->second->m_shapeInterface->prepareChildren();
        }
    }
    Workspace *workspace = m_workspace;
    if (workspace->m_engine.isKeyDown(VK_CONTROL) ||
        (workspace->m_vertexModule->m_editMode == EDIT_SELECT &&
         workspace->m_engine.m_pressedKeys.find(VK_MENU) == workspace->m_engine.m_pressedKeys.end())) {
        if (!isOverHandle() || !m_workspace->m_vertexModule->m_scaleBoxEnabled) {
            m_selectionMode = m_workspace->m_vertexModule->m_selectionMode;
            beginSelection();
        }
    }
    updateBox();
}

// The pointer went up: selects the faces in the selection rectangle or lasso, then tells the shapes and commits.
void FaceTool::pointerReleased()
{
    if (m_selectionMode != SELECTION_NONE && m_workspace->getOpenDesignSheet()) {
        endSelection();
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && !it->first->isLocked() &&
                it->second->m_shapeInterface) {
                if (m_workspace->getTextureSheet()) {
                    it->second->getShapeInterface()->setMatrix(m_workspace->getTextureSheet()->getMatrix());
                }
                it->second->m_shapeInterface->selectFacesInBox(this, true);
                it->second->m_shapeInterface->resetMatrix();
            }
        }
        cancelSelection();
        updateBox();
    }
    refreshChangedShapes();
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->getOpenDesignSheet()->endModifyChanged();
        m_workspace->getOpenDesignSheet()->recordModifiedNodes();
        m_workspace->getOpenDesignSheet()->commit();
    }
}

// The pointer moved with the button held: extends the selection rectangle or lasso; or, dragging a handle, scales
// the selection about the anchor (proportionally with Ctrl); or rotates it in the rotate mode; or else moves it by
// the drag offset.
void FaceTool::pointerMoved(int, int)
{
    if (!m_workspace->getOpenSheet() || !(m_workspace->m_engine.m_buttons & (1 << plasma::MOUSE_BUTTON_LEFT)))
        return;
    if (m_selectionMode == SELECTION_NONE) {
        if (m_dragHandle != NoHandle && m_workspace->m_vertexModule->m_scaleBoxEnabled) {
            plasma::Node *root = m_workspace->getOpenSheet()->getRoot();
            m_dragPosition += root->m_inverseMatrix.transformDirection(m_workspace->getDragMovement());
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
            plasma::Vector<2, float> pivot = m_workspace->getOpenSheet()->getRoot()->m_matrix * m_dragAnchor;
            plasma::Vector<2, float> scale(to.m_elements[0] / from.m_elements[0],
                                           to.m_elements[1] / from.m_elements[1]);
            if (m_workspace->getDesignSheet()) {
                std::map<Node *, NodeInterface *>::iterator it;
                for (it = m_workspace->getDesignSheet()->sheetNodesBegin();
                     it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
                    if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                        it->second->m_shapeInterface->restorePositions();
                        it->second->m_shapeInterface->scaleSelectedVertices(pivot, scale);
                    }
                }
            } else {
                if (m_workspace->getTextureSheet() && m_workspace->getOpenDesignSheet()) {
                    for (std::map<Node *, NodeInterface *>::iterator it =
                             m_workspace->getOpenDesignSheet()->sheetNodesBegin();
                         it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
                        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                            it->second->getShapeInterface()->setMatrix(m_workspace->getTextureSheet()->getMatrix());
                            it->second->m_shapeInterface->restorePositions();
                            it->second->m_shapeInterface->scaleSelectedVertices(pivot, scale);
                            it->second->m_shapeInterface->resetMatrix();
                        }
                    }
                }
                updateBox();
            }
        } else if (isRotating()) {
            plasma::Vector<2, float> pivot =
                m_workspace->getOpenSheet()->getRoot()->m_matrix * m_workspace->m_sheetPointer;
            plasma::Vector<3, float> rotation(0.0f, 0.0f, m_workspace->m_pointerDeltaX * (double)0.4f);
            if (m_workspace->getDesignSheet()) {
                for (std::map<Node *, NodeInterface *>::iterator it = m_workspace->getDesignSheet()->sheetNodesBegin();
                     it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it)
                    if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface)
                        it->second->m_shapeInterface->rotateSelectedVertices(pivot, rotation);
            } else {
                if (m_workspace->getTextureSheet() && m_workspace->getOpenDesignSheet()) {
                    for (std::map<Node *, NodeInterface *>::iterator it =
                             m_workspace->getOpenDesignSheet()->sheetNodesBegin();
                         it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
                        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                            it->second->getShapeInterface()->setMatrix(m_workspace->getTextureSheet()->getMatrix());
                            it->second->m_shapeInterface->rotateSelectedVertices(pivot, rotation);
                            it->second->m_shapeInterface->resetMatrix();
                        }
                    }
                }
                updateBox();
            }
        } else {
            if (m_workspace->getDesignSheet()) {
                for (std::map<Node *, NodeInterface *>::iterator it = m_workspace->getDesignSheet()->sheetNodesBegin();
                     it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it)
                    if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface)
                        it->second->m_shapeInterface->moveSelectedVertices(m_workspace->getDragMovement());
            } else {
                if (m_workspace->getTextureSheet() && m_workspace->getOpenDesignSheet()) {
                    for (std::map<Node *, NodeInterface *>::iterator it =
                             m_workspace->getOpenDesignSheet()->sheetNodesBegin();
                         it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
                        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                            it->second->getShapeInterface()->setMatrix(m_workspace->getTextureSheet()->getMatrix());
                            it->second->m_shapeInterface->moveSelectedVertices(m_workspace->getDragMovement());
                            it->second->m_shapeInterface->resetMatrix();
                        }
                    }
                }
                updateBox();
            }
        }
    } else {
        updateSelection();
    }
}

// Empty bounds start with positive minima and negative maxima. Resets the box, grows it over the selected faces of
// the selected shapes (in the texture sheet's space while it is shown) and places the handles.
void FaceTool::updateBox()
{
    bool first = true;
    m_boxMin.m_elements[0] = 1.0e9f;
    m_boxMin.m_elements[1] = 1.0e8f;
    m_boxMax.m_elements[0] = -1.0e9f;
    m_boxMax.m_elements[1] = -1.0e9f;
    if (m_workspace->getDesignSheet()) {
        DesignSheet *sheet = m_workspace->getDesignSheet();
        std::map<Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && !it->first->isLocked() &&
                it->second->m_shapeInterface) {
                it->second->m_shapeInterface->applyFaceSelection();
                if (it->second->m_shapeInterface->countSelectedVertices() > 0) {
                    plasma::Vector<2, float> boxMin, boxMax;
                    it->second->m_shapeInterface->getSelectionBounds(&boxMin, &boxMax);
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
                it->second->m_shapeInterface->applyFaceSelection();
                if (it->second->m_shapeInterface->countSelectedVertices() > 0) {
                    it->second->getShapeInterface()->setMatrix(m_workspace->getTextureSheet()->getMatrix());
                    plasma::Vector<2, float> boxMin, boxMax;
                    it->second->m_shapeInterface->getSelectionBounds(&boxMin, &boxMax);
                    it->second->m_shapeInterface->resetMatrix();
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
        m_boxMin.transform(m_workspace->getTextureSheet()->m_root->m_inverseMatrix);
        m_boxMax.transform(m_workspace->getTextureSheet()->m_root->m_inverseMatrix);
    }
    updateHandles();
}

// "Detach Faces": each connected group of a mesh's selected faces becomes a new "Mesh" node beside it (a copy of
// the shape keeping only the group's faces), then the selected faces are deleted from the code. A mesh whose faces
// are all selected is left alone.
void FaceTool::detachFaces()
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
        if (!it->first->isShown() || !it->second->m_selected)
            continue;
        ShapeInterface *shapeInterface = it->second->m_shapeInterface;
        if (!shapeInterface || shapeInterface->m_type != INTERFACE_MESH)
            continue;
        MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
        std::list<MeshFace *> faces;
        for (HalfEdgeMesh::FaceIterator face = mesh->m_mesh.beginFaces(); face != mesh->m_mesh.endFaces(); ++face) {
            if (mesh->isFaceSelected(*face))
                faces.push_back(*face);
        }
        if (faces.size() == mesh->m_mesh.getFaceCount())
            continue;
        std::list<std::list<MeshFace *>> groups;
        mesh->m_mesh.getConnectedFaceGroups(faces, groups);
        for (std::list<std::list<MeshFace *>>::iterator group = groups.begin(); group != groups.end(); ++group) {
            plasma::Shape *shape = mesh->m_mesh.m_shape->clone();
            plasma::Shape *copy = shape && shape->getType() == plasma::SHAPE_SMOOTH_MESH ? shape : 0;
            Node *node =
                m_workspace->m_engine.createNode(it->first->m_transformation->clone(), copy,
                                                 it->first->m_display->clone(), it->first->getParent(), L"Mesh");
            NodeInterface *sheetNode = m_workspace->getOpenDesignSheet()->createSheetNode(node);
            sheetNode->setSelected(true);
            ShapeInterface *copyInterface = sheetNode->m_shapeInterface;
            MeshInterface *detached = copyInterface && copyInterface->m_type == INTERFACE_MESH
                                          ? static_cast<MeshInterface *>(copyInterface)
                                          : 0;

            // The copy's faces are in the code's order: drop those outside the group.
            std::set<MeshFace *> kept;
            for (std::list<MeshFace *>::iterator face = group->begin(); face != group->end(); ++face)
                kept.insert(*face);
            std::list<MeshFace *> removed;
            HalfEdgeMesh::FaceIterator copyFace = detached->m_mesh.beginFaces();
            for (HalfEdgeMesh::FaceIterator face = mesh->m_mesh.beginFaces(); face != mesh->m_mesh.endFaces(); ++face) {
                if (kept.find(*face) == kept.end())
                    removed.push_back(*copyFace);
                ++copyFace;
            }
            detached->m_mesh.removeFaces(removed);
            detached->m_mesh.updateShape();
            detached->refresh();
            m_workspace->getOpenDesignSheet()->addOperation(
                new CreateNodeOperation(m_workspace->getOpenDesignSheet(), "create mesh", node));
        }
        mesh->deleteFaces();
    }
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->redraw();
}

// The scale box moved or resized the box (ScaleBoxModule): restores the selection's positions, then moves it by
// the minimum corner's offset and scales it about the new minimum corner by the ratio of the sizes.
void FaceTool::setBox(const plasma::Vector<2, float> &boxMin, const plasma::Vector<2, float> &boxMax)
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it)
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface)
            it->second->m_shapeInterface->restorePositions();
    updateBox();
    plasma::Vector<2, float> offset =
        m_workspace->getOpenSheet()->getRoot()->m_matrix.transformDirection(boxMin - m_boxMin);
    plasma::Vector<2, float> scale(1.0f, 1.0f);
    for (int i = 0; i < 2; ++i) {
        if (fabs(m_boxMax.m_elements[i] - m_boxMin.m_elements[i]) > 1.0e-20f)
            scale.m_elements[i] =
                (boxMax.m_elements[i] - boxMin.m_elements[i]) / (m_boxMax.m_elements[i] - m_boxMin.m_elements[i]);
    }
    plasma::Vector<2, float> pivot = m_workspace->getOpenSheet()->getRoot()->m_matrix.transform(boxMin);
    if (m_workspace->getDesignSheet()) {
        DesignSheet *sheet = m_workspace->getDesignSheet();
        std::map<Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                it->second->m_shapeInterface->moveSelectedVertices(offset);
                it->second->m_shapeInterface->scaleSelectedVertices(pivot, scale);
            }
        }
    } else if (m_workspace->getTextureSheet() && m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                it->second->getShapeInterface()->setMatrix(m_workspace->getTextureSheet()->getMatrix());
                it->second->m_shapeInterface->moveSelectedVertices(offset);
                it->second->m_shapeInterface->scaleSelectedVertices(pivot, scale);
                it->second->m_shapeInterface->resetMatrix();
            }
        }
    }
    updateBox();
}

// Rotates the selected shapes' selection by the angle (degrees) about the box's middle (ScaleBoxModule's
// "scalebox:rotation").
void FaceTool::rotateBox(float rotation)
{
    beginBoxChange();
    plasma::Vector<2, float> sum = m_boxMin + m_boxMax;
    plasma::Vector<2, float> center(sum.m_elements[0] * 0.5, sum.m_elements[1] * 0.5);
    plasma::Vector<2, float> pivot = m_workspace->getOpenSheet()->m_root->m_matrix * center;
    plasma::Vector<3, float> angles(0.0f, 0.0f, rotation);
    if (m_workspace->getDesignSheet()) {
        DesignSheet *sheet = m_workspace->getDesignSheet();
        std::map<Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it)
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface)
                it->second->m_shapeInterface->rotateSelectedVertices(pivot, angles);
    } else if (m_workspace->getTextureSheet() && m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                it->second->getShapeInterface()->setMatrix(m_workspace->getTextureSheet()->getMatrix());
                it->second->m_shapeInterface->rotateSelectedVertices(pivot, angles);
                it->second->m_shapeInterface->resetMatrix();
            }
        }
    }
    updateBox();
    endBoxChange();
}

void FaceTool::draw()
{
    glUseProgram(0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    updateBox();
    if (m_workspace->m_vertexModule->m_scaleBoxEnabled)
        renderBox();
    renderSelection();
}

bool FaceTool::isRotating()
{
    return (m_workspace->m_engine.isKeyDown(VK_MENU) && m_workspace->m_vertexModule->m_editMode == EDIT_MOVE) ||
           (!m_workspace->m_engine.isKeyDown(VK_MENU) && m_workspace->m_vertexModule->m_editMode == EDIT_ROTATE);
}

bool FaceTool::getSnapOffsets(const plasma::Vector<2, float> &point, std::list<plasma::Vector<2, float>> &offsets)
{
    if (m_workspace->m_vertexModule->m_scaleBoxEnabled)
        return BoundingBoxTool::getSnapOffsets(point, offsets);
    return false;
}

void FaceTool::beginBoxChange()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    m_workspace->getOpenDesignSheet()->beginModifySelected();
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
            it->second->m_shapeInterface->storePositions();
            it->second->m_shapeInterface->prepareChildren();
        }
    }
}

void FaceTool::endBoxChange()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    refreshChangedShapes();
    m_workspace->getOpenDesignSheet()->endModifyChanged();
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->getOpenDesignSheet()->commit();
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it)
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface)
            it->second->m_shapeInterface->refresh();
    m_workspace->redraw();
}

void FaceTool::fillContextMenu()
{
    int count = 0;
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it)
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface)
            count += it->second->m_shapeInterface->countSelectedFaces();
    {
        std::wstring caption(L"Delete Faces");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &FaceTool::deleteSelected, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Detach Faces");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &FaceTool::detachFaces, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
}

// Deletes the selected faces ("Delete Faces"), once per shape shared by several nodes.
void FaceTool::deleteSelected()
{
    std::set<void *> shapes; // Shapes already processed, keyed by pointer.
    bool deleted = false;
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface &&
            shapes.find(it->first->m_shape) == shapes.end() && it->second->m_shapeInterface->deleteFaces()) {
            shapes.insert(static_cast<void *>(it->first->m_shape));
            deleted = true;
        }
    }
    if (deleted) {
        m_workspace->getOpenDesignSheet()->rebuildSharedInterfaces();
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->redraw();
    }
}
