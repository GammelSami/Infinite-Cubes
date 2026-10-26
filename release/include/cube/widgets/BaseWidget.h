// BaseWidget provides text drawing with configurable font, size, spacing, colours, stroke and wrapping. Derived
// widgets set these properties before each text draw.
#pragma once
#include "plasma/TextIds.h"
#include <list>
#include <string>
#include <vector>
#include "plasma/Vector.h"
#include "plasma/Widget.h"
#include "cube/Dialog.h"

namespace plasma {
class Engine;
class Node;
class ScalableFont;
}

namespace cube {

class BaseWidget : public plasma::Widget {
public:
    BaseWidget(plasma::Engine *engine, plasma::Node *node, const std::wstring &name);

    void setFont(const std::wstring &path);
    // Setters for the text drawing settings below.
    void setTextColor(const plasma::Vector<4, float> &color);
    void setTextFlags(int flags);
    void setTextLineSpacing(float lineSpacing);
    void setTextSize(float size);
    void setTextStrokeColor(const plasma::Vector<4, float> &color);
    void setTextStrokeRadius(float radius);
    void setTextWrapWidth(float width);
    void measureText(const std::wstring &text, plasma::Vector<2, float> &minimum, plasma::Vector<2, float> &maximum);
    // Draws text at (x, y) plus the node's pivot, scaled by the model-view matrix: first the stroke alone when
    // m_textStrokeRadius > 0, then the fill; returns the end position back in the widget's units (zero without a
    // font).
    plasma::Vector<2, float> renderText(const std::wstring &text, float x, float y);
    // Draws the phrases of each dialog from (x, y), wrapping at width, each phrase in its colour; returns (x, the y
    // after the last line).
    plasma::Vector<2, float> renderTextLines(const std::vector<Dialog> &dialogs, float x, float y, float width);
    // renderText with each character on a moving wave (the engine's caret clock).
    plasma::Vector<2, float> renderWavyText(const std::wstring &text, float x, float y);

    plasma::ScalableFont *m_scalableFont;       // Font used by renderText; initially null.
    float m_textSize;                           // Text size used by renderText; initially 12.0.
    float m_textStrokeRadius;                   // Stroke radius used by renderText; initially zero.
    float m_textSpacing;                        // Character spacing; initially zero.
    float m_textLineSpacing;                    // Line leading; initially zero.
    float m_textWrapWidth;                      // Wrapping width; initially zero.
    plasma::Vector<4, float> m_textColor;       // Fill colour; initially white.
    plasma::Vector<4, float> m_textStrokeColor; // Outline colour; initially white.
    plasma::TextFlags m_textFlags;              // TextFlags used by renderText; initially zero.
};

}
