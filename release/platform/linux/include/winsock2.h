// The game keeps a socket handle but never opens one in this build; closing it maps to close().
#pragma once
#include <windows.h>
#include <unistd.h>

typedef UINT_PTR SOCKET;
inline int closesocket(SOCKET socket) { return close((int)socket); }
