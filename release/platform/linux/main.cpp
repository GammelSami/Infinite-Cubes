// The Linux entry point, in place of src/main.cpp: an SDL3 window, Direct3D 11 through DXVK-native and SDL's keyboard,
// mouse and gamepad translated to the DirectInput and XInput states the game reads. The start-up, the per-frame display
// option handling and the frame loop follow src/main.cpp; full screen is a borderless window over the desktop.
#include <SDL3/SDL.h>
#include <windows.h>
#include <mmsystem.h>
#include <d3d11.h>
#include <dinput.h>
#include <xinput.h>
#include <direct.h>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include "cube/World.h"
#include "cube/XAudio2Engine.h"
#include "cube/Game.h"
#include "cube/Options.h"
#include "plasma/D3D11Engine.h"
#include "plasma/FontEngine.h"
#include "steam/isteamnetworkingutils.h"
#include "gfx/D3D11Graphics.h"
#include "gfx/D3D11Renderer.h"
#include "cube/SpriteManager.h"
#include "cube/WorldMap.h"
#include "FrameInput.h"
#include "TestScript.h"

cube::Game *game;
// The device's quality levels for the options' sample count, minus one.
int multisampleQuality;
// Set while the pointer is the menus' own: entering the menus moves the system cursor to the interface's pointer.
bool cursorInMenu;
ID3D11Device *device;
ID3D11DeviceContext *deviceContext;
IDXGISwapChain *swapChain;
ID3D11RenderTargetView *renderTargetView;
ID3D11DepthStencilView *depthStencilView;
ID3D11Texture2D *depthStencilBuffer;
plasma::D3D11Engine *engine;
SDL_Window *sdlWindow;
SDL_Gamepad *gamepad;
// The mouse's movement since the last frame (DirectInput's relative axes).
static float s_mouseX;
static float s_mouseY;
// The keyboard and mouse as the events left them (kept here so that the test script's events count too): the keys
// held, the mouse buttons held (SDL_BUTTON_MASK) and the cursor in window coordinates.
static bool s_keyHeld[SDL_SCANCODE_COUNT];
static SDL_MouseButtonFlags s_buttonsHeld;
static float s_cursorX;
static float s_cursorY;
// Set when the next presented frame is saved (the test script's "screenshot").
static std::string s_screenshotRequest;
// Current options; draw adopts Game's copy when it changes.
static cube::Options s_options;
// "log_startup.txt": every step of the start-up is written to it.
static std::ofstream s_startupLog("log_startup.txt");

// DirectInput scan codes by SDL scan code (USB usage); zero where DirectInput has none.
static unsigned char s_dikByScancode[SDL_SCANCODE_COUNT];

static void initializeScancodes()
{
    static const struct {
        SDL_Scancode scancode;
        unsigned char dik;
    } table[] = {
        {SDL_SCANCODE_ESCAPE, DIK_ESCAPE},
        {SDL_SCANCODE_1, DIK_1},
        {SDL_SCANCODE_2, DIK_2},
        {SDL_SCANCODE_3, DIK_3},
        {SDL_SCANCODE_4, DIK_4},
        {SDL_SCANCODE_5, DIK_5},
        {SDL_SCANCODE_6, DIK_6},
        {SDL_SCANCODE_7, DIK_7},
        {SDL_SCANCODE_8, DIK_8},
        {SDL_SCANCODE_9, DIK_9},
        {SDL_SCANCODE_0, DIK_0},
        {SDL_SCANCODE_MINUS, DIK_MINUS},
        {SDL_SCANCODE_EQUALS, DIK_EQUALS},
        {SDL_SCANCODE_BACKSPACE, DIK_BACK},
        {SDL_SCANCODE_TAB, DIK_TAB},
        {SDL_SCANCODE_Q, DIK_Q},
        {SDL_SCANCODE_W, DIK_W},
        {SDL_SCANCODE_E, DIK_E},
        {SDL_SCANCODE_R, DIK_R},
        {SDL_SCANCODE_T, DIK_T},
        {SDL_SCANCODE_Y, DIK_Y},
        {SDL_SCANCODE_U, DIK_U},
        {SDL_SCANCODE_I, DIK_I},
        {SDL_SCANCODE_O, DIK_O},
        {SDL_SCANCODE_P, DIK_P},
        {SDL_SCANCODE_LEFTBRACKET, DIK_LBRACKET},
        {SDL_SCANCODE_RIGHTBRACKET, DIK_RBRACKET},
        {SDL_SCANCODE_RETURN, DIK_RETURN},
        {SDL_SCANCODE_LCTRL, DIK_LCONTROL},
        {SDL_SCANCODE_A, DIK_A},
        {SDL_SCANCODE_S, DIK_S},
        {SDL_SCANCODE_D, DIK_D},
        {SDL_SCANCODE_F, DIK_F},
        {SDL_SCANCODE_G, DIK_G},
        {SDL_SCANCODE_H, DIK_H},
        {SDL_SCANCODE_J, DIK_J},
        {SDL_SCANCODE_K, DIK_K},
        {SDL_SCANCODE_L, DIK_L},
        {SDL_SCANCODE_SEMICOLON, DIK_SEMICOLON},
        {SDL_SCANCODE_APOSTROPHE, DIK_APOSTROPHE},
        {SDL_SCANCODE_GRAVE, DIK_GRAVE},
        {SDL_SCANCODE_LSHIFT, DIK_LSHIFT},
        {SDL_SCANCODE_BACKSLASH, DIK_BACKSLASH},
        {SDL_SCANCODE_Z, DIK_Z},
        {SDL_SCANCODE_X, DIK_X},
        {SDL_SCANCODE_C, DIK_C},
        {SDL_SCANCODE_V, DIK_V},
        {SDL_SCANCODE_B, DIK_B},
        {SDL_SCANCODE_N, DIK_N},
        {SDL_SCANCODE_M, DIK_M},
        {SDL_SCANCODE_COMMA, DIK_COMMA},
        {SDL_SCANCODE_PERIOD, DIK_PERIOD},
        {SDL_SCANCODE_SLASH, DIK_SLASH},
        {SDL_SCANCODE_RSHIFT, DIK_RSHIFT},
        {SDL_SCANCODE_KP_MULTIPLY, DIK_MULTIPLY},
        {SDL_SCANCODE_LALT, DIK_LMENU},
        {SDL_SCANCODE_SPACE, DIK_SPACE},
        {SDL_SCANCODE_CAPSLOCK, DIK_CAPITAL},
        {SDL_SCANCODE_F1, DIK_F1},
        {SDL_SCANCODE_F2, DIK_F2},
        {SDL_SCANCODE_F3, DIK_F3},
        {SDL_SCANCODE_F4, DIK_F4},
        {SDL_SCANCODE_F5, DIK_F5},
        {SDL_SCANCODE_F6, DIK_F6},
        {SDL_SCANCODE_F7, DIK_F7},
        {SDL_SCANCODE_F8, DIK_F8},
        {SDL_SCANCODE_F9, DIK_F9},
        {SDL_SCANCODE_F10, DIK_F10},
        {SDL_SCANCODE_NUMLOCKCLEAR, DIK_NUMLOCK},
        {SDL_SCANCODE_SCROLLLOCK, DIK_SCROLL},
        {SDL_SCANCODE_KP_7, DIK_NUMPAD7},
        {SDL_SCANCODE_KP_8, DIK_NUMPAD8},
        {SDL_SCANCODE_KP_9, DIK_NUMPAD9},
        {SDL_SCANCODE_KP_MINUS, DIK_SUBTRACT},
        {SDL_SCANCODE_KP_4, DIK_NUMPAD4},
        {SDL_SCANCODE_KP_5, DIK_NUMPAD5},
        {SDL_SCANCODE_KP_6, DIK_NUMPAD6},
        {SDL_SCANCODE_KP_PLUS, DIK_ADD},
        {SDL_SCANCODE_KP_1, DIK_NUMPAD1},
        {SDL_SCANCODE_KP_2, DIK_NUMPAD2},
        {SDL_SCANCODE_KP_3, DIK_NUMPAD3},
        {SDL_SCANCODE_KP_0, DIK_NUMPAD0},
        {SDL_SCANCODE_KP_PERIOD, DIK_DECIMAL},
        {SDL_SCANCODE_NONUSBACKSLASH, DIK_OEM_102},
        {SDL_SCANCODE_F11, DIK_F11},
        {SDL_SCANCODE_F12, DIK_F12},
        {SDL_SCANCODE_KP_EQUALS, DIK_NUMPADEQUALS},
        {SDL_SCANCODE_KP_ENTER, DIK_NUMPADENTER},
        {SDL_SCANCODE_RCTRL, DIK_RCONTROL},
        {SDL_SCANCODE_MUTE, DIK_MUTE},
        {SDL_SCANCODE_VOLUMEDOWN, DIK_VOLUMEDOWN},
        {SDL_SCANCODE_VOLUMEUP, DIK_VOLUMEUP},
        {SDL_SCANCODE_KP_DIVIDE, DIK_DIVIDE},
        {SDL_SCANCODE_PRINTSCREEN, DIK_SYSRQ},
        {SDL_SCANCODE_RALT, DIK_RMENU},
        {SDL_SCANCODE_PAUSE, DIK_PAUSE},
        {SDL_SCANCODE_HOME, DIK_HOME},
        {SDL_SCANCODE_UP, DIK_UP},
        {SDL_SCANCODE_PAGEUP, DIK_PRIOR},
        {SDL_SCANCODE_LEFT, DIK_LEFT},
        {SDL_SCANCODE_RIGHT, DIK_RIGHT},
        {SDL_SCANCODE_END, DIK_END},
        {SDL_SCANCODE_DOWN, DIK_DOWN},
        {SDL_SCANCODE_PAGEDOWN, DIK_NEXT},
        {SDL_SCANCODE_INSERT, DIK_INSERT},
        {SDL_SCANCODE_DELETE, DIK_DELETE},
        {SDL_SCANCODE_LGUI, DIK_LWIN},
        {SDL_SCANCODE_RGUI, DIK_RWIN},
        {SDL_SCANCODE_APPLICATION, DIK_APPS},
    };
    for (const auto &entry : table) {
        s_dikByScancode[entry.scancode] = entry.dik;
    }
}

// The Windows virtual-key code of a key, as WM_KEYDOWN's wParam: letters and digits follow the layout (SDL's key
// code), the others the key's position.
static int virtualKey(const SDL_KeyboardEvent &key)
{
    if (key.key >= 'a' && key.key <= 'z') {
        return (int)(key.key - 'a' + 'A');
    }
    if (key.key >= '0' && key.key <= '9') {
        return (int)key.key;
    }
    switch (key.scancode) {
    case SDL_SCANCODE_BACKSPACE: return VK_BACK;
    case SDL_SCANCODE_TAB: return VK_TAB;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER: return VK_RETURN;
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT: return VK_SHIFT;
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL: return VK_CONTROL;
    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT: return VK_MENU;
    case SDL_SCANCODE_PAUSE: return VK_PAUSE;
    case SDL_SCANCODE_CAPSLOCK: return VK_CAPITAL;
    case SDL_SCANCODE_ESCAPE: return VK_ESCAPE;
    case SDL_SCANCODE_SPACE: return VK_SPACE;
    case SDL_SCANCODE_PAGEUP: return VK_PRIOR;
    case SDL_SCANCODE_PAGEDOWN: return VK_NEXT;
    case SDL_SCANCODE_END: return VK_END;
    case SDL_SCANCODE_HOME: return VK_HOME;
    case SDL_SCANCODE_LEFT: return VK_LEFT;
    case SDL_SCANCODE_UP: return VK_UP;
    case SDL_SCANCODE_RIGHT: return VK_RIGHT;
    case SDL_SCANCODE_DOWN: return VK_DOWN;
    case SDL_SCANCODE_INSERT: return VK_INSERT;
    case SDL_SCANCODE_DELETE: return VK_DELETE;
    default: break;
    }
    if (key.scancode >= SDL_SCANCODE_F1 && key.scancode <= SDL_SCANCODE_F12) {
        return VK_F1 + (key.scancode - SDL_SCANCODE_F1);
    }
    if (key.scancode >= SDL_SCANCODE_KP_1 && key.scancode <= SDL_SCANCODE_KP_9) {
        return VK_NUMPAD0 + 1 + (key.scancode - SDL_SCANCODE_KP_1);
    }
    if (key.scancode == SDL_SCANCODE_KP_0) {
        return VK_NUMPAD0;
    }
    return 0;
}

// The pixels of the window per SDL window coordinate, for the mouse's movement.
static float pixelDensity()
{
    float density = SDL_GetWindowPixelDensity(sdlWindow);
    return density > 0.0f ? density : 1.0f;
}

// The back buffer's size: the resolution in full screen, as in src/main.cpp, otherwise the window's size in pixels.
static void backBufferSize(int &width, int &height)
{
    if (s_options.fullscreen) {
        width = s_options.resolutionX;
        height = s_options.resolutionY;
    } else {
        SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
    }
}

// The back buffer's pixels per SDL window coordinate, for the cursor's position.
static void pointerScale(float &x, float &y)
{
    int bufferWidth = 0;
    int bufferHeight = 0;
    backBufferSize(bufferWidth, bufferHeight);
    int windowWidth = 0;
    int windowHeight = 0;
    SDL_GetWindowSize(sdlWindow, &windowWidth, &windowHeight);
    x = windowWidth > 0 && bufferWidth > 0 ? (float)bufferWidth / windowWidth : 1.0f;
    y = windowHeight > 0 && bufferHeight > 0 ? (float)bufferHeight / windowHeight : 1.0f;
}

void setPointerInClient(int x, int y)
{
    float scaleX, scaleY;
    pointerScale(scaleX, scaleY);
    SDL_WarpMouseInWindow(sdlWindow, x / scaleX, y / scaleY);
}

DWORD XInputGetState(DWORD userIndex, XINPUT_STATE *state)
{
    if (userIndex != 0 || !gamepad) {
        return ERROR_DEVICE_NOT_CONNECTED;
    }
    static const struct {
        SDL_GamepadButton button;
        WORD mask;
    } buttons[] = {
        {SDL_GAMEPAD_BUTTON_DPAD_UP, XINPUT_GAMEPAD_DPAD_UP},
        {SDL_GAMEPAD_BUTTON_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_DOWN},
        {SDL_GAMEPAD_BUTTON_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_LEFT},
        {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, XINPUT_GAMEPAD_DPAD_RIGHT},
        {SDL_GAMEPAD_BUTTON_START, XINPUT_GAMEPAD_START},
        {SDL_GAMEPAD_BUTTON_BACK, XINPUT_GAMEPAD_BACK},
        {SDL_GAMEPAD_BUTTON_LEFT_STICK, XINPUT_GAMEPAD_LEFT_THUMB},
        {SDL_GAMEPAD_BUTTON_RIGHT_STICK, XINPUT_GAMEPAD_RIGHT_THUMB},
        {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, XINPUT_GAMEPAD_LEFT_SHOULDER},
        {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER},
        {SDL_GAMEPAD_BUTTON_SOUTH, XINPUT_GAMEPAD_A},
        {SDL_GAMEPAD_BUTTON_EAST, XINPUT_GAMEPAD_B},
        {SDL_GAMEPAD_BUTTON_WEST, XINPUT_GAMEPAD_X},
        {SDL_GAMEPAD_BUTTON_NORTH, XINPUT_GAMEPAD_Y},
    };
    ZeroMemory(state, sizeof(*state));
    for (const auto &entry : buttons) {
        if (SDL_GetGamepadButton(gamepad, entry.button)) {
            state->Gamepad.wButtons |= entry.mask;
        }
    }
    // SDL's vertical axes grow downwards, XInput's upwards.
    auto up = [](Sint16 value) { return (SHORT)(value == -32768 ? 32767 : -value); };
    state->Gamepad.sThumbLX = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
    state->Gamepad.sThumbLY = up(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY));
    state->Gamepad.sThumbRX = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
    state->Gamepad.sThumbRY = up(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY));
    state->Gamepad.bLeftTrigger = (BYTE)(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) * 255 / 32767);
    state->Gamepad.bRightTrigger = (BYTE)(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) * 255 / 32767);
    return ERROR_SUCCESS;
}

// Rebuilds the back buffer's views and the depth buffer for backBufferSize and tells the engine and the game. Its
// argument is not used (callers pass 0).
void resize(int unused)
{
    if (!deviceContext) {
        return;
    }
    int width = 0;
    int height = 0;
    backBufferSize(width, height);
    if (width <= 0 || height <= 0) {
        return;
    }
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    renderTargetView->Release();
    swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    ID3D11Texture2D *backBuffer;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backBuffer);
    device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
    backBuffer->Release();
    D3D11_TEXTURE2D_DESC depthDesc;
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = s_options.antiAliasingSamples;
    depthDesc.SampleDesc.Quality = multisampleQuality;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;
    if (depthStencilBuffer) {
        depthStencilBuffer->Release();
    }
    if (depthStencilView) {
        depthStencilView->Release();
    }
    device->CreateTexture2D(&depthDesc, nullptr, &depthStencilBuffer);
    D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDesc;
    ZeroMemory(&depthViewDesc, sizeof(depthViewDesc));
    depthViewDesc.Format = depthDesc.Format;
    depthViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    device->CreateDepthStencilView(depthStencilBuffer, &depthViewDesc, &depthStencilView);
    deviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
    D3D11_VIEWPORT viewport;
    viewport.Width = (float)width;
    viewport.Height = (float)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    deviceContext->RSSetViewports(1, &viewport);
    if (engine) {
        engine->setViewport(0, 0, width, height, true);
    }
    if (game) {
        game->resize(width, height);
    }
}

// Saves the back buffer as a BMP file (for testing: CUBE_SCREENSHOT=<file> takes one CUBE_SCREENSHOT_FRAME frames in,
// default 300). Multisampled back buffers are resolved first.
static void saveScreenshot(const char *fileName)
{
    ID3D11Texture2D *backBuffer = nullptr;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backBuffer);
    D3D11_TEXTURE2D_DESC desc;
    backBuffer->GetDesc(&desc);
    ID3D11Texture2D *source = backBuffer;
    ID3D11Texture2D *resolved = nullptr;
    if (desc.SampleDesc.Count > 1) {
        D3D11_TEXTURE2D_DESC resolvedDesc = desc;
        resolvedDesc.SampleDesc.Count = 1;
        resolvedDesc.SampleDesc.Quality = 0;
        resolvedDesc.BindFlags = 0;
        device->CreateTexture2D(&resolvedDesc, nullptr, &resolved);
        deviceContext->ResolveSubresource(resolved, 0, backBuffer, 0, desc.Format);
        source = resolved;
    }
    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.SampleDesc.Quality = 0;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;
    ID3D11Texture2D *staging = nullptr;
    device->CreateTexture2D(&stagingDesc, nullptr, &staging);
    deviceContext->CopyResource(staging, source);
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(deviceContext->Map(staging, 0, D3D11_MAP_READ, 0, &mapped))) {
        int width = (int)desc.Width;
        int height = (int)desc.Height;
        int rowSize = width * 3;
        int padding = (4 - rowSize % 4) % 4;
        std::ofstream file(fileName, std::ios::binary);
        auto put32 = [&](uint32_t value) { file.write((const char *)&value, 4); };
        auto put16 = [&](uint16_t value) { file.write((const char *)&value, 2); };
        file.write("BM", 2);
        put32(54 + (rowSize + padding) * height);
        put32(0);
        put32(54);
        put32(40);
        put32(width);
        put32(height);
        put16(1);
        put16(24);
        put32(0);
        put32((rowSize + padding) * height);
        put32(2835);
        put32(2835);
        put32(0);
        put32(0);
        std::vector<char> row(rowSize + padding);
        for (int y = height - 1; y >= 0; y--) {
            const unsigned char *pixels = (const unsigned char *)mapped.pData + y * mapped.RowPitch;
            for (int x = 0; x < width; x++) {
                row[x * 3 + 0] = pixels[x * 4 + 2];
                row[x * 3 + 1] = pixels[x * 4 + 1];
                row[x * 3 + 2] = pixels[x * 4 + 0];
            }
            file.write(row.data(), row.size());
        }
        deviceContext->Unmap(staging, 0);
        std::cout << "screenshot " << fileName << std::endl;
    }
    staging->Release();
    if (resolved) {
        resolved->Release();
    }
    backBuffer->Release();
}

// Puts the window into the state s_options asks for: windowed at the resolution, or borderless full screen, where the
// back buffer keeps the resolution and is stretched to the screen (Wayland compositors do not switch display modes).
void applyDisplayOptions()
{
    if (s_options.fullscreen) {
        SDL_SetWindowFullscreenMode(sdlWindow, nullptr);
        SDL_SetWindowFullscreen(sdlWindow, true);
    } else {
        SDL_SetWindowFullscreen(sdlWindow, false);
        // Sized only once out of full screen: before, the window manager restores the old size afterwards.
        SDL_SyncWindow(sdlWindow);
        // A window as large as the screen is maximized instead: mutter left one of exactly the screen's size undrawn
        // after full screen, and Present waited for it forever.
        SDL_Rect usable;
        if (SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(sdlWindow), &usable) &&
            (s_options.resolutionX >= usable.w || s_options.resolutionY >= usable.h)) {
            SDL_MaximizeWindow(sdlWindow);
        } else {
            SDL_RestoreWindow(sdlWindow);
            SDL_SetWindowSize(sdlWindow, s_options.resolutionX, s_options.resolutionY);
            SDL_SetWindowPosition(sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
    }
    SDL_SyncWindow(sdlWindow);
}

// One frame: clears the buffers, runs the game for the time since the previous frame, follows the game's display
// options, waits out the minimum time step and presents.
void draw()
{
    if (!device || !game) {
        return;
    }
    DWORD start = timeGetTime();
    deviceContext->ClearRenderTargetView(renderTargetView, &plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)[0]);
    deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    static std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = now - last;
    last = now;
    game->update(elapsed.count() * 1000.0);
    // Unlike src/main.cpp, s_options.fullscreen stays the state last requested: not every compositor reports full
    // screen in the window flags, and reading them would rebuild the buffers every frame.
    if (game->m_options.resolutionX != s_options.resolutionX || game->m_options.resolutionY != s_options.resolutionY ||
        game->m_options.refreshRate != s_options.refreshRate || game->m_options.fullscreen != s_options.fullscreen ||
        game->m_options.antiAliasing != s_options.antiAliasing || game->m_options.vsync != s_options.vsync) {
        s_options = game->m_options;
        applyDisplayOptions();
        resize(0);
    }
    DWORD frameTime = timeGetTime() - start;
    if (frameTime < (DWORD)game->m_options.minTimeStep) {
        Sleep(game->m_options.minTimeStep - frameTime);
    }
    static const char *screenshot = getenv("CUBE_SCREENSHOT");
    static int screenshotFrame = getenv("CUBE_SCREENSHOT_FRAME") ? atoi(getenv("CUBE_SCREENSHOT_FRAME")) : 300;
    if (screenshot && --screenshotFrame == 0) {
        saveScreenshot(screenshot);
    }
    if (!s_screenshotRequest.empty()) {
        saveScreenshot(s_screenshotRequest.c_str());
        s_screenshotRequest.clear();
    }
    swapChain->Present(s_options.vsync != 0, 0);
}

// Creates the device, the swap chain for the window, the render target and depth buffer and the engine.
bool initDirect3D()
{
    DXGI_MODE_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));
    int width = 0;
    int height = 0;
    backBufferSize(width, height);
    bufferDesc.Width = width;
    bufferDesc.Height = height;
    bufferDesc.RefreshRate.Numerator = s_options.refreshRate;
    bufferDesc.RefreshRate.Denominator = 1;
    bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    s_startupLog << "D3D11CreateDevice" << std::endl;
    HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                                       &device, nullptr, &deviceContext);
    if (result != S_OK) {
        s_startupLog << "failed: " << result << std::endl;
        return false;
    }
    s_startupLog << "bufferDesc " << bufferDesc.Width << " " << bufferDesc.Height << std::endl;
    s_startupLog << "CheckMultisampleQualityLevels" << std::endl;
    UINT qualityLevels;
    result = device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, s_options.antiAliasingSamples,
                                                   &qualityLevels);
    if (result != S_OK) {
        s_startupLog << "failed: " << result << std::endl;
        return false;
    }
    s_startupLog << "CreateSwapChain" << std::endl;
    multisampleQuality = qualityLevels - 1;
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferDesc = bufferDesc;
    swapChainDesc.SampleDesc.Count = s_options.antiAliasingSamples;
    swapChainDesc.SampleDesc.Quality = multisampleQuality;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = (HWND)sdlWindow;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    IDXGIDevice *dxgiDevice = nullptr;
    device->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDevice);
    IDXGIAdapter *adapter = nullptr;
    dxgiDevice->GetAdapter(&adapter);
    IDXGIFactory *factory = nullptr;
    adapter->GetParent(__uuidof(IDXGIFactory), (void **)&factory);
    result = factory->CreateSwapChain(device, &swapChainDesc, &swapChain);
    if (result != S_OK) {
        s_startupLog << "failed: " << result << std::endl;
        return false;
    }
    std::cout << "QUALITY LEVELS " << qualityLevels << std::endl;
    ID3D11Texture2D *backBuffer;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backBuffer);
    D3D11_RENDER_TARGET_VIEW_DESC renderTargetDesc;
    ZeroMemory(&renderTargetDesc, sizeof(renderTargetDesc));
    renderTargetDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    renderTargetDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
    device->CreateRenderTargetView(backBuffer, &renderTargetDesc, &renderTargetView);
    backBuffer->Release();
    s_startupLog << "CreateDepthStencil" << std::endl;
    D3D11_TEXTURE2D_DESC depthDesc;
    depthDesc.Width = bufferDesc.Width;
    depthDesc.Height = bufferDesc.Height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc = swapChainDesc.SampleDesc;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;
    result = device->CreateTexture2D(&depthDesc, nullptr, &depthStencilBuffer);
    if (result != S_OK) {
        s_startupLog << "failed: " << result << std::endl;
        return false;
    }
    D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDesc;
    ZeroMemory(&depthViewDesc, sizeof(depthViewDesc));
    depthViewDesc.Format = depthDesc.Format;
    depthViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    result = device->CreateDepthStencilView(depthStencilBuffer, &depthViewDesc, &depthStencilView);
    if (result != S_OK) {
        s_startupLog << "failed: " << result << std::endl;
        return false;
    }
    deviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
    s_startupLog << "create D3D11Engine" << std::endl;
    engine = new plasma::D3D11Engine(device, deviceContext);
    engine->getFontEngine()->addFontPath(L"/usr/share/fonts/TTF");
    if (!engine->setupShadersAndFrameBuffer()) {
        s_startupLog << "failed: " << result << std::endl;
        device->Release();
        device = nullptr;
        deviceContext->Release();
        deviceContext = nullptr;
        return false;
    }
    s_startupLog << "ok" << std::endl;
    return true;
}

// Handles one SDL event as the Windows window procedure handles the corresponding message. False ends the program.
static bool handleEvent(const SDL_Event &event)
{
    switch (event.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        return false;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        resize(0);
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        if (event.key.scancode < SDL_SCANCODE_COUNT) {
            s_keyHeld[event.key.scancode] = event.type == SDL_EVENT_KEY_DOWN;
        }
        int key = virtualKey(event.key);
        if (key == 0) {
            break;
        }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            engine->keyPressed((unsigned short)key);
            game->handleKeyDown((unsigned char)key);
            // TranslateMessage's characters for the editing keys; text arrives as SDL_EVENT_TEXT_INPUT.
            if (key == VK_BACK || key == VK_TAB || key == VK_RETURN || key == VK_ESCAPE) {
                engine->characterEntered((wchar_t)key);
                game->onKeyTyped((unsigned char)key);
            }
        } else {
            engine->keyReleased((unsigned short)key);
            game->handleKeyUp((unsigned char)key);
        }
        break;
    }
    case SDL_EVENT_TEXT_INPUT: {
        const unsigned char *text = (const unsigned char *)event.text.text;
        while (*text) {
            Uint32 character = SDL_StepUTF8((const char **)&text, nullptr);
            if (character == 0) {
                break;
            }
            engine->characterEntered((wchar_t)character);
            game->onKeyTyped((unsigned char)character);
        }
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
        s_mouseX += event.motion.xrel * pixelDensity();
        s_mouseY += event.motion.yrel * pixelDensity();
        s_cursorX = event.motion.x;
        s_cursorY = event.motion.y;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            s_buttonsHeld |= SDL_BUTTON_MASK(event.button.button);
        } else {
            s_buttonsHeld &= ~SDL_BUTTON_MASK(event.button.button);
        }
        s_cursorX = event.button.x;
        s_cursorY = event.button.y;
        int button = event.button.button == SDL_BUTTON_LEFT    ? 0
                     : event.button.button == SDL_BUTTON_RIGHT ? 1
                     : event.button.button == SDL_BUTTON_MIDDLE ? 2
                                                                 : -1;
        if (button < 0) {
            break;
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            game->handleButtonDown(button);
        } else {
            game->handleButtonUp(button);
        }
        break;
    }
    case SDL_EVENT_MOUSE_WHEEL:
        if (event.wheel.y < 0) {
            game->handleMouseWheel(-1);
        } else if (event.wheel.y > 0) {
            game->handleMouseWheel(1);
        }
        break;
    case SDL_EVENT_GAMEPAD_ADDED:
        if (!gamepad) {
            gamepad = SDL_OpenGamepad(event.gdevice.which);
        }
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if (gamepad && SDL_GetGamepadID(gamepad) == event.gdevice.which) {
            SDL_CloseGamepad(gamepad);
            gamepad = nullptr;
        }
        break;
    default:
        break;
    }
    return true;
}

// The program: starts Steam (the offline stand-in), creates the window, collects the display modes, loads the
// options, creates Direct3D, the audio engine and the game with its parts, and then runs the frame loop until the
// window is closed: events first, then the cursor, the keyboard and mouse bindings and the gamepad, then one frame.
int main(int argc, char **argv)
{
    s_startupLog << "Start" << std::endl;
    tzset();
    // DXVK-native presents to SDL3 windows.
    setenv("DXVK_WSI_DRIVER", "SDL3", 0);
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        std::cout << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    initializeScancodes();
    s_startupLog << "SteamAPI_Init" << std::endl;
    if (!SteamAPI_Init()) {
        std::cout << "STEAM API INIT FAILED" << std::endl;
        return 0;
    }
    std::cout << "STEAM API INITIALIZED SUCCESSFULLY" << std::endl;
    SteamNetworkingUtils()->InitRelayNetworkAccess();

    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *desktop = SDL_GetDesktopDisplayMode(display);
    s_options.resolutionX = desktop ? desktop->w : 1920;
    s_options.resolutionY = desktop ? desktop->h : 1080;
    s_startupLog << "enum display settings" << std::endl;
    // The display's modes: the refresh rates by size.
    std::map<std::array<int, 2>, std::set<int>> modes;
    int modeCount = 0;
    SDL_DisplayMode **displayModes = SDL_GetFullscreenDisplayModes(display, &modeCount);
    for (int index = 0; displayModes && index < modeCount; index++) {
        std::array<int, 2> size = {displayModes[index]->w, displayModes[index]->h};
        modes[size].insert((int)std::lround(displayModes[index]->refresh_rate));
    }
    SDL_free(displayModes);
    if (desktop) {
        modes[std::array<int, 2> {desktop->w, desktop->h}].insert((int)std::lround(desktop->refresh_rate));
    }
    s_startupLog << "load options" << std::endl;
    s_options.load();
    s_options.save();
    s_startupLog << "select resolution" << std::endl;
    std::map<std::array<int, 2>, std::set<int>>::iterator selected =
        modes.find(std::array<int, 2> {s_options.resolutionX, s_options.resolutionY});
    if (selected == modes.end() || selected->second.find(s_options.refreshRate) == selected->second.end()) {
        s_options.resolutionX = desktop ? desktop->w : 1920;
        s_options.resolutionY = desktop ? desktop->h : 1080;
        s_options.refreshRate = desktop ? (int)std::lround(desktop->refresh_rate) : 60;
    }
    s_startupLog << "create main window" << std::endl;
    sdlWindow = SDL_CreateWindow("Cube World", s_options.resolutionX, s_options.resolutionY,
                                 SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!sdlWindow) {
        std::cout << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    // Before the swap chain, which takes the window's size.
    applyDisplayOptions();
    s_startupLog << "initialize graphics" << std::endl;
    if (!initDirect3D()) {
        s_startupLog << "failed" << std::endl;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Cube World", "Could not initialize Direct3D.", sdlWindow);
        return 1;
    }
    s_startupLog << "initialize audio" << std::endl;
    // On the heap, to be destroyed before SDL_Quit: FAudio plays through SDL.
    cube::XAudio2Engine *audio = new cube::XAudio2Engine;
    if (!audio->initialize()) {
        s_startupLog << "failed" << std::endl;
    }
    s_startupLog << "done" << std::endl;
    audio->setMusicVolume(s_options.musicVolume * 0.01f, 0.0f);
    s_startupLog << "create save dir" << std::endl;
    _mkdir("Save");
    s_startupLog << "create graphics" << std::endl;
    gfx::D3D11Graphics *graphics = new gfx::D3D11Graphics(device, deviceContext);
    s_startupLog << "create sprites" << std::endl;
    cube::SpriteManager *spriteManager = new cube::SpriteManager(graphics);
    s_startupLog << "create world" << std::endl;
    cube::World *world = new cube::World(spriteManager);
    s_startupLog << "create world map" << std::endl;
    cube::WorldMap *worldMap = new cube::WorldMap(world, engine);
    s_startupLog << "create renderer" << std::endl;
    gfx::D3D11Renderer *renderer = new gfx::D3D11Renderer(world, worldMap, engine, device, deviceContext);
    s_startupLog << "create game" << std::endl;
    std::vector<cube::ResolutionInfo> resolutions;
    for (std::map<std::array<int, 2>, std::set<int>>::iterator it = modes.begin(); it != modes.end(); ++it) {
        resolutions.push_back(*it);
    }
    game = new cube::Game(world, worldMap, renderer, engine, audio, &s_options, &resolutions);
    s_startupLog << "OK" << std::endl;
    SDL_HideCursor();
    SDL_StartTextInput(sdlWindow);
    SDL_RaiseWindow(sdlWindow);
    resize(0);
    unsigned char previousKeys[256];
    ZeroMemory(previousKeys, sizeof(previousKeys));
    DIMOUSESTATE previousMouseState;
    ZeroMemory(&previousMouseState, sizeof(DIMOUSESTATE));
    s_startupLog << "START" << std::endl;
    TestScript *testScript = getenv("CUBE_TEST_SCRIPT") ? new TestScript(getenv("CUBE_TEST_SCRIPT")) : nullptr;
    bool running = true;
    while (running && !game->m_shutdown) {
        if (testScript && !testScript->step(sdlWindow, s_screenshotRequest)) {
            running = false;
        }
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (!handleEvent(event)) {
                running = false;
            }
        }
        if (!running) {
            break;
        }
        // A test script's input counts whether or not the window has the focus.
        if ((SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_INPUT_FOCUS) || testScript) {
            // In the menus the system cursor is the pointer; in the game the mouse moves relatively.
            if (game->m_interface.isInMenu()) {
                if (!cursorInMenu) {
                    SDL_SetWindowRelativeMouseMode(sdlWindow, false);
                    setPointerInClient((int)engine->getPointer()[0], (int)engine->getPointer()[1]);
                }
                cursorInMenu = true;
            } else if (cursorInMenu) {
                cursorInMenu = false;
                SDL_SetWindowRelativeMouseMode(sdlWindow, true);
            }
            unsigned char keys[256];
            ZeroMemory(keys, sizeof(keys));
            for (int scancode = 0; scancode < SDL_SCANCODE_COUNT; scancode++) {
                if (s_keyHeld[scancode] && s_dikByScancode[scancode]) {
                    keys[s_dikByScancode[scancode]] = 0x80;
                }
            }
            float cursorX = s_cursorX;
            float cursorY = s_cursorY;
            SDL_MouseButtonFlags mouseButtons = s_buttonsHeld;
            DIMOUSESTATE mouseState;
            ZeroMemory(&mouseState, sizeof(mouseState));
            mouseState.lX = (LONG)s_mouseX;
            mouseState.lY = (LONG)s_mouseY;
            s_mouseX -= mouseState.lX;
            s_mouseY -= mouseState.lY;
            mouseState.rgbButtons[0] = (mouseButtons & SDL_BUTTON_LMASK) ? 0x80 : 0;
            mouseState.rgbButtons[1] = (mouseButtons & SDL_BUTTON_RMASK) ? 0x80 : 0;
            mouseState.rgbButtons[2] = (mouseButtons & SDL_BUTTON_MMASK) ? 0x80 : 0;
            mouseState.rgbButtons[3] = (mouseButtons & SDL_BUTTON_X1MASK) ? 0x80 : 0;
            if (game->isAssigningControl()) {
                assignControlFromDevices(game, keys, mouseState, previousKeys, previousMouseState);
            } else {
                applyDeviceBindings(game, keys, mouseState);
                // In the menus the game gets the cursor's position, in the game the mouse's movement.
                if (game->m_interface.isInMenu()) {
                    float scaleX, scaleY;
                    pointerScale(scaleX, scaleY);
                    game->handleMouseMove(cursorX * scaleX, cursorY * scaleY);
                } else {
                    game->handleMouseMove((float)mouseState.lX, (float)mouseState.lY);
                }
                applyGamepad(game, engine);
            }
            memcpy(previousKeys, keys, sizeof(keys));
            previousMouseState = mouseState;
        } else {
            s_mouseX = 0.0f;
            s_mouseY = 0.0f;
            Sleep(16);
        }
        draw();
    }
    SDL_SetWindowRelativeMouseMode(sdlWindow, false);
    delete game;
    // Not deleted: its D3D11Shader releases m_vertexShader twice (as the original does), which DXVK does not survive.
    // The process ends right after.
    (void)renderer;
    std::cout << "delete world map" << std::endl;
    delete worldMap;
    std::cout << "delete world" << std::endl;
    delete world;
    std::cout << "delete gui" << std::endl;
    delete engine;
    std::cout << "delete sprites" << std::endl;
    delete spriteManager;
    std::cout << "delete graphics" << std::endl;
    delete graphics;
    swapChain->Release();
    device->Release();
    deviceContext->Release();
    delete audio;
    SteamAPI_Shutdown();
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
    return 0;
}
