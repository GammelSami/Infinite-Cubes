// ChatWidget draws chat messages and edits the outgoing text using a Plasma widget.
#pragma once
#include <list>
#include <string>
#include "plasma/Vector.h"
#include "plasma/Widget.h"

namespace plasma {
class Engine;
class Node;
class ScalableFont;
}

namespace cube {

class ChatWidget : public plasma::Widget {
public:
    struct MessageData {
        MessageData(std::wstring text, const plasma::Vector<3, unsigned char> &color)
            : text(text),
              color(color)
        {}

        std::wstring text;
        plasma::Vector<3, unsigned char> color; // RGB byte colour.
    };

    ChatWidget(plasma::Engine *engine, plasma::Node *node);

    virtual void draw();
    virtual plasma::Vector<2, float> getMaximum();
    virtual bool hasBounds() { return true; }

    // Inserts a character (>= ' ') at the cursor, replacing the selection.
    void typeCharacter(wchar_t character);
    // Cursor keys, Home/End (Shift selects), Delete, Backspace; sets m_changed.
    void handleKeyDown(unsigned short key);
    // The float colour is converted to bytes (x 255) for the byte version.
    void printMessage(const std::wstring &text, const plasma::Vector<4, float> &color);
    void printMessage(const std::wstring &text, const plasma::Vector<3, unsigned char> &color);

    // Clamps the cursor into the text and the selection into the text; returns the selection's far end (cursor +
    // length; unused by the callers).
    int clampSelection();
    // Erases the selection, or the character at the cursor when nothing is selected, and moves the cursor to its
    // start.
    void deleteSelection();

    std::list<std::list<MessageData>> m_messageSections; // Chat messages, oldest first.
    std::wstring m_typeboxText;                          // Game supplied at construction.
    bool m_typeboxActive;                                // Initially false; used by draw.
    plasma::ScalableFont *m_scalableFont;                // Font file resource1.dat used by draw.
    int m_typeboxCursorPosition;                         // Cursor position, initially zero and clamped to the text.
    int m_backwardsHighlightLength;                      // Selection length, initially zero.
};

}
