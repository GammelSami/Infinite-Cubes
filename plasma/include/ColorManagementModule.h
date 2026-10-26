// The CMYK profile and rendering-intent panel ("module:color:management").
#pragma once

// Allocation counts wchar_t elements; Little CMS takes a byte capacity.
enum {
    ProfileDescriptionWideCapacity = 1000,
    ProfileDescriptionByteCapacity = 1000
};

#include "ColorModule.h"
#include "../vendor/lcms2-2.0/include/lcms2.h"

class ColorManagementModule : public Module {
public:
    ColorManagementModule();
    virtual ~ColorManagementModule();
    virtual void build();
    void refreshColor();
    EditorColor toCmyk(const EditorColor &color);
    plasma::Vector<3, float> toRgb(const EditorColor &color);
    void chooseProfile(plasma::Widget *sender);
    void choosePerceptual();
    void chooseSaturation();

    // LCMS transforms count packed BGRA8 pixels in place.
    void softProof(void *bgraPixels, unsigned count);
    void chooseRelativeColorimetric();
    void chooseAbsoluteColorimetric();
    void exportCmyk();

    std::wstring m_profileName;
    cmsHPROFILE m_profile;
    cmsHPROFILE m_srgbProfile;
    cmsHTRANSFORM m_exportTransform;
    cmsHTRANSFORM m_softProofTransform;
    cmsHTRANSFORM m_rgbToCmykTransform;
    cmsUInt32Number m_intent;
    bool m_exportCmyk;
    bool m_enabled;
};

// The profile's description (cmsGetProfileInfo).
void getProfileDescription(cmsHPROFILE profile, wchar_t *description);
namespace plasma {
template<>
void Widget::setConnection<ColorManagementModule>(Connection *&connection, ColorManagementModule *object,
                                                  void (ColorManagementModule::*method)(), bool enabledOnly);
}

// The byte string of a wide one, character by character.
std::string narrowString(const std::wstring &text);
