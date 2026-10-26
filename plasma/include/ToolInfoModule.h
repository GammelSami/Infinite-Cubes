// Descriptions loaded from "toolinfos_en.xml" for the current editor tool.
#pragma once
#include <map>
#include <string>
#include "Module.h"

struct ToolInfo {
    ToolInfo();
    ~ToolInfo();

    std::wstring m_left;
    std::wstring m_leftShift;
    std::wstring m_leftControl;
    std::wstring m_leftAlt;
    std::wstring m_right;
    std::wstring m_rightShift;
    std::wstring m_rightControl;
    std::wstring m_rightAlt;
};

class ToolInfoModule : public Module {
public:
    ToolInfoModule();
    virtual ~ToolInfoModule();
    virtual void updatePanel();

    std::map<std::string, ToolInfo *> m_toolInfos; // XML "key".
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
