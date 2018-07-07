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

char unescapeSettingChar(char c)
{
    switch (c)
    {
#define M(f, t) case f: return t
        M('n', '\n');
        M('\"', '\"');
        M('\\', '\\');
        M('r', '\r');
        M('t', '\t');
        M('f', '\f');
        M('b', '\b');
#undef M
    }
    return 0;
}

String escapeSettingPart(const String &s)
{
    String s2;
    for (auto &c : s)
    {
        if (c >= 0 && c <= 127 && iscntrl(c))
        {
            static const auto hex = "0123456789ABCDEF";
            s2 += "\\u00";
            s2 += hex[(c & 0xF0) >> 4];
            s2 += hex[c & 0x0F];
        }
        else
            s2 += c;
    }
    return s2;
}

std::string cp2utf8(int cp)
{
    char c[5] = { 0x00,0x00,0x00,0x00,0x00 };
    if (cp <= 0x7F) { c[0] = cp; }
    else if (cp <= 0x7FF) { c[0] = (cp >> 6) + 192; c[1] = (cp & 63) + 128; }
    else if (0xd800 <= cp && cp <= 0xdfff) {} //invalid block of utf8
    else if (cp <= 0xFFFF) { c[0] = (cp >> 12) + 224; c[1] = ((cp >> 6) & 63) + 128; c[2] = (cp & 63) + 128; }
    else if (cp <= 0x10FFFF) { c[0] = (cp >> 18) + 240; c[1] = ((cp >> 12) & 63) + 128; c[2] = ((cp >> 6) & 63) + 128; c[3] = (cp & 63) + 128; }
    return std::string(c);
}

}

SettingPath::SettingPath()
{
}

SettingPath::SettingPath(const String &s)
{
    parse(s);
}

void SettingPath::parse(const String &s)
{
    if (s.empty())
    {
        parts.clear();
        return;
    }

    PathParserDriver d;
    auto r = d.parse(s);
    if (!d.bb.error_msg.empty())
        throw std::runtime_error(d.bb.error_msg);
    parts = d.sp;
}

bool SettingPath::isBasic(const String &part)
{
    for (auto &c : part)
    {
        if (!(c >= 0 && c <= 127 && (isalnum(c) || c == '_' || c == '-')))
            return false;
    }
    return true;
}

String SettingPath::toString() const
{
    String s;
    for (auto &p : parts)
    {
        if (isBasic(p))
            s += p;
        else
            s += "\"" + detail::escapeSettingPart(p) + "\"";
        s += ".";
    }
    if (!s.empty())
        s.resize(s.size() - 1);
    return s;
}

SettingPath SettingPath::operator/(const SettingPath &rhs) const
{
    SettingPath tmp(*this);
    tmp.parts.insert(tmp.parts.end(), rhs.parts.begin(), rhs.parts.end());
    return tmp;
}

void Settings::load(const path &fn, SettingsType type)
{
    auto f = primitives::filesystem::fopen_checked(fn);

    SettingsParserDriver d;
    auto r = d.parse(f);
    if (!d.bb.error_msg.empty())
        throw std::runtime_error(d.bb.error_msg);
    settings = d.sm;
}

void Settings::load(const String &s, SettingsType type)
{
    SettingsParserDriver d;
    auto r = d.parse(s);
    if (!d.bb.error_msg.empty())
        throw std::runtime_error(d.bb.error_msg);
    settings = d.sm;
}

void Settings::save(const path &fn) const
{
    write_file(fn, save());
}

String Settings::save() const
{
    String s;
    return s;
}

Setting &Settings::operator[](const String &k)
{
    return settings[k];
}

const Setting &Settings::operator[](const String &k) const
{
    auto i = settings.find(k);
    if (i == settings.end())
        throw std::runtime_error("No such setting: " + k);
    return i->second;
}

primitives::SettingStorage<primitives::Settings> &getSettings()
{
    static primitives::SettingStorage<primitives::Settings> settings;
    return settings;
}

}
