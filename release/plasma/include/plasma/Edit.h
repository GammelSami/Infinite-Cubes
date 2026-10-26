// Edit provides single-line editing on a TextShape node, with a caret, signed selection length and optional EditFilter
// to validate changes.
#pragma once
#include <string>
#include "plasma/Widget.h"
#include "plasma/Vector.h"

namespace plasma {

class EditFilter;
class Engine;
class Node;
class TextShape;

class Edit : public Widget {
public:
    Edit(Engine *engine, Node *node, const std::wstring &name);
    Edit(const Edit &other, Node *node); // Copied by clone.
    virtual ~Edit();

    virtual void draw();
    // The whole edit takes the pointer.
    virtual bool contains(const Vector<2, float> &point) { return true; }
    // Finds the node's TextShape (shape type 3), clamps the selection, turns on the parent's display clipping and
    // records the node's translation in m_initialTranslation. Does not call Widget::initialize.
    virtual void initialize();
    virtual void pointerPressed();
    virtual void pointerReleased();
    // Selects all text.
    virtual void pointerDoubleClicked();
    // Extends the drag selection while the left button is down.
    virtual void pointerMoved();
    // Return, Left, Right, Delete, Backspace, Home, End (Shift extends).
    virtual void keyPressed(unsigned short key);
    virtual void characterEntered(wchar_t character);
    virtual Widget *clone(Node *node);

    // Deletes the selection, or the character after the caret when nothing is selected (keyPressed's Delete,
    // characterEntered).
    void deleteSelection();
    // Scrolls the text node so that the caret stays visible.
    void ensureCaretVisible();
    // Moves the caret to the character nearest point (node space).
    void setCaretFromPoint(const Vector<2, float> &point);
    // Keeps the caret and the selection inside the text.
    void clampSelection();

protected:
    TextShape *m_textShape; // Node's TextShape, found by initialize.
    EditFilter *m_filter;   // Optional input validation filter, copied by clone.
    bool
        m_selecting; // Whether a drag selection is active; set by pointerPressed and pointerMoved, not initialised by the constructors.
    Vector<2, float> m_initialTranslation; // the node's translation at initialize
    int m_caret;
    int m_selectionLength; // signed, relative to the caret
};

}
