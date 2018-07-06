// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/enums.h>
#include <primitives/filesystem.h>
#include <primitives/templates.h>

namespace primitives
{

enum class SettingsType
{
    Local,  // in current dir
    User,   // in user's home
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

//String unescapeSettingChar(const String &s);

String escapeSettingPart(const String &s);

std::string cp2utf8(int cp);

} // detail

struct PRIMITIVES_SETTINGS_API SettingPath
{
    using Parts = Strings;

    SettingPath();
    SettingPath(const String &s);

    String toString() const;

    const String &operator[](int i) const { return parts[i]; }

private:
    Parts parts;

    static bool isBasic(const String &part);
    void parse(const String &s);
};

struct PRIMITIVES_SETTINGS_API Setting
{

};

struct PRIMITIVES_SETTINGS_API Settings
{
    std::map<String, String> settings;

    void load(const path &fn, SettingsType type);
    void save(const path &fn);
};

struct SettingStorageBase
{
    path configRoot;
    path configFilename;
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

                auto fn = configFilename;
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
            break;
        case SettingsType::System:
        {
            RUN_ONCE
            {
                auto fn = configRoot;
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
