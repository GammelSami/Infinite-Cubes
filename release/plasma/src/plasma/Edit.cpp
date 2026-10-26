#include "plasma/Edit.h"
#include "plasma/EditFilter.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Display.h"
#include "plasma/Transformation.h"
#include "plasma/TextShape.h"
#include "plasma/ScalableFont.h"
#include "plasma/FontEngine.h"
#include <windows.h>

namespace plasma {

static const float CaretHeightInEm = 1.25f;
static const int CaretScrollLookaheadCharacters = 6;
static const float CaretAdvanceHitFraction = 0.7f;

bool IntegerFilter::validate(const std::wstring &text)
{
    int length = text.size();
    if (length == 0)
        return true;
    for (int i = text[0] == L'-'; i < length; i++)
        if (text[i] < L'0' || text[i] > L'9')
            return false;
    return true;
}

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

bool FloatFilter::validate(const std::wstring &text)
{
    int length = text.size();
    if (length == 0)
        return true;
    int i = text[0] == L'-';
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
      m_textShape(nullptr),
      m_filter(nullptr),
      m_caret(0),
      m_selectionLength(0)
{
    m_kind = WIDGET_EDIT;
}

Edit::Edit(const Edit &other, Node *node)
    : Widget(other, node),
      m_textShape(nullptr),
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

void Edit::deleteSelection()
{
    clampSelection();
    if (m_caret <= static_cast<int>(m_textShape->m_string.value().size())) {
        int start = m_caret;
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
        m_textShape->attributesChanged(true);
        invokeConnection(WIDGET_EVENT_TEXT_CHANGED);
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
            last = m_textShape->m_string.value().size();
        Matrix<float> matrix;
        m_node->getMatrix(matrix);
        Vector<2, float> position;
        Vector<2, float> extent;
        m_textShape->m_font->getCaret(m_textShape->m_string.value(), first, matrix, m_textShape->m_pixelSize,
                                      m_textShape->m_strokeRadius, m_textShape->m_spacing, m_textShape->m_lineSpacing,
                                      m_textShape->m_wrapWidth, position, extent, m_textShape->m_flags,
                                      m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
        float startX = position.m_elements[0];
        m_textShape->m_font->getCaret(m_textShape->m_string.value(), last, matrix, m_textShape->m_pixelSize,
                                      m_textShape->m_strokeRadius, m_textShape->m_spacing, m_textShape->m_lineSpacing,
                                      m_textShape->m_wrapWidth, position, extent, m_textShape->m_flags,
                                      m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
        float endX = position.m_elements[0];
        position.m_elements[0] = startX;
        position.m_elements[1] = -m_textShape->m_pixelSize;
        extent.m_elements[0] = endX - startX;
        extent.m_elements[1] = m_textShape->m_pixelSize * CaretHeightInEm;
        m_engine->invertRectangle(position, extent);
        if (m_caret >= 0 && m_caret <= (int)m_textShape->m_string.value().size()) {
            Vector<2, float> caretPosition;
            Vector<2, float> caretExtent;
            m_textShape->m_font->getCaret(
                m_textShape->m_string.value(), m_caret, matrix, m_textShape->m_pixelSize, m_textShape->m_strokeRadius,
                m_textShape->m_spacing, m_textShape->m_lineSpacing, m_textShape->m_wrapWidth, caretPosition,
                caretExtent, m_textShape->m_flags, m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
            caretPosition.m_elements[1] = -m_textShape->m_pixelSize;
            caretExtent.m_elements[0] = 1.0f;
            caretExtent.m_elements[1] = m_textShape->m_pixelSize * CaretHeightInEm;
            m_engine->drawCaret(caretPosition, caretExtent, Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f));
        }
        m_engine->getFontEngine()->unlock();
    }
}

void Edit::initialize()
{
    Shape *shape = m_node->m_shape;
    m_textShape = shape && shape->getType() == SHAPE_TEXT ? static_cast<TextShape *>(shape) : nullptr;
    clampSelection();
    if (m_node && m_node->m_parent && m_node->m_parent->m_shape)
        m_node->m_parent->m_display->m_clipping.value() = DISPLAY_CLIPPING_ENABLED;
    m_initialTranslation = m_node->m_transformation->m_translation.value();
}

void Edit::characterEntered(wchar_t character)
{
    if (m_textShape && character >= L' ') {
        m_changed = true;
        std::wstring inserted(L"");
        inserted += character;
        if (m_selectionLength) {
            if (m_filter) {
                bool rejected;
                {
                    wchar_t oneCharacter[] = {character, 0};
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
        invokeConnection(WIDGET_EVENT_TEXT_CHANGED);
    }
}

void Edit::keyPressed(unsigned short key)
{
    if (!m_textShape)
        return;
    clampSelection();
    m_changed = true;
    if (key == VK_RETURN) {
        m_engine->setFocusWidget(nullptr);
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
            m_textShape->attributesChanged(true);
            ensureCaretVisible();
            invokeConnection(WIDGET_EVENT_TEXT_CHANGED);
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

void Edit::pointerDoubleClicked()
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
        Matrix<float> inverse;
        getNode()->getInverseMatrix(inverse);
        Vector<2, float> position = inverse.transform(Vector<2, float>(m_engine->m_pointer[0], m_engine->m_pointer[1]));
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

void Edit::pointerReleased()
{
    m_engine->releasePointerCapture();
}

void Edit::pointerMoved()
{
    if ((m_engine->m_buttons & (1 << MOUSE_BUTTON_LEFT)) && m_engine->getFocusWidget() == this && m_selecting) {
        int caret = m_caret;
        Matrix<float> inverse;
        getNode()->getInverseMatrix(inverse);
        setCaretFromPoint(inverse.transform(m_engine->getPointer()));
        m_selectionLength += caret - m_caret;
        clampSelection();
        ensureCaretVisible();
    }
}

void Edit::ensureCaretVisible()
{
    if (m_textShape && m_node->getParent()) {
        Vector<2, float> parentMinimum = m_node->getParent()->getMinimum(1 << BOUNDS_SKIP_WIDGETS);
        Vector<2, float> parentMaximum = m_node->getParent()->getMaximum(1 << BOUNDS_SKIP_WIDGETS);
        Matrix<float> nodeMatrix;
        m_node->getMatrix(nodeMatrix);
        m_engine->getFontEngine()->lock();
        Vector<2, float> position;
        Vector<2, float> extent;
        m_textShape->m_font->getCaret(m_textShape->m_string.value(), m_caret, nodeMatrix, m_textShape->m_pixelSize,
                                      m_textShape->m_strokeRadius, m_textShape->m_spacing, m_textShape->m_lineSpacing,
                                      m_textShape->m_wrapWidth, position, extent, m_textShape->m_flags,
                                      m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
        Matrix<float> matrix = m_node->getParent()->m_transformation->m_matrix;
        matrix *= m_node->m_transformation->m_matrix;
        position = matrix.transform(position);
        extent = matrix.transformDirection(extent);
        int delta;
        if (position.m_elements[0] + extent.m_elements[0] >= parentMaximum.m_elements[0]) {
            Matrix<float> textMatrix;
            m_node->getMatrix(textMatrix);
            m_textShape->m_font->getCaret(m_textShape->m_string.value() + L" ",
                                          m_caret + CaretScrollLookaheadCharacters, textMatrix,
                                          m_textShape->m_pixelSize, m_textShape->m_strokeRadius, m_textShape->m_spacing,
                                          m_textShape->m_lineSpacing, m_textShape->m_wrapWidth, position, extent,
                                          m_textShape->m_flags, m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
            position = matrix.transform(position);
            delta = static_cast<int>(position.m_elements[0] - parentMaximum.m_elements[0]);
        } else {
            if (!(parentMinimum.m_elements[0] > position.m_elements[0])) {
                m_engine->getFontEngine()->unlock();
                return;
            }
            Matrix<float> textMatrix;
            m_node->getMatrix(textMatrix);
            m_textShape->m_font->getCaret(m_textShape->m_string.value() + L" ",
                                          m_caret - CaretScrollLookaheadCharacters, textMatrix,
                                          m_textShape->m_pixelSize, m_textShape->m_strokeRadius, m_textShape->m_spacing,
                                          m_textShape->m_lineSpacing, m_textShape->m_wrapWidth, position, extent,
                                          m_textShape->m_flags, m_textShape->m_hintingFlags & (1 << TEXT_HINTED));
            position = matrix.transform(position);
            delta = static_cast<int>(position.m_elements[0] - parentMinimum.m_elements[0]);
        }
        m_node->m_transformation->m_translation.value().m_elements[0] -= delta;
        if (m_caret - CaretScrollLookaheadCharacters <= 0 ||
            m_node->m_transformation->m_translation.value().m_elements[0] > m_initialTranslation.m_elements[0])
            m_node->m_transformation->m_translation.value().m_elements[0] = m_initialTranslation.m_elements[0];
        m_node->m_transformation->attributesChanged(true);
        m_engine->getFontEngine()->unlock();
    }
}

void Edit::setCaretFromPoint(const Vector<2, float> &point)
{
    std::wstring text = m_textShape->m_string.value();
    m_caret = 0;
    int size = text.size();
    Matrix<float> matrix;
    m_node->getMatrix(matrix);
    for (int i = 0; i < size; ++i) {
        m_engine->getFontEngine()->lock();
        Vector<2, float> position;
        Vector<2, float> extent;
        int hintingFlags = m_textShape->m_hintingFlags;
        int flags = m_textShape->m_flags;
        ScalableFont *font = m_textShape->m_font;
        font->getCaret(text, i, matrix, m_textShape->m_pixelSize, m_textShape->m_strokeRadius, m_textShape->m_spacing,
                       m_textShape->m_lineSpacing, m_textShape->m_wrapWidth, position, extent, flags,
                       hintingFlags & (1 << TEXT_HINTED));
        m_engine->getFontEngine()->unlock();
        if (point[0] > extent[0] * CaretAdvanceHitFraction + position[0])
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
        m_caret = m_textShape->m_string.value().size();
    int caret = m_caret;
    if (caret + m_selectionLength < 0)
        m_selectionLength = -caret;
    int length = m_selectionLength;
    if (length + caret > static_cast<int>(m_textShape->m_string.value().size()))
        m_caret = m_textShape->m_string.value().size() - length;
}

}
