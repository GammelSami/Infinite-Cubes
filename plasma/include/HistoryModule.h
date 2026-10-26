// The history toolbar ("module:history", "history:undo", "history:redo").
#pragma once
#include "Module.h"

class HistoryModule : public Module {
public:
    HistoryModule();
    virtual void build();
    void undo();
    void redo();
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
