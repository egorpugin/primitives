// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Some parts of this code are taken from LLVM.Support.CommandLine.

#pragma once

#include "settings_program_name.h"

#include <primitives/settings.h>
#include <primitives/CommandLine.h>

namespace cl
{

using namespace ::primitives::cl;

using ::primitives::cl::ParseCommandLineOptions;

inline bool ParseCommandLineOptions(const Strings &args,
    llvm::StringRef Overview = "",
    llvm::raw_ostream *Errs = nullptr)
{
    std::vector<const char *> argv;
    for (auto &a : args)
        argv.push_back(a.data());
    return ParseCommandLineOptions(argv.size(), argv.data(), Overview, Errs);
}

}

namespace sw
{

using primitives::SettingsType;

template <class T>
struct SettingStorage : public ::primitives::SettingStorage<T>
{
    using base = ::primitives::SettingStorage<T>;

    ~SettingStorage();
};

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4661)
PRIMITIVES_SETTINGS_API_EXTERN
template struct PRIMITIVES_SW_SETTINGS_API SettingStorage<::primitives::Settings>;
#pragma warning(pop)
#elif defined(__APPLE__)
extern
template struct PRIMITIVES_SW_SETTINGS_API SettingStorage<::primitives::Settings>;
#else
template struct SettingStorage<::primitives::Settings>;
#endif

PRIMITIVES_SW_SETTINGS_API
primitives::SettingStorage<primitives::Settings> &getSettingStorage();

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
        base::setSettings(::sw::getSettingStorage().getLocalSettings());
        base::init(Ms...);
    }

    using base::operator=;
};

} // namespace sw

// consider removal?
/*inline primitives::Settings &getSettings(primitives::SettingsType type)
{
    auto &s = ::sw::getSettings(type);
    // TODO: optimize? return slices?
    // cache slices like
    // static std::map<const char *PACKAGE_NAME_CLEAN, SettingsSlice> slices;
    s.prefix = PACKAGE_NAME_CLEAN;
    return s;
}*/
