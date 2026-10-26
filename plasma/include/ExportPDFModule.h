// The "module:export:pdf" dialog.
#pragma once
#include "Module.h"

class ExportPDFModule : public Module {
public:
    ExportPDFModule();
    virtual void build();
    void exportFile();
    void close();
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
