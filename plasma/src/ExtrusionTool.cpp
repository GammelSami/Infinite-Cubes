#include "ExtrusionTool.h"
#include "DesignSheet.h"
#include "MeshInterface.h"
#include "plasma/Node.h"
#include "plasma/ListWidget.h"
#include <GL/glew.h>
#include "plasma/SmoothMeshShape.h"
#include <set>
#include <windows.h>

using plasma::Matrix;
using plasma::Vector;

ExtrusionTool::ExtrusionTool()
    : BoundingBoxTool(TOOL_EXTRUSION)
{}

void ExtrusionTool::pointerPressed()
{
    if (!m_workspace->getDesignSheet())
        return;
    m_workspace->getOpenDesignSheet()->beginModifySelected();
    updateBox();
    beginHandleDrag();
    DesignSheet *sheet = m_workspace->getDesignSheet();
    std::map<plasma::Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it)
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface &&
            it->second->m_shapeInterface->m_type == INTERFACE_MESH)
            static_cast<MeshInterface *>(it->second->m_shapeInterface)->storeExtrusionMatrix();
}

void ExtrusionTool::pointerReleased()
{
    refreshChangedShapes();
    if (m_workspace->getDesignSheet()) {
        m_workspace->getDesignSheet()->endModifyChanged();
        m_workspace->getDesignSheet()->commit();
    }
}

void ExtrusionTool::rightButtonReleased()
{
    pointerReleased();
}

void ExtrusionTool::rightButtonPressed()
{
    pointerPressed();
}

// The pointer moved with a button held: dragging a handle scales the extrusion of the selected meshes about the
// anchor (proportionally with Ctrl); otherwise the drag moves it. Each shared shape is changed once.
void ExtrusionTool::pointerMoved(int, int)
{
    if (!m_workspace->getDesignSheet())
        return;
    if (!m_workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) &&
        !m_workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT))
        return;
    if (m_dragHandle != NoHandle) {
        Vector<2, float> movement =
            m_workspace->getOpenSheet()->getRoot()->m_inverseMatrix.transformDirection(m_workspace->getDragMovement());
        m_dragPosition.m_elements[0] += movement.m_elements[0];
        m_dragPosition.m_elements[1] += movement.m_elements[1];
        Vector<2, float> from = m_dragStart - m_dragAnchor;
        Vector<2, float> to = m_dragPosition - m_dragAnchor;
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
            float ratioX = to.m_elements[0] / from.m_elements[0];
            float ratioY = to.m_elements[1] / from.m_elements[1];
            float ratio = ratioX < ratioY ? ratioY : ratioX;
            to = from * ratio;
        }
        Vector<2, float> pivot = m_workspace->getDesignSheet()->getRoot()->m_matrix * m_dragAnchor;
        std::set<plasma::SmoothMeshShape *> shapes;
        DesignSheet *sheet = m_workspace->getDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it =
            sheet->isBusy() ? sheet->m_sheetNodes.end() : sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface &&
                it->second->m_shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(it->second->m_shapeInterface);
                plasma::SmoothMeshShape *shape = mesh->m_mesh.m_shape;
                if (shapes.find(shape) == shapes.end()) {
                    shapes.insert(shape);
                    mesh->restoreExtrusionMatrix();
                    Matrix<float> matrix = it->first->m_matrix;
                    matrix *= mesh->m_mesh.m_shape->m_extrusionMatrix.value();
                    Matrix<float> inverse = matrix;
                    inverse.invert();
                    mesh->m_mesh.m_shape->m_extrusionMatrix.value() *= inverse;
                    mesh->m_mesh.m_shape->m_extrusionMatrix.value().translate(pivot);
                    Vector<2, float> scale = to / from;
                    mesh->m_mesh.m_shape->m_extrusionMatrix.value().scale(scale.m_elements[0], scale.m_elements[1]);
                    Vector<2, float> back = pivot;
                    back.m_elements[0] *= -1.0;
                    back.m_elements[1] *= -1.0;
                    mesh->m_mesh.m_shape->m_extrusionMatrix.value().translate(back);
                    mesh->m_mesh.m_shape->m_extrusionMatrix.value() *= matrix;
                    mesh->m_hasChanges = true;
                    mesh->m_refreshPending = true;
                }
            }
        }
        updateBox();
    } else {
        std::set<plasma::SmoothMeshShape *> shapes;
        DesignSheet *sheet = m_workspace->getDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it =
            sheet->isBusy() ? sheet->m_sheetNodes.end() : sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface &&
                it->second->m_shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(it->second->m_shapeInterface);
                plasma::SmoothMeshShape *shape = mesh->m_mesh.m_shape;
                if (shapes.find(shape) == shapes.end()) {
                    shapes.insert(shape);
                    Matrix<float> matrix = it->first->m_matrix;
                    matrix *= mesh->m_mesh.m_shape->m_extrusionMatrix.value();
                    Matrix<float> inverse = matrix;
                    inverse.invert();
                    Vector<2, float> movement = inverse.transformDirection(m_workspace->getDragMovement());
                    mesh->m_mesh.m_shape->m_extrusionMatrix.value().translate(movement);
                    mesh->m_hasChanges = true;
                    mesh->m_refreshPending = true;
                }
            }
        }
    }
}

// The code sums the selected vertices' counts without using the sum.
void ExtrusionTool::fillContextMenu()
{
    int count = 0;
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<plasma::Node *, NodeInterface *>::iterator it;
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
    std::wstring caption(L"Reset Extrusion Deformation");
    plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
    item->setPopUpItem(true, true);
    item->setCaption(caption, true);
    item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &ExtrusionTool::resetDeformation, true, true);
}

// Empty bounds start with positive minima and negative maxima.
void ExtrusionTool::updateBox()
{
    m_boxMin = Vector<2, float>(1.0e9f, 1.0e8f);
    m_boxMax = Vector<2, float>(-1.0e9f, -1.0e9f);

    // The code shares this flag across the meshes in the selection.
    bool first = true;
    if (m_workspace->getDesignSheet()) {
        DesignSheet *sheet = m_workspace->getDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && !it->first->isLocked() &&
                it->second->m_shapeInterface && it->second->m_shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(it->second->m_shapeInterface);
                Vector<2, float> minimum(1.0e9f, 1.0e8f);
                Vector<2, float> maximum(-1.0e9f, -1.0e8f);
                Matrix<float> matrix = it->first->m_matrix;
                matrix *= mesh->m_mesh.m_shape->m_extrusionMatrix.value();
                for (HalfEdgeMesh::VertexIterator vertex = mesh->m_mesh.beginVertices();
                     vertex != mesh->m_mesh.endVertices(); ++vertex) {
                    Vector<2, float> point = matrix.transform(mesh->getVertexPosition(*vertex));
                    if (first) {
                        first = false;
                        minimum = point;
                        maximum = point;
                    } else {
                        if (point.m_elements[0] < minimum.m_elements[0])
                            minimum.m_elements[0] = point.m_elements[0];
                        if (point.m_elements[1] < minimum.m_elements[1])
                            minimum.m_elements[1] = point.m_elements[1];
                        if (point.m_elements[0] > maximum.m_elements[0])
                            maximum.m_elements[0] = point.m_elements[0];
                        if (point.m_elements[1] > maximum.m_elements[1])
                            maximum.m_elements[1] = point.m_elements[1];
                    }
                }
                if (minimum.m_elements[0] < m_boxMin.m_elements[0])
                    m_boxMin.m_elements[0] = minimum.m_elements[0];
                if (minimum.m_elements[1] < m_boxMin.m_elements[1])
                    m_boxMin.m_elements[1] = minimum.m_elements[1];
                if (maximum.m_elements[0] > m_boxMax.m_elements[0])
                    m_boxMax.m_elements[0] = maximum.m_elements[0];
                if (maximum.m_elements[1] > m_boxMax.m_elements[1])
                    m_boxMax.m_elements[1] = maximum.m_elements[1];
            }
        }
        m_boxMin.transform(m_workspace->getDesignSheet()->getRoot()->m_inverseMatrix);
        m_boxMax.transform(m_workspace->getDesignSheet()->getRoot()->m_inverseMatrix);
    }
    updateHandles();
}

void ExtrusionTool::draw()
{
    glUseProgram(0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    updateBox();
    renderBox();
}

void ExtrusionTool::resetDeformation()
{
    if (m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && !it->first->isLocked() &&
                it->second->m_shapeInterface && it->second->m_shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(it->second->m_shapeInterface);
                mesh->beginModify();
                Matrix<float> identity;
                identity.identity();

                // The code uses indeterminate bounds when the mesh has no vertices.
                Vector<2, float> minimum, maximum;
                bool first = true;
                mesh->m_mesh.m_shape->getVertexBounds(minimum, maximum, identity, first);
                Vector<2, float> center = maximum + minimum;
                center.m_elements[0] *= 0.5;
                center.m_elements[1] *= 0.5;
                Matrix<float> &matrix = mesh->m_mesh.m_shape->m_extrusionMatrix.value();
                Vector<2, float> offset = matrix * center;
                matrix.identity();
                offset = offset - center;
                matrix.translate(offset);
                mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                mesh->refresh();
                mesh->endModify();
            }
        }
        m_workspace->getOpenDesignSheet()->commit();
    }
}
