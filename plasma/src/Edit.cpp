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

static const int CaretScrollLookaheadCharacters = 6;

// Digits, optionally after a leading minus sign.
bool IntegerFilter::validate(const std::wstring &text)
{
    int length = text.size();
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

// Digits only.
bool UnsignedIntegerFilter::validate(const std::wstring &text)
{
    int length = text.size();
    if (length == 0)
        return true;
    for (int i = 0; i < length; i++)
        if (text[i] < L'0' || text[i] > L'9')
            return false;
    return true;
}

// Digits with at most one decimal separator ('.' or ','), optionally after a leading minus sign.
bool FloatFilter::validate(const std::wstring &text)
{
    int length = text.size();
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

// Digits with at most one decimal separator ('.' or ',').
bool UnsignedFloatFilter::validate(const std::wstring &text)
{
    int length = text.size();
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

void Edit::initialize()
{
    Shape *shape = m_node->m_shape;
    m_textShape = shape && shape->getType() == SHAPE_TEXT ? static_cast<TextShape *>(shape) : 0;
    clampSelection();
    if (m_node && m_node->m_parent && m_node->m_parent->m_shape)
        setDisplayClipping(m_node->m_parent->m_display, 1);
    m_initialTranslation = getTranslation(m_node->m_transformation);
}

std::wstring &Edit::getText()
{
    if (!m_textShape) {
        static std::wstring empty;
        return empty;
    }
    return m_textShape->m_string.value();
}

void Edit::insertText(const std::wstring &text)
{
    if (m_textShape) {
        std::wstring previous = m_textShape->m_string.value();
        if (m_selectionLength)
            deleteSelection();
        std::wstring next = m_textShape->m_string.value();
        clampSelection();

        // The code subtracts one from find's result. npos - 1 still copies all text; a newline skips the preceding
        // character, and zero wraps.
        next.insert(m_caret, text.substr(0, text.find(L'\n') - 1), 0, std::wstring::npos);
        if (m_filter && !m_filter->validate(next)) {
            m_textShape->m_string = previous;
            m_textShape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
            return;
        }
        m_caret += text.size();
        m_textShape->m_string.value().assign(next);
        m_textShape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
        notifyTextChanged();
    }
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
        m_caret = m_textShape->m_string.value().size();
    int caret = m_caret;
    if (caret + m_selectionLength < 0)
        m_selectionLength = -caret;
    int length = m_selectionLength;
    if (length + caret > static_cast<int>(m_textShape->m_string.value().size()))
        m_caret = m_textShape->m_string.value().size() - length;
}

std::wstring Edit::getSelectedText()
{
    int end = 0;
    if (!m_selectionLength)
        return std::wstring(L"");
    end = m_caret + m_selectionLength;
    return m_textShape->m_string.value().substr(std::min<int>(m_caret, end), std::abs(m_selectionLength));
}

void Edit::selectAll()
{
    if (m_textShape) {
        m_caret = m_textShape->m_string.value().size();
        m_selectionLength = -static_cast<int>(m_textShape->m_string.value().size());
        ensureCaretVisible();
    }
}

void Edit::pointerPressed()
{
    if (m_textShape) {
        m_selecting = false;
        Vector<2, float> position = m_node->m_inverseMatrix.transform(Vector<2, float>(m_engine->m_pointer));
        if (m_engine->getFocusWidget() != this) {
            m_caret = m_textShape->m_string.value().size();
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

void Edit::setCaretFromPoint(const Vector<2, float> &point)
{
    std::wstring text = m_textShape->m_string.value();
    m_caret = 0;
    int size = text.size();
    for (int i = 0; i < size; ++i) {
        m_engine->getFontEngine()->lock();
        Vector<2, float> position;
        Vector<2, float> extent;
        m_textShape->m_font->getCaret(m_node->m_matrix, text, i, m_textShape->m_pixelSize, m_textShape->m_strokeRadius,
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

void Edit::pointerDoubleClicked()
{
    if (m_textShape) {
        m_caret = m_textShape->m_string.value().size();
        m_selectionLength = -static_cast<int>(m_textShape->m_string.value().size());
        ensureCaretVisible();
    }
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
        Vector<2, float> parentMinimum = m_node->m_parent->getMinimum(1 << BOUNDS_SKIP_WIDGETS);
        Vector<2, float> parentMaximum = m_node->m_parent->getMaximum(1 << BOUNDS_SKIP_WIDGETS);
        m_engine->getFontEngine()->lock();
        Vector<2, float> position;
        Vector<2, float> extent;
        m_textShape->m_font->getCaret(m_node->m_matrix, m_textShape->m_string.value(), m_caret,
                                      m_textShape->m_pixelSize, m_textShape->m_strokeRadius, m_textShape->m_spacing,
                                      m_textShape->m_lineSpacing, m_textShape->m_wrapWidth, position, extent,
                                      m_textShape->m_flags, m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
        Matrix<float> matrix = getPlacement(m_node->m_parent->m_transformation);
        matrix *= getPlacement(m_node->m_transformation);
        position.transform(matrix);
        matrix.transformDirectionInPlace(extent);
        if (parentMaximum.m_elements[0] <= extent.m_elements[0] + position.m_elements[0]) {
            m_textShape->m_font->getCaret(m_node->m_matrix, m_textShape->m_string.value() + L" ",
                                          m_caret + CaretScrollLookaheadCharacters, m_textShape->m_pixelSize,
                                          m_textShape->m_strokeRadius, m_textShape->m_spacing,
                                          m_textShape->m_lineSpacing, m_textShape->m_wrapWidth, position, extent,
                                          m_textShape->m_flags, m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
            position.transform(matrix);
            int delta = static_cast<int>(position.m_elements[0] - parentMaximum.m_elements[0]);
            getTranslation(m_node->m_transformation).m_elements[0] -= delta;
        } else {
            // The code also leaves the translation unchanged for an unordered comparison.
            if (!(parentMinimum.m_elements[0] > position.m_elements[0])) {
                m_engine->getFontEngine()->unlock();
                return;
            }
            m_textShape->m_font->getCaret(m_node->m_matrix, m_textShape->m_string.value() + L" ", m_caret - 6,
                                          m_textShape->m_pixelSize, m_textShape->m_strokeRadius, m_textShape->m_spacing,
                                          m_textShape->m_lineSpacing, m_textShape->m_wrapWidth, position, extent,
                                          m_textShape->m_flags, m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
            position.transform(matrix);
            int delta = static_cast<int>(position.m_elements[0] - parentMinimum.m_elements[0]);
            getTranslation(m_node->m_transformation).m_elements[0] -= delta;
        }
        if (m_caret - 6 <= 0 ||
            getTranslation(m_node->m_transformation).m_elements[0] > m_initialTranslation.m_elements[0])
            getTranslation(m_node->m_transformation).m_elements[0] = m_initialTranslation.m_elements[0];
        m_node->m_transformation->attributesChanged(ATTRIBUTES_CHANGED_ALL);
        m_engine->getFontEngine()->unlock();
    }
}

// Deletes the selection, or the character after the caret when nothing is selected (the Delete key, KeyPressed).
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
        notifyTextChanged();
        ensureCaretVisible();
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
            notifyTextChanged();
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
            m_selectionLength += m_caret - m_textShape->m_string.value().size();
        else
            m_selectionLength = 0;
        m_caret = m_textShape->m_string.value().size();
        ensureCaretVisible();
    }
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
        m_textShape->m_string.value().assign(next);
        ++m_caret;
        m_textShape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
        ensureCaretVisible();
        notifyTextChanged();
    }
}

Widget *Edit::clone(Node *node)
{
    return new Edit(*this, node);
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
            last = m_textShape->m_string.value().size();
        Vector<2, float> position;
        Vector<2, float> extent;
        m_textShape->m_font->getCaret(m_node->m_matrix, m_textShape->m_string.value(), first, m_textShape->m_pixelSize,
                                      m_textShape->m_strokeRadius, m_textShape->m_spacing, m_textShape->m_lineSpacing,
                                      m_textShape->m_wrapWidth, position, extent, m_textShape->m_flags,
                                      m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
        float startX = position.m_elements[0];
        m_textShape->m_font->getCaret(m_node->m_matrix, m_textShape->m_string.value(), last, m_textShape->m_pixelSize,
                                      m_textShape->m_strokeRadius, m_textShape->m_spacing, m_textShape->m_lineSpacing,
                                      m_textShape->m_wrapWidth, position, extent, m_textShape->m_flags,
                                      m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
        float endX = position.m_elements[0];
        position = Vector<2, float>(startX, -m_textShape->m_pixelSize);
        extent = Vector<2, float>(endX - startX, m_textShape->m_pixelSize * 1.25);
        m_engine->invertRectangle(position, extent);
        if (m_caret >= 0 && m_caret <= static_cast<int>(m_textShape->m_string.value().size())) {
            m_textShape->m_font->getCaret(m_node->m_matrix, m_textShape->m_string.value(), m_caret,
                                          m_textShape->m_pixelSize, m_textShape->m_strokeRadius, m_textShape->m_spacing,
                                          m_textShape->m_lineSpacing, m_textShape->m_wrapWidth, position, extent,
                                          m_textShape->m_flags, m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
            position.m_elements[1] = -m_textShape->m_pixelSize;
            extent = Vector<2, float>(1.0f, m_textShape->m_pixelSize * 1.25);
            m_engine->drawCaret(position, extent, Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f));
        }
        m_engine->getFontEngine()->unlock();
    }
}

}
