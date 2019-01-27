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

#include <mutex>

using Args = std::list<std::string>;

namespace primitives
{

using Args = std::list<std::string>;

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

auto &getParserStorage()
{
    static std::map<size_t, std::unique_ptr<parser_base>> parsers;
    return parsers;
}

parser_base *getParser(const std::type_info &t)
{
    auto &parsers = getParserStorage();
    auto i = parsers.find(t.hash_code());
    if (i == parsers.end())
        return nullptr;
    return i->second.get();
}

parser_base *addParser(const std::type_info &t, std::unique_ptr<parser_base> p)
{
    static std::mutex m;
    std::unique_lock lk(m);
    auto &parsers = getParserStorage();
    auto ptr = p.get();
    parsers[t.hash_code()] = std::move(p);
    return ptr;
}

} // namespace detail

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
        throw SW_RUNTIME_ERROR("While parsing setting path '" + s + "':" + d.bb.error_msg);
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
            throw SW_RUNTIME_ERROR(d.bb.error_msg);
        settings = d.sm;
    }
}

void Settings::load(const String &s, SettingsType type)
{
    SettingsParserDriver d;
    auto r = d.parse(s);
    if (!d.bb.error_msg.empty())
        throw SW_RUNTIME_ERROR(d.bb.error_msg);
    settings = d.sm;
}

void Settings::load(const yaml &root, const SettingPath &prefix)
{
    //if (root.IsNull() || root.IsScalar())
        //return;
    if (!root.IsMap())
        return;
        //throw SW_RUNTIME_ERROR("Config must be a map");

    for (auto kv : root)
    {
        if (kv.second.IsScalar())
            operator[](prefix / SettingPath::fromRawString(kv.first.as<String>())).representation = kv.second.as<String>();
        else if (kv.second.IsMap())
            load(kv.second, prefix / SettingPath::fromRawString(kv.first.as<String>()));
        else
            throw SW_RUNTIME_ERROR("sequences are not supported");
    }
}

String Setting::toString() const
{
    if (!representation.empty())
        return representation;
    return parser->toString(value);
}

bool Settings::hasSaveableItems() const
{
    int n_saveable = 0;
    for (auto &[k, v] : settings)
    {
        if (v.saveable)
            n_saveable++;
    }
    return n_saveable != 0;
}

void Settings::save(const path &fn) const
{
    if (!hasSaveableItems())
        return;

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
    throw SW_RUNTIME_ERROR("not implemented");
    return s;
}

void Settings::dump(yaml &root) const
{
    for (auto &[k, v] : settings)
    {
        if (!v.saveable)
            continue;
        auto node = root;
        for (auto &e : k)
            node = node[e];
        node = v.toString();
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
        throw SW_RUNTIME_ERROR("No such setting: " + k);
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
        throw SW_RUNTIME_ERROR("No such setting: " + k.toString());
    return i->second;
}

template <class T>
SettingStorage<T>::SettingStorage()
{
    // to overlive setting storage
    detail::getParserStorage();
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
        throw SW_RUNTIME_ERROR("No such settings storage");

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
                    auto ss = get(SettingsType::System);
                    if (ss.hasSaveableItems())
                    {
                        error_code ec;
                        fs::create_directories(fn.parent_path(), ec);
                        if (ec)
                            throw SW_RUNTIME_ERROR(ec.message());
                        ss.save(fn);
                    }
                }
                else
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

// every os
template struct SettingStorage<primitives::Settings>;

primitives::SettingStorage<primitives::Settings> &getSettingStorage(primitives::SettingStorage<primitives::Settings> *v)
{
    static StaticValueOrRef<primitives::SettingStorage<primitives::Settings>> settings(v);
    return settings;
}

primitives::Settings &getSettings(SettingsType type, primitives::SettingStorage<primitives::Settings> *v)
{
    return getSettingStorage(v).get(type);
}

}
