// ID catalog companion to Interface.h.
#pragma once

namespace cube {

// interface-panels identifiers.
enum InterfacePanel {
    InterfacePanelEquipment = 0,   // Interface::showTab; ..1058
    InterfacePanelCrafting = 1,    // Interface::showTab; ..1058
    InterfacePanelMultiplayer = 2, // Interface::showTab; ..1058
};

enum StartMenuButton {
    StartMenuPlay = 0,
    StartMenuOptions = 1,
    StartMenuExit = 2,
    StartMenuContinue = 3,
};

enum SystemMenuButton {
    SystemMenuOptions = 0,
    SystemMenuControls = 1,
    SystemMenuStartMenu = 2,
    SystemMenuExit = 3,
};

// Interface::handleMouseDown/Up route 0,1,2 to primary, secondary, middle pointer events; main.cpp passes these values
// on WM_LBUTTON*, WM_RBUTTON*, WM_MBUTTON*.
enum MouseButton {
    MouseButton_Left = 0,
    MouseButton_Right = 1,
    MouseButton_Middle = 2,
};

// InventoryWidget::draw switches m_type to bag tabs, equipment, crafting tabs and vendor title; stack click branches
// confirm bag/crafting/vendor actions.
enum InventoryViewMode : int {
    InventoryViewBag = 0,
    InventoryViewEquipment = 1,
    InventoryViewCrafting = 2,
    InventoryViewVendor = 3,
};

enum InGameMenuButton {
    InGameMenuCrafting = 0,
    InGameMenuInventory = 1,
    InGameMenuWorldMap = 2,
    InGameMenuSystem = 3,
    InGameMenuMultiplayer = 4,
    InGameMenuHelp = 5,
};

// Each accepts the item category its member holds (Game::handleButtonUp's equip switch, the Creature stat sums):
// weapons ItemCategoryWeapon (shields and arrows left, two-handed right), rings ItemCategoryRing, then Neck, Shoulder,
// Chest, Hands, Feet and Pet.
enum EquipmentSlot {
    EquipmentSlotWeaponLeft = 0,  // "Left\nWeapon"; Equipment::weaponLeft
    EquipmentSlotWeaponRight = 1, // "Right\nWeapon"; Equipment::weaponRight
    EquipmentSlotRingLeft = 2,    // "Left Ring"; Equipment::ringLeft
    EquipmentSlotRingRight = 3,   // "Right Ring"; Equipment::ringRight
    EquipmentSlotNeck = 4,        // "Neck"; Equipment::neck
    EquipmentSlotShoulder = 5,    // "Shoulder"; Equipment::shoulder
    EquipmentSlotChest = 6,       // "Chest"; Equipment::chest
    EquipmentSlotHands = 7,       // "Hands"; Equipment::hands
    EquipmentSlotFeet = 8,        // "Feet"; Equipment::feet
    EquipmentSlotPet = 9,         // "Pet"; Equipment::pet
    EquipmentSlotCount = 10,
};

enum QuickSlot {
    QuickSlotItem = 0,   // the ring menu's selected food or potion, its count and name
    QuickSlotUse = 1,    // the interaction prompt: "Pick up", "Climb", "Revive", the offered item
    QuickSlotSkill = 2,  // the class's special skill (Creature::specialSkill) and its cooldown
    QuickSlotUnused = 3, // created and cleared each frame, never labelled, placed or filled
    QuickSlotCount = 4,
};

}
