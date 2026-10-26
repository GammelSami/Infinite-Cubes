// A single-line text editor, with a caret and a signed selection length.
#pragma once
#include "plasma/Widget.h"
#include "plasma/ScrollSlider.h"

namespace plasma {

// Edit validation uses a virtual method without a virtual destructor.
class EditFilter {
public:
    virtual bool validate(const std::wstring &text) = 0;
};

// The edit box input filters (IntegerFilter, UnsignedIntegerFilter, FloatFilter, UnsignedFloatFilter, all
// EditFilter, which this header calls EditFilter). The modules define their own filter objects (FormatModule.h).
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
    // Releases pointer capture through the shared implementation.
    virtual void pointerReleased();
    virtual void pointerDoubleClicked();
    virtual void pointerMoved();
    virtual void keyPressed(unsigned short key);
    virtual void characterEntered(int character);
    virtual Widget *clone(Node *node);

    std::wstring &getText();
    void insertText(const std::wstring &text);
    void clampSelection();
    std::wstring getSelectedText();
    void selectAll();
    void setCaretFromPoint(const Vector<2, float> &point);
    void ensureCaretVisible();
    void deleteSelection();

    TextShape *m_textShape;
    EditFilter *m_filter;
    bool m_selecting; // Untouched by constructors.
    Vector<2, float> m_initialTranslation;
    int m_caret;
    int m_selectionLength; // (signed relative to the caret).
};

}
