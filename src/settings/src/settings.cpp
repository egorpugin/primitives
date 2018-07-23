// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/settings.h>

#include <path_parser.h>
#include <settings_parser.h>

#include <primitives/overload.h>

#include <boost/algorithm/string.hpp>

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

namespace cl
{

using Args = std::list<std::string>;

void parseCommandLineOptions1(const String &prog, Args args)
{
    // TODO: convert args to utf8

    // expand response files and settings
    bool found;
    do
    {
        found = false;
        for (auto i = args.begin(); i != args.end(); i++)
        {
            auto &a = *i;
            if (a.size() > 1 && a[0] == '@' && a[1] != '@')
            {
                // response file
                auto f = read_file(a.substr(1));
                boost::replace_all(f, "\r", "");

                std::vector<String> lines;
                boost::split(lines, f, boost::is_any_of("\n"));
                // do we need trim?
                //for (auto &l : lines)
                    //boost::trim(l);

                for (auto &l : lines)
                    args.insert(i, l);
                i = args.erase(i);
                i--;
                found = true;
            }
        }
    }
    while (found);

    // read settings
    for (auto i = args.begin(); i != args.end(); i++)
    {
        auto &a = *i;
        if (a.size() > 1 && a[0] == '@' && a[1] == '@')
        {
            // settings file
            getSettings().getLocalSettings().load(a.substr(2));
            i = args.erase(i);
            i--;
        }
    }
}

void parseCommandLineOptions(int argc, char **argv)
{
    std::list<std::string> args;
    for (int i = 1; i < argc; i++)
        args.push_back(argv[i]);
    parseCommandLineOptions1(argv[0], args);
}

void parseCommandLineOptions(const Strings &args)
{
    parseCommandLineOptions1(*args.begin() , { args.begin() + 1, args.end() });
}

}

SettingPath::SettingPath()
{
}

SettingPath::SettingPath(const String &s)
{
    parse(s);
}

SettingPath &SettingPath::operator=(const String &s)
{
    return *this = SettingPath(s);
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
        throw std::runtime_error("While parsing setting path '" + s + "':" + d.bb.error_msg);
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

SettingPath SettingPath::fromRawString(const String &s)
{
    SettingPath p;
    p.parts.push_back(s);
    return p;
}

void Settings::clear()
{
    settings.clear();
}

void Settings::load(const path &fn, SettingsType type)
{
    if (fn.extension() == ".yml" || fn.extension() == ".yaml" || fn.extension() == ".settings")
    {
        load(YAML::LoadFile(fn.u8string()));
    }
    else
    {
        std::unique_ptr<FILE, decltype(&fclose)> f(primitives::filesystem::fopen_checked(fn), fclose);

        SettingsParserDriver d;
        auto r = d.parse(f.get());
        if (!d.bb.error_msg.empty())
            throw std::runtime_error(d.bb.error_msg);
        settings = d.sm;
    }
}

void Settings::load(const String &s, SettingsType type)
{
    SettingsParserDriver d;
    auto r = d.parse(s);
    if (!d.bb.error_msg.empty())
        throw std::runtime_error(d.bb.error_msg);
    settings = d.sm;
}

void Settings::load(const yaml &root, const SettingPath &prefix)
{
    /*if (root.IsNull() || root.IsScalar())
        return;*/
    if (!root.IsMap())
        return;
        //throw std::runtime_error("Config must be a map");

    for (auto kv : root)
    {
        if (kv.second.IsScalar())
            operator[](prefix / SettingPath::fromRawString(kv.first.as<String>())) = not_converted{ kv.second.as<String>() };
        else if (kv.second.IsMap())
            load(kv.second, SettingPath::fromRawString(kv.first.as<String>()));
        else
            throw std::runtime_error("sequences are not supported");
    }
}

String Setting::toString() const
{
    auto v = overload(
        [](const std::string &s) { return s; },
        //[](int s) { return std::to_string(s); },
        [](int64_t s) { return std::to_string(s); },
        //[](float s) { return std::to_string(s); },
        [](double s) { return std::to_string(s); },
        [](bool s) { return std::to_string(s); },
        [](const auto &) { return ""s; }
    );
    return boost::apply_visitor(v, *this);
}

void Settings::save(const path &fn) const
{
    if (!fn.empty())
    {
        if (fn.extension() == ".yml" || fn.extension() == ".yaml" || fn.extension() == ".settings")
        {
            yaml root;
            dump(root);
            write_file(fn, YAML::Dump(root));
        }
        else
        {
            write_file(fn, dump());
        }
    }
}

String Settings::dump() const
{
    String s;
    throw std::runtime_error("not implemented");
    return s;
}

void Settings::dump(yaml &root) const
{
    for (auto &[k, v] : settings)
    {
        //if (v.saveable)
            root[k.toString()] = v.toString();
    }
}

Setting &Settings::operator[](const String &k)
{
    return settings[prefix / k];
}

const Setting &Settings::operator[](const String &k) const
{
    auto i = settings.find(prefix / k);
    if (i == settings.end())
        throw std::runtime_error("No such setting: " + k);
    return i->second;
}

Setting &Settings::operator[](const SettingPath &k)
{
    return settings[prefix / k];
}

const Setting &Settings::operator[](const SettingPath &k) const
{
    auto i = settings.find(prefix / k);
    if (i == settings.end())
        throw std::runtime_error("No such setting: " + k.toString());
    return i->second;
}

template <class T>
T &SettingStorage<T>::getSystemSettings()
{
    return get(SettingsType::System);
}

template <class T>
T &SettingStorage<T>::getUserSettings()
{
    return get(SettingsType::User);
}

template <class T>
T &SettingStorage<T>::getLocalSettings()
{
    return get(SettingsType::Local);
}

template <class T>
void SettingStorage<T>::clearLocalSettings()
{
    getLocalSettings() = getUserSettings();
}

template <class T>
T &SettingStorage<T>::get(SettingsType type)
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

template struct SettingStorage<primitives::Settings>;

primitives::SettingStorage<primitives::Settings> &getSettings(primitives::SettingStorage<primitives::Settings> *v)
{
    static StaticValueOrRef<primitives::SettingStorage<primitives::Settings>> settings(v);
    return settings;
}

primitives::Settings &getSettings(SettingsType type, primitives::SettingStorage<primitives::Settings> *v)
{
    return getSettings(v).get(type);
}

}
