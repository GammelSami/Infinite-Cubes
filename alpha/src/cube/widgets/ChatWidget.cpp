// ChatWidget lays out and draws chat messages.
#include "cube/widgets/ChatWidget.h"
#include "cube/DefaultBlocks.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
#include "plasma/Node.h"
namespace cube {

ChatWidget::ChatWidget(plasma::Engine *engine, plasma::Node *node)
    : Widget(engine, node),
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

// The last ten lines of messages, 18 units apart, each section in its colour over a black outline;
// then the type box and, while it is active, the cursor blinking every 500 units of the engine's
// caret clock. In pixels: the first baseline at 19, the lines 3 in, 10 high with a stroke of 2, a
// space 5 wide; the type box 12 up from the bottom.
void ChatWidget::draw()
{
    int y = 19;
    while (m_messageSections.size() > 10)
        m_messageSections.pop_front();
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
                m_scalableFont->measure(section->text, getNode()->m_matrix, 10.0f, 2.0f, 0.0f, 0.0f, 0.0f, minimum,
                                        maximum, 0, true, -1, false);
                // The outline in black with the colour's alpha, then the fill.
                m_scalableFont->renderText(section->text, x, (float)y, 10.0f, 2.0f, 0.0f, 0.0f, 0.0f,
                                           plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                           (color * plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f)).m_elements,
                                           plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
                m_scalableFont->renderText(section->text, x, (float)y, 10.0f, 2.0f, 0.0f, 0.0f, 0.0f, color.m_elements,
                                           plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                           plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
                x += maximum[0] - minimum[0];
            }
        }
        y += 18;
    }

    m_scalableFont->renderText(m_typeboxText, 3.0f, getContentHeight() - 12.0f, 10.0f, 2.0f, 0.0f, 0.0f, 0.0f,
                               plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                               plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                               plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
    m_scalableFont->renderText(m_typeboxText, 3.0f, getContentHeight() - 12.0f, 10.0f, 2.0f, 0.0f, 0.0f, 0.0f,
                               plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                               plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                               plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);

    if (m_typeboxActive && m_engine->getCaretClock() / 500 % 2) {
        clampSelection();
        plasma::Vector<2, float> position;
        plasma::Vector<2, float> extent;
        m_scalableFont->getCaret(m_typeboxText, m_typeboxCursorPosition, getNode()->m_matrix, 10.0f, 2.0f, 0.0f, 0.0f,
                                 0.0f, position, extent, 0, true);
        m_scalableFont->renderText(L"|", position[0] + 3.0f, getContentHeight() - 12.0f, 10.0f, 2.0f, 0.0f, 0.0f, 0.0f,
                                   plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                   plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                   plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
        m_scalableFont->renderText(L"|", position[0] + 3.0f, getContentHeight() - 12.0f, 10.0f, 2.0f, 0.0f, 0.0f, 0.0f,
                                   plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                   plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                   plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
    }
    m_engine->endTextDrawing(m_node);
}

void ChatWidget::deleteSelection()
{
    clampSelection();
    if (m_typeboxCursorPosition <= static_cast<int>(m_typeboxText.size())) {
        int start = m_typeboxCursorPosition;
        int count = m_backwardsHighlightLength;
        if (count == 0)
            count = 1;
        else if (count < 0) {
            start += count;
            count = -count;
        }
        m_typeboxText.erase(start, count);
        if (m_backwardsHighlightLength < 0)
            m_typeboxCursorPosition += m_backwardsHighlightLength;
        m_backwardsHighlightLength = 0;
    }
}

void ChatWidget::typeCharacter(wchar_t character)
{
    clampSelection();
    if (character >= L' ') {
        std::wstring text = L"";
        text += character;
        if (m_backwardsHighlightLength != 0)
            deleteSelection();
        clampSelection();
        m_typeboxText.insert(m_typeboxCursorPosition, text);
        ++m_typeboxCursorPosition;
    }
}

void ChatWidget::handleKeyDown(unsigned short key)
{
    clampSelection();
    m_changed = true;
    if (key == VK_RETURN)
        return;
    if (key == VK_LEFT) {
        if (m_typeboxCursorPosition > 0) {
            --m_typeboxCursorPosition;
            if (m_engine->isKeyDown(VK_SHIFT))
                ++m_backwardsHighlightLength;
            else
                m_backwardsHighlightLength = 0;
        }
        return;
    }
    if (key == VK_RIGHT) {
        if (m_typeboxCursorPosition < static_cast<int>(m_typeboxText.size())) {
            ++m_typeboxCursorPosition;
            if (m_engine->isKeyDown(VK_SHIFT))
                --m_backwardsHighlightLength;
            else
                m_backwardsHighlightLength = 0;
        }
        return;
    }
    if (key == VK_DELETE) {
        deleteSelection();
        return;
    }
    if (key == VK_BACK) {
        if (m_backwardsHighlightLength != 0)
            deleteSelection();
        else if (m_typeboxCursorPosition > 0) {
            m_typeboxText.erase(m_typeboxText.begin() + m_typeboxCursorPosition - 1);
            --m_typeboxCursorPosition;
        }
        return;
    }
    if (key == VK_HOME) {
        if (m_engine->isKeyDown(VK_SHIFT))
            m_backwardsHighlightLength += m_typeboxCursorPosition;
        else
            m_backwardsHighlightLength = 0;
        m_typeboxCursorPosition = 0;
        return;
    }
    if (key == VK_END) {
        if (m_engine->isKeyDown(VK_SHIFT))
            m_backwardsHighlightLength += m_typeboxCursorPosition - static_cast<int>(m_typeboxText.size());
        else
            m_backwardsHighlightLength = 0;
        m_typeboxCursorPosition = static_cast<int>(m_typeboxText.size());
    }
}

void ChatWidget::clampSelection()
{
    if (m_typeboxCursorPosition < 0)
        m_typeboxCursorPosition = 0;
    if (m_typeboxCursorPosition > static_cast<int>(m_typeboxText.size()))
        m_typeboxCursorPosition = static_cast<int>(m_typeboxText.size());
    if (m_typeboxCursorPosition + m_backwardsHighlightLength < 0)
        m_backwardsHighlightLength = -m_typeboxCursorPosition;
    // The alpha moves the cursor when the far end exceeds the text.
    if (m_typeboxCursorPosition + m_backwardsHighlightLength > static_cast<int>(m_typeboxText.size()))
        m_typeboxCursorPosition = static_cast<int>(m_typeboxText.size()) - m_backwardsHighlightLength;
}

// Wrap words twenty pixels before the right edge. spaces are five pixels wide; measure text at size ten with a stroke of two.
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

    int maximumWidth = (int)(getContentWidth() - 20.0f);
    float lineWidth = 0.0f;
    if (m_messageSections.empty())
        m_messageSections.push_back(std::list<MessageData>());

    // The width of the last line so far.
    for (std::list<MessageData>::iterator section = m_messageSections.back().begin();
         section != m_messageSections.back().end(); ++section) {
        if (section->text == L" ") {
            lineWidth += 5.0f;
        } else {
            plasma::Vector<2, float> minimum;
            plasma::Vector<2, float> maximum;
            m_scalableFont->measure(section->text, getNode()->m_matrix, 10.0f, 2.0f, 0.0f, 0.0f, 0.0f, minimum, maximum,
                                    0, true, -1, false);
            lineWidth += maximum[0] - minimum[0];
        }
    }
    // A line that ends in a word may take one more word past the width.
    bool continuesWord = !m_messageSections.back().empty() && m_messageSections.back().back().text != L" ";

    for (std::list<std::wstring>::iterator word = words.begin(); word != words.end(); ++word) {
        plasma::Vector<2, float> minimum;
        plasma::Vector<2, float> maximum;
        m_scalableFont->measure(*word, m_node->m_matrix, 10.0f, 2.0f, 0.0f, 0.0f, 0.0f, minimum, maximum, 0, true, -1,
                                false);
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
    plasma::Vector<3, unsigned char> rgb;
    rgb[0] = static_cast<unsigned char>(color[0] * 255.0f);
    rgb[1] = static_cast<unsigned char>(color[1] * 255.0f);
    rgb[2] = static_cast<unsigned char>(color[2] * 255.0f);
    printMessage(text, rgb);
}
}
