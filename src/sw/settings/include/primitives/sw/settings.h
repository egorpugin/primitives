// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Some parts of this code are taken from LLVM.Support.CommandLine.

#pragma once

#include "primitives/settings.h"

#if defined(CPPAN_EXECUTABLE) && defined(CPPAN_SHARED_BUILD)
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

////////////////////////////////////////////////////////////////////////////////

template <class T>
struct SettingStorage : public ::primitives::SettingStorage<T>
{
    using base = ::primitives::SettingStorage<T>;

    ~SettingStorage();
};

PRIMITIVES_SW_SETTINGS_API_EXTERN
template struct PRIMITIVES_SW_SETTINGS_API SettingStorage<::primitives::Settings>;

PRIMITIVES_SW_SETTINGS_API
primitives::SettingStorage<primitives::Settings> &getSettings();

PRIMITIVES_SW_SETTINGS_API
primitives::Settings &getSettings(SettingsType type);

using ::primitives::init;
using ::primitives::do_not_save;
using ::primitives::must_not_be;

template <class DataType>
struct setting : ::primitives::setting<DataType>
{
    using base = ::primitives::setting<DataType>;

    template <class... Mods>
    explicit setting(const Mods &... Ms)
        : base()
    {
        base::setSettings(sw::getSettings().getLocalSettings());
        base::init(Ms...);
    }
};

} // namespace sw

////////////////////////////////////////////////////////////////////////////////

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
