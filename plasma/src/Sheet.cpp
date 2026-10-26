// Sheet: the base class of the workspace's sheets (DesignSheet, TextureSheet). See DesignSheet.h.
#include <windows.h>
#include <algorithm>
#include "DesignSheet.h"
#include "FormatModule.h"
#include "Workspace.h"
#include "GridModule.h"
#include "OptionsModule.h"
#include "ProceduralShapeModule.h"
#include "StampModule.h"
#include "ToolModule.h"
#include "ColorModule.h"
#include "ColorManagementModule.h"
#include "SpecialColorModule.h"
#include "ExportModule.h"
#include "plasma/FontEngine.h"
#include "plasma/ListWidget.h"
#include "plasma/ScalableFont.h"
#include "plasma/Transformation.h"
#include <cmath>
#include <sstream>
#include <GL/glew.h>
#include "plasma/Display.h"
#include "plasma/Matrix.h"
#include "plasma/RenderSurface.h"
#include "plasma/GLRenderSurface.h"
#include <vector>

RenderWidget::RenderWidget(Workspace *workspace, plasma::Node *node)
    : plasma::Widget(&workspace->m_engine, node, L""),
      m_workspace(workspace)
{}

// With colour management on, reads the rendered view back, soft-proofs it through the output profile and draws it
// again.
void RenderWidget::surfaceRendered()
{
    if (!m_workspace->m_colorManagementModule->m_enabled)
        return;
    plasma::GLRenderSurface *surface = dynamic_cast<plasma::GLRenderSurface *>(m_surface);
    if (!surface || surface->m_width < 1 || surface->m_height < 1)
        return;
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
    std::vector<plasma::Vector<4, unsigned char>> pixels; // Resize.
    pixels.resize(surface->m_width * surface->m_height);
    glReadPixels(0, 0, surface->m_width, surface->m_height, GL_BGRA, GL_UNSIGNED_BYTE, &pixels[0]);
    m_workspace->m_colorManagementModule->softProof(&pixels[0], pixels.size());
    glDisable(GL_DEPTH_TEST);
    glDrawPixels(surface->m_width, surface->m_height, GL_BGRA, GL_UNSIGNED_BYTE, &pixels[0]);
    glEnable(GL_DEPTH_TEST);
}

Sheet::Sheet(Workspace *workspace)
    : m_page(1000.0f, 1000.0f, 72.0f),
      m_gridSize(10.0f),
      m_loadResult(LOAD_SUCCEEDED),
      m_closeIfLoadFails(false),
      m_workspace(workspace),
      m_thread(0),
      m_busy(false),
      m_loadFinished(false),
      m_root(0),
      m_progressNode(0),
      m_frame(0),
      m_panel(0),
      m_viewWidget(0),
      m_zoomScale(1.0f),
      m_modified(false)
{
    // A copy of the workspace's "sheet" template node gives the frame and the panel; the frame moves to the
    // workspace's sheets node.
    plasma::Node *sheetNode = workspace->m_sheetTemplate->clone(workspace->m_sheetsNode);
    m_frame = sheetNode->findWidget(L"sheet:frame");
    if (!m_frame)
        return;
    m_frame->getNode()->m_userData = this;
    m_panel = sheetNode->findWidget(L"sheet:panel");
    m_panel->getNode()->m_display->m_clipping.value() = 0;
    m_frame->getNode()->setParent(workspace->m_sheetsNode);
    plasma::Engine *engine = &workspace->m_engine;
    engine->destroyNode(sheetNode);

    plasma::Node *viewNode = engine->createNode(0, 0, 0, m_panel->getNode(), L"");
    m_viewWidget = new RenderWidget(workspace, viewNode);
    m_viewWidget->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_viewWidget->setViewRect(plasma::Vector<2, float>(0.0f, 0.0f), m_panel->getViewSize());
    m_viewWidget->m_flags |= (1 << plasma::WIDGET_ADAPT_WIDTH);
    m_viewWidget->m_flags |= (1 << plasma::WIDGET_ADAPT_HEIGHT);
    plasma::Node *contentNode = engine->createNode(0, 0, 0, viewNode, L"");
    engine->createWidget(contentNode);
    m_root = engine->createNode(0, 0, 0, contentNode, L"");
    // Played even while hidden (Node::Animate forces such nodes).
    m_root->m_flags |= (1 << plasma::NODE_ALWAYS_ANIMATE);

    // The "progress" animation, from the workspace's template.
    m_progressNode = workspace->m_progressDisplay->clone(m_panel->getNode());
    engine->createWidget(m_progressNode);
    m_progressNode->m_display->m_visibility.value() = 0;
    m_frame->getNode()->animate(0);
}

Sheet::~Sheet() {}

// Expands the frame to fill the window, preserving its view rectangle for RestoreFrame. The first two results are
// unused.
void Sheet::expandFrame()
{
    plasma::Vector<2, float> screenSize = m_frame->screenSize();
    plasma::Vector<2, float> contentPosition = m_frame->getContentPosition();
    m_savedFramePosition = m_frame->getViewPosition();
    m_savedFrameSize = m_frame->getViewSize();
    plasma::Vector<2, float> panelPosition = m_panel->getViewPosition();
    plasma::Vector<2, float> origin(panelPosition.m_elements[0] * -1.0, -1.0 * panelPosition.m_elements[1]);
    m_frame->setViewPosition(m_workspace->m_sheetsNode->m_inverseMatrix.transform(origin), true);
    plasma::Vector<2, float> size(panelPosition.m_elements[0] * 2.0 + m_workspace->m_engine.m_viewportWidth,
                                  2.0 * panelPosition.m_elements[1] + m_workspace->m_engine.m_viewportHeight);
    m_frame->resize(m_workspace->m_sheetsNode->m_inverseMatrix.transformDirection(size));
}

// The frame's view rectangle from before ExpandFrame.
void Sheet::restoreFrame()
{
    m_frame->setViewRect(m_savedFramePosition, m_savedFrameSize);
    m_frame->m_node->animate(0);
}

void Sheet::setBusy(bool busy, bool saving)
{
    m_busy = busy;
    m_saving = saving;
    if (m_busy) {
        m_progressNode->m_display->m_visibility.value() = 1;
        m_progressNode->play(L"progress", m_progressNode->getSequenceEnd(L"progress"));
    } else {
        m_progressNode->m_display->m_visibility.value() = 0;
        m_progressNode->stop();
    }
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void Sheet::renderOverlays()
{
    // While the sheet loads or saves, the progress animation sits in the middle of the view.
    if (m_busy) {
        plasma::Widget *panel = m_panel;
        float width = panel->viewWidth();
        float height = panel->getViewHeight();
        plasma::Vector<2, float> halfSize(width * 0.5, 0.5 * height);
        m_progressNode->m_transformation->m_translation.value() = m_panel->getBindPos() + halfSize;
        m_progressNode->m_transformation->m_deformation.value().identity();
        float size = min(m_panel->viewWidth(), m_panel->getViewHeight()); // Not used.
        m_progressNode->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
    Workspace *workspace = m_workspace;
    if (workspace->isPanning())
        return;
    // With the sheet tool, the view is lightened and the edge zones under the pointer are shaded: a press there
    // resizes the frame from that edge, inside it moves the frame (Workspace::PointerPressed).
    if (workspace->m_toolModule->m_currentTool->getType() == Tool::TOOL_SHEET &&
        !workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) && !workspace->m_currentSheetOnly) {
        float width = m_panel->viewWidth();
        float height = m_panel->getViewHeight();

        // Computed and not used.
        plasma::Vector<2, float> pointer =
            m_panel->m_node->m_inverseMatrix * plasma::Vector<2, float>(m_workspace->m_engine.m_pointer);
        if (m_panel->getNode()->isAncestorOf(m_workspace->getHoveredNode())) {
            plasma::Vector<2, float> local =
                m_panel->m_node->m_inverseMatrix * plasma::Vector<2, float>(m_workspace->m_engine.m_pointer);
            plasma::Vector<2, float> point = m_panel->undeform(local, m_panel->m_node);
            plasma::Vector<2, float> minimum = m_panel->m_node->m_shape->getMinimum();
            plasma::Vector<2, float> maximum = m_panel->m_node->m_shape->getMaximum();
            if (m_panel->m_node->m_shape && m_panel->m_node->m_shape->contains(point)) {
                m_workspace->m_engine.useFixedFunction();
                glDisable(GL_TEXTURE_2D);
                glDisable(GL_DEPTH_TEST);
                glBegin(GL_QUADS);
                glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
                glVertex2f(0.0f, 0.0f);
                glVertex2f(width, 0.0f);
                glVertex2f(width, height);
                glVertex2f(0.0f, height);
                glEnd();
                float left, top, right, bottom;
                if (point.m_elements[0] < minimum.m_elements[0] + 20.0) {
                    left = 0.0f;
                    right = 20.0;
                } else if (point.m_elements[0] >= maximum.m_elements[0] - 20.0) {
                    left = width - 20.0;
                    right = width;
                } else {
                    left = 20.0;
                    right = width - 20.0;
                }
                if (point.m_elements[1] < minimum.m_elements[1] + 20.0) {
                    top = 0.0f;
                    bottom = 20.0;
                } else if (point.m_elements[1] >= maximum.m_elements[1] - 20.0) {
                    top = height - 20.0;
                    bottom = height;
                } else {
                    top = 20.0;
                    bottom = height - 20.0;
                }
                glBegin(GL_QUADS);
                glColor4f(0.0f, 0.0f, 1.0f, 0.25f);
                glVertex2f(left, top);
                glVertex2f(right, top);
                glVertex2f(right, bottom);
                glVertex2f(left, bottom);
                glEnd();
            }
        }
    }

    // The title in the middle of the view: "Saving <title>..." or "Loading <title>..." while busy, with "*" when
    // modified; an outline pass, then the fill.
    if (m_workspace->m_toolModule->m_currentTool->getType() == Tool::TOOL_SHEET || m_busy) {
        std::wstring title = getTitle();
        if (title.empty())
            title = L"New design";
        if (m_busy) {
            if (m_saving)
                title = std::wstring(L"Saving ") + title + L"...";
            else
                title = std::wstring(L"Loading ") + title + L"...";
        }
        if (m_modified)
            title += L"*";
        plasma::Vector<2, float> center;
        center = m_panel->getViewSize();
        center.m_elements[0] *= 0.5;
        center.m_elements[1] *= 0.5;
        m_workspace->m_engine.getFontEngine()->renderText(
            L"tahomabd.ttf", title, 0.0f, 0.0f, center.m_elements[0], center.m_elements[1], 12.0f, 1.5f,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
            plasma::Vector<4, float>(0.3f, 0.3f, 0.3f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 5, -1.0f, true);
        m_workspace->m_engine.getFontEngine()->renderText(
            L"tahomabd.ttf", title, 0.0f, 0.0f, center.m_elements[0], center.m_elements[1], 12.0f, 0.0f,
            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.3f, 0.3f, 0.3f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 5, -1.0f, true);
    }

    // The status lines left of the top centre, each a 50-pixel icon with a label, 35 pixels apart (lower while the
    // current sheet fills the window).
    plasma::Vector<2, float> size;
    size = m_panel->getViewSize();
    size.m_elements[0] *= 0.5;
    float x = size.m_elements[0] - 100.0;
    float y = m_workspace->m_currentSheetOnly ? 60 : 5;

    // The view renders at a reduced scale: the export's pixel preview.
    if (m_viewWidget->m_surfaceScale < 1.0f) {
        plasma::Node *icon = m_workspace->findIcon(L"icon:preview");
        if (icon) {
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            plasma::Vector<2, float> minimum = icon->getMinimum(0);
            plasma::Vector<2, float> maximum = icon->getMaximum(0);
            plasma::Vector<2, float> diagonal = maximum - minimum;
            float scale =
                diagonal.m_elements[0] * diagonal.m_elements[0] + diagonal.m_elements[1] * diagonal.m_elements[1];
            scale = 50.0 / std::sqrt(scale);
            glTranslatef(x, y, 0.0f);
            glScalef(scale, scale, 1.0f);
            glTranslatef(-minimum.m_elements[0], -minimum.m_elements[1], 0.0f);
            icon->draw(0);
            glPopMatrix();
        }
        std::wstringstream text;
        text << L"Pixel preview: "
             << m_workspace->m_exportModule->m_antialiasing * m_workspace->m_exportModule->m_antialiasing << L"x AA, "
             << m_page.size.m_elements[2] << L" dpi" << std::endl;
        m_workspace->m_engine.getFontEngine()->renderText(
            L"tahomabd.ttf", text.str(), 0.0f, 0.0f, x + 50.0, y + 25.0, 12.0f, 1.5f,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
            plasma::Vector<4, float>(0.3f, 0.3f, 0.3f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f, true);
        m_workspace->m_engine.getFontEngine()->renderText(
            L"tahomabd.ttf", text.str(), 0.0f, 0.0f, x + 50.0, y + 25.0, 12.0f, 0.0f,
            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f, true);
        y = y + 35.0;
    }

    // Colour management: the CMYK preview with the profile's name.
    if (m_workspace->m_colorModule->getPanel()->isChecked(L"color:management", true)) {
        plasma::Node *icon = m_workspace->findIcon(L"icon:cmyk");
        if (icon) {
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            plasma::Vector<2, float> minimum = icon->getMinimum(0);
            plasma::Vector<2, float> maximum = icon->getMaximum(0);
            plasma::Vector<2, float> diagonal = maximum - minimum;
            float scale =
                diagonal.m_elements[0] * diagonal.m_elements[0] + diagonal.m_elements[1] * diagonal.m_elements[1];
            scale = 50.0 / std::sqrt(scale);
            glTranslatef(x, y, 0.0f);
            glScalef(scale, scale, 1.0f);
            glTranslatef(-minimum.m_elements[0], -minimum.m_elements[1], 0.0f);
            icon->draw(0);
            glPopMatrix();
        }
        std::wstringstream text;
        text << L"CMYK preview: "
             << m_workspace->m_colorManagementModule->getPanel()->getNode()->getText(L"color:profile", true);
        m_workspace->m_engine.getFontEngine()->renderText(
            L"tahomabd.ttf", text.str(), 0.0f, 0.0f, x + 50.0, y + 25.0, 12.0f, 1.5f,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
            plasma::Vector<4, float>(0.3f, 0.3f, 0.3f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f, true);
        m_workspace->m_engine.getFontEngine()->renderText(
            L"tahomabd.ttf", text.str(), 0.0f, 0.0f, x + 50.0, y + 25.0, 12.0f, 0.0f,
            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f, true);
        y = y + 35.0;
    }

    // Special colours: what the colouring applies to.
    if (m_workspace->m_colorModule->getPanel()->isChecked(L"color:special", true)) {
        plasma::Node *icon = m_workspace->findIcon(L"icon:special:color");
        if (icon) {
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            plasma::Vector<2, float> minimum = icon->getMinimum(0);
            plasma::Vector<2, float> maximum = icon->getMaximum(0);
            plasma::Vector<2, float> diagonal = maximum - minimum;
            float scale =
                diagonal.m_elements[0] * diagonal.m_elements[0] + diagonal.m_elements[1] * diagonal.m_elements[1];
            scale = 50.0 / std::sqrt(scale);
            glTranslatef(x, y, 0.0f);
            glScalef(scale, scale, 1.0f);
            glTranslatef(-minimum.m_elements[0], -minimum.m_elements[1], 0.0f);
            icon->draw(0);
            glPopMatrix();
        }
        std::wstring text = L"Coloring: ";
        if (m_workspace->m_specialColorModule->getPanel()->isChecked(L"special:color:drawing", true))
            text += L"Drawing";
        else
            text += L"Selection";
        m_workspace->m_engine.getFontEngine()->renderText(
            L"tahomabd.ttf", text, 0.0f, 0.0f, x + 50.0, y + 25.0, 12.0f, 1.5f,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
            plasma::Vector<4, float>(0.3f, 0.3f, 0.3f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f, true);
        m_workspace->m_engine.getFontEngine()->renderText(
            L"tahomabd.ttf", text, 0.0f, 0.0f, x + 50.0, y + 25.0, 12.0f, 0.0f,
            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f, true);
    }
}

std::wstring Sheet::getTitle()
{
    int slash = m_path.rfind(L'/');
    int backslash = m_path.rfind(L'\\');
    int separator = (std::max)(slash, backslash);
    std::wstring title = m_path.substr(separator + 1, m_path.size() - separator - 1);
    if (title.empty())
        title = L"New design";
    return title;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void Sheet::drawPageBorders()
{
    if (!m_workspace->m_formatModule->m_visible)
        return;

    // The panel's view size is read twice and not used.
    m_panel->viewWidth();
    m_panel->getViewHeight();

    // The panel's view rectangle in the sheet's coordinates.
    plasma::Vector<2, float> topLeft, bottomRight;
    const plasma::Matrix<float> &inverseMatrix = m_root->m_inverseMatrix;
    topLeft = inverseMatrix.transformPoint(m_panel->getContentPosition());
    plasma::Widget *panel = m_panel;
    bottomRight = m_root->m_inverseMatrix.transform(panel->getContentPosition() + panel->getViewSize());
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(m_root->m_matrix.m_elements);

    // Shades the view outside the page: above, left, right and below it.
    glBegin(GL_QUADS);
    glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
    for (int i = 0; i < 2; i++) {
        glVertex2f(topLeft.m_elements[0], topLeft.m_elements[1]);
        glVertex2f(bottomRight.m_elements[0], topLeft.m_elements[1]);
        glVertex2f(bottomRight.m_elements[0], 0.0f);
        glVertex2f(topLeft.m_elements[0], 0.0f);
        glVertex2f(topLeft.m_elements[0], 0.0f);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(0.0f, m_page.size.m_elements[1]);
        glVertex2f(topLeft.m_elements[0], m_page.size.m_elements[1]);
        glVertex2f(m_page.size.m_elements[0], 0.0f);
        glVertex2f(bottomRight.m_elements[0], 0.0f);
        glVertex2f(bottomRight.m_elements[0], m_page.size.m_elements[1]);
        glVertex2f(m_page.size.m_elements[0], m_page.size.m_elements[1]);
        glVertex2f(topLeft.m_elements[0], m_page.size.m_elements[1]);
        glVertex2f(bottomRight.m_elements[0], m_page.size.m_elements[1]);
        glVertex2f(bottomRight.m_elements[0], bottomRight.m_elements[1]);
        glVertex2f(topLeft.m_elements[0], bottomRight.m_elements[1]);
        glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
    }
    glEnd();
    glPopMatrix();
}

// Rounds ruler labels to thousandths, handles negatives by symmetry, and displays -0 as 0.
inline float roundToThousandths(float value)
{
    if (value < 0.0f) {
        value = -roundToThousandths(-value);
        return value == 0.0f ? 0.0 : value;
    }
    return (float)((double)(int)((double)value * 1000.0 + 0.5) * (double)0.001f);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void Sheet::drawGrid()
{
    glDisable(GL_LINE_SMOOTH);
    float width = m_panel->viewWidth();
    float height = m_panel->getViewHeight();
    float opacity = m_workspace->m_optionsModule->m_gridOpacity * (double)0.01f;

    // The grid: two passes, the spacing and its tenfold multiple, each a dark wide line under a light thin one.
    if (m_workspace->m_gridModule->m_enabled) {
        float step = m_root->m_matrix.m_elements[0] * m_gridSize;
        float size = m_gridSize;
        if (step > 4.0f) {
            float alpha = step * (0.5 * opacity) / 10.0;
            if (alpha > opacity)
                alpha = opacity;
            if (m_workspace->isPanning())
                alpha = alpha * (double)0.2f;
            float lightAlpha = alpha + alpha;
            for (int pass = 0; pass < 2; pass++) {
                int columns = (int)(width / step) + 2;
                int rows = (int)(height / step) + 2;
                plasma::Vector<2, float> origin =
                    m_root->transformation()->m_inverseMatrix * plasma::Vector<2, float>(0.0f, 0.0f);
                origin.m_elements[0] = (int)(origin.m_elements[0] / size) * size;
                origin.m_elements[1] = (int)(origin.m_elements[1] / size) * size;
                origin.transform(m_root->transformation()->m_matrix);
                glLineWidth(3.0f);
                glColor4f(0.0f, 0.0f, 0.0f, alpha);
                glBegin(GL_LINES);
                for (int i = 0; i < columns; i++) {
                    float x = i * step + origin.m_elements[0];
                    glVertex2f(x, 0.0f);
                    glVertex2f(x, height);
                }
                for (int i = 0; i < rows; i++) {
                    float y = i * step + origin.m_elements[1];
                    glVertex2f(0.0f, y);
                    glVertex2f(width, y);
                }
                glEnd();
                glLineWidth(1.0f);
                glColor4f(1.0f, 1.0f, 1.0f, lightAlpha);
                glBegin(GL_LINES);
                for (int i = 0; i < columns; i++) {
                    float x = i * step + origin.m_elements[0];
                    glVertex2f(x, 0.0f);
                    glVertex2f(x, height);
                }
                for (int i = 0; i < rows; i++) {
                    float y = i * step + origin.m_elements[1];
                    glVertex2f(0.0f, y);
                    glVertex2f(width, y);
                }
                glEnd();
                step = step * 10.0;
                size = size * 10.0;
            }
        }
    }

    // The rulers along the top and left edges, with a tick and a label in the page's unit every 100 to 200 pixels.
    if (m_workspace->m_gridModule->m_enabled) {
        glDisable(GL_TEXTURE_2D);
        glPushMatrix();
        float step = m_root->m_matrix.m_elements[0] * m_gridSize;
        float scale = 200.0 / step;
        scale = log10(scale);
        scale = pow(10.0, (double)(int)scale);
        scale = (int)scale;
        step = step * scale;
        float size = scale * m_gridSize;
        if (step < 100.0) {
            step = step * 2.0;
            size = size * 2.0;
        }
        int columns = (int)(width / step) + 2;
        int rows = (int)(height / step) + 2;
        plasma::Vector<2, float> origin =
            m_root->transformation()->m_inverseMatrix * plasma::Vector<2, float>(0.0f, 0.0f);
        origin.m_elements[0] = (int)(origin.m_elements[0] / size) * size;
        origin.m_elements[1] = (int)(origin.m_elements[1] / size) * size;
        origin.transform(m_root->transformation()->m_matrix);

        // The left ruler clears the procedural shape or stamp panel when the current sheet fills the window.
        int left = 100;
        int top = 100;
        if (m_workspace->m_currentSheetOnly) {
            top = 150;
            left = 150;
            if (m_workspace->m_proceduralShapeModule->getPanel()->getNode()->m_display->m_visibility.value())
                left = (int)(m_workspace->m_guiLeftNode->transformation()->m_matrix.m_elements[0] *
                             m_workspace->m_proceduralShapeModule->getPanel()->viewWidth()) +
                       120;
            else if (m_workspace->m_stampModule->getPanel()->getNode()->m_display->m_visibility.value())
                left = (int)(m_workspace->m_guiLeftNode->transformation()->m_matrix.m_elements[0] *
                             m_workspace->m_stampModule->getPanel()->viewWidth()) +
                       120;
        }
        glBegin(GL_QUADS);
        glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(width, 0.0f);
        glVertex2f(width, top + 5);
        glVertex2f(left + 5, top + 5);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(left + 5, top + 5);
        glVertex2f(left + 5, height);
        glVertex2f(0.0f, height);
        glEnd();
        glLineWidth(1.0f);
        glBegin(GL_LINE_STRIP);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2f(width, top);
        glVertex2f(left, top);
        glVertex2f(left, height - 10.0);
        glEnd();
        glBegin(GL_LINES);
        for (int i = 0; i < columns; i++) {
            float x = i * step + origin.m_elements[0];
            if (left <= x && width > x) {
                glVertex2f(x, top - 5);
                glVertex2f(x, top);
            }
        }
        for (int i = 0; i < rows; i++) {
            float y = i * step + origin.m_elements[1];
            if (top <= y && height > y) {
                glVertex2f(left - 5, y);
                glVertex2f(left, y);
            }
        }
        glEnd();
        plasma::ScalableFont *font = m_workspace->m_engine.getFontEngine()->getFont(L"tahoma.ttf");
        if (font) {
            std::wstring unit;
            switch (m_page.unit) {
            case UNIT_PX:
                unit = L"px";
                break;
            case UNIT_IN:
                unit = L"in";
                break;
            case UNIT_FT:
                unit = L"ft";
                break;
            case UNIT_MM:
                unit = L"mm";
                break;
            case UNIT_CM:
                unit = L"cm";
                break;
            case UNIT_M:
                unit = L"m";
                break;
            case UNIT_PT:
                unit = L"pt";
                break;
            case UNIT_PC:
                unit = L"pc";
                break;
            }
            for (int i = 0; i < columns; i++) {
                float x = i * step + origin.m_elements[0];
                if (left <= x && width > x) {
                    std::wstringstream text;
                    float value = (x - m_root->transformation()->m_translation.value().m_elements[0]) /
                                  m_root->transformation()->m_deformation.value().m_elements[0];
                    text << roundToThousandths(plasma::fromInternalUnits(m_page.unit, value)) << L" " << unit;
                    m_workspace->m_engine.getFontEngine()->lock();
                    font->renderText(text.str(), x, top - 10, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                     plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                     plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                     plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
                    m_workspace->m_engine.getFontEngine()->unlock();
                }
            }
            for (int i = 0; i < rows; i++) {
                float y = i * step + origin.m_elements[1];
                if (top <= y && height > y) {
                    std::wstringstream text;
                    float value = (y - m_root->transformation()->m_translation.value().m_elements[1]) /
                                  m_root->transformation()->m_deformation.value().m_elements[5];
                    text << roundToThousandths(plasma::fromInternalUnits(m_page.unit, value)) << L" " << unit;
                    m_workspace->m_engine.getFontEngine()->lock();
                    font->renderText(text.str(), left - 35, y, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                     plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                     plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                     plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
                    m_workspace->m_engine.getFontEngine()->unlock();
                }
            }
        }
        glPopMatrix();
        glDisable(GL_TEXTURE_2D);
    }
}

void Sheet::setPath(const std::wstring &path)
{
    m_path = path;
}

void Sheet::applyBackgroundColor()
{
    m_viewWidget->updateSurface();
    if (m_viewWidget->m_surface)
        m_viewWidget->m_surface->m_clearColor = plasma::Vector<4, float>(
            m_page.color.m_elements[0], m_page.color.m_elements[1], m_page.color.m_elements[2], 1.0f);
}

void Sheet::waitForThread()
{
    if (m_thread) {
        WaitForSingleObject(m_thread, INFINITE);
        m_thread = 0;
    }
}
