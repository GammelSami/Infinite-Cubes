#include "cube/Noise.h"
#include "cube/Controls.h"
#include "cube/InterfaceIds.h"
#include <dinput.h>
#include <fstream>
#include <string>

namespace cube {

Controls::Controls()
{
    reset();
}

std::wstring Controls::buttonName(int action)
{
    switch (action) {
    case ButtonAttack:
        return L"Normal Attack";
    case ButtonSpecialAttack:
        return L"Special Attack";
    case ButtonUp:
        return L"Move Forward";
    case ButtonDown:
        return L"Move Backward";
    case ButtonLeft:
        return L"Move Left";
    case ButtonRight:
        return L"Move Right";
    case ButtonJump:
        return L"Jump";
    case ButtonDodge:
        return L"Dodge";
    case ButtonUse:
        return L"Use/Interact";
    case ButtonLamp:
        return L"Lamp";
    case ButtonUltimateSkill:
        return L"Ultimate Skill";
    case ButtonClassSkill:
        return L"Class Skill";
    case ButtonWalk:
        return L"Walk";
    case ButtonCallPet:
        return L"Call Pet";
    case ButtonWorldMap:
        return L"World Map";
    case ButtonQuickItem:
        return L"Use Quick Item";
    case ButtonRingMenu:
        return L"Ring Menu";
    case ButtonInventory:
        return L"Inventory";
    case ButtonMultiplayerMenu:
        return L"Multiplayer Menu";
    case ButtonCraftingMenu:
        return L"Crafting Menu";
    case ButtonSystemMenu:
        return L"System Menu";
    case ButtonZoomIn:
        return L"Zoom In";
    case ButtonZoomOut:
        return L"Zoom Out";
    default:
        return L"Unknown " + std::to_wstring(action);
    }
}

void Controls::load()
{
    std::ifstream file("controls.cfg");
    while (file.good()) {
        std::string key;
        file >> key;
        if (!file.good()) {
            break;
        }
        int index = -1;
        if (key == "ButtonAttack") {
            index = ButtonAttack;
        }
        if (key == "ButtonSpecialAttack") {
            index = ButtonSpecialAttack;
        }
        if (key == "ButtonQuickItem") {
            index = ButtonQuickItem;
        }
        if (key == "ButtonUp") {
            index = ButtonUp;
        }
        if (key == "ButtonDown") {
            index = ButtonDown;
        }
        if (key == "ButtonLeft") {
            index = ButtonLeft;
        }
        if (key == "ButtonRight") {
            index = ButtonRight;
        }
        if (key == "ButtonJump") {
            index = ButtonJump;
        }
        if (key == "ButtonDodge") {
            index = ButtonDodge;
        }
        if (key == "ButtonUse") {
            index = ButtonUse;
        }
        if (key == "ButtonLamp") {
            index = ButtonLamp;
        }
        if (key == "ButtonUltimateSkill") {
            index = ButtonUltimateSkill;
        }
        if (key == "ButtonClassSkill") {
            index = ButtonClassSkill;
        }
        if (key == "ButtonWalk") {
            index = ButtonWalk;
        }
        if (key == "ButtonCallPet") {
            index = ButtonCallPet;
        }
        if (key == "ButtonWorldMap") {
            index = ButtonWorldMap;
        }
        if (key == "ButtonRingMenu") {
            index = ButtonRingMenu;
        }
        if (key == "ButtonInventory") {
            index = ButtonInventory;
        }
        if (key == "ButtonMultiplayerMenu") {
            index = ButtonMultiplayerMenu;
        }
        if (key == "ButtonCraftingMenu") {
            index = ButtonCraftingMenu;
        }
        if (key == "ButtonSystemMenu") {
            index = ButtonSystemMenu;
        }
        if (key == "ButtonZoomIn") {
            index = ButtonZoomIn;
        }
        if (key == "ButtonZoomOut") {
            index = ButtonZoomOut;
        }
        if (index < 0 || index >= ButtonCount) {
            continue;
        }
        int code = 0;
        file >> code;
        if (!file.good()) {
            break;
        }
        int device = 0;
        file >> device;
        if (!file.good()) {
            break;
        }
        buttons[index].code = code;
        buttons[index].device = device;
    }
}

void Controls::reset()
{
    buttons[ButtonAttack] = Button(MouseButton_Left, DeviceMouse);
    buttons[ButtonSpecialAttack] = Button(MouseButton_Right, DeviceMouse);
    buttons[ButtonQuickItem] = Button(DIK_Q, DeviceKeyboard);
    buttons[ButtonUp] = Button(DIK_W, DeviceKeyboard);
    buttons[ButtonDown] = Button(DIK_S, DeviceKeyboard);
    buttons[ButtonLeft] = Button(DIK_A, DeviceKeyboard);
    buttons[ButtonRight] = Button(DIK_D, DeviceKeyboard);
    buttons[ButtonJump] = Button(DIK_SPACE, DeviceKeyboard);
    buttons[ButtonDodge] = Button(MouseButton_Middle, DeviceMouse);
    buttons[ButtonUse] = Button(DIK_E, DeviceKeyboard);
    buttons[ButtonLamp] = Button(DIK_F, DeviceKeyboard);
    buttons[ButtonUltimateSkill] = Button(DIK_R, DeviceKeyboard);
    buttons[ButtonWalk] = Button(DIK_LCONTROL, DeviceKeyboard);
    buttons[ButtonClassSkill] = Button(DIK_LSHIFT, DeviceKeyboard);
    buttons[ButtonCallPet] = Button(DIK_T, DeviceKeyboard);
    buttons[ButtonWorldMap] = Button(DIK_M, DeviceKeyboard);
    buttons[ButtonRingMenu] = Button(DIK_TAB, DeviceKeyboard);
    buttons[ButtonInventory] = Button(DIK_B, DeviceKeyboard);
    buttons[ButtonMultiplayerMenu] = Button(DIK_J, DeviceKeyboard);
    buttons[ButtonCraftingMenu] = Button(DIK_C, DeviceKeyboard);
    buttons[ButtonSystemMenu] = Button(DIK_O, DeviceKeyboard);
}

void Controls::save()
{
    std::ofstream file("controls.cfg");
    file << "ButtonAttack " << buttons[ButtonAttack].code << " " << buttons[ButtonAttack].device << std::endl;
    file << "ButtonSpecialAttack " << buttons[ButtonSpecialAttack].code << " " << buttons[ButtonSpecialAttack].device
         << std::endl;
    file << "ButtonQuickItem " << buttons[ButtonQuickItem].code << " " << buttons[ButtonQuickItem].device << std::endl;
    file << "ButtonUp " << buttons[ButtonUp].code << " " << buttons[ButtonUp].device << std::endl;
    file << "ButtonDown " << buttons[ButtonDown].code << " " << buttons[ButtonDown].device << std::endl;
    file << "ButtonLeft " << buttons[ButtonLeft].code << " " << buttons[ButtonLeft].device << std::endl;
    file << "ButtonRight " << buttons[ButtonRight].code << " " << buttons[ButtonRight].device << std::endl;
    file << "ButtonJump " << buttons[ButtonJump].code << " " << buttons[ButtonJump].device << std::endl;
    file << "ButtonDodge " << buttons[ButtonDodge].code << " " << buttons[ButtonDodge].device << std::endl;
    file << "ButtonUse " << buttons[ButtonUse].code << " " << buttons[ButtonUse].device << std::endl;
    file << "ButtonLamp " << buttons[ButtonLamp].code << " " << buttons[ButtonLamp].device << std::endl;
    file << "ButtonUltimateSkill " << buttons[ButtonUltimateSkill].code << " " << buttons[ButtonUltimateSkill].device
         << std::endl;
    file << "ButtonClassSkill " << buttons[ButtonClassSkill].code << " " << buttons[ButtonClassSkill].device
         << std::endl;
    file << "ButtonWalk " << buttons[ButtonWalk].code << " " << buttons[ButtonWalk].device << std::endl;
    file << "ButtonCallPet " << buttons[ButtonCallPet].code << " " << buttons[ButtonCallPet].device << std::endl;
    file << "ButtonWorldMap " << buttons[ButtonWorldMap].code << " " << buttons[ButtonWorldMap].device << std::endl;
    file << "ButtonRingMenu " << buttons[ButtonRingMenu].code << " " << buttons[ButtonRingMenu].device << std::endl;
    file << "ButtonInventory " << buttons[ButtonInventory].code << " " << buttons[ButtonInventory].device << std::endl;
    file << "ButtonMultiplayerMenu " << buttons[ButtonMultiplayerMenu].code << " "
         << buttons[ButtonMultiplayerMenu].device << std::endl;
    file << "ButtonCraftingMenu " << buttons[ButtonCraftingMenu].code << " " << buttons[ButtonCraftingMenu].device
         << std::endl;
    file << "ButtonSystemMenu " << buttons[ButtonSystemMenu].code << " " << buttons[ButtonSystemMenu].device
         << std::endl;
    file << "ButtonZoomIn " << buttons[ButtonZoomIn].code << " " << buttons[ButtonZoomIn].device << std::endl;
    file << "ButtonZoomOut " << buttons[ButtonZoomOut].code << " " << buttons[ButtonZoomOut].device << std::endl;
}

}
