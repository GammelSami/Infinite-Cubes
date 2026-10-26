// InterfaceViewWidget: the design sheet's view widget. Its rendering draws the sheet's grid and page borders, the
// scene through the root's sheet node and, around the scene pointer, the radius circles of the tools that work
// within a radius.
#include <cmath>
#include "DesignSheet.h"
#include "Workspace.h"
#include "ToolModule.h"
#include "Tool.h"
#include "MeshModule.h"
#include "StrokeWidthModule.h"
#include "VertexModule.h"
#include "TextureSpaceWidget.h"
#include "plasma/GLEngine.h"
#include "plasma/Transformation.h"
#include <GL/glew.h>

using plasma::Vector;

static const int SoftSelectionDiscSegments = 50;
static const int DefaultSelectionDetail = 6;
static const int MediumSelectionThreshold = 50;
static const int MediumSelectionDetail = 3;
static const int LargeSelectionThreshold = 100;
static const int LargeSelectionDetail = 2;
static const int VeryLargeSelectionThreshold = 200;
static const int VeryLargeSelectionDetail = 1;

InterfaceViewWidget::InterfaceViewWidget(plasma::Engine *engine, DesignSheet *sheet, plasma::Node *node)
    : plasma::Widget(engine, node, L"interfaceview"),
      m_sheet(sheet)
{}

InterfaceViewWidget::~InterfaceViewWidget() {}

Vector<2, float> InterfaceViewWidget::getMinimum()
{
    return transformPoint(m_node->m_inverseMatrix, Vector<2, float>(0.0f, 0.0f));
}

Vector<2, float> InterfaceViewWidget::getMaximum()
{
    return m_node->m_inverseMatrix *
           Vector<2, float>((float)m_engine->m_viewportWidth, (float)m_engine->m_viewportHeight);
}

// The vertex tools' soft selection: six discs of growing radius, more opaque towards the centre.
static void drawSoftSelection(const Vector<2, float> &center, float radius)
{
    for (int i = 0; i < 6; ++i) {
        // The disc's fraction of the radius, scaled to its radius below.
        float t = ((double)i + 1.0) / 6.0;
        glBegin(GL_TRIANGLE_FAN);
        float green = (t + 1.0) * 0.5;
        glColor4f(0.0f, green, green, 0.1f);
        t *= radius;
        for (int j = 0; j <= SoftSelectionDiscSegments; ++j) {
            float angle = (double)j * 6.283185307179586 / (double)SoftSelectionDiscSegments;
            float x = cos(angle);
            float y = sin(angle);
            Vector<2, float> point = center + Vector<2, float>(x, y) * t;
            glVertex2fv(point.m_elements);
        }
        glEnd();
    }
}

// Pointer preview rings: one-pixel inner inset, two-pixel outer outset; cyan stroke preview alpha 0.4 and white
// tool preview alpha 0.6.
void InterfaceViewWidget::draw()
{
    static_cast<plasma::GLEngine *>(m_engine)->useFixedFunction();
    glEnable(GL_LINE_SMOOTH);
    glDisable(GL_TEXTURE_2D);
    float width = m_sheet->getPanel()->viewWidth();
    float height = m_sheet->getPanel()->getViewHeight();
    glPushAttrib(GL_SCISSOR_BIT);
    glEnable(GL_SCISSOR_TEST);
    int viewportHeight = m_engine->m_viewportHeight;
    glScissor((int)m_sheet->getPanel()->getContentPosition().m_elements[0],
              viewportHeight - (int)(height + m_sheet->getPanel()->getContentPosition().m_elements[1]), (int)width,
              (int)height);
    glDisableClientState(GL_INDEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    m_sheet->drawGrid();
    m_sheet->drawPageBorders();
    DesignSheet *sheet = m_sheet;
    if (!sheet->m_workspace->isPanning() && !sheet->isBusy()) {
        glEnable(GL_LINE_SMOOTH);
        int selected = m_sheet->countSelectedNodes();
        m_sheet->m_detailLevel = DefaultSelectionDetail;
        if (selected > MediumSelectionThreshold)
            m_sheet->m_detailLevel = MediumSelectionDetail;
        if (selected > LargeSelectionThreshold)
            m_sheet->m_detailLevel = LargeSelectionDetail;
        if (selected > VeryLargeSelectionThreshold)
            m_sheet->m_detailLevel = VeryLargeSelectionDetail;
        if (isDisplayVisible(m_sheet->m_root->m_display) && m_sheet->m_root->isShown())
            m_sheet->findSheetNode(m_sheet->m_root)->draw();
        glDisableClientState(GL_INDEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glMultMatrixf(m_node->m_inverseMatrix.m_elements);
        glLineWidth(1.0f);

        // The stroke tools' radius: the stroke width at the root's scale.
        Workspace *workspace = m_sheet->m_workspace;
        bool stroked = workspace->m_meshModule->m_shapeMode != MeshModule::SHAPE_MODE_CONTOUR &&
                       workspace->m_meshModule->m_shapeMode != MeshModule::SHAPE_MODE_MESH;
        int type = workspace->m_toolModule->getCurrentTool()->getType();
        if (type == Tool::TOOL_DRAW_FACE && stroked || type == Tool::TOOL_DRAW_FREEHAND || type == Tool::TOOL_WIDTH) {
            Vector<2, float> center = workspace->m_sceneMousePosition;
            if (workspace->m_guiPointerNode) {
                // The panel's centre.
                Vector<2, float> size = m_sheet->getPanel()->getViewSize();
                Vector<2, float> half = size;
                half.m_elements[0] *= 0.5;
                half.m_elements[1] *= 0.5;
                center = m_sheet->getPanel()->getContentPosition() + half;
            }
            float radius = m_sheet->m_root->m_transformation->m_deformation.value().m_elements[0] * 0.5 *
                           m_sheet->m_workspace->m_strokeWidthModule->m_width;
            drawAnnulus(center, radius - 1.0, radius + 2.0, Vector<4, float>(0.0f, 0.0f, 0.0f, 0.4f),
                        Vector<4, float>(0.0f, 1.0f, 1.0f, 0.4f));
        }

        // The tool radius ("Resize Tool Radius...").
        if (!m_sheet->m_workspace->m_guiPointerNode) {
            int type = m_sheet->m_workspace->m_toolModule->getCurrentTool()->getType();
            if (type != Tool::TOOL_DRAW_FREEHAND && type != Tool::TOOL_EXTRUSION ||
                m_sheet->m_workspace->m_resizeToolRadius) {
                static_cast<plasma::GLEngine *>(m_engine)->useFixedFunction();
                glEnable(GL_BLEND);
                glEnable(GL_LINE_SMOOTH);
                Workspace *workspace = m_sheet->m_workspace;
                int type = workspace->m_toolModule->getCurrentTool()->getType();
                if ((type == Tool::TOOL_VERTEX || type == Tool::TOOL_EDGE || type == Tool::TOOL_FACE) &&
                    workspace->m_vertexModule->m_softSelection)
                    drawSoftSelection(m_sheet->m_workspace->m_sceneMousePosition, m_sheet->m_workspace->m_toolRadius);
                Workspace *current = m_sheet->m_workspace;
                drawAnnulus(current->m_sceneMousePosition, current->m_toolRadius - 1.0, current->m_toolRadius + 2.0,
                            Vector<4, float>(0.0f, 0.0f, 0.0f, 0.6f), Vector<4, float>(1.0f, 1.0f, 1.0f, 0.6f));
            }
        }
        DesignSheet *designSheet = m_sheet;
        if (designSheet == designSheet->m_workspace->getDesignSheet())
            designSheet->m_workspace->drawConstraintIcons();
        glPopMatrix();
    }
    m_sheet->renderOverlays();
    glPopAttrib();
}
