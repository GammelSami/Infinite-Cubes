#include "plasma/FilePath.h"
#include "OnlineModule.h"
#include "TextModule.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "FormatModule.h"
std::string narrowString(const std::wstring &text);
#include <sstream>
#include <cstdlib>

#include "main.h"
#include <fstream>
#include <algorithm>
#include <cctype>

static const int SerialCharacterCount = 16;
static const int SerialSegmentLength = 4;

void __stdcall setNodeText(plasma::Node *node, const std::wstring &name, const std::string &text)
{
    std::wstring wide(text.begin(), text.end());
    node->setText(name, wide, true);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
OnlineModule::OnlineModule()
    : m_licenseMode(LICENSE_OFFLINE)
{
    {
        std::wstring name(L"module:online");
        std::wstring kind(L"sidewidget");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 300.0f, 140.0f, 120.0f, 100.0f, g_workspace.m_guiNode, kind);
    }
    {
        std::wstring caption(L"Online options");
        m_panel->setCaption(caption, true);
    }
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, 0);
#ifndef PLASMA_NO_LICENSING
    std::ifstream file(PLASMA_FILE_PATH(getCommonAppDataPath() + L"\\config"));
    if (file.is_open()) {
        file >> m_serial;
        file >> m_licenseMode;
        if (m_licenseMode < LICENSE_OFFLINE || m_licenseMode > LICENSE_LOGIN)
            m_licenseMode = LICENSE_OFFLINE;
    }
#endif
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void OnlineModule::build()
{
#if !defined(PLASMA_NO_LICENSING)
    std::list<ModuleWidgetItem> items;
    addWidget(L"Serial number:", L"", L"static", 70, 25, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"online:serial:visible", L"checkbutton2", 0, 0, 30, 25));
    addPanel(items, L"online:serial:visible:block", 3);
    placeIcon(L"online:serial:visible", L"icon:visible");
    addWidget(L"", L"online:serial:1", L"editbox", 40, 25, 0);
    addWidget(L"", L"online:serial:2", L"editbox", 40, 25, 0);
    addWidget(L"", L"online:serial:3", L"editbox", 40, 25, 0);
    addWidget(L"", L"online:serial:4", L"editbox", 40, 25, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"online:serial:paste", L"button2", 0, 0, 30, 25));
    addPanel(items, L"online:serial:paste:block", 3);
    placeIcon(L"online:serial:paste", L"icon:paste");
    if (m_serial.size() == SerialCharacterCount) {
        setNodeText(getPanel()->getNode(), L"online:serial:1", m_serial.substr(0, SerialSegmentLength));
        setNodeText(getPanel()->getNode(), L"online:serial:2",
                    m_serial.substr(SerialSegmentLength, SerialSegmentLength));
        setNodeText(getPanel()->getNode(), L"online:serial:3",
                    m_serial.substr(2 * SerialSegmentLength, SerialSegmentLength));
        setNodeText(getPanel()->getNode(), L"online:serial:4",
                    m_serial.substr(3 * SerialSegmentLength, SerialSegmentLength));
    } else {
        setNodeText(getPanel()->getNode(), L"online:serial:1", "");
        setNodeText(getPanel()->getNode(), L"online:serial:2", "");
        setNodeText(getPanel()->getNode(), L"online:serial:3", "");
        setNodeText(getPanel()->getNode(), L"online:serial:4", "");
    }
    newRow(3);
    addWidget(L" ", L"online:status", L"static", 80, 20, 0);
    newRow(3);
    items.clear();
    items.push_back(ModuleWidgetItem(L"Login", L"online:login", L"button2", 0, 0, 100, 30));
    items.push_back(ModuleWidgetItem(L"Activate", L"online:activate", L"button2", 100, 0, 100, 30));
    addPanel(items, L"", 0);
    fitWidth(10);
    fitHeight(10);
    items.clear();
    items.push_back(ModuleWidgetItem(L"Close", L"online:close", L"button2", 0, 0, 60, 30));
    addPanelAt(items, L"", (int)(m_panel->viewWidth() - 70.0), (int)(m_panel->getViewHeight() - 40.0));
    getPanel()->getNode()->connectWidget(L"online:login", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OnlineModule::login, true);
    getPanel()->getNode()->connectWidget(L"online:activate", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OnlineModule::activate, true);
    getPanel()->getNode()->connectWidget(L"online:close", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OnlineModule::close, true);
    getPanel()->getNode()->connectWidget(L"online:serial:1", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &OnlineModule::editSerial, true);
    getPanel()->getNode()->connectWidget(L"online:serial:2", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &OnlineModule::editSerial, true);
    getPanel()->getNode()->connectWidget(L"online:serial:3", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &OnlineModule::editSerial, true);
    getPanel()->getNode()->connectWidget(L"online:serial:4", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &OnlineModule::editSerial, true);
    getPanel()->getNode()->connectWidget(L"online:serial:1", plasma::WIDGET_EVENT_TEXT_CHANGED, this,
                                         &OnlineModule::editSerial1, true);
    getPanel()->getNode()->connectWidget(L"online:serial:2", plasma::WIDGET_EVENT_TEXT_CHANGED, this,
                                         &OnlineModule::editSerial2, true);
    getPanel()->getNode()->connectWidget(L"online:serial:3", plasma::WIDGET_EVENT_TEXT_CHANGED, this,
                                         &OnlineModule::editSerial3, true);
    getPanel()->getNode()->connectWidget(L"online:serial:4", plasma::WIDGET_EVENT_TEXT_CHANGED, this,
                                         &OnlineModule::editSerial4, true);
    getPanel()->getNode()->connectWidget(L"online:serial:paste", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OnlineModule::pasteSerial, true);
    getPanel()->getNode()->connectWidget(L"online:serial:visible", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OnlineModule::chooseSerialVisible, true);
    getPanel()->getNode()->connectWidget(L"online:serial:visible", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &OnlineModule::chooseSerialVisible, true);
    // The serial shows while none is stored (m_serial's size).
    m_panel->setChecked(L"online:serial:visible", m_serial.size() == 0, false, true);
    chooseSerialVisible();
    update();
#endif
}

void OnlineModule::pasteSerial()
{
    std::wstring clipboard;
    if (!getClipboardText(clipboard))
        return;
    std::wstring serial;
    int size = clipboard.size();
    for (int i = 0; i < size; ++i) {
        if ((clipboard[i] >= L'0' && clipboard[i] <= L'9') || (clipboard[i] >= L'A' && clipboard[i] <= L'Z'))
            serial += clipboard[i];
    }
    if (serial.size() == SerialCharacterCount) {
        m_serial = narrowString(serial);
        setNodeText(getPanel()->getNode(), L"online:serial:1", m_serial.substr(0, SerialSegmentLength));
        setNodeText(getPanel()->getNode(), L"online:serial:2",
                    m_serial.substr(SerialSegmentLength, SerialSegmentLength));
        setNodeText(getPanel()->getNode(), L"online:serial:3",
                    m_serial.substr(2 * SerialSegmentLength, SerialSegmentLength));
        setNodeText(getPanel()->getNode(), L"online:serial:4",
                    m_serial.substr(3 * SerialSegmentLength, SerialSegmentLength));
    }
}

void OnlineModule::update()
{
#if !defined(PLASMA_NO_LICENSING)
    bool editable = m_workspace->getSheetTemplate()->m_children.size() == 0;
    getPanel()->getNode()->setEnabled(L"online:serial:1", editable, true);
    getPanel()->getNode()->setEnabled(L"online:serial:2", editable, true);
    getPanel()->getNode()->setEnabled(L"online:serial:3", editable, true);
    getPanel()->getNode()->setEnabled(L"online:serial:4", editable, true);
    getPanel()->getNode()->setEnabled(L"online:serial:paste", editable, true);
    if (!m_workspace->m_activated) {
        if (editable) {
            getPanel()->getNode()->setText(L"online:status", L"This product has not been activated.", true);
            getPanel()->getNode()->setText(L"online:login", L"Login", true);
        } else {
            if (m_workspace->isDemo())
                getPanel()->getNode()->setText(L"online:status", L"You are using the demo version.", true);
            else
                getPanel()->getNode()->setText(L"online:status", L"You are using the online version.", true);
            getPanel()->getNode()->setText(L"online:login", L"Logout", true);
        }
        getPanel()->getNode()->setEnabled(L"online:login", true, true);
        getPanel()->getNode()->setText(L"online:activate", L"Activate", true);
    } else {
        getPanel()->getNode()->setText(L"online:status", L"This product is activated.", true);
        getPanel()->getNode()->setEnabled(L"online:login", false, true);
        getPanel()->getNode()->setText(L"online:activate", L"Deactivate", true);
    }
#endif
}

void OnlineModule::login()
{
#if !defined(PLASMA_NO_LICENSING)
    if (m_workspace->getSheetTemplate()->m_children.size() != 0) {
        m_licenseMode = LICENSE_OFFLINE;
        std::ofstream file(PLASMA_FILE_PATH(getCommonAppDataPath() + L"\\config"));
        file << getLicenseSerial();
        file << " " << m_licenseMode;
        file.close();
        m_workspace->startLogout();
    } else {
        m_licenseMode = LICENSE_LOGIN;
        std::ofstream file(PLASMA_FILE_PATH(getCommonAppDataPath() + L"\\config"));
        file << getLicenseSerial();
        file << " " << m_licenseMode;
        file.close();
        m_workspace->startLogin();
    }
#endif
}

void OnlineModule::activate()
{
#if !defined(PLASMA_NO_LICENSING)
    bool activated = m_workspace->m_activated;
    if (activated) {
        m_licenseMode = LICENSE_OFFLINE;
        std::ofstream file(PLASMA_FILE_PATH(getCommonAppDataPath() + L"\\config"));
        file << getLicenseSerial();
        file << " " << m_licenseMode;
        file.close();
        m_workspace->startDeactivation();
    } else {
        m_licenseMode = LICENSE_ACTIVATE;
        std::ofstream file(PLASMA_FILE_PATH(getCommonAppDataPath() + L"\\config"));
        file << getLicenseSerial();
        file << " " << m_licenseMode;
        file.close();
        m_workspace->startActivation();
    }
#endif
}

void OnlineModule::close()
{
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, 0);
}

void OnlineModule::editSerial()
{
    m_serial = narrowString(getPanel()->getNode()->getText(L"online:serial:1", true) +
                            getPanel()->getNode()->getText(L"online:serial:2", true) +
                            getPanel()->getNode()->getText(L"online:serial:3", true) +
                            getPanel()->getNode()->getText(L"online:serial:4", true));
}

void OnlineModule::editSerial1()
{
    plasma::Edit *edit;
    {
        std::wstring name(L"online:serial:1:edit");
        plasma::Widget *widget = getPanel()->getNode()->findWidget(name);
        edit = widget && widget->m_kind == plasma::WIDGET_EDIT ? static_cast<plasma::Edit *>(widget) : 0;
    }
    if (edit) {
        std::wstring text = edit->getText();
        std::transform(text.begin(), text.end(), text.begin(), toupper);
        edit->getNode()->setText(text);
        if (edit->getText().size() > SerialSegmentLength) {
            edit->getNode()->setText(edit->getText().substr(0, SerialSegmentLength));
            edit->clampSelection();
        }
        if (edit->m_caret == 4) {
            plasma::Edit *next;
            {
                const std::wstring name(L"online:serial:2:edit");
                plasma::Widget *widget = getPanel()->getNode()->findWidget(name);
                next = widget && widget->m_kind == plasma::WIDGET_EDIT ? static_cast<plasma::Edit *>(widget) : 0;
            }
            if (!next)
                return;
            m_workspace->m_engine.setFocusWidget(next);
            next->selectAll();
        }
    }
}

void OnlineModule::editSerial2()
{
    plasma::Edit *edit;
    {
        std::wstring name(L"online:serial:2:edit");
        plasma::Widget *widget = getPanel()->getNode()->findWidget(name);
        edit = widget && widget->m_kind == plasma::WIDGET_EDIT ? static_cast<plasma::Edit *>(widget) : 0;
    }
    if (edit) {
        std::wstring text = edit->getText();
        std::transform(text.begin(), text.end(), text.begin(), toupper);
        edit->getNode()->setText(text);
        if (edit->getText().size() > SerialSegmentLength) {
            edit->getNode()->setText(edit->getText().substr(0, SerialSegmentLength));
            edit->clampSelection();
        }
        if (edit->m_caret == 4) {
            plasma::Edit *next;
            {
                const std::wstring name(L"online:serial:3:edit");
                plasma::Widget *widget = getPanel()->getNode()->findWidget(name);
                next = widget && widget->m_kind == plasma::WIDGET_EDIT ? static_cast<plasma::Edit *>(widget) : 0;
            }
            if (!next)
                return;
            m_workspace->m_engine.setFocusWidget(next);
            next->selectAll();
        }
    }
}

void OnlineModule::editSerial3()
{
    plasma::Edit *edit;
    {
        std::wstring name(L"online:serial:3:edit");
        plasma::Widget *widget = getPanel()->getNode()->findWidget(name);
        edit = widget && widget->m_kind == plasma::WIDGET_EDIT ? static_cast<plasma::Edit *>(widget) : 0;
    }
    if (edit) {
        std::wstring text = edit->getText();
        std::transform(text.begin(), text.end(), text.begin(), toupper);
        edit->getNode()->setText(text);
        if (edit->getText().size() > SerialSegmentLength) {
            edit->getNode()->setText(edit->getText().substr(0, SerialSegmentLength));
            edit->clampSelection();
        }
        if (edit->m_caret == 4) {
            plasma::Edit *next;
            {
                const std::wstring name(L"online:serial:4:edit");
                plasma::Widget *widget = getPanel()->getNode()->findWidget(name);
                next = widget && widget->m_kind == plasma::WIDGET_EDIT ? static_cast<plasma::Edit *>(widget) : 0;
            }
            if (!next)
                return;
            m_workspace->m_engine.setFocusWidget(next);
            next->selectAll();
        }
    }
}

void OnlineModule::editSerial4()
{
    plasma::Edit *edit;
    {
        std::wstring name(L"online:serial:4:edit");
        plasma::Widget *widget = getPanel()->getNode()->findWidget(name);
        edit = widget && widget->m_kind == plasma::WIDGET_EDIT ? static_cast<plasma::Edit *>(widget) : 0;
    }
    if (edit) {
        std::wstring text = edit->getText();
        std::transform(text.begin(), text.end(), text.begin(), toupper);
        edit->getNode()->setText(text);
        if (edit->getText().size() > SerialSegmentLength) {
            edit->getNode()->setText(edit->getText().substr(0, SerialSegmentLength));
            edit->clampSelection();
        }
    }
}

std::string OnlineModule::getLicenseSerial()
{
    return m_serial;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void OnlineModule::chooseSerialVisible()
{
    bool visible = m_panel->isChecked(L"online:serial:visible", true);
    plasma::Widget::setDisplayVisibility(getPanel()->getNode()->findNode(L"online:serial:1")->m_display,
                                         visible ? 1 : 0);
    plasma::Widget::setDisplayVisibility(getPanel()->getNode()->findNode(L"online:serial:2")->m_display,
                                         visible ? 1 : 0);
    plasma::Widget::setDisplayVisibility(getPanel()->getNode()->findNode(L"online:serial:3")->m_display,
                                         visible ? 1 : 0);
    plasma::Widget::setDisplayVisibility(getPanel()->getNode()->findNode(L"online:serial:4")->m_display,
                                         visible ? 1 : 0);
    plasma::Widget::setDisplayVisibility(getPanel()->getNode()->findNode(L"online:serial:paste:block")->m_display,
                                         visible ? 1 : 0);
    if (visible) {
        {
            std::wstring name(L"online:serial:visible:block");
            plasma::Widget *widget = getPanel()->getNode()->findNode(name)->m_widget;
            widget->resizeTo(30.0f, 25.0f);
        }
        {
            std::wstring name(L"online:serial:visible");
            plasma::Widget *widget = getPanel()->getNode()->findNode(name)->m_widget;
            widget->resizeTo(30.0f, 25.0f);
        }
    } else {
        {
            std::wstring name(L"online:serial:visible:block");
            plasma::Widget *widget = getPanel()->getNode()->findNode(name)->m_widget;
            widget->resizeTo(226.0f, 25.0f);
        }
        {
            std::wstring name(L"online:serial:visible");
            plasma::Widget *widget = getPanel()->getNode()->findNode(name)->m_widget;
            widget->resizeTo(226.0f, 25.0f);
        }
    }
}

float roundToTenths(float value)
{
    if (value < 0.0f) {
        value = -roundToTenths(-value);
        if (value == 0.0f)
            value = 0.0f;
        return value;
    }

    // The float-to-int conversion has no range check.
    return (float)((double)(int)((double)value * 10.0 + 0.5) * (double)0.1f);
}
