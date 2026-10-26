// Thread helpers run a function on a new thread, optionally with an OpenGL context that shares the main context's
// objects.
#pragma once
#include <windows.h>
#include <functional>

// Runs the function on a new thread (Workspace).
// Each returns the new thread's handle (_beginthread's result).
HANDLE startThread(const std::tr1::function<void()> &function);

// Runs the function on a new thread with a new OpenGL context on the main window, sharing the main context's lists
// (the file loaders and writers, the special colour module's preview worker).
HANDLE startGLThread(const std::tr1::function<void()> &function);
