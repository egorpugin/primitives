// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/enums.h>
#include <primitives/string.h>

#include <filesystem>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;
using error_code = std::error_code;

using path = fs::path;

#ifdef _WIN32
// on win and C++20 we can use std::u8string for path strings
// to convert paths in both directions safely
#define PRIMITIVES_FS_USE_UTF8_PATH_STRINGS 1
#else
#define PRIMITIVES_FS_USE_UTF8_PATH_STRINGS 0
#endif

#if PRIMITIVES_FS_USE_UTF8_PATH_STRINGS
// on win and C++20 we can use std::u8string for path strings
// to convert paths in both directions safely
using path_u8string = std::u8string;
using path_char_t = char8_t;
#else
using path_u8string = String;
using path_char_t = char;
#endif

using FilesSorted = std::set<path>;
using FilesOrdered = std::vector<path>;
using Files = std::unordered_set<path>;
using FilesMap = std::unordered_map<path, path>;

PRIMITIVES_FILESYSTEM_API
path get_home_directory();

PRIMITIVES_FILESYSTEM_API
path current_thread_path(const path &p = path());

PRIMITIVES_FILESYSTEM_API
String read_file(const path &p, uintmax_t offset = 0, uintmax_t count = UINTMAX_MAX);

PRIMITIVES_FILESYSTEM_API
String read_file_without_bom(const path &p, uintmax_t offset_after_bom = 0, uintmax_t count = UINTMAX_MAX);

PRIMITIVES_FILESYSTEM_API
void write_file(const path &p, const String &);

PRIMITIVES_FILESYSTEM_API
void write_file(const path &p, const std::vector<uint8_t> &);

PRIMITIVES_FILESYSTEM_API
void write_file_if_different(const path &p, const String &s);

PRIMITIVES_FILESYSTEM_API
void write_file_if_not_exists(const path &p, const String &s);

PRIMITIVES_FILESYSTEM_API
void prepend_file(const path &p, const String &s);

PRIMITIVES_FILESYSTEM_API
void append_file(const path &p, const String &s);

PRIMITIVES_FILESYSTEM_API
Strings read_lines(const path &p);

PRIMITIVES_FILESYSTEM_API
void remove_all_from_dir(const path &dir);

/// change all '\\' into '/', make drive letter uppercase
PRIMITIVES_FILESYSTEM_API
path normalize_path(const path &p);

/// change all '/' into '\\', make drive letter uppercase
PRIMITIVES_FILESYSTEM_API
path normalize_path_windows(const path &p);

/// returns utf-8 string
PRIMITIVES_FILESYSTEM_API
path_u8string to_path_string(const path &p);

/// returns utf-8 string in std::string
PRIMITIVES_FILESYSTEM_API
String to_printable_string(const path &p);

// checks for real file
PRIMITIVES_FILESYSTEM_API
bool is_under_root(path existing_path, const path &root_dir);

// checks path that may not exist
PRIMITIVES_FILESYSTEM_API
bool is_under_root_by_prefix_path(const path &p, const path &root_dir);

PRIMITIVES_FILESYSTEM_API
void copy_dir(const path &source, const path &destination);

PRIMITIVES_FILESYSTEM_API
void remove_files(const Files &files);

PRIMITIVES_FILESYSTEM_API
void remove_files_like(const path &dir, const String &regex, bool recursive = true);

PRIMITIVES_FILESYSTEM_API
void remove_files_like(const Files &files, const String &regex);

PRIMITIVES_FILESYSTEM_API
Files enumerate_files(const path &dir, bool recursive = true);

PRIMITIVES_FILESYSTEM_API
Files enumerate_files_like(const path &dir, const String &regex, bool recursive = true);

PRIMITIVES_FILESYSTEM_API
Files enumerate_files_like(const Files &files, const String &regex);

PRIMITIVES_FILESYSTEM_API
Files filter_files_like(const Files &files, const String &regex);

PRIMITIVES_FILESYSTEM_API
bool compare_files(const path &fn1, const path &fn2);

PRIMITIVES_FILESYSTEM_API
bool compare_dirs(const path &dir1, const path &dir2);

PRIMITIVES_FILESYSTEM_API
path unique_path(const path &p = "%%%%-%%%%-%%%%-%%%%");

PRIMITIVES_FILESYSTEM_API
time_t file_time_type2time_t(fs::file_time_type);

PRIMITIVES_FILESYSTEM_API
fs::file_time_type time_t2file_time_type(time_t);

namespace primitives::filesystem
{

PRIMITIVES_FILESYSTEM_API
void remove_file(const path &p);

PRIMITIVES_FILESYSTEM_API
path canonical(const path &p);

}

#if __cplusplus < 202000L
namespace std
{
    template<> struct hash<path>
    {
        size_t operator()(const path& p) const
        {
            return fs::hash_value(p);
        }
    };
}
#endif

enum class CurrentPathScope
{
    Process     = 0x1,
    Thread      = 0x2,

    Max,
    All = Process | Thread,
};
ENABLE_ENUM_CLASS_BITMASK(CurrentPathScope);

class PRIMITIVES_FILESYSTEM_API ScopedCurrentPath
{
public:
    ScopedCurrentPath(CurrentPathScope scope = CurrentPathScope::Process);
    ScopedCurrentPath(const path &p, CurrentPathScope scope = CurrentPathScope::Process);
    ~ScopedCurrentPath();

    void return_back();

    path get_cwd() const;
    path get_old(CurrentPathScope scope = CurrentPathScope::Process) const;

private:
    path old[(int)CurrentPathScope::Max];
    path cwd;
    bool active = true;
    CurrentPathScope ctp = CurrentPathScope::Process;

    void set_cwd();
};

namespace primitives::filesystem
{

PRIMITIVES_FILESYSTEM_API
FILE *fopen(const path &p, const char *mode = "rb");

PRIMITIVES_FILESYSTEM_API
FILE *fopen_checked(const path &p, const char *mode = "rb");

PRIMITIVES_FILESYSTEM_API
void create(const path &p);

}

class PRIMITIVES_FILESYSTEM_API ScopedFile
{
public:
    ScopedFile(const path &p, const char *mode = "rb");
    ScopedFile(const ScopedFile &) = delete;
    ScopedFile &operator=(const ScopedFile &) = delete;
    ScopedFile(ScopedFile &&);
    ScopedFile &operator=(ScopedFile &&) = delete;
    ~ScopedFile();

    FILE *getHandle() const { return f; }

    size_t read(void *buf, size_t sz);
    void seek(uintmax_t offset);
    void close();

private:
    FILE *f = nullptr;
};
