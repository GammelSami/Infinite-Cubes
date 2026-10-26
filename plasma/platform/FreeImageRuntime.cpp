#include <windows.h>

#include <FreeImage.h>

namespace {
// Static linking replaces the initialization previously performed by FreeImage's DLL.
struct FreeImageRuntime {
    FreeImageRuntime() { FreeImage_Initialise(TRUE); }
    ~FreeImageRuntime() { FreeImage_DeInitialise(); }
};
FreeImageRuntime freeImageRuntime;
}
