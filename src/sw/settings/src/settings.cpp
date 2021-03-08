// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/sw/settings.h>

// this file provides definition to sw::getProgramName()
// do not remove
#include <primitives/sw/settings_program_name.h>

#define YAML_SETTINGS_FILENAME "settings.yml"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace sw
{

static std::pair<std::string, std::string> getProgramName1()
{
#if defined(_WIN32)
    auto h = GetModuleHandle(0);
    if (!h)
        return { {}, "sw.settings: GetModuleHandle(0) error: " + std::to_string(GetLastError()) };
    //auto f = (std::string(*)())GetProcAddress(h, "?getProgramName@sw@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ");
    auto f = (std::string(*)())GetProcAddress(h, "?getProgramName@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ");
    if (!f)
        return { {},"sw.settings: calling getProgramName(), but function is not defined; result is unknown" };
#else
    auto h = dlopen(0, RTLD_LAZY);
    if (!h)
        return { {},"sw.settings: dlopen(0) error: "s + dlerror() };
    SCOPE_EXIT{ dlclose(h); };
    //auto f = (std::string(*)())dlsym(h, "_ZN2sw14getProgramNameB5cxx11Ev");
    auto f = (std::string(*)())dlsym(h, "_Z14getProgramNameB5cxx11v");
    if (!f)
    {
        f = (std::string(*)())dlsym(h, "_Z14getProgramNamev");
        if (!f)
            return { {},"sw.settings: calling getProgramName(), but function is not defined (did you forget to add '-Wl,--export-dynamic' ?); result is unknown" };
    }
#endif
    auto n = f();
    if (n.empty())
        return { {},"empty program name" };
    return { n,{} };
}

std::string getProgramName()
{
    auto [name, err] = getProgramName1();
    if (!err.empty())
        throw SW_RUNTIME_ERROR(err);
    return name;
}

std::string getProgramNameSilent()
{
    auto [name, err] = getProgramName1();
    return name;
}

template <class T>
SettingStorage<T>::~SettingStorage()
{
    base::getUserSettings().save(base::userConfigFilename);
}

#if defined(_WIN32) || defined(__APPLE__)
template struct SettingStorage<::primitives::Settings>;
#endif

primitives::SettingStorage<primitives::Settings> &getSettingStorage()
{
    static auto &settings = []() -> primitives::SettingStorage<primitives::Settings> &
    {
        static SettingStorage<primitives::Settings> s;
        primitives::getSettingStorage(&s); // save to common storage
        auto prog_name = getProgramName();
        auto settings_dir = get_home_directory() / ".sw_settings" / prog_name;
        s.userConfigFilename = settings_dir / YAML_SETTINGS_FILENAME;
        auto sfile = prog_name + ".settings";
        path local_name = sfile;
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
