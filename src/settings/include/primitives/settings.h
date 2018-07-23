// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Some parts of this code are taken from LLVM.Support.CommandLine.

#pragma once

#include <primitives/enums.h>
#include <primitives/filesystem.h>
#include <primitives/stdcompat/variant.h>
#include <primitives/templates.h>
#include <primitives/yaml.h>

#include <boost/variant.hpp>

namespace primitives
{

struct Setting;

enum class SettingsType
{
    // File or Document? // per document settings!
    // Project
    // Other levels below? Or third one here

    Local,  // in current dir
    User,   // in user's home
    // Group?
    System, // in OS

    Max,

    Default = User
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
        O.setArgStr(Str);
    }
};
template <unsigned n> struct applicator<const char[n]> {
    template <class Opt> static void opt(const String &Str, Opt &O) {
        O.setArgStr(Str);
    }
};
template <> struct applicator<String> {
    template <class Opt> static void opt(const String &Str, Opt &O) {
        O.setArgStr(Str);
    }
};

template <class Opt, class Mod, class... Mods>
void apply(Opt *O, const Mod &M, const Mods &... Ms) {
    applicator<Mod>::opt(M, *O);
    if constexpr (sizeof...(Ms) > 0)
        apply(O, Ms...);
}

} // namespace detail

////////////////////////////////////////////////////////////////////////////////

namespace cl
{

struct desc
{
    String description;
};

struct value_desc
{
    String value_description;
};

template <class T>
struct option
{
    String cmd_name;

    template <class... Mods>
    explicit option(const Mods &... Ms)
        //: Option(Optional, NotHidden), Parser(*this)
    {
        detail::apply(this, Ms...);
        //done();
    }

    void setArgStr(const String &s) { cmd_name = s; }
};

template <class T>
using opt = option<T>;

PRIMITIVES_SETTINGS_API
void parseCommandLineOptions(int argc, char **argv);

PRIMITIVES_SETTINGS_API
void parseCommandLineOptions(const Strings &args);

} // namespace cl

////////////////////////////////////////////////////////////////////////////////

namespace detail
{

char unescapeSettingChar(char c);
String escapeSettingPart(const String &s);
std::string cp2utf8(int cp);

} // namespace detail

////////////////////////////////////////////////////////////////////////////////

struct PRIMITIVES_SETTINGS_API SettingPath
{
    using Parts = Strings;

    SettingPath();
    SettingPath(const String &s);
    SettingPath(const SettingPath &s) = default;
    SettingPath &operator=(const String &s);
    SettingPath &operator=(const SettingPath &s) = default;

    String toString() const;

    const String &at(size_t i) const { return parts.at(i); }
    const String &operator[](size_t i) const { return parts[i]; }
    bool operator<(const SettingPath &rhs) const { return parts < rhs.parts; }
    bool operator==(const SettingPath &rhs) const { return parts == rhs.parts; }

    SettingPath operator/(const SettingPath &rhs) const;

    static SettingPath fromRawString(const String &s);

#ifndef HAVE_BISON_SETTINGS_PARSER
private:
#endif
    Parts parts;

    static bool isBasic(const String &part);
    void parse(const String &s);
};

struct not_converted : std::string {};

//using SettingBase = variant<std::string, int64_t, double, bool>;
//#define SettingGet std::get
using SettingBase = boost::make_recursive_variant<
    not_converted,
    std::string,
    //int, // ?
    int64_t,
    //float, // ?
    double,
    bool,
    std::vector<boost::recursive_variant_>>::type;
#define SettingGet boost::get

// rename to value?
struct Setting : SettingBase
{
    using base = SettingBase;

    // scope = local, user, thread
    // read only
    //bool saveable = true;

    using base::base;
    using base::operator=;

    Setting() = default;
    Setting(const Setting &) = default;
    Setting &operator=(const Setting &) = default;

    Setting(int v)
        : base((int64_t)v)
    {
    }

    Setting(const std::vector<Setting> &v)
        : base((const std::vector<SettingBase> &)v)
    {
    }

    Setting &operator=(int v)
    {
        base::operator=((int64_t)v);
        return *this;
    }

    Setting &operator=(const std::vector<Setting> &v)
    {
        base::operator=((const std::vector<SettingBase> &)v);
        return *this;
    }

    /*template <class U, typename std::enable_if_t<std::is_integral_v<U>> = 0>
    Setting &operator=(U v)
    {
        base::operator=((int64_t)v);
        return *this;
    }*/

    /*
    template <class U>
    Setting &operator=(const U &v)
    {
        if constexpr (std::is_integral_v<U>)
            base::operator=((int64_t)v);
        else if constexpr (std::is_floating_point_v<U>)
            base::operator=((double)v);
        else
            base::operator=(v);
        return *this;
    }*/

    bool operator==(const std::string &s) const
    {
        return SettingGet<std::string>(*this) == s;
    }

    template <class U>
    bool operator==(const U &v) const
    {
        if constexpr (std::is_same_v<U, bool>)
            return SettingGet<bool>(*this) == v;
        else if constexpr (
            std::is_same_v<U, not_converted> ||
            std::is_same_v<U, std::string> ||
            std::is_same_v<U, std::string_view> ||
            std::is_same_v<U, const char *> ||
            std::is_same_v<U, const char[]> ||
            std::is_same_v<U, char[]> ||
            std::is_array_v<U>
            )
            return SettingGet<std::string>(*this) == v;
        else if constexpr (std::is_integral_v<U>)
            return SettingGet<int64_t>(*this) == v;
        else if constexpr (std::is_floating_point_v<U>)
            return SettingGet<double>(*this) == v;
    }

    String toString() const;

    template <class U>
    U &as()
    {
        if (which() == 0)
        {
            auto s = SettingGet<not_converted>(*this);
            if constexpr (std::is_same_v<U, bool>)
                *this = s == "true";
            else if constexpr (std::is_same_v<U, std::string>)
                *this = s;
            else if constexpr (std::is_integral_v<U>)
                *this = std::stoll(s);
            else if constexpr (std::is_floating_point_v<U>)
                *this = std::stod(s);
        }
        return SettingGet<U>(*this);
    }

    template <class U>
    const U &as() const
    {
        if (which() == 0)
            return const_cast<Setting*>(this)->as<U>();
        return SettingGet<U>(*this);
    }
};
#undef SettingGet

struct PRIMITIVES_SETTINGS_API Settings
{
    using SettingsMap = std::map<SettingPath, Setting>;

    SettingPath prefix;

    Settings(const Settings &s) = default;
    Settings &operator=(const Settings &s) = default;

    void clear();

    // remove type parameter?
    void load(const path &fn, SettingsType type = SettingsType::Default);
    void load(const String &s, SettingsType type = SettingsType::Default);
    void load(const yaml &root, const SettingPath &prefix = {});

    // dump to object
    String dump() const;
    void dump(yaml &root) const;

    // save to file (default storage)
    void save(const path &fn = {}) const;
    // also from yaml, json, registry

    Setting &operator[](const String &k);
    const Setting &operator[](const String &k) const;
    Setting &operator[](const SettingPath &k);
    const Setting &operator[](const SettingPath &k) const;
    //void getSubSettings();

    SettingsMap::iterator begin() { return settings.begin(); }
    SettingsMap::iterator end() { return settings.end(); }

    SettingsMap::const_iterator begin() const { return settings.begin(); }
    SettingsMap::const_iterator end() const { return settings.end(); }

#ifndef HAVE_BISON_SETTINGS_PARSER
private:
#endif
    SettingsMap settings;

    Settings() = default;

    template <class T>
    friend struct SettingStorage;
};

////////////////////////////////////////////////////////////////////////////////

struct SettingStorageBase
{
    path systemConfigFilename;
    path userConfigFilename;
};

template <class T>
struct SettingStorage : SettingStorageBase
{
    T &getSystemSettings();
    T &getUserSettings();
    T &getLocalSettings();

    void clearLocalSettings();

    T &get(SettingsType type = SettingsType::Default);

    // We could add here more interfaces from Settings,
    // but instead we add getSettings(type) overload.

private:
    T settings[toIndex(SettingsType::Max)];
};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4661)
#endif

PRIMITIVES_SETTINGS_API_EXTERN
template struct PRIMITIVES_SETTINGS_API SettingStorage<primitives::Settings>;

#ifdef _MSC_VER
#pragma warning(pop)
#endif

////////////////////////////////////////////////////////////////////////////////

/// Users may want to define their own getSettings()
/// with some initial setup.
/// 'settings_auto' package will also provide tuned global getSettings() to use.
PRIMITIVES_SETTINGS_API
primitives::SettingStorage<primitives::Settings> &getSettings(primitives::SettingStorage<primitives::Settings> * = nullptr);

PRIMITIVES_SETTINGS_API
primitives::Settings &getSettings(SettingsType type, primitives::SettingStorage<primitives::Settings> * = nullptr);

////////////////////////////////////////////////////////////////////////////////

} // namespace primitives

using primitives::SettingsType;

// request by boost
inline std::ostream &operator<<(std::ostream &o, const primitives::SettingBase &)
{
    return o;
}
