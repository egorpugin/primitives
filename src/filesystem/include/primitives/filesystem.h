// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/string.h>

#include <flags/flags.hpp>

#include <filesystem>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;
using error_code = std::error_code;

using path = fs::path;

using FilesSorted = std::set<path>;
using FilesOrdered = std::vector<path>;
using Files = std::unordered_set<path>;
using FilesMap = std::unordered_map<path, path>;

PRIMITIVES_FILESYSTEM_API
path get_home_directory();

PRIMITIVES_FILESYSTEM_API
path current_thread_path(const path &p = path());

PRIMITIVES_FILESYSTEM_API
String read_file(const path &p, uintmax_t max_size = UINTMAX_MAX);

PRIMITIVES_FILESYSTEM_API
String read_file_bytes(const path &p, uintmax_t offset = 0, uintmax_t count = UINTMAX_MAX);

PRIMITIVES_FILESYSTEM_API
String read_file_from_offset(const path &p, uintmax_t offset, uintmax_t max_size = UINTMAX_MAX);

PRIMITIVES_FILESYSTEM_API
String read_file_without_bom(const path &p, uintmax_t max_size = UINTMAX_MAX);

PRIMITIVES_FILESYSTEM_API
void write_file(const path &p, const String &s);

PRIMITIVES_FILESYSTEM_API
void write_file_if_different(const path &p, const String &s);

PRIMITIVES_FILESYSTEM_API
void prepend_file(const path &p, const String &s);

PRIMITIVES_FILESYSTEM_API
void append_file(const path &p, const String &s);

PRIMITIVES_FILESYSTEM_API
Strings read_lines(const path &p);

PRIMITIVES_FILESYSTEM_API
void remove_file(const path &p);

PRIMITIVES_FILESYSTEM_API
void remove_all_from_dir(const path &dir);

PRIMITIVES_FILESYSTEM_API
String normalize_path(const path &p);

PRIMITIVES_FILESYSTEM_API
String normalize_path_windows(const path &p);

PRIMITIVES_FILESYSTEM_API
std::wstring wnormalize_path(const path &p);

PRIMITIVES_FILESYSTEM_API
std::wstring wnormalize_path_windows(const path &p);

PRIMITIVES_FILESYSTEM_API
bool is_under_root(path p, const path &root_dir);

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

enum class CurrentPathScope
{
    Process     = 0x1,
    Thread      = 0x2,

    Max,
    All = Process | Thread,
};
ALLOW_FLAGS_FOR_ENUM(CurrentPathScope);

class PRIMITIVES_FILESYSTEM_API ScopedCurrentPath
{
public:
    ScopedCurrentPath(CurrentPathScope scope = CurrentPathScope::Process)
        : ctp(scope)
    {
        if (ctp & CurrentPathScope::Thread)
            old[(int)CurrentPathScope::Thread] = current_thread_path();
        if (ctp & CurrentPathScope::Process)
            old[(int)CurrentPathScope::Process] = fs::current_path();

        set_cwd();
    }
    ScopedCurrentPath(const path &p, CurrentPathScope scope = CurrentPathScope::Process)
        : ScopedCurrentPath(scope)
    {
        if (!p.empty())
        {
            if (ctp & CurrentPathScope::Thread)
                current_thread_path(p);
            if (ctp & CurrentPathScope::Process)
                fs::current_path(p);

            set_cwd();
        }
    }
    ~ScopedCurrentPath()
    {
        return_back();
    }

    void return_back()
    {
        if (!active)
            return;

        if (ctp & CurrentPathScope::Thread)
            current_thread_path(cwd = old[(int)CurrentPathScope::Thread]);
        if (ctp & CurrentPathScope::Process)
            fs::current_path(cwd = old[(int)CurrentPathScope::Process]);

        active = false;
    }

    path get_cwd() const { return cwd; }
    path get_old(CurrentPathScope scope = CurrentPathScope::Process) const { return old[(int)scope]; }

private:
    path old[(int)CurrentPathScope::Max];
    path cwd;
    bool active = true;
    CurrentPathScope ctp = CurrentPathScope::Process;

    void set_cwd()
    {
        // abs path, not possibly relative p
        if (ctp & CurrentPathScope::Thread)
            cwd = current_thread_path();
        if (ctp & CurrentPathScope::Process)
            cwd = fs::current_path();
    }
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
    ~ScopedFile();

    FILE *getHandle() const { return f; }

    size_t read(void *buf, size_t sz);
    void seek(uintmax_t offset);
    void close();

private:
    FILE *f = nullptr;
};
