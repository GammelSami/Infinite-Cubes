// Generic-shape support for scene bounds and source drawing.
#include "plasma/GenericShape.h"
#include "plasma/Drawing.h"
#include "TexturePreviewWidget.h"
#include "TextureSpaceWidget.h"
#include "TextureModule.h"
#include "DesignSheet.h"
#include "FormatModule.h"
#include "ShapeInterface.h"
#include <GL/glew.h>

using namespace plasma;

void GenericShape::updateDrawing()
{
    m_drawing->clear();
    if (!m_instanceNode) {
        Vector<2, float> minimum = m_position;
        Vector<2, float> maximum = m_position + m_size;
        m_drawing->m_pathVertices.resize(4);
        m_drawing->m_pathColors.resize(4);
        m_drawing->m_pathColors[0] = Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f);
        m_drawing->m_pathColors[1] = Vector<4, float>(0.6f, 0.6f, 0.6f, 1.0f);
        m_drawing->m_pathColors[2] = Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f);
        m_drawing->m_pathColors[3] = Vector<4, float>(0.6f, 0.6f, 0.6f, 1.0f);
        m_drawing->m_pathTextureCoordinates.resize(4, Vector<2, float>(0.0f, 0.0f));
        m_drawing->m_pathVertices[0] = minimum;
        m_drawing->m_pathVertices[1] = Vector<2, float>(maximum.m_elements[0], minimum.m_elements[1]);
        m_drawing->m_pathVertices[2] = maximum;
        m_drawing->m_pathVertices[3] = Vector<2, float>(minimum.m_elements[0], maximum.m_elements[1]);
        m_drawing->m_pathIndices.resize(6);
        m_drawing->m_pathIndices[0] = 0;
        m_drawing->m_pathIndices[1] = 1;
        m_drawing->m_pathIndices[2] = 2;
        m_drawing->m_pathIndices[3] = 2;
        m_drawing->m_pathIndices[4] = 3;
        m_drawing->m_pathIndices[5] = 0;
    }
    m_drawing->update();
}
