// Pen tablet input through Wintab: one global instance (owned by WindowsUI.cpp) reports the pen pressure to the
// drawing tools.
#pragma once
#include <windows.h>
#include "wintab_api.h"
#include "plasma/Vector.h"

// The packet layout these flags select (pktdef.h with PACKETDATA and PACKETMODE defined so): buttons as relative
// events, position, pressure.
#define PACKETDATA (PK_BUTTONS | PK_X | PK_Y | PK_NORMAL_PRESSURE)
#define PACKETMODE PK_BUTTONS

struct PACKET {
    DWORD pkButtons; // HIWORD: TBN_UP or TBN_DOWN, LOWORD: button.
    LONG pkX;
    LONG pkY;
    UINT pkNormalPressure;
};

class Tablet {
public:
    Tablet();
    virtual ~Tablet();

    // Opens a system context on the main window; false without a tablet.
    bool init();
    void resetPressure();

    // Drains the packet queue and returns the current pen pressure (1 when no tablet is present or the pen is up).
    float update();

private:
    bool m_present;
    HCTX m_context;
    HWND m_window;
    float m_unused10; // Zeroed by the constructor, never read (private; Tablet.cpp only).
    float m_unused14; // Zeroed by the constructor, never read (private; Tablet.cpp only).
    float m_pressure;
    float m_pressureScale;                  // 1 / the device's maximum normal pressure.
    plasma::Vector<2, float> m_outputScale; // 1 / the context's output extent.
    bool m_penDown;
};
