#include "NodeTreeWidget.h"
#include "Workspace.h"
#include "DesignSheet.h"
#include "Operations.h"
#include "ToolModule.h"
#include "plasma/Node.h"
#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
#include <GL/glew.h>

using plasma::Matrix;
using plasma::Node;
using plasma::Vector;

static const int MaximumThumbnailShapeCount = 100;

NodeTreeWidget::NodeTreeWidget(plasma::Engine *engine, Workspace *workspace, Node *node)
    : plasma::Widget(engine, node, L"NodeTreeWidget"),
      m_workspace(workspace)
{
    m_lockedIcon = workspace->m_engine.findNode(L"icon:locked");
    m_unlockedIcon = workspace->m_engine.findNode(L"icon:unlocked");
    m_visibleIcon = workspace->m_engine.findNode(L"icon:visible2");
    m_invisibleIcon = workspace->m_engine.findNode(L"icon:invisible");
    m_openIcon = workspace->m_engine.findNode(L"icon:expander:open");
    m_closedIcon = workspace->m_engine.findNode(L"icon:expander:closed");
    m_disabledIcon = workspace->m_engine.findNode(L"icon:expander:disabled");
}

NodeTreeWidget::~NodeTreeWidget() {}

void NodeTreeWidget::rebuild()
{
    m_contentWidth = 0.0f;
    m_currentNodePosition = Vector<2, int>(0, 0);
    if (m_workspace->getOpenDesignSheet()) {
        if (!m_workspace->getOpenDesignSheet()->m_busy)
            walkRows(0, false, &NodeTreeWidget::measureRow);
    }

    // The code leaves m_row untouched when there is no open, expanded sheet.
    resize(Vector<2, float>(m_contentWidth, (float)(ROW_HEIGHT * m_row)));
    if (getParentWidget()) {
        Vector<2, float> extent = getParentWidget()->getPlacementMatrix().transformDirection(getViewSize());
        getParentWidget()->resize(extent);
        getParentWidget()->clampToParent();
        getParentWidget()->updateScrollSliders();
    }
    m_firstVisibleRow = 0;
    m_lastVisibleRow = 0;
    if (getParentWidget()) {
        if (getParentWidget()->getParentWidget()) {
            // The conversions are unchecked for NaN and out-of-range values.
            m_firstVisibleRow = (int)(-getParentWidget()->getOffsetInParent()[1] / (double)ROW_HEIGHT);
            m_lastVisibleRow =
                (int)getParentWidget()->getParentWidget()->getContentHeight() / ROW_HEIGHT + m_firstVisibleRow + 1;
        }
    }
    m_changed = true;
}

int countThumbnailShapes(Node *node)
{
    if (!node->isShown() || !plasma::Widget::isDisplayVisible(node->m_display))
        return 0;
    int count = 0;
    if (node->m_shape)
        count = 1;
    std::list<Node *>::iterator it = node->m_children.begin();
    while (it != node->m_children.end()) {
        int childCount = countThumbnailShapes(*it);
        ++it;
        count += childCount;
    }
    return count;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void NodeTreeWidget::renderLabel(const std::wstring &name, const Vector<4, float> &fill, int depth, int row,
                                 const Vector<4, float> &outline, bool bold)
{
    float x = (double)depth * 20.0f + 12.0f + 50.0f;
    float rowY = (float)(ROW_HEIGHT * row);
    glPushMatrix();
    glTranslatef(x, rowY + (double)ROW_HEIGHT - 3.0f, 0.0f);
    m_engine->getFontEngine()->renderText(bold ? L"tahomabd.ttf" : L"tahoma.ttf", name, 0.0f, 0.0f, 0.0f, 0.0f, 11.0f,
                                          1.5f, Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, outline.m_elements,
                                          Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f, true);
    m_engine->getFontEngine()->renderText(bold ? L"tahomabd.ttf" : L"tahoma.ttf", name, 0.0f, 0.0f, 0.0f, 0.0f, 11.0f,
                                          0.0f, fill.m_elements, Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f, true);
    plasma::ScalableFont *font = m_engine->getFontEngine()->getFont(bold ? L"tahomabd.ttf" : L"tahoma.ttf");
    if (font) {
        Matrix<float> matrix;
        m_engine->getModelViewMatrix(matrix);
        m_engine->getFontEngine()->lock();
        Vector<2, float> minimum, maximum;
        font->measure(name, matrix, 11.0f, 1.5f, 0.0f, 0.0f, 0.0f, minimum, maximum, 0, true);
        m_engine->getFontEngine()->unlock();
        if ((double)maximum.m_elements[0] > m_contentWidth)
            m_contentWidth = maximum.m_elements[0];
    }
    glPopMatrix();
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
bool NodeTreeWidget::measureRow(int depth, int row, Node *node)
{
    if (node) {
        float x = (double)depth * 20.0f + 20.0f + 50.0f;
        float y = (float)(ROW_HEIGHT * row);
        if (m_workspace->getOpenDesignSheet()) {
            if (m_workspace->getOpenDesignSheet()->m_currentNode == node) {
                // The conversions are unchecked for NaN and out-of-range values.
                m_currentNodePosition = Vector<2, int>((int)x, (int)y);
            }
        }
        Matrix<float> matrix;
        matrix.identity();
        y = y + (double)20.0f + 4.0f;

        // Expands the translation's matrix product here.
        matrix.m_elements[12] = matrix.m_elements[0] * x + matrix.m_elements[4] * y + matrix.m_elements[12];
        matrix.m_elements[13] = matrix.m_elements[1] * x + matrix.m_elements[5] * y + matrix.m_elements[13];
        matrix.m_elements[14] = matrix.m_elements[2] * x + matrix.m_elements[6] * y + matrix.m_elements[14];
        matrix.m_elements[15] = matrix.m_elements[3] * x + matrix.m_elements[7] * y + matrix.m_elements[15];
        plasma::ScalableFont *font = m_engine->getFontEngine()->getFont(L"tahomabd.ttf");
        if (!font)
            return false;
        m_engine->getFontEngine()->lock();
        Vector<2, float> minimum, maximum;
        font->measure(node->getName(), matrix, 11.0f, 1.5f, 0.0f, 0.0f, 0.0f, minimum, maximum, 0, true);
        m_engine->getFontEngine()->unlock();
        maximum.m_elements[0] = x + (double)ROW_HEIGHT + maximum.m_elements[0];
        if ((double)maximum.m_elements[0] > m_contentWidth)
            m_contentWidth = maximum.m_elements[0];
    }
    return true;
}

void NodeTreeWidget::deselectSheetNode(NodeInterface *node)
{
    if (node->m_selected) {
        node->setSelected(false);
        node->m_sheet->addOperation(new NodeSelectionOperation(node->m_sheet, "deselect node", node, false));
    }
}

void NodeTreeWidget::pointerPressed()
{
    m_engine->setPointerCapture(m_node);
    if (m_workspace->getOpenDesignSheet()) {
        m_doubleClick = false;
        walkRows(0, true, &NodeTreeWidget::clickRow);
        m_workspace->getOpenDesignSheet()->commit();
        rebuild();
    }
}

void NodeTreeWidget::pointerReleased()
{
    m_engine->releasePointerCapture();
}

void NodeTreeWidget::pointerDoubleClicked()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_doubleClick = true;
        walkRows(0, true, &NodeTreeWidget::clickRow);
        m_changed = true;
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->refreshTimeline();
    }
}

void NodeTreeWidget::pointerMoved()
{
    walkRows(0, true, &NodeTreeWidget::hoverRow);
}

Vector<2, float> NodeTreeWidget::getMaximum()
{
    return getViewSize();
}

bool NodeTreeWidget::hasBounds()
{
    return true;
}

bool NodeTreeWidget::contains(const Vector<2, float> &point)
{
    return true;
}

void NodeTreeWidget::draw()
{
    if (m_workspace->getOpenDesignSheet()) {
        if (!m_workspace->getOpenDesignSheet()->m_busy) {
            glDisable(GL_LINE_SMOOTH);
            glEnable(GL_TEXTURE_2D);
            m_contentWidth = 0.0f;
            m_firstVisibleRow = 0;
            m_lastVisibleRow = 0;
            if (getParentWidget()) {
                if (getParentWidget()->getParentWidget()) {
                    m_firstVisibleRow =
                        (int)(-getParentWidget()->getOffsetInParent().m_elements[1] / (double)ROW_HEIGHT);
                    m_lastVisibleRow = (int)getParentWidget()->getParentWidget()->getContentHeight() / ROW_HEIGHT +
                                       m_firstVisibleRow + 1;
                }
            }
            m_workspace->m_engine.m_renderFlags &= ~(1 << plasma::RENDER_BLUR);
            walkRows(0, true, &NodeTreeWidget::renderRow);
            glLineWidth(1.0f);
            glDisable(GL_TEXTURE_2D);
            m_workspace->m_engine.m_renderFlags |= (1 << plasma::RENDER_BLUR);
        }
    }
}

bool NodeTreeWidget::walkRows(Node *node, bool visibleOnly, RowFunction function)
{
    if (!node) {
        m_depth = 0;
        m_row = 0;
        if (m_workspace->getOpenDesignSheet()) {
            if (m_workspace->getOpenDesignSheet()->m_root->isShown()) {
                // The code walks siblings backwards, starting before end.
                std::list<Node *>::iterator it = m_workspace->getOpenDesignSheet()->m_root->m_children.end();
                while (it != m_workspace->getOpenDesignSheet()->m_root->m_children.begin()) {
                    --it;
                    if ((*it)->isShown() && !(*it)->isInternal() && !walkRows(*it, visibleOnly, function))
                        return false;
                }
                return true;
            }
        }
        return false;
    }
    float y = (double)(ROW_HEIGHT * m_row) + getParentWidget()->getOffsetInParent().m_elements[1];
    NodeInterface *sheetNode = m_workspace->getOpenDesignSheet()->findSheetNode(node);
    sheetNode->m_layerRow = m_row;
    sheetNode->m_layerEndRow = m_row;
    sheetNode->m_layerDepth = m_depth;
    if (!visibleOnly ||
        (y + (double)ROW_HEIGHT >= 0.0f && getParentWidget()->getParentWidget()->getContentHeight() > (double)y)) {
        if (!(this->*function)(m_depth, m_row, node))
            return false;
    }
    ++m_row;
    if (sheetNode->m_expanded) {
        ++m_depth;
        std::list<Node *>::iterator it = node->m_children.end();
        while (it != node->m_children.begin()) {
            sheetNode->m_layerEndRow = m_row;
            --it;
            if ((*it)->isShown() && !(*it)->isInternal() && !walkRows(*it, visibleOnly, function))
                return false;
        }
        --m_depth;
    }
    return true;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
bool NodeTreeWidget::renderThumbnail(int depth, int row, Node *node)
{
    if (row >= m_firstVisibleRow && row <= m_lastVisibleRow &&
        countThumbnailShapes(node) <= MaximumThumbnailShapeCount) {
        glPushMatrix();
        glTranslatef((double)depth * 20.0f + 8.0f + 50.0f, (float)(ROW_HEIGHT * row), 0.0f);

        // The subtree scaled to fit a 32 x 32 square, centred in it.
        Vector<2, float> minimum = node->getMinimum(0);
        Vector<2, float> maximum = node->getMaximum(0);
        double width = maximum.m_elements[0] - minimum.m_elements[0];
        double height = maximum.m_elements[1] - minimum.m_elements[1];
        float scale = 32.0f / (width > height ? width : height);
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(32.0f, 0.0f);
        glVertex2f(32.0f, 32.0f);
        glVertex2f(0.0f, 32.0f);
        glEnd();
        glPushMatrix();
        glTranslatef(16.0f, 16.0f, 0.0f);
        glScalef(scale, scale, 1.0f);
        glTranslatef(-0.5f * (minimum.m_elements[0] + maximum.m_elements[0]),
                     (minimum.m_elements[1] + maximum.m_elements[1]) * -0.5f, 0.0f);
        bool saved = m_engine->m_unused1F4;
        m_engine->m_unused1F4 = false;
        node->draw(2);
        m_engine->m_unused1F4 = saved;
        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_TEXTURE_2D);
        glPopMatrix();
        glLineWidth(1.0f);
        glColor3f(0.6f, 0.6f, 0.6f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(33.0f, 0.0f);
        glVertex2f(33.0f, 33.0f);
        glVertex2f(0.0f, 33.0f);
        glEnd();
        glPopMatrix();
    }
    return true;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
bool NodeTreeWidget::renderRow(int depth, int row, Node *node)
{
    if (node) {
        Vector<4, float> outline(0.0f, 0.0f, 0.0f, 1.0f);
        Vector<4, float> fill;
        if (m_workspace->getOpenDesignSheet()->findSheetNode(node)->m_selected) {
            if (m_workspace->getOpenDesignSheet()->m_currentNode == node)
                fill = Vector<4, float>(0.8f, 0.5f, 1.0f, 1.0f);
            else
                fill = Vector<4, float>(0.5f, 1.0f, 0.5f, 1.0f);
        } else {
            if (m_workspace->getOpenDesignSheet()->m_editRoot == node)
                fill = Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
            else {
                fill = Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
                outline = Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
        if (node->isLocked()) {
            fill.m_elements[3] = 0.75f;
            outline.m_elements[3] = 0.75f;
        }

        // The lock, visibility and expander icons, each centred on its column.
        Node *lockIcon = node->isLocked() ? m_lockedIcon : m_unlockedIcon;
        if (lockIcon) {
            Vector<2, float> center = (lockIcon->getMinimum(0) + lockIcon->getMaximum(0)) * 0.5f;
            glPushMatrix();
            glTranslatef(20.0f * depth + (7.0f - center[0]), (float)(ROW_HEIGHT * row) + 20.0f - center[1], 0.0f);
            lockIcon->draw(0);
            glPopMatrix();
        }
        Node *visibilityIcon = isDisplayVisible(node->m_display) ? m_visibleIcon : m_invisibleIcon;
        if (visibilityIcon) {
            Vector<2, float> center = (visibilityIcon->getMinimum(0) + visibilityIcon->getMaximum(0)) * 0.5f;
            glPushMatrix();
            glTranslatef(20.0f * depth + (25.0f - center[0]), (float)(ROW_HEIGHT * row) + 20.0f - center[1], 0.0f);
            visibilityIcon->draw(0);
            glPopMatrix();
        }
        Node *expanderIcon;
        if (node->countVisibleChildren() > 0)
            expanderIcon =
                m_workspace->getOpenDesignSheet()->findSheetNode(node)->m_expanded ? m_openIcon : m_closedIcon;
        else
            expanderIcon = m_disabledIcon;

        // The visibility icon's pointer also guards drawing the expander icon.
        if (visibilityIcon) {
            Vector<2, float> center = (expanderIcon->getMinimum(0) + expanderIcon->getMaximum(0)) * 0.5f;
            glPushMatrix();
            glTranslatef(20.0f * depth + (45.0f - center[0]), (float)(ROW_HEIGHT * row) + 20.0f - center[1], 0.0f);
            expanderIcon->draw(0);
            glPopMatrix();
        }
        renderThumbnail(depth, row, node);
        renderLabel(node->getName(), fill, depth, row, outline, m_workspace->getOpenDesignSheet()->m_editRoot == node);
    }
    return true;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. The row's columns:
// the lock, visibility and expander icons, then the label.
bool NodeTreeWidget::clickRow(int depth, int row, Node *node)
{
    Vector<2, float> pointer = m_engine->m_pointer;
    Vector<2, float> point = m_node->m_inverseMatrix.transform(pointer);

    // A NaN pointer counts as inside the row.
    if (!(point.m_elements[1] < (double)(ROW_HEIGHT * row) ||
          point.m_elements[1] >= (double)(ROW_HEIGHT * row + ROW_HEIGHT))) {
        double x = point.m_elements[0];
        if ((double)(20 * depth + 15) > x) {
            if (m_doubleClick) {
                m_workspace->m_toolModule->m_nodeTool.setLockedRecursively(node, node->isLocked());
            } else {
                m_workspace->getOpenDesignSheet()->findSheetNode(node)->saveState();
                bool lock = !node->isLocked();
                if (lock)
                    node->m_flags |= (1 << plasma::NODE_LOCKED);
                else
                    node->m_flags &= ~(1 << plasma::NODE_LOCKED);
                if (node->m_flags & (1 << plasma::NODE_LOCKED)) {
                    if (m_workspace->getOpenDesignSheet()->findSheetNode(node)->m_selected) {
                        m_workspace->getOpenDesignSheet()->findSheetNode(node)->setSelected(false);
                        m_workspace->getOpenDesignSheet()->addOperation(
                            new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "deselect node",
                                                       m_workspace->getOpenDesignSheet()->findSheetNode(node), false));
                    }
                    if (node == m_workspace->getOpenDesignSheet()->m_currentNode)
                        m_workspace->getOpenDesignSheet()->updateCurrentNode(0);
                }
                m_workspace->getOpenDesignSheet()->findSheetNode(node)->recordModification();
            }
            m_workspace->getOpenDesignSheet()->commit();
            m_workspace->redraw();
        } else if ((double)(20 * depth + 35) > x) {
            m_workspace->getOpenDesignSheet()->findSheetNode(node)->saveState();
            setDisplayVisibility(node->m_display, !isDisplayVisible(node->m_display));
            m_workspace->getOpenDesignSheet()->findSheetNode(node)->recordModification();
            m_workspace->getOpenDesignSheet()->commit();
            m_workspace->markDesignSheetChanged();
            m_workspace->redraw();
        } else if ((double)(20 * depth + 55) > x) {
            m_workspace->getOpenDesignSheet()->findSheetNode(node)->m_expanded =
                !m_workspace->getOpenDesignSheet()->findSheetNode(node)->m_expanded;
        } else {
            if (!(node->m_flags & (1 << plasma::NODE_LOCKED))) {
                if (m_workspace->pickRequestedNode(node))
                    return false;
                if (!m_workspace->m_engine.isKeyDown(VK_SHIFT) && !m_workspace->m_engine.isKeyDown(VK_CONTROL))
                    m_workspace->forEachUnlockedSheetNode(&NodeTreeWidget::deselectSheetNode);
                NodeInterface *sheetNode = m_workspace->getOpenDesignSheet()->findSheetNode(node);
                if (m_workspace->m_engine.isKeyDown(VK_CONTROL)) {
                    deselectSheetNode(sheetNode);
                    m_workspace->getOpenDesignSheet()->updateCurrentNode(0);
                    m_workspace->updateModules();
                } else {
                    if (!sheetNode->m_selected) {
                        sheetNode->saveState();
                        sheetNode->setSelected(true);
                        m_workspace->getOpenDesignSheet()->addOperation(
                            new NodeSelectionOperation(m_workspace->getOpenDesignSheet(), "select node",
                                                       m_workspace->getOpenDesignSheet()->findSheetNode(node), true));
                    }
                    m_workspace->getOpenDesignSheet()->m_currentNode = node;
                    m_workspace->updateModules();
                }
            }
            if (m_doubleClick) {
                Node *root = node;
                if (m_workspace->getOpenDesignSheet()->m_editRoot == node)
                    root = m_workspace->getOpenDesignSheet()->m_root;
                m_workspace->getOpenDesignSheet()->setEditRoot(root, true);
                m_workspace->getOpenDesignSheet()->commit();
            }
        }
    }
    return true;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
bool NodeTreeWidget::hoverRow(int depth, int row, Node *node)
{
    Vector<2, float> pointer = m_engine->m_pointer;
    Vector<2, float> point = m_node->m_inverseMatrix.transform(pointer);
    if (point.m_elements[1] < (double)(ROW_HEIGHT * row))
        return true;
    if (point.m_elements[1] >= (double)(ROW_HEIGHT * row + ROW_HEIGHT))
        return true;
    double x = point.m_elements[0];
    if ((double)(20 * depth + 15) > x || (double)(20 * depth + 35) > x || (double)(20 * depth + 55) > x ||
        (node->m_flags & (1 << plasma::NODE_LOCKED)))
        return true;
    m_workspace->m_hoveredNode = node;
    return false;
}
