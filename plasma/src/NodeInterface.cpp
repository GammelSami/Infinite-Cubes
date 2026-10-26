// NodeInterface's editing operations; see NodeInterface.h.
#include "NodeInterface.h"
#include "DesignSheet.h"
#include "Operations.h"
#include "ShapeInterface.h"
#include "MeshInterface.h"
#include "TextInterface.h"
#include "plasma/GenericShape.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/TextShape.h"
#include "plasma/Display.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/Widget.h"
#include <GL/glew.h>
#include "Workspace.h"
#include "ToolModule.h"
#include "Tool.h"

using plasma::Node;

// The shape as a mesh, text or generic shape when it is one.
static inline plasma::SmoothMeshShape *asSmoothMeshShape(plasma::Shape *shape)
{
    return shape && shape->getType() == plasma::SHAPE_SMOOTH_MESH ? static_cast<plasma::SmoothMeshShape *>(shape) : 0;
}

static inline plasma::TextShape *asTextShape(plasma::Shape *shape)
{
    return shape && shape->getType() == plasma::SHAPE_TEXT ? static_cast<plasma::TextShape *>(shape) : 0;
}

static inline plasma::GenericShape *asGenericShape(plasma::Shape *shape)
{
    return dynamic_cast<plasma::GenericShape *>(shape);
}

NodeInterface::NodeInterface(DesignSheet *sheet, Node *node)
    : m_sheet(sheet),
      m_expanded(false),
      m_unused439(false),
      m_unused43A(false),
      m_unused43B(false),
      m_selected(false),
      m_modificationPending(false),
      m_node(node),
      m_shapeInterface(0)
{
    createShapeInterface();
}

NodeInterface::~NodeInterface()
{
    delete m_shapeInterface;
}

void NodeInterface::setSelected(bool selected)
{
    m_selected = selected;
}

void NodeInterface::createShapeInterface()
{
    delete m_shapeInterface;
    m_shapeInterface = 0;
    if (asSmoothMeshShape(m_node->m_shape))
        m_shapeInterface = new MeshInterface(this, asSmoothMeshShape(m_node->m_shape));
    if (asTextShape(m_node->m_shape))
        m_shapeInterface = new TextInterface(this, asTextShape(m_node->m_shape));
    if (asGenericShape(m_node->m_shape))
        m_shapeInterface = new GenericInterface(this, asGenericShape(m_node->m_shape));
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void NodeInterface::draw()
{
    // Hidden nodes' matrices are not kept up to date by playback.
    if (!m_node->m_display->m_visibility.value())
        m_node->updateMatrices();
    glPushMatrix();
    glLoadMatrixf(m_node->m_matrix.m_elements);
    if (m_shapeInterface)
        m_shapeInterface->draw();
    for (std::list<Node *>::iterator it = m_node->m_children.begin(); it != m_node->m_children.end(); ++it)
        if ((*it)->isShown() && !(*it)->isInternal() && m_sheet->findSheetNode(*it))
            m_sheet->findSheetNode(*it)->draw();

    // The node tool shows the pivot of the selected nodes (of every selected node while "Set Pivot..." is requested,
    // else of the current node only), and the line from the pivot to the pointer while the pivot is set.
    if (m_sheet->m_workspace->m_toolModule->getCurrentTool()->getType() == Tool::TOOL_NODE && m_selected &&
        (m_sheet->m_currentNode == m_node || m_sheet->m_workspace->m_setPivotRequested)) {
        glPushMatrix();
        glMultMatrixf(m_node->m_transformation->m_inverseMatrix.m_elements);
        glTranslatef(m_node->m_transformation->m_translation.value().m_elements[0],
                     m_node->m_transformation->m_translation.value().m_elements[1], 0.0f);
        if (m_sheet->m_workspace->m_setPivotRequested) {
            plasma::Vector<2, float> &translation = m_node->m_transformation->m_translation.value();
            Node *parent = m_node->m_parent;
            plasma::Vector<2, float> pointer =
                parent->m_inverseMatrix.transform(m_sheet->m_workspace->getSnappedPointer(false)) - translation;
            glLineWidth(4.0f);
            glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
            glBegin(GL_LINES);
            glVertex2fv(m_node->m_transformation->m_pivot.value().m_elements);
            glVertex2fv(pointer.m_elements);
            glEnd();
            glLineWidth(2.0f);
            glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
            glBegin(GL_LINES);
            glVertex2fv(m_node->m_transformation->m_pivot.value().m_elements);
            glVertex2fv(pointer.m_elements);
            glEnd();
        }
        if (m_sheet->m_currentNode == m_node) {
            glPointSize(5.0f);
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_POINTS);
            glVertex2fv(m_node->m_transformation->m_pivot.value().m_elements);
            glEnd();
            glPointSize(3.0f);
            if (m_sheet->m_workspace->m_setPivotRequested)
                glColor3f(0.5f, 0.5f, 0.5f);
            else
                glColor3f(1.0f, 1.0f, 0.0f);
            glBegin(GL_POINTS);
            glVertex2fv(m_node->m_transformation->m_pivot.value().m_elements);
            glEnd();
        }
        glPopMatrix();
    }

    // The widget tool outlines the shown, unlocked widgets, and shows the visible part of the content of the selected
    // one and the one under the pointer (outlined in cyan).
    if (m_sheet->m_workspace->m_toolModule->getCurrentTool()->getType() == Tool::TOOL_WIDGET &&
        m_node->m_display->m_visibility.value() && !m_node->isLocked() && m_node->m_widget) {
        bool hovered = m_sheet->m_workspace->getHoveredNode() &&
                       m_sheet->m_workspace->getHoveredNode()->m_enclosingWidget == m_node->m_widget;
        glPushMatrix();
        glMultMatrixf(m_node->m_widget->m_inverseBindMatrix.m_elements);
        glLineWidth(4.0f);
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);

        // The scroll area's outline: its left, right, top and bottom edges.
        glBegin(GL_LINES);
        glVertex2f(m_node->m_widget->getFramePos()[0], m_node->m_widget->getFramePos()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0],
                   m_node->m_widget->getFramePos()[1] + m_node->m_widget->getFrameSize()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0] + m_node->m_widget->getFrameSize()[0],
                   m_node->m_widget->getFramePos()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0] + m_node->m_widget->getFrameSize()[0],
                   m_node->m_widget->getFramePos()[1] + m_node->m_widget->getFrameSize()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0], m_node->m_widget->getFramePos()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0] + m_node->m_widget->getFrameSize()[0],
                   m_node->m_widget->getFramePos()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0],
                   m_node->m_widget->getFramePos()[1] + m_node->m_widget->getFrameSize()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0] + m_node->m_widget->getFrameSize()[0],
                   m_node->m_widget->getFramePos()[1] + m_node->m_widget->getFrameSize()[1]);
        glEnd();
        if (hovered || m_selected) {
            // The visible part of the content: lines across the scroll area at its left and right, then at its top and bottom.
            glLineWidth(3.0f);
            glBegin(GL_LINES);
            glVertex2f(m_node->m_widget->getScrollPosition()[0], m_node->m_widget->getFramePos()[1]);
            glVertex2f(m_node->m_widget->getScrollPosition()[0],
                       m_node->m_widget->getFramePos()[1] + m_node->m_widget->getFrameSize()[1]);
            glVertex2f(m_node->m_widget->getScrollPosition()[0] + m_node->m_widget->getClampedVisibleExtent()[0],
                       m_node->m_widget->getFramePos()[1]);
            glVertex2f(m_node->m_widget->getScrollPosition()[0] + m_node->m_widget->getClampedVisibleExtent()[0],
                       m_node->m_widget->getFramePos()[1] + m_node->m_widget->getFrameSize()[1]);
            glVertex2f(m_node->m_widget->getFramePos()[0], m_node->m_widget->getScrollPosition()[1]);
            glVertex2f(m_node->m_widget->getFramePos()[0] + m_node->m_widget->getFrameSize()[0],
                       m_node->m_widget->getScrollPosition()[1]);
            glVertex2f(m_node->m_widget->getFramePos()[0],
                       m_node->m_widget->getScrollPosition()[1] + m_node->m_widget->getClampedVisibleExtent()[1]);
            glVertex2f(m_node->m_widget->getFramePos()[0] + m_node->m_widget->getFrameSize()[0],
                       m_node->m_widget->getScrollPosition()[1] + m_node->m_widget->getClampedVisibleExtent()[1]);
            glEnd();
        }
        glLineWidth(2.0f);
        glColor4f(hovered ? 0.0f : 1.0f, 1.0f, 1.0f, 1.0f);
        glBegin(GL_LINES);
        glVertex2f(m_node->m_widget->getFramePos()[0], m_node->m_widget->getFramePos()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0],
                   m_node->m_widget->getFramePos()[1] + m_node->m_widget->getFrameSize()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0] + m_node->m_widget->getFrameSize()[0],
                   m_node->m_widget->getFramePos()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0] + m_node->m_widget->getFrameSize()[0],
                   m_node->m_widget->getFramePos()[1] + m_node->m_widget->getFrameSize()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0], m_node->m_widget->getFramePos()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0] + m_node->m_widget->getFrameSize()[0],
                   m_node->m_widget->getFramePos()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0],
                   m_node->m_widget->getFramePos()[1] + m_node->m_widget->getFrameSize()[1]);
        glVertex2f(m_node->m_widget->getFramePos()[0] + m_node->m_widget->getFrameSize()[0],
                   m_node->m_widget->getFramePos()[1] + m_node->m_widget->getFrameSize()[1]);
        glEnd();
        if (hovered || m_selected) {
            glLineWidth(1.0f);
            glBegin(GL_LINES);
            glVertex2f(m_node->m_widget->getScrollPosition()[0], m_node->m_widget->getFramePos()[1]);
            glVertex2f(m_node->m_widget->getScrollPosition()[0],
                       m_node->m_widget->getFramePos()[1] + m_node->m_widget->getFrameSize()[1]);
            glVertex2f(m_node->m_widget->getScrollPosition()[0] + m_node->m_widget->getClampedVisibleExtent()[0],
                       m_node->m_widget->getFramePos()[1]);
            glVertex2f(m_node->m_widget->getScrollPosition()[0] + m_node->m_widget->getClampedVisibleExtent()[0],
                       m_node->m_widget->getFramePos()[1] + m_node->m_widget->getFrameSize()[1]);
            glVertex2f(m_node->m_widget->getFramePos()[0], m_node->m_widget->getScrollPosition()[1]);
            glVertex2f(m_node->m_widget->getFramePos()[0] + m_node->m_widget->getFrameSize()[0],
                       m_node->m_widget->getScrollPosition()[1]);
            glVertex2f(m_node->m_widget->getFramePos()[0],
                       m_node->m_widget->getScrollPosition()[1] + m_node->m_widget->getClampedVisibleExtent()[1]);
            glVertex2f(m_node->m_widget->getFramePos()[0] + m_node->m_widget->getFrameSize()[0],
                       m_node->m_widget->getScrollPosition()[1] + m_node->m_widget->getClampedVisibleExtent()[1]);
            glEnd();
        }
        glPopMatrix();
    }
    glPopMatrix();
}

void NodeInterface::saveState(NodeState &state)
{
    state.m_name = m_node->getName();
    state.m_translation = m_node->m_transformation->m_translation;
    state.m_deformation = m_node->m_transformation->m_deformation;
    state.m_rotation = m_node->m_transformation->m_rotation;
    state.m_pivot = m_node->m_transformation->m_pivot;
    state.m_visibility = m_node->m_display->m_visibility;
    state.m_clipping = m_node->m_display->m_clipping;
    state.m_fillColor = m_node->m_display->m_fillColor;
    state.m_strokeColor = m_node->m_display->m_strokeColor;
    state.m_flags = m_node->m_flags;
    state.m_displayFlags = m_node->m_display->m_flags;
    state.m_blurRadius = m_node->m_display->m_blurRadius;
    state.m_selected = m_selected;
}

void NodeInterface::restoreState(const NodeState &state)
{
    m_node->setName(state.m_name);
    m_node->m_transformation->m_translation = state.m_translation;
    m_node->m_transformation->m_deformation = state.m_deformation;
    m_node->m_transformation->m_rotation = state.m_rotation;
    m_node->m_transformation->m_pivot = state.m_pivot;
    m_node->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    m_node->m_display->m_visibility = state.m_visibility;
    m_node->m_display->m_clipping = state.m_clipping;
    m_node->m_display->m_fillColor = state.m_fillColor;
    m_node->m_display->m_strokeColor = state.m_strokeColor;
    m_node->m_display->m_blurRadius = state.m_blurRadius;
    m_node->m_display->m_flags = state.m_displayFlags;
    m_node->m_flags = state.m_flags;
    m_selected = state.m_selected;
}

void NodeInterface::beginModifyRecursively()
{
    if (m_shapeInterface)
        m_shapeInterface->beginModify();
    for (std::list<Node *>::iterator it = m_node->m_children.begin(); it != m_node->m_children.end(); ++it)
        if ((*it)->isShown() && !(*it)->m_widget)
            m_sheet->findSheetNode(*it)->beginModifyRecursively();
}

void NodeInterface::endModifyRecursively()
{
    if (m_shapeInterface)
        m_shapeInterface->endModify();
    for (std::list<Node *>::iterator it = m_node->m_children.begin(); it != m_node->m_children.end(); ++it)
        if ((*it)->isShown() && !(*it)->m_widget)
            m_sheet->findSheetNode(*it)->endModifyRecursively();
}

void NodeInterface::saveState()
{
    saveState(m_state);
}

void NodeInterface::recordModification()
{
    NodeState state;
    saveState(state);
    m_sheet->addOperation(new NodeOperation(m_sheet, "modify node", this, m_state, state));
}
