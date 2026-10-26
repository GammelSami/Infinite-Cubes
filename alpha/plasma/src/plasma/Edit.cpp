// Edit implements a single-line editor with a caret and signed selection length.
#include "plasma/Edit.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Display.h"
#include "plasma/Transformation.h"
#include "plasma/ScalableFont.h"
#include "plasma/FontEngine.h"
#include <algorithm>
#include <cstdlib>

namespace plasma {
// The caret (anonymous: this file's own): its height in text sizes, the characters measured past
// it when the text scrolls, and the share of a character's width left of which a click puts the
// caret before it.
namespace {
enum {
    CaretScrollLookahead = 6
};
}

Edit::Edit(Engine *engine, Node *node, const std::wstring &name)
    : Widget(engine, node, name),
      m_textShape(0),
      m_filter(0),
      m_caret(0),
      m_selectionLength(0)
{
    m_kind = WIDGET_EDIT;
}

Edit::Edit(const Edit &other, Node *node)
    : Widget(other, node),
      m_textShape(0),
      m_filter(other.m_filter),
      m_caret(0),
      m_selectionLength(0)
{
    m_kind = WIDGET_EDIT;
}

Edit::~Edit() {}

Widget *Edit::clone(Node *node)
{
    return new Edit(*this, node);
}

// Deletes the selection, or the character after the caret when nothing is selected.
void Edit::deleteSelection()
{
    clampSelection();
    if (m_caret <= static_cast<int>(m_textShape->m_string.value().size())) {
        unsigned start = m_caret;
        int count = m_selectionLength;
        if (!count) {
            count = 1;
        } else if (count < 0) {
            start += count;
            count = -count;
        }
        m_textShape->m_string.value().erase(start, count);
        if (m_selectionLength < 0)
            m_caret += m_selectionLength;
        m_selectionLength = 0;
        m_textShape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
        sendEvent(WIDGET_EVENT_TEXT_CHANGED);
        ensureCaretVisible();
    }
}

void Edit::draw()
{
    if (m_engine->getFocusWidget() == this && m_textShape->m_font) {
        m_engine->getFontEngine()->lock();
        m_textShape->m_font->setSize(m_textShape->m_pixelSize, 0.0f);
        int first = m_caret;
        int last = first + m_selectionLength;
        if (last < first)
            std::swap(first, last);
        if (first < 0)
            first = 0;
        if (last > (int)m_textShape->m_string.value().size())
            last = (int)m_textShape->m_string.value().size();
        Vector<2, float> position;
        Vector<2, float> extent;
        m_textShape->m_font->getCaret(m_textShape->m_string.value(), first, m_node->m_matrix, m_textShape->m_pixelSize,
                                      m_textShape->m_strokeRadius, m_textShape->m_spacing, m_textShape->m_lineSpacing,
                                      m_textShape->m_wrapWidth, position, extent, m_textShape->m_flags,
                                      m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
        float startX = position.m_elements[0];
        m_textShape->m_font->getCaret(m_textShape->m_string.value(), last, m_node->m_matrix, m_textShape->m_pixelSize,
                                      m_textShape->m_strokeRadius, m_textShape->m_spacing, m_textShape->m_lineSpacing,
                                      m_textShape->m_wrapWidth, position, extent, m_textShape->m_flags,
                                      m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
        float endX = position.m_elements[0];
        position.m_elements[0] = startX;
        position.m_elements[1] = -m_textShape->m_pixelSize;
        extent.m_elements[0] = endX - startX;
        extent.m_elements[1] = m_textShape->m_pixelSize * 1.25f;
        m_engine->invertRectangle(position, extent);
        if (m_caret >= 0 && m_caret <= static_cast<int>(m_textShape->m_string.value().size())) {
            Vector<2, float> caretPosition;
            Vector<2, float> caretExtent;
            m_textShape->m_font->getCaret(m_textShape->m_string.value(), m_caret, m_node->m_matrix,
                                          m_textShape->m_pixelSize, m_textShape->m_strokeRadius, m_textShape->m_spacing,
                                          m_textShape->m_lineSpacing, m_textShape->m_wrapWidth, caretPosition,
                                          caretExtent, m_textShape->m_flags,
                                          m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
            caretPosition.m_elements[1] = -m_textShape->m_pixelSize;
            caretExtent.m_elements[0] = 1.0f;
            caretExtent.m_elements[1] = m_textShape->m_pixelSize * 1.25f;
            m_engine->drawCaret(caretPosition, caretExtent, Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f));
        }
        m_engine->getFontEngine()->unlock();
    }
}

// Digits with at most one decimal separator ('.' or ','), optionally after a leading minus sign.
bool FloatFilter::validate(const std::wstring &text)
{
    int length = (int)text.size();
    if (length == 0)
        return true;
    int i = 0;
    if (text[0] == L'-')
        i = 1;
    bool separator = false;
    for (; i < length; i++) {
        if (text[i] == L'.' || text[i] == L',') {
            if (separator)
                return false;
            separator = true;
        } else if (text[i] < L'0' || text[i] > L'9')
            return false;
    }
    return true;
}

// Digits, optionally after a leading minus sign.
bool IntegerFilter::validate(const std::wstring &text)
{
    int length = (int)text.size();
    if (length == 0)
        return true;
    int i = 0;
    if (text[0] == L'-')
        i = 1;
    for (; i < length; i++)
        if (text[i] < L'0' || text[i] > L'9')
            return false;
    return true;
}

// Digits with at most one decimal separator ('.' or ',').
bool UnsignedFloatFilter::validate(const std::wstring &text)
{
    int length = (int)text.size();
    if (length == 0)
        return true;
    bool separator = false;
    for (int i = 0; i < length; i++) {
        if (text[i] == L'.' || text[i] == L',') {
            if (separator)
                return false;
            separator = true;
        } else if (text[i] < L'0' || text[i] > L'9')
            return false;
    }
    return true;
}

// Digits only.
bool UnsignedIntegerFilter::validate(const std::wstring &text)
{
    int length = (int)text.size();
    if (length == 0)
        return true;
    for (int i = 0; i < length; i++)
        if (text[i] < L'0' || text[i] > L'9')
            return false;
    return true;
}

void Edit::initialize()
{
    Shape *shape = m_node->m_shape;
    m_textShape = shape && shape->getType() == SHAPE_TEXT ? static_cast<TextShape *>(shape) : 0;
    clampSelection();
    if (m_node && m_node->m_parent && m_node->m_parent->m_shape)
        m_node->m_parent->m_display->m_clipping.value() = DISPLAY_CLIPPING_ENABLED;
    m_initialTranslation = m_node->m_transformation->m_translation.value();
}

void Edit::characterEntered(int character)
{
    wchar_t value = static_cast<wchar_t>(character);
    if (m_textShape && value >= L' ') {
        m_changed = true;
        std::wstring inserted(L"");
        inserted += value;
        if (m_selectionLength) {
            if (m_filter) {
                bool rejected;
                {
                    wchar_t oneCharacter[] = {value, 0};
                    std::wstring validated(oneCharacter);
                    rejected = !m_filter->validate(validated);
                }
                if (rejected)
                    return;
            }
            deleteSelection();
        }
        std::wstring next = m_textShape->m_string.value();
        clampSelection();
        next.insert(m_caret, inserted, 0, std::wstring::npos);
        if (m_filter && !m_filter->validate(next))
            return;
        m_node->setText(next);
        ++m_caret;
        ensureCaretVisible();
        sendEvent(WIDGET_EVENT_TEXT_CHANGED);
    }
}

void Edit::keyPressed(unsigned short key)
{
    if (!m_textShape)
        return;
    clampSelection();
    m_changed = true;
    if (key == VK_RETURN) {
        m_engine->setFocusWidget(0);
        ensureCaretVisible();
    } else if (key == VK_LEFT) {
        if (m_caret > 0) {
            --m_caret;
            if (m_engine->isKeyDown(VK_SHIFT))
                ++m_selectionLength;
            else
                m_selectionLength = 0;
        }
        ensureCaretVisible();
    } else if (key == VK_RIGHT) {
        if (m_caret < static_cast<int>(m_textShape->m_string.value().size())) {
            ++m_caret;
            if (m_engine->isKeyDown(VK_SHIFT))
                --m_selectionLength;
            else
                m_selectionLength = 0;
        }
        ensureCaretVisible();
    } else if (key == VK_DELETE) {
        deleteSelection();
        ensureCaretVisible();
    } else if (key == VK_BACK) {
        if (m_selectionLength) {
            deleteSelection();
            ensureCaretVisible();
        } else if (m_caret > 0) {
            m_textShape->m_string.value().erase(m_textShape->m_string.value().begin() + m_caret - 1);
            --m_caret;
            m_textShape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
            ensureCaretVisible();
            sendEvent(WIDGET_EVENT_TEXT_CHANGED);
        }
    } else if (key == VK_HOME) {
        if (m_engine->isKeyDown(VK_SHIFT))
            m_selectionLength += m_caret;
        else
            m_selectionLength = 0;
        m_caret = 0;
        ensureCaretVisible();
    } else if (key == VK_END) {
        if (m_engine->isKeyDown(VK_SHIFT))
            m_selectionLength += (int)(m_caret - m_textShape->m_string.value().size());
        else
            m_selectionLength = 0;
        m_caret = (int)m_textShape->m_string.value().size();
        ensureCaretVisible();
    }
}

void Edit::pointerDoubleClicked()
{
    if (m_textShape) {
        m_caret = (int)m_textShape->m_string.value().size();
        m_selectionLength = -static_cast<int>(m_textShape->m_string.value().size());
        ensureCaretVisible();
    }
}

void Edit::pointerPressed()
{
    if (m_textShape) {
        m_selecting = false;
        Vector<2, float> position = transformPoint(m_node->m_inverseMatrix, m_engine->getPointer());
        if (m_engine->getFocusWidget() != this) {
            m_caret = (int)m_textShape->m_string.value().size();
            m_selectionLength = -static_cast<int>(m_textShape->m_string.value().size());
            m_engine->setFocusWidget(this);
        } else {
            if (m_engine->isKeyDown(VK_SHIFT)) {
                int caret = m_caret;
                setCaretFromPoint(position);
                m_selectionLength -= caret + m_caret;
            } else {
                setCaretFromPoint(position);
                m_selectionLength = 0;
            }
            m_selecting = true;
        }
        clampSelection();
        ensureCaretVisible();
        m_engine->setPointerCapture(m_node);
    }
}

void Edit::pointerReleased()
{
    m_engine->releasePointerCapture();
}

void Edit::pointerMoved()
{
    if ((m_engine->m_buttons & (1 << MOUSE_BUTTON_LEFT)) && m_engine->getFocusWidget() == this && m_selecting) {
        int caret = m_caret;
        setCaretFromPoint(transformPoint(m_node->m_inverseMatrix, m_engine->getPointer()));
        m_selectionLength += caret - m_caret;
        clampSelection();
        ensureCaretVisible();
    }
}

void Edit::ensureCaretVisible()
{
    if (m_textShape && m_node->m_parent) {
        Vector<2, float> parentMinimum = m_node->m_parent->getMinimum((1 << BOUNDS_SKIP_WIDGETS));
        Vector<2, float> parentMaximum = m_node->m_parent->getMaximum((1 << BOUNDS_SKIP_WIDGETS));
        m_engine->getFontEngine()->lock();
        Vector<2, float> position;
        Vector<2, float> extent;
        m_textShape->m_font->getCaret(m_textShape->m_string.value(), m_caret, m_node->m_matrix,
                                      m_textShape->m_pixelSize, m_textShape->m_strokeRadius, m_textShape->m_spacing,
                                      m_textShape->m_lineSpacing, m_textShape->m_wrapWidth, position, extent,
                                      m_textShape->m_flags, m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
        Matrix<float> matrix = m_node->m_parent->m_transformation->m_matrix;
        matrix *= m_node->m_transformation->m_matrix;
        position = matrix.transform(position);
        matrix.transformDirectionInPlace(extent);
        if (parentMaximum.m_elements[0] <= extent.m_elements[0] + position.m_elements[0]) {
            m_textShape->m_font->getCaret(m_textShape->m_string.value() + L" ", m_caret + CaretScrollLookahead,
                                          m_node->m_matrix, m_textShape->m_pixelSize, m_textShape->m_strokeRadius,
                                          m_textShape->m_spacing, m_textShape->m_lineSpacing, m_textShape->m_wrapWidth,
                                          position, extent, m_textShape->m_flags,
                                          m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
            position = matrix.transform(position);
            int delta = static_cast<int>(position.m_elements[0] - parentMaximum.m_elements[0]);
            m_node->m_transformation->m_translation.value().m_elements[0] -= delta;
        } else {
            // Also leaves the translation unchanged for an unordered comparison.
            if (!(parentMinimum.m_elements[0] > position.m_elements[0])) {
                m_engine->getFontEngine()->unlock();
                return;
            }
            m_textShape->m_font->getCaret(m_textShape->m_string.value() + L" ", m_caret - 6, m_node->m_matrix,
                                          m_textShape->m_pixelSize, m_textShape->m_strokeRadius, m_textShape->m_spacing,
                                          m_textShape->m_lineSpacing, m_textShape->m_wrapWidth, position, extent,
                                          m_textShape->m_flags, m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
            position = matrix.transform(position);
            int delta = static_cast<int>(position.m_elements[0] - parentMinimum.m_elements[0]);
            m_node->m_transformation->m_translation.value().m_elements[0] -= delta;
        }
        if (m_caret - 6 <= 0 ||
            m_node->m_transformation->m_translation.value().m_elements[0] > m_initialTranslation.m_elements[0])
            m_node->m_transformation->m_translation.value().m_elements[0] = m_initialTranslation.m_elements[0];
        m_node->m_transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
        m_engine->getFontEngine()->unlock();
    }
}

void Edit::setCaretFromPoint(const Vector<2, float> &point)
{
    std::wstring text = m_textShape->m_string.value();
    m_caret = 0;
    int size = (int)text.size();
    for (int i = 0; i < size; ++i) {
        m_engine->getFontEngine()->lock();
        Vector<2, float> position;
        Vector<2, float> extent;
        m_textShape->m_font->getCaret(text, i, m_node->m_matrix, m_textShape->m_pixelSize, m_textShape->m_strokeRadius,
                                      m_textShape->m_spacing, m_textShape->m_lineSpacing, m_textShape->m_wrapWidth,
                                      position, extent, m_textShape->m_flags,
                                      m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
        m_engine->getFontEngine()->unlock();
        float boundary = extent[0] * 0.7f + position[0];
        if (point[0] > boundary)
            m_caret = i + 1;
    }
    clampSelection();
    m_changed = true;
}

void Edit::clampSelection()
{
    if (!m_textShape) {
        m_caret = 0;
        m_selectionLength = 0;
        return;
    }
    if (m_caret < 0)
        m_caret = 0;
    if (m_caret > static_cast<int>(m_textShape->m_string.value().size()))
        m_caret = (int)m_textShape->m_string.value().size();
    int caret = m_caret;
    if (caret + m_selectionLength < 0)
        m_selectionLength = -caret;
    int length = m_selectionLength;
    if (length + caret > static_cast<int>(m_textShape->m_string.value().size()))
        m_caret = (int)(m_textShape->m_string.value().size() - length);
}

}
