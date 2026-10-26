#include "Thread.h"
#include <windows.h>
#include <process.h>
#include <GL/glew.h>
#include "main.h"

static void __cdecl threadProc(void *argument)
{
    std::tr1::function<void()> *function = static_cast<std::tr1::function<void()> *>(argument);
    (*function)();
    delete function;
}

HANDLE startThread(const std::tr1::function<void()> &function)
{
    return (HANDLE)_beginthread(threadProc, 0, new std::tr1::function<void()>(function));
}

// What a GL thread receives: its function, and the context it makes current on the main window's device context.
struct GLThread {
    std::tr1::function<void()> *function;
    HGLRC context;
    HDC deviceContext;
};

static void __cdecl gLThreadProc(void *argument)
{
    GLThread *thread = static_cast<GLThread *>(argument);
    wglMakeCurrent(thread->deviceContext, thread->context);
    (*thread->function)();
    glFlush();
    glFinish();
    wglMakeCurrent(0, 0);
    wglDeleteContext(thread->context);
    delete thread->function;
    delete thread;
    _endthread();
}

HANDLE startGLThread(const std::tr1::function<void()> &function)
{
    GLThread *thread = new GLThread();
    thread->deviceContext = GetDC(getMainWindow());
    HGLRC mainContext = getGLContext();
    wglMakeCurrent(0, 0);
    thread->context = wglCreateContext(thread->deviceContext);
    wglShareLists(mainContext, thread->context);
    wglMakeCurrent(thread->deviceContext, mainContext);
    thread->function = new std::tr1::function<void()>(function);
    return (HANDLE)_beginthread(gLThreadProc, 0, thread);
}
