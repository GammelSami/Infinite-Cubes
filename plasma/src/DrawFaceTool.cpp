#include "DrawFaceTool.h"
#include <windows.h>
#include "DesignSheet.h"
#include "MeshInterface.h"
#include "plasma/ListWidget.h"
#include "plasma/Node.h"
#include "plasma/SmoothMeshShape.h"
#include <GL/glew.h>
#include "NodeInterface.h"
#include "ColorModule.h"
#include "Workspace.h"
#include "MeshModule.h"
#include "TextureModule.h"
#include "StrokeWidthModule.h"
#include "CreateNodeOperation.h"
#include "MeshOperation.h"
#include "plasma/Transformation.h"
#include "plasma/Texture.h"

DrawFaceTool::DrawFaceTool()
    : Tool(TOOL_DRAW_FACE),
      m_meshInterface(0),
      m_face(0)
{}

void DrawFaceTool::restoreDrawingState(MeshInterface *meshInterface, const std::list<MeshVertex *> &vertices,
                                       MeshFace *face)
{
    m_meshInterface = meshInterface;
    m_vertices = vertices;
    m_face = face;
}

// MESH_OPEN marks an open outline.
static inline bool isClosed(unsigned flags)
{
    return !(flags & (1 << plasma::MESH_OPEN));
}

static inline MeshInterface *asMeshInterface(ShapeInterface *shapeInterface)
{
    MeshInterface *meshInterface = static_cast<MeshInterface *>(shapeInterface);
    if (!shapeInterface || shapeInterface->m_type != INTERFACE_MESH)
        meshInterface = 0;
    return meshInterface;
}

// A press adds a vertex to the face being drawn: the nearest mesh vertex within the tool radius (merging its mesh
// into the drawn one), or a new vertex at the pointer. The first press picks the mesh: with Shift the current
// node's, else the snapped vertex's, else a new "Mesh" node.
void DrawFaceTool::pointerPressed()
{
    if (!m_workspace->getDesignSheet())
        return;
    EditableMesh *nearestMesh;
    plasma::Node *nearestNode;
    MeshVertex *nearestVertex;
    float distance;
    m_workspace->findNearestVertex(&nearestMesh, &nearestNode, m_workspace->m_sceneMousePosition, &nearestVertex,
                                   &distance);
    Workspace *workspace = m_workspace;
    if (workspace->m_engine.isKeyDown(VK_SHIFT) && workspace->getDesignSheet()->getCurrentNode()) {
        plasma::Node *current = m_workspace->getDesignSheet()->getCurrentNode();
        ShapeInterface *shapeInterface = m_workspace->getDesignSheet()->findSheetNode(current)->m_shapeInterface;
        if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH)
            m_meshInterface = static_cast<MeshInterface *>(shapeInterface);
    }
    if (!m_meshInterface) {
        m_vertices.clear();
        float radius = m_workspace->m_toolRadius;
        if (distance <= radius) {
            m_meshInterface =
                asMeshInterface(m_workspace->getDesignSheet()->findSheetNode(nearestNode)->m_shapeInterface);
        } else {
            plasma::SmoothMeshShape *shape = m_workspace->m_engine.createSmoothMeshShape(L"Mesh");
            TextureModule *textureModule = m_workspace->m_textureModule;
            if (textureModule->m_texture) {
                shape->m_texture.value() = textureModule->m_texture->m_id;
                shape->m_strokeTexture.value() = m_workspace->m_textureModule->m_texture->m_id;
            }
            shape->setSubdivisions(m_workspace->m_meshModule->m_subdivisions, true);
            shape->m_strokeCapType = m_workspace->m_meshModule->m_strokeCap;
            shape->m_strokeJointType = m_workspace->m_meshModule->m_strokeJoint;
            if (m_workspace->m_meshModule->m_extruded)
                shape->m_flags |= (1 << plasma::MESH_EXTRUDED);
            else
                shape->m_flags &= ~(1 << plasma::MESH_EXTRUDED);
            shape->setStrokePattern(m_workspace->m_meshModule->m_strokePattern);
            shape->setStrokeDash(m_workspace->m_meshModule->m_strokeDash);
            shape->setStrokeGap(m_workspace->m_meshModule->m_strokeGap);
            shape->m_textureOpacity.value() = m_workspace->m_textureModule->m_opacity;
            shape->m_textureBrightness.value() = m_workspace->m_textureModule->m_brightness;
            shape->m_textureContrast.value() = m_workspace->m_textureModule->m_contrast;
            shape->m_textureSaturation.value() = m_workspace->m_textureModule->m_saturation;
            shape->m_strokeTextureOpacity.value() = m_workspace->m_textureModule->m_opacity;
            shape->m_strokeTextureBrightness.value() = m_workspace->m_textureModule->m_brightness;
            shape->m_strokeTextureContrast.value() = m_workspace->m_textureModule->m_contrast;
            shape->m_strokeTextureSaturation.value() = m_workspace->m_textureModule->m_saturation;
            shape->m_strokeTextureStretch.value() = m_workspace->m_textureModule->m_stretch;
            m_workspace->m_meshModule->setShapeModeFlags(shape);
            plasma::Node *node =
                m_workspace->m_engine.createNode(0, shape, 0, m_workspace->getDesignSheet()->getRoot(), L"Mesh");
            NodeInterface *sheetNode = m_workspace->getDesignSheet()->createSheetNode(node);
            sheetNode->createShapeInterface();
            sheetNode->setSelected(true);
            m_workspace->getOpenDesignSheet()->m_currentNode = node;
            m_workspace->updateModules();
            m_workspace->getDesignSheet()->addOperation(
                new CreateNodeOperation(m_workspace->getDesignSheet(), "create mesh", node));
            m_workspace->moveNode(node, m_workspace->getOpenDesignSheet()->m_editRoot, 0);
            m_meshInterface = asMeshInterface(sheetNode->m_shapeInterface);
            m_workspace->redraw();
        }
    }

    MeshState before;
    m_meshInterface->captureState(before);
    float area = m_meshInterface->m_mesh.getSignedArea();
    if (m_face)
        m_meshInterface->removeFace(m_face);
    Workspace *snapWorkspace = m_workspace;
    float toolRadius = snapWorkspace->m_toolRadius;
    if ((!nearestMesh || nearestMesh == &m_meshInterface->m_mesh ||
         nearestMesh->m_shape != m_meshInterface->m_mesh.m_shape) &&
        distance <= toolRadius) {
        // Snapped to an existing vertex: no vertex is added when it would close an edge another face already uses.
        bool add = true;
        if (m_vertices.size()) {
            for (std::list<MeshHalfEdge *>::iterator it = nearestVertex->m_edges.begin();
                 it != nearestVertex->m_edges.end(); ++it) {
                MeshHalfEdge *edge = *it;
                MeshVertex *next = edge->m_next->m_vertex;
                if ((next == m_vertices.front() || next == m_vertices.back()) && edge->m_opposite) {
                    add = false;
                    break;
                }
            }
        }
        if (add) {
            if (nearestMesh != &m_meshInterface->m_mesh) {
                // The vertex's mesh joins the drawn one, in its coordinates.
                MeshInterface *other =
                    asMeshInterface(snapWorkspace->getDesignSheet()->findSheetNode(nearestNode)->m_shapeInterface);
                MeshState otherBefore;
                other->captureState(otherBefore);
                plasma::Matrix<float> matrix = m_meshInterface->m_node->getNode()->m_inverseMatrix;
                matrix *= nearestNode->m_matrix;
                nearestMesh->transform(matrix);
                float otherArea = nearestMesh->getSignedArea();
                MeshState otherAfter;
                other->captureState(otherAfter);
                m_workspace->getDesignSheet()->addOperation(new MeshOperation(
                    m_workspace->getDesignSheet(), "transform frames", other, otherBefore, otherAfter));
                m_meshInterface->mergeMesh(nearestMesh, &nearestVertex, otherArea * area < 0.0, true);
                m_workspace->getDesignSheet()->deleteNode(nearestNode);
                m_workspace->redraw();
            }
            m_vertices.push_back(nearestVertex);
        }
    } else {
        plasma::Node *node = m_meshInterface->m_node->getNode();
        plasma::Vector<2, float> position =
            m_meshInterface->undeform(node->m_inverseMatrix.transformPoint(m_workspace->getSnappedPointer(false)));
        plasma::Matrix<float> identity;
        identity.identity();
        plasma::Vector<2, float> texturePosition = identity * position;
        ColorModule *colorModule = m_workspace->getColorModule();
        MeshVertex *vertex = m_meshInterface->addVertex(
            position, texturePosition, colorModule->m_fillColor, colorModule->m_strokeColor,
            colorModule->m_extrusionColor, colorModule->m_extrusionColor, m_workspace->m_strokeWidthModule->m_width);
        m_vertices.push_back(vertex);

        // With Alt the new vertex is a corner.
        if (m_workspace->m_engine.isKeyDown(VK_MENU))
            m_meshInterface->m_mesh.m_shape->m_vertexFlags[m_meshInterface->vertexIndex(m_vertices.back())] =
                plasma::VERTEX_SHARP;
    }

    m_meshInterface->selectAllVertices(false);
    m_meshInterface->setVertexSelected(m_vertices.back(), true);
    std::list<MeshVertex *> vertices(m_vertices);
    if (!isClosed(m_meshInterface->m_mesh.m_shape->m_flags) && (int)m_meshInterface->m_mesh.getFaceCount() > 0) {
        // An open outline continues its single face: the face's vertices follow the drawn ones and the face is rebuilt.
        MeshFace *face = *m_meshInterface->m_mesh.beginFaces();
        MeshHalfEdge *edge = face->m_edge;
        if (vertices.back() == edge->m_vertex) {
            for (MeshHalfEdge *other = edge->m_next; other != edge; other = other->m_next)
                vertices.push_back(other->m_vertex);
        } else {
            for (MeshHalfEdge *other = edge->m_previous->m_previous; other != edge->m_previous;
                 other = other->m_previous)
                vertices.push_back(other->m_vertex);
        }
        vertices.unique();
        m_vertices = vertices;
        m_meshInterface->m_mesh.removeFace(face);
    }
    if (vertices.size() > 2 && (int)m_meshInterface->m_mesh.getFaceCount() > 0) {
        // Keep the faces' orientation: reverse the loop if its signed area has the other sign than the mesh's.
        float sum = 0.0f;
        for (std::list<MeshVertex *>::iterator it = vertices.begin(); it != vertices.end();) {
            std::list<MeshVertex *>::iterator next = it;
            ++next;
            std::list<MeshVertex *>::iterator following = next == vertices.end() ? vertices.begin() : next;
            plasma::SmoothMeshShape *shape = m_meshInterface->m_mesh.m_shape;
            const plasma::Vector<2, float> &a = shape->m_vertexPositions.value()[m_meshInterface->vertexIndex(*it)];
            const plasma::Vector<2, float> &b =
                shape->m_vertexPositions.value()[m_meshInterface->vertexIndex(*following)];
            sum += b.m_elements[1] * a.m_elements[0] - a.m_elements[1] * b.m_elements[0];
            it = next;
        }
        if (sum * area < 0.0) {
            vertices.reverse();
            vertices.push_back(vertices.front());
            vertices.pop_front();
        }
    }

    // Vertices of other faces that no edge joins to their neighbours in the loop are left out of the face.
    std::list<MeshVertex *> loop(vertices);
    for (std::list<MeshVertex *>::iterator it = loop.begin(); it != loop.end(); ++it) {
        std::list<MeshVertex *>::iterator next = it;
        ++next;
        if (next == loop.end())
            next = loop.begin();
        std::list<MeshVertex *>::iterator previous = it == loop.begin() ? loop.end() : it;
        --previous;
        MeshVertex *vertex = *it;
        bool keep = vertex->m_edges.empty();
        for (std::list<MeshHalfEdge *>::iterator edge = vertex->m_edges.begin(); edge != vertex->m_edges.end();
             ++edge) {
            if ((*edge)->m_next->m_vertex == *previous || (*edge)->m_previous->m_vertex == *next) {
                keep = true;
                break;
            }
        }
        if (!keep)
            vertices.remove(*it);
    }
    if (vertices.size() > 1)
        m_face = m_meshInterface->addFace(vertices);
    else
        m_face = 0;
    if (m_meshInterface->m_mesh.getFaceCount() == 1) {
        plasma::Transformation *transformation = m_meshInterface->m_node->getNode()->m_transformation;
        transformation->setPivot(m_meshInterface->m_mesh.getCenter());
        m_meshInterface->m_node->getNode()->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
    m_workspace->m_meshModule->setShapeModeFlags(m_meshInterface->m_mesh.m_shape);
    m_meshInterface->m_mesh.m_shape->updateSubdivision();
    m_meshInterface->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    m_meshInterface->refresh();
    MeshState after;
    m_meshInterface->captureState(after);
    m_workspace->getDesignSheet()->addOperation(
        new MeshOperation(m_workspace->getDesignSheet(), "draw face", m_meshInterface, before, after));
    m_workspace->getDesignSheet()->rebuildSharedInterfaces();
    m_meshInterface->beginModify();
    if (m_workspace->m_engine.isKeyDown(VK_CONTROL))
        stopDrawing();
}

void DrawFaceTool::pointerReleased()
{
    refreshChangedShapes();
    if (m_meshInterface) {
        m_meshInterface->endModify();
        m_meshInterface->m_hasChanges = false;
    }
    if (m_workspace->getDesignSheet())
        m_workspace->getDesignSheet()->commit();
}

void DrawFaceTool::pointerMoved(int, int)
{
    if ((m_workspace->m_engine.m_buttons & (1 << plasma::MOUSE_BUTTON_LEFT)) && m_meshInterface) {
        m_meshInterface->moveSelectedVertices(m_workspace->getDragMovement());
        m_meshInterface->refresh();
    }
}

// Draws the face so far with a rubber band to the pointer (snapped to the nearest mesh vertex within the tool
// radius), and the pointer as a dot in the current colour when it does not snap. Layout positions, extents and
// spacing use display units; colour tuples style the overlays.
void DrawFaceTool::draw()
{
    if (m_workspace->getDesignSheet()) {
        glUseProgram(0);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_INDEX_ARRAY);
        EditableMesh *nearestMesh;
        plasma::Node *nearestNode;
        MeshVertex *nearestVertex;
        float distance;
        m_workspace->findNearestVertex(&nearestMesh, &nearestNode, m_workspace->m_sceneMousePosition, &nearestVertex,
                                       &distance);
        MeshInterface *nearest = 0;
        if (nearestNode) {
            ShapeInterface *shapeInterface =
                m_workspace->getDesignSheet()->findSheetNode(nearestNode)->m_shapeInterface;
            if (!shapeInterface ||
                (nearest = static_cast<MeshInterface *>(shapeInterface), shapeInterface->m_type != INTERFACE_MESH))
                nearest = 0;
        }
        if (m_meshInterface && m_vertices.size()) {
            glPushMatrix();
            glLoadMatrixf(m_meshInterface->m_node->m_node->m_matrix.m_elements);
            glLineWidth(4.0f);
            glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
            Workspace *workspace = m_workspace;
            plasma::Node *node = m_meshInterface->m_node->m_node;
            plasma::Vector<2, float> end;
            if (distance <= workspace->m_toolRadius)
                end = node->m_inverseMatrix * (nearestNode->m_matrix * nearest->getVertexPosition(nearestVertex));
            else
                end = node->m_inverseMatrix * workspace->getSnappedPointer(false);
            glBegin(GL_LINE_STRIP);
            for (std::list<MeshVertex *>::iterator it = m_vertices.begin(); it != m_vertices.end(); ++it)
                glVertex2fv(m_meshInterface->getVertexPosition(*it).m_elements);
            if (!m_workspace->m_guiPointerNode)
                glVertex2fv(end.m_elements);
            if (isClosed(m_meshInterface->m_mesh.m_shape->m_flags))
                glVertex2fv(m_meshInterface->getVertexPosition(m_vertices.front()).m_elements);
            glEnd();
            glLineWidth(2.0f);
            glColor3f(0.5f, 1.0f, 1.0f);
            glBegin(GL_LINE_STRIP);
            for (std::list<MeshVertex *>::iterator it = m_vertices.begin(); it != m_vertices.end(); ++it)
                glVertex2fv(m_meshInterface->getVertexPosition(*it).m_elements);
            if (!m_workspace->m_guiPointerNode) {
                glVertex2fv(end.m_elements);
                glColor3f(0.7f, 0.7f, 0.7f);
                glVertex2fv(end.m_elements);
            }
            if (isClosed(m_meshInterface->m_mesh.m_shape->m_flags))
                glVertex2fv(m_meshInterface->getVertexPosition(m_vertices.front()).m_elements);
            glEnd();
            glPopMatrix();
        }
        if (!nearestMesh || !(distance <= m_workspace->m_toolRadius)) {
            glPointSize(9.0f);
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_POINTS);
            glVertex2fv(m_workspace->getSnappedPointer(false).m_elements);
            glEnd();
            glPointSize(7.0f);
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_POINTS);
            glVertex2fv(m_workspace->getSnappedPointer(false).m_elements);
            glEnd();
            glPointSize(5.0f);
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_POINTS);
            glVertex2fv(m_workspace->getSnappedPointer(false).m_elements);
            glEnd();
            glPointSize(3.0f);
            glColor3fv(m_workspace->m_colorModule->getColor().m_elements);
            glBegin(GL_POINTS);
            glVertex2fv(m_workspace->getSnappedPointer(false).m_elements);
            glEnd();
        }
    }
}

void DrawFaceTool::deactivate()
{
    stopDrawing();
}

void DrawFaceTool::stopDrawing()
{
    if (m_meshInterface)
        m_meshInterface->selectAllVertices(false);
    m_meshInterface = 0;
    m_face = 0;
    m_vertices.clear();
}

void DrawFaceTool::fillContextMenu()
{
    {
        std::wstring caption(L"Stop Drawing");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &DrawFaceTool::stopDrawing, true, true);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Resize Tool Radius...");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::resizeToolRadius, true, true);
    }
}
