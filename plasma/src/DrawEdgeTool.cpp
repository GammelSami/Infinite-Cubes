#include "DrawEdgeTool.h"
#include "plasma/ListWidget.h"
#include <set>
#include "DesignSheet.h"
#include "MeshInterface.h"
#include "NodeInterface.h"
#include "plasma/Node.h"
#include "plasma/SmoothMeshShape.h"
#include <GL/glew.h>
#include "MeshOperation.h"

DrawEdgeTool::DrawEdgeTool()
    : Tool(TOOL_DRAW_EDGE),
      m_mesh(0),
      m_vertex(0)
{}

// The dot product, summed in a float (as in Workspace.cpp).
static inline float dot(const plasma::Vector<2, float> &a, const plasma::Vector<2, float> &b)
{
    float result = a.m_elements[0] * b.m_elements[0];
    result += a.m_elements[1] * b.m_elements[1];
    return result;
}

// The squared length, as one expression.
static inline float squaredLength(const plasma::Vector<2, float> &v)
{
    return v.m_elements[0] * v.m_elements[0] + v.m_elements[1] * v.m_elements[1];
}

// A click connects: on a vertex within the tool radius, the vertex becomes the current one, and when it shares a
// face with the previous current vertex of the same mesh, that face is split between the two; on an edge within
// the radius, a vertex is inserted at the pointer's projection on the edge (its attributes interpolated in every
// frame), and the face it shares with the current vertex is split between them. Elsewhere drawing stops.
void DrawEdgeTool::pointerPressed()
{
    EditableMesh *nearestMesh;
    plasma::Node *nearestNode;
    MeshVertex *nearestVertex;
    float distance;
    m_workspace->findNearestVertex(&nearestMesh, &nearestNode, m_workspace->m_sceneMousePosition, &nearestVertex,
                                   &distance);
    if (nearestVertex && distance <= m_workspace->getToolRadius()) {
        ShapeInterface *shapeInterface =
            m_workspace->getOpenDesignSheet()->findSheetNode(nearestNode)->m_shapeInterface;
        MeshInterface *mesh;
        if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH)
            mesh = static_cast<MeshInterface *>(shapeInterface);
        else
            mesh = 0;
        if (mesh == m_mesh && m_vertex) {
            std::set<MeshFace *> faces;
            for (std::list<MeshHalfEdge *>::iterator it = m_vertex->m_edges.begin(); it != m_vertex->m_edges.end();
                 ++it)
                faces.insert((*it)->m_face);
            bool shareFace = false;
            for (std::list<MeshHalfEdge *>::iterator it = nearestVertex->m_edges.begin();
                 it != nearestVertex->m_edges.end(); ++it) {
                if (faces.find((*it)->m_face) != faces.end() && (*it)->m_next->m_vertex != m_vertex &&
                    (*it)->m_previous->m_vertex != m_vertex) {
                    shareFace = true;
                    break;
                }
            }
            if (shareFace) {
                MeshState before;
                mesh->captureState(before);
                std::list<MeshVertex *> vertices;
                vertices.push_back(m_vertex);
                vertices.push_back(nearestVertex);
                nearestMesh->connectVertices(vertices);
                nearestMesh->updateShape();
                m_vertex = nearestVertex;
                MeshState after;
                mesh->captureState(after);
                m_workspace->getOpenDesignSheet()->addOperation(
                    new MeshOperation(m_workspace->getOpenDesignSheet(), "connect vertices", mesh, before, after));
                m_workspace->getOpenDesignSheet()->commit();
                m_workspace->getOpenDesignSheet()->rebuildSharedInterfaces();
                mesh->refresh();
            } else {
                m_vertex = nearestVertex;
                m_mesh = mesh;
            }
        } else {
            m_mesh = mesh;
            m_vertex = nearestVertex;
        }
        return;
    }
    MeshHalfEdge *nearestEdge;
    m_workspace->findNearestEdge(&nearestMesh, &nearestNode, m_workspace->m_sceneMousePosition, &nearestEdge,
                                 &distance);
    if (!nearestEdge || !(distance <= m_workspace->getToolRadius())) {
        stopDrawing();
        return;
    }
    ShapeInterface *shapeInterface = m_workspace->getOpenDesignSheet()->findSheetNode(nearestNode)->m_shapeInterface;
    MeshInterface *mesh;
    if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH)
        mesh = static_cast<MeshInterface *>(shapeInterface);
    else
        mesh = 0;
    if (m_workspace->getTextureSheet())
        mesh->setMatrix(m_workspace->getTextureSheet()->getMatrix());
    plasma::Matrix<float> matrix;
    mesh->getMatrix(matrix);
    MeshState before;
    mesh->captureState(before);
    MeshVertex *first = nearestEdge->m_vertex;
    MeshVertex *second = nearestEdge->m_next->m_vertex;
    plasma::Vector<2, float> start = matrix * mesh->getVertexPosition(first);
    plasma::Vector<2, float> end = matrix * mesh->getVertexPosition(second);
    plasma::Vector<2, float> offset = m_workspace->getSnappedPointer(false) - start;
    plasma::Vector<2, float> direction = end - start;
    float fraction = dot(offset, direction) / squaredLength(direction);
    mesh->resetMatrix();
    mesh->prepareChildren();
    plasma::SmoothMeshShape *shape = nearestMesh->m_shape;
    MeshVertex *vertex = nearestMesh->addShapeVertex(
        mesh->undeform(mesh->getVertexPosition(first) * (1.0 - fraction) + mesh->getVertexPosition(second) * fraction),
        shape->m_vertexTexCoords.value()[nearestMesh->vertexIndex(first)] * (1.0 - fraction) +
            shape->m_vertexTexCoords.value()[nearestMesh->vertexIndex(second)] * fraction,
        shape->m_vertexColors.value()[nearestMesh->vertexIndex(first)] * (1.0 - fraction) +
            shape->m_vertexColors.value()[nearestMesh->vertexIndex(second)] * fraction,
        shape->m_strokeColors.value()[nearestMesh->vertexIndex(first)] * (1.0 - fraction) +
            shape->m_strokeColors.value()[nearestMesh->vertexIndex(second)] * fraction,
        shape->m_extrusionFrontColors.value()[nearestMesh->vertexIndex(first)] * (1.0 - fraction) +
            shape->m_extrusionFrontColors.value()[nearestMesh->vertexIndex(second)] * fraction,
        shape->m_extrusionBackColors.value()[nearestMesh->vertexIndex(first)] * (1.0 - fraction) +
            shape->m_extrusionBackColors.value()[nearestMesh->vertexIndex(second)] * fraction,
        shape->m_strokeWidths.value()[nearestMesh->vertexIndex(second)] * fraction +
            shape->m_strokeWidths.value()[nearestMesh->vertexIndex(first)] * (1.0 - fraction));
    nearestMesh->splitEdge(nearestEdge, vertex);
    nearestMesh->updateShape();

    // The new vertex also interpolates the animation frames.
    for (int i = 0; i < nearestMesh->m_shape->m_vertexPositions.size(); ++i)
        (*nearestMesh->m_shape->m_vertexPositions.m_values[i])[nearestMesh->vertexIndex(vertex)] = mesh->undeform(
            mesh->deform((*nearestMesh->m_shape->m_vertexPositions.m_values[i])[nearestMesh->vertexIndex(first)]) *
                (1.0 - fraction) +
            mesh->deform((*nearestMesh->m_shape->m_vertexPositions.m_values[i])[nearestMesh->vertexIndex(second)]) *
                fraction);
    for (int i = 0; i < nearestMesh->m_shape->m_vertexColors.size(); ++i) {
        std::vector<plasma::Vector<4, float>> &colors = *nearestMesh->m_shape->m_vertexColors.m_values[i];
        colors[nearestMesh->vertexIndex(vertex)] = colors[nearestMesh->vertexIndex(first)] * (1.0 - fraction) +
                                                   colors[nearestMesh->vertexIndex(second)] * fraction;
    }
    for (int i = 0; i < nearestMesh->m_shape->m_vertexTexCoords.size(); ++i) {
        std::vector<plasma::Vector<2, float>> &texCoords = *nearestMesh->m_shape->m_vertexTexCoords.m_values[i];
        texCoords[nearestMesh->vertexIndex(vertex)] = texCoords[nearestMesh->vertexIndex(first)] * (1.0 - fraction) +
                                                      texCoords[nearestMesh->vertexIndex(second)] * fraction;
    }
    for (int i = 0; i < nearestMesh->m_shape->m_strokeColors.size(); ++i) {
        std::vector<plasma::Vector<4, float>> &colors = *nearestMesh->m_shape->m_strokeColors.m_values[i];
        colors[nearestMesh->vertexIndex(vertex)] = colors[nearestMesh->vertexIndex(first)] * (1.0 - fraction) +
                                                   colors[nearestMesh->vertexIndex(second)] * fraction;
    }
    for (int i = 0; i < nearestMesh->m_shape->m_strokeWidths.size(); ++i) {
        std::vector<float> &widths = *nearestMesh->m_shape->m_strokeWidths.m_values[i];
        float width = widths[nearestMesh->vertexIndex(second)] * fraction +
                      widths[nearestMesh->vertexIndex(first)] * (1.0 - fraction);
        widths[nearestMesh->vertexIndex(vertex)] = width;
    }
    if (mesh == m_mesh && m_vertex) {
        std::set<MeshFace *> faces;
        for (std::list<MeshHalfEdge *>::iterator it = m_vertex->m_edges.begin(); it != m_vertex->m_edges.end(); ++it)
            faces.insert((*it)->m_face);
        for (std::list<MeshHalfEdge *>::iterator it = vertex->m_edges.begin(); it != vertex->m_edges.end(); ++it) {
            if (faces.find((*it)->m_face) != faces.end() && (*it)->m_next->m_vertex != m_vertex &&
                (*it)->m_previous->m_vertex != m_vertex) {
                nearestMesh->splitFace(m_vertex, vertex, (*it)->m_face);
                nearestMesh->updateShape();
                break;
            }
        }
        m_vertex = vertex;
    } else {
        m_vertex = vertex;
        m_mesh = mesh;
    }
    MeshState after;
    mesh->captureState(after);
    mesh->updateChildren();
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->getOpenDesignSheet()->addOperation(
        new MeshOperation(m_workspace->getOpenDesignSheet(), "connect vertices", mesh, before, after));
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->getOpenDesignSheet()->rebuildSharedInterfaces();
    mesh->refresh();
}

void DrawEdgeTool::rightButtonPressed()
{
    stopDrawing();
}

void DrawEdgeTool::deactivate()
{
    stopDrawing();
}

void DrawEdgeTool::stopDrawing()
{
    m_mesh = 0;
    m_vertex = 0;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Marks the split
// point: the vertex nearest to the pointer, or else the point of the nearest edge closest to it, in the vertex's
// (the edge's interpolated) colour; with a line from the current vertex when the two share a face.
void DrawEdgeTool::draw()
{
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
            m_workspace->getOpenDesignSheet()->findSheetNode(nearestNode)->m_shapeInterface;
        if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH)
            nearest = static_cast<MeshInterface *>(shapeInterface);
        else
            nearest = 0;
    }
    bool shareFace = false;
    bool found = false;
    float radius = m_workspace->m_toolRadius;
    plasma::Vector<2, float> position;
    plasma::Vector<4, float> color;
    if (nearestVertex && distance <= radius) {
        if (m_vertex) {
            std::set<MeshFace *> faces;
            for (std::list<MeshHalfEdge *>::iterator it = m_vertex->m_edges.begin(); it != m_vertex->m_edges.end();
                 ++it)
                faces.insert((*it)->m_face);
            for (std::list<MeshHalfEdge *>::iterator it = nearestVertex->m_edges.begin();
                 it != nearestVertex->m_edges.end(); ++it) {
                if (faces.find((*it)->m_face) != faces.end() && (*it)->m_next->m_vertex != m_vertex &&
                    (*it)->m_previous->m_vertex != m_vertex) {
                    shareFace = true;
                    break;
                }
            }
        }
        found = true;
        if (m_workspace->getTextureSheet())
            nearest->setMatrix(m_workspace->getTextureSheet()->getMatrix());
        position = nearest->getVertexPosition(nearestVertex);
        color = nearestMesh->m_shape->m_vertexColors.value()[nearestMesh->vertexIndex(nearestVertex)];
        nearest->resetMatrix();
    } else {
        MeshHalfEdge *nearestEdge;
        m_workspace->findNearestEdge(&nearestMesh, &nearestNode, m_workspace->m_sceneMousePosition, &nearestEdge,
                                     &distance);
        if (nearestNode) {
            ShapeInterface *shapeInterface =
                m_workspace->getOpenDesignSheet()->findSheetNode(nearestNode)->m_shapeInterface;
            if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH)
                nearest = static_cast<MeshInterface *>(shapeInterface);
            else
                nearest = 0;
        }
        if (nearestEdge && distance <= m_workspace->m_toolRadius) {
            found = true;
            if (m_vertex) {
                for (std::list<MeshHalfEdge *>::iterator it = m_vertex->m_edges.begin(); it != m_vertex->m_edges.end();
                     ++it) {
                    if (nearestEdge->m_face == (*it)->m_face ||
                        (nearestEdge->m_opposite && nearestEdge->m_opposite->m_face == (*it)->m_face)) {
                        shareFace = true;
                        break;
                    }
                }
            }
            MeshVertex *first = nearestEdge->m_vertex;
            MeshVertex *second = nearestEdge->m_next->m_vertex;
            if (m_workspace->getTextureSheet())
                nearest->setMatrix(m_workspace->getTextureSheet()->getMatrix());
            plasma::Matrix<float> matrix;
            nearest->getMatrix(matrix);
            plasma::Vector<2, float> start = matrix.transformPoint(nearest->getVertexPosition(first));
            plasma::Vector<2, float> end = matrix.transformPoint(nearest->getVertexPosition(second));
            plasma::Vector<2, float> offset = m_workspace->getSnappedPointer(false) - start;
            plasma::Vector<2, float> direction = end - start;
            float dot = offset.m_elements[0] * direction.m_elements[0];
            dot += offset.m_elements[1] * direction.m_elements[1];
            float lengthSquared =
                direction.m_elements[0] * direction.m_elements[0] + direction.m_elements[1] * direction.m_elements[1];
            float fraction = dot / lengthSquared;
            position =
                nearest->getVertexPosition(first) * (1.0f - fraction) + nearest->getVertexPosition(second) * fraction;
            color = nearestMesh->m_shape->m_vertexColors.value()[nearestMesh->vertexIndex(first)] * (1.0f - fraction) +
                    nearestMesh->m_shape->m_vertexColors.value()[nearestMesh->vertexIndex(second)] * fraction;
            nearest->resetMatrix();
        }
    }
    if (nearest) {
        if (m_workspace->getTextureSheet())
            nearest->setMatrix(m_workspace->getTextureSheet()->getMatrix());
        plasma::Matrix<float> matrix;
        nearest->getMatrix(matrix);
        glPushMatrix();
        glLoadMatrixf(matrix.m_elements);
        if (shareFace) {
            glLineWidth(4.0f);
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_LINE_STRIP);
            glVertex2fv(nearest->getVertexPosition(m_vertex).m_elements);
            glVertex2fv(position.m_elements);
            glEnd();
            glLineWidth(2.0f);
            glColor3f(0.5f, 1.0f, 1.0f);
            glBegin(GL_LINE_STRIP);
            glVertex2fv(nearest->getVertexPosition(m_vertex).m_elements);
            glVertex2fv(position.m_elements);
            glEnd();
        }
        if (found) {
            glPointSize(9.0f);
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_POINTS);
            glVertex2fv(position.m_elements);
            glEnd();
            glPointSize(7.0f);
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_POINTS);
            glVertex2fv(position.m_elements);
            glEnd();
            glPointSize(5.0f);
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_POINTS);
            glVertex2fv(position.m_elements);
            glEnd();
            glPointSize(3.0f);
            glColor3fv(color.m_elements);
            glBegin(GL_POINTS);
            glVertex2fv(position.m_elements);
            glEnd();
        }
        glPopMatrix();
        nearest->resetMatrix();
    }
}

void DrawEdgeTool::fillContextMenu()
{
    {
        std::wstring caption(L"Stop Drawing");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &DrawEdgeTool::stopDrawing, true, true);
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
