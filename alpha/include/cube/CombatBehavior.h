// CombatBehavior chooses attacks and movement against enemies.
#pragma once
#include "cube/Behavior.h"

namespace cube {

class CombatBehavior : public Behavior {
public:
    CombatBehavior(float distance);
    CombatBehavior(const CombatBehavior &other)
        : m_specialSkillDelay(other.m_specialSkillDelay),
          m_targetAcquisitionRadius(other.m_targetAcquisitionRadius),
          m_pursuitTime(0),
          m_targetDelay(0)
    {}
    virtual bool Update(Creature *creature, World *world, int elapsed, ServerUpdates *updates);
    virtual Behavior *clone();

    int m_specialSkillDelay;         // starts at 20000 milliseconds
    float m_targetAcquisitionRadius; // constructor argument
    int m_pursuitTime;               // milliseconds chasing an unreachable target; past 20000 it gives up
    int m_targetDelay;               // milliseconds before it looks for a new target again (3000 after giving up)
};

}
