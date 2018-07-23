#pragma once

#include "primitives/settings.h"

#ifdef CPPAN_EXECUTABLE
#ifdef _WIN32
extern "C" __declspec(dllexport)
#else
// visibility default
#endif
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4190)
#endif
String getProgramName();
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace sw
{

PRIMITIVES_SW_SETTINGS_API
primitives::SettingStorage<primitives::Settings> &getSettings();

PRIMITIVES_SW_SETTINGS_API
primitives::Settings &getSettings(SettingsType type);

}

inline primitives::SettingStorage<primitives::Settings> &getSettings()
{
    return sw::getSettings();
}

inline primitives::Settings &getSettings(SettingsType type)
{
    auto &s = sw::getSettings(type);
    // TODO: optimize? return slices?
    // cache slices like
    // static std::map<const char *PACKAGE_NAME_CLEAN, SettingsSlice> slices;
    s.prefix = PACKAGE_NAME_CLEAN;
    return s;
}
