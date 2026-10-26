// A single-line text editor, with a caret and a signed selection length.
#pragma once
#include "plasma/Widget.h"
#include "plasma/ScrollSlider.h"
#include "plasma/TextShape.h"

namespace plasma {

// Edit validation uses one virtual method and has no virtual destructor.
class EditFilter {
public:
    virtual bool validate(const std::wstring &text) = 0;
};

// Numeric input filters.
class IntegerFilter : public EditFilter {
public:
    virtual bool validate(const std::wstring &text);
};
class UnsignedIntegerFilter : public EditFilter {
public:
    virtual bool validate(const std::wstring &text);
};
class FloatFilter : public EditFilter {
public:
    virtual bool validate(const std::wstring &text);
};
class UnsignedFloatFilter : public EditFilter {
public:
    virtual bool validate(const std::wstring &text);
};

class Edit : public Widget {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    Edit(Engine *engine, Node *node, const std::wstring &name);
    Edit(const Edit &other, Node *node);
    virtual ~Edit();
    virtual void draw();
    virtual void initialize();
    virtual bool contains(const Vector<2, float> &point) { return true; }
    virtual void pointerPressed();
    // Releases the engine pointer capture.
    virtual void pointerReleased();
    virtual void pointerDoubleClicked();
    virtual void pointerMoved();
    virtual void keyPressed(unsigned short key);
    virtual void characterEntered(int character);
    virtual Widget *clone(Node *node);

    void clampSelection();
    void setCaretFromPoint(const Vector<2, float> &point);
    void ensureCaretVisible();
    void deleteSelection();

    TextShape *m_textShape;
    EditFilter *m_filter;
    bool m_selecting; // untouched by constructors
    Vector<2, float> m_initialTranslation;
    int m_caret;
    int m_selectionLength; // (signed relative to the caret)
};

}
