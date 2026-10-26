// Text-tool support for grouping nodes within an invisible rectangle.
#include "ToolModule.h"
#include "DesignSheet.h"
#include "WidgetModule.h"
#include "ProceduralShapeModule.h"
#include "CreateNodeOperation.h"
#include <GL/glew.h>

#include "plasma/SmoothMeshShape.h"

plasma::Shape *createGroupBoundary(const plasma::Vector<4, float> &color, plasma::Node *node)
{
    plasma::Matrix<float> matrix;
    matrix.identity();
    bool empty = true;
    plasma::Vector<2, float> minimum;
    plasma::Vector<2, float> maximum;
    node->extendShapeBounds(minimum, maximum, matrix, empty);
    plasma::SmoothMeshShape *shape = node->getEngine()->createSmoothMeshShape(L"Group");
    shape->m_vertexPositions.resize(4);
    shape->m_vertexTexCoords.resize(4);
    shape->m_vertexFlags.resize(4, plasma::VERTEX_SHARP);
    shape->m_vertexColors.resize(4);
    shape->m_strokeColors.resize(4);
    shape->m_extrusionBackColors.resize(4);
    shape->m_extrusionFrontColors.resize(4);
    shape->m_vertexPositions.value()[0] = minimum;
    shape->m_vertexPositions.value()[1] = plasma::Vector<2, float>(maximum.m_elements[0], minimum.m_elements[1]);
    shape->m_vertexPositions.value()[2] = maximum;
    shape->m_vertexPositions.value()[3] = plasma::Vector<2, float>(minimum.m_elements[0], maximum.m_elements[1]);
    shape->m_vertexTexCoords.value()[0] = plasma::Vector<2, float>(0.0f, 0.0f);
    shape->m_vertexTexCoords.value()[1] = plasma::Vector<2, float>(1.0f, 0.0f);
    shape->m_vertexTexCoords.value()[2] = plasma::Vector<2, float>(1.0f, 1.0f);
    shape->m_vertexTexCoords.value()[3] = plasma::Vector<2, float>(0.0f, 1.0f);
    shape->m_vertexColors.value()[0] = color;
    shape->m_vertexColors.value()[1] = color;
    shape->m_vertexColors.value()[2] = color;
    shape->m_vertexColors.value()[3] = color;
    shape->m_strokeColors.value()[0] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    shape->m_strokeColors.value()[1] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    shape->m_strokeColors.value()[2] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    shape->m_strokeColors.value()[3] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    shape->m_extrusionFrontColors.value()[0] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionFrontColors.value()[1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionFrontColors.value()[2] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionFrontColors.value()[3] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionBackColors.value()[0] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionBackColors.value()[1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionBackColors.value()[2] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionBackColors.value()[3] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->setSubdivisions(1, false);
    std::vector<std::vector<int>> faces(1);
    faces[0].resize(4);
    faces[0][0] = 0;
    faces[0][1] = 1;
    faces[0][2] = 2;
    faces[0][3] = 3;
    shape->setFaces(faces, true);
    node->setShape(shape);
    return shape;
}

#include "StampModule.h"
#include "ClipboardModule.h"
#include <cmath>
