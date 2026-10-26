#include "PaletteWidget.h"
#include "PaletteModule.h"
#include "Workspace.h"
#include "ColorModule.h"
#include "plasma/Node.h"
#include <GL/glew.h>

static const int DefaultPaletteCellExtent = 15;

PaletteWidget::PaletteWidget(plasma::Engine *engine, Workspace *workspace, plasma::Node *node)
    : plasma::Widget(engine, node, L"palettewidget"),
      m_workspace(workspace),
      m_cellWidth(DefaultPaletteCellExtent),
      m_cellHeight(DefaultPaletteCellExtent)
{}

PaletteWidget::~PaletteWidget() {}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void PaletteWidget::draw()
{
    glDisable(GL_DEPTH_TEST);
    __glewUseProgram(0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    int width = (int)getContentWidth();
    int height = (int)getContentHeight();
    int rows = m_workspace->m_paletteModule->m_colors.size() / COLUMNS;
    if (m_workspace->m_paletteModule->m_colors.size() % COLUMNS)
        ++rows;
    glBegin(GL_QUADS);
    for (int i = 0; i < (int)m_workspace->m_paletteModule->m_colors.size(); ++i) {
        int row = i / COLUMNS;
        int column = i % COLUMNS;
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2i(column * m_cellWidth, row * m_cellHeight);
        glVertex2i((column + 1) * m_cellWidth, row * m_cellHeight);
        glVertex2i((column + 1) * m_cellWidth, (row + 1) * m_cellHeight);
        glVertex2i(column * m_cellWidth, (row + 1) * m_cellHeight);
        if (i < (int)m_workspace->m_paletteModule->m_colors.size()) {
            glColor4fv(m_workspace->m_paletteModule->m_colors[i].m_elements);
            glVertex2i(column * m_cellWidth, row * m_cellHeight);
            glVertex2i((column + 1) * m_cellWidth, row * m_cellHeight);
            glVertex2i((column + 1) * m_cellWidth, (row + 1) * m_cellHeight);
            glVertex2i(column * m_cellWidth, (row + 1) * m_cellHeight);
        }
    }
    glEnd();
    glLineWidth(1.0f);
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    for (int i = 1; i < rows; ++i) {
        glVertex2f(0.0f, (float)(i * m_cellHeight));
        glVertex2f((float)width, (float)(i * m_cellHeight));
    }
    int remainder = m_workspace->m_paletteModule->m_colors.size() % COLUMNS;
    for (int i = 1; i < COLUMNS; ++i) {
        glVertex2f((float)(i * m_cellWidth), 0.0f);
        if (remainder <= 0 || i <= remainder)
            glVertex2f((float)(i * m_cellWidth), (float)height);
        else
            glVertex2f((float)(i * m_cellWidth), (float)(height - m_cellHeight));
    }
    glEnd();
    if (m_workspace->m_paletteModule->m_selection >= 0 &&
        m_workspace->m_paletteModule->m_selection < (int)m_workspace->m_paletteModule->m_colors.size()) {
        float x = (m_workspace->m_paletteModule->m_selection % COLUMNS + (double)0.5f) * m_cellWidth;
        float y = (m_workspace->m_paletteModule->m_selection / COLUMNS + (double)0.5f) * m_cellHeight;
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
        glVertex2f(x, y);
        glEnd();
        glPointSize(3.0f);
        glBegin(GL_POINTS);
        glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
        glVertex2f(x, y);
        glEnd();
    }
}

bool PaletteWidget::contains(const plasma::Vector<2, float> &point)
{
    return true;
}

void PaletteWidget::colorChanged()
{
    m_changed = true;
}

void PaletteWidget::refresh()
{
    int cellHeight = m_cellHeight;
    int height = cellHeight * (m_workspace->m_paletteModule->m_colors.size() / COLUMNS);
    if (m_workspace->m_paletteModule->m_colors.size() % COLUMNS)
        height += cellHeight;
    plasma::Vector<2, float> size((float)(COLUMNS * m_cellWidth), (float)height);
    resize(getParentWidget()->getPlacementMatrix().transformDirection(size));
    getParentWidget()->resize(getViewSize());
    getParentWidget()->clampToParent();
    getParentWidget()->updateScrollSliders();
}

plasma::Vector<2, float> PaletteWidget::getMaximum()
{
    return getViewSize();
}

bool PaletteWidget::hasBounds()
{
    return true;
}

void PaletteWidget::pointerPressed()
{
    plasma::Vector<2, float> pointer = m_engine->m_pointer;
    plasma::Vector<2, float> point = m_node->m_inverseMatrix.transform(pointer);

    // The float-to-int conversion is unchecked.
    m_workspace->m_paletteModule->m_selection =
        (int)point.m_elements[0] / m_cellWidth + COLUMNS * ((int)point.m_elements[1] / m_cellWidth);
    if (m_workspace->m_paletteModule->m_selection >= (int)m_workspace->m_paletteModule->m_colors.size())
        m_workspace->m_paletteModule->m_selection = m_workspace->m_paletteModule->m_colors.size() - 1;
    if (m_workspace->m_engine.isKeyDown(VK_SHIFT)) {
        m_workspace->m_paletteModule->setColor(m_workspace->m_colorModule->getColor());
    } else {
        if (!m_workspace->m_engine.isKeyDown(VK_CONTROL)) {
            ColorModule *colorModule = m_workspace->m_colorModule;
            colorModule->setColor(m_workspace->m_paletteModule->getColor(), true, true, false);
        }
        m_engine->setPointerCapture(m_node);
    }
    m_changed = true;
}

void PaletteWidget::pointerReleased()
{
    if (m_engine->m_pointerCapture == m_node) {
        m_engine->releasePointerCapture();
        plasma::Vector<2, float> pointer = m_engine->m_pointer;

        // Transforms the point with perspective division, adding m[7] * y before m[3] * x when computing w.
        const float *m = m_node->m_inverseMatrix.m_elements;
        float w = m[7] * pointer.m_elements[1] + m[3] * pointer.m_elements[0] + m[15];
        plasma::Vector<2, float> point =
            plasma::Vector<2, float>(m[4] * pointer.m_elements[1] + pointer.m_elements[0] * m[0] + m[12],
                                     m[1] * pointer.m_elements[0] + m[5] * pointer.m_elements[1] + m[13]) *
            (1 / w);
        int column = (int)point.m_elements[0] / m_cellWidth;
        int row = (int)point.m_elements[1] / m_cellWidth;

        // A NaN view extent passes these ordered tests.
        if (column < 0 || row < 0 || (float)column >= viewWidth() || (float)row >= getViewHeight())
            return;
        int previous = m_workspace->m_paletteModule->m_selection;
        if (previous < 0 || previous >= (int)m_workspace->m_paletteModule->m_colors.size())
            return;
        m_workspace->m_paletteModule->m_selection = row + column + (COLUMNS - 1) * row;
        if (m_workspace->m_paletteModule->m_selection >= (int)m_workspace->m_paletteModule->m_colors.size())
            m_workspace->m_paletteModule->m_selection = m_workspace->m_paletteModule->m_colors.size() - 1;
        plasma::Vector<4, float> color = m_workspace->m_paletteModule->m_colors[previous];
        m_workspace->m_paletteModule->m_colors[previous] =
            m_workspace->m_paletteModule->m_colors[m_workspace->m_paletteModule->m_selection];
        m_workspace->m_paletteModule->m_colors[m_workspace->m_paletteModule->m_selection] = color;
        m_changed = true;
    }
}

void PaletteWidget::pointerDoubleClicked()
{
    plasma::Vector<2, float> pointer = m_engine->m_pointer;
    plasma::Vector<2, float> point = m_node->m_inverseMatrix.transform(pointer);
    m_workspace->getPaletteModule()->m_selection =
        (int)point.m_elements[0] / m_cellWidth + COLUMNS * ((int)point.m_elements[1] / m_cellWidth);
    ColorModule *colorModule = m_workspace->getColorModule();
    colorModule->setColor(m_workspace->getPaletteModule()->getColor(), true, true, false);
    m_changed = true;
}
