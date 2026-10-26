#pragma once
namespace cube {
// World::World : m_skillKeys labels..5 by movement/pet skill and 6..10 by class-ability position.
enum SkillId {
    SkillPetTaming = 0,
    SkillPetRiding = 1,
    SkillClimbing = 2,
    SkillHangGliding = 3,
    SkillSwimming = 4,
    SkillBoatDriving = 5,
    SkillClassAbilityFirst = 6,
    SkillClassAbilitySecond = 7,
    SkillClassAbilityThird = 8,
    SkillClassAbilityFourth = 9,
    SkillClassAbilityFifth = 10
};
// Number of entries, not an ID.
enum {
    SkillCount = 11
};
// The skill points: two per level after the first (and, in the alpha's skill screen, one per four
// mana cubes).
enum {
    SkillPointsPerLevel = 2
};
}
