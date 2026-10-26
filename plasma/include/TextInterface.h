// TextInterface edits a text shape. A NodeInterface whose shape is a TextShape owns one. It keeps the text cursor,
// selection, and state captured when a modification begins; text edits and "set color key" record that state.
#pragma once
#include "TextModule.h"
#include <list>
#include <string>
#include "ShapeInterface.h"
#include "Operation.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/DiscreteAttribute.h"
#include "plasma/Vector.h"
#include "plasma/TextShape.h"

class BoundingBoxTool;

class TextInterface;
class DesignSheet;

// TextOperation: a change of a text shape, kept as its text state before and after.
class TextOperation : public Operation {
public:
    TextOperation(DesignSheet *sheet, const std::string &name, TextInterface *text, const TextState &before,
                  const TextState &after);

    virtual void execute();
    virtual void undo();

protected:
    TextState m_before;
    TextState m_after;
    TextInterface *m_text;
};

class TextInterface : public ShapeInterface {
public:
    TextInterface(NodeInterface *node, plasma::TextShape *shape);

    // Draws the text frame when selected, hovered with the node tool, or used as the edit root. Also draws wrap
    // markers and, during editing, the mouse caret, selection and blinking cursor.
    virtual void draw();
    virtual void getSnapOffsets(const plasma::Vector<2, float> &origin, std::list<plasma::Vector<2, float>> &offsets);
    virtual void paintColor(int colorTarget, bool update);
    virtual bool pickColor(plasma::Vector<4, float> &color, plasma::Vector<2, float> &position);

    // Adds colour 'colorTarget' if the mouse is over the text.
    virtual void getColorsUnderMouse(std::list<plasma::Vector<4, float>> &colors, int colorTarget);

    // Shifts colour 'colorTarget' towards the target (the colour tool).
    virtual void shiftColors(const std::list<plasma::Vector<4, float>> &source, const plasma::Vector<4, float> &target,
                             float tolerance, int colorTarget, bool update);

    // The stroke width tool (the stroke is 2 * m_strokeRadius wide).
    virtual bool pickStrokeWidth(float &width, plasma::Vector<2, float> &position);
    virtual void paintStrokeWidth();
    virtual void scaleStrokeWidths(float minimum, float maximum);
    virtual bool getStrokeWidthRange(float &minimum, float &maximum);

    // Colour changes of colour 'colorTarget' (0: fill, 1: stroke, 2: extrusion); the alpha is kept and the result
    // clamped to [0, 1].
    virtual void adjustBrightness(float factor, int colorTarget, bool update); // Scales it.
    virtual void adjustContrast(float factor, int colorTarget, bool update);   // Blends it with grey.
    virtual void adjustSaturation(float factor, int colorTarget, bool update); // Blends it with its luminance.

    // Key the colour, stroke colour and text at frame 0 of the current sequence ("set color key", "set stroke color
    // key", "set text key").
    virtual bool setVertexColorsKey();
    virtual bool setStrokeColorsKey();
    virtual bool setTextKey();

    // The text's bounds on the sheet.
    virtual void getBounds(plasma::Vector<2, float> *minimum, plasma::Vector<2, float> *maximum);
    virtual void beginModify(); // Keeps the state before the change.

    // Records the change since BeginModify ("modify text").
    virtual void endModify();

    // Records making the colours' element 1 current ("modify text").
    virtual void recordColorEdit();

    // With true: the colours' element 1 becomes a working copy of element 0 and the current one (m_colorPreview
    // records it).
    virtual void setColorPreview(bool preview);

    // Copies the working colours back to element 0 and makes it current.
    virtual void applyColorPreview();

    // Whether the text's bounds lie inside the tool's box.
    virtual bool isInsideBox(BoundingBoxTool *tool);

    // The fill colour, and the stroke colour if the text has a stroke.
    virtual void getColors(std::list<plasma::Vector<4, float>> &colors);

    // The text shape used by TextModule.
    plasma::TextShape *getShape() const { return m_shape; }

    // Copies the shape's text state.
    void captureState(TextState &state);

    // Gives the shape the text state back (TextOperation's Do and Undo).
    void restoreState(const TextState &state);
    std::wstring getSelectedText();

    // Types a character at the cursor, replacing the selection (control characters are ignored).
    void typeCharacter(wchar_t character);

    // A key went down while the text is edited (Workspace::KeyPressed): cursor movement (Shift selects), Delete,
    // Backspace, Escape, Return.
    void keyPressed(unsigned short key);

    // Inserts the pasted text at the cursor (Ctrl+V), up to its first line break.
    void insertText(const std::wstring &text);

    // Deletes the selection, or the character at the cursor without one.
    void deleteSelection();

    // Moves the cursor to the mouse and extends the selection by the distance moved.
    void moveCursorSelecting();

    // Puts the cursor at the character under the mouse.
    void setCursorToMouse();

    // The index of the character at the point (in the node's coordinates).
    int characterAt(const plasma::Vector<2, float> &point);
    void clearSelection();

    // The current value of colour 'colorTarget' (0: fill, 1: stroke, otherwise extrusion).
    plasma::Vector<4, float> &getColor(int colorTarget);

    // Constructor: state, shape, edited, signed selection length and cursor.
    TextState m_state; // The state when the modification began.
    plasma::TextShape *m_shape;
    bool m_edited;         // Text changed by typing, deletion or Return.
    int m_selectionLength; // Characters from the cursor; negative: before it.
    int m_cursor;          // Character index.
};

inline plasma::Vector<4, float> &TextInterface::getColor(int colorTarget)
{
    if (colorTarget == COLOR_TARGET_FILL)
        return m_shape->m_color.value();
    if (colorTarget == COLOR_TARGET_STROKE)
        return m_shape->m_strokeColor.value();
    return m_shape->m_extrusionColor.value();
}

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
