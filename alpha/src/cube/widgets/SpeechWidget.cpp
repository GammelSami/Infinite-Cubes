// SpeechWidget lays out dialog text and choices.
#include "cube/DefaultBlocks.h"
#include "cube/widgets/SpeechWidget.h"
#include "cube/SoundIds.h"
#include "cube/Creature.h"
#include "cube/GameController.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/Node.h"
#include "plasma/ScalableFont.h"
#include "plasma/Transformation.h"
namespace cube {
// The text is uncovered a character per DefaultCharacterTime milliseconds (anonymous: this file's
// own).
namespace {
enum {
    DefaultCharacterTime = 40
};
}

// The widget is 250 x 200 pixels with a 10-pixel border; its text 14 pixels high, stroked 3.
SpeechWidget::SpeechWidget(plasma::Engine *engine, plasma::Node *node, GameController *game)
    : Widget(engine, node),
      m_readTime(0),
      m_dialogIndex(0),
      m_hoveredChoice(0),
      m_creature(0),
      m_characterTime(DefaultCharacterTime),
      m_game(game)
{
    plasma::Matrix<float> identity;
    identity.identity();
    setGeometry(plasma::Vector<2, float>(0.0f, 0.0f), plasma::Vector<2, float>(250.0f, 200.0f),
                plasma::Vector<2, float>(10.0f, 10.0f), plasma::Vector<2, float>(230.0f, 180.0f), identity);
    m_font = L"resource1.dat";
    m_textSize = 14;
    m_strokeRadius = 3;
}

// The current dialog's phrases, uncovered with the time read (a voice sound every third character,
// at a pitch of 1 to 1.5); once the dialog is read, the answers in a row under it, the hovered one
// in cyan. The answers start at 15, 75 pixels up from the bottom, 20 apart; the hovered one cyan
// (RGBA).
void SpeechWidget::draw()
{
    if (m_dialogIndex < 0 || m_dialogIndex >= (int)m_dialogs.size())
        return;
    m_textCursor = plasma::Vector<2, int>(0, 0);
    m_charactersShown = 0;
    m_engine->beginTextDrawing(m_node);
    bool noSpace = true;
    for (std::list<Dialog::Phrase>::iterator phrase = m_dialogs[m_dialogIndex].phrases.begin();
         phrase != m_dialogs[m_dialogIndex].phrases.end(); ++phrase) {
        if (!drawPhrase(phrase->text, phrase->color, noSpace))
            break;
    }
    if (m_textUncovered / 3 != m_charactersShown / 3) {
        Creature *player = m_game->m_player;
        m_game->playSound(SoundHumanVoice02, player->m_entityData.position, 1.0f,
                          (float)rand() * 0.5f / float(RAND_MAX) + 1.0f);
    }
    m_textUncovered = m_charactersShown;
    if (m_readTime >= getDialogDuration() && !m_choices.empty()) {
        int y = (int)(getContentHeight() - 75.0f);
        int x = 15;
        plasma::ScalableFont *font = m_engine->getFontEngine()->getFont(m_font);
        if (font) {
            plasma::Matrix<float> matrix;
            m_engine->getModelViewMatrix(matrix);
            plasma::Vector<2, float> pivot = m_node->transformation()->m_pivot.value();
            for (int i = 0; i < (int)m_choices.size(); i++) {
                std::wstring text = m_choices[i];
                font->renderText(text, (float)x + pivot[0], (float)y + pivot[1], (float)m_textSize,
                                 (float)m_strokeRadius, 0.0f, 0.0f, 0.0f,
                                 plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                 plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                 plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
                font->renderText(text, (float)x + pivot[0], (float)y + pivot[1], (float)m_textSize, 0.0f, 0.0f, 0.0f,
                                 0.0f,
                                 (i == m_hoveredChoice ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                       : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f))
                                     .m_elements,
                                 plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                 plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
                plasma::Vector<2, float> minimum;
                plasma::Vector<2, float> maximum;
                font->measure(text, matrix, (float)m_textSize, 0.0f, 0.0f, 0.0f, 0.0f, minimum, maximum, 0, true, -1,
                              false);
                x += 20 + (int)(maximum[0] - minimum[0]);
            }
        }
    }
    m_engine->endTextDrawing(m_node);
}

bool SpeechWidget::isDialogFinished()
{
    return m_readTime >= getDialogDuration();
}

int SpeechWidget::getDialogDuration()
{
    if (m_dialogIndex >= 0 && m_dialogIndex < static_cast<int>(m_dialogs.size())) {
        int characters = 0;
        for (std::list<Dialog::Phrase>::iterator it = m_dialogs[m_dialogIndex].phrases.begin();
             it != m_dialogs[m_dialogIndex].phrases.end(); it++)
            characters += (int)(1 + it->text.size());
        return characters * m_characterTime;
    }
    return 0;
}

// Draws the next phrase at the text cursor, as far as the time read so far uncovers it
// (m_characterTime milliseconds per character), and returns false once the uncovered text ends
// before it. Punctuation follows the previous word closely and never wraps; a line is 18 units
// high. The words are spaced 14 pixels (3 before punctuation), wrap 30 pixels before the right
// edge into lines 18 apart, and are drawn 10, 25 pixels in.
bool SpeechWidget::drawPhrase(const std::wstring &text, const plasma::Vector<4, float> &color, bool &noSpace)
{
    if (text.size() == 0)
        return true;
    int uncovered = m_readTime / m_characterTime;
    if (m_charactersShown > uncovered)
        return false;
    plasma::ScalableFont *font = m_engine->getFontEngine()->getFont(m_font);
    if (!font)
        return true;
    plasma::Matrix<float> matrix;
    m_engine->getModelViewMatrix(matrix);
    plasma::Vector<2, float> minimum;
    plasma::Vector<2, float> maximum;
    font->measure(text, matrix, (float)m_textSize, 0.0f, 0.0f, 0.0f, 0.0f, minimum, maximum, 0, true, -1, false);
    int wordWidth = (int)(maximum[0] - minimum[0]);
    bool punctuation =
        text == L"." || text == L"-" || text == L"," || text == L";" || text == L"!" || text == L"?" || text == L":";
    if (!noSpace) {
        if (punctuation)
            m_textCursor[0] += 3;
        else
            m_textCursor[0] += 14;
    }
    noSpace = text == L"-" ? true : false;
    if (!punctuation && (float)(m_textCursor[0] + wordWidth) > getContentWidth() - 30.0f) {
        m_textCursor[0] = 0;
        m_textCursor[1] += 18;
    }
    plasma::Vector<2, float> pivot = m_node->transformation()->m_pivot.value();
    int length = (int)text.size();
    std::wstring visible = text;
    if (m_charactersShown + length > uncovered) {
        length = uncovered - m_charactersShown;
        visible = text.substr(0, length);
    }
    font->renderText(visible, (float)(m_textCursor[0] + 10) + pivot[0], (float)(m_textCursor[1] + 25) + pivot[1],
                     (float)m_textSize, (float)m_strokeRadius, 0.0f, 0.0f, 0.0f, color.m_elements,
                     plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                     plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
    font->renderText(visible, (float)(m_textCursor[0] + 10) + pivot[0], (float)(m_textCursor[1] + 25) + pivot[1],
                     (float)m_textSize, 0.0f, 0.0f, 0.0f, 0.0f, color.m_elements,
                     plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                     plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, true);
    m_textCursor[0] += wordWidth;
    m_charactersShown += length + 1;
    return true;
}
}
