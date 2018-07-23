// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Some parts of this code are taken from LLVM.Support.CommandLine.

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

////////////////////////////////////////////////////////////////////////////////

template <class T>
struct SettingStorage : public ::primitives::SettingStorage<T>
{
    using base = ::primitives::SettingStorage<T>;

    ~SettingStorage()
    {
        base::getUserSettings().save(userConfigFilename);
    }
};

PRIMITIVES_SW_SETTINGS_API
primitives::SettingStorage<primitives::Settings> &getSettings();

PRIMITIVES_SW_SETTINGS_API
primitives::Settings &getSettings(SettingsType type);

////////////////////////////////////////////////////////////////////////////////

namespace settings
{

// tags

// if data type is changed
struct discard_malformed_on_load {};

// if data is critical
struct do_not_save {};

struct PRIMITIVES_SW_SETTINGS_API BaseSetting
{
    ::primitives::Setting *s = nullptr;

    void setSettingPath(const String &name) { s = &::sw::getSettings(SettingsType::Default)[name]; }
    void setNonSaveable() { /*s->saveable = false;*/ }
};

////////////////////////////////////////////////////////////////////////////////

template <class DataType>
struct setting_storage : BaseSetting
{
    template <class T> void setValue(const T &V, bool initial = false) {
        *S = V;
        if (initial)
            Default = V;
    }

    DataType &getValue() {
        return *S;
    }
    const DataType &getValue() const {
        return *S;
    }

    operator DataType() const { return this->getValue(); }
};

////////////////////////////////////////////////////////////////////////////////

namespace detail
{

// applicator class - This class is used because we must use partial
// specialization to handle literal string arguments specially (const char* does
// not correctly respond to the apply method).  Because the syntax to use this
// is a pain, we have the 'apply' method below to handle the nastiness...
//
template <class Mod> struct applicator {
    template <class Opt> static void opt(const Mod &M, Opt &O) { M.apply(O); }
};

// Handle const char* as a special case...
template <unsigned n> struct applicator<char[n]> {
    template <class Opt> static void opt(const String &Str, Opt &O) {
        O.setSettingPath(Str);
    }
};
template <unsigned n> struct applicator<const char[n]> {
    template <class Opt> static void opt(const String &Str, Opt &O) {
        O.setSettingPath(Str);
    }
};
template <> struct applicator<String> {
    template <class Opt> static void opt(const String &Str, Opt &O) {
        O.setSettingPath(Str);
    }
};

template <> struct applicator<do_not_save> {
    static void opt(const do_not_save &, settings::BaseSetting &O) { O.setNonSaveable(); }
};

template <class Opt, class Mod> void apply(Opt *O, const Mod &M) {
    applicator<Mod>::opt(M, *O);
}

template <class Opt, class Mod, class... Mods>
void apply(Opt *O, const Mod &M, const Mods &... Ms) {
    applicator<Mod>::opt(M, *O);
    apply(O, Ms...);
}

} // namespace detail

////////////////////////////////////////////////////////////////////////////////

template <class T>
struct setting : setting_storage<T>
{
    template <class... Mods>
    explicit setting(const Mods &... Ms)
    {
        detail::apply(this, Ms...);
    }
};

} // namespace settings

////////////////////////////////////////////////////////////////////////////////

} // namespace sw

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
