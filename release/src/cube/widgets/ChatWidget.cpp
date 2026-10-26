// See cube/ChatWidget.h.
#include <windows.h>
#include "cube/widgets/ChatWidget.h"
#include "cube/Noise.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/Node.h"
#include "plasma/ScalableFont.h"
#include "plasma/EditFilter.h"

namespace cube {

// The text size and stroke of the chat's font, in window units.
static const float CHAT_TEXT_SIZE = 12.0f;
static const float CHAT_TEXT_STROKE = 3.0f;

ChatWidget::ChatWidget(plasma::Engine *engine, plasma::Node *node)
    : plasma::Widget(engine, node, L""),
      m_typeboxActive(false),
      m_typeboxCursorPosition(0),
      m_backwardsHighlightLength(0)
{
    m_scalableFont = engine->getFontEngine()->getFont(L"resource1.dat");
    m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
}

plasma::Vector<2, float> ChatWidget::getMaximum()
{
    return getViewSize();
}

void ChatWidget::deleteSelection()
{
    clampSelection();
    int position = m_typeboxCursorPosition;
    if (position <= (int)m_typeboxText.size()) {
        int length = m_backwardsHighlightLength;
        if (length == 0) {
            length = 1;
        } else if (length < 0) {
            position += length;
            length = -length;
        }
        m_typeboxText.erase(position, length);
        if (m_backwardsHighlightLength < 0)
            m_typeboxCursorPosition += m_backwardsHighlightLength;
        m_backwardsHighlightLength = 0;
    }
}

void ChatWidget::draw()
{
    int y = 19;
    // Keep only as many messages as fit, 18 units per line.
    size_t lines = (int)(getContentHeight() / 18.0f);
    while (m_messageSections.size() > lines)
        m_messageSections.pop_front();

    plasma::Matrix<float> matrix;
    m_node->getMatrix(matrix);
    m_engine->beginTextDrawing(m_node);

    for (std::list<std::list<MessageData>>::const_iterator message = m_messageSections.begin();
         message != m_messageSections.end(); ++message) {
        float x = 3.0f;
        for (std::list<MessageData>::const_iterator section = message->begin(); section != message->end(); ++section) {
            if (section->text == L" ") {
                x += 5.0f;
            } else {
                plasma::Vector<4, float> color(section->color[0] / 255.0f, section->color[1] / 255.0f,
                                               section->color[2] / 255.0f, 1.0f);
                plasma::Vector<2, float> minimum;
                plasma::Vector<2, float> maximum;
                m_scalableFont->measure(section->text, matrix, CHAT_TEXT_SIZE, CHAT_TEXT_STROKE, 0.0f, 0.0f, 0.0f,
                                        minimum, maximum, 0, true, -1, false);
                // The outline in black with the colour's alpha, then the fill.
                m_scalableFont->renderText(section->text, matrix.m_elements[0] * x, y * matrix.m_elements[0],
                                           CHAT_TEXT_SIZE, CHAT_TEXT_STROKE, 0.0f, 0.0f, 0.0f,
                                           plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                           (color * plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f)).m_elements,
                                           plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
                m_scalableFont->renderText(section->text, matrix.m_elements[0] * x, y * matrix.m_elements[0],
                                           CHAT_TEXT_SIZE, CHAT_TEXT_STROKE, 0.0f, 0.0f, 0.0f, color.m_elements,
                                           plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                           plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
                x += maximum[0] - minimum[0];
            }
        }
        y += 18;
    }

    // The type box: a black outline, then the text in white.
    m_scalableFont->renderText(m_typeboxText, matrix.m_elements[0] * 3.0f,
                               (getContentHeight() - 12.0f) * matrix.m_elements[0], CHAT_TEXT_SIZE, CHAT_TEXT_STROKE,
                               0.0f, 0.0f, 0.0f, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                               plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                               plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
    m_scalableFont->renderText(m_typeboxText, matrix.m_elements[0] * 3.0f,
                               (getContentHeight() - 12.0f) * matrix.m_elements[0], CHAT_TEXT_SIZE, CHAT_TEXT_STROKE,
                               0.0f, 0.0f, 0.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                               plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                               plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);

    // The blinking cursor (every 500 units of the engine's caret clock).
    if (m_typeboxActive && (m_engine->getCaretClock() / 500) & 1) {
        clampSelection();
        plasma::Vector<2, float> position;
        plasma::Vector<2, float> extent;
        m_scalableFont->getCaret(m_typeboxText, m_typeboxCursorPosition, matrix, CHAT_TEXT_SIZE, CHAT_TEXT_STROKE, 0.0f,
                                 0.0f, 0.0f, position, extent, 0, true);
        m_scalableFont->renderText(L"|", (position[0] + 3.0f) * matrix.m_elements[0],
                                   (getContentHeight() - 12.0f) * matrix.m_elements[0], CHAT_TEXT_SIZE,
                                   CHAT_TEXT_STROKE, 0.0f, 0.0f, 0.0f,
                                   plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                   plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                   plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
        m_scalableFont->renderText(L"|", (position[0] + 3.0f) * matrix.m_elements[0],
                                   (getContentHeight() - 12.0f) * matrix.m_elements[0], CHAT_TEXT_SIZE,
                                   CHAT_TEXT_STROKE, 0.0f, 0.0f, 0.0f,
                                   plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                   plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                   plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
    }
    m_engine->endTextDrawing(m_node);
}

void ChatWidget::typeCharacter(wchar_t character)
{
    clampSelection();
    if (character < L' ')
        return;
    std::wstring text = L"";
    text.append(1, character);
    if (m_backwardsHighlightLength)
        deleteSelection();
    clampSelection();
    m_typeboxText.insert(m_typeboxCursorPosition, text);
    ++m_typeboxCursorPosition;
}

void ChatWidget::handleKeyDown(unsigned short key)
{
    clampSelection();
    m_changed = true;
    if (key == VK_RETURN)
        return;
    if (key == VK_LEFT) {
        // Shift extends the selection.
        if (m_typeboxCursorPosition > 0) {
            --m_typeboxCursorPosition;
            if (m_engine->isKeyDown(VK_SHIFT))
                ++m_backwardsHighlightLength;
            else
                m_backwardsHighlightLength = 0;
        }
    } else if (key == VK_RIGHT) {
        if (m_typeboxCursorPosition < (int)m_typeboxText.size()) {
            ++m_typeboxCursorPosition;
            if (!m_engine->isKeyDown(VK_SHIFT))
                m_backwardsHighlightLength = 0;
            else
                --m_backwardsHighlightLength;
        }
    } else if (key == VK_DELETE) {
        deleteSelection();
    } else if (key == VK_BACK) {
        if (m_backwardsHighlightLength) {
            deleteSelection();
        } else if (m_typeboxCursorPosition > 0) {
            m_typeboxText.erase(m_typeboxText.begin() + m_typeboxCursorPosition - 1);
            --m_typeboxCursorPosition;
        }
    } else if (key == VK_HOME) {
        if (m_engine->isKeyDown(VK_SHIFT))
            m_backwardsHighlightLength += m_typeboxCursorPosition;
        else
            m_backwardsHighlightLength = 0;
        m_typeboxCursorPosition = 0;
    } else if (key == VK_END) {
        if (m_engine->isKeyDown(VK_SHIFT))
            m_backwardsHighlightLength += m_typeboxCursorPosition - (int)m_typeboxText.size();
        else
            m_backwardsHighlightLength = 0;
        m_typeboxCursorPosition = (int)m_typeboxText.size();
    }
}

int ChatWidget::clampSelection()
{
    if (m_typeboxCursorPosition < 0)
        m_typeboxCursorPosition = 0;
    if (m_typeboxCursorPosition > (int)m_typeboxText.size())
        m_typeboxCursorPosition = (int)m_typeboxText.size();
    if (m_typeboxCursorPosition + m_backwardsHighlightLength < 0)
        m_backwardsHighlightLength = -m_typeboxCursorPosition;
    int end = m_typeboxCursorPosition + m_backwardsHighlightLength;
    if (end > (int)m_typeboxText.size())
        m_typeboxCursorPosition = (int)m_typeboxText.size() - m_backwardsHighlightLength;
    return end;
}

void ChatWidget::printMessage(const std::wstring &text, const plasma::Vector<3, unsigned char> &color)
{
    // Split the text into words and separators (spaces and line breaks).
    int start = 0;
    std::list<std::wstring> words;
    int length = (int)text.size();
    for (int i = 0; i <= length; i++) {
        if (text[i] == L' ' || text[i] == L'\n' || text[i] == L'\r' || i == length) {
            if (i - start > 0)
                words.push_back(text.substr(start, i - start));
            if (i != length)
                words.push_back(text.substr(i, 1));
            start = i + 1;
        }
    }

    int maximumWidth = (int)(getContentWidth() - 30.0f);
    float lineWidth = 0.0f;
    if (m_messageSections.empty())
        m_messageSections.push_back(std::list<MessageData>());

    // The width of the last line so far.
    plasma::Matrix<float> matrix;
    m_node->getMatrix(matrix);
    for (std::list<MessageData>::iterator section = m_messageSections.back().begin();
         section != m_messageSections.back().end(); ++section) {
        if (section->text == L" ") {
            lineWidth += 5.0f;
        } else {
            plasma::Vector<2, float> minimum;
            plasma::Vector<2, float> maximum;
            m_scalableFont->measure(section->text, matrix, CHAT_TEXT_SIZE, CHAT_TEXT_STROKE, 0.0f, 0.0f, 0.0f, minimum,
                                    maximum, 0, true, -1, false);
            lineWidth += maximum[0] - minimum[0];
        }
    }
    // A line that ends in a word may take one more word past the width.
    bool continuesWord = !m_messageSections.back().empty() && m_messageSections.back().back().text != L" ";

    for (std::list<std::wstring>::iterator word = words.begin(); word != words.end(); ++word) {
        plasma::Vector<2, float> minimum;
        plasma::Vector<2, float> maximum;
        m_scalableFont->measure(*word, matrix, CHAT_TEXT_SIZE, CHAT_TEXT_STROKE, 0.0f, 0.0f, 0.0f, minimum, maximum, 0,
                                true, -1, false);
        if (*word == L"\n" || *word == L"\r") {
            m_messageSections.push_back(std::list<MessageData>());
            lineWidth = 0.0f;
            continue;
        }
        int width = *word == L" " ? 5 : (int)(maximum[0] - minimum[0]);
        lineWidth += width;
        if (lineWidth >= maximumWidth) {
            if (continuesWord) {
                continuesWord = false;
            } else {
                // Wrap: a new line starts with this word (a space is dropped).
                m_messageSections.push_back(std::list<MessageData>());
                lineWidth = (float)width;
                if (*word == L" ")
                    continue;
            }
        }
        m_messageSections.back().push_back(MessageData(*word, color));
    }
    m_changed = true;
}

void ChatWidget::printMessage(const std::wstring &text, const plasma::Vector<4, float> &color)
{
    printMessage(text, plasma::Vector<3, unsigned char>((unsigned char)(color[0] * 255.0f),
                                                        (unsigned char)(color[1] * 255.0f),
                                                        (unsigned char)(color[2] * 255.0f)));
}

}
