// SkillWidget displays skill levels, learning and specialization.
#pragma once
#include "plasma/Widget.h"
#include "cube/SkillIds.h"
namespace cube {
class GameController;

class SkillWidget : public plasma::Widget {
public:
    SkillWidget(plasma::Engine *engine, plasma::Node *node, GameController *game);
    virtual void draw();
    // The right-hand button (Cancel), active while the levels or the specialization shown differ from
    // the player's.
    bool isPointerOverCancel();
    // The left-hand button (Learn), active while they differ, the player can pay for it, and a trainer
    // is near when it costs anything.
    bool isPointerOverLearn();
    // The coins applying the shown levels costs: lowering skills and changing the specialization are
    // paid for.
    int getCost();
    int m_skillLevels[SkillCount]; // compared with Creature::m_entityData.skillLevels
    int m_specialization;          // compared with Creature::m_entityData.specialization
    GameController *m_game;
};
}
