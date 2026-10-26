// GenericInterface.cpp: the editing interface of the other shapes (a GenericShape): it has no vertices, edges or
// faces, so it keeps ShapeInterface's defaults except for the bounds.
#include "NodeInterface.h"
#include "HalfEdgeMesh.h"
#include "plasma/GenericShape.h"
#include "plasma/Node.h"
#include "plasma/Widget.h"

GenericInterface::GenericInterface(NodeInterface *node, plasma::GenericShape *shape)
    : ShapeInterface(node, INTERFACE_GENERIC),
      m_shape(shape)
{}

// Draws nothing.
void GenericInterface::draw()
{
    plasma::Vector<2, float> unused;
    unused = plasma::Vector<2, float>(0.0f, 0.0f);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Empty bounds start
// with positive minima and negative maxima. The bounds of the shape's four corners in sheet coordinates: deformed
// by the node's enclosing widget, if any, then transformed by the node's matrix.
void GenericInterface::getBounds(plasma::Vector<2, float> *minimum, plasma::Vector<2, float> *maximum)
{
    plasma::Vector<2, float> corners[4] = {
        plasma::Vector<2, float>(m_shape->getMinimum()[0], m_shape->getMinimum()[1]),
        plasma::Vector<2, float>(m_shape->getMaximum()[0], m_shape->getMinimum()[1]),
        plasma::Vector<2, float>(m_shape->getMaximum()[0], m_shape->getMaximum()[1]),
        plasma::Vector<2, float>(m_shape->getMinimum()[0], m_shape->getMaximum()[1]),
    };
    if (m_node->getNode()->m_enclosingWidget) {
        for (int i = 0; i < 4; i++)
            corners[i] = m_node->getNode()->m_enclosingWidget->deform(corners[i], m_node->getNode());
    }
    *minimum = plasma::Vector<2, float>(1e10f, 1e10f);
    plasma::Node *node = m_node->getNode();
    *maximum = plasma::Vector<2, float>(-1e10f, -1e10f);
    for (int i = 0; i < 4; i++) {
        plasma::Vector<2, float> point = node->m_matrix.transformPoint(corners[i]);
        if (i == 0 || minimum->m_elements[0] > point.m_elements[0])
            minimum->m_elements[0] = point.m_elements[0];
        if (i == 0 || minimum->m_elements[1] > point.m_elements[1])
            minimum->m_elements[1] = point.m_elements[1];
        if (i == 0 || maximum->m_elements[0] < point.m_elements[0])
            maximum->m_elements[0] = point.m_elements[0];
        if (i == 0 || maximum->m_elements[1] < point.m_elements[1])
            maximum->m_elements[1] = point.m_elements[1];
    }
}

// Each mesh element owns a copy of its byte attributes.
MeshVertex::MeshVertex(const std::vector<char> &attributes)
    : m_attributes(attributes)
{}

MeshHalfEdge::MeshHalfEdge(const std::vector<char> &attributes)
    : m_attributes(attributes),
      m_vertex(0),
      m_opposite(0),
      m_next(0),
      m_previous(0),
      m_face(0)
{}

MeshFace::MeshFace(const std::vector<char> &attributes)
    : m_attributes(attributes),
      m_edge(0)
{}

// The number of the face's corners.
int countCorners(MeshFace *face)
{
    int count = 0;
    for (MeshFaceIterator corner(face); corner.isValid(); ++corner)
        count++;
    return count;
}
