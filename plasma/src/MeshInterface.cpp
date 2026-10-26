// MeshInterface.cpp: the editor's mesh editing interface (see MeshInterface.h).
#include "plasma/PointerTypes.h"
#include "MeshInterface.h"
#include "SequenceModule.h"
#include "TextureModule.h"
#include "DesignSheet.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/Widget.h"
#include "plasma/SmoothMeshShape.h"
#include "Module.h"
#include "ToolModule.h"
#include "DrawFaceTool.h"
#include "NodeToolModule.h"
#include "StrokeWidthModule.h"
#include "ColorModule.h"
#include "ColorToolModule.h"
#include "Operations.h"
#include <GL/glew.h>
#include <GL/glu.h>
#include <algorithm>
#include "Concurrency.h"
#include <cmath>
#include <map>

// A contour whose last segment is not left out (MESH_OPEN clear).
static inline bool isClosed(unsigned flags)
{
    return !(flags & (1 << plasma::MESH_OPEN));
}

// Sets or clears one of the mesh shape's flags (the bits MeshModule's shape modes use).
static inline void setShapeFlag(plasma::SmoothMeshShape *shape, unsigned flag, bool set)
{
    if (set)
        shape->m_flags |= flag;
    else
        shape->m_flags &= ~flag;
}

static inline void clearShapeFlag(plasma::SmoothMeshShape *shape, unsigned flag, bool clear)
{
    if (!clear)
        shape->m_flags |= flag;
    else
        shape->m_flags &= ~flag;
}

// The dot product, summed in a float (as in Workspace.cpp).
static inline float dot(const plasma::Vector<2, float> &a, const plasma::Vector<2, float> &b)
{
    float result = a[0] * b[0];
    for (unsigned i = 1; i < 2; ++i)
        result += a[i] * b[i];
    return result;
}

// The GLU edge-flag callback of Drawing::Tessellate, also set here so that the tessellator emits only triangles.
namespace plasma {
void __stdcall tessellationEdgeFlag(unsigned char flag);
}

// What CreateInnerEdges' tessellation callbacks build: the triangles' corner indices, three per triangle, in the
// shape's vertex arrays.
struct MeshTessellation {
    std::vector<int> m_indices;
    plasma::SmoothMeshShape *m_shape;
};

static void refineContourPoints(std::vector<int> &types, unsigned step,
                                std::vector<plasma::Vector<2, float>> &positions, int segments, bool closed,
                                float weight, int level);

MeshInterface::MeshInterface(NodeInterface *node, plasma::SmoothMeshShape *shape)
    : ShapeInterface(node, INTERFACE_MESH),
      m_mesh(shape)
{}

MeshInterface::~MeshInterface() {}

void MeshInterface::rebuild()
{
    m_mesh.rebuild();
}

// Adds a vertex with its attributes and updates the shape.
MeshVertex *MeshInterface::addVertex(const plasma::Vector<2, float> &position,
                                     const plasma::Vector<2, float> &textureCoordinate,
                                     const plasma::Vector<4, float> &vertexColor,
                                     const plasma::Vector<4, float> &strokeColor,
                                     const plasma::Vector<4, float> &extrusionFrontColor,
                                     const plasma::Vector<4, float> &extrusionBackColor, float width)
{
    MeshVertex *vertex = m_mesh.addShapeVertex(position, textureCoordinate, vertexColor, strokeColor,
                                               extrusionFrontColor, extrusionBackColor, width);
    m_mesh.updateShape();
    if (!m_mesh.getFaceCount())
        fitTextureToMesh();
    return vertex;
}

// Adds a face over the vertices and updates the shape.
MeshFace *MeshInterface::addFace(const std::list<MeshVertex *> &vertices)
{
    // A copy is made and not used: the face is added from the argument itself.
    std::list<MeshVertex *> corners(vertices);
    MeshFace *face = m_mesh.addFace(vertices, std::vector<char>());
    m_mesh.updateShape();
    m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    return face;
}

void MeshInterface::beginModify()
{
    captureState(m_state);
}

// Records the change since BeginModify as one "modify mesh" operation.
void MeshInterface::endModify()
{
    MeshState after;
    captureState(after);
    m_sheet->addOperation(new MeshOperation(m_sheet, "modify mesh", this, m_state, after));
}

// Records the colours being edited (SetColorPreview) as one "modify mesh" operation: the state before shows the
// stored colours (element 1).
void MeshInterface::recordColorEdit()
{
    MeshState after;
    captureState(after);
    MeshState before(after);
    before.m_vertexColors.copyToCurrent(plasma::Attribute::STORED_ELEMENT);
    before.m_strokeColors.copyToCurrent(plasma::Attribute::STORED_ELEMENT);
    before.m_extrusionFrontColors.copyToCurrent(plasma::Attribute::STORED_ELEMENT);
    before.m_extrusionBackColors.copyToCurrent(plasma::Attribute::STORED_ELEMENT);
    m_sheet->addOperation(new MeshOperation(m_sheet, "modify mesh", this, before, after));
}

void MeshInterface::removeFace(MeshFace *face)
{
    m_mesh.removeFace(face);
    m_mesh.updateShape();
}

// The vertex's position in the node, or its texture position while the interface edits the texture (m_hasMatrix).
plasma::Vector<2, float> MeshInterface::getVertexPosition(MeshVertex *vertex)
{
    if (m_hasMatrix)
        return m_mesh.m_shape->m_vertexTexCoords.value()[m_mesh.vertexIndex(vertex)];
    if (m_node->m_node->m_enclosingWidget) {
        plasma::Widget *widget = m_node->m_node->m_enclosingWidget;
        return widget->deform(m_mesh.m_shape->m_vertexPositions.value()[m_mesh.vertexIndex(vertex)], m_node->m_node);
    }
    return m_mesh.m_shape->m_vertexPositions.value()[m_mesh.vertexIndex(vertex)];
}

// The node's enclosing widget maps between the node's and the widget's coordinates.
plasma::Vector<2, float> MeshInterface::deform(const plasma::Vector<2, float> &point)
{
    if (m_node->m_node->m_enclosingWidget)
        return m_node->m_node->m_enclosingWidget->deform(point, m_node->m_node);
    return point;
}

plasma::Vector<2, float> MeshInterface::undeform(const plasma::Vector<2, float> &point)
{
    if (m_node->m_node->m_enclosingWidget)
        return m_node->m_node->m_enclosingWidget->undeform(point, m_node->m_node);
    return point;
}

// The mesh's edges on the sheet as point pairs, for the snapable nodes' snap lines (Workspace); with
// unselectedOnly, only the edges between two unselected vertices.
void MeshInterface::getEdgeSegments(std::vector<plasma::Vector<2, float>> &segments, bool unselectedOnly)
{
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it) {
        if (!unselectedOnly || (!(*it)->m_vertex->m_attributes[m_mesh.m_vertexSelectedOffset] &&
                                !(*it)->m_next->m_vertex->m_attributes[m_mesh.m_vertexSelectedOffset])) {
            plasma::Vector<2, float> first = matrix.transformPoint(getVertexPosition((*it)->m_vertex));
            plasma::Vector<2, float> second = matrix.transformPoint(getVertexPosition((*it)->m_next->m_vertex));
            segments.push_back(first);
            segments.push_back(second);
        }
    }
}

// The colour (of the colour module's kind) of the vertex nearest the mouse within the pick radius, and the
// vertex's position on the sheet.
bool MeshInterface::pickColor(plasma::Vector<4, float> &color, plasma::Vector<2, float> &position)
{
    int colorTarget = m_sheet->m_workspace->m_colorModule->getMode();
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    float nearest = 0.0f;
    bool found = false;
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        plasma::Vector<2, float> vertexPosition = getVertexPosition(*it);
        int count = 1;
        if (colorTarget == COLOR_TARGET_EXTRUSION)
            count = 2;
        plasma::Vector<4, float> *candidateColor;
        for (int i = 0; i < count; i++) {
            plasma::Vector<2, float> point = matrix.transformPoint(vertexPosition);
            plasma::Vector<2, float> offset = point - m_sheet->m_workspace->m_sceneMousePosition;
            float distance = offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1];
            float radius = m_sheet->m_workspace->m_toolRadius * m_sheet->m_workspace->m_toolRadius;
            if (distance <= radius) {
                // The code leaves the color pointer unset for an invalid target.
                switch (colorTarget) {
                case COLOR_TARGET_FILL:
                    candidateColor = &mesh.m_shape->m_vertexColors.value()[mesh.vertexIndex(*it)];
                    break;
                case COLOR_TARGET_STROKE:
                    candidateColor = &mesh.m_shape->m_strokeColors.value()[mesh.vertexIndex(*it)];
                    break;
                case COLOR_TARGET_EXTRUSION:
                    if (i)
                        candidateColor = &mesh.m_shape->m_extrusionBackColors.value()[mesh.vertexIndex(*it)];
                    else
                        candidateColor = &mesh.m_shape->m_extrusionFrontColors.value()[mesh.vertexIndex(*it)];
                    break;
                }
                plasma::Vector<2, float> offset = point - m_sheet->m_workspace->m_sceneMousePosition;
                float candidate =
                    (float)(offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1]);
                if (!found || nearest > candidate) {
                    nearest = candidate;
                    found = true;
                    color = *candidateColor;
                    position = point;
                }
            }
            vertexPosition.transform(mesh.m_shape->m_extrusionMatrix.value());
        }
    }
    return found;
}

// The stroke width (as seen on the sheet) of the vertex nearest the mouse within the pick radius, and the vertex's
// position on the sheet.
bool MeshInterface::pickStrokeWidth(float &width, plasma::Vector<2, float> &position)
{
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    float nearest = 0.0f;
    plasma::Matrix<float> sheetMatrix = m_sheet->m_root->m_inverseMatrix;
    bool found = false;
    sheetMatrix *= m_node->m_node->m_matrix;
    plasma::Vector<2, float> xAxis = sheetMatrix.transformDirection(plasma::Vector<2, float>(1.0f, 0.0f));
    plasma::Vector<2, float> yAxis = sheetMatrix.transformDirection(plasma::Vector<2, float>(0.0f, 1.0f));
    float xScale = std::sqrt(xAxis.m_elements[0] * xAxis.m_elements[0] + xAxis.m_elements[1] * xAxis.m_elements[1]);
    float yScale = std::sqrt(yAxis.m_elements[0] * yAxis.m_elements[0] + yAxis.m_elements[1] * yAxis.m_elements[1]);
    float scale = xScale < yScale ? xScale : yScale;
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        plasma::Vector<2, float> vertexPosition = getVertexPosition(*it);
        plasma::Vector<2, float> point = matrix.transformPoint(vertexPosition);
        plasma::Vector<2, float> offset = point - m_sheet->m_workspace->m_sceneMousePosition;
        float distance = offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1];
        float radius = m_sheet->m_workspace->m_toolRadius * m_sheet->m_workspace->m_toolRadius;
        if (distance <= radius) {
            float vertexWidth = mesh.m_shape->m_strokeWidths.value()[mesh.vertexIndex(*it)] * scale;
            plasma::Vector<2, float> offset = point - m_sheet->m_workspace->m_sceneMousePosition;
            float distance = offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1];
            if (!found || nearest > distance) {
                nearest = distance;
                found = true;
                width = vertexWidth;
                position = point;
            }
        }
    }
    return found;
}

// Adds the colour of each vertex under the mouse: 0 fill, 1 stroke, 2 the front colour, then the back colour at
// the extruded position.
void MeshInterface::getColorsUnderMouse(std::list<plasma::Vector<4, float>> &colors, int colorTarget)
{
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        plasma::Vector<2, float> position = getVertexPosition(*it);
        int count = 1;
        if (colorTarget == COLOR_TARGET_EXTRUSION)
            count = 2;
        plasma::Vector<4, float> *color;
        for (int i = 0; i < count; i++) {
            plasma::Vector<2, float> offset =
                matrix.transformPoint(position) - m_sheet->m_workspace->getSceneMousePosition();
            float distance = offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1];
            float radius = m_sheet->m_workspace->m_toolRadius * m_sheet->m_workspace->m_toolRadius;
            if (distance <= radius) {
                // The code leaves the color pointer unset for an invalid target.
                switch (colorTarget) {
                case COLOR_TARGET_FILL:
                    color = &mesh.m_shape->m_vertexColors.value()[mesh.vertexIndex(*it)];
                    break;
                case COLOR_TARGET_STROKE:
                    color = &mesh.m_shape->m_strokeColors.value()[mesh.vertexIndex(*it)];
                    break;
                case COLOR_TARGET_EXTRUSION:
                    if (i)
                        color = &mesh.m_shape->m_extrusionBackColors.value()[mesh.vertexIndex(*it)];
                    else
                        color = &mesh.m_shape->m_extrusionFrontColors.value()[mesh.vertexIndex(*it)];
                    break;
                }
                colors.push_back(*color);
            }
            position.transform(mesh.m_shape->m_extrusionMatrix.value());
        }
    }
}

// Paints the colour module's colour into the colours (0 fill, 1 stroke, 2 front and back) of the vertices under
// the mouse. With Alt the vertex's colour is picked into the module instead; the colour tool's alpha option paints
// only the alpha (or the whole colour with its colour option, unless Ctrl is down), Shift paints only the colour.
void MeshInterface::paintColor(int colorTarget, bool update)
{
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        plasma::Vector<2, float> position = getVertexPosition(*it);
        int count = 1;
        if (colorTarget == COLOR_TARGET_EXTRUSION)
            count = 2;
        plasma::Vector<4, float> *color;
        for (int i = 0; i < count; i++) {
            Workspace *workspace = m_sheet->m_workspace;
            plasma::Vector<2, float> offset = matrix.transform(position) - workspace->m_sceneMousePosition;
            float distance = offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1];
            if (distance <= (float)(workspace->m_toolRadius * workspace->m_toolRadius)) {
                // The code leaves the color pointer unset for an invalid target.
                switch (colorTarget) {
                case COLOR_TARGET_FILL:
                    color = &m_mesh.m_shape->m_vertexColors.value()[vertexIndex(*it)];
                    break;
                case COLOR_TARGET_STROKE:
                    color = &m_mesh.m_shape->m_strokeColors.value()[vertexIndex(*it)];
                    break;
                case COLOR_TARGET_EXTRUSION:
                    if (i)
                        color = &m_mesh.m_shape->m_extrusionBackColors.value()[vertexIndex(*it)];
                    else
                        color = &m_mesh.m_shape->m_extrusionFrontColors.value()[vertexIndex(*it)];
                    break;
                }
                if (workspace->m_engine.isKeyDown(VK_MENU)) {
                    workspace->m_colorModule->setColor(*color, true, true, false);
                } else if (!workspace->m_engine.isKeyDown(VK_SHIFT) && workspace->m_colorToolModule->m_alpha) {
                    if (!workspace->m_engine.isKeyDown(VK_CONTROL) && workspace->m_colorToolModule->m_color)
                        *color = workspace->m_colorModule->getColor();
                    else
                        color->m_elements[RGBA_ALPHA] = workspace->m_colorModule->getColor().m_elements[RGBA_ALPHA];
                } else {
                    color->m_elements[0] = workspace->m_colorModule->getColor().m_elements[0];
                    color->m_elements[1] = m_sheet->m_workspace->m_colorModule->getColor().m_elements[1];
                    color->m_elements[2] = m_sheet->m_workspace->m_colorModule->getColor().m_elements[2];
                }
                m_hasChanges = true;
                m_refreshPending = true;
                m_brushHit = true;
            }
            position.transform(m_mesh.m_shape->m_extrusionMatrix.value());
        }
    }
    if (update && m_hasChanges) {
        m_mesh.m_shape->m_vertexColors.m_changed = true;
        m_mesh.m_shape->m_strokeColors.m_changed = true;
        m_mesh.m_shape->m_extrusionFrontColors.m_changed = true;
        m_mesh.m_shape->m_extrusionBackColors.m_changed = true;
    }
}

// Sets the stroke width of the vertices under the mouse to the stroke width module's width (as seen on the sheet);
// with Alt, picks the vertex's width into the module instead.
void MeshInterface::paintStrokeWidth()
{
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    plasma::Matrix<float> sheetMatrix = m_sheet->m_root->m_inverseMatrix;
    sheetMatrix *= m_node->m_node->m_matrix;
    plasma::Vector<2, float> xAxis = sheetMatrix.transformDirection(plasma::Vector<2, float>(1.0f, 0.0f));
    plasma::Vector<2, float> yAxis = sheetMatrix.transformDirection(plasma::Vector<2, float>(0.0f, 1.0f));
    float xScale = std::sqrt(xAxis.m_elements[0] * xAxis.m_elements[0] + xAxis.m_elements[1] * xAxis.m_elements[1]);
    float yScale = std::sqrt(yAxis.m_elements[0] * yAxis.m_elements[0] + yAxis.m_elements[1] * yAxis.m_elements[1]);
    float scale = xScale < yScale ? xScale : yScale;
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        plasma::Vector<2, float> vertexPosition = getVertexPosition(*it);
        plasma::Vector<2, float> point = matrix.transformPoint(vertexPosition);
        plasma::Vector<2, float> offset = point - m_sheet->m_workspace->m_sceneMousePosition;
        float distance = offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1];
        float radius = m_sheet->m_workspace->m_toolRadius * m_sheet->m_workspace->m_toolRadius;
        if (distance <= radius) {
            if (m_sheet->m_workspace->m_engine.isKeyDown(VK_MENU))
                m_sheet->m_workspace->m_strokeWidthModule->setWidth(
                    mesh.m_shape->m_strokeWidths.value()[mesh.vertexIndex(*it)] * scale);
            else
                mesh.m_shape->m_strokeWidths.value()[mesh.vertexIndex(*it)] =
                    m_sheet->m_workspace->m_strokeWidthModule->m_width / scale;
            m_hasChanges = true;
            m_refreshPending = true;
            m_brushHit = true;
        }
    }
}

// Scales the vertex stroke widths to the stroke width module's width, as seen on the sheet; widths outside
// [minimum, maximum] keep their ratio.
void MeshInterface::scaleStrokeWidths(float minimum, float maximum)
{
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    matrix = m_sheet->m_root->m_inverseMatrix;
    matrix *= m_node->m_node->m_matrix;
    plasma::Vector<2, float> xAxis = matrix.transformDirection(plasma::Vector<2, float>(1.0f, 0.0f));
    plasma::Vector<2, float> yAxis = matrix.transformDirection(plasma::Vector<2, float>(0.0f, 1.0f));
    float xScale = std::sqrt(xAxis.m_elements[0] * xAxis.m_elements[0] + xAxis.m_elements[1] * xAxis.m_elements[1]);
    float yScale = std::sqrt(yAxis.m_elements[0] * yAxis.m_elements[0] + yAxis.m_elements[1] * yAxis.m_elements[1]);
    float scale = xScale < yScale ? xScale : yScale;
    float width = m_sheet->m_workspace->m_strokeWidthModule->m_width / scale;
    minimum /= scale;
    maximum /= scale;
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        getVertexPosition(*it);
        float &vertexWidth = mesh.m_shape->m_strokeWidths.value()[mesh.vertexIndex(*it)];
        if (vertexWidth < minimum)
            vertexWidth = width / minimum * vertexWidth;
        else if (vertexWidth > maximum)
            vertexWidth = width / maximum * vertexWidth;
        else
            vertexWidth = width;
    }
    m_hasChanges = true;
    m_refreshPending = true;
}

// The range of the stroke widths (as seen on the sheet) of the vertices under the mouse.
bool MeshInterface::getStrokeWidthRange(float &minimum, float &maximum)
{
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    plasma::Matrix<float> sheetMatrix = m_sheet->m_root->m_inverseMatrix;
    bool found = false;
    sheetMatrix *= m_node->m_node->m_matrix;
    plasma::Vector<2, float> xAxis = sheetMatrix.transformDirection(plasma::Vector<2, float>(1.0f, 0.0f));
    plasma::Vector<2, float> yAxis = sheetMatrix.transformDirection(plasma::Vector<2, float>(0.0f, 1.0f));
    float xScale = std::sqrt(xAxis.m_elements[0] * xAxis.m_elements[0] + xAxis.m_elements[1] * xAxis.m_elements[1]);
    float yScale = std::sqrt(yAxis.m_elements[0] * yAxis.m_elements[0] + yAxis.m_elements[1] * yAxis.m_elements[1]);
    float scale = xScale < yScale ? xScale : yScale;
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        plasma::Vector<2, float> vertexPosition = getVertexPosition(*it);
        plasma::Vector<2, float> point = matrix.transformPoint(vertexPosition);
        plasma::Vector<2, float> offset = point - m_sheet->m_workspace->m_sceneMousePosition;
        float distance = offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1];
        float radius = m_sheet->m_workspace->m_toolRadius * m_sheet->m_workspace->m_toolRadius;
        if (distance <= radius) {
            float vertexWidth = mesh.m_shape->m_strokeWidths.value()[mesh.vertexIndex(*it)] * scale;

            // Computed again but unused.
            plasma::Vector<2, float> vertexOffset = point - m_sheet->m_workspace->m_sceneMousePosition;
            if (found) {
                minimum = minimum < vertexWidth ? minimum : vertexWidth;
                maximum = vertexWidth < maximum ? maximum : vertexWidth;
            } else {
                minimum = vertexWidth;
                maximum = vertexWidth;
                found = true;
            }
        }
    }
    return found;
}

// Selects the vertices under the mouse (Ctrl deselects them; without Shift or Ctrl the others are deselected
// first) as one "select vertices" operation. Each vertex gets a weight that falls off with its distance from the
// mouse over the pick radius (0 outside it).
void MeshInterface::selectVertices()
{
    m_hasChanges = false;
    m_brushHit = false;
    bool select = !m_sheet->m_workspace->m_engine.isKeyDown(VK_CONTROL);
    std::vector<bool> before = getVertexSelection();
    bool changed = false;
    Workspace *workspace = m_sheet->m_workspace;
    if (!workspace->m_engine.isKeyDown(VK_SHIFT) && !workspace->m_engine.isKeyDown(VK_CONTROL))
        changed = selectAllVertices(false);
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        plasma::Matrix<float> matrix;
        getMatrix(matrix);
        m_softSelectionWeights.resize(m_mesh.getVertexCount());
        plasma::Vector<2, float> point = matrix * getVertexPosition(*it);
        plasma::Vector<2, float> offset = point - m_sheet->m_workspace->m_sceneMousePosition;
        float distance = offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1];
        float weight;

        // The squared radius is rounded to float; a NaN distance gets weight 0.
        if (distance <= (float)(m_sheet->m_workspace->m_toolRadius * m_sheet->m_workspace->m_toolRadius)) {
            if (isVertexSelected(*it) != select) {
                setVertexSelected(*it, select);
                changed = true;
            }
            plasma::Vector<2, float> offset = point - m_sheet->m_workspace->m_sceneMousePosition;
            float fraction =
                std::sqrt(offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1]) /
                m_sheet->m_workspace->m_toolRadius;
            float falloff = 1.0f - fraction * fraction;
            weight = falloff * falloff;
        } else {
            weight = 0.0f;
        }
        m_softSelectionWeights[vertexIndex(*it)] = weight;
    }
    if (changed) {
        std::vector<bool> after = getVertexSelection();
        m_sheet->addOperation(new MeshVertexSelectionOperation(m_sheet, "select vertices", this, before, after));
    }
    refresh();
}

// Selects the vertices whose extruded position is under the mouse (Ctrl deselects them).
void MeshInterface::selectExtrudedVertices()
{
    m_hasChanges = false;
    m_brushHit = false;
    bool select = !m_sheet->m_workspace->m_engine.isKeyDown(VK_CONTROL);
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        plasma::Matrix<float> &extrusion = m_mesh.m_shape->m_extrusionMatrix.value();
        plasma::Vector<2, float> position = matrix.transformPoint(extrusion.transformPoint(getVertexPosition(*it)));
        plasma::Vector<2, float> offset = position - m_sheet->m_workspace->m_sceneMousePosition;
        float distance = offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1];
        float radius = m_sheet->m_workspace->m_toolRadius * m_sheet->m_workspace->m_toolRadius;
        if (distance <= radius && isVertexSelected(*it) != select) {
            setVertexSelected(*it, select);
            m_hasChanges = true;
            m_refreshPending = true;
        }
    }
    refresh();
}

// Selects the edges under the mouse (Ctrl deselects them; without Shift or Ctrl the others are deselected first)
// and then their vertices instead, as one "select edges" operation. The vertices of the edges near the mouse get
// weights that fall off with the edge's distance over the pick radius. A contour mesh skips the closing edge of
// its first face.
void MeshInterface::selectEdges()
{
    m_hasChanges = false;
    m_brushHit = false;
    bool select = !m_sheet->m_workspace->m_engine.isKeyDown(VK_CONTROL);
    bool changed = false;
    std::vector<bool> before = getEdgeSelection();
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    m_softSelectionWeights.resize(m_mesh.getVertexCount());
    for (int i = 0; i < (int)m_softSelectionWeights.size(); i++)
        m_softSelectionWeights[i] = 0.0f;
    Workspace *workspace = m_sheet->m_workspace;
    if (!workspace->m_engine.isKeyDown(VK_SHIFT) && !workspace->m_engine.isKeyDown(VK_CONTROL))
        changed = selectAllEdges(false);
    MeshHalfEdge *skipped = 0;
    if (!isClosed(m_mesh.m_shape->m_flags) && (int)m_mesh.getFaceCount() > 0)
        skipped = (*m_mesh.beginFaces())->m_edge->m_previous;
    for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it) {
        if (*it != skipped) {
            plasma::Vector<2, float> first = getVertexPosition((*it)->m_vertex);
            plasma::Vector<2, float> second = getVertexPosition((*it)->m_next->m_vertex);
            float distance = squaredDistanceToSegment(m_sheet->m_workspace->m_sceneMousePosition,
                                                      matrix.transformPoint(first), matrix.transformPoint(second));
            if (distance <= (float)(m_sheet->m_workspace->m_toolRadius * m_sheet->m_workspace->m_toolRadius)) {
                if (isEdgeSelected(*it) != select) {
                    setEdgeSelected(*it, select);
                    changed = true;
                }
                float falloff =
                    1.0f - distance / (m_sheet->m_workspace->m_toolRadius * m_sheet->m_workspace->m_toolRadius);
                float weight = falloff * falloff;
                if (m_softSelectionWeights[vertexIndex((*it)->m_vertex)] < weight)
                    m_softSelectionWeights[vertexIndex((*it)->m_vertex)] = weight;
                if (m_softSelectionWeights[vertexIndex((*it)->m_next->m_vertex)] < weight)
                    m_softSelectionWeights[vertexIndex((*it)->m_next->m_vertex)] = weight;
            }
        }
    }
    selectAllVertices(false);
    for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it) {
        if (isEdgeSelected(*it)) {
            setVertexSelected((*it)->m_vertex, true);
            setVertexSelected((*it)->m_next->m_vertex, true);
        }
    }
    refresh();
    if (changed) {
        std::vector<bool> after = getEdgeSelection();
        m_sheet->addOperation(new MeshEdgeSelectionOperation(m_sheet, "select edges", this, before, after));
    }
}

// Selects the faces under the mouse (Ctrl deselects them; without Shift or Ctrl the others are deselected first)
// and then their vertices instead, as one "select faces" operation. The vertices get weights that fall off with
// their distance from the mouse over the pick radius.
void MeshInterface::selectFaces()
{
    m_hasChanges = false;
    m_brushHit = false;
    bool select = !m_sheet->m_workspace->m_engine.isKeyDown(VK_CONTROL);
    m_softSelectionWeights.resize(m_mesh.getVertexCount());
    for (int i = 0; i < (int)m_softSelectionWeights.size(); i++)
        m_softSelectionWeights[i] = 0.0f;
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    m_softSelectionWeights.resize(m_mesh.getVertexCount());
    bool changed = false;
    std::vector<bool> before = getFaceSelection();
    Workspace *workspace = m_sheet->m_workspace;
    if (!workspace->m_engine.isKeyDown(VK_SHIFT) && !workspace->m_engine.isKeyDown(VK_CONTROL))
        changed = selectAllFaces(false);
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        plasma::Vector<2, float> point = matrix.transformPoint(getVertexPosition(*it));
        workspace = m_sheet->m_workspace;
        plasma::Vector<2, float> offset = point - workspace->m_sceneMousePosition;
        float distance = offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1];
        if (distance <= (float)(workspace->m_toolRadius * workspace->m_toolRadius)) {
            plasma::Vector<2, float> offset = point - workspace->m_sceneMousePosition;
            float fraction =
                std::sqrt(offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1]) /
                workspace->m_toolRadius;
            float falloff = 1.0f - fraction * fraction;
            m_softSelectionWeights[vertexIndex(*it)] = falloff * falloff;
        } else {
            m_softSelectionWeights[vertexIndex(*it)] = 0.0f;
        }
    }
    for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it) {
        if (isFaceUnderMouse(*it) && isFaceSelected(*it) != select) {
            setFaceSelected(*it, select);
            changed = true;
        }
    }
    selectAllVertices(false);
    for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it) {
        if (isFaceSelected(*it)) {
            for (MeshFaceIterator corner(*it); corner.isValid(); ++corner)
                setVertexSelected(corner.getVertex(), true);
        }
    }
    refresh();
    if (changed) {
        std::vector<bool> after = getFaceSelection();
        m_sheet->addOperation(new MeshFaceSelectionOperation(m_sheet, "select faces", this, before, after));
    }
}

bool MeshInterface::selectAllVertices(bool selected)
{
    bool changed = false;
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        if (isVertexSelected(*it) != selected) {
            changed = true;
            setVertexSelected(*it, selected);
        }
    }
    return changed;
}

bool MeshInterface::selectAllEdges(bool selected)
{
    bool changed = false;
    for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it) {
        if (isEdgeSelected(*it) != selected) {
            changed = true;
            setEdgeSelected(*it, selected);
        }
    }
    return changed;
}

bool MeshInterface::selectAllFaces(bool selected)
{
    bool changed = false;
    for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it) {
        if (isFaceSelected(*it) != selected) {
            changed = true;
            setFaceSelected(*it, selected);
        }
    }
    return changed;
}

int MeshInterface::countSelectedVertices()
{
    int count = 0;
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        if (isVertexSelected(*it))
            ++count;
    }
    return count;
}

// Both halves of a selected edge count once.
int MeshInterface::countSelectedEdges()
{
    int count = 0;
    std::set<MeshHalfEdge *> counted;
    for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it) {
        if (isEdgeSelected(*it) && counted.find((*it)->m_opposite) == counted.end()) {
            ++count;
            counted.insert(*it);
        }
    }
    return count;
}

int MeshInterface::countSelectedFaces()
{
    int count = 0;
    for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it) {
        if (isFaceSelected(*it))
            ++count;
    }
    return count;
}

// Rebuilds the wireframe from the mesh: the vertices (as drawn, on the texture and through the extrusion matrix)
// with their colours, the edges (boundary edges twice, selected ones apart), the faces as triangle fans (selected
// ones apart) and the contours subdivided as the shape draws them.
void MeshInterface::refresh()
{
    int count = m_mesh.getVertexCount();
    m_wireFrame.m_points.resize(count);
    m_wireFrame.m_texturePoints.resize(count);
    m_wireFrame.m_extrudedPoints.resize(count);
    m_wireFrame.m_fillColors.resize(count);
    m_wireFrame.m_strokeColors.resize(count);
    m_wireFrame.m_frontColors.resize(count);
    m_wireFrame.m_backColors.resize(count);

    // The positions as on the sheet, not on the texture.
    bool hasMatrix = m_hasMatrix;
    m_hasMatrix = false;
    int index = 0;
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it, ++index) {
        m_wireFrame.m_points[index] = getVertexPosition(*it);
        m_wireFrame.m_extrudedPoints[index] =
            m_mesh.m_shape->m_extrusionMatrix.value().transform(getVertexPosition(*it));
        m_wireFrame.m_texturePoints[index] = m_mesh.m_shape->m_vertexTexCoords.value()[vertexIndex(*it)];
        m_wireFrame.m_fillColors[index] = mesh.m_shape->m_vertexColors.value()[mesh.vertexIndex(*it)];
        m_wireFrame.m_fillColors[index].m_elements[RGBA_ALPHA] = 1.0f;
        m_wireFrame.m_strokeColors[index] = mesh.m_shape->m_strokeColors.value()[mesh.vertexIndex(*it)];
        m_wireFrame.m_strokeColors[index].m_elements[RGBA_ALPHA] = 1.0f;
        m_wireFrame.m_frontColors[index] = mesh.m_shape->m_extrusionFrontColors.value()[mesh.vertexIndex(*it)];
        m_wireFrame.m_frontColors[index].m_elements[RGBA_ALPHA] = 1.0f;
        m_wireFrame.m_backColors[index] = mesh.m_shape->m_extrusionBackColors.value()[mesh.vertexIndex(*it)];
        m_wireFrame.m_backColors[index].m_elements[RGBA_ALPHA] = 1.0f;
    }
    m_hasMatrix = hasMatrix;

    // An open contour leaves out the edge that closes it.
    MeshHalfEdge *closing = 0;
    if (!isClosed(m_mesh.m_shape->m_flags) && (int)m_mesh.getFaceCount() > 0)
        closing = (*m_mesh.beginFaces())->m_edge->m_previous;
    m_wireFrame.m_lineIndices.clear();
    m_wireFrame.m_unselectedLineIndices.clear();
    m_wireFrame.m_selectedLineIndices.clear();
    m_wireFrame.m_hoveredLineIndices.clear();
    for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it) {
        if (*it == closing)
            continue;
        unsigned first = vertexIndex((*it)->m_vertex);
        unsigned second = vertexIndex((*it)->m_next->m_vertex);
        int repeat = 1;
        if (!(*it)->m_opposite)
            repeat = 2;
        for (int k = 0; k < repeat; ++k) {
            m_wireFrame.m_lineIndices.push_back(first);
            m_wireFrame.m_lineIndices.push_back(second);
            if (isEdgeSelected(*it)) {
                m_wireFrame.m_selectedLineIndices.push_back(first);
                m_wireFrame.m_selectedLineIndices.push_back(second);
            } else {
                m_wireFrame.m_unselectedLineIndices.push_back(first);
                m_wireFrame.m_unselectedLineIndices.push_back(second);
            }
        }
    }

    m_wireFrame.m_triangleIndices.clear();
    m_wireFrame.m_unselectedTriangleIndices.clear();
    m_wireFrame.m_selectedTriangleIndices.clear();
    for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it) {
        if (countCorners(*it) <= 2)
            continue;
        MeshFaceIterator corner(*it);
        MeshFaceIterator second = corner;
        ++second;
        MeshFaceIterator third = second;
        ++third;
        unsigned origin = vertexIndex(corner.getVertex());
        for (; third.isValid(); ++second, ++third) {
            unsigned b = vertexIndex(second.getVertex());
            unsigned c = vertexIndex(third.getVertex());
            m_wireFrame.m_triangleIndices.push_back(origin);
            m_wireFrame.m_triangleIndices.push_back(b);
            m_wireFrame.m_triangleIndices.push_back(c);
            if (isFaceSelected(*it)) {
                m_wireFrame.m_selectedTriangleIndices.push_back(origin);
                m_wireFrame.m_selectedTriangleIndices.push_back(b);
                m_wireFrame.m_selectedTriangleIndices.push_back(c);
            } else {
                m_wireFrame.m_unselectedTriangleIndices.push_back(origin);
                m_wireFrame.m_unselectedTriangleIndices.push_back(b);
                m_wireFrame.m_unselectedTriangleIndices.push_back(c);
            }
        }
    }

    // The contours, subdivided as often as the shape's own (step points per contour edge).
    m_wireFrame.m_outlineIndices.clear();
    m_wireFrame.m_outlinePoints.clear();
    int step = (int)std::pow(2.0, m_mesh.m_shape->m_subdivisions);
    int total = 0;
    for (int c = 0; c < (int)m_mesh.m_shape->m_contours.size(); ++c) {
        if (!isClosed(m_mesh.m_shape->m_flags))
            total += step * (m_mesh.m_shape->m_contours[c].size() - 1) + 1;
        else
            total += step * m_mesh.m_shape->m_contours[c].size();
    }
    m_wireFrame.m_outlinePoints.resize(total);
    m_wireFrame.m_textureOutlinePoints.resize(total);
    int offset = 0;
    for (int c = 0; c < (int)m_mesh.m_shape->m_contours.size(); ++c) {
        const std::vector<int> &contour = m_mesh.m_shape->m_contours[c];
        int points;
        if (!isClosed(m_mesh.m_shape->m_flags))
            points = step * (contour.size() - 1) + 1;
        else
            points = step * contour.size();
        std::vector<plasma::Vector<2, float>> positions(points);
        std::vector<plasma::Vector<2, float>> textureCoordinates(points);
        std::vector<int> types(points, 0);
        for (int i = 0; i < (int)contour.size(); ++i) {
            positions[i * step] = m_mesh.m_shape->m_vertexPositions.value()[contour[i]];
            textureCoordinates[i * step] = m_mesh.m_shape->m_vertexTexCoords.value()[contour[i]];
            types[i * step] = m_mesh.m_shape->m_vertexFlags[contour[i]];
        }
        for (int level = 0; level < m_mesh.m_shape->m_subdivisions; ++level) {
            int levelStep = (int)std::pow(2.0, m_mesh.m_shape->m_subdivisions - level);
            int segments = positions.size() / levelStep;
            refineContourPoints(types, levelStep, positions, segments, isClosed(m_mesh.m_shape->m_flags),
                                m_mesh.m_shape->m_smoothWeight, level);
            refineContourPoints(types, levelStep, textureCoordinates, segments, isClosed(m_mesh.m_shape->m_flags),
                                m_mesh.m_shape->m_smoothWeight, level);
        }
        int lines = points;
        if (!isClosed(m_mesh.m_shape->m_flags))
            lines = points - 1;
        for (int i = 0; i < points; ++i) {
            m_wireFrame.m_outlinePoints[offset + i] = positions[i];
            m_wireFrame.m_textureOutlinePoints[offset + i] = textureCoordinates[i];
        }
        for (int i = 0; i < lines; ++i) {
            m_wireFrame.m_outlineIndices.push_back(offset + i);
            m_wireFrame.m_outlineIndices.push_back(offset + (i + 1) % points);
        }
        offset += points;
    }
    m_wireFrame.upload();
}

// Colours the wireframe's points (hovered, selected, or by vertex type) and collects the hovered edges (the edge
// tool) or the hovered faces' fan triangles (the face tool) for highlighting. Layout positions, extents and
// spacing use display units; colour tuples style the overlays. Wireframe palette: cyan hover, purple selected
// hover/current, green selection, white smooth/root, dark sharp, blue Bezier and red round; yellow/magenta
// constraints.
void MeshInterface::updateWireFrameColors()
{
    int tool = m_sheet->m_workspace->m_toolModule->getCurrentTool()->getType();
    m_wireFrame.m_pointColors.resize(m_mesh.getVertexCount());
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        plasma::Vector<2, float> offset =
            matrix.transform(getVertexPosition(*it)) - m_sheet->m_workspace->getSceneMousePosition();
        float distance = offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1];
        float radius = m_sheet->m_workspace->m_toolRadius * m_sheet->m_workspace->m_toolRadius;
        if (distance <= radius) {
            if (!isVertexSelected(*it))
                m_wireFrame.m_pointColors[vertexIndex(*it)] = WireFrame::Color(0.0f, 1.0f, 1.0f, 1.0f);
            else
                m_wireFrame.m_pointColors[vertexIndex(*it)] = WireFrame::Color(0.75f, 0.25f, 1.0f, 1.0f);
        } else {
            if (isVertexSelected(*it)) {
                m_wireFrame.m_pointColors[vertexIndex(*it)] = WireFrame::Color(0.5f, 1.0f, 0.5f, 1.0f);
            } else {
                switch ((unsigned short)m_mesh.m_shape->m_vertexFlags[vertexIndex(*it)]) {
                case plasma::VERTEX_SMOOTH:
                    m_wireFrame.m_pointColors[vertexIndex(*it)] = WireFrame::Color(1.0f, 1.0f, 1.0f, 1.0f);
                    break;
                case plasma::VERTEX_SHARP:
                    m_wireFrame.m_pointColors[vertexIndex(*it)] = WireFrame::Color(0.25f, 0.25f, 0.25f, 1.0f);
                    break;
                case plasma::VERTEX_BEZIER:
                    m_wireFrame.m_pointColors[vertexIndex(*it)] = WireFrame::Color(0.5f, 0.5f, 1.0f, 1.0f);
                    break;
                case plasma::VERTEX_ROUND:
                    m_wireFrame.m_pointColors[vertexIndex(*it)] = WireFrame::Color(1.0f, 0.5f, 0.5f, 1.0f);
                    break;
                }
            }
        }
    }
    m_wireFrame.m_hoveredLineIndices.clear();
    if (tool == Tool::TOOL_EDGE) {
        getMatrix(matrix);

        // An open contour's closing edge is not highlighted.
        MeshHalfEdge *closing = 0;
        if (!isClosed(m_mesh.m_shape->m_flags) && m_mesh.getFaceCount() == 1)
            closing = (*m_mesh.beginFaces())->m_edge->m_previous;
        for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it) {
            if (*it != closing) {
                plasma::Vector<2, float> first = getVertexPosition((*it)->m_vertex);
                plasma::Vector<2, float> second = getVertexPosition((*it)->m_next->m_vertex);
                float radius = m_sheet->m_workspace->m_toolRadius;
                if ((float)(radius * radius) >= squaredDistanceToSegment(m_sheet->m_workspace->getSceneMousePosition(),
                                                                         matrix * first, matrix * second)) {
                    m_wireFrame.m_hoveredLineIndices.push_back(vertexIndex((*it)->m_vertex));
                    m_wireFrame.m_hoveredLineIndices.push_back(vertexIndex((*it)->m_next->m_vertex));
                }
            }
        }
    }
    m_wireFrame.m_hoveredTriangleIndices.clear();
    if (tool == Tool::TOOL_FACE) {
        for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it) {
            if (isFaceUnderMouse(*it) && countCorners(*it) > 2) {
                MeshFaceIterator first(*it);
                MeshFaceIterator second = first;
                ++second;
                MeshFaceIterator third = second;
                ++third;
                int origin = vertexIndex(first.getVertex());
                while (third.isValid()) {
                    m_wireFrame.m_hoveredTriangleIndices.push_back(origin);
                    m_wireFrame.m_hoveredTriangleIndices.push_back(vertexIndex(second.getVertex()));
                    m_wireFrame.m_hoveredTriangleIndices.push_back(vertexIndex(third.getVertex()));
                    ++second;
                    ++third;
                }
            }
        }
    }
    m_wireFrame.uploadEditArrays();
}

// Draws the wireframe when the shape is selected, snapable, hovered with the node tool or a modal node, or is the
// sheet's edit root; else empties it. Layout positions, extents and spacing use display units; colour tuples style
// the overlays. Wireframe palette: cyan hover, purple selected hover/current, green selection, white smooth/root,
// dark sharp, blue Bezier and red round; yellow/magenta constraints.
void MeshInterface::draw()
{
    Workspace *workspace = m_sheet->m_workspace;
    int tool = workspace->m_toolModule->getCurrentTool()->getType();
    if (tool != Tool::TOOL_DRAW_FREEHAND && !workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_MIDDLE) &&
        !workspace->isPanning() &&
        ((tool == Tool::TOOL_NODE || workspace->m_guiPointerNode) && m_node->m_node == workspace->getHoveredNode() &&
             !workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) &&
             !workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT) ||
         m_node->m_selected || m_node->m_node->isSnapable() || m_node->m_node == m_sheet->m_editRoot)) {
        glDisable(GL_TEXTURE_2D);
        if (m_wireFrame.m_points.empty())
            this->refresh();
        updateWireFrameColors();
        unsigned flags = 0;
        if (m_sheet->m_workspace->m_colorModule->getMode() == COLOR_TARGET_EXTRUSION)
            flags = WIREFRAME_EXTRUSION_COLORS;
        if (m_sheet->m_workspace->m_colorModule->getMode() == COLOR_TARGET_STROKE)
            flags |= WIREFRAME_STROKE_COLORS;
        if (m_hasMatrix)
            flags |= WIREFRAME_TEXTURE_SPACE;

        // NODE_SNAPABLE is "node:snapable".
        bool snapable = m_node->m_node->isSnapable();
        if (snapable)
            flags |= WIREFRAME_DRAW_LINES;
        bool selected = m_node->m_selected;
        if (selected) {
            flags |= WIREFRAME_DRAW_LINES;
            switch (m_sheet->m_workspace->m_toolModule->getCurrentTool()->getType()) {
            case Tool::TOOL_VERTEX:
            case Tool::TOOL_COLOR:
            case Tool::TOOL_DRAW_FACE:
            case Tool::TOOL_DRAW_EDGE:
            case Tool::TOOL_WIDTH:
                flags |= WIREFRAME_DRAW_POINTS;
                break;
            case Tool::TOOL_EDGE:
                flags |= WIREFRAME_HIGHLIGHT;
                break;
            case Tool::TOOL_FACE:
                flags |= WIREFRAME_DRAW_TRIANGLES;
                break;
            }
        }
        if (selected || !snapable)
            flags |= WIREFRAME_DRAW_OUTLINE;
        WireFrame::Color color(0.5f, 1.0f, 0.5f, 1.0f);
        if (m_sheet->m_workspace->m_toolModule->getCurrentTool()->getType() == Tool::TOOL_NODE &&
            m_node->m_node == m_sheet->m_workspace->getHoveredNode()) {
            color = WireFrame::Color(0.0f, 1.0f, 1.0f, 1.0f);
        } else {
            if (m_sheet->m_editRoot == m_node->m_node)
                color = WireFrame::Color(1.0f, 1.0f, 1.0f, 1.0f);
            if (m_sheet->m_currentNode == m_node->m_node)
                color = WireFrame::Color(0.8f, 0.5f, 1.0f, 1.0f);
        }
        if (m_hasMatrix) {
            glPushMatrix();
            glMultMatrixf(m_mesh.m_shape->m_textureMatrix.m_elements);
            m_wireFrame.draw(flags, color);
            glPopMatrix();
        } else {
            glUseProgram(0);
            glDisable(GL_TEXTURE_2D);
            if (flags & WIREFRAME_DRAW_POINTS) {
                // The corner and tangent vertices (types above plasma::VERTEX_KIND_MASK) get a black border and a dot.
                EditableMesh &mesh = m_mesh;
                glPointSize(13.0f);
                glBegin(GL_POINTS);
                glColor3f(0.0f, 0.0f, 0.0f);
                for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
                    if (mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] & plasma::VERTEX_CONSTRAINT_MASK)
                        glVertex2fv(getVertexPosition(*it).m_elements);
                }
                glEnd();
                glPointSize(11.0f);
                glColor3f(1.0f, 1.0f, 0.0f);
                glBegin(GL_POINTS);
                for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
                    unsigned kind = mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] & plasma::VERTEX_CONSTRAINT_MASK;
                    if (kind) {
                        if (kind == (1 << plasma::VERTEX_ROUNDED_CORNER))
                            glColor3f(1.0f, 1.0f, 0.0f);
                        else
                            glColor3f(1.0f, 0.0f, 1.0f);
                        glVertex2fv(getVertexPosition(*it).m_elements);
                    }
                }
                glEnd();
            }
            m_wireFrame.draw(flags, color);
        }
    } else {
        m_wireFrame.clear();
    }
}

void MeshInterface::mergeMesh(EditableMesh *source, MeshVertex **vertex, bool reverse, bool update)
{
    int vertexCount = m_mesh.m_shape->m_vertexPositions.m_values[0]->size();
    m_mesh.m_shape->m_vertexPositions.appendElements(source->m_shape->m_vertexPositions);
    m_mesh.m_shape->m_vertexColors.appendElements(source->m_shape->m_vertexColors);
    m_mesh.m_shape->m_vertexTexCoords.appendElements(source->m_shape->m_vertexTexCoords);
    m_mesh.m_shape->m_strokeColors.appendElements(source->m_shape->m_strokeColors);
    m_mesh.m_shape->m_extrusionBackColors.appendElements(source->m_shape->m_extrusionBackColors);
    m_mesh.m_shape->m_extrusionFrontColors.appendElements(source->m_shape->m_extrusionFrontColors);
    m_mesh.m_shape->m_strokeWidths.appendElements(source->m_shape->m_strokeWidths);
    std::map<MeshVertex *, MeshVertex *> copies;
    int first = m_mesh.m_shape->m_vertexFlags.size();
    m_mesh.m_shape->m_vertexFlags.resize(source->m_shape->m_vertexFlags.size() + m_mesh.m_shape->m_vertexFlags.size());
    m_mesh.m_shape->m_vertexParameters.resize(source->m_shape->m_vertexParameters.size() +
                                              m_mesh.m_shape->m_vertexParameters.size());
    int index = 0;
    for (HalfEdgeMesh::VertexIterator it = source->beginVertices(); it != source->endVertices(); ++it) {
        MeshVertex *copy = m_mesh.addVertex();
        copies[*it] = copy;
        *reinterpret_cast<int *>(&copy->m_attributes[m_mesh.m_vertexIndexOffset]) =
            *reinterpret_cast<int *>(&(*it)->m_attributes[source->m_vertexIndexOffset]) + vertexCount;
        m_mesh.m_shape->m_vertexFlags[first] = source->m_shape->m_vertexFlags[index];
        m_mesh.m_shape->m_vertexParameters[first] = source->m_shape->m_vertexParameters[index];
        first++;
        index++;
    }
    for (HalfEdgeMesh::FaceIterator it = source->beginFaces(); it != source->endFaces(); ++it) {
        std::list<MeshVertex *> corners;
        for (MeshFaceIterator corner(*it); corner.isValid(); ++corner)
            corners.push_back(copies[corner.getVertex()]);
        if (reverse)
            corners.reverse();
        m_mesh.addFace(corners, std::vector<char>());
    }
    if (update)
        m_mesh.updateShape();
    *vertex = copies[*vertex];
}

// The selection flags are bool attributes of the elements at the offsets the editable mesh reserved for them.
void MeshInterface::setVertexSelected(MeshVertex *vertex, bool selected)
{
    vertex->m_attributes[m_mesh.m_vertexSelectedOffset] = selected;
}

void MeshInterface::setEdgeSelected(MeshHalfEdge *edge, bool selected)
{
    edge->m_attributes[m_mesh.m_edgeSelectedOffset] = selected;
}

void MeshInterface::setFaceSelected(MeshFace *face, bool selected)
{
    face->m_attributes[m_mesh.m_faceSelectedOffset] = selected;
}

bool MeshInterface::isVertexSelected(MeshVertex *vertex)
{
    return *reinterpret_cast<bool *>(&vertex->m_attributes[m_mesh.m_vertexSelectedOffset]);
}

bool MeshInterface::isEdgeSelected(MeshHalfEdge *edge)
{
    return edge->m_attributes[m_mesh.m_edgeSelectedOffset] ||
           edge->m_opposite && edge->m_opposite->m_attributes[m_mesh.m_edgeSelectedOffset];
}

bool MeshInterface::isFaceSelected(MeshFace *face)
{
    return *reinterpret_cast<bool *>(&face->m_attributes[m_mesh.m_faceSelectedOffset]);
}

// Adds the offsets from the origin of the vertices on the sheet (of the selected vertices for the vertex, edge and
// face tools; extruded for the extrusion tool).
void MeshInterface::getSnapOffsets(const plasma::Vector<2, float> &origin, std::list<plasma::Vector<2, float>> &offsets)
{
    plasma::Matrix<float> matrix = m_hasMatrix ? m_sheet->m_workspace->getTextureSheet()->m_root->m_inverseMatrix *
                                                     m_matrix * m_mesh.m_shape->getTextureMatrix()
                                               : m_sheet->m_root->m_inverseMatrix * m_node->m_node->m_matrix;
    int tool = m_sheet->m_workspace->m_toolModule->getCurrentTool()->getType();
    if (tool == Tool::TOOL_EXTRUSION)
        matrix *= m_mesh.m_shape->m_extrusionMatrix.value();
    if (tool == Tool::TOOL_VERTEX || tool == Tool::TOOL_EDGE || tool == Tool::TOOL_FACE) {
        for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
            if (isVertexSelected(*it))
                offsets.push_back(matrix.transformPoint(getVertexPosition(*it)) - origin);
        }
    } else {
        for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it)
            offsets.push_back(matrix.transformPoint(getVertexPosition(*it)) - origin);
    }
}

// The matrix the mesh is drawn with: the node's, or the texture's while the interface edits the texture.
void MeshInterface::getMatrix(plasma::Matrix<float> &matrix)
{
    if (m_hasMatrix) {
        plasma::Matrix<float> textureMatrix = m_matrix;
        textureMatrix *= m_mesh.m_shape->m_textureMatrix;
        matrix = textureMatrix;
    } else {
        matrix = m_node->m_node->m_matrix;
    }
}

// Binds the meshes in the sheet node's shown subtree to the shape and starts their modification.
static void bindShapeRecursively(NodeInterface *node, plasma::SmoothMeshShape *shape,
                                 const plasma::Matrix<float> &matrix)
{
    if (node->m_node->isShown() && !node->m_node->isLocked()) {
        MeshInterface *mesh = static_cast<MeshInterface *>(node->m_shapeInterface);
        if (mesh && mesh->m_type == INTERFACE_MESH) {
            plasma::Matrix<float> nodeMatrix = matrix;
            nodeMatrix *= node->m_node->m_matrix;
            mesh->bindToShape(shape, nodeMatrix);
            mesh->beginModify();
        }
        for (std::list<plasma::Node *>::iterator it = node->m_node->m_children.begin();
             it != node->m_node->m_children.end(); ++it)
            bindShapeRecursively(node->m_sheet->findSheetNode(*it), shape, matrix);
    }
}

static void followShapeRecursively(NodeInterface *node, plasma::SmoothMeshShape *shape,
                                   const plasma::Matrix<float> &matrix)
{
    if (node->m_node->isShown() && !node->m_node->isLocked()) {
        MeshInterface *mesh = static_cast<MeshInterface *>(node->m_shapeInterface);
        if (mesh && mesh->m_type == INTERFACE_MESH && !node->m_node->fitsShapeToChildren()) {
            plasma::Matrix<float> nodeMatrix = node->m_node->m_inverseMatrix;
            nodeMatrix *= matrix;
            mesh->followShape(shape, nodeMatrix);
        }
        for (std::list<plasma::Node *>::iterator it = node->m_node->m_children.begin();
             it != node->m_node->m_children.end(); ++it)
            followShapeRecursively(node->m_sheet->findSheetNode(*it), shape, matrix);
    }
}

// Starts a drag: a node whose children follow its mesh binds their meshes to it; one whose children are saved (bit
// 8) saves their node states.
void MeshInterface::prepareChildren()
{
    if (m_node->m_node->isDeformer()) {
        for (std::list<plasma::Node *>::iterator it = m_node->m_node->m_children.begin();
             it != m_node->m_node->m_children.end(); ++it)
            bindShapeRecursively(m_sheet->findSheetNode(*it), m_mesh.m_shape, m_node->m_node->m_inverseMatrix);
    } else if (m_node->m_node->alignsToOutline()) {
        for (std::list<plasma::Node *>::iterator it = m_node->m_node->m_children.begin();
             it != m_node->m_node->m_children.end(); ++it) {
            if ((*it)->isShown())
                m_sheet->findSheetNode(*it)->saveState();
        }
    }
}

// Moves the selected vertices (or their texture positions) by the drag offset on the sheet, weighted by the vertex
// weights while the vertex module's option is on and a vertex, edge or face tool is current. The vertex parameters
// of the moved contour vertices are updated.
void MeshInterface::moveSelectedVertices(const plasma::Vector<2, float> &drag)
{
    countSelectedVertices();
    bool moved = false;
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    matrix.m_elements[2] = 0.0f;
    matrix.m_elements[6] = 0.0f;
    matrix.m_elements[10] = 1.0f;
    matrix.m_elements[14] = 0.0f;
    matrix.invert();
    bool movedRoundedCorner = false;
    bool hasRoundedCorner = false;
    plasma::Vector<2, float> offset = matrix.transformDirection(drag);
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        if (isVertexSelected(*it)) {
            plasma::Vector<2, float> delta = offset;
            Workspace *workspace = m_sheet->m_workspace;
            if (workspace->m_vertexModule->m_softSelection) {
                int tool = workspace->getToolModule()->getCurrentTool()->getType();
                if (tool == Tool::TOOL_VERTEX || tool == Tool::TOOL_EDGE || tool == Tool::TOOL_FACE)
                    delta = delta * m_softSelectionWeights[vertexIndex(*it)];
            }
            if (m_hasMatrix) {
                plasma::Vector<2, float> &textureCoordinate =
                    m_mesh.m_shape->m_vertexTexCoords.value()[vertexIndex(*it)];
                textureCoordinate.m_elements[0] += delta.m_elements[0];
                textureCoordinate.m_elements[1] += delta.m_elements[1];
            } else {
                plasma::Vector<2, float> position = getVertexPosition(*it) + delta;
                m_mesh.m_shape->m_vertexPositions.value()[vertexIndex(*it)] = undeform(position);
            }
            if ((m_mesh.m_shape->m_vertexFlags[vertexIndex(*it)] & plasma::VERTEX_CONSTRAINT_MASK) ==
                (1 << plasma::VERTEX_ROUNDED_CORNER))
                movedRoundedCorner = true;
            moved = true;
        }
        if ((m_mesh.m_shape->m_vertexFlags[vertexIndex(*it)] & plasma::VERTEX_CONSTRAINT_MASK) ==
            (1 << plasma::VERTEX_ROUNDED_CORNER))
            hasRoundedCorner = true;
    }
    if (!moved)
        return;
    if (!movedRoundedCorner && hasRoundedCorner) {
        for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
            if (isVertexSelected(*it) && (m_mesh.m_shape->m_vertexFlags[vertexIndex(*it)] &
                                          plasma::VERTEX_CONSTRAINT_MASK) != (1 << plasma::VERTEX_ROUNDED_CORNER))
                m_mesh.m_shape->updateVertexParameter(vertexIndex(*it));
        }
    }
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        if (isVertexSelected(*it) && (m_mesh.m_shape->m_vertexFlags[vertexIndex(*it)] &
                                      plasma::VERTEX_CONSTRAINT_MASK) == (1 << plasma::VERTEX_FIXED_ON_EDGE))
            m_mesh.m_shape->updateVertexParameter(vertexIndex(*it));
    }
    if (m_hasMatrix) {
        m_mesh.m_shape->m_vertexTexCoords.m_changed = true;
    } else {
        m_mesh.m_shape->m_vertexPositions.m_changed = true;
        if (m_mesh.m_shape->m_flags & (1 << plasma::MESH_CONTOUR))
            m_mesh.m_shape->m_vertexColors.m_changed = true;
    }
    m_mesh.m_shape->applyVertexParameters();
    m_hasChanges = true;
    m_refreshPending = true;
}

void MeshInterface::updateChildren()
{
    if (m_node->m_node->isDeformer()) {
        for (std::list<plasma::Node *>::iterator it = m_node->m_node->m_children.begin();
             it != m_node->m_node->m_children.end(); ++it)
            followShapeRecursively(m_sheet->findSheetNode(*it), m_mesh.m_shape, m_node->m_node->m_matrix);
    }
    if (m_node->m_node->alignsToOutline())
        alignChildrenToOutline();
}

// Scales the selected vertices (or their texture positions) about the pivot on the sheet.
void MeshInterface::scaleSelectedVertices(const plasma::Vector<2, float> &pivot, const plasma::Vector<2, float> &scale)
{
    bool moved = false;
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    matrix.m_elements[2] = 0.0f;
    matrix.m_elements[6] = 0.0f;
    matrix.m_elements[10] = 1.0f;
    matrix.m_elements[14] = 0.0f;
    plasma::Matrix<float> inverse = matrix;
    inverse.invert();
    plasma::Matrix<float> transform;
    transform.identity();
    transform *= inverse;
    transform.translate(pivot);
    transform.scale(scale.m_elements[0], scale.m_elements[1]);
    transform.translate(pivot * -1.0f);
    transform *= matrix;
    bool movedRoundedCorner = false;
    bool hasRoundedCorner = false;
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        if (isVertexSelected(*it)) {
            plasma::Vector<2, float> *position;
            if (m_hasMatrix)
                position = &m_mesh.m_shape->m_vertexTexCoords.value()[vertexIndex(*it)];
            else
                position = &m_mesh.m_shape->m_vertexPositions.value()[vertexIndex(*it)];
            position->transform(transform);
            if ((m_mesh.m_shape->m_vertexFlags[vertexIndex(*it)] & plasma::VERTEX_CONSTRAINT_MASK) ==
                (1 << plasma::VERTEX_ROUNDED_CORNER))
                movedRoundedCorner = true;
            moved = true;
        }
        if ((m_mesh.m_shape->m_vertexFlags[vertexIndex(*it)] & plasma::VERTEX_CONSTRAINT_MASK) ==
            (1 << plasma::VERTEX_ROUNDED_CORNER))
            hasRoundedCorner = true;
    }
    if (!moved)
        return;
    if (!movedRoundedCorner && hasRoundedCorner) {
        for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
            if (isVertexSelected(*it) && (m_mesh.m_shape->m_vertexFlags[vertexIndex(*it)] &
                                          plasma::VERTEX_CONSTRAINT_MASK) != (1 << plasma::VERTEX_ROUNDED_CORNER))
                m_mesh.m_shape->updateVertexParameter(vertexIndex(*it));
        }
    }
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        if (isVertexSelected(*it) && (m_mesh.m_shape->m_vertexFlags[vertexIndex(*it)] &
                                      plasma::VERTEX_CONSTRAINT_MASK) == (1 << plasma::VERTEX_FIXED_ON_EDGE))
            m_mesh.m_shape->updateVertexParameter(vertexIndex(*it));
    }
    if (m_hasMatrix) {
        m_mesh.m_shape->m_vertexTexCoords.m_changed = true;
    } else {
        m_mesh.m_shape->m_vertexPositions.m_changed = true;
        if (m_mesh.m_shape->m_flags & (1 << plasma::MESH_CONTOUR))
            m_mesh.m_shape->m_vertexColors.m_changed = true;
    }
    m_mesh.m_shape->applyVertexParameters();
    m_hasChanges = true;
    m_refreshPending = true;
}

// Rotates the selected vertices (or their texture positions) about the pivot on the sheet by the rotation's z
// angle in degrees; with the vertex module's weighting on, the selection tools scale the angle by each vertex's
// weight.
void MeshInterface::rotateSelectedVertices(const plasma::Vector<2, float> &pivot,
                                           const plasma::Vector<3, float> &rotation)
{
    bool moved = false;
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    plasma::Matrix<float> inverse = matrix;
    inverse.invert();
    plasma::Matrix<float> transform;
    transform.identity();
    transform *= inverse;
    transform.translate(pivot);
    transform.rotateZ(rotation.m_elements[2]);
    transform.translate(pivot * -1.0f);
    transform *= matrix;
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        if (isVertexSelected(*it)) {
            Workspace *workspace = m_sheet->m_workspace;
            if (workspace->m_vertexModule->m_softSelection) {
                int tool = workspace->getToolModule()->getCurrentTool()->getType();
                if (tool == Tool::TOOL_VERTEX || tool == Tool::TOOL_EDGE || tool == Tool::TOOL_FACE) {
                    transform.identity();
                    transform *= inverse;
                    transform.translate(pivot);
                    transform.rotateZ(m_softSelectionWeights[vertexIndex(*it)] * rotation.m_elements[2]);
                    transform.translate(pivot * -1.0f);
                    transform *= matrix;
                }
            }
            plasma::Vector<2, float> *position;
            if (m_hasMatrix)
                position = &m_mesh.m_shape->m_vertexTexCoords.value()[vertexIndex(*it)];
            else
                position = &m_mesh.m_shape->m_vertexPositions.value()[vertexIndex(*it)];
            position->transform(transform);
            moved = true;
        }
    }
    if (!moved)
        return;
    if (m_hasMatrix) {
        m_mesh.m_shape->m_vertexTexCoords.m_changed = true;
    } else {
        m_mesh.m_shape->m_vertexPositions.m_changed = true;
        if (m_mesh.m_shape->m_flags & (1 << plasma::MESH_CONTOUR))
            m_mesh.m_shape->m_vertexColors.m_changed = true;
    }
    m_mesh.m_shape->applyVertexParameters();
    m_hasChanges = true;
    m_refreshPending = true;
}

// Moves the extrusion by the drag offset (in the mesh's coordinates) when a vertex is selected.
void MeshInterface::moveExtrusion()
{
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    matrix.invert();
    plasma::Vector<2, float> offset;
    offset = matrix.transformDirection(m_sheet->m_workspace->getDragMovement());
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        if (isVertexSelected(*it)) {
            plasma::Matrix<float> &extrusion = m_mesh.m_shape->m_extrusionMatrix.value();
            for (int i = 0; i < 4; i++)
                extrusion.m_elements[12 + i] = extrusion.m_elements[4 + i] * offset.m_elements[1] +
                                               extrusion.m_elements[i] * offset.m_elements[0] +
                                               extrusion.m_elements[12 + i];
            m_mesh.m_shape->applyVertexParameters();
            m_hasChanges = true;
            m_refreshPending = true;
            return;
        }
    }
}

// Copies the mesh shape's editable state. While the face tool draws a face into this mesh, the state also records
// the drawn vertices (as vertex indices) and the face's index.
void MeshInterface::captureState(MeshState &state)
{
    state.m_vertexPositions = m_mesh.m_shape->m_vertexPositions;
    state.m_vertexColors = m_mesh.m_shape->m_vertexColors;
    state.m_vertexTexCoords = m_mesh.m_shape->m_vertexTexCoords;
    state.m_strokeColors = m_mesh.m_shape->m_strokeColors;
    state.m_extrusionFrontColors = m_mesh.m_shape->m_extrusionFrontColors;
    state.m_extrusionBackColors = m_mesh.m_shape->m_extrusionBackColors;
    state.m_strokeWidths = m_mesh.m_shape->m_strokeWidths;
    state.m_texture = m_mesh.m_shape->m_texture;
    state.m_strokeTexture = m_mesh.m_shape->m_strokeTexture;
    state.m_textureTranslation = m_mesh.m_shape->m_textureTranslation;
    state.m_textureRotation = m_mesh.m_shape->m_textureRotation;
    state.m_texturePivot = m_mesh.m_shape->m_texturePivot;
    state.m_textureDeformation = m_mesh.m_shape->m_textureDeformation;
    state.m_textureOpacity = m_mesh.m_shape->m_textureOpacity;
    state.m_textureBrightness = m_mesh.m_shape->m_textureBrightness;
    state.m_textureContrast = m_mesh.m_shape->m_textureContrast;
    state.m_textureSaturation = m_mesh.m_shape->m_textureSaturation;
    state.m_strokeTextureOpacity = m_mesh.m_shape->m_strokeTextureOpacity;
    state.m_strokeTextureBrightness = m_mesh.m_shape->m_strokeTextureBrightness;
    state.m_strokeTextureContrast = m_mesh.m_shape->m_strokeTextureContrast;
    state.m_strokeTextureSaturation = m_mesh.m_shape->m_strokeTextureSaturation;
    state.m_strokeTextureStretch = m_mesh.m_shape->m_strokeTextureStretch;
    state.m_extrusionMatrix = m_mesh.m_shape->m_extrusionMatrix;
    state.m_vertexFlags = m_mesh.m_shape->m_vertexFlags;
    state.m_vertexParameters = m_mesh.m_shape->m_vertexParameters;
    state.m_faces = m_mesh.m_shape->m_faces;
    state.m_subdivisions = m_mesh.m_shape->getSubdivisions();
    state.m_filled = !(m_mesh.m_shape->m_flags & (1 << plasma::MESH_NO_FILL));
    state.m_contour = m_mesh.m_shape->m_flags & (1 << plasma::MESH_CONTOUR);
    state.m_closed = !(m_mesh.m_shape->m_flags & (1 << plasma::MESH_OPEN));
    state.m_stroke = (m_mesh.m_shape->m_flags & (1 << plasma::MESH_STROKE)) != 0;
    state.m_extruded = (m_mesh.m_shape->m_flags & (1 << plasma::MESH_EXTRUDED)) != 0;
    state.m_strokeCapType = m_mesh.m_shape->getStrokeCapType();
    state.m_strokeJointType = m_mesh.m_shape->getStrokeJointType();
    state.m_strokePattern = m_mesh.m_shape->getStrokePattern();
    state.m_strokeDash = m_mesh.m_shape->getStrokeDash();
    state.m_strokeGap = m_mesh.m_shape->getStrokeGap();
    state.m_drawnVertexIndices.clear();
    state.m_drawnFaceIndex = -1;
    if (this == m_sheet->m_workspace->getToolModule()->m_drawFaceTool.m_meshInterface &&
        m_sheet->m_workspace->getToolModule()->m_drawFaceTool.m_vertices.size() != 0) {
        std::map<MeshVertex *, int> indices;
        int index = 0;
        for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it)
            indices[*it] = index++;
        for (std::list<MeshVertex *>::iterator it =
                 m_sheet->m_workspace->getToolModule()->m_drawFaceTool.m_vertices.begin();
             it != m_sheet->m_workspace->getToolModule()->m_drawFaceTool.m_vertices.end(); ++it)
            state.m_drawnVertexIndices.push_back(indices[*it]);
        index = 0;
        for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it) {
            if (*it == m_sheet->m_workspace->getToolModule()->m_drawFaceTool.m_face) {
                state.m_drawnFaceIndex = index;
                break;
            }
            index++;
        }
    }
}

// Puts a captured state back (MeshOperation::Do and Undo): the shape's attributes and settings, then the editable
// mesh, keeping the selections when the element counts still match, and the face being drawn.
void MeshInterface::restoreState(const MeshState &state)
{
    std::vector<bool> vertexSelection = getVertexSelection();
    std::vector<bool> edgeSelection = getEdgeSelection();
    std::vector<bool> faceSelection = getFaceSelection();
    m_mesh.m_shape->m_vertexPositions = state.m_vertexPositions;
    m_mesh.m_shape->m_vertexColors = state.m_vertexColors;
    m_mesh.m_shape->m_vertexTexCoords = state.m_vertexTexCoords;
    m_mesh.m_shape->m_strokeColors = state.m_strokeColors;
    m_mesh.m_shape->m_extrusionFrontColors = state.m_extrusionFrontColors;
    m_mesh.m_shape->m_extrusionBackColors = state.m_extrusionBackColors;
    m_mesh.m_shape->m_strokeWidths = state.m_strokeWidths;
    m_mesh.m_shape->m_texture = state.m_texture;
    m_mesh.m_shape->m_strokeTexture = state.m_strokeTexture;
    m_mesh.m_shape->m_textureTranslation = state.m_textureTranslation;
    m_mesh.m_shape->m_textureRotation = state.m_textureRotation;
    m_mesh.m_shape->m_texturePivot = state.m_texturePivot;
    m_mesh.m_shape->m_textureDeformation = state.m_textureDeformation;
    m_mesh.m_shape->m_textureOpacity = state.m_textureOpacity;
    m_mesh.m_shape->m_textureBrightness = state.m_textureBrightness;
    m_mesh.m_shape->m_textureContrast = state.m_textureContrast;
    m_mesh.m_shape->m_textureSaturation = state.m_textureSaturation;
    m_mesh.m_shape->m_strokeTextureOpacity = state.m_strokeTextureOpacity;
    m_mesh.m_shape->m_strokeTextureBrightness = state.m_strokeTextureBrightness;
    m_mesh.m_shape->m_strokeTextureContrast = state.m_strokeTextureContrast;
    m_mesh.m_shape->m_strokeTextureSaturation = state.m_strokeTextureSaturation;
    m_mesh.m_shape->m_strokeTextureStretch = state.m_strokeTextureStretch;
    m_mesh.m_shape->m_extrusionMatrix = state.m_extrusionMatrix;
    m_mesh.m_shape->m_vertexFlags = state.m_vertexFlags;
    m_mesh.m_shape->m_vertexParameters = state.m_vertexParameters;
    m_mesh.m_shape->setSubdivisions(state.m_subdivisions, false);
    m_mesh.m_shape->setFaces(state.m_faces, false);
    m_mesh.m_shape->setStrokeCapType(state.m_strokeCapType);
    m_mesh.m_shape->setStrokeJointType(state.m_strokeJointType);
    m_mesh.m_shape->setStrokePattern(state.m_strokePattern);
    m_mesh.m_shape->setStrokeDash(state.m_strokeDash);
    m_mesh.m_shape->setStrokeGap(state.m_strokeGap);
    clearShapeFlag(m_mesh.m_shape, (1 << plasma::MESH_OPEN), state.m_closed);
    clearShapeFlag(m_mesh.m_shape, (1 << plasma::MESH_NO_FILL), state.m_filled);
    setShapeFlag(m_mesh.m_shape, (1 << plasma::MESH_CONTOUR), state.m_contour);
    setShapeFlag(m_mesh.m_shape, (1 << plasma::MESH_STROKE), state.m_stroke);
    setShapeFlag(m_mesh.m_shape, (1 << plasma::MESH_EXTRUDED), state.m_extruded);
    m_mesh.m_shape->updateSubdivision();
    m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    m_mesh.rebuild();
    if (m_mesh.getVertexCount() == vertexSelection.size() && m_mesh.getEdgeCount() == edgeSelection.size() &&
        m_mesh.getFaceCount() == faceSelection.size()) {
        int index = 0;
        for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it)
            (*it)->m_attributes[m_mesh.m_vertexSelectedOffset] = vertexSelection[index++];
        index = 0;
        for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it)
            (*it)->m_attributes[m_mesh.m_edgeSelectedOffset] = edgeSelection[index++];
        index = 0;
        for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it)
            (*it)->m_attributes[m_mesh.m_faceSelectedOffset] = faceSelection[index++];
    }
    std::list<MeshVertex *> drawnVertices;
    std::map<int, MeshVertex *> vertices;
    int index = 0;
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it)
        vertices[index++] = *it;
    for (std::list<int>::const_iterator it = state.m_drawnVertexIndices.begin(); it != state.m_drawnVertexIndices.end();
         ++it)
        drawnVertices.push_back(vertices[*it]);
    MeshFace *drawnFace = 0;
    int face = 0;
    for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it) {
        if (face == state.m_drawnFaceIndex) {
            drawnFace = *it;
            break;
        }
        face++;
    }
    if (drawnVertices.size())
        m_sheet->m_workspace->getToolModule()->m_drawFaceTool.restoreDrawingState(this, drawnVertices, drawnFace);
    else
        m_sheet->m_workspace->getToolModule()->m_drawFaceTool.restoreDrawingState(0, drawnVertices, 0);
    m_sheet->rebuildShapeSharers(this);
    refresh();
}

// Corner modes occupy the low half; rounded-corner and fixed-on-edge constraints occupy the high half and are
// preserved when changing the mode.
void MeshInterface::smoothVertices()
{
    MeshState before;
    captureState(before);
    bool changed = false;
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        if (isVertexSelected(*it) &&
            (m_mesh.m_shape->m_vertexFlags[m_mesh.vertexIndex(*it)] & plasma::VERTEX_KIND_MASK)) {
            changed = true;
            m_mesh.m_shape->m_vertexFlags[m_mesh.vertexIndex(*it)] &= plasma::VERTEX_CONSTRAINT_MASK;
        }
    }
    if (changed) {
        m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        MeshState after;
        captureState(after);
        m_sheet->addOperation(new MeshOperation(m_sheet, "smooth vertices", this, before, after));
        this->refresh();
    }
}

void MeshInterface::sharpenVertices()
{
    MeshState before;
    captureState(before);
    bool changed = false;
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        if (isVertexSelected(*it) &&
            (mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] & plasma::VERTEX_KIND_MASK) != plasma::VERTEX_SHARP) {
            mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] &= plasma::VERTEX_CONSTRAINT_MASK;
            mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] |= plasma::VERTEX_SHARP;
            changed = true;
        }
    }
    if (changed) {
        m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        MeshState after;
        captureState(after);
        m_sheet->addOperation(new MeshOperation(m_sheet, "sharpen vertices", this, before, after));
        this->refresh();
    }
}

void MeshInterface::bezierVertices()
{
    MeshState before;
    captureState(before);
    bool changed = false;
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        if (isVertexSelected(*it) &&
            (mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] & plasma::VERTEX_KIND_MASK) != plasma::VERTEX_BEZIER) {
            mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] &= plasma::VERTEX_CONSTRAINT_MASK;
            mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] |= plasma::VERTEX_BEZIER;
            changed = true;
        }
    }
    if (changed) {
        m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        MeshState after;
        captureState(after);
        m_sheet->addOperation(new MeshOperation(m_sheet, "bezier vertices", this, before, after));
        this->refresh();
    }
}

void MeshInterface::roundCornerVertices()
{
    MeshState before;
    captureState(before);
    bool changed = false;
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        if (isVertexSelected(*it) &&
            (mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] & plasma::VERTEX_KIND_MASK) != plasma::VERTEX_ROUND) {
            mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] &= plasma::VERTEX_CONSTRAINT_MASK;
            mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] |= plasma::VERTEX_ROUND;
            changed = true;
        }
    }
    if (changed) {
        m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        MeshState after;
        captureState(after);
        m_sheet->addOperation(new MeshOperation(m_sheet, "round corner vertices", this, before, after));
        this->refresh();
    }
}

void MeshInterface::smoothAllVertices()
{
    MeshState before;
    captureState(before);
    bool changed = false;
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        if (mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] & plasma::VERTEX_BEZIER) {
            mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] &= ~plasma::VERTEX_BEZIER;
            changed = true;
        }
    }
    if (changed) {
        mesh.updateShape();
        MeshState after;
        captureState(after);
        m_sheet->addOperation(new MeshOperation(m_sheet, "smooth vertices", this, before, after));
        this->refresh();
    }
}

void MeshInterface::sharpenAllVertices()
{
    MeshState before;
    captureState(before);
    bool changed = false;
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        if (!(mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] & plasma::VERTEX_BEZIER)) {
            mesh.m_shape->m_vertexFlags[mesh.vertexIndex(*it)] |= plasma::VERTEX_BEZIER;
            changed = true;
        }
    }
    if (changed) {
        mesh.updateShape();
        MeshState after;
        captureState(after);
        m_sheet->addOperation(new MeshOperation(m_sheet, "sharpen vertices", this, before, after));
        this->refresh();
    }
}

void MeshInterface::subdivideMesh()
{
    // An outline without a fill is subdivided as a contour.
    if (!plasma::SmoothMeshShape::hasFill(m_mesh.m_shape->m_flags)) {
        subdivideContour();
        return;
    }
    MeshState before;
    captureState(before);
    m_mesh.refine();
    MeshState after;
    captureState(after);
    m_sheet->addOperation(new MeshOperation(m_sheet, "subdivide mesh", this, before, after));
    refresh();
}

// Mesh menu "Subdivide Contour": inserts a point in every boundary edge and places the contour points as one
// subdivision step of the contour would.
void MeshInterface::subdivideContour()
{
    MeshState before;
    captureState(before);
    std::vector<std::vector<plasma::Vector<2, float>>> positions;
    std::vector<std::vector<int>> types;
    for (int c = 0; c < (int)m_mesh.m_shape->m_contours.size(); c++) {
        std::vector<int> &contour = m_mesh.m_shape->m_contours[c];
        int count;
        if (isClosed(m_mesh.m_shape->m_flags))
            count = 2 * contour.size();
        else
            count = 2 * contour.size() - 1;
        std::vector<plasma::Vector<2, float>> points(count);
        std::vector<int> pointTypes(count, 0);
        for (int i = 0; i < (int)contour.size(); i++) {
            points[2 * i] = m_mesh.m_shape->m_vertexPositions.value()[contour[i]];
            pointTypes[2 * i] = m_mesh.m_shape->m_vertexFlags[contour[i]];
        }
        refineContourPoints(pointTypes, 2, points, count / 2, isClosed(m_mesh.m_shape->m_flags),
                            m_mesh.m_shape->m_smoothWeight, 0);
        positions.push_back(points);
        types.push_back(pointTypes);
    }

    // An open contour keeps the edge that closes it.
    MeshHalfEdge *closing = 0;
    if (!isClosed(m_mesh.m_shape->m_flags) && (int)m_mesh.getFaceCount() > 0)
        closing = (*m_mesh.beginFaces())->m_edge->m_previous;
    std::list<MeshHalfEdge *> edges;
    for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it) {
        if (*it != closing && !(*it)->m_opposite)
            edges.push_back(*it);
    }
    for (std::list<MeshHalfEdge *>::iterator it = edges.begin(); it != edges.end(); ++it)
        m_mesh.subdivideEdge(*it);
    m_mesh.updateShape();
    for (int c = 0; c < (int)m_mesh.m_shape->m_contours.size(); c++) {
        std::vector<int> &contour = m_mesh.m_shape->m_contours[c];
        for (int i = 0; i < (int)contour.size(); i++) {
            m_mesh.m_shape->m_vertexPositions.value()[contour[i]] = positions[c][i];
            m_mesh.m_shape->m_vertexFlags[contour[i]] = types[c][i];
            if ((unsigned short)m_mesh.m_shape->m_vertexFlags[contour[i]] == plasma::VERTEX_ROUND)
                m_mesh.m_shape->m_vertexFlags[contour[i]] = plasma::VERTEX_SMOOTH;
        }
    }
    m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    MeshState after;
    captureState(after);
    m_sheet->addOperation(new MeshOperation(m_sheet, "subdivide contour", this, before, after));
    refresh();
}

void MeshInterface::addBorder()
{
    MeshState before;
    captureState(before);
    m_mesh.subdivideBoundaryCorners();
    MeshState after;
    captureState(after);
    m_sheet->addOperation(new MeshOperation(m_sheet, "add border", this, before, after));
    refresh();
}

void MeshInterface::setSubdivisions(int subdivisions)
{
    m_mesh.m_shape->setSubdivisions(subdivisions, true);
}

int MeshInterface::getSubdivisions()
{
    return m_mesh.m_shape->m_subdivisions;
}

// Deletes the selected vertices; each remaining face then starts after the last unselected corner seen. A mesh
// left without vertices deletes its node.
bool MeshInterface::deleteVertices()
{
    MeshState before;
    captureState(before);
    std::list<MeshVertex *> vertices;
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        if (isVertexSelected(*it))
            vertices.push_back(*it);
    }
    if (!vertices.size())
        return false;
    MeshVertex *kept = 0;
    for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it) {
        for (MeshFaceIterator corner(*it); corner.isValid(); ++corner) {
            if (!isVertexSelected(corner.getVertex()))
                kept = corner.getVertex();
        }
    }
    m_mesh.removeVertices(vertices);
    for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it) {
        for (MeshFaceIterator corner(*it); corner.isValid(); ++corner) {
            if (corner.getEdge()->m_previous->m_vertex == kept) {
                (*it)->m_edge = corner.getEdge();
                break;
            }
        }
    }
    m_mesh.updateShape();
    MeshState after;
    captureState(after);
    m_sheet->addOperation(new MeshOperation(m_sheet, "delete vertices", this, before, after));
    if (!m_mesh.getVertexCount())
        m_sheet->deleteNode(m_node->m_node);
    refresh();
    return true;
}

bool MeshInterface::deleteEdges()
{
    MeshState before;
    captureState(before);
    std::list<MeshHalfEdge *> edges;
    for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it) {
        if (isEdgeSelected(*it))
            edges.push_back(*it);
    }
    if (!edges.size())
        return false;
    m_mesh.dissolveEdges(edges);
    m_mesh.updateShape();
    selectAllVertices(false);
    MeshState after;
    captureState(after);
    m_sheet->addOperation(new MeshOperation(m_sheet, "delete edges", this, before, after));
    refresh();
    return true;
}

bool MeshInterface::deleteFaces()
{
    MeshState before;
    captureState(before);
    std::list<MeshFace *> faces;
    for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it) {
        if (isFaceSelected(*it))
            faces.push_back(*it);
    }
    if (!faces.size())
        return false;
    m_mesh.removeFaces(faces);
    m_mesh.updateShape();
    selectAllVertices(false);
    MeshState after;
    captureState(after);
    m_sheet->addOperation(new MeshOperation(m_sheet, "delete faces", this, before, after));
    refresh();
    return true;
}

// True when the point is strictly inside the triangle (on the same side of all three edges, either winding).
bool isPointInTriangle(const plasma::Vector<2, float> *corners, const plasma::Vector<2, float> &point)
{
    int positive = 0;
    int negative = 0;
    for (int i = 0; i < 3; ++i) {
        const plasma::Vector<2, float> &corner = corners[i];
        const plasma::Vector<2, float> &next = corners[(i + 1) % 3];
        plasma::Vector<2, float> normal(corner.m_elements[1] - next.m_elements[1],
                                        next.m_elements[0] - corner.m_elements[0]);
        plasma::Vector<2, float> offset = point - corner;
        float side = dot(offset, normal);
        if (side > 0)
            ++positive;
        if (side < 0)
            ++negative;
    }
    return positive == 3 || negative == 3;
}

bool MeshInterface::isFaceUnderMouse(MeshFace *face)
{
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    for (MeshFaceIterator corner(face); corner.isValid(); ++corner) {
        MeshHalfEdge *edge;
        plasma::Vector<2, float> corners[3] = {
            (matrix * getVertexPosition(face->m_edge->m_vertex)),
            (matrix * getVertexPosition((edge = corner.getEdge())->m_vertex)),
            (matrix * getVertexPosition(edge->m_next->m_vertex)),
        };
        if (isPointInTriangle(corners, m_sheet->m_workspace->m_sceneMousePosition))
            return true;
        float radius = m_sheet->m_workspace->m_toolRadius;
        if (squaredDistanceToSegment(m_sheet->m_workspace->m_sceneMousePosition, corners[1], corners[2]) <=
            (float)(radius * radius))
            return true;
    }
    return false;
}

// The texture module's target picks the fill or the stroke texture. Each keys one attribute at frame 0 of the
// current sequence.
bool MeshInterface::setVertexPositionsKey()
{
    MeshState before;
    captureState(before);
    const std::wstring &sequence = m_sheet->m_workspace->m_sequenceModule->m_sequenceName;
    m_mesh.m_shape->m_vertexPositions.setKey(sequence, 0, 1.0f);
    MeshState after;
    captureState(after);
    m_sheet->addOperation(new MeshOperation(m_sheet, "set vertex positions key", this, before, after));
    return true;
}

bool MeshInterface::setVertexColorsKey()
{
    MeshState before;
    captureState(before);
    const std::wstring &sequence = m_sheet->m_workspace->m_sequenceModule->m_sequenceName;
    m_mesh.m_shape->m_vertexColors.setKey(sequence, 0, 1.0f);
    MeshState after;
    captureState(after);
    m_sheet->addOperation(new MeshOperation(m_sheet, "set vertex colors key", this, before, after));
    return true;
}

bool MeshInterface::setVertexTextureCoordsKey()
{
    MeshState before;
    captureState(before);
    const std::wstring &sequence = m_sheet->m_workspace->m_sequenceModule->m_sequenceName;
    m_mesh.m_shape->m_vertexTexCoords.setKey(sequence, 0, 1.0f);
    MeshState after;
    captureState(after);
    m_sheet->addOperation(new MeshOperation(m_sheet, "set vertex texture coords key", this, before, after));
    return true;
}

bool MeshInterface::setStrokeColorsKey()
{
    MeshState before;
    captureState(before);
    const std::wstring &sequence = m_sheet->m_workspace->m_sequenceModule->m_sequenceName;
    m_mesh.m_shape->m_strokeColors.setKey(sequence, 0, 1.0f);
    MeshState after;
    captureState(after);
    m_sheet->addOperation(new MeshOperation(m_sheet, "set stroke colors key", this, before, after));
    return true;
}

bool MeshInterface::setStrokeWidthsKey()
{
    MeshState before;
    captureState(before);
    const std::wstring &sequence = m_sheet->m_workspace->m_sequenceModule->m_sequenceName;
    m_mesh.m_shape->m_strokeWidths.setKey(sequence, 0, 1.0f);
    MeshState after;
    captureState(after);
    m_sheet->addOperation(new MeshOperation(m_sheet, "set stroke widths key", this, before, after));
    return true;
}

// The bounds of the mesh on the sheet: of the drawn shape while the node tool shows shape boundaries, else of the
// vertices. Empty bounds start with positive minima and negative maxima.
void MeshInterface::getBounds(plasma::Vector<2, float> *minimum, plasma::Vector<2, float> *maximum)
{
    minimum->m_elements[0] = 1e10f;
    minimum->m_elements[1] = 1e10f;
    maximum->m_elements[0] = -1e10f;
    maximum->m_elements[1] = -1e10f;
    bool first = true;
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    if (m_sheet->m_workspace->m_nodeToolModule->m_shapeBoundary) {
        first = true;
        m_mesh.m_shape->getTransformedBounds(*minimum, *maximum, matrix, first);
        return;
    }
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        plasma::Vector<2, float> position = matrix.transform(getVertexPosition(*it));
        if (first) {
            *minimum = position;
            first = false;
            *maximum = position;
        } else {
            if (position.m_elements[0] < minimum->m_elements[0])
                minimum->m_elements[0] = position.m_elements[0];
            if (position.m_elements[1] < minimum->m_elements[1])
                minimum->m_elements[1] = position.m_elements[1];
            if (position.m_elements[0] > maximum->m_elements[0])
                maximum->m_elements[0] = position.m_elements[0];
            if (position.m_elements[1] > maximum->m_elements[1])
                maximum->m_elements[1] = position.m_elements[1];
        }
    }
}

// The bounds of the selected vertices on the sheet. Empty bounds start with positive minima and negative maxima.
void MeshInterface::getSelectionBounds(plasma::Vector<2, float> *minimum, plasma::Vector<2, float> *maximum)
{
    minimum->m_elements[0] = 1e10f;
    minimum->m_elements[1] = 1e10f;
    maximum->m_elements[0] = -1e10f;
    maximum->m_elements[1] = -1e10f;
    bool first = true;
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        if ((*it)->m_attributes[m_mesh.m_vertexSelectedOffset]) {
            plasma::Vector<2, float> position = matrix * getVertexPosition(*it);
            if (first) {
                *minimum = position;
                first = false;
                *maximum = position;
            } else {
                if (position.m_elements[0] < minimum->m_elements[0])
                    minimum->m_elements[0] = position.m_elements[0];
                if (position.m_elements[1] < minimum->m_elements[1])
                    minimum->m_elements[1] = position.m_elements[1];
                if (maximum->m_elements[0] < position.m_elements[0])
                    maximum->m_elements[0] = position.m_elements[0];
                if (maximum->m_elements[1] < position.m_elements[1])
                    maximum->m_elements[1] = position.m_elements[1];
            }
        }
    }
}

void MeshInterface::setTexture(int texture)
{
    if (m_sheet->m_workspace->m_textureModule->m_mode == TextureModule::TEXTURE_STROKE)
        m_mesh.m_shape->m_strokeTexture.value() = texture;
    else
        m_mesh.m_shape->m_texture.value() = texture;
    m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
}

plasma::Texture *MeshInterface::getTexture()
{
    if (m_sheet->m_workspace->m_textureModule->m_mode == TextureModule::TEXTURE_STROKE)
        return m_mesh.m_shape->m_outlineTexture;
    return m_mesh.m_shape->m_fillTexture;
}

double MeshInterface::getTextureOpacity()
{
    if (m_sheet->m_workspace->m_textureModule->m_mode == TextureModule::TEXTURE_STROKE)
        return m_mesh.m_shape->m_strokeTextureOpacity.value();
    return m_mesh.m_shape->m_textureOpacity.value();
}

// Maps the texture over the mesh: each vertex's texture position becomes its position in the node's parent
// relative to the vertex bounds, and the texture transform is reset.
void MeshInterface::fitTextureToMesh()
{
    plasma::Vector<2, float> minimum;
    plasma::Vector<2, float> maximum;
    bool first = true;
    m_mesh.m_shape->getVertexBounds(minimum, maximum, m_node->m_node->m_matrix, first);
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        plasma::Vector<2, float> size = maximum - minimum;
        int index = vertexIndex(*it);
        plasma::Vector<2, float> &position = mesh.m_shape->m_vertexPositions.value()[index];
        mesh.m_shape->m_vertexTexCoords.value()[index] =
            (m_node->m_node->m_matrix.transform(position) - minimum) / size;
    }
    m_mesh.m_shape->m_textureDeformation.value().identity();
    m_mesh.m_shape->m_textureRotation.value() = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    m_mesh.m_shape->m_textureTranslation.value() = plasma::Vector<2, float>(0.0f, 0.0f);
    m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
}

// Maps the texture over the box: each vertex's texture position becomes its position on the sheet relative to the
// box, and the texture transform is reset.
void MeshInterface::fitTextureToBox(const plasma::Vector<2, float> &minimum, const plasma::Vector<2, float> &maximum)
{
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        plasma::Vector<2, float> size = maximum - minimum;
        int index = vertexIndex(*it);
        plasma::Vector<2, float> &position = m_mesh.m_shape->m_vertexPositions.value()[index];
        plasma::Matrix<float> matrix = m_sheet->m_root->m_inverseMatrix;
        matrix *= m_node->m_node->m_matrix;
        m_mesh.m_shape->m_vertexTexCoords.value()[index] = (matrix.transformPoint(position) - minimum) / size;
    }
    m_mesh.m_shape->m_textureDeformation.value().identity();
    m_mesh.m_shape->m_textureRotation.value() = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    m_mesh.m_shape->m_textureTranslation.value() = plasma::Vector<2, float>(0.0f, 0.0f);
    m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
}

// Moves the point to the vertex nearest the mouse within the pick radius. The code returns true for a nonempty
// mesh even when no vertex is in range.
bool MeshInterface::snapToVertex(plasma::Vector<2, float> &point)
{
    if (!m_mesh.getVertexCount())
        return false;
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    float nearest = m_sheet->m_workspace->m_toolRadius * m_sheet->m_workspace->m_toolRadius;
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        plasma::Vector<2, float> position = matrix.transformPoint(getVertexPosition(*it));
        plasma::Vector<2, float> offset = m_sheet->m_workspace->m_sceneMousePosition - position;
        float distance = offset.m_elements[0] * offset.m_elements[0] + offset.m_elements[1] * offset.m_elements[1];
        if (distance <= nearest) {
            nearest = distance;
            point = position;
        }
    }
    return true;
}

void MeshInterface::storeTextureTransform()
{
    m_texturePivot = m_mesh.m_shape->m_texturePivot.value();
    m_textureTranslation = m_mesh.m_shape->m_textureTranslation.value();
    m_textureRotation = m_mesh.m_shape->m_textureRotation.value();
    m_textureDeformation = m_mesh.m_shape->m_textureDeformation.value();
}

// Puts the texture transform kept by StoreTextureTransform back.
void MeshInterface::restoreTextureTransform()
{
    m_mesh.m_shape->m_texturePivot.value() = m_texturePivot;
    m_mesh.m_shape->m_textureTranslation.value() = m_textureTranslation;
    m_mesh.m_shape->m_textureRotation.value() = m_textureRotation;
    m_mesh.m_shape->m_textureDeformation.value() = m_textureDeformation;
    m_mesh.m_shape->updateTextureMatrix();
}

void MeshInterface::storeExtrusionMatrix()
{
    m_extrusionMatrix = m_mesh.m_shape->m_extrusionMatrix.value();
}

void MeshInterface::restoreExtrusionMatrix()
{
    m_mesh.m_shape->m_extrusionMatrix.value() = m_extrusionMatrix;
}

// The shapes keep a second element of the position arrays for this.
void MeshInterface::storePositions()
{
    m_mesh.m_shape->m_vertexPositions.copyFromCurrent(plasma::Attribute::STORED_ELEMENT);
    m_mesh.m_shape->m_vertexTexCoords.copyFromCurrent(plasma::Attribute::STORED_ELEMENT);
}

void MeshInterface::restorePositions()
{
    m_mesh.m_shape->m_vertexPositions.copyToCurrent(plasma::Attribute::STORED_ELEMENT);
    m_mesh.m_shape->m_vertexTexCoords.copyToCurrent(plasma::Attribute::STORED_ELEMENT);
}

bool MeshInterface::isDrawingInBox(plasma::Drawing *drawing, BoundingBoxTool *tool, plasma::Node *node)
{
    if (drawing->m_pathVertices.empty())
        return false;

    // The box of the drawing on the sheet; computed but not used.
    plasma::Vector<2, float> minimum = drawing->m_minimum;
    plasma::Vector<2, float> maximum = drawing->m_maximum;
    if (node->m_enclosingWidget) {
        minimum = node->m_enclosingWidget->deform(minimum, node);
        maximum = node->m_enclosingWidget->deform(maximum, node);
    }
    plasma::Vector<2, float> first = node->m_matrix.transform(minimum);
    plasma::Vector<2, float> second = node->m_matrix.transform(maximum);
    minimum.m_elements[0] = second.m_elements[0] <= first.m_elements[0] ? second.m_elements[0] : first.m_elements[0];
    minimum.m_elements[1] = second.m_elements[1] <= first.m_elements[1] ? second.m_elements[1] : first.m_elements[1];
    maximum.m_elements[0] = first.m_elements[0] > second.m_elements[0] ? first.m_elements[0] : second.m_elements[0];
    maximum.m_elements[1] = first.m_elements[1] > second.m_elements[1] ? first.m_elements[1] : second.m_elements[1];
    try {
        Concurrency::parallel_for_each(drawing->m_pathVertices.begin(), drawing->m_pathVertices.end(),
                                       [&node, tool](plasma::Vector<2, float> point) {
                                           if (node->m_enclosingWidget)
                                               point = node->m_enclosingWidget->deform(point, node);
                                           point.transform(node->m_matrix);
                                           if (!tool->contains(point))
                                               throw false;
                                       });
    } catch (...) {
        return false;
    }
    return true;
}

// True when one of the shape's drawings lies entirely in the tool's box.
bool MeshInterface::isInsideBox(BoundingBoxTool *tool)
{
    if (m_mesh.m_shape->isFilled() && m_mesh.m_shape->m_fillDrawing &&
        isDrawingInBox(m_mesh.m_shape->m_fillDrawing, tool, m_node->getNode()))
        return true;
    if (m_mesh.m_shape->isStroked() && m_mesh.m_shape->m_strokeDrawing &&
        isDrawingInBox(m_mesh.m_shape->m_strokeDrawing, tool, m_node->getNode()))
        return true;
    if (m_mesh.m_shape->drawsExtrusion() && m_mesh.m_shape->m_extrusionDrawing &&
        isDrawingInBox(m_mesh.m_shape->m_extrusionDrawing, tool, m_node->getNode()))
        return true;
    return false;
}

void MeshInterface::selectVerticesInBox(BoundingBoxTool *tool, bool select)
{
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        plasma::Vector<2, float> position = getVertexPosition(*it);
        if (tool->contains(matrix.transformPoint(position)) && isVertexSelected(*it) != select) {
            setVertexSelected(*it, select);
            m_hasChanges = true;
        }
    }
}

// Selects the vertices of the selected edges instead.
void MeshInterface::applyEdgeSelection()
{
    selectAllVertices(false);
    for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it) {
        MeshHalfEdge *edge = *it;
        if (isEdgeSelected(edge)) {
            setVertexSelected(edge->m_vertex, true);
            setVertexSelected((*it)->m_next->m_vertex, true);
        }
    }
}

// Selects the vertices of the selected faces instead.
void MeshInterface::applyFaceSelection()
{
    selectAllVertices(false);
    for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it) {
        if (isFaceSelected(*it)) {
            for (MeshFaceIterator corner(*it); corner.isValid(); ++corner)
                setVertexSelected(corner.getVertex(), true);
        }
    }
}

// An edge is in the box when both its vertices are. A contour mesh skips the closing edge of its first face.
void MeshInterface::selectEdgesInBox(BoundingBoxTool *tool, bool select)
{
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    MeshHalfEdge *skipped = 0;
    if (!isClosed(m_mesh.m_shape->m_flags) && (int)m_mesh.getFaceCount() > 0)
        skipped = (*m_mesh.beginFaces())->m_edge->m_previous;
    for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it) {
        if (*it != skipped) {
            plasma::Vector<2, float> first = getVertexPosition((*it)->m_vertex);
            plasma::Vector<2, float> second = getVertexPosition((*it)->m_next->m_vertex);
            if (tool->contains(matrix.transformPoint(first)) && tool->contains(matrix.transformPoint(second)) &&
                isEdgeSelected(*it) != select) {
                setEdgeSelected(*it, select);
                m_hasChanges = true;
            }
        }
    }
    applyEdgeSelection();
    refresh();
}

// A face is in the box when all its corners are.
void MeshInterface::selectFacesInBox(BoundingBoxTool *tool, bool select)
{
    plasma::Matrix<float> matrix;
    getMatrix(matrix);
    for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it) {
        bool inside = true;
        for (MeshFaceIterator corner(*it); corner.isValid(); ++corner) {
            plasma::Vector<2, float> position = getVertexPosition(corner.getVertex());
            if (!tool->contains(matrix.transformPoint(position))) {
                inside = false;
                break;
            }
        }
        if (inside && isFaceSelected(*it) != select) {
            setFaceSelected(*it, select);
            m_hasChanges = true;
        }
    }
    applyFaceSelection();
}

// Binds each vertex (its position mapped by the matrix into the source's coordinates) to the first face of the
// source's finest subdivision level that contains it: on an edge (within 1e-7 squared) the weights interpolate the
// edge's two corners, otherwise each corner gets the ratio of the signed areas around it, normalized to sum 1.
// Vertices outside every face stay unbound.
void MeshInterface::bindToShape(plasma::SmoothMeshShape *source, const plasma::Matrix<float> &matrix)
{
    m_bindings.clear();
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        plasma::Vector<2, float> point =
            matrix.transform(mesh.m_shape->m_vertexPositions.value()[mesh.vertexIndex(*it)]);
        for (int face = 0; face < (source->m_subdivisions > 0 && !(source->m_flags & (1 << plasma::MESH_CONTOUR))
                                       ? (int)source->m_levels[source->m_subdivisions - 1].m_faces.size()
                                       : 0);
             face++) {
            plasma::Vector<2, float> corners[MeshBinding::CORNER_COUNT];
            corners[0] = *source->m_levels[source->m_subdivisions - 1].m_faces[face].m_vertices[0]->m_position;
            corners[1] = *source->m_levels[source->m_subdivisions - 1].m_faces[face].m_vertices[1]->m_position;
            corners[2] = *source->m_levels[source->m_subdivisions - 1].m_faces[face].m_vertices[2]->m_position;
            corners[3] = *source->m_levels[source->m_subdivisions - 1].m_faces[face].m_vertices[3]->m_position;
            MeshBinding binding;
            binding.m_sourceFace = face;
            binding.m_targetVertex = vertexIndex(*it);
            int edge;
            for (edge = 0; edge < MeshBinding::CORNER_COUNT; edge++) {
                if (squaredDistanceToSegment(point, corners[edge], corners[(edge + 1) % MeshBinding::CORNER_COUNT]) <
                    1e-7f)
                    break;
            }
            if (edge < MeshBinding::CORNER_COUNT) {
                binding.m_cornerWeights[0] = 0.0f;
                binding.m_cornerWeights[1] = 0.0f;
                binding.m_cornerWeights[2] = 0.0f;
                binding.m_cornerWeights[3] = 0.0f;
                plasma::Vector<2, float> offset = point - corners[edge];
                int next = (edge + 1) % MeshBinding::CORNER_COUNT;
                plasma::Vector<2, float> direction = corners[next] - corners[edge];
                float along =
                    (offset.m_elements[0] * direction.m_elements[0] + offset.m_elements[1] * direction.m_elements[1]) /
                    (direction.m_elements[0] * direction.m_elements[0] +
                     direction.m_elements[1] * direction.m_elements[1]);
                binding.m_cornerWeights[edge] = 1.0f - along;
                binding.m_cornerWeights[next] = along;
                m_bindings.push_back(binding);
                break;
            }
            float sum = 0.0f;
            int corner = 0;
            bool inside = true;
            for (int following = 1; following < MeshBinding::CORNER_COUNT + 1; following++) {
                const plasma::Vector<2, float> &opposite = corners[(following + 2) % MeshBinding::CORNER_COUNT];
                plasma::Vector<2, float> side = corners[corner] - opposite;
                plasma::Vector<2, float> toPoint = point - opposite;
                plasma::Vector<2, float> nextSide = corners[following % MeshBinding::CORNER_COUNT] - corners[corner];
                plasma::Vector<2, float> fromCorner = point - corners[corner];
                float area = fromCorner.m_elements[1] * nextSide.m_elements[0] -
                             fromCorner.m_elements[0] * nextSide.m_elements[1];
                area =
                    (float)(toPoint.m_elements[1] * side.m_elements[0] - toPoint.m_elements[0] * side.m_elements[1]) *
                    area;
                if (!(std::fabs(area) > 0.0f)) {
                    if (area < 0.0f) {
                        inside = false;
                        break;
                    }
                    binding.m_cornerWeights[0] = 0.0f;
                    binding.m_cornerWeights[1] = 0.0f;
                    binding.m_cornerWeights[2] = 0.0f;
                    binding.m_cornerWeights[3] = 0.0f;
                    binding.m_cornerWeights[corner] = 1.0f;
                    sum = 1.0f;
                    break;
                }
                plasma::Vector<2, float> first = corners[corner] - opposite;
                plasma::Vector<2, float> second = corners[following % MeshBinding::CORNER_COUNT] - opposite;
                float weight =
                    (float)(second.m_elements[1] * first.m_elements[0] - second.m_elements[0] * first.m_elements[1]) /
                    area;
                binding.m_cornerWeights[corner] = weight;
                if (weight < 0.0f) {
                    inside = false;
                    break;
                }
                corner++;
                sum = weight + sum;
            }
            if (!inside || sum < 1e-15f)
                continue;
            binding.m_cornerWeights[0] = binding.m_cornerWeights[0] / sum;
            binding.m_cornerWeights[1] = binding.m_cornerWeights[1] / sum;
            binding.m_cornerWeights[2] = binding.m_cornerWeights[2] / sum;
            binding.m_cornerWeights[3] = binding.m_cornerWeights[3] / sum;
            m_bindings.push_back(binding);
            break;
        }
    }
}

// Moves the bound vertices onto the source mesh's subdivided faces: each binding weights the four corners of one
// face of the source's finest subdivision level; the matrix maps the source's coordinates into this mesh's.
void MeshInterface::followShape(plasma::SmoothMeshShape *source, const plasma::Matrix<float> &matrix)
{
    for (int i = 0; i < (int)m_bindings.size(); i++) {
        // BindToShape only creates bindings for an existing subdivision level.
        MeshBinding &binding = m_bindings[i];
        plasma::Vector<2, float> corners[MeshBinding::CORNER_COUNT];
        corners[0] =
            *source->m_levels[source->m_subdivisions - 1].m_faces[binding.m_sourceFace].m_vertices[0]->m_position;
        corners[1] =
            *source->m_levels[source->m_subdivisions - 1].m_faces[binding.m_sourceFace].m_vertices[1]->m_position;
        corners[2] =
            *source->m_levels[source->m_subdivisions - 1].m_faces[binding.m_sourceFace].m_vertices[2]->m_position;
        corners[3] =
            *source->m_levels[source->m_subdivisions - 1].m_faces[binding.m_sourceFace].m_vertices[3]->m_position;
        plasma::Vector<2, float> position(0.0f, 0.0f);
        for (int corner = 0; corner < MeshBinding::CORNER_COUNT; corner++)
            position += corners[corner] * binding.m_cornerWeights[corner];
        m_mesh.m_shape->m_vertexPositions.value()[binding.m_targetVertex] = matrix * position;
    }
    m_hasChanges = true;
    m_mesh.m_shape->m_vertexPositions.m_changed = true;
    if (m_mesh.m_shape->m_flags & (1 << plasma::MESH_CONTOUR))
        m_mesh.m_shape->m_vertexColors.m_changed = true;
    m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_VALUES);
    if (m_node->m_selected)
        this->refresh();
}

// "node:align:outline": spreads the node's shown children evenly along the shape's contours, each contour taking a
// share of them proportional to its length; with "node:align:outline:rotation" (node flag bit 9) each child is
// also turned to the outline's direction. The children's sheet nodes are marked (m_modificationPending) for their
// update.
void MeshInterface::alignChildrenToOutline()
{
    int count = 0;
    for (std::list<plasma::Node *>::iterator it = m_node->m_node->m_children.begin();
         it != m_node->m_node->m_children.end(); ++it) {
        if ((*it)->isShown()) {
            ++count;
            m_sheet->findSheetNode(*it)->m_modificationPending = true;
        }
    }
    float total = 0.0f;
    std::vector<float> lengths(m_mesh.m_shape->m_contourCenters.size());
    for (int i = 0; i < (int)m_mesh.m_shape->m_contourCenters.size(); i++) {
        std::vector<plasma::Vector<2, float>> &contour = m_mesh.m_shape->m_contourCenters[i];
        int segments = contour.size();
        if (!isClosed(m_mesh.m_shape->m_flags))
            --segments;
        lengths[i] = 0.0f;
        for (int j = 0; j < segments; j++) {
            plasma::Vector<2, float> edge = contour[j] - contour[(j + 1) % contour.size()];
            lengths[i] += std::sqrt(edge.m_elements[1] * edge.m_elements[1] + edge.m_elements[0] * edge.m_elements[0]);
        }
        total += lengths[i];
    }
    std::list<plasma::Node *>::iterator child = m_node->m_node->m_children.begin();
    int placed = 0;
    for (int i = 0; i < (int)m_mesh.m_shape->m_contourCenters.size(); i++) {
        std::vector<plasma::Vector<2, float>> &contour = m_mesh.m_shape->m_contourCenters[i];
        int segments = contour.size();
        if (!isClosed(m_mesh.m_shape->m_flags))
            segments = contour.size() - 1;
        int share = (int)(lengths[i] / total * (float)count + 0.5);
        if (count > placed && i == (int)m_mesh.m_shape->m_contourCenters.size() - 1)
            share = count - placed;
        placed += share;
        int segment = 0;
        if (!share)
            continue;
        float start = 0.0f; // The distance along the contour where the segment starts.
        float spacing = lengths[i] / (double)share;
        float distance = 0.0f; // The next child's distance along the contour.
        if (!isClosed(m_mesh.m_shape->m_flags))
            spacing = lengths[i] / (double)(share - 1);
        plasma::Vector<2, float> direction(0.0f, 1.0f);
        for (int k = 0; k < share; ++k, ++child) {
            plasma::Node *node = m_node->m_node;
            if (child == node->m_children.end())
                break;
            while (!(*child)->isShown()) {
                if (++child == node->m_children.end())
                    break;
            }
            if (child == node->m_children.end())
                break;
            if (segment >= segments)
                continue;
            float length;
            for (;;) {
                plasma::Vector<2, float> edge = contour[segment] - contour[(segment + 1) % contour.size()];
                length = std::sqrt(edge.m_elements[1] * edge.m_elements[1] + edge.m_elements[0] * edge.m_elements[0]);
                if (segment == segments - 1 || distance <= length + start)
                    break;
                ++segment;
                start += length;
                if (segment >= segments)
                    break;
            }
            if (segment >= segments)
                continue;
            float fraction = length > 0.0f ? (distance - start) / length : 0.0f;
            float t = fraction > 1.0f ? 1.0f : fraction;
            plasma::Vector<2, float> point =
                contour[segment] * (1.0f - t) + contour[(segment + 1) % contour.size()] * t;
            plasma::Transformation *transformation = (*child)->m_transformation;
            plasma::Vector<2, float> offset =
                point - transformation->m_matrix.transformPoint(transformation->m_pivot.value());
            plasma::Vector<2, float> &translation = transformation->m_translation.value();
            translation.m_elements[0] = offset.m_elements[0] + translation.m_elements[0];
            translation.m_elements[1] = offset.m_elements[1] + translation.m_elements[1];
            if (node->alignsRotationToOutline()) {
                plasma::Vector<2, float> edge = contour[segment] - contour[(segment + 1) % contour.size()];
                if (length > 0.0f) {
                    float scale = 1.0f / length;
                    direction = plasma::Vector<2, float>(edge.m_elements[1] * scale, scale * -edge.m_elements[0]);
                }
                float angle = acos(direction.m_elements[1]) * 57.29577951308232;
                if (direction.m_elements[0] < 0.0f)
                    angle = -angle;
                transformation->m_rotation.value() = plasma::Vector<3, float>(0.0f, 0.0f, -angle);
            }
            transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            distance = spacing - start + distance;
            start = 0.0f;
        }
    }
}

// The colour as hue (degrees), saturation and value (percent) with its alpha, as the colour picker shows it
// (ColorModule::SetColor).
EditorColor rgbToHsv(const EditorColor &color)
{
    float maximum = color.m_elements[0];
    float minimum = maximum;
    if (color.m_elements[1] > maximum)
        maximum = color.m_elements[1];
    if (color.m_elements[2] > maximum)
        maximum = color.m_elements[2];
    if (color.m_elements[1] < minimum)
        minimum = color.m_elements[1];
    if (color.m_elements[2] < minimum)
        minimum = color.m_elements[2];
    EditorColor hsv;
    hsv.m_elements[2] = maximum * 100.0;
    hsv.m_elements[RGBA_ALPHA] = color.m_elements[RGBA_ALPHA];
    if (maximum == minimum) {
        hsv.m_elements[0] = 0.0f;
        hsv.m_elements[1] = 0.0f;
        return hsv;
    }
    hsv.m_elements[1] = 0.0 == maximum ? 0.0 : 100.0 * ((maximum - minimum) / maximum);
    if (maximum == color[0]) {
        hsv[0] = (color[1] - color[2]) * 60.0 / (maximum - minimum) + 360.0;
        hsv[0] -= 360 * (int)(hsv[0] / 360.0);
    } else if (maximum == color[1]) {
        hsv.m_elements[0] = (color.m_elements[2] - color.m_elements[0]) * 60.0 / (maximum - minimum) + 120.0;
    } else {
        hsv.m_elements[0] = (color.m_elements[0] - color.m_elements[1]) * 60.0 / (maximum - minimum) + 240.0;
    }
    return hsv;
}

// Shifts the vertex colours ('colorTarget': 0 fill, 1 stroke, 2 front and back) that match one of the source
// colours towards the target. As the colour tool's painting: Shift keeps the alpha, Ctrl changes only the alpha
// when the tool edits both.
void MeshInterface::shiftColors(const std::list<plasma::Vector<4, float>> &source,
                                const plasma::Vector<4, float> &target, float tolerance, int colorTarget, bool update)
{
    m_mesh.m_shape->lock();
    EditableMesh &mesh = m_mesh;
    plasma::Vector<4, float> *color;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        int count = 1;
        if (colorTarget == COLOR_TARGET_EXTRUSION)
            count = 2;
        for (int i = 0; i < count; i++) {
            // 'color' keeps its previous value for 'colorTarget' outside 0..2.
            switch (colorTarget) {
            case COLOR_TARGET_FILL:
                color = &mesh.m_shape->m_vertexColors.value()[mesh.vertexIndex(*it)];
                break;
            case COLOR_TARGET_STROKE:
                color = &mesh.m_shape->m_strokeColors.value()[mesh.vertexIndex(*it)];
                break;
            case COLOR_TARGET_EXTRUSION:
                if (i)
                    color = &mesh.m_shape->m_extrusionBackColors.value()[mesh.vertexIndex(*it)];
                else
                    color = &mesh.m_shape->m_extrusionFrontColors.value()[mesh.vertexIndex(*it)];
                break;
            }
            plasma::Vector<4, float> shifted = shiftColor(source, target, tolerance, *color);
            Workspace *workspace = m_sheet->m_workspace;
            if (!workspace->m_engine.isKeyDown(VK_SHIFT) && workspace->m_colorToolModule->m_alpha) {
                if (!workspace->m_engine.isKeyDown(VK_CONTROL) && workspace->m_colorToolModule->m_color)
                    *color = shifted;
                else
                    color->m_elements[RGBA_ALPHA] = shifted.m_elements[RGBA_ALPHA];
            } else {
                color->m_elements[0] = shifted.m_elements[0];
                color->m_elements[1] = shifted.m_elements[1];
                color->m_elements[2] = shifted.m_elements[2];
            }
        }
    }
    if (colorTarget == COLOR_TARGET_FILL) {
        m_mesh.m_shape->m_vertexColors.m_changed = true;
    } else if (colorTarget == COLOR_TARGET_STROKE) {
        m_mesh.m_shape->m_strokeColors.m_changed = true;
    } else if (colorTarget == COLOR_TARGET_EXTRUSION) {
        m_mesh.m_shape->m_extrusionFrontColors.m_changed = true;
        m_mesh.m_shape->m_extrusionBackColors.m_changed = true;
    }
    if (update)
        m_hasChanges = true;
    m_mesh.m_shape->unlock();
}

// Scales the colours' brightness: 0 the fill colours, 1 the stroke colours, 2 the front and back colours.
void MeshInterface::adjustBrightness(float factor, int colorTarget, bool update)
{
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        int count = 1;
        if (colorTarget == COLOR_TARGET_EXTRUSION)
            count = 2;

        // The code leaves color unset for targets outside COLOR_TARGET_FILL..COLOR_TARGET_EXTRUSION.
        plasma::Vector<4, float> *color;
        for (int i = 0; i < count; i++) {
            switch (colorTarget) {
            case COLOR_TARGET_FILL:
                color = &mesh.m_shape->m_vertexColors.value()[vertexIndex(*it)];
                break;
            case COLOR_TARGET_STROKE:
                color = &mesh.m_shape->m_strokeColors.value()[vertexIndex(*it)];
                break;
            case COLOR_TARGET_EXTRUSION:
                if (i == 0)
                    color = &mesh.m_shape->m_extrusionFrontColors.value()[vertexIndex(*it)];
                else
                    color = &mesh.m_shape->m_extrusionBackColors.value()[vertexIndex(*it)];
                break;
            }
            plasma::Vector<4, float> scale(factor, factor, factor, 1.0f); // The alpha is kept.
            for (int j = 0; j < RGBA_COMPONENT_COUNT; j++)
                color->m_elements[j] *= scale.m_elements[j];
            for (int j = 0; j < RGB_COMPONENT_COUNT; j++) {
                if (color->m_elements[j] < 0.0f)
                    color->m_elements[j] = 0.0f;
                if (color->m_elements[j] > 1.0f)
                    color->m_elements[j] = 1.0f;
            }
        }
    }
    switch (colorTarget) {
    case COLOR_TARGET_FILL:
        m_mesh.m_shape->m_vertexColors.m_changed = true;
        break;
    case COLOR_TARGET_STROKE:
        m_mesh.m_shape->m_strokeColors.m_changed = true;
        break;
    case COLOR_TARGET_EXTRUSION:
        m_mesh.m_shape->m_extrusionFrontColors.m_changed = true;
        m_mesh.m_shape->m_extrusionBackColors.m_changed = true;
        break;
    }
    if (update)
        m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_VALUES);
}

// Blends the colours with grey (contrast): 0 the fill colours, 1 the stroke colours, 2 the front and back colours.
// The alpha is kept.
void MeshInterface::adjustContrast(float factor, int colorTarget, bool update)
{
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        int count = 1;
        if (colorTarget == COLOR_TARGET_EXTRUSION)
            count = 2;

        // The code leaves color unset for targets outside COLOR_TARGET_FILL..COLOR_TARGET_EXTRUSION.
        plasma::Vector<4, float> *color;
        for (int i = 0; i < count; i++) {
            switch (colorTarget) {
            case COLOR_TARGET_FILL:
                color = &mesh.m_shape->m_vertexColors.value()[vertexIndex(*it)];
                break;
            case COLOR_TARGET_STROKE:
                color = &mesh.m_shape->m_strokeColors.value()[vertexIndex(*it)];
                break;
            case COLOR_TARGET_EXTRUSION:
                if (i == 0)
                    color = &mesh.m_shape->m_extrusionFrontColors.value()[vertexIndex(*it)];
                else
                    color = &mesh.m_shape->m_extrusionBackColors.value()[vertexIndex(*it)];
                break;
            }
            float alpha = color->m_elements[RGBA_ALPHA];
            plasma::Vector<4, float> grey(0.5f, 0.5f, 0.5f, 0.0f);
            *color = *color * factor + grey * (1.0f - factor);
            color->m_elements[RGBA_ALPHA] = alpha;
            for (int j = 0; j < RGB_COMPONENT_COUNT; j++) {
                if (color->m_elements[j] < 0.0f)
                    color->m_elements[j] = 0.0f;
                if (color->m_elements[j] > 1.0f)
                    color->m_elements[j] = 1.0f;
            }
        }
    }
    switch (colorTarget) {
    case COLOR_TARGET_FILL:
        m_mesh.m_shape->m_vertexColors.m_changed = true;
        break;
    case COLOR_TARGET_STROKE:
        m_mesh.m_shape->m_strokeColors.m_changed = true;
        break;
    case COLOR_TARGET_EXTRUSION:
        m_mesh.m_shape->m_extrusionFrontColors.m_changed = true;
        m_mesh.m_shape->m_extrusionBackColors.m_changed = true;
        break;
    }
    if (update)
        m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_VALUES);
}

// Blends RGB with luminance to adjust saturation; targets are the same as AdjustContrast. Alpha is kept. Luminance
// accumulates green * 0.59, red * 0.3 and blue * 0.11.
void MeshInterface::adjustSaturation(float factor, int colorTarget, bool update)
{
    EditableMesh &mesh = m_mesh;
    for (HalfEdgeMesh::VertexIterator it = mesh.beginVertices(); it != mesh.endVertices(); ++it) {
        int count = 1;
        if (colorTarget == COLOR_TARGET_EXTRUSION)
            count = 2;

        // The code leaves color unset for targets outside COLOR_TARGET_FILL..COLOR_TARGET_EXTRUSION.
        plasma::Vector<4, float> *color;
        for (int i = 0; i < count; i++) {
            switch (colorTarget) {
            case COLOR_TARGET_FILL:
                color = &mesh.m_shape->m_vertexColors.value()[mesh.vertexIndex(*it)];
                break;
            case COLOR_TARGET_STROKE:
                color = &mesh.m_shape->m_strokeColors.value()[mesh.vertexIndex(*it)];
                break;
            case COLOR_TARGET_EXTRUSION:
                if (i == 0)
                    color = &mesh.m_shape->m_extrusionFrontColors.value()[mesh.vertexIndex(*it)];
                else
                    color = &mesh.m_shape->m_extrusionBackColors.value()[mesh.vertexIndex(*it)];
                break;
            }
            plasma::Vector<3, float> rgb(color->m_elements[0], color->m_elements[1], color->m_elements[2]);
            float luminance = rgb.m_elements[1] * 0.59f + rgb.m_elements[0] * 0.3f + rgb.m_elements[2] * 0.11f;
            float alpha = color->m_elements[RGBA_ALPHA];
            *color =
                *color * factor + plasma::Vector<4, float>(luminance, luminance, luminance, 0.0f) * (1.0f - factor);
            color->m_elements[RGBA_ALPHA] = alpha;
            for (int j = 0; j < RGB_COMPONENT_COUNT; j++) {
                if (color->m_elements[j] < 0.0f)
                    color->m_elements[j] = 0.0f;
                if (color->m_elements[j] > 1.0f)
                    color->m_elements[j] = 1.0f;
            }
        }
    }
    switch (colorTarget) {
    case COLOR_TARGET_FILL:
        m_mesh.m_shape->m_vertexColors.m_changed = true;
        break;
    case COLOR_TARGET_STROKE:
        m_mesh.m_shape->m_strokeColors.m_changed = true;
        break;
    case COLOR_TARGET_EXTRUSION:
        m_mesh.m_shape->m_extrusionFrontColors.m_changed = true;
        m_mesh.m_shape->m_extrusionBackColors.m_changed = true;
        break;
    }
    if (update)
        m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_VALUES);
}

// The point's barycentric coordinates in the triangle (a, b, c): the weights of c and b; (-1, -1) for a degenerate
// triangle.
static plasma::Vector<2, float> getBarycentric(const plasma::Vector<2, float> &a, const plasma::Vector<2, float> &b,
                                               const plasma::Vector<2, float> &c, const plasma::Vector<2, float> &point)
{
    plasma::Vector<2, float> toC = c - a;
    plasma::Vector<2, float> toB = b - a;
    plasma::Vector<2, float> toPoint = point - a;
    float cc = dot(toC, toC);
    float bc = dot(toB, toC);
    float pc = dot(toC, toPoint);
    float bb = dot(toB, toB);
    float pb = dot(toB, toPoint);
    float denominator = bb * cc - bc * bc;
    plasma::Vector<2, float> result;
    if (std::fabs(denominator) < 1e-7f) {
        result.m_elements[0] = -1.0f;
        result.m_elements[1] = -1.0f;
    } else {
        float scale = 1.0f / denominator;
        result.m_elements[0] = (bb * pc - pb * bc) * scale;
        result.m_elements[1] = (pb * cc - bc * pc) * scale;
    }
    return result;
}

plasma::Node *MeshInterface::findNodeUnder(const plasma::Vector<2, float> &point, plasma::Node *node,
                                           plasma::Node *self, bool &reachedSelf)
{
    if (reachedSelf)
        return 0;
    if (node == self) {
        reachedSelf = true;
        return 0;
    }
    plasma::Vector<2, float> local;
    local = node->m_inverseMatrix.transform(point);
    if (node->m_enclosingWidget && node->m_shape && node->m_shape->isDeformable())
        local = node->m_enclosingWidget->undeform(local, node);

    // Under a smooth mesh shape (type 2)?
    bool under =
        node->m_shape && node->m_shape->getType() == plasma::SHAPE_SMOOTH_MESH && node->m_shape->contains(local);

    // A clipping node's children are only seen through its shape.
    if (node->m_display->m_clipping.value() && !under)
        return 0;
    plasma::Node *result = 0;
    std::for_each(node->m_children.begin(), node->m_children.end(), [&](plasma::Node *child) {
        plasma::Node *match = findNodeUnder(point, child, self, reachedSelf);
        if (match)
            result = match;
    });
    if (!result)
        return under ? node : 0;
    return result;
}

bool MeshInterface::getDrawingColor(plasma::Drawing *drawing, const plasma::Vector<2, float> &point,
                                    plasma::Vector<4, float> &color)
{
    // Tessellation produces complete triples of valid path-vertex indices.
    for (int i = 0; i < (int)drawing->m_pathIndices.size(); i += 3) {
        plasma::Vector<2, float> weights = getBarycentric(
            drawing->m_pathVertices[drawing->m_pathIndices[i]], drawing->m_pathVertices[drawing->m_pathIndices[i + 1]],
            drawing->m_pathVertices[drawing->m_pathIndices[i + 2]], point);
        if (weights.m_elements[0] >= 0.0f && weights.m_elements[1] >= 0.0f &&
            weights.m_elements[1] + weights.m_elements[0] <= 1.0f) {
            const plasma::Vector<4, float> &a = drawing->m_pathColors[drawing->m_pathIndices[i]];
            const plasma::Vector<4, float> &b = drawing->m_pathColors[drawing->m_pathIndices[i + 1]];
            const plasma::Vector<4, float> &c = drawing->m_pathColors[drawing->m_pathIndices[i + 2]];
            color = a + (c - a) * weights.m_elements[0] + (b - a) * weights.m_elements[1];
            return true;
        }
    }
    return false;
}

// Blends each translucent vertex's colours with the colour of the mesh drawn under it (fill, stroke and
// extrusion), making them opaque.
void MeshInterface::flattenTransparency()
{
    bool changed = false;
    beginModify();
    std::for_each(m_mesh.beginVertices(), m_mesh.endVertices(), [this, &changed](MeshVertex *vertex) {
        // Layout positions, extents and spacing use display units; colour tuples style the overlays. Wireframe palette:
        // cyan hover, purple selected hover/current, green selection, white smooth/root, dark sharp, blue Bezier and red
        // round; yellow/magenta constraints.
        float fillAlpha = m_mesh.m_shape->m_vertexColors.value()[vertexIndex(vertex)].m_elements[RGBA_ALPHA];
        float strokeAlpha = m_mesh.m_shape->m_strokeColors.value()[vertexIndex(vertex)].m_elements[RGBA_ALPHA];
        float frontAlpha = m_mesh.m_shape->m_extrusionFrontColors.value()[vertexIndex(vertex)].m_elements[RGBA_ALPHA];
        float backAlpha = m_mesh.m_shape->m_extrusionBackColors.value()[vertexIndex(vertex)].m_elements[RGBA_ALPHA];
        if (fillAlpha < 1.0f || strokeAlpha < 1.0f || frontAlpha < 1.0f || backAlpha < 1.0f) {
            plasma::Vector<2, float> position = m_node->m_node->m_matrix.transform(getVertexPosition(vertex));
            bool reachedSelf = false;
            plasma::Node *under = findNodeUnder(position, m_sheet->m_root, m_node->m_node, reachedSelf);
            plasma::Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
            if (under) {
                plasma::Shape *shape = under->m_shape;
                if (shape && shape->getType() == plasma::SHAPE_SMOOTH_MESH) {
                    plasma::SmoothMeshShape *mesh = static_cast<plasma::SmoothMeshShape *>(shape);
                    plasma::Vector<2, float> local = under->m_inverseMatrix.transform(position);
                    if (under->m_enclosingWidget && shape->isDeformable())
                        local = under->m_enclosingWidget->undeform(local, under);
                    if (mesh->drawsExtrusion() && mesh->m_extrusionDrawing)
                        getDrawingColor(mesh->m_extrusionDrawing, local, color);
                    if (mesh->isFilled() && mesh->m_fillDrawing)
                        getDrawingColor(mesh->m_fillDrawing, local, color);
                    if (mesh->isStroked() && mesh->m_strokeDrawing)
                        getDrawingColor(mesh->m_strokeDrawing, local, color);
                }
            }
            m_mesh.m_shape->m_vertexColors.value()[vertexIndex(vertex)] =
                color * (1.0f - fillAlpha) + m_mesh.m_shape->m_vertexColors.value()[vertexIndex(vertex)] * fillAlpha;
            m_mesh.m_shape->m_vertexColors.value()[vertexIndex(vertex)].m_elements[RGBA_ALPHA] = 1.0f;
            m_mesh.m_shape->m_strokeColors.value()[vertexIndex(vertex)] =
                color * (1.0f - strokeAlpha) +
                m_mesh.m_shape->m_strokeColors.value()[vertexIndex(vertex)] * strokeAlpha;
            m_mesh.m_shape->m_strokeColors.value()[vertexIndex(vertex)].m_elements[RGBA_ALPHA] = 1.0f;
            m_mesh.m_shape->m_extrusionFrontColors.value()[vertexIndex(vertex)] =
                color * (1.0f - frontAlpha) +
                m_mesh.m_shape->m_extrusionFrontColors.value()[vertexIndex(vertex)] * frontAlpha;
            m_mesh.m_shape->m_extrusionFrontColors.value()[vertexIndex(vertex)].m_elements[RGBA_ALPHA] = 1.0f;
            m_mesh.m_shape->m_extrusionBackColors.value()[vertexIndex(vertex)] =
                color * (1.0f - backAlpha) +
                m_mesh.m_shape->m_extrusionBackColors.value()[vertexIndex(vertex)] * backAlpha;
            m_mesh.m_shape->m_extrusionBackColors.value()[vertexIndex(vertex)].m_elements[RGBA_ALPHA] = 1.0f;
            changed = true;
        }
    });
    if (changed) {
        m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        endModify();
    }
}

// Adds the average colour of each drawn part: fill, stroke, extrusion.
void MeshInterface::getColors(std::list<plasma::Vector<4, float>> &colors)
{
    if (m_mesh.m_shape->drawsFill()) {
        plasma::Vector<4, float> sum(0.0f, 0.0f, 0.0f, 0.0f);
        int count = 0;
        for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
            sum += m_mesh.m_shape->m_vertexColors.value()[m_mesh.vertexIndex(*it)];
            count++;
        }
        if (count > 0)
            colors.push_back(sum * (1.0f / count));
    }
    if (m_mesh.m_shape->drawsStroke()) {
        plasma::Vector<4, float> sum(0.0f, 0.0f, 0.0f, 0.0f);
        int count = 0;
        for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
            sum += m_mesh.m_shape->m_strokeColors.value()[m_mesh.vertexIndex(*it)];
            count++;
        }
        if (count > 0)
            colors.push_back(sum * (1.0f / count));
    }
    if (m_mesh.m_shape->drawsExtrusion()) {
        plasma::Vector<4, float> sum(0.0f, 0.0f, 0.0f, 0.0f);
        int count = 0;
        for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
            sum += m_mesh.m_shape->m_extrusionFrontColors.value()[m_mesh.vertexIndex(*it)];
            sum += m_mesh.m_shape->m_extrusionBackColors.value()[m_mesh.vertexIndex(*it)];
            count += 2;
        }
        if (count > 0)
            colors.push_back(sum * (1.0f / count));
    }
}

// Starts editing the colours: element 0 keeps them, element 1 (when 'preview') receives a copy that becomes
// current.
void MeshInterface::setColorPreview(bool preview)
{
    m_mesh.m_shape->m_vertexColors.m_current = plasma::Attribute::BASE_ELEMENT;
    m_mesh.m_shape->m_strokeColors.m_current = plasma::Attribute::BASE_ELEMENT;
    m_mesh.m_shape->m_extrusionFrontColors.m_current = plasma::Attribute::BASE_ELEMENT;
    m_mesh.m_shape->m_extrusionBackColors.m_current = plasma::Attribute::BASE_ELEMENT;
    if (preview) {
        m_mesh.m_shape->m_vertexColors.copyFromCurrent(plasma::Attribute::STORED_ELEMENT);
        m_mesh.m_shape->m_strokeColors.copyFromCurrent(plasma::Attribute::STORED_ELEMENT);
        m_mesh.m_shape->m_extrusionFrontColors.copyFromCurrent(plasma::Attribute::STORED_ELEMENT);
        m_mesh.m_shape->m_extrusionBackColors.copyFromCurrent(plasma::Attribute::STORED_ELEMENT);
        m_mesh.m_shape->m_vertexColors.m_current = plasma::Attribute::STORED_ELEMENT;
        m_mesh.m_shape->m_strokeColors.m_current = plasma::Attribute::STORED_ELEMENT;
        m_mesh.m_shape->m_extrusionFrontColors.m_current = plasma::Attribute::STORED_ELEMENT;
        m_mesh.m_shape->m_extrusionBackColors.m_current = plasma::Attribute::STORED_ELEMENT;
    }
    m_colorPreview = preview;
}

// Keeps the edited colours: element 1 is copied back to element 0.
void MeshInterface::applyColorPreview()
{
    if (m_colorPreview) {
        m_mesh.m_shape->m_vertexColors.m_current = plasma::Attribute::STORED_ELEMENT;
        m_mesh.m_shape->m_strokeColors.m_current = plasma::Attribute::STORED_ELEMENT;
        m_mesh.m_shape->m_extrusionFrontColors.m_current = plasma::Attribute::STORED_ELEMENT;
        m_mesh.m_shape->m_extrusionBackColors.m_current = plasma::Attribute::STORED_ELEMENT;
        m_mesh.m_shape->m_vertexColors.copyFromCurrent(plasma::Attribute::BASE_ELEMENT);
        m_mesh.m_shape->m_strokeColors.copyFromCurrent(plasma::Attribute::BASE_ELEMENT);
        m_mesh.m_shape->m_extrusionFrontColors.copyFromCurrent(plasma::Attribute::BASE_ELEMENT);
        m_mesh.m_shape->m_extrusionBackColors.copyFromCurrent(plasma::Attribute::BASE_ELEMENT);
        m_mesh.m_shape->m_vertexColors.m_current = plasma::Attribute::BASE_ELEMENT;
        m_mesh.m_shape->m_strokeColors.m_current = plasma::Attribute::BASE_ELEMENT;
        m_mesh.m_shape->m_extrusionFrontColors.m_current = plasma::Attribute::BASE_ELEMENT;
        m_mesh.m_shape->m_extrusionBackColors.m_current = plasma::Attribute::BASE_ELEMENT;
        m_colorPreview = false;
    }
}

// The node's deformation about its pivot, applied to the vertex positions ("Apply Deformation to Vertices",
// VertexTool): the stroke widths scale by the smaller of the node matrix's axis lengths, and the deformation
// becomes the identity.
void MeshInterface::applyDeformation()
{
    plasma::Matrix<float> matrix;
    matrix.identity();
    plasma::Vector<2, float> pivot = m_node->getNode()->transformation()->m_pivot.value();
    matrix.translate(pivot);
    matrix *= m_node->getNode()->transformation()->m_deformation.value();
    pivot = m_node->getNode()->transformation()->m_pivot.value() * -1.0;
    matrix.translate(pivot);
    const plasma::Matrix<float> &nodeMatrix = m_node->getNode()->transformation()->m_matrix;
    plasma::Vector<2, float> xAxis = nodeMatrix.transformDirection(plasma::Vector<2, float>(1.0f, 0.0f));
    plasma::Vector<2, float> yAxis = nodeMatrix.transformDirection(plasma::Vector<2, float>(0.0f, 1.0f));
    float xLength = std::sqrt(xAxis.m_elements[0] * xAxis.m_elements[0] + xAxis.m_elements[1] * xAxis.m_elements[1]);
    float yLength = std::sqrt(yAxis.m_elements[0] * yAxis.m_elements[0] + yAxis.m_elements[1] * yAxis.m_elements[1]);
    float scale = xLength < yLength ? xLength : yLength;
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        m_mesh.m_shape->m_vertexPositions.value()[vertexIndex(*it)].transform(matrix);
        m_mesh.m_shape->m_strokeWidths.value()[vertexIndex(*it)] *= scale;
    }
    m_node->getNode()->transformation()->m_deformation.value().identity();
    m_node->getNode()->transformation()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    refresh();
}

// The same with the node's rotation about z ("Apply Rotation and Deformation to Vertices", VertexTool); the
// rotation becomes zero.
void MeshInterface::applyRotationAndDeformation()
{
    plasma::Matrix<float> matrix;
    matrix.identity();
    plasma::Vector<2, float> pivot = m_node->m_node->transformation()->m_pivot.value();
    matrix.translate(pivot);
    matrix.rotateZ(m_node->m_node->transformation()->m_rotation.value().m_elements[2]);
    plasma::Transformation *transformation = m_node->m_node->transformation();
    matrix *= transformation->m_deformation.value();
    pivot = transformation->m_pivot.value() * -1.0;
    matrix.translate(pivot);
    const plasma::Matrix<float> &nodeMatrix = m_node->m_node->transformation()->m_matrix;
    plasma::Vector<2, float> xAxis = nodeMatrix.transformDirection(plasma::Vector<2, float>(1.0f, 0.0f));
    plasma::Vector<2, float> yAxis = nodeMatrix.transformDirection(plasma::Vector<2, float>(0.0f, 1.0f));
    float xLength = std::sqrt(xAxis.m_elements[0] * xAxis.m_elements[0] + xAxis.m_elements[1] * xAxis.m_elements[1]);
    float yLength = std::sqrt(yAxis.m_elements[0] * yAxis.m_elements[0] + yAxis.m_elements[1] * yAxis.m_elements[1]);
    float scale = xLength < yLength ? xLength : yLength;
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it) {
        m_mesh.m_shape->m_vertexPositions.value()[vertexIndex(*it)].transform(matrix);
        m_mesh.m_shape->m_strokeWidths.value()[vertexIndex(*it)] *= scale;
    }
    transformation = m_node->m_node->transformation();
    transformation->m_deformation.value().identity();
    transformation->m_rotation.value() = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    m_node->m_node->transformation()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    refresh();
}

// The selection flags in element order (the selection operations' records).
std::vector<bool> MeshInterface::getVertexSelection()
{
    std::vector<bool> selection(m_mesh.getVertexCount());
    int i = 0;
    for (HalfEdgeMesh::VertexIterator it = m_mesh.beginVertices(); it != m_mesh.endVertices(); ++it)
        selection[i++] = isVertexSelected(*it);
    return selection;
}

std::vector<bool> MeshInterface::getEdgeSelection()
{
    std::vector<bool> selection(m_mesh.getEdgeCount());
    int i = 0;
    for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it)
        selection[i++] = isEdgeSelected(*it);
    return selection;
}

std::vector<bool> MeshInterface::getFaceSelection()
{
    std::vector<bool> selection(m_mesh.getFaceCount());
    int i = 0;
    for (HalfEdgeMesh::FaceIterator it = m_mesh.beginFaces(); it != m_mesh.endFaces(); ++it)
        selection[i++] = isFaceSelected(*it);
    return selection;
}

void MeshInterface::refreshOrRelease(bool refresh)
{
    if (refresh) {
        this->refresh();
        return;
    }
    m_wireFrame.clear();
    discardModifyState();
}

// Forgets the state recorded before the current modification.
void MeshInterface::discardModifyState()
{
    m_state = MeshState();
}

// GLU treats vertex data as opaque. Here it carries a 32-bit vertex index, not a pointer to a vertex; the callback
// never dereferences it.
static void __stdcall appendMeshIndex(void *vertexData, MeshTessellation *tessellation)
{
    tessellation->m_indices.push_back(static_cast<int>(reinterpret_cast<PLASMA_POINTER_INT>(vertexData)));
}

// The GLU combine callback blends attributes from four vertex indices. The void* slots carry indices, not object
// pointers; the result is another index.
static void __stdcall combineMeshVertex(double *coordinates, void **vertexData, float *weights, void **result,
                                        MeshTessellation *tessellation)
{
    plasma::Vector<2, float> position(0.0f, 0.0f);
    plasma::Vector<2, float> textureCoordinate(0.0f, 0.0f);
    plasma::Vector<4, float> vertexColor(0.0f, 0.0f, 0.0f, 0.0f);
    plasma::Vector<4, float> strokeColor(0.0f, 0.0f, 0.0f, 0.0f);
    plasma::Vector<4, float> extrusionFrontColor(0.0f, 0.0f, 0.0f, 0.0f);
    plasma::Vector<4, float> extrusionBackColor(0.0f, 0.0f, 0.0f, 0.0f);
    float width = 0.0f;
    plasma::SmoothMeshShape *shape = tessellation->m_shape;
    const plasma::Vector<2, float> *positions = shape->m_vertexPositions.value().data();
    const plasma::Vector<2, float> *textureCoordinates = shape->m_vertexTexCoords.value().data();
    const plasma::Vector<4, float> *vertexColors = shape->m_vertexColors.value().data();
    const plasma::Vector<4, float> *strokeColors = shape->m_strokeColors.value().data();
    const plasma::Vector<4, float> *extrusionFrontColors = shape->m_extrusionFrontColors.value().data();
    const plasma::Vector<4, float> *extrusionBackColors = shape->m_extrusionBackColors.value().data();
    const float *widths = shape->m_strokeWidths.value().data();
    for (int i = 0; i < 4; ++i) {
        int index = static_cast<int>(reinterpret_cast<PLASMA_POINTER_INT>(vertexData[i]));
        position += positions[index] * weights[i];
        textureCoordinate += textureCoordinates[index] * weights[i];
        vertexColor += vertexColors[index] * weights[i];
        strokeColor += strokeColors[index] * weights[i];
        extrusionFrontColor += extrusionFrontColors[index] * weights[i];
        extrusionBackColor += extrusionBackColors[index] * weights[i];
        width += widths[index] * weights[i];
    }
    int index = tessellation->m_shape->m_vertexPositions.value().size();
    tessellation->m_shape->m_vertexPositions.append(position);
    tessellation->m_shape->m_vertexTexCoords.append(textureCoordinate);
    tessellation->m_shape->m_vertexColors.append(vertexColor);
    tessellation->m_shape->m_strokeColors.append(strokeColor);
    tessellation->m_shape->m_extrusionFrontColors.append(extrusionFrontColor);
    tessellation->m_shape->m_extrusionBackColors.append(extrusionBackColor);
    tessellation->m_shape->m_strokeWidths.append(width);
    *result = reinterpret_cast<void *>(index);
}

// Edit menu "Create Inner Edges": tessellates the shape's contours into triangles, makes them the shape's faces
// and rebuilds the mesh, then dissolves every inner edge whose two faces together form a polygon that winds one
// way around its centre.
void MeshInterface::createInnerEdges()
{
    GLUtesselator *tessellator = gluNewTess();
    gluTessCallback(tessellator, GLU_TESS_VERTEX_DATA, reinterpret_cast<void(__stdcall *)()>(appendMeshIndex));
    gluTessCallback(tessellator, GLU_TESS_COMBINE_DATA, reinterpret_cast<void(__stdcall *)()>(combineMeshVertex));
    gluTessCallback(tessellator, GLU_TESS_EDGE_FLAG,
                    reinterpret_cast<void(__stdcall *)()>(plasma::tessellationEdgeFlag));
    gluTessProperty(tessellator, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE);
    MeshTessellation tessellation;
    tessellation.m_shape = m_mesh.m_shape;
    gluTessBeginPolygon(tessellator, &tessellation);
    for (int contour = 0; contour < (int)m_mesh.m_shape->m_contours.size(); ++contour) {
        gluTessBeginContour(tessellator);
        for (int i = 0; i < (int)m_mesh.m_shape->m_contours[contour].size(); ++i) {
            int index = m_mesh.m_shape->m_contours[contour][i];
            const plasma::Vector<2, float> &position = m_mesh.m_shape->m_vertexPositions.value()[index];
            plasma::Vector<3, GLdouble> coordinates(position.m_elements[0], position.m_elements[1], 0.0);
            gluTessVertex(tessellator, coordinates.m_elements, reinterpret_cast<void *>(index));
        }
        gluTessEndContour(tessellator);
    }
    gluTessEndPolygon(tessellator);
    gluDeleteTess(tessellator);

    // The shape's faces become the triangles.
    std::vector<std::vector<int>> triangles;

    // GLU emits complete triangle triples when an edge-flag callback is set.
    for (int i = 0; i < (int)tessellation.m_indices.size(); i += 3) {
        std::vector<int> triangle(3);
        triangle[0] = tessellation.m_indices[i];
        triangle[1] = tessellation.m_indices[i + 1];
        triangle[2] = tessellation.m_indices[i + 2];
        triangles.push_back(triangle);
    }
    m_mesh.m_shape->setFaces(triangles, true);
    m_mesh.rebuild();
    refresh();

// Each merge changes the edge list: start over.
restart:
    for (HalfEdgeMesh::EdgeIterator it = m_mesh.beginEdges(); it != m_mesh.endEdges(); ++it) {
        if ((*it)->m_opposite && (*it)->m_opposite->m_face) {
            std::list<MeshFace *> faces;
            faces.push_back((*it)->m_face);
            faces.push_back((*it)->m_opposite->m_face);
            std::list<MeshHalfEdge *> boundary;
            m_mesh.getBoundaryEdges(boundary, faces);
            m_mesh.orderBoundaryEdges(boundary);
            std::vector<MeshHalfEdge *> edges(boundary.begin(), boundary.end());
            plasma::Vector<2, float> center(0.0f, 0.0f);
            for (int i = 0; i < (int)edges.size(); ++i)
                center += m_mesh.m_shape->m_vertexPositions.value()[vertexIndex(edges[i]->m_vertex)];
            center = center / (float)edges.size();
            int negative = 0;
            int positive = 0;
            for (int i = 0; i < (int)edges.size(); ++i) {
                unsigned next = (i + 1) % edges.size();
                plasma::Vector<2, float> first =
                    m_mesh.m_shape->m_vertexPositions.value()[vertexIndex(edges[i]->m_vertex)] - center;
                plasma::Vector<2, float> second =
                    m_mesh.m_shape->m_vertexPositions.value()[vertexIndex(edges[next]->m_vertex)] - center;
                float cross = second.m_elements[0] * first.m_elements[1] - second.m_elements[1] * first.m_elements[0];
                if (cross < 0.0f)
                    ++negative;
                if (cross > 0.0f)
                    ++positive;
            }
            if (negative + positive && !(negative * positive)) {
                std::list<MeshVertex *> vertices;
                for (int i = 0; i < (int)edges.size(); ++i)
                    vertices.push_back(edges[i]->m_vertex);
                m_mesh.removeEdgeFaces(*it);
                m_mesh.addFace(vertices, std::vector<char>());
                goto restart;
            }
        }
    }
    m_mesh.updateShape();
    refresh();
    m_hasChanges = true;
}

// One subdivision step of contour points; every second point is new. SmoothMeshShape.cpp uses the same refinement
// with a separate corner weight.
static void refineContourPoints(std::vector<int> &types, unsigned step,
                                std::vector<plasma::Vector<2, float>> &positions, int segments, bool closed,
                                float weight, int level)
{
    unsigned previous = 0;
    unsigned next = step;

    // Offset + next wraps to the midpoint index before the modulo access.
    int midpointOffset = (int)step / 2 - step;
    for (int i = 0; i < segments; ++i) {
        unsigned middle = midpointOffset + next;
        plasma::Vector<2, float> average =
            (positions[previous % positions.size()] + positions[next % positions.size()]) * 0.5f;
        positions[middle % positions.size()] = average;
        float blend = 0.5f;
        if (!level) {
            // The code rounds the cosine to float before the double arithmetic.
            static float cornerWeight = 1.0 / ((double)(float)cos((double)0.785398185f) * 2.0 + 1.0);
            unsigned short firstType = (unsigned short)types[previous % types.size()];
            if (firstType == plasma::VERTEX_ROUND && (unsigned short)types[next % types.size()] != plasma::VERTEX_ROUND)
                blend = cornerWeight;
            if ((unsigned short)types[next % types.size()] == plasma::VERTEX_ROUND && firstType != plasma::VERTEX_ROUND)
                blend = 1.0f - cornerWeight;
        }
        positions[middle % positions.size()] =
            positions[previous % positions.size()] * blend + positions[next % positions.size()] * (1.0f - blend);
        unsigned short firstType = (unsigned short)types[previous % types.size()];
        unsigned short secondType;
        if (firstType == plasma::VERTEX_BEZIER ||
            (secondType = (unsigned short)types[next % types.size()]) == plasma::VERTEX_BEZIER ||
            (!level && (firstType == plasma::VERTEX_ROUND || secondType == plasma::VERTEX_ROUND)))
            types[middle % types.size()] = plasma::VERTEX_BEZIER;
        else
            types[middle % types.size()] = plasma::VERTEX_SMOOTH;
        previous += step;
        next += step;
    }
    if (closed)
        ++segments;
    unsigned current = step;
    for (int i = 1; i < segments; ++i) {
        unsigned short type = (unsigned short)types[current % types.size()];
        if (type != plasma::VERTEX_SHARP && (type != plasma::VERTEX_ROUND || !level)) {
            plasma::Vector<2, float> midpoint = (positions[(current - step / 2) % positions.size()] +
                                                 positions[(current + step / 2) % positions.size()]) *
                                                0.5f;
            if (!type)
                positions[current % positions.size()] =
                    positions[current % positions.size()] + (midpoint - positions[current % positions.size()]) * weight;
            else
                positions[current % positions.size()] = midpoint;
        }
        current += step;
    }
    previous = 0;
    for (int i = 0; i < segments; ++i) {
        unsigned short firstType = (unsigned short)types[previous % types.size()];
        unsigned short secondType = (unsigned short)types[(previous + step) % types.size()];
        if ((firstType == plasma::VERTEX_BEZIER || (!level && firstType == plasma::VERTEX_ROUND)) &&
            (secondType == plasma::VERTEX_BEZIER || (!level && secondType == plasma::VERTEX_ROUND))) {
            positions[(previous + step / 2) % positions.size()] =
                (positions[previous % positions.size()] + positions[(previous + step) % positions.size()]) * 0.5f;
            types[(previous + step / 2) % types.size()] = plasma::VERTEX_SHARP;
        }
        previous += step;
    }
}
