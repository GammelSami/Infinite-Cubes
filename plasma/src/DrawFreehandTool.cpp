#include "DrawFreehandTool.h"
#include <windows.h>
#include "ColorModule.h"
#include "DesignSheet.h"
#include "MeshInterface.h"
#include "MeshModule.h"
#include "DrawFreehandModule.h"
#include "TextureModule.h"
#include "CreateNodeOperation.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/Texture.h"
#include <cmath>
#include "StrokeWidthModule.h"
#include <GL/glew.h>

using plasma::Vector;

DrawFreehandTool::DrawFreehandTool()
    : Tool(TOOL_DRAW_FREEHAND),
      m_meshInterface(0),
      m_picking(false)
{
    // The constructor leaves m_savedColor indeterminate until picking.
}

void simplifyPath(float tolerance, const std::vector<Vector<2, float>> &path, std::vector<int> &kept)
{
    int size = (int)path.size();
    std::vector<Vector<2, float>> points(size);
    std::vector<int> flags(size, 0);

    // The callers supply a nonempty path; the empty case is unchecked.
    points[0] = path[0];
    int previous = 0;
    int count = 1;
    for (int i = 1; i < size; ++i) {
        // The code computes this displacement but retains every input point.
        Vector<2, float> displacement = path[i] - path[previous];
        points[count++] = path[i];
        previous = i;
    }
    if (previous < size - 1)
        points[count++] = path[size - 1];
    flags[count - 1] = 1;
    flags[0] = 1;
    simplifyPathSegment(tolerance, points, 0, count - 1, flags);
    kept = flags;
}

void DrawFreehandTool::pointerReleased()
{
    if (!m_workspace->getDesignSheet())
        return;
    m_workspace->getDesignSheet();
    if (m_picking)
        return;

    bool extending = false;
    if (!m_meshInterface) {
        if (m_points.size() < 2)
            return;
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
        shape->m_strokePattern = m_workspace->m_meshModule->m_strokePattern;
        shape->m_strokeDash = m_workspace->m_meshModule->m_strokeDash;
        shape->m_strokeGap = m_workspace->m_meshModule->m_strokeGap;
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
        m_workspace->getDesignSheet()->addOperation(
            new CreateNodeOperation(m_workspace->getDesignSheet(), "create mesh", node));
        m_workspace->moveNode(node, m_workspace->getOpenDesignSheet()->m_editRoot, 0);
        ShapeInterface *shapeInterface = sheetNode->m_shapeInterface;
        m_meshInterface = shapeInterface && shapeInterface->m_type == INTERFACE_MESH
                              ? static_cast<MeshInterface *>(shapeInterface)
                              : 0;
    } else {
        extending = true;
        std::vector<plasma::Vector<2, float>> points;
        std::vector<plasma::Vector<4, float>> strokeColors;
        std::vector<plasma::Vector<4, float>> extrusionFrontColors;
        std::vector<plasma::Vector<4, float>> extrusionBackColors;
        std::vector<plasma::Vector<4, float>> fillColors;
        std::vector<float> widths;
        m_meshInterface->m_node->m_node->updateMatrices();
        plasma::Matrix<float> matrix = m_workspace->getDesignSheet()->getRoot()->getInverseMatrix();
        matrix *= m_meshInterface->m_node->m_node->getMatrix();
        for (HalfEdgeMesh::VertexIterator it = m_meshInterface->m_mesh.beginVertices();
             it != m_meshInterface->m_mesh.endVertices(); ++it) {
            int index = m_meshInterface->vertexIndex(*it);
            plasma::SmoothMeshShape *shape = m_meshInterface->m_mesh.m_shape;
            points.push_back(matrix.transform(shape->m_vertexPositions.value()[index]));
            fillColors.push_back(shape->m_vertexColors.value()[index]);
            strokeColors.push_back(shape->m_strokeColors.value()[index]);
            extrusionFrontColors.push_back(shape->m_extrusionFrontColors.value()[index]);
            extrusionBackColors.push_back(shape->m_extrusionBackColors.value()[index]);
            widths.push_back(shape->m_strokeWidths.value()[index]);
        }
        for (int i = 0; i < (int)m_points.size(); ++i) {
            points.push_back(m_points[i]);
            fillColors.push_back(m_fillColors[i]);
            strokeColors.push_back(m_strokeColors[i]);
            extrusionFrontColors.push_back(m_extrusionFrontColors[i]);
            extrusionBackColors.push_back(m_extrusionBackColors[i]);
            widths.push_back(m_widths[i]);
        }
        m_fillColors = fillColors;
        m_strokeColors = strokeColors;
        m_extrusionFrontColors = extrusionFrontColors;
        m_extrusionBackColors = extrusionBackColors;
        m_widths = widths;
        m_points = points;
        if (m_points.size() < 2)
            return;
        m_meshInterface->beginModify();
        m_meshInterface->m_mesh.clear();
    }

    std::vector<int> kept;
    float tolerance = m_workspace->m_drawFreehandModule->m_simplification;
    tolerance =
        (tolerance / 50.0 + tolerance / 50.0) / m_workspace->getDesignSheet()->getRoot()->getMatrix().m_elements[0];
    simplifyPath(tolerance, m_points, kept);
    m_meshInterface->m_node->m_node->updateMatrices();
    plasma::Matrix<float> matrix = m_meshInterface->m_node->m_node->getInverseMatrix();
    matrix *= m_workspace->getDesignSheet()->getRoot()->getMatrix();
    std::list<MeshVertex *> vertices;
    for (int i = 0; i < (int)m_points.size(); ++i) {
        if (kept[i]) {
            plasma::Vector<2, float> position = matrix.transform(m_points[i]);
            plasma::Vector<2, float> texturePosition = matrix.transform(m_points[i]);
            vertices.push_back(m_meshInterface->m_mesh.addShapeVertex(position, texturePosition, m_fillColors[i],
                                                                      m_strokeColors[i], m_extrusionFrontColors[i],
                                                                      m_extrusionBackColors[i], m_widths[i]));
        }
    }
    MeshFace *face = m_meshInterface->addFace(vertices);
    if (m_workspace->m_drawFreehandModule->m_corners) {
        bool changed = false;
        MeshFaceIterator it(face);
        while (it.isValid()) {
            MeshHalfEdge *edge = it.getEdge();
            plasma::SmoothMeshShape *shape = m_meshInterface->m_mesh.m_shape;
            int index = m_meshInterface->vertexIndex(edge->m_vertex);
            plasma::Vector<2, float> previous =
                shape->m_vertexPositions.value()[m_meshInterface->vertexIndex(edge->m_previous->m_vertex)] -
                shape->m_vertexPositions.value()[index];
            plasma::Vector<2, float> next =
                shape->m_vertexPositions.value()[m_meshInterface->vertexIndex(edge->m_next->m_vertex)] -
                shape->m_vertexPositions.value()[index];
            float lengthSquared =
                previous.m_elements[0] * previous.m_elements[0] + previous.m_elements[1] * previous.m_elements[1];
            if (lengthSquared > 0.0f) {
                float scale = 1.0 / std::sqrt(lengthSquared);
                previous = previous * scale;
            }
            lengthSquared = next.m_elements[0] * next.m_elements[0] + next.m_elements[1] * next.m_elements[1];
            if (lengthSquared > 0.0f) {
                float scale = 1.0 / std::sqrt(lengthSquared);
                next = next * scale;
            }
            float dot = next.m_elements[0] * previous.m_elements[0];
            dot = next.m_elements[1] * previous.m_elements[1] + dot;
            if (dot > -0.3f) {
                shape->m_vertexFlags[index] = plasma::VERTEX_SHARP;
                changed = true;
            }
            ++it;
        }
        if (changed)
            m_meshInterface->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
    m_workspace->redraw();
    m_workspace->getOpenDesignSheet()->m_currentNode = m_meshInterface->m_node->m_node;
    m_workspace->updateModules();
    if (extending)
        m_meshInterface->endModify();
    m_meshInterface = 0;
    m_workspace->getDesignSheet()->commit();
    m_points.clear();
    m_widths.clear();
    m_fillColors.clear();
    m_strokeColors.clear();
    m_extrusionFrontColors.clear();
    m_extrusionBackColors.clear();
}

void DrawFreehandTool::pointerMoved(int x, int y)
{
    Workspace *workspace = m_workspace;
    if (workspace->m_engine.isKeyDown(VK_MENU)) {
        if (!workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT))
            pickColor(true);
    } else {
        if (workspace->getDesignSheet() && m_workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT)) {
            plasma::Node *root = m_workspace->getDesignSheet()->m_root;
            Vector<2, float> point = root->m_inverseMatrix.transform(m_workspace->getSnappedPointer(false));
            m_points.push_back(point);
            float width = m_workspace->m_strokeWidthModule->m_width;
            m_widths.push_back(width);
            workspace = m_workspace;
            DrawFreehandModule *module = workspace->m_drawFreehandModule;
            if (module->m_pressureStroke) {
                m_widths.back() *= workspace->m_tabletPressure;
                if (m_widths.size() == 2)
                    m_widths.front() = m_widths.back();
            }
            float opacity = 1.0f;
            if (module->m_pressureOpacity)
                opacity = workspace->m_tabletPressure;
            Vector<4, float> color = workspace->m_colorModule->m_fillColor;
            color = Vector<4, float>(color[0], color[1], color[2], color[3] * opacity);
            m_fillColors.push_back(color);
            color = m_workspace->m_colorModule->m_strokeColor;
            color = Vector<4, float>(color[0], color[1], color[2], color[3] * opacity);
            m_strokeColors.push_back(color);
            color = m_workspace->m_colorModule->m_extrusionColor;
            color = Vector<4, float>(color[0], color[1], color[2], color[3] * opacity);
            m_extrusionFrontColors.push_back(color);
            color = m_workspace->m_colorModule->m_extrusionColor;
            color = Vector<4, float>(color[0], color[1], color[2], color[3] * opacity);
            m_extrusionBackColors.push_back(color);
            if (m_workspace->m_drawFreehandModule->m_pressureOpacity && m_fillColors.size() == 2) {
                m_fillColors.front().m_elements[3] = m_fillColors.back().m_elements[3];
                m_strokeColors.front().m_elements[3] = m_strokeColors.back().m_elements[3];
                m_extrusionFrontColors.front().m_elements[3] = m_extrusionFrontColors.back().m_elements[3];
                m_extrusionBackColors.front().m_elements[3] = m_extrusionBackColors.back().m_elements[3];
            }
        }
    }
}

void DrawFreehandTool::draw()
{
    if (!m_workspace->m_engine.isKeyDown(VK_MENU) || !m_workspace->getOpenSheet()) {
        if (m_workspace->getDesignSheet()) {
            glUseProgram(0);
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_INDEX_ARRAY);
            MeshInterface *mesh = m_meshInterface;
            if (!mesh && m_workspace->m_engine.isKeyDown(VK_SHIFT) && m_workspace->getDesignSheet()->m_currentNode) {
                plasma::Node *current = m_workspace->getDesignSheet()->m_currentNode;
                NodeInterface *node = m_workspace->getDesignSheet()->findSheetNode(current);
                ShapeInterface *shapeInterface = node->m_shapeInterface;
                mesh = shapeInterface && shapeInterface->m_type == INTERFACE_MESH
                           ? static_cast<MeshInterface *>(shapeInterface)
                           : 0;
            }
            glLineWidth(1.0f);
            if (mesh && (int)mesh->m_mesh.m_shape->m_vertexPositions.m_values[0]->size() > 0) {
                glBegin(GL_LINE_STRIP);
                glColor4fv(m_workspace->m_colorModule->getColor().m_elements);
                const Vector<2, float> &endpoint =
                    mesh->m_mesh.m_shape->m_vertexPositions
                        .value()[mesh->m_mesh.m_shape->m_vertexPositions.m_values[0]->size() - 1];
                plasma::Node *node = mesh->m_node->m_node;
                glVertex2fv((node->m_matrix * endpoint).m_elements);
                if (m_points.empty()) {
                    glVertex2fv(m_workspace->m_sceneMousePosition.m_elements);
                } else {
                    const Vector<2, float> &point = m_points.front();
                    plasma::Node *root = m_workspace->getDesignSheet()->m_root;
                    glVertex2fv((root->m_matrix * point).m_elements);
                }
                glEnd();
            }
            glPushMatrix();
            glLoadMatrixf(m_workspace->getDesignSheet()->m_root->m_matrix.m_elements);
            glBegin(GL_LINE_STRIP);
            glColor4fv(m_workspace->m_colorModule->getColor().m_elements);
            for (int i = 0; i < (int)m_points.size(); ++i)
                glVertex2fv(m_points[i].m_elements);
            glEnd();
            glPopMatrix();
        }
    } else {
        drawIcon(L"icon:pipette", m_workspace->m_sceneMousePosition);
    }
}

void DrawFreehandTool::keyPressed(unsigned short key)
{
    if (key == VK_MENU) {
        m_picking = true;
        m_savedColor = m_workspace->m_colorModule->getColor();
        pickColor(true);
        m_workspace->m_redrawRequested = true;
    }
}

void DrawFreehandTool::keyReleased(unsigned short key)
{
    if (key == VK_MENU && m_picking) {
        m_picking = false;
        m_workspace->m_colorModule->setColor(m_savedColor, true, false, false);
        m_workspace->m_redrawRequested = true;
    }
}
