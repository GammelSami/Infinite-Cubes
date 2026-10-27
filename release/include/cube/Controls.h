#pragma once
#include <string>

namespace cube {

struct Controls {
    // Indices of Controls::buttons, also used by controls.cfg.
    enum Action {
        ButtonAttack = 0,           // default (0, 1): left mouse button
        ButtonSpecialAttack = 1,    // (1, 1): right mouse button
        ButtonClassSkill = 2,       // (left shift)
        ButtonUltimateSkill = 3,    // (R)
        ButtonQuickItem = 4,        // (Q)
        ButtonUp = 5,               // (W)
        ButtonDown = 6,             // (S)
        ButtonLeft = 7,             // (A)
        ButtonRight = 8,            // (D)
        ButtonJump = 9,             // (space)
        ButtonDodge = 10,           // (2, 1): middle mouse button
        ButtonUse = 11,             // (E)
        ButtonLamp = 12,            // (F)
        ButtonWalk = 13,            // (left control)
        ButtonCallPet = 14,         // (T)
        ButtonWorldMap = 15,        // (M)
        ButtonRingMenu = 16,        // (tab)
        ButtonInventory = 17,       // (B)
        ButtonCraftingMenu = 18,    // (C)
        ButtonSystemMenu = 19,      // (O)
        ButtonMultiplayerMenu = 20, // (J)
        ButtonZoomIn = 21,          // no default (zero)
        ButtonZoomOut = 22,         // no default (zero)
        // The mod's alpha controls (src/mod): the class skills of the ability bar and the skill menu.
        ButtonSkill1 = 23,    // (1)
        ButtonSkill2 = 24,    // (2)
        ButtonSkill3 = 25,    // (3)
        ButtonSkillMenu = 26, // (X)
        ButtonCount = 27
    };

    // Binding devices: the frame loop reads keyboard scan codes or DIMOUSESTATE::rgbButtons; KeyNames displays mouse
    // bindings as "Mouse N".
    enum Device {
        DeviceKeyboard = 0, // code: a DirectInput scan code (DIK_*)
        DeviceMouse = 1     // code: an rgbButtons index (0 left, 1 right, 2 middle)
    };

    // One binding: a keyboard scan code or a mouse button number.
    struct Button {
        Button()
            : code(0),
              device(0)
        {}
        Button(int code, int device)
            : code(code),
              device(device)
        {}

        int code;
        int device;
    };

    Controls();

    // The display name of an action (L"Normal Attack"...), by Action index; L"Unknown <n>" for any other value.
    static std::wstring buttonName(int action);
    // Reads "controls.cfg" if present: lines "ButtonName code device". A line with an unrecognised name is skipped; a
    // missing number ends the reading.
    void load();
    // Stores the defaults listed above (the zoom bindings keep their value).
    void reset();
    // Writes "controls.cfg".
    void save();

    Button buttons[ButtonCount];
};

}
