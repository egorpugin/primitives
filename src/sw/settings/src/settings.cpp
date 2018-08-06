// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "primitives/sw/settings.h"

#define YAML_SETTINGS_FILENAME "settings.yml"

#if defined(_WIN32) && defined(CPPAN_SHARED_BUILD)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
static String getProgramName()
{
    auto h = GetModuleHandle(0);
    auto f = (String(*)())GetProcAddress(h, "getProgramName");
    if (!f)
        return "unk";
    return f();
}
#endif

namespace sw
{

path getSettingsDir()
{
    return get_home_directory() / ".sw_settings" / getProgramName();
}

template <class T>
SettingStorage<T>::~SettingStorage()
{
    base::getUserSettings().save(base::userConfigFilename);
}

template struct SettingStorage<::primitives::Settings>;

primitives::SettingStorage<primitives::Settings> &getSettingStorage()
{
    static auto &settings = []() -> primitives::SettingStorage<primitives::Settings> &
    {
        static SettingStorage<primitives::Settings> s;
        primitives::getSettingStorage(&s); // save to common storage
        s.userConfigFilename = getSettingsDir() / YAML_SETTINGS_FILENAME;
        path local_name = getProgramName() + ".settings";
        if (fs::exists(local_name))
            s.getLocalSettings().load(local_name);
        return s;
    }();
    return settings;
}

primitives::Settings &getSettings(SettingsType type)
{
    return getSettingStorage().get(type);
}

}
