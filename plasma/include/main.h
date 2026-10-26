// WindowsUI.cpp: the application window, its globals and the Windows helpers the rest of the program uses.
#pragma once
#include <windows.h>
#include <list>
#include <map>
#include <set>
#include <string>
#include "plasma/Engine.h"
#include "plasma/Display.h"

class Tablet;
class Workspace;
class Sheet;
class ExportModule;
class HistoryModule;
class ClipboardModule;
class DesignSheet;
class TextTool;
class NodeTool;
class VertexTool;
class PaletteModule;
class AboutModule;
class SheetTool;
namespace plasma {
class Engine;
class Display;
class FontEngine;
template<class T> class DiscreteAttribute;
}

extern HWND g_window; // The main window.
extern HINSTANCE g_instance;
extern HGLRC g_glContext;
extern HDC g_deviceContext; // The main window's device context.
extern HMENU g_fileMenu;
extern HMENU g_settingsMenu;
extern bool g_initialized;    // InitPlasma succeeded.
extern bool g_screenshotMode; // F1.
extern Tablet g_tablet;
extern std::list<std::wstring> g_recentFiles; // The file menu's recent files.
extern std::map<int, HCURSOR> g_cursors;      // By cursor number (SetCursorShape).

std::wstring getAppDataPath(); // <application data>\Picroma\Plasma.
std::wstring getCommonAppDataPath();
std::string getColorDirectory();

// The chosen file name, or "" if the user cancelled.
std::wstring openFileBox(const wchar_t *title, const wchar_t *filter);
std::wstring saveFileBox(const wchar_t *title, const wchar_t *file, const wchar_t *filter, DWORD filterIndex);

// Shows a message box with caption and text.
void showMessageBox(const std::wstring &caption, const std::wstring &text);

// Yes/no(/cancel) box: returns an ANSWER_* value.
enum {
    ANSWER_YES = 0,
    ANSWER_NO = 1,
    ANSWER_CANCEL = 2
};
int askQuestion(const std::wstring &caption, const std::wstring &text, bool allowCancel);
#ifdef PLASMA_NO_LICENSING

// One-line text box: true with the entered text, false if the user cancelled (asks for the license key of a sealed
// file; Workspace::OpenPlasmaFile).
bool askText(const std::wstring &caption, const std::wstring &text, std::wstring &answer);
#endif
void setClipboardText(const std::wstring &text);
bool getClipboardText(std::wstring &text);
std::string getWindowsFontsPath();
BOOL pumpMessages();

// The cursor numbers of g_cursors (WinMain loads them) for SetCursorShape.
enum {
    CURSOR_NONE = -1,      // No cursor (the software cursor, screenshot mode).
    CURSOR_ARROW = 0,      // IDC_ARROW.
    CURSOR_TOOL_ARROW = 1, // IDC_ARROW (the drawing tools).
    CURSOR_HAND = 2,       // IDC_HAND.
    CURSOR_HAND2 = 3,      // IDC_HAND as well; no SetCursorShape(3) seen.
    CURSOR_IBEAM = 6,      // IDC_IBEAM.
    CURSOR_SIZE_ALL = 7,   // IDC_SIZEALL.
    CURSOR_SIZE_WE = 8,    // IDC_SIZEWE.
    CURSOR_SIZE_NS = 9,    // IDC_SIZENS.
    CURSOR_SIZE_NESW = 10, // IDC_SIZENESW.
    CURSOR_SIZE_NWSE = 11  // IDC_SIZENWSE.
};
void setCursorShape(int cursor);
BOOL setupPixelFormat();
bool initPlasma();
void updateDimensions();
void draw();
bool isPlasmaFile(std::wstring path);
BOOL createFileMenu();
void buildSheetMenu();
DWORD hideUserInterface();
void print();
void addRecentFile(const std::wstring &path);
LRESULT CALLBACK wndProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
// Rebuilds the sheet menu's list of open sheets.
void buildSheetMenu();

// "<name> - Picroma Plasma" (just "Picroma Plasma" without a name), then the sheet menu.
void updateWindowTitle(const std::wstring &name);
HWND getMainWindow();
HGLRC getGLContext();

// The byte string of a wide one, character by character (defined in ColorManagementModule.cpp).
std::string narrowString(const std::wstring &text);
// Sets a named widget's text from a byte string.
void __stdcall setNodeText(plasma::Node *node, const std::wstring &name, const std::string &text);
