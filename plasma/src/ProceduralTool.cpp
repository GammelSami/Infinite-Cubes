#include "ProceduralTool.h"
#include <windows.h>
#include "DesignSheet.h"
#include "ProceduralShapeModule.h"
#include "ColorModule.h"
#include "CreateNodeOperation.h"
#include "MeshModule.h"
#include "StrokeWidthModule.h"
#include "TextureModule.h"
#include <GL/glew.h>
#include "plasma/Node.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/Texture.h"
#include "plasma/Transformation.h"
#include "plasma/Widget.h"
#include <cmath>

static const int PreviewEllipseSegments = 50;

ProceduralTool::ProceduralTool()
    : Tool(TOOL_PROCEDURAL),
      m_angle(0.0f),
      m_refreshPreview(false)
{
    m_previewShape = g_workspace.m_engine.createSmoothMeshShape(L"procedural:preview");
    m_previewNode = g_workspace.m_engine.createNode(0, m_previewShape, 0, 0, L"");
}

void ProceduralTool::pointerPressed()
{
    if (m_workspace->getDesignSheet()) {
        Workspace *workspace = m_workspace;
        m_angle = workspace->m_proceduralShapeModule->m_angle;
        plasma::Vector<2, float> halfSize(workspace->m_proceduralShapeModule->m_width,
                                          workspace->m_proceduralShapeModule->m_height);
        halfSize.m_elements[0] *= 0.5;
        halfSize.m_elements[1] *= 0.5;
        m_dragOrigin =
            workspace->getDesignSheet()->m_root->m_inverseMatrix.transformPoint(workspace->m_sceneMousePosition) -
            halfSize;
    }
}

// Releasing places a copy of the preview shape at the snapped pointer, selects it, records "create mesh" and moves
// it under the edit root.
void ProceduralTool::pointerReleased()
{
    if (m_workspace->getDesignSheet()) {
        plasma::Node *node = m_previewNode->clone(m_workspace->getDesignSheet()->getRoot());
        node->setName(L"Shape");
        m_workspace->collectSnapOffsets();
        plasma::Widget::getTranslation(node->m_transformation) =
            m_workspace->getDesignSheet()->getRoot()->m_inverseMatrix.transformPoint(
                m_workspace->getSnappedPointer(true));
        node->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        node->updateMatrices();
        NodeInterface *sheetNode = m_workspace->getDesignSheet()->createSheetNode(node);
        sheetNode->createShapeInterface();
        sheetNode->setSelected(true);
        m_workspace->getDesignSheet()->addOperation(
            new CreateNodeOperation(m_workspace->getDesignSheet(), "create mesh", node));
        m_workspace->moveSelectedNodes(node, m_workspace->getDesignSheet()->m_editRoot);
        m_workspace->getOpenDesignSheet()->m_currentNode = node;
        m_workspace->updateModules();
        m_workspace->getDesignSheet()->commit();
    }
}

void ProceduralTool::rightButtonPressed()
{
    Workspace *workspace = m_workspace;
    m_angle = workspace->m_proceduralShapeModule->m_angle;
    plasma::Vector<2, float> halfSize(workspace->m_proceduralShapeModule->m_width,
                                      workspace->m_proceduralShapeModule->m_height);
    halfSize.m_elements[0] *= 0.5;
    halfSize.m_elements[1] *= 0.5;
    const plasma::Matrix<float> &matrix = workspace->getDesignSheet()->m_root->m_inverseMatrix;
    const plasma::Vector<2, float> &point = workspace->m_sceneMousePosition;
    float w =
        point.m_elements[1] * matrix.m_elements[7] + point.m_elements[0] * matrix.m_elements[3] + matrix.m_elements[15];
    plasma::Vector<2, float> transformed(matrix.m_elements[4] * point.m_elements[1] +
                                             point.m_elements[0] * matrix.m_elements[0] + matrix.m_elements[12],
                                         point.m_elements[1] * matrix.m_elements[5] +
                                             point.m_elements[0] * matrix.m_elements[1] + matrix.m_elements[13]);
    m_dragOrigin = transformed * (1 / w) - halfSize;
}

// Dragging out a procedural shape. The drag origin and the opposite corner are snapped to the sheet's snapable
// edges in sheet space.
void ProceduralTool::pointerMoved(int x, int y)
{
    if (m_workspace->getDesignSheet()) {
        Workspace *workspace = m_workspace;
        if (workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) ||
            workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT)) {
            if (workspace->m_engine.isKeyDown(VK_MENU)) {
                m_angle += workspace->m_pointerDeltaX;
                workspace->m_proceduralShapeModule->setAngle(m_angle);
            } else {
                float stickiness = workspace->m_cursorStickiness;
                if (stickiness < 0.0f) {
                    workspace->collectSnapOffsets();
                    plasma::Vector<2, float> origin = m_dragOrigin;
                    std::vector<plasma::Vector<2, float>> segments;
                    m_workspace->collectSnapSegments(m_workspace->getDesignSheet()->getRoot(), segments);
                    {
                        const plasma::Matrix<float> &toScene = m_workspace->getDesignSheet()->getRoot()->getMatrix();
                        const plasma::Matrix<float> &toSheet =
                            m_workspace->getDesignSheet()->getRoot()->getInverseMatrix();
                        origin = toSheet * m_workspace->snapPoint(segments, toScene * origin);
                    }
                    plasma::Vector<2, float> corner =
                        origin + (m_workspace->getDesignSheet()->getRoot()->getInverseMatrix() *
                                      m_workspace->getSceneMousePosition() -
                                  origin) *
                                     2.0f;
                    {
                        const plasma::Matrix<float> &toScene = m_workspace->getDesignSheet()->getRoot()->getMatrix();
                        const plasma::Matrix<float> &toSheet =
                            m_workspace->getDesignSheet()->getRoot()->getInverseMatrix();
                        corner = toSheet * m_workspace->snapPoint(segments, toScene * corner);
                    }
                    m_workspace->m_proceduralShapeModule->setSize(corner - origin);
                }
            }
        }
    }
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void ProceduralTool::draw()
{
    if (!m_workspace->getDesignSheet())
        return;

    // Regenerate the preview when the mesh, colour, stroke width or texture settings no longer match it (the panel's
    // own changes set the flag through InvalidatePreview).
    bool contour = false;
    bool mesh = true;
    bool stroke = false;
    bool closed = true;
    switch (m_workspace->m_meshModule->m_shapeMode) {
    case MeshModule::SHAPE_MODE_CONTOUR:
        contour = true;
        break;
    case MeshModule::SHAPE_MODE_MESH:
        break;
    case MeshModule::SHAPE_MODE_CONTOUR_STROKE:
        contour = true;
        stroke = true;
        break;
    case MeshModule::SHAPE_MODE_MESH_STROKE:
        stroke = true;
        break;
    case MeshModule::SHAPE_MODE_STROKE:
        mesh = false;
        stroke = true;
        break;
    case MeshModule::SHAPE_MODE_OPEN_STROKE:
        mesh = false;
        closed = false;
        stroke = true;
        break;
    }
    if (m_workspace->m_proceduralShapeModule->m_kind != ProceduralShapeModule::KIND_SPIRAL) {
        if ((m_previewShape->m_flags & (1 << plasma::MESH_CONTOUR)) != contour ||
            plasma::SmoothMeshShape::hasFill(m_previewShape->m_flags) != mesh ||
            plasma::SmoothMeshShape::hasStroke(m_previewShape->m_flags) != stroke ||
            !(m_previewShape->m_flags & (1 << plasma::MESH_OPEN)) != closed)
            m_refreshPreview = true;
    }
    MeshModule *meshModule = m_workspace->m_meshModule;
    plasma::SmoothMeshShape *shape = m_previewShape;
    if (shape->getStrokePattern() != meshModule->m_strokePattern ||
        shape->getStrokeCapType() != meshModule->m_strokeCap ||
        shape->getStrokeJointType() != meshModule->m_strokeJoint ||
        shape->getStrokeDash() != meshModule->m_strokeDash || shape->getStrokeGap() != meshModule->m_strokeGap ||
        plasma::SmoothMeshShape::hasExtrusion(shape->m_flags) != meshModule->m_extruded ||
        shape->getSubdivisions() != meshModule->m_subdivisions)
        m_refreshPreview = true;
    if ((int)shape->m_vertexColors.m_values[0]->size() > 0 &&
        shape->m_vertexColors.value()[0] != m_workspace->getColorModule()->m_fillColor)
        m_refreshPreview = true;
    if ((int)shape->m_strokeColors.m_values[0]->size() > 0 &&
        shape->m_strokeColors.value()[0] != m_workspace->getColorModule()->m_strokeColor)
        m_refreshPreview = true;
    if ((int)shape->m_strokeWidths.m_values[0]->size() > 0 &&
        shape->m_strokeWidths.value()[0] != m_workspace->m_strokeWidthModule->m_width)
        m_refreshPreview = true;
    if (m_workspace->m_textureModule->m_mode == TextureModule::TEXTURE_FILL) {
        if (shape->m_fillTexture != m_workspace->m_textureModule->m_texture)
            m_refreshPreview = true;
        if (shape->m_textureOpacity.value() != m_workspace->m_textureModule->m_opacity)
            m_refreshPreview = true;
        if (shape->m_textureBrightness.value() != m_workspace->m_textureModule->m_brightness)
            m_refreshPreview = true;
        if (shape->m_textureContrast.value() != m_workspace->m_textureModule->m_contrast)
            m_refreshPreview = true;
        if (shape->m_textureSaturation.value() != m_workspace->m_textureModule->m_saturation)
            m_refreshPreview = true;
    } else {
        if (shape->m_outlineTexture != m_workspace->m_textureModule->m_texture)
            m_refreshPreview = true;
        if (shape->m_strokeTextureOpacity.value() != m_workspace->m_textureModule->m_opacity)
            m_refreshPreview = true;
        if (shape->m_strokeTextureBrightness.value() != m_workspace->m_textureModule->m_brightness)
            m_refreshPreview = true;
        if (shape->m_strokeTextureContrast.value() != m_workspace->m_textureModule->m_contrast)
            m_refreshPreview = true;
        if (shape->m_strokeTextureSaturation.value() != m_workspace->m_textureModule->m_saturation)
            m_refreshPreview = true;
    }
    if (shape->m_strokeTextureStretch.value() != m_workspace->m_textureModule->m_stretch)
        m_refreshPreview = true;

    if (m_refreshPreview) {
        // The texture settings of the panel's target, the other texture off.
        shape->m_texture.value() =
            m_workspace->m_textureModule->m_texture ? m_workspace->m_textureModule->m_texture->m_id : -1;
        if (m_workspace->m_textureModule->m_mode == TextureModule::TEXTURE_FILL) {
            m_previewShape->m_texture.value() =
                m_workspace->m_textureModule->m_texture ? m_workspace->m_textureModule->m_texture->m_id : -1;
            m_previewShape->m_strokeTexture.value() = -1;
            m_previewShape->m_textureOpacity.value() = m_workspace->m_textureModule->m_opacity;
            m_previewShape->m_textureBrightness.value() = m_workspace->m_textureModule->m_brightness;
            m_previewShape->m_textureContrast.value() = m_workspace->m_textureModule->m_contrast;
            m_previewShape->m_textureSaturation.value() = m_workspace->m_textureModule->m_saturation;
        } else {
            m_previewShape->m_strokeTexture.value() =
                m_workspace->m_textureModule->m_texture ? m_workspace->m_textureModule->m_texture->m_id : -1;
            m_previewShape->m_texture.value() = -1;
            m_previewShape->m_strokeTextureOpacity.value() = m_workspace->m_textureModule->m_opacity;
            m_previewShape->m_strokeTextureBrightness.value() = m_workspace->m_textureModule->m_brightness;
            m_previewShape->m_strokeTextureContrast.value() = m_workspace->m_textureModule->m_contrast;
            m_previewShape->m_strokeTextureSaturation.value() = m_workspace->m_textureModule->m_saturation;
        }
        m_previewShape->m_strokeTextureStretch.value() = m_workspace->m_textureModule->m_stretch;

        // The shape fills the larger side; the node's deformation stretches it to the width and height.
        m_previewNode->m_transformation->m_deformation.value().identity();
        ProceduralShapeModule *module = m_workspace->m_proceduralShapeModule;
        if (module->m_kind != ProceduralShapeModule::KIND_RECTANGULAR) {
            float size = max(module->m_width, module->m_height);
            float width = module->m_width;
            float height = module->m_height;
            m_previewNode->m_transformation->m_deformation.value().scale(width / size, height / size);
        }
        m_previewNode->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        m_workspace->m_proceduralShapeModule->generateShape(m_previewShape);
        m_previewNode->m_transformation->m_rotation.value().m_elements[2] =
            m_workspace->m_proceduralShapeModule->m_angle;
        m_previewNode->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        m_refreshPreview = false;
    }

    glPushMatrix();
    glLoadMatrixf(m_workspace->getDesignSheet()->m_root->m_matrix.m_elements);
    m_workspace->collectSnapOffsets();

    // At the pointer, or at the view's centre while a popup is open.
    const plasma::Matrix<float> &matrix = m_workspace->getDesignSheet()->m_root->m_inverseMatrix;
    const plasma::Vector<2, float> &point = m_workspace->getSnappedPointer(true);
    float w =
        matrix.m_elements[7] * point.m_elements[1] + matrix.m_elements[3] * point.m_elements[0] + matrix.m_elements[15];
    float x =
        matrix.m_elements[4] * point.m_elements[1] + point.m_elements[0] * matrix.m_elements[0] + matrix.m_elements[12];
    float y =
        matrix.m_elements[5] * point.m_elements[1] + matrix.m_elements[1] * point.m_elements[0] + matrix.m_elements[13];
    w = 1 / w;
    plasma::Vector<2, float> position(x * w, w * y);
    if (m_workspace->m_guiPointerNode) {
        plasma::Vector<2, float> center = m_workspace->getDesignSheet()->m_panel->getViewSize();
        center.m_elements[0] *= 0.5;
        center.m_elements[1] *= 0.5;
        plasma::Vector<2, float> content = m_workspace->getDesignSheet()->m_panel->getContentPosition();
        position = m_workspace->getDesignSheet()->m_root->m_inverseMatrix.transform(center + content);
    }
    glTranslatef(position.m_elements[0], position.m_elements[1], 0.0f);

    // The faint outline of the shape's box (an ellipse for the round kinds).
    glPushMatrix();
    glUseProgram(0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glRotatef(m_workspace->m_proceduralShapeModule->m_angle, 0.0f, 0.0f, 1.0f);
    glLineWidth(1.0f);
    glEnable(GL_LINE_SMOOTH);
    if (m_workspace->m_proceduralShapeModule->m_kind != ProceduralShapeModule::KIND_RECTANGULAR) {
        plasma::Vector<2, float> radius(m_workspace->m_proceduralShapeModule->m_width,
                                        m_workspace->m_proceduralShapeModule->m_height);
        radius.m_elements[0] *= 0.5;
        radius.m_elements[1] *= 0.5;
        glColor4f(0.0f, 0.0f, 0.0f, 0.1f);
        glBegin(GL_TRIANGLE_FAN);
        for (int i = 0; i <= PreviewEllipseSegments; i++) {
            float angle = i * 6.283185307179586 / (double)PreviewEllipseSegments;
            plasma::Vector<2, float> vertex;
            vertex.m_elements[0] = std::cos(angle) * radius.m_elements[0];
            vertex.m_elements[1] = std::sin(angle) * radius.m_elements[1];
            glVertex2fv(vertex.m_elements);
        }
        glEnd();
        glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
        glBegin(GL_TRIANGLE_FAN);
        for (int i = 0; i <= PreviewEllipseSegments; i++) {
            float angle = i * 6.283185307179586 / (double)PreviewEllipseSegments;
            plasma::Vector<2, float> vertex;
            vertex.m_elements[0] = std::cos(angle) * radius.m_elements[0];
            vertex.m_elements[1] = std::sin(angle) * radius.m_elements[1];
            glVertex2fv(vertex.m_elements);
        }
        glEnd();
        glColor4f(0.0f, 0.0f, 0.0f, 0.1f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i <= PreviewEllipseSegments; i++) {
            float angle = i * 6.283185307179586 / (double)PreviewEllipseSegments;
            plasma::Vector<2, float> vertex;
            vertex.m_elements[0] = std::cos(angle) * radius.m_elements[0];
            vertex.m_elements[1] = std::sin(angle) * radius.m_elements[1];
            glVertex2fv(vertex.m_elements);
        }
    } else {
        glBegin(GL_QUADS);
        glColor4f(0.0f, 0.0f, 0.0f, 0.1f);
        glVertex2f(m_workspace->m_proceduralShapeModule->m_width * -0.5,
                   m_workspace->m_proceduralShapeModule->m_height * -0.5);
        glVertex2f(m_workspace->m_proceduralShapeModule->m_width * 0.5,
                   m_workspace->m_proceduralShapeModule->m_height * -0.5);
        glVertex2f(m_workspace->m_proceduralShapeModule->m_width * 0.5,
                   m_workspace->m_proceduralShapeModule->m_height * 0.5);
        glVertex2f(m_workspace->m_proceduralShapeModule->m_width * -0.5,
                   m_workspace->m_proceduralShapeModule->m_height * 0.5);
        glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
        glVertex2f(m_workspace->m_proceduralShapeModule->m_width * -0.5,
                   m_workspace->m_proceduralShapeModule->m_height * -0.5);
        glVertex2f(m_workspace->m_proceduralShapeModule->m_width * 0.5,
                   m_workspace->m_proceduralShapeModule->m_height * -0.5);
        glVertex2f(m_workspace->m_proceduralShapeModule->m_width * 0.5,
                   m_workspace->m_proceduralShapeModule->m_height * 0.5);
        glVertex2f(m_workspace->m_proceduralShapeModule->m_width * -0.5,
                   m_workspace->m_proceduralShapeModule->m_height * 0.5);
        glEnd();
        glBegin(GL_LINE_LOOP);
        glColor4f(0.0f, 0.0f, 0.0f, 0.1f);
        glVertex2f(m_workspace->m_proceduralShapeModule->m_width * -0.5,
                   m_workspace->m_proceduralShapeModule->m_height * -0.5);
        glVertex2f(m_workspace->m_proceduralShapeModule->m_width * 0.5,
                   m_workspace->m_proceduralShapeModule->m_height * -0.5);
        glVertex2f(m_workspace->m_proceduralShapeModule->m_width * 0.5,
                   m_workspace->m_proceduralShapeModule->m_height * 0.5);
        glVertex2f(m_workspace->m_proceduralShapeModule->m_width * -0.5,
                   m_workspace->m_proceduralShapeModule->m_height * 0.5);
    }
    glEnd();
    glPopMatrix();

    m_previewNode->draw(0);

    // While a popup is open, the preview's control polygon and vertices (coloured by vertex type).
    if (m_workspace->m_guiPointerNode) {
        glUseProgram(0);
        glDisable(GL_DEPTH_TEST);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_INDEX_ARRAY);
        glDisable(GL_TEXTURE_2D);
        glMultMatrixf(m_previewNode->m_transformation->m_matrix.m_elements);
        glLineWidth(3.0f);
        glBegin(GL_LINE_LOOP);
        glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
        for (int i = 0; i < (int)m_previewShape->m_vertexPositions.m_values[0]->size(); i++)
            glVertex2fv(m_previewShape->m_vertexPositions.value()[i].m_elements);
        glEnd();
        glLineWidth(1.0f);
        glBegin(GL_LINE_LOOP);
        glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
        for (int i = 0; i < (int)m_previewShape->m_vertexPositions.m_values[0]->size(); i++)
            glVertex2fv(m_previewShape->m_vertexPositions.value()[i].m_elements);
        glEnd();
        glPointSize(4.0f);
        glBegin(GL_POINTS);
        glColor3f(0.0f, 0.0f, 0.0f);
        for (int i = 0; i < (int)m_previewShape->m_vertexPositions.m_values[0]->size(); i++)
            glVertex2fv(m_previewShape->m_vertexPositions.value()[i].m_elements);
        glEnd();
        glPointSize(2.0f);
        glBegin(GL_POINTS);
        for (int i = 0; i < (int)m_previewShape->m_vertexPositions.m_values[0]->size(); i++) {
            switch (m_previewShape->m_vertexFlags[i]) {
            case plasma::VERTEX_SMOOTH:
                glColor3f(1.0f, 1.0f, 1.0f);
                break;
            case plasma::VERTEX_SHARP:
                glColor3f(0.25f, 0.25f, 0.25f);
                break;
            case plasma::VERTEX_BEZIER:
                glColor3f(0.5f, 0.5f, 1.0f);
                break;
            case plasma::VERTEX_ROUND:
                glColor3f(1.0f, 0.5f, 0.5f);
                break;
            }
            glVertex2fv(m_previewShape->m_vertexPositions.value()[i].m_elements);
        }
        glEnd();
    }
    glPopMatrix();
}

void ProceduralTool::invalidatePreview()
{
    m_refreshPreview = true;
}

bool ProceduralTool::getSnapOffsets(const plasma::Vector<2, float> &point, std::list<plasma::Vector<2, float>> &offsets)
{
    if (!m_workspace->getDesignSheet())
        return false;
    plasma::Vector<2, float> halfSize(m_workspace->m_proceduralShapeModule->m_width,
                                      m_workspace->m_proceduralShapeModule->m_height);
    halfSize.m_elements[0] *= 0.5;
    halfSize.m_elements[1] *= 0.5;
    plasma::Vector<2, float> minimum =
        m_workspace->getDesignSheet()->m_root->m_inverseMatrix.transform(m_workspace->m_sceneMousePosition) - halfSize;
    halfSize = plasma::Vector<2, float>(m_workspace->m_proceduralShapeModule->m_width,
                                        m_workspace->m_proceduralShapeModule->m_height);
    halfSize.m_elements[0] *= 0.5;
    halfSize.m_elements[1] *= 0.5;
    plasma::Vector<2, float> maximum =
        m_workspace->getDesignSheet()->m_root->m_inverseMatrix.transform(m_workspace->m_sceneMousePosition) + halfSize;
    offsets.push_back(minimum - point);
    offsets.push_back(plasma::Vector<2, float>(minimum.m_elements[0], maximum.m_elements[1]) - point);
    offsets.push_back(plasma::Vector<2, float>(maximum.m_elements[0], minimum.m_elements[1]) - point);
    offsets.push_back(maximum - point);
    return true;
}

bool ProceduralTool::isRotating()
{
    return m_workspace->m_engine.isKeyDown(VK_MENU);
}
