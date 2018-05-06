// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

using String = std::string;
using Strings = std::vector<String>;
using StringMap = std::map<String, String>;
using StringSet = std::set<String>;

using namespace std::literals;

PRIMITIVES_STRING_API
Strings split_string(const String &s, const String &delims);

PRIMITIVES_STRING_API
Strings split_lines(const String &s);

PRIMITIVES_STRING_API
String trim_double_quotes(String s);

#ifdef _WIN32
PRIMITIVES_STRING_API
void normalize_string(String &s);

PRIMITIVES_STRING_API
void normalize_string(std::wstring &s);

PRIMITIVES_STRING_API
String normalize_string_copy(String s);

PRIMITIVES_STRING_API
std::wstring normalize_string_copy(std::wstring s);
#else
#define normalize_string(s) (s)
#define normalize_string_copy(s) (s)
#endif

PRIMITIVES_STRING_API
std::wstring to_wstring(const std::string &s);

PRIMITIVES_STRING_API
std::string to_string(const std::wstring &s);
