#pragma once
#include <vector>
#include "cube/Item.h"
namespace cube {
// A formula consists of ingredients paired with their quantities.
struct Ingredient {
    Item item;
    int quantity;
};
// The station a formula needs.
enum Workstation {
    WorkstationNone = 0,
    WorkstationWater = 1,
    WorkstationType2 = 2, // no case in the alpha's switch
    WorkstationFurnace = 3,
    WorkstationAnvil = 4,
    WorkstationSpinningWheel = 5,
    WorkstationLoom = 6,
    WorkstationSaw = 7,
    WorkstationWorkbench = 8,
    WorkstationCampfire = 9
};
struct Formula {
    Formula()
        : workstationRequired(WorkstationNone)
    {}
    Item result;
    Workstation workstationRequired;
    std::vector<Ingredient> ingredients;
};
}
