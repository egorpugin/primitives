// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/enums.h>
#include <primitives/filesystem.h>
#include <primitives/stdcompat/variant.h>
#include <primitives/templates.h>

#include <boost/variant.hpp>

namespace primitives
{

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
struct opt
{
};

}

namespace detail
{

char unescapeSettingChar(char c);
String escapeSettingPart(const String &s);
std::string cp2utf8(int cp);

} // detail

struct PRIMITIVES_SETTINGS_API SettingPath
{
    using Parts = Strings;

    SettingPath();
    SettingPath(const String &s);
    SettingPath(const SettingPath &s) = default;
    SettingPath &operator=(const SettingPath &s) = default;

    String toString() const;

    const String &at(size_t i) const { return parts.at(i); }
    const String &operator[](size_t i) const { return parts[i]; }
    bool operator<(const SettingPath &rhs) const { return parts < rhs.parts; }
    bool operator==(const SettingPath &rhs) const { return parts == rhs.parts; }

    SettingPath operator/(const SettingPath &rhs) const;

#ifndef HAVE_BISON_SETTINGS_PARSER
private:
#endif
    Parts parts;

    static bool isBasic(const String &part);
    void parse(const String &s);
};

//using SettingBase = variant<std::string, int64_t, double, bool>;
//#define SettingGet std::get
using SettingBase = boost::make_recursive_variant<
    std::string, int64_t, double, bool,
    std::vector<boost::recursive_variant_>>::type;
#define SettingGet boost::get

// rename to value?
struct Setting : SettingBase
{
    using base = SettingBase;

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

};

#undef SettingGet

struct PRIMITIVES_SETTINGS_API Settings
{
    using SettingsMap = std::map<SettingPath, Setting>;

    Settings(const Settings &s) = default;
    Settings &operator=(const Settings &s) = default;

    // remove type parameter?
    void load(const path &fn, SettingsType type = SettingsType::Default);
    void load(const String &s, SettingsType type = SettingsType::Default);
    void save(const path &fn) const;
    String save() const;

    Setting &operator[](const String &k);
    const Setting &operator[](const String &k) const;

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

struct SettingStorageBase
{
    path systemConfigFilename;
    path userConfigFilename;
};

template <class T>
struct SettingStorage : SettingStorageBase
{
    T &getSystemSettings()
    {
        return get(SettingsType::System);
    }

    T &getUserSettings()
    {
        return get(SettingsType::User);
    }

    T &getLocalSettings()
    {
        return get(SettingsType::Local);
    }

    void clearLocalSettings()
    {
        getLocalSettings() = getUserSettings();
    }

private:
    T settings[toIndex(SettingsType::Max)];

    T &get(SettingsType type = SettingsType::Default)
    {
        if (type < SettingsType::Local || type >= SettingsType::Max)
            throw std::runtime_error("No such settings storage");

        auto &s = settings[toIndex(type)];
        switch (type)
        {
        case SettingsType::Local:
        {
            RUN_ONCE
            {
                s = get(SettingsType::User);
            }
        }
            break;
        case SettingsType::User:
        {
            RUN_ONCE
            {
                s = get(SettingsType::System);

                auto fn = userConfigFilename;
                if (!fn.empty())
                {
                    if (!fs::exists(fn))
                    {
                        error_code ec;
                        fs::create_directories(fn.parent_path(), ec);
                        if (ec)
                            throw std::runtime_error(ec.message());
                        auto ss = get(SettingsType::System);
                        ss.save(fn);
                    }
                    s.load(fn, SettingsType::User);
                }
            }
        }
            break;
        case SettingsType::System:
        {
            RUN_ONCE
            {
                auto fn = systemConfigFilename;
                if (fs::exists(fn))
                    s.load(fn, SettingsType::System);
            }
        }
            break;
        default:
            break;
        }
        return s;
    }
};

/// Users may want to define their own getSettings()
/// with some initial setup.
/// 'settings_auto' package will also provide tuned global getSettings() to use.
PRIMITIVES_SETTINGS_API
primitives::SettingStorage<primitives::Settings> &getSettings();

}

// request by boost
inline std::ostream &operator<<(std::ostream &o, const primitives::SettingBase &)
{
    return o;
}
