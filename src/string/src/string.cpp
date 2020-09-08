// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/string.h>

#include <boost/algorithm/string.hpp>

#include <codecvt>
#include <locale>

Strings split_string(const String &s, const String &delims)
{
    std::vector<String> v, lines;
    boost::split(v, s, boost::is_any_of(delims));
    for (auto &l : v)
    {
        boost::trim(l);
        if (!l.empty())
            lines.push_back(l);
    }
    return lines;
}

Strings split_lines(const String &s)
{
    return split_string(s, "\r\n");
}

#ifdef _WIN32
void normalize_string(String &s)
{
    std::replace(s.begin(), s.end(), '\\', '/');
}

void normalize_string(std::u8string &s)
{
    std::replace(s.begin(), s.end(), '\\', '/');
}

void normalize_string(std::wstring &s)
{
    std::replace(s.begin(), s.end(), L'\\', L'/');
}

String normalize_string_copy(String s)
{
    normalize_string(s);
    return s;
}

std::wstring normalize_string_copy(std::wstring s)
{
    normalize_string(s);
    return s;
}

void normalize_string_windows(String &s)
{
    std::replace(s.begin(), s.end(), '/', '\\');
}

void normalize_string_windows(std::u8string &s)
{
    std::replace(s.begin(), s.end(), '/', '\\');
}

void normalize_string_windows(std::wstring &s)
{
    std::replace(s.begin(), s.end(), L'/', L'\\');
}

String normalize_string_windows_copy(String s)
{
    normalize_string_windows(s);
    return s;
}

std::wstring normalize_string_windows_copy(std::wstring s)
{
    normalize_string_windows(s);
    return s;
}
#endif

String trim_double_quotes(String s)
{
    boost::trim(s);
    while (!s.empty())
    {
        if (s.front() == '"')
        {
            s = s.substr(1);
            continue;
        }
        if (s.back() == '"')
        {
            s.resize(s.size() - 1);
            continue;
        }
        break;
    }
    boost::trim(s);
    return s;
}

static auto& get_string_converter()
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter;
}

std::wstring to_wstring(const std::string &s)
{
    auto &converter = get_string_converter();
    return converter.from_bytes(s.c_str());
}

std::string to_string(const std::u8string &s)
{
    return {s.begin(), s.end()};
}

std::string to_string(const std::wstring &s)
{
    auto &converter = get_string_converter();
    return converter.to_bytes(s.c_str());
}

std::string errno2string(int e)
{
    throw std::runtime_error("not implemented");
}
