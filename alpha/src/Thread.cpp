// Thread helpers copy a callable into a new Windows thread and release it when the thread finishes.
#include "Thread.h"

#include <process.h>

// The thread's entry: runs the function, frees the copy StartThread made and ends the thread.
static void __cdecl ThreadProc(void *argument)
{
    std::function<void()> *function = static_cast<std::function<void()> *>(argument);
    (*function)();
    delete function;
    _endthread();
}

HANDLE StartThread(const std::function<void()> &function)
{
    return (HANDLE)_beginthread(ThreadProc, 0, new std::function<void()>(function));
}
