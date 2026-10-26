// The Wintab 1.x tablet API subset used by plasma. The declarations follow the Wintab packet and context
// specification. The platform wrapper loads WTInfoW, WTOpenW and WTPacketsGet dynamically from Wintab32.dll.
#pragma once
#include <windows.h>

DECLARE_HANDLE(HCTX);
typedef DWORD WTPKT;
typedef DWORD FIX32;

#define WT_DEFBASE 0x7FF0

#define WTI_DEFCONTEXT 3
#define WTI_DEVICES 100
#define DVC_NPRESSURE 15

#define CXO_SYSTEM 0x0001

#define PK_BUTTONS 0x0040
#define PK_X 0x0080
#define PK_Y 0x0100
#define PK_NORMAL_PRESSURE 0x0400

#define TBN_UP 1
#define TBN_DOWN 2

#define LCNAMELEN 40

typedef struct tagAXIS {
    LONG axMin;
    LONG axMax;
    UINT axUnits;
    FIX32 axResolution;
} AXIS;

typedef struct tagLOGCONTEXTW {
    WCHAR lcName[LCNAMELEN];
    UINT lcOptions;
    UINT lcStatus;
    UINT lcLocks;
    UINT lcMsgBase;
    UINT lcDevice;
    UINT lcPktRate;
    WTPKT lcPktData;
    WTPKT lcPktMode;
    WTPKT lcMoveMask;
    DWORD lcBtnDnMask;
    DWORD lcBtnUpMask;
    LONG lcInOrgX;
    LONG lcInOrgY;
    LONG lcInOrgZ;
    LONG lcInExtX;
    LONG lcInExtY;
    LONG lcInExtZ;
    LONG lcOutOrgX;
    LONG lcOutOrgY;
    LONG lcOutOrgZ;
    LONG lcOutExtX;
    LONG lcOutExtY;
    LONG lcOutExtZ;
    FIX32 lcSensX;
    FIX32 lcSensY;
    FIX32 lcSensZ;
    BOOL lcSysMode;
    int lcSysOrgX;
    int lcSysOrgY;
    int lcSysExtX;
    int lcSysExtY;
    FIX32 lcSysSensX;
    FIX32 lcSysSensY;
} LOGCONTEXTW;

extern "C" {
UINT WINAPI WTInfoW(UINT category, UINT index, LPVOID output);
HCTX WINAPI WTOpenW(HWND window, LOGCONTEXTW *context, BOOL enable);
int WINAPI WTPacketsGet(HCTX context, int maximum, LPVOID packets);
}
