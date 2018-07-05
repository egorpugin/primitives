// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/settings.h>

#include <path_parser.h>
#include <settings_parser.h>

namespace primitives
{

namespace detail
{

String escapeSettingPart(const String &s)
{
    return s;
}

}

void Settings::load(const path &fn, SettingsType type)
{

}

void Settings::save(const path &fn)
{

}

SettingPath parseSettingPath(const String &s)
{
    PathParserDriver d;
    d.bb.error_msg = "";
    /*auto r = d.parse(s);
    auto error = d.bb.error_msg;
    /*if (r == 0)
    {
        if (auto res = d.bb.getResult<VersionRange>(); res)
        {
            vr = res.value();
            return {};
        }
        else
            error = "parser error: empty result";
    }*/
    return {};
}

primitives::SettingStorage<primitives::Settings> &getSettings()
{
    static primitives::SettingStorage<primitives::Settings> settings;
    return settings;
}

}
