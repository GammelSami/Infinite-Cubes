// BlueprintPreviewWidget displays crafting formulas and requirements.
#pragma once
#include "plasma/Widget.h"
#include "cube/Formula.h"
namespace cube {
class GameController;
class BlueprintPreviewWidget : public plasma::Widget {
public:
    BlueprintPreviewWidget(plasma::Engine *engine, plasma::Node *node, GameController *game, plasma::Node *itemShadow,
                           plasma::Node *craftButton, plasma::Node *craftBar);
    virtual void draw();
    virtual void resized();
    void setProgress(float progress);

    bool isCraftButtonHovered() const { return m_craftButtonHovered; }
    plasma::Vector<3, float> m_resultItemRotation;
    Formula m_formula;
    GameController *m_game;
    plasma::Node *m_itemShadow;
    plasma::Node *m_craftButton;
    plasma::Node *m_craftBar;
    plasma::Node *m_craftProgressFill;
    int m_hoveredIngredientIndex;
    Item m_hoveredItem;
    bool m_craftButtonHovered;
};
}
