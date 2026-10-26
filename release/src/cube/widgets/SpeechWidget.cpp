// See cube/SpeechWidget.h.
#include <math.h>
#include <stdlib.h>
#include <string>
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/widgets/SpeechWidget.h"
#include "cube/World.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/EditFilter.h"

namespace cube {

SpeechWidget::SpeechWidget(Game *game, plasma::Node *node, plasma::Node *nextNode, plasma::Node *endNode)
    : BaseWidget(game->m_engine, node, L""),
      m_readTime(0.0f),
      m_dialogIndex(0),
      m_speakerId(0),
      m_listenerId(0),
      m_game(game),
      m_speechNext(nextNode),
      m_speechEnd(endNode),
      m_characterTime(40)
{
    setFont(L"resource1.dat");
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    // Own copies of the two nodes under the bubble.
    if (m_speechNext) {
        m_speechNext = m_speechNext->instance(node);
        m_speechNext->makeTransformationUnique();
        m_speechNext->makeDisplayUnique();
    }
    if (m_speechEnd) {
        m_speechEnd = m_speechEnd->instance(node);
        m_speechEnd->makeTransformationUnique();
        m_speechEnd->makeDisplayUnique();
    }
}

void SpeechWidget::draw()
{
    if (m_dialogIndex >= 0 && m_dialogIndex < (int)m_dialogs.size()) {
        // Draw the phrases uncovered so far.
        m_textCursor = plasma::Vector<2, int>(0, 0);
        m_charactersShown = 0;
        int lastShake = 0;
        bool noSpace = true;
        std::list<Dialog::Phrase> &phrases = m_dialogs[m_dialogIndex].phrases;
        for (auto &phrase : phrases) {
            if (!drawPhrase(phrase.text, phrase.color, phrase.shake, noSpace))
                break;
            lastShake = phrase.shake;
        }

        // A voice sound every third character, pitched by the speaker's size (and raised for shaking words), from the
        // local creature's place.
        if ((int)m_textUncovered / 3 != m_charactersShown / 3) {
            float pitch = 1.0f;
            if (m_speakerId) {
                Creature *speaker = m_game->m_world->creatureByID(m_speakerId);
                if (speaker)
                    pitch = 0.8f * 1.2f / speaker->m_entityData.appearance.size[0];
            }
            if (lastShake == 1)
                pitch *= rand() / (float)RAND_MAX + 1.5f;
            Creature *listener = m_game->m_world->m_localCreature;
            m_game->playSound(SoundHumanVoice02, listener->m_entityData.position, 1.0f,
                              rand() * 0.5f / (float)RAND_MAX + pitch, true);
        }

        // "Next" bobs under the text once it is uncovered and more dialogs follow; "end" takes its place after the
        // last one.
        if (m_speechNext) {
            m_speechNext->display()->m_visibility.value() =
                m_readTime > getDialogDuration() + 100 && m_dialogIndex < (int)m_dialogs.size() - 1;
            int clock = m_engine->getCaretClock();
            plasma::Widget *next = m_speechNext->widget();
            float y = getContentHeight() - 50.0f + cosf(clock * 0.01f) * 5.0f;
            next->moveTo((getContentWidth() - next->viewWidth()) * 0.5f, y, true);
        }
        if (m_speechEnd) {
            m_speechEnd->display()->m_visibility.value() =
                m_readTime > getDialogDuration() + 100 && m_dialogIndex == (int)m_dialogs.size() - 1;
            plasma::Widget *next = m_speechNext->widget();
            plasma::Widget *end = m_speechEnd->widget();
            float y = getContentHeight() - 50.0f;
            end->moveTo((getContentWidth() - next->viewWidth()) * 0.5f, y, true);
        }
        m_textUncovered = (float)m_charactersShown;
    }
}

int SpeechWidget::getDialogDuration()
{
    if (m_dialogIndex < 0 || m_dialogIndex >= (int)m_dialogs.size())
        return 0;
    int characters = 0;
    for (std::list<Dialog::Phrase>::iterator phrase = m_dialogs[m_dialogIndex].phrases.begin();
         phrase != m_dialogs[m_dialogIndex].phrases.end(); ++phrase)
        characters += phrase->text.size() + 1;
    return characters * m_characterTime;
}

bool SpeechWidget::drawPhrase(const std::wstring &text, const plasma::Vector<4, float> &color, int shake, bool &noSpace)
{
    if (text.size()) {
        int uncovered = (int)(m_readTime / m_characterTime);
        if (m_charactersShown > uncovered)
            return false;
        plasma::Matrix<float> modelView;
        m_engine->getModelViewMatrix(modelView);
        plasma::Vector<2, float> minimum;
        plasma::Vector<2, float> maximum;
        measureText(text, minimum, maximum);
        int width = (int)(maximum[0] - minimum[0]);
        bool punctuation = text == L"." || text == L"-" || text == L"," || text == L";" || text == L"!" ||
                           text == L"?" || text == L":" || text == L"'";
        plasma::Matrix<float> matrix;
        m_node->getMatrix(matrix);
        if (!noSpace) {
            // The space before the phrase: a third of the text size before punctuation.
            if (punctuation)
                m_textCursor[0] += (int)m_textSize / 3;
            else
                m_textCursor[0] += (int)m_textSize;
        }
        noSpace = text == L"-" || text == L"'";
        int advance = text == L"'" ? width + 4 : width;
        if (!punctuation) {
            // The text size counts in whole units here, as for the spaces.
            int size = (int)m_textSize;
            if ((float)(advance + m_textCursor[0]) > getContentWidth() - 3 * size) {
                m_textCursor[0] = 0;
                m_textCursor[1] = (int)((int)m_textSize * 1.35f + m_textCursor[1]);
            }
        }
        int length = text.size();
        std::wstring shown = text;
        if (length + m_charactersShown > uncovered) {
            length = uncovered - m_charactersShown;
            shown = text.substr(0, length);
        }
        setTextColor(color);
        setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
        float y = (float)(m_textCursor[1] + 2 * (int)m_textSize);
        float x = (float)((int)m_textSize + m_textCursor[0]);
        if (shake == 1)
            renderWavyText(shown, x, y);
        else
            renderText(shown, x, y);
        m_textCursor[0] += advance;
        m_charactersShown += length + 1;
    }
    return true;
}

}
