// QuestionWidget displays a conversation's answer dialogs in a vertical list, tracks the answer under the pointer and
// resizes to fit their measured text.
#pragma once
#include <vector>
#include "cube/widgets/BaseWidget.h"
#include "cube/Item.h"
#include "cube/widgets/SpeechWidget.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class QuestionWidget : public BaseWidget {
public:
    QuestionWidget(Game *game, plasma::Node *node);

    virtual void draw();

    // Resizes the widget (Widget::resizeScaled) to the widest answer plus 50 and the answers' total height plus 20.
    void fitToDialogs();
    // The size of one answer laid out as Render lays it out: the widest line and the height plus 32 (lines 18 apart).
    plasma::Vector<2, float> measureDialog(const Dialog &dialog);

    // The command of the answer under the pointer, or an empty string.
    std::string getSelectedAnswer()
    {
        if (m_hoveredDialog >= 0 && m_hoveredDialog < (int)m_dialogs.size())
            return m_dialogs[m_hoveredDialog].answer;
        return "";
    }

public:
    std::vector<Dialog> m_dialogs; // Answer dialogs drawn by draw.
    long long m_speakerId;         // Initially 0.
    Game *m_game;
    int m_unused210;                 // Initially 0.
    unsigned char m_unused214[0x28]; // ..: not initialised; no access
    float m_unused23c;               // Initially 1.0.
    bool m_unused240;                // true
    int m_unused244;                 // Initially 0.
    long long m_unused248;           // Initially 0.
    Item m_item;                     // Initially zero; unused in this file.
    bool m_unused2f0;                // true
    unsigned char m_unused2f1[0x7];  // ..: no access
    int m_hoveredDialog;             // Initially -1.
};

}
