#include "ColorManagementModule.h"
#include "SpecialColorModule.h"
#include "main.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "plasma/ListWidget.h"
#include "plasma/TextShape.h"
#include <windows.h>
#include <algorithm>
#include <sstream>

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
ColorManagementModule::ColorManagementModule()
{
    m_profile = 0;
    m_exportTransform = 0;
    m_softProofTransform = 0;
    m_rgbToCmykTransform = 0;
    m_exportCmyk = false;
    m_enabled = false;
    {
        std::wstring name(L"module:color:management");
        std::wstring kind(L"bar");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 230.0f, 200.0f, g_workspace.m_guiRightNode,
                                                           kind);
    }
    plasma::Widget::setDisplayVisibility(m_panel->m_node->m_display, plasma::DISPLAY_INVISIBLE);
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_intent = INTENT_RELATIVE_COLORIMETRIC;
    m_srgbProfile = cmsCreate_sRGBProfile();
}

ColorManagementModule::~ColorManagementModule()
{
    if (m_profile)
        cmsCloseProfile(m_profile);
    if (m_softProofTransform)
        cmsDeleteTransform(m_softProofTransform);
    if (m_exportTransform)
        cmsDeleteTransform(m_exportTransform);
    if (m_rgbToCmykTransform)
        cmsDeleteTransform(m_rgbToCmykTransform);
    cmsCloseProfile(m_srgbProfile);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void ColorManagementModule::build()
{
    m_cursor.m_elements[1] = 10;
    std::list<ModuleWidgetItem> items;
    addComboButton(L"CMYK Profile", L"color:profile", L"color:profile:listbox", 200, 30, 300, 200, 0);

    // The profile name may be long: wrap the button's caption.
    plasma::Node *caption = getPanel()->getNode()->findNode(L"color:profile")->findNode(L"caption");
    if (caption && caption->m_shape && caption->m_shape->getType() == plasma::SHAPE_TEXT) {
        plasma::TextShape *text = static_cast<plasma::TextShape *>(caption->m_shape);
        text->m_wrapWidth = 140.0f;
        text->m_flags = text->m_flags & ~(plasma::TEXT_VERTICAL_MASK | (1 << plasma::TEXT_WRAP)) |
                        ((1 << plasma::TEXT_MIDDLE) | (1 << plasma::TEXT_WRAP));
        text->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
    plasma::ListWidget *profileList =
        dynamic_cast<plasma::ListWidget *>(getPanel()->getNode()->findWidget(L"color:profile:listbox:list"));
    if (profileList) {
        std::list<plasma::ListItem> profiles;
        WIN32_FIND_DATAW found;
        HANDLE search = FindFirstFileW((widenString(getColorDirectory()) + L"\\*").c_str(), &found);
        std::list<std::wstring> names;
        if (search != INVALID_HANDLE_VALUE) {
            do {
                if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::wstring file(found.cFileName);
                    std::wstring extension = file.substr(file.find_last_of(L'.') + 1);
                    std::transform(extension.begin(), extension.end(), extension.begin(), toupper);
                    if (extension == L"ICC" || extension == L"ICM") {
                        std::string path = getColorDirectory() + "\\" + narrowString(file);
                        cmsHPROFILE profile = cmsOpenProfileFromFile(path.c_str(), "r");
                        if (profile) {
                            cmsHTRANSFORM probe = cmsCreateTransform(m_srgbProfile, TYPE_RGB_FLT, profile, TYPE_CMYK_8,
                                                                     INTENT_PERCEPTUAL, 0);
                            if (probe) {
                                cmsDeleteTransform(probe);

                                // The code leaves this uninitialized when the profile has no description.
                                wchar_t description[ProfileDescriptionWideCapacity];
                                getProfileDescription(profile, description);
                                if (std::wstring(description) == L"U.S. Web Coated (SWOP) v2") {
                                    m_profile = profile;
                                    m_profileName = widenString(path);
                                    getPanel()->getNode()->findWidget(L"color:profile")->setCaption(description, true);
                                } else {
                                    cmsCloseProfile(profile);
                                }
                                profiles.push_back(
                                    plasma::ListItem(description, widenString(path), L"listitem", false));
                                names.push_back(widenString(path));
                            }
                        }
                    }
                }
            } while (FindNextFileW(search, &found));
            FindClose(search);
        }
        profileList->setItems(profiles);
        for (std::list<std::wstring>::iterator it = names.begin(); it != names.end(); ++it)
            getPanel()->getNode()->connectWidget(*it, plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                                 &ColorManagementModule::chooseProfile, true);
        profileList->getNode()->setChecked(m_profileName, true, false, true);
    }
    newRow(2);
    addComboButton(L"Intent: Relative Colorimetric", L"color:management:intent", L"color:management:intent:listbox",
                   200, 25, 200, 90, 0);
    plasma::ListWidget *intentList =
        dynamic_cast<plasma::ListWidget *>(getPanel()->getNode()->findWidget(L"color:management:intent:listbox:list"));
    std::list<plasma::ListItem> intents;
    intents.push_back(
        plasma::ListItem(L"Intent: Perceptual", L"color:management:intent:perceptual", L"listitem", false));
    intents.push_back(
        plasma::ListItem(L"Intent: Saturation", L"color:management:intent:saturation", L"listitem", false));
    intents.push_back(plasma::ListItem(L"Intent: Relative Colorimetric",
                                       L"color:management:intent:relative:colorimetric", L"listitem", false));
    intents.push_back(plasma::ListItem(L"Intent: Absolute Colorimetric",
                                       L"color:management:intent:absolute:colorimetric", L"listitem", false));
    intentList->setItems(intents);
    intentList->getNode()->setChecked(L"color:management:intent:relative:colorimetric", true, false, true);
    intentList->getNode()->connectWidget(L"color:management:intent:perceptual", plasma::WIDGET_EVENT_POINTER_PRESSED,
                                         this, &ColorManagementModule::choosePerceptual, true);
    intentList->getNode()->connectWidget(L"color:management:intent:saturation", plasma::WIDGET_EVENT_POINTER_PRESSED,
                                         this, &ColorManagementModule::chooseSaturation, true);
    intentList->getNode()->connectWidget(L"color:management:intent:relative:colorimetric",
                                         plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorManagementModule::chooseRelativeColorimetric, true);
    intentList->getNode()->connectWidget(L"color:management:intent:absolute:colorimetric",
                                         plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorManagementModule::chooseAbsoluteColorimetric, true);
    newRow(2);
    items.clear();
    items.push_back(
        ModuleWidgetItem(L"Export CMYK (JPEG, PDF)", L"color:management:export:cmyk", L"checkbutton2", 0, 0, 200, 25));
    addPanel(items, L"", 0);
    fitHeight(10);
    getPanel()->getNode()->connectWidget(L"color:management:export:cmyk", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorManagementModule::exportCmyk, true);
    getPanel()->getNode()->connectWidget(L"color:management:export:cmyk", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &ColorManagementModule::exportCmyk, true);
}

void ColorManagementModule::refreshColor()
{
    ColorModule *color = m_workspace->m_colorModule;
    color->setColor(color->getColor(), false, false, false);
}

EditorColor ColorManagementModule::toCmyk(const EditorColor &color)
{
    if (m_profile && !m_rgbToCmykTransform)
        m_rgbToCmykTransform = cmsCreateTransform(m_srgbProfile, TYPE_RGB_FLT, m_profile, TYPE_CMYK_FLT, m_intent,
                                                  cmsFLAGS_BLACKPOINTCOMPENSATION);
    if (m_profile && m_rgbToCmykTransform) {
        EditorColor result;
        cmsDoTransform(m_rgbToCmykTransform, &color, &result, 1);
        return result;
    }
    return EditorColor(0.0f, 0.0f, 0.0f, 0.0f);
}

plasma::Vector<3, float> ColorManagementModule::toRgb(const EditorColor &color)
{
    if (m_profile) {
        cmsHTRANSFORM transform = cmsCreateTransform(m_profile, TYPE_CMYK_FLT, m_srgbProfile, TYPE_RGB_FLT, m_intent,
                                                     cmsFLAGS_BLACKPOINTCOMPENSATION);
        if (transform) {
            plasma::Vector<3, float> result;
            cmsDoTransform(transform, &color, &result, 1);
            cmsDeleteTransform(transform);
            return result;
        }
    }
    return plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
}

void ColorManagementModule::chooseProfile(plasma::Widget *sender)
{
    if (m_softProofTransform) {
        cmsDeleteTransform(m_softProofTransform);
        m_softProofTransform = 0;
    }
    if (m_exportTransform) {
        cmsDeleteTransform(m_exportTransform);
        m_exportTransform = 0;
    }
    if (m_rgbToCmykTransform) {
        cmsDeleteTransform(m_rgbToCmykTransform);
        m_rgbToCmykTransform = 0;
    }
    if (m_profile) {
        cmsCloseProfile(m_profile);
        m_profile = 0;
    }
    m_profile = cmsOpenProfileFromFile(narrowString(sender->getName()).c_str(), "r");
    m_workspace->markSheetsChanged();
    refreshColor();
}

void ColorManagementModule::choosePerceptual()
{
    m_intent = INTENT_PERCEPTUAL;
    if (m_softProofTransform) {
        cmsDeleteTransform(m_softProofTransform);
        m_softProofTransform = 0;
    }
    if (m_exportTransform) {
        cmsDeleteTransform(m_exportTransform);
        m_exportTransform = 0;
    }
    if (m_rgbToCmykTransform) {
        cmsDeleteTransform(m_rgbToCmykTransform);
        m_rgbToCmykTransform = 0;
    }
    m_workspace->markSheetsChanged();
    refreshColor();
}

void ColorManagementModule::chooseSaturation()
{
    m_intent = INTENT_SATURATION;
    if (m_softProofTransform) {
        cmsDeleteTransform(m_softProofTransform);
        m_softProofTransform = 0;
    }
    if (m_exportTransform) {
        cmsDeleteTransform(m_exportTransform);
        m_exportTransform = 0;
    }
    if (m_rgbToCmykTransform) {
        cmsDeleteTransform(m_rgbToCmykTransform);
        m_rgbToCmykTransform = 0;
    }
    m_workspace->markSheetsChanged();
    refreshColor();
}

void ColorManagementModule::chooseRelativeColorimetric()
{
    m_intent = INTENT_RELATIVE_COLORIMETRIC;
    if (m_softProofTransform) {
        cmsDeleteTransform(m_softProofTransform);
        m_softProofTransform = 0;
    }
    if (m_exportTransform) {
        cmsDeleteTransform(m_exportTransform);
        m_exportTransform = 0;
    }
    if (m_rgbToCmykTransform) {
        cmsDeleteTransform(m_rgbToCmykTransform);
        m_rgbToCmykTransform = 0;
    }
    m_workspace->markSheetsChanged();
    refreshColor();
}

void ColorManagementModule::chooseAbsoluteColorimetric()
{
    m_intent = INTENT_ABSOLUTE_COLORIMETRIC;
    if (m_softProofTransform) {
        cmsDeleteTransform(m_softProofTransform);
        m_softProofTransform = 0;
    }
    if (m_exportTransform) {
        cmsDeleteTransform(m_exportTransform);
        m_exportTransform = 0;
    }
    if (m_rgbToCmykTransform) {
        cmsDeleteTransform(m_rgbToCmykTransform);
        m_rgbToCmykTransform = 0;
    }
    m_workspace->markSheetsChanged();
    refreshColor();
}

void ColorManagementModule::softProof(void *bgraPixels, unsigned count)
{
    if (!m_softProofTransform)
        m_softProofTransform = cmsCreateProofingTransform(m_srgbProfile, TYPE_BGRA_8, m_srgbProfile, TYPE_BGRA_8,
                                                          m_profile, m_intent, INTENT_RELATIVE_COLORIMETRIC,
                                                          cmsFLAGS_SOFTPROOFING | cmsFLAGS_BLACKPOINTCOMPENSATION);
    if (m_softProofTransform)
        cmsDoTransform(m_softProofTransform, bgraPixels, bgraPixels, count);
}

void ColorManagementModule::exportCmyk()
{
    m_exportCmyk = m_panel->isChecked(L"color:management:export:cmyk", true);
}

ModuleWidgetItem::~ModuleWidgetItem() {}

ModuleWidgetItem::ModuleWidgetItem(const ModuleWidgetItem &other)
    : m_caption(other.m_caption),
      m_name(other.m_name),
      m_shape(other.m_shape),
      m_left(other.m_left),
      m_top(other.m_top),
      m_width(other.m_width),
      m_height(other.m_height)
{}

float roundToHundredths(float value)
{
    if (value < 0.0f) {
        value = -roundToHundredths(-value);
        if (value == 0.0f)
            value = 0.0f; // No negative zero.
        return value;
    }
    value = (int)(value * 100.0 + 0.5) * (double)0.01f;
    return value;
}

std::string narrowString(const std::wstring &text)
{
    return std::string(text.begin(), text.end());
}

template void plasma::Node::connectWidget<ColorManagementModule>(const std::wstring &, int, ColorManagementModule *,
                                                                 void (ColorManagementModule::*)(), bool);

template void plasma::Node::connectWidget<ColorManagementModule>(const std::wstring &, int, ColorManagementModule *,
                                                                 void (ColorManagementModule::*)(plasma::Widget *),
                                                                 bool);

namespace plasma {
template<>
void Widget::setConnection<ColorManagementModule>(Connection *&connection, ColorManagementModule *object,
                                                  void (ColorManagementModule::*method)(), bool enabledOnly)
{
    if (connection)
        delete connection;
    connection = new MemberFunctionConnection<ColorManagementModule>(object, method, enabledOnly);
}
}

template void plasma::Widget::setConnection<ColorManagementModule>(plasma::Widget::Connection *&,
                                                                   ColorManagementModule *,
                                                                   void (ColorManagementModule::*)(plasma::Widget *),
                                                                   bool);

template void plasma::Widget::connect<ColorManagementModule>(int, ColorManagementModule *,
                                                             void (ColorManagementModule::*)(), bool, bool);

template void plasma::Widget::connect<ColorManagementModule>(int, ColorManagementModule *,
                                                             void (ColorManagementModule::*)(plasma::Widget *), bool,
                                                             bool);
