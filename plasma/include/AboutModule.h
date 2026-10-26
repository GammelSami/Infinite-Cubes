// "module:about", caption "About Picroma Plasma".
#pragma once
#include "Module.h"

class AboutModule : public Module {
public:
    AboutModule();
    virtual void build();
    void close();
};
