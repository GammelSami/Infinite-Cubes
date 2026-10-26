// PlasmaFont.cpp: glyph nodes and text layout for scene fonts.
#pragma once
#include "plasma/Font.h"

namespace plasma {

struct PlasmaGlyph {
    Vector<2, float> m_position;
    Vector<2, float> m_size;
    float m_advance;
    Node *m_node;
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
    void setGlyph(unsigned short character, const Vector<2, float> &position, const Vector<2, float> &size, Node *node,
                  float advance);
    PlasmaGlyph *getGlyph(unsigned short character);
    PlasmaFont *clone();

    std::map<unsigned short, PlasmaGlyph *> m_glyphs;
    Vector<2, float> m_minimum;
    Vector<2, float> m_maximum;
};

}
