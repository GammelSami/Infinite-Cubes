// Colour-management support for loading a runtime profile path and reading its description through Little CMS.
// GetProfileDescription uses cmsReadTag and cmsMLUgetWide; the profile-info helper uses cmsGetProfileInfo.
#include "../vendor/lcms2-2.0/include/lcms2.h"
#include "ColorManagementModule.h"

static void listProfile(const char *path)
{
    cmsHPROFILE profile = cmsOpenProfileFromFile(path, "r");
    if (!profile)
        return;
    wchar_t description[ProfileDescriptionWideCapacity];
    cmsGetProfileInfo(profile, cmsInfoDescription, "en", "US", description, ProfileDescriptionByteCapacity);
    cmsCloseProfile(profile);
}

#pragma comment(linker, "/INCLUDE:_StandIn_ColorManagementProfiles")
extern "C" void StandIn_ColorManagementProfiles(const char *path)
{
    volatile double alignedFrame = 0;
    listProfile(path);
}

#include "ColorModule.h"
#include "SpecialColorModule.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "ShapeInterface.h"
#include <windows.h>
#include <process.h>
#include <cmath>

void getProfileDescription(cmsHPROFILE profile, wchar_t *description)
{
    // cmsGetProfileInfo reads the description and converts it to wide text through cmsMLUgetWide.
    const cmsMLU *info = static_cast<const cmsMLU *>(cmsReadTag(profile, cmsSigProfileDescriptionTag));
    if (info)
        cmsMLUgetWide(info, "en", "US", description, ProfileDescriptionByteCapacity);
}
