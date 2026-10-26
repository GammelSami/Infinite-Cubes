#include <winsock2.h>
#include <windows.h>
#include <mmsystem.h>
#include <windows.h>
#include <shellapi.h>
#include <d3d11.h>
#include <dinput.h>
#include <xinput.h>
#include <direct.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include "cube/World.h"
#include "plasma/EditFilter.h"
#include "cube/XAudio2Engine.h"
#include "cube/Game.h"
#include "cube/Options.h"
#include "plasma/D3D11Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
#include "steam/isteamnetworkingutils.h"
#include "gfx/D3D11Graphics.h"
#include "gfx/D3D11Renderer.h"
#include "cube/SpriteManager.h"
#include "cube/WorldMap.h"
#include "cube/widgets/InventoryWidget.h"
#include "cube/widgets/FormulaDetailsWidget.h"
#include "cube/widgets/MultiplayerWidget.h"
#include "cube/widgets/VoxelWidget.h"
#include "cube/widgets/EnchantWidget.h"
#include "cube/widgets/SystemWidget.h"
#include "cube/ServerUpdateIds.h"
#include "cube/widgets/OptionsWidget.h"
#include "cube/widgets/ControlsWidget.h"

cube::Game *game;
// The device's quality levels for the options' sample count, minus one.
int multisampleQuality;
// Set while the pointer is the menus' own: the first frame in a menu moves the system cursor to the interface's
// pointer, and leaving the menus clears it when the cursor is next recentred. Only WinMain uses it.
bool cursorInMenu;
// Set while the splash window exists: the WM_DESTROY that follows is the splash window's and does not end the program.
bool splashWindowOpen;
ID3D11Device *device;
ID3D11DeviceContext *deviceContext;
IDXGISwapChain *swapChain;
ID3D11RenderTargetView *renderTargetView;
ID3D11DepthStencilView *depthStencilView;
ID3D11Texture2D *depthStencilBuffer;
plasma::D3D11Engine *engine;
HWND window;
HINSTANCE instance;
// Hybrid-graphics driver exports for NVIDIA Optimus and AMD PowerXpress, requesting the discrete GPU with the vendors'
// documented names and values.
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 1;
// Gates cursor and input-device reads. Always remains set because focus messages are not handled.
bool focus = true;
// Current options; draw adopts Game's copy when it changes.
static cube::Options s_options;
// "log_startup.txt": every step of the start-up is written to it.
static std::ofstream s_startupLog("log_startup.txt");

void resize(int unused);

// Handles messages for the main and splash windows. Input is forwarded to the engine and game; destroying the splash
// window does not terminate the program.
BOOL CALLBACK MessageHandler(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_DESTROY:
        if (splashWindowOpen) {
            splashWindowOpen = false;
        } else {
            PostQuitMessage(0);
        }
        break;
    case WM_SIZE:
        resize(0);
        break;
    case WM_SETFOCUS:
        break;
    case WM_KILLFOCUS:
        break;
    case WM_CLOSE:
        DestroyWindow(handle);
        break;
    case WM_KEYDOWN:
        engine->keyPressed((unsigned short)wParam);
        game->handleKeyDown((unsigned char)wParam);
        return 0;
    case WM_KEYUP:
        engine->keyReleased((unsigned short)wParam);
        game->handleKeyUp((unsigned char)wParam);
        return 0;
    case WM_CHAR:
        engine->characterEntered((wchar_t)wParam);
        game->onKeyTyped((unsigned char)wParam);
        return 0;
    case WM_MOUSEMOVE:
        break;
    case WM_LBUTTONDOWN:
        SetCapture(handle);
        game->handleButtonDown(0);
        break;
    case WM_LBUTTONUP:
        ReleaseCapture();
        game->handleButtonUp(0);
        break;
    case WM_RBUTTONDOWN:
        SetCapture(handle);
        game->handleButtonDown(1);
        break;
    case WM_RBUTTONUP:
        ReleaseCapture();
        game->handleButtonUp(1);
        break;
    case WM_MBUTTONDOWN:
        SetCapture(handle);
        game->handleButtonDown(2);
        break;
    case WM_MBUTTONUP:
        ReleaseCapture();
        game->handleButtonUp(2);
        break;
    case WM_MOUSEWHEEL: {
        short delta = HIWORD(wParam);
        if (delta < 0) {
            delta = -1;
        } else if (delta > 0) {
            delta = 1;
        }
        game->handleMouseWheel(delta);
        break;
    }
    default:
        return DefWindowProcW(handle, message, wParam, lParam);
    }
    return 1;
}

// Shows the bitmap in the layered splash window, centred on the primary monitor's work area. WinMain logs "set splash
// image" before the call.
void SetSplashImage(HWND splashWindow, HBITMAP splashBitmap)
{
    BITMAP bitmap;
    GetObjectW(splashBitmap, sizeof(bitmap), &bitmap);
    SIZE size = {bitmap.bmWidth, bitmap.bmHeight};
    POINT zero = {0};
    HMONITOR monitor = MonitorFromPoint(zero, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info = {0};
    info.cbSize = sizeof(info);
    GetMonitorInfoW(monitor, &info);
    const RECT &work = info.rcWork;
    POINT origin;
    origin.x = work.left + (work.right - work.left - size.cx) / 2;
    origin.y = work.top + (work.bottom - work.top - size.cy) / 2;
    HDC screen = GetDC(nullptr);
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP previous = (HBITMAP)SelectObject(memory, splashBitmap);
    UpdateLayeredWindow(splashWindow, screen, &origin, &size, memory, &zero, RGB(0, 0, 0), nullptr, 0);
    SelectObject(memory, previous);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);
}

// One frame: clears the buffers, runs the game for the time since the previous frame, follows the game's display
// options, waits out the minimum time step and presents.
void draw()
{
    if (!device || !game) {
        return;
    }
    timeBeginPeriod(1);
    DWORD start = timeGetTime();
    timeEndPeriod(1);
    deviceContext->ClearRenderTargetView(renderTargetView, &plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)[0]);
    deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    static std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = now - last;
    last = now;
    game->update(elapsed.count() * 1000.0);
    if (GetForegroundWindow() == window) {
        BOOL fullscreen = FALSE;
        if (device && swapChain) {
            swapChain->GetFullscreenState(&fullscreen, nullptr);
        }
        s_options.fullscreen = fullscreen != 0;
    }
    if (device && swapChain) {
        if (game->m_options.resolutionX != s_options.resolutionX ||
            game->m_options.resolutionY != s_options.resolutionY ||
            game->m_options.refreshRate != s_options.refreshRate ||
            game->m_options.fullscreen != s_options.fullscreen ||
            game->m_options.antiAliasing != s_options.antiAliasing || game->m_options.vsync != s_options.vsync) {
            s_options = game->m_options;
            swapChain->SetFullscreenState(s_options.fullscreen != 0, nullptr);
            resize(0);
            DXGI_MODE_DESC mode;
            ZeroMemory(&mode, sizeof(mode));
            mode.Width = s_options.resolutionX;
            mode.Height = s_options.resolutionY;
            mode.RefreshRate.Numerator = s_options.refreshRate;
            mode.RefreshRate.Denominator = 1;
            mode.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            mode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            mode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            swapChain->ResizeTarget(&mode);
        }
    }
    timeBeginPeriod(1);
    DWORD frameTime = timeGetTime() - start;
    if (frameTime < (DWORD)game->m_options.minTimeStep) {
        Sleep(game->m_options.minTimeStep - frameTime);
    }
    timeEndPeriod(1);
    swapChain->Present(s_options.vsync != 0, 0);
}

// Creates the device, the swap chain for the window, the render target and depth buffer and the engine.
bool initDirect3D()
{
    DXGI_MODE_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(DXGI_MODE_DESC));
    bufferDesc.Width = s_options.resolutionX;
    bufferDesc.Height = s_options.resolutionY;
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
    swapChainDesc.OutputWindow = window;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
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
    engine->getFontEngine()->addFontPath(L"c:\\windows\\fonts");
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

// Rebuilds the back buffer's views and the depth buffer for the window's client size (the options' resolution in
// full-screen mode) and tells the engine and the game. Its argument is not used (callers pass 0).
void resize(int unused)
{
    if (!deviceContext) {
        return;
    }
    RECT client;
    GetClientRect(window, &client);
    int width = client.right;
    int height = client.bottom;
    BOOL fullscreen = FALSE;
    if (device && swapChain) {
        swapChain->GetFullscreenState(&fullscreen, nullptr);
        if (fullscreen) {
            width = s_options.resolutionX;
            height = s_options.resolutionY;
        }
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

// The thumb sticks count as moved in the menus beyond twice XInput's dead zones.
static const int kLeftStickThreshold = 2 * XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
static const int kRightStickThreshold = 2 * XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;

// Whether the pointer is over an open inventory panel.
#define POINTED_AT(widget) ((widget)->getNode()->isVisible() && (widget)->getNode()->isUnderPointer())

// The program: shows the splash window, starts Steam, creates the main window, collects the display modes, loads the
// options, creates Direct3D, the audio engine, DirectInput's keyboard and mouse and the game with its parts, and then
// runs the frame loop until the window is closed: window messages first, otherwise the cursor, the keyboard and mouse
// bindings and the gamepad, then one frame (draw). Every step of the start-up is written to log_startup.txt.
int WINAPI WinMain(HINSTANCE programInstance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand)
{
    s_startupLog << "Start" << std::endl;
    instance = programInstance;
    WNDCLASSEXW splashClass;
    ZeroMemory(&splashClass, sizeof(WNDCLASSEXW));
    splashClass.cbSize = sizeof(WNDCLASSEXW);
    splashClass.style = 0;
    splashClass.lpfnWndProc = DefWindowProcW;
    splashClass.cbClsExtra = 0;
    splashClass.cbWndExtra = 0;
    splashClass.hInstance = programInstance;
    splashClass.hIcon = nullptr;
    splashClass.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    splashClass.hbrBackground = nullptr;
    splashClass.lpszMenuName = nullptr;
    splashClass.lpszClassName = L"Cube World Splash";
    splashClass.hIconSm = splashClass.hIcon;
    s_startupLog << "load logo" << std::endl;
    HBITMAP logo = (HBITMAP)LoadImageW(instance, L"logo.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    RECT desktop;
    GetWindowRect(GetDesktopWindow(), &desktop);
    s_startupLog << "create splash window" << std::endl;
    RegisterClassExW(&splashClass);
    HWND splashWindow = CreateWindowExW(WS_EX_LAYERED, L"Cube World Splash", nullptr, WS_POPUP, 0, 0, 0, 0, nullptr,
                                        nullptr, instance, nullptr);
    s_startupLog << "set splash image" << std::endl;
    SetSplashImage(splashWindow, logo);
    ShowWindow(splashWindow, showCommand);
    s_startupLog << "SteamAPI_Init" << std::endl;
    if (SteamAPI_Init()) {
        std::cout << "STEAM API INITIALIZED SUCCESSFULLY" << std::endl;
        std::cout << "InitRelayNetworkAccess..." << std::endl;
        s_startupLog << "InitRelayNetworkAccess" << std::endl;
        SteamNetworkingUtils()->InitRelayNetworkAccess();
        s_startupLog << "create main window" << std::endl;
        WSADATA socketData;
        WSAStartup(MAKEWORD(2, 2), &socketData);
        WNDCLASSW windowClass;
        windowClass.style = 0;
        windowClass.lpfnWndProc = (WNDPROC)MessageHandler;
        windowClass.cbClsExtra = 0;
        windowClass.cbWndExtra = 0;
        windowClass.hInstance = instance;
        windowClass.hIcon = LoadIconW(instance, L"Cube World");
        windowClass.hCursor = nullptr;
        windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        windowClass.lpszMenuName = L"Cube World";
        windowClass.lpszClassName = L"Cube World";
        if (!RegisterClassW(&windowClass)) {
            return 0;
        }
        window =
            CreateWindowExW(0, L"Cube World", L"Cube World", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0,
                            0, 1920, 1080, nullptr, nullptr, instance, nullptr);
        if (!window) {
            return 0;
        }
        s_options.resolutionX = GetSystemMetrics(SM_CXSCREEN);
        s_options.resolutionY = GetSystemMetrics(SM_CYSCREEN);
        s_startupLog << "enum display settings" << std::endl;
        // The 32-bit modes the display accepts: the refresh rates by size.
        std::map<std::array<int, 2>, std::set<int>> modes;
        DEVMODEW mode;
        ZeroMemory(&mode, sizeof(DEVMODEW));
        mode.dmSize = sizeof(DEVMODEW);
        for (int index = 0; EnumDisplaySettingsW(nullptr, index, &mode); index++) {
            if (ChangeDisplaySettingsW(&mode, CDS_TEST) == DISP_CHANGE_SUCCESSFUL && mode.dmBitsPerPel == 32) {
                int refreshRate = mode.dmDisplayFrequency;
                std::array<int, 2> size = {(int)mode.dmPelsWidth, (int)mode.dmPelsHeight};
                modes[size].insert(refreshRate);
            }
        }
        s_startupLog << "load options" << std::endl;
        s_options.load();
        s_options.save();
        s_startupLog << "select resolution" << std::endl;
        std::map<std::array<int, 2>, std::set<int>>::iterator selected =
            modes.find(std::array<int, 2> {s_options.resolutionX, s_options.resolutionY});
        if (selected == modes.end() || selected->second.find(s_options.refreshRate) == selected->second.end()) {
            DEVMODEW current;
            ZeroMemory(&current, sizeof(DEVMODEW));
            current.dmSize = sizeof(DEVMODEW);
            EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &current);
            s_options.resolutionX = current.dmPelsWidth;
            s_options.resolutionY = current.dmPelsHeight;
            s_options.refreshRate = current.dmDisplayFrequency;
        }
        s_startupLog << "initialize graphics" << std::endl;
        if (!initDirect3D()) {
            s_startupLog << "failed" << std::endl;
            MessageBoxA(window, "Could not initialize Direct3D.", "Cube World", MB_OK);
        } else {
            s_startupLog << "initialize audio" << std::endl;
            cube::XAudio2Engine audio;
            if (!audio.initialize()) {
                s_startupLog << "failed" << std::endl;
            }
            s_startupLog << "done" << std::endl;
            audio.setMusicVolume(s_options.musicVolume * 0.01f, 0.0f);
            s_startupLog << "DirectInput8Create" << std::endl;
            IDirectInput8W *directInput;
            if (DirectInput8Create(programInstance, DIRECTINPUT_VERSION, IID_IDirectInput8W, (void **)&directInput,
                                   nullptr) != DI_OK) {
                s_startupLog << "failed" << std::endl;
                MessageBoxA(window, "Could not initialize DirectInput8.", "Cube World", MB_OK);
            } else {
                s_startupLog << "create directinput devices" << std::endl;
                IDirectInputDevice8W *keyboard;
                directInput->CreateDevice(GUID_SysKeyboard, &keyboard, nullptr);
                keyboard->SetDataFormat(&c_dfDIKeyboard);
                keyboard->Acquire();
                IDirectInputDevice8W *mouse;
                directInput->CreateDevice(GUID_SysMouse, &mouse, nullptr);
                mouse->SetDataFormat(&c_dfDIMouse);
                mouse->SetCooperativeLevel(window, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
                mouse->Acquire();
                // The first argument, unless it is "server". Nothing reads it.
                int argumentCount = 0;
                std::string address;
                LPWSTR *arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
                if (arguments) {
                    if (argumentCount >= 2) {
                        if (!(std::wstring(arguments[1]) == L"server")) {
                            std::wstring argument(arguments[1]);
                            address = NarrowString(argument);
                        }
                    }
                    LocalFree(arguments);
                }
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
                for (std::map<std::array<int, 2>, std::set<int>>::iterator it = modes.begin(); it != modes.end();
                     ++it) {
                    resolutions.push_back(*it);
                }
                game = new cube::Game(world, worldMap, renderer, engine, &audio, &s_options, &resolutions);
                s_startupLog << "OK" << std::endl;
                DestroyWindow(splashWindow);
                ShowWindow(window, SW_SHOWNORMAL);
                SetCursor(nullptr);
                ShowCursor(FALSE);
                UpdateWindow(window);
                SetFocus(window);
                SetForegroundWindow(window);
                unsigned char previousKeys[256];
                ZeroMemory(previousKeys, sizeof(previousKeys));
                DIMOUSESTATE previousMouseState;
                ZeroMemory(&previousMouseState, sizeof(DIMOUSESTATE));
                s_startupLog << "START" << std::endl;
                MSG message;
                for (;;) {
                    if (game->m_shutdown) {
                        PostQuitMessage(0);
                    }
                    if (PeekMessageW(&message, nullptr, 0, 0, PM_NOREMOVE) == TRUE) {
                        if (!GetMessageW(&message, nullptr, 0, 0)) {
                            break;
                        }
                        TranslateMessage(&message);
                        DispatchMessageW(&message);
                    } else {
                        if (focus) {
                            // In the menus the system cursor is the pointer; in the game it is kept near the centre of
                            // the window.
                            RECT client;
                            GetClientRect(window, &client);
                            POINT cursor;
                            GetCursorPos(&cursor);
                            ScreenToClient(window, &cursor);
                            if (game->m_interface.isInMenu()) {
                                if (!cursorInMenu) {
                                    POINT pointer;
                                    pointer.x = (int)engine->getPointer()[0];
                                    pointer.y = (int)engine->getPointer()[1];
                                    ClientToScreen(window, &pointer);
                                    // Compares a screen position with a client position.
                                    if (pointer.x != cursor.x || pointer.y != cursor.y) {
                                        SetCursorPos(pointer.x, pointer.y);
                                    }
                                }
                                cursorInMenu = true;
                            } else if ((abs(cursor.x - client.right / 2) > 500 ||
                                        abs(cursor.y - client.bottom / 2) > 500) &&
                                       GetForegroundWindow() == window) {
                                cursorInMenu = false;
                                cursor.x = client.right / 2;
                                cursor.y = client.bottom / 2;
                                ClientToScreen(window, &cursor);
                                SetCursorPos(cursor.x, cursor.y);
                            }
                        }
                        if (focus && GetForegroundWindow() == window) {
                            unsigned char keys[256];
                            keyboard->GetDeviceState(sizeof(keys), keys);
                            DIMOUSESTATE mouseState;
                            mouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState);
                            if (game->isAssigningControl()) {
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
                            } else {
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
                                // In the menus the game gets the cursor's position, in the game the mouse's movement.
                                if (game->m_interface.isInMenu()) {
                                    POINT position;
                                    GetCursorPos(&position);
                                    if (!s_options.fullscreen) {
                                        ScreenToClient(window, &position);
                                    }
                                    game->handleMouseMove((float)position.x, (float)position.y);
                                } else {
                                    game->handleMouseMove((float)mouseState.lX, (float)mouseState.lY);
                                }
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
                                            POINT pointer;
                                            pointer.x = (int)x;
                                            pointer.y = (int)y;
                                            ClientToScreen(window, &pointer);
                                            SetCursorPos(pointer.x, pointer.y);
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
                            memcpy(previousKeys, keys, sizeof(keys));
                            previousMouseState = mouseState;
                        } else {
                            Sleep(16);
                        }
                        draw();
                    }
                }
                CloseWindow(window);
                delete game;
                std::cout << "delete renderer" << std::endl;
                delete renderer;
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
                keyboard->Release();
                mouse->Release();
                directInput->Release();
                swapChain->Release();
                device->Release();
                deviceContext->Release();
                WSACleanup();
                SteamAPI_Shutdown();
            }
        }
    } else {
        std::cout << "STEAM API INIT FAILED" << std::endl;
    }
    return 0;
}
