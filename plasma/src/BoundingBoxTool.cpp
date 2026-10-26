#include "BoundingBoxTool.h"
#include <windows.h>
#include <cmath>
#include <sstream>
#include "DesignSheet.h"
#include "FormatModule.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/ListWidget.h"
#include "plasma/Matrix.h"
#include "plasma/Node.h"
#include <GL/glew.h>

using plasma::Matrix;
using plasma::Vector;

BoundingBoxTool::BoundingBoxTool(int type)
    : Tool(type),
      m_dragHandle(NoHandle),
      m_selectionMode(SELECTION_NONE)
{}

void BoundingBoxTool::updateHandles()
{
    m_handles[0] = Vector<2, float>(m_boxMin.m_elements[0], m_boxMin.m_elements[1]);
    m_handles[1] = Vector<2, float>(m_boxMax.m_elements[0], m_boxMin.m_elements[1]);
    m_handles[2] = Vector<2, float>(m_boxMax.m_elements[0], m_boxMax.m_elements[1]);
    m_handles[3] = Vector<2, float>(m_boxMin.m_elements[0], m_boxMax.m_elements[1]);
    m_handles[4] = Vector<2, float>((m_boxMax.m_elements[0] + m_boxMin.m_elements[0]) * 0.5, m_boxMin.m_elements[1]);
    m_handles[5] = Vector<2, float>(m_boxMin.m_elements[0], (m_boxMax.m_elements[1] + m_boxMin.m_elements[1]) * 0.5);
    m_handles[6] = Vector<2, float>((m_boxMax.m_elements[0] + m_boxMin.m_elements[0]) * 0.5, m_boxMax.m_elements[1]);
    m_handles[7] = Vector<2, float>(m_boxMax.m_elements[0], 0.5 * (m_boxMax.m_elements[1] + m_boxMin.m_elements[1]));
}

int BoundingBoxTool::findHandle()
{
    if (!m_workspace->getOpenSheet())
        return NoHandle;
    Vector<2, float> size = m_boxMax - m_boxMin;
    Vector<2, float> screenSize = m_workspace->getOpenSheet()->m_root->m_matrix.transformDirection(size);
    for (int i = 0; i < 8; i++) {
        Vector<2, float> handle = m_workspace->getOpenSheet()->m_root->m_matrix.transform(m_handles[i]);

        // The handles grab within 5 pixels; the edge midpoints along their whole edge.
        float rangeX = 5.0f;
        float rangeY = 5.0f;
        if (i == 4 || i == 6)
            rangeX = screenSize.m_elements[0] * 0.5;
        if (i == 5 || i == 7)
            rangeY = 0.5 * screenSize.m_elements[1];
        if (fabs(m_workspace->m_sceneMousePosition.m_elements[0] - handle.m_elements[0]) < rangeX &&
            fabs(m_workspace->m_sceneMousePosition.m_elements[1] - handle.m_elements[1]) < rangeY)
            return i;
    }
    return NoHandle;
}

bool BoundingBoxTool::handlesShown()
{
    if (!m_workspace->getOpenSheet())
        return false;
    if (m_workspace->m_setPivotRequested || m_workspace->m_chooseParentRequested ||
        m_workspace->m_sendBehindRequested || m_workspace->m_sendInFrontOfRequested)
        return false;
    Vector<2, float> boxMin = m_workspace->getOpenSheet()->getRoot()->m_matrix * m_boxMin;
    Vector<2, float> boxMax = m_workspace->getOpenSheet()->getRoot()->m_matrix * m_boxMax;
    return fabs(boxMin.m_elements[0] - boxMax.m_elements[0]) > 20.0f ||
           fabs(boxMin.m_elements[1] - boxMax.m_elements[1]) > 20.0f;
}

bool BoundingBoxTool::isOverHandle()
{
    return findHandle() != NoHandle;
}

bool BoundingBoxTool::isDraggingHandle()
{
    return m_dragHandle != NoHandle;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void BoundingBoxTool::renderBox()
{
    glDisable(GL_LINE_SMOOTH);
    if (m_boxMin.m_elements[0] < m_boxMax.m_elements[0]) {
        int highlighted = findHandle();
        if (m_workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) && !m_workspace->m_guiPointerNode &&
            m_dragHandle != NoHandle)
            highlighted = m_dragHandle;
        glPushMatrix();
        glMultMatrixf(m_workspace->getOpenSheet()->getRoot()->m_matrix.m_elements);

        // A dark outline under a light one.
        glLineWidth(3.0f);
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(m_boxMin.m_elements[0], m_boxMin.m_elements[1]);
        glVertex2f(m_boxMax.m_elements[0], m_boxMin.m_elements[1]);
        glVertex2f(m_boxMax.m_elements[0], m_boxMax.m_elements[1]);
        glVertex2f(m_boxMin.m_elements[0], m_boxMax.m_elements[1]);
        glEnd();
        glLineWidth(1.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(m_boxMin.m_elements[0], m_boxMin.m_elements[1]);
        glVertex2f(m_boxMax.m_elements[0], m_boxMin.m_elements[1]);
        glVertex2f(m_boxMax.m_elements[0], m_boxMax.m_elements[1]);
        glVertex2f(m_boxMin.m_elements[0], m_boxMax.m_elements[1]);
        glEnd();
        if (handlesShown()) {
            glPointSize(9.0f);
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_POINTS);
            for (int i = 0; i < 8; i++)
                glVertex2fv(m_handles[i].m_elements);
            glEnd();

            // White handles, the highlighted one cyan.
            glPointSize(7.0f);
            glBegin(GL_POINTS);
            for (int i = 0; i < 8; i++) {
                float red = 1.0f;
                if (i == highlighted)
                    red = 0.0f;
                glColor3f(red, 1.0f, 1.0f);
                glVertex2fv(m_handles[i].m_elements);
            }
            glEnd();
        }
        glLoadIdentity();
        if ((!m_workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) && findHandle() != NoHandle) ||
            (m_workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) && m_dragHandle != NoHandle)) {
            if (!m_workspace->m_guiPointerNode) {
                std::wstring unit;
                switch (m_workspace->getOpenSheet()->m_page.unit) {
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

                // "width x height unit", outlined, below right of the pointer.
                std::wstringstream text;
                text << roundToHundredths(plasma::fromInternalUnits(m_workspace->getOpenSheet()->m_page.unit,
                                                                    m_boxMax.m_elements[0] - m_boxMin.m_elements[0]))
                     << L" x "
                     << roundToHundredths(plasma::fromInternalUnits(m_workspace->getOpenSheet()->m_page.unit,
                                                                    m_boxMax.m_elements[1] - m_boxMin.m_elements[1]))
                     << L" " << unit;
                Vector<2, float> position = m_workspace->m_sceneMousePosition;
                position.m_elements[0] += 15.0;
                position.m_elements[1] += 15.0;
                m_workspace->m_engine.m_fontEngine->renderText(
                    L"verdanab.ttf", text.str(), 0.0f, 0.0f, position.m_elements[0], position.m_elements[1], 8.0f, 1.5f,
                    Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                    Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f).m_elements,
                    Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f, true);
                m_workspace->m_engine.m_fontEngine->renderText(
                    L"verdanab.ttf", text.str(), 0.0f, 0.0f, position.m_elements[0], position.m_elements[1], 8.0f, 0.0f,
                    Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                    Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                    Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f, true);
            }
        }
        glPopMatrix();
    }
}

void BoundingBoxTool::renderSelection()
{
    if (m_selectionMode == SELECTION_BOX) {
        glPushMatrix();
        glMultMatrixf(m_workspace->getOpenSheet()->m_root->m_matrix.m_elements);
        glLineWidth(3.0f);
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(m_rectangleStart.m_elements[0], m_rectangleStart.m_elements[1]);
        glVertex2f(m_rectangleEnd.m_elements[0], m_rectangleStart.m_elements[1]);
        glVertex2f(m_rectangleEnd.m_elements[0], m_rectangleEnd.m_elements[1]);
        glVertex2f(m_rectangleStart.m_elements[0], m_rectangleEnd.m_elements[1]);
        glEnd();
        glLineWidth(1.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(m_rectangleStart.m_elements[0], m_rectangleStart.m_elements[1]);
        glVertex2f(m_rectangleEnd.m_elements[0], m_rectangleStart.m_elements[1]);
        glVertex2f(m_rectangleEnd.m_elements[0], m_rectangleEnd.m_elements[1]);
        glVertex2f(m_rectangleStart.m_elements[0], m_rectangleEnd.m_elements[1]);
        glEnd();
        glPopMatrix();
    }
    if (m_selectionMode == SELECTION_LASSO) {
        glPushMatrix();
        glMultMatrixf(m_workspace->getOpenSheet()->m_root->m_matrix.m_elements);
        glLineWidth(3.0f);
        glBegin(GL_LINE_LOOP);
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
        for (int i = 0; i < (int)m_lasso.size(); i++)
            glVertex2fv(m_lasso[i].m_elements);
        glEnd();
        glLineWidth(1.0f);
        glBegin(GL_LINE_LOOP);
        glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
        for (int i = 0; i < (int)m_lasso.size(); i++)
            glVertex2fv(m_lasso[i].m_elements);
        glEnd();
        glPopMatrix();
    }
}

void BoundingBoxTool::beginSelection()
{
    if (m_selectionMode == SELECTION_BOX) {
        m_rectangleStart = m_workspace->getOpenSheet()->m_root->m_inverseMatrix * m_workspace->getSceneMousePosition();
        m_rectangleEnd = m_rectangleStart;
    } else {
        m_lasso.clear();
        m_lasso.push_back(m_workspace->getOpenSheet()->m_root->m_inverseMatrix * m_workspace->getSceneMousePosition());
    }
}

void BoundingBoxTool::endSelection()
{
    if (m_selectionMode == SELECTION_LASSO) {
        // Within two pixels at the sheet's zoom.
        std::vector<int> kept;
        float zoom = m_workspace->getOpenDesignSheet()->m_root->m_matrix.m_elements[0];
        simplifyPath(2.0 / zoom, m_lasso, kept);
        std::vector<Vector<2, float>> lasso;
        for (int i = 0; i < (int)kept.size(); i++)
            if (kept[i])
                lasso.push_back(m_lasso[i]);
        m_lasso = lasso;
    }
}

void BoundingBoxTool::cancelSelection()
{
    m_selectionMode = SELECTION_NONE;
    m_lasso.clear();
}

bool BoundingBoxTool::contains(const Vector<2, float> &point)
{
    if (m_workspace->getOpenSheet()) {
        if (m_selectionMode == SELECTION_BOX) {
            const Vector<2, float> &start = m_rectangleStart;
            const Vector<2, float> &end = m_rectangleEnd;
            Vector<2, float> low(start.m_elements[0] < end.m_elements[0] ? start.m_elements[0] : end.m_elements[0],
                                 start.m_elements[1] < end.m_elements[1] ? start.m_elements[1] : end.m_elements[1]);
            Vector<2, float> high(start.m_elements[0] > end.m_elements[0] ? start.m_elements[0] : end.m_elements[0],
                                  start.m_elements[1] > end.m_elements[1] ? start.m_elements[1] : end.m_elements[1]);
            low.transform(m_workspace->getOpenSheet()->m_root->m_matrix);
            high.transform(m_workspace->getOpenSheet()->m_root->m_matrix);
            return point.m_elements[0] >= low.m_elements[0] && point.m_elements[0] <= high.m_elements[0] &&
                   point.m_elements[1] >= low.m_elements[1] && point.m_elements[1] <= high.m_elements[1];
        }
        if (m_selectionMode == SELECTION_LASSO) {
            // Counts the lasso's edges crossing the horizontal ray from the point.
            bool inside = false;
            Vector<2, float> sheetPoint = m_workspace->getOpenSheet()->m_root->m_inverseMatrix.transformPoint(point);
            int count = m_lasso.size();
            for (int i = 0, j = count - 1; i < count; j = i++) {
                if ((m_lasso[i].m_elements[1] <= sheetPoint.m_elements[1] &&
                     m_lasso[j].m_elements[1] > sheetPoint.m_elements[1]) ||
                    (m_lasso[j].m_elements[1] <= sheetPoint.m_elements[1] &&
                     m_lasso[i].m_elements[1] > sheetPoint.m_elements[1])) {
                    float x = (sheetPoint.m_elements[1] - m_lasso[i].m_elements[1]) *
                                  (m_lasso[j].m_elements[0] - m_lasso[i].m_elements[0]) /
                                  (m_lasso[j].m_elements[1] - m_lasso[i].m_elements[1]) +
                              m_lasso[i].m_elements[0];
                    if (x > sheetPoint.m_elements[0])
                        inside = !inside;
                }
            }
            return inside;
        }
    }
    return false;
}

void BoundingBoxTool::updateSelection()
{
    if (m_selectionMode == SELECTION_BOX)
        m_rectangleEnd =
            m_workspace->getOpenSheet()->m_root->m_inverseMatrix.transform(m_workspace->getSceneMousePosition());
    if (m_selectionMode == SELECTION_LASSO)
        m_lasso.push_back(
            m_workspace->getOpenSheet()->m_root->m_inverseMatrix.transform(m_workspace->getSceneMousePosition()));
}

void BoundingBoxTool::beginHandleDrag()
{
    m_dragHandle = NoHandle;
    if (handlesShown()) {
        int handle = findHandle();
        if (handle != NoHandle) {
            m_dragHandle = handle;
            m_dragStart = m_handles[handle];
            m_dragPosition = m_dragStart;
            m_workspace->m_sheetPointer = m_dragPosition;

            // The opposite corner, or the opposite edge's midpoint.
            m_dragAnchor = m_handles[(handle + 2) % 4 + handle / 4 * 4];
            if (m_workspace->m_engine.isKeyDown(VK_SHIFT))
                m_dragAnchor = (m_dragAnchor + m_dragPosition) * 0.5f;
        }
    }
}

bool BoundingBoxTool::getSnapOffsets(const Vector<2, float> &point, std::list<Vector<2, float>> &offsets)
{
    offsets.push_back(m_boxMin - point);
    offsets.push_back(Vector<2, float>(m_boxMin.m_elements[0], m_boxMax.m_elements[1]) - point);
    offsets.push_back(Vector<2, float>(m_boxMax.m_elements[0], m_boxMin.m_elements[1]) - point);
    offsets.push_back(m_boxMax - point);
    return true;
}
