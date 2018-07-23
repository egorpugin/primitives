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
    DataType Default;

    template <class T> void setValue(const T &V, bool initial = false) {
        *s = V;
        if (initial)
            Default = V;
    }

    void setInitialValue(const DataType &V) { this->setValue(V, true); }

    DataType &getValue() {
        return *s;
    }
    const DataType &getValue() const {
        return *s;
    }

    operator DataType() const { return this->getValue(); }
};

////////////////////////////////////////////////////////////////////////////////

// init - Specify a default (initial) value for the command line argument, if
// the default constructor for the argument type does not give you what you
// want.  This is only valid on "opt" arguments, not on "list" arguments.
//
template <class Ty> struct initializer {
    const Ty &Init;
    initializer(const Ty &Val) : Init(Val) {}

    template <class Opt> void apply(Opt &O) const { O.setInitialValue(Init); }
};

template <class Ty> initializer<Ty> init(const Ty &Val) {
    return initializer<Ty>(Val);
}

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

//

template <class Opt, class Mod, class... Mods>
void apply(Opt *O, const Mod &M, const Mods &... Ms) {
    applicator<Mod>::opt(M, *O);
    if constexpr (sizeof...(Ms) > 0)
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

    // Some options may take their value from a different data type.
    template <class DT> setting<T> &operator=(const DT &V) {
        this->setValue(V);
        return *this;
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
