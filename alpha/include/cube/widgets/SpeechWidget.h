// SpeechWidget displays dialog text and choices.
#pragma once
#include "plasma/Widget.h"
#include "cube/Speech.h"
#include "cube/Fixed.h"
namespace cube {
class GameController;
class Creature;
class SpeechWidget : public plasma::Widget {
public:
    SpeechWidget(plasma::Engine *engine, plasma::Node *node, GameController *game);
    virtual void draw();
    bool isDialogFinished();
    int getDialogDuration();
    bool drawPhrase(const std::wstring &text, const plasma::Vector<4, float> &color, bool &noSpace);

    std::vector<Dialog> m_dialogs;
    int m_readTime; // compared with getDialogDuration
    int m_dialogIndex;
    int m_textUncovered;                 // previous revealed character count
    std::vector<std::wstring> m_choices; // 24-byte text entries
    int m_hoveredChoice;                 // highlighted by Render
    Creature *m_creature;
    plasma::Vector<3, Fixed> m_anchorPosition;
    std::wstring m_font;
    int m_textSize;
    int m_strokeRadius;
    plasma::Vector<2, int> m_textCursor;
    int m_charactersShown;
    int m_characterTime; // milliseconds per character
    GameController *m_game;
};
}
