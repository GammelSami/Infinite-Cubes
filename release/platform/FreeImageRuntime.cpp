#include <windows.h>

extern "C" void __stdcall FreeImage_Initialise(BOOL loadLocalPluginsOnly);
extern "C" void __stdcall FreeImage_DeInitialise();

namespace {
// Static linking replaces the initialization previously performed by FreeImage's DLL.
struct FreeImageRuntime {
    FreeImageRuntime() { FreeImage_Initialise(TRUE); }
    ~FreeImageRuntime() { FreeImage_DeInitialise(); }
};
FreeImageRuntime freeImageRuntime;
}
