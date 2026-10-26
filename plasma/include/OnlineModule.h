// "module:online": activation/login options and the four "online:serial:*" blocks.
#pragma once
#include "Module.h"
#include "plasma/Node.h"
#include "plasma/Edit.h"
#include "plasma/Display.h"
#include "plasma/DiscreteAttribute.h"
#include "ColorModule.h"
#include "StrokeWidthModule.h"

class OnlineModule : public Module {
public:
    enum LicenseMode {
        LICENSE_OFFLINE = 0,
        LICENSE_ACTIVATE = 1,
        LICENSE_LOGIN = 2
    };

    OnlineModule();
    virtual void build();
    void pasteSerial();
    void update();
    void login();
    void activate();
    void close();
    void editSerial();
    void editSerial1();
    void editSerial2();
    void editSerial3();
    void editSerial4();
    std::string getLicenseSerial();
    void chooseSerialVisible();

    std::string m_serial; // Configuration's serial number.
    int m_licenseMode;    // 0 inactive, 1 activate, 2 online login.
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif

// Helpers for accessing members of other classes.
namespace plasma {
class Display;
class Engine;
}

// Accessors implemented in support/.
plasma::Node *getWorkspaceDialogNode(Workspace *workspace);
void setNodeFillColor(plasma::Node *node, const plasma::Vector<4, float> &color);
float getWorkspaceStrokeWidth(Workspace *workspace);

float roundToTenths(float value);
