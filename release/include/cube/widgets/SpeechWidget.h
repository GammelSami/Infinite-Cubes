// SpeechWidget displays queued dialogs, revealing words over time at 40 ms per character. It plays a voice sound every
// third character, pitched by the speaker's size, and shows next or end when the text is complete.
#pragma once
#include <list>
#include <string>
#include <vector>
#include "plasma/Vector.h"
#include "cube/Fixed.h"
#include "cube/widgets/BaseWidget.h"
#include "cube/Dialog.h"
#include "cube/Item.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class SpeechWidget : public BaseWidget {
public:
    SpeechWidget(Game *game, plasma::Node *node, plasma::Node *nextNode, plasma::Node *endNode);

    virtual void draw();

    // The time the current dialog takes to uncover: (characters + one per phrase) * m_characterTime; 0 without a
    // current dialog.
    int getDialogDuration();
    // Draws one phrase of the current dialog at m_textCursor, wrapping before it when it would pass the right edge,
    // and only the characters uncovered so far (m_readTime / m_characterTime); false once nothing more is uncovered.
    // noSpace: in, whether the previous phrase was "-" or "'" (then no space goes before this one); out, the same for
    // this phrase.
    bool drawPhrase(const std::wstring &text, const plasma::Vector<4, float> &color, int shake, bool &noSpace);

public:
    std::vector<Dialog> m_dialogs;
    float m_readTime;  // Elapsed time in the current dialog, used by draw.
    int m_dialogIndex; // Current dialog index used by draw.
    float m_textUncovered;
    // the speaker.
    long long m_speakerId;
    long long m_listenerId; // Initially 0.
    Item m_item;            // Initially zero; unused in this file.
    plasma::Vector<3, Fixed> m_anchorPosition;
    Game *m_game;
    plasma::Node *m_speechNext; // Next indicator, shown when more dialogs follow.
    plasma::Node *m_speechEnd;  // End indicator, shown after the last dialog.
    plasma::Vector<2, int> m_textCursor;
    int m_charactersShown; // Number of characters drawn this frame.
    int m_characterTime;   // 40; milliseconds per character
};

}
