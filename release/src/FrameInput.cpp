// The per-frame keyboard, mouse and gamepad handling of the frame loop, shared by the Windows (src/main.cpp) and
// Linux (platform/linux/main.cpp) entry points. Moved unchanged from src/main.cpp.
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <list>
#include <windows.h>
#include <mmsystem.h>
#include <dinput.h>
#include <xinput.h>

#include "FrameInput.h"
#include "cube/Game.h"
#include "plasma/D3D11Engine.h"
#include "cube/widgets/InventoryWidget.h"
#include "cube/widgets/FormulaDetailsWidget.h"
#include "cube/widgets/MultiplayerWidget.h"
#include "cube/widgets/VoxelWidget.h"
#include "cube/widgets/EnchantWidget.h"
#include "cube/widgets/SystemWidget.h"
#include "cube/widgets/OptionsWidget.h"
#include "cube/widgets/ControlsWidget.h"

// The thumb sticks count as moved in the menus beyond twice XInput's dead zones.
static const int kLeftStickThreshold = 2 * XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
static const int kRightStickThreshold = 2 * XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;

// Whether the pointer is over an open inventory panel.
#define POINTED_AT(widget) ((widget)->getNode()->isVisible() && (widget)->getNode()->isUnderPointer())

void assignControlFromDevices(cube::Game *game, const unsigned char keys[256], const DIMOUSESTATE &mouseState,
                              const unsigned char previousKeys[256], const DIMOUSESTATE &previousMouseState)
{
    // The first key or mouse button newly pressed becomes the binding; Escape and the
    // Enter keys only end the assignment.
    for (int key = 0; key < 256; key++) {
        if (!previousKeys[key] && keys[key]) {
            if (key != DIK_ESCAPE && key != DIK_RETURN && key != DIK_NUMPADENTER) {
                game->assignControl(
                    cube::Controls::Button(key, cube::Controls::DeviceKeyboard));
            }
            game->cancelBinding();
            break;
        }
        for (int button = 0; button < 4; button++) {
            if (!previousMouseState.rgbButtons[button] && mouseState.rgbButtons[button]) {
                game->assignControl(
                    cube::Controls::Button(button, cube::Controls::DeviceMouse));
            }
        }
    }
}

void applyDeviceBindings(cube::Game *game, const unsigned char keys[256], const DIMOUSESTATE &mouseState)
{
    for (int control = 0; control < cube::Controls::ButtonCount; control++) {
        game->setControl(control, false);
        const cube::Controls::Button &binding = game->m_controls.buttons[control];
        if (binding.device == cube::Controls::DeviceKeyboard && binding.code == 0) {
            continue;
        }
        if (binding.device == cube::Controls::DeviceKeyboard) {
            if (binding.code >= 0 && binding.code < 256) {
                bool pressed = keys[binding.code] != 0;
                game->setControl(control, pressed);
                if (pressed) {
                    game->m_gamepadActive = false;
                }
            } else {
                game->setControl(control, false);
            }
        } else if (binding.device == cube::Controls::DeviceMouse) {
            if (binding.code >= 0 && binding.code < 4) {
                bool pressed = mouseState.rgbButtons[binding.code] != 0;
                game->setControl(control, pressed);
                if (pressed) {
                    game->m_gamepadActive = false;
                }
            } else {
                game->setControl(control, false);
            }
        }
    }
}

void applyGamepad(cube::Game *game, plasma::D3D11Engine *engine)
{
    XINPUT_STATE pad;
    ZeroMemory(&pad, sizeof(XINPUT_STATE));
    game->setControl(cube::Controls::ButtonZoomIn, false);
    game->setControl(cube::Controls::ButtonZoomOut, false);
    if (XInputGetState(0, &pad) == ERROR_SUCCESS) {
        if (pad.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) {
            game->setControl(cube::Controls::ButtonZoomIn, true);
            game->m_gamepadActive = true;
        }
        if (pad.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
            game->setControl(cube::Controls::ButtonZoomOut, true);
            game->m_gamepadActive = true;
        }
        if (pad.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) {
            game->setControl(cube::Controls::ButtonCraftingMenu, true);
            game->setControl(cube::Controls::ButtonInventory, true);
            game->m_gamepadActive = true;
        }
        if (pad.Gamepad.wButtons & XINPUT_GAMEPAD_START) {
            game->setControl(cube::Controls::ButtonSystemMenu, true);
            game->m_gamepadActive = true;
        }
        if (pad.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
            game->setControl(cube::Controls::ButtonAttack, true);
            game->m_gamepadActive = true;
        }
        if (pad.Gamepad.bRightTrigger > 128) {
            game->setControl(cube::Controls::ButtonSpecialAttack, true);
            game->m_gamepadActive = true;
        }
        // The pad's buttons and stick states of the previous frame, and the timers of the
        // menu pointer (ms).
        static bool bWasDown;
        static bool aWasDown;
        static bool xWasDown;
        static int menuTime;
        static DWORD lastTime = timeGetTime();
        DWORD now = timeGetTime();
        int elapsed = now - lastTime;
        lastTime = now;
        if (game->m_interface.isInMenu() && !game->m_interface.m_mapOpen) {
            // A menu: the left stick steps the pointer from button to button, the right
            // stick scrolls or turns the character, the triggers change tabs, A and X
            // click.
            game->setStickMove(0.0f, 0.0f);
            menuTime += elapsed;
            static bool rightStickXMoved;
            static bool rightStickYMoved;
            static int scrollDelay;
            if (POINTED_AT(game->m_interface.m_inGame.m_equipmentInventoryWidget) ||
                POINTED_AT(game->m_interface.m_inGame.m_craftingInventoryWidget) ||
                POINTED_AT(game->m_interface.m_inGame.m_vendorInventoryWidget)) {
                scrollDelay -= elapsed;
                if (scrollDelay < 0) {
                    scrollDelay = 0;
                }
                if (scrollDelay == 0 && abs(pad.Gamepad.sThumbRY) > kRightStickThreshold) {
                    scrollDelay = !rightStickYMoved ? 400 : 100;
                    if (pad.Gamepad.sThumbRY < 0) {
                        game->handleMouseWheel(-1);
                    } else {
                        game->handleMouseWheel(1);
                    }
                }
                if (!rightStickXMoved &&
                    POINTED_AT(game->m_interface.m_inGame.m_equipmentInventoryWidget) &&
                    abs(pad.Gamepad.sThumbRX) > kRightStickThreshold) {
                    if (pad.Gamepad.sThumbRX < 0) {
                        game->m_interface.m_inGame.m_equipmentInventoryWidget
                            ->selectPreviousRegion();
                    } else {
                        game->m_interface.m_inGame.m_equipmentInventoryWidget
                            ->selectNextRegion();
                    }
                }
            } else {
                scrollDelay = 0;
                game->setStickTurn(pad.Gamepad.sThumbRX / 65536.0f * 2.0f,
                                   pad.Gamepad.sThumbRY / 65536.0f * 2.0f);
            }
            rightStickXMoved = abs(pad.Gamepad.sThumbRX) > kRightStickThreshold;
            rightStickYMoved = abs(pad.Gamepad.sThumbRY) > kRightStickThreshold;
            if (!rightStickYMoved) {
                scrollDelay = 0;
            }
            static bool leftTriggerWasDown;
            static bool rightTriggerWasDown;
            bool leftTrigger = pad.Gamepad.bLeftTrigger > 128;
            bool rightTrigger = pad.Gamepad.bRightTrigger > 128;
            if (leftTrigger && !leftTriggerWasDown) {
                if (POINTED_AT(game->m_interface.m_inGame.m_equipmentInventoryWidget)) {
                    game->m_interface.m_inGame.m_equipmentInventoryWidget
                        ->selectPreviousTab();
                }
                if (POINTED_AT(game->m_interface.m_inGame.m_craftingInventoryWidget)) {
                    game->m_interface.m_inGame.m_craftingInventoryWidget
                        ->selectPreviousTab();
                }
                if (POINTED_AT(game->m_interface.m_inGame.m_vendorInventoryWidget)) {
                    game->m_interface.m_inGame.m_vendorInventoryWidget->selectPreviousTab();
                }
            }
            if (rightTrigger && !rightTriggerWasDown) {
                if (POINTED_AT(game->m_interface.m_inGame.m_equipmentInventoryWidget)) {
                    game->m_interface.m_inGame.m_equipmentInventoryWidget->selectNextTab();
                }
                if (POINTED_AT(game->m_interface.m_inGame.m_craftingInventoryWidget)) {
                    game->m_interface.m_inGame.m_craftingInventoryWidget->selectNextTab();
                }
                if (POINTED_AT(game->m_interface.m_inGame.m_vendorInventoryWidget)) {
                    game->m_interface.m_inGame.m_vendorInventoryWidget->selectNextTab();
                }
            }
            leftTriggerWasDown = leftTrigger;
            rightTriggerWasDown = rightTrigger;
            float stepX = pad.Gamepad.sThumbLX / 65536.0f * 20.0f;
            float stepY = pad.Gamepad.sThumbLY / 65536.0f * 20.0f;
            static int stepDelay;
            static bool firstStep = true;
            stepDelay -= elapsed;
            if (stepDelay < 0) {
                stepDelay = 0;
            }
            if (abs(pad.Gamepad.sThumbLX) > kLeftStickThreshold ||
                abs(pad.Gamepad.sThumbLY) > kLeftStickThreshold) {
                game->m_gamepadActive = true;
                plasma::Vector<2, int> pointerPosition(engine->getPointer());
                float x = (float)pointerPosition[0];
                float y = (float)pointerPosition[1];
                std::list<plasma::Vector<2, int>> &buttons =
                    game->m_interface.m_buttonCenters;
                if (stepDelay == 0 && buttons.size() != 0) {
                    // Jumps to the nearest button in the stick's main direction: first
                    // only within 45 degrees of it, then anywhere on that side.
                    int pointerX = (int)x;
                    int pointerY = (int)y;
                    plasma::Vector<2, int> target(pointerX, pointerY);
                    int best = -1;
                    std::list<plasma::Vector<2, int>>::iterator it;
                    for (it = buttons.begin(); it != buttons.end(); ++it) {
                        if ((*it - plasma::Vector<2, int>(pointerX, pointerY))
                                .squaredLength() < 25) {
                            continue;
                        }
                        plasma::Vector<2, float> offset(
                            *it - plasma::Vector<2, int>(pointerX, pointerY));
                        int along = 1000;
                        int across = 1000;
                        if ((float)fabs(stepX) > (float)fabs(stepY)) {
                            if (stepX > 0.0f &&
                                (offset[0] <= 0.0f ||
                                 (float)fabs(offset[1] / offset[0]) > 1.0f)) {
                                continue;
                            }
                            if (stepX < 0.0f &&
                                (offset[0] >= 0.0f ||
                                 (float)fabs(offset[1] / offset[0]) > 1.0f)) {
                                continue;
                            }
                            along = (int)(float)fabs(offset[0]);
                            across = (int)(float)fabs(offset[1]);
                        }
                        if ((float)fabs(stepY) > (float)fabs(stepX)) {
                            if (stepY > 0.0f &&
                                (offset[1] >= 0.0f ||
                                 (float)fabs(offset[0] / offset[1]) > 1.0f)) {
                                continue;
                            }
                            if (stepY < 0.0f &&
                                (offset[1] <= 0.0f ||
                                 (float)fabs(offset[0] / offset[1]) > 1.0f)) {
                                continue;
                            }
                            along = (int)(float)fabs(offset[1]);
                            across = (int)(float)fabs(offset[0]);
                        }
                        int score = across * 100 + along;
                        if (best < 0 || score < best) {
                            target = *it;
                            best = score;
                        }
                    }
                    if (best < 0) {
                        for (it = buttons.begin(); it != buttons.end(); ++it) {
                            if ((*it - plasma::Vector<2, int>(pointerX, pointerY))
                                    .squaredLength() < 25) {
                                continue;
                            }
                            plasma::Vector<2, float> offset(
                                *it - plasma::Vector<2, int>(pointerX, pointerY));
                            int along = 1000;
                            int across = 1000;
                            if ((float)fabs(stepX) > (float)fabs(stepY)) {
                                if (stepX > 0.0f && offset[0] <= 0.0f) {
                                    continue;
                                }
                                if (stepX < 0.0f && offset[0] >= 0.0f) {
                                    continue;
                                }
                                along = (int)(float)fabs(offset[0]);
                                across = (int)(float)fabs(offset[1]);
                            }
                            if ((float)fabs(stepY) > (float)fabs(stepX)) {
                                if (stepY > 0.0f && offset[1] >= 0.0f) {
                                    continue;
                                }
                                if (stepY < 0.0f && offset[1] <= 0.0f) {
                                    continue;
                                }
                                along = (int)(float)fabs(offset[1]);
                                across = (int)(float)fabs(offset[0]);
                            }
                            int score = across * 10 + along;
                            if (best < 0 || score < best) {
                                target = *it;
                                best = score;
                            }
                        }
                    }
                    if (best > 0) {
                        game->playSound(cube::SoundMenuSelect);
                    }
                    x = (float)target[0];
                    y = (float)target[1];
                    stepDelay = firstStep ? 250 : 100;
                    firstStep = false;
                }
                if (buttons.size() == 0) {
                    x += stepX;
                    y -= stepY;
                }
                setPointerInClient((int)x, (int)y);
            } else {
                stepDelay = 0;
                firstStep = true;
            }
            if ((pad.Gamepad.wButtons & XINPUT_GAMEPAD_A) && !aWasDown && menuTime > 200) {
                game->handleButtonDown(0);
                game->m_gamepadActive = true;
                std::cout << menuTime << std::endl;
            }
            if (!(pad.Gamepad.wButtons & XINPUT_GAMEPAD_A) && aWasDown && menuTime > 200) {
                game->handleButtonUp(0);
                game->m_gamepadActive = true;
                std::cout << menuTime << std::endl;
            }
            if ((pad.Gamepad.wButtons & XINPUT_GAMEPAD_X) && !xWasDown) {
                game->handleButtonDown(1);
                game->m_gamepadActive = true;
            }
            if (!(pad.Gamepad.wButtons & XINPUT_GAMEPAD_X) && xWasDown) {
                game->handleButtonUp(1);
                game->m_gamepadActive = true;
            }
        } else {
            menuTime = 0;
            game->setStickMove(pad.Gamepad.sThumbLX / 65536.0f * 2.0f,
                               pad.Gamepad.sThumbLY / 65536.0f * 2.0f);
            game->setStickTurn(pad.Gamepad.sThumbRX / 65536.0f * 2.0f,
                               pad.Gamepad.sThumbRY / 65536.0f * 2.0f);
            // Tests the left stick's y and the right stick's x.
            if (abs(pad.Gamepad.sThumbLY) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
                abs(pad.Gamepad.sThumbRX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
                game->m_gamepadActive = true;
            }
            if (game->m_interface.m_mapOpen) {
                // The map: A, X and Y are the mouse buttons, the shoulder buttons step
                // through the visited lands.
                static bool yWasDown;
                static bool leftShoulderWasDown;
                static bool rightShoulderWasDown;
                if ((pad.Gamepad.wButtons & XINPUT_GAMEPAD_A) && !aWasDown) {
                    game->handleButtonDown(0);
                    game->m_gamepadActive = true;
                }
                if (!(pad.Gamepad.wButtons & XINPUT_GAMEPAD_A) && aWasDown) {
                    game->handleButtonUp(0);
                    game->m_gamepadActive = true;
                }
                if ((pad.Gamepad.wButtons & XINPUT_GAMEPAD_X) && !xWasDown) {
                    game->handleButtonDown(1);
                    game->m_gamepadActive = true;
                }
                if (!(pad.Gamepad.wButtons & XINPUT_GAMEPAD_X) && xWasDown) {
                    game->handleButtonUp(1);
                    game->m_gamepadActive = true;
                }
                if ((pad.Gamepad.wButtons & XINPUT_GAMEPAD_Y) && !yWasDown) {
                    game->handleButtonDown(2);
                    game->m_gamepadActive = true;
                }
                if (!(pad.Gamepad.wButtons & XINPUT_GAMEPAD_Y) && yWasDown) {
                    game->handleButtonUp(2);
                    game->m_gamepadActive = true;
                }
                yWasDown = (pad.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
                if ((pad.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) &&
                    !leftShoulderWasDown) {
                    game->m_interface.nextLand();
                    game->m_gamepadActive = true;
                }
                leftShoulderWasDown =
                    (pad.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
                if ((pad.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) &&
                    !rightShoulderWasDown) {
                    game->m_interface.lastLand();
                    game->m_gamepadActive = true;
                }
                rightShoulderWasDown =
                    (pad.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
            } else {
                if (pad.Gamepad.wButtons & XINPUT_GAMEPAD_Y) {
                    game->setControl(cube::Controls::ButtonUltimateSkill, true);
                    game->m_gamepadActive = true;
                }
                if ((pad.Gamepad.wButtons & XINPUT_GAMEPAD_B) && !bWasDown &&
                    !game->m_interface.isInMenu()) {
                    game->setControl(cube::Controls::ButtonJump, true);
                    game->m_gamepadActive = true;
                }
                if (pad.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) {
                    game->setControl(cube::Controls::ButtonDodge, true);
                    game->m_gamepadActive = true;
                }
                if (pad.Gamepad.bLeftTrigger > 128) {
                    game->setControl(cube::Controls::ButtonClassSkill, true);
                    game->m_gamepadActive = true;
                }
                if (pad.Gamepad.wButtons & XINPUT_GAMEPAD_X) {
                    game->setControl(cube::Controls::ButtonQuickItem, true);
                    game->m_gamepadActive = true;
                }
                if (pad.Gamepad.wButtons & XINPUT_GAMEPAD_A) {
                    game->setControl(cube::Controls::ButtonUse, true);
                    game->m_gamepadActive = true;
                }
            }
            if (pad.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
                game->setControl(cube::Controls::ButtonLamp, true);
                game->m_gamepadActive = true;
            }
            if (pad.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) {
                game->setControl(cube::Controls::ButtonCallPet, true);
                game->m_gamepadActive = true;
            }
            if (pad.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) {
                game->setControl(cube::Controls::ButtonRingMenu, true);
                game->m_gamepadActive = true;
            }
            if (pad.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
                game->setControl(cube::Controls::ButtonWorldMap, true);
                game->m_gamepadActive = true;
            }
            // The dead zones are applied after the sticks were used; nothing reads the
            // state again.
            if (pad.Gamepad.sThumbLX < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
                pad.Gamepad.sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
                pad.Gamepad.sThumbLX = 0;
            }
            if (pad.Gamepad.sThumbLY < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
                pad.Gamepad.sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
                pad.Gamepad.sThumbLY = 0;
            }
            if (pad.Gamepad.sThumbRX < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
                pad.Gamepad.sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
                pad.Gamepad.sThumbRX = 0;
            }
            if (pad.Gamepad.sThumbRY < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
                pad.Gamepad.sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
                pad.Gamepad.sThumbRY = 0;
            }
        }
        aWasDown = (pad.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
        xWasDown = (pad.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
        // B closes every panel and menu.
        if (game->m_interface.isInMenu() && (pad.Gamepad.wButtons & XINPUT_GAMEPAD_B)) {
            bWasDown = true;
            game->m_interface.m_mapOpen = false;
            game->m_interface.m_inGame.m_equipmentInventoryWidget->getNode()->setVisible(
                false);
            game->m_interface.m_inGame.m_vendorInventoryWidget->getNode()->setVisible(
                false);
            game->m_interface.m_inGame.m_craftingInventoryWidget->getNode()->setVisible(
                false);
            game->m_interface.m_inGame.m_formulaDetailsWidget->getNode()->setVisible(false);
            game->m_interface.m_inGame.m_multiplayerWidget->getNode()->setVisible(false);
            game->m_interface.m_inGame.m_voxelWidget->getNode()->setVisible(false);
            game->m_interface.m_inGame.m_enchantWidget->getNode()->setVisible(false);
            game->m_interface.m_inGame.m_menuOpen = false;
            game->m_interface.m_inGame.m_systemWidget->getNode()->setVisible(false);
            game->m_interface.m_optionsWidget->getNode()->setVisible(false);
            game->m_interface.m_controlsWidget->getNode()->setVisible(false);
        }
        if (!(pad.Gamepad.wButtons & XINPUT_GAMEPAD_B)) {
            bWasDown = false;
        }
    } else {
        game->setStickMove(0.0f, 0.0f);
        game->setStickTurn(0.0f, 0.0f);
    }
}
