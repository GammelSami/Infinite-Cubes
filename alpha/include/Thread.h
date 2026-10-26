// Runs a callable on a new Windows thread.
#pragma once
#include <windows.h>
#include <functional>

// Returns the thread handle so callers can wait for completion.
HANDLE StartThread(const std::function<void()> &function);
