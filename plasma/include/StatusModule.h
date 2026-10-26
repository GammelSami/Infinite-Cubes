// "module:status", descriptions from tooltips_en.xml.
#pragma once
#include "Module.h"
#include "plasma/TextShape.h"
#include "StatusWidget.h"

struct StatusInfo {
    StatusInfo(const std::wstring &title, const std::wstring &text, const std::string &icon,
               const std::wstring &annotation, const std::wstring &extraText);
    ~StatusInfo();
    std::wstring m_title;      // XML "title" / status:name.
    std::wstring m_text;       // XML "text".
    std::string m_icon;        // XML "icon".
    std::wstring m_annotation; // XML "annotation".

    // Always empty. UpdatePanel copies it into StatusWidget, but Render never draws it.
    std::wstring m_extraText;
};

class StatusModule : public Module {
public:
    StatusModule();
    virtual ~StatusModule();
    virtual void build();
    virtual void updatePanel();

    plasma::Node *getIcon() { return m_icon; }

    StatusWidget *m_tooltip;
    plasma::Node *m_icon;
    std::map<std::string, StatusInfo *> m_tooltips; // XML "key".
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
