// The multimedia timer: milliseconds since an arbitrary start (platform/linux/Win32.cpp).
#pragma once
#include <windows.h>

typedef UINT MMRESULT;
#define TIMERR_NOERROR 0
DWORD timeGetTime();
MMRESULT timeBeginPeriod(UINT period);
MMRESULT timeEndPeriod(UINT period);
