// Shared world-generation helpers.
#pragma once
#include "plasma/Vector.h"
namespace cube {
// PopulateRecord's race families; their styles also select guards and followers.
enum EncounterTheme {
    EncounterOrcs = 0,
    EncounterUndead = 1,
    EncounterHumans = 2,
    EncounterDwarves = 3,
    EncounterLizardmen = 4,
    EncounterGoblins = 5,
    EncounterPlantCreatures = 6,
    EncounterInsectoids = 7,
    EncounterBeetles = 8,
    EncounterLakeCreatures = 9,
    EncounterForestAnimals = 10
};
struct Item;
plasma::Vector<3, float> clampColor(const plasma::Vector<3, float> &color);
Item randomInventoryItem(int level, int rarity);
}
