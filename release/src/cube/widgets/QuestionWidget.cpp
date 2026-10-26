#include <algorithm>
// See cube/QuestionWidget.h.
#include <string>
#include "cube/Game.h"
#include "cube/widgets/QuestionWidget.h"
#include "cube/World.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/EditFilter.h"

namespace cube {

QuestionWidget::QuestionWidget(Game *game, plasma::Node *node)
    : BaseWidget(game->m_engine, node, L""),
      m_speakerId(0),
      m_game(game),
      m_unused210(0),
      m_unused23c(1.0f),
      m_unused240(true),
      m_unused244(0),
      m_unused248(0),
      m_unused2f0(true),
      m_hoveredDialog(-1)
{
    setFont(L"resource1.dat");
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
}

void QuestionWidget::fitToDialogs()
{
    int width = 0;
    int height = 0;
    for (Dialog &dialog : m_dialogs) {
        plasma::Vector<2, float> size = measureDialog(dialog);
        height = (int)(height + size[1]);
        if (size[0] > width)
            width = (int)size[0];
    }
    resizeScaled((float)(width + 50), (float)(height + 20), true);
}

void QuestionWidget::draw()
{
    // The answers one under the other, 32 apart, each laid out word by word like a speech bubble (measureDialog); the
    // one under the pointer is drawn cyan.
    m_hoveredDialog = -1;
    int y = 0;
    int index = -1;
    for (Dialog &dialog : m_dialogs) {
        ++index;
        bool noSpace = true;
        bool hovered;
        if (m_node->isUnderPointer() && getPointerOffset()[1] >= y &&
            getPointerOffset()[1] < y + measureDialog(dialog)[1]) {
            hovered = true;
            m_hoveredDialog = index;
        } else {
            hovered = false;
        }
        float top = (float)y;
        int x = 0;
        for (auto &phrase : dialog.phrases) {
            plasma::Matrix<float> modelView;
            m_engine->getModelViewMatrix(modelView);
            plasma::Vector<2, float> minimum;
            plasma::Vector<2, float> maximum;
            measureText(phrase.text, minimum, maximum);
            int width = (int)(maximum[0] - minimum[0]);
            bool punctuation = phrase.text == L"." || phrase.text == L"-" || phrase.text == L"," ||
                               phrase.text == L";" || phrase.text == L"!" || phrase.text == L"?" ||
                               phrase.text == L":" || phrase.text == L"'";
            plasma::Matrix<float> matrix;
            m_node->getMatrix(matrix);
            if (!noSpace) {
                int space = (int)m_textSize;
                if (punctuation)
                    space /= 4;
                x += space;
            }
            noSpace = phrase.text == L"-" || phrase.text == L"'";
            int advance = phrase.text == L"'" ? width + 4 : width;
            if (!punctuation) {
                int size = (int)m_textSize;
                if ((float)(advance + x) > getContentWidth() - 2 * size) {
                    x = 0;
                    y = (int)((int)m_textSize * 1.35f + y);
                }
            }
            setTextColor(hovered ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f) : phrase.color);
            setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
            renderText(phrase.text, (float)(x + 20), (float)(y + 25));
            x += advance;
        }
        y += 32;
        // The answer's centre, for the interface's button list.
        m_game->m_interface.addButtonCenter(this, (int)(getContentWidth() * 0.5f), (int)((y + top) * 0.5f + 8.0f));
    }
}

plasma::Vector<2, float> QuestionWidget::measureDialog(const Dialog &dialog)
{
    // Lays the phrases out as Render does, with lines 18 apart: the widest line and the height (plus 32).
    int x = 0;
    int height = 0;
    int width = 0;
    bool noSpace = true;
    for (const auto &phrase : dialog.phrases) {
        plasma::Matrix<float> modelView;
        m_engine->getModelViewMatrix(modelView);
        plasma::Vector<2, float> minimum;
        plasma::Vector<2, float> maximum;
        measureText(phrase.text, minimum, maximum);
        int phraseWidth = (int)(maximum[0] - minimum[0]);
        bool punctuation = phrase.text == L"." || phrase.text == L"-" || phrase.text == L"," || phrase.text == L";" ||
                           phrase.text == L"!" || phrase.text == L"?" || phrase.text == L":" || phrase.text == L"'";
        plasma::Matrix<float> matrix;
        m_node->getMatrix(matrix);
        if (!noSpace) {
            int space = (int)m_textSize;
            if (punctuation)
                space /= 4;
            x += space;
        }
        noSpace = phrase.text == L"-" || phrase.text == L"'";
        int advance = !phrase.text.compare(L"'") ? phraseWidth + 4 : phraseWidth;
        if (!punctuation) {
            int size = (int)m_textSize;
            if ((float)(x + advance) > getContentWidth() - 2 * size) {
                x = 0;
                height += 18;
            }
        }
        x += advance;
        width = std::max(x, width);
    }
    return plasma::Vector<2, float>((float)width, (float)(height + 32));
}

}
