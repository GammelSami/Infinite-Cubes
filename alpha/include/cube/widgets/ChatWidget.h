// ChatWidget displays chat messages.
#pragma once
#include <list>
#include <string>
#include "plasma/Widget.h"
namespace plasma {
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
        plasma::Vector<3, unsigned char> color;
    };
    ChatWidget(plasma::Engine *engine, plasma::Node *node);
    virtual void draw();
    virtual plasma::Vector<2, float> getMaximum();
    virtual bool hasBounds() { return true; }
    void typeCharacter(wchar_t character);
    void handleKeyDown(unsigned short key);
    void printMessage(const std::wstring &, const plasma::Vector<3, unsigned char> &);
    void printMessage(const std::wstring &, const plasma::Vector<4, float> &);
    void clampSelection();
    void deleteSelection();
    std::list<std::list<MessageData>> m_messageSections;
    std::wstring m_typeboxText;
    bool m_typeboxActive;
    plasma::ScalableFont *m_scalableFont;
    int m_typeboxCursorPosition;
    int m_backwardsHighlightLength; // signed length from cursor
};
}
