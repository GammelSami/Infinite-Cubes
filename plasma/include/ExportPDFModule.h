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

