#include "VertexTool.h"
#include <windows.h>
#include <set>
#include "DesignSheet.h"
#include "ShapeInterface.h"
#include "MeshInterface.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/Node.h"
#include "plasma/Shape.h"
#include <GL/glew.h>
#include "plasma/ListWidget.h"
#include "VertexModule.h"

using plasma::Node;

VertexTool::VertexTool()
    : BoundingBoxTool(TOOL_VERTEX)
{}

void VertexTool::draw()
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

// The pointer went down: starts dragging a handle, or else selects the vertices under the pointer; then keeps the
// selected shapes' positions and starts a selection rectangle or lasso (Ctrl held, or the select mode without
// Alt).
void VertexTool::pointerPressed()
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
                    it->second->m_shapeInterface->selectVertices();
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
                    it->second->m_shapeInterface->selectVertices();
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

// The pointer went up: selects the vertices in the selection rectangle or lasso, then tells the shapes and
// commits.
void VertexTool::pointerReleased()
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
                it->second->m_shapeInterface->selectVerticesInBox(this, true);
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
void VertexTool::pointerMoved(int, int)
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

// Empty bounds start with positive minima and negative maxima. Resets the box, grows it over the selected vertices
// of the selected shapes (in the texture sheet's space while it is shown) and places the handles.
void VertexTool::updateBox()
{
    m_boxMin = plasma::Vector<2, float>(1.0e9f, 1.0e8f);
    m_boxMax = plasma::Vector<2, float>(-1.0e9f, -1.0e9f);
    bool first = true;
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
                if (it->second->m_shapeInterface->countSelectedVertices() > 0) {
                    plasma::Vector<2, float> boxMin, boxMax;
                    it->second->m_shapeInterface->getSelectionBounds(&boxMin, &boxMax);
                    if (first) {
                        m_boxMin = boxMin;
                        first = false;
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
                if (it->second->m_shapeInterface->countSelectedVertices() > 0) {
                    it->second->getShapeInterface()->setMatrix(m_workspace->getTextureSheet()->getMatrix());
                    plasma::Vector<2, float> boxMin, boxMax;
                    it->second->m_shapeInterface->getSelectionBounds(&boxMin, &boxMax);
                    it->second->m_shapeInterface->resetMatrix();
                    if (first) {
                        m_boxMin = boxMin;
                        m_boxMax = boxMax;
                        first = false;
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

// The scale box moved or resized the box (ScaleBoxModule): restores the selection's positions, then moves it by
// the minimum corner's offset and scales it about the new minimum corner by the ratio of the sizes.
void VertexTool::setBox(const plasma::Vector<2, float> &boxMin, const plasma::Vector<2, float> &boxMax)
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
void VertexTool::rotateBox(float rotation)
{
    beginBoxChange();
    plasma::Vector<2, float> center = (m_boxMin + m_boxMax) * 0.5f;
    plasma::Vector<2, float> pivot = m_workspace->getOpenSheet()->m_root->m_matrix.transform(center);
    plasma::Vector<3, float> angles(0.0f, 0.0f, rotation);
    if (m_workspace->getDesignSheet()) {
        DesignSheet *sheet = m_workspace->getDesignSheet();
        std::map<Node *, NodeInterface *>::iterator it = sheet->sheetNodesBegin();
        for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it)
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface)
                it->second->m_shapeInterface->rotateSelectedVertices(pivot, angles);
    } else if (m_workspace->getTextureSheet() && m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<Node *, NodeInterface *>::iterator it = sheet->sheetNodesBegin();
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

// "Apply Deformation to Vertices": bakes the deformation into the selected meshes' vertices, one undoable node
// modification each.
void VertexTool::applyDeformation()
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
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface &&
            it->second->m_shapeInterface->m_type == INTERFACE_MESH) {
            MeshInterface *mesh = static_cast<MeshInterface *>(it->second->m_shapeInterface);
            mesh->beginModify();
            it->second->saveState();
            mesh->applyDeformation();
            mesh->endModify();
            it->second->recordModification();
        }
    }
    m_workspace->redraw();
    m_workspace->getOpenDesignSheet()->commit();
}

// "Apply Rotation and Deformation to Vertices", as ApplyDeformation.
void VertexTool::applyRotationAndDeformation()
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
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface &&
            it->second->m_shapeInterface->m_type == INTERFACE_MESH) {
            MeshInterface *mesh = static_cast<MeshInterface *>(it->second->m_shapeInterface);
            mesh->beginModify();
            it->second->saveState();
            mesh->applyRotationAndDeformation();
            mesh->endModify();
            it->second->recordModification();
        }
    }
    m_workspace->redraw();
    m_workspace->getOpenDesignSheet()->commit();
}

// "Soft".
void VertexTool::setSoft()
{
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
            it->second->m_shapeInterface->prepareChildren();
            it->second->m_shapeInterface->smoothVertices();
            it->second->m_shapeInterface->updateChildren();
        }
    }
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->getOpenDesignSheet()->commit();
}

// "Sharp".
void VertexTool::setSharp()
{
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
            it->second->m_shapeInterface->prepareChildren();
            it->second->m_shapeInterface->sharpenVertices();
            it->second->m_shapeInterface->updateChildren();
        }
    }
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->getOpenDesignSheet()->commit();
}

// "Bezier".
void VertexTool::setBezier()
{
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
            it->second->m_shapeInterface->prepareChildren();
            it->second->m_shapeInterface->bezierVertices();
            it->second->m_shapeInterface->updateChildren();
        }
    }
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->getOpenDesignSheet()->commit();
}

// "Arc".
void VertexTool::setArc()
{
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
            it->second->m_shapeInterface->prepareChildren();
            it->second->m_shapeInterface->roundCornerVertices();
            it->second->m_shapeInterface->updateChildren();
        }
    }
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->getOpenDesignSheet()->commit();
}

bool VertexTool::getSnapOffsets(const plasma::Vector<2, float> &point, std::list<plasma::Vector<2, float>> &offsets)
{
    if (m_workspace->m_vertexModule->m_scaleBoxEnabled)
        return BoundingBoxTool::getSnapOffsets(point, offsets);
    return false;
}

bool VertexTool::isRotating()
{
    return (m_workspace->m_engine.isKeyDown(VK_MENU) && m_workspace->m_vertexModule->m_editMode == EDIT_MOVE) ||
           (!m_workspace->m_engine.isKeyDown(VK_MENU) && m_workspace->m_vertexModule->m_editMode == EDIT_ROTATE);
}

void VertexTool::beginBoxChange()
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

void VertexTool::endBoxChange()
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

void VertexTool::fillContextMenu()
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
            count += it->second->m_shapeInterface->countSelectedVertices();
    {
        std::wstring caption(L"Resize Tool Radius...");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::resizeToolRadius, true, true);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Delete");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &VertexTool::deleteSelected, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Soft");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &VertexTool::setSoft, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Sharp");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &VertexTool::setSharp, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Bezier");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &VertexTool::setBezier, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Arc");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &VertexTool::setArc, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Free");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &VertexTool::setFree, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Fixed on edge");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &VertexTool::setFixedOnEdge, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Rounded corner");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &VertexTool::setRoundedCorner, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
}

// Deletes the selected vertices ("Delete"), once per shape shared by several nodes.
void VertexTool::deleteSelected()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
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
            shapes.find(it->first->m_shape) == shapes.end() && it->second->m_shapeInterface->deleteVertices()) {
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

// "Free": clears the upper half of the selected vertices' type words.
void VertexTool::setFree()
{
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
            {
                HalfEdgeMesh::VertexIterator vertex = mesh->m_mesh.beginVertices();
                if (vertex != mesh->m_mesh.endVertices()) {
                    do {
                        if (mesh->isVertexSelected(*vertex)) {
                            mesh->m_mesh.m_shape->m_vertexFlags[mesh->m_mesh.vertexIndex(*vertex)] &=
                                plasma::VERTEX_KIND_MASK;
                            mesh->m_hasChanges = true;
                        }
                        ++vertex;
                    } while (vertex != mesh->m_mesh.endVertices());
                }
            }
        }
    }
    m_workspace->endModifySelected();
}

void VertexTool::setFixedOnEdge()
{
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
            it->second->m_shapeInterface->prepareChildren();

            // Set all type words before recomputing parameters that depend on neighbours.
            for (HalfEdgeMesh::VertexIterator vertex = mesh->m_mesh.beginVertices();
                 vertex != mesh->m_mesh.endVertices(); ++vertex) {
                if (mesh->isVertexSelected(*vertex)) {
                    mesh->m_mesh.vertexFlags(*vertex) &= plasma::VERTEX_KIND_MASK;
                    mesh->m_mesh.vertexFlags(*vertex) |= (1 << plasma::VERTEX_FIXED_ON_EDGE);
                    mesh->m_hasChanges = true;
                }
            }
            for (HalfEdgeMesh::VertexIterator vertex = mesh->m_mesh.beginVertices();
                 vertex != mesh->m_mesh.endVertices(); ++vertex) {
                if (mesh->isVertexSelected(*vertex))
                    mesh->m_mesh.m_shape->updateVertexParameter(mesh->m_mesh.vertexIndex(*vertex));
            }
            mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            mesh->refresh();
            it->second->m_shapeInterface->updateChildren();
        }
    }
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->endModifySelected();
}

void VertexTool::setRoundedCorner()
{
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
            mesh->prepareChildren();
            mesh->beginModify();

            // Set all type words before recomputing parameters that depend on neighbours.
            for (HalfEdgeMesh::VertexIterator vertex = mesh->m_mesh.beginVertices();
                 vertex != mesh->m_mesh.endVertices(); ++vertex) {
                if (mesh->isVertexSelected(*vertex)) {
                    mesh->m_mesh.m_shape->m_vertexFlags[mesh->m_mesh.vertexIndex(*vertex)] &= plasma::VERTEX_KIND_MASK;
                    mesh->m_mesh.m_shape->m_vertexFlags[mesh->m_mesh.vertexIndex(*vertex)] |=
                        (1 << plasma::VERTEX_ROUNDED_CORNER);
                    mesh->m_hasChanges = true;
                }
            }
            for (HalfEdgeMesh::VertexIterator vertex = mesh->m_mesh.beginVertices();
                 vertex != mesh->m_mesh.endVertices(); ++vertex) {
                if (mesh->isVertexSelected(*vertex))
                    mesh->m_mesh.m_shape->updateVertexParameter(mesh->m_mesh.vertexIndex(*vertex));
            }
            mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            mesh->refresh();
            it->second->m_shapeInterface->updateChildren();
        }
    }
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->endModifySelected();
}
