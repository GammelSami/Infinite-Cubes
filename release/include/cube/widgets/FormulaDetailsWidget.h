// FormulaDetailsWidget displays a crafting formula's result, workstation, ingredients, craft button and progress bar.
// Game supplies the selected formula and crafts while the enabled button is held.
#pragma once
#include <vector>
#include "cube/widgets/BaseWidget.h"
#include "cube/Item.h"
#include "plasma/Vector.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class FormulaDetailsWidget : public BaseWidget {
public:
    FormulaDetailsWidget(Game *game, plasma::Node *node, plasma::Node *itemShadow, plasma::Node *craftButton,
                         plasma::Node *craftBar);

    virtual void draw();
    virtual void resized();

    // Shows the crafting progress (0..1 of the bar's width, clamped below at 0): resets the bar's deformation to the
    // identity, scales its x by (bar width - 8) * progress / the bar child's content width, and applies the
    // transformation.
    void setProgress(float progress);

    plasma::Vector<3, float> m_resultItemRotation; // Model render position, initially (-120, 0, 0).
    int m_unused1f4;                               // No access in the program.
    Formula m_formula;
    plasma::Vector<2, int> m_formulaCell; // Initially (-1, -1).
    Game *m_game;                         // Used by draw
    plasma::Node *m_itemShadow;           // Used by resized
    plasma::Node *m_craftButton;          // Used by draw (its widget's isPointerOver)
    plasma::Node *m_craftBar;             // Used by draw
    plasma::Node *m_craftBarBar;          // Craft progress bar's "bar" child; initially null.
    int m_hoveredIngredientIndex;         // Initially -1.
    Item m_hoveredItem;                   // Zero-initialised by the constructor.
    bool m_craftButtonHovered;            // Initially false.
};

}
