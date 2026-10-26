// The application window, menus, and Windows helpers for file and message boxes, the clipboard and paths.
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <icm.h>
#undef GetColorDirectory
#include <mmsystem.h>
#include <direct.h>
#include <fstream>
#include "plasma/FilePath.h"
#include <GL/glew.h>
#include <algorithm>
#include <ctype.h>
#include <list>
#include <map>
#include <string>
#ifdef PLASMA_NO_LICENSING
#include <vector>
#endif

#include "main.h"
#include "Module.h"
#include "Workspace.h"
#include "plasma/Engine.h"
#include "Tablet.h"
#include "DesignSheet.h"
#include "ExportModule.h"
#include "OptionsModule.h"
#include "ToolModule.h"
#include "ClipboardModule.h"
#include "AboutModule.h"
#include "PaletteModule.h"
#include "plasma/FontEngine.h"
#include "plasma/Node.h"
#include "plasma/TextShape.h"

// The executable's icon resource (the window class icon).
static const WORD APPLICATION_ICON = 125;

// Every minute the window's timer asks Workspace::Relogin to renew the login. Network requests are disabled in the
// offline build.
static const UINT_PTR RELOGIN_TIMER = 1;
static const UINT RELOGIN_INTERVAL = 60000;

#include "MainMenuIds.inc"

HWND g_window;
HINSTANCE g_instance;
HGLRC g_glContext;
HMENU g_fileMenu;
HDC g_deviceContext;
HMENU g_sheetMenu;
HMENU g_editMenu;
HMENU g_interfaceZoomMenu;
HMENU g_settingsMenu;
bool g_initialized; // InitPlasma succeeded.

// Toggled by F1 (Workspace::KeyPressed); the scene then takes no pointer presses (Workspace).
bool g_screenshotMode;
Workspace g_workspace;
Tablet g_tablet;

static DWORD s_lastFontCleanup;
// Unused string.
static std::string s_unusedString;
static std::wstring s_appDataPath; // <application data>\Picroma\Plasma.
static std::wstring s_commonAppDataPath;
static std::string s_colorDirectory;
std::list<std::wstring> g_recentFiles;

// The file menu lists at most this many recent files (Cache\recent.log).
static const size_t MAX_RECENT_FILES = 5;
std::map<int, HCURSOR> g_cursors;

#include "MainSheetOrder.inc"

enum {
    DialogStaticClassAtom = 0x0082
};
enum {
    DialogEditClassAtom = 0x0081
};
enum {
    DialogButtonClassAtom = 0x0080
};
static const int RecentFileLineCapacity = 512;
static const int FontCacheCleanupIntervalMilliseconds = 10000;

std::wstring getAppDataPath()
{
    return s_appDataPath;
}

std::wstring getCommonAppDataPath()
{
    return s_commonAppDataPath;
}

std::string getColorDirectory()
{
    return s_colorDirectory;
}

// The chosen file, or "" if the user cancelled.
std::wstring openFileBox(const wchar_t *title, const wchar_t *filter)
{
    WCHAR file[MAX_PATH] = {0};
    OPENFILENAMEW dialog;
    memset(&dialog, 0, sizeof(dialog));
    dialog.hwndOwner = g_window;
    dialog.hInstance = g_instance;
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFile = file;
    dialog.lpstrTitle = title;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrFilter = filter;
    dialog.nFilterIndex = 1;
    dialog.lpstrFileTitle = NULL;
    dialog.nMaxFileTitle = 0;
    dialog.lpstrInitialDir = NULL;
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameW(&dialog))
        return file;
    return L"";
}

// The chosen file, or "" if the user cancelled; file is the suggested name.
std::wstring saveFileBox(const wchar_t *title, const wchar_t *file, const wchar_t *filter, DWORD filterIndex)
{
    WCHAR path[MAX_PATH] = {0};

    // File is passed as the format string (no arguments), as in the code.
    if (file)
        wsprintfW(path, file);
    OPENFILENAMEW dialog;
    memset(&dialog, 0, sizeof(dialog));
    dialog.hwndOwner = g_window;
    dialog.hInstance = g_instance;
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFile = path;
    dialog.lpstrTitle = title;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrFilter = filter;
    dialog.nFilterIndex = filterIndex;
    dialog.lpstrDefExt = L"";
    dialog.lpstrFileTitle = NULL;
    dialog.nMaxFileTitle = 0;
    dialog.lpstrInitialDir = NULL;
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (GetSaveFileNameW(&dialog))
        return path;
    return L"";
}

void showMessageBox(const std::wstring &caption, const std::wstring &text)
{
    MessageBoxW(g_window, text.c_str(), caption.c_str(), MB_OK);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
int askQuestion(const std::wstring &caption, const std::wstring &text, bool allowCancel)
{
    switch (MessageBoxW(g_window, text.c_str(), caption.c_str(), allowCancel ? MB_YESNOCANCEL : MB_YESNO)) {
    case IDYES:
        return ANSWER_YES;
    case IDNO:
        return ANSWER_NO;
    }
    return ANSWER_CANCEL;
}

#ifdef PLASMA_NO_LICENSING

// AskText's dialog: a prompt, an edit box and OK/Cancel, built in memory.
struct TextDialog {
    const std::wstring *text;
    std::wstring answer;
};

enum TextDialogControl {
    ID_PROMPT = 100,
    ID_ANSWER = 101
};

static INT_PTR CALLBACK textDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    TextDialog *state = reinterpret_cast<TextDialog *>(GetWindowLongPtrW(dialog, DWLP_USER));
    switch (message) {
    case WM_INITDIALOG:
        SetWindowLongPtrW(dialog, DWLP_USER, lParam);
        state = reinterpret_cast<TextDialog *>(lParam);
        SetDlgItemTextW(dialog, ID_PROMPT, state->text->c_str());
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            HWND edit = GetDlgItem(dialog, ID_ANSWER);
            std::vector<wchar_t> buffer(GetWindowTextLengthW(edit) + 1);
            GetWindowTextW(edit, &buffer[0], static_cast<int>(buffer.size()));
            state->answer = &buffer[0];
            EndDialog(dialog, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// Appends a word-aligned dialog item (a predefined control class by atom).
static void addDialogItem(std::vector<WORD> &memory, DWORD style, short x, short y, short width, short height, WORD id,
                          WORD controlClass, const wchar_t *title)
{
    if (memory.size() % 2)
        memory.push_back(0);
    DLGITEMTEMPLATE item = {style | WS_CHILD | WS_VISIBLE, 0, x, y, width, height, id};
    const WORD *words = reinterpret_cast<const WORD *>(&item);
    memory.insert(memory.end(), words, words + sizeof(item) / sizeof(WORD));
    memory.push_back(0xffff);
    memory.push_back(controlClass);
    for (; *title; ++title)
        memory.push_back(*title);
    memory.push_back(0);
    memory.push_back(0);
}

bool askText(const std::wstring &caption, const std::wstring &text, std::wstring &answer)
{
    std::vector<WORD> memory;
    DLGTEMPLATE header = {
        DS_MODALFRAME | DS_CENTER | DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU, 0, 4, 0, 0, 240, 80};
    const WORD *words = reinterpret_cast<const WORD *>(&header);
    memory.insert(memory.end(), words, words + sizeof(header) / sizeof(WORD));
    memory.push_back(0); // No menu.
    memory.push_back(0); // The standard dialog class.
    memory.insert(memory.end(), caption.begin(), caption.end());
    memory.push_back(0);
    memory.push_back(8); // DS_SETFONT: 8 point MS Shell Dlg.
    const wchar_t font[] = L"MS Shell Dlg";
    memory.insert(memory.end(), font, font + sizeof(font) / sizeof(font[0]));
    addDialogItem(memory, SS_LEFT, 7, 7, 226, 24, ID_PROMPT, DialogStaticClassAtom, L"");
    addDialogItem(memory, ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 7, 34, 226, 14, ID_ANSWER, DialogEditClassAtom, L"");
    addDialogItem(memory, BS_DEFPUSHBUTTON | WS_TABSTOP, 129, 59, 50, 14, IDOK, DialogButtonClassAtom, L"OK");
    addDialogItem(memory, BS_PUSHBUTTON | WS_TABSTOP, 183, 59, 50, 14, IDCANCEL, DialogButtonClassAtom, L"Cancel");
    TextDialog state = {&text, L""};
    INT_PTR result = DialogBoxIndirectParamW(g_instance, reinterpret_cast<DLGTEMPLATE *>(&memory[0]), g_window,
                                             textDialogProc, reinterpret_cast<LPARAM>(&state));
    if (result != IDOK)
        return false;
    answer = state.answer;
    return true;
}
#endif

void setClipboardText(const std::wstring &text)
{
    if (!text.size() || !OpenClipboard(g_window))
        return;
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
    if (!memory) {
        CloseClipboard();
        return;
    }
    EmptyClipboard();
    void *data = GlobalLock(memory);
    memcpy(data, text.c_str(), (text.size() + 1) * sizeof(wchar_t));
    GlobalUnlock(memory);
    SetClipboardData(CF_UNICODETEXT, memory);
    CloseClipboard();
}

bool getClipboardText(std::wstring &text)
{
    if (!OpenClipboard(g_window))
        return false;
    HANDLE memory = GetClipboardData(CF_UNICODETEXT);
    const wchar_t *data = 0;
    if (memory)
        data = (const wchar_t *)GlobalLock(memory);
    if (!data) {
        CloseClipboard();
        return false;
    }
    text.assign(data, wcslen(data));
    GlobalUnlock(memory);
    CloseClipboard();
    return true;
}

std::string getWindowsFontsPath()
{
    char directory[MAX_PATH];
    GetWindowsDirectoryA(directory, MAX_PATH);
    return std::string(directory) + "\\fonts";
}

// Removes every queued message for g_window, translating each but dispatching only WM_PAINT; other messages are
// consumed. Then updates the workspace pointer position and shows the frame during a long operation.
BOOL pumpMessages()
{
    MSG message;
    while (PeekMessageW(&message, g_window, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        if (message.message == WM_PAINT)
            DispatchMessageW(&message);
    }
    POINT cursor;
    GetCursorPos(&cursor);
    ScreenToClient(g_window, &cursor);
    g_workspace.mouseMove(cursor.x, cursor.y);
    g_workspace.update(0);
    g_workspace.draw();
    return SwapBuffers(GetDC(g_window));
}

void setCursorShape(int cursor)
{
    SetCursor(g_cursors[cursor]);
}

BOOL setupPixelFormat()
{
    PIXELFORMATDESCRIPTOR format = {
        sizeof(format),                                             // NSize.
        1,                                                          // NVersion.
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // DwFlags.
        PFD_TYPE_RGBA,                                              // IPixelType.
        32,                                                         // CColorBits: requests 32-bit colour.
        0,
        0, // CRedBits, cRedShift.
        0,
        0, // CGreenBits, cGreenShift.
        0,
        0, // CBlueBits, cBlueShift.
        0,
        0, // CAlphaBits, cAlphaShift: no alpha requested.
        0, // CAccumBits.
        0,
        0,
        0,
        0,              // CAccumRedBits, cAccumGreenBits, cAccumBlueBits, cAccumAlphaBits.
        32,             // CDepthBits: requests 32-bit depth.
        0,              // CStencilBits: no stencil requested.
        0,              // CAuxBuffers.
        PFD_MAIN_PLANE, // ILayerType.
        0,              // BReserved.
        0,
        0,
        0 // DwLayerMask, dwVisibleMask, dwDamageMask.
    };
    int pixelFormat = ChoosePixelFormat(g_deviceContext, &format);
    return SetPixelFormat(g_deviceContext, pixelFormat, &format);
}

bool initPlasma()
{
    g_deviceContext = GetDC(g_window);
    setupPixelFormat();
    g_glContext = wglCreateContext(g_deviceContext);
    wglMakeCurrent(g_deviceContext, g_glContext);
    glewInit();
    g_initialized = g_workspace.init();
    return g_initialized;
}

void updateDimensions()
{
    RECT rect;
    GetClientRect(g_window, &rect);
    g_workspace.setDimensions(rect.right, rect.bottom);
    GetClientRect(GetDesktopWindow(), &rect);
    g_workspace.m_engine.setDimensions(rect.right, rect.bottom);
}

void draw()
{
    if (g_initialized) {
        g_workspace.draw();
        SwapBuffers(g_deviceContext);
    }
}

void print()
{
    if (!g_workspace.getOpenDesignSheet())
        return;
    if (!g_workspace.m_designSheet)
        return;
    plasma::Vector<2, float> boundsMin(0.0f, 0.0f);
    plasma::Vector<2, float> boundsMax(0.0f, 0.0f);
    g_workspace.m_exportModule->getExportBounds(boundsMin, boundsMax);

    // Unordered bounds also pass this guard.
    if (!(boundsMax.m_elements[0] <= boundsMin.m_elements[0]) &&
        !(boundsMax.m_elements[1] <= boundsMin.m_elements[1])) {
        PRINTDLGW dialog = {0};
        dialog.lStructSize = sizeof(dialog);
        dialog.Flags = PD_RETURNDC;
        if (PrintDlgW(&dialog)) {
            g_workspace.m_exportModule->renderImageToFile(getAppDataPath() + L"\\Cache\\print.bmp", boundsMin,
                                                          boundsMax);
            DOCINFOW document = {0};
            document.cbSize = sizeof(document);
            document.lpszDocName = L"Plasma Printout";
            document.lpszOutput = NULL;
            document.lpszDatatype = NULL;
            document.fwType = 0;
            HBITMAP image =
                (HBITMAP)LoadImageW(GetModuleHandleW(NULL), (getAppDataPath() + L"\\Cache\\print.bmp").c_str(),
                                    IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE | LR_DEFAULTSIZE);

            // The code keeps the print-dialog DC when loading the image fails.
            if (image) {
                HDC memoryDC = CreateCompatibleDC(NULL);
                HGDIOBJ oldImage = SelectObject(memoryDC, image);
                BITMAP bitmap;
                GetObjectW(image, sizeof(bitmap), &bitmap);
                int width = bitmap.bmWidth;
                int height = bitmap.bmHeight;
                BITMAPINFOHEADER header = {0};
                header.biSize = sizeof(header);
                header.biWidth = width;
                header.biHeight = height;
                header.biPlanes = 1;
                header.biBitCount = 24;
                header.biCompression = BI_RGB;
                GetDIBits(memoryDC, image, 0, height, NULL, (BITMAPINFO *)&header, DIB_RGB_COLORS);
                char *pixels = new char[header.biSizeImage];
                GetDIBits(memoryDC, image, 0, height, pixels, (BITMAPINFO *)&header, DIB_RGB_COLORS);
                float scaleX = (float)GetDeviceCaps(dialog.hDC, HORZRES) / (float)width;
                float scaleY = (float)GetDeviceCaps(dialog.hDC, VERTRES) / (float)height;
                if (scaleX > scaleY)
                    scaleX = scaleY;
                else
                    scaleY = scaleX;
                GetDeviceCaps(dialog.hDC, HORZRES);
                GetDeviceCaps(dialog.hDC, VERTRES);
                StartDocW(dialog.hDC, &document);
                StartPage(dialog.hDC);
                StretchDIBits(dialog.hDC, 0, 0, (int)((float)width * scaleX), (int)((float)height * scaleY), 0, 0,
                              width, height, pixels, (BITMAPINFO *)&header, DIB_RGB_COLORS, SRCCOPY);
                EndPage(dialog.hDC);
                EndDoc(dialog.hDC);
                DeleteDC(dialog.hDC);
                SelectObject(memoryDC, oldImage);
                DeleteObject(image);
                DeleteDC(memoryDC);
                delete[] pixels;
            }
        }
    }
}

#include "MainMenuHelpers.inc"

LRESULT CALLBACK wndProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (!g_initialized)
        return DefWindowProcW(window, message, wparam, lparam);
    g_workspace.m_tabletPressure = g_tablet.update();
    bool shift = GetKeyState(VK_SHIFT) < 0;
    bool control = GetKeyState(VK_CONTROL) < 0;
    bool alt = GetKeyState(VK_MENU) < 0;
    bool space = GetKeyState(VK_SPACE) < 0;
    std::set<unsigned short> &pressedKeys = g_workspace.m_engine.m_pressedKeys;
    if (shift) {
        if (pressedKeys.find(VK_SHIFT) == pressedKeys.end())
            g_workspace.keyPressed(VK_SHIFT);
    } else {
        if (pressedKeys.find(VK_SHIFT) != pressedKeys.end())
            g_workspace.keyReleased(VK_SHIFT);
    }
    if (alt) {
        if (pressedKeys.find(VK_MENU) == pressedKeys.end())
            g_workspace.keyPressed(VK_MENU);
    } else {
        if (pressedKeys.find(VK_MENU) != pressedKeys.end())
            g_workspace.keyReleased(VK_MENU);
    }
    if (control) {
        if (pressedKeys.find(VK_CONTROL) == pressedKeys.end())
            g_workspace.keyPressed(VK_CONTROL);
    } else {
        if (pressedKeys.find(VK_CONTROL) != pressedKeys.end())
            g_workspace.keyReleased(VK_CONTROL);
    }
    if (space) {
        if (pressedKeys.find(VK_SPACE) == pressedKeys.end())
            g_workspace.keyPressed(VK_SPACE);
    } else {
        if (pressedKeys.find(VK_SPACE) != pressedKeys.end())
            g_workspace.keyReleased(VK_SPACE);
    }
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 1;
    case WM_CLOSE:
        if (g_workspace.canClose()) {
            g_workspace.closeAllSheets();
            DestroyWindow(window);
        }
        return 1;
    case WM_SIZE:
        updateDimensions();
        if (g_initialized)
            draw();
        return 1;
    case WM_PAINT:
        if (g_initialized)
            draw();
        break;
    case WM_KILLFOCUS:
        g_workspace.releaseButtons();
        break;
    case WM_CHAR:
        g_workspace.characterEntered((int)wparam);
        break;
    case WM_KEYDOWN:
        if (wparam == VK_F11)
            hideUserInterface();
        else
            g_workspace.keyPressed((unsigned short)wparam);
        return 0;
    case WM_KEYUP:
        g_workspace.keyReleased((unsigned short)wparam);
        return 0;
    case WM_COPYDATA: {
        COPYDATASTRUCT *data = reinterpret_cast<COPYDATASTRUCT *>(lparam);
        const char *path = static_cast<const char *>(data->lpData);
        if (path && isPlasmaFile(widenString(std::string(path)))) {
            g_workspace.newSheet();

            // The code assumes GetOpenSheet succeeded.
            g_workspace.getOpenSheet()->m_closeIfLoadFails = true;
            g_workspace.openPlasmaFile(widenString(std::string(path)));
            g_workspace.m_redrawRequested = true;
        }
        break;
    }
#include "MainMenuCommands.inc"

    case WM_SYSCOMMAND:
        if (wparam == SC_SCREENSAVE || wparam == SC_MONITORPOWER || wparam == SC_KEYMENU)
            return 0;
        break;
    case WM_ERASEBKGND:
        return 0;
    case WM_TIMER:
        g_workspace.relogin();
        break;
    case WM_MOUSEMOVE:
        g_workspace.mouseMove((short)LOWORD(lparam), (short)HIWORD(lparam));
        g_workspace.updateCursor();
        return 0;
    case WM_MOUSEWHEEL: {
        short delta = (short)HIWORD(wparam);
        if (delta < 0)
            delta = -1;
        else if (delta > 0)
            delta = 1;
        g_workspace.pointerWheel(delta);
        break;
    }
    case WM_LBUTTONDOWN:
        g_workspace.pointerPressed();
        SetCapture(window);
        break;
    case WM_LBUTTONUP:
        g_tablet.resetPressure();
        g_workspace.pointerReleased();
        if (GetCapture() == window)
            ReleaseCapture();
        break;
    case WM_LBUTTONDBLCLK:
        g_workspace.pointerDoubleClicked();
        break;
    case WM_RBUTTONDOWN:
        g_workspace.rightButtonPressed();
        SetCapture(window);
        break;
    case WM_RBUTTONUP:
        g_workspace.rightButtonReleased();
        if (GetCapture() == window)
            ReleaseCapture();
        break;
    case WM_MBUTTONDOWN:
        g_workspace.middleButtonPressed();
        SetCapture(window);
        break;
    case WM_MBUTTONUP:
        g_workspace.middleButtonReleased();
        if (GetCapture() == window)
            ReleaseCapture();
        break;
    case WM_DROPFILES: {
        wchar_t path[MAX_PATH];
        DragQueryFileW((HDROP)wparam, 0, path, MAX_PATH);
        if (isPlasmaFile(std::wstring(path))) {
            g_workspace.newSheet();

            // The code assumes GetOpenSheet succeeded.
            g_workspace.getOpenSheet()->m_closeIfLoadFails = true;
            g_workspace.openPlasmaFile(std::wstring(path));
            g_workspace.m_redrawRequested = true;

            // The code leaves non-Plasma drops unfinished.
            DragFinish((HDROP)wparam);
        }
        break;
    }
    }
    if (message == WM_NCMOUSEMOVE && !g_workspace.m_optionsModule->m_cursorHardware) {
        setCursorShape(CURSOR_ARROW);
        g_workspace.hideSoftwareCursor();
    }
#include "MainMenuState.inc"

    return DefWindowProcW(window, message, wparam, lparam);
}

void updateWindowTitle(const std::wstring &name)
{
    std::wstring title(L"Picroma Plasma");
    if (name.empty())
        SetWindowTextW(g_window, title.c_str());
    else
        SetWindowTextW(g_window, (name + L" - " + title).c_str());
    buildSheetMenu();
}

void addRecentFile(const std::wstring &path)
{
    buildSheetMenu();
    g_recentFiles.remove(std::wstring(path));
    g_recentFiles.push_front(std::wstring(path));
    if (g_recentFiles.size() > MAX_RECENT_FILES)
        g_recentFiles.pop_back();
    std::wofstream stream(PLASMA_FILE_PATH(s_appDataPath + L"\\Cache\\recent.log"));
    if (stream.is_open()) {
        for (std::list<std::wstring>::iterator it = g_recentFiles.begin(); it != g_recentFiles.end(); ++it)
            stream << *it << std::endl;
    }
    createFileMenu();
}

HWND getMainWindow()
{
    return g_window;
}

HGLRC getGLContext()
{
    return g_glContext;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR commandLine, int showCommand)
{
    // The window class, menu and window use the same name.
    const wchar_t *windowName = L"Picroma Plasma";
    HWND existing = FindWindowW(windowName, NULL);
    std::string command(commandLine);
    if (!command.empty() && existing) {
        std::string path = command.substr(1, command.size() - 2);
        COPYDATASTRUCT data;
        data.dwData = 0;
        data.lpData = const_cast<char *>(path.c_str());
        data.cbData = path.size() + 1;
        SendMessageW(existing, WM_COPYDATA, 0, (LPARAM)&data);
        return 0;
    }
    g_instance = instance;
    WNDCLASSW windowClass;
    windowClass.style = CS_DBLCLKS;
    windowClass.lpfnWndProc = wndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(APPLICATION_ICON));
    windowClass.hCursor = NULL;
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    windowClass.lpszMenuName = windowName;
    windowClass.lpszClassName = windowName;
    if (!RegisterClassW(&windowClass))
        return 0;
    wchar_t folder[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, folder))) {
        s_appDataPath = folder;
        s_appDataPath += L"\\Picroma";
        _wmkdir(s_appDataPath.c_str());
        s_appDataPath += L"\\Plasma";
        _wmkdir(s_appDataPath.c_str());
    }
    _wmkdir((s_appDataPath + L"\\Cache").c_str());
    _wmkdir((s_appDataPath + L"\\Backup").c_str());
    std::wifstream recentFile(PLASMA_FILE_PATH(s_appDataPath + L"\\Cache\\recent.log"));
    wchar_t line[RecentFileLineCapacity];
    if (recentFile.is_open()) {
        while (!recentFile.eof() && g_recentFiles.size() < MAX_RECENT_FILES) {
            recentFile.getline(line, RecentFileLineCapacity);
            if (std::wstring(line).empty())
                continue;
            std::wstring file(line);
            g_recentFiles.push_back(file);
        }
    }
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, folder))) {
        s_commonAppDataPath = folder;
        s_commonAppDataPath += L"\\Picroma";
        _wmkdir(s_commonAppDataPath.c_str());
        s_commonAppDataPath += L"\\Plasma";
        _wmkdir(s_commonAppDataPath.c_str());
    }
    char colorDirectory[MAX_PATH];
    DWORD colorDirectorySize = MAX_PATH;

    // The code ignores failure and still uses the uninitialized buffer.
    GetColorDirectoryA(NULL, colorDirectory, &colorDirectorySize);
    s_colorDirectory = colorDirectory;
#include "MainMenuBuild.inc"

    g_window = CreateWindowExW(0, windowName, windowName, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0,
                               1280, 960, NULL, menu, instance, NULL);
    if (!g_window)
        return 0;
    DragAcceptFiles(g_window, TRUE);
    g_cursors[CURSOR_NONE] = NULL;
    g_cursors[CURSOR_ARROW] = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    g_cursors[CURSOR_IBEAM] = LoadCursorW(NULL, (LPCWSTR)IDC_IBEAM);
    g_cursors[CURSOR_TOOL_ARROW] = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    g_cursors[CURSOR_HAND] = LoadCursorW(NULL, (LPCWSTR)IDC_HAND);
    g_cursors[CURSOR_HAND2] = LoadCursorW(NULL, (LPCWSTR)IDC_HAND);
    g_cursors[CURSOR_SIZE_ALL] = LoadCursorW(NULL, (LPCWSTR)IDC_SIZEALL);
    g_cursors[CURSOR_SIZE_WE] = LoadCursorW(NULL, (LPCWSTR)IDC_SIZEWE);
    g_cursors[CURSOR_SIZE_NS] = LoadCursorW(NULL, (LPCWSTR)IDC_SIZENS);
    g_cursors[CURSOR_SIZE_NESW] = LoadCursorW(NULL, (LPCWSTR)IDC_SIZENESW);
    g_cursors[CURSOR_SIZE_NWSE] = LoadCursorW(NULL, (LPCWSTR)IDC_SIZENWSE);
    setCursorShape(CURSOR_ARROW);
    char executable[MAX_PATH];
    GetModuleFileNameA(NULL, executable, MAX_PATH - 1);
    std::string path(executable);
    path = path.substr(0, path.rfind("\\"));
    _chdir(path.c_str());
    initPlasma();
    if (!g_initialized) {
        MessageBoxW(NULL, L"Could not initialize Plasma. Please reinstall Plasma to fix this problem.",
                    L"Picroma Plasma", MB_OK);
        DestroyWindow(g_window);
        return 0;
    }
    ShowWindow(g_window, SW_SHOWMAXIMIZED);
    updateDimensions();
    draw();
    g_workspace.runStartupScript();
#include "MainMenuZoom.inc"

    if (g_workspace.m_activated)
        g_workspace.newSheet();
    g_tablet.init();
#if !defined(PLASMA_NO_LICENSING) && !defined(PLASMA_NO_INTERNET)
    SetTimer(g_window, RELOGIN_TIMER, RELOGIN_INTERVAL, NULL);
#endif
    bool running = true;
    MSG message;
    do {
        if (g_workspace.m_restartAfterUpdate)
            break;
        // A destroyed window can make GetMessage fail during shutdown.
        if (GetMessageW(&message, g_window, 0, 0) <= 0)
            break;
        TranslateMessage(&message);
        if (!message.message || message.message == WM_QUIT)
            break;
        DispatchMessageW(&message);
        DWORD now = timeGetTime();
        DWORD previousTime = now;
        while (g_workspace.m_redrawRequested || g_workspace.m_engine.m_root->m_changed ||
               g_workspace.m_guiPointerNode && g_workspace.m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) ||
               g_workspace.m_textInterface) {
            if (!running || !IsWindowVisible(g_window))
                break;
            if (!command.empty() && (int)g_workspace.m_sheetTemplate->m_children.size() > 0) {
                std::wstring file = widenString(command.substr(1, command.size() - 2));
                if (isPlasmaFile(file))
                    g_workspace.openPlasmaFile(file);
                command.clear();
            }
            if ((int)(now - s_lastFontCleanup) > FontCacheCleanupIntervalMilliseconds) {
                g_workspace.m_engine.getFontEngine()->collectUnusedGlyphs();
                s_lastFontCleanup = now;
            }
            if (g_workspace.m_restartAfterUpdate)
                break;
            now = timeGetTime();
            g_workspace.update(now - previousTime);
            if (g_workspace.m_restartAfterUpdate)
                break;
            previousTime = now;
            draw();
            if (PeekMessageW(&message, g_window, 0, 0, PM_REMOVE)) {
                do {
                    TranslateMessage(&message);
                    if (!message.message || message.message == WM_QUIT) {
                        running = false;
                        break;
                    }
                    DispatchMessageW(&message);
                } while (PeekMessageW(&message, g_window, 0, 0, PM_REMOVE));
            }
        }
    } while (running);
    g_workspace.cleanupForExit();
    wglDeleteContext(g_glContext);
    ReleaseDC(g_window, g_deviceContext);
    if (g_workspace.m_restartAfterUpdate) {
        char restart[MAX_PATH];
        GetModuleFileNameA(NULL, restart, MAX_PATH - 1);
        WinExec(restart, SW_SHOWDEFAULT);
    }
    return 1;
}

#include "MainWidgets.inc"
