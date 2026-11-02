// The native Linux entry point (src/main.cpp's WinMain on Windows): SDL3 opens the window and reads
// keyboard, mouse and gamepad, DXVK-native provides Direct3D 9 on Vulkan for that window and FAudio plays the
// XAudio2 sound. The frame loop, the device setup and the input the game receives are WinMain's: key
// events carry Windows virtual-key codes, the held keys and buttons are polled each frame as
// DirectInput did, and while the game hides the cursor the mouse reports relative motion.
#include <SDL3/SDL.h>
#include <windows.h>
#include <winsock2.h>
#include <d3d9.h>
#include <direct.h>
#include <malloc.h>
#include <unistd.h>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

#include "plasma/D3D9Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/Node.h"
#include "plasma/StringConversion.h"
#include "plasma/Vector.h"
#include "cube/CubeShader.h"
#include "cube/Creature.h"
#include "cube/CreatureIds.h"
#include "cube/GameController.h"
#include "cube/DefaultBlocks.h"
#include "cube/Options.h"
#include "cube/XAudio2Engine.h"

using namespace cube;

// A failed reset is retried every 100 ms.
namespace {
enum {
    DeviceResetRetryDelay = 100
};
}

IDirect3DDevice9 *g_device;

IDirect3D9 *g_direct3d;

CubeShader *g_shader;

SDL_Window *g_window;

GameController *g_game;

plasma::D3D9Engine *g_engine;

bool g_cursorShown; // the cursor was free in the last frame
bool g_deviceLost;  // Present reported D3DERR_DEVICELOST.
// The global options hold the display settings in use.
Options options;

bool g_readInput = true; // never cleared

// The back buffer's size; the mouse position is scaled from window points to it.
int g_backBufferWidth;
int g_backBufferHeight;

// Relative mouse motion since the last frame, while the game hides the cursor.
float g_mouseDeltaX;
float g_mouseDeltaY;
float g_wheel; // scrolled distance not yet passed on as whole steps

void resetDevice(bool unused);

static void showError(const char *message)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Cube World", message, g_window);
}

// The window's drawable size in pixels (GetClientRect).
static void clientSize(int &width, int &height)
{
    width = 0;
    height = 0;
    SDL_GetWindowSizeInPixels(g_window, &width, &height);
}

static bool hasFocus()
{
    return (SDL_GetWindowFlags(g_window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

// The Windows virtual-key code of an SDL key (WM_KEYDOWN's wParam), or 0. Letters and digits follow
// the keyboard layout, as virtual keys do.
static unsigned char virtualKey(const SDL_KeyboardEvent &event)
{
    SDL_Keycode key = event.key;
    if (key >= SDLK_A && key <= SDLK_Z)
        return (unsigned char)('A' + (key - SDLK_A));
    if (key >= SDLK_0 && key <= SDLK_9)
        return (unsigned char)('0' + (key - SDLK_0));
    if (key >= SDLK_F1 && key <= SDLK_F12)
        return (unsigned char)(VK_F1 + (key - SDLK_F1));
    if (key >= SDLK_KP_1 && key <= SDLK_KP_9)
        return (unsigned char)(VK_NUMPAD0 + 1 + (key - SDLK_KP_1));
    switch (key) {
    case SDLK_BACKSPACE: return VK_BACK;
    case SDLK_TAB: return VK_TAB;
    case SDLK_RETURN:
    case SDLK_KP_ENTER: return VK_RETURN;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT: return VK_SHIFT;
    case SDLK_LCTRL:
    case SDLK_RCTRL: return VK_CONTROL;
    case SDLK_PAUSE: return VK_PAUSE;
    case SDLK_CAPSLOCK: return VK_CAPITAL;
    case SDLK_ESCAPE: return VK_ESCAPE;
    case SDLK_SPACE: return VK_SPACE;
    case SDLK_PAGEUP: return VK_PRIOR;
    case SDLK_PAGEDOWN: return VK_NEXT;
    case SDLK_END: return VK_END;
    case SDLK_HOME: return VK_HOME;
    case SDLK_LEFT: return VK_LEFT;
    case SDLK_UP: return VK_UP;
    case SDLK_RIGHT: return VK_RIGHT;
    case SDLK_DOWN: return VK_DOWN;
    case SDLK_INSERT: return VK_INSERT;
    case SDLK_DELETE: return VK_DELETE;
    case SDLK_KP_0: return VK_NUMPAD0;
    case SDLK_KP_MULTIPLY: return VK_MULTIPLY;
    case SDLK_KP_PLUS: return VK_ADD;
    case SDLK_KP_MINUS: return VK_SUBTRACT;
    case SDLK_KP_PERIOD: return VK_DECIMAL;
    case SDLK_KP_DIVIDE: return VK_DIVIDE;
    case SDLK_SEMICOLON: return VK_OEM_1;
    case SDLK_PLUS:
    case SDLK_EQUALS: return VK_OEM_PLUS;
    case SDLK_COMMA: return VK_OEM_COMMA;
    case SDLK_MINUS: return VK_OEM_MINUS;
    case SDLK_PERIOD: return VK_OEM_PERIOD;
    case SDLK_SLASH: return VK_OEM_2;
    case SDLK_GRAVE: return VK_OEM_3;
    case SDLK_LEFTBRACKET: return VK_OEM_4;
    case SDLK_BACKSLASH: return VK_OEM_5;
    case SDLK_RIGHTBRACKET: return VK_OEM_6;
    case SDLK_APOSTROPHE: return VK_OEM_7;
    }
    // The key left of 1 and the extra key next to the left shift, by position (VK_OEM_3, VK_OEM_102).
    if (event.scancode == SDL_SCANCODE_GRAVE)
        return VK_OEM_3;
    if (event.scancode == SDL_SCANCODE_NONUSBACKSLASH)
        return VK_OEM_102;
    return 0;
}

// The mouse position in back-buffer pixels (window points scaled to the back buffer).
static void cursorPosition(float &x, float &y)
{
    SDL_GetMouseState(&x, &y);
    int width = 0, height = 0;
    SDL_GetWindowSize(g_window, &width, &height);
    if (width > 0 && height > 0 && g_backBufferWidth > 0 && g_backBufferHeight > 0) {
        x *= (float)g_backBufferWidth / width;
        y *= (float)g_backBufferHeight / height;
    }
}

// The gamepad (Windows had none: the game's stick inputs were never set). It acts as more keys and mouse
// buttons, laid out like the release's own XInput layout where the alpha has the same action (see
// platform/linux/README.md for the table). In play the left stick fills m_moveStick and the right stick
// turns the camera like the mouse; while the cursor is free the left stick moves it, A and X click and B
// closes the open window.
SDL_Gamepad *g_gamepad;
bool g_padPointer;   // the pad moved the free cursor last; the mouse's position is ignored until it moves
float g_padPointerX; // the pad's cursor in back-buffer pixels
float g_padPointerY;
bool g_padHeld[InputCount];             // inputs the pad holds this frame
bool g_padButtons[3];                   // mouse buttons the pad holds (plasma::MOUSE_BUTTON_*)
plasma::Vector<2, int> g_padMove(0, 0); // m_moveStick (-32767..32767, up positive as in XInput)

namespace {
enum {
    PadDeadZone = 8000,         // of 32767, for both sticks
    PadTriggerThreshold = 8000, // of 32767
    PadFirstRepeat = 300,       // ms before a held wheel direction repeats
    PadRepeat = 100,            // ms between repeated wheel steps
    PadLeftTrigger = SDL_GAMEPAD_BUTTON_COUNT, // bit numbers of the triggers next to the buttons'
    PadRightTrigger
};
// Degrees per second the camera turns at full deflection, whatever the mouse's camera speed option.
const float PadLookSpeed = 180.0f;
// Window heights per second the free cursor moves at full deflection.
const float PadPointerSpeed = 1.2f;
}

// A stick with a round dead zone: (0, 0) inside it, outside rescaled to start at 0 and reach length 1.
// y grows downwards, as SDL reports it.
static void padStick(SDL_GamepadAxis xAxis, SDL_GamepadAxis yAxis, float &x, float &y)
{
    x = SDL_GetGamepadAxis(g_gamepad, xAxis) / 32767.0f;
    y = SDL_GetGamepadAxis(g_gamepad, yAxis) / 32767.0f;
    float length = sqrtf(x * x + y * y);
    const float deadZone = PadDeadZone / 32767.0f;
    if (length <= deadZone) {
        x = 0.0f;
        y = 0.0f;
        return;
    }
    float scaled = (length - deadZone) / (1.0f - deadZone);
    if (scaled > 1.0f)
        scaled = 1.0f;
    x *= scaled / length;
    y *= scaled / length;
}

// A key event as the keyboard sends it.
static void padKey(unsigned char key, bool down)
{
    if (down) {
        g_engine->keyPressed(key);
        g_game->handleKeyDown(key);
    } else {
        g_engine->keyReleased(key);
        g_game->handleKeyUp(key);
    }
}

// Reads the pad once per frame: sends key and button events for changed outputs, scrolls, moves the free
// cursor or adds the camera turn to the mouse motion, and fills g_padHeld, g_padButtons and g_padMove for
// the input poll. Without focus or pad everything is released.
static void updateGamepad(bool active)
{
    static DWORD s_lastTime;
    DWORD now = timeGetTime();
    int milliseconds = s_lastTime ? (int)(now - s_lastTime) : 0;
    if (milliseconds > 100)
        milliseconds = 100;
    s_lastTime = now;

    bool keys[256] = {};
    bool buttons[3] = {};
    for (int i = 0; i < InputCount; i++)
        g_padHeld[i] = false;
    g_padMove = plasma::Vector<2, int>(0, 0);
    int wheel = 0;
    int pulseKey = 0; // a key pressed and released at once

    if (active && g_gamepad) {
        Uint32 down = 0;
        for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++)
            if (SDL_GetGamepadButton(g_gamepad, (SDL_GamepadButton)i))
                down |= 1u << i;
        if (SDL_GetGamepadAxis(g_gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > PadTriggerThreshold)
            down |= 1u << PadLeftTrigger;
        if (SDL_GetGamepadAxis(g_gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > PadTriggerThreshold)
            down |= 1u << PadRightTrigger;
        // A button held while the cursor appears or disappears does nothing until released: B closing
        // the inventory does not also jump.
        bool menu = g_game->isCursorVisible();
        static bool s_menu;
        static Uint32 s_ignored;
        if (menu != s_menu)
            s_ignored = down;
        s_menu = menu;
        s_ignored &= down;
        down &= ~s_ignored;
        static Uint32 s_down;
        Uint32 pressed = down & ~s_down;
        s_down = down;
        auto held = [down](int bit) { return (down & (1u << bit)) != 0; };

        float leftX, leftY, rightX, rightY;
        padStick(SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY, leftX, leftY);
        padStick(SDL_GAMEPAD_AXIS_RIGHTX, SDL_GAMEPAD_AXIS_RIGHTY, rightX, rightY);
        // The left trigger switches the face buttons to the skills and the d-pad to more windows.
        bool layer = held(PadLeftTrigger);

        keys[VK_ESCAPE] = held(SDL_GAMEPAD_BUTTON_START);
        keys['I'] = held(SDL_GAMEPAD_BUTTON_BACK);
        if (layer) {
            keys['G'] = held(SDL_GAMEPAD_BUTTON_DPAD_UP);
            keys['X'] = held(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
            keys['C'] = held(SDL_GAMEPAD_BUTTON_DPAD_LEFT);
            keys['V'] = held(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
        } else {
            keys['M'] = held(SDL_GAMEPAD_BUTTON_DPAD_LEFT);
            keys['F'] = held(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
            if (held(SDL_GAMEPAD_BUTTON_DPAD_UP))
                wheel = 1;
            else if (held(SDL_GAMEPAD_BUTTON_DPAD_DOWN))
                wheel = -1;
        }

        if (menu) {
            buttons[plasma::MOUSE_BUTTON_LEFT] = held(SDL_GAMEPAD_BUTTON_SOUTH);
            buttons[plasma::MOUSE_BUTTON_RIGHT] = held(SDL_GAMEPAD_BUTTON_WEST);
            if (rightY < -0.5f)
                wheel = 1;
            else if (rightY > 0.5f)
                wheel = -1;
            // B closes the topmost window the keyboard closes with a key.
            if (pressed & (1u << SDL_GAMEPAD_BUTTON_EAST)) {
                if (g_game->m_mapOpen)
                    pulseKey = 'M';
                else if (g_game->m_inventoryNode && g_game->m_inventoryNode->isVisible())
                    pulseKey = 'I';
                else if (g_game->m_craftingNode && g_game->m_craftingNode->isVisible())
                    pulseKey = 'C';
                else if (g_game->m_skillsOpen)
                    pulseKey = 'X';
                else if (g_game->m_systemPanel && g_game->m_systemPanel->isVisible())
                    pulseKey = 'O';
                else if (g_game->m_gameMenuOpen)
                    pulseKey = VK_ESCAPE;
            }
            if (leftX != 0.0f || leftY != 0.0f) {
                if (!g_padPointer) {
                    g_padPointerX = g_engine->getPointer()[0];
                    g_padPointerY = g_engine->getPointer()[1];
                    g_padPointer = true;
                }
                // Squared, so that small deflections move finely.
                float speed = sqrtf(leftX * leftX + leftY * leftY) * PadPointerSpeed * g_backBufferHeight *
                              milliseconds * 0.001f;
                g_padPointerX = SDL_clamp(g_padPointerX + leftX * speed, 0.0f, (float)(g_backBufferWidth - 1));
                g_padPointerY = SDL_clamp(g_padPointerY + leftY * speed, 0.0f, (float)(g_backBufferHeight - 1));
            }
        } else {
            buttons[plasma::MOUSE_BUTTON_LEFT] = held(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
            buttons[plasma::MOUSE_BUTTON_RIGHT] = held(PadRightTrigger);
            buttons[plasma::MOUSE_BUTTON_MIDDLE] = held(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
            if (layer) {
                g_padHeld[InputKey1] = held(SDL_GAMEPAD_BUTTON_WEST);
                g_padHeld[InputKey2] = held(SDL_GAMEPAD_BUTTON_NORTH);
                g_padHeld[InputKey3] = held(SDL_GAMEPAD_BUTTON_EAST);
                g_padHeld[InputKey4] = held(SDL_GAMEPAD_BUTTON_SOUTH);
            } else {
                g_padHeld[InputKeySpace] = held(SDL_GAMEPAD_BUTTON_EAST);
                g_padHeld[InputKeyR] = held(SDL_GAMEPAD_BUTTON_SOUTH);
                g_padHeld[InputKeyE] = held(SDL_GAMEPAD_BUTTON_SOUTH);
                g_padHeld[InputKeyQ] = held(SDL_GAMEPAD_BUTTON_WEST);
                g_padHeld[InputKeyControl] = held(SDL_GAMEPAD_BUTTON_NORTH);
                // B again in the air opens the glider (key G), as jumping in the air does in the release.
                if ((pressed & (1u << SDL_GAMEPAD_BUTTON_EAST)) && g_game->m_player) {
                    const Creature::EntityData &player = g_game->m_player->m_entityData;
                    if (!player.binaryToggles.gliding &&
                        !(player.flags & ((1 << CreatureFlagOnGround) | (1 << CreatureFlagInWater))))
                        pulseKey = 'G';
                }
            }
            g_padHeld[InputKeyT] = held(SDL_GAMEPAD_BUTTON_LEFT_STICK);
            keys[VK_TAB] = held(SDL_GAMEPAD_BUTTON_RIGHT_STICK);
            g_padMove = plasma::Vector<2, int>((int)(leftX * 32767.0f), (int)(-leftY * 32767.0f));
            // Squared like the cursor; down turns the camera down as the mouse does. A mouse unit turns
            // cameraSpeed * 0.005 degrees.
            float degreesPerUnit = (g_game->m_options.cameraSpeed > 0 ? g_game->m_options.cameraSpeed : 1) * 0.005f;
            float turn = sqrtf(rightX * rightX + rightY * rightY) * PadLookSpeed / degreesPerUnit * milliseconds *
                         0.001f;
            g_mouseDeltaX += rightX * turn;
            g_mouseDeltaY += rightY * turn;
        }
    }

    static bool s_keys[256];
    for (int key = 0; key < 256; key++) {
        if (keys[key] != s_keys[key]) {
            s_keys[key] = keys[key];
            padKey((unsigned char)key, keys[key]);
        }
    }
    if (pulseKey) {
        padKey((unsigned char)pulseKey, true);
        padKey((unsigned char)pulseKey, false);
    }
    for (int button = 0; button < 3; button++) {
        if (buttons[button] != g_padButtons[button]) {
            g_padButtons[button] = buttons[button];
            if (buttons[button])
                g_game->handleButtonDown(button);
            else
                g_game->handleButtonUp(button);
        }
    }
    // The wheel steps once on the press, then repeats while held.
    static int s_wheel;
    static int s_wheelDelay;
    if (wheel != s_wheel) {
        s_wheel = wheel;
        s_wheelDelay = 0;
        if (wheel) {
            g_game->handleMouseWheel(wheel);
            s_wheelDelay = PadFirstRepeat;
        }
    } else if (wheel) {
        s_wheelDelay -= milliseconds;
        if (s_wheelDelay <= 0) {
            g_game->handleMouseWheel(wheel);
            s_wheelDelay = PadRepeat;
        }
    }
}

// The window procedure's part: device resets on size and focus changes, typed characters, keys,
// buttons and the wheel. Returns false when the window is closed.
static bool handleEvent(const SDL_Event &event)
{
    switch (event.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        return false;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        resetDevice(false);
        break;
    case SDL_EVENT_TEXT_INPUT: {
        // One WM_CHAR per character: the engine gets the code point, the chat its low byte.
        const char *text = event.text.text;
        while (text && *text) {
            Uint32 character = SDL_StepUTF8(&text, nullptr);
            if (!character)
                break;
            g_engine->characterEntered((int)character);
            g_game->onKeyTyped((char)character);
        }
        break;
    }
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        unsigned char key = virtualKey(event.key);
        if (!key)
            break;
        if (event.type == SDL_EVENT_KEY_DOWN) {
            g_engine->keyPressed(key);
            g_game->handleKeyDown(key);
        } else {
            g_engine->keyReleased(key);
            g_game->handleKeyUp(key);
        }
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        int button;
        if (event.button.button == SDL_BUTTON_LEFT)
            button = plasma::MOUSE_BUTTON_LEFT;
        else if (event.button.button == SDL_BUTTON_RIGHT)
            button = plasma::MOUSE_BUTTON_RIGHT;
        else if (event.button.button == SDL_BUTTON_MIDDLE)
            button = plasma::MOUSE_BUTTON_MIDDLE;
        else
            break;
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            g_game->handleButtonDown(button);
        else
            g_game->handleButtonUp(button);
        break;
    }
    case SDL_EVENT_MOUSE_WHEEL: {
        // One step per notch (WM_MOUSEWHEEL's sign); smooth scrolling adds up to whole steps.
        float distance = event.wheel.y;
        if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
            distance = -distance;
        g_wheel += distance;
        while (g_wheel >= 1.0f) {
            g_game->handleMouseWheel(1);
            g_wheel -= 1.0f;
        }
        while (g_wheel <= -1.0f) {
            g_game->handleMouseWheel(-1);
            g_wheel += 1.0f;
        }
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
        g_mouseDeltaX += event.motion.xrel;
        g_mouseDeltaY += event.motion.yrel;
        g_padPointer = false;
        break;
    case SDL_EVENT_GAMEPAD_ADDED:
        if (!g_gamepad)
            g_gamepad = SDL_OpenGamepad(event.gdevice.which);
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if (g_gamepad && SDL_GetGamepadID(g_gamepad) == event.gdevice.which) {
            SDL_CloseGamepad(g_gamepad);
            g_gamepad = nullptr;
            // Another connected pad takes over.
            int count = 0;
            SDL_JoystickID *pads = SDL_GetGamepads(&count);
            if (pads && count > 0)
                g_gamepad = SDL_OpenGamepad(pads[0]);
            SDL_free(pads);
        }
        break;
    }
    return true;
}

// One frame: updates the game with the elapsed time, applies changed display options, renders and
// presents.
void frame()
{
    if (!g_device || !g_game)
        return;
    static DWORD s_lastTime; // Previous frame time.
    DWORD now = timeGetTime();
    if (!s_lastTime)
        s_lastTime = now;
    g_game->update(now - s_lastTime);
    if (g_game->m_options.resolutionX != options.resolutionX || g_game->m_options.resolutionY != options.resolutionY ||
        g_game->m_options.fullscreen != options.fullscreen || g_game->m_options.antiAliasing != options.antiAliasing) {
        options = g_game->m_options;
        resetDevice(false);
    }
    s_lastTime = now;
    g_game->render();
    if (g_device->Present(0, 0, 0, 0) == D3DERR_DEVICELOST)
        g_deviceLost = true;
}

// Creates the device for the window, the engine (with the system font directory) and the shader.
bool initializeDirect3D(int screenWidth, int screenHeight)
{
    g_direct3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_direct3d)
        return false;
    D3DPRESENT_PARAMETERS parameters;
    ZeroMemory(&parameters, sizeof(parameters));
    parameters.MultiSampleType = D3DMULTISAMPLE_NONE;
    parameters.MultiSampleQuality = 0;
    parameters.BackBufferWidth = screenWidth;
    parameters.BackBufferHeight = screenHeight;
    parameters.Windowed = TRUE;
    parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    parameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    parameters.EnableAutoDepthStencil = TRUE;
    parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
    parameters.hDeviceWindow = (HWND)g_window;
    if (g_direct3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, (HWND)g_window,
                                 D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &parameters,
                                 &g_device) != D3D_OK) {
        g_direct3d->Release();
        g_direct3d = 0;
        return false;
    }
    g_backBufferWidth = screenWidth;
    g_backBufferHeight = screenHeight;
    g_engine = new plasma::D3D9Engine(g_device);
    g_engine->getFontEngine()->addFontPath(L"/usr/share/fonts/TTF");
    if (!g_engine->setupShadersAndFrameBuffer()) {
        g_device->Release();
        g_device = 0;
        g_direct3d->Release();
        g_direct3d = 0;
        return false;
    }
    g_shader = new CubeShader(g_device);
    if (!g_shader->initialize()) {
        delete g_shader;
        g_device->Release();
        g_device = 0;
        g_direct3d->Release();
        g_direct3d = 0;
        return false;
    }
    int width, height;
    clientSize(width, height);
    g_engine->setViewport(0, 0, width, height, true);
    return true;
}

// Resets the device to the window's client size (or the options' resolution in fullscreen) and the
// options' antialiasing, under the game's update locks.
void resetDevice(bool unused)
{
    bool fullscreen = options.fullscreen != 0;
    int width, height;
    if (fullscreen) {
        width = options.resolutionX;
        height = options.resolutionY;
    } else {
        clientSize(width, height);
    }
    if (width <= 0 || height <= 0)
        return;
    if (g_device) {
        if (g_engine) {
            g_engine->releaseDeviceResources();
            g_engine->setViewport(0, 0, width, height, true);
        }
        D3DPRESENT_PARAMETERS parameters;
        ZeroMemory(&parameters, sizeof(parameters));
        // 2^step samples, halved until the adapter supports them for the back and depth buffers
        // (an unsupported count would make the reset below fail forever).
        int samples = options.antiAliasing > 0 ? 1 << options.antiAliasing : 0;
        while (samples >= 2 &&
               (g_direct3d->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, !fullscreen,
                                                       (D3DMULTISAMPLE_TYPE)samples, 0) != D3D_OK ||
                g_direct3d->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_D24S8, !fullscreen,
                                                       (D3DMULTISAMPLE_TYPE)samples, 0) != D3D_OK))
            samples /= 2;
        parameters.MultiSampleType = samples >= 2 ? (D3DMULTISAMPLE_TYPE)samples : D3DMULTISAMPLE_NONE;
        parameters.MultiSampleQuality = 0;
        parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
        parameters.BackBufferWidth = width;
        parameters.BackBufferHeight = height;
        parameters.Windowed = !fullscreen;
        parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
        parameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        parameters.EnableAutoDepthStencil = TRUE;
        parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
        parameters.hDeviceWindow = (HWND)g_window;
        if (g_game)
            g_game->beginUpdate();
        while (g_device->Reset(&parameters) != D3D_OK)
            Sleep(DeviceResetRetryDelay);
        g_backBufferWidth = width;
        g_backBufferHeight = height;
        if (g_game) {
            g_game->endUpdate();
            g_game->setWindowSize(width, height);
        }
        if (g_engine)
            g_engine->restoreDeviceResources();
    }
    g_deviceLost = false;
}

// Opens the window and the devices, creates the game and runs the event loop until the window closes.
// Command line: "server" hosts, any other first argument is a server address to join.
int main(int argc, char **argv)
{
    // glibc hands the top of the heap back to the system after every free of more than 128 KiB and fetches it
    // again at once; the path search and DXVK allocate and free many blocks every frame. Up to 256 MiB stay
    // with the game and the heap grows by 64 MiB at a time. GLIBC_TUNABLES, when set (even empty), decides
    // instead.
    if (!getenv("GLIBC_TUNABLES")) {
        mallopt(M_TRIM_THRESHOLD, 256 << 20);
        mallopt(M_TOP_PAD, 64 << 20);
    }

    // The game's files (data*.db, *.plx, Save/) are next to the executable.
    const char *base = SDL_GetBasePath();
    if (base && chdir(base) != 0)
        SDL_Log("chdir %s failed", base);

    // DXVK-native draws into the SDL3 window (its window-system driver must be named).
    setenv("DXVK_WSI_DRIVER", "SDL3", 0);

    WSADATA winsock;
    WSAStartup(MAKEWORD(2, 2), &winsock);

    SDL_SetHint(SDL_HINT_APP_ID, "infinite-cubes-alpha-linux");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD)) {
        showError(SDL_GetError());
        return 0;
    }
    // The screen's size in pixels (GetSystemMetrics), for the first back buffer and the default
    // fullscreen resolution.
    int screenWidth = 800, screenHeight = 600;
    const SDL_DisplayMode *desktop = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    if (desktop) {
        screenWidth = (int)lroundf(desktop->w * desktop->pixel_density);
        screenHeight = (int)lroundf(desktop->h * desktop->pixel_density);
    }
    // The window starts at 800 x 600 and is then maximized.
    g_window = SDL_CreateWindow("Cube World", 800, 600,
                                SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                                    SDL_WINDOW_HIDDEN);
    if (!g_window) {
        showError(SDL_GetError());
        return 0;
    }
    if (!initializeDirect3D(screenWidth, screenHeight)) {
        showError("Could not initialize Direct3D 9 (DXVK-native). Please make sure Vulkan works "
                  "(vulkaninfo) and lib/libdxvk_d3d9.so is next to the game.");
        return 0;
    }

    // The distinct display resolutions, for the options screen.
    UINT modeCount = g_direct3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
    std::vector<plasma::Vector<2, int>> resolutions;
    for (UINT i = 0; i < modeCount; i++) {
        D3DDISPLAYMODE mode;
        g_direct3d->EnumAdapterModes(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &mode);
        bool known = false;
        for (std::vector<plasma::Vector<2, int>>::iterator it = resolutions.begin(); it != resolutions.end(); ++it) {
            if (*it == plasma::Vector<2, int>(mode.Width, mode.Height)) {
                known = true;
                break;
            }
        }
        if (!known)
            resolutions.push_back(plasma::Vector<2, int>(mode.Width, mode.Height));
    }
    options.resolutionX = screenWidth;
    options.resolutionY = screenHeight;
    options.load();

    // Deleted before SDL_Quit: FAudio plays through SDL.
    XAudio2Engine *audio = new XAudio2Engine;
    if (!audio->initialize()) {
        showError("Could not initialize the sound (FAudio).");
        return 0;
    }
    audio->setMusicVolume(options.musicVolume * 0.01f, 0);

    bool server = false;
    std::string serverAddress;
    if (argc >= 2) {
        if (std::string(argv[1]) == "server")
            server = true;
        else
            serverAddress = argv[1];
    }
    _mkdir("Save");

    g_game = new GameController(g_device, g_shader, g_engine, audio, &options, &resolutions, server, serverAddress);
    SDL_ShowWindow(g_window);
    SDL_MaximizeWindow(g_window);
    SDL_HideCursor();
    SDL_RaiseWindow(g_window);
    SDL_StartTextInput(g_window);
    resetDevice(false);

    bool relativeMouse = false;
    bool running = true;
    while (running) {
        DWORD frameStart = timeGetTime();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (!handleEvent(event)) {
                running = false;
                break;
            }
        }
        if (!running || g_game->m_quit)
            break;
        bool focused = hasFocus();
        if (g_readInput) {
            // A free cursor appears where the engine's pointer is; otherwise the mouse is captured
            // and reports relative motion (WinMain kept the cursor in the window's middle).
            bool visible = g_game->isCursorVisible();
            bool wantRelative = !visible && focused;
            if (wantRelative != relativeMouse) {
                SDL_SetWindowRelativeMouseMode(g_window, wantRelative);
                relativeMouse = wantRelative;
                g_mouseDeltaX = 0.0f;
                g_mouseDeltaY = 0.0f;
            }
            if (visible) {
                if (!g_cursorShown && focused) {
                    int width = 0, height = 0;
                    SDL_GetWindowSize(g_window, &width, &height);
                    float x = g_engine->getPointer()[0], y = g_engine->getPointer()[1];
                    if (g_backBufferWidth > 0 && g_backBufferHeight > 0) {
                        x *= (float)width / g_backBufferWidth;
                        y *= (float)height / g_backBufferHeight;
                    }
                    SDL_WarpMouseInWindow(g_window, x, y);
                }
                g_cursorShown = true;
            } else if (focused) {
                g_cursorShown = false;
                g_padPointer = false;
            }
        }
        updateGamepad(g_readInput && focused);
        if (g_readInput && focused) {
            const bool *keys = SDL_GetKeyboardState(nullptr);
            g_game->m_inputs[InputKeyW] = keys[SDL_SCANCODE_W];
            g_game->m_inputs[InputKeyS] = keys[SDL_SCANCODE_S];
            g_game->m_inputs[InputKeyA] = keys[SDL_SCANCODE_A];
            g_game->m_inputs[InputKeyD] = keys[SDL_SCANCODE_D];
            g_game->m_inputs[InputKeyE] = keys[SDL_SCANCODE_E];
            g_game->m_inputs[InputKeyR] = keys[SDL_SCANCODE_R];
            g_game->m_inputs[InputKeyT] = keys[SDL_SCANCODE_T];
            g_game->m_inputs[InputKeySpace] = keys[SDL_SCANCODE_SPACE];
            g_game->m_inputs[InputKeyShift] = keys[SDL_SCANCODE_LSHIFT];
            g_game->m_inputs[InputKeyControl] = keys[SDL_SCANCODE_LCTRL];
            g_game->m_inputs[InputKeyQ] = keys[SDL_SCANCODE_Q];
            float x, y;
            SDL_MouseButtonFlags buttons = SDL_GetMouseState(&x, &y);
            if (g_game->isCursorVisible()) {
                if (g_padPointer) {
                    x = g_padPointerX;
                    y = g_padPointerY;
                } else {
                    cursorPosition(x, y);
                }
                g_game->handleMouseMove(x, y);
            } else {
                g_game->handleMouseMove(g_mouseDeltaX, g_mouseDeltaY);
            }
            g_game->m_inputs[InputMouseLeft] = (buttons & SDL_BUTTON_LMASK) != 0;
            g_game->m_inputs[InputKeyShift2] = keys[SDL_SCANCODE_LSHIFT];
            g_game->m_inputs[InputMouseMiddle] = (buttons & SDL_BUTTON_MMASK) != 0;
            g_game->m_inputs[InputMouseRight] = (buttons & SDL_BUTTON_RMASK) != 0;
            g_game->m_inputs[InputKey1] = keys[SDL_SCANCODE_1];
            g_game->m_inputs[InputKey2] = keys[SDL_SCANCODE_2];
            g_game->m_inputs[InputKey3] = keys[SDL_SCANCODE_3];
            g_game->m_inputs[InputKey4] = keys[SDL_SCANCODE_4];
            g_game->m_moveStick = g_padMove;
            g_game->m_lookStick = plasma::Vector<2, int>(0, 0);
            // The pad's keys and buttons count as well.
            for (int i = 0; i < InputCount; i++)
                g_game->m_inputs[i] = g_game->m_inputs[i] || g_padHeld[i];
            g_game->m_inputs[InputMouseLeft] = g_game->m_inputs[InputMouseLeft] || g_padButtons[plasma::MOUSE_BUTTON_LEFT];
            g_game->m_inputs[InputMouseRight] =
                g_game->m_inputs[InputMouseRight] || g_padButtons[plasma::MOUSE_BUTTON_RIGHT];
            g_game->m_inputs[InputMouseMiddle] =
                g_game->m_inputs[InputMouseMiddle] || g_padButtons[plasma::MOUSE_BUTTON_MIDDLE];
        }
        g_mouseDeltaX = 0.0f;
        g_mouseDeltaY = 0.0f;
        frame();
        DWORD elapsed = timeGetTime() - frameStart;
        if ((int)elapsed < g_game->m_options.minTimeStep)
            Sleep(g_game->m_options.minTimeStep - elapsed);
    }

    SDL_StopTextInput(g_window);
    SDL_SetWindowRelativeMouseMode(g_window, false);
    delete g_game;
    delete audio;
    delete g_engine;
    delete g_shader;
    if (g_device)
        g_device->Release();
    if (g_direct3d)
        g_direct3d->Release();
    if (g_gamepad)
        SDL_CloseGamepad(g_gamepad);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    WSACleanup();
    return 0;
}
