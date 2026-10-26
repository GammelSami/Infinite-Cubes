#pragma once

// Pointer representations for the x86 build.
#if defined(PLASMA_PORTABLE) || defined(_WIN64)
#include <stdint.h>
#define PLASMA_POINTER_INT intptr_t
#define PLASMA_POINTER_UINT uintptr_t
#else
#define PLASMA_POINTER_INT int
#define PLASMA_POINTER_UINT unsigned
#endif
