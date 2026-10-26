#include "wintab_api.h"

namespace {
HMODULE tabletDriver()
{
    static HMODULE driver = LoadLibraryW(L"Wintab32.dll");
    return driver;
}
template<class T> T tabletFunction(const char *name)
{
    HMODULE driver = tabletDriver();
    return driver ? reinterpret_cast<T>(GetProcAddress(driver, name)) : nullptr;
}
}
extern "C" UINT WINAPI WTInfoW(UINT category, UINT index, LPVOID output)
{
    static auto function = tabletFunction<UINT(WINAPI *)(UINT, UINT, LPVOID)>("WTInfoW");
    return function ? function(category, index, output) : 0;
}
extern "C" HCTX WINAPI WTOpenW(HWND window, LOGCONTEXTW *context, BOOL enable)
{
    static auto function = tabletFunction<HCTX(WINAPI *)(HWND, LOGCONTEXTW *, BOOL)>("WTOpenW");
    return function ? function(window, context, enable) : nullptr;
}
extern "C" int WINAPI WTPacketsGet(HCTX context, int maximum, LPVOID packets)
{
    static auto function = tabletFunction<int(WINAPI *)(HCTX, int, LPVOID)>("WTPacketsGet");
    return function ? function(context, maximum, packets) : 0;
}
