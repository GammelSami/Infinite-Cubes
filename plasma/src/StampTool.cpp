#include "StampTool.h"
#include "DesignSheet.h"
#include "ClipboardModule.h"
#include "StampModule.h"
#include <GL/glew.h>
#include "plasma/Transformation.h"
#include "plasma/Widget.h"
#include <cstdlib>
#include <cmath>

static const int StampFrequencyDistanceBase = 100;

StampTool::StampTool()
    : Tool(TOOL_STAMP),
      m_unusedFloat(0.0f)
{
    m_matrix.identity();
}

void StampTool::pointerPressed()
{
    if (m_workspace->getDesignSheet() && !m_workspace->m_clipboardModule->m_instances.empty()) {
        m_distance = 0.0f;
        stamp();
    }
}

void StampTool::reset()
{
    m_unusedFloat = 0.0f;
    m_matrix.identity();
}

// Stamps the clipboard trees once at the snapped pointer: the stamp module's rotation, scale and shear with their
// random jitter, centred on the trees' bounds; copies when the module copies, instances otherwise.
void StampTool::stamp()
{
    if (!m_workspace->getDesignSheet() || m_workspace->m_clipboardModule->m_instances.empty())
        return;

    StampModule *module = m_workspace->m_stampModule;
    plasma::Vector<2, float> positionRange(module->m_positionJitter, module->m_positionJitter);
    plasma::Vector<2, float> positionJitter;
    positionJitter[0] = ((double)rand() / RAND_MAX * 2.0 - 1.0) * positionRange[0];
    positionJitter[1] = ((double)rand() / RAND_MAX * 2.0 - 1.0) * positionRange[1];
    float rotationRange = m_workspace->m_stampModule->m_rotationJitter;
    float rotationJitter = ((double)rand() / RAND_MAX * 2.0 - 1.0) * rotationRange;
    float scaleRange = m_workspace->m_stampModule->m_scaleJitter;
    float scaleJitter = ((double)rand() / RAND_MAX * 2.0 - 1.0) * scaleRange + 1.0;

    std::list<plasma::Node *> &nodes = m_workspace->m_stampModule->isCopying()
                                           ? m_workspace->m_clipboardModule->m_nodes
                                           : m_workspace->m_clipboardModule->m_instances;
    module = m_workspace->m_stampModule;
    plasma::Matrix<float> matrix;
    matrix.identity();
    matrix.rotateZ(module->m_rotation + rotationJitter);
    plasma::Vector<2, float> scale = module->m_scale * scaleJitter;
    matrix.scale(scale[0], scale[1]);
    plasma::Vector<2, float> shear = m_workspace->m_stampModule->m_shear;
    shear.applyShear(matrix);

    bool empty = true;
    plasma::Vector<2, float> minimum;
    plasma::Vector<2, float> maximum;
    for (std::list<plasma::Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (m_workspace->m_stampModule->m_shapeBoundary)
            (*it)->extendBounds(minimum, maximum, matrix * plasma::Widget::getPlacement((*it)->m_transformation), empty,
                                true);
        else
            (*it)->extendShapeBounds(minimum, maximum, matrix * plasma::Widget::getPlacement((*it)->m_transformation),
                                     empty);
    }

    // The code assumes the selected clipboard list has at least one node.
    plasma::Vector<2, float> center = (minimum + maximum) * 0.5f;
    minimum = minimum - center;
    maximum = maximum - center;
    matrix.identity();
    m_workspace->collectSnapOffsets();
    plasma::Node *root = m_workspace->getDesignSheet()->getRoot();
    plasma::Vector<2, float> position = root->m_inverseMatrix.transform(m_workspace->getSnappedPointer(true));
    module = m_workspace->m_stampModule;
    position = position - center;
    position = position + positionJitter;
    matrix.translate(position);
    matrix.rotateZ(module->m_rotation + rotationJitter);
    scale = module->m_scale * scaleJitter;
    matrix.scale(scale[0], scale[1]);
    shear = m_workspace->m_stampModule->m_shear;
    plasma::Matrix<float> shearMatrix;
    shearMatrix.identity();
    shearMatrix.m_elements[4] = shear[0];
    shearMatrix.m_elements[1] = shear[1];
    matrix *= shearMatrix;
    bool copying = m_workspace->m_stampModule->isCopying();
    m_workspace->m_clipboardModule->pasteNodes(!copying, matrix, false);
    m_workspace->markDesignSheetChanged();
}

void StampTool::pointerReleased()
{
    if (m_workspace->getDesignSheet()) {
        m_workspace->refreshPointer();
        m_workspace->getDesignSheet()->commit();
    }
}

// The right button resets the stamp matrix to the module's rotation, scale and shear, and anchors the drag at the
// pointer offset by the clipboard bounds' lower corner relative to their centre. The maximum is centred too, but
// that result is unused.
void StampTool::rightButtonPressed()
{
    if (m_workspace->getDesignSheet()) {
        plasma::Vector<2, float> minimum;
        plasma::Vector<2, float> maximum;
        getBounds(minimum, maximum);
        m_matrix.identity();
        m_matrix.rotateZ(m_workspace->m_stampModule->m_rotation);
        m_matrix.scale(m_workspace->m_stampModule->m_scale.m_elements[0],
                       m_workspace->m_stampModule->m_scale.m_elements[1]);
        plasma::Matrix<float> shear;
        shear.identity();
        shear.m_elements[4] = m_workspace->m_stampModule->m_shear.m_elements[0];
        shear.m_elements[1] = m_workspace->m_stampModule->m_shear.m_elements[1];
        m_matrix *= shear;
        plasma::Vector<2, float> center = minimum + maximum;
        center.m_elements[0] *= 0.5;
        center.m_elements[1] *= 0.5;
        minimum = minimum - center;
        Workspace *workspace = m_workspace;
        m_dragOrigin = workspace->getDesignSheet()->m_root->m_inverseMatrix * workspace->m_sceneMousePosition + minimum;
    }
}

void StampTool::pointerMoved(int x, int y)
{
    if (m_workspace->getDesignSheet()) {
        Workspace *workspace = m_workspace;
        if (workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT)) {
            float distance = workspace->m_pointerDeltaY * workspace->m_pointerDeltaY +
                             workspace->m_pointerDeltaX * workspace->m_pointerDeltaX;
            m_distance += std::sqrt(distance);
            if (m_distance > StampFrequencyDistanceBase - workspace->m_stampModule->m_frequency) {
                m_distance = 0.0f;
                stamp();
            }
        }
        if (m_workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT)) {
            bool copying = m_workspace->m_stampModule->isCopying();
            Workspace *workspace = m_workspace;
            std::list<plasma::Node *> &nodes =
                copying ? workspace->m_clipboardModule->m_nodes : workspace->m_clipboardModule->m_instances;
            if (workspace->m_engine.isKeyDown(VK_MENU)) {
                workspace->m_stampModule->setRotation(workspace->m_stampModule->m_rotation +
                                                      workspace->m_pointerDeltaX);
            } else {
                bool empty = true;
                plasma::Vector<2, float> minimum;
                plasma::Vector<2, float> maximum;
                for (std::list<plasma::Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
                    if (m_workspace->m_stampModule->m_shapeBoundary)
                        (*it)->extendBounds(minimum, maximum,
                                            m_matrix * plasma::Widget::getPlacement((*it)->m_transformation), empty,
                                            true);
                    else
                        (*it)->extendShapeBounds(
                            minimum, maximum, m_matrix * plasma::Widget::getPlacement((*it)->m_transformation), empty);
                }
                std::vector<plasma::Vector<2, float>> segments;
                plasma::Vector<2, float> size = maximum - minimum;
                m_workspace->collectSnapSegments(m_workspace->getDesignSheet()->getRoot(), segments);
                m_workspace->collectSnapOffsets();
                plasma::Vector<2, float> origin;
                {
                    plasma::Vector<2, float> point =
                        m_workspace->getDesignSheet()->getRoot()->m_matrix.transformPoint(m_dragOrigin);
                    const plasma::Matrix<float> &toSheet = m_workspace->getDesignSheet()->getRoot()->m_inverseMatrix;
                    origin = toSheet.transformPoint(m_workspace->snapPoint(segments, point));
                }
                plasma::Vector<2, float> corner =
                    origin + (m_workspace->getDesignSheet()->getRoot()->m_inverseMatrix.transformPoint(
                                  m_workspace->getSceneMousePosition()) -
                              origin) *
                                 2.0f;
                {
                    plasma::Vector<2, float> point =
                        m_workspace->getDesignSheet()->getRoot()->m_matrix.transformPoint(corner);
                    const plasma::Matrix<float> &toSheet = m_workspace->getDesignSheet()->getRoot()->m_inverseMatrix;
                    corner = toSheet.transformPoint(m_workspace->snapPoint(segments, point));
                }
                plasma::Vector<2, float> extent = corner - origin;
                if (m_workspace->m_engine.isKeyDown(VK_CONTROL) && std::fabs(size.m_elements[0]) > 0.0f)
                    extent.m_elements[1] = extent.m_elements[0] * size.m_elements[1] / size.m_elements[0];
                plasma::Matrix<float> transform;
                transform.identity();
                transform.rotateZ(-m_workspace->m_stampModule->m_rotation);
                transform.scale(extent.m_elements[0] / size.m_elements[0], extent.m_elements[1] / size.m_elements[1]);
                transform *= m_matrix;
                m_workspace->m_stampModule->setScale(
                    plasma::Vector<2, float>(transform.m_elements[0], transform.m_elements[5]));
                plasma::Vector<2, float> shear(transform.m_elements[4], transform.m_elements[1]);
                if (std::fabs(transform.m_elements[0]) > 1e-9f)
                    shear.m_elements[0] = transform.m_elements[4] / transform.m_elements[0];
                if (std::fabs(transform.m_elements[5]) > 1e-9f)
                    shear.m_elements[1] = transform.m_elements[1] / transform.m_elements[5];
                m_workspace->m_stampModule->setShear(shear);
            }
        }
    }
}

// The bounds of the clipboard trees (copies or instances, as the stamp module stamps them) under the module's
// rotation, scale and shear: their whole bounds with the shape boundary option, else their shapes' bounds. Leaves
// minimum and maximum untouched when the list is empty.
void StampTool::getBounds(plasma::Vector<2, float> &minimum, plasma::Vector<2, float> &maximum)
{
    bool copying = m_workspace->m_stampModule->isCopying();
    std::list<plasma::Node *> &nodes =
        copying ? m_workspace->m_clipboardModule->m_nodes : m_workspace->m_clipboardModule->m_instances;
    StampModule *module = m_workspace->m_stampModule;
    plasma::Matrix<float> matrix;
    matrix.identity();
    matrix.rotateZ(module->m_rotation);
    matrix.scale(module->m_scale.m_elements[0], module->m_scale.m_elements[1]);
    plasma::Matrix<float> shear;
    shear.identity();
    shear.m_elements[4] = m_workspace->m_stampModule->m_shear.m_elements[0];
    shear.m_elements[1] = m_workspace->m_stampModule->m_shear.m_elements[1];
    matrix *= shear;
    bool empty = true;
    for (std::list<plasma::Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (m_workspace->m_stampModule->m_shapeBoundary)
            (*it)->extendBounds(minimum, maximum, matrix * plasma::Widget::getPlacement((*it)->m_transformation), empty,
                                true);
        else
            (*it)->extendShapeBounds(minimum, maximum, matrix * plasma::Widget::getPlacement((*it)->m_transformation),
                                     empty);
    }
}

// The stamp preview: the trees' bounds as a faint rectangle (black, then white, alpha 0.1, with a black outline)
// centred at the snapped pointer, or on the sheet panel's view while the pointer is over the interface, and the
// trees themselves under the stamp module's rotation, scale and shear.
void StampTool::draw()
{
    if (!m_workspace->getDesignSheet() || m_workspace->m_clipboardModule->m_instances.empty())
        return;
    glPushMatrix();
    StampModule *module = m_workspace->m_stampModule;
    std::list<plasma::Node *> &nodes =
        module->isCopying() ? m_workspace->m_clipboardModule->m_nodes : m_workspace->m_clipboardModule->m_instances;
    plasma::Vector<2, float> minimum;
    plasma::Vector<2, float> maximum;
    getBounds(minimum, maximum);
    plasma::Vector<2, float> center = (maximum + minimum) * 0.5f;
    m_workspace->collectSnapOffsets();
    plasma::Node *root = m_workspace->getDesignSheet()->m_root;
    plasma::Vector<2, float> position =
        root->m_inverseMatrix.transformPoint(m_workspace->getSnappedPointer(true)) - center;
    m_workspace->getDesignSheet(); // Calls the getter and discards the result.
    if (m_workspace->m_guiPointerNode) {
        plasma::Vector<2, float> panelCenter = m_workspace->getDesignSheet()->m_panel->getViewSize() * 0.5f +
                                               m_workspace->getDesignSheet()->m_panel->getContentPosition();
        position = m_workspace->getDesignSheet()->m_root->m_inverseMatrix.transformPoint(panelCenter) - center;
    }
    plasma::Matrix<float> matrix = m_workspace->getDesignSheet()->m_root->m_matrix;
    matrix.translate(position);
    module = m_workspace->m_stampModule;
    matrix.rotateZ(module->m_rotation);
    plasma::Vector<2, float> scale = module->m_scale;
    matrix.scale(scale[0], scale[1]);
    plasma::Vector<2, float> shear = m_workspace->m_stampModule->m_shear;
    shear.applyShear(matrix);

    glUseProgram(0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glLoadMatrixf(m_workspace->getDesignSheet()->m_root->m_matrix.m_elements);
    glTranslatef(position.m_elements[0], position.m_elements[1], 0.0f);
    glBegin(GL_QUADS);
    glColor4f(0.0f, 0.0f, 0.0f, 0.1f);
    glVertex2f(minimum.m_elements[0], minimum.m_elements[1]);
    glVertex2f(maximum.m_elements[0], minimum.m_elements[1]);
    glVertex2f(maximum.m_elements[0], maximum.m_elements[1]);
    glVertex2f(minimum.m_elements[0], maximum.m_elements[1]);
    glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
    glVertex2f(minimum.m_elements[0], minimum.m_elements[1]);
    glVertex2f(maximum.m_elements[0], minimum.m_elements[1]);
    glVertex2f(maximum.m_elements[0], maximum.m_elements[1]);
    glVertex2f(minimum.m_elements[0], maximum.m_elements[1]);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glColor4f(0.0f, 0.0f, 0.0f, 0.1f);
    glVertex2f(minimum.m_elements[0], minimum.m_elements[1]);
    glVertex2f(maximum.m_elements[0], minimum.m_elements[1]);
    glVertex2f(maximum.m_elements[0], maximum.m_elements[1]);
    glVertex2f(minimum.m_elements[0], maximum.m_elements[1]);
    glEnd();
    glLoadMatrixf(matrix.m_elements);
    for (std::list<plasma::Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it)
        (*it)->draw(0);
    glPopMatrix();
}

bool StampTool::isRotating()
{
    return m_workspace->m_engine.isKeyDown(VK_MENU);
}

bool StampTool::getSnapOffsets(const plasma::Vector<2, float> &point, std::list<plasma::Vector<2, float>> &offsets)
{
    if (!m_workspace->getDesignSheet())
        return false;
    plasma::Vector<2, float> minimum;
    plasma::Vector<2, float> maximum;

    // The GetBounds leaves these uninitialised when the clipboard is empty.
    getBounds(minimum, maximum);
    Workspace *workspace = m_workspace;
    plasma::Vector<2, float> position;
    position = workspace->getDesignSheet()->m_root->m_inverseMatrix.transformPoint(workspace->m_sceneMousePosition);
    plasma::Vector<2, float> center = (minimum + maximum) * 0.5f;
    plasma::Vector<2, float> offset = center - position;
    maximum -= offset;
    minimum -= offset;
    offsets.push_back(maximum - point);
    offsets.push_back(plasma::Vector<2, float>(maximum.m_elements[0], minimum.m_elements[1]) - point);
    offsets.push_back(plasma::Vector<2, float>(minimum.m_elements[0], maximum.m_elements[1]) - point);
    offsets.push_back(minimum - point);
    return true;
}
