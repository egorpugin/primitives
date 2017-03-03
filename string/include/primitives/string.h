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

Strings split_string(const String &s, const String &delims);
Strings split_lines(const String &s);
String trim_double_quotes(String s);

#ifdef _WIN32
void normalize_string(String &s);
String normalize_string_copy(String s);
#else
#define normalize_string(s) (s)
#define normalize_string_copy(s) (s)
#endif
