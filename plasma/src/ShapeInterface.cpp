#include "ShapeInterface.h"
#include "NodeInterface.h"

// The default bounds: empty (the minimum above the maximum).
void ShapeInterface::getBounds(plasma::Vector<2, float> *minimum, plasma::Vector<2, float> *maximum)
{
    *minimum = plasma::Vector<2, float>(1.0f, 1.0f);
    *maximum = plasma::Vector<2, float>(-1.0f, -1.0f);
}

// Returns empty bounds, as GetBounds does.
void ShapeInterface::getSelectionBounds(plasma::Vector<2, float> *minimum, plasma::Vector<2, float> *maximum)
{
    *minimum = plasma::Vector<2, float>(1.0f, 1.0f);
    *maximum = plasma::Vector<2, float>(-1.0f, -1.0f);
}

ShapeInterface::ShapeInterface(NodeInterface *node, ShapeInterfaceType type)
    : m_sheet(node->m_sheet),
      m_node(node),
      m_type(type),
      m_hasChanges(false),
      m_refreshPending(false),
      m_hasMatrix(false),
      m_colorPreview(false)
{}

ShapeInterface::~ShapeInterface() {}

void ShapeInterface::setMatrix(const plasma::Matrix<float> &matrix)
{
    m_hasMatrix = true;
    m_matrix = matrix;
}

void ShapeInterface::resetMatrix()
{
    m_hasMatrix = false;
}
