// PDF writer support for transformation matrices and rasterized scene nodes.
#include "pdf/PDFWriter.h"
#include "Workspace.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"

const plasma::Matrix<float> &getTransformationMatrix(const plasma::Transformation *transformation)
{
    return transformation->getMatrix();
}

#include "pdf/PDFWriter.h"
#include "Workspace.h"
#include "ExportModule.h"
#include "plasma/Display.h"
#include "plasma/Drawing.h"
#include "plasma/Node.h"
#include "plasma/SmoothMeshShape.h"

void PDFWriter::prepareRasterizedNodes(plasma::Node *node)
{
    if (!node->m_display->m_visibility.value() || !node->isShown())
        return;
    bool blur = (m_options & kPDFRasterizeBlur) && (node->m_display->m_flags & (1 << plasma::DISPLAY_BLUR)) &&
                node->m_display->m_blurRadius.value() > 0.0f;
    ++m_nodeCount;
    if (blur || (m_options & kPDFRasterizeAll)) {
        plasma::Matrix<float> matrix = m_currentRoot->m_inverseMatrix;
        matrix *= node->m_matrix;
        plasma::Vector<2, float> minimum, maximum;
        bool empty = true;
        node->extendBounds(minimum, maximum, matrix, empty, true);
        if (!empty) {
            Image *&image = m_rasterizedNodes[node];
            image = m_workspace->m_exportModule->renderImage(node, minimum, maximum, 0);
        }
        return;
    }
    if (m_options & kPDFRasterizeTextures) {
        plasma::Shape *shape = node->m_shape;
        if (shape && shape->getType() == plasma::SHAPE_SMOOTH_MESH) {
            plasma::SmoothMeshShape *mesh = static_cast<plasma::SmoothMeshShape *>(shape);
            if (!(mesh->m_flags & (1 << plasma::MESH_NO_FILL)) && mesh->m_fillDrawing &&
                mesh->m_fillDrawing->m_texture) {
                plasma::Vector<2, float> minimum, maximum;
                bool empty = true;
                mesh->m_fillDrawing->getTransformedBounds(m_currentRoot->m_inverseMatrix * node->m_matrix, minimum,
                                                          maximum, empty);
                if (!empty) {
                    plasma::Drawing *drawing = mesh->m_fillDrawing;
                    Image *&image = m_rasterizedDrawings[std::make_pair(node, (const plasma::Drawing *)drawing)];
                    image = m_workspace->m_exportModule->renderImage(node, minimum, maximum, drawing);
                }
            }
            if ((mesh->m_flags & (1 << plasma::MESH_STROKE)) && mesh->m_strokeDrawing &&
                mesh->m_strokeDrawing->m_texture) {
                plasma::Vector<2, float> minimum, maximum;
                bool empty = true;
                mesh->m_strokeDrawing->getTransformedBounds(m_currentRoot->m_inverseMatrix * node->m_matrix, minimum,
                                                            maximum, empty);
                if (!empty) {
                    plasma::Drawing *drawing = mesh->m_strokeDrawing;
                    Image *&image = m_rasterizedDrawings[std::make_pair(node, (const plasma::Drawing *)drawing)];
                    image = m_workspace->m_exportModule->renderImage(node, minimum, maximum, drawing);
                }
            }
        }
    }
    for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        prepareRasterizedNodes(*it);
}
