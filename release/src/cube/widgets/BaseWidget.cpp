// See cube/BaseWidget.h.
#include <math.h>
#include <sstream>
#include "cube/widgets/BaseWidget.h"
#include "cube/World.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/Node.h"
#include "plasma/ScalableFont.h"
#include "plasma/Transformation.h"
#include "plasma/EditFilter.h"

namespace cube {

BaseWidget::BaseWidget(plasma::Engine *engine, plasma::Node *node, const std::wstring &name)
    : plasma::Widget(engine, node, name),
      m_scalableFont(nullptr),
      m_textSize(12.0f),
      m_textStrokeRadius(0.0f),
      m_textSpacing(0.0f),
      m_textLineSpacing(0.0f),
      m_textWrapWidth(0.0f),
      m_textColor(1.0f, 1.0f, 1.0f, 1.0f),
      m_textStrokeColor(1.0f, 1.0f, 1.0f, 1.0f),
      m_textFlags(static_cast<plasma::TextFlags>(0))
{
    // The widget draws with the node's own transformation and display.
    node->makeTransformationUnique();
    node->makeDisplayUnique();
}

// Returns where the text ends; width shrinks by each character's advance. The font's parameters otherwise as
// ScalableFont::renderText.
static plasma::Vector<2, float> RenderWavyCharacters(plasma::ScalableFont *font, int phase, const std::wstring &text,
                                                     float x, float y, float size, float stroke, float spacing,
                                                     float leading, float width, const float *fill,
                                                     const float *outline, const float *extrusion, int flags)
{
    int length = text.size();
    for (int i = 0; i < length; i++) {
        std::wstringstream character;
        character << text[i];
        float wave = cosf(i + phase * 0.01f) * (size * 0.1f);
        plasma::Vector<2, float> end = font->renderText(character.str(), x, y + wave, size, stroke, spacing, leading,
                                                        width, fill, outline, extrusion, flags, true);
        width -= end[0] - x;
        x = end[0];
        y = end[1] - wave;
    }
    return plasma::Vector<2, float>(x, y);
}

void BaseWidget::measureText(const std::wstring &text, plasma::Vector<2, float> &minimum,
                             plasma::Vector<2, float> &maximum)
{
    if (m_scalableFont) {
        plasma::Matrix<float> matrix;
        m_engine->getModelViewMatrix(matrix);
        m_scalableFont->measure(text, matrix, m_textSize, 0.0f, m_textSpacing, m_textLineSpacing, 0.0f, minimum,
                                maximum, 0, true, -1, false);
    } else {
        minimum = plasma::Vector<2, float>(0.0f, 0.0f);
        maximum = plasma::Vector<2, float>(0.0f, 0.0f);
    }
}

void BaseWidget::setFont(const std::wstring &path)
{
    m_scalableFont = m_engine->getFontEngine()->getFont(path);
}

void BaseWidget::setTextColor(const plasma::Vector<4, float> &color)
{
    m_textColor = color;
}

void BaseWidget::setTextFlags(int flags)
{
    m_textFlags = static_cast<plasma::TextFlags>(flags);
}

void BaseWidget::setTextLineSpacing(float lineSpacing)
{
    m_textLineSpacing = lineSpacing;
}

void BaseWidget::setTextSize(float size)
{
    m_textSize = size;
}

void BaseWidget::setTextStrokeColor(const plasma::Vector<4, float> &color)
{
    m_textStrokeColor = color;
}

void BaseWidget::setTextStrokeRadius(float radius)
{
    m_textStrokeRadius = radius;
}

void BaseWidget::setTextWrapWidth(float width)
{
    m_textWrapWidth = width;
}

plasma::Vector<2, float> BaseWidget::renderText(const std::wstring &text, float x, float y)
{
    plasma::Vector<2, float> position(x, y);
    if (m_scalableFont) {
        m_engine->beginTextDrawing(m_node);
        // The text is placed in window units: only the x scale of the model-view matrix (element 0) is used, for both
        // axes.
        plasma::Matrix<float> matrix;
        m_engine->getModelViewMatrix(matrix);
        position = (m_node->transformation()->m_pivot.value() + position) * matrix.m_elements[0];
        plasma::Vector<2, float> end;
        if (m_textStrokeRadius > 0.0f) {
            // The outline alone first (a transparent fill), then the fill without a stroke.
            end = m_scalableFont->renderText(
                text, position[0], position[1], m_textSize, m_textStrokeRadius, m_textSpacing, m_textLineSpacing,
                m_textWrapWidth, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                m_textStrokeColor.m_elements, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, m_textFlags,
                true);
            m_scalableFont->renderText(text, position[0], position[1], m_textSize, 0.0f, m_textSpacing,
                                       m_textLineSpacing, m_textWrapWidth, m_textColor.m_elements,
                                       m_textStrokeColor.m_elements,
                                       plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, m_textFlags, true);
        } else {
            end = m_scalableFont->renderText(
                text, position[0], position[1], m_textSize, m_textStrokeRadius, m_textSpacing, m_textLineSpacing,
                m_textWrapWidth, m_textColor.m_elements, m_textStrokeColor.m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, m_textFlags, true);
        }
        m_engine->endTextDrawing(m_node);
        // Back to the widget's units, relative to the pivot.
        return plasma::Vector<2, float>(end[0] / matrix.m_elements[0], end[1] / matrix.m_elements[0]) -
               m_node->transformation()->m_pivot.value();
    } else {
        return plasma::Vector<2, float>(0.0f, 0.0f);
    }
}

plasma::Vector<2, float> BaseWidget::renderTextLines(const std::vector<Dialog> &dialogs, float x, float y, float width)
{
    setTextFlags(0);
    setTextLineSpacing(2.0f);
    setTextWrapWidth(0.0f);
    for (const Dialog &dialog : dialogs) {
        // No space before a line's first word, after "-", or before punctuation (3 units instead of 12).
        float wordX = x;
        bool joined = true;
        for (const Dialog::Phrase &word : dialog.phrases) {
            plasma::Vector<2, float> minimum;
            plasma::Vector<2, float> maximum;
            measureText(word.text, minimum, maximum);
            int wordWidth = maximum[0] - minimum[0];
            bool punctuation = word.text == L"." || word.text == L"-" || word.text == L"," || word.text == L";" ||
                               word.text == L"!" || word.text == L"?" || word.text == L":" || word.text == L"/" ||
                               word.text == L")";
            if (!joined) {
                if (punctuation)
                    wordX += 3.0f;
                else
                    wordX += 12.0f;
            }
            joined = word.text == L"-";
            // Wrap before a word that would pass the width.
            if (!punctuation && wordWidth + wordX > x + width) {
                wordX = x;
                y += m_textLineSpacing + m_textSize;
            }
            setTextColor(word.color);
            renderText(word.text, wordX, y);
            wordX += wordWidth;
            // "/" and "(" pull the next word closer.
            if (word.text == L"/" || word.text == L"(")
                wordX += -9.0f;
        }
        y += (m_textLineSpacing + m_textSize) * 1.5f;
    }
    return plasma::Vector<2, float>((float)(int)x, (float)(int)y);
}

plasma::Vector<2, float> BaseWidget::renderWavyText(const std::wstring &text, float x, float y)
{
    if (m_scalableFont) {
        m_engine->beginTextDrawing(m_node);
        plasma::Matrix<float> matrix;
        m_engine->getModelViewMatrix(matrix);
        plasma::Vector<2, float> position =
            (m_node->transformation()->m_pivot.value() + plasma::Vector<2, float>(x, y)) * matrix.m_elements[0];
        plasma::Vector<2, float> end;
        if (m_textStrokeRadius > 0.0f) {
            // The outlined text first, then the fill again without a stroke.
            end = RenderWavyCharacters(m_scalableFont, m_engine->getCaretClock(), text, position[0], position[1],
                                       m_textSize, m_textStrokeRadius, m_textSpacing, m_textLineSpacing,
                                       m_textWrapWidth, m_textColor.m_elements, m_textStrokeColor.m_elements,
                                       plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, m_textFlags);
            RenderWavyCharacters(m_scalableFont, m_engine->getCaretClock(), text, position[0], position[1], m_textSize,
                                 0.0f, m_textSpacing, m_textLineSpacing, m_textWrapWidth, m_textColor.m_elements,
                                 m_textStrokeColor.m_elements,
                                 plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, m_textFlags);
        } else {
            end = RenderWavyCharacters(m_scalableFont, m_engine->getCaretClock(), text, position[0], position[1],
                                       m_textSize, m_textStrokeRadius, m_textSpacing, m_textLineSpacing,
                                       m_textWrapWidth, m_textColor.m_elements, m_textStrokeColor.m_elements,
                                       plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, m_textFlags);
        }
        m_engine->endTextDrawing(m_node);
        return plasma::Vector<2, float>(end[0] / matrix.m_elements[0], end[1] / matrix.m_elements[0]) -
               m_node->transformation()->m_pivot.value();
    } else {
        return plasma::Vector<2, float>(0.0f, 0.0f);
    }
}

}
