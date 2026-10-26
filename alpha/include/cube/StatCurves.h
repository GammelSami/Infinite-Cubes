// Shared stat curves for creatures and items.
#pragma once

namespace cube {
float skillBonus(int points);
int abilitySkillIndex(int ability);
float levelBonus(float level);
float bonusFromLevel(int points);
float climbingSpeed(int points);
float hangGlidingSpeed(int points);
float itemPowerScale(float level, int rarity);
float itemDamageScale(float level, int rarity);
float levelFromBonus(float bonus);
float ridingSpeed(int points);
float swimmingSpeed(int points);
}
