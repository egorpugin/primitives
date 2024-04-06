// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <boost/algorithm/string.hpp>

#include <codecvt>
#include <locale>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

using String = std::string;
using Strings = std::vector<String>;
// using StringMap = std::map<String, String>;
using StringSet = std::set<String>;

template <class T>
using StringMap = std::map<String, T>;

template <class T>
using StringHashMap = std::unordered_map<String, T>;

using namespace std::literals;

inline Strings split_string(const String &s, const String &delims, bool allow_empty) {
    std::vector<String> v, lines;
    boost::split(v, s, boost::is_any_of(delims));
    for (auto &l : v) {
        boost::trim(l);
        if (allow_empty || !l.empty())
            lines.push_back(l);
    }
    return lines;
}
inline Strings split_string(const String &s, const String &delims) {
    return split_string(s, delims, false);
}

inline Strings split_lines(const String &s, bool allow_empty) {
    return split_string(s, "\r\n", allow_empty);
}
inline Strings split_lines(const String &s) {
    return split_lines(s, false);
}

inline String trim_double_quotes(String s) {
    boost::trim(s);
    boost::trim_if(s, boost::is_any_of("\""));
    boost::trim(s);
    return s;
}

#ifdef _WIN32
inline void normalize_string(String &s) {
    std::replace(s.begin(), s.end(), '\\', '/');
}

inline void normalize_string(std::u8string &s) {
    std::replace(s.begin(), s.end(), '\\', '/');
}

inline void normalize_string(std::wstring &s) {
    std::replace(s.begin(), s.end(), L'\\', L'/');
}

inline String normalize_string_copy(String s) {
    normalize_string(s);
    return s;
}

inline std::wstring normalize_string_copy(std::wstring s) {
    normalize_string(s);
    return s;
}

inline void normalize_string_windows(String &s) {
    std::replace(s.begin(), s.end(), '/', '\\');
}

inline void normalize_string_windows(std::u8string &s) {
    std::replace(s.begin(), s.end(), '/', '\\');
}

inline void normalize_string_windows(std::wstring &s) {
    std::replace(s.begin(), s.end(), L'/', L'\\');
}

inline String normalize_string_windows_copy(String s) {
    normalize_string_windows(s);
    return s;
}

inline std::wstring normalize_string_windows_copy(std::wstring s) {
    normalize_string_windows(s);
    return s;
}
#else
#define normalize_string(s) (void)(s)
#define normalize_string_copy(s) (s)
#define normalize_string_windows(s) (void)(s)
#define normalize_string_windows_copy(s) (s)
#endif

inline std::u8string to_u8string(const std::string &s) {
    return {s.begin(), s.end()};
}

static auto &get_string_converter() {
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter;
}

inline std::wstring to_wstring(const std::string &s) {
    auto &converter = get_string_converter();
    return converter.from_bytes(s.c_str());
}

inline std::string to_string(const std::string &s) {
    return s;
}

inline std::string to_string(const std::u8string &s) {
    return {s.begin(), s.end()};
}

inline std::string to_string(const std::wstring &s) {
    auto &converter = get_string_converter();
    return converter.to_bytes(s.c_str());
}

inline std::string errno2string(int e) {
    throw std::runtime_error("not implemented");
}
