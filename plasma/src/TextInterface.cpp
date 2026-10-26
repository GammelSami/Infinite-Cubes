#include "TextInterface.h"
#include "BoundingBoxTool.h"
#include "DesignSheet.h"
#include "plasma/Node.h"
#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
#include "plasma/TextShape.h"
#include "SequenceModule.h"
#include "Workspace.h"
#include "main.h"
#include "ColorModule.h"
#include "ColorToolModule.h"
#include "NodeToolModule.h"
#include "ToolModule.h"
#include "StrokeWidthModule.h"
#include "ClipboardModule.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <set>
#include <windows.h>
#include <GL/glew.h>

using plasma::TextShape;
using plasma::Vector;

// The caret's blink period.
enum {
    CARET_BLINK_INTERVAL_MS = 500
};

TextInterface::TextInterface(NodeInterface *node, TextShape *shape)
    : ShapeInterface(node, INTERFACE_TEXT),
      m_shape(shape),
      m_edited(false),
      m_selectionLength(0),
      m_cursor(0)
{}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void TextInterface::draw()
{
    Workspace *workspace;
    if (!m_hasMatrix && m_shape->m_font) {
        workspace = m_sheet->m_workspace;
        if (!workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_MIDDLE) && !workspace->isPanning() &&
            (workspace->m_toolModule->getCurrentTool()->getType() == Tool::TOOL_NODE &&
                 m_node->getNode() == workspace->getHoveredNode() && !workspace->m_guiPointerNode &&
                 !workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) &&
                 !workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT) ||
             m_node->m_selected ||
             workspace->getDesignSheet() && m_node->getNode() == m_sheet->m_workspace->getDesignSheet()->m_editRoot)) {
            glUseProgram(0);
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_INDEX_ARRAY);
            plasma::Matrix<float> matrix;
            matrix.identity();
            m_sheet->m_workspace->m_engine.m_fontEngine->lock();
            m_shape->m_font->prepare(matrix, m_shape->m_pixelSize, m_shape->m_strokeRadius, m_shape->m_spacing,
                                     m_shape->m_lineSpacing, false);
            Vector<2, float> minimum, maximum;
            if (m_shape->m_flags & (1 << plasma::TEXT_WRAP)) {
                // Wrapped: measure the text as laid out.
                std::wstring text = m_shape->m_string.value();
                m_shape->m_font->wrapText(text, m_shape->m_wrapWidth);
                m_shape->m_font->getBounds(text, minimum, maximum, m_shape->m_flags, -1,
                                           !m_sheet->m_workspace->getNodeToolModule()->m_shapeBoundary);
            } else {
                m_shape->m_font->getBounds(m_shape->m_string.value(), minimum, maximum, m_shape->m_flags, -1,
                                           !m_sheet->m_workspace->getNodeToolModule()->m_shapeBoundary);
            }
            m_sheet->m_workspace->m_engine.m_fontEngine->unlock();
            if (m_shape->m_flags & (1 << plasma::TEXT_WRAP)) {
                float right = m_shape->m_wrapWidth + minimum.m_elements[0];
                if (maximum.m_elements[0] < right)
                    maximum.m_elements[0] = right;
            }
            glDisable(GL_TEXTURE_2D);
            Vector<2, float> corners[4] = {
                Vector<2, float>(minimum.m_elements[0], minimum.m_elements[1]),
                Vector<2, float>(maximum.m_elements[0], minimum.m_elements[1]),
                Vector<2, float>(maximum.m_elements[0], maximum.m_elements[1]),
                Vector<2, float>(minimum.m_elements[0], maximum.m_elements[1]),
            };
            Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
            if (m_node->m_selected ||
                m_sheet->m_workspace->m_toolModule->getCurrentTool()->getType() == Tool::TOOL_NODE &&
                    m_node->getNode() == m_sheet->m_workspace->getHoveredNode()) {
                if (this == m_sheet->m_workspace->m_hoveredTextInterface)
                    color = Vector<4, float>(0.0f, 1.0f, 1.0f, 0.5f);
                else if (!m_sheet->m_workspace->m_hoveredTextInterface &&
                         m_node->getNode() == m_sheet->m_workspace->getHoveredNode())
                    color = Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f);
                else if (m_node->getNode() == m_sheet->m_currentNode)
                    color = Vector<4, float>(0.8f, 0.5f, 1.0f, 1.0f);
                else
                    color = Vector<4, float>(0.5f, 1.0f, 0.5f, 1.0f);
            }

            // A wide frame in half the colour under a thin one.
            glLineWidth(3.0f);
            glBegin(GL_LINE_LOOP);
            glColor4fv((color * 0.5f).m_elements);
            for (int i = 0; i < 4; ++i)
                glVertex2fv(corners[i].m_elements);
            glEnd();
            glLineWidth(1.0f);
            glColor4fv(color.m_elements);
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 4; ++i)
                glVertex2fv(corners[i].m_elements);
            glEnd();
            if (this == m_sheet->m_workspace->getEditingTextInterface() &&
                m_shape->m_flags & (1 << plasma::TEXT_WRAP)) {
                // The wrap width, from the aligned side.
                float wrapWidth = m_shape->m_wrapWidth;
                if (!(m_shape->m_flags & plasma::TEXT_HORIZONTAL_MASK)) {
                    glLineWidth(3.0f);
                    glBegin(GL_LINES);
                    glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
                    float x = minimum.m_elements[0] + wrapWidth;
                    glVertex2f(x, minimum.m_elements[1]);
                    glVertex2f(x, maximum.m_elements[1]);
                    glEnd();
                    glLineWidth(1.0f);
                    glBegin(GL_LINES);
                    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                    glVertex2f(x, minimum.m_elements[1]);
                    glVertex2f(x, maximum.m_elements[1]);
                    glEnd();
                } else if (!(m_shape->m_flags & (1 << plasma::TEXT_CENTER))) {
                    glLineWidth(3.0f);
                    glBegin(GL_LINES);
                    glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
                    float x = maximum.m_elements[0] - wrapWidth;
                    glVertex2f(x, minimum.m_elements[1]);
                    glVertex2f(x, maximum.m_elements[1]);
                    glEnd();
                    glLineWidth(1.0f);
                    glBegin(GL_LINES);
                    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                    glVertex2f(x, minimum.m_elements[1]);
                    glVertex2f(x, maximum.m_elements[1]);
                    glEnd();
                } else {
                    float center = (maximum.m_elements[0] + minimum.m_elements[0]) * 0.5;
                    glLineWidth(3.0f);
                    glBegin(GL_LINES);
                    glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
                    double half = wrapWidth * 0.5;
                    float left = center - half;
                    glVertex2f(left, minimum.m_elements[1]);
                    glVertex2f(left, maximum.m_elements[1]);
                    float right = center + half;
                    glVertex2f(right, minimum.m_elements[1]);
                    glVertex2f(right, maximum.m_elements[1]);
                    glEnd();
                    glLineWidth(1.0f);
                    glBegin(GL_LINES);
                    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                    glVertex2f(left, minimum.m_elements[1]);
                    glVertex2f(left, maximum.m_elements[1]);
                    glVertex2f(right, minimum.m_elements[1]);
                    glVertex2f(right, maximum.m_elements[1]);
                    glEnd();
                }
            }
        }
    }

    // The caret where a click would put it, while the text tool hovers the text.
    workspace = m_sheet->m_workspace;
    if (workspace->m_hoveredTextInterface == this && m_shape->m_font) {
        int index = characterAt(m_node->getNode()->m_inverseMatrix.transform(workspace->m_sceneMousePosition));
        if (index >= 0 && index <= (int)m_shape->m_string.value().size()) {
            plasma::Matrix<float> matrix;
            matrix.identity();
            m_sheet->m_workspace->m_engine.m_fontEngine->lock();
            Vector<2, float> position, height, extent;
            m_shape->m_font->getCaret(matrix, std::wstring(L"A"), 0, m_shape->m_pixelSize, m_shape->m_strokeRadius,
                                      m_shape->m_spacing, m_shape->m_lineSpacing, m_shape->m_wrapWidth, position,
                                      height, m_shape->m_flags, false);
            m_shape->m_font->getCaret(matrix, m_shape->m_string.value(), index, m_shape->m_pixelSize,
                                      m_shape->m_strokeRadius, m_shape->m_spacing, m_shape->m_lineSpacing,
                                      m_shape->m_wrapWidth, position, extent, m_shape->m_flags, false);
            m_sheet->m_workspace->m_engine.m_fontEngine->unlock();
            glDisable(GL_TEXTURE_2D);
            glLineWidth(3.0f);
            glBegin(GL_LINES);
            glColor4f(0.0f, 0.5f, 0.5f, 0.5f);
            float bottom = extent.m_elements[1] + position.m_elements[1];
            glVertex2f(position.m_elements[0], bottom);
            float top = extent.m_elements[1] + position.m_elements[1] - height.m_elements[1];
            glVertex2f(position.m_elements[0], top);
            glEnd();
            glLineWidth(1.0f);
            glBegin(GL_LINES);
            glColor3f(0.0f, 1.0f, 1.0f);
            glVertex2f(position.m_elements[0], bottom);
            glVertex2f(position.m_elements[0], top);
            glEnd();
        }
    }

    // The selection and the blinking cursor of the text being edited.
    if (m_sheet->m_workspace->getEditingTextInterface() == this && m_shape->m_font) {
        int first = m_cursor;
        int last = m_cursor + m_selectionLength;
        plasma::Matrix<float> matrix;
        matrix.identity();
        if (last < first) {
            int swap = first;
            first = last;
            last = swap;
        }
        if (first < 0)
            first = 0;
        if (last > (int)m_shape->m_string.value().size())
            last = m_shape->m_string.value().size();
        if (last != first) {
            Vector<2, float> position, extent;
            m_sheet->m_workspace->m_engine.m_fontEngine->lock();
            m_shape->m_font->getCaret(matrix, m_shape->m_string.value(), first, m_shape->m_pixelSize,
                                      m_shape->m_strokeRadius, m_shape->m_spacing, m_shape->m_lineSpacing,
                                      m_shape->m_wrapWidth, position, extent, m_shape->m_flags, false);
            m_shape->m_font->getCaret(matrix, m_shape->m_string.value(), last, m_shape->m_pixelSize,
                                      m_shape->m_strokeRadius, m_shape->m_spacing, m_shape->m_lineSpacing,
                                      m_shape->m_wrapWidth, position, extent, m_shape->m_flags, false);
            m_sheet->m_workspace->m_engine.m_fontEngine->unlock();
            std::wstring text = m_shape->m_string.value();
            m_sheet->m_workspace->m_engine.m_fontEngine->lock();
            if (m_shape->m_flags & (1 << plasma::TEXT_WRAP))
                m_shape->m_font->wrapText(text, m_shape->m_wrapWidth);
            Vector<2, float> minimum, maximum;
            m_shape->m_font->getBounds(text, minimum, maximum, m_shape->m_flags, -1, false);
            m_sheet->m_workspace->m_engine.m_fontEngine->unlock();

            // A quad per line from the line's first selected character to its end.
            glBegin(GL_QUADS);
            bool lineStart = true;
            Vector<2, float> start, end;
            for (; first <= last; ++first) {
                if (lineStart) {
                    m_sheet->m_workspace->m_engine.m_fontEngine->lock();
                    m_shape->m_font->getCaret(matrix, text, first, m_shape->m_pixelSize, m_shape->m_strokeRadius,
                                              m_shape->m_spacing, m_shape->m_lineSpacing, m_shape->m_wrapWidth, start,
                                              extent, m_shape->m_flags, false);
                    m_sheet->m_workspace->m_engine.m_fontEngine->unlock();
                    lineStart = false;
                }

                // The code reads the terminating NUL through mutable operator[] at text.size.
                if (text[first] == L'\n' || first == last) {
                    lineStart = true;
                    m_sheet->m_workspace->m_engine.m_fontEngine->lock();
                    m_shape->m_font->getCaret(matrix, text, first, m_shape->m_pixelSize, m_shape->m_strokeRadius,
                                              m_shape->m_spacing, m_shape->m_lineSpacing, m_shape->m_wrapWidth, end,
                                              extent, m_shape->m_flags, false);
                    m_sheet->m_workspace->m_engine.m_fontEngine->unlock();
                    glColor4f(0.0f, 1.0f, 1.0f, 0.25f);
                    glVertex2f(start.m_elements[0], start.m_elements[1]);
                    glVertex2f(end.m_elements[0], start.m_elements[1]);
                    glVertex2f(end.m_elements[0], m_shape->m_pixelSize + end.m_elements[1]);
                    glVertex2f(start.m_elements[0], m_shape->m_pixelSize + end.m_elements[1]);
                    glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
                    glVertex2f(start.m_elements[0], start.m_elements[1]);
                    glVertex2f(end.m_elements[0], start.m_elements[1]);
                    glVertex2f(end.m_elements[0], m_shape->m_pixelSize + end.m_elements[1]);
                    glVertex2f(start.m_elements[0], m_shape->m_pixelSize + end.m_elements[1]);
                }
            }
            glEnd();
        }
        if (m_cursor >= 0 && m_cursor <= (int)m_shape->m_string.value().size()) {
            workspace = m_sheet->m_workspace;

            // Shown for the first half of every second.
            if (!(workspace->m_engine.m_caretClock / CARET_BLINK_INTERVAL_MS % 2)) {
                workspace->m_engine.m_fontEngine->lock();
                Vector<2, float> position, height, extent;
                m_shape->m_font->getCaret(matrix, std::wstring(L"A"), 0, m_shape->m_pixelSize, m_shape->m_strokeRadius,
                                          m_shape->m_spacing, m_shape->m_lineSpacing, m_shape->m_wrapWidth, position,
                                          height, m_shape->m_flags, false);
                m_shape->m_font->getCaret(matrix, m_shape->m_string.value(), m_cursor, m_shape->m_pixelSize,
                                          m_shape->m_strokeRadius, m_shape->m_spacing, m_shape->m_lineSpacing,
                                          m_shape->m_wrapWidth, position, extent, m_shape->m_flags, false);
                m_sheet->m_workspace->m_engine.m_fontEngine->unlock();
                glDisable(GL_TEXTURE_2D);
                glLineWidth(4.0f);
                glBegin(GL_LINES);
                glColor3f(1.0f, 1.0f, 1.0f);
                float bottom = extent.m_elements[1] + position.m_elements[1];
                glVertex2f(position.m_elements[0], bottom);
                float top = extent.m_elements[1] + position.m_elements[1] - height.m_elements[1];
                glVertex2f(position.m_elements[0], top);
                glEnd();
                glLineWidth(2.0f);
                glBegin(GL_LINES);
                glColor3f(0.0f, 0.0f, 0.0f);
                glVertex2f(position.m_elements[0], bottom);
                glVertex2f(position.m_elements[0], top);
                glEnd();
            }
        }
    }
}

bool TextInterface::isInsideBox(BoundingBoxTool *tool)
{
    Vector<2, float> minimum, maximum;
    getBounds(&minimum, &maximum);
    return tool->contains(Vector<2, float>(minimum.m_elements[0], minimum.m_elements[1])) &&
           tool->contains(Vector<2, float>(minimum.m_elements[0], maximum.m_elements[1])) &&
           tool->contains(Vector<2, float>(maximum.m_elements[0], maximum.m_elements[1])) &&
           tool->contains(Vector<2, float>(maximum.m_elements[0], minimum.m_elements[1]));
}

void TextInterface::captureState(TextState &state)
{
    state.m_string = m_shape->m_string;
    state.m_color = m_shape->m_color;
    state.m_strokeColor = m_shape->m_strokeColor;
    state.m_extrusionColor = m_shape->m_extrusionColor;
    state.m_flags = m_shape->m_flags;
    state.m_fontName = m_shape->m_fontName;
    state.m_pixelSize = m_shape->m_pixelSize;
    state.m_strokeRadius = m_shape->m_strokeRadius;
    state.m_spacing = m_shape->m_spacing;
    state.m_lineSpacing = m_shape->m_lineSpacing;
    state.m_wrapWidth = m_shape->m_wrapWidth;
}

void TextInterface::restoreState(const TextState &state)
{
    m_shape->m_string = state.m_string;
    m_shape->m_color = state.m_color;
    m_shape->m_strokeColor = state.m_strokeColor;
    m_shape->m_extrusionColor = state.m_extrusionColor;
    m_shape->m_fontName = state.m_fontName;
    m_shape->m_flags = state.m_flags;
    m_shape->m_pixelSize = state.m_pixelSize;
    m_shape->m_strokeRadius = state.m_strokeRadius;
    m_shape->m_spacing = state.m_spacing;
    m_shape->m_lineSpacing = state.m_lineSpacing;
    m_shape->m_wrapWidth = state.m_wrapWidth;
    m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
}

void TextInterface::deleteSelection()
{
    if (m_cursor > (int)m_shape->m_string.value().size())
        return;
    int position = m_cursor;
    int count = m_selectionLength;
    if (!count) {
        count = 1;
    } else if (count < 0) {
        position += count;
        count = -count;
    }
    m_shape->m_string.value().erase(position, count);
    if (m_selectionLength < 0)
        m_cursor += m_selectionLength;
    m_selectionLength = 0;
    m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
}

void TextInterface::keyPressed(unsigned short key)
{
    if (!m_shape->m_string.value().empty()) {
        if (key == VK_LEFT) {
            if (m_cursor > 0) {
                --m_cursor;
                std::set<unsigned short> &keys = m_sheet->m_workspace->m_engine.m_pressedKeys;
                if (keys.find(VK_SHIFT) != keys.end())
                    ++m_selectionLength;
                else
                    m_selectionLength = 0;
            }
            return;
        }
        if (key == VK_RIGHT) {
            if (m_cursor < (int)m_shape->m_string.value().size()) {
                ++m_cursor;
                std::set<unsigned short> &keys = m_sheet->m_workspace->m_engine.m_pressedKeys;
                if (keys.find(VK_SHIFT) != keys.end())
                    --m_selectionLength;
                else
                    m_selectionLength = 0;
            }
            return;
        }
        if (key == VK_DOWN) {
            plasma::Matrix<float> matrix;
            matrix.identity();
            Vector<2, float> position, extent;
            m_sheet->m_workspace->m_engine.m_fontEngine->lock();
            m_shape->m_font->getCaret(matrix, m_shape->m_string.value(), m_cursor, m_shape->m_pixelSize,
                                      m_shape->m_strokeRadius, m_shape->m_spacing, m_shape->m_lineSpacing,
                                      m_shape->m_wrapWidth, position, extent, m_shape->m_flags, false);
            m_sheet->m_workspace->m_engine.m_fontEngine->unlock();
            int cursor = m_cursor;

            // The character a line below the caret.
            m_cursor = characterAt(
                Vector<2, float>(position.m_elements[0] + 0.0, position.m_elements[1] + extent.m_elements[1]));
            if (m_sheet->m_workspace->m_engine.m_pressedKeys.find(VK_SHIFT) !=
                m_sheet->m_workspace->m_engine.m_pressedKeys.end())
                m_selectionLength = cursor - m_cursor;
            else
                m_selectionLength = 0;
            return;
        }
        if (key == VK_UP) {
            plasma::Matrix<float> matrix;
            matrix.identity();
            Vector<2, float> position, extent;
            m_sheet->m_workspace->m_engine.m_fontEngine->lock();
            m_shape->m_font->getCaret(matrix, m_shape->m_string.value(), m_cursor, m_shape->m_pixelSize,
                                      m_shape->m_strokeRadius, m_shape->m_spacing, m_shape->m_lineSpacing,
                                      m_shape->m_wrapWidth, position, extent, m_shape->m_flags, false);
            m_sheet->m_workspace->m_engine.m_fontEngine->unlock();
            int cursor = m_cursor;

            // The character a line above the caret.
            m_cursor = characterAt(
                Vector<2, float>(position.m_elements[0] - 0.0, position.m_elements[1] - extent.m_elements[1]));
            if (m_sheet->m_workspace->m_engine.m_pressedKeys.find(VK_SHIFT) !=
                m_sheet->m_workspace->m_engine.m_pressedKeys.end())
                m_selectionLength = cursor - m_cursor;
            else
                m_selectionLength = 0;
            return;
        }
        if (key == VK_HOME) {
            plasma::Matrix<float> matrix;
            matrix.identity();
            m_sheet->m_workspace->m_engine.m_fontEngine->lock();
            m_shape->m_font->prepare(matrix, m_shape->m_pixelSize, m_shape->m_strokeRadius, m_shape->m_spacing,
                                     m_shape->m_lineSpacing, false);

            // The text as laid out, with the line breaks of its wrapping.
            std::wstring text = m_shape->m_string.value();
            if (m_shape->m_flags & (1 << plasma::TEXT_WRAP))
                m_shape->m_font->wrapText(text, m_shape->m_wrapWidth);
            m_sheet->m_workspace->m_engine.m_fontEngine->unlock();
            int cursor = m_cursor;

            // The code trusts the cursor, even after a truncated paste advanced it past the text.
            while (m_cursor > 0 && text[m_cursor - 1] != L'\n')
                --m_cursor;
            std::set<unsigned short> &keys = m_sheet->m_workspace->m_engine.m_pressedKeys;
            if (keys.find(VK_SHIFT) != keys.end())
                m_selectionLength = cursor - m_cursor;
            else
                m_selectionLength = 0;
            return;
        }
        if (key == VK_END) {
            plasma::Matrix<float> matrix;
            matrix.identity();
            m_sheet->m_workspace->m_engine.m_fontEngine->lock();
            m_shape->m_font->prepare(matrix, m_shape->m_pixelSize, m_shape->m_strokeRadius, m_shape->m_spacing,
                                     m_shape->m_lineSpacing, false);
            std::wstring text = m_shape->m_string.value();
            if (m_shape->m_flags & (1 << plasma::TEXT_WRAP))
                m_shape->m_font->wrapText(text, m_shape->m_wrapWidth);
            m_sheet->m_workspace->m_engine.m_fontEngine->unlock();
            int cursor = m_cursor;
            while (m_cursor < (int)text.size() && text[m_cursor] != L'\n')
                ++m_cursor;
            // Tests the held key.
            if (m_sheet->m_workspace->m_engine.isKeyDown(VK_SHIFT))
                m_selectionLength = cursor - m_cursor;
            else
                m_selectionLength = 0;
            return;
        }
    }
    if (key == VK_DELETE) {
        beginModify();
        deleteSelection();
        endModify();
        m_sheet->commit();
        m_sheet->m_workspace->redraw();
        m_edited = true;
    } else if (key == VK_BACK) {
        if (m_selectionLength) {
            beginModify();
            deleteSelection();
            endModify();
            m_sheet->commit();
            m_edited = true;
            m_sheet->m_workspace->redraw();
        } else if (m_cursor > 0) {
            beginModify();
            m_shape->m_string.value().erase(m_shape->m_string.value().begin() + (m_cursor - 1));
            --m_cursor;
            m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            endModify();
            m_sheet->commit();
            m_sheet->m_workspace->redraw();
            m_edited = true;
        }
    } else if (key == VK_ESCAPE) {
        m_sheet->m_workspace->setTextInterface(0);
    } else if (key == VK_RETURN) {
        beginModify();
        m_shape->m_string.value().insert(m_cursor, L"\n");
        m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        ++m_cursor;
        m_edited = true;
        endModify();
        m_sheet->commit();
        m_sheet->m_workspace->redraw();
    }
}

void TextInterface::typeCharacter(wchar_t character)
{
    if (character < L' ')
        return;
    std::wstring text(L"");
    text += character;
    beginModify();
    if (m_selectionLength)
        deleteSelection();
    m_shape->m_string.value().insert(m_cursor, text);
    ++m_cursor;
    m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    endModify();
    m_sheet->commit();
    m_sheet->m_workspace->redraw();
    m_edited = true;
}

bool TextInterface::setVertexColorsKey()
{
    TextState before;
    captureState(before);
    const std::wstring &sequence = m_sheet->m_workspace->m_sequenceModule->m_sequenceName;
    m_shape->m_color.setKey(sequence, 0, 1.0f);
    TextState after;
    captureState(after);
    m_sheet->m_workspace->getOpenDesignSheet()->addOperation(
        new TextOperation(m_sheet->m_workspace->getOpenDesignSheet(), "set color key", this, before, after));
    return true;
}

bool TextInterface::setStrokeColorsKey()
{
    TextState before;
    captureState(before);
    const std::wstring &sequence = m_sheet->m_workspace->m_sequenceModule->m_sequenceName;
    m_shape->m_strokeColor.setKey(sequence, 0, 1.0f);
    TextState after;
    captureState(after);
    m_sheet->m_workspace->getOpenDesignSheet()->addOperation(
        new TextOperation(m_sheet->m_workspace->getOpenDesignSheet(), "set stroke color key", this, before, after));
    return true;
}

bool TextInterface::setTextKey()
{
    TextState before;
    captureState(before);
    const std::wstring &sequence = m_sheet->m_workspace->m_sequenceModule->m_sequenceName;
    m_shape->m_string.setKey(sequence, 0, 1.0f);
    TextState after;
    captureState(after);
    m_sheet->m_workspace->getOpenDesignSheet()->addOperation(
        new TextOperation(m_sheet->m_workspace->getOpenDesignSheet(), "set text key", this, before, after));
    return true;
}

void TextInterface::insertText(const std::wstring &text)
{
    if (!m_shape)
        return;
    beginModify();

    // A copy of the text before the paste; unused.
    std::wstring previous = m_shape->m_string.value();
    if (m_selectionLength)
        deleteSelection();
    std::wstring updatedText = m_shape->m_string.value();

    // Subtracts one even for bare LF and advances by the entire pasted text, including unsigned wrap when LF is
    // absent.
    updatedText.insert(m_cursor, text.substr(0, text.find(L'\n') - 1));
    m_cursor += text.size();
    m_shape->m_string.value() = updatedText;
    m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    endModify();
    m_sheet->commit();
}

void TextInterface::setCursorToMouse()
{
    m_cursor = characterAt(m_node->m_node->m_inverseMatrix.transform(m_sheet->m_workspace->getSceneMousePosition()));
}

int TextInterface::characterAt(const Vector<2, float> &point)
{
    plasma::Matrix<float> matrix;
    std::wstring text = m_shape->m_string.value();
    matrix.identity();
    m_sheet->m_workspace->m_engine.m_fontEngine->lock();
    int index =
        m_shape->m_font->hitTest(text, matrix, point, m_shape->m_pixelSize, m_shape->m_strokeRadius, m_shape->m_spacing,
                                 m_shape->m_lineSpacing, m_shape->m_wrapWidth, m_shape->m_flags);
    m_sheet->m_workspace->m_engine.m_fontEngine->unlock();
    return index;
}

void TextInterface::moveCursorSelecting()
{
    int cursor = m_cursor;
    setCursorToMouse();
    m_selectionLength += cursor - m_cursor;
}

void TextInterface::clearSelection()
{
    m_selectionLength = 0;
}

void TextInterface::getColors(std::list<Vector<4, float>> &colors)
{
    colors.push_back(m_shape->m_color.value());
    if (m_shape->m_strokeRadius > 0.0f)
        colors.push_back(m_shape->m_strokeColor.value());
}

void TextInterface::beginModify()
{
    captureState(m_state);
}

void TextInterface::endModify()
{
    TextState after;
    captureState(after);
    m_sheet->addOperation(new TextOperation(m_sheet, "modify text", this, m_state, after));
}

std::wstring TextInterface::getSelectedText()
{
    if (!m_selectionLength)
        return std::wstring(L"");

    // A selection that extends backwards starts before the cursor.
    int start = m_cursor < m_cursor + m_selectionLength ? m_cursor : m_cursor + m_selectionLength;
    return m_shape->m_string.value().substr(start, abs(m_selectionLength));
}

void TextInterface::setColorPreview(bool preview)
{
    m_shape->m_color.m_current = plasma::Attribute::BASE_ELEMENT;
    m_shape->m_strokeColor.m_current = plasma::Attribute::BASE_ELEMENT;
    m_shape->m_extrusionColor.m_current = plasma::Attribute::BASE_ELEMENT;
    if (preview) {
        m_shape->m_color.copyFromCurrent(plasma::Attribute::STORED_ELEMENT);
        m_shape->m_strokeColor.copyFromCurrent(plasma::Attribute::STORED_ELEMENT);
        m_shape->m_extrusionColor.copyFromCurrent(plasma::Attribute::STORED_ELEMENT);
        m_shape->m_color.m_current = plasma::Attribute::STORED_ELEMENT;
        m_shape->m_strokeColor.m_current = plasma::Attribute::STORED_ELEMENT;
        m_shape->m_extrusionColor.m_current = plasma::Attribute::STORED_ELEMENT;
    }
    m_colorPreview = preview;
}

void TextInterface::applyColorPreview()
{
    if (!m_colorPreview)
        return;
    m_shape->m_color.m_current = plasma::Attribute::STORED_ELEMENT;
    m_shape->m_strokeColor.m_current = plasma::Attribute::STORED_ELEMENT;
    m_shape->m_extrusionColor.m_current = plasma::Attribute::STORED_ELEMENT;
    m_shape->m_color.copyFromCurrent(plasma::Attribute::BASE_ELEMENT);
    m_shape->m_strokeColor.copyFromCurrent(plasma::Attribute::BASE_ELEMENT);
    m_shape->m_extrusionColor.copyFromCurrent(plasma::Attribute::BASE_ELEMENT);
    m_shape->m_color.m_current = plasma::Attribute::BASE_ELEMENT;
    m_shape->m_strokeColor.m_current = plasma::Attribute::BASE_ELEMENT;
    m_shape->m_extrusionColor.m_current = plasma::Attribute::BASE_ELEMENT;
    m_colorPreview = false;
}

void TextInterface::getColorsUnderMouse(std::list<Vector<4, float>> &colors, int colorTarget)
{
    Vector<2, float> point = m_node->getNode()->m_inverseMatrix.transform(m_sheet->m_workspace->m_sceneMousePosition);
    if (m_shape->contains(point))
        colors.push_back(getColor(colorTarget));
}

void TextInterface::recordColorEdit()
{
    TextState after;
    captureState(after);
    TextState before(after);
    before.m_color.copyToCurrent(plasma::Attribute::STORED_ELEMENT);
    before.m_strokeColor.copyToCurrent(plasma::Attribute::STORED_ELEMENT);
    before.m_extrusionColor.copyToCurrent(plasma::Attribute::STORED_ELEMENT);
    m_sheet->addOperation(new TextOperation(m_sheet, "modify text", this, before, after));
}

void TextInterface::adjustBrightness(float factor, int colorTarget, bool update)
{
    Vector<4, float> &color = getColor(colorTarget);
    Vector<4, float> scale(factor, factor, factor, 1.0f); // The alpha is kept.
    for (int i = 0; i < RGBA_COMPONENT_COUNT; ++i)
        color.m_elements[i] *= scale.m_elements[i];
    for (int i = 0; i < RGB_COMPONENT_COUNT; ++i) {
        if (color.m_elements[i] < 0.0f)
            color.m_elements[i] = 0.0f;
        if (color.m_elements[i] > 1.0f)
            color.m_elements[i] = 1.0f;
    }
    if (update)
        m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
}

void TextInterface::adjustContrast(float factor, int colorTarget, bool update)
{
    Vector<4, float> &color = getColor(colorTarget);
    float alpha = color.m_elements[RGBA_ALPHA];
    color = color * factor + Vector<4, float>(0.5f, 0.5f, 0.5f, 0.0f) * (1.0f - factor);
    color.m_elements[RGBA_ALPHA] = alpha;
    for (int i = 0; i < RGB_COMPONENT_COUNT; ++i) {
        if (color.m_elements[i] < 0.0f)
            color.m_elements[i] = 0.0f;
        if (color.m_elements[i] > 1.0f)
            color.m_elements[i] = 1.0f;
    }
    if (update)
        m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
}

// RGB luminance accumulates green * 0.59, red * 0.3 and blue * 0.11.
void TextInterface::adjustSaturation(float factor, int colorTarget, bool update)
{
    Vector<4, float> &color = getColor(colorTarget);
    Vector<3, float> rgb(color.m_elements[0], color.m_elements[1], color.m_elements[2]);
    float luminance = rgb.m_elements[1] * 0.59f + rgb.m_elements[0] * 0.3f + rgb.m_elements[2] * 0.11f;
    float alpha = color.m_elements[RGBA_ALPHA];
    color = color * factor + Vector<4, float>(luminance, luminance, luminance, 0.0f) * (1.0f - factor);
    color.m_elements[RGBA_ALPHA] = alpha;
    for (int i = 0; i < RGB_COMPONENT_COUNT; ++i) {
        if (color.m_elements[i] < 0.0f)
            color.m_elements[i] = 0.0f;
        if (color.m_elements[i] > 1.0f)
            color.m_elements[i] = 1.0f;
    }
    if (update)
        m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
}

// The colour shifted towards the target by the source colour nearest to it in hue (the colour tool's shift,
// MeshInterface::ShiftColors and here): the hue moves to the target's as the hue distance falls below the
// tolerance, saturation and value move by the target's offset from that source colour.
Vector<4, float> shiftColor(const std::list<Vector<4, float>> &source, const Vector<4, float> &target, float tolerance,
                            const Vector<4, float> &color)
{
    Vector<4, float> targetHsv = rgbToHsv(target);
    Vector<4, float> hsv = rgbToHsv(color);
    float distance = 10.0f;
    Vector<4, float> shift(0.0f, 0.0f, 0.0f, 0.0f);
    for (std::list<Vector<4, float>>::const_iterator it = source.begin(); it != source.end(); ++it) {
        Vector<4, float> sourceHsv = rgbToHsv(*it);
        float hueDistance = std::fabs((float)(sourceHsv.m_elements[0] - hsv.m_elements[0])) / 360.0;
        if (hueDistance <= distance) {
            distance = hueDistance;
            shift = targetHsv - sourceHsv;
        }
        hueDistance = std::fabs((float)(sourceHsv.m_elements[0] - hsv.m_elements[0] + 360.0)) / 360.0;
        if (hueDistance <= distance) {
            distance = hueDistance;
            shift = targetHsv - sourceHsv;
        }
    }
    float weight = distance / tolerance;
    weight = weight * weight;
    if (weight > 1.0f)
        weight = 1.0f;
    hsv.m_elements[0] = weight * hsv.m_elements[0] + targetHsv.m_elements[0] * (1.0f - weight);
    hsv.m_elements[1] += shift.m_elements[1] * (1.0f - weight);
    hsv.m_elements[2] += (1.0f - weight) * shift.m_elements[2];
    if (hsv.m_elements[1] > 100.0)
        hsv.m_elements[1] = 100.0f;
    else if (hsv.m_elements[1] < 0.0f)
        hsv.m_elements[1] = 0.0f;
    if (hsv.m_elements[2] > 100.0f)
        hsv.m_elements[2] = 100.0f;
    else if (hsv.m_elements[2] < 0.0f)
        hsv.m_elements[2] = 0.0f;
    return hsvToRgb(hsv);
}

// Shifts colour 'colorTarget' (0: fill, 1: stroke, 2: extrusion) towards the target as the colour tool paints:
// Shift keeps the alpha, Ctrl changes only the alpha when the tool edits both.
void TextInterface::shiftColors(const std::list<Vector<4, float>> &source, const Vector<4, float> &target,
                                float tolerance, int colorTarget, bool update)
{
    // 'attribute' is left unset for 'colorTarget' outside 0..2.
    plasma::ContinuousAttribute<Vector<4, float>> *attribute;
    switch (colorTarget) {
    case COLOR_TARGET_FILL:
        attribute = &m_shape->m_color;
        break;
    case COLOR_TARGET_STROKE:
        attribute = &m_shape->m_strokeColor;
        break;
    case COLOR_TARGET_EXTRUSION:
        attribute = &m_shape->m_extrusionColor;
        break;
    }
    Vector<4, float> shifted = shiftColor(source, target, tolerance, attribute->value());
    if (!m_sheet->m_workspace->m_engine.isKeyDown(VK_SHIFT) && m_sheet->m_workspace->m_colorToolModule->m_alpha) {
        if (!m_sheet->m_workspace->m_engine.isKeyDown(VK_CONTROL) && m_sheet->m_workspace->m_colorToolModule->m_color)
            attribute->value() = shifted;
        else
            attribute->value().m_elements[RGBA_ALPHA] = shifted.m_elements[RGBA_ALPHA];
    } else {
        attribute->value().m_elements[0] = shifted.m_elements[0];
        attribute->value().m_elements[1] = shifted.m_elements[1];
        attribute->value().m_elements[2] = shifted.m_elements[2];
    }
    m_hasChanges = true;
    if (update)
        m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
}

// The offsets of the text's bounds corners from the origin, on the sheet (the node tool's drag start).
void TextInterface::getSnapOffsets(const Vector<2, float> &origin, std::list<Vector<2, float>> &offsets)
{
    Vector<2, float> minimum, maximum;
    getBounds(&minimum, &maximum);
    offsets.push_back(m_sheet->getRoot()->m_inverseMatrix.transform(minimum) - origin);
    offsets.push_back(m_sheet->getRoot()->m_inverseMatrix.transform(maximum) - origin);
}

// Paints colour 'colorTarget' with the colour module's colour if the mouse is over the text (Alt picks it up
// instead; Shift keeps the alpha, Ctrl changes only the alpha).
void TextInterface::paintColor(int colorTarget, bool update)
{
    Workspace *workspace = m_sheet->m_workspace;
    if (m_shape->contains(m_node->getNode()->m_inverseMatrix.transform(workspace->m_sceneMousePosition))) {
        Vector<4, float> &color = getColor(colorTarget);
        if (m_sheet->m_workspace->m_engine.isKeyDown(VK_MENU)) {
            m_sheet->m_workspace->getColorModule()->setColor(color, true, true, false);
        } else if (m_sheet->m_workspace->m_engine.isKeyDown(VK_SHIFT)) {
            color.m_elements[0] = m_sheet->m_workspace->getColorModule()->getColor().m_elements[0];
            color.m_elements[1] = m_sheet->m_workspace->getColorModule()->getColor().m_elements[1];
            color.m_elements[2] = m_sheet->m_workspace->getColorModule()->getColor().m_elements[2];
        } else if (m_sheet->m_workspace->m_engine.isKeyDown(VK_CONTROL)) {
            color.m_elements[RGBA_ALPHA] = m_sheet->m_workspace->getColorModule()->getColor().m_elements[RGBA_ALPHA];
        } else {
            color = m_sheet->m_workspace->getColorModule()->getColor();
        }
        m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        m_hasChanges = true;
    }
}

// The colour the colour module edits and the centre of the text's bounds, if the mouse is over the text (the
// colour tool's pick).
bool TextInterface::pickColor(Vector<4, float> &color, Vector<2, float> &position)
{
    int colorTarget = m_sheet->m_workspace->m_colorModule->getMode();
    if (m_shape->contains(m_node->m_node->m_inverseMatrix.transform(m_sheet->m_workspace->m_sceneMousePosition))) {
        color = getColor(colorTarget);
        Vector<2, float> minimum, maximum;
        getBounds(&minimum, &maximum);
        position = (minimum + maximum) * 0.5f;
        return true;
    }
    return false;
}

// The stroke width on the sheet and the centre of the text's bounds, if the mouse is over the text (the stroke
// width tool's pick).
// The stroke width tool's functions work at the smaller scale of the node's axes on the sheet.
bool TextInterface::pickStrokeWidth(float &width, Vector<2, float> &position)
{
    if (!m_shape->contains(m_node->m_node->m_inverseMatrix.transform(m_sheet->m_workspace->m_sceneMousePosition)))
        return false;
    plasma::Matrix<float> matrix = m_sheet->m_root->m_inverseMatrix;
    matrix *= m_node->m_node->m_matrix;
    Vector<2, float> xAxis = matrix.transformDirection(Vector<2, float>(1.0f, 0.0f));
    Vector<2, float> yAxis = matrix.transformDirection(Vector<2, float>(0.0f, 1.0f));
    float xScale = std::sqrt(xAxis.m_elements[0] * xAxis.m_elements[0] + xAxis.m_elements[1] * xAxis.m_elements[1]);
    float yScale = std::sqrt(yAxis.m_elements[0] * yAxis.m_elements[0] + yAxis.m_elements[1] * yAxis.m_elements[1]);
    float scale = xScale < yScale ? xScale : yScale;
    width = (m_shape->m_strokeRadius + m_shape->m_strokeRadius) * scale;
    Vector<2, float> minimum, maximum;
    getBounds(&minimum, &maximum);
    position = (minimum + maximum) * 0.5f;
    return true;
}

// Sets the stroke to the stroke width module's width if the mouse is over the text (the stroke width tool).
void TextInterface::paintStrokeWidth()
{
    if (m_shape->contains(m_node->m_node->m_inverseMatrix.transform(m_sheet->m_workspace->getSceneMousePosition()))) {
        plasma::Matrix<float> matrix = m_sheet->m_root->m_inverseMatrix;
        matrix *= m_node->m_node->m_matrix;
        Vector<2, float> xAxis = matrix.transformDirection(Vector<2, float>(1.0f, 0.0f));
        Vector<2, float> yAxis = matrix.transformDirection(Vector<2, float>(0.0f, 1.0f));
        float xScale = std::sqrt(xAxis.m_elements[0] * xAxis.m_elements[0] + xAxis.m_elements[1] * xAxis.m_elements[1]);
        float yScale = std::sqrt(yAxis.m_elements[0] * yAxis.m_elements[0] + yAxis.m_elements[1] * yAxis.m_elements[1]);
        float scale = xScale < yScale ? xScale : yScale;
        m_shape->m_strokeRadius = m_sheet->m_workspace->m_strokeWidthModule->m_width * 0.5 / scale;
        m_hasChanges = true;
    }
}

// Scales the stroke to the stroke width module's width when it lies outside the range, else sets it (the stroke
// width tool over a selection).
void TextInterface::scaleStrokeWidths(float minimum, float maximum)
{
    if (m_shape->contains(m_node->m_node->m_inverseMatrix.transform(m_sheet->m_workspace->m_sceneMousePosition))) {
        plasma::Matrix<float> matrix = m_sheet->m_root->m_inverseMatrix;
        matrix *= m_node->m_node->m_matrix;
        Vector<2, float> xAxis = matrix.transformDirection(Vector<2, float>(1.0f, 0.0f));
        Vector<2, float> yAxis = matrix.transformDirection(Vector<2, float>(0.0f, 1.0f));
        float xScale = std::sqrt(xAxis.m_elements[0] * xAxis.m_elements[0] + xAxis.m_elements[1] * xAxis.m_elements[1]);
        float yScale = std::sqrt(yAxis.m_elements[0] * yAxis.m_elements[0] + yAxis.m_elements[1] * yAxis.m_elements[1]);
        float scale = xScale < yScale ? xScale : yScale;
        float width = m_sheet->m_workspace->m_strokeWidthModule->m_width / scale;
        minimum /= scale;
        maximum /= scale;
        float strokeWidth = m_shape->m_strokeRadius + m_shape->m_strokeRadius;
        if (strokeWidth < minimum)
            m_shape->m_strokeRadius = width / minimum * m_shape->m_strokeRadius;
        else if (strokeWidth > maximum)
            m_shape->m_strokeRadius = width / maximum * m_shape->m_strokeRadius;
        else
            m_shape->m_strokeRadius = width * 0.5f;
    }
}

// Widens the range to the stroke width on the sheet if the mouse is over the text.
bool TextInterface::getStrokeWidthRange(float &minimum, float &maximum)
{
    if (!m_shape->contains(m_node->m_node->m_inverseMatrix.transform(m_sheet->m_workspace->m_sceneMousePosition)))
        return false;
    plasma::Matrix<float> matrix = m_sheet->m_root->m_inverseMatrix;
    matrix *= m_node->m_node->m_matrix;
    Vector<2, float> xAxis = matrix.transformDirection(Vector<2, float>(1.0f, 0.0f));
    Vector<2, float> yAxis = matrix.transformDirection(Vector<2, float>(0.0f, 1.0f));
    float xScale = std::sqrt(xAxis.m_elements[0] * xAxis.m_elements[0] + xAxis.m_elements[1] * xAxis.m_elements[1]);
    float yScale = std::sqrt(yAxis.m_elements[0] * yAxis.m_elements[0] + yAxis.m_elements[1] * yAxis.m_elements[1]);
    float scale = xScale < yScale ? xScale : yScale;
    float width = (m_shape->m_strokeRadius + m_shape->m_strokeRadius) * scale;
    minimum = minimum < width ? minimum : width;
    maximum = width < maximum ? maximum : width;
    return true;
}

// The text's bounds on the sheet: the laid-out text's box (the shape's own bounds with the node tool's
// "boundary:shape"), wrapped text at least as wide as the wrap width, through the node's matrix. Without a font,
// the node's origin. Empty bounds start with positive minima and negative maxima.
void TextInterface::getBounds(Vector<2, float> *minimum, Vector<2, float> *maximum)
{
    if (!m_shape->m_font) {
        *minimum = m_node->getNode()->m_matrix * Vector<2, float>(0.0f, 0.0f);
        *maximum = m_node->getNode()->m_matrix * Vector<2, float>(0.0f, 0.0f);
        return;
    }
    Vector<2, float> low, high;
    Workspace *workspace = m_sheet->m_workspace;
    if (workspace->getNodeToolModule()->m_shapeBoundary) {
        low = m_shape->getMinimum();
        high = m_shape->getMaximum();
    } else {
        plasma::Matrix<float> identity;
        identity.identity();
        workspace->m_engine.m_fontEngine->lock();
        m_shape->m_font->prepare(identity, m_shape->m_pixelSize, m_shape->m_strokeRadius, m_shape->m_spacing,
                                 m_shape->m_lineSpacing, false);
        if (m_shape->m_flags & (1 << plasma::TEXT_WRAP)) {
            std::wstring text = m_shape->m_string.value();
            m_shape->m_font->wrapText(text, m_shape->m_wrapWidth);
            m_shape->m_font->getBounds(text, low, high, m_shape->m_flags, -1, true);
        } else {
            m_shape->m_font->getBounds(m_shape->m_string.value(), low, high, m_shape->m_flags, -1, true);
        }
        m_sheet->m_workspace->m_engine.m_fontEngine->unlock();
        if (m_shape->m_flags & (1 << plasma::TEXT_WRAP) &&
            high.m_elements[0] < m_shape->m_wrapWidth + low.m_elements[0])
            high.m_elements[0] = m_shape->m_wrapWidth + low.m_elements[0];
    }
    Vector<2, float> corners[4];
    corners[0] = low;
    corners[1] = Vector<2, float>(high.m_elements[0], low.m_elements[1]);
    corners[2] = high;
    corners[3] = Vector<2, float>(low.m_elements[0], high.m_elements[1]);
    minimum->m_elements[0] = 1e10f;
    minimum->m_elements[1] = 1e10f;
    maximum->m_elements[0] = -1e10f;
    maximum->m_elements[1] = -1e10f;
    const plasma::Matrix<float> &matrix = m_node->getNode()->m_matrix;
    for (int i = 0; i < 4; i++) {
        Vector<2, float> corner = matrix.transform(corners[i]);
        if (!i || minimum->m_elements[0] > corner.m_elements[0])
            minimum->m_elements[0] = corner.m_elements[0];
        if (!i || minimum->m_elements[1] > corner.m_elements[1])
            minimum->m_elements[1] = corner.m_elements[1];
        if (!i || maximum->m_elements[0] < corner.m_elements[0])
            maximum->m_elements[0] = corner.m_elements[0];
        if (!i || maximum->m_elements[1] < corner.m_elements[1])
            maximum->m_elements[1] = corner.m_elements[1];
    }
}
