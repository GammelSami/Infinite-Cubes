// PlasmaFont is a legacy font whose glyphs are scene nodes loaded from a Plasma document. Each character's node is
// positioned, scaled and drawn by renderText.
#pragma once
#include <map>
#include <string>
#include "plasma/Font.h"
#include "plasma/Vector.h"

namespace plasma {

class Engine;
class Node;

struct PlasmaGlyph {
    Vector<2, float> m_position;
    Vector<2, float> m_size;
    float m_advance; // added to the pen position after the glyph
    Node *m_node;    // Glyph node drawn by renderText.
};

class PlasmaFont : public Font {
public:
    PlasmaFont(Engine *engine, const std::wstring &name);
    virtual ~PlasmaFont();
    virtual void getCaret(const std::wstring &text, int index, Vector<2, float> &position, Vector<2, float> &size,
                          int flags);
    virtual void getBounds(const std::wstring &text, Vector<2, float> &minimum, Vector<2, float> &maximum, int flags);
    virtual Vector<2, float> getMinimum();
    virtual Vector<2, float> getMaximum();
    virtual void renderText(const std::wstring &text, const Vector<2, float> &position, const float *fill,
                            const float *outline, int flags);

    // Returns the glyph for character, or null if it is absent.
    PlasmaGlyph *getGlyph(wchar_t character);
    void setGlyph(wchar_t character, const Vector<2, float> &position, const Vector<2, float> &size, Node *node,
                  float advance);
    PlasmaFont *clone();

    std::map<wchar_t, PlasmaGlyph *> m_glyphs; // constructor, destructor
    Vector<2, float> m_minimum;                // getMinimum
    Vector<2, float> m_maximum;                // getMaximum
};

}
