// The Win32 subset the game uses, on top of DXVK-native's Windows base types. Implemented in platform/linux/Win32.cpp.
#pragma once
#include <windows_base.h>
#include <unknwn.h>
#include <pthread.h>
#include <cstdlib>
// The Windows C runtime's range for rand (platform/linux/Rand.c).
#undef RAND_MAX
#define RAND_MAX 0x7fff
#include <cstdio>
#include <cstring>

#define CALLBACK
#define __cdecl
#define __int64 long long
#define __forceinline inline __attribute__((always_inline))
#define __declspec(x)

typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
typedef long long __time64_t;

// Critical sections are recursive, as on Windows.
typedef struct CRITICAL_SECTION {
    pthread_mutex_t mutex;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;

void InitializeCriticalSection(CRITICAL_SECTION *section);
BOOL InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION *section, DWORD spinCount);
void EnterCriticalSection(CRITICAL_SECTION *section);
BOOL TryEnterCriticalSection(CRITICAL_SECTION *section);
void LeaveCriticalSection(CRITICAL_SECTION *section);
void DeleteCriticalSection(CRITICAL_SECTION *section);

void Sleep(DWORD milliseconds);
DWORD GetLastError();

// Threads: priorities are advisory and ignored.
#define THREAD_PRIORITY_LOWEST -2
#define THREAD_PRIORITY_BELOW_NORMAL -1
#define THREAD_PRIORITY_NORMAL 0
#define THREAD_PRIORITY_ABOVE_NORMAL 1
#define THREAD_PRIORITY_HIGHEST 2
HANDLE GetCurrentThread();
BOOL SetThreadPriority(HANDLE thread, int priority);

// Files, for the sounds read with CreateFileW / ReadFile.
#define GENERIC_READ 0x80000000u
#define GENERIC_WRITE 0x40000000u
#define FILE_SHARE_READ 0x00000001u
#define FILE_SHARE_WRITE 0x00000002u
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define FILE_ATTRIBUTE_NORMAL 0x80
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#define ERROR_SUCCESS 0L
#define ERROR_HANDLE_EOF 38L
#define ERROR_DEVICE_NOT_CONNECTED 1167L
#define HRESULT_FROM_WIN32(x) \
    ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : ((HRESULT)(((x)&0x0000FFFF) | (7 << 16) | 0x80000000)))
HANDLE CreateFileW(LPCWSTR fileName, DWORD access, DWORD shareMode, void *security, DWORD creation, DWORD flags,
                   HANDLE templateFile);
BOOL ReadFile(HANDLE file, void *buffer, DWORD size, DWORD *read, void *overlapped);
DWORD SetFilePointer(HANDLE file, LONG distance, LONG *distanceHigh, DWORD method);
BOOL CloseHandle(HANDLE handle);

int strcpy_s(char *destination, size_t size, const char *source);

// Virtual-key codes, as WM_KEYDOWN's wParam (platform/linux/main.cpp translates SDL keys to them).
#define VK_LBUTTON 0x01
#define VK_RBUTTON 0x02
#define VK_MBUTTON 0x04
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_NUMPAD0 0x60
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_LMENU 0xA4
#define VK_RMENU 0xA5


#define LOWORD(l) ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
typedef ULONG_PTR DWORD_PTR;

BOOL DeleteFileA(LPCSTR fileName);

// The C runtime's daylight-saving flag; the renderer scales ground shadows by it.
#include <ctime>
#define _daylight daylight
