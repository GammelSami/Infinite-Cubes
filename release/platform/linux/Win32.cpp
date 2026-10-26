// The Win32 functions declared by platform/linux/include, on POSIX.
#include <windows.h>
#include <mmsystem.h>
#include <cerrno>
#include <chrono>
#include <string>
#include <thread>
#include <unistd.h>

void InitializeCriticalSection(CRITICAL_SECTION *section)
{
    pthread_mutexattr_t attributes;
    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&section->mutex, &attributes);
    pthread_mutexattr_destroy(&attributes);
}

BOOL InitializeCriticalSectionAndSpinCount(CRITICAL_SECTION *section, DWORD)
{
    InitializeCriticalSection(section);
    return TRUE;
}

void EnterCriticalSection(CRITICAL_SECTION *section)
{
    pthread_mutex_lock(&section->mutex);
}

BOOL TryEnterCriticalSection(CRITICAL_SECTION *section)
{
    return pthread_mutex_trylock(&section->mutex) == 0;
}

void LeaveCriticalSection(CRITICAL_SECTION *section)
{
    pthread_mutex_unlock(&section->mutex);
}

void DeleteCriticalSection(CRITICAL_SECTION *section)
{
    pthread_mutex_destroy(&section->mutex);
}

void Sleep(DWORD milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// errno stands in for the thread's last error; only file failures set it.
DWORD GetLastError()
{
    return (DWORD)errno;
}

HANDLE GetCurrentThread()
{
    return (HANDLE)(intptr_t)-2;
}

BOOL SetThreadPriority(HANDLE, int)
{
    return TRUE;
}

DWORD timeGetTime()
{
    static const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    return (DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start)
        .count();
}

MMRESULT timeBeginPeriod(UINT)
{
    return TIMERR_NOERROR;
}

MMRESULT timeEndPeriod(UINT)
{
    return TIMERR_NOERROR;
}

// Wide paths are UTF-32 here; the game's file names are ASCII, so a narrowing copy suffices.
static std::string narrowPath(LPCWSTR path)
{
    std::string narrow;
    for (; *path; path++) {
        narrow += *path == L'\\' ? '/' : (char)*path;
    }
    return narrow;
}

HANDLE CreateFileW(LPCWSTR fileName, DWORD access, DWORD, void *, DWORD creation, DWORD, HANDLE)
{
    const char *mode = "rb";
    if (access & GENERIC_WRITE) {
        mode = creation == CREATE_ALWAYS ? "wb" : "r+b";
    }
    FILE *file = fopen(narrowPath(fileName).c_str(), mode);
    return file ? (HANDLE)file : INVALID_HANDLE_VALUE;
}

BOOL ReadFile(HANDLE file, void *buffer, DWORD size, DWORD *read, void *)
{
    size_t count = fread(buffer, 1, size, (FILE *)file);
    if (read) {
        *read = (DWORD)count;
    }
    return count == size || feof((FILE *)file);
}

DWORD SetFilePointer(HANDLE file, LONG distance, LONG *distanceHigh, DWORD method)
{
    long long offset = distance;
    if (distanceHigh) {
        offset = ((long long)*distanceHigh << 32) | (DWORD)distance;
    }
    int whence = method == FILE_CURRENT ? SEEK_CUR : method == FILE_END ? SEEK_END : SEEK_SET;
    if (fseeko((FILE *)file, offset, whence) != 0) {
        return INVALID_SET_FILE_POINTER;
    }
    long long position = ftello((FILE *)file);
    if (distanceHigh) {
        *distanceHigh = (LONG)(position >> 32);
    }
    return (DWORD)position;
}

BOOL CloseHandle(HANDLE handle)
{
    return handle && handle != INVALID_HANDLE_VALUE && fclose((FILE *)handle) == 0;
}

int strcpy_s(char *destination, size_t size, const char *source)
{
    if (!destination || !source || size == 0) {
        return EINVAL;
    }
    size_t length = strlen(source);
    if (length >= size) {
        destination[0] = 0;
        return ERANGE;
    }
    memcpy(destination, source, length + 1);
    return 0;
}

BOOL DeleteFileA(LPCSTR fileName)
{
    return unlink(fileName) == 0;
}
