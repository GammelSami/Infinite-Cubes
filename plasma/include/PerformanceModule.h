// The "module:performance" toolbar's blur and realtime controls.
#pragma once
#include "Module.h"

class PerformanceModule : public Module {
public:
    enum BlurMode {
        BLUR_DISABLED = 0,
        BLUR_AFTERWARDS = 1,
        BLUR_REALTIME = 2
    };

    PerformanceModule();
    virtual void build();
    void disableBlur();
    void blurAfterwards();
    void blurRealtime();
    void chooseRealtimeUpdate();

    int m_blurMode;        // "performance:blur:*".
    bool m_realtimeUpdate; // "performance:realtime:update".
};

