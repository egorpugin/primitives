#include "primitives/sw/settings.h"

#define YAML_SETTINGS_FILENAME "settings.yml"

#ifdef _WIN32
#include <Windows.h>
static String getProgramName()
{
    auto h = GetModuleHandle(0);
    auto gpn = (String(*)())GetProcAddress(h, "getProgramName");
    if (!gpn)
        // Unknown program name: missing getProgramName() symbol.
        return "unk";
    return gpn();
}
#endif

namespace sw
{

primitives::SettingStorage<primitives::Settings> &getSettings()
{
    static auto &settings = []() -> decltype(auto)
    {
        auto &s = primitives::getSettings();
        s.userConfigFilename = get_home_directory() / ".sw_settings" / getProgramName() / YAML_SETTINGS_FILENAME;
        path local_name = getProgramName() + ".settings";
        if (fs::exists(local_name))
            s.getLocalSettings().load(local_name);
        return s;
    }();
    return settings;
}

primitives::Settings &getSettings(SettingsType type)
{
    return getSettings().get(type);
}

}
